/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#ifndef H_GC_MONSTER
#define H_GC_MONSTER
#ifdef __cplusplus
extern "C" {
#endif

#define DRAGON_RANGE 20 /* Max. Distanz zum n�chsten Drachenziel */

void age_illusion(struct unit *u);

void monsters_kill_peasants(void);
void plan_monsters(void);
void age_unit(struct region * r, struct unit * u);
struct unit *random_unit(const struct region * r);
void check_split(void);

#ifdef __cplusplus
}
#endif
#endif
