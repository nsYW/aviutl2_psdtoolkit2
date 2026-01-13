--- OverwriterStates for PSDToolKit
-- Manages layer selector overwriter storage and retrieval by character ID or layer number.
-- States are organized by frame number to support multi-frame state management.
local CharacterID = require("PSDToolKit.CharacterID")
local FrameState = require("PSDToolKit.FrameState")

local OverwriterStates = {}
OverwriterStates.__index = OverwriterStates

function OverwriterStates.new()
	return setmetatable({
		states = {}, -- frame_number -> {id_key -> values}
	}, OverwriterStates)
end

--- Clears overwriter states for a specific frame.
-- @param frame_number number: The frame number to clean up
function OverwriterStates:cleanup_frame(frame_number)
	self.states[frame_number] = nil
end

--- Clears all overwriter states (for cache invalidation).
function OverwriterStates:clear_all()
	self.states = {}
end

--- Merge values into existing state or create new state.
-- @param states table: The states table for the current frame
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
	local frame = FrameState.get_current_frame()
	FrameState.track_frame_access(frame)

	if not self.states[frame] then
		self.states[frame] = {}
	end

	local frame_states = self.states[frame]
	CharacterID.set_callback(id, layer, function(key)
		merge_values(frame_states, key, values)
	end)
end

--- Gets the overwriter state for a given character ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The character identifier or layer specifier (e.g., "L1").
-- @return table|nil: The overwriter values table, or nil if not found.
function OverwriterStates:get(id)
	local frame = FrameState.get_current_frame()
	local frame_states = self.states[frame]
	if not frame_states then
		return nil
	end
	return CharacterID.get(frame_states, id)
end

local singleton = OverwriterStates.new()
return singleton
