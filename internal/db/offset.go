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
	tableName  string
	flushTimer *time.Ticker
	batch      []model.IFrameOffset
}

func NewDatabaseWriter(cameraID string) *DatabaseWriter {
	return &DatabaseWriter{
		cameraID:   cameraID,
		tableName:  config.App.Database.TableIFrameByteOffsets,
		flushTimer: time.NewTicker(flushInterval),
		batch:      make([]model.IFrameOffset, 0, batchSize),
	}
}

func (dw *DatabaseWriter) Run(ctx context.Context, offsetsChan <-chan model.IFrameOffset) {
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
	grouped := make(map[key][]model.IFrameOffset)

	for _, offset := range dw.batch {
		folder := getFolderName(offset.Timestamp)
		file := getFileName(offset.Timestamp, config.App.ArchiveManager.FragmentLength)
		k := key{folder: folder, file: file}
		grouped[k] = append(grouped[k], offset)
	}

	stmt, err := tx.PrepareContext(ctx, fmt.Sprintf(
		"INSERT INTO %s (camera_id, folder, file, iframe_indexes) VALUES ($1, $2, $3, $4) "+
			"ON CONFLICT (camera_id, folder, file) DO UPDATE SET iframe_indexes = %s.iframe_indexes || $4",
		dw.tableName,
		dw.tableName,
	))
	if err != nil {
		return fmt.Errorf("failed to prepare statement: %w", err)
	}
	defer stmt.Close()

	for k, offsets := range grouped {
		// Build array of iframe_index composites: ARRAY[(time1, byte1), (time2, byte2), ...]::iframe_index[]
		arrayValues := make([]string, len(offsets))
		for i, offset := range offsets {
			arrayValues[i] = fmt.Sprintf("(%d, %d)", offset.Timestamp, offset.Offset)
		}
		arrayStr := fmt.Sprintf("ARRAY[%s]::iframe_index[]", string(arrayValues[0]))
		for i := 1; i < len(arrayValues); i++ {
			arrayStr = fmt.Sprintf("%s, %s", arrayStr[:len(arrayStr)-17], arrayValues[i]) + "]::iframe_index[]"
		}

		if _, err := stmt.ExecContext(ctx, dw.cameraID, k.folder, k.file, arrayStr); err != nil {
			return fmt.Errorf("failed to insert offsets for folder=%d file=%d: %w", k.folder, k.file, err)
		}
	}

	if err := tx.Commit(); err != nil {
		return fmt.Errorf("failed to commit transaction: %w", err)
	}

	log.Info.Printf("Flushed %d I-frame offsets to database for camera %s", len(dw.batch), dw.cameraID)
	dw.batch = dw.batch[:0] // Clear batch

	return nil
}

func getFolderName(timestamp int64) int64 {
	return timestamp / 86400
}

func getFileName(timestamp int64, fragmentLength time.Duration) int64 {
	secondsSinceMidnight := timestamp % 86400
	return secondsSinceMidnight - (secondsSinceMidnight % int64(fragmentLength.Seconds()))
}
