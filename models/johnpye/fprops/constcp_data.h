#ifndef FPROPS_CONSTCP_DATA_H
#define FPROPS_CONSTCP_DATA_H

#include "constcp_species.h"

/* Placeholder condensed-phase data for Fe/O/H species. */

const ConstCpSpecies *constcp_data_lookup(const char *name, const char *source);

int constcp_data_build_element_matrix(const char **names, int ns, const char **elements, int ne,
		const char *source, double *A_out);

#endif /* FPROPS_CONSTCP_DATA_H */
