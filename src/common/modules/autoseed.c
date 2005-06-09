/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <kernel/eressea.h>
#include "autoseed.h"

/* kernel includes */
#include <kernel/alliance.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/plane.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/unit.h>

/* util includes */
#include <util/base36.h>
#include <util/sql.h>
#include <util/goodies.h>
#include <util/sql.h>

/* libc includes */
#include <limits.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>


static int
count_demand(const region *r)
{
  struct demand *dmd;
  int c = 0;
  if (r->land) {
    for (dmd=r->land->demands;dmd;dmd=dmd->next) c++;
  }
  return c;
}

static int
recurse_regions(region *r, region_list **rlist, boolean(*fun)(const region *r))
{
  if (!fun(r)) return 0;
  else {
    int len = 0;
    direction_t d;
    region_list * rl = calloc(sizeof(region_list), 1);
    rl->next = *rlist;
    rl->data = r;
    (*rlist) = rl;
    fset(r, FL_MARK);
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * nr = rconnect(r, d);
      if (nr && !fval(nr, FL_MARK)) len += recurse_regions(nr, rlist, fun);
    }
    return len+1;
  }
}

static int maxluxuries = 0;

static boolean
f_nolux(const region * r)
{
  if (r->land && count_demand(r) != maxluxuries) return true;
  return false;
}

int
fix_demand(region *r)
{
  region_list *rl, *rlist = NULL;
  static const struct luxury_type **mlux = 0, ** ltypes;
  const luxury_type *sale = NULL;
  int maxlux = 0;
  
  if (maxluxuries==0) for (sale=luxurytypes;sale;sale=sale->next) {
    ++maxluxuries;
  }
  recurse_regions(r, &rlist, f_nolux);
  if (mlux==0) {
    int i = 0;
    mlux = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
    ltypes = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
    for (sale=luxurytypes;sale;sale=sale->next) {
      ltypes[i++] = sale;
    }
  }
  else {
    int i;
    for (i=0;i!=maxluxuries;++i) mlux[i] = 0;
  }
  for (rl=rlist;rl;rl=rl->next) {
    region * r = rl->data;
    direction_t d;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * nr = rconnect(r, d);
      if (nr && nr->land && nr->land->demands) {
        struct demand * dmd;
        for (dmd = nr->land->demands;dmd;dmd=dmd->next) {
          if (dmd->value == 0) {
            int i;
            for (i=0;i!=maxluxuries;++i) {
              if (mlux[i]==NULL) {
                maxlux = i;
                mlux[i] = dmd->type;
                break;
              } else if (mlux[i]==dmd->type) {
                break;
              }
            }
            break;
          }
        }
      }
    }
    freset(r, FL_MARK); /* undo recursive marker */
  }
  if (maxlux<2) {
    int i;
    for (i=maxlux;i!=2;++i) {
      int j;
      do {
        int k = rand() % maxluxuries;
        mlux[i] = ltypes[k];
        for (j=0;j!=i;++j) {
          if (mlux[j]==mlux[i]) break;
        }
      } while (j!=i);
    }
    maxlux=2;
  }
  for (rl=rlist;rl;rl=rl->next) {
    region * r = rl->data;
    if (!fval(r, RF_CHAOTIC)) {
      log_warning(("fixing demand in %s\n", regionname(r, NULL)));
    }
    setluxuries(r, mlux[rand() % maxlux]);
  }
  while (rlist) {
    rl = rlist->next;
    free(rlist);
    rlist = rl;
  }
  return 0;
}

newfaction *
read_newfactions(const char * filename)
{
  newfaction * newfactions = NULL;
  FILE * F = fopen(filename, "r");
  if (F==NULL) return NULL;
  for (;;) {
    faction * f;
    char race[20], email[64], lang[8], password[16];
    newfaction *nf, **nfi;
    int bonus, subscription;
    int alliance = 0;

    if (alliances!=NULL) {
      /* email;race;locale;startbonus;subscription;alliance */
      if (fscanf(F, "%s %s %s %d %d %s %d", email, race, lang, &bonus, 
                 &subscription, password, &alliance)<=0) break;
    } else {
      /* email;race;locale;startbonus;subscription */
      if (fscanf(F, "%s %s %s %d %d %s", email, race, lang, &bonus, 
                 &subscription, password)<=0) break;
    }
    for (f=factions;f;f=f->next) {
      if (strcmp(f->email, email)==0 && f->subscription) break;
    }
    if (f && f->units) continue; /* skip the ones we've already got */
    for (nf=newfactions;nf;nf=nf->next) {
      if (strcmp(nf->email, email)==0) break;
    }
    if (nf) continue;
    nf = calloc(sizeof(newfaction), 1);
    if (set_email(&nf->email, email)!=0) {
      log_error(("Invalid email address for subscription %s: %s\n", 
                 itoa36(subscription), email));
      continue;
    }
    nf->password = strdup(password);
    nf->race = rc_find(race);
    nf->subscription = subscription;
    if (alliances!=NULL) {
      struct alliance * al = findalliance(alliance);
      if (al==NULL) {
        char zText[64];
        sprintf(zText, "Allianz %d", alliance);
        al = makealliance(alliance, zText);
      }
      nf->allies = al;
    } else {
      nf->allies = NULL;
    }
    if (nf->race==NULL) nf->race = findrace(race, default_locale);
    nf->lang = find_locale(lang);
    nf->bonus = bonus;
    assert(nf->race && nf->email && nf->lang);
    nfi = &newfactions;
    while (*nfi) {
      if ((*nfi)->race==nf->race) break;
      nfi=&(*nfi)->next;
    }
    nf->next = *nfi;
    *nfi = nf;
  }
  fclose(F);
  return newfactions;
}

