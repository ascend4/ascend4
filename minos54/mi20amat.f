************************************************************************
*
*     File  mi20amat fortran.
*
*     m2core   m2amat   m2aprd   m2apr1   m2apr5
*     m2crsh   m2scal   m2scla   m2unpk   matcol
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2core( mode, mincor )

      implicit           double precision (a-h,o-z)

*     ------------------------------------------------------------------
*     m2core  allocates all array storage for MINOS,
*     using various dimensions stored in common blocks as follows:
*        (m2len )   mrows, mcols, melms
*        (m3len )   nscl
*        (m5len )   maxr, maxs, nn
*        (m7len )   nnobj
*        (m8len )   njac, nncon, nnjac
*
*     if mode = 1, the call is from minos1 to estimate the storage
*                  needed for all stages of the problem.
*     if mode = 2, the call is from m3inpt to find the minimum storage
*                  needed for mps input.
*     if mode = 3, all dimensions should be known exactly.
*                  The call is from m3inpt to find the minimum
*                  storage to solve the problem and output the solution.
*                  The basis factorization routines will say later
*                  if they have sufficient storage.
*     if mode = 4, the call is from minoss and all dimensions are known
*                  exactly.  a, ha, ka etc need not be allocated.
*     ------------------------------------------------------------------

      common    /m1word/ nwordr,nwordi,nwordh
      common    /m2len / mrows,mcols,melms
      common    /m2mapa/ ne    ,nka   ,la    ,lha   ,lka
      common    /m2mapz/ maxw  ,maxz
      common    /m3len / m     ,n     ,nb    ,nscl
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m3mps1/ lname1,lname2,lkeynm,nname
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m7cg2 / lcg1,lcg2,lcg3,lcg4,modtcg,nitncg,nsubsp
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
*     ------------------------------------------------------------------

      m      = mrows
      n      = mcols
      ne     = melms
      mbs    = m + maxs
      nka    = n + 1
      nb     = n + m
      nscl   = nb
      if (lscale .eq. 0) nscl  = 1
      nname  = nb
      if (mode   .eq. 4) nname = 1

      ncoli  = n/nwordi + 1
      nrowi  = m/nwordi + 1
      lenh   = max( 3*nrowi, 100 )

      nn0    = max( nn   , 1 )
      nncon0 = max( nncon, 1 )
      nnobj0 = max( nnobj, 1 )
      maxr   = max( maxr , 1 )
      nr     = maxr*(maxr + 1)/2  +  (maxs - maxr)
      nx     = max( mbs, nn )
      nx2    = 0
      if (lscale .ge. 2) nx2 = nn

      if (mode .le. 3) then

*        MINOS is getting its data from an MPS file.
*        Allocate arrays that are arguments of minoss and misolv.
*        These are for the data,
*                 A, ha, ka, bl, bu, name1, name2,
*        and for the solution
*                 hs, xn, pi, rc.

         la     = maxw   + 1
         lha    = la     + ne
         lka    = lha    + 1 + (ne /nwordh)
         lbl    = lka    + 1 + (nka/nwordi)
         lbu    = lbl    + nb
         lname1 = lbu    + nb
         lname2 = lname1 + 1 + (nname/nwordi)
         lhs    = lname2 + 1 + (nname/nwordi)
         lxn    = lhs    + 1 + (nb /nwordh)
         lpi    = lxn    + nb
         lrc    = lpi    + m
         minsub = lrc    + nb
      else

*        The subroutine version can use all of z except up to maxw.

         minsub = maxw   + 1
      end if

*     Allocate arrays needed during and after MPS input.

      lhrtyp = minsub
      lkb    = lhrtyp + 1 + (mbs/nwordh)
      minz   = lkb    + 1 + (mbs/nwordi)

      if (mode .le. 2) then

*        Allocate additional arrays needed for MPS input.
*        Currently just keynam -- the hash table.

         lkeynm = minz
         minmps = lkeynm + lenh
      end if

*     LP and reduced-gradient algorithm.
***** Beware -- length  0000  is used below for arrays that are
***** not needed in this version of MINOS.

      lascal = minz
      lpi2   = lascal + nscl
      lw     = lpi2   + 0000
      lw2    = lw     + nx
      lx     = lw2    + 0000
      lx2    = lx     + nx
      ly     = lx2    + nx2
      ly2    = ly     + nx
      lgsub  = ly2    + nx
      lgsub2 = lgsub  + nn
      lgrd   = lgsub2 + 0000
      lr     = lgrd   + mbs
      lrg    = lr     + nr
      lrg2   = lrg    + maxs
      minz   = lrg2   + maxs

*     Nonlinear objective.

      lgobj  = minz
      lgobj2 = lgobj  + nnobj
      minz   = lgobj2 + nnobj

*     Nonlinear constraints.
*     If Jacobian = Dense we can set njac correctly.
*     Otherwise we have to guess if the MPS file hasn't been
*     input yet (mode = 1 or 2).

      if (nden .eq. 1) then
         njac = nncon * nnjac
      else if (mode .le. 2) then
         njac =     5 * nnjac
      end if
      njac   = max( njac, 1 )
      lfcon  = minz
      lfcon2 = lfcon  + nncon
      lfdif  = lfcon2 + nncon
      lfdif2 = lfdif  + nncon
      lfold  = lfdif2 + 0000
      lblslk = lfold  + nncon
      lbuslk = lblslk + nncon
      lxlam  = lbuslk + nncon
      lrhs   = lxlam  + nncon
      lgcon  = lrhs   + nncon
      lgcon2 = lgcon  + njac
      lxdif  = lgcon2 + njac
      lxold  = lxdif  + nn
      minz   = lxold  + nn

*     Work arrays that can be overwritten by the names (see below)
*     in between cycles.

      lbbl   = minz
      lbbu   = lbbl   + mbs
      lgrd2  = lbbu   + mbs
      minz   = lgrd2  + mbs

*     Allocate arrays for the basis factorization routines.
*     minz points to the beginning of the LU factorization.
*     This is the beginning of free space between cycles
*     if the LU factors are allowed to be overwritten.

      call m2bmap( mode, m, n, ne, minz, maxz, nguess )

      if (mode .eq. 1) mincor = max( minmps, nguess, minz )
      if (mode .eq. 2) mincor = minmps
      if (mode .ge. 3) mincor = minz

*     end of m2core
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2amat( mode, m, n, nb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, hrtype )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hrtype(m)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb)

