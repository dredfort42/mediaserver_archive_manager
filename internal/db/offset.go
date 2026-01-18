package db

import (
	"context"
	"fmt"
	"time"

	"archive_manager/internal/config"
	"archive_manager/internal/model"

	log "github.com/dredfort42/go_logger"
)

const (
	batchSize     = 50
	flushInterval = 5 * time.Second
)

type DatabaseWriter struct {
	cameraID   string
	flushTimer *time.Ticker
	batch      []model.BatchMetadata
}

func NewDatabaseWriter(cameraID string) *DatabaseWriter {
	return &DatabaseWriter{
		cameraID:   cameraID,
		flushTimer: time.NewTicker(flushInterval),
		batch:      make([]model.BatchMetadata, 0, batchSize),
	}
}

func (dw *DatabaseWriter) Run(ctx context.Context, offsetsChan <-chan model.BatchMetadata) {
	defer dw.flushTimer.Stop()
	defer dw.flush(ctx) // Flush remaining on exit

	for {
		select {
		case <-ctx.Done():
			log.Info.Println("Database writer stopping...")
			return

		case offset, ok := <-offsetsChan:
			if !ok {
				log.Info.Println("Offsets channel closed")
				return
			}

			dw.batch = append(dw.batch, offset)

			// Flush if batch is full
			if len(dw.batch) >= batchSize {
				if err := dw.flush(ctx); err != nil {
					log.Error.Printf("Failed to flush batch: %v", err)
				}
			}

		case <-dw.flushTimer.C:
			// Periodic flush
			if len(dw.batch) > 0 {
				if err := dw.flush(ctx); err != nil {
					log.Error.Printf("Failed to flush batch: %v", err)
				}
			}
		}
	}
}

func (dw *DatabaseWriter) flush(ctx context.Context) error {
	if len(dw.batch) == 0 {
		return nil
	}

	// Build multi-row INSERT with COPY-like performance
	tx, err := db.BeginTx(ctx, nil)
	if err != nil {
		return fmt.Errorf("failed to begin transaction: %w", err)
	}
	defer tx.Rollback()

	// Group offsets by (folder, file)
	type key struct {
		folder int64
		file   int64
	}
	grouped := make(map[key][]model.BatchMetadata)

	for _, offset := range dw.batch {
		folder := GetFolderName(offset.IFrameTimeCode)
		file := GetFileName(offset.IFrameTimeCode, config.App.ArchiveManager.FragmentDuration)
		k := key{folder: folder, file: file}
		grouped[k] = append(grouped[k], offset)
	}

	// Insert each group with dynamic SQL
	for k, offsets := range grouped {
		var arrayValues string
		var videoFrameCount int64 = 0

		for _, offset := range offsets {
			if len(arrayValues) > 0 {
				arrayValues += ","
			}
			arrayValues += fmt.Sprintf("ROW(%d,%d)", offset.IFrameTimeCode, offset.IFrameOffset)

			if offset.VideoFramesInBatch > videoFrameCount {
				videoFrameCount = offset.VideoFramesInBatch
			}
		}

		// Skip if no offsets to insert
		if arrayValues == "" {
			continue
		}

		query := fmt.Sprintf(
			"INSERT INTO %s (camera_id, folder, file, iframe_indexes, total_packets) VALUES ($1, $2, $3, ARRAY[%s]::iframe_index[], $4) "+
				"ON CONFLICT (camera_id, folder, file) DO UPDATE SET iframe_indexes = %s.iframe_indexes || ARRAY[%s]::iframe_index[], total_packets = GREATEST(%s.total_packets, $4)",
			config.App.Database.TableIFrameByteOffsets,
			arrayValues,
			config.App.Database.TableIFrameByteOffsets,
			arrayValues,
			config.App.Database.TableIFrameByteOffsets,
		)

		if _, err := tx.ExecContext(ctx, query, dw.cameraID, k.folder, k.file, videoFrameCount); err != nil {
			return fmt.Errorf("failed to insert offsets for folder=%d file=%d: %w", k.folder, k.file, err)
		}
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("failed to commit transaction: %w", err)
	}

	log.Debug.Printf("Flushed %d I-frame offsets to database for camera %s", len(dw.batch), dw.cameraID)

	// Reset batch and periodically shrink capacity to prevent unbounded growth
	if cap(dw.batch) > batchSize*4 {
		dw.batch = make([]model.BatchMetadata, 0, batchSize)
	} else {
		dw.batch = dw.batch[:0]
	}

	return nil
}

func GetFolderName(timestamp int64) int64 {
	return (timestamp / 1000) / 86400 // Converts ms to seconds, then to days
}

func GetFileName(timestamp int64, fragmentLength time.Duration) int64 {
	secondsSinceMidnight := (timestamp / 1000) % 86400 // Converts ms to seconds
	return secondsSinceMidnight - (secondsSinceMidnight % int64(fragmentLength.Seconds()))
}

func DeleteAllOffsetsByCameraID(cameraID string) error {
	query := `DELETE FROM iframe_byte_offsets WHERE camera_id = $1;`
	_, err := db.Exec(query, cameraID)
	return err
}

func DeleteOffsetsByCameraIDOlderThan(cameraID string, folder, file int64) error {
	query := `
DELETE FROM iframe_byte_offsets
WHERE camera_id = $1
  AND (folder < $2 OR (folder = $2 AND file <= $3));
`
	_, err := db.Exec(query, cameraID, folder, file)
	return err
}
