package metrics

//nolint:gci
import (
	"strings"
	"sync"

	"archive_manager/internal/config"

	"github.com/prometheus/client_golang/prometheus"
)

// Metrics holds all Prometheus metrics for the live_webrtc_streamer service.
type Metrics struct {
	// HTTP Request metrics
	httpRequestsTotal    *prometheus.CounterVec
	httpRequestDuration  *prometheus.HistogramVec
	httpRequestsInFlight prometheus.Gauge
	httpResponseSize     *prometheus.HistogramVec
}

// GlobalMetrics metrics instance for easy access.
var GlobalMetrics *Metrics

// Global sync.Once to ensure metrics are initialized only once across all packages and tests.
var initMetricsOnce sync.Once

// InitializeMetrics initializes the global metrics collector.
// It uses sync.Once to ensure metrics are only initialized once, even when called multiple times.
func InitializeMetrics() {
	initMetricsOnce.Do(func() {
		namespace := strings.ToLower(config.App.AppName)

		labels := prometheus.Labels{
			"service": namespace,
			"version": config.App.Version,
		}

		GlobalMetrics = &Metrics{}

		Add(namespace, labels)
	})
}
