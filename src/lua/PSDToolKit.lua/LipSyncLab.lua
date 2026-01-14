--- LipSyncLab - Vowel-based lip sync (aiueo)
-- This module provides lip sync animation based on Japanese vowel patterns.
-- Supports lab file phoneme timing data with multiple consonant processing modes.
local LipSyncLab = {}
LipSyncLab.__index = LipSyncLab

local debug = require("PSDToolKit.debug")
local dbg = debug.dbg

-- Per-layer state cache for smooth animation (used by mode 1)
local states = {}

--- Clear the per-layer state cache.
-- Called when project is loaded or cache is cleared.
function LipSyncLab.clear()
	states = {}
end

-- Vowel set for quick lookup
local vowels = {
	a = true,
	i = true,
	u = true,
	e = true,
	o = true,
	A = true,
	I = true,
	U = true,
	E = true,
	O = true,
}

--- Check if phoneme is a vowel
-- @param p string: Phoneme name
-- @return number: 1 if normal vowel, -1 if unvoiced vowel (uppercase), 0 if not vowel
local function isvowel(p)
	if p == "a" or p == "e" or p == "i" or p == "o" or p == "u" then
		return 1
	end
	if p == "A" or p == "E" or p == "I" or p == "O" or p == "U" then
		return -1 -- Unvoiced vowel
	end
	return 0
end

-- Mode 3: Articulation-based mouth shape mapping
-- Maps consonants to the closest vowel shape based on articulation position
local articulation_map = {
	-- Vowels (use as-is, map uppercase to lowercase for pattern lookup)
	a = "a",
	i = "i",
	u = "u",
	e = "e",
	o = "o",
	A = "a",
	I = "i",
	U = "u",
	E = "e",
	O = "o",

	-- Bilabial (lips closed) -> N
	m = "N",
	p = "N",
	b = "N",

	-- Labiodental (lower lip and upper teeth) -> u
	f = "u",
	v = "u",

	-- Alveolar (tongue tip and alveolar ridge)
	t = "u",
	d = "u",
	n = "u",
	s = "i",
	z = "i",
	r = "u",

	-- Alveopalatal (sh, j, ch sounds) -> i (lips spread)
	sh = "i",
	j = "i",
	ch = "i",
	ts = "u",

	-- Palatal -> i (lips spread)
	y = "i",
	hy = "i",
	ny = "i",
	ky = "i",
	gy = "i",
	by = "i",
	py = "i",
	my = "i",
	ry = "i",

	-- Velar (back of tongue) -> u
	k = "u",
	g = "u",
	w = "u",

	-- Glottal -> u
	h = "u",

	-- Special phonemes -> N (closed)
	N = "N", -- Syllabic nasal (ん)
	cl = "N", -- Geminate consonant (っ)
	pau = "N", -- Pause
	sil = "N", -- Silence
}

-- Fallback for unknown phonemes
local UNKNOWN_FALLBACK = "e"

--- Validate that all required patterns are present.
-- @param pat table: Patterns table
-- @return boolean: true if valid
local function validate(pat)
	if pat.a == "" or pat.i == "" or pat.u == "" or pat.e == "" or pat.o == "" or pat.N == "" then
		error("LipSyncLab requires patterns for all vowels: a, i, u, e, o, N")
	end
	return true
end

local consonant_mode_map = {
	["閉じる"] = 0,
	["前を継続"] = 1,
	["前後で補間"] = 2,
	["調音位置"] = 3,
}
local function normalize_consonant_mode(v)
	if type(v) == "number" then
		return v
	end
	if type(v) == "string" then
		local n = tonumber(v)
		if n ~= nil then
			return n
		end
		return consonant_mode_map[v]
	end
	return nil
end

--- Create a new LipSyncLab object from legacy parameters.
-- This function is called from converted anm scripts.
-- @param pat table: Vowel patterns {a=string, i=string, u=string, e=string, o=string, N=string}
-- @param mode number: Consonant processing mode (0, 1, 2, or 3)
-- @param alwaysapply boolean: Apply even when no voice data is available
-- @return LipSyncLab object
function LipSyncLab.new_legacy(pat, mode, alwaysapply)
	local opts = {
		["あ~ptkl"] = pat.a or "",
		["い~ptkl"] = pat.i or "",
		["う~ptkl"] = pat.u or "",
		["え~ptkl"] = pat.e or "",
		["お~ptkl"] = pat.o or "",
		["ん~ptkl"] = pat.N or "",
		["子音処理"] = tostring(mode or 0),
		["発声がなくても有効"] = alwaysapply and "1" or "0",
	}
	return LipSyncLab.new(opts)
