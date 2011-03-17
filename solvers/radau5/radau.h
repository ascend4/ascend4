
void radau5_(int *N,
   void FCN(int*,double*,double*,double*,double*,int*),
   double *X, double *Y, double *XEND, double *H,
           double *RTOL, double *ATOL, int *ITOL,
   void JAC(int*, double*, double*, double*, int*, double*, double*),
    int *IJAC, int *MLJAC, int *MUJAC,
   void MAS(int *n,double *am, int *lmas,int *rpar, int *ipar),
    int *IMAS, int *MLMAS, int *MUMAS,
   void SOLOUT(int*,double*,double*,double*,double*,int*,int*,double*,int*,int*),
    int *IOUT,
   double *WORK, int *LWORK,int *IWORK, int *LIWORK,
   double *RPAR, int *IPAR, int *IDID);

/* Interface to the FORTRAN function for contignuous output.(see above) */
double contr5_(int *I, double *S, double *CONT, int *LRC);

/* FORTRAN function.
   Prints the FORTRAN interpretation of the (n,m)-matrix A */
void PRINT_MAT(int *n, int *m, double *A);
