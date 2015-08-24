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

#define NRHOP 200
#define NTP 200

#define TTSE_DEBUG //sid change
#ifdef TTSE_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif

#include <ascend/general/ascMalloc.h>
#include <ascend/utilities/error.h>

#define TAB_FOLDER  "tables"

#define SO
//#define TTSE_USE_FILE_CACHE

/* Note, at the moment, all our TTSE tables are fixed size, per #defs for NTP and NRHOP */

#define TTSE_ALLOC(TP, RHOP) ASC_NEW_ARRAY(double,TP*RHOP);
#define TTSE_FREE(MAT) ASC_FREE(MAT)
#define TTSE_SET(TAB,NAME,TI,RHOI,VAL) TAB->NAME[TI*NRHOP + RHOI] = VAL
#define TTSE_GET(TAB,NAME,TI,RHOI) TAB->NAME[TI*NRHOP + RHOI]

/* NOTE, the #define list 'TTSE_MATRICES' list is defined in rundata.h. */

/* TODO refine the above to separate first-order and second-order derivs */

void alloc_tables(Ttse * table){
	table->satFRho =  ASC_NEW_ARRAY ( double , NSAT);
	table->satFdRhodt =  ASC_NEW_ARRAY (double, NSAT);
	table->satFd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);
	table->satGRho =  ASC_NEW_ARRAY (double, NSAT);
	table->satGdRhodt =  ASC_NEW_ARRAY (double, NSAT);
	table->satGd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);

#define X ;
#define I(M) table->M = TTSE_ALLOC(NTP,NRHOP)
	TTSE_MATRICES(X,I);
#undef X
#undef I
}

void remove_tables(Ttse *table){
	ASC_FREE(table->satFRho );
	ASC_FREE(table->satFdRhodt );
	ASC_FREE(table->satFd2RhodT2 );
	ASC_FREE(table->satGRho );
	ASC_FREE(table->satGdRhodt );
	ASC_FREE(table->satGd2RhodT2 );

#define X ;
#define I(M) TTSE_FREE(table->M);table->M = NULL;
	TTSE_MATRICES(X,I)
#undef X
#undef I
}

/*
    Actual building of tables is done here.
*/
void build_tables(PureFluid *P){
#undef PT
#define PT P->data->table

	int i,j;
	FpropsError err = FPROPS_NO_ERROR;

	double Tt = P->data->T_t;
	double Tc = P->data->T_c;
	double dt = (Tc - Tt)/(NSAT);

	double  rho1,rho2;
	P->sat_fn(Tc,&rho1,&rho2,P->data,&err);
	MSG("triple point and critical temperature and critical density-->  %f  %f  %f",Tt,Tc,rho1);

	MSG("Building saturation tables...");
	for(i=0; i<NSAT; ++i)    {

		double T = Tt + i*dt;

		double  rhof,rhog;
		P->sat_fn(T,&rhof,&rhog,P->data,&err);

		//The fluid saturation line rho's and 1st & 2nd  derivatives of rho with respect to T

		double dpdT_rho  = P->dpdT_rho_fn(T,rhof,P->data,&err);
		double dpdrho_T  = P->dpdrho_T_fn(T,rhof,P->data,&err);
		double drhodT_p =  (-dpdT_rho )/(dpdrho_T);

		double d2pdrho2_T = P->d2pdrho2_T_fn(T,rhof,P->data,&err);
		double d2pdrhodT = P->d2pdTdrho_fn(T,rhof,P->data,&err);
		double d2pdT2_rho = P->d2pdT2_rho_fn(T,rhof,P->data,&err);

		PT->satFRho[i] = rhof;
		PT->satFdRhodt[i] = drhodT_p;
		//PT->satFd2RhodT2[i] =  ddT_drhodT_p_constrho  +  ddrho_drhodT_p_constT * drhodT_p;
		PT->satFd2RhodT2[i] =  (-1.0/pow(dpdrho_T,3))*( d2pdrho2_T*dpdT_rho*dpdT_rho -2*dpdT_rho*dpdrho_T*d2pdrhodT + dpdrho_T*dpdrho_T*d2pdT2_rho );

		//The Vapour saturation line rho's and 1st & 2nd derivatives of rho with respect to T

		dpdT_rho  = P->dpdT_rho_fn(T,rhog,P->data,&err);
		dpdrho_T  = P->dpdrho_T_fn(T,rhog,P->data,&err);
		drhodT_p =  (-dpdT_rho )/(dpdrho_T);

		d2pdrho2_T = P->d2pdrho2_T_fn(T,rhog,P->data,&err);
		d2pdrhodT = P->d2pdTdrho_fn(T,rhog,P->data,&err);
		d2pdT2_rho = P->d2pdT2_rho_fn(T,rhog,P->data,&err);

		//   ddrho_drhodT_p_constT = ( dpdT_rho*d2pdrho2_T - dpdrho_T*d2pdrhodT ) / pow(dpdrho_T,2);
		//   ddT_drhodT_p_constrho = ( dpdT_rho*d2pdrhodT - dpdrho_T*d2pdT2_rho ) / pow(dpdrho_T,2);

		PT->satGRho[i] = rhog;
		PT->satGdRhodt[i] = drhodT_p;
		//PT->satGd2RhodT2[i] =  ddT_drhodT_p_constrho  +  ddrho_drhodT_p_constT * drhodT_p;
		PT->satGd2RhodT2[i] =  (-1.0/pow(dpdrho_T,3))*( d2pdrho2_T*dpdT_rho*dpdT_rho -2*dpdT_rho*dpdrho_T*d2pdrhodT + dpdrho_T*dpdrho_T*d2pdT2_rho );

		//MSG("%f  %f  %f ---  %f  %f  %f",PT->satFRho[i] , PT->satFdRhodt[i], PT->satFd2RhodT2[i],PT->satGRho[i] , PT->satGdRhodt[i], PT->satGd2RhodT2[i]) ;
	}

	double tmin,tmax,rhomin,rhomax;

	tmin = PT->tmin;
	tmax = PT->tmax;
	rhomin = PT->rhomin;
	rhomax = PT->rhomax;

	dt = (tmax-tmin)/NTP;
	double drho = (rhomax-rhomin)/NRHOP;
	MSG("DT = %f K, Drho =  %f kg/m3",dt,drho);

	MSG("Building main TTSE tables...");

	clock_t start = clock();

	for( i = 0; i < NTP; i++)
	for( j = 0; j < NRHOP; j++){
		double t  = tmin+i*dt;
		double rho  = rhomin+j*drho;
#define X
#define EVAL_SET(VAR) TTSE_SET(PT,VAR,i,j,P->VAR##_fn(t,rho,P->data,&err));
		TTSE_MATRICES(X, EVAL_SET)
#undef EVAL_SET
#undef X
	}

	clock_t end = clock();
	double msec = (double)(end - start) / (CLOCKS_PER_SEC/1000);
	MSG("Tables built in %f seconds", msec/1000);
	P->data->IsTableBuilt=1;
#undef PT
}


