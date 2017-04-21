/*
 *   Copyright (c) 1998 David Parsons. All rights reserved.
 *   
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *  3. All advertising materials mentioning features or use of this
 *     software must display the following acknowledgement:
 *     
 *   This product includes software developed by David Parsons
 *   (orc@pell.portland.or.us)
 *
 *  4. My name may not be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *     
 *  THIS SOFTWARE IS PROVIDED BY DAVID PARSONS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 *  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL DAVID
 *  PARSONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MAKEPKG_D
#define _MAKGPKG_D
/*
 * file information, for installation sanity checks.
 */
struct file {
    char *name;		/* the filename */
    char *dest;		/* destination filename (either from /prefix or => escape) */
    int  tobemoved;	/* TRUE if the file is moved with a => escape */
    struct stat st;	/* stat() info */
    int pad;		/* cpio output-- header padding */
};

/*
 * all the info we might want to wedge into an rpm package
 */
struct info {
    char *name;		/* [PACKAGE] section;	NAME= */
    char *version;			/* VERSION= */
    char *release;			/* RELEASE= */
    char *prefix;			/* PREFIX= */
    char *author;			/* PUBLISHER= */
    char *summary;			/* SUMMARY= */
    char *url;				/* URL= */
    char *os;				/* OS= */
    char *arch;				/* ARCH= */
    char *copyright;			/* COPYRIGHT= */
    char *distribution;			/* DISTRIBUTION= */
    char *description;	/* [DESCRIPTION] section */
    char **supplies;	/* [PROVIDES] section */
    int  nrsupplies;
    char **needs;	/* [NEEDS] section */
    int  nrneeds;
    struct file *file;	/* [FILES] section */
    int        nrfile;
    char *preinstall;	/* [PREINSTALL] section */
    char *install;	/* [INSTALL] section */
    char *uninstall;	/* [UNINSTALL] section */
};


extern char xrpm_version[];

#endif/*MAKEPKG_D*/
