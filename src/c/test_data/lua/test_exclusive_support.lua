--- Test for exclusive support helper functions in LayerSelector.lua

local test_dir = arg[0]:match("(.*[/\\])") or "./"
package.path = test_dir .. "?.lua;" .. package.path

local T = require("testlib")
local TEST = T.TEST
local TEST_CHECK = T.TEST_CHECK
local TEST_MSG = T.TEST_MSG

-- Get the source path from environment variable
local source_dir = os.getenv("LUA_SOURCE_DIR")
if not source_dir then
	error("LUA_SOURCE_DIR environment variable not set")
end

-- Set up package.path to load PSDToolKit modules
-- PSDToolKit.lua is a folder, so we need to load PSDToolKit.lua/LayerSelector.lua
package.path = source_dir .. "/PSDToolKit.lua/?.lua;" .. package.path

-- Mock PSDToolKit.debug module
package.loaded["PSDToolKit.debug"] = {
	dbg = function() end,
}

local LayerSelector = require("LayerSelector")
local has_exclusive_marker = LayerSelector.has_exclusive_marker
local build_exclusive_values = LayerSelector.build_exclusive_values

--- Helper function to compare arrays
local function array_eq(a, b)
	if type(a) ~= "table" or type(b) ~= "table" then
		return a == b
	end
	if #a ~= #b then
		return false
	end
	for i = 1, #a do
		if a[i] ~= b[i] then
			return false
		end
	end
	return true
end

--- Helper function to convert array to string for display
local function array_to_string(arr)
	if type(arr) ~= "table" then
		return tostring(arr)
	end
	local parts = {}
	for i, v in ipairs(arr) do
		if type(v) == "string" then
			parts[i] = string.format("%q", v)
		else
			parts[i] = "<object>"
		end
	end
	return "{" .. table.concat(parts, ", ") .. "}"
end

-- ============================================================================
-- has_exclusive_marker tests
-- ============================================================================

TEST("has_exclusive_marker_simple_path_no_marker", function()
	TEST_CHECK(has_exclusive_marker("v1.char/eye/01") == false)
	TEST_MSG("v1.char/eye/01 should not have marker")
end)

TEST("has_exclusive_marker_marker_on_final_layer", function()
	TEST_CHECK(has_exclusive_marker("v1.char/eye/*01") == true)
	TEST_MSG("v1.char/eye/*01 should have marker on final layer")
end)

TEST("has_exclusive_marker_marker_on_parent_folder_not_final", function()
	TEST_CHECK(has_exclusive_marker("v1.char/*eye/01") == false)
	TEST_MSG("v1.char/*eye/01 should NOT have marker (marker is on parent)")
end)

TEST("has_exclusive_marker_marker_on_root", function()
	TEST_CHECK(has_exclusive_marker("v1.*char/eye/01") == false)
	TEST_MSG("v1.*char/eye/01 should NOT have marker on final layer")
end)

TEST("has_exclusive_marker_final_layer_is_root_with_marker", function()
	TEST_CHECK(has_exclusive_marker("v1.*layer") == true)
	TEST_MSG("v1.*layer should have marker")
end)

TEST("has_exclusive_marker_final_layer_is_root_without_marker", function()
	TEST_CHECK(has_exclusive_marker("v1.layer") == false)
end)

TEST("has_exclusive_marker_v0_prefix", function()
	TEST_CHECK(has_exclusive_marker("v0.char/eye/*01") == true)
	TEST_MSG("v0 prefix should work the same as v1")
end)

-- ============================================================================
-- build_exclusive_values tests
-- Per PLAN.md: "all string values are hidden first, then selected one is shown"
-- If any string has * marker, returns raw_values unchanged (disabled)
-- ============================================================================

TEST("build_exclusive_values_single_value", function()
	local result = build_exclusive_values({ "v1.eye/01" })
	local expected = { "v0.eye/01 v1.eye/01" }
	TEST_CHECK(array_eq(result, expected))
	TEST_MSG("Expected %s, got %s", array_to_string(expected), array_to_string(result))
end)

TEST("build_exclusive_values_two_values", function()
	local result = build_exclusive_values({ "v1.eye/01", "v1.eye/02" })
	local expected = {
		"v0.eye/01 v0.eye/02 v1.eye/01",
		"v0.eye/01 v0.eye/02 v1.eye/02",
	}
	TEST_CHECK(array_eq(result, expected))
	TEST_MSG("Expected %s, got %s", array_to_string(expected), array_to_string(result))
end)

TEST("build_exclusive_values_three_values", function()
	local result = build_exclusive_values({ "v1.eye/01", "v1.eye/02", "v1.eye/03" })
	local expected = {
		"v0.eye/01 v0.eye/02 v0.eye/03 v1.eye/01",
		"v0.eye/01 v0.eye/02 v0.eye/03 v1.eye/02",
		"v0.eye/01 v0.eye/02 v0.eye/03 v1.eye/03",
	}
	TEST_CHECK(array_eq(result, expected))
	TEST_MSG("Expected %s, got %s", array_to_string(expected), array_to_string(result))
end)

TEST("build_exclusive_values_with_marker_disabled", function()
	-- If any string has * marker, exclusive support is DISABLED (returns raw_values)
	local input = { "v1.eye/01", "v1.eye/*02" }
	local result = build_exclusive_values(input)
	TEST_CHECK(result == input)
	TEST_MSG("Any marker should disable exclusive support, returning raw_values")
end)

