/*****************************************************************************/
/*  File: conopt.h                                                           */
/*  Version 3.13d                                                            */
/*                                                                           */
/*  Copyright (C) 1995-2003 by ARKI Consulting & Development A/S             */
/*  All Rights Reserved.                                                     */
/*                                                                           */
/*  THIS MATERIAL IS CONSIDERED A TRADE SECRET.                              */
/*  UNAUTHORIZED ACCESS, USE, REPRODUCTION OR DISTRIBUTION IS PROHIBITED.    */
/*                                                                           */
/*  Last Modified December 16, 2003                                          */
/*****************************************************************************/

/*
	Permission has been obtained from Arne Drud, Fri Jul 6, 2007, to include
	this file in the ASCEND distribution. He is aware that ASCEND is an
	open source project license under the GNU General Public License.

	Note that this version of the file comes from CONOPT version 3.14.
*/

#if defined(_WIN32)
#define COI_CALL __stdcall
#define FNAME_UCASE_NODECOR 1
#else
#define COI_CALL
#endif

typedef int (COI_CALL *COI_READMATRIX) ( double* LOWER, double* CURR, double* UPPER, int* VSTA, int* TYPE, double* RHS,
                                         int* ESTA, int* COLSTA, int* ROWNO, double* VALUE,
                                         int* NLFLAG, int* N, int* M, int* NZ, double* USRMEM );
typedef int (COI_CALL *COI_FDEVAL    ) ( double* X, double* G, double* JAC, int* ROWNO, int* JCNM, int* MODE,
                                         int* IGNERR, int* ERRCNT, int* NEWPT, int* N, int* NJ, double* USRMEM );
typedef int (COI_CALL *COI_STATUS    ) ( int* MODSTA, int* SOLSTA, int* ITER, double* OBJVAL, double* USRMEM );
typedef int (COI_CALL *COI_SOLUTION  ) ( double* XVAL, double* XMAR, int* XBAS, int* XSTA,
                                         double* YVAL, double* YMAR, int* YBAS, int* YSTA,
                                         int* N, int* M, double* USRMEM );
typedef int (COI_CALL *COI_MESSAGE   ) ( int* SMSG, int* DMSG, int* NMSG, int* LLEN,
                                         double* USRMEM, char* MSGV, int MSGLEN );
typedef int (COI_CALL *COI_PROGRESS  ) ( int* LEN_INT, int* INT, int* LEN_RL, double* RL, double* X, double* USRMEM );
typedef int (COI_CALL *COI_OPTFILE   ) ( int* FNLEN, double* USRMEM, char* FN, int LENFN );
typedef int (COI_CALL *COI_OPTION    ) ( int* NCALL, double* RVAL, int* IVAL, int* LVAL,
                                         double* USRMEM, char* NAME, int LENNAME );
typedef int (COI_CALL *COI_ERRMSG    ) ( int* ROWNO, int* COLNO, int* POSNO, int* MSGLEN,
                                         double* USRMEM, char* MSG, int LENMSG );
typedef int (COI_CALL *COI_TRIORD    ) ( int* CASE, int* ROWNO, int* COLNO, double* VALUE, double* RESID,
                                         int* INF, double* USRMEM );
typedef int (COI_CALL *COI_FDINTERVAL) ( double* XMIN, double* XMAX, double* GMIN, double* GMAX,
                                         double* JMIN, double* JAMX, int* ROWNO, int* JCNM, int* MODE,
                                         double* PINF, int* N, int* NJ, double* USRMEM );
typedef int (COI_CALL *COI_2DDIR     ) ( double* X, double* DX, double* D2G, int* ROWNO, int* JCNM, int* NEWPT,
                                         int* NODRV, int* N, int* NJ, double* USRMEM );
typedef int (COI_CALL *COI_2DDIRLAG  ) ( double* X, double* DX, double* U, double* D2G, int* NEWPT,
                                         int* NODRV, int* N, int* M, double* USRMEM );
typedef int (COI_CALL *COI_2DLAGR    ) ( double* X, double* U, int* HSRW, int* HSCL, double* HSVL, int* NODRV, int* N,
                                         int* M, int* NHESS, int* MODE, double* USRMEM );
