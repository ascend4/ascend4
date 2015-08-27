/*	ASCEND modelling environment
	Copyright (C) 2015 Sidharth, John Pye

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//** @file
Implementation for Tabulated Taylor Series Expansion (TTSE) property evaluation
in FPROPS. For more details see http://ascend4.org/User:Sidharth
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rundata.h"
#include "ttse.h"
#include "helmholtz.h"

#define NRHOP 200
#define NTP 200

#define TTSE_DEBUG //sid change
#ifdef TTSE_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
# include <assert.h>
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
# define assert(ARGS...)
#endif

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>


#define SO
//#define TTSE_USE_FILE_CACHE

/* TODO: at the moment, all our TTSE tables are fixed size, per #defs for NTP and NRHOP */
#define TTSE_ALLOC(TP, RHOP) ASC_NEW_ARRAY(double,TP*RHOP);
#define TTSE_FREE(MAT) ASC_FREE(MAT)
#define TTSE_SET(TAB,NAME,TI,RHOI,VAL) TAB->NAME[TI*NRHOP + RHOI] = VAL
#define TTSE_GET(TAB,NAME,TI,RHOI) TAB->NAME[TI*NRHOP + RHOI]

/* NOTE, the #define list 'TTSE_MATRICES' list is defined in rundata.h. */
/* TODO refine the above to separate first-order and second-order derivs */

/* forward decls */
static void build_tables(PureFluid *Ph,PureFluid *P);

/*------------------------------------------------------------------------------
	PREPARE DATA, INITIALISATION
*/

PureFluid * ttse_prepare(const EosData *E, const ReferenceState *ref){

    PureFluid *P;
    PureFluid *Ph;


#define TTD P->data->corr.ttse
#define TABLE P->data->corr.ttse->table

    if(E->type == FPROPS_HELMHOLTZ)
    {
       P = helmholtz_prepare(E, NULL);
       Ph = helmholtz_prepare(E, NULL);
       P->data->corr.ttse = FPROPS_NEW(TtseData);// we overwrite the helmholtz corr with ttsedata
       helmholtz_ttse(& P->data->corr);
    }
    else
    {

		ERRMSG("TTSE not implemented for the current correlation selection");
		return NULL;
    }

#ifdef TTSE_DEBUG
	//FILE *F1 = fopen("ttse.txt","w");
	//fprintf(F1,"%f   %f\n",t, P->p_fn( t, rho , P->data,&err) );
#endif


	/* common data across all correlation types */
	MSG("Inside TTSE");
	TABLE = FPROPS_NEW(Ttse);

	/* allocate space for data tables */
	TABLE->satFRho =  ASC_NEW_ARRAY ( double, NSAT);
	TABLE->satFdRhodt =  ASC_NEW_ARRAY (double, NSAT);
	TABLE->satFd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);
	TABLE->satGRho =  ASC_NEW_ARRAY (double, NSAT);
	TABLE->satGdRhodt =  ASC_NEW_ARRAY (double, NSAT);
	TABLE->satGd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);

#define X ;
#define I(M) TABLE->M = TTSE_ALLOC(NTP,NRHOP)
	TTSE_MATRICES(X,I);
#undef X
#undef I
	//Pseudo values for water
	/* FIXME TODO XXX Should be implemented elsewhere on per-fluid basis */
	TTD->Tmin = 200;
	TTD->Tmax = 4200;
	TTD->rhomin = 0.0001;
	TTD->rhomax = 2000;

#ifdef TTSE_USE_FILE_CACHE
	if(doesdbexist(P))//file exists in tables/
		load_tables(P);
	else{
		build_tables(P);
		save_tables(P);
	}
#else
	build_tables(Ph,P);
	helmholtz_destroy(Ph);
#endif

#define FN(VAR) P->VAR##_fn = &ttse_##VAR;
	FN(p); FN(u); FN(h); FN(s); FN(g);
	FN(cp); FN(cv); FN(w);
	FN(sat);
	// the undefined ones will raise errors
	FN(a); FN(alphap); FN(betap); FN(dpdrho_T);
#undef FN

