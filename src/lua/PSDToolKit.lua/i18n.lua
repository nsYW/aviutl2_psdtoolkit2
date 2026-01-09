--- i18n module for PSDToolKit %VERSION% by oov
local M = {}

-- Cached preferred language list
local preferred_langs = nil

--- Get preferred languages from script module (cached)
-- @return table: Array of language codes (e.g., {"ja_JP", "en_US"})
local function get_preferred_languages()
	if preferred_langs then
		return preferred_langs
	end
	local ptk = obj.module("PSDToolKit")
	if ptk and ptk.get_preferred_languages then
		preferred_langs = ptk.get_preferred_languages()
		if not preferred_langs or #preferred_langs == 0 then
			preferred_langs = { "en_US" }
		end
	else
		preferred_langs = { "en_US" }
	end
	return preferred_langs
end

--- Extract primary language from language code
-- @param code string: Language code (e.g., "ja_JP")
-- @return string: Primary language (e.g., "ja")
local function get_primary_lang(code)
	return string.match(code, "^([^_]+)")
end

--- Select the best matching text from a table based on system language.
-- @param tbl table: Keys are language codes (ja_JP, en_US, etc.), values are text
-- @param override string|nil: Optional language code to override system preference
-- @return string|nil: Selected text
function M.i18n(tbl, override)
	if type(tbl) ~= "table" then
		error("i18n requires a table argument")
	end

	local langs = get_preferred_languages()

	-- If override is provided, prepend it to the language list
	if override and type(override) == "string" then
		local override_langs = { override }
		for _, v in ipairs(langs) do
			table.insert(override_langs, v)
		end
		langs = override_langs
	end

	-- Exact match
	for _, lang in ipairs(langs) do
		if tbl[lang] then
			return tbl[lang]
		end
	end

	-- Primary language match
	for _, lang in ipairs(langs) do
		local primary = get_primary_lang(lang)
		for key, value in pairs(tbl) do
			if get_primary_lang(key) == primary then
				return value
			end
		end
	end

	-- Fallback: en_US, then first entry
	if tbl.en_US then
		return tbl.en_US
	end

	for _, value in pairs(tbl) do
		return value
	end

	return nil
end

return M
