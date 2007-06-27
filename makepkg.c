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

static const char rcsid[] = "Mastodon $Id: makepkg.c,v 1.14 2000/05/21 16:33:42 orc Exp $";

/*
 * makepkg: generate a rpm package
 */
#include "config.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_X_GETOPT
#include <basis/options.h>
#endif

#include "makepkg.h"
#include "xrpm.h"
#include "mapping.h"

extern struct mapping osmap[];
extern int          nrosmap;
extern struct mapping archmap[];
extern int          nrarchmap;

struct utsname SystemInfo;


/* 
 * command-line options
 */
int verbose = 0;

/*
 * line cache, for getsectionline() and getsectionheader()
 */
static char cache[512];
static int iscached = 0;

/*
 * package sections
 */
enum { PACKAGE, DESCRIPTION, NEEDS, PREINSTALL, INSTALL, UNINSTALL, FILES };

struct sections {
    char *name;
    int section;
} sections [] = {
    { "[PACKAGE]",	PACKAGE },
    { "[DESCRIPTION]",	DESCRIPTION },
    { "[NEEDS]",	NEEDS },
    { "[PREINSTALL]",	PREINSTALL },
    { "[INSTALL]",	INSTALL },
    { "[FILES]",	FILES },
    { "[UNINSTALL]",	UNINSTALL },
};
#define NRSECTIONS	(sizeof sections/sizeof sections[0])

/*
 * error() spits out an error message
 */
static int lineno = 0;
static char *file = 0;
static int errors = 0;

void
error(char* fmt, ...)
{
    va_list ptr;

    if (file)
	fprintf(stderr, "%s : ", file);
    if (lineno > 0)
	fprintf(stderr, "%d : ", lineno);
    va_start(ptr, fmt);
    vfprintf(stderr, fmt, ptr);
    va_end(ptr);
    fputc('\n', stderr);
    errors = 1;
} /* error */


/*
 * read a line, but refuse to read section headers
 */
char*
getsectionline()
{
    static char line[512];
    char *ptr;

    if (iscached) {
	strcpy(line, cache);
	iscached = 0;
    }
    else {
	if (fgets(line, sizeof line, stdin) == 0)
	    return 0;
	++lineno;
	strtok(line, "\r\n");
    }
    if (line[0] == '[') {
	strcpy(cache, line);
	iscached = 1;
	return 0;
    }
    return line;
} /* getsectionline */


/*
 * read nothing but a section header
 */
char*
getsectionheader()
{
    static char line[512];

    if (iscached) {
	strcpy(line, cache);
	iscached = 0;
    }
    else {
	if (fgets(line, sizeof line, stdin) == 0)
	    return 0;
	lineno++;
	strtok(line, "\r\n");
    }
    if (line[0] != '[') {
	strcpy(cache, line);
	iscached = 1;
	return 0;
    }
    return line;
} /* getsectionheader */


/*
 * pick a white-space delimited token off a line, also handling strings
 */
char *
gettoken(char *line)
{
    static char token[512];
    char *p, *q;

    while (isspace(*line))
	++line;
    if (*line == 0)
	return 0;

    p = token;
    while (!isspace(*line) && *line) {
	if (*line == '"') {
	    ++line;
	    while (*line && *line != '"')
		*p++ = *line++;
	    if (*line)
		++line;
	}
	else
	    *p++ = *line++;
    }
    *p = 0;
    return (p > token) ? token : 0;
} /*gettoken*/


/*
 * eatspace() trims space off the start and end of a token
 */
char *
eatspace(char *token)
{
    char *p;
    while (isspace(*token))
	++token;

    p = token + strlen(token);
    while (p > token && isspace(p[-1]))
	--p;
    if (p > token) {
	*p = 0;
	return token;
    }
    return 0;
} /* eatspace */


/*
 * handle the [PACKAGE] section of an rpm file
 */
