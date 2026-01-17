package model

type Frame struct {
	CameraID     string
	Timestamp    int64
	Data         []byte
	IsVideoFrame bool
	IsIFrame     bool
}

type BatchMetadata struct {
	CameraID               string // ID of the camera
	IFrameTimestamp        int64  // Timestamp of the keyframe in milliseconds
	IFrameOffset           int64  // Byte offset in the file
	IFramePacketNumInBatch int64  // Packet number within the batch
	TotalPackets           int64  // Total packets in the batch (0 if batch not complete)
}
