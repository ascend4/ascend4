************************************************************************
*
*     file  mi50lp   fortran.
*
*     m5bsx    m5chzr   m5dgen   m5frmc   m5hs     m5log    m5lpit
*     m5pric   m5rc     m5setp   m5setx   m5solv
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5bsx ( mode, ms, nb, kb, x, xn )

      implicit           double precision (a-h,o-z)
      integer            kb(ms)
      double precision   x(ms), xn(nb)

*     ------------------------------------------------------------------
*     m5bsx   copies basic and superbasic components from  x  into  xn
*     or vice versa, depending on whether  mode = 1 or 2.
*     ------------------------------------------------------------------

      if (mode .eq. 1) then
         do 10  k = 1, ms
            j     = kb(k)
            xn(j) = x(k)
   10    continue
      else
         do 30  k = 1, ms
            j     = kb(k)
            x(k)  = xn(j)
   30    continue
      end if

*     end of m5bsx
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5chzr( ms    , stepmx, plinfy, tolpiv,
     $                   hrtype, bbl   , bbu   , x     , y,
     $                   hitlow, move  , onbnd , unbndd,
     $                   jp    , bound , exact , alpha )

      implicit           double precision (a-h,o-z)
      integer*4          hrtype(ms)
      double precision   bbl(ms), bbu(ms), x(ms), y(ms)
      logical            hitlow , move   , onbnd, unbndd

*     ------------------------------------------------------------------
*     m5chzr  finds a step  alpha  such that the point  x + alpha*y
*     reaches one of the bounds on  x.
*
*     In this version of chuzr, when x is infeasible, the number of
*     infeasibilities will never increase.  If the number stays the
*     same, the sum of infeasibilities will decrease.
*     If the number decreases by one or more,
*     the sum of infeasibilities will usually decrease also, but
*     occasionally it will increase after the step  alpha  is taken.
*     (Convergence is still assured because the number has decreased.)
*
*     Two possible steps are computed as follows:
*
*     alphaf = the maximum step that can be taken without violating
*              one of the bounds that are currently satisfied.
*
*     alphai = the maximum (nonzero) step that has the property of
*              reaching a bound that is currently violated,
*              subject to the pivot being reasonably close to the
*              maximum pivot among infeasible variables.
*              (alphai is not defined if x is feasible.)
*
*     alphai is needed occasionally when infeasible, to prevent
*     going unnecessarily far when alphaf is quite large.  It will
*     always come into effect when x is about to become feasible.
*     The sum of infeasibilities will decrease initially as alpha
*     increases from zero, but may start increasing for larger steps.
*     choosing a large alphai allows several elements of  x  to
*     become feasible at the same time.
*
*     In the end, we take  alpha = alphaf  if x is feasible, or if
*     alphai > alphap (where alphap is the perturbed step from pass 1).
*     Otherwise,  we take  alpha = alphai.
*
*     Input parameters
*     ----------------
*     ms     is  m + 1  for m5lpit,  m + ns  for m7rgit.
*     stepmx defines what should be treated as an unbounded step.
*     plinfy provides insurance for detecting unboundedness.
*            if alpha reaches a bound as large as plinfy, it is
*            classed as an unbounded step.
*     tolpiv is a tolerance to exclude negligible elements of y.
*     featol (in common) is the current feasibility tolerance used by
*            m5frmc.  typically in the range 0.5*tolx to 0.99*tolx,
*            where tolx is the featol specified by the user.
*     tolinc (in common) is used to determine stepmn (see below),
*            the minimum positive step.
*     hrtype is set by  m5frmc  as follows:
*            hrtype(j) = -2  if x(j) .lt. bl(j) - featol
*                      =  0  if x(j)  is feasible
*                      = +2  if x(j) .gt. bu(j) + featol
*     bbl    the lower bounds on the basic and superbasic variables.
*     bbu    the upper bounds on ditto.
*     x      the values of       ditto.
*     y      the search direction.
*
*
*     Output parameters
*     -----------------
*     hitlow  = true  if a lower bound restricted alpha.
*             = false otherwise.
*     move    = true  if exact ge stepmn (defined at end of code).
*     onbnd   = true  if alpha =  exact.
*                     this means that the step alpha moves x(jp)
*                     exactly onto one of its bounds, namely bound.
*             = false if the exact step would be too small
*                     ( exact lt stepmn ).
*               (with these definitions,  move = onbnd).
*     unbndd  = true  if alpha = stepmx.  jp may possibly be zero.
*               the parameters hitlow, move, onbnd, bound and exact
*               should not be used.
*     jp      = the index (if any) such that x(jp) reaches a bound.
*     bound   = the bound value bbl(jp) or bbu(jp) corresponding
*               to hitlow.
*     exact   = the step that would take x(jp) exactly onto bound.
*     alpha   = an allowable, positive step.
*               if unbndd is true,  alpha = stepmx.
*               otherwise,          alpha = max( stepmn, exact ).
*
*
*
*
*     Original version written November 1981.
*     The two-pass approach used follows Paula Harris (1973).
*
*     September 1986:  Modified to deal with x + alpha*y only, and to
*     return a few extra parameters.
*
*     20 aug 1987:  EXPAND procedure implemented to deal with
*     degeneracy in a more rigorous way.  The step alphaf is chosen
*     the same way as Harris, but since this may be negative, this
*     version insists on returning a positive step, alpha.
*     Two features make this possible:
*
*        1. featol increases slightly each iteration.
*
*        2. The blocking variable, when made nonbasic, is allowed to
*           retain the value x(jp) + alpha * y(jp), even if this is
*           not exactly on the blocking bound.
*
*     15 feb 1988:  Modified to prevent a very small pivot being
*     selected in connection with alphai.  For infeasible variables
*     moving towards their bound, we now require the chosen pivot
*     to be at least gamma times as large as the biggest available.
*     This still gives us freedom in pass 2.
*     gamma = 0.1 and 0.01 seemed to inhibit phase 1 somewhat.
*     gamma = 0.001 seems to be safe.
*
*     Note: if the weight on the objective is positive, there is still a
*     danger of small pivots in phase 1.  We have to let an "unbounded"
*     exit occur.  m5solv will set wtobj = zero and try again.
*     ------------------------------------------------------------------

      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)

      intrinsic          abs, max
      logical            blockf, blocki

      parameter        ( zero = 0.0d+00,  gamma = 0.001d+00 )

*     ------------------------------------------------------------------
*     First pass.
*     For feasible variables, find the step alphap that reaches the
*     nearest perturbed (expanded) bound.  alphap will be slight larger
*     than the step to the nearest true bound.
*     For infeasible variables, find the maximum pivot pivmxi.
*     ------------------------------------------------------------------

      delta  = featol
      alphap = stepmx
      pivmxi = zero

      do 200 j  = 1, ms
         pivot  = y(j)
         pivabs = abs( pivot )
         if (pivabs .le. tolpiv) go to 200
         jtype  = hrtype(j)
         if (pivot  .gt. zero  ) go to 150

*        x  is decreasing.
*        Test for smaller alphap if lower bound is satisfied.

         if (jtype .lt. 0) go to 200
         res    = x(j) - bbl(j) + delta
         if (alphap*pivabs .gt. res) alphap = res / pivabs

*        Test for bigger pivot if upper bound is violated.

         if (jtype .gt. 0) pivmxi = max( pivmxi, pivabs )
         go to 200

*        x  is increasing.
*        Test for smaller alphap if upper bound is satisfied.

  150    if (jtype .gt. 0) go to 200
         res    = bbu(j) - x(j) + delta
         if (alphap*pivabs .gt. res) alphap = res / pivabs

*        Test for bigger pivot if lower bound is violated.

         if (jtype .lt. 0) pivmxi = max( pivmxi, pivabs )
  200 continue

*     ------------------------------------------------------------------
*     Second pass.
*     For feasible variables, recompute steps without perturbation.
*     Choose the largest pivot element subject to the step being
*     no greater than alphap.
*     For infeasible variables, find the largest step subject to the
*     pivot element being no smaller than gamma * pivmxi.
*     ------------------------------------------------------------------

      alphai = zero
      pivmxf = zero
      pivmxi = gamma * pivmxi
      jhitf  = 0
      jhiti  = 0

      do 400 j  = 1, ms
         pivot  = y(j)
         pivabs = abs( pivot )
         if (pivabs .le. tolpiv) go to 400
         jtype  = hrtype(j)
         if (pivot  .gt. zero  ) go to 350

*        x  is decreasing.
*        Test for bigger pivot if lower bound is satisfied.

         if (jtype    .lt.     0   ) go to 400
         if (pivabs   .le.   pivmxf) go to 340
         res    = x(j) - bbl(j)
         if (alphap*pivabs .lt. res) go to 340
         pivmxf = pivabs
         jhitf  = j

*        Test for bigger alphai if upper bound is violated.

  340    if (jtype    .eq.     0   ) go to 400
         if (pivabs   .lt.   pivmxi) go to 400
         res    = x(j) - bbu(j)
         if (alphai*pivabs .ge. res) go to 400
         alphai = res / pivabs
         jhiti  = j
         go to 400

*        x  is increasing.
*        Test for bigger pivot if upper bound is satisfied.

  350    if (jtype    .gt.     0   ) go to 400
         if (pivabs   .le.   pivmxf) go to 360
         res    = bbu(j) - x(j)
         if (alphap*pivabs .lt. res) go to 360
         pivmxf = pivabs
         jhitf  = j

*        Test for bigger alphai if lower bound is violated.

  360    if (jtype    .eq.     0   ) go to 400
         if (pivabs   .lt.   pivmxi) go to 400
         res    = bbl(j) - x(j)
         if (alphai*pivabs .ge. res) go to 400
         alphai = res / pivabs
         jhiti  = j
  400 continue

*     ------------------------------------------------------------------
*     See if a feasible and/or infeasible variable blocks.
*     ------------------------------------------------------------------
      blockf = jhitf .gt. 0
      blocki = jhiti .gt. 0
      unbndd = .not. ( blockf  .or.  blocki )
      if (       unbndd ) go to 900
      if ( .not. blockf ) go to 500

*     ------------------------------------------------------------------
*     A variable hits a bound for which it is currently feasible.
*     the corresponding step alphaf is not used, so no need to get it,
*     but we know that alphaf .le. alphap, the step from pass 1.
*     ------------------------------------------------------------------
      jp     = jhitf
      pivot  = y(jp)
      hitlow = pivot .lt. zero

*     If there is a choice between alphaf and alphai, it is probably
*     best to take alphai (so we can kick the infeasible variable jhiti
*     out of the basis).
*     However, we can't if alphai is bigger than alphap.

      if (   .not. blocki   ) go to 600
      if (alphai .gt. alphap) go to 600

*     ------------------------------------------------------------------
*     An infeasible variable reaches its violated bound.
*     ------------------------------------------------------------------
  500 jp     = jhiti
      pivot  = y(jp)
      hitlow = pivot .gt. zero

*     ------------------------------------------------------------------
*     Try to step exactly onto bound, but make sure the exact step
*     is sufficiently positive.  (exact will be alphaf or alphai.)
*     Since featol increases by tolinc each iteration, we know that
*     a step as large as stepmn (below) will not cause any feasible
*     variables to become infeasible (where feasibility is measured
*     by the current featol).
*     ------------------------------------------------------------------
  600 if ( hitlow ) then
         bound = bbl(jp)
      else
         bound = bbu(jp)
      end if
      unbndd = abs( bound ) .ge. plinfy
      if ( unbndd ) go to 900

      stepmn = tolinc / abs( pivot )
      exact  = (bound - x(jp)) / pivot
      alpha  = max( stepmn, exact )
      onbnd  = alpha .eq. exact
      move   = exact .ge. stepmn
      if (.not. move) ndegen = ndegen + 1
      return

