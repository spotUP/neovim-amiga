-- Run by build/v012/host/lua51-m68kdump: every Lua file given is compiled,
-- dumped in the Amiga's bytecode format, loaded back, and dumped again; the
-- two dumps must be byte-identical and the header the Amiga's. Prints
-- "ok N files" or the failures; exits 1 on any.
local want = '\27Lua\81\0\0\4\4\4\8\0'
local fails, n = 0, 0
for _, path in ipairs(arg) do
  local fn, err = loadfile(path)
  if not fn then
    print('SKIP ' .. path .. ': ' .. err)   -- 5.1 cannot parse it
  else
    n = n + 1
    local d1 = string.dump(fn)
    local g, lerr = loadstring(d1, '=' .. path)
    if d1:sub(1, 12) ~= want then
      print('FAIL header ' .. path); fails = fails + 1
    elseif not g then
      print('FAIL load ' .. path .. ': ' .. lerr); fails = fails + 1
    elseif string.dump(g) ~= d1 then
      print('FAIL redump ' .. path); fails = fails + 1
    end
  end
end
-- and one module run both ways: vim.inspect from source and from the dump
local src = 'vendor012/neovim/runtime/lua/vim/inspect.lua'
local f = io.open(src)
if f then
  f:close()
  local a = assert(loadfile(src))()
  local b = assert(loadstring(string.dump(assert(loadfile(src)))))()
  local v = { 1, 2.5, 'x', { y = true, z = { -3, 1e300 } } }
  if a.inspect(v) ~= b.inspect(v) then
    print('FAIL vim.inspect differs when run from the dump'); fails = fails + 1
  else
    print('ok vim.inspect runs the same from the dump: ' .. b.inspect(v):gsub('%s+', ' '))
  end
end
print((fails == 0 and 'ok ' or 'FAIL ') .. n .. ' files, ' .. fails .. ' failed')
os.exit(fails == 0 and 0 or 1)
