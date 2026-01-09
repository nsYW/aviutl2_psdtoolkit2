package prop

import "testing"

// testEncodeData tests the new simplified encoding (no Shift_JIS escaping)
var testEncodeData = [][2]string{
	{"表現不可能なソース文字列", ".表現不可能なソース文字列"},
	{"表現不可能", ".表現不可能"},
	{"ソース顔", ".ソース顔"},
	{"👍グッドな文字", ".👍グッドな文字"},
	{"グッド👍な文字", ".グッド👍な文字"},
	{"グッドな文字👍", ".グッドな文字👍"},
	{"backslash\\test", ".backslash%ytest"},
	{"hello", ".hello"},
	{"", ""},
	{"日本語テスト", ".日本語テスト"},
	{"한국어", ".한국어"},           // Korean
	{"中文测试", ".中文测试"},         // Simplified Chinese
	{"Ελληνικά", ".Ελληνικά"}, // Greek
	{"العربية", ".العربية"},   // Arabic
	{"🎉🎊🎈", ".🎉🎊🎈"},           // Multiple emojis
}

// testDecodeLegacyData tests backward compatibility with old encoded strings
var testDecodeLegacyData = [][2]string{
	{"表現不可能なソース文字列", ".%xQ現不可%xPな%xBース文字列"},
	{"表現不可能", ".%xQ現不可%xP"},
	{"ソース顔", ".%xBース顔"},
	{"👍グッドな文字", ".%vTfQBグッドな文字"},
	{"グッド👍な文字", ".グッド%vTfQBな文字"},
	{"グッドな文字👍", ".グッドな文字%vTfQB"},
}

func TestEncode(t *testing.T) {
	for idx, data := range testEncodeData {
		if got := Encode(data[0]); data[1] != got {
			t.Errorf("[%d] want %q, got %q", idx, data[1], got)
		}
	}
}

func TestDecode(t *testing.T) {
	// Test decoding of new format (should decode what Encode produces)
	for idx, data := range testEncodeData {
		got, err := Decode(data[1])
		if err != nil {
			t.Fatalf("[%d] %v", idx, err)
		}
		if data[0] != got {
			t.Errorf("[%d] want %q, got %q", idx, data[0], got)
		}
	}
}

func TestDecodeLegacy(t *testing.T) {
	// Test backward compatibility: decoding old format with Shift_JIS escaping
	for idx, data := range testDecodeLegacyData {
		got, err := Decode(data[1])
		if err != nil {
			t.Fatalf("[%d] %v", idx, err)
		}
		if data[0] != got {
			t.Errorf("[%d] want %q, got %q", idx, data[0], got)
		}
	}
}

func TestEncodeDecodeRoundTrip(t *testing.T) {
	// Test that Encode followed by Decode returns original string
	testStrings := []string{
		"simple",
		"with\\backslash",
		"日本語",
		"表ソ能", // damemoji characters (now passed through as-is)
		"emoji👍test",
		"mixed 日本語 and English",
		"path/to/layer",
		"",
	}
	for idx, s := range testStrings {
		encoded := Encode(s)
		decoded, err := Decode(encoded)
		if err != nil {
			t.Fatalf("[%d] encode/decode error: %v", idx, err)
		}
		if s != decoded {
			t.Errorf("[%d] roundtrip failed: original=%q, encoded=%q, decoded=%q", idx, s, encoded, decoded)
		}
	}
}
