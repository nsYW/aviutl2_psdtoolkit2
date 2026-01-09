#pragma once

#include <ovbase.h>

#include "alias.h"

struct ptk_anm2editor;
struct aviutl2_edit_handle;

/**
 * @brief Target item for ~ptkl parameter assignment
 *
 * Represents a parameter that ends with "~ptkl" suffix in an animation item.
 */
struct ptk_anm2editor_ptkl_target {
  char *selector_name; // Selector group name, e.g., "目パチ"
  char *effect_name;   // Effect/item display name, e.g., "目パチ@PSDToolKit"
  char *param_key;     // Parameter key, e.g., "開き~ptkl"
  size_t sel_idx;      // Selector index
  size_t item_idx;     // Item index within selector
  size_t param_idx;    // Parameter index within item
};

/**
 * @brief Collection of ~ptkl targets
 */
struct ptk_anm2editor_ptkl_targets {
  struct ptk_anm2editor_ptkl_target *items; // ovarray
};

/**
 * @brief Free ptkl targets structure
 *
 * @param targets Targets to free
 */
void ptk_anm2editor_ptkl_targets_free(struct ptk_anm2editor_ptkl_targets *targets);

/**
 * @brief Create the PSDToolKit anm2 Editor instance
 *
 * Creates the editor and optionally a window for it.
 *
 * @param editor [out] Pointer to receive the created editor
 * @param title Window title (if window is not NULL)
 * @param edit_handle Edit handle for accessing AviUtl2 edit section (if window is not NULL)
 * @param window [out] Optional pointer to receive window handle (HWND), pass NULL to skip window creation
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_create(struct ptk_anm2editor **editor,
                                     wchar_t const *title,
                                     struct aviutl2_edit_handle *edit_handle,
                                     void **window,
                                     struct ov_error *err);

/**
 * @brief Destroy the PSDToolKit anm2 Editor instance
 *
 * @param editor [in,out] Pointer to editor to destroy, will be set to NULL
 */
void ptk_anm2editor_destroy(struct ptk_anm2editor **editor);

/**
 * @brief Get the editor window handle
 *
 * @param editor Editor instance
 * @return Window handle (HWND), or NULL if not created
 */
void *ptk_anm2editor_get_window(struct ptk_anm2editor *editor);

/**
 * @brief Create a new empty document for editing
 *
 * Initializes the editor with an empty metadata structure.
 * If there are unsaved changes, prompts the user to save.
 *
 * @param editor Editor instance
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_new(struct ptk_anm2editor *editor, struct ov_error *err);

/**
 * @brief Open an existing anm2 file for editing
 *
 * Loads the file and populates the editor with its contents.
 * If there are unsaved changes, prompts the user to save.
 *
 * @param editor Editor instance
 * @param path Path to the anm2 file (NULL to show open dialog)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_open(struct ptk_anm2editor *editor, wchar_t const *path, struct ov_error *err);

/**
 * @brief Save the current document
 *
 * Saves the metadata to the current file path.
 * If no path is set, shows a save dialog.
 *
 * @param editor Editor instance
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_save(struct ptk_anm2editor *editor, struct ov_error *err);

/**
 * @brief Save the current document to a new path
 *
 * Shows a save dialog and saves the metadata to the selected path.
 *
 * @param editor Editor instance
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_save_as(struct ptk_anm2editor *editor, struct ov_error *err);

/**
 * @brief Check if the document has unsaved changes
 *
 * @param editor Editor instance
 * @return true if there are unsaved changes
 */
bool ptk_anm2editor_is_modified(struct ptk_anm2editor *editor);

/**
 * @brief Check if the editor is open (window exists and visible)
 *
 * @param editor Editor instance
 * @return true if the editor window is open
 */
bool ptk_anm2editor_is_open(struct ptk_anm2editor *editor);

/**
 * @brief Get the current PSD file path
 *
 * @param editor Editor instance
 * @return PSD file path (UTF-8), or NULL if not set. Caller must NOT free this.
 */
