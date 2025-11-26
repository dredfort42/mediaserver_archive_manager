package config

import (
	"context"
	"fmt"
	"time"

	config "github.com/dredfort42/go_config_reader"
	log "github.com/dredfort42/go_logger"
)

// ServerConfig is a struct for streams server configuration
type ServerConfig struct {
	Host           string
	Port           int
	ReadTimeout    time.Duration
	WriteTimeout   time.Duration
	IdleTimeout    time.Duration
	MaxHeaderBytes int
}

// ArchiveManagerConfig represents archive manager specific configuration
type ArchiveManagerConfig struct {
	StoragePath      string
	FragmentDuration time.Duration
	RetentionWorkers int
}

// DatabaseConfig represents database configuration
type DatabaseConfig struct {
	Driver                 string
	Host                   string
	Port                   int
	Name                   string
	User                   string
	Password               string
	SSLEnabled             bool
	MaxOpenConn            int
	MaxIdleConn            int
	MaxLifetime            time.Duration
	MaxIdleTime            time.Duration
	TableIFrameByteOffsets string
}

// KafkaConfig is a struct for Kafka configuration
type KafkaConfig struct {
	Brokers           []string
	TopicSystemDigest string
	TopicCameras      string
	ConsumerConfig    map[string]any
	ProducerConfig    map[string]any
}

// AppConfig represents complete application configuration
type AppConfig struct {
	UUID              string
	AppName           string
	Version           string
	Environment       string
	HeartbeatInterval time.Duration
	Debug             bool
	Server            ServerConfig
	ArchiveManager    ArchiveManagerConfig
	Database          DatabaseConfig
	Kafka             KafkaConfig
}

// App is the global application configuration
var App AppConfig

func Init(ctx context.Context, configPath string) error {
	log.Info.Println("Initializing config...")

	cfg, err := config.New()
	if err != nil {
		return fmt.Errorf("failed to create config instance: %v", err)
	}

	// Define comprehensive defaults
	defaults := map[string]any{
		"debug":              false,
		"heartbeat_interval": 10 * time.Second,

		"archive_manager.host":             "localhost",
		"archive_manager.port":             9090,
		"archive_manager.read_timeout":     "30s",
		"archive_manager.write_timeout":    "30s",
		"archive_manager.idle_timeout":     "120s",
		"archive_manager.max_header_bytes": 1048576, // 1MB

		"archive_manager.fragment_duration": "5m",
		"archive_manager.retention_workers": 5,

		"database.driver":        "postgres",
		"database.ssl_enabled":   false,
		"database.max_open_conn": 25,
		"database.max_idle_conn": 5,
		"database.max_lifetime":  "5m",
		"database.max_idle_time": "2m",

		"kafka.consumer_config": map[string]any{"auto.offset.reset": "latest"},
		"kafka.producer_config": map[string]any{"acks": "all"},
	}

	// Try to load from config file with defaults and validation
	opts := &config.LoadOptions{
		DefaultValues: defaults,
		RequiredKeys: []string{
			"archive_manager.storage_path",

			"database.host",
			"database.port",
			"database.name",
			"database.user",
			"database.password",
			"database.table_iframe_byte_offsets",

			"kafka.brokers",
			"kafka.topic_system_digest",
			"kafka.topic_cameras",
		},
		ValidationFunc: func(data map[string]any) error {
			// Validate port range
			if port, ok := data["archive_manager.port"].(int); ok {
				if port < 1024 || port > 65535 {
					return fmt.Errorf("archive manager port must be between 1024 and 65535, got %d", port)
				}
			}

			return nil
		},
		IgnoreEnv: false,
	}

	var configFiles []string

	if configPath != "" {
		configFiles = append(configFiles, configPath)
	} else {
		configFiles = []string{"config.yaml", "config.yml", "config.json", "config.ini"}
	}

	loaded := false

	for _, file := range configFiles {
		err := cfg.LoadFromFile(file, opts)
		if err == nil {
			log.Info.Println("Loaded configuration from:", file)
			loaded = true
			break
		} else {
			log.Warning.Printf("Failed to load configuration from %s: %v", file, err)
		}
	}

	if !loaded {
		return fmt.Errorf("failed to load configuration from any of the specified files: %v", configFiles)
	}

	log.Info.Println("Configuration loaded successfully")

	parseConfiguration(cfg)

	if App.Debug {
		log.Debug.Println("Debug mode is enabled")
		printConfiguration(log.Debug)
	}

	return nil
}
