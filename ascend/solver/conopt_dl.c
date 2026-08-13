/*	ASCEND modelling environment
	Copyright (C) 2006, 2007 Carnegie Mellon University

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	This file allows CONOPT to be dlopened at runtime.
*//*
	By John Pye
	Based on conopt.c by Vicente Rico Ramirez (created 05/97)
*/

#include <ascend/utilities/config.h>
#include <ascend/general/platform.h>
#include <ascend/utilities/error.h>
#include <ascend/utilities/ascEnvVar.h>
#include <ascend/general/env.h>
#include <ascend/general/ascMalloc.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef _WIN32
# include <unistd.h>
#endif
#include "conopt_dl.h"

#ifndef ASC_WITH_CONOPT
#ifdef __GNUC__
# warning "Shouldn't compile this file unless ASC_WITH_CONOPT set"
#endif
#else

#ifndef ASC_LINKED_CONOPT
# include <ascend/utilities/ascDynaLoad.h>

//#define ASC_CONOPT_DEBUG
#ifdef ASC_CONOPT_DEBUG
# define MSG(...) CONSOLE_DEBUG(__VA_ARGS__)
#else
# define MSG(...) (void)0
#endif

/*------------------------------------------------------------------------------
  DLOPENING CONOPT SUPPORT FUNCTIONS
*/

# define INTINT (int*cntvect,int*v)
# define INTINT1 (cntvect,v)
# define INTDOUBLE (int*cntvect,double*v)
# define INTDOUBLE1 (cntvect,v)
# define SEMICOLON ;
# define SPACE

