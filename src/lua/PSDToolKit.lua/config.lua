-- Configuration loading for PSDToolKit
local M = {}

local debug = require("PSDToolKit.debug")
local cached_config = nil

--- Get configuration
-- Retrieves and caches config on first call per frame.
-- Also sets debug mode on first call.
-- @return table Config table with debug_mode, cache_index, resize_quality
function M.get()
	if not cached_config then
		local ptk = obj.module("PSDToolKit")
		if not ptk then
			error("PSDToolKit script module is not available")
		end
		local debug_mode, cache_index, resize_quality = ptk.get_render_config()
		if cache_index < 0 then
			error("PSDToolKit initialization failed")
		end
		debug.set_debug(debug_mode)
		cached_config = {
			debug_mode = debug_mode,
			cache_index = cache_index,
			resize_quality = resize_quality,
		}
	end
	return cached_config
end

--- Clear cached configuration
-- Called from init.lua at the start of each frame
function M.clear()
	cached_config = nil
end

return M
