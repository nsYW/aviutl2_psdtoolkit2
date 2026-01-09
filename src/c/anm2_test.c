// Stub out external dependencies before including anm2.c
#include "logf.h"

void ptk_logf_warn(struct ov_error const *const err, char const *const reference, char const *const format, ...) {
  (void)err;
  (void)reference;
  (void)format;
}

void ptk_logf_error(struct ov_error const *const err, char const *const reference, char const *const format, ...) {
  (void)err;
  (void)reference;
  (void)format;
}

// Include anm2.c directly to test static functions
#include "anm2.c"

#include <ovtest.h>

#include <string.h>

#ifdef _WIN32
#  include <windows.h>
#endif

#ifndef SOURCE_DIR
#  define SOURCE_DIR .
#endif

#define LSTR(x) L##x
#define LSTR2(x) LSTR(#x)
#define STRINGIZE(x) LSTR2(x)
#define TEST_PATH(relative_path) STRINGIZE(SOURCE_DIR) L"/test_data/anm2doc/" relative_path

// Test data file paths
static wchar_t const *const test_data_basic = TEST_PATH(L"basic.lua");
static wchar_t const *const test_data_animation = TEST_PATH(L"animation.lua");
static wchar_t const *const test_data_mixed = TEST_PATH(L"mixed.lua");

// ============================================================================
// Document lifecycle tests
// ============================================================================

static void test_new_destroy(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);

  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);
  TEST_CHECK(doc != NULL);
  if (doc) {
    TEST_CHECK(ptk_anm2_get_version(doc) == 1);
    TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "PSD") == 0);
    TEST_CHECK(ptk_anm2_get_psd_path(doc) == NULL || strlen(ptk_anm2_get_psd_path(doc)) == 0);
    TEST_CHECK(ptk_anm2_selector_count(doc) == 0);
    TEST_CHECK(!ptk_anm2_can_undo(doc));
    TEST_CHECK(!ptk_anm2_can_redo(doc));

    ptk_anm2_destroy(&doc);
    TEST_CHECK(doc == NULL);
  }
}

static void test_destroy_null(void) {
  // Should not crash
  ptk_anm2_destroy(NULL);
  struct ptk_anm2 *doc = NULL;
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Selector operation tests (Phase 2)
// ============================================================================

static void test_selector_add(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  uint32_t id = ptk_anm2_selector_add(doc, "Group1", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);

  TEST_CHECK(id > 0);
  TEST_MSG("Expected non-zero ID, got %u", id);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group1") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

  // Add another selector and verify ID is different
  uint32_t id2 = ptk_anm2_selector_add(doc, "Group2", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);
  TEST_CHECK(id2 > id);
  TEST_MSG("Expected id2 (%u) > id (%u)", id2, id);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);

  ptk_anm2_destroy(&doc);
}

static void test_selector_remove(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group2", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_remove(doc, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group2") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_selector_set_group(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Original", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_set_group(doc, 0, "Modified", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Modified") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_selector_move_to(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "A", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "B", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "C", &err), &err)) {
    goto cleanup;
  }

  // Move A (index 0) to index 2 -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_selector_move_to(doc, 0, 2, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "B") == 0);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 1), "C") == 0);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 2), "A") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_selector_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);

  // Undo add
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 0);

  // Redo add
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group1") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Item operation tests (Phase 2)
// ============================================================================

static void test_item_add_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  uint32_t id = 0;
  uint32_t id2 = 0;
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  id = ptk_anm2_item_add_value(doc, 0, "Item1", "path/to/layer", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);

  TEST_CHECK(id > 0);
  TEST_MSG("Expected non-zero ID, got %u", id);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 0));
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "path/to/layer") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

  // Add another item and verify ID is different
  id2 = ptk_anm2_item_add_value(doc, 0, "Item2", "path2", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);
  TEST_CHECK(id2 > id);
  TEST_MSG("Expected id2 (%u) > id (%u)", id2, id);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_insert_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  uint32_t id = 0;
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "First", "path1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Third", "path3", &err), &err)) {
    goto cleanup;
  }
  // Insert at position 1 - verify ID returned
  id = ptk_anm2_item_insert_value(doc, 0, 1, "Second", "path2", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);
  TEST_CHECK(id > 0);

  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 3);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "First") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Second") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "Third") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_add_animation(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  uint32_t id = 0;
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  id = ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);

  TEST_CHECK(id > 0);
  TEST_MSG("Expected non-zero ID, got %u", id);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(ptk_anm2_item_is_animation(doc, 0, 0));
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "目パチ") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "PSDToolKit.Blinker") == 0);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_insert_animation(void) {
  struct ov_error err = {0};
  uint32_t id = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  // Add two value items first
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "First", "path1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Third", "path3", &err), &err)) {
    goto cleanup;
  }
  // Insert animation at position 1 - verify ID returned
  id = ptk_anm2_item_insert_animation(doc, 0, 1, "PSDToolKit.Blinker", "Second", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);
  TEST_CHECK(id > 0);

  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 3);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "First") == 0);
  TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 0));
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Second") == 0);
  TEST_CHECK(ptk_anm2_item_is_animation(doc, 0, 1));
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 1), "PSDToolKit.Blinker") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "Third") == 0);
  TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 2));
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_remove(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "First", "path1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Second", "path2", &err), &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_item_remove(doc, 0, 0, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Second") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_move_to(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "A", "pathA", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "B", "pathB", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "C", "pathC", &err), &err)) {
    goto cleanup;
  }

  // Move A (index 0) to index 2 -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_item_move_to(doc, 0, 0, 0, 2, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "B") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "C") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "A") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item1", "path1", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);

  // Undo add item
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 0);

  // Redo add item
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Parameter operation tests (Phase 2)
// ============================================================================

static void test_param_add(void) {
  struct ov_error err = {0};
  uint32_t id = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err)) {
    goto cleanup;
  }
  id = ptk_anm2_param_add(doc, 0, 0, "間隔(秒)", "5.00", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);

  TEST_CHECK(id > 0);
  TEST_MSG("Expected non-zero ID, got %u", id);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "間隔(秒)") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "5.00") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_insert(void) {
  struct ov_error err = {0};
  uint32_t id = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "first", "1", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "third", "3", &err)) {
    goto cleanup;
  }
  // Insert at position 1 - verify ID returned
  id = ptk_anm2_param_insert(doc, 0, 0, 1, "second", "2", &err);
  TEST_ASSERT_SUCCEEDED(id != 0, &err);

  TEST_CHECK(id > 0);
  TEST_MSG("Expected non-zero ID, got %u", id);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 3);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "first") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 1), "second") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 2), "third") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_remove(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "key1", "val1", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "key2", "val2", &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_param_remove(doc, 0, 0, 0, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "key2") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_set_key_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "oldkey", "oldval", &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_param_set_key(doc, 0, 0, 0, "newkey", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "newkey") == 0);

  if (!TEST_SUCCEEDED(ptk_anm2_param_set_value(doc, 0, 0, 0, "newval", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "newval") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_param_add(doc, 0, 0, "key1", "val1", &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 1);

  // Undo add param
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 0);

  // Redo add param
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "key1") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// ID and userdata tests (Phase 3)
// ============================================================================

static void test_selector_id_userdata(void) {
  struct ov_error err = {0};
  uint32_t id1 = 0;
  uint32_t id2 = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add two selectors
  id1 = ptk_anm2_selector_add(doc, "Group1", &err);
  TEST_ASSERT_SUCCEEDED(id1 != 0, &err);
  id2 = ptk_anm2_selector_add(doc, "Group2", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);

  // IDs should be unique and non-zero
  TEST_CHECK(id1 > 0);
  TEST_CHECK(id2 > 0);
  TEST_CHECK(id1 != id2);

  // IDs should be retrievable via get_id
  TEST_CHECK(ptk_anm2_selector_get_id(doc, 0) == id1);
  TEST_CHECK(ptk_anm2_selector_get_id(doc, 1) == id2);

  // Invalid index should return 0
  TEST_CHECK(ptk_anm2_selector_get_id(doc, 999) == 0);

  // Test userdata (default is 0)
  TEST_CHECK(ptk_anm2_selector_get_userdata(doc, 0) == 0);
  TEST_CHECK(ptk_anm2_selector_get_userdata(doc, 1) == 0);

  // Set and get userdata
  ptk_anm2_selector_set_userdata(doc, 0, 0x12345678);
  ptk_anm2_selector_set_userdata(doc, 1, 0xDEADBEEF);
  TEST_CHECK(ptk_anm2_selector_get_userdata(doc, 0) == 0x12345678);
  TEST_CHECK(ptk_anm2_selector_get_userdata(doc, 1) == 0xDEADBEEF);

  // Invalid index should return 0
  TEST_CHECK(ptk_anm2_selector_get_userdata(doc, 999) == 0);

  ptk_anm2_destroy(&doc);
}

static void test_item_id_userdata(void) {
  struct ov_error err = {0};
  uint32_t sel_id = 0;
  uint32_t id1 = 0;
  uint32_t id2 = 0;
  uint32_t id3 = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add a selector
  sel_id = ptk_anm2_selector_add(doc, "Group1", &err);
  TEST_ASSERT_SUCCEEDED(sel_id != 0, &err);

  // Add items
  id1 = ptk_anm2_item_add_value(doc, 0, "Value1", "path1", &err);
  TEST_ASSERT_SUCCEEDED(id1 != 0, &err);
  id2 = ptk_anm2_item_add_animation(doc, 0, "Script", "Anim1", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);
  id3 = ptk_anm2_item_add_value(doc, 0, "Value2", "path2", &err);
  TEST_ASSERT_SUCCEEDED(id3 != 0, &err);

  // IDs should be unique and non-zero
  TEST_CHECK(id1 > 0);
  TEST_CHECK(id2 > 0);
  TEST_CHECK(id3 > 0);
  TEST_CHECK(id1 != id2 && id2 != id3 && id1 != id3);

  // IDs should be retrievable
  TEST_CHECK(ptk_anm2_item_get_id(doc, 0, 0) == id1);
  TEST_CHECK(ptk_anm2_item_get_id(doc, 0, 1) == id2);
  TEST_CHECK(ptk_anm2_item_get_id(doc, 0, 2) == id3);

  // Invalid indices should return 0
  TEST_CHECK(ptk_anm2_item_get_id(doc, 0, 999) == 0);
  TEST_CHECK(ptk_anm2_item_get_id(doc, 999, 0) == 0);

  // Test userdata (default is 0)
  TEST_CHECK(ptk_anm2_item_get_userdata(doc, 0, 0) == 0);

  // Set and get userdata
  ptk_anm2_item_set_userdata(doc, 0, 0, 0xAAAA);
  ptk_anm2_item_set_userdata(doc, 0, 1, 0xBBBB);
  TEST_CHECK(ptk_anm2_item_get_userdata(doc, 0, 0) == 0xAAAA);
  TEST_CHECK(ptk_anm2_item_get_userdata(doc, 0, 1) == 0xBBBB);

  ptk_anm2_destroy(&doc);
}

