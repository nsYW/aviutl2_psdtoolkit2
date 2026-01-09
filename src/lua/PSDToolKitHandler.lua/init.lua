-- PSDToolKit audio file drop extension handler
local M = {}

M.name = "psd/wav/object handler %VERSION% by oov"
M.priority = 1000

local config = require("PSDToolKitHandler.config")
local debug = require("PSDToolKitHandler.debug")
local wav = require("PSDToolKitHandler.wav")
local psd = require("PSDToolKitHandler.psd")

--- Handle drag enter event
-- @param files table List of file objects
-- @param state table Drop state
-- @return boolean True if drag should be accepted
function M.drag_enter(files, state)
	debug.set_debug(gcmz.get_script_module("PSDToolKit").get_debug_mode())
	local r = wav.has_wav_or_object(files) or psd.find_psd(files)
	-- If only PSD files are left in the list, the drop will be rejected,
	-- so replace them with placeholder even in drag_enter.
	psd.create_placeholder(files)
	return r
end

--- Handle drag leave event
function M.drag_leave() end

--- Handle file drop event
-- @param files table List of file objects to process
-- @param state table Drop state
-- @return boolean Always returns true to indicate processing completed
function M.drop(files, state)
	local cfg = config.get()
	psd.process(files, state, cfg)
	wav.process(files, state, cfg)
	return true
end

return M
