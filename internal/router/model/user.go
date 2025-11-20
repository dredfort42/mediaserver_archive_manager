package model

import (
	"encoding/json"
	"time"
)

const (
	AccessTokenCookie  = "access_token"
	RefreshTokenCookie = "refresh_token"
	SessionCookie      = "sid"
)

const (
	CtxRequestID = "request_id"
	CtxSessionID = "session_id"
	CtxAccessJTI = "access_jti"
	CtxUserID    = "user_id"
	CtxUserRole  = "user_role"
)

type User struct {
	ID          int        `json:"id"`
	Email       string     `json:"email"`
	Role        string     `json:"role"`
	FirstName   *string    `json:"first_name,omitempty"`
	LastName    *string    `json:"last_name,omitempty"`
	IsActive    bool       `json:"is_active"`
	CreatedAt   time.Time  `json:"created_at"`
	UpdatedAt   *time.Time `json:"updated_at,omitempty"`
	LastLoginAt *time.Time `json:"last_login_at,omitempty"`
	ExpiresAt   *int64     `json:"expires_at,omitempty"` // access token expiration date (epoch seconds)
}

type CreateUserRequest struct {
	Email    string `json:"email" binding:"required"`
	Password string `json:"password" binding:"required"`
	Role     string `json:"role" binding:"required"`
}

type UpdateUserRequest struct {
	Email    *string `json:"email,omitempty"`
	IsActive *bool   `json:"is_active,omitempty"`
}

type ChangePasswordRequest struct {
	CurrentPassword string `json:"current_password,omitempty"`
	NewPassword     string `json:"new_password" binding:"required"`
}

type Role struct {
	ID          int    `json:"id"`
	Name        string `json:"name"`
	Description string `json:"description"`
}

type UpdateUserRoleRequest struct {
	Role string `json:"role" binding:"required"`
}

type RefreshToken struct {
	ID         int
	UserID     int
	TokenHash  string
	AccessJTI  string
	CreatedAt  time.Time
	ExpiresAt  time.Time
	Revoked    bool
	DeviceInfo string
}

type Session struct {
	ID         int       `json:"id"`
	AccessJTI  string    `json:"-"`
	DeviceInfo string    `json:"device_info"`
	CreatedAt  time.Time `json:"created_at"`
	ExpiresAt  time.Time `json:"expires_at"`
	IsCurrent  bool      `json:"is_current"`
}

type Profile struct {
	ID          int             `json:"id"`
	UserID      int             `json:"user_id"`
	FirstName   string          `json:"first_name"`
	LastName    string          `json:"last_name"`
	Preferences json.RawMessage `json:"preferences"`
	UpdatedAt   time.Time       `json:"updated_at"`
}

type UpdateUserProfileRequest struct {
	FirstName   *string          `json:"first_name,omitempty"`
	LastName    *string          `json:"last_name,omitempty"`
	Preferences *json.RawMessage `json:"preferences,omitempty"`
}
