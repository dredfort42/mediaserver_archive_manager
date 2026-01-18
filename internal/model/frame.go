package model

type Frame struct {
	CameraID     string
	Timestamp    int64
	Data         []byte
	IsVideoFrame bool
	IsIFrame     bool
}

type BatchMetadata struct {
	CameraID           string // ID of the camera
	IFrameTimeCode     int64  // Timestamp of the keyframe in milliseconds
	IFrameOffset       int64  // Byte offset in the file
	VideoFramesInBatch int64  // Video frames in the batch
}
