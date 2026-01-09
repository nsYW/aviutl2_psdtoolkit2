--- Animations module for PSDToolKit
local SubObjectStates = require("PSDToolKit.SubObjectStates")
local easing = require("PSDToolKit.easing")
local debug = require("PSDToolKit.debug")
local dbg = debug.dbg

local Animations = {}

local easing_by_index = {
	[0] = easing.linear, -- None (same as linear)
	[1] = easing.linear,
	[2] = easing.sineIn,
	[3] = easing.sineOut,
	[4] = easing.sineInOut,
	[5] = easing.quadIn,
	[6] = easing.quadOut,
	[7] = easing.quadInOut,
	[8] = easing.cubicIn,
	[9] = easing.cubicOut,
	[10] = easing.cubicInOut,
	[11] = easing.quartIn,
	[12] = easing.quartOut,
	[13] = easing.quartInOut,
	[14] = easing.quintIn,
	[15] = easing.quintOut,
	[16] = easing.quintInOut,
	[17] = easing.expoIn,
	[18] = easing.expoOut,
	[19] = easing.expoInOut,
	[20] = easing.backIn,
	[21] = easing.backOut,
	[22] = easing.backInOut,
	[23] = easing.elasticIn,
	[24] = easing.elasticOut,
	[25] = easing.elasticInOut,
	[26] = easing.bounceIn,
	[27] = easing.bounceOut,
	[28] = easing.bounceInOut,
}

--- Applies bounce animation.
-- @param opts table: Options (id, duration, interval, speed, size, vert, horz)
-- @param obj table: The AviUtl object
function Animations.bounce(opts, obj)
	if opts == nil then
		error("opts cannot be nil")
	end
	local id = opts.id
	if id == nil or id == "" then
		return
	end

	local subobj = SubObjectStates:get(id)
	if subobj == nil then
		return
	end

	local duration = opts.duration or 0.30
	local interval = opts.interval or 0.03
	local speed = opts.speed or 0.1
	local size = opts.size or 5
	local vert = opts.vert or 1
	local horz = opts.horz or 1

	local p = math.max(0, math.min(1, (duration - subobj.time + obj.index * interval) / duration))
	if p == 0 or p == 1 then
		return
	end

	local scale = math.sin(math.pi * (subobj.time - obj.index * interval) / speed)
	local z = obj.zoom
	obj.zoom = obj.zoom + scale * size * 0.01 * p

	if horz ~= 1 then
		local ofs = (obj.w * obj.zoom - obj.w * z) * 0.5
		obj.ox = obj.ox + (horz == 0 and ofs or -ofs)
	end
	if vert ~= 1 then
		local ofs = (obj.h * obj.zoom - obj.h * z) * 0.5
		obj.oy = obj.oy + (vert == 0 and ofs or -ofs)
	end
end

