#include <platform.h>
#include "tests.h"
#include "keyword.h"
#include "seen.h"
#include "prefix.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>
#include <kernel/messages.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/message.h>
#include <util/log.h>

#include <CuTest.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct race *test_create_race(const char *name)
{
    race *rc = rc_get_or_create(name);
    rc->maintenance = 10;
    rc->ec_flags |= GETITEM;
    return rc;
}

struct region *test_create_region(int x, int y, const terrain_type *terrain)
{
    region *r = new_region(x, y, NULL, 0);
    if (!terrain) {
        terrain_type *t = get_or_create_terrain("plain");
        t->size = 1000;
        fset(t, LAND_REGION);
        terraform_region(r, t);
    } else
        terraform_region(r, terrain);
    rsettrees(r, 0, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 2, 0);
    rsethorses(r, 0);
    rsetpeasants(r, r->terrain->size);
    return r;
}

struct faction *test_create_faction(const struct race *rc)
{
    faction *f = addfaction("nobody@eressea.de", NULL, rc ? rc : rc_get_or_create("human"), default_locale, 0);
    return f;
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
    const struct race * rc = f ? f->race : 0;
    assert(f || !r);
    return create_unit(r, f, 1, rc ? rc : rc_get_or_create("human"), 0, 0, 0);
}

void test_cleanup(void)
{
    int i;

    free_terrains();
    free_resources();
    global.functions.maintenance = NULL;
    global.functions.wage = NULL;
    free_params(&global.parameters);
    default_locale = 0;
    close_orders();
    free_locales();
    free_spells();
    free_buildingtypes();
    free_shiptypes();
    free_races();
    free_spellbooks();
    free_gamedata();
    free_seen();
    free_prefixes();
    mt_clear();
    for (i = 0; i != MAXSKILLS; ++i) {
        enable_skill(i, true);
    }
    for (i = 0; i != MAXKEYWORDS; ++i) {
        enable_keyword(i, true);
    }
    if (!mt_find("missing_message")) {
        mt_register(mt_new_va("missing_message", "name:string", 0));
        mt_register(mt_new_va("missing_feedback", "unit:unit", "region:region", "command:order", "name:string", 0));
    }
    if (errno) {
        int error = errno;
        errno = 0;
        log_error("errno: %d", error);
    }
}

terrain_type *
test_create_terrain(const char * name, unsigned int flags)
{
    terrain_type * t = get_or_create_terrain(name);
    t->flags = flags;
    return t;
}

building * test_create_building(region * r, const building_type * btype)
{
    building * b = new_building(btype ? btype : test_create_buildingtype("castle"), r, default_locale);
    b->size = b->type->maxsize > 0 ? b->type->maxsize : 1;
    return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
    ship * s = new_ship(stype ? stype : test_create_shiptype("boat"), r, default_locale);
    s->size = s->type->construction ? s->type->construction->maxsize : 1;
    return s;
}

ship_type * test_create_shiptype(const char * name)
{
    ship_type * stype = st_get_or_create(name);
    stype->cptskill = 1;
    stype->sumskill = 1;
    stype->minskill = 1;
    stype->range = 2;
    if (!stype->construction) {
        stype->construction = calloc(1, sizeof(construction));
        stype->construction->maxsize = 5;
        stype->construction->minskill = 1;
        stype->construction->reqsize = 1;
        stype->construction->skill = SK_SHIPBUILDING;
    }
    if (default_locale) {
        locale_setstring(default_locale, name, name);
    }
    return stype;
}

building_type * test_create_buildingtype(const char * name)
{
    building_type *btype = bt_get_or_create(name);
    btype->flags = BTF_NAMECHANGE;
    btype->_name = _strdup(name);
    btype->construction = (construction *)calloc(sizeof(construction), 1);
    btype->construction->skill = SK_BUILDING;
    btype->construction->maxsize = -1;
    btype->construction->minskill = 1;
    btype->construction->reqsize = 1;
    btype->construction->materials = (requirement *)calloc(sizeof(requirement), 2);
    btype->construction->materials[1].number = 0;
    btype->construction->materials[0].number = 1;
    btype->construction->materials[0].rtype = get_resourcetype(R_STONE);
    if (default_locale) {
        locale_setstring(default_locale, name, name);
    }
    return btype;
}

