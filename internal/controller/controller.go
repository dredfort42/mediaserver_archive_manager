package controller

import (
	"context"
	"sync"
	"time"

	"archive_manager/internal/broker"
	pb "archive_manager/internal/protobuf"

	log "github.com/dredfort42/go_logger"
	"google.golang.org/protobuf/proto"
)

const (
	CameraOFFStatus = 0
	CameraONStatus  = 1

	MainStreamSuffix = "_main"
)

func controller(ctx context.Context, wg *sync.WaitGroup, controlledArchiveStreams *sync.Map) {
	defer wg.Done()

	log.Info.Println("Controller started")
	defer log.Info.Println("Controller stopped")

	tiker := time.NewTicker(CheckInterval)
	defer tiker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-tiker.C:
			broker.Cameras.Range(func(key, value any) bool {
				cameraID := key.(string)
				cameraData := value.([]byte)

				camera := &pb.ProtoCamera{}
				err := proto.Unmarshal(cameraData, camera)
				if err != nil {
					log.Error.Printf("Failed to unmarshal camera data for stream ID %s: %v\n", cameraID, err)
					return true
				}

				if camera.GetStatusCode() == CameraOFFStatus {
					log.Debug.Printf("Camera %s is OFF\n", camera.GetCameraUuid())

					broker.Cameras.Delete(cameraID)
					controlledArchiveStreams.Delete(camera.GetCameraUuid())
				} else {
					controlledArchiveStreams.Store(camera.GetCameraUuid(), nil)
				}

				pb.PrintProtoStruct(camera, nil)

				return true
			})
		}
	}
}
