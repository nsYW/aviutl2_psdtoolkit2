--- CharacterID utilities for PSDToolKit
-- Provides common functions for converting character IDs and layer numbers to storage keys.
-- Character ID can be:
--   - A named ID like "foo" (stored as "_foo")
--   - A layer number like "L3" (stored as "L3")
--   - Multiple IDs separated by commas like "foo,bar"
local M = {}

--- Converts a single character ID to a storage key for retrieval.
-- @param id string: The character ID (e.g., "foo", "L3"). Must not be nil or empty.
-- @return string: The storage key
local function to_key(id)
	-- Check if id matches "L[0-9]+" pattern (layer number)
	if id:match("^L%d+$") then
		return id
	end
	-- Otherwise, it's a named character ID
	return "_" .. id
end

--- Converts a layer number to a storage key.
-- @param layer number: The layer number
-- @return string: The storage key (e.g., "L3")
local function layer_to_key(layer)
	return "L" .. tostring(layer)
end

--- Iterates over each ID in a comma-separated string and calls the callback with the storage key.
-- @param id string|nil: The character identifier(s), comma-separated
-- @param callback function: Called with each storage key (e.g., "_foo", "_bar")
local function each_id_key(id, callback)
	if id == nil or id == "" then
		return
	end
	for single_id in string.gmatch(id, "([^,]+)") do
		-- Trim whitespace from each id
		single_id = single_id:match("^%s*(.-)%s*$")
		if single_id ~= "" then
			callback(to_key(single_id))
		end
	end
end

--- Gets a value from states by character ID (supports comma-separated fallback search).
-- @param states table: The states table to search
-- @param id string: The character ID(s), comma-separated for fallback search
-- @return any: The first found value, or nil if not found
function M.get(states, id)
	if id == nil or id == "" then
		return nil
	end
	local result = nil
	each_id_key(id, function(key)
		if result == nil then
			result = states[key]
		end
	end)
	return result
end

--- Sets a value in states for both layer key and character ID keys.
-- @param states table: The states table to store into
-- @param id string|nil: The character identifier(s), comma-separated
-- @param layer number: The layer number
-- @param value any: The value to set
function M.set(states, id, layer, value)
	states[layer_to_key(layer)] = value
	each_id_key(id, function(key)
		states[key] = value
	end)
end

--- Calls a callback for each key (layer key and character ID keys).
-- @param id string|nil: The character identifier(s), comma-separated
-- @param layer number: The layer number
-- @param fn function: Called with each storage key
function M.set_callback(id, layer, fn)
	fn(layer_to_key(layer))
	each_id_key(id, fn)
end

return M
