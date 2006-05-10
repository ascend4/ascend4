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
*/

#include "env.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(TEST) && !defined(VERBOSE)
# define NDEBUG
#endif

#ifndef NDEBUG
# include <assert.h>
# define M(MSG) fprintf(stderr,"%s:%d: (%s) %s\n",__FILE__,__LINE__,__FUNCTION__,MSG);fflush(stderr)
# define MC(CLR,MSG) fprintf(stderr,"\033[%sm%s:%d: (%s) %s\033[0m\n",CLR,__FILE__,__LINE__,__FUNCTION__,MSG)
# define MM(MSG) MC("34",MSG)
# define X(VAR) fprintf(stderr,"%s:%d: (%s) %s=%s\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
# define XC(CLR,VAR) fprintf(stderr,"\033[%sm%s:%d: (%s) %s=%s\033[0m\n",CLR,__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
# define C(VAR) fprintf(stderr,"%s:%d: (%s) %s=%c\n",__FILE__,__LINE__,__FUNCTION__,#VAR,VAR)
# define V(VAR) fprintf(stderr,"%s:%d: (%s) %s=%d\n",__FILE__,__LINE__,__FUNCTION__,#VAR,(VAR))
# define D(VAR) fprintf(stderr,"%s:%d: (%s) %s=",__FILE__,__LINE__,__FUNCTION__,#VAR);ospath_debug(VAR)
# define DD(VAR) fprintf(stderr,"%c[34;1m%s:%d: (%s)%c[0m %s=",27,__FILE__,__LINE__,__FUNCTION__,27,#VAR);ospath_debug(VAR)
#else
# include <assert.h>
# define M(MSG) ((void)0)
# define MC(CLR,MSG) ((void)0)
# define X(VAR) ((void)0)
# define XC(CLR,VAR) ((void)0)
# define C(VAR) ((void)0)
# define V(VAR) ((void)0)
# define D(VAR) ((void)0)
# define DD(VAR) ((void)0)
# define MM(VAR) ((void)0)
#endif

#if !defined(FREE) && !defined(MALLOC)
# define FREE free
# define MALLOC malloc
#endif

#define ENV_MAX_VAR_NAME 64 // arbitrarily

char * env_subst_level(const char *path,GetEnvFn *getenvptr, int level);

char * env_subst(const char *path,GetEnvFn *getenvptr){
	char *dest;

	X(path);

	// no substitution required
	if(getenvptr==NULL){
		dest = MALLOC(sizeof(char) * (strlen(path) + 1));
		strcpy(dest,path);
		return dest;
	}

	return env_subst_level(path,getenvptr, 0);
}

char * env_subst_level(const char *path,GetEnvFn *getenvptr, int level){
	char *dest, *dest1;
	char *msg;
	char *p, *q, *i, *j, *val;
	char varname[ENV_MAX_VAR_NAME+1];
	int len, vallen, newlen;
	int copy_in_place;
	size_t L;
	len = strlen(path);

	dest = MALLOC(sizeof(char)*(strlen(path)+1));
	strcpy(dest,path);

	X(dest);
	V(len);

	// scan backwards for $
	for(p=dest+len-1; p>=dest; --p){
		C(*p);

		if(*p=='$'){
			M("FOUND DOLLAR SIGN");
			++p;
			for(i=p, j=varname; i<dest+len, j<varname+ENV_MAX_VAR_NAME; ++i,++j){
				//C(*i);
				if(!(
					(*i >= 'A' && *i < 'Z')
					|| (*i == '_')
				)){
					M("NON-VARNAME CHAR FOUND");
					break;
				}
				//M("ADDING TO VARNAME");
				*j=*i;
			}
			//M("COMPLETED VARNAME");
			*j='\0';
			X(varname);

			if(j==varname+ENV_MAX_VAR_NAME){
				FREE(dest);
				msg = "__VAR_NAME_TOO_LONG__";
				dest = MALLOC(sizeof(char)*(strlen(msg)+1));
				strcpy(dest,msg);
				return dest;
			}
			M("FOUND VAR NAME");
			X(varname);
			val = (*getenvptr)(varname);
			if(val==NULL){
				//replace with null
				q = --p;
				for(j=i; j<dest+strlen(varname); ++j, ++q){
					*q=*j;
					M(p);
				}
				*q='\0';
				M(p);
			}else{
				vallen=strlen(val);
				X(val);
				V(strlen(val));
				X(dest);
				V(strlen(dest));
				X(varname);
				V(strlen(varname));
				--p;
				V((i-p));
				C(*i);
				C(*p);

				if(vallen > (i-p)){
					copy_in_place = 0;
				}else{
					copy_in_place = 1;
				}

				if(copy_in_place){
					M("COPY_IN_PLACE");

					for(j=p, q=val; *q!='\0'; ++q, ++j){
						*j=*q;
					}

					for(q=i;q!='\0'; ++q, ++j){
						*j=*q;
					}

				}else{
					MC("1","COPY FROM DUPLICATE");
					newlen = strlen(dest)+vallen-(i-p);

					dest1 = MALLOC(sizeof(char)*(newlen+1));
					strcpy(dest1,dest);

					p = dest1 + (p - dest);

					X(p);
					X(i);
					X(dest1);

					for(j=p, q=val; *q!='\0'; ++q, ++j){
						C(*q);
						*j=*q;
					}
					X(p);

					X(dest);
					for(q=i;*q!='\0'; ++q, ++j){
						C(*q);
						*j=*q;
					}

					*j='\0';

					i = dest1 + (i - dest);

					/* throw away the old copy */
					FREE(dest);
					dest = dest1;

				}

				XC("34;1",dest);

				/* move to the the end of the just-inserted chars */
				p = i+1;
				C(*p);

			}

		}
	}
	M("DONE");
	return dest;
}

/*--------------------------------
	some simple test routines...
*/
#ifdef TEST

#include <assert.h>

// switch to boldface for messages in 'main'
#undef D
#define D DD
#undef M
#define M MM

int main(void){
	char s1[]="$ASCENDTK/bitmaps";
	char *r;

	M(s1);

	r = env_subst(s1,getenv);
	M(r);

	//assert(strcmp(r,"C:/msys/1.0/share/ascend/share")==0);
}

#endif