typedef int (COI_CALL *COI_SRFILE    ) ( int* FNLEN, double* USRMEM, char* FN, int LENFN );
typedef int (COI_CALL *COI_DUALBND   ) ( double* LBND, double* UBND, int* COLS, int* NBND, double* USRMEM );


#if   defined(FNAME_LCASE_DECOR) /* fortran names: lower case, trailing _ */
# define COIDEF_Size       coidef_size_
# define COIDEF_Ini        coidef_ini_
# define COIDEF_NumVar     coidef_numvar_
# define COIDEF_NumCon     coidef_numcon_
# define COIDEF_NumNZ      coidef_numnz_
# define COIDEF_NumNlNz    coidef_numnlnz_
# define COIDEF_Base       coidef_base_
# define COIDEF_OptDir     coidef_optdir_
# define COIDEF_ObjCon     coidef_objcon_
# define COIDEF_ObjVar     coidef_objvar_
# define COIDEF_ItLim      coidef_itlim_
# define COIDEF_ErrLim     coidef_errlim_
# define COIDEF_IniStat    coidef_inistat_
# define COIDEF_FVincLin   coidef_fvinclin_
# define COIDEF_FVforAll   coidef_fvforall_
# define COIDEF_DebugFV    coidef_debugfv_
# define COIDEF_MaxSup     coidef_maxsup_
# define COIDEF_Square     coidef_square_
# define COIDEF_EmptyRow   coidef_emptyrow_
# define COIDEF_EmptyCol   coidef_emptycol_
# define COIDEF_Num2D      coidef_num2d_
# define COIDEF_Debug2D    coidef_debug2d_
# define COIDEF_DisCont    coidef_discont_
# define COIDEF_StdOut     coidef_stdout_
# define COIDEF_ClearM     coidef_clearm_
# define COIDEF_2DPerturb  coidef_2dperturb_
# define COIDEF_NDual      coidef_ndual_

# define COIDEF_ResLim     coidef_reslim_
# define COIDEF_WorkSpace  coidef_workspace_
# define COIDEF_WorkFactor coidef_workfactor_

# define COIDEF_ReadMatrix coidef_readmatrix_
# define COIDEF_FDEval     coidef_fdeval_
# define COIDEF_Status     coidef_status_
# define COIDEF_Solution   coidef_solution_
# define COIDEF_Message    coidef_message_
# define COIDEF_ErrMsg     coidef_errmsg_
# define COIDEF_Progress   coidef_progress_
# define COIDEF_Optfile    coidef_optfile_
# define COIDEF_Option     coidef_option_
# define COIDEF_TriOrd     coidef_triord_
# define COIDEF_FDInterval coidef_fdinterval_
# define COIDEF_2DDir      coidef_2ddir_
# define COIDEF_2DDirLag   coidef_2ddirlag_
# define COIDEF_2DLagr     coidef_2dlagr_
# define COIDEF_SRFile     coidef_srfile_
# define COIDEF_DualBnd    coidef_dualbnd_

# define COIDEF_UsrMem     coidef_usrmem_
# define COIDEF_WorkMem    coidef_workmem_

# define COIGET_MaxUsed    coiget_maxused_
# define COIGET_CurUsed    coiget_curused_

# define COI_Solve         coi_solve_
# define COI_MemEst        coi_memest_
# define COI_Version       coi_version_

# define COI_ADDRESSOF     coi_addressof_
# define CALL_READMATRIX   call_readmatrix_
# define CALL_FDEVAL       call_fdeval_
# define CALL_STATUS       call_status_
# define CALL_SOLUTION     call_solution_
# define CALL_MESSAGE      call_message_
# define CALL_PROGRESS     call_progress_
# define CALL_OPTFILE      call_optfile_
# define CALL_OPTION       call_option_
# define CALL_ERRMSG       call_errmsg_
# define CALL_TRIORD       call_triord_
# define CALL_FDINTERVAL   call_fdinterval_
# define CALL_2DDIR        call_2ddir_
# define CALL_2DDIRLAG     call_2ddirlag_
# define CALL_2DLAGR       call_2dlagr_
# define CALL_SRFILE       call_srfile_
# define CALL_DUALBND      call_dualbnd_
# define COEAII            coeaii_
# define COEAI             coeai_

