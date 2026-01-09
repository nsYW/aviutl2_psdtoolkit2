#include <ovtest.h>

#include <ovprintf.h>

#include <string.h>

#include "ini_reader.h"

#ifndef SOURCE_DIR
#  define SOURCE_DIR .
#endif

#define LSTR(x) L##x
#define LSTR2(x) LSTR(#x)
#define STRINGIZE(x) LSTR2(x)
#define TEST_PATH(relative_path) STRINGIZE(SOURCE_DIR) L"/test_data/" relative_path

static void test_create_destroy(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};

  TEST_ASSERT_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err);
  TEST_ASSERT(reader != NULL);
  ptk_ini_reader_destroy(&reader);
  ptk_ini_reader_destroy(NULL);
  TEST_FAILED_WITH(ptk_ini_reader_create(NULL, &err), &err, ov_error_type_generic, ov_error_generic_invalid_argument);
}

static void check_value_equals(struct ptk_ini_value result, char const *expected) {
  if (!expected) {
    TEST_CHECK(result.ptr == NULL);
    TEST_MSG("want NULL, got '%.*s' (len=%zu)", (int)result.size, result.ptr, result.size);
  } else {
    TEST_CHECK(result.ptr != NULL);
    TEST_MSG("want '%s', got NULL", expected);

    if (result.ptr != NULL) {
      size_t expected_len = strlen(expected);
      bool matches = result.size == expected_len && strncmp(result.ptr, expected, expected_len) == 0;
      TEST_CHECK(matches);
      TEST_MSG("want '%s' (len=%zu), got '%.*s' (len=%zu)",
               expected,
               expected_len,
               (int)result.size,
               result.ptr,
               result.size);
    }
  }
}

static void test_key_value_operations(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  struct ptk_ini_value result1 = ptk_ini_reader_get_value(reader, NULL, "nonexistent");
  TEST_ASSERT(result1.ptr == NULL);
  TEST_ASSERT(result1.size == 0);

  struct ptk_ini_value result2 = ptk_ini_reader_get_value(reader, "section", "nonexistent");
  TEST_ASSERT(result2.ptr == NULL);
  TEST_ASSERT(result2.size == 0);

  struct ptk_ini_value result3 = ptk_ini_reader_get_value(NULL, NULL, "key");
  TEST_ASSERT(result3.ptr == NULL);
  TEST_ASSERT(result3.size == 0);

  struct ptk_ini_value result4 = ptk_ini_reader_get_value(reader, NULL, NULL);
  TEST_ASSERT(result4.ptr == NULL);
  TEST_ASSERT(result4.size == 0);

  ptk_ini_reader_destroy(&reader);
}

static void test_basic_parsing(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/basic.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  struct ptk_ini_value global_value = ptk_ini_reader_get_value(reader, NULL, "global_key");
  check_value_equals(global_value, "global_value");
  struct ptk_ini_value value1 = ptk_ini_reader_get_value(reader, NULL, "key1");
  check_value_equals(value1, "value1");
  struct ptk_ini_value value2 = ptk_ini_reader_get_value(reader, NULL, "key2");
  check_value_equals(value2, "value2");
  struct ptk_ini_value value3 = ptk_ini_reader_get_value(reader, NULL, "key3");
  check_value_equals(value3, "value3");
  struct ptk_ini_value section_value = ptk_ini_reader_get_value(reader, "section1", "section_key");
  check_value_equals(section_value, "section_value");
  struct ptk_ini_value another_value = ptk_ini_reader_get_value(reader, "section1", "another_key");
  check_value_equals(another_value, "another_value");
  struct ptk_ini_value section2_value1 = ptk_ini_reader_get_value(reader, "section2", "key1");
  check_value_equals(section2_value1, "section2_value1");
  struct ptk_ini_value nonexistent1 = ptk_ini_reader_get_value(reader, NULL, "nonexistent");
  TEST_ASSERT(nonexistent1.ptr == NULL);
  struct ptk_ini_value nonexistent2 = ptk_ini_reader_get_value(reader, "section1", "nonexistent");
  TEST_ASSERT(nonexistent2.ptr == NULL);
  struct ptk_ini_value nonexistent3 = ptk_ini_reader_get_value(reader, "nonexistent_section", "key1");
  TEST_ASSERT(nonexistent3.ptr == NULL);

  ptk_ini_reader_destroy(&reader);
}