*     ------------------------------------------------------------------
*     If mode = 1 or 2,  m2amat defines hrtype, the set of row types.
*     If mode = 1,       m2amat also prints the matrix statistics.
*
*     The vector of row-types is as follows:
*        hrtype(i) = 0  for e rows (equalities),
*        hrtype(i) = 2  for n rows (objective or free rows),
*        hrtype(i) = 1  otherwise.
*     They are used in m2scal and m2crsh.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m8len / njac  ,nncon ,nncon0,nnjac

      intrinsic          abs, max, min
      parameter        ( zero = 0.0d+0 )

      bplus  = 0.9*plinfy
      bminus = - bplus

*     Construct the vector of row-types.

      nfixed = 0
      nfree  = 0
      nnorml = 0
      do 100 i  = 1, m
         j      = n + i
         b1     = bl(j)
         b2     = bu(j)
         itype  = 1
         if (b1 .eq. b2                          ) itype = 0
         if (b1 .le. bminus  .and.  b2 .ge. bplus) itype = 2
         hrtype(i) = itype
         if (itype .eq. 0) nfixed = nfixed + 1
         if (itype .eq. 2) nfree  = nfree  + 1
         if (itype .ne. 1) go to 100
         if (b1 .le. bminus  .or.   b2 .ge. bplus) nnorml = nnorml + 1
  100 continue

      if (mode   .eq. 2) return
      if (iprint .le. 0) return

      nbnded = m - nfixed - nfree - nnorml
      write(iprint, 2200)
      write(iprint, 2300) ' Rows   ', m, nnorml, nfree, nfixed, nbnded

      nfixed = 0
      nfree  = 0
      nnorml = 0

      do 200 j  = 1, n
         b1     = bl(j)
         b2     = bu(j)
         if (b1 .eq. b2) then
             nfixed = nfixed + 1
         else
             if      (b1 .eq. zero  ) then
                if      (b2 .ge. bplus) then
                    nnorml = nnorml + 1
                end if
             else if (b1 .le. bminus) then
                if      (b2 .eq. zero ) then
                    nnorml = nnorml + 1
                else if (b2 .ge. bplus) then
                    nfree  = nfree  + 1
                end if
             end if
         end if
  200 continue

      nbnded = n - nfixed - nfree - nnorml
      write(iprint, 2300) ' Columns', n, nnorml, nfree, nfixed, nbnded

*     Find the biggest and smallest elements in a, excluding free rows
*     and fixed columns.
*     Also find the largest objective coefficients in non-fixed columns.

      aijmax = zero
      aijmin = bplus
      cmax   = zero
      cmin   = bplus
      ncost  = 0

      do 300 j = 1, n
         if (bl(j) .lt. bu(j)) then
            do 250 k = ka(j), ka(j+1) - 1
               i     = ha(k)
               if (hrtype(i) .eq. 2) then
                   if (i .eq. iobj) then
                       aij   = abs( a(k) )
                       if (aij .gt. zero) then
                           ncost  = ncost + 1
                           cmax   = max( cmax, aij )
                           cmin   = min( cmin, aij )
                       end if
                   end if
               else
                   aij    = abs( a(k) )
                   aijmax = max( aijmax, aij )
                   aijmin = min( aijmin, aij )
               end if
  250       continue
         end if
  300 continue

      if (aijmin .eq. bplus) aijmin = zero
      adnsty = 100.0*ne / (m*n)
      write(iprint, 2400) ne, adnsty, aijmax, aijmin, ncost
      if (ncost .gt. 0) write(iprint, 2410) cmax, cmin

*     Print a few things to be gathered as statistics
*     from a bunch of test runs.

      write(iprint, 2500) nncon, m-nncon, n
      return

 2200 format(///
     $   ' Matrix Statistics' /
     $   ' -----------------' /
     $   15x, 'Total', 6x, 'Normal', 8x, 'Free',  7x, 'Fixed',
     $    5x, 'Bounded')
 2300 format(a, 5i12)
 2400 format(/ ' No. of matrix elements', i21, 5x, 'Density', f12.3
     $       / ' Biggest ', 1p, e35.4, '  (excluding fixed columns,'
     $       / ' Smallest',     e35.4, '   free rows, and RHS)'
     $      // ' No. of objective coefficients', i14)
 2410 format(  ' Biggest ', 1p, e35.4, '  (excluding fixed columns)'
     $       / ' Smallest',     e35.4)
 2500 format(//' Number of Nonlinear constraints', i12
     $       / ' Number of Linear    constraints', i12
     $       / ' Number of Variables            ', i12)

*     end of m2amat
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2aprd( mode, x, lenx, y, leny,
     $                   ne, nka, a, ha, ka,
     $                   z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne)
      double precision   x(lenx), y(leny), z(nwcore)

*     ------------------------------------------------------------------
*     m2aprd computes various products involving the matrix  A.
*     For mode 1 to 4,  x is the basic and/or superbasic variables.
*     For mode 5 to 8,  x is xn, the variables in natural order.
*
*     mode   function                       lenx  leny  called by
*     ----   --------                       ----  ----  ---------
*       1    y  =  y - B*x                  m     m     not used
*       1    y  =  y - (B S)*x              ms    m     not used
*       2    y  =  y - S*x                  ns    m     m7rgit
*       3    y  =  y - B(t)*x               m     m     not used
*       3    y  =  y - (B S)(t)*x           m     ms    not used
*       4    y  =  y - S(t)*x               m     ms    m7chzq, m7rg
*
*       5    y  =  y - A*xn                 n     m     m5setx, m5solv
*       6                                               not used
*       7    y  =  y - (nonlinear  A)*xn    nnjac nncon m6fun1, m8setj
*       8    y  =  y - (linear     A)*xn    n     nncon m8viol
*     mode 7 and 8 deal with nonlinear rows only and ignore slacks.
*
*     14 Oct 1991: a, ha, ka added as parameters to help with minoss.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m3len / m     ,n     ,nb    ,nscl
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m8len / njac  ,nncon ,nncon0,nnjac

      tolz   = eps0

      if (mode .lt. 5) then
         call m2apr1( mode, m, mbs, n, tolz,
     $                ne, nka, a, ha, ka, z(lkb),
     $                x, lenx, y, leny )
      else
         call m2apr5( mode, m, n, nb, nncon, nnjac, tolz,
     $                ne, nka, a, ha, ka,
     $                x, lenx, y, leny )
      end if

*     end of m2aprd
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2apr1( mode, m, mbs, n, tolz,
     $                   ne, nka, a, ha, ka, kb, x, lenx, y, leny )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka), kb(mbs)
      double precision   a(ne)
      double precision   x(lenx), y(leny)

