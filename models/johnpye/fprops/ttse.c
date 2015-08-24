
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

#define TAB_FOLDER  "tables"

//#define FO
#define SO
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

    table->satFRho =  ASC_NEW_ARRAY ( double , NSAT);
    table->satFdRhodt =  ASC_NEW_ARRAY (double, NSAT);
    table->satFd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);
    table->satGRho =  ASC_NEW_ARRAY (double, NSAT);
    table->satGdRhodt =  ASC_NEW_ARRAY (double, NSAT);
    table->satGd2RhodT2 =  ASC_NEW_ARRAY (double, NSAT);



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

    ASC_FREE(table->satFRho );
    ASC_FREE(table->satFdRhodt );
    ASC_FREE(table->satFd2RhodT2 );

    ASC_FREE(table->satGRho );
    ASC_FREE(table->satGdRhodt );
    ASC_FREE(table->satGd2RhodT2 );

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

/*
    Actual building of tables is done here.
*/
void build_tables(PureFluid *P){

    #ifndef PT
    #define PT P->data->table

    int i,j;
    FpropsError err = FPROPS_NO_ERROR;

    double Tt = P->data->T_t;
    double Tc = P->data->T_c;
    double dt = (Tc - Tt)/(NSAT);

    double  rho1,rho2;
    P->sat_fn(Tc,&rho1,&rho2,P->data,&err);
    MSG("triple point and critical temperature and critical density-->  %f  %f  %f",Tt,Tc,rho1);

    for(i=0; i<NSAT; ++i)
    {

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

     //   MSG("%f  %f  %f ---  %f  %f  %f",PT->satFRho[i] , PT->satFdRhodt[i], PT->satFd2RhodT2[i],PT->satGRho[i] , PT->satGdRhodt[i], PT->satGd2RhodT2[i]) ;
    }


    double tmin,tmax,rhomin,rhomax;




    tmin = PT->tmin;
    tmax = PT->tmax;
    rhomin = PT->rhomin;
    rhomax = PT->rhomax;

    dt = (tmax-tmin)/NTP;
    double drho = (rhomax-rhomin)/NRHOP;


    MSG("DTemp is %f and DRho is  %f",dt,drho);
    MSG("BUILDING TABLES");

    clock_t start = clock();

    for( i = 0; i < NTP; i++)
    for( j = 0; j < NRHOP; j++){

        double t  = tmin+i*dt;
        double rho  = rhomin+j*drho;

        PT->p[i][j] = P->p_fn( t, rho , P->data, &err);
        PT->dpdt[i][j] = P->dpdT_rho_fn( t, rho , P->data, &err);
        PT->dpdrho[i][j] = P->dpdrho_T_fn( t, rho , P->data, &err);
#ifdef SO
        PT->d2pdt2[i][j] = P->d2pdT2_rho_fn( t, rho , P->data, &err);
        PT->d2pdrho2[i][j] = P->d2pdrho2_T_fn( t, rho , P->data, &err);
        PT->d2pdtdrho[i][j] = P->d2pdTdrho_fn( t, rho , P->data, &err);
#endif

        PT->h[i][j] = P->h_fn( t, rho , P->data, &err);
        PT->dhdt[i][j] = P->dhdT_rho_fn( t, rho , P->data, &err);
        PT->dhdrho[i][j] = P->dhdrho_T_fn( t, rho , P->data, &err);
#ifdef SO
        PT->d2hdt2[i][j] = P->d2hdT2_rho_fn( t, rho , P->data, &err);
        PT->d2hdrho2[i][j] = P->d2hdrho2_T_fn( t, rho , P->data, &err);
        PT->d2hdtdrho[i][j] = P->d2hdTdrho_fn( t, rho , P->data, &err);
#endif

        PT->s[i][j] = P->s_fn( t, rho , P->data, &err);
        PT->dsdt[i][j] = P->dsdT_rho_fn( t, rho , P->data, &err);
        PT->dsdrho[i][j] = P->dsdrho_T_fn( t, rho , P->data, &err);
#ifdef SO
        PT->d2sdt2[i][j] = P->d2sdT2_rho_fn( t, rho , P->data, &err);
        PT->d2sdrho2[i][j] = P->d2sdrho2_T_fn( t, rho , P->data, &err);
        PT->d2sdtdrho[i][j] = P->d2sdTdrho_fn( t, rho , P->data, &err);
#endif
        PT->u[i][j] = P->u_fn( t, rho , P->data, &err);
        PT->dudt[i][j] = P->dudT_rho_fn( t, rho , P->data, &err);
        PT->dudrho[i][j] = P->dudrho_T_fn( t, rho , P->data, &err);
#ifdef SO
        PT->d2udt2[i][j] = P->d2udT2_rho_fn( t, rho , P->data, &err);
        PT->d2udrho2[i][j] = P->d2udrho2_T_fn( t, rho , P->data, &err);
        PT->d2udtdrho[i][j] = P->d2udTdrho_fn( t, rho , P->data, &err);
#endif

        PT->g[i][j] = P->g_fn( t, rho , P->data, &err);
        PT->dgdt[i][j] = P->dgdT_rho_fn( t, rho , P->data, &err);
        PT->dgdrho[i][j] = P->dgdrho_T_fn( t, rho , P->data, &err);
#ifdef SO
        PT->d2gdt2[i][j] = P->d2gdT2_rho_fn( t, rho , P->data, &err);
        PT->d2gdrho2[i][j] = P->d2gdrho2_T_fn( t, rho , P->data, &err);
        PT->d2gdtdrho[i][j] = P->d2gdTdrho_fn( t, rho , P->data, &err);
#endif
    }


    clock_t end = clock();
    double msec = (double)(end - start) / (CLOCKS_PER_SEC/1000);
    MSG("Tables built in %f seconds", msec/1000);

    P->data->IsTableBuilt=1;

    #undef PT
    #endif
}



