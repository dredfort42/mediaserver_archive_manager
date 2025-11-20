package broker

import (
	"context"
	"fmt"
	"strings"
	"sync"
	"time"

	"archive_manager/internal/config"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kgo"
	"github.com/twmb/franz-go/pkg/kmsg"
)

const (
	MaxBufferedRecords    = 100000
	ProducerBatchMaxBytes = 1048576 // 1MB batches
	RecordRetries         = 5
	MessageChanBufferSize = 1000
	StartStopTimeout      = 5 * time.Second
)

// Message is a struct for message
type Message struct {
	Topic string
	Key   string
	Value string
}

var brokerCtx context.Context
var Client *kgo.Client
var MsgChan chan *Message

func Init(ctx context.Context, cancel context.CancelFunc, wg *sync.WaitGroup) (err error) {
	brokerCtx = ctx
	opts := []kgo.Opt{
		kgo.SeedBrokers(config.App.Kafka.Brokers...),
		kgo.TransactionalID(config.App.AppName + "_producer_" + config.App.UUID),
		kgo.MaxBufferedRecords(MaxBufferedRecords),       // tune for high throughput
		kgo.ProducerBatchMaxBytes(ProducerBatchMaxBytes), // 1MB batches
		kgo.RecordRetries(RecordRetries),
		kgo.FetchIsolationLevel(kgo.ReadCommitted()), // only read messages that have been written as part of committed transactions
		kgo.ConsumerGroup(config.App.AppName + "_group_" + config.App.UUID),
		kgo.ClientID(config.App.AppName + "_" + config.App.UUID),
	}

	acks := config.App.Kafka.ProducerConfig["acks"].(string)
	acks = strings.ToLower(acks)

	switch acks {
	case "all":
		opts = append(opts, kgo.RequiredAcks(kgo.AllISRAcks()))
	case "leader":
		opts = append(opts, kgo.RequiredAcks(kgo.LeaderAck()))
	case "none":
		opts = append(opts, kgo.RequiredAcks(kgo.NoAck()))
	default:
		log.Warning.Printf("unrecognized acks value %q, defaulting to 'all'\n", acks)
		opts = append(opts, kgo.RequiredAcks(kgo.AllISRAcks()))
	}

	offsetReset := config.App.Kafka.ConsumerConfig["auto.offset.reset"].(string)
	offsetReset = strings.ToLower(offsetReset)

	switch offsetReset {
	case "earliest":
		opts = append(opts, kgo.ConsumeResetOffset(kgo.NewOffset().AtStart()))
	case "latest":
		opts = append(opts, kgo.ConsumeResetOffset(kgo.NewOffset().AtEnd()))
	default:
		log.Warning.Printf("unrecognized auto.offset.reset value %q, defaulting to 'latest'\n", offsetReset)
		opts = append(opts, kgo.ConsumeResetOffset(kgo.NewOffset().AtEnd()))
	}

	// Topics to consume from
	var consumeTopicsList []string
	consumeTopicsList = append(consumeTopicsList, config.App.Kafka.TopicCameras)

	if len(consumeTopicsList) > 0 {
		opts = append(opts, kgo.ConsumeTopics(consumeTopicsList...))
	}

	Client, err = kgo.NewClient(opts...)
	if err != nil {
		return fmt.Errorf("failed to create Kafka client: %w", err)
	}

	log.Info.Println("Connecting to Kafka...")
	if err = validateBrokerConnection(); err != nil {
		return err
	}

	log.Info.Println("Kafka connection established successfully")

	for _, topic := range consumeTopicsList {
		if err = checkTopicExists(topic); err != nil {
			return fmt.Errorf("failed to verify existence of topic '%s': %w", topic, err)
		}
	}

	// Topics to produce to
	var producerTopicsList []string
	producerTopicsList = append(producerTopicsList, config.App.Kafka.TopicSystemDigest)

	for _, topic := range producerTopicsList {
		if err = checkTopicExists(topic); err != nil {
			return fmt.Errorf("failed to verify existence of topic '%s': %w", topic, err)
		}
	}

	MsgChan = make(chan *Message, MessageChanBufferSize)
	StreamConsumers = make(chan *kgo.Record, MessageChanBufferSize)

	wg.Add(1)
	go startConsuming(wg)

	wg.Add(1)
	go startProducing(wg, MsgChan)

	SendServiceDigest(ServiceStateStarting)
	time.Sleep(StartStopTimeout)

	return nil
}

func Close() {
	SendServiceDigest(ServiceStateStopping)
	time.Sleep(StartStopTimeout)

	if Client != nil {
		Client.Close()
	}

	if MsgChan != nil {
		close(MsgChan)
	}

	log.Info.Println("Kafka connection closed")
}

func validateBrokerConnection() error {
	req := kmsg.NewPtrMetadataRequest()
	_, err := req.RequestWith(brokerCtx, Client)
	if err != nil {
		return fmt.Errorf("broker connectivity test failed: %w", err)
	}
	return nil
}
