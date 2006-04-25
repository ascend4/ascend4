#include <stdio.h>
#include <ctype.h>

#include "ospath.h"

#if defined(__WIN32__) /* && !defined(__MINGW32__) */
# define WINPATHS
#endif

struct FilePath{
    char path[PATHMAX]; /// the string version of the represented POSIX path

#ifdef WINPATHS
    char drive[2]; /// the drive the path resides on (field is absent in POSIX systems)
#endif
};

#include <string.h>

#define MALLOC malloc
#define FREE free
#define CALLOC calloc

#define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG)
#define E(MSG) fprintf(stderr,"%s:%d: (%s) ERROR: %s\n",__FILE__,__LINE__,__FUNCTION__,MSG)
#define X(VAR) fprintf(stderr,"%s:%d: (%s) %s=%s\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
#define V(VAR) fprintf(stderr,"%s:%d: (%s) %s=%d\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
#define D(VAR) ospath_debug(VAR,#VAR)

/**
	Static function that can be used to retrieve the current users HOME path
*/
struct FilePath *ospath_gethomepath(void);

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
#else
# define PATH_SEPARATOR_STR "/"
# define PATH_SEPARATOR_CHAR '/'
#endif

/**
	Create a new path structure from a string
*/
struct FilePath *ospath_new(const char *path){
	struct FilePath *fp;
	fp = MALLOC(sizeof(struct FilePath));
	X(path);
	strcpy(fp->path, path);

#ifdef WINPATHS
	X(fp->path);
	ospath_extractdriveletter(fp);
#endif

	ospath_cleanup(fp);

	return fp;
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
	char *p;
	strcpy(fp->path,posixpath);
#ifdef WINPATHS
	X(fp->path);
	ospath_extractdriveletter(fp);
#endif

	char path[PATHMAX];
	strcpy(path,fp->path);

	//X(path);

	int startslash = (strlen(path) > 0 && path[0] == '/');
	int endslash = (strlen(path) > 1 && path[strlen(path) - 1] == '/');

	//V(startslash);
	//V(endslash);

	// reset fp->path as required.
	strcpy(fp->path, (startslash ? PATH_SEPARATOR_STR : ""));

	for(p = strtok(path, "/");
			p!=NULL;
			p = strtok(NULL,"/")
	){
		// add a separator if we've already got some stuff
		if(
			strlen(fp->path) > 0
			&& fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR
		){
			strcat(fp->path,PATH_SEPARATOR_STR);
		}

		strcat(fp->path,p);
	}

	// put / on end as required, according to what the starting path had
	if(endslash && (strlen(fp->path) > 0 ? (fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR) : 1))
	{
		//M("adding endslash!");

		strcat(fp->path, PATH_SEPARATOR_STR);
	}

	//X(fp->path);

	ospath_cleanup(fp);

	return(fp);
}

/**
	As for ospath_new but the 'cleanup' call is now optional
*/
struct FilePath *ospath_new_noclean(const char *path){
	struct FilePath *fp = MALLOC(sizeof(struct FilePath));
	strcpy(fp->path,path);

#ifdef WINPATHS
	X(fp->path);
	ospath_extractdriveletter(fp);
#endif

	return fp;
}

/**
	Use getenv() function to retrieve HOME path, or if not set, use
	the password database and try to retrieve it that way (???)
*/
struct FilePath *ospath_gethomepath(void){

	const char *pfx = getenv("HOME");

#ifndef __WIN32__
	if(pfx==NULL){
		struct passwd * pw = getpwuid(getuid());

		if(pw){
			pfx = pw -> pw_dir;
		}
	}
#endif

	// create path object from HOME, but don't compress it!
	return ospath_new_noclean(pfx ? pfx : "");
}

#ifdef WINPATHS
void ospath_extractdriveletter(struct FilePath *fp)
{
	X(fp->path);
	// extract the drive the path resides on...
	if(strlen(fp->path) >= 2 && fp->path[1] == ':')
	{
		char driveletter = '\0';

		char firsttwo[2];
		strncpy(firsttwo,fp->path,2);

		if(sscanf(firsttwo, "%c:", &driveletter) == 1){
			strncpy(fp->drive,fp->path,2);
			fp->drive[0]=tolower(fp->drive[0]);
			strcpy(fp->path, fp->path+2);
		}else{
			E("WHY HERE?");
		}
	}else{
		strcpy(fp->drive,"");
	}
	X(fp->drive);
}
#endif

