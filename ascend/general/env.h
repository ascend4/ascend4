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
	Environment variable helper routines
*//*
	by John Pye, May 2006.
*/

#ifndef ASC_ENV_H
#define ASC_ENV_H

#ifdef TEST
# define ASC_DLLSPEC T T
#else
# include <utilities/config.h>
# include <utilities/ascConfig.h>
# include <utilities/ascMalloc.h>
# define FREE ascfree
# define MALLOC ascmalloc
#endif

/**
	This is the type of env var function that you must send to 'env_subst'.
	It doesn't have to actually consult the environment; it could do all
	sorts of other stuff if you wanted.
*/
typedef char *(GetEnvFn)(const char *name);

/**
	This the type of a putenv function that can be used to set and environment
	variable.
*/
typedef int (PutEnvFn)(const char *inputstring);

/**
	Attempts to read from a getenv function, and if the value is found, write it
	using a putenv function. You would use this to copy values from one
	environment to another.

	@return nonzero on error, -1 means that the value didn't exist in getenv,
	otherwise the errors are those returned by putenv.
*/
ASC_DLLSPEC int env_import(const char *varname,GetEnvFn *getenvptr,PutEnvFn *putenvptr);

/**
	Perform variable substitution on a string in shell-like way.
	This should replace any $VARNAME with the result of
	(*getenvptr)(VARNAME).

	At present there will be no allowance for lowercase env var names
	only uppercase and underscores. No escaping of dollar signs is
	allowed for yet, and not ${BRACKETING}TO_STOP adjacent characters
	from being swallowed up is allowed for, either. These can be added
	later, 'as an exercise'.
*/
ASC_DLLSPEC char *env_subst(const char *path,GetEnvFn *getenvptr);

#endif
