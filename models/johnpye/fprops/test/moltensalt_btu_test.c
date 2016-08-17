// "Model of 60% NaNO3 + 40% KNO3 as specified in Sandia National Laboratories document SAND2001-2100 rev 0."


#include "../incompressible.h"
#include "../fprops.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>


#define BUF 1024

#define MAXC 10

#define PATH "incomp_misc/incomp_liq_data/moltensalt_btu.dat"

#define ABORT \
		printf("\nExiting Program!\n"); \
		exit(0);

// json parser
#define READ(Q)	\
		pos = ftell(in); \
		while(!strstr(word,"},")) { \
			fgets(word,BUF,in); \
			if(strstr(word,"\"type\": \"notdefined\"")) { \
				test_liq->Q.type = (char*)malloc(sizeof("notdefined")); \
				strcpy(test_liq->Q.type,"notdefined"); \
			} \
			else if(strstr(word,"\"type\": \"polynomial\"")) { \
				test_liq->Q.type = (char*)malloc(sizeof("polynomial")); \
				strcpy(test_liq->Q.type,"polynomial"); \
			} \
			else if(strstr(word,"\"type\": \"exppolynomial\"")) { \
				test_liq->Q.type = (char*)malloc(sizeof("exppolynomial")); \
				strcpy(test_liq->Q.type,"exppolynomial"); \
			} \
			else if(strstr(word,"\"type\": \"exponential\"")) { \
				test_liq->Q.type = (char*)malloc(sizeof("exponential")); \
				strcpy(test_liq->Q.type,"exponential"); \
			} \
		} \
		if(strcmp(test_liq->Q.type,"notdefined")) { \
			fseek(in,pos,SEEK_SET); \
			fgets(word,BUF,in); \
			double coefs[MAXC][MAXC]; \
			int numc_row = 0, numc_col = 0; \
			while(!strstr(word,"},")) { \
				if((strstr(word,"e+")||strstr(word,"e-"))&&!strstr(word,"NRMS")) { \
					coefs[numc_row][numc_col] = to_number(word); \
					numc_col++; \
					assert(numc_col<MAXC);  \
				} \
				else if(strstr(word,"],")) { \
					numc_row++; \
					test_liq->Q.numc_c = numc_col; \
					numc_col = 0; \
					assert(numc_row<MAXC);  \
				} \
				fgets(word,BUF,in); \
			} \
			test_liq->Q.numc_r = numc_row; \
			test_liq->Q.coeff = (double**)malloc(test_liq->Q.numc_r*sizeof(double*)); \
			int i,j; \
			for(i=0;i<test_liq->Q.numc_r;i++) \
				test_liq->Q.coeff[i] = (double*)malloc(test_liq->Q.numc_c*sizeof(double)); \
			for(i=0;i<test_liq->Q.numc_r;i++) \
				for(j=0;j<test_liq->Q.numc_c;j++) \
					test_liq->Q.coeff[i][j] = coefs[i][j]; \
		} 

typedef struct {

	char* type;
	double** coeff;
	int numc_r, numc_c;	// number of rows and columns in coefficient matrix respectively

} coefficients;

typedef struct {

	coefficients T_freeze, conductivity, density, specific_heat, viscosity, saturation_pressure;
	char* x_id;
	char* description;
	double T_base, T_max, T_min, T_minPsat, x_base, x_max, x_min; 

} fprops;

double extracted_number(char w[BUF]) {

	char *token;
   
	/* get the first token */
	token = strtok(w, " ,");

	/* get the second token which is the number*/
	token = strtok(NULL, " ,");

	return (atof(token));

}

double to_number(char w[BUF]) {

	char *token;
   
	/* get the first token */
	token = strtok(w, " ,");

	return (atof(token));

}
/*
// evaluation function for polynamial type coefficients
double eval_poly(coefficients c, double T, double x) {

	double res = 0.0;
	int i,j;

	for(i=0;i<c.numc_r;i++)
		for(j=0;j<c.numc_c;j++)
			res += pow(x,j)*c.coeff[i][j]*pow(T,i);

	return res;

}

// evaluation function for exponential-polynomial type coefficients
double eval_exppoly(coefficients c, double T, double x) {

	return exp(eval_poly(c,T,x));

}

// evaluation function for exponential type coefficients
double eval_expo(coefficients c, double T, double x) {

	return exp(c.coeff[0][0]/(T+c.coeff[0][1])-c.coeff[0][2]);

}	

// generalized property evaluation functions: macro definition
#define PROP_EVAL(Q) \
	double eval_ ## Q (fprops *fluid, double T, double x) { \
		assert(T>0&&T>=fluid->T_min&&T<=fluid->T_max); \
		assert(x>=fluid->x_min&&x<=fluid->x_max); \
		if(!strcmp(fluid->Q.type,"polynomial")) return eval_poly(fluid->Q,T-fluid->T_base,x-fluid->x_base); \
		else if(!strcmp(fluid->Q.type,"exppolynomial")) return eval_exppoly(fluid->Q,T-fluid->T_base,x-fluid->x_base); \
		else if(!strcmp(fluid->Q.type,"exponential")) return eval_expo(fluid->Q,T,x); \
		else { \
			printf("\nType not defined.\n"); \
			ABORT \
		} \
	}

// declaration and definition of property evaluation functions for conductivity, density, specific-heat and viscosity
PROP_EVAL(conductivity)
PROP_EVAL(density)
PROP_EVAL(specific_heat)
PROP_EVAL(viscosity)
PROP_EVAL(saturation_pressure)
*/

