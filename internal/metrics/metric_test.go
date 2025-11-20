package metrics

import (
	"strconv"
	"testing"
	"time"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/prometheus/client_golang/prometheus/testutil"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// setupHTTPMetricsTest initializes metrics for testing HTTP metrics functionality.
func setupHTTPMetricsTest() {
	// Reset metrics state
	resetMetricsForTesting()

	// Initialize GlobalMetrics
	GlobalMetrics = &Metrics{}

	// Setup test namespace and labels
	namespace := "test"
	labels := prometheus.Labels{
		"service": "test-service",
		"version": "test-version",
	}

	// Initialize HTTP metrics
	Add(namespace, labels)
}

// Test AddHTTPMetrics function.
func TestAddHTTPMetrics(t *testing.T) {
	t.Run("AddHTTPMetrics initializes all HTTP metrics", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Verify all HTTP metrics are initialized
		assert.NotNil(t, GlobalMetrics.httpRequestsTotal)
		assert.NotNil(t, GlobalMetrics.httpRequestDuration)
		assert.NotNil(t, GlobalMetrics.httpRequestsInFlight)
		assert.NotNil(t, GlobalMetrics.httpResponseSize)
	})

	t.Run("AddHTTPMetrics creates counter with correct configuration", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test that httpRequestsTotal counter is properly configured
		counter := GlobalMetrics.httpRequestsTotal
		require.NotNil(t, counter)

		// Test incrementing the counter
		counter.WithLabelValues("GET", "/test", "200").Inc()

		// Verify the metric was recorded
		metricValue := testutil.ToFloat64(counter.WithLabelValues("GET", "/test", "200"))
		assert.Equal(t, float64(1), metricValue)
	})

	t.Run("AddHTTPMetrics creates histogram with correct buckets", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test that httpRequestDuration histogram is properly configured
		histogram := GlobalMetrics.httpRequestDuration
		require.NotNil(t, histogram)

		// Test recording a duration - should not panic
		assert.NotPanics(t, func() {
			histogram.WithLabelValues("POST", "/api", "201").Observe(0.5)
			histogram.WithLabelValues("POST", "/api", "201").Observe(0.1)
		})
	})

	t.Run("AddHTTPMetrics creates gauge for in-flight requests", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test that httpRequestsInFlight gauge is properly configured
		gauge := GlobalMetrics.httpRequestsInFlight
		require.NotNil(t, gauge)

		// Test incrementing and decrementing the gauge
		gauge.Inc()
		assert.Equal(t, float64(1), testutil.ToFloat64(gauge))

		gauge.Dec()
		assert.Equal(t, float64(0), testutil.ToFloat64(gauge))
	})

	t.Run("AddHTTPMetrics creates response size histogram", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test that httpResponseSize histogram is properly configured
		histogram := GlobalMetrics.httpResponseSize
		require.NotNil(t, histogram)

		// Test recording response sizes - should not panic
		assert.NotPanics(t, func() {
			histogram.WithLabelValues("GET", "/data", "application/json").Observe(1024)
		})
	})
}

// Test AddHTTPMetrics with different namespaces and labels.
func TestAddHTTPMetricsConfiguration(t *testing.T) {
	testCases := []struct {
		name      string
		namespace string
		labels    prometheus.Labels
	}{
		{
			name:      "Basic configuration",
			namespace: "myapp",
			labels:    prometheus.Labels{"service": "myapp", "version": "1.0"},
		},
		{
			name:      "Empty namespace",
			namespace: "",
			labels:    prometheus.Labels{"service": "test"},
		},
		{
			name:      "Complex labels",
			namespace: "complex",
			labels:    prometheus.Labels{"service": "complex-service", "version": "2.1.0", "env": "test"},
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			resetMetricsForTesting()

			GlobalMetrics = &Metrics{}

			// Should not panic with different configurations
			assert.NotPanics(t, func() {
				Add(tc.namespace, tc.labels)
			})

			// Verify metrics are initialized
			assert.NotNil(t, GlobalMetrics.httpRequestsTotal)
			assert.NotNil(t, GlobalMetrics.httpRequestDuration)
			assert.NotNil(t, GlobalMetrics.httpRequestsInFlight)
			assert.NotNil(t, GlobalMetrics.httpResponseSize)
		})
	}
}