void ospath_cleanup(struct FilePath *fp){
	char *pBuff;
	char path[PATHMAX];
	char *p;
	struct FilePath *home;
	struct FilePath *workingPath;
	struct FilePath *parent;

	// compress the path, and resolve ~
	int startslash = (strlen(fp->path) > 0 && fp->path[0] == PATH_SEPARATOR_CHAR);
	int endslash = (strlen(fp->path) > 1 && fp->path[strlen(fp->path) - 1] == PATH_SEPARATOR_CHAR);

	//fprintf(stderr,"FS ON START = %d\n",startslash);
	//fprintf(stderr,"FS ON END   = %d\n",endslash);
	//fprintf(stderr,"FIRST CHAR = %c\n",fp->path[0]);

	home = ospath_gethomepath();

	// create a copy of fp->path.
	strcpy(path, fp->path);

	// reset fp->path as required.
	strcpy(fp->path, (startslash ? PATH_SEPARATOR_STR : ""));

	//X(fp->path);

	// split path into it tokens, using strtok which is NOT reentrant
	// so be careful!

	for(p = strtok(path, PATH_SEPARATOR_STR);
			p!=NULL;
			p = strtok(NULL,PATH_SEPARATOR_STR)
	){
		//X(p);
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
				// get current working directory
				pBuff = (char *)getcwd(NULL, 0);
				//X(pBuff);

				// create new path with resolved working directory
				workingPath = ospath_new_noclean(pBuff != NULL ? pBuff : ".");

				if(pBuff == NULL){
					E("Unable to resolve current working directory");
				}else{
					FREE(pBuff);
				}

				ospath_copy(fp,workingPath);
				FREE(workingPath);
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
			FREE(parent);
			continue;
		}

		// add a separator if we've already got some stuff
		if(
			strlen(fp->path) > 0
			&& fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR
		){
			strcat(fp->path,PATH_SEPARATOR_STR);
		}

		// add the present path component
		strcat(fp->path, p);
	}

	// put / on end as required, according to what the starting path had
	if(endslash && (strlen(fp->path) > 0 ? (fp->path[strlen(fp->path) - 1] != PATH_SEPARATOR_CHAR) : 1))
	{
		strcat(fp->path, PATH_SEPARATOR_STR);
	}

	FREE(parent);
	FREE(workingPath);
	FREE(home);
}


int ospath_isvalid(struct FilePath *fp){
	return strlen(fp->path) > 0 ? 1 : 0;
}


char *ospath_str(struct FilePath *fp){
	char *s;
#ifdef WINPATHS
	s = CALLOC(strlen(fp->drive)+strlen(fp->path),sizeof(char));
	strcpy(s,fp->drive);
	strcat(s,fp->path);
#else
	s = CALLOC(strlen(fp->path),sizeof(char));
	strcpy(s,fp->path);
#endif
	return s;
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
	if(strlen(fp->path) == 0 || ospath_isroot(fp))
	{
		// return empty path.
		return ospath_new("");
	}

	// reverse find a / ignoring the end / if it exists.
	int length = strlen(fp->path);
	int offset = (
			fp->path[length - 1] == PATH_SEPARATOR_CHAR
			&& length - 2 > 0
		) ? length - 2 : length ;
	char *pos = strrchr(fp->path,PATH_SEPARATOR_CHAR);

	// create new path object given position of find / and return it.
	if(pos != fp->path+length)
	{
		char temp[PATHMAX];
#ifdef WINPATHS
		strcpy(temp,fp->drive);
#endif
		strncat(temp,fp->path,(pos-fp->path)+1);
		return ospath_new_noclean(temp);
	}else{
		// not parent path avaliable, return an empty path.
		return ospath_new("");
	}
}

struct FilePath *ospath_getparentatdepthn(struct FilePath *fp, unsigned depth)
{
	if(
		!ospath_isvalid(fp)
		|| depth >= ospath_depth(fp)
	){
		return fp;
	}

	// create FilePath object to parent object at depth N relative to this
	// path object.
	int startslash = (strlen(fp->path) > 0 && fp->path[0] == PATH_SEPARATOR_CHAR);

	// create a copy of fp->path.
	char path[PATHMAX];
	strcpy(path, fp->path);

	// reset fp->path as required.
	char *temp = startslash ? PATH_SEPARATOR_STR : "";

	// split path into it tokens.
	char *p = strtok(path, PATH_SEPARATOR_STR);

	while(p && depth > 0)
	{
		if(strlen(temp) > 0 && temp[strlen(temp) - 1] != PATH_SEPARATOR_CHAR)
		{
			temp += PATH_SEPARATOR_CHAR;
		}

		strcat(temp,p);
		--depth;

		p = strtok(NULL, PATH_SEPARATOR_STR);
	}

	// put / on end as required
	if(strlen(temp) > 0 ? (temp[strlen(temp) - 1] != PATH_SEPARATOR_CHAR) : 1)
	{
		temp += PATH_SEPARATOR_CHAR;
	}

#ifdef WINPATHS
	char temp2[PATHMAX];
	strcpy(temp2,fp->drive);
	strcat(temp2,temp);
	return ospath_new_noclean(temp2);
#else
	return ospath_new_noclean(temp);
#endif
}

