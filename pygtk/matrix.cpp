extern "C"{
#include <utilities/config.h>
#include <utilities/ascPanic.h>
}

#include <stdexcept>
#include <sstream>

#include "matrix.h"

using namespace std;

Matrix::Matrix() : M(NULL){
	// nothing else
}

Matrix::Matrix(const Matrix &old) : M(old.M){
	asc_assert(M!=NULL);
}

Matrix::Matrix(mtx_matrix_t M) : M(M){
	asc_assert(M!=NULL);
}

/**
	Write out a matrix to a file, in a format by the user

	@param format format in which to write output ("mtx","debug", others as
	implemented)

	Return 0 on success, 1 on error
*/
int
Matrix::write(FILE *fp, const string &format) const{

	CONSOLE_DEBUG("HERE IN MATRIX::WRITE");

	CONSOLE_DEBUG("Writing matrix in format '%s'",format.c_str());

	if(M==NULL)throw runtime_error("Matrix is NULL");
	if(fp==NULL)throw runtime_error("File is NULL");

	if(format=="matlab"){
		mtx_write_region_matlab(fp, M, mtx_ENTIRE_MATRIX);
#ifdef ASC_WITH_MMIO
	}else if(format=="mtx"||format=="mmio"||format=="matrixmarket"){
		return mtx_write_region_mmio(fp, M, mtx_ENTIRE_MATRIX);
#endif
    }else if(format=="debug"){
		mtx__debug_output(fp, M);
	}else{
		throw runtime_error("Unrecognised export format requested");
	}

	return 0;
}

/**
	Return the size of the mtx (the entire matrix)
*/
vector<unsigned>
Matrix::size() const{
	asc_assert(M!=NULL);
	vector<unsigned> v;
	v.push_back(mtx_order(M));
	v.push_back(mtx_order(M));
	return v;
}





