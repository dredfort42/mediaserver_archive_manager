package main

import (
	"context"
	"flag"
	"fmt"
	"os"
	"os/signal"
	"sync"
	"syscall"

	"archive_manager/internal/broker"
	"archive_manager/internal/config"
	// "archive_manager/internal/controller"
	"archive_manager/internal/router"
	"archive_manager/internal/db"

	log "github.com/dredfort42/go_logger"
)

// Application constants.
const (
	Version     = "0.0.0"
	AppName     = "mediaserver_archive_manager"
	Environment = "development"
)

func main() {
	var (
		versionFlag = flag.Bool("version", false, "Show version information")
		helpFlag    = flag.Bool("help", false, "Show help information")
		configFlag  = flag.String("config", "", "Path to config file")
	)

	flag.Parse()

	if *versionFlag {
		fmt.Printf("%s version %s\n", AppName, Version)
		os.Exit(0)
	}

	if *helpFlag {
		showUsage()
		os.Exit(0)
	}

	log.Info.Println("An archive manager is starting...")
	defer log.Info.Println("An archive manager stopped successfully")

	ctx, cancel := signal.NotifyContext(context.Background(), os.Interrupt, syscall.SIGINT, syscall.SIGTERM)
	defer cancel()

	config.App.AppName = AppName
	config.App.Version = Version
	config.App.Environment = Environment

	if err := config.Init(ctx, *configFlag); err != nil {
		log.Error.Println("Config initialization failed:", err)
		showUsage()
		cancel()
	}

	if err := db.Init(); err != nil {
		log.Error.Println("Database initialization failed:", err)
		cancel()
	}
	defer db.Close()

	var wg sync.WaitGroup

	if err := broker.Init(ctx, cancel, &wg); err != nil {
		log.Error.Println("Broker initialization failed:", err)
		cancel()
	}
	defer broker.Close()

	// // if err := controller.Init(ctx, &wg); err != nil {
	// // 	log.Error.Println("Controller initialization failed:", err)
	// // 	cancel()
	// // }

	if err := router.Init(cancel); err != nil {
		log.Error.Println("Router initialization failed:", err)
		cancel()
	}
	defer router.Close()

	wg.Add(1)
	go broker.ServiceDigestHeartbeat(&wg)

	<-ctx.Done()
	log.Warning.Println("Termination signal received")
	log.Info.Println("An archive manager is stopping...")

	// Wait for all services to finish
	wg.Wait()
}

// showUsage displays the usage information for the application.
func showUsage() {
	fmt.Printf(`%s - MediaServer an archive manager

Usage:
  %s [flags]

Flags:
  --version      Show version information and exit
  --help         Show this help message and exit
  --config PATH  Specify path to config file.
                Supports YAML, JSON and INI formats.

Example:
  %s --config /path/to/config.yaml

`, AppName, AppName, AppName)
}