/*
	Typedefs for the various function pointers
*/
# define FN_TYPE_DECL(T,A,V,L) \
	typedef int COI_CALL (T##_fn_t) A

CONOPT_FNS(FN_TYPE_DECL,SEMICOLON);
# undef FN_TYPE_DECL

/*
	Define a struct to hold all the function pointers, then
	declare it as a global variable.
*/
# define FN_PTR_DECL(T,A,V,L) \
	T##_fn_t* T##_ptr

typedef struct{
    CONOPT_FNS(FN_PTR_DECL,SEMICOLON);
} conopt_fptrs_t;
# undef FN_PTR_DECL

conopt_fptrs_t conopt_fptrs;
# ifdef ASC_CONOPT_API4
typedef void COI_CALL (COIGET_Version_fn_t)(int *major, int *minor, int *patch);
static COIGET_Version_fn_t *COIGET_Version_ptr = NULL;
# endif
static int conopt_loaded = 0;
static char *conopt_libpath = NULL;


/*
	Declare local functions to hook into the DLL
*/
# define FN_PTR_EXEC(T,A,V,L) \
	int COI_CALL T A{ \
		if(conopt_fptrs.T##_ptr==NULL){ \
			return 1; \
		} \
		return (* conopt_fptrs.T##_ptr) V ; \
	}

CONOPT_FNS(FN_PTR_EXEC,SPACE)

# undef FN_PTR_EXEC

/**
	This function will load the DLL and resolve all the required symbols
*/
int asc_conopt_load(){
# ifdef ASC_LINKED_CONOPT
#  error "We don't use this if we've got linked CONOPT!"
# endif
	char *libpath;
	int status;
	char fnsymbol[400], *c;
	const char *libname=ASC_CONOPT_LIB;
	const char *envvar;
# ifdef ASC_CONOPT_API4
	(void)c;
# endif

	if(conopt_loaded) {
		return 0; /* already loaded */
	}

	/* MSG("LOADING CONOPT..."); */

	envvar  = ASC_CONOPT_ENVVAR;

	/* need to import this variable into the ascend 'environment' */
	if(-1!=env_import(ASC_CONOPT_ENVVAR,getenv,Asc_PutEnv,0)){
		MSG("Searching in path '%s' (from env var '%s')",getenv(envvar),envvar);
	}/*else{
		MSG("Default conopt search path: %s", ASC_CONOPT_DLPATH);
	}*/

	/** @TODO replace with a direct call to ospath and/or importhandler? */
	libpath = SearchArchiveLibraryPath(libname, ASC_CONOPT_DLPATH, envvar);

	if(libpath==NULL){
		ERROR_REPORTER_NOLINE(ASC_PROG_ERR
			, "Library '%s' could not be located (check value of env var '%s' and/or default path '%s')"
			, libname, envvar, ASC_CONOPT_DLPATH
		);
		return 1;
	}

	status = Asc_DynamicLoad(libpath, NULL);
	if (status != 0) {
		ASC_FREE(libpath);
		return 1; /* failed to load */
	}

# ifdef ASC_CONOPT_API4
#  define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	conopt_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(conopt_fptrs.T##_ptr==NULL)status+=1;
# else
#  if defined(FNAME_UCASE_NODECOR) || defined(FNAME_UCASE_DECOR) || defined(FNAME_UCASE_PREDECOR)
#  define FNCASE(C) C=toupper(C)
#  elif defined(FNAME_LCASE_NODECOR) || defined(FNAME_LCASE_DECOR)
#  define FNCASE(C) C=tolower(C)
#  else
#  error "CONOPT case rule not defined"
#  endif

#  if defined(FNAME_UCASE_DECOR) || defined(FNAME_LCASE_DECOR)
#  define FNDECOR(S,L) strcat(S,"_")
#  elif defined(FNAME_UCASE_PREDECOR) /* on windows, precede with _ and append @L (integer value of L) */
#  define FNDECOR(S,L) strcat(S,L);for(c=S+strlen(S)+1;c>S;--c){*c=*(c-1);} *S='_';
#  else
#  define FNDECOR(S,L) (void)0
#  endif

# define FN_PTR_GET(T,A,V,L) \
	sprintf(fnsymbol,"%s",#T); \
	for(c=fnsymbol;*c!='\0';++c){ \
		FNCASE(*c); \
	} \
	FNDECOR(fnsymbol,L); \
	conopt_fptrs.T##_ptr = (T##_fn_t *)Asc_DynamicFunction(libpath,fnsymbol); \
	if(conopt_fptrs.T##_ptr==NULL)status+=1;
# endif

	CONOPT_FNS(FN_PTR_GET,SPACE)

# undef FN_PTR_GET
# ifndef ASC_CONOPT_API4
# undef FNDECOR
# undef FNCASE
# else
	COIGET_Version_ptr = (COIGET_Version_fn_t *)Asc_DynamicFunction(libpath,"COIGET_Version");
# endif

	if(status!=0){
		Asc_DynamicUnLoad(libpath);
		ASC_FREE(libpath);
		memset(&conopt_fptrs,0,sizeof(conopt_fptrs));
# ifdef ASC_CONOPT_API4
		COIGET_Version_ptr = NULL;
# endif
		return 1; /* failed to resolve all symbols */
	}

	conopt_libpath = libpath;
	conopt_loaded = 1;
	return 0;
}

int asc_conopt_unload(){
	int status = 0;

	if(!conopt_loaded){
		return 0;
	}

	memset(&conopt_fptrs,0,sizeof(conopt_fptrs));
# ifdef ASC_CONOPT_API4
	COIGET_Version_ptr = NULL;
# endif
	if(conopt_libpath != NULL){
		status = Asc_DynamicUnLoad(conopt_libpath);
		ASC_FREE(conopt_libpath);
		conopt_libpath = NULL;
	}
	conopt_loaded = 0;
	return status;
}

# ifdef ASC_CONOPT_API4
int asc_conopt_get_version(int *major, int *minor, int *patch){
	if(!conopt_loaded || COIGET_Version_ptr == NULL){
		return 1;
	}
	COIGET_Version_ptr(major,minor,patch);
	return 0;
}
# endif

#endif

#ifdef ASC_CONOPT_API4

#define ASC_CONOPT_SECRET_LINE_MAX 8192

static void asc_conopt_secure_free(char *value){
	if(value != NULL){
		volatile char *p = value;
		size_t n = strlen(value);
		while(n-- > 0){
			*p++ = '\0';
		}
		ASC_FREE(value);
	}
}

void asc_conopt_license_destroy(struct asc_conopt_license *license){
	if(license == NULL){
		return;
	}
	asc_conopt_secure_free(license->licstring);
	memset(license,0,sizeof(*license));
}

static int asc_conopt_parse_int(const char *text, int *value){
	char *end;
	long parsed;
	if(text == NULL || value == NULL){
		return 1;
	}
	while(isspace((unsigned char)*text)){
		++text;
	}
	if(*text == '\0'){
		return 1;
	}
	errno = 0;
	parsed = strtol(text,&end,10);
	if(errno == ERANGE || end == text || parsed < INT_MIN || parsed > INT_MAX){
		return 1;
	}
	while(isspace((unsigned char)*end)){
		++end;
	}
	if(*end != '\0'){
		return 1;
	}
	*value = (int)parsed;
	return 0;
}

int asc_conopt_parse_license(
	const char *encoded, struct asc_conopt_license *license
){
	char *work;
	char *sep;
	char *fields[3];
	size_t n;
	int i;
	if(license == NULL){
		return ASC_CONOPT_LICENSE_ERROR;
	}
	memset(license,0,sizeof(*license));
	if(encoded == NULL || *encoded == '\0'
		|| strchr(encoded,'\n') != NULL || strchr(encoded,'\r') != NULL
	){
		return ASC_CONOPT_LICENSE_ERROR;
	}
	n = strlen(encoded) + 1;
	work = ASC_NEW_ARRAY(char,n);
	if(work == NULL){
		return ASC_CONOPT_LICENSE_ERROR;
	}
	memcpy(work,encoded,n);
	for(i = 2; i >= 0; --i){
		sep = strrchr(work,',');
		if(sep == NULL){
			asc_conopt_secure_free(work);
			return ASC_CONOPT_LICENSE_ERROR;
		}
		*sep = '\0';
		fields[i] = sep + 1;
	}
	if(*work == '\0'
		|| asc_conopt_parse_int(fields[0],&license->licint1)
		|| asc_conopt_parse_int(fields[1],&license->licint2)
		|| asc_conopt_parse_int(fields[2],&license->licint3)
	){
		asc_conopt_secure_free(work);
		memset(license,0,sizeof(*license));
		return ASC_CONOPT_LICENSE_ERROR;
	}
	license->licstring = work;
	return ASC_CONOPT_LICENSE_APPLIED;
}

static char *asc_conopt_trim_left(char *text){
	while(*text != '\0' && isspace((unsigned char)*text)){
		++text;
	}
	return text;
}

static void asc_conopt_trim_right(char *text){
	size_t n = strlen(text);
	while(n > 0 && isspace((unsigned char)text[n - 1])){
		text[--n] = '\0';
	}
}

static char *asc_conopt_config_path(const char *base, const char *suffix){
	char *path;
	size_t n;
	if(base == NULL || *base == '\0'){
		return NULL;
	}
	n = strlen(base) + strlen(suffix) + 1;
	path = ASC_NEW_ARRAY(char,n);
	if(path != NULL){
		snprintf(path,n,"%s%s",base,suffix);
	}
	return path;
}

static char *asc_conopt_default_secrets_path(void){
#ifdef _WIN32
	const char *appdata = getenv("APPDATA");
	if(appdata != NULL && *appdata != '\0'){
		return asc_conopt_config_path(appdata,"/ascend/secrets.ini");
	}
#else
	const char *xdg = getenv("XDG_CONFIG_HOME");
	if(xdg != NULL && *xdg != '\0'){
		return asc_conopt_config_path(xdg,"/ascend/secrets.ini");
	}
#endif
	return asc_conopt_config_path(getenv("HOME"),"/.config/ascend/secrets.ini");
}

static int asc_conopt_check_secret_file(FILE *fp, const char *path){
#ifndef _WIN32
	struct stat st;
	if(fstat(fileno(fp),&st) != 0){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"Unable to inspect CONOPT secrets file '%s': %s",path,strerror(errno));
		return 1;
	}
	if(!S_ISREG(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077) != 0){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"CONOPT secrets file '%s' must be a regular file owned by the current user with no group or other permissions (for example, mode 0600)",path);
		return 1;
	}
#else
	(void)fp;
	(void)path;
#endif
	return 0;
}

static int asc_conopt_read_secret_file(
	const char *path, int explicit_path, char **encoded
){
	FILE *fp;
	char buf[ASC_CONOPT_SECRET_LINE_MAX];
	int in_conopt = 0;
	int found = 0;
	unsigned lineno = 0;
	int result = ASC_CONOPT_LICENSE_ABSENT;
	if(encoded == NULL){
		return ASC_CONOPT_LICENSE_ERROR;
	}
	*encoded = NULL;
	fp = fopen(path,"r");
	if(fp == NULL){
		if(errno == ENOENT && !explicit_path){
			return ASC_CONOPT_LICENSE_ABSENT;
		}
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"Unable to read CONOPT secrets file '%s': %s",path,strerror(errno));
		return ASC_CONOPT_LICENSE_ERROR;
	}
	if(asc_conopt_check_secret_file(fp,path)){
		fclose(fp);
		return ASC_CONOPT_LICENSE_ERROR;
	}
	while(fgets(buf,sizeof(buf),fp) != NULL){
		char *line;
		char *end;
		char *eq;
		char *key;
		char *value;
		size_t n;
		++lineno;
		n = strlen(buf);
		if(n > 0 && buf[n - 1] != '\n' && !feof(fp)){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Line %u is too long in CONOPT secrets file '%s'",lineno,path);
			result = ASC_CONOPT_LICENSE_ERROR;
			goto cleanup;
		}
		buf[strcspn(buf,"\r\n")] = '\0';
		line = asc_conopt_trim_left(buf);
		asc_conopt_trim_right(line);
		if(*line == '\0' || *line == '#' || *line == ';'){
			continue;
		}
		if(*line == '['){
			end = strchr(line,']');
			if(end == NULL){
				in_conopt = 0;
				continue;
			}
			*end = '\0';
			++end;
			end = asc_conopt_trim_left(end);
			in_conopt = (*end == '\0' && strcmp(line + 1,"conopt") == 0);
			continue;
		}
		if(!in_conopt){
			continue;
		}
		eq = strchr(line,'=');
		if(eq == NULL){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Malformed entry at line %u in CONOPT secrets file '%s'",lineno,path);
			result = ASC_CONOPT_LICENSE_ERROR;
			goto cleanup;
		}
		*eq = '\0';
		key = asc_conopt_trim_left(line);
		asc_conopt_trim_right(key);
		if(strcmp(key,"license") != 0){
			continue;
		}
		if(found){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Duplicate CONOPT license entry in secrets file '%s'",path);
			result = ASC_CONOPT_LICENSE_ERROR;
			goto cleanup;
		}
		value = asc_conopt_trim_left(eq + 1);
		asc_conopt_trim_right(value);
		if(*value == '\0'){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Empty CONOPT license entry in secrets file '%s'",path);
			result = ASC_CONOPT_LICENSE_ERROR;
			goto cleanup;
		}
		n = strlen(value) + 1;
		*encoded = ASC_NEW_ARRAY(char,n);
		if(*encoded == NULL){
			result = ASC_CONOPT_LICENSE_ERROR;
			goto cleanup;
		}
		memcpy(*encoded,value,n);
		found = 1;
	}
	if(ferror(fp)){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"Error while reading CONOPT secrets file '%s'",path);
		result = ASC_CONOPT_LICENSE_ERROR;
		goto cleanup;
	}
	result = found ? ASC_CONOPT_LICENSE_APPLIED : ASC_CONOPT_LICENSE_ABSENT;

cleanup:
	memset(buf,0,sizeof(buf));
	fclose(fp);
	if(result == ASC_CONOPT_LICENSE_ERROR){
		asc_conopt_secure_free(*encoded);
		*encoded = NULL;
	}
	return result;
}

