package broker

import (
	"sync"
	"time"

	"archive_manager/internal/config"
	pb "archive_manager/internal/protobuf"

	log "github.com/dredfort42/go_logger"
	"google.golang.org/protobuf/proto"
)

type ServiceStatus uint32

// Service state codes
const (
	ServiceStateUnknown     ServiceStatus = 100
	ServiceStateStarting    ServiceStatus = 101
	ServiceStateReady       ServiceStatus = 102
	ServiceStateDegraded    ServiceStatus = 103
	ServiceStateMaintenance ServiceStatus = 104
	ServiceStateStopping    ServiceStatus = 105
	ServiceStateStopped     ServiceStatus = 106
	ServiceStateError       ServiceStatus = 107
)

// ServiceDigestHeartbeat sends periodic service digest heartbeats to the broker
func ServiceDigestHeartbeat(wg *sync.WaitGroup) error {
	defer wg.Done()

	log.Info.Println("Service digest heartbeat started")
	defer log.Info.Println("Service digest heartbeat stopped")

	ticker := time.NewTicker(config.App.HeartbeatInterval)
	defer ticker.Stop()

	for {
		select {
		case <-brokerCtx.Done():
			return nil
		case <-ticker.C:
			SendServiceDigest(ServiceStateReady)
		}
	}
}

// SendServiceDigest sends a service digest message to the broker
func SendServiceDigest(status ServiceStatus) error {
	digest := createServiceDigest(status)

	data, err := proto.Marshal(digest)
	if err != nil {
		log.Error.Printf("failed to marshal service digest: %v\n", err)
		return err
	}

	msg := &Message{
		Topic: config.App.Kafka.TopicSystemDigest,
		Key:   digest.ServiceUuid,
		Value: string(data),
	}

	select {
	case <-brokerCtx.Done():
		return nil
	case MsgChan <- msg:
	}

	return nil
}

func createServiceDigest(status ServiceStatus) *pb.ProtoServiceDigest {
	digest := &pb.ProtoServiceDigest{
		ServiceUuid:       config.App.UUID,
		ServiceName:       config.App.AppName,
		ServiceVersion:    config.App.Version,
		ServiceStatusCode: uint32(status),
		Endpoints:         []string{},
		LastHeartbeat:     uint64(time.Now().UnixMilli()),
	}

	return digest
}
