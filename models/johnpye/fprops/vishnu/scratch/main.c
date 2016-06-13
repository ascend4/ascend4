
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define BUF 1024

#define MAXC 10

#define PATH "incomp_liq_data/ZS55.dat"

#define ABORT \
		printf("\nExiting Program!\n"); \ 
		exit(0);

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
			double coefs[MAXC]; \
			int numc = 0; \
			while(!strstr(word,"},")) { \
				if(strstr(word,"NRMS")) { \
					coefs[numc] = extracted_number(word); \
					numc++; \
					assert(numc==1); \
				} \
				else if(!strstr(word,"coeffs")&&strstr(word,"[")) { \
					fgets(word,BUF,in); \
					coefs[numc] = to_number(word); \
					numc++; \
					assert(numc<MAXC);  \
				} \
				fgets(word,BUF,in); \
			} \
			test_liq->Q.coeff = (double*)malloc(numc*sizeof(double)); \
			test_liq->Q.numc = numc; \
			while(numc) { \
				numc--; \
				test_liq->Q.coeff[numc] = coefs[numc]; \
			} \
		} 

typedef struct {

	char* type;
	double* coeff;
	int numc;	

} coefficients;

typedef struct {

	coefficients T_freeze, conductivity, density, specific_heat, viscosity;
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
	token = strtok(w, " ");

	return (atof(token));

}

// eveluation function for polynamial type coefficients
double eval_poly(coefficients c, double T, double x) {

	double res = 0.0;
	int i;

	for(i=1;i<c.numc;i++)
		res += c.coeff[i]*pow(T,i-1);

	return res;

}

// eveluation function for exponential-polynomial type coefficients
double eval_exppoly(coefficients c, double T, double x) {

	return exp(eval_poly(c,T,x));

}

// eveluation function for exponential type coefficients
double eval_expo(coefficients c, double T, double x) {

	return exp(c.coeff[1]/(T+c.coeff[2])+c.coeff[3]);

}	

// generalized property evaluation functions: macro definition
#define PROP_EVAL(Q) \
	double eval_ ## Q (fprops *fluid, double T, double x) { \
		assert(T>0&&T>=fluid->T_min&&T<=fluid->T_max); \
		assert(x>=fluid->x_min&&x<=fluid->x_max); \
		if(!strcmp(fluid->Q.type,"polynomial")) return eval_poly(fluid->Q,T,x); \
		else if(!strcmp(fluid->Q.type,"exppolynomial")) return eval_exppoly(fluid->Q,T,x); \
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

int main() {

// parsing from input (json format) and loading data to new fluid data structures
	FILE* in;

	fprops* test_liq;
	test_liq = (fprops*)malloc(sizeof(fprops));

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
		else if(strstr(word,"\"xbase\"")) test_liq->x_base = extracted_number(word);
		else if(strstr(word,"\"xmax\"")) test_liq->x_max = extracted_number(word);
		else if(strstr(word,"\"xmin\"")) test_liq->x_min = extracted_number(word);
		else if(strstr(word,"\"xid\": \"pure\"")) {
				test_liq->x_id = (char*)malloc(sizeof("pure")); 
				strcpy(test_liq->x_id,"pure"); 
		}

	}

	fclose(in);

// test
	double T = test_liq->T_min;
	
	FILE *out;
	out = fopen("test_res.dat","w");

	fprintf(out,"VARIABLES = \"Temperature [C]\", \"Conductivity [W/m/K]\", \"Density [kg/m<sup>3</sup>]\", \"Heat Capacity [J/Kg/K]\", \"Viscosity [Pa s]\"\n");
	do {

		fprintf(out,"%e\t%e\t%e\t%e\t%e\n",T-273.0,eval_conductivity(test_liq, T, 1),eval_density(test_liq, T, 1),eval_specific_heat(test_liq, T, 1),eval_viscosity(test_liq, T, 1));
		T+=1;

	} while(T<=test_liq->T_max);

	fclose(out);

	return 0;

}
