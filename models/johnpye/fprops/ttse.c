
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include "rundata.h"
#include "ttse.h"



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

#define INIT -23456789.123


inline TtseMatrix alloc_matrix(int tp, int rhop) {

    TtseMatrix matrix =  ASC_NEW_ARRAY (TtseMatrix, tp*rhop);  //tp rows and rhop columns
/*
    matrix[10][24] = 99;
    printf("%f\n", *((double*)matrix+(tp*10)+24));
    matrix[20][44] = -99;
    printf("%f\n", *((double*)matrix+(tp*20)+44));
    matrix[30][64] = 199;
    printf("%f\n", *((double*)matrix+(tp*30)+64));
*/
    return matrix;
}


inline void remove_matrix(TtseMatrix mat , int tp){

    ASC_FREE(mat);
}


void alloc_tables(Ttse * table)
{

    table->s = alloc_matrix(NTP,NRHOP);
    table->dsdt = alloc_matrix(NTP,NRHOP);
    table->d2sdt2 = alloc_matrix(NTP,NRHOP);
    table->dsdrho = alloc_matrix(NTP,NRHOP);
    table->d2sdrho2 = alloc_matrix(NTP,NRHOP);
    table->d2sdtdrho = alloc_matrix(NTP,NRHOP);


    table->p = alloc_matrix(NTP,NRHOP);
    table->dpdt = alloc_matrix(NTP,NRHOP);
    table->d2pdt2 = alloc_matrix(NTP,NRHOP);
    table->dpdrho = alloc_matrix(NTP,NRHOP);
    table->d2pdrho2 = alloc_matrix(NTP,NRHOP);
    table->d2pdtdrho = alloc_matrix(NTP,NRHOP);


    table->u = alloc_matrix(NTP,NRHOP);
    table->dudt = alloc_matrix(NTP,NRHOP);
    table->d2udt2 = alloc_matrix(NTP,NRHOP);
    table->dudrho = alloc_matrix(NTP,NRHOP);
    table->d2udrho2 = alloc_matrix(NTP,NRHOP);
    table->d2udtdrho = alloc_matrix(NTP,NRHOP);


    table->g = alloc_matrix(NTP,NRHOP);
    table->dgdt = alloc_matrix(NTP,NRHOP);
    table->d2gdt2 = alloc_matrix(NTP,NRHOP);
    table->dgdrho = alloc_matrix(NTP,NRHOP);
    table->d2gdrho2 = alloc_matrix(NTP,NRHOP);
    table->d2gdtdrho = alloc_matrix(NTP,NRHOP);



    table->h = alloc_matrix(NTP,NRHOP);
    table->dhdt = alloc_matrix(NTP,NRHOP);
    table->d2hdt2 = alloc_matrix(NTP,NRHOP);
    table->dhdrho = alloc_matrix(NTP,NRHOP);
    table->d2hdrho2 = alloc_matrix(NTP,NRHOP);
    table->d2hdtdrho = alloc_matrix(NTP,NRHOP);
}


void remove_tables(Ttse *table)
{
    remove_matrix(table->dsdt,NTP);
    remove_matrix(table->d2sdt2,NTP);
    remove_matrix(table->dsdrho,NTP);
    remove_matrix(table->d2sdrho2,NTP);
    remove_matrix(table->d2sdtdrho,NTP);

    remove_matrix(table->dpdt,NTP);
    remove_matrix(table->d2pdt2,NTP);
    remove_matrix(table->dpdrho,NTP);
    remove_matrix(table->d2pdrho2,NTP);
    remove_matrix(table->d2pdtdrho,NTP);


    remove_matrix(table->dudt,NTP);
    remove_matrix(table->d2udt2,NTP);
    remove_matrix(table->dudrho,NTP);
    remove_matrix(table->d2udrho2,NTP);
    remove_matrix(table->d2udtdrho,NTP);

    remove_matrix(table->dgdt,NTP);
    remove_matrix(table->d2gdt2,NTP);
    remove_matrix(table->dgdrho,NTP);
    remove_matrix(table->d2gdrho2,NTP);
    remove_matrix(table->d2gdtdrho,NTP);

    remove_matrix(table->dhdt,NTP);
    remove_matrix(table->d2hdt2,NTP);
    remove_matrix(table->dhdrho,NTP);
    remove_matrix(table->d2hdrho2,NTP);
    remove_matrix(table->d2hdtdrho,NTP);

}



void nearest_neighbor(double t, double rho, int *i, int *j){



}


void load_tables(PureFluid *P){


}


void save_tables(PureFluid *P){


}

