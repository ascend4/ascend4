#ifndef FPROPS_FLASH_UNIFAC_H
#define FPROPS_FLASH_UNIFAC_H

typedef struct FpropsUNIFACSubgroupData{
	const char *name;
	int group;
	double R;
	double Q;
} FpropsUNIFACSubgroupData;

typedef struct FpropsUNIFACComponentData{
	const char *name;
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
	int nsub;
	const int *sub_index;
	const double *nu;
	double r;
	double q;
} FpropsUNIFACComponentData;

typedef struct FpropsUNIFACFlashPackage{
	int nc;
	int nsub;
	const FpropsUNIFACComponentData *components;
	const FpropsUNIFACSubgroupData *subgroups;
	const double *a;
} FpropsUNIFACFlashPackage;

typedef struct FpropsFlashTPZ{
	double T;
	double P;
	const double *z;
} FpropsFlashTPZ;

typedef struct FpropsFlashVLResult{
	int status;
	double beta;
	double *x;
	double *y;
} FpropsFlashVLResult;

int fprops_unifac_psat(const FpropsUNIFACComponentData *comp, double T, double *Psat);
int fprops_unifac_gamma(const FpropsUNIFACFlashPackage *pkg, double T, const double *x, double *gamma);
int fprops_unifac_flash_tpz(const FpropsUNIFACFlashPackage *pkg, const FpropsFlashTPZ *in, FpropsFlashVLResult *out);
int fprops_unifac_liq_fugacity(const FpropsUNIFACFlashPackage *pkg, double T, double P, const double *x, double *fugacity);

#endif
