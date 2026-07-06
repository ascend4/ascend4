/* CONOPT C API
 *
 * Copyright (C) 1995-2025 GAMS Software GmbH
 * Copyright (C) 1995-2025 GAMS Development Corporation
 */

/** @file conopt.h
 *  @ingroup PUBLICAPI_FILES
 *  @brief public C API header file
 */

#ifndef CONOPT_H_
#define CONOPT_H_

#define CONOPT_VERSION_MAJOR 4
#define CONOPT_VERSION_MINOR 39
#define CONOPT_VERSION_PATCH 0

/** Windows calling convention */
#if defined(_WIN32)
#define COI_CALLCONV __stdcall
#else
#define COI_CALLCONV
#endif

/** Attribute for visibility (or export) of API functions */
#if defined(COI_API)
#elif defined(_WIN32)
#define COI_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#define COI_API __attribute__((__visibility__("default")))
#else
#define COI_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /**@cond UNDOCUMENTED_CAPI  */
    typedef struct coiRec* coiHandle_t;

    typedef int(COI_CALLCONV *COI_READMATRIX_t)(double LOWER[], double CURR[], double UPPER[], int VSTA[], int TYPEX[], double RHS[], int ESTA[], int COLSTA[], int ROWNO[], double VALUE[], int NLFLAG[], int NUMVAR, int NUMCON, int NUMNZ, void* USRMEM);
    typedef int(COI_CALLCONV *COI_FDEVAL_t)(const double X[], double* G, double JAC[], int ROWNO, const int JACNUM[], int MODE, int IGNERR, int* ERRCNT, int NUMVAR, int NUMJAC, int THREAD, void* USRMEM);
    typedef int(COI_CALLCONV *COI_FDEVALINI_t)(const double X[], const int ROWLIST[], int MODE, int LISTSIZE, int NUMTHREAD, int IGNERR, int* ERRCNT, int NUMVAR, void* USRMEM);
    typedef int(COI_CALLCONV *COI_FDEVALEND_t)(int IGNERR, int* ERRCNT, void* USRMEM);
    typedef int(COI_CALLCONV *COI_STATUS_t)(int MODSTA, int SOLSTA, int ITER, double OBJVAL, void* USRMEM);
    typedef int(COI_CALLCONV *COI_SOLUTION_t)(const double XVAL[], const double XMAR[], const int XBAS[], const int XSTA[], const double YVAL[], const double YMAR[], const int YBAS[], const int YSTA[], int NUMVAR, int NUMCON, void* USRMEM);
    typedef int(COI_CALLCONV *COI_MESSAGE_t)(int SMSG, int DMSG, int NMSG, char* MSGV[], void* USRMEM);
    typedef int(COI_CALLCONV *COI_ERRMSG_t)(int ROWNO, int COLNO, int POSNO, const char* MSG, void* USRMEM);
    typedef int(COI_CALLCONV *COI_PROGRESS_t)(int LEN_INT, const int INTX[], int LEN_RL, const double RL[], const double X[], void* USRMEM);
    typedef int(COI_CALLCONV *COI_OPTION_t)(int NCALL, double* RVAL, int* IVAL, int* LVAL, char* NAME, void* USRMEM);
    typedef int(COI_CALLCONV *COI_TRIORD_t)(int MODE, int TYPEX, int STATUS, int ROWNO, int COLNO, int INF, double VALUE, double RESID, void* USRMEM);
    typedef int(COI_CALLCONV *COI_FDINTERVAL_t)(const double XMIN[], const double XMAX[], double* GMIN, double* GMAX, double JMIN[], double JMAX[], int ROWNO, const int JACNUM[], int MODE, double PINF, int NUMVAR, int NUMJAC, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DDIR_t)(const double X[], const double DX[], double D2G[], int ROWNO, const int JACNUM[], int* NODRV, int NUMVAR, int NUMJAC, int THREAD, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DDIRINI_t)(const double X[], const double DX[], const int ROWLIST[], int LISTSIZE, int NUMTHREAD, int NEWPT, int* NODRV, int NUMVAR, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DDIREND_t)(int* NODRV, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DDIRLAGR_t)(const double X[], const double DX[], const double U[], double D2G[], int NEWPT, int* NODRV, int NUMVAR, int NUMCON, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DLAGRSIZE_t)(int* NODRV, int NUMVAR, int NUMCON, int* NHESS, int MAXHESS, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DLAGRSTR_t)(int HSRW[], int HSCL[], int* NODRV, int NUMVAR, int NUMCON, int NHESS, void* USRMEM);
    typedef int(COI_CALLCONV *COI_2DLAGRVAL_t)(const double X[], const double U[], const int HSRW[], const int HSCL[], double HSVL[], int* NODRV, int NUMVAR, int NUMCON, int NHESS, void* USRMEM);
    /**@endcond */

    /** @copydoc conopt::coi_solve()
     *
     *  @ingroup UTILITY_ROUTINES_C
     */
    COI_API int COI_CALLCONV COI_Solve(coiHandle_t cntvect);

    /** @copydoc conopt::coiget_version()
     *
     *  @ingroup UTILITY_ROUTINES_C
     */
    COI_API void COI_CALLCONV COIGET_Version(int* major, int* minor, int* patch);

    /** @copydoc conopt::coiget_maxthreads()
     *
     *  @ingroup UTILITY_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIGET_MaxThreads(coiHandle_t cntvect);

    /** @copydoc conopt::coiget_maxheapused()
     *
     *  @ingroup UTILITY_ROUTINES_C
     */
    COI_API double COI_CALLCONV COIGET_MaxHeapUsed(coiHandle_t cntvect);

    /** @copydoc conopt::coiget_rangeerrors()
     *
     *  @ingroup UTILITY_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIGET_RangeErrors(coiHandle_t cntvect);

    /** @copydoc conopt::coi_create()
     *
     *  @ingroup THE_CONTROL_VECTOR_C
     */
    COI_API int COI_CALLCONV COI_Create(coiHandle_t *cntvect);

    /** @copydoc conopt::coi_free()
     *
     *  @ingroup THE_CONTROL_VECTOR_C
     */
    COI_API int COI_CALLCONV COI_Free(coiHandle_t *cntvect);

    /** @brief finializes the solving process for CONOPT. This must be called when using OpenMP. It will terminate the running
     * processes cleanly.
     *
     *  @ingroup THE_CONTROL_VECTOR_C
     */
    COI_API void COI_CALLCONV COI_Finalize(void);

    /** @copydoc conopt::coidef_numvar()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_NumVar(coiHandle_t cntvect, int numvar);

    /** @copydoc conopt::coidef_numcon()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_NumCon(coiHandle_t cntvect, int numcon);

    /** @copydoc conopt::coidef_numnz()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_NumNz(coiHandle_t cntvect, int numnz);

    /** @copydoc conopt::coidef_numnlnz()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_NumNlNz(coiHandle_t cntvect, int numnlnz);

    /** @copydoc conopt::coidef_numhess()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_NumHess(coiHandle_t cntvect, int numhess);

    /** @copydoc conopt::coidef_optdir()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_OptDir(coiHandle_t cntvect, int optdir);

    /** @copydoc conopt::coidef_objvar()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_ObjVar(coiHandle_t cntvect, int objvar);

    /** @copydoc conopt::coidef_objcon()
     *
     *  @ingroup REGISTRATION_OF_SIZES_C
     */
    COI_API int COI_CALLCONV COIDEF_ObjCon(coiHandle_t cntvect, int objcon);

    /** @copydoc conopt::coidef_license()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_License(coiHandle_t cntvect, int licint1, int licint2, int licint3, const char *licstring);

    /** @copydoc conopt::coidef_itlim()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ItLim(coiHandle_t cntvect, int itlim);

    /** @copydoc conopt::coidef_errlim()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ErrLim(coiHandle_t cntvect, int errlim);

    /** @copydoc conopt::coidef_reslim()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ResLim(coiHandle_t cntvect, double reslim);

    /** @copydoc conopt::coidef_maxheap()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_MaxHeap(coiHandle_t cntvect, double maxheap);

    /** @copydoc conopt::coidef_inistat()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_IniStat(coiHandle_t cntvect, int inistat);

    /** @copydoc conopt::coidef_fvinclin()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_FVincLin(coiHandle_t cntvect, int fvinclin);

    /** @copydoc conopt::coidef_fvforall()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_FVforAll(coiHandle_t cntvect, int fvforall);

    /** @copydoc conopt::coidef_maxsup()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_MaxSup(coiHandle_t cntvect, int maxsup);

    /** @copydoc conopt::coidef_square()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_Square(coiHandle_t cntvect, int square);

    /** @copydoc conopt::coidef_emptyrow()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_EmptyRow(coiHandle_t cntvect, int emptyrow);

    /** @copydoc conopt::coidef_emptycol()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_EmptyCol(coiHandle_t cntvect, int emptycol);

    /** @copydoc conopt::coidef_discont()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_DisCont(coiHandle_t cntvect, int discont);

    /** @copydoc conopt::coidef_hessfac()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_HessFac(coiHandle_t cntvect, double hessfac);

    /** @copydoc conopt::coidef_debugfv()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_DebugFV(coiHandle_t cntvect, int debugfv);

    /** @copydoc conopt::coidef_debug2d()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_Debug2D(coiHandle_t cntvect, int debug2d);

    /** @copydoc conopt::coidef_optorder()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_OptOrder(coiHandle_t cntvect, int optorder);

    /** @copydoc conopt::coidef_clearm()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ClearM(coiHandle_t cntvect, int clearm);

    /** @copydoc conopt::coidef_zeronoise()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ZeroNoise(coiHandle_t cntvect, int zeronoise);

    /** @copydoc conopt::coidef_threads()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ThreadS(coiHandle_t cntvect, int threads);

    /** @copydoc conopt::coidef_threadf()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ThreadF(coiHandle_t cntvect, int threadf);

    /** @copydoc conopt::coidef_thread2d()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_Thread2D(coiHandle_t cntvect, int thread2d);

    /** @copydoc conopt::coidef_threadc()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_ThreadC(coiHandle_t cntvect, int threadc);

    /** @copydoc conopt::coidef_stdout()
     *
     *  @ingroup REGISTRATION_OF_OPTIONS_C
     */
    COI_API int COI_CALLCONV COIDEF_StdOut(coiHandle_t cntvect, int tostdout);

    /** @copydoc conopt::coidef_optfile()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Optfile(coiHandle_t cntvect, const char *optfile);

    /** @copydoc conopt::coidef_readmatrix()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_ReadMatrix(coiHandle_t cntvect, COI_READMATRIX_t coi_readmatrix);

    /** @copydoc conopt::coidef_fdeval()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_FDEval(coiHandle_t cntvect, COI_FDEVAL_t coi_fdeval);

    /** @copydoc conopt::coidef_fdevalini()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_FDEvalIni(coiHandle_t cntvect, COI_FDEVALINI_t coi_fdevalini);

    /** @copydoc conopt::coidef_fdevalend()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_FDEvalEnd(coiHandle_t cntvect, COI_FDEVALEND_t coi_fdevalend);

    /** @copydoc conopt::coidef_status()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Status(coiHandle_t cntvect, COI_STATUS_t coi_status);

    /** @copydoc conopt::coidef_solution()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Solution(coiHandle_t cntvect, COI_SOLUTION_t coi_solution);

    /** @copydoc conopt::coidef_message()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Message(coiHandle_t cntvect, COI_MESSAGE_t coi_message);

    /** @copydoc conopt::coidef_errmsg()
     *
     *  @ingroup REGISTRATION_OF_MANDATORY_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_ErrMsg(coiHandle_t cntvect, COI_ERRMSG_t coi_errmsg);

    /** @copydoc conopt::coidef_progress()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Progress(coiHandle_t cntvect, COI_PROGRESS_t coi_progress);

    /** @copydoc conopt::coidef_option()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_Option(coiHandle_t cntvect, COI_OPTION_t coi_option);

    /** @copydoc conopt::coidef_triord()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_TriOrd(coiHandle_t cntvect, COI_TRIORD_t coi_triord);

    /** @copydoc conopt::coidef_fdinterval()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_FDInterval(coiHandle_t cntvect, COI_FDINTERVAL_t coi_fdinterval);

    /** @copydoc conopt::coidef_2ddir()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DDir(coiHandle_t cntvect, COI_2DDIR_t coi_2ddir);

    /** @copydoc conopt::coidef_2ddirini()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DDirIni(coiHandle_t cntvect, COI_2DDIRINI_t coi_2ddirini);

    /** @copydoc conopt::coidef_2ddirend()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DDirEnd(coiHandle_t cntvect, COI_2DDIREND_t coi_2ddirend);

    /** @copydoc conopt::coidef_2ddirlagr()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DDirLagr(coiHandle_t cntvect, COI_2DDIRLAGR_t coi_2ddirlagr);

    COI_API int COI_CALLCONV COIDEF_2DLagrSize(coiHandle_t cntvect, COI_2DLAGRSIZE_t coi_2dlagrsize);

    /** @copydoc conopt::coidef_2dlagrstr()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DLagrStr(coiHandle_t cntvect, COI_2DLAGRSTR_t coi_2dlagrstr);

    /** @copydoc conopt::coidef_2dlagrval()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_2DLagrVal(coiHandle_t cntvect, COI_2DLAGRVAL_t coi_2dlagrval);

    /** @copydoc conopt::coidef_usrmem()
     *
     *  @ingroup REGISTRATION_OF_OPTIONAL_CALLBACK_ROUTINES_C
     */
    COI_API int COI_CALLCONV COIDEF_UsrMem(coiHandle_t cntvect, void* usrmem);

#ifdef __cplusplus
}
#endif

#endif /* CONOPT_H_ */
