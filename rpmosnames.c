/*
 * various operating system names and how they map to magic numbers
 */

#include "mapping.h"

struct mapping osmap[] = {
    { "Linux", 1 },
    { "IRIX",  2 },
    { "solaris", 3 },
    { "SunOS5", 3 },		/* Solaris, both aliases */
    { "SunOS", 4 },
    { "SunOS4", 4 },		/* SunOS, both aliases */
} ;

int nrosmap = (sizeof osmap / sizeof osmap[0]);
