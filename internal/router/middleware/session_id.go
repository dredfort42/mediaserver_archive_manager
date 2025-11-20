package middleware

import (
	"net/http"
	"time"

	"archive_manager/internal/router/model"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
)

func SessionID() gin.HandlerFunc {
	return func(c *gin.Context) {
		sid, err := c.Cookie(model.SessionCookie)
		if err != nil || sid == "" {
			sid = uuid.NewString()
			http.SetCookie(c.Writer, &http.Cookie{
				Name:     model.SessionCookie,
				Value:    sid,
				Path:     "/",
				Expires:  time.Now().Add(30 * 24 * time.Hour),
				HttpOnly: true,
				Secure:   true,
				// SameSite: http.SameSiteLaxMode,

				// TMP for local dev over HTTP
				SameSite: http.SameSiteNoneMode,
			})
		}

		c.Set(model.CtxSessionID, sid)

		c.Next()
	}
}
