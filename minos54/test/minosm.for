*     ------------------------------------------------------------------
*     File minosm.for
*     This is a main program to test subroutine matmps, which is
*     part of MINOS 5.4.  It uses matmps to read the MPS file
*     associated with problem t4manne, then asks minoss to solve it.
*
*     
*     19 Apr 1992: First version, derived from minoss.for.
*     ------------------------------------------------------------------

      program            minosm

      implicit           double precision (a-h,o-z)

      parameter        ( maxm   = 60,
     $                   maxn   = 90,
     $                   maxnb  = maxm + maxn,
     $                   maxne  = 200,
     $                   nname  = maxnb )

      character*8        names(5)
      integer*2          ha(maxne) , hs(maxnb)
      integer            ka(maxn+1), name1(nname), name2(nname)
      double precision   a(maxne)  , bl(maxnb)   , bu(maxnb),
     $                   xn(maxnb) , pi(maxm)    , rc(maxn)

      parameter          ( nwcore = 50000 )
      double precision   z(nwcore)
*     ------------------------------------------------------------------

*     Specify some of the MINOS files.
*     ispecs  is the Specs file.
*     iprint  is the Print file.
*     isumm   is the Summary file.
*     (mioptn opens these files via mifile and m1open.)
*     nout    is an output file to be used here by mitest.

      ispecs = 4
      iprint = 9
      isumm  = 6
      nout   = 6

*     ------------------------------------------------------------------
*     Read a SPECS file from file ispecs.
*     ------------------------------------------------------------------
      call mioptn( ispecs, iprint, isumm, nwcore, inform )

      if (inform .ge. 2) then
         write(nout, *) 'No Specs file found'
         stop
      end if

*     ------------------------------------------------------------------
*     Read an MPS file from unit imps.
*     File imps should be opened here if necessary.
*
*     nncon, nnjac and nnobj can be hardwired here, or specified in the
*     SPECS file as
*        Nonlinear constraints
*        Nonlinear Jacobian  variables
*        Nonlinear objective variables
*     respectively.  For linear programs they would all be zero.
*     ------------------------------------------------------------------
      imps   = 10
      nncon  = 10
      nnjac  = 10
      nnobj  = 20

*     matmps will load names(1) with the problem name from the MPS file.
*     Here we can load names(2)-names(5) with the desired 8-character
*     Objective, Rhs, Ranges and Bounds names.
*     Blank means grab the first names encountered in the MPS file.

      names(2) = 'funobj'
      names(3) = ' '
      names(4) = ' '
      names(5) = ' '

      call matmps( imps, maxm, maxn, maxnb, maxne,
     $             nncon, nnjac, nnobj,
     $             m, n, nb, ne,
     $             iobj, objadd, names,
     $             a, ha, ka, bl, bu, name1, name2,
     $             hs, xn,
     $             inform, ns, z, nwcore )

      if (inform .ge. 1) then
         write(nout, *) 'Error while reading MPS file from unit', imps
         stop
      end if             

*     ------------------------------------------------------------------
*     Go for it.
*     mode   is not used by minoss yet.
*     ------------------------------------------------------------------
      mode   = 2
      call minoss( mode, m, n, nb, ne, nname,
     $             nncon, nnobj, nnjac,
     $             iobj, objadd, names,
     $             a, ha, ka, bl, bu, name1, name2,
     $             hs, xn, pi, rc, 
     $             inform, mincor, ns, ninf, sinf, obj,
     $             z, nwcore )

      write(nout, *) ' '
      write(nout, *) 'minoss finished.'
      write(nout, *) 'inform =', inform
      write(nout, *) 'ninf   =', ninf
      write(nout, *) 'sinf   =', sinf
      write(nout, *) 'obj    =', obj

*     ------------------------------------------------------------------
*     Alter some options and call minoss again.
*     The following illustrates the use of miopt, miopti and mioptr
*     to set specific options.  We can first cause all options
*     to take default values by saying
*     call miopt ( 'Defaults', ... ).
*     This is a bit silly since certain things from the SPECS file
*     then have to be redefined.  However, it shows the effect.
*     ------------------------------------------------------------------
      write(nout, *) ' '
      write(nout, *) 'Re-enter:'

      inform = 0
      call miopt ( '                  ',         iprint, isumm, inform )
      call miopt ( 'Defaults          ',         iprint, isumm, inform )
      call miopti( 'Problem number    ',   1114, iprint, isumm, inform )
      call miopt ( 'Maximize          ',         iprint, isumm, inform )
      call miopt ( 'Jacobian    Sparse',         iprint, isumm, inform )

      itnlim = 20
      penpar = 0.01
      call miopt ( 'Derivative level 3',         iprint, isumm, inform )
      call miopt ( 'Verify level     0',         iprint, isumm, inform )
      call miopt ( 'Scale option     0',         iprint, isumm, inform )
      call miopti( 'Iterations        ', itnlim, iprint, isumm, inform )
      call mioptr( 'Penalty parameter ', penpar, iprint, isumm, inform )

      if (inform .gt. 0) then
         write(nout, *) 'NOTE: Some of the options were not recognized'
      end if

*     If we don't alter hs, the existing basis will be retained
*     (by Crash).

      call minoss( mode, m, n, nb, ne, nname,
     $             nncon, nnobj, nnjac,
     $             iobj, objadd, names,
     $             a, ha, ka, bl, bu, name1, name2,
     $             hs, xn, pi, rc, 
     $             inform, mincor, ns, ninf, sinf, obj,
     $             z, nwcore )

      write(nout, *) ' '
      write(nout, *) 'minoss finished again.'
      write(nout, *) 'inform =', inform
      write(nout, *) 'obj    =', obj

*     end of main program to test subroutine matmps
      end

