package prop

import (
	"encoding/base64"

	"github.com/pkg/errors"
)

// damemojiRev is used for decoding legacy encoded strings (backward compatibility)
var damemojiRev = [42]string{
	"―", "ソ", "Ы", "Ⅸ", "噂", "浬", "欺", "圭",
	"構", "蚕", "十", "申", "曾", "箪", "貼", "能",
	"表", "暴", "予", "禄", "兔", "喀", "媾", "彌",
	"拿", "杤", "歃", "濬", "畚", "秉", "綵", "臀",
	"藹", "觸", "軆", "鐔", "饅", "鷭", "纊", "犾",
	"偆", "砡",
}

func Encode(s string) string {
	if len(s) == 0 {
		return ""
	}
	runes := []rune(s)
	escaped := make([]rune, 0, len(runes)+1)
	escaped = append(escaped, '.')
	for _, r := range runes {
		if r == '\\' {
			escaped = append(escaped, '%', 'y')
			continue
		}
		escaped = append(escaped, r)
	}
	return string(escaped)
}

func rune64ToInt(r byte) int {
	if 'A' <= r && r <= 'Z' {
		return int(r - 'A')
	}
	if 'a' <= r && r <= 'z' {
		return int(26 + r - 'a')
	}
	if '0' <= r && r <= '9' {
		return int(52 + r - '0')
	}
	if r == '-' {
		return 62
	}
	if r == '_' {
		return 63
	}
	return -1
}

func unescape(s string) string {
	l := len(s)
	r := make([]byte, 0, l)
	for i := 0; i < l; i++ {
		if s[i] != '%' {
			r = append(r, s[i])
			continue
		}
		if i+1 < l && s[i+1] == 'y' {
			r = append(r, '\\')
			i++
			continue
		} else if i+2 < l {
			switch s[i+1] {
			case 'x':
				if idx := rune64ToInt(s[i+2]); 0 <= idx && idx < len(damemojiRev) {
					r = append(r, damemojiRev[idx]...)
					i += 2
					continue
				}
			case 't', 'u', 'v', 'w':
				if encoded := base64.RawURLEncoding.EncodedLen(int(s[i+1] - 't' + 1)); i+1+encoded < l {
					if b, err := base64.RawURLEncoding.DecodeString(s[i+2 : i+2+encoded]); err == nil {
						var rn rune
						for i := len(b) - 1; i >= 0; i-- {
							rn <<= 8
							rn |= rune(b[i])
						}
						r = append(r, string(rn)...)
						i += 1 + encoded
						continue
					}
				}
			}
		}
		r = append(r, s[i])
	}
	return string(r)
}

func Decode(s string) (string, error) {
	if len(s) <= 1 {
		return "", nil
	}
	switch s[0] {
	case '_':
		b, err := base64.RawURLEncoding.DecodeString(s[1:])
		if err != nil {
			return "", err
		}
		return string(b), nil
	case '.':
		return unescape(s[1:]), nil
	}
	return "", errors.Errorf("unsupported encoding type: %q", s[0:1])
}
