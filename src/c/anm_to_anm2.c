#include "anm_to_anm2.h"

#include <ovarray.h>
#include <ovmo.h>
#include <ovutf.h>

#include <string.h>

/**
 * @brief Replace all occurrences of old_str with new_str in the buffer
 *
 * @param buf [in,out] Buffer to process (ovarray, will be modified)
 * @param old_str String to find
 * @param old_len Length of old_str
 * @param new_str Replacement string
 * @param new_len Length of new_str
 * @param err [out] Error information
 * @return true on success, false on failure
 */
static bool replace_all(
    char **buf, char const *old_str, size_t old_len, char const *new_str, size_t new_len, struct ov_error *const err) {
  if (!buf || !*buf || !old_str || old_len == 0) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  char *result = NULL;
  bool success = false;

  size_t const src_len = OV_ARRAY_LENGTH(*buf);
  char const *src = *buf;
  char const *pos = src;
  char const *const end = src + src_len;

  // First pass: count occurrences and calculate new size
  size_t count = 0;
  {
    char const *p = src;
    while (p + old_len <= end) {
      if (memcmp(p, old_str, old_len) == 0) {
        count++;
        p += old_len;
      } else {
        p++;
      }
    }
  }

  if (count == 0) {
    // No replacements needed
    success = true;
    goto cleanup;
  }

  {
    // Calculate new size
    size_t const new_size = src_len - (count * old_len) + (count * new_len);

    if (!OV_ARRAY_GROW(&result, new_size + 1)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }

    // Second pass: build result
    char *d = result;
    while (pos + old_len <= end) {
      if (memcmp(pos, old_str, old_len) == 0) {
        memcpy(d, new_str, new_len);
        d += new_len;
        pos += old_len;
      } else {
        *d++ = *pos++;
      }
    }
    // Copy remaining bytes
    while (pos < end) {
      *d++ = *pos++;
    }
    *d = '\0';
    OV_ARRAY_SET_LENGTH(result, new_size);

    // Swap buffers
    OV_ARRAY_DESTROY(buf);
    *buf = result;
    result = NULL;
    success = true;
  }

cleanup:
  if (result) {
    OV_ARRAY_DESTROY(&result);
  }
  return success;
}

// Replacement rules for legacy API calls
struct replacement_rule {
  char const *old_str;
  size_t old_len;
  char const *new_str;
  size_t new_len;
};

bool ptk_anm_to_anm2(char const *src, size_t src_len, char **dst, struct ov_error *const err) {
  static struct replacement_rule const rules[] = {
      {
          .old_str = "require(\"PSDToolKit\").Blinker.new(",
          .old_len = sizeof("require(\"PSDToolKit\").Blinker.new(") - 1,
          .new_str = "require(\"PSDToolKit.Blinker\").new_legacy(",
          .new_len = sizeof("require(\"PSDToolKit.Blinker\").new_legacy(") - 1,
      },
      {
          .old_str = "require(\"PSDToolKit\").LipSyncSimple.new(",
          .old_len = sizeof("require(\"PSDToolKit\").LipSyncSimple.new(") - 1,
          .new_str = "require(\"PSDToolKit.LipSync\").new_legacy(",
          .new_len = sizeof("require(\"PSDToolKit.LipSync\").new_legacy(") - 1,
      },
      {
          .old_str = "require(\"PSDToolKit\").LipSyncLab.new(",
          .old_len = sizeof("require(\"PSDToolKit\").LipSyncLab.new(") - 1,
          .new_str = "require(\"PSDToolKit.LipSyncLab\").new_legacy(",
          .new_len = sizeof("require(\"PSDToolKit.LipSyncLab\").new_legacy(") - 1,
      },
      {
          .old_str = "PSD:addstate(",
          .old_len = sizeof("PSD:addstate(") - 1,
          .new_str = "require(\"PSDToolKit\").add_state_legacy(",
          .new_len = sizeof("require(\"PSDToolKit\").add_state_legacy(") - 1,
      },
  };
  static size_t const rules_count = sizeof(rules) / sizeof(rules[0]);

  if (!dst) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  *dst = NULL;

  if (!src || src_len == 0) {
    // Empty input produces empty output
    if (!OV_ARRAY_GROW(dst, 1)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      return false;
    }
    (*dst)[0] = '\0';
    OV_ARRAY_SET_LENGTH(*dst, 0);
    return true;
  }

  char *utf8_buf = NULL;
  bool success = false;

  // Step 1: Validate input - check if it's a legacy script
  {
    // Look for "PSD:addstate(" in the source (Shift_JIS)
    char const *const pattern = "PSD:addstate(";
    size_t const pattern_len = sizeof("PSD:addstate(") - 1;
    bool found = false;

    for (size_t i = 0; i + pattern_len <= src_len; i++) {
      if (memcmp(src + i, pattern, pattern_len) == 0) {
        found = true;
        break;
      }
    }

    if (!found) {
      OV_ERROR_SET(err,
                   ov_error_type_generic,
                   ptk_anm_to_anm2_error_not_legacy_script,
                   gettext("The file does not appear to be a legacy PSDToolKit anm script."));
      goto cleanup;
    }
  }

  // Step 2: Convert Shift_JIS to UTF-8
  {
    // Calculate required buffer size
    size_t const utf8_len = ov_sjis_to_utf8_len(src, src_len);
    if (utf8_len == 0) {
      OV_ERROR_SET(err, ov_error_type_generic, ov_error_generic_fail, "Failed to calculate UTF-8 length");
      goto cleanup;
    }

    if (!OV_ARRAY_GROW(&utf8_buf, utf8_len + 1)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }

    size_t const written = ov_sjis_to_utf8(src, src_len, utf8_buf, utf8_len + 1, NULL);
    if (written == 0) {
      OV_ERROR_SET(err, ov_error_type_generic, ov_error_generic_fail, "Failed to convert Shift_JIS to UTF-8");
      goto cleanup;
    }

    utf8_buf[written] = '\0';
    OV_ARRAY_SET_LENGTH(utf8_buf, written);
  }

  // Step 3: Apply replacement rules
  for (size_t i = 0; i < rules_count; i++) {
    if (!replace_all(&utf8_buf, rules[i].old_str, rules[i].old_len, rules[i].new_str, rules[i].new_len, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
  }

  // Transfer ownership
  *dst = utf8_buf;
  utf8_buf = NULL;
  success = true;

cleanup:
  if (utf8_buf) {
    OV_ARRAY_DESTROY(&utf8_buf);
  }
  return success;
}