*     ------------------------------------------------------------------
*     Unbounded.
*     ------------------------------------------------------------------
  900 alpha  = stepmx
      move   = .true.
      onbnd  = .false.

*     end of m5chzr
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5dgen( mode, m, n, nb, ms, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, bl, bu, x, xn, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), kb(ms)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   x(ms), xn(nb), y(m), y2(m), z(nwcore)

*     ------------------------------------------------------------------
*     m5dgen performs most of the manoeuvres associated with degeneracy.
*     The degeneracy-resolving strategy operates in the following way.
*
*     Over a cycle of iterations, the feasibility tolerance featol
*     increases slightly (from tolx0 to tolx1 in steps of tolinc).
*     This ensures that all steps taken will be positive.
*
*     After kdegen consecutive iterations, nonbasic variables within
*     featol of their bounds are set exactly on their bounds and the
*     basic variables are recomputed to satisfy ax = b.
*     featol is then reduced to tolx0 for the next cycle of iterations.
*
*
*     If mode = 1, m5dgen initializes the parameters in
*     common block m5step:
*
*     featol  is the current feasibility tolerance.
*     tolx0   is the minimum feasibility tolerance.
*     tolx1   is the maximum feasibility tolerance.
*     tolinc  is the increment to featol.
*     kdegen  is the expand frequency (specified by the user).
*             it is the frequency of resetting featol to tolx0.
*     ndegen  counts the number of degenerate steps (incremented
*             by m5chzr).
*     itnfix  is the last iteration at which a mode 2 or 3 entry
*             set nonbasics onto their bound.
*     nfix(j) counts the number of times a mode 3 entry has
*             set nonbasics onto their bound,
*             where j=1 if infeasible, j=2 if feasible.
*
*     tolx0 and tolx1 are both close to the feasibility tolerance tolx
*     specified by the user.  (They must both be less than tolx.)
*
*
*     If mode = 2,  m5dgen has been called after a cycle of kdegen
*     iterations.  Nonbasic xn(j)s are examined to see if any are
*     outside their bounds. (It will never be as much as featol).
*     inform returns how many.  Deviations as small as tolz
*     (e.g. 1.0d-11) are not counted.
*     If inform is positive, the basic variables are recomputed.
*     It is assumed that m5solv will then continue iterations.
*
*
*     If mode = 3,  m5dgen is being called after a subproblem has been
*     judged optimal, infeasible or unbounded.  Nonbasic xn(j)s are
*     examined as above.
*
*     First version: August 1987.
*     22 Sep 1987: itnfix, nfix(j) and maxfix introduced to allow
*                  more than one mode 3 call at the end of a run.
*     14 Oct 1991: a, ha, ka passed in to help with minoss.
*     08 Apr 1992: Internal values of hs(*) now used.
*     04 Jun 1992: mode 2 and 3 fixed to allow for hs(j) = 4.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m8len / njac  ,nncon ,nncon0,nnjac
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0 )

      inform = 0
      if (mode .eq. 1) then

*        mode = 1.
*        Initialize at the start of each major iteration.
*        kdegen is the expand frequency      and
*        tolx   is the feasibility tolerance
*        (specified by the user).  They are not changed.

         ndegen  = 0
         itnfix  = 0
         nfix(1) = 0
         nfix(2) = 0
         tolx0   = tolx  * 0.5
         tolx1   = tolx  * 0.99
         if (kdegen .lt. 99999999) then
            tolinc = (tolx1 - tolx0) / kdegen
         else
            tolinc = 0.0
         end if
         featol  = tolx0
      else

*        mode = 2 or 3.
*        Initialize local variables maxfix and tolz.

         maxfix = 2
         tolz   = eps1
         if (mode .eq. 3) then

*           mode = 3.
*           Return with inform = 0 if the last call was at the
*           same itn, or if there have already been maxfix calls
*           with the same state of feasibility.

            if (itnfix .eq. itn   ) return
            if (ninf   .gt.   0   ) j = 1
            if (ninf   .eq.   0   ) j = 2
            if (nfix(j).ge. maxfix) return
            nfix(j) = nfix(j) + 1
         end if

*        Set nonbasics on their nearest bound if they are within
*        the current featol of that bound.  hs(j) = 0, 1 or 4.
*        To avoid go to's we test on diff for all hs(j).

         itnfix = itn

         do 250 j = 1, nb
            diff  = zero
            js    = hs(j)

            if (js .eq. 0) then
               bnd   = bl(j)
               diff  = bnd - xn(j)
            else if (js .eq. 1  .or.  js .eq. 4) then
               bnd   = bu(j)
               diff  = xn(j) - bnd
            end if

            if (diff .gt. zero) then
               if (diff .gt. tolz) inform = inform + 1
               xn(j) = bnd
            end if
  250    continue

*        Reset featol to its minimum value.

         featol = tolx0
         if (inform .gt. 0) then

*           Reset the basic variables.
*           Set ninf positive to make sure m5frmc tests for feasibility.

            call m5setx( 1, m, n, nb, ms, kb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, x, xn, y, y2, z, nwcore )
            ninf   = 1
            if (prnt1  .or.  nncon .eq. 0) then
               if (iprint .gt. 0) write(iprint, 1000) itn, inform
               if (isumm  .gt. 0) write(isumm , 1000) itn, inform
            end if
         end if
      end if

      return

 1000 format(' Itn', i7, ' --', i7,
     $       '  nonbasics set on bound, basics recomputed')

