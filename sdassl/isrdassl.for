      DOUBLE PRECISION FUNCTION D1MACH (IDUM)
      INTEGER IDUM
C
C***********************************************************************
C
C               SDASSL routines
C               by Linda Petzold, Andreas Kroener, Wolfgang Marquardt
C               Created: 15/03/83
C               Version: 1.1   Rev: 1989/12/11
C               Date last modified: 1994/09/02
C
C This file is part of the SDASSL differential/algebraic system solver.
C
C Copyright (C) 1983, 1989, 1994 Linda Petzold, Andreas Kroener, 
C                                Wolfgang Marquardt
C
C The SDASSL differential/algebraic system solver is free software;
C you can redistribute it and/or modify it under the terms of the GNU
C General Public License as published by the Free Software Foundation;
C either version 2 of the License, or (at your option) any later version.
C
C The SDASSL system solver is distributed in hope that it will be
C useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
C MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
C General Public License for more details.
C
C You should have received a copy of the GNU General Public License along
C with the program; if not, write to the Free Software Foundation, Inc.,
C 675 Mass Ave, Cambridge, MA 02139 USA.  Check the file named COPYING.
C
C***********************************************************************
C
C-----------------------------------------------------------------------
C THIS ROUTINE COMPUTES THE UNIT ROUNDOFF OF THE MACHINE IN DOUBLE
C PRECISION.  THIS IS DEFINED AS THE SMALLEST POSITIVE MACHINE NUMBER
C U SUCH THAT  1.0D0 + U .NE. 1.0D0 (IN DOUBLE PRECISION).
C-----------------------------------------------------------------------
      DOUBLE PRECISION U, COMP
      U = 1.0D0
 10   U = U*0.5D0
      COMP = 1.0D0 + U
      IF (COMP .NE. 1.0D0) GO TO 10
      D1MACH = U*2.0D0
      RETURN
C----------------------- END OF FUNCTION D1MACH ------------------------
      END
      SUBROUTINE DAXPY(N,DA,DX,INCX,DY,INCY)
C
C     CONSTANT TIMES A VECTOR PLUS A VECTOR.
C     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE.
C     JACK DONGARRA, LINPACK, 3/11/78.
C
      DOUBLE PRECISION DX(1),DY(1),DA
      INTEGER I,INCX,INCY,IX,IY,M,MP1,N
C
      IF(N.LE.0)RETURN
      IF (DA .EQ. 0.0D0) RETURN
      IF(INCX.EQ.1.AND.INCY.EQ.1)GO TO 20
C
C        CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS
C          NOT EQUAL TO 1
C
      IX = 1
      IY = 1
      IF(INCX.LT.0)IX = (-N+1)*INCX + 1
      IF(INCY.LT.0)IY = (-N+1)*INCY + 1
      DO 10 I = 1,N
        DY(IY) = DY(IY) + DA*DX(IX)
        IX = IX + INCX
        IY = IY + INCY
   10 CONTINUE
      RETURN
C
C        CODE FOR BOTH INCREMENTS EQUAL TO 1
C
C
C        CLEAN-UP LOOP
C
   20 M = MOD(N,4)
      IF( M .EQ. 0 ) GO TO 40
      DO 30 I = 1,M
        DY(I) = DY(I) + DA*DX(I)
   30 CONTINUE
      IF( N .LT. 4 ) RETURN
   40 MP1 = M + 1
      DO 50 I = MP1,N,4
        DY(I) = DY(I) + DA*DX(I)
        DY(I + 1) = DY(I + 1) + DA*DX(I + 1)
        DY(I + 2) = DY(I + 2) + DA*DX(I + 2)
        DY(I + 3) = DY(I + 3) + DA*DX(I + 3)
   50 CONTINUE
      RETURN
      END
      SUBROUTINE DDAINI(X,Y,YPRIME,NEQ,
     *   RES,JAC,H,WT,IDID,RPAR,IPAR,
     *   PHI,DELTA,E,WM,IWM,
     *   HMIN,UROUND,NONNEG,PTN)
C
C***BEGIN PROLOGUE  DDAINI
C***REFER TO  DDASSL
C***ROUTINES CALLED  DDANRM,DDAJAC,DDASLV
C***COMMON BLOCKS    DDA001
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE DDAINI
C
C-------------------------------------------------------
C     ddaini takes one step of size h or smaller
C     with the backward euler method, to
C     find yprime at the initial time x. a modified
C     damped newton iteration is used to
C     solve the corrector iteration.
C
C     the initial guess yprime is used in the
C     prediction, and in forming the iteration
C     matrix, but is not involved in the
C     error test. this may have trouble
C     converging if the initial guess is no
C     good, or if g(xy,yprime) depends
C     nonlinearly on yprime.
C
C     the parameters represent:
C     x --         independent variable
C     y --         solution vector at x
C     yprime --    derivative of solution vector
C     neq --       number of equations
C     h --         stepsize. imder may use a stepsize
C                  smaller than h.
C     wt --        vector of weights for error
C                  criterion
C     idid --      completion code with the following meanings
C                  idid= 1 -- yprime was found successfully
C                  idid=-12 -- ddaini failed to find yprime
C     rpar,ipar -- real and integer parameter arrays
C                  that are not altered by ddaini
C     phi --       work space for ddaini
C     delta,e --   work space for ddaini
C     wm,iwm --    real and integer arrays storing
C                  matrix information
C
C-----------------------------------------------------------------
C
C C_1 R. KOE EINBAU VON PTN (EXT, PARAMETERKLAMMERN DDAINI,DDAJAC)
C
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      LOGICAL CONVGD
      DIMENSION Y(1),YPRIME(1),WT(1)
      DIMENSION PHI(NEQ,1),DELTA(1),E(1)
      DIMENSION WM(1),IWM(1)
      DIMENSION RPAR(1),IPAR(1)
      EXTERNAL RES,JAC,PTN
      COMMON/DDA001/NPD,NTEMP,
     *  LML,LMU,LMXORD,LMTYPE,
     *  LNST,LNRE,LNJE,LETF,LCTF,LIPVT
 
      DATA MAXIT/10/,MJAC/5/
      DATA DAMP/0.75D0/
 
C
C
C---------------------------------------------------
C     block 1.
C     initializations.
C---------------------------------------------------
C
      IDID=1
      NEF=0
      NCF=0
      NSF=0
      YNORM=DDANRM(NEQ,Y,WT,RPAR,IPAR)
C
C     save y and yprime in phi
      DO 100 I=1,NEQ
         PHI(I,1)=Y(I)
100      PHI(I,2)=YPRIME(I)
 
C
C
C----------------------------------------------------
C     block 2.
C     do one backward euler step.
C----------------------------------------------------
C
C     set up for start of corrector iteration
200   CJ=1.0D0/H
      XNEW=X+H
C
C     predict solution and derivative
 
      DO 250 I=1,NEQ
250     Y(I)=Y(I)+H*YPRIME(I)
C
      JCALC=-1
      M=0
      CONVGD=.TRUE.
C
C
C     corrector loop.
300   IWM(LNRE)=IWM(LNRE)+1
      IRES=0
 
      CALL RES(XNEW,Y,YPRIME,DELTA,IRES,RPAR,IPAR)
      IF (IRES.LT.0) GO TO 430
C
C
C     evaluate the iteration matrix
      IF (JCALC.NE.-1) GO TO 310
      IWM(LNJE)=IWM(LNJE)+1
      JCALC=0
      CALL DDAJAC(NEQ,XNEW,Y,YPRIME,DELTA,CJ,H,
     *   IER,WT,E,WM,IWM,RES,IRES,
     *   UROUND,JAC,RPAR,IPAR,PTN)
 
      S=1000000.D0
      IF (IRES.LT.0) GO TO 430
      IF (IER.NE.0) GO TO 430
      NSF=0
 
C
C
C
C     multiply residual by damping factor
310   CONTINUE
      DO 320 I=1,NEQ
320      DELTA(I)=DELTA(I)*DAMP
 
C
C     compute a new iterate (back substitution)
C     store the correction in delta
 
      CALL DDASLV(NEQ,DELTA,WM,IWM)
 
C
C     update y and yprime
 
      DO 330 I=1,NEQ
         Y(I)=Y(I)-DELTA(I)
330      YPRIME(I)=YPRIME(I)-CJ*DELTA(I)
 
C
C     test for convergence of the iteration.
 
      DELNRM=DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      IF (DELNRM.LE.100.D0*UROUND*YNORM)
     *   GO TO 400
 
      IF (M.GT.0) GO TO 340
         OLDNRM=DELNRM
         GO TO 350
 
340   RATE=(DELNRM/OLDNRM)**(1.0D0/DFLOAT(M))
      IF (RATE.GT.0.90D0) GO TO 430
      S=RATE/(1.0D0-RATE)
 
350   IF (S*DELNRM .LE. 0.33D0) GO TO 400
C
C
C     the corrector has not yet converged. update
C     m and and test whether the maximum
C     number of iterations have been tried.
C     every mjac iterations, get a new
C     iteration matrix.
 
      M=M+1
      IF (M.GE.MAXIT) GO TO 430
 
      IF ((M/MJAC)*MJAC.EQ.M) JCALC=-1
 
      GO TO 300
 
C
C
C     the iteration has converged.
C     check nonnegativity constraints
400   IF (NONNEG.EQ.0) GO TO 450
      DO 410 I=1,NEQ
410      DELTA(I)=DMIN1(Y(I),0.0D0)
 
      DELNRM=DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      IF (DELNRM.GT.0.33D0) GO TO 430
 
      DO 420 I=1,NEQ
         Y(I)=Y(I)-DELTA(I)
420      YPRIME(I)=YPRIME(I)-CJ*DELTA(I)
      GO TO 450
C
C
C     exits from corrector loop.
430   CONVGD=.FALSE.
450   IF (.NOT.CONVGD) GO TO 600
C
C
C
C-----------------------------------------------------
C     block 3.
C     the corrector iteration converged.
C     do error test.
C-----------------------------------------------------
C
 
      DO 510 I=1,NEQ
510      E(I)=Y(I)-PHI(I,1)
 
      ERR=DDANRM(NEQ,E,WT,RPAR,IPAR)
 
      IF (ERR.LE.1.0D0) RETURN
 
C
C
C
C--------------------------------------------------------
C     block 4.
C     the backward euler step failed. restore y
C     and yprime to their original values.
C     reduce stepsize and try again, if
C     possible.
C---------------------------------------------------------
C
 
600   CONTINUE
      DO 610 I=1,NEQ
         Y(I)=PHI(I,1)
610      YPRIME(I)=PHI(I,2)
 
      IF (CONVGD) GO TO 640
      IF (IER.EQ.0) GO TO 620
         NSF=NSF+1
         H=H*0.25D0
         IF (NSF.LT.3.AND.DABS(H).GE.HMIN) GO TO 690
         IDID=-12
         RETURN
620   IF (IRES.GT.-2) GO TO 630
         IDID=-12
         RETURN
630   NCF=NCF+1
      H=H*0.25D0
      IF (NCF.LT.10.AND.DABS(H).GE.HMIN) GO TO 690
         IDID=-12
         RETURN
 
640   NEF=NEF+1
      R=0.90D0/(2.0D0*ERR+0.0001D0)
      R=DMAX1(0.1D0,DMIN1(0.5D0,R))
      H=H*R
      IF (DABS(H).GE.HMIN.AND.NEF.LT.10) GO TO 690
         IDID=-12
         RETURN
690      GO TO 200
 
C-------------end of subroutine ddaini----------------------
      END
      SUBROUTINE DDAJAC(NEQ,X,Y,YPRIME,DELTA,CJ,H,
     *  IER,WT,E,WM,IWM,RES,IRES,UROUND,JAC,RPAR,IPAR,PTN)
C
C***BEGIN PROLOGUE  DDAJAC
C***REFER TO  DDASSL
C***ROUTINES CALLED  DGEFA,DGBFA
C***COMMON BLOCKS    DDA001
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDAJAC
C-----------------------------------------------------------------------
C     this routine computes the iteration matrix
C     pd=dg/dy+cj*dg/dyprime (where g(x,y,yprime)=0).
C     here pd is computed by the user-supplied
C     routine jac if iwm(mtype) is 1 or 4, and
C     it is computed by numerical finite differencing
C     if iwm(mtype)is 2 or 5
C     the parameters have the following meanings.
C     y        = array containing predicted values
C     yprime   = array containing predicted derivatives
C     delta    = residual evaluated at (x,y,yprime)
C_2                (used only if iwm(mtype)=2 or 5)
C                (used only if iwm(itype)=2 or 3)
C     cj       = scalar parameter defining iteration matrix
C     h        = current stepsize in integration
C     ier      = variable which is .ne. 0
C                if iteration matrix is singular or could not be 
C                decomposed for another reason,
C                and 0 otherwise.
C     wt       = vector of weights for computing norms
C     e        = work space (temporary) of length neq
C     wm       = real work space for matrices. on
C                output it contains the lu decomposition
C                of the iteration matrix.
C     iwm      = integer work space containing
C                matrix information
C     res      = name of the external user-supplied routine
C                to evaluate the residual function g(x,y,yprime)
C     ires     = flag which is equal to zero if no illegal values
C                in res, and less than zero otherwise.  (if ires
C                is less than zero, the matrix was not completed)
C                in this case (if ires .lt. 0), then ier = 0.
C     uround   = the unit roundoff error of the machine being used.
C     jac      = name of the external user-supplied routine
C                to evaluate the iteration matrix (this routine
C                is only used if iwm(mtype) is 1 or 4)
C-----------------------------------------------------------------------C
C
C   C_2 R.KOENIGSDORFF 18.9.86 BER. D. NUM. ABLEITUNG FUER SPARSE MATRIX
C   C_3 "   "      "    " " "  EINBAU VON PTN IN PARAMETERKLAMMER UND EXT
C   C_5 "   "      "   26.9.86 ERWEITERUNG COMMON/DDA001/
C   C_6 "   "      "   20.10.86 EINBAU VON DDALDJ UND MA30BD
C   C_9 "   "      "   12.01.87 AENDERUNG DER SKELETT-MATRIX FUER PIVOT
C   C_10 "  "      "   16.05.87 ZURUECK ZU MA30AD BEI FEHLER IN MA30BD
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      EXTERNAL RES,JAC,PTN
      DIMENSION Y(1),YPRIME(1),DELTA(1),WT(1),E(1)
      DIMENSION WM(1),IWM(1),RPAR(1),IPAR(1)
C_5
      COMMON/DDA001/NPD,NTEMP,
     *  LML,LMU,LMXORD,LMTYPE,
     *  LNST,LNRE,LNJE,LETF,LCTF,LIPVT,
     *  KWKN,KJAC,KLUD,KIPTR,KLENB,KICNB,KLENR,KLENRL,
     *  KIPA,KIQA,KICN,JIRN,KLENC,JIFIRST,JLASTR,JNEXTR,JLASTC,JNEXTC,
     *  JIPC,IDISP(2),LENOFF(1),JACSIZ,LUDSIZ,PT1,NCG,KICNG,KICGP,LIRN
C_5 END
          COMMON/MA30FD/ IRNCP, ICNCP, IRANK, MINIRN, MINICN
C
      IER = 0
C_10
      IFMA30=0
C_10
      NPDM1=NPD-1
      MTYPE=IWM(LMTYPE)
      GO TO (100,200,300),MTYPE
C
C
C     dense user-supplied matrix
100   LENPD=NEQ*NEQ
      DO 110 I=1,LENPD
110      WM(NPDM1+I)=0.0D0
      CALL JAC(X,Y,YPRIME,WM(NPD),CJ,RPAR,IPAR)
      GO TO 230
C
C
C     dense finite-difference-generated matrix
200   IRES=0
      NROW=NPDM1
      SQUR = DSQRT(UROUND)
      DO 210 I=1,NEQ
         DEL=SQUR*DMAX1(DABS(Y(I)),DABS(H*YPRIME(I)),
     *     DABS(WT(I)))
         DEL=DSIGN(DEL,H*YPRIME(I))
         DEL=(Y(I)+DEL)-Y(I)
         YSAVE=Y(I)
         YPSAVE=YPRIME(I)
         Y(I)=Y(I)+DEL
         YPRIME(I)=YPRIME(I)+CJ*DEL
         CALL RES(X,Y,YPRIME,E,IRES,RPAR,IPAR)
         IF (IRES .LT. 0) RETURN
         DELINV=1.0D0/DEL
         DO 220 L=1,NEQ
220      WM(NROW+L)=(E(L)-DELTA(L))*DELINV
      NROW=NROW+NEQ
      Y(I)=YSAVE
      YPRIME(I)=YPSAVE
210   CONTINUE
C
C
C     do dense-matrix lu decomposition on pd
230      CALL DGEFA(WM(NPD),NEQ,NEQ,IWM(LIPVT),IER)
      RETURN
C
C
C_2     dummy section for iwm(mtype)=3
C     sparse finite-difference-generated matrix
300   IF (IWM(LNJE) .GT. 1) GO TO 310
         CALL PTN(IWM(KLENB),NEQ,IWM(KICNB),JACSIZ,NIRN,RPAR,IPAR)
         IF (JACSIZ.LT.NIRN) STOP 'DASSL: JACSIZ.LT.NIRN'
         NEQ1=NEQ+1
         NEQ2=2*NEQ
         CALL DDASCO(IWM(KICNB),IWM(KLENB),NEQ,NEQ,IWM(KICNG),
     *     IWM(KICGP),NCG,IWM(KLENR),NEQ2,NEQ1,NIRN)
C
310   KLUDP=KLUD+NEQ
      CALL DDADIF (NEQ,X,Y,YPRIME,DELTA,CJ,H,WT,E,WM(KJAC),WM(KLUD),
     *  WM(KWKN),WM(KLUDP),NCG,IWM(KIPTR),IWM(KLENB),IWM(KICNG),
     *  IWM(KICGP),IWM(KICNB),RES,IRES,UROUND,RPAR,IPAR)
      IF (IRES .LT. 0) RETURN
      IF (IWM(LNJE).GT.1) GO TO 340
350      CALL DDALDS(WM(KJAC),IWM(KLENB),IWM(KICNB),NIRN,WM(KLUD),
     *     IWM(KICN),LUDSIZ,IWM(KLENR),IDISP,IWM(KIPA),IWM(KIQA),
     *     NEQ, NEQ)
C_9     *     NEQ, 0)
         U=PT1
         CALL MA30AD(NEQ,IWM(KICN),WM(KLUD),LUDSIZ,IWM(KLENR),
     *     IWM(KLENRL),IDISP,IWM(KIPA),IWM(KIQA),IWM(JIRN),LIRN,
     *     IWM(KLENC),IWM(JIFIRST),IWM(JLASTR),IWM(JNEXTR),IWM(JLASTC),
     *     IWM(JNEXTC),IWM(KIPTR),IWM(JIPC),U,IFLAG)
         IF (IFLAG.NE.0) THEN
           WRITE(*,*) ' DASSL, DDAJAC: MA30AD, IFLAG.NE.0, IFLAG =',
     *                  IFLAG
           IER = - 1
         ENDIF
         IF (IER .NE. 0) RETURN
C
C
C     do sparse-matrix lu decomposition on pd
C_6
 340     CALL DDALDJ(WM(KJAC),IWM(KICNB),NIRN,IWM(KLENB),WM(KLUD),
     *     IWM(KICN),LUDSIZ,IWM(KLENR),IWM(KIPA),IWM(KIQA),IDISP,
     *     WM(KWKN),IWM(KIPTR),NEQ)
         CALL MA30BD(NEQ,IWM(KICN),WM(KLUD),LUDSIZ,IWM(KLENR),
     *     IWM(KLENRL),IDISP,IWM(KIPA),IWM(KIQA),WM(KWKN),
     *     IWM(KIPTR),IFLAG)
         IF (IFLAG.NE.0) WRITE(*,*) ' DASSL, DDAJAC: MA30BD, IFLAG.NE.0'
         IF (IFLAG.GT.0) WRITE(*,*) ' Pivot I is very small, I =',IFLAG
         IF (IFLAG.LT.0) WRITE(*,*) ' Unexpected singularity at stage I
     *                                of the decomposition, I =',IFLAG
C_6
C_10
         IF (IFLAG.NE.0 .AND. IFMA30.EQ.0) THEN
           IFMA30=1
           GOTO 350
         ELSE IF (IFLAG .NE. 0) THEN
           IER = -2
         ENDIF
C_10
      RETURN
C------end of subroutine ddajac------
      END
      DOUBLE PRECISION FUNCTION DDANRM(NEQ,V,WT,RPAR,IPAR)
C
C***BEGIN PROLOGUE  DDANRM
C***REFER TO  DDASSL
C***ROUTINES CALLED  (NONE)
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDANRM
C-----------------------------------------------------------------------
C     this function routine computes the weighted
C     root-mean-square norm of the vector of length
C     neq contained in the array v,with weights
C     contained in the array wt of length neq.
C        ddanrm=sqrt((1/neq)*sum(v(i)/wt(i))**2)
C-----------------------------------------------------------------------
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION V(NEQ),WT(NEQ)
      DIMENSION RPAR(1),IPAR(1)
      DDANRM = 0.0D0
      VMAX = 0.0D0
      DO 10 I = 1,NEQ
10      IF(DABS(V(I)/WT(I)) .GT. VMAX) VMAX = DABS(V(I)/WT(I))
      IF(VMAX .LE. 0.0D0) GO TO 30
      SUM = 0.0D0
      DO 20 I = 1,NEQ
20      SUM = SUM + ((V(I)/WT(I))/VMAX)**2
      DDANRM = VMAX*DSQRT(SUM/DFLOAT(NEQ))
30    CONTINUE
      RETURN
C------end of function ddanrm------
      END
      SUBROUTINE DDASLV(NEQ,DELTA,WM,IWM)
C
C***BEGIN PROLOGUE  DDASLV
C***REFER TO  DDASSL
C***ROUTINES CALLED DGESL,DGBSL
C***COMMON BLOCKS    DDA001
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDASLV
C-----------------------------------------------------------------------
C     this routine manages the solution of the linear
C     system arising in the newton iteration.
C     matrices and real temporary storage and
C     real information are stored in the array wm.
C     integer matrix information is stored in
C     the array iwm.
C     for a dense matrix, the linpack routine
C     dgesl is called.
C     for a banded matrix,the linpack routine
C     dgbsl is called
C-----------------------------------------------------------------------
C
C     C_4 R.KOENIGSDORFF 24.9.86 MA30CD EINGEBAUT
C     C_5 "  "   "    "  26.9.86 ERWEITERUNG COMMON/DDA001/
C
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION DELTA(1),WM(1),IWM(1)
C_5
      COMMON/DDA001/NPD,NTEMP,LML,LMU,
     *  LMXORD,LMTYPE,
     *  LNST,LNRE,LNJE,LETF,LCTF,LIPVT,
     *  KWKN,KJAC,KLUD,KIPTR,KLENB,KICNB,KLENR,KLENRL,
     *  KIPA,KIQA,KICN,JIRN,KLENC,JIFIRST,JLASTR,JNEXTR,JLASTC,JNEXTC,
     *  JIPC,IDISP(2),LENOFF(1),JACSIZ,LUDSIZ,PT1,NCG,KICNG,KICGP,LIRN
C_5 END
C
      MTYPE=IWM(LMTYPE)
      GO TO(100,100,300),MTYPE
C
C     dense matrix
100   CALL DGESL(WM(NPD),NEQ,NEQ,IWM(LIPVT),DELTA,0)
      RETURN
C
C_4     sparse matrix
300   CALL MA30CD (NEQ,IWM(KICN),WM(KLUD),LUDSIZ,IWM(KLENR),IWM(KLENRL),
     *  LENOFF,IDISP,IWM(KIPA),IWM(KIQA),DELTA,WM(KWKN),2)
      RETURN
C------end of subroutine ddaslv------
      END
      SUBROUTINE DDALDJ(B,ICNB,JACLEN,LENB,A,ICN,LUDLEN,LENR
     1,IP,IQ,IDISP,W,IW,N)
      DOUBLE PRECISION B(JACLEN), A(LUDLEN), W(N), ZERO
      INTEGER DBLK, IW(N), IDISP(2)
      INTEGER ICNB(JACLEN), LENB(N), ICN(LUDLEN), LENR(N)
     1, IP(N),IQ(N)
      DATA ZERO /0.D0/
C
C   RELOAD TRANSPOSED NEWTON MATRIX, PARTITIONED (NALGB,NEQ-NALGB)
C
C                        T
C      ( J(1,1)  J(1,2) )
C      ( J(2,1)  J(2,2) )
C
C   FOR DECOMPOSITION BY MA30B USING OLD PIVOTAL SEQUENCE
C
C IDISP IS THE POSITION IN A/ICN OF THE FIRST ELEMENT
      LTF=1
      DBLK=IDISP(1)
      IW(1)=1
      DO 10 I=1,N
      IF(I.LT.N)IW(I+1)=IW(I)+LENB(I)
 10   W(I)=ZERO
C EACH PASS THROUGH THIS MAIN LOOP PUTS ROW I OF THE PERMUTED FORM
C     (ROW IOLD IN THE ORIGINAL MATRIX) INTO THE RIGHT PLACE IN A
      DO 60 I=1,N
C LOAD ROW IOLD OF B TRANSPOSED INTO VECTOR W.
      IOLD=IABS(IP(I)+0)
      J1=IW(IOLD)
      J2=J1+LENB(IOLD)-1
      IF(J1.GT.J2)GO TO 30
      DO 20 JJ=J1,J2
      J=ICNB(JJ)
      W(J)=B(JJ)
   20 CONTINUE
   30 IF (LENR(I).EQ.0) GO TO 60
C UNLOAD ROW IOLD (ROW I IN PERMUTED FORM)
C     FROM W INTO APPROPRIATE PART OF A.
      J1=DBLK
      J2=J1+LENR(I)-1
      DO 50 JJ=J1,J2
      K=ICN(JJ)
      J=IQ(K)
      A(JJ)=W(J)
 50   W(J)=ZERO
      DBLK=J2+1
 60   CONTINUE
      RETURN
      END
      SUBROUTINE DDALDS(B,LENB,ICNB,JACLEN,A,ICN,LUDLEN,LENR,IDX
     1,IP,IQ,N,IMPLI)
      DOUBLE PRECISION B(JACLEN), A(LUDLEN), ZERO, ONE
      INTEGER LENB(N), ICNB(JACLEN), ICN(LUDLEN), LENR(N),
     1 IP(N), IQ(N)
      INTEGER IDX(2)
      DATA ZERO, ONE /0.D0, 1.D0/
C
C   LOADS SKELETON NEWTON MATRIX (PARTITIONED IF NALGB .GT. 0)
C
C                          T
C      (    I         0   )
C      (    0         I   )
C
C   FOR DECOMPOSITION BY MA30A TO CHOOSE PIVOTAL SEQUENCE
C
      K2=0
      J2=0
      DO 5 I=1,N
      J1=J2+1
      J2=J2+LENB(I)
      LR=LENB(I)
      IF(I.LE.IMPLI) GO TO 3
      LR=LR+1
      IF(J1.GT.J2)GO TO 3
      DO 2 J=J1,J2
      IF(ICNB(J).EQ.I)LR=LR-1
    2 CONTINUE
    3 LENR(I)=LR
      IP(I)=I
      IQ(I)=I
    5 K2=K2+LR
      K1=LUDLEN+1-K2
      IDX(1)=1
      IDX(2)=K1
      J2=0
      DO 200 I=1,N
      JM=LENR(I)
      J1=J2+1
      J2=J2+LENB(I)
      IF(I.GT.IMPLI)GO TO 8
      IF(J1.GT.J2)GO TO 20
      DO 6 J=J1,J2
      IC=ICNB(J)
      A(K1)=ZERO
      IF(IC.LE.IMPLI)A(K1)=B(J)
      ICN(K1)=IC
    6 K1=K1+1
      GO TO 20
    8 IF(J1.GT.J2)GO TO 15
      J0=0
      DO 10 J=J1,J2
      IC=ICNB(J)
      IF(IC.LE.IMPLI)GO TO 63
      IF(IC-I)63,62,61
   61 IF(J0.GT.0)GO TO 63
      J0=1
      A(K1)=ONE
      ICN(K1)=I
      K1=K1+1
      GO TO 63
   62 A(K1)=ONE
      J0=1
      GO TO 64
   63 A(K1)=ZERO
   64 ICN(K1)=IC
      K1=K1+1
   10 CONTINUE
      IF(J0.NE.0)GO TO 20
   15 A(K1)=ONE
      ICN(K1)=I
      K1=K1+1
   20 CONTINUE
  200 CONTINUE
      RETURN
      END
      SUBROUTINE DDASCO(IR,IP,M,N,IC,IPC,NC,IW,MPN,NP1,JACSIZ)
C
C   GROUPS COLUMNS OF SPARSE JACOBIAN MATRIX FOR FINITE
C   DIFFERENCE EVALUATION
C
C      M IS NUMBER OF ROWS
C      N IS NUMBER OF COLUMNS
C      IR CONTAINS ROW NUMBERS OF NON-ZEROS.
C      IP CONTAINS NUMBERS OF NONZEROS IN EACH COLUMN.
C      IC IS SET TO COLUMN NUMBERS WITHIN GROUPS
C            AND MUST HAVE N ENTRIES.
C      IPC IS SET TO POINT TO FIRST ENTRY OF IC FOR EACH
C            GROUP, AND MAY NEED N+1 ENTRIES.
C      NC IS SET TO (NUMBER OF GROUPS+1).
Change
C      INTEGER*2 IR(JACSIZ),IP(N),IC(N),IPC(NP1)
      INTEGER IR(JACSIZ),IP(N),IC(N),IPC(NP1)
