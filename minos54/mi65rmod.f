************************************************************************
*
*     File  mi65rmod fortran.
*
*     m6bfgs   m6bswp   m6radd   m6rcnd   m6rdel
*     m6rmod   m6rset   m6rsol   m6swap
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6bfgs( maxr, n, nr, r, g, g2, p, v,
     $                   step, told, tolz, inform )

      implicit           double precision (a-h,o-z)
      double precision   r(nr), g(n), g2(n), p(n), v(n)

*     ------------------------------------------------------------------
*     m6bfgs  applies the BFGS update to the upper-triangular matrix  r,
*     which holds the cholesky factor of the quasi-newton approximation
*     of the (projected) hessian.
*
*     r  contains a triangle of size  ncolr = min( n, maxr ).
*     If  n .gt. maxr,  r  also contains a diagonal of size  n - maxr.
*
*     p       holds the search direction.  It is overwritten.
*     v       must satisfy  r(t)*v = g.    It is overwritten.
*
*     On exit,
*     inform = 0  if no update was performed,
*            = 1  if the update was successful,
*            = 2  if it was nearly singular.
*     ------------------------------------------------------------------

      intrinsic          abs, min, sqrt
      parameter        ( one = 1.0d+0 )

      inform = 0
      ncolr  = min( n, maxr )
      gtp    = ddot  ( n, g , 1, p, 1 )
      gtp2   = ddot  ( n, g2, 1, p, 1 )
      if (gtp2 .le. 0.91*gtp) return

      delta1 = one / sqrt( abs( gtp ) )
      delta2 = one / sqrt( step*(gtp2 - gtp) )

*     Normalize  v  and change its sign.

      call dscal ( n, (- delta1), v, 1 )

*     Avoid cancellation error in forming the new vector  p.

      if ( abs( delta1/delta2 - one ) .ge. 0.5d+0) then
         do 100 j = 1, n
            p(j)  = delta2*( g2(j) - g(j) )  +  delta1*g(j)
  100    continue
      else
         d      = delta1 - delta2
         do 120 j = 1, n
            p(j)  = delta2*g2(j)  +  d*g(j)
  120    continue
      end if

*     Find the last nonzero in  v.

      do 450 ii = 1, ncolr
         lastv  = ncolr + 1 - ii
         if (abs( v(lastv) ) .gt. tolz) go to 500
  450 continue
      lastv = 1

*     Triangularize   r  +  v * p(t).

  500 call m6rmod( ncolr, nr, r, v, p, lastv, told, tolz, inform )

*     Deal with surplus diagonals of  r.

      if (n .gt. maxr) then
         j1     = maxr + 1
         l      = maxr*j1/2
         do 650 j = j1, n
            l     = l + 1
            r(l)  = r(l) + v(j)*p(j)
  650    continue
      end if

*     end of m6bfgs
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6bswp( n, nr, r, v, w, lastv, told, tolz, inform )

      implicit           double precision (a-h,o-z)
      double precision   r(nr), v(n), w(n)

*     ------------------------------------------------------------------
*     m6bswp  modifies the upper-triangular matrix  r
*     to account for a basis exchange in which the  lastv-th
*     superbasic variable becomes basic.  r  is changed to
*     r  +  v*w(t),  which is triangularized by  r1mod,
*     where  v  is the  lastv-th column of  r,  and  w  is input.
*     ------------------------------------------------------------------

*     Set  v  =  lastv-th column of  r  and find its norm.

      l      = (lastv - 1)*lastv/2
      call dcopy ( lastv, r(l+1), 1, v, 1 )
      vnorm  = dasum ( lastv, v, 1 )
      call m6rmod( n, nr, r, v, w, lastv,
     $             (vnorm*told), (vnorm*tolz), inform )

*     end of m6bswp
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6radd( maxr, nr, ns, r )

      implicit           double precision (a-h,o-z)
      double precision   r(nr)

