// Test file for alias.c
//
// Tests the script enumeration and parameter extraction functionality.

#include "alias.h"
#include "ini_reader.h"

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

static void test_enumerate_available_scripts(void) {
  struct ov_error err = {0};
  struct ptk_alias_script_definitions defs = {0};
  struct ptk_alias_available_scripts scripts = {0};
  char *alias_data = NULL;
  size_t alias_len = 0;
  size_t count = 0;
  bool found_blinker = false;
  bool found_lipsync = false;
  bool found_lipsync_lab = false;

  // Read real object alias data
  {
    FILE *f = fopen(TEST_PATH("alias/realdata.object"), "rb");
    if (!TEST_CHECK(f != NULL)) {
      TEST_MSG("Failed to open realdata.object");
      return;
    }

    fseek(f, 0, SEEK_END);
    long const size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (!OV_REALLOC(&alias_data, (size_t)size + 1, sizeof(char))) {
      fclose(f);
      return;
    }
    alias_len = fread(alias_data, 1, (size_t)size, f);
    alias_data[alias_len] = '\0';
    fclose(f);
  }

  // Manually create script definitions (simulating INI load)
  {
    if (!OV_ARRAY_GROW(&defs.items, 3)) {
      OV_FREE(&alias_data);
      TEST_CHECK(false);
      TEST_MSG("Failed to allocate definitions");
      return;
    }

    // Blinker
    defs.items[0].script_name = NULL;
    defs.items[0].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[0].script_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[0].script_name, "PSDToolKit.Blinker");
    if (!OV_REALLOC(&defs.items[0].effect_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[0].effect_name, "目パチ@PSDToolKit");

    // LipSync
    defs.items[1].script_name = NULL;
    defs.items[1].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[1].script_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[1].script_name, "PSDToolKit.LipSync");
    if (!OV_REALLOC(&defs.items[1].effect_name, 40, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[1].effect_name, "口パク 開閉のみ@PSDToolKit");

    // LipSyncLab
    defs.items[2].script_name = NULL;
    defs.items[2].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[2].script_name, 22, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[2].script_name, "PSDToolKit.LipSyncLab");
    if (!OV_REALLOC(&defs.items[2].effect_name, 40, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[2].effect_name, "口パク あいうえお@PSDToolKit");

    OV_ARRAY_SET_LENGTH(defs.items, 3);
  }

  // Enumerate available scripts
  if (!TEST_SUCCEEDED(ptk_alias_enumerate_available_scripts(alias_data, alias_len, &defs, &scripts, &err), &err)) {
    goto cleanup;
  }

  // Should find all 3 scripts
  count = OV_ARRAY_LENGTH(scripts.items);
  TEST_CHECK(count == 3);
  TEST_MSG("want 3, got %zu", count);

  // All should be selected by default
  for (size_t i = 0; i < count; i++) {
    TEST_CHECK(scripts.items[i].selected);
    TEST_MSG("Script %zu should be selected", i);
  }

  // Check script names
  for (size_t i = 0; i < count; i++) {
    if (strcmp(scripts.items[i].script_name, "PSDToolKit.Blinker") == 0) {
      found_blinker = true;
    } else if (strcmp(scripts.items[i].script_name, "PSDToolKit.LipSync") == 0) {
      found_lipsync = true;
    } else if (strcmp(scripts.items[i].script_name, "PSDToolKit.LipSyncLab") == 0) {
      found_lipsync_lab = true;
    }
  }
  TEST_CHECK(found_blinker);
  TEST_MSG("Blinker not found");
  TEST_CHECK(found_lipsync);
  TEST_MSG("LipSync not found");
  TEST_CHECK(found_lipsync_lab);
  TEST_MSG("LipSyncLab not found");

  // Check PSD path extracted
  TEST_CHECK(scripts.psd_path != NULL);
  if (scripts.psd_path) {
    TEST_CHECK(strstr(scripts.psd_path, ".psd") != NULL);
    TEST_MSG("PSD path should contain .psd: %s", scripts.psd_path);
  }

cleanup:
  ptk_alias_available_scripts_free(&scripts);
  ptk_alias_script_definitions_free(&defs);
  if (alias_data) {
    OV_FREE(&alias_data);
  }
}

