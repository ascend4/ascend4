************************************************************************
*
*     File  mi80ncon fortran.
*
*     m8ajac   m8augl   m8aug1   m8chkj   m8prtj   m8sclj
*     m8setj   m8viol
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8ajac( gotjac, nncon, nnjac, njac,
     $                   ne, nka, a, ha, ka,
     $                   fcon, fcon2, gcon, gcon2, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            gotjac
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne)
      double precision   fcon(nncon), fcon2(nncon),
     $                   gcon(njac ), gcon2(njac ),
     $                   xn  (nnjac), y(nncon), z(nwcore)

*     ------------------------------------------------------------------
*     m8ajac  stores the Jacobian into  A.
*     If  gotjac  is true, the Jacobian is already in  gcon.
*     ------------------------------------------------------------------

      common    /m5log1/ idebug,ierr,lprint
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      if ( .not.  gotjac ) then
         if (lderiv .lt. 2)
     $      call m6dmmy( njac, gcon )

         call m6fcon( 2, nncon, nnjac, njac, fcon, gcon,
     $                ne, nka, ha, ka,
     $                xn, z, nwcore )

         if (lderiv .lt. 2)
     $      call m6dcon( nncon, nnjac, njac,
     $                   ne, nka, ha, ka,
     $                   fcon, fcon2, gcon, gcon2, xn, y, z, nwcore )
         if (ierr .ne. 0) return
      end if

      l     = 0
      do 400 j = 1, nnjac
         k1    = ka(j)
         k2    = ka(j+1) - 1
         do 300 k = k1, k2
            ir    = ha(k)
            if (ir .gt. nncon) go to 400
            l     = l + 1
            a(k)  = gcon(l)
  300    continue
  400 continue

*     end of m8ajac
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8augl( mode, m, n, nb, ns, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, bl, bu, xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb), xn(nb), z(nwcore)

*     ------------------------------------------------------------------
*     m8augl  is a front-end for m8aug1.  It is called by  misolv  and
*     m5solv  at various stages of the augmented Lagrangian algorithm.
*     11 Oct 1991: a, ha, ka etc added as parameters.
*     05 Mar 1992: mode 3 now makes nonlinear rows free for first major.
*     11 Sep 1992: hs added as parameter.
*     ------------------------------------------------------------------

      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold

      ms     = m + ns
      call       m8aug1( mode, ms, nncon, nnjac, njac, n, nb, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, z(lkb), bl, bu, z(lbbl), z(lbbu),
     $                   z(lblslk), z(lbuslk),
     $                   z(lgcon ), z(lgcon2), z(lxlam), xn )

*     end of m8augl
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8aug1( mode, ms, nncon, nnjac, njac, n, nb, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, bl, bu, bbl, bbu, blslk, buslk,
     $                   gcon, gcon2, xlam, xn )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), kb(ms)
      double precision   a(ne), bl(nb), bu(nb), bbl(ms), bbu(ms),
     $                   blslk(nncon), buslk(nncon),
     $                   gcon (njac ), gcon2(njac ),
     $                   xlam (nncon), xn(nb)

*     ------------------------------------------------------------------
*     m8aug1  does the work for  m8augl.
*     11 Sep 1992: hs added as parameter for mode 5, to allow
*                  nonbasic slacks to be moved onto the relaxed bounds.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs

      intrinsic          max, min
      parameter        ( zero = 0.0d+0 )

      inform = 0
      bplus  = 0.9*plinfy
      bminus = - bplus

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        Save the Jacobian from the initial a(*) in gcon, gcon2.
*        ---------------------------------------------------------------
         l      = 0
         do 140 j = 1, nnjac
            k1    = ka(j)
            k2    = ka(j+1) - 1
            do 120 k = k1, k2
               ir       = ha(k)
               if (ir .gt. nncon) go to 140
               l        = l + 1
               gcon (l) = a(k)
               gcon2(l) = a(k)
  120       continue
  140    continue

      else if (mode .eq. 2) then
*        ---------------------------------------------------------------
*        Make sure the initial xn values are feasible.
*        Do the slacks too, in case the RHS has been perturbed.
*        ---------------------------------------------------------------
         do 220 j = 1, nb
            xn(j) = max( xn(j), bl(j) )
            xn(j) = min( xn(j), bu(j) )
  220    continue

      else if (mode .eq. 3) then
*        ---------------------------------------------------------------
*        Start of the first major iteration.
*        1. Initialize  modpen, rhsmod.
*        2. Save the bounds on the nonlinear rows,
*        3. Make the nonlinear rows free (relax the bounds to infinity).
*        ---------------------------------------------------------------
         modpen = 0
         rhsmod = zero
         call dcopy ( nncon, bl(n+1), 1, blslk, 1 )
         call dcopy ( nncon, bu(n+1), 1, buslk, 1 )

         do 320 i = 1, nncon
            bl(n+i) = - plinfy
            bu(n+i) =   plinfy
  320    continue

      else if (mode .eq. 4) then