typedef struct seed_t {
	struct region * region;
	struct newfaction * player;
	struct seed_t * next[MAXDIRECTIONS];
} seed_t;

double
get_influence(struct seed_t * seed, struct seed_t * target)
{
	double q = 0.0;
	direction_t d;
	if (target==0 || seed==0) return q;
	for (d=0;d!=MAXDIRECTIONS;++d) {
		seed_t * s = seed->next[d];
		if (s==target) {
			if (target->player) {
				if (seed->player) {
					/* neighbours bad. */
					q -= 4.0;
					if (seed->player->race==target->player->race) {
						/* don't want similar people next to each other */
						q -= 2.0;
					} else if (seed->player->race==new_race[RC_ELF]) {
						if (target->player->race==new_race[RC_TROLL]) {
							/* elf sitting next to troll: poor troll */
							q -= 5.0;
						}
					}
				}
				else if (target->player) {
					/* empty regions good */
					q += 1.0;
				}
			}
		}
	}
	return q;
}

double
get_quality(struct seed_t * seed)
{
	double q = 0.0;
	int nterrains[MAXTERRAINS];
	direction_t d;

	memset(nterrains, 0, sizeof(nterrains));
	for (d=0;d!=MAXDIRECTIONS;++d) {
		seed_t * ns = seed->next[d];
		if (ns) ++nterrains[rterrain(ns->region)];
	}
	++nterrains[rterrain(seed->region)];

	if (nterrains[T_MOUNTAIN]) {
		/* 5 points for the first, and 2 points for every other mountain */
		q += (3+nterrains[T_MOUNTAIN]*2);
	} else if (seed->player->race==new_race[RC_DWARF]) {
		/* -10 for a dwarf who isn't close to a mountain */
		q -= 10.0;
	}

	if (nterrains[T_PLAIN]) {
		/* 5 points for the first, and 2 points for every other plain */
		q += (3+nterrains[T_PLAIN]*2);
	} else if (seed->player->race==new_race[RC_ELF]) {
		/* -10 for an elf who isn't close to a plain */
		q -= 10.0;
	}

	if (nterrains[T_DESERT]) {
		/* +10 points for the first if we are insects, and -2 points for every
		 * other desert */
		if (seed->player->race==new_race[RC_INSECT]) q += 12;
		q -= (nterrains[T_DESERT]*2);
	}

	switch (rterrain(seed->region)) {
	case T_DESERT:
		q -=8.0;
		break;
	case T_VOLCANO:
		q -=20.0;
		break;
	case T_GLACIER:
		q -=10.0;
		break;
	case T_SWAMP:
		q -= 1.0;
		break;
	case T_PLAIN:
		q += 4.0;
		break;
	case T_MOUNTAIN:
		q += 5.0;
		break;
	}

	return 2.0;
}

double
recalc(seed_t * seeds, int nseeds, int nplayers)
{
	int i, p = 0;
	double quality = 0.0, q = 0.0, *qarr = malloc(sizeof(int) * nplayers);
	for (i=0;i!=nseeds;++i) {
		if (seeds[i].player) {
			double quality = get_quality(seeds+i);
			direction_t d;
			for (d=0;d!=MAXDIRECTIONS;++d) {
				quality += get_influence(seeds[i].next[d], seeds+i);
			}
			qarr[p++] = quality;
			q += quality;
		}
	}

	return quality + q;
}

extern int numnewbies;

