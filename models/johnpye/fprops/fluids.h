#ifndef FPROPS_FLUIDS_H
#define FPROPS_FLUIDS_H

#include "fprops.h"
#include "rundata.h"
#include "constcp_species.h"

/**
	Look up the named fluid and return its internal data structure, or
	NULL if not found.
*/
const PureFluid *fprops_fluid(const char *name, const char *corrtype, const char *source);

void fprops_fluid_destroy(PureFluid *fluid);

/**
	Look up the named fluid and return its EosData (metadata) record.
*/
const EosData *fprops_eos(const char *name, const char *corrtype, const char *source);

/**
	Build an element matrix A[ne * ns] (row-major) from species composition data.
*/
int fprops_build_element_matrix(const char **names, int ns, const char **elements, int ne, double *A_out);

/**
	@return number of fluids in the database.
*/
int fprops_num_fluids();

/**
	Retrieve fluid according to its position in the list of added fluids. The
	index number can't be assumed to be stable; this function is simply to
	allow iteration through all the fluids, searching, etc.
	@param i fluid index number [0,fprops_num_fluids()-1]
	@return NULL if i is out of bounds.
*/
const PureFluid *fprops_get_fluid(int i);

/**
	Build an element matrix A[ne * ns] (row-major) from species composition data, using a source filter.
*/
int fprops_build_element_matrix_source(const char **names, int ns, const char **elements, int ne,
		const char *source, double *A_out);

const ConstCpSpecies *fprops_constcp_species(const char *name, const char *source);

/**
	Resolve a source selector for one species.

	If `source_spec` is a plain source string (no '='), that string is returned.
	If `source_spec` is a map, the syntax is:
	    species=source;other=source2;*=defaultsource
	(`default=` is also accepted instead of `*=`).
	The resolved source is written to `out` and returned as `out`.
	Returns NULL if no source is resolved.
*/
const char *fprops_resolve_species_source(const char *source_spec, const char *species_name,
		char *out, unsigned out_len);

#endif
