/*
 *   Copyright (c) 1998,2017 David Parsons. All rights reserved.
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
 * xrpm: extract things from Redhat's proprietary packaging system.
 */
#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_MALLOC_H
#   include <malloc.h>
#endif

#include <basis/options.h>

#include "xrpm.h"
#include "mapping.h"

/* external tables that contain human-readable names for the
 * two types of rpm tags (signature vs. header tags)
 */
extern char *rpmtagnames[];
extern int nrrpmtagnames;
extern char *rpmsignames[];
extern int nrrpmsignames;

extern struct mapping osmap[];
extern int          nrosmap;
extern struct mapping archmap[];
extern int          nrarchmap;

/* PACKAGE TYPES
 */
char* pkgtypes[] = {
    "binary",
    "source",
};
#define NRTYPES	(sizeof pkgtypes/sizeof pkgtypes[0])


/* SIGNATURE TYPES
 */
char* pkgsig[] = {
    "None",
    "PGP",
    "Bad",
    "MD5",
    "MD5+PGP",
    "Header-style",
};
#define NRSIGS	(sizeof pkgsig/sizeof pkgsig[0])

/* Flags
 */
int quieter = 0;
char *this_file = "This file";


/*
 * describe_header() describes an rpm header
 */
void
describe_header(struct rpm_header* hdr)
{
    int os, arch;
    int x;

    if (strlen(hdr->name) > 0)
	printf("(%s)", hdr->name);
    else
	printf("This");
    printf(" is a rpm v%d.%d file\n", hdr->major, hdr->minor);
    if (ntohs(hdr->type) < NRTYPES)
	printf("It is a %s package", pkgtypes[ntohs(hdr->type)]);
    else
	printf("It is package type=%u", ntohs(hdr->type));
    arch = ntohs(hdr->archnum);
    os = ntohs(hdr->osnum);

    for (x = 0; x < nrosmap; x++)
	if (os == osmap[x].number) {
	    printf(" for %s", osmap[x].name);
	    break;
	}
    if (x >= nrosmap)
	printf(" for os=%u", os);

    for (x = 0; x < nrarchmap; x++)
	if (arch == archmap[x].number) {
	    printf("/%s\n", archmap[x].name);
	    break;
	}
    if (x >= nrarchmap)
	printf("/arch=%u\n", arch);

    if (ntohs(hdr->signature_type) < NRSIGS && pkgsig[ntohs(hdr->signature_type)])
	printf("Signature type: %s\n", pkgsig[ntohs(hdr->signature_type)]);
    else
	printf("Unknown signature type %u\n", ntohs(hdr->signature_type));

} /* describe_header */


/*
 * free_info_header() cleans up malloc()ed memory block in and rpm_info_header
 */
void
free_info_header(struct rpm_info_header *p)
{
    if ( p ) {
	if ( p->ino ) {
	    free( p->ino );
	    p->ino = 0;
	}
	if ( p->data ) {
	    free( p->data );
	    p->data = 0;
	}
    }
} /* free_info_header */


/*
 * dumpheaderblock() cats a header block to stdout
 */
void
dumpheaderblock(struct rpm_header *pkg, int fd, struct rpm_info_header *sb)
{
    size_t start = tell(fd);
    BYTE magic[8];
    struct rpm_super superblock;
    int nritems, size, sz;
    struct rpm_info *ino;
    BYTE *data;
    
    if ( pkg->major >= 3 ) {
	sz = read(fd, magic, 8);
	if ( sz == 8 )
	    write(1, magic, 8);
	else {
	    perror("magic");
	    exit(2);
	}
    }
    
    if ((sz = read(fd, &superblock, sizeof superblock)) != sizeof superblock) {
	perror("headerheader");
	exit(2);
    }
    write(1, &superblock, sizeof superblock);
    nritems = ntohl(superblock.nritems);
    size    = ntohl(superblock.size);
    
    sz = nritems * sizeof ino[0];
    if ( (ino = malloc(sz)) == 0 || read(fd, ino, sz) != sz ) {
	perror("read header directory");
	exit(2);
    }
    write(1, ino, size);
    
    if ( (data = malloc(size)) == 0 || read(fd, data, size) != size ) {
	perror("read header data");
	exit(2);
    }
    write(1, data, sz);
    free(ino);
    free(data);
    lseek(fd, start, SEEK_SET);
} /* dumpheaderblock */
 
 
/*
 * readheaderblock() reads a header segment
 */
