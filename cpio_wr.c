/*-
 * Copyright (c) 1992 Keith Muller.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Keith Muller of the University of California, San Diego.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: cpio.c,v 1.3 1995/05/30 00:06:54 rgrimes
 */

static const char sccsid[] = "@(#)cpio.c	8.1 (Berkeley) 5/31/93";
static const char rcsid[] = "$Id: cpio_wr.c,v 1.2 1998/05/21 07:40:55 orc Exp $";

#include "config.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#include "pax.h"
#include "cpio_wr.h"
#include "makepkg.h"

#include "linux/kdev_t.h"	/* for MAJOR() and MINOR() */

long cur_archive_pos = 0;

/*
 * push() writes data to the output device, keeping track of how many bytes
 * have been written, for alignment tweakery.
 */
int
push(int fd, char* bfr, int buflen)
{
    int sz;

    sz = write(fd, bfr, buflen);
    if (sz < buflen)
	return -1;
    cur_archive_pos += sz;
    return 0;
} /* push */


/*
 * align() forces the archive data onto a word boundary
 */
int
align(int fd)
{
    int towrite  = VCPIO_PAD(cur_archive_pos);

    return (towrite > 0) ? push(fd, "\0\0\0\0", towrite) : 0;
} /* align */


/*
 *	convert an unsigned long into an hex/oct ascii string. pads with LEADING
 *	ascii 0's to fill string completely
 *	NOTE: the string created is NOT TERMINATED.
 */

static int
ul_asc(u_long val, register char *str, register int len, register int base)
{
	register char *pt;
	u_long digit;

	/*
	 * WARNING str is not '\0' terminated by this routine
	 */
	pt = str + len - 1;

	/*
	 * do a tailwise conversion (start at right most end of string to place
	 * least significant digit). Keep shifting until conversion value goes
	 * to zero (all digits were converted)
	 */
	if (base == HEX) {
		while (pt >= str) {
			if ((digit = (val & 0xf)) < 10)
				*pt-- = '0' + (char)digit;
			else
				*pt-- = 'a' + (char)(digit - 10);
			if ((val = (val >> 4)) == (u_long)0)
				break;
		}
	} else {
		while (pt >= str) {
			*pt-- = '0' + (char)(val & 0x7);
			if ((val = (val >> 3)) == (u_long)0)
				break;
		}
	}

	/*
	 * pad with leading ascii ZEROS. We return -1 if we ran out of space.
	 */
	while (pt >= str)
		*pt-- = '0';
	if (val != (u_long)0)
		return(-1);
	return(0);
}

/*
 * cpio_end_wr()
 *	write the special file with the name trailer in the proper format
 * Return:
 *	result of the write of the trailer from the cpio specific write func
 */

int
cpio_endwr(int dest)
{
	struct file eof;
	char blk[512];
	int st;

	/*
	 * create a trailer request and call the proper format write function
	 */
	memset(&eof, 0, sizeof(eof));
	eof.st.st_nlink = 1;
	eof.name = TRAILER;
	eof.dest = TRAILER;
	st = vcpio_wr(dest, &eof);

	if (st != -1) {
	    memset(blk, 0, sizeof blk);
	    st = 512 - (cur_archive_pos % 512);
	    push(dest, blk, st);
	}
}

/*
 * Routines common to the system VR4 version of cpio (with/without file CRC)
 */

/*
 * vcpio_wr()
 *	copy the data in the `struct file' to buffer in system VR4 cpio
 *	(with/without crc) format.
 * Return
 *	0 if file has data to be written after the header, 1 if file has
 *	NO data to write after the header, -1 if archive write failed
 */

