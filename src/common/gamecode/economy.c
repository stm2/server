/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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

#define RESERVE_DONATIONS 1 /* shall we reserve objects given to us by other factions? */
#define RESERVE_GIVE 1 /* reserve anything that's given from one unit to another? */
#define TWOPASS_GIVE 0 /* give items in two passes: first to others, than to self */

#include <config.h>
#include "eressea.h"
#include "economy.h"
#include "randenc.h"

#include <ship.h>
#include <unit.h>
#include <building.h>
#include <faction.h>
#include <plane.h>
#include <alchemy.h>
#include <item.h>
#include <magic.h>
#include <build.h>
#include <goodies.h>
#include <study.h>
#include <movement.h>
#include <race.h>
#include <spy.h>
#include <build.h>
#include <pool.h>
#include <region.h>
#include <resources.h>
#include <unit.h>
#include <skill.h>
#include <message.h>
#include <reports.h>
#include <karma.h>

/* gamecode includes */
#include "laws.h"

/* util includes */
#include <attrib.h>
#include <base36.h>
#include <event.h>
#include <util/message.h>

/* libs includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include <attributes/reduceproduction.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>

#if GROWING_TREES
# include <items/seed.h>
#endif

/* - static global symbols ------------------------------------- */
typedef struct spende {
	struct spende *next;
	struct faction *f1, *f2;
	struct region *region;
	int betrag;
} spende;
static spende *spenden;
/* ------------------------------------------------------------- */

static request workers[1024];
static request *nextworker;
static int working;

static request entertainers[1024];
static request *nextentertainer;
static int entertaining;

static int norders;
static request *oa;

int
income(const unit * u)
{
	switch(old_race(u->race)) {
	case RC_FIREDRAGON:
		return 150 * u->number;
	case RC_DRAGON:
		return 1000 * u->number;
	case RC_WYRM:
		return 5000 * u->number;
	}
	return 20 * u->number;
}

/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */

static struct scramble {
	int index;
	int rnd;
} * vec;

int
scramblecmp(const void *p1, const void *p2)
{
	return ((struct scramble *)p1)->rnd - ((struct scramble *)p2)->rnd;
}

void
scramble(void *data, int n, size_t width)
{
	int i;
	static int vecsize = 0;
	void *buffer = NULL, *temp = NULL;

	if (n > vecsize) {
		vecsize = n;
		vec = (struct scramble *) realloc(vec, vecsize * sizeof(struct scramble));
	}
	for (i = 0; i != n; i++) {
		vec[i].rnd = rand();
		vec[i].index = i;
	}
	qsort(vec, n, sizeof(struct scramble), scramblecmp);
	for (i = 0; i != n; i++) {
		/* in vec[k].index steht, wohin der block k soll */
		/* src soll nach target geschoben werden. daf�r wird target gemerkt,
		 * src verschoben, und target zum neuen src. ende, wenn k wieder i ist */
		if (vec[i].index!=i) {
			char * src = ((char*)data)+width*i;
			int k = i;
			int dest = vec[k].index;
			if (temp==NULL) {
				temp = malloc(width);
			}
			buffer = temp;
			do {
				char * target = ((char*)data)+width*dest;
				memcpy(buffer, target, width);
				memcpy(target, src, width);
				k = dest; /* wo das gerettete target hin soll */
				dest = vec[dest].index;
				vec[k].index = k; /* dest ist an der richtigen stelle */
				/* swap buffer and src. misuse target as intermediate var. */
				target = buffer;
				buffer = src;
				src = target;
			} while (vec[i].index!=i);
		}
	}
	if (buffer!=NULL) free(temp);
}
#if 0
#define MAX 6
int oi[MAX];

void
test_scramble(void)
{
	int i;
	for (i=0;i!=MAX;++i) {
		oi[i] = i;
	}
	scramble(oi, MAX, sizeof(int));
	for (i=0;i!=MAX;++i) {
		int j;
		for (j=0;j!=MAX;++j) {
			if (oi[j]==i) break;
		}
		assert(j!=MAX);
	}
}
#endif
static void
expandorders(region * r, request * requests)
{
	int i, j;
	unit *u;
	request *o;

	/* Alle Units ohne request haben ein -1, alle units mit orders haben ein
	 * 0 hier stehen */

	for (u = r->units; u; u = u->next)
		u->n = -1;

	norders = 0;

	for (o = requests; o; o = o->next)
		if (o->qty > 0) {
			norders += o->qty;
		}

	if (norders > 0) {
		oa = (request *) calloc(norders, sizeof(request));
		i = 0;
		for (o = requests; o; o = o->next) {
			if (o->qty > 0) {
				for (j = o->qty; j; j--) {
					oa[i] = *o;
					oa[i].unit->n = 0;
					i++;
				}
			}
		}
		scramble(oa, norders, sizeof(request));
	} else {
		oa = NULL;
	}
#if 0
	freelist(requests);
#else
	while (requests) {
		request * o = requests->next;
		free(requests);
		requests = o;
	}
#endif
}
/* ------------------------------------------------------------- */

#if SKILLPOINTS == 0

static void
change_level(unit * u, skill_t sk, int bylevel)
{
	skill * sv = get_skill(u, sk);
	assert(bylevel>0);
	if (sv==0) set_skill(u, sk, bylevel, 0);
	else {
		sv->level = (unsigned char)(sv->level+bylevel);
		sv->learning = 0;
	}
}
#endif

static void
expandrecruit(region * r, request * recruitorders)
{
	/* Rekrutierung */

	int i, n, p = rpeasants(r), h = rhorses(r), uruks = 0;
	int rfrac = p / RECRUITFRACTION;
	unit * u;

	expandorders(r, recruitorders);
	if (!norders) return;

	for (i = 0, n = 0; i != norders; i++) {
		unit * u = oa[i].unit;
		const race * rc = u->faction->race;
		int recruitcost = rc->recruitcost;

		/* check if recruiting is limited.
		 * either horses or peasant fraction or not at all */
		if ((rc->ec_flags & ECF_REC_UNLIMITED)==0) {
			/* not unlimited, and everything's gone: */
			if (rc->ec_flags & ECF_REC_HORSES) {
				/* recruit from horses if not all gone */
				if (h <= 0) continue;
			} else if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
				/* recruit from peasants if any space left */
				if (n - (uruks+1)/2 >= rfrac) continue;
			}
		}
		if (recruitcost) {
			if (get_pooled(oa[i].unit, r, R_SILVER) < recruitcost) continue;
			use_pooled(oa[i].unit, r, R_SILVER, recruitcost);
		}
		if ((rc->ec_flags & ECF_REC_UNLIMITED)==0) {
			if (rc->ec_flags & ECF_REC_HORSES) h--; /* use a horse */
			else {
				if ((rc->ec_flags & ECF_REC_ETHEREAL)==0) {
					p--; /* use a peasant */
					if(rc == new_race[RC_URUK]) uruks++;
				}
				n++;
			}
		}
/*		set_number(u, u->number + 1); */
		u->race = rc;
		u->n++;
	}

	assert(p>=0 && h>=0);
	rsetpeasants(r, p+uruks/2);
	rsethorses(r, h);

	free(oa);

	for (u = r->units; u; u = u->next) {
		if (u->n >= 0) {
			unit * unew;
			if (u->number==0) unew = u;
			else unew = createunit(r, u->faction, u->n, u->race);
			if (unew->race == new_race[RC_URUK]) {
				change_level(unew, SK_SWORD, 1);
				change_level(unew, SK_SPEAR, 1);
			}
			if (unew->race->ec_flags & ECF_REC_HORSES) {
				change_level(unew, SK_RIDING, 1);
			}
			i = fspecial(unew->faction, FS_MILITIA);
			if (i > 0) {
				if (unew->race->bonus[SK_SPEAR] >= 0) 
					change_level(unew, SK_SPEAR, i);
				if (unew->race->bonus[SK_SWORD] >= 0) 
					change_level(unew, SK_SWORD, i);
				if (unew->race->bonus[SK_LONGBOW] >= 0) 
					change_level(unew, SK_LONGBOW, i);
				if (unew->race->bonus[SK_CROSSBOW] >= 0) 
					change_level(unew, SK_CROSSBOW, i);
				if (unew->race->bonus[SK_RIDING] >= 0) 
					change_level(unew, SK_RIDING, i);
				if (unew->race->bonus[SK_AUSDAUER] >= 0) 
					change_level(unew, SK_AUSDAUER, i);
			}
			if (unew!=u) {
				transfermen(unew, u, unew->number);
				destroy_unit(unew);
			}
			if (u->n < u->wants) {
				ADDMSG(&u->faction->msgs, msg_message("recruit", 
					"unit region amount want", u, r, u->n, u->wants));
			}
		}
	}
}

