/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schr�der
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "move.h"

#include "alchemy.h"
#include "border.h"
#include "build.h"
#include "building.h"
#include "calendar.h"
#include "curse.h"
#include "faction.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "render.h"
#include "save.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "teleport.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/storage.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/targetregion.h>
#include <attributes/movement.h>
#include <attributes/otherfaction.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

int * storms;

typedef struct traveldir {
	int no;
	direction_t dir;
	int age;
} traveldir;

static attrib_type at_traveldir = {
	"traveldir",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,					/* Weil normales Aging an ung�nstiger Stelle */
	a_writechars,
	a_readchars
};

typedef struct follower {
  struct follower * next;
  unit * uf;
  unit * ut;
  const region_list * route_end;
} follower;

static void
get_followers(unit * target, region * r, const region_list * route_end, follower ** followers)
{
  unit * uf;
  for (uf=r->units;uf;uf=uf->next) {
    if (fval(uf, UFL_FOLLOWING) && !fval(uf, UFL_NOTMOVING)) {
      const attrib * a = a_findc(uf->attribs, &at_follow);
      if (a && a->data.v == target) {
        follower * fnew = malloc(sizeof(follower));
        fnew->uf = uf;
        fnew->ut = target;
        fnew->route_end = route_end;
        fnew->next = *followers;
        *followers = fnew;
      }
    }
  }
}

static void
shiptrail_init(attrib *a)
{
	a->data.v = calloc(1, sizeof(traveldir));
}

static void
shiptrail_finalize(attrib *a)
{
	free(a->data.v);
}

static int
shiptrail_age(attrib *a)
{
	traveldir *t = (traveldir *)(a->data.v);

	t->age--;
    return (t->age>0)?AT_AGE_KEEP:AT_AGE_REMOVE;
}

static int
shiptrail_read(attrib *a, storage * store)
{
	traveldir *t = (traveldir *)(a->data.v);

	t->no  = store->r_int(store);
	t->dir = (direction_t)store->r_int(store);
	t->age = store->r_int(store);
	return AT_READ_OK;
}

static void
shiptrail_write(const attrib *a, storage * store)
{
  traveldir *t = (traveldir *)(a->data.v);
  store->w_int(store, t->no);
  store->w_int(store, t->dir);
  store->w_int(store, t->age);
}

attrib_type at_shiptrail = {
	"traveldir_new",
	shiptrail_init,
	shiptrail_finalize,
	shiptrail_age,
	shiptrail_write,
	shiptrail_read
};

static int
age_speedup(attrib *a)
{
  if (a->data.sa[0] > 0) {
    a->data.sa[0] = a->data.sa[0] - a->data.sa[1];
  }
  return (a->data.sa[0]>0)?AT_AGE_KEEP:AT_AGE_REMOVE;
}

attrib_type at_speedup = {
  "speedup",
  NULL, NULL,
  age_speedup,
  a_writeint,
  a_readint
};

/* ------------------------------------------------------------- */

direction_t
getdirection(const struct locale * lang)
{
	return finddirection(getstrtoken(), lang);
}
/* ------------------------------------------------------------- */

static attrib_type at_driveweight = {
	"driveweight", NULL, NULL, NULL, NULL, NULL
};

static boolean
entrance_allowed(const struct unit * u, const struct region * r)
{
#ifdef REGIONOWNERS
  faction * owner = region_owner(r);
  if (owner == NULL || u->faction == owner) return true;
  if (alliedfaction(r->planep, owner, u->faction, HELP_TRAVEL)) return true;
#ifdef ENEMIES
  if (is_enemy(u->faction, owner)) return true;
#endif
  return false;
#else
  return true;
#endif
}

int
personcapacity(const unit *u)
{
	int cap = u->race->weight+u->race->capacity;
	
#if KARMA_MODULE
	if (fspecial(u->faction, FS_QUICK))
		cap -= 200;
#endif /* KARMA_MODULE */

	return cap;
}

static int
eff_weight(const unit *u)
{
	attrib *a = a_find(u->attribs, &at_driveweight);

	if (a) return weight(u) + a->data.i;

	return weight(u);
}

static void
get_transporters(const item * itm, int * p_animals, int *p_acap, int * p_vehicles, int * p_vcap)
{
  int vehicles = 0, vcap = 0;
  int animals = 0, acap = 0;

  for (;itm!=NULL;itm=itm->next) {
    const item_type * itype = itm->type;
    if (itype->capacity>0) {
      if (itype->flags & ITF_ANIMAL) {
        animals += itm->number;
        if (acap==0) acap = itype->capacity;
        assert(acap==itype->capacity || !"animals with different capacity not supported");
      }
      if (itype->flags & ITF_VEHICLE) {
        vehicles += itm->number;
        if (vcap==0) vcap = itype->capacity;
        assert(vcap==itype->capacity || !"vehicles with different capacity not supported");
      }
    }
  }
  *p_vehicles = vehicles;
  *p_animals = animals;
  *p_vcap = vcap;
  *p_acap = acap;
}

static int
ridingcapacity(unit * u)
{
	int vehicles = 0, vcap = 0;
  int animals = 0, acap = 0;

  get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

  /* Man tr�gt sein eigenes Gewicht plus seine Kapazit�t! Die Menschen
   * tragen nichts (siehe walkingcapacity). Ein Wagen z�hlt nur, wenn er
   * von zwei Pferden gezogen wird */

  animals = min(animals, effskill(u, SK_RIDING) * u->number * 2);
	if (fval(u->race, RCF_HORSE)) animals += u->number;

	/* maximal diese Pferde k�nnen zum Ziehen benutzt werden */
	vehicles = min(animals / HORSESNEEDED, vehicles);

	return vehicles * vcap + animals * acap;
}

int
walkingcapacity(const struct unit * u)
{
	int n, tmp, people, pferde_fuer_wagen;
	int wagen_ohne_pferde, wagen_mit_pferden, wagen_mit_trollen;
  int vehicles = 0, vcap = 0;
  int animals = 0, acap = 0;

  get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

  /* Das Gewicht, welches die Pferde tragen, plus das Gewicht, welches
	 * die Leute tragen */

  pferde_fuer_wagen = min(animals, effskill(u, SK_RIDING) * u->number * 4);
	if (fval(u->race, RCF_HORSE)) {
		animals += u->number;
		people = 0;
	} else {
		people = u->number;
	}

	/* maximal diese Pferde k�nnen zum Ziehen benutzt werden */
	wagen_mit_pferden = min(vehicles, pferde_fuer_wagen / HORSESNEEDED);

	n = wagen_mit_pferden * vcap;

	if (u->race == new_race[RC_TROLL]) {
		/* 4 Trolle ziehen einen Wagen. */
		/* Unbesetzte Wagen feststellen */
		wagen_ohne_pferde = vehicles - wagen_mit_pferden;

		/* Genug Trolle, um die Restwagen zu ziehen? */
		wagen_mit_trollen = min(u->number / 4, wagen_ohne_pferde);

		/* Wagenkapazit�t hinzuz�hlen */
		n += wagen_mit_trollen * vcap;
		wagen_ohne_pferde -= wagen_mit_trollen;
	}

	n += animals * acap;
	n += people * personcapacity(u);
	/* Goliathwasser */
  tmp = get_effect(u, oldpotiontype[P_STRONG]);
  if (tmp>0) {
    int horsecap = olditemtype[I_HORSE]->capacity;
    if (tmp>people) tmp = people;
  	n += tmp * (horsecap - personcapacity(u));
  }
  /* change_effect wird in ageing gemacht */
  tmp = get_item(u, I_TROLLBELT);
	n += min(people, tmp) * (STRENGTHMULTIPLIER-1) * personcapacity(u);

	return n;
}

enum {
  E_CANWALK_OK = 0,
  E_CANWALK_TOOMANYHORSES,
  E_CANWALK_TOOMANYCARTS,
  E_CANWALK_TOOHEAVY
};

static int
canwalk(unit * u)
{
	int maxwagen, maxpferde;
  int vehicles = 0, vcap = 0;
  int animals = 0, acap = 0;

  /* workaround: monsters are too stupid to drop items, therefore they have
	 * infinite carrying capacity */

	if (is_monsters(u->faction)) return E_CANWALK_OK;

  get_transporters(u->items, &animals, &acap, &vehicles, &vcap);

	maxwagen = effskill(u, SK_RIDING) * u->number * 2;
	if (u->race == new_race[RC_TROLL]) {
		maxwagen = max(maxwagen, u->number / 4);
	}
	maxpferde = effskill(u, SK_RIDING) * u->number * 4 + u->number;

	if (animals > maxpferde)
		return E_CANWALK_TOOMANYHORSES;

	if (walkingcapacity(u) - eff_weight(u) >= 0)
		return E_CANWALK_OK;

	/* Stimmt das Gewicht, impliziert dies hier, da� alle Wagen ohne
	 * Zugpferde/-trolle als Fracht aufgeladen wurden: zu viele Pferde hat
	 * die Einheit nicht zum Ziehen benutzt, also nicht mehr Wagen gezogen
	 * als erlaubt. */

	if (vehicles > maxwagen)
		return E_CANWALK_TOOMANYCARTS;
	/* Es mu� nicht zwingend an den Wagen liegen, aber egal... (man
	 * k�nnte z.B. auch 8 Eisen abladen, damit ein weiterer Wagen als
	 * Fracht draufpa�t) */

	return E_CANWALK_TOOHEAVY;
}