double evaluate_ttse_sat(double T, double *rhof_out, double * rhog_out, PureFluid *P, FpropsError *err){
#undef PT
#define PT P->data->table
	int i,j;
	double tmin = P->data->T_t;
	double tmax = P->data->T_c;
	if(T < tmin-1e-8){
		ERRMSG("Input Temperature %f K is below triple-point temperature %f K",T,P->data->T_t);
		return FPROPS_RANGE_ERROR;
	}

	if(T > tmax+1e-8){
		ERRMSG("Input Temperature is above critical point temperature");
		*err = FPROPS_RANGE_ERROR;
	}

	double dt = (tmax-tmin)/NSAT;
	i = (int)round(((T - tmin)/(tmax - tmin)*(NSAT)));
	assert(i>=0 && i<NSAT);
	double delt = T - ( tmin + i*dt);
	*rhof_out =  PT->satFRho[i] + delt*PT->satFdRhodt[i] + 0.5*delt*delt*PT->satFd2RhodT2[i];
	*rhog_out =  PT->satGRho[i] + delt*PT->satGdRhodt[i] + 0.5*delt*delt*PT->satGd2RhodT2[i];

	/* return Psat from the single phase table        */
	tmin = PT->tmin;
	tmax = PT->tmax;
	double rhomin  = PT->rhomin;
	double rhomax = PT->rhomax;

	dt = (tmax-tmin)/NTP;
	double drho = (rhomax-rhomin)/NRHOP;
	i = (int)round(((T-tmin)/(tmax-tmin)*(NTP)));
	j = (int)round(((*rhog_out-rhomin)/(rhomax-rhomin)*(NRHOP)));

	assert(i>=0&&i<NTP);
	assert(j>=0&&j<NRHOP);
	delt = T - ( tmin + i*dt);
	double delrho = *rhog_out - ( rhomin + j*drho);
	//  MSG("%d  %d  %f  %f  %f  %f  %f  %f  %f",i,j,T,*rhof_out,*rhog_out,tmin,tmax,rhomin,rhomax);
	double ttseP = TTSE_GET(PT,p,i,j)
		 + delt*TTSE_GET(PT,dpdT_rho,i,j) + 0.5*delt*delt*TTSE_GET(PT,d2pdT2_rho,i,j)
		 + delrho*TTSE_GET(PT,dpdrho_T,i,j) + 0.5*delrho*delrho*TTSE_GET(PT,d2pdrho2_T,i,j)
		 + delrho*delt*TTSE_GET(PT,d2pdTdrho,i,j);
	return ttseP; // return P_sat

	#undef PT
	}