Change
C      IW IS WORKSPACE, IT NEEDS (M+N) ENTRIES
      INTEGER IW(MPN)
      NC=1
      ICC=1
      NM=N+M
      DO 1 J=1,NM
    1 IW(J)=0
   10 IPC(NC)=ICC
      DO 2 J=1,M
    2 IW(J)=0
      KCOL=1
      JST=1
   50 JND=JST+IP(KCOL)-1
      K=KCOL+M
      IF(IW(K).NE.0)GO TO 20
      IF(JND.LT.JST)GO TO 30
      DO 3 J=JST,JND
      K=IR(J)
      IF(IW(K).NE.0)GO TO 40
    3 CONTINUE
C      ACCEPT COLUMN
      DO 4 J=JST,JND
      K=IR(J)
    4 IW(K)=1
   30 IC(ICC)=KCOL
      ICC=ICC+1
      K=KCOL+M
      IW(K)=1
C      REJECT COLUMN
   40 CONTINUE
C      COLUMN ALREADY USED
   20 KCOL=KCOL+1
      JST=JND+1
      IF(KCOL.LE.N)GO TO 50
      IF(ICC.EQ.IPC(NC))GO TO 60
      NC=NC+1
      GO TO 10
   60 RETURN
      END
      SUBROUTINE DDADIF(NEQ,X,Y,YPRIME,DELTA,CJ,H,WT,E,B,DELY,YSAVE,
     *  YPSAVE,NCG,IPTR,LENB,ICNG,ICGP,ICNB,RES,IRES,UROUND,RPAR,IPAR)
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      EXTERNAL RES
      DIMENSION Y(1),YPRIME(1),DELTA(1),WT(1),E(1),YSAVE(1),YPSAVE(1)
      DIMENSION IPTR(1),LENB(1),ICNG(1),ICGP(1),ICNB(1)
      DIMENSION B(1),DELY(1),RPAR(1),IPAR(1)
C
C        R.KOENIGSDORFF 24.10.86
C
      IRES=0
      SQUR=DSQRT(UROUND)
      IPTR(1)=1
      N1=NEQ-1
      DO 5 I=1,N1
         IPTR(I+1)=IPTR(I)+LENB(I)
    5 CONTINUE
      DO 10 KK=2,NCG
         JST=ICGP(KK-1)
         JND=ICGP(KK)-1
         DO 20 K=JST,JND
            J=ICNG(K)
    	    DELY(J)=SQUR*DMAX1(DABS(Y(J)),DABS(H*YPRIME(J)),DABS(WT(J)))
            DELY(J)=DSIGN(DELY(J),H*YPRIME(J))
            DELY(J)=(Y(J)+DELY(J))-Y(J)
            YSAVE(J)=Y(J)
            YPSAVE(J)=YPRIME(J)
            Y(J)=Y(J)+DELY(J)
            YPRIME(J)=YPRIME(J)+CJ*DELY(J)
   20    CONTINUE
         CALL RES(X,Y,YPRIME,E,IRES,RPAR,IPAR)
         IF (IRES .LT. 0) RETURN
         DO 30 K=JST,JND
            J=ICNG(K)
 	    IST=IPTR(J)
            IND=IST+LENB(J)-1
            DO 40 II=IST,IND
               I=ICNB(II)
               DELINV=1.0D0/DELY(J)
      	       B(II)=(E(I)-DELTA(I))*DELINV
   40       CONTINUE
            Y(J)=YSAVE(J)
            YPRIME(J)=YPSAVE(J)
   30    CONTINUE
   10 CONTINUE
      RETURN
      END
      SUBROUTINE DDASSL (RES,NEQ,T,Y,YPRIME,TOUT,
     *  INFO,RTOL,ATOL,IDID,
     *  RWORK,LRW,IWORK,LIW,RPAR,IPAR,
     *  JAC,PTN,ICHAN)
C
C***BEGIN PROLOGUE  DDASSL
C
C    :
C    :
C    :
C    :
C
CC***ROUTINES CALLED  DDASTP,DDAINI,DDANRM,DDAWTS,DDATRP,XERRWV,D1MACH
C***COMMON BLOCKS    DDA001
C***END PROLOGUE DDASSL
C
C
C    C_2 "    "      "   17.9.86 SPARSE OPTION:MTYPE,LENPD,LENRW,LENIW
C    C_3 "    "      "    " " "  EINBAU VON PTN IN PARAMETERKLAMM. U.EXT
C    C_5 "    "      "   26.9.86 ERWEITERUNG VON COMMON/DDA001/
C    C_8 R,KOENIGSDORFF 12.01.87 EINBAU VON COMMON/JACV1/
C    C_10 A.K.          14.07.87 JACOBIDIMENSION. MIT LENJVD+LENJVS
C    C_11 A.K.          19.11.87 fuer ICHAN=1 Neustart ohne Patterbest.
C    C_12 A.K.          26.05.89 Adressrechng. ueberprueft, KWKN=1 da
C                                Restlaenge von RWORK uebergeben wird.
C                     
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      LOGICAL DONE
      EXTERNAL RES,JAC,PTN
      DIMENSION Y(1),YPRIME(1)
      DIMENSION INFO(15)
      DIMENSION RWORK(1),IWORK(1)
      DIMENSION RTOL(1),ATOL(1)
      DIMENSION RPAR(1),IPAR(1)
C_5
      COMMON/DDA001/NPD,NTEMP,
     *   LML,LMU,LMXORD,LMTYPE,
     *   LNST,LNRE,LNJE,LETF,LCTF,LIPVT,
     *   KWKN,KJAC,KLUD,KIPTR,KLENB,KICNB,KLENR,KLENRL,
     *   KIPA,KIQA,KICN,JIRN,KLENC,JIFIRST,JLASTR,JNEXTR,JLASTC,JNEXTC,
     *   JIPC,IDISPX(2),LENOFF(1),JACSIZ,LUDSIZ,PT1,NCG,KICNG,KICGP,LIRN
C_5 END
C_8
C
      COMMON /ANVE   / LENAVX,LENAVU,LENAVP,LENB
C
      COMMON / JACV  / LENFX,LENBX,LENRX,LENBU
C      COMMON / JACV1 / LENJVG,LENJVD,LENJVS,LENJAD,LENJAS,LENJMD,LENJMS
C_8 END
      DATA LTSTOP,LHMAX,LH,LTN,
     *   LCJ,LCJOLD,LHOLD,LS,LROUND,
     *   LALPHA,LBETA,LGAMMA,
     *   LPSI,LSIGMA,LDELTA
     *   /1,2,3,4,
     *   5,6,7,8,9,
     *   11,17,23,
     *   29,35,41/
      IF(INFO(1).NE.0)GO TO 100
C
C-----------------------------------------------------------------------
C     this block is executed for the initial call only.
C     it contains checking of inputs and initializations.
C-----------------------------------------------------------------------
C
C     first check info array to make sure all elements of info
C     are either zero or one.
      DO 10 I=2,11
         IF(INFO(I).NE.0.AND.INFO(I).NE.1)GO TO 701
10       CONTINUE
C
      IF(NEQ.LE.0)GO TO 702
C
C     set pointers into iwork
      LML=1
      LMU=2
      LMXORD=3
      LMTYPE=4
      LJCALC=5
      LPHASE=6
      LK=7
      LKOLD=8
      LNS=9
      LNSTL=10
      LNST=11
      LNRE=12
      LNJE=13
      LETF=14
      LCTF=15
      LIPVT=21
      LIWM=1
C
C     check and compute maximum order
      MXORD=5
      IF(INFO(9).EQ.0)GO TO 20
         MXORD=IWORK(LMXORD)
         IF(MXORD.LT.1.OR.MXORD.GT.5)GO TO 703
20       IWORK(LMXORD)=MXORD
C
C     compute mtype,lenpd,lenrw.check ml and mu.
      IF(INFO(6).NE.0)GO TO 40
         LENPD=NEQ**2
         LENRW=40+(IWORK(LMXORD)+4)*NEQ+LENPD
         LENIW=20+NEQ 
         IF(INFO(5).NE.0)GO TO 30
            IWORK(LMTYPE)=2
            GO TO 60
30          IWORK(LMTYPE)=1
            GO TO 60
C_2     SPARSE CASE
40    IWORK(LMTYPE)=3
         NII2=1
C_10>
         NJAC=LENFX + LENBX + LENB
C_ak160689         NLUD=2*(LENFX + LENBX + LENB)
         NLUD=3*(LENFX + LENBX + LENB)
C_10<
         MLUD=NLUD
         LENPD=NEQ+NJAC+NLUD
         LENRW=40+(IWORK(LMXORD)+4)*NEQ+LENPD
         LENIW=20+NEQ*(2+13/NII2)+(NJAC+NLUD+MLUD)/NII2
C
C     check lengths of rwork and iwork
C_2 60    LENIW=20+NEQ
60    IF(LRW.LT.LENRW)GO TO 704
      IF(LIW.LT.LENIW)GO TO 705
C
C     check to see that tout is different from t
      IF(TOUT .EQ. T)GO TO 719
C
C     check hmax
      IF(INFO(7).EQ.0)GO TO 70
         HMAX=RWORK(LHMAX)
         IF(HMAX.LE.0.0D0)GO TO 710
70    CONTINUE
C
C     initialize counters
      IWORK(LNST)=0
      IWORK(LNRE)=0
C_11>
      IF (ICHAN .EQ. 1) THEN
         IWORK(LNJE)=1
      ELSE 
         IWORK(LNJE)=0
      ENDIF
C_11<
      IWORK(LNSTL)=0
      IDID=1
      GO TO 200
C
C-----------------------------------------------------------------------
C     this block is for continuation calls
C     only. here we check info(1),and if the
C     last step was interrupted we check whether
C     appropriate action was taken.
C-----------------------------------------------------------------------
C
100   CONTINUE
      IF(INFO(1).EQ.1)GO TO 110
      IF(INFO(1).NE.-1)GO TO 701
C     if we are here, the last step was interrupted
C     by an error condition from ddastp,and
C     appropriate action was not taken. this
C     is a fatal error.
      CALL XERRWV(
     *49HDASSL--  THE LAST STEP TERMINATED WITH A NEGATIVE,
     *49,201,0,0,0,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *47HDASSL--  VALUE (=I1) OF IDID AND NO APPROPRIATE,
     *47,202,0,1,IDID,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *41HDASSL--  ACTION WAS TAKEN. RUN TERMINATED,
     *41,203,1,0,0,0,0,0.0D0,0.0D0)
      RETURN
110   CONTINUE
      IWORK(LNSTL)=IWORK(LNST)
C
C-----------------------------------------------------------------------
C     this block is executed on all calls.
C     the error tolerance parameters are
C     checked, and the work array pointers
C     are set.
C-----------------------------------------------------------------------
C
200   CONTINUE
C     check rtol,atol
      NZFLG=0
      RTOLI=RTOL(1)
      ATOLI=ATOL(1)
      DO 210 I=1,NEQ
         IF(INFO(2).EQ.1)RTOLI=RTOL(I)
         IF(INFO(2).EQ.1)ATOLI=ATOL(I)
         IF(RTOLI.GT.0.0D0.OR.ATOLI.GT.0.0D0)NZFLG=1
         IF(RTOLI.LT.0.0D0)GO TO 706
         IF(ATOLI.LT.0.0D0)GO TO 707
210      CONTINUE
      IF(NZFLG.EQ.0)GO TO 708
C
C     set up rwork storage.iwork storage is fixed
C     in data statement.
      LE=LDELTA+NEQ
      LWT=LE+NEQ
      LPHI=LWT+NEQ
      LPD=LPHI+(IWORK(LMXORD)+1)*NEQ
      LWM=LPD
C_7      
      IF (IWORK(LMTYPE).EQ.3) THEN
         LENOFF(1)=-1
         PT1=0.1D0
         JACSIZ=NJAC
C_12         KWKN=LWM
         KWKN=1
         KJAC=KWKN+NEQ
         KLUD=KJAC+JACSIZ
         KIPTR=LIPVT
         KLENB=KIPTR+NEQ
         MYY=(NEQ-1)/NII2+1
         KICNB=KLENB+MYY
         KICNG=KICNB+(JACSIZ-1)/NII2+1
         KICGP=KICNG+MYY
         KLENR=KICGP+MYY+1
         KLENRL=MYY+KLENR
         KIPA=KLENRL+MYY
         KIQA=KIPA+MYY
         KICN=KIQA+MYY
         JIPC=LIW+1-NEQ
         KLENC=JIPC-MYY
         JIFIRST=KLENC-MYY
         JLASTR=JIFIRST-MYY
         JNEXTR=JLASTR-MYY
         JLASTC=JNEXTR-MYY
         JNEXTC=JLASTC-MYY
C      remaning length on IWORK: KSIZ
         KSIZ=JNEXTC-KICN    
         KSIZN=(JACSIZ+1+2*NEQ)/NII2         
         KSIZM=KSIZN
         IF(KSIZ.LT.2*KSIZN) KSIZM=KSIZ/2
         KSIZM=(KSIZM+KSIZ)/3
C      remaning length on RWORK: LIAN
         LIAN=(LRW-KLUD-LWM+1)/NII2
C_12         LIAN=(LRW-KLUD+1)/NII2
         LUDSIZ=MIN0(KSIZ-KSIZM,LIAN)
         LIRN=MIN0(KSIZ-LUDSIZ,LIAN)
         JIRN=KICN+LUDSIZ         
         LUDSIZ=LUDSIZ*NII2
         LIRN=LIRN*NII2
      ENDIF
C_7 END
      NPD=1
      NTEMP=NPD+LENPD
      IF(INFO(1).EQ.1)GO TO 400
C
C-----------------------------------------------------------------------
C     this block is executed on the initial call
C     only. set the initial step size, and
C     the error weight vector, and phi.
C     compute initial yprime, if necessary.
C-----------------------------------------------------------------------
C
300   CONTINUE
      TN=T
      IDID=1
C
C     set error weight vector wt
      CALL DDAWTS(NEQ,INFO(2),RTOL,ATOL,Y,RWORK(LWT),RPAR,IPAR)
      DO 305 I = 1,NEQ
         IF(RWORK(LWT+I-1).LE.0.0D0) GO TO 713
305      CONTINUE
C
C     compute unit roundoff and hmin
      UROUND = D1MACH(4)
      RWORK(LROUND) = UROUND
      HMIN = 4.0D0*UROUND*DMAX1(DABS(T),DABS(TOUT))
C
C     check initial interval to see that it is long enough
      TDIST = DABS(TOUT - T)
      IF(TDIST .LT. HMIN) GO TO 714
C
C     check ho, if this was input
      IF (INFO(8) .EQ. 0) GO TO 310
         HO = RWORK(LH)
         IF ((TOUT - T)*HO .LT. 0.0D0) GO TO 711
         IF (HO .EQ. 0.0D0) GO TO 712
         GO TO 320
310    CONTINUE
C
C     compute initial stepsize, to be used by either
C     ddastp or ddaini, depending on info(11)
      HO = 0.001D0*TDIST
      YPNORM = DDANRM(NEQ,YPRIME,RWORK(LWT),RPAR,IPAR)
      IF (YPNORM .GT. 0.5D0/HO) HO = 0.5D0/YPNORM
      HO = DSIGN(HO,TOUT-T)
C     adjust ho if necessary to meet hmax bound
320   IF (INFO(7) .EQ. 0) GO TO 330
         RH = DABS(HO)/HMAX
         IF (RH .GT. 1.0D0) HO = HO/RH
C     compute tstop, if applicable
330   IF (INFO(4) .EQ. 0) GO TO 340
         TSTOP = RWORK(LTSTOP)
         IF ((TSTOP - T)*HO .LT. 0.0D0) GO TO 715
         IF ((T + HO - TSTOP)*HO .GT. 0.0D0) HO = TSTOP - T
         IF ((TSTOP - TOUT)*HO .LT. 0.0D0) GO TO 709
C
C     compute initial derivative, if applicable
340   IF (INFO(11) .EQ. 0) GO TO 350
C_020490ak
C     test : rwork(lphi) is the first address of (iwrok(maxord)+1)*neq elements
C     on rwork to hold the Nordsieck vector later on. Now 4*neq elements are
C     used to store old and intermediate values of Y and YPRIME during
C     determination of consistent intial conditions.
      if (iwork(lmxord) .lt. 3) then
         write(6,*) 'Array PHI is too small for DDAINI'
         write(6,*) 'maximum order should be greater than 3'
         stop '****DDASSL just before call to DDANINI *****'
      endif
C_020490ak
      CALL DDAINI(T,Y,YPRIME,NEQ,
     *  RES,JAC,HO,RWORK(LWT),IDID,RPAR,IPAR,
     *  RWORK(LPHI),RWORK(LDELTA),RWORK(LE),
     *  RWORK(LWM),IWORK(LIWM),HMIN,RWORK(LROUND),INFO(10),PTN)
      IF (IDID .LT. 0) GO TO 390
C
C     load h with ho.  store h in rwork(lh)
350   H = HO
      RWORK(LH) = H
C
C     load y and h*yprime into phi(*,1) and phi(*,2)
360   ITEMP = LPHI + NEQ
      DO 370 I = 1,NEQ
         RWORK(LPHI + I - 1) = Y(I)
370      RWORK(ITEMP + I - 1) = H*YPRIME(I)
C
390   GO TO 500
C
C-------------------------------------------------------
C     this block is for continuation calls only. its
C     purpose is to check stop conditions before
C     taking a step.
C     adjust h if necessary to meet hmax bound
C-------------------------------------------------------
C
400   CONTINUE
      DONE = .FALSE.
      TN=RWORK(LTN)
      H=RWORK(LH)
      IF(INFO(7) .EQ. 0) GO TO 410
         RH = DABS(H)/HMAX
         IF(RH .GT. 1.0D0) H = H/RH
410   CONTINUE
      IF(T .EQ. TOUT) GO TO 719
      IF((T - TOUT)*H .GT. 0.0D0) GO TO 711
      IF(INFO(4) .EQ. 1) GO TO 430
      IF(INFO(3) .EQ. 1) GO TO 420
      IF((TN-TOUT)*H.LT.0.0D0)GO TO 490
      CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,IWORK(LKOLD),
     *  RWORK(LPHI),RWORK(LPSI))
      T=TOUT
      IDID = 3
      DONE = .TRUE.
      GO TO 490
420   IF((TN-T)*H .LE. 0.0D0) GO TO 490
      IF((TN - TOUT)*H .GT. 0.0D0) GO TO 425
      CALL DDATRP(TN,TN,Y,YPRIME,NEQ,IWORK(LKOLD),
     *  RWORK(LPHI),RWORK(LPSI))
      T = TN
      IDID = 1
      DONE = .TRUE.
      GO TO 490
425   CONTINUE
      CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,IWORK(LKOLD),
     *  RWORK(LPHI),RWORK(LPSI))
      T = TOUT
      IDID = 3
      DONE = .TRUE.
      GO TO 490
430   IF(INFO(3) .EQ. 1) GO TO 440
      TSTOP=RWORK(LTSTOP)
      IF((TN-TSTOP)*H.GT.0.0D0) GO TO 715
      IF((TSTOP-TOUT)*H.LT.0.0D0)GO TO 709
      IF((TN-TOUT)*H.LT.0.0D0)GO TO 450
      CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,IWORK(LKOLD),
     *   RWORK(LPHI),RWORK(LPSI))
      T=TOUT
      IDID = 3
      DONE = .TRUE.
      GO TO 490
440   TSTOP = RWORK(LTSTOP)
      IF((TN-TSTOP)*H .GT. 0.0D0) GO TO 715
      IF((TSTOP-TOUT)*H .LT. 0.0D0) GO TO 709
      IF((TN-T)*H .LE. 0.0D0) GO TO 450
      IF((TN - TOUT)*H .GT. 0.0D0) GO TO 445
      CALL DDATRP(TN,TN,Y,YPRIME,NEQ,IWORK(LKOLD),
     *  RWORK(LPHI),RWORK(LPSI))
      T = TN
      IDID = 1
      DONE = .TRUE.
      GO TO 490
445   CONTINUE
      CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,IWORK(LKOLD),
     *  RWORK(LPHI),RWORK(LPSI))
      T = TOUT
      IDID = 3
      DONE = .TRUE.
      GO TO 490
450   CONTINUE
C     check whether we are with in roundoff of tstop
      IF(DABS(TN-TSTOP).GT.100.0D0*UROUND*
     *   (DABS(TN)+DABS(H)))GO TO 460
      IDID=2
      T=TSTOP
      DONE = .TRUE.
      GO TO 490
460   TNEXT=TN+H*(1.0D0+4.0D0*UROUND)
      IF((TNEXT-TSTOP)*H.LE.0.0D0)GO TO 490
      H=(TSTOP-TN)*(1.0D0-4.0D0*UROUND)
      RWORK(LH)=H
C
490   IF (DONE) GO TO 590
C
C-------------------------------------------------------
C     the next block contains the call to the
C     one-step integrator ddastp.
C     this is a looping point for the integration
C     steps.
C     check for too many steps.
C     update wt.
C     check for too much accuracy requested.
C     compute minimum stepsize.
C-------------------------------------------------------
C
500   CONTINUE
C     check for failure to compute initial yprime
      IF (IDID .EQ. -12) GO TO 527
C
C     check for too many steps
      IF((IWORK(LNST)-IWORK(LNSTL)).LT.500)
     *   GO TO 510
           IDID=-1
           GO TO 527
C
C     update wt
510   CALL DDAWTS(NEQ,INFO(2),RTOL,ATOL,RWORK(LPHI),
     *  RWORK(LWT),RPAR,IPAR)
      DO 520 I=1,NEQ
         IF(RWORK(I+LWT-1).GT.0.0D0)GO TO 520
           IDID=-3
           GO TO 527
520   CONTINUE
C
C     test for too much accuracy requested.
      R=DDANRM(NEQ,RWORK(LPHI),RWORK(LWT),RPAR,IPAR)*
     *   100.0D0*UROUND
      IF(R.LE.1.0D0)GO TO 525
C     multiply rtol and atol by r and return
      IF(INFO(2).EQ.1)GO TO 523
           RTOL(1)=R*RTOL(1)
           ATOL(1)=R*ATOL(1)
           IDID=-2
           GO TO 527
523   DO 524 I=1,NEQ
           RTOL(I)=R*RTOL(I)
524        ATOL(I)=R*ATOL(I)
      IDID=-2
      GO TO 527
525   CONTINUE
C
C     compute minimum stepsize
      HMIN=4.0D0*UROUND*DMAX1(DABS(TN),DABS(TOUT))
C
      CALL DDASTP(TN,Y,YPRIME,NEQ,
     *   RES,JAC,H,RWORK(LWT),INFO(1),IDID,RPAR,IPAR,
     *   RWORK(LPHI),RWORK(LDELTA),RWORK(LE),
     *   RWORK(LWM),IWORK(LIWM),
     *   RWORK(LALPHA),RWORK(LBETA),RWORK(LGAMMA),
     *   RWORK(LPSI),RWORK(LSIGMA),
     *   RWORK(LCJ),RWORK(LCJOLD),RWORK(LHOLD),
     *   RWORK(LS),HMIN,RWORK(LROUND),
     *   IWORK(LPHASE),IWORK(LJCALC),IWORK(LK),
     *   IWORK(LKOLD),IWORK(LNS),INFO(10),PTN)
527   IF(IDID.LT.0)GO TO 600
C
C------------------------------------------------------
C     this block handles the case of a successful
C     return from ddastp (idid=1) test for
C     stop conditions.
C--------------------------------------------------------
C
      IF(INFO(4).NE.0)GO TO 540
           IF(INFO(3).NE.0)GO TO 530
             IF((TN-TOUT)*H.LT.0.0D0)GO TO 500
             CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,
     *         IWORK(LKOLD),RWORK(LPHI),RWORK(LPSI))
             IDID=3
             T=TOUT
             GO TO 580
530          IF((TN-TOUT)*H.GE.0.0D0)GO TO 535
             T=TN
             IDID=1
             GO TO 580
535          CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,
     *         IWORK(LKOLD),RWORK(LPHI),RWORK(LPSI))
             IDID=3
             T=TOUT
             GO TO 580
540   IF(INFO(3).NE.0)GO TO 550
      IF((TN-TOUT)*H.LT.0.0D0)GO TO 542
         CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,
     *     IWORK(LKOLD),RWORK(LPHI),RWORK(LPSI))
         T=TOUT
         IDID=3
         GO TO 580
542   IF(DABS(TN-TSTOP).LE.100.0D0*UROUND*
     *   (DABS(TN)+DABS(H)))GO TO 545
      TNEXT=TN+H*(1.0D0+4.0D0*UROUND)
      IF((TNEXT-TSTOP)*H.LE.0.0D0)GO TO 500
      H=(TSTOP-TN)*(1.0D0-4.0D0*UROUND)
      GO TO 500
545   IDID=2
      T=TSTOP
      GO TO 580
550   IF((TN-TOUT)*H.GE.0.0D0)GO TO 555
      IF(DABS(TN-TSTOP).LE.100.0D0*UROUND*(DABS(TN)+DABS(H)))GO TO 552
      T=TN
      IDID=1
      GO TO 580
552   IDID=2
      T=TSTOP
      GO TO 580
555   CALL DDATRP(TN,TOUT,Y,YPRIME,NEQ,
     *   IWORK(LKOLD),RWORK(LPHI),RWORK(LPSI))
      T=TOUT
      IDID=3
580   CONTINUE
C
C--------------------------------------------------------
C     all successful returns from ddassl are made from
C     this block.
C--------------------------------------------------------
C
590   CONTINUE
      RWORK(LTN)=TN
      RWORK(LH)=H
      RETURN
C
C-----------------------------------------------------------------------
C     this block handles all unsuccessful
C     returns other than for illegal input.
C-----------------------------------------------------------------------
C
600   CONTINUE
      ITEMP=-IDID
      GO TO (610,620,630,690,690,640,650,660,670,675,
     *  680,685), ITEMP
