#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <ctype.h>

#include "ospath.h"

// to test this code, 'gcc -DTEST ospath.c && ./a'

#if defined(WIN32) || defined(__WIN32) || defined(_MSC_VER)
# ifndef __WIN32__
#  define __WIN32__
# endif
#endif

//#define VERBOSE

#if !defined(TEST) && !defined(VERBOSE)
# define NDEBUG
#endif

//#define TRY_GETPWUID

#define DO_FIXSLASHES

#ifndef NDEBUG
# include <assert.h>
# define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG)
# define X(VAR) fprintf(stderr,"%s:%d: (%s) %s=%s\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
# define V(VAR) fprintf(stderr,"%s:%d: (%s) %s=%d\n",__FILE__,__LINE__,__FUNCTION__,#VAR,(VAR))
# define D(VAR) fprintf(stderr,"%s:%d: (%s) %s=",__FILE__,__LINE__,__FUNCTION__,#VAR);ospath_debug(VAR)
#else
# include <assert.h>
# define M(MSG) ((void)0)
# define X(VAR) ((void)0)
# define V(VAR) ((void)0)
# define D(VAR) ((void)0)
#endif

#if defined(__WIN32__) && !defined(__MINGW32__)
# include <direct.h>
# include <stdlib.h>
# define STRCPY strcpy
# define STRNCPY(dest,src,n) strncpy_s(dest,n,src,n)
# define STRCAT strcat
# define STRNCAT strncat
# define STRTOK(STR,PAT,VAR) strtok_s(STR,PAT,&VAR)
# define STRTOKVAR(VAR) char *VAR
# define GETCWD getcwd
# define GETENV(VAR) getenv(VAR)
#else
# define STRCPY strcpy
# define STRNCPY(dest,src,n) strncpy(dest,src,n)
# define STRCAT strcat
# define STRNCAT strncat
# define STRTOK(STR,PAT,VAR) strtok(STR,PAT)
# define STRTOKVAR(VAR) ((void)0)
# define GETCWD getcwd
# define GETENV(VAR) getenv(VAR)
#endif

// PATH_MAX is in ospath.h
#define DRIVEMAX 3
#define LISTMAX 256

#ifdef __WIN32__ /* && !defined(__MINGW32__) */
# define WINPATHS
#endif

struct FilePath{
    char path[PATH_MAX]; /// the string version of the represented POSIX path

#ifdef WINPATHS
    char drive[DRIVEMAX]; /// the drive the path resides on (field is absent in POSIX systems)
#endif
};

#include <string.h>

#define MALLOC malloc
#define FREE free
#define CALLOC calloc

#define E(MSG) fprintf(stderr,"%s:%d: (%s) ERROR: %s\n",__FILE__,__LINE__,__FUNCTION__,MSG)

#ifdef DO_FIXSLASHES
void ospath_fixslash(char *path);
#endif

struct FilePath *ospath_getcwd();

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
void ospath_cleanup(struct FilePath *);

void ospath_copy(struct FilePath *dest, struct FilePath *src);


#ifdef WINPATHS
/**
	This function splits out the drive letter in the path string, thus completing
	the correct construction of a FilePath object under Win32.
*/
void ospath_extractdriveletter(struct FilePath *);
#endif

#ifdef WINPATHS
# define PATH_SEPARATOR_STR  "\\"
# define PATH_SEPARATOR_CHAR  '\\'
# define PATH_LISTSEP_CHAR ';'
# define PATH_LISTSEP_STR ";"
# define PATH_WRONGSLASH_CHAR '/'
# define PATH_WRONGSLASH_STR "/"
#else
# define PATH_SEPARATOR_STR "/"
# define PATH_SEPARATOR_CHAR '/'
# define PATH_LISTSEP_CHAR ':'
# define PATH_LISTSEP_STR ":"
# define PATH_WRONGSLASH_CHAR '\\'
# define PATH_WRONGSLASH_STR "\\"
#endif

/**
	Create a new path structure from a string
*/
struct FilePath *ospath_new(const char *path){
	struct FilePath *fp;
	fp = MALLOC(sizeof(struct FilePath));
	STRNCPY(fp->path, path, PATH_MAX);
	//X(fp->path);
	//X(path);
	assert(strcmp(fp->path,path)==0);
#ifdef WINPATHS
	//X(fp->drive);
	ospath_extractdriveletter(fp);
#endif

	//X(fp->drive);

#ifdef DO_FIXSLASHES
	ospath_fixslash(fp->path);
#endif

	ospath_cleanup(fp);

	//D(fp);

	return fp;
}

void ospath_free(struct FilePath *fp){
	FREE(fp);
}