void
package_section(struct info *info)
{
    char *line;
    char *content;

    while ((line = getsectionline()) != 0) {
	if ((line = eatspace(line)) == 0)
	    continue;
	if (*line == ';')
	    continue;
	for (content=line; *content && *content != '='; ++content)
	    if (*content == 0) {
		error("malformed line");
		continue;
	    }
	    *content++ = 0;
	    if (strcasecmp(line, "NAME") == 0)
		info->name = strdup(gettoken(content));
	    else if (strcasecmp(line, "VERSION") == 0)
		info->version = strdup(gettoken(content));
	    else if (strcasecmp(line, "RELEASE") == 0)
		info->release = strdup(gettoken(content));
	    else if (strcasecmp(line, "PREFIX") == 0)
		info->prefix = strdup(gettoken(content));
	    else if (strcasecmp(line, "AUTHOR") == 0)
		info->author = strdup(gettoken(content));
	    else if (strcasecmp(line, "SUMMARY") == 0)
		info->summary = strdup(gettoken(content));
	    else if (strcasecmp(line, "URL") == 0)
		info->url = strdup(gettoken(content));
	    else if (strcasecmp(line, "ARCH") == 0)
		info->arch = strdup(gettoken(content));
	    else if (strcasecmp(line, "DISTRIBUTION") == 0)
		info->distribution = strdup(gettoken(content));
	    else if (strcasecmp(line, "COPYRIGHT") == 0)
		info->copyright = strdup(gettoken(content));
	    else if (strcasecmp(line, "OS") == 0)
		info->os = strdup(gettoken(content));
    }
} /* package_section */


/*
 * getstring() build a long string from all of the lines in
 * a section, trimming blank lines at the start and the end
 * of the section.
 */
char*
getstring()
{
    int size = 1;
    int firstline = 1;
    int nrlines = 0;
    char *p;
    char *string = malloc(1);

    *string = 0;

    while ((p = getsectionline()) != 0) {
	while (isspace(*p))
	    ++p;
	if (*p != 0) {
	    size += (strlen(p)+1);
	    if (firstline) {
		firstline = 0;
		nrlines = 0;
	    }
	    else
		size += nrlines;
	    string = realloc(string, size);
	    while (nrlines-- > 0)
		strcat(string, "\n");
	    strcat(string, p);
	    nrlines = 0;
	}
	nrlines++;
    }
    return string;
} /* getstring */


/*
 * handle the [NEEDS] section
 */
void
needs_section(struct info* info)
{
    char *p;

    while ((p = getsectionline()) != 0) {
	if ((p = eatspace(p)) == 0)
	    continue;
	if (*p == ';')
	    continue;

	info->needs = realloc(info->needs, (1+info->nrneeds) * sizeof info->needs[0]);
	info->needs[info->nrneeds] = strdup(p);
	(info->nrneeds)++;
    }
} /* needs_section */


/*
 * build a filelist of the files we want to install.  The [FILES] section
 * is a list of files, one per name, with, optionally, an escape telling
 * makepkg where they want to end up.  If you give a filename of
 * SOURCE => DESTDIR, makepkg will read the file off SOURCE, but write
 * into the archive so it will be unpacked as DEST.
 */
file_section(struct info* info)
{
    char *p;
    char *to;
    struct stat st;

    info->file = malloc(1);
    info->nrfile = 0;

    while ((p = getsectionline()) != 0) {
	if ((p = eatspace(p)) == 0)
	    continue;
	if (*p == ';')
	    continue;
	if ((to = strstr(p, "=>")) != 0) {
	    char *q = to-1;

	    /* what we're looking for is [ws]=>[ws] */
	    if (isspace(to[-1]) && isspace(to[2])) {
		to += 2;
		while (isspace(*to))
		   ++to;
		if (*to == 0) {
		    error("malformed file line");
		    continue;
		}

		while (q >= p && isspace(*q))
		    --q;
		q[1] = 0;
	    }
	}

	if (lstat(p, &st) == 0) {
	    info->file = realloc(info->file, sizeof(info->file[0]) * (20+info->nrfile));
	    info->file[info->nrfile].name = strdup(p);
	    if (to) {
		info->file[info->nrfile].tobemoved = 1;
		info->file[info->nrfile].dest = strdup(to);
	    }
	    else {
		info->file[info->nrfile].dest = 0;
		info->file[info->nrfile].tobemoved = 0;
	    }

	    info->file[info->nrfile].st = st;
	    info->nrfile++;
	}
	else
	    error("cannot pack nonexistant file %s\n", p);
    }
} /* file_section */


/*
 * makeheader() builds up a rpm header from the info block
 */
static struct rpm_header hdr;
static struct rpm_super sb;
static struct rpm_info *ino;
static char *data;