*     ------------------------------------------------------------------
*     m2apr1  computes various matrix-vector products involving
*     B  and  S,  the basic and superbasic columns of  A.
*
*     mode=1    y  =  y - B*x   or   y - (B S)*x  (depending on  lenx)
*     mode=2    y  =  y - S*x
*     mode=3    y  =  y - B(transpose)*x   or   y - (B S)(transpose)*x
*    (mode 3  is not used as yet)
*     mode=4    y  =  y - S(transpose)*x
*     ------------------------------------------------------------------

      kbase  = 0
      if (mode .le. 2) then
*        -------------
*        mode 1 and 2.
*        -------------
         if (mode .eq. 2) kbase = m

         do 200 k = 1, lenx
            xj    = x(k)
            if (abs(xj) .le. tolz) go to 200
            j     = kb(kbase + k)
            if (j .le. n) then
               do 150 i = ka(j), ka(j+1) - 1
                  ir    = ha(i)
                  y(ir) = y(ir) - a(i)*xj
  150          continue
            else
               ir    = j - n
               y(ir) = y(ir) - xj
            end if
  200    continue
      else
*        -------------
*        mode 3 and 4.
*        -------------
         if (mode .eq. 4) kbase = m

         do 400 k = 1, leny
            t     = y(k)
            j     = kb(kbase + k)
            if (j .le. n) then
               do 350 i = ka(j), ka(j+1) - 1
                  ir    = ha(i)
                  t     = t - x(ir)*a(i)
  350          continue
               y(k) = t
            else
               ir   = j - n
               y(k) = t - x(ir)
            end if
  400    continue
      end if

*     end of m2apr1
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2apr5( mode, m, n, nb, nncon, nnjac, tolz,
     $                   ne, nka, a, ha, ka, xn, lenx, y, leny )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne)
      double precision   xn(lenx), y(leny)

*     ------------------------------------------------------------------
*     m2apr5  computes products involving parts of  A  and  xn.
*     ------------------------------------------------------------------

      intrinsic          abs

      if (mode .eq. 5) then
*        ----------------------------------
*        mode = 5.   Set  y = y - A*xn.
*        ----------------------------------
         do 500 j = 1, n
            xj    = xn(j)
            if (abs( xj ) .gt. tolz) then
               do 450 k = ka(j), ka(j+1) - 1
                  ir    = ha(k)
                  y(ir) = y(ir) - a(k)*xj
  450          continue
            end if
  500    continue

      else
*        ------------------------------------------------------------
*        If mode = 7, set  y  =  y - (Jacobian)*xn.
*        If mode = 8, set  y  =  y - (linear A)*xn.
*        Only the first nncon rows are involved, and no slacks.
*        ------------------------------------------------------------
         if (mode .eq. 7) then
            j1 = 1
            j2 = nnjac
         else
            j1 = nnjac + 1
            j2 = n
         end if

         do 800 j = j1, j2
            xj    = xn(j)
            if (abs( xj ) .gt. tolz) then
               do 750 k = ka(j), ka(j+1) - 1
                  ir    = ha(k)
                  if (ir .le. nncon) y(ir) = y(ir) - a(k)*xj
  750          continue
            end if
  800    continue
      end if

*     end of m2apr5
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2crsh( lcrash, m     , n     , nb    , nn    ,
     $                   ne    , nka   , a     , ha    , ka    ,
     $                   hpiv  , hs    , hrtype, bl    , bu    ,
     $                   xn    , z     , nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hpiv(m), hs(nb), hrtype(m)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb), xn(nb), z(nwcore )