static void test_param_id_userdata(void) {
  struct ov_error err = {0};
  uint32_t id1 = 0;
  uint32_t id2 = 0;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add a selector and animation item
  if (!ptk_anm2_selector_add(doc, "Group1", &err)) {
    goto cleanup;
  }
  if (!ptk_anm2_item_add_animation(doc, 0, "Script", "Anim1", &err)) {
    goto cleanup;
  }

  // Add params
  id1 = ptk_anm2_param_add(doc, 0, 0, "key1", "val1", &err);
  TEST_ASSERT_SUCCEEDED(id1 != 0, &err);
  id2 = ptk_anm2_param_add(doc, 0, 0, "key2", "val2", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);

  // IDs should be unique and non-zero
  TEST_CHECK(id1 > 0);
  TEST_CHECK(id2 > 0);
  TEST_CHECK(id1 != id2);

  // IDs should be retrievable
  TEST_CHECK(ptk_anm2_param_get_id(doc, 0, 0, 0) == id1);
  TEST_CHECK(ptk_anm2_param_get_id(doc, 0, 0, 1) == id2);

  // Invalid indices should return 0
  TEST_CHECK(ptk_anm2_param_get_id(doc, 0, 0, 999) == 0);
  TEST_CHECK(ptk_anm2_param_get_id(doc, 0, 999, 0) == 0);
  TEST_CHECK(ptk_anm2_param_get_id(doc, 999, 0, 0) == 0);

  // Test userdata (default is 0)
  TEST_CHECK(ptk_anm2_param_get_userdata(doc, 0, 0, 0) == 0);

  // Set and get userdata
  ptk_anm2_param_set_userdata(doc, 0, 0, 0, 0x1111);
  ptk_anm2_param_set_userdata(doc, 0, 0, 1, 0x2222);
  TEST_CHECK(ptk_anm2_param_get_userdata(doc, 0, 0, 0) == 0x1111);
  TEST_CHECK(ptk_anm2_param_get_userdata(doc, 0, 0, 1) == 0x2222);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// ID reverse lookup tests (Phase 5)
// ============================================================================

static void test_find_selector_by_id(void) {
  struct ov_error err = {0};
  uint32_t id1 = 0;
  uint32_t id2 = 0;
  uint32_t id3 = 0;
  size_t found_idx = SIZE_MAX;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add selectors
  id1 = ptk_anm2_selector_add(doc, "Group1", &err);
  TEST_ASSERT_SUCCEEDED(id1 != 0, &err);
  id2 = ptk_anm2_selector_add(doc, "Group2", &err);
  TEST_ASSERT_SUCCEEDED(id2 != 0, &err);
  id3 = ptk_anm2_selector_add(doc, "Group3", &err);
  TEST_ASSERT_SUCCEEDED(id3 != 0, &err);

  // Find by ID
  TEST_CHECK(ptk_anm2_find_selector_by_id(doc, id1, &found_idx));
  TEST_CHECK(found_idx == 0);
  TEST_CHECK(ptk_anm2_find_selector_by_id(doc, id2, &found_idx));
  TEST_CHECK(found_idx == 1);
  TEST_CHECK(ptk_anm2_find_selector_by_id(doc, id3, &found_idx));
  TEST_CHECK(found_idx == 2);

  // ID not found
  TEST_CHECK(!ptk_anm2_find_selector_by_id(doc, 999999, &found_idx));

  // ID 0 should not be found
  TEST_CHECK(!ptk_anm2_find_selector_by_id(doc, 0, &found_idx));

  // NULL output param is allowed
  TEST_CHECK(ptk_anm2_find_selector_by_id(doc, id2, NULL));

  // After removal, ID should not be found anymore
  if (!TEST_SUCCEEDED(ptk_anm2_selector_remove(doc, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(!ptk_anm2_find_selector_by_id(doc, id1, &found_idx));
  // id2 should now be at index 0
  TEST_CHECK(ptk_anm2_find_selector_by_id(doc, id2, &found_idx));
  TEST_CHECK(found_idx == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_find_item_by_id(void) {
  struct ov_error err = {0};
  uint32_t sel_id1 = 0;
  uint32_t sel_id2 = 0;
  uint32_t item_id1 = 0;
  uint32_t item_id2 = 0;
  uint32_t item_id3 = 0;
  size_t found_sel_idx = SIZE_MAX;
  size_t found_item_idx = SIZE_MAX;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add selectors and items
  sel_id1 = ptk_anm2_selector_add(doc, "Group1", &err);
  TEST_ASSERT_SUCCEEDED(sel_id1 != 0, &err);
  sel_id2 = ptk_anm2_selector_add(doc, "Group2", &err);
  TEST_ASSERT_SUCCEEDED(sel_id2 != 0, &err);

  item_id1 = ptk_anm2_item_add_value(doc, 0, "Item1", "path1", &err);
  TEST_ASSERT_SUCCEEDED(item_id1 != 0, &err);
  item_id2 = ptk_anm2_item_add_value(doc, 0, "Item2", "path2", &err);
  TEST_ASSERT_SUCCEEDED(item_id2 != 0, &err);
  item_id3 = ptk_anm2_item_add_value(doc, 1, "Item3", "path3", &err);
  TEST_ASSERT_SUCCEEDED(item_id3 != 0, &err);

  // Find by ID
  TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id1, &found_sel_idx, &found_item_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 0);

  TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id2, &found_sel_idx, &found_item_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 1);

  TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id3, &found_sel_idx, &found_item_idx));
  TEST_CHECK(found_sel_idx == 1);
  TEST_CHECK(found_item_idx == 0);

  // ID not found
  TEST_CHECK(!ptk_anm2_find_item_by_id(doc, 999999, &found_sel_idx, &found_item_idx));

  // ID 0 should not be found
  TEST_CHECK(!ptk_anm2_find_item_by_id(doc, 0, &found_sel_idx, &found_item_idx));

  // NULL output params are allowed
  TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id1, NULL, NULL));

  // Selector ID should NOT be found as item
  TEST_CHECK(!ptk_anm2_find_item_by_id(doc, sel_id1, &found_sel_idx, &found_item_idx));

  // After removal, ID should not be found
  if (!TEST_SUCCEEDED(ptk_anm2_item_remove(doc, 0, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(!ptk_anm2_find_item_by_id(doc, item_id1, &found_sel_idx, &found_item_idx));
  // item_id2 should now be at index 0 of selector 0
  TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id2, &found_sel_idx, &found_item_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_find_param_by_id(void) {
  struct ov_error err = {0};
  uint32_t param_id1 = 0;
  uint32_t param_id2 = 0;
  uint32_t param_id3 = 0;
  uint32_t item_id1 = 0;
  size_t found_sel_idx = SIZE_MAX;
  size_t found_item_idx = SIZE_MAX;
  size_t found_param_idx = SIZE_MAX;
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add selector with animation items (only animation items can have params)
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group2", &err), &err)) {
    goto cleanup;
  }

  item_id1 = ptk_anm2_item_add_animation(doc, 0, "Script1", "Anim1", &err);
  TEST_ASSERT_SUCCEEDED(item_id1 != 0, &err);
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 1, "Script2", "Anim2", &err), &err)) {
    goto cleanup;
  }

  // Add params
  param_id1 = ptk_anm2_param_add(doc, 0, 0, "key1", "val1", &err);
  TEST_ASSERT_SUCCEEDED(param_id1 != 0, &err);
  param_id2 = ptk_anm2_param_add(doc, 0, 0, "key2", "val2", &err);
  TEST_ASSERT_SUCCEEDED(param_id2 != 0, &err);
  param_id3 = ptk_anm2_param_add(doc, 1, 0, "key3", "val3", &err);
  TEST_ASSERT_SUCCEEDED(param_id3 != 0, &err);

  // Find by ID
  TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id1, &found_sel_idx, &found_item_idx, &found_param_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 0);
  TEST_CHECK(found_param_idx == 0);

  TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id2, &found_sel_idx, &found_item_idx, &found_param_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 0);
  TEST_CHECK(found_param_idx == 1);

  TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id3, &found_sel_idx, &found_item_idx, &found_param_idx));
  TEST_CHECK(found_sel_idx == 1);
  TEST_CHECK(found_item_idx == 0);
  TEST_CHECK(found_param_idx == 0);

  // ID not found
  TEST_CHECK(!ptk_anm2_find_param_by_id(doc, 999999, &found_sel_idx, &found_item_idx, &found_param_idx));

  // ID 0 should not be found
  TEST_CHECK(!ptk_anm2_find_param_by_id(doc, 0, &found_sel_idx, &found_item_idx, &found_param_idx));

  // NULL output params are allowed
  TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id1, NULL, NULL, NULL));

  // Item ID should NOT be found as param
  TEST_CHECK(!ptk_anm2_find_param_by_id(doc, item_id1, &found_sel_idx, &found_item_idx, &found_param_idx));

  // After removal, ID should not be found
  if (!TEST_SUCCEEDED(ptk_anm2_param_remove(doc, 0, 0, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(!ptk_anm2_find_param_by_id(doc, param_id1, &found_sel_idx, &found_item_idx, &found_param_idx));
  // param_id2 should now be at index 0
  TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id2, &found_sel_idx, &found_item_idx, &found_param_idx));
  TEST_CHECK(found_sel_idx == 0);
  TEST_CHECK(found_item_idx == 0);
  TEST_CHECK(found_param_idx == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Metadata operation tests (Phase 4)
// ============================================================================

static void test_set_label(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Test Label", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "Test Label") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_can_redo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_set_psd_path(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "C:/path/to/test.psd", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_psd_path(doc), "C:/path/to/test.psd") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_metadata_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Set label
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "First", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "First") == 0);

  // Change label
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Second", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "Second") == 0);

  // Undo - should restore "First"
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "First") == 0);
  TEST_CHECK(ptk_anm2_can_undo(doc));
  TEST_CHECK(ptk_anm2_can_redo(doc));

  // Redo - should restore "Second"
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "Second") == 0);

  // Undo twice - should clear label
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "PSD") == 0);
  TEST_CHECK(!ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Transaction (grouped undo) tests
// ============================================================================

static void test_transaction_basic(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Begin transaction
  if (!TEST_SUCCEEDED(ptk_anm2_begin_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  // Multiple operations
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Label1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "path.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }

  // End transaction
  if (!TEST_SUCCEEDED(ptk_anm2_end_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "Label1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_get_psd_path(doc), "path.psd") == 0);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);

  // Single undo should revert all operations in the transaction
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "PSD") == 0);
  TEST_CHECK(ptk_anm2_get_psd_path(doc) == NULL || strlen(ptk_anm2_get_psd_path(doc)) == 0);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 0);

  // Single redo should restore all operations
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "Label1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_get_psd_path(doc), "path.psd") == 0);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_transaction_nested(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Begin outer transaction
  if (!TEST_SUCCEEDED(ptk_anm2_begin_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Outer", &err), &err)) {
    goto cleanup;
  }

  // Begin nested transaction
  if (!TEST_SUCCEEDED(ptk_anm2_begin_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Nested", &err), &err)) {
    goto cleanup;
  }

  // End nested transaction (should not record GROUP_END yet)
  if (!TEST_SUCCEEDED(ptk_anm2_end_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "after.psd", &err), &err)) {
    goto cleanup;
  }

  // End outer transaction
  if (!TEST_SUCCEEDED(ptk_anm2_end_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  // All operations should be grouped - single undo reverts all
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_get_label(doc), "PSD") == 0);
  TEST_CHECK(ptk_anm2_get_psd_path(doc) == NULL || strlen(ptk_anm2_get_psd_path(doc)) == 0);
  TEST_CHECK(ptk_anm2_selector_count(doc) == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Change callback tests
// ============================================================================

// Helper structure to track callback invocations
struct callback_record {
  enum ptk_anm2_op_type op_type;
  size_t sel_idx;
  size_t item_idx;
  size_t param_idx;
  size_t to_sel_idx;
  size_t to_idx;
};

struct callback_tracker {
  struct callback_record *records; // ovarray
  size_t count;
};

static void test_change_callback_fn(void *userdata,
                                    enum ptk_anm2_op_type op_type,
                                    size_t sel_idx,
                                    size_t item_idx,
                                    size_t param_idx,
                                    size_t to_sel_idx,
                                    size_t to_idx) {
  struct callback_tracker *tracker = (struct callback_tracker *)userdata;
  size_t const len = OV_ARRAY_LENGTH(tracker->records);
  if (OV_ARRAY_GROW(&tracker->records, len + 1)) {
    tracker->records[len] = (struct callback_record){
        .op_type = op_type,
        .sel_idx = sel_idx,
        .item_idx = item_idx,
        .param_idx = param_idx,
        .to_sel_idx = to_sel_idx,
        .to_idx = to_idx,
    };
    OV_ARRAY_SET_LENGTH(tracker->records, len + 1);
    tracker->count++;
  }
}

static void callback_tracker_clear(struct callback_tracker *tracker) {
  if (tracker->records) {
    OV_ARRAY_SET_LENGTH(tracker->records, 0);
  }
  tracker->count = 0;
}

static void callback_tracker_destroy(struct callback_tracker *tracker) {
  OV_ARRAY_DESTROY(&tracker->records);
  tracker->count = 0;
}

static void test_change_callback_basic(void) {
  struct ov_error err = {0};
  struct callback_tracker tracker = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  ptk_anm2_set_change_callback(doc, test_change_callback_fn, &tracker);

  // Test selector_add triggers callback
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err) != 0, &err)) {
    goto cleanup;
  }
  TEST_CHECK(tracker.count == 1);
  TEST_MSG("want 1 callback, got %zu", tracker.count);
  if (tracker.count >= 1) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_selector_insert);
    TEST_MSG("want op_type=%u, got %u", (unsigned)ptk_anm2_op_selector_insert, (unsigned)tracker.records[0].op_type);
  }

  // Test item_add triggers callback
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item1", "value1", &err) != 0, &err)) {
    goto cleanup;
  }
  TEST_CHECK(tracker.count == 1);
  TEST_MSG("want 1 callback, got %zu", tracker.count);
  if (tracker.count >= 1) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_item_insert);
  }