/*
 * add a string to the info block, returning the offset to it
 */
int
datastring(char *string)
{
    int offset = sb.size;

    sb.size += strlen(string) + 1;
    data = realloc(data, sb.size);

    strcpy(data + offset, string);

    return offset;
} /* datastring */


/*
 * add an 8-bit quantity to the data block, returning the offset
 * to it.
 */
int
data8bit(unsigned char value)
{
    int offset = sb.size;

    sb.size += 1;
    data = realloc(data, sb.size);
    data[offset] = value;
    return offset;
} /* data8bit */


/*
 * add a 16-bit quantity to the data block, returning the
 * offset to it.
 */
int
data16bit(unsigned short value)
{
    int offset = sb.size;

    if (offset & 1) {	/* force word alignment */
	offset++;
	sb.size++;
    }
    sb.size += sizeof value;
    data = realloc(data, sb.size);

    *((short*)(data+offset)) = htons(value);

    return offset;
} /* data16bit */


/*
 * add a 32 bit quantity to the data block, returning
 * the offset to it.
 */
int
data32bit(unsigned long value)
{
    int offset = sb.size;

    if (offset & 3) {	/* force doubleword alignment */
	sb.size = (sb.size + 4) & ~3;
	offset = sb.size;
    }
    sb.size += sizeof value;
    data = realloc(data, sb.size);

    *((long*)(data+offset)) = htonl(value);

    return offset;
} /* data32bit */


/*
 * create a new inode in the rpm_info array, returning
 * the inode number
 */
newino(short tag, int type, int count)
{
    ino = realloc(ino, (sb.nritems+1) * sizeof ino[0]);
    ino[sb.nritems].tag = htonl(tag);
    ino[sb.nritems].type = htonl(type);
    ino[sb.nritems].count = htonl(count);
    return sb.nritems++;
} /* newino */


/*
 * add a string to the rpm_info array
 */
addstring(short tag, char* string)
{
    int x;

    if (string) {
	x = newino(tag, RI_STRING, 1);
	ino[x].offset = htonl(datastring(string));
    }
} /* addstring */


/*
 * add an 8-bit quantity to the rpm_info array
 */
add8bit(short tag, char value)
{
    int x;

    x = newino(tag, RI_8BIT, 1);
    ino[x].offset = htonl(data8bit(value));
} /* add8bit */


/*
 * add a 32-bit quantity to the rpm_info array
 */
add32bit(short tag, unsigned long value)
{
    int x;

    x = newino(tag, RI_32BIT, 1);
    ino[x].offset = htonl(data32bit(value));
} /* add32bit */

/*
 * finally, the procedure that generates the header
 */
