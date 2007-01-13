/** @file
	Object-orient wrapper for the ASCEND 'mtx' data structure and
	related methods. Primary for use in debugging models and/or exporting
	matrices to other programs.

	Accessing mtx_matrix_t objects through this interface should add only a
	single dereference; this is to be a very lightweight wrapper only.

	Note that mtx_matrix_t objects contain a great deal more information than
	most Matrix data structures. They include space to store block decomposition
	and row/column permutation information that help with efficient solving
	of matrix problems like A*x=b.

	When a matrix is exported (such as writeMatrixMarket) this information will
	mostly be lost in the resulting file.

	Some interesting possibilities exist in exporting to numpy/numarray formats
	as these do include some support for permutations and so on.

	@NOTE we don't use iostream for input/output here as (a) it's not compatible
	with the underlying C routines and (b) we want to wrap the FILE* objects
	and allow access to them using Python's file methods, eg:
	http://www.swig.org/Doc1.1/HTML/Python.htm
	(search down to " Converting a Python file object to a FILE *")
	@ENDNOTE
*/
#ifndef ASCXX_MATRIX_H
#define ASCXX_MATRIX_H

extern "C"{
#include <solver/mtx.h>
}

#include <cstdio>
#include <string>
#include <vector>

class Matrix{
protected:
	mtx_matrix_t M; /* mtx_matrix_t is a typedef for 'struct mtx_header*' */
public:

	Matrix();
	Matrix(const Matrix &M);

	Matrix(mtx_matrix_t M);

	int write(FILE *fp, const std::string &format) const;
	std::vector<unsigned> size() const;
};

#endif
