****************************************************************
*
*     file  mi60srch fortran.
*
*     m6dmmy   m6fcon   m6fobj   m6fun    m6fun1   m6grd    m6grd1
*     m6dobj   m6dcon   m6srch   srchc    srchq
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6dmmy( n, g )

      implicit           double precision (a-h,o-z)
      double precision   g(n)

*     ------------------------------------------------------------------
*     m6dmmy  sets the elements of  g  to a dummy value.
*     When  funcon  or  funobj  are subsequently called, we can
*     then determine which elements of  g  are known.
*     ------------------------------------------------------------------

      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      call dload ( n, gdummy, g, 1 )

*     end of m6dmmy
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6fcon( modefg, nncon, nnjac, njac, f, g,
     $                   ne, nka, ha, ka,
     $                   x, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   f(nncon), g(njac), x(nnjac), z(nwcore)

*     ------------------------------------------------------------------
*     m6fcon  calls the user-written routine  funcon  to evaluate
*     the nonlinear constraints and possibly their gradients.
*
*     08-Jun-1987: Rewritten for GAMS during Tony Brooke's visit.
*     20-Mar-1988: Tony found it hadn't been installed (gasp!).
*     12 Sep 1991: More if-then-elses.
*     19 Nov 1991: ha, ka passed in as parameters to help with minoss.
*     11 Jul 1992: Clock 4 added.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime
      common    /m3len / m     ,n     ,nb    ,nscl
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m5log1/ idebug,ierr,lprint
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2

      logical            scaled

      if (ltime  .ge. 2) call m1time( 4,0 )
      scaled   = lscale .eq. 2
      mode     = modefg
      nfcon(1) = nfcon(1) + 1
      if (mode   .eq. 2) nfcon(2) = nfcon(2) + 1

*     ------------------------------------------------------------------
*     On first entry, count how many Jacobian elements are known.
*     ------------------------------------------------------------------
      if (nstat2 .eq. 1) then
         call m6dmmy( njac, g )
         call funcon( mode, nncon, nnjac, njac, x, f, g,
     $                nstat2, nprob, z, nwcore )
         nstat2    = 0
         if (mode .lt. 0) go to 900

         ngrad     = 0
         do 100 l  = 1, njac
            if (g(l) .ne. gdummy) ngrad  = ngrad + 1
  100    continue

         knowng(2) = ngrad
         if (iprint .gt. 0) then
            write(iprint, 1100)
            write(iprint, 2000) ngrad, njac
         end if
         if (isumm  .gt. 0) then
            write(isumm , 2000) ngrad, njac
         end if
         if (ngrad .eq. njac) go to 990

*        Some Jacobian elements are missing.
*        If the Jacobian is supposedly known (lderiv .ge. 2), the
*        missing elements are assumed to be constant, and must be
*        restored.  They have been saved from the initial a(*) by
*        m8augl( 1, ... ) and saved in gcon2.

         if (lderiv .ge. 2) call dcopy ( njac, z(lgcon2), 1, g, 1 )
         nfcon(2) = nfcon(2) + 1
         go to 400
      end if

*     Test for last entry.
*     Scaling will have been disabled, so it's ok to
*     skip the subsequent part.

      if (nstat2 .ge. 2) then
         if (iprint .gt. 0) write(iprint, 3000) nstat2
         if (isumm  .gt. 0) write(isumm , 3000) nstat2
         go to 400
      end if

*     ------------------------------------------------------------------
*     Normal entry.
*     If the Jacobian is known and scaled, and if there are some
*     constant elements saved in gcon2, we have to copy them into g.
*     ------------------------------------------------------------------

      if (scaled) then
         call dcopy ( nnjac, x        , 1, z(lx2), 1 )
         call ddscl ( nnjac, z(lascal), 1, x,      1 )
         if (lderiv .ge. 2  .and.  knowng(2) .lt. njac
     $                      .and.  modefg    .eq. 2   ) then
            call dcopy ( njac, z(lgcon2), 1, g, 1 )
         end if
      end if

 400  continue
      call funcon( mode, nncon, nnjac, njac, x, f, g,
     $             nstat2, nprob, z, nwcore )

      if (scaled) then
         call dcopy ( nnjac, z(lx2)     , 1, x, 1 )
         call dddiv ( nncon, z(lascal+n), 1, f, 1 )
         if (modefg .eq. 2) then
            call m8sclj( 2, nncon, nnjac, njac, n, nb, ne, nka,
     $                   z(lascal), ha, ka, g )
         end if
      end if

      if (mode .lt. 0) go to 900
      go to 990

*     -----------------------
*     The user wants to stop.
*     -----------------------
  900 ierr   = 6
      call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1000)
      if (isumm  .gt. 0) write(isumm , 1000)

  990 if (ltime  .ge. 2) call m1time(-4,0 )
      return

 1000 format(' EXIT -- Termination requested by User',
     $   ' in subroutine funcon')
 1100 format(//)
 2000 format(' funcon  sets', i8, '   out of', i8,
     $   '   constraint gradients.')
 3000 format(/ ' funcon called with nstate =', i4)

*     end of m6fcon
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6fobj( modefg, n, f, g, x, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   g(n), x(n), z(nwcore)

*     ------------------------------------------------------------------
*     m6fobj  calls the user-written routine  funobj  to evaluate
*     the nonlinear objective function and possibly its gradient.
*     12 Sep 1991: More if-then-elses.
*     11 Jul 1992: Clock 5 added.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m5log1/ idebug,ierr,lprint
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2

      logical            scaled

      if (ltime  .ge. 2) call m1time( 5,0 )
      scaled   = lscale .eq. 2
      mode     = modefg
      nfobj(1) = nfobj(1) + 1
      if (mode   .eq. 2) nfobj(2) = nfobj(2) + 1

*     Test for first or last entry.

      if (nstat1 .eq. 1) then
         call m6dmmy( n, g )
      else if (nstat1 .ge. 2) then
         if (iprint .gt. 0) write(iprint, 3000) nstat1
         if (isumm  .gt. 0) write(isumm , 3000) nstat1
      end if

*     Normal entry.

      if (scaled) then
         call dcopy ( n, x        , 1, z(lx2), 1 )
         call ddscl ( n, z(lascal), 1, x,      1 )
      end if

      call funobj( mode, n, x, f, g, nstat1, nprob, z, nwcore )

      if (scaled) then
         call dcopy ( n, z(lx2), 1, x, 1 )
         if (modefg .eq. 2) then
            call m8sclj( 1, 1, n, n, 1, n, 1, 1, z(lascal), z, z, g )
         end if
      end if

      if (mode .lt. 0) go to 900

*     ------------------------------------------------------------------
*     On first entry, count components to be estimated by differences.
*     ------------------------------------------------------------------
      if (nstat1 .eq. 1) then
         nstat1 = 0
         ngrad  = 0
         do 100 l = 1, n
            if (g(l) .ne. gdummy) ngrad = ngrad + 1
  100    continue

         knowng(1) = ngrad
         if (iprint .gt. 0) then
            write(iprint, 1100)
            write(iprint, 2000) ngrad, n
         end if
         if (isumm  .gt. 0) then
            write(isumm , 2000) ngrad, n
         end if

         if (ngrad .lt. n) then
            if (lderiv .eq. 1  .or.  lderiv .eq. 3) then
               lderiv = lderiv - 1
               if (iprint .gt. 0) write(iprint, 2200) lderiv
               if (isumm  .gt. 0) write(isumm , 2200) lderiv
            end if
         end if
      end if

      go to 990

*     -----------------------
*     The user wants to stop.
*     -----------------------
  900 ierr   = 6
      call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1000) nfobj(1)
      if (isumm  .gt. 0) write(isumm , 1000) nfobj(1)

  990 if (ltime  .ge. 2) call m1time(-5,0 )
      return

 1000 format(' EXIT -- Termination requested by User',
     $   ' in subroutine funobj after', i8, '  calls')
 1100 format(//)
 2000 format(' funobj  sets', i8, '   out of', i8,
     $   '   objective  gradients.')
 2200 format(/ ' Derivative Level  now reduced to', i3)
 3000 format(/ ' funobj called with nstate =', i4)

*     end of m6fobj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6fun ( mode, modefg, n, nb, ms, fsub,
     $                   ne, nka, a, ha, ka,
     $                   x, xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), x(ms), xn(nb), z(nwcore)

*     ------------------------------------------------------------------
*     m6fun  is called mainly by the linesearch, m6srch,
*     to compute the subproblem function value fsub.
*     It is also called by m5frmc when a feasible point is first found,
*     and by m6rgit when the linesearch says the max step is too small.
*
*     If mode = 0, m6fun  sets objective and constraint gradients to
*     dummy values (if necessary) in preparation for calls with mode=1.
*
*     If mode = 1, m6fun  returns the subproblem function value in fsub
*     for the current value of the basic and superbasic vars in  x.
*     The user functions funcon and/or funobj are called
*     via m6fcon and m6fobj, using modefg to control the gradients
*     as follows:
*
*        If modefg = 2, gradients will be requested.
*        If modefg = 0, gradients will NOT be requested.  This is used
*        by m6srch during a function-only linesearch.  An extra call
*        with modefg = 2 is needed once the linesearch finds a good
*        point, but this may be cheaper than getting gradients every
*        time, e.g. if the user estimates gradients by differencing.
*     
*     12 Sep 1991: More if-then-elses.
*     02 Oct 1991: modefg added following request from Hilary Hoynes,
*                  Hoover Institution (Hoynes@Hoover.bitnet, 723-9511).
*     14 Oct 1991: a, ha, ka added as parameters.
*     ------------------------------------------------------------------

      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      logical            grdcon, grdobj

      if (mode .eq. 0) then
*        ---------------------------------------------------------------
*        mode = 0.  See if any gradients will have to be estimated.
*        This call is from m5frmc after a refactorize,
*        or from m6srch at the beginning of a linesearch.
*        ---------------------------------------------------------------
         grdcon = nncon .eq. 0  .or.  lderiv .ge. 2  .or.  nlag   .eq. 0
         grdobj = nnobj .eq. 0  .or.  lderiv .eq. 1  .or.  lderiv .eq. 3
         if (.not. grdcon) call m6dmmy( njac , z(lgcon) )
         if (.not. grdobj) call m6dmmy( nnobj, z(lgobj) )
      else
*        ---------------------------------------------------------------
*        mode = 1.  Evaluate the subproblem objective.
*        Copy the free variables  x  into  xn  first.
*        ---------------------------------------------------------------
         call m5bsx ( 1, ms, nb, z(lkb), x, xn )

         call m6fun1( modefg, ms, n, nn,
     $                ne, nka, a, ha, ka,
     $                nncon, nncon0, nnobj, nnobj0, nnjac, njac,
     $                z(lfcon), z(lgcon), fobj, z(lgobj), fsub,
     $                z(lfdif), z(lfold), z(lxlam),
     $                x, xn, z(lxdif), z(lxold), z, nwcore )
      end if

*     end of m6fun
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6fun1( modefg, ms, n, nn,
     $                   ne, nka, a, ha, ka,
     $                   nncon, nncon0, nnobj, nnobj0, nnjac, njac,
     $                   fcon, gcon, fobj, gobj, fsub,
     $                   fdif, fold, xlam,
     $                   x, xn, xdif, xold, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne),
     $                   fcon(nncon0), gcon(njac  ), gobj(nnobj0),
     $                   fdif(nncon0), fold(nncon0), xlam(nncon0),
     $                   x(ms), xn(nn), xdif(nn), xold(nn), z(nwcore)

*     ------------------------------------------------------------------
*     m6fun1  calls the constraint and objective functions, and then
*     computes the subproblem objective function from them.
*
*     03 Sep 1992: Penalty parameter (penpar) is now used as a
*                  multiple of a default value, pendef = 100 / nncon.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor

      logical            lagrng
      parameter        ( zero = 0.0d+0,  dhalf = 0.5d+0 )

*     Evaluate the functions  (constraints first, then objective).
*     If Lagrangian = No, we only evaluate the true objective.

      lagrng = nnjac .gt. 0  .and.  nlag .gt. 0

      if (lagrng) then
         call m6fcon( modefg, nncon, nnjac, njac, fcon, gcon,
     $                ne, nka, ha, ka,
     $                xn, z, nwcore )
         if (ierr  .ne. 0) return
      end if

      fobj   = zero
      if (nnobj .gt. 0) then
         call m6fobj( modefg, nnobj, fobj, gobj, xn, z, nwcore )
         if (ierr  .ne. 0) return
      end if

*     Include the linear objective.

      fsub   = fobj
      if (iobj   .ne. 0) fsub =   fsub  -  x(iobj) * sclobj
      if (minimz .lt. 0) fsub = - fsub

*     Compute the augmented Lagrangian terms:
*        fdif  =  (fcon - fold)  -  (Jacobian)*(xn - xold).

      if (lagrng) then
         do 110  i  = 1, nncon
            fdif(i) = fcon(i) - fold(i)
  110    continue

         do 120  j  = 1, nnjac
            xdif(j) = xn(j) - xold(j)
  120    continue

         call m2aprd( 7, xdif, nnjac, fdif, nncon,
     $                ne, nka, a, ha, ka,
     $                z, nwcore )

         fsub   = fsub  -  ddot( nncon, xlam, 1, fdif, 1 )
         if (penpar .gt. zero) then
            pendef = 100.0d+0 / nncon
            rho    = penpar * pendef
            fdnorm = dnrm2( nncon, fdif, 1 )
            fsub   = fsub  +  dhalf * rho * fdnorm**2
         end if
      end if

  900 return

*     end of m6fun1
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6grd ( ms, nb, nn, gsub, grd,
     $                   ne, nka, a, ha, ka,
     $                   xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), gsub(nn), grd(ms), xn(nb), z(nwcore)

*     ------------------------------------------------------------------
*     m6grd   returns the gradient of the subproblem objective in  gsub.
*
*     14 Oct 1991: a, ha, ka added as parameters.
*     ------------------------------------------------------------------

      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold

      call m6grd1( n, nn, fobj,
     $             nncon, nncon0, nnobj, nnobj0, nnjac, njac,
     $             ne, nka, a, ha, ka,
     $             z(lfcon), z(lfcon2), z(lfdif ), z(lxlam),
     $             z(lgcon), z(lgcon2), z(lgobj ), z(lgobj2), gsub,
     $             xn, z(ly), z, nwcore )

      call m7bsg ( ms, nn, z(lkb), gsub, grd )

*     end of m6grd
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6grd1( n, nn, fobj,
     $                   nncon, nncon0, nnobj, nnobj0, nnjac, njac,
     $                   ne, nka, a, ha, ka,
     $                   fcon, fcon2, fdif, xlam,
     $                   gcon, gcon2, gobj, gobj2, gsub,
     $                   xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), fcon(nncon0), fcon2(nncon0),
     $                   fdif(nncon0), xlam (nncon0),
     $                   gcon(njac  ), gcon2(njac  ),
     $                   gobj(nnobj0), gobj2(nnobj0), gsub(nn),
     $                   xn(nn), y(nncon0), z(nwcore)

*     ------------------------------------------------------------------
*     m6grd1 computes gsub(j), the gradient of the subproblem objective.
*
*     NOTE --  m6dcon overwrites the first  nncon  elements of  y
*     if central differences are needed.
*
*     03 Sep 1992: Penalty parameter (penpar) is now used as a
*                  multiple of a default value, pendef = 100 / nncon.
*     ------------------------------------------------------------------

      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      logical            grdcon, grdobj, lagrng
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      grdcon = nncon  .eq. 0  .or.  lderiv .ge. 2  .or.  nlag   .eq. 0
      grdobj = nnobj  .eq. 0  .or.  lderiv .eq. 1  .or.  lderiv .eq. 3
      lagrng = nnjac  .gt. 0 .and.  nlag   .gt. 0

*     ------------------------------------------------------
*     Fill in any missing constraint or objective gradients.
*     ------------------------------------------------------

      if (.not. grdcon) then
         call m6dcon( nncon, nnjac, njac,
     $                ne, nka, ha, ka,
     $                fcon, fcon2, gcon, gcon2, xn, y, z, nwcore )
         if (ierr .ne. 0) return
      end if

      if (.not. grdobj) then
         call m6dobj( nnobj, fobj, gobj, gobj2, xn, z, nwcore )
         if (ierr .ne. 0) return
      end if

*     -----------------------------------------
*     Compute the required elements of  gsub.
*     -----------------------------------------

*     Set  gsub = ( gobj, 0 )  and allow for the sign of the objective.

      l      = nn - nnobj
      if (nnobj  .gt. 0) call dcopy ( nnobj , gobj, 1, gsub, 1 )
      if (l      .gt. 0) call dload ( l     , zero, gsub(nnobj+1), 1 )
      if (minimz .lt. 0) call dscal ( nnobj0, (- one), gsub, 1 )

*     Compute the augmented Lagrangian terms.
*     m6fun1  has set up   fdif  =  (fcon - fold)  -  Jac * (x - xold).
*     To save work, set    fdif  =  rho * fdif  -  xlam.

      if (lagrng) then
         pendef = 100.0d+0 / nncon
         rho    = penpar * pendef
         call dscal ( nncon,  rho, fdif, 1 )
         call daxpy ( nncon, (- one), xlam, 1, fdif, 1 )

         l      = 0
         do 600 j = 1, nnjac
            g     = gsub(j)
            k1    = ka(j)
            k2    = ka(j+1) - 1

            do 500 k = k1, k2
               ir    = ha(k)
               if (ir .gt. nncon) go to 590
               l     = l  +  1
               g     = g  +  (gcon(l) - a(k)) * fdif(ir)
  500       continue

  590       gsub(j) = g
  600    continue
      end if

*     end of m6grd1
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6dobj( nnobj, fobj, gobj, gobj2, xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   gobj(nnobj), gobj2(nnobj)
      double precision   xn(nnobj), z(nwcore)

*     ------------------------------------------------------------------
*     m6dobj  estimates missing objective gradients
*     using finite differences of the objective function  fobj.
*     ------------------------------------------------------------------

      common    /m5log1/ idebug,ierr,lprint
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2

      intrinsic          abs

      logical            centrl
      parameter        ( one = 1.0d+0 )

      centrl = lvldif .eq. 2
      delta  = difint(lvldif)
      numf   = 0
      fback  = fobj

      do 200 j  = 1, nnobj
         if (gobj(j) .eq. gdummy) then
            xj     = xn(j)
            dx     = delta*(one + abs( xj ))
            xn(j)  = xj + dx
            numf   = numf + 1
            call m6fobj( 0, nnobj, fforwd, gobj2, xn, z, nwcore )
            if (ierr .ne. 0) go to 900

            if (centrl) then
               xn(j)  = xj - dx
               dx     = dx + dx
               numf   = numf + 1
               call m6fobj( 0, nnobj, fback, gobj2, xn, z, nwcore )
               if (ierr .ne. 0) go to 900
            end if

            gobj(j) = (fforwd - fback) / dx
            xn(j)   = xj
         end if
  200 continue

  900 l        = lvldif + 2
      nfobj(l) = nfobj(l) + numf

*     end of m6dobj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6dcon( nncon, nnjac, njac,
     $                   ne, nka, ha, ka,
     $                   fcon, fcon2, gcon, gcon2, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   fcon(nncon), fcon2(nncon),
     $                   gcon(njac ), gcon2(njac ),
     $                   xn(nnjac), y(nncon), z(nwcore)

*     ------------------------------------------------------------------
*     m6dcon  estimates missing elements in the columns of the
*     Jacobian (the first  nncon  rows and  nnjac  cols of  A)
*     using finite differences of the constraint functions  fcon.
*     ------------------------------------------------------------------

      common    /m5log1/ idebug,ierr,lprint
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2

      intrinsic          abs

      logical            centrl
      parameter        ( one = 1.0d+0 )

      centrl = lvldif .eq. 2
      delta  = difint(lvldif)
      numf   = 0
      l      = 0

*     If central differences are needed, we first save  fcon  in  y.
*     When  fcon  is later loaded with  f(x - h)  for each pertbn  h,
*     it can be used the same as when forward differences are in effect.

      if (centrl) call dcopy ( nncon, fcon, 1, y, 1 )

      do 200 j  = 1, nnjac

*        Look for the first missing element in this column.

         lsave  = l
         k1     = ka(j)
         k2     = ka(j+1) - 1
         do 100 k = k1, k2
            ir    = ha(k)
            if (ir .gt. nncon) go to 200
            l     = l + 1
            if (gcon(l) .eq. gdummy) go to 120
  100    continue
         go to 200

*        Found one. Approximate it and any others by finite differences.

  120    l      = lsave
         xj     = xn(j)
         dx     = delta * (one + abs( xj ))
         xn(j)  = xj + dx
         numf   = numf + 1
         call m6fcon( 0, nncon, nnjac, njac, fcon2, gcon2,
     $                ne, nka, ha, ka,
     $                xn, z, nwcore )
         if (ierr .ne. 0) go to 900

         if (centrl) then
            xn(j)  = xj - dx
            dx     = dx + dx
            numf   = numf + 1
            call m6fcon( 0, nncon, nnjac, njac, fcon, gcon2, 
     $                   ne, nka, ha, ka,
     $                   xn, z, nwcore )
            if (ierr .ne. 0) go to 900
         end if

         do 150 k = k1, k2
            ir    = ha(k)
            if (ir .gt. nncon) go to 190
            l     = l + 1
            if (gcon(l) .eq. gdummy) then
                gcon(l) = (fcon2(ir) - fcon(ir)) / dx
            end if
  150    continue
  190    xn(j)  = xj
  200 continue

  900 if (centrl) call dcopy ( nncon, y, 1, fcon, 1 )
      l        = lvldif + 2
      nfcon(l) = nfcon(l) + numf

*     end of m6dcon
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6srch( ms, ns, n, nb, nn, itn, inform, 
     $                   debug, fonly, switch,
     $                   ne, nka, a, ha, ka,
     $                   alfa, alfmax, difint, epsmch, epsrf, eta,
     $                   fsub, gnorm , pnorm , xnorm,
     $                   gsub, grd, p, x, x2, xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            debug, fonly, switch
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), gsub(nn), grd(ms),
     $                   p(ms), x(ms), x2(ms), xn(nb), z(nwcore)

*     ------------------------------------------------------------------
*     m6srch  finds a step-length  alfa  along the search direction  p,
*     such that the (subproblem) objective function  F  is sufficiently
*     reduced:    F(x + alpha*p)  <  F(x).
*
*     Input parameters...
*
*     alfa        Initial estimate of the step.
*     alfmax      Maximum allowable step.
*     difint      A difference interval that has been used for forward
*                 differences (not central differences) to get p.
*                 (difint is used if switch is true).
*     epsmch      Relative machine precision.
*     epsrf       Relative function precision.
*     eta         Linesearch accuracy parameter in the range (0,1).
*                 0.001 means very accurate.   0.99 means quite sloppy.
*     fonly       true if function-only search should be used.
*     switch      true if there is a possibility of switching to
*                 central differences to get a better p.
*     fsub        Current value of subproblem objective, F(x).
*     gsub(*)     Current gradient of subproblem objective.
*     grd(*)      gsub  reordered like x and p  (free variables only).
*     p(*)        Search direction for the free variables.
*     x(*)        Current free variables (basic and superbasic).
*
*     Output parameters...
*
*     alfa
*     fsub        New value of the subproblem objective, F(x + alfa*p).
*     gsub(*)     New gradient of the subproblem objective.
*     grd(*)      New  gsub  reordered parallel to  x  and  p.
*     x           New free variables (basic and superbasic).
*     inform = -1 if the user wants to stop.
*            =  1 to 8 -- see below.
*
*     02 Oct 1991: fonly search uses modefg = 0 during the search,
*                  and modefg = 2 after the search.
*     04 Oct 1991: switch added to suppress request for central diffs
*                  when fonly = true but all gradients are really known.
*     16 Oct 1991: getptc, getptq replaced by srchc, srchq.
*                  inform values altered.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m5log1/ idebug,ierr,lprint

      intrinsic          abs   , max  
      logical            done  , first , imprvd
      parameter        ( zero = 0.0d+0,  one = 1.0d+0,  two = 2.0d+0 )
      parameter        ( rmu  = 1.0d-4  )

*     ------------------------------------------------------------------
*     Set the input parameters for  srchc  or  srchq.
*
*     alfsml  is used by srchq.  If  alfa  would be less than  alfsml,
*             the search will be terminated early.
*             If  p  was computed using forward or backward differences,
*             alfsml  should be positive (and related to the difference
*             interval used).
*             If  p  was computed using central differences (lvldif = 2)
*             alfsml  should be zero.
*
*     epsrf   is the relative function precision (specified by user).
*
*     epsaf   is the absolute function precision. If F(x1) and F(x2) are
*             as close as  epsaf,  we cannot safely conclude from the
*             function values alone which of x1 or x2 is a better point.
*
*     tolabs  is an estimate of the absolute spacing between points
*             along  p.  This step should produce a perturbation
*             of  epsaf  in the subproblem objective function.
*
*     tolrel  is an estimate of the relative spacing between points
*             along  p.
*
*     toltny  is the minimum allowable absolute spacing between points
*             along  p.
*     ------------------------------------------------------------------
      first  = .true.
      maxf   = 10
      if (fonly) maxf = 15
      nout   = iprint

      alfsml = zero
      if (fonly  .and.  switch) then
         alfsml = difint * (one + xnorm) / pnorm
      end if

      epsaf  = max( epsrf, epsmch ) * (one + abs( fsub ))
      oldf   = fsub
      oldg   = ddot ( ms, grd, 1, p, 1 )
      eps0   = epsmch**0.8
      tolax  = eps0
      tolrx  = eps0
      tolrel = max( tolrx, epsmch )
      t      = xnorm * tolrx  +  tolax
      tolabs = alfmax
      if (t .lt. pnorm * alfmax) tolabs = t / pnorm

      t      = zero
      do 10 j = 1, ms
         s    = abs( p(j) )
         q    = abs( x(j) )*tolrx + tolax
         if (s .gt. q*t) t = s / q
   10 continue

      if (t*tolabs .gt. one) then
         toltny = one / t
      else
         toltny = tolabs
      end if

      alfbst = zero
      fbest  = zero
      gbest  = (one - rmu)*oldg
      targtg = (rmu - eta)*oldg
      g0     = gbest

      if (debug) write(nout, 1000) itn, pnorm, gnorm

*     ------------------------------------------------------------------
*     Commence main loop, entering srchc or srchq two or more times.
*     first = true for the first entry,  false for subsequent entries.
*     done  = true indicates termination, in which case the value of
*     inform gives the result of the search.
*     inform = 1 if the search is successful and alfa < alfmax.
*            = 2 if the search is successful and alfa = alfmax.
*            = 3 if a better point was found but too many functions
*                were needed (not sufficient decrease).
*            = 4 if alfmax < tolabs (too small to do a search).
*            = 5 if alfa < alfsml (srchq only -- maybe want to switch
*                to central differences to get a better direction).
*            = 6 if the search found that there is no useful step.
*                The interval of uncertainty is less than 2*tolabs.
*                The minimizer is very close to alfa = zero
*                or the gradients are not sufficiently accurate.
*            = 7 if there were too many function calls.
*            = 8 if the input parameters were bad
*                (alfmax le toltny  or  oldg ge 0).
*     ------------------------------------------------------------------
*     Start loop
  100    if (fonly) then
            call srchq ( first , debug , done  , imprvd, inform,
     $                   maxf  , numf  , nout  , 
     $                   alfmax, alfsml, epsaf , 
     $                   g0    , targtg, ftry  ,         
     $                   tolabs, tolrel, toltny,
     $                   alfa  , alfbst, fbest  )
         else
            call srchc ( first , debug , done  , imprvd, inform,
     $                   maxf  , numf  , nout  ,
     $                   alfmax,         epsaf , 
     $                   g0    , targtg, ftry  , gtry  , 
     $                   tolabs, tolrel, toltny,
     $                   alfa  , alfbst, fbest , gbest )
         end if

         if (.not. done ) then
*           ------------------------------------------------------------
*           Compute the function value fsub at the new trial point
*           x + alfa*p.
*           ------------------------------------------------------------
            do 180 j = 1, ms
               x2(j) = x(j) + alfa*p(j)
  180       continue

*           Check for first function request.
*           Maybe set dummy gradients.

            if (numf .eq. 0) then
               modefg = 2
               if ( fonly ) then
                  modefg = 0
                  call m6fun ( 0, modefg, n, nb, ms, fsub,
     $                         ne, nka, a, ha, ka,
     $                         x2, xn, z, nwcore )
               end if
            end if

            call m6fun ( 1, modefg, n, nb, ms, fsub,
     $                   ne, nka, a, ha, ka,
     $                   x2, xn, z, nwcore )
            if (ierr .ne. 0) go to 999
         
            ftry  = fsub - oldf - rmu*oldg*alfa 

*           If gradients are being used, also get the
*           directional derivative gtry.
         
            if (.not. fonly) then
               call m6grd ( ms, nb, nn, gsub, grd,
     $                      ne, nka, a, ha, ka,
     $                      xn, z, nwcore )
               gtp   = ddot( ms, grd, 1, p, 1 )
               gtry  = gtp - rmu*oldg
            end if
         
            go to 100
         end if
*     end loop

*     ------------------------------------------------------------------
*     The search has finished.
*     If some function evaluations were made, 
*     set alfa = alfbst  and  x = x + alfa*p.
*     ------------------------------------------------------------------
      if (inform .le. 7) then
         alfa   = alfbst
         call daxpy ( ms, alfa, p, 1, x, 1 )

*        If only function values were used in the search,
*        or if the last call to m6fun was not at the best point,
*        we have to recompute the function and gradient there
*        (and reset grd).
*        Beware: if central differences are used, m6grd overwrites p.
      
         if (fonly  .or.  .not. imprvd) then
            modefg = 2
            call m6fun ( 1, modefg, n, nb, ms, fsub,
     $                   ne, nka, a, ha, ka,
     $                   x, xn, z, nwcore )
            if (ierr .ne. 0) go to 999
      
            call m6grd ( ms, nb, nn, gsub, grd,
     $                   ne, nka, a, ha, ka,
     $                   xn, z, nwcore )
            if (ierr .ne. 0) go to 999
         end if
      end if

*     ------------------------------------------------------------------
*     Print a few error msgs if necessary.
*     ------------------------------------------------------------------
      if (inform .ge. 7) then
         if (inform .eq. 7) then
*-          if (iprint .gt. 0) write(iprint, 1700) numf
*-          if (isumm  .gt. 0) write(isumm , 1700) numf
         else if (oldg .ge. zero) then
*-          if (iprint .gt. 0) write(iprint, 1600) oldg
         end if

         if (.not. debug) then
            if (iprint .gt. 0) then
               write(iprint, 1800) alfmax, pnorm, gnorm, oldg, numf
            end if
         end if
      end if

      return

*     The user wants to stop.

  999 inform = -1
      return

 1000 format(// ' --------------------------------------------'
     $       /  ' Output from m6srch following iteration', i7,
     $      5x, ' Norm p =', 1p, e11.2, 5x, ' Norm g =', e11.2)
*1600 format(/ ' XXX  The search direction is uphill.   gtp =', 1p,e9.1)
*1700 format(  ' XXX  The linesearch has evaluated the functions', i5,
*    $   '  times')
 1800 format(' alfmax =', 1p, e11.2, '    pnorm  =', e11.2,
     $    '    gnorm  =',     e11.2, '    g(t)p  =', e11.2,
     $    '    numf =', i3)

*     end of m6srch
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine srchc ( first , debug , done  , imprvd, inform,
     $                   maxf  , numf  , nout  ,
     $                   alfmax,         epsaf , 
     $                   g0    , targtg, ftry  , gtry  , 
     $                   tolabs, tolrel, toltny,
     $                   alfa  , alfbst, fbest , gbest )

      implicit           double precision (a-h,o-z)
      logical            first , debug , done  , imprvd

************************************************************************
*     srchc  finds a sequence of improving estimates of a minimizer of
*     the univariate function f(alpha) in the interval (0,alfmax].
*     f(alpha) is a smooth function such that  f(0) = 0  and  f'(0) < 0.
*     srchc  requires both  f(alpha)  and  f'(alpha)  to be evaluated at
*     points in the interval.  Estimates of the minimizer are computed
*     using safeguarded cubic interpolation.
*
*     Reverse communication is used to allow the calling program to
*     evaluate f and f'.  Some of the parameters must be set or tested
*     by the calling program.  The remainder would ordinarily be local
*     variables.
*
*     Input parameters (relevant to the calling program)
*     --------------------------------------------------
*
*     first         must be true on the first entry. It is subsequently
*                   altered by srchc.
*
*     debug         specifies whether detailed output is wanted.
*
*     maxf          is an upper limit on the number of times srchc is to
*                   be entered consecutively with done = false
*                   (following an initial entry with first = true).
*
*     alfa          is the first estimate of a minimizer.  alfa is
*                   subsequently altered by srchc (see below).
*
*     alfmax        is the upper limit of the interval to be searched.
*
*     epsaf         is an estimate of the absolute precision in the
*                   computed value of f(0).
*
*     ftry, gtry    are the values of f, f'  at the new point
*                   alfa = alfbst + xtry.
*
*     g0            is the value of f'(0).  g0 must be negative.
*
*     tolabs,tolrel define a function tol(alfa) = tolrel*alfa + tolabs
*                   such that if f has already been evaluated at alfa,
*                   it will not be evaluated closer than tol(alfa).
*                   These values may be reduced by srchc.
*
*     targtg        is the target value of abs(f'(alfa)). The search
*                   is terminated when 
*                    abs(f'(alfa)) le targtg and f(alfa) lt 0.
*
*     toltny        is the smallest value that tolabs is allowed to be
*                   reduced to.
*
*     Output parameters (relevant to the calling program)
*     ---------------------------------------------------
*
*     imprvd        is true if the previous alfa was the best point so
*                   far.  Any related quantities should be saved by the
*                   calling program (e.g., gradient arrays) before
*                   paying attention to the variable done.
*
*     done = false  means the calling program should evaluate
*                      ftry = f(alfa),  gtry = f'(alfa)
*                   for the new trial alfa, and re-enter srchc.
*
*     done = true   means that no new alfa was calculated.  The value
*                   of inform gives the result of the search as follows
*
*                   inform = 1 means the search has terminated
*                              successfully with alfbst < alfmax.
*
*                   inform = 2 means the search has terminated
*                              successfully with alfbst = alfmax.
*
*                   inform = 3 means that the search failed to find a 
*                              point of sufficient decrease in maxf
*                              functions, but a lower point was found.
*
*                   inform = 4 means alfmax is so small that a search
*                              should not have been attempted.
*
*                   inform = 5 is never set by srchc.
*
*                   inform = 6 means the search has failed to find a
*                              useful step.  The interval of uncertainty 
*                              is [0,b] with b < 2*tolabs. A minimizer
*                              lies very close to alfa = 0, or f'(0) is
*                              not sufficiently accurate.
*
*                   inform = 7 if no better point could be found after 
*                              maxf  function calls.
*
*                   inform = 8 means the input parameters were bad.
*                              alfmax le toltny  or g0 ge zero.
*                              No function evaluations were made.
*
*     numf          counts the number of times srchc has been entered
*                   consecutively with done = false (i.e., with a new
*                   function value ftry).
*
*     alfa          is the point at which the next function ftry and
*                   derivative gtry must be computed.
*
*     alfbst        should be accepted by the calling program as the
*                   approximate minimizer, whenever srchc returns
*                   inform = 1 or 2 (and possibly 3).
*
*     fbest, gbest  will be the corresponding values of f, f'.
*
*
*     The following parameters retain information between entries
*     -----------------------------------------------------------
*
*     braktd        is false if f and f' have not been evaluated at
*                   the far end of the interval of uncertainty.  In this
*                   case, the point b will be at alfmax + tol(alfmax).
*
*     crampd        is true if alfmax is very small (le tolabs).  If the
*                   search fails, this indicates that a zero step should
*                   be taken.
*
*     extrap        is true if xw lies outside the interval of
*                   uncertainty.  In this case, extra safeguards are
*                   applied to allow for instability in the polynomial
*                   fit.
*
*     moved         is true if a better point has been found, i.e.,
*                   alfbst gt 0.
*
*     wset          records whether a second-best point has been
*                   determined it will always be true when convergence
*                   is tested.
*
*     nsamea        is the number of consecutive times that the 
*                   left-hand end point of the interval of uncertainty
*                   has remained the same.
*
*     nsameb        similarly for the right-hand end.
*
*     a, b, alfbst  define the current interval of uncertainty. 
*                   A minimizer lies somewhere in the interval 
*                   [alfbst + a, alfbst + b].
*
*     alfbst        is the best point so far.  It is always at one end 
*                   of the interval of uncertainty.  hence we have
*                   either  a lt 0,  b = 0  or  a = 0,  b gt 0.
*
*     fbest, gbest  are the values of f, f' at the point alfbst.
*
*     factor        controls the rate at which extrapolated estimates 
*                   of alfa may expand into the interval of uncertainty.
*                   factor is not used if a minimizer has been bracketed
*                   (i.e., when the variable braktd is true).
*
*     fw, gw        are the values of f, f' at the point alfbst + xw.
*                   they are not defined until wset is true.
*
*     xtry          is the trial point in the shifted interval (a, b).
*
*     xw            is such that  alfbst + xw  is the second-best point.
*                   it is not defined until  wset  is true.
*                   in some cases,  xw  will replace a previous  xw  
*                   that has a lower function but has just been excluded
*                   from the interval of uncertainty.
*
*
*     Systems Optimization Laboratory, Stanford University, California.
*     Original version February 1982.  Rev. May 1983.
*     Original f77 version 22-August-1985.
*     This version of srchc dated  24-Oct-91.
************************************************************************

      logical            braktd, crampd, extrap, moved , wset
      save               braktd, crampd, extrap, moved , wset

      save               nsamea, nsameb
      save               a     , b     , factor
      save               xtry  , xw    , fw    , gw    , tolmax

      logical            badfun, closef, found 
      logical            quitF , quitI
      logical            fitok , setxw 
      intrinsic          abs   , sqrt

      parameter        ( zero  =0.0d+0, point1 =0.1d+0, half   =0.5d+0 )
      parameter        ( one   =1.0d+0, two    =2.0d+0, three  =3.0d+0 )
      parameter        ( five  =5.0d+0, ten    =1.0d+1, eleven =1.1d+1 )

*     ------------------------------------------------------------------
*     Local variables
*     ===============
*
*     closef     is true if the new function ftry is within epsaf of
*                fbest (up or down).
*
*     found      is true if the sufficient decrease conditions hold at
*                alfbst.
*
*     quitF      is true when  maxf  function calls have been made.
*
*     quitI      is true when the interval of uncertainty is less than
*                2*tol.
*  ---------------------------------------------------------------------

      badfun = .false.
      quitF  = .false.
      quitI  = .false.
      imprvd = .false.

      if (first) then
*        ---------------------------------------------------------------
*        First entry.  Initialize various quantities, check input data
*        and prepare to evaluate the function at the initial alfa.
*        ---------------------------------------------------------------
         first  = .false.
         numf   = 0
         alfbst = zero
         badfun = alfmax .le. toltny  .or.  g0 .ge. zero
         done   = badfun
         moved  = .false.

         if (.not. done) then
            braktd = .false.
            crampd = alfmax .le. tolabs
            extrap = .false.
            wset   = .false.
            nsamea = 0
            nsameb = 0

            tolmax = tolabs + tolrel*alfmax
            a      = zero
            b      = alfmax + tolmax
            factor = five
            tol    = tolabs
            xtry   = alfa
            if (debug) then
               write (nout, 1000) g0    , tolabs, alfmax, 
     $                            targtg, tolrel, epsaf , crampd
            end if
         end if
      else
*        ---------------------------------------------------------------
*        Subsequent entries. The function has just been evaluated at
*        alfa = alfbst + xtry,  giving ftry and gtry.
*        ---------------------------------------------------------------
         if (debug) write (nout, 1100) alfa, ftry, gtry

         numf   = numf   + 1
         nsamea = nsamea + 1
         nsameb = nsameb + 1

         if (.not. braktd) then
            tolmax = tolabs + tolrel*alfmax
            b      = alfmax - alfbst + tolmax
         end if

*        See if the new step is better.  If alfa is large enough that
*        ftry can be distinguished numerically from zero,  the function
*        is required to be sufficiently negative.

         closef = abs( ftry - fbest ) .le.  epsaf
         if (closef) then
            imprvd =  abs( gtry ) .le. abs( gbest )
         else
            imprvd = ftry .lt. fbest
         end if

         if (imprvd) then

*           We seem to have an improvement.  The new point becomes the
*           origin and other points are shifted accordingly.

            fw     = fbest
            fbest  = ftry
            gw     = gbest
            gbest  = gtry
            alfbst = alfa
            moved  = .true.

            a      = a    - xtry
            b      = b    - xtry
            xw     = zero - xtry
            wset   = .true.
            extrap =       xw .lt. zero  .and.  gbest .lt. zero
     $               .or.  xw .gt. zero  .and.  gbest .gt. zero

*           Decrease the length of the interval of uncertainty.

            if (gtry .le. zero) then
               a      = zero
               nsamea = 0
            else
               b      = zero
               nsameb = 0
               braktd = .true.
            end if
         else

*           The new function value is not better than the best point so
*           far.  The origin remains unchanged but the new point may
*           qualify as xw.  xtry must be a new bound on the best point.

            if (xtry .le. zero) then
               a      = xtry
               nsamea = 0
            else
               b      = xtry
               nsameb = 0
               braktd = .true.
            end if

*           If xw has not been set or ftry is better than fw, update the
*           points accordingly.

            if (wset) then
               setxw = ftry .lt. fw  .or.  .not. extrap
            else
               setxw = .true.
            end if

            if (setxw) then
               xw     = xtry
               fw     = ftry
               gw     = gtry
               wset   = .true.
               extrap = .false.
            end if
         end if

*        ---------------------------------------------------------------
*        Check the termination criteria.  wset will always be true.
*        ---------------------------------------------------------------
         tol    = tolabs + tolrel*alfbst
         truea  = alfbst + a
         trueb  = alfbst + b

         found  = abs(gbest) .le. targtg
         quitF  = numf       .ge. maxf
         quitI  = b - a      .le. tol + tol

         if (quitI  .and. .not. moved) then

*           The interval of uncertainty appears to be small enough,
*           but no better point has been found.  Check that changing 
*           alfa by b-a changes f by less than epsaf.

            tol    = tol/ten
            tolabs = tol
            quitI  = abs(fw) .le. epsaf  .or.  tol .le. toltny
         end if

         done  = quitF  .or.  quitI  .or.  found

         if (debug) then
            write (nout, 1200) truea    , trueb , b - a , tol   ,
     $                         nsamea   , nsameb, numf  , 
     $                         braktd   , extrap, closef, imprvd,
     $                         found    , quitI ,
     $                         alfbst   , fbest , gbest ,
     $                         alfbst+xw, fw    , gw
         end if

*        ---------------------------------------------------------------
*        Proceed with the computation of a trial steplength.
*        The choices are...
*        1. Parabolic fit using derivatives only, if the f values are
*           close.
*        2. Cubic fit for a minimizer, using both f and f'.
*        3. Damped cubic or parabolic fit if the regular fit appears to
*           be consistently overestimating the distance to a minimizer.
*        4. Bisection, geometric bisection, or a step of  tol  if
*           choices 2 or 3 are unsatisfactory.
*        ---------------------------------------------------------------
         if (.not. done) then
            xmidpt = half*(a + b)
            s      = zero
            q      = zero

            if (closef) then
*              ---------------------------------------------------------
*              Fit a parabola to the two best gradient values.
*              ---------------------------------------------------------
               s      = gbest
               q      = gbest - gw
               if (debug) write (nout, 2200)
            else
*              ---------------------------------------------------------
*              Fit cubic through  fbest  and  fw.
*              ---------------------------------------------------------
               if (debug) write (nout, 2100)
               fitok  = .true.
               r      = three*(fbest - fw)/xw + gbest + gw
               absr   = abs( r )
               s      = sqrt( abs( gbest ) ) * sqrt( abs( gw ) )

*              Compute  q =  the square root of  r*r - gbest*gw.
*              The method avoids unnecessary underflow and overflow.

               if ((gw .lt. zero  .and.  gbest .gt. zero) .or.
     $             (gw .gt. zero  .and.  gbest .lt. zero)) then
                  scale  = absr + s
                  if (scale .eq. zero) then
                     q  = zero
                  else
                     q  = scale*sqrt( (absr/scale)**2 + (s/scale)**2 )
                  end if
               else if (absr .ge. s) then
                  q     = sqrt(absr + s)*sqrt(absr - s)
               else
                  fitok = .false.
               end if

               if (fitok) then

*                 Compute a minimizer of the fitted cubic.

                  if (xw .lt. zero) q = - q
                  s  = gbest -  r - q
                  q  = gbest - gw - q - q
               end if
            end if
*           ------------------------------------------------------------
*           Construct an artificial interval  (artifa, artifb)  in which
*           the new estimate of a minimizer must lie.  Set a default
*           value of xtry that will be used if the polynomial fit fails.
*           ------------------------------------------------------------
            artifa = a
            artifb = b
            if (.not. braktd) then

*              A minimizer has not been bracketed.  Set an artificial
*              upper bound by expanding the interval  xw  by a suitable
*              factor.

               xtry   = - factor*xw
               artifb =   xtry
               if (alfbst + xtry .lt. alfmax) factor = five*factor

            else if (extrap) then

*              The points are configured for an extrapolation.
*              Set a default value of  xtry  in the interval  (a, b)
*              that will be used if the polynomial fit is rejected.  In
*              the following,  dtry  and  daux  denote the lengths of
*              the intervals  (a, b)  and  (0, xw)  (or  (xw, 0),  if
*              appropriate).  The value of  xtry is the point at which
*              the exponents of  dtry  and  daux  are approximately
*              bisected.

               daux = abs( xw )
               dtry = b - a
               if (daux .ge. dtry) then
                  xtry = five*dtry*(point1 + dtry/daux)/eleven
               else
                  xtry = half * sqrt( daux ) * sqrt( dtry )
               end if
               if (xw .gt. zero)   xtry = - xtry
               if (debug) write (nout, 2400) xtry, daux, dtry

*              Reset the artificial bounds.  If the point computed by
*              extrapolation is rejected,  xtry will remain at the
*              relevant artificial bound.

               if (xtry .le. zero) artifa = xtry
               if (xtry .gt. zero) artifb = xtry
            else

*              The points are configured for an interpolation.  The
*              default value xtry bisects the interval of uncertainty.
*              the artificial interval is just (a, b).

               xtry   = xmidpt
               if (debug) write (nout, 2300) xtry
               if (nsamea .ge. 3  .or.  nsameb .ge. 3) then

*                 If the interpolation appears to be overestimating the
*                 distance to a minimizer,  damp the interpolation.

                  factor = factor / five
                  s      = factor * s
               else
                  factor = one
               end if
            end if
*           ------------------------------------------------------------
*           The polynomial fits give  (s/q)*xw  as the new step.
*           Reject this step if it lies outside  (artifa, artifb).
*           ------------------------------------------------------------
            if (q .ne. zero) then
               if (q .lt. zero) s = - s
               if (q .lt. zero) q = - q
               if (s*xw .ge. q*artifa  .and.  s*xw .le. q*artifb) then

*                 Accept the polynomial fit.

                  if (abs( s*xw ) .ge. q*tol) then
                     xtry = (s/q)*xw
                  else
                     xtry = zero
                  end if
                  if (debug) write (nout, 2500) xtry
               end if
            end if
         end if
      end if

*     ==================================================================

      if (.not. done) then
         alfa  = alfbst + xtry
         if (braktd  .or.  alfa .lt. alfmax - tolmax) then

*           The function must not be evaluated too close to a or b.
*           (It has already been evaluated at both those points.)

            if (xtry .le. a + tol  .or.  xtry .ge. b - tol) then
               if (half*(a + b) .le. zero) then
                  xtry = - tol
               else
                  xtry =   tol
               end if
               alfa = alfbst + xtry
            end if
         else

*           The step is close to, or larger than alfmax, replace it by
*           alfmax to force evaluation of  f  at the boundary.

            braktd = .true.
            xtry   = alfmax - alfbst
            alfa   = alfmax
         end if
      end if

*     ------------------------------------------------------------------
*     Exit.
*     ------------------------------------------------------------------
      if (done) then
         if      (badfun) then
            inform = 8
         else if (found) then
            if (alfbst .lt. alfmax) then
               inform = 1
            else
               inform = 2
            end if
         else if (moved ) then
            inform = 3
         else if (quitF) then
            inform = 7
         else if (crampd) then
            inform = 4
         else
            inform = 6
         end if
      end if

      if (debug) write (nout, 3000)
      return

 1000 format(/'     g0  tolabs  alfmax        ', 1p, 2e22.14,   e16.8
     $       /' targtg  tolrel   epsaf        ', 1p, 2e22.14,   e16.8
     $       /' crampd                        ',  l3)
 1100 format(/' alfa    ftry    gtry          ', 1p, 2e22.14,   e16.8)
 1200 format(/' a       b       b - a   tol   ', 1p, 2e22.14,  2e16.8
     $       /' nsamea  nsameb  numf          ', 3i3
     $       /' braktd  extrap  closef  imprvd', 4l3
     $       /' found   quitI                 ', 2l3
     $       /' alfbst  fbest   gbest         ', 1p, 3e22.14          
     $       /' alfaw   fw      gw            ', 1p, 3e22.14)
 2100 format( ' Cubic.   ')
 2200 format( ' Parabola.')
 2300 format( ' Bisection.              xmidpt', 1p,  e22.14)
 2400 format( ' Geo. bisection. xtry,daux,dtry', 1p, 3e22.14)
 2500 format( ' Polynomial fit accepted.  xtry', 1p,  e22.14)
 3000 format( ' ----------------------------------------------------'/)

*     End of  srchc .
      end
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine srchq ( first , debug , done  , imprvd, inform,
     $                   maxf  , numf  , nout  , 
     $                   alfmax, alfsml, epsaf , 
     $                   g0    , targtg, ftry  , 
     $                   tolabs, tolrel, toltny,
     $                   alfa  , alfbst, fbest  )

      implicit           double precision (a-h,o-z)
      logical            first , debug , done  , imprvd

************************************************************************
*     srchq  finds a sequence of improving estimates of a minimizer of
*     the univariate function f(alpha) in the interval (0,alfmax].
*     f(alpha) is a smooth function such that  f(0) = 0  and  f'(0) < 0.
*     srchq  requires  f(alpha) (but not f'(alpha)) to be evaluated
*     in the interval.  New estimates of a minimizer are computed using
*     safeguarded quadratic interpolation.
*
*     Reverse communication is used to allow the calling program to
*     evaluate f.  Some of the parameters must be set or tested by the
*     calling program.  The remainder would ordinarily be local
*     variables.
*
*     Input parameters (relevant to the calling program)
*     --------------------------------------------------
*
*     first         must be true on the first entry.  It is subsequently
*                   altered by srchq.
*
*     debug         specifies whether detailed output is wanted.
*
*     maxf          is an upper limit on the number of times srchq is to
*                   be entered consecutively with done = false 
*                   (following an initial entry with first = true).
*
*     alfa          is the first estimate of a minimizer.  alfa is
*                   subsequently altered by srchq (see below).
*
*     alfmax        is the upper limit of the interval to be searched.
*
*     alfsml        is intended to prevent inefficiency when a minimizer
*                   is very small, for cases where the calling program
*                   would prefer to redefine f'(alfa).  alfsml is
*                   allowed to be zero.  Early termination will occur if
*                   srchq determines that a minimizer lies somewhere in
*                   the interval [0, alfsml) (but not if alfmax is 
*                   smaller that alfsml).
*
*     epsaf         is an estimate of the absolute precision in the
*                   computed value of f(0).
*
*     ftry          the value of f at the new point
*                   alfa = alfbst + xtry.
*
*     g0            is the value of f'(0).  g0 must be negative.
*
*     tolabs,tolrel define a function tol(alfa) = tolrel*alfa + tolabs
*                   such that if f has already been evaluated at alfa,
*                   it will not be evaluated closer than tol(alfa).
*                   These values may be reduced by srchc.
*
*     targtg        is the target value of abs(f'(alfa)). The search
*                   is terminated when 
*                    abs(f'(alfa)) le targtg and f(alfa) lt 0.
*
*     toltny        is the smallest value that tolabs is allowed to be
*                   reduced to.
*
*     Output parameters (relevant to the calling program)
*     ---------------------------------------------------
*
*     imprvd        is true if the previous alfa was the best point so
*                   far.  Any related quantities should be saved by the
*                   calling program (e.g., arrays) before paying
*                   attention to the variable done.
*
*     done = false  means the calling program should evaluate ftry
*                   for the new trial step alfa, and reenter srchq.
*
*     done = true   means that no new alfa was calculated.  The value
*                   of inform gives the result of the search as follows
*
*                   inform = 1 means the search has terminated 
*                              successfully with alfbst < alfmax.
*
*                   inform = 2 means the search has terminated
*                              successfully with alfbst = alfmax.
*
*                   inform = 3 means that the search failed to find a 
*                              point of sufficient decrease in maxf
*                              functions, but a lower point was found.
*
*                   inform = 4 means alfmax is so small that a search
*                              should not have been attempted.
*
*                   inform = 5 means that the search was terminated
*                              because of alfsml (see above).
*
*                   inform = 6 means the search has failed to find a
*                              useful step.  The interval of uncertainty 
*                              is [0,b] with b < 2*tolabs. A minimizer
*                              lies very close to alfa = 0, or f'(0) is
*                              not sufficiently accurate.
*
*                   inform = 7 if no better point could be found after 
*                              maxf  function calls.
*
*                   inform = 8 means the input parameters were bad.
*                              alfmax le toltny  or  g0 ge zero.
*                              No function evaluations were made.
*
*     numf          counts the number of times srchq has been entered
*                   consecutively with done = false (i.e., with a new
*                   function value ftry).
*
*     alfa          is the point at which the next function ftry must 
*                   be computed.
*
*     alfbst        should be accepted by the calling program as the
*                   approximate minimizer, whenever srchq returns
*                   inform = 1, 2 or 3.
*
*     fbest         will be the corresponding value of f.
*
*     The following parameters retain information between entries
*     -----------------------------------------------------------
*
*     braktd        is false if f has not been evaluated at the far end
*                   of the interval of uncertainty.  In this case, the
*                   point b will be at alfmax + tol(alfmax).
*
*     crampd        is true if alfmax is very small (le tolabs).  If the
*                   search fails, this indicates that a zero step should
*                   be taken.
*
*     extrap        is true if alfbst has moved at least once and xv 
*                   lies outside the interval of uncertainty.  In this
*                   case, extra safeguards are applied to allow for
*                   instability in the polynomial fit.
*
*     moved         is true if a better point has been found, i.e., 
*                   alfbst gt 0.
*
*     vset          records whether a third-best point has been defined.
*
*     wset          records whether a second-best point has been 
*                   defined.  It will always be true by the time the
*                   convergence test is applied.
*
*     nsamea        is the number of consecutive times that the
*                   left-hand end point of the interval of uncertainty
*                   has remained the same.
*
*     nsameb        similarly for the right-hand end.
*
*     a, b, alfbst  define the current interval of uncertainty.
*                   A minimizer lies somewhere in the  interval
*                   [alfbst + a, alfbst + b].
*
*     alfbst        is the best point so far.  It lies strictly within
*                   [atrue,btrue]  (except when alfbst has not been
*                   moved, in which case it lies at the left-hand end
*                   point).  Hence we have a .le. 0 and b .gt. 0.
*
*     fbest         is the value of f at the point alfbst.
*
*     fa            is the value of f at the point alfbst + a.
*
*     factor        controls the rate at which extrapolated estimates of
*                   alfa  may expand into the interval of uncertainty.
*                   Factor is not used if a minimizer has been bracketed
*                   (i.e., when the variable braktd is true).
*
*     fv, fw        are the values of f at the points alfbst + xv  and
*                   alfbst + xw.  They are not defined until  vset  or
*                   wset  are true.
*
*     xtry          is the trial point within the shifted interval
*                   (a, b).  The new trial function value must be
*                   computed at the point alfa = alfbst + xtry.
*
*     xv            is such that alfbst + xv is the third-best point. 
*                   It is not defined until vset is true.
*
*     xw            is such that alfbst + xw is the second-best point. 
*                   It is not defined until wset is true.  In some
*                   cases,  xw will replace a previous xw that has a
*                   lower function but has just been excluded from 
*                   (a,b).
*
*     Systems Optimization Laboratory, Stanford University, California.
*     Original version February 1982.  Rev. May 1983.
*     Original F77 version 22-August-1985.
*     This version of srchq dated  24-Oct-91.
************************************************************************

      logical            braktd, crampd, extrap, moved , vset  , wset
      save               braktd, crampd, extrap, moved , vset  , wset

      save               nsamea, nsameb
      save               a     , b     , fa    , factor
      save               xtry  , xw    , fw    , xv    , fv    , tolmax

      logical            badfun, closef, found 
      logical            quitF , quitFZ, quitI , quitS 
      logical            setxv , xinxw
      intrinsic          abs   , sqrt

      parameter        ( zero  =0.0d+0, point1 =0.1d+0, half   =0.5d+0 )
      parameter        ( one   =1.0d+0, two    =2.0d+0, five   =5.0d+0 )
      parameter        ( ten   =1.0d+1, eleven =1.1d+1                 )

*     ------------------------------------------------------------------
*     Local variables
*     ===============
*
*     closef     is true if the worst function fv is within epsaf of
*                fbest (up or down).
*
*     found      is true if the sufficient decrease conditions holds at
*                alfbst.
*
*     quitF      is true when  maxf  function calls have been made.
*
*     quitFZ     is true when the three best function values are within
*                epsaf of each other, and the new point satisfies
*                fbest le ftry le fbest+epsaf.
*
*     quitI      is true when the interval of uncertainty is less than
*                2*tol.
*
*     quitS      is true as soon as alfa is too small to be useful;
*                i.e., btrue le alfsml.
*
*     xinxw      is true if xtry is in (xw,0) or (0,xw).
*     ------------------------------------------------------------------

      imprvd = .false.
      badfun = .false.
      quitF  = .false.
      quitFZ = .false.
      quitS  = .false.
      quitI  = .false.

      if (first) then
*        ---------------------------------------------------------------
*        First entry.  Initialize various quantities, check input data
*        and prepare to evaluate the function at the initial step alfa.
*        ---------------------------------------------------------------
         first  = .false.
         numf   = 0
         alfbst = zero
         badfun = alfmax .le. toltny  .or.  g0 .ge. zero
         done   = badfun
         moved  = .false.

         if (.not. done) then
            braktd = .false.
            crampd = alfmax .le. tolabs
            extrap = .false.
            vset   = .false.
            wset   = .false.
            nsamea = 0
            nsameb = 0

            tolmax = tolrel*alfmax + tolabs
            a      = zero
            b      = alfmax + tolmax
            fa     = zero
            factor = five
            tol    = tolabs
            xtry   = alfa
            if (debug) then
               write (nout, 1000) g0    , tolabs, alfmax, 
     $                            targtg, tolrel, epsaf , crampd
            end if
         end if
      else
*        ---------------------------------------------------------------
*        Subsequent entries.  The function has just been evaluated at
*        alfa = alfbst + xtry,  giving ftry.
*        ---------------------------------------------------------------
         if (debug) write (nout, 1100) alfa, ftry

         numf   = numf   + 1
         nsamea = nsamea + 1
         nsameb = nsameb + 1

         if (.not. braktd) then
            tolmax = tolabs + tolrel*alfmax
            b      = alfmax - alfbst + tolmax
         end if

*        Check if xtry is in the interval (xw,0) or (0,xw).

         if (wset) then
            xinxw =        zero .lt. xtry  .and.  xtry .le. xw
     $               .or.    xw .le. xtry  .and.  xtry .lt. zero
         else
            xinxw = .false.
         end if

         imprvd = ftry .lt. fbest
         if (vset) then
            closef = abs( fbest - fv ) .le. epsaf
         else
            closef = .false.
         end if

         if (imprvd) then

*           We seem to have an improvement.  The new point becomes the
*           origin and other points are shifted accordingly.

            if (wset) then
               xv     = xw - xtry
               fv     = fw
               vset   = .true.
            end if

            xw     = zero - xtry
            fw     = fbest
            wset   = .true.
            fbest  = ftry
            alfbst = alfa
            moved  = .true.

            a      = a    - xtry
            b      = b    - xtry
            extrap = .not. xinxw

*           Decrease the length of (a,b).

            if (xtry .ge. zero) then
               a      = xw
               fa     = fw
               nsamea = 0
            else
               b      = xw
               nsameb = 0
               braktd = .true.
            end if
         else if (closef  .and.  ftry - fbest .lt. epsaf) then

*           Quit if there has been no progress and ftry, fbest, fw
*           and fv are all within epsaf of each other.

            quitFZ = .true.
         else

*           The new function value is no better than the current best
*           point.  xtry must an end point of the new (a,b).

            if (xtry .lt. zero) then
               a      = xtry
               fa     = ftry
               nsamea = 0
            else
               b      = xtry
               nsameb = 0
               braktd = .true.
            end if

*           The origin remains unchanged but xtry may qualify as xw.

            if (wset) then
               if (ftry .lt. fw) then
                  xv     = xw
                  fv     = fw
                  vset   = .true.

                  xw     = xtry
                  fw     = ftry
                  if (moved) extrap = xinxw
               else if (moved) then
                  if (vset) then
                     setxv = ftry .lt. fv  .or.  .not. extrap
                  else
                     setxv = .true.
                  end if

                  if (setxv) then
                     if (vset  .and.  xinxw) then
                        xw = xv
                        fw = fv
                     end if
                     xv   = xtry
                     fv   = ftry
                     vset = .true.
                  end if
               else
                  xw  = xtry
                  fw  = ftry
               end if
            else
               xw     = xtry
               fw     = ftry
               wset   = .true.
            end if
         end if

*        ---------------------------------------------------------------
*        Check the termination criteria.
*        ---------------------------------------------------------------
         tol    = tolabs + tolrel*alfbst
         truea  = alfbst + a
         trueb  = alfbst + b

         found  = moved  .and.  abs(fa - fbest) .le. -a*targtg
         quitF  = numf  .ge. maxf
         quitI  = b - a .le. tol + tol
         quitS  = trueb .le. alfsml

         if (quitI  .and.  .not. moved) then

*           The interval of uncertainty appears to be small enough,
*           but no better point has been found.  Check that changing 
*           alfa by b-a changes f by less than epsaf.

            tol    = tol/ten
            tolabs = tol
            quitI  = abs(fw) .le. epsaf  .or.  tol .le. toltny
         end if

         done  = quitF  .or.  quitFZ  .or.  quitS  .or.  quitI
     $                  .or.  found

         if (debug) then
            write (nout, 1200) truea    , trueb , b-a   , tol   ,
     $                         nsamea   , nsameb, numf  ,
     $                         braktd   , extrap, closef, imprvd,
     $                         found    , quitI , quitFZ, quitS ,
     $                         alfbst   , fbest ,
     $                         alfbst+xw, fw
            if (vset) then
               write (nout, 1300) alfbst + xv, fv
            end if
         end if

*        ---------------------------------------------------------------
*        Proceed with the computation of an estimate of a minimizer.
*        The choices are...
*        1. Parabolic fit using function values only.
*        2. Damped parabolic fit if the regular fit appears to be
*           consistently overestimating the distance to a minimizer.
*        3. Bisection, geometric bisection, or a step of tol if the
*           parabolic fit is unsatisfactory.
*        ---------------------------------------------------------------
         if (.not. done) then
            xmidpt = half*(a + b)
            s      = zero
            q      = zero

*           ============================================================
*           Fit a parabola.
*           ============================================================
*           See if there are two or three points for the parabolic fit.

            gw = (fw - fbest)/xw
            if (vset  .and.  moved) then

*              Three points available.  Use fbest, fw and fv.

               gv = (fv - fbest)/xv
               s  = gv - (xv/xw)*gw
               q  = two*(gv - gw)
               if (debug) write (nout, 2200)
            else

*              Only two points available.  Use fbest, fw and g0.

               if (moved) then
                  s  = g0 - two*gw
               else
                  s  = g0
               end if
               q = two*(g0 - gw)
               if (debug) write (nout, 2100)
            end if

*           ------------------------------------------------------------
*           Construct an artificial interval (artifa, artifb) in which 
*           the new estimate of the steplength must lie.  Set a default
*           value of  xtry  that will be used if the polynomial fit is
*           rejected. In the following, the interval (a,b) is considered
*           the sum of two intervals of lengths  dtry  and  daux, with
*           common end point the best point (zero).  dtry is the length
*           of the interval into which the default xtry will be placed
*           and endpnt denotes its non-zero end point.  The magnitude of
*           xtry is computed so that the exponents of dtry and daux are
*           approximately bisected.
*           ------------------------------------------------------------
            artifa = a
            artifb = b
            if (.not. braktd) then

*              A minimizer has not yet been bracketed.  
*              Set an artificial upper bound by expanding the interval
*              xw  by a suitable factor.

               xtry   = - factor*xw
               artifb =   xtry
               if (alfbst + xtry .lt. alfmax) factor = five*factor
            else if (vset .and. moved) then

*              Three points exist in the interval of uncertainty.
*              Check if the points are configured for an extrapolation
*              or an interpolation.

               if (extrap) then

*                 The points are configured for an extrapolation.

                  if (xw .lt. zero) endpnt = b
                  if (xw .gt. zero) endpnt = a
               else

*                 If the interpolation appears to be overestimating the
*                 distance to a minimizer,  damp the interpolation step.

                  if (nsamea .ge. 3  .or.   nsameb .ge. 3) then
                     factor = factor / five
                     s      = factor * s
                  else
                     factor = one
                  end if

*                 The points are configured for an interpolation.  The
*                 artificial interval will be just (a,b).  Set endpnt so
*                 that xtry lies in the larger of the intervals (a,b) 
*                 and  (0,b).

                  if (xmidpt .gt. zero) then
                     endpnt = b
                  else
                     endpnt = a
                  end if

*                 If a bound has remained the same for three iterations,
*                 set endpnt so that  xtry  is likely to replace the
*                 offending bound.

                  if (nsamea .ge. 3) endpnt = a
                  if (nsameb .ge. 3) endpnt = b
               end if

*              Compute the default value of  xtry.

               dtry = abs( endpnt )
               daux = b - a - dtry
               if (daux .ge. dtry) then
                  xtry = five*dtry*(point1 + dtry/daux)/eleven
               else
                  xtry = half*sqrt( daux )*sqrt( dtry )
               end if
               if (endpnt .lt. zero) xtry = - xtry
               if (debug) write (nout, 2500) xtry, daux, dtry

*              If the points are configured for an extrapolation set the
*              artificial bounds so that the artificial interval lies
*              within (a,b).  If the polynomial fit is rejected,  xtry 
*              will remain at the relevant artificial bound.

               if (extrap) then
                  if (xtry .le. zero) then
                     artifa = xtry
                  else
                     artifb = xtry
                  end if
               end if
            else

*              The gradient at the origin is being used for the
*              polynomial fit.  Set the default xtry to one tenth xw.

               if (extrap) then
                  xtry = - xw
               else
                  xtry   = xw/ten
               end if
               if (debug) write (nout, 2400) xtry
            end if

*           ------------------------------------------------------------
*           The polynomial fits give (s/q)*xw as the new step.  Reject
*           this step if it lies outside (artifa, artifb).
*           ------------------------------------------------------------
            if (q .ne. zero) then
               if (q .lt. zero) s = - s
               if (q .lt. zero) q = - q
               if (s*xw .ge. q*artifa   .and.   s*xw .le. q*artifb) then
 
*                 Accept the polynomial fit.

                  if (abs( s*xw ) .ge. q*tol) then
                     xtry = (s/q)*xw
                  else
                     xtry = zero
                  end if
                  if (debug) write (nout, 2600) xtry
               end if
            end if
         end if
      end if
*     ==================================================================

      if (.not. done) then
         alfa  = alfbst + xtry
         if (braktd  .or.  alfa .lt. alfmax - tolmax) then

*           The function must not be evaluated too close to a or b.
*           (It has already been evaluated at both those points.)

            xmidpt = half*(a + b)
            if (xtry .le. a + tol  .or.  xtry .ge. b - tol) then
               if (xmidpt .le. zero) then
                  xtry = - tol
               else
                  xtry =   tol
               end if
            end if

            if (abs( xtry ) .lt. tol) then
               if (xmidpt .le. zero) then
                  xtry = - tol
               else
                  xtry =   tol
               end if
            end if
            alfa  = alfbst + xtry
         else

*           The step is close to or larger than alfmax, replace it by
*           alfmax to force evaluation of the function at the boundary.

            braktd = .true.
            xtry   = alfmax - alfbst
            alfa   = alfmax
         end if
      end if
*     ------------------------------------------------------------------
*     Exit.
*     ------------------------------------------------------------------
      if (done) then
         if      (badfun) then
            inform = 8
         else if (quitS ) then
            inform = 5
         else if (found) then
            if (alfbst .lt. alfmax) then
               inform = 1
            else
               inform = 2
            end if
         else if (moved ) then
            inform = 3
         else if (quitF) then
            inform = 7
         else if (crampd) then
            inform = 4
         else
            inform = 6
         end if
      end if

      if (debug) write (nout, 3000)
      return

 1000 format(/'     g0  tolabs  alfmax        ', 1p, 2e22.14,   e16.8
     $       /' targtg  tolrel   epsaf        ', 1p, 2e22.14,   e16.8
     $       /' crampd                        ',  l3)
 1100 format(/' alfa    ftry                  ', 1p,2e22.14          )
 1200 format(/' a       b       b - a   tol   ', 1p,2e22.14,   2e16.8
     $       /' nsamea  nsameb  numf          ', 3i3
     $       /' braktd  extrap  closef  imprvd', 4l3
     $       /' found   quitI   quitFZ  quitS ', 4l3
     $       /' alfbst  fbest                 ', 1p,2e22.14          
     $       /' alfaw   fw                    ', 1p,2e22.14)
 1300 format( ' alfav   fv                    ', 1p,2e22.14 /)
 2100 format( ' Parabolic fit,    two points. ')
 2200 format( ' Parabolic fit,  three points. ')
 2400 format( ' Exponent reduced.  Trial point', 1p,  e22.14)
 2500 format( ' Geo. bisection. xtry,daux,dtry', 1p, 3e22.14)
 2600 format( ' Polynomial fit accepted.  xtry', 1p,  e22.14)
 3000 format( ' ----------------------------------------------------'/)

*     End of  srchq .
      end
