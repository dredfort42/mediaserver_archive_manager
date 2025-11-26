package config

import (
	config "github.com/dredfort42/go_config_reader"
	"github.com/google/uuid"
)

func parseConfiguration(cfg *config.Config) {
	App.Debug = cfg.GetBool("debug")
	App.HeartbeatInterval = cfg.GetDuration("heartbeat_interval")
	App.UUID = uuid.New().String()

	App.Server = ServerConfig{
		Host:           cfg.GetString("archive_manager.host"),
		Port:           cfg.GetInt("archive_manager.port"),
		ReadTimeout:    cfg.GetDuration("archive_manager.read_timeout"),
		WriteTimeout:   cfg.GetDuration("archive_manager.write_timeout"),
		IdleTimeout:    cfg.GetDuration("archive_manager.idle_timeout"),
		MaxHeaderBytes: cfg.GetInt("archive_manager.max_header_bytes"),
	}

	App.ArchiveManager = ArchiveManagerConfig{
		StoragePath:      cfg.GetString("archive_manager.storage_path"),
		FragmentDuration: cfg.GetDuration("archive_manager.fragment_duration"),
		RetentionWorkers: cfg.GetInt("archive_manager.retention_workers"),
	}

	App.Database = DatabaseConfig{
		Driver:                 cfg.GetString("database.driver"),
		Host:                   cfg.GetString("database.host"),
		Port:                   cfg.GetInt("database.port"),
		Name:                   cfg.GetString("database.name"),
		User:                   cfg.GetString("database.user"),
		Password:               cfg.GetString("database.password"),
		SSLEnabled:             cfg.GetBool("database.ssl_enabled"),
		MaxOpenConn:            cfg.GetInt("database.max_open_conn"),
		MaxIdleConn:            cfg.GetInt("database.max_idle_conn"),
		MaxLifetime:            cfg.GetDuration("database.max_lifetime"),
		MaxIdleTime:            cfg.GetDuration("database.max_idle_time"),
		TableIFrameByteOffsets: cfg.GetString("database.table_iframe_byte_offsets"),
	}

	App.Kafka = KafkaConfig{
		Brokers:           cfg.GetStringSlice("kafka.brokers"),
		TopicSystemDigest: cfg.GetString("kafka.topic_system_digest"),
		TopicCameras:      cfg.GetString("kafka.topic_cameras"),
		ConsumerConfig:    cfg.GetNestedMap("kafka.consumer_config"),
		ProducerConfig:    cfg.GetNestedMap("kafka.producer_config"),
	}
}
