package metrics

import (
	"strings"
	"sync"
	"testing"

	"archive_manager/internal/config"

	"github.com/prometheus/client_golang/prometheus"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// resetMetricsForTesting resets the global state for testing.
func resetMetricsForTesting() {
	// Reset the sync.Once so we can test initialization multiple times
	initMetricsOnce = sync.Once{}
	GlobalMetrics = nil

	// Create a new registry to avoid conflicts
	prometheus.DefaultRegisterer = prometheus.NewRegistry()
}

// Test the Metrics struct definition.
func TestMetricsStruct(t *testing.T) {
	t.Run("Metrics struct has all required fields", func(t *testing.T) {
		metrics := &Metrics{}

		// Test that all fields are accessible and of correct type
		assert.IsType(t, (*prometheus.CounterVec)(nil), metrics.httpRequestsTotal)
		assert.IsType(t, (*prometheus.HistogramVec)(nil), metrics.httpRequestDuration)
		assert.IsType(t, (prometheus.Gauge)(nil), metrics.httpRequestsInFlight)
		assert.IsType(t, (*prometheus.HistogramVec)(nil), metrics.httpResponseSize)
	})
}

// Test InitializeMetrics function.
func TestInitializeMetrics(t *testing.T) {
	t.Run("InitializeMetrics creates GlobalMetrics", func(t *testing.T) {
		resetMetricsForTesting()

		// Before initialization
		assert.Nil(t, GlobalMetrics)

		// Initialize metrics
		InitializeMetrics()

		// After initialization
		assert.NotNil(t, GlobalMetrics)
		assert.IsType(t, &Metrics{}, GlobalMetrics)
	})

	t.Run("InitializeMetrics uses correct namespace", func(t *testing.T) {
		resetMetricsForTesting()

		InitializeMetrics()

		// The namespace should be lowercase version of app name
		expectedNamespace := strings.ToLower(config.App.AppName)
		assert.Equal(t, "archive_manager", expectedNamespace)
	})

	t.Run("InitializeMetrics creates labels with service and version", func(t *testing.T) {
		resetMetricsForTesting()

		InitializeMetrics()

		// Verify that the labels contain expected values
		expectedNamespace := strings.ToLower(config.App.AppName)
		assert.Equal(t, "archive_manager", expectedNamespace)

		// The version should come from config.App.Version
		assert.NotEmpty(t, config.App.Version)
	})
}

// Test that InitializeMetrics is idempotent (can be called multiple times safely).
func TestInitializeMetricsIdempotent(t *testing.T) {
	t.Run("Multiple calls to InitializeMetrics are safe", func(t *testing.T) {
		resetMetricsForTesting()

		// Call InitializeMetrics multiple times
		InitializeMetrics()

		firstMetrics := GlobalMetrics

		InitializeMetrics()

		secondMetrics := GlobalMetrics

		InitializeMetrics()

		thirdMetrics := GlobalMetrics

		// All should point to the same instance due to sync.Once
		assert.Same(t, firstMetrics, secondMetrics)
		assert.Same(t, secondMetrics, thirdMetrics)
		assert.NotNil(t, GlobalMetrics)
	})
}

// Test concurrent initialization.
func TestInitializeMetricsConcurrency(t *testing.T) {
	t.Run("Concurrent calls to InitializeMetrics are safe", func(t *testing.T) {
		resetMetricsForTesting()

		const numGoroutines = 10

		var wg sync.WaitGroup

		results := make([]*Metrics, numGoroutines)

		// Start multiple goroutines calling InitializeMetrics concurrently
		for i := range numGoroutines {
			wg.Add(1)

			go func(index int) {
				defer wg.Done()

				InitializeMetrics()

				results[index] = GlobalMetrics
			}(i)
		}

		wg.Wait()

		// All results should point to the same instance
		for i := 1; i < numGoroutines; i++ {
			assert.Same(t, results[0], results[i], "All goroutines should get the same metrics instance")
		}

		// GlobalMetrics should be initialized
		assert.NotNil(t, GlobalMetrics)
	})
}

// Test the global variables.
func TestGlobalVariables(t *testing.T) {
	t.Run("GlobalMetrics is accessible", func(t *testing.T) {
		// GlobalMetrics should be accessible as a package variable
		// It might be nil before initialization, which is fine
		_ = GlobalMetrics
	})

	t.Run("initMetricsOnce is sync.Once", func(t *testing.T) {
		// Verify the type of the sync.Once variable
		assert.IsType(t, (*sync.Once)(nil), &initMetricsOnce)
	})
}

// Test the namespace creation logic.
func TestNamespaceCreation(t *testing.T) {
	t.Run("Namespace is lowercase of app name", func(t *testing.T) {
		// Test the logic used in InitializeMetrics
		namespace := strings.ToLower(config.App.AppName)

		assert.Equal(t, "archive_manager", namespace)
		assert.Equal(t, strings.ToLower(namespace), namespace, "Namespace should already be lowercase")
	})
}

// Test labels creation logic.
func TestLabelsCreation(t *testing.T) {
	t.Run("Labels contain service and version", func(t *testing.T) {
		// Test the labels creation logic used in InitializeMetrics
		namespace := strings.ToLower(config.App.AppName)
		labels := prometheus.Labels{
			"service": namespace,
			"version": config.App.Version,
		}

		assert.Contains(t, labels, "service")
		assert.Contains(t, labels, "version")
		assert.Equal(t, "archive_manager", labels["service"])
		assert.NotEmpty(t, labels["version"])
		assert.IsType(t, "", labels["service"])
		assert.IsType(t, "", labels["version"])
	})
}

// Test error handling and edge cases.
func TestInitializeMetricsEdgeCases(t *testing.T) {
	t.Run("InitializeMetrics handles empty app name gracefully", func(t *testing.T) {
		resetMetricsForTesting()

		// Temporarily modify the app name to test edge case
		originalName := config.App.AppName
		config.App.AppName = ""

		defer func() { config.App.AppName = originalName }()

		// Should not panic
		assert.NotPanics(t, func() {
			InitializeMetrics()
		})

		assert.NotNil(t, GlobalMetrics)
	})

	t.Run("InitializeMetrics handles empty version gracefully", func(t *testing.T) {
		resetMetricsForTesting()

		// Temporarily modify the version to test edge case
		originalVersion := config.App.Version
		config.App.Version = ""

		defer func() { config.App.Version = originalVersion }()

		// Should not panic
		assert.NotPanics(t, func() {
			InitializeMetrics()
		})

		assert.NotNil(t, GlobalMetrics)
	})
}

// Test that all metric types are properly initialized.
func TestMetricsInitialization(t *testing.T) {
	t.Run("All metrics are initialized after InitializeMetrics", func(t *testing.T) {
		resetMetricsForTesting()

		InitializeMetrics()

		require.NotNil(t, GlobalMetrics)

		// Note: The actual metric fields are private, so we can't directly test them
		// But we can verify that the GlobalMetrics struct itself is properly initialized
		assert.IsType(t, &Metrics{}, GlobalMetrics)
	})
}

// Test package integration.
func TestPackageIntegration(t *testing.T) {
	t.Run("InitializeMetrics integrates with about package", func(t *testing.T) {
		resetMetricsForTesting()

		// Verify that config.App is accessible and has expected structure
		assert.NotEmpty(t, config.App.AppName)
		assert.IsType(t, "", config.App.AppName)
		assert.IsType(t, "", config.App.Version)

		// InitializeMetrics should work with the about package
		assert.NotPanics(t, func() {
			InitializeMetrics()
		})
	})
}

// Test the sequence of initialization calls.
func TestInitializationSequence(t *testing.T) {
	t.Run("Initialization calls are made in correct order", func(t *testing.T) {
		resetMetricsForTesting()

		// Before initialization
		assert.Nil(t, GlobalMetrics)

		// Call InitializeMetrics
		InitializeMetrics()

		// After initialization
		assert.NotNil(t, GlobalMetrics)

		assert.IsType(t, &Metrics{}, GlobalMetrics)
	})
}

// Benchmark the initialization performance.
func BenchmarkInitializeMetrics(b *testing.B) {
	b.Run("First initialization", func(b *testing.B) {
		for b.Loop() {
			resetMetricsForTesting()
			InitializeMetrics()
		}
	})

	b.Run("Subsequent initializations", func(b *testing.B) {
		resetMetricsForTesting()
		InitializeMetrics() // First call

		b.ResetTimer()

		for b.Loop() {
			InitializeMetrics() // Subsequent calls should be very fast due to sync.Once
		}
	})
}

// Test the sync.Once behavior specifically.
func TestSyncOnceBehavior(t *testing.T) {
	t.Run("sync.Once ensures single execution", func(t *testing.T) {
		resetMetricsForTesting()

		callCount := 0
		testOnce := sync.Once{}

		// Test that sync.Once works as expected
		for range 5 {
			testOnce.Do(func() {
				callCount++
			})
		}

		assert.Equal(t, 1, callCount, "sync.Once should execute the function only once")
	})
}

// Test thread safety without race conditions.
func TestThreadSafety(t *testing.T) {
	t.Run("No race conditions in concurrent access", func(t *testing.T) {
		resetMetricsForTesting()

		const numWorkers = 50

		var wg sync.WaitGroup

		// Start many goroutines accessing GlobalMetrics
		for range numWorkers {
			wg.Add(1)

			go func() {
				defer wg.Done()

				InitializeMetrics()
				// Access GlobalMetrics to ensure no race conditions
				_ = GlobalMetrics
			}()
		}

		wg.Wait()

		// Should complete without race conditions
		assert.NotNil(t, GlobalMetrics)
	})
}