char *ospath_getbasefilename(struct FilePath *fp){
	char *temp;

	if(strlen(fp->path) == 0){
		// return empty name.
		return "";
	}

	// reverse find a / ignoring the end / if it exists.
	unsigned length = strlen(fp->path);
	unsigned offset = (
			fp->path[length - 1] == PATH_SEPARATOR_CHAR
			&& length - 2 > 0
		) ? length - 2
		: length;

	char *pos = strrchr(fp->path, PATH_SEPARATOR_CHAR); /* OFFSET! */

	// extract filename given position of find / and return it.
	if(pos != fp->path + length){
		int length1 = length - ((pos - fp->path) + 1) - (offset != length ? 1 : 0);
		temp = CALLOC(length1,sizeof(char));

		strncpy(temp, pos + 1, length1);
		return temp;
	}else{
		temp = CALLOC(length,sizeof(char));
		strncpy(temp, fp->path, length);
		return temp;
	}
}

char *ospath_getbasefiletitle(struct FilePath *fp){
	if(!ospath_isvalid(fp)){
		return NULL;
	}

	char *temp = ospath_getbasefilename(fp);
	char *pos = strrchr(temp,'.');

	if(pos != NULL){
		// remove extension.
		*pos = '\0';
	}

	return temp;
}

char *ospath_getbasefileextension(struct FilePath *fp){
	if(!ospath_isvalid(fp)){
		return NULL;
	}

	char *temp = ospath_getbasefilename(fp);
	char *temp2;

	// make sure there is no / on the end.
	if(temp[strlen(temp) - 1] == PATH_SEPARATOR_CHAR){
		temp[strlen(temp)-1] = '\0';
	}

	char *pos = strrchr(temp,'.');

	if(pos != NULL)
	{
		// extract extension.
		int len1 = temp + strlen(temp) - pos;
		temp2 = CALLOC(len1, sizeof(char));
		strncpy(temp2, pos, len1);
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
	char *temp;
	struct FilePath *r;
	if(strlen(fp->drive)){
		temp = CALLOC(strlen(fp->drive)+1, sizeof(char));
		strcpy(temp,fp->drive);
		strcat(temp,PATH_SEPARATOR_STR);
		r = ospath_new(temp);
		FREE(temp);
	}else{
		r = ospath_new(fp->drive);
	}
	return r;
#else
	return ospath_new(PATH_SEPARATOR_STR);
#endif
}

int ospath_cmp(struct FilePath *fp1, struct FilePath *fp2)
{
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

	char temp[2][PATHMAX];
#ifdef WINPATHS
	X(fp1->drive);
	strcpy(temp[0],fp1->drive);
	X(temp[0]);
	X(fp1->path);
	strcat(temp[0],fp1->path);
	X(temp[0]);
	strcpy(temp[1],fp2->drive);
	strcat(temp[1],fp2->path);
#else
	strcpy(temp[0], fp1->path);
	strcpy(temp[1], fp2->path);
#endif

	// we will count two paths that different only in a trailing slash to be the *same*
	// so we add trailing slashes to both now:
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR){
		strcat(temp[0],PATH_SEPARATOR_STR);
	}

	if(temp[1][strlen(temp[1]) - 1] != PATH_SEPARATOR_CHAR){
		strcat(temp[1],PATH_SEPARATOR_STR);
	}

	X(temp[0]);
	X(temp[1]);

	return strcmp(temp[0],temp[1]);
}

struct FilePath *ospath_concat(struct FilePath *fp1, struct FilePath *fp2){

	struct FilePath *fp;
	fp = MALLOC(sizeof(struct FilePath));

	if(!ospath_isvalid(fp1)){
		if(ospath_isvalid(fp2)){
			ospath_copy(fp2,fp);
		}else{
			// both invalid
			ospath_copy(fp1,fp);
		}
		return fp;
	}

	ospath_copy(fp1,fp);

	if(!ospath_isvalid(fp2)){
		return fp;
	}

	// not just a copy of one or the other...
	FREE(fp);

	// now, both paths are valid...

	char *temp[2];
#ifdef WINPATHS
	temp[0] = CALLOC(strlen(fp1->drive)+strlen(fp1->path), sizeof(char));
	strcpy(temp[0],fp1->drive);
	strcat(temp[0],fp1->path);
#else
	temp[0] = CALLOC(strlen(fp1->path), sizeof(char));
	strcpy(temp[0], fp1->path);
#endif
	temp[1] = CALLOC(strlen(fp2->path), sizeof(char));
	strcpy(temp[1], fp2->path);