void
readheaderblock(struct rpm_header* pkg, int fd, struct rpm_info_header *sb)
{
    struct rpm_super superblock;
    int ct, i, sz;
    BYTE magic[8];
    static BYTE hmagic[4] = { 0x8E, 0xAD, 0xE8, 0x01 };

    if (pkg->major >= 3) {
	sz = read(fd, magic, 8);
	if (sz != 8) {
	    perror("magic");
	    exit(2);
	}
	if (memcmp(magic, hmagic, 3) != 0) {
	    fprintf(stderr, "header magic failed! (%02x %02x %02x)\n",
	    magic[0], magic[1], magic[2]);
	    exit(1);
	}
    }

    /* read the superblock, convert fields into native byte order */
    if ((sz = read(fd, &superblock, sizeof superblock)) != sizeof superblock) {
	perror("headerheader");
	exit(2);
    }
    sb->super.nritems = ntohl(superblock.nritems);
    sb->super.size    = ntohl(superblock.size);
    
    sz = sb->super.nritems * sizeof sb->ino[0];

    /* read the ino array, convert fields into native byte order */
    sb->ino = malloc(sz);
    if (sb->ino == 0) {
	perror("allocate header directory");
	exit(2);
    }
    if (read(fd, sb->ino, sz) != sz) {
	perror("read header directory");
	exit(2);
    }
    for (i=0; i < sb->super.nritems; i++) {
	sb->ino[i].tag = ntohl(sb->ino[i].tag);
	sb->ino[i].type = ntohl(sb->ino[i].type);
	sb->ino[i].count = ntohl(sb->ino[i].count);
	sb->ino[i].offset = ntohl(sb->ino[i].offset);
    }


    /* read the data block */
    sb->data = malloc(sb->super.size);
    if (sb->data == 0) {
	perror("allocate header data");
	exit(2);
    }
    if (read(fd, sb->data, sb->super.size) != sb->super.size) {
	perror("read header data");
	exit(2);
    }
} /* readheaderblock */


static char *datatypes[] = {
	"Null",
	"Char",
	"8bit",
	"16bit",
	"32bit",
	"64bit",
	"String",
	"Binary",
	"Stringarray",
	"Internationalized string",
};
#define NRDATATYPES	(sizeof datatypes/sizeof datatypes[0])


/*
 * display a single field out of a rpm header block
 */
static void
display_field(struct rpm_info *inode, BYTE *data, char *iname, int inum)
{
    int i;

    if (!quieter) {
	if (iname)
	    printf("[%d] = %s (%d)", inum, iname, inode->tag);
	else
	    printf("[%d] = %d", inum, inode->tag);

	if (inode->type < NRDATATYPES)
	    printf(" %s\n", datatypes[inode->type]);
	else
	    printf(" datatype %d\n", inode->type);
    }

    data += inode->offset;

    for (i = 0; i < inode->count; i++) {
	switch (inode->type) {
	case RI_NULL:
		    break;
	case RI_CHAR:
		    printf("%d: %c\n", i, *data++);
		    break;
	case RI_8BIT:
		    printf("%d: %u\n", i, *data++);
		    break;
	case RI_16BIT:
		    printf("%d: %u\n", i, ntohs(*((WORD*)data)));
		    data += 2;
		    break;
	case RI_32BIT:
		    printf("%d: %u\n", i, ntohl(*((DWORD*)data)));
		    data += 4;
		    break;
	case RI_64BIT:
		    printf("%d: %u %u\n", i, ntohl(*((DWORD*)data)), ntohl(*((DWORD*)(data+4))));
		    data += 8;
		    break;
	case RI_STRING:
	case RI_STRING_INTL:
		    printf("%d: %s\n", i, data);
		    i = inode->count;
		    break;
	case RI_BINARY:
		    printf("%02x", *data++);
		    if (i == inode->count - 1)
			putchar('\n');
		    break;
	case RI_STRINGARRAY:
		    printf("%d: %s\n", i, data);
		    while (*data) ++data;
		    ++data;
		    break;
	}
    }
} /* display_field */


/*
 * return the name of the given field
 */
static char *
fieldname(int tag, char **names, int nrnames)
{
    if (tag >= 1000 && tag < 1000+nrnames)
	return names[tag-1000];
    else
	return 0;
}


/*
 * display the contents of a header block (either signature or db -- we
 * pass in names[] and nrnames to point at the appropriate names for each
 * block)
 */
