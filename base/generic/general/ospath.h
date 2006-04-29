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
	modules (a4c) and external library (.dll) files, etc.

	Heavily modified version of C++ code from codeproject.com
	originally written by Simon Parkinson-Bates.
*/

#ifndef OSPATH_H
#define OSPATH_H

#ifndef PATH_MAX
# define PATH_MAX 1023
#endif

struct FilePath;

/**
	Create a new ospath object from a string. This will
	normalise the path and attempt to add a drive-prefix
	if one is missing.
*/
struct FilePath *ospath_new(const char *path);

/**
	Free an ospath
*/
void ospath_free(struct FilePath *);

/**
	Create a new ospath object from a string, assuming
	that standard forward-slash paths are used.
*/
struct FilePath *ospath_new_from_posix(const char *posixpath);

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
char *ospath_str(struct FilePath *fp);

/**
	Return the FilePath in the string location given.
	If the user has allocated a local array before calling ospath_strcpy,
	then this allows the FREE(dest) call to be avoided.

	@param dest the location of allocated storage space where the FilePath
		will be written.
	@param destsize the amount of allocated string space at dest.
*/
void ospath_strcpy(struct FilePath *fp,char *dest, int destsize);

/**
	Output the FilePath to a file
*/
void ospath_fwrite(struct FilePath *fp, FILE *dest);

/**
	Write out the internal structure of the FilePath object
	to stderr.
*/
void ospath_debug(struct FilePath *fp);

/**
	Return length of path string
*/
unsigned int ospath_length(struct FilePath *fp);

/**
	Create a new path that is the direct parent of this one.
*/
struct FilePath *ospath_getparent(struct FilePath *fp);

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
char *ospath_getbasefilename(struct FilePath *fp);

/**
	retrieve the path's bottom level filename without the extension.
	A path that ends in a slash will be assumed to be directory, so the
	file stem will be NULL (ie not a file)
*/
char *ospath_getfilestem(struct FilePath *fp);

/**
	retrieve the paths extension (if it has one) does not include the dot
*/
char *ospath_getfileext(struct FilePath *fp);

/**
	retrieve the directory component of the path (everything up the the
	last slash
*/
struct FilePath *ospath_getdir(struct FilePath *fp);

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
struct FilePath *ospath_root(struct FilePath *fp);

int ospath_cmp(struct FilePath *fp1, struct FilePath *fp2);

struct FilePath *ospath_concat(struct FilePath *fp1, struct FilePath *fp2);

void ospath_append(struct FilePath *fp, struct FilePath *fp1);

/**
	File-open function. Simply a wrapper about the 'fopen' call, except
	that if the FilePath is not 'valid' it will return NULL without
	attempting to open.
*/
FILE *ospath_fopen(struct FilePath *fp, const char *mode);

//------------------------
// SEARCH PATH FUNCTIONS

struct FilePath **ospath_searchpath_new(const char *path);

typedef int (FilePathTestFn)(struct FilePath *,void *);

struct FilePath *ospath_searchpath_iterate(
		struct FilePath **searchpath
		, FilePathTestFn *testfn
		, void *searchdata
);

#endif