static int asc_conopt_load_license(struct asc_conopt_license *license){
	const char *environment_license = getenv(ASC_CONOPT_LICENSE_ENV);
	const char *explicit_path = getenv(ASC_CONOPT_SECRETS_FILE_ENV);
	char *default_path = NULL;
	char *encoded = NULL;
	const char *path;
	int result;
	if(environment_license != NULL && *environment_license != '\0'){
		result = asc_conopt_parse_license(environment_license,license);
		if(result == ASC_CONOPT_LICENSE_ERROR){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Malformed CONOPT license in environment variable %s",ASC_CONOPT_LICENSE_ENV);
		}
		return result;
	}
	if(explicit_path != NULL && *explicit_path != '\0'){
		path = explicit_path;
	}else{
		default_path = asc_conopt_default_secrets_path();
		path = default_path;
	}
	if(path == NULL){
		return ASC_CONOPT_LICENSE_ABSENT;
	}
	result = asc_conopt_read_secret_file(
		path, explicit_path != NULL && *explicit_path != '\0', &encoded
	);
	if(result == ASC_CONOPT_LICENSE_APPLIED){
		result = asc_conopt_parse_license(encoded,license);
		if(result == ASC_CONOPT_LICENSE_ERROR){
			ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
				"Malformed CONOPT license in secrets file '%s'",path);
		}
	}
	asc_conopt_secure_free(encoded);
	ASC_FREE(default_path);
	return result;
}

