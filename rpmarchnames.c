/*
 * map architecture names to magic numbers
 */

#include "mapping.h"

struct mapping archmap[] = {
    { "Eniac",	0, "The ENIAC" },
    { "ia32",   1, "Intel 80386, 80486, 8050x, 8052x, and clones" },
    { "i386",	1 },	/* 80386 */
    { "i486",   1 },	/* 80486, AMD K5, Cyrix S */
    { "8050x",  1 },	/* Proper model number for the Intel `Pentium' chip */
    { "8052x",  1 },	/* Proper model number for the Intel `Pentium Pro' and
			 * `Pentium 2' chips */
    { "i586",   1 },	/* 8050x, NextGen, Cyrix Cx */
    { "i686",	1 },	/* 8052x, AMD K6 */
    { "i786",	1 },	/* I'm sure the ia64 machines will be misclassified here */
    { "osfmach3_i986",	1 },	/* you can never have too many names for the */
    { "osfmach3_i886",	1 },	/* same architecture */
    { "osfmach3_i786",	1 },
    { "osfmach3_i686",	1 },
    { "osfmach3_i586",	1 },
    { "osfmach3_i486",	1 },
    { "osfmach3_i386",	1 },
    { "alpha",	2, "DEC/Compaq ALPHA chip" },
    { "axp",	2 },
    { "sparc",	3, "SUN Sparc" },
    { "sun4",	3 },
    { "mips",	4, "MIPS/SGI Rx0000 chip family" },
    { "ppc",	5, "Motorola PowerPC family" },
    { "osfmach3_ppc",	5 },
    { "68000",	6, "Motorola 680x0 family" },
    { "68k",	6 },
    { "IP",	7, "Silicon Graphics workstations" },
    { "sgi",	7 },
} ;

int nrarchmap = (sizeof archmap / sizeof archmap[0]);
