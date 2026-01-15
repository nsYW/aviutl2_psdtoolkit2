--- FrameState - Per-frame state management (singleton)
-- Manages frame-level state such as initialization status and error state.
-- Also tracks recently accessed frames for multi-frame state management.
-- This module is used by init.lua and other modules that need to check
-- whether the current frame has been properly initialized.
local FrameState = {}

local initialized = false
local has_error = false

-- Frame tracking for multi-frame state management
local MAX_RECENT_FRAMES = 4
local recent_frames = {} -- Array of {frame=number, access_order=number}
local access_counter = 0 -- Monotonically increasing counter for LRU-like ordering

--- Clear frame state for a new frame.
-- Called at the beginning of each frame processing.
function FrameState.clear()
	initialized = true
	has_error = false
end

--- Check if the frame has been initialized.
-- @return boolean: true if new_frame() has been called for this frame
function FrameState.is_initialized()
	return initialized
end

--- Check if an error has occurred during frame processing.
-- @return boolean: true if an error has been set
function FrameState.has_error()
	return has_error
end

--- Set the error state.
-- Once set, has_error() will return true until the next clear().
function FrameState.set_error()
	has_error = true
end

--- Get the current project frame number.
-- This returns the absolute frame number in the project (not object-relative).
-- @return number: The current frame number
function FrameState.get_current_frame()
	return obj.getvalue("frame_s") + obj.frame
end

--- Track that a frame was accessed (for data registration).
-- This is called when data is registered for a specific frame.
-- @param frame_number number: The frame number that was accessed
function FrameState.track_frame_access(frame_number)
	access_counter = access_counter + 1

	-- Check if frame already tracked, update access order
	for _, entry in ipairs(recent_frames) do
		if entry.frame == frame_number then
			entry.access_order = access_counter
			return
		end
	end

	-- Add new frame
	table.insert(recent_frames, {
		frame = frame_number,
		access_order = access_counter,
	})
end

--- Get frames to cleanup (frames not in the most recent MAX_RECENT_FRAMES).
-- Returns and removes stale frame entries from tracking.
-- @return table: Array of frame numbers to clean up
function FrameState.get_frames_to_cleanup()
	if #recent_frames <= MAX_RECENT_FRAMES then
		return {}
	end

	-- Sort by access_order (oldest first)
	table.sort(recent_frames, function(a, b)
		return a.access_order < b.access_order
	end)

	-- Collect frames to remove (all but the last MAX_RECENT_FRAMES)
	local to_cleanup = {}
	local remove_count = #recent_frames - MAX_RECENT_FRAMES
	for i = 1, remove_count do
		table.insert(to_cleanup, recent_frames[i].frame)
	end

	-- Remove from tracking
	for _ = 1, remove_count do
		table.remove(recent_frames, 1)
	end

	return to_cleanup
end

--- Clear all frame tracking data.
-- Called when cache is invalidated (project load or cache clear).
function FrameState.clear_all_frames()
	recent_frames = {}
	access_counter = 0
end

return FrameState
