#include <math.h>
#include <utilities/ascConfig.h>
#include <utilities/ascMalloc.h>
#include <utilities/mem.h>
#include "mtx.h"
#ifdef ASC_WITH_MMIO
# include <mmio.h>
#endif

/* grab our private parts */
#define __MTX_C_SEEN__
#include "mtx_use_only.h"
#include <general/mathmacros.h>

#include "mtx_csparse.h"

#include <utilities/error.h>
#include <utilities/ascPanic.h>

#ifdef ASC_WITH_UFSPARSE

/**
	We don't try to make this super-optimal at the moment. Just try to create	
	the csparse matrix in triplet form.

	@return NULL on error
*/
cs *mtx_to_cs(mtx_matrix_t M){
	cs *S;
	mtx_region_t R;
	int j, r;
	int nnz;
	int *to_cur;
	struct element_t *elt;
	struct mtx_header X;

	if(M == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"Matrix M is NULL");
		return NULL;
	}

	mtx_region( &R, 0, mtx_order(M)-1, 0, mtx_order(M)-1 );
	nnz = mtx_nonzeros_in_region(M, mtx_ENTIRE_MATRIX);

	CONSOLE_DEBUG("Matrix order is %d",mtx_order(M));
	CONSOLE_DEBUG("Region is rows [%d,%d], cols [%d,%d]",R.row.low,R.row.high,R.col.low,R.col.high);
	CONSOLE_DEBUG("Nonzeros: %d",nnz);

	S = cs_spalloc(mtx_order(M),mtx_order(M),nnz, 1/*alloc for values*/,1/*tripled format*/);
	j = 0; /* current csparse triplet */

	to_cur = M->perm.col.org_to_cur;
	for(r = R.row.low; r <= R.row.high; ++(r) ){		
		for( 
			elt = M->hdr.row[M->perm.row.cur_to_org[r]]
			; elt != NULL
			; elt = elt->next.col
		){
			S->i[j] = r;
			S->p[j] = to_cur[elt->col];
			S->x[j] = elt->value;
		    CONSOLE_DEBUG("%d %d %.20g",r,to_cur[elt->col],elt->value);
			++j;
		}
	}
	asc_assert(j==nnz);
	
	S->nz = nnz;
	CONSOLE_DEBUG("S->nz = %d", S->nz);

	return S;
}

/*
	We can't assume the CSparse matrix is in triplet form now. But to 
	make things easy we'll just write to the matrix using mtx_set_value.

	@return NULL on error
*/
mtx_matrix_t mtx_from_cs(const cs *A){
	mtx_matrix_t M;
	mtx_coord_t coord;

	if(A == NULL){
		ERROR_REPORTER_HERE(ASC_PROG_ERR,"CSparse matrix is NULL");
		return NULL;
	}

	M = mtx_create();
	mtx_set_order(M, MAX(A->n, A->m));

    int p, j, m, n, nzmax, nz, *Ap, *Ai ;
    double *Ax ;

    m = A->m ; n = A->n ; Ap = A->p ; Ai = A->i ; Ax = A->x ;
    nzmax = A->nzmax ; nz = A->nz ;

    if(nz < 0){ /* compressed-col format */
		for(j = 0; j < n; j++){
		    for (p = Ap [j] ; p < Ap [j+1] ; p++){
				mtx_set_value(M,mtx_coord(&coord, Ai[p], j), Ax ? Ax[p] : 1);
		    }
		}
    }else{ /* tripled format */
		printf ("triplet: %d-by-%d, nzmax: %d nnz: %d\n", m, n, nzmax, nz);
		for (p = 0 ; p < nz ; p++){
			mtx_set_value(M,mtx_coord(&coord, Ai[p], Ap[p]), Ax ? Ax [p] : 1);
		}
    }
    
	return M;
}

#endif
