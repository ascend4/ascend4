      PROGRAM DEMO
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
C     Author:  A. Kroener                      25 - June - 1990
C              Institut f"ur Systemdynamik und Regelungstechnik
C              Universit"at Stuttgart
C              Pfaffenwaldring 9
C              D - 7000 Stuttgart 80
C              
C
C     Datum     Bearbeiter    Aenderungen
C
C***********************************************************************
C
C     Aufgabe des Programms:
C     ----------------------
C
C     Driver routine for the sparse DA-integrator SDASSL.
C     For further details see the updated description
C     SDASSL_INFO.TXT.
C     The values of the control arrau INFO(1:11) are input from the
C     data file INFO.DAT.
C     In the data file CONTRL.DAT the user provides the required scalar
C     relative (RTOL) and absolute (ATOL) error tolerances and the
C     number of examples to be calculated. Two simple examples are
C     included.
C     
C     Each of the examples consists of three routines:
C     INIT#     reads from the file INIT#.DAT the system dimension,
C               initial and final time and an proposed initial time
C               step.
C               Initial conditions for the states Y and their
C               derivatives YP are also provided in this file.
C
C     PTN#      provides the sparsity pattern of the merged jacobian
C               dF/dY and dF/dYP in sparse colum vector notation.
C
C     FUN#      evaluates the system equations 0=F(Y,YP,T)
C
C***********************************************************************
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      IMPLICIT INTEGER (I-N)
C
      PARAMETER (NGL=20,NII2 = 1)
      PARAMETER (NJAC = NGL*NGL) 
      PARAMETER (NLUD = 2*NJAC) 
      PARAMETER (MLUD = NLUD)
      PARAMETER (LIW  = 20 + NGL*(2+13/NII2) + (NJAC+NLUD+MLUD)/NII2)
      PARAMETER (LRW  = 40 + 10*NGL + NJAC + NLUD)
C
      EXTERNAL PTN1, FUN1, INIT1
      EXTERNAL PTN2, FUN2, INIT2
C	
      DIMENSION Y(NGL), YP(NGL)
      DIMENSION RWS(LRW)
      DIMENSION IWS(LIW)
C
      COMMON / ANVE / IDUM1,IDUM2,IDUM3,LENB
      COMMON / JACV / LENFX,LENBX,IDUM4,IDUM5
C
      NG = NGL
      NOUT = 6
      IU1 = 11
      IU2 = 12
C
      OPEN (IU1, FILE='CONTRL.DAT', STATUS='OLD', FORM='FORMATTED',
     $      ACCESS='SEQUENTIAL')
C
C ---- read tolerances for integration
C
      READ (IU1,*) RTOL, ATOL
C
C ---- loop over several examples
C
 100  CONTINUE
C
C ---- read no. of example
C
      READ (IU1,*,END=200) JBSP
      WRITE (NOUT,*) 'Example No.', JBSP
C
C
      N = NGL
      ISTEP = 0
C
      IF (JBSP .EQ. 1) THEN
C
C ---- initialization and initial values
C
        CALL INIT1 (N, Y, YP, T0, TE, H0)
C
C ---- integration
C
        CALL INTG (N, Y, YP, IWS, IWS(16), (LIW-15), RWS, LRW, T0, TE,
     1             H0, RTOL, ATOL, PTN1, FUN1)
      ELSE IF (JBSP .EQ. 2) THEN
        CALL INIT2 (N, Y, YP, T0, TE, H0)
        CALL INTG (N, Y, YP, IWS, IWS(16), (LIW-15), RWS, LRW, T0, TE,
     1             H0, RTOL, ATOL, PTN2, FUN2)
      ELSE
        STOP ' Error in no. of example'
      END IF
C
      GOTO 100
C
 200  CLOSE (IU1)
      WRITE (NOUT,'(/5X,A/)') 'DEMO successfully completed'
      STOP
      END

      SUBROUTINE INTG (N, Y, YP, INFO, IWORK, LIW, RWORK, LRW, T0,
     1                 TFINAL, H0, RTOL, ATOL, PTN, FUN)
C
C***********************************************************************
C
C       supply parameters for integrator DDASSL
C
C***********************************************************************
C
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
C
      DIMENSION Y(N), YP(N)
      DIMENSION INFO(1), IWORK(1), RWORK(1)
