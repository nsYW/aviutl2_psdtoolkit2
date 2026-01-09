--- LipSync - Simple open/close lip sync
-- This module provides lip sync animation based on audio volume levels.
-- It uses a pattern system with 2-5 states (closed to fully open).
local LipSync = {}
LipSync.__index = LipSync

local debug = require("PSDToolKit.debug")
local dbg = debug.dbg

-- Per-layer state cache for smooth animation
local states = {}

--- Clear the per-layer state cache.
function LipSync.clear()
	states = {}
end

-- Display name keys for pattern parameters
-- Order: closed -> almost_closed -> half -> almost_open -> open
-- This order matches the animation logic where patterns[1] is closed (default)
-- and patterns[n] is open
local pattern_keys = {
	"閉じ~ptkl",
	"ほぼ閉じ~ptkl",
	"半開き~ptkl",
	"ほぼ開き~ptkl",
	"開き~ptkl",
}

--- Validate that at least 2 patterns are present.
-- @param patterns table: Patterns array
-- @return boolean: true if valid
local function validate(patterns)
	if #patterns < 2 then
		error("LipSync requires at least 2 patterns (closed and open)")
	end
	return true
end

--- Create a new LipSync object from legacy parameters.
-- This function is called from converted anm scripts.
-- @param patterns table: Array of layer state strings (2-5 elements, closed to open)
-- @param sensitivity number: Number of frames for moving average (default: 1)
-- @param alwaysapply boolean: Apply even when no voice data is available
-- @return LipSync object
function LipSync.new_legacy(patterns, sensitivity, alwaysapply)
	local opts = {
		["閉じ~ptkl"] = patterns[1],
		["開き~ptkl"] = patterns[#patterns],
		["感度"] = tostring(sensitivity or 1),
		["発声がなくても有効"] = alwaysapply and "1" or "0",
	}
	if #patterns >= 3 then
		opts["ほぼ閉じ~ptkl"] = patterns[2]
	end
	if #patterns >= 4 then
		opts["半開き~ptkl"] = patterns[3]
	end
	if #patterns >= 5 then
		opts["ほぼ開き~ptkl"] = patterns[4]
	end
	return LipSync.new(opts)
end

--- Creates a new LipSync object.
-- @param opts table: Options table with display name keys
--   Pattern keys (values are layer paths):
--     "閉じ~ptkl": Closed state
--     "ほぼ閉じ~ptkl": Almost closed state (optional)
--     "半開き~ptkl": Half open state (optional)
--     "ほぼ開き~ptkl": Almost open state (optional)
--     "開き~ptkl": Open state
--   Numeric parameters (stored as strings):
--     "ローカット": Low frequency cutoff in Hz (default 100)
--     "ハイカット": High frequency cutoff in Hz (default 1000)
--     "しきい値": Volume threshold for opening mouth (default 20)
--     "感度": Number of frames for moving average (default 1)
--     "発声がなくても有効": Apply even when no voice data is available (0 or 1)
-- @return LipSync: New LipSync object
function LipSync.new(opts)
	if not opts then
		error("opts cannot be nil")
	end

	-- Build patterns array from display name keys
	-- Collect non-empty patterns in order
	local patterns = {}
	for _, key in ipairs(pattern_keys) do
		local value = opts[key]
		if value and value ~= "" then
			table.insert(patterns, value)
		end
	end

	-- Parse numeric parameters (stored as strings)
	local locut = tonumber(opts["ローカット"]) or 100
	local hicut = tonumber(opts["ハイカット"]) or 1000
	local threshold = tonumber(opts["しきい値"]) or 20
	local sensitivity = tonumber(opts["感度"]) or 1

	-- Convert alwaysapply to boolean (stored as "0" or "1")
	local alwaysapply_val = tonumber(opts["発声がなくても有効"]) or 0
	local alwaysapply = alwaysapply_val ~= 0

	return setmetatable({
		patterns = patterns,
		sensitivity = sensitivity,
		alwaysapply = alwaysapply,
		locut = locut,
		hicut = hicut,
		threshold = threshold,
	}, LipSync)
end

--- Calculate moving average volume.
-- @param stat table: State table with vols array
-- @param volume number: Current volume
-- @param time number: Current time
-- @param sensitivity number: Number of frames for moving average
-- @return number: Averaged volume
local function calculate_moving_average(stat, volume, time, sensitivity)
	-- Reset if time goes backwards or jumps too far forward
	-- Allow 1 second tolerance for preview scrubbing
	if stat.time > time or stat.time + 1 < time then
		stat.vols = {}
	end

	-- Add current volume to history
	table.insert(stat.vols, volume)

	-- Keep only the last 'sensitivity' frames
	while #stat.vols > sensitivity do
		table.remove(stat.vols, 1)
	end

	stat.time = time

	-- Calculate average
	local sum = 0
	for _, v in ipairs(stat.vols) do
		sum = sum + v
	end

	return sum / #stat.vols
end

--- Gets the current lip state based on audio volume.
-- This method is called from PSD:buildlayer() during animation rendering.
-- @param ctx table: Context object with get_voice and get_audio_level methods
-- @return string: The layer state string for current lip position
function LipSync:getstate(ctx)
	validate(self.patterns)

	local obj = ctx.obj

	-- Resolve voice_id from ctx.psd.character_id, or fallback to ctx.obj.layer
	local voice_id
	if ctx.psd and ctx.psd.character_id and ctx.psd.character_id ~= "" then
		voice_id = ctx.psd.character_id
	else
		voice_id = obj.layer
	end

	-- Get voice data from context
	local voice = ctx:get_voice(voice_id)

	if voice == nil then
		if self.alwaysapply then
			return self.patterns[1] -- Return closed state
		end
		return ""
	end

	-- Get or initialize per-layer state
	local stat = states[obj.layer]
	if not stat then
		stat = { time = obj.time, pat = 0, vols = {}, voice_obj_id = voice.obj_id }
	end

	-- Reset animation state if voice object changed
	if stat.voice_obj_id ~= voice.obj_id then
		stat = { time = obj.time, pat = 0, vols = {}, voice_obj_id = voice.obj_id }
	end

	-- Reset animation state if time goes backwards or jumps too far forward
	if stat.time > obj.time or stat.time + 1 < obj.time then
		stat.pat = 0
		stat.vols = {}
	end

	-- Get audio level from Voice's pre-captured fourier data (via voice_id)
	local raw_volume = ctx:get_audio_level(voice_id, self.locut, self.hicut)

	-- Apply moving average for smooth animation
	local avg_volume = calculate_moving_average(stat, raw_volume, obj.time, self.sensitivity)

	dbg("LipSync: raw=%.2f avg=%.2f threshold=%.2f pat=%d", raw_volume, avg_volume, self.threshold, stat.pat)

	-- Determine target pattern based on volume vs threshold
	if avg_volume >= self.threshold then
		-- Volume is above threshold - open mouth more
		if stat.pat < #self.patterns - 1 then
			stat.pat = stat.pat + 1
		end
	else
		-- Volume is below threshold - close mouth more
		if stat.pat > 0 then
			stat.pat = stat.pat - 1
		end
	end

	stat.time = obj.time
	states[obj.layer] = stat

	return self.patterns[stat.pat + 1]
end

return LipSync
