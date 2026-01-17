package broker

import (
	"context"
	"fmt"
	"strings"
	"sync"

	"archive_manager/internal/config"
	"archive_manager/internal/model"
	pb "archive_manager/internal/protobuf"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kgo"
	"google.golang.org/protobuf/proto"
)

var (
	Cameras    sync.Map // map[string]pb.ProtoCamera -> map[StreamUUID]CameraProperties
	Retentions sync.Map // map[string]int -> map[CameraUUID]RetentionDays

	ArchiveTopics map[string]chan<- model.Frame // map[StreamUUID]chan<- model.Frame
	// ArchiveTopics   map[string]chan<- *kgo.Record // map[StreamUUID]chan<- *kgo.Record
	ArchiveTopicsMu sync.RWMutex
)

func startConsuming(wg *sync.WaitGroup) {
	defer wg.Done()

	log.Info.Println("Kafka consumer started")
	defer log.Info.Println("Kafka consumer stopped")

	ArchiveTopics = make(map[string]chan<- model.Frame)
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
					camera := &pb.ProtoCamera{}
					if err := proto.Unmarshal(record.Value, camera); err != nil {
						log.Error.Printf("failed to unmarshal camera protobuf for key %v: %v", record.Key, err)
						continue
					}

					Retentions.Store(camera.CameraUuid, camera.ArchiveRetentionDays)
					Cameras.Store(string(record.Key), record.Value)
				default:
					ArchiveTopicsMu.RLock()
					consumerChan, exists := ArchiveTopics[record.Topic]
					ArchiveTopicsMu.RUnlock()
					if exists {
						sendFrameToArchive(record, consumerChan)
					} else {
						log.Warning.Printf("received message for unknown topic '%s'\n", record.Topic)
					}
				}
			}
		}
	}
}

func sendFrameToArchive(record *kgo.Record, consumerChan chan<- model.Frame) {
	headers := make(map[string]string)
	for _, header := range record.Headers {
		headers[header.Key] = string(header.Value)
	}

	frame := model.Frame{
		CameraID:     strings.Split(string(record.Topic), "_")[0],
		Timestamp:    record.Timestamp.UnixMilli(),
		Data:         record.Value,
		IsVideoFrame: string(record.Key) == "video",
		IsIFrame:     headers["key_frame"] == "1",
	}

	consumerChan <- frame

	// log.Debug.Printf("Sent frame to archive: CameraID=%s, Timestamp=%d, IsIFrame=%v\n",
	// 	frame.CameraID, frame.Timestamp, frame.IsIFrame)
}

func AddArchiveTopic(topic string, consumerChan chan<- model.Frame) error {
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

func RemoveArchiveTopic(topic string) error {
	ArchiveTopicsMu.Lock()
	defer ArchiveTopicsMu.Unlock()

	if _, exists := ArchiveTopics[topic]; !exists {
		return fmt.Errorf("topic %s does not exist", topic)
	}

	Client.PurgeTopicsFromConsuming(topic)

	delete(ArchiveTopics, topic)

	log.Info.Printf("Removed topic %s from consumer\n", topic)

	return nil
}