*     end of m5dgen
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5frmc( n, nb, nn, ns, ms, maxs,
     $                   lcrash, first, fsub, featol, objadd,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, bbl, bbu, hrtype, hs, kb,
     $                   grd2, x, xn, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            first
      integer*4          ha(ne), hrtype(ms), hs(nb)
      integer            ka(nka), kb(ms)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   bbl(ms), bbu(ms),
     $                   grd2(ms), x(ms), xn(nb), z(nwcore)

*     ------------------------------------------------------------------
*     m5frmc  sets up a vector in  grd2  to be used to compute  pi.
*     It also defines  hrtype  to be used in  m5chzr.
*     After  m2bfac,  or at a newly feasible point, the subproblem
*     objective function is evaluated.
*
*     lcrash is used at a first feasible point to print a suitable msg.
*
*     10 Apr 1992: objadd added as an input parameter.
*     04 Jun 1992: lcrash added as an input parameter.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lp2 / invrq,invitn,invmod
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2

      logical            kludge, feasbl, infsbl,
     $                   linear, nonlin, lincon, nlncon

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     ------------------------------------------------------------------

      feasbl = ninf .eq. 0
      linear = nn   .eq. 0
      if ( first ) feasbl = .false.
      infsbl = .not. feasbl
      nonlin = .not. linear

      call dload ( ms, zero, grd2, 1 )
      if (iobj .ne. 0) grd2(iobj) = - minimz * sclobj

*     Revert to the simplex method if the number of superbasics
*     has just decreased to zero.
*     Note that it is ok to do phase 1 simplex on nonlinear problems
*     as long as we are at a vertex.

      if (ns .eq. 0) then
         if ( infsbl ) then
            nphs = 1
         else if ( linear ) then
            nphs = 2
         end if
      end if

*     Exit if the previous iteration was feasible and the
*     basis has not just been refactorized.

      if (feasbl  .and.  invitn .gt. 0) return

*     ------------------------------------------------------------------
*     Find the current number and sum of infeasibilities.
*     ------------------------------------------------------------------
      lincon = nncon  .eq. 0
      nlncon = nncon  .gt. 0
      numinf = 0
      suminf = zero

      do 100 k = 1, ms
         hrtype(k) = 0
         xk        = x(k)
         res       = bbl(k) - xk
         if (res .le. featol) go to 50
         grd2(k)   = - one
         hrtype(k) = - 2
         go to 60

   50    res       = xk - bbu(k)
         if (res .le. featol) go to 100
         grd2(k)   = one
         hrtype(k) = 2

   60    numinf    = numinf + 1
         suminf    = suminf + res
  100 continue

      sinf   = suminf
      ninf   = numinf

      if (numinf .gt. 0) then
*        ---------------------------------------------------------------
*        Infeasible.
*        ---------------------------------------------------------------
                             
*        If first iteration, set  nphs.
      
         if (first) then
            nphs   = 1
            if (ns .gt. 0) nphs = 4
         end if
      
*        Set  grd2(iobj)  to allow for a composite objective.
      
         if (nphs .eq. 2) nphs = 1
         kmodpi = 1
         fsub   = sinf
         fobj   = zero
         if (iobj .ne. 0) grd2(iobj) = - minimz * wtobj * sclobj
      
*        Print something if the basis has just been refactorized.
      
         if (prnt1  .and.  invitn .eq. 0)
     $       write(iprint, 1010) itn, numinf, suminf

      else
*        ---------------------------------------------------------------
*        Feasible.
*        ---------------------------------------------------------------
           
*        Reset  nphs  if the previous itn was infeasible.
      
         if (first .or. infsbl) then
            nphs   = 2
            if ( nonlin  ) nphs = 3
            if (ns .gt. 0) nphs = 4
         end if
      
*        Set up the feasible objective value.
      
         kmodpi = 1
         flin   = zero
         if (iobj .ne. 0) flin = - x(iobj) * sclobj
         fsub   = minimz * flin

         if (lcrash .eq. 3) then

*           Only the linear equalities (E rows) have been satisfied.
*           We don't want to evaluate the functions yet.
*           Print something and quit.

            if (iprint .gt. 0) then
               if (prnt1 .or. lincon) then
                  write(iprint, 1000)
                  write(iprint, 1015) itn, fsub
               end if
            end if
            if (isumm  .gt. 0) then
               if (summ1 .or. lincon) write(isumm , 1015) itn, fsub
            end if
            return
         end if

         if ( nonlin ) then
            modefg = 2
            call m6fun ( 0, modefg, n, nb, ms, fsub,
     $                   ne, nka, a, ha, ka,
     $                   x, xn, z, nwcore )
            call m6fun ( 1, modefg, n, nb, ms, fsub,
     $                   ne, nka, a, ha, ka,
     $                   x, xn, z, nwcore )
            if (ierr .ne. 0) return
         end if
      
         wtobj  = zero
         obj    = minimz * fsub  +  objadd
         objtru = flin   + fobj  +  objadd
      
*        Print something unless Print level = 0 and there are
*        nonlinear constraints.
      
         if (iprint .gt. 0) then
            if (prnt1  .or.  (lincon  .and.  infsbl)) then
               if (infsbl) write(iprint, 1000)
               if (lincon) write(iprint, 1020) itn, obj
               if (nlncon) write(iprint, 1030) itn, objtru, obj
               if (infsbl) write(iprint, 1000)
            end if
         end if
      
         if (isumm  .gt. 0) then
            if (infsbl) then
               if (lincon            ) write(isumm , 1020) itn, obj
               if (nlncon .and. summ1) write(isumm , 1040) itn, obj
            end if
         end if
      end if

      return

 1000 format(' ')
 1010 format(' Itn', i7, ' -- infeasible.  Num =', i5,
     $   '   Sum =', 1p, e18.9)
 1015 format(' Itn', i7, ' -- linear E rows feasible.   Obj =',
     $   1p, e18.9)
 1020 format(' Itn', i7, ' -- feasible solution.  Objective =',
     $   1p, e18.9)
 1030 format(' Itn', i7, ' -- feasible subproblem.  True obj =',
     $   1p, e17.9, 4x, ' Auglag obj =', e17.9)
 1040 format(' Itn', i7, ' -- feasible subproblem.',
     $   1p,        4x, ' Auglag obj =', e17.9)

*     end of m5frmc
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5hs  ( mode, nb, bl, bu, hs, xn )

      implicit           double precision (a-h,o-z)
      character*8        mode
      double precision   bl(nb), bu(nb)
      integer*4          hs(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m5hs   is called from m5solv and m8setj.
*     if mode = 'Internal', m5hs sets hs(j) = -1 or 4 for certain
*        nonbasic variables.  This allows m5pric to operate more
*        efficiently.  The internal values of hs are now as follows:
*
*        hs(j) = -1  Nonbasic between bounds (bl     <  x <  bu    )
*        hs(j) =  0  Nonbasic on lower bound (bl-tol <  x <= bl    )
*        hs(j) =  1  Nonbasic on upper bound (bu     <= x <  bu+tol)
*        hs(j) =  2  Superbasic
*        hs(j) =  3  Basic
*        hs(j) =  4  Nonbasic and fixed      (bl     = x  =  bu    )
*
*        where 0 <= tol < the feasibility tolerance.
*
*     if mode = 'External', m5hs changes -1 or 4 values to hs(j) = 0,
*        ready for basis saving and the outside world.
*
*     08 Apr 1992: First version.
*     ------------------------------------------------------------------

      if (mode .eq. 'Internal') then
*        ---------------------------------------------------------------
*        Change nonbasic hs(j) to internal values (including 4 and -1).
*        This may change existing internal values if bl and bu have been
*        changed -- e.g. at the start of each major iteration.
*        ---------------------------------------------------------------
         do 100 j = 1, nb
            if (hs(j) .le. 1) then
               if (bl(j) .eq. bu(j)) then
                  hs(j) =  4
               else if (xn(j) .le. bl(j)) then
                  hs(j) =  0
               else if (xn(j) .ge. bu(j)) then
                  hs(j) =  1
               else
                  hs(j) = -1
               end if
            end if
  100    continue

      else
*        ---------------------------------------------------------------
*        Change hs to external values.
*        Some nonbasic hs(j) may be 4 or -1.  Change them to 0.
*        ---------------------------------------------------------------
         do 200 j = 1, nb
            if (hs(j) .eq. 4  .or.  hs(j) .eq. -1) hs(j) = 0
  200    continue
      end if

*     end of m5hs
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5log ( m, maxs, mbs, n, nb, nn, ns, nx, fsub, objadd,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, hs, kb, x, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   a(ne), bl(nb), bu(nb)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), kb(mbs)
      double precision   x(nx), xn(nb), y(nx), z(nwcore)

*     ------------------------------------------------------------------
*     m5log  prints the iteration log.
*     Normally the only parameters used are nn, ns and fsub.
*     The others are there to allow monitoring of various items for
*     experimental purposes.
*     mbs = m + maxs  .ge.  m + ns,  and   nx = max( mbs, nn ).
*     The array  y(nx)  is available for workspace.
*
*     The print controls are as follows:
*
*     prnt0 is true if print level = 0.
*     For lp and lc, a brief log is output every  k  minor iterations,
*     where  k  is the log frequency.
*     For nlc, a brief log is output by m8setj every major iteration.
*
*     prnt1 is true if print level > 0.
*     A fuller log is output every  k  minor iterations.
*
*     summ0 and summ1 are the same as prnt0 and prnt1, but are false
*     if there is no summary file.  summary frequency defines  k.
*
*     newhed is true if a new heading is required for some reason other
*     than frequency (e.g., after a basis factorization).
*
*     lines1 and lines2 count how many lines have been output to the
*     print and summary files respectively since the last heading.
*     They too may force a new heading.
*
*     10 Apr 1992: objadd added as an input parameter.
*     ------------------------------------------------------------------

      logical            conv
      common    /m1file/ iread,iprint,isumm
      common    /m2lu3 / lenl,lenu,ncp,lrow,lcol
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5freq/ kchk,kinv,ksav,klog,ksumm,i1freq,i2freq,msoln
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log2/ jq1,jq2,jr1,jr2,lines1,lines2
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lp2 / invrq,invitn,invmod
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7phes/ rgmin1,rgnrm1,rgnrm2,jz1,jz2,labz,nfullz,mfullz
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave

      intrinsic          mod
      logical            first , newset, phead , pline, hshort, lshort
      parameter        ( zero = 0.0d+0 )
      character*4        label(2), l
      data               label(1)/' '/, label(2)/'r'/
*     ------------------------------------------------------------------

      first  = minits .le. 1
      hshort = ns     .eq. 0  .and.  nn .eq. 0
      lshort = ns     .eq. 0
      it     = mod( itn, 1000000 )
      ms     = m + ns
      if (ninf .gt. 0) then
         if (iobj .gt. 0) then
            obj = - x(iobj) * sclobj
         else
            obj =   zero
         end if
      else
         obj    = fsub
      end if
      obj    = minimz * obj  +  objadd

      if (prnt0  .and.  nncon .eq. 0) then
*        ---------------------------------------------------------------
*        Terse print.
*        If there are nonlinear constraints, this is handled by m8setj.
*        ---------------------------------------------------------------
         lu     = lenl + lenu
         newset = lines1  .ge.  40
         pline  = first   .or.  mod( itn, klog ) .eq. 0
         phead  = pline  .and. ( newhed .or. newset )

         if ( phead ) then
            newhed = .false.
            lines1 = 0
            if ( hshort ) then
               write(iprint, 1000)
            else
               write(iprint, 1100)
            end if
         end if

         if ( pline ) then
            lines1 = lines1 + 1
            if ( lshort ) then
               write(iprint, 1200) it, djq, ninf, sinf, obj, lu
            else
               write(iprint, 1200) it, djq, ninf, sinf, obj, lu,
     $                             nfobj(1), ns, cond
            end if
         end if
      end if

      if (summ0  .and.  nncon .eq. 0) then
*        ---------------------------------------------------------------
*        Output to the summary file.
*        ---------------------------------------------------------------
         newset = lines2  .ge.  10
         pline  = first   .or.  mod( itn, ksumm ) .eq. 0
         phead  = pline  .and. ( first .or. newset )

         if ( phead ) then
            lines2 = 0
            if ( hshort ) then
               write(isumm , 2000)
            else
               write(isumm , 2100)
            end if
         end if

         if ( pline ) then
            lines2 = lines2 + 1
            if ( lshort ) then
               write(isumm , 2200) it, djq, ninf, sinf, obj
            else
               write(isumm , 2200) it, djq, ninf, sinf, obj, nfobj(1),ns
            end if
         end if
      end if

      if ( prnt1 ) then
*        ---------------------------------------------------------------
*        Detailed print.
*        ---------------------------------------------------------------
         l      = label(labz)
         newset = lines1  .ge.  40
         pline  = first   .or.  mod( itn, klog ) .eq. 0
         phead  = pline  .and. ( newhed .or. newset )

         if ( phead ) then
            newhed = .false.
            lines1 = 0
            if ( hshort ) then
               write(iprint, 3000)
            else
               write(iprint, 3100)
            end if
         end if

         if ( pline ) then
            lines1 = lines1 + 1
            if ( ninf .gt. 0 ) then
               sumobj = sinf
            else
               sumobj = obj
            end if

            if ( lshort ) then
               write(iprint, 3200) it, l, nphs, kprc, djq, jq2, jr2,jr1,
     $            theta, pivot, ninf, sumobj, lenl, lenu, ncp
            else
               write(iprint, 3200) it, l, nphs, kprc, djq, jq2, jr2,jr1,
     $            theta, pivot, ninf, sumobj, lenl, lenu, ncp,
     $            nfobj(1), nfcon(1), ns, modr1, modr2, cond, conv
            end if

*           Special output if Hessian dimension is less than the current
*           number of superbasics, and if variable  jz2  has just been
*           moved from set  z2  into set  z1  in place of variable  jz1.

            if ( jz2 .gt. 0 ) then
               write(iprint, 3300) rgnrm2, jz2, jz1, rgmin1
            end if
         end if
      end if

      if ( summ1 ) then
*        ---------------------------------------------------------------
*        Output to the summary file.
*        ---------------------------------------------------------------
         newset = lines2  .ge.  10
         pline  = first   .or.  mod( itn, ksumm ) .eq. 0
         phead  = pline  .and. ( first .or. newset )

         if ( phead ) then
            lines2 = 0
            if ( hshort ) then
               write(isumm , 4000)
            else
               write(isumm , 4100)
            end if
         end if

         if ( pline ) then
            lines2 = lines2 + 1
            if ( lshort ) then
               write(isumm , 4200) it, djq, ninf, sinf, obj
            else
               write(isumm , 4200) it, djq, ninf, sinf, obj,
     $                             nfobj(1), nfcon(1), ns
            end if
         end if
      end if

*     ------------------------------------------------------------------
*     Debug output.
*     ------------------------------------------------------------------
      if (idebug .eq. 100) then
         write(iprint, 5000) (kb(k), x(k), k = 1, ms)
      end if

*     Exit.

      return

 1000 format(/ '    Itn       dj  ninf      sinf       objective',
     $         '     LU')
 1100 format(/ '    Itn       rg  ninf      sinf       objective',
     $         '     LU  nobj  nsb  cond(H)')
 1200 format(1p, i7, e9.1, i6, e10.3, e16.8, i7, i6, i5, e8.1)
 2000 format(/ '    Itn       dj  ninf      sinf       objective')
 2100 format(/ '    Itn       rg  ninf      sinf       objective',
     $         '  nobj  nsb')
 2200 format(1p, i7, e9.1, i6, e10.3, e16.8, i6, i5)
 3000 format(/ '    Itn ph pp     dj    +sbs  -sbs   -bs',
     $   '  step    pivot   ninf  sinf,objective     L     U ncp')
 3100 format(/ '    Itn ph pp     rg    +sbs  -sbs   -bs',
     $   '  step    pivot   ninf  sinf,objective     L     U ncp',
     $   '  nobj  ncon  nsb Hmod cond(H) conv')
 3200 format(1p, i7, 1x, a1, i1, i3, e9.1, 3i6,
     $   e8.1, e9.1, i5, e16.8, 2i6, i4,
     $   2i6, i5, i3, i2, e8.1, 1x, 4l1)
 3300 format(1p, 17x, e9.1, 2i5, e9.1)
 4000 format(/ '    Itn       dj  ninf      sinf       objective')
 4100 format(/ '    Itn       rg  ninf      sinf       objective',
     $         '  nobj  ncon  nsb')
 4200 format(1p, i7, e9.1, i6, e10.3, e16.8, 2i6, i5)
 5000 format(/ ' BS and SB values...' / (5(i7, g17.8)))

*     end of m5log
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5lpit( m, m1, n, nb, incres,
     $                   ne, nka, a, ha, ka,
     $                   hrtype, hs, kb,
     $                   bl, bu, bbl, bbu, x, xn, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            incres
      integer*4          ha(ne), hrtype(m1), hs(nb)
      integer            ka(nka), kb(m1)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   bbl(m1), bbu(m1),
     $                   x(m1), xn(nb), y(m1), y2(m), z(nwcore)

*     ------------------------------------------------------------------
*     m5lpit performs an iteration of the primal simplex method.
*     jq is the variable entering the basis and djq is its reduced cost.
*
*     00 Aug 1987  This version allows the variable leaving the basis
*                  to be up to featol outside its bounds.
*     26 Mar 1992  incres now passed in as a parameter from m5pric.
*                  We used to say  incres = djq .lt. zero  to mean that
*                  the new variable x(jq) wants to increase, but that
*                  didn't allow a rigorous way of eliminating nonbasics
*                  that are between their bounds with djq = zero.
*     08 Apr 1992  Internal values of hs(*) mean hs(jq) and hs(jr)
*                  must be set more carefully.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log2/ jq1,jq2,jr1,jr2,lines1,lines2
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lp2 / invrq,invitn,invmod
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm

      logical            hitlow, move, onbnd, unbndd
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )
*     ------------------------------------------------------------------

      jq2        = jq
      jr2        = jq
      hrtype(m1) = 0
      bbl(m1)    = bl(jq)
      bbu(m1)    = bu(jq)
      x(m1)      = xn(jq)

*     Unpack column jq into  y2  and solve  B*y = y2.
*     y2  will be altered, and is needed later to modify L and U.

      call m2unpk( jq, m, n, ne, nka, a, ha, ka, y2 )
      call m2bsol( 2, m, y2, y, z, nwcore )

*     Select a variable to be dropped from B.
*     m5chzr  uses the m+1-th element of hrtype, bbl, bbu, x and y.

      if (incres) then
         call dscal ( m, (- one), y, 1 )
         y(m1) =   one
      else
         y(m1) = - one
      end if
      stepmx = 1.0d+11

      call m5chzr( m1    , stepmx, plinfy, tolpiv,
     $             hrtype, bbl   , bbu   , x     , y,
     $             hitlow, move  , onbnd , unbndd,
     $             jp    , bound , exact , theta )

      if (unbndd) go to 800

*     See if there is a basis change.

      jr      = kb(jp)
      if (jp .eq. m1) then

*        Variable jq reaches its opposite bound.

         if (incres) then
            hs(jq)  = 1
         else
            hs(jq)  = 0
         end if
         pivot   = zero
         kmodlu  = 0
         if (ninf .eq. 0) kmodpi = 0
      else

*        Variable jq replaces jr, the jp-th variable of  B.
*        If jr is a fixed variable, its new state is hs(jr) = 4.

         jq1     = jq
         jr1     = jr
         hs(jq)  = 3
         if (bbl(jp) .eq. bbu(jp)) then
            hs(jr) = 4
         else if (hitlow) then
            hs(jr) = 0
         else
            hs(jr) = 1
         end if
         bbl(jp) = bbl(m1)
         bbu(jp) = bbu(m1)
         pivot   = - y(jp)
      end if

*     Update the basic variables x and copy them into xn.

      call daxpy ( m1, theta, y, 1, x, 1 )
      call m5bsx ( 1, m1, nb, kb, x, xn )
      kb(jp) = jq
      x(jp)  = x(m1)
      if (onbnd) xn(jr) = bound
      go to 900

*     The solution is apparently unbounded.

  800 if (iprint .gt. 0) then
         if (incres) then
            write(iprint, 1000) jq
         else
            write(iprint, 1100) jq
         end if
      end if
      ierr   = 2

*     Exit.

  900 return

 1000 format(' Variable', i6, '  can increase indefinitely')
 1100 format(' Variable', i6, '  can decrease indefinitely')

*     end of m5lpit
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5pric( m, mbs, n, nb, nn, nn0, ns, maxr, maxs, incres,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, bl, bu, gsub,
     $                   pi, rc, rg )

      implicit           double precision (a-h,o-z)
      logical            incres
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), kb(mbs)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   gsub(nn0), pi(m), rc(nb), rg(maxs)

*     ------------------------------------------------------------------
*     m5pric  selects a nonbasic variable to enter the basis,
*     using the reduced gradients  dj = gsub(j) - pi(t)*a(j).
*
*     This version does partial pricing on both structurals and slacks.
*     Also, multiple price no longer cancels partial price.
*     Dynamic tolerances are used if partial price is in effect.
*
*     Partial pricing here means sectional pricing, because the
*     columns of  A  and  I  are both sliced up into nparpr sections
*     of equal size.  (The last section of each may be a little bigger,
*     since nparpr is unlikely to divide evenly into  n  or  m.)
*
*     Input    gsub   = gradient for nonlinear variables
*                       (for the subproblem objective function).
*              pi     = pricing vector.
*              kprc   = the no. of the section where  m5pric  last found
*                       a useful dj.
*                       (kprc = 0 at the start of each major iteration.)
*              toldj(1-2) hold the current told if partial pricing, for
*                       phase 1 and 2 respectively.  told is used to
*                       determine if a dj is significant.
*              toldj(3) holds the specified optimality tolerance.
*              biggst   keeps track of the biggest dj found during the
*                       last scan of all sections of ( A  I ).
*
*     Output   kprc   = the last section scanned.
*              nonopt = the no. of useful djs found in that section.
*              jq     = best column found.
*              djq    = best dj.
*              newsb  = no. of new superbasics selected, if nonopt > 0.
*              toldj(1-2) save the current told if partial pricing.
*              incres   says if variable jq should increase or decrease.
*              kb(ns+1), ... kb(ns+newsb) = superbasics selected,
*              rg(ns+1), ... rg(ns+newsb) = their reduced gradients.
*
*     In the code below,
*     the next section of  A  contains npr1 structurals (j1+1 thru k1),
*     the next section of  I  contains npr2 slacks      (j2+1 thru k2).
*     If  nparpr  is rather large, either npr1 or npr2 could be zero,
*     but not both.
*
*     Original version written September 1981.
*
*     00 sep 1986: Modified to allow nonbasic variables to have
*     any value xn(j) between bl(j) and bu(j).  Variables that are
*     strictly between their bounds are allowed to go either way.
*
*x    00 aug 1987: Modified to let nonbasic variables go either way
*x    only if they are greater than bl(j) + featol
*x                    and less than bu(j) - featol,
*x    where featol (in common) is the current feasibility tolerance.
*x
*x    01 sep 1987: Modified to ignore nonbasics whose bounds are
*x    featol or less apart.  this prevents foolish data such as
*x                      0 .le. xn(j) .le. 0.0000001
*x    from causing lots of bound swaps.  m4chek has done its best to
*x    keep xn(j) at zero in such cases.  The final solution may appear
*x    to be non-optimal if xn(j) should really be at the other bound.
*
*     17 sep 1987: Previous two mods undone.  The preceding one allows
*     nonbasic xn(j)'s to be frozen at either 0 or 0.0000001
*     (in that example), and it may then be impossible to make
*     certain basic variables feasible.  pilotja strikes again!
*
*     We now price as follows.  Fixed variables are ignored as always.
*     Nonbasics that are on or outside a bound are allowed to move only
*     one way, towards the opposite bound.
*     Nonbasics strictly between their bounds are allowed to move
*     either way.
*
*     In the latter case, the worry is that rounding error in previous
*     steps (xn = xn + theta*y) might have put a blocking variable
*     very slightly inside one of its bounds.  Letting it move towards
*     that bound will give a very small (and essentially wasted) step.
*     To guard against this, m5chzr chooses a step that puts the
*     blocking variable exactly onto a bound if possible, and now
*     returns the logical onbnd and the relevant value bound so that
*     m5lpit and m7rgit will know whether to set xn(jr) = bound exactly.
*
*     16 Sep 1988: Partial pricing strategy altered to reduce told more
*     systematically.  biggst keeps track of the biggest dj encountered
*     during a scan from section 1 to section nparpr.  In general, told
*     is held fixed until the problem is optimal to within that
*     tolerance.  told is then reduced to reduce * told.
*     (This strategy abandoned April 1992.)
*
*     31 Jan 1992: m5rc   used to compute reduced costs.
*     26 Mar 1992: incres added as output parameter.
*                  Floating nonbasics treated more rigorously
*                  (i.e. nonbasics with djq = 0 or very small
*                  that we want to move towards a bound).
*     08 Apr 1992: Internal values of hs(*) now used (-1 and 4).
*                  xn, bl and bu are no longer needed.
*     09 Apr 1992: Initialize toldj(1-2) = plinfy in m5solv, so told
*                  is always set on the first iteration of Phase 1 or 2.
*     09 Apr 1992: We have had reduce = 0.2 for a while.
*                  Tried 0.1, 0.25 and 0.5.  Decided to go with 0.25.
*                  It still gives a moderate number of reductions
*                  to told, and did a bit better on STAIR and ETAMACRO.
*     11 Aug 1992: Went back to 0.2.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg

      intrinsic          abs, max, min
      logical            feasbl, mulprc, normal
      parameter        ( zero  = 0.0d+0, reduce = 0.2d+0 )
*     ------------------------------------------------------------------

      maxmp  = min( nmulpr, maxs - ns )
      feasbl = ninf   .eq. 0
      mulprc = maxmp  .gt. 1
      normal = .not. mulprc
      djmax  = - plinfy
      djq    = zero
      jq     = 0
      jfree  = 0
      ms     = m + ns
      ncan   = 0
      newsb  = 1
      nonopt = 0
      nprc   = 0
      nparp  = nparpr
      npr1   = n / nparp
      npr2   = m / nparp
      if (max( npr1, npr2 ) .le. 0) nparp = 1

*     Set the tolerance for a significant dj.

      tolmin = toldj(3) * pinorm
      lvldj  = 1
      if ( feasbl )  lvldj = 2
      told   = toldj(lvldj)
      if (nparp .eq. 1) told = tolmin

*     Set pointers to the next section of  A  and  I.
*     nprc counts how many sections have been scanned in this call.
*     kprc keeps track of which one to start with.

  100 nprc = nprc + 1
      kprc = kprc + 1
      if (kprc .gt. nparp) kprc = 1

      npr1 = n / nparp
      j1   = (kprc - 1)*npr1
      k1   = j1 + npr1
      if (kprc .eq. nparp) k1 = n
      npr1 = max( 0, k1-j1 )

      npr2 = m / nparp
      j2   = n + (kprc - 1)*npr2
      k2   = j2 + npr2
      if (kprc .eq. nparp) k2 = nb
      npr2 = max( 0, k2-j2 )

*     ------------------------------------------------------------------
*     Main loops for partial pricing (or full pricing).
*     Compute reduced costs rc(*)
*     for the kprc-th section of structurals
*     and the kprc-th section of slacks.
*     ------------------------------------------------------------------
      call m5rc  ( j1+1, k1, feasbl,
     $             m, n, nn, nn0,
     $             ne, nka, a, ha, ka,
     $             hs, gsub, pi, rc )

      do 200 j = j2+1, k2
         rc(j) = - pi(j - n)
  200 continue

*     ------------------------------------------------------------------
*     Main loop for testing rc(*).
*     dj is rc(j), the reduced cost.
*     d  is -dj or +dj, depending on which way x(j) can move.
*     We are looking for the largest d (which will be positive).
*     ------------------------------------------------------------------
      np   = npr1 + npr2
      j    = j1
      jslk = npr1 + 1

      if ( normal ) then
*        ---------------------------------------------------------------
*        Regular case -- no multiple pricing.
*        ---------------------------------------------------------------
         do 400 jj = 1, np
            if (jj .eq. jslk) j = j2
            j      = j + 1
            js     = hs(j)

            if (js .le. 1) then
               dj     = rc(j)

               if      (js .eq. 0) then
*                 xj  is allowed to increase.
                  d      = - dj
               else if (js .eq. 1) then
*                 xj  is allowed to decrease.
                  d      =   dj
               else
*                 xj  is free to move either way.
*                 Remember him as jfree in case he is the only one.
                  d      = abs( dj )
                  jfree  = j
               end if

*              See if this dj is significant.
*              Also see if it is the biggest dj so far.

               if (d  .gt. told) nonopt = nonopt + 1
               if (djmax .lt. d) then
                  djmax  = d
                  djq    = dj
                  jq     = j
                  kpsav  = kprc
               end if
            end if
  400    continue

      else
*        ---------------------------------------------------------------
*        Multiple pricing.  The first part of the loop is the same.
*        ---------------------------------------------------------------
         do 500 jj = 1, np
            if (jj .eq. jslk) j = j2
            j      = j + 1
            js     = hs(j)

            if (js .le. 1) then
               dj     = rc(j)

               if      (js .eq. 0) then
*                 xj  is allowed to increase.
                  d      = - dj
               else if (js .eq. 1) then
*                 xj  is allowed to decrease.
                  d      =   dj
               else
*                 xj  is free to move either way.
*                 Remember him as jfree in case he is the only one.
                  d      = abs( dj )
                  jfree  = j
               end if

*              See if this dj is significant.
*              Also see if it is the biggest dj so far.

               if (d  .gt. told) nonopt = nonopt + 1
               if (djmax .lt. d) then
                  djmax  = d
                  djq    = dj
                  jq     = j
                  kpsav  = kprc
               end if

*              ========================================
*              Extra stuff for multiple pricing.
*              We have to build the list of candidates.
*              ========================================
               if (d .gt. tolmin) then

*                 Search the list backwards,
*                 starting with smallest rg values.

                  do 410 i  = ncan, 1, -1
                     msi    = ms + i
                     nsi    = ns + i

*                    See if this list element will stay put.
*                    If not, make sure we have space to move it.

                     if (d .le. abs( rg(nsi) )) go to 420

                     if (i .lt. maxmp) then
*                       This element lost the contest but is
*                       staying in the list.
                        kb(msi + 1) = kb(msi)
                        rg(nsi + 1) = rg(nsi)
                     end if
  410             continue

*                 This is the biggest dj so far.

                  i     = 0

*                 If room, put the new candidate into the list.

  420             if (i .lt. maxmp) then
                     i          = i + 1
                     kb(ms + i) = j
                     rg(ns + i) = dj
                     ncan       = min( ncan + 1, maxmp )
                  end if
               end if
            end if
  500    continue
      end if
*     ------------------------------------------------------------------
*     End of loop looking for biggest dj in the kprc-th section.
*     ------------------------------------------------------------------

      if (nonopt .eq. 0) then
         if (nparp .gt. 1) then
*           ============================================================
*           No significant dj has been found.  (All are less than told.)
*           Price the next section, if any remain.
*           ============================================================
            if (nprc .lt. nparp) go to 100

*           ============================================================
*           All sections have been scanned.  Reduce told
*           and grab the best dj if it is bigger than tolmin.
*           ============================================================
            if (djmax .gt. tolmin) then
               nonopt = 1
               kprc   = kpsav
               told   = max( reduce * djmax, tolmin  )
               toldj(lvldj) = told
               if (itn    .gt. 0) wtobj = reduce * wtobj
               if (idebug .ge. 1) then
                  if (iprint .gt. 0)
     $            write(iprint, 1000) itn, told, pinorm, wtobj
                  if (isumm  .gt. 0)
     $            write(isumm , 1000) itn, told, pinorm, wtobj
               end if
            end if
         end if
      end if

*     Finish up multiple pricing.
*     The new dj's are now in descending order, with the biggest
*     in rg(ns+1).  Truncate the list if any are much smaller.

      if ( mulprc ) then
         d      = reduce * djmax
         do 550 j = 2, ncan
            if (abs( rg(ns+j) ) .ge. d) newsb = newsb + 1
  550    continue
      end if

*     -----------------------------------------------------------------
*     Finish if we found someone nonoptimal (nonopt .gt. 0)
*     or if there's a nonbasic floating free
*     between its bounds                    (jfree  .eq. 0)
*     or if the problem is nonlinear        (nn     .gt. 0)
*     or there are some superbasics         (ns     .gt. 0).
*     -----------------------------------------------------------------
      incres = djq .lt. zero
      if (nonopt .gt. 0) go to 900
      if (jfree  .eq. 0) go to 900
      if ( feasbl ) then
         if (nn  .gt. 0) go to 900
         if (ns  .gt. 0) go to 900
      end if

*     -----------------------------------------------------------------
*     jfree > 0.
*     We prefer not to end an LP problem (or an infeasible problem)
*     with some nonbasic variables floating free between their bounds
*     (hs(j) = -1).  Their dj's will be essentially zero
*     but we might as well jam them into the basis.
*
*     We leave true free variables alone -- they will probably be zero.
*     (To be rigorous, we should test if they ARE zero and move them
*     towards zero if not.  This has yet to be done...)
*     -----------------------------------------------------------------
      do 600 j = 1, nb
         if (hs(j) .eq. -1) then
            b1     = bl(j)
            b2     = bu(j)
            if (b1 .gt. -plinfy  .or.  b2 .lt. plinfy) then
      
*              We just found a floating variable with finite bounds.
*              Ask for a move towards the bound nearest zero.
      
               incres = abs(b1) .ge. abs(b2)
               nonopt = 1
               jq     = j
               djq    = rc(j)
               go to 900
            end if
         end if
  600 continue

*     Exit.

  900 kb(ms + 1) = jq
      rg(ns + 1) = djq
      return

 1000 format(' Itn', i7, ' -- toldj =', 1p, e8.1,
     $       '    Norm pi =', e8.1, '    wtobj = ', e8.1)

*     end of m5pric
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5rc  ( j1, j2, feasbl,
     $                   m, n, nn, nn0,
     $                   ne, nka, a, ha, ka,
     $                   hs, gsub, pi, rc )

      implicit           double precision (a-h,o-z)
      logical            feasbl
      integer*4          ha(ne), hs(n)
      integer            ka(nka)
      double precision   a(ne)
      double precision   gsub(nn0), pi(m), rc(n)

*     ------------------------------------------------------------------
*     m5rc   computes reduced costs rc(j) for nonbasic columns of A
*     in the range j = j1 to j2.  It is called by m5pric.
*
*     The loop for computing dj for each column could conceivably be
*     optimized on some machines.  However, there are seldom more than
*     5 or 10 entries in a column.
*
*     Note that we could skip fixed variables by passing in the bounds
*     and testing if bl(j) .eq. bu(j), but these are relatively rare.
*     But see comment for 08 Apr 1992.
*
*     31 Jan 1992: First version
*     08 Apr 1992: Internal values of hs(j) are now used, so fixed
*                  variables (hs(j) = 4) are skipped as we would like.
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0 )

      do 300 j = j1, j2
         if (hs(j) .le. 1) then
            dj    = zero

            do 200 i = ka(j), ka(j+1) - 1
               ir    = ha(i)
               dj    = dj  +  pi(ir) * a(i)
  200       continue

            rc(j) = - dj
         end if
  300 continue

