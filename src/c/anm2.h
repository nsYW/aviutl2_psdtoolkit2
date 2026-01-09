#pragma once

#include <ovbase.h>

struct ptk_anm2;

/**
 * @brief Custom error codes for anm2 operations
 *
 * These codes are used with ov_error_type_generic to identify specific
 * error conditions during document operations.
 */
enum ptk_anm2_error {
  /**
   * @brief Invalid file format - not a PSDToolKit anm2 script
   *
   * The file does not contain the expected JSON metadata header.
   * This typically means the file is not a *.ptk.anm2 file created by PSDToolKit.
   */
  ptk_anm2_error_invalid_format = 3000,
};

/**
 * @brief Operation types for change notifications
 *
 * These are exposed for the change callback to identify what operation occurred.
 */
enum ptk_anm2_op_type {
  // Special operation: document reset (load/new)
  ptk_anm2_op_reset = 0,

  // Group markers (for transaction boundaries)
  ptk_anm2_op_group_begin,
  ptk_anm2_op_group_end,

  // Metadata operations
  ptk_anm2_op_set_label,
  ptk_anm2_op_set_psd_path,
  ptk_anm2_op_set_exclusive_support_default,
  ptk_anm2_op_set_information,

  // Selector operations
  ptk_anm2_op_selector_insert,
  ptk_anm2_op_selector_remove,
  ptk_anm2_op_selector_set_group,
  ptk_anm2_op_selector_move,

  // Item operations
  ptk_anm2_op_item_insert,
  ptk_anm2_op_item_remove,
  ptk_anm2_op_item_set_name,
  ptk_anm2_op_item_set_value,
  ptk_anm2_op_item_set_script_name,
  ptk_anm2_op_item_move,

  // Parameter operations
  ptk_anm2_op_param_insert,
  ptk_anm2_op_param_remove,
  ptk_anm2_op_param_set_key,
  ptk_anm2_op_param_set_value,
};

/**
 * @brief Callback function type for document change notifications
 *
 * Called after each operation is applied to the document.
 *
 * @param userdata User-provided context
 * @param op_type Type of operation that was performed
 * @param sel_idx Selector index (for selector/item/param operations)
 * @param item_idx Item index (for item/param operations)
 * @param param_idx Parameter index (for param operations)
 * @param to_sel_idx Destination selector index (for move operations)
 * @param to_idx Destination index (for move operations)
 */
typedef void (*ptk_anm2_change_callback)(void *userdata,
                                         enum ptk_anm2_op_type op_type,
                                         size_t sel_idx,
                                         size_t item_idx,
                                         size_t param_idx,
                                         size_t to_sel_idx,
                                         size_t to_idx);

/**
 * @brief Set the change callback for document modifications
 *
 * The callback will be invoked after each successful operation.
 * Set to NULL to disable notifications.
 *
 * @param doc Document handle
 * @param callback Callback function (or NULL to disable)
 * @param userdata User-provided context passed to callback
 */
void ptk_anm2_set_change_callback(struct ptk_anm2 *doc, ptk_anm2_change_callback callback, void *userdata);

// ============================================================================
// Document lifecycle
// ============================================================================

/**
 * @brief Create a new empty anm2 document
 *
 * @param err Error information
 * @return New document handle on success, NULL on failure
 */
NODISCARD struct ptk_anm2 *ptk_anm2_new(struct ov_error *const err);

/**
 * @brief Destroy an anm2 document and free all resources
 *
 * @param doc Pointer to document handle (will be set to NULL)
 */
void ptk_anm2_destroy(struct ptk_anm2 **doc);

// ============================================================================
// File operations
// ============================================================================

