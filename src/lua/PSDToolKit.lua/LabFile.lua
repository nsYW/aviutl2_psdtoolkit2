--- LabFile - LAB file parser with LRU cache
-- Parses .lab files (phoneme timing data) and caches results.
-- LAB file format: each line contains "start_time end_time phoneme"
-- where times are in 100-nanosecond units (1e7 per second).
local LabFile = {}

-- Time unit: 100 nanoseconds (1e7 per second)
-- Change this value if lab file uses different unit
local TIME_UNIT = 10000000

-- LRU cache
local cache = {} -- {[labpath] = entries}
local cache_order = {} -- oldest first (for LRU eviction)
local MAX_CACHE_SIZE = 4

--- Update LRU order (move to end = most recently used)
-- @param labpath string: Path to lab file
local function touch_cache(labpath)
	-- Remove existing entry from order
	for i = #cache_order, 1, -1 do
		if cache_order[i] == labpath then
			table.remove(cache_order, i)
			break
		end
	end
	-- Add to end (most recently used)
	table.insert(cache_order, labpath)
end

--- Evict oldest cache entry if over limit
local function evict_if_needed()
	while #cache_order > MAX_CACHE_SIZE do
		local oldest = table.remove(cache_order, 1)
		cache[oldest] = nil
	end
end

--- Parse a lab file
-- @param labpath string: Path to .lab file
-- @return table|nil: Array of {start=number, end_=number, phoneme=string}
--   Times are converted to seconds. Returns nil if file cannot be opened.
local function parse_lab_file(labpath)
	local debug = require("PSDToolKit.debug")
	local dbg = debug.dbg

	-- Use script module's read_text_file for proper UTF-8 path handling on Windows
	local ptk = obj.module("PSDToolKit")
	if not ptk then
		dbg("parse_lab_file: PSDToolKit script module not available")
		return nil
	end

	local content = ptk.read_text_file(labpath)
	if content == nil then
		dbg("parse_lab_file: failed to open file: %s", labpath)
		return nil
	end

	local entries = {}
	local line_count = 0
	for line in content:gmatch("[^\r\n]+") do
		line_count = line_count + 1
		local st, ed, p = string.match(line, "([0-9.]+)%s+([0-9.]+)%s+(.+)")
		if st and ed and p then
			-- Trim whitespace from phoneme
			p = p:match("^%s*(.-)%s*$")
			local start_sec = tonumber(st) / TIME_UNIT
			local end_sec = tonumber(ed) / TIME_UNIT
			table.insert(entries, {
				start = start_sec,
				end_ = end_sec,
				phoneme = p,
			})
			if #entries <= 3 then
				dbg(
					"parse_lab_file: entry[%d] raw=%s,%s,%s -> start=%.4f end=%.4f phoneme=%s",
					#entries,
					st,
					ed,
					p,
					start_sec,
					end_sec,
					p
				)
			end
		else
			if line_count <= 3 then
				dbg("parse_lab_file: line %d did not match pattern: [%s]", line_count, line)
			end
		end
	end

	dbg("parse_lab_file: parsed %d entries from %s", #entries, labpath)
	return entries
end

--- Get or parse lab file with caching
-- @param labpath string: Path to .lab file
-- @return table|nil: Cached entries array, or nil if file not found
function LabFile.get_entries(labpath)
	-- Check cache first
	local entries = cache[labpath]
	if entries then
		touch_cache(labpath)
		return entries
	end

	-- Parse file
	entries = parse_lab_file(labpath)
	if entries == nil then
		return nil
	end

	-- Store in cache
	cache[labpath] = entries
	touch_cache(labpath)
	evict_if_needed()

	return entries
end

--- Binary search for phoneme at given time
-- @param entries table: Parsed entries array (sorted by start time)
-- @param time number: Time in seconds
-- @return number|nil: Index of entry containing time (1-based), or nil if not found
local function binary_search(entries, time)
	local lo, hi = 1, #entries
	while lo <= hi do
		local mid = math.floor((lo + hi) / 2)
		local entry = entries[mid]
		if time < entry.start then
			hi = mid - 1
		elseif time >= entry.end_ then
			lo = mid + 1
		else
			-- entry.start <= time < entry.end_
			return mid
		end
	end
	return nil -- Not found (time outside all entries)
end

--- Clear the LRU cache.
function LabFile.clear()
	cache = {}
	cache_order = {}
end

--- Get phoneme info at specified time
-- @param labpath string: Path to .lab file
-- @param time number: Current time in seconds
-- @return table|nil: {
--   index = number,      -- 1-based index in entries
--   phoneme = string,    -- Current phoneme name
--   start = number,      -- Start time (seconds)
--   end_ = number,       -- End time (seconds)
--   duration = number,   -- Duration (seconds)
--   progress = number,   -- Progress within phoneme (0.0-1.0)
--   entries = table,     -- Reference to cached entries array
-- }
-- Returns nil if file not found or time is outside all entries.
function LabFile.get_phoneme_at(labpath, time)
	local debug = require("PSDToolKit.debug")
	local dbg = debug.dbg

	local entries = LabFile.get_entries(labpath)
	if entries == nil then
		dbg("get_phoneme_at: entries is nil for %s", labpath)
		return nil
	end
	if #entries == 0 then
		dbg("get_phoneme_at: entries is empty for %s", labpath)
		return nil
	end

	dbg(
		"get_phoneme_at: searching time=%.4f in %d entries (range %.4f - %.4f)",
		time,
		#entries,
		entries[1].start,
		entries[#entries].end_
	)

	local index = binary_search(entries, time)
	if index == nil then
		dbg("get_phoneme_at: binary_search returned nil (time outside range)")
		return nil
	end

	local entry = entries[index]
	local duration = entry.end_ - entry.start
	local progress = 0
	if duration > 0 then
		progress = (time - entry.start) / duration
	end

	dbg("get_phoneme_at: found index=%d phoneme=%s", index, entry.phoneme)

	return {
		index = index,
		phoneme = entry.phoneme,
		start = entry.start,
		end_ = entry.end_,
		duration = duration,
		progress = progress,
		entries = entries,
	}
end

return LabFile