int main() {

// parsing from input (json format) and loading data to new fluid data structures
	FILE* in;
	IncompressibleData* test_liq;
	test_liq = (IncompressibleData*)malloc(sizeof(IncompressibleData));

	long int pos;

	if(!(in = fopen(PATH,"r"))) {

		printf("\nInput file cannot be located.\n");
		ABORT

	}

	char word[BUF];

	while(!feof(in)) {

		fgets(word,BUF,in); 

		if(strstr(word,"\"T_freeze\"")) { READ(T_freeze); } // not required for incompressible EOS; implemented to check input coefficients of "type": "notdefined"
		else if(strstr(word,"\"Tbase\"")) test_liq->T_base = extracted_number(word);
		else if(strstr(word,"\"Tmax\"")) test_liq->T_max = extracted_number(word); 
		else if(strstr(word,"\"Tmin\"")) test_liq->T_min = extracted_number(word); 
		else if(strstr(word,"\"TminPsat\"")) test_liq->T_minPsat = extracted_number(word);
		else if(strstr(word,"\"conductivity\"")) { READ(conductivity); }
		else if(strstr(word,"\"density\"")) { READ(density); }
		else if(strstr(word,"\"specific_heat\"")) { READ(specific_heat); }
		else if(strstr(word,"\"viscosity\"")) { READ(viscosity); }
		else if(strstr(word,"\"saturation_pressure\"")) { READ(saturation_pressure); }
		else if(strstr(word,"\"xbase\"")) test_liq->x_base = extracted_number(word);
		else if(strstr(word,"\"xmax\"")) test_liq->x_max = extracted_number(word);
		else if(strstr(word,"\"xmin\"")) test_liq->x_min = extracted_number(word);
		else if(strstr(word,"\"xid\": \"pure\"")) {
				test_liq->x_id = (char*)malloc(sizeof("pure")); 
				strcpy(test_liq->x_id,"pure"); 
		}

	}

	fclose(in);

	const EosData eos_moltensalt = {
		"moltensalt"
		,"test"
		, NULL
		,100
		,FPROPS_INCOMPRESSIBLE
		,.data = {.incomp = test_liq}
	};

// test
	PureFluid *P = incompressible_prepare(&eos_moltensalt, NULL);
	FpropsError fprops_err = FPROPS_NO_ERROR;

#define D P->data->corr.incomp	

	double T = 500.0;
	double p = 1.0; // no pressure dependencies yet (may be required for enthalpy evaluation)

	//FILE *out;
	//out = fopen("test_res.dat","w");
	// each row = {density, specific heat, viscosity, thermal conductivity} all in british units ... data from http://prod.sandia.gov/techlib/access-control.cgi/2001/012100.pdf
	double testpoints[13][5] = 	{{500, 120.10, 0.356, 10.5058, 0.284557},
					 {550, 118.98, 0.358, 8.6073, 0.287692},
					 {600, 117.87, 0.359, 7.0853, 0.290827},
					 {650, 116.76, 0.360, 5.8940, 0.293962},
	 				 {700, 115.65, 0.361, 4.9873, 0.297097},
					 {750, 114.54, 0.362, 4.3196, 0.300232},
					 {800, 113.43, 0.363, 3.8450, 0.303367},
					 {850, 112.32, 0.364, 3.5175, 0.306502},
					 {900, 111.21, 0.366, 3.2913, 0.309637},
					 {950, 110.10, 0.367, 3.1206, 0.312771},
					 {1000, 108.99, 0.368, 2.9596, 0.315906},
					 {1050, 107.88, 0.369, 2.7623, 0.319041},
					 {1100, 106.77, 0.370, 2.4830, 0.322176}};
	int point = 0;
	//fprintf(out,"VARIABLES = \"Temperature [C]\", \"Cp [W/m/K]\", \"Cv [W/m/K]\", \"Density [kg/m<sup>3</sup>]\", \"Heat Capacity [J/Kg/K]\", \"Viscosity [Pa s]\", \"Internal Energy [SI]\", \"Entropy [SI]\"\n");
	do {
		FluidState S = fprops_set_Tp(T,p,P,&fprops_err);
		//fprintf(out,"%e\t%e\t%e\t%e\t%e\t%e\t%e\t%e\n",T-273.0,fprops_cp(S,&fprops_err),fprops_cv(S,&fprops_err),
		//fprops_rho(S,&fprops_err),fprops_lam(S,&fprops_err),fprops_mu(S,&fprops_err),fprops_u(S,&fprops_err),fprops_s(S,&fprops_err));
		printf("\nTest point: %d {Temperature \tDensity \tSpecific Heat \tViscosity \tConductivity}\n", point);
		printf("FPROPS at T = %e\t%e\t%e\t%e\t%e\n",T,fprops_rho(S,&fprops_err),fprops_cp(S,&fprops_err),fprops_mu(S,&fprops_err),fprops_lam(S,&fprops_err));
		printf("Error at T =  %e\t%e\t%e\t%e\t%e\n",T,abs(fprops_rho(S,&fprops_err)-testpoints[point][1])/testpoints[point][1],abs(fprops_cp(S,&fprops_err)-testpoints[point][2])/testpoints[point][2],abs(fprops_mu(S,&fprops_err)-testpoints[point][3])/testpoints[point][3],abs(fprops_lam(S,&fprops_err)-testpoints[point][4])/testpoints[point][4]);
		T+=50;
		point+=1;

	} while(T<=D->T_max);

	//fclose(out);

	return 0;

}