TEST("build_exclusive_values_all_have_markers_disabled", function()
	-- All values have * marker, exclusive support is DISABLED
	local input = { "v1.eye/*01", "v1.eye/*02" }
	local result = build_exclusive_values(input)
	TEST_CHECK(result == input)
	TEST_MSG("All marked values should disable exclusive support")
end)

TEST("build_exclusive_values_marker_on_parent_still_works", function()
	-- * marker on parent folder, not final layer - exclusive support still works
	local result = build_exclusive_values({ "v1.*eye/01", "v1.*eye/02" })
	local expected = {
		"v0.*eye/01 v0.*eye/02 v1.*eye/01",
		"v0.*eye/01 v0.*eye/02 v1.*eye/02",
	}
	TEST_CHECK(array_eq(result, expected))
	TEST_MSG("Marker on parent should still work: expected %s, got %s", array_to_string(expected), array_to_string(result))
end)

TEST("build_exclusive_values_empty_input", function()
	local result = build_exclusive_values({})
	TEST_CHECK(#result == 0)
	TEST_MSG("Empty input should return empty result")
end)

TEST("build_exclusive_values_with_objects_having_getstate", function()
	-- Objects with getstate method should be wrapped with ExclusiveWrapper
	local obj1 = {
		name = "blinker",
		getstate = function(self, ctx)
			return "v1.blink/01"
		end,
	}
	local obj2 = {
		name = "lipsync",
		getstate = function(self, ctx)
			return "v1.lip/open"
		end,
	}
	local result = build_exclusive_values({ "v1.eye/01", obj1, "v1.eye/02", obj2 })
	-- Hide prefix built from strings only: v0.eye/01 v0.eye/02
	-- Strings get prefix, objects with getstate are wrapped
	TEST_CHECK(result[1] == "v0.eye/01 v0.eye/02 v1.eye/01")
	-- obj1 should be wrapped, not the same object
	TEST_CHECK(result[2] ~= obj1)
	TEST_CHECK(type(result[2]) == "table")
	TEST_CHECK(type(result[2].getstate) == "function")
	-- Verify wrapper works correctly
	local wrapped_state = result[2]:getstate({})
	TEST_CHECK(wrapped_state == "v0.eye/01 v0.eye/02 v1.blink/01")
	TEST_MSG("Expected wrapped getstate to return 'v0.eye/01 v0.eye/02 v1.blink/01', got '%s'", tostring(wrapped_state))

	TEST_CHECK(result[3] == "v0.eye/01 v0.eye/02 v1.eye/02")
	-- obj2 should also be wrapped
	TEST_CHECK(result[4] ~= obj2)
	local wrapped_state2 = result[4]:getstate({})
	TEST_CHECK(wrapped_state2 == "v0.eye/01 v0.eye/02 v1.lip/open")
end)

TEST("build_exclusive_values_with_objects_without_getstate", function()
	-- Objects without getstate method should pass through unchanged
	local obj1 = { name = "plain_object" }
	local result = build_exclusive_values({ "v1.eye/01", obj1, "v1.eye/02" })
	-- Hide prefix built from strings only: v0.eye/01 v0.eye/02
	TEST_CHECK(result[1] == "v0.eye/01 v0.eye/02 v1.eye/01")
	TEST_CHECK(result[2] == obj1)
	TEST_CHECK(result[3] == "v0.eye/01 v0.eye/02 v1.eye/02")
	TEST_MSG("Objects without getstate should pass through unchanged")
end)

TEST("build_exclusive_values_only_objects", function()
	-- No string values, nothing to do
	local obj1 = { name = "blinker" }
	local obj2 = { name = "lipsync" }
	local input = { obj1, obj2 }
	local result = build_exclusive_values(input)
	TEST_CHECK(result == input)
	TEST_MSG("Only objects should return raw_values unchanged")
end)

TEST("build_exclusive_values_objects_with_marked_string", function()
	-- If any string has marker, returns raw_values (including objects)
	local obj1 = { name = "blinker" }
	local input = { "v1.eye/*01", obj1, "v1.eye/02" }
	local result = build_exclusive_values(input)
	TEST_CHECK(result == input)
	TEST_MSG("Marker in any string should disable exclusive support")
end)

TEST("build_exclusive_values_wrapper_with_array_return", function()
	-- Test wrapper handles getstate returning array of strings
	local obj1 = {
		name = "multi_state",
		getstate = function(self, ctx)
			return { "v1.layer/a", "v1.layer/b" }
		end,
	}
	local result = build_exclusive_values({ "v1.eye/01", obj1 })
	-- Hide prefix: v0.eye/01
	TEST_CHECK(result[1] == "v0.eye/01 v1.eye/01")
	-- Wrapper should handle array return
	local wrapped_state = result[2]:getstate({})
	TEST_CHECK(type(wrapped_state) == "table")
	TEST_CHECK(wrapped_state[1] == "v0.eye/01 v1.layer/a")
	TEST_CHECK(wrapped_state[2] == "v0.eye/01 v1.layer/b")
	TEST_MSG("Wrapper should prepend hide_prefix to each string in array")
end)

TEST("build_exclusive_values_wrapper_with_empty_return", function()
	-- Test wrapper handles getstate returning empty string
	local obj1 = {
		name = "empty_state",
		getstate = function(self, ctx)
			return ""
		end,
	}
	local result = build_exclusive_values({ "v1.eye/01", obj1 })
	-- Wrapper should pass through empty string unchanged
	local wrapped_state = result[2]:getstate({})
	TEST_CHECK(wrapped_state == "")
	TEST_MSG("Wrapper should pass through empty string unchanged")
end)

-- Run all tests
T.run_all()
os.exit(T.exit_code())