*     Include the nonlinear objective gradient if relevant.

      if (feasbl  .and.  j1 .le. nn) then
         do 350 j = j1, min( j2, nn )
            if (hs(j) .le. 1) rc(j) = rc(j) + gsub(j)
  350    continue
      end if

*     end of m5rc
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5setp( mode, m, rhs, pi, z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   pi(m), rhs(m), z(nwcore)

*     ------------------------------------------------------------------
*     m5setp solves for pi or finds pinorm (or does both).
*     mode = 1     means solve B'pi = rhs and get pinorm.
*     mode = 2     means solve B'pi = rhs.
*     mode = 3     means                 just get pinorm.
*
*     Beware -- in mode 1 and 2, rhs is altered by m2bsol.
*
*     10 Mar 1992: Until now we have required pinorm ge 1.
*                  However, many users have very small objectives,
*                  and the effect is to cause premature termination.
*                  Using pimin < 1 should help fix this.
*     16 Mar 1992: mode used more fully as described above.
*     ------------------------------------------------------------------

      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg

      intrinsic          max
      parameter        ( pimin = 1.0d-2 )

      if (mode .eq. 1  .or.  mode .eq. 2) then
         call m2bsol( 3, m, rhs, pi, z, nwcore )
      end if

      if (mode .eq. 1  .or.  mode .eq. 3) then
         pinorm = dnorm1( m, pi, 1 )
         pinorm = max( pinorm, pimin )
      end if

*     end of m5setp
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5setx( mode, m, n, nb, ms, kb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, x, xn, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka), kb(ms)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   x(ms), xn(nb), y(m), y2(m), z(nwcore)

*     ------------------------------------------------------------------
*     m5setx performs the following functions:
*
*     If mode = 0, the slacks are set to satisfy Ax + s = rhs
*                  if possible.  (Thus s = rhs - Ax, but s may
*                  have to be moved inside its bounds.)
*                  Called (perhaps) from the top of m5solv.
*
*     If mode = 1, the basic components of x are computed to satisfy
*                  Ax + s = rhs; that is  (A I)*xn = rhs.
*                  Then a row check is performed to see how well
*                  (A I)*xn = rhs is satisfied.
*                  y is set to be the row residuals, y = rhs - Ax - s,
*                  and the row error is norm(y).
*                  Called from m2bfac, m5dgen, m5solv.
*
*     If mode = 2  or more, a row check is performed.
*                  Called from m5solv.
*
*     14 Oct 1991: a, ha, ka added as parameters to help with minoss.
*     06 Dec 1991: mode 0 added.  n, bl, bu added as parameters.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5log1/ idebug,ierr,lprint
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold

      intrinsic          abs
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     ------------------------------------------------------------------
*     Compute row residuals  y  =  rhs  -  A*x.
*     ------------------------------------------------------------------
      if (nncon .gt. 0) call dcopy ( nncon, z(lrhs), 1, y, 1 )
      if (nncon .lt. m) call dload ( m-nncon, zero, y(nncon+1), 1 )
      call m2aprd( 5, xn, n, y, m,
     $             ne, nka, a, ha, ka,
     $             z, nwcore )

      if (mode .eq. 0) then
*        ---------------------------------------------------------------
*        Set the slacks.
*        s = y in principle, but we have to make s feasible.
*        Any elements CLOSE to a bound are moved exactly onto the bound.
*        ---------------------------------------------------------------
         tolb   = eps1
         do 10  i = 1, m
            j     = n + i
            b1    = bl(j)
            b2    = bu(j)
            s     = max( y(i), b1 )
            s     = min( s   , b2 )
            if ((s - b1) .gt. (b2 - s)) b1 = b2
            if (abs(s - b1) .le.  tolb) s  = b1
            xn(j) = s
   10    continue

      else
*        ---------------------------------------------------------------
*        Do a row check, perhaps after recomputing the basic x.
*        ---------------------------------------------------------------

*        Get the full row residuals  y  =  rhs  -  (A  I)*xn.

         call daxpy ( m, (- one), xn(n+1), 1, y, 1 )

         if (mode .eq. 1) then
*           ===============================================
*           Extract the basic and superbasic x from xn.
*           See if iterative refinement is worth doing.
*           ===============================================
            call m5bsx ( 2, ms, nb, kb, x, xn )
            ir     = idamax( m, y, 1 )
            rmax   = abs( y(ir) )
            ir     = idamax( m, x, 1 )
            xnorm  = abs( x(ir) )
            rowerr = rmax / (one + xnorm)

            if (rowerr .gt. eps0) then
*              ===============================================
*              Compute a correction to basic x from  B*y2 = y.
*              Set basic x = x + y2.
*              Store the new basic x into xn.
*              ===============================================
               call m2bsol( 2, m, y, y2, z, nwcore )
               call daxpy ( m, one, y2, 1, x, 1 )
               call m5bsx ( 1, m , nb, kb, x, xn )
         
*              Compute  y  =  rhs  -  (A  I)*xn  again for the new xn.
         
               if (nncon .gt. 0) call dcopy ( nncon, z(lrhs), 1, y, 1 )
               if (nncon .lt. m) call dload ( m-nncon, zero,
     $                                                  y(nncon+1), 1 )
               call m2aprd( 5, xn, n, y, m,
     $                      ne, nka, a, ha, ka,
     $                      z, nwcore )
               call daxpy ( m, (- one), xn(n+1), 1, y, 1 )
            end if
         end if

*        Find the norm of x(BS), the basic and superbasic x.
*        Allow for the rare case  x = 0.

         xnorm  = dnorm1( ms, x, 1 )
         if (xnorm .le. eps4) xnorm = one

*        Find the maximum row residual.

         ir     = idamax( m, y, 1 )
         rmax   = abs( y(ir) )
         rowerr = rmax / (one + xnorm)
         if (rowerr .gt. tolrow) ierr = 10
         if (rowerr .gt. tolrow  .or.  idebug .ge. 2) then
            if (iprint .gt. 0) write(iprint, 1000) itn, rmax, ir, xnorm
         end if
      end if
      return

 1000 format(' Itn', i7, ' -- row check',
     $   '.   Max residual =', 1p, e8.1, '  on row', i5,
     $   '.   Norm x =', e9.2)

*     end of m5setx
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m5solv( m, maxr, maxs, mbs, n, nb, nn, nn0, nr,
     $                   lcrash, ns, nscl, nx, objadd,
     $                   ne, nka, a, ha, ka,
     $                   hrtype, hs, kb, ascale, bl, bu,
     $                   bbl, bbu, fsub, gsub, grd, grd2,
     $                   pi, r, rc, rg, rg2,
     $                   x, xn, y, y2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hrtype(mbs), hs(nb)
      integer            ka(nka), kb(mbs)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   bbl(mbs), bbu(mbs),
     $                   gsub(nn0), grd(mbs), grd2(mbs),
     $                   pi(m), r(nr), rc(nb), rg(maxs), rg2(maxs),
     $                   x(nx), xn(nb), y(nx), y2(nx), z(nwcore)

*     ------------------------------------------------------------------
*     m5solv  solves the current problem.  A basis is assumed to be
*     specified by ns, hs(*), xn(*) and the superbasic parts of kb(*).
*     In particular, there must be ns values hs(j) = 2, and the
*     corresponding j's must be listed in kb(m+1) thru kb(m+ns).
*     The ordering in kb matches the projected Hessian R (if any).
*
*     GOTFAC and GOTHES are input and output variables.
*     On entry they say whether to preserve LU and R.  For the first
*              cycle they will normally be false.
*     On exit  they say whether LU and R exist, for possible use on
*              on later cycles.
*     gotlam   no longer exists.  Instead, initial estimates of the
*              Lagrange multipliers for the nonlinear constraints are
*              always extracted from pi.
*
*     On exit, if  ierr .lt. 30  it is safe to save the final
*     basis files and print the solution.  Otherwise, a fatal error
*     condition exists and numerous items will be undefined.
*     The last basis map saved (if any) retains the only useful
*     information.
*
*     31 Jan 1992: rc(*) introduced as workspace for m5pric.
*     06 Mar 1992: Major iteration 1 now just satisfies the linear
*                  constraints and bounds.  It may take as long as it
*                  likes -- the minor iterations limit does not apply.
*     17 Mar 1992: m7fixb implemented to try and fix an ill-conditioned
*                  basis by swapping a column B with a column of S.
*                  In this version, we consider it only at the start of
*                  a major iteration.
*                  fixb   says when we want to try.
*     08 Apr 1992: m5hs   implemented to switch nonbasic hs(j) between
*                  external and internal values to help speed up m5pric.
*     10 Apr 1992: objadd added as an input parameter.
*     04 Jun 1992: Crash level 3 implemented.
*     07 Oct 1992: Minor iterations limit now restricts each major itn
*                  to nminor FEASIBLE iterations.
*     ------------------------------------------------------------------

      logical            conv,restrt
      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2lu3 / lenl,lenu,ncp,lrow,lcol
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3mps2/ lcnam1,lrnam1,lcnam2,lrnam2,lxs,lxl,lfree
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5freq/ kchk,kinv,ksav,klog,ksumm,i1freq,i2freq,msoln
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5log2/ jq1,jq2,jr1,jr2,lines1,lines2
      common    /m5log3/ djq,theta,pivot,cond,nonopt,jp,jq,modr1,modr2
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5lp2 / invrq,invitn,invmod
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m7cg1 / cgbeta,itncg,msgcg,modcg,restrt
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7phes/ rgmin1,rgnrm1,rgnrm2,jz1,jz2,labz,nfullz,mfullz
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      logical            gotbas,gotfac,gothes,gotscl
      common    /cycle1/ gotbas,gotfac,gothes,gotscl
      common    /cycle2/ objtru,suminf,numinf

      character*12       istate
      logical            first , fixb  , incres,
     $                   feasbl, infsbl, jstfes,
     $                   linear, nonlin, lincon, nlncon,
     $                   major1, convgd, optsub

      intrinsic          abs, min, max, mod

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

*     ------------------------------------------------------------------

*     Initialize scalars (in alphabetical order).

      fixb   = .false.
      linear = nn     .eq. 0
      nonlin = nn     .gt. 0
      lincon = nncon  .eq. 0
      nlncon = nncon  .gt. 0

      cond   = zero
      flin   = zero
      fobj   = zero
      fsub   = zero
      fxprev = zero

      ierr   = 0
      invitn = 0
      invrq  = 0
      itn    = 0
      lines1 = 0
      lines2 = 0
      majits = 0
      minits = 0
      minfes = 0
      modwt  = 0
      m1     = m + 1
      nfac   = 0
      nfail  = 0
      ninf   = 0
      nphs   = 1
      nsamef = 0
      msamef = max( 2*nb, 200 )
      msamef = min( msamef, 1000 )
      nssave = ns

      objtru = zero
      penpar = dparm(7)
      pinorm = one
      rgnorm = zero
      rgtest = zero
      sinf   = zero
      tolz   = eps0
      xnorm  = one
      do 30 i = 1, 4
         conv(i) = .false.
   30 continue

      newhed = .true.
      if (.not. gothes) r(1) = zero

      if ( lincon ) then

*        Set nonbasic hs(j) to the correct internal values.

         call m5hs  ( 'Internal', nb, bl, bu, hs, xn )
      else
*        ( nlncon )
         infsub = 0
         lcstat = 0
         nomove = 0
         nreduc = 0
         gotfac = .false.
         optsub = .false.
      end if

*     ------------------------------------------------------------------
*     Start of a Major Iteration.
*     "first" says we've just started, and tells m5frmc to set nphs.
*     If the problem is linearly constrained, there is only one
*     major iteration (only one subproblem to be solved).
*     ------------------------------------------------------------------
   90 majits = majits + 1
      major1 = majits .eq. 1
      first  = .true.
      ierr   = 0
      kprc   = 0
      lvldif = 1
      lvltol = 1
      ms     = m + ns
      nfail  = 0
      nfullz = 0
      nonopt = 0
      toldj(1) = plinfy
      toldj(2) = plinfy
      tolrg  = zero
      call m5dgen( 1, m, n, nb, ms, inform,
     $             ne, nka, a, ha, ka,
     $             hs, kb, bl, bu, x, xn, y, y2, z, nwcore )

      if (nlncon) then
         call m8setj( lcstat, m, n, nb, nn, nncon, nnjac, njac, nscl,
     $                lcrash, ns,
     $                nconvg, nomove, nreduc, optsub, convgd,
     $                ne, nka, a, ha, ka, hs, ascale, bl, bu,
     $                z(lblslk),z(lbuslk),z(lfcon), z(lfcon2),
     $                z(lfdif), z(lfold), z(lgcon), z(lgcon2), z(lxlam),
     $                z(lrhs ), z(lxdif), z(lxold),
     $                pi, xn, y, z, nwcore )

         minits = 0
         minfes = 0
         fixb   = ns .gt. 0
         call m1envt( 999 )
         if (ierr   .ne.   0   ) go to 900
         if (      convgd      ) go to 800
         if (majits .gt. nmajor) go to 825
         if (itn    .ge. itnlim) go to 830
         if (nomove .ge.   4   ) go to 850
         optsub = .false.
      end if

*--   if (major1) then

*        Set the slacks s using the current structurals x.
*        If lincon, try and satisfy Ax + s = 0.
*        If nlncon, try and satisfy Ax + s = rhs.

*--      call m5setx( 0, m, n, nb, ms, kb,
*--  $                ne, nka, a, ha, ka,
*--  $                bl, bu, x, xn, y, y2, z, nwcore )
*--   end if

*     ------------------------------------------------------------------
*     Factorize the basis ( B = L*U ).  If gotfac is true on entry to
*     m5solv, the first call to m2bfac will try to use existing factors.
*     ------------------------------------------------------------------
  100 if (first) then
*        relax
      else
*        Safeguard against system error: If there have been no
*        iterations since the last factorize, something is haywire.
         if (invitn .eq. 0) go to 865
      end if

  105 call m2bfac( gotfac, nfac, m, mbs, n, nb, nr, nn, ns,
     $             lcrash, fsub, objadd,
     $             ne, nka, a, ha, ka,
     $             kb, hs, bl, bu, bbl, bbu,
     $             r, grd, x, xn, y, y2, z, nwcore )
      if (ierr .gt. 0) go to 860

      nssave = ns
      lusiz0 = lenl + lenu
      lumax  = 2*lusiz0
      kmodpi = 1
      restrt = .true.
      if (mod(lprint,10) .gt. 0) newhed = .true.

*     Under certain circumstances, see if B is ill-conditioned
*     and perhaps improve it by swapping in a superbasic.

  110 if ( fixb ) then
         fixb   = .false.
         ms     = m + ns
         call m7fixb( m, maxr, ms, n, nb, nr, ns, nx, inform,
     $                ne, nka, a, ha, ka,
     $                hs, kb, bl, bu, bbl, bbu,
     $                r, x, y, y2, z, nwcore )

*        Refactorize if m7fixb tried to update B = L*U and failed.

         if (inform .eq. 5) go to 105
      end if

*     ------------------------------------------------------------------
*     MAIN ITERATION LOOP.
*     ------------------------------------------------------------------
  200 ierr   = 0
      ms     = m + ns
      ninf0  = ninf
      sinf0  = sinf

*     Increment featol every iteration.
*     Every kdegen iterations, reset featol and move nonbasic variables
*     onto their bounds if they are currently very close.

      featol = featol + tolinc
      if (mod( minits, kdegen ) .eq. 0) then
         if (minits .gt. 0) then
            call m5dgen( 2, m, n, nb, ms, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, bl, bu, x, xn, y, y2, z, nwcore )
         end if
      end if

*     ------------------------------------------------------------------
*     Check feasibility.
*     ------------------------------------------------------------------
      call m5frmc( n, nb, nn, ns, ms, maxs,
     $             lcrash, first, fsub, featol, objadd,
     $             ne, nka, a, ha, ka,
     $             bl, bu, bbl, bbu, hrtype, hs, kb,
     $             grd2, x, xn, z, nwcore )

      if (ierr .ne. 0) go to 900
      feasbl =  ninf  .eq. 0
      infsbl = .not. feasbl
      jstfes =  feasbl  .and.  (first  .or.  ninf0 .gt. 0)
      first  = .false.

      if (infsbl) then

*        Reduce wtobj if sinf has increased noticeably.

         if (wtobj .gt. zero) then
            if (itn .gt. 0) then
               if (sinf  .gt.  sinf0 * 1.1) go to 655
            end if
         end if
      end if

*     ------------------------------------------------------------------
*     See if the next phase of Crash is needed.
*     ------------------------------------------------------------------
      if (jstfes  .and.  lcrash .gt. 0) then
         if      (lcrash .eq. 1) then

*           All constraints have now been satisfied.
*           Major1 will be terminated below after pi is set.

            lcrash = 0

         else if (lcrash .eq. 2  .or.  lcrash .eq. 4) then

*           Linear constraints have been satisfied.
*           Major1 will be terminated and Major2 will
*           call Crash on nonlinear rows.

            if (lincon) lcrash = 0
            if (nlncon) lcrash = 5

         else if (lcrash .eq. 3) then
            if (nncon .lt. m) then

*              So far the linear inequality constraints (if any)
*              have been ignored.  Do another Crash and continue Major1.
*              m2amat sets row-types.
            
               lcrash = 4
               call m5hs  ( 'External', nb, bl, bu, hs, xn )
               call m2amat( 2, m, n, nb,
     $                      ne, nka, a, ha, ka,
     $                      bl, bu, hrtype )
               call m2crsh( lcrash, m, n, nb, nn,
     $                      ne, nka, a, ha, ka,
     $                      kb, hs, hrtype, bl, bu, xn, z, nwcore )
               call m5hs  ( 'Internal', nb, bl, bu, hs, xn )
               ninf   = 1
               go to 105

            else
               lcrash = 5
            end if
         end if
      end if

*     ------------------------------------------------------------------
*     Compute pi.
*     ------------------------------------------------------------------
      if (kmodpi .gt. 0) then
         if (feasbl  .and.  nonlin) then
            call m6grd ( ms, nb, nn, gsub, grd2,
     $                   ne, nka, a, ha, ka,
     $                   xn, z, nwcore )
            if (ierr .ne. 0) go to 900
         end if
         call dcopy ( ms, grd2, 1, grd, 1 )
         call m5setp( 1, m, grd2, pi, z, nwcore )
      end if

*     ------------------------------------------------------------------
*     If we have just got feasible, reset a few things.
*     If there are nonlinear constraints and it's Major1,
*     we have satisfied the linear constraints for the first time.
*     That is enough for now --- time to relinearize.
*     ------------------------------------------------------------------
      if (jstfes) then
          lvltol = 1
          tolrg  = zero
          if (nlncon  .and.  major1) go to 500
      end if

*     ------------------------------------------------------------------
*     Compute rg = reduced gradient.
*     ------------------------------------------------------------------
      rgnorm = zero
      if (nphs .gt. 2) then
         if (ns .gt. 0) then
            call m7rg  ( m, ms, ns, grd, pi, rg, rgnorm,
     $                   ne, nka, a, ha, ka,
     $                   z, nwcore )
         end if
         if (tolrg .eq. zero) tolrg = etarg * rgnorm
      end if

*     ------------------------------------------------------------------
*     Proceed with the next iteration.
*     ------------------------------------------------------------------
  250 kmodlu = 1
      kmodpi = 1
      modr1  = 0
      modr2  = 0
      jq1    = 0
      jq2    = 0
      jr1    = 0
      jr2    = 0
      jz2    = 0
      labz   = 1
      nxtphs = nphs
      if (jobj .gt. 0) flin = - xn(jobj) * sclobj  +  objadd
      objtru = flin + fobj

      if (nphs .le. 3) then

*        Phase 1, 2 or 3.
*        PRICE:
*        Select one or more nonbasic variables to become superbasic.

         if (ns .eq. maxs) go to 840

         call m5pric( m, mbs, n, nb, nn, nn0, ns, maxr, maxs, incres,
     $                ne, nka, a, ha, ka,
     $                hs, kb, bl, bu, gsub,
     $                pi, rc, rg )

         if (nonopt .eq. 0) go to 400
         if (newsb  .gt. 1) nphs   = 3
         if (   nlncon    ) nconvg = 0
         lvldif = 1
         lvltol = 1
         nsubsp = 0
      end if

*     Test for excess iterations or time.

  260 if (lincon) then
         if (itn    .ge. itnlim) go to 830
         if (mod(itn,10) .eq. 0) call m1envt( 999 )
      else
         if (itn    .ge. itnlim) go to 500

*        The first major iteration may take as long as it likes
*        to satisfy the linear constraints.  Later major iterations
*        are terminated by the minor iterations limit.
*        07 Oct 1992: Let all majors take as long as they like
*                     to get feasible.  Use minfes instead of minits.

         if (minfes .ge. nminor) go to 500
      end if
      if (ierr .gt. 0) go to 880

*     ==================================================================
*     Perform a minor iteration (either LP or reduced-gradient method).
*     ==================================================================
  280 if (nphs .le. 2) then
         call m5lpit( m, m1, n, nb, incres,
     $                ne, nka, a, ha, ka,
     $                hrtype, hs, kb,
     $                bl, bu, bbl, bbu, x, xn, y, y2, z, nwcore )
      else
         if (nphs .eq. 4  .and.  ns .le. 0) then
            nphs = 3
            go to 250
         end if

         call m7rgit( m, maxr, maxs, mbs, n, nb, incres,
     $                nn, nn0, nr, ns, nx, inform, nxtphs,
     $                ne, nka, a, ha, ka,
     $                hrtype, hs, kb, bl, bu, bbl, bbu,
     $                fsub, gsub, grd, grd2,
     $                pi, r, rg, rg2, x, xn, y, y2, z, nwcore )

*        For certain values of  inform  we want to try again.
*        If inform = 6, m7fixb has been requested (to fix R).

         if (inform .eq. 6) then
            fixb   = .true.
            if (invitn .gt. 0) go to 105
            go to 110
         end if

*        If inform = 5, m2bfac has been requested
*                       (part of error recovery).
*        If inform = 4, we want a better  gsub  and  pi  using
*                       central differences (non-derivative case only).
*        If inform = 1, we want to call  m5pric.

         if (inform .eq. 5) go to 100
         if (inform .eq. 4) go to 200
         if (inform .eq. 1) go to 250

*        Otherwise, m7rgit exits with a good pi and grd
*        (unless ierr gt 0).
*        Set kmodpi to indicate that we don't need a new pi.

         kmodpi = 0
      end if

*     ==================================================================
*     Test for error condition and/or frequency interrupts.
*     ==================================================================
  300 ms     = m + ns
      if (ierr .eq. 2) go to 460
      if (ierr .gt. 0) go to 680
      itn    = itn    + 1
      invitn = invitn + 1
      minits = minits + 1
      if (ninf .eq. 0) minfes = minfes + 1
      if (linear  .and.  iobj .ne. 0) fsub = - minimz * x(iobj) * sclobj

*     Print the iteration log.

      call m5log ( m, maxs, mbs, n, nb, nn, ns, nx, fsub, objadd,
     $             ne, nka, a, ha, ka,
     $             bl, bu, hs, kb, x, xn, y, z, nwcore )

*     Check for the dreaded infinite loop
*     by seeing if the objective has stayed the same much too long.

      nsamef = nsamef + 1
      fx     = sinf
      if (     feasbl       ) fx = fsub
      if (abs( fxprev - fx )  .gt.  (one + abs( fx )) * eps1) nsamef = 0
      if (nsamef .ge. msamef) go to 835

*     Press on.

      fxprev = fx
      nphs   = nxtphs
      if (nlncon) then
         if (infsbl  .or.  rgnorm .gt. rgtest) nconvg = 0
      end if

*     Save a basis map (frequency controlled).
*     We have to convert hs(*) to External and then back to Internal.

      if (mod(itn,ksav) .eq. 0) then
         if (inewb .gt. 0  .and.  itn .lt. itnlim) then
            call m5hs  ( 'External', nb, bl, bu, hs, xn )
            call m4stat( 0, istate )
            call m4newb( 1, inewb, m, n, nb, nn, ns, ms, nscl, fsub,
     $                   kb, hs, ascale, bl, bu, x, xn, istate )
            if (iback .gt. 0)
     $      call m4newb( 1, iback, m, n, nb, nn, ns, ms, nscl, fsub,
     $                   kb, hs, ascale, bl, bu, x, xn, istate )
            call m5hs  ( 'Internal', nb, bl, bu, hs, xn )
         end if
      end if

*     Refactorize the basis if it has been modified a lot.

      if (invmod .ge. kinv - 1) then
         invrq  = 1
         go to 100
      end if

      lusize = lenl + lenu
      if (invmod .ge. 20  .and.  lusize .gt. lumax) then
         bgrwth = lusize
         bold   = lusiz0
         bgrwth = bgrwth / bold
         if (prnt1) write(iprint, 1030) bgrwth
         invrq  = 2
         go to 100
      end if

*     Update the LU factors of the basis if requested.

      if (kmodlu .eq. 1) call m2bsol( 4, m, y2, y, z, nwcore )
      if (invrq  .ne. 0) go to 100

*     Check row error (frequency controlled).

      if (mod(invitn,kchk) .eq. 0) then
         call m5setx( 2, m, n, nb, ms, kb,
     $                ne, nka, a, ha, ka,
     $                bl, bu, x, xn, y, y2, z, nwcore )
         if (ierr .gt. 0) then
            invrq = 10
            go to 100
         end if
      end if

      go to 200

*     ------------------------------------------------------------------
*     END OF MAIN ITERATION LOOP.
*     ------------------------------------------------------------------

*     The solution is apparently optimal -- check rgtols, etc.

  400 if ( infsbl ) then
*        =============================
*        Optimal but still infeasible.
*        =============================
         if (wtobj  .ne. zero ) go to 655

*        See if any nonbasics have to be set back on their bound.

         call m5dgen( 3, m, n, nb, ms, inform,
     $                ne, nka, a, ha, ka,
     $                hs, kb, bl, bu, x, xn, y, y2, z, nwcore )
         if (inform .gt.   0  ) go to 200

         if (ns     .eq.   0  ) go to 410
         if (lvltol .eq.   2  ) go to 410
         lvltol = 2
         tolrg  = toldj(3) * pinorm
         if (rgnorm .le. tolrg) go to 410

         nphs   = 4
         if (prnt1) write(iprint, 1400) tolrg, lvltol
         go to 280

*        The subproblem is infeasible.
*        Stop if the constraints are linear
*        or if this is the first major iteration
*        (which could not satisfy the linear constraints).

  410    lcstat = 1
         if (lincon  .or.  major1) go to 810
         if (modrhs .eq. 0) infsub = infsub + 1
         if (infsub .gt. 5) go to 810

*        Slightly relax the bounds on the linearized constraints.

         call m8augl( 5, m, n, nb, ns, inform,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, xn, z, nwcore )

         if (inform .eq. 0) then

*           Reset nonbasic hs(j) to internal values.
*           Reset the basic x's and check the row error.

            kmodpi = 1
            call m5hs  ( 'Internal', nb, bl, bu, hs, xn )
            call m5setx( 1, m, n, nb, ms, kb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, x, xn, y, y2, z, nwcore )
            if (ierr .gt. 0) then
               invrq = 10
               go to 100
            end if
            go to 200
         end if

*        The rhs has been relaxed often enough for this subproblem.
*        Keep going if it's the first time or something is happening.

         if (infsub .le. 1  .or.  minits .gt. 0) go to 500
         go to 810
      else
*        =============================================
*        Feasible and maybe optimal.
*        m5pric says there are no nonbasics to bring in.
*        We can stop if rgnorm is suitably small.
*        For Partial Completion (ncom = 0), a looser
*        measure of small will do.
*        =============================================
         if (ns   .eq. 0) go to 420
         if (ncom .eq. 0) then
            tolmin = 1.0d-2   * pinorm
         else
            tolmin = toldj(3) * pinorm
         end if
         if (rgnorm .le. tolmin) go to 420
         if (lvltol .eq.    2  ) go to 420
         if (nfail  .gt. 0  .and.
     $       nfullz .le.    1  ) go to 420

         nfail  = 0
         nphs   = 4
         tolrg  = 0.1 * min( tolrg, rgnorm )
         if (tolrg  .le. tolmin) then
            lvltol = 2
            tolrg  = tolmin
         end if
         if (prnt1) write(iprint, 1400) tolrg, lvltol
         go to 280

*        The rgtols are small enough.
*        See if any nonbasics have to be set back on their bound.

  420    call m5dgen( 3, m, n, nb, ms, inform,
     $                ne, nka, a, ha, ka,
     $                hs, kb, bl, bu, x, xn, y, y2, z, nwcore )
         if (inform .gt. 0) go to 200

*        Check residuals before terminating.

         djq    = minimz * djq
         if (prnt1) write(iprint, 1410) djq, jq, rgnorm, pinorm

         if (invitn .gt. 0) then
            call m5setx( 3, m, n, nb, ms, kb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, x, xn, y, y2, z, nwcore )
            if (ierr .gt. 0) then
               invrq = 10
               go to 100
            end if
         end if

*        The subproblem is optimal.

         optsub = .true.
         lcstat = 0
         if ( nlncon ) then
            if (prnt1 ) write(iprint, 1510) majits, minits, itn
            if (summ1 ) write(isumm , 1511) minits, itn
            rgtest = 100.0 * toldj(3) * pinorm
            if (modrhs .eq. 0) infsub = 0
            if (modrhs .gt. 0) lcstat = 1
            nphs   = 4
            go to 90
         end if
         go to 800
      end if

*     The subproblem is unbounded.
*     Reduce wtobj or increase the penalty parameter.

  460 if (infsbl  .and.  wtobj .gt. zero) go to 650
      call m5dgen( 3, m, n, nb, ms, inform,
     $             ne, nka, a, ha, ka,
     $             hs, kb, bl, bu, x, xn, y, y2, z, nwcore )
      if (inform .gt. 0) go to 200

      lcstat = 2
      if ( nlncon ) then
         call m8augl( 4, m, n, nb, ns, inform,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, xn, z, nwcore )
         if (inform .eq. 0) then
            kmodpi = 1
            ninf   = 1
            go to 200
         end if
      end if
      go to 820

*     Minor iterations terminated.

  500 lcstat = 3
      if (prnt1) write(iprint, 1500) majits, minits, itn
      if (summ1) write(isumm , 1501) minits, itn
      nphs   = 4
      go to 90

*     Reset or reduce wtobj.

  650 wtobj  = zero
  655 modwt  = modwt + 1
      wtobj  = 0.1 * wtobj
      if (modwt .gt. 5) wtobj = zero
      if (prnt1 .or. idebug .ge. 1) write(iprint, 1200) wtobj
      go to 200

*     Error condition.
*     If it was a linesearch failure, keep the major iterations
*     going if this one did at least one minor iteration.
*     Otherwise, print something helpful and terminate.
*     t1  is something that ought to be small at a solution.
*     t2  is what we consider 'reasonably small'.

  680 if (ierr .ne. 9) go to 900
      t1     = rgnorm / pinorm
      t2     = 0.1
      if ( nlncon ) then
         if (minits .gt. 0) go to 500
         t1  = max( t1, virel )
      end if

      if (iprint .gt. 0) write(iprint, 1104)
      if (isumm  .gt. 0) write(isumm , 1104)
      if (lderiv .gt. 0  .and.  .not. GAMS) then
         if (iprint .gt. 0) write(iprint, 1105)
         if (isumm  .gt. 0) write(isumm , 1105)
      end if
      if (t1 .lt. t2) then
         if (iprint .gt. 0) write(iprint, 1114)
         if (isumm  .gt. 0) write(isumm , 1114)
      else
         if (iprint .gt. 0) write(iprint, 1115)
         if (isumm  .gt. 0) write(isumm , 1115)
      end if
      go to 850

*     ------------------------------------------------------------------
*     Exit.
*     m1page( ) decides whether to do a page eject before the EXIT msg.
*     m1page(2) also calls m1envt(1) to print '=1' for GAMS
*     ------------------------------------------------------------------

*     Optimal.

  800 call m1page( 2 )
      rgtest = rgnorm / pinorm
      if (rgtest .lt. 0.1) then
         if (iprint .gt. 0) write(iprint, 1800)
         if (isumm  .gt. 0) write(isumm , 1800)
      else
         if (iprint .gt. 0) write(iprint, 1802)
         if (isumm  .gt. 0) write(isumm , 1802)
         ierr   = 13
      end if
      go to 900

*     Infeasible.

  810 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1810)
      if (isumm  .gt. 0) write(isumm , 1810)
      ierr   = 1
      go to 900

*     Unbounded.

  820 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1820)
      if (isumm  .gt. 0) write(isumm , 1820)
      go to 900