boolean
canfly(unit *u)
{
	if (get_item(u, I_PEGASUS) >= u->number && effskill(u, SK_RIDING) >= 4)
		return true;

	if (fval(u->race, RCF_FLY)) return true;

	if (get_movement(&u->attribs, MV_FLY)) return true;

	return false;
}

boolean
canswim(unit *u)
{
  if (get_item(u, I_DOLPHIN) >= u->number && effskill(u, SK_RIDING) >= 4)
    return true;

#if KARMA_MODULE
  if (fspecial(u->faction, FS_AMPHIBIAN)) return true;
#endif /* KARMA_MODULE */
  if (u->race->flags & RCF_FLY) return true;

  if (u->race->flags & RCF_SWIM) return true;

  if (get_movement(&u->attribs, MV_FLY)) return true;

  if (get_movement(&u->attribs, MV_SWIM)) return true;

  return false;
}

static int
canride(unit * u)
{
  int pferde, maxpferde, unicorns, maxunicorns;
  int skill = effskill(u, SK_RIDING);

  unicorns = get_item(u, I_ELVENHORSE);
  pferde = get_item(u, I_HORSE);
  maxunicorns = (skill/5) * u->number;
  maxpferde = skill * u->number * 2;

  if(!(u->race->flags & RCF_HORSE)
    && ((pferde == 0 && unicorns == 0)
    || pferde > maxpferde || unicorns > maxunicorns)) {
      return 0;
  }

  if (ridingcapacity(u) - eff_weight(u) >= 0) {
    if(pferde == 0 && unicorns >= u->number && !(u->race->flags & RCF_HORSE)) {
      return 2;
    }
    return 1;
  }

  return 0;
}

static boolean
cansail(const region * r, ship * sh)
{
	int n = 0, p = 0;

	 /* sonst ist construction:: size nicht ship_type::maxsize */
	assert(!sh->type->construction || sh->type->construction->improvement == NULL);

	if (sh->type->construction && sh->size!=sh->type->construction->maxsize)
	  return false;
	getshipweight(sh, &n, &p);

	if (n > shipcapacity(sh)) return false;
	if (p > sh->type->cabins) return false;

	return true;
}

int
enoughsailors(const ship * sh, const region * r)
{
	int n;
	unit *u;

	n = 0;

	for (u = r->units; u; u = u->next) {
		if (u->ship == sh)
			n += eff_skill(u, SK_SAILING, r) * u->number;
  }
	return n >= sh->type->sumskill;
}
/* ------------------------------------------------------------- */

static ship *
do_maelstrom(region *r, unit *u)
{
  int damage;
  ship * sh = u->ship;

  damage = rng_int()%150 - eff_skill(u, SK_SAILING, r)*5;

  if (damage <= 0) {
    return sh;
  }

  damage_ship(u->ship, 0.01*damage);

  if (sh->damage >= sh->size * DAMAGE_SCALE) {
    ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
      "region ship damage sink", r, sh, damage, 1));
    remove_ship(&sh->region->ships, sh);
    return NULL;
  }
  ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
    "region ship damage sink", r, sh, damage, 0));
  return u->ship;
}

/** sets a marker in the region telling that the unit has travelled through it
 * this is used for two distinctly different purposes:
 * - to report that a unit has travelled through. the report function
 *   makes sure to only report the ships of travellers, not the travellers 
 *   themselves
 * - to report the region to the traveller
 */
void
travelthru(const unit * u, region * r)
{
  attrib *ru = a_add(&r->attribs, a_new(&at_travelunit));

  fset(r, RF_TRAVELUNIT);

  ru->data.v = (void*)u;

  /* the first and last region of the faction gets reset, because travelthrough
   * could be in regions that are located before the [first, last] interval,
   * and recalculation is needed */
#ifdef SMART_INTERVALS
  update_interval(u->faction, r);
#endif
}

static void
leave_trail(ship * sh, region * from, region_list *route)
{
  region * r = from;
  
  while (route!=NULL) {
    region * rn = route->data;
    direction_t dir = reldirection(r, rn);

    /* TODO: we cannot leave a trail into special directions 
    * if we use this kind of direction-attribute */
    if (dir<MAXDIRECTIONS && dir>=0) {
      traveldir * td = NULL;
      attrib * a = a_find(r->attribs, &at_shiptrail);

      while (a!=NULL && a->type==&at_shiptrail) {
        td = (traveldir *)a->data.v;
        if (td->no == sh->no) break;
        a = a->next;
      }

      if (a == NULL || a->type!=&at_shiptrail) {
        a = a_add(&(r->attribs), a_new(&at_shiptrail));
        td = (traveldir *)a->data.v;
        td->no = sh->no;
      }
      td->dir = dir;
      td->age = 2;
    }
    route = route->next;
    r = rn;
  }
}

static void
mark_travelthru(const unit * u, region * r, const region_list * route, const region_list * route_end)
{
  /* kein travelthru in der letzten region! */
  while (route!=route_end) {
    travelthru(u, r);
    r = route->data;
    route = route->next;
  }
}

ship *
move_ship(ship * sh, region * from, region * to, region_list * route)
{
  unit **iunit = &from->units;
  unit **ulist = &to->units;
  boolean trail = (route == NULL);

  if (from!=to) {
    translist(&from->ships, &to->ships, sh);
    sh->region = to;
  }

  while (*iunit!=NULL) {
    unit *u = *iunit;
    assert(u->region == from);

    if (u->ship == sh) {
      if (!trail) {
        leave_trail(sh, from, route);
        trail = true;
      }
      if (route!=NULL) mark_travelthru(u, from, route, NULL);
      if (from!=to) {
        u->ship = NULL;		/* damit move_unit() kein leave() macht */
        move_unit(u, to, ulist);
        ulist = &u->next;
        u->ship = sh;
      }
      if (route && eff_skill(u, SK_SAILING, from) >= 1) {
        produceexp(u, SK_SAILING, u->number);
      }
    }
    if (*iunit == u) iunit=&u->next;
  }
  
  return sh;
}

static boolean
check_working_buildingtype(const region * r, const building_type * bt)
{
	building *b;

	for (b = rbuildings(r); b; b = b->next) {
		if (b->type == bt) {
			if (b->size >= bt->maxsize && fval(b, BLD_WORKING)) {
				return true;
			}
		}
	}

	return false;
}

static boolean
is_freezing(const unit * u)
{
  if (u->race!=new_race[RC_INSECT]) return false;
  if (is_cursed(u->attribs, C_KAELTESCHUTZ, 0)) return false;
  return true;
}

static boolean
ship_allowed(const struct ship * sh, const region * r)
{
  int c = 0;
  static const building_type * bt_harbour=NULL;

  if (bt_harbour == NULL) bt_harbour=bt_find("harbour");

  if (r_insectstalled(r)) {
    /* insekten d�rfen nicht hier rein. haben wir welche? */
    unit * u;

    for (u=sh->region->units;u!=NULL;u=u->next) {
      if (u->ship!=sh) continue;

      if (is_freezing(u)) {
        unit * captain = shipowner(sh);
        if (captain) {
          ADDMSG(&captain->faction->msgs, msg_message("detectforbidden", 
            "unit region", u, r));
        }

        return false;
      }
    }
  }

  if (check_working_buildingtype(r, bt_harbour)) return true;
  for (c=0;sh->type->coasts[c]!=NULL;++c) {
    if (sh->type->coasts[c] == r->terrain) return true;
  }

  return false;
}

static boolean
flying_ship(const ship * sh)
{
  static const curse_type * ct_flyingship;
  if (!ct_flyingship) {
    ct_flyingship = ct_find("flyingship");
    assert(ct_flyingship);
  }
  if (sh->type->flags & SFL_FLY) return true;
  if (curse_active(get_curse(sh->attribs, ct_flyingship))) return true;
  return false;
}

static void
set_coast(ship * sh, region * r, region * rnext)
{
  if (!fval(rnext->terrain, SEA_REGION) && !flying_ship(sh)) {
    sh->coast = reldirection(rnext, r);
    assert(fval(r->terrain, SEA_REGION));
  } else {
    sh->coast = NODIRECTION;
  }
}