void
recruit(region * r, unit * u, strlist * S,
		request ** recruitorders)
{
	int n;
	plane * pl;
	request *o;
	int recruitcost;

	if (u->faction->race == new_race[RC_INSECT]) {
		if (month_season[month(0)] == 0 && rterrain(r) != T_DESERT) {
#ifdef INSECT_POTION
			boolean usepotion = false;
			unit *u2;

			for (u2 = r->units; u2; u2 = u2->next)
				if (fval(u2, FL_WARMTH)) {
					usepotion = true;
					break;
				}
			if (!usepotion)
#endif
			{
				cmistake(u, S->s, 98, MSG_EVENT);
				return;
			}
		}
		/* in Gletschern, Eisbergen gar nicht rekrutieren */
		if (r_insectstalled(r)) {
			cmistake(u, S->s, 97, MSG_EVENT);
			return;
		}
	}
	if (is_cursed(r->attribs, C_RIOT, 0)) {
		/* Die Region befindet sich in Aufruhr */
		cmistake(u, S->s, 237, MSG_EVENT);
		return;
	}

#if RACE_ADJUSTMENTS
	if (fval(r, RF_ORCIFIED) && u->faction->race != new_race[RC_URUK] &&
#else
	if (fval(r, RF_ORCIFIED) && u->faction->race != new_race[RC_ORC] &&
#endif
			!(u->faction->race->ec_flags & ECF_REC_HORSES)) {
		cmistake(u, S->s, 238, MSG_EVENT);
		return;
	}


	recruitcost = u->faction->race->recruitcost;
	if (recruitcost) {
		pl = getplane(r);
		if (pl && fval(pl, PFL_NORECRUITS)) {
			add_message(&u->faction->msgs,
				msg_error(u, S->s, "error_pflnorecruit", ""));
			return;
		}

		if (get_pooled(u, r, R_SILVER) < recruitcost) {
			cmistake(u, S->s, 142, MSG_EVENT);
			return;
		}
	}
	if (!playerrace(u->race) || idle(u->faction)) {
		cmistake(u, S->s, 139, MSG_EVENT);
		return;
	}
	/* snotlinge sollten hiermit bereits abgefangen werden, die
	 * parteirasse ist uruk oder ork*/
	if (u->race != u->faction->race) {
		if (u->number != 0) {
			cmistake(u, S->s, 139, MSG_EVENT);
			return;
		}
		else u->race = u->faction->race;
	}
	n = geti();

	if (get_skill(u, SK_MAGIC)) {
		/* error158;de;{unit} in {region}: '{command}' - Magier arbeiten
		 * grunds�tzlich nur alleine! */
		cmistake(u, S->s, 158, MSG_EVENT);
		return;
	}
	if (get_skill(u, SK_ALCHEMY)
		&& count_skill(u->faction, SK_ALCHEMY) + n >
		max_skill(u->faction, SK_ALCHEMY))
	{
		cmistake(u, S->s, 156, MSG_EVENT);
		return;
	}
	if (recruitcost) n = min(n, get_pooled(u, r, R_SILVER) / recruitcost);

	u->wants = n;

	if (!n) {
		cmistake(u, S->s, 142, MSG_EVENT);
		return;
	}
	o = (request *) calloc(1, sizeof(request));
	o->qty = n;
	o->unit = u;
	addlist(recruitorders, o);

	return;
}
/* ------------------------------------------------------------- */

int
count_max_migrants(faction * f)
{
	int x = (int)(log10(count_all(f) / 50.0) * 20);
	return max(0, x);
}

extern const char* resname(resource_t res, int i);

void
add_give(unit * u, unit * u2, int n, const resource_type * rtype, const char * cmd, int error)
{
	if (error)
		cmistake(u, cmd, error, MSG_COMMERCE);
	else if (!u2 || u2->faction!=u->faction) {
		assert(rtype);
		add_message(&u->faction->msgs,
			msg_message("give", "unit target resource amount",
			u, u2?ucansee(u->faction, u2, &u_unknown):&u_peasants, rtype, n));
		if (u2) {
			add_message(&u2->faction->msgs,
				msg_message("give", "unit target resource amount",
					ucansee(u2->faction, u, &u_unknown), u2, rtype, n));
		}
	}
}

void
addgive(unit * u, unit * u2, int n, resource_t res, const char * cmd, int error)
{
	add_give(u, u2, n, oldresourcetype[res], cmd, error);
}

/***************/

/* Derzeit fallen nur Silber und Pferde an die Region
 * */
void
give_peasants(int n, const item_type * itype, unit * src)
{
	region *r = src->region;

	if (itype == olditemtype[I_HORSE]){
		rsethorses(r, rhorses(r) + n);
	} else if (itype->rtype==r_silver) {
		rsetmoney(r, rmoney(r) + n);
	}
}

void
give_item(int want, const item_type * itype, unit * src, unit * dest, const char * cmd)
{
	short error = 0;
	int n;
	assert(itype!=NULL);
	n = new_get_pooled(src, item2resource(itype), GET_DEFAULT);
	n = min(want, n);
#define GIVERESTRICTION 3
	if (dest && src->faction != dest->faction && src->faction->age < GIVERESTRICTION) {
		ADDMSG(&src->faction->msgs, msg_error(src, cmd, "giverestriction", 
			"turns", GIVERESTRICTION));
		return;
	} else if (n == 0) {
		error = 36;
	} else if (itype->flags & ITF_CURSED) {
		error = 25;
	} else if (itype->give && !itype->give(src, dest, itype, n, cmd)) {
		return;
	} else {
		int use = new_use_pooled(src, item2resource(itype), GET_SLACK, n);
 		if (use<n) use += new_use_pooled(src, item2resource(itype), GET_RESERVE|GET_POOLED_SLACK, n-use);
		if (dest) {
			i_change(&dest->items, itype, n);
#if RESERVE_DONATIONS
			new_change_resvalue(dest, item2resource(itype), n);
#elif RESERVE_GIVE
			if (src->faction==dest->faction) {
				new_change_resvalue(dest, item2resource(itype), n);
			}
#endif
			handle_event(&src->attribs, "give", dest);
			handle_event(&dest->attribs, "receive", src);
#if defined(MUSEUM_PLANE) && defined(TODO)
			TODO: Einen Trigger benutzen!
			if (a_find(dest->attribs, &at_warden)) {
				/* warden_add_give(src, dest, itype, n); */
			}
#endif
		}else{
			/* gib bauern */
			give_peasants(use, itype, src);
		}
	}
	add_give(src, dest, n, item2resource(itype), cmd, error);
}

void
givemen(int n, unit * u, unit * u2, const char * cmd)
{
	ship *sh;
	int k = 0;
	int error = 0;

	if (u2 && u->faction != u2->faction && u->faction->age < GIVERESTRICTION) {
		ADDMSG(&u->faction->msgs, msg_error(u, cmd, "giverestriction", 
			"turns", GIVERESTRICTION));
		return;
	} else if (u == u2) {
		error = 10;
#if RACE_ADJUSTMENTS
	} else if (u->race == new_race[RC_URUK] || u->race == new_race[RC_SNOTLING]) {
		/* Uruks/Snotlings k�nnen nicht an Bauern �bergeben werden. */
		error = 307;
#endif
	} else if ((u && unit_has_cursed_item(u)) || (u2 && unit_has_cursed_item(u2))) {
		error = 78;
	} else if (fval(u, FL_LOCKED) || fval(u, FL_HUNGER) || is_cursed(u->attribs, C_SLAVE, 0)) {
		error = 74;
	} else if (u2 && (fval(u2, FL_LOCKED)|| is_cursed(u2->attribs, C_SLAVE, 0))) {
		error = 75;
	} else if (u2 != (unit *) NULL
		&& u2->faction != u->faction
		&& ucontact(u2, u) == 0) {
		error = 73;
	} else if (u2 && (get_skill(u, SK_MAGIC) || get_skill(u2, SK_MAGIC))) {
		error = 158;
	} else {
		if (n > u->number) n = u->number;
		if (n == 0) {
			error = 96;
		} else if (u2 && u->faction != u2->faction) {
			if (u2->faction->newbies + n > MAXNEWBIES) {
				error = 129;
			} else if (u->race != u2->faction->race) {
				if (u2->faction->race != new_race[RC_HUMAN]) {
					error = 120;
				} else if (count_migrants(u2->faction) + n > count_max_migrants(u2->faction)) {
					error = 128;
				}
				else if (teure_talente(u) || teure_talente(u2)) {
					error = 154;
				} else if (u2->number!=0) {
					error = 139;
				}
			}
		}
	}
	if (u2 && (get_skill(u, SK_ALCHEMY) || get_skill(u2, SK_ALCHEMY))) {

		k = count_skill(u2->faction, SK_ALCHEMY);

		/* Falls die Zieleinheit keine Alchemisten sind, werden sie nun
		 * welche. */
		if (!get_skill(u2, SK_ALCHEMY) && get_skill(u, SK_ALCHEMY) > 0)
			k += u2->number;

		/* Wenn in eine Alchemisteneinheit Personen verschoben werden */
		if (get_skill(u2, SK_ALCHEMY) && !get_skill(u, SK_ALCHEMY))
			k += n;

		/* Wenn Parteigrenzen �berschritten werden */
		if (u2->faction != u->faction)
			k += n;

		/* wird das Alchemistenmaximum ueberschritten ? */

		if (k > max_skill(u2->faction, SK_ALCHEMY)) {
			error = 156;
		}
	}

	if (!error) {
		if (u2 && u2->number == 0) {
			set_racename(&u2->attribs, get_racename(u->attribs));
			u2->race = u->race;
			u2->irace = u->irace;
		} else if (u2 && u2->race != u->race) {
			error = 139;
		}
		if (u2) {
			/* Einheiten von Schiffen k�nnen nicht NACH in von
			 * Nicht-alliierten bewachten Regionen ausf�hren */
			sh = leftship(u);
			if (sh) set_leftship(u2, sh);

			transfermen(u, u2, n);
			if (u->faction != u2->faction) {
				u2->faction->newbies += n;
			}
		} else {
			if (getunitpeasants) {
#ifdef ORCIFICATION
				if (u->race == new_race[RC_ORC] && !fval(u->region, RF_ORCIFIED)) {
					attrib *a = a_find(u->region->attribs, &at_orcification);
					if (!a) a = a_add(&u->region->attribs, a_new(&at_orcification));
					a->data.i += n;
				}
#endif
				transfermen(u, NULL, n);
			} else {
				error = 159;
			}
		}
	}
	addgive(u, u2, n, R_PERSON, cmd, error);
}

void
giveunit(region * r, unit * u, unit * u2, strlist * S)
{
	int n = u->number;

	if ((u && unit_has_cursed_item(u)) || (u2 && unit_has_cursed_item(u2))) {
		cmistake(u, S->s, 78, MSG_COMMERCE);
		return;
	}

	if (fval(u, FL_LOCKED) || fval(u, FL_HUNGER)) {
		cmistake(u, S->s, 74, MSG_COMMERCE);
		return;
	}

	if (u2 == NULL) {
		if (rterrain(r) == T_OCEAN) {
			cmistake(u, S->s, 152, MSG_COMMERCE);
		} else if (getunitpeasants) {
			unit *u3;

			for(u3 = r->units; u3; u3 = u3->next)
				if(u3->faction == u->faction && u != u3) break;

			if(u3) {
				while (u->items) {
					item * iold = i_remove(&u->items, u->items);
					item * inew = *i_find(&u3->items, iold->type);
					if (inew==NULL) i_add(&u3->items, iold);
					else {
						inew->number += iold->number;
						i_free(iold);
					}
				}
			}
			givemen(u->number, u, NULL, S->s);
			cmistake(u, S->s, 153, MSG_COMMERCE);
		} else {
			cmistake(u, S->s, 64, MSG_COMMERCE);
		}
		return;
	}

	if (ucontact(u2, u) == 0) {
		cmistake(u, S->s, 73, MSG_COMMERCE);
		return;
	}
	if (u->number == 0) {
		cmistake(u, S->s, 105, MSG_COMMERCE);
		return;
	}
	if (u2->faction->newbies + n > MAXNEWBIES) {
		cmistake(u, S->s, 129, MSG_COMMERCE);
		return;
	}
	if (u->race != u2->faction->race) {
		if (u2->faction->race != new_race[RC_HUMAN]) {
			cmistake(u, S->s, 120, MSG_COMMERCE);
			return;
		}
		if (count_migrants(u2->faction) + u->number > count_max_migrants(u2->faction)) {
			cmistake(u, S->s, 128, MSG_COMMERCE);
			return;
		}
		if (teure_talente(u)) {
			cmistake(u, S->s, 154, MSG_COMMERCE);
			return;
		}
	}
	if (get_skill(u, SK_MAGIC)) {
		if (count_skill(u2->faction, SK_MAGIC) + u->number >
		max_skill(u2->faction, SK_MAGIC))
		{
			cmistake(u, S->s, 155, MSG_COMMERCE);
			return;
		}
		if (u2->faction->magiegebiet != find_magetype(u)) {
			cmistake(u, S->s, 157, MSG_COMMERCE);
			return;
		}
	}
	if (get_skill(u, SK_ALCHEMY)
		&& count_skill(u2->faction, SK_ALCHEMY) + u->number >
		max_skill(u2->faction, SK_ALCHEMY))
	{
		cmistake(u, S->s, 156, MSG_COMMERCE);
		return;
	}
	add_give(u, u2, 1, r_unit, S->s, 0);
	u_setfaction(u, u2->faction);
	u2->faction->newbies += n;

	/* "Ich gebe einer Partei eine Einheit, die den Befehl hat, effektiv
	 * alles Silber der Partei an mich zu �bergeben. Zack, bin ich reich."
	 * Bl�dsinn, da Silberpool nicht auf geben wirkt. Aber Rekrutierungen,
	 * teure Talente lernen und Gegenst�nde erschaffen! Kritisch sind:
	 * BIETE, FORSCHEN, KAUFE, LERNE, REKRUTIERE, ZAUBERE,
	 * HELFE, PASSWORT, STIRB (Katja)
	 * Nat�rlich wirkt der Silberpool bei gib!
	 * Und damit nicht irgendein zuk�nftiger Befehl vergessen wird,
	 * l�schen wir der Einfachheit halber alle! (Enno).
	 */

	/* freelist(u->orders);
    * FEHLER! Das darf nicht, weil es auf orders noch pointer geben kann!
    */
	while (u->orders) {
		/* dieser code _kennt_ die implementaion von strlists... */
		strlist * o = u->orders;
		u->orders = o->next;
		gc_add(o->s); /* delete it later */
		o->s = NULL;
	}
}

static void
dogive(region * r, unit * u, strlist * S, boolean liefere, int mode)
/*
 * mode=0: give to any units
 * mode=1: give to other units and peasants only
 * mode=2: give to own units only
 */
{
	unit *u2;
	char *s;
	int i, n;
	const item_type * itype;
	int notfound_error = 64;

	if (liefere) notfound_error = 63;

	u2 = getunit(r, u->faction);

	switch (mode) {
	case 0:
		/* any destination is okay */
		break;
	case 1:
		/* not to myself */
		if (u2 && u2->faction==u->faction) return;
		break;
#if TWOPASS_GIVE
	case 2:
		/* not to peasants or others */
		if (!u2 || u2->faction!=u->faction) return;
		break;
#endif
	default:
		assert(!"invalid mode for dogive");
	}
	if (!u2 && !getunitpeasants) {
		cmistake(u, S->s, notfound_error, MSG_COMMERCE);
		return;
	}

	/* Damit Tarner nicht durch die Fehlermeldung enttarnt werden k�nnen */
	if (u2 && (!cansee(u->faction,r,u2,0) && !ucontact(u2, u) && !fval(u2, FL_TAKEALL))) {
		cmistake(u, S->s, notfound_error, MSG_COMMERCE);
		return;
	}
	if (u == u2){
		cmistake(u, S->s, 8, MSG_COMMERCE);
		return;
	}

	/* FL_TAKEALL ist ein grober Hack. Generalisierung tut not, ist aber nicht
	 * wirklich einfach. */
	if (r->planep && fval(r->planep, PFL_NOGIVE) && (!u2 || !fval(u2, FL_TAKEALL))) {
		cmistake(u, S->s, 268, MSG_COMMERCE);
		return;
	}

	s = getstrtoken();

	if (findparam(s, u->faction->locale) == P_CONTROL) {
		if (!u2) {
			cmistake(u, S->s, notfound_error, MSG_EVENT);
			return;
		}
		if (!u->building && !u->ship) {
			cmistake(u, S->s, 140, MSG_EVENT);
			return;
		}
		if (u->building && u2->building != u->building) {
			cmistake(u, S->s, 33, MSG_EVENT);
			return;
		}
		if (u->ship && u2->ship != u->ship) {
			cmistake(u, S->s, 32, MSG_EVENT);
			return;
		}
		if (!fval(u, FL_OWNER)) {
			cmistake(u, S->s, 49, MSG_EVENT);
			return;
		}
		if (!ucontact(u2, u)) {
			cmistake(u, S->s, 40, MSG_EVENT);
			return;
		}
		freset(u, FL_OWNER);
		fset(u2, FL_OWNER);

		add_message(&u->faction->msgs,
			msg_message("givecommand", "unit receipient", u, u2));
		if (u->faction != u2->faction) {
			add_message(&u2->faction->msgs,
				msg_message("givecommand", "unit receipient",
					ucansee(u2->faction, u, &u_unknown), u2));
		}
		return;
	}
	if (u2 && u2->race == new_race[RC_SPELL]) {
		cmistake(u, S->s, notfound_error, MSG_COMMERCE);
		return;
	}

	/* if ((u->race->ec_flags & NOGIVE) || fval(u,FL_LOCKED)) {*/
	if (u->race->ec_flags & NOGIVE) {
		add_message(&u->faction->msgs,
			msg_error(u, S->s, "race_nogive", "race", u->race));
		return;
	}
	if (u2 && !(u2->race->ec_flags & GETITEM)) {
		add_message(&u->faction->msgs,
			msg_error(u, S->s, "race_notake", "race", u2->race));
		return;
	}

	/* �bergabe aller Kr�uter */
	if (findparam(s, u->faction->locale) == P_HERBS) {
		if (!(u->race->ec_flags & GIVEITEM)) {
			add_message(&u->faction->msgs,
				msg_error(u, S->s, "race_nogive", "race", u->race));
			return;
		}
		if (!u2) {
			if (!getunitpeasants) {
				cmistake(u, S->s, notfound_error, MSG_COMMERCE);
				return;
			}
		}
		n = 0;
		if (u->items) {
			item **itmp=&u->items;
			while (*itmp) {
				const herb_type * htype = resource2herb((*itmp)->type->rtype);
				if (htype && (*itmp)->number>0) {
					give_item((*itmp)->number, (*itmp)->type, u, u2, S->s);
					n = 1;
				}
				else itmp = &(*itmp)->next;
			}
		}
		if (!n) cmistake(u, S->s, 38, MSG_COMMERCE);
		return;
	}
	if (findparam(s, u->faction->locale) == P_ZAUBER) {
		cmistake(u, S->s, 7, MSG_COMMERCE);
		/* geht nimmer */
		return;
	}
	if (findparam(s, u->faction->locale) == P_UNIT) {	/* Einheiten uebergeben */
		if (!(u->race->ec_flags & GIVEUNIT)) {
			cmistake(u, S->s, 167, MSG_COMMERCE);
			return;
		}

		if (u2 && !ucontact(u2, u)) {
			cmistake(u, S->s, 40, MSG_COMMERCE);
			return;
		}
		giveunit(r, u, u2, S);
		return;
	}
	if (findparam(s, u->faction->locale) == P_ANY) { /* Alle Gegenst�nde �bergeben */
		char * s = getstrtoken();
		const resource_type * rtype = findresourcetype(s, u->faction->locale);

		/* Dumpfbackenbrot kann ohne kontaktiere �bergeben werden */
		if (rtype!=NULL) {
			const potion_type * ptype = resource2potion(rtype);
			if (ptype!=NULL && ptype == oldpotiontype[P_FOOL]) {
				item * i = *i_find(&u->items, ptype->itype);
				if (i!=NULL) {
					n = i->number - new_get_resvalue(u, i->type->rtype);
					give_item(n, i->type, u, u2, S->s);
					return;
				}
			}
		}

		if(u2 && !ucontact(u2, u)) {
			cmistake(u, S->s, 40, MSG_COMMERCE);
			return;
		}

		if (*s == 0) {
			if (!(u->race->ec_flags & GIVEITEM)) {
				add_message(&u->faction->msgs,
					msg_error(u, S->s, "race_nogive", "race", u->race));
				return;
			}
			/* f�r alle items einmal pr�fen, ob wir mehr als von diesem Typ
			 * reserviert ist besitzen und diesen Teil dann �bergeben */
			if (u->items) {
				item **itmp=&u->items;
				while (*itmp) {
					if ((*itmp)->number > 0 
							&& (*itmp)->number - new_get_resvalue(u, (*itmp)->type->rtype) > 0) {
						give_item((*itmp)->number, (*itmp)->type, u, u2, S->s);
					}
					else itmp = &(*itmp)->next;
				}
			}
			return;
		}
		i = findparam(s, u->faction->locale);
		if (i == P_PERSON) {
			if (!(u->race->ec_flags & GIVEPERSON)) {
				add_message(&u->faction->msgs,
					msg_error(u, S->s, "race_noregroup", "race", u->race));
				return;
			}
			n = u->number;
			givemen(n, u, u2, S->s);
			return;
		}

		if (!(u->race->ec_flags & GIVEITEM)) {
				add_message(&u->faction->msgs,
					msg_error(u, S->s, "race_nogive", "race", u->race));
			return;
		}
		itype = finditemtype(s, u->faction->locale);
		if (itype!=NULL) {
			item * i = *i_find(&u->items, itype);
			if (i!=NULL) {
				n = i->number - new_get_resvalue(u, itype->rtype);
				give_item(n, itype, u, u2, S->s);
				return;
			}
		}
	}

	n = atoip(s);		/* n: anzahl */
	s = getstrtoken();

	if (s == NULL) {
			cmistake(u, S->s, 113, MSG_COMMERCE);
			return;
	}

	if (u2 && !ucontact(u2, u)) {
		const resource_type * rtype = findresourcetype(s, u->faction->locale);
		if (rtype==NULL || !fval(rtype, RTF_SNEAK))
		{
			cmistake(u, S->s, 40, MSG_COMMERCE);
			return;
		}
	}
	i = findparam(s, u->faction->locale);
	if (i == P_PERSON) {
		if (!(u->race->ec_flags & GIVEPERSON)) {
			add_message(&u->faction->msgs,
				msg_error(u, S->s, "race_noregroup", "race", u->race));
			return;
		}
		givemen(n, u, u2, S->s);
		return;
	}

	if (!(u->race->ec_flags & GIVEITEM)) {
		add_message(&u->faction->msgs,
			msg_error(u, S->s, "race_nogive", "race", u->race));
		return;
	}
	itype = finditemtype(s, u->faction->locale);
	if (itype!=NULL) {
		give_item(n, itype, u, u2, S->s);
		return;
	}
	cmistake(u, S->s, 123, MSG_COMMERCE);
}
/* ------------------------------------------------------------- */
void
forgetskill(unit * u)
{
	skill_t talent;
	char *s;

	s = getstrtoken();

	if ((talent = findskill(s, u->faction->locale)) != NOSKILL) {
		struct message * m = add_message(&u->faction->msgs,
			msg_message("forget", "unit skill", u, talent));
		msg_release(m);
		set_level(u, talent, 0);
	}
}

/* ------------------------------------------------------------- */

void
report_donations(void)
{
	spende * sp;
	for (sp = spenden; sp; sp = sp->next) {
		region * r = sp->region;
		if (sp->betrag > 0) {
			struct message * msg = msg_message("donation",
				"from to amount", sp->f1, sp->f2, sp->betrag);
			r_addmessage(r, sp->f1, msg);
			r_addmessage(r, sp->f2, msg);
			msg_release(msg);
		}
	}
}

void
add_spende(faction * f1, faction * f2, int betrag, region * r)
{
	spende *sp;

	sp = spenden;

	while (sp) {
		if (sp->f1 == f1 && sp->f2 == f2 && sp->region == r) {
			sp->betrag += betrag;
			return;
		}
		sp = sp->next;
	}

	sp = calloc(1, sizeof(spende));
	sp->f1 = f1;
	sp->f2 = f2;
	sp->region = r;
	sp->betrag = betrag;
	sp->next = spenden;
	spenden = sp;
}

static boolean
maintain(building * b, boolean first)
/* first==false -> take money from wherever you can */
{
	int c;
	region * r = b->region;
	boolean paid = true, work = first;
	unit * u;
	if (fval(b, BLD_MAINTAINED)) return true;
	if (b->type->maintenance==NULL) return true;
	if (is_cursed(b->attribs, C_NOCOST, 0)) {
		fset(b, BLD_MAINTAINED);
		fset(b, BLD_WORKING);
		return true;
	}
	u = buildingowner(r, b);
	if (u==NULL) return false;
	for (c=0;b->type->maintenance[c].number;++c) {
		const maintenance * m = b->type->maintenance+c;
		int need = m->number;

		if (fval(m, MTF_VARIABLE)) need = need * b->size;
		if (u) {
			/* first ist im ersten versuch true, im zweiten aber false! Das
			 * bedeutet, das in der Runde in die Region geschafften Resourcen
			 * nicht genutzt werden k�nnen, weil die reserviert sind! */
			if (!first) need -= get_all(u, m->type);
			else need -= get_pooled(u, r, m->type);
			if (!first && need > 0) {
				unit * ua;
				for (ua=r->units;ua;ua=ua->next) freset(ua->faction, FL_DH);
				fset(u->faction, FL_DH); /* hat schon */
				for (ua=r->units;ua;ua=ua->next) {
					if (!fval(ua->faction, FL_DH) && (ua->faction == u->faction || allied(ua, u->faction, HELP_MONEY))) {
						need -= new_get_pooled(ua, oldresourcetype[m->type], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE);
						fset(ua->faction, FL_DH);
						if (need<=0) break;
					}
				}
			}
			if (need > 0) {
				if (!fval(m, MTF_VITAL)) work = false;
				else {
					paid = false;
					break;
				}
			}
		}
	}
	if (paid && c>0) {
		/* TODO: wieviel von was wurde bezahlt */
		message * msg;
		if (first) {
			msg = add_message(&u->faction->msgs,
				msg_message("maintenance", "unit building", u, b));
		} else {
			msg = add_message(&u->faction->msgs,
				msg_message("maintenance_late", "building", b));
		}
		msg_release(msg);
		fset(b, BLD_MAINTAINED);
		if (work) fset(b, BLD_WORKING);
		for (c=0;b->type->maintenance[c].number;++c) {
			const maintenance * m = b->type->maintenance+c;
			int cost = m->number;

			if (!fval(m, MTF_VITAL) && !work) continue;
			if (fval(m, MTF_VARIABLE)) cost = cost * b->size;

			if (!first) cost -= new_use_pooled(u, oldresourcetype[m->type], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK|GET_POOLED_RESERVE|GET_POOLED_FORCE, cost);
			else cost -= new_use_pooled(u, oldresourcetype[m->type], GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, cost);
			if (!first && cost > 0) {
				unit * ua;
				for (ua=r->units;ua;ua=ua->next) freset(ua->faction, FL_DH);
				fset(u->faction, FL_DH); /* hat schon */
				for (ua=r->units;ua;ua=ua->next) {
					if (!fval(ua->faction, FL_DH) && allied(ua, u->faction, HELP_MONEY)) {
						int give = use_all(ua, m->type, cost);
						if (!give) continue;
						cost -= give;
						fset(ua->faction, FL_DH);
						if (m->type==R_SILVER) add_spende(ua->faction, u->faction, give, r);
						if (cost<=0) break;
					}
				}
			}
			assert(cost==0);
		}
	} else {
		message * msg = add_message(&u->faction->msgs,
			msg_message("maintenancefail", "unit building", u, b));
		msg_release(msg);
		return false;
	}
	return true;
}

static void
gebaeude_stuerzt_ein(region * r, building * b)
{
	unit *u;
	int n, i;
	direction_t d;
	int opfer = 0;
	int road = 0;
	struct message * msg;
	/*
	"$building($crashed) st�rzte ein.$if($road," Beim Einsturz wurde die halbe Stra�e vernichtet.","")$if($victims,"$int($victims) $if($eq($victims,1),"ist","sind"),"") zu beklagen."
	*/

	/* Falls Karawanserei, Damm oder Tunnel einst�rzen, wird die schon
	 * gebaute Stra�e zur H�lfte vernichtet */
	for (d=0;d!=MAXDIRECTIONS;++d) if (rroad(r, d) > 0 &&
		(b->type == &bt_caravan ||
		 b->type == &bt_dam ||
		 b->type == &bt_tunnel))
	{
		rsetroad(r, d, rroad(r, d) / 2);
		road = 1;
	}
	for (u = r->units; u; u = u->next) {
		if (u->building == b) {
			int loss = 0;

			fset(u->faction, FL_MARK);
			freset(u, FL_OWNER);
			leave(r,u);
			n = u->number;
			for (i = 0; i < n; i++) {
				if (rand() % 100 >= EINSTURZUEBERLEBEN) {
					++loss;
				}
			}
			scale_number(u, u->number - loss);
			opfer += loss;
		}
	}

	msg = msg_message("buildingcrash", "region building opfer road", r, b, opfer, road);
	add_message(&r->msgs, msg);
	for (u=r->units; u; u=u->next) {
		faction * f = u->faction;
		if (fval(f, FL_MARK)) {
			freset(u->faction, FL_MARK);
			add_message(&f->msgs, msg);
		}
	}
	msg_release(msg);
	destroy_building(b);
}

void
maintain_buildings(boolean crash)
{
	region * r;
	for (r = regions; r; r = r->next) {
		building **bp = &r->buildings;
		while (*bp) {
			building * b = *bp;
			if (!maintain(b, !crash) && crash) {
				if (rand() % 100 < EINSTURZCHANCE) {
					gebaeude_stuerzt_ein(r, b);
					continue;
				} else {
					unit * u = buildingowner(r, b);
					struct message * msg = msg_message("nomaintenance", "building", b);
					if (u) {
						add_message(&u->faction->msgs, msg);
						r_addmessage(r, u->faction, msg);
					}
				}
			}
			bp=&b->next;
		}
	}
}

void
economics(void)
{
	region *r;
	unit *u;
	strlist *S;

	/* Geben vor Selbstmord (doquit)! Hier alle unmittelbaren Befehle.
	 * Rekrutieren vor allen Einnahmequellen. Bewachen JA vor Steuern
	 * eintreiben. */

	for (r = regions; r; r = r->next) if (r->units) {
		request *recruitorders = NULL;

		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				switch (igetkeyword(S->s, u->faction->locale)) {

				case K_DESTROY:
					destroy(r, u, S->s);
					break;

				case K_GIVE:
					dogive(r, u, S, S->s[0]=='@'?true:false, TWOPASS_GIVE);
					break;

				case K_LIEFERE:
					dogive(r, u, S, true, TWOPASS_GIVE);
					break;

				case K_FORGET:
					forgetskill(u);
					break;

				}
			}
		}
#if TWOPASS_GIVE
		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				switch (igetkeyword(S->s, u->faction->locale)) {
				case K_GIVE:
					dogive(r, u, S, S->s[0]=='@'?true:false, 2);
					break;
				case K_LIEFERE:
					dogive(r, u, S, true, 2);
					break;
				}
			}
		}
