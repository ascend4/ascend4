#include "defaultpaths.h"

#include <utilities/config.h>

#ifdef WIN32
#include <windows.h>
#endif

const char *get_default_solvers_path(){
#ifdef WIN32
# define MAXLEN 3000
	HKEY key;
	DWORD datatype, len = MAXLEN;
	long res;
	static char value[MAXLEN];
	res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASCEND", 0L, KEY_QUERY_VALUE, &key);
	if(res==ERROR_SUCCESS){
		res = RegQueryValueEx(key, ASC_ENV_SOLVERS, NULL, &datatype, value, &len);
		if(res==ERROR_SUCCESS){
			RegCloseKey(key);
			return value;
		}
	}
#endif
	return ASC_DEFAULT_ASCENDSOLVERS;
}

const char *get_default_library_path(){
#ifdef WIN32
# define MAXLEN 3000
	HKEY key;
	DWORD datatype, len = MAXLEN;
	long res;
	static char value[MAXLEN];
	res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\ASCEND", 0L, KEY_QUERY_VALUE, &key);
	if(res==ERROR_SUCCESS){
		res = RegQueryValueEx(key, ASC_ENV_LIBRARY, NULL, &datatype, value, &len);
		if(res==ERROR_SUCCESS){
			RegCloseKey(key);
			return value;
		}
	}
#endif
	return ASC_DEFAULT_ASCENDLIBRARY;
}
