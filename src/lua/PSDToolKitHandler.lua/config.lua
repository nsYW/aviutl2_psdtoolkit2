-- Configuration loading for PSDToolKit handler
local M = {}

local ptk = gcmz.get_script_module("PSDToolKit")
if not ptk then
	error("PSDToolKit script module is not available")
end

--- Get configuration
-- Always returns a valid config table.
-- @return table Config table
function M.get()
	local cfg = ptk.get_drop_config()
	if not cfg then
		error("failed to get drop configuration from PSDToolKit script module")
	end
	return cfg
end

return M
