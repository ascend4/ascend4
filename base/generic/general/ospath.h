/*	ASCEND modelling environment
	Copyright (C) 2006 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*//**
	@file
	A C-language class for simple file-path manipulations alla python os.path.
	Attempts to handle windows 'drive' prefixes (eg "C:") and automatic
	substitution of forward slashes with appropriate platform-specific
	path separators.

	Has the goal of providing path-search functionality to ASCEND for
	modules (.a4c) and external library (.so/.dll) files, etc.

	Heavily modified version of C++ code from codeproject.com
	originally written by Simon Parkinson-Bates.

	@TODO add environment variable expansion here for use in case like
	"$ASCENDTK/bitmaps" or "ascend-$VERSION$EXESUFFIX"

	@NOTE this library makes no allowance for fancy escape characters.
	You better not try to put escaped slashes into your paths, and
	make sure you've unescaped everything before you send it to
	ospath.
*//*
	by John Pye, May 2006
*/

#ifndef OSPATH_H
#define OSPATH_H

#ifdef TEST
# define ASC_DLLSPEC(T) T
# include "env.h"
#else
# include <utilities/config.h>
# include <utilities/ascConfig.h>
# include <utilities/ascMalloc.h>
# include "env.h"
#endif

#if defined(__WIN32__) && !defined(__MINGW32__)
# include <direct.h>
# include <stdlib.h>
#else
# ifdef __MINGW32__
#  include <io.h>
#  include <limits.h>
#  include <sys/stat.h>
# else
#  include <sys/stat.h>
# endif
#endif

#ifndef PATH_MAX
# define PATH_MAX 1023
#endif

struct FilePath;

/**
	Create a new ospath object from a string. This will
	normalise the path and attempt to add a drive-prefix
	if one is missing.
*/
ASC_DLLSPEC(struct FilePath *) ospath_new(const char *path);

ASC_DLLSPEC(struct FilePath *) ospath_new_noclean(const char *path);

/**
	Free an ospath
*/
ASC_DLLSPEC(void) ospath_free(struct FilePath *);

/**
	Free a string allocated from ospath.
*/
ASC_DLLSPEC(void) ospath_free_str(char *str);

/**
	Create a new ospath object from a string, assuming
	that standard forward-slash paths are used.
*/
ASC_DLLSPEC(struct FilePath *) ospath_new_from_posix(const char *posixpath);

/**
	Path with *VERY SIMPLE* environment variable expansion.
	You must specify what 'getenv' function should be used.
*/
ASC_DLLSPEC(struct FilePath *) ospath_new_expand_env(const char *path, GetEnvFn *getenvptr);

/**
	This function cleans up the path string used to construct the FilePath object:
	1. Get rid of multiple / 's one after the other...

	   ie. "///usr/bin///hello/////there// --> "/usr/bin/hello/there/"

	2. Resolve a leading tilde (~) to the current user's HOME path

	3. Remove redundant /./ in middle of path

	4. Remove reduncant dir/.. in path

	5. Environment substitution??

	6. On windows, drive reference if not specified

	7. What about \\server\path and URLs, gnomefs, etc?
*/
ASC_DLLSPEC(void) ospath_cleanup(struct FilePath *);

/**
	Check that the created FilePath was valid (i.e. able
	to be parsed. Doesn't check that the directory/file
	actually exists.)
*/
int ospath_isvalid(struct FilePath *fp);

/**
	Return the FilePath in the form of a string.
	You must FREE the allocated string when you don't need it any more.
*/
ASC_DLLSPEC(char *) ospath_str(struct FilePath *fp);

/**
	Return the FilePath in the string location given.
	If the user has allocated a local array before calling ospath_strcpy,
	then this allows the FREE(dest) call to be avoided.

	@param dest the location of allocated storage space where the FilePath
		will be written.
	@param destsize the amount of allocated string space at dest.
*/
ASC_DLLSPEC(void) ospath_strcpy(struct FilePath *fp,char *dest, int destsize);

