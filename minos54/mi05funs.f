************************************************************************
*
*     File  mi05funs fortran.
*
*     funobj   funcon   matmod
*     t2obj    t3obj    t4obj    t4con   t5obj   t6con   t7obj
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine funobj( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     This is the default version of funobj for MINOS.
*     It is for one of the test problems
*        t2banana, t3qp, t4manne, t5weapon or t6wood,
*     which will specify
*        Problem number   1112, 1113, 1114, 1115 or 1116
*     respectively in order to identify themselves.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      if      (nprob .eq. 1112) then
         call t2obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )
      else if (nprob .eq. 1113) then
         call t3obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )
      else if (nprob .eq. 1114) then
         call t4obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )
      else if (nprob .eq. 1115) then
         call t5obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )
      else if (nprob .eq. 1116) then
*        t6wood doesn't have a nonlinear objective
      else if (nprob .eq. 1117) then
         call t7obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )
      else
         if (iprint .gt. 0) write(iprint, 9000)
         if (isumm  .gt. 0) write(isumm , 9000)
         mode   = -1
      end if

 9000 format(/ ' XXX Subroutine funobj has not been loaded.')

*     end of funobj
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine funcon( mode, m, n, njac, x, f, g,
     $                   nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), f(m), g(njac), z(nwcore)

*     ------------------------------------------------------------------
*     This is the default version of funcon for MINOS.
*     It is for one of the test problems
*        t4manne or t6wood,
*     which will specify
*        Problem number   1114 or 1116
*     respectively in order to identify themselves.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      if      (nprob .eq. 1114) then
         call t4con ( mode, m, n, njac, x, f, g,
     $                nstate, nprob, z, nwcore )
      else if (nprob .eq. 1116) then
         call t6con ( mode, m, n, njac, x, f, g,
     $                nstate, nprob, z, nwcore )
      else
         if (iprint .gt. 0) write(iprint, 9000)
         if (isumm  .gt. 0) write(isumm , 9000)
         mode   = -1
      end if

 9000 format(/ ' XXX Subroutine funcon has not been loaded.')

*     end of funcon
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine matmod( ncycle, nprob, finish,
     $                   m, n, nb, ne, nka, ns, nscl, nname,
     $                   a, ha, ka, bl, bu,
     $                   ascale, hs, name1, name2,
     $                   x, pi, rc, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), name1(nname), name2(nname)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   x(nb), pi(m), rc(nb), z(nwcore)
      logical            finish

*     ------------------------------------------------------------------
*     This is the default version of  matmod  for MINOS.
*     It belongs to the nonlinear test problem t4manne,
*     which will specify
*        Problem number   1114
*     in order to identify itself.
*
*     If Cycle limit > 1,  matmod  is called before the first cycle
*     (ncycle = 0) and at the end of each cycle except the last.
*
*     25 Nov 1991: nname and rc added as parameters.
*                  nname = nb for regular MINOS, 1 for minoss.
*     26 Jun 1992: If ncycle ge 1, rc contains reduced costs for the
*                  columns and rows, using the appropriate objective
*                  and pi.  (If infeasible, it is the Phase 1 obj.
*                  If feasible and maximizing, the sign is changed.)
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /cyclcm/ cnvtol,jnew,materr,maxcy,nephnt,nphant,nprint

      if (nprob .eq. 1114) then
         bu(n)  = 0.1
         if (iprint .gt. 0) write(iprint, 1000) bu(n)
         if (isumm  .gt. 0) write(isumm , 1000) bu(n)
      else
         if (iprint .gt. 0) write(iprint, 9000)
         if (isumm  .gt. 0) write(isumm , 9000)
         finish = .true.
      end if

 1000 format(/ ' matmod  changes last upper bound to', f5.2)
 9000 format(/ ' XXX Subroutine matmod has not been loaded.')

