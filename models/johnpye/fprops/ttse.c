
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "rundata.h"
#include "ideal.h"
#include "ttse.h"

#define NTPoints 200
#define NRhoPoints 200


#define FLUIDS_DEBUG //sid change
#ifdef FLUIDS_DEBUG
# include "color.h"
# define MSG FPROPS_MSG
# define ERRMSG FPROPS_ERRMSG
#else
# define MSG(ARGS...) ((void)0)
# define ERRMSG(ARGS...) ((void)0)
#endif



inline double ** alloc_matrix(int TP, int RhoP) {

    double ** matrix = (double**) malloc(sizeof(double*) * TP);

    int i,j;

    for( i = 0; i < TP; i++) {
        matrix[i] = (double*) malloc(sizeof(double) * RhoP);
    }

    for( i = 0; i < TP; i++)
    for( j = 0; j < RhoP; j++)
        matrix[i][j] = 0;


    return matrix;
}

inline void remove_matrix(double **Mat , int TP){
    int i;

    for(i = 0; i < TP; i++)
        free(Mat[i]);
}


void alloc_tables(ttse * table)
{
    int i,j;

    table->dsdT = alloc_matrix(NTPoints,NRhoPoints);
    table->d2sdT2 = alloc_matrix(NTPoints,NRhoPoints);
    table->dsdRho = alloc_matrix(NTPoints,NRhoPoints);
    table->d2sdRho2 = alloc_matrix(NTPoints,NRhoPoints);
    table->d2sdTdRho = alloc_matrix(NTPoints,NRhoPoints);


    table->dPdT = alloc_matrix(NTPoints,NRhoPoints);
    table->d2PdT2 = alloc_matrix(NTPoints,NRhoPoints);
    table->dPdRho = alloc_matrix(NTPoints,NRhoPoints);
    table->d2PdRho2 = alloc_matrix(NTPoints,NRhoPoints);
    table->d2PdTdRho = alloc_matrix(NTPoints,NRhoPoints);


    table->dudT = alloc_matrix(NTPoints,NRhoPoints);
    table->d2udT2 = alloc_matrix(NTPoints,NRhoPoints);
    table->dudRho = alloc_matrix(NTPoints,NRhoPoints);
    table->d2udRho2 = alloc_matrix(NTPoints,NRhoPoints);
    table->d2udTdRho = alloc_matrix(NTPoints,NRhoPoints);


    table->dgdT = alloc_matrix(NTPoints,NRhoPoints);
    table->d2gdT2 = alloc_matrix(NTPoints,NRhoPoints);
    table->dgdRho = alloc_matrix(NTPoints,NRhoPoints);
    table->d2gdRho2 = alloc_matrix(NTPoints,NRhoPoints);
    table->d2gdTdRho = alloc_matrix(NTPoints,NRhoPoints);


    table->dhdT = alloc_matrix(NTPoints,NRhoPoints);
    table->d2hdT2 = alloc_matrix(NTPoints,NRhoPoints);
    table->dhdRho = alloc_matrix(NTPoints,NRhoPoints);
    table->d2hdRho2 = alloc_matrix(NTPoints,NRhoPoints);
    table->d2hdTdRho = alloc_matrix(NTPoints,NRhoPoints);
}


void remove_tables(ttse *table)
{
    remove_matrix(table->dsdT,NTPoints);
    remove_matrix(table->d2sdT2,NTPoints);
    remove_matrix(table->dsdRho,NTPoints);
    remove_matrix(table->d2sdRho2,NTPoints);
    remove_matrix(table->d2sdTdRho,NTPoints);

    remove_matrix(table->dPdT,NTPoints);
    remove_matrix(table->d2PdT2,NTPoints);
    remove_matrix(table->dPdRho,NTPoints);
    remove_matrix(table->d2PdRho2,NTPoints);
    remove_matrix(table->d2PdTdRho,NTPoints);


    remove_matrix(table->dudT,NTPoints);
    remove_matrix(table->d2udT2,NTPoints);
    remove_matrix(table->dudRho,NTPoints);
    remove_matrix(table->d2udRho2,NTPoints);
    remove_matrix(table->d2udTdRho,NTPoints);

    remove_matrix(table->dgdT,NTPoints);
    remove_matrix(table->d2gdT2,NTPoints);
    remove_matrix(table->dgdRho,NTPoints);
    remove_matrix(table->d2gdRho2,NTPoints);
    remove_matrix(table->d2gdTdRho,NTPoints);

    remove_matrix(table->dhdT,NTPoints);
    remove_matrix(table->d2hdT2,NTPoints);
    remove_matrix(table->dhdRho,NTPoints);
    remove_matrix(table->d2hdRho2,NTPoints);
    remove_matrix(table->d2hdTdRho,NTPoints);

}

void ttse_prepare(PureFluid *P){


    MSG("Inside TTSE \n");

    if(!P->Table->UseTTSE)
        return;


    alloc_tables(P->Table);



}


void ttse_clean(PureFluid *P){

    remove_tables(P->Table);


}
