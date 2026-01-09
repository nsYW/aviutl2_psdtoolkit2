// PSDToolKit image cache system
#include "cache.h"

#include <ovarray.h>
#include <ovhashmap.h>
#include <ovprintf.h>
#include <ovutf.h>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string.h>

#include "logf.h"

enum {
  CACHEKEY_HEX_LEN = 16,
  MEMORY_CACHE_LIMIT = 256 * 1024 * 1024, // 256MB
  FILE_CACHE_LIMIT = 256 * 1024 * 1024,   // 256MB
};

// Convert uint64 cache key to 16-character hex string
static void ckey_to_hex(uint64_t ckey, char hex[CACHEKEY_HEX_LEN + 1]) {
  static char const hexchars[] = "0123456789abcdef";
  for (int i = 15; i >= 0; --i) {
    hex[i] = hexchars[ckey & 0xf];
    ckey >>= 4;
  }
  hex[CACHEKEY_HEX_LEN] = '\0';
}

struct cache_entry {
  char cachekey_hex[CACHEKEY_HEX_LEN + 1]; // key for hashmap
  int32_t width;
  int32_t height;
  uint8_t *data;    // BGRA pixel data (memory tier only)
  size_t data_size; // width * height * 4
  bool in_file;     // true if data is in file tier
  // LRU doubly-linked list
  struct cache_entry *lru_prev;
  struct cache_entry *lru_next;
};

struct ptk_cache {
  wchar_t *temp_dir;            // %TEMP%/ptk_{pid}_{id}/ (null-terminated, OV_ARRAY)
  HANDLE dir_lock;              // directory lock handle
  struct ov_hashmap *entries;   // cachekey_hex -> cache_entry* (pointer to heap-allocated entry)
  struct cache_entry *lru_head; // oldest (first to evict)
  struct cache_entry *lru_tail; // newest
  size_t memory_used;
  size_t file_used;
};

// Instance counter for unique directory names
static LONG g_instance_counter = 0;

static void get_entry_key(void const *const item, void const **const key, size_t *const key_bytes) {
  // item is a pointer to cache_entry* (i.e., cache_entry**)
  struct cache_entry *const *const entry_ptr = (struct cache_entry *const *)item;
  *key = (*entry_ptr)->cachekey_hex;
  *key_bytes = CACHEKEY_HEX_LEN;
}

// Move entry to tail of LRU list (most recently used)
static void lru_touch(struct ptk_cache *const c, struct cache_entry *entry) {
  if (entry == c->lru_tail) {
    return; // already at tail
  }
  // Remove from current position
  if (entry->lru_prev) {
    entry->lru_prev->lru_next = entry->lru_next;
  } else {
    c->lru_head = entry->lru_next;
  }
  if (entry->lru_next) {
    entry->lru_next->lru_prev = entry->lru_prev;
  }
  // Add to tail
  entry->lru_prev = c->lru_tail;
  entry->lru_next = NULL;
  if (c->lru_tail) {
    c->lru_tail->lru_next = entry;
  }
  c->lru_tail = entry;
  if (!c->lru_head) {
    c->lru_head = entry;
  }
}

// Remove entry from LRU list
static void lru_remove(struct ptk_cache *const c, struct cache_entry *entry) {
  if (entry->lru_prev) {
    entry->lru_prev->lru_next = entry->lru_next;
  } else {
    c->lru_head = entry->lru_next;
  }
  if (entry->lru_next) {
    entry->lru_next->lru_prev = entry->lru_prev;
  } else {
    c->lru_tail = entry->lru_prev;
  }
  entry->lru_prev = NULL;
  entry->lru_next = NULL;
}

// Add entry to tail of LRU list
static void lru_add(struct ptk_cache *const c, struct cache_entry *entry) {
  entry->lru_prev = c->lru_tail;
  entry->lru_next = NULL;
  if (c->lru_tail) {
    c->lru_tail->lru_next = entry;
  }
  c->lru_tail = entry;
  if (!c->lru_head) {
    c->lru_head = entry;
  }
}