*     Too many iterations.

  825 call m1envt( 1 )
      if (iprint .gt. 0) write(iprint, 1825)
      call m1envt( 2 )
  830 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1830)
      if (isumm  .gt. 0) write(isumm , 1830)
      ierr   = 3
      go to 900

*     The objective hasn't changed for ages.

  835 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1835) msamef
      if (isumm  .gt. 0) write(isumm , 1835) msamef
      ierr   = 4
      go to 900

*     maxs  too small.

  840 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1840) maxs
      if (isumm  .gt. 0) write(isumm , 1840) maxs
      ierr   = 5
      go to 900

*     Linesearch failure.

  850 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1850)
      if (isumm  .gt. 0) write(isumm , 1850)
      ierr   = 9
      go to 900

*     Fatal error after LU factorization.

  860 if (ierr .ne. 10) go to 900
      call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1860)
      if (isumm  .gt. 0) write(isumm , 1860)
      go to 900

*     m2bfac called twice in a row.

  865 call m1page( 2 )
      if (iprint .gt. 0) write(iprint, 1865)
      if (isumm  .gt. 0) write(isumm , 1865)
      ierr   = 12
      go to 900

*     Resource interrupt (too much time, etc.)

  880 call m1envt( 1 )
      if (iprint .gt. 0) write(iprint, 1880)
      if (isumm  .gt. 0) write(isumm , 1880)
      ierr   = 19
      go to 900

