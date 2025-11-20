package middleware

import (
	"archive_manager/internal/router/model"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
)

func RequestID() gin.HandlerFunc {
	return func(c *gin.Context) {
		rid := c.GetHeader("X-Request-ID")
		if rid == "" {
			rid = uuid.NewString()
		}
		c.Set(model.CtxRequestID, rid)
		c.Writer.Header().Set("X-Request-ID", rid)
		c.Next()
	}
}