/**
	This function will serve to allow #include-style file paths
	to be specified with platform-independent forward slashes then
	translated into the local filesystem format for subsequent use.

	This method should be identical to ospath_new on posix, right?

	@NOTE: on windows, we also want:
				    C:dir/file --> c:$PWD\dir\file
	                  e:/hello --> e:\hello
	                 here/i/am --> here\i\am

	@NOTE:
	A path-search function should create full file paths by
	appending relative file to each component of the search path
	then performing a callback on each one to determine if the
	match is OK or not.
*/
struct FilePath *ospath_new_from_posix(char *posixpath){
	struct FilePath *fp = MALLOC(sizeof(struct FilePath));
	STRNCPY(fp->path,posixpath,PATH_MAX);
#ifdef WINPATHS
	X(fp->path);
	ospath_extractdriveletter(fp);
#endif

#ifdef DO_FIXSLASHES
	ospath_fixslash(fp->path);
#endif

	//X(fp->path);

	ospath_cleanup(fp);

	return fp;
}

#ifdef DO_FIXSLASHES
void ospath_fixslash(char *path){

	char *p;
	char temp[PATH_MAX];
	int startslash;
	int endslash;
	STRTOKVAR(nexttok);

	STRNCPY(temp,path,PATH_MAX);

	//X(path);

	startslash = (strlen(temp) > 0 && temp[0] == PATH_WRONGSLASH_CHAR);
	endslash = (strlen(temp) > 1 && temp[strlen(temp) - 1] == PATH_WRONGSLASH_CHAR);

	//V(startslash);
	//V(endslash);

	// reset fp->path as required.
	STRNCPY(path, (startslash ? PATH_SEPARATOR_STR : ""), PATH_MAX);

	//M("STARTING STRTOK");
	for(p = STRTOK(temp, PATH_WRONGSLASH_STR,nexttok);
			p!=NULL;
			p = STRTOK(NULL,PATH_WRONGSLASH_STR,nexttok)
	){
		// add a separator if we've already got some stuff
		if(
			strlen(path) > 0
			&& path[strlen(path) - 1] != PATH_SEPARATOR_CHAR
		){
			STRCAT(path,PATH_SEPARATOR_STR);
		}

		STRCAT(path,p);
	}
	//M("FINISHED STRTOK");

	// put / on end as required, according to what the starting path had
	if(endslash && (strlen(path) > 0 ? (path[strlen(path) - 1] != PATH_SEPARATOR_CHAR) : 1))
	{
		//M("adding endslash!");

		STRCAT(path, PATH_SEPARATOR_STR);
	}

	//X(path);
}
#endif


/// Create but with no 'cleanup' call
struct FilePath *ospath_new_noclean(const char *path){
	struct FilePath *fp = MALLOC(sizeof(struct FilePath));
	STRNCPY(fp->path,path,PATH_MAX);

#ifdef WINPATHS
	//X(fp->path);
	ospath_extractdriveletter(fp);
#endif

	//D(fp);

/*
#ifdef DO_FIXSLASHES
	ospath_fixslash(fp->path);
	D(fp);
#endif
*/
	return fp;
}

struct FilePath *ospath_getcwd(void){
	struct FilePath *fp = MALLOC(sizeof(struct FilePath));
	char *cwd;

	// get current working directory
	cwd = (char *)GETCWD(NULL, 0);

	// create new path with resolved working directory
	fp = ospath_new_noclean(cwd != NULL ? cwd : ".");

	D(fp);

	return fp;
}

/**
	Use getenv() function to retrieve HOME path, or if not set, use
	the password database and try to retrieve it that way (???)
*/
struct FilePath *ospath_gethomepath(void){

	const char *pfx = (const char *)getenv("HOME");
	struct FilePath *fp;

#ifndef __WIN32__
# ifdef TRY_GETPWUID
	struct passwd *pw;

	if(pfx==NULL){
		 pw = (struct passwd*)getpwuid(getuid());

		if(pw){
			pfx = pw->pw_dir;
		}
	}
# endif
#endif

	// create path object from HOME, but don't compress it
	// (because that would lead to an infinite loop)
	fp = ospath_new_noclean(pfx ? pfx : "");

#ifdef DO_FIXSLASHES
	ospath_fixslash(fp->path);
#endif

	return fp;
}

#ifdef WINPATHS
void ospath_extractdriveletter(struct FilePath *fp)
{
	char *p;
	//M("SOURCE");
	//X(fp->path);
	//fprintf(stderr,"CHAR 1 = %c\n",fp->path[1]);

	// extract the drive the path resides on...
	if(strlen(fp->path) >= 2 && fp->path[1] == ':')
	{
		STRNCPY(fp->drive,fp->path,2);
		fp->drive[2]='\0';
		for(p=fp->path+2; *p!='\0'; ++p){
			*(p-2)=*p;
		}
		*(p-2)='\0';
	}else{
		STRNCPY(fp->drive,"",DRIVEMAX);
	}
	//M("RESULT");
	//X(fp->path);
	//X(fp->drive);
}
#endif

