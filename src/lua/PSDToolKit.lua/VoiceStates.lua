--- VoiceStates for PSDToolKit
-- Manages voice state storage and retrieval by ID or layer number.
local CharacterID = require("PSDToolKit.CharacterID")

local VoiceStates = {}
VoiceStates.__index = VoiceStates

function VoiceStates.new()
	return setmetatable({
		states = {},
	}, VoiceStates)
end

--- Clears all voice states.
function VoiceStates:clear()
	self.states = {}
end

--- Sets the voice state for a given ID.
-- You can also search by layer number even if the id is omitted.
-- @param id string: The identifier for the voice state. Can be nil or empty. Multiple IDs can be specified separated by commas.
-- @param layer number: The layer number.
-- @param v Voice: The voice state object to set.
function VoiceStates:set(id, layer, v)
	CharacterID.set(self.states, id, layer, v)
end

--- Gets the voice state for a given ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The identifier for the voice state.
-- @return table: The voice state object, or nil if not found.
function VoiceStates:get(id)
	return CharacterID.get(self.states, id)
end

local singleton = VoiceStates.new()
return singleton