item_type * test_create_itemtype(const char * name) {
    resource_type * rtype;
    item_type * itype;

    rtype = rt_get_or_create(name);
    itype = it_get_or_create(rtype);

    return itype;
}

void test_translate_param(const struct locale *lang, param_t param, const char *text) {
    struct critbit_tree **cb;

    assert(lang && text);
    cb = (struct critbit_tree **)get_translations(lang, UT_PARAMS);
    add_translation(cb, text, param);
}

/** creates a small world and some stuff in it.
 * two terrains: 'plain' and 'ocean'
 * one race: 'human'
 * one ship_type: 'boat'
 * one building_type: 'castle'
 * in 0.0 and 1.0 is an island of two plains, around it is ocean.
 */
void test_create_world(void)
{
    terrain_type *t_plain, *t_ocean;
    region *island[2];
    int i;
    item_type * itype;
    struct locale * loc;

    loc = get_or_create_locale("de");
    locale_setstring(loc, keyword(K_RESERVE), "RESERVIEREN");
    locale_setstring(loc, "money", "SILBER");
    init_resources();

    itype = test_create_itemtype("horse");
    itype->flags |= ITF_BIG | ITF_ANIMAL;
    itype->weight = 5000;
    itype->capacity = 7000;

    itype = test_create_itemtype("cart");
    itype->flags |= ITF_BIG | ITF_VEHICLE;
    itype->weight = 4000;
    itype->capacity = 14000;

    test_create_itemtype("iron");
    test_create_itemtype("stone");

    t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION);
    t_plain->size = 1000;
    t_plain->max_road = 100;
    t_ocean = test_create_terrain("ocean", SEA_REGION | SAIL_INTO | SWIM_INTO);
    t_ocean->size = 0;

    island[0] = test_create_region(0, 0, t_plain);
    island[1] = test_create_region(1, 0, t_plain);
    for (i = 0; i != 2; ++i) {
        int j;
        region *r = island[i];
        for (j = 0; j != MAXDIRECTIONS; ++j) {
            region *rn = r_connect(r, (direction_t)j);
            if (!rn) {
                rn = test_create_region(r->x + delta_x[j], r->y + delta_y[j], t_ocean);
            }
        }
    }

    test_create_race("human");

    test_create_buildingtype("castle");
    test_create_shiptype("boat");
}

message * test_get_last_message(message_list *msgs) {
    if (msgs) {
        struct mlist *iter = msgs->begin;
        while (iter->next) {
            iter = iter->next;
        }
        return iter->msg;
    }
    return 0;
}

const char * test_get_messagetype(const message *msg) {
    const char * name;
    assert(msg);
    name = msg->type->name;
    if (strcmp(name, "missing_message") == 0) {
        name = (const char *)msg->parameters[0].v;
    }
    else if (strcmp(name, "missing_feedback") == 0) {
        name = (const char *)msg->parameters[3].v;
    }
    return name;
}

struct message * test_find_messagetype(struct message_list *msgs, const char *name) {
    struct mlist *ml;
    if (!msgs) return 0;
    for (ml = msgs->begin; ml; ml = ml->next) {
        if (strcmp(name, test_get_messagetype(ml->msg)) == 0) {
            return ml->msg;
        }
    }
    return 0;
}

void test_clear_messages(faction *f) {
    free_messagelist(f->msgs);
    f->msgs = 0;
}

void disabled_test(void *suite, void (*test)(CuTest *), const char *name) {
    (void)test;
    fprintf(stderr, "%s: SKIP\n", name);
}