// Build file path for cache entry
static bool build_cache_file_path(struct ptk_cache const *const c,
                                  wchar_t **path,
                                  char const *cachekey_hex,
                                  struct ov_error *const err) {
  size_t const dir_len = OV_ARRAY_LENGTH(c->temp_dir);
  size_t const filename_len = CACHEKEY_HEX_LEN + 4; // "xxxx.bin"
  bool result = false;

  if (!OV_ARRAY_GROW(path, dir_len + filename_len + 1)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  memcpy(*path, c->temp_dir, dir_len * sizeof(wchar_t));
  for (size_t i = 0; i < CACHEKEY_HEX_LEN; ++i) {
    (*path)[dir_len + i] = (wchar_t)cachekey_hex[i];
  }
  (*path)[dir_len + CACHEKEY_HEX_LEN + 0] = L'.';
  (*path)[dir_len + CACHEKEY_HEX_LEN + 1] = L'b';
  (*path)[dir_len + CACHEKEY_HEX_LEN + 2] = L'i';
  (*path)[dir_len + CACHEKEY_HEX_LEN + 3] = L'n';
  (*path)[dir_len + CACHEKEY_HEX_LEN + 4] = L'\0';
  OV_ARRAY_SET_LENGTH(*path, dir_len + filename_len);
  result = true;

cleanup:
  return result;
}

// Write entry data to file
static bool
write_entry_to_file(struct ptk_cache const *const c, struct cache_entry *entry, struct ov_error *const err) {
  wchar_t *path = NULL;
  HANDLE file = INVALID_HANDLE_VALUE;
  DWORD written = 0;
  size_t data_size = 0;
  bool result = false;

  if (!build_cache_file_path(c, &path, entry->cachekey_hex, err)) {
    OV_ERROR_ADD_TRACE(err);
    goto cleanup;
  }

  file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  // Write header: width, height
  if (!WriteFile(file, &entry->width, sizeof(entry->width), &written, NULL) || written != sizeof(entry->width)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }
  if (!WriteFile(file, &entry->height, sizeof(entry->height), &written, NULL) || written != sizeof(entry->height)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  // Write pixel data
  data_size = entry->data_size;
  if (!WriteFile(file, entry->data, (DWORD)data_size, &written, NULL) || written != data_size) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  result = true;

cleanup:
  if (file != INVALID_HANDLE_VALUE) {
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
  }
  if (path) {
    OV_ARRAY_DESTROY(&path);
  }
  return result;
}

// Read entry data from file
static bool
read_entry_from_file(struct ptk_cache const *const c, struct cache_entry *entry, struct ov_error *const err) {
  wchar_t *path = NULL;
  HANDLE file = INVALID_HANDLE_VALUE;
  DWORD bytes_read = 0;
  int32_t width = 0;
  int32_t height = 0;
  size_t data_size = 0;
  bool result = false;

  if (!build_cache_file_path(c, &path, entry->cachekey_hex, err)) {
    OV_ERROR_ADD_TRACE(err);
    goto cleanup;
  }

  file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file == INVALID_HANDLE_VALUE) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  // Read header
  if (!ReadFile(file, &width, sizeof(width), &bytes_read, NULL) || bytes_read != sizeof(width)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }
  if (!ReadFile(file, &height, sizeof(height), &bytes_read, NULL) || bytes_read != sizeof(height)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  // Validate dimensions
  if (width != entry->width || height != entry->height) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_fail);
    goto cleanup;
  }

  // Allocate and read pixel data
  data_size = (size_t)width * (size_t)height * 4;
  if (!OV_REALLOC(&entry->data, data_size, 1)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  if (!ReadFile(file, entry->data, (DWORD)data_size, &bytes_read, NULL) || bytes_read != data_size) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }
  entry->data_size = data_size;

  result = true;

cleanup:
  if (file != INVALID_HANDLE_VALUE) {
    CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
  }
  if (path) {
    OV_ARRAY_DESTROY(&path);
  }
  return result;
}

// Delete cache file for entry
static void delete_entry_file(struct ptk_cache const *const c, struct cache_entry const *entry) {
  wchar_t *path = NULL;
  struct ov_error err = {0};

  if (!build_cache_file_path(c, &path, entry->cachekey_hex, &err)) {
    OV_ERROR_REPORT(&err, NULL);
    goto cleanup;
  }

  DeleteFileW(path);

cleanup:
  if (path) {
    OV_ARRAY_DESTROY(&path);
  }
}

