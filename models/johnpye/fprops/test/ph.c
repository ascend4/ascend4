
#include "../fluids.h"
#include "../fprops.h"
#include "../solve_ph.h"
#include "../refstate.h"
#include "../sat.h"

#include <assert.h>
#include <math.h>

#include "../color.h"

#define MSG(FMT, ...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"%s:%d: ",__FILE__,__LINE__);\
	color_on(stderr,ASC_FG_BRIGHTBLUE);\
	fprintf(stderr,"%s: ",__func__);\
	color_off(stderr);\
	fprintf(stderr,FMT "\n",##__VA_ARGS__)

#define ERRMSG(STR,...) \
	color_on(stderr,ASC_FG_BRIGHTRED);\
	fprintf(stderr,"ERROR:");\
	color_off(stderr);\
	fprintf(stderr," %s:%d:" STR "\n", __func__, __LINE__ ,##__VA_ARGS__)

#define TOL_T 1e-8
#define TOL_RHO 1e-3


void test_ph_array(const PureFluid *P, int nT, int nv, double Trmin, double Trmax, double prskip, int *testedpoints, int *nerr, FpropsError *err){
	*err = FPROPS_NO_ERROR;
	*testedpoints = 0;
	int i,j;
	double Tmin, Tmax, dT;
	double vmin, vmax, dlv;

	if(!P){
		ERRMSG("Pure fluid is not defined");
		*err = FPROPS_INVALID_REQUEST;
		return;
	}

	double Tc = P->data->T_c;
	double vc = 1./P->data->rho_c;
	double pc = P->data->p_c;
	Tmax = Trmax * Tc;
	Tmin = P->data->T_t;
	if(0 == Tmin){
		double psat,rhof, rhog;
		Tmin = Trmin * Tc;
		fprops_sat_T(Tmin, &psat, &rhof, &rhog, P, err);
		if(*err){
			ERRMSG("Saturation at T = %f failed for '%s",Tmin,P->name);
			return;
		}
		vmin = 1./rhof;
		vmax = 1./rhog;
	}else{
		double pt, rhoft, rhogt;
		fprops_triple_point(&pt, &rhoft, &rhogt, P, err);
		if(*err){
			ERRMSG("Triple point properties failed for '%s",P->name);
			return;
		}
		vmin = 1./rhoft;
		vmax = 1.2/rhogt;
		if(vmax > 1e4*vc)vmax=1e4*vc;
	}
	/* embarrassingly parallel loops coming up... */

//#define TOLT 1e-5
//#define TOLV 1e-5
#define TOLT 1e-1
#define TOLV 1e-1

	*nerr = 0;

	FILE *conff = fopen("pherr.ini","w");
	fprintf(conff,"[main]\n");
	fprintf(conff,"fluid: %s\n",P->name);
	fprintf(conff,"type: %s\n",fprops_corr_type(P->type));
	fprintf(conff,"source: %s\n",P->source);
	fprintf(conff,"tmin: %e\n",Tmin);
	fprintf(conff,"tmax: %e\n",Tmax);
	fprintf(conff,"vmin: %e\n",vmin);
	fprintf(conff,"vmax: %e\n",vmax);
	// lower limit for saturation curves, might not be same as tmin in all cases
	fprintf(conff,"tt: %e\n",Tmin);
#define PHERR_DAT "pherr.dat"
	fprintf(conff,"data: %s\n",PHERR_DAT);

	fclose(conff);

	FILE *plotf = fopen(PHERR_DAT,"w");

	// linear spacing on T, log spacing on v
	dT = (Tmax - Tmin) / (nT - 1);
	dlv = (log(vmax) - log(vmin)) / (nv - 1);
	double T = Tmin, lv = log(vmin);
	for(i=0; i<nT; ++i, T += dT){
		fprintf(stderr,">>> T = %6.3f %-40s      \r",T,P->name);
		lv = log(vmin);
		for(j=0; j<nv; ++j, lv += dlv){
			double v = exp(lv);
			double rho = 1./v;
			*err = FPROPS_NO_ERROR;
			FluidState S = fprops_set_Trho(T,rho,P,err);
			if(*err){ERRMSG("Can't set (T,rho)");return;}
			double p = fprops_p(S,err);
			if(*err){ERRMSG("Couldn't calculate p");return;}

			if(p / pc > prskip)continue;
	
			double h = fprops_h(S,err);
			if(*err){ERRMSG("Couldn't calculate h");return;}

			double T1, rho1;
			fprops_solve_ph(p,h,&T1,&rho1,0,P,err);
			(*testedpoints)++;
			if(*err){
				/*ERRMSG("Couldn't solve (p,h) at T = %f, rho = %f",T,1./v);*/
				fprintf(plotf,"%e\t%e\n",T,v);
				(*nerr)++;
				continue;
			}
			
			double Terr = fabs(T1 - T)/T;
			double verr = fabs(1./rho1 - v)/v;
			if(Terr > TOLT || verr > TOLV){
				//ERRMSG("T or v fail tol at T = %f, rho = %f (Terr %f%%, verr %f%%)",T,rho,Terr*100,verr*100);
				fprintf(plotf,"%e\t%e\n",T,v);
				(*nerr)++;
				continue;
			}
		}
	}
	fclose(plotf);
	if(*nerr){
		ERRMSG("Got %d errors for %s off %d tested points",*nerr,P->name,*testedpoints);
		*err = FPROPS_NUMERIC_ERROR;
	}else{
		ERRMSG("Tested %d points for %s with no failures (T tol %.0e, v tol %.0e)",*testedpoints,P->name,TOLT,TOLV);
	}
}


int main(void){
	const PureFluid *P;
	//const char *fluids[] = {"water","toluene","ethanol","isohexane","n_eicosane", NULL};
	const char *fluids[] = {"isohexane",NULL};
	const char **fi = fluids;
	int nfluiderrors = 0;
	while(*fi){
		FpropsError err = FPROPS_NO_ERROR;
		P = fprops_fluid(*fi,"pengrob",NULL);
		assert(P);
		int testedpoints;
		int nerr;
		test_ph_array(P, /*nT*/300, /*nv*/300, /*Trmin*/0.4, /*Trmax*/1.8, /*prskip*/1.5, &testedpoints, &nerr, &err);
		if(err)nfluiderrors++;
		++fi;
	}
	
	if(nfluiderrors){
		ERRMSG("There were %d fluids with (p,h) errors",nfluiderrors);
		ERRMSG("Run 'python python/view_ph_results.py' to view results from '%s'",*(fi-1));
		return 1;
	}


#if 0
	FpropsError err = FPROPS_NO_ERROR;
	FluidState S;
	double T0, rho0, p, h, T, rho;



#define TEST_PH(T1,RHO1) \
	err = FPROPS_NO_ERROR;\
	/*MSG("TEST_PH(T1=%f, RHO1=%f)",T1,RHO1);*/ \
	T0 = T1; \
	rho0 = RHO1; \
	/*MSG("setting T,rho");*/\
	S = fprops_set_Trho(T0,rho0,P,&err); \
	assert(!err); \
	/*MSG("calc p");*/\
	p = fprops_p(S,&err); \
	assert(!err); \
	/*MSG("calc h");*/\
	h = fprops_h(S,&err); \
	assert(!err); \
	/*MSG("solving ph");*/\
	fprops_solve_ph(p,h,&T,&rho,0,P,&err); \
	assert(!err); \
	MSG("T err: %f",(T-T0));\
	MSG("rho err: %f",(rho-rho0));\
	assert(fabs(T - T0) < TOL_T); \
	assert(fabs(rho - rho0) < TOL_RHO);

	P = fprops_fluid("water","pengrob","IAPWS");
	ReferenceState R = {FPROPS_REF_TPF};
	fprops_set_reference_state(P,&R);
	MSG("Testing '%s' (type '%d', source '%s')", P->name, P->type, P->source);
	assert(P);
	err = FPROPS_NO_ERROR;

	TEST_PH(2.731600000000e+02, 8.618514819566e+02);
	MSG("p = %f",p);
	MSG("h = %f",h);

	P = fprops_fluid("isohexane","pengrob","J. Chem. Eng. Data, 51");
	assert(P);

	TEST_PH(128.4465, 1.);
	TEST_PH(1.284465e+02, 1/1.693087e-03);
	TEST_PH(1.284465e+02, 1/2.232836e-03);
	TEST_PH(1.284465e+02, 1/2.944654e-03);
	TEST_PH(1.284465e+02, 1/3.883396e-03);
	TEST_PH(1.284465e+02, 1/5.121405e-03);

	P = fprops_fluid("isohexane","helmholtz","J. Chem. Eng. Data, 51");
	assert(P);

	TEST_PH(119.6, 807.530551164909);
	
	P = fprops_fluid("water","helmholtz",NULL);
	assert(P);

	TEST_PH(314.4054538268115, 999.7897902747587);

	TEST_PH(4.278618181818e+02, 3.591421624513e-03);
	TEST_PH(3.453541818182e+02, 6.899880592960e-03);
	TEST_PH(7.166385454545e+02, 6.899880592960e-03);
	TEST_PH(7.372654545455e+02, 4.092431601778e-03);

	TEST_PH(304.10372142680086, 999.7863801065582);
	TEST_PH(283.4886584572104, 999.7900620473787);
	TEST_PH(293.8028752316878, 999.7858245521049);

#endif

	/* all done? report success */
	fprintf(stderr,"\n");
	color_on(stderr,ASC_FG_BRIGHTGREEN);
	fprintf(stderr,"SUCCESS (%s)",__FILE__);
	color_off(stderr);	
	fprintf(stderr,"\n");
	return 0;
}

