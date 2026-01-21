#include "anm2editor_convert.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ovarray.h>
#include <ovmo.h>
#include <ovprintf.h>

#include <ovl/dialog.h>
#include <ovl/file.h>
#include <ovl/path.h>

#include "anm_to_anm2.h"
#include "dialog.h"
#include "error.h"

static wchar_t const *get_window_title(void) {
  static wchar_t buf[64];
  if (!buf[0]) {
    ov_snprintf_wchar(buf, sizeof(buf) / sizeof(buf[0]), L"%1$hs", L"%1$hs", gettext("PSDToolKit anm2 Editor"));
  }
  return buf;
}

static wchar_t const *get_anm_file_filter(void) {
  static wchar_t buf[256];
  if (!buf[0]) {
    ov_snprintf_char2wchar(buf,
                           sizeof(buf) / sizeof(buf[0]),
                           "%1$hs%2$hs",
                           "%1$hs (*.anm)|*.anm|%2$hs (*.*)|*.*|",
                           pgettext("anm2editor", "AviUtl1 Animation Script"),
                           pgettext("anm2editor", "All Files"));
    for (wchar_t *p = buf; *p; ++p) {
      if (*p == L'|') {
        *p = L'\0';
      }
    }
  }
  return buf;
}

static wchar_t const *get_anm2_convert_save_filter(void) {
  static wchar_t buf[256];
  if (!buf[0]) {
    ov_snprintf_char2wchar(buf,
                           sizeof(buf) / sizeof(buf[0]),
                           "%1$hs%2$hs",
                           "%1$hs (*.anm2)|*.anm2|%2$hs (*.*)|*.*|",
                           pgettext("anm2editor", "AviUtl ExEdit2 Animation Script"),
                           pgettext("anm2editor", "All Files"));
    for (wchar_t *p = buf; *p; ++p) {
      if (*p == L'|') {
        *p = L'\0';
      }
    }
  }
  return buf;
}

