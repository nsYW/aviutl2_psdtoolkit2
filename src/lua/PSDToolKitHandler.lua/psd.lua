-- PSD drop metadata helpers for PSDToolKit handler
local M = {}

local util = require("PSDToolKitHandler.util")

--- Get directory part of a file path
-- @param filepath string File path
-- @return string Directory path (including trailing separator)
local function get_directory(filepath)
	local filename = util.get_filename(filepath)
	return filepath:sub(1, #filepath - #filename)
end

--- Find the pfv filename for the first PSD/PSB target
-- @param files table List of file objects
-- @return string|nil PFV filename, empty string if no matching pfv exists, nil if no PSD/PSB exists
function M.find_pending_pfv_filename(files)
	for _, file in ipairs(files) do
		local ext = util.get_extension(file.filepath)
		if ext == "psd" or ext == "psb" then
			local psd_dir = get_directory(file.filepath)
			for _, pfv_file in ipairs(files) do
				if util.get_extension(pfv_file.filepath) == "pfv" then
					local pfv_path = pfv_file.filepath
					local pfv_dir = get_directory(pfv_path)
					if psd_dir == pfv_dir then
						return util.get_filename(pfv_path)
					end
				end
			end
			return ""
		end
	end
	return nil
end

return M
