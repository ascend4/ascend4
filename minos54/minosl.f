************************************************************************
*
*     File  minosl   fortran.
*
*     MINOSL   funobj (+ dummy entries)
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      program            MINOSL

      implicit           double precision (a-h,o-z)

*     ------------------------------------------------------------------
*     MINOSL and this version of funobj are simplified routines
*     for MINOS 5.4 when only linear programs are to be solved.
*     They should be used in conjunction with files
*           mi10mach, mi15blas, mi20amat, mi25bfac, mi30spec,
*           mi35inpt, mi40bfil, mi50lp  .
*     They are substitutes for files
*           mi00main, mi05funs, mi60srch, mi65rmod, mi70nobj, mi80ncon.
*     ------------------------------------------------------------------

      parameter           (nwcore = 100000)
      double precision   z(nwcore)

      call minos1( z, nwcore )

*     end of main program for MINOSL
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine funobj
      entry funcon
      entry matmod
      entry m6fobj
      entry m6fcon
      entry m6dmmy
      entry m6fun
      entry m6grd
      entry m6rdel
      entry m7chkg
      entry m7fixb
      entry m7rg
      entry m7rgit
      entry m8ajac
      entry m8augl
      entry m8chkj
      entry m8setj
      entry m8viol

*     end of dummy routines for MINOSL
      end