static void
drifting_ships(region * r)
{
  direction_t d;

  if (fval(r->terrain, SEA_REGION)) {
    ship** shp = &r->ships;
    while (*shp) {
      ship * sh = *shp;
      region * rnext = NULL;
      region_list * route = NULL;
      unit *firstu = NULL, *captain;
      int d_offset;
      direction_t dir = 0;

      /* Schiff schon abgetrieben oder durch Zauber gesch�tzt? */
      if (fval(sh, SF_DRIFTED) || is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
        shp = &sh->next;
        continue;
      }

      /* Kapit�n bestimmen */
      for (captain = r->units; captain; captain = captain->next) {
        if (captain->ship != sh) continue;
        if (firstu==NULL) firstu = captain;
        if (eff_skill(captain, SK_SAILING, r) >= sh->type->cptskill) {
          break;
        }
      }
      /* Kapit�n da? Besch�digt? Gen�gend Matrosen?
      * Gen�gend leicht? Dann ist alles OK. */

      assert(sh->type->construction->improvement == NULL); /* sonst ist construction::size nicht ship_type::maxsize */
      if (captain && sh->size == sh->type->construction->maxsize && enoughsailors(sh, r) && cansail(r, sh)) {
        shp = &sh->next;
        continue;
      }

      /* Auswahl einer Richtung: Zuerst auf Land, dann
      * zuf�llig. Falls unm�gliches Resultat: vergi� es. */
      d_offset = rng_int() % MAXDIRECTIONS;
      for (d = 0; d != MAXDIRECTIONS; ++d) {
        region * rn;
        dir = (direction_t)((d + d_offset) % MAXDIRECTIONS);
        rn = rconnect(r, dir);
        if (rn!=NULL && fval(rn->terrain, SAIL_INTO) && ship_allowed(sh, rn)) {
          rnext = rn;
          if (!fval(rnext->terrain, SEA_REGION)) break;
        }
      }

      if (rnext == NULL) {
        shp = &sh->next;
        continue;
      }

      /* Das Schiff und alle Einheiten darin werden nun von r
      * nach rnext verschoben. Danach eine Meldung. */
      add_regionlist(&route, rnext);

      set_coast(sh, r, rnext);
      sh = move_ship(sh, r, rnext, route);
      free_regionlist(route);

      if (firstu!=NULL) {
        unit *u, *lastu = NULL;
        message * msg = msg_message("ship_drift", "ship dir", sh, dir);
        for (u=firstu;u;u=u->next) {
          if (u->ship==sh && !fval(u->faction, FFL_MARK)) {
            fset(u->faction, FFL_MARK);
            add_message(&u->faction->msgs, msg);
            lastu = u->next;
          }
        }
        for (u=firstu;u!=lastu;u=u->next) {
          freset(u->faction, FFL_MARK);
        }
        msg_release(msg);
      }
     
      if (sh!=NULL) {
        fset(sh, SF_DRIFTED);

        damage_ship(sh, 0.02);
        if (sh->damage>=sh->size * DAMAGE_SCALE) {
          remove_ship(&sh->region->ships, sh);
        }
      }

      if (*shp == sh) shp = &sh->next;
    }
  }
}

static boolean
present(region * r, unit * u)
{
	return (boolean) (u && u->region == r);
}

static void
caught_target(region * r, unit * u)
{
	attrib * a = a_find(u->attribs, &at_follow);

	/* Verfolgungen melden */
	/* Misserfolgsmeldung, oder bei erfolgreichem Verfolgen unter
	 * Umstaenden eine Warnung. */

	if (a) {
		unit * target = (unit*)a->data.v;

    if (!present(r, target)) {
      ADDMSG(&u->faction->msgs, msg_message("followfail", "unit follower",
        target, u));
    } else if (!alliedunit(target, u->faction, HELP_ALL)
      && cansee(target->faction, r, u, 0))
    {
      ADDMSG(&target->faction->msgs, msg_message("followdetect", 
        "unit follower", target, u));
    }
	}
}

/* TODO: Unsichtbarkeit bedenken ! */

static unit *
bewegung_blockiert_von(unit * reisender, region * r)
{
	unit *u;
	int perception = 0;
	boolean contact = false;
	unit * guard = NULL;

	if (fval(reisender->race, RCF_ILLUSIONARY)) return NULL;
	for (u=r->units;u && !contact;u=u->next) {
		if (getguard(u) & GUARD_TRAVELTHRU) {
			int sk = eff_skill(u, SK_PERCEPTION, r);
			if (invisible(reisender, u) >= reisender->number) continue;
			if (u->faction == reisender->faction) contact = true;
      else if (ucontact(u, reisender)) contact = true;
			else if (alliedunit(u, reisender->faction, HELP_GUARD)) contact = true;
			else if (sk>=perception) {
				perception = sk;
				guard = u;
			}
		}
	}
	if (!contact && guard) {
		double prob = 0.3; /* 30% base chance */
		prob += 0.1 * (perception - eff_stealth(reisender, r));
		prob += 0.1 * min(guard->number, get_item(guard, I_AMULET_OF_TRUE_SEEING));

		if (chance(prob)) {
			return guard;
		}
	}
	return NULL;
}

static boolean
is_guardian_u(unit * u2, unit *u, unsigned int mask)
{
  if (u2->faction == u->faction) return false;
  if ((getguard(u2) & mask) == 0) return false;
  if (alliedunit(u2, u->faction, HELP_GUARD)) return false;
  if (ucontact(u2, u)) return false;
  if (!cansee(u2->faction, u->region, u, 0)) return false;
  
  return true;
}

static boolean
is_guardian_r(unit * u2)
{
  if (u2->number == 0) return false;
  if ((u2->flags&UFL_GUARD)==0) return false;
  if (besieged(u2)) return false;
  if (!armedmen(u2) && !fval(u2->race, RCF_UNARMEDGUARD)) return false;
  return true;
}

#define MAXGUARDCACHE 16
unit *
is_guarded(region * r, unit * u, unsigned int mask)
{
  unit *u2 = NULL;
  int i;
  static unit * guardcache[MAXGUARDCACHE], * lastguard; /* STATIC_XCALL: used across calls */
  static int gamecookie = -1;
  if (gamecookie!=global.cookie) {
    if (gamecookie>=0) {
      /* clear the previous turn's cache */
      memset(guardcache, 0, sizeof(guardcache));
      lastguard = NULL;
    }
    gamecookie = global.cookie;
  }

  if (!fval(r, RF_GUARDED)) {
    return NULL;
  }

  if (lastguard && lastguard->region==r) {
    if (is_guardian_u(lastguard, u, mask)) {
      return lastguard;
    }
  }
  for (i=0;i!=MAXGUARDCACHE;++i) {
    unit * guard = guardcache[i];
    if (guard && guard!=lastguard && guard->region==r) {
      if (is_guardian_u(guard, u, mask)) {
        lastguard = guard;
        return guard;
      }
      if (u2==guard) {
        /* same guard twice signals we've tested everyone */
        return NULL;
      }
      u2 = guard;
    } else {
      /* exhausted all the guards in the cache, but maybe we'll find one later? */
      break;
    }
  }

  /* at this point, u2 is the last unit we tested to 
   * be a guard (and failed), or NULL
   * i is the position of the first free slot in the cache */
  
  for (u2 = (u2?u2->next:r->units); u2; u2=u2->next) {
    if (is_guardian_r(u2)) {
      /* u2 is a guard, so worth remembering */
      if (i<MAXGUARDCACHE) guardcache[i++] = u2;
      if (is_guardian_u(u2, u, mask)) {
        /* u2 is our guard. stop processing (we might have to go further next time) */
        lastguard = u2;
        return u2;
      }
    }
  }
  /* there are no more guards. we signal this by duplicating the last one.
   * i is still the position of the first free slot in the cache */
  if (i>0 && i<MAXGUARDCACHE) {
    guardcache[i] = guardcache[i-1];
  }

  return NULL;
}

static const char *shortdirections[MAXDIRECTIONS] =
{
	"dir_nw",
	"dir_ne",
	"dir_east",
	"dir_se",
	"dir_sw",
	"dir_west"
};

