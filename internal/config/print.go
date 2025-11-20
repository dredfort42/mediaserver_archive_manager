package config

import (
	"log"
)

func printConfiguration(l *log.Logger) {
	l.Println("========================================")
	l.Println()

	l.Printf("Application Configuration:\n")
	l.Printf("├─ Name: %s v%s\n", App.AppName, App.Version)
	l.Printf("├─ Environment: %s\n", App.Environment)
	l.Printf("├─ UUID: %s\n", App.UUID)
	l.Printf("├─ Heartbeat Interval: %v\n", App.HeartbeatInterval)
	l.Printf("└─ Debug Mode: %v\n", App.Debug)
	l.Println()

	l.Printf("Server Configuration:\n")
	l.Printf("├─ Host: %s\n", App.Server.Host)
	l.Printf("├─ Port: %d\n", App.Server.Port)
	l.Printf("├─ Read Timeout: %v\n", App.Server.ReadTimeout)
	l.Printf("├─ Write Timeout: %v\n", App.Server.WriteTimeout)
	l.Printf("├─ Idle Timeout: %v\n", App.Server.IdleTimeout)
	l.Printf("└─ Max Header Bytes: %d\n", App.Server.MaxHeaderBytes)
	l.Println()

	l.Printf("Archive Manager Configuration:\n")
	l.Printf("├─ Storage Path: %s\n", App.ArchiveManager.StoragePath)
	l.Printf("└─ Fragment Length: %v\n", App.ArchiveManager.FragmentLength)
	l.Println()

	l.Printf("Database Configuration:\n")
	l.Printf("├─ Driver: %s\n", App.Database.Driver)
	l.Printf("├─ Address: %s:%d\n", App.Database.Host, App.Database.Port)
	l.Printf("├─ Database: %s\n", App.Database.Name)
	l.Printf("├─ User: %s\n", App.Database.User)
	l.Printf("├─ SSL Enabled: %v\n", App.Database.SSLEnabled)
	l.Printf("├─ Connection Pool: %d/%d (max idle/open)\n", App.Database.MaxIdleConn, App.Database.MaxOpenConn)
	l.Printf("├─ Max Lifetime: %v\n", App.Database.MaxLifetime)
	l.Printf("├─ Max Idle Time: %v\n", App.Database.MaxIdleTime)
	l.Printf("└─ Table IFrame Byte Offsets: %s\n", App.Database.TableIFrameByteOffsets)
	l.Println()

	l.Printf("Kafka Configuration:\n")
	l.Printf("├─ Brokers: %v\n", App.Kafka.Brokers)
	l.Printf("├─ Topic System Digest: %s\n", App.Kafka.TopicSystemDigest)
	l.Printf("├─ Topic Cameras: %s\n", App.Kafka.TopicCameras)
	l.Printf("├─ Consumer Config: %v\n", App.Kafka.ConsumerConfig)
	l.Printf("└─ Producer Config: %v\n", App.Kafka.ProducerConfig)

	l.Println()
	l.Println("========================================")
}
