package archive

import (
	db "archive_manager/internal/db"
	"archive_manager/internal/model"
	"context"
	"fmt"
	"os"
	"path/filepath"
	"time"

	log "github.com/dredfort42/go_logger"
)

type Writer struct {
	outputPath     string
	fragmentLength time.Duration

	// File state
	cameraID         string
	file             *os.File
	fileStartTime    time.Time
	iFrameTimeCode   int64
	iFrameByteOffset int64
	videoFrameCount  int64
}

func NewWriter(outputPath string, fragmentLength time.Duration) *Writer {
	return &Writer{
		outputPath:     outputPath,
		fragmentLength: fragmentLength,
	}
}

func (w *Writer) Run(ctx context.Context, framesChan <-chan model.Frame, offsetsChan chan<- model.BatchMetadata) {
	defer w.closeCurrentFile()

	for {
		select {
		case <-ctx.Done():
			log.Info.Println("Writer stopping...")
			return

		case frame, ok := <-framesChan:
			if !ok {
				log.Info.Println("Frames channel closed")
				return
			}

			if err := w.processFrame(frame, offsetsChan); err != nil {
				log.Error.Printf("Error processing frame: %v", err)
			}
		}
	}
}

func (w *Writer) closeCurrentFile() {
	if w.file != nil {
		w.file.Close()
		w.file = nil
	}
}

func (w *Writer) processFrame(frame model.Frame, offsetsChan chan<- model.BatchMetadata) error {
	if len(frame.Data) == 0 {
		return nil
	}

	w.cameraID = frame.CameraID

	// Only create/rotate files on I-frames to ensure first I-frame has offset 0
	if w.file == nil {
		if !frame.IsIFrame {
			return nil // Skip non-I-frames when no file exists
		}
		if err := w.rotateFile(frame.Timestamp); err != nil {
			return fmt.Errorf("failed to rotate file: %w", err)
		}
	} else if frame.IsIFrame && w.isFileRotationNeeded() {
		if err := w.rotateFile(frame.Timestamp); err != nil {
			return fmt.Errorf("failed to rotate file: %w", err)
		}
	}

	// Write frame to file
	bytesWritten, err := w.file.Write(frame.Data)
	if err != nil {
		return fmt.Errorf("failed to write frame: %w", err)
	}

	if frame.IsVideoFrame {
		w.videoFrameCount++
	}

	// If I-frame, send offset to database writer
	if frame.IsIFrame {
		offset := model.BatchMetadata{
			CameraID:           frame.CameraID,
			IFrameTimeCode:     frame.Timestamp,
			IFrameOffset:       w.iFrameByteOffset,
			VideoFramesInBatch: w.videoFrameCount,
		}

		select {
		case offsetsChan <- offset:
		default:
			log.Warning.Println("Offset channel full, dropping offset")
		}
	}

	// Update byte offset after writing
	w.iFrameByteOffset += int64(bytesWritten)

	return nil
}

func (w *Writer) isFileRotationNeeded() bool {
	return (w.file == nil || time.Since(w.fileStartTime) >= w.fragmentLength)
}

func (w *Writer) rotateFile(timestamp int64) error {
	folderInt := db.GetFolderName(timestamp)
	folderPath := filepath.Join(w.outputPath, w.cameraID, fmt.Sprintf("%d", folderInt))

	if err := os.MkdirAll(folderPath, 0755); err != nil {
		return fmt.Errorf("failed to create folder %s: %w", folderPath, err)
	}

	fileInt := db.GetFileName(timestamp, w.fragmentLength)
	filename := fmt.Sprintf("%d.bin", fileInt)
	filepath := filepath.Join(folderPath, filename)

	file, err := os.Create(filepath)
	if err != nil {
		return err
	}

	w.closeCurrentFile()

	w.file = file
	w.fileStartTime = time.Unix(folderInt*86400+fileInt, 0)
	w.iFrameTimeCode = timestamp
	w.iFrameByteOffset = 0
	w.videoFrameCount = 0

	log.Debug.Printf("Created new archive file: %s", filepath)

	return nil
}

// func (w *Writer) sendFinalMetadata(offsetsChan chan<- model.BatchMetadata) {
// 	if w.cameraID == "" {
// 		return
// 	}

// 	finalMetadata := model.BatchMetadata{
// 		CameraID:               w.cameraID,
// 		IFrameTimestamp:        w.iFrameTimeCode,
// 		IFrameOffset:           0,
// 		IFramePacketNumInBatch: 0,
// 		TotalPackets:           w.videoFrameCount,
// 	}

// 	select {
// 	case offsetsChan <- finalMetadata:
// 		log.Debug.Printf("Sent final metadata for file: total_packets=%d", w.videoFrameCount)
// 	default:
// 		log.Warning.Println("Offset channel full, dropping final metadata")
// 	}
// }