char const *ptk_anm2editor_get_psd_path(struct ptk_anm2editor *editor);

/**
 * @brief Set the PSD file path
 *
 * @param editor Editor instance
 * @param path PSD file path (UTF-8)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_set_psd_path(struct ptk_anm2editor *editor, char const *path, struct ov_error *err);

/**
 * @brief Add a selector with value items to the editor
 *
 * Creates a new selector with the specified group name and adds value items
 * (name/value pairs). If psd_path is provided and the editor has no PSD path set,
 * it will be set as part of the same undo transaction.
 *
 * @param editor Editor instance
 * @param psd_path PSD file path to set if editor has none (UTF-8, can be NULL)
 * @param group Selector group name (UTF-8)
 * @param names Array of item names (UTF-8, count elements)
 * @param values Array of item values (UTF-8, count elements)
 * @param count Number of items
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_add_value_items(struct ptk_anm2editor *editor,
                                              char const *psd_path,
                                              char const *group,
                                              char const *const *names,
                                              char const *const *values,
                                              size_t count,
                                              struct ov_error *err);

/**
 * @brief Add a single value item to the selected selector or create a new one
 *
 * If a selector is currently selected, adds the value item to that selector.
 * If no selector is selected, creates a new selector with the specified group
 * name and adds the value item to it. If psd_path is provided and the editor
 * has no PSD path set, it will be set as part of the same undo transaction.
 *
 * @param editor Editor instance
 * @param psd_path PSD file path to set if editor has none (UTF-8, can be NULL)
 * @param group Selector group name (UTF-8, used only when creating new selector)
 * @param name Item name (UTF-8)
 * @param value Item value (UTF-8)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_add_value_item_to_selected(struct ptk_anm2editor *editor,
                                                         char const *psd_path,
                                                         char const *group,
                                                         char const *name,
                                                         char const *value,
                                                         struct ov_error *err);

/**
 * @brief Collect ~ptkl parameter targets from the currently selected selector
 *
 * Scans animation items in the currently selected selector for parameters
 * that end with "~ptkl" suffix. If no selector is selected, returns an empty result.
 *
 * @param editor Editor instance
 * @param targets [out] Output targets (caller must free with ptk_anm2editor_ptkl_targets_free)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_collect_selected_ptkl_targets(struct ptk_anm2editor *editor,
                                                            struct ptk_anm2editor_ptkl_targets *targets,
                                                            struct ov_error *err);

/**
 * @brief Set a parameter value in the editor
 *
 * Updates the specified parameter value and refreshes the UI.
 *
 * @param editor Editor instance
 * @param sel_idx Selector index
 * @param item_idx Item index within selector
 * @param param_idx Parameter index within item
 * @param value New value to set (UTF-8)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_set_param_value(struct ptk_anm2editor *editor,
                                              size_t sel_idx,
                                              size_t item_idx,
                                              size_t param_idx,
                                              char const *value,
                                              struct ov_error *err);

/**
 * @brief Get the index of the currently selected selector
 *
 * @param editor Editor instance
 * @return Index of the selected selector, or SIZE_MAX if no selector is selected
 */
size_t ptk_anm2editor_get_selected_selector_index(struct ptk_anm2editor *editor);

/**
 * @brief Add an animation item to the editor
 *
 * Adds an animation item (e.g., Blinker, LipSync) to the specified selector.
 * The item is inserted at the beginning of the selector's item list.
 *
 * @param editor Editor instance
 * @param sel_idx Selector index (use ptk_anm2editor_get_selected_selector_index() or 0)
 * @param script_name Script name (e.g., "PSDToolKit.Blinker")
 * @param display_name Display name (e.g., "目パチ@PSDToolKit")
 * @param params Array of parameter key-value pairs
 * @param param_count Number of parameters
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2editor_add_animation_item(struct ptk_anm2editor *editor,
                                                 size_t sel_idx,
                                                 char const *script_name,
                                                 char const *display_name,
                                                 struct ptk_alias_extracted_param const *params,
                                                 size_t param_count,
                                                 struct ov_error *err);