cleanup:
  callback_tracker_destroy(&tracker);
  ptk_anm2_destroy(&doc);
}

static void test_change_callback_transaction(void) {
  struct ov_error err = {0};
  struct callback_tracker tracker = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  ptk_anm2_set_change_callback(doc, test_change_callback_fn, &tracker);

  // Begin transaction should trigger group_begin callback
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_begin_transaction(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(tracker.count == 1);
  TEST_MSG("want 1 callback for begin_transaction, got %zu", tracker.count);
  if (tracker.count >= 1) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_group_begin);
    TEST_MSG("want op_type=%u (group_begin), got %u",
             (unsigned)ptk_anm2_op_group_begin,
             (unsigned)tracker.records[0].op_type);
  }

  // Operations inside transaction
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err) != 0, &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group2", &err) != 0, &err)) {
    goto cleanup;
  }
  TEST_CHECK(tracker.count == 2);
  TEST_MSG("want 2 callbacks for 2 selector_add, got %zu", tracker.count);

  // End transaction should trigger group_end callback
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_end_transaction(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(tracker.count == 1);
  TEST_MSG("want 1 callback for end_transaction, got %zu", tracker.count);
  if (tracker.count >= 1) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_group_end);
    TEST_MSG(
        "want op_type=%u (group_end), got %u", (unsigned)ptk_anm2_op_group_end, (unsigned)tracker.records[0].op_type);
  }

cleanup:
  callback_tracker_destroy(&tracker);
  ptk_anm2_destroy(&doc);
}

static void test_change_callback_undo_redo_transaction(void) {
  struct ov_error err = {0};
  struct callback_tracker tracker = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create a transaction with multiple operations
  if (!TEST_SUCCEEDED(ptk_anm2_begin_transaction(doc, &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err) != 0, &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group2", &err) != 0, &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_end_transaction(doc, &err), &err)) {
    goto cleanup;
  }

  // Now register callback and test UNDO
  ptk_anm2_set_change_callback(doc, test_change_callback_fn, &tracker);

  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }

  // UNDO of transaction
  // Original operations pushed reverse_ops to undo stack:
  //   begin_transaction -> push group_begin (no apply_op, pushed directly)
  //   selector_add -> apply_op(insert) -> push selector_remove
  //   selector_add -> apply_op(insert) -> push selector_remove
  //   end_transaction -> push group_end (no apply_op, pushed directly)
  // Undo stack: [group_begin, selector_remove, selector_remove, group_end]
  //
  // UNDO processes from end: group_end, selector_remove, selector_remove, group_begin
  // Each is passed to apply_op which notifies with op->type
  TEST_CHECK(tracker.count == 4);
  TEST_MSG("want 4 callbacks for undo of transaction, got %zu", tracker.count);
  if (tracker.count >= 4) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_group_end);
    TEST_MSG("want first op_type=%u (group_end), got %u",
             (unsigned)ptk_anm2_op_group_end,
             (unsigned)tracker.records[0].op_type);
    TEST_CHECK(tracker.records[1].op_type == ptk_anm2_op_selector_remove);
    TEST_CHECK(tracker.records[2].op_type == ptk_anm2_op_selector_remove);
    TEST_CHECK(tracker.records[3].op_type == ptk_anm2_op_group_begin);
    TEST_MSG("want last op_type=%u (group_begin), got %u",
             (unsigned)ptk_anm2_op_group_begin,
             (unsigned)tracker.records[3].op_type);
  }

  // Test REDO
  callback_tracker_clear(&tracker);
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }

  // REDO processes redo stack from end
  // Redo stack after UNDO: [group_begin, selector_insert, selector_insert, group_end]
  // REDO order: group_end, selector_insert, selector_insert, group_begin
  TEST_CHECK(tracker.count == 4);
  TEST_MSG("want 4 callbacks for redo of transaction, got %zu", tracker.count);
  if (tracker.count >= 4) {
    TEST_CHECK(tracker.records[0].op_type == ptk_anm2_op_group_end);
    TEST_MSG("want first op_type=%u (group_end), got %u",
             (unsigned)ptk_anm2_op_group_end,
             (unsigned)tracker.records[0].op_type);
    TEST_CHECK(tracker.records[1].op_type == ptk_anm2_op_selector_insert);
    TEST_CHECK(tracker.records[2].op_type == ptk_anm2_op_selector_insert);
    TEST_CHECK(tracker.records[3].op_type == ptk_anm2_op_group_begin);
    TEST_MSG("want last op_type=%u (group_begin), got %u",
             (unsigned)ptk_anm2_op_group_begin,
             (unsigned)tracker.records[3].op_type);
  }

cleanup:
  callback_tracker_destroy(&tracker);
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// UNDO/REDO edge case tests
// ============================================================================

