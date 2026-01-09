--- FrameState - Per-frame state management (singleton)
-- Manages frame-level state such as initialization status and error state.
-- This module is used by init.lua and other modules that need to check
-- whether the current frame has been properly initialized.
local FrameState = {}

local initialized = false
local has_error = false

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

return FrameState
