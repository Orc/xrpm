/*
 * names of the various signature types we can find in a header-style
 * signature.  This is likely to be incorrect.
 */
char *rpmsignames[]	= {
    "Size",
    "MD5",
    "PGP",
} ;

int nrrpmsignames = (sizeof rpmsignames / sizeof rpmsignames[0]);