void asc_conopt_license_string_destroy(char *licstring){
	asc_conopt_secure_free(licstring);
}

int asc_conopt_license_status(char **licstring){
	struct asc_conopt_license license = {0};
	int result = asc_conopt_load_license(&license);
	if(licstring != NULL){
		*licstring = NULL;
	}
	if(result == ASC_CONOPT_LICENSE_APPLIED){
		if(licstring != NULL){
			*licstring = license.licstring;
			license.licstring = NULL;
		}
		asc_conopt_license_destroy(&license);
	}
	return result;
}

#define ASC_CONOPT_LICENSE_PROBE_N 1001
#define ASC_CONOPT_LICENSE_PROBE_M 1002
#define ASC_CONOPT_LICENSE_PROBE_NZ 2002

struct asc_conopt_license_probe {
	int invalid_message;
	int demo_limit_message;
	int readmatrix_calls;
};

static int COI_CALL asc_conopt_license_probe_readmatrix(
	double lower[], double curr[], double upper[], int vsta[], int typex[],
	double rhs[], int esta[], int colsta[], int rowno[], double value[],
	int nlflag[], int numvar, int numcon, int numnz, void *usrmem
){
	struct asc_conopt_license_probe *probe = usrmem;
	int i;
	if(probe == NULL
		|| numvar != ASC_CONOPT_LICENSE_PROBE_N
		|| numcon != ASC_CONOPT_LICENSE_PROBE_M
		|| numnz != ASC_CONOPT_LICENSE_PROBE_NZ
	){
		return 1;
	}
	probe->readmatrix_calls++;
	for(i = 0; i < ASC_CONOPT_LICENSE_PROBE_N; ++i){
		lower[i] = -2.0;
		curr[i] = 1.0;
		upper[i] = 2.0;
		vsta[i] = 0;
		colsta[i] = 2 * i;
		rowno[2 * i] = i;
		value[2 * i] = 2.0;
		nlflag[2 * i] = 1;
		rowno[2 * i + 1] = ASC_CONOPT_LICENSE_PROBE_N;
		value[2 * i + 1] = 1.0;
		nlflag[2 * i + 1] = 0;
	}
	colsta[ASC_CONOPT_LICENSE_PROBE_N] = ASC_CONOPT_LICENSE_PROBE_NZ;
	for(i = 0; i < ASC_CONOPT_LICENSE_PROBE_N; ++i){
		typex[i] = 0;
		rhs[i] = 1.0;
		esta[i] = 0;
	}
	typex[ASC_CONOPT_LICENSE_PROBE_N] = 3;
	rhs[ASC_CONOPT_LICENSE_PROBE_N] = 0.0;
	esta[ASC_CONOPT_LICENSE_PROBE_N] = 0;
	return 0;
}

