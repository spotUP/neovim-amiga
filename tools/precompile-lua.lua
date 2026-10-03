-- Run by build/v012/host/lua51-m68kdump: replace each Lua file given with
-- its bytecode in the Amiga's format (same name: Lua's loadfile detects a
-- binary chunk by its signature), so the 68020 does not compile it at every
-- start. Debug info is kept (line numbers in errors). A file Lua 5.1
-- cannot compile is left as source and listed.
local done, kept = 0, 0
for _, path in ipairs(arg) do
  local f = io.open(path, 'rb')
  local src = f:read('*a')
  f:close()
  if src:sub(1, 4) ~= '\27Lua' then
    local fn, err = loadstring(src, '@' .. path:gsub('^.*/runtime/', ''))
    if fn then
      local out = io.open(path, 'wb')
      out:write(string.dump(fn))
      out:close()
      done = done + 1
    else
      print('kept as source: ' .. err)
      kept = kept + 1
    end
  end
end
print(string.format('precompiled %d files, %d kept as source', done, kept))
