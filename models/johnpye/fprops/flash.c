#include "flash.h"
#include <string.h>

int fprops_flash_prepare_unifac(
		FpropsMultiphasePackage *pkg,
		const char *source,
		const char **components,
		int nc
){
	const FpropsUNIFACSourceData *src;
	FpropsUNIFACRunData *run;

	if(!pkg || !components || nc <= 0){
		return -1;
	}

	memset(pkg, 0, sizeof(*pkg));
	if(!source || !*source){
		source = "UNIFAC-orig-2003";
	}
	src = fprops_unifac_source(source);
	if(!src){
		return -2;
	}
	run = fprops_unifac_prepare(src, components, nc);
	if(!run){
		return -3;
	}
	pkg->kind = FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL;
	pkg->nc = nc;
	pkg->data.unifac_ideal_vl.run = run;
	pkg->data.unifac_ideal_vl.pkg = fprops_unifac_flash_package(run);
	return 0;
}

void fprops_flash_destroy_package(FpropsMultiphasePackage *pkg){
	if(!pkg){
		return;
	}
	switch(pkg->kind){
	case FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL:
		if(pkg->data.unifac_ideal_vl.run){
			fprops_unifac_destroy(pkg->data.unifac_ideal_vl.run);
		}
		break;
	case FPROPS_FLASH_PACKAGE_INVALID:
	default:
		break;
	}
	memset(pkg, 0, sizeof(*pkg));
}

int fprops_flash_tpz(const FpropsMultiphasePackage *pkg, const FpropsFlashTPZ *in, FpropsFlashVLResult *out){
	if(!pkg){
		return -1;
	}
	switch(pkg->kind){
	case FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL:
		if(!pkg->data.unifac_ideal_vl.pkg){
			return -3;
		}
		return fprops_unifac_flash_tpz(pkg->data.unifac_ideal_vl.pkg, in, out);
	case FPROPS_FLASH_PACKAGE_INVALID:
	default:
		return -2;
	}
}
