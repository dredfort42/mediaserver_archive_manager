package metrics

import (
	"strconv"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/promauto"
)

// Add initializes HTTP request metrics.
func Add(namespace string, labels prometheus.Labels) {
	GlobalMetrics.httpRequestsTotal = promauto.NewCounterVec(
		prometheus.CounterOpts{
			Namespace:   namespace,
			Name:        "http_requests_total",
			Help:        "Total number of HTTP requests processed",
			ConstLabels: labels,
		},
		[]string{"method", "endpoint", "status_code"},
	)

	GlobalMetrics.httpRequestDuration = promauto.NewHistogramVec(
		prometheus.HistogramOpts{
			Namespace:   namespace,
			Name:        "http_request_duration_seconds",
			Help:        "Duration of HTTP requests in seconds",
			ConstLabels: labels,
			Buckets:     []float64{.005, .01, .025, .05, .1, .25, .5, 1, 2.5, 5, 10, 30, 60},
		},
		[]string{"method", "endpoint", "status_code"},
	)

	GlobalMetrics.httpRequestsInFlight = promauto.NewGauge(
		prometheus.GaugeOpts{
			Namespace:   namespace,
			Name:        "http_requests_in_flight",
			Help:        "Current number of HTTP requests being processed",
			ConstLabels: labels,
		},
	)

	GlobalMetrics.httpResponseSize = promauto.NewHistogramVec(
		prometheus.HistogramOpts{
			Namespace:   namespace,
			Name:        "http_response_size_bytes",
			Help:        "Size of HTTP responses in bytes",
			ConstLabels: labels,
			Buckets:     prometheus.ExponentialBuckets(1024, 2, 20), // 1KB to ~1GB
		},
		[]string{"method", "endpoint", "content_type"},
	)
}

// RecordRequest records metrics for HTTP requests.
func (mc *Metrics) RecordRequest(
	method, endpoint string,
	statusCode int,
	duration time.Duration,
	responseSize int64) {
	status := strconv.Itoa(statusCode)

	mc.httpRequestsTotal.WithLabelValues(method, endpoint, status).Inc()
	mc.httpRequestDuration.WithLabelValues(method, endpoint, status).Observe(duration.Seconds())

	// Determine content type based on endpoint
	contentType := "application/json"
	// if endpoint == "/streamshot/get" || endpoint == "/avg_streamshot/get" {
	// 	contentType = "image/jpeg" // Default, could be refined
	// }

	mc.httpResponseSize.WithLabelValues(method, endpoint, contentType).Observe(float64(responseSize))
}

// InFlight tracks in-flight HTTP requests.
func (mc *Metrics) InFlight() (increment func(), decrement func()) {
	return func() {
			mc.httpRequestsInFlight.Inc()
		}, func() {
			mc.httpRequestsInFlight.Dec()
		}
}

// GetInFlight returns the HTTP requests in-flight gauge for testing.
func (mc *Metrics) GetInFlight() prometheus.Gauge {
	return mc.httpRequestsInFlight
}

// GetRequestsTotal returns the HTTP requests total counter for testing.
func (mc *Metrics) GetRequestsTotal() *prometheus.CounterVec {
	return mc.httpRequestsTotal
}

// GetRequestDuration returns the HTTP request duration histogram for testing.
func (mc *Metrics) GetRequestDuration() *prometheus.HistogramVec {
	return mc.httpRequestDuration
}

// GetResponseSize returns the HTTP response size histogram for testing.
func (mc *Metrics) GetResponseSize() *prometheus.HistogramVec {
	return mc.httpResponseSize
}
