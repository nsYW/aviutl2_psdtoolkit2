#include "win32.h"

#include <ovarray.h>
#include <ovutf.h>

struct disable_family_windows_data {
  HWND *ptr;
  size_t len;
  size_t cap;
  DWORD pid;
  HWND exclude;
};

static WINBOOL CALLBACK disable_family_windows_callback(HWND const window, LPARAM const lparam) {
  struct disable_family_windows_data *const d = (struct disable_family_windows_data *)lparam;
  if (!IsWindowVisible(window) || !IsWindowEnabled(window) || d->exclude == window) {
    return TRUE;
  }
  DWORD pid = 0;
  GetWindowThreadProcessId(window, &pid);
  if (pid != d->pid) {
    return TRUE;
  }
  if (!OV_ARRAY_GROW(&d->ptr, d->len + 1)) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  d->ptr[d->len++] = window;
  return TRUE;
}

HWND *ptk_win32_disable_family_windows(HWND const exclude) {
  struct disable_family_windows_data d = {
      .pid = GetCurrentProcessId(),
      .exclude = exclude,
  };
  if (!EnumWindows(disable_family_windows_callback, (LPARAM)&d)) {
    OV_ARRAY_DESTROY(&d.ptr);
    return NULL;
  }
  if (!OV_ARRAY_GROW(&d.ptr, d.len + 1)) {
    OV_ARRAY_DESTROY(&d.ptr);
    return NULL;
  }
  d.ptr[d.len] = NULL;

  HWND const *h = d.ptr;
  while (*h != NULL) {
    EnableWindow(*h++, FALSE);
  }
  return d.ptr;
}

void ptk_win32_restore_disabled_family_windows(HWND *disabled_windows) {
  if (!disabled_windows) {
    return;
  }
  HWND *h = disabled_windows;
  while (*h != NULL) {
    EnableWindow(*h++, TRUE);
  }
  OV_ARRAY_DESTROY(&disabled_windows);
}

static bool utf8_to_wstr(wchar_t **const dest, char const *const src, struct ov_error *const err) {
  size_t const src_len = strlen(src);
  size_t const wlen = ov_utf8_to_wchar_len(src, src_len);
  if (!OV_ARRAY_GROW(dest, wlen + 1)) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_out_of_memory);
    return false;
  }
  ov_utf8_to_wchar(src, src_len, *dest, wlen + 1, NULL);
  OV_ARRAY_SET_LENGTH(*dest, wlen + 1);
  return true;
}

bool ptk_win32_copy_to_clipboard(HWND const owner, char const *const text_utf8, struct ov_error *const err) {
  if (!owner || !text_utf8) {
    OV_ERROR_SET_GENERIC(err, ov_error_generic_invalid_argument);
    return false;
  }

  wchar_t *wtext = NULL;
  bool clipboard_open = false;
  HGLOBAL h = NULL;
  wchar_t *p = NULL;
  bool success = false;
  size_t len = 0;

  if (!utf8_to_wstr(&wtext, text_utf8, err)) {
    OV_ERROR_ADD_TRACE(err);
    goto cleanup;
  }
  len = wcslen(wtext);

  if (!OpenClipboard(owner)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }
  clipboard_open = true;

  h = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
  if (!h) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  p = (wchar_t *)GlobalLock(h);
  if (!p) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  memcpy(p, wtext, len * sizeof(wchar_t));
  p[len] = L'\0';
  GlobalUnlock(h);
  p = NULL;

  if (!EmptyClipboard()) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }

  if (!SetClipboardData(CF_UNICODETEXT, h)) {
    OV_ERROR_SET_HRESULT(err, HRESULT_FROM_WIN32(GetLastError()));
    goto cleanup;
  }
  h = NULL;

  success = true;

cleanup:
  if (p) {
    GlobalUnlock(h);
  }
  if (h) {
    GlobalFree(h);
  }
  if (clipboard_open) {
    CloseClipboard();
  }
  if (wtext) {
    OV_ARRAY_DESTROY(&wtext);
  }
  return success;
}
