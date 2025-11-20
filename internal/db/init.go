package db

import (
	"context"
	"database/sql"
	"errors"
	"fmt"
	"net/url"
	"time"

	"archive_manager/internal/config"

	log "github.com/dredfort42/go_logger"
	_ "github.com/jackc/pgx/v5/stdlib"
)

var db *sql.DB

// Init initializes a database/sql DB using pgx stdlib and applies pool tuning from config.
func Init() (err error) {
	log.Info.Println("Connecting to PostgreSQL database...")

	db, err = open()
	if err != nil {
		return
	}

	if exists, err := isTableExists(config.App.Database.TableIFrameByteOffsets); err != nil {
		return fmt.Errorf("checking table %s existence failed: %w", config.App.Database.TableIFrameByteOffsets, err)
	} else if !exists {
		return fmt.Errorf("required table %s does not exist", config.App.Database.TableIFrameByteOffsets)
	} else {
		log.Info.Printf("Table %s exists and is accessible", config.App.Database.TableIFrameByteOffsets)
	}

	return
}

// buildDSN builds a PostgreSQL DSN using pgx v5 stdlib driver and app config.
// It uses a URL form: postgres://user:pass@host:port/db?sslmode=...&application_name=...&connect_timeout=...
func buildDSN(c config.DatabaseConfig, appName string) (string, error) {
	if c.Driver != "postgres" {
		return "", fmt.Errorf("unsupported database driver: %s", c.Driver)
	}

	if c.Host == "" || c.Name == "" || c.User == "" || c.Port == 0 {
		return "", fmt.Errorf("incomplete database config: credentials, host, port and name are required")
	}

	u := &url.URL{
		Scheme: "postgres",
		User:   url.UserPassword(c.User, c.Password),
		Host:   fmt.Sprintf("%s:%d", c.Host, c.Port),
		Path:   "/" + c.Name,
	}

	// Query parameters for performance and safety
	query := url.Values{}
	if c.SSLEnabled {
		query.Set("sslmode", "require")
	} else {
		query.Set("sslmode", "disable")
	}

	query.Set("connect_timeout", "5") // seconds
	query.Set("application_name", appName)
	query.Set("timezone", "UTC")

	u.RawQuery = query.Encode()

	return u.String(), nil
}

// open opens a new *sql.DB connection using app config and runs migrations. Caller should Close().
func open() (*sql.DB, error) {
	dsn, err := buildDSN(config.App.Database, config.App.AppName)
	if err != nil {
		return nil, err
	}

	// Use pgx stdlib driver to leverage its performance while keeping *sql.DB API.
	db, err := sql.Open("pgx", dsn)
	if err != nil {
		return nil, fmt.Errorf("sql.Open failed: %w", err)
	}

	// Pool tuning
	if maxOpen := config.App.Database.MaxOpenConn; maxOpen > 0 {
		db.SetMaxOpenConns(maxOpen)
	}
	if maxIdle := config.App.Database.MaxIdleConn; maxIdle >= 0 {
		db.SetMaxIdleConns(maxIdle)
	}
	if life := config.App.Database.MaxLifetime; life > 0 {
		db.SetConnMaxLifetime(life)
	}
	if idle := config.App.Database.MaxIdleTime; idle > 0 {
		db.SetConnMaxIdleTime(idle)
	}

	// Verify connectivity with a bounded context
	pingCtx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if err := db.PingContext(pingCtx); err != nil {
		_ = db.Close()
		return nil, fmt.Errorf("database ping failed: %w", err)
	}

	log.Info.Println("PostgreSQL connection established")

	return db, nil
}

// isTableExists checks if a table with the given name exists in the connected database.
func isTableExists(tableName string) (bool, error) {
	var exists bool
	query := `
		SELECT EXISTS (
			SELECT 1
			FROM information_schema.tables
			WHERE table_name = $1
		);
	`
	err := db.QueryRow(query, tableName).Scan(&exists)
	if err != nil {
		return false, fmt.Errorf("isTableExists query failed: %w", err)
	}
	return exists, nil
}

// Close closes the global database connection pool.
func Close() {
	if db != nil {
		_ = db.Close()

		log.Info.Println("PostgreSQL connection closed")
	}
}

// IsNoRowsErr reports whether the provided error represents sql.ErrNoRows.
func IsNoRowsErr(err error) bool {
	if err == nil {
		return false
	}

	return errors.Is(err, sql.ErrNoRows)
}
