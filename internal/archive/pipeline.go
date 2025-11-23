package archive

import (
	"archive_manager/internal/config"
	"archive_manager/internal/db"
	"archive_manager/internal/model"
)

type Pipeline struct {
	cameraID   string
	kafkaTopic string

	framesChan  chan model.Frame
	offsetsChan chan model.BatchMetadata

	// consumer *Consumer
	writer   *Writer
	dbWriter *db.DatabaseWriter
}

func NewPipeline(cameraID, kafkaTopic, outputPath string) *Pipeline {
	return &Pipeline{
		cameraID:   cameraID,
		kafkaTopic: kafkaTopic,

		framesChan:  make(chan model.Frame, 300),         // Buffer ~10s at 30fps
		offsetsChan: make(chan model.BatchMetadata, 100), // Buffer ~100 I-frames
		// consumer:    NewConsumer(kafkaTopic),
		writer:   NewWriter(config.App.ArchiveManager.StoragePath, config.App.ArchiveManager.FragmentLength),
		dbWriter: db.NewDatabaseWriter(cameraID),
	}
}

// func (p *Pipeline) Start(ctx context.Context) error {
// 	var wg sync.WaitGroup

// 	// Start consumer goroutine
// 	wg.Add(1)
// 	go func() {
// 		defer wg.Done()
// 		p.consumer.Run(ctx, p.framesChan)
// 	}()

// 	// Start file writer goroutine
// 	wg.Add(1)
// 	go func() {
// 		defer wg.Done()
// 		p.writer.Run(ctx, p.framesChan, p.offsetsChan)
// 	}()

// 	// Start database writer goroutine
// 	wg.Add(1)
// 	go func() {
// 		defer wg.Done()
// 		p.dbWriter.Run(ctx, p.offsetsChan)
// 	}()

// 	// Wait for context cancellation
// 	<-ctx.Done()

// 	// Close channels to signal goroutines to stop
// 	close(p.framesChan)
// 	close(p.offsetsChan)

// 	// Wait for all goroutines to finish
// 	wg.Wait()

// 	return nil
// }
