package model

type Frame struct {
	CameraID  string
	Timestamp int64
	Data      []byte
	IsIFrame  bool
}

type IFrameOffset struct {
	CameraID  string
	Timestamp int64
	Offset    int64
}
