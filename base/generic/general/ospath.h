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
	Operating system file-path manipulations, alla python os.path

	Based on C++ code from codeproject.com written by Simon Parkinson-Bates
*/

#ifndef OSPATH_H
#define OSPATH_H

#include <string.h>
#include <malloc.h>

#define PATHMAX 1024

struct FilePath;

struct FilePath *ospath_new(const char *path);

struct FilePath *ospath_new_from_posix(char *posixpath);

/**
	Is the path valid? Returns true if so, false otherwise
*/
int ospath_isvalid(struct FilePath *fp);

/**
	Cast to string (char *)
	(you must free the string when you don't need it any more)
*/
char *ospath_str(struct FilePath *fp);

/**
	Output string-cast to FILE
*/
void ospath_fwrite(struct FilePath *fp, FILE *dest);

void ospath_debug(struct FilePath *fp, char *label);

/**
	Return length of path string
*/
unsigned int ospath_length(struct FilePath *fp);

/**
	Create a new path that is the direct parent of this one
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
	retrieve the path's bottom level filename with out the extension
*/
char *ospath_getbasefiletitle(struct FilePath *fp);

/**
	retrieve the paths extension (if it has one) does not include the dot
*/
char *ospath_getbasefileextension(struct FilePath *fp);

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


#endif
