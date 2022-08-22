-- local lunit = require('lunit')

function rules_tame()
    eressea.settings.set("rules.food.flags", "4") -- 4: food is free
    eressea.settings.set("rules.ship.damage.nocrewocean", "0")
    eressea.settings.set("rules.ship.damage.nocrew", "0")
    eressea.settings.set("rules.ship.drifting", "0")
    eressea.settings.set("rules.ship.storms", "0")
end

function rules_wild()
    eressea.settings.set("NewbieImmunity", "0")
    eressea.settings.set("rules.food.flags", "0") -- 0: default
    eressea.settings.set("rules.ship.damage.nocrewocean", "1")
    eressea.settings.set("rules.ship.damage.nocrew", "1")
    eressea.settings.set("rules.ship.drifting", "1")
    eressea.settings.set("rules.ship.storms", "1")
end

local function max_aura(mskill)
  return math.modf(math.pow(mskill, 2.1)/1.2+1)
end

local function printf(s, ...)
  return io.write(s:format(...))
end

function get_plane_bounds()
    local bounds = {x = -2^32, xmin = 2^32, y = -2^32,ymin = 2^32}
    for r in regions() do
        if r.x > bounds.x then bounds.x = r.x end
        if r.y < bounds.xmin then bounds.xmin = r.x end
        if r.y > bounds.y then bounds.y = r.y end
        if r.y < bounds.ymin then bounds.ymin = r.y end
    end
    if bounds.x < bounds.y then
        bounds.x = 0
        bounds.xmin = 0
        bounds.y = 0
        bounds.ymin = 0
    else
        bounds.x = bounds.x + 100
        bounds.y = bounds.y + 100
        bounds.xmin = bounds.xmin - 100
        bounds.ymin = bounds.ymin - 100
    end
    return bounds
end

local demo_bounds = {}

function get_demo_region(x, y)
    if demo_bounds == nil or demo_bounds.x == nil then error("bounds not initialized") end
    return get_region(demo_bounds.x + x, demo_bounds.y + y)
end

function create_eressea_map()
    demo_bounds = get_plane_bounds()

    local points = {
        {0, 0}, {1 ,0}, {2, 0},
        {4, 0}, {6, 0},
        {8, 0}, {9, 0}, {10, 0},
        {12, 0}, {13, 0}, {14, 0},
        {16, 0}, {17, 0}, {18, 0},
        {20, 0}, {21, 0}, {22, 0},
        {24, 0}, {26, 0},

        {0, 1},
        {4, 1}, {5, 1},
        {8, 1},
        {14, 1},
        {18, 1},
        {20, 1},
        {24, 1}, {26, 1},

        {0,2}, {1,2}, {2, 2},
        {4, 2}, {5, 2}, {6, 2},
        {8, 2}, {9, 2}, {10, 2},
        {13, 2},
        {17, 2},
        {20, 2}, {21, 2}, {22, 2},
        {24, 2}, {25, 2}, {26, 2},

        {0,3},
        {4, 3}, {6, 3},
        {8, 3},
        {12, 3},
        {16, 3},
        {20, 3},
        {24, 3}, {26, 3},

        {0,4}, {1,4}, {2, 4},
        {4, 4}, {5, 4}, {6, 4},
        {8, 4}, {9, 4}, {10, 4},
        {12, 4}, {13, 4}, {14, 4},
        {16, 4}, {17, 4}, {18, 4},
        {20, 4}, {21, 4}, {22, 4},
        {24, 4}, {25, 4}
    }
    local map = {}
    local xmax, ymax = 0, 0
    local types = { 'plain', 'swamp', 'highland', 'volcano', 'glacier' }
    local ntypes = 5
    for _, p in ipairs(points) do
        local x, y = p[1], p[2]
        if map[x] == nil then
            map[x] = {}
        end
        map[x][y] = 1
        xmax = math.max(xmax, x)
        ymax = math.max(ymax, y)
    end
    for x = -5, 2*xmax+5 do
        local x2 = math.modf(x/2)
        if x < 0 then x2 = math.modf(x-1/2) end
        if map[x2] == nil then map[x2] = {} end
        for y = -5, 2*ymax+5 do
            local y2 = math.modf(y/2)
            if y < 0 then y2 = math.modf(y-1/2) end
            local type = 'ocean'
            if map[x2][y2] == 1 then
                type = types[math.random(ntypes)]
    --            elseif (x2 < -1 or y2 < -1 or x2 > xmax+1 or y2 > ymax+1) and math.random(7) == 1 then
            elseif x == -5 or y == -5 or x == 2*xmax+5 or y == 2*ymax+5 then
                type = 'firewall'
            end
            r = region.create(demo_bounds.x + x, demo_bounds.y + y, type)
        end
    end
