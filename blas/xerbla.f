      SUBROUTINE XERBLA(SRNAME,INFO)
C     .. Scalar Arguments ..
      INTEGER INFO
      CHARACTER SRNAME*6
C     ..
C
C  Purpose
C  =======
C
C  XERBLA  is an error handler for the Level 2 BLAS routines.
C
C  It is called by the Level 2 BLAS routines if an input parameter is
C  invalid.
C
C  Installers should consider modifying the STOP statement in order to
C  call system-specific exception-handling facilities.
C
C  Parameters
C  ==========
C
C  SRNAME - CHARACTER*6.
C           On entry, SRNAME specifies the name of the routine which
C           called XERBLA.
C
C  INFO   - INTEGER.
C           On entry, INFO specifies the position of the invalid
C           parameter in the parameter-list of the calling routine.
C
C
C  Auxiliary routine for Level 2 Blas.
C
C  Written on 20-July-1986.
C
C     .. Executable Statements ..
C
      WRITE (*,FMT=99999) SRNAME,INFO
C
      STOP
C
99999 FORMAT (' ** On entry to ',A6,' parameter number ',I2,
     +       ' had an illegal value')
C
C     End of XERBLA.
C
      END