#endif
		/* RECRUIT orders */

		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				if (igetkeyword(S->s, u->faction->locale) == K_RECRUIT) {
					recruit(r, u, S, &recruitorders);
					break;
				}
			}
		}

		if (recruitorders) expandrecruit(r, recruitorders);

	}
}
/* ------------------------------------------------------------- */


static void
manufacture(unit * u, const item_type * itype, int want)
{
	int n;
	int skill;
	int minskill = itype->construction->minskill;
	skill_t sk = itype->construction->skill;

	skill = effskill(u, sk);
	skill = skillmod(itype->rtype->attribs, u, u->region, sk, skill, SMF_PRODUCTION);

	if (skill < 0) {
		/* an error occured */
		int err = -skill;
		cmistake(u, findorder(u, u->thisorder), err, MSG_PRODUCE);
		return;
	}

	if(want==0)
		want=maxbuild(u, itype->construction);

	n = build(u, itype->construction, 0, want);
	switch (n) {
	case ENEEDSKILL:
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "skill_needed", "skill", sk));
		return;
	case ELOWSKILL:
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "manufacture_skills", "skill minskill product",
			sk, minskill, itype->rtype, 1));
		return;
	case ENOMATERIALS:
		/* something missing from the list of materials */
		strcpy(buf, "Daf�r braucht man mindestens:");
		{
			int c, n;
			const construction * cons = itype->construction;
			char * ch = buf+strlen(buf);
			assert(cons);
			for (c=0;cons->materials[c].number; c++) {
			  if (c!=0)
				 strcat(ch++, ",");
			  n=cons->materials[c].number / cons->reqsize;
			  sprintf(ch, " %d %s", n?n:1,
						locale_string(u->faction->locale,
							resname(cons->materials[c].type, cons->materials[c].number!=1)));
			  ch = ch+strlen(ch);
			}
			mistake(u, findorder(u, u->thisorder), buf, MSG_PRODUCE);
			return;
		}
	}
	if (n>0) {
		i_change(&u->items, itype, n);
		if (want==INT_MAX) want = n;
		ADDMSG(&u->faction->msgs, msg_message("manufacture",
			"unit region amount wanted resource", u, u->region, n, want, itype->rtype));
	}
}