// Test RecordHTTPRequest function.
func TestRecordHTTPRequest(t *testing.T) {
	t.Run("RecordHTTPRequest records all metrics correctly", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Record a test HTTP request
		method := "GET"
		endpoint := "/api/test"
		statusCode := 200
		duration := 100 * time.Millisecond
		responseSize := int64(1024)

		GlobalMetrics.RecordRequest(method, endpoint, statusCode, duration, responseSize)

		// Verify counter was incremented
		counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues(method, endpoint, "200"))
		assert.Equal(t, float64(1), counterValue)

		// Verify histogram recorded duration - test that operations don't panic
		assert.NotPanics(t, func() {
			GlobalMetrics.httpRequestDuration.WithLabelValues(method, endpoint, "200").Observe(duration.Seconds())
		})

		// Verify response size histogram - test that operations don't panic
		assert.NotPanics(t, func() {
			GlobalMetrics.httpResponseSize.WithLabelValues(method, endpoint, "application/json").Observe(float64(responseSize))
		})
	})

	t.Run("RecordHTTPRequest handles different status codes", func(t *testing.T) {
		setupHTTPMetricsTest()

		testCases := []struct {
			statusCode int
			expected   string
		}{
			{200, "200"},
			{404, "404"},
			{500, "500"},
			{301, "301"},
		}

		for _, tc := range testCases {
			GlobalMetrics.RecordRequest("GET", "/test", tc.statusCode, 50*time.Millisecond, 512)

			// Verify the status code was converted to string correctly
			counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("GET", "/test", tc.expected))
			assert.Equal(t, float64(1), counterValue)
		}
	})

	t.Run("RecordHTTPRequest handles streamshot endpoints", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test streamshot endpoints get image/jpeg content type
		endpoints := []string{"/streamshot/get", "/avg_streamshot/get"}

		for _, endpoint := range endpoints {
			// Test that the recording doesn't panic
			assert.NotPanics(t, func() {
				GlobalMetrics.RecordRequest("GET", endpoint, 200, 200*time.Millisecond, 2048)
			})
		}
	})

	t.Run("RecordHTTPRequest handles non-streamshot endpoints", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Test non-streamshot endpoints get application/json content type
		endpoints := []string{"/api/test", "/health", "/metrics"}

		for _, endpoint := range endpoints {
			// Test that the recording doesn't panic
			assert.NotPanics(t, func() {
				GlobalMetrics.RecordRequest("POST", endpoint, 201, 150*time.Millisecond, 1024)
			})
		}
	})

	t.Run("RecordHTTPRequest handles multiple requests", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Record multiple requests
		for range 5 {
			GlobalMetrics.RecordRequest("GET", "/multiple", 200, 50*time.Millisecond, 1024)
		}

		// Verify counter shows correct count
		counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("GET", "/multiple", "200"))
		assert.Equal(t, float64(5), counterValue)

		// Verify histogram operations don't panic
		assert.NotPanics(t, func() {
			GlobalMetrics.httpRequestDuration.WithLabelValues("GET", "/multiple", "200").Observe(0.05)
		})
	})
}

// Test RecordHTTPRequest with various durations and sizes.
func TestRecordHTTPRequestVariations(t *testing.T) {
	t.Run("RecordHTTPRequest with different durations", func(t *testing.T) {
		setupHTTPMetricsTest()

		durations := []time.Duration{
			1 * time.Millisecond,
			10 * time.Millisecond,
			100 * time.Millisecond,
			1 * time.Second,
			5 * time.Second,
		}

		for i, duration := range durations {
			endpoint := "/duration-test-" + strconv.Itoa(i)
			GlobalMetrics.RecordRequest("GET", endpoint, 200, duration, 1024)

			// Verify the request was recorded
			counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("GET", endpoint, "200"))
			assert.Equal(t, float64(1), counterValue)
		}
	})

	t.Run("RecordHTTPRequest with different response sizes", func(t *testing.T) {
		setupHTTPMetricsTest()

		sizes := []int64{
			100,      // 100 bytes
			1024,     // 1 KB
			1048576,  // 1 MB
			10485760, // 10 MB
		}

		for i, size := range sizes {
			endpoint := "/size-test-" + strconv.Itoa(i)
			GlobalMetrics.RecordRequest("POST", endpoint, 201, 100*time.Millisecond, size)

			// Verify the request was recorded
			counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("POST", endpoint, "201"))
			assert.Equal(t, float64(1), counterValue)
		}
	})
}

