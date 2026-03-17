#ifndef FPROPS_FLASH_H
#define FPROPS_FLASH_H

#include "flash_unifac.h"
#include "mixtures/unifac_rundata.h"

typedef enum FpropsFlashPackageKind{
	FPROPS_FLASH_PACKAGE_INVALID = 0,
	FPROPS_FLASH_PACKAGE_UNIFAC_IDEAL_VL = 1
} FpropsFlashPackageKind;

typedef struct FpropsMultiphaseUNIFACPackage{
	FpropsUNIFACRunData *run;
	const FpropsUNIFACFlashPackage *pkg;
} FpropsMultiphaseUNIFACPackage;

typedef struct FpropsMultiphasePackage{
	FpropsFlashPackageKind kind;
	int nc;
	union{
		FpropsMultiphaseUNIFACPackage unifac_ideal_vl;
	} data;
} FpropsMultiphasePackage;

int fprops_flash_prepare_unifac(
		FpropsMultiphasePackage *pkg,
		const char *source,
		const char **components,
		int nc);

void fprops_flash_destroy_package(FpropsMultiphasePackage *pkg);

int fprops_flash_tpz(const FpropsMultiphasePackage *pkg, const FpropsFlashTPZ *in, FpropsFlashVLResult *out);

#endif
