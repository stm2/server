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

#include <config.h>
#include "aggressive.h"

#include <attrib.h>

attrib_type at_aggressive = {
	"aggressive",
	NULL,
	NULL,
	NULL,
  a_writedefault,
  a_readdefault,
  ATF_UNIQUE
};

void
init_aggressive(void)
{
	at_register(&at_aggressive);
}
