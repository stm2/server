require "lunit"

module("tests.items", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.storms", "0")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

function test_use_mistletoe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item('mistletoe', 3)
    u:add_order("BENUTZEN 2 Mistelzweig")
    process_orders()
    assert_equal(2, u:effect('mistletoe'))
    assert_equal(1, u:get_item('mistletoe'))
    assert_equal(1, f:count_msg_type('use_item'))
end

function test_mistletoe_survive()
    local r = region.create(0, 0, "plain")
    local u = unit.create(faction.create("human"), r, 1)
    local u2 = unit.create(faction.create("human"), r, 1)
    local uno = u.id
    u:add_item('mistletoe', 2)
    u:add_order("BENUTZEN 2 Mistelzweig")
    u2:add_order('ATTACKIERE ' .. itoa36(uno))
    process_orders()
    u = get_unit(uno)
    assert_not_nil(u)
    if u:effect('mistletoe') ~= 1 then
        print(u2.faction)
        for _, m in ipairs(u2.faction.messages) do
            print(m)
        end
        print(u.faction)
        for _, m in ipairs(u.faction.messages) do
            print(m)
        end
    end
    assert_equal(1, u:effect('mistletoe'))
end

function test_dreameye()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("dreameye", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Traumauge")
    assert_nil(u:get_curse('skillmod'))
    turn_begin()
    turn_process()
    assert_not_nil(u:get_curse('skillmod'))
    assert_equal(1, u:get_item("dreameye"))
    assert_equal(1, f:count_msg_type('use_tacticcrystal'))
    turn_end()
    assert_nil(u:get_curse('skillmod'))
end

function test_manacrystal()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("manacrystal", 2)
    u:clear_orders()
    u.magic = "gwyrrd"
    u:set_skill('magic', 1)
    u.aura = 0
    u:add_order("BENUTZEN 1 Astralkristall")
    turn_begin()
    turn_process()
    assert_equal(1, u:get_item("manacrystal"))
    assert_equal(25, u.aura)
    assert_equal(1, f:count_msg_type('manacrystal_use'))
    turn_end()
end

function test_skillpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("skillpotion", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Talenttrunk")
    process_orders()
    assert_equal(1, u:get_item("skillpotion"))
    assert_equal(1, f:count_msg_type('skillpotion_use'))
end

function test_studypotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    turn_begin()
    u:add_item("studypotion", 2)
    u:clear_orders()
    u:add_order("LERNE Unterhaltung")
    u:add_order("BENUTZEN 1 Lerntrank")
    turn_process()
    -- cannot sense the "learning" attribute, because study_cmd
    -- removes it during processing :(
    assert_equal(1, u:get_item("studypotion"))
    turn_end()
end

function test_antimagic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)

    turn_begin()
    u:add_item("antimagic", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Antimagiekristall")
    assert_equal(nil, r:get_curse('antimagiczone'))
    turn_process()
    assert_equal(5, r:get_curse('antimagiczone'))
    assert_equal(1, r:count_msg_type('use_antimagiccrystal'))
    assert_equal(1, u:get_item("antimagic"))
    turn_end()
    assert_equal(5, r:get_curse('antimagiczone')) -- haelt zwei wochen
    turn_end() -- hack: age the curse again
    assert_equal(nil, r:get_curse('antimagiczone'))
end

function test_ointment()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    local hp = u.hp
    u.hp = 1
    u:add_item("ointment", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Wundsalbe")
    process_orders()
    assert_equal(0, u:get_item("ointment"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal(hp, u.hp)
end

function test_use_domore()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("p3", 1)
    u:add_order("BENUTZEN 1 Schaffenstrunk")
    process_orders()
    assert_equal(10, u:effect("p3"))
    assert_equal(0, u:get_item("p3"))
    assert_equal(1, f:count_msg_type('usepotion'))
    u:clear_orders()
    u:set_skill('weaponsmithing', 3)
    u:add_item("iron", 2)
    u:add_order("MACHEN Schwert")
    process_orders()
    assert_equal(9, u:effect("p3"))
    assert_equal(0, u:get_item("iron"))
    assert_equal(2, u:get_item("sword"))
end

function test_bloodpotion_demon()
    local r = region.create(0, 0, "plain")
    local f = faction.create("demon")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(100, u:effect('peasantblood'))
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("demon", u.race)
end

function test_bloodpotion_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("human")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:effect('peasantblood'))
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("smurf", u.race)
end