static void test_undo_clears_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "First", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Second", &err), &err)) {
    goto cleanup;
  }

  // Undo to "First"
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_can_redo(doc));

  // New operation should clear redo stack
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Third", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(!ptk_anm2_can_redo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_clear_undo_history(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Test", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(ptk_anm2_can_redo(doc));

  ptk_anm2_clear_undo_history(doc);

  TEST_CHECK(!ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_can_redo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_undo_empty_returns_false(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  TEST_CHECK(!ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_undo(doc, &err));

  ptk_anm2_destroy(&doc);
}

static void test_redo_empty_returns_false(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  TEST_CHECK(!ptk_anm2_can_redo(doc));
  TEST_CHECK(!ptk_anm2_redo(doc, &err));

  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Error case tests
// ============================================================================

static void test_invalid_selector_index(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // No selectors - should return NULL/0/false
  TEST_CHECK(ptk_anm2_selector_get_group(doc, 0) == NULL);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 0);

  TEST_FAILED_WITH(
      ptk_anm2_selector_remove(doc, 0, &err), &err, ov_error_type_generic, ov_error_generic_invalid_argument);
  TEST_FAILED_WITH(ptk_anm2_selector_set_group(doc, 0, "test", &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

  ptk_anm2_destroy(&doc);
}

static void test_invalid_item_index(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }

  // No items - should return NULL/false
  TEST_CHECK(ptk_anm2_item_get_name(doc, 0, 0) == NULL);
  TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 0));

  TEST_FAILED_WITH(
      ptk_anm2_item_remove(doc, 0, 0, &err), &err, ov_error_type_generic, ov_error_generic_invalid_argument);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_invalid_param_index(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err), &err)) {
    goto cleanup;
  }

  // No params - should return NULL
  TEST_CHECK(ptk_anm2_param_get_key(doc, 0, 0, 0) == NULL);
  TEST_CHECK(ptk_anm2_param_get_value(doc, 0, 0, 0) == NULL);

  TEST_FAILED_WITH(
      ptk_anm2_param_remove(doc, 0, 0, 0, &err), &err, ov_error_type_generic, ov_error_generic_invalid_argument);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_on_value_item(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "ValueItem", "path", &err), &err)) {
    goto cleanup;
  }

  // Param operations on value item should fail
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 0);
  TEST_FAILED_WITH(ptk_anm2_param_add(doc, 0, 0, "key", "val", &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// File operation tests
// ============================================================================

static void test_load_basic(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_load(doc, test_data_basic, &err), &err)) {
    goto cleanup;
  }

  // Verify metadata
  TEST_CHECK(ptk_anm2_get_version(doc) == 1);
  {
    char const *psd_path = ptk_anm2_get_psd_path(doc);
    TEST_CHECK(psd_path != NULL && strcmp(psd_path, "C:/path/to/test.psd") == 0);
    TEST_MSG("want \"C:/path/to/test.psd\", got \"%s\"", psd_path ? psd_path : "(null)");
  }

  // Verify selector
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  {
    char const *group = ptk_anm2_selector_get_group(doc, 0);
    TEST_CHECK(group != NULL && strcmp(group, "表情") == 0);
    TEST_MSG("want \"表情\", got \"%s\"", group ? group : "(null)");
  }

  // Verify items
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  {
    TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 0));
    char const *name = ptk_anm2_item_get_name(doc, 0, 0);
    char const *value = ptk_anm2_item_get_value(doc, 0, 0);
    TEST_CHECK(name != NULL && strcmp(name, "通常") == 0);
    TEST_MSG("want \"通常\", got \"%s\"", name ? name : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "レイヤー/表情/通常") == 0);
    TEST_MSG("want \"レイヤー/表情/通常\", got \"%s\"", value ? value : "(null)");
  }
  {
    TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 1));
    char const *name = ptk_anm2_item_get_name(doc, 0, 1);
    char const *value = ptk_anm2_item_get_value(doc, 0, 1);
    TEST_CHECK(name != NULL && strcmp(name, "笑顔") == 0);
    TEST_MSG("want \"笑顔\", got \"%s\"", name ? name : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "レイヤー/表情/笑顔") == 0);
    TEST_MSG("want \"レイヤー/表情/笑顔\", got \"%s\"", value ? value : "(null)");
  }

  // Verify undo is cleared after load
  TEST_CHECK(!ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_can_redo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_load_animation(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_load(doc, test_data_animation, &err), &err)) {
    goto cleanup;
  }

  // Verify metadata
  TEST_CHECK(ptk_anm2_get_version(doc) == 1);
  {
    char const *psd_path = ptk_anm2_get_psd_path(doc);
    TEST_CHECK(psd_path != NULL && strcmp(psd_path, "C:/path/to/anim.psd") == 0);
    TEST_MSG("want \"C:/path/to/anim.psd\", got \"%s\"", psd_path ? psd_path : "(null)");
  }

  // Verify selector
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  {
    char const *group = ptk_anm2_selector_get_group(doc, 0);
    TEST_CHECK(group != NULL && strcmp(group, "目パチ") == 0);
    TEST_MSG("want \"目パチ\", got \"%s\"", group ? group : "(null)");
  }

  // Verify animation item
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(ptk_anm2_item_is_animation(doc, 0, 0));
  {
    char const *script_name = ptk_anm2_item_get_script_name(doc, 0, 0);
    TEST_CHECK(script_name != NULL && strcmp(script_name, "PSDToolKit.Blinker") == 0);
    TEST_MSG("want \"PSDToolKit.Blinker\", got \"%s\"", script_name ? script_name : "(null)");
  }
  {
    char const *name = ptk_anm2_item_get_name(doc, 0, 0);
    TEST_CHECK(name != NULL && strcmp(name, "目パチアニメ") == 0);
    TEST_MSG("want \"目パチアニメ\", got \"%s\"", name ? name : "(null)");
  }

  // Verify params
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 2);
  {
    char const *key = ptk_anm2_param_get_key(doc, 0, 0, 0);
    char const *value = ptk_anm2_param_get_value(doc, 0, 0, 0);
    TEST_CHECK(key != NULL && strcmp(key, "間隔(秒)") == 0);
    TEST_MSG("want \"間隔(秒)\", got \"%s\"", key ? key : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "5.00") == 0);
    TEST_MSG("want \"5.00\", got \"%s\"", value ? value : "(null)");
  }
  {
    char const *key = ptk_anm2_param_get_key(doc, 0, 0, 1);
    char const *value = ptk_anm2_param_get_value(doc, 0, 0, 1);
    TEST_CHECK(key != NULL && strcmp(key, "開き時間(秒)") == 0);
    TEST_MSG("want \"開き時間(秒)\", got \"%s\"", key ? key : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "0.06") == 0);
    TEST_MSG("want \"0.06\", got \"%s\"", value ? value : "(null)");
  }

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_load_mixed(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_load(doc, test_data_mixed, &err), &err)) {
    goto cleanup;
  }

  // Verify 2 selectors
  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);

  // First selector: value items
  {
    char const *group = ptk_anm2_selector_get_group(doc, 0);
    TEST_CHECK(group != NULL && strcmp(group, "表情") == 0);
    TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
    TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 0));
    TEST_CHECK(!ptk_anm2_item_is_animation(doc, 0, 1));
  }

  // Second selector: animation item
  {
    char const *group = ptk_anm2_selector_get_group(doc, 1);
    TEST_CHECK(group != NULL && strcmp(group, "目パチ") == 0);
    TEST_CHECK(ptk_anm2_item_count(doc, 1) == 1);
    TEST_CHECK(ptk_anm2_item_is_animation(doc, 1, 0));
    TEST_CHECK(ptk_anm2_param_count(doc, 1, 0) == 1);
  }

cleanup:
  ptk_anm2_destroy(&doc);
}

// Helper to create a temporary file path
static bool create_temp_path(wchar_t *buf, size_t buf_size) {
#ifdef _WIN32
  wchar_t temp_dir[MAX_PATH];
  if (!GetTempPathW(MAX_PATH, temp_dir)) {
    return false;
  }
  if (!GetTempFileNameW(temp_dir, L"anm2", 0, buf)) {
    return false;
  }
  // Add .lua extension
  size_t len = wcslen(buf);
  if (len + 5 >= buf_size) {
    return false;
  }
  wcscat(buf, L".lua");
  return true;
#else
  (void)buf;
  (void)buf_size;
  return false;
#endif
}

static void delete_temp_file(wchar_t const *path) {
#ifdef _WIN32
  DeleteFileW(path);
  // Also delete the original temp file (without .lua extension)
  wchar_t base_path[MAX_PATH];
  wcscpy(base_path, path);
  size_t len = wcslen(base_path);
  if (len > 4 && wcscmp(base_path + len - 4, L".lua") == 0) {
    base_path[len - 4] = L'\0';
    DeleteFileW(base_path);
  }
#else
  (void)path;
#endif
}

