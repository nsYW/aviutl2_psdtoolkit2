local Voice = {}
Voice.__index = Voice

function Voice.new(text, audio, x, y, z, frame, time, totalframe, totaltime, obj_id)
	return setmetatable({
		text = text,
		audio = audio,
		x = x,
		y = y,
		z = z,
		frame = frame,
		time = time,
		totalframe = totalframe,
		totaltime = totaltime,
		obj_id = obj_id, -- Object ID for cache invalidation
		-- Fourier data for current frame
		fourier_data = nil,
		fourier_sample_rate = nil,
	}, Voice)
end

return Voice