double evaluate_ttse_sat(double T, double *rhof_out, double * rhog_out, const FluidData *data, FpropsError *err){

    #ifndef PT
    #define PT data->table

    int i,j;
    double tmin = data->T_t;
    double tmax = data->T_c;

    if(T < tmin-1e-8){
    ERRMSG("Input Temperature %f K is below triple-point temperature %f K",T,data->T_t);
    return FPROPS_RANGE_ERROR;
    }

    if(T > tmax+1e-8){
    ERRMSG("Input Temperature is above critical point temperature");
    *err = FPROPS_RANGE_ERROR;
    }

    double dt = (tmax-tmin)/NSAT;
    i = (int)round(((T - tmin)/(tmax - tmin)*(NSAT)));
    //MSG("%d %f %f %f",i,T,tmax,tmin);
    if(i<0)i=0;
    if(i>=NSAT)i=NSAT-1;
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
    double ttseP = PT->p[i][j]
         + delt*PT->dpdt[i][j] + 0.5*delt*delt*PT->d2pdt2[i][j]
         + delrho*PT->dpdrho[i][j] + 0.5*delrho*delrho*PT->d2pdrho2[i][j]
         + delrho*delt*PT->d2pdtdrho[i][j];
    return ttseP; // return P_sat

    #undef PT
    #endif

}

/*
    Second Order Taylor series expansion
*/
#ifdef SO
#define EVALTTSEFN(VAR) \
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
        double ttse##VAR = table->VAR[i][j]\
             + delt*table->d##VAR##dt[i][j] + 0.5*delt*delt*table->d2##VAR##dt2[i][j]\
             + delrho*table->d##VAR##drho[i][j] + 0.5*delrho*delrho*table->d2##VAR##drho2[i][j]\
             + delrho*delt*table->d2##VAR##dtdrho[i][j];\
        return ttse##VAR;\
        }
#endif
/*  snippet for generic calls
        double tmin = P->data->T_t;\
        double tmax = P->data->T_c;\
		if(t >= tmin  && t< tmax) {\
            evaluate_ttse_sat(t, &rho_f, &rho_g, P->data, &err);\
            if(rho_g < rho && rho < rho_f){\
                    double x = rho_g*(rho_f/rho - 1)/(rho_f - rho_g);\
                    double Qf = P->VAR##_fn( t,rho_f,P->data,&err);\
                    double Qg = P->VAR##_fn( t,rho_g,P->data,&err);\
                    return x*Qg + (1-x)*Qf;\
                }\
            }\*/
/*
    First Order Taylor series expansion
*/
#ifdef FO
#define EVALTTSEFNFO(VAR) \
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
//Second order accurate evaluation

#ifdef SO
EVALTTSEFN(p);
EVALTTSEFN(h);
EVALTTSEFN(s);
EVALTTSEFN(g);
EVALTTSEFN(u);
#endif

//First order accurate evaluation
#ifdef FO
EVALTTSEFNFO(p);
EVALTTSEFNFO(h);
EVALTTSEFNFO(s);
EVALTTSEFNFO(g);
EVALTTSEFNFO(u);
#endif