#define FN(VAR) TTD->VAR##_fn = NULL;
    FN(p);FN(h);FN(s);FN(u);FN(g);
    FN(dpdrho_T);FN(d2pdrho2_T);FN(dpdT_rho);FN(d2pdT2_rho);FN(d2pdTdrho);
	FN(dhdrho_T);FN(d2hdrho2_T);FN(dhdT_rho);FN(d2hdT2_rho);FN(d2hdTdrho);
	FN(dsdrho_T);FN(d2sdrho2_T);FN(dsdT_rho);FN(d2sdT2_rho);FN(d2sdTdrho);
	FN(dudrho_T);FN(d2udrho2_T);FN(dudT_rho);FN(d2udT2_rho);FN(d2udTdrho);
	FN(dgdrho_T);FN(d2gdrho2_T);FN(dgdT_rho);FN(d2gdT2_rho);FN(d2gdTdrho);
#undef FN

#ifdef TTSE_DEBUG
	//fclose(F1);
#endif

#undef TABLE
#undef TTD

    return P;
}

void ttse_destroy(PureFluid *P){
#define TABLE P->data->corr.ttse->table
	ASC_FREE(TABLE->satFRho );
	ASC_FREE(TABLE->satFdRhodt );
	ASC_FREE(TABLE->satFd2RhodT2 );
	ASC_FREE(TABLE->satGRho );
	ASC_FREE(TABLE->satGdRhodt );
	ASC_FREE(TABLE->satGd2RhodT2 );

#define X ;
#define I(M) TTSE_FREE(TABLE->M);TABLE->M = NULL;
	TTSE_MATRICES(X,I)
#undef X
#undef I
#undef TABLE
}

/*------------------------------------------------------------------------------
  BUILD TABLES, LOAD AND SAVE FROM FILE
*/

