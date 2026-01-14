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
	outputPath          string
	currentFile         *os.File
	currentOffset       int64
	fileStartTime       time.Time
	fragmentLength      time.Duration
	fileFrameCount      int64
	lastIFrameTimestamp int64
	lastFileFrameCount  int64
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

func (w *Writer) processFrame(frame model.Frame, offsetsChan chan<- model.BatchMetadata) error {
	if len(frame.Data) == 0 {
		return nil // Skip empty frames
	}

	// Create file if needed (first frame or no file)
	if w.currentFile == nil {
		if err := w.rotateFile(frame.CameraID, frame.Timestamp); err != nil {
			return fmt.Errorf("failed to create initial file: %w", err)
		}
	}

	// Rotate file if I-frame and time-based rotation is needed
	if frame.IsIFrame && w.shouldRotateFile() {
		// Send final metadata with total frame count for the file being closed
		if w.lastIFrameTimestamp > 0 && w.fileFrameCount > 0 {
			offset := model.BatchMetadata{
				CameraID:               frame.CameraID,
				IFrameTimestamp:        w.lastIFrameTimestamp,
				IFrameOffset:           0,
				IFramePacketNumInBatch: 0,
				TotalPackets:           w.lastFileFrameCount,
			}

			select {
			case offsetsChan <- offset:
			default:
				log.Warning.Println("Offset channel full, dropping offset")
			}
		}

		if err := w.rotateFile(frame.CameraID, frame.Timestamp); err != nil {
			return fmt.Errorf("failed to rotate file: %w", err)
		}
	}

	// Write frame to file BEFORE counting
	n, err := w.currentFile.Write(frame.Data)
	if err != nil {
		return fmt.Errorf("failed to write frame: %w", err)
	}

	// If I-frame, send offset to database writer with current offset (before updating)
	if frame.IsIFrame {
		w.lastIFrameTimestamp = frame.Timestamp

		offset := model.BatchMetadata{
			CameraID:               frame.CameraID,
			IFrameTimestamp:        frame.Timestamp,
			IFrameOffset:           w.currentOffset,
			IFramePacketNumInBatch: w.fileFrameCount, // Count of frames BEFORE this I-frame
			TotalPackets:           0,                // Will be updated when file is closed
		}

		w.lastFileFrameCount = w.fileFrameCount

		select {
		case offsetsChan <- offset:
		default:
			log.Warning.Println("Offset channel full, dropping offset")
		}
	}

	// Update counters AFTER writing and sending offset
	w.currentOffset += int64(n)
	if frame.IsVideoFrame {
		w.fileFrameCount++
	}
	return nil
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
	w.closeCurrentFile()

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
	w.currentOffset = 0
	w.fileStartTime = time.Unix(timestamp-(timestamp%int64(w.fragmentLength.Seconds())), 0)
	w.fileFrameCount = 0
	w.lastIFrameTimestamp = 0
	w.lastFileFrameCount = 0

	log.Info.Printf("Created new archive file: %s", filepath)
	return nil
}

func (w *Writer) closeCurrentFile() {
	if w.currentFile != nil {
		w.currentFile.Close()
		w.currentFile = nil
	}
}