/*
    This will load the binary file from tables/ for the liquid of interest and the EOS and populate the matrices.
    If the files are not present in tables/ then build_tables() should be used.
*/

void load_tables(PureFluid *P){

    int i;
    MSG("Table file exists @ %s",P->data->path);
    FILE * readtablefile = fopen(P->data->path,"rb");


    #define RD(VAR)\
        fread( P->data->table->VAR, sizeof(double), NSAT, readtablefile );
    RD(satFRho);    RD(satFdRhodt);    RD(satFd2RhodT2);
    RD(satGRho);    RD(satGdRhodt);    RD(satGd2RhodT2);
    #undef RD


    #define RD(VAR)\
    for(i=0;i<NTP;i++)\
        fread( P->data->table->VAR[i] ,sizeof(double), NRHOP, readtablefile );
    RD(s);  RD(dsdt); RD(d2sdt2); RD(dsdrho); RD(d2sdrho2); RD(d2sdtdrho);
    RD(p);  RD(dpdt); RD(d2pdt2); RD(dpdrho); RD(d2pdrho2); RD(d2pdtdrho);
    RD(u);  RD(dudt); RD(d2udt2); RD(dudrho); RD(d2udrho2); RD(d2udtdrho);
    RD(g);  RD(dgdt); RD(d2gdt2); RD(dgdrho); RD(d2gdrho2); RD(d2gdtdrho);
    RD(h);  RD(dhdt); RD(d2hdt2); RD(dhdrho); RD(d2hdrho2); RD(d2hdtdrho);
    #undef RD



    fclose(readtablefile);

    P->data->IsTableBuilt=1;

}

/*
    After building the tables once this should be called to save the files in binary inside tables/
*/
void save_tables(PureFluid *P){

    int i;
    MSG("Saving table @ %s",P->data->path);
    FILE * writetablefile = fopen(P->data->path,"wb");


    #define WR(VAR)\
        fwrite( P->data->table->VAR, sizeof(double), NSAT, writetablefile );
    WR(satFRho);     WR(satFdRhodt);    WR(satFd2RhodT2);
    WR(satGRho);     WR(satGdRhodt);    WR(satGd2RhodT2);
    #undef WR


    #define WR(VAR)\
    for(i=0;i<NTP;i++)\
        fwrite( P->data->table->VAR[i] ,sizeof(double), NRHOP, writetablefile );
    WR(s);  WR(dsdt); WR(d2sdt2); WR(dsdrho); WR(d2sdrho2); WR(d2sdtdrho);
    WR(p);  WR(dpdt); WR(d2pdt2); WR(dpdrho); WR(d2pdrho2); WR(d2pdtdrho);
    WR(u);  WR(dudt); WR(d2udt2); WR(dudrho); WR(d2udrho2); WR(d2udtdrho);
    WR(g);  WR(dgdt); WR(d2gdt2); WR(dgdrho); WR(d2gdrho2); WR(d2gdtdrho);
    WR(h);  WR(dhdt); WR(d2hdt2); WR(dhdrho); WR(d2hdrho2); WR(d2hdtdrho);
    #undef WR


    fclose(writetablefile);
}



int doesdbexist(PureFluid *P)
{
    char  path[200]  = TAB_FOLDER;


    strcat(path,"/helm_");
    strcat(path, P->name );
    strcat(path,"_TR.bin");

    P->data->path = FPROPS_NEW_ARRAY( char,strlen(path)+1 );
    strcpy(P->data->path, path);
    P->data->path[strlen(path)]='\0';

  //  MSG("Table file path --> <%s>",P->path);

    FILE *test=fopen(P->data->path,"r");

    if(test)
    {
        MSG("Saved Table Found");
        fclose(test);
        return 1;
    }

    MSG("NO Saved Table");
    return 0;
}

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
//Should be implemented elsewhere per fluid
    P->data->table->tmin = 200;
    P->data->table->tmax = 4200;
    P->data->table->rhomin = 0.0001;
    P->data->table->rhomax = 2000;



    if(doesdbexist(P))//file exists in tables/
        load_tables(P);
    else
    {
        build_tables(P);
        save_tables(P);
    }

//exit(1);
#ifdef TTSE_DEBUG
    //fclose(F1);
#endif


}


void ttse_destroy(PureFluid *P){
    remove_tables(P->data->table);
}