*     ------------------------------------------------------------------
*     m6radd  adds column  ns  to the upper triangular matrix  r.
*     In this version (Sep 1984) it is just a unit vector.
*
*     Modified Jan 1983 to add a diagonal only, if  ns .gt. maxr.
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      if (ns .eq. 1) then
         r(1)      = one
      else if (ns .le. maxr) then
         ldiag1    = (ns - 1)*ns/2
         ldiag2    = ldiag1 + ns
         call dload ( ns, zero, r(ldiag1+1), 1 )
         r(ldiag2) = one
      else
         ldiag2    = maxr*(maxr + 1)/2  +  (ns - maxr)
         r(ldiag2) = one
      end if

*     end of m6radd
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6rcnd( maxr, nr, ns, r, dmax, dmin, cond )

      implicit           double precision (a-h,o-z)
      double precision   r(nr)

*     ------------------------------------------------------------------
*     m6rcnd  finds the largest and smallest diagonals of the
*     upper triangular matrix  r, and returns the square of their ratio.
*     This is a lower bound on the condition number of  r(t)*r.
*     ------------------------------------------------------------------

      intrinsic          abs, max, min

      ncolr  = min( maxr, ns )
      dmax   = abs( r(1) )
      dmin   = dmax

      if (ncolr .gt. 1) then
         l   = 1
         do 100 j = 2, ncolr
            l     = l + j
            d     = abs( r(l) )
            dmax  = max( dmax, d )
            dmin  = min( dmin, d )
  100    continue
      end if

      cond   = (dmax/dmin)**2

*     end of m6rcnd
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6rdel( m, maxr, nr, ns, ms,
     $                   kb, bbl, bbu, grd, r, rg, x, jq, rset )

      implicit           double precision (a-h,o-z)
      integer            kb(ms)
      double precision   bbl(ms), bbu(ms), grd(ms)
      double precision   r(nr), rg(ns), x(ms)
      logical            rset

*     ------------------------------------------------------------------
*     m6rdel  deletes the  jq-th  superbasic variable from  r
*     and from various arrays  kb, bbl, bbu, grd, rg, x.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy

      intrinsic          abs, max, min, sqrt
      parameter        ( zero = 0.0d+0 )

      if (jq .eq. ns) return
      if (.not. rset) go to 500

*     Delete the  jq-th  column of  r  and triangularize the
*     remaining columns using a partial forward sweep of rotations.

      ncolr  = min( maxr, ns )
      k      = jq*(jq + 1)/2

      do  50 i  = jq + 1, ncolr
         k      = k + i
         t1     = r(k-1)
         t2     = r(k)
         d      = sqrt( t1*t1 + t2*t2 )
         r(k-1) = d
         if (i .eq. ncolr) go to 20
         sn     = t2/d
         cs     = t1/d
         if (abs( cs ) .le. eps0) cs = zero
         j1     = i + 1
         k1     = k + i
         do  10 j   = j1, ncolr
            t1      = r(k1-1)
            t2      = r(k1)
            r(k1-1) = cs*t1 + sn*t2
            r(k1)   = sn*t1 - cs*t2
            k1      = k1 + j
   10    continue
   20    k1     = i - 1
         j2     = k - i
         j1     = j2 - i + 2
         do 30 j = j1, j2
            r(j) = r(j+k1)
   30    continue
   50 continue

*     If necessary, clear out the last column of  r.

      if (ns .gt. maxr) then
         lastr  = maxr*(maxr + 1)/2
         l      = lastr - maxr + 1
         if (jq .le. maxr) call dload ( maxr, zero, r(l), 1 )

*        Shift surplus diagonals of  r  to the left.

         j1     = max( maxr, jq )
         j2     = ns - 1
         l      = lastr + (j1 - maxr)
         do 320 j = j1, j2
            r(l)  = r(l+1)
  320    continue
      end if

*     Shift all the arrays one place to the left.

  500 j2     = ns - 1
      do 550 j  = jq, j2
         rg(j)  = rg(j+1)
         k      = m + j
         kb(k)  = kb(k+1)
         bbl(k) = bbl(k+1)
         bbu(k) = bbu(k+1)
         grd(k) = grd(k+1)
         x(k)   = x(k+1)
  550 continue

*     end of m6rdel
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6rmod( n, nr, r, v, w, lastv, told, tolz, inform )

      implicit           double precision (a-h,o-z)
      double precision   r(nr), v(n), w(n)

