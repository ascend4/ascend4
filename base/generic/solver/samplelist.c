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
*//*
	by John Pye, May 2006.
*/

#include "samplelist.h"
#include <utilities/ascMalloc.h>
#include <utilities/error.h>

/**
	Free memory allocated by a SampleList.
*/
void samplelist_free(SampleList *l){
	/* trash the stuff contained inside */

	if(l==NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"samplelist already freed!");
		return;
	}

	l->ns = 0L;
	if (l->x != NULL) {
		ASC_FREE(l->x);
		l->x = NULL;
	}

	/* now trash the struct itself */
	ASC_FREE(l);
}

/**
	Create an empty samplelist, and allocate memory for the required number
	of data points.

	@NOTE
	SampleList is assumed to be the owner of the contained
	data now, and will destroy the data when samplelist_free is
	called.
*/
SampleList *samplelist_new(unsigned long n, const dim_type *d){
	SampleList *l;
	double *values;

	l = ASC_NEW(SampleList);
	assert(n>0);
	values = ASC_NEW_ARRAY_CLEAR(double,n);
	l->ns = n;
	if(values==NULL){
		samplelist_free(l);
		return NULL;
	}
	if(!samplelist_assign(l,n,values,d)){
		samplelist_free(l);
		return NULL;
	}
	return l;
}

int samplelist_assign(SampleList *l, unsigned long n, double *values, const dim_type *d){
	/* store d given or copy and store WildDimension */
	if (d != NULL) {
		l->d = d;

#if SAMPLELIST_DEBUG
		FPRINTF(ASCERR,"sample received dimen\n");
		PrintDimen(ASCERR,l->d);
#endif

	} else {
		l->d = ASC_NEW(dim_type);
		if (l->d == NULL) {
			ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory.");
			return 0; /* out of memory */
		}
# if 0
		CopyDimensions(WildDimension(),l->d);
// copydim is not a reference copy, which is what is needed.
// persistent dimen references all come from the global
// dim table or Wild or Dimensionless.
#else
		l->d = WildDimension();
#endif 

#if SAMPLELIST_DEBUG
		FPRINTF(ASCERR,"copy of wild dimen looks like\n");
		PrintDimen(ASCERR,l->d);
#endif

	}

	l->ns = n;
	l->x = values;
	return 1; /* all ok */
}

long samplelist_length(CONST SampleList *l){
	return l->ns;
}


const dim_type *samplelist_dim(CONST SampleList *l){
	return l->d;
}

double samplelist_get(CONST SampleList *l, CONST long i){
	if(i>=0 && i < samplelist_length(l)){
		return l->x[i];
	}
    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Invalid sample index %ld."
            " Returning 0.", i);
    return 0.;	
}

void samplelist_set(CONST SampleList *l, CONST long i, CONST double x){
	if(i>=0 && i < samplelist_length(l)){
		l->x[i]=x;
		return;
	}

    ERROR_REPORTER_HERE(ASC_PROG_WARNING,"Invalid sample index %ld. Ignored (length is %ld)."
		,i
		,samplelist_length(l)
	);
}