static int COI_CALL asc_conopt_license_probe_fdeval(
	const double x[], double *g, double jac[], int rowno,
	const int jacnum[], int mode, int ignerr, int *errcnt, int numvar,
	int numjac, int thread, void *usrmem
){
	int i;
	(void)ignerr;
	(void)errcnt;
	(void)thread;
	(void)usrmem;
	if(numvar != ASC_CONOPT_LICENSE_PROBE_N
		|| rowno < 0 || rowno >= ASC_CONOPT_LICENSE_PROBE_M
	){
		return 1;
	}
	if(mode == 1 || mode == 3){
		if(rowno == ASC_CONOPT_LICENSE_PROBE_N){
			*g = 0.0;
			for(i = 0; i < ASC_CONOPT_LICENSE_PROBE_N; ++i){
				*g += x[i];
			}
		}else{
			*g = x[rowno] * x[rowno];
		}
	}
	if(mode == 2 || mode == 3){
		for(i = 0; i < numjac; ++i){
			int col = jacnum != NULL ? jacnum[i] : i;
			jac[i] = rowno == ASC_CONOPT_LICENSE_PROBE_N
				? 1.0 : (col == rowno ? 2.0 * x[col] : 0.0);
		}
	}
	return 0;
}

static int COI_CALL asc_conopt_license_probe_status(
	int modsta, int solsta, int iter, double objval, void *usrmem
){
	(void)modsta;
	(void)solsta;
	(void)iter;
	(void)objval;
	(void)usrmem;
	return 0;
}

static int COI_CALL asc_conopt_license_probe_solution(
	const double xval[], const double xmar[], const int xbas[],
	const int xsta[], const double yval[], const double ymar[],
	const int ybas[], const int ysta[], int numvar, int numcon, void *usrmem
){
	(void)xval;
	(void)xmar;
	(void)xbas;
	(void)xsta;
	(void)yval;
	(void)ymar;
	(void)ybas;
	(void)ysta;
	(void)numvar;
	(void)numcon;
	(void)usrmem;
	return 0;
}

static int COI_CALL asc_conopt_license_probe_message(
	int smsg, int dmsg, int nmsg, char *msgv[], void *usrmem
){
	struct asc_conopt_license_probe *probe = usrmem;
	int i;
	(void)smsg;
	(void)dmsg;
	if(probe == NULL || msgv == NULL){
		return 0;
	}
	for(i = 0; i < nmsg; ++i){
		if(msgv[i] == NULL){
			continue;
		}
		if(strstr(msgv[i],"No valid license") != NULL){
			probe->invalid_message = 1;
		}
		if(strstr(msgv[i],"Limits for Demo Version Exceeded") != NULL){
			probe->demo_limit_message = 1;
		}
	}
	return 0;
}

static int COI_CALL asc_conopt_license_probe_errmsg(
	int rowno, int colno, int posno, const char *msg, void *usrmem
){
	(void)rowno;
	(void)colno;
	(void)posno;
	(void)msg;
	(void)usrmem;
	return 0;
}

