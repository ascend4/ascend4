*     ------------------------------------------------------------------
*     File minost.for
*     This is a main program to test subroutine minoss, which is
*     part of MINOS 5.4.  It generates the problem called MANNE on
*     Pages 98-108 of the MINOS 5.1 User's Guide, then asks minoss
*     to solve it.
*
*     
*     11 Nov 1991: First version.
*     27 Nov 1991: miopt, miopti, mioptr used to alter some options
*                  for a second call to minoss.
*     10 Apr 1992: objadd added as input parameter to minoss.
*     26 Jun 1992: integer*2 changed to integer*4.
*     ------------------------------------------------------------------

      program            minost

      implicit           double precision (a-h,o-z)

      parameter        ( maxm   = 100,
     $                   maxn   = 150,
     $                   maxnb  = maxm + maxn,
     $                   maxne  = 500,
     $                   nname  = 1 )

      character*8        names(5)
      integer*4          ha(maxne) , hs(maxnb)
      integer            ka(maxn+1), name1(nname), name2(nname)
      double precision   a(maxne)  , bl(maxnb)   , bu(maxnb),
     $                   xn(maxnb) , pi(maxm)    , rc(maxnb)

      parameter          ( nwcore = 50000 )
      double precision   z(nwcore)
      character*72       buf72
*     ------------------------------------------------------------------

*     Give names to the Problem, Objective, Rhs, Ranges and Bounds.

      names(1) = 'manne10 '
      names(2) = 'funobj  '
      names(3) = 'zero    '
      names(4) = 'range1  '
      names(5) = 'bound1  '

*     Specify some of the MINOS files.
*     ispecs  is the Specs file   (0 if none).
*     iprint  is the Print file   (0 if none).
*     isumm   is the Summary file (0 if none).
*     (mispec opens these files via mifile and m1open.)
*     nout    is an output file used here by mitest.

      ispecs = 4
      iprint = 9
      isumm  = 6
      nout   = 6

*     ------------------------------------------------------------------
*     Set options to default values.
*     Read a Specs file (if ispecs > 0).
*     ------------------------------------------------------------------
      call mispec( ispecs, iprint, isumm, nwcore, inform )

      if (inform .ge. 2) then
         write(nout, *) 'ispecs > 0 but no Specs file found'
         stop
      end if

*     ------------------------------------------------------------------
*     Generate a 10-period problem (nt = 10).
*     Instead of hardwiring nt here, we could do the following:
*     1. Say    Nonlinear constraints  10      in the Specs file.
*     2. At the top of this program include the following common block:
*               common    /m8len / njac  ,nncon ,nncon0,nnjac
*     3. Say    nt = nncon    in the line below.
*     ------------------------------------------------------------------
      nt     = 10
      call t4data( nt, maxm, maxn, maxnb, maxne, inform,
     $             m, n, nb, ne, nncon, nnobj, nnjac,
     $             a, ha, ka, bl, bu, xn, hs )

      if (inform .ge. 1) then
         write(nout, *) 'Not enough storage to generate a problem ',
     $                  'with  nt =', nt
         stop
      end if             