void ospath_cleanup(struct FilePath *fp){
	char path[PATH_MAX];
	char *p;
	struct FilePath *home;
	struct FilePath *working;
	struct FilePath *parent;
	STRTOKVAR(nexttok);

	// compress the path, and resolve ~
	int startslash = (strlen(fp->path) > 0 && fp->path[0] == PATH_SEPARATOR_CHAR);
	int endslash = (strlen(fp->path) > 1 && fp->path[strlen(fp->path) - 1] == PATH_SEPARATOR_CHAR);

	//fprintf(stderr,"FS ON START = %d\n",startslash);
	//fprintf(stderr,"FS ON END   = %d\n",endslash);
	//fprintf(stderr,"FIRST CHAR = %c\n",fp->path[0]);

	home = ospath_gethomepath();

	// create a copy of fp->path.
	STRCPY(path, fp->path);

	// reset fp->path as required.
	STRCPY(fp->path, (startslash ? PATH_SEPARATOR_STR : ""));

	X(path);

	// split path into it tokens, using STRTOK which is NOT reentrant
	// so be careful!

	//M("STARTING STRTOK");
	for(p = STRTOK(path, PATH_SEPARATOR_STR,nexttok);
			p!=NULL;
			p = STRTOK(NULL,PATH_SEPARATOR_STR,nexttok)
	){
		//M("NEXT TOKEN");
		//X(p);
		//X(path+strlen(p)+1);
		if(strcmp(p, "~")==0){

			if(p == path){ // check that the ~ is the first character in the path
				if(ospath_isvalid(home)){
					ospath_copy(fp,home);
					continue;
				}else{
					E("HOME does not resolve to valid path");
				}
			}else{
				E("A tilde (~) present as a component in a file path must be at the start!");
			}
		}else if(strcmp(p, ".") == 0){

			if(p==path){// start of path:
				M("EXPANDING LEADING '.' IN PATH");
				X(path);

				working = ospath_getcwd();

				D(working);
#ifdef WINPATHS
				X(working->drive);
#endif
				X(p);
				X(path);

				ospath_copy(fp,working);


				D(fp);
				X(p);
				//X(path+strlen(p)+1);

				//ospath_free(working);
				continue;
			}else{// later in the path: just skip it
				M("SKIPPING '.' IN PATH");
				continue;
			}

		}else if(strcmp(p, "..") == 0){
			M("GOING TO PARENT");
			parent = ospath_getparent(fp);
			if(ospath_isvalid(parent)){
				ospath_copy(fp,parent);
			}
			//ospath_free(parent);
			continue;
		}

		// add a separator if we've already got some stuff
		if(
			strlen(fp->path) > 0
			&& fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR
		){
			STRCAT(fp->path,PATH_SEPARATOR_STR);
		}

		// add the present path component
		STRCAT(fp->path, p);
	}
	//M("FINISHED STRTOK");

	// put / on end as required, according to what the starting path had
	if(endslash && (strlen(fp->path) > 0 ? (fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR) : 1))
	{
		STRCAT(fp->path, PATH_SEPARATOR_STR);
	}
}


int ospath_isvalid(struct FilePath *fp){
	//if(fp==NULL) return 0;
	return strlen(fp->path) > 0 ? 1 : 0;
}


char *ospath_str(struct FilePath *fp){
	char *s;
#ifdef WINPATHS
	s = CALLOC(strlen(fp->drive)+strlen(fp->path),sizeof(char));
	STRCPY(s,fp->drive);
	STRCAT(s,fp->path);
#else
	s = CALLOC(strlen(fp->path),sizeof(char));
	STRCPY(s,fp->path);
#endif
	return s;
}

void ospath_strcpy(struct FilePath *fp, char *dest, int destsize){
#ifdef WINPATHS
	STRNCPY(dest,fp->drive,destsize);
	STRNCAT(dest,fp->path,destsize-strlen(dest));
#else
	STRNCPY(dest,fp->path,destsize);
#endif
}

void ospath_fwrite(struct FilePath *fp, FILE *dest){
#ifdef WINPATHS
	fprintf(dest,"%s%s",fp->drive,fp->path);
#else
	fprintf(dest,"%s",fp->path);
#endif
}

unsigned ospath_length(struct FilePath *fp){
#ifdef  WINPATHS
	// we've already validated this path, so it's on to just add it up
	// (unless someone has been tinkering with the internal structure here)
	return (unsigned) (strlen(fp->drive) + strlen(fp->path));
#else
	return (unsigned) (strlen(fp->path));
#endif
}

struct FilePath *ospath_getparent(struct FilePath *fp)
{
	int length;
	int offset;
	char *pos;
	int len1;
	char sub[PATH_MAX];
	struct FilePath *fp1;

	D(fp);

	if(strlen(fp->path) == 0 || ospath_isroot(fp))
	{
		// return empty path.
		return ospath_new("");
	}

	// reverse find a / ignoring the end / if it exists.
	/// FIXME
	length = strlen(fp->path);
	offset = (
			fp->path[length - 1] == PATH_SEPARATOR_CHAR // last char is slash?
			&& length > 1 // and more than just leading slash...
		) ? length - 1 : length; // then remove last char

	for(pos = fp->path + offset - 1; *pos!=PATH_SEPARATOR_CHAR && pos>=fp->path; --pos){
		//fprintf(stderr,"CURRENT CHAR: %c\n",*pos);
	}

	len1 = (int)pos - (int)fp->path;
	//fprintf(stderr,"POS = %d\n",len1);