static void test_enumerate_available_scripts_partial(void) {
  struct ov_error err = {0};
  struct ptk_alias_script_definitions defs = {0};
  struct ptk_alias_available_scripts scripts = {0};
  size_t count = 0;

  // Alias with only Blinker effect
  char const alias[] = "[Object.0]\n"
                       "effect.name=PSDファイル@PSDToolKit\n"
                       "PSDファイル=C:\\test\\sample.psd|sample.pfv\n"
                       "[Object.1]\n"
                       "effect.name=目パチ@PSDToolKit\n"
                       "開き~ptkl=v1.!目/*通常\n"
                       "閉じ~ptkl=v1.!目/*つぶり\n";
  size_t const alias_len = sizeof(alias) - 1;

  // Define 3 scripts, but only 1 exists in alias
  {
    if (!OV_ARRAY_GROW(&defs.items, 3)) {
      TEST_CHECK(false);
      return;
    }

    defs.items[0].script_name = NULL;
    defs.items[0].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[0].script_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[0].script_name, "PSDToolKit.Blinker");
    if (!OV_REALLOC(&defs.items[0].effect_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[0].effect_name, "目パチ@PSDToolKit");

    defs.items[1].script_name = NULL;
    defs.items[1].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[1].script_name, 20, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[1].script_name, "PSDToolKit.LipSync");
    if (!OV_REALLOC(&defs.items[1].effect_name, 40, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[1].effect_name, "口パク 開閉のみ@PSDToolKit");

    defs.items[2].script_name = NULL;
    defs.items[2].effect_name = NULL;
    if (!OV_REALLOC(&defs.items[2].script_name, 22, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[2].script_name, "PSDToolKit.LipSyncLab");
    if (!OV_REALLOC(&defs.items[2].effect_name, 40, sizeof(char))) {
      goto cleanup;
    }
    strcpy(defs.items[2].effect_name, "口パク あいうえお@PSDToolKit");

    OV_ARRAY_SET_LENGTH(defs.items, 3);
  }

  if (!TEST_SUCCEEDED(ptk_alias_enumerate_available_scripts(alias, alias_len, &defs, &scripts, &err), &err)) {
    goto cleanup;
  }

  // Should find only Blinker
  count = OV_ARRAY_LENGTH(scripts.items);
  TEST_CHECK(count == 1);
  TEST_MSG("want 1, got %zu", count);

  if (count >= 1) {
    TEST_CHECK(strcmp(scripts.items[0].script_name, "PSDToolKit.Blinker") == 0);
    TEST_MSG("want PSDToolKit.Blinker, got %s", scripts.items[0].script_name);
  }

  // Check PSD path (should include pfv info)
  TEST_CHECK(scripts.psd_path != NULL);
  if (scripts.psd_path) {
    TEST_CHECK(strcmp(scripts.psd_path, "C:\\test\\sample.psd|sample.pfv") == 0);
    TEST_MSG("want C:\\test\\sample.psd|sample.pfv, got %s", scripts.psd_path);
  }

cleanup:
  ptk_alias_available_scripts_free(&scripts);
  ptk_alias_script_definitions_free(&defs);
}

static void test_extract_animation_from_alias_blinker(void) {
  struct ov_error err = {0};
  struct ptk_alias_extracted_animation anim = {0};

  // Read real object alias data
  char *alias_data = NULL;
  size_t alias_len = 0;
  {
    FILE *f = fopen(TEST_PATH("alias/realdata.object"), "rb");
    if (!TEST_CHECK(f != NULL)) {
      TEST_MSG("Failed to open realdata.object");
      return;
    }

    fseek(f, 0, SEEK_END);
    long const size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (!OV_REALLOC(&alias_data, (size_t)size + 1, sizeof(char))) {
      fclose(f);
      return;
    }
    alias_len = fread(alias_data, 1, (size_t)size, f);
    alias_data[alias_len] = '\0';
    fclose(f);
  }

  if (!TEST_SUCCEEDED(
          ptk_alias_extract_animation(alias_data, alias_len, "PSDToolKit.Blinker", "目パチ@PSDToolKit", &anim, &err),
          &err)) {
    OV_FREE(&alias_data);
    return;
  }

  TEST_CHECK(anim.script_name != NULL);
  TEST_CHECK(strcmp(anim.script_name, "PSDToolKit.Blinker") == 0);
  TEST_MSG("want PSDToolKit.Blinker, got %s", anim.script_name ? anim.script_name : "(null)");

  TEST_CHECK(anim.effect_name != NULL);
  TEST_CHECK(strcmp(anim.effect_name, "目パチ@PSDToolKit") == 0);
  TEST_MSG("want 目パチ@PSDToolKit, got %s", anim.effect_name ? anim.effect_name : "(null)");

  size_t const param_count = OV_ARRAY_LENGTH(anim.params);
  TEST_CHECK(param_count > 0);
  TEST_MSG("Expected params, got %zu", param_count);

  // Check for expected keys
  bool found_open = false, found_close = false, found_interval = false;
  for (size_t i = 0; i < param_count; i++) {
    if (strcmp(anim.params[i].key, "開き~ptkl") == 0) {
      found_open = true;
      TEST_CHECK(strcmp(anim.params[i].value, "v1.!目/*通常") == 0);
    } else if (strcmp(anim.params[i].key, "閉じ~ptkl") == 0) {
      found_close = true;
      TEST_CHECK(strcmp(anim.params[i].value, "v1.!目/*つぶり") == 0);
    } else if (strstr(anim.params[i].key, "間隔") != NULL) {
      found_interval = true;
    }
  }
  TEST_CHECK(found_open);
  TEST_MSG("開き~ptkl not found");
  TEST_CHECK(found_close);
  TEST_MSG("閉じ~ptkl not found");
  TEST_CHECK(found_interval);
  TEST_MSG("間隔(秒) not found");

  // Check that effect.name is NOT included in params
  for (size_t i = 0; i < param_count; i++) {
    TEST_CHECK(strcmp(anim.params[i].key, "effect.name") != 0);
  }

  ptk_alias_extracted_animation_free(&anim);
  OV_FREE(&alias_data);
}

static void test_extract_animation_from_alias_lipsync_lab(void) {
  struct ov_error err = {0};
  struct ptk_alias_extracted_animation anim = {0};

  // Read real object alias data
  char *alias_data = NULL;
  size_t alias_len = 0;
  {
    FILE *f = fopen(TEST_PATH("alias/realdata.object"), "rb");
    if (!TEST_CHECK(f != NULL)) {
      TEST_MSG("Failed to open realdata.object");
      return;
    }

    fseek(f, 0, SEEK_END);
    long const size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (!OV_REALLOC(&alias_data, (size_t)size + 1, sizeof(char))) {
      fclose(f);
      return;
    }
    alias_len = fread(alias_data, 1, (size_t)size, f);
    alias_data[alias_len] = '\0';
    fclose(f);
  }

  if (!TEST_SUCCEEDED(ptk_alias_extract_animation(
                          alias_data, alias_len, "PSDToolKit.LipSyncLab", "口パク あいうえお@PSDToolKit", &anim, &err),
                      &err)) {
    OV_FREE(&alias_data);
    return;
  }

  size_t const param_count = OV_ARRAY_LENGTH(anim.params);
  TEST_CHECK(param_count > 0);
  TEST_MSG("Expected params, got %zu", param_count);

  // Check for LipSyncLab-specific keys
  bool found_a = false, found_o = false, found_n = false;
  for (size_t i = 0; i < param_count; i++) {
    if (strcmp(anim.params[i].key, "あ~ptkl") == 0) {
      found_a = true;
    } else if (strcmp(anim.params[i].key, "お~ptkl") == 0) {
      found_o = true;
    } else if (strcmp(anim.params[i].key, "ん~ptkl") == 0) {
      found_n = true;
    }
  }
  TEST_CHECK(found_a);
  TEST_MSG("あ~ptkl not found");
  TEST_CHECK(found_o);
  TEST_MSG("お~ptkl not found");
  TEST_CHECK(found_n);
  TEST_MSG("ん~ptkl not found");

  ptk_alias_extracted_animation_free(&anim);
  OV_FREE(&alias_data);
}

static void test_extract_animation_not_found(void) {
  struct ov_error err = {0};
  struct ptk_alias_extracted_animation anim = {0};

  char const alias[] = "[Object.0]\n"
                       "effect.name=SomeOtherEffect\n";

  bool const result =
      ptk_alias_extract_animation(alias, sizeof(alias) - 1, "PSDToolKit.Blinker", "目パチ@PSDToolKit", &anim, &err);

  TEST_CHECK(!result);
  TEST_MSG("Should fail when effect not found");

  OV_ERROR_DESTROY(&err);
  ptk_alias_extracted_animation_free(&anim);
}

TEST_LIST = {
    {"enumerate_available_scripts", test_enumerate_available_scripts},
    {"enumerate_available_scripts_partial", test_enumerate_available_scripts_partial},
    {"extract_animation_from_alias_blinker", test_extract_animation_from_alias_blinker},
    {"extract_animation_from_alias_lipsync_lab", test_extract_animation_from_alias_lipsync_lab},
    {"extract_animation_not_found", test_extract_animation_not_found},
    {NULL, NULL},
};