C
C      COMMON / IO / NOUT
C      COMMON / HLP / NN, JBSP
C
      EXTERNAL PTN, FUN
C
      OPEN (UNIT=20,FILE='INFO.DAT',STATUS='OLD')
      READ(20,21)(INFO(I),I=1,15)
 21   FORMAT(I3)
      WRITE(6,25)(info(i),I = 1,15)
 25   FORMAT(1x,I3)
      WRITE(6,*) 'info complete'
      CLOSE(20)

      T = T0
      DELTAT = .1
      TOUT = T0 + DELTAT                   ! next output time
      RWORK(1) = T0 + DELTAT               ! TSTOP
      RWORK(2) = 1.0                       ! HMAX 
      RWORK(3) = H0
C
      IWORK(1) = 4
      IWORK(2) = 4
      IWORK(3) = 1
      ICHAN = 0

C
C ---- integration
C
 10   CONTINUE
      CALL OUT (T, N, Y, YP)
      DO 40 WHILE (T .LE. TFINAL)
         DO 45 WHILE (T .LE. TOUT)
            CALL DDASSL (FUN, N, T, Y, YP, TOUT, INFO, RTOL, ATOL, IDID,
     $                   RWORK, LRW, IWORK, LIW, RPAR, IPAR, JAC,PTN,
     1                   ICHAN) 
C
            WRITE(6,*)idid
            CALL OUT (T, N, Y, YP)
            IF (idid .LT. 0 ) GO TO 60
            IF (IDID .EQ. -1) THEN
               INFO(1) = 1
               ICHAN = 0
            ELSEIF (idid .EQ. 1 .OR. idid .EQ. 3) THEN
               INFO(1) = 1
               ICHAN = 0
            ENDIF
            IF ((IDID .EQ. 3) .OR. (IDID .EQ. 2) ) GO TO 55
 45      END DO
 55      TOUT = t + DELTAT
         RWORK(1) = t + DELTAT
 40   END DO
 60   CONTINUE
   
      RETURN
      END

      SUBROUTINE OUT (T, N, Y, YP)
C
C***********************************************************************
C
C       solution output
C
C***********************************************************************
C
      IMPLICIT DOUBLE PRECISION (A-H,O-Z)
      IMPLICIT INTEGER (I-N)
      DIMENSION Y(N), YP(N)
C
      SAVE /ISTEP/
      DATA ISTEP /1/
      SAVE /TT/
      DATA TT /-1./

      IF (TT .GT. T ) ISTEP = 1
C
      WRITE (6,*) ISTEP, T
      WRITE (6,*)(Y(I),I=1,N)
      WRITE (6,*)(YP(I),I=1,N)
      ISTEP = ISTEP + 1
      TT = T
C
      RETURN
      END
      SUBROUTINE FUN1 (T, Y, YP, DELTA, IRES, RPAR, IPAR)
C
      DOUBLE PRECISION T, Y(2), YP(2), DELTA(2), RPAR(1), RL
C
      INTEGER IRES, IPAR(1)
C
      RL = 8.0D-3
      DELTA(1) = -YP(1) - RL / (Y(1)*Y(1)) * DEXP (-1.5D0*Y(2))
      DELTA(2) =             - 5.D0 * Y(1) + DEXP (-0.5D0*Y(2))
      IRES = 0
C
      RETURN
      END
      SUBROUTINE FUN2 (T, Y, YP, DELTA, IRES, RPAR, IPAR)
C
      DOUBLE PRECISION T, Y(3), YP(3), DELTA(3), RPAR(1)
      DOUBLE PRECISION R_1,SK_1,SK_2,SK_3
C
      INTEGER IRES, IPAR(1)
C
      r_1 = .01
      SK_1 = 100
      SK_2 = 1
      SK_3 = 1000
C
      DELTA(1) = YP(1) - sk_1 * (Y(2))
      DELTA(2) = YP(2) - sk_2 * (-Y(2) - Y(3) + R_1 )
      DELTA(3) = YP(3) - sk_3 * ( Y(1) - Y(3))
C
      IRES = 0
