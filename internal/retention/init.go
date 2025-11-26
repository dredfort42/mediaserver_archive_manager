package retention

import (
	"context"
	"fmt"
	"os"
	"sync"

	"archive_manager/internal/config"
)

func Init(ctx context.Context, cancel context.CancelFunc, wg *sync.WaitGroup) (err error) {
	if info, err := os.Stat(config.App.ArchiveManager.StoragePath); err != nil {
		if os.IsNotExist(err) {
			return fmt.Errorf("storage path does not exist: %s", config.App.ArchiveManager.StoragePath)
		}
		return fmt.Errorf("failed to stat storage path %s: %w", config.App.ArchiveManager.StoragePath, err)
	} else if !info.IsDir() {
		return fmt.Errorf("storage path is not a directory: %s", config.App.ArchiveManager.StoragePath)
	}

	wg.Add(1)
	go purge(ctx, wg)

	return nil
}