/**
 * @brief Load an anm2 document from file
 *
 * Parses the PTK JSON metadata embedded in the script file.
 * Clears UNDO/REDO history after successful load.
 *
 * @param doc Document handle
 * @param path Path to the anm2 file
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_load(struct ptk_anm2 *doc, wchar_t const *path, struct ov_error *const err);

/**
 * @brief Check if a document can be saved
 *
 * Returns true if the document has a PSD path set and contains at least one
 * selector with items. A document without a PSD path or without any items
 * would produce an invalid/useless script.
 *
 * @param doc Document handle
 * @return true if save is possible, false otherwise
 */
bool ptk_anm2_can_save(struct ptk_anm2 const *doc);

/**
 * @brief Save an anm2 document to file
 *
 * Generates the script with embedded JSON metadata and writes to file.
 *
 * @param doc Document handle
 * @param path Path to save the anm2 file
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_save(struct ptk_anm2 *doc, wchar_t const *path, struct ov_error *const err);

/**
 * @brief Verify checksum of a loaded anm2 document
 *
 * Compares the stored checksum (from JSON metadata) with the calculated checksum
 * (from script body). This detects manual edits to the file.
 *
 * @param doc Document handle (must be loaded via ptk_anm2_load)
 * @return true if checksum matches, false if mismatch
 */
NODISCARD bool ptk_anm2_verify_checksum(struct ptk_anm2 const *doc);

// ============================================================================
// Metadata operations
// ============================================================================

/**
 * @brief Get the document label
 *
 * @param doc Document handle
 * @return Label string (owned by doc, do not free)
 */
char const *ptk_anm2_get_label(struct ptk_anm2 const *doc);

/**
 * @brief Set the document label
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param label New label string
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_set_label(struct ptk_anm2 *doc, char const *label, struct ov_error *const err);

/**
 * @brief Get the PSD file path
 *
 * @param doc Document handle
 * @return PSD path string (owned by doc, do not free)
 */
char const *ptk_anm2_get_psd_path(struct ptk_anm2 const *doc);

/**
 * @brief Set the PSD file path
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param path New PSD path string
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_set_psd_path(struct ptk_anm2 *doc, char const *path, struct ov_error *const err);

/**
 * @brief Get the exclusive support control default value
 *
 * When true, the generated script will have exclusive support control enabled by default.
 *
 * @param doc Document handle
 * @return Current exclusive support default value (default: true for new documents)
 */
bool ptk_anm2_get_exclusive_support_default(struct ptk_anm2 const *doc);

/**
 * @brief Set the exclusive support control default value
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param exclusive_support_default New exclusive default value
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_set_exclusive_support_default(struct ptk_anm2 *doc,
                                                      bool exclusive_support_default,
                                                      struct ov_error *const err);

/**
 * @brief Get the custom information text
 *
 * @param doc Document handle
 * @return Custom information string (owned by doc, do not free), NULL if auto-generate from filename
 */
char const *ptk_anm2_get_information(struct ptk_anm2 const *doc);

/**
 * @brief Set the custom information text
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param information New information string (NULL to auto-generate from PSD filename)
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_set_information(struct ptk_anm2 *doc, char const *information, struct ov_error *const err);

/**
 * @brief Get the document version
 *
 * @param doc Document handle
 * @return Version number (read-only, internally managed)
 */
int ptk_anm2_get_version(struct ptk_anm2 const *doc);

// ============================================================================
// Selector operations
// ============================================================================

/**
 * @brief Get the number of selectors
 *
 * @param doc Document handle
 * @return Number of selectors
 */
size_t ptk_anm2_selector_count(struct ptk_anm2 const *doc);

/**
 * @brief Add a new selector at the end
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param group Group name for the new selector
 * @param err Error information
 * @return ID of the new selector on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_selector_add(struct ptk_anm2 *doc, char const *group, struct ov_error *const err);

/**
 * @brief Remove a selector at the specified index
 *
 * Records UNDO operation (including all contained items).
 *
 * @param doc Document handle
 * @param idx Index of the selector to remove
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_selector_remove(struct ptk_anm2 *doc, size_t idx, struct ov_error *const err);

/**
 * @brief Get the group name of a selector
 *
 * @param doc Document handle
 * @param idx Selector index
 * @return Group name string (owned by doc, do not free), NULL if index is invalid
 */
