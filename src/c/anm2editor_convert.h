#pragma once

#include <ovbase.h>

/**
 * @brief Execute ANM to ANM2 conversion
 *
 * Shows file dialogs to select source ANM file and destination ANM2 file,
 * converts the file, and shows a result dialog.
 *
 * @param parent_window Parent window handle (HWND cast to void*)
 * @param script_dir Default directory for file dialogs
 * @param err Error information on failure
 * @return true on success or user cancel, false on error
 */
NODISCARD bool anm2editor_convert_execute(void *parent_window, wchar_t const *script_dir, struct ov_error *err);
