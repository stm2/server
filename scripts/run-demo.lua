-- Basic test without loading XML Config. Test care about needed settings.
-- Tests are under scripts/test/ and all files must be in scripts/test/init.lua

math.randomseed(rng.random())

path = 'scripts'
if config.install then
    path = config.install .. '/' .. path
end
package.path = package.path .. ';' .. path .. '/?.lua;' .. path .. '/?/init.lua'

config.rules = 'e2'

require 'eressea'
require 'eressea.path'
require 'eressea.xmlconf' -- read xml data


local rules = {}
if config.rules then
    rules = require('eressea.' .. config.rules)
    eressea.log.info('loaded ' .. #rules .. ' modules for ' .. config.rules)
else
    eressea.log.warning('no rule modules loaded, specify a game in eressea.ini or with -r')
end

eressea.settings.set("nmr.timeout", "0")
-- eressea.settings.set("NewbieImmunity", "0")

demo = require 'demo'

-- turn_begin()
-- callbacks(rules, 'init')
local ok, result = xpcall(demo.create_demo, debug.traceback)
if not ok then
  print(result)
end
--turn_process()
-- callbacks(rules, 'update')
  -- turn_end() -- ageing, etc.
print("done")

local file = 'demo_report.dat'
if eressea.write_game(file)~=0 then
  eressea.log.error("could not write game")
  return -1
end
return 0
