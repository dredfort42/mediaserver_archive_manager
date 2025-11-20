package middleware

//nolint:gci
import (
	"time"

	"archive_manager/internal/metrics"

	"github.com/gin-gonic/gin"
)

const metricsEndpoint = "/metrics"

// Metrics creates middleware that records Prometheus metrics for HTTP requests.
func Metrics() gin.HandlerFunc {
	return func(c *gin.Context) {
		// Skip metrics collection for the metrics endpoint itself to avoid recursion
		if c.Request.URL.Path == metricsEndpoint {
			c.Next()

			return
		}

		start := time.Now()

		// Track in-flight requests
		if metrics.GlobalMetrics != nil {
			increment, decrement := metrics.GlobalMetrics.InFlight()
			increment()

			defer decrement()
		}

		// Process request
		c.Next()

		// Record metrics after request completion
		if metrics.GlobalMetrics != nil {
			duration := time.Since(start)
			statusCode := c.Writer.Status()
			responseSize := int64(c.Writer.Size())

			metrics.GlobalMetrics.RecordRequest(
				c.Request.Method,
				c.Request.URL.Path,
				statusCode,
				duration,
				responseSize,
			)
		}
	}
}