*     ------------------------------------------------------------------
*     Set output variables and print a summary of the final solution.
*     M1ENVT( 2 ) prints '=2' for GAMS.
*     ------------------------------------------------------------------

  900 call m1envt( 2 )
      gotfac = ierr .le. 9
      gothes = r(1) .ne. zero
      degen  = 100.0 * ndegen / max( itn, 1 )
      infsbl = ninf .gt. 0
      suminf = sinf
      numinf = ninf
      rgtest = rgnorm / pinorm
      xnorm  = dnorm1( nb, xn, 1 )

*     Count basic nonlinear variables (not used for anything).

      nnb    = 0
      do 910 j = 1, nn
         if (hs(j) .eq. 3) nnb = nnb + 1
  910 continue

*     Restore nonlinear constraints bounds, in case they were relaxed.
*     Maybe reduce majits for printing purposes.

      if (nlncon) then
         call m8augl( 6, m, n, nb, ns, inform,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, xn, z, nwcore )

         if (minits .eq. 0) majits = majits - 1
      end if

*     Set nonbasic hs(j) to external values.
*     Save the final basis file.

      call m5hs  ( 'External', nb, bl, bu, hs, xn )
      if (inewb .gt. 0  .and.  ierr .lt. 20) then
         k = ierr + 1
         call m4stat( k, istate )
         call m4newb( 2, inewb, m, n, nb, nn, ns, ms, nscl, fsub,
     $                kb, hs, ascale, bl, bu, x, xn, istate )
      end if