/**
	Append string from FilePath to the already-allocated string 'dest',
	while ensuring that the 'dest' strings *total* size doesn't exceed
	destsize.
*/
ASC_DLLSPEC(void) ospath_strcat(struct FilePath *fp,char *dest, int destsize);

/**
	Output the FilePath to a file
*/
void ospath_fwrite(struct FilePath *fp, FILE *dest);

/**
	Write out the internal structure of the FilePath object
	to stderr.
*/
ASC_DLLSPEC(void) ospath_debug(struct FilePath *fp);

/**
	Return length of path string
*/
unsigned int ospath_length(struct FilePath *fp);

/**
	Create a new path that is the direct parent of this one.
*/
ASC_DLLSPEC(struct FilePath *) ospath_getparent(struct FilePath *fp);

/**
	Return a new path object which is a parent path of the current path up to the specified depth.
	If the specifed depth is >= to the current path's depth, then a copy of the current path object is returned.

	ie.
		FilePath("/lev1/lev2/lev3/lev4/lev5").GetParentAtDepthN(3).c_str()
	returns
		"/lev1/lev2/lev3/"
*/
struct FilePath *ospath_getparentatdepthn(struct FilePath *fp, unsigned nDepth);

/**
	Return then name of the bottom most level path entry (includes any extension)
*/
ASC_DLLSPEC(char *) ospath_getbasefilename(struct FilePath *fp);

/**
	Retrieve the path's bottom level filename without the extension.
	A path that ends in a slash will be assumed to be directory, so the
	file stem will be NULL (ie not a file)
*/
ASC_DLLSPEC(char *) ospath_getfilestem(struct FilePath *fp);

/**
	retrieve the paths extension (if it has one) does not include the dot
*/
ASC_DLLSPEC(char *) ospath_getfileext(struct FilePath *fp);

/**
	Return a new FilePath containing the directory component of the path
	(everything up the the last slash).
*/
ASC_DLLSPEC(struct FilePath *) ospath_getdir(struct FilePath *fp);

/**
	Function returns true if the current path is the root directory, otherwise it returns false.
*/
int ospath_isroot(struct FilePath *fp);

/**
	Return the current paths' depth
	ie. "/usr/some directory with spaces in it/hello"
	returns a depth value of 3
*/
unsigned ospath_depth(struct FilePath *fp);

/**
	Return the root path
*/
ASC_DLLSPEC(struct FilePath *) ospath_root(struct FilePath *fp);

ASC_DLLSPEC(int) ospath_cmp(struct FilePath *fp1, struct FilePath *fp2);

ASC_DLLSPEC(struct FilePath *) ospath_concat(struct FilePath *fp1, struct FilePath *fp2);

ASC_DLLSPEC(void) ospath_append(struct FilePath *fp, struct FilePath *fp1);

/**
	File-open function. Simply a wrapper about the 'fopen' call, except
	that if the FilePath is not 'valid' it will return NULL without
	attempting to open.
*/
ASC_DLLSPEC(FILE *) ospath_fopen(struct FilePath *fp, const char *mode);

/**
	Stat function. Simply a wrappen around the 'stat' call.
	The exception is that if the FilePath is not 'valid', -1 is returned
	and no call to 'stat' is made.
*/
ASC_DLLSPEC(int) ospath_stat(struct FilePath *fp,struct stat *buf);

//------------------------
// SEARCH PATH FUNCTIONS

ASC_DLLSPEC(struct FilePath **) ospath_searchpath_new(const char *path);

ASC_DLLSPEC(void) ospath_searchpath_free(struct FilePath **searchpath);

typedef int (FilePathTestFn)(struct FilePath *,void *);

ASC_DLLSPEC(int) ospath_searchpath_length(struct FilePath **searchpath);

/**
	@return pointer to path component in which testfn passed, or else
	NULL if no component passed the test. There is no need to free the returned
	path component, as it will be freed when you (later) free the searchpath
	param.
*/
ASC_DLLSPEC(struct FilePath *) ospath_searchpath_iterate(
		struct FilePath **searchpath
		, FilePathTestFn *testfn
		, void *searchdata
);

#endif
