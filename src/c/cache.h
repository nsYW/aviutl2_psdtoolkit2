#pragma once

#include <ovbase.h>

#include <stdint.h>

struct ptk_cache;

/**
 * Create a new cache instance.
 *
 * Creates a temporary directory under TEMP/ptk_{pid}_{instance}/ and acquires an exclusive lock.
 * Also cleans up orphaned cache directories from previous crashed processes.
 *
 * @param err Error details on failure
 * @return Pointer to created cache instance, or NULL on failure
 */
NODISCARD struct ptk_cache *ptk_cache_create(struct ov_error *err);

/**
 * Destroy a cache instance.
 *
 * Releases the directory lock and deletes all cached files.
 *
 * @param c Pointer to cache instance pointer (will be set to NULL)
 */
void ptk_cache_destroy(struct ptk_cache **c);

/**
 * Store rendered image data in the cache.
 *
 * The data is first stored in memory. When memory usage exceeds the limit,
 * older entries are moved to file storage. When file storage also exceeds
 * its limit, the oldest entries are deleted.
 *
 * @param c Cache instance
 * @param ckey 64-bit cache key
 * @param data BGRA pixel data (width * height * 4 bytes)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @param err Error details on failure
 * @return true on success, false on failure
 */
NODISCARD bool ptk_cache_put(
    struct ptk_cache *c, uint64_t ckey, void const *data, int32_t width, int32_t height, struct ov_error *err);

/**
 * Retrieve cached image data.
 *
 * On cache hit, allocates and returns a copy of the pixel data.
 * On cache miss, sets *data to NULL (not an error).
 * The caller is responsible for freeing the returned data with OV_FREE.
 *
 * @param c Cache instance
 * @param ckey 64-bit cache key
 * @param data Output: allocated copy of BGRA pixel data, or NULL on miss
 * @param width Output: image width in pixels
 * @param height Output: image height in pixels
 * @param err Error details on failure
 * @return true on success (including cache miss), false on error
 */
NODISCARD bool
ptk_cache_get(struct ptk_cache *c, uint64_t ckey, void **data, int32_t *width, int32_t *height, struct ov_error *err);

/**
 * Clear all cached entries.
 *
 * Removes all entries from both memory and file tiers.
 * The cache instance remains valid for continued use.
 *
 * @param c Cache instance
 */
void ptk_cache_clear(struct ptk_cache *c);