#elif defined(FNAME_LCASE_NODECOR) /* fortran names: lower case, no _ */
# define COIDEF_Size       coidef_size
# define COIDEF_Ini        coidef_ini
# define COIDEF_NumVar     coidef_numvar
# define COIDEF_NumCon     coidef_numcon
# define COIDEF_NumNZ      coidef_numnz
# define COIDEF_NumNlNz    coidef_numnlnz
# define COIDEF_Base       coidef_base
# define COIDEF_OptDir     coidef_optdir
# define COIDEF_ObjCon     coidef_objcon
# define COIDEF_ObjVar     coidef_objvar
# define COIDEF_ItLim      coidef_itlim
# define COIDEF_ErrLim     coidef_errlim
# define COIDEF_IniStat    coidef_inistat
# define COIDEF_FVincLin   coidef_fvinclin
# define COIDEF_FVforAll   coidef_fvforall
# define COIDEF_DebugFV    coidef_debugfv
# define COIDEF_MaxSup     coidef_maxsup
# define COIDEF_Square     coidef_square
# define COIDEF_EmptyRow   coidef_emptyrow
# define COIDEF_EmptyCol   coidef_emptycol
# define COIDEF_Num2D      coidef_num2d
# define COIDEF_Debug2D    coidef_debug2d
# define COIDEF_DisCont    coidef_discont
# define COIDEF_StdOut     coidef_stdout
# define COIDEF_ClearM     coidef_clearm
# define COIDEF_2DPerturb  coidef_2dperturb
# define COIDEF_NDual      coidef_ndual

# define COIDEF_ResLim     coidef_reslim
# define COIDEF_WorkSpace  coidef_workspace
# define COIDEF_WorkFactor coidef_workfactor

# define COIDEF_ReadMatrix coidef_readmatrix
# define COIDEF_FDEval     coidef_fdeval
# define COIDEF_Status     coidef_status
# define COIDEF_Solution   coidef_solution
# define COIDEF_Message    coidef_message
# define COIDEF_ErrMsg     coidef_errmsg
# define COIDEF_Progress   coidef_progress
# define COIDEF_Optfile    coidef_optfile
# define COIDEF_Option     coidef_option
# define COIDEF_TriOrd     coidef_triord
# define COIDEF_FDInterval coidef_fdinterval
# define COIDEF_2DDir      coidef_2ddir
# define COIDEF_2DDirLag   coidef_2ddirlag
# define COIDEF_2DLagr     coidef_2dlagr
# define COIDEF_SRFile     coidef_srfile
# define COIDEF_DualBnd    coidef_dualbnd

# define COIDEF_UsrMem     coidef_usrmem
# define COIDEF_WorkMem    coidef_workmem

# define COIGET_MaxUsed    coiget_maxused
# define COIGET_CurUsed    coiget_curused

# define COI_Solve         coi_solve
# define COI_MemEst        coi_memest
# define COI_Version       coi_version

# define COI_ADDRESSOF     coi_addressof
# define CALL_READMATRIX   call_readmatrix
# define CALL_FDEVAL       call_fdeval
# define CALL_STATUS       call_status
# define CALL_SOLUTION     call_solution
# define CALL_MESSAGE      call_message
# define CALL_PROGRESS     call_progress
# define CALL_OPTFILE      call_optfile
# define CALL_OPTION       call_option
# define CALL_ERRMSG       call_errmsg
# define CALL_TRIORD       call_triord
# define CALL_FDINTERVAL   call_fdinterval
# define CALL_2DDIR        call_2ddir
# define CALL_2DDIRLAG     call_2ddirlag
# define CALL_2DLAGR       call_2dlagr
# define CALL_SRFILE       call_srfile
# define CALL_DUALBND      call_dualbnd
# define COEAII            coeaii
# define COEAI             coeai