*     ------------------------------------------------------------------
*     m2crsh  looks for a basis in the columns of ( A  I ).
*
*     ON ENTRY
*
*     iparm(1)  = Crash option    (icrash)   has been used by m4getb
*                 to set lcrash.
*     dparm(5)  = Crash tolerance (tcrash).  Default = 0.1
*
*     lcrash      specifies the action to be taken by Crash.
*        0,1,2,3  The call is from m4getb.
*        4        The call is from m5solv.
*        5        The call is from m8setj.
*
*        0        The all-slack basis is set up.
*
*        1        A triangular crash is applied to the columns of A.
*                 hs(1:n) is used to help select columns.
*                 tcrash is used to ignore small entries in each column.
*                 Depending on the size of tcrash, the resulting basis
*                 will be nearly (but not strictly) lower triangular.
*
*        2        As for 1, but nonlinear rows are ignored.
*
*        3        As for 2, but linear LG rows are also ignored.
*
*        4        Linear LG rows are now included.
*                 All hs(1:nb) and xn(n+i) are defined.
*                 Slack values of xn(n+i) are used to select LG rows.
*
*        5        Nonlinear rows are now included.
*
*     hrtype(*)   should be defined as described in m2amat:
*     hrtype(i) = 0  for E rows      (equalities)
*     hrtype(i) = 1  for L or G rows (inequalities)
*     hrtype(i) = 2  for N rows      (objective or free rows)
*
*     xn          If lcrash <= 4, xn(1:n) is used to initialize
*                 slacks as xn(n+1:nb) = - A*xn.
*                 Used to select slacks from LG rows to be in B (basis).
*                 If lcrash  = 5, xn(n+1:n+nncon) contains slack values
*                 evaluated from xn(1:n) and fcon(*).
*                 Used to select slacks from nonlinear rows to be in B.
*
*     hs          If lcrash = 1, 2 or 3, hs(1:n)  is used.
*                 If lcrash =    4 or 5, hs(1:nb) is used.
*                 If hs(j) =  0, 1 or 3, column j is eligible for B,
*                                        with 3 being "preferred".
*                 If hs(j) =  2, 4 or 5, column j is ignored.
*
*
*     Crash has several stages.
*
*     Stage 1: Insert any slacks (N, L or G rows, hrtype = 1 or 2).
*
*     Stage 2: Do triangular crash on any free columns (wide bounds)
*
*     Stage 3: Do triangular crash on "preferred" columns (hs(j) < 0).
*              For the linear crash, this includes variables set
*              between their bounds in the MPS file via FR INITIAL.
*              For the nonlinear crash, it includes nonbasics
*              between their bounds.
*              (That is, "pegged" variables in both cases.)
*
*     Stage 4: Grab unit columns.
*
*     Stage 5: Grab double columns.
*
*     Stage 6: Do triangular crash on all columns.
*
*     Slacks are then used to pad the basis.
*
*
*     ON EXIT
*
*     hs          is set to denote an initial (B S N) partition.
*                 hs(j) = 3 denotes variables for the initial basis.
*                 If hs(j) = 2 still, variable j will be superbasic.
*                 If hs(j) = 4 or 5 still, it will be changed to 0 or 1
*                 by m4chek and variable j will be nonbasic.
*
*     xn          If lcrash <= 4, slacks xn(n+1:nb) are initialized.
*
*     ------------------------------------------------------------------
*        Nov 1986: Essentially the same as in 1976.
*                  Crash tolerance added.
*                  Attention paid to various hs values.
*
*     12 Nov 1988: After free rows and columns have been processed
*                  (stage 1 and 2), slacks on L or G rows are inserted
*                  if their rows have not yet been assigned.
*
*     28 Dec 1988: Refined as follows.
*                  Stage 1 inserts free and preferred rows (slacks).
*                  Stage 2 performs a triangular crash on free or
*                          preferred columns, ignoring free rows.
*                          Unpivoted L or G slacks are then inserted.
*                  Stage 3 performs a triangular crash on the other
*                          columns, ignoring rows whose slack is basic.
*                          (Such rows form the top part of U.  The
*                          remaining rows form the beginning of L.)
*
*     30 Apr 1989: Stage 1 now also looks for singleton columns
*                  (ignoring free and preferred rows).
*     05 May 1989: Stage 2 doesn't insert slacks if crash option < 0.
*
*     06 Dec 1989: Stage 2, 3, 4 modified.  Columns of length 2 are
*                  now treated specially.
*
*     20 Dec 1989: Stage 2 thru 5 modified.  Free columns done before
*                  unit and double columns.
*
*     19 May 1992: xn now used to help initialize slacks.
*                  Stage 1 thru 7 redefined as above.
*
*     01 Jun 1992: abs used to define closeness of slacks to bounds.
*                  Unfortunately, xn(1:n) seldom has meaningful values.
*
*     02 Jun 1992: Poor performance on the larger problems.
*                  Reverted to simple approach: all slacks grabbed.
*
*     04 Jun 1992: Compromise -- Crash 3 now has 3 phases:
*                  (a) E rows.
*                  (b) LG rows.
*                  (c) Nonlinear rows.
*                  xn(1:n) should then define the slack values better
*                  for (b) and (c).
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2parm/ dparm(30),iparm(30)
      common    /m5log1/ idebug,ierr,lprint
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m8len / njac  ,nncon ,nncon0,nnjac

      equivalence      ( iparm(1), icrash )
      equivalence      ( dparm(5), tcrash )

      intrinsic          abs, max
      double precision   apiv(2)
      integer            ipiv(2)
      parameter        ( nstage = 6 )
      integer            num(nstage), stage
      logical            free, prefer, gotslk,
     $                   stage2, stage3, stage4, stage5
      parameter        ( zero  = 0.0d+0,
     $                   small = 1.0d-3,  big = 1.0d+4 )

      if (iprint .gt. 0) then
         if (lcrash .le. 3) write(iprint, 1000) icrash
         if (lcrash .eq. 3  .and.  nncon .lt. m) write(iprint, 1030)
         if (lcrash .eq. 4) write(iprint, 1040)
         if (lcrash .eq. 5) write(iprint, 1050)
      end if

      if (lcrash .le. 4) then

*        Sets slacks xn(n+1:nb) = - A*xn.
*        This is where the slacks are initialized.
*        They may be altered later (see the end of Crash).

         call dload ( m,  zero, xn(n+1), 1 )
         call m2aprd( 5, xn, n, xn(n+1), m,
     $                ne, nka, a, ha, ka,
     $                z, nwcore )
      end if

*     ------------------------------------------------------------------
*     For Crash option 0, set hs(j) = 3 for all slacks and quit.
*     ------------------------------------------------------------------
      if (lcrash .eq. 0) then
         call hload ( n, 0, hs     , 1 )
         call hload ( m, 3, hs(n+1), 1 )
         go to 900
      end if

*     ------------------------------------------------------------------
*     Crash option 1, 2 or 3.   lcrash = 1, 2, 3, 4, or 5.
*     tolslk measures closeness of slacks to bounds.
*     i1,i2  are the first and last rows of A involved in Stage 1.
*     ------------------------------------------------------------------
      tolslk = 0.25
      call iload ( nstage, 0, num, 1 )

      if (lcrash .le. 3) then
*        ---------------------------------------------------------------
*        First call.   lcrash = 1, 2 or 3.
*        Initialize hpiv(*) for all rows and hs(*) for all slacks.
*        ---------------------------------------------------------------
         i1     = 1
         if (lcrash .ge. 2) i1 = nncon + 1
         i2     = m
         nrows  = i2 - i1 + 1

*        Make sure there are no basic columns already (hs(j) = 3).
*        If there are, make them "preferred".

         do 20 j = 1, n
            if (hs(j) .eq. 3) hs(j) = -1
   20    continue

*        Make relevant rows available:  hpiv(i) = 1, hs(n+i) = 0.

         if (nrows .gt. 0) then
            call hload ( nrows, 1, hpiv(i1), 1 )
            call hload ( nrows, 0, hs(n+i1), 1 )
         end if

         if (lcrash .eq. 1) then
            nbasic = 0
         else

*           lcrash = 2 or 3:  Insert nonlinear slacks.

            nbasic = nncon
            if (nncon .gt. 0) then
               call hload ( nncon, 3, hpiv   , 1 )
               call hload ( nncon, 3, hs(n+1), 1 )
            end if
         end if

         if (lcrash .eq. 3) then

*           Insert linear inequality slacks (including free rows).

            do 25 i = i1, m
               if (hrtype(i) .ge. 1) then
                  nbasic  = nbasic + 1
                  nrows   = nrows  - 1
                  hpiv(i) = 3
                  hs(n+i) = 3
               end if
   25       continue
         end if

*        We're done if there are no relevant rows.

         if (nrows .eq. 0) go to 800

      else
*        ---------------------------------------------------------------
*        Second or third call.  lcrash = 4 or 5.
*        Initialize hpiv(*) for all rows.
*        hs(*) already defines a basis for the full problem,
*        but we want to do better by including only some of the slacks.
*        ---------------------------------------------------------------
         if (lcrash .eq. 4) then