*        ---------------------------------------------------------------
*        Unbounded subproblem.
*        Increase the penalty parameter.
*        ---------------------------------------------------------------
         modpen = modpen + 1
         if (modpen .gt. 5  .or.  nlag .eq. 0) then
            inform = 1
         else
            t      = nncon
            if (penpar .le. zero) penpar = t / 100.0
            penpar = 10.0 * penpar
            if (iprint .gt. 0) write(iprint, 1400) penpar
            if (isumm  .gt. 0) write(isumm , 1400) penpar
         end if

      else if (mode .eq. 5) then
*        ---------------------------------------------------------------
*        Infeasible subproblem.
*        Relax the bounds on the linearized constraints.
*        Also, move nonbasic slacks onto the new bounds
*        so there won't be large numbers of them floating in between.
*        ---------------------------------------------------------------
         modrhs = modrhs + 1
         if (modrhs .gt. 5) then
            inform = 1
         else
            if (modrhs .eq. 1) rhsmod = tolx
            t      = sinf / nncon
            rhsmod = 10.0 * max( rhsmod, t )
            if (iprint .gt. 0) write(iprint, 1500) rhsmod
            if (isumm  .gt. 0) write(isumm , 1500) rhsmod

            do 520 i = 1, nncon
               j     = n + i
               if (bl(j) .gt. bminus) then
                   bl(j) = blslk(i) - rhsmod
                   if (hs(j) .eq. 0) xn(j) = bl(j)
               end if
               if (bu(j) .lt. bplus ) then
                   bu(j) = buslk(i) + rhsmod
                   if (hs(j) .eq. 1) xn(j) = bu(j)
               end if
  520       continue

            do 540 k  = 1, ms
               j      = kb(k)
               bbl(k) = bl(j)
               bbu(k) = bu(j)
  540       continue
         end if

      else if (mode .eq. 6) then
*        ---------------------------------------------------------------
*        Restore the bounds on the nonlinear constraints.
*        ---------------------------------------------------------------
         do 620 i = 1, nncon
            jslack     = n + i
            bl(jslack) = blslk(i)
            bu(jslack) = buslk(i)
  620    continue
      end if
      return

 1400 format(' Penalty parameter increased to', 1p, e12.2)
 1500 format(' XXX  Infeasible subproblem.  ',
     $   ' RHS perturbation increased to', 1p, e12.2)

