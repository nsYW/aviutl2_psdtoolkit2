--- ValueCache - Cache for layer selector values
-- Caches factory function results to avoid rebuilding on every frame.
-- Cache is invalidated when cache_index changes (project load or cache clear).
local ValueCache = {}

-- Internal cache storage
local cache = {}

--- Get or create cached value.
-- @param effect_id number: The effect ID from obj.effect_id
-- @param index number: Cache index within the effect (1, 2, 3, ...)
-- @param factory function: Function that returns the value
-- @return any: The cached or newly created value
function ValueCache.get(effect_id, index, factory)
	local key = tostring(effect_id) .. ":" .. tostring(index)
	local cached = cache[key]
	if cached then
		return cached
	end
	cached = factory()
	cache[key] = cached
	return cached
end

--- Clear the cache.
-- Called when cache_index changes (project load or cache clear).
function ValueCache.clear()
	cache = {}
end

return ValueCache