typedef struct allocation {
	struct allocation * next;
	int want, get;
	double save;
	unsigned int flags;
	unit * unit;
} allocation;
#define new_allocation() calloc(sizeof(allocation), 1)
#define free_allocation(a) free(a)

typedef struct allocation_list {
	struct allocation_list * next;
	allocation * data;
	const resource_type * type;
} allocation_list;

static allocation_list * allocations;


static void
allocate_resource(unit * u, const resource_type * rtype, int want)
{
	const item_type * itype = resource2item(rtype);
	region * r = u->region;
	int busy = u->number;
	int dm = 0;
	allocation_list * alist;
	allocation * al;
	unit *u2;
	int amount, skill;

	/* momentan kann man keine ressourcen abbauen, wenn man daf�r
	 * materialverbruach hat: */
	assert(itype!=NULL && itype->construction->materials==NULL);

	if (itype == olditemtype[I_WOOD] && fval(r, RF_MALLORN)) {
		cmistake(u, findorder(u, u->thisorder), 92, MSG_PRODUCE);
		return;
	}
	if (itype == olditemtype[I_MALLORN] && !fval(r, RF_MALLORN)) {
		cmistake(u, findorder(u, u->thisorder), 91, MSG_PRODUCE);
		return;
	}
	if (itype == olditemtype[I_LAEN]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype != &bt_mine) {
			cmistake(u, findorder(u, u->thisorder), 104, MSG_PRODUCE);
			return;
		}
	}

	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_PRODUCE);
		return;
	}
	/* Elfen k�nnen Holzf�llen durch Bewachen verhindern, wenn sie
		 die Holzf�ller sehen. */
	if (itype == olditemtype[I_WOOD] || itype == olditemtype[I_MALLORN]) {
		for (u2 = r->units; u2; u2 = u2->next) {
			if (getguard(u2) & GUARD_TREES
					&& u2->number
					&& cansee(u2->faction,r,u,0)
					&& !ucontact(u2, u)
					&& !besieged(u2)
					&& !allied(u2, u->faction, HELP_GUARD)
#ifdef WACH_WAFF
					&& armedmen(u2)
#endif
					) {
				add_message(&u->faction->msgs,
					msg_error(u, findorder(u, u->thisorder), "region_guarded", "guard", u2));
				return;
			}
		}
	}

	/* Bergw�chter k�nnen Abbau von Eisen/Laen durch Bewachen verhindern.
	 * Als magische Wesen 'sehen' Bergw�chter alles und werden durch
	 * Belagerung nicht aufgehalten.  (Ansonsten wie oben bei Elfen anpassen).
	 */
	if (itype == olditemtype[I_IRON] || itype == olditemtype[I_LAEN])
	{
		for (u2 = r->units; u2; u2 = u2->next ) {
			if (getguard(u) & GUARD_MINING
				&& !fval(u2, FL_ISNEW)
				&& u2->number
				&& !ucontact(u2, u)
				&& !allied(u2, u->faction, HELP_GUARD))
			{
				add_message(&u->faction->msgs,
					msg_error(u, findorder(u, u->thisorder), "region_guarded", "guard", u2));
				return;
			}
		}
	}

	assert(itype->construction->skill!=0 || "limited resource needs a required skill for making it");
	skill = eff_skill(u, itype->construction->skill, u->region);
	if (skill == 0) {
		skill_t sk = itype->construction->skill;
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "skill_needed", "skill", sk));
		return;
	}
	if (skill < itype->construction->minskill) {
		skill_t sk = itype->construction->skill;
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "manufacture_skills", "skill minskill product",
			sk, itype->construction->minskill, itype->rtype));
		return;
	} else {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (itype == olditemtype[I_IRON] && btype == &bt_mine) {
			++skill;
		}
		else if (itype == olditemtype[I_STONE] && btype == &bt_quarry) {
			++skill;
		}
		else if (itype == olditemtype[I_WOOD] && btype == &bt_sawmill) {
			++skill;
		}
		else if (itype == olditemtype[I_MALLORN] && btype == &bt_sawmill) {
			++skill;
		}
	}
	amount = skill * u->number;
	/* nun ist amount die Gesamtproduktion der Einheit (in punkten) */

	/* mit Flinkfingerring verzehnfacht sich die Produktion */
	amount += skill * min(u->number, get_item(u,I_RING_OF_NIMBLEFINGER)) * 9;

	/* Schaffenstrunk: */
	if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
		dm = min(dm, u->number);
		change_effect(u, oldpotiontype[P_DOMORE], -dm);
		amount += dm * skill;	/* dm Personen produzieren doppelt */
	}

	amount /= itype->construction->minskill;

	/* Limitierung durch Parameter m. */
	if (want > 0 && want < amount) amount = want;

	busy = (amount + skill - 1) / skill; /* wieviel leute tun etwas? */

	alist = allocations;
	while (alist && alist->type!=rtype) alist = alist->next;
	if (!alist) {
		alist = calloc(sizeof(struct allocation_list), 1);
		alist->next = allocations;
		alist->type = rtype;
		allocations = alist;
	}
	al = new_allocation();
	al->want = amount;
	al->save = 1.0;
	al->next = alist->data;
	al->unit = u;
	alist->data = al;
	if (itype==olditemtype[I_IRON]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==&bt_mine)
			al->save *= 0.5;
		if (u->race == new_race[RC_DWARF])
#if RACE_ADJUSTMENTS
			al->save *= 0.75;
#else
			al->save *= 0.5;
#endif
	} else if (itype==olditemtype[I_STONE]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==&bt_quarry)
			al->save = al->save*0.5;
#if RACE_ADJUSTMENTS
		if (u->race == new_race[RC_TROLL])
			al->save = al->save*0.75;
#endif
	} else if (itype==olditemtype[I_MALLORN]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==&bt_sawmill)
			al->save *= 0.5;
	} else if (itype==olditemtype[I_WOOD]) {
		struct building * b = inside_building(u);
		const struct building_type * btype = b?b->type:NULL;
		if (btype==&bt_sawmill)
			al->save *= 0.5;
	}
}

typedef struct allocator {
	const struct resource_type * type;
	void (*allocate)(const struct allocator *, region *, allocation *);

	struct allocator * next;
} allocator;

static struct allocator * allocators;

static const allocator *
get_allocator(const struct resource_type * type)
{
	const struct allocator * alloc = allocators;
	while (alloc && alloc->type!=type) alloc = alloc->next;
	return alloc;
}

static allocator *
make_allocator(const struct resource_type * type, void (*allocate)(const struct allocator *, region *, allocation *))
{
	allocator * alloc = (allocator *)malloc(sizeof(allocator));
	alloc->type = type;
	alloc->allocate = allocate;
	return alloc;
}

static void
add_allocator(allocator * alloc)
{
	alloc->next = allocators;
	allocators = alloc;
}

enum {
	AFL_DONE = 1<<0,
	AFL_LOWSKILL = 1<<1
};

static int
required(int want, double save)
{
	int norders = (int)(want * save);
	if (norders < want*save) ++norders;
	return norders;
}