#ifdef SO /* second-order Taylor series expansion */
# define EVALTTSEFN(VAR) \
	double evaluate_ttse_##VAR( double t, double rho , Ttse* table){\
		int i,j;\
		double tmin = table->tmin;\
		double tmax = table->tmax;\
		double rhomin = table->rhomin;\
		double rhomax = table->rhomax;\
		double dt = (tmax-tmin)/NTP;\
		double drho = (rhomax-rhomin)/NRHOP;\
		i = (int)round(((t-tmin)/(tmax-tmin)*(NTP)));\
		j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP)));\
		double delt = t - ( tmin + i*dt);\
		double delrho = rho - ( rhomin + j*drho);\
		double val = TTSE_GET(table,VAR,i,j)\
			 + delt*TTSE_GET(table,d##VAR##dT_rho,i,j) + 0.5*delt*delt*TTSE_GET(table,d2##VAR##dT2_rho,i,j)\
			 + delrho*TTSE_GET(table,d##VAR##drho_T,i,j) + 0.5*delrho*delrho*TTSE_GET(table,d2##VAR##drho2_T,i,j)\
			 + delrho*delt*TTSE_GET(table,d2##VAR##dTdrho,i,j);\
		return val;\
		}
/*  snippet for generic calls
        double tmin = P->data->T_t;\
        double tmax = P->data->T_c;\
		if(t >= tmin  && t< tmax) {\
            evaluate_ttse_sat(t, &rho_f, &rho_g, P, &err);\
            if(rho_g < rho && rho < rho_f){\
                    double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);\
                    double Qf = P->VAR##_fn( t,rho_f,P->data,&err);\
                    double Qg = P->VAR##_fn( t,rho_g,P->data,&err);\
                    return x*Qg + (1-x)*Qf;\
                }\
            }\*/
#else /* first-order Taylor series expansion */
# define EVALTTSEFNFO(VAR) \
	double evaluate_ttse_##VAR(PureFluid *P , double t, double rho){\
			int i,j;\
			double tmin = P->table->tmin; double tmax = P->table->tmax;\
			double rhomin  = P->table->rhomin; double rhomax= P->table->rhomax;\
			double dt = (tmax-tmin)/NTP;\
			double drho = (rhomax-rhomin)/NRHOP;\
			i = (int)round(((t-tmin)/(tmax-tmin)*(NTP-1)));\
			j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP-1)));\
			double delt = t - ( tmin + i*dt);\
			double delrho = rho - ( rhomin + j*drho);\
			double ttse##VAR = P->table->VAR[i][j]\
				 + delt*P->table->d##VAR##dt[i][j] \
				 + delrho*P->table->d##VAR##drho[i][j] ;\
			return ttse##VAR;\
		}
#endif

#ifdef SO // second-order TTSE evaluation
EVALTTSEFN(p);
EVALTTSEFN(h);
EVALTTSEFN(s);
EVALTTSEFN(g);
EVALTTSEFN(u);
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


/*------------------------------------------------------------------------------
	PREPARE DATA, INITIALISATION
*/

void ttse_prepare(PureFluid *P){
#ifdef TTSE_DEBUG
	//FILE *F1 = fopen("ttse.txt","w");
	//fprintf(F1,"%f   %f\n",t, P->p_fn( t, rho , P->data,&err) );
#endif

	if(P->data->IsTableBuilt)
		return;

	MSG("Inside TTSE");

	P->data->table = FPROPS_NEW(Ttse);
	alloc_tables(P->data->table);

	//Pseudo values for water
	/* FIXME TODO XXX Should be implemented elsewhere on per-fluid basis */
	P->data->table->tmin = 200;
	P->data->table->tmax = 4200;
	P->data->table->rhomin = 0.0001;
	P->data->table->rhomax = 2000;

#ifdef TTSE_USE_FILE_CACHE
	if(doesdbexist(P))//file exists in tables/
		load_tables(P);
	else{
		build_tables(P);
		save_tables(P);
	}
#else
	build_tables(P);
#endif

	//exit(1);
#ifdef TTSE_DEBUG
	//fclose(F1);
#endif
}

void ttse_destroy(PureFluid *P){
	remove_tables(P->data->table);
}