*     Print statistics.

      if (iprint .gt. 0) then
                     write(iprint, 1900) name, itn, objtru
         if (infsbl) write(iprint, 1910) ninf, sinf
         if (nonlin) write(iprint, 1920) majits, flin, penpar, fobj
         if (nonlin) write(iprint, 1950) nfobj(1), nfcon(1)
         if (nonlin  .and.  lderiv .lt. 3)
     $               write(iprint, 1960) (nfobj(i), nfcon(i), i=2,4)
         if (nonlin  .or.  ns .gt. 0)
     $               write(iprint, 1970) ns, rgnorm, nnb, rgtest
                     write(iprint, 1975) ndegen, degen
      end if

      if (isumm  .gt. 0) then
                     write(isumm , 1900) name, itn, objtru
         if (infsbl) write(isumm , 1910) ninf, sinf
         if (nonlin) write(isumm , 1920) majits, flin, penpar, fobj
         if (nonlin) write(isumm , 1950) nfobj(1), nfcon(1)
         if (nonlin  .and.  lderiv .lt. 3)
     $               write(isumm , 1960) (nfobj(i), nfcon(i), i=2,4)
         if (nonlin  .or.  ns .gt. 0)
     $               write(isumm , 1970) ns, rgnorm, nnb, rgtest
                     write(isumm , 1975) ndegen, degen
      end if

  950 return


 1030 format(  ' LU file has increased by a factor of', f6.1)
 1104 format(/ ' XXX  The functions are non-smooth or very nonlinear')
 1105 format(  ' XXX  MAKE SURE THE GRADIENTS ARE CODED CORRECTLY')
 1114 format(  ' XXX  The final point is REASONABLY NEAR AN OPTIMUM' /)
 1115 format(  ' XXX  The final point is NOT CLOSE TO AN OPTIMUM' /)
 1200 format(/ ' Weight on obj reduced to', 1p, e12.2)
 1400 format(  ' tolrg  reduced to', 1p, e11.3, 5x, ' lvltol =', i2)
 1410 format(/ ' Biggest dj =', 1p, e11.3, ' (variable', i7, ')',
     $   '   norm rg =', e11.3, '   norm pi =', e11.3)
 1500 format(/ ' End of major itn', i4,
     $   '  -  minor itns terminated at', i4,
     $   '  -  total itns =', i7)
 1501 format(' Minor itns terminated at', i4,
     $   '  -  total itns =', i7)
 1510 format(/ ' End of major itn', i4,
     $   '  -  optimal subproblem at minor itn', i4,
     $   '  -  total itns =', i7)
 1511 format(' Optimal subproblem at minor itn', i4,
     $   '  -  total itns =', i7)

 1800 format(' EXIT -- optimal solution found')
 1802 format(' EXIT -- near-optimal solution found'
     $  /' XXX WARNING: reduced gradient is large --'
     $   ' solution is not really optimal')
 1810 format(' EXIT -- the problem is infeasible')
 1820 format(' EXIT -- the problem is unbounded ',
     $   ' (or badly scaled).')
 1825 format(' XXX  Major iterations terminated before convergence')
 1830 format(' EXIT -- too many iterations')
 1835 format(' EXIT -- the objective has not changed',
     $   ' for the last', i8, '  iterations')
 1840 format(' EXIT -- the superbasics limit is too small:', i7)
 1850 format(' EXIT -- the current point cannot be improved upon')
 1860 format(' EXIT -- numerical error.  General constraints',
     $   ' cannot be satisfied accurately')
 1865 format(' EXIT -- basis factorization requested',
     $   ' twice in a row')
 1880 format(' EXIT -- resource interrupt')

 1900 format(/ 1x, 2a4
     $       / ' No. of iterations', i20,
     $     2x, ' Objective value', 1p, e22.10)
 1910 format(  ' No. of infeasibilities', i15,
     $     2x, ' Sum of infeas', 1p, e24.10)
 1920 format(  ' No. of major iterations', i14,
     $     2x, ' Linear objective', 1p, e21.10
     $       / ' Penalty parameter', 0p, f20.6,
     $     2x, ' Nonlinear objective', 1p, e18.10)
 1950 format(  ' No. of calls to funobj', i15,
     $     2x, ' No. of calls to funcon', i15)
 1960 format(  ' Calls with mode=2 (f, known g)', i7,
     $     2x, ' Calls with mode=2 (f, known g)', i7
     $       / ' Calls for forward differencing', i7,
     $     2x, ' Calls for forward differencing', i7
     $       / ' Calls for central differencing', i7,
     $     2x, ' Calls for central differencing', i7)
 1970 format(  ' No. of superbasics', i19,
     $     2x, ' Norm of reduced gradient', 1p, e13.3
     $       / ' No. of basic nonlinears', i14,
     $     2x, ' Norm rg / Norm pi', e20.3)
 1975 format(  ' No. of degenerate steps', i14,
     $     2x, ' Percentage', f27.2)

*     end of m5solv
      end   