char const *ptk_anm2_selector_get_group(struct ptk_anm2 const *doc, size_t idx);

/**
 * @brief Set the group name of a selector
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param idx Selector index
 * @param group New group name
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool
ptk_anm2_selector_set_group(struct ptk_anm2 *doc, size_t idx, char const *group, struct ov_error *const err);

/**
 * @brief Move a selector to a new position
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param from_idx Current index of the selector
 * @param to_idx Target index (element at to_idx will shift)
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool
ptk_anm2_selector_move_to(struct ptk_anm2 *doc, size_t from_idx, size_t to_idx, struct ov_error *const err);

// ============================================================================
// Item operations
// ============================================================================

/**
 * @brief Get the number of items in a selector
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @return Number of items, 0 if selector index is invalid
 */
size_t ptk_anm2_item_count(struct ptk_anm2 const *doc, size_t sel_idx);

/**
 * @brief Check if an item is an animation item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return true if animation item, false if value item or indices are invalid
 */
bool ptk_anm2_item_is_animation(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Add a value item at the end of a selector
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param name Display name
 * @param value Layer path value
 * @param err Error information
 * @return ID of the new item on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_item_add_value(
    struct ptk_anm2 *doc, size_t sel_idx, char const *name, char const *value, struct ov_error *const err);

/**
 * @brief Insert a value item at the specified position
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Position to insert
 * @param name Display name
 * @param value Layer path value
 * @param err Error information
 * @return ID of the new item on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_item_insert_value(struct ptk_anm2 *doc,
                                              size_t sel_idx,
                                              size_t item_idx,
                                              char const *name,
                                              char const *value,
                                              struct ov_error *const err);

/**
 * @brief Add an animation item at the end of a selector
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param script_name Script name (e.g., "PSDToolKit.Blinker")
 * @param name Display name
 * @param err Error information
 * @return ID of the new item on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_item_add_animation(
    struct ptk_anm2 *doc, size_t sel_idx, char const *script_name, char const *name, struct ov_error *const err);

/**
 * @brief Insert an animation item at the specified position
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Position to insert
 * @param script_name Script name (e.g., "PSDToolKit.Blinker")
 * @param name Display name
 * @param err Error information
 * @return ID of the new item on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_item_insert_animation(struct ptk_anm2 *doc,
                                                  size_t sel_idx,
                                                  size_t item_idx,
                                                  char const *script_name,
                                                  char const *name,
                                                  struct ov_error *const err);

/**
 * @brief Remove an item from a selector
 *
 * Records UNDO operation (including all parameters for animation items).
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_item_remove(struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, struct ov_error *const err);

/**
 * @brief Move an item to a new position
 *
 * Moves an item from one selector to another, or within the same selector.
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param from_sel_idx Source selector index
 * @param from_idx Current item index within source selector
 * @param to_sel_idx Destination selector index (can be same as from_sel_idx)
 * @param to_idx Target index within destination selector
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_item_move_to(struct ptk_anm2 *doc,
                                     size_t from_sel_idx,
                                     size_t from_idx,
                                     size_t to_sel_idx,
                                     size_t to_idx,
                                     struct ov_error *const err);

/**
 * @brief Get the display name of an item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Name string (owned by doc), NULL if indices are invalid
 */
char const *ptk_anm2_item_get_name(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Set the display name of an item
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param name New display name
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_item_set_name(
    struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, char const *name, struct ov_error *const err);

/**
 * @brief Get the value (layer path) of a value item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Value string (owned by doc), NULL if indices are invalid or item is animation
 */
char const *ptk_anm2_item_get_value(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Set the value (layer path) of a value item
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param value New layer path value
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_item_set_value(
    struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, char const *value, struct ov_error *const err);

/**
 * @brief Get the script name of an animation item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Script name (owned by doc), NULL if indices are invalid or item is value
 */
char const *ptk_anm2_item_get_script_name(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Set the script name of an animation item
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param script_name New script name
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_item_set_script_name(
    struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, char const *script_name, struct ov_error *const err);

// ============================================================================
// Parameter operations (for animation items)
// ============================================================================

/**
 * @brief Get the number of parameters for an animation item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Number of parameters, 0 if indices are invalid or item is not animation
 */
size_t ptk_anm2_param_count(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Add a parameter to an animation item
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param key Parameter key
 * @param value Parameter value
 * @param err Error information
 * @return ID of the new parameter on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_param_add(struct ptk_anm2 *doc,
                                      size_t sel_idx,
                                      size_t item_idx,
                                      char const *key,
                                      char const *value,
                                      struct ov_error *const err);

/**
 * @brief Insert a parameter at the specified position
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Position to insert
 * @param key Parameter key
 * @param value Parameter value
 * @param err Error information
 * @return ID of the new parameter on success, 0 on failure
 */
NODISCARD uint32_t ptk_anm2_param_insert(struct ptk_anm2 *doc,
                                         size_t sel_idx,
                                         size_t item_idx,
                                         size_t param_idx,
                                         char const *key,
                                         char const *value,
                                         struct ov_error *const err);

/**
 * @brief Remove a parameter from an animation item
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_param_remove(
    struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, size_t param_idx, struct ov_error *const err);

/**
 * @brief Get the key of a parameter
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @return Key string (owned by doc), NULL if indices are invalid
 */
char const *ptk_anm2_param_get_key(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx, size_t param_idx);

/**
 * @brief Set the key of a parameter
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @param key New key string
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_param_set_key(struct ptk_anm2 *doc,
                                      size_t sel_idx,
                                      size_t item_idx,
                                      size_t param_idx,
                                      char const *key,
                                      struct ov_error *const err);

/**
 * @brief Get the value of a parameter
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @return Value string (owned by doc), NULL if indices are invalid
 */
char const *ptk_anm2_param_get_value(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx, size_t param_idx);

/**
 * @brief Set the value of a parameter
 *
 * Records UNDO operation.
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @param value New value string
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_param_set_value(struct ptk_anm2 *doc,
                                        size_t sel_idx,
                                        size_t item_idx,
                                        size_t param_idx,
                                        char const *value,
                                        struct ov_error *const err);

// ============================================================================
// UNDO/REDO operations
// ============================================================================

/**
 * @brief Check if undo is available
 *
 * @param doc Document handle
 * @return true if undo is available
 */
bool ptk_anm2_can_undo(struct ptk_anm2 const *doc);

/**
 * @brief Check if redo is available
 *
 * @param doc Document handle
 * @return true if redo is available
 */
bool ptk_anm2_can_redo(struct ptk_anm2 const *doc);

/**
 * @brief Undo the last operation
 *
 * If the last operation was grouped (via transaction), undoes all operations
 * in the group.
 *
 * @param doc Document handle
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_undo(struct ptk_anm2 *doc, struct ov_error *const err);

/**
 * @brief Redo the last undone operation
 *
 * If the undone operation was grouped (via transaction), redoes all operations
 * in the group.
 *
 * @param doc Document handle
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_redo(struct ptk_anm2 *doc, struct ov_error *const err);

/**
 * @brief Clear all undo/redo history
 *
 * @param doc Document handle
 */
void ptk_anm2_clear_undo_history(struct ptk_anm2 *doc);

/**
 * @brief Begin a transaction (group multiple operations for single undo)
 *
 * Transactions can be nested. GROUP_BEGIN is recorded only when depth becomes 1.
 *
 * @param doc Document handle
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_begin_transaction(struct ptk_anm2 *doc, struct ov_error *const err);

/**
 * @brief End a transaction
 *
 * GROUP_END is recorded only when depth returns to 0.
 *
 * @param doc Document handle
 * @param err Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm2_end_transaction(struct ptk_anm2 *doc, struct ov_error *const err);

// ============================================================================
// ID and userdata operations
// ============================================================================

/**
 * @brief Get the unique ID of a selector
 *
 * @param doc Document handle
 * @param idx Selector index
 * @return Unique ID, 0 if index is invalid
 */
uint32_t ptk_anm2_selector_get_id(struct ptk_anm2 const *doc, size_t idx);

/**
 * @brief Get the userdata of a selector
 *
 * @param doc Document handle
 * @param idx Selector index
 * @return Userdata value, 0 if index is invalid
 */
uintptr_t ptk_anm2_selector_get_userdata(struct ptk_anm2 const *doc, size_t idx);

/**
 * @brief Set the userdata of a selector
 *
 * This does NOT record UNDO operation (userdata is UI state, not document data).
 *
 * @param doc Document handle
 * @param idx Selector index
 * @param userdata New userdata value
 */
void ptk_anm2_selector_set_userdata(struct ptk_anm2 *doc, size_t idx, uintptr_t userdata);

/**
 * @brief Get the unique ID of an item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Unique ID, 0 if indices are invalid
 */
uint32_t ptk_anm2_item_get_id(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Get the userdata of an item
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @return Userdata value, 0 if indices are invalid
 */
uintptr_t ptk_anm2_item_get_userdata(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx);

/**
 * @brief Set the userdata of an item
 *
 * This does NOT record UNDO operation (userdata is UI state, not document data).
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param userdata New userdata value
 */
void ptk_anm2_item_set_userdata(struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, uintptr_t userdata);

/**
 * @brief Get the unique ID of a parameter
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @return Unique ID, 0 if indices are invalid
 */
uint32_t ptk_anm2_param_get_id(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx, size_t param_idx);

/**
 * @brief Get the userdata of a parameter
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @return Userdata value, 0 if indices are invalid
 */
uintptr_t ptk_anm2_param_get_userdata(struct ptk_anm2 const *doc, size_t sel_idx, size_t item_idx, size_t param_idx);

/**
 * @brief Set the userdata of a parameter
 *
 * This does NOT record UNDO operation (userdata is UI state, not document data).
 *
 * @param doc Document handle
 * @param sel_idx Selector index
 * @param item_idx Item index
 * @param param_idx Parameter index
 * @param userdata New userdata value
 */
void ptk_anm2_param_set_userdata(
    struct ptk_anm2 *doc, size_t sel_idx, size_t item_idx, size_t param_idx, uintptr_t userdata);

// ============================================================================
// ID reverse lookup operations
// ============================================================================

/**
 * @brief Find a selector by its unique ID
 *
 * @param doc Document handle
 * @param id ID to search for
 * @param out_sel_idx Output: selector index if found
 * @return true if found, false if not found
 */
bool ptk_anm2_find_selector_by_id(struct ptk_anm2 const *doc, uint32_t id, size_t *out_sel_idx);

/**
 * @brief Find an item by its unique ID
 *
 * @param doc Document handle
 * @param id ID to search for
 * @param out_sel_idx Output: selector index if found
 * @param out_item_idx Output: item index if found
 * @return true if found, false if not found
 */
bool ptk_anm2_find_item_by_id(struct ptk_anm2 const *doc, uint32_t id, size_t *out_sel_idx, size_t *out_item_idx);

/**
 * @brief Find a parameter by its unique ID
 *
 * @param doc Document handle
 * @param id ID to search for
 * @param out_sel_idx Output: selector index if found
 * @param out_item_idx Output: item index if found
 * @param out_param_idx Output: parameter index if found
 * @return true if found, false if not found
 */
bool ptk_anm2_find_param_by_id(
    struct ptk_anm2 const *doc, uint32_t id, size_t *out_sel_idx, size_t *out_item_idx, size_t *out_param_idx);