int
vcpio_wr(int dest, register struct file *arcn)
{
	register HD_VCPIO *hd;
	unsigned int nsz;
	char hdblk[sizeof(HD_VCPIO)];
	char symlink[1000];
	long pos;

	/*
	 * check and repair truncated device and inode fields in the cpio
	 * header
	 */
	nsz = strlen(arcn->dest) + 1;
	hd = (HD_VCPIO *)hdblk;
	if (!S_ISCHR(arcn->st.st_mode) && !S_ISBLK(arcn->st.st_mode))
		arcn->st.st_rdev = 0;

	/*
	 * add the proper magic value depending whether we were asked for
	 * file data crc's, and the crc if needed.
	 */
	if (ul_asc((u_long)VMAGIC/*VCMAGIC*/, hd->c_magic, sizeof(hd->c_magic), OCT) ||
		ul_asc(0/*filecrc(arcn)*/, hd->c_chksum, sizeof(hd->c_chksum), HEX))
	    goto out;

	if (S_ISREG(arcn->st.st_mode)) {
		/*
		 * caller will copy file data to the archive. tell him how
		 * much to pad.
		 */
		arcn->pad = VCPIO_PAD(arcn->st.st_size);
		if (ul_asc((u_long)arcn->st.st_size, hd->c_filesize,
		    sizeof(hd->c_filesize), HEX)) {
			error("%s is too large for cpio header", arcn->name);
			return(1);
		}
	}
	else if (S_ISLNK(arcn->st.st_mode)) {
		/*
		 * no file data for the caller to process, the file data has
		 * the size of the link
		 */
		arcn->pad = 0L;
		if (ul_asc((u_long)arcn->st.st_size, hd->c_filesize,
		    sizeof(hd->c_filesize), HEX))
			goto out;
	}
	else {
		/*
		 * no file data for the caller to process
		 */
		arcn->pad = 0L;
		if (ul_asc((u_long)0L, hd->c_filesize, sizeof(hd->c_filesize),
		    HEX))
			goto out;
	}

	/*
	 * set the other fields in the header
	 */
	if (ul_asc((u_long)arcn->st.st_ino, hd->c_ino, sizeof(hd->c_ino),
		HEX) ||
	    ul_asc((u_long)arcn->st.st_mode, hd->c_mode, sizeof(hd->c_mode),
		HEX) ||
	    ul_asc((u_long)arcn->st.st_uid, hd->c_uid, sizeof(hd->c_uid),
		HEX) ||
	    ul_asc((u_long)arcn->st.st_gid, hd->c_gid, sizeof(hd->c_gid),
    		HEX) ||
	    ul_asc((u_long)arcn->st.st_mtime, hd->c_mtime, sizeof(hd->c_mtime),
    		HEX) ||
	    ul_asc((u_long)arcn->st.st_nlink, hd->c_nlink, sizeof(hd->c_nlink),
    		HEX) ||
	    ul_asc((u_long)MAJOR(arcn->st.st_dev),hd->c_maj, sizeof(hd->c_maj),
		HEX) ||
	    ul_asc((u_long)MINOR(arcn->st.st_dev),hd->c_min, sizeof(hd->c_min),
		HEX) ||
	    ul_asc((u_long)MAJOR(arcn->st.st_rdev),hd->c_rmaj,sizeof(hd->c_maj),
		HEX) ||
	    ul_asc((u_long)MINOR(arcn->st.st_rdev),hd->c_rmin,sizeof(hd->c_min),
		HEX) ||
	    ul_asc((u_long)nsz, hd->c_namesize, sizeof(hd->c_namesize), HEX))
		goto out;

	/*
	 * write the header, the file name and padding as required.
	 */
	if (push(dest, hdblk, sizeof(HD_VCPIO)) != 0 || push(dest, arcn->dest, nsz) != 0) {
		error("Could not write cpio header for %s", arcn->name);
		return(-1);
	}
	align(dest);

	/*
	 * if we have file data, tell the caller we are done, copy the file
	 */
	if (S_ISREG(arcn->st.st_mode))
		return 0;

	/*
	 * if we are not a link, tell the caller we are done, go to next file
	 */
	if (!S_ISLNK(arcn->st.st_mode))
		return 1;

	/*
	 * write the link name, tell the caller we are done.
	 */
	if (readlink(arcn->name, symlink, sizeof symlink) < 0) {
	    error("%s: %s", arcn->name, strerror(errno));
	    return -1;
	}
	if (push(dest, symlink, arcn->st.st_size) != 0) {
		error("Could not write link info for %s", arcn->name);
		return(-1);
	}
	align(dest);
	return(1);

    out:
	/*
	 * header field is out of range
	 */
	error("cpio header is too small for file %s", arcn->name);
	return(1);
}