*     end of matmod
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t2obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     This is funobj for problem t2banana
*     (Rosenbrock's banana function).
*     f(x) = 100(x2 - x1**2)**2 + (1 - x1)**2.
*     ------------------------------------------------------------------

      x1     =   x(1)
      x2     =   x(2)
      t1     =   x2 - x1**2
      t2     =   1.0d+0 - x1
      f      =   100.0d+0 * t1**2  +  t2**2
      g(1)   = - 400.0d+0 * t1*x1  -  2.0d+0 * t2
      g(2)   =   200.0d+0 * t1

*     end of t2obj  (funobj for t2banana)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t3obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     This is funobj for problem t3qp.
*     f(x) = 1/2 x'Qx,   g(x) = Qx.
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0,  half = 0.5d+0,
     $                   two  = 2.0d+0,  four = 4.0d+0,
     $                   maxn = 10 )
      double precision   Q(maxn,maxn)
      save               Q

      if (nstate .eq. 1) then

*        Define Q on the first entry.
*        Here we assume n = 3.

         Q(1,1) = four
         Q(1,2) = two
         Q(1,3) = two
         Q(2,2) = four
         Q(2,3) = zero
         Q(3,3) = two

*        Make Q symmetric.

         do 20 j = 1, n-1
            do 10 i = 2, n
               Q(i,j) = Q(j,i)
   10       continue
   20    continue
      end if

*     Compute f and g on all entries.
*     We first compute g = Qx, then f = 1/2 x'g.
*     In Fortran it is best to run down the columns of Q.

      do 200 i = 1, n      
         g(i)  = zero
  200 continue

      do 300 j = 1, n
         xj    = x(j)
         do 250 i = 1, n
            g(i)  = g(i)  +  Q(i,j) * xj
  250    continue
  300 continue

      f      = half  *  ddot  ( n, x, 1, g, 1 )

*     end of t3obj  (funobj for t3qp)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t4obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     This is funobj for problem t4manne.
*
*     The data bt(*) is computed by t4con  on its first entry.
*
*     For test purposes, we look at    Derivative level
*     and sometimes pretend that we don't know the first
*     three elements of the gradient.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /manne / b,at(100),bt(100)

      intrinsic          log
      logical            gknown
      parameter        ( zero = 0.0d+0 )

      gknown = lderiv .eq. 1  .or.  lderiv .eq. 3
      nt     = n/2
      f      = zero

      do 50 j = 1, nt
         xcon = x(nt+j)
         f    = f  +  bt(j) * log(xcon)
         if (mode .eq. 2) then
            g(j) = zero
            if (gknown  .or.  j .gt. 3) g(nt+j) = bt(j) / xcon
         end if
   50 continue

*     end of t4obj  (funobj for t4manne)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t4con ( mode, m, n, njac, x, f, g,
     $                   nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), f(m), g(njac), z(nwcore)

*     ------------------------------------------------------------------
*     This is funcon for problem t4manne.
*
*     For test purposes, we look at    Derivative level
*     and sometimes pretend that we don't know the first
*     three elements of the gradient.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /manne / b,at(100),bt(100)

      logical            gknown
      parameter        ( one = 1.0d+0 )

      gknown = lderiv .ge. 2
      nt     = n

*     ---------------------------------------
*     First entry.  Define b, at(*) and bt(*) 
*     for this and all subsequent entries.
*     ---------------------------------------
      if (nstate .eq. 1) then
         grow   = 0.03
         beta   = 0.95
         xk0    = 3.0
         xc0    = 0.95
         xi0    = 0.05
         b      = 0.25
         if (iprint .gt. 0) write(iprint, 1000) nt, b
      
         a      = (xc0 + xi0) / xk0**b
         gfac   = (one + grow)**(one - b)
         at(1)  = a*gfac
         bt(1)  = beta

         do 10 j  = 2, nt
            at(j) = at(j-1)*gfac
            bt(j) = bt(j-1)*beta
   10    continue

         bt(nt) = bt(nt) / (one - beta)
      end if

