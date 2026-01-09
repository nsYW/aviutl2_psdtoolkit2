#pragma once

#include <ovbase.h>

struct ptk_config;

/**
 * @brief Show PSDToolKit configuration dialog
 *
 * Displays a modal dialog for editing PSDToolKit audio drop extension settings.
 * Changes are saved to PSDToolKit.json when the user clicks OK.
 *
 * @param config Configuration object to use for the dialog
 * @param parent_window Parent window handle for dialog positioning (can be NULL)
 * @param err [out] Error information on failure
 * @return true on success, false on failure
 */
bool ptk_config_dialog_show(struct ptk_config *config, void *parent_window, struct ov_error *const err);
