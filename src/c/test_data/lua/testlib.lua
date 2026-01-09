--- Simple test framework for Lua, inspired by Acutest/ovtest
-- Provides TEST_CHECK, TEST_MSG style testing similar to C tests

local M = {}

-- Test state
local current_test_failed = false
local last_check_passed = true
local total_tests = 0
local failed_tests = 0
local test_list = {}

--- Register a test function
-- @param name string: Test name
-- @param fn function: Test function
function M.TEST(name, fn)
	table.insert(test_list, { name = name, fn = fn })
end

--- Check a condition (does not abort on failure)
-- @param condition boolean: Condition to check
-- @return boolean: The condition value
function M.TEST_CHECK(condition)
	last_check_passed = condition
	if not condition then
		current_test_failed = true
		local info = debug.getinfo(2, "Sl")
		io.write(string.format("  %s:%d: CHECK FAILED\n", info.short_src, info.currentline))
	end
	return condition
end

--- Print additional message for last failed check
-- Only prints if the last TEST_CHECK failed
-- @param msg string: Message format
-- @param ...: Format arguments
function M.TEST_MSG(msg, ...)
	if not last_check_passed then
		io.write(string.format("    %s\n", string.format(msg, ...)))
	end
end

--- Run a single test
-- @param test table: Test entry {name, fn}
-- @return boolean: true if test passed
local function run_test(test)
	current_test_failed = false
	last_check_passed = true

	io.write(string.format("Test %s...", test.name))
	io.flush()

	local ok, err = pcall(test.fn)
	if not ok then
		current_test_failed = true
		io.write(string.format("\n  Unexpected error: %s\n", tostring(err)))
	end

	if current_test_failed then
		io.write(" [ FAILED ]\n")
		return false
	else
		io.write(" [ OK ]\n")
		return true
	end
end

--- Run all registered tests
-- @return boolean: true if all tests passed
function M.run_all()
	total_tests = #test_list
	failed_tests = 0

	for _, test in ipairs(test_list) do
		if not run_test(test) then
			failed_tests = failed_tests + 1
		end
	end

	if failed_tests == 0 then
		io.write(string.format("SUCCESS: All %d tests passed.\n", total_tests))
		return true
	else
		io.write(string.format("FAILED: %d of %d tests failed.\n", failed_tests, total_tests))
		return false
	end
end

--- Get exit code based on test results
-- @return number: 0 if all passed, 1 if any failed
function M.exit_code()
	return failed_tests > 0 and 1 or 0
end

return M