displayheaderblock(struct rpm_info_header *sb,
                   char *names[], int nrnames,
		   char *field_to_display)
{
    char *nm = 0;
    int thisino, i, j;

    if (field_to_display == 0) {
	printf("header\n"
	       "------\n"
	       "nritems:   %d\n"
	       "data size: %u\n", sb->super.nritems, sb->super.size);
	puts("----");
    }

    if (field_to_display) {
	for (i=0; i < nrnames; i++)
	    if (strcasecmp(names[i], field_to_display) == 0)
		break;
	if (i < nrnames)
	    i += 1000;
	else
	    i = atoi(field_to_display);

	for (j=0; j < sb->super.nritems; j++)
	    if (sb->ino[j].tag == i)
		break;

	if (j >= sb->super.nritems) {
	    fprintf(stderr, "field ``%s'' not found\n", field_to_display);
	    exit(1);
	}

	nm = fieldname(i, names, nrnames);
	display_field(&sb->ino[j], sb->data, nm, j);

    }
    else for (j=0; j < sb->super.nritems; j++) {

	nm = fieldname(sb->ino[j].tag, names, nrnames);
	display_field(&sb->ino[j], sb->data, nm, j);
	puts("----");

    }
} /* displayheaderblock */


char *
get_string_field(int tag, struct rpm_info_header *sb)
{
    int i;

    for (i=0; i < sb->super.nritems; i++) {
	if (sb->ino[i].tag == tag) {
	    /*display_field(&sb->ino[i], sb->data, 0, 0);*/

	    if (sb->ino[i].type != RI_STRING &&
		sb->ino[i].type != RI_STRING_INTL) {
		/*fprintf(stderr, "no\n");*/

		return 0;
	    }

	    /*fprintf(stderr, "here it is [%s]\n", sb->data + sb->ino[i].offset);*/

	    return sb->data + sb->ino[i].offset;
	}
    }
    return 0;
}


/*
 * options to the program
 */
struct x_option options[] = {
    { 'a', 'a', "all",		0,	"Display all package information" },
    { 'b', 'b', "buildroot",   "DIR",   "extract to DIR" },
    { 'i', 'i', "package-header",0,	"Display the package header" },
    { 's', 's', "signature", 	0,	"Display the package signature" },
    { 'p', 'p', "package-info",	0,	"Display the package info database" },
    { 'R', 'R', "field-info", "FIELD",	"Display a specific item from the\n"
					"package info database (either name\n"
					"or numeric index)\n" },
    { 'q', 'q', "quieter",      0,      "Be a bit less chattery" },
    { 'l', 'l', "list-fields",  0,      "List all the database fields that\n"
					"xrpm knows by name" },
    { 't', 't', "list-contents",0,	"Display the contents of the package" },
    { 'd', 'd', "dump", 	0,	"Dump the contents of the package to\n"
					"stdout. The contents of rpm packages\n"
					"are compressed cpio files, so you'll\n"
					"need to decompress the file before\n"
					"you can do anything with it." },
    { 'D', 'D', "dump-header",  0,	"dump the payload header block" },
    { 'x',	'x', "extract",	0,	"Extract the contents of the archive" },
    { 'h',	'h', "help",	0,	"Show the valid options for xrpm" },
    { 'V',	'V', "version",	0,	"Show the program version, then quit" },
};
#  define NROPT	(sizeof options/sizeof options[0])

#  define GETOPT(ac,av)	x_getopt(ac,av, NROPT, options)
#  define OPTARG	x_optarg
#  define OPTIND	x_optind
#  define OPTERR	x_opterr


/*
 * xrpm, in mortal flesh
 */
