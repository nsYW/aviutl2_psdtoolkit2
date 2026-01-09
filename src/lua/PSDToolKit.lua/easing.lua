--- Easing functions module for PSDToolKit

local easing = {}

function easing.linear(p)
	return p
end

function easing.sineIn(p)
	return 1 - math.cos(p * math.pi / 2)
end

function easing.sineOut(p)
	return math.sin(p * math.pi / 2)
end

function easing.sineInOut(p)
	return -(math.cos(math.pi * p) - 1) / 2
end

function easing.quadIn(p)
	return p * p
end

function easing.quadOut(p)
	return 1 - (1 - p) * (1 - p)
end

function easing.quadInOut(p)
	return p < 0.5 and 2 * p * p or 1 - (-2 * p + 2) ^ 2 / 2
end

function easing.cubicIn(p)
	return p * p * p
end

function easing.cubicOut(p)
	return 1 - (1 - p) ^ 3
end

function easing.cubicInOut(p)
	return p < 0.5 and 4 * p * p * p or 1 - (-2 * p + 2) ^ 3 / 2
end

function easing.quartIn(p)
	return p * p * p * p
end

function easing.quartOut(p)
	return 1 - (1 - p) ^ 4
end

function easing.quartInOut(p)
	return p < 0.5 and 8 * p * p * p * p or 1 - (-2 * p + 2) ^ 4 / 2
end

function easing.quintIn(p)
	return p * p * p * p * p
end

function easing.quintOut(p)
	return 1 - (1 - p) ^ 5
end

function easing.quintInOut(p)
	return p < 0.5 and 16 * p * p * p * p * p or 1 - (-2 * p + 2) ^ 5 / 2
end

function easing.expoIn(p)
	return p == 0 and 0 or 2 ^ (10 * (p - 1))
end

function easing.expoOut(p)
	return p == 1 and 1 or 1 - 2 ^ (-10 * p)
end

function easing.expoInOut(p)
	if p == 0 then
		return 0
	end
	if p == 1 then
		return 1
	end
	return p < 0.5 and 2 ^ (20 * p - 10) / 2 or (2 - 2 ^ (-20 * p + 10)) / 2
end

function easing.backIn(p)
	local c1 = 1.70158
	local c3 = c1 + 1
	return c3 * p * p * p - c1 * p * p
end

function easing.backOut(p)
	local c1 = 1.70158
	local c3 = c1 + 1
	return 1 + c3 * (p - 1) ^ 3 + c1 * (p - 1) ^ 2
end

function easing.backInOut(p)
	local c1 = 1.70158
	local c2 = c1 * 1.525
	if p < 0.5 then
		return ((2 * p) ^ 2 * ((c2 + 1) * 2 * p - c2)) / 2
	else
		return ((2 * p - 2) ^ 2 * ((c2 + 1) * (p * 2 - 2) + c2) + 2) / 2
	end
end

function easing.elasticIn(p)
	if p == 0 then
		return 0
	end
	if p == 1 then
		return 1
	end
	local c4 = (2 * math.pi) / 3
	return -2 ^ (10 * p - 10) * math.sin((p * 10 - 10.75) * c4)
end

function easing.elasticOut(p)
	if p == 0 then
		return 0
	end
	if p == 1 then
		return 1
	end
	local c4 = (2 * math.pi) / 3
	return 2 ^ (-10 * p) * math.sin((p * 10 - 0.75) * c4) + 1
end

function easing.elasticInOut(p)
	if p == 0 then
		return 0
	end
	if p == 1 then
		return 1
	end
	local c5 = (2 * math.pi) / 4.5
	if p < 0.5 then
		return -(2 ^ (20 * p - 10) * math.sin((20 * p - 11.125) * c5)) / 2
	else
		return (2 ^ (-20 * p + 10) * math.sin((20 * p - 11.125) * c5)) / 2 + 1
	end
end

function easing.bounceOut(p)
	local n1 = 7.5625
	local d1 = 2.75
	if p < 1 / d1 then
		return n1 * p * p
	elseif p < 2 / d1 then
		p = p - 1.5 / d1
		return n1 * p * p + 0.75
	elseif p < 2.5 / d1 then
		p = p - 2.25 / d1
		return n1 * p * p + 0.9375
	else
		p = p - 2.625 / d1
		return n1 * p * p + 0.984375
	end
end

function easing.bounceIn(p)
	return 1 - easing.bounceOut(1 - p)
end

function easing.bounceInOut(p)
	if p < 0.5 then
		return (1 - easing.bounceOut(1 - 2 * p)) / 2
	else
		return (1 + easing.bounceOut(2 * p - 1)) / 2
	end
end

return easing