#if NEW_RESOURCEGROWTH
static void
leveled_allocation(const allocator * self, region * r, allocation * alist)
{
	const resource_type * rtype = self->type;
	const item_type * itype = resource2item(rtype);
	rawmaterial * rm = rm_get(r, rtype);
	int need;
	boolean first = true;

	if (rm!=NULL) do {
		int avail = rm->amount;
		int norders = 0;
		allocation * al;

		assert (avail>0);

		for (al=alist;al;al=al->next) if (!fval(al, AFL_DONE)) {
			int req = required(al->want-al->get, al->save);
			assert(al->get<=al->want && al->get >= 0);
			if (eff_skill(al->unit, itype->construction->skill, r) >= rm->level+itype->construction->minskill-1) {
				if (req) {
					norders += req;
				} else {
					fset(al, AFL_DONE);
				}
			} else {
				fset(al, AFL_DONE);
				if (first) fset(al, AFL_LOWSKILL);
			}
		}
		need = norders;

		avail = min(avail, norders);
		if (need>0) {
			int use = 0;
			for (al=alist;al;al=al->next) if (!fval(al, AFL_DONE)) {
				if (avail > 0) {
					int want = required(al->want-al->get, al->save);
					int x = avail*want/norders;
					/* Wenn Rest, dann w�rfeln, ob ich was bekomme: */
					if (rand() % norders < (avail*want) % norders)
						++x;
					avail -= x;
					use += x;
					norders -= want;
					need -= x;
					al->get = min(al->want, al->get+(int)(x/al->save));
				}
			}
			if (use) {
				assert(use<=rm->amount);
				rm->type->use(rm, r, use);
			}
			assert(avail==0 || norders==0);
		}
		first = false;
	} while (need>0);
}
#endif

static void
attrib_allocation(const allocator * self, region * r, allocation * alist)
{
	allocation * al;
	const resource_type * rtype = self->type;
	int avail = 0;
	int norders = 0;
	attrib * a = a_find(rtype->attribs, &at_resourcelimit);
	resource_limit * rdata = (resource_limit*)a->data.v;

	for (al=alist;al;al=al->next) {
		norders += required(al->want, al->save);
	}

	if (rdata->limit) {
		avail = rdata->limit(r, rtype);
		if (avail < 0) avail = 0;
	}
	else avail = rdata->value;

	avail = min(avail, norders);
	for (al=alist;al;al=al->next) {
		if (avail > 0) {
			int want = required(al->want, al->save);
			int x = avail*want/norders;
			/* Wenn Rest, dann w�rfeln, ob ich was bekomme: */
			if (rand() % norders < (avail*want) % norders)
				++x;
			avail -= x;
			norders -= want;
			al->get = min(al->want, (int)(x/al->save));
			if (rdata->use) {
				int use = (int)(al->get * al->save);
				if (use) rdata->use(r, rtype, use);
			}
		}
	}
	assert(avail==0 || norders==0);
}

static void
split_allocations(region * r)
{
	allocation_list ** p_alist=&allocations;
	freset(r, RF_DH);
	while (*p_alist) {
		allocation_list * alist = *p_alist;
		const resource_type * rtype = alist->type;
		const allocator * alloc = get_allocator(rtype);
		const item_type * itype = resource2item(rtype);
		allocation ** p_al = &alist->data;

		freset(r, RF_DH);
		alloc->allocate(alloc, r, alist->data);

		while (*p_al) {
			allocation * al = *p_al;
			if (al->get) {
				assert(itype || !"not implemented for non-items");
				i_change(&al->unit->items, itype, al->get);
				produceexp(al->unit, itype->construction->skill, al->unit->number);
#if NEW_RESOURCEGROWTH
				fset(r, RF_DH);
#endif
			}
			if (al->want==INT_MAX) al->want = al->get;
			if (fval(al, AFL_LOWSKILL)) {
				add_message(&al->unit->faction->msgs,
					msg_message("produce_lowskill", "unit region resource",
					al->unit, al->unit->region, rtype));
			} else {
				add_message(&al->unit->faction->msgs,
					msg_message("produce", "unit region amount wanted resource",
					al->unit, al->unit->region, al->get, al->want, rtype));
			}
			*p_al=al->next;
			free_allocation(al);
		}
		*p_alist=alist->next;
		free(alist);
	}
	allocations = NULL;
}

static void
create_potion(unit * u, const potion_type * ptype, int want)
{
	int built;

	if(want==0)
		want=maxbuild(u, ptype->itype->construction);
	built = build(u, ptype->itype->construction, 0, want);
	switch (built) {
	case ELOWSKILL:
	case ENEEDSKILL:
		/* no skill, or not enough skill points to build */
		cmistake(u, findorder(u, u->thisorder), 50, MSG_PRODUCE);
		break;
	case ECOMPLETE:
		assert(0);
		break;
	case ENOMATERIALS:
		/* something missing from the list of materials */
		strcpy(buf, "Daf�r braucht man mindestens:");
		{
			int c, n;
			const construction * cons = ptype->itype->construction;
			char * ch = buf+strlen(buf);
			assert(cons);
			for (c=0;cons->materials[c].number; c++) {
			  if (c!=0)
				 strcat(ch++, ",");
			  n=cons->materials[c].number / cons->reqsize;
			  sprintf(ch, " %d %s", n?n:1,
						locale_string(u->faction->locale,
							resname(cons->materials[c].type, cons->materials[c].number!=1)));
			  ch = ch+strlen(ch);
			}
			strcat(ch,".");
			mistake(u, findorder(u, u->thisorder), buf, MSG_PRODUCE);
			return;
		}
		break;
	default:
		i_change(&u->items, ptype->itype, built);
		if (want==INT_MAX) want = built;
		ADDMSG(&u->faction->msgs, msg_message("manufacture",
			"unit region amount wanted resource", u, u->region, built, want, ptype->itype->rtype));
		break;
	}
}

static void
create_item(unit * u, const item_type * itype, int want)
{
	if (fval(itype->rtype, RTF_LIMITED))
		allocate_resource(u, itype->rtype, want);
	else {
		const potion_type * ptype = resource2potion(itype->rtype);
		if (ptype!=NULL) create_potion(u, ptype, want);
		else if (itype->construction) manufacture(u, itype, want);
		else cmistake(u, findorder(u, u->thisorder), 125, MSG_PRODUCE);
	}
}

static void
make(region * r, unit * u)
{
	char *s;
	const building_type * btype;
	const ship_type * stype;
	param_t p;
	int m;
	const item_type * itype;

	s = getstrtoken();
	m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	p = findparam(s, u->faction->locale);

	/* MACHE TEMP kann hier schon gar nicht auftauchen, weil diese nicht in
	 * thisorder abgespeichert werden - und auf den ist getstrtoken() beim
	 * aufruf von make geeicht */

	itype = finditemtype(s, u->faction->locale);
	if (itype!=NULL) {
		create_item(u, itype, m);
		return;
	}

	if (p == P_HERBS) {
		herbsearch(r, u, m);
		return;
	} else if (p == P_ROAD) {
		direction_t d;
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, findorder(u, u->thisorder), 275, MSG_PRODUCE);
			return;
		}
		d = finddirection(getstrtoken(), u->faction->locale);
		if (d!=NODIRECTION) {
			if(r->planep && fval(r->planep, PFL_NOBUILD)) {
				cmistake(u, findorder(u, u->thisorder), 94, MSG_PRODUCE);
				return;
			}
			build_road(r, u, m, d);
		} else cmistake(u, findorder(u, u->thisorder), 71, MSG_PRODUCE);
		return;
	} else if (p == P_SHIP) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, findorder(u, u->thisorder), 276, MSG_PRODUCE);
			return;
		}
		continue_ship(r, u, m);
		return;
	}

	stype = findshiptype(s, u->faction->locale);
	if (stype != NOSHIP) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, findorder(u, u->thisorder), 276, MSG_PRODUCE);
			return;
		}
		create_ship(r, u, stype, m);
		return;
	}

	btype = findbuildingtype(s, u->faction->locale);
	if (btype != NOBUILDING) {
		if(r->planep && fval(r->planep, PFL_NOBUILD)) {
			cmistake(u, findorder(u, u->thisorder), 94, MSG_PRODUCE);
			return;
		}
		build_building(u, btype, m);
		return;
	}

	cmistake(u, findorder(u, u->thisorder), 125, MSG_PRODUCE);
}
/* ------------------------------------------------------------- */

const attrib_type at_luxuries = {
	"luxuries", NULL, NULL, NULL, NULL, NULL
};

static void
expandbuying(region * r, request * buyorders)
{
	int max_products;
	unit *u;
	static struct trade {
		const luxury_type * type;
		int number;
		int multi;
	} *trades, *trade;
	static int ntrades=0;
	int i, j;
	const luxury_type * ltype;

	if (ntrades==0) {
		for (ltype=luxurytypes;ltype;ltype=ltype->next)
			++ntrades;
		trades = gc_add(calloc(sizeof(struct trade), ntrades));
		for (i=0, ltype=luxurytypes;i!=ntrades;++i, ltype=ltype->next)
			trades[i].type = ltype;
	}
	for (i=0;i!=ntrades;++i) {
		trades[i].number = 0;
		trades[i].multi = 1;
	}

	if (!buyorders) return;

	/* Initialisation. multiplier ist der Multiplikator auf den
	 * Verkaufspreis. F�r max_products Produkte kauft man das Produkt zum
	 * einfachen Verkaufspreis, danach erh�ht sich der Multiplikator um 1.
	 * counter ist ein Z�hler, der die gekauften Produkte z�hlt. money
	 * wird f�r die debug message gebraucht. */

	max_products = rpeasants(r) / TRADE_FRACTION;

	/* Kauf - auch so programmiert, da� er leicht erweiterbar auf mehrere
	 * G�ter pro Monat ist. j sind die Befehle, i der Index des
	 * gehandelten Produktes. */

	expandorders(r, buyorders);
	if (!norders) return;

	for (j = 0; j != norders; j++) {
		int price, multi;
		ltype = oa[j].type.ltype;
		trade = trades;
		while (trade->type!=ltype) ++trade;
		multi = trade->multi;
		if (trade->number + 1 > max_products) ++multi;
		price = ltype->price * multi;

		if (get_pooled(oa[j].unit, r, R_SILVER) >= price) {
			unit * u = oa[j].unit;

			/* litems z�hlt die G�ter, die verkauft wurden, u->n das Geld, das
			 * verdient wurde. Dies mu� gemacht werden, weil der Preis st�ndig sinkt,
			 * man sich also das verdiente Geld und die verkauften Produkte separat
			 * merken mu�. */
			attrib * a = a_find(u->attribs, &at_luxuries);
			if (a==NULL) a = a_add(&u->attribs, a_new(&at_luxuries));
			i_change((item**)&a->data.v, ltype->itype, 1);
			i_change(&oa[j].unit->items, ltype->itype, 1);
			use_pooled(u, r, R_SILVER, price);
			if (u->n < 0)
				u->n = 0;
			u->n += price;

			rsetmoney(r, rmoney(r) + price);

			/* Falls mehr als max_products Bauern ein Produkt verkauft haben, steigt
			 * der Preis Multiplikator f�r das Produkt um den Faktor 1. Der Z�hler
			 * wird wieder auf 0 gesetzt. */
			if (++trade->number > max_products) {
				trade->number = 0;
				++trade->multi;
			}
		}
	}
	free(oa);

	/* Ausgabe an Einheiten */

	for (u = r->units; u; u = u->next) {
		attrib * a = a_find(u->attribs, &at_luxuries);
		item * itm;
		if (a==NULL) continue;
		add_message(&u->faction->msgs, new_message(u->faction,
			"buy%u:unit%i:money", u, u->n));
		for (itm=(item*)a->data.v; itm; itm=itm->next) {
			if (itm->number) {
				add_message(&u->faction->msgs, new_message(u->faction,
					"buyamount%u:unit%i:amount%X:resource", u, itm->number, itm->type->rtype));
			}
		}
		a_remove(&u->attribs, a);
	}
}