static terrain_t
preferred_terrain(const struct race * rc)
{
	if (rc==rc_find("dwarf")) return T_MOUNTAIN;
	if (rc==rc_find("insect")) return T_DESERT;
	if (rc==rc_find("halfling")) return T_SWAMP;
	if (rc==rc_find("troll")) return T_MOUNTAIN;
	return T_PLAIN;
}

#define REGIONS_PER_FACTION 2
#define PLAYERS_PER_ISLAND 20
#define TURNS_PER_ISLAND 3
#define MINFACTIONS 1
#define MAXAGEDIFF 5
#define VOLCANO_CHANCE 100

static boolean
virgin_region(const region * r)
{
  direction_t d;
  if (r==NULL) return true;
  if (r->age>MAXAGEDIFF) return false;
  if (r->units) return false;
  for (d=0;d!=MAXDIRECTIONS;++d) {
    const region * rn = rconnect(r, d);
    if (rn) {
      if (rn->age>MAXAGEDIFF) return false;
      if (rn->units) return false;
    }
  }
  return true;
}

void
get_island(region * root, region_list ** rlist)
{
  region_list ** rnext = rlist;
  while (*rnext) rnext=&(*rnext)->next;

  fset(root, FL_MARK);
  add_regionlist(rnext, root);

  while (*rnext) {
    direction_t dir;

    region * rcurrent = (*rnext)->data;
    rnext = &(*rnext)->next;

    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
      region * r = rconnect(rcurrent, dir);
      if (r!=NULL && r->land && !fval(r, FL_MARK)) {
        fset(r, FL_MARK);
        add_regionlist(rnext, r);
      }
    }
  }
  rnext=rlist;
  while (*rnext) {
    region_list * rptr = *rnext;
    freset(rptr->data, FL_MARK);
    rnext = &rptr->next;
  }
}

/** create new island with up to nsize players
 * returns the number of players placed on the new island.
 */