void build_tables(PureFluid *P){

    #ifndef PT
    #define PT P->table

    int i,j;
    FpropsError err = FPROPS_NO_ERROR;

    double tmin,tmax,rhomin,rhomax;

    PT->tmin = 260;
    PT->tmax = 400;
    PT->rhomin = 0.00001;
    PT->rhomax = 2;

    tmin = PT->tmin;
    tmax = PT->tmax;
    rhomin = PT->rhomin;
    rhomax = PT->rhomax;

    double dt = (tmax-tmin)/NTP;
    double drho = (rhomax-rhomin)/NRHOP;


    MSG("%e  %e",dt,drho);

    clock_t start = clock();

    for( i = 0; i < NTP; i++)
    for( j = 0; j < NRHOP; j++){

        double t  = tmin+i*dt;
        double rho  = rhomin+j*drho;

        PT->p[i][j] = P->p_fn( t, rho , P->data, &err);
        PT->dpdt[i][j] = P->dpdT_rho_fn( t, rho , P->data, &err);
        PT->dpdrho[i][j] = P->dpdrho_T_fn( t, rho , P->data, &err);
        PT->d2pdt2[i][j] = P->d2pdT2_rho_fn( t, rho , P->data, &err);
        PT->d2pdrho2[i][j] = P->d2pdrho2_T_fn( t, rho , P->data, &err);
        PT->d2pdtdrho[i][j] = P->d2pdTdrho_fn( t, rho , P->data, &err);


        PT->h[i][j] = P->h_fn( t, rho , P->data, &err);
        PT->dhdt[i][j] = P->dhdT_rho_fn( t, rho , P->data, &err);
        PT->dhdrho[i][j] = P->dhdrho_T_fn( t, rho , P->data, &err);
        PT->d2hdt2[i][j] = P->d2hdT2_rho_fn( t, rho , P->data, &err);
        PT->d2hdrho2[i][j] = P->d2hdrho2_T_fn( t, rho , P->data, &err);
        PT->d2hdtdrho[i][j] = P->d2hdTdrho_fn( t, rho , P->data, &err);


        PT->s[i][j] = P->s_fn( t, rho , P->data, &err);
        PT->dsdt[i][j] = P->dsdT_rho_fn( t, rho , P->data, &err);
        PT->dsdrho[i][j] = P->dsdrho_T_fn( t, rho , P->data, &err);
        PT->d2sdt2[i][j] = P->d2sdT2_rho_fn( t, rho , P->data, &err);
        PT->d2sdrho2[i][j] = P->d2sdrho2_T_fn( t, rho , P->data, &err);
        PT->d2sdtdrho[i][j] = P->d2sdTdrho_fn( t, rho , P->data, &err);
    }


    clock_t end = clock();
    double msec = (double)(end - start) / (CLOCKS_PER_SEC/1000);
    MSG("Table prepared in %f seconds", msec/1000);



    #undef PT
    #endif
}



#define EVALTTSEFN(VAR) \
	double evaluate_ttse_##VAR(PureFluid *P , double t, double rho){\
            int i,j;\
            FpropsError err = FPROPS_NO_ERROR;\
            double tmin = P->table->tmin; double tmax = P->table->tmax;\
            double rhomin  = P->table->rhomin; double rhomax= P->table->rhomax;\
            double dt = (tmax-tmin)/NTP;\
            double drho = (rhomax-rhomin)/NRHOP;\
            i = (int)round(((t-tmin)/(tmax-tmin)*(NTP-1)));\
            j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP-1)));\
            double delt = t - ( tmin + i*dt);\
            double delrho = rho - ( rhomin + j*drho);\
            double ttse##VAR = P->table->VAR[i][j]+ 0.5*delt*delt*P->table->d2##VAR##dt2[i][j]\
                 + delrho*P->table->d##VAR##drho[i][j] + 0.5*delrho*delrho*P->table->d2##VAR##drho2[i][j]\
                 + delrho*delt*P->table->d2##VAR##dtdrho[i][j];\
            return ttse##VAR;\
        }


EVALTTSEFN(p);
EVALTTSEFN(h);
EVALTTSEFN(s);



/*
double evaluate_ttse_p(PureFluid *P , double t, double rho)
{

    #ifndef PT
    #define PT P->table

    int i,j;
    FpropsError err = FPROPS_NO_ERROR;

    double tmin = PT->tmin;
    double tmax = PT->tmax;
    double rhomin  = PT->rhomin;
    double rhomax= PT->rhomax;

    double dt = (tmax-tmin)/NTP;
    double drho = (rhomax-rhomin)/NRHOP;

	i = (int)round(((t-tmin)/(tmax-tmin)*(NTP-1)));
	j = (int)round(((rho-rhomin)/(rhomax-rhomin)*(NRHOP-1)));

    double delt = t - ( tmin + i*dt);
    double delrho = rho - ( rhomin + j*drho);

    MSG("%f %f",i ,j ,delt ,delrho );
    MSG("%f  %f  %f  %f  %f  %f", PT->p[i][j], PT->dpdt[i][j], PT->d2pdt2[i][j], PT->dpdrho[i][j], PT->d2pdrho2[i][j], PT->d2pdtdrho[i][j]);


    double ttse_p = PT->p[i][j] + delt*PT->dpdt[i][j] + 0.5*delt*delt*PT->d2pdt2[i][j]
            + delrho*PT->dpdrho[i][j] + 0.5*delrho*delrho*PT->d2pdrho2[i][j]
            + delrho*delt*PT->d2pdtdrho[i][j];


    MSG("%e  %e", ttse_p , P->p_fn(t, rho, P->data,&err)  );


    return(ttse_p)
    #undef PT
    #endif
}

*/


void ttse_prepare(PureFluid *P){

#ifdef TTSE_DEBUG
	FILE *F1 = fopen("ttse.txt","w");
    //fprintf(F1,"%f   %f\n",t, P->p_fn( t, rho , P->data,&err) );
#endif


    if(!P->table->usettse)
        return;

    FpropsError err = FPROPS_NO_ERROR;

    MSG("Inside TTSE");

    alloc_tables(P->table);

    build_tables(P);

    MSG("helmholtz computed %f  ", P->p_fn(380, 1, P->data,&err) );
    MSG("TTSE Evaluated %f  ", evaluate_ttse_p(P,380, 1) );



    MSG("helmholtz computed %f  ", P->h_fn(380, 1, P->data,&err) );
    MSG("TTSE Evaluated %f  ", evaluate_ttse_h(P,380, 1) );



    MSG("helmholtz computed %f  ", P->s_fn(380, 1, P->data,&err) );
    MSG("TTSE Evaluated %f  ", evaluate_ttse_s(P,380, 1) );

#ifdef TTSE_DEBUG
    fclose(F1);
#endif

}


void ttse_clean(PureFluid *P){
    remove_tables(P->table);
}