int asc_conopt_validate_license(char **licstring){
	struct asc_conopt_license license = {0};
	struct asc_conopt_license_probe probe = {0};
	coiHandle_t cntvect = NULL;
	int result;
	if(licstring != NULL){
		*licstring = NULL;
	}
	result = asc_conopt_load_license(&license);
	if(result != ASC_CONOPT_LICENSE_APPLIED){
		return result;
	}
	if(COI_Create(&cntvect) != 0 || cntvect == NULL){
		result = ASC_CONOPT_LICENSE_ERROR;
		goto cleanup;
	}
	if(COIDEF_License(
		cntvect,license.licint1,license.licint2,license.licint3,
		license.licstring
	) != 0){
		result = ASC_CONOPT_LICENSE_ERROR;
		goto cleanup;
	}
	COIDEF_NumVar(cntvect,ASC_CONOPT_LICENSE_PROBE_N);
	COIDEF_NumCon(cntvect,ASC_CONOPT_LICENSE_PROBE_M);
	COIDEF_NumNz(cntvect,ASC_CONOPT_LICENSE_PROBE_NZ);
	COIDEF_NumNlNz(cntvect,ASC_CONOPT_LICENSE_PROBE_N);
	COIDEF_OptDir(cntvect,1);
	COIDEF_ObjCon(cntvect,ASC_CONOPT_LICENSE_PROBE_N);
	COIDEF_ItLim(cntvect,0);
	COIDEF_ErrLim(cntvect,20);
	COIDEF_StdOut(cntvect,0);
	COIDEF_UsrMem(cntvect,&probe);
	COIDEF_ReadMatrix(cntvect,asc_conopt_license_probe_readmatrix);
	COIDEF_FDEval(cntvect,asc_conopt_license_probe_fdeval);
	COIDEF_Status(cntvect,asc_conopt_license_probe_status);
	COIDEF_Solution(cntvect,asc_conopt_license_probe_solution);
	COIDEF_Message(cntvect,asc_conopt_license_probe_message);
	COIDEF_ErrMsg(cntvect,asc_conopt_license_probe_errmsg);
	(void)COI_Solve(cntvect);
	if(probe.invalid_message || probe.demo_limit_message){
		result = ASC_CONOPT_LICENSE_INVALID;
	}else if(probe.readmatrix_calls > 0){
		result = ASC_CONOPT_LICENSE_VALID;
	}else{
		result = ASC_CONOPT_LICENSE_ERROR;
	}

cleanup:
	if(cntvect != NULL){
		COI_Free(&cntvect);
	}
	if(licstring != NULL){
		*licstring = license.licstring;
		license.licstring = NULL;
	}
	asc_conopt_license_destroy(&license);
	return result;
}

int asc_conopt_apply_license(coiHandle_t cntvect){
	struct asc_conopt_license license;
	int result;
	int conopt_result;
	if(cntvect == NULL){
		return ASC_CONOPT_LICENSE_ERROR;
	}
	result = asc_conopt_load_license(&license);
	if(result != ASC_CONOPT_LICENSE_APPLIED){
		return result;
	}
	conopt_result = COIDEF_License(
		cntvect,license.licint1,license.licint2,license.licint3,license.licstring
	);
	asc_conopt_license_destroy(&license);
	if(conopt_result != 0){
		ERROR_REPORTER_NOLINE(ASC_USER_ERROR,
			"CONOPT rejected the configured license information");
		return ASC_CONOPT_LICENSE_ERROR;
	}
	return ASC_CONOPT_LICENSE_APPLIED;
}

#endif /* ASC_CONOPT_API4 */

/*-----------------------------------------------------------------------------
   std.c (modified from the version provided with CONOPT)

   This file has some 'standard' implementations for the mandatory
   callback routines Message, ErrMsg, Status, and Solution.
   The routines use global file pointers, so they are only intended
   as examples that can be used for further refinements.

	FIXME this code could probably be moved to asc_conopt.c?
*/

#define MAXLINE 133  /* maximum line length plus an extra character
                        for the null terminator                       */

#ifdef ASC_CONOPT_API4

int COI_CALL asc_conopt_progress( int LEN_INT, const int INT[]
		, int LEN_RL, const double RL[], const double X[], void* USRMEM
){
	(void)LEN_INT;
	(void)LEN_RL;
	(void)X;
	(void)USRMEM;
	MSG("Iteration %d, phase %d: %d infeasible, %d non-optimal; objective = %e"
		, INT[0], INT[1], INT[2], INT[3], RL[1]
	);
	return 0;
}

int COI_CALL asc_conopt_message( int SMSG, int DMSG, int NMSG
		, char* MSGV[], void* USRMEM
){
	int i;
	(void)DMSG;
	(void)USRMEM;
	for(i = 0; i < SMSG; ++i){
		MSG("%s", MSGV[i]);
	}
	for(i = 0; i < NMSG; ++i){
		ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"(CONOPT) %s", MSGV[i]);
	}
	return 0;
}

int COI_CALL asc_conopt_errmsg( int ROWNO, int COLNO, int POSNO
		, const char* MSG, void* USRMEM
){
	(void)POSNO;
	(void)USRMEM;
	ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
	if ( ROWNO == -1 ) {
		FPRINTF(ASCERR,"Variable %d : ",COLNO);
	}else if ( COLNO == -1 ) {
		FPRINTF(ASCERR,"Equation %d : ",ROWNO);
	}else{
		FPRINTF(ASCERR,"Variable %d appearing in Equation %d : ",COLNO, ROWNO);
	}
	FPRINTF(ASCERR,"%s\n", MSG);
	error_reporter_end_flush();
	return 0;
}

