c********************************************************************\
c minos C interface
c ASCEND
c (C) Ben Allan, August 9, 1994
c $Revision: 1.1.1.1 $
c $Date: 1996/04/30 16:34:15 $
c
c MINOS 5.4 is proprietary software sitelicensed to Carnegie Mellon.
c Others who wish to use minos with ASCEND must get their own license
c and MINOS 5.4 sources. We provide only interface code to feed problems
c to MINOS 5.4.
c********************************************************************/


c********************************************************************\
c Function to return values of various common block values.
c  major: number of major iterations by minos (int)
c  minor: number of minor iterations by minos (int)
c********************************************************************/
      subroutine get_minos_common(major,minor)

      implicit           double precision (a-h,o-z)
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      integer major, minor
      major= majits
      minor= minits

      return
      end

*********************************************************************
* The following utilities are also used by minos.
* They are defined in mi15blas.f in normal minos installations.
*
*     dddiv    ddscl    dload    dnorm1
*     hcopy    hload    icopy    iload    iload1
*
*     These could be tuned to the machine being used.
*     dload  is used the most.
*
*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


      subroutine dddiv ( n, d, incd, x, incx )

      implicit           double precision (a-h,o-z)
      double precision   d(*), x(*)

*     dddiv  performs the diagonal scaling  x  =  x / d.

      integer            i, id, ix
      external           dscal
      intrinsic          abs
      parameter        ( one = 1.0d+0 )

      if (n .gt. 0) then
         if (incd .eq. 0  .and.  incx .ne. 0) then
            call dscal ( n, one/d(1), x, abs(incx) )
         else if (incd .eq. incx  .and.  incd .gt. 0) then
            do 10 id = 1, 1 + (n - 1)*incd, incd
               x(id) = x(id) / d(id)
   10       continue
         else
            if (incx .ge. 0) then
               ix = 1
            else
               ix = 1 - (n - 1)*incx
            end if
            if (incd .gt. 0) then
               do 20 id = 1, 1 + (n - 1)*incd, incd
                  x(ix) = x(ix) / d(id)
                  ix    = ix   + incx
   20          continue
            else
               id = 1 - (n - 1)*incd
               do 30  i = 1, n
                  x(ix) = x(ix) / d(id)
                  id    = id + incd
                  ix    = ix + incx
   30          continue
            end if
         end if
      end if

*     end of dddiv
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine ddscl ( n, d, incd, x, incx )

      integer            incd, incx, n
      double precision   d(*), x(*)

*     ddscl  performs the diagonal scaling  x  =  d * x.

      integer            i, id, ix
      external           dscal
      intrinsic          abs

      if (n .gt. 0) then
         if (incd .eq. 0  .and.  incx .ne. 0) then
            call dscal ( n, d(1), x, abs(incx) )
         else if (incd .eq. incx  .and.  incd .gt. 0) then
            do 10 id = 1, 1 + (n - 1)*incd, incd
               x(id) = d(id)*x(id)
   10       continue
         else
            if (incx .ge. 0) then
               ix = 1
            else
               ix = 1 - (n - 1)*incx
            end if
            if (incd .gt. 0) then
               do 20 id = 1, 1 + (n - 1)*incd, incd
                  x(ix) = d(id)*x(ix)
                  ix    = ix + incx
   20          continue
            else
               id = 1 - (n - 1)*incd
               do 30  i = 1, n
                  x(ix) = d(id)*x(ix)
                  id    = id + incd
                  ix    = ix + incx
   30          continue
            end if
         end if
      end if

*     end of ddscl
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine dload ( n, const, x, incx )

      double precision   const
      integer            incx, n
      double precision   x(*)

*     dload loads elements of x with const.

      double precision   zero
      parameter        ( zero = 0.0d+0 )
      integer            ix

      if (n .gt. 0) then
         if (incx .eq. 1  .and.  const .eq. zero) then
            do 10 ix = 1, n
               x(ix) = zero
   10       continue
         else
            do 20 ix = 1, 1 + (n - 1)*incx, incx
               x(ix) = const
   20       continue
         end if
      end if

