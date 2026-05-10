-- PSDToolKit audio file drop extension handler
local M = {}

M.name = "psd/wav/object handler %VERSION% by oov"
M.priority = 1000

local config = require("PSDToolKitHandler.config")
local wav = require("PSDToolKitHandler.wav")
local psd = require("PSDToolKitHandler.psd")

--- Handle drag enter event
-- @param files table List of file objects
-- @param state table Drop state
-- @return boolean True if drag should be accepted
function M.drag_enter(files, state)
	config.get()
	local r = wav.has_wav_or_object(files) or psd.find_pending_pfv_filename(files) ~= nil
	return r
end

--- Handle drag leave event
function M.drag_leave() end

--- Handle file drop event
-- @param files table List of file objects to process
-- @param state table Drop state
-- @return boolean Always returns true to indicate processing completed
function M.drop(files, state)
	local pending_pfv_filename = psd.find_pending_pfv_filename(files)
	if pending_pfv_filename ~= nil then
		local ptk = gcmz.get_script_module("PSDToolKit")
		if not ptk then
			error("PSDToolKit script module is not available")
		end
		if not ptk.set_pending_psd_pfv_filename(pending_pfv_filename) then
			error("PSDToolKit: failed to store pending pfv filename")
		end
	end
	local cfg = config.get()
	wav.process(files, state, cfg)
	return true
end

return M