end

-- types = { 'plain', 'swamp', 'highland', 'volcano', 'glacier' }
habitable = { ['plain'] = true, ['swamp'] = true, ['highland'] = true, ['volcano'] = true, ['glacier'] =true }

local function create_unit(f, r, n, name)
    local u = unit.create(f,r,n)
    u.name = name
    u.id = atoi36(name)
    return u
end


local function magiclines(s)
   if s:sub(-1)~="\n" then s=s.."\n" end
   return s:gmatch("(.-)\n")
end

local function add_orders(u, orders)
   for line in magiclines(orders) do
     u:add_order(line)
   end
end

local function add_order(id, orders)
    local u = get_unit(atoi36(id))
    add_orders(u, orders)
end

local function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end

local demo_units = { ['factions'] = {}, ['numbers'] = {}, ['units'] = {}}

function create_demo_faction(race, email, lang, name, id)
    local f = faction.create(race, email, lang)
    if name ~= NULL then
      f.name = name
    end
    if id ~= NULL then
      f.id = atoi36(id)
    end

    table.insert(demo_units['factions'], f)
    demo_units['numbers'][f]= 0
    demo_units['units'][f] = {}

    return f
end

function create_demo_unit(f, r, number, name, id, skills, items, orders)
  if f == NULL then
      f = demo_units['factions'][1]
  end
  if r == NULL then
      error("no region")
  end
  if number == NULL then number = 1 end
  local u = unit.create(f, r, number)
  if u == NULL then
    error("unit " .. f .. "-" .. r .. "-"..number)
  end
  if name ~= NULL then
    u.name = name
  end
  if id ~= NULL then
    u.id = atoi36(id)
  end
  if skills ~= NULL then
      for skill, level in pairs(skills) do
          u:set_skill(skill, level)
      end
  end
  if items ~= NULL then
      for item, amount in pairs(items) do
          u:add_item(item, amount)
      end
  end
  if orders ~= NULL then
      for _,o in ipairs(orders) do
          u:add_order(o)
      end
  end

  demo_units['numbers'][f] = demo_units['numbers'][f] + 1
  local i = demo_units['numbers'][f]
  demo_units['units'][f][i] = u
  u.hp = u.hp_max * u.number
  return u
end

function demo_module_starters()
    local r0 = get_demo_region(24, 0)
    local f0 = demo_units['factions'][1]
    local friends = demo_units['factions'][2]
    local foes = demo_units['factions'][3]

    u0 = create_demo_unit(f0, r0, 1, 'Einer', 'demo')
    v0 = create_demo_unit(friends, r0, 1, 'Ein Freundy', 'nice', {}, { money = 1000 })
    x0 = create_demo_unit(foes, r0, 1, 'Ein Andery', 'foe', {}, { money = 1000 })

    u0:add_order("HELFE nice ALLES")
    v0:add_order("HELFE demo ALLES")

    u0:add_item("money", 100)
    v0:add_item("money", 100)
    x0:add_item("money", 100)

    for r in regions() do
        r.age = 100
        if (habitable[r.terrain] and r.x < 40) or r.terrain == 'fog' then
            local u = unit.create(f0, r)
            u:add_item("money", 30)
            u:clear_orders()
            u:add_order("ARBEITE")
        end
    end
