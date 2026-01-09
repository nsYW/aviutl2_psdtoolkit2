--- Blinker (eye blink animator) for PSDToolKit %VERSION% by oov
local Blinker = {}
Blinker.__index = Blinker

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
		error("Blinker requires at least 2 patterns (closed and open)")
	end
	return true
end

--- Create a new Blinker object from legacy parameters.
-- This function is called from converted anm scripts.
-- @param patterns table: Array of layer state strings (2-5 elements, closed to open)
-- @param interval number: Animation interval in seconds (default: 4)
-- @param speed number: Animation speed in frames (default: 1)
-- @param offset number: Animation offset (default: 0)
-- @return Blinker object
function Blinker.new_legacy(patterns, interval, speed, offset)
	local opts = {
		["閉じ~ptkl"] = patterns[1],
		["開き~ptkl"] = patterns[#patterns],
		["間隔(秒)"] = tostring(interval or 4),
		["速さ"] = tostring(speed or 1),
		["オフセット"] = tostring(offset or 0),
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
	return Blinker.new(opts)
end

--- Create a new Blinker object.
-- @param opts table: Options table with display name keys
--   Pattern keys (values are layer paths):
--     "閉じ~ptkl": Closed state
--     "ほぼ閉じ~ptkl": Almost closed state (optional)
--     "半開き~ptkl": Half open state (optional)
--     "ほぼ開き~ptkl": Almost open state (optional)
--     "開き~ptkl": Open state
--   Numeric parameters (stored as strings):
--     "間隔(秒)": Animation interval in seconds (default: 4)
--     "速さ": Animation speed (frames per pattern) (default: 1)
--     "オフセット": Animation start offset (default: 0)
-- @return Blinker object
function Blinker.new(opts)
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

	if #patterns > 3 then
		-- Insert "almost open" at the beginning for asymmetric animation:
		-- Original patterns: closed, almost_closed, half, almost_open, open
		-- After insert: almost_open, closed, almost_closed, half, almost_open, open
		-- This creates: open -> almost_open -> closed -> almost_closed -> half -> almost_open -> open
		-- The quick transition from almost_open to closed makes the blink look natural
		table.insert(patterns, 1, patterns[#patterns - 1])
	end

	-- Parse numeric parameters (stored as strings)
	local interval = tonumber(opts["間隔(秒)"]) or 4
	local speed = tonumber(opts["速さ"]) or 1
	local offset = tonumber(opts["オフセット"]) or 0

	return setmetatable({
		patterns = patterns,
		interval = interval,
		speed = speed,
		offset = offset,
	}, Blinker)
end

--- Get current layer state for animation.
-- @param ctx table: Context object with obj and psd properties
-- @return string: The current pattern string
function Blinker:getstate(ctx)
	validate(self.patterns)
	local obj = ctx.obj
	local interval = self.interval * obj.framerate + self.speed * #self.patterns * 2
	local basetime = obj.frame + interval + self.offset
	local blink = basetime % interval
	local blink2 = (basetime + self.speed * #self.patterns) % (interval * 5)
	for i, v in ipairs(self.patterns) do
		local l = self.speed * i
		local r = l + self.speed
		if (l <= blink and blink < r) or (l <= blink2 and blink2 < r) then
			return v
		end
	end
	return self.patterns[#self.patterns]
end

return Blinker