static void
cycle_route(order * ord, unit *u, int gereist)
{
	int bytes, cm = 0;
	char tail[1024], * bufp = tail;
	char neworder[2048];
	const char *token;
	direction_t d = NODIRECTION;
	boolean paused = false;
	boolean pause;
  order * norder;
  size_t size = sizeof(tail) - 1;

	if (get_keyword(ord) != K_ROUTE) return;
	tail[0] = '\0';

  init_tokens(ord);
  skip_token();

  neworder[0]=0;
	for (cm=0;;++cm) {
		const struct locale * lang = u->faction->locale;
		pause = false;
		token = getstrtoken();
		d = finddirection(token, lang);
		if(d == D_PAUSE) {
			pause = true;
		} else if (d == NODIRECTION) {
			break;
		}
		if (cm<gereist) {
			/* hier sollte keine PAUSE auftreten */
			assert(!pause);
      if (!pause) {
        const char * loc = LOC(lang, shortdirections[d]);
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, loc, size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
		}
		else if (strlen(neworder)>sizeof(neworder)/2) break;
		else if (cm == gereist && !paused && pause) {
      const char * loc = LOC(lang, parameters[P_PAUSE]);
      bytes = (int)strlcpy(bufp, " ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, loc, size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
			paused = true;
		}
		else if (pause) {
			/* da PAUSE nicht in ein shortdirections[d] umgesetzt wird (ist
			 * hier keine normale direction), muss jede PAUSE einzeln
			 * herausgefiltert und explizit gesetzt werden */
      if (neworder[0]) strcat(neworder, " ");
      strcat(neworder, LOC(lang, parameters[P_PAUSE]));
		} else {
      if (neworder[0]) strcat(neworder, " ");
			strcat(neworder, LOC(lang, shortdirections[d]));
		}
	}

	strcat(neworder, tail);
  norder = create_order(K_ROUTE, u->faction->locale, "%s", neworder);
  replace_order(&u->orders, ord, norder);
  free_order(norder);
}

static boolean 
transport(unit * ut, unit * u)
{
  order * ord;

  if (LongHunger(u) || fval(ut->region->terrain, SEA_REGION)) {
    return false;
  }

  for (ord = ut->orders; ord; ord = ord->next) {
    if (get_keyword(ord) == K_TRANSPORT) {
      init_tokens(ord);
      skip_token();
      if (getunit(ut->region, ut->faction) == u) {
        return true;
      }
    }
  }
  return false;
}

static boolean
can_move(const unit * u)
{
  if (u->race->flags & RCF_CANNOTMOVE) return false;
  if (get_movement(&u->attribs, MV_CANNOTMOVE)) return false;
  return true;
}

static void
init_transportation(void)
{
  region *r;

  for (r=regions; r; r=r->next) {
    unit *u;

    /* This is just a simple check for non-corresponding K_TRANSPORT/
     * K_DRIVE. This is time consuming for an error check, but there
     * doesn't seem to be an easy way to speed this up. */
    for (u=r->units; u; u=u->next) {
      if (get_keyword(u->thisorder) == K_DRIVE && can_move(u) && !fval(u, UFL_NOTMOVING) && !LongHunger(u)) {
        unit * ut;

        init_tokens(u->thisorder);
        skip_token();
        ut = getunit(r, u->faction);
        if (ut == NULL) {
          ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "feedback_unit_not_found", ""));
          continue;
        }
        if (!transport(ut, u)) {
          if (cansee(u->faction, r, ut, 0)) {
            cmistake(u, u->thisorder, 286, MSG_MOVE);
          } else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "feedback_unit_not_found", ""));
          }
        }
      }
    }

    /* This calculates the weights of all transported units and
     * adds them to an internal counter which is used by travel () to
     * calculate effective weight and movement. */
    
    if (!fval(r->terrain, SEA_REGION)) {
      for (u=r->units; u; u=u->next) {
        order * ord;
        int w = 0;

        for (ord = u->orders; ord; ord = ord->next) {
          if (get_keyword(ord) == K_TRANSPORT) {
            init_tokens(ord);
            skip_token();
            for (;;) {
              unit * ut = getunit(r, u->faction);

              if (ut == NULL) break;
              if (get_keyword(ut->thisorder) == K_DRIVE && can_move(ut) && !fval(ut, UFL_NOTMOVING) && !LongHunger(ut)) {
                init_tokens(ut->thisorder);
                skip_token();
                if (getunit(r, ut->faction) == u) {
                  w += weight(ut);
                }
              }
            }
          }
        }
        if (w > 0) a_add(&u->attribs, a_new(&at_driveweight))->data.i = w;
      }
    }
  }
}

static boolean
roadto(const region * r, direction_t dir)
{
  /* wenn es hier genug strassen gibt, und verbunden ist, und es dort
  * genug strassen gibt, dann existiert eine strasse in diese richtung */
  region * r2;
  static const curse_type * roads_ct = NULL;

  if (dir>=MAXDIRECTIONS || dir<0) return false;
  r2 = rconnect(r, dir);
  if (r == NULL || r2 == NULL) return false;

  if (roads_ct == NULL) roads_ct = ct_find("magicstreet"); 
  if (roads_ct!=NULL) {
    if (get_curse(r->attribs, roads_ct)!=NULL) return true;
    if (get_curse(r2->attribs, roads_ct)!=NULL) return true;
  }
  
  if (r->terrain->max_road <= 0) return false;
  if (r2->terrain->max_road <= 0) return false;
  if (rroad(r, dir) < r->terrain->max_road) return false;
  if (rroad(r2, dir_invert(dir)) < r2->terrain->max_road) return false;
  return true;
}

static const region_list *
cap_route(region * r, const region_list * route, const region_list * route_end, int speed)
{
  region * current = r;
  int moves = speed;
  const region_list * iroute = route;
  while (iroute!=route_end) {
    region * next = iroute->data;
    direction_t reldir = reldirection(current, next);

    /* adjust the range of the unit */
    if (roadto(current, reldir)) moves -= BP_ROAD;
    else moves -= BP_NORMAL;
    if (moves<0) break;
    iroute = iroute->next;
    current = next;
  }
  return iroute;
}

static region *
next_region(unit * u, region * current, region * next)
{
  border * b;

  b = get_borders(current, next);
  while (b!=NULL) {
    if (b->type->move) {
      region * rto = b->type->move(b, u, current, next, true);
      if (rto!=next) {
        /* the target region was changed (wisps, for example). check the
        * new target region for borders */
        next = rto;
        b = get_borders(current, next);
        continue;
      }
    }
    b = b->next;
  }
  return next;
}

static const region_list *
reroute(unit * u, const region_list * route, const region_list * route_end)
{
  region * current = u->region;
  while (route!=route_end) {
    region * next = next_region(u, current, route->data);
    if (next!=route->data) break;
    route = route->next;
  }
  return route;
}

static void
make_route(unit * u, order * ord, region_list ** routep)
{
  region_list **iroute = routep;
  region * current = u->region;
  region * next = NULL;
  const char * token = getstrtoken();
  int error = movewhere(u, token, current, &next);

  if (error!=E_MOVE_OK) {
    message * msg = movement_error(u, token, ord, error);
    if (msg!=NULL) {
      add_message(&u->faction->msgs, msg);
      msg_release(msg);
    }
    next = NULL;
  }

  while (next!=NULL) {
    direction_t reldir;

    if (current == next) {
      /* PAUSE */
      break;
    }
    next = next_region(u, current, next);
    reldir = reldirection(current, next);
    
    add_regionlist(iroute, next);
    iroute = &(*iroute)->next;

    current = next;
    token = getstrtoken();
    error = movewhere(u, token, current, &next);
    if (error) {
      message * msg = movement_error(u, token, ord, error);
      if (msg!=NULL) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
      }
      next = NULL;
    }
  }
}

/** calculate the speed of a unit
 *
 * zu Fu� reist man 1 Region, zu Pferd 2 Regionen. Mit Stra�en reist
 * man zu Fu� 2, mit Pferden 3 weit.
 *
 * Berechnet wird das mit BPs. Zu Fu� hat man 4 BPs, zu Pferd 6.
 * Normalerweise verliert man 3 BP pro Region, bei Stra�en nur 2 BP.
 * Au�erdem: Wenn Einheit transportiert, nur halbe BP 
 */
static int 
movement_speed(unit * u)
{
  int mp;
  static const curse_type * speed_ct;
  static boolean init = false;
  double dk = u->race->speed;

  assert(u->number);
  /* dragons have a fixed speed, and no other effects work on them: */
  switch (old_race(u->race)) {
    case RC_DRAGON:
    case RC_WYRM:
    case RC_FIREDRAGON:
    case RC_BIRTHDAYDRAGON:
    case RC_SONGDRAGON:
      return BP_DRAGON;
  }

  if (!init) { 
    init = true; 
    speed_ct = ct_find("speed"); 
  }
  if (speed_ct) {
    curse *c = get_curse(u->attribs, speed_ct);
    if (c!=NULL) {
      int men = get_cursedmen(u, c);
      dk *= 1.0 + (double)men/(double)u->number;
    }
  }

  switch (canride(u)) {

  case 1:		/* Pferd */
    mp = BP_RIDING;
    break;

  case 2:		/* Einhorn */
    mp = BP_UNICORN;
    break;

  default:
    mp = BP_WALKING;
#if KARMA_MODULE
    /* faction special */
    if (fspecial(u->faction, FS_QUICK)) mp = BP_RIDING;
#endif /* KARMA_MODULE */

    /* Siebenmeilentee */
    if (get_effect(u, oldpotiontype[P_FAST]) >= u->number) {
      mp *= 2;
      change_effect(u, oldpotiontype[P_FAST], -u->number);
    }

    /* unicorn in inventory */
    if (u->number <= get_item(u, I_FEENSTIEFEL)) {
      mp *= 2;
    }

    /* Im Astralraum sind Tyb und Ill-Magier doppelt so schnell.
    * Nicht kumulativ mit anderen Beschleunigungen! */
    if (mp*dk <= BP_WALKING*u->race->speed && getplane(u->region) == get_astralplane() && is_mage(u)) {
      sc_mage * mage = get_mage(u);
      if (mage->magietyp == M_ASTRAL || mage->magietyp == M_TRAUM) {
        mp *= 2;
      }
    }
    break;
  }
  return (int)(dk*mp);
}

enum {
  TRAVEL_NORMAL,
  TRAVEL_FOLLOWING,
  TRAVEL_TRANSPORTED,
  TRAVEL_RUNNING
};