	if(*pos==PATH_SEPARATOR_CHAR){
#ifdef WINPATHS
		STRCPY(sub,fp->drive);
		STRNCAT(sub,fp->path,len1);
#else
		STRNCPY(sub,fp->path,len1);
		sub[len1]='\0';
#endif
		X(sub);
		if(strcmp(sub,"")==0){
			M("DIRECTORY IS EMPTY");
			STRCAT(sub,PATH_SEPARATOR_STR);
		}
	}else{
		E("NO PARENT DIR");
		return ospath_new_noclean(fp->path);
	}

	fp1 = ospath_new_noclean(sub);
	D(fp1);
	return fp1;
}

struct FilePath *ospath_getparentatdepthn(struct FilePath *fp, unsigned depth)
{
	int startslash;
	char path[PATH_MAX];
	char *temp;
	char *p;
	STRTOKVAR(nexttok);
#ifdef WINPATHS
	char temp2[PATH_MAX];
#endif

	if(
		!ospath_isvalid(fp)
		|| depth >= ospath_depth(fp)
	){
		return fp;
	}

	// create FilePath object to parent object at depth N relative to this
	// path object.
	startslash = (strlen(fp->path) > 0 && fp->path[0] == PATH_SEPARATOR_CHAR);

	// create a copy of fp->path.
	STRCPY(path, fp->path);

	// reset fp->path as required.
	temp = startslash ? PATH_SEPARATOR_STR : "";

	// split path into it tokens.
	//M("STARTING STRTOK");
	p = STRTOK(path, PATH_SEPARATOR_STR, nexttok);

	while(p && depth > 0)
	{
		if(strlen(temp) > 0 && temp[strlen(temp) - 1] != PATH_SEPARATOR_CHAR)
		{
			strcat(temp,PATH_SEPARATOR_STR);
		}

		STRCAT(temp,p);
		--depth;

		p = STRTOK(NULL, PATH_SEPARATOR_STR, nexttok);
	}
	//M("FINISHED STRTOK");

	// put / on end as required
	if(strlen(temp) > 0 ? (temp[strlen(temp) - 1] != PATH_SEPARATOR_CHAR) : 1)
	{
		strcat(temp,PATH_SEPARATOR_STR);
	}

#ifdef WINPATHS
	STRCPY(temp2,fp->drive);
	STRCAT(temp2,temp);
	return ospath_new_noclean(temp2);
#else
	return ospath_new_noclean(temp);
#endif
}

char *ospath_getbasefilename(struct FilePath *fp){
	char *temp;
	unsigned length, offset;
	char *pos;

	if(strlen(fp->path) == 0){
		// return empty name.
		return "";
	}

	if(fp->path[strlen(fp->path)-1]==PATH_SEPARATOR_CHAR){
		return NULL;
	}

	// reverse find '/' but DON'T ignore a trailing slash
	// (this is changed from the original implementation)
	length = strlen(fp->path);
	offset = length;

	pos = strrchr(fp->path, PATH_SEPARATOR_CHAR); /* OFFSET! */

	// extract filename given position of find / and return it.
	if(pos != NULL){
		unsigned length1 = length - ((pos - fp->path) + 1);
		temp = CALLOC(length1,sizeof(char));
		
		V(length1);
		STRNCPY(temp, pos + 1, length1);
		return temp;
	}else{
		temp = CALLOC(length,sizeof(char));
		STRNCPY(temp, fp->path, length);
		return temp;
	}
}

char *ospath_getfilestem(struct FilePath *fp){
	char *temp;
	char *pos;

	if(!ospath_isvalid(fp)){
		return NULL;
	}

	temp = ospath_getbasefilename(fp);
	if(temp==NULL){
		// it's a directory
		return NULL;
	}

	pos = strrchr(temp,'.');

	if(pos==NULL || pos==temp){
		// no extension, or a filename starting with '.'
		// -- return the whole filename
		return temp;
	}

	// remove extension.
	*pos = '\0';

	return temp;
}

char *ospath_getfileext(struct FilePath *fp){
	char *temp, *temp2, *pos;
	int len1;

	if(!ospath_isvalid(fp)){
		return NULL;
	}

	temp = ospath_getbasefilename(fp);
	if(temp==NULL){
		// it's a directory
		return NULL;
	}

	// make sure there is no / on the end.
	/// FIXME: is this good policy, removing a trailing slash?
	if(temp[strlen(temp) - 1] == PATH_SEPARATOR_CHAR){
		temp[strlen(temp)-1] = '\0';
	}

	pos = strrchr(temp,'.');

	if(pos != NULL && pos!=temp){
		// extract extension.
		len1 = temp + strlen(temp) - pos;
		temp2 = CALLOC(len1, sizeof(char));
		STRNCPY(temp2, pos, len1);
	}else{
		// no extension
		temp2 = NULL;
	}
	FREE(temp);
	return temp2;
}

int ospath_isroot(struct FilePath *fp)
{
	if(!ospath_isvalid(fp))
	{
		return 0;
	}

	return fp->path == PATH_SEPARATOR_STR ? 1 : 0;
}

