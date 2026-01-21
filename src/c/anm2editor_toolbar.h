#pragma once

#include <ovbase.h>

struct anm2editor_toolbar;

/**
 * @brief Callbacks for toolbar button commands
 *
 * These callbacks are invoked when the user clicks toolbar buttons.
 */
struct anm2editor_toolbar_callbacks {
  void *userdata;

  void (*on_file_new)(void *userdata);
  void (*on_file_open)(void *userdata);
  void (*on_file_save)(void *userdata);
  void (*on_file_saveas)(void *userdata);
  void (*on_edit_undo)(void *userdata);
  void (*on_edit_redo)(void *userdata);
  void (*on_edit_import_scripts)(void *userdata);
  void (*on_edit_convert_anm)(void *userdata);
};

/**
 * @brief Create a toolbar component
 *
 * Creates a Windows toolbar control with predefined buttons for the anm2 editor.
 * Loads PNG icons from resources and creates normal/disabled image lists.
 *
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param control_id Control ID for WM_NOTIFY
 * @param callbacks Event callbacks for button clicks
 * @param err Error information on failure
 * @return Created toolbar, or NULL on failure
 */
NODISCARD struct anm2editor_toolbar *anm2editor_toolbar_create(void *parent_window,
                                                               int control_id,
                                                               struct anm2editor_toolbar_callbacks const *callbacks,
                                                               struct ov_error *err);

/**
 * @brief Destroy a toolbar component
 *
 * @param toolbar Pointer to toolbar to destroy, will be set to NULL
 */
void anm2editor_toolbar_destroy(struct anm2editor_toolbar **toolbar);

/**
 * @brief Get the window handle of the toolbar
 *
 * @param toolbar Toolbar instance
 * @return Window handle (HWND cast to void*)
 */
void *anm2editor_toolbar_get_window(struct anm2editor_toolbar *toolbar);

/**
 * @brief Auto-size the toolbar
 *
 * Call this when the parent window is resized to adjust toolbar layout.
 *
 * @param toolbar Toolbar instance
 */
void anm2editor_toolbar_autosize(struct anm2editor_toolbar *toolbar);

/**
 * @brief Get the height of the toolbar
 *
 * @param toolbar Toolbar instance
 * @return Height in pixels
 */
int anm2editor_toolbar_get_height(struct anm2editor_toolbar *toolbar);

/**
 * @brief Update toolbar button states for undo/redo/save
 *
 * @param toolbar Toolbar instance
 * @param can_undo true if undo is available
 * @param can_redo true if redo is available
 * @param can_save true if save is available
 */
void anm2editor_toolbar_update_state(struct anm2editor_toolbar *toolbar, bool can_undo, bool can_redo, bool can_save);

/**
 * @brief Handle WM_COMMAND message for toolbar buttons
 *
 * Call this from the parent window's WM_COMMAND handler.
 * If the command ID matches a toolbar button, the corresponding callback is invoked.
 *
 * @param toolbar Toolbar instance
 * @param cmd_id Command ID from LOWORD(wparam)
 * @return true if command was handled, false otherwise
 */
bool anm2editor_toolbar_handle_command(struct anm2editor_toolbar *toolbar, int cmd_id);

/**
 * @brief Handle WM_NOTIFY message for toolbar tooltips
 *
 * Call this from the parent window's WM_NOTIFY handler.
 * Handles TTN_GETDISPINFOW to provide tooltip text for toolbar buttons.
 *
 * @param toolbar Toolbar instance
 * @param lparam LPARAM from WM_NOTIFY (pointer to NMHDR)
 * @return true if notification was handled, false otherwise
 */
bool anm2editor_toolbar_handle_notify(struct anm2editor_toolbar *toolbar, void *lparam);