// Evict entries from memory to file tier
static bool evict_memory_to_file(struct ptk_cache *const c, struct ov_error *const err) {
  bool result = false;

  while (c->memory_used > MEMORY_CACHE_LIMIT && c->lru_head) {
    // Find oldest entry in memory
    struct cache_entry *entry = c->lru_head;
    while (entry && entry->in_file) {
      entry = entry->lru_next;
    }
    if (!entry) {
      break; // No more memory entries
    }

    // Write to file
    if (!write_entry_to_file(c, entry, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }

    // Free memory, mark as file-based
    c->memory_used -= entry->data_size;
    c->file_used += entry->data_size;
    OV_FREE(&entry->data);
    entry->in_file = true;
  }
  result = true;

cleanup:
  return result;
}

// Evict entries from file tier (delete)
static void evict_file_tier(struct ptk_cache *const c) {
  while (c->file_used > FILE_CACHE_LIMIT && c->lru_head) {
    // Find oldest entry in file tier
    struct cache_entry *entry = c->lru_head;
    while (entry && !entry->in_file) {
      entry = entry->lru_next;
    }
    if (!entry) {
      break;
    }

    // Delete file and remove from cache
    delete_entry_file(c, entry);
    c->file_used -= entry->data_size;
    lru_remove(c, entry);
    struct cache_entry *entry_for_delete = entry;
    OV_HASHMAP_DELETE(c->entries, &entry_for_delete);
    OV_FREE(&entry);
  }
}

// Try to lock a directory (for orphan detection)
static HANDLE try_lock_directory(wchar_t const *dir_path) {
  return CreateFileW(dir_path,
                     GENERIC_READ,
                     0, // no sharing = exclusive
                     NULL,
                     OPEN_EXISTING,
                     FILE_FLAG_BACKUP_SEMANTICS,
                     NULL);
}

// Delete directory recursively
static void delete_directory_recursive(wchar_t const *dir_path) {
  wchar_t *pattern = NULL;
  WIN32_FIND_DATAW find_data = {0};
  HANDLE find_handle = INVALID_HANDLE_VALUE;
  wchar_t *file_path = NULL;

  size_t const dir_len = wcslen(dir_path);

  // Build search pattern: dir_path/*
  if (!OV_ARRAY_GROW(&pattern, dir_len + 3)) {
    goto cleanup;
  }
  memcpy(pattern, dir_path, dir_len * sizeof(wchar_t));
  pattern[dir_len] = L'\\';
  pattern[dir_len + 1] = L'*';
  pattern[dir_len + 2] = L'\0';
  OV_ARRAY_SET_LENGTH(pattern, dir_len + 2);

  find_handle = FindFirstFileW(pattern, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    goto cleanup;
  }

  do {
    if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0) {
      continue;
    }

    size_t const name_len = wcslen(find_data.cFileName);
    if (!OV_ARRAY_GROW(&file_path, dir_len + 1 + name_len + 1)) {
      goto cleanup;
    }
    memcpy(file_path, dir_path, dir_len * sizeof(wchar_t));
    file_path[dir_len] = L'\\';
    memcpy(file_path + dir_len + 1, find_data.cFileName, (name_len + 1) * sizeof(wchar_t));
    OV_ARRAY_SET_LENGTH(file_path, dir_len + 1 + name_len);

    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      delete_directory_recursive(file_path);
    } else {
      DeleteFileW(file_path);
    }
  } while (FindNextFileW(find_handle, &find_data));

cleanup:
  if (find_handle != INVALID_HANDLE_VALUE) {
    FindClose(find_handle);
    find_handle = INVALID_HANDLE_VALUE;
  }
  if (pattern) {
    OV_ARRAY_DESTROY(&pattern);
  }
  if (file_path) {
    OV_ARRAY_DESTROY(&file_path);
  }

  RemoveDirectoryW(dir_path);
}