char *
makeheader(struct info *info)
{
    char scratch[200];
    int x;
    int os=EOF,
	arch=EOF;
    time_t now;

    /* allocate the parts of the header */
    memset(&sb, 0, sizeof sb);
    memset(&hdr, 0, sizeof hdr);
    ino = malloc(1);

    /* populate the rpm header */
    hdr.magic = MAGIC;
    hdr.major = 2;
    hdr.minor = 0;
    hdr.type  = 0;
    for (x = 0; x < nrosmap; x++)
	if (strcmp(info->os, osmap[x].name) == 0) {
	    os = osmap[x].number;
	    break;
	}
    for (x = 0; x < nrarchmap; x++)
	if (strcmp(info->arch, archmap[x].name) == 0) {
	    arch = archmap[x].number;
	    break;
	}

    if (os < 0)
	error("unknown os %s", info->os);
    if (arch < 0)
	error("unknown architecture %s", info->arch);
    if (errors)
	exit(1);

    hdr.archnum = htons(arch);
    hdr.osnum = htons(os);
    strcpy(hdr.name, info->name);
    hdr.signature_type = 0;	/* no signatures */

    /* add all single-valued tags
     */
    addstring(RPMTAG_NAME, info->name);
    addstring(RPMTAG_VERSION, info->version);
    addstring(RPMTAG_RELEASE, info->release ? info->release : "0");
    addstring(RPMTAG_SUMMARY, info->summary);
    if (info->description)
	addstring(RPMTAG_DESCRIPTION, info->description);
    time(&now);
    add32bit(RPMTAG_BUILDTIME, now);
#if HAVE_UTSNAME_DOMAINNAME
    sprintf(scratch, "%s.%s", SystemInfo.nodename, SystemInfo.domainname);
    addstring(RPMTAG_BUILDHOST, scratch);
#else
    addstring(RPMTAG_BUILDHOST, SystemInfo.nodename);
#endif
    if (info->distribution)
	addstring(RPMTAG_DISTRIBUTION, info->distribution);
    addstring(RPMTAG_VENDOR, info->author);
    addstring(RPMTAG_COPYRIGHT, info->copyright);
    add8bit(RPMTAG_OS, os);
    add8bit(RPMTAG_ARCH, arch);

    if (info->preinstall)
	addstring(RPMTAG_PREIN, info->preinstall);
    if (info->install)
	addstring(RPMTAG_POSTIN, info->install);
    if (info->uninstall)
	addstring(RPMTAG_PREUN, info->uninstall);


    /* add file information.
     */
    if (info->nrfile > 0) {
	x = newino(RPMTAG_FILENAMES, RI_STRINGARRAY, info->nrfile);
	ino[x].offset = htonl(datastring(info->file[0].dest));
	for (x = 1; x < info->nrfile; x++)
	    datastring(info->file[x].dest);

	x = newino(RPMTAG_FILESIZES, RI_32BIT, info->nrfile);
	ino[x].offset = htonl(data32bit(info->file[0].st.st_size));
	for (x = 1; x < info->nrfile; x++)
	    data32bit(info->file[x].st.st_size);

	x = newino(RPMTAG_FILEMODES, RI_16BIT, info->nrfile);
	ino[x].offset = htonl(data16bit(info->file[0].st.st_mode));
	for (x = 1; x < info->nrfile; x++)
	    data16bit(info->file[x].st.st_mode);

	x = newino(RPMTAG_FILEUIDS, RI_32BIT, info->nrfile);
	ino[x].offset = htonl(data32bit(info->file[0].st.st_uid));
	for (x = 1; x < info->nrfile; x++)
	    data32bit(info->file[x].st.st_uid);

	x = newino(RPMTAG_FILEGIDS, RI_32BIT, info->nrfile);
	ino[x].offset = htonl(data32bit(info->file[0].st.st_gid));
	for (x = 1; x < info->nrfile; x++)
	    data32bit(info->file[x].st.st_gid);
    }

    /* add prerequisites
     */
    if (info->nrneeds) {
	char *version;
	char *p;

	x = newino(RPMTAG_REQUIREVERSION, RI_STRINGARRAY, info->nrneeds);

	version = strchr(info->needs[0], ' ');
	ino[x].offset = htonl(datastring(version ? eatspace(version) : ""));
	for (x = 1; x < info->nrneeds; x++) {
	    version = strchr(info->needs[x], ' ');
	    datastring(version ? eatspace(version) : "");
	}

	x = newino(RPMTAG_REQUIRENAME, RI_STRINGARRAY, info->nrneeds);

	if ((p = strchr(info->needs[0], ' ')) != 0)
	    *p = 0;

	ino[x].offset = htonl(datastring(eatspace(info->needs[0])));
	for (x = 1; x < info->nrneeds; x++) {
	    if ((p = strchr(info->needs[x], ' ')) != 0)
		*p = 0;
	    datastring(eatspace(info->needs[x]));
	}
    }

    /* after everything else, write out the packager version.  I'll bet
     * that the reference implementation will fail on this.
     */
    addstring(RPMTAG_RPMVERSION, "makepkg $Revision: 1.14 $");

    /* write out the completed header
     */
    write(1, &hdr, sizeof hdr);
    sb.nritems = htonl(sb.nritems);
    sb.size = htonl(sb.size);
    write(1, &sb, sizeof sb);
    write(1, ino, ntohl(sb.nritems) * sizeof ino[0]);
    write(1, data, ntohl(sb.size));
    if (verbose)
	fprintf(stderr, "header: %d bytes\n",
			sizeof sb + (ntohl(sb.nritems) * sizeof ino[0]) + ntohl(sb.size));
} /* makeheader */


/*
 * writepackage() writes all the files (properly translated) into the archive
 */
