--- Context module for PSDToolKit
-- Provides context object for state evaluation (LipSync, Blinker, etc.)
local Context = {}

local LabFile = require("PSDToolKit.LabFile")
local voice_states = require("PSDToolKit.VoiceStates")
local overwriter_states = require("PSDToolKit.OverwriterStates")

-- Audio level cache (cleared each frame when context is created)
local audio_cache = {}

--- Calculate audio level from pre-captured fourier data in Voice object.
-- @param voice table: Voice object with fourier_data, fourier_sample_rate, fourier_n
-- @param locut number: Low frequency cutoff (Hz)
-- @param hicut number: High frequency cutoff (Hz)
-- @return number: Audio level
local function calculate_audio_level_from_voice(voice, locut, hicut)
	if not voice then
		return 0
	end

	local buf = voice.fourier_data
	local sample_rate = voice.fourier_sample_rate
	local n = voice.fourier_n

	if not buf or not sample_rate or not n or n == 0 then
		return 0
	end

	-- Calculate frequency per bin
	local freq_per_bin = sample_rate / 2048

	-- Calculate bin range for locut/hicut
	local lo_bin = math.floor(locut / freq_per_bin)
	local hi_bin = math.ceil(hicut / freq_per_bin)

	-- Clamp to valid range (1 to n)
	if lo_bin < 1 then
		lo_bin = 1
	end
	if hi_bin > n then
		hi_bin = n
	end

	-- Sum amplitude in the frequency range
	local sum = 0
	local count = 0
	for i = lo_bin, hi_bin do
		sum = sum + math.abs(buf[i] or 0)
		count = count + 1
	end

	if count == 0 then
		return 0
	end

	return sum / count
end

--- Create a context object for state evaluation.
-- @param psd table: The PSD object
-- @param obj table: The AviUtl object
-- @return table: Context object with get_voice and get_audio_level methods
function Context.new(psd, obj)
	-- Clear audio cache for new frame
	audio_cache = {}

	local ctx = {
		obj = obj,
		psd = psd,
	}

	--- Get voice by ID.
	-- @param id string|number: Character ID or layer number (required)
	-- @return table|nil: Voice object or nil
	function ctx:get_voice(id)
		if id == nil then
			error("id is required")
		end
		return voice_states:get(id)
	end

	--- Get audio level from Voice object's pre-captured fourier data.
	-- The fourier data is captured in set_voice() at the correct audio timing.
	-- @param voice_id string|number: Voice ID to look up
	-- @param locut number: Low frequency cutoff (Hz)
	-- @param hicut number: High frequency cutoff (Hz)
	-- @return number: Audio level
	function ctx:get_audio_level(voice_id, locut, hicut)
		if voice_id == nil then
			error("voice_id is required")
		end
		if locut == nil then
			error("locut is required")
		end
		if hicut == nil then
			error("hicut is required")
		end

		-- Create cache key using voice_id
		local key = tostring(voice_id) .. ":" .. tostring(locut) .. ":" .. tostring(hicut)
		if audio_cache[key] then
			return audio_cache[key]
		end

		-- Get voice and calculate level from pre-captured fourier data
		local voice = voice_states:get(voice_id)
		local level = calculate_audio_level_from_voice(voice, locut, hicut)
		audio_cache[key] = level
		return level
	end

	--- Get overwriter values by character ID.
	-- @param id string: Character ID (can be nil or empty for default)
	-- @return table|nil: Overwriter values table {p1=number, ...}, or nil if not found
	function ctx:get_overwriter(id)
		return overwriter_states:get(id)
	end

	--- Get phoneme info from lab file at current voice time.
	-- The lab file path is derived from the voice's audio path by replacing
	-- the extension with .lab.
	-- @param voice_id string|number: Voice ID to look up
	-- @return table|nil: Phoneme info from LabFile.get_phoneme_at(), or nil if
	--   voice not found, no audio path, or lab file not found/parseable.
	--   See LabFile.get_phoneme_at() for return value structure.
	function ctx:get_phoneme(voice_id)
		local debug = require("PSDToolKit.debug")
		local dbg = debug.dbg

		if voice_id == nil then
			error("voice_id is required")
		end

		local voice = voice_states:get(voice_id)
		if not voice then
			dbg("get_phoneme: voice not found for id=%s", tostring(voice_id))
			return nil
		end
		if not voice.audio or voice.audio == "" then
			dbg("get_phoneme: voice.audio is empty for id=%s", tostring(voice_id))
			return nil
		end

		-- Convert audio path to lab path (replace extension with .lab)
		local lab_path = voice.audio:gsub("%.[^.]+$", ".lab")
		dbg("get_phoneme: lab_path=%s time=%.4f", lab_path, voice.time or -1)

		-- Get phoneme at voice's current time
		local result = LabFile.get_phoneme_at(lab_path, voice.time)
		if not result then
			dbg("get_phoneme: LabFile.get_phoneme_at returned nil")
		end
		return result
	end

	return ctx
end

return Context