/**
    Actual building of tables is done here.
*/
void build_tables(PureFluid *Ph,PureFluid *P){
#define TTD P->data->corr.ttse
#define TABLE P->data->corr.ttse->table
#define HELMD Ph->data

	int i,j;
	FpropsError err = FPROPS_NO_ERROR;

	double Tt = Ph->data->T_t;
	double Tc = Ph->data->T_c;
	double dT = (Tc - Tt)/(NSAT);

	double  rho1,rho2;
	P->sat_fn(Tc,&rho1,&rho2,HELMD,&err);
	MSG("triple point and critical temperature and critical density-->  %f  %f  %f",Tt,Tc,rho1);

	MSG("Building saturation tables...");

	for(i=0; i<NSAT; ++i)    {

		double T = Tt + i*dT;

		double  rhof,rhog;
		P->sat_fn(T,&rhof,&rhog,HELMD,&err);

		// fluid saturation line rho plus 1st & 2nd derivatives of rho wrt T
		double dpdT_rho  = TTD->dpdT_rho_fn(T,rhof,HELMD,&err);
		double dpdrho_T  = TTD->dpdrho_T_fn(T,rhof,HELMD,&err);
		double drhodT_p =  (-dpdT_rho )/(dpdrho_T);

		double d2pdrho2_T = TTD->d2pdrho2_T_fn(T,rhof,HELMD,&err);
		double d2pdrhodT = TTD->d2pdTdrho_fn(T,rhof,HELMD,&err);
		double d2pdT2_rho = TTD->d2pdT2_rho_fn(T,rhof,HELMD,&err);
		TABLE->satFRho[i] = rhof;
		TABLE->satFdRhodt[i] = drhodT_p;
		//PT->satFd2RhodT2[i] =  ddT_drhodT_p_constrho  +  ddrho_drhodT_p_constT * drhodT_p;
		TABLE->satFd2RhodT2[i] =  (-1.0/pow(dpdrho_T,3))*(d2pdrho2_T*dpdT_rho*dpdT_rho -2*dpdT_rho*dpdrho_T*d2pdrhodT + dpdrho_T*dpdrho_T*d2pdT2_rho);

		// vapour saturation line rho plus 1st & 2nd derivatives of rho wrt T
		dpdT_rho  = TTD->dpdT_rho_fn(T,rhog,HELMD,&err);
		dpdrho_T  = TTD->dpdrho_T_fn(T,rhog,HELMD,&err);
		drhodT_p =  (-dpdT_rho )/(dpdrho_T);

		d2pdrho2_T = TTD->d2pdrho2_T_fn(T,rhog,HELMD,&err);
		d2pdrhodT = TTD->d2pdTdrho_fn(T,rhog,HELMD,&err);
		d2pdT2_rho = TTD->d2pdT2_rho_fn(T,rhog,HELMD,&err);

		//   ddrho_drhodT_p_constT = ( dpdT_rho*d2pdrho2_T - dpdrho_T*d2pdrhodT ) / pow(dpdrho_T,2);
		//   ddT_drhodT_p_constrho = ( dpdT_rho*d2pdrhodT - dpdrho_T*d2pdT2_rho ) / pow(dpdrho_T,2);

		TABLE->satGRho[i] = rhog;
		TABLE->satGdRhodt[i] = drhodT_p;
		//PT->satGd2RhodT2[i] =  ddT_drhodT_p_constrho  +  ddrho_drhodT_p_constT * drhodT_p;
		TABLE->satGd2RhodT2[i] =  (-1.0/pow(dpdrho_T,3))*( d2pdrho2_T*dpdT_rho*dpdT_rho -2*dpdT_rho*dpdrho_T*d2pdrhodT + dpdrho_T*dpdrho_T*d2pdT2_rho );

		// MSG("%f  %f  %f ---  %f  %f  %f",PT->satFRho[i] , PT->satFdRhodt[i], PT->satFd2RhodT2[i],PT->satGRho[i] , PT->satGdRhodt[i], PT->satGd2RhodT2[i]) ;
	}

	double Tmin,Tmax,rhomin,rhomax;
	Tmin = TTD->Tmin;
	Tmax = TTD->Tmax;
	rhomin = TTD->rhomin;
	rhomax = TTD->rhomax;
	dT = (Tmax-Tmin)/NTP;
	double drho = (rhomax-rhomin)/NRHOP;
	MSG("DT = %f K, Drho =  %f kg/m3",dT,drho);

	MSG("Building main TTSE tables...");
	clock_t start = clock();
	for( i = 0; i < NTP; i++){
		for( j = 0; j < NRHOP; j++){
#ifdef TTSE_DEBUG
			if(0 == j + i*NRHOP % 300){
				fprintf(stderr," %5.1f %%\r",100*(j+i*(float)(NRHOP))/(NRHOP*NTP));
			}
#endif
			double t  = Tmin+i*dT;
			double rho  = rhomin+j*drho;
#define X
#define EVAL_SET(VAR) TTSE_SET(TABLE,VAR,i,j,TTD->VAR##_fn(t,rho,HELMD,&err));
		TTSE_MATRICES(X, EVAL_SET)
#undef EVAL_SET
#undef X
		}
	}

	clock_t end = clock();
	double msec = (double)(end - start) / (CLOCKS_PER_SEC/1000);
	MSG("Tables built in %f seconds", msec/1000);
	TTD->IsTableBuilt=1;
#undef PT
#undef TTD
#undef TABLE

}

/*------------------------------------------------------------------------------
  TTSE EVALUATION ROUTINES
*/

