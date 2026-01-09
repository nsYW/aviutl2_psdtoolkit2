// PSDToolKit input plugin for .ptkcache files
#pragma once

#include <ovbase.h>

struct ptk_input;
struct ptk_cache;
struct aviutl2_input_info;

typedef void *aviutl2_input_handle;

/**
 * Create a new input plugin instance.
 *
 * @param cache Cache instance (not owned, must outlive input)
 * @param err Error details on failure
 * @return Pointer to created instance, or NULL on failure
 */
NODISCARD struct ptk_input *ptk_input_create(struct ptk_cache *cache, struct ov_error *err);

/**
 * Destroy an input plugin instance.
 *
 * @param inp Pointer to instance pointer (will be set to NULL)
 */
void ptk_input_destroy(struct ptk_input **inp);

/**
 * Open a .ptkcache file.
 *
 * @param inp Input instance
 * @param file Path to .ptkcache file
 * @return Handle on success, NULL on failure
 */
aviutl2_input_handle ptk_input_open(struct ptk_input *inp, wchar_t const *file);

/**
 * Close an input handle.
 *
 * @param inp Input instance
 * @param ih Handle to close
 * @return true on success
 */
bool ptk_input_close(struct ptk_input *inp, aviutl2_input_handle ih);

/**
 * Get input file information.
 *
 * @param inp Input instance
 * @param ih Input handle
 * @param iip Output: input info structure
 * @return true on success
 */
bool ptk_input_info_get(struct ptk_input *inp, aviutl2_input_handle ih, struct aviutl2_input_info *iip);

/**
 * Read video frame data.
 *
 * @param inp Input instance
 * @param ih Input handle
 * @param frame Frame number
 * @param buf Output buffer for pixel data
 * @return Number of bytes read, or 0 on failure
 */
int ptk_input_read_video(struct ptk_input *inp, aviutl2_input_handle ih, int frame, void *buf);