*           ------------------------------------------------------------
*           Crash on linear LG rows.
*           ------------------------------------------------------------
            if (nncon .eq. m) go to 900
            i1     = nncon + 1
            i2     = m

*           Mark nonlinear rows as pivoted: hpiv(i) = 3.

            nbasic = nncon
            if (nbasic .gt. 0) then
               call hload ( nbasic, 3, hpiv, 1 )
            end if

*           Mark linear E  rows as pivoted: hpiv(i) = 3
*           Make linear LG rows available:  hpiv(i) = 1, hs(n+i) = 0.

            do 30 i = i1, m
               if (hrtype(i) .eq. 0) then
                  nbasic  = nbasic + 1
                  hpiv(i) = 3
               else
                  hpiv(i) = 1
                  hs(n+i) = 0
               end if
   30       continue

*           Mark linear LG rows with hpiv(i) = 2
*           if any basic columns contain a nonzero in row i.

            do 50 j = 1, n
               if (hs(j) .eq. 3) then
                  do 40 k = ka(j), ka(j+1) - 1
                     i    = ha(k)
                     if (hrtype(i) .eq. 1) then
                        if (i .gt. nncon) then
                           if (a(k) .ne. zero) hpiv(i) = 2
                        end if
                     end if
   40             continue
               end if
   50       continue

         else
*           ------------------------------------------------------------
*           lcrash = 5.  Crash on nonlinear rows.
*           ------------------------------------------------------------
            i1     = 1
            i2     = nncon

*           Mark all linear rows as pivoted: hpiv(i) = 3

            nbasic = m - nncon
            if (nbasic .gt. 0) then
               call hload ( nbasic, 3, hpiv(nncon+1), 1 )
            end if

*           Make nonlinear rows available:  hpiv(i) = 1, hs(n+i) = 0.

            call hload ( nncon, 1, hpiv   , 1 )
            call hload ( nncon, 0, hs(n+1), 1 )

*           Mark nonlinear rows with hpiv(i) = 2
*           if any basic columns contain a nonzero in row i.

            do 70 j = 1, n
               if (hs(j) .eq. 3) then
                  do 60 k = ka(j), ka(j+1) - 1
                     i    = ha(k)
                     if (i .le. nncon) then
                        if (a(k) .ne. zero) hpiv(i) = 2
                     end if
   60             continue
               end if
   70       continue
         end if
      end if

*     ------------------------------------------------------------------
*     Stage 1: Insert relevant slacks (N, L or G rows, hrtype = 1 or 2).
*              If lcrash = 4 or 5, grab them only if they are more than
*              tolslk from their bound.
*     ------------------------------------------------------------------
      stage  = 1
      gotslk = lcrash .eq. 4  .or.  lcrash .eq. 5

      do 100 i = i1, i2
         j     = n + i
         if (hs(j) .le. 1  .and.  hrtype(i) .gt. 0) then
            if (gotslk) then
               d1 = xn(j) - bl(j)
               d2 = bu(j) - xn(j)
               if (min( d1, d2 ) .le. tolslk) then
              
*                 The slack is close to a bound or infeasible.
*                 Move it exactly onto the bound.
              
                  if (d1 .le. d2) then
                     xn(j) = bl(j)
                     hs(j) = 0
                  else
                     xn(j) = bu(j)
                     hs(j) = 1
                  end if
                  go to 100
               end if
            end if

            nbasic     = nbasic     + 1
            num(stage) = num(stage) + 1
            hpiv(i)    = 3
            hs(j)      = 3
         end if
  100 continue

      if (nbasic .eq. m) go to 700

*     ------------------------------------------------------------------
*     Apply a triangular crash to various subsets of the columns of A.
*
*        hpiv(i) = 1  if row i is unmarked (initial state).
*        hpiv(i) = 3  if row i has been given a pivot
*                     in one of a set of triangular columns.
*        hpiv(i) = 2  if one of the triangular columns contains
*                     a nonzero in row i below the triangle.
*     ------------------------------------------------------------------

      do 600 stage = 2, nstage
         stage2    = stage .eq. 2
         stage3    = stage .eq. 3
         stage4    = stage .eq. 4
         stage5    = stage .eq. 5

*        ---------------------------------------------------------------
*        Main loop for triangular crash.
*        ---------------------------------------------------------------
         do 200  j = 1, n
            js     = hs(j)
            if (js    .gt.    1 ) go to 200
            if (bl(j) .eq. bu(j)) go to 200

            if ( stage2 ) then
               free   = bl(j) .le. - big  .and.  bu(j) .ge. big
               if ( .not. free  ) go to 200

            else if ( stage3 ) then
               prefer = js .lt. 0
               if ( .not. prefer) go to 200
            end if

*           Find the biggest aij, ignoring free rows.

            k1     = ka(j)
            k2     = ka(j+1) - 1
            aimax  = zero

            do 130  k = k1, k2
               i      = ha(k)
               if (hrtype(i) .ne. 2) then
                  ai    = a(k)
                  aimax = max( aimax, abs( ai ) )
               end if
  130       continue

*           Prevent small pivots if crash tol is too small.

            if (aimax .le. small) go to 200

*           Find the biggest pivots in rows that are still
*           unpivoted and unmarked.  Ignore smallish elements.
*           nz counts the number of relevant nonzeros.

            aitol   = aimax * tcrash
            nz      = 0
            npiv    = 0
            ipiv(1) = 0
            ipiv(2) = 0
            apiv(1) = zero
            apiv(2) = zero

            do 150 k = k1, k2
               i     = ha(k)
               if (hs(n+i) .ne. 3) then
                  ai = abs( a(k) )
                  if (ai .gt. aitol) then
                     nz = nz + 1
                     ip = hpiv(i)
                     if (ip .le. 2) then
                        if (apiv(ip) .lt. ai) then
                            apiv(ip) = ai
                            ipiv(ip) = i
                        end if
                     else
                        npiv = npiv + 1
                     end if
                  end if
               end if
  150       continue

*           Grab unit or double columns.

            if      ( stage4 ) then
               if (nz .ne. 1) go to 200
            else if ( stage5 ) then
               if (nz .ne. 2) go to 200
            end if

*           See if the column contained a potential pivot.
*           An unmarked row is favored over a marked row.

            ip     = 1
            if (ipiv(1)  .eq.  0  .and.  npiv .eq. 0) ip = 2
            i      = ipiv(ip)

            if (i .gt. 0) then
               nbasic     = nbasic     + 1
               num(stage) = num(stage) + 1
               hpiv(i)    = 3
               hs(j)      = 3
               if (nbasic .ge. m) go to 700