double ttse_sat(double T, double *rhof_out, double * rhog_out, const FluidData *data, FpropsError *err){
#define TTD data->corr.ttse
#define TABLE data->corr.ttse->table
	int i,j;
	double Tmin = data->T_t;
	double Tmax = data->T_c;

	if(T < Tmin-1e-8){
		ERRMSG("Input Temperature %f K is below triple-point temperature %f K",T,data->T_t);
		return FPROPS_RANGE_ERROR;
	}

	if(T > Tmax+1e-8){
		ERRMSG("Input Temperature is above critical point temperature");
		*err = FPROPS_RANGE_ERROR;
	}

	double dT = (Tmax-Tmin)/NSAT;
	i = (int)(((T - Tmin)/(Tmax - Tmin)*(NSAT)));

	assert(i>=0 && i<NSAT);
	double delt = T - ( Tmin + i*dT);
	*rhof_out =  TABLE->satFRho[i] + delt*TABLE->satFdRhodt[i] + 0.5*delt*delt*TABLE->satFd2RhodT2[i];
	*rhog_out =  TABLE->satGRho[i] + delt*TABLE->satGdRhodt[i] + 0.5*delt*delt*TABLE->satGd2RhodT2[i];
	/* return Psat from the single phase table        */
	Tmin = TTD->Tmin;
	Tmax = TTD->Tmax;
	double rhomin  = TTD->rhomin;
	double rhomax = TTD->rhomax;

	dT = (Tmax-Tmin)/NTP;
	double drho = (rhomax-rhomin)/NRHOP;
	i = (int)round(((T-Tmin)/(Tmax-Tmin)*(NTP)));
	j = (int)round(((*rhog_out-rhomin)/(rhomax-rhomin)*(NRHOP)));

	assert(i>=0&&i<NTP);
	assert(j>=0&&j<NRHOP);
	delt = T - ( Tmin + i*dT);
	double delrho = *rhog_out - ( rhomin + j*drho);
	//  MSG("%d  %d  %f  %f  %f  %f  %f  %f  %f",i,j,T,*rhof_out,*rhog_out,Tmin,Tmax,rhomin,rhomax);
	double ttseP = TTSE_GET(TABLE,p,i,j)
		 + delt*TTSE_GET(TABLE,dpdT_rho,i,j) + 0.5*delt*delt*TTSE_GET(TABLE,d2pdT2_rho,i,j)
		 + delrho*TTSE_GET(TABLE,dpdrho_T,i,j) + 0.5*delrho*delrho*TTSE_GET(TABLE,d2pdrho2_T,i,j)
		 + delrho*delt*TTSE_GET(TABLE,d2pdTdrho,i,j);
	return ttseP; // return P_sat

#undef PT
#undef TTD
#undef TABLE
	}

#ifdef SO /* second-order Taylor series expansion */
# define EVALTTSEFN(VAR) \
	double ttse_##VAR(double T, double rho, const FluidData *data, FpropsError *err){\
		int i,j;\
		assert(data->corr.ttse->IsTableBuilt);\
		const TtseData *td = data->corr.ttse;\
		const Ttse *table = data->corr.ttse->table;\
		double Tmin = td->Tmin;\
		double Tmax = td->Tmax;\
		double rhomin = td->rhomin;\
		double rhomax = td->rhomax;\
		double dT = (Tmax-Tmin)/NTP;\
		double drho = (rhomax-rhomin)/NRHOP;\
		i = (int)round(((T-Tmin)/(Tmax-Tmin)*(NTP)));\
		j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP)));\
		double delt = T - ( Tmin + i*dT);\
		double delrho = rho - ( rhomin + j*drho);\
		double val = TTSE_GET(table,VAR,i,j)\
			 + delt*TTSE_GET(table,d##VAR##dT_rho,i,j) + 0.5*delt*delt*TTSE_GET(table,d2##VAR##dT2_rho,i,j)\
			 + delrho*TTSE_GET(table,d##VAR##drho_T,i,j) + 0.5*delrho*delrho*TTSE_GET(table,d2##VAR##drho2_T,i,j)\
			 + delrho*delt*TTSE_GET(table,d2##VAR##dTdrho,i,j);\
		return val;\
		}
/*  snippet for generic calls
		double Tmin = P->data->T_t;\
		double Tmax = P->data->T_c;\
		if(t >= Tmin  && t< Tmax) {\
			ttse_sat(t, &rho_f, &rho_g, P, &err);\
			if(rho_g < rho && rho < rho_f){\
				double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);\
				double Qf = P->VAR##_fn( t,rho_f,P->data,&err);\
				double Qg = P->VAR##_fn( t,rho_g,P->data,&err);\
				return x*Qg + (1-x)*Qf;\
			}\
		}\*/
#else /* first-order Taylor series expansion */
# define EVALTTSEFNFO(VAR) \
	double ttse_##VAR(PureFluid *P , double T, double rho){\
			int i,j;\
			double Tmin = P->table->Tmin; double Tmax = P->table->Tmax;\
			double rhomin  = P->table->rhomin; double rhomax= P->table->rhomax;\
			double dT = (Tmax-Tmin)/NTP;\
			double drho = (rhomax-rhomin)/NRHOP;\
			i = (int)round(((T-Tmin)/(Tmax-Tmin)*(NTP-1)));\
			j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP-1)));\
			double delt = T - ( Tmin + i*dT);\
			double delrho = rho - ( rhomin + j*drho);\
			double ttse##VAR = P->table->VAR[i][j]\
				 + delt*P->table->d##VAR##dT[i][j] \
				 + delrho*P->table->d##VAR##drho[i][j] ;\
			return ttse##VAR;\
		}
