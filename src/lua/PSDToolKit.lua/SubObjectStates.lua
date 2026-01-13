--- SubObjectStates for PSDToolKit
-- Manages sub-object timing information storage and retrieval by character ID or layer number.
-- This is used by animation effects (like bounce) that need timing info from voice or overwriter objects.
-- States are organized by frame number to support multi-frame state management.
local CharacterID = require("PSDToolKit.CharacterID")
local FrameState = require("PSDToolKit.FrameState")

local SubObjectStates = {}
SubObjectStates.__index = SubObjectStates

function SubObjectStates.new()
	return setmetatable({
		states = {}, -- frame_number -> {id_key -> state}
	}, SubObjectStates)
end

--- Clears sub-object states for a specific frame.
-- @param frame_number number: The frame number to clean up
function SubObjectStates:cleanup_frame(frame_number)
	self.states[frame_number] = nil
end

--- Clears all sub-object states (for cache invalidation).
function SubObjectStates:clear_all()
	self.states = {}
end

--- Sets the sub-object state for a given ID and layer.
-- @param id string: The character identifier. Can be nil or empty. Multiple IDs can be specified separated by commas.
-- @param layer number: The layer number.
-- @param obj table: The AviUtl object containing time, frame, totaltime, totalframe.
function SubObjectStates:set(id, layer, obj)
	local frame = FrameState.get_current_frame()
	FrameState.track_frame_access(frame)

	if not self.states[frame] then
		self.states[frame] = {}
	end

	local state = {
		time = obj.time,
		frame = obj.frame,
		totaltime = obj.totaltime,
		totalframe = obj.totalframe,
	}
	CharacterID.set(self.states[frame], id, layer, state)
end

--- Gets the sub-object state for a given ID or layer number.
-- Supports comma-separated IDs for fallback search (returns first found).
-- @param id string: The identifier for the sub-object state.
-- @return table|nil: The sub-object state {time, frame, totaltime, totalframe}, or nil if not found.
function SubObjectStates:get(id)
	local frame = FrameState.get_current_frame()
	local frame_states = self.states[frame]
	if not frame_states then
		return nil
	end
	return CharacterID.get(frame_states, id)
end

local singleton = SubObjectStates.new()
return singleton