static void test_utf8_bom(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/utf8_bom.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  struct ptk_ini_value value = ptk_ini_reader_get_value(reader, "config", "bom_key");
  check_value_equals(value, "bom_value");

  ptk_ini_reader_destroy(&reader);
}

static void test_edge_cases(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/empty.ini"), &err), &err);

  TEST_FAILED_WITH(ptk_ini_reader_load_file(reader, TEST_PATH(L"nonexistent.ini"), &err),
                   &err,
                   ov_error_type_hresult,
                   HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));

  ptk_ini_reader_destroy(&reader);
}

static void test_complex_ini(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/complex.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  struct ptk_ini_value global1 = ptk_ini_reader_get_value(reader, NULL, "global1");
  check_value_equals(global1, "global_value1");
  struct ptk_ini_value spaces_key = ptk_ini_reader_get_value(reader, "Section With Spaces", "key with spaces");
  check_value_equals(spaces_key, "value with spaces");
  struct ptk_ini_value quoted = ptk_ini_reader_get_value(reader, "Section With Spaces", "quoted");
  check_value_equals(quoted, "\"quoted value\"");
  struct ptk_ini_value equals_val = ptk_ini_reader_get_value(reader, "Section With Spaces", "equals_in_value");
  check_value_equals(equals_val, "a=b=c");
  struct ptk_ini_value trailing = ptk_ini_reader_get_value(reader, "Section With Spaces", "trailing_spaces");
  check_value_equals(trailing, "value with trailing spaces");
  struct ptk_ini_value leading = ptk_ini_reader_get_value(reader, "Section With Spaces", "leading_spaces");
  check_value_equals(leading, "value with leading spaces");
  struct ptk_ini_value semicolon = ptk_ini_reader_get_value(reader, "Special Characters", "semicolon_in_value");
  check_value_equals(semicolon, "value");
  struct ptk_ini_value hash = ptk_ini_reader_get_value(reader, "Special Characters", "hash_in_value");
  check_value_equals(hash, "value");
  struct ptk_ini_value key_val = ptk_ini_reader_get_value(reader, "Malformed", "key");
  check_value_equals(key_val, "value");
  struct ptk_ini_value nonexistent1 = ptk_ini_reader_get_value(reader, "Empty Section", "nonexistent");
  TEST_ASSERT(nonexistent1.ptr == NULL);
  struct ptk_ini_value nonexistent2 = ptk_ini_reader_get_value(reader, "Malformed", "no_equals_line");
  TEST_ASSERT(nonexistent2.ptr == NULL);

  ptk_ini_reader_destroy(&reader);
}

static void test_empty_section(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/empty_section.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  struct ptk_ini_value empty_val = ptk_ini_reader_get_value(reader, "", "empty_section_key");
  check_value_equals(empty_val, "empty_value");
  struct ptk_ini_value normal_val = ptk_ini_reader_get_value(reader, "normal_section", "normal_key");
  check_value_equals(normal_val, "normal_value");
  struct ptk_ini_value empty_global = ptk_ini_reader_get_value(reader, "", "global_key");
  TEST_ASSERT(empty_global.ptr == NULL); // Should not exist in empty section
  struct ptk_ini_value global_empty = ptk_ini_reader_get_value(reader, NULL, "empty_section_key");
  TEST_ASSERT(global_empty.ptr == NULL); // Should not exist in global section

  ptk_ini_reader_destroy(&reader);
}