main(int argc, char ** argv)
{
    struct rpm_header hdr;
    struct rpm_info_header signature;
    struct rpm_info_header db;
    int sz;
    unsigned char pgpsig[259];
    long pos;
    int opt;
    char *p, *decompress, *cpio_cmd;
    char *build_root = 0;

    int show_header = 0;
    int show_signature = 0;
    int show_database = 0;
    int list_archive = 0;
    int dump_archive = 0;
    int dump_header = 0;
    int extract_archive = 0;

    char* field_to_display = (char*)0;

    memset(&signature, 0, sizeof signature);
    memset(&db, 0, sizeof db);

    OPTERR = 1;
    while ((opt = GETOPT(argc, argv)) != EOF) {
	switch (opt) {
	case 'i':   show_header = 1;
		    break;
	case 's':   show_signature = 1;
		    break;
	case 'p':   show_database = 1;
		    field_to_display = (char*)0;
		    break;
	case 'a':   show_header = 1;
		    show_signature = 1;
		    show_database = 1;
		    break;
	case 'R':   show_database = 1;
		    field_to_display = OPTARG;
		    break;
	case 't':   list_archive = 1;
		    break;
	case 'd':   dump_archive = 1;
		    break;
	case 'D':   dump_header = 1;
		    break;
	case 'b':   build_root = OPTARG;
		    break;
	case 'l':   {   int j;
			for (j=0; j < nrrpmtagnames; j++)
			    puts(rpmtagnames[j]);
			exit(0);
		    }
	case 'q':   quieter = 1;
		    break;
	case 'x':   extract_archive = 1;
		    break;
	case 'V':   puts(xrpm_version);
		    exit(0);
	default:
	case 'h':
		    fprintf(stderr, "usage: xrpm [options] [package]\n\n");
		    showopts(stderr, NROPT, options);
		    exit( opt == 'h' ? 0 : 1);
	}
    }

    if ((argc > OPTIND) && freopen(this_file = argv[OPTIND], "r", stdin) == 0) {
	perror(argv[OPTIND]);
	exit(2);
    }
    else if (isatty(0)) {
	fprintf(stderr, "Please specify a package to examine.\n");
	exit(2);
    }


    if ( (sz=read(0, &hdr, sizeof hdr)) < sizeof hdr) {
	fprintf(stderr, "%s is too short to be a rpm file.\n", this_file);
	exit(1);
    }

    if (hdr.magic != MAGIC) {
	printf("%s is not a rpm file (magic %x vs %x)\n", hdr.magic, MAGIC, this_file);
	exit(1);
    }

    if (hdr.major < MIN_MAJOR) {
	printf("%s is too old to process (rpm v%d -- we need at least rpm v%d)\n",
		this_file, MIN_MAJOR, hdr.major);
	exit(1);
    }
    switch (ntohs(hdr.signature_type)) {
    case 0: break;
    case 1:	/* PGP */
	    sz = read(0, pgpsig, 256);
	    if (sz != 256) {
		perror("read");
		exit(1);
	    }
	    break;
    case 2:	/* BAD */
    case 3:	/* MD5 */
    case 4:	/* MD5+PGP */
	    fprintf(stderr, "I can't handle %s signatures\n", pkgsig[ntohs(hdr.signature_type)]);
	    exit(2);

    case 5:	/* HEADER */
	    readheaderblock(&hdr, 0, &signature);

	    /* type 5 header blocks are apparently padded out to multiples of
	     * eight bytes.
	     */
	    pos = tell(0);
	    if ((pos % 8) != 0) {
		char filler[8];
		read(0, filler, 8-(pos % 8));
	    }

	    break;
    default:
	    fprintf(stderr, "I don't know about signature type %d\n", ntohs(hdr.signature_type));
	    exit(2);
    }

    if ( dump_header )
	dumpheaderblock(&hdr, 0, &db);

    readheaderblock(&hdr, 0, &db);

    if (show_header)
	describe_header(&hdr);

    if (show_signature) {
	if (ntohs(hdr.signature_type) == 1 /* PGP */) {
	    int i;
	    printf("PGP signature=[");
	    for (i=0; i < 256; i++)
		printf("%02x", pgpsig[i]);
	    printf("]\n");
	}
	else if (ntohs(hdr.signature_type) == 5 /* header-style */)
	    displayheaderblock(&signature, rpmsignames, nrrpmsignames, 0);
    }

    if (show_database)
	displayheaderblock(&db, rpmtagnames, nrrpmtagnames, field_to_display);

    /* pick out the program (field 1125) that was used to compress the payload
     */
    if ( (p = get_string_field(1125, &db)) == 0 || *p == 0)
	decompress = "gzip -d";
    else if ( decompress = malloc(strlen(p) + 4 /*strlen("-d\0")*/ ) )
	sprintf(decompress, "%s -d", p);

    /* allocate memory for the uncompress + cpio command */
    cpio_cmd = malloc(strlen(decompress) + 4 + 12 /*strlen("|cpio -ivdmu")*/ ); 

    if (decompress == 0 || cpio_cmd == 0) {
	perror("allocate working storage");
	exit(2);
    }

    /* and what follows should be a compressed cpio file */

    if (dump_archive) {
	char block[512];
	int sz;

	while ((sz = read(0, block, sizeof block)) > 0)
	    write(1, block, sz);
    }
    else if (list_archive) {
	sprintf(cpio_cmd, "%s | cpio -i%st", decompress, quieter ? "" : "v");
	system(cpio_cmd);
    }
    else if (extract_archive) {
	if ( build_root && chdir(build_root) != 0 ) {
	    perror(build_root);
	    exit(2);
	}
	sprintf(cpio_cmd, "%s | cpio -i%sdmu", decompress, quieter ? "" : "v");
	system(cpio_cmd);
    }
    free_info_header(&db);
    free_info_header(&signature);
    free(cpio_cmd);
    exit(0);
} /* xrpm */