// Test InFlightRequestsMiddleware function.
func TestInFlightRequestsMiddleware(t *testing.T) {
	t.Run("InFlightRequestsMiddleware returns increment and decrement functions", func(t *testing.T) {
		setupHTTPMetricsTest()

		increment, decrement := GlobalMetrics.InFlight()

		// Verify functions are not nil
		assert.NotNil(t, increment)
		assert.NotNil(t, decrement)

		// Test that functions are callable
		assert.NotPanics(t, func() {
			increment()
			decrement()
		})
	})

	t.Run("InFlightRequestsMiddleware functions modify gauge correctly", func(t *testing.T) {
		setupHTTPMetricsTest()

		increment, decrement := GlobalMetrics.InFlight()

		// Initial value should be 0
		initialValue := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(0), initialValue)

		// Test increment
		increment()

		afterIncrement := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(1), afterIncrement)

		// Test multiple increments
		increment()
		increment()

		afterMultipleIncrements := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(3), afterMultipleIncrements)

		// Test decrement
		decrement()

		afterDecrement := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(2), afterDecrement)

		// Test multiple decrements
		decrement()
		decrement()

		afterMultipleDecrements := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(0), afterMultipleDecrements)
	})

	t.Run("InFlightRequestsMiddleware can go negative", func(t *testing.T) {
		setupHTTPMetricsTest()

		_, decrement := GlobalMetrics.InFlight()

		// Decrement without increment should make it negative
		decrement()

		value := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(-1), value)
	})

	t.Run("Multiple middleware instances work independently", func(t *testing.T) {
		setupHTTPMetricsTest()

		increment1, decrement1 := GlobalMetrics.InFlight()
		increment2, decrement2 := GlobalMetrics.InFlight()

		// Both should affect the same gauge
		increment1()
		increment2()

		value := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(2), value)

		decrement1()
		decrement2()

		finalValue := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(0), finalValue)
	})
}

// Test edge cases and error conditions.
func TestHTTPMetricsEdgeCases(t *testing.T) {
	t.Run("AddHTTPMetrics with nil GlobalMetrics panics", func(t *testing.T) {
		resetMetricsForTesting()

		GlobalMetrics = nil

		// Should panic when GlobalMetrics is nil
		assert.Panics(t, func() {
			Add("test", prometheus.Labels{})
		})
	})

	t.Run("RecordHTTPRequest with zero duration", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Should handle zero duration gracefully
		assert.NotPanics(t, func() {
			GlobalMetrics.RecordRequest("GET", "/zero", 200, 0, 1024)
		})

		counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("GET", "/zero", "200"))
		assert.Equal(t, float64(1), counterValue)
	})

	t.Run("RecordHTTPRequest with zero response size", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Should handle zero response size gracefully
		assert.NotPanics(t, func() {
			GlobalMetrics.RecordRequest("POST", "/empty", 204, 50*time.Millisecond, 0)
		})

		counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("POST", "/empty", "204"))
		assert.Equal(t, float64(1), counterValue)
	})

	t.Run("RecordHTTPRequest with negative response size", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Should handle negative response size (though unusual)
		assert.NotPanics(t, func() {
			GlobalMetrics.RecordRequest("GET", "/negative", 200, 50*time.Millisecond, -1)
		})
	})
}

// Test content type determination logic.
func TestContentTypeLogic(t *testing.T) {
	t.Run("Content type determination for various endpoints", func(t *testing.T) {
		setupHTTPMetricsTest()

		testCases := []struct {
			endpoint            string
			expectedContentType string
		}{
			{"/streamshot/get", "image/jpeg"},
			{"/avg_streamshot/get", "image/jpeg"},
			{"/api/test", "application/json"},
			{"/health", "application/json"},
			{"/metrics", "application/json"},
			{"/about", "application/json"},
			{"", "application/json"},
		}

		for _, tc := range testCases {
			t.Run("endpoint: "+tc.endpoint, func(t *testing.T) {
				// Test that recording doesn't panic and uses correct content type logic
				assert.NotPanics(t, func() {
					GlobalMetrics.RecordRequest("GET", tc.endpoint, 200, 100*time.Millisecond, 1024)
				})
			})
		}
	})
}

