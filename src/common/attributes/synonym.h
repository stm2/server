/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_ATTRIBUTE_SYNONYM
#define H_ATTRIBUTE_SYNONYM
#ifdef __cplusplus
extern "C" {
#endif

struct attrib_type;
struct attrib;

typedef struct {
	const char *synonyms[4];
} frace_synonyms;

extern void init_synonym(void);
extern struct attrib_type at_synonym;

#ifdef __cplusplus
}
#endif
#endif