*     ------------------------------------------------------------------
*     m6rmod  modifies the upper-triangular matrix  r  so that
*     q*(r + v*w(t))  is upper triangular, where  q  is orthogonal,
*     v  and  w  are vectors, and the new  r  overwrites the old.
*
*     q  is the product of two sweeps of plane rotations (not stored).
*     These affect the  lastv-th  row of  r,  which is temporarily held
*     in the vector  v.  Thus,  v  is overwritten.  w  is not altered.
*
*     lastv  points to the last nonzero of  v.  The value lastv = n
*            would always be ok, but sometimes it is known to be less
*            than  n,  so  q  reduces to two partial sweeps of
*            rotations.
*
*     told   is a tolerance on the lastv-th diagonal of  r.
*     tolz   is a tolerance for negligible elements in  v.
*
*     On exit,
*     inform = 1  if the diagonal of  r  is larger than  told,
*            = 2  if not (in which case it is reset to  told).
*     ------------------------------------------------------------------

      intrinsic          sqrt
      parameter        ( zero = 0.0d+0 )

      vlast  = v(lastv)
      lm1    = lastv - 1
      lastr  = lastv*(lastv + 1)/2

*     Copy the lastv-th row of  r  into the end of  v.

      l      = lastr
      do 100 j = lastv, n
         v(j)  = r(l)
         l     = l + j
  100 continue

*     Reduce  v  to a multiple of  e(lastv)
*     using a partial backward sweep of rotations.
*     This fills in the  lastv-th  row of  r  (held in  v).

      v2     = vlast**2
      ldiag  = lastr

      do 300 ii = 1, lm1
         i      = lastv - ii
         ldiag  = ldiag - i - 1
         s      = v(i)
         v(i)   = zero
         if (abs(s) .le. tolz) go to 300
         v2     = s**2 + v2
         root   = sqrt(v2)
         cs     = vlast/root
         sn     = s/root
         vlast  = root
         l      = ldiag

         do 200 j = i, n
            s     = v(j)
            t     = r(l)
            v(j)  = cs*s + sn*t
            r(l)  = sn*s - cs*t
            l     = l + j
  200    continue
  300 continue

*     Set  v  =  v  +  vlast*w.

      call daxpy ( n, vlast, w, 1, v, 1 )

*     Eliminate the front of the  lastv-th  row of  r  (held in  v)
*     using a partial forward sweep of rotations.

      ldiag  = 0

      do 700 i  = 1, lm1
         ldiag  = ldiag + i
         t      = v(i)
         if (abs(t) .le. tolz) go to 700
         s      = r(ldiag)
         root   = sqrt(s**2 + t**2)
         cs     = s/root
         sn     = t/root
         r(ldiag) = root
         l      = ldiag + i

         do 600 j = i + 1, n
            s     = r(l)
            t     = v(j)
            r(l)  = cs*s + sn*t
            v(j)  = sn*s - cs*t
            l     = l + j
  600    continue
  700 continue

*     Insert the new lastv-th row of  r.

      l      = lastr
      do 900 j = lastv, n
         r(l)  = v(j)
         l     = l + j
  900 continue

*     Test for (unlikely) singularity.

      inform = 1
      if (abs( r(lastr) ) .le. told) then
         inform   = 2
         r(lastr) = told
      end if

*     end of m6rmod
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6rset( maxr, nr, ns, r, cond )

      implicit           double precision (a-h,o-z)
      double precision   r(nr)

*     ------------------------------------------------------------------
*     m6rset  alters  r,  the upper-triangular factor of the
*     approximation to the reduced hessian.
*
*     If  r(1) = zero,  r  does not exist.
*     In this case,  r  is initialized to the identity matrix.
*
*     Otherwise,  r  already exists and we attempt to make it better
*     conditioned by scaling its columns by the square roots of its
*     current diagonals.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      intrinsic          abs, max, min, sqrt
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      cond   = one
      ncolr  = min( maxr, ns )
      if (ncolr .eq.  0  ) return

      dmax   = abs( r(1) )
      if (dmax  .eq. zero) then

