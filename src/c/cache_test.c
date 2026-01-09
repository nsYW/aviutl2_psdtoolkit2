#include "cache.h"
#include "logf.h"

#include <ovtest.h>

#include <stdarg.h>
#include <string.h>

// Stub for logging functions (cache.c depends on logf.h)
void ptk_logf_warn(struct ov_error const *const err, char const *const reference, char const *const format, ...) {
  (void)err;
  (void)reference;
  (void)format;
}

static void test_cache_create_and_destroy(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  ptk_cache_destroy(&c);
  TEST_CHECK(c == NULL);

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }
  ptk_cache_destroy(&c);
}

static void test_cache_put_invalid_args(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  uint8_t data[16] = {0};

  // NULL cache
  TEST_FAILED_WITH(ptk_cache_put(NULL, 0x0123456789abcdefULL, data, 2, 2, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // NULL data
  TEST_FAILED_WITH(ptk_cache_put(c, 0x0123456789abcdefULL, NULL, 2, 2, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // Invalid width
  TEST_FAILED_WITH(ptk_cache_put(c, 0x0123456789abcdefULL, data, 0, 2, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  TEST_FAILED_WITH(ptk_cache_put(c, 0x0123456789abcdefULL, data, -1, 2, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // Invalid height
  TEST_FAILED_WITH(ptk_cache_put(c, 0x0123456789abcdefULL, data, 2, 0, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  TEST_FAILED_WITH(ptk_cache_put(c, 0x0123456789abcdefULL, data, 2, -1, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  ptk_cache_destroy(&c);
}

static void test_cache_get_invalid_args(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  void *data = NULL;
  int32_t width = 0;
  int32_t height = 0;

  // NULL cache
  TEST_FAILED_WITH(ptk_cache_get(NULL, 0x0123456789abcdefULL, &data, &width, &height, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // NULL data pointer
  TEST_FAILED_WITH(ptk_cache_get(c, 0x0123456789abcdefULL, NULL, &width, &height, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // NULL width pointer
  TEST_FAILED_WITH(ptk_cache_get(c, 0x0123456789abcdefULL, &data, NULL, &height, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  // NULL height pointer
  TEST_FAILED_WITH(ptk_cache_get(c, 0x0123456789abcdefULL, &data, &width, NULL, &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  ptk_cache_destroy(&c);
}

static void test_cache_put_and_get_basic(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  // Create test data (2x2 BGRA image)
  uint8_t input_data[16] = {
      // Row 0
      0x00,
      0x00,
      0xFF,
      0xFF, // Red pixel (BGRA)
      0x00,
      0xFF,
      0x00,
      0xFF, // Green pixel
      // Row 1
      0xFF,
      0x00,
      0x00,
      0xFF, // Blue pixel
      0xFF,
      0xFF,
      0xFF,
      0xFF, // White pixel
  };

  // Put data
  if (!TEST_SUCCEEDED(ptk_cache_put(c, 0x0123456789abcdefULL, input_data, 2, 2, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  // Get data back
  void *output_data = NULL;
  int32_t width = 0;
  int32_t height = 0;

  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0x0123456789abcdefULL, &output_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  // Verify data
  if (TEST_CHECK(output_data != NULL)) {
    TEST_CHECK(width == 2);
    TEST_MSG("want 2, got %d", width);
    TEST_CHECK(height == 2);
    TEST_MSG("want 2, got %d", height);
    TEST_CHECK(memcmp(output_data, input_data, 16) == 0);
    TEST_DUMP("want:", input_data, 16);
    TEST_DUMP("got:", output_data, 16);
  }

  if (output_data) {
    OV_FREE(&output_data);
  }

  ptk_cache_destroy(&c);
}

static void test_cache_miss(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  void *data = NULL;
  int32_t width = 0;
  int32_t height = 0;

  // Get non-existent key - should succeed but return NULL data
  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0xfedcba9876543210ULL, &data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  TEST_CHECK(data == NULL);
  TEST_CHECK(width == 0);
  TEST_CHECK(height == 0);

  ptk_cache_destroy(&c);
}

static void test_cache_duplicate_put(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  uint8_t data1[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  uint8_t data2[16] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

  // Put first data
  if (!TEST_SUCCEEDED(ptk_cache_put(c, 0xaaaaaaaaaaaaaaaaULL, data1, 2, 2, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  // Put second data with same key - should succeed (update LRU only, not replace data)
  if (!TEST_SUCCEEDED(ptk_cache_put(c, 0xaaaaaaaaaaaaaaaaULL, data2, 2, 2, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  // Get data - should be original data (first put)
  void *output_data = NULL;
  int32_t width = 0;
  int32_t height = 0;

  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0xaaaaaaaaaaaaaaaaULL, &output_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  if (TEST_CHECK(output_data != NULL)) {
    // Original data should be preserved
    TEST_CHECK(memcmp(output_data, data1, 16) == 0);
    TEST_DUMP("want:", data1, 16);
    TEST_DUMP("got:", output_data, 16);
  }

  if (output_data) {
    OV_FREE(&output_data);
  }

  ptk_cache_destroy(&c);
}

static void test_cache_multiple_entries(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  // Create multiple entries
  struct {
    uint64_t key;
    uint8_t value;
  } entries[] = {
      {0x1111111111111111ULL, 0x11},
      {0x2222222222222222ULL, 0x22},
      {0x3333333333333333ULL, 0x33},
      {0x4444444444444444ULL, 0x44},
      {0x5555555555555555ULL, 0x55},
  };
  size_t const num_entries = sizeof(entries) / sizeof(entries[0]);

  // Put all entries
  for (size_t i = 0; i < num_entries; ++i) {
    uint8_t data[4] = {entries[i].value, entries[i].value, entries[i].value, entries[i].value};
    if (!TEST_SUCCEEDED(ptk_cache_put(c, entries[i].key, data, 1, 1, &err), &err)) {
      ptk_cache_destroy(&c);
      return;
    }
  }

  // Get all entries and verify
  for (size_t i = 0; i < num_entries; ++i) {
    TEST_CASE_("0x%016llx", (unsigned long long)entries[i].key);

    void *data = NULL;
    int32_t width = 0;
    int32_t height = 0;

    if (!TEST_SUCCEEDED(ptk_cache_get(c, entries[i].key, &data, &width, &height, &err), &err)) {
      ptk_cache_destroy(&c);
      return;
    }

    if (TEST_CHECK(data != NULL)) {
      TEST_CHECK(width == 1);
      TEST_CHECK(height == 1);
      uint8_t *bytes = (uint8_t *)data;
      TEST_CHECK(bytes[0] == entries[i].value);
      TEST_MSG("want 0x%02x, got 0x%02x", entries[i].value, bytes[0]);
    }

    if (data) {
      OV_FREE(&data);
    }
  }

  TEST_CASE_(NULL);
  ptk_cache_destroy(&c);
}

static void test_cache_large_image(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  int32_t const w = 256;
  int32_t const h = 256;
  size_t const data_size = (size_t)w * (size_t)h * 4;
  uint8_t *input_data = NULL;

  if (!TEST_CHECK(OV_REALLOC(&input_data, data_size, 1))) {
    ptk_cache_destroy(&c);
    return;
  }

  // Fill with pattern
  for (size_t i = 0; i < data_size; ++i) {
    input_data[i] = (uint8_t)(i & 0xFF);
  }

  // Put data
  if (!TEST_SUCCEEDED(ptk_cache_put(c, 0x1a49e1a9e1234ab0ULL, input_data, w, h, &err), &err)) {
    OV_FREE(&input_data);
    ptk_cache_destroy(&c);
    return;
  }

  // Get data back
  void *output_data = NULL;
  int32_t out_w = 0;
  int32_t out_h = 0;

  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0x1a49e1a9e1234ab0ULL, &output_data, &out_w, &out_h, &err), &err)) {
    OV_FREE(&input_data);
    ptk_cache_destroy(&c);
    return;
  }

  if (TEST_CHECK(output_data != NULL)) {
    TEST_CHECK(out_w == w);
    TEST_MSG("want %d, got %d", w, out_w);
    TEST_CHECK(out_h == h);
    TEST_MSG("want %d, got %d", h, out_h);
    TEST_CHECK(memcmp(output_data, input_data, data_size) == 0);
  }

  if (output_data) {
    OV_FREE(&output_data);
  }
  OV_FREE(&input_data);
  ptk_cache_destroy(&c);
}

static void test_cache_recreate_clears_data(void) {
  struct ov_error err = {0};
  struct ptk_cache *c = NULL;

  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  // Put some data
  uint8_t data[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  if (!TEST_SUCCEEDED(ptk_cache_put(c, 0x9e1017e123456789ULL, data, 1, 1, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }

  // Verify it's there
  void *out_data = NULL;
  int32_t width = 0;
  int32_t height = 0;
  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0x9e1017e123456789ULL, &out_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }
  TEST_CHECK(out_data != NULL);
  if (out_data) {
    OV_FREE(&out_data);
  }

  ptk_cache_destroy(&c);
  c = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c != NULL, &err)) {
    return;
  }

  // Data should be gone (new instance has different directory)
  out_data = NULL;
  width = 0;
  height = 0;
  if (!TEST_SUCCEEDED(ptk_cache_get(c, 0x9e1017e123456789ULL, &out_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c);
    return;
  }
  TEST_CHECK(out_data == NULL);

  ptk_cache_destroy(&c);
}

static void test_cache_multiple_instances(void) {
  struct ov_error err = {0};
  struct ptk_cache *c1 = NULL;
  struct ptk_cache *c2 = NULL;

  c1 = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c1 != NULL, &err)) {
    return;
  }
  c2 = ptk_cache_create(&err);
  if (!TEST_SUCCEEDED(c2 != NULL, &err)) {
    ptk_cache_destroy(&c1);
    return;
  }

  // Put data in first instance
  uint8_t data1[4] = {0x11, 0x22, 0x33, 0x44};
  if (!TEST_SUCCEEDED(ptk_cache_put(c1, 0x696e7374316b6579ULL, data1, 1, 1, &err), &err)) {
    ptk_cache_destroy(&c1);
    ptk_cache_destroy(&c2);
    return;
  }

  // Put different data in second instance with same key
  uint8_t data2[4] = {0xAA, 0xBB, 0xCC, 0xDD};
  if (!TEST_SUCCEEDED(ptk_cache_put(c2, 0x696e7374316b6579ULL, data2, 1, 1, &err), &err)) {
    ptk_cache_destroy(&c1);
    ptk_cache_destroy(&c2);
    return;
  }

  // Get from first instance - should return first data
  void *out_data = NULL;
  int32_t width = 0;
  int32_t height = 0;
  if (!TEST_SUCCEEDED(ptk_cache_get(c1, 0x696e7374316b6579ULL, &out_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c1);
    ptk_cache_destroy(&c2);
    return;
  }
  if (TEST_CHECK(out_data != NULL)) {
    TEST_CHECK(memcmp(out_data, data1, 4) == 0);
    TEST_DUMP("want:", data1, 4);
    TEST_DUMP("got:", out_data, 4);
  }
  if (out_data) {
    OV_FREE(&out_data);
  }

  // Get from second instance - should return second data
  out_data = NULL;
  if (!TEST_SUCCEEDED(ptk_cache_get(c2, 0x696e7374316b6579ULL, &out_data, &width, &height, &err), &err)) {
    ptk_cache_destroy(&c1);
    ptk_cache_destroy(&c2);
    return;
  }
  if (TEST_CHECK(out_data != NULL)) {
    TEST_CHECK(memcmp(out_data, data2, 4) == 0);
    TEST_DUMP("want:", data2, 4);
    TEST_DUMP("got:", out_data, 4);
  }
  if (out_data) {
    OV_FREE(&out_data);
  }

  ptk_cache_destroy(&c1);
  ptk_cache_destroy(&c2);
}

TEST_LIST = {
    {"test_cache_create_and_destroy", test_cache_create_and_destroy},
    {"test_cache_put_invalid_args", test_cache_put_invalid_args},
    {"test_cache_get_invalid_args", test_cache_get_invalid_args},
    {"test_cache_put_and_get_basic", test_cache_put_and_get_basic},
    {"test_cache_miss", test_cache_miss},
    {"test_cache_duplicate_put", test_cache_duplicate_put},
    {"test_cache_multiple_entries", test_cache_multiple_entries},
    {"test_cache_large_image", test_cache_large_image},
    {"test_cache_recreate_clears_data", test_cache_recreate_clears_data},
    {"test_cache_multiple_instances", test_cache_multiple_instances},
    {NULL, NULL},
};