C
      RETURN
      END
C****************************************************************************
C SUBROUTINE WHICH RETURNS THE MERGED SPARSITY PATTERNS OF THE JACOBIAN
C*********************************************************************

        SUBROUTINE PTN1(IP,M,IRN,LIRN,LDIM,RDUM,IDUM)
        INTEGER M,LIRN,LDIM
        INTEGER IP(M),IRN(LIRN), IDUM(1)
        DOUBLEPRECISION RDUM(1)
C
 
        IP(1) = 2
        IP(2) = 2
C
        IRN(1) = 1
        IRN(2) = 2
        IRN(3) = 1
        IRN(4) = 2

        LDIM = 4

        END
C****************************************************************************
C SUBROUTINE WHICH RETURNS THE MERGED SPARSITY PATTERNS OF THE JACOBIAN
C*********************************************************************

        SUBROUTINE PTN2(IP,M,IRN,LIRN,LDIM,RDUM,IDUM)
        INTEGER M,LIRN,LDIM
        INTEGER IP(M),IRN(LIRN), IDUM(1)
        DOUBLEPRECISION RDUM(1)
C
 
        IP(1) = 2
        IP(2) = 2
        IP(3) = 2
C
        IRN(1) = 1
        IRN(2) = 3
        IRN(3) = 1
        IRN(4) = 2
        IRN(5) = 2
        IRN(6) = 3

        LDIM = 6

        END
C
C***********************************************************************
C
C initialization
C
C***********************************************************************
C
C N     : system order (on input at least 100)
C Y0    : initial values
C YP0   : initial values of derivatives
C T0    : initial time
C TE    : end time
C H0    : initial stepsize (suggestion)
C        
C************************************************************************
C
      SUBROUTINE INIT1 (N, Y0, YP0, T0, TE, H0)
 
      INTEGER N, IUNIT, I
      INTEGER IDUM1,IDUM2,IDUM3,LENB
      INTEGER LENFX,LENBX,IDUM4,IDUM5
C
      DOUBLE PRECISION Y0(N), YP0(N)
      DOUBLE PRECISION T0, TE, H0
C
      COMMON / ANVE / IDUM1,IDUM2,IDUM3,LENB
      COMMON / JACV / LENFX,LENBX,IDUM4,IDUM5

      LENB = 1
      LENFX = 4
      LENBX = 0
C
      IUNIT = 21
C
      OPEN (IUNIT, FILE='INIT1.DAT', STATUS='OLD', FORM ='FORMATTED',
     1      ACCESS='SEQUENTIAL')
C
      READ (IUNIT,*) N, T0, TE, H0
C
      READ (IUNIT,*) (Y0(I), I=1,N)
      READ (IUNIT,*) (YP0(I), I=1,N)

      CLOSE (IUNIT)
      RETURN
      END
C
C***********************************************************************
C
C initialization
C
C***********************************************************************
C
C N     : system order (on input at least 100)
C Y0    : initial values
C YP0   : initial values of derivatives
C T0    : initial time
C TE    : end time
C H0    : initial stepsize (suggestion)
C        
C************************************************************************
C
      SUBROUTINE INIT2 (N, Y0, YP0, T0, TE, H0)
 
      INTEGER N, IUNIT, I
      INTEGER IDUM1,IDUM2,IDUM3,LENB
      INTEGER LENFX,LENBX,IDUM4,IDUM5
C
      DOUBLE PRECISION Y0(N), YP0(N)
      DOUBLE PRECISION T0, TE, H0
C
      COMMON / ANVE / IDUM1,IDUM2,IDUM3,LENB
      COMMON / JACV / LENFX,LENBX,IDUM4,IDUM5

      LENB = 3
      LENFX = 5
      LENBX = 0
C
      IUNIT = 21
C
      OPEN (IUNIT, FILE='INIT2.DAT', STATUS='OLD', FORM ='FORMATTED',
     1      ACCESS='SEQUENTIAL')
C
      READ (IUNIT,*) N, T0, TE, H0
C
      READ (IUNIT,*) (Y0(I), I=1,N)
      READ (IUNIT,*) (YP0(I), I=1,N)

      CLOSE (IUNIT)
      RETURN
      END

