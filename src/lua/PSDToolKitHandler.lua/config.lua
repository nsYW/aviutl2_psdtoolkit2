-- Configuration loading for PSDToolKit handler
local M = {}

local debug = require("PSDToolKitHandler.debug")
local ptk = gcmz.get_script_module("PSDToolKit")
if not ptk then
	error("PSDToolKit script module is not available")
end

local cached_config = nil

--- Get configuration
-- Retrieves and caches config on first call.
-- Also sets debug mode on first call.
-- @return table Config table
function M.get()
	if not cached_config then
		local cfg = ptk.get_drop_config()
		if not cfg then
			error("failed to get drop configuration from PSDToolKit script module")
		end
		debug.set_debug(cfg.debug_mode)
		cached_config = cfg
	end
	return cached_config
end

--- Clear cached configuration
-- Should be called when config needs to be refreshed
function M.clear()
	cached_config = nil
end

return M
