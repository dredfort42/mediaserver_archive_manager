package controller

import (
	"context"
	"sync"
	"time"

	"archive_manager/internal/archive"
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
			for _, topic := range broker.Client.GetConsumeTopics() {
				log.Debug.Printf("Consuming topic: %s\n", topic)
			}

			broker.Cameras.Range(func(key, value any) bool {
				cameraID := key.(string)
				cameraData := value.([]byte)

				camera := &pb.ProtoCamera{}
				err := proto.Unmarshal(cameraData, camera)
				if err != nil {
					log.Error.Printf("Failed to unmarshal camera data for stream ID %s: %v\n", cameraID, err)
					return true
				}

				if camera.GetStatusCode() == CameraOFFStatus || camera.GetArchiveRetentionDays() == 0 {
					archiveCancel, exists := controlledArchiveStreams.Load(camera.GetCameraUuid())
					if !exists {
						return true
					}

					archiveCancel.(context.CancelFunc)()
					controlledArchiveStreams.Delete(camera.GetCameraUuid())
					
					// Clean up camera data from broker map to prevent memory leak
					broker.Cameras.Delete(camera.GetCameraUuid())

					log.Debug.Printf("Archive for camera %s is OFF\n", camera.GetCameraUuid())
				} else {
					_, exists := controlledArchiveStreams.Load(camera.GetCameraUuid())
					if exists {
						return true
					}

					archiveCtx, archiveCancel := context.WithCancel(ctx)
					controlledArchiveStreams.Store(camera.GetCameraUuid(), archiveCancel)

					pipeline := archive.NewPipeline(camera.GetCameraUuid(), camera.GetCameraUuid()+MainStreamSuffix)

					wg.Go(func() {
						err := pipeline.Start(archiveCtx)
						if err != nil {
							log.Error.Printf("Failed to start archive pipeline for camera %s: %v\n", camera.GetCameraUuid(), err)
							controlledArchiveStreams.Delete(camera.GetCameraUuid())
							broker.RemoveArchiveTopic(camera.GetCameraUuid() + MainStreamSuffix)
						}
					})

					log.Debug.Printf("Archive for camera %s is ON\n", camera.GetCameraUuid())
				}

				pb.PrintProtoStruct(camera, nil)

				return true
			})
		}
	}
}