*     end of m8aug1
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8chkj( m, n, njac, nx,
     $                   ne, nka, ha, ka,
     $                   bl, bu, f, f2, g, g2,
     $                   x, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   bl(n), bu(n), f(m), f2(m), g(njac), g2(njac),
     $                   x(n), y(n), y2(nx), z(nwcore)

*     ------------------------------------------------------------------
*     m8chkj  verifies the Jacobian  gcon  using
*     finite differences on the constraint functions  f.
*     The various verify levels are described in  m7chkg.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5log1/ idebug,ierr,lprint
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8veri/ jverif(4),lverif(2)

      intrinsic          abs, max, min
      parameter        ( zero = 0.0d+0,  one = 1.0d+0,  ok = 0.1d+0 )

      logical            cheap, first
      character*4        key
      character*4        lbad         , lgood
      data               lbad /'bad?'/, lgood /'ok  '/

      lvl    = lverif(1)
      if (lvl .lt. 0) return
      j1     = max( jverif(3), 1 )
      j2     = min( jverif(4), n )
      cheap  = lvl .le. 1  .or.  j1 .gt. j2
      lssave = lscale
      lscale = 0

*     Evaluate f and g at the base point  x.

      if (lderiv .le. 1) call m6dmmy( njac, g )
      call m6fcon( 2, m, n, njac, f, g,
     $             ne, nka, ha, ka,
     $             x, z, nwcore )
      if (ierr      .ne. 0) go to 900
      if (knowng(2) .eq. 0) go to 900

      if (iprint .gt. 0) then
         if (cheap) then
            write(iprint, 1100)
         else
            write(iprint, 1000)
         end if
      end if

*     --------------------------
*     Cheap test.
*     --------------------------

*     Generate a direction in which to perturb  x.

      yj    = one/n
      do 10 j  =  1, n
         y(j)  =   yj
         y2(j) =   yj
         yj    = - yj * 0.99999
   10 continue

*     Define a difference interval.
*     If needed, alter y to ensure that it will be a feasible direction.
*     If this gives zero, go back to original y and forget feasibility.

      dx     = difint(1) * (one  +  dnorm1( n, x, 1 ))
      call m7chkd( n, bl, bu, x, dx, y, nfeas )
      if (nfeas .eq. 0) call dcopy ( n, y2, 1, y, 1 )

*     ------------------------------------------------------------------
*     We must not perturb x(j) if the j-th column of the Jacobian
*     contains any unknown elements.
*     ------------------------------------------------------------------
      nder   = 0
      l      = 0
      do 30 j = 1, n
         k1   = ka(j)
         k2   = ka(j+1) - 1

         do 20 k = k1, k2
            ir   = ha(k)
            if (ir .gt. m) go to 25
            l    = l + 1
            if (g(l) .eq. gdummy) y(j) = zero
   20    continue

   25    if (y(j) .ne. zero) nder = nder + 1
   30 continue

      if (nder .eq. 0) go to 900

*     Set f2 = constraint values at a short step along  y.

      do 40 j  = 1, n
         y2(j) = x(j) + dx*y(j)
   40 continue

      call m6fcon( 0, m, n, njac, f2, g2,
     $             ne, nka, ha, ka,
     $             y2, z, nwcore )
      if (ierr .ne. 0) go to 900

*     Set   y2  =  (f2 - f)/dx  -  Jacobian*y.  This should be small.
*     At the same time, find the first Jacobian element in column j1.

      do 60 i  = 1, m
         y2(i) = (f2(i) - f(i)) / dx
   60 continue

      l      = 0
      lsave  = 0
      do 100 j = 1, n
         yj    = y(j)
         k1    = ka(j)
         k2    = ka(j+1) - 1
         do 80 k = k1, k2
            ir   = ha(k)
            if (ir .gt. m) go to 100
            l    = l + 1
            if (j .lt. j1) lsave = l
            y2(ir) = y2(ir) - g(l)*yj
   80    continue
  100 continue

      imax   = idamax( m,y2,1 )
      gmax   = (f2(imax) - f(imax)) / dx
      emax   = abs( y2(imax) ) / (one + abs( gmax ))
      if (emax .le. ok) then
         if (iprint .gt. 0) write(iprint, 1400)
      else
         if (iprint .gt. 0) write(iprint, 1500)
         if (isumm  .gt. 0) write(isumm , 1500)
      end if
      if (iprint .gt. 0) write(iprint, 1600) emax, imax
      if (cheap) go to 900

*     ----------------------------------------------------
*     Proceed with the verification of columns j1 thru j2.
*     ----------------------------------------------------
      if (iprint .gt. 0) write(iprint, 2000)
      l      = lsave
      nwrong = 0
      ngood  = 0
      emax   = - one
      jmax   = 0

      do 200 j = j1, j2

*        See if there are any known gradients in this column.

         k1    = ka(j)
         k2    = ka(j+1) - 1
         lsave = l

         do 120 k = k1, k2
            ir    = ha(k)
            if (ir .gt. m) go to 200
            l     = l + 1
            if (g(l) .ne. gdummy) go to 140
  120    continue
         go to 200

*        Found one.

  140    xj     = x(j)
         dx     = difint(1) * (one + abs( xj ))
         if (bl(j) .lt. bu(j)  .and.  xj .ge. bu(j)) dx = -dx
         x(j)   = xj + dx
         call m6fcon( 0, m, n, njac, f2, g2,
     $                ne, nka, ha, ka,
     $                x, z, nwcore )
         if (ierr .ne. 0) go to 900

*        Check nonzeros in the jth column of the Jacobian.
*        Don't bother printing a line if it looks like an exact zero.

         l     = lsave
         first = .true.

         do 160 k = k1, k2
            ir    = ha(k)
            if (ir .gt. m) go to 180
            l     = l + 1
            gi    = g(l)
            if (gi .eq. gdummy) go to 160
            gdiff = (f2(ir) - f(ir))/dx
            err   = abs( gdiff - gi ) / (one + abs( gi ))

            if (emax .lt. err) then
               emax  = err
               imax  = ir
               jmax  = j
            end if

            key   = lgood
            if (err .gt. eps5) key = lbad
            if (key .eq. lbad) nwrong = nwrong + 1
            if (key .eq.lgood) ngood  = ngood  + 1
            if (abs( gi ) + err  .le.  eps0) go to 160
            if (iprint .gt. 0) then
               if (first) then
                  write(iprint, 2100) j,xj,dx,l,ir,gi,gdiff,key
               else
                  write(iprint, 2200)         l,ir,gi,gdiff,key
               end if
            end if
            first = .false.
  160    continue

  180    x(j)  = xj
  200 continue

      if (iprint .gt. 0) then
         if (nwrong .eq. 0) then
            write(iprint, 2500) ngood ,j1,j2
         else
            write(iprint, 2600) nwrong,j1,j2
         end if
         write(iprint, 2700) emax,imax,jmax
      end if

      if (emax .ge. one) then

*        Bad gradients in  funcon.

         ierr   = 8
         call m1envt( 1 )
         if (iprint .gt. 0) write(iprint, 3800)
         if (isumm  .gt. 0) write(isumm , 3800)
      end if

*     Exit.

  900 lscale = lssave
      return

 1000 format(/// ' Verification of constraint gradients',
     $   ' returned by subroutine funcon.')
 1100 format(/ ' Cheap test on funcon...')
 1400 format(  ' The Jacobian seems to be OK.')
 1500 format(  ' XXX  The Jacobian seems to be incorrect.')
 1600 format(  ' The largest discrepancy was', 1p, e12.2,
     $   '  in constraint', i6)
 2000 format(// ' Column       x(j)        dx(j)', 3x,
     $   ' Element no.    Row    Jacobian value    Difference approxn')
 2100 format(/ i7, 1p, e16.8, e10.2, 2i10, 2e18.8, 2x, a4)
 2200 format(           33x, 2i10, 1pe18.8, e18.8, 2x, a4)
 2500 format(/ i7, '  Jacobian elements in cols ', i6, '  thru', i6,
     $         '  seem to be OK.')
 2600 format(/ ' XXX  There seem to be', i6,
     $   '  incorrect Jacobian elements in cols', i6, '  thru', i6)
 2700 format(/ ' XXX  The largest relative error was', 1p, e12.2,
     $   '   in row', i6, ',  column', i6 /)
 3800 format(// ' EXIT -- subroutine funcon appears to be',
     $   ' giving incorrect gradients')

*     end of m8chkj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8prtj( n, nb, nncon, nnjac, lprint, majits, nlag,nscl,
     $                   ne, nka, a, ha, ka, hs,
     $                   ascale, bl, bu, fcon, xlam, xn )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nnjac)
      integer            ka(nka)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb),
     $                   fcon(nncon), xlam(nncon), xn(nb)

