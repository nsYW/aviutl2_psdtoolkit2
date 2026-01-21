#pragma once

#include <ovbase.h>
#include <stddef.h>

struct aviutl2_edit_handle;
struct ptk_alias_available_scripts;

/**
 * @brief Callback invoked when import data is ready
 *
 * The callback receives the collected import data and should execute the actual import.
 * All data is owned by the caller and will be freed after the callback returns.
 *
 * @param context User data pointer
 * @param alias Alias string from the selected object
 * @param scripts Available scripts with selection state
 * @param selector_name Suggested selector name (NULL if using existing selector)
 * @param update_psd_path Whether PSD path should be updated
 * @param err Error information on failure
 * @return true on success, false on failure
 */
typedef bool (*anm2editor_import_callback)(void *context,
                                           char const *alias,
                                           struct ptk_alias_available_scripts const *scripts,
                                           char const *selector_name,
                                           bool update_psd_path,
                                           struct ov_error *err);

/**
 * @brief Show import dialog and execute import via callback
 *
 * Gets alias from the currently focused object in AviUtl ExEdit2,
 * enumerates available scripts, shows script picker dialog if needed,
 * and calls the callback with collected data to execute the import.
 *
 * The callback will not be called if user cancels or no items are selected
 * (unless update_psd_path is requested).
 *
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param edit_handle AviUtl2 edit handle for accessing edit section
 * @param current_psd_path Current PSD path in the editor (for comparison)
 * @param has_selected_selector true if a selector is currently selected
 * @param callback Callback to execute import
 * @param callback_context User data for the callback
 * @param err Error information on failure
 * @return true on success or user cancel, false on error
 */
NODISCARD bool anm2editor_import_execute(void *parent_window,
                                         struct aviutl2_edit_handle *edit_handle,
                                         char const *current_psd_path,
                                         bool has_selected_selector,
                                         anm2editor_import_callback callback,
                                         void *callback_context,
                                         struct ov_error *err);
