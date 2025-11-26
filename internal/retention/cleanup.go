package retention

import (
	"archive_manager/internal/broker"
	"archive_manager/internal/config"
	"context"
	"os"
	"path/filepath"
	"sync"
	"time"

	log "github.com/dredfort42/go_logger"
)

func cleanup(ctx context.Context, wg *sync.WaitGroup) {
	defer wg.Done()

	log.Info.Println("Archive cleanup started")

	initialTimer := time.NewTimer(time.Minute * 15)
	defer initialTimer.Stop()

	select {
	case <-initialTimer.C:
		runCleanup()
	case <-ctx.Done():
		log.Info.Println("Archive cleanup stopped before first run")
		return
	}

	ticker := time.NewTicker(time.Hour)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			runCleanup()
		case <-ctx.Done():
			log.Info.Println("Archive cleanup stopped")
			return
		}
	}
}

func runCleanup() {
	log.Info.Println("Running archive cleanup task...")

	entries, err := os.ReadDir(config.App.ArchiveManager.StoragePath)
	if err != nil {
		log.Error.Println("Failed to read storage path:", err)
		return
	}

	for _, entry := range entries {
		if entry.IsDir() {
			_, exists := broker.Retentions.Load(entry.Name())
			if !exists {
				log.Info.Println("Removing orphaned archive folder:", entry.Name())
				folderPath := filepath.Join(config.App.ArchiveManager.StoragePath, entry.Name())
				if err := os.RemoveAll(folderPath); err != nil {
					log.Error.Println("Failed to remove orphaned archive folder:", err)
				}
			}
		} else {
			log.Warning.Println("Unexpected file in storage path, removing:", entry.Name())
			filePath := filepath.Join(config.App.ArchiveManager.StoragePath, entry.Name())
			if err := os.Remove(filePath); err != nil {
				log.Error.Println("Failed to remove unexpected file:", err)
			}
		}
	}
}