end

--- Creates a new LipSyncLab object.
-- @param opts table: Options table with display name keys
--   Pattern keys (values are layer paths):
--     "あ~ptkl": Pattern for vowel 'a'
--     "い~ptkl": Pattern for vowel 'i'
--     "う~ptkl": Pattern for vowel 'u'
--     "え~ptkl": Pattern for vowel 'e'
--     "お~ptkl": Pattern for vowel 'o'
--     "ん~ptkl": Pattern for closed mouth (N/pause)
--   Numeric parameters (stored as strings):
--     "子音処理": Consonant processing mode (0, 1, 2, or 3)
--       0: All consonants -> N (closed)
--       1: Inherit previous vowel (except for certain consonants)
--       2: Interpolate between adjacent vowels
--       3: Articulation-based (map consonants to closest vowel shape)
--     "発声がなくても有効": Apply even when no voice data is available (0 or 1)
-- @return LipSyncLab: New LipSyncLab object
function LipSyncLab.new(opts)
	if not opts then
		error("opts cannot be nil")
	end

	-- Build patterns table from display name keys
	local pat = {
		a = opts["あ~ptkl"] or "",
		i = opts["い~ptkl"] or "",
		u = opts["う~ptkl"] or "",
		e = opts["え~ptkl"] or "",
		o = opts["お~ptkl"] or "",
		N = opts["ん~ptkl"] or "",
	}

	-- Copy uppercase aliases for unvoiced vowels
	pat.A = pat.a
	pat.I = pat.i
	pat.U = pat.u
	pat.E = pat.e
	pat.O = pat.o

	-- Parse parameters ("子音処理" may come as number, numeric string, or caption)
	local mode = normalize_consonant_mode(opts["子音処理"]) or 0

	-- Convert alwaysapply to boolean (stored as "0" or "1")
	local alwaysapply_val = tonumber(opts["発声がなくても有効"]) or 0
	local alwaysapply = alwaysapply_val ~= 0

	-- Note: locut, hicut, threshold are not used in current LipSyncLab
	-- but kept for fallback volume-based detection
	local locut = 100
	local hicut = 1000
	local threshold = 20

	return setmetatable({
		patterns = pat,
		mode = mode,
		alwaysapply = alwaysapply,
		locut = locut,
		hicut = hicut,
		threshold = threshold,
	}, LipSyncLab)
end

--- Get adjacent phoneme from entries
-- @param entries table: Phoneme entries array
-- @param index number: Current index
-- @param offset number: Offset (-1 for previous, +1 for next)
-- @return table|nil: Adjacent entry or nil if out of bounds
local function get_adjacent(entries, index, offset)
	local adj_index = index + offset
	if adj_index < 1 or adj_index > #entries then
		return nil
	end
	return entries[adj_index]
end

--- Mode 0: All consonants become N (closed)
-- @param cur string: Current phoneme
-- @param pat table: Patterns table
-- @return string: Layer state string
function LipSyncLab:process_mode0(cur, pat)
	if isvowel(cur) ~= 0 then
		return pat[cur] or pat.N
	end
	return pat.N
end

--- Mode 1: Inherit previous vowel (except for certain consonants)
-- @param cur string: Current phoneme
-- @param phoneme_info table: Phoneme info from LabFile
-- @param pat table: Patterns table
-- @param obj table: AviUtl object
-- @param voice_obj_id number: Voice object ID for cache invalidation
-- @return string: Layer state string
function LipSyncLab:process_mode1(cur, phoneme_info, pat, obj, voice_obj_id)
	-- Get or initialize per-layer state
	local stat = states[obj.layer]
	if not stat then
		stat = { frame = obj.frame - 1, p = "N", voice_obj_id = voice_obj_id }
	end

	-- Reset if voice object changed
	if stat.voice_obj_id ~= voice_obj_id then
		stat = { frame = obj.frame - 1, p = "N", voice_obj_id = voice_obj_id }
	end

	-- Reset if time goes backwards or jumps too far forward
	if stat.frame >= obj.frame or stat.frame + obj.framerate < obj.frame then
		stat.p = "N"
	end
	stat.frame = obj.frame

	if isvowel(cur) == 1 then
		-- Normal vowel: use it and remember
		stat.p = cur
	elseif cur == "pau" or cur == "N" or cur == "cl" then
		-- Pause / syllabic nasal / geminate -> closed
		stat.p = "N"
	else
		-- Other consonants: keep previous vowel (stat.p unchanged)
	end

	states[obj.layer] = stat
	return pat[stat.p] or pat.N