static const region_list *
travel_route(unit * u, const region_list * route_begin, const region_list * route_end, order * ord, int mode)
{
  region * r = u->region;
  region * current = u->region;
  const region_list * iroute = route_begin;
  int steps = 0;
  boolean landing = false; /* aquarians have landed */

  while (iroute && iroute!=route_end) {
    region * next = iroute->data;
    direction_t reldir = reldirection(current, next);
    border * b = get_borders(current, next);

    /* check if we are caught by guarding units */
    if (iroute!=route_begin && mode!=TRAVEL_RUNNING && mode!=TRAVEL_TRANSPORTED) {
      unit * wache = bewegung_blockiert_von(u, current);
      if (wache!=NULL) {
        ADDMSG(&u->faction->msgs, msg_message("moveblockedbyguard", 
          "unit region guard", u, current, wache));
        break;
      }
    }

    /* check movement from/to oceans. 
     * aquarian special, flying units, horses, the works */
    if ((u->race->flags & RCF_FLY) == 0) {
      if (!fval(next->terrain, SEA_REGION)) {
        /* next region is land */
        if (fval(current->terrain, SEA_REGION)) {
          int moving = u->race->flags & (RCF_SWIM|RCF_WALK|RCF_COASTAL);
          /* Die Einheit kann nicht fliegen, ist im Ozean, und will an Land */
          if (moving != (RCF_SWIM|RCF_WALK) && (moving&RCF_COASTAL) == 0) {
            /* can't swim+walk and isn't allowed to enter coast from sea */
            if (ord!=NULL) cmistake(u, ord, 44, MSG_MOVE);
            break;
          }
          landing = true;
        } else if ((u->race->flags & RCF_WALK) == 0) {
          /* Spezialeinheiten, die nicht laufen k�nnen. */
          ADDMSG(&u->faction->msgs, msg_message("detectocean",
            "unit region", u, next));
          break;
        } else if (landing) {
          /* wir sind diese woche angelandet */
          ADDMSG(&u->faction->msgs, msg_message("detectocean",
            "unit region", u, next));
          break;
        }
      } else {
        /* Ozeanfelder k�nnen nur von Einheiten mit Schwimmen und ohne
         * Pferde betreten werden. */
        if (!(canswim(u) || canfly(u))) {
          ADDMSG(&u->faction->msgs, msg_message("detectocean",
            "unit region", u, next));
          break;
        }
      }

      if (fval(current->terrain, SEA_REGION) || fval(next->terrain, SEA_REGION)) {
        /* trying to enter or exit ocean with horses, are we? */
        if (get_item(u, I_HORSE) > 0 || get_item(u, I_ELVENHORSE) > 0) {
          /* tries to do it with horses */
          if (ord!=NULL) cmistake(u, ord, 67, MSG_MOVE);
          break;
        }
      }

    }

    /* movement blocked by a wall */
    if (reldir>=0 && move_blocked(u, current, next)) {
      ADDMSG(&u->faction->msgs, msg_message("leavefail", 
        "unit region", u, next));
      break;
    }

    /* region ownership only: region owned by enemies */
    if (!entrance_allowed(u, next)) {
      ADDMSG(&u->faction->msgs, msg_message("regionowned", 
        "unit region target", u, current, next));
      break;
    }

    /* illusionary units disappear in antimagic zones */
    if (fval(u->race, RCF_ILLUSIONARY)) {
      curse * c = get_curse(next->attribs, ct_find("antimagiczone"));
      if (curse_active(c)) {
        curse_changevigour(&next->attribs, c, (float)-u->number);
        ADDMSG(&u->faction->msgs, msg_message("illusionantimagic", "unit", u));
        set_number(u, 0);
        break;
      }
    }

    /* terrain is marked as forbidden (curse, etc) */
    if (fval(next->terrain, FORBIDDEN_REGION)) {
      ADDMSG(&u->faction->msgs, msg_message("detectforbidden", 
        "unit region", u, next));
      break;
    }

    /* unit is an insect and cannot move into a glacier */
    if (u->race == new_race[RC_INSECT]) {
      if (r_insectstalled(next) && is_freezing(u)) {
        ADDMSG(&u->faction->msgs, msg_message("detectforbidden",
          "unit region", u, next));
        break;
      }
    }

    /* effect of borders */
    while (b!=NULL) {
      if (b->type->move) {
        b->type->move(b, u, current, next, false);
      }
      b = b->next;
    }

    current = next;
    iroute = iroute->next;
    ++steps;
    if (u->number == 0) break;
  }

  if (iroute!=route_begin) {
    /* the unit has moved at least one region */
    int walkmode;

    setguard(u, GUARD_NONE);
    cycle_route(ord, u, steps);

    if (mode == TRAVEL_RUNNING) {
      walkmode = 0;
    } if (canride(u)) {
      walkmode = 1;
      produceexp(u, SK_RIDING, u->number);
    } else {
      walkmode = 2;
    }

    /* Berichte �ber Durchreiseregionen */

    if (mode!=TRAVEL_TRANSPORTED) {
      ADDMSG(&u->faction->msgs, msg_message("travel", 
        "unit mode start end regions", u, walkmode, r, current, route_begin->next?route_begin:NULL));
    }

    mark_travelthru(u, r, route_begin, iroute);
    move_unit(u, current, NULL);

    /* make orders for the followers */
  }
  fset(u, UFL_LONGACTION|UFL_NOTMOVING);
  setguard(u, GUARD_NONE);
  assert(u->region == current);
  return iroute;
}

static boolean
ship_ready(const region * r, unit * u)
{
	if (!fval(u, UFL_OWNER)) {
		cmistake(u, u->thisorder, 146, MSG_MOVE);
		return false;
	}
	if (eff_skill(u, SK_SAILING, r) < u->ship->type->cptskill) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "error_captain_skill_low",
      "value ship", u->ship->type->cptskill, u->ship));
		return false;
	}
	assert(u->ship->type->construction->improvement == NULL); /* sonst ist construction::size nicht ship_type::maxsize */
	if (u->ship->size!=u->ship->type->construction->maxsize) {
		cmistake(u, u->thisorder, 15, MSG_MOVE);
		return false;
	}
	if (!enoughsailors(u->ship, r)) {
		cmistake(u, u->thisorder, 1, MSG_MOVE);
/*		mistake(u, u->thisorder,
				"Auf dem Schiff befinden sich zuwenig erfahrene Seeleute.", MSG_MOVE); */
		return false;
	}
	if (!cansail(r, u->ship)) {
    cmistake(u, u->thisorder, 18, MSG_MOVE);
		return false;
	}
	return true;
}

unit *
owner_buildingtyp(const region * r, const building_type * bt)
{
	building *b;
	unit *owner;

	for (b = rbuildings(r); b; b = b->next) {
		owner = buildingowner(r, b);
		if (b->type == bt && owner != NULL) {
			if (b->size >= bt->maxsize) {
				return owner;
			}
		}
	}

	return NULL;
}

boolean
buildingtype_exists(const region * r, const building_type * bt)
{
	building *b;

	for (b = rbuildings(r); b; b = b->next) {
		if (b->type == bt) {
			if (b->size >= bt->maxsize) {
				return true;
			}
		}
	}

	return false;
}

/* Pr�ft, ob Ablegen von einer K�ste in eine der erlaubten Richtungen erfolgt. */

static boolean
check_takeoff(ship *sh, region *from, region *to)
{
  if (!fval(from->terrain, SEA_REGION) && sh->coast != NODIRECTION) {
    direction_t coast = sh->coast;
    direction_t dir   = reldirection(from, to);
    direction_t coastr  = (direction_t)((coast+1) % MAXDIRECTIONS);
    direction_t coastl  = (direction_t)((coast+MAXDIRECTIONS-1) % MAXDIRECTIONS);

    if (dir!=coast && dir!=coastl && dir!=coastr
      && !check_working_buildingtype(from, bt_find("harbour")))
    {
      return false;
    }
  }

  return true;
}