attrib_type at_trades = {
	"trades",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

void
buy(region * r, unit * u, request ** buyorders, const char * cmd)
{
	int n, k;
	request *o;
	attrib * a;
	const item_type * itype = NULL;
	const luxury_type * ltype = NULL;
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, cmd, 69, MSG_INCOME);
		return;
	}
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, cmd, 69, MSG_INCOME);
		return;
	}
	/* Im Augenblick kann man nur 1 Produkt kaufen. expandbuying ist aber
	 * schon daf�r ausger�stet, mehrere Produkte zu kaufen. */

	n = geti();
	if (!n) {
		cmistake(u, cmd, 26, MSG_COMMERCE);
		return;
	}
	if (besieged(u)) {
		/* Belagerte Einheiten k�nnen nichts kaufen. */
		cmistake(u, cmd, 60, MSG_COMMERCE);
		return;
	}

	if (u->race == new_race[RC_INSECT]) {
		/* entweder man ist insekt, oder... */
		if (rterrain(r) != T_SWAMP && rterrain(r) != T_DESERT && !rbuildings(r)) {
			cmistake(u, cmd, 119, MSG_COMMERCE);
			return;
		}
	} else {
		/* ...oder in der Region mu� es eine Burg geben. */
		if (!rbuildings(r)) {
			cmistake(u, cmd, 119, MSG_COMMERCE);
			return;
		}
	}

	/* Ein H�ndler kann nur 10 G�ter pro Talentpunkt handeln. */
	k = u->number * 10 * eff_skill(u, SK_TRADE, r);

	/* hat der H�ndler bereits gehandelt, muss die Menge der bereits
	 * verkauften/gekauften G�ter abgezogen werden */
	a = a_find(u->attribs, &at_trades);
	if (!a) {
		a = a_add(&u->attribs, a_new(&at_trades));
	} else {
		k -= a->data.i;
	}

	n = min(n, k);

	if (!n) {
		cmistake(u, cmd, 102, MSG_COMMERCE);
		return;
	}

	assert(n>=0);
	/* die Menge der verkauften G�ter merken */
	a->data.i += n;

	itype = finditemtype(getstrtoken(), u->faction->locale);
	if (itype!=NULL) {
		ltype = resource2luxury(itype->rtype);
		if (ltype==NULL) {
			cmistake(u, cmd, 124, MSG_COMMERCE);
			return;
		}
	}
	if (r_demand(r, ltype)) {
		add_message(&u->faction->msgs,
			msg_error(u, cmd, "luxury_notsold", ""));
		return;
	}
	o = (request *) calloc(1, sizeof(request));
	o->type.ltype = ltype; /* sollte immer gleich sein */

	o->unit = u;
	o->qty = n;
	addlist(buyorders, o);

	if (n && !fval(u, FL_TRADER)) fset(u, FL_TRADER);
}
/* ------------------------------------------------------------- */

/* Steuers�tze in % bei Burggr��e */
static int tax_per_size[7] =
{0, 6, 12, 18, 24, 30, 36};

static void
expandselling(region * r, request * sellorders)
{
	int money, price, j, max_products;
	/* int m, n = 0; */
	int maxsize = 0, maxeffsize = 0;
	int taxcollected = 0;
	int hafencollected = 0;
	unit *maxowner = (unit *) NULL;
	building *maxb = (building *) NULL;
	building *b;
	unit *u;
	unit *hafenowner;
	static int *counter;
	static int ncounter = 0;

	if (ncounter==0) {
		const luxury_type * ltype;
		for (ltype=luxurytypes;ltype;ltype=ltype->next) ++ncounter;
		counter=(int*)gc_add(calloc(sizeof(int), ncounter));
	} else {
		memset(counter, 0, sizeof(int)*ncounter);
	}

	if (!sellorders)	/* NEIN, denn Insekten k�nnen in	|| !r->buildings) */
		return;					/* S�mpfen und W�sten auch so handeln */

	/* Stelle Eigent�mer der gr��ten Burg fest. Bekommt Steuern aus jedem
	 * Verkauf. Wenn zwei Burgen gleicher Gr��e bekommt gar keiner etwas. */

	for (b = rbuildings(r); b; b = b->next) {
		if (b->size > maxsize && buildingowner(r, b) != NULL
			&& b->type == &bt_castle) {
			maxb = b;
			maxsize = b->size;
			maxowner = buildingowner(r, b);
		} else if (b->size == maxsize && b->type == &bt_castle) {
			maxb = (building *) NULL;
			maxowner = (unit *) NULL;
		}
	}

	hafenowner = owner_buildingtyp(r, &bt_harbour);

	if (maxb != (building *) NULL && maxowner != (unit *) NULL) {
		maxeffsize = buildingeffsize(maxb, false);
		if (maxeffsize == 0) {
			maxb = (building *) NULL;
			maxowner = (unit *) NULL;
		}
	}
	/* Die Region muss genug Geld haben, um die Produkte kaufen zu k�nnen. */

	money = rmoney(r);

	/* max_products sind 1/100 der Bev�lkerung, falls soviele Produkte
	 * verkauft werden - counter[] - sinkt die Nachfrage um 1 Punkt.
	 * multiplier speichert r->demand f�r die debug message ab. */

	max_products = rpeasants(r) / TRADE_FRACTION;
	if (max_products <= 0) return;

	if (rterrain(r) == T_DESERT && buildingtype_exists(r, &bt_caravan)) {
		max_products = rpeasants(r) * 2 / TRADE_FRACTION;
	}
	/* Verkauf: so programmiert, dass er leicht auf mehrere Gueter pro
	 * Runde erweitert werden kann. */

	expandorders(r, sellorders);
	if (!norders) return;

	for (j = 0; j != norders; j++) {
		static const luxury_type * search=NULL;
		const luxury_type * ltype = oa[j].type.ltype;
		int multi = r_demand(r, ltype);
		static int i=-1;
		int use = 0;
		if (search!=ltype) {
			i=0;
			for (search=luxurytypes;search!=ltype;search=search->next) ++i;
		}
		if (counter[i]+1 > max_products && multi > 1) --multi;
		price = ltype->price * multi;

		if (money >= price) {
			int abgezogenhafen = 0;
			int abgezogensteuer = 0;
			unit * u = oa[j].unit;
			attrib * a = a_find(u->attribs, &at_luxuries);
			if (a==NULL) a = a_add(&u->attribs, a_new(&at_luxuries));
			i_change((item**)&a->data.v, ltype->itype, 1);
			++use;
			if (u->n < 0)
				u->n = 0;

			if (hafenowner != NULL) {
				if (hafenowner->faction != u->faction) {
					abgezogenhafen = price / 10;
					hafencollected += abgezogenhafen;
					price -= abgezogenhafen;
					money -= abgezogenhafen;
				}
			}
			if (maxb != NULL) {
				if (maxowner->faction != u->faction) {
					abgezogensteuer = price * tax_per_size[maxeffsize] / 100;
					taxcollected += abgezogensteuer;
					price -= abgezogensteuer;
					money -= abgezogensteuer;
				}
			}
			u->n += price;
			change_money(u, price);

			/* r->money -= price; --- dies wird eben nicht ausgef�hrt, denn die
			 * Produkte k�nnen auch als Steuern eingetrieben werden. In der Region
			 * wurden Silberst�cke gegen Luxusg�ter des selben Wertes eingetauscht!
			 * Falls mehr als max_products Kunden ein Produkt gekauft haben, sinkt
			 * die Nachfrage f�r das Produkt um 1. Der Z�hler wird wieder auf 0
			 * gesetzt. */

			if (++counter[i] > max_products) {
				int d = r_demand(r, ltype);
				if (d > 1)
					r_setdemand(r, ltype, d-1);
				counter[i] = 0;
			}
		}
		if (use>0) {
#ifdef NDEBUG
			new_use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
#else
			/* int i = */ new_use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
			/* assert(i==use); */
#endif
		}
	}
	free(oa);

	/* Steuern. Hier werden die Steuern dem Besitzer der gr��ten Burg gegeben. */

	if (maxowner) {
		if (taxcollected > 0) {
			change_money(maxowner, (int) taxcollected);
			add_income(maxowner, IC_TRADETAX, taxcollected, taxcollected);
/*			sprintf(buf, "%s verdient %d Silber durch den Handel in %s.",
				 unitname(maxowner), (int) taxcollected, regionid(r)); */
		}
	}
	if (hafenowner) {
		if (hafencollected > 0) {
			change_money(hafenowner, (int) hafencollected);
			add_income(hafenowner, IC_TRADETAX, hafencollected, hafencollected);
		}
	}
	/* Berichte an die Einheiten */

	for (u = r->units; u; u = u->next) {

		attrib * a = a_find(u->attribs, &at_luxuries);
		item * itm;
		if (a==NULL) continue;
		for (itm=(item*)a->data.v; itm; itm=itm->next) {
			if (itm->number) {
				add_message(&u->faction->msgs, new_message(u->faction,
					"sellamount%u:unit%i:amount%X:resource", u, itm->number, itm->type->rtype));
			}
		}
		a_remove(&u->attribs, a);
		add_income(u, IC_TRADE, u->n, u->n);
	}
}

void
sell(region * r, unit * u, request ** sellorders, const char * cmd)
{
	const item_type * itype;
	const luxury_type * ltype=NULL;
	int n;
	request *o;
	char *s;

	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, cmd, 69, MSG_INCOME);
		return;
	}
	/* sellorders sind KEIN array, weil f�r alle items DIE SELBE resource
	 * (das geld der region) aufgebraucht wird. */

	s = getstrtoken();
	if (findparam(s, u->faction->locale) == P_ANY) {
		n = rpeasants(r) / TRADE_FRACTION;
		if (rterrain(r) == T_DESERT && buildingtype_exists(r, &bt_caravan))
			n *= 2;
		if (n==0) {
			cmistake(u, cmd, 303, MSG_COMMERCE);
			return;
		}
	} else {
		n = atoi(s);
		if (n==0) {
			cmistake(u, cmd, 27, MSG_COMMERCE);
			return;
		}
	}
	/* Belagerte Einheiten k�nnen nichts verkaufen. */

	if (besieged(u)) {
		add_message(&u->faction->msgs,
			msg_error(u, cmd, "error60", ""));
		return;
	}
	/* In der Region mu� es eine Burg geben. */

	if (u->race == new_race[RC_INSECT]) {
		if (rterrain(r) != T_SWAMP && rterrain(r) != T_DESERT && !rbuildings(r)) {
			cmistake(u, cmd, 119, MSG_COMMERCE);
			return;
		}
	} else {
		if (!rbuildings(r)) {
			cmistake(u, cmd, 119, MSG_COMMERCE);
			return;
		}
	}

	/* Ein H�ndler kann nur 10 G�ter pro Talentpunkt verkaufen. */

	n = min(n, u->number * 10 * eff_skill(u, SK_TRADE, r));

	if (!n) {
		cmistake(u, cmd, 54, MSG_COMMERCE);
		return;
	}
	s=getstrtoken();
	itype = finditemtype(s, u->faction->locale);
	if (itype!=NULL) ltype = resource2luxury(itype->rtype);
	if (ltype==NULL) {
		cmistake(u, cmd, 126, MSG_COMMERCE);
		return;
	}
	else {
		attrib * a;
		int k, available;
		if (!r_demand(r, ltype)) {
			cmistake(u, cmd, 263, MSG_COMMERCE);
			return;
		}
		available = new_get_pooled(u, itype->rtype, GET_DEFAULT);

		/* Wenn andere Einheiten das selbe verkaufen, mu� ihr Zeug abgezogen
		 * werden damit es nicht zweimal verkauft wird: */
	 	for (o=*sellorders;o;o=o->next) {
			if (o->type.ltype==ltype && o->unit->faction == u->faction) {
				int fpool = o->qty - new_get_pooled(o->unit, itype->rtype, GET_RESERVE);
				available -= max(0, fpool);
			}
		}

		n = min(n, available);

		if (n <= 0) {
			cmistake(u, cmd, 264, MSG_COMMERCE);
			return;
		}
		/* Hier wird request->type verwendet, weil die obere limit durch
		 * das silber gegeben wird (region->money), welches f�r alle
		 * (!) produkte als summe gilt, als nicht wie bei der
		 * produktion, wo f�r jedes produkt einzeln eine obere limite
		 * existiert, so dass man arrays von orders machen kann. */

		/* Ein H�ndler kann nur 10 G�ter pro Talentpunkt handeln. */
		k = u->number * 10 * eff_skill(u, SK_TRADE, r);

		/* hat der H�ndler bereits gehandelt, muss die Menge der bereits
		 * verkauften/gekauften G�ter abgezogen werden */
		a = a_find(u->attribs, &at_trades);
		if (!a) {
			a = a_add(&u->attribs, a_new(&at_trades));
		} else {
			k -= a->data.i;
		}

		n = min(n, k);
		assert(n>=0);
		/* die Menge der verkauften G�ter merken */
		a->data.i += n;
		o = (request *) calloc(1, sizeof(request));
		o->unit = u;
		o->qty = n;
		o->type.ltype = ltype;
		addlist(sellorders, o);

		if (n && !fval(u, FL_TRADER)) fset(u, FL_TRADER);
	}
}
/* ------------------------------------------------------------- */