#elif defined(FNAME_UCASE_DECOR) /* fortran names: upper case, trailing _ */
# define COIDEF_Size       COIDEF_SIZE_
# define COIDEF_Ini        COIDEF_INI_
# define COIDEF_NumVar     COIDEF_NUMVAR_
# define COIDEF_NumCon     COIDEF_NUMCON_
# define COIDEF_NumNZ      COIDEF_NUMNZ_
# define COIDEF_NumNlNz    COIDEF_NUMNLNZ_
# define COIDEF_Base       COIDEF_BASE_
# define COIDEF_OptDir     COIDEF_OPTDIR_
# define COIDEF_ObjCon     COIDEF_OBJCON_
# define COIDEF_ObjVar     COIDEF_OBJVAR_
# define COIDEF_ItLim      COIDEF_ITLIM_
# define COIDEF_ErrLim     COIDEF_ERRLIM_
# define COIDEF_IniStat    COIDEF_INISTAT_
# define COIDEF_FVincLin   COIDEF_FVINCLIN_
# define COIDEF_FVforAll   COIDEF_FVFORALL_
# define COIDEF_DebugFV    COIDEF_DEBUGFV_
# define COIDEF_MaxSup     COIDEF_MAXSUP_
# define COIDEF_Square     COIDEF_SQUARE_
# define COIDEF_EmptyRow   COIDEF_EMPTYROW_
# define COIDEF_EmptyCol   COIDEF_EMPTYCOL_
# define COIDEF_Num2D      COIDEF_NUM2D_
# define COIDEF_Debug2D    COIDEF_DEBUG2D_
# define COIDEF_DisCont    COIDEF_DISCONT_
# define COIDEF_StdOut     COIDEF_STDOUT_
# define COIDEF_ClearM     COIDEF_CLEARM_
# define COIDEF_2DPerturb  COIDEF_2DPERTURB_
# define COIDEF_NDual      COIDEF_NDUAL_

# define COIDEF_ResLim     COIDEF_RESLIM_
# define COIDEF_WorkSpace  COIDEF_WORKSPACE_
# define COIDEF_WorkFactor COIDEF_WORKFACTOR_

# define COIDEF_ReadMatrix COIDEF_READMATRIX_
# define COIDEF_FDEval     COIDEF_FDEVAL_
# define COIDEF_Status     COIDEF_STATUS_
# define COIDEF_Solution   COIDEF_SOLUTION_
# define COIDEF_Message    COIDEF_MESSAGE_
# define COIDEF_ErrMsg     COIDEF_ERRMSG_
# define COIDEF_Progress   COIDEF_PROGRESS_
# define COIDEF_Optfile    COIDEF_OPTFILE_
# define COIDEF_Option     COIDEF_OPTION_
# define COIDEF_TriOrd     COIDEF_TRIORD_
# define COIDEF_FDInterval COIDEF_FDINTERVAL_
# define COIDEF_2DDir      COIDEF_2DDIR_
# define COIDEF_2DDirLag   COIDEF_2DDIRLAG_
# define COIDEF_2DLagr     COIDEF_2DLAGR_
# define COIDEF_SRFile     COIDEF_SRFILE_
# define COIDEF_DualBnd    COIDEF_DUALBND_

# define COIDEF_UsrMem     COIDEF_USRMEM_
# define COIDEF_WorkMem    COIDEF_WORKMEM_

# define COIGET_MaxUsed    COIGET_MAXUSED_
# define COIGET_CurUsed    COIGET_CURUSED_

# define COI_Solve         COI_SOLVE_
# define COI_MemEst        COI_MEMEST_
# define COI_Version       COI_VERSION_

# define COI_ADDRESSOF     COI_ADDRESSOF_
# define CALL_READMATRIX   CALL_READMATRIX_
# define CALL_FDEVAL       CALL_FDEVAL_
# define CALL_STATUS       CALL_STATUS_
# define CALL_SOLUTION     CALL_SOLUTION_
# define CALL_MESSAGE      CALL_MESSAGE_
# define CALL_PROGRESS     CALL_PROGRESS_
# define CALL_OPTFILE      CALL_OPTFILE_
# define CALL_OPTION       CALL_OPTION_
# define CALL_ERRMSG       CALL_ERRMSG_
# define CALL_TRIORD       CALL_TRIORD_
# define CALL_FDINTERVAL   CALL_FDINTERVAL_
# define CALL_2DDIR        CALL_2DDIR_
# define CALL_2DDIRLAG     CALL_2DDIRLAG_
# define CALL_2DLAGR       CALL_2DLAGR_
# define CALL_SRFILE       CALL_SRFILE_
# define CALL_DUALBND      CALL_DUALBND_
# define COEAII            COEAII_
# define COEAI             COEAI_