*     -------------
*     Normal entry.
*     -------------
      do 150 j = 1, nt
         xkap  = x(j)
         f(j)  = at(j) * xkap**b
         if (mode .eq. 2) then
            if (gknown  .or.  j .gt. 3) g(j) = b*f(j) / xkap
         end if
  150 continue

*     ------------
*     Final entry.
*     ------------
      if (nstate .ge. 2) then
         if (iprint .gt. 0) write(iprint, 2000) (f(j), j = 1, nt)
      end if
      return

 1000 format(// ' This is problem  t4manne.   nt =', i4, '   b =', f8.3)
 2000 format(// ' Final nonlinear function values' / (5f12.5))

*     end of t4con  (funcon for t4manne)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t5obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     t5obj  is funobj for the Weapon Assignment problem t5weapon.
*     It assumes the Specs file contains input data after the End card.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt

      intrinsic          log
      parameter        ( nweapn = 5,  ntargt = 20,  zero   = 0.0d+0 )
      double precision   q(nweapn,ntargt), ql(nweapn,ntargt), u(ntargt)
      save               q               , ql               , u

      if (nstate .eq. 1) then
*        ----------------------------------------------------
*        First entry.  Read some data defining the objective.
*        ----------------------------------------------------
         do 20 i = 1, nweapn
            read (ispecs, '(18f4.2)') (q(i,j), j = 1, ntargt)
            do 10 j = 1, ntargt
               ql(i,j) = log( q(i,j) )
   10       continue
   20    continue
      
         read (ispecs, '(18f4.0)') u
      end if

*     -------------
*     Normal entry.
*     -------------
      k      = 0
      f      = zero
      
      do 990 j = 1, ntargt
         t     = u(j)
         do 960 i = 1, nweapn
            xk    = x(k+i)
            if (xk .gt. zero) t = t * q(i,j)**xk
  960    continue
      
         if (mode .eq. 2) then
            do 970  i = 1, nweapn
               g(k+i) = t * ql(i,j)
  970       continue
         end if
      
         k     = k + nweapn
         f     = f + (t - u(j))
  990 continue

*     end of t5obj  (funobj for t5weapon)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t6con ( mode, m, n, njac, x, f, g,
     $                   nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), f(m), g(m,n), z(nwcore)

*     ------------------------------------------------------------------
*     t6con  is funcon for test problem t6wood,
*     a chemical engineering design problem.
*     Originally called  woplant  (wood plant?).
*     m = 5,  n = 10.
*
*     For test purposes, we test  Derivative level
*     to decide whether or not to compute gradients.
*
*     Dec 1981: Original MINOS version obtained via Bruce Murtagh.
*     Oct 1991: Converted to f77 for test problem t6wood.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)

      parameter (one   = 1.0d+0,  two  = 2.0d+0,
     $           three = 3.0d+0,  four = 4.0d+0,
     $           half  = 0.5d+0,  tenk = 10000.0d+0,  vrho = 3000.0d+0)

      if (nstate .eq. 1) then
         if (iprint .gt. 0) write(iprint, 5) lderiv
         if (isumm  .gt. 0) write(isumm , 5) lderiv
    5    format(/ ' This is problem t6wood.  Derivative level =', i3 /)
      end if

*     Transform to original variables.

      fg     = tenk*(one + x(1))
      fp     = tenk*(one + x(2))
      fd     = tenk*(one + x(3))
      fra    = tenk*(one + x(4))
      frp    = tenk*(one + x(5))
      fre    = tenk*(one + x(6))
      frb    = tenk*(one + x(7))
      frc    = tenk*(one + x(8))
      fr     = tenk*(one + x(9))
      temp   = 630.0d+0 + 50.0d+0*x(10)

*     Rate constants.

      ak1    = 5.9755d+09 * dexp(-1.2d+4/temp)
      ak2    = 2.5962d+12 * dexp(-1.5d+4/temp)
      ak3    = 9.6283d+15 * dexp(-2.0d+4/temp)

*     Rate terms.

      fr2    = fr**2
      r1     = ak1*fra*frb*vrho/fr2
      r2     = ak2*frb*frc*vrho/fr2
      r3     = ak3*frc*frp*vrho/fr2

*     Nonlinear functions.

      recip  = one/(fr - fg - fp)
      f(1)   = two*r2       - fd*recip*fre
      f(2)   = r2 - half*r3 - fd*recip*(frp - fp) - fp
      f(3)   = - r1         - fd*recip*fra
      f(4)   = - r1 - r2    - fd*recip*frb
      f(5)   = 1.5d+0*r3    - fg

*     Scale them.

      do 10 i = 1, m
         f(i) = f(i) / tenk
   10 continue

*     Compute the Jacobian (if MINOS wants it).

      if (mode .eq. 0  .or.  lderiv .lt. 2) return

      b1t    = 1.2d+4/temp**2
      b2t    = 1.5d+4/temp**2
      b3t    = 2.0d+4/temp**2
      rr     = recip**2

      g(1,1) = - fd*fre*rr
      g(1,2) =   g(1,1)
      g(1,3) = - fre*recip
      g(1,6) = - fd *recip
      g(1,7) =   two*r2/frb
      g(1,8) =   two*r2/frc
      g(1,9) = - four*r2/fr - g(1,1)
      g(1,10)=   two*r2*b2t

      g(2,1) = - fd*(frp - fp)*rr
      g(2,2) =   fd*(fr - frp - fg)*rr - one
      g(2,3) = - (frp - fp)*recip
      g(2,5) = - half*r3/frp - fd*recip
      g(2,7) =   r2/frb
      g(2,8) =   (r2 - half*r3)/frc
      g(2,9) = - two*(r2 - half*r3)/fr - g(2,1)
      g(2,10)=   r2*b2t - half*r3*b3t

      g(3,1) = - fd*fra*rr
      g(3,2) =   g(3,1)
      g(3,3) = - fra*recip
      g(3,4) = - r1/fra - fd*recip
      g(3,7) = - r1/frb
      g(3,9) =   two*r1/fr - g(3,1)
      g(3,10)= - r1*b1t

      g(4,1) = - fd*frb*rr
      g(4,2) =   g(4,1)
      g(4,3) = - frb*recip
      g(4,4) = - r1/fra
      g(4,7) = - (r1 + r2)/frb - fd*recip
      g(4,8) = - r2/frc
      g(4,9) =   two*(r1+r2)/fr - g(4,1)
      g(4,10)= - r1*b1t - r2*b2t

      g(5,1) = - 1.0d+0
      g(5,5) =   1.5d+0*r3/frp
      g(5,8) =   1.5d+0*r3/frc
      g(5,9) = - three *r3/fr
      g(5,10)=   1.5d+0*r3*b3t

*     Rescale the temperature derivatives.

      do 20 i = 1, m
         g(i,10) = g(i,10) * 5.0d-3
   20 continue

*     end of t6con  (funcon for Chemical Design Problem woplant)
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine t7obj ( mode, n, x, f, g, nstate, nprob, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   x(n), g(n), z(nwcore)

*     ------------------------------------------------------------------
*     t7obj  is funobj for Alan Manne's energy model ETAMACRO.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      intrinsic          log
      parameter        ( maxt = 16,  zero =  0.0d+0,  one = 1.0d+0 )

      double precision   a1, b1, alpha, beta, rho, rate, esub, refp
      save               a1, b1, alpha, beta, rho, rate, esub, refp

      double precision   deltat(maxt), zks(maxt), zls(maxt), zes(maxt),
     $                   zns   (maxt), zys(maxt), zlb(maxt)
      save               deltat      , zks      , zls      , zes      ,
     $                   zns         , zys      , zlb

      data               zlb   / 1.160d+0, 1.446d+0, 1.717d+0, 2.039d+0,
     $                           2.364d+0, 2.740d+0, 3.101d+0, 3.508d+0,
     $                           3.873d+0, 4.276d+0, 4.721d+0, 5.213d+0,
     $                           5.755d+0, 6.354d+0, 7.016d+0, 7.746d+0/

      data               rate  / 10.000d+0 /,  esub  / 0.2000d+0 /,
     $                   beta  / 0.4000d+0 /,  alpha / 0.3333d+0 /,
     $                   refp  / 0.8000d+0 /

      if (nstate .eq. 1) then
*        ----------------------------------------------------
*        First entry.  Define some data.
*        ----------------------------------------------------
         spda   = 0.96d+0
         s5     = spda**5
         rho    = (esub - one) / esub
         delta  = one / (one + rate/100)

*        Compute coefficients a1, b1 with given data at 1970.

         zel    = 1.650d+0
         zne    = 0.509d+0
         pn     = 0.080d+0
         gnp    = 1.360d+0
         zkp    = 2.5d+0 * gnp
         y      = gnp  +  zne * pn / (one - beta)
         b1     = pn * y**(rho - one) / (one - beta)
     $               * zel**(-rho*beta)
     $               * zne**(one - rho*(one - beta))
         a1     = b1 * zel**( rho*beta) * zne**(rho*(one - beta))
         a1     = (y**rho  -  a1) / zkp**(alpha*rho)
         if (iprint .gt. 0) then
            write(iprint, '(/ 1p,   a   , e16.8, 4x,  a   , e16.8)')
     $                            ' a =', a1   ,    ' b =', b1
         end if

*        Compute deltat(j).

         d5        = delta**5
         deltat(1) = 1000
         do 130  j = 2, maxt
            deltat(j) = deltat(j-1) * d5
  130    continue
         deltat(maxt) = deltat(maxt) / (one - d5)

*        Compute surviving quantities.

         do 140  j = 1, maxt
            s5j    = s5**j
            zys(j) = y   * s5j
            zks(j) = zkp * s5j
            zls(j) = (zlb(j) - s5j)**(one - alpha)
            zes(j) = zel * s5j
            zns(j) = zne * s5j
  140    continue
      end if

*     -------------------------------------------
*     Normal entry.
*     Some output is produced on the final entry.
*     -------------------------------------------
      if (nstate .eq. 2) then
         if (iprint .gt. 0) then
            write(iprint, '(/ a / a)')
     $         ' Time series of Gross Output, Annual Consumption and',
     $         ' Cumulative Consumption discounted at 5% and 10% are'
         end if
      end if

      f      = zero
      cdc5   = zero
      cdc10  = zero

      do 190 j = 1, maxt
         j1    = j
         j2    = j1 + maxt
         j3    = j2 + maxt
         j4    = j3 + maxt
         j5    = j4 + maxt
         dxk   = x(j1) - zks(j)
         dxe   = x(j2) - zes(j)
         dxn   = x(j3) - zns(j)
         r     = (dxk**alpha * zls(j))**rho
         e     = ((dxe/dxn)**beta * dxn)**rho
         d     = (a1*r + b1*e)**(one/rho)
         y     = zys(j) + d
         c     = y - x(j4) - x(j5)
         f     = f + deltat(j) * log(c)

         if (nstate .eq. 2) then
            it     = 5*(j - 1)
            cdc5   = cdc5  +  5*(one / 1.05d+0)**it * c
            cdc10  = cdc10 +  5*(one / 1.10d+0)**it * c
            iyr    = 1975  +  it
            if (iprint .gt. 0) then
               write(iprint, '(i5, 4f9.3)') iyr, y, c, cdc5, cdc10
            end if
         end if

*        Compute the gradient.

         dc     = deltat(j) / c
         d      = dc * d**(one - rho)
         g(j1)  = d  * a1 * r * alpha / dxk
         g(j2)  = d  * b1 * e * beta  / dxe
         g(j3)  = d  * b1 * e * (one - beta) / dxn
         g(j4)  = - dc
         g(j5)  = g(j4)
  190 continue

*     end of t7obj  (funobj for ETAMACRO)
      end
