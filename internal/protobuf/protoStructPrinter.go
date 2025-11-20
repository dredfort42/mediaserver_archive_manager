package __

import (
	"log"
	"reflect"
	"strconv"
	"strings"
)

func PrintProtoStruct(proto any, excludeFields *[]string) {
	excluded := make(map[string]struct{})

	if excludeFields != nil {
		for _, field := range *excludeFields {
			excluded[field] = struct{}{}
		}
	}

	h := reflect.TypeOf(proto)
	if h.Kind() == reflect.Pointer {
		h = h.Elem() // Dereference the pointer
	}

	v := reflect.ValueOf(proto)
	if v.Kind() == reflect.Pointer {
		v = v.Elem() // Dereference the pointer
	}

	t := v.Type()
	if t.Kind() != reflect.Struct {
		log.Println("WARNING: Provided input is not a struct")
		return
	}

	var maxKeyLen, maxValueLen int

	type keyVal struct {
		key string
		val string
	}
	tableToPrint := []keyVal{}

	for i := 0; i < v.NumField(); i++ {
		field := t.Field(i)
		fieldName := field.Name

		// Skip unexported fields or excluded fields
		if _, ok := excluded[fieldName]; ok || !field.IsExported() {
			continue
		}

		rawValue := v.Field(i).Interface()

		if maxKeyLen < len(fieldName) {
			maxKeyLen = len(fieldName)
		}

		value := ""

		switch rawValue := rawValue.(type) {
		case string:
			value = rawValue
		case int:
			value = strconv.Itoa(rawValue)
		case int32:
			value = strconv.Itoa(int(rawValue))
		case int64:
			value = strconv.Itoa(int(rawValue))
		case uint:
			value = strconv.Itoa(int(rawValue))
		case uint32:
			value = strconv.Itoa(int(rawValue))
		case uint64:
			value = strconv.Itoa(int(rawValue))
		case float32:
			value = strconv.FormatFloat(float64(rawValue), 'f', -1, 32)
		case float64:
			value = strconv.FormatFloat(rawValue, 'f', -1, 64)
		default:
			value = "--UNSUPORTED TYPE--"
		}

		if maxValueLen < len(value) {
			maxValueLen = len(value)
		}

		tableToPrint = append(tableToPrint, keyVal{fieldName, value})
	}

	tableLen := maxKeyLen + maxValueLen + 5

	log.Printf("  +%v+\n", strings.Repeat("-", tableLen))
	log.Printf("  | \033[32mProtocol Buffers:\033[0m %s%v|\n", h.Name(), strings.Repeat(" ", tableLen-len(h.Name())-19))
	log.Printf("  +-%v-+-%v-+\n", strings.Repeat("-", maxKeyLen), strings.Repeat("-", maxValueLen))

	for _, kv := range tableToPrint {
		log.Printf("  | %s%v | %v%v |\n", kv.key, strings.Repeat(" ", maxKeyLen-len(kv.key)), kv.val, strings.Repeat(" ", maxValueLen-len(kv.val)))
	}

	log.Printf("  +-%v-+-%v-+\n", strings.Repeat("-", maxKeyLen), strings.Repeat("-", maxValueLen))
}
