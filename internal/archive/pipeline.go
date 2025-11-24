package archive

import (
	"archive_manager/internal/broker"
	"archive_manager/internal/config"
	"archive_manager/internal/db"
	"archive_manager/internal/model"
	"context"
	"sync"
)

type Pipeline struct {
	cameraID string
	topic    string

	framesChan  chan model.Frame
	offsetsChan chan model.BatchMetadata

	// consumer *Consumer
	writer   *Writer
	dbWriter *db.DatabaseWriter
}

func NewPipeline(cameraID, topic string) *Pipeline {
	return &Pipeline{
		cameraID: cameraID,
		topic:    topic,

		framesChan:  make(chan model.Frame, 300),         // Buffer ~10s at 30fps
		offsetsChan: make(chan model.BatchMetadata, 100), // Buffer ~100 I-frames

		writer:   NewWriter(config.App.ArchiveManager.StoragePath, config.App.ArchiveManager.FragmentLength),
		dbWriter: db.NewDatabaseWriter(cameraID),
	}
}

func (p *Pipeline) Start(ctx context.Context) error {
	var wg sync.WaitGroup

	// Register topic with broker and start consuming frames
	broker.AddArchiveTopic(p.topic, p.framesChan)

	// Start file writer goroutine
	wg.Go(func() {
		p.writer.Run(ctx, p.framesChan, p.offsetsChan)
	})

	// Start database writer goroutine
	wg.Go(func() {
		p.dbWriter.Run(ctx, p.offsetsChan)
	})

	// Wait for context cancellation
	<-ctx.Done()

	// Unregister topic from broker
	broker.RemoveArchiveTopic(p.topic)

	// Close channels to signal goroutines to stop
	close(p.framesChan)
	close(p.offsetsChan)

	// Wait for all goroutines to finish
	wg.Wait()

	return nil
}
