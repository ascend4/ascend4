/* Exploratory SUNDIALS 6.x DENSE/KLU benchmark, GPL-2.0-or-later.
 * See ../AUTO-SELECTION.md for build command, methodology and limitations.
 * Loading/allocation costs are not included in factorization timings.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sundials/sundials_context.h>
#include <nvector/nvector_serial.h>
#include <sunmatrix/sunmatrix_dense.h>
#include <sunmatrix/sunmatrix_sparse.h>
#include <sunlinsol/sunlinsol_dense.h>
#include <sunlinsol/sunlinsol_klu.h>
static double now(void){struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+1e-9*t.tv_nsec;}
static uint32_t rng=192319;
static double random_value(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (rng>>8)/16777216.;}
static int cmp(const void *x,const void *y){double a=*(const double *)x,b=*(const double *)y;return (a>b)-(a<b);}
static void check(int f){if(f){fprintf(stderr,"solver failure %d\n",f);exit(1);}}
static void load(SUNMatrix A,int sparse,int n,const double *v,const sunindextype *cp,const sunindextype *ri,int nnz){
 if(sparse){memcpy(SM_INDEXPTRS_S(A),cp,(n+1)*sizeof(*cp));memcpy(SM_INDEXVALS_S(A),ri,nnz*sizeof(*ri));memcpy(SM_DATA_S(A),v,nnz*sizeof(*v));}
 else {SUNMatZero(A);for(int j=0;j<n;j++)for(int k=cp[j];k<cp[j+1];k++)SM_ELEMENT_D(A,ri[k],j)=v[k];}
}
static void run(int n,const char *pattern,double density,int sparse,SUNContext ctx){
 double *full=calloc((size_t)n*n,sizeof(double)),*rowsum=calloc(n,sizeof(double));
 if(!full||!rowsum)exit(2);
 rng=192319;
 for(int j=0;j<n;j++)for(int i=0;i<n;i++)if(i!=j){
  int present=0;
  if(!strcmp(pattern,"band"))present=abs(i-j)<=2;
  else if(!strcmp(pattern,"block"))present=i/16==j/16;
  else if(!strcmp(pattern,"dense"))present=1;
  else present=random_value()<density;
  if(present){double a=(random_value()-.5);full[(size_t)j*n+i]=a;rowsum[i]+=fabs(a);}
 }
 for(int i=0;i<n;i++)full[(size_t)i*n+i]=1+rowsum[i];
 sunindextype *cp=calloc(n+1,sizeof(*cp)),*ri=malloc((size_t)n*n*sizeof(*ri));
 double *v=malloc((size_t)n*n*sizeof(*v));int nnz=0;
 if(!cp||!ri||!v)exit(2);
 for(int j=0;j<n;j++){cp[j]=nnz;for(int i=0;i<n;i++)if(full[(size_t)j*n+i]!=0){ri[nnz]=i;v[nnz++]=full[(size_t)j*n+i];}}cp[n]=nnz;
 N_Vector x=N_VNew_Serial(n,ctx),b=N_VNew_Serial(n,ctx);
 if(!x||!b)exit(2);
 for(int i=0;i<n;i++){NV_Ith_S(b,i)=0;for(int j=0;j<n;j++)NV_Ith_S(b,i)+=full[(size_t)j*n+i];}
 SUNMatrix A=sparse?SUNSparseMatrix(n,n,nnz,CSC_MAT,ctx):SUNDenseMatrix(n,n,ctx);
 if(!A||!x||!b)exit(2);
 double first[5];SUNLinearSolver S=NULL;
 for(int r=0;r<5;r++){
  load(A,sparse,n,v,cp,ri,nnz);
  double start=now();S=sparse?SUNLinSol_KLU(x,A,ctx):SUNLinSol_Dense(x,A,ctx);
  if(!S)exit(2);
  check(SUNLinSolInitialize(S));check(SUNLinSolSetup(S,A));first[r]=now()-start;
  if(r<4)SUNLinSolFree(S);
 }
 qsort(first,5,sizeof(double),cmp);
 double setup=0,copy=0,solv=0;int ns=0,nsolve=0;
 while(setup<.025&&ns<10000){
  double start=now();load(A,sparse,n,v,cp,ri,nnz);copy+=now()-start;
  start=now();check(SUNLinSolSetup(S,A));setup+=now()-start;ns++;
 }
 while(solv<.025&&nsolve<100000){double start=now();check(SUNLinSolSolve(S,A,x,b,0));solv+=now()-start;nsolve++;}
 double error=0;for(int i=0;i<n;i++)if(fabs(NV_Ith_S(x,i)-1)>error)error=fabs(NV_Ith_S(x,i)-1);
 if(error>1e-10){fprintf(stderr,"bad solution %.9g\n",error);exit(3);}
 long long factors=0;size_t memory=0;
 if(sparse){sun_klu_numeric *num=SUNLinSol_KLUGetNumeric(S);factors=num->lnz+num->unz+num->nzoff;memory=SUNLinSol_KLUGetCommon(S)->mempeak;}
 printf("%d,%s,%.6g,%d,%s,%.9g,%.9g,%.9g,%.9g,%lld,%zu,%.3g\n",n,pattern,(double)nnz/n/n,nnz,sparse?"KLU":"DENSE",first[2],copy/ns,setup/ns,solv/nsolve,factors,memory,error);fflush(stdout);
 SUNLinSolFree(S);SUNMatDestroy(A);N_VDestroy(x);N_VDestroy(b);free(full);free(rowsum);free(cp);free(ri);free(v);
}
int main(void){
 SUNContext ctx=NULL;check(SUNContext_Create(NULL,&ctx));
 printf("n,pattern,density,nnz,solver,first_s,load_s,setup_s,solve_s,factor_entries,klu_peak_bytes,solution_error\n");
 int sizes[]={8,16,32,64,128,256,512,1024};
 const char *patterns[]={"band","block","random1","random5","random20","dense"};
 double densities[]={0,0,.01,.05,.20,1};
 for(int i=0;i<8;i++)for(int j=0;j<6;j++)for(int s=0;s<2;s++)run(sizes[i],patterns[j],densities[j],s,ctx);
 SUNContext_Free(&ctx);return 0;
}