*              Mark off all relevant unmarked rows.

               do 180 k = k1, k2
                  i     = ha(k)
                  if (hs(n+i) .ne. 3) then
                     ai = abs( a(k) )
                     if (ai .gt. aitol) then
                        if (hpiv(i) .eq. 1) hpiv(i) = 2
                     end if
                  end if
  180          continue
            end if
  200    continue
  600 continue

*     ------------------------------------------------------------------
*     All stages finished.
*     Fill remaining gaps with slacks.
*     ------------------------------------------------------------------
  700 npad   = m - nbasic
      if (iprint .gt. 0) write(iprint, 1200) num, npad

      if (npad .gt. 0) then
         do 720 i = 1, m
            if (hpiv(i) .lt. 3) then
               nbasic  = nbasic + 1
               hs(n+i) = 3
               if (nbasic .ge. m) go to 800
            end if
  720    continue
      end if

*     ------------------------------------------------------------------
*     Make sure there aren't lots of nonbasic slacks floating off
*     their bounds.  They could take lots of iterations to move.
*     ------------------------------------------------------------------
  800 do 850 i = i1, i2
         j     = n + i
         if (hs(j) .le. 1  .and.  hrtype(i) .gt. 0) then
            d1 = xn(j) - bl(j)
            d2 = bu(j) - xn(j)
            if (min( d1, d2 ) .le. tolslk) then
              