// Cleanup orphaned cache directories from crashed processes
static void cleanup_orphaned_directories(void) {
  wchar_t *temp_path = NULL;
  wchar_t *pattern = NULL;
  WIN32_FIND_DATAW find_data = {0};
  HANDLE find_handle = INVALID_HANDLE_VALUE;
  wchar_t *dir_path = NULL;
  size_t actual_temp_len = 0;

  // Get temp directory
  DWORD const temp_len = GetTempPathW(0, NULL);
  if (temp_len == 0) {
    goto cleanup;
  }
  if (!OV_ARRAY_GROW(&temp_path, temp_len)) {
    goto cleanup;
  }
  GetTempPathW(temp_len, temp_path);
  OV_ARRAY_SET_LENGTH(temp_path, temp_len - 1); // exclude trailing null

  // Build search pattern: temp_path + ptk_*
  actual_temp_len = wcslen(temp_path);
  if (!OV_ARRAY_GROW(&pattern, actual_temp_len + 6)) {
    goto cleanup;
  }
  memcpy(pattern, temp_path, actual_temp_len * sizeof(wchar_t));
  pattern[actual_temp_len + 0] = L'p';
  pattern[actual_temp_len + 1] = L't';
  pattern[actual_temp_len + 2] = L'k';
  pattern[actual_temp_len + 3] = L'_';
  pattern[actual_temp_len + 4] = L'*';
  pattern[actual_temp_len + 5] = L'\0';
  OV_ARRAY_SET_LENGTH(pattern, actual_temp_len + 5);

  find_handle = FindFirstFileW(pattern, &find_data);
  if (find_handle == INVALID_HANDLE_VALUE) {
    goto cleanup;
  }

  do {
    if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      continue;
    }
    if (wcscmp(find_data.cFileName, L".") == 0 || wcscmp(find_data.cFileName, L"..") == 0) {
      continue;
    }

    // Build full path
    size_t const name_len = wcslen(find_data.cFileName);
    if (!OV_ARRAY_GROW(&dir_path, actual_temp_len + 1 + name_len + 1)) {
      continue;
    }
    memcpy(dir_path, temp_path, actual_temp_len * sizeof(wchar_t));
    dir_path[actual_temp_len] = L'\\';
    memcpy(dir_path + actual_temp_len + 1, find_data.cFileName, (name_len + 1) * sizeof(wchar_t));
    OV_ARRAY_SET_LENGTH(dir_path, actual_temp_len + 1 + name_len);

    // Try to lock - if successful, it's orphaned
    HANDLE lock = try_lock_directory(dir_path);
    if (lock != INVALID_HANDLE_VALUE) {
      CloseHandle(lock);
      lock = INVALID_HANDLE_VALUE;
      delete_directory_recursive(dir_path);
    }
  } while (FindNextFileW(find_handle, &find_data));

cleanup:
  if (find_handle != INVALID_HANDLE_VALUE) {
    FindClose(find_handle);
    find_handle = INVALID_HANDLE_VALUE;
  }
  if (temp_path) {
    OV_ARRAY_DESTROY(&temp_path);
  }
  if (pattern) {
    OV_ARRAY_DESTROY(&pattern);
  }
  if (dir_path) {
    OV_ARRAY_DESTROY(&dir_path);
  }
}

struct ptk_cache *ptk_cache_create(struct ov_error *const err) {
  struct ptk_cache *cache = NULL;

  // Cleanup orphaned directories first
  cleanup_orphaned_directories();