*     end of dload
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      function   dnorm1( n, x, incx )

      implicit           double precision (a-h,o-z)
      double precision   x(*)

*     dnorm1  returns the 1-norm of the vector  x,  scaled by root(n).
*     This approximates an "average" element of x with some allowance
*     for x being sparse.

      intrinsic          sqrt
      external           dasum

      d      = n
      d      = dasum ( n, x, incx ) / sqrt(d)
      dnorm1 = d

*     end of dnorm1
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine hcopy ( n, hx, incx, hy, incy )

      integer*4          hx(*), hy(*)
      integer            incx, incy

*     hcopy  is the half-integer version of dcopy.
*     In this version of MINOS we no longer use half integers.

      integer            ix, iy

      if (n .gt. 0) then
         if (incx .eq. incy  .and.  incy .gt. 0) then
            do 10 iy  = 1, 1 + (n - 1)*incy, incy
               hy(iy) = hx(iy)
   10       continue
         else
            if (incx .ge. 0) then
               ix = 1
            else
               ix = 1 - (n - 1)*incx
            end if
            if (incy .gt. 0) then
               do 20 iy  = 1, 1 + ( n - 1 )*incy, incy
                  hy(iy) = hx(ix)
                  ix     = ix + incx
   20          continue
            else
               iy = 1 - (n - 1)*incy
               do 30  i  = 1, n
                  hy(iy) = hx(ix)
                  iy    = iy + incy
                  ix    = ix + incx
   30          continue
            end if
         end if
      end if

*     end of hcopy
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine hload ( n, const, hx, incx )

      integer            incx, n
      integer            const
      integer*4          hx(*)

*     hload loads elements of hx with const.
*     Beware that const is INTEGER, not half integer.

      integer            ix

      if (n .gt. 0) then
         if (incx .eq. 1  .and.  const .eq. 0) then
            do 10 ix  = 1, n
               hx(ix) = 0
   10       continue
         else
            do 20 ix  = 1, 1 + (n - 1)*incx, incx
               hx(ix) = const
   20       continue
         end if
      end if

*     end of hload
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine icopy ( n, x, incx, y, incy )

      integer            x(*), y(*)
      integer            incx, incy

*     icopy  is the integer version of dcopy.

      integer            ix, iy

      if (n .gt. 0) then
         if (incx .eq. incy  .and.  incy .gt. 0) then
            do 10 iy = 1, 1 + (n - 1)*incy, incy
               y(iy) = x(iy)
   10       continue
         else
            if (incx .ge. 0) then
               ix = 1
            else
               ix = 1 - (n - 1)*incx
            end if
            if (incy .gt. 0) then
               do 20 iy = 1, 1 + ( n - 1 )*incy, incy
                  y(iy) = x(ix)
                  ix    = ix + incx
   20          continue
            else
               iy = 1 - (n - 1)*incy
               do 30 i  = 1, n
                  y(iy) = x(ix)
                  iy    = iy + incy
                  ix    = ix + incx
   30          continue
            end if
         end if
      end if

*     end of icopy
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine iload ( n, const, x, incx )

      integer            incx, n
      integer            const
      integer            x(*)

*     iload  loads elements of x with const.

      integer            ix

      if (n .gt. 0) then
         if (incx .eq. 1  .and.  const .eq. 0) then
            do 10 ix = 1, n
               x(ix) = 0
   10       continue
         else
            do 20 ix = 1, 1 + (n - 1)*incx, incx
               x(ix) = const
   20       continue
         end if
      end if

*     end of iload
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine iload1( n, const, x, incx )

      integer            incx, n
      integer            const
      integer            x(*)

*     iload1 loads elements of x with const, by calling iload.
*     iload1 is needed in MINOS because iload  is a file number.

      call iload ( n, const, x, incx )

*     end of iload1
      end
