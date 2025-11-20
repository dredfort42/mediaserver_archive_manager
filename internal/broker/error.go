package broker

func getKafkaErrorMessage(errorCode int16) string {
	switch errorCode {
	case 0:
		return "No error"
	case 1:
		return "Offset out of range"
	case 2:
		return "Corrupt message"
	case 3:
		return "Unknown topic or partition"
	case 4:
		return "Invalid fetch size"
	case 5:
		return "Leader not available"
	case 6:
		return "Not leader for partition"
	case 7:
		return "Request timed out"
	case 8:
		return "Broker not available"
	case 9:
		return "Replica not available"
	case 10:
		return "Message too large"
	case 11:
		return "Stale controller epoch"
	case 12:
		return "Offset metadata too large"
	case 13:
		return "Network exception"
	case 14:
		return "Coordinator load in progress"
	case 15:
		return "Coordinator not available"
	case 16:
		return "Not coordinator"
	case 17:
		return "Invalid topic exception"
	case 18:
		return "Record list too large"
	case 19:
		return "Not enough replicas"
	case 20:
		return "Not enough replicas after append"
	case 21:
		return "Invalid required acks"
	case 22:
		return "Illegal generation"
	case 23:
		return "Inconsistent group protocol"
	case 24:
		return "Invalid group id"
	case 25:
		return "Unknown member id"
	case 26:
		return "Invalid session timeout"
	case 27:
		return "Rebalance in progress"
	case 28:
		return "Invalid commit offset size"
	case 29:
		return "Topic authorization failed"
	case 30:
		return "Group authorization failed"
	case 31:
		return "Cluster authorization failed"
	case 32:
		return "Invalid timestamp"
	case 33:
		return "Unsupported SASL mechanism"
	case 34:
		return "Illegal SASL state"
	case 35:
		return "Unsupported version"
	case 36:
		return "Topic already exists"
	case 37:
		return "Invalid partitions"
	case 38:
		return "Invalid replication factor"
	case 39:
		return "Invalid replica assignment"
	case 40:
		return "Invalid config"
	case 41:
		return "Not controller"
	case 42:
		return "Invalid request"
	case 43:
		return "Unsupported for message format"
	case 44:
		return "Policy violation"
	case 45:
		return "Out of order sequence number"
	case 46:
		return "Duplicate sequence number"
	case 47:
		return "Invalid producer epoch"
	case 48:
		return "Invalid txn state"
	case 49:
		return "Invalid producer id mapping"
	case 50:
		return "Invalid transaction timeout"
	case 51:
		return "Concurrent transactions"
	case 52:
		return "Transaction coordinator fenced"
	case 53:
		return "Transactional id authorization failed"
	case 54:
		return "Security disabled"
	case 55:
		return "Operation not attempted"
	case 56:
		return "Kafka storage error"
	case 57:
		return "Log dir not found"
	case 58:
		return "SASL authentication failed"
	case 59:
		return "Unknown producer id"
	case 60:
		return "Reassignment in progress"
	case 61:
		return "Delegation token auth disabled"
	case 62:
		return "Delegation token not found"
	case 63:
		return "Delegation token owner mismatch"
	case 64:
		return "Delegation token request not allowed"
	case 65:
		return "Delegation token authorization failed"
	case 66:
		return "Delegation token expired"
	case 67:
		return "Invalid principal type"
	case 68:
		return "Non empty group"
	case 69:
		return "Group id not found"
	case 70:
		return "Fetch session id not found"
	case 71:
		return "Invalid fetch session epoch"
	case 72:
		return "Listener not found"
	case 73:
		return "Topic deletion disabled"
	case 74:
		return "Fenced leader epoch"
	case 75:
		return "Unknown leader epoch"
	case 76:
		return "Unsupported compression type"
	case 77:
		return "Stale broker epoch"
	case 78:
		return "Offset not available"
	case 79:
		return "Member id required"
	case 80:
		return "Preferred leader not available"
	case 81:
		return "Group max size reached"
	case 82:
		return "Fenced instance id"
	default:
		return "Unknown error code"
	}
}
