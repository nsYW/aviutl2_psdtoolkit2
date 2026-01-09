--- OverwriterStates for PSDToolKit
-- Manages layer selector overwriter storage and retrieval by character ID or layer number.
local CharacterID = require("PSDToolKit.CharacterID")

local OverwriterStates = {}
OverwriterStates.__index = OverwriterStates

function OverwriterStates.new()
	return setmetatable({
		states = {},
	}, OverwriterStates)
end

--- Clears all overwriter states.
function OverwriterStates:clear()
	self.states = {}
end

--- Merge values into existing state or create new state.
-- @param key string: The state key
-- @param values table: The overwriter values table {p1=number, p2=number, ...}
local function merge_values(states, key, values)
	local existing = states[key]
	if existing then
		-- Merge: later values override earlier ones
		for k, v in pairs(values) do
			existing[k] = v
		end
	else
		-- First assignment: copy values
		local copy = {}
		for k, v in pairs(values) do
			copy[k] = v
		end
		states[key] = copy
	end
end

--- Sets the overwriter state for a given character ID and layer.
-- Multiple calls with the same ID will merge values (later values win).
-- You can also search by layer number even if the id is omitted.
-- @param id string: The character identifier. Can be nil or empty. Multiple IDs can be specified separated by commas.
-- @param layer number: The layer number.
-- @param values table: The overwriter values table {p1=number, p2=number, ...}
function OverwriterStates:set(id, layer, values)
	CharacterID.set_callback(id, layer, function(key)
		merge_values(self.states, key, values)
	end)
end

--- Gets the overwriter state for a given character ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The character identifier or layer specifier (e.g., "L1").
-- @return table|nil: The overwriter values table, or nil if not found.
function OverwriterStates:get(id)
	return CharacterID.get(self.states, id)
end

local singleton = OverwriterStates.new()
return singleton
