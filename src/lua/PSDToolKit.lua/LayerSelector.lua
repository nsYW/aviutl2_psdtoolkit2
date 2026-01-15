--- LayerSelector - Simple layer state selector for PSDToolKit %VERSION% by oov
-- This module selects a layer state from a predefined list based on an index.
-- It is designed to work with layer selector scripts exported from the GUI.
local LayerSelector = {}
LayerSelector.__index = LayerSelector

local debug = require("PSDToolKit.debug")
local dbg = debug.dbg

-- Wrapper for objects with getstate method in exclusive mode.
-- Prepends hide prefix to the result of inner:getstate().
local ExclusiveWrapper = {}
ExclusiveWrapper.__index = ExclusiveWrapper

function ExclusiveWrapper.new(hide_prefix, inner)
	return setmetatable({
		hide_prefix = hide_prefix,
		inner = inner,
	}, ExclusiveWrapper)
end

function ExclusiveWrapper:getstate(ctx)
	local state = self.inner:getstate(ctx)
	if type(state) == "string" and state ~= "" then
		return self.hide_prefix .. " " .. state
	elseif type(state) == "table" then
		-- Array of strings - prepend to each string
		local result = {}
		for i, s in ipairs(state) do
			if type(s) == "string" and s ~= "" then
				result[i] = self.hide_prefix .. " " .. s
			else
				result[i] = s
			end
		end
		return result
	end
	return state
end

--- Check if a layer path has an exclusive marker (*) in the final component.
-- The marker must be at the start of the final path component after the last '/'.
-- Examples:
--   "v1.eye/*01" -> true (marker on final component)
--   "v1.*eye/01" -> false (marker on non-final component)
--   "v1.eye/01" -> false (no marker)
-- @param path string: A single layer path (e.g., "v1.eye/01")
-- @return boolean: true if the final component has the exclusive marker
function LayerSelector.has_exclusive_marker(path)
	-- Find the last '/' to get the final component
	local last_slash = path:match(".*()/")
	local final_component
	if last_slash then
		final_component = path:sub(last_slash + 1)
	else
		-- No slash, check after the visibility prefix (v0. or v1.)
		final_component = path:match("^v[01]%.(.*)$") or path
	end
	-- Check if final component starts with '*'
	return final_component:sub(1, 1) == "*"
end

--- Build exclusive values array from raw values.
-- For each value, prepend hide commands for ALL string values.
-- If any string value has exclusive marker (*), returns raw_values unchanged.
-- Non-string values (objects like Blinker, LipSync) are passed through unchanged.
-- @param raw_values table: Array of layer state strings or objects
-- @return table: Array of transformed layer state strings with exclusive prefix
function LayerSelector.build_exclusive_values(raw_values)
	-- First pass: collect hide strings (v0 converted) and check for markers
	local hide_parts = {}
	for _, value in ipairs(raw_values) do
		if type(value) == "string" then
			-- If any string has exclusive marker, return raw_values unchanged
			if LayerSelector.has_exclusive_marker(value) then
				return raw_values
			end
			local hidden = value:gsub("^v1%.", "v0.")
			table.insert(hide_parts, hidden)
		end
	end

	-- No string values means nothing to do
	if #hide_parts == 0 then
		return raw_values
	end

	local hide_prefix = table.concat(hide_parts, " ")

	-- Build result: for each value, prepend hide prefix if it's a string,
	-- or wrap with ExclusiveWrapper if it's an object with getstate method
	local result = {}
	for i, value in ipairs(raw_values) do
		if type(value) == "string" then
			result[i] = hide_prefix .. " " .. value
		elseif type(value) == "table" and type(value.getstate) == "function" then
			-- Wrap object to prepend hide_prefix to its getstate result
			result[i] = ExclusiveWrapper.new(hide_prefix, value)
		else
			-- Other non-string values pass through unchanged
			result[i] = value
		end
	end
	return result
end

--- Creates a new LayerSelector object.
-- @param values table: Array of layer state strings (already processed with exclusive prefix if needed)
-- @param selected number: Index of the selected layer (1-based, Lua convention)
-- @param opts table|nil: Optional parameters {character_id=string, part_index=number}
-- @return LayerSelector: New LayerSelector object
function LayerSelector.new(values, selected, opts)
	if not values then
		error("values cannot be nil")
	end
	if #values < 1 then
		error("LayerSelector requires at least 1 value")
	end

	opts = opts or {}

	-- Validate and clamp selected index
	local sel = selected or 0
	if sel < 0 then
		sel = 0
	elseif sel > #values then
		sel = #values
	end

	dbg(
		"LayerSelector.new: values=%d selected=%d part_index=%s character_id=%s",
		#values,
		sel,
		tostring(opts.part_index),
		tostring(opts.character_id)
	)

	return setmetatable({
		values = values,
		selected = sel,
		character_id = opts.character_id or "",
		part_index = opts.part_index or 0,
	}, LayerSelector)
end

--- Gets the current layer state.
-- This method is called from PSD:buildlayer() during animation rendering.
-- Checks for overwriter values and applies them if available.
-- @param ctx table: Context object with get_overwriter method
-- @return string: The selected layer state string
function LayerSelector:getstate(ctx)
	local sel = self.selected

	-- Check for overwriter values
	if self.part_index > 0 then
		local overwriter = ctx:get_overwriter(self.character_id)
		if overwriter then
			local key = "p" .. tostring(self.part_index)
			local override_value = overwriter[key]
			if override_value and override_value >= 1 and override_value <= #self.values then
				dbg("LayerSelector:getstate: overwrite %s from %d to %d", key, sel, override_value)
				sel = override_value
			end
		end
	end

	local state = self.values[sel]

	-- If state is an object with getstate method (e.g., Blinker, LipSync),
	-- call its getstate to get the actual layer string
	if type(state) == "table" and type(state.getstate) == "function" then
		state = state:getstate(ctx)
	end

	dbg("LayerSelector:getstate: selected=%d state=%s", sel, tostring(state))
	return state
end

return LayerSelector