static void
sail(unit * u, order * ord, boolean move_on_land, region_list **routep)
{
  region *starting_point = u->region;
  region *current_point, *last_point;
  int k, step = 0;
  region_list **iroute = routep;
  ship * sh = u->ship;
  faction * f = u->faction;
  region * next_point = NULL;
  int error;
  const char * token = getstrtoken();

  if (routep) *routep = NULL;

  error = movewhere(u, token, starting_point, &next_point);
  if (error) {
    message * msg = movement_error(u, token, ord, error);
    if (msg!=NULL) {
      add_message(&u->faction->msgs, msg);
      msg_release(msg);
    }
    return;
  }

  if (!ship_ready(starting_point, u)) return;

  /* Wir suchen so lange nach neuen Richtungen, wie es geht. Diese werden
  * dann nacheinander ausgef�hrt. */

  k = shipspeed(sh, u);

  last_point = starting_point;
  current_point = starting_point;

  /* die n�chste Region, in die man segelt, wird durch movewhere () aus der
  * letzten Region bestimmt.
  *
  * Anfangen tun wir bei starting_point. next_point ist beim ersten
  * Durchlauf schon gesetzt (Parameter!). current_point ist die letzte g�ltige,
  * befahrene Region. */

  while (next_point && current_point!=next_point && step < k) {
    const char * token;
    int error;
    const terrain_type * tthis = current_point->terrain;
    /* these values need to be updated if next_point changes (due to storms): */
    const terrain_type * tnext = next_point->terrain;
    direction_t dir = reldirection(current_point, next_point);

    assert(sh == u->ship || !"ship has sunk, but we didn't notice it");

    if (fval(next_point->terrain, FORBIDDEN_REGION)) {
      plane *pl = getplane(next_point);
      if (pl && fval(pl, PFL_NOCOORDS)) {
        ADDMSG(&f->msgs,  msg_message("sailforbiddendir", 
          "ship direction", sh, dir));
      } else {
        ADDMSG(&f->msgs, msg_message("sailforbidden", 
          "ship region", sh, next_point));
      }
      break;
    }

    if (!flying_ship(sh)) {
      int stormchance;
      static int stormyness;
      static int gamecookie = -1;

      if (gamecookie != global.cookie) {
        gamedate date;
        get_gamedate(turn, &date);
        stormyness = storms[date.month] * 5;
        gamecookie = global.cookie;
      }

      /* storms should be the first thing we do. */
      stormchance = stormyness / shipspeed(sh, u);
      if (check_leuchtturm(next_point, NULL)) stormchance /= 3;

      if (rng_int()%10000 < stormchance && fval(current_point->terrain, SEA_REGION)) {
        if (!is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
          region * rnext = NULL;
          boolean storm = true;
          int d_offset = rng_int() % MAXDIRECTIONS;
          direction_t d;
          /* Sturm nur, wenn n�chste Region Hochsee ist. */
          for (d=0;d!=MAXDIRECTIONS;++d) {
            direction_t dnext = (direction_t)((d + d_offset) % MAXDIRECTIONS);
            region * rn = rconnect(current_point, dnext);

            if (rn!=NULL) {
              if (fval(rn->terrain, FORBIDDEN_REGION)) continue;
              if (!fval(rn->terrain, SEA_REGION)) {
                storm = false;
                break;
              }
              if (rn!=next_point) rnext = rn;
            }
          }
          if (storm && rnext!=NULL) {
            ADDMSG(&f->msgs, msg_message("storm", "ship region sink", 
              sh, current_point, sh->damage>=sh->size * DAMAGE_SCALE));

            /* damage the ship. we handle destruction in the end */
            damage_ship(sh, 0.02);
            if (sh->damage>=sh->size * DAMAGE_SCALE) break;

            next_point = rnext;
            /* these values need to be updated if next_point changes (due to storms): */
            tnext = next_point->terrain;
            dir = reldirection(current_point, next_point);
          }
        }
      }

      if (!fval(tthis, SEA_REGION)) {
        if (!fval(tnext, SEA_REGION)) {
          if (!move_on_land) {
            /* check that you're not traveling from one land region to another. */
            ADDMSG(&u->faction->msgs, msg_message("shipnoshore",
              "ship region", sh, next_point));
            break;
          }
        } else {
          if (check_takeoff(sh, current_point, next_point) == false) {
            /* Schiff kann nicht ablegen */
            cmistake(u, ord, 182, MSG_MOVE);
            break;
          }
        }
      } else if (fval(tnext, SEA_REGION)) {
        /* target region is an ocean, and we're not leaving a shore */
        if (!(sh->type->flags & SFL_OPENSEA)) {
          /* ship can only stay close to shore */
          direction_t d;
          
          for (d=0;d!=MAXDIRECTIONS;++d) {
            region * rc = rconnect(next_point, d);
            if (!fval(rc->terrain, SEA_REGION)) break;
          }
          if (d == MAXDIRECTIONS) {
            /* Schiff kann nicht aufs offene Meer */
            cmistake(u, ord, 249, MSG_MOVE);
            break;
          }
        }
      }
    
      if (!ship_allowed(sh, next_point)) {
        /* for some reason or another, we aren't allowed in there.. */
        if (check_leuchtturm(current_point, NULL)) {
          ADDMSG(&f->msgs, msg_message("sailnolandingstorm", "ship", sh));
        } else {
          ADDMSG(&f->msgs, msg_message("sailnolanding", "ship region", sh, next_point));
          damage_ship(sh, 0.10);
          /* we handle destruction at the end */
        }
        break;
      }

      if (is_cursed(next_point->attribs, C_MAELSTROM, 0)) {
        if (do_maelstrom(next_point, u) == NULL) break;
      }

    } /* !flying_ship */

    /* Falls Blockade, endet die Seglerei hier */
    if (move_blocked(u, current_point, next_point)) {
      ADDMSG(&u->faction->msgs, msg_message("sailfail", "ship region", sh, current_point));
      break;
    }

    /* Falls kein Problem, eines weiter ziehen */
    fset(sh, SF_MOVED);
    if (iroute) {
      add_regionlist(iroute, next_point);
      iroute = &(*iroute)->next;
    }
    step++;

    last_point = current_point;
    current_point = next_point;

    if (!fval(current_point->terrain, SEA_REGION) && !is_cursed(sh->attribs, C_SHIP_FLYING, 0)) break;
    token = getstrtoken();
    error = movewhere(u, token, current_point, &next_point);
    if (error || next_point == NULL) {
      message * msg = movement_error(u, token, ord, error);
      if (msg!=NULL) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
      }
      next_point = current_point;
      break;
    }
  }

  if (sh->damage>=sh->size * DAMAGE_SCALE) {
    ADDMSG(&f->msgs, msg_message("shipsink", "ship", sh));
    remove_ship(&sh->region->ships, sh);
    sh = NULL;
  }

  /* Nun enth�lt current_point die Region, in der das Schiff seine Runde
  * beendet hat. Wir generieren hier ein Ereignis f�r den Spieler, das
  * ihm sagt, bis wohin er gesegelt ist, falls er �berhaupt vom Fleck
  * gekommen ist. Das ist nicht der Fall, wenn er von der K�ste ins
  * Inland zu segeln versuchte */

  if (sh!=NULL && fval(sh, SF_MOVED)) {
    unit * hafenmeister;
    /* nachdem alle Richtungen abgearbeitet wurden, und alle Einheiten
    * transferiert wurden, kann der aktuelle Befehl gel�scht werden. */
    cycle_route(ord, u, step);
    set_order(&u->thisorder, NULL);
    set_coast(sh, last_point, current_point);

    if( is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
      ADDMSG(&f->msgs, msg_message("shipfly", "ship from to", sh, starting_point, current_point));
    } else {
      ADDMSG(&f->msgs, msg_message("shipsail", "ship from to", sh, starting_point, current_point));
    }

    /* Das Schiff und alle Einheiten darin werden nun von
    * starting_point nach current_point verschoben */

    /* Verfolgungen melden */
    if (fval(u, UFL_FOLLOWING)) caught_target(current_point, u);

    sh = move_ship(sh, starting_point, current_point, *routep);

    /* Hafengeb�hren ? */

    hafenmeister = owner_buildingtyp(current_point, bt_find("harbour"));
    if (sh && hafenmeister != NULL) {
      item * itm;
      unit * u2;
      item * trans = NULL;

      for (u2 = current_point->units; u2; u2 = u2->next) {
        if (u2->ship == sh &&
          !alliedunit(hafenmeister, u->faction, HELP_GUARD)) {


            if (effskill(hafenmeister, SK_PERCEPTION) > effskill(u2, SK_STEALTH)) {
              for (itm=u2->items; itm; itm=itm->next) {
                const luxury_type * ltype = resource2luxury(itm->type->rtype);
                if (ltype!=NULL && itm->number>0) {
                  int st = itm->number * effskill(hafenmeister, SK_TRADE) / 50;
                  st = min(itm->number, st);

                  if (st > 0) {
                    i_change(&u2->items, itm->type, -st);
                    i_change(&hafenmeister->items, itm->type, st);
                    i_add(&trans, i_new(itm->type, st));
                  }
                }
              }
            }
          }
      }
      if (trans) {
        message * msg = msg_message("harbor_trade", "unit items ship", hafenmeister, trans, u->ship);
        add_message(&u->faction->msgs, msg);
        add_message(&hafenmeister->faction->msgs, msg);
        msg_release(msg);
        while (trans) i_remove(&trans, trans);
      }
    }
  }
}

unit *
get_captain(const ship * sh)
{
  const region * r = sh->region;
  unit *u;

  for (u = r->units; u; u = u->next) {
    if (u->ship == sh && eff_skill(u, SK_SAILING, r) >= sh->type->cptskill)
      return u;
  }

  return NULL;
}

/* Segeln, Wandern, Reiten 
* when this routine returns a non-zero value, movement for the region needs 
* to be done again because of followers that got new MOVE orders. 
* Setting FL_LONGACTION will prevent a unit from being handled more than once
* by this routine 
*
* the token parser needs to be initialized before calling this function!
*/

/** fleeing units use this function 
*/
void 
run_to(unit * u, region * to)
{
  region_list * route = NULL;
  add_regionlist(&route, to);
  travel_route(u, route, NULL, NULL, TRAVEL_RUNNING);
  free_regionlist(route);
  /* weder transport noch follow */
}

