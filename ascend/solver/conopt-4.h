#ifndef ASC_CONOPT_4_H
#define ASC_CONOPT_4_H

/*
	Minimal CONOPT 4 C API declarations used by ASCEND's runtime-loaded
	CONOPT interface. This header intentionally contains only the ABI surface
	that ASCEND calls or registers, so ASCEND can be built without an installed
	CONOPT development package. A CONOPT shared library is still required at
	runtime to use the solver.
*/

#define CONOPT_VERSION_MAJOR 4
#define CONOPT_VERSION_MINOR 0
#define CONOPT_VERSION_PATCH 0

#if defined(_WIN32)
# define COI_CALLCONV __stdcall
#else
# define COI_CALLCONV
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct coiRec *coiHandle_t;

typedef int (COI_CALLCONV *COI_READMATRIX_t)(
	double lower[], double curr[], double upper[], int vsta[], int type[],
	double rhs[], int esta[], int colsta[], int rowno[], double value[],
	int nlflag[], int n, int m, int nz, void *usrmem
);

typedef int (COI_CALLCONV *COI_FDEVAL_t)(
	const double x[], double *g, double jac[], int rowno,
	const int jcnm[], int mode, int ignerr, int *errcnt, int n, int nj,
	int thread, void *usrmem
);

typedef int (COI_CALLCONV *COI_STATUS_t)(
	int modsta, int solsta, int iter, double objval, void *usrmem
);

typedef int (COI_CALLCONV *COI_SOLUTION_t)(
	const double xval[], const double xmar[], const int xbas[],
	const int xsta[], const double yval[], const double ymar[],
	const int ybas[], const int ysta[], int n, int m, void *usrmem
);

typedef int (COI_CALLCONV *COI_MESSAGE_t)(
	int smsg, int dmsg, int nmsg, char *msgv[], void *usrmem
);

typedef int (COI_CALLCONV *COI_ERRMSG_t)(
	int rowno, int colno, int posno, const char *msg, void *usrmem
);

typedef int (COI_CALLCONV *COI_PROGRESS_t)(
	int len_int, const int intv[], int len_rl, const double rlv[],
	const double x[], void *usrmem
);

typedef int (COI_CALLCONV *COI_OPTION_t)(
	int ncall, double *rval, int *ival, int *lval, char *name, void *usrmem
);

int COI_CALLCONV COI_Create(coiHandle_t *cntvect);
int COI_CALLCONV COI_Free(coiHandle_t *cntvect);
int COI_CALLCONV COI_Solve(coiHandle_t cntvect);

int COI_CALLCONV COIDEF_NumVar(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_NumCon(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_NumNz(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_NumNlNz(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_OptDir(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_ObjCon(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_ItLim(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_ErrLim(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_StdOut(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_DebugFV(coiHandle_t cntvect, int v);
int COI_CALLCONV COIDEF_ReadMatrix(coiHandle_t cntvect, COI_READMATRIX_t f);
int COI_CALLCONV COIDEF_FDEval(coiHandle_t cntvect, COI_FDEVAL_t f);
int COI_CALLCONV COIDEF_Status(coiHandle_t cntvect, COI_STATUS_t f);
int COI_CALLCONV COIDEF_Solution(coiHandle_t cntvect, COI_SOLUTION_t f);
int COI_CALLCONV COIDEF_Message(coiHandle_t cntvect, COI_MESSAGE_t f);
int COI_CALLCONV COIDEF_ErrMsg(coiHandle_t cntvect, COI_ERRMSG_t f);
int COI_CALLCONV COIDEF_Progress(coiHandle_t cntvect, COI_PROGRESS_t f);
int COI_CALLCONV COIDEF_Option(coiHandle_t cntvect, COI_OPTION_t f);
int COI_CALLCONV COIDEF_UsrMem(coiHandle_t cntvect, void *v);

#ifdef __cplusplus
}
#endif

#endif /* ASC_CONOPT_4_H */