static void test_save_load_roundtrip(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  struct ptk_anm2 *loaded_doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create temp file path
  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Set up document with various content
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Test Label", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "C:/path/to/test.psd", &err), &err)) {
    goto cleanup;
  }

  // Add selector with value items
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "通常", "レイヤー/表情/通常", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "笑顔", "レイヤー/表情/笑顔", &err), &err)) {
    goto cleanup;
  }

  // Add selector with animation item
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "目パチ", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 1, "PSDToolKit.Blinker", "目パチアニメ", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 1, 0, "間隔(秒)", "5.00", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 1, 0, "開き時間(秒)", "0.06", &err), &err)) {
    goto cleanup;
  }

  // Save the document
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Create a new document and load
  loaded_doc = ptk_anm2_new(&err);
  if (!TEST_CHECK(loaded_doc != NULL)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_load(loaded_doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Verify loaded content matches original
  TEST_CHECK(ptk_anm2_get_version(loaded_doc) == 1);

  // Check PSD path
  {
    char const *psd_path = ptk_anm2_get_psd_path(loaded_doc);
    if (!TEST_CHECK(psd_path != NULL)) {
      TEST_MSG("want non-NULL psd_path, got NULL");
    } else {
      TEST_CHECK(strcmp(psd_path, "C:/path/to/test.psd") == 0);
      TEST_MSG("want \"C:/path/to/test.psd\", got \"%s\"", psd_path);
    }
  }

  // Check label
  {
    char const *label = ptk_anm2_get_label(loaded_doc);
    if (!TEST_CHECK(label != NULL)) {
      TEST_MSG("want non-NULL label, got NULL");
    } else {
      TEST_CHECK(strcmp(label, "Test Label") == 0);
      TEST_MSG("want \"Test Label\", got \"%s\"", label);
    }
  }

  // Check selector count
  TEST_CHECK(ptk_anm2_selector_count(loaded_doc) == 2);
  TEST_MSG("want 2 selectors, got %zu", ptk_anm2_selector_count(loaded_doc));

  // Check first selector
  {
    char const *group = ptk_anm2_selector_get_group(loaded_doc, 0);
    if (!TEST_CHECK(group != NULL && strcmp(group, "表情") == 0)) {
      TEST_MSG("want \"表情\", got \"%s\"", group ? group : "(null)");
    }
    TEST_CHECK(ptk_anm2_item_count(loaded_doc, 0) == 2);

    // First item
    TEST_CHECK(!ptk_anm2_item_is_animation(loaded_doc, 0, 0));
    {
      char const *name = ptk_anm2_item_get_name(loaded_doc, 0, 0);
      TEST_CHECK(name != NULL && strcmp(name, "通常") == 0);
      TEST_MSG("want \"通常\", got \"%s\"", name ? name : "(null)");
    }
    {
      char const *value = ptk_anm2_item_get_value(loaded_doc, 0, 0);
      TEST_CHECK(value != NULL && strcmp(value, "レイヤー/表情/通常") == 0);
      TEST_MSG("want \"レイヤー/表情/通常\", got \"%s\"", value ? value : "(null)");
    }

    // Second item
    TEST_CHECK(!ptk_anm2_item_is_animation(loaded_doc, 0, 1));
    {
      char const *name = ptk_anm2_item_get_name(loaded_doc, 0, 1);
      TEST_CHECK(name != NULL && strcmp(name, "笑顔") == 0);
      TEST_MSG("want \"笑顔\", got \"%s\"", name ? name : "(null)");
    }
  }

  // Check second selector (animation)
  {
    char const *group = ptk_anm2_selector_get_group(loaded_doc, 1);
    if (!TEST_CHECK(group != NULL && strcmp(group, "目パチ") == 0)) {
      TEST_MSG("want \"目パチ\", got \"%s\"", group ? group : "(null)");
    }
    TEST_CHECK(ptk_anm2_item_count(loaded_doc, 1) == 1);

    // Animation item
    TEST_CHECK(ptk_anm2_item_is_animation(loaded_doc, 1, 0));
    {
      char const *script_name = ptk_anm2_item_get_script_name(loaded_doc, 1, 0);
      TEST_CHECK(script_name != NULL && strcmp(script_name, "PSDToolKit.Blinker") == 0);
      TEST_MSG("want \"PSDToolKit.Blinker\", got \"%s\"", script_name ? script_name : "(null)");
    }
    {
      char const *name = ptk_anm2_item_get_name(loaded_doc, 1, 0);
      TEST_CHECK(name != NULL && strcmp(name, "目パチアニメ") == 0);
      TEST_MSG("want \"目パチアニメ\", got \"%s\"", name ? name : "(null)");
    }

    // Check params
    TEST_CHECK(ptk_anm2_param_count(loaded_doc, 1, 0) == 2);
    {
      char const *key = ptk_anm2_param_get_key(loaded_doc, 1, 0, 0);
      char const *value = ptk_anm2_param_get_value(loaded_doc, 1, 0, 0);
      TEST_CHECK(key != NULL && strcmp(key, "間隔(秒)") == 0);
      TEST_MSG("want key \"間隔(秒)\", got \"%s\"", key ? key : "(null)");
      TEST_CHECK(value != NULL && strcmp(value, "5.00") == 0);
      TEST_MSG("want value \"5.00\", got \"%s\"", value ? value : "(null)");
    }
    {
      char const *key = ptk_anm2_param_get_key(loaded_doc, 1, 0, 1);
      char const *value = ptk_anm2_param_get_value(loaded_doc, 1, 0, 1);
      TEST_CHECK(key != NULL && strcmp(key, "開き時間(秒)") == 0);
      TEST_MSG("want key \"開き時間(秒)\", got \"%s\"", key ? key : "(null)");
      TEST_CHECK(value != NULL && strcmp(value, "0.06") == 0);
      TEST_MSG("want value \"0.06\", got \"%s\"", value ? value : "(null)");
    }
  }

cleanup:
  if (temp_path[0] != L'\0') {
    delete_temp_file(temp_path);
  }
  ptk_anm2_destroy(&doc);
  ptk_anm2_destroy(&loaded_doc);
}

static void test_load_clears_undo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create temp file path
  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Set up minimal document for saving
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item", "path", &err), &err)) {
    goto cleanup;
  }

  // Save the document
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Make some changes that add to undo stack
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "New Label", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_can_undo(doc));

  // Perform undo to add to redo stack
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_can_redo(doc));

  // Make another change
  if (!TEST_SUCCEEDED(ptk_anm2_set_label(doc, "Another Label", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_can_undo(doc));

  // Load should clear both undo and redo stacks
  if (!TEST_SUCCEEDED(ptk_anm2_load(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(!ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_can_redo(doc));

cleanup:
  if (temp_path[0] != L'\0') {
    delete_temp_file(temp_path);
  }
  ptk_anm2_destroy(&doc);
}

static void test_save_without_psd_path(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create temp file path
  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Save without PSD path - should succeed (empty document is valid)
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

cleanup:
  if (temp_path[0] != L'\0') {
    delete_temp_file(temp_path);
  }
  ptk_anm2_destroy(&doc);
}

static void test_save_load_empty_param_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  struct ptk_anm2 *loaded_doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create temp file path
  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Set up document with animation item that has empty param value
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Test", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "TestScript", "Test Item", &err), &err)) {
    goto cleanup;
  }

  // Add params with various empty/non-empty combinations
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key1", "value1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key2", "", &err), &err)) { // empty value
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key3", "value3", &err), &err)) {
    goto cleanup;
  }

  // Verify before save
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 3);

  // Save the document
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Create a new document and load
  loaded_doc = ptk_anm2_new(&err);
  if (!TEST_CHECK(loaded_doc != NULL)) {
    goto cleanup;
  }

  if (!TEST_SUCCEEDED(ptk_anm2_load(loaded_doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Verify all params were loaded including the one with empty value
  if (!TEST_CHECK(ptk_anm2_param_count(loaded_doc, 0, 0) == 3)) {
    TEST_MSG("want 3 params, got %zu", ptk_anm2_param_count(loaded_doc, 0, 0));
    goto cleanup;
  }

  // Check first param
  {
    char const *key = ptk_anm2_param_get_key(loaded_doc, 0, 0, 0);
    char const *value = ptk_anm2_param_get_value(loaded_doc, 0, 0, 0);
    TEST_CHECK(key != NULL && strcmp(key, "key1") == 0);
    TEST_MSG("param 0: want key \"key1\", got \"%s\"", key ? key : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "value1") == 0);
    TEST_MSG("param 0: want value \"value1\", got \"%s\"", value ? value : "(null)");
  }

  // Check second param (empty value)
  {
    char const *key = ptk_anm2_param_get_key(loaded_doc, 0, 0, 1);
    char const *value = ptk_anm2_param_get_value(loaded_doc, 0, 0, 1);
    TEST_CHECK(key != NULL && strcmp(key, "key2") == 0);
    TEST_MSG("param 1: want key \"key2\", got \"%s\"", key ? key : "(null)");
    // Empty string is stored as NULL internally, so we check for NULL or empty
    TEST_CHECK(value == NULL || strcmp(value, "") == 0);
    TEST_MSG("param 1: want value \"\" or NULL, got \"%s\"", value ? value : "(null)");
  }

  // Check third param
  {
    char const *key = ptk_anm2_param_get_key(loaded_doc, 0, 0, 2);
    char const *value = ptk_anm2_param_get_value(loaded_doc, 0, 0, 2);
    TEST_CHECK(key != NULL && strcmp(key, "key3") == 0);
    TEST_MSG("param 2: want key \"key3\", got \"%s\"", key ? key : "(null)");
    TEST_CHECK(value != NULL && strcmp(value, "value3") == 0);
    TEST_MSG("param 2: want value \"value3\", got \"%s\"", value ? value : "(null)");
  }

cleanup:
  if (temp_path[0] != L'\0') {
    delete_temp_file(temp_path);
  }
  ptk_anm2_destroy(&loaded_doc);
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Script generation tests
// ============================================================================

static void test_generate_script_single_selector(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up document
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "通常", "layer/normal", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "笑顔", "layer/smile", &err), &err)) {
    goto cleanup;
  }

  // Generate script
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }

  // Verify output contains expected patterns
  TEST_CHECK(content != NULL);
  TEST_CHECK(strstr(content, "--select@sel1:表情") != NULL);
  TEST_MSG("Expected --select@sel1 line");
  TEST_CHECK(strstr(content, "psdcall(function()") != NULL);
  TEST_MSG("Expected psdcall wrapper");
  TEST_CHECK(strstr(content, "add_layer_selector(1, function() return {") != NULL);
  TEST_MSG("Expected cached function call");
  TEST_CHECK(strstr(content, "} end, sel1, {exclusive = exclusive ~= 0})") != NULL);
  TEST_MSG("Expected add_layer_selector call with exclusive option");
  TEST_CHECK(strstr(content, "end)\n") != NULL);
  TEST_MSG("Expected psdcall closing");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_generate_script_multiple_selectors(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up document with 2 selectors
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }

  // First selector
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "通常", "layer/normal", &err), &err)) {
    goto cleanup;
  }

  // Second selector
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "目パチ", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 1, "PSDToolKit.Blinker", "目パチアニメ", &err), &err)) {
    goto cleanup;
  }

  // Generate script
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }

  // Verify output contains separate values for each selector
  TEST_CHECK(content != NULL);

  // psdcall wrapper
  TEST_CHECK(strstr(content, "psdcall(function()") != NULL);
  TEST_MSG("Expected psdcall wrapper");

  // First selector
  TEST_CHECK(strstr(content, "--select@sel1:表情") != NULL);
  TEST_MSG("Expected --select@sel1 line");
  TEST_CHECK(strstr(content, "add_layer_selector(1, function() return {") != NULL);
  TEST_MSG("Expected first cached function call");
  TEST_CHECK(strstr(content, "} end, sel1, {exclusive = exclusive ~= 0})") != NULL);
  TEST_MSG("Expected add_layer_selector(sel1) call with exclusive option");

  // Second selector
  TEST_CHECK(strstr(content, "--select@sel2:目パチ") != NULL);
  TEST_MSG("Expected --select@sel2 line");
  TEST_CHECK(strstr(content, "add_layer_selector(2, function() return {") != NULL);
  TEST_MSG("Expected second cached function call");
  TEST_CHECK(strstr(content, "} end, sel2, {exclusive = exclusive ~= 0})") != NULL);
  TEST_MSG("Expected add_layer_selector(sel2) call with exclusive option");

  // Closing psdcall
  TEST_CHECK(strstr(content, "end)\n") != NULL);
  TEST_MSG("Expected psdcall closing");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_generate_script_empty_selector_skipped(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up document with empty selector followed by non-empty
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }

  // First selector (empty - should be skipped)
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Empty", &err), &err)) {
    goto cleanup;
  }

  // Second selector (has items)
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 1, "通常", "layer/normal", &err), &err)) {
    goto cleanup;
  }

  // Generate script
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }

  // Verify empty selector is skipped
  TEST_CHECK(content != NULL);
  TEST_CHECK(strstr(content, "--select@sel1:Empty") == NULL);
  TEST_MSG("Empty selector should be skipped");
  TEST_CHECK(strstr(content, "psdcall(function()") != NULL);
  TEST_MSG("Expected psdcall wrapper");
  TEST_CHECK(strstr(content, "add_layer_selector(1, function() return {") != NULL);
  TEST_MSG("First non-empty selector should use cache index 1");

  // Second selector uses sel2 (index is preserved)
  TEST_CHECK(strstr(content, "--select@sel2:表情") != NULL);
  TEST_MSG("Expected --select@sel2 line");
  TEST_CHECK(strstr(content, "} end, sel2, {exclusive = exclusive ~= 0})") != NULL);
  TEST_MSG("Expected add_layer_selector(sel2) call with exclusive option");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_generate_script_animation_params(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up document with animation item with params
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "目パチ", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチアニメ", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "間隔(秒)", "5.00", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "開き時間(秒)", "0.06", &err), &err)) {
    goto cleanup;
  }

  // Generate script
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }

  // Verify animation code
  TEST_CHECK(content != NULL);
  TEST_CHECK(strstr(content, "require(\"PSDToolKit.Blinker\").new({") != NULL);
  TEST_MSG("Expected require().new({ call");
  TEST_CHECK(strstr(content, "[\"間隔(秒)\"]") != NULL);
  TEST_MSG("Expected param key");
  TEST_CHECK(strstr(content, "\"5.00\"") != NULL);
  TEST_MSG("Expected param value");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_generate_script_null_param_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up document with animation item with empty param value
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Test", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "Script", "Name", &err), &err)) {
    goto cleanup;
  }
  // Add param with empty value (which becomes NULL internally)
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key", "", &err), &err)) {
    goto cleanup;
  }

  // Generate script - should not crash
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }

  // Verify script was generated
  TEST_CHECK(content != NULL);
  TEST_CHECK(strstr(content, "[\"key\"] = \"\"") != NULL);
  TEST_MSG("Expected empty value to be escaped as empty string");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Checksum verification tests
