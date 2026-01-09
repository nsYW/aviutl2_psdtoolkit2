#pragma once

#include <ovbase.h>

/**
 * @brief Custom error codes for anm to anm2 conversion
 *
 * These codes are used with ov_error_type_generic to identify specific
 * error conditions during conversion.
 */
enum ptk_anm_to_anm2_error {
  /**
   * @brief Not a valid legacy anm script
   *
   * The input file does not appear to be a legacy PSDToolKit anm script.
   * This typically means "PSD:addstate(" was not found in the file.
   */
  ptk_anm_to_anm2_error_not_legacy_script = 2000,
};

/**
 * @brief Convert legacy anm script to anm2 format
 *
 * Converts Shift_JIS encoded *.anm script to UTF-8 encoded *.anm2 format.
 * Performs string replacements for legacy PSDToolKit API calls.
 *
 * Replacements performed:
 * - require("PSDToolKit").Blinker.new(      -> require("PSDToolKit.Blinker").new_legacy(
 * - require("PSDToolKit").LipSyncSimple.new( -> require("PSDToolKit.LipSync").new_legacy(
 * - require("PSDToolKit").LipSyncLab.new(   -> require("PSDToolKit.LipSyncLab").new_legacy(
 * - PSD:addstate(                           -> require("PSDToolKit").add_state_legacy(
 *
 * @param src Source data (Shift_JIS encoded)
 * @param src_len Source data length in bytes
 * @param dst [out] Output data (UTF-8 encoded, ovarray). Caller must destroy with OV_ARRAY_DESTROY.
 * @param err [out] Error information
 * @return true on success, false on failure
 */
NODISCARD bool ptk_anm_to_anm2(char const *src, size_t src_len, char **dst, struct ov_error *err);
