
typedef struct IntegratorIdaPrecDJStruct{
	N_Vector PIii; /**< diagonal elements of the inversed Jacobi preconditioner */
} IntegratorIdaPrecDataJacobi;

typedef struct IntegratorIdaPrecDJFStruct{
	linsolqr_system_t L;
} IntegratorIdaPrecDataJacobian;

/**
	Hold all the function pointers associated with a particular preconditioner
	We don't need to store the 'pfree' function here as it is allocated to the enginedata struct
	by the pcreate function (ensures that corresponding 'free' and 'create' are always used)

	@note IDA uses a different convention for function pointer types, so no '*'.
*/
typedef struct IntegratorIdaPrecStruct{
	IntegratorIdaPrecCreateFn *pcreate;
	IDASpilsPrecSetupFn psetup;
	IDASpilsPrecSolveFn psolve;
} IntegratorIdaPrec;

/*------
  Full jacobian preconditioner -- experimental
*/

void integrator_ida_pfree_jacobian(IntegratorIdaData *enginedata);

const IntegratorIdaPrec prec_jacobian;

/*------
  Jacobi preconditioner -- experimental
*/

void integrator_ida_pfree_jacobi(IntegratorIdaData *enginedata);

const IntegratorIdaPrec prec_jacobi;

