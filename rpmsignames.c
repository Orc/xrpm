/*
 * names of the various signature types we can find in a header-style
 * signature.  This is likely to be incorrect.
 */
char *rpmsignames[]	= {
    "Size",
    0,
    "PGP",
    0,
    "MD5",
    "GPG",
    "Payload",
    0,
    0,
    "sha1",
    "dsa",
    "rsa",
} ;

int nrrpmsignames = (sizeof rpmsignames / sizeof rpmsignames[0]);