*              The slack is close to a bound or infeasible.
*              Move it exactly onto the bound.
              
               if (d1 .le. d2) then
                  xn(j) = bl(j)
                  hs(j) = 0
               else
                  xn(j) = bu(j)
                  hs(j) = 1
               end if
            end if
         end if
  850 continue

  900 return

 1000 format(// ' Crash option', i3)
 1030 format(   ' Crash on linear E  rows:')
 1040 format(   ' Crash on linear LG rows:')
 1050 format(   ' Crash on nonlinear rows:')
 1200 format(
     $   ' Slacks', i6, '  Free cols', i6, '  Preferred', i6
     $ / ' Unit  ', i6, '  Double   ', i6, '  Triangle ', i6,
     $     '  Pad', i6)

*     end of m2crsh
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2scal( m, n, nb, ne, nka, nn, nncon, nnjac,
     $                   hrtype, ha, ka, a, ascale, bl, bu, rmin, rmax )

      implicit           double precision (a-h,o-z)
      integer*4          hrtype(m), ha(ne)
      integer            ka(nka)
      double precision   a(ne), ascale(nb), bl(nb), bu(nb)
      double precision   rmin(m), rmax(m)

*     ------------------------------------------------------------------
*     m2scal computes scale factors ascale from A, bl, bu.
*
*     In phase 1, an iterative procedure based on geometric means is
*     used to compute scales from a alone.  This procedure is derived
*     from a routine written by Robert Fourer, 1979.  The main steps
*     are:
*
*        (1) Compute aratio = max(i1,i2,j)  a(i1,j) / a(i2,j).
*        (2) Divide each row i by
*               ( min(j) a(i,j) * max(j) a(i,j) ) ** 1/2.
*        (3) Divide each column j by
*               ( min(i) a(i,j) * max(i) a(i,j) ) ** 1/2.
*        (4) Compute sratio as in (1).
*        (5) If sratio .lt. scltol * aratio, repeat from step (1).
*
*        Free rows (hrtype=2) and fixed columns (bl=bu) are not used
*        at this stage.
*
*     In phase 2, the scales for free rows are set to be their largest
*     element.
*
*     In phase 3, fixed columns are summed in order to compute
*     a scale factor sigma that allows for the effective rhs of the
*     constraints.  All scales are then multiplied by sigma.
*
*
*     If lscale = 1, the first nncon rows and the first nn columns will
*     retain scales of 1.0 during phases 1-2, and phase 3 will not be
*     performed.  (This takes effect if the problem is nonlinear but
*     the user has specified scale linear variables only.)
*     However, all rows    contribute to the linear column scales,
*     and      all columns contribute to the linear row    scales.
*
*     If lscale = 2, all rows and columns are scaled.  To guard against
*     misleadingly small Jacobians, if the maximum element in any of
*     the first nncon rows or the first nnjac columns is less than
*     smallj, the corresponding row or column retains a scale of 1.0.
*
*     Initial version  March 1981.
*     Feb 1983.  For convenience, everything is now double-precision.
*     Aug 1983.  Revised to damp the effect of very small elements.
*                On each pass, a new row or column scale will not be
*                smaller than  sqrt(damp)  times the largest (scaled)
*                element in that row or column.
*     Oct 1986.  Scale option 2 and phase 3 implemented.
*     Nov 1989.  Large scales are now reduced to at most 0.1/tolx (say)
*                to help prevent obvious infeasibilities when the
*                problem is unscaled.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5log1/ idebug,ierr,lprint
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm

      intrinsic          abs, min, max, sqrt

      double precision   ac, ar, amin, amax, aratio, bnd, b1, b2,
     $                   cmin, cmax, cratio, damp, sigma,
     $                   small, smallj, sratio

      parameter        ( zero = 0.0d+0,  one    = 1.0d+0,
     $                   damp = 1.0d-4,  smallj = 1.0d-2 )

      if (iprint .gt.  0 ) write(iprint, 1000)
      if (scltol .ge. one) scltol = 0.99
      bplus  = 0.1*plinfy
      aratio = bplus
      maxk   = 11
      if (lscale .eq. 1) then
*        Do linear rows and columns only.
         istart = nncon + 1
         jstart = nn    + 1
      else
*        Do all rows and cols if lscale .ge. 2
         istart = 1
         jstart = 1
      end if

      do 50 j = 1, nb
         ascale(j) = one
   50 continue

      if (jstart .gt. n) go to 900

*     ------------------------------------------------------------------
*     Main loop for phase 1.
*     Only the following row-types are used:
*        hrtype(i) = 2       for n rows (objective or free rows),
*        hrtype(i) = 0 or 1  otherwise.
*     ------------------------------------------------------------------

      do 400 kk = 1, maxk

*        Find the largest column ratio.
*        Also set new column scales (except on pass 0).

         npass  = kk - 1
         amin   = bplus
         amax   = zero
         small  = smallj
         sratio = one

         do 250 j = jstart, n
            if (bl(j) .lt. bu(j)) then
               cmin  = bplus
               cmax  = zero

               do 230  k = ka(j), ka(j+1) - 1
                  i      = ha(k)
                  if (hrtype(i) .eq. 2   ) go to 230
                  ar     = abs( a(k) )
                  if (ar        .eq. zero) go to 230
                  ar     = ar / ascale(n+i)
                  cmin   = min( cmin, ar )
                  cmax   = max( cmax, ar )
  230          continue

               ac     = max( cmin, damp*cmax )
               ac     = sqrt( ac ) * sqrt( cmax )
               if (j     .gt. nnjac) small     = zero
               if (cmax  .le. small) ac        = one
               if (npass .gt. 0    ) ascale(j) = ac
               amin   = min( amin, cmin / ascale(j) )
               amax   = max( amax, cmax / ascale(j) )
               cratio = cmax / cmin
               sratio = max( sratio, cratio )
            end if
  250    continue

         if (iprint .gt. 0) then
            write(iprint, 1200) npass, amin, amax, sratio
         end if
         if (npass  .ge. 3       .and.
     $       sratio .ge. aratio*scltol) go to 420
         if (kk     .eq. maxk         ) go to 420
         aratio = sratio


*        Set new row scales for the next pass.

         if (istart .le. m) then
            do 300  i  = istart, m
               rmin(i) = bplus
               rmax(i) = zero
  300       continue

            do 350 j = 1, n
               if (bl(j) .lt. bu(j)) then
                  ac = ascale(j)

                  do 330  k  = ka(j), ka(j+1) - 1
                     i       = ha(k)
                     if (i  .lt. istart) go to 330
                     ar      = abs( a(k) )
                     if (ar .eq. zero  ) go to 330
                     ar      = ar / ac
                     rmin(i) = min( rmin(i), ar )
                     rmax(i) = max( rmax(i), ar )
  330             continue
               end if
  350       continue

            do 360 i = istart, m
               j     = n + i
               ar    = rmax(i)
               if (i .le. nncon  .and.  ar .le. smallj) then
                  ascale(j) = one
               else
                  ac        = max( rmin(i), damp*ar )
                  ascale(j) = sqrt( ac ) * sqrt( ar )
               end if
  360       continue
         end if
  400 continue

*     ------------------------------------------------------------------
*     End of main loop.
*     ------------------------------------------------------------------

*     Invert the column scales, so that structurals and logicals
*     can be treated the same way during subsequent unscaling.
*     Find the min and max column scales while we're at it.
*     Nov 1989: nclose counts how many are "close" to 1.
*     For problems that are already well-scaled, it seemed sensible to
*     set the "close" ones exactly equal to 1.
*     Tried "close" = (0.5,2.0) and (0.9,1.1), but they helped only
*     occasionally.  Have decided not to interfree.

  420 amin   = bplus
      amax   = zero
      close1 = 0.5
      close2 = 2.0
      nclose = 0


      do 430 j = 1, n
         ac    = one / ascale(j)

         if (amin .gt. ac) then
             amin   =  ac
             jmin   =  j
         end if
         if (amax .lt. ac) then
             amax   =  ac
             jmax   =  j
         end if

         if (ac .gt. close1  .and.  ac .lt. close2) then
             nclose =  nclose + 1
*----        ac     =  one
         end if

         ascale(j)  = ac
  430 continue

*     Remember, column scales are upside down.

      amax   = one / amax
      amin   = one / amin
      k      = max( 1, n - jstart + 1 )
      percnt = nclose * 100.0 / k
      if (iprint .gt. 0) then
         write(iprint, 1300)
         write(iprint, 1310) 'Col', jmax, amax, 'Col', jmin, amin,
     $                       nclose, percnt
      end if

*     ------------------------------------------------------------------
*     Phase 2.  Deal with empty rows and free rows.
*     Find the min and max row scales while we're at it.
*     ------------------------------------------------------------------
      amin   = bplus
      amax   = zero
      imin   = 0
      imax   = 0
      nclose = 0

      do 440 i = istart, m
         j     = n + i

         if (hrtype(i) .eq. 2) then
            ar = rmax(i)
            if (ar .eq. zero) ar = one
         else
            ar = ascale(j)
            if (ar .eq. zero) ar = one

            if (amin .gt. ar) then
                amin  =   ar
                imin  =   i
            end if
            if (amax .lt. ar) then
                amax  =   ar
                imax  =   i
            end if

            if (ar .gt. close1  .and.  ar .lt. close2) then
                nclose =  nclose + 1
*----           ar     =  one
            end if
         end if

         ascale(j) = ar
  440 continue

      if (imin .eq. 0) then
          amin   = zero
          amax   = zero
      end if
      k      = max( 1, m - istart + 1 )
      percnt = nclose * 100.0 / k
      if (iprint .gt. 0) then
         write(iprint, 1310) 'Row', imin, amin, 'Row', imax, amax,
     $                       nclose, percnt
      end if

*     ------------------------------------------------------------------
*     Phase 3.
*     Compute what is effectively the rhs for the constraints.
*     We set  rmax  =  ( A  I ) * x  for fixed columns and slacks,
*     including positive lower bounds and negative upper bounds.
*     ------------------------------------------------------------------
      if (lscale .ge. 2) then
         call dload ( m, zero, rmax, 1 )

         do 500 j = 1, nb
            bnd   = zero
            b1    = bl(j)
            b2    = bu(j)
            if (b1  .eq. b2  ) bnd = b1
            if (b1  .gt. zero) bnd = b1
            if (b2  .lt. zero) bnd = b2
            if (bnd .eq. zero) go to 500

            if (j   .le. n   ) then
               do 480 k = ka(j), ka(j+1) - 1
                  i       = ha(k)
                  rmax(i) = rmax(i)  +  a(k) * bnd
  480          continue
            else
*              Free slacks never get here,
*              so we don't have to skip them.
               i       = j - n
               rmax(i) = rmax(i)  +  bnd
            end if
  500    continue

*        We don't want nonzeros in free rows to interfere.

         do 520 i = 1, m
            if (hrtype(i) .eq. 2) rmax(i) = zero
  520    continue

*        Scale rmax = rmax / (row scales),  and use its norm sigma
*        to adjust both row and column scales.

         ac     = dnorm1( m, rmax, 1 )
         call dddiv ( m, ascale(n+1), 1, rmax, 1 )
         sigma  = dnorm1( m, rmax, 1 )
         if (iprint .gt. 0) write(iprint, 1400) ac, sigma
         sigma  = max( sigma, one )
         call dscal ( nb, sigma, ascale, 1 )

*        Big scales might lead to excessive infeasibility when the
*        problem is unscaled.  If any are too big, scale them down.

         amax  = zero
         do 540 j = 1, n
            amax  = max( amax, ascale(j) )
  540    continue

         do 550 i = 1, m
            if (hrtype(i) .ne. 2) then
               amax  = max( amax, ascale(n + i) )
            end if
  550    continue

         big    = 0.1 / tolx
         sigma  = big / amax
         if (sigma .lt. one) then
            call dscal ( nb, sigma, ascale, 1 )
            if (iprint .gt. 0) write(iprint, 1410) sigma
         end if
      end if

      if (iparm(4) .gt. 0  .and.  iprint .gt. 0) then
        if (istart .le. m) write(iprint,1500) (i,ascale(n+i),i=istart,m)
        if (jstart .le. n) write(iprint,1600) (j,ascale(j  ),j=jstart,n)
      end if

  900 return

 1000 format(// ' Scaling' / ' -------'
     $        / '             Min elem    Max elem       Max col ratio')
 1200 format(   ' After', i3, 1p, e12.2, e12.2, 0p, f20.2)
 1300 format(/  12x, 'Min scale', 23x, 'Max scale', 6x,
     $          'Between 0.5 and 2.0')
 1310 format(1x, a, i7, 1p, e10.1, 12x, a, i7, e10.1, i17, 0p, f8.1)
 1400 format(/  ' Norm of fixed columns and slacks', 1p, e20.1
     $       /  ' (before and after row scaling)  ', 1p, e20.1)
 1410 format(   ' Scales are large --- reduced by ', 1p, e20.1)
 1500 format(// ' Row scales  r(i)',
     $      8x, ' a(i,j)  =   r(i)  *  scaled a(i,j)  /  c(j)'
     $        / ' ----------------'    // 5(i6, g16.5))
 1600 format(// ' Column scales  c(j)',
     $      5x, ' x(j)    =   c(j)  *  scaled x(j)'
     $        / ' -------------------' // 5(i6, g16.5))

*     end of m2scal
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2scla( mode, m, n, nb, ne, nka,
     $                   ha, ka, a, ascale, bl, bu, pi, xn )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), ascale(nb), bl(nb), bu(nb)
      double precision   pi(m), xn(nb)

*     ------------------------------------------------------------------
*     m2scla scales or unscales a, bl, bu, pi, xn using ascale.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj

      parameter        ( one = 1.0d+0 )

      bplus  = 0.1*plinfy
      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        mode = 1  ---  scale a, bl, bu, xn and pi.
*        ---------------------------------------------------------------
         do 150 j = 1, nb
            scale = ascale(j)
            if (j .le. n) then
               do 110 k = ka(j), ka(j+1) - 1
                  i     = ha(k)
                  a(k)  = a(k) * ( scale / ascale(n+i) )
  110          continue
            end if
            xn(j) = xn(j) / scale
            if (bl(j) .gt. -bplus) bl(j) = bl(j) / scale
            if (bu(j) .lt.  bplus) bu(j) = bu(j) / scale
  150    continue

         call ddscl ( m, ascale(n+1), 1, pi, 1 )
         if (jobj .gt. 0) sclobj = ascale(jobj)
      else
*        ---------------------------------------------------------------
*        mode = 2  ---  unscale everything.
*        ---------------------------------------------------------------
         do 250 j = 1, nb
            scale = ascale(j)
            if (j .le. n) then
               do 210 k = ka(j), ka(j+1) - 1
                  i     = ha(k)
                  a(k)  = a(k) * ( ascale(n+i) / scale )
  210          continue
            end if
            xn(j) = xn(j) * scale
            if (bl(j) .gt. -bplus) bl(j) = bl(j) * scale
            if (bu(j) .lt.  bplus) bu(j) = bu(j) * scale
  250    continue

         call dddiv ( m, ascale(n+1), 1, pi, 1 )
         sclobj = one
      end if

*     end of m2scla
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m2unpk( jq, m, n, ne, nka, a, ha, ka, y )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne)
      double precision   y(m)

*     ------------------------------------------------------------------
*     m2unpk  expands the jq-th column of  ( A  I )  into  y.
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      call dload ( m, zero, y, 1 )

      if (jq .le. n) then
         do 10 k = ka(jq), ka(jq+1) - 1
            i    = ha(k)
            y(i) = a(k)
   10    continue
      else
         islack    = jq - n
         y(islack) = one
      end if

*     end of m2unpk
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine matcol( m, n, nb, ne, nka,
     $                   a, ha, ka, bl, bu, col, ztol )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb), col(m)