static const region_list *
travel_i(unit * u, const region_list * route_begin, const region_list * route_end, order * ord, int mode, follower ** followers)
{
  region * r = u->region;

  switch (canwalk(u)) {
    case E_CANWALK_TOOHEAVY:
      cmistake(u, ord, 57, MSG_MOVE);
      return route_begin;
    case E_CANWALK_TOOMANYHORSES:
      cmistake(u, ord, 56, MSG_MOVE);
      return route_begin;
    case E_CANWALK_TOOMANYCARTS:
      cmistake(u, ord, 42, MSG_MOVE);
      return route_begin;
  }
  route_end = cap_route(r, route_begin, route_end, movement_speed(u));

  route_end = travel_route(u, route_begin, route_end, ord, mode);
  get_followers(u, r, route_end, followers);

  /* transportation */
  for (ord = u->orders; ord; ord = ord->next) {
    unit * ut;

    if (get_keyword(ord) != K_TRANSPORT) continue;
    
    init_tokens(ord);
    skip_token();
    ut = getunit(r, u->faction);
    if (ut!=NULL) {
      boolean found = false;
      if (get_keyword(ut->thisorder) == K_DRIVE && can_move(ut)) {
        if (!fval(ut, UFL_NOTMOVING) && !LongHunger(ut)) {
          init_tokens(ut->thisorder);
          skip_token();
          if (getunit(u->region, ut->faction) == u) {
            const region_list * route_to = travel_route(ut, route_begin, route_end, ord, TRAVEL_TRANSPORTED);

            if (route_to!=route_begin) {
              get_followers(ut, r, route_to, followers);
            }
            ADDMSG(&ut->faction->msgs, msg_message("transport", 
              "unit target start end", u, ut, r, ut->region));
            found = true;
          }
        }
        if (!found) {
          if (cansee(u->faction, u->region, ut, 0)) {
            cmistake(u, ord, 90, MSG_MOVE);
          } else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
          }
        }
      } else {
        cmistake(u, ord, 99, MSG_MOVE);
      }
    } else {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
    }
  }
  return route_end;
}

/** traveling without ships
 * walking, flying or riding units use this function
 */
static void
travel(unit * u, region_list ** routep)
{
  region * r = u->region;
  region_list * route_begin = NULL;
  follower * followers = NULL;

  if (routep) *routep = NULL;

  /* a few pre-checks that need not be done for each step: */
  if (!fval(r->terrain, SEA_REGION)) {
    ship * sh = u->ship;
    /* An Land kein NACH wenn in dieser Runde Schiff VERLASSEN! */
    if (sh == NULL) {
      sh = leftship(u);
      if (sh && sh->region!=u->region) sh = NULL;
    }
    if (sh) {
      unit * guard = is_guarded(r, u, GUARD_LANDING);
      if (guard) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "region_guarded", "guard", guard));
        return;
      }
    }
    if (u->ship && u->race->flags & RCF_SWIM) {
      cmistake(u, u->thisorder, 143, MSG_MOVE);
      return;
    }
  }
  else if (u->ship && fval(u->ship, SF_MOVED)) {
    /* die Einheit ist auf einem Schiff, das sich bereits bewegt hat */
    cmistake(u, u->thisorder, 13, MSG_MOVE);
    return;
  }

  make_route(u, u->thisorder, routep);
  route_begin = *routep;

  /* und ab die post: */
  travel_i(u, route_begin, NULL, u->thisorder, TRAVEL_NORMAL, &followers);

  /* followers */
  while (followers!=NULL) {
    follower * fnext = followers->next;
    unit * uf = followers->uf;
    unit * ut = followers->ut;
    const region_list * route_end = followers->route_end;
    
    free(followers);
    followers = fnext;

    if (uf->region == r) {
      order * follow_order;
      const struct locale * lang = u->faction->locale;

      /* construct an order */
      follow_order = create_order(K_FOLLOW, lang, "%s %i",
        LOC(uf->faction->locale, parameters[P_UNIT]), ut->no);

      route_end = reroute(uf, route_begin, route_end);
      travel_i(uf, route_begin, route_end, follow_order, TRAVEL_FOLLOWING, &followers);
      caught_target(uf->region, uf);
      free_order(follow_order);
    }
  }

}

static void
move(unit * u, boolean move_on_land)
{
  region_list * route = NULL;

  assert(u->number);
  if (u->ship && fval(u, UFL_OWNER)) {
    sail(u, u->thisorder, move_on_land, &route);
  } else {
    travel(u, &route);
  }

  fset(u, UFL_LONGACTION|UFL_NOTMOVING);
  set_order(&u->thisorder, NULL);

  if (route!=NULL) free_regionlist(route);
}

typedef struct piracy_data {
  const struct faction * pirate;
  const struct faction * target;
  direction_t dir;
} piracy_data;

static void 
piracy_init(struct attrib * a)
{
  a->data.v = calloc(1, sizeof(piracy_data));
}

static void 
piracy_done(struct attrib * a)
{
  free(a->data.v);
}

