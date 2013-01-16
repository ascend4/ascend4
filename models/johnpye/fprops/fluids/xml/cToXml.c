#include <stdio.h>
#include "../helmholtz.h"
#include "../ideal.h"
#include "../sat.h"
#include "xenon.c"


int main(void)
{
	HelmholtzData h=helmholtz_data_xenon;
	IdealData i=ideal_data_xenon;
	int count;
	printf(
	"﻿<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	"<fluid name=\"%s\">\n"
	"\t<idealData>\n"
	"\t\t<molarMass value=\"%f\" />\n"
	"\t\t<specificGasConstant value=\"%f\" />\n"
	"\t\t<normalizationTemperature value=\"%f\" />\n"
	"\t\t<constantTerm value=\"%f\" />\n"
	"\t\t<linearTerm value=\"%f\" />\n"
	"\t\t<idealPowerTerms>\n",h.name,h.M,h.R,h.T_star,i.c,i.m);
	for(count=0; count<i.np; count++) {
		printf("\t\t\t<idealPowerTerm c=\"%f\" t=\"%f\" />\n",i.pt[count].c,i.pt[count].t);
	}
	printf("\t\t</idealPowerTerms>\n"
	"\t\t<exponentialTerms>\n");
	for(count=0; count<i.ne; count++) {
		printf("\t\t\t<exponentialTerm b=\"%f\" beta=\"%f\" />\n",i.et[count].b,i.et[count].beta);
	}
	printf("\t\t</exponentialTerms>\n"
	"\t</idealData>\n"
	"\t<helmholtzData>\n"
	"\t\t<rhoStar value=\"%f\" />\n"
	"\t\t<rhoC value=\"%f\" />\n"
	"\t\t<triplePointTemp value=\"%f\" />\n"
	"\t\t<acentricFactor value=\"%f\" />\n"
    "\t\t<helmholtzPowerTerms>\n",h.rho_star, h.rho_c, h.T_t, h.omega);
    for(count=0; count<h.np; count++) {
		printf("\t\t\t<powerTerm a=\"%f\" t=\"%f\" d=\"%d\" l=\"%d\" />\n",h.pt[count].a,h.pt[count].t,h.pt[count].d,h.pt[count].l);
	}
	printf("\t\t</helmholtzPowerTerms>\n"
	"\t\t<criticalTermsFirstKind>\n");
	for(count=0; count<h.ng; count++) {
		printf("\t\t\t<criticalTerm n=\"%f\" t=\"%f\" d=\"%f\" alpha=\"%f\" beta=\"%f\" gamma=\"%f\" epsilon=\"%f\" />\n",
		h.gt[count].n,h.gt[count].t,h.gt[count].d,h.gt[count].alpha, h.gt[count].beta, h.gt[count].gamma, h.gt[count].epsilon);
	}
	printf("\t\t</criticalTermsFirstKind>\n"
	"\t\t<criticalTermsSecondKind>\n");
	for(count=0; count<h.nc; count++) {
		printf("\t\t\t<criticalTerm n=\"%f\" a=\"%f\" b=\"%f\" beta=\"%f\" A=\"%f\" B=\"%f\" C=\"%f\" D=\"%f\" />\n",
		h.ct[count].n,h.ct[count].a,h.ct[count].b,h.ct[count].beta,h.ct[count].A,h.ct[count].B,h.ct[count].C,h.ct[count].D);
	}
	printf("\t\t</criticalTermsSecondKind>\n"
	"\t</helmholtzData>\n"
	"</fluid>\n");
	return 0;
}