end

--- Mode 2: Interpolate between adjacent vowels
-- @param cur string: Current phoneme
-- @param phoneme_info table: Phoneme info from LabFile
-- @param pat table: Patterns table
-- @return string: Layer state string
function LipSyncLab:process_mode2(cur, phoneme_info, pat)
	if isvowel(cur) ~= 0 then
		-- Vowel: use as-is
		return pat[cur] or pat.N
	end

	-- Special cases
	if cur == "pau" or cur == "N" or cur == "m" or cur == "p" or cur == "b" or cur == "v" then
		-- Pause / syllabic nasal / bilabial consonants -> closed
		return pat.N
	end

	local entries = phoneme_info.entries
	local index = phoneme_info.index
	local progress = phoneme_info.progress

	-- Get adjacent entries
	local prev_entry = get_adjacent(entries, index, -1)
	local next_entry = get_adjacent(entries, index, 1)

	if cur == "cl" then
		-- Geminate consonant (っ)
		if progress < 0.5 then
			-- First half: use previous vowel if adjacent
			if prev_entry and isvowel(prev_entry.phoneme) ~= 0 then
				local cur_entry = entries[index]
				if prev_entry.end_ == cur_entry.start then
					return pat[prev_entry.phoneme] or pat.N
				end
			end
			return pat.N
		else
			-- Second half: prepare for next vowel with "u" shape
			return pat.u
		end
	end

	-- Default consonant handling: interpolate based on progress
	if progress < 0.5 then
		-- First half: inherit from previous vowel
		if prev_entry and isvowel(prev_entry.phoneme) ~= 0 then
			local cur_entry = entries[index]
			if prev_entry.end_ == cur_entry.start then
				local prev_p = prev_entry.phoneme
				-- Transition to smaller shape
				if prev_p == "a" or prev_p == "A" then
					return pat.o
				elseif prev_p == "i" or prev_p == "I" then
					return pat.i
				else
					return pat.u
				end
			end
		end
		return pat.N
	else
		-- Second half: prepare for next vowel
		if next_entry and isvowel(next_entry.phoneme) ~= 0 then
			local cur_entry = entries[index]
			if next_entry.start == cur_entry.end_ then
				local next_p = next_entry.phoneme
				-- Transition to smaller shape
				if next_p == "a" or next_p == "A" then
					return pat.o
				elseif next_p == "i" or next_p == "I" then
					return pat.i
				else
					return pat.u
				end
			end
		end
		return pat.N
	end
end

--- Mode 3: Articulation-based mapping
-- @param cur string: Current phoneme
-- @param pat table: Patterns table
-- @return string: Layer state string
function LipSyncLab:process_mode3(cur, pat)
	local mapped = articulation_map[cur]
	if mapped then
		return pat[mapped] or pat.N
	end
	-- Unknown phoneme: use fallback
	return pat[UNKNOWN_FALLBACK] or pat.N
end

--- Gets the current lip state based on lab file phoneme data.
-- Falls back to volume-based detection if lab file is not available.
-- @param ctx table: Context object with get_voice, get_audio_level, and get_phoneme methods
-- @return string: The layer state string for current lip position
function LipSyncLab:getstate(ctx)
	local pat = self.patterns

	validate(pat)

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
			return pat.N -- Return closed state (ん)
		end
		return ""
	end

	-- Always get volume (for fallback and potential future use)
	local volume = ctx:get_audio_level(voice_id, self.locut, self.hicut)

	-- Try to get phoneme from lab file
	local phoneme_info = ctx:get_phoneme(voice_id)

	dbg(
		"LipSyncLab: volume=%.2f threshold=%.2f phoneme=%s mode=%d",
		volume,
		self.threshold,
		phoneme_info and phoneme_info.phoneme or "(nil)",
		self.mode
	)

	if not phoneme_info then
		-- Fallback: volume-based detection
		if volume >= self.threshold then
			return pat.a
		end
		return pat.N
	end

	local cur = phoneme_info.phoneme

	-- Mode dispatch
	if self.mode == 0 then
		return self:process_mode0(cur, pat)
	elseif self.mode == 1 then
		return self:process_mode1(cur, phoneme_info, pat, obj, voice.obj_id)
	elseif self.mode == 2 then
		return self:process_mode2(cur, phoneme_info, pat)
	elseif self.mode == 3 then
		return self:process_mode3(cur, pat)
	end

	error("unexpected consonant processing mode: " .. tostring(self.mode))
end

return LipSyncLab