*     ------------------------------------------------------------------
*     m8prtj  prints x, lambda, f(x) and/or the Jacobian
*     at the start of each major iteration.
*
*     08 Apr 1992: Internal values of hs(*) allow more values for key.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3scal/ sclobj,scltol,lscale

      intrinsic          min, mod

      logical            prtx, prtl, prtf, prtj, scaled
      character*2        key(-1:4)
      data               key/'FR', 'LO', 'UP', 'SB', 'BS', 'FX'/

      if (iprint .le. 0) return

*     Unscale everything if necessary.

      scaled = lscale .ge. 2
      if (scaled) call m2scla( 2, nncon, n, nb, ne, nka,
     $                         ha, ka, a, ascale, bl, bu, xlam, xn )
      if (scaled) call ddscl ( nncon, ascale(n+1), 1, fcon, 1 )

      nxn    = min( nnjac, 5 )
      nlam   = min( nncon, 5 )
      l      = lprint/10
      prtx   = mod( l,10 ) .gt. 0
      l      = l/10
      prtl   = mod( l,10 ) .gt. 0  .and.  majits .gt. 1
      l      = l/10
      prtf   = mod( l,10 ) .gt. 0
      l      = l/10
      prtj   = mod( l,10 ) .gt. 0

      if ( prtx ) write(iprint, 1100) (xn(j), j=1,nnjac)
      if ( prtl ) write(iprint, 1200) (xlam(i), i=1,nncon)
      if ( prtf ) write(iprint, 1300) (fcon(i), i=1,nncon)
      if ( prtj ) then
         write(iprint, 1400)
         do 100 j = 1, nnjac
            k1    = ka(j)
            k2    = ka(j+1) - 1
            do 50 k = k1, k2
               ir   = ha(k)
               if (ir .gt. nncon) go to 60
   50       continue
            k    = k2 + 1
   60       k2   = k  - 1
            l    = hs(j)
            write(iprint, 1410) j,xn(j),key(l), (ha(k),a(k), k=k1,k2)
  100    continue
      end if
   
*     Rescale if necessary.
         
      if (scaled) call m2scla( 1, nncon, n, nb, ne, nka,
     $                         ha, ka, a, ascale, bl, bu, xlam, xn )
      if (scaled) call dddiv ( nncon, ascale(n+1), 1, fcon, 1 )
      return

 1100 format(/ ' Jacobian variables'
     $       / ' ------------------'   / 1p, (5e16.7))
 1200 format(/ ' Multiplier estimates'
     $       / ' --------------------' / 1p, (5e16.7))
 1300 format(/ ' Constraint functions'
     $       / ' --------------------' / 1p, (5e16.7))
 1400 format(/ ' x  and  Jacobian' / ' ----------------')
 1410 format(i6, 1p, e13.5, 1x, a2, 1x, 4(i9, e13.5)
     $       / (22x, 4(i9, e13.5)))

*     end of m8prtj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8sclj( mode, nncon, nn, ng, n, nb, ne, nka,
     $                   ascale, ha, ka, g )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   ascale(nb), g(ng)

*     ------------------------------------------------------------------
*     If mode = 1,  m8sclj  scales the objective gradient.
*     If mode = 2,  m8sclj  scales the Jacobian.
*     nn, ng, g  are  nnobj, nnobj, gobj  or  nnjac, njac, gcon  resp.
*
*     m8sclj is called by m6fobj and m6fcon only if modefg = 2.
*     Hence, it is used to scale known gradient elements (if any),
*     but is not called when missing gradients are being estimated
*     by m6dobj or m6dcon.
*     ------------------------------------------------------------------

      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      if (knowng(mode) .eq. 0) return

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        Scale known objective gradients.
*        ---------------------------------------------------------------
         do 100 j = 1, nn
            grad  = g(j)
            if (grad .ne. gdummy) g(j) = grad * ascale(j)
  100    continue

      else