unsigned ospath_depth(struct FilePath *fp){
	unsigned length;
	unsigned depth;
	unsigned i;

	length = strlen(fp->path);
	depth = 0;

	for(i = 0; i < length; i++){
		if(fp->path[i] == PATH_SEPARATOR_CHAR){
			++depth;
		}
	}

	if(
		depth > 0
		&& length > 0
		&& fp->path[length - 1] == PATH_SEPARATOR_CHAR
	){
		// PATH_SEPARATOR_CHAR on the end, reduce count by 1
		--depth;
	}

	return depth;
}

struct FilePath *ospath_root(struct FilePath *fp){
#ifdef WINPATHS
	//M("WIN ROOT");
	char *temp;
	struct FilePath *r;

	if(strlen(fp->drive)){
		temp = CALLOC(strlen(fp->drive)+1, sizeof(char));
		STRCPY(temp,fp->drive);
		STRCAT(temp,PATH_SEPARATOR_STR);
		X(temp);
		r = ospath_new(temp);
		FREE(temp);
	}else{
		r = ospath_new(fp->path);
	}
	return r;
#else
	//M("JUST RETURNING PATH SEP");
	return ospath_new(PATH_SEPARATOR_STR);
#endif
}

struct FilePath *ospath_getdir(struct FilePath *fp){
	char *pos;
	char s[PATH_MAX];

	pos = strrchr(fp->path,PATH_SEPARATOR_CHAR);
	if(pos==NULL){
		return ospath_new(".");
	}
#ifdef WINPATHS
	strncpy(s,fp->drive,PATH_MAX);
	strncat(s,fp->path,pos - fp->path);
#else
	strncpy(s,fp->path,pos - fp->path);
#endif
	return ospath_new(s);
}

int ospath_cmp(struct FilePath *fp1, struct FilePath *fp2){
	char temp[2][PATH_MAX];
#ifdef WINPATHS
	char *p;
#endif

	if(!ospath_isvalid(fp1)){
		if(!ospath_isvalid(fp2)){
			return 0;
		}else{
			return -1;
		}
	}else if(!ospath_isvalid(fp2)){
		return 1;
	}

	// now, both are valid...
	//M("BOTH ARE VALID");

#ifdef WINPATHS
	//X(fp1->drive);
	STRCPY(temp[0],fp1->drive);
	//X(temp[0]);
	//X(fp1->path);
	STRCAT(temp[0],fp1->path);
	//X(temp[0]);
	STRCPY(temp[1],fp2->drive);
	STRCAT(temp[1],fp2->path);
#else
	STRCPY(temp[0], fp1->path);
	STRCPY(temp[1], fp2->path);
#endif

#ifdef WINPATHS
	for(p=temp[0];*p!='\0';++p){
		*p=tolower(*p);
	}
	for(p=temp[1];*p!='\0';++p){
		*p=tolower(*p);
	}
	X(temp[0]);
	X(temp[1]);
#endif

	// we will count two paths that different only in a trailing slash to be the *same*
	// so we add trailing slashes to both now:
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR){
		STRCAT(temp[0],PATH_SEPARATOR_STR);
	}

	if(temp[1][strlen(temp[1]) - 1] != PATH_SEPARATOR_CHAR){
		STRCAT(temp[1],PATH_SEPARATOR_STR);
	}

	//X(temp[0]);
	//X(temp[1]);

	return strcmp(temp[0],temp[1]);
}

struct FilePath *ospath_concat(struct FilePath *fp1, struct FilePath *fp2){

	struct FilePath *fp;
	char temp[2][PATH_MAX];
	char temp2[PATH_MAX];
	struct FilePath *r;

	fp = MALLOC(sizeof(struct FilePath));

	D(fp1);
	D(fp2);

	if(!ospath_isvalid(fp1)){
		if(ospath_isvalid(fp2)){
			ospath_copy(fp,fp2);
		}else{
			// both invalid
			ospath_copy(fp,fp1);
		}
		return fp;
	}

	if(!ospath_isvalid(fp2)){
		ospath_copy(fp,fp1);
		return fp;
	}

	// not just a copy of one or the other...
	ospath_free(fp);

	// now, both paths are valid...

#ifdef WINPATHS
	STRNCPY(temp[0],fp1->drive,PATH_MAX);
	STRNCAT(temp[0],fp1->path,PATH_MAX-strlen(temp[0]));
#else
	STRNCPY(temp[0], fp1->path,PATH_MAX);
#endif

	STRNCPY(temp[1], fp2->path,PATH_MAX);

	// make sure temp has a / on the end.
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR)
	{
		STRNCAT(temp[0],PATH_SEPARATOR_STR,PATH_MAX-strlen(temp[0]));
	}

#ifdef DO_FIXSLASHES
	ospath_fixslash(temp[0]);
	ospath_fixslash(temp[1]);