end

function demo_module_paula(f0)
    local r0 = get_demo_region(24, 0)
    r0.terrain = 'highland'
    r0.name = 'Piratenbucht'

    local sh = ship.create(r0, "trireme")
    sh.size = 190
    sh.name = "Paulas Wellenkreuzer"
    local p = create_demo_unit(f0, r0, 1, 'Piratenpaula', 'pla', { ["sailing"] = 20, ['tactics'] = 10, ['melee'] = 2 }, { ['money'] = 1000}, { 'ROUTE sw sw sw o o o o o o o o o o o o o o ', 'KÄMPFE HINTEN'})
    local pc = create_demo_unit(f0, r0, 70, 'Paulas Crew', 'pla', { ['sailing'] = 14, ['shipcraft'] = 6, ['melee'] = 15 }, {log= 100, sword = 100}, { 'MACHE SCHIFF ' .. itoa36(sh.id) })
    sh.owner = p
    p.ship = sh
    pc.ship = sh
    return p
end

function demo_module_paula2(p)
    local r1 = p.region
    local serpent = unit.create(get_monsters(), r1, 1, "seaserpent")
    serpent:add_order("ATTACKIERE " .. itoa36(p.id) )
end

function demo_module_piratenbucht()
    local r0 = get_demo_region(24, 0)
    local bc = building.create(r0, "castle", 250)
    bc.name = 'Paulas Schloss'

    ------- LIGHTHOUSE
    local bl = building.create(r0, "lighthouse", 1000)
    bl.name = 'Der große bunte Leuchtturm'
    -- function create_demo_unit(f, r, number, name, id, skills, items, orders)
    local p= create_demo_unit(null, r0, 1, 'Peer Weitsicht', 'peer', { perception=20}, {money=1000} )
    p.building = bl

    local bt = building.create(r0, "inn", 10)
    bt.name = 'Zum Rostigen Anker'

    ------- INN
    local heal = create_demo_unit(null, r0, 20, "Verwundete", "vwd", {stamina=10}, {money = 220})
    heal.hp = heal.number * math.floor(heal.hp_max *.72 -1)
    for i = 2, 20 do
        heal:add_order("BETRETE GEBÄUDE " .. itoa36(bt.id))
        heal:add_order("GIB TEMP vw" .. i .. " 1 PERSONEN")
        heal:add_order("GIB TEMP vw" .. i .. " 14 Silber")
        heal:add_order("MACHE TEMP vw" .. i)
        heal:add_order("BENENNE EINHEIT Verwundete")
        heal:add_order("ARBEITE")

        if i%2 == 0 then
            heal:add_order("BETRETE GEBÄUDE " .. itoa36(bt.id))
        end
        heal:add_order("ENDE")
    end


    local teach = create_demo_unit(null, r0, 1, "Lehrer", "leer", {mining = 1, perception = 1, sailing = 1 }, {money = 100})
    teach:add_order("LEHRE stu0 stu1 stu2 stu3 stu4 stu5 stu6 stu7 stu8 stu9")

    for i = 0, 9 do
    local stud = create_demo_unit(null, r0, 1, "Schüler", "stu" .. i)
    if teach:eff_skill('mining') > teach:get_skill('mining') then
        stud:add_order("LERNE Bergbau")
    elseif teach:eff_skill('perception') > teach:get_skill('perception') then
        stud:add_order("LERNE Wahrnehmung")
    elseif teach:eff_skill('sailing') > teach:get_skill('sailing') then
        stud:add_order("LERNE Segeln")
    end
    end
end

