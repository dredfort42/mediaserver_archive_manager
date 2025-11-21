package broker

import (
	"context"
	"fmt"
	"sync"

	"archive_manager/internal/config"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kgo"
)

var (
	Cameras sync.Map // map[string]pb.ProtoCamera -> map[StreamUUID]CameraProperties

	ArchiveTopics   map[string]chan<- *kgo.Record // map[StreamUUID]chan<- *kgo.Record
	ArchiveTopicsMu sync.RWMutex
)

func startConsuming(wg *sync.WaitGroup) {
	defer wg.Done()

	log.Info.Println("Kafka consumer started")
	defer log.Info.Println("Kafka consumer stopped")

	// defer close(StreamConsumers)

	for {
		select {
		case <-brokerCtx.Done():
			return
		default:
			fetches := Client.PollFetches(brokerCtx)
			iter := fetches.RecordIter()

			for _, fetchErr := range fetches.Errors() {
				if fetchErr.Err != context.Canceled {
					log.Error.Printf("failed to fetch records from broker: %v\n", fetchErr.Err)
				}
			}

			for !iter.Done() {
				record := iter.Next()

				if record == nil {
					continue
				}

				switch record.Topic {
				case config.App.Kafka.TopicCameras:
					Cameras.Store(string(record.Key), record.Value)
				default:
					log.Warning.Printf("received message for unknown topic '%s'\n", record.Topic)
				}
			}
		}
	}
}

func AddTopic(topic string, consumerChan chan<- *kgo.Record) error {
	ArchiveTopicsMu.Lock()
	defer ArchiveTopicsMu.Unlock()

	if _, exists := ArchiveTopics[topic]; exists {
		return fmt.Errorf("topic %s already exists", topic)
	}

	ArchiveTopics[topic] = consumerChan

	Client.AddConsumeTopics(topic)

	log.Info.Printf("Added topic %s to consumer\n", topic)

	return nil
}

func RemoveTopic(topic string) error {
	ArchiveTopicsMu.Lock()
	defer ArchiveTopicsMu.Unlock()

	if _, exists := ArchiveTopics[topic]; !exists {
		return fmt.Errorf("topic %s does not exist", topic)
	}

	delete(ArchiveTopics, topic)

	Client.PurgeTopicsFromConsuming(topic)

	log.Info.Printf("Removed topic %s from consumer\n", topic)

	return nil
}
