      LOGICAL FUNCTION LSAME ( CA, CB )
*     .. Scalar Arguments  ..
      CHARACTER*1       CA,  CB
*     ..
*  Purpose
*  =======
*
*  LSAME  tests if CA is the same letter as CB regardless of case.
*
*  N.B.  This version of the routine is only correct for ASCII code.
*        Installers must modify the routine for other character-codes.
*
*        For EBCDIC systems the constant IOFF must be changed to -64.
*        For CDC system using 6-12 bit representations, the system-
*        specific code in comments must be activated.
*
*  Parameters
*  ==========
*
*  CA    - CHARACTER*1
*  CB    - CHARACTER*1
*          On entry, CA and CB specify characters to be compared.
*          Unchanged on exit.
*
*
*  Auxiliary routine for Level 2 Blas.
*
*  -- Written on 11-October-1988.
*     Richard Hanson, Sandia National Labs.
*     Jeremy Du Croz, Nag Central Office.
*
*     .. Parameters ..
      INTEGER           IOFF
      PARAMETER       ( IOFF = 32 )
*     .. Intrinsic Functions ..
      INTRINSIC         ICHAR
*     .. Executable Statements ..
*
*     Test if the characters are equal
*
      LSAME = CA .EQ. CB
*
*     Now test for equivalence
*
      IF ( .NOT. LSAME ) THEN
	 LSAME = ICHAR( CA) - IOFF .EQ. ICHAR( CB)
      END IF
      IF ( .NOT. LSAME ) THEN
	 LSAME = ICHAR( CA) .EQ. ICHAR( CB) - IOFF 
      END IF
*
      RETURN
*
*  The following comments contain code for CDC systems using 6-12 bit
*  representations.
*
*     .. Parameters ..
C     INTEGER           ICIRFX
C     PARAMETER       ( ICIRFX = 62 )
*     .. Scalar Arguments ..
C     CHARACTER*1       CB
*     .. Array Arguments ..
C     CHARACTER*1       CA(*)
*     .. Local Scalars ..
C     INTEGER           IVAL
*     .. Intrinsic Functions ..
C     INTRINSIC         ICHAR,  CHAR
*     .. Executable Statements ..
*
*     See if the first character in string CA equals string CB.
*
C     LSAME = CA(1) .EQ. CB  .AND.  CA(1) .NE. CHAR(ICIRFX)
C
C     IF (LSAME) RETURN
*
*     The characters are not identical.  Now check them for equivalence.
*     Look for the 'escape' character, circumflex, followed by the
*     letter.
*
C     IVAL = ICHAR( CA(2))
C     IF ( IVAL .GE. ICHAR('A')  .AND.  IVAL .LE. ICHAR('Z')) THEN
C        LSAME = CA(1) .EQ. CHAR(ICIRFX)  .AND.  CA(2) .EQ. CB
C     END IF
C
C     RETURN
C
*     End of LSAME.
*
      END