#elif defined(FNAME_UCASE_NODECOR) /* fortran names: upper case, no _ */
# define COIDEF_Size       COIDEF_SIZE
# define COIDEF_Ini        COIDEF_INI
# define COIDEF_NumVar     COIDEF_NUMVAR
# define COIDEF_NumCon     COIDEF_NUMCON
# define COIDEF_NumNZ      COIDEF_NUMNZ
# define COIDEF_NumNlNz    COIDEF_NUMNLNZ
# define COIDEF_Base       COIDEF_BASE
# define COIDEF_OptDir     COIDEF_OPTDIR
# define COIDEF_ObjCon     COIDEF_OBJCON
# define COIDEF_ObjVar     COIDEF_OBJVAR
# define COIDEF_ItLim      COIDEF_ITLIM
# define COIDEF_ErrLim     COIDEF_ERRLIM
# define COIDEF_IniStat    COIDEF_INISTAT
# define COIDEF_FVincLin   COIDEF_FVINCLIN
# define COIDEF_FVforAll   COIDEF_FVFORALL
# define COIDEF_DebugFV    COIDEF_DEBUGFV
# define COIDEF_MaxSup     COIDEF_MAXSUP
# define COIDEF_Square     COIDEF_SQUARE
# define COIDEF_EmptyRow   COIDEF_EMPTYROW
# define COIDEF_EmptyCol   COIDEF_EMPTYCOL
# define COIDEF_Num2D      COIDEF_NUM2D
# define COIDEF_Debug2D    COIDEF_DEBUG2D
# define COIDEF_DisCont    COIDEF_DISCONT
# define COIDEF_StdOut     COIDEF_STDOUT
# define COIDEF_ClearM     COIDEF_CLEARM
# define COIDEF_2DPerturb  COIDEF_2DPERTURB
# define COIDEF_NDual      COIDEF_NDUAL

# define COIDEF_ResLim     COIDEF_RESLIM
# define COIDEF_WorkSpace  COIDEF_WORKSPACE
# define COIDEF_WorkFactor COIDEF_WORKFACTOR

# define COIDEF_ReadMatrix COIDEF_READMATRIX
# define COIDEF_FDEval     COIDEF_FDEVAL
# define COIDEF_Status     COIDEF_STATUS
# define COIDEF_Solution   COIDEF_SOLUTION
# define COIDEF_Message    COIDEF_MESSAGE
# define COIDEF_ErrMsg     COIDEF_ERRMSG
# define COIDEF_Progress   COIDEF_PROGRESS
# define COIDEF_Optfile    COIDEF_OPTFILE
# define COIDEF_Option     COIDEF_OPTION
# define COIDEF_TriOrd     COIDEF_TRIORD
# define COIDEF_FDInterval COIDEF_FDINTERVAL
# define COIDEF_2DDir      COIDEF_2DDIR
# define COIDEF_2DDirLag   COIDEF_2DDIRLAG
# define COIDEF_2DLagr     COIDEF_2DLAGR
# define COIDEF_SRFile     COIDEF_SRFILE
# define COIDEF_DualBnd    COIDEF_DUALBND

# define COIDEF_UsrMem     COIDEF_USRMEM
# define COIDEF_WorkMem    COIDEF_WORKMEM

# define COIGET_MaxUsed    COIGET_MAXUSED
# define COIGET_CurUsed    COIGET_CURUSED

# define COI_Solve         COI_SOLVE
# define COI_MemEst        COI_MEMEST
# define COI_Version       COI_VERSION

# define COI_ADDRESSOF     COI_ADDRESSOF
# define CALL_READMATRIX   CALL_READMATRIX
# define CALL_FDEVAL       CALL_FDEVAL
# define CALL_STATUS       CALL_STATUS
# define CALL_SOLUTION     CALL_SOLUTION
# define CALL_MESSAGE      CALL_MESSAGE
# define CALL_PROGRESS     CALL_PROGRESS
# define CALL_OPTFILE      CALL_OPTFILE
# define CALL_OPTION       CALL_OPTION
# define CALL_ERRMSG       CALL_ERRMSG
# define CALL_TRIORD       CALL_TRIORD
# define CALL_FDINTERVAL   CALL_FDINTERVAL
# define CALL_2DDIR        CALL_2DDIR
# define CALL_2DDIRLAG     CALL_2DDIRLAG
# define CALL_2DLAGR       CALL_2DLAGR
# define CALL_SRFILE       CALL_SRFILE
# define CALL_DUALBND      CALL_DUALBND
# define COEAII            COEAII
# define COEAI             COEAI