// ============================================================================

static void test_verify_checksum(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  struct ptk_anm2 *loaded_doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Create temp file path
  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Set up document
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item", "path", &err), &err)) {
    goto cleanup;
  }

  // Save the document
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Load and verify checksum matches
  loaded_doc = ptk_anm2_new(&err);
  if (!TEST_CHECK(loaded_doc != NULL)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_load(loaded_doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Checksum should match for unmodified file
  TEST_CHECK(ptk_anm2_verify_checksum(loaded_doc));
  TEST_MSG("Checksum should match for unmodified file: stored=%016llx calculated=%016llx",
           (unsigned long long)loaded_doc->stored_checksum,
           (unsigned long long)loaded_doc->calculated_checksum);

cleanup:
  if (temp_path[0] != L'\0') {
    delete_temp_file(temp_path);
  }
  ptk_anm2_destroy(&doc);
  ptk_anm2_destroy(&loaded_doc);
}

// ============================================================================
// Item set_script_name tests
// ============================================================================

static void test_item_set_script_name(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "目パチ", &err), &err)) {
    goto cleanup;
  }

  // Change script name
  if (!TEST_SUCCEEDED(ptk_anm2_item_set_script_name(doc, 0, 0, "PSDToolKit.LipSync", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "PSDToolKit.LipSync") == 0);
  TEST_MSG("Expected script name to be changed");
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_set_script_name_on_value_item(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "ValueItem", "path", &err), &err)) {
    goto cleanup;
  }

  // Setting script name on value item should fail
  TEST_FAILED_WITH(ptk_anm2_item_set_script_name(doc, 0, 0, "Script", &err),
                   &err,
                   ov_error_type_generic,
                   ov_error_generic_invalid_argument);

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// UNDO/REDO roundtrip tests for set operations
// ============================================================================

static void test_selector_set_group_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  char *content = NULL;
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Setup: add selector with items
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Original", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item1", "path/to/layer1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "PSDToolKit.Blinker", "Anim1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 1, "interval", "4", &err), &err)) {
    goto cleanup;
  }

  // Verify initial state
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Original") == 0);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "path/to/layer1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Anim1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 1), "PSDToolKit.Blinker") == 0);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 1) == 1);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 1, 0), "interval") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 1, 0), "4") == 0);

  // Generate script and verify initial content
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "\"group\":\"Original\"") != NULL);
  TEST_MSG("Initial content should have group 'Original'");
  TEST_CHECK(strstr(content, "--select@sel1:Original") != NULL);
  TEST_MSG("Initial content should have selector line with 'Original'");
  TEST_CHECK(strstr(content, "\"path/to/layer1\"") != NULL);
  TEST_MSG("Initial content should have item value");
  TEST_CHECK(strstr(content, "PSDToolKit.Blinker") != NULL);
  TEST_MSG("Initial content should have script name");
  TEST_CHECK(strstr(content, "interval") != NULL);
  TEST_MSG("Initial content should have param key");
  OV_ARRAY_DESTROY(&content);

  // Change group
  if (!TEST_SUCCEEDED(ptk_anm2_selector_set_group(doc, 0, "Modified", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Modified") == 0);
  // Verify items are unchanged
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "path/to/layer1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Anim1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 1), "PSDToolKit.Blinker") == 0);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 1) == 1);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 1, 0), "interval") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 1, 0), "4") == 0);

  // Generate script and verify modified content
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "\"group\":\"Modified\"") != NULL);
  TEST_MSG("Modified content should have group 'Modified'");
  TEST_CHECK(strstr(content, "--select@sel1:Modified") != NULL);
  TEST_MSG("Modified content should have selector line with 'Modified'");
  TEST_CHECK(strstr(content, "\"path/to/layer1\"") != NULL);
  TEST_MSG("Modified content should have item value");
  TEST_CHECK(strstr(content, "PSDToolKit.Blinker") != NULL);
  TEST_MSG("Modified content should have script name");
  TEST_CHECK(strstr(content, "interval") != NULL);
  TEST_MSG("Modified content should have param key");
  OV_ARRAY_DESTROY(&content);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_MSG("After undo, selector count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Original") == 0);
  TEST_MSG("After undo, group should be 'Original'");
  // Verify items are unchanged
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_MSG("After undo, item count should be 2");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_MSG("After undo, item 0 name should be 'Item1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "path/to/layer1") == 0);
  TEST_MSG("After undo, item 0 value should be 'path/to/layer1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Anim1") == 0);
  TEST_MSG("After undo, item 1 name should be 'Anim1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 1), "PSDToolKit.Blinker") == 0);
  TEST_MSG("After undo, item 1 script_name should be 'PSDToolKit.Blinker'");
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 1) == 1);
  TEST_MSG("After undo, param count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 1, 0), "interval") == 0);
  TEST_MSG("After undo, param key should be 'interval'");
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 1, 0), "4") == 0);
  TEST_MSG("After undo, param value should be '4'");
  TEST_CHECK(ptk_anm2_can_undo(doc));
  TEST_CHECK(ptk_anm2_can_redo(doc));

  // Generate script and verify undo content
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "\"group\":\"Original\"") != NULL);
  TEST_MSG("After undo, content should have group 'Original'");
  TEST_CHECK(strstr(content, "--select@sel1:Original") != NULL);
  TEST_MSG("After undo, content should have selector line with 'Original'");
  TEST_CHECK(strstr(content, "\"group\":\"Modified\"") == NULL);
  TEST_MSG("After undo, content should NOT have group 'Modified'");
  TEST_CHECK(strstr(content, "\"path/to/layer1\"") != NULL);
  TEST_MSG("After undo, content should have item value");
  TEST_CHECK(strstr(content, "PSDToolKit.Blinker") != NULL);
  TEST_MSG("After undo, content should have script name");
  TEST_CHECK(strstr(content, "interval") != NULL);
  TEST_MSG("After undo, content should have param key");
  OV_ARRAY_DESTROY(&content);

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_MSG("After redo, selector count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Modified") == 0);
  TEST_MSG("After redo, group should be 'Modified'");
  // Verify items are unchanged
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_MSG("After redo, item count should be 2");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_MSG("After redo, item 0 name should be 'Item1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "path/to/layer1") == 0);
  TEST_MSG("After redo, item 0 value should be 'path/to/layer1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Anim1") == 0);
  TEST_MSG("After redo, item 1 name should be 'Anim1'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 1), "PSDToolKit.Blinker") == 0);
  TEST_MSG("After redo, item 1 script_name should be 'PSDToolKit.Blinker'");
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 1) == 1);
  TEST_MSG("After redo, param count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 1, 0), "interval") == 0);
  TEST_MSG("After redo, param key should be 'interval'");
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 1, 0), "4") == 0);
  TEST_MSG("After redo, param value should be '4'");
  TEST_CHECK(ptk_anm2_can_undo(doc));
  TEST_CHECK(!ptk_anm2_can_redo(doc));

  // Generate script and verify redo content
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "\"group\":\"Modified\"") != NULL);
  TEST_MSG("After redo, content should have group 'Modified'");
  TEST_CHECK(strstr(content, "--select@sel1:Modified") != NULL);
  TEST_MSG("After redo, content should have selector line with 'Modified'");
  TEST_CHECK(strstr(content, "\"group\":\"Original\"") == NULL);
  TEST_MSG("After redo, content should NOT have group 'Original'");
  TEST_CHECK(strstr(content, "\"path/to/layer1\"") != NULL);
  TEST_MSG("After redo, content should have item value");
  TEST_CHECK(strstr(content, "PSDToolKit.Blinker") != NULL);
  TEST_MSG("After redo, content should have script name");
  TEST_CHECK(strstr(content, "interval") != NULL);
  TEST_MSG("After redo, content should have param key");
  OV_ARRAY_DESTROY(&content);

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_item_set_name_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Original", "path", &err), &err)) {
    goto cleanup;
  }

  // Change name
  if (!TEST_SUCCEEDED(ptk_anm2_item_set_name(doc, 0, 0, "Modified", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Modified") == 0);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Original") == 0);
  TEST_MSG("After undo, name should be 'Original'");

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Modified") == 0);
  TEST_MSG("After redo, name should be 'Modified'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_set_value_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item", "original/path", &err), &err)) {
    goto cleanup;
  }

  // Change value
  if (!TEST_SUCCEEDED(ptk_anm2_item_set_value(doc, 0, 0, "modified/path", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "modified/path") == 0);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "original/path") == 0);
  TEST_MSG("After undo, value should be 'original/path'");

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_value(doc, 0, 0), "modified/path") == 0);
  TEST_MSG("After redo, value should be 'modified/path'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_set_script_name_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "Original.Script", "Anim", &err), &err)) {
    goto cleanup;
  }

  // Change script name
  if (!TEST_SUCCEEDED(ptk_anm2_item_set_script_name(doc, 0, 0, "Modified.Script", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "Modified.Script") == 0);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "Original.Script") == 0);
  TEST_MSG("After undo, script_name should be 'Original.Script'");

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "Modified.Script") == 0);
  TEST_MSG("After redo, script_name should be 'Modified.Script'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_set_key_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "Script", "Anim", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "original_key", "value", &err), &err)) {
    goto cleanup;
  }

  // Change key
  if (!TEST_SUCCEEDED(ptk_anm2_param_set_key(doc, 0, 0, 0, "modified_key", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "modified_key") == 0);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "original_key") == 0);
  TEST_MSG("After undo, key should be 'original_key'");

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "modified_key") == 0);
  TEST_MSG("After redo, key should be 'modified_key'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_param_set_value_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "Script", "Anim", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key", "original_value", &err), &err)) {
    goto cleanup;
  }

  // Change value
  if (!TEST_SUCCEEDED(ptk_anm2_param_set_value(doc, 0, 0, 0, "modified_value", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "modified_value") == 0);

  // Undo
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "original_value") == 0);
  TEST_MSG("After undo, value should be 'original_value'");

  // Redo
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "modified_value") == 0);
  TEST_MSG("After redo, value should be 'modified_value'");

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// UNDO/REDO roundtrip tests for move operations
// ============================================================================