*     ------------------------------------------------------------------
*     Specify options that were not set in the Specs file.
*     i1 and i2 may refer to the Print and Summary file respectively.
*     Setting them to 0 suppresses printing.
*     ------------------------------------------------------------------
      i1     = 0
      i2     = 0
      ltime  = 2
      
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Jacobian Sparse'
      call miopt ( buf72, i1, i2, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Timing level'
      call miopti( buf72, ltime, i1, i2, inform )

*     ------------------------------------------------------------------
*     Go for it, using a Cold start.
*     iobj   = 0 means there is no linear objective row in a(*).
*     objadd = 0.0 means there is no constant to be added to the
*            objective.
*     hs     need not be set if a basis file is to be input.
*            Otherwise, each hs(1:n) should be 0, 1, 2, 3, 4, or 5.
*            The values are used by the Crash procedure m2crsh
*            to choose an initial basis B.
*            If hs(j) = 0 or 1, column j is eligible for B.
*            If hs(j) = 2, column j is initially superbasic (not in B).
*            If hs(j) = 3, column j is eligible for B and is given
*                          preference over columns with hs(j) = 0 or 1.
*            If hs(j) = 4 or 5, column j is initially nonbasic.
*     ------------------------------------------------------------------
      iobj   = 0
      objadd = 0.0
      call minoss( 'Cold', m, n, nb, ne, nname,
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
*     to set specific options.  If necessary, we could ensure that
*     all unspecified options take default values
*     by first calling miopt ( 'Defaults', ... ).
*     Beware that certain parameters would then need to be redefined.
*     ------------------------------------------------------------------
      write(nout, *) ' '
      write(nout, *) 'Re-enter:'

      inform = 0
      itnlim = 20
      penpar = 0.01

      do i = 1,72
         buf72 = ' '
      end do
      call miopt ( buf72, iprint, isumm, inform )
*---  call miopt ( 'Defaults          ',         iprint, isumm, inform )
*---  call miopti( 'Problem number    ',   1114, iprint, isumm, inform )
*---  call miopt ( 'Maximize          ',         iprint, isumm, inform )
*---  call miopt ( 'Jacobian    Sparse',         iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Derivative level 3'
      call miopt ( buf72, iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Print level 0'
      call miopt ( buf72, iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Verify level 0'
      call miopt ( buf72, iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Scale option 0'
      call miopt ( buf72, iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Iterations'
      call miopti( buf72, itnlim, iprint, isumm, inform )
      do i = 1,72
         buf72 = ' '
      end do
      buf72 = 'Penalty parameter'
      call mioptr( buf72, penpar, iprint, isumm, inform )

      if (inform .gt. 0) then
         write(nout, *) 'NOTE: Some of the options were not recognized'
      end if

*     Test the Warm start.
*     hs(*) specifies a complete basis from the previous call.
*     A Warm start uses hs(*) directly, without calling Crash.
*     
*     Warm and Hot starts are normally used after minoss has solved a
*     problem with the SAME DIMENSIONS but perhaps altered data.
*     Here we have not altered the data, so very few iterations
*     should be required.

      call minoss( 'Warm', m, n, nb, ne, nname,
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

*     Check that a Hot start works.
*     As with a Warm start, hs(*) specifies a basis from the
*     previous call.  In addition, up to three items from the previous
*     call can be reused.  They are denoted by F, H and S as follows:
*     'Hot F'    means use the existing basis FACTORS (B = LU).
*     'Hot H'    means use the existing reduced HESSIAN approximation.
*     'Hot S'    means use the existing column and row SCALES.
*     'Hot FS'   means use the Factors and Scales but not the Hessian.
*     'Hot FHS'  means use all three items.
*     'Hot'      is equivalent to 'Hot FHS'.
*     The letters F,H,S may be in any order.

      call minoss( 'Hot', m, n, nb, ne, nname,
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

*     end of main program to test subroutine minoss
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t4data( nt, maxm, maxn, maxnb, maxne, inform,
     $                   m, n, nb, ne, nncon, nnobj, nnjac,
     $                   a, ha, ka, bl, bu, xn, hs )

      implicit           double precision (a-h,o-z)
      integer*4          ha(maxne), hs(maxnb)
      integer            ka(maxn+1)
      double precision   a(maxne) , bl(maxnb), bu(maxnb), xn(maxnb)

*     ------------------------------------------------------------------
*     t4data  generates data for the test problem t4manne
*     (called problem MANNE in the MINOS 5.1 User's Guide).
*     The constraints take the form
*              f(x) + A*x + s = 0,
*     where the Jacobian for f(x) + Ax is stored in a(*), and any
*     terms coming from f(x) are in the TOP LEFT-HAND CORNER of a(*),
*     with dimensions  nncon x nnjac.
*     Note that the right-hand side is zero.
*     s is a set of slack variables whose bounds contain any constants
*     that might have formed a right-hand side.
*
*     The objective function is
*             F(x) + c'x
*     where c would be row iobj of A (but there is no such row in
*     this example).  F(x) involves only the FIRST nnobj variables.
*   
*     On entry,
*     nt      is T, the number of time periods.
*     maxm, maxn, maxnb, maxne are upper limits on m, n, nb, ne.
*
*     On exit,
*     inform  is 0 if there is enough storage, 1 otherwise.
*     m       is the number of nonlinear and linear constraints.
*     n       is the number of variables.
*     nb      is n + m.
*     ne      is the number of nonzeros in a(*).
*     nncon   is the number of nonlinear constraints (they come first).
*     nnobj   is the number of nonlinear objective variables.
*     nnjac   is the number of nonlinear Jacobian variables.
*     a       is the constraint matrix (Jacobian), stored column-wise.
*     ha      is the list of row indices for each nonzero in a(*).
*     ka      is a set of pointers to the beginning of each column of a.
*     bl      is the lower bounds on x and s.
*     bu      is the upper bounds on x and s.
*     xn(1:n) is a set of initial values for x.
*     hs(1:n) is a set of initial states for each x  (0,1,2,3,4,5).
*
*     09 Jul 1992: No need to initialize xn and hs for the slacks.
*     ------------------------------------------------------------------

      parameter      ( zero   = 0.0d+0,   one    = 1.0d+0,
     $                 dummy  = 0.1d+0,   growth = .03d+0,
     $                 bplus  = 1.0d+20,  bminus = - bplus )

*     nt defines the dimension of the problem.

      m      = nt*2
      n      = nt*3
      nb     = n + m
      nncon  = nt
      nnobj  = nt*2
      nnjac  = nt
      ne     = nt*6 - 1

*     Check if there is enough storage.

      inform = 0
      if (m      .gt. maxm ) inform = 1
      if (n      .gt. maxn ) inform = 1
      if (nb     .gt. maxnb) inform = 1
      if (ne     .gt. maxne) inform = 1
      if (inform .gt.   0  ) return

*     Generate columns for Capital (Kt, t = 1 to nt).
*     The first nt rows are nonlinear, and the next nt are linear.
*     The Jacobian is an nt x nt diagonal.
*     We generate the sparsity pattern here.
*     We put in dummy numerical values of 0.1 for the gradients.
*     Real values for the gradients are computed by t4con.

      ne     = 0
      do 100  k = 1, nt

*        There is one Jacobian nonzero per column.

         ne     = ne + 1
         ka(k)  = ne
         ha(ne) = k
         a(ne)  = dummy

*        The linear constraints form an upper bidiagonal pattern.

         if (k .gt. 1) then
            ne     = ne + 1
            ha(ne) = nt + k - 1
            a(ne)  = one
         end if

         ne     = ne + 1
         ha(ne) = nt + k
         a(ne)  = - one
  100 continue

*     The last nonzero is special.

      a(ne)  = growth

*     Generate columns for Consumption (Ct for t = 1 to nt).
*     They form -I in the first nt rows.
*     jC and jI are base indices for the Ct and It variables.

      jC    = nt
      jI    = nt*2

      do 200 k = 1, nt
         ne       = ne + 1
         ka(jC+k) = ne
         ha(ne)   = k
         a(ne)    = - one
  200 continue

*     Generate columns for Investment (It for t = 1 to nt).
*     They form -I in the first nt rows and -I in the last nt rows.

      do 300 k = 1, nt
         ne       = ne + 1
         ka(jI+k) = ne
         ha(ne)   = k
         a(ne)    = - one
         ne       = ne + 1
         a(ne)    = - one
         ha(ne)   = nt + k
  300 continue

*     ka(*) has one extra element.

      ka(n+1) = ne + 1

*     Set lower and upper bounds for Kt, Ct, It.
*     Also initial values and initial states for all variables.
*     The Jacobian variables are the most important.
*     Set hs(k) = 2 to make them initially superbasic.
*     The others might as well be on their smallest bounds (hs(j) = 0).

      do 400  k = 1, nt
         bl(   k) = 3.05d+0
         bu(   k) = bplus
         bl(jC+k) = 0.95d+0
         bu(jC+k) = bplus
         bl(jI+k) = 0.05d+0
         bu(jI+k) = bplus

         xn(   k) = 3.0d+0 + (k - 1)/10.0d+0
         xn(jC+k) = bl(jC+k)
         xn(jI+k) = bl(jI+k)

         hs(   k) = 2
         hs(jC+k) = 0
         hs(jI+k) = 0
  400 continue

*     The first Capital is fixed.
*     The last three Investments are bounded.

      bu(1)       = bl(1)
      xn(1)       = bl(1)
      hs(1)       = 0
      bu(jI+nt-2) = 0.112d+0
      bu(jI+nt-1) = 0.114d+0
      bu(jI+nt  ) = 0.116d+0

*     Set bounds on the slacks.
*     The nt nonlinear (Money)    rows are >=.
*     The nt    linear (CapacitY) rows are <=.
*     We no longer need to set initial values and states for slacks.

      jM     = n
      jY     = n + nt

      do 500    k = 1, nt
         bl(jM+k) = bminus
         bu(jM+k) = zero
         bl(jY+k) = zero
         bu(jY+k) = bplus

*-       xn(jM+k) = zero
*-       xn(jY+k) = zero
*-       hs(jM+k) = 0
*-       hs(jY+k) = 0
  500 continue

*     The last Money and Capacity rows have a Range.

      bl(jM+nt) = - 10.0d+0
      bu(jY+nt) =   20.0d+0

*     end of t4data
      end
