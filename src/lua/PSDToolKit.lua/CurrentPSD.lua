--- CurrentPSD - Current PSD instance management (singleton)
-- Manages the current PSD object being processed in the current frame.
-- Provides functions to initialize, draw, and add states to the current PSD.
local CurrentPSD = {}

local debug = require("PSDToolKit.debug")
local dbg = debug.dbg
local i18n = require("PSDToolKit.i18n").i18n
local FrameState = require("PSDToolKit.FrameState")
local PSD = require("PSDToolKit.PSD")
local LayerSelector = require("PSDToolKit.LayerSelector")
local ValueCache = require("PSDToolKit.ValueCache")

local current = nil

--- Clear the current PSD reference.
-- Called at the beginning of each frame processing.
function CurrentPSD.clear()
	current = nil
end

--- Get the current PSD object.
-- @return PSD|nil: The current PSD object, or nil if not initialized
function CurrentPSD.get()
	return current
end

--- Get the current PSD object, or error if not initialized.
-- @return PSD: The current PSD object
function CurrentPSD.require()
	if not current then
		error(i18n({
			ja_JP = "PSDが初期化されていません。init_psdを先に呼び出してください。",
			en_US = "No PSD initialized. Call init_psd first.",
			zh_CN = "PSD未初始化。请先调用init_psd。",
		}))
	end
	return current
end

--- Initialize a PSD object for the current layer.
-- @param opts table: Options {file=string, scene=number, tag=number, layer=string, scale=number, offsetx=number, offsety=number, character_id=string}
-- @param obj table: The AviUtl object
-- @return PSD: The initialized PSD object
function CurrentPSD.init(opts, obj)
	if not opts then
		error("opts cannot be nil")
	end
	if not opts.file or opts.file == "" then
		error("opts.file is required")
	end

	local ptk = obj.module("PSDToolKit")
	if not ptk then
		error("PSDToolKit script module is not available")
	end

	-- Generate unique ID using PSD.make_id (includes validation)
	local scene = opts.scene or 0
	local id = PSD.make_id(scene, obj.layer)

	-- Generate tag if not provided
	local tag = opts.tag
	if not tag or tag == 0 then
		tag = ptk.generate_tag()
	end

	-- Add PSD file to Go process
	local ok = ptk.add_psd_file(opts.file, tag)
	if not ok then
		error("failed to add PSD file")
	end

	-- Create PSD object
	local psd = PSD.new(id, opts.file, tag, {
		layer = opts.layer,
		scale = opts.scale,
		offsetx = opts.offsetx,
		offsety = opts.offsety,
		character_id = opts.character_id,
	})

	dbg(
		"CurrentPSD.init: id=%s file=%s tag=%s character_id=%s",
		tostring(id),
		tostring(opts.file),
		tostring(tag),
		tostring(opts.character_id)
	)

	-- Store reference
	current = psd

	return psd
end

--- Draw the current PSD object.
-- @param obj table: The AviUtl object
function CurrentPSD.draw(obj)
	if FrameState.has_error() then
		return
	end
	if not current then
		error(i18n({
			ja_JP = "「PSDファイル@PSDToolKit」が配置されていません。",
			en_US = "'PSDファイル@PSDToolKit' is not placed.",
			zh_CN = "未放置「PSDファイル@PSDToolKit」。",
		}))
	end
	current:draw(obj)
end

--- Add a lip sync state to the current PSD.
-- @param opts table: Options with display name keys (passed directly to LipSync.new)
-- @param obj table: The AviUtl object
function CurrentPSD.add_lipsync(opts, obj)
	if FrameState.has_error() then
		return
	end
	local psd = CurrentPSD.require()
	if not opts then
		error("opts cannot be nil")
	end

	local LipSync = require("PSDToolKit.LipSync")
	local lipsync = LipSync.new(opts)

	psd:add_state(lipsync)
	dbg("CurrentPSD.add_lipsync")
end

--- Add a vowel-based lip sync (aiueo) state to the current PSD.
-- @param opts table: Options with display name keys (passed directly to LipSyncLab.new)
-- @param obj table: The AviUtl object
function CurrentPSD.add_lipsync_lab(opts, obj)
	if FrameState.has_error() then
		return
	end
	local psd = CurrentPSD.require()
	if not opts then
		error("opts cannot be nil")
	end

	local LipSyncLab = require("PSDToolKit.LipSyncLab")
	local lipsync = LipSyncLab.new(opts)

	psd:add_state(lipsync)
	dbg("CurrentPSD.add_lipsync_lab")
end

--- Add a blinker (eye blink) state to the current PSD.
-- @param opts table: Options with display name keys (passed directly to Blinker.new)
-- @param obj table: The AviUtl object
function CurrentPSD.add_blinker(opts, obj)
	if FrameState.has_error() then
		return
	end
	local psd = CurrentPSD.require()
	if not opts then
		error("opts cannot be nil")
	end

	local Blinker = require("PSDToolKit.Blinker")
	local blinker = Blinker.new(opts)

	psd:add_state(blinker)
	dbg("CurrentPSD.add_blinker")
end

--- Add a layer selector state to the current PSD.
-- Uses ValueCache internally to cache factory results.
-- @param part_index number: Explicit part index for this selector (1, 2, 3, ...)
-- @param factory function: Factory function that returns array of layer state strings
-- @param selected number: Index of the selected layer (1-based)
-- @param opts table|nil: Optional parameters {exclusive=boolean}
function CurrentPSD.add_layer_selector(part_index, factory, selected, opts)
	if FrameState.has_error() then
		return
	end
	local psd = CurrentPSD.require()
	if type(part_index) ~= "number" then
		error("part_index must be a number")
	end
	if type(factory) ~= "function" then
		error("factory must be a function")
	end

	opts = opts or {}
	local exclusive = opts.exclusive or false

	-- Cache key includes exclusive flag to separate cached values
	local effect_key = obj.effect_id .. (exclusive and "_1" or "_0")

	-- Wrap factory to build exclusive values if needed
	local wrapped_factory
	if exclusive then
		wrapped_factory = function()
			return LayerSelector.build_exclusive_values(factory())
		end
	else
		wrapped_factory = factory
	end

	-- Get cached values
	local values = ValueCache.get(effect_key, part_index, wrapped_factory)

	if not values or #values < 1 then
		error("factory must return at least 1 value")
	end

	local selector = LayerSelector.new(values, selected, {
		character_id = psd.character_id,
		part_index = part_index,
	})

	psd:add_state(selector)
	dbg(
		"CurrentPSD.add_layer_selector part_index=%d values=%d selected=%s character_id=%s exclusive=%s",
		part_index,
		#values,
		tostring(selected),
		tostring(psd.character_id),
		tostring(exclusive)
	)
end

--- Add layer state to the current PSD.
-- @param layer string|table: Layer state string or table with getstate method, or a list of layer states
-- @param index number|nil: If provided with a list, select layer[index]
function CurrentPSD.add_state(layer, index)
	if FrameState.has_error() then
		return
	end
	local psd = CurrentPSD.require()
	psd:add_state(layer, index)
end

--- Add layer state to the current PSD (legacy API).
-- This function is called from converted AviUtl1 *.anm scripts via PSD:addstate().
-- Supports two calling conventions:
--   add_state_legacy(values, selected_index) - table with index
--   add_state_legacy("layer/path") - single string
-- @param values table|string: Array of layer state strings, or a single layer state string
-- @param selected number|nil: Index of the selected layer (1-based), only used when values is a table
function CurrentPSD.add_state_legacy(values, selected)
	CurrentPSD.add_state(values, selected)
end

return CurrentPSD
