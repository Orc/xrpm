/*
 * special meaning for tags in the rpm file's header section.
 * This is extracted from the code (grr), because redhat didn't
 * want to publish this interface.
 */
char *rpmtagnames[]	= {
	"Name",
	"Version",
	"Release",
	"Serial",
	"Summary",
	"Description",
	"Buildtime",
	"Buildhost",
	"Installtime",
	"Size",
	"Distribution",
	"Vendor",
	"GIF",
	"xpm",
	"Copyright",
	"Packager",
	"Group",
	"Changelog",
	"Source",
	"Patch",
	"URL",
	"OS",
	"Arch",
	"Prein",
	"Postin",
	"Preun",
	"Postun",
	"Filenames",
	"Filesizes",
	"Filestates",
	"Filemodes",
	"FileUIDs",
	"FileGIDs",
	"Filerdevs",
	"Filemtimes",
	"FileMD5s",
	"Filelinktos",
	"Fileflags",
	"Root",
	"Fileusername",
	"Filegroupname",
	"Exclude",
	"Exclusive",
	"Icon",
	"Sourcerpm",
	"Fileverifyflags",
	"Archivesize",
	"Provides",
	"Requireflags",
	"Requirename",
	"Requireversion",
	"Nosource",
	"Nopatch",
	"Conflictflags",
	"Conflictname",
	"Conflictversion",
	"Defaultprefix",
	"Buildroot",
	"Installprefix",
	"Excludearch",
	"Excludeos",
	"Exclusivearch",
	"Exclusiveos",
	"Autoreqprov",
	"Rpmversion",
} ;

int nrrpmtagnames = (sizeof rpmtagnames / sizeof rpmtagnames[0]);