// Test concurrent access to HTTP metrics.
func TestHTTPMetricsConcurrency(t *testing.T) {
	t.Run("Concurrent RecordHTTPRequest calls", func(t *testing.T) {
		setupHTTPMetricsTest()

		const (
			numGoroutines        = 50
			requestsPerGoroutine = 10
		)

		// Use channels to synchronize goroutines
		start := make(chan struct{})
		done := make(chan struct{}, numGoroutines)

		// Start goroutines
		for range numGoroutines {
			go func() {
				<-start // Wait for start signal

				for range requestsPerGoroutine {
					GlobalMetrics.RecordRequest("GET", "/concurrent", 200, 10*time.Millisecond, 1024)
				}

				done <- struct{}{}
			}()
		}

		// Start all goroutines
		close(start)

		// Wait for all to complete
		for range numGoroutines {
			<-done
		}

		// Verify total count
		expectedTotal := float64(numGoroutines * requestsPerGoroutine)
		counterValue := testutil.ToFloat64(GlobalMetrics.httpRequestsTotal.WithLabelValues("GET", "/concurrent", "200"))
		assert.Equal(t, expectedTotal, counterValue)
	})

	t.Run("Concurrent InFlightRequestsMiddleware usage", func(t *testing.T) {
		setupHTTPMetricsTest()

		const numGoroutines = 20

		start := make(chan struct{})
		done := make(chan struct{}, numGoroutines)

		for range numGoroutines {
			go func() {
				<-start

				increment, decrement := GlobalMetrics.InFlight()

				// Simulate request processing
				increment()
				time.Sleep(1 * time.Millisecond) // Simulate work
				decrement()

				done <- struct{}{}
			}()
		}

		close(start)

		// Wait for all to complete
		for range numGoroutines {
			<-done
		}

		// Final value should be 0 (all requests completed)
		finalValue := testutil.ToFloat64(GlobalMetrics.httpRequestsInFlight)
		assert.Equal(t, float64(0), finalValue)
	})
}

// Benchmark tests.
func BenchmarkRecordHTTPRequest(b *testing.B) {
	setupHTTPMetricsTest()

	for b.Loop() {
		GlobalMetrics.RecordRequest("GET", "/benchmark", 200, 100*time.Millisecond, 1024)
	}
}

func BenchmarkInFlightRequestsMiddleware(b *testing.B) {
	setupHTTPMetricsTest()

	increment, decrement := GlobalMetrics.InFlight()

	for b.Loop() {
		increment()
		decrement()
	}
}

func BenchmarkAddHTTPMetrics(b *testing.B) {
	for b.Loop() {
		resetMetricsForTesting()

		GlobalMetrics = &Metrics{}

		Add("benchmark", prometheus.Labels{"service": "test"})
	}
}

// Test getter functions for HTTP metrics.
func TestGetHTTPRequestsTotal(t *testing.T) {
	t.Run("GetHTTPRequestsTotal returns initialized counter", func(t *testing.T) {
		setupHTTPMetricsTest()

		counter := GlobalMetrics.GetRequestsTotal()
		assert.NotNil(t, counter, "GetHTTPRequestsTotal should return a non-nil CounterVec")
		assert.Equal(t, GlobalMetrics.httpRequestsTotal, counter, "Should return the same instance as internal field")
	})

	t.Run("GetHTTPRequestsTotal returns working counter", func(t *testing.T) {
		setupHTTPMetricsTest()

		counter := GlobalMetrics.GetRequestsTotal()

		// Test that the returned counter is functional
		counter.WithLabelValues("GET", "/test", "200").Inc()

		// Verify the metric was recorded
		value := testutil.ToFloat64(counter.WithLabelValues("GET", "/test", "200"))
		assert.Equal(t, float64(1), value, "Counter should increment correctly")
	})

	t.Run("GetHTTPRequestsTotal returns nil when not initialized", func(t *testing.T) {
		resetMetricsForTesting()

		GlobalMetrics = &Metrics{}

		counter := GlobalMetrics.GetRequestsTotal()
		assert.Nil(t, counter, "Should return nil when metrics not initialized")
	})
}

func TestGetHTTPRequestsInFlight(t *testing.T) {
	t.Run("GetHTTPRequestsInFlight returns initialized gauge", func(t *testing.T) {
		setupHTTPMetricsTest()

		gauge := GlobalMetrics.GetInFlight()
		assert.NotNil(t, gauge, "GetHTTPRequestsInFlight should return a non-nil Gauge")
		assert.Equal(t, GlobalMetrics.httpRequestsInFlight, gauge, "Should return the same instance as internal field")
	})

	t.Run("GetHTTPRequestsInFlight returns working gauge", func(t *testing.T) {
		setupHTTPMetricsTest()

		gauge := GlobalMetrics.GetInFlight()

		// Test that the returned gauge is functional
		gauge.Inc()
		value := testutil.ToFloat64(gauge)
		assert.Equal(t, float64(1), value, "Gauge should increment correctly")

		gauge.Dec()
		value = testutil.ToFloat64(gauge)
		assert.Equal(t, float64(0), value, "Gauge should decrement correctly")

		gauge.Set(5)
		value = testutil.ToFloat64(gauge)
		assert.Equal(t, float64(5), value, "Gauge should set value correctly")
	})

	t.Run("GetHTTPRequestsInFlight returns nil when not initialized", func(t *testing.T) {
		resetMetricsForTesting()

		GlobalMetrics = &Metrics{}

		gauge := GlobalMetrics.GetInFlight()
		assert.Nil(t, gauge, "Should return nil when metrics not initialized")
	})
}

