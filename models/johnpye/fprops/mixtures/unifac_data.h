#ifndef FPROPS_UNIFAC_DATA_H
#define FPROPS_UNIFAC_DATA_H

/*
	Generated/source-data declarations for original UNIFAC-style databases.

	This layer is the mixture analogue of the pure-fluid source-data layer in
	filedata.h. It is intentionally immutable. Runtime preparation, caching,
	and evaluator-specific compaction belong in unifac_rundata.h and the
	corresponding implementation files.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FpropsUNIFACSubgroupSource_struct{
	const char *name;
	int subgroup_id;
	int main_group_id;
	double R;
	double Q;
} FpropsUNIFACSubgroupSource;

typedef struct FpropsUNIFACComponentSubgroupSource_struct{
	int subgroup_index;
	double nu;
} FpropsUNIFACComponentSubgroupSource;

typedef struct FpropsUNIFACComponentSource_struct{
	const char *name;
	const char *formula;
	int nsubgroups;
	const FpropsUNIFACComponentSubgroupSource *subgroups;
	double r;
	double q;
	double Tc;
	double Pc;
	int vp_correlation;
	double vpa;
	double vpb;
	double vpc;
	double vpd;
	double T0;
	double P0;
	double H0;
	double G0;
	double cpvapa;
	double cpvapb;
	double cpvapc;
	double cpvapd;
	double omega;
	double Zc;
	double Vliq;
	double Tliq;
} FpropsUNIFACComponentSource;

typedef struct FpropsUNIFACInteractionSource_struct{
	int ngroups;
	const double *aij;
} FpropsUNIFACInteractionSource;

typedef struct FpropsUNIFACAliasSource_struct{
	const char *alias;
	int component_index;
} FpropsUNIFACAliasSource;

typedef struct FpropsUNIFACSourceData_struct{
	const char *name;
	const FpropsUNIFACSubgroupSource *subgroups;
	int nsubgroups;
	const FpropsUNIFACComponentSource *components;
	int ncomponents;
	const FpropsUNIFACInteractionSource *interactions;
	const FpropsUNIFACAliasSource *aliases;
	int naliases;
} FpropsUNIFACSourceData;

/*
	Canonical public database object for the current generated original-UNIFAC
	parameter set. The exact symbol name is part of the generated-data contract.
*/
extern const FpropsUNIFACSourceData fprops_unifac_orig_2003;

/*
	Lookup helpers to be implemented in the future generated/native UNIFAC path.
	These mirror the role of fprops_eos(...) / fprops_fluid(...) on the pure side.
*/
const FpropsUNIFACSourceData *fprops_unifac_source(const char *name);

const FpropsUNIFACComponentSource *fprops_unifac_component(
		const FpropsUNIFACSourceData *src, const char *name);

const FpropsUNIFACSubgroupSource *fprops_unifac_subgroup(
		const FpropsUNIFACSourceData *src, const char *name);

#ifdef __cplusplus
}
#endif

#endif