--- Applies enter/exit animation.
-- @param opts table: Options
--   id: Character ID
--   speed: Animation duration in seconds (default: 0.30)
--   interval: Delay between characters in seconds (default: 0.03)
--   alpha: Target alpha 0-100 (default: 100)
--   ein: Enter easing 0=None, 1=Linear, 2=SineIn, 3=SineOut, ... (default: 3)
--   eout: Exit easing 0=None, 1=Linear, 2=SineIn, 3=SineOut, ... (default: 2)
--   outside: Behavior when outside SubObject range 0=target, 1=normal (default: 0)
--   x, y, z: Position offset (default: 0)
--   sx, sy: Scale X/Y 0-100 (default: 100)
--   vert, horz: Scale anchor 0=top/left, 1=center, 2=bottom/right (default: 1)
--   rx, ry, rz: Rotation angles (default: 0)
-- @param obj table: The AviUtl object
function Animations.enter_exit(opts, obj)
	if opts == nil then
		error("opts cannot be nil")
	end
	local id = opts.id

	local subobj = nil
	if id ~= nil and id ~= "" then
		subobj = SubObjectStates:get(id)
	end

	local speed = opts.speed or 0.30
	local interval = opts.interval or 0.03
	local target_alpha = (opts.alpha or 100) / 100
	local ein = opts.ein or 3
	local eout = opts.eout or 2
	local outside = opts.outside or 0

	local x = opts.x or 0
	local y = opts.y or 0
	local z = opts.z or 0

	local sx = (opts.sx or 100) / 100
	local sy = (opts.sy or 100) / 100

	local vert = opts.vert or 1
	local horz = opts.horz or 1

	local rx = opts.rx or 0
	local ry = opts.ry or 0
	local rz = opts.rz or 0

	local ein_func = easing_by_index[ein] or easing.linear
	local eout_func = easing_by_index[eout] or easing.linear

	-- Calculate progress: p=1 normal, p=0 target
	local p
	if subobj == nil then
		p = outside ~= 0 and 0 or 1
	elseif speed > 0 then
		local exit_offset = (obj.num - obj.index - 1) * interval
		local sp = ein == 0 and 1 or 1 - math.max(0, math.min(1, (speed - subobj.time + obj.index * interval) / speed))
		local ep = eout == 0 and 1 or math.max(0, math.min(1, (subobj.totaltime - subobj.time - exit_offset) / speed))
		p = sp * ep
		dbg("enter_exit: sp=%f ep=%f p=%f speed=%f interval=%f", sp, ep, p, speed, interval)
		if p > 0 and p < 1 then
			if sp < 1 then
				p = ein_func(p)
			else
				p = eout_func(p)
			end
		end
	else
		p = 1
	end

	obj.alpha = math.max(0, math.min(1, obj.alpha * p + obj.alpha * target_alpha * (1 - p)))
	obj.ox = obj.ox + x * (1 - p)
	obj.oy = obj.oy + y * (1 - p)
	obj.oz = obj.oz + z * (1 - p)

	local old_sx, old_sy = obj.sx, obj.sy
	obj.sx = obj.sx * p + obj.sx * sx * (1 - p)
	obj.sy = obj.sy * p + obj.sy * sy * (1 - p)

	if horz ~= 1 then
		local ofs = obj.w * (old_sx - obj.sx) * 0.5
		obj.ox = obj.ox + (horz == 0 and -ofs or ofs)
	end
	if vert ~= 1 then
		local ofs = obj.h * (old_sy - obj.sy) * 0.5
		obj.oy = obj.oy + (vert == 0 and -ofs or ofs)
	end

	local old_rx = obj.rx
	obj.rx = obj.rx * p + rx * (1 - p)
	local old_ry = obj.ry
	obj.ry = obj.ry * p + ry * (1 - p)
	local old_rz = obj.rz
	obj.rz = obj.rz * p + rz * (1 - p)

	-- X-axis rotation pivot adjustment (affects oy/oz based on vert)
	if vert ~= 1 and obj.rx ~= old_rx then
		local pivot_y = vert == 0 and obj.h * old_sy * 0.5 or -obj.h * old_sy * 0.5
		local rad = math.rad(obj.rx - old_rx)
		local cos_r, sin_r = math.cos(rad), math.sin(rad)
		local rotated_y = pivot_y * cos_r
		local rotated_z = pivot_y * sin_r
		obj.oy = obj.oy + (rotated_y - pivot_y)
		obj.oz = obj.oz + rotated_z
	end

	-- Y-axis rotation pivot adjustment (affects ox/oz based on horz)
	if horz ~= 1 and obj.ry ~= old_ry then
		local pivot_x = horz == 0 and obj.w * old_sx * 0.5 or -obj.w * old_sx * 0.5
		local rad = math.rad(obj.ry - old_ry)
		local cos_r, sin_r = math.cos(rad), math.sin(rad)
		local rotated_x = pivot_x * cos_r
		local rotated_z = -pivot_x * sin_r
		obj.ox = obj.ox + (rotated_x - pivot_x)
		obj.oz = obj.oz + rotated_z
	end

	-- Z-axis rotation pivot adjustment (affects ox/oy based on horz/vert)
	if (horz ~= 1 or vert ~= 1) and obj.rz ~= old_rz then
		local pivot_x = horz == 0 and obj.w * old_sx * 0.5 or (horz == 2 and -obj.w * old_sx * 0.5 or 0)
		local pivot_y = vert == 0 and obj.h * old_sy * 0.5 or (vert == 2 and -obj.h * old_sy * 0.5 or 0)
		local rad = math.rad(obj.rz - old_rz)
		local cos_r, sin_r = math.cos(rad), math.sin(rad)
		local rotated_x = pivot_x * cos_r - pivot_y * sin_r
		local rotated_y = pivot_x * sin_r + pivot_y * cos_r
		obj.ox = obj.ox + (rotated_x - pivot_x)
		obj.oy = obj.oy + (rotated_y - pivot_y)
	end
end

return Animations
