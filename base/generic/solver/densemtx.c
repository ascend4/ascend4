/*	ASCEND modelling environment
	Copyright (C) 2007 Carnegie Mellon University

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
	Implementation of the dense matrix module, as used by the LSODE Integrator 
	in ASCEND.
*//*
	by John Pye, Jan 2007. Pulled out the routines from lsode.c and added
	a file export function so that these matrices can be viewed using outside
	tools.
*/

#include "densemtx.h"
#include <utilities/ascMalloc.h>
#include <utilities/error.h>
#include <mmio.h>

DenseMatrix densematrix_create_empty(){
	DenseMatrix result = {NULL,0,0};
	return result;
}

/**
	Allocate space for a new dense matrix object of rows x cols
*/
DenseMatrix densematrix_create(unsigned nrows, unsigned ncols){
  unsigned c;
  DenseMatrix result;
  assert(nrows>0);
  assert(ncols>0);
  result.data = ASC_NEW_ARRAY(double *, nrows);
  for (c=0;c<nrows;c++) {
    result.data[c] = ASC_NEW_ARRAY_CLEAR(double, ncols);
  }
  result.nrows = nrows;
  result.ncols = ncols;
  return result;
}

/**
	Deallocate space used by the dense matrix
*/
void densematrix_destroy(DenseMatrix matrix){
  unsigned c;
  if(matrix.data != NULL){
    for(c=0;c<matrix.nrows;c++){
      if(matrix.data[c]){
        ascfree((char *)matrix.data[c]);
      }
    }
    ASC_FREE((char *)matrix.data);
	matrix.data = NULL;
  }
}

void densematrix_write_mmio(DenseMatrix matrix, FILE *fp){
	int i,j;

    MM_typecode matcode;                        

    mm_initialize_typecode(&matcode);
    mm_set_matrix(&matcode);
    mm_set_array(&matcode);
    mm_set_real(&matcode);

    mm_write_banner(fp, matcode); 
	fprintf(fp,"%% Dense matrix output from ASCEND modelling environment\n");
	fprintf(fp,"%% ASCEND (c) 2007 Carnegie Mellon University\n");
	fprintf(fp,"%% rows = %d, cols = %d\n", matrix.nrows, matrix.ncols);

    mm_write_mtx_array_size(fp, matrix.nrows, matrix.ncols);

	for(j=0;j<matrix.nrows;++j){
		for(i=0;i<matrix.ncols;++i){
			fprintf(fp,"%0.20g\n",DENSEMATRIX_ELEM(matrix,i,j));
		}
	}
	
	CONSOLE_DEBUG("Wrote dense matrix (%u x %u) to file", matrix.nrows, matrix.ncols);
}