static void test_section_iteration(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/basic.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  size_t section_count = 0;
  bool found_global = false, found_section1 = false, found_section2 = false;

  struct ptk_ini_iter info = {.index = 0};
  while (ptk_ini_reader_iter_sections(reader, &info)) {
    section_count++;

    if (info.name == NULL) { // Global section
      found_global = true;
      struct ptk_ini_value global_val = ptk_ini_reader_get_value(reader, NULL, "global_key");
      check_value_equals(global_val, "global_value");
    } else if (info.name_len == 8 && strncmp(info.name, "section1", 8) == 0) {
      found_section1 = true;
      struct ptk_ini_value section_val = ptk_ini_reader_get_value(reader, "section1", "section_key");
      check_value_equals(section_val, "section_value");
    } else if (info.name_len == 8 && strncmp(info.name, "section2", 8) == 0) {
      found_section2 = true;
      struct ptk_ini_value section2_val = ptk_ini_reader_get_value(reader, "section2", "key1");
      check_value_equals(section2_val, "section2_value1");
    }
    TEST_CHECK(info.line_number > 0);
  }

  TEST_CHECK(section_count == 3);
  TEST_MSG("want 3, got %zu", section_count);

  TEST_CHECK(found_global);
  TEST_CHECK(found_section1);
  TEST_CHECK(found_section2);
  TEST_CHECK(!ptk_ini_reader_iter_sections(NULL, &info));
  TEST_CHECK(!ptk_ini_reader_iter_sections(reader, NULL));

  ptk_ini_reader_destroy(&reader);
}

static void test_entry_iteration(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/basic.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  size_t global_count = 0;
  bool found_global_key = false, found_key1 = false, found_key2 = false, found_key3 = false;

  struct ptk_ini_iter global_info = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries(reader, NULL, &global_info)) {
    global_count++;

    char key_buffer[256];
    strncpy(key_buffer, global_info.name, global_info.name_len);
    key_buffer[global_info.name_len] = '\0';

    struct ptk_ini_value val = ptk_ini_reader_get_value(reader, NULL, key_buffer);
    TEST_CHECK(val.ptr != NULL);

    if (global_info.name_len == 10 && strncmp(global_info.name, "global_key", 10) == 0) {
      found_global_key = true;
      check_value_equals(val, "global_value");
    } else if (global_info.name_len == 4 && strncmp(global_info.name, "key1", 4) == 0) {
      found_key1 = true;
      check_value_equals(val, "value1");
    } else if (global_info.name_len == 4 && strncmp(global_info.name, "key2", 4) == 0) {
      found_key2 = true;
      check_value_equals(val, "value2");
    } else if (global_info.name_len == 4 && strncmp(global_info.name, "key3", 4) == 0) {
      found_key3 = true;
      check_value_equals(val, "value3");
    }

    TEST_CHECK(global_info.line_number > 0);
  }

  TEST_CHECK(global_count == 4);
  TEST_MSG("want 4, got %zu", global_count);

  TEST_CHECK(found_global_key);
  TEST_CHECK(found_key1);
  TEST_CHECK(found_key2);
  TEST_CHECK(found_key3);

  size_t section1_count = 0;
  bool found_section_key = false, found_another_key = false;

  struct ptk_ini_iter section1_info = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries(reader, "section1", &section1_info)) {
    section1_count++;

    char key_buffer[256];
    strncpy(key_buffer, section1_info.name, section1_info.name_len);
    key_buffer[section1_info.name_len] = '\0';

    struct ptk_ini_value val = ptk_ini_reader_get_value(reader, "section1", key_buffer);
    TEST_CHECK(val.ptr != NULL);

    if (section1_info.name_len == 11 && strncmp(section1_info.name, "section_key", 11) == 0) {
      found_section_key = true;
      check_value_equals(val, "section_value");
    } else if (section1_info.name_len == 11 && strncmp(section1_info.name, "another_key", 11) == 0) {
      found_another_key = true;
      check_value_equals(val, "another_value");
    }

    TEST_CHECK(section1_info.line_number > 0);
  }

  TEST_CHECK(section1_count == 2);
  TEST_MSG("want 2, got %zu", section1_count);

  TEST_CHECK(found_section_key);
  TEST_CHECK(found_another_key);

  size_t section2_count = 0;
  struct ptk_ini_iter section2_info = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries(reader, "section2", &section2_info)) {
    section2_count++;

    char key_buffer[256];
    strncpy(key_buffer, section2_info.name, section2_info.name_len);
    key_buffer[section2_info.name_len] = '\0';

    struct ptk_ini_value val = ptk_ini_reader_get_value(reader, "section2", key_buffer);
    TEST_CHECK(val.ptr != NULL);

    if (section2_info.name_len == 4 && strncmp(section2_info.name, "key1", 4) == 0) {
      check_value_equals(val, "section2_value1");
    }

    TEST_CHECK(section2_info.line_number > 0);
  }

  TEST_CHECK(section2_count == 1);
  TEST_MSG("want 1, got %zu", section2_count);

  size_t nonexistent_count = 0;
  struct ptk_ini_iter nonexistent_info = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries(reader, "nonexistent", &nonexistent_info)) {
    nonexistent_count++;
  }

  TEST_CHECK(nonexistent_count == 0);
  TEST_MSG("want 0, got %zu", nonexistent_count);

  struct ptk_ini_iter null_test_info = {.index = 0, .state = NULL};
  TEST_CHECK(!ptk_ini_reader_iter_entries(NULL, "section1", &null_test_info));
  TEST_CHECK(!ptk_ini_reader_iter_entries(reader, "section1", NULL));

  ptk_ini_reader_destroy(&reader);
}