#endif

	//V(strlen(temp[0]));
	//X(temp[0]);
	//V(strlen(temp[1]));
	//X(temp[1]);

	// make sure rhs path has NOT got a / at the start.
	if(temp[1][0] == PATH_SEPARATOR_CHAR){
		return NULL;
	}

	// create a new path object with the two path strings appended together.
	STRNCPY(temp2,temp[0],PATH_MAX);
	STRNCAT(temp2,temp[1],PATH_MAX-strlen(temp2));
	//V(strlen(temp2));
	//X(temp2);
	r = ospath_new_noclean(temp2);
	D(r);
	/* ospath_cleanup(r);*/
	return r;
}

void ospath_append(struct FilePath *fp, struct FilePath *fp1){
	char *p;
	char *temp[2];
	struct FilePath fp2;

	ospath_copy(&fp2,fp1);
#ifdef DO_FIXSLASHES
	ospath_fixslash(fp2.path);
#endif

	if(!ospath_isvalid(&fp2)){
		M("fp1 invalid");
		return;
	}

	if(!ospath_isvalid(fp) && ospath_isvalid(&fp2)){
		// set this object to be the same as the rhs
		M("fp invalid");
		ospath_copy(fp,&fp2);
#ifdef DO_FIXSLASHES
		ospath_fixslash(fp->path);
#endif
		return;
	}

	//X(fp->path);
	//X(fp2.path);

	// both paths are valid...
	temp[0] = CALLOC(1+strlen(fp->path), sizeof(char));
	STRCPY(temp[0], fp->path);
	temp[1] = CALLOC(strlen(fp2.path), sizeof(char));
	STRCPY(temp[1], fp2.path);

	//X(temp[0]);
	//X(temp[1]);

	// make sure temp has a / on the end.
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR)
	{
		STRCAT(temp[0],PATH_SEPARATOR_STR);
	}

	// make sure rhs path has NOT got a / at the start.
	if(temp[1][0] == PATH_SEPARATOR_CHAR){
		for(p=temp[1]+1; *p != '\0'; ++p){
			*(p-1)=*p;
		}
		*(p-1)='\0';
	}

	//X(temp[0]);
	//X(temp[1]);

	// create new path string.
	STRCPY(fp->path,temp[0]);
	STRCAT(fp->path,temp[1]);

	FREE(temp[0]);
	FREE(temp[1]);

	X(fp);
	ospath_cleanup(fp);
}

void ospath_copy(struct FilePath *dest, struct FilePath *src){
	STRCPY(dest->path,src->path);
#ifdef WINPATHS
	STRCPY(dest->drive,src->drive);
#endif
}

void ospath_debug(struct FilePath *fp){
#ifdef WINPATHS
	fprintf(stderr,"{\"%s\",\"%s\"}\n",fp->drive,fp->path);
#else
	fprintf(stderr,"{\"%s\"}\n",fp->path);
#endif
}

FILE *ospath_fopen(struct FilePath *fp, const char *mode){
	char s[PATH_MAX];
	if(!ospath_isvalid(fp)){
		E("Invalid path");
		return NULL;
	}
	ospath_strcpy(fp,s,PATH_MAX);
	FILE *f = fopen(s,mode);
	return f;
}

//------------------------
// SEARCH PATH FUNCTIONS

struct FilePath **ospath_searchpath_new(const char *path){
	char *p;
	char *list[LISTMAX];
	unsigned n=0;
	char *c;
	unsigned i;
	struct FilePath **pp;
	STRTOKVAR(nexttok);

	char path1[PATH_MAX];
	strncpy(path1,path,PATH_MAX);

	X(path1);
	X(PATH_LISTSEP_STR);

	V(strlen(path1));
	V(strlen(PATH_LISTSEP_STR));

	/*
	c = strstr(path,PATH_LISTSEP_CHAR);
	if(c==NULL){
		E("NO TOKEN FOUND");
	}
	*/

	p=STRTOK(path1,PATH_LISTSEP_STR,nexttok);
	X(p);
	for(; p!= NULL; p=STRTOK(NULL,PATH_LISTSEP_STR,nexttok)){
		c = CALLOC(strlen(p),sizeof(char));
		X(p);
		STRCPY(c,p);
		if(n>=LISTMAX){
			E("IGNORING SOME PATH COMPONENTS");
			break;
		}
		list[n++]=c;
	}

	/*
	for(i=0;i<n;++i){
		X(list[i]);
	}
	V(n);
	*/

	pp = CALLOC(n+1,sizeof(struct FilePath*));
	for(i=0; i<n; ++i){
		//V(i);
		//X(list[i]);
		pp[i] = ospath_new_noclean(list[i]);
		//D(pp[i]);
	}
	pp[n] = NULL;

	for(i=0;i<n;++i){
#ifdef DO_FIXSLASHES
		ospath_fixslash(pp[i]->path);
#endif
		D(pp[i]);
	}

	return pp;
}

void ospath_searchpath_free(struct FilePath **searchpath){
	struct FilePath **p;
	for(p=searchpath; *p!=NULL; ++p){
		ospath_free(*p);
	}
	FREE(searchpath);
}