  // Allocate cache structure
  if (!OV_REALLOC(&cache, 1, sizeof(struct ptk_cache))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  *cache = (struct ptk_cache){
      .dir_lock = INVALID_HANDLE_VALUE,
  };

  // Get temp path
  {
    DWORD const temp_len = GetTempPathW(0, NULL);
    if (temp_len == 0) {
      OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
      goto cleanup;
    }

    wchar_t *temp_path = NULL;
    if (!OV_ARRAY_GROW(&temp_path, temp_len)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }
    GetTempPathW(temp_len, temp_path);
    size_t const actual_temp_len = wcslen(temp_path);
    OV_ARRAY_SET_LENGTH(temp_path, actual_temp_len);

    // Build directory name: ptk_{pid}_{instance}/
    DWORD const pid = GetCurrentProcessId();
    LONG const instance_id = InterlockedIncrement(&g_instance_counter);
    wchar_t suffix_str[32] = {0};
    int const suffix_len = ov_snprintf_wchar(
        suffix_str, sizeof(suffix_str) / sizeof(suffix_str[0]), NULL, L"%1$lu_%2$ld", pid, instance_id);
    if (suffix_len < 0) {
      OV_ARRAY_DESTROY(&temp_path);
      OV_ERROR_SET_GENERIC(err, ov_error_generic_fail);
      goto cleanup;
    }

    // temp_path + "ptk_" + suffix + "\"
    size_t const dir_len = actual_temp_len + 4 + (size_t)suffix_len + 1;
    if (!OV_ARRAY_GROW(&cache->temp_dir, dir_len + 1)) {
      OV_ARRAY_DESTROY(&temp_path);
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }
    memcpy(cache->temp_dir, temp_path, actual_temp_len * sizeof(wchar_t));
    cache->temp_dir[actual_temp_len + 0] = L'p';
    cache->temp_dir[actual_temp_len + 1] = L't';
    cache->temp_dir[actual_temp_len + 2] = L'k';
    cache->temp_dir[actual_temp_len + 3] = L'_';
    memcpy(cache->temp_dir + actual_temp_len + 4, suffix_str, (size_t)suffix_len * sizeof(wchar_t));
    cache->temp_dir[actual_temp_len + 4 + (size_t)suffix_len] = L'\\';
    cache->temp_dir[actual_temp_len + 4 + (size_t)suffix_len + 1] = L'\0';
    OV_ARRAY_SET_LENGTH(cache->temp_dir, dir_len);

    OV_ARRAY_DESTROY(&temp_path);
  }

  // Create directory
  if (!CreateDirectoryW(cache->temp_dir, NULL)) {
    DWORD const e = GetLastError();
    if (e != ERROR_ALREADY_EXISTS) {
      OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(e));
      goto cleanup;
    }
  }

  // Lock directory
  cache->dir_lock = CreateFileW(cache->temp_dir,
                                GENERIC_READ,
                                0, // no sharing
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
  if (cache->dir_lock == INVALID_HANDLE_VALUE) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  // Create hashmap (stores cache_entry* pointers, not cache_entry directly)
  cache->entries = OV_HASHMAP_CREATE_DYNAMIC(sizeof(struct cache_entry *), 64, get_entry_key);
  if (!cache->entries) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }

  return cache;

cleanup:
  ptk_cache_destroy(&cache);
  return NULL;
}

void ptk_cache_destroy(struct ptk_cache **const c) {
  if (!c || !*c) {
    return;
  }

  struct ptk_cache *cache = *c;

  ptk_cache_clear(cache);
  if (cache->entries) {
    OV_HASHMAP_DESTROY(&cache->entries);
  }
  if (cache->dir_lock != INVALID_HANDLE_VALUE) {
    CloseHandle(cache->dir_lock);
    cache->dir_lock = INVALID_HANDLE_VALUE;
  }
  if (cache->temp_dir) {
    // Delete the directory (should be empty now)
    RemoveDirectoryW(cache->temp_dir);
    OV_ARRAY_DESTROY(&cache->temp_dir);
  }
  OV_FREE(c);
}

