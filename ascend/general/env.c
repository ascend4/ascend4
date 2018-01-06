/*	ASCEND modeling environment
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
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "env.h"
#include <ascend/utilities/error.h>
#include <ascend/general/ascMalloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#define ENV_DEBUG
#ifdef ENV_DEBUG
# define MSG CONSOLE_DEBUG
#else
# define MSG(ARGS...) ((void)0)
#endif
#define X(V) MSG("%s=%s",#V,V)

#include <assert.h>

#if !defined(ASC_FREE) || !defined(ASC_NEW) || !defined(ASC_NEW_ARRAY)
# error "We should have ASC_FREE,ASC_NEW,ASC_NEW_ARRAY here...?"
# define ASC_FREE free
# define ASC_NEW(T) malloc(sizeof(T))
# define ASC_NEW_ARRAY(T,N) malloc((N)*sizeof(T))
#endif

#define ENV_MAX_VAR_NAME 64 /* arbitrarily*/

int env_import(const char *varname,GetEnvFn *getenvptr,PutEnvFn *putenvptr
		,int free_after_getenv
){
	char *val = (*getenvptr)(varname);
	char *envcmd;
	int res;
	int len;
	if(val!=NULL){
		len = strlen(varname) + 1 + strlen(val) + 1;
		envcmd = ASC_NEW_ARRAY(char,len);
		snprintf(envcmd,len,"%s=%s",varname,val);
		res = (*putenvptr)(envcmd);
		ASC_FREE(envcmd);
		if(free_after_getenv)ASC_FREE(val);
		return res;
	}
	return -1;
}


int env_import_default(const char *varname,GetEnvFn *getenvptr,PutEnvFn *putenvptr,const char *defaultvalue, int free_after_getenv){
	char *gotval = (*getenvptr)(varname);
	char *envcmd;
	int res;
	int len;
	const char *val = gotval;
	if(gotval==NULL)val = defaultvalue;
	len = strlen(varname) + 1 + strlen(val) + 1;
	envcmd = ASC_NEW_ARRAY(char,len);
	snprintf(envcmd,len,"%s=%s",varname,val);
	res = (*putenvptr)(envcmd);
	ASC_FREE(envcmd);
	if(NULL!=gotval && free_after_getenv)ASC_FREE(gotval);
	return res;
}


char *env_subst(const char *src,GetEnvFn *getenvptr,int free_after_getenv){
	char *res;

	MSG("src=%s",src);

	/* no substitution required */
	if(getenvptr==NULL){
		MSG("NO SUBST");
		res = ASC_NEW_ARRAY(char, strlen(src) + 1);
		strcpy(res,src);
		MSG("res=%s",res);
		return res;
	}

	char *dest, *dest1;
	char *msg;
	char *p, *q, *i, *j, *val;
	char varname[ENV_MAX_VAR_NAME+1];
	int len, vallen, newlen;
	len = strlen(src);

	dest = ASC_NEW_ARRAY(char, strlen(src)+1);
	strcpy(dest,src);

	X(dest);
	MSG("len=%d",len);

	/* scan backwards from end, looking for '$' */
	for(p=dest+len-1; p>=dest; --p){
		if(*p=='$'){
			MSG("found '$': dest=%s",dest);
			MSG("                %*s",(int)(p-dest+1),"^");
			++p;
			/* FIXME: note this is i = j = varname in C; p is ignored. */
			for(i=p, j=varname; i<dest+len && j<varname+ENV_MAX_VAR_NAME; ++i,++j){
				/*C(*i);*/
				if(!(
					(*i >= 'A' && *i < 'Z')
					|| (*i == '_')
				)){
					MSG("non-varname char '%c' found",*i);
					break;
				}
				/*M("ADDING TO VARNAME");*/
				*j=*i;
			}
			/*M("COMPLETED VARNAME");*/
			*j='\0';
			if(j==varname+ENV_MAX_VAR_NAME){
				MSG("varname '%s' too long",varname);
				ASC_FREE(dest);
				msg = "__VAR_NAME_TOO_LONG__";
				dest = ASC_NEW_ARRAY(char, strlen(msg)+1);
				strcpy(dest,msg);
				return dest;
			}
			X(varname);
			val = (*getenvptr)(varname);
			if(val==NULL){
				/* varname was null, just remove the varname from dest */
				MSG("remove empty varname ${%s}",varname);
				MSG("varname=%s",varname);
				MSG("strlen(varname)=%lu",strlen(varname));
				X(p);
				q = --p;
				X(q);
				X(i);
				for(j=i; j<i+strlen(i); ++j, ++q){
					*q=*j;
					X(p);
					X(q);
				}
				*q='\0';
				X(p);
			}else{
				MSG("substitute $%s with '%s'",varname,val);
				vallen=strlen(val);
				X(val);
				MSG("strlen(val)=%lu",strlen(val));
				X(dest);
				MSG("strlen(dest)=%lu",strlen(dest));
				X(varname);
				MSG("strlen(varname)=%lu",strlen(varname));
				--p;
				MSG("i-p=%ld",i-p);
				MSG("*i=%c",*i);
				MSG("*p=%c",*p);

				if(vallen > (i-p)){
					MSG("copy from duplicate");
					newlen = strlen(dest)+vallen-(i-p);

					dest1 = ASC_NEW_ARRAY(char, newlen+1);
					strcpy(dest1,dest);

					p = dest1 + (p - dest);

					X(p);
					X(i);
					X(dest1);

					for(j=p, q=val; *q!='\0'; ++q, ++j){
						MSG("*q='%c'",*q);
						*j=*q;
					}
					X(p);

					X(dest);
					for(q=i;*q!='\0'; ++q, ++j){
						MSG("*q='%c'",*q);
						*j=*q;
					}

					*j='\0';

					i = dest1 + (i - dest);

					/* throw away the old copy */
					ASC_FREE(dest);
					dest = dest1;
				}else{
					MSG("copy in place");

					for(j=p, q=val; *q!='\0'; ++q, ++j){
						*j=*q;
					}

					X(p);
					X(i);
					MSG("*j='%c'",*j);

					for(q=i;*q!='\0'; ++q, ++j){
						*j=*q;
						MSG("*q='%c'",*q);
					}
					*j='\0';
				}

				MSG("dest=\"%s\"",dest);

				/* move to the the end of the just-inserted chars */
				p = i+1;
				MSG("*p='%c'",*p);

				if(free_after_getenv)ASC_FREE(val);
			}
		}
	}
	MSG("DONE");
	return dest;
}

/* test code has been moved to test/test_env.c */

