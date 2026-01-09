// PSDToolKit input plugin for .ptkcache files
#include "input.h"

#include "cache.h"

#include <ovl/path.h>
#include <ovutf.h>

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <mmsystem.h>
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-declarations"
#include <aviutl2_input2.h>
#pragma clang diagnostic pop

// Input plugin instance structure
struct ptk_input {
  struct ptk_cache *cache; // Not owned, must outlive this instance
};

// Input file handle structure
struct ptk_input_handle {
  BITMAPINFOHEADER bih;
  uint8_t *data;    // Cached pixel data (BGRA)
  size_t data_size; // Size of pixel data
};

/**
 * @brief Extract cachekey from a .ptkcache filename
 *
 * Parses the filename to extract the 16-character hex cachekey and converts to uint64.
 * Expected format: "0123456789abcdef.ptkcache"
 *
 * @param file Full path or filename to parse
 * @param ckey Output for the extracted cachekey
 * @param err Error information (can be NULL)
 * @return true on success, false on failure
 */
static bool extract_cachekey(wchar_t const *const file, uint64_t *const ckey, struct ov_error *const err) {
  if (!file || !ckey) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  wchar_t const *const filename = ovl_path_extract_file_name(file);
  if (!ovl_path_is_same_ext(ovl_path_find_ext(filename), L".ptkcache")) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  uint64_t result = 0;
  size_t i;
  for (i = 0; i < 16 && filename[i] && filename[i] != L'.'; i++) {
    wchar_t const c = filename[i];
    uint64_t digit;
    if (c >= L'0' && c <= L'9') {
      digit = (uint64_t)(c - L'0');
    } else if (c >= L'a' && c <= L'f') {
      digit = (uint64_t)(c - L'a' + 10);
    } else if (c >= L'A' && c <= L'F') {
      digit = (uint64_t)(c - L'A' + 10);
    } else {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
      return false;
    }
    result = (result << 4) | digit;
  }

  // Must be exactly 16 hex characters followed by '.'
  if (i != 16 || filename[16] != L'.') {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  *ckey = result;
  return true;
}

struct ptk_input *ptk_input_create(struct ptk_cache *const cache, struct ov_error *const err) {
  if (!cache) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return NULL;
  }

  struct ptk_input *p = NULL;

  if (!OV_REALLOC(&p, 1, sizeof(struct ptk_input))) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    return NULL;
  }
  *p = (struct ptk_input){
      .cache = cache,
  };

  return p;
}

void ptk_input_destroy(struct ptk_input **const inp) {
  if (!inp || !*inp) {
    return;
  }
  OV_FREE(inp);
}

aviutl2_input_handle ptk_input_open(struct ptk_input *const inp, wchar_t const *const file) {
  struct ov_error err = {0};
  struct ptk_input_handle *h = NULL;
  uint64_t ckey = 0;
  void *data = NULL;
  int32_t width = 0;
  int32_t height = 0;
  aviutl2_input_handle result = NULL;

  if (!inp || !inp->cache) {
    goto cleanup;
  }

  // Extract cachekey from filename
  if (!extract_cachekey(file, &ckey, NULL)) {
    goto cleanup;
  }

  // Try to get from cache
  if (!ptk_cache_get(inp->cache, ckey, &data, &width, &height, &err)) {
    // Error occurred
    OV_ERROR_REPORT(&err, NULL);
    goto cleanup;
  }

  // Allocate handle
  if (!OV_REALLOC(&h, 1, sizeof(struct ptk_input_handle))) {
    goto cleanup;
  }
  *h = (struct ptk_input_handle){0};

  if (data) {
    // Cache hit
    h->data = (uint8_t *)data;
    h->data_size = (size_t)width * (size_t)height * 4;
    data = NULL; // Transfer ownership
  } else {
    // Cache miss - return error
    goto cleanup;
  }

  h->bih = (BITMAPINFOHEADER){
      .biSize = sizeof(BITMAPINFOHEADER),
      .biWidth = width,
      .biHeight = height,
      .biPlanes = 1,
      .biBitCount = 32,
      .biCompression = BI_RGB,
      .biSizeImage = (DWORD)((size_t)width * (size_t)height * 4),
      .biXPelsPerMeter = 0,
      .biYPelsPerMeter = 0,
      .biClrUsed = 0,
      .biClrImportant = 0,
  };

  result = (aviutl2_input_handle)h;
  h = NULL;

cleanup:
  if (data) {
    OV_FREE(&data);
  }
  if (h) {
    if (h->data) {
      OV_FREE(&h->data);
    }
    OV_FREE(&h);
  }
  return result;
}

bool ptk_input_close(struct ptk_input *const inp, aviutl2_input_handle const ih) {
  (void)inp;
  struct ptk_input_handle *h = (struct ptk_input_handle *)ih;
  if (h) {
    if (h->data) {
      OV_FREE(&h->data);
    }
    OV_FREE(&h);
  }
  return true;
}

bool ptk_input_info_get(struct ptk_input *const inp,
                        aviutl2_input_handle const ih,
                        struct aviutl2_input_info *const iip) {
  (void)inp;
  struct ptk_input_handle *h = (struct ptk_input_handle *)ih;
  if (!h || !iip) {
    return false;
  }

  iip->flag = aviutl2_input_info_flag_video;
  iip->rate = 1;
  iip->scale = 1;
  iip->n = 1; // 1 frame (still image)
  iip->format = &h->bih;
  iip->format_size = sizeof(BITMAPINFOHEADER);
  iip->audio_n = 0;
  iip->audio_format = NULL;
  iip->audio_format_size = 0;

  return true;
}

int ptk_input_read_video(struct ptk_input *const inp, aviutl2_input_handle const ih, int const frame, void *const buf) {
  (void)inp;
  (void)frame;
  struct ptk_input_handle *h = (struct ptk_input_handle *)ih;
  if (!h || !buf) {
    return 0;
  }

  size_t const buf_size = (size_t)(h->bih.biWidth * h->bih.biHeight) * 4;

  if (h->data && h->data_size == buf_size) {
    // Copy cached data to buffer
    memcpy(buf, h->data, h->data_size);
    return (int)h->data_size;
  }

  // No data available
  return 0;
}
