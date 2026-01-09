// Test file for anm_to_anm2.c
//
// Tests the anm to anm2 conversion functionality.

#include "anm_to_anm2.h"

#include <ovarray.h>
#include <ovbase.h>

#ifndef SOURCE_DIR
#  define SOURCE_DIR .
#endif

#define STR(x) #x
#define STRINGIZE(x) STR(x)
#define TEST_PATH(relative_path) STRINGIZE(SOURCE_DIR) "/test_data/" relative_path

#include <ovtest.h>

#include <stdio.h>
#include <string.h>

// Helper function to read file contents
static bool read_file(char const *path, char **data, size_t *len) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return false;
  }

  fseek(f, 0, SEEK_END);
  long const size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (!OV_REALLOC(data, (size_t)size + 1, sizeof(char))) {
    fclose(f);
    return false;
  }

  *len = fread(*data, 1, (size_t)size, f);
  (*data)[*len] = '\0';
  fclose(f);
  return true;
}

static void test_anm_to_anm2_legacy(void) {
  struct ov_error err = {0};
  char *input = NULL;
  size_t input_len = 0;
  char *expected = NULL;
  size_t expected_len = 0;
  char *output = NULL;
  size_t output_len = 0;

  // Read input file (Shift_JIS)
  if (!TEST_CHECK(read_file(TEST_PATH("anm_to_anm2/legacy.anm"), &input, &input_len))) {
    TEST_MSG("Failed to read legacy.anm");
    goto cleanup;
  }

  // Read expected output file (UTF-8)
  if (!TEST_CHECK(read_file(TEST_PATH("anm_to_anm2/legacy.ptk.anm2"), &expected, &expected_len))) {
    TEST_MSG("Failed to read legacy.ptk.anm2");
    goto cleanup;
  }

  // Perform conversion
  if (!TEST_SUCCEEDED(ptk_anm_to_anm2(input, input_len, &output, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(output != NULL);
  if (!output) {
    TEST_MSG("Output is NULL");
    goto cleanup;
  }

  output_len = OV_ARRAY_LENGTH(output);
  TEST_CHECK(output_len == expected_len);
  TEST_MSG("want len=%zu, got len=%zu", expected_len, output_len);

  if (output_len == expected_len) {
    TEST_CHECK(memcmp(output, expected, expected_len) == 0);
    if (memcmp(output, expected, expected_len) != 0) {
      TEST_MSG("Output content does not match expected");
      // Find first difference for debugging
      for (size_t i = 0; i < expected_len; i++) {
        if (output[i] != expected[i]) {
          TEST_MSG("First difference at offset %zu: got 0x%02x, want 0x%02x",
                   i,
                   (unsigned char)output[i],
                   (unsigned char)expected[i]);
          break;
        }
      }
    }
  }

cleanup:
  if (output) {
    OV_ARRAY_DESTROY(&output);
  }
  if (expected) {
    OV_FREE(&expected);
  }
  if (input) {
    OV_FREE(&input);
  }
}

static void test_anm_to_anm2_at_legacy(void) {
  struct ov_error err = {0};
  char *input = NULL;
  size_t input_len = 0;
  char *expected = NULL;
  size_t expected_len = 0;
  char *output = NULL;
  size_t output_len = 0;

  // Read input file (Shift_JIS, starts with @)
  if (!TEST_CHECK(read_file(TEST_PATH("anm_to_anm2/@legacy.anm"), &input, &input_len))) {
    TEST_MSG("Failed to read @legacy.anm");
    goto cleanup;
  }

  // Read expected output file (UTF-8)
  if (!TEST_CHECK(read_file(TEST_PATH("anm_to_anm2/@legacy.ptk.anm2"), &expected, &expected_len))) {
    TEST_MSG("Failed to read @legacy.ptk.anm2");
    goto cleanup;
  }

  // Perform conversion
  if (!TEST_SUCCEEDED(ptk_anm_to_anm2(input, input_len, &output, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(output != NULL);
  if (!output) {
    TEST_MSG("Output is NULL");
    goto cleanup;
  }

  output_len = OV_ARRAY_LENGTH(output);
  TEST_CHECK(output_len == expected_len);
  TEST_MSG("want len=%zu, got len=%zu", expected_len, output_len);

  if (output_len == expected_len) {
    TEST_CHECK(memcmp(output, expected, expected_len) == 0);
    if (memcmp(output, expected, expected_len) != 0) {
      TEST_MSG("Output content does not match expected");
      // Find first difference for debugging
      for (size_t i = 0; i < expected_len; i++) {
        if (output[i] != expected[i]) {
          TEST_MSG("First difference at offset %zu: got 0x%02x, want 0x%02x",
                   i,
                   (unsigned char)output[i],
                   (unsigned char)expected[i]);
          break;
        }
      }
    }
  }

cleanup:
  if (output) {
    OV_ARRAY_DESTROY(&output);
  }
  if (expected) {
    OV_FREE(&expected);
  }
  if (input) {
    OV_FREE(&input);
  }
}

static void test_anm_to_anm2_empty(void) {
  struct ov_error err = {0};
  char *output = NULL;

  // Empty input should produce empty output
  if (!TEST_SUCCEEDED(ptk_anm_to_anm2("", 0, &output, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(output != NULL);
  if (output) {
    size_t const output_len = OV_ARRAY_LENGTH(output);
    TEST_CHECK(output_len == 0);
    TEST_MSG("want len=0, got len=%zu", output_len);
  }

cleanup:
  if (output) {
    OV_ARRAY_DESTROY(&output);
  }
}

static void test_anm_to_anm2_null_src(void) {
  struct ov_error err = {0};
  char *output = NULL;

  // NULL input should produce empty output
  if (!TEST_SUCCEEDED(ptk_anm_to_anm2(NULL, 0, &output, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(output != NULL);
  if (output) {
    size_t const output_len = OV_ARRAY_LENGTH(output);
    TEST_CHECK(output_len == 0);
    TEST_MSG("want len=0, got len=%zu", output_len);
  }

cleanup:
  if (output) {
    OV_ARRAY_DESTROY(&output);
  }
}

static void test_anm_to_anm2_null_dst(void) {
  struct ov_error err = {0};

  // NULL dst should return error
  bool const result = ptk_anm_to_anm2("test", 4, NULL, &err);

  TEST_CHECK(!result);
  TEST_MSG("Should fail when dst is NULL");

  OV_ERROR_DESTROY(&err);
}

static void test_anm_to_anm2_not_legacy_script(void) {
  struct ov_error err = {0};
  char *output = NULL;

  // Simple ASCII text without "PSD:addstate(" pattern
  // Should be rejected as not a legacy script
  char const input[] = "-- comment\nlocal x = 1\n";
  size_t const input_len = sizeof(input) - 1;

  bool const result = ptk_anm_to_anm2(input, input_len, &output, &err);

  TEST_CHECK(!result);
  TEST_MSG("Should fail when input is not a legacy script");

  if (!result) {
    TEST_CHECK(ov_error_is(&err, ov_error_type_generic, ptk_anm_to_anm2_error_not_legacy_script));
    TEST_MSG("Error code should be ptk_anm_to_anm2_error_not_legacy_script");
  }

  TEST_CHECK(output == NULL);
  TEST_MSG("Output should be NULL on error");

  OV_ERROR_DESTROY(&err);
}

TEST_LIST = {
    {"anm_to_anm2_legacy", test_anm_to_anm2_legacy},
    {"anm_to_anm2_at_legacy", test_anm_to_anm2_at_legacy},
    {"anm_to_anm2_empty", test_anm_to_anm2_empty},
    {"anm_to_anm2_null_src", test_anm_to_anm2_null_src},
    {"anm_to_anm2_null_dst", test_anm_to_anm2_null_dst},
    {"anm_to_anm2_not_legacy_script", test_anm_to_anm2_not_legacy_script},
    {NULL, NULL},
};