int
autoseed(newfaction ** players, int nsize)
{
	short x = 0, y = 0;
	region * r = NULL;
	region_list * rlist = NULL;
	int rsize, tsize = 0;
  int isize = REGIONS_PER_FACTION; /* target size for the island */
  int psize = 0; /* players on this island */

  if (listlen(*players)<MINFACTIONS) return 0;

  if (turn % TURNS_PER_ISLAND) {
    region * rmin = NULL;
    /* find a spot that's adjacent to the previous island, but virgin.
     * like the last land virgin ocean region adjacent to land.
     */
    for (r=regions;r;r=r->next) {
      struct plane * p = rplane(r);
      if (r->terrain==T_OCEAN && p==NULL && virgin_region(r)) {
        direction_t d;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * rn = rconnect(r, d);
          if (rn && rn->land) {
            rmin = rn;
            break;
          }
        }
      }
    }
    if (rmin!=NULL) {
      region_list * rlist = NULL, * rptr;
      faction * f;
      get_island(rmin, &rlist);
      for (rptr=rlist;rptr;rptr=rptr->next) {
        region * r = rlist->data;
        unit * u;
        for (u=r->units;u;u=u->next) {
          f = u->faction;
          if (!fval(f, FL_MARK)) {
            ++psize;
            fset(f, FL_MARK);
          }
        }
      }
      free_regionlist(rlist);
      if (psize>0) for (f=factions;f;f=f->next) freset(f, FL_MARK);
      if (psize<PLAYERS_PER_ISLAND) {
        r = rmin;
      }
    }
  }
  if (r==NULL) {
    region * rmin = NULL;
    direction_t dmin = MAXDIRECTIONS;
    /* find an empty spot.
     * rmin = the youngest ocean region that has a missing neighbour 
     * dmin = direction in which it's empty
     */
    for (r=regions;r;r=r->next) {
      struct plane * p = rplane(r);
      if (r->terrain==T_OCEAN && p==0 && (rmin==NULL || r->age<=MAXAGEDIFF)) {
        direction_t d;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          if (rconnect(r, d)==NULL) break;
        }
        if (d!=MAXDIRECTIONS) {
          rmin=r;
          dmin=d;
        }
      }
    }

    /* create a new region where we found the empty spot, and make it the first 
    * in our island. island regions are kept in rlist, so only new regions can 
    * get populated, and old regions are not overwritten */
    if (rmin!=NULL) {
      assert(virgin_region(rconnect(rmin, dmin)));
      x = rmin->x + delta_x[dmin];
      y = rmin->y + delta_y[dmin];
    }
    r = new_region(x, y);
    terraform(r, T_OCEAN); /* we change the terrain later */
  }

  add_regionlist(&rlist, r);
  fset(r, FL_MARK);
  rsize = 1;

  while (rsize && (nsize || isize>=REGIONS_PER_FACTION)) {
    int i = rand() % rsize;
    region_list ** rnext = &rlist;
    region_list * rfind;
    direction_t d;
    while (i--) rnext=&(*rnext)->next;
    rfind = *rnext;
    r = rfind->data;
    freset(r, FL_MARK);
    *rnext = rfind->next;
    free(rfind);
    --rsize;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * rn = rconnect(r, d);
      if (rn && fval(rn, FL_MARK)) continue;
      if (virgin_region(rn)) {
        if (rn==NULL) {
          rn = new_region(r->x + delta_x[d], r->y + delta_y[d]);
          terraform(rn, T_OCEAN);
        }
        add_regionlist(&rlist, rn);
        fset(rn, FL_MARK);
        ++rsize;
      }
    }
    if (rand() % VOLCANO_CHANCE == 0) {
      terraform(r, T_VOLCANO);
    } else if (nsize && (rand() % isize == 0 || rsize==0)) {
      newfaction ** nfp, * nextf = *players;
      faction * f;
      unit * u;

      isize += REGIONS_PER_FACTION;
      terraform(r, preferred_terrain(nextf->race));
      ++tsize;
      assert(r->land && r->units==0);
      u = addplayer(r, addfaction(nextf->email, nextf->password, nextf->race,
                                  nextf->lang, nextf->subscription));
      f = u->faction;
			f->alliance = nextf->allies;
			log_printf("New faction (%s), %s at %s\n", itoa36(f->no),
								 f->email, regionname(r, NULL));
      if (f->subscription) {
        sql_print(("UPDATE subscriptions SET status='ACTIVE', faction='%s', lastturn=%d, password='%s' WHERE id=%u;\n",
          factionid(f), f->lastorders, f->override, f->subscription));
      }

      /* remove duplicate email addresses */
      nfp = players;
      while (*nfp) {
        newfaction * nf = *nfp;
        if (strcmp(nextf->email, nf->email)==0) {
          *nfp = nf->next;
          if (nextf!=nf) free(nf);
        }
        else nfp = &nf->next;
      }
      ++psize;
      --nsize;
      --isize;
      if (psize>=PLAYERS_PER_ISLAND) break;
    } else {
      terraform(r, (terrain_t)((rand() % T_GLACIER)+1));
      --isize;
    }
  }

  while (rlist) {
    region_list * self = rlist;
    rlist = rlist->next;
    freset(self->data, FL_MARK);
    free(self);
  }

  if (r!=NULL) {
    /* reicht das? */
    fix_demand(r);
  }
  
  if (nsize!=0) {
    log_error(("Could not place all factions on the same island as requested\n"));
  }
  
  
  if (rlist) {
#define MINOCEANDIST 3
#define MAXFILLDIST 10
#define SPECIALCHANCE 80
    region_list ** rbegin = &rlist;
    int i;
    int special = 1;
    
    for (i=0;i!=MINOCEANDIST;++i) {
      region_list ** rend = rbegin;
      while (*rend) rend=&(*rend)->next;
      while (rbegin!=rend) {
        direction_t d;
        region * r = (*rbegin)->data;
        rbegin=&(*rbegin)->next;
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * rn = rconnect(r, d);
          if (rn==NULL) {
            terrain_t terrain = T_OCEAN;
            rn = new_region(r->x + delta_x[d], r->y + delta_y[d]);
            if (rand() % SPECIALCHANCE < special) {
              terrain = (terrain_t)(1 + rand() % T_GLACIER);
              special = SPECIALCHANCE / 3; /* 33% chance auf noch eines */
            } else {
              special = 1;
            }
						terraform(rn, terrain);
            /* the new region has an extra 15% chance to have laen */
            if (rand() % 100 < 15) rsetlaen(r, 5 + rand() % 5);
            /* the new region has an extra 20% chance to have mallorn */
            if (rand() % 100 < 20) fset(r, RF_MALLORN);
            add_regionlist(rend, rn);
          }
        }
      }
      
    }
    while (*rbegin) {
      region * r = (*rbegin)->data;
      direction_t d;
      rbegin=&(*rbegin)->next;
      for (d=0;d!=MAXDIRECTIONS;++d) if (rconnect(r, d)==NULL) {
        short i;
        for (i=1;i!=MAXFILLDIST;++i) {
          if (findregion(r->x + i*delta_x[d], r->y + i*delta_y[d]))
            break;
          
        }
        if (i!=MAXFILLDIST) {
          while (--i) {
            region * rn = new_region(r->x + i*delta_x[d], r->y + i*delta_y[d]);
            terraform(rn, T_OCEAN);
          }
        }
      }
    }
  }
  return tsize;
}