struct FilePath *ospath_searchpath_iterate(
		struct FilePath **searchpath
		, FilePathTestFn *testfn
		, void *searchdata
){
	struct FilePath **p;

	p = searchpath;

	M("SEARCHING IN...");
	for(p=searchpath; *p!=NULL; ++p){
		D(*p);
	}
	
	for(p=searchpath; *p!=NULL; ++p){
		D(*p);
		if((*testfn)(*p,searchdata)){
			return *p;
		}
	}
	return NULL;
}


/*--------------------------------
	some simple test routines...
*/
#ifdef TEST

FilePathTestFn ospath_searchpath_testexists;

/**
	Return 1 if the file exists relative inside path
*/
int ospath_searchpath_testexists(struct FilePath *path,void *file){
	struct FilePath *fp, *fp1, *fp2;
	fp = (struct FilePath *)file;
	D(fp);
	fp1 = ospath_concat(path,fp);
	D(fp1);

	fp2 = ospath_new("\\GTK\\bin\\johnpye\\extfn");
	if(ospath_cmp(fp1,fp2)==0){
		return 1;
	}
	return 0;
}

#include <assert.h>

int main(void){
	struct FilePath *fp1, *fp2, *fp3, *fp4;
	char *s1, *s2;
	struct FilePath **pp, **p1;// will be returned null-terminated
#ifdef WINPATHS
	char pathtext[]="c:\\Program Files\\GnuWin32\\bin;c:\\GTK\\bin;e:\\ascend\\;..\\..\\pygtk";
	char pathtext2[]="c:\\Program Files\\ASCEND\\models";
#else
	char pathtext[]="\\Program Files\\GnuWin32\\bin:\\GTK\\bin:\\ascend\\:..\\..\\pygtk";
	char pathtext2[]="/usr/local/ascend/models";
#endif

	//------------------------

	fp1 = ospath_new_from_posix("/usr/local/hello/");
	fp2 = ospath_getparent(fp1);
	fp3 = ospath_new_from_posix("/usr/local");

	D(fp1);
	D(fp2);
	D(fp3);
	assert(ospath_cmp(fp2,fp3)==0);
	M("Passed 'getparent' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3);

	//------------------------

	fp1 = ospath_new_from_posix("models/johnpye/extfn/extfntest");
	D(fp1);
	fp2 = ospath_new("models\\johnpye\\extfn\\extfntest");
	D(fp2);
	D(fp1);
	assert(ospath_cmp(fp1,fp2)==0);
	M("Passed 'new_from_posix' test\n");

	ospath_free(fp1);
	ospath_free(fp2);

	//------------------------
	fp1 = ospath_new(".\\src/.\\images\\..\\\\movies\\");
	fp2 = ospath_new(".\\src\\movies");

	D(fp1);
	D(fp2);

	assert(ospath_cmp(fp1,fp2)==0);
	M("Passed mid-path '..' cleanup test\n");

	ospath_free(fp2);

	fp2 = ospath_new("./src/movies\\kubrick");
	fp3 = ospath_getparent(fp2);

	D(fp2);
	D(fp3);

	assert(ospath_cmp(fp1,fp3)==0);
	M("Passed 'second cleanup' test\n");

	//------------------------

	fp2 = ospath_new("\\home\\john");
	fp3 = ospath_new("where\\mojo");

	D(fp2);
	D(fp3);

	ospath_append(fp2,fp3);

	D(fp2);

	fp4 = ospath_new("\\home\\john\\where\\mojo\\");

	D(fp2);
	assert(ospath_cmp(fp2,fp4)==0);
	M("Passed 'append' test\n");

	ospath_free(fp3);
	ospath_free(fp2);

	//---------------------------

	fp3 = ospath_new_noclean("../..");
	D(fp3);

	// test with appending ../.. to an existing path
	fp2 = ospath_new("\\home\\john");
	M("ORIGINAL PATH");
	D(fp2);
	ospath_append(fp2,fp3);
	M("AFTER APPENDING ../..");
	D(fp2);

	M("GETTING ROOT");
	fp4 = ospath_root(fp2);
	M("ROOT FOUND:");
	D(fp4);

	assert(ospath_cmp(fp2,fp4)==0);
	M("Passed 'append ../..' test\n");

	ospath_free(fp2);
	ospath_free(fp3);
	ospath_free(fp4);

	//-------------------------

	fp1 = ospath_new("~\\somewhere\\..");
	fp2 = ospath_new("~/.");

	assert(ospath_cmp(fp1,fp2)==0);

	D(fp2);

	ospath_free(fp1);
	ospath_free(fp2);

	fp1 = ospath_new("/usr/local/include");
	fp2 = ospath_new("/usr/include/../local/include");

	D(fp1);
	D(fp2);

	assert(ospath_cmp(fp1,fp2)==0);
	M("Passed another mid-path '..' test\n");

	ospath_free(fp1);
	ospath_free(fp2);

	//---------------------------

	fp1 = ospath_new("/home");
	fp2 = ospath_new("john");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("/home/john");

	assert(ospath_cmp(fp3,fp4)==0);
	M("Passed 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);

	//---------------------------

	fp1 = ospath_new("c:/Program Files");
	fp2 = ospath_new("GnuWin32\\bin");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin");

	assert(ospath_cmp(fp3,fp4)==0);
	M("Passed 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);

	//---------------------------

	fp1 = ospath_new("c:/Program Files/");
	fp2 = ospath_new("GnuWin32\\bin");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin");

	assert(ospath_cmp(fp3,fp4)==0);
	M("Passed trailing-slash 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);

	//---------------------------

	fp1 = ospath_new("c:/Program Files/GnuWin32/bin");
	fp2 = ospath_new("johnpye/extfn");
	fp3 = ospath_concat(fp1, fp2);

	fp4 = ospath_new("c:/Program Files/GnuWin32/bin/johnpye/extfn");

	assert(ospath_cmp(fp3,fp4)==0);
	M("Passed trailing-slash 'ospath_concat' test\n");

	ospath_free(fp1); ospath_free(fp2); ospath_free(fp3); ospath_free(fp4);

	//---------------------------

	pp = ospath_searchpath_new(pathtext);

	for(p1=pp; *p1!=NULL; ++p1){
		D(*p1);
	}

#ifdef WINPATHS
	fp1 = ospath_new("c:\\program files\\GnuWin32\\bin");
#else
	fp1 = ospath_new("\\Program Files\\GnuWin32\\bin");
#endif

	D(fp1);
	D(pp[0]);

	assert(ospath_cmp(pp[0],fp1)==0);

	fp2 = ospath_new_noclean("johnpye/extfn");

	fp3 = ospath_searchpath_iterate(pp,&ospath_searchpath_testexists,(void*)fp2);

	assert(ospath_cmp(fp3,pp[1])==0);
	M("Passed path-search test\n");

	ospath_free(fp1);	
	ospath_free(fp2);
	ospath_searchpath_free(pp);

	//-------------------------------

	pp = ospath_searchpath_new(pathtext2);

	for (p1=pp; *p1!=NULL; ++p1){
		D(*p1);
	}

	fp2 = ospath_new_noclean("johnpye/extfn/extfntest");
	fp3 = ospath_searchpath_iterate(pp,&ospath_searchpath_testexists,(void*)fp2);

	D(fp2);
	D(fp3);

	assert(fp3==NULL);
	M("Passed path-search test 2\n");

	ospath_free(fp2);
	ospath_free(fp3);
	ospath_searchpath_free(pp);
	
	//-------------------------------

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	assert(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	assert(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test 2\n");

	ospath_free(fp1);
	FREE(s1);


	//-------------------------------

	fp1 = ospath_new("/here/is/my/path.dir/");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	assert(NULL==s1);
	M("Passed getbasefilename test 3\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

#ifdef WINPATHS
	fp1 = ospath_new("c:extfntest.a4c");
	D(fp1);
	s1 = ospath_getbasefilename(fp1);
	X(s1);
	assert(strcmp(s1,"extfntest.a4c")==0);
	M("Passed getbasefilename test WINPATHS\n");

	ospath_free(fp1);
	FREE(s1);
#endif

	//-------------------------------

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest.a4c");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(strcmp(s1,"extfntest")==0);
	M("Passed getfilestem test\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("/usr/share/data/ascend/models/johnpye/extfn/extfntest");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(strcmp(s1,"extfntest")==0);
	M("Passed getfilestem test 2\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("/usr/share/data/ascend/.ascend.ini");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(strcmp(s1,".ascend")==0);
	M("Passed getfilestem test 3\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("~/.vimrc");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(strcmp(s1,".vimrc")==0);
	M("Passed getfilestem test 3\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("~/src/ascend-0.9.5-1.jdpipe.src.rpm");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(strcmp(s1,"ascend-0.9.5-1.jdpipe.src")==0);
	M("Passed getfilestem test 4\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("~/dir1/dir2/");
	D(fp1);
	s1 = ospath_getfilestem(fp1);
	X(s1);
	assert(NULL==s1);
	M("Passed getfilestem test 5\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("~/src/ascend-0.9.5-1.jdpipe.src.rpm");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	assert(strcmp(s1,".rpm")==0);
	M("Passed getbasefileext test\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("~/.vimrc");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	assert(s1==NULL);
	M("Passed getbasefileext test 2\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("./ascend4");
	D(fp1);
	s1 = ospath_getfileext(fp1);
	X(s1);
	assert(s1==NULL);
	M("Passed getbasefileext test 3\n");

	ospath_free(fp1);
	FREE(s1);

	//-------------------------------

	fp1 = ospath_new("/home/myfile");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home");
	assert(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);

	//-------------------------------

	fp1 = ospath_new("/home/myfile.ext");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home");
	assert(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test 2\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);

	//-------------------------------

	fp1 = ospath_new("/home/mydir/");
	fp2 = ospath_getdir(fp1);
	fp3 = ospath_new("/home/mydir");
	assert(ospath_cmp(fp2,fp3)==0);
	M("Passed ospath_getdir test 3\n");

	ospath_free(fp1);
	ospath_free(fp2);
	ospath_free(fp3);

	//---------------------------------	
	M("ALL TESTS PASSED");
}

#endif