void
writepackage(struct info *info)
{
    int x;		/* index */
    int status;		/* vcpio_wr() status; do we want to write this file? */
    int sz;		/* for file copy: number of bytes read */
    int fd;		/* for file copy: file descriptor */
    char blk[512];	/* for file copy: transfer blk */
    int io[2];
    pid_t child;

    if (pipe(io) != 0) {
	error("cannot set up to compress package");
	exit(1);
    }

    if ((child = fork()) == 0) {
	close(0);
	dup(io[0]);
	close(io[1]);
	execlp("gzip","gzip", 0);
	error("cannot execute gzip");
	exit(255);
    }
    else if (child < 0) {
	error("cannot start compression process");
	exit(1);
    }

    close(io[0]);
    for (x = 0; x < info->nrfile; x++) {
	status = vcpio_wr(io[1], &(info->file[x]) );
	if (status == 0) /* need to copy the file contents */ {
	    if ((fd = open(info->file[x].name, O_RDONLY)) != EOF) {

		if (verbose)
		    if (strcmp(info->file[x].name, info->file[x].dest) != 0)
			fprintf(stderr, "packaging %s as %s\n",
				info->file[x].name, info->file[x].dest);
		    else
			fprintf(stderr, "packaging %s\n",
				info->file[x].dest);

		while ((sz = read(fd, blk, sizeof blk)) > 0)
		    push(io[1], blk, sz);
		close(fd);
		align(io[1]);
	    }
	    else {
		error("cannot add %s to package", info->file[x].name);
		exit(1);
	    }
	}
	else if (status < 0)
	    exit(1);
    }
    cpio_endwr(io[1]);
    close(io[1]);
    wait(&status);
} /* writepackage */


/*
 * checkreq() checks to see if a given string is nonzero, and if
 * it isn't, adds the given description to the error string.
 */
void
checkreq(char** errstr, char* candidate, char* name)
{
    if (candidate == 0) {
	if (*errstr == 0)
	    *errstr = strdup(name);
	else {
	    *errstr = realloc(*errstr, strlen(*errstr) + strlen(name) + 3);
	    strcat(*errstr, ", ");
	    strcat(*errstr, name);
	}
    }
} /* checkreq */


/* 
 * commandline options
 */
#if HAVE_X_GETOPT

struct x_option options[] = {
    { 'v', 'v', "verbose", 0, "Be chattery while we're making the package" },
    { 'h', 'h', "help",    0, "Give this message" },
    { 'V', 'V', "version", 0, "Give the current version number, then exit" },
    { 'o', 'o', "show-os", 0, "Show the supported operating systems" },
    { 'a', 'a', "show-arch", 0, "Show the supported computer architectures" },
} ;
#  define NROPTIONS	(sizeof options / sizeof options[0])
#  define GETOPT(ac,av)	x_getopt(ac,av,NROPTIONS,options)
#  define OPTERR	x_opterr
#  define OPTIND	x_optind
#  define OPTARG	x_optarg
#else
#  define GETOPT(ac,av)	getopt(ac,av,"vhVoa")
#  define OPTERR	opterr
#  define OPTIND	optind
#  define OPTARG	optarg
#endif


/*
 * showmaps() shows the contents of a map
 */
void
showmaps(struct mapping *map, int nrmap, char* desc)
{
    int ix, val = EOF;

    printf("\n%s\n", desc);

    for (ix = 0; ix < nrmap; ix++) {
	if (val != map[ix].number)
	    if (map[ix].desc)
		printf("\n%s:", map[ix].desc);
	    else
		putchar('\n');
	printf(" %s", map[ix].name);
	val = map[ix].number;
    }
    putchar('\n');
}

/*
 * makepkg, in mortal flesh
 */
