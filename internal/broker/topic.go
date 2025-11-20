package broker

import (
	"archive_manager/internal/config"
	"fmt"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kmsg"
)

func checkTopicExists(topic string) error {
	req := kmsg.NewPtrMetadataRequest()
	req.Topics = append(req.Topics, kmsg.MetadataRequestTopic{Topic: &topic})

	resp, err := req.RequestWith(brokerCtx, Client)
	if err != nil {
		return fmt.Errorf("failed to retrieve metadata for topic '%v' from brokers %v: %w",
			topic, config.App.Kafka.Brokers, err)
	}

	topicMap := make(map[string]bool)
	for _, t := range resp.Topics {
		if t.ErrorCode == 0 {
			log.Info.Printf("topic '%s' exists and is accessible", *t.Topic)
			topicMap[*t.Topic] = true
		} else {
			return fmt.Errorf("topic '%s' exists but has error (code %d): %s",
				*t.Topic, t.ErrorCode, getKafkaErrorMessage(t.ErrorCode))
		}
	}

	if !topicMap[topic] {
		return fmt.Errorf("topic '%s' does not exist on brokers %v", topic, config.App.Kafka.Brokers)
	}

	return nil
}
