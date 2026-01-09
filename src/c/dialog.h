#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <commctrl.h>

/**
 * @brief Custom button for TaskDialog
 */
struct ptk_dialog_button {
  int id;              // Button ID (returned when clicked)
  wchar_t const *text; // Button caption
};

/**
 * @brief Parameters for ptk_dialog_show
 */
struct ptk_dialog_params {
  HWND owner;                                     // Parent window handle (can be NULL)
  PCWSTR icon;                                    // Dialog icon (TD_WARNING_ICON, TD_ERROR_ICON, etc.)
  TASKDIALOG_COMMON_BUTTON_FLAGS buttons;         // Common buttons (TDCBF_OK_BUTTON, etc.)
  struct ptk_dialog_button const *custom_buttons; // Custom buttons (can be NULL)
  size_t custom_button_count;                     // Number of custom buttons
  int default_button;                             // Default button ID, or 0 for system default
  wchar_t const *window_title;                    // Dialog window title
  wchar_t const *main_instruction;                // Main instruction text (bold header)
  wchar_t const *content;                         // Content text (can be NULL)
  wchar_t const *expanded_info;                   // Expandable detail text (can be NULL)
};

/**
 * @brief Show a TaskDialog with automatic window management
 *
 * This function automatically disables all other windows in the current process
 * (except the dialog itself) before showing the dialog, and restores them after.
 * It also handles activation context for common controls.
 *
 * When custom_buttons is NULL, the common buttons specified by the buttons field are used.
 * When custom_buttons is provided, the custom buttons are used instead.
 *
 * @param params Dialog parameters
 * @return Clicked button ID (IDOK, IDCANCEL, etc.), or 0 on failure
 */
int ptk_dialog_show(struct ptk_dialog_params const *const params);