void /*smooooch!*/
main(int argc, char **argv)
{
    struct info info;
    char *p;
    int x;
    int needtomove = 0;
    int showarch=0;
    int showos=0;
    int sectionid;
    char *missing = 0;
    int opt;

    OPTERR = 1;
    while  ((opt = GETOPT(argc, argv)) != EOF) {
	switch (opt) {
	case 'v':
		verbose++;
		break;

	case 'V':
		puts("makepkg $Revision: 1.14 $");
		exit(0);

	case 'o':
		showmaps(osmap, nrosmap, "Supported operating systems");
		exit (0);

	case 'a':
		showmaps(archmap, nrarchmap, "Supported architectures");
		exit (0);

	default:
#if HAVE_X_GETOPT
		fprintf(stderr, "usage: makepkg [options] [command-file]\n\n");
		showopts(stderr, NROPTIONS, options);
#else
		fprintf(stderr, "usage: makepkg [-vVoah] [command-file]\n");
#endif
		exit ( opt == 'h' ? 0 : 1);
	}
    }

    if ((argc > OPTIND) && freopen(file = argv[OPTIND], "r", stdin) == 0) {
	error("%s", strerror(errno));
	exit(1);
    }

    /* initialize what we need to initialize
     */
    uname(&SystemInfo);

    memset(&info, 0, sizeof info);

    /* eat up lines until we find our first section header
     */
    while ((p=getsectionline()) != 0)
	;

    /* masticate all the sections
     */
    while ((p = getsectionheader()) != 0) {
	sectionid = EOF;
	for (x = 0; x < NRSECTIONS; x++)
	    if (strncasecmp(p, sections[x].name, strlen(sections[x].name)) == 0) {
		sectionid = sections[x].section;
		break;
	    }

	switch (sectionid) {
	case PACKAGE:	package_section(&info);
			break;
	case DESCRIPTION:
			info.description = getstring();
			break;
	case NEEDS:	needs_section(&info);
			break;
	case PREINSTALL:info.preinstall = getstring();
			break;
	case INSTALL:	info.install = getstring();
			break;
	case UNINSTALL:	info.uninstall = getstring();
			break;
	case FILES:	file_section(&info);
			break;
	}
    }
    lineno = 0;

    if (info.nrfile < 1)
	error("package contains no files");

    checkreq(&missing, info.name, "name");
    checkreq(&missing, info.version, "version");
    checkreq(&missing, info.author, "author/vendor");
    checkreq(&missing, info.summary, "summary");
    checkreq(&missing, info.copyright, "copyright");
    checkreq(&missing, info.distribution, "distribution");

    if (missing)
	error("the [package] section needs the following headers: %s", missing);

    if (errors)
	exit(1);


    /* fill in architecture/os defaults
     */
    if (info.os == 0)
	info.os = strdup(SystemInfo.sysname);
    if (info.arch == 0)
	info.arch= strdup(SystemInfo.machine);

    /* populate destination file names */
    for (x = 0; x < info.nrfile; x++) {
	needtomove |= info.file[x].tobemoved;
	if (info.file[x].dest == 0)
	    if (info.prefix) {
		info.file[x].dest = malloc(strlen(info.prefix) + strlen(info.file[x].name) + 2);
		sprintf(info.file[x].dest, "%s/%s", info.prefix, info.file[x].name);
	    }
	    else
		info.file[x].dest = info.file[x].name;
    }

    makeheader(&info);
    writepackage(&info);

    if (verbose) {
	fprintf(stderr, "%s, version %s, for %s\n", info.name, info.version, info.distribution);
	if (info.release)
	    fprintf(stderr, "\trelease   %s\n", info.release);
	fprintf(stderr, "\tauthor    %s\n", info.author);
	fprintf(stderr, "\tsummary   %s\n", info.summary);
	fprintf(stderr, "\turl       %s\n", info.url);
	fprintf(stderr, "\tos        %s\n", info.os);
	fprintf(stderr, "\tarch      %s\n", info.arch);
	fprintf(stderr, "\tcopyright %s\n", info.copyright);

	if (info.description)
	    fprintf(stderr, "description = [%s]\n", info.description);
	if (info.preinstall)
	    fprintf(stderr, "preinstall  = [%s]\n", info.preinstall);
	if (info.install)
	    fprintf(stderr, "install     = [%s]\n", info.install);
	if (info.uninstall)
	    fprintf(stderr, "uninstall   = [%s]\n", info.uninstall);

	fprintf(stderr, "files       = [\n");
	for (x=0; x < info.nrfile; x++) {
	    fprintf(stderr, "       %s, uid=%u, gid=%u, mode=%u, size=%lu, mtime=%d\n",
		    info.file[x].dest,
		    info.file[x].st.st_uid, info.file[x].st.st_gid,
		    info.file[x].st.st_mode, info.file[x].st.st_size,
		    info.file[x].st.st_mtime);
	}
	fprintf(stderr, "              ]\n");
    }
    exit(0);
} /* makepkg */
