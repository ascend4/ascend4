#ifndef FPROPS_UNIFAC_RUNDATA_H
#define FPROPS_UNIFAC_RUNDATA_H

#include "unifac_data.h"
#include "../flash_unifac.h"

/*
	Prepared/runtime declarations for original UNIFAC-style databases.

	This layer is the mixture analogue of the pure-fluid runtime layer in
	rundata.h. It is where component selection, compaction, and evaluation
	caches belong. Nothing here should depend on the ASCEND instance tree.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FpropsUNIFACRunData_struct{
	const FpropsUNIFACSourceData *src;

	int nc;
	const FpropsUNIFACComponentSource **components;

	int nactive_subgroups;
	const FpropsUNIFACSubgroupSource **active_subgroups;

	int nactive_main_groups;
	int *active_main_group_ids;

	/* Compact active-main-group interaction matrix, row-major. */
	double *aij;

	/* Per-component convenience vectors. */
	double *r;
	double *q;

	/* Current prepared package for flash/gamma evaluators. */
	FpropsUNIFACFlashPackage pkg;
	FpropsUNIFACComponentData *flash_components;
	FpropsUNIFACSubgroupData *flash_subgroups;
	int *sub_index_data;
	double *nu_data;
} FpropsUNIFACRunData;

FpropsUNIFACRunData *fprops_unifac_prepare(
		const FpropsUNIFACSourceData *src,
		const char **components,
		int nc);

void fprops_unifac_destroy(FpropsUNIFACRunData *run);

int fprops_unifac_gamma_run(
		const FpropsUNIFACRunData *run,
		double T,
		const double *x,
		double *gamma);

const FpropsUNIFACFlashPackage *fprops_unifac_flash_package(
		const FpropsUNIFACRunData *run);

#ifdef __cplusplus
}
#endif

#endif