#endif

#define EVALFNUNDEF(VAR) \
	double ttse_##VAR(double T, double rho, const FluidData *data, FpropsError *err){ \
		ERRMSG("TTSE function for '" #VAR "' is not yet implemented"); \
		*err = FPROPS_NOT_IMPLEMENTED; \
		return -1e99; \
	}

#ifdef SO // second-order TTSE evaluation
EVALTTSEFN(p);
EVALTTSEFN(h);
EVALTTSEFN(s);
EVALTTSEFN(u);
EVALTTSEFN(g);
EVALFNUNDEF(a);
EVALFNUNDEF(cp);
EVALFNUNDEF(cv);
EVALFNUNDEF(w);
EVALFNUNDEF(dpdrho_T);
EVALFNUNDEF(alphap);
EVALFNUNDEF(betap);
#else // first order TTSE evaluation
EVALTTSEFNFO(p);
EVALTTSEFNFO(h);
EVALTTSEFNFO(s);
EVALTTSEFNFO(g);
EVALTTSEFNFO(u);
#endif

/*------------------------------------------------------------------------------
	FILE CACHING OF TTSE CALCULATED RESULTS... OPTIONAL COMPONENT
*/

#ifdef TTSE_USE_FILE_CACHE

/* FIXME this table folder needs to be an absolute location, writable, and
should also work on c:\Windows\type\paths... more work required here */
#define TAB_FOLDER  "tables"

static void save_tables(PureFluid *P);
static int doesdbexist(PureFluid *P);
static void load_tables(PureFluid *P);

/**
	This will load the binary file from tables/ for the liquid of interest and the EOS and populate the matrices.
	If the files are not present in tables/ then build_tables() should be used.

	FIXME need to standardise the location of the 'tables' folder...?
*/
static void load_tables(PureFluid *P){
	MSG("Table file exists @ %s",P->data->path);
	FILE * readtablefile = fopen(P->data->path,"rb");

#define RD(VAR) fread( P->data->table->VAR, sizeof(double), NSAT, readtablefile );
	RD(satFRho); RD(satFdRhodt); RD(satFd2RhodT2);
	RD(satGRho); RD(satGdRhodt); RD(satGd2RhodT2);
#undef RD

#define X
#define RD(VAR) fread(P->data->table->VAR, sizeof(double), NRHOP*NTP, readtablefile);
	TTSE_MATRICES(X, RD)
#undef RD
#undef X

	fclose(readtablefile);
	P->data->IsTableBuilt=1;
}

/**
	After building the tables once this should be called to save the files in binary inside tables/
*/
static void save_tables(PureFluid *P){
	MSG("Saving table %s",P->data->path);
	FILE * writetablefile = fopen(P->data->path,"wb");

#define WR(VAR) fwrite( P->data->table->VAR, sizeof(double), NSAT, writetablefile);
	WR(satFRho); WR(satFdRhodt); WR(satFd2RhodT2);
	WR(satGRho); WR(satGdRhodt); WR(satGd2RhodT2);
#undef WR

#define X
#define WR(VAR) fwrite(P->data->table->VAR,sizeof(double), NRHOP*NTP, writetablefile);
	TTSE_MATRICES(X, WR)
#undef WR
#undef X
	fclose(writetablefile);
}

static int doesdbexist(PureFluid *P){
	char path[200]  = TAB_FOLDER;

	strcat(path,"/helm_");
	strcat(path, P->name );
	strcat(path,"_TR.bin");

	P->data->path = FPROPS_NEW_ARRAY( char,strlen(path)+1 );
	strcpy(P->data->path, path);
	P->data->path[strlen(path)]='\0';

	//  MSG("Table file path --> <%s>",P->path);

	FILE *test=fopen(P->data->path,"r");

	if(test){
		MSG("Saved Table Found");
		fclose(test);
		return 1;
	}

	MSG("NO Saved Table");
	return 0;
}

#endif