#else
#error "No compile define for fortran naming convention"
No_compile_define_for_fortran_naming_convention;
#endif

extern void COI_CALL COEAI( int* Lwork, double* Work, int* CntVect );

extern int COI_CALL COIDEF_Size      ( );
extern int COI_CALL COIDEF_Ini       ( int* );
extern int COI_CALL COIDEF_NumVar    ( int*, int* );
extern int COI_CALL COIDEF_NumCon    ( int*, int* );
extern int COI_CALL COIDEF_NumNZ     ( int*, int* );
extern int COI_CALL COIDEF_NumNlNz   ( int*, int* );
extern int COI_CALL COIDEF_Base      ( int*, int* );
extern int COI_CALL COIDEF_OptDir    ( int*, int* );
extern int COI_CALL COIDEF_ObjCon    ( int*, int* );
extern int COI_CALL COIDEF_ObjVar    ( int*, int* );
extern int COI_CALL COIDEF_ItLim     ( int*, int* );
extern int COI_CALL COIDEF_ErrLim    ( int*, int* );
extern int COI_CALL COIDEF_IniStat   ( int*, int* );
extern int COI_CALL COIDEF_FVincLin  ( int*, int* );
extern int COI_CALL COIDEF_FVforAll  ( int*, int* );
extern int COI_CALL COIDEF_DebugFV   ( int*, int* );
extern int COI_CALL COIDEF_MaxSup    ( int*, int* );
extern int COI_CALL COIDEF_Square    ( int*, int* );
extern int COI_CALL COIDEF_EmptyRow  ( int*, int* );
extern int COI_CALL COIDEF_EmptyCol  ( int*, int* );
extern int COI_CALL COIDEF_Num2D     ( int*, int* );
extern int COI_CALL COIDEF_Debug2D   ( int*, int* );
extern int COI_CALL COIDEF_DisCont   ( int*, int* );
extern int COI_CALL COIDEF_StdOut    ( int*, int* );
extern int COI_CALL COIDEF_ClearM    ( int*, int* );
extern int COI_CALL COIDEF_2DPerturb ( int*, int* );
extern int COI_CALL COIDEF_NDual     ( int*, int* );

extern int COI_CALL COIDEF_ResLim    ( int*, double* );
extern int COI_CALL COIDEF_WorkSpace ( int*, double* );
extern int COI_CALL COIDEF_WorkFactor( int*, double* );

extern int COI_CALL COIDEF_ReadMatrix( int*, COI_READMATRIX );
extern int COI_CALL COIDEF_FDEval    ( int*, COI_FDEVAL     );
extern int COI_CALL COIDEF_Status    ( int*, COI_STATUS     );
extern int COI_CALL COIDEF_Solution  ( int*, COI_SOLUTION   );
extern int COI_CALL COIDEF_Message   ( int*, COI_MESSAGE    );
extern int COI_CALL COIDEF_ErrMsg    ( int*, COI_ERRMSG     );
extern int COI_CALL COIDEF_Progress  ( int*, COI_PROGRESS   );
extern int COI_CALL COIDEF_Optfile   ( int*, COI_OPTFILE    );
extern int COI_CALL COIDEF_Option    ( int*, COI_OPTION     );
extern int COI_CALL COIDEF_TriOrd    ( int*, COI_TRIORD     );
extern int COI_CALL COIDEF_FDInterval( int*, COI_FDINTERVAL );
extern int COI_CALL COIDEF_2DDir     ( int*, COI_2DDIR      );
extern int COI_CALL COIDEF_2DDirLag  ( int*, COI_2DDIRLAG   );
extern int COI_CALL COIDEF_2DLagr    ( int*, COI_2DLAGR     );
extern int COI_CALL COIDEF_SRFile    ( int*, COI_SRFILE     );
extern int COI_CALL COIDEF_DualBnd   ( int*, COI_DUALBND    );

extern int COI_CALL COIDEF_UsrMem    ( int*, double* );
extern int COI_CALL COIDEF_WorkMem   ( int*, double*, int* );

extern int COI_CALL COIGET_MaxUsed   ( int* );
extern int COI_CALL COIGET_CurUsed   ( int* );

extern int COI_CALL COI_Solve        ( int* );
extern int COI_CALL COI_MemEst       ( int*, double*, double* );
extern int COI_CALL COI_Version      ( float*, char*, int );
