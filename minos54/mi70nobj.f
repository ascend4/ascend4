************************************************************************
*
*     File  mi70nobj fortran.
*
*     m7bsg    m7chkd   m7chkg   m7chzq   m7fixb
*     m7rg     m7rgit   m7sdir   m7sscv
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7bsg ( ms, nn, kb, gsub, grd )

      implicit           double precision (a-h,o-z)
      integer            kb(ms)
      double precision   gsub(nn), grd(ms)

*     ------------------------------------------------------------------
*     m7bsg   sets  grd = basic and superbasic components of  gsub.
*     ------------------------------------------------------------------

      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj

      parameter        ( zero = 0.0d+0 )

      do 20 k = 1, ms
         j    = kb(k)
         if (j .le. nn) then
            grd(k) = gsub(j)
         else
            grd(k) = zero
         end if
   20 continue

      if (iobj .ne. 0) grd(iobj) = - minimz * sclobj

*     end of m7bsg
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7chkd( n, bl, bu, x, dx, d, nfeas )

      implicit           double precision (a-h,o-z)
      double precision   bl(n), bu(n), x(n), dx, d(n)

*     ------------------------------------------------------------------
*     m7chkd  checks that x + dx*d is feasible.
*     It is used by m7chkg and m8chkj for the cheap gradient checks.
*     Original:    Just looked at the sign of d for variables on a bound.
*     13 Mar 1992: dx added as a parameter to make certain that
*                  x + dx*d does not lie outside the bounds.
*                  d may be altered to achieve this.
*     ------------------------------------------------------------------

      parameter        ( zero  = 0.0d+0 )

      nfeas  = 0
      do 500 j = 1, n
         xj    = x(j)
         b1    = bl(j)
         b2    = bu(j)
         if (b1   .eq. b2  ) d(j) = zero

         if (d(j) .ne. zero) then

*           x(j) is not fixed, so there is room to move.
*           If xj + dx*dj is beyond one bound, reverse dj
*           and make sure it is not beyond the other.
*           Give up and use set dj = zero if both bounds are too close.

            dj     = d(j)
            xnew   = xj  +  dx*dj

            if (dj .gt. zero) then
               if (xnew .gt. b2) then
                  dj     = - dj
                  xnew   =   xj  +  dx*dj
                  if (xnew .lt. b1) dj = zero
               end if
            else
               if (xnew .lt. b1) then
                  dj     = - dj
                  xnew   =   xj  +  dx*dj
                  if (xnew .gt. b2) dj = zero
               end if
            end if

            d(j)   = dj
            if (dj .ne. zero) nfeas  = nfeas + 1
         end if
  500 continue

