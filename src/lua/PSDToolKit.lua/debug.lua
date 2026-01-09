local M = {}

local is_debug = false

--- Update debug mode
-- @param debug boolean Enable/disable debug mode
function M.set_debug(debug)
	is_debug = debug
end

--- Debug print function with lazy formatting
-- Only formats and prints if debug mode is enabled.
-- Usage: local dbg = debug.dbg
--        dbg("format string %s %d", arg1, arg2)
--        dbg("simple message")
-- @param fmt string Format string (passed to string.format if additional arguments exist)
-- @param ... Additional arguments for string.format
function M.dbg(fmt, ...)
	if is_debug then
		if select("#", ...) > 0 then
			debug_print(string.format(fmt, ...))
		else
			debug_print(fmt)
		end
	end
end

return M