bool ptk_cache_put(struct ptk_cache *const c,
                   uint64_t ckey,
                   void const *data,
                   int32_t width,
                   int32_t height,
                   struct ov_error *const err) {
  if (!c || !data || width <= 0 || height <= 0) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  char cachekey_hex[CACHEKEY_HEX_LEN + 1];
  ckey_to_hex(ckey, cachekey_hex);

  bool result = false;
  size_t const data_size = (size_t)width * (size_t)height * 4;
  struct cache_entry *new_entry = NULL;

  // Check if already exists - need a temporary entry for key lookup
  struct cache_entry key_entry = {0};
  memcpy(key_entry.cachekey_hex, cachekey_hex, CACHEKEY_HEX_LEN);
  struct cache_entry *key_ptr = &key_entry;

  // Look up existing entry
  {
    void const *const_ptr = OV_HASHMAP_GET(c->entries, &key_ptr);
    if (const_ptr) {
      // Entry exists - get pointer from hashmap
      struct cache_entry *existing = *(struct cache_entry *const *)const_ptr;
      // Already cached, just touch LRU
      lru_touch(c, existing);
      result = true;
      goto cleanup;
    }
  }

  // Allocate new entry on heap
  if (!OV_REALLOC(&new_entry, 1, sizeof(struct cache_entry))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  *new_entry = (struct cache_entry){0};
  memcpy(new_entry->cachekey_hex, cachekey_hex, CACHEKEY_HEX_LEN);
  new_entry->width = width;
  new_entry->height = height;
  new_entry->data_size = data_size;
  new_entry->in_file = false;

  // Copy pixel data
  if (!OV_REALLOC(&new_entry->data, data_size, 1)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  memcpy(new_entry->data, data, data_size);

  // Add pointer to hashmap
  if (!OV_HASHMAP_SET(c->entries, &new_entry)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }

  // Add to LRU (new_entry is heap-allocated, address is stable)
  lru_add(c, new_entry);
  c->memory_used += data_size;

  // Evict if needed
  if (c->memory_used > MEMORY_CACHE_LIMIT) {
    struct ov_error evict_err = {0};
    if (!evict_memory_to_file(c, &evict_err)) {
      // Non-fatal, just log
      ptk_logf_warn(&evict_err, "%1$hs", "%1$hs", "failed to evict cache to file tier");
      OV_ERROR_REPORT(&evict_err, NULL);
    }
  }
  if (c->file_used > FILE_CACHE_LIMIT) {
    evict_file_tier(c);
  }

  new_entry = NULL; // ownership transferred to hashmap
  result = true;

cleanup:
  if (new_entry) {
    if (new_entry->data) {
      OV_FREE(&new_entry->data);
    }
    OV_FREE(&new_entry);
  }
  return result;
}

bool ptk_cache_get(struct ptk_cache *const c,
                   uint64_t ckey,
                   void **data,
                   int32_t *width,
                   int32_t *height,
                   struct ov_error *const err) {
  if (!c || !data || !width || !height) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  char cachekey_hex[CACHEKEY_HEX_LEN + 1];
  ckey_to_hex(ckey, cachekey_hex);

  *data = NULL;
  *width = 0;
  *height = 0;

  bool result = false;
  struct cache_entry *entry = NULL;

  // Create temporary entry for key lookup
  struct cache_entry key_entry = {0};
  memcpy(key_entry.cachekey_hex, cachekey_hex, CACHEKEY_HEX_LEN);
  struct cache_entry *key_ptr = &key_entry;

  // Look up entry
  {
    void const *const_ptr = OV_HASHMAP_GET(c->entries, &key_ptr);
    if (!const_ptr) {
      // Cache miss - not an error
      result = true;
      goto cleanup;
    }
    entry = *(struct cache_entry *const *)const_ptr;
  }

  // Touch LRU
  lru_touch(c, entry);

  // If in file tier, read back to memory
  if (entry->in_file) {
    if (!read_entry_from_file(c, entry, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
    entry->in_file = false;
    c->file_used -= entry->data_size;
    c->memory_used += entry->data_size;
    // Delete file
    delete_entry_file(c, entry);

    // Evict if needed
    if (c->memory_used > MEMORY_CACHE_LIMIT) {
      struct ov_error evict_err = {0};
      if (!evict_memory_to_file(c, &evict_err)) {
        ptk_logf_warn(&evict_err, "%1$hs", "%1$hs", "failed to evict cache to file tier");
        OV_ERROR_REPORT(&evict_err, NULL);
      }
    }
  }

  // Allocate and copy data for caller
  if (!OV_REALLOC(data, entry->data_size, 1)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    goto cleanup;
  }
  memcpy(*data, entry->data, entry->data_size);
  *width = entry->width;
  *height = entry->height;

  result = true;

cleanup:
  return result;
}

void ptk_cache_clear(struct ptk_cache *const c) {
  if (!c) {
    return;
  }

  if (c->entries) {
    // Free all entry data and entries themselves
    size_t iter = 0;
    struct cache_entry **entry_ptr = NULL;
    while (OV_HASHMAP_ITER(c->entries, &iter, &entry_ptr)) {
      struct cache_entry *entry = *entry_ptr;
      if (entry->data) {
        OV_FREE(&entry->data);
      }
      if (entry->in_file) {
        delete_entry_file(c, entry);
      }
      OV_FREE(&entry);
    }
    OV_HASHMAP_CLEAR(c->entries);
  }

  c->lru_head = NULL;
  c->lru_tail = NULL;
  c->memory_used = 0;
  c->file_used = 0;
}