int COI_CALL asc_conopt_status(int MODSTA, int SOLSTA
		, int ITER, double OBJVAL, void* USRMEM
){
	int *modsta = &MODSTA;
	int *solsta = &SOLSTA;
	int *iter = &ITER;
	double *objval = &OBJVAL;
	(void)iter;
	(void)objval;
	(void)USRMEM;

	MSG("CONOPT has finished Optimizing");
	MSG("Model status    = %8d", *modsta);
	MSG("Solver status   = %8d", *solsta);
	MSG("Iteration count = %8d", *iter);
	MSG("Objective value = %10f", *objval);

	const char *modstatxt;
	error_severity_t t = ASC_USER_SUCCESS;
	switch(*modsta){
		case 1: modstatxt = "optimal"; break;
		case 2: modstatxt = "locally optimal"; break;
		case 3: t = ASC_USER_ERROR; modstatxt = "unbounded"; break;
		case 4: t = ASC_USER_ERROR; modstatxt = "infeasible"; break;
		case 5: modstatxt = "locally infeasible"; break;
		case 6: modstatxt = "intermediate infeasible"; break;
		case 7: modstatxt = "intermediate non-optimal"; break;
		case 12: modstatxt = "unknown type of error"; break;
		case 13: modstatxt = "error no solution"; break;
		case 15: modstatxt = "solved unique"; break;
		case 16: modstatxt = "solved"; break;
		case 17: modstatxt = "solved singular"; break;
		default: t = ASC_PROG_ERR; modstatxt = "UNKNOWN MODSTA";
	}
	const char *solstatxt;
	switch(*solsta){
		case 1: solstatxt = "normal completion"; break;
		case 2: t = ASC_USER_NOTE; solstatxt = "iteration interrupted"; break;
		case 3: t = ASC_PROG_NOTE; solstatxt = "time limit exceeded"; break;
		case 4: t = ASC_PROG_ERR; solstatxt = "failed (terminated by solver)"; break;
		case 5: t = ASC_PROG_ERR; solstatxt = "Error evaluation limit"; break;
		case 8: t = ASC_USER_NOTE; solstatxt = "User interrupt"; break;
		case 9: t = ASC_PROG_ERR; solstatxt = "Error: setup failure"; break;
		case 10:t = ASC_PROG_ERR; solstatxt = "Error: solver failure"; break;
		case 11:t = ASC_PROG_ERR; solstatxt = "Error: internal solver error"; break;
		case 15:t = ASC_PROG_ERR; solstatxt = "Terminated by Quick Mode"; break;
		default: t = ASC_PROG_ERR; solstatxt = "UNKNOWN SOLSTA";
	}

	MSG("CONOPT %s (%d): %s (%d)", solstatxt, *solsta, modstatxt, *modsta);
	ERROR_REPORTER_NOLINE(t,"CONOPT %s: %s", solstatxt, modstatxt);

	return 0;
}

int COI_CALL asc_conopt_solution( const double XVAL[], const double XMAR[]
		, const int XBAS[], const int XSTA[], const double YVAL[], const double YMAR[]
		, const int YBAS[], const int YSTA[], int N, int M, void* USRMEM
){
	int i;
	const char *status[4] = {"Lower","Upper","Basic","Super"};
	FILE *fd = stderr;
	(void)XSTA;
	(void)YVAL;
	(void)YMAR;
	(void)YSTA;
	(void)USRMEM;

	fprintf(fd,"\n Variable   Solution value    Reduced cost    Status\n\n");
	for ( i=0; i<N; i++ )
		fprintf(fd,"%6d%18f%18f%10s\n", i, XVAL[i], XMAR[i], status[XBAS[i]] );
	fprintf(fd,"\n Constrnt   Activity level    Marginal cost   Status\n\n");
	for ( i=0; i<M; i++ )
		fprintf(fd,"%6d%18f%18f%10s\n", i, YVAL[i], YMAR[i], status[YBAS[i]] );

	return 0;
}

#else

int COI_CALL asc_conopt_progress( int* LEN_INT, int* INT
		, int* LEN_RL, double* RL, double* X, double* USRMEM
){
	MSG("Iteration %d, phase %d: %d infeasible, %d non-optimal; objective = %e"
		, INT[0], INT[1], INT[2], INT[3], RL[1]
	);
	/* FIXME: NEED TO IMPLEMENT SOME KIND OF CALLBACK TO THE SOLVERREPORTER */
	return 0;
}

int COI_CALL asc_conopt_message( int* SMSG, int* DMSG, int* NMSG, int* LLEN
		,double* USRMEM, char* MSGV, int MSGLEN
){
/* This implementation is writing the screen file to stdout
   the documentation file to a file opened in main with the name
   document.txt and the status file to a file with the name
   status.txt.                                                        */
   int i,j,k,l;
   char line[MAXLINE];
   k = 0;
   for( i=0; i<*SMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      MSG("%s", line);
      k += MSGLEN;
   }
/*   k = 0;
   for( i=0; i<*DMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      ERROR_REPORTER_NOLINE(ASC_PROG_NOTE,"%s\n", line);
      k += MSGLEN;
   }
*/
   k = 0;
   for( i=0; i<*NMSG;i++ ){
      j = LLEN[i];
      for( l= 0; l<j; l++ ) line[l] = MSGV[k+l];
      line[j] = '\0';
      ERROR_REPORTER_NOLINE(ASC_USER_NOTE,"(CONOPT) %s", line);
      k += MSGLEN;
   }
   return 0;
}