C
C     the maximum number of steps was taken before
C     reaching tout
610   CALL XERRWV(
     *38HDASSL--  AT CURRENT T (=R1)  500 STEPS,
     *38,610,0,0,0,0,1,TN,0.0D0)
      CALL XERRWV(48HDASSL--  TAKEN ON THIS CALL BEFORE REACHING TOUT,
     *48,611,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     too much accuracy for machine precision
620   CALL XERRWV(
     *47HDASSL--  AT T (=R1) TOO MUCH ACCURACY REQUESTED,
     *47,620,0,0,0,0,1,TN,0.0D0)
      CALL XERRWV(
     *48HDASSL--  FOR PRECISION OF MACHINE. RTOL AND ATOL,
     *48,621,0,0,0,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *45HDASSL--  WERE INCREASED TO APPROPRIATE VALUES,
     *45,622,0,0,0,0,0,0.0D0,0.0D0)
C
      GO TO 690
C     wt(i) .le. 0.0d0 for some i (not at start of problem)
630   CALL XERRWV(
     *38HDASSL--  AT T (=R1) SOME ELEMENT OF WT,
     *38,630,0,0,0,0,1,TN,0.0D0)
      CALL XERRWV(28HDASSL--  HAS BECOME .LE. 0.0,
     *28,631,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     error test failed repeatedly or with h=hmin
640   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,640,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *57HDASSL--  ERROR TEST FAILED REPEATEDLY OR WITH ABS(H)=HMIN,
     *57,641,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     corrector convergence failed repeatedly or with h=hmin
650   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,650,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *48HDASSL--  CORRECTOR FAILED TO CONVERGE REPEATEDLY,
     *48,651,0,0,0,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *28HDASSL--  OR WITH ABS(H)=HMIN,
     *28,652,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     the iteration matrix is singular
660   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,660,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *37HDASSL--  ITERATION MATRIX IS SINGULAR,
     *37,661,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     corrector failure preceeded by error test failures.
670   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,670,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *49HDASSL--  CORRECTOR COULD NOT CONVERGE.  ALSO, THE,
     *49,671,0,0,0,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *38HDASSL--  ERROR TEST FAILED REPEATEDLY.,
     *38,672,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     corrector failure because ires = -1
675   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,675,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *45HDASSL--  CORRECTOR COULD NOT CONVERGE BECAUSE,
     *455,676,0,0,0,0,0,0.0D0,0.0D0)
      CALL XERRWV(
     *36HDASSL--  IRES WAS EQUAL TO MINUS ONE,
     *36,677,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     failure because ires = -2
680   CALL XERRWV(
     *40HDASSL--  AT T (=R1) AND STEPSIZE H (=R2),
     *40,680,0,0,0,0,2,TN,H)
      CALL XERRWV(
     *36HDASSL--  IRES WAS EQUAL TO MINUS TWO,
     *36,681,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
C
C     failed to compute initial yprime
685   CALL XERRWV(
     *44HDASSL--  AT T (=R1) AND STEPSIZE H (=R2) THE,
     *44,685,0,0,0,0,2,TN,HO)
      CALL XERRWV(
     *45HDASSL--  INITIAL YPRIME COULD NOT BE COMPUTED,
     *45,686,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 690
690   CONTINUE
      INFO(1)=-1
      T=TN
      RWORK(LTN)=TN
      RWORK(LH)=H
      RETURN
C-----------------------------------------------------------------------
C     this block handles all error returns due
C     to illegal input, as detected before calling
C     ddastp. first the error message routine is
C     called. if this happens twice in
C     succession, execution is terminated
C
C-----------------------------------------------------------------------
701   CALL XERRWV(
     *55HDASSL--  SOME ELEMENT OF INFO VECTOR IS NOT ZERO OR ONE,
     *55,1,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
702   CALL XERRWV(25HDASSL--  NEQ (=I1) .LE. 0,
     *25,2,0,1,NEQ,0,0,0.0D0,0.0D0)
      GO TO 750
703   CALL XERRWV(34HDASSL--  MAXORD (=I1) NOT IN RANGE,
     *34,3,0,1,MXORD,0,0,0.0D0,0.0D0)
      GO TO 750
704   CALL XERRWV(
     *60HDASSL--  RWORK LENGTH NEEDED, LENRW (=I1), EXCEEDS LRW (=I2),
     *60,4,0,2,LENRW,LRW,0,0.0D0,0.0D0)
      GO TO 750
705   CALL XERRWV(
     *60HDASSL--  IWORK LENGTH NEEDED, LENIW (=I1), EXCEEDS LIW (=I2),
     *60,5,0,2,LENIW,LIW,0,0.0D0,0.0D0)
      GO TO 750
706   CALL XERRWV(
     *39HDASSL--  SOME ELEMENT OF RTOL IS .LT. 0,
     *39,6,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
707   CALL XERRWV(
     *39HDASSL--  SOME ELEMENT OF ATOL IS .LT. 0,
     *39,7,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
708   CALL XERRWV(
     *47HDASSL--  ALL ELEMENTS OF RTOL AND ATOL ARE ZERO,
     *47,8,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
709   CALL XERRWV(
     *54HDASSL--  INFO(4) = 1 AND TSTOP (=R1) BEHIND TOUT (=R2),
     *54,9,0,0,0,0,2,TSTOP,TOUT)
      GO TO 750
710   CALL XERRWV(28HDASSL--  HMAX (=R1) .LT. 0.0,
     *28,10,0,0,0,0,1,HMAX,0.0D0)
      GO TO 750
711   CALL XERRWV(34HDASSL--  TOUT (=R1) BEHIND T (=R2),
     *34,11,0,0,0,0,2,TOUT,T)
      GO TO 750
712   CALL XERRWV(29HDASSL--  INFO(8)=1 AND H0=0.0,
     *29,12,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
713   CALL XERRWV(39HDASSL--  SOME ELEMENT OF WT IS .LE. 0.0,
     *39,13,0,0,0,0,0,0.0D0,0.0D0)
      GO TO 750
714   CALL XERRWV(
     *61HDASSL--  TOUT (=R1) TOO CLOSE TO T (=R2) TO START INTEGRATION,
     *61,14,0,0,0,0,2,TOUT,T)
      GO TO 750
715   CALL XERRWV(
     *49HDASSL--  INFO(4)=1 AND TSTOP (=R1) BEHIND T (=R2),
     *49,15,0,0,0,0,2,TSTOP,T)
      GO TO 750
719   CALL XERRWV(
     *39HDASSL--  TOUT (=R1) IS EQUAL TO T (=R2),
     *39,19,0,0,0,0,2,TOUT,T)
      GO TO 750
750   IF(INFO(1).EQ.-1) GO TO 760
      INFO(1)=-1
      IDID=-33
      RETURN
760   CALL XERRWV(
     *46HDASSL--  REPEATED OCCURRENCES OF ILLEGAL INPUT,
     *46,801,0,0,0,0,0,0.0D0,0.0D0)
770   CALL XERRWV(
     *47HDASSL--  RUN TERMINATED. APPARENT INFINITE LOOP,
     *47,802,1,0,0,0,0,0.0D0,0.0D0)
      RETURN
C-----------end of subroutine ddassl-------------------------------------
      END
      SUBROUTINE DDASTP(X,Y,YPRIME,NEQ,
     *  RES,JAC,H,WT,JSTART,IDID,RPAR,IPAR,
     *  PHI,DELTA,E,WM,IWM,
     *  ALPHA,BETA,GAMMA,PSI,SIGMA,
     *  CJ,CJOLD,HOLD,S,HMIN,UROUND,
     *  IPHASE,JCALC,K,KOLD,NS,NONNEG,PTN)
C
C***BEGIN PROLOGUE  DDASTP
C***REFER TO  DDASSL
C***ROUTINES CALLED  DDANRM,DDAJAC,DDASLV,DDATRP
C***COMMON BLOCKS    DDA001
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDASTP
C
C
C-----------------------------------------------------------------------
C     dastep solves a system of differential/
C     algebraic equations of the form
C     g(x,y,yprime) = 0,  for one step (normally
C     from x to x+h).
C
C     the methods used are modified divided
C     difference,fixed leading coefficient
C     forms of backward differentiation
C     formulas. the code adjusts the stepsize
C     and order to control the local error per
C     step.
C
C
C     the parameters represent
C     x  --        independent variable
C     y  --        solution vector at x
C     yprime --    derivative of solution vector
C                  after successful step
C     neq --       number of equations to be integrated
C     res --       external user-supplied subroutine
C                  to evaluate the residual.  the call is
C                  call res(x,y,yprime,delta,ires,rpar,ipar)
C                  x,y,yprime are input.  delta is output.
C                  on input, ires=0.  res should alter ires only
C                  if it encounters an illegal value of y or a
C                  stop condition.  set ires=-1 if an input value
C                  of y is illegal, and dastep will try to solve
C                  the problem without getting ires = -1.  if
C                  ires=-2, dastep returns control to the calling
C                  program with idid = -11.
C     jac --       external user-supplied routine to evaluate
C                  the iteration matrix (this is optional)
C                  the call is of the form
C                  call jac(x,y,yprime,pd,cj,rpar,ipar)
C                  pd is the matrix of partial derivatives,
C                  pd=dg/dy+cj*dg/dyprime
C     h --         appropriate step size for next step.
C                  normally determined by the code
C     wt --        vector of weights for error criterion.
C     jstart --    integer variable set 0 for
C                  first step, 1 otherwise.
C     idid --      completion code with the following meanings%
C                  idid= 1 -- the step was completed successfully
C                  idid=-6 -- the error test failed repeatedly
C                  idid=-7 -- the corrector could not converge
C                  idid=-8 -- the iteration matrix is singular
C                  idid=-9 -- the corrector could not converge.
C                             there were repeated error test
C                             failures on this step.
C                  idid=-10-- the corrector could not converge
C                             because ires was equal to minus one
C                  idid=-11-- ires equal to -2 was encountered,
C                             and control is being returned to
C                             the calling program
C     rpar,ipar -- real and integer parameter arrays that
C                  are used for communication between the
C                  calling program and external user routines
C                  they are not altered by dastep
C     phi --       array of divided differences used by
C                  dastep. the length is neq*(k+1),where
C                  k is the maximum order
C     delta,e --   work vectors for dastep of length neq
C     wm,iwm --    real and integer arrays storing
C                  matrix information such as the matrix
C                  of partial derivatives,permutation
C                  vector,and various other information.
C
C     the other parameters are information
C     which is needed internally by dastep to
C     continue from step to step.
C
C-----------------------------------------------------------------------
C_1 R.KOE EINBAU VON PTN IN PARAMETERKAMMERN DDASTP,DDAJAC U.EXT
C
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      LOGICAL CONVGD
      DIMENSION Y(1),YPRIME(1),WT(1)
      DIMENSION PHI(NEQ,1),DELTA(1),E(1)
      DIMENSION WM(1),IWM(1)
      DIMENSION PSI(1),ALPHA(1),BETA(1),GAMMA(1),SIGMA(1)
      DIMENSION RPAR(1),IPAR(1)
      EXTERNAL RES,JAC,PTN
      COMMON/DDA001/NPD,NTEMP,
     *   LML,LMU,LMXORD,LMTYPE,
     *   LNST,LNRE,LNJE,LETF,LCTF,LIPVT
      DATA MAXIT/4/
      DATA XRATE/0.25D0/
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 1.
C     initialize. on the first call,set
C     the order to 1 and initialize
C     other variables.
C-----------------------------------------------------------------------
C
C     initializations for all calls
      IDID=1
      XOLD=X
      NCF=0
      NSF=0
      NEF=0
      IF(JSTART .NE. 0) GO TO 120
C
C     if this is the first step,perform
C     other initializations
C     AUCH BEI CONTINUATION CALLS!!!!!
      IWM(LETF) = 0
      IWM(LCTF) = 0
      K=1
      KOLD=0
      HOLD=0.0D0
      JSTART=1
      PSI(1)=H
      CJOLD = 1.0D0/H
      CJ = CJOLD
      S = 100.D0
      JCALC = -1
      DELNRM=1.0D0
      IPHASE = 0
      NS=0
120   CONTINUE
C
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 2
C     compute coefficients of formulas for
C     this step.
C-----------------------------------------------------------------------
200   CONTINUE
      KP1=K+1
      KP2=K+2
      KM1=K-1
      XOLD=X
      IF(H.NE.HOLD.OR.K .NE. KOLD) NS = 0
      NS=MIN0(NS+1,KOLD+2)
      NSP1=NS+1
      IF(KP1 .LT. NS)GO TO 230
C
      BETA(1)=1.0D0
      ALPHA(1)=1.0D0
      TEMP1=H
      GAMMA(1)=0.0D0
      SIGMA(1)=1.0D0
      DO 210 I=2,KP1
         TEMP2=PSI(I-1)
         PSI(I-1)=TEMP1
         BETA(I)=BETA(I-1)*PSI(I-1)/TEMP2
         TEMP1=TEMP2+H
         ALPHA(I)=H/TEMP1
         SIGMA(I)=DFLOAT(I-1)*SIGMA(I-1)*ALPHA(I)
         GAMMA(I)=GAMMA(I-1)+ALPHA(I-1)/H
210      CONTINUE
      PSI(KP1)=TEMP1
230   CONTINUE
C
C     compute alphas, alpha0
      ALPHAS = 0.0D0
      ALPHA0 = 0.0D0
      DO 240 I = 1,K
        ALPHAS = ALPHAS - 1.0D0/DFLOAT(I)
        ALPHA0 = ALPHA0 - ALPHA(I)
240     CONTINUE
C
C     compute leading coefficient cj
      CJLAST = CJ
      CJ = -ALPHAS/H
C
C     compute variable stepsize error coefficient ck
      CK = DABS(ALPHA(KP1) + ALPHAS - ALPHA0)
      CK = DMAX1(CK,ALPHA(KP1))
C
C     decide whether new jacobian is needed
      TEMP1 = (1.0D0 - XRATE)/(1.0D0 + XRATE)
      TEMP2 = 1.0D0/TEMP1
      IF (CJ/CJOLD .LT. TEMP1 .OR. CJ/CJOLD .GT. TEMP2) JCALC = -1
      IF (CJ .NE. CJLAST) S = 100.D0
C
C     change phi to phi star
      IF(KP1 .LT. NSP1) GO TO 280
      DO 270 J=NSP1,KP1
         DO 260 I=1,NEQ
260         PHI(I,J)=BETA(J)*PHI(I,J)
270      CONTINUE
280   CONTINUE
C
C     update time
      X=X+H
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 3
C     predict the solution and derivative,
C     and solve the corrector equation
C-----------------------------------------------------------------------
C
C     first,predict the solution and derivative
300   CONTINUE
      DO 310 I=1,NEQ
         Y(I)=PHI(I,1)
310      YPRIME(I)=0.0D0
      DO 330 J=2,KP1
         DO 320 I=1,NEQ
            Y(I)=Y(I)+PHI(I,J)
320         YPRIME(I)=YPRIME(I)+GAMMA(J)*PHI(I,J)
330   CONTINUE
      PNORM = DDANRM (NEQ,Y,WT,RPAR,IPAR)
C
C
C
C     solve the corrector equation using a
C     modified newton scheme.
      CONVGD= .TRUE.
      M=0
      IWM(LNRE)=IWM(LNRE)+1
      IRES = 0
      CALL RES(X,Y,YPRIME,DELTA,IRES,RPAR,IPAR)
      IF (IRES .LT. 0) GO TO 380
C
C
C     if indicated,reevaluate the
C     iteration matrix pd = dg/dy + cj*dg/dyprime
C     (where g(x,y,yprime)=0). set
C     jcalc to 0 as an indicator that
C     this has been done.
      IF(JCALC .NE. -1)GO TO 340
      IWM(LNJE)=IWM(LNJE)+1
      JCALC=0
      CALL DDAJAC(NEQ,X,Y,YPRIME,DELTA,CJ,H,
     * IER,WT,E,WM,IWM,RES,IRES,UROUND,JAC,RPAR,IPAR,PTN)
      CJOLD=CJ
      S = 100.D0
      IF (IRES .LT. 0) GO TO 380
      IF(IER .NE. 0)GO TO 380
      NSF=0
C
C
C     initialize the error accumulation vector e.
340   CONTINUE
      DO 345 I=1,NEQ
345      E(I)=0.0D0
C
      S = 100.E0
C
C
C     corrector loop.
350   CONTINUE
C
C     multiply residual by temp1 to accelerate convergence
      TEMP1 = 2.0D0/(1.0D0 + CJ/CJOLD)
      DO 355 I = 1,NEQ
355     DELTA(I) = DELTA(I) * TEMP1
C
C     compute a new iterate (back-substitution).
C     store the correction in delta.
      CALL DDASLV(NEQ,DELTA,WM,IWM)
C
C     update y,e,and yprime
      DO 360 I=1,NEQ
         Y(I)=Y(I)-DELTA(I)
         E(I)=E(I)-DELTA(I)
360      YPRIME(I)=YPRIME(I)-CJ*DELTA(I)
C
C     test for convergence of the iteration
      DELNRM=DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      IF (DELNRM .LE. 100.D0*UROUND*PNORM) GO TO 375
      IF (M .GT. 0) GO TO 365
         OLDNRM = DELNRM
         GO TO 367
365   RATE = (DELNRM/OLDNRM)**(1.0D0/DFLOAT(M))
      IF (RATE .GT. 0.90D0) GO TO 370
      S = RATE/(1.0D0 - RATE)
367   IF (S*DELNRM .LE. 0.33D0) GO TO 375
C
C     the corrector has not yet converged.
C     update m and test whether the
C     maximum number of iterations have
C     been tried.
      M=M+1
      IF(M.GE.MAXIT)GO TO 370
C
C     evaluate the residual
C     and go back to do another iteration
      IWM(LNRE)=IWM(LNRE)+1
      IRES = 0
      CALL RES(X,Y,YPRIME,DELTA,IRES,
     *  RPAR,IPAR)
      IF (IRES .LT. 0) GO TO 380
      GO TO 350
C
C
C     the corrector failed to converge in maxit
C     iterations. if the iteration matrix
C     is not current,re-do the step with
C     a new iteration matrix.
370   CONTINUE
      IF(JCALC.EQ.0)GO TO 380
      JCALC=-1
      GO TO 300
C
C
C     the iteration has converged.  if nonnegativity of solution is
C     required, set the solution nonnegative, if the perturbation
C     to do it is small enough.  if the change is too large, then
C     consider the corrector iteration to have failed.
375   IF(NONNEG .EQ. 0) GO TO 390
      DO 377 I = 1,NEQ
377      DELTA(I) = DMIN1(Y(I),0.0D0)
      DELNRM = DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      IF(DELNRM .GT. 0.33D0) GO TO 380
      DO 378 I = 1,NEQ
378      E(I) = E(I) - DELTA(I)
      GO TO 390
C
C
C     exits from block 3
C     no convergence with current iteration
C     matrix,or singular iteration matrix
380   CONVGD= .FALSE.
390   JCALC = 1
      IF(.NOT.CONVGD)GO TO 600
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 4
C     estimate the errors at orders k,k-1,k-2
C     as if constant stepsize was used. estimate
C     the local error at order k and test
C     whether the current step is successful.
C-----------------------------------------------------------------------
C
C     estimate errors at orders k,k-1,k-2
      ENORM = DDANRM(NEQ,E,WT,RPAR,IPAR)
      ERK = SIGMA(K+1)*ENORM
      TERK = FLOAT(K+1)*ERK
      EST = ERK
      KNEW=K
      IF(K .EQ. 1)GO TO 430
      DO 405 I = 1,NEQ
405     DELTA(I) = PHI(I,KP1) + E(I)
      ERKM1=SIGMA(K)*DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      TERKM1 = FLOAT(K)*ERKM1
      IF(K .GT. 2)GO TO 410
      IF(TERKM1 .LE. 0.5*TERK)GO TO 420
      GO TO 430
410   CONTINUE
      DO 415 I = 1,NEQ
415     DELTA(I) = PHI(I,K) + DELTA(I)
      ERKM2=SIGMA(K-1)*DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      TERKM2 = FLOAT(K-1)*ERKM2
      IF(DMAX1(TERKM1,TERKM2).GT.TERK)GO TO 430
C     lower the order
420   CONTINUE
      KNEW=K-1
      EST = ERKM1
C
C
C     calculate the local error for the current step
C     to see if the step was successful
430   CONTINUE
      ERR = CK * ENORM
      IF(ERR .GT. 1.0D0)GO TO 600
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 5
C     the step is successful. determine
C     the best order and stepsize for
C     the next step. update the differences
C     for the next step.
C-----------------------------------------------------------------------
      IDID=1
      IWM(LNST)=IWM(LNST)+1
      KDIFF=K-KOLD
      KOLD=K
      HOLD=H
C
C
C     estimate the error at order k+1 unless%
C        already decided to lower order, or
C        already using maximum order, or
C        stepsize not constant, or
C        order raised in previous step
      IF(KNEW.EQ.KM1.OR.K.EQ.IWM(LMXORD))IPHASE=1
      IF(IPHASE .EQ. 0)GO TO 545
      IF(KNEW.EQ.KM1)GO TO 540
      IF(K.EQ.IWM(LMXORD)) GO TO 550
      IF(KP1.GE.NS.OR.KDIFF.EQ.1)GO TO 550
      DO 510 I=1,NEQ
510      DELTA(I)=E(I)-PHI(I,KP2)
      ERKP1 = (1.0D0/DFLOAT(K+2))*DDANRM(NEQ,DELTA,WT,RPAR,IPAR)
      TERKP1 = FLOAT(K+2)*ERKP1
      IF(K.GT.1)GO TO 520
      IF(TERKP1.GE.0.5D0*TERK)GO TO 550
      GO TO 530
520   IF(TERKM1.LE.DMIN1(TERK,TERKP1))GO TO 540
      IF(TERKP1.GE.TERK.OR.K.EQ.IWM(LMXORD))GO TO 550
C
C     raise order
530   K=KP1
      EST = ERKP1
      GO TO 550
C
C     lower order
540   K=KM1
      EST = ERKM1
      GO TO 550
C
C     if iphase = 0, increase order by one and multiply stepsize by
C     factor two
545   K = KP1
      HNEW = H*2.0D0
      H = HNEW
      GO TO 575
C
C
C     determine the appropriate stepsize for
C     the next step.
550   HNEW=H
      TEMP2=K+1
      R=(2.0D0*EST+0.0001D0)**(-1.0D0/TEMP2)
      IF(R .LT. 2.0D0) GO TO 555
      HNEW = 2.0D0*H
      GO TO 560
555   IF(R .GT. 1.0D0) GO TO 560
      R = DMAX1(0.5D0,DMIN1(0.9D0,R))
      HNEW = H*R
560   H=HNEW
C
C
C     update differences for next step
575   CONTINUE
      IF(KOLD.EQ.IWM(LMXORD))GO TO 585
      DO 580 I=1,NEQ
580      PHI(I,KP2)=E(I)
585   CONTINUE
      DO 590 I=1,NEQ
590      PHI(I,KP1)=PHI(I,KP1)+E(I)
      DO 595 J1=2,KP1
         J=KP1-J1+1
         DO 595 I=1,NEQ
595      PHI(I,J)=PHI(I,J)+PHI(I,J+1)
      RETURN
C
C
C
C
C
C-----------------------------------------------------------------------
C     block 6
C     the step is unsuccessful. restore x,psi,phi
C     determine appropriate stepsize for
C     continuing the integration, or exit with
C     an error flag if there have been many
C     failures.
C-----------------------------------------------------------------------
600   IPHASE = 1
C
C     restore x,phi,psi
      X=XOLD
      IF(KP1.LT.NSP1)GO TO 630
      DO 620 J=NSP1,KP1
         TEMP1=1.0D0/BETA(J)
         DO 610 I=1,NEQ
610         PHI(I,J)=TEMP1*PHI(I,J)
620      CONTINUE
630   CONTINUE
      DO 640 I=2,KP1
640      PSI(I-1)=PSI(I)-H
C
C
C     test whether failure is due to corrector iteration
C     or error test
      IF(CONVGD)GO TO 660
      IWM(LCTF)=IWM(LCTF)+1
C
C
C     the newton iteration failed to converge with
C     a current iteration matrix.  determine the cause
C     of the failure and take appropriate action.
      IF(IER.EQ.0)GO TO 650
C
C     the iteration matrix is singular. reduce
C     the stepsize by a factor of 4. if
C     this happens three times in a row on
C     the same step, return with an error flag
      NSF=NSF+1
      R = 0.25D0
      H=H*R
      IF (NSF .LT. 3 .AND. DABS(H) .GE. HMIN) GO TO 690
      IDID=-8
      GO TO 675
C
C
C     the newton iteration failed to converge for a reason
C     other than a singular iteration matrix.  if ires = -2, then
C     return.  otherwise, reduce the stepsize and try again, unless
C     too many failures have occured.
650   CONTINUE
      IF (IRES .GT. -2) GO TO 655
      IDID = -11
      GO TO 675
655   NCF = NCF + 1
      R = 0.25D0
      H = H*R
      IF (NCF .LT. 10 .AND. DABS(H) .GE. HMIN) GO TO 690
      IDID = -7
      IF (IRES .LT. 0) IDID = -10
      IF (NEF .GE. 3) IDID = -9
      GO TO 675
C
C
C     the newton scheme converged,and the cause
C     of the failure was the error estimate
C     exceeding the tolerance.
660   NEF=NEF+1
      IWM(LETF)=IWM(LETF)+1
      IF (NEF .GT. 1) GO TO 665
C
C     on first error test failure, keep current order or lower
C     order by one.  compute new stepsize based on differences
C     of the solution.
      K = KNEW
      TEMP2 = K + 1
      R = 0.90D0*(2.0D0*EST+0.0001D0)**(-1.0D0/TEMP2)
      R = DMAX1(0.25D0,DMIN1(0.9D0,R))
      H = H*R
      IF (DABS(H) .GE. HMIN) GO TO 690
      IDID = -6
      GO TO 675
C
C     on second error test failure, use the current order or
C     decrease order by one.  reduce the stepsize by a factor of
C     one quarter.
665   IF (NEF .GT. 2) GO TO 670
      K = KNEW
      H = 0.25D0*H
      IF (DABS(H) .GE. HMIN) GO TO 690
      IDID = -6
      GO TO 675
C
C     on third and subsequent error test failures, set the order to
C     one and reduce the stepsize by a factor of one quarter
670   K = 1
      H = 0.25D0*H
      IF (DABS(H) .GE. HMIN) GO TO 690
      IDID = -6
      GO TO 675
C
C
C
C
C     for all crashes, restore y to its last value,
C     interpolate to find yprime at last x, and return
675   CONTINUE
      CALL DDATRP(X,X,Y,YPRIME,NEQ,K,PHI,PSI)
      RETURN
C
C
C     go back and try this step again
690   GO TO 200
C
C------end of subroutine dastep------
      END
      SUBROUTINE DDATRP(X,XOUT,YOUT,YPOUT,NEQ,KOLD,PHI,PSI)
C
C***BEGIN PROLOGUE  DDATRP
C***REFER TO  DDASSL
C***ROUTINES CALLED  (NONE)
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDATRP
C
C-----------------------------------------------------------------------
C     the methods in subroutine dastep use polynomials
C     to approximate the solution. ddatrp approximates the
C     solution and its derivative at time xout by evaluating
C     one of these polynomials,and its derivative,there.
C     information defining this polynomial is passed from
C     dastep, so ddatrp cannot be used alone.
C
C     the parameters are%
C     x     the current time in the integration.
C     xout  the time at which the solution is desired
C     yout  the interpolated approximation to y at xout
C           (this is output)
C     ypout the interpolated approximation to yprime at xout
C           (this is output)
C     neq   number of equations
C     kold  order used on last successful step
C     phi   array of scaled divided differences of y
C     psi   array of past stepsize history
C-----------------------------------------------------------------------
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION YOUT(1),YPOUT(1)
      DIMENSION PHI(NEQ,1),PSI(1)
      KOLDP1=KOLD+1
      TEMP1=XOUT-X
      DO 10 I=1,NEQ
         YOUT(I)=PHI(I,1)
10       YPOUT(I)=0.0D0
      C=1.0D0
      D=0.0D0
      GAMMA=TEMP1/PSI(1)
      DO 30 J=2,KOLDP1
         D=D*GAMMA+C/PSI(J-1)
         C=C*GAMMA
         GAMMA=(TEMP1+PSI(J-1))/PSI(J)
         DO 20 I=1,NEQ
            YOUT(I)=YOUT(I)+C*PHI(I,J)
20          YPOUT(I)=YPOUT(I)+D*PHI(I,J)
30       CONTINUE
      RETURN
C
C------end of subroutine ddatrp------
      END
      SUBROUTINE DDAWTS(NEQ,IWT,RTOL,ATOL,Y,WT,RPAR,IPAR)
C
C***BEGIN PROLOGUE  DDAWTS
C***REFER TO  DDASSL
C***ROUTINES CALLED  (NONE)
C***DATE WRITTEN   830315   (YYMMDD)
C***REVISION DATE  830315   (YYMMDD)
C***END PROLOGUE  DDAWTS
C-----------------------------------------------------------------------
C     this subroutine sets the error weight vector
C     wt according to wt(i)=rtol(i)*abs(y(i))+atol(i),
C     i=1,-,n.
C     rtol and atol are scalars if iwt = 0,
C     and vectors if iwt = 1.
C-----------------------------------------------------------------------
C
      IMPLICIT DOUBLE PRECISION(A-H,O-Z)
      DIMENSION RTOL(1),ATOL(1),Y(1),WT(1)
      DIMENSION RPAR(1),IPAR(1)
      RTOLI=RTOL(1)
      ATOLI=ATOL(1)
      DO 20 I=1,NEQ
         IF (IWT .EQ.0) GO TO 10
           RTOLI=RTOL(I)
           ATOLI=ATOL(I)
10         WT(I)=RTOLI*DABS(Y(I))+ATOLI
20         CONTINUE
      RETURN
C-----------end of subroutine ddawts-------------------------------------
      END
      DOUBLE PRECISION FUNCTION DDOT(N,DX,INCX,DY,INCY)
C
C     FORMS THE DOT PRODUCT OF TWO VECTORS.
C     USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE.
C     JACK DONGARRA, LINPACK, 3/11/78.
C
      DOUBLE PRECISION DX(1),DY(1),DTEMP
      INTEGER I,INCX,INCY,IX,IY,M,MP1,N
C
      DDOT = 0.0D0
      DTEMP = 0.0D0
      IF(N.LE.0)RETURN
      IF(INCX.EQ.1.AND.INCY.EQ.1)GO TO 20
C
C        CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS
C          NOT EQUAL TO 1
C
      IX = 1
      IY = 1
      IF(INCX.LT.0)IX = (-N+1)*INCX + 1
      IF(INCY.LT.0)IY = (-N+1)*INCY + 1
      DO 10 I = 1,N
        DTEMP = DTEMP + DX(IX)*DY(IY)
        IX = IX + INCX
        IY = IY + INCY
   10 CONTINUE
      DDOT = DTEMP
      RETURN
C
C        CODE FOR BOTH INCREMENTS EQUAL TO 1
C
C
C        CLEAN-UP LOOP
C
   20 M = MOD(N,5)
      IF( M .EQ. 0 ) GO TO 40
      DO 30 I = 1,M
        DTEMP = DTEMP + DX(I)*DY(I)
   30 CONTINUE
      IF( N .LT. 5 ) GO TO 60
   40 MP1 = M + 1
      DO 50 I = MP1,N,5
        DTEMP = DTEMP + DX(I)*DY(I) + DX(I + 1)*DY(I + 1) +
     *   DX(I + 2)*DY(I + 2) + DX(I + 3)*DY(I + 3) + DX(I + 4)*DY(I + 4)
   50 CONTINUE
   60 DDOT = DTEMP
      RETURN
      END
      SUBROUTINE DGBSL(ABD,LDA,N,ML,MU,IPVT,B,JOB)
      INTEGER LDA,N,ML,MU,IPVT(1),JOB
      DOUBLE PRECISION ABD(LDA,1),B(1)
C
C     DGBSL SOLVES THE DOUBLE PRECISION BAND SYSTEM
C     A * X = B  OR  TRANS(A) * X = B
C     USING THE FACTORS COMPUTED BY DGBCO OR DGBFA.
C
C     ON ENTRY
C
C        ABD     DOUBLE PRECISION(LDA, N)
C                THE OUTPUT FROM DGBCO OR DGBFA.
C
C        LDA     INTEGER
C                THE LEADING DIMENSION OF THE ARRAY  ABD .
C
C        N       INTEGER
C                THE ORDER OF THE ORIGINAL MATRIX.
C
C        ML      INTEGER
C                NUMBER OF DIAGONALS BELOW THE MAIN DIAGONAL.
C
C        MU      INTEGER
C                NUMBER OF DIAGONALS ABOVE THE MAIN DIAGONAL.
C
C        IPVT    INTEGER(N)
C                THE PIVOT VECTOR FROM DGBCO OR DGBFA.
C
C        B       DOUBLE PRECISION(N)
C                THE RIGHT HAND SIDE VECTOR.
C
C        JOB     INTEGER
C                = 0         TO SOLVE  A*X = B ,
C                = NONZERO   TO SOLVE  TRANS(A)*X = B , WHERE
C                            TRANS(A)  IS THE TRANSPOSE.
C
C     ON RETURN
C
C        B       THE SOLUTION VECTOR  X .
C
C     ERROR CONDITION
C
C        A DIVISION BY ZERO WILL OCCUR IF THE INPUT FACTOR CONTAINS A
C        ZERO ON THE DIAGONAL.  TECHNICALLY THIS INDICATES SINGULARITY
C        BUT IT IS OFTEN CAUSED BY IMPROPER ARGUMENTS OR IMPROPER
C        SETTING OF LDA .  IT WILL NOT OCCUR IF THE SUBROUTINES ARE
C        CALLED CORRECTLY AND IF DGBCO HAS SET RCOND .GT. 0.0
C        OR DGBFA HAS SET INFO .EQ. 0 .
C
C     TO COMPUTE  INVERSE(A) * C  WHERE  C  IS A MATRIX
C     WITH  P  COLUMNS
C           CALL DGBCO(ABD,LDA,N,ML,MU,IPVT,RCOND,Z)
C           IF (RCOND IS TOO SMALL) GO TO ...
C           DO 10 J = 1, P
C              CALL DGBSL(ABD,LDA,N,ML,MU,IPVT,C(1,J),0)
C        10 CONTINUE
C
C     LINPACK. THIS VERSION DATED 08/14/78 .
C     CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.
C
C     SUBROUTINES AND FUNCTIONS
C
C     BLAS DAXPY,DDOT
C     FORTRAN MIN0
C
C     INTERNAL VARIABLES
C
      DOUBLE PRECISION DDOT,T
      INTEGER K,KB,L,LA,LB,LM,M,NM1
C
      M = MU + ML + 1
      NM1 = N - 1
      IF (JOB .NE. 0) GO TO 50
C
C        JOB = 0 , SOLVE  A * X = B
C        FIRST SOLVE L*Y = B
C
         IF (ML .EQ. 0) GO TO 30
         IF (NM1 .LT. 1) GO TO 30
            DO 20 K = 1, NM1
               LM = MIN0(ML,N-K)
               L = IPVT(K)
               T = B(L)
               IF (L .EQ. K) GO TO 10
                  B(L) = B(K)
                  B(K) = T
   10          CONTINUE
               CALL DAXPY(LM,T,ABD(M+1,K),1,B(K+1),1)
   20       CONTINUE
   30    CONTINUE
C
C        NOW SOLVE  U*X = Y
C
         DO 40 KB = 1, N
            K = N + 1 - KB
            B(K) = B(K)/ABD(M,K)
            LM = MIN0(K,M) - 1
            LA = M - LM
            LB = K - LM
            T = -B(K)
            CALL DAXPY(LM,T,ABD(LA,K),1,B(LB),1)
   40    CONTINUE
      GO TO 100
   50 CONTINUE
C
C        JOB = NONZERO, SOLVE  TRANS(A) * X = B
C        FIRST SOLVE  TRANS(U)*Y = B
C
         DO 60 K = 1, N
            LM = MIN0(K,M) - 1
            LA = M - LM
            LB = K - LM
            T = DDOT(LM,ABD(LA,K),1,B(LB),1)
            B(K) = (B(K) - T)/ABD(M,K)
   60    CONTINUE
C
C        NOW SOLVE TRANS(L)*X = Y
C
         IF (ML .EQ. 0) GO TO 90
         IF (NM1 .LT. 1) GO TO 90
            DO 80 KB = 1, NM1
               K = N - KB
               LM = MIN0(ML,N-K)
               B(K) = B(K) + DDOT(LM,ABD(M+1,K),1,B(K+1),1)
               L = IPVT(K)
               IF (L .EQ. K) GO TO 70
                  T = B(L)
                  B(L) = B(K)
                  B(K) = T
   70          CONTINUE
   80       CONTINUE
   90    CONTINUE
  100 CONTINUE
      RETURN
      END
      SUBROUTINE DGEFA(A,LDA,N,IPVT,INFO)
      INTEGER LDA,N,IPVT(1),INFO
      DOUBLE PRECISION A(LDA,1)
C
C     DGEFA FACTORS A DOUBLE PRECISION MATRIX BY GAUSSIAN ELIMINATION.
C
C     DGEFA IS USUALLY CALLED BY DGECO, BUT IT CAN BE CALLED
C     DIRECTLY WITH A SAVING IN TIME IF  RCOND  IS NOT NEEDED.
C     (TIME FOR DGECO) = (1 + 9/N)*(TIME FOR DGEFA) .
C
C     ON ENTRY
C
C        A       DOUBLE PRECISION(LDA, N)
C                THE MATRIX TO BE FACTORED.
C
C        LDA     INTEGER
C                THE LEADING DIMENSION OF THE ARRAY  A .
C
C        N       INTEGER
C                THE ORDER OF THE MATRIX  A .
C
C     ON RETURN
C
C        A       AN UPPER TRIANGULAR MATRIX AND THE MULTIPLIERS
C                WHICH WERE USED TO OBTAIN IT.
C                THE FACTORIZATION CAN BE WRITTEN  A = L*U  WHERE
C                L  IS A PRODUCT OF PERMUTATION AND UNIT LOWER
C                TRIANGULAR MATRICES AND  U  IS UPPER TRIANGULAR.
C
C        IPVT    INTEGER(N)
C                AN INTEGER VECTOR OF PIVOT INDICES.
C
C        INFO    INTEGER
C                = 0  NORMAL VALUE.
C                = K  IF  U(K,K) .EQ. 0.0 .  THIS IS NOT AN ERROR
C                     CONDITION FOR THIS SUBROUTINE, BUT IT DOES
C                     INDICATE THAT DGESL OR DGEDI WILL DIVIDE BY ZERO
C                     IF CALLED.  USE  RCOND  IN DGECO FOR A RELIABLE
C                     INDICATION OF SINGULARITY.
C
C     LINPACK. THIS VERSION DATED 08/14/78 .
C     CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.
C
C     SUBROUTINES AND FUNCTIONS
C
C     BLAS DAXPY,DSCAL,IDAMAX
C
C     INTERNAL VARIABLES
C
      DOUBLE PRECISION T
      INTEGER IDAMAX,J,K,KP1,L,NM1
C
C
C     GAUSSIAN ELIMINATION WITH PARTIAL PIVOTING
C
      INFO = 0
      NM1 = N - 1
      IF (NM1 .LT. 1) GO TO 70
      DO 60 K = 1, NM1
         KP1 = K + 1
C
C        FIND L = PIVOT INDEX
C
         L = IDAMAX(N-K+1,A(K,K),1) + K - 1
         IPVT(K) = L
C
C        ZERO PIVOT IMPLIES THIS COLUMN ALREADY TRIANGULARIZED
C
         IF (A(L,K) .EQ. 0.0D0) GO TO 40
C
C           INTERCHANGE IF NECESSARY
C
            IF (L .EQ. K) GO TO 10
               T = A(L,K)
               A(L,K) = A(K,K)
               A(K,K) = T
   10       CONTINUE
C
C           COMPUTE MULTIPLIERS
C
            T = -1.0D0/A(K,K)
            CALL DSCAL(N-K,T,A(K+1,K),1)
C
C           ROW ELIMINATION WITH COLUMN INDEXING
C
            DO 30 J = KP1, N
               T = A(L,J)
               IF (L .EQ. K) GO TO 20
                  A(L,J) = A(K,J)
                  A(K,J) = T
   20          CONTINUE
               CALL DAXPY(N-K,T,A(K+1,K),1,A(K+1,J),1)
   30       CONTINUE
         GO TO 50
   40    CONTINUE
            INFO = K
   50    CONTINUE
   60 CONTINUE
   70 CONTINUE
      IPVT(N) = N
      IF (A(N,N) .EQ. 0.0D0) INFO = N
      RETURN
      END
      SUBROUTINE DGESL(A,LDA,N,IPVT,B,JOB)
      INTEGER LDA,N,IPVT(1),JOB
      DOUBLE PRECISION A(LDA,1),B(1)
C
C     DGESL SOLVES THE DOUBLE PRECISION SYSTEM
C     A * X = B  OR  TRANS(A) * X = B
C     USING THE FACTORS COMPUTED BY DGECO OR DGEFA.
C
C     ON ENTRY
C
C        A       DOUBLE PRECISION(LDA, N)
C                THE OUTPUT FROM DGECO OR DGEFA.
C
C        LDA     INTEGER
C                THE LEADING DIMENSION OF THE ARRAY  A .
C
C        N       INTEGER
C                THE ORDER OF THE MATRIX  A .
C
C        IPVT    INTEGER(N)
C                THE PIVOT VECTOR FROM DGECO OR DGEFA.
C
C        B       DOUBLE PRECISION(N)
C                THE RIGHT HAND SIDE VECTOR.
C
C        JOB     INTEGER
C                = 0         TO SOLVE  A*X = B ,
C                = NONZERO   TO SOLVE  TRANS(A)*X = B  WHERE
C                            TRANS(A)  IS THE TRANSPOSE.
C
C     ON RETURN
C
C        B       THE SOLUTION VECTOR  X .
C
C     ERROR CONDITION
C
C        A DIVISION BY ZERO WILL OCCUR IF THE INPUT FACTOR CONTAINS A
C        ZERO ON THE DIAGONAL.  TECHNICALLY THIS INDICATES SINGULARITY
C        BUT IT IS OFTEN CAUSED BY IMPROPER ARGUMENTS OR IMPROPER
C        SETTING OF LDA .  IT WILL NOT OCCUR IF THE SUBROUTINES ARE
C        CALLED CORRECTLY AND IF DGECO HAS SET RCOND .GT. 0.0
C        OR DGEFA HAS SET INFO .EQ. 0 .
C
C     TO COMPUTE  INVERSE(A) * C  WHERE  C  IS A MATRIX
C     WITH  P  COLUMNS
C           CALL DGECO(A,LDA,N,IPVT,RCOND,Z)
C           IF (RCOND IS TOO SMALL) GO TO ...
C           DO 10 J = 1, P
C              CALL DGESL(A,LDA,N,IPVT,C(1,J),0)
C        10 CONTINUE
C
C     LINPACK. THIS VERSION DATED 08/14/78 .
C     CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.
C
C     SUBROUTINES AND FUNCTIONS
C
C     BLAS DAXPY,DDOT
C
C     INTERNAL VARIABLES
C
      DOUBLE PRECISION DDOT,T
      INTEGER K,KB,L,NM1
C
      NM1 = N - 1
      IF (JOB .NE. 0) GO TO 50
C
C        JOB = 0 , SOLVE  A * X = B
C        FIRST SOLVE  L*Y = B
C
         IF (NM1 .LT. 1) GO TO 30
         DO 20 K = 1, NM1
            L = IPVT(K)
            T = B(L)
            IF (L .EQ. K) GO TO 10
               B(L) = B(K)
               B(K) = T
   10       CONTINUE
            CALL DAXPY(N-K,T,A(K+1,K),1,B(K+1),1)
   20    CONTINUE
   30    CONTINUE
C
C        NOW SOLVE  U*X = Y
C
         DO 40 KB = 1, N
            K = N + 1 - KB
            B(K) = B(K)/A(K,K)
            T = -B(K)
            CALL DAXPY(K-1,T,A(1,K),1,B(1),1)
   40    CONTINUE
      GO TO 100
   50 CONTINUE
C
C        JOB = NONZERO, SOLVE  TRANS(A) * X = B
C        FIRST SOLVE  TRANS(U)*Y = B
C
         DO 60 K = 1, N
            T = DDOT(K-1,A(1,K),1,B(1),1)
            B(K) = (B(K) - T)/A(K,K)
   60    CONTINUE
C
C        NOW SOLVE TRANS(L)*X = Y
C
         IF (NM1 .LT. 1) GO TO 90
         DO 80 KB = 1, NM1
            K = N - KB
            B(K) = B(K) + DDOT(N-K,A(K+1,K),1,B(K+1),1)
            L = IPVT(K)
            IF (L .EQ. K) GO TO 70
               T = B(L)
               B(L) = B(K)
               B(K) = T
   70       CONTINUE
   80    CONTINUE
   90    CONTINUE
  100 CONTINUE
      RETURN
      END
      SUBROUTINE DSCAL(N,DA,DX,INCX)
C
C     SCALES A VECTOR BY A CONSTANT.
C     USES UNROLLED LOOPS FOR INCREMENT EQUAL TO ONE.
C     JACK DONGARRA, LINPACK, 3/11/78.
C
      DOUBLE PRECISION DA,DX(1)
      INTEGER I,INCX,M,MP1,N,NINCX
C
      IF(N.LE.0)RETURN
      IF(INCX.EQ.1)GO TO 20
C
C        CODE FOR INCREMENT NOT EQUAL TO 1
C
      NINCX = N*INCX
      DO 10 I = 1,NINCX,INCX
        DX(I) = DA*DX(I)
   10 CONTINUE
      RETURN
C
C        CODE FOR INCREMENT EQUAL TO 1
C
C
C        CLEAN-UP LOOP
C
   20 M = MOD(N,5)
      IF( M .EQ. 0 ) GO TO 40
      DO 30 I = 1,M
        DX(I) = DA*DX(I)
   30 CONTINUE
      IF( N .LT. 5 ) RETURN
   40 MP1 = M + 1
      DO 50 I = MP1,N,5
        DX(I) = DA*DX(I)
        DX(I + 1) = DA*DX(I + 1)
        DX(I + 2) = DA*DX(I + 2)
        DX(I + 3) = DA*DX(I + 3)
        DX(I + 4) = DA*DX(I + 4)
   50 CONTINUE
      RETURN
      END
      INTEGER FUNCTION IDAMAX(N,DX,INCX)
C
C     FINDS THE INDEX OF ELEMENT HAVING MAX. ABSOLUTE VALUE.
C     JACK DONGARRA, LINPACK, 3/11/78.
C
      DOUBLE PRECISION DX(1),DMAX
      INTEGER I,INCX,IX,N
C
      IDAMAX = 0
      IF( N .LT. 1 ) RETURN
      IDAMAX = 1
      IF(N.EQ.1)RETURN
      IF(INCX.EQ.1)GO TO 20
C
C        CODE FOR INCREMENT NOT EQUAL TO 1
C
      IX = 1
      DMAX = DABS(DX(1))
      IX = IX + INCX
      DO 10 I = 2,N
         IF(DABS(DX(IX)).LE.DMAX) GO TO 5
         IDAMAX = I
         DMAX = DABS(DX(IX))
    5    IX = IX + INCX
   10 CONTINUE
      RETURN
C
C        CODE FOR INCREMENT EQUAL TO 1
C
   20 DMAX = DABS(DX(1))
      DO 30 I = 2,N
         IF(DABS(DX(I)).LE.DMAX) GO TO 30
         IDAMAX = I
         DMAX = DABS(DX(I))
   30 CONTINUE
      RETURN
      END
      SUBROUTINE MA30AD(NN, ICN, A, LICN, LENR, LENRL, IDISP, IP, IQ,           
     * IRN, LIRN, LENC, IFIRST, LASTR, NEXTR, LASTC, NEXTC, IPTR, IPC,  0000000 
     * U, IFLAG)                                                        0000000 
c_270390
      EXTERNAL MA30$DATA
c_270390
C IF  THE USER REQUIRES A MORE CONVENIENT DATA INTERFACE THEN THE MA28  0000000 
C     PACKAGE SHOULD BE USED.  THE MA28 SUBROUTINES CALL THE MA30       0000000 
C     SUBROUTINES AFTER CHECKING THE USER'S INPUT DATA AND OPTIONALLY   0000000 
C     USING MC23A/AD TO PERMUTE THE MATRIX TO BLOCK TRIANGULAR FORM.    0000000 
C THIS PACKAGE OF SUBROUTINES (MA30A/AD, MA30B/BD, MA30C/CD AND         0000000 
C     MA30D/DD) PERFORMS OPERATIONS PERTINENT TO THE SOLUTION OF A      0000000 
C     GENERAL SPARSE N BY N SYSTEM OF LINEAR EQUATIONS (I.E. SOLVE      0000000 
C     AX=B). STRUCTUALLY SINGULAR MATRICES ARE PERMITTED INCLUDING      0000000 
C     THOSE WITH ROW OR COLUMNS CONSISTING ENTIRELY OF ZEROS (I.E.      0000000 
C     INCLUDING RECTANGULAR MATRICES).  IT IS ASSUMED THAT THE          0000000 
C     NON-ZEROS OF THE MATRIX A DO NOT DIFFER WIDELY IN SIZE.  IF       0000000 
C     NECESSARY A PRIOR CALL OF THE SCALING SUBROUTINE MC19A/AD MAY BE  0000000 
C     MADE.                                                             0000000 
C A DISCUSSION OF THE DESIGN OF THESE SUBROUTINES IS GIVEN BY DUFF AND  0000000 
C     REID (ACM TRANS MATH SOFTWARE 5 PP 18-35,1979 (CSS 48)) WHILE     0000000 
C     FULLER DETAILS OF THE IMPLEMENTATION ARE GIVEN IN DUFF (HARWELL   0000000 
C     REPORT AERE-R 8730,1977).  THE ADDITIONAL PIVOTING OPTION IN      0000000 
C     MA30A/AD AND THE USE OF DROP TOLERANCES (SEE COMMON BLOCK         0000000 
C     MA30I/ID) WERE ADDED TO THE PACKAGE AFTER JOINT WORK WITH REID,   0000000 
C     SCHAUMBURG, WASNIEWSKI AND ZLATEV (DUFF, REID, SCHAUMBURG,        0000000 
C     WASNIEWSKI AND ZLATEV, HARWELL REPORT CSS 135, 1983).             0000000 
C                                                                       0000000 
C MA30A/AD PERFORMS THE LU DECOMPOSITION OF THE DIAGONAL BLOCKS OF THE  0000000 
C     PERMUTATION PAQ OF A SPARSE MATRIX A, WHERE INPUT PERMUTATIONS    0000000 
C     P1 AND Q1 ARE USED TO DEFINE THE DIAGONAL BLOCKS.  THERE MAY BE   0000000 
C     NON-ZEROS IN THE OFF-DIAGONAL BLOCKS BUT THEY ARE UNAFFECTED BY   0000000 
C     MA30A/AD. P AND P1 DIFFER ONLY WITHIN BLOCKS AS DO Q AND Q1. THE  0000000 
C     PERMUTATIONS P1 AND Q1 MAY BE FOUND BY CALLING MC23A/AD OR THE    0000000 
C     MATRIX MAY BE TREATED AS A SINGLE BLOCK BY USING P1=Q1=I. THE     0000000 
C     MATRIX NON-ZEROS SHOULD BE HELD COMPACTLY BY ROWS, ALTHOUGH IT    0000000 
C     SHOULD BE NOTED THAT THE USER CAN SUPPLY THE MATRIX BY COLUMNS    0000000 
C     TO GET THE LU DECOMPOSITION OF A TRANSPOSE.                       0000000 
C                                                                       0000000 
C THE PARAMETERS ARE...                                                 0000000 
C THIS DESCRIPTION SHOULD ALSO BE CONSULTED FOR FURTHER INFORMATION ON  0000000 
C     MOST OF THE PARAMETERS OF MA30B/BD AND MA30C/CD.                  0000000 
C                                                                       0000000 
C N  IS AN INTEGER VARIABLE WHICH MUST BE SET BY THE USER TO THE                
C     ORDER OF THE MATRIX. BECAUSE OF THE USE OF INTEGER*2 ARRAYS IN            
C     THE IBM VERSION, THE VALUE OF N SHOULD BE LESS THAN 32768.  IT            
C     IS NOT ALTERED BY MA30A/AD.                                               
C ICN IS AN INTEGER*2  ARRAY OF LENGTH LICN. POSITIONS IDISP(2) TO              
C     LICN MUST BE SET BY THE USER TO CONTAIN THE COLUMN INDICES OF     0000000 
C     THE NON-ZEROS IN THE DIAGONAL BLOCKS OF P1*A*Q1. THOSE BELONGING  0000000 
C     TO A SINGLE ROW MUST BE CONTIGUOUS BUT THE ORDERING OF COLUMN     0000000 
C     INDICES WITH EACH ROW IS UNIMPORTANT. THE NON-ZEROS OF ROW I      0000000 
C     PRECEDE THOSE OF ROW I+1,I=1,...,N-1 AND NO WASTED SPACE IS       0000000 
C     ALLOWED BETWEEN THE ROWS.  ON OUTPUT THE COLUMN INDICES OF THE    0000000 
C     LU DECOMPOSITION OF PAQ ARE HELD IN POSITIONS IDISP(1) TO         0000000 
C     IDISP(2), THE ROWS ARE IN PIVOTAL ORDER, AND THE COLUMN INDICES   0000000 
C     OF THE L PART OF EACH ROW ARE IN PIVOTAL ORDER AND PRECEDE THOSE  0000000 
C     OF U. AGAIN THERE IS NO WASTED SPACE EITHER WITHIN A ROW OR       0000000 
C     BETWEEN THE ROWS. ICN(1) TO ICN(IDISP(1)-1), ARE NEITHER          0000000 
C     REQUIRED NOR ALTERED. IF MC23A/AD BEEN CALLED, THESE WILL HOLD    0000000 
C     INFORMATION ABOUT THE OFF-DIAGONAL BLOCKS.                        0000000 
C A IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH LICN WHOSE ENTRIES       0000000 
C     IDISP(2) TO LICN MUST BE SET BY THE USER TO THE  VALUES OF THE    0000000 
C     NON-ZERO ENTRIES OF THE MATRIX IN THE ORDER INDICATED BY  ICN.    0000000 
C     ON OUTPUT A WILL HOLD THE LU FACTORS OF THE MATRIX WHERE AGAIN    0000000 
C     THE POSITION IN THE MATRIX IS DETERMINED BY THE CORRESPONDING     0000000 
C     VALUES IN ICN. A(1) TO A(IDISP(1)-1) ARE NEITHER REQUIRED NOR     0000000 
C     ALTERED.                                                          0000000 
C LICN  IS AN INTEGER VARIABLE WHICH MUST BE SET BY THE USER TO THE     0000000 
C     LENGTH OF ARRAYS ICN AND A. IT MUST BE BIG ENOUGH FOR A AND ICN   0000000 
C     TO HOLD ALL THE NON-ZEROS OF L AND U AND LEAVE SOME "ELBOW        0000000 
C     ROOM".  IT IS POSSIBLE TO CALCULATE A MINIMUM VALUE FOR LICN BY   0000000 
C     A PRELIMINARY RUN OF MA30A/AD. THE ADEQUACY OF THE ELBOW ROOM     0000000 
C     CAN BE JUDGED BY THE SIZE OF THE COMMON BLOCK VARIABLE ICNCP. IT  0000000 
C     IS NOT ALTERED BY MA30A/AD.                                       0000000 
C LENR  IS AN INTEGER*2  ARRAY OF LENGTH N.  ON INPUT, LENR(I) SHOULD           
C     EQUAL THE NUMBER OF NON-ZEROS IN ROW I, I=1,...,N OF THE          0000000 
C     DIAGONAL BLOCKS OF P1*A*Q1. ON OUTPUT, LENR(I) WILL EQUAL THE     0000000 
C     TOTAL NUMBER OF NON-ZEROS IN ROW I OF L AND ROW I OF U.           0000000 
C LENRL  IS AN INTEGER*2 ARRAY OF LENGTH N. ON OUTPUT FROM MA30A/AD,            
C     LENRL(I) WILL HOLD THE NUMBER OF NON-ZEROS IN ROW I OF L.         0000000 
C IDISP  IS AN INTEGER ARRAY OF LENGTH 2. THE USER SHOULD SET IDISP(1)  0000000 
C     TO BE THE FIRST AVAILABLE POSITION IN A/ICN FOR THE LU            0000000 
C     DECOMPOSITION WHILE IDISP(2) IS SET TO THE POSITION IN A/ICN OF   0000000 
C     THE FIRST NON-ZERO IN THE DIAGONAL BLOCKS OF P1*A*Q1. ON OUTPUT,  0000000 
C     IDISP(1) WILL BE UNALTERED WHILE IDISP(2) WILL BE SET TO THE      0000000 
C     POSITION IN A/ICN OF THE LAST NON-ZERO OF THE LU DECOMPOSITION.   0000000 
C IP  IS AN INTEGER*2  ARRAY OF LENGTH N WHICH HOLDS A PERMUTATION OF           
C     THE INTEGERS 1 TO N.  ON INPUT TO MA30A/AD, THE ABSOLUTE VALUE OF 0000000 
C     IP(I) MUST BE SET TO THE ROW OF A WHICH IS ROW I OF P1*A*Q1. A    0000000 
C     NEGATIVE VALUE FOR IP(I) INDICATES THAT ROW I IS AT THE END OF A  0000000 
C     DIAGONAL BLOCK.  ON OUTPUT FROM MA30A/AD, IP(I) INDICATES THE ROW 0000000 
C     OF A WHICH IS THE I TH ROW IN PAQ. IP(I) WILL STILL BE NEGATIVE   0000000 
C     FOR THE LAST ROW OF EACH BLOCK (EXCEPT THE LAST).                 0000000 
C IQ IS AN INTEGER*2  ARRAY OF LENGTH N WHICH AGAIN HOLDS A                     
C     PERMUTATION OF THE INTEGERS 1 TO N.  ON INPUT TO MA30A/AD, IQ(J)  0000000 
C     MUST BE SET TO THE COLUMN OF A WHICH IS COLUMN J OF P1*A*Q1. ON   0000000 
C     OUTPUT FROM MA30A/AD, THE ABSOLUTE VALUE OF IQ(J) INDICATES THE   0000000 
C     COLUMN OF A WHICH IS THE J TH IN PAQ.  FOR ROWS, I SAY, IN WHICH  0000000 
C     STRUCTURAL OR NUMERICAL SINGULARITY IS DETECTED IQ(I) IS          0000000 
C     NEGATED.                                                          0000000 
C IRN  IS AN INTEGER*2  ARRAY OF LENGTH LIRN USED AS WORKSPACE BY               
C     MA30A/AD.                                                         0000000 
C LIRN  IS AN INTEGER VARIABLE. IT SHOULD BE GREATER THAN THE           0000000 
C     LARGEST NUMBER OF NON-ZEROS IN A DIAGONAL BLOCK OF P1*A*Q1 BUT    0000000 
C     NEED NOT BE AS LARGE AS LICN. IT IS THE LENGTH OF ARRAY IRN AND   0000000 
C     SHOULD BE LARGE ENOUGH TO HOLD THE ACTIVE PART OF ANY BLOCK,      0000000 
C     PLUS SOME "ELBOW ROOM", THE  A POSTERIORI  ADEQUACY OF WHICH CAN  0000000 
C     BE ESTIMATED BY EXAMINING THE SIZE OF COMMON BLOCK VARIABLE       0000000 
C     IRNCP.                                                            0000000 
C LENC,IFIRST,LASTR,NEXTR,LASTC,NEXTC ARE ALL INTEGER*2  ARRAYS OF              
C     LENGTH N WHICH ARE USED AS WORKSPACE BY MA30A/AD.  IF NSRCH IS    0000000 
C     SET TO A VALUE LESS THAN OR EQUAL TO N, THEN ARRAYS LASTC AND     0000000 
C     NEXTC ARE NOT REFERENCED BY MA30A/AD AND SO CAN BE DUMMIED IN     0000000 
C     THE CALL TO MA30A/AD.                                             0000000 
C IPTR,IPC ARE INTEGER ARRAYS OF LENGTH N WHICH ARE USED AS WORKSPACE   0000000 
C     BY MA30A/AD.                                                      0000000 
C U  IS A REAL/DOUBLE PRECISION VARIABLE WHICH SHOULD BE SET BY THE     0000000 
C     USER TO A VALUE BETWEEN 0. AND 1.0. IF LESS THAN ZERO IT IS       0000000 
C     RESET TO ZERO AND IF ITS VALUE IS 1.0 OR GREATER IT IS RESET TO   0000000 
C     0.9999 (0.999999999 IN D VERSION).  IT DETERMINES THE BALANCE     0000000 
C     BETWEEN PIVOTING FOR SPARSITY AND FOR STABILITY, VALUES NEAR      0000000 
C     ZERO EMPHASIZING SPARSITY AND VALUES NEAR ONE EMPHASIZING         0000000 
C     STABILITY. WE RECOMMEND U=0.1 AS A POSIBLE FIRST TRIAL VALUE.     0000000 
C     THE STABILITY CAN BE JUDGED BY A LATER CALL TO MC24A/AD OR BY     0000000 
C     SETTING LBIG TO .TRUE.                                            0000000 
C IFLAG  IS AN INTEGER VARIABLE. IT WILL HAVE A NON-NEGATIVE VALUE IF   0000000 
C     MA30A/AD IS SUCCESSFUL. NEGATIVE VALUES INDICATE ERROR            0000000 
C     CONDITIONS WHILE POSITIVE VALUES INDICATE THAT THE MATRIX HAS     0000000 
C     BEEN SUCCESSFULLY DECOMPOSED BUT IS SINGULAR. FOR EACH NON-ZERO   0000000 
C     VALUE, AN APPROPRIATE MESSAGE IS OUTPUT ON UNIT LP.  POSSIBLE     0000000 
C     NON-ZERO VALUES FOR IFLAG ARE ...                                 0000000 
C                                                                       0000000 
C -1  THE MATRIX IS STRUCTUALLY SINGULAR WITH RANK GIVEN BY IRANK IN    0000000 
C     COMMON BLOCK MA30F/FD.                                            0000000 
C +1  IF, HOWEVER, THE USER WANTS THE LU DECOMPOSITION OF A             0000000 
C     STRUCTURALLY SINGULAR MATRIX AND SETS THE COMMON BLOCK VARIABLE   0000000 
C     ABORT1 TO .FALSE., THEN, IN THE EVENT OF SINGULARITY AND A        0000000 
C     SUCCESSFUL DECOMPOSITION, IFLAG IS RETURNED WITH THE VALUE +1     0000000 
C     AND NO MESSAGE IS OUTPUT.                                         0000000 
C -2  THE MATRIX IS NUMERICALLY SINGULAR (IT MAY ALSO BE STRUCTUALLY    0000000 
C     SINGULAR) WITH ESTIMATED RANK GIVEN BY IRANK IN COMMON BLOCK      0000000 
C     MA30F/FD.                                                         0000000 
C +2  THE  USER CAN CHOOSE TO CONTINUE THE DECOMPOSITION EVEN WHEN A    0000000 
C     ZERO PIVOT IS ENCOUNTERED BY SETTING COMMON BLOCK VARIABLE        0000000 
C     ABORT2 TO .FALSE.  IF A SINGULARITY IS ENCOUNTERED, IFLAG WILL    0000000 
C     THEN RETURN WITH A VALUE OF +2, AND NO MESSAGE IS OUTPUT IF THE   0000000 
C     DECOMPOSITION HAS BEEN COMPLETED SUCCESSFULLY.                    0000000 
C -3  LIRN HAS NOT BEEN LARGE ENOUGH TO CONTINUE WITH THE               0000000 
C     DECOMPOSITION.  IF THE STAGE WAS ZERO THEN COMMON BLOCK VARIABLE  0000000 
C     MINIRN GIVES THE LENGTH SUFFICIENT TO START THE DECOMPOSITION ON  0000000 
C     THIS BLOCK.  FOR A SUCCESSFUL DECOMPOSITION ON THIS BLOCK THE     0000000 
C     USER SHOULD MAKE LIRN SLIGHTLY (SAY ABOUT N/2) GREATER THAN THIS  0000000 
C     VALUE.                                                            0000000 
C -4  LICN NOT LARGE ENOUGH TO CONTINUE WITH THE DECOMPOSITION.         0000000 
C -5  THE DECOMPOSITION HAS BEEN COMPLETED BUT SOME OF THE LU FACTORS   0000000 
C     HAVE BEEN DISCARDED TO CREATE ENOUGH ROOM IN A/ICN TO CONTINUE    0000000 
C     THE DECOMPOSITION. THE VARIABLE MINICN IN COMMON BLOCK MA30F/FD   0000000 
C     THEN GIVES THE SIZE THAT LICN SHOULD BE TO ENABLE THE             0000000 
C     FACTORIZATION TO BE SUCCESSFUL.  IF THE USER SETS COMMON BLOCK    0000000 
C     VARIABLE ABORT3 TO .TRUE., THEN THE SUBROUTINE WILL EXIT          0000000 
C     IMMEDIATELY INSTEAD OF DESTROYING ANY FACTORS AND CONTINUING.     0000000 
C -6  BOTH LICN AND LIRN ARE TOO SMALL. TERMINATION HAS BEEN CAUSED BY  0000000 
C     LACK OF SPACE IN IRN (SEE ERROR IFLAG= -3), BUT ALREADY SOME OF   0000000 
C     THE LU FACTORS IN A/ICN HAVE BEEN LOST (SEE ERROR IFLAG= -5).     0000000 
C     MINICN GIVES THE MINIMUM AMOUNT OF SPACE REQUIRED IN A/ICN FOR    0000000 
C     DECOMPOSITION UP TO THIS POINT.                                   0000000 
C                                                                       0000000 
      DOUBLE PRECISION A(LICN), U, AU, UMAX, AMAX, ZERO, PIVRAT, PIVR,          
     * TOL, BIG, ANEW, AANEW, SCALE                                             
      INTEGER IPTR(NN), PIVOT, PIVEND, DISPC, OLDPIV, OLDEND, PIVROW,   0000000 
     * ROWI, IPC(NN), IDISP(2)                                          0000000 
Change
C      INTEGER*2 ICN(LICN), LENR(NN), LENRL(NN), IP(NN), IQ(NN),                 
C     * LENC(NN), IRN(LIRN), IFIRST(NN), LASTR(NN), NEXTR(NN),           0000000 
C     * LASTC(NN), NEXTC(NN)                                             0000000 
      INTEGER  ICN(LICN), LENR(NN), LENRL(NN), IP(NN), IQ(NN),                 
     * LENC(NN), IRN(LIRN), IFIRST(NN), LASTR(NN), NEXTR(NN),           0000000 
     * LASTC(NN), NEXTC(NN)                                             0000000 
Change
      LOGICAL ABORT1, ABORT2, ABORT3, LBIG                              0000000 
C FOR COMMENTS OF COMMON BLOCK VARIABLES SEE BLOCK DATA SUBPROGRAM.             
      COMMON /MA30ED/ LP, ABORT1, ABORT2, ABORT3                                
      COMMON /MA30FD/ IRNCP, ICNCP, IRANK, MINIRN, MINICN                       
      COMMON /MA30ID/ TOL, BIG, NDROP, NSRCH, LBIG                              
C                                                                       0000000 
      DATA UMAX/.999999999D0/                                                   
      DATA ZERO /0.0D0/                                                         
      MSRCH = NSRCH                                                     0000000 
      NDROP = 0                                                         0000000 
      MINIRN = 0                                                        0000000 
      MINICN = IDISP(1) - 1                                             0000000 
      MOREI = 0                                                         0000000 
      IRANK = NN                                                        0000000 
      IRNCP = 0                                                         0000000 
      ICNCP = 0                                                         0000000 
      IFLAG = 0                                                         0000000 
C RESET U IF NECESSARY.                                                 0000000 
      U = DMIN1(U,UMAX)                                                         
C IBEG IS THE POSITION OF THE NEXT PIVOT ROW AFTER ELIMINATION STEP     0000000 
C     USING IT.                                                         0000000 
      U = DMAX1(U,ZERO)                                                         
      IBEG = IDISP(1)                                                   0000000 
C IACTIV IS THE POSITION OF THE FIRST ENTRY IN THE ACTIVE PART OF A/ICN.0000000 
      IACTIV = IDISP(2)                                                 0000000 
C NZROW IS CURRENT NUMBER OF NON-ZEROS IN ACTIVE AND UNPROCESSED PART   0000000 
C     OF ROW FILE ICN.                                                  0000000 
      NZROW = LICN - IACTIV + 1                                         0000000 
      MINICN = NZROW + MINICN                                           0000000 
C                                                                       0000000 
C COUNT THE NUMBER OF DIAGONAL BLOCKS AND SET UP POINTERS TO THE        0000000 
C     BEGINNINGS OF THE ROWS.                                           0000000 
C NUM IS THE NUMBER OF DIAGONAL BLOCKS.                                 0000000 
      NUM = 1                                                           0000000 
      IPTR(1) = IACTIV                                                  0000000 
      IF (NN.EQ.1) GO TO 20                                             0000000 
      NNM1 = NN - 1                                                     0000000 
      DO 10 I=1,NNM1                                                    0000000 
        IF (IP(I).LT.0) NUM = NUM + 1                                   0000000 
        IPTR(I+1) = IPTR(I) + LENR(I)                                   0000000 
   10 CONTINUE                                                          0000000 
C ILAST IS THE LAST ROW IN THE PREVIOUS BLOCK.                          0000000 
   20 ILAST = 0                                                         0000000 
C                                                                       0000000 
C ***********************************************                       0000000 
C ****    LU DECOMPOSITION OF BLOCK NBLOCK   ****                       0000000 
C ***********************************************                       0000000 
C                                                                       0000000 
C EACH PASS THROUGH THIS LOOP PERFORMS LU DECOMPOSITION ON ONE          0000000 
C     OF THE DIAGONAL BLOCKS.                                           0000000 
      DO 1000 NBLOCK=1,NUM                                              0000000 
        ISTART = ILAST + 1                                              0000000 
        DO 30 IROWS=ISTART,NN                                           0000000 
          IF (IP(IROWS).LT.0) GO TO 40                                  0000000 
   30   CONTINUE                                                        0000000 
        IROWS = NN                                                      0000000 
   40   ILAST = IROWS                                                   0000000 
C N IS THE NUMBER OF ROWS IN THE CURRENT BLOCK.                         0000000 
C ISTART IS THE INDEX OF THE FIRST ROW IN THE CURRENT BLOCK.            0000000 
C ILAST IS THE INDEX OF THE LAST ROW IN THE CURRENT BLOCK.              0000000 
C IACTIV IS THE POSITION OF THE FIRST ENTRY IN THE BLOCK.               0000000 
C ITOP IS THE POSITION OF THE LAST ENTRY IN THE BLOCK.                  0000000 
        N = ILAST - ISTART + 1                                          0000000 
        IF (N.NE.1) GO TO 90                                            0000000 
C                                                                       0000000 
C CODE FOR DEALING WITH 1X1 BLOCK.                                      0000000 
        LENRL(ILAST) = 0                                                0000000 
        ISING = ISTART                                                  0000000 
        IF (LENR(ILAST).NE.0) GO TO 50                                  0000000 
C BLOCK IS STRUCTURALLY SINGULAR.                                       0000000 
        IRANK = IRANK - 1                                               0000000 
        ISING = -ISING                                                  0000000 
        IF (IFLAG.NE.2 .AND. IFLAG.NE.-5) IFLAG = 1                     0000000 
        IF (.NOT.ABORT1) GO TO 80                                       0000000 
        IDISP(2) = IACTIV                                               0000000 
        IFLAG = -1                                                      0000000 
        IF (LP.NE.0) WRITE (LP,99999)                                   0000000 
C     RETURN                                                            0000000 
        GO TO 1120                                                      0000000 
   50   SCALE = DABS(A(IACTIV))                                                 
        IF (SCALE.EQ.ZERO) GO TO 60                                     0000000 
        IF (LBIG) BIG = DMAX1(BIG,SCALE)                                        
        GO TO 70                                                        0000000 
   60   ISING = -ISING                                                  0000000 
        IRANK = IRANK - 1                                               0000000 
        IPTR(ILAST) = 0                                                 0000000 
        IF (IFLAG.NE.-5) IFLAG = 2                                      0000000 
        IF (.NOT.ABORT2) GO TO 70                                       0000000 
        IDISP(2) = IACTIV                                               0000000 
        IFLAG = -2                                                      0000000 
        IF (LP.NE.0) WRITE (LP,99998)                                   0000000 
        GO TO 1120                                                      0000000 
   70   A(IBEG) = A(IACTIV)                                             0000000 
        ICN(IBEG) = ICN(IACTIV)                                         0000000 
        IACTIV = IACTIV + 1                                             0000000 
        IPTR(ISTART) = 0                                                0000000 
        IBEG = IBEG + 1                                                 0000000 
        NZROW = NZROW - 1                                               0000000 
   80   LASTR(ISTART) = ISTART                                          0000000 
        IPC(ISTART) = -ISING                                            0000000 
        GO TO 1000                                                      0000000 
C                                                                       0000000 
C NON-TRIVIAL BLOCK.                                                    0000000 
   90   ITOP = LICN                                                     0000000 
        IF (ILAST.NE.NN) ITOP = IPTR(ILAST+1) - 1                       0000000 
C                                                                       0000000 
C SET UP COLUMN ORIENTED STORAGE.                                       0000000 
        DO 100 I=ISTART,ILAST                                           0000000 
          LENRL(I) = 0                                                  0000000 
          LENC(I) = 0                                                   0000000 
  100   CONTINUE                                                        0000000 
        IF (ITOP-IACTIV.LT.LIRN) GO TO 110                              0000000 
        MINIRN = ITOP - IACTIV + 1                                      0000000 
        PIVOT = ISTART - 1                                              0000000 
        GO TO 1100                                                      0000000 
C                                                                       0000000 
C CALCULATE COLUMN COUNTS.                                              0000000 
  110   DO 120 II=IACTIV,ITOP                                           0000000 
          I = ICN(II)                                                   0000000 
          LENC(I) = LENC(I) + 1                                         0000000 
  120   CONTINUE                                                        0000000 
C SET UP COLUMN POINTERS SO THAT IPC(J) POINTS TO POSITION AFTER END    0000000 
C     OF COLUMN J IN COLUMN FILE.                                       0000000 
        IPC(ILAST) = LIRN + 1                                           0000000 
        J1 = ISTART + 1                                                 0000000 
        DO 130 JJ=J1,ILAST                                              0000000 
          J = ILAST - JJ + J1 - 1                                       0000000 
          IPC(J) = IPC(J+1) - LENC(J+1)                                 0000000 
  130   CONTINUE                                                        0000000 
        DO 150 INDROW=ISTART,ILAST                                      0000000 
          J1 = IPTR(INDROW)                                             0000000 
          J2 = J1 + LENR(INDROW) - 1                                    0000000 
          IF (J1.GT.J2) GO TO 150                                       0000000 
          DO 140 JJ=J1,J2                                               0000000 
            J = ICN(JJ)                                                 0000000 
            IPOS = IPC(J) - 1                                           0000000 
            IRN(IPOS) = INDROW                                          0000000 
            IPC(J) = IPOS                                               0000000 
  140     CONTINUE                                                      0000000 
  150   CONTINUE                                                        0000000 
C DISPC IS THE LOWEST INDEXED ACTIVE LOCATION IN THE COLUMN FILE.       0000000 
        DISPC = IPC(ISTART)                                             0000000 
        NZCOL = LIRN - DISPC + 1                                        0000000 
        MINIRN = MAX0(NZCOL,MINIRN)                                     0000000 
        NZMIN = 1                                                       0000000 
C                                                                       0000000 
C INITIALIZE ARRAY IFIRST.  IFIRST(I) = +/- K INDICATES THAT ROW/COL    0000000 
C     K HAS I NON-ZEROS.  IF IFIRST(I) = 0, THERE IS NO ROW OR COLUMN   0000000 
C     WITH I NON ZEROS.                                                 0000000 
        DO 160 I=1,N                                                    0000000 
          IFIRST(I) = 0                                                 0000000 
  160   CONTINUE                                                        0000000 
C                                                                       0000000 
C COMPUTE ORDERING OF ROW AND COLUMN COUNTS.                            0000000 
C FIRST RUN THROUGH COLUMNS (FROM COLUMN N TO COLUMN 1).                0000000 
        DO 180 JJ=ISTART,ILAST                                          0000000 
          J = ILAST - JJ + ISTART                                       0000000 
          NZ = LENC(J)                                                  0000000 
          IF (NZ.NE.0) GO TO 170                                        0000000 
          IPC(J) = 0                                                    0000000 
          GO TO 180                                                     0000000 
  170     IF (NSRCH.LE.NN) GO TO 180                                    0000000 
          ISW = IFIRST(NZ)                                              0000000 
          IFIRST(NZ) = -J                                               0000000 
          LASTC(J) = 0                                                  0000000 
          NEXTC(J) = -ISW                                               0000000 
          ISW1 = IABS(ISW)                                              0000000 
          IF (ISW.NE.0) LASTC(ISW1) = J                                 0000000 
  180   CONTINUE                                                        0000000 
C NOW RUN THROUGH ROWS (AGAIN FROM N TO 1).                             0000000 
        DO 210 II=ISTART,ILAST                                          0000000 
          I = ILAST - II + ISTART                                       0000000 
          NZ = LENR(I)                                                  0000000 
          IF (NZ.NE.0) GO TO 190                                        0000000 
          IPTR(I) = 0                                                   0000000 
          LASTR(I) = 0                                                  0000000 
          GO TO 210                                                     0000000 
  190     ISW = IFIRST(NZ)                                              0000000 
          IFIRST(NZ) = I                                                0000000 
          IF (ISW.GT.0) GO TO 200                                       0000000 
          NEXTR(I) = 0                                                  0000000 
          LASTR(I) = ISW                                                0000000 
          GO TO 210                                                     0000000 
  200     NEXTR(I) = ISW                                                0000000 
          LASTR(I) = LASTR(ISW)                                         0000000 
          LASTR(ISW) = I                                                0000000 
  210   CONTINUE                                                        0000000 
C                                                                       0000000 
C                                                                       0000000 
C **********************************************                        0000000 
C ****    START OF MAIN ELIMINATION LOOP    ****                        0000000 
C **********************************************                        0000000 
        DO 980 PIVOT=ISTART,ILAST                                       0000000 
C                                                                       0000000 
C FIRST FIND THE PIVOT USING MARKOWITZ CRITERION WITH STABILITY         0000000 
C     CONTROL.                                                          0000000 
C JCOST IS THE MARKOWITZ COST OF THE BEST PIVOT SO FAR,.. THIS          0000000 
C     PIVOT IS IN ROW IPIV AND COLUMN JPIV.                             0000000 
          NZ2 = NZMIN                                                   0000000 
          JCOST = N*N                                                   0000000 
C                                                                       0000000 
C EXAMINE ROWS/COLUMNS IN ORDER OF ASCENDING COUNT.                     0000000 
          DO 340 L=1,2                                                  0000000 
            PIVRAT = ZERO                                               0000000 
            ISRCH = 1                                                   0000000 
            LL = L                                                      0000000 
C A PASS WITH L EQUAL TO 2 IS ONLY PERFORMED IN THE CASE OF SINGULARITY.0000000 
            DO 330 NZ=NZ2,N                                             0000000 
              IF (JCOST.LE.(NZ-1)**2) GO TO 420                         0000000 
              IJFIR = IFIRST(NZ)                                        0000000 
              IF (IJFIR) 230, 220, 240                                  0000000 
  220         IF (LL.EQ.1) NZMIN = NZ + 1                               0000000 
              GO TO 330                                                 0000000 
  230         LL = 2                                                    0000000 
              IJFIR = -IJFIR                                            0000000 
              GO TO 290                                                 0000000 
  240         LL = 2                                                    0000000 
C SCAN ROWS WITH NZ NON-ZEROS.                                          0000000 
              DO 270 IDUMMY=1,N                                         0000000 
                IF (JCOST.LE.(NZ-1)**2) GO TO 420                       0000000 
                IF (ISRCH.GT.MSRCH) GO TO 420                           0000000 
                IF (IJFIR.EQ.0) GO TO 280                               0000000 
C ROW IJFIR IS NOW EXAMINED.                                            0000000 
                I = IJFIR                                               0000000 
                IJFIR = NEXTR(I)                                        0000000 
C FIRST CALCULATE MULTIPLIER THRESHOLD LEVEL.                           0000000 
                AMAX = ZERO                                             0000000 
                J1 = IPTR(I) + LENRL(I)                                 0000000 
                J2 = IPTR(I) + LENR(I) - 1                              0000000 
                DO 250 JJ=J1,J2                                         0000000 
                  AMAX = DMAX1(AMAX,DABS(A(JJ)))                                
  250           CONTINUE                                                0000000 
                AU = AMAX*U                                             0000000 
                ISRCH = ISRCH + 1                                       0000000 
C SCAN ROW FOR POSSIBLE PIVOTS                                          0000000 
                DO 260 JJ=J1,J2                                         0000000 
                  IF (DABS(A(JJ)).LE.AU .AND. L.EQ.1) GO TO 260                 
                  J = ICN(JJ)                                           0000000 
                  KCOST = (NZ-1)*(LENC(J)-1)                            0000000 
                  IF (KCOST.GT.JCOST) GO TO 260                         0000000 
                  PIVR = ZERO                                           0000000 
                  IF (AMAX.NE.ZERO) PIVR = DABS(A(JJ))/AMAX                     
                  IF (KCOST.EQ.JCOST .AND. (PIVR.LE.PIVRAT .OR.         0000000 
     *             NSRCH.GT.NN+1)) GO TO 260                            0000000 
C BEST PIVOT SO FAR IS FOUND.                                           0000000 
                  JCOST = KCOST                                         0000000 
                  IJPOS = JJ                                            0000000 
                  IPIV = I                                              0000000 
                  JPIV = J                                              0000000 
                  IF (MSRCH.GT.NN+1 .AND. JCOST.LE.(NZ-1)**2) GO TO 420 0000000 
                  PIVRAT = PIVR                                         0000000 
  260           CONTINUE                                                0000000 
  270         CONTINUE                                                  0000000 
C                                                                       0000000 
C COLUMNS WITH NZ NON-ZEROS NOW EXAMINED.                               0000000 
  280         IJFIR = IFIRST(NZ)                                        0000000 
              IJFIR = -LASTR(IJFIR)                                     0000000 
  290         IF (JCOST.LE.NZ*(NZ-1)) GO TO 420                         0000000 
              IF (MSRCH.LE.NN) GO TO 330                                0000000 
              DO 320 IDUMMY=1,N                                         0000000 
                IF (IJFIR.EQ.0) GO TO 330                               0000000 
                J = IJFIR                                               0000000 
                IJFIR = NEXTC(IJFIR)                                    0000000 
                I1 = IPC(J)                                             0000000 
                I2 = I1 + NZ - 1                                        0000000 
C SCAN COLUMN J.                                                        0000000 
                DO 310 II=I1,I2                                         0000000 
                  I = IRN(II)                                           0000000 
                  KCOST = (NZ-1)*(LENR(I)-LENRL(I)-1)                   0000000 
                  IF (KCOST.GE.JCOST) GO TO 310                         0000000 
C PIVOT HAS BEST MARKOWITZ COUNT SO FAR ... NOW CHECK ITS               0000000 
C     SUITABILITY ON NUMERIC GROUNDS BY EXAMINING THE OTHER NON-ZEROS   0000000 
C     IN ITS ROW.                                                       0000000 
                  J1 = IPTR(I) + LENRL(I)                               0000000 
                  J2 = IPTR(I) + LENR(I) - 1                            0000000 
C WE NEED A STABILITY CHECK ON SINGLETON COLUMNS BECAUSE OF POSSIBLE    0000000 
C     PROBLEMS WITH UNDERDETERMINED SYSTEMS.                            0000000 
                  AMAX = ZERO                                           0000000 
                  DO 300 JJ=J1,J2                                       0000000 
                    AMAX = DMAX1(AMAX,DABS(A(JJ)))                              
                    IF (ICN(JJ).EQ.J) JPOS = JJ                         0000000 
  300             CONTINUE                                              0000000 
                  IF (DABS(A(JPOS)).LE.AMAX*U .AND. L.EQ.1) GO TO 310           
                  JCOST = KCOST                                         0000000 
                  IPIV = I                                              0000000 
                  JPIV = J                                              0000000 
                  IJPOS = JPOS                                          0000000 
                  IF (AMAX.NE.ZERO) PIVRAT = DABS(A(JPOS))/AMAX                 
                  IF (JCOST.LE.NZ*(NZ-1)) GO TO 420                     0000000 
  310           CONTINUE                                                0000000 
C                                                                       0000000 
  320         CONTINUE                                                  0000000 
C                                                                       0000000 
  330       CONTINUE                                                    0000000 
C IN THE EVENT OF SINGULARITY, WE MUST MAKE SURE ALL ROWS AND COLUMNS   0000000 
C ARE TESTED.                                                           0000000 
            MSRCH = N                                                   0000000 
C                                                                       0000000 
C MATRIX IS NUMERICALLY OR STRUCTURALLY SINGULAR  ... WHICH IT IS WILL  0000000 
C     BE DIAGNOSED LATER.                                               0000000 
            IRANK = IRANK - 1                                           0000000 
  340     CONTINUE                                                      0000000 
C ASSIGN REST OF ROWS AND COLUMNS TO ORDERING ARRAY.                    0000000 
C MATRIX IS STRUCTURALLY SINGULAR.                                      0000000 
          IF (IFLAG.NE.2 .AND. IFLAG.NE.-5) IFLAG = 1                   0000000 
          IRANK = IRANK - ILAST + PIVOT + 1                             0000000 
          IF (.NOT.ABORT1) GO TO 350                                    0000000 
          IDISP(2) = IACTIV                                             0000000 
          IFLAG = -1                                                    0000000 
          IF (LP.NE.0) WRITE (LP,99999)                                 0000000 
          GO TO 1120                                                    0000000 
  350     K = PIVOT - 1                                                 0000000 
          DO 390 I=ISTART,ILAST                                         0000000 
            IF (LASTR(I).NE.0) GO TO 390                                0000000 
            K = K + 1                                                   0000000 
            LASTR(I) = K                                                0000000 
            IF (LENRL(I).EQ.0) GO TO 380                                0000000 
            MINICN = MAX0(MINICN,NZROW+IBEG-1+MOREI+LENRL(I))           0000000 
            IF (IACTIV-IBEG.GE.LENRL(I)) GO TO 360                      0000000 
            CALL MA30DD(A, ICN, IPTR(ISTART), N, IACTIV, ITOP, .TRUE.)          
C CHECK NOW TO SEE IF MA30D/DD HAS CREATED ENOUGH AVAILABLE SPACE.      0000000 
            IF (IACTIV-IBEG.GE.LENRL(I)) GO TO 360                      0000000 
C CREATE MORE SPACE BY DESTROYING PREVIOUSLY CREATED LU FACTORS.        0000000 
            MOREI = MOREI + IBEG - IDISP(1)                             0000000 
            IBEG = IDISP(1)                                             0000000 
            IF (LP.NE.0) WRITE (LP,99997)                               0000000 
            IFLAG = -5                                                  0000000 
            IF (ABORT3) GO TO 1090                                      0000000 
  360       J1 = IPTR(I)                                                0000000 
            J2 = J1 + LENRL(I) - 1                                      0000000 
            IPTR(I) = 0                                                 0000000 
            DO 370 JJ=J1,J2                                             0000000 
              A(IBEG) = A(JJ)                                           0000000 
              ICN(IBEG) = ICN(JJ)                                       0000000 
              ICN(JJ) = 0                                               0000000 
              IBEG = IBEG + 1                                           0000000 
  370       CONTINUE                                                    0000000 
            NZROW = NZROW - LENRL(I)                                    0000000 
  380       IF (K.EQ.ILAST) GO TO 400                                   0000000 
  390     CONTINUE                                                      0000000 
  400     K = PIVOT - 1                                                 0000000 
          DO 410 I=ISTART,ILAST                                         0000000 
            IF (IPC(I).NE.0) GO TO 410                                  0000000 
            K = K + 1                                                   0000000 
            IPC(I) = K                                                  0000000 
            IF (K.EQ.ILAST) GO TO 990                                   0000000 
  410     CONTINUE                                                      0000000 
C                                                                       0000000 
C THE PIVOT HAS NOW BEEN FOUND IN POSITION (IPIV,JPIV) IN LOCATION      0000000 
C     IJPOS IN ROW FILE.                                                0000000 
C UPDATE COLUMN AND ROW ORDERING ARRAYS TO CORRESPOND WITH REMOVAL      0000000 
C     OF THE ACTIVE PART OF THE MATRIX.                                 0000000 
  420     ISING = PIVOT                                                 0000000 
          IF (A(IJPOS).NE.ZERO) GO TO 430                               0000000 
C NUMERICAL SINGULARITY IS RECORDED HERE.                               0000000 
          ISING = -ISING                                                0000000 
          IF (IFLAG.NE.-5) IFLAG = 2                                    0000000 
          IF (.NOT.ABORT2) GO TO 430                                    0000000 
          IDISP(2) = IACTIV                                             0000000 
          IFLAG = -2                                                    0000000 
          IF (LP.NE.0) WRITE (LP,99998)                                 0000000 
          GO TO 1120                                                    0000000 
  430     OLDPIV = IPTR(IPIV) + LENRL(IPIV)                             0000000 
          OLDEND = IPTR(IPIV) + LENR(IPIV) - 1                          0000000 
C CHANGES TO COLUMN ORDERING.                                           0000000 
          IF (NSRCH.LE.NN) GO TO 460                                    0000000 
          DO 450 JJ=OLDPIV,OLDEND                                       0000000 
            J = ICN(JJ)                                                 0000000 
            LC = LASTC(J)                                               0000000 
            NC = NEXTC(J)                                               0000000 
            IF (NC.NE.0) LASTC(NC) = LC                                 0000000 
            IF (LC.EQ.0) GO TO 440                                      0000000 
            NEXTC(LC) = NC                                              0000000 
            GO TO 450                                                   0000000 
  440       NZ = LENC(J)                                                0000000 
            ISW = IFIRST(NZ)                                            0000000 
            IF (ISW.GT.0) LASTR(ISW) = -NC                              0000000 
            IF (ISW.LT.0) IFIRST(NZ) = -NC                              0000000 
  450     CONTINUE                                                      0000000 
C CHANGES TO ROW ORDERING.                                              0000000 
  460     I1 = IPC(JPIV)                                                0000000 
          I2 = I1 + LENC(JPIV) - 1                                      0000000 
          DO 480 II=I1,I2                                               0000000 
            I = IRN(II)                                                 0000000 
            LR = LASTR(I)                                               0000000 
            NR = NEXTR(I)                                               0000000 
            IF (NR.NE.0) LASTR(NR) = LR                                 0000000 
            IF (LR.LE.0) GO TO 470                                      0000000 
            NEXTR(LR) = NR                                              0000000 
            GO TO 480                                                   0000000 
  470       NZ = LENR(I) - LENRL(I)                                     0000000 
            IF (NR.NE.0) IFIRST(NZ) = NR                                0000000 
            IF (NR.EQ.0) IFIRST(NZ) = LR                                0000000 
  480     CONTINUE                                                      0000000 
C                                                                       0000000 
C MOVE PIVOT TO POSITION LENRL+1 IN PIVOT ROW AND MOVE PIVOT ROW        0000000 
C     TO THE BEGINNING OF THE AVAILABLE STORAGE.                        0000000 
C THE L PART AND THE PIVOT IN THE OLD COPY OF THE PIVOT ROW IS          0000000 
C     NULLIFIED WHILE, IN THE STRICTLY UPPER TRIANGULAR PART, THE       0000000 
C     COLUMN INDICES, J SAY, ARE OVERWRITTEN BY THE CORRESPONDING       0000000 
C     ENTRY OF IQ (IQ(J)) AND IQ(J) IS SET TO THE NEGATIVE OF THE       0000000 
C     DISPLACEMENT OF THE COLUMN INDEX FROM THE PIVOT ENTRY.            0000000 
          IF (OLDPIV.EQ.IJPOS) GO TO 490                                0000000 
          AU = A(OLDPIV)                                                0000000 
          A(OLDPIV) = A(IJPOS)                                          0000000 
          A(IJPOS) = AU                                                 0000000 
          ICN(IJPOS) = ICN(OLDPIV)                                      0000000 
          ICN(OLDPIV) = JPIV                                            0000000 
C CHECK TO SEE IF THERE IS SPACE IMMEDIATELY AVAILABLE IN A/ICN TO      0000000 
C     HOLD NEW COPY OF PIVOT ROW.                                       0000000 
  490     MINICN = MAX0(MINICN,NZROW+IBEG-1+MOREI+LENR(IPIV))           0000000 
          IF (IACTIV-IBEG.GE.LENR(IPIV)) GO TO 500                      0000000 
          CALL MA30DD(A, ICN, IPTR(ISTART), N, IACTIV, ITOP, .TRUE.)            
          OLDPIV = IPTR(IPIV) + LENRL(IPIV)                             0000000 
          OLDEND = IPTR(IPIV) + LENR(IPIV) - 1                          0000000 
C CHECK NOW TO SEE IF MA30D/DD HAS CREATED ENOUGH AVAILABLE SPACE.      0000000 
          IF (IACTIV-IBEG.GE.LENR(IPIV)) GO TO 500                      0000000 
C CREATE MORE SPACE BY DESTROYING PREVIOUSLY CREATED LU FACTORS.        0000000 
          MOREI = MOREI + IBEG - IDISP(1)                               0000000 
          IBEG = IDISP(1)                                               0000000 
          IF (LP.NE.0) WRITE (LP,99997)                                 0000000 
          IFLAG = -5                                                    0000000 
          IF (ABORT3) GO TO 1090                                        0000000 
          IF (IACTIV-IBEG.GE.LENR(IPIV)) GO TO 500                      0000000 
C THERE IS STILL NOT ENOUGH ROOM IN A/ICN.                              0000000 
          IFLAG = -4                                                    0000000 
          GO TO 1090                                                    0000000 
C COPY PIVOT ROW AND SET UP IQ ARRAY.                                   0000000 
  500     IJPOS = 0                                                     0000000 
          J1 = IPTR(IPIV)                                               0000000 
C                                                                       0000000 
          DO 530 JJ=J1,OLDEND                                           0000000 
            A(IBEG) = A(JJ)                                             0000000 
            ICN(IBEG) = ICN(JJ)                                         0000000 
            IF (IJPOS.NE.0) GO TO 510                                   0000000 
            IF (ICN(JJ).EQ.JPIV) IJPOS = IBEG                           0000000 
            ICN(JJ) = 0                                                 0000000 
            GO TO 520                                                   0000000 
  510       K = IBEG - IJPOS                                            0000000 
            J = ICN(JJ)                                                 0000000 
            ICN(JJ) = IQ(J)                                             0000000 
            IQ(J) = -K                                                  0000000 
  520       IBEG = IBEG + 1                                             0000000 
  530     CONTINUE                                                      0000000 
C                                                                       0000000 
          IJP1 = IJPOS + 1                                              0000000 
          PIVEND = IBEG - 1                                             0000000 
          LENPIV = PIVEND - IJPOS                                       0000000 
          NZROW = NZROW - LENRL(IPIV) - 1                               0000000 
          IPTR(IPIV) = OLDPIV + 1                                       0000000 
          IF (LENPIV.EQ.0) IPTR(IPIV) = 0                               0000000 
C                                                                       0000000 
C REMOVE PIVOT ROW (INCLUDING PIVOT) FROM COLUMN ORIENTED FILE.         0000000 
          DO 560 JJ=IJPOS,PIVEND                                        0000000 
            J = ICN(JJ)                                                 0000000 
            I1 = IPC(J)                                                 0000000 
            LENC(J) = LENC(J) - 1                                       0000000 
C I2 IS LAST POSITION IN NEW COLUMN.                                    0000000 
            I2 = IPC(J) + LENC(J) - 1                                   0000000 
            IF (I2.LT.I1) GO TO 550                                     0000000 
            DO 540 II=I1,I2                                             0000000 
              IF (IRN(II).NE.IPIV) GO TO 540                            0000000 
              IRN(II) = IRN(I2+1)                                       0000000 
              GO TO 550                                                 0000000 
  540       CONTINUE                                                    0000000 
  550       IRN(I2+1) = 0                                               0000000 
  560     CONTINUE                                                      0000000 
          NZCOL = NZCOL - LENPIV - 1                                    0000000 
C                                                                       0000000 
C GO DOWN THE PIVOT COLUMN AND FOR EACH ROW WITH A NON-ZERO ADD         0000000 
C     THE APPROPRIATE MULTIPLE OF THE PIVOT ROW TO IT.                  0000000 
C WE LOOP ON THE NUMBER OF NON-ZEROS IN THE PIVOT COLUMN SINCE          0000000 
C     MA30D/DD MAY CHANGE ITS ACTUAL POSITION.                          0000000 
C                                                                       0000000 
          NZPC = LENC(JPIV)                                             0000000 
          IF (NZPC.EQ.0) GO TO 900                                      0000000 
          DO 840 III=1,NZPC                                             0000000 
            II = IPC(JPIV) + III - 1                                    0000000 
            I = IRN(II)                                                 0000000 
C SEARCH ROW I FOR NON-ZERO TO BE ELIMINATED, CALCULATE MULTIPLIER,     0000000 
C     AND PLACE IT IN POSITION LENRL+1 IN ITS ROW.                      0000000 
C  IDROP IS THE NUMBER OF NON-ZERO ENTRIES DROPPED FROM ROW    I        0000000 
C        BECAUSE THESE FALL BENEATH TOLERANCE LEVEL.                    0000000 
C                                                                       0000000 
            IDROP = 0                                                   0000000 
            J1 = IPTR(I) + LENRL(I)                                     0000000 
            IEND = IPTR(I) + LENR(I) - 1                                0000000 
            DO 570 JJ=J1,IEND                                           0000000 
              IF (ICN(JJ).NE.JPIV) GO TO 570                            0000000 
C IF PIVOT IS ZERO, REST OF COLUMN IS AND SO MULTIPLIER IS ZERO.        0000000 
              AU = ZERO                                                 0000000 
              IF (A(IJPOS).NE.ZERO) AU = -A(JJ)/A(IJPOS)                0000000 
              IF (LBIG) BIG = DMAX1(BIG,DABS(AU))                               
              A(JJ) = A(J1)                                             0000000 
              A(J1) = AU                                                0000000 
              ICN(JJ) = ICN(J1)                                         0000000 
              ICN(J1) = JPIV                                            0000000 
              LENRL(I) = LENRL(I) + 1                                   0000000 
              GO TO 580                                                 0000000 
  570       CONTINUE                                                    0000000 
C JUMP IF PIVOT ROW IS A SINGLETON.                                     0000000 
  580       IF (LENPIV.EQ.0) GO TO 840                                  0000000 
C NOW PERFORM NECESSARY OPERATIONS ON REST OF NON-PIVOT ROW I.          0000000 
            ROWI = J1 + 1                                               0000000 
            IOP = 0                                                     0000000 
C JUMP IF ALL THE PIVOT ROW CAUSES FILL-IN.                             0000000 
            IF (ROWI.GT.IEND) GO TO 650                                 0000000 
C PERFORM OPERATIONS ON CURRENT NON-ZEROS IN ROW I.                     0000000 
C INNERMOST LOOP.                                                       0000000 
            DO 590 JJ=ROWI,IEND                                         0000000 
              J = ICN(JJ)                                               0000000 
              IF (IQ(J).GT.0) GO TO 590                                 0000000 
              IOP = IOP + 1                                             0000000 
              PIVROW = IJPOS - IQ(J)                                    0000000 
              A(JJ) = A(JJ) + AU*A(PIVROW)                              0000000 
              IF (LBIG) BIG = DMAX1(DABS(A(JJ)),BIG)                            
              ICN(PIVROW) = -ICN(PIVROW)                                0000000 
              IF (DABS(A(JJ)).LT.TOL) IDROP = IDROP + 1                         
  590       CONTINUE                                                    0000000 
C                                                                       0000000 
C  JUMP IF NO NON-ZEROS IN NON-PIVOT ROW HAVE BEEN REMOVED              0000000 
C       BECAUSE THESE ARE BENEATH THE DROP-TOLERANCE  TOL.              0000000 
C                                                                       0000000 
            IF (IDROP.EQ.0) GO TO 650                                   0000000 
C                                                                       0000000 
C  RUN THROUGH NON-PIVOT ROW COMPRESSING ROW SO THAT ONLY               0000000 
C      NON-ZEROS GREATER THAN   TOL   ARE STORED.  ALL NON-ZEROS        0000000 
C      LESS THAN   TOL   ARE ALSO REMOVED FROM THE COLUMN STRUCTURE.    0000000 
C                                                                       0000000 
            JNEW = ROWI                                                 0000000 
            DO 630 JJ=ROWI,IEND                                         0000000 
              IF (DABS(A(JJ)).LT.TOL) GO TO 600                                 
              A(JNEW) = A(JJ)                                           0000000 
              ICN(JNEW) = ICN(JJ)                                       0000000 
              JNEW = JNEW + 1                                           0000000 
              GO TO 630                                                 0000000 
C                                                                       0000000 
C  REMOVE NON-ZERO ENTRY FROM COLUMN STRUCTURE.                         0000000 
C                                                                       0000000 
  600         J = ICN(JJ)                                               0000000 
              I1 = IPC(J)                                               0000000 
              I2 = I1 + LENC(J) - 1                                     0000000 
              DO 610 II=I1,I2                                           0000000 
                IF (IRN(II).EQ.I) GO TO 620                             0000000 
  610         CONTINUE                                                  0000000 
  620         IRN(II) = IRN(I2)                                         0000000 
              IRN(I2) = 0                                               0000000 
              LENC(J) = LENC(J) - 1                                     0000000 
  630       CONTINUE                                                    0000000 
            DO 640 JJ=JNEW,IEND                                         0000000 
              ICN(JJ) = 0                                               0000000 
  640       CONTINUE                                                    0000000 
C THE VALUE OF IDROP MIGHT BE DIFFERENT FROM THAT CALCULATED EARLIER    0000000 
C     BECAUSE, WE MAY NOW HAVE DROPPED SOME NON-ZEROS WHICH WERE NOT    0000000 
C     MODIFIED BY THE PIVOT ROW.                                        0000000 
            IDROP = IEND + 1 - JNEW                                     0000000 
            IEND = JNEW - 1                                             0000000 
            LENR(I) = LENR(I) - IDROP                                   0000000 
            NZROW = NZROW - IDROP                                       0000000 
            NZCOL = NZCOL - IDROP                                       0000000 
            NDROP = NDROP + IDROP                                       0000000 
  650       IFILL = LENPIV - IOP                                        0000000 
C JUMP IS IF THERE IS NO FILL-IN.                                       0000000 
            IF (IFILL.EQ.0) GO TO 750                                   0000000 
C NOW FOR THE FILL-IN.                                                  0000000 
            MINICN = MAX0(MINICN,MOREI+IBEG-1+NZROW+IFILL+LENR(I))      0000000 
C SEE IF THERE IS ROOM FOR FILL-IN.                                     0000000 
C GET MAXIMUM SPACE FOR ROW I IN SITU.                                  0000000 
            DO 660 JDIFF=1,IFILL                                        0000000 
              JNPOS = IEND + JDIFF                                      0000000 
              IF (JNPOS.GT.LICN) GO TO 670                              0000000 
              IF (ICN(JNPOS).NE.0) GO TO 670                            0000000 
  660       CONTINUE                                                    0000000 
C THERE IS ROOM FOR ALL THE FILL-IN AFTER THE END OF THE ROW SO IT      0000000 
C     CAN BE LEFT IN SITU.                                              0000000 
C NEXT AVAILABLE SPACE FOR FILL-IN.                                     0000000 
            IEND = IEND + 1                                             0000000 
            GO TO 750                                                   0000000 
C JMORE SPACES FOR FILL-IN ARE REQUIRED IN FRONT OF ROW.                0000000 
  670       JMORE = IFILL - JDIFF + 1                                   0000000 
            I1 = IPTR(I)                                                0000000 
C WE NOW LOOK IN FRONT OF THE ROW TO SEE IF THERE IS SPACE FOR          0000000 
C     THE REST OF THE FILL-IN.                                          0000000 
            DO 680 JDIFF=1,JMORE                                        0000000 
              JNPOS = I1 - JDIFF                                        0000000 
              IF (JNPOS.LT.IACTIV) GO TO 690                            0000000 
              IF (ICN(JNPOS).NE.0) GO TO 700                            0000000 
  680       CONTINUE                                                    0000000 
  690       JNPOS = I1 - JMORE                                          0000000 
            GO TO 710                                                   0000000 
C WHOLE ROW MUST BE MOVED TO THE BEGINNING OF AVAILABLE STORAGE.        0000000 
  700       JNPOS = IACTIV - LENR(I) - IFILL                            0000000 
C JUMP IF THERE IS SPACE IMMEDIATELY AVAILABLE FOR THE SHIFTED ROW.     0000000 
  710       IF (JNPOS.GE.IBEG) GO TO 730                                0000000 
            CALL MA30DD(A, ICN, IPTR(ISTART), N, IACTIV, ITOP, .TRUE.)          
            I1 = IPTR(I)                                                0000000 
            IEND = I1 + LENR(I) - 1                                     0000000 
            JNPOS = IACTIV - LENR(I) - IFILL                            0000000 
            IF (JNPOS.GE.IBEG) GO TO 730                                0000000 
C NO SPACE AVAILABLE SO TRY TO CREATE SOME BY THROWING AWAY PREVIOUS    0000000 
C     LU DECOMPOSITION.                                                 0000000 
            MOREI = MOREI + IBEG - IDISP(1) - LENPIV - 1                0000000 
            IF (LP.NE.0) WRITE (LP,99997)                               0000000 
            IFLAG = -5                                                  0000000 
            IF (ABORT3) GO TO 1090                                      0000000 
C KEEP RECORD OF CURRENT PIVOT ROW.                                     0000000 
            IBEG = IDISP(1)                                             0000000 
            ICN(IBEG) = JPIV                                            0000000 
            A(IBEG) = A(IJPOS)                                          0000000 
            IJPOS = IBEG                                                0000000 
            DO 720 JJ=IJP1,PIVEND                                       0000000 
              IBEG = IBEG + 1                                           0000000 
              A(IBEG) = A(JJ)                                           0000000 
              ICN(IBEG) = ICN(JJ)                                       0000000 
  720       CONTINUE                                                    0000000 
            IJP1 = IJPOS + 1                                            0000000 
            PIVEND = IBEG                                               0000000 
            IBEG = IBEG + 1                                             0000000 
            IF (JNPOS.GE.IBEG) GO TO 730                                0000000 
C THIS STILL DOES NOT GIVE ENOUGH ROOM.                                 0000000 
            IFLAG = -4                                                  0000000 
            GO TO 1090                                                  0000000 
  730       IACTIV = MIN0(IACTIV,JNPOS)                                 0000000 
C MOVE NON-PIVOT ROW I.                                                 0000000 
            IPTR(I) = JNPOS                                             0000000 
            DO 740 JJ=I1,IEND                                           0000000 
              A(JNPOS) = A(JJ)                                          0000000 
              ICN(JNPOS) = ICN(JJ)                                      0000000 
              JNPOS = JNPOS + 1                                         0000000 
              ICN(JJ) = 0                                               0000000 
  740       CONTINUE                                                    0000000 
C FIRST NEW AVAILABLE SPACE.                                            0000000 
            IEND = JNPOS                                                0000000 
  750       NZROW = NZROW + IFILL                                       0000000 
C INNERMOST FILL-IN LOOP WHICH ALSO RESETS ICN.                         0000000 
            DO 830 JJ=IJP1,PIVEND                                       0000000 
              J = ICN(JJ)                                               0000000 
              IF (J.LT.0) GO TO 820                                     0000000 
              ANEW = AU*A(JJ)                                           0000000 
              AANEW = DABS(ANEW)                                                
              IF (AANEW.GE.TOL) GO TO 760                               0000000 
              NDROP = NDROP + 1                                         0000000 
              NZROW = NZROW - 1                                         0000000 
              MINICN = MINICN - 1                                       0000000 
              IFILL = IFILL - 1                                         0000000 
              GO TO 830                                                 0000000 
  760         IF (LBIG) BIG = DMAX1(AANEW,BIG)                                  
              A(IEND) = ANEW                                            0000000 
              ICN(IEND) = J                                             0000000 
              IEND = IEND + 1                                           0000000 
C                                                                       0000000 
C PUT NEW ENTRY IN COLUMN FILE.                                         0000000 
              MINIRN = MAX0(MINIRN,NZCOL+LENC(J)+1)                     0000000 
              JEND = IPC(J) + LENC(J)                                   0000000 
              JROOM = NZPC - III + 1 + LENC(J)                          0000000 
              IF (JEND.GT.LIRN) GO TO 770                               0000000 
              IF (IRN(JEND).EQ.0) GO TO 810                             0000000 
  770         IF (JROOM.LT.DISPC) GO TO 780                             0000000 
C COMPRESS COLUMN FILE TO OBTAIN SPACE FOR NEW COPY OF COLUMN.          0000000 
              CALL MA30DD(A, IRN, IPC(ISTART), N, DISPC, LIRN, .FALSE.)         
              IF (JROOM.LT.DISPC) GO TO 780                             0000000 
              JROOM = DISPC - 1                                         0000000 
              IF (JROOM.GE.LENC(J)+1) GO TO 780                         0000000 
C COLUMN FILE IS NOT LARGE ENOUGH.                                      0000000 
              GO TO 1100                                                0000000 
C COPY COLUMN TO BEGINNING OF FILE.                                     0000000 
  780         JBEG = IPC(J)                                             0000000 
              JEND = IPC(J) + LENC(J) - 1                               0000000 
              JZERO = DISPC - 1                                         0000000 
              DISPC = DISPC - JROOM                                     0000000 
              IDISPC = DISPC                                            0000000 
              DO 790 II=JBEG,JEND                                       0000000 
                IRN(IDISPC) = IRN(II)                                   0000000 
                IRN(II) = 0                                             0000000 
                IDISPC = IDISPC + 1                                     0000000 
  790         CONTINUE                                                  0000000 
              IPC(J) = DISPC                                            0000000 
              JEND = IDISPC                                             0000000 
              DO 800 II=JEND,JZERO                                      0000000 
                IRN(II) = 0                                             0000000 
  800         CONTINUE                                                  0000000 
  810         IRN(JEND) = I                                             0000000 
              NZCOL = NZCOL + 1                                         0000000 
              LENC(J) = LENC(J) + 1                                     0000000 
C END OF ADJUSTMENT TO COLUMN FILE.                                     0000000 
              GO TO 830                                                 0000000 
C                                                                       0000000 
  820         ICN(JJ) = -J                                              0000000 
  830       CONTINUE                                                    0000000 
            LENR(I) = LENR(I) + IFILL                                   0000000 
C END OF SCAN OF PIVOT COLUMN.                                          0000000 
  840     CONTINUE                                                      0000000 
C                                                                       0000000 
C                                                                       0000000 
C REMOVE PIVOT COLUMN FROM COLUMN ORIENTED STORAGE AND UPDATE ROW       0000000 
C     ORDERING ARRAYS.                                                  0000000 
          I1 = IPC(JPIV)                                                0000000 
          I2 = IPC(JPIV) + LENC(JPIV) - 1                               0000000 
          NZCOL = NZCOL - LENC(JPIV)                                    0000000 
          DO 890 II=I1,I2                                               0000000 
            I = IRN(II)                                                 0000000 
            IRN(II) = 0                                                 0000000 
            NZ = LENR(I) - LENRL(I)                                     0000000 
            IF (NZ.NE.0) GO TO 850                                      0000000 
            LASTR(I) = 0                                                0000000 
            GO TO 890                                                   0000000 
  850       IFIR = IFIRST(NZ)                                           0000000 
            IFIRST(NZ) = I                                              0000000 
            IF (IFIR) 860, 880, 870                                     0000000 
  860       LASTR(I) = IFIR                                             0000000 
            NEXTR(I) = 0                                                0000000 
            GO TO 890                                                   0000000 
  870       LASTR(I) = LASTR(IFIR)                                      0000000 
            NEXTR(I) = IFIR                                             0000000 
            LASTR(IFIR) = I                                             0000000 
            GO TO 890                                                   0000000 
  880       LASTR(I) = 0                                                0000000 
            NEXTR(I) = 0                                                0000000 
            NZMIN = MIN0(NZMIN,NZ)                                      0000000 
  890     CONTINUE                                                      0000000 
C RESTORE IQ AND NULLIFY U PART OF OLD PIVOT ROW.                       0000000 
C    RECORD THE COLUMN PERMUTATION IN LASTC(JPIV) AND THE ROW           0000000 
C    PERMUTATION IN LASTR(IPIV).                                        0000000 
  900     IPC(JPIV) = -ISING                                            0000000 
          LASTR(IPIV) = PIVOT                                           0000000 
          IF (LENPIV.EQ.0) GO TO 980                                    0000000 
          NZROW = NZROW - LENPIV                                        0000000 
          JVAL = IJP1                                                   0000000 
          JZER = IPTR(IPIV)                                             0000000 
          IPTR(IPIV) = 0                                                0000000 
          DO 910 JCOUNT=1,LENPIV                                        0000000 
            J = ICN(JVAL)                                               0000000 
            IQ(J) = ICN(JZER)                                           0000000 
            ICN(JZER) = 0                                               0000000 
            JVAL = JVAL + 1                                             0000000 
            JZER = JZER + 1                                             0000000 
  910     CONTINUE                                                      0000000 
C ADJUST COLUMN ORDERING ARRAYS.                                        0000000 
          DO 970 JJ=IJP1,PIVEND                                         0000000 
            J = ICN(JJ)                                                 0000000 
            NZ = LENC(J)                                                0000000 
            IF (NZ.NE.0) GO TO 920                                      0000000 
            IPC(J) = 0                                                  0000000 
            GO TO 970                                                   0000000 
  920       IF (NSRCH.LE.NN) GO TO 960                                  0000000 
            IFIR = IFIRST(NZ)                                           0000000 
            LASTC(J) = 0                                                0000000 
            IF (IFIR) 930, 940, 950                                     0000000 
  930       IFIRST(NZ) = -J                                             0000000 
            IFIR = -IFIR                                                0000000 
            LASTC(IFIR) = J                                             0000000 
            NEXTC(J) = IFIR                                             0000000 
            GO TO 970                                                   0000000 
  940       IFIRST(NZ) = -J                                             0000000 
            NEXTC(J) = 0                                                0000000 
            GO TO 960                                                   0000000 
  950       LC = -LASTR(IFIR)                                           0000000 
            LASTR(IFIR) = -J                                            0000000 
            NEXTC(J) = LC                                               0000000 
            IF (LC.NE.0) LASTC(LC) = J                                  0000000 
  960       NZMIN = MIN0(NZMIN,NZ)                                      0000000 
  970     CONTINUE                                                      0000000 
  980   CONTINUE                                                        0000000 
C ********************************************                          0000000 
C ****    END OF MAIN ELIMINATION LOOP    ****                          0000000 
C ********************************************                          0000000 
C                                                                       0000000 
C RESET IACTIV TO POINT TO THE BEGINNING OF THE NEXT BLOCK.             0000000 
  990   IF (ILAST.NE.NN) IACTIV = IPTR(ILAST+1)                         0000000 
 1000 CONTINUE                                                          0000000 
C                                                                       0000000 
C ********************************************                          0000000 
C ****    END OF DEOMPOSITION OF BLOCK    ****                          0000000 
C ********************************************                          0000000 
C                                                                       0000000 
C RECORD SINGULARITY (IF ANY) IN IQ ARRAY.                              0000000 
      IF (IRANK.EQ.NN) GO TO 1020                                       0000000 
      DO 1010 I=1,NN                                                    0000000 
        IF (IPC(I).LT.0) GO TO 1010                                     0000000 
        ISING = IPC(I)                                                  0000000 
        IQ(ISING) = -IQ(ISING)                                          0000000 
        IPC(I) = -ISING                                                 0000000 
 1010 CONTINUE                                                          0000000 
C                                                                       0000000 
C RUN THROUGH LU DECOMPOSITION CHANGING COLUMN INDICES TO THAT OF NEW   0000000 
C     ORDER AND PERMUTING LENR AND LENRL ARRAYS ACCORDING TO PIVOT      0000000 
C     PERMUTATIONS.                                                     0000000 
 1020 ISTART = IDISP(1)                                                 0000000 
      IEND = IBEG - 1                                                   0000000 
      IF (IEND.LT.ISTART) GO TO 1040                                    0000000 
      DO 1030 JJ=ISTART,IEND                                            0000000 
        JOLD = ICN(JJ)                                                  0000000 
        ICN(JJ) = -IPC(JOLD)                                            0000000 
 1030 CONTINUE                                                          0000000 
 1040 DO 1050 II=1,NN                                                   0000000 
        I = LASTR(II)                                                   0000000 
        NEXTR(I) = LENR(II)                                             0000000 
        IPTR(I) = LENRL(II)                                             0000000 
 1050 CONTINUE                                                          0000000 
      DO 1060 I=1,NN                                                    0000000 
        LENRL(I) = IPTR(I)                                              0000000 
        LENR(I) = NEXTR(I)                                              0000000 
 1060 CONTINUE                                                          0000000 
C                                                                       0000000 
C UPDATE PERMUTATION ARRAYS IP AND IQ.                                  0000000 
      DO 1070 II=1,NN                                                   0000000 
        I = LASTR(II)                                                   0000000 
        J = -IPC(II)                                                    0000000 
        NEXTR(I) = IABS(IP(II)+0)                                       0000000 
        IPTR(J) = IABS(IQ(II)+0)                                        0000000 
 1070 CONTINUE                                                          0000000 
      DO 1080 I=1,NN                                                    0000000 
        IF (IP(I).LT.0) NEXTR(I) = -NEXTR(I)                            0000000 
        IP(I) = NEXTR(I)                                                0000000 
        IF (IQ(I).LT.0) IPTR(I) = -IPTR(I)                              0000000 
        IQ(I) = IPTR(I)                                                 0000000 
 1080 CONTINUE                                                          0000000 
      IP(NN) = IABS(IP(NN)+0)                                           0000000 
      IDISP(2) = IEND                                                   0000000 
      GO TO 1120                                                        0000000 
C                                                                       0000000 
C   ***    ERROR RETURNS    ***                                         0000000 
 1090 IDISP(2) = IACTIV                                                 0000000 
      IF (LP.EQ.0) GO TO 1120                                           0000000 
      WRITE (LP,99996)                                                  0000000 
      GO TO 1110                                                        0000000 
 1100 IF (IFLAG.EQ.-5) IFLAG = -6                                       0000000 
      IF (IFLAG.NE.-6) IFLAG = -3                                       0000000 
      IDISP(2) = IACTIV                                                 0000000 
      IF (LP.EQ.0) GO TO 1120                                           0000000 
      IF (IFLAG.EQ.-3) WRITE (LP,99995)                                 0000000 
      IF (IFLAG.EQ.-6) WRITE (LP,99994)                                 0000000 
 1110 PIVOT = PIVOT - ISTART + 1                                        0000000 
      WRITE (LP,99993) PIVOT, NBLOCK, ISTART, ILAST                     0000000 
      IF (PIVOT.EQ.0) WRITE (LP,99992) MINIRN                           0000000 
C                                                                       0000000 
C                                                                       0000000 
 1120 RETURN                                                            0000000 
99999 FORMAT (54H ERROR RETURN FROM MA30A/AD BECAUSE MATRIX IS STRUCTUR,0000000 
     * 13HALLY SINGULAR)                                                0000000 
99998 FORMAT (54H ERROR RETURN FROM MA30A/AD BECAUSE MATRIX IS NUMERICA,0000000 
     * 12HLLY SINGULAR)                                                 0000000 
99997 FORMAT (48H LU DECOMPOSITION DESTROYED TO CREATE MORE SPACE)      0000000 
99996 FORMAT (54H ERROR RETURN FROM MA30A/AD BECAUSE LICN NOT BIG ENOUG,0000000 
     * 1HH)                                                             0000000 
99995 FORMAT (54H ERROR RETURN FROM MA30A/AD BECAUSE LIRN NOT BIG ENOUG,0000000 
     * 1HH)                                                             0000000 
99994 FORMAT (51H ERROR RETURN FROM MA30A/AD LIRN AND LICN TOO SMALL)   0000000 
99993 FORMAT (10H AT STAGE , I5, 10H IN BLOCK , I5, 16H WITH FIRST ROW ,0000000 
     * I5, 14H AND LAST ROW , I5)                                       0000000 
99992 FORMAT (34H TO CONTINUE SET LIRN TO AT LEAST , I8)                0000000 
      END                                                               0000000 
      SUBROUTINE MA30DD(A, ICN, IPTR, N, IACTIV, ITOP, REALS)                   
c_270390
      EXTERNAL MA30$DATA
c_270390
C THIS SUBROUTINE PERFORMS GARBAGE COLLECTION OPERATIONS ON THE         0000000 
C     ARRAYS A, ICN AND IRN.                                            0000000 
C IACTIV IS THE FIRST POSITION IN ARRAYS A/ICN FROM WHICH THE COMPRESS  0000000 
C     STARTS.  ON EXIT, IACTIV EQUALS THE POSITION OF THE FIRST ENTRY   0000000 
C     IN THE COMPRESSED PART OF A/ICN                                   0000000 
C                                                                       0000000 
      DOUBLE PRECISION A(ITOP)                                                  
      LOGICAL REALS                                                     0000000 
      INTEGER IPTR(N)                                                   0000000 
Change
C      INTEGER*2 ICN(ITOP)                                                       
      INTEGER ICN(ITOP)                                                       
Change
C SEE BLOCK DATA FOR COMMENTS ON VARIABLES IN COMMON.                           
      COMMON /MA30FD/ IRNCP, ICNCP, IRANK, MINIRN, MINICN                       
C                                                                       0000000 
      IF (REALS) ICNCP = ICNCP + 1                                      0000000 
      IF (.NOT.REALS) IRNCP = IRNCP + 1                                 0000000 
C SET THE FIRST NON-ZERO ENTRY IN EACH ROW TO THE NEGATIVE OF THE       0000000 
C     ROW/COL NUMBER AND HOLD THIS ROW/COL INDEX IN THE ROW/COL         0000000 
C     POINTER.  THIS IS SO THAT THE BEGINNING OF EACH ROW/COL CAN       0000000 
C     BE RECOGNIZED IN THE SUBSEQUENT SCAN.                             0000000 
      DO 10 J=1,N                                                       0000000 
        K = IPTR(J)                                                     0000000 
        IF (K.LT.IACTIV) GO TO 10                                       0000000 
        IPTR(J) = ICN(K)                                                0000000 
        ICN(K) = -J                                                     0000000 
   10 CONTINUE                                                          0000000 
      KN = ITOP + 1                                                     0000000 
      KL = ITOP - IACTIV + 1                                            0000000 
C GO THROUGH ARRAYS IN REVERSE ORDER COMPRESSING TO THE BACK SO         0000000 
C     THAT THERE ARE NO ZEROS HELD IN POSITIONS IACTIV TO ITOP IN ICN.  0000000 
C     RESET FIRST ENTRY OF EACH ROW/COL AND POINTER ARRAY IPTR.         0000000 
      DO 30 K=1,KL                                                      0000000 
        JPOS = ITOP - K + 1                                             0000000 
        IF (ICN(JPOS).EQ.0) GO TO 30                                    0000000 
        KN = KN - 1                                                     0000000 
        IF (REALS) A(KN) = A(JPOS)                                      0000000 
        IF (ICN(JPOS).GE.0) GO TO 20                                    0000000 
C FIRST NON-ZERO OF ROW/COL HAS BEEN LOCATED                            0000000 
        J = -ICN(JPOS)                                                  0000000 
        ICN(JPOS) = IPTR(J)                                             0000000 
        IPTR(J) = KN                                                    0000000 
   20   ICN(KN) = ICN(JPOS)                                             0000000 
   30 CONTINUE                                                          0000000 
      IACTIV = KN                                                       0000000 
      RETURN                                                            0000000 
      END                                                               0000000 
      SUBROUTINE MA30BD(N, ICN, A, LICN, LENR, LENRL, IDISP, IP, IQ, W,         
     * IW, IFLAG)                                                       0000000 
c_270390
      EXTERNAL MA30$DATA
c_270390
C MA30B/BD PERFORMS THE LU DECOMPOSITION OF THE DIAGONAL BLOCKS OF A    0000000 
C     NEW MATRIX PAQ OF THE SAME SPARSITY PATTERN, USING INFORMATION    0000000 
C     FROM A PREVIOUS CALL TO MA30A/AD. THE ENTRIES OF THE INPUT        0000000 
C     MATRIX  MUST ALREADY BE IN THEIR FINAL POSITIONS IN THE LU        0000000 
C     DECOMPOSITION STRUCTURE.  THIS ROUTINE EXECUTES ABOUT FIVE TIMES  0000000 
C     FASTER THAN MA30A/AD.                                             0000000 
C                                                                       0000000 
C WE NOW DESCRIBE THE ARGUMENT LIST FOR MA30B/BD. CONSULT MA30A/AD FOR  0000000 
C     FURTHER INFORMATION ON THESE PARAMETERS.                          0000000 
C N  IS AN INTEGER VARIABLE SET TO THE ORDER OF THE MATRIX.             0000000 
C ICN IS AN INTEGER*2 ARRAY OF LENGTH LICN. IT SHOULD BE UNCHANGED              
C     SINCE THE LAST CALL TO MA30A/AD. IT IS NOT ALTERED BY MA30B/BD.   0000000 
C A  IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH LICN THE USER MUST SET  0000000 
C     ENTRIES IDISP(1) TO IDISP(2) TO CONTAIN THE ENTRIES IN THE        0000000 
C     DIAGONAL BLOCKS OF THE MATRIX PAQ WHOSE COLUMN NUMBERS ARE HELD   0000000 
C     IN ICN, USING CORRESPONDING POSITIONS. NOTE THAT SOME ZEROS MAY   0000000 
C     NEED TO BE HELD EXPLICITLY. ON OUTPUT ENTRIES IDISP(1) TO         0000000 
C     IDISP(2) OF ARRAY A CONTAIN THE LU DECOMPOSITION OF THE DIAGONAL  0000000 
C     BLOCKS OF PAQ. ENTRIES A(1) TO A(IDISP(1)-1) ARE NEITHER          0000000 
C     REQUIRED NOR ALTERED BY MA30B/BD.                                 0000000 
C LICN  IS AN INTEGER VARIABLE WHICH MUST BE SET BY THE USER TO THE     0000000 
C     LENGTH OF ARRAYS A AND ICN. IT IS NOT ALTERED BY MA30B/BD.        0000000 
C LENR,LENRL ARE INTEGER*2 ARRAYS OF LENGTH N. THEY SHOULD BE                   
C     UNCHANGED SINCE THE LAST CALL TO MA30A/AD. THEY ARE NOT ALTERED   0000000 
C     BY MA30B/BD.                                                      0000000 
C IDISP  IS AN INTEGER ARRAY OF LENGTH 2. IT SHOULD BE UNCHANGED SINCE  0000000 
C     THE LAST CALL TO MA30A/AD. IT IS NOT ALTERED BY MA30B/BD.         0000000 
C IP,IQ  ARE INTEGER*2  ARRAYS OF LENGTH N. THEY SHOULD BE UNCHANGED            
C     SINCE THE LAST CALL TO MA30A/AD. THEY ARE NOT ALTERED BY          0000000 
C     MA30B/BD.                                                         0000000 
C W  IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH N WHICH IS USED AS      0000000 
C     WORKSPACE BY MA30B/BD.                                            0000000 
C IW  IS AN INTEGER ARRAY OF LENGTH N WHICH IS USED AS WORKSPACE BY     0000000 
C     MA30B/BD.                                                         0000000 
C IFLAG  IS AN INTEGER VARIABLE. ON OUTPUT FROM MA30B/BD, IFLAG HAS     0000000 
C     THE VALUE ZERO IF THE FACTORIZATION WAS SUCCESSFUL, HAS THE       0000000 
C     VALUE I IF PIVOT I WAS VERY SMALL AND HAS THE VALUE -I IF AN      0000000 
C     UNEXPECTED SINGULARITY WAS DETECTED AT STAGE I OF THE             0000000 
C     DECOMPOSITION.                                                    0000000 
C                                                                       0000000 
      DOUBLE PRECISION A(LICN), W(N), AU, EPS, ROWMAX, ZERO, ONE, RMIN,         
     * TOL, BIG                                                                 
      LOGICAL ABORT1, ABORT2, ABORT3, STAB, LBIG                        0000000 
      INTEGER IW(N), IDISP(2), PIVPOS                                   0000000 
Change
C      INTEGER*2 ICN(LICN), LENR(N), LENRL(N), IP(N), IQ(N)                      
      INTEGER ICN(LICN), LENR(N), LENRL(N), IP(N), IQ(N)                      
Change
C SEE BLOCK DATA FOR COMMENTS ON VARIABLES IN COMMON.                           
      COMMON /MA30ED/ LP, ABORT1, ABORT2, ABORT3                                
      COMMON /MA30ID/ TOL, BIG, NDROP, NSRCH, LBIG                              
      COMMON /MA30GD/ EPS, RMIN                                                 
      DATA ZERO /0.0D0/, ONE /1.0D0/                                            
      STAB = EPS.LE.ONE                                                 0000000 
      RMIN = EPS                                                        0000000 
      ISING = 0                                                         0000000 
      IFLAG = 0                                                         0000000 
      DO 10 I=1,N                                                       0000000 
        W(I) = ZERO                                                     0000000 
   10 CONTINUE                                                          0000000 
C SET UP POINTERS TO THE BEGINNING OF THE ROWS.                         0000000 
      IW(1) = IDISP(1)                                                  0000000 
      IF (N.EQ.1) GO TO 25                                              0000000 
      DO 20 I=2,N                                                       0000000 
        IW(I) = IW(I-1) + LENR(I-1)                                     0000000 
   20 CONTINUE                                                          0000000 
C                                                                       0000000 
C   ****   START  OF MAIN LOOP    ****                                  0000000 
C AT STEP I, ROW I OF A IS TRANSFORMED TO ROW I OF L/U BY ADDING        0000000 
C     APPROPRIATE MULTIPLES OF ROWS 1 TO I-1.                           0000000 
C     .... USING ROW-GAUSS ELIMINATION.                                 0000000 
   25 DO 160 I=1,N                                                      0000000 
C ISTART IS BEGINNING OF ROW I OF A AND ROW I OF L.                     0000000 
        ISTART = IW(I)                                                  0000000 
C IFIN IS END OF ROW I OF A AND ROW I OF U.                             0000000 
        IFIN = ISTART + LENR(I) - 1                                     0000000 
C ILEND IS END OF ROW I OF L.                                           0000000 
        ILEND = ISTART + LENRL(I) - 1                                   0000000 
        IF (ISTART.GT.ILEND) GO TO 90                                   0000000 
C LOAD ROW I OF A INTO VECTOR W.                                        0000000 
        DO 30 JJ=ISTART,IFIN                                            0000000 
          J = ICN(JJ)                                                   0000000 
          W(J) = A(JJ)                                                  0000000 
   30   CONTINUE                                                        0000000 
C                                                                       0000000 
C ADD MULTIPLES OF APPROPRIATE ROWS OF  I TO I-1  TO ROW I.             0000000 
        DO 70 JJ=ISTART,ILEND                                           0000000 
          J = ICN(JJ)                                                   0000000 
C IPIVJ IS POSITION OF PIVOT IN ROW J.                                  0000000 
          IPIVJ = IW(J) + LENRL(J)                                      0000000 
C FORM MULTIPLIER AU.                                                   0000000 
          AU = -W(J)/A(IPIVJ)                                           0000000 
          IF (LBIG) BIG = DMAX1(DABS(AU),BIG)                                   
          W(J) = AU                                                     0000000 
C AU * ROW J (U PART) IS ADDED TO ROW I.                                0000000 
          IPIVJ = IPIVJ + 1                                             0000000 
          JFIN = IW(J) + LENR(J) - 1                                    0000000 
          IF (IPIVJ.GT.JFIN) GO TO 70                                   0000000 
C INNERMOST LOOP.                                                       0000000 
          IF (LBIG) GO TO 50                                            0000000 
          DO 40 JAYJAY=IPIVJ,JFIN                                       0000000 
            JAY = ICN(JAYJAY)                                           0000000 
            W(JAY) = W(JAY) + AU*A(JAYJAY)                              0000000 
   40     CONTINUE                                                      0000000 
          GO TO 70                                                      0000000 
   50     DO 60 JAYJAY=IPIVJ,JFIN                                       0000000 
            JAY = ICN(JAYJAY)                                           0000000 
            W(JAY) = W(JAY) + AU*A(JAYJAY)                              0000000 
            BIG = DMAX1(DABS(W(JAY)),BIG)                                       
   60     CONTINUE                                                      0000000 
   70   CONTINUE                                                        0000000 
C                                                                       0000000 
C RELOAD W BACK INTO A (NOW L/U)                                        0000000 
        DO 80 JJ=ISTART,IFIN                                            0000000 
          J = ICN(JJ)                                                   0000000 
          A(JJ) = W(J)                                                  0000000 
          W(J) = ZERO                                                   0000000 
   80   CONTINUE                                                        0000000 
C WE NOW PERFORM THE STABILITY CHECKS.                                  0000000 
   90   PIVPOS = ILEND + 1                                              0000000 
        IF (IQ(I).GT.0) GO TO 140                                       0000000 
C MATRIX HAD SINGULARITY AT THIS POINT IN MA30A/AD.                     0000000 
C IS IT THE FIRST SUCH PIVOT IN CURRENT BLOCK ?                         0000000 
        IF (ISING.EQ.0) ISING = I                                       0000000 
C DOES CURRENT MATRIX HAVE A SINGULARITY IN THE SAME PLACE ?            0000000 
        IF (PIVPOS.GT.IFIN) GO TO 100                                   0000000 
        IF (A(PIVPOS).NE.ZERO) GO TO 170                                0000000 
C IT DOES .. SO SET ISING IF IT IS NOT THE END OF THE CURRENT BLOCK     0000000 
C CHECK TO SEE THAT APPROPRIATE PART OF L/U IS ZERO OR NULL.            0000000 
  100   IF (ISTART.GT.IFIN) GO TO 120                                   0000000 
        DO 110 JJ=ISTART,IFIN                                           0000000 
          IF (ICN(JJ).LT.ISING) GO TO 110                               0000000 
          IF (A(JJ).NE.ZERO) GO TO 170                                  0000000 
  110   CONTINUE                                                        0000000 
  120   IF (PIVPOS.LE.IFIN) A(PIVPOS) = ONE                             0000000 
        IF (IP(I).GT.0 .AND. I.NE.N) GO TO 160                          0000000 
C END OF CURRENT BLOCK ... RESET ZERO PIVOTS AND ISING.                 0000000 
        DO 130 J=ISING,I                                                0000000 
          IF ((LENR(J)-LENRL(J)).EQ.0) GO TO 130                        0000000 
          JJ = IW(J) + LENRL(J)                                         0000000 
          A(JJ) = ZERO                                                  0000000 
  130   CONTINUE                                                        0000000 
        ISING = 0                                                       0000000 
        GO TO 160                                                       0000000 
C MATRIX HAD NON-ZERO PIVOT IN MA30A/AD AT THIS STAGE.                  0000000 
  140   IF (PIVPOS.GT.IFIN) GO TO 170                                   0000000 
        IF (A(PIVPOS).EQ.ZERO) GO TO 170                                0000000 
        IF (.NOT.STAB) GO TO 160                                        0000000 
        ROWMAX = ZERO                                                   0000000 
        DO 150 JJ=PIVPOS,IFIN                                           0000000 
          ROWMAX = DMAX1(ROWMAX,DABS(A(JJ)))                                    
  150   CONTINUE                                                        0000000 
        IF (DABS(A(PIVPOS))/ROWMAX.GE.RMIN) GO TO 160                           
        IFLAG = I                                                       0000000 
        RMIN = DABS(A(PIVPOS))/ROWMAX                                           
C   ****    END OF MAIN LOOP    ****                                    0000000 
  160 CONTINUE                                                          0000000 
C                                                                       0000000 
      GO TO 180                                                         0000000 
C   ***   ERROR RETURN   ***                                            0000000 
  170 IF (LP.NE.0) WRITE (LP,99999) I                                   0000000 
      IFLAG = -I                                                        0000000 
C                                                                       0000000 
  180 RETURN                                                            0000000 
99999 FORMAT (54H ERROR RETURN FROM MA30B/BD SINGULARITY DETECTED IN RO,0000000 
     * 1HW, I8)                                                         0000000 
      END                                                               0000000 
      SUBROUTINE MA30CD(N, ICN, A, LICN, LENR, LENRL, LENOFF, IDISP, IP,        
     * IQ, X, W, MTYPE)                                                 0000000 
c_270390
      EXTERNAL MA30$DATA
c_270390
C MA30C/CD USES THE FACTORS PRODUCED BY MA30A/AD OR MA30B/BD TO SOLVE   0000000 
C     AX=B OR A TRANSPOSE X=B WHEN THE MATRIX P1*A*Q1 (PAQ) IS BLOCK    0000000 
C     LOWER TRIANGULAR (INCLUDING THE CASE OF ONLY ONE DIAGONAL         0000000 
C     BLOCK).                                                           0000000 
C                                                                       0000000 
C WE NOW DESCRIBE THE ARGUMENT LIST FOR MA30C/CD.                       0000000 
C N  IS AN INTEGER VARIABLE SET TO THE ORDER OF THE MATRIX. IT IS NOT   0000000 
C     ALTERED BY THE SUBROUTINE.                                        0000000 
C ICN IS AN INTEGER*2  ARRAY OF LENGTH LICN. ENTRIES IDISP(1) TO                
C     IDISP(2) SHOULD BE UNCHANGED SINCE THE LAST CALL TO MA30A/AD. IF  0000000 
C     THE MATRIX HAS MORE THAN ONE DIAGONAL BLOCK, THEN COLUMN INDICES  0000000 
C     CORRESPONDING TO NON-ZEROS IN SUB-DIAGONAL BLOCKS OF PAQ MUST     0000000 
C     APPEAR IN POSITIONS 1 TO IDISP(1)-1. FOR THE SAME ROW THOSE       0000000 
C     ENTRIES MUST BE CONTIGUOUS, WITH THOSE IN ROW I PRECEDING THOSE   0000000 
C     IN ROW I+1 (I=1,...,N-1) AND NO WASTED SPACE BETWEEN ROWS.        0000000 
C     ENTRIES MAY BE IN ANY ORDER WITHIN EACH ROW. IT IS NOT ALTERED    0000000 
C     BY MA30C/CD.                                                      0000000 
C A  IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH LICN.  ENTRIES          0000000 
C     IDISP(1) TO IDISP(2) SHOULD BE UNCHANGED SINCE THE LAST CALL TO   0000000 
C     MA30A/AD OR MA30B/BD.  IF THE MATRIX HAS MORE THAN ONE DIAGONAL   0000000 
C     BLOCK, THEN THE VALUES OF THE NON-ZEROS IN SUB-DIAGONAL BLOCKS    0000000 
C     MUST BE IN POSITIONS 1 TO IDISP(1)-1 IN THE ORDER GIVEN BY ICN.   0000000 
C     IT IS NOT ALTERED BY MA30C/CD.                                    0000000 
C LICN  IS AN INTEGER VARIABLE SET TO THE SIZE OF ARRAYS ICN AND A.     0000000 
C     IT IS NOT ALTERED BY MA30C/CD.                                    0000000 
C LENR,LENRL ARE INTEGER*2  ARRAYS OF LENGTH N WHICH SHOULD BE                  
C     UNCHANGED SINCE THE LAST CALL TO MA30A/AD. THEY ARE NOT ALTERED   0000000 
C     BY MA30C/CD.                                                      0000000 
C LENOFF  IS AN INTEGER*2  ARRAY OF LENGTH N. IF THE MATRIX PAQ (OR             
C     P1*A*Q1) HAS MORE THAN ONE DIAGONAL BLOCK, THEN LENOFF(I),        0000000 
C     I=1,...,N SHOULD BE SET TO THE NUMBER OF NON-ZEROS IN ROW I OF    0000000 
C     THE MATRIX PAQ WHICH ARE IN SUB-DIAGONAL BLOCKS.  IF THERE IS     0000000 
C     ONLY ONE DIAGONAL BLOCK THEN LENOFF(1) MAY BE SET TO -1, IN       0000000 
C     WHICH CASE THE OTHER ENTRIES OF LENOFF ARE NEVER ACCESSED. IT IS  0000000 
C     NOT ALTERED BY MA30C/CD.                                          0000000 
C IDISP  IS AN INTEGER ARRAY OF LENGTH 2 WHICH SHOULD BE UNCHANGED      0000000 
C     SINCE THE LAST CALL TO MA30A/AD. IT IS NOT ALTERED BY MA30C/CD.   0000000 
C IP,IQ ARE INTEGER*2 ARRAYS OF LENGTH N WHICH SHOULD BE UNCHANGED              
C     SINCE THE LAST CALL TO MA30A/AD. THEY ARE NOT ALTERED BY          0000000 
C     MA30C/CD.                                                         0000000 
C X IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH N. IT MUST BE SET BY     0000000 
C     THE USER TO THE VALUES OF THE RIGHT HAND SIDE VECTOR B FOR THE    0000000 
C     EQUATIONS BEING SOLVED.  ON EXIT FROM MA30C/CD IT WILL BE EQUAL   0000000 
C     TO THE SOLUTION X REQUIRED.                                       0000000 
C W  IS A REAL/DOUBLE PRECISION ARRAY OF LENGTH N WHICH IS USED AS      0000000 
C     WORKSPACE BY MA30C/CD.                                            0000000 
C MTYPE IS AN INTEGER VARIABLE WHICH MUST BE SET BY THE USER. IF        0000000 
C     MTYPE=1, THEN THE SOLUTION TO THE SYSTEM AX=B IS RETURNED; ANY    0000000 
C     OTHER VALUE FOR MTYPE WILL RETURN THE SOLUTION TO THE SYSTEM A    0000000 
C     TRANSPOSE X=B. IT IS NOT ALTERED BY MA30C/CD.                     0000000 
C                                                                       0000000 
      DOUBLE PRECISION A(LICN), X(N), W(N), WII, WI, RESID, ZERO                
      LOGICAL NEG, NOBLOC                                               0000000 
      INTEGER IDISP(2)                                                  0000000 
Change
C      INTEGER*2 ICN(LICN), LENR(N), LENRL(N), LENOFF(N), IP(N), IQ(N)           
      INTEGER ICN(LICN), LENR(N), LENRL(N), LENOFF(N), IP(N), IQ(N)           
Change
C SEE BLOCK DATA FOR COMMENTS ON VARIABLES IN COMMON.                           
      COMMON /MA30HD/ RESID                                                     
      DATA ZERO /0.0D0/                                                         
C                                                                       0000000 
C THE FINAL VALUE OF RESID IS THE MAXIMUM RESIDUAL FOR AN INCONSISTENT  0000000 
C     SET OF EQUATIONS.                                                 0000000 
      RESID = ZERO                                                      0000000 
C NOBLOC IS .TRUE. IF SUBROUTINE BLOCK HAS BEEN USED PREVIOUSLY AND     0000000 
C     IS .FALSE. OTHERWISE.  THE VALUE .FALSE. MEANS THAT LENOFF        0000000 
C     WILL NOT BE SUBSEQUENTLY ACCESSED.                                0000000 
      NOBLOC = LENOFF(1).LT.0                                           0000000 
      IF (MTYPE.NE.1) GO TO 140                                         0000000 
C                                                                       0000000 
C WE NOW SOLVE   A * X = B.                                             0000000 
C NEG IS USED TO INDICATE WHEN THE LAST ROW IN A BLOCK HAS BEEN         0000000 
C     REACHED.  IT IS THEN SET TO TRUE WHEREAFTER BACKSUBSTITUTION IS   0000000 
C     PERFORMED ON THE BLOCK.                                           0000000 
      NEG = .FALSE.                                                     0000000 
C IP(N) IS NEGATED SO THAT THE LAST ROW OF THE LAST BLOCK CAN BE        0000000 
C     RECOGNISED.  IT IS RESET TO ITS POSITIVE VALUE ON EXIT.           0000000 
      IP(N) = -IP(N)                                                    0000000 
C PREORDER VECTOR ... W(I) = X(IP(I))                                   0000000 
      DO 10 II=1,N                                                      0000000 
        I = IP(II)                                                      0000000 
        I = IABS(I)                                                     0000000 
        W(II) = X(I)                                                    0000000 
   10 CONTINUE                                                          0000000 
C LT HOLDS THE POSITION OF THE FIRST NON-ZERO IN THE CURRENT ROW OF THE 0000000 
C     OFF-DIAGONAL BLOCKS.                                              0000000 
      LT = 1                                                            0000000 
C IFIRST HOLDS THE INDEX OF THE FIRST ROW IN THE CURRENT BLOCK.         0000000 
      IFIRST = 1                                                        0000000 
C IBLOCK HOLDS THE POSITION OF THE FIRST NON-ZERO IN THE CURRENT ROW    0000000 
C     OF THE LU DECOMPOSITION OF THE DIAGONAL BLOCKS.                   0000000 
      IBLOCK = IDISP(1)                                                 0000000 
C IF I IS NOT THE LAST ROW OF A BLOCK, THEN A PASS THROUGH THIS LOOP    0000000 
C     ADDS THE INNER PRODUCT OF ROW I OF THE OFF-DIAGONAL BLOCKS AND W  0000000 
C     TO W AND PERFORMS FORWARD ELIMINATION USING ROW I OF THE LU       0000000 
C     DECOMPOSITION.   IF I IS THE LAST ROW OF A BLOCK THEN, AFTER      0000000 
C     PERFORMING THESE AFOREMENTIONED OPERATIONS, BACKSUBSTITUTION IS   0000000 
C     PERFORMED USING THE ROWS OF THE BLOCK.                            0000000 
      DO 120 I=1,N                                                      0000000 
        WI = W(I)                                                       0000000 
        IF (NOBLOC) GO TO 30                                            0000000 
        IF (LENOFF(I).EQ.0) GO TO 30                                    0000000 
C OPERATIONS USING LOWER TRIANGULAR BLOCKS.                             0000000 
C LTEND IS THE END OF ROW I IN THE OFF-DIAGONAL BLOCKS.                 0000000 
        LTEND = LT + LENOFF(I) - 1                                      0000000 
        DO 20 JJ=LT,LTEND                                               0000000 
          J = ICN(JJ)                                                   0000000 
          WI = WI - A(JJ)*W(J)                                          0000000 
   20   CONTINUE                                                        0000000 
C LT IS SET THE BEGINNING OF THE NEXT OFF-DIAGONAL ROW.                 0000000 
        LT = LTEND + 1                                                  0000000 
C SET NEG TO .TRUE. IF WE ARE ON THE LAST ROW OF THE BLOCK.             0000000 
   30   IF (IP(I).LT.0) NEG = .TRUE.                                    0000000 
        IF (LENRL(I).EQ.0) GO TO 50                                     0000000 
C FORWARD ELIMINATION PHASE.                                            0000000 
C IEND IS THE END OF THE L PART OF ROW I IN THE LU DECOMPOSITION.       0000000 
        IEND = IBLOCK + LENRL(I) - 1                                    0000000 
        DO 40 JJ=IBLOCK,IEND                                            0000000 
          J = ICN(JJ)                                                   0000000 
          WI = WI + A(JJ)*W(J)                                          0000000 
   40   CONTINUE                                                        0000000 
C IBLOCK IS ADJUSTED TO POINT TO THE START OF THE NEXT ROW.             0000000 
   50   IBLOCK = IBLOCK + LENR(I)                                       0000000 
        W(I) = WI                                                       0000000 
        IF (.NOT.NEG) GO TO 120                                         0000000 
C BACK SUBSTITUTION PHASE.                                              0000000 
C J1 IS POSITION IN A/ICN AFTER END OF BLOCK BEGINNING IN ROW IFIRST    0000000 
C     AND ENDING IN ROW I.                                              0000000 
        J1 = IBLOCK                                                     0000000 
C ARE THERE ANY SINGULARITIES IN THIS BLOCK?  IF NOT, CONTINUE WITH     0000000 
C     THE BACKSUBSTITUTION.                                             0000000 
        IB = I                                                          0000000 
        IF (IQ(I).GT.0) GO TO 70                                        0000000 
        DO 60 III=IFIRST,I                                              0000000 
          IB = I - III + IFIRST                                         0000000 
          IF (IQ(IB).GT.0) GO TO 70                                     0000000 
          J1 = J1 - LENR(IB)                                            0000000 
          RESID = DMAX1(RESID,DABS(W(IB)))                                      
          W(IB) = ZERO                                                  0000000 
   60   CONTINUE                                                        0000000 
C ENTIRE BLOCK IS SINGULAR.                                             0000000 
        GO TO 110                                                       0000000 
C EACH PASS THROUGH THIS LOOP PERFORMS THE BACK-SUBSTITUTION            0000000 
C     OPERATIONS FOR A SINGLE ROW, STARTING AT THE END OF THE BLOCK AND 0000000 
C     WORKING THROUGH IT IN REVERSE ORDER.                              0000000 
   70   DO 100 III=IFIRST,IB                                            0000000 
          II = IB - III + IFIRST                                        0000000 
C J2 IS END OF ROW II.                                                  0000000 
          J2 = J1 - 1                                                   0000000 
C J1 IS BEGINNING OF ROW II.                                            0000000 
          J1 = J1 - LENR(II)                                            0000000 
C JPIV IS THE POSITION OF THE PIVOT IN ROW II.                          0000000 
          JPIV = J1 + LENRL(II)                                         0000000 
          JPIVP1 = JPIV + 1                                             0000000 
C JUMP IF ROW  II OF U HAS NO NON-ZEROS.                                0000000 
          IF (J2.LT.JPIVP1) GO TO 90                                    0000000 
          WII = W(II)                                                   0000000 
          DO 80 JJ=JPIVP1,J2                                            0000000 
            J = ICN(JJ)                                                 0000000 
            WII = WII - A(JJ)*W(J)                                      0000000 
   80     CONTINUE                                                      0000000 
          W(II) = WII                                                   0000000 
   90     W(II) = W(II)/A(JPIV)                                         0000000 
  100   CONTINUE                                                        0000000 
  110   IFIRST = I + 1                                                  0000000 
        NEG = .FALSE.                                                   0000000 
  120 CONTINUE                                                          0000000 
C                                                                       0000000 
C REORDER SOLUTION VECTOR ... X(I) = W(IQINVERSE(I))                    0000000 
      DO 130 II=1,N                                                     0000000 
        I = IQ(II)                                                      0000000 
        I = IABS(I)                                                     0000000 
        X(I) = W(II)                                                    0000000 
  130 CONTINUE                                                          0000000 
      IP(N) = -IP(N)                                                    0000000 
      GO TO 320                                                         0000000 
C                                                                       0000000 
C                                                                       0000000 
C WE NOW SOLVE   ATRANSPOSE * X = B.                                    0000000 
C PREORDER VECTOR ... W(I)=X(IQ(I))                                     0000000 
  140 DO 150 II=1,N                                                     0000000 
        I = IQ(II)                                                      0000000 
        I = IABS(I)                                                     0000000 
        W(II) = X(I)                                                    0000000 
  150 CONTINUE                                                          0000000 
C LJ1 POINTS TO THE BEGINNING THE CURRENT ROW IN THE OFF-DIAGONAL       0000000 
C     BLOCKS.                                                           0000000 
      LJ1 = IDISP(1)                                                    0000000 
C IBLOCK IS INITIALIZED TO POINT TO THE BEGINNING OF THE BLOCK AFTER    0000000 
C     THE LAST ONE ]                                                    0000000 
      IBLOCK = IDISP(2) + 1                                             0000000 
C ILAST IS THE LAST ROW IN THE CURRENT BLOCK.                           0000000 
      ILAST = N                                                         0000000 
C IBLEND POINTS TO THE POSITION AFTER THE LAST NON-ZERO IN THE          0000000 
C     CURRENT BLOCK.                                                    0000000 
      IBLEND = IBLOCK                                                   0000000 
C EACH PASS THROUGH THIS LOOP OPERATES WITH ONE DIAGONAL BLOCK AND      0000000 
C     THE OFF-DIAGONAL PART OF THE MATRIX CORRESPONDING TO THE ROWS     0000000 
C     OF THIS BLOCK.  THE BLOCKS ARE TAKEN IN REVERSE ORDER AND THE     0000000 
C     NUMBER OF TIMES THE LOOP IS ENTERED IS MIN(N,NO. BLOCKS+1).       0000000 
      DO 290 NUMBLK=1,N                                                 0000000 
        IF (ILAST.EQ.0) GO TO 300                                       0000000 
        IBLOCK = IBLOCK - LENR(ILAST)                                   0000000 
C THIS LOOP FINDS THE INDEX OF THE FIRST ROW IN THE CURRENT BLOCK..     0000000 
C     IT IS FIRST AND IBLOCK IS SET TO THE POSITION OF THE BEGINNING    0000000 
C     OF THIS FIRST ROW.                                                0000000 
        DO 160 K=1,N                                                    0000000 
          II = ILAST - K                                                0000000 
          IF (II.EQ.0) GO TO 170                                        0000000 
          IF (IP(II).LT.0) GO TO 170                                    0000000 
          IBLOCK = IBLOCK - LENR(II)                                    0000000 
  160   CONTINUE                                                        0000000 
  170   IFIRST = II + 1                                                 0000000 
C J1 POINTS TO THE POSITION OF THE BEGINNING OF ROW I (LT PART) OR PIVOT0000000 
        J1 = IBLOCK                                                     0000000 
C FORWARD ELIMINATION.                                                  0000000 
C EACH PASS THROUGH THIS LOOP PERFORMS THE OPERATIONS FOR ONE ROW OF THE0000000 
C     BLOCK.  IF THE CORRESPONDING ENTRY OF W IS ZERO THEN THE          0000000 
C     OPERATIONS CAN BE AVOIDED.                                        0000000 
        DO 210 I=IFIRST,ILAST                                           0000000 
          IF (W(I).EQ.ZERO) GO TO 200                                   0000000 
C JUMP IF ROW I SINGULAR.                                               0000000 
          IF (IQ(I).LT.0) GO TO 220                                     0000000 
C J2 FIRST POINTS TO THE PIVOT IN ROW I AND THEN IS MADE TO POINT TO THE0000000 
C     FIRST NON-ZERO IN THE U TRANSPOSE PART OF THE ROW.                0000000 
          J2 = J1 + LENRL(I)                                            0000000 
          WI = W(I)/A(J2)                                               0000000 
          IF (LENR(I)-LENRL(I).EQ.1) GO TO 190                          0000000 
          J2 = J2 + 1                                                   0000000 
C J3 POINTS TO THE END OF ROW I.                                        0000000 
          J3 = J1 + LENR(I) - 1                                         0000000 
          DO 180 JJ=J2,J3                                               0000000 
            J = ICN(JJ)                                                 0000000 
            W(J) = W(J) - A(JJ)*WI                                      0000000 
  180     CONTINUE                                                      0000000 
  190     W(I) = WI                                                     0000000 
  200     J1 = J1 + LENR(I)                                             0000000 
  210   CONTINUE                                                        0000000 
        GO TO 240                                                       0000000 
C DEALS WITH REST OF BLOCK WHICH IS SINGULAR.                           0000000 
  220   DO 230 II=I,ILAST                                               0000000 
          RESID = DMAX1(RESID,DABS(W(II)))                                      
          W(II) = ZERO                                                  0000000 
  230   CONTINUE                                                        0000000 
C BACK SUBSTITUTION.                                                    0000000 
C THIS LOOP DOES THE BACK SUBSTITUTION ON THE ROWS OF THE BLOCK IN      0000000 
C     THE REVERSE ORDER DOING IT SIMULTANEOUSLY ON THE L TRANSPOSE PART 0000000 
C     OF THE DIAGONAL BLOCKS AND THE OFF-DIAGONAL BLOCKS.               0000000 
  240   J1 = IBLEND                                                     0000000 
        DO 280 IBACK=IFIRST,ILAST                                       0000000 
          I = ILAST - IBACK + IFIRST                                    0000000 
C J1 POINTS TO THE BEGINNING OF ROW I.                                  0000000 
          J1 = J1 - LENR(I)                                             0000000 
          IF (LENRL(I).EQ.0) GO TO 260                                  0000000 
C J2 POINTS TO THE END OF THE L TRANSPOSE PART OF ROW I.                0000000 
          J2 = J1 + LENRL(I) - 1                                        0000000 
          DO 250 JJ=J1,J2                                               0000000 
            J = ICN(JJ)                                                 0000000 
            W(J) = W(J) + A(JJ)*W(I)                                    0000000 
  250     CONTINUE                                                      0000000 
  260     IF (NOBLOC) GO TO 280                                         0000000 
C OPERATIONS USING LOWER TRIANGULAR BLOCKS.                             0000000 
          IF (LENOFF(I).EQ.0) GO TO 280                                 0000000 
C LJ2 POINTS TO THE END OF ROW I OF THE OFF-DIAGONAL BLOCKS.            0000000 
          LJ2 = LJ1 - 1                                                 0000000 
C LJ1 POINTS TO THE BEGINNING OF ROW I OF THE OFF-DIAGONAL BLOCKS.      0000000 
          LJ1 = LJ1 - LENOFF(I)                                         0000000 
          DO 270 JJ=LJ1,LJ2                                             0000000 
            J = ICN(JJ)                                                 0000000 
            W(J) = W(J) - A(JJ)*W(I)                                    0000000 
  270     CONTINUE                                                      0000000 
  280   CONTINUE                                                        0000000 
        IBLEND = J1                                                     0000000 
        ILAST = IFIRST - 1                                              0000000 
  290 CONTINUE                                                          0000000 
C REORDER SOLUTION VECTOR ... X(I)=W(IPINVERSE(I))                      0000000 
  300 DO 310 II=1,N                                                     0000000 
        I = IP(II)                                                      0000000 
        I = IABS(I)                                                     0000000 
        X(I) = W(II)                                                    0000000 
  310 CONTINUE                                                          0000000 
C                                                                       0000000 
  320 RETURN                                                            0000000 
      END                                                               0000000 
      BLOCK DATA ma30$data
C_270390ak      BLOCK DATA                                                        0000000 
C ALTHOUGH ALL COMMON BLOCK VARIABLES DO NOT HAVE DEFAULT VALUES,       0000000 
C     WE COMMENT ON ALL THE COMMON BLOCK VARIABLES HERE.                0000000 
C                                                                       0000000 
C COMMON BLOCK MA30E/ED HOLDS CONTROL PARAMETERS ....                   0000000 
C     COMMON /MA30ED/ LP, ABORT1, ABORT2, ABORT3                        0000000 
C THE INTEGER LP IS THE UNIT NUMBER TO WHICH THE ERROR MESSAGES ARE     0000000 
C     SENT. LP HAS A DEFAULT VALUE OF 6.  THIS DEFAULT VALUE CAN BE     0000000 
C     RESET BY THE USER, IF DESIRED.  A VALUE OF 0 SUPPRESSES ALL       0000000 
C     MESSAGES.                                                         0000000 
C THE LOGICAL VARIABLES ABORT1,ABORT2,ABORT3 ARE USED TO CONTROL THE    0000000 
C     CONDITIONS UNDER WHICH THE SUBROUTINE WILL TERMINATE.             0000000 
C IF ABORT1 IS .TRUE. THEN THE SUBROUTINE WILL EXIT  IMMEDIATELY ON     0000000 
C     DETECTING STRUCTURAL SINGULARITY.                                 0000000 
C IF ABORT2 IS .TRUE. THEN THE SUBROUTINE WILL EXIT IMMEDIATELY ON      0000000 
C     DETECTING NUMERICAL SINGULARITY.                                  0000000 
C IF ABORT3 IS .TRUE. THEN THE SUBROUTINE WILL EXIT IMMEDIATELY WHEN    0000000 
C     THE AVAILABLE SPACE IN A/ICN IS FILLED UP BY THE PREVIOUSLY       0000000 
C     DECOMPOSED, ACTIVE, AND UNDECOMPOSED PARTS OF THE MATRIX.         0000000 
C THE DEFAULT VALUES FOR ABORT1,ABORT2,ABORT3 ARE SET TO .TRUE.,.TRUE.  0000000 
C     AND .FALSE. RESPECTIVELY.                                         0000000 
C                                                                       0000000 
C THE VARIABLES IN THE COMMON BLOCK MA30F/FD ARE USED TO PROVIDE THE    0000000 
C     USER WITH INFORMATION ON THE DECOMPOSITION.                       0000000 
C     COMMON /MA30FD/ IRNCP, ICNCP, IRANK, MINIRN, MINICN               0000000 
C IRNCP AND ICNCP ARE INTEGER VARIABLES USED TO MONITOR THE ADEQUACY    0000000 
C     OF THE ALLOCATED SPACE IN ARRAYS IRN AND A/ICN RESPECTIVELY, BY   0000000 
C     TAKING ACCOUNT OF THE NUMBER OF DATA MANAGEMENT COMPRESSES        0000000 
C     REQUIRED ON THESE ARRAYS. IF IRNCP OR ICNCP IS FAIRLY LARGE (SAY  0000000 
C     GREATER THAN N/10), IT MAY BE ADVANTAGEOUS TO INCREASE THE SIZE   0000000 
C     OF THE CORRESPONDING ARRAY(S).  IRNCP AND ICNCP ARE INITIALIZED   0000000 
C     TO ZERO ON ENTRY TO MA30A/AD AND ARE INCREMENTED EACH TIME THE    0000000 
C     COMPRESSING ROUTINE MA30D/DD IS ENTERED.                          0000000 
C ICNCP IS THE NUMBER OF COMPRESSES ON A/ICN.                           0000000 
C IRNCP IS THE NUMBER OF COMPRESSES ON IRN.                             0000000 
C IRANK IS AN INTEGER VARIABLE WHICH GIVES AN ESTIMATE (ACTUALLY AN     0000000 
C     UPPER BOUND) OF THE RANK OF THE MATRIX. ON AN EXIT WITH IFLAG     0000000 
C     EQUAL TO 0, THIS WILL BE EQUAL TO N.                              0000000 
C MINIRN IS AN INTEGER VARIABLE WHICH, AFTER A SUCCESSFUL CALL TO       0000000 
C     MA30A/AD, INDICATES THE MINIMUM LENGTH TO WHICH IRN CAN BE        0000000 
C     REDUCED WHILE STILL PERMITTING A SUCCESSFUL DECOMPOSITION OF THE  0000000 
C     SAME MATRIX. IF, HOWEVER, THE USER WERE TO DECREASE THE LENGTH    0000000 
C     OF IRN TO THAT SIZE, THE NUMBER OF COMPRESSES (IRNCP) MAY BE      0000000 
C     VERY HIGH AND QUITE COSTLY. IF LIRN IS NOT LARGE ENOUGH TO BEGIN  0000000 
C     THE DECOMPOSITION ON A DIAGONAL BLOCK, MINIRN WILL BE EQUAL TO    0000000 
C     THE VALUE REQUIRED TO CONTINUE THE DECOMPOSITION AND IFLAG WILL   0000000 
C     BE SET TO -3 OR -6. A VALUE OF LIRN SLIGHTLY GREATER THAN THIS    0000000 
C     (SAY ABOUT N/2) WILL USUALLY PROVIDE ENOUGH SPACE TO COMPLETE     0000000 
C     THE DECOMPOSITION ON THAT BLOCK. IN THE EVENT OF ANY OTHER        0000000 
C     FAILURE MINIRN GIVES THE MINIMUM SIZE OF IRN REQUIRED FOR A       0000000 
C     SUCCESSFUL DECOMPOSITION UP TO THAT POINT.                        0000000 
C MINICN IS AN INTEGER VARIABLE WHICH AFTER A SUCCESSFUL CALL TO        0000000 
C     MA30A/AD, INDICATES THE MINIMUM SIZE OF LICN REQUIRED TO ENABLE   0000000 
C     A SUCCESSFUL DECOMPOSITION. IN THE EVENT OF FAILURE WITH IFLAG=   0000000 
C     -5, MINICN WILL, IF ABORT3 IS LEFT SET TO .FALSE., INDICATE THE   0000000 
C     MINIMUM LENGTH THAT WOULD BE SUFFICIENT TO PREVENT THIS ERROR IN  0000000 
C     A SUBSEQUENT RUN ON AN IDENTICAL MATRIX. AGAIN THE USER MAY       0000000 
C     PREFER TO USE A VALUE OF ICN SLIGHTLY GREATER THAN MINICN FOR     0000000 
C     SUBSEQUENT RUNS TO AVOID TOO MANY CONPRESSES (ICNCP). IN THE      0000000 
C     EVENT OF FAILURE WITH IFLAG EQUAL TO ANY NEGATIVE VALUE EXCEPT    0000000 
C     -4, MINICN WILL GIVE THE MINIMUM LENGTH TO WHICH LICN COULD BE    0000000 
C     REDUCED TO ENABLE A SUCCESSFUL DECOMPOSITION TO THE POINT AT      0000000 
C     WHICH FAILURE OCCURRED.  NOTICE THAT, ON A SUCCESSFUL ENTRY       0000000 
C     IDISP(2) GIVES THE AMOUNT OF SPACE IN A/ICN REQUIRED FOR THE      0000000 
C     DECOMPOSITION WHILE MINICN WILL USUALLY BE SLIGHTLY GREATER       0000000 
C     BECAUSE OF THE NEED FOR "ELBOW ROOM".  IF THE USER IS VERY        0000000 
C     UNSURE HOW LARGE TO MAKE LICN, THE VARIABLE MINICN CAN BE USED    0000000 
C     TO PROVIDE THAT INFORMATION. A PRELIMINARY RUN SHOULD BE          0000000 
C     PERFORMED WITH ABORT3 LEFT SET TO .FALSE. AND LICN ABOUT 3/2      0000000 
C     TIMES AS BIG AS THE NUMBER OF NON-ZEROS IN THE ORIGINAL MATRIX.   0000000 
C     UNLESS THE INITIAL PROBLEM IS VERY SPARSE (WHEN THE RUN WILL BE   0000000 
C     SUCCESSFUL) OR FILLS IN EXTREMELY BADLY (GIVING AN ERROR RETURN   0000000 
C     WITH IFLAG EQUAL TO -4), AN ERROR RETURN WITH IFLAG EQUAL TO -5   0000000 
C     SHOULD RESULT AND MINICN WILL GIVE THE AMOUNT OF SPACE REQUIRED   0000000 
C     FOR A SUCCESSFUL DECOMPOSITION.                                   0000000 
C                                                                       0000000 
C COMMON BLOCK MA30G/GD IS USED BY THE MA30B/BD ENTRY ONLY.             0000000 
C     COMMON /MA30GD/ EPS, RMIN                                         0000000 
C EPS IS A REAL/DOUBLE PRECISION VARIABLE. IT IS USED TO TEST FOR       0000000 
C     SMALL PIVOTS. ITS DEFAULT VALUE IS 1.0E-4 (1.0D-4 IN D VERSION).  0000000 
C     IF THE USER SETS EPS TO ANY VALUE GREATER THAN 1.0, THEN NO       0000000 
C     CHECK IS MADE ON THE SIZE OF THE PIVOTS. ALTHOUGH THE ABSENCE OF  0000000 
C     SUCH A CHECK WOULD FAIL TO WARN THE USER OF BAD INSTABILITY, ITS  0000000 
C     ABSENCE WILL ENABLE MA30B/BD TO RUN SLIGHTLY FASTER. AN  A        0000000 
C     POSTERIORI  CHECK ON THE STABILITY OF THE FACTORIZATION CAN BE    0000000 
C     OBTAINED FROM MC24A/AD.                                           0000000 
C RMIN IS A REAL/DOUBLE PRECISION VARIABLE WHICH GIVES THE USER SOME    0000000 
C     INFORMATION ABOUT THE STABILITY OF THE DECOMPOSITION.  AT EACH    0000000 
C     STAGE OF THE LU DECOMPOSITION THE MAGNITUDE OF THE PIVOT APIV     0000000 
C     IS COMPARED WITH THE LARGEST OFF-DIAGONAL ENTRY CURRENTLY IN ITS  0000000 
C     ROW (ROW OF U), ROWMAX SAY. IF THE RATIO                          0000000 
C                       MIN (APIV/ROWMAX)                               0000000 
C     WHERE THE MINIMUM IS TAKEN OVER ALL THE ROWS, IS LESS THAN EPS    0000000 
C     THEN RMIN IS SET TO THIS MINIMUM VALUE AND IFLAG IS RETURNED      0000000 
C     WITH THE VALUE +I WHERE I IS THE ROW IN WHICH THIS MINIMUM        0000000 
C     OCCURS.  IF THE USER SETS EPS GREATER THAN ONE, THEN THIS TEST    0000000 
C     IS NOT PERFORMED. IN THIS CASE, AND WHEN THERE ARE NO SMALL       0000000 
C     PIVOTS RMIN WILL BE SET EQUAL TO EPS.                             0000000 
C                                                                       0000000 
C COMMON BLOCK MA30H/HD IS USED BY MA30C/CD ONLY.                       0000000 
C     COMMON /MA30HD/ RESID                                             0000000 
C RESID IS A REAL/DOUBLE PRECISION VARIABLE. IN THE CASE OF SINGULAR    0000000 
C     OR RECTANGULAR MATRICES ITS FINAL VALUE WILL BE EQUAL TO THE      0000000 
C     MAXIMUM RESIDUAL FOR THE UNSATISFIED EQUATIONS; OTHERWISE ITS     0000000 
C     VALUE WILL BE SET TO ZERO.                                        0000000 
C                                                                       0000000 
C COMMON  BLOCK MA30I/ID CONTROLS THE USE OF DROP TOLERANCES, THE       0000000 
C     MODIFIED PIVOT OPTION AND THE THE CALCULATION OF THE LARGEST      0000000 
C     ENTRY IN THE FACTORIZATION PROCESS. THIS COMMON BLOCK WAS ADDED   0000000 
C     TO THE MA30 PACKAGE IN FEBRUARY, 1983.                            0000000 
C     COMMON /MA30ID/ TOL, BIG, NDROP, NSRCH, LBIG                      0000000 
C TOL IS A REAL/DOUBLE PRECISION VARIABLE.  IF IT IS SET TO A POSITIVE  0000000 
C     VALUE, THEN MA30A/AD WILL DROP FROM THE FACTORS ANY NON-ZERO      0000000 
C     WHOSE MODULUS IS LESS THAN TOL.  THE FACTORIZATION WILL THEN      0000000 
C     REQUIRE LESS STORAGE BUT WILL BE INACCURATE.  AFTER A RUN OF      0000000 
C     MA30A/AD WHERE ENTRIES HAVE BEEN DROPPED, MA30B/BD  SHOULD NOT    0000000 
C     BE CALLED.  THE DEFAULT VALUE FOR TOL IS 0.0.                     0000000 
C BIG IS A REAL/DOUBLE PRECISION VARIABLE.  IF LBIG HAS BEEN SET TO     0000000 
C     .TRUE., BIG WILL BE SET TO THE LARGEST ENTRY ENCOUNTERED DURING   0000000 
C     THE FACTORIZATION.                                                0000000 
C NDROP IS AN INTEGER VARIABLE. IF TOL HAS BEEN SET POSITIVE, ON EXIT   0000000 
C     FROM MA30A/AD, NDROP WILL HOLD THE NUMBER OF ENTRIES DROPPED      0000000 
C     FROM THE DATA STRUCTURE.                                          0000000 
C NSRCH IS AN INTEGER VARIABLE. IF NSRCH IS SET TO A VALUE LESS THAN    0000000 
C     OR EQUAL TO N, THEN A DIFFERENT PIVOT OPTION WILL BE EMPLOYED BY  0000000 
C     MA30A/AD.  THIS MAY RESULT IN DIFFERENT FILL-IN AND EXECUTION     0000000 
C     TIME FOR MA30A/AD. IF NSRCH IS LESS THAN OR EQUAL TO N, THE       0000000 
C     WORKSPACE ARRAYS LASTC AND NEXTC ARE NOT REFERENCED BY MA30A/AD.  0000000 
C     THE DEFAULT VALUE FOR NSRCH IS 32768.                             0000000 
C LBIG IS A LOGICAL VARIABLE. IF LBIG IS SET TO .TRUE., THE VALUE OF    0000000 
C     THE LARGEST ENTRY ENCOUNTERED IN THE FACTORIZATION BY MA30A/AD    0000000 
C     IS RETURNED IN BIG.  SETTING LBIG TO .TRUE.  WILL MARGINALLY      0000000 
C     INCREASE THE FACTORIZATION TIME FOR MA30A/AD AND WILL INCREASE    0000000 
C     THAT FOR MA30B/BD BY ABOUT 20%.  THE DEFAULT VALUE FOR LBIG IS    0000000 
C     .FALSE.                                                           0000000 
C                                                                       0000000 
      DOUBLE PRECISION EPS, RMIN, TOL, BIG                                      
      LOGICAL ABORT1, ABORT2, ABORT3, LBIG                                      
      COMMON /MA30ED/ LP, ABORT1, ABORT2, ABORT3                                
      COMMON /MA30GD/ EPS, RMIN                                                 
      COMMON /MA30ID/ TOL, BIG, NDROP, NSRCH, LBIG                              
      DATA EPS /1.0D-4/, TOL /0.0D0/, BIG /0.0D0/                               
      DATA LP /6/, NSRCH /32768/                                                
      DATA LBIG /.FALSE./                                                       
      DATA ABORT1 /.TRUE./, ABORT2 /.TRUE./, ABORT3 /.FALSE./                   
      END                                                                       
      SUBROUTINE XERRWV (MSG, NMES, NERR, IERT, NI, I1, I2, NR, R1, R2)
      INTEGER MSG, NMES, NERR, IERT, NI, I1, I2, NR,
     1   I, LUN, LUNIT, MESFLG, NCPW, NCH, NWDS
      DOUBLE PRECISION R1, R2
      DIMENSION MSG(NMES)
C-----------------------------------------------------------------------
C SUBROUTINES XERRWV, XSETF, AND XSETUN, AS GIVEN HERE, CONSTITUTE
C A SIMPLIFIED VERSION OF THE SLATEC ERROR HANDLING PACKAGE.
C WRITTEN BY A. C. HINDMARSH AT LLNL.  VERSION OF AUGUST 13, 1981.
C THIS VERSION IS IN DOUBLE PRECISION.
C
C ALL ARGUMENTS ARE INPUT ARGUMENTS.
C
C MSG    = THE MESSAGE (HOLLERITH LITTERAL OR INTEGER ARRAY).
C NMES   = THE LENGTH OF MSG (NUMBER OF CHARACTERS).
C NERR   = THE ERROR NUMBER (NOT USED).
C IERT   = THE ERROR TYPE..
C          1 MEANS RECOVERABLE (CONTROL RETURNS TO CALLER).
C          2 MEANS FATAL (RUN IS ABORTED--SEE NOTE BELOW).
C NI     = NUMBER OF INTEGERS (0, 1, OR 2) TO BE PRINTED WITH MESSAGE.
C I1,I2  = INTEGERS TO BE PRINTED, DEPENDING ON NI.
C NR     = NUMBER OF REALS (0, 1, OR 2) TO BE PRINTED WITH MESSAGE.
C R1,R2  = REALS TO BE PRINTED, DEPENDING ON NR.
C
C NOTE..  THIS ROUTINE IS MACHINE-DEPENDENT AND SPECIALIZED FOR USE
C IN LIMITED CONTEXT, IN THE FOLLOWING WAYS..
C 1. THE NUMBER OF HOLLERITH CHARACTERS STORED PER WORD, DENOTED
C    BY NCPW BELOW, IS A DATA-LOADED CONSTANT.
C 2. THE VALUE OF NMES IS ASSUMED TO BE AT MOST 60.
C    (MULTI-LINE MESSAGES ARE GENERATED BY REPEATED CALLS.)
C 3. IF IERT = 2, CONTROL PASSES TO THE STATEMENT   STOP
C    TO ABORT THE RUN.  THIS STATEMENT MAY BE MACHINE-DEPENDENT.
C 4. R1 AND R2 ARE ASSUMED TO BE IN DOUBLE PRECISION AND ARE PRINTED
C    IN D21.13 FORMAT.
C 5. THE COMMON BLOCK /EH0001/ BELOW IS DATA-LOADED (A MACHINE-
C    DEPENDENT FEATURE) WITH DEFAULT VALUES.
C    THIS BLOCK IS NEEDED FOR PROPER RETENTION OF PARAMETERS USED BY
C    THIS ROUTINE WHICH THE USER CAN RESET BY CALLING XSETF OR XSETUN.
C    THE VARIABLES IN THIS BLOCK ARE AS FOLLOWS..
C       MESFLG = PRINT CONTROL FLAG..
C                1 MEANS PRINT ALL MESSAGES (THE DEFAULT).
C                0 MEANS NO PRINTING.
C       LUNIT  = LOGICAL UNIT NUMBER FOR MESSAGES.
C                THE DEFAULT IS 6 (MACHINE-DEPENDENT).
C-----------------------------------------------------------------------
C THE FOLLOWING ARE INSTRUCTIONS FOR INSTALLING THIS ROUTINE
C IN DIFFERENT MACHINE ENVIRONMENTS.
C
C TO CHANGE THE DEFAULT OUTPUT UNIT, CHANGE THE DATA STATEMENT
C IN THE BLOCK DATA SUBPROGRAM BELOW.
C
C FOR A DIFFERENT NUMBER OF CHARACTERS PER WORD, CHANGE THE
C DATA STATEMENT SETTING NCPW BELOW, AND FORMAT 10.  ALTERNATIVES FOR
C VARIOUS COMPUTERS ARE SHOWN IN COMMENT CARDS.
C
C FOR A DIFFERENT RUN-ABORT COMMAND, CHANGE THE STATEMENT FOLLOWING
C STATEMENT 100 AT THE END.
C-----------------------------------------------------------------------
      COMMON /EH0001/ MESFLG, LUNIT
C-----------------------------------------------------------------------
C THE FOLLOWING DATA-LOADED VALUE OF NCPW IS VALID FOR THE CDC-6600
C AND CDC-7600 COMPUTERS.
C     DATA NCPW/10/
C THE FOLLOWING IS VALID FOR THE CRAY-1 COMPUTER.
C     DATA NCPW/8/
C THE FOLLOWING IS VALID FOR THE BURROUGHS 6700 AND 7800 COMPUTERS.
C     DATA NCPW/6/
C THE FOLLOWING IS VALID FOR THE PDP-10 COMPUTER.
C     DATA NCPW/5/
C THE FOLLOWING IS VALID FOR THE VAX COMPUTER WITH 4 BYTES PER INTEGER,
C AND FOR THE IBM-360, IBM-370, IBM-303X, AND IBM-43XX COMPUTERS.
      DATA NCPW/4/
C THE FOLLOWING IS VALID FOR THE PDP-11, OR VAX WITH 2-BYTE INTEGERS.
C     DATA NCPW/2/
C-----------------------------------------------------------------------
      IF (MESFLG .EQ. 0) GO TO 100
C GET LOGICAL UNIT NUMBER. ---------------------------------------------
      LUN = LUNIT
C GET NUMBER OF WORDS IN MESSAGE. --------------------------------------
      NCH = MIN0(NMES,60)
      NWDS = NCH/NCPW
      IF (NCH .NE. NWDS*NCPW) NWDS = NWDS + 1
C WRITE THE MESSAGE. ---------------------------------------------------
      WRITE (LUN, 10) (MSG(I),I=1,NWDS)
C-----------------------------------------------------------------------
C THE FOLLOWING FORMAT STATEMENT IS TO HAVE THE FORM
C 10  FORMAT(1X,MMANN)
C WHERE NN = NCPW AND MM IS THE SMALLEST INTEGER .GE. 60/NCPW.
C THE FOLLOWING IS VALID FOR NCPW = 10.
C 10  FORMAT(1X,6A10)
C THE FOLLOWING IS VALID FOR NCPW = 8.
C 10  FORMAT(1X,8A8)
C THE FOLLOWING IS VALID FOR NCPW = 6.
C 10  FORMAT(1X,10A6)
C THE FOLLOWING IS VALID FOR NCPW = 5.
C 10  FORMAT(1X,12A5)
C THE FOLLOWING IS VALID FOR NCPW = 4.
  10  FORMAT(1X,15A4)
C THE FOLLOWING IS VALID FOR NCPW = 2.
C 10  FORMAT(1X,30A2)
C-----------------------------------------------------------------------
      IF (NI .EQ. 1) WRITE (LUN, 20) I1
 20   FORMAT(6X,23HIN ABOVE MESSAGE,  I1 =,I10)
      IF (NI .EQ. 2) WRITE (LUN, 30) I1,I2
 30   FORMAT(6X,23HIN ABOVE MESSAGE,  I1 =,I10,3X,4HI2 =,I10)
      IF (NR .EQ. 1) WRITE (LUN, 40) R1
 40   FORMAT(6X,23HIN ABOVE MESSAGE,  R1 =,D21.13)
      IF (NR .EQ. 2) WRITE (LUN, 50) R1,R2
 50   FORMAT(6X,15HIN ABOVE,  R1 =,D21.13,3X,4HR2 =,D21.13)
C ABORT THE RUN IF IERT = 2. -------------------------------------------
 100  IF (IERT .NE. 2) RETURN
      STOP
C----------------------- END OF SUBROUTINE XERRWV ----------------------
      END

