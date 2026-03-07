#include "unifac_rundata.h"

#include <stdlib.h>
#include <string.h>

FpropsUNIFACRunData *fprops_unifac_prepare(
		const FpropsUNIFACSourceData *src,
		const char **components,
		int nc
){
	FpropsUNIFACRunData *run = NULL;
	int *global_to_local = NULL;
	int total_nu = 0;
	int nactive_subgroups = 0;
	int nactive_main_groups = 0;
	int offset = 0;
	int i, j;

	if(!src || !components || nc <= 0 || !src->interactions || !src->interactions->aij){
		return NULL;
	}

	run = (FpropsUNIFACRunData *)calloc(1, sizeof(*run));
	if(!run){
		return NULL;
	}
	run->src = src;
	run->nc = nc;
	run->components = (const FpropsUNIFACComponentSource **)calloc((size_t)nc, sizeof(*run->components));
	run->r = (double *)calloc((size_t)nc, sizeof(*run->r));
	run->q = (double *)calloc((size_t)nc, sizeof(*run->q));
	global_to_local = (int *)calloc((size_t)src->nsubgroups, sizeof(*global_to_local));
	if(!run->components || !run->r || !run->q || !global_to_local){
		fprops_unifac_destroy(run);
		free(global_to_local);
		return NULL;
	}
	for(i = 0; i < src->nsubgroups; ++i){
		global_to_local[i] = -1;
	}

	for(i = 0; i < nc; ++i){
		const FpropsUNIFACComponentSource *comp = fprops_unifac_component(src, components[i]);
		if(!comp){
			fprops_unifac_destroy(run);
			free(global_to_local);
			return NULL;
		}
		run->components[i] = comp;
		run->r[i] = comp->r;
		run->q[i] = comp->q;
		total_nu += comp->nsubgroups;
		for(j = 0; j < comp->nsubgroups; ++j){
			int gi = comp->subgroups[j].subgroup_index;
			if(gi < 0 || gi >= src->nsubgroups){
				fprops_unifac_destroy(run);
				free(global_to_local);
				return NULL;
			}
			if(global_to_local[gi] < 0){
				global_to_local[gi] = nactive_subgroups++;
			}
		}
	}

	run->nactive_subgroups = nactive_subgroups;
	run->active_subgroups = (const FpropsUNIFACSubgroupSource **)calloc((size_t)nactive_subgroups, sizeof(*run->active_subgroups));
	run->flash_subgroups = (FpropsUNIFACSubgroupData *)calloc((size_t)nactive_subgroups, sizeof(*run->flash_subgroups));
	run->sub_index_data = (int *)calloc((size_t)total_nu, sizeof(*run->sub_index_data));
	run->nu_data = (double *)calloc((size_t)total_nu, sizeof(*run->nu_data));
	run->flash_components = (FpropsUNIFACComponentData *)calloc((size_t)nc, sizeof(*run->flash_components));
	if((nactive_subgroups > 0 && (!run->active_subgroups || !run->flash_subgroups))
			|| !run->sub_index_data || !run->nu_data || !run->flash_components){
		fprops_unifac_destroy(run);
		free(global_to_local);
		return NULL;
	}

	for(i = 0; i < src->nsubgroups; ++i){
		int li = global_to_local[i];
		if(li >= 0){
			run->active_subgroups[li] = &src->subgroups[i];
			run->flash_subgroups[li].name = src->subgroups[i].name;
			run->flash_subgroups[li].group = src->subgroups[i].main_group_id;
			run->flash_subgroups[li].R = src->subgroups[i].R;
			run->flash_subgroups[li].Q = src->subgroups[i].Q;
		}
	}

	run->active_main_group_ids = (int *)calloc((size_t)nactive_subgroups, sizeof(*run->active_main_group_ids));
	if(nactive_subgroups > 0 && !run->active_main_group_ids){
		fprops_unifac_destroy(run);
		free(global_to_local);
		return NULL;
	}
	for(i = 0; i < nactive_subgroups; ++i){
		int gid = run->flash_subgroups[i].group;
		int seen = 0;
		for(j = 0; j < nactive_main_groups; ++j){
			if(run->active_main_group_ids[j] == gid){
				seen = 1;
				break;
			}
		}
		if(!seen){
			run->active_main_group_ids[nactive_main_groups++] = gid;
		}
	}
	run->nactive_main_groups = nactive_main_groups;
	if(nactive_main_groups > 0){
		run->aij = (double *)calloc((size_t)nactive_main_groups * (size_t)nactive_main_groups, sizeof(*run->aij));
		if(!run->aij){
			fprops_unifac_destroy(run);
			free(global_to_local);
			return NULL;
		}
		for(i = 0; i < nactive_main_groups; ++i){
			for(j = 0; j < nactive_main_groups; ++j){
				int gi = run->active_main_group_ids[i];
				int gj = run->active_main_group_ids[j];
				run->aij[i * nactive_main_groups + j] =
					src->interactions->aij[(gi - 1) * src->interactions->ngroups + (gj - 1)];
			}
		}
	}

	for(i = 0; i < nc; ++i){
		const FpropsUNIFACComponentSource *comp = run->components[i];
		FpropsUNIFACComponentData *dst = &run->flash_components[i];

		dst->name = comp->name;
		dst->Tc = comp->Tc;
		dst->Pc = comp->Pc;
		dst->vp_correlation = comp->vp_correlation;
		dst->vpa = comp->vpa;
		dst->vpb = comp->vpb;
		dst->vpc = comp->vpc;
		dst->vpd = comp->vpd;
		dst->T0 = comp->T0;
		dst->P0 = comp->P0;
		dst->H0 = comp->H0;
		dst->G0 = comp->G0;
		dst->cpvapa = comp->cpvapa;
		dst->cpvapb = comp->cpvapb;
		dst->cpvapc = comp->cpvapc;
		dst->cpvapd = comp->cpvapd;
		dst->omega = comp->omega;
		dst->Zc = comp->Zc;
		dst->Vliq = comp->Vliq;
		dst->Tliq = comp->Tliq;
		dst->nsub = comp->nsubgroups;
		dst->sub_index = &run->sub_index_data[offset];
		dst->nu = &run->nu_data[offset];
		dst->r = comp->r;
		dst->q = comp->q;
		for(j = 0; j < comp->nsubgroups; ++j){
			int gi = comp->subgroups[j].subgroup_index;
			int li = global_to_local[gi];
			if(li < 0){
				fprops_unifac_destroy(run);
				free(global_to_local);
				return NULL;
			}
			run->sub_index_data[offset + j] = li;
			run->nu_data[offset + j] = comp->subgroups[j].nu;
		}
		offset += comp->nsubgroups;
	}

	run->pkg.nc = nc;
	run->pkg.nsub = nactive_subgroups;
	run->pkg.components = run->flash_components;
	run->pkg.subgroups = run->flash_subgroups;
	run->pkg.a = src->interactions->aij;

	free(global_to_local);
	return run;
}

void fprops_unifac_destroy(FpropsUNIFACRunData *run){
	if(!run){
		return;
	}
	free(run->components);
	free(run->active_subgroups);
	free(run->active_main_group_ids);
	free(run->aij);
	free(run->r);
	free(run->q);
	free(run->flash_components);
	free(run->flash_subgroups);
	free(run->sub_index_data);
	free(run->nu_data);
	free(run);
}

int fprops_unifac_gamma_run(
		const FpropsUNIFACRunData *run,
		double T,
		const double *x,
		double *gamma
){
	if(!run){
		return -1;
	}
	return fprops_unifac_gamma(&run->pkg, T, x, gamma);
}

const FpropsUNIFACFlashPackage *fprops_unifac_flash_package(
		const FpropsUNIFACRunData *run)
{
	if(!run){
		return NULL;
	}
	return &run->pkg;
}
