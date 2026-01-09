--- SubObjectStates for PSDToolKit
-- Manages sub-object timing information storage and retrieval by character ID or layer number.
-- This is used by animation effects (like bounce) that need timing info from voice or overwriter objects.
local CharacterID = require("PSDToolKit.CharacterID")

local SubObjectStates = {}
SubObjectStates.__index = SubObjectStates

function SubObjectStates.new()
	return setmetatable({
		states = {},
	}, SubObjectStates)
end

--- Clears all sub-object states.
function SubObjectStates:clear()
	self.states = {}
end

--- Sets the sub-object state for a given ID and layer.
-- @param id string: The character identifier. Can be nil or empty. Multiple IDs can be specified separated by commas.
-- @param layer number: The layer number.
-- @param obj table: The AviUtl object containing time, frame, totaltime, totalframe.
function SubObjectStates:set(id, layer, obj)
	local state = {
		time = obj.time,
		frame = obj.frame,
		totaltime = obj.totaltime,
		totalframe = obj.totalframe,
	}
	CharacterID.set(self.states, id, layer, state)
end

--- Gets the sub-object state for a given ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The identifier for the sub-object state.
-- @return table|nil: The sub-object state {time, frame, totaltime, totalframe}, or nil if not found.
function SubObjectStates:get(id)
	return CharacterID.get(self.states, id)
end

local singleton = SubObjectStates.new()
return singleton