*     ------------------------------------------------------------------
*     matcol  creates a new matrix column from the dense vector  col.
*     It becomes column number  jnew,  which is updated accordingly.
*     Elements as small as the zero tolerance  ztol  are not retained.
*     Default bounds are given to the new variable.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /cyclcm/ cnvtol,jnew,materr,maxcy,nephnt,nphant,nprint

      intrinsic          abs

      if (jnew .ge. n) go to 900
      jnew   = jnew + 1
      ia     = ka(jnew)

      do 100 i  = 1, m
         if (abs( col(i) )  .le.  ztol) go to 100
         if (ia .gt. ne) go to 910
         a (ia) = col(i)
         ha(ia) = i
         ia     = ia + 1
  100 continue

      if (ia .eq. ka(jnew)) go to 920
      bl(jnew) = bstruc(1)
      bu(jnew) = bstruc(2)
      ka(jnew + 1) = ia
      return

*     Too many columns.

  900 if (iprint .gt. 0) write(iprint, 1000)
      if (isumm  .gt. 0) write(isumm , 1000)
      go to 990

*     Too many elements.

  910 if (iprint .gt. 0) write(iprint, 1100)
      if (isumm  .gt. 0) write(isumm , 1100)
      go to 990

*     Zero column.

  920 if (iprint .gt. 0) write(iprint, 1200)
      if (isumm  .gt. 0) write(isumm , 1200)

*     Error exit.

  990 materr = materr + 1
      return

 1000 format(/ ' XXX  MATCOL  error.  Not enough Phantom columns.')
 1100 format(/ ' XXX  MATCOL  error.  Not enough Phantom elements.')
 1200 format(/ ' XXX  MATCOL  error.  New column of  A  was zero.')

*     end of matcol
      end
