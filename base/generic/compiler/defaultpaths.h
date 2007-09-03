#ifndef ASC_DEFAULTPATHS_H
#define ASC_DEFAULTPATHS_H

/**
	Return the system's default ASCENDSOLVERS value.
	You don't own the value thatt is returned, so don't try to free() it.

	On Windows, this default path is stored in the registry. On Linux,
	it is a compile-time setting.
*/
extern const char *get_default_solvers_path();

/**
	Return the system's default ASCENDLIBRARY value.
	You don't own the value thatt is returned, so don't try to free() it.

	On Windows, this default path is stored in the registry. On Linux,
	it is a compile-time setting.
*/
extern const char *get_default_library_path();

#endif
