package controller

import (
	"context"
	"sync"
	"time"
)

const CheckInterval = 5 * time.Second

func Init(ctx context.Context, wg *sync.WaitGroup) error {
	var controlledArchiveStreams sync.Map // map[string]context.CancelFunc -> map[StreamID]archiveCancel

	wg.Add(1)
	go controller(ctx, wg, &controlledArchiveStreams)

	// wg.Add(1)
	// go connectorsManager(ctx, wg, &controlledStreams)

	return nil
}