static void test_empty_section_iteration(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/empty_section.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  size_t section_count = 0;
  bool found_global = false, found_empty = false;

  struct ptk_ini_iter info = {.index = 0};
  while (ptk_ini_reader_iter_sections(reader, &info)) {
    section_count++;

    if (info.name == NULL) { // Global section
      found_global = true;
      struct ptk_ini_value global_val = ptk_ini_reader_get_value(reader, NULL, "global_key");
      check_value_equals(global_val, "global_value");
    } else if (info.name_len == 0 && info.name != NULL && info.name[0] == '\0') { // Empty section []
      found_empty = true;
      struct ptk_ini_value empty_val = ptk_ini_reader_get_value(reader, "", "empty_section_key");
      check_value_equals(empty_val, "empty_value");
    }
  }

  TEST_CHECK(found_global);
  TEST_CHECK(found_empty);
  TEST_CHECK(section_count >= 2);
  TEST_MSG("want at least 2, got %zu", section_count);

  ptk_ini_reader_destroy(&reader);
}

static void test_get_value_n(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/basic.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  // Test with explicit lengths
  struct ptk_ini_value val1 = ptk_ini_reader_get_value_n(reader, "section1", 8, "section_key", 11);
  check_value_equals(val1, "section_value");

  // Test with non-null-terminated strings (using longer buffer)
  char const section_buf[] = "section1_extra_stuff";
  char const key_buf[] = "section_key_extra_stuff";
  struct ptk_ini_value val2 = ptk_ini_reader_get_value_n(reader, section_buf, 8, key_buf, 11);
  check_value_equals(val2, "section_value");

  // Test global section (NULL section)
  struct ptk_ini_value val3 = ptk_ini_reader_get_value_n(reader, NULL, 0, "global_key", 10);
  check_value_equals(val3, "global_value");

  // Test empty section
  struct ptk_ini_value val4 = ptk_ini_reader_get_value_n(reader, "", 0, "nonexistent", 11);
  TEST_CHECK(val4.ptr == NULL);

  // Test invalid arguments
  struct ptk_ini_value val5 = ptk_ini_reader_get_value_n(NULL, "section1", 8, "key", 3);
  TEST_CHECK(val5.ptr == NULL);

  struct ptk_ini_value val6 = ptk_ini_reader_get_value_n(reader, "section1", 8, NULL, 0);
  TEST_CHECK(val6.ptr == NULL);

  struct ptk_ini_value val7 = ptk_ini_reader_get_value_n(reader, "section1", 8, "key", 0);
  TEST_CHECK(val7.ptr == NULL);

  ptk_ini_reader_destroy(&reader);
}

