package archive

import (
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

	// Current file state
	currentFile             *os.File
	currentCameraID         string
	currentFileTimestamp    int64
	currentIFrameByteOffset int64
	fileStartTime           time.Time
	totalVideoFramesInFile  int64
}

func NewWriter(outputPath string, fragmentLength time.Duration) *Writer {
	return &Writer{
		outputPath:     outputPath,
		fragmentLength: fragmentLength,
	}
}

func (w *Writer) Run(ctx context.Context, framesChan <-chan model.Frame, offsetsChan chan<- model.BatchMetadata) {
	defer func() {
		if w.currentFile != nil {
			w.sendFinalMetadata(offsetsChan)
		}
		w.closeCurrentFile()
	}()

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

func (w *Writer) processFrame(frame model.Frame, offsetsChan chan<- model.BatchMetadata) error {
	if len(frame.Data) == 0 {
		return nil
	}

	if w.currentFile == nil || (frame.IsIFrame && w.shouldRotateFile()) {
		// Send final total_packets for previous file before rotating
		if w.currentFile != nil {
			w.sendFinalMetadata(offsetsChan)
		}
		
		if err := w.rotateFile(frame.CameraID, frame.Timestamp); err != nil {
			return fmt.Errorf("failed to rotate file: %w", err)
		}
	}

	// Write frame to file
	n, err := w.currentFile.Write(frame.Data)
	if err != nil {
		return fmt.Errorf("failed to write frame: %w", err)
	}

	if frame.IsVideoFrame {
		w.totalVideoFramesInFile++
	}

	// If I-frame, send offset to database writer
	if frame.IsIFrame {
		offset := model.BatchMetadata{
			CameraID:               frame.CameraID,
			IFrameTimestamp:        frame.Timestamp,
			IFrameOffset:           w.currentIFrameByteOffset,
			IFramePacketNumInBatch: w.totalVideoFramesInFile,
			TotalPackets:           0, // Not complete yet
		}

		select {
		case offsetsChan <- offset:
		default:
			log.Warning.Println("Offset channel full, dropping offset")
		}
	}

	// Update byte offset after writing
	w.currentIFrameByteOffset += int64(n)

	return nil
}

func (w *Writer) sendFinalMetadata(offsetsChan chan<- model.BatchMetadata) {
	if w.currentCameraID == "" {
		return
	}

	finalMetadata := model.BatchMetadata{
		CameraID:               w.currentCameraID,
		IFrameTimestamp:        w.currentFileTimestamp,
		IFrameOffset:           0,
		IFramePacketNumInBatch: 0,
		TotalPackets:           w.totalVideoFramesInFile,
	}

	select {
	case offsetsChan <- finalMetadata:
		log.Debug.Printf("Sent final metadata for file: total_packets=%d", w.totalVideoFramesInFile)
	default:
		log.Warning.Println("Offset channel full, dropping final metadata")
	}
}

func (w *Writer) shouldRotateFile() bool {
	if w.currentFile == nil {
		return true
	}

	if time.Since(w.fileStartTime) >= w.fragmentLength {
		return true
	}

	return false
}

func (w *Writer) rotateFile(cameraID string, timestamp int64) error {
	folder := timestamp / 86400 // Daily folders
	folderPath := filepath.Join(w.outputPath, cameraID, fmt.Sprintf("%d", folder))

	if err := os.MkdirAll(folderPath, 0755); err != nil {
		return fmt.Errorf("failed to create folder %s: %w", folderPath, err)
	}

	secondsSinceMidnight := timestamp % 86400
	filename := fmt.Sprintf("%d.bin", secondsSinceMidnight-(secondsSinceMidnight%int64(w.fragmentLength.Seconds())))
	filepath := filepath.Join(folderPath, filename)

	file, err := os.Create(filepath)
	if err != nil {
		return err
	}

	w.currentFile = file
	w.currentCameraID = cameraID
	w.currentFileTimestamp = timestamp
	w.currentIFrameByteOffset = 0
	w.fileStartTime = time.Unix(timestamp-(timestamp%int64(w.fragmentLength.Seconds())), 0)
	w.totalVideoFramesInFile = 0

	log.Info.Printf("Created new archive file: %s", filepath)
	return nil
}

func (w *Writer) closeCurrentFile() {
	if w.currentFile != nil {
		w.currentFile.Close()
		w.currentFile = nil
	}
}
