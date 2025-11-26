# Multi-stage build for optimized production image
# Use the official Go image as the build environment
FROM golang:1.25-alpine AS manager_builder

# Set the working directory inside the container
WORKDIR /app

# Copy go mod and sum files for dependency caching
COPY go.mod go.sum ./

# Download dependencies
RUN go mod download

# Verify dependencies
RUN go mod verify

# Copy the source code
COPY . .

# Build the application with optimizations
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o archive_manager ./cmd/archive_manager

# Production stage - use minimal base image
FROM alpine:3.20

# Install runtime dependencies, node-exporter, and cAdvisor
RUN apk --no-cache add \
    wget \
    && rm -rf /var/cache/apk/*


# Create non-root user for security
RUN addgroup -g 1001 -S archive_manager && \
    adduser -u 1001 -S archive_manager -G archive_manager

# Set working directory
WORKDIR /app

# Copy the binary from builder stage
COPY --from=manager_builder /app/archive_manager .

# Switch to non-root user
USER archive_manager

# Expose the application port (default 8090 based on config)
EXPOSE 9090

# Health check with correct endpoint for all services
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD wget --no-verbose --tries=1 -O- http://localhost:9090/health > /dev/null 2>&1 || exit 1

# Set environment variables
ENV GIN_MODE=release
ENV TZ=UTC

# Labels for better maintainability
ARG SERVER_VERSION=0.0.0
LABEL maintainer="MediaServer Team"
LABEL description="Camera Archive Manager Service"
LABEL version="${SERVER_VERSION}"

# Run the application
CMD ["./archive_manager", "--config", "config.ini"]