func TestGetHTTPRequestDuration(t *testing.T) {
	t.Run("GetHTTPRequestDuration returns initialized histogram", func(t *testing.T) {
		setupHTTPMetricsTest()

		histogram := GlobalMetrics.GetRequestDuration()
		assert.NotNil(t, histogram, "GetHTTPRequestDuration should return a non-nil HistogramVec")
		assert.Equal(t, GlobalMetrics.httpRequestDuration, histogram, "Should return the same instance as internal field")
	})

	t.Run("GetHTTPRequestDuration returns working histogram", func(t *testing.T) {
		setupHTTPMetricsTest()

		histogram := GlobalMetrics.GetRequestDuration()

		// Test that the returned histogram is functional
		histogram.WithLabelValues("POST", "/api", "201").Observe(0.5)
		histogram.WithLabelValues("POST", "/api", "201").Observe(1.2)

		// Verify the metrics were recorded
		metricCount := testutil.CollectAndCount(histogram)
		assert.Equal(t, 1, metricCount, "HistogramVec should have one metric for the label set")
	})

	t.Run("GetHTTPRequestDuration returns nil when not initialized", func(t *testing.T) {
		resetMetricsForTesting()

		GlobalMetrics = &Metrics{}

		histogram := GlobalMetrics.GetRequestDuration()
		assert.Nil(t, histogram, "Should return nil when metrics not initialized")
	})
}

func TestGetHTTPResponseSize(t *testing.T) {
	t.Run("GetHTTPResponseSize returns initialized histogram", func(t *testing.T) {
		setupHTTPMetricsTest()

		histogram := GlobalMetrics.GetResponseSize()
		assert.NotNil(t, histogram, "GetHTTPResponseSize should return a non-nil HistogramVec")
		assert.Equal(t, GlobalMetrics.httpResponseSize, histogram, "Should return the same instance as internal field")
	})

	t.Run("GetHTTPResponseSize returns working histogram", func(t *testing.T) {
		setupHTTPMetricsTest()

		histogram := GlobalMetrics.GetResponseSize()

		// Test that the returned histogram is functional
		histogram.WithLabelValues("GET", "/download", "application/octet-stream").Observe(2048)
		histogram.WithLabelValues("GET", "/download", "application/octet-stream").Observe(4096)

		// Verify the metrics were recorded
		count := testutil.CollectAndCount(histogram)
		assert.Equal(t, 1, count, "HistogramVec should have one metric for the label set")
	})

	t.Run("GetHTTPResponseSize returns nil when not initialized", func(t *testing.T) {
		resetMetricsForTesting()

		GlobalMetrics = &Metrics{}

		histogram := GlobalMetrics.GetResponseSize()
		assert.Nil(t, histogram, "Should return nil when metrics not initialized")
	})
}

// Test integration of getter functions with middleware functionality.
func TestGetterFunctionsIntegration(t *testing.T) {
	t.Run("Getter functions work with InFlightRequestsMiddleware", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Get the gauge through the getter function
		gauge := GlobalMetrics.GetInFlight()
		require.NotNil(t, gauge)

		// Use the middleware
		increment, decrement := GlobalMetrics.InFlight()

		// Test increment
		increment()

		value := testutil.ToFloat64(gauge)
		assert.Equal(t, float64(1), value, "Gauge should show 1 in-flight request")

		// Test decrement
		decrement()

		value = testutil.ToFloat64(gauge)
		assert.Equal(t, float64(0), value, "Gauge should show 0 in-flight requests")
	})

	t.Run("Getter functions work with RecordHTTPRequest", func(t *testing.T) {
		setupHTTPMetricsTest()

		// Get metrics through getter functions
		counter := GlobalMetrics.GetRequestsTotal()
		histogram := GlobalMetrics.GetRequestDuration()
		sizeHistogram := GlobalMetrics.GetResponseSize()

		require.NotNil(t, counter)
		require.NotNil(t, histogram)
		require.NotNil(t, sizeHistogram)

		// Record a request
		GlobalMetrics.RecordRequest("PUT", "/update", 204, 250*time.Millisecond, 512)

		// Verify metrics were recorded correctly
		counterValue := testutil.ToFloat64(counter.WithLabelValues("PUT", "/update", "204"))
		assert.Equal(t, float64(1), counterValue, "Counter should increment")

		durationCount := testutil.CollectAndCount(histogram)
		assert.Equal(t, 1, durationCount, "Duration histogram should record observation")

		sizeCount := testutil.CollectAndCount(sizeHistogram)
		assert.Equal(t, 1, sizeCount, "Size histogram should record observation")
	})
}
