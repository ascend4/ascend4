************************************************************************
*
*     File  mi00main fortran
*
************************************************************************
*                                                                      *
*                            M I N O S                                 *
*                                                                      *
*          A Modular In-core Nonlinear Optimization System             *
*                                                                      *
*               Version 5.4            10 Jul 1992                     *
*                                                                      *
*          Bruce A. Murtagh            Michael A. Saunders             *
*                                                                      *
*----------------------------------------------------------------------*
*                                                                      *
*                 Copyright  1992  Stanford University.                *
*                                                                      *
*     MINOS is distributed by OTL, Stanford University.  Enquiries     *
*     should be directed to one of the following addresses:            *
*                                                                      *
*     Office of Technology Licensing    Michael Saunders               *
*     857 Serra Street                  Dept of Operations Research    *
*     Stanford University               Stanford University            *
*     Stanford, CA 94305-6225           Stanford, CA 94305-4022        *
*                                                                      *
*     (415) 723-0651                    (415) 723-1875                 *
*                                                                      *
************************************************************************
*
*  MINOS Fortran source files:
*
*  1. mi00main   Main program
*  2. mi05funs   User routines
*  3. mi10mach   Machine-dependent routines
*  4. mi15blas   Basic Linear Algebra Subprograms (a subset)
*  5. mi20amat   Core allocation and manipulation of the ( A  I )
*  6. mi25bfac   Basis factorization routines
*  7. mi30spec   SPECS file routines
*  8. mi35inpt   MPS file routines
*  9. mi40bfil   Basis file and solution output routines
* 10. mi50lp     Routines for the primal simplex method
* 11. mi60srch   Routines for the linesearch and subproblem obj
* 12. mi65rmod   For maintaining R, the approximate reduced Hessian
* 13. mi70nobj   For handling a nonlinear objective function
*                via the reduced-gradient algorithm
* 14. mi80ncon   To handle nonlinear constraints
*                via the projected augmented Lagrangian algorithm
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      program            MINOS

      implicit           double precision (a-h,o-z)

*     ------------------------------------------------------------------
*     This is the default main program for MINOS.
*     It provides all of the necessary workspace.
*     If your compiler wants all common blocks to be in the main program
*     (e.g. MACFORTRAN), grab them from subroutine misolv in file mi10..
*     ------------------------------------------------------------------

      parameter           (nwcore = 50000)
      double precision   z(nwcore)

      call minos1( z, nwcore )

*     end of main program for stand-alone MINOS.
      end
