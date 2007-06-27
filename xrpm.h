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
 *   (orc@pell.chi.il.us)
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

/*
 * xrpm: a third-party attempt to write something that can deal
 * with redhat's proprietary packaging system
 */
#ifndef _XRPM_D
#define _XRPM_D

/*  This header is from the documentation for rpm 2.2.6.  It will
 *  probably not work with any other version of rpm.
 */
#define MAJOR		2
#define MAGIC		(DWORD)0xDBEEABED

#define SIGHDR_MAGIC	(DWORD)0x01E8AD8E

struct rpm_header {
    DWORD magic;
    BYTE major, minor;
    WORD type;
    WORD archnum;
    BYTE name[66];
    WORD osnum;
    WORD signature_type;
    BYTE reserved[16];
};


/* the data (and, optionally, the signature) of a rpm file is stored in
 * a funky USCD-Pascal type directory.  The directory contains one header
 * block, an array of info blocks, and finally a chunk of data containing
 * all the data affiliated with the info blocks.  It looks rather like
 * this:
 *
 *              +---info[0].offset----+
 *              |                     |
 *              |      +---info[1].offset----+
 *              |      |              |      |
 *              |      |              v      v
 * +------+---------------/ /--------+-------------------------+
 * |HEADER|INFO 0|INFO 1| ... |INFO N|AMORPHOUS DATA BLOB....  |
 * +------+---------------/ /--------+-------------------------+
 *        |<-- 4 * header.nritems -->|<----- header.size ----->|
 */


/* the header size apparently varies from rpm version to rpm
 * version.  Version 2 rpm headers don't have magic numbers,
 * and version 3 rpm headers have 8 bytes of magic header.
 * This is, of course, not documented anywhere aside from the
 * source.
 */
struct rpm_super {
    DWORD nritems;	/* number of rpm_info items */
    DWORD size;		/* size of the data. */
} ;

struct rpm_info {
    DWORD tag;			/* what this info describes (not officially
				 * published -- this list is from rpm 2.2.6) */

#define RPMTAG_NAME  			1000
#define RPMTAG_VERSION			1001
#define RPMTAG_RELEASE			1002
#define RPMTAG_SERIAL   		1003
#define	RPMTAG_SUMMARY			1004
#define RPMTAG_DESCRIPTION		1005
#define RPMTAG_BUILDTIME		1006
#define RPMTAG_BUILDHOST		1007
#define RPMTAG_INSTALLTIME		1008
#define RPMTAG_SIZE			1009
#define RPMTAG_DISTRIBUTION		1010
#define RPMTAG_VENDOR			1011
#define RPMTAG_GIF			1012
#define RPMTAG_XPM			1013
#define RPMTAG_COPYRIGHT                1014
#define RPMTAG_PACKAGER                 1015
#define RPMTAG_GROUP                    1016
#define RPMTAG_CHANGELOG                1017
#define RPMTAG_SOURCE                   1018
#define RPMTAG_PATCH                    1019
#define RPMTAG_URL                      1020
#define RPMTAG_OS                       1021
#define RPMTAG_ARCH                     1022
#define RPMTAG_PREIN                    1023
#define RPMTAG_POSTIN                   1024
#define RPMTAG_PREUN                    1025
#define RPMTAG_POSTUN                   1026
#define RPMTAG_FILENAMES		1027
#define RPMTAG_FILESIZES		1028
#define RPMTAG_FILESTATES		1029
#define RPMTAG_FILEMODES		1030
#define RPMTAG_FILEUIDS			1031
#define RPMTAG_FILEGIDS			1032
#define RPMTAG_FILERDEVS		1033
#define RPMTAG_FILEMTIMES		1034
#define RPMTAG_FILEMD5S			1035
#define RPMTAG_FILELINKTOS		1036
#define RPMTAG_FILEFLAGS		1037
#define RPMTAG_ROOT                     1038
#define RPMTAG_FILEUSERNAME             1039
#define RPMTAG_FILEGROUPNAME            1040
#define RPMTAG_EXCLUDE                  1041 /* not used */
#define RPMTAG_EXCLUSIVE                1042 /* not used */
#define RPMTAG_ICON                     1043
#define RPMTAG_SOURCERPM                1044
#define RPMTAG_FILEVERIFYFLAGS          1045
#define RPMTAG_ARCHIVESIZE              1046
#define RPMTAG_PROVIDES                 1047
#define RPMTAG_REQUIREFLAGS             1048
#define RPMTAG_REQUIRENAME              1049
#define RPMTAG_REQUIREVERSION           1050
#define RPMTAG_NOSOURCE                 1051
#define RPMTAG_NOPATCH                  1052
#define RPMTAG_CONFLICTFLAGS            1053
#define RPMTAG_CONFLICTNAME             1054
#define RPMTAG_CONFLICTVERSION          1055
#define RPMTAG_DEFAULTPREFIX            1056
#define RPMTAG_BUILDROOT                1057
#define RPMTAG_INSTALLPREFIX		1058
#define RPMTAG_EXCLUDEARCH              1059
#define RPMTAG_EXCLUDEOS                1060
#define RPMTAG_EXCLUSIVEARCH            1061
#define RPMTAG_EXCLUSIVEOS              1062
#define RPMTAG_AUTOREQPROV              1063 /* used internally by builds */
#define RPMTAG_RPMVERSION		1064

    DWORD type;			/* data type, from the list below */

#define RI_NULL		0
#define RI_CHAR		1
#define RI_8BIT		2
#define RI_16BIT	3
#define RI_32BIT	4
#define RI_64BIT	5
#define RI_STRING	6
#define RI_BINARY	7
#define RI_STRINGARRAY	8
#define RI_STRING_INTL	9

    DWORD offset;		/* offset of the data in the data segment */
    DWORD count;			/* how many items can be found in the data segment */
} ;


/*
 * this is a compiled RPM header block.  It contains a superblock
 * (number of rpm_info records, size of data segment), an array
 * of rpm_info entries, and a data area for all the information
 * contained in those entries.
 */
struct rpm_info_header {
    struct rpm_super super;
    struct rpm_info* ino;
    BYTE*            data;
} ;


#endif/*_XRPM_D*/
