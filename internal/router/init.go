package router

import (
	"context"
	"errors"
	"net/http"
	"strconv"
	"time"

	"archive_manager/internal/config"
	"archive_manager/internal/router/handler"
	"archive_manager/internal/router/middleware"

	log "github.com/dredfort42/go_logger"
	"github.com/gin-gonic/gin"
)

var server *http.Server

func Init(cancel context.CancelFunc) error {
	setGinMode()

	router := gin.New()

	// Add global middleware in order of execution
	router.Use(middleware.RequestID()) // 1. Correlation ID (must be first so logs/other middleware get the ID)
	router.Use(middleware.SessionID()) // 2. Ensure session id
	router.Use(middleware.Metrics())   // 3. Metrics (wraps the whole request)

	router.Use(gin.Logger())   // 4. Access log (can include request ID)
	router.Use(gin.Recovery()) // 5. Panic recovery (after logger so panics get logged)

	publicGroup := router.Group("/")
	publicGroup.Use(middleware.NoCache())
	{
		publicGroup.GET("/health", handler.Health())
		publicGroup.GET("/metrics", handler.Metrics())
	}

	// Configure server
	server = &http.Server{
		Addr:           config.App.Server.Host + ":" + strconv.Itoa(config.App.Server.Port),
		Handler:        router,
		ReadTimeout:    config.App.Server.ReadTimeout,
		WriteTimeout:   config.App.Server.WriteTimeout,
		IdleTimeout:    config.App.Server.IdleTimeout,
		MaxHeaderBytes: config.App.Server.MaxHeaderBytes,
	}

	// Start server in a goroutine
	go func(cancel context.CancelFunc) {
		log.Info.Printf("Server starting on %s:%d", config.App.Server.Host, config.App.Server.Port)

		if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			cancel()
			log.Error.Printf("Server failed to start: %v", err)
		}
	}(cancel)

	return nil
}

func Close() {
	// Create a deadline for shutdown
	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
	defer cancel()

	// Attempt graceful shutdown
	if err := server.Shutdown(ctx); err != nil {
		log.Error.Fatalf("API server forced to shutdown: %v", err)
	}

	log.Info.Println("API server stopped")
}

// setGinMode sets the Gin mode based on the DEBUG environment variable
func setGinMode() {
	if config.App.Debug {
		gin.SetMode(gin.DebugMode)
	} else {
		gin.SetMode(gin.ReleaseMode)
	}
}
