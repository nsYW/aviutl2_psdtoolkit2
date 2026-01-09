#include "error.h"
#include "dialog.h"
#include "logf.h"

#include <ovarray.h>
#include <ovutf.h>

static HWND find_owner_window(void) {
  DWORD const pid = GetCurrentProcessId();
  wchar_t const class_name[] = L"aviutl2Manager";
  DWORD wpid;
  HWND h = NULL;
  while ((h = FindWindowExW(NULL, h, class_name, NULL)) != NULL) {
    GetWindowThreadProcessId(h, &wpid);
    if (wpid != pid) {
      continue;
    }
    return h;
  }
  return GetDesktopWindow();
}

// Internal helper to extract main message (assumes autofill already done)
static bool get_main_message_internal(struct ov_error const *const err, wchar_t **const dest) {
  if (!err || !dest) {
    return false;
  }

  char const *msg = err->stack[0].info.context;
  if (!msg) {
    return false;
  }

  size_t const len = strlen(msg);
  size_t const wchar_len = ov_utf8_to_wchar_len(msg, len);
  if (wchar_len == 0) {
    return false;
  }
  if (!OV_ARRAY_GROW(dest, wchar_len + 1)) {
    return false;
  }
  ov_utf8_to_wchar(msg, len, *dest, wchar_len + 1, NULL);
  return true;
}

// Get the main error message from ov_error (without error code or file position).
bool ptk_error_get_main_message(struct ov_error *const err, wchar_t **const dest) {
  if (!err || !dest) {
    return false;
  }
  ov_error_autofill_message(&err->stack[0], NULL);
  return get_main_message_internal(err, dest);
}

int ptk_error_dialog(HWND owner,
                     struct ov_error *const err,
                     wchar_t const *const window_title,
                     wchar_t const *const main_instruction,
                     wchar_t const *const content,
                     PCWSTR icon,
                     TASKDIALOG_COMMON_BUTTON_FLAGS buttons) {
  if (!err || !window_title || !main_instruction) {
    return 0;
  }

  if (!owner) {
    owner = find_owner_window();
  }

  // Autofill error message before extracting
  ov_error_autofill_message(&err->stack[0], NULL);

  char *msg_utf8 = NULL;
  wchar_t *msg_wchar = NULL;
  wchar_t *content_wchar = NULL;
  int button_id = 0;
  bool success = false;
  struct ov_error err2 = {0};

  {
    if (!ov_error_to_string(err, &msg_utf8, true, &err2)) {
      OV_ERROR_ADD_TRACE(&err2);
      goto cleanup;
    }

    // If content is NULL, use the main error message as content
    wchar_t const *content_to_use = content;
    if (!content_to_use) {
      if (get_main_message_internal(err, &content_wchar)) {
        content_to_use = content_wchar;
      }
    }

    ptk_logf_error(NULL,
                   "%1$ls%2$ls%3$hs",
                   "%1$ls\n%2$ls\n----------------\n%3$hs",
                   main_instruction,
                   content_to_use ? content_to_use : L"",
                   msg_utf8);

    size_t const len = strlen(msg_utf8);
    size_t const wchar_len = ov_utf8_to_wchar_len(msg_utf8, len);
    if (wchar_len == 0) {
      OV_ERROR_SET_GENERIC(&err2, ov_error_generic_fail);
      goto cleanup;
    }
    if (!OV_ARRAY_GROW(&msg_wchar, wchar_len + 1)) {
      OV_ERROR_SET_GENERIC(&err2, ov_error_generic_out_of_memory);
      goto cleanup;
    }
    ov_utf8_to_wchar(msg_utf8, len, msg_wchar, wchar_len + 1, NULL);

    button_id = ptk_dialog_show(&(struct ptk_dialog_params){
        .owner = owner,
        .icon = icon,
        .buttons = buttons,
        .window_title = window_title,
        .main_instruction = main_instruction,
        .content = content_to_use,
        .expanded_info = msg_wchar,
    });
    success = true;
  }

cleanup:
  if (msg_wchar) {
    OV_ARRAY_DESTROY(&msg_wchar);
  }
  if (msg_utf8) {
    OV_ARRAY_DESTROY(&msg_utf8);
  }
  if (content_wchar) {
    OV_ARRAY_DESTROY(&content_wchar);
  }
  if (!success) {
    OV_ERROR_REPORT(&err2, NULL);
  }
  return button_id;
}