function demo_module_mages()
    local r0 = get_demo_region(24, 0)
    local r1 = get_demo_region(0, 8)
    local r2 = get_demo_region(8, 8)

    local schools = { 'cerddor', 'draig', 'gwyrrd', 'illaun', 'tybied'}
    -- FIXME
    schools = { 'draig' }
    local school = schools[math.random(#schools)]
    local mage = create_demo_unit(null, r0, 1, "Mort der Magier", "mort", null, { money=6950 }, {"LERNE Magie " .. school} )
    local mage2, mage3 = create_demo_unit(null, r1, 1, 'Zoé die Zauberin', 'zoe', { }, { money=11700 }, {"LERNE Magie " .. school} )
    local mage3 = create_demo_unit(null, r2, 1, 'Momo', 'momo', { }, { money=6950 }, {"LERNE Magie " .. school} )
    mage.magic = school
    mage:set_skill('magic', 15)
    mage2.magic = school
    mage2:set_skill('magic', 20)
    mage3.magic = school
    mage3:set_skill('magic', 15)

end

function try_cerddor(mage1, mage2, mage3)
    local r = get_demo_region(48, 0)
    create_demo_unit(demo_units['factions'][3], r)
    mage1:add_order("ZAUBERE STUFE 2 Regentanz")
    mage2:add_order("ZAUBERE Aushorchen foe 48 0" )
end

function try_draig(mage1, mage2, mage3)
    create_demo_unit(demo_units['factions'][3], mage1.region, 1, 'Ziel', 'wart')
    mage1:add_order("ZAUBERE 'Verwünschung' wart")
    mage1:add_order("ZAUBERE 'Gabe des Chaos'")
    mage1:add_order("ZAUBERE 'Kleines Blutopfer'")
--    mage1:add_order("ZAUBERE STUFE 1 'Traumsenden' nice Träumchen")

   get_demo_region(0,5):set_resource("grave", 1000)
   mage2:add_order("ZAUBERE REGION 0 5 STUFE 10 'Mächte des Todes'")
   mage2:add_order("ZAUBERE STUFE 10 'Mächte des Todes'")

   -- Schattendämonen, Schattenmeister
end


function try_gwyrrd(mage1, mage2, mage3)
    local s1, u1 = create_ship(mage2.region, mage2.faction, 'boat')
    local s2, u2 = create_ship(mage2.region, mage2.faction, 'boat')

    print("xxxxx", mage2.region, s1.name, s1.id, s1.region)

    u1:add_order("NACH so o o o o o o o ")
    u2:add_order("NACH so o o o o o o o ")
    mage2:add_item("rop", 1)
    mage2:add_order("ZAUBERE STUFE 1 Sturmelementar " .. itoa36(s1.id) .. " " .. itoa36(s2.id))

    -- TODO Steinkreis, Ents
end

function try_illaun(mage1, mage2, mage3)
    local r = get_demo_region(2, 0)
    create_demo_unit(demo_units['factions'][2], r, 1, 'Ziel', 'read')
    create_demo_unit(demo_units['factions'][2], r, 1, 'Versteckt', null, { stealth = 100 })

    mage1:add_order("ZAUBERE REGION 2 0 'Traumlesen' read")
    mage1:add_order("ZAUBERE STUFE 1 'Traumsenden' nice Träumchen")

    mage2:add_order("ZAUBERE 'Vertrauten rufen'")
    create_demo_unit(demo_units['factions'][2], mage2.region, 1, 'Hier', 'hid0', {stamina =5}, {money = 100})
    mage2:add_order("ZAUBERE 'Traumdeuten' hid0")

    create_demo_unit(null, mage3.region, 1, 'Lehrer', 'leer', {stamina = 20}, {}, {"LEHRE schl"})
    create_demo_unit(null, mage3.region, 1, 'Schüler', 'schl', {stamina = 10}, {}, {"LERNE Ausdauer"})
    mage3:add_order("ZAUBERE 'Schöne Träume'")
end

function try_tybied(mage1, mage2, mage3)
    create_demo_unit(mage2.faction, mage2.region, 1, 'Astralreisender', 'ast', { perception = 20 }, { money = 100})

    mage2:add_order("ZAUBERE STUFE 4 'Astraler Weg' ast")

    create_demo_unit(mage1.faction, mage1.region, 1, 'Hirntöterspäher', 'hirn', { perception = 20 }, { money = 100})
    mage1:add_order("ZAUBERE STUFE 5 'Störe Astrale Integrität'")
    local r = get_astral(mage1.region)
    create_demo_unit(mage1.faction, r, 1, "Astral Gestrandete", 'ast2', {perception=20})
    for i=1,100 do
        unit.create(get_monsters(), r, 10, "braineater")
    end
    unit.create(get_monsters(), get_astral(mage2.region), 10, "braineater")

end

function is_astral(r)
    if r == nil then return nil end
    return r.terrain == 'fog' or r.terrain == 'thickfog'
end

function get_astral(r)
    if is_astral(r) then return r:get_astral() else return nil end
end

function demo_module_mages2()
    local r0 = get_demo_region(24, 0)

    local mage1 = get_unit(atoi36('mort'))
    local mage2 = get_unit(atoi36('zoe'))
    local mage3 = get_unit(atoi36('momo'))
    mage1.aura = max_aura(mage1:eff_skill('magic'))
    mage2.aura = max_aura(mage2:eff_skill('magic'))
    mage3.aura = max_aura(mage3:eff_skill('magic'))
    mage1:clear_orders()
    mage2:clear_orders()
    mage3:clear_orders()

    if mage1.magic == 'cerddor' then
        try_cerddor(mage1, mage2, mage3)
    elseif mage1.magic == 'draig' then
        try_draig(mage1, mage2, mage3)
    elseif mage1.magic == 'gwyrrd' then
        try_gwyrrd(mage1, mage2, mage3)
    elseif mage1.magic == 'illaun' then
        try_illaun(mage1, mage2, mage3)
    elseif mage1.magic == 'tybied' then
        try_tybied(mage1, mage2, mage3)
    end
end

function print_stats()
    print('factions')
    for k,f in pairs(demo_units['factions']) do
        print(k, f, demo_units['numbers'][f])
    end
    for _,f in pairs(demo_units['factions']) do
        for _, u in pairs(demo_units['units'][f]) do
            print (u.name, u.id, u.number, f.id)
        end
    end
end

function spawn_monsters(rounds)
    eressea.settings.set("monsters.spawn.min_age", "0")
    eressea.settings.set("monsters.spawn.chance", "1")
    for i = 1, rounds do
        spawn_dragons()
        spawn_undead()
        spawn_braineaters(1)
        process_orders()
    end
end

function create_demo()
    rules_wild()
    rng.active()
    local races = { 'aquarian', 'cat', 'demon', 'dwarf', 'elf', 'goblin', 'halfling', 'human', 'insect', 'orc'}
    local f0 = create_demo_faction(races[math.random(#races)], "demo@example.com", "de", 'Paulas Crew', 'demo')
    local friends = create_demo_faction(races[math.random(#races)], "friend@example.com", "de", 'Freunde', 'nice')
    local foes = create_demo_faction(races[math.random(#races)], "foe@example.com", "de", 'Andere', 'foe')

    create_eressea_map(f0)
    f0.age = 100
    friends.age = 100
    foes.age = 100

    local plusage = 1
    set_turn(998-plusage)
    spawn_monsters(plusage)

    demo_module_starters()
    local p=demo_module_paula(f0)
    demo_module_piratenbucht()

    demo_module_mages()
    -- TODO
    -- Almosen
    -- Parteitarnung
    -- Beklauen
    -- Spione
    -- Fehler
    -- Traumschlösschen

    -- STARTER
    -- LERNE Magie (ohne Gebiet)

    print_stats()

    process_orders()

    demo_module_paula2(p)
    demo_module_mages2()

    init_reports()
    write_reports()

    process_orders()

    init_reports()
    write_reports()

    rules_tame()
end

function create_battle(r1, f1, f2)
    local a2 = unit.create(f2, r1, 1000)
    local a1 = unit.create(f1, r1, 1000)
    local h1 = unit.create(f1, r1, 1000)
    local h2 = unit.create(f2, r1, 1000)

    a1:set_skill("stamina", 10)
    a2:set_skill("stamina", 10)
    h1:set_skill("stamina", 10)
    h2:set_skill("stamina", 10)

    a1.hp = a1.hp_max * a1.number
    a2.hp = a2.hp_max * a1.number
    h1.hp = h1.hp_max * a1.number
    h2.hp = h2.hp_max * a1.number

    a1:set_skill("melee", 20)
    a2:set_skill("melee", 20)
    h1:set_skill("bow", 20)
    h2:set_skill("crossbow", 20)

    a1:add_item("sword", 1000)
    a1:add_item("shield", 1000)
    a1:add_item("plate", 1000)
    a2:add_item("sword", 1000)
    a2:add_item("shield", 1000)
    a2:add_item("plate", 1000)
    h1:add_item("bow", 1000)
    h2:add_item("crossbow", 1000)
    h1:add_item("shield", 1000)
    h2:add_item("shield", 1000)
    a1:add_order("KÄMPFE AGGRESSIV")
    a2:add_order("KÄMPFE AGGRESSIV")
    h1:add_order("KÄMPFE HINTEN")
    h2:add_order("KÄMPFE HINTEN")

    a1:add_item("money", 100 * a1.number)
    a2:add_item("money", 100 * a2.number)
    h1:add_item("money", 100 * h1.number)
    h2:add_item("money", 100 * h2.number)

    a1:add_order("ATTACKIERE " .. itoa36(h2.id))
    h1:add_order("ATTACKIERE " .. itoa36(h2.id))
    a1.guard = true

    local mage = unit.create(f1, r1, 1)
    mage.magic = 'gwyrrd'
    mage:set_skill('magic', 10)
    mage.aura = 1000
    mage:add_spell('eternal_walls')
    mage:add_spell('hail')
    mage:add_order('LERNE Magie Gwyrrd')
    mage:add_order('KAMPFZAUBER STUFE 8 Hagel')
    mage:add_order('KÄMPFE HINTEN')
    mage:add_order('ATTACKIERE ' .. itoa36(h2.id))

    return a1, a2, mage
end

function create_ship(r, f, stype)
    local s = ship.create(r, stype)
    local u = nil
    if f ~= nil then
       u = unit.create(f, r, 3)
       u:set_skill("sailing", 50)
       s.owner = u
       u.ship = s
   end
    return s, u
end

local minsize = { ['academy'] = 25, ['harbour'] = 10, ['caravan'] = 25, ['magictower'] = 50, ['dam'] = 50, ['tunnel'] = 100, ['stonecircle'] = 100 }

function create_building(r, btype)
    local b = building.create(r, btype)
    if minsize[btype] ~= nil then
        b.size = minsize[btype]
    else
        b.size = 10
    end
    return b
end

local example_bounds = {}

function init_example_bounds()
     example_bounds = get_plane_bounds()
end

function get_example_region(x, y)
    if example_bounds.x == nil then error("bounds not initialized") end
    return get_region(example_bounds.x + x, example_bounds.y + y)
end

local function create_region(x, y, terrain)
    if example_bounds.x == nil then error("bounds not initialized") end

    local r1 = region.create(example_bounds.x + x, example_bounds.y + y, terrain)

    if  r1:get_terrain_flag(B_LAND) and not r1:get_terrain_flag(B_FORBIDDEN) then
        r1:set_flag(F_MALLORN, false) -- no mallorn
        if r1.terrain ~= 'plain' then
            r1.peasants = 200
            make_trees(r1, 10)
            r1:set_resource("money", 1000)
            r1:set_resource("horse", 30)
        else
            r1.peasants = 2000
            make_trees(r1, 100)
            r1:set_resource("money", 10000)
            r1:set_resource("horse", 20000)
        end
    end
    return r1
end

function make_trees(r, t)
    r:set_resource("tree", t)
    r:set_resource("seed", t/10)
    r:set_resource("sapling", t/10)
end

function create_example()
    set_turn(333)
    rules_wild()
    rng.active()
    init_example_bounds()

    local r0 = create_region(0, 0, 'plain')
    local rs = create_region(1, 0, 'swamp')
    local rm = create_region(0, 1, 'mountain')
    local rh = create_region(-1, 1, 'highland')
    local rd = create_region(-1, 0, 'desert')
    local rg = create_region(0, -1, 'glacier')
    local rv = create_region(1, -1, 'volcano')
    local ri = create_region(2, 0, 'iceberg')
    local ro = create_region(1, 1, 'ocean')
    local rf = create_region(3, 0, 'firewall')
    local rw = create_region(4, 0, 'wall1')
    local rc = create_region(5, 0, 'corridor1')
    local rl = create_region(0, 2, 'plain')
    make_trees(rl, 600)

    local f = faction.create('human', "fex@eressea.de", "de")
    f.id = 146
    -- viewers
    for r in regions() do
        if r:get_terrain_flag(B_LAND) and not r:get_terrain_flag(B_FORBIDDEN) then
            local u = unit.create(f, r, 1)
            u:clear_orders()
            u:add_order("KÄMPFE NICHT")
        end
    end

    for x = -2, 2 do
        for y = -2, 3 do
            local r = get_example_region(x, y)
            if r == nil then
                create_region(x, y, 'ocean')
            end
        end
    end

    for _, btype in ipairs { 'lighthouse', 'mine', 'quarry', 'sawmill', 'smithy', 'stables', 'harbour', 'caravan', 'academy', 'magictower', 'dam', 'tunnel', 'inn', 'monument', 'stonecircle' } do
        create_building(r0, btype)
    end

    local f2 = faction.create('human', "war@eressea.de", "de")
    local a1, a2, mage = create_battle(r0, f, f2)

    for _, stype in ipairs { 'boat', 'longboat', 'dragonship', 'caravel', 'trireme', 'galleon' } do
        create_ship(rl, f, stype)
    end

    local s, u = create_ship(rl, f, 'caravel')
    s.coast = 3
    u:add_order("NACH o nw sw")

    s, u = create_ship(rl, f, 'caravel')
    s.damage = 30
    s = create_ship(rl, nil, 'caravel')
    s.coast = 2

    local light = create_building(rs, 'lighthouse')
    light.size = 10
    u = unit.create(f, light.region, 1)
    u:add_item("money", 1000)
    u:set_skill("perception", 10)
    u.building = light
    u:add_order("BOTSCHAFT REGION 'Hey, Welt'")

    process_orders()
    init_reports()
    write_reports()
end

function create_start()
    --[[
    Elfen bekommen Feenstiefel
   Zwerge bekommen eine Axt, ein Kettenhemd und 30 Lerntage Hiebwaffen
   Orks bekommen T4 in allen Waffentalenten
   Katzen bekommen einen RdU
   Goblins bekommen eine Starteinheit die 10 und nicht nur eine Person gross ist, ausserdem einen RdU.
   Insekten bekommen neun Nestwärmetränke, genug um einen Winter lang zu rekrutieren.
   Meermenschen bekommen ein Boot und 30 Lerntage Segeln
   Menschen bekommen eine Befestigung (Damals bestand diese nur aus 2 Steinen)
   Halblinge bekommen einen Wagen, zwei Pferde, fünf Luxusgüter jeder sorte und T1 Reiten.
   Trolle bekommen 10 Steine und Wahrnehmung T3.
   Dämonen bekommen T15 Ausdauer.
   --]]
end

return { create_demo = create_demo, create_example = create_example }