*     end of m7chkd
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7chkg( n, bl, bu, g, g2,
     $                   x, da, db, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   bl(n), bu(n), da(n), db(n),
     $                   g(n), g2(n), x(n), z(nwcore)

*     ------------------------------------------------------------------
*     This routine checks that the gradient of an n-dimensional
*     function has been defined and programmed correctly.
*
*     First, a cheap heuristic test is performed, as in
*     subroutine chkgrd by the following authors:
*     Philip E. Gill, Walter Murray, Susan M. Picken and Hazel M. Barber
*     D.N.A.C., National Physical Laboratory, England  (circa 1975).
*
*     Next, a more reliable test is performed on each component of the
*     gradient, for indices in the range  jverif(1)  thru  jverif(2).
*
*     lverif(1) is the verify level, which has the following meaning:
*
*     -1         do not perform any check.
*      0         do the cheap test only.
*      1 or 3    do both cheap and full test on objective gradients.
*      2 or 3    do both cheap and full test on the jacobian.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5log1/ idebug,ierr,lprint
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8veri/ jverif(4),lverif(2)

      intrinsic          abs, max, min

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      logical            cheap, nodd
      character*4        key
      character*4        lbad        , lgood
      data               lbad/'bad?'/, lgood/'ok  '/

      lvl    = lverif(1)
      if (lvl .lt. 0) return

      j1     = max( jverif(1), 1 )
      j2     = min( jverif(2), n )
      cheap  = lvl .eq. 0  .or.  lvl .eq. 2  .or.  j1 .gt. j2
      lssave = lscale
      lscale = 0

*     Evaluate the function f and gradient g at the base point x.

      call m6dmmy( n, g )
      call m6fobj( 2, n, f, g, x, z, nwcore )
      if (ierr      .ne. 0) go to 900
      if (knowng(1) .eq. 0) go to 900

      if (iprint .gt. 0) then
         if ( cheap ) then
            write(iprint, 1100)
         else
            write(iprint, 1000)
         end if
      end if

*     --------------------------
*     Cheap test.
*     --------------------------

*     If n is odd, u is set to 1/(n - 1).  Otherwise, u = 1/n.

      nodd = 2*(n/2) .ne. n
      rn   = n
      r    = one/rn
      u    = r
      if (nodd  .and.  n .ne. 1) u = one/(rn - one)

*     Set arrays da and db to be (almost) orthogonal.
*     We must not perturb x(j) if g(j) is unknown.

      call dload ( n, zero, da, 1 )
      call dload ( n, zero, db, 1 )
      do 20 j  = 1, n
         if (g(j) .eq. gdummy) go to 20
         db(j) = r
         da(j) = u
   20 continue

      do 30 j  = 1, n, 2
         if (da(j) .ne. zero) da(j) = - da(j)  -  j / rn
   30 continue
      if (nodd) da(n) = zero

*     Define a difference interval.
*     Make sure da and db are feasible directions.

      dx     = difint(1) * (one + dnorm1( n, x, 1 ))
      call m7chkd( n, bl, bu, x, dx, da, nfeas1 )
      call m7chkd( n, bl, bu, x, dx, db, nfeas2 )

      if (nfeas1 + nfeas2 .eq. 0) then
         if (iprint .gt. 0) write(iprint, 1200)
      else

*        Set w1 = g(t)*da and w2 = g(t)*db.

         w1     = ddot  ( n, da, 1, g, 1 )
         w2     = ddot  ( n, db, 1, g, 1 )

*        Make a forward-difference approximation to the gradient
*        along da and db.

         do 40 i  = 1, n
            da(i) = x(i) + dx*da(i)
   40    continue
         f1     = f
         if (n    .ne. 1) call m6fobj( 0, n, f1, g2, da, z, nwcore )
         if (ierr .ne. 0) go to 900
         v1     = (f1 - f)/dx

         do 50 i  = 1, n
            db(i) = x(i) + dx*db(i)
   50    continue
         call m6fobj( 0, n, f2, g2, db, z, nwcore )
         if (ierr .ne. 0) go to 900
         v2     = (f2 - f)/dx

*        c1 and c2 are the differences between approximated and
*        programmed gradient projected along da and db respectively.

         c1     = v1 - w1
         c2     = v2 - w2

*        Set an error indicator if c1 or c2 is too large.

         ifail  = 0
         if (c1*c1 .ge. dx*(w1*w1 + one)  .or.
     $       c2*c2 .ge. dx*(w2*w2 + one)) ifail = 2

         if (ifail .eq. 0) then
            if (iprint .gt. 0) write(iprint, 1400)
         else
            if (iprint .gt. 0) write(iprint, 1500)
            if (isumm  .gt. 0) write(isumm , 1500)
         end if
         if (iprint .gt. 0) write(iprint, 1600) w1,w2,v1,v2
      end if

*     ------------------------------------------------------------------
*     Check each component by differencing along
*     the coordinate directions.
*     Don't bother printing a line if it looks like an exact zero.
*     ------------------------------------------------------------------

      if (cheap) go to 900
      if (iprint .gt. 0) write(iprint, 2000)
      top    =  (one + abs( f ))*eps2
      emax   = - one
      jmax   = 0
      nwrong = 0
      ngood  = 0

      do 200 j = j1, j2
         xj    = x(j)
         gj    = g(j)
         if (gj .eq. gdummy) go to 200
         gabs  = one + abs( gj )
         dx    = top / gabs
         x(j)  = xj + dx
         call m6fobj( 0, n, fforwd, g2, x, z, nwcore )
         if (ierr .ne. 0) go to 900

         gdiff = (fforwd - f) / dx
         err   = abs( gdiff - gj ) / gabs

         if (emax .lt. err) then
            emax  = err
            jmax  = j
         end if

         key   = lgood
         if (err .gt. eps5) key = lbad
         if (key .eq. lbad) nwrong = nwrong + 1
         if (key .eq.lgood) ngood  = ngood  + 1
         if (abs( gj ) + err  .gt.  eps0) then
            if (iprint .gt. 0) write(iprint, 2100) j,xj,dx,gj,gdiff,key
         end if
         x(j)  = xj
  200 continue

      if (iprint .gt. 0) then
         if (nwrong .eq. 0) then
            write(iprint, 2500) ngood ,j1,j2
         else
            write(iprint, 2600) nwrong,j1,j2
         end if
         write(iprint, 2700) emax,jmax
      end if
      if (emax .lt. one) go to 900

*     Bad gradients in  funobj.

      ierr   = 7
      call m1envt( 1 )
      if (iprint .gt. 0) write(iprint, 3700)
      if (isumm  .gt. 0) write(isumm , 3700)

*     Exit.

  900 lscale = lssave
      return

 1000 format(/// ' Verification of objective gradients',
     $   ' returned by subroutine funobj.')
 1100 format(/ ' Cheap test on funobj...')
 1200 format(/ ' XXX  Can''t find a feasible step -',
     $   ' objective gradients not verified.')
 1400 format(  ' The objective gradients seem to be OK.')
 1500 format(  ' XXX  The objective gradients seem to be incorrect.')
 1600 format(  ' Gradient projected in two directions', 1p, 2e20.11,
     $       / ' Difference approximations           ', 2e20.11)
 2000 format(// 6x, 'j', 7x, 'x(j)', 8x, 'dx(j)',
     $   11x, 'g(j)', 9x, 'Difference approxn' /)
 2100 format(i7, 1p, e16.8, e10.2, 2e18.8, 2x, a4)
 2500 format(/ i7, '  objective gradients out of', i6, '  thru', i6,
     $         '  seem to be OK.')
 2600 format(/ ' XXX  There seem to be', i6,
     $   '  incorrect objective gradients in cols', i6, '  thru', i6)
 2700 format(/ ' XXX  The largest relative error was', 1p, e12.2,
     $   '   in column', i6 /)
 3700 format(// ' EXIT -- subroutine funobj appears to be',
     $   ' giving incorrect gradients')

*     end of m7chkg
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7chzq( m, nb, ms, ns, Rset, jq, pivot,
     $                   ne, nka, a, ha, ka,
     $                   kb, bl, bu, x, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            Rset
      integer*4          ha(ne)
      integer            ka(ne), kb(ms)
      double precision   a(ne), bl(nb), bu(nb), x(ms), y(ms), z(nwcore)

*     ------------------------------------------------------------------
*     m7chzq  selects a superbasic to replace the jp-th basic variable.
*     On entry, y(1:m) contains the jp-th row of B(inverse).
*     On exit,  if Rset is true, y(S) = y(m1:ms)  holds the vector v
*               needed by m6bswp to update R following a basis change.
*
*     29 Nov 1991: qnewtn changed to Rset (should fix a bug).
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm

      intrinsic          abs, min
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     Set  y(S) = 0 - S(t)*y.  Beware of the minus sign when using y(S).

      m1     = m + 1
      call dload ( ns, zero, y(m1), 1 )
      call m2aprd( 4, y, m, y(m1), ns,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )
      jq     = m  +  idamax( ns, y(m1), 1 )
      pivot  = abs( y(jq) )

*     Exit if the pivot is too small.

      if (pivot .lt. tolpiv) then
         if (iprint .gt. 0) then
            write(iprint, '(/ a, 1p, e11.1)')
     $         ' XXX  m7chzq.  Max pivot is too small:', pivot
         end if
         jq     = - ms
      else

*        Choose one away from its bounds if possible.

         tol    =   0.1*pivot
         dmax   = - one

         do 200 k = m1, ms
            if (abs( y(k) ) .ge. tol) then
               j     = kb(k)
               xj    = x(k)
               d1    = xj - bl(j)
               d2    = bu(j) - xj
               d1    = min( abs( d1 ), abs( d2 ) )
               if (dmax .le. d1) then
                  dmax  = d1
                  jq    = k
               end if
            end if
  200    continue

         pivot = - y(jq)

*        Finish computing v, the vector needed to modify R.

         if ( Rset ) then
            y(jq) = - (one + pivot)
            call dscal ( ns, (one/pivot), y(m1), 1 )
         end if
      end if

*     end of m7chzq
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7fixb( m, maxr, ms, n, nb, nr, ns, nx, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, bl, bu, bbl, bbu,
     $                   r, x, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(ne), kb(ms)
      double precision   a(ne), bl(nb), bu(nb), bbl(ms), bbu(ms)
      double precision   r(nr), x(ms), y(ms), y2(nx), z(nwcore)

*     ------------------------------------------------------------------
*     m7fixb looks to see if the current basis B is ill-conditioned.
*     We assume that the factorization B = L*U has just been computed.
*     If the smallest diagonal of U seems too small, a superbasic is
*     chosen to be swapped with the corresponding column of B.
*     The reduced Hessian R is updated accordingly.
*
*     On exit,
*     inform = 0 means no error condition.
*     inform = 5 means the LU update failed.
*
*     15 Mar 1992: First version, derived from bits of m7chzq, m7rgit.
*     18 Mar 1992: If we call m7fixb only after m2bfac, we don't need
*                  to make it update grd, pi and rg.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2lu4 / parmlu(30),luparm(30)
      common    /m2parm/ dparm(30),iparm(30)
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      common    /m5lp2 / invrq,invitn,invmod

      logical            Rset, swap
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     LUSOL gives umax  = the largest element of U,
*                 dumin = the smallest diagonal of U,
*             and jumin = the corresponding column of B.

      inform = 0
      umax   = parmlu(12)
      dumin  = parmlu(14)
      jumin  = luparm(19)

*     Since modifying B is an expensive operation,
*     we don't do anything if dumin seems reasonably large.
*     The SPECS file gives swaptl = Swap tolerance = eps4 by default.
*     NOTE: If B has rank 0, dumin = umax = 0.
*           If B has rank 1, dumin = umax.
*           Hence we need both relative and absolute tests.

      swaptl = dparm(8)
      if (dumin .gt. swaptl * (umax + one)) return

*     U looks ill-conditioned.  We want to replace the jumin-th column.
*     Use jp = jumin for brevity, and for the final call to m2bsol.

      inform = 1
      jp     = jumin
      Rset   = r(1) .ne. zero
      m1     = m + 1
      nz1    = min( ns, maxr )

*     ------------------------------------------------------------------
*     Solve  B(t)*y = e(jp).
*     ------------------------------------------------------------------
      call dload ( m, zero, y2, 1 )
      y2(jp) = one       
      call m2bsol( 3, m, y2, y, z, nwcore )

*     ------------------------------------------------------------------
*     Use y to "price" the superbasics.
*     For each column of S, y(S) returns (minus) the pivot element
*     that would arise if that column replaced the jumin-th basic.
*     ------------------------------------------------------------------

*     Set  y(S) = 0 - S(t)*y.  Beware of the minus sign when using y(S).

      call dload ( ns, zero, y(m1), 1 )
      call m2aprd( 4, y, m, y(m1), ns,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )
      kq     =   idamax( ns, y(m1), 1 )
      jq     =   m + kq
      pivot  = - y(jq)

*     Continue if pivot is noticeably larger than dumin.
*     It is not clear that this the best test, but it will do for now.

      cond0  = cond
      dtol   = 2.0d+0
      swap   = abs( pivot ) .ge. dtol*dumin

      if ( swap ) then
         if (Rset  .and.  kq .le. maxr) then

*           Finish computing v, the vector needed to modify R.
*           v is stored in y(S) for use in m6bswp.

            y(jq) = - (one + pivot)
            call dscal ( ns, (one/pivot), y(m1), 1 )

*           Modify R to account for the change in basis.
*           y2 is workspace.
*           Get cond(R) before and after to see how we're doing.

            call m6rcnd( maxr, nr, ns, r, dmax, dmin, cond0 )
            call m6bswp( nz1, nr, r, y2, y(m1), kq,
     $                   eps0, eps2, modr2 )
            call m6rcnd( maxr, nr, ns, r, dmax, dmin, cond )

*           We may want to cancel the swap if the new R
*           seems a lot worse than before.
*           At present we don't know enough about it, so let it go.

            worse  = 1.0d+3
            if (cond .gt. worse*cond0) then
*---           swap   = .false.
            end if

*           If cond is now pretty bad, set R = I.
*           (Alternatively we could set swap = .false.
*           and restore the old R by calling m6bswp.)

            if (cond .ge. one/eps1) then
               r(1) = zero
               call m6rset( maxr, nr, ns, r, cond )
            end if
         end if

*        Swap columns jp and jq of (B S).

         if ( swap ) then
            jr1     = kb(jp)
            jq1     = kb(jq)
            kb(jp)  = jq1
            kb(jq)  = jr1
            hs(jq1) = 3
            hs(jr1) = 2

            if (iprint .gt. 0)
     $      write(iprint, 1000) jr1, jq1, dumin, pivot, cond0, cond
            if (isumm  .gt. 0)
     $      write(isumm , 1000) jr1, jq1, dumin, pivot, cond0, cond

            t       = bbl(jp)
            bbl(jp) = bbl(jq)
            bbl(jq) = t

            t       = bbu(jp)
            bbu(jp) = bbu(jq)
            bbu(jq) = t

            t       = x(jp)
            x(jp)   = x(jq)
            x(jq)   = t

*           Set  y2  for modifying L and U.   L*y2 = a(jq1).
*           Then modify L and U, using jp in /m5log3/.

            call m2unpk( jq1, m, n, ne, nka, a, ha, ka, y2 )
            call m2bsol( 1, m, y2, y, z, nwcore )
            call m2bsol( 4, m, y2, y, z, nwcore )
            if (invrq .ne. 0) inform = 5
         end if
      end if

      return

 1000 format(1p, ' Swap', 2i6, '   Diag, pivot =', 2e9.1,
     $       '   cond(R) =', 2e9.1)

*     end of m7fixb
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7rg  ( m, ms, ns, grd, pi, rg, rgnorm,
     $                   ne, nka, a, ha, ka,
     $                   z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), grd(ms), pi(m), rg(ns), z(nwcore)

*     ------------------------------------------------------------------
*     m7rg    calculates the reduced gradient  rg = g(S) - S(t)*pi.
*     ------------------------------------------------------------------

      intrinsic          abs, min
      external           idamax

      call dcopy ( ns, grd(m+1), 1, rg, 1 )
      call m2aprd( 4, pi, m, rg, ns,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )
      kmax   = idamax( ns, rg, 1 )
      rgnorm = abs( rg(kmax) )

*     end of m7rg
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7rgit( m, maxr, maxs, mbs, n, nb, incres,
     $                   nn, nn0, nr, ns, nx, inform, nxtphs,
     $                   ne, nka, a, ha, ka,
     $                   hrtype, hs, kb, bl, bu, bbl, bbu,
     $                   fsub, gsub, grd, grd2,
     $                   pi, r, rg, rg2, x, xn, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            incres
      integer*4          ha(ne), hrtype(mbs), hs(nb)
      integer            ka(nka), kb(mbs)
      double precision   a(ne), bl(nb), bu(nb), bbl(mbs), bbu(mbs),
     $                   gsub(nn0), grd(mbs), grd2(mbs),
     $                   pi(m), r(nr), rg(maxs), rg2(maxs),
     $                   x(mbs), xn(nb), y(nx), y2(nx), z(nwcore)

*     ------------------------------------------------------------------
*     m7rgit performs an iteration of the reduced-gradient algorithm.
*
*     incres in Phase 3 says if the new variable should increase or not.
*            It is used only in linear mode when m5pric is moving free
*            nonbasics toward their closest bound (and djq = zero).
*
*     Rset   says if a useful quasi-newton  r  exists.
*            It will be false if a feasible point has not yet been
*            found.
*            Note that Rset could be true even if the current itn
*            is infeasible.  This allows  r  to be updated in
*            certain ways during temporary loss of feasibility.
*
*     qnewtn in this version (Jan 1983) is the same as  nonlin.
*
*     parhes is true if  r  is not big enough to store a full
*            quasi-newton estimate of the projected hessian.
*            The null space is then  z = ( z1  z2 ),  and  r  estimates
*            the Hessian only in the subspace  z1.  A diagonal estimate
*            is used (also in  r) for the subspace  z2.
*
*     fullz  is true if the search direction for the superbasics is
*            computed using all of the reduced gradients.  Otherwise,
*            the components corresponding to  z2  are zero.
*
*     grd2   is not used in this version (Aug 1986).
*
*     xx Sep 1987: Anti-degeneracy m5chzr incorporated.
*     29 Sep 1991: a, ha, ka etc. passed in.  Argument list altered.
*     04 Oct 1991: switch and difint added as arguments to m6srch.
*     08 Apr 1992: hs(*) now has internal values.  We have to be more
*                  careful in setting jrstat, the state of a blocking
*                  variable.
*     15 Apr 1992: incres added as input parameter for linear Phase 3
*                  when djq = 0.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2parm/ dparm(30),iparm(30)
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log2/ jq1,jq2,jr1,jr2,lines1,lines2
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lp2 / invrq,invitn,invmod
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      logical            conv
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7phes/ rgmin1,rgnrm1,rgnrm2,jz1,jz2,labz,nfullz,mfullz
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
*     ------------------------------------------------------------------

      intrinsic          abs, max, min, sqrt

      logical            feasbl, infsbl, linear, nonlin,
     $                   fullz , parhes, qnewtn, Rset  ,
     $                   debug , fonly , grdcon, grdobj,
     $                   hitlow, move  , onbnd , switch, unbndd,
     $                   unbddf, unbddx, uncon , vertex

      parameter        ( zero   = 0.0d+0,  one    = 1.0d+0,
     $                   point1 = 0.1d+0,  point9 = 0.9d+0 )

      character*19       msg(4:8)
      data               msg/'max step too small.',
     $                       'step too small.    ',
     $                       'no minimizer.      ',
     $                       'too many functions.',
     $                       'uphill direction.  '/

      inform = 0
      m1     = m + 1
      tolz   = eps0
      feasbl = ninf .eq. 0
      infsbl = .not. feasbl
      linear = infsbl  .or.  nn .eq. 0
      nonlin = .not. linear
      qnewtn = nonlin
      Rset   = r(1) .ne. zero
      unbndd = .false.
      bplus  = point1 * plinfy
      toobig = sqrt(plinfy)
      oldfx  = fsub
      if ( qnewtn ) then
         if (.not. Rset) then
            call m6rset( maxr, nr, ns, r, cond )
            Rset = .true.
         end if
      end if

      if (nphs .eq. 3) then
*        ---------------------------------------------------------------
*        Phase 3.      First, make sure we dont want to stay in Phase 4.
*        Since price can select nonbasics floating free between their
*        bounds with zero reduced cost, we have to check that dqj is
*        not zero.
*        ---------------------------------------------------------------
         if (linear) then
            if (djq .eq. zero) then
               djq      = one
               if (incres) djq = - one
               rg(ns+1) = djq
            end if
         end if
               
         jq2    = jq
         djqmod = abs( djq )
         ratio  = rgnorm / djqmod

         if (      linear              ) go to 100
         if (ns     .eq. 0             ) go to 100
         if (nfail  .gt. 0             ) go to 100
         if (ratio  .le. point9        ) go to 100
         if (rgnorm .le. gtol(2)*pinorm) go to 100

            tolrg  =  point9 * rgnorm
            jq2    = -jq
            nphs   =  4
            go to 200

*        Add the superbasics selected during pricing.
*        The last newsb entries in kb and rg have been set by m5pric.

  100    nfullz = 0
         jqstat = hs(jq)
         rgnorm = max( rgnorm, djqmod )
         tolrg  = etarg * djqmod

*        NOTE.  The above line sets the level to which rgnorm
*        must be reduced in the current subspace (Phase 4)
*        before we consider moving off another constraint (Phase 3).
*        etarg between (0, 1) is set by the user at his/her own peril.
*        It is the Subspace tolerance in the SPECS file.

         do 130 j = 1, newsb
            ns         = ns + 1
            ms         = m + ns
            kq         = kb(ms)
            hs(kq)     = 2
            grd(ms)    = zero
            if (kq .le. nn  .and.  feasbl) grd(ms) = gsub(kq)
            hrtype(ms) = 0
            bbl(ms)    = bl(kq)
            bbu(ms)    = bu(kq)
            x(ms)      = xn(kq)
            if (Rset) call m6radd( maxr, nr, ns, r )
  130    continue

      else
*        ---------------------------------------------------------------
*        Phase 4.      Exit if rgnorm is already very small.
*        ---------------------------------------------------------------
         if (rgnorm  .le.  point1 * toldj(3) * pinorm) go to 910
      end if


*     ==================================================================
*     Get a search direction ys for the superbasics (in y(m1)...y(ms))
*     and then a search direction y for the basics.
*     ==================================================================
  200 ms     = m + ns
      nssave = ns
      vertex = ns .eq. 1
      fullz  = .true.
      parhes = nonlin  .and.  ns .gt. maxr
      nz1    = ns
      if (parhes) nz1 = maxr
      nz2    = ns - maxr
      lastr  = maxr*(maxr+1)/2

*     Compute the search direction for the superbasics.
*     rg2  is used for the search direction, but it is promptly
*     copied into  y(m1)  below.
*     If  r  is being updated,  m7sdir  saves w such that R(t)*w = rg.
*     w  is used later by  m6bfgs.

      mode   = 0
      if (nonlin) mode = 1
      call m7sdir( mode, maxr, nr, ns, r, rg, rg2, z(lw), z, nwcore )
      call dcopy ( ns, rg2, 1, y(m1), 1 )

      if ( parhes ) then

*        Partial Hessian.  Let all superbasics move for the first
*        mfullz iterations, but then move only the first set.

         mfullz = 3
         nfullz = nfullz + 1
         if (nfullz .gt. mfullz) then
            fullz  = .false.
            labz   = 2
            j      = m1 + maxr
            call dload ( nz2, zero, y(j), 1 )
         end if
      end if

*     ==================================================================
*     The search direction for the superbasics, ys, is now in
*     y(m+1), ..., y(m+ns).
*     Find norms of xs and ys.
*     ==================================================================
      xsnrm1 = dasum( nz1, x(m1), 1 )
      ysnrm1 = dasum( nz1, y(m1), 1 )
      if (parhes) then
         xsnrm2 = dasum( nz2, x(m1+maxr), 1 )
         ysnrm2 = dasum( nz2, y(m1+maxr), 1 )
      else
         xsnrm2 = zero
         ysnrm2 = zero
      end if
      xsnorm = xsnrm1 + xsnrm2
      ysnorm = ysnrm1 + ysnrm2

*     Compute  y2 = - S*ys and prepare to solve  B*y = y2
*     to get y, the search direction for the basics.
*     We first normalize y2 so the LU solver won't ignore
*     too many "small" elements while computing y.

      call dscal ( ns, (one / ysnorm), y(m1), 1 )
      call dload ( m, zero, y2, 1 )
      call m2aprd( 2, y(m1), ns, y2, m,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )

*     Solve  B*y = y2  and then unnormalize all of y.

      call m2bsol( 2, m, y2, y, z, nwcore )
      call dscal ( ms, ysnorm, y, 1 )
      ynorm  = dasum( m, y, 1 )  +  ysnorm
      xnorm  = dasum( m, x, 1 )  +  xsnorm

*     ------------------------------------------------------------------
*     Find the nearest constraint in direction  x + theta*y  (theta>=0).
*     With the present version of m5chzr (Sep 1987), theta is always
*     positive.
*     Exact  is the step that takes x(jp) exactly onto bound.
*     It may be positive or slightly negative. (Not defined if unbndd.)
*
*     For the linesearch, theta becomes stepmx, the largest
*     step that the linesearch is allowed to take.
*
*     If exact is positive, or if we are at a vertex, we do a linesearch
*     to find a new step theta in the range 0 lt theta le stepmx.
*     Sometimes, this interval may be too small for the search to be
*     successful.  One thing to remember is that increasing the
*     feasibility tolerance has the effect of increasing stepmx,
*     and therefore increasing the chance of a successful search.
*
*     If exact isn't positive and we are not at a vertex, we change
*     theta and stepmx to zero and don't move.
*
*     If onbnd  is true, theta is a step that reaches a bound exactly.
*     x(jp) reaches the value bound.  If the linesearch says to
*     take a constrained step, bound is used to put the new nonbasic
*     variable xn(jr) exactly on its bound.
*
*     If unbndd is true, theta = stepmx.
*     ------------------------------------------------------------------

*xxx  stepmn = eps1    / (one + ynorm)  > 9 mar 1988: these are wrong if
*xxx  stepmx = 1.0d+12 / (one + ynorm)  > ynorm is small (john stone).
      stepmx = 1.0d+12 /  ynorm
      tolp   = tolpiv  *  ynorm

      call m5chzr( ms    , stepmx, plinfy, tolp  ,
     $             hrtype, bbl   , bbu   , x     , y,
     $             hitlow, move  , onbnd , unbndd,
     $             jp    , bound , exact , theta )

      if (.not. unbndd) then
         pivot  = - y(jp)
         stepmx =   theta
         jr     =   kb(jp)
      end if

      if (unbndd  .or.  vertex  .or.  exact .gt. zero) then
*        ===============================================================
*        A move looks possible.
*        If linear, do a normal update to  x  and skip the linesearch.
*        ===============================================================
         if ( linear ) then
            if ( unbndd ) go to 955
            call daxpy ( ms, theta, y, 1, x, 1 )
            call m5bsx ( 1, ms, nb, kb, x, xn )
            go to 500
         end if
      else
*        ===============================================================
*        Zero step.
*        The blocking variable x(jp) is currently on its bound or
*        slightly infeasible.  We shall make it nonbasic and not move.
*        The iteration proceeds as a constrained step with theta = zero
*        and again we skip the linesearch.
*
*        NOTE: If it is Phase 3 and the blocking variable is the one
*        we have just brought in, we would have to go back and try again
*        without it, to avoid "Cycling in Phase 3".
*        However, since r is expanded with a unit vector,
*        cycling of this kind can never happen.  Hence, we no longer
*        guard against it.
*        ===============================================================
         theta  = zero
         stepmx = zero
         onbnd  = .false.
         go to 500
      end if

*     ------------------------------------------------------------------
*     Perform a linesearch to find a downhill point  (x = x + theta*y).
*     switch tells m6srch whether there is an option to switch to
*     central differences to get a better search direction.
*
*     m6srch returns the following values:
*
*     inform =-1 (and ierr = 6) if the user wants to stop.
*     inform = 1 if the search is successful and theta < stepmx.
*            = 2 if the search is successful and theta = stepmx.
*            = 3 if a better point was found but too many functions
*                were needed (not sufficient decrease).
*            = 4 if stepmx < tolabs (too small to do a search).
*            = 5 if theta  < alfsml (srchq only -- maybe want to switch
*                to central differences to get a better direction).
*            = 6 if the search found that there is no useful step.
*                The interval of uncertainty is less than 2*tolabs.
*                The minimizer is very close to theta = zero
*                or the gradients are not sufficiently accurate.
*            = 7 if there were too many function calls.
*            = 8 if the input parameters were bad
*                (stepmx le toltny  or  uphill).
*     ------------------------------------------------------------------
      debug  = itn .ge. iparm(2)
      grdcon = nncon .eq. 0  .or.  lderiv .ge. 2
      grdobj = nnobj .eq. 0  .or.  lderiv .eq. 1  .or.  lderiv .eq. 3
      fonly  = .not. (grdcon  .and.  grdobj)
      switch = lvldif .eq. 1
     $           .and.
     $         ((.not. grdobj  .and.  knowng(1) .lt. nnobj)  .or.
     $          (.not. grdcon  .and.  knowng(2) .lt. njac )      )

      epsrf  = dparm(3)
      damp   = dparm(6) * (one + xnorm) / ynorm
      theta  = min( one, damp )

      call m6srch( ms, ns, n, nb, nn, itn, inform, 
     $             debug, fonly, switch,
     $             ne, nka, a, ha, ka,
     $             theta, stepmx, difint(1), eps , epsrf, etash,
     $             fsub , rgnorm, ynorm, xnorm,
     $             gsub , grd, y, x, y2, xn, z, nwcore )

      if (inform .lt. 0) return
      if (inform .le. 3) go to 500

      if (inform .eq. 4) then
*        ---------------------------------------------------------------
*        The linesearch says stepmx is too small.
*        (Function precision affects when this happens.)
*
*        If m5chzr set move false, stepmx is very small and we probably
*        should not have attempted the linesearch.  Rather than taking a
*        zero step (which might lead to cycling if vertex is true) we
*        force a step of stepmx regardless of the effect on the
*        objective.  The step will be constrained.
*        onbnd has the correct value.
*
*        Otherwise, we treat it as a linesearch failure
*        and try for a better direction, possibly with more superbasics.
*        ---------------------------------------------------------------
         if ( unbndd ) go to 440
         if ( move   ) go to 440

         theta  = stepmx
         modefg = 2
         call daxpy ( ms, theta, y, 1, x, 1 )
         call m6fun ( 0, modefg, n, nb, ms, fsub,
     $                ne, nka, a, ha, ka,
     $                x, xn, z, nwcore )
         call m6fun ( 1, modefg, n, nb, ms, fsub,
     $                ne, nka, a, ha, ka,
     $                x, xn, z, nwcore )
         if (ierr .ne. 0) return

         call m6grd ( ms, nb, nn, gsub, grd,
     $                ne, nka, a, ha, ka,
     $                xn, z, nwcore )
         t      = stepmx * ynorm
         if (iprint .gt. 0) write(iprint, 1075) t
         go to 500
      end if

*     ------------------------------------------------------------------
*     See if we should switch to central differences.
*     ------------------------------------------------------------------
      if (inform .eq. 5) then
         if (switch  .and.  lvltol .eq. 2) go to 920
      end if

*     ------------------------------------------------------------------
*     Trouble -- no function decrease.
*     Try resetting the Hessian, deleting constraints,
*     and finally refactorizing the basis.
*     Give up after 8 consecutive failures.
*     ------------------------------------------------------------------
  440 if (inform .ge. 4) then
         if (iprint .gt. 0)
     $      write(iprint, 1050) inform, msg(inform), itn, rgnorm
         if (isumm  .gt. 0)
     $      write(isumm , 1050) inform, msg(inform), itn, rgnorm
      end if
      if (inform .eq. 5  .and.  nfail .lt. 1) nfail = 1
      inform = 0

*     This is the top of the loop through various recovery procedures.

  450 nfail  = nfail + 1
      if ( prnt1 ) write(iprint, 1090) nfail

      if (nfail .eq. 1) then
         if ( qnewtn  .and.  cond .ge. one/eps1 ) then
            call m6rset( maxr, nr, ns, r, cond )
            go to 200
         end if

      else if (nfail .eq. 2) then

*        Switch to central differences if we have not yet done so
*        and if we're trying to minimize accurately.

         if (switch  .and.  lvltol .eq. 2) go to 920

      else if (nfail .le. 5) then

*        Ask for price (up to 3 times) if rgnorm is not too big.

         if (nfail  .ge. 5   .and.  nonopt .eq. 0) go to 475
         if (ns     .ge. maxs                    ) go to 475
         t      = 10.0
         if (lvltol .eq. 2) t = one
         if (rgnorm .gt. t * pinorm              ) go to 475
         go to 910

  475    nfail  = 5

      else if (nfail .eq. 6) then

*        Switch to central differences even if we're not trying to
*        minimize accurately.

         if (switch  .and.  lvltol .eq. 1) go to 920

      else if (nfail .eq. 7) then

*        Request refactorization of the basis (inform = 5).

         if (invitn .gt. 0) go to 925

      else if (nfail .eq. 8) then

*        Request a change of basis as a last resort (inform = 6).

         go to 930
      end if

      if (nfail .lt. 8) go to 450

*     Can't think what to do now other than stop.

      if (ns .eq. maxs) go to 960
      ierr   = 9
      return

*     ------------------------------------------------------------------
*     We got past the linesearch (or didn't have to do one).
*     See if the step is unbounded, unconstrained, zero, or otherwise.
*     ------------------------------------------------------------------
  500 inform = 0
      unbddf = abs( fsub )  .ge. dparm(1)
      unbddx = theta*ynorm  .ge. dparm(2)
      unbndd = unbddf       .or. unbddx
      uncon  = nonlin  .and.  theta .lt. stepmx
      if (unbndd) go to 950

*     ==================================================================
*     Get the new reduced gradient rg2.
*     ==================================================================
      modr1  = 0
      modr2  = 0
      if (linear  .or.  theta .eq. zero) then
         call dcopy ( ns, rg, 1, rg2, 1 )
      else
         call dcopy ( m, grd, 1, y, 1 )
         call m5setp( 1, m, y, pi, z, nwcore )
         call m7rg  ( m, ms, ns, grd, pi, rg2, rgnorm,
     $                ne, nka, a, ha, ka,
     $                z, nwcore )

*        Update the reduced Hessian.

         if (qnewtn) then
            nsmove = ns
            if (parhes  .and.  .not. fullz) nsmove = maxr
            call m6bfgs( maxr, nsmove, nr, r, rg, rg2, y(m1), z(lw),
     $                   theta, eps2, eps0, modr1 )
         end if
      end if

*     ------------------------------------------------------------------
*     Update the active constraint set if necessary, and modify R,
*     the Cholesky factorization of the approximate reduced Hessian.
*     ------------------------------------------------------------------
      if (  uncon  ) then
*        ===============================================================
*        The step is unconstrained.
*        ===============================================================
         pivot  = zero
         kmodlu = 0
         call dcopy ( ns, rg2, 1, rg, 1 )

      else
         if (jp .eq. 0) go to 955

*        ===============================================================
*        There is a blocking variable.
*        It could be a fixed variable, whose new state must be 4.
*        ===============================================================
         if (bbl(jp) .eq. bbu(jp)) then
            jrstat = 4
         else if (hitlow) then
            jrstat = 0
         else
            jrstat = 1
         end if
         if (  onbnd  ) xn(jr) = bound

         if (jp .le. m) then
*           ============================================================
*           A variable in B hit a bound.
*           Find a column in S to replace it.
*           ============================================================

*           Solve  B(t)*y = e(jp).

            call dload ( m, zero, y2, 1 )
            y2(jp) = one
            call m2bsol( 3, m, y2, y, z, nwcore )

*           Select a superbasic to become basic.
*           If Rset is true, y(S) is returned for use in m6bswp.

            call m7chzq( m, nb, ms, ns, Rset, jq, pivot,
     $                   ne, nka, a, ha, ka,
     $                   kb, bl, bu, x, y, z, nwcore )

            if (jq .le. 0) go to 940
            hs(jr)  = jrstat
            jr1     = jr
            jr2     = kb(jq)
            jq1     = jr2
            kb(jp)  = jr2
            bbl(jp) = bbl(jq)
            bbu(jp) = bbu(jq)
            grd(jp) = grd(jq)
            x(jp)   = x(jq)
            kq      = jq - m

*           Modify R to account for the change in basis.
*           y2 is workspace.

            if (Rset  .and.  ns .gt. 1  .and.  kq .le. maxr) then
               call m6bswp( nz1, nr, r, y2, y(m1), kq,
     $                      eps0, eps2, modr2 )
            end if

*           Modify  pi  using  y  where  B(t)*y = e(jp).

            t      = rg2(kq) / pivot
            call daxpy ( m, t, y, 1, pi, 1 )
            pinorm = dnorm1( m, pi, 1 )
            pinorm = max( pinorm, one )

*           Set  y2  for modifying L and U.

            hs(jq1) = 3
            call m2unpk( jq1, m, n, ne, nka, a, ha, ka, y2 )
            call m2bsol( 1, m, y2, y, z, nwcore )

         else
*           ============================================================
*           A variable in S hit a bound.
*           ============================================================
            hs(jr) = jrstat
            jr2    = jr
            kmodlu = 0
            kq     = jp - m
         end if

*        ===============================================================
*        If necessary, swap the largest reduced-gradient in  Z2  into
*        the front of  Z2,  so it will end up in the end of  Z1.
*        ===============================================================
         call m7rg  ( m, ms, ns, grd, pi, rg, rgnorm,
     $                ne, nka, a, ha, ka,
     $                z, nwcore )

         rgdel  = abs( rg(kq) )
         if ( parhes  .and.  kq .le. maxr ) then
            call m6swap( m, maxr, nr, ns, ms, kb, bbl, bbu,
     $                   grd, r, rg, x )
         end if

*        Delete the kq-th superbasic, updating R if it exists.

         call m6rdel( m, maxr, nr, ns, ms, kb, bbl, bbu,
     $                grd, r, rg, x, kq, Rset )
         ns     = ns - 1
         ms     = m + ns
         nssave = ns
         nfullz = 0

         if (rgnorm .le. rgdel) then
            rgnorm = zero
            if (ns  .gt.   0  ) then
               imax   = idamax( ns, rg, 1 )
               rgnorm = abs( rg(imax) )
            end if
         end if
      end if

*     ------------------------------------------------------------------
*     Estimate the condition of  R(t)*R  using the diagonals of  R.
*     ------------------------------------------------------------------
      if ( Rset  .and.  ns .gt. 0 ) then
         call m6rcnd( maxr, nr, ns, r, dmax, dmin, cond )

         if (infsbl) then
*           Infeasible.  If cond is pretty big, throw  R  away.

            if (cond .ge. one/eps2) r(1) = zero
         else
*           Feasible.
*           If cond is hoplessly big, try a basis change.
*           If cond is big but not quite that big, try modifying it.

            if (cond .ge. one/eps0) go to 930
            if (cond .ge. one/eps1) call m6rset( maxr, nr, ns, r, cond )
         end if
      end if

*     ==================================================================
*     Test for convergence in the current subspace.
*     ==================================================================
      nfail  = 0
      parhes = nonlin  .and.  ns .gt. maxr
      call m7sscv( m, maxr, maxs, ms, nr, ns, parhes, nxtphs,
     $             fsub, oldfx, theta, xsnorm, xsnrm1, ysnorm, ysnrm1,
     $             kb, bbl, bbu, grd, r, rg, x )

*     If no room for more superbasics, try to stay in phase 4.

      if (nxtphs .eq. 3  .and.  ns .eq. maxs) nxtphs = 4
      go to 990

*     ------------------------------------------------------------------
*     Various exits.
*     ------------------------------------------------------------------

*     rgnorm is small -- return to Phase 3.

  910 nphs   = 3
      inform = 1
      return

*     ==================================================================
*     Switch to central differences.
*     (Reset ninf to cause m5frmc to set dummy gradients.)
*     ==================================================================
  920 lvldif = 2
      nfail  = 0
      ninf   = 1
      inform = 4
      if (prnt1) write(iprint, 1040)
      return

*     ==================================================================
*     Request a basis factorization.
*     ==================================================================
  925 invrq  = 22
      inform = 5
      return

*     ==================================================================
*     Request a change of basis via m7fixb.
*     ==================================================================
  930 invrq  = 23
      inform = 6
      return

*     ==================================================================
*     m7chzq failed.
*     ==================================================================
  940 nfail  = nfail + 1
      if (nfail .lt. 5) then

*        Treat as an unconstrained step.  Then ask for price.

         jr1    = - jr1
         kmodlu = 0
         kmodpi = 0
         nxtphs = 3
         go to 990
      else

*        Fail.

         call m1page( 2 )
         if (iprint .gt. 0) write(iprint, 1200)
         ierr   = 11
         return
      end if

*     ==================================================================
*     Unbounded.
*     ==================================================================
  950 if (iprint .gt. 0) then
         if (unbddf) write(iprint, 2000) fsub
         if (unbddx) write(iprint, 2100) theta
      end if

  955 if (iprint .gt. 0) write(iprint, 2200) nphs, kb(ms), rg(ns)
      ierr   = 2
      return

*     ==================================================================
*     Too many superbasics.
*     ==================================================================
  960 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1600) maxs
      if (isumm  .gt. 0) write(isumm , 1600) maxs
      ierr   = 5
      return

*     ------------------------------------------------------------------
*     Normal exit.
*     ------------------------------------------------------------------
  990 if (nphs .eq. 4) djq = rgnorm
      if (.not. fullz) djq = rgnrm1
      return

 1040 format(' Switch to central differences.')
 1050 format(' Search exit', i3, ' -- ', a,
     $    '   Itn =', i7, '   Norm rg =', 1p, e11.3)
 1075 format(' Forcing a small step.  alpha * norm(p) =', 1p, e9.1)
 1090 format(' Retry', i3)
 1200 format(' EXIT -- cannot find a superbasic to replace',
     $   ' basic variable')
 1600 format(' EXIT -- the superbasics limit is too small...', i7)
 2000 format(/ ' XXX  Linesearch has exceeded the Unbounded function',
     $   ' value.  fsub =', 1p, e15.5)
 2100 format(/ ' XXX  Linesearch has exceeded the Unbounded step size.',
     $   4x, ' step =', 1p, e13.3)
 2200 format(/ ' XXX  Unbounded in Phase', i3,
     $   '   Last SB =', i7, 1p, e13.3)

*     end of m7rgit
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7sdir( mode, maxr, nr, ns, r, rg, p, w, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   r(nr), rg(ns), p(ns), w(ns), z(nwcore)

*     ------------------------------------------------------------------
*     m7sdir  computes a search direction  p  for the superbasic
*     variables, using the current reduced gradient  rg.
*
*     mode       method
*       0     steepest descent:          p = - rg
*       1     quasi-Newton:       R(t)*R p = - rg
*     ------------------------------------------------------------------

      parameter        ( one = 1.0d+0 )

      if (mode .eq. 0) then

*        Steepest descent  (used when infeasible).

         do 20 j = 1, ns
            p(j) = - rg(j)
   20    continue
      else

*        Quasi-Newton.  We must save  w  satisfying  R(t)*w = rg.

         call dcopy ( ns, rg, 1, p, 1 )
         call m6rsol( 2, maxr, nr, ns, r, p )
         call dcopy ( ns, p, 1, w, 1 )
         call m6rsol( 1, maxr, nr, ns, r, p )
         call dscal ( ns, (- one), p, 1 )
      end if

*     end of m7sdir
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m7sscv( m, maxr, maxs, ms, nr, ns, parhes, nxtphs,
     $                   fx, oldfx, theta, xsnorm,xsnrm1, ysnorm,ysnrm1,
     $                   kb, bbl, bbu, grd, r, rg, x )

      implicit           double precision (a-h,o-z)
      logical            parhes
      integer            kb(ms)
      double precision   bbl(ms), bbu(ms)
      double precision   grd(ms), r(nr), rg(maxs), x(ms)

*     ------------------------------------------------------------------
*     m7sscv  (subspace convergence)  decides whether or not
*     optimization should continue on the current subspace.
*     On exit,  nxtphs = 4  means it should,
*               nxtphs = 3  means it should not.
*
*     parhes  is false if this is a linear iteration or if there
*     is sufficient storage in  r  for the full projected Hessian.
*
*     parhes  is true  if  r  is just a partial Hessian for the
*     first  maxr  superbasic variables.  The superbasics are then
*     in two sets  Z1  (containing   nz1 = maxr       variables)
*             and  Z2  (containing   nz2 = ns - maxr  variables).
*
*     The null-space matrix is similarly partitioned as  Z = ( Z1  Z2 ).
*
*     The normal convergence test is first applied to  Z1.  If it looks
*     like we are near enough to an optimum in that restricted
*     subspace, we find the largest reduced gradient in  Z2  (index k2).
*     If this is less than  tolrg  we exit with  nxtphs = 3  (to ask for
*     price).  Otherwise we make room in  Z1  for the corresponding
*     variable by the moving the superbasic with the smallest reduced
*     gradient in  Z1  (index  k1)  to the end of  Z2.
*     ------------------------------------------------------------------

      logical            conv
      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7phes/ rgmin1,rgnrm1,rgnrm2,jz1,jz2,labz,nfullz,mfullz
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg

      intrinsic          abs
      parameter        ( one = 1.0d+0 )

      rgnrm1 = rgnorm
      if (parhes) then
         xsnorm = xsnrm1
         ysnorm = ysnrm1
         k1     = idamax( maxr, rg, 1 )
         rgnrm1 = abs( rg(k1) )
      end if

      deltax  =  theta * ysnorm
      deltaf  =  abs( fx - oldfx )
      conv(1) =  deltax  .le.  xtol(lvltol) * (one +  xsnorm  )
      conv(2) =  deltaf  .le.  ftol(lvltol) * (one + abs( fx ))
      conv(3) =  rgnrm1  .le.  tolrg
      conv(4) =  rgnrm1  .le.  0.1 * tolrg     .or.
     $           rgnrm1  .le.  gtol(2) * pinorm

      nxtphs  =  4
      if (conv(1).and.conv(2).and.conv(3) .or. conv(4)) nxtphs = 3
      if (.not. parhes ) go to 900
      if (nxtphs .eq. 4) go to 900

*     Swap the largest reduced gradient in  Z2  into the front of  Z2
*     and see if it is significantly large.

      call m6swap( m, maxr, nr, ns, ms, kb, bbl, bbu, grd, r, rg, x )
      k2     = maxr + 1
      rgnrm2 = abs( rg(k2) )
      if (ns .lt. maxs  .and.  rgnrm2 .le. tolrg) go to 900

*     Find the smallest component of  Z1(t)*g.

      rgmin1 = abs( rg(1) )
      k1     = 1
      do 200 k  = 1, maxr
         if (rgmin1 .ge. abs( rg(k) )) then
             rgmin1   =  abs( rg(k) )
             k1       =  k
         end if
  200 continue

      if (rgmin1 .lt. rgnrm2) then

*        Save the relevant values.

         nxtphs = 4
         nfullz = 0
         lastr  = maxr*(maxr + 1)/2
         ldiag1 = k1 * (k1   + 1)/2
         rdiag1 = r(ldiag1)
         rg1    = rg(k1)
         k      = m + k1
         jz1    = kb(k)
         jz2    = kb(m + k2)
         bl1    = bbl(k)
         bu1    = bbu(k)
         grd1   = grd(k)
         x1     = x(k)

*        Delete the k1-th variable from  Z1,  and shift the remaining
*        superbasics in  Z1  and  Z2  one place to the left.

         call m6rdel( m, maxr, nr, ns, ms,
     $                kb, bbl, bbu, grd, r, rg, x, k1, .true. )

*        Put the old k1-th superbasic in at the very end.

         lr      = lastr + (ns - maxr)
         r(lr)   = rdiag1
         rg(ns)  = rg1
         kb(ms)  = jz1
         bbl(ms) = bl1
         bbu(ms) = bu1
         grd(ms) = grd1
         x(ms)   = x1
      end if

  900 return

*     end of m7sscv
      end