*        Set  r = the identity.

         l      = ncolr*(ncolr + 1)/2
         call dload ( l, zero, r, 1 )
         ldiag  = 0

         do 100 k = 1, ncolr
            ldiag    = ldiag + k
            r(ldiag) = one
  100    continue

         do 120 k = maxr + 1, ns
            ldiag    = ldiag + 1
            r(ldiag) = one
  120    continue
      else

*        Scale the columns of  r.

         dmin   = dmax
         ldiag  = 0

         do 250 k = 1, ncolr
            l     = ldiag + 1
            ldiag = ldiag + k
            diag  = abs( r(ldiag) )
            dmax  = max( dmax, diag )
            dmin  = min( dmin, diag )
            s     = one / sqrt( diag )
            do 240 i = l, ldiag
               r(i)  =  r(i)*s
  240       continue
  250    continue

         cond   = dmax / dmin
         if (iprint .gt. 0) write(iprint, 2000) dmax, dmin, cond
      end if
      return

 2000 format(/ ' XXX  Hessian modified.    Diags =', 1p, 2e11.2,
     $         ',    Cond R(t)*R =', e11.2)

*     end of m6rset
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6rsol( mode, maxr, nr, ns, r, y )

      implicit           double precision (a-h,o-z)
      double precision   r(nr), y(ns)

*     ------------------------------------------------------------------
*     m6rsol  solves  R*x = y  or  R(t)*x = y,  where  R  is an
*     upper-triangular matrix stored by columns in  r(nr).
*     The solution  x  overwrites  y.
*     ------------------------------------------------------------------

      intrinsic          min

      ncolr  = min( maxr, ns )

      if (mode .eq. 1) then

*        mode = 1  --  solve  R*y = y.

         l    = ncolr*(ncolr + 1)/2
         do 120 i  = ncolr, 2, -1
            t      = y(i) / r(l)
            y(i)   = t
            l      = l - i
            call daxpy ( i-1, (-t), r(l+1), 1, y, 1 )
  120    continue
         y(1) = y(1) / r(1)
      else

*        mode = 2  --  solve  R(t)*y = y.

         y(1) = y(1) / r(1)
         l    = 1
         do 220 i = 2, ncolr
            t     = ddot  ( i-1, r(l+1), 1, y, 1 )
            l     = l + i
            y(i)  = (y(i) - t) / r(l)
  220    continue
      end if

*     Deal with surplus diagonals of  r.

      if (ns .gt. maxr) then
         l     = maxr*i/2
         do 320 j = maxr + 1, ns
            l     = l + 1
            y(j)  = y(j) / r(l)
  320    continue
      end if

*     end of m6rsol
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m6swap( m, maxr, nr, ns, ms,
     $                   kb, bbl, bbu, grd, r, rg, x )

      implicit           double precision (a-h,o-z)
      integer            kb(ms)
      double precision   bbl(ms), bbu(ms), grd(ms)
      double precision   r(nr), rg(ns), x(ms)

*     ------------------------------------------------------------------
*     m6swap  (superbasic swap)  finds the largest reduced gradient in
*     the range  rg(maxr+1), ..., rg(ns)  and swaps it into position
*     maxr + 1  (so we know where it is).
*     ------------------------------------------------------------------

      k1     = maxr + 1
      if (ns .gt. k1) then
         nz2    = ns - maxr
         k2     = maxr + idamax( nz2, rg(k1), 1 )
         if (k2 .gt. k1) then
            j      = m + k1
            k      = m + k2
            lastr  = maxr*k1/2
            ldiag1 = lastr + 1
            ldiag2 = lastr + (k2 - maxr)

            rdiag1    = r(ldiag1)
            rg1       = rg(k1)
            j1        = kb(j)
            bl1       = bbl(j)
            bu1       = bbu(j)
            grd1      = grd(j)
            x1        = x(j)

            r(ldiag1) = r(ldiag2)
            rg(k1)    = rg(k2)
            kb(j)     = kb(k)
            bbl(j)    = bbl(k)
            bbu(j)    = bbu(k)
            grd(j)    = grd(k)
            x(j)      = x(k)

            r(ldiag2) = rdiag1
            rg(k2)    = rg1
            kb(k)     = j1
            bbl(k)    = bl1
            bbu(k)    = bu1
            grd(k)    = grd1
            x(k)      = x1
         end if
      end if

*     end of m6swap
      end