static void test_iter_entries_n(void) {
  struct ptk_ini_reader *reader = NULL;
  struct ov_error err = {0};
  if (!TEST_SUCCEEDED(ptk_ini_reader_create(&reader, &err), &err)) {
    return;
  }

  if (!TEST_SUCCEEDED(ptk_ini_reader_load_file(reader, TEST_PATH(L"ini_reader/basic.ini"), &err), &err)) {
    ptk_ini_reader_destroy(&reader);
    return;
  }

  // Test iteration with explicit length for section name
  size_t section1_count = 0;
  bool found_section_key = false, found_another_key = false;

  struct ptk_ini_iter iter = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries_n(reader, "section1", 8, &iter)) {
    section1_count++;

    if (iter.name_len == 11 && strncmp(iter.name, "section_key", 11) == 0) {
      found_section_key = true;
    } else if (iter.name_len == 11 && strncmp(iter.name, "another_key", 11) == 0) {
      found_another_key = true;
    }

    TEST_CHECK(iter.line_number > 0);
  }

  TEST_CHECK(section1_count == 2);
  TEST_MSG("want 2, got %zu", section1_count);
  TEST_CHECK(found_section_key);
  TEST_CHECK(found_another_key);

  // Test with non-null-terminated section name (using longer buffer)
  char const section_buf[] = "section1_extra_stuff";
  size_t section1_count2 = 0;

  struct ptk_ini_iter iter2 = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries_n(reader, section_buf, 8, &iter2)) {
    section1_count2++;
  }

  TEST_CHECK(section1_count2 == 2);
  TEST_MSG("want 2, got %zu", section1_count2);

  // Test global section (NULL section)
  size_t global_count = 0;
  struct ptk_ini_iter iter3 = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries_n(reader, NULL, 0, &iter3)) {
    global_count++;
  }

  TEST_CHECK(global_count == 4);
  TEST_MSG("want 4, got %zu", global_count);

  // Test nonexistent section
  size_t nonexistent_count = 0;
  struct ptk_ini_iter iter4 = {.index = 0, .state = NULL};
  while (ptk_ini_reader_iter_entries_n(reader, "nonexistent", 11, &iter4)) {
    nonexistent_count++;
  }

  TEST_CHECK(nonexistent_count == 0);
  TEST_MSG("want 0, got %zu", nonexistent_count);

  // Test invalid arguments
  struct ptk_ini_iter iter5 = {.index = 0, .state = NULL};
  TEST_CHECK(!ptk_ini_reader_iter_entries_n(NULL, "section1", 8, &iter5));
  TEST_CHECK(!ptk_ini_reader_iter_entries_n(reader, "section1", 8, NULL));

  ptk_ini_reader_destroy(&reader);
}

TEST_LIST = {
    {"create_destroy", test_create_destroy},
    {"key_value_operations", test_key_value_operations},
    {"basic_parsing", test_basic_parsing},
    {"utf8_bom", test_utf8_bom},
    {"edge_cases", test_edge_cases},
    {"complex_ini", test_complex_ini},
    {"empty_section", test_empty_section},
    {"section_iteration", test_section_iteration},
    {"entry_iteration", test_entry_iteration},
    {"empty_section_iteration", test_empty_section_iteration},
    {"get_value_n", test_get_value_n},
    {"iter_entries_n", test_iter_entries_n},
    {NULL, NULL},
};