static void
expandstealing(region * r, request * stealorders)
{
	int i;

	expandorders(r, stealorders);
	if (!norders) return;

	/* F�r jede unit in der Region wird Geld geklaut, wenn sie Opfer eines
	 * Beklauen-Orders ist. Jedes Opfer mu� einzeln behandelt werden.
	 *
	 * u ist die beklaute unit. oa.unit ist die klauende unit.
	 */

	for (i = 0; i != norders && oa[i].unit->n <= oa[i].unit->wants; i++) {
		unit *u = findunitg(oa[i].no, r);
		int n = 0;
		if (u && u->region==r) n = get_all(u, R_SILVER);
#ifndef GOBLINKILL
		if (oa[i].type.goblin) { /* Goblin-Spezialklau */
			int uct = 0;
			unit *u2;
			assert(effskill(oa[i].unit, SK_STEALTH)>=4 || !"this goblin\'s talent is too low");
			for (u2 = r->units; u2; u2 = u2->next)
				if (u2->faction == u->faction)
					uct += u2->number;
			n -= uct * 2 * MAINTENANCE;
		}
#endif
		if (n>10 && rplane(r) && (rplane(r)->flags & PFL_NOALLIANCES)) {
			/* In Questen nur reduziertes Klauen */
			n = 10;
		}
		if (n > 0) {
			n = min(n, oa[i].unit->wants);
			use_all(u, R_SILVER, n);
			oa[i].unit->n = n;
			change_money(oa[i].unit, n);
		}
		add_income(oa[i].unit, IC_STEAL, oa[i].unit->wants, oa[i].unit->n);
	}
	free(oa);
}

/* ------------------------------------------------------------- */
void
plant(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
	const herb_type * htype;

	if (rterrain(r) == T_OCEAN) {
		return;
	}
	if (rherbtype(r) == NULL) {
		cmistake(u, findorder(u, u->thisorder), 108, MSG_PRODUCE);
		return;
	}

	/* Skill pr�fen */
	skill = eff_skill(u, SK_HERBALISM, r);
	if (skill < 6) {
		cmistake(u, findorder(u, u->thisorder), 150, MSG_PRODUCE);
		return;
	}
	/* Wasser des Lebens pr�fen */
	if (get_pooled(u, r, R_TREES) == 0) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "resource_missing", "missing",
				oldresourcetype[R_TREES]));
		return;
	}
	htype = rherbtype(r);
	n = new_get_pooled(u, htype->itype->rtype, GET_DEFAULT);
	/* Kr�uter pr�fen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "resource_missing", "missing",
				htype->itype->rtype));
		return;
	}

	n = min(skill*u->number, n);
	n = min(raw, n);
	/* F�r jedes Kraut Talent*10% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%10 < skill) planted++;
	}
	produceexp(u, SK_HERBALISM, u->number);

	/* Alles ok. Abziehen. */
	new_use_pooled(u, oldresourcetype[R_TREES], GET_DEFAULT, 1);
	new_use_pooled(u, htype->itype->rtype, GET_DEFAULT, n);
	rsetherbs(r, rherbs(r)+planted);
	add_message(&u->faction->msgs, new_message(u->faction,
		"plant%u:unit%r:region%i:amount%X:herb", u, r, planted, htype->itype->rtype));
}

#if GROWING_TREES
static void
planttrees(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
	const item_type * itype;
	
	if (rterrain(r) == T_OCEAN) {
		return;
	}
	
	/* Mallornb�ume kann man nur in Mallornregionen z�chten */
	if (fval(r, RF_MALLORN)) {
		itype = &it_mallornseed;
	} else {
		itype = &it_seed;
	}

	/* Skill pr�fen */
	skill = eff_skill(u, SK_HERBALISM, r);
	if (skill < 6) {
		cmistake(u, findorder(u, u->thisorder), 150, MSG_PRODUCE);
		return;
	}
	n = new_get_pooled(u, itype->rtype, GET_DEFAULT);
	/* Samen pr�fen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "resource_missing", "missing",
				itype->rtype));
		return;
	}

	/* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
	n = min(raw, n);
	n = min(skill*u->number, n);

	/* F�r jeden Samen Talent*10% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%10 < skill) planted++;
	}
	rsettrees(r, 0, rtrees(r, 0)+planted);
	
	/* Alles ok. Abziehen. */
	produceexp(u, SK_HERBALISM, u->number);
	new_use_pooled(u, itype->rtype, GET_DEFAULT, n);

	ADDMSG(&u->faction->msgs, msg_message("plant", 
		"unit region amount herb", u, r, planted, itype->rtype));
}
#endif

void
pflanze(region *r, unit *u)
{
	int m;
	char *s;
	param_t p;
	const item_type * itype = NULL;

	/* pflanze [<anzahl>] <parameter> */
	s = getstrtoken();
	m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	if(!s[0]){
		p = P_ANY;
	} else {
		p = findparam(s, u->faction->locale);
		itype = finditemtype(s, u->faction->locale);
	}

	if (p==P_HERBS){
		plant(r, u, m);
		return;
	}
#if GROWING_TREES
	else if (p==P_TREES){
		planttrees(r, u, m);
		return;
	}
	else if (itype!=NULL){
		if (itype==&it_mallornseed || itype==&it_seed){
			planttrees(r, u, m);
			return;
		}
	}
#endif
}

#if GROWING_TREES

/* z�chte b�ume */
void
breedtrees(region *r, unit *u, int raw)
{
	int n, i, skill, planted = 0;
	const item_type * itype;
	int current_season = season(turn);
	
	/* B�ume z�chten geht nur im Fr�hling */
	if (current_season != SEASON_SPRING){
		planttrees(r, u, raw);
		return;
	}

	if (rterrain(r) == T_OCEAN) {
		return;
	}
	
	/* Mallornb�ume kann man nur in Mallornregionen z�chten */
	if (fval(r, RF_MALLORN)) {
		itype = &it_mallornseed;
	} else {
		itype = &it_seed;
	}

	/* Skill pr�fen */
	skill = eff_skill(u, SK_HERBALISM, r);
	if (skill < 12) {
		cmistake(u, findorder(u, u->thisorder), 150, MSG_PRODUCE);
		return;
	}
	n = new_get_pooled(u, itype->rtype, GET_DEFAULT);
	/* Samen pr�fen */
	if (n==0) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "resource_missing", "missing",
				itype->rtype));
		return;
	}

	/* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
	n = min(raw, n);
	n = min(skill*u->number, n);

	/* F�r jeden Samen Talent*5% Erfolgschance. */
	for(i = n; i>0; i--) {
		if (rand()%100 < skill*5) planted++;
	}
	rsettrees(r, 1, rtrees(r, 1)+planted);
	
	/* Alles ok. Abziehen. */
	produceexp(u, SK_HERBALISM, u->number);
	new_use_pooled(u, itype->rtype, GET_DEFAULT, n);

	add_message(&u->faction->msgs, new_message(u->faction,
		"plant%u:unit%r:region%i:amount%X:herb", u, r, planted, itype->rtype));
}
#endif

/* z�chte pferde */
void
breedhorses(region *r, unit *u)
{
	int n, c;
	int gezuechtet = 0;
	struct building * b = inside_building(u);
	const struct building_type * btype = b?b->type:NULL;

	if (btype!=&bt_stables) {
		cmistake(u, findorder(u, u->thisorder), 122, MSG_PRODUCE);
		return;
	}
	if (get_item(u, I_HORSE) < 2) {
		cmistake(u, findorder(u, u->thisorder), 107, MSG_PRODUCE);
		return;
	}
	n = min(u->number * eff_skill(u, SK_HORSE_TRAINING, r), get_item(u, I_HORSE));

	for (c = 0; c < n; c++) {
		if (rand() % 100 < eff_skill(u, SK_HORSE_TRAINING, r)) {
			change_item(u, I_HORSE, 1);
			gezuechtet++;
		}
	}

	if (gezuechtet > 0) {
		produceexp(u, SK_HORSE_TRAINING, u->number);
	}

	add_message(&u->faction->msgs, new_message(u->faction,
		"raised%u:unit%i:amount", u, gezuechtet));
}

void
zuechte(region *r, unit *u)
{
	int m;
	char *s;
	param_t p;

	/* z�chte [<anzahl>] <parameter> */
	s = getstrtoken();
	m = atoi(s);
	sprintf(buf, "%d", m);
	if (!strcmp(buf, s)) {
		/* first came a want-paramter */
		s = getstrtoken();
	} else {
		m = INT_MAX;
	}

	if(!s[0]){
		p = P_ANY;
	} else {
		p = findparam(s, u->faction->locale);
	}

	switch (p) {
		case P_HERBS:
			plant(r, u, m);
			return;
#if GROWING_TREES
		case P_TREES:
			breedtrees(r, u, m);
			return;
#endif
		default:
			breedhorses(r, u);
			return;
	}
}

static const char *
rough_amount(int a, int m)

{
	int p = (a * 100)/m;

	if (p < 10) {
		return "sehr wenige";
	} else if (p < 30) {
		return "wenige";
	} else if (p < 60) {
		return "relativ viele";
	} else if (p < 90) {
		return "viele";
	}
	return "sehr viele";
}

static void
research(region *r, unit *u)
{
	char *s;

	s = getstrtoken();

	if (findparam(s, u->faction->locale) == P_HERBS) {

		if (eff_skill(u, SK_HERBALISM, r) < 7) {
			cmistake(u, findorder(u, u->thisorder), 227, MSG_EVENT);
			return;
		}

		produceexp(u, SK_HERBALISM, u->number);

		if (rherbs(r) > 0) {
			const herb_type *rht = rherbtype(r);

			if (rht != NULL) {
				add_message(&u->faction->msgs, new_message(u->faction,
					"researchherb%u:unit%r:region%s:amount%X:herb", u, r,
					rough_amount(rherbs(r), 100), rht->itype->rtype));
			} else {
				add_message(&u->faction->msgs, new_message(u->faction,
					"researchherb_none%u:unit%r:region", u, r));
			}
		} else {
			add_message(&u->faction->msgs, new_message(u->faction,
				"researchherb_none%u:unit%r:region", u, r));
		}
	}
}

int
wahrnehmung(region * r, faction * f)
{
	unit *u;
	int w = 0;

	for (u = r->units; u; u = u->next) {
		if (u->faction == f
#ifdef HELFE_WAHRNEHMUNG
				|| allied(u, f, HELP_OBSERVE)
#endif
			) {
			if (eff_skill(u, SK_OBSERVATION, r) > w) {
				w = eff_skill(u, SK_OBSERVATION, r);
			}
		}
	}

	return w;
}

void
steal(region * r, unit * u, request ** stealorders)
{
	int n, i, id;
	boolean goblin = false;
	request * o;
	unit * u2 = NULL;
	faction * f = NULL;

	if (r->planep && fval(r->planep, PFL_NOATTACK)) {
		cmistake(u, findorder(u, u->thisorder), 270, MSG_INCOME);
		return;
	}

	id = read_unitid(u->faction, r);
	u2 = findunitr(r, id);

	if (u2 && u2->region==u->region) {
		f = u2->faction;
	} else {
		f = dfindhash(id);
	}

	for (u2=r->units;u2;u2=u2->next) {
		if (u2->faction == f && cansee(u->faction, r, u2, 0)) break;
	}

	if (!u2) {
		cmistake(u, findorder(u, u->thisorder), 64, MSG_INCOME);
		return;
	}

	if (u2->faction->age < IMMUN_GEGEN_ANGRIFF) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "newbie_immunity_error", "turns", IMMUN_GEGEN_ANGRIFF));
		return;
	}

	assert(u->region==u2->region);
	if (!can_contact(r, u, u2)) {
		add_message(&u->faction->msgs, msg_error(u, findorder(u, u->thisorder), "error60", ""));
		return;
	}
	n = eff_skill(u, SK_STEALTH, r) - wahrnehmung(r, f);

	if (n == 0) {
		/* Wahrnehmung == Tarnung */
		if (u->race != new_race[RC_GOBLIN] || eff_skill(u, SK_STEALTH, r) <= 3) {
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfail%u:unit%u:target", u, u2));
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"stealdetect%u:unit", u2));
			return;
		} else {
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfatal%u:unit%u:target", u, u2));
			n = 1;
			goblin = true;
		}
	} else if (n < 0) {
		/* Wahrnehmung > Tarnung */
		if (u->race != new_race[RC_GOBLIN] || eff_skill(u, SK_STEALTH, r) <= 3) {
			add_message(&u->faction->msgs, new_message(u->faction,
				"stealfatal%u:unit%u:target", u, u2));

			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			return;
		} else {	/* Goblin-Spezialdiebstahl, Meldung an Beklauten */
			add_message(&u2->faction->msgs, new_message(u2->faction,
				"thiefdiscover%u:unit%u:target", u, u2));
			n = 1;
			goblin = true;
		}
	}
	add_message(&u2->faction->msgs, new_message(u2->faction,
		"stealeffect%u:unit%r:region", u2, u2->region));

	n = max(0, n);

	i = min(u->number, get_item(u,I_RING_OF_NIMBLEFINGER));
	if (i > 0) {
		n *= STEALINCOME * (u->number + i * 9);
	} else {
		n *= u->number * STEALINCOME;
	}

	u->wants = n;

	/* wer dank unsichtbarkeitsringen klauen kann, muss nicht unbedingt ein
	 * guter dieb sein, schliesslich macht man immer noch sehr viel laerm */

	o = (request *) calloc(1, sizeof(request));
	o->unit = u;
	o->qty = 1; /* Betrag steht in u->wants */
	o->no = u2->no;
	o->type.goblin = goblin; /* Merken, wenn Goblin-Spezialklau */
	addlist(stealorders, o);

	/* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */

	produceexp(u, SK_STEALTH, min(n, u->number));
}
/* ------------------------------------------------------------- */