bool anm2editor_convert_execute(void *const parent_window,
                                wchar_t const *const script_dir,
                                struct ov_error *const err) {
  bool success = false;
  bool cancelled = false;
  wchar_t *src_path = NULL;
  wchar_t *dst_path = NULL;
  wchar_t *default_save_path = NULL;
  char *src_data = NULL;
  char *dst_data = NULL;
  struct ovl_file *src_file = NULL;
  struct ovl_file *dst_file = NULL;
  HWND const hwnd = (HWND)parent_window;

  // Show file open dialog for source anm file
  {
    // {83F03793-4997-442C-A4E4-AEE63D6117FC}
    static GUID const open_dialog_guid = {0x83f03793, 0x4997, 0x442c, {0xa4, 0xe4, 0xae, 0xe6, 0x3d, 0x61, 0x17, 0xfc}};
    wchar_t title[256];
    ov_snprintf_wchar(
        title, sizeof(title) / sizeof(title[0]), L"%hs", L"%hs", pgettext("anm2editor", "Select *.anm to convert"));
    if (!ovl_dialog_select_file(hwnd, title, get_anm_file_filter(), &open_dialog_guid, script_dir, &src_path, err)) {
      if (ov_error_is(err, ov_error_type_hresult, (int)HRESULT_FROM_WIN32(ERROR_CANCELLED))) {
        cancelled = true;
        goto cleanup;
      }
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
  }

  // Read source file
  {
    uint64_t file_size = 0;
    size_t bytes_read = 0;

    if (!ovl_file_open(src_path, &src_file, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }

    if (!ovl_file_size(src_file, &file_size, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }

    if (!OV_ARRAY_GROW(&src_data, file_size + 1)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }

    if (!ovl_file_read(src_file, src_data, (size_t)file_size, &bytes_read, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
    src_data[bytes_read] = '\0';
    OV_ARRAY_SET_LENGTH(src_data, bytes_read);

    ovl_file_close(src_file);
    src_file = NULL;
  }

  // Convert anm to anm2
  {
    size_t const src_len = OV_ARRAY_LENGTH(src_data);
    if (!ptk_anm_to_anm2(src_data, src_len, &dst_data, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
  }

  // Generate default output path: same name without extension (dialog will add .anm2)
  {
    // Find base name without extension
    wchar_t const *ext = ovl_path_find_ext(src_path);
    size_t const base_len = ext ? (size_t)(ext - src_path) : wcslen(src_path);

    if (!OV_ARRAY_GROW(&default_save_path, base_len + 1)) {
      OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
      goto cleanup;
    }
    memcpy(default_save_path, src_path, base_len * sizeof(wchar_t));
    default_save_path[base_len] = L'\0';
    OV_ARRAY_SET_LENGTH(default_save_path, base_len);

    // Show save dialog
    // {B979B21D-C448-4079-A94F-DCD12FC8D15C}
    static GUID const save_dialog_guid = {0xb979b21d, 0xc448, 0x4079, {0xa9, 0x4f, 0xdc, 0xd1, 0x2f, 0xc8, 0xd1, 0x5c}};
    wchar_t title[256];
    ov_snprintf_wchar(
        title, sizeof(title) / sizeof(title[0]), L"%hs", L"%hs", pgettext("anm2editor", "Save converted *.anm2"));
    if (!ovl_dialog_save_file(hwnd,
                              title,
                              get_anm2_convert_save_filter(),
                              &save_dialog_guid,
                              default_save_path,
                              L"anm2",
                              &dst_path,
                              err)) {
      if (ov_error_is(err, ov_error_type_hresult, (int)HRESULT_FROM_WIN32(ERROR_CANCELLED))) {
        cancelled = true;
        goto cleanup;
      }
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }
  }

  // Write destination file
  {
    size_t const dst_len = OV_ARRAY_LENGTH(dst_data);
    size_t bytes_written = 0;

    if (!ovl_file_create(dst_path, &dst_file, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }

    if (!ovl_file_write(dst_file, dst_data, dst_len, &bytes_written, err)) {
      OV_ERROR_ADD_TRACE(err);
      goto cleanup;
    }

    ovl_file_close(dst_file);
    dst_file = NULL;
  }

  // Show success dialog with warning
  {
    wchar_t msg[256];
    wchar_t content[512];
    ov_snprintf_wchar(
        msg, sizeof(msg) / sizeof(msg[0]), L"%hs", L"%hs", pgettext("anm2editor", "Conversion completed."));
    ov_snprintf_wchar(
        content,
        sizeof(content) / sizeof(content[0]),
        L"%hs",
        L"%hs",
        pgettext("anm2editor",
                 "Note: This conversion uses simple string replacement and may not work correctly in all cases.\n"
                 "Also, this converted script is different from *.ptk.anm2 and cannot be edited in this editor."));
    ptk_dialog_show(&(struct ptk_dialog_params){
        .owner = hwnd,
        .icon = TD_INFORMATION_ICON,
        .buttons = TDCBF_OK_BUTTON,
        .window_title = get_window_title(),
        .main_instruction = msg,
        .content = content,
    });
  }

  success = true;

cleanup:
  if (src_file) {
    ovl_file_close(src_file);
  }
  if (dst_file) {
    ovl_file_close(dst_file);
  }
  if (default_save_path) {
    OV_ARRAY_DESTROY(&default_save_path);
  }
  if (src_path) {
    OV_ARRAY_DESTROY(&src_path);
  }
  if (dst_path) {
    OV_ARRAY_DESTROY(&dst_path);
  }
  if (src_data) {
    OV_ARRAY_DESTROY(&src_data);
  }
  if (dst_data) {
    OV_ARRAY_DESTROY(&dst_data);
  }
  if (cancelled) {
    OV_ERROR_DESTROY(err);
    return true;
  }
  return success;
}