	// make sure temp has a / on the end.
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR)
	{
		temp[0] += PATH_SEPARATOR_CHAR;
	}

	// make sure rhs path has NOT got a / at the start.
	if(temp[1][0] == PATH_SEPARATOR_CHAR){
		FREE(temp[0]);
		FREE(temp[1]);
		return NULL;
	}

	// create a new path object with the two path strings appended together.
	char *temp2;
	temp2 = CALLOC(strlen(temp[0])+strlen(temp[1]), sizeof(char));
	strcpy(temp2,temp[0]);
	strcat(temp2,temp[1]);
	struct FilePath *r;
	r = ospath_new_noclean(temp2);
	FREE(temp2);
	FREE(temp[0]);
	FREE(temp[1]);
	return r;
}

void ospath_append(struct FilePath *fp, struct FilePath *fp1){
	char *p;

	if(!ospath_isvalid(fp1)){
		M("fp1 invalid");
		return;
	}

	if(!ospath_isvalid(fp) && ospath_isvalid(fp1)){
		// set this object to be the same as the rhs
		M("fp invalid");
		ospath_copy(fp,fp1);
		return;
	}

	X(fp->path);
	X(fp1->path);

	// both paths are valid...
	char *temp[2];
	temp[0] = CALLOC(1+strlen(fp->path), sizeof(char));
	strcpy(temp[0], fp->path);
	temp[1] = CALLOC(strlen(fp1->path), sizeof(char));
	strcpy(temp[1], fp1->path);

	X(temp[0]);
	X(temp[1]);

	// make sure temp has a / on the end.
	if(temp[0][strlen(temp[0]) - 1] != PATH_SEPARATOR_CHAR)
	{
		strcat(temp[0],PATH_SEPARATOR_STR);
	}

	// make sure rhs path has NOT got a / at the start.
	if(temp[1][0] == PATH_SEPARATOR_CHAR){
		for(p=temp[1]+1; *p != '\0'; ++p){
			*(p-1)=*p;
		}
		*(p-1)='\0';
	}

	X(temp[0]);
	X(temp[1]);

	// create new path string.
	strcpy(fp->path,temp[0]);
	strcat(fp->path,temp[1]);

	FREE(temp[0]);
	FREE(temp[1]);
}

void ospath_copy(struct FilePath *dest, struct FilePath *src){
	strcpy(dest->path,src->path);
#ifdef WINPATHS
	strcpy(dest->drive,src->drive);
#endif
}

void ospath_debug(struct FilePath *fp, char *label){
	fprintf(stderr,"%s\n---------------------\n",label);
	fprintf(stderr,"PATH  = %s\n",fp->path);
#ifdef WINPATHS
	fprintf(stderr,"DRIVE = %s\n",fp->drive);
#endif
	fprintf(stderr,"\n");
}

/*--------------------------------
	some simple test routines...
*/
#ifdef TEST
#include <assert.h>

int main(void){
	struct FilePath *fp1, *fp2, *fp3, *fp4;

	fp1 = ospath_new_from_posix("models/johnpye/extfn/extfntest");
	D(fp1);
	fp2 = ospath_new("models\\johnpye\\extfn\\extfntest");
	D(fp2);
	assert(ospath_cmp(fp1,fp2)==0);
	M("Passed 'new_from_posix' test");

	FREE(fp1); FREE(fp2);

	//------------------------
	fp1 = ospath_new(".\\src\\.\\images\\..\\\\movies\\");
	fp2 = ospath_new(".\\src\\movies");

	D(fp1);
	D(fp2);

	assert(ospath_cmp(fp1,fp2)==0);
	M("Passed 'cleanup' test");

	FREE(fp2);

	fp2 = ospath_new(".\\src\\movies\\kubrick");
	fp3 = ospath_getparent(fp2);

	assert(ospath_cmp(fp1,fp3)==0);
	M("Passed 'parent' test");

	FREE(fp1); FREE(fp2); FREE(fp3);
	//------------------------

	fp2 = ospath_new("\\home\\john");
	fp3 = ospath_new("where\\mojo");

	D(fp2);
	D(fp3);

	ospath_append(fp2,fp3);

	fp4 = ospath_new("\\home\\john\\where\\mojo\\");

	D(fp2);
	assert(ospath_cmp(fp2,fp4)==0);
	M("Passed 'append' test");

	ospath_append(fp2,fp3);
	fp4 = ospath_root(fp2);
	D(fp2);
	D(fp4);

	assert(ospath_cmp(fp2,fp4)==0);
	M("Passed 'concat' test");

	D(fp2);

	FREE(fp2);

	fp2 = ospath_new("~/.");

	assert(ospath_cmp(fp1,fp2)==0);

	D(fp2);

	FREE(fp1);
	FREE(fp2);

	fp1 = ospath_new("/usr/local/include");
	fp2 = ospath_new("/usr/include/../local/include");

	D(fp1);
	D(fp2);


}

#endif
