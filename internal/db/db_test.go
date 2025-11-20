package db

import (
	"os"
	"strings"
	"testing"

	"archive_manager/internal/config"
)

func TestBuildDSN(t *testing.T) {
	c := config.DatabaseConfig{
		Driver:     "postgres",
		Host:       "localhost",
		Port:       5432,
		Name:       "testdb",
		User:       "user",
		Password:   "pass",
		SSLEnabled: false,
	}
	dsn, err := buildDSN(c, "testapp")
	if err != nil {
		t.Fatalf("BuildDSN error: %v", err)
	}
	if !strings.Contains(dsn, "postgres://user:pass@localhost:5432/testdb") {
		t.Fatalf("unexpected dsn path: %s", dsn)
	}
	if !strings.Contains(dsn, "sslmode=disable") {
		t.Fatalf("ssl mode not set: %s", dsn)
	}
	if !strings.Contains(dsn, "application_name=testapp") {
		t.Fatalf("app name not present: %s", dsn)
	}
}

// Optional integration test; requires a running local Postgres. Guarded by env DB_INTEGRATION=1
func TestOpenOptional(t *testing.T) {
	if os.Getenv("DB_INTEGRATION") != "1" {
		t.Skip("skipping integration test; set DB_INTEGRATION=1 to enable")
	}
	// Provide minimal App config
	config.App.AppName = "mediaserver_manager_test"
	config.App.Database = config.DatabaseConfig{
		Driver:      "postgres",
		Host:        os.Getenv("PGHOST"),
		Port:        5432,
		Name:        "postgres",
		User:        os.Getenv("PGUSER"),
		Password:    os.Getenv("PGPASSWORD"),
		SSLEnabled:  false,
		MaxOpenConn: 5,
		MaxIdleConn: 2,
	}
	if config.App.Database.Host == "" {
		config.App.Database.Host = "localhost"
	}
	if config.App.Database.User == "" {
		config.App.Database.User = "postgres"
	}

	dbh, err := open()
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}
	t.Cleanup(func() { _ = dbh.Close() })
}