int
entertainmoney(const region *r)
{
	int n;

	if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
		return 0;
	}

	n = rmoney(r) / ENTERTAINFRACTION;

	if (is_cursed(r->attribs, C_GENEROUS, 0)) {
		n *= get_curseeffect(r->attribs, C_GENEROUS, 0);
	}

	return n;
}

static void
expandentertainment(region * r)
{
	unit *u;
	int m = entertainmoney(r);
	request *o;

	for (o = &entertainers[0]; o != nextentertainer; ++o) {
		double part = m / (double) entertaining;
		u = o->unit;
		if (entertaining <= m)
			u->n = o->qty;
		else
			u->n = (int) (o->qty * part);
		change_money(u, u->n);
		rsetmoney(r, rmoney(r) - u->n);
		m -= u->n;
		entertaining -= u->n;

		/* Nur soviel PRODUCEEXP wie auch tats�chlich gemacht wurde */
		produceexp(u, SK_ENTERTAINMENT, min(u->n, u->number));
		add_income(u, IC_ENTERTAIN, o->qty, u->n);
	}
}

void
entertain(region * r, unit * u)
{
	int max_e;
	request *o;

	if (!effskill(u, SK_ENTERTAINMENT)) {
		cmistake(u, findorder(u, u->thisorder), 58, MSG_INCOME);
		return;
	}
	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_INCOME);
		return;
	}
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, findorder(u, u->thisorder), 69, MSG_INCOME);
		return;
	}
	if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
		cmistake(u, findorder(u, u->thisorder), 28, MSG_INCOME);
		return;
	}

	u->wants = u->number * effskill(u, SK_ENTERTAINMENT) * ENTERTAININCOME;
	if ((max_e = geti()) != 0)
		u->wants = min(u->wants,max_e);

	o = nextentertainer++;
	o->unit = u;
	o->qty = u->wants;
	entertaining += o->qty;
}

/* ------------------------------------------------------------- */


static void
expandwork(region * r)
{
	int n, earnings;
	/* n: verbleibende Einnahmen */
	/* m: maximale Arbeiter */
	int m = maxworkingpeasants(r);
	int p_wage = wage(r,NULL,false);
	int verdienst = 0;
	request *o;

	for (o = &workers[0]; o != nextworker; ++o) {
		unit * u = o->unit;
		int workers;

		if (u->number == 0) continue;

		if (m>=working) workers = u->number;
		else {
			workers = u->number * m / working;
			if (rand() % working < (u->number * m) % working) workers++;
		}

		assert(workers>=0);

		u->n = workers * wage(u->region, u, false);

		m -= workers;
		assert(m>=0);

		change_money(u, u->n);
		working -= o->unit->number;
		add_income(u, IC_WORK, o->qty, u->n);
	}

	n = m * p_wage;

	/* Der Rest wird von den Bauern verdient. n ist das uebriggebliebene
	 * Geld. */
#if AFFECT_SALARY
	{
		attrib * a = a_find(r->attribs, &at_salary);
		if (a) verdienst = a->data.i;
	}
#endif
	earnings = min(n, rpeasants(r) * p_wage) + verdienst;
	/* Mehr oder weniger durch Trank "Riesengrass" oder "Faulobstschnaps" */

	if (verdienst) {
		sprintf(buf, "%d Bauern verdienen ein Silber %s durch %s.",
				abs(verdienst), (verdienst > 0) ? "mehr" : "weniger",
				(verdienst > 0) ? "Riesengras" : "Faulobstschnaps");
	}
	rsetmoney(r, rmoney(r) + earnings);
}

void
work(region * r, unit * u)
{
	request *o;
	int w;

	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_INCOME);
		return;
	}
	if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
		cmistake(u, findorder(u, u->thisorder), 69, MSG_INCOME);
		return;
	}
	w = wage(r,u,false);
	u->wants = u->number * w;
	o = nextworker++;
	o->unit = u;
	o->qty = u->number * w;
	working += u->number;
}
/* ------------------------------------------------------------- */

static void
expandtax(region * r, request * taxorders)
{
	unit *u;
	int i;

	expandorders(r, taxorders);
	if (!norders) return;

	for (i = 0; i != norders && rmoney(r) > TAXFRACTION; i++) {
		change_money(oa[i].unit, TAXFRACTION);
		oa[i].unit->n += TAXFRACTION;
		rsetmoney(r, rmoney(r) -TAXFRACTION);
	}
	free(oa);

	for (u = r->units; u; u = u->next)
		if (u->n >= 0)
			add_income(u, IC_TAX, u->wants, u->n);
}

void
tax(region * r, unit * u, request ** taxorders)
{
	/* Steuern werden noch vor der Forschung eingetrieben */

	unit *u2;
	int n;
	request *o;
	int max;

	if (!humanoidrace(u->race) && u->faction != findfaction(MONSTER_FACTION)) {
		cmistake(u, findorder(u, u->thisorder), 228, MSG_INCOME);
		return;
	}

	if (besieged(u)) {
		cmistake(u, findorder(u, u->thisorder), 60, MSG_INCOME);
		return;
	}
	n = armedmen(u);

	if (!n) {
		cmistake(u, findorder(u, u->thisorder), 48, MSG_INCOME);
		return;
	}
	if ((max = geti()) == 0) max = INT_MAX;
	if (!playerrace(u->race)) {
		u->wants = min(income(u), max);
	} else {
		u->wants = min(n * eff_skill(u, SK_TAXING, r) * 20, max);
	}

	u2 = is_guarded(r, u, GUARD_TAX);
	if (u2) {
		add_message(&u->faction->msgs,
			msg_error(u, findorder(u, u->thisorder), "region_guarded", "guard", u2));
		return;
	}

	/* die einnahmen werden in fraktionen von 10 silber eingeteilt: diese
	 * fraktionen werden dann bei eintreiben unter allen eintreibenden
	 * einheiten aufgeteilt. */

	o = (request *) calloc(1, sizeof(request));
	o->qty = u->wants / TAXFRACTION;
	o->unit = u;
	addlist(taxorders, o);
	return;
}
/* ------------------------------------------------------------- */

void
produce(void)
{
	region *r;
	request *taxorders, *sellorders, *stealorders, *buyorders;
	unit *u;
	int todo;

	/* das sind alles befehle, die 30 tage brauchen, und die in thisorder
	 * stehen! von allen 30-tage befehlen wird einfach der letzte verwendet
	 * (dosetdefaults).
	 *
	 * kaufen vor einnahmequellen. da man in einer region dasselbe produkt
	 * nicht kaufen und verkaufen kann, ist die reihenfolge wegen der
	 * produkte egal. nicht so wegen dem geld.
	 *
	 * lehren vor lernen. */

	for (r = regions; r; r = r->next) {

		assert(rmoney(r) >= 0);
		assert(rpeasants(r) >= 0);

		buyorders = 0;
		sellorders = 0;
		nextworker = &workers[0];
		working = 0;
		nextentertainer = &entertainers[0];
		entertaining = 0;
		taxorders = 0;
		stealorders = 0;

		for (u = r->units; u; u = u->next) {
			strlist * s;

			if (u->race == new_race[RC_SPELL] || fval(u, FL_LONGACTION))
				continue;

			if (u->race == new_race[RC_INSECT] && r_insectstalled(r) &&
					!is_cursed(u->attribs, C_KAELTESCHUTZ,0))
				continue;

			if (attacked(u) && *(u->thisorder) != 0) {
				cmistake(u, findorder(u, u->thisorder), 52, MSG_PRODUCE);
				continue;
			}

			for (s=u->orders;s;s=s->next) {
				todo = igetkeyword(s->s, u->faction->locale);
				switch (todo) {
				case K_BUY:
					buy(r, u, &buyorders, s->s);
					break;
				case K_SELL:
					sell(r, u, &sellorders, s->s);
					break;
				}
			}

			if (fval(u, FL_TRADER)) {
				attrib * a = a_find(u->attribs, &at_trades);
				if (a && a->data.i) {
					produceexp(u, SK_TRADE, u->number);	
				}
				u->thisorder[0]=0;
				continue;
			}

			todo = igetkeyword(u->thisorder, u->faction->locale);
			if (todo == NOKEYWORD) continue;

			if (rterrain(r) == T_OCEAN && u->race != new_race[RC_AQUARIAN]
					&& !(u->race->flags & RCF_SWIM)
					&& todo != K_STEAL && todo != K_SPY && todo != K_SABOTAGE)
				continue;

			switch (todo) {
			case K_MAKE:
				make(r, u);
				break;

			case K_ENTERTAIN:
				entertain(r, u);
				break;

			case K_WORK:
				if (!!playerrace(u->race))
					work(r, u);
				else if (u->faction->no > 0) {
					add_message(&u->faction->msgs,
						msg_error(u, findorder(u, u->thisorder), "race_cantwork", "race", u->race));
				}
				break;

			case K_TAX:
				tax(r, u, &taxorders);
				break;

			case K_STEAL:
				steal(r, u, &stealorders);
				break;

			case K_SPY:
				spy(r, u);
				break;

			case K_SABOTAGE:
				sabotage(r, u);
				break;

			case K_ZUECHTE:
				zuechte(r, u);
				break;

#if GROWING_TREES
			case K_PFLANZE:
				planttrees(r, u, INT_MAX);
				break;
#endif

			case K_RESEARCH:
				research(r, u);
				break;
			}
		}

		/* Entertainment (expandentertainment) und Besteuerung (expandtax) vor den
		 * Befehlen, die den Bauern mehr Geld geben, damit man aus den Zahlen der
		 * letzten Runde berechnen kann, wieviel die Bauern f�r Unterhaltung
		 * auszugeben bereit sind. */
		split_allocations(r);
		if (entertaining) expandentertainment(r);
		expandwork(r);
		if (taxorders) expandtax(r, taxorders);

		/* An erster Stelle Kaufen (expandbuying), die Bauern so Geld bekommen, um
		 * nachher zu beim Verkaufen (expandselling) den Spielern abkaufen zu
		 * k�nnen. */

		if (buyorders) expandbuying(r, buyorders);
		if (sellorders) expandselling(r, sellorders);

		/* Die Spieler sollen alles Geld verdienen, bevor sie beklaut werden
		 * (expandstealing). */

		if (stealorders) expandstealing(r, stealorders);

		assert(rmoney(r) >= 0);
		assert(rpeasants(r) >= 0);

	}
	if (vec) {
		free(vec);
		vec = 0;
	}
}

void
init_economy(void)
{
	add_allocator(make_allocator(item2resource(olditemtype[I_HORSE]), attrib_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_WOOD]), attrib_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_MALLORN]), attrib_allocation));
#if NEW_RESOURCEGROWTH
	add_allocator(make_allocator(item2resource(olditemtype[I_STONE]), leveled_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_IRON]), leveled_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_LAEN]), leveled_allocation));
#if GROWING_TREES
	add_allocator(make_allocator(&rt_seed, attrib_allocation));
	add_allocator(make_allocator(&rt_mallornseed, attrib_allocation));
#endif
#else
	add_allocator(make_allocator(item2resource(olditemtype[I_STONE]), attrib_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_IRON]), attrib_allocation));
	add_allocator(make_allocator(item2resource(olditemtype[I_LAEN]), attrib_allocation));
#endif
}