static void test_selector_move_to_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "A", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "B", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "C", &err), &err)) {
    goto cleanup;
  }

  // Move A (index 0) to index 2 -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_selector_move_to(doc, 0, 2, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "B") == 0);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 1), "C") == 0);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 2), "A") == 0);

  // Undo -> order should be A, B, C
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "A") == 0);
  TEST_MSG("After undo, index 0 should be 'A'");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 1), "B") == 0);
  TEST_MSG("After undo, index 1 should be 'B'");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 2), "C") == 0);
  TEST_MSG("After undo, index 2 should be 'C'");

  // Redo -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "B") == 0);
  TEST_MSG("After redo, index 0 should be 'B'");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 1), "C") == 0);
  TEST_MSG("After redo, index 1 should be 'C'");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 2), "A") == 0);
  TEST_MSG("After redo, index 2 should be 'A'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_move_to_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "A", "pathA", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "B", "pathB", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "C", "pathC", &err), &err)) {
    goto cleanup;
  }

  // Move A (index 0) to index 2 -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_item_move_to(doc, 0, 0, 0, 2, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "B") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "C") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "A") == 0);

  // Undo -> order should be A, B, C
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "A") == 0);
  TEST_MSG("After undo, index 0 should be 'A'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "B") == 0);
  TEST_MSG("After undo, index 1 should be 'B'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "C") == 0);
  TEST_MSG("After undo, index 2 should be 'C'");

  // Redo -> order should be B, C, A
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "B") == 0);
  TEST_MSG("After redo, index 0 should be 'B'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "C") == 0);
  TEST_MSG("After redo, index 1 should be 'C'");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 2), "A") == 0);
  TEST_MSG("After redo, index 2 should be 'A'");

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// UNDO/REDO roundtrip tests for remove operations
// ============================================================================

static void test_selector_remove_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Add selector with items
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item1", "path1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item2", "path2", &err), &err)) {
    goto cleanup;
  }

  // Add another selector
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group2", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);

  // Remove first selector (with items)
  if (!TEST_SUCCEEDED(ptk_anm2_selector_remove(doc, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group2") == 0);

  // Undo -> selector and items should be restored
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);
  TEST_MSG("After undo, selector count should be 2");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group1") == 0);
  TEST_MSG("After undo, index 0 should be 'Group1'");
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_MSG("After undo, items should be restored");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "Item1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 1), "Item2") == 0);

  // Redo -> selector should be removed again
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_selector_count(doc) == 1);
  TEST_MSG("After redo, selector count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_selector_get_group(doc, 0), "Group2") == 0);
  TEST_MSG("After redo, index 0 should be 'Group2'");

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_item_remove_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group1", &err), &err)) {
    goto cleanup;
  }
  // Add animation item with params
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_animation(doc, 0, "Script", "AnimItem", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key1", "val1", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_param_add(doc, 0, 0, "key2", "val2", &err), &err)) {
    goto cleanup;
  }
  // Add another item
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "ValueItem", "path", &err), &err)) {
    goto cleanup;
  }

  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 2);

  // Remove first item (animation with params)
  if (!TEST_SUCCEEDED(ptk_anm2_item_remove(doc, 0, 0, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "ValueItem") == 0);

  // Undo -> animation item and params should be restored
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_MSG("After undo, item count should be 2");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "AnimItem") == 0);
  TEST_MSG("After undo, first item should be 'AnimItem'");
  TEST_CHECK(ptk_anm2_item_is_animation(doc, 0, 0));
  TEST_CHECK(strcmp(ptk_anm2_item_get_script_name(doc, 0, 0), "Script") == 0);
  TEST_CHECK(ptk_anm2_param_count(doc, 0, 0) == 2);
  TEST_MSG("After undo, params should be restored");
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 0), "key1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 0), "val1") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_key(doc, 0, 0, 1), "key2") == 0);
  TEST_CHECK(strcmp(ptk_anm2_param_get_value(doc, 0, 0, 1), "val2") == 0);

  // Redo -> item should be removed again
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 1);
  TEST_MSG("After redo, item count should be 1");
  TEST_CHECK(strcmp(ptk_anm2_item_get_name(doc, 0, 0), "ValueItem") == 0);
  TEST_MSG("After redo, first item should be 'ValueItem'");

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Exclusive default and information tests (Phase 6)
// ============================================================================

static void test_exclusive_support_default_default_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // New document should have exclusive_support_default = true
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == true);

  ptk_anm2_destroy(&doc);
}

static void test_exclusive_support_default_set_get(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Set exclusive_support_default to false
  if (!TEST_SUCCEEDED(ptk_anm2_set_exclusive_support_default(doc, false, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == false);

  // Set exclusive_support_default to true
  if (!TEST_SUCCEEDED(ptk_anm2_set_exclusive_support_default(doc, true, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == true);

  // Should be undoable
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_exclusive_support_default_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Initially true
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == true);

  // Set to false
  if (!TEST_SUCCEEDED(ptk_anm2_set_exclusive_support_default(doc, false, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == false);

  // Undo -> should be true again
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == true);
  TEST_CHECK(ptk_anm2_can_redo(doc));

  // Redo -> should be false again
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(doc) == false);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_information_default_value(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // New document should have information = NULL (auto-generate)
  TEST_CHECK(ptk_anm2_get_information(doc) == NULL);

  ptk_anm2_destroy(&doc);
}

static void test_information_set_get(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Set custom information
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, "Custom Info", &err), &err)) {
    goto cleanup;
  }
  {
    char const *info = ptk_anm2_get_information(doc);
    if (!TEST_CHECK(info != NULL)) {
      TEST_MSG("want non-NULL information, got NULL");
    } else {
      TEST_CHECK(strcmp(info, "Custom Info") == 0);
      TEST_MSG("want \"Custom Info\", got \"%s\"", info);
    }
  }

  // Set to NULL (back to auto-generate mode)
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, NULL, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_information(doc) == NULL);

  // Should be undoable
  TEST_CHECK(ptk_anm2_can_undo(doc));

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_information_undo_redo(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  // Initially NULL
  TEST_CHECK(ptk_anm2_get_information(doc) == NULL);

  // Set custom value
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, "First", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_information(doc), "First") == 0);

  // Change to another value
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, "Second", &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_information(doc), "Second") == 0);

  // Undo -> should be "First"
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_information(doc), "First") == 0);

  // Undo -> should be NULL
  if (!TEST_SUCCEEDED(ptk_anm2_undo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(ptk_anm2_get_information(doc) == NULL);

  // Redo -> should be "First"
  if (!TEST_SUCCEEDED(ptk_anm2_redo(doc, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strcmp(ptk_anm2_get_information(doc), "First") == 0);

cleanup:
  ptk_anm2_destroy(&doc);
}

static void test_save_load_exclusive_support_and_information(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = NULL;
  struct ptk_anm2 *loaded_doc = NULL;
  wchar_t temp_path[MAX_PATH] = {0};

  doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_CHECK(create_temp_path(temp_path, MAX_PATH))) {
    TEST_MSG("Failed to create temp path");
    goto cleanup;
  }

  // Set up document
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "Group", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "Item", "layer/path", &err), &err)) {
    goto cleanup;
  }

  // Set exclusive_support_default to false and custom information
  if (!TEST_SUCCEEDED(ptk_anm2_set_exclusive_support_default(doc, false, &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, "Custom Information Text", &err), &err)) {
    goto cleanup;
  }

  // Save
  if (!TEST_SUCCEEDED(ptk_anm2_save(doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Load into new document
  loaded_doc = ptk_anm2_new(&err);
  if (!TEST_CHECK(loaded_doc != NULL)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_load(loaded_doc, temp_path, &err), &err)) {
    goto cleanup;
  }

  // Verify exclusive_support_default and information are preserved
  TEST_CHECK(ptk_anm2_get_exclusive_support_default(loaded_doc) == false);
  TEST_MSG("want exclusive_support_default=false, got true");

  {
    char const *info = ptk_anm2_get_information(loaded_doc);
    if (!TEST_CHECK(info != NULL)) {
      TEST_MSG("want non-NULL information, got NULL");
    } else {
      TEST_CHECK(strcmp(info, "Custom Information Text") == 0);
      TEST_MSG("want \"Custom Information Text\", got \"%s\"", info);
    }
  }

cleanup:
  DeleteFileW(temp_path);
  ptk_anm2_destroy(&doc);
  ptk_anm2_destroy(&loaded_doc);
}

static void test_generate_script_with_exclusive_support(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up basic document
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "通常", "layer/normal", &err), &err)) {
    goto cleanup;
  }

  // Default: exclusive_support_default = true
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "--check@exclusive:") != NULL);
  TEST_MSG("Expected --check@exclusive line");
  TEST_CHECK(strstr(content, ",1\n") != NULL);
  TEST_MSG("Expected default value 1 (true)");

  OV_ARRAY_DESTROY(&content);

  // Set exclusive_support_default = false
  if (!TEST_SUCCEEDED(ptk_anm2_set_exclusive_support_default(doc, false, &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "--check@exclusive:") != NULL);
  TEST_MSG("Expected --check@exclusive line");
  TEST_CHECK(strstr(content, ",0\n") != NULL);
  TEST_MSG("Expected default value 0 (false)");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

static void test_generate_script_with_custom_information(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  char *content = NULL;

  // Set up basic document
  if (!TEST_SUCCEEDED(ptk_anm2_set_psd_path(doc, "path/to/test.psd", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_selector_add(doc, "表情", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(ptk_anm2_item_add_value(doc, 0, "通常", "layer/normal", &err), &err)) {
    goto cleanup;
  }

  // Default: auto-generated information from filename
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "--information:PSD Layer Selector for test.psd") != NULL);
  TEST_MSG("Expected auto-generated information line");

  OV_ARRAY_DESTROY(&content);

  // Set custom information
  if (!TEST_SUCCEEDED(ptk_anm2_set_information(doc, "My Custom Description", &err), &err)) {
    goto cleanup;
  }
  if (!TEST_SUCCEEDED(generate_script_content(doc, &content, &err), &err)) {
    goto cleanup;
  }
  TEST_CHECK(strstr(content, "--information:My Custom Description") != NULL);
  TEST_MSG("Expected custom information line");
  TEST_CHECK(strstr(content, "--information:PSD Layer Selector") == NULL);
  TEST_MSG("Should not have auto-generated information");

cleanup:
  if (content) {
    OV_ARRAY_DESTROY(&content);
  }
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Load assigns IDs test (regression for drag & drop bug)
// ============================================================================

static void test_load_assigns_ids(void) {
  struct ov_error err = {0};
  struct ptk_anm2 *doc = ptk_anm2_new(&err);
  TEST_ASSERT_SUCCEEDED(doc != NULL, &err);

  if (!TEST_SUCCEEDED(ptk_anm2_load(doc, test_data_mixed, &err), &err)) {
    goto cleanup;
  }

  // Verify document has selectors and items
  TEST_CHECK(ptk_anm2_selector_count(doc) == 2);
  TEST_CHECK(ptk_anm2_item_count(doc, 0) == 2);
  TEST_CHECK(ptk_anm2_item_count(doc, 1) == 1);
  TEST_CHECK(ptk_anm2_param_count(doc, 1, 0) == 1);

  // Verify selector IDs are non-zero
  {
    uint32_t sel_id0 = ptk_anm2_selector_get_id(doc, 0);
    uint32_t sel_id1 = ptk_anm2_selector_get_id(doc, 1);
    TEST_CHECK(sel_id0 != 0);
    TEST_MSG("want non-zero selector ID at index 0, got %u", sel_id0);
    TEST_CHECK(sel_id1 != 0);
    TEST_MSG("want non-zero selector ID at index 1, got %u", sel_id1);
    TEST_CHECK(sel_id0 != sel_id1);
    TEST_MSG("selector IDs should be unique: %u vs %u", sel_id0, sel_id1);

    // Verify find_selector_by_id works for loaded selectors
    size_t found_idx = SIZE_MAX;
    TEST_CHECK(ptk_anm2_find_selector_by_id(doc, sel_id0, &found_idx));
    TEST_MSG("find_selector_by_id should find selector with id %u", sel_id0);
    TEST_CHECK(found_idx == 0);
    TEST_MSG("want index 0, got %zu", found_idx);

    TEST_CHECK(ptk_anm2_find_selector_by_id(doc, sel_id1, &found_idx));
    TEST_CHECK(found_idx == 1);
  }

  // Verify item IDs are non-zero
  {
    uint32_t item_id00 = ptk_anm2_item_get_id(doc, 0, 0);
    uint32_t item_id01 = ptk_anm2_item_get_id(doc, 0, 1);
    uint32_t item_id10 = ptk_anm2_item_get_id(doc, 1, 0);
    TEST_CHECK(item_id00 != 0);
    TEST_MSG("want non-zero item ID at (0,0), got %u", item_id00);
    TEST_CHECK(item_id01 != 0);
    TEST_MSG("want non-zero item ID at (0,1), got %u", item_id01);
    TEST_CHECK(item_id10 != 0);
    TEST_MSG("want non-zero item ID at (1,0), got %u", item_id10);
    TEST_CHECK(item_id00 != item_id01 && item_id01 != item_id10 && item_id00 != item_id10);
    TEST_MSG("item IDs should be unique");

    // Verify find_item_by_id works for loaded items
    size_t found_sel_idx = SIZE_MAX;
    size_t found_item_idx = SIZE_MAX;
    TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id00, &found_sel_idx, &found_item_idx));
    TEST_MSG("find_item_by_id should find item with id %u", item_id00);
    TEST_CHECK(found_sel_idx == 0 && found_item_idx == 0);

    TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id01, &found_sel_idx, &found_item_idx));
    TEST_CHECK(found_sel_idx == 0 && found_item_idx == 1);

    TEST_CHECK(ptk_anm2_find_item_by_id(doc, item_id10, &found_sel_idx, &found_item_idx));
    TEST_CHECK(found_sel_idx == 1 && found_item_idx == 0);
  }

  // Verify param IDs are non-zero (animation item at (1,0) has 1 param)
  {
    uint32_t param_id = ptk_anm2_param_get_id(doc, 1, 0, 0);
    TEST_CHECK(param_id != 0);
    TEST_MSG("want non-zero param ID at (1,0,0), got %u", param_id);

    // Verify find_param_by_id works for loaded params
    size_t found_sel_idx = SIZE_MAX;
    size_t found_item_idx = SIZE_MAX;
    size_t found_param_idx = SIZE_MAX;
    TEST_CHECK(ptk_anm2_find_param_by_id(doc, param_id, &found_sel_idx, &found_item_idx, &found_param_idx));
    TEST_MSG("find_param_by_id should find param with id %u", param_id);
    TEST_CHECK(found_sel_idx == 1 && found_item_idx == 0 && found_param_idx == 0);
  }