int COI_CALL asc_conopt_errmsg( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN
		, double* USRMEM, char* MSG, int LENMSG
){
   /* Standard ErrMsg routine. Write to Documentation and Status file*/
   int j,l;
   char line[MAXLINE];
   ERROR_REPORTER_START_NOLINE(ASC_PROG_ERR);
   if ( *ROWNO == -1 ) {
      FPRINTF(ASCERR,"Variable %d : ",*COLNO); }
   else if ( *COLNO == -1 ) {
      FPRINTF(ASCERR,"Equation %d : ",*ROWNO); }
   else  {
      FPRINTF(ASCERR,"Variable %d appearing in Equation %d : ",*COLNO, *ROWNO); }
   j = *MSGLEN;
   for( l= 0; l<j; l++ ) line[l] = MSG[l];
   line[j] = '\0';
   FPRINTF(ASCERR,"%s\n", line);
   error_reporter_end_flush();
   return 0;
}

int COI_CALL asc_conopt_status(int* MODSTA, int* SOLSTA
		, int* ITER, double* OBJVAL, double* USRMEM
){
	/* Standard Status routine. Write to all files */
	MSG("CONOPT has finished Optimizing");
	MSG("Model status    = %8d", *MODSTA);
	MSG("Solver status   = %8d", *SOLSTA);
	MSG("Iteration count = %8d", *ITER);
	MSG("Objective value = %10f", *OBJVAL);

	const char *modsta;
	error_severity_t t = ASC_USER_SUCCESS;
	switch(*MODSTA){
		case 1: modsta = "optimal"; break;
		case 2: modsta = "locally optimal"; break;
		case 3: t = ASC_USER_ERROR; modsta = "unbounded"; break;
		case 4: t = ASC_USER_ERROR; modsta = "infeasible"; break;
		case 5: modsta = "locally infeasible"; break;
		case 6: modsta = "intermediate infeasible"; break;
		case 7: modsta = "intermediate non-optimal"; break;
		case 12: modsta = "unknown type of error"; break;
		case 13: modsta = "error no solution"; break;
		case 15: modsta = "solved unique"; break;
		case 16: modsta = "solved"; break;
		case 17: modsta = "solved singular"; break;
		default: t = ASC_PROG_ERR; modsta = "UNKNOWN MODSTA";
	}
	const char *solsta;
	switch(*SOLSTA){
		case 1: solsta = "normal completion"; break;
		case 2: t = ASC_USER_NOTE; solsta = "iteration interrupted"; break;
		case 3: t = ASC_PROG_NOTE; solsta = "time limit exceeded"; break;
		case 4: t = ASC_PROG_ERR; solsta = "failed (terminated by solver)"; break;
		case 5: t = ASC_PROG_ERR; solsta = "Error evaluation limit"; break;
		case 8: t = ASC_USER_NOTE; solsta = "User interrupt"; break;
		case 9: t = ASC_PROG_ERR; solsta = "Error: setup failure"; break;
		case 10:t = ASC_PROG_ERR; solsta = "Error: solver failure"; break;
		case 11:t = ASC_PROG_ERR; solsta = "Error: internal solver error"; break;
		case 15:t = ASC_PROG_ERR; solsta = "Terminated by Quick Mode"; break;
		default: t = ASC_PROG_ERR; solsta = "UNKNOWN SOLSTA";
	}

	MSG("CONOPT %s (%d): %s (%d)", solsta, *SOLSTA, modsta, *MODSTA);
	ERROR_REPORTER_NOLINE(t,"CONOPT %s: %s", solsta, modsta);

	return 0;
}

int COI_CALL asc_conopt_solution( double* XVAL, double* XMAR, int* XBAS
		, int* XSTA, double* YVAL, double* YMAR, int* YBAS, int* YSTA
		, int* N, int* M, double* USRMEM
){
   /* Standard Solution routine */
   int i;
   char *status[4] = {"Lower","Upper","Basic","Super"};
   FILE *fd = stderr;

   fprintf(fd,"\n Variable   Solution value    Reduced cost    Status\n\n");
   for ( i=0; i<*N; i++ )
      fprintf(fd,"%6d%18f%18f%10s\n", i, XVAL[i], XMAR[i], status[XBAS[i]] );
   fprintf(fd,"\n Constrnt   Activity level    Marginal cost   Status\n\n");
   for ( i=0; i<*M; i++ )
      fprintf(fd,"%6d%18f%18f%10s\n", i, YVAL[i], YMAR[i], status[YBAS[i]] );

   return 0;
}

#endif

#endif /* ASC_WITH_CONOPT */
