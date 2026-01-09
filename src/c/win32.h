#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <ovbase.h>

/**
 * @brief Disable all visible windows in the current process except the excluded window
 *
 * This function enumerates all top-level windows belonging to the current process,
 * disables them, and returns an array of the disabled window handles.
 * Use this before showing a modal dialog to prevent interaction with other windows.
 *
 * @param exclude Window handle to exclude from disabling (typically the modal dialog itself)
 * @return Array of disabled window handles (NULL-terminated), or NULL on failure.
 *         The caller must pass this to ptk_win32_restore_disabled_family_windows() to restore.
 */
HWND *ptk_win32_disable_family_windows(HWND exclude);

/**
 * @brief Restore windows disabled by ptk_win32_disable_family_windows
 *
 * Re-enables all windows in the provided array and frees the array memory.
 * Safe to call with NULL.
 *
 * @param disabled_windows Array returned by ptk_win32_disable_family_windows (will be freed)
 */
void ptk_win32_restore_disabled_family_windows(HWND *disabled_windows);

/**
 * @brief Copy UTF-8 text to the Windows clipboard
 *
 * Converts the UTF-8 text to UTF-16 and copies it to the clipboard as CF_UNICODETEXT.
 *
 * @param owner Owner window handle for clipboard operations
 * @param text_utf8 UTF-8 encoded text to copy
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool ptk_win32_copy_to_clipboard(HWND owner, char const *text_utf8, struct ov_error *const err);
