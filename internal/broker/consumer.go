package broker

import (
	"context"
	"sync"

	"archive_manager/internal/config"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kgo"
)

var (
	Cameras         sync.Map // map[string]pb.ProtoCamera -> map[StreamUUID]CameraProperties
	StreamConsumers chan *kgo.Record
)

func startConsuming(wg *sync.WaitGroup) {
	defer wg.Done()

	log.Info.Println("Kafka consumer started")
	defer log.Info.Println("Kafka consumer stopped")

	defer close(StreamConsumers)

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