*        ---------------------------------------------------------------
*        Scale known Jacobian elements.
*        ---------------------------------------------------------------
         l     = 0

         do 300 j  = 1, nn
            k1     = ka(j)
            k2     = ka(j+1) - 1
            cscale = ascale(j)

            do 250 k = k1, k2
               ir    = ha(k)
               if (ir .gt. nncon) go to 300
               l     = l + 1
               grad  = g(l)
               if (grad .ne. gdummy)
     $         g(l)  = grad * cscale / ascale(n + ir)
  250       continue
  300    continue
      end if

*     end of m8sclj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8setj( lcstat, m, n, nb, nn, nncon, nnjac, njac, nscl,
     $                   lcrash, ns,
     $                   nconvg, nomove, nreduc, optsub, convgd,
     $                   ne, nka, a, ha, ka, hs,
     $                   ascale, bl, bu,
     $                   blslk, buslk, fcon, fcon2,
     $                   fdif , fold , gcon, gcon2, xlam,
     $                   rhs  , xdif , xold, pi, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            optsub, convgd
      integer*4          ha(ne), hs(nb)
      integer            ka(nka)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   blslk(nncon), buslk(nncon),
     $                   fcon (nncon), fcon2(nncon),
     $                   fdif (nncon), fold (nncon),
     $                   gcon (njac ), gcon2(njac ),
     $                   xlam (nncon), rhs  (nncon), xdif(nn), xold(nn),
     $                   pi(m), xn(nb), y(m), z(nwcore)

*     ------------------------------------------------------------------
*     m8setj  sets up the linearly constrained subproblem for the
*     next major iteration of the augmented Lagrangian algorithm.
*
*     lcrash is an input parameter.  If lcrash = 5, the nonlinear
*     constraints have been relaxed so far.  On major itn 2,
*     Crash is called to find a basis including them.
*
*     nconvg  counts no. of times optimality test has been satisfied.
*     nreduc  counts no. of times  penpar  has been reduced.
*     optsub  (input ) says if the previous subproblem was optimal.
*     convgd  (output) says if it is time to stop.
*
*     06 Mar 1992: The first major iteration now relaxes nonlinear rows
*                  and terminates as soon as the linear constraints are
*                  satisfied.  xold is not defined until Major 2.
*     08 Apr 1992: Internal values of hs(*) simplify setting xlam2.
*     22 May 1992: penpar reduced for more majors before setting to zero.
*                  Stops Steinke2 (and maybe OTIS?) from suddenly
*                  getting a huge x and lambda.
*     04 Jun 1992: lcrash added.
*     03 Sep 1992: Penalty parameter is now a multiple of the
*                  default value 100/nncon.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2lu3 / lenl,lenu,ncp,lrow,lcol
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log2/ jq1,jq2,jr1,jr2,lines1,lines2
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs
*-    common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      common    /cycle2/ objtru,suminf,numinf

      intrinsic          abs, max, min

      parameter        ( zero   = 0.0d+0,  one    = 1.0d+0,
     $                   biglam = 1.0d+5,  dmaxlm = 1.0d+8 )

      logical            gotjac, feasbl, major1, major2, phead , zerlam
      double precision   objold
      save               objold

      character*8        line
      character*1        lstate(4), label
      data               line   /'--------'/
      data               lstate /' ','I','U','T'/

      rhsmod = zero
      modrhs = 0
      bplus  = 0.9*plinfy
      bminus = - bplus
      dlmax  = zero
      dlrel  = zero
      dxrel  = zero
      step   = one
      feasbl = ninf .eq. 0
      zerlam = .true.
      major1 = majits .eq. 1
      major2 = majits .eq. 2

*     Output heading for terse log.

      phead  = newhed .or.
     $         majits .gt. 2  .and.  mod( majits, 20 ) .eq. 2
      if (prnt0  .and.  phead) write(iprint, 1100)
      if (              phead) newhed = .false.
      phead  = major1 .or.
     $         majits .gt. 2  .and.  mod( majits, 10 ) .eq. 2
      if (summ0  .and.  phead) write(isumm , 1110)

*     Output heading for detailed log.

      if (prnt1  .and.  .not. major1) call m1page( 0 )
      if (prnt1) write(iprint, 2010) (line, j=1,16), majits, penpar
      if (summ1) write(isumm , 2020) (line, j=1, 8), majits, penpar

*     ------------------------------------------------------------------
*     Beginning of first major iteration.
*     misolv has already called m8augl( 2,... ) to make sure that
*     all nonlinear variables are inside their bounds.
*     ------------------------------------------------------------------
      if (major1) then

*        For interest, get the initial constraint violation.

         call m8viol( n, nb, nncon,
     $                ne, nka, a, ha, ka,
     $                bl, bu, fcon, xn, y, z, nwcore )

*        Save the nonlinear constraint bounds and relax them.
*        (The first major will just satisfy the linear constraints.)
*        Then set internal values for hs.

         call m8augl( 3, m, n, nb, ns, inform,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, xn, z, nwcore )
         call m5hs  ( 'Internal', nb, bl, bu, hs, xn )

*        xlam has been initialized from pi in m4getb.
*        Initialize xold and fold, so they are defined when m5frmc
*        evaluates the augmented Lagrangian at the first feasible point.

         call dcopy ( nn   , xn  , 1, xold, 1 )
         call dcopy ( nncon, fcon, 1, fold, 1 )
         go to 500
      end if

*     ------------------------------------------------------------------
*     Beginning of later major iterations.
*     ------------------------------------------------------------------

*     Restore the bounds on the nonlinear constraints.
      
      call m8augl( 6, m, n, nb, ns, inform,
     $             ne, nka, a, ha, ka,
     $             hs, bl, bu, xn, z, nwcore )
      
*     Make sure all variables are inside their bounds.
*     Then set internal values for hs.
      
      call m8augl( 2, m, n, nb, ns, inform,
     $             ne, nka, a, ha, ka,
     $             hs, bl, bu, xn, z, nwcore )
      call m5hs  ( 'Internal', nb, bl, bu, hs, xn )

      if (major2) then

*        Initialize xold.
*        xlam still contains pi from m4getb.  Check it is OK.
*        fold is set later.

         call dcopy ( nn, xn, 1, xold, 1 )
         step   = one

         do 120 i = 1, nncon
            j     = n + i
            js    = hs(j)
            py    = xlam(i)
            xlam2 = py
            if (js .eq. 0  .and.  py .gt. zero) xlam2 = zero
            if (js .eq. 1  .and.  py .lt. zero) xlam2 = zero
            xlam2 = min( xlam2,    dmaxlm  )
            xlam2 = max( xlam2, (- dmaxlm) )
            xlam(i) = xlam2
  120    continue

         go to 400
      end if

*     ------------------------------------------------------------------
*     Major 3 and later.
*     If xn is feasible, reset the multiplier estimates.
*     Since the previous subproblem could have been stopped early,
*     the pi's for inequality constraints must be checked.
*     If constraint i is active and pi(i) has the wrong sign,
*     we just use zero.
*     22 May 1992: If the subproblem is optimal and a slack is
*                  superbasic, the pi(i) should again be zero.
*     ------------------------------------------------------------------
      if ( feasbl ) then
         do 150 i = 1, nncon
            j     = n + i
            js    = hs(j)
            py    = pi(i)
            xlam2 = py
            if (js .eq. 0  .and.  py .gt. zero) xlam2 = zero
            if (js .eq. 1  .and.  py .lt. zero) xlam2 = zero
            if (js .eq. 2  .and.     optsub   ) xlam2 = zero
            xlam2 = min( xlam2,    dmaxlm  )
            xlam2 = max( xlam2, (- dmaxlm) )
            pi(i) = xlam2
            y(i)  = xlam2 - xlam(i)
  150    continue
      
*        Find the relative change in  xlam.
*        If xlam is very small, it is probably still zero (never set).
*        We treat it specially to avoid taking a small step below.
      
         imax   = idamax( nncon, y   , 1 )
         dlmax  = abs   ( y(imax) )
         dlnorm = dasum ( nncon, y   , 1 )
         xlnorm = dasum ( nncon, xlam, 1 )
         zerlam = xlnorm .le. eps0
         dlrel  = dlnorm / (one + xlnorm)
      end if

*     ------------------------------------------------------------------
*     Find the relative change in the Jacobian variables.
*     Note that xdif and xold allow for all nonlinear variables,
*     in case "step" below turns out not to be one.
*     ------------------------------------------------------------------
      do 220  j  = 1, nn
         xdif(j) = xn(j) - xold(j)
  220 continue

      imax   = idamax( nnjac, xdif, 1 )
      dxmax  = abs   ( xdif(imax) )
      dxrel  = dasum ( nnjac, xdif, 1 ) / (one + dasum ( nnjac,xold,1 ))
      if (prnt1) write(iprint, 2400) dxmax, dxrel, dlmax, dlrel
      if (summ1) write(isumm , 2410) dxmax, dlmax

*     ------------------------------------------------------------------
*     Determine a step to be used as follows:
*        xlam  =  xlam + step*y,    xn  =  xold + step*xdif.
*
*     damp = 2.0 (for example) prevents  x  or  lambda  from changing
*     by more than 200 per cent.  This is an exceedingly crude attempt
*     to avoid trouble when the new subproblem solution  xn  is wildly
*     different from the previous solution  xold.
*
*     Note that the current active set is retained even if  step  ends
*     up being less than 1.0.  Some nonbasic variables may lie strictly
*     between their bounds (which is ok now that xn retains all values).
*     ------------------------------------------------------------------
      damp   = dparm(4)
      step   = one
      if (.not. zerlam) then
        step = min( step, damp/(dlrel + eps) )
      end if
      step   = min( step, damp/(dxrel + eps) )

      if (step .ge. 0.99) then
*        =====================
*        A unit step looks ok.
*        =====================
         step   = one
         if (feasbl) call dcopy ( nncon, pi, 1, xlam, 1 )
         call dcopy ( nn, xn, 1, xold, 1 )
      
*        see if  xn  is the same as  xold.
      
         if (dxrel .le. eps0) then
            nomove = nomove + 1
            if (prnt1) write(iprint, 2100)
            if (summ1) write(isumm , 2100)
            go to 450
         end if
      else
*        ====================
*        Take a shorter step.
*        ====================
         if (feasbl) call daxpy ( nncon, step, y, 1, xlam, 1 )
         call daxpy ( nn, step, xdif, 1, xold, 1 )
         call dcopy ( nn, xold, 1, xn, 1 )
         
         if (prnt1) write(iprint, 2500) step
         if (summ1) write(isumm , 2500) step
      end if

*     ------------------------------------------------------------------
*     Evaluate the nonlinear constraints and the Jacobian
*     and copy the Jacobian into  A  to form the next linearization.
*     ------------------------------------------------------------------
  400 nomove = 0
      gotjac = feasbl  .and.  nlag .eq. 1  .and.  step .eq. one

      call m8ajac( gotjac, nncon, nnjac, njac,
     $             ne, nka, a, ha, ka,
     $             fcon, fcon2, gcon, gcon2, xn, y, z, nwcore )
      if (ierr .ne. 0) go to 900

*     Save the function values in  fold.

      call dcopy ( nncon, fcon, 1, fold, 1 )

*     Find how well the new xn satisfies the nonlinear constraints.

  450 call m8viol( n, nb, nncon,
     $             ne, nka, a, ha, ka,
     $             bl, bu, fcon, xn, y, z, nwcore )

*     ------------------------------------------------------------------
*     Print a line.
*     ------------------------------------------------------------------
  500 label  = lstate(lcstat + 1)
      lu     = lenl + lenu
      if (prnt0) write(iprint, 1200) majits - 1, minits, label, itn,
     $           ninf, objtru, vimax, rgnorm, nssave,
     $           nfcon(1), dxrel, dlrel, step, lu, penpar
C     write(isumm , 1210) majits - 1, minits, label,
      if (summ0) write(isumm , 1210) majits - 1, minits, label,
     $           objtru, vimax, rgnorm, nssave,
     $           nfcon(1), dxrel, dlrel, step

      if (prnt1 .and. (major1 .or. major2)) write(iprint, 1000)
      if (prnt1) write(iprint, 2600) vimax, virel
      if (summ1) write(isumm , 2700) vimax

*     ------------------------------------------------------------------
*     Compute  rhs  =  fcon  -  Jacobian*xn.
*     This is minus the required  rhs  for the new subproblem.
*     ------------------------------------------------------------------
      if (lprint .gt. 1)
     $call m8prtj( n, nb, nncon, nnjac, lprint, majits, nlag, nscl,
     $             ne, nka, a, ha, ka, hs,
     $             ascale, bl, bu, fcon, xlam, xn )

      call dcopy ( nncon, fcon, 1, rhs, 1 )
      call m2aprd( 7, xn, nnjac, rhs, nncon,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )
      call dscal ( nncon, (- one), rhs, 1 )

*     ------------------------------------------------------------------
*     Do Crash on nonlinear rows at start of Major 2
*     unless a basis is known already.
*     ------------------------------------------------------------------
      if (major2  .and.  lcrash .eq. 5) then

*        m8viol has set y to be the exact slacks on nonlinear rows.
*        Copy these into xn(n+i) and make sure they are feasible.
*        m2crsh uses them to decide which slacks to grab for the basis.

         do 550 i = 1, nncon
            j     = n + i
            xn(j) = max(  y(i), bl(j) )
            xn(j) = min( xn(j), bu(j) )
  550    continue

*        Set row types in hrtype.
*        m2crsh uses kb as workspace.  It may alter xn(n+i) for
*        nonlinear slacks.
*        Set internal values for hs again to match xn.

         call m2amat( 2, m, n, nb,
     $                ne, nka, a, ha, ka,
     $                bl, bu, z(lhrtyp) )
         call m2crsh( lcrash, m     , n     , nb    , nn,
     $                ne    , nka   , a     , ha    , ka,
     $                z(lkb), hs    , z(lhrtyp), bl , bu,
     $                xn    , z     , nwcore )
         lcrash = 0
         call m5hs  ( 'Internal', nb, bl, bu, hs, xn )
      end if

*     ------------------------------------------------------------------
*     Test for optimality.
*     For safety, we require the convergence test to be satisfied
*     for  2  subproblems in a row.
*     ------------------------------------------------------------------
      if (major1  .or.  major2) then
         objold = objtru
         convgd = .false.
         nconvg = 0
      else
         dfrel  = abs( objtru - objold ) / (one + objold)
         objold = objtru

         convgd = dxrel .le. 0.1      .and.   dlrel .le. 0.1  .and.
     $            virel .le. rowtol   .and.   optsub          .and.
     $            dfrel .le. 0.001
         nconvg = nconvg + 1
         if (.not. convgd) nconvg = 0
         convgd = nconvg .ge. 2

*        Change from Partial Completion to Full Completion if
*        the iterations seem to be converging.
*        (This may be begging the question!  More theory needed.)

         if (ncom .ne. 1) then      
            if (dxrel .le. 0.1  .and.  dlrel .le. 0.1  .and.
     $          virel .le. 0.1  .and.  optsub        ) then
               ncom = 1
               if (summ1) write(isumm , 2800)
               if (prnt1) write(iprint, 2800)
            end if
         end if
      
*        =============================
*        Adjust the penalty parameter.
*        =============================
         if (feasbl .and. optsub) then
      
            if (virel  .le. radius  .and.  dlrel .le. radius) then
               if (penpar .gt. zero) then
                  nreduc = nreduc + 1
                  penpar = penpar / 10.0

*                 The next line caused trouble on Steinke2
*                 (suddenly a huge search direction).
*                 For safety we should always reduce penpar gradually.
*------>          if (nconvg .ge. 1  .or.  nreduc .ge. 5) penpar = zero

                  if (penpar .le. 1.0d-6  .or.  nreduc .ge. 10) then
                     penpar = zero
                  end if
               end if
      
            else if (majits .ge. 5  .and.  dlrel .ge. 10.0) then
               nreduc = 0
               oldpen = penpar
               if (penpar .le. zero) penpar = 1.0d+0
*-04 Sep 1992: Raising the penalty parameter (below) seems to cause
*-             more trouble than it fixes.  Give it up for now.
*-             fac    = 1.5
*-             if (dlrel .ge. biglam) fac = 2.0
*-             penpar = penpar * fac
*-             penmax = 1.0d+3
*-             penpar = min( penpar, penmax )
               if (iprint .gt. 0  .and.  penpar .gt. oldpen) then
                  write(iprint, 2900) penpar
               end if
            end if
         end if
      end if

*     ------------------------------------------------------------------
*     Exit.
*     ------------------------------------------------------------------
  900 return

 1000 format(' ')
 1100 format(/ ' Major minor   total ninf  true objective    viol',
     $         '    rg    nsb   ncon xchange lchange    step',
     $         '     LU penalty')
 1110 format(/ ' Major minor   true objective    viol',
     $         '    rg    nsb   ncon xchange lchange    step')
C1210 FORMAT(2I6, A1,     I5, 1PE16.8, 2E8.1, I5,  I6, 2E8.1)
 1210 format(1p, 2i6, a1,         e16.8, 2e8.1, i5, i7, 3e8.1)

 1200 format(1p, 2i6, a1, i7, i5, e16.8, 2e8.1, i5, i7, 3e8.1, i7, e8.1)
 2010 format(  1x, 16a8 / ' Start of major itn', i4,
     $   10x, ' Penalty parameter =', 1p, e12.2)
 2020 format(/ 1x,  8a8 / ' Start of major itn', i4,
     $   10x, ' Penalty parameter =', 1p, e12.2)
 2100 format(/ ' Jacobian variables have not changed --',
     $         ' old linearization kept')
 2400 format(/ ' Maximum change in Jacobian vars =', 1p, e12.4,
     $     4x,  ' ( =', e11.4, ' normalized)'
     $       / ' Maximum change in multipliers   =',   e12.4,
     $     4x,  ' ( =', e11.4, ' normalized)' )
 2410 format(' Change in Jacobn vars =', 1p, e12.4
     $     / ' Change in multipliers =', e12.4)
 2500 format(  ' Step size for  x  and  lamda  =', f12.5)
 2600 format(  ' Maximum constraint violation    =', 1p, e12.4,
     $     4x,  ' ( =', e11.4, ' normalized)' )
 2700 format(' Constraint violation  =', 1p, e12.4)
 2800 format(/ ' Completion  Full     requested as from now')
 2900 format(/ ' Penalty parameter increased to', 1p, e12.2 /)

*     end of m8setj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m8viol( n, nb, nncon,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, fcon, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb), fcon(nncon),
     $                   xn(nb), y(nncon), z(nwcore)

*     ------------------------------------------------------------------
*     m8viol  finds how much xn violates the nonlinear constraints.
*     y(*)    is output as the vector of violations.
*     maxvi   points to the biggest violation.
*     vimax   is the biggest violation.
*     virel   is the biggest violation normalized by xnorm.
*     ------------------------------------------------------------------

      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave

      intrinsic          max
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     Set  y  =  - fcon - (linear A)*xn,   excluding slacks.

      do 520 i = 1, nncon
         y(i)  = - fcon(i)
  520 continue
      call m2aprd( 8, xn, n, y, nncon,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )

*     See how much  y  violates the bounds on the nonlinear slacks.

      vimax  = zero
      maxvi  = 1

      do 550 i = 1, nncon
         j     = n + i
         slack = y(i)
         viol  = max( zero, bl(j) - slack, slack - bu(j) )
         if (vimax .lt. viol) then
             vimax   =  viol
             maxvi   =  i
         end if
  550 continue

      xnorm  = dnorm1( nb, xn, 1 )
      virel  = vimax / (one + xnorm)

*     end of m8viol
      end