cleanup:
  ptk_anm2_destroy(&doc);
}

// ============================================================================
// Test list
// ============================================================================

TEST_LIST = {
    // Document lifecycle
    {"new_destroy", test_new_destroy},
    {"destroy_null", test_destroy_null},
    // Selector operations (Phase 2)
    {"selector_add", test_selector_add},
    {"selector_remove", test_selector_remove},
    {"selector_set_group", test_selector_set_group},
    {"selector_move_to", test_selector_move_to},
    {"selector_undo_redo", test_selector_undo_redo},
    // Item operations (Phase 2)
    {"item_add_value", test_item_add_value},
    {"item_insert_value", test_item_insert_value},
    {"item_add_animation", test_item_add_animation},
    {"item_insert_animation", test_item_insert_animation},
    {"item_remove", test_item_remove},
    {"item_move_to", test_item_move_to},
    {"item_undo_redo", test_item_undo_redo},
    // Param operations (Phase 2)
    {"param_add", test_param_add},
    {"param_insert", test_param_insert},
    {"param_remove", test_param_remove},
    {"param_set_key_value", test_param_set_key_value},
    {"param_undo_redo", test_param_undo_redo},
    // ID and userdata operations (Phase 3)
    {"selector_id_userdata", test_selector_id_userdata},
    {"item_id_userdata", test_item_id_userdata},
    {"param_id_userdata", test_param_id_userdata},
    // ID reverse lookup operations (Phase 5)
    {"find_selector_by_id", test_find_selector_by_id},
    {"find_item_by_id", test_find_item_by_id},
    {"find_param_by_id", test_find_param_by_id},
    // Metadata operations (Phase 4)
    {"set_label", test_set_label},
    {"set_psd_path", test_set_psd_path},
    {"metadata_undo_redo", test_metadata_undo_redo},
    // Transaction operations (Phase 4)
    {"transaction_basic", test_transaction_basic},
    {"transaction_nested", test_transaction_nested},
    // Change callback tests
    {"change_callback_basic", test_change_callback_basic},
    {"change_callback_transaction", test_change_callback_transaction},
    {"change_callback_undo_redo_transaction", test_change_callback_undo_redo_transaction},
    // UNDO/REDO edge cases (Phase 4)
    {"undo_clears_redo", test_undo_clears_redo},
    {"clear_undo_history", test_clear_undo_history},
    {"undo_empty_returns_false", test_undo_empty_returns_false},
    {"redo_empty_returns_false", test_redo_empty_returns_false},
    // Error cases (Phase 4)
    {"invalid_selector_index", test_invalid_selector_index},
    {"invalid_item_index", test_invalid_item_index},
    {"invalid_param_index", test_invalid_param_index},
    {"param_on_value_item", test_param_on_value_item},
    // File operations (Phase 4)
    {"load_basic", test_load_basic},
    {"load_animation", test_load_animation},
    {"load_mixed", test_load_mixed},
    {"load_assigns_ids", test_load_assigns_ids},
    {"save_load_roundtrip", test_save_load_roundtrip},
    {"load_clears_undo", test_load_clears_undo},
    {"save_without_psd_path", test_save_without_psd_path},
    {"save_load_empty_param_value", test_save_load_empty_param_value},
    // Script generation tests (Phase 4)
    {"generate_script_single_selector", test_generate_script_single_selector},
    {"generate_script_multiple_selectors", test_generate_script_multiple_selectors},
    {"generate_script_empty_selector_skipped", test_generate_script_empty_selector_skipped},
    {"generate_script_animation_params", test_generate_script_animation_params},
    {"generate_script_null_param_value", test_generate_script_null_param_value},
    {"verify_checksum", test_verify_checksum},
    // Item script name tests (Phase 4)
    {"item_set_script_name", test_item_set_script_name},
    {"item_set_script_name_on_value_item", test_item_set_script_name_on_value_item},
    // Additional undo/redo tests (Phase 4)
    {"selector_set_group_undo_redo", test_selector_set_group_undo_redo},
    {"item_set_name_undo_redo", test_item_set_name_undo_redo},
    {"item_set_value_undo_redo", test_item_set_value_undo_redo},
    {"item_set_script_name_undo_redo", test_item_set_script_name_undo_redo},
    {"param_set_key_undo_redo", test_param_set_key_undo_redo},
    {"param_set_value_undo_redo", test_param_set_value_undo_redo},
    {"selector_move_to_undo_redo", test_selector_move_to_undo_redo},
    {"item_move_to_undo_redo", test_item_move_to_undo_redo},
    {"selector_remove_undo_redo", test_selector_remove_undo_redo},
    {"item_remove_undo_redo", test_item_remove_undo_redo},
    // Exclusive default and information tests (Phase 6)
    {"exclusive_support_default_default_value", test_exclusive_support_default_default_value},
    {"exclusive_support_default_set_get", test_exclusive_support_default_set_get},
    {"exclusive_support_default_undo_redo", test_exclusive_support_default_undo_redo},
    {"information_default_value", test_information_default_value},
    {"information_set_get", test_information_set_get},
    {"information_undo_redo", test_information_undo_redo},
    {"save_load_exclusive_and_information", test_save_load_exclusive_support_and_information},
    {"generate_script_with_exclusive", test_generate_script_with_exclusive_support},
    {"generate_script_with_custom_information", test_generate_script_with_custom_information},
    {NULL, NULL},
};