static attrib_type at_piracy_direction = {
	"piracy_direction",
	piracy_init,
	piracy_done,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

static attrib * 
mk_piracy(const faction * pirate, const faction * target, direction_t target_dir)
{
  attrib * a = a_new(&at_piracy_direction);
  piracy_data * data = a->data.v;
  data->pirate = pirate;
  data->target = target;
  data->dir = target_dir;
  return a;
}

static void
piracy_cmd(unit *u, struct order * ord)
{
  region *r = u->region;
  ship *sh = u->ship, *sh2;
  direction_t dir, target_dir = NODIRECTION;
  struct {
    const faction * target;
    int value;
  } aff[MAXDIRECTIONS];
  int saff = 0;
  int *il = NULL;
  const char *s;
  attrib *a;
  
  if (!sh) {
    cmistake(u, ord, 144, MSG_MOVE);
    return;
  }
  
  if (!fval(u, UFL_OWNER)) {
    cmistake(u, ord, 146, MSG_MOVE);
    return;
  }
  
  /* Feststellen, ob schon ein anderer alliierter Pirat ein
   * Ziel gefunden hat. */
  
  
  init_tokens(ord);
  skip_token();
  s = getstrtoken();
  if (s!=NULL && *s) {
    il = intlist_init();
    while (s && *s) {
      il = intlist_add(il, atoi36(s));
      s = getstrtoken();
    }
  }

  for (a = a_find(r->attribs, &at_piracy_direction); a && a->type==&at_piracy_direction; a=a->next) {
    piracy_data * data = a->data.v;
    const faction * p = data->pirate;
    const faction * t = data->target;

    if (alliedunit(u, p, HELP_FIGHT)) {
      if (il == 0 || (t && intlist_find(il, t->no))) {
        target_dir = data->dir;
        break;
      }
    }
  }

  /* Wenn nicht, sehen wir, ob wir ein Ziel finden. */

  if (target_dir == NODIRECTION) {
    /* Einheit ist also Kapit�n. Jetzt gucken, in wievielen
     * Nachbarregionen potentielle Opfer sind. */

    for(dir = 0; dir < MAXDIRECTIONS; dir++) {
      region *rc = rconnect(r, dir);
      aff[dir].value = 0;
      aff[dir].target = 0;
      if (rc && fval(rc->terrain, SWIM_INTO)
          && check_takeoff(sh, r, rc) == true) {
        
        for (sh2 = rc->ships; sh2; sh2 = sh2->next) {
          unit * cap = shipowner(sh2);
          if (cap) {
            faction * f = visible_faction(cap->faction, cap);
            if (alliedunit(u, f, HELP_FIGHT)) continue;
            if (il == 0 || intlist_find(il, cap->faction->no)) {
              ++aff[dir].value;
              if (rng_int() % aff[dir].value == 0) {
                aff[dir].target = f;
              }
            }
          }
        }

        /* Und aufaddieren. */
        saff += aff[dir].value;
      }
    }
    
    if (saff != 0) {
      saff = rng_int() % saff;
      for (dir=0; dir!=MAXDIRECTIONS; ++dir) {
        if (saff!=aff[dir].value) break;
        saff -= aff[dir].value;
      }
      target_dir = dir;
      a = a_add(&r->attribs, mk_piracy(u->faction, aff[dir].target, target_dir));
    }
  }
  
  free(il);
  
  /* Wenn kein Ziel gefunden, entsprechende Meldung generieren */
  if (target_dir == NODIRECTION) {
    ADDMSG(&u->faction->msgs, msg_message("piratenovictim", 
                                          "ship region", sh, r));
    return;
  }
  
  /* Meldung generieren */
  ADDMSG(&u->faction->msgs, msg_message("piratesawvictim",
                                        "ship region dir", sh, r, target_dir));
  
  /* Befehl konstruieren */
  set_order(&u->thisorder, create_order(K_MOVE, u->faction->locale, "%s",
    LOC(u->faction->locale, directions[target_dir])));
  
  /* Bewegung ausf�hren */
  init_tokens(u->thisorder);
  skip_token();
  move(u, true);
}

static void
age_traveldir(region *r)
{
	attrib *a = a_find(r->attribs, &at_traveldir);

	while(a && a->type==&at_traveldir) {
    attrib *an = a->next;
		a->data.ca[3]--;
		if(a->data.ca[3] <= 0) {
			a_remove(&r->attribs, a);
		}
    a = an;
	}
}

static direction_t
hunted_dir(attrib *at, int id)
{
  attrib *a = a_find(at, &at_shiptrail);
  direction_t d = NODIRECTION;

  while (a!=NULL && a->type==&at_shiptrail) {
    traveldir *t = (traveldir *)(a->data.v);
    if (t->no == id) {
      d = t->dir;
      /* do not break, because we want the last one for this ship */
    }
    a = a->next;
  }

  return d;
}

static int
hunt(unit *u, order * ord)
{
  region *rc = u->region;
  int bytes, moves, id, speed;
  char command[256], * bufp = command;
  size_t size = sizeof(command);
  direction_t dir;

  if (fval(u, UFL_NOTMOVING)) {
    return 0;
  } else if (u->ship == NULL) {
    cmistake(u, ord, 144, MSG_MOVE);
    fset(u, UFL_LONGACTION|UFL_NOTMOVING); /* FOLGE SCHIFF ist immer lang */
    return 0;
  } else if (!fval(u, UFL_OWNER)) {
    cmistake(u, ord, 146, MSG_MOVE);
    fset(u, UFL_LONGACTION|UFL_NOTMOVING); /* FOLGE SCHIFF ist immer lang */
    return 0;
  } else if (!can_move(u)) {
    cmistake(u, ord, 55, MSG_MOVE);
    fset(u, UFL_LONGACTION|UFL_NOTMOVING); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  id = getshipid();

  if (id <= 0) {
    cmistake(u,  ord, 20, MSG_MOVE);
    fset(u, UFL_LONGACTION|UFL_NOTMOVING); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  dir = hunted_dir(rc->attribs, id);

  if (dir == NODIRECTION) {
    ship * sh = findship(id);
    if (sh == NULL || sh->region!=rc) {
      cmistake(u, ord, 20, MSG_MOVE);
    }
    fset(u, UFL_LONGACTION|UFL_NOTMOVING); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  bufp = command;
  bytes = snprintf(bufp, size, "%s %s", LOC(u->faction->locale, keywords[K_MOVE]),
    LOC(u->faction->locale, directions[dir]));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  moves = 1;

  speed = getuint();
  if (speed==0) {
    speed = shipspeed(u->ship, u);
  } else {
    int maxspeed = shipspeed(u->ship, u);
    if (maxspeed<speed) speed = maxspeed;
  }
  rc = rconnect(rc, dir);
  while (moves < speed && (dir = hunted_dir(rc->attribs, id)) != NODIRECTION) 
  {
    bytes = (int)strlcpy(bufp, " ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, LOC(u->faction->locale, directions[dir]), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    moves++;
    rc = rconnect(rc, dir);
  }

  /* In command steht jetzt das NACH-Kommando. */

  /* NACH ignorieren und Parsing initialisieren. */
  igetstrtoken(command);
  /* NACH ausf�hren */
  move(u, false);
  return 1; /* true -> Einheitenliste von vorne durchgehen */
}

void
destroy_damaged_ships(void)
{
	region *r;
	ship   *sh,*shn;

	for(r=regions;r;r=r->next) {
		for(sh=r->ships;sh;) {
			shn = sh->next;
			if (sh->damage>=sh->size * DAMAGE_SCALE) {
				remove_ship(&sh->region->ships, sh);
			}
			sh = shn;
		}
	}
}

/* Bewegung, Verfolgung, Piraterie */

/** ships that folow other ships
 * Dann generieren die jagenden Einheiten ihre Befehle und
 * bewegen sich. 
 * Following the trails of other ships.
 */
static void
move_hunters(void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit ** up = &r->units;

    while (*up!=NULL) {
      unit * u = *up;

      if (!fval(u, UFL_MOVED|UFL_NOTMOVING)) {
        order * ord;

        for (ord=u->orders;ord;ord=ord->next) {
          if (get_keyword(ord) == K_FOLLOW) {
            param_t p;

            init_tokens(ord);
            skip_token();
            p = getparam(u->faction->locale);
            if (p != P_SHIP) {
              if (p != P_UNIT) {
                cmistake(u, ord, 240, MSG_MOVE);
              }
              break;
            }

            /* wir folgen definitiv einem Schiff. */

            if (fval(u, UFL_NOTMOVING)) {
              cmistake(u, ord, 52, MSG_MOVE);
              break;
            } else if (!can_move(u)) {
              cmistake(u, ord, 55, MSG_MOVE);
              break;
            }

            if (!fval(u, UFL_NOTMOVING) && !LongHunger(u) && hunt(u, ord)) {
              up = &r->units;
              break;
            }
          }
        }
      }
      if (*up == u) up=&u->next;
    }
  }
}

/** Piraten and Drift 
 * 
 */
static void
move_pirates(void)
{
  region * r;

  for (r = regions; r; r = r->next) {
    unit ** up = &r->units;

    while (*up) {
      unit *u = *up;

      if (!fval(u, UFL_NOTMOVING) && get_keyword(u->thisorder) == K_PIRACY) {
        piracy_cmd(u, u->thisorder);
        fset(u, UFL_LONGACTION|UFL_NOTMOVING);
      }

      if (*up == u) {
        /* not moved, use next unit */
        up = &u->next;
      } else if (*up && (*up)->region!=r) {
        /* moved the previous unit along with u (units on same ship)
        * must start from the beginning again */
        up = &r->units;
      }
      /* else *up is already the next unit */
    }

    a_removeall(&r->attribs, &at_piracy_direction);
    age_traveldir(r);
  }
}

void
movement(void)
{
  int ships;

  /* Initialize the additional encumbrance by transported units */
  init_transportation();

  /* Move ships in last phase, others first 
  * This is to make sure you can't land someplace and then get off the ship
  * in the same turn.
  */
  for (ships=0;ships<=1;++ships) {
    region * r = regions;
    while (r!=NULL) {
      unit ** up = &r->units;
      boolean repeat = false;

      while (*up) {
        unit *u = *up;
        keyword_t kword;
 
        if (u->ship && fval(u->ship, SF_DRIFTED)) {
          up = &u->next;
          continue;
        }
        kword = get_keyword(u->thisorder);

        switch (kword) {
        case K_ROUTE:
        case K_MOVE:
          /* after moving, the unit has no thisorder. this prevents
           * it from moving twice (or getting error messages twice).
           * UFL_NOTMOVING is set in combat if the unit is not allowed
           * to move because it was involved in a battle.
           */
          if (fval(u, UFL_NOTMOVING)) {
            if (fval(u, UFL_LONGACTION)) {
              cmistake(u, u->thisorder, 52, MSG_MOVE);
              set_order(&u->thisorder, NULL);
            } else {
              cmistake(u, u->thisorder, 319, MSG_MOVE);
              set_order(&u->thisorder, NULL);
            }
          } else if (fval(u, UFL_MOVED)) {
            cmistake(u, u->thisorder, 187, MSG_MOVE);
            set_order(&u->thisorder, NULL);
          } else if (!can_move(u)) {
            cmistake(u, u->thisorder, 55, MSG_MOVE);
            set_order(&u->thisorder, NULL);
          } else {
            if (ships) {
              if (u->ship && fval(u, UFL_OWNER)) {
                init_tokens(u->thisorder);
                skip_token();
                move(u, false);
              }
            } else {
              if (u->ship == NULL || !fval(u, UFL_OWNER)) {
                init_tokens(u->thisorder);
                skip_token();
                move(u, false);
              }
            }
          }
          break;
        }
        if (u->region == r) {
          /* not moved, use next unit */
          up = &u->next;
        } else {
          if (*up && (*up)->region!=r) {
            /* moved the upcoming unit along with u (units on ships or followers,
            * for example). must start from the beginning again immediately */
            up = &r->units;
            repeat = false;
          } else {
            repeat = true;
          }
        }
        /* else *up is already the next unit */
      }
      if (repeat) continue;
      if (ships == 0) {
        /* Abtreiben von besch�digten, unterbemannten, �berladenen Schiffen */
        drifting_ships(r);
      }
      r = r->next;
    }
  }

  move_hunters();
  move_pirates();
}

/** Overrides long orders with a FOLLOW order if the target is moving.
 * FOLLOW SHIP is a long order, and doesn't need to be treated in here.
 */
void
follow_unit(unit * u)
{
  region * r = u->region;
  attrib * a = NULL;
  order * ord;

  if (fval(u, UFL_NOTMOVING) || LongHunger(u)) return;

  for (ord=u->orders;ord;ord=ord->next) {
    const struct locale * lang = u->faction->locale;

    if (get_keyword(ord) == K_FOLLOW) {
      init_tokens(ord);
      skip_token();
      if (getparam(lang) == P_UNIT) {
        int id = read_unitid(u->faction, r);

        if (a!=NULL) {
          a = a_find(u->attribs, &at_follow);
        }

        if (id>0) {
          unit * uf = findunit(id);
          if (!a) {
            a = a_add(&u->attribs, make_follow(uf));
          } else {
            a->data.v = uf;
          }
        } else if (a) {
          a_remove(&u->attribs, a);
          a = NULL;
        }
      }
    }
  }

  if (a && !fval(u, UFL_MOVED|UFL_NOTMOVING)) {
    unit * u2 = a->data.v;
    boolean follow = false;

    if (!u2 || u2->region!=r || !cansee(u->faction, r, u2, 0)) {
      return;
    }

    switch (get_keyword(u2->thisorder)) {
      case K_MOVE:
      case K_ROUTE:
      case K_DRIVE:
        follow = true;
        break;
      default:
        for (ord=u2->orders;ord;ord=ord->next) {
          switch (get_keyword(ord)) {
            case K_FOLLOW:
            case K_PIRACY:
              follow = true;
              break;
            default:
              continue;
          }
          break;
        }
        break;
    }
    if (!follow) {
      attrib * a2 = a_find(u2->attribs, &at_follow);
      if (a2!=NULL) {
        unit * u3 = a2->data.v;
        follow = (u3 && u2->region == u2->region);
      }
    }
    if (follow) {
      fset(u, UFL_FOLLOWING);
      fset(u2, UFL_FOLLOWED);
      set_order(&u->thisorder, NULL);
    }
  }
}
