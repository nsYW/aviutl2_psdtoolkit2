--- VoiceStates for PSDToolKit
-- Manages voice state storage and retrieval by ID or layer number.
-- States are organized by frame number to support multi-frame state management.
local CharacterID = require("PSDToolKit.CharacterID")
local FrameState = require("PSDToolKit.FrameState")

local VoiceStates = {}
VoiceStates.__index = VoiceStates

function VoiceStates.new()
	return setmetatable({
		states = {}, -- frame_number -> {id_key -> voice}
	}, VoiceStates)
end

--- Clears voice states for a specific frame.
-- @param frame_number number: The frame number to clean up
function VoiceStates:cleanup_frame(frame_number)
	self.states[frame_number] = nil
end

--- Clears all voice states (for cache invalidation).
function VoiceStates:clear_all()
	self.states = {}
end

--- Sets the voice state for a given ID.
-- You can also search by layer number even if the id is omitted.
-- @param id string: The identifier for the voice state. Can be nil or empty. Multiple IDs can be specified separated by commas.
-- @param layer number: The layer number.
-- @param v Voice: The voice state object to set.
function VoiceStates:set(id, layer, v)
	local frame = FrameState.get_current_frame()
	FrameState.track_frame_access(frame)

	if not self.states[frame] then
		self.states[frame] = {}
	end

	CharacterID.set(self.states[frame], id, layer, v)
end

--- Gets the voice state for a given ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The identifier for the voice state.
-- @return table: The voice state object, or nil if not found.
function VoiceStates:get(id)
	local frame = FrameState.get_current_frame()
	local frame_states = self.states[frame]
	if not frame_states then
		return nil
	end
	return CharacterID.get(frame_states, id)
end

local singleton = VoiceStates.new()
return singleton
