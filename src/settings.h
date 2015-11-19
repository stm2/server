/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#ifndef H_SETTINGS
#define H_SETTINGS

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    E_RULES_GOBLIN_KILL,
    E_RULES_TROLLBELT_MULTIPLIER,
    ERESSEA_NUMPARAMETERS
} parameter_name_t;

/* TODO: turn defines into config parameters */

/*
 * Contains defines for the "free" game (Eressea) .
 * Include this file from settings.h to make eressea work.
 */
#define ENTERTAINFRACTION 20
#define TEACHDIFFERENCE 2
#define GUARD_DISABLES_RECRUIT 1
#define GUARD_DISABLES_PRODUCTION 1
#define RESOURCE_QUANTITY 0.5
#define RECRUITFRACTION 40      /* 100/RECRUITFRACTION% */
#define COMBAT_TURNS 5
#undef NEWATSROI

/* Vermehrungsrate Bauern in 1/10000.
* TODO: Evt. Berechnungsfehler, reale Vermehrungsraten scheinen h�her. */
#define PEASANTGROWTH 10
#define BATTLE_KILLS_PEASANTS 20
#define PEASANTLUCK 10

#define ROW_FACTOR 3            /* factor for combat row advancement rule */

/* optional game components. TODO: These should either be 
 * configuration variables (XML), script extensions (lua),
 * or both. We don't want separate binaries for different games
 */
#define MUSEUM_MODULE 1
#define ARENA_MODULE 1

#undef REGIONOWNERS             /* (WIP) region-owner uses HELP_TRAVEL to control entry to region */

#ifdef __cplusplus
}
#endif
#endif
