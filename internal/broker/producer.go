package broker

import (
	"sync"
	"time"

	log "github.com/dredfort42/go_logger"
	"github.com/twmb/franz-go/pkg/kerr"
	"github.com/twmb/franz-go/pkg/kgo"
)

func startProducing(wg *sync.WaitGroup, msgChan <-chan *Message) error {
	defer wg.Done()

	const batchSize = 100
	const batchTimeout = 500 * time.Millisecond

	batch := make([]*kgo.Record, 0, batchSize)
	ticker := time.NewTicker(batchTimeout)
	defer ticker.Stop()

	log.Info.Println("Kafka producer started")
	defer log.Info.Println("Kafka producer stopped")

	for {
		select {
		case <-brokerCtx.Done():
			return nil
		case msg, ok := <-msgChan:
			if !ok {
				if len(batch) > 0 {
					produceBatch(batch)
				}
				log.Info.Println("Kafka producer context canceled")
				return nil
			}

			batch = append(batch, &kgo.Record{
				Topic: msg.Topic,
				Key:   []byte(msg.Key),
				Value: []byte(msg.Value),
			})

			if len(batch) >= batchSize {
				produceBatch(batch)
				batch = batch[:0]
			}

		case <-ticker.C:
			if len(batch) > 0 {
				produceBatch(batch)
				batch = batch[:0]
			}
		}
	}
}

func produceBatch(batch []*kgo.Record) {
	if err := Client.BeginTransaction(); err != nil {
		log.Error.Printf("beginning transaction: %v\n", err)
		return
	}

	Client.ProduceSync(brokerCtx, batch...)

	if err := Client.Flush(brokerCtx); err != nil {
		log.Error.Printf("flush was killed due to context cancelation: %v\n", err)
	}

	switch err := Client.EndTransaction(brokerCtx, kgo.TryCommit); err {
	case nil:
	case kerr.OperationNotAttempted:
		rollback()
	default:
		log.Error.Printf("committing transaction: %v\n", err)
	}
}

func rollback() {
	if err := Client.AbortBufferedRecords(brokerCtx); err != nil {
		log.Error.Printf("aborting buffered records: %v\n", err)
		return
	}

	if err := Client.EndTransaction(brokerCtx, kgo.TryAbort); err != nil {
		log.Error.Printf("rolling back transaction: %v\n", err)
		return
	}

	log.Info.Println("transaction rolled back")
}
