package retention

import (
	"context"
	"os"
	"path/filepath"
	"strconv"
	"sync"
	"time"

	"archive_manager/internal/broker"
	"archive_manager/internal/config"
	"archive_manager/internal/db"

	log "github.com/dredfort42/go_logger"
)

func purge(ctx context.Context, wg *sync.WaitGroup) {
	defer wg.Done()

	log.Info.Println("Retention purge started")

	ticker := time.NewTicker(config.App.ArchiveManager.FragmentDuration)
	defer ticker.Stop()

	for {
		select {
		case <-ticker.C:
			log.Info.Println("Running archive purging task...")

			type job struct {
				cameraID      string
				retentionDays uint32
			}

			jobs := make(chan job)
			var wgWorkers sync.WaitGroup

			wgWorkers.Add(config.App.ArchiveManager.RetentionWorkers)
			for i := range config.App.ArchiveManager.RetentionWorkers {
				go func(id int) {
					defer wgWorkers.Done()
					for {
						select {
						case <-ctx.Done():
							return
						case j, ok := <-jobs:
							if !ok {
								return
							}
							purgeOnce(j.cameraID, j.retentionDays)
						}
					}
				}(i)
			}

			broker.Retentions.Range(func(key, value any) bool {
				select {
				case <-ctx.Done():
					return false
				default:
					keyStr, ok := key.(string)
					if !ok {
						return true
					}

					valueUInt32, ok := value.(uint32)
					if !ok {
						return true
					}

					select {
					case <-ctx.Done():
						return false
					default:
						jobs <- job{cameraID: keyStr, retentionDays: valueUInt32}
					}
					return true
				}
			})

			close(jobs)
			wgWorkers.Wait()

		case <-ctx.Done():
			log.Info.Println("Retention purge stopped")
			return
		}
	}
}

func purgeOnce(cameraID string, retentionDays uint32) {
	log.Debug.Printf("Purging archives for camera UUID: %s with retention days: %d\n", cameraID, retentionDays)

	if retentionDays == 0 {
		if err := db.DeleteAllOffsetsByCameraID(cameraID); err != nil {
			log.Error.Printf("Failed to delete offsets for camera %s: %v\n", cameraID, err)
		}

		if err := deleteAllFilesAndFoldersByCameraID(cameraID); err != nil {
			log.Error.Printf("Failed to remove archive folder for camera %s: %v\n", cameraID, err)
		}

		broker.Retentions.Delete(cameraID)
	} else {
		cutoffTime := time.Now().Add(-time.Duration(retentionDays)*24*time.Hour + config.App.ArchiveManager.FragmentDuration).UnixMilli()

		folder := cutoffTime / 86400000
		file := (cutoffTime % 86400000) / 1000

		if err := db.DeleteOffsetsByCameraIDOlderThan(cameraID, folder, file); err != nil {
			log.Error.Printf("Failed to delete offsets for camera %s older than folder %d and file %d: %v\n", cameraID, folder, file, err)
		}

		if err := deleteFilesAndFoldersByCameraIDOlderThan(cameraID, folder, file); err != nil {
			log.Error.Printf("Failed to remove old archive files for camera %s older than folder %d and file %d: %v\n", cameraID, folder, file, err)
		}
	}
}

func deleteAllFilesAndFoldersByCameraID(cameraID string) error {
	dir := filepath.Join(config.App.ArchiveManager.StoragePath, cameraID)

	if err := os.RemoveAll(dir); err != nil {
		return err
	}

	log.Debug.Printf("Removed archive folder for camera %s: %s\n", cameraID, dir)

	return nil
}

func deleteFilesAndFoldersByCameraIDOlderThan(cameraID string, folder, file int64) error {
	dirPath := filepath.Join(config.App.ArchiveManager.StoragePath, cameraID)

	folders, err := os.ReadDir(dirPath)
	if err != nil {
		return err
	}

	for _, f := range folders {
		if !f.IsDir() {
			continue
		}

		folderNum, err := strconv.ParseInt(f.Name(), 10, 64)
		if err != nil {
			continue
		}

		if folderNum < folder {
			folderPath := filepath.Join(dirPath, f.Name())
			if err := os.RemoveAll(folderPath); err != nil {
				log.Error.Printf("Failed to remove folder %s: %v\n", folderPath, err)
			} else {
				log.Info.Printf("Removed folder %s\n", folderPath)
			}
		} else if folderNum == folder {
			files, err := os.ReadDir(filepath.Join(dirPath, f.Name()))
			if err != nil {
				continue
			}

			for _, fileEntry := range files {
				fileNum, err := strconv.ParseInt(fileEntry.Name(), 10, 64)
				if err != nil {
					continue
				}

				if fileNum <= file {
					filePath := filepath.Join(dirPath, f.Name(), fileEntry.Name())
					if err := os.Remove(filePath); err != nil {
						log.Error.Printf("Failed to remove file %s: %v\n", filePath, err)
					} else {
						log.Info.Printf("Removed file %s\n", filePath)
					}

					jsonFilePath := filePath + ".json"
					if err := os.Remove(jsonFilePath); err != nil {
						log.Error.Printf("Failed to remove file %s: %v\n", jsonFilePath, err)
					} else {
						log.Info.Printf("Removed file %s\n", jsonFilePath)
					}
				}
			}
		}
	}

	return nil
}
