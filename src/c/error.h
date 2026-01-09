#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <commctrl.h>

#include <ovbase.h>

/**
 * @brief Get the main error message from ov_error (without error code or file position)
 *
 * @param err Error information to extract message from
 * @param dest Pointer to wchar_t* that will receive the allocated message (must be freed with OV_ARRAY_DESTROY)
 * @return true on success, false on failure
 */
bool ptk_error_get_main_message(struct ov_error *const err, wchar_t **const dest);

/**
 * @brief Display an error dialog with detailed error information
 *
 * @param owner Parent window handle (NULL for auto-detect)
 * @param err Error information to display
 * @param window_title Window title text
 * @param main_instruction Main instruction text displayed prominently
 * @param content Content text (can be NULL)
 * @param icon Icon to display (e.g., TD_ERROR_ICON, TD_WARNING_ICON)
 * @param buttons Button flags (e.g., TDCBF_OK_BUTTON, TDCBF_RETRY_BUTTON | TDCBF_CANCEL_BUTTON)
 * @return Button ID that was clicked (e.g., IDOK, IDRETRY, IDCANCEL), or 0 on failure
 */
int ptk_error_dialog(HWND owner,
                     struct ov_error *const err,
                     wchar_t const *const window_title,
                     wchar_t const *const main_instruction,
                     wchar_t const *const content,
                     PCWSTR icon,
                     TASKDIALOG_COMMON_BUTTON_FLAGS buttons);
