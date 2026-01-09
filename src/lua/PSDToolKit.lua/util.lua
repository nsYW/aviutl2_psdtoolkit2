local M = {}

--- Gets the byte offset for the nth UTF-8 character in a string.
-- @param s string: The input UTF-8 string.
-- @param pos number: The character position (1-based, positive only).
-- @return number: The byte offset of the character, or #s + 1 if out of bounds.
local function utf8_offset(s, pos)
	if pos == 1 then
		return 1
	end
	local char_count = 0
	local i = 1
	while i <= #s and char_count < pos - 1 do
		local c = s:byte(i)
		if c < 128 then
			i = i + 1
		elseif c < 224 then
			i = i + 2
		elseif c < 240 then
			i = i + 3
		else
			i = i + 4
		end
		char_count = char_count + 1
	end
	if char_count == pos - 1 then
		return i
	else
		return #s + 1
	end
end

--- Gets a substring of a UTF-8 encoded string.
-- @param s string: The input UTF-8 string.
-- @param i number: The starting character position (1-based, positive only).
-- @param j number: The ending character position (1-based, positive only, -1 for end of string, defaults to -1).
-- @return string: The extracted substring, or nil if the input is not valid UTF-8.
local function utf8_sub(s, i, j)
	if not s then
		return nil
	end
	i = i or 1
	j = j or -1
	if i < 1 then
		return ""
	end
	if j ~= -1 and (j < 1 or i > j) then
		return ""
	end
	local start_byte = utf8_offset(s, i)
	if not start_byte then
		return nil
	end
	local end_byte = j == -1 and #s + 1 or utf8_offset(s, j + 1)
	if not end_byte then
		return nil
	end
	return s:sub(start_byte, end_byte - 1)
end

--- Gets a truncated version of s.
-- @param s string: The input string.
-- @param length number: The maximum length of the string.
-- @return string: The truncated string with "..." appended if it exceeds the specified length.
function M.truncate_string(s, length)
	local d = utf8_sub(s, 1, length)
	if #s > #d then
		return d .. "..."
	end
	return s
end

return M
