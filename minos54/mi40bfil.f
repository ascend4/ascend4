************************************************************************
*
*     File  mi40bfil fortran.
*
*     m4getb   m4chek   m4id     m4name   m4inst   m4load   m4oldb
*     m4savb   m4dump   m4newb   m4pnch   m4rc     m4infs
*     m4rept   m4soln   m4solp   m4stat
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4getb( ncycle, istart, m, mbs, n, nb, nn, nname, nscl,
     $                   lcrash, ns,
     $                   ne, nka, a, ha, ka,
     $                   hrtype, hs, kb, ascale, bl, bu,
     $                   pi, xn, y, y2, name1, name2, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hrtype(mbs), hs(nb)
      integer            ka(nka), kb(mbs), name1(nname), name2(nname)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   pi(m), xn(nb), y(m), y2(m), z(nwcore)

*     ------------------------------------------------------------------
*     m4getb is called with ncycle = 0 before the first cycle.
*     A basis file is loaded (if any exists).
*
*     m4getb is called with ncycle > 0 before every cycle.
*     The Jacobian is evaluated and stored in a (unscaled).
*     If gotscl is false, m2scal is called to compute scales.
*     If relevant, the scales are applied to a, bl, bu, xn, pi, fcon.
*     If gotbas is false, m2crsh is called to initialize hs.
*     (kb, y, y2  are used as workspace by m2crsh.)
*
*     In both cases, lcrash is an output parameter to tell m5solv
*     if further calls to crash are needed.
*
*     14 May 1992: pi(1:nncon) assumed to be initialized on entry.
*                  xn passed to m2crsh.
*     04 Jun 1992: lcrash added as an output parameter.
*     09 Jul 1992: istart added as an input parameter.
*                  Used when ncycle = 0 to load a basis file only if
*                  it is a cold start.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      logical            gotbas,gotfac,gothes,gotscl
      common    /cycle1/ gotbas,gotfac,gothes,gotscl

      intrinsic          max, min
      equivalence      ( iparm(1), icrash )
      logical            gotjac
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      lcrash = 0

      if (ncycle .eq. 0) then
*        ---------------------------------------------------------------
*        This call is made before the cycle loop.
*        ---------------------------------------------------------------

*        Initialize  lvldif = 1, nfcon(*) = 0, nfobj(*) = 0.
*        Also set linear pi(i) = 0 in case they are printed.
*        We have to do this before scales are applied anyway.
*        One day the basis files might load nonlinear pi(i).

         lvldif = 1
         call iload1( 4, 0, nfcon, 1 )
         call iload1( 4, 0, nfobj, 1 )
         if (nncon .lt. m) call dload ( m-nncon, zero, pi(nncon+1), 1 )

*        Load a basis file if one exists and istart = 0 (Cold start).

         if (istart .eq. 0) then
            if      (ioldb .gt. 0) then
               call m4oldb( m, n, nb, ns, hs, bl, bu, xn )
            else if (insrt .gt. 0) then
               call m4inst( m, n, nb, ns, hs, bl, bu, xn, name1, name2 )
            else if (iload .gt. 0) then
               call m4load( m, n, nb, ns, hs, bl, bu, xn, name1, name2 )
            end if
            nssave = ns
         end if

*        Make sure the nonlinear variables are within bounds
*        (for the gradient checkers if nothing else).

         do 160 j = 1, nn
            xn(j) = max( xn(j), bl(j) )
            xn(j) = min( xn(j), bu(j) )
  160    continue

      else
*        ---------------------------------------------------------------
*        ncycle > 0.  This call is made every cycle.
*        ---------------------------------------------------------------
         if (nncon .gt. 0) then

*           Evaluate the Jacobian at the initial  xn
*           and store it in  a(*)  for the first linearization.
*           We may have to copy some constant (unscaled) Jacobian
*           elements from gcon2 into gcon.
*           Also, we have to disable scaling temporarily, since if this
*           is the first cycle, the scales have not yet been computed.

            if (lscale .eq. 2  .and.  lderiv .ge. 2) then
               call dcopy ( njac, z(lgcon2), 1, z(lgcon), 1 )
            end if
            lssave = lscale
            lscale = 0
            gotjac = .false.
            call m8ajac( gotjac, nncon, nnjac, njac,
     $                   ne, nka, a, ha, ka,
     $                   z(lfcon), z(lfcon2), z(lgcon), z(lgcon2),
     $                   xn, y, z, nwcore )
            lscale = lssave
            if (ierr .ne. 0) go to 900
         end if

*        Compute scales from  a, bl, bu  (unless we already have them).
*        Then apply them to   a, bl, bu, pi, xn and fcon.

         if (lscale .gt. 0) then
            if (.not. gotscl) then
               gotscl = .true.
               call m2scal( m, n, nb, ne, nka, nn, nncon, nnjac,
     $                      hrtype, ha, ka, a, ascale, bl, bu, y, y2 )
            end if

            call m2scla( 1, m, n, nb, ne, nka,
     $                   ha, ka, a, ascale, bl, bu, pi, xn )

            if (lscale .eq. 2  .and.  nncon .gt. 0) then
               call dddiv ( nncon, ascale(n+1), 1, z(lfcon), 1 )
            end if
         end if

*        Initialize xlam from pi.  These are Lagrange multipliers
*        for the nonlinear constraints.
*        Do it at the very start, or on later cycles if the present
*        solution is feasible.  (ninf = 0 in both cases.
*        Keep the previous xlam if solution is infeasible.)
*        First change the sign of pi if maximizing.
*
*        If pi(1:nncon) seems ridiculously big, assume that it
*        has not been correctly initialized and just use xlam = 0.
                          
         if (ninf .eq. 0  .and.  nncon .gt. 0) then
            if (minimz .lt. 0) then
               call dscal ( nncon, -one, pi, 1 )
            end if
            toobig = 1.0d+10
            imax   = idamax( nncon, pi, 1 )
            xlmax  = abs( pi(imax) )
            if (xlmax .lt. toobig) then
               call dcopy ( nncon, pi, 1, z(lxlam), 1 )
            else
               call dload ( nncon,  zero, z(lxlam), 1 )
               if (iprint .gt. 0) write(iprint, 1000)
               if (isumm  .gt. 0) write(isumm , 1000)
            end if
         end if

*        ---------------------------------------------------------------
*        If there was no basis file, find an initial basis via Crash.
*        This works best with A scaled (hence much of the complication).
*        xn(1:n) is input.  xn(n+1:nb) is initialized by m2crsh.
*        ---------------------------------------------------------------
         if (.not. gotbas) then
            gotbas = .true.
            if (icrash .gt. 0  .and.  icrash .le. 3) lcrash = icrash

            call m2crsh( lcrash, m, n, nb, nn,
     $                   ne, nka, a, ha, ka,
     $                   kb, hs, hrtype, bl, bu, xn, z, nwcore )
         end if
      end if

*     Exit.

  900 return

 1000 format(' XXX  pi(1:nncon) is big  (perhaps not initialized).'
     $     / ' XXX  Will set lambda = 0 for nonlinear rows.')
*     end of m4getb
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4chek( m, maxs, mbs, n, nb, ns,
     $                   hs, kb, bl, bu, xn )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            kb(mbs)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m4chek  takes hs and xn and checks they contain reasonable values.
*     The entries hs(j) = 2 are used to set ns and nssave and possibly
*     the list of superbasic variables kb(m+1) thru kb(m+ns).
*     Scaling, if any, has taken place by this stage.
*
*     If gotbas and gothes are both true, nssave and the superbasic kb's
*     are assumed to be set.  It must be a Hot start, or ncycle > 1.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      logical            gotbas,gotfac,gothes,gotscl
      common    /cycle1/ gotbas,gotfac,gothes,gotscl

      intrinsic          abs, max, min
      logical            setkb
      parameter        ( zero = 0.0d+0,  tolb = 1.0d-4 )

*     Make sure hs(j) = 0, 1, 2 or 3 only.

      do 5 j = 1, nb
         js  = hs(j)
         if (js .lt. 0) hs(j) = 0
         if (js .ge. 4) hs(j) = js - 4
    5 continue

      setkb  = .not. (gotbas .and. gothes)

*     ------------------------------------------------------------------
*     Make sure the objective is basic and free.
*     Then count the basics and superbasics, making sure they don't
*     exceed m and maxs respectively.  Also, set ns and possibly
*     kb(m+1) thru kb(m+ns) to define the list of superbasics.
*     Mar 1988: Loop 100 now goes backwards to make sure we grab obj.
*     Apr 1992: Backwards seems a bit silly in the documentation.
*               We now go forward through the slacks,
*               then forward through the columns.
*     ------------------------------------------------------------------
   10 nbasic = 0
      ns     = 0
      if (iobj .gt. 0) then
          jobj     =   n + iobj
          hs(jobj) =   3
          bl(jobj) = - plinfy
          bu(jobj) =   plinfy
      end if

*     If too many basics or superbasics, make them nonbasic.
*     Do slacks first to make sure we grab the objective slack.

      j = n

      do 100 jj = 1, nb
         j      = j + 1
         if (j .gt. nb) j = 1
         js     = hs(j)
         if (js .eq. 2) then
            ns     = ns + 1
            if (ns .le. maxs) then
               if ( setkb ) kb(m + ns) = j
            else
               hs(j) = 0
            end if

         else if (js .eq. 3) then
            nbasic = nbasic + 1
            if (nbasic .gt. m) hs(j) = 0
         end if
  100 continue

*     Proceed if the superbasic kbs were reset, or if ns seems to
*     agree with nssave from the previous cycle.
*     Otherwise, give up trying to save the projected Hessian, and
*     reset the superbasic kbs after all.

      if (setkb) then
*        ok
      else if (ns .ne. nssave) then
         setkb  = .true.
         gothes = .false.
         if (iprint .gt. 0) write(iprint, 1000) ns, nssave
         if (isumm  .gt. 0) write(isumm , 1000) ns, nssave
         go to 10
      end if

*     Check the number of basics.

      ns     = min( ns, maxs )
      nssave = ns
      if (nbasic .ne. m ) then
         gothes = .false.
         if (iprint .gt. 0) write(iprint, 1100) nbasic, m
         if (isumm  .gt. 0) write(isumm , 1100) nbasic, m
      end if

*     -----------------------------------------------------------
*     On all cycles, set each nonbasic xn(j) to be exactly on its
*     nearest bound if it is within tolb of that bound.
*     -----------------------------------------------------------
      bplus  = 0.1*plinfy
      do 300 j = 1, nb
         xj    = xn(j)
         if (abs( xj ) .ge.  bplus) xj = zero
         if (hs(j)     .le.  1    ) then
            b1    = bl(j)
            b2    = bu(j)
            xj    = max( xj, b1 )
            xj    = min( xj, b2 )
            if ((xj - b1) .gt. (b2 - xj)) b1 = b2
            if (abs(xj - b1)  .le.  tolb) xj = b1
            hs(j) = 0
            if (xj .gt. bl(j)) hs(j) = 1
         end if
         xn(j) = xj
  300 continue

      return

 1000 format(/ ' WARNING:', i6, ' superbasics in hs(*);',
     $         ' previously ns =', i6, '.  Hessian not saved')
 1100 format(/ ' WARNING:', i7, ' basics specified;',
     $         ' preferably should have been', i7)

*     end of m4chek
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4id  ( j, m, n, nb, nname, name1, name2, id1, id2 )

      integer            name1(nname), name2(nname)

*     ------------------------------------------------------------------
*     m4id   returns a name id1-id2 for the j-th variable.
*     If nname = nb, the name is already in name1, name2.
*     Otherwise nname = 1. Some generic column or row name is cooked up
*     ------------------------------------------------------------------

      character*7        f1
      character*5        f2
      character*1        cname
      character*1        rname
      character*8        gname
      data               f1    /'(a1,i7)'/
      data               f2    /'(2a4)'/
      data               cname /'x'/
      data               rname /'r'/

      if (nname .eq. nb) then
         id1 = name1(j)
         id2 = name2(j)
      else if (j .le. n) then
         write(gname, f1) cname, j
         read (gname, f2) id1, id2
      else
         i   = j - n
         write(gname, f1) rname, i
         read (gname, f2) id1, id2
      end if

*     end of m4id
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4name( n, name1, name2, id1, id2,
     $                   ncard, notfnd, maxmsg, j1, j2, jmark, jfound )

      implicit           double precision (a-h,o-z)
      integer            name1(n), name2(n)

*     ------------------------------------------------------------------
*     m4name searches for name1-name2 in arrays name1-2(j), j = j1, j2.
*     jmark  will probably speed the search on the next entry.
*     Used by subroutines m3mpsc, m4inst, m4load.
*
*     Left-justified alphanumeric data is being tested for a match.
*     On Burroughs B6700-type systems, one could replace .eq. by .is.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      do 50 j = jmark, j2
         if (id1 .eq. name1(j)  .and.  id2 .eq. name2(j)) go to 100
   50 continue

      do 60 j = j1, jmark
         if (id1 .eq. name1(j)  .and.  id2 .eq. name2(j)) go to 100
   60 continue

*     Not found.

      jfound = 0
      jmark  = j1
      notfnd = notfnd + 1
      if (notfnd .le. maxmsg) then
         if (iprint .gt. 0) write(iprint, 1000) ncard, id1, id2
      end if
      return

*     Got it.

  100 jfound = j
      jmark  = j
      return

 1000 format(' XXX  Line', i6, '  --  name not found:', 8x, 2a4)

*     end of m4name
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4inst( m, n, nb, ns,
     $                   hs, bl, bu, xn, name1, name2 )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            name1(nb), name2(nb)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     This impression of INSERT reads a file produced by  m4pnch.
*     It is intended to read files similar to those produced by
*     standard MPS systems.  It recognizes SB as an additional key.
*     Also, values are extracted from columns 25--36.
*
*     17 May 1992: John Stone (Ketron) mentioned trouble if rows and
*                  columns have the same name.  The quick fix is to
*                  search column names from the beginning always,
*                  rather than from position jmark.  Just set jmark = 1.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj

      intrinsic          abs
      integer            id(5)
      character*4        key
      character*4        lll         , lul         , lxl         ,
     $                   lxu         , lsb         , lend
      data               lll /' LL '/, lul /' UL '/, lxl /' XL '/,
     $                   lxu /' XU '/, lsb /' SB '/, lend/'ENDA'/

      bplus  = 0.9*plinfy
      if (iprint .gt. 0) write(iprint, 1999) insrt
      if (isumm  .gt. 0) write(isumm , 1999) insrt
                         read (insrt , 1000) id
      if (iprint .gt. 0) write(iprint, 2000) id
      l1    = n + 1

*     Make logicals basic.

      do 20 j  = l1, nb
         hs(j) = 3
   20 continue

      ignord = 0
      nbs    = 0
      ns     = 0
      notfnd = 0
      ncard  = 0
*     jmark  = 1
      lmark  = l1
      ndum   = n + 100000

*     Read names until ENDATA

      do 300 nloop = 1, ndum
         read(insrt, 1020) key, name1c, name2c, name1r, name2r, xj
         if (key .eq. lend) go to 310

*        Look for name1.  It may be a column or a row,
*        since a superbasic variable could be either.
*        17 May 1992: Set jmark = 1 so columns are searched first.
*        This avoids trouble when columns and rows have the same name.

         ncard  = nloop
         jmark  = 1
         call m4name( nb, name1, name2, name1c, name2c,
     $                ncard, notfnd, mer, 1, nb, jmark, j )
         if (   j  .le. 0) go to 300
         if (hs(j) .gt. 1) go to 290

         if (key .eq. lxl  .or.  key .eq. lxu) then
*           ------------------------------------------------------------
*           XL, XU (exchange card) -- make col j basic, row l nonbasic.
*           ------------------------------------------------------------
                  
*           Look for name2.  It has to be a row.
         
            call m4name( nb, name1, name2, name1r, name2r,
     $                   ncard, notfnd, mer, l1, nb, lmark, l )
            if (l  .le.  0  ) go to 300
            if (l  .eq. jobj) go to 290
            if (hs(l) .ne. 3) go to 290

            nbs    = nbs + 1
            hs(j)  = 3
            if (key .eq. lxl) then
               hs(l) = 0
               if (bl(l) .gt. -bplus) xn(l) = bl(l)
            else
               hs(l) = 1
               if (bu(l) .lt.  bplus) xn(l) = bu(l)
            end if

*        ---------------------------------------------------------------
*        else LL, UL, SB  --  only  j  and  xj  are relevant.
*        ---------------------------------------------------------------
         else if (key .eq. lll) then
            hs(j) = 0
         else if (key .eq. lul) then
            hs(j) = 1
         else if (key .eq. lsb) then
            hs(j) = 2
            ns    = ns + 1
         else
            go to 290
         end if

*        Save xj.

         if (abs(xj) .lt. bplus) xn(j) = xj
         go to 300

*        Card ignored.

  290    ignord = ignord + 1
         if (iprint .gt. 0  .and.  ignord .le. mer) then
            write(iprint, 2010) ncard, key, name1c,name2c,name1r,name2r
         end if
  300 continue

  310 ignord = ignord + notfnd
      if (iprint .gt. 0) write(iprint, 2050) ncard, ignord, nbs, ns
      if (isumm  .gt. 0) write(isumm , 2050) ncard, ignord, nbs, ns
      if (insrt  .ne. iread) rewind insrt
      return

 1000 format(14x, 2a4, 2x, 3a4)
 1005 format(2a4)
 1020 format(3a4, 2x, 2a4, 2x, e12.5)
 1999 format(/ ' INSERT file to be input from file', i4)
 2000 format(/ ' NAME', 10x, 2a4, 2x, 3a4)
 2010 format(' XXX  Line', i6, '  ignored:', 8x, 3a4, 2x, 2a4)
 2050 format(/ ' No. of lines read      ', i6, '  Lines ignored', i6
     $       / ' No. of basics specified', i6, '  Superbasics  ', i6)

*     end of m4inst
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4load( m, n, nb, ns,
     $                   hs, bl, bu, xn, name1, name2 )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            name1(nb), name2(nb)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m4load  inputs a load file, which may contain a full or partial
*     list of row and column names and their states and values.
*     Valid keys are   BS, LL, UL, SB.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj

      intrinsic          abs
      integer            id(5)
      character*4        key
      character*4        lbs         , lll         , lul         ,
     $                   lsb         , lend
      data               lbs /' BS '/, lll /' LL '/, lul /' UL '/,
     $                   lsb /' SB '/, lend/'ENDA'/

      bplus  = 0.9*plinfy
      if (iprint .gt. 0) write(iprint, 1999) iload
      if (isumm  .gt. 0) write(isumm , 1999) iload
                         read (iload , 1000) id
      if (iprint .gt. 0) write(iprint, 2000) id
      l1     = n + 1
      ignord = 0
      nbs    = 0
      ns     = 0
      notfnd = 0
      ncard  = 0
      jmark  = 1
      ndum   = n + 100000

*     Read names until ENDATA is found.

      do 300 nloop = 1, ndum
         read (iload, 1020) key, id1, id2, xj
         if (key .eq. lend) go to 310

         ncard  = nloop
         call m4name( nb, name1, name2, id1, id2,
     $                ncard, notfnd, mer, 1, nb, jmark, j )
         if (j .le. 0) go to 300

*        The name id1-id2 belongs to the j-th variable.

         if (hs(j) .gt. 1) go to 290
         if (j   .eq.jobj) go to  90
         if (key .eq. lbs) go to  90
         if (key .eq. lll) go to 100
         if (key .eq. lul) go to 150
         if (key .eq. lsb) go to 200
         go to 290

*        Make basic.

   90    nbs    = nbs + 1
         hs(j)  = 3
         go to 250

*        LO or UP.

  100    hs(j)  = 0
         go to 250

  150    hs(j)  = 1
         go to 250

*        Make superbasic.

  200    ns     = ns + 1
         hs(j)  = 2

*        Save  x  values.

  250    if (abs(xj) .lt. bplus) xn(j) = xj
         go to 300

*        Card ignored.

  290    ignord = ignord + 1
         if (ignord .le. mer) then
            if (iprint .gt. 0) write(iprint, 2010) ncard, key, id1, id2
         end if
  300 continue

  310 ignord = ignord + notfnd
      if (iprint .gt. 0) write(iprint, 2050) ncard, ignord, nbs, ns
      if (isumm  .gt. 0) write(isumm , 2050) ncard, ignord, nbs, ns

*     Make sure the linear objective is basic.

      if (iobj  .gt. 0  .and.  hs(jobj) .ne. 3) then
         hs(jobj) = 3

*        Swap obj with last basic variable.

         do 850 j = nb, 1, -1
            if (hs(j) .eq. 3) go to 860
  850    continue

  860    hs(j)  = 0
      end if

      if (iload .ne. iread) rewind iload
      return

 1000 format(14x, 2a4, 2x, 3a4)
 1005 format(2a4)
 1020 format(3a4, 12x, e12.5)
 1999 format(/ ' LOAD file to be input from file', i4)
 2000 format(/ ' NAME', 10x, 2a4, 2x, 3a4)
 2010 format(' XXX  Line', i6, '  ignored:', 8x, 3a4, 2x, 2a4)
 2050 format(/ ' No. of lines read      ', i6, '  Lines ignored', i6
     $       / ' No. of basics specified', i6, '  Superbasics  ', i6)

*     end of m4load
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4oldb( m, n, nb, ns,
     $                   hs, bl, bu, xn )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m4oldb  inputs a compact basis file from file  ioldb.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m5log1/ idebug,ierr,lprint

      character*80       id

      bplus  = 0.9*plinfy
      if (iprint .gt. 0) write(iprint, 1999) ioldb
      if (isumm  .gt. 0) write(isumm , 1999) ioldb
         read (ioldb , 1000) id
      if (iprint .gt. 0) then
         write(iprint, 2000) id
      end if

         read (ioldb , 1005) id(1:52), newm, newn, ns
      if (iprint .gt. 0) then
         write(iprint, 2005) id(1:52), newm, newn, ns
      end if

      if (newm .ne. m  .or.  newn .ne. n) go to 900
      read (ioldb , 1010) hs

*     Set values for nonbasic variables.

      do 200 j = 1, nb
         js    = hs(j)
         if (js .le. 1) then
            if (js .eq. 0) xj = bl(j)
            if (js .eq. 1) xj = bu(j)
            if (abs(xj) .lt. bplus) xn(j) = xj
         end if
  200 continue

*     Load superbasics.

      ns     = 0
      ndummy = m + n + 10000

      do 300 idummy = 1, ndummy
         read(ioldb, 1020, end=310) j, xj
         if (j .le.  0) go to  310
         if (j .le. nb) then
            xn(j)  = xj
            if (hs(j) .eq. 2) ns = ns + 1
         end if
  300 continue

  310 if (ns .gt. 0) then
         if (iprint .gt. 0) write(iprint, 2010) ns
         if (isumm  .gt. 0) write(isumm , 2010) ns
      end if
      go to 990

*     Error exits.

  900 call m1page( 1 )
      if (iprint .gt. 0) write(iprint, 3000)
      if (isumm  .gt. 0) write(isumm , 3000)
      ierr   = 30

  990 if (ioldb .ne. iread) rewind ioldb
      return

 1000 format(a80)
 1005 format(a52, 2x, i7, 3x, i7, 4x, i5)
 1010 format(80i1)
 1020 format(i8, e24.14)
 1999 format(/ ' OLD BASIS file to be input from file', i4)
 2000 format(1x, a80)
 2005 format(1x, a52, 'M=', i7, ' N=', i7, ' SB=', i5)
 2010 format(' No. of superbasics loaded', i7)
 3000 format(' EXIT -- the basis file dimensions do not match',
     $   ' this problem')

*     end of m4oldb
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4savb( mode, m, mbs, n, nb, nn, nname, nscl, msoln,ns,
     $                   ne, nka, a, ha, ka,
     $                   hs, kb, ascale, bl, bu,
     $                   name1, name2, pi, rc, xn, y, z, nwcore )

      implicit           double precision (a-h,o-z)
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), kb(mbs), name1(nname), name2(nname)
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   pi(m), rc(nb), xn(nb), y(m), z(nwcore)

*     ------------------------------------------------------------------
*     m4savb  saves basis files  and/or  prints the solution.
*
*     If mode = 1, the problem is first unscaled, then from 0 to 4 files
*     are saved (PUNCH file, DUMP file, SOLUTION file, REPORT file,
*     in that order).
*     A new BASIS file, if any, will already have been saved by m5solv.
*     A call with mode = 1 must precede a call with mode = 2.
*
*     If mode = 2, the solution is printed under the control of msoln
*     (which is set by the Solution keyword in the SPECS file).
*
*     18 Nov 1991: Scaled pinorm saved for use in m4soln.
*     25 Nov 1991: rc added as parameter to return reduced costs.
*     31 Jan 1991: Call m4rc   to get the reduced costs.
*     18 Dec 1992: Maximum primal and dual infeasibilities computed
*                  and printed here.
*     ------------------------------------------------------------------

      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5inf / prinf, duinf, jprinf, jduinf
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave

      intrinsic          max
      character*12       istate
      logical            feasbl, prnt
      save               pnorm1, pnorm2
      parameter        ( one = 1.0d+0 )

      feasbl = ninf .eq. 0
      ms     = m + ns
      k      = ierr + 1
      call m4stat( k, istate )

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        mode = 1.
*        Compute rc and unscale everything.
*        Then save basis files.
*        ---------------------------------------------------------------

*        Compute reduced costs rc(*) for all columns and rows.
*        Find the maximum primal and dual infeasibilities.

         call m4rc  ( feasbl, featol, minimz,
     $                m, n, nb, nnobj, nnobj0,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, z(lgobj), pi, rc, xn )
         call m4infs( nb, jobj, bl, bu, rc, xn )

*        Unscale a, bl, bu, pi, xn, rc, fcon, gobj and xnorm, pinorm.
*        (m4soln uses scaled pinorm, so save it.)

         pnorm1 = pinorm
         xnorm1 = xnorm
         jpinf1 = jprinf
         jdinf1 = jduinf
         prinf1 = prinf
         duinf1 = duinf

         if (lscale .gt. 0) then
            call m2scla( 2, m, n, nb, ne, nka,
     $                   ha, ka, a, ascale, bl, bu, pi, xn )

            call dddiv ( nb, ascale, 1, rc, 1 )

            if (lscale .eq. 2) then
               if (nncon .gt. 0)
     $         call ddscl ( nncon, ascale(n+1), 1, z(lfcon), 1 )

               if (nnobj .gt. 0)
     $         call dddiv ( nnobj, ascale     , 1, z(lgobj), 1 )
            end if

*           Call m5setp to redefine pinorm.  y is not used.

            xnorm  = dnorm1( nb, xn, 1 )
            call m5setp( 3, m, y, pi, z, nwcore )
            call m4infs( nb, jobj, bl, bu, rc, xn )
         end if
         pnorm2 = pinorm

*        ---------------------------------------------------------------
*        Print various scaled and unscaled norms.
*        ---------------------------------------------------------------
         if (lscale .gt. 0) then
            if (iprint .gt. 0) write(iprint, 1010) xnorm1, pnorm1
            if (isumm  .gt. 0) write(isumm , 1010) xnorm1, pnorm1
         end if
            if (iprint .gt. 0) write(iprint, 1020) xnorm , pinorm
            if (isumm  .gt. 0) write(isumm , 1020) xnorm , pinorm
         if (lscale .gt. 0) then
            if (iprint .gt. 0) write(iprint, 1030) jpinf1, prinf1,
     $                                             jdinf1, duinf1
            if (isumm  .gt. 0) write(isumm , 1030) jpinf1, prinf1,
     $                                             jdinf1, duinf1
         end if
            if (iprint .gt. 0) write(iprint, 1040) jprinf, prinf,
     $                                             jduinf, duinf
            if (isumm  .gt. 0) write(isumm , 1040) jprinf, prinf,
     $                                             jduinf, duinf

*        Change the sign of pi and rc if feasible and maximizing.

         if (ninf .eq. 0  .and.  minimz .lt. 0) then
            call dscal ( m , -one, pi, 1 )
            call dscal ( nb, -one, rc, 1 )
         end if

*        Compute nonlinear constraint violations.

         if (nncon .gt. 0) then
            call m8viol( n, nb, nncon,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, z(lfcon), xn, y, z, nwcore )
            if (iprint .gt. 0) write(iprint, 1080) vimax, virel
            if (isumm  .gt. 0) write(isumm , 1080) vimax, virel
         end if

*        ---------------------------------------------------------------
*        Output PUNCH, DUMP, SOLUTION and/or REPORT files.
*        ---------------------------------------------------------------
         if (ipnch .gt. 0)
     $   call m4pnch( ipnch, n, nb, hs, bl, bu, xn,
     $                name1, name2 )

         if (idump .gt. 0)
     $   call m4dump( idump, n, nb, hs, bl, bu, xn,
     $                name1, name2 )

         pinorm = pnorm1
         if (isoln .gt. 0)
     $      call m4soln( .true., m, n, nb, nname, nscl,
     $                nn, nnobj, nnobj0, ns,
     $                ne, nka, a, ha, ka,
     $                hs, ascale, bl, bu,
     $                z(lgobj), pi, rc, xn, y,
     $                name1, name2, istate, z, nwcore )

         if (ireprt .gt. 0)
     $   call m4rept( .true., m, n, nb, nname, nscl,
     $                nn, nnobj, nnobj0, ns,
     $                ne, nka, a, ha, ka,
     $                hs, ascale, bl, bu,
     $                z(lgobj), pi, rc, xn, y,
     $                name1, name2, istate, z, nwcore )
         pinorm = pnorm2
      else
*        ---------------------------------------------------------------
*        mode = 2.    Print solution if requested.
*
*        msoln  = 0   means   no
*               = 1   means   if optimal, infeasible or unbounded
*               = 2   means   yes
*               = 3   means   if error condition
*        ---------------------------------------------------------------
         prnt   = iprint .gt. 0  .and.  msoln .gt. 0
         if ((msoln .eq. 1  .and.  ierr .gt. 2)  .or.
     $       (msoln .eq. 3  .and.  ierr .le. 2)) prnt = .false.
         if ( prnt ) then
            pinorm = pnorm1
            call m4soln( .false., m, n, nb, nname, nscl,
     $                   nn, nnobj, nnobj0, ns,
     $                   ne, nka, a, ha, ka,
     $                   hs, ascale, bl, bu,
     $                   z(lgobj), pi, rc, xn, y,
     $                   name1, name2, istate, z, nwcore )
            pinorm = pnorm2
            if (isumm  .gt. 0) write(isumm, 1200) iprint

         else if (.not. (GAMS .or. AMPL)) then
            if (isumm  .gt. 0) write(isumm, 1300)
         end if
      end if

      return

 1010 format(  ' Norm of x  (scaled)  ', 1p, e16.3,
     $     2x, ' Norm of pi (scaled)  ',     e16.3)
 1020 format(  ' Norm of x ', 1p, e27.3,
     $     2x, ' Norm of pi',     e27.3)
 1030 format(  ' Primal inf (scaled)', i7, 1p, e11.1,
     $     2x, ' Dual inf   (scaled)', i7,     e11.1)
 1040 format(  ' Primal infeas      ', i7, 1p, e11.1,
     $     2x, ' Dual infeas        ', i7,     e11.1)
 1080 format(  ' Constraint violation ', 1p, e16.3,
     $     2x, ' Normalized           ',     e16.3)
 1100 format(2a4)
 1200 format(/ ' Solution printed on file', i4)
 1300 format(/ ' Solution not printed')

*     end of m4savb
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4dump( idump, n, nb, hs, bl, bu, xn, name1, name2 )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            name1(nb), name2(nb)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m4dump outputs basis names in a format compatible with m4load.
*     This file is normally easier to modify than a punch file.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax

      character*4        key(4)
      data               key   /' LL ', ' UL ', ' SB ', ' BS '/

      write(idump, 2000) name

      do 500 j = 1, nb
         k     = hs(j) + 1
         write(idump, 2100) key(k), name1(j), name2(j), xn(j)
  500 continue

      write(idump , 2200)
      if (iprint .gt. 0) write(iprint, 3000) idump
      if (isumm  .gt. 0) write(isumm , 3000) idump
      if (idump .ne. iprint) rewind idump
      return

 2000 format('NAME', 10x, 2a4, 2x, '   DUMP/LOAD')
 2100 format(3a4, 12x, 1p, e12.5)
 2200 format('ENDATA')
 3000 format(/ ' DUMP file saved on file', i4)

*     end of m4dump
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4newb( mode, inewb, m, n, nb, nn, ns, ms, nscl, fsub,
     $                   kb, hs, ascale, bl, bu, x, xn, istate )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            kb(ms)
      double precision   ascale(nscl), bl(nb), bu(nb)
      double precision   x(ms), xn(nb)
      character*12       istate

*     ------------------------------------------------------------------
*     m4newb  saves a compact basis on file inewb.  Called from m5solv.
*     If mode = 1, the save is a periodic one due to the save frequency.
*     If mode = 2, m5solv has just finished the current problem.
*                 (Hence, mode 2 calls occur at the end of every cycle.)
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m8len / njac  ,nncon ,nncon0,nnjac

      logical            scaled

      scaled = lscale .gt. 0
      obj    = sinf
      if (ninf .eq. 0) obj = minimz * fsub

*     Output header cards and the state vector.

      write(inewb, 1000) name, itn, istate, ninf, obj
      write(inewb, 1005) mobj, mrhs, mrng, mbnd, m, n, ns
      write(inewb, 1010) hs

*     Output the superbasic variables.

      do 50 k = m + 1, ms
         j    = kb(k)
         xj   = x(k)
         if (scaled) xj = xj * ascale(j)
         write(inewb, 1020) j, xj
   50 continue

*     If there are nonlinear constraints,
*     output the values of all other (non-sb) nonlinear variables.

      do 100 j = 1, nnjac
         if (hs(j) .ne. 2) then
            xj    = xn(j)
            if (scaled) xj = xj * ascale(j)
            write(inewb, 1020) j, xj
         end if
  100 continue

*     Output nonbasic variables that are not at a bound.
*     Ignore ones that the EXPAND anti-cycling procedure
*     has put slightly outside their bound.

      do 250 j = nnjac + 1, nb
         if (hs(j) .le. 1 ) then
            xj    = xn(j)
            if (xj .le. bl(j)) go to 250
            if (xj .ge. bu(j)) go to 250
            if (scaled) xj = xj * ascale(j)
            write(inewb, 1020) j, xj
         end if
  250 continue

*     Terminate the list with a zero.

      j     = 0
      write(inewb, 1020) j
      if (inewb .ne. iprint) rewind inewb
      if (iprint .gt. 0) write(iprint, 1030) inewb, itn
      if (isumm  .gt. 0) write(isumm , 1030) inewb, itn
      return

 1000 format(2a4, '  ITN', i8, 4x, a12, '  NINF', i7,
     $       '      OBJ', 1p, e21.12)
 1005 format('OBJ=', 2a4, ' RHS=', 2a4, ' RNG=', 2a4, ' BND=', 2a4,
     $       ' M=', i7,  ' N=', i7, ' SB=', i5)
 1010 format(80i1)
 1020 format(i8, 1p, e24.14)
 1030 format(/ ' NEW BASIS file saved on file', i4, '    itn =', i7)

*     end of m4newb
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4pnch( ipnch, n, nb, hs, bl, bu, xn, name1, name2 )

      implicit           double precision (a-h,o-z)
      integer*4          hs(nb)
      integer            name1(nb), name2(nb)
      double precision   bl(nb), bu(nb)
      double precision   xn(nb)

*     ------------------------------------------------------------------
*     m4pnch  outputs a PUNCH file (list of basis names, states and
*     values) in a format that is compatible with MPS/360.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax

      parameter        ( zero = 0.0d+0 )
      character*4        key(5), ibl
      data               key    /' LL ', ' UL ', ' SB ', ' XL ', ' XU '/
      data               ibl    /'    '/

      write(ipnch, 2000) name
      irow   = n

      do 500  j = 1, n
         id1    = name1(j)
         id2    = name2(j)
         k      = hs(j)

         if (k .eq. 3) then

*           Basics -- find the next row that isn't basic.

  300       irow   = irow + 1
            if (irow .le. nb) then
               k      = hs(irow)
               if (k .eq. 3) go to 300

               if (k .eq. 2) k = 0
               write(ipnch, 2100) key(k+4), id1, id2,
     $                            name1(irow), name2(irow), xn(j)
            end if
         else

*           Skip nonbasic variables with zero lower bounds.

            if (k .le. 1) then
               if (bl(j) .eq. zero  .and.  xn(j) .eq. zero) go to 500
            end if
            write(ipnch, 2100) key(k+1), id1, id2, ibl, ibl, xn(j)
         end if
  500 continue

*     Output superbasic slacks.

      do 700 j = n + 1, nb
         if (hs(j) .eq. 2)
     $   write(ipnch, 2100) key(3), name1(j), name2(j), ibl, ibl, xn(j)
  700 continue

      write(ipnch , 2200)
      if (iprint .gt. 0) write(iprint, 3000) ipnch
      if (isumm  .gt. 0) write(isumm , 3000) ipnch
      if (ipnch .ne. iprint) rewind ipnch
      return

 2000 format('NAME', 10x, 2a4, 2x, 'PUNCH/INSERT')
 2100 format(3a4, 2x, 2a4, 2x, 1p, e12.5)
 2200 format('ENDATA')
 3000 format(/ ' PUNCH file saved on file', i4)

*     end of m4pnch
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4rc  ( feasbl, featol, minimz,
     $                   m, n, nb, nnobj, nnobj0,
     $                   ne, nka, a, ha, ka,
     $                   hs, bl, bu, gobj, pi, rc, xn )

      implicit           double precision (a-h,o-z)
      logical            feasbl
      integer*4          ha(ne), hs(nb)
      integer            ka(nka)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   gobj(nnobj0), pi(m), rc(nb), xn(nb)

*     ------------------------------------------------------------------
*     m4rc   computes reduced costs rc(*) for all columns of ( A  I ).
*     If xn is feasible, the true nonlinear objective gradient gobj(*)
*     is used (not the gradient of the augmented Lagrangian).
*     Otherwise, the Phase-1 objective is included.
*
*     m4rc   is called by m4savb BEFORE unscaling.
*     External values of hs(*) are used (0, 1, 2, 3),
*     but internal would be ok too since we only test for > 1.
*
*     31 Jan 1992: First version.
*     ------------------------------------------------------------------

      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )

      do 300 j = 1, n
         dj    = zero
         do 200 k = ka(j), ka(j+1) - 1
            i     = ha(k)
            dj    = dj  +  pi(i) * a(k)
  200    continue
         rc(j) = - dj
  300 continue

      do 320 i = 1, m
         rc(n+i) = - pi(i)
  320 continue

      if (feasbl  .and.  nnobj .gt. 0) then

*        Include the nonlinear objective gradient.

         sgnobj = minimz
         call daxpy ( nnobj, sgnobj, gobj, 1, rc, 1 )
      else

*        Include the Phase 1 objective.
*        Only basics and superbasics can be infeasible.

         do 500 j = 1, nb
            if (hs(j) .gt. 1) then
               d1 = bl(j) - xn(j)
               d2 = xn(j) - bu(j)
               if (d1 .gt. featol) rc(j) = rc(j) - one
               if (d2 .gt. featol) rc(j) = rc(j) + one
            end if
  500    continue
      end if

*     end of m4rc
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4infs( nb, jobj, bl, bu, rc, xn )

      implicit           double precision (a-h,o-z)
      double precision   bl(nb), bu(nb), rc(nb), xn(nb)

*     ------------------------------------------------------------------
*     m4infs computes the maximum primal and dual infeasibilities,
*     using bl, bu, rc and xn.
*
*     m4infs is called by m4savb before and after unscaling.
*
*     18 Dec 1992: First version.
*                  prinf , duinf  return the max primal and dual infeas.
*                  jprinf, jduinf return the corresponding column nos.
*     ------------------------------------------------------------------

      common    /m5inf / prinf, duinf, jprinf, jduinf
      parameter        ( zero = 0.0d+0 )

      jprinf = 0
      jduinf = 0
      prinf  = zero
      duinf  = zero
      
      do 500 j = 1, nb
         d1  = bl(j) - xn(j)
         d2  = xn(j) - bu(j)
         if (prinf .lt. d1) then
             prinf  =   d1
             jprinf =   j
         end if
         if (prinf .lt. d2) then
             prinf  =   d2
             jprinf =   j
         end if

         if (   j  .eq. jobj ) go to 500
         if (bl(j) .eq. bu(j)) go to 500

         dj     = rc(j)
         if      (d1 .ge. zero) then
            dj  = - dj
         else if (d2 .le. zero) then
*           dj  = + dj
         else
            dj  = abs( dj )
         end if
         
         if (duinf .lt. dj) then
             duinf   =  dj
             jduinf  =  j
         end if
  500 continue

*     end of m4infs
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4rept( ondisk, m, n, nb, nname, nscl,
     $                   nn, nnobj, nnobj0, ns,
     $                   ne, nka, a, ha, ka,
     $                   hs, ascale, bl, bu, gobj, pi, rc, xn, y,
     $                   name1, name2, istate, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            ondisk
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), name1(nname), name2(nname)
      character*12       istate
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   gobj(nnobj0), pi(m), rc(nb), xn(nb), y(m),
     $                   z(nwcore)

*     ------------------------------------------------------------------
*     m4rept  has the same parameter list as m4soln, the routine that
*     prints the solution.  It will be called if the SPECS file
*     specifies  REPORT file  n  for some positive value of  n.
*
*     pi contains the unscaled dual solution.
*     xn contains the unscaled primal solution.  There are n + m = nb
*        values (n structural variables and m slacks, in that order).
*     y  contains the true slack values for nonlinear constraints
*        in its first nncon components (computed by m8viol).
*
*     This version of m4rept does nothing.    Added for PILOT, Oct 1985.
*     31 Oct 1991: Name changed from "report" to "m4rept".
*                  Parameters altered to allow for MPS or generic names.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /cycle2/ objtru,suminf,numinf
*     ------------------------------------------------------------------

      if (iprint .gt. 0) write(iprint, 1000)
      if (isumm  .gt. 0) write(isumm , 1000)
      return

 1000 format(/ ' XXX Report file requested.  m4rept does nothing.')

*     end of m4rept
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4soln( ondisk, m, n, nb, nname, nscl,
     $                   nn, nnobj, nnobj0, ns,
     $                   ne, nka, a, ha, ka,
     $                   hs, ascale, bl, bu, gobj, pi, rc, xn, y,
     $                   name1, name2, istate, z, nwcore )

      implicit           double precision (a-h,o-z)
      logical            ondisk
      integer*4          ha(ne), hs(nb)
      integer            ka(nka), name1(nname), name2(nname)
      character*12       istate
      double precision   a(ne), ascale(nscl), bl(nb), bu(nb)
      double precision   gobj(nnobj0), pi(m), rc(nb), xn(nb), y(m),
     $                   z(nwcore)

*     ------------------------------------------------------------------
*     m4soln  is the standard output routine for printing the solution.
*
*     On entry,
*     pi    contains the dual solution.
*     xn    contains the primal solution.  There are n + m = nb values
*           (n structural variables and m slacks, in that order).
*     rc    contains reduced costs for all variables:
*           rc(1:n)    =  gobj + c - A'pi  for structurals,
*           rc(n+1:nb) = -pi  for slacks.
*           pi and rc corresond to the Phase-1 objective
*           if the solution is infeasible.
*     y     contains the true slack values for nonlinear constraints
*           in its first nncon components (computed by m8viol).
*
*     All quantities a, bl, bu, pi, rc, xn, y, fcon, gobj are unscaled
*     and adjusted in sign if maximizing.  (fcon is not used here.)  
*
*     If ondisk is true, the solution is output to the solution file.
*     Otherwise, it is output to the printer.
*
*     31 Jan 1991: rc is now an input parameter.  It is set in m4savb.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /cycle2/ objtru,suminf,numinf

      intrinsic          abs
      logical            feasbl, infsbl, maximz, scaled
      parameter        ( zero = 0.0d+0,  one = 1.0d+0 )
*     ------------------------------------------------------------------

      bplus  = 0.1*plinfy
      scale  = one
      feasbl = ninf   .eq. 0
      infsbl = .not. feasbl
      maximz = minimz .lt. 0
      scaled = lscale .gt. 0
      lpr    = iprint
      if (ondisk) lpr = isoln

      call m1page( 1 )
      if (infsbl) write(lpr, 1000) name, ninf, sinf
      if (feasbl) write(lpr, 1002) name, objtru
      write(lpr, 1004) istate, itn, ns
      write(lpr, 1005) mobj, minmax, mrhs, mrng, mbnd
      write(lpr, 1010)
**    tolfea = 0.1 * tolx
**    tolopt = 0.1 * toldj(3) * pinorm
** 05 Oct 1991: Might as well flag according to the tolerances
*               actually used.
      tolfea = tolx
      tolopt = toldj(3) * pinorm

*     ------------------------------------------------------------------
*     Output the ROWS section.
*     ------------------------------------------------------------------
      do 300 iloop = 1, m
         i      = iloop
         j      = n + i
         if (scaled) scale = ascale(j)
         js     = hs(j)
         b1     = bl(j)
         b2     = bu(j)
         xj     = xn(j)
         py     = pi(i)
         dj     = rc(j)

*        Define row and slack activities.

         if (i .le. nncon) xj = y(i)
         row    = - xj
         d1     =   b1 - xj
         d2     =   xj - b2
         slk    = - d1
         if (abs( d1  )  .gt.  abs( d2 )) slk =   d2
         if (abs( slk )  .ge.  bplus    ) slk = - row
         d1     = d1 / scale
         d2     = d2 / scale
         djtest = dj * scale
         if (feasbl) then
            if (   maximz   ) djtest =  - djtest
         end if

*        Change slacks into rows.

         if (js .le. 1) js = 1 - js
         b1     = - b2
         b2     = - bl(j)

         call m4id  ( j, m, n, nb, nname, name1, name2, id1, id2 )
         call m4solp( ondisk, bplus, tolfea, tolopt,
     $                js,  d1,  d2, djtest,
     $                j , id1, id2, row, slk, b1, b2, py, i )
  300 continue

*     ------------------------------------------------------------------
*     Output the COLUMNS section.
*     ------------------------------------------------------------------
      call m1page( 1 )
      write(lpr, 1020)

      do 400 jloop = 1, n
         j      = jloop
         if (scaled) scale = ascale(j)
         js     = hs(j)
         b1     = bl(j)
         b2     = bu(j)
         xj     = xn(j)
         cj     = zero
         dj     = rc(j)

         do 320 k = ka(j), ka(j+1) - 1
            ir    = ha(k)
            if (ir .eq. iobj) cj = a(k)
  320    continue

         d1     =   (b1 - xj) / scale
         d2     =   (xj - b2) / scale
         djtest = - dj * scale
         if (feasbl) then
            if (j .le. nnobj) cj     =    cj + gobj(j)
            if (   maximz   ) djtest =  - djtest
         end if

         call m4id  ( j, m, n, nb, nname, name1, name2, id1, id2 )
         call m4solp( ondisk, bplus, tolfea, tolopt,
     $                js,  d1,  d2, djtest,
     $                j , id1, id2, xj, cj, b1, b2, dj, m+j )
  400 continue

      if (ondisk) then
         if (isoln .ne. iprint) rewind isoln
         if (iprint .gt. 0) write(iprint, 1400) isoln
         if (isumm  .gt. 0) write(isumm , 1400) isoln
      end if
      return

 1000 format(' NAME', 11x, 2a4, 13x,
     $   ' INFEASIBILITIES', i7, 1p, e16.4)
 1002 format(' NAME', 11x, 2a4, 13x,
     $   ' OBJECTIVE VALUE', 1p, e23.10)
 1004 format(/ ' STATUS', 9x, a12, 9x,
     $   ' ITERATION', i7, '    SUPERBASICS', i7)
 1005 format(/
     $   ' OBJECTIVE', 6x, 2a4, ' (', a3, ')' /
     $   ' RHS      ', 6x, 2a4 /
     $   ' RANGES   ', 6x, 2a4 /
     $   ' BOUNDS   ', 6x, 2a4)
 1010 format(/ ' SECTION 1 - ROWS' //
     $   '  NUMBER  ...ROW.. STATE  ...ACTIVITY...  SLACK ACTIVITY',
     $   '  ..LOWER LIMIT.  ..UPPER LIMIT.  .DUAL ACTIVITY    ..I' /)
 1020 format(  ' SECTION 2 - COLUMNS' //
     $   '  NUMBER  .COLUMN. STATE  ...ACTIVITY...  .OBJ GRADIENT.',
     $   '  ..LOWER LIMIT.  ..UPPER LIMIT.  REDUCED GRADNT    M+J' /)
 1400 format(/ ' SOLUTION file saved on file', i4)

*     end of m4soln
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4solp( ondisk, bplus, tolfea, tolopt,
     $                   js,  d1,  d2, djtest,
     $                   j , id1, id2, xj, cj, b1, b2, dj, k )

      implicit           double precision (a-h,o-z)
      logical            ondisk

*     ------------------------------------------------------------------
*     m4solp  prints one line of the Solution file.
*
*     The following conditions are marked by key:
*
*        D  degenerate basic or superbasic variable.
*        I  infeasible basic or superbasic variable.
*        A  alternative optimum      (degenerate nonbasic dual).
*        N  nonoptimal superbasic or nonbasic (infeasible dual).
*
*     Tests for these conditions are performed on scaled quantities
*     d1, d2, djtest,
*     since the correct indication is then more likely to be given.
*     On badly scaled problems, the unscaled solution may then appear
*     to be flagged incorrectly, but this is just an illusion.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt

      intrinsic          abs
      character*4        jstat, jstate(6)
      character*1        key, lblank, laltop, ldegen, linfea, lnotop
      data               jstate /' LL ', ' UL ', 'SBS ',
     $                           ' BS' , ' EQ' , ' FR '/
      data               lblank /' '/, laltop /'A'/, ldegen /'D'/,
     $                   linfea /'I'/, lnotop /'N'/
*     ------------------------------------------------------------------

      key    = lblank
      if (js .le. 1) then

*        Set key for nonbasic variables.

         if (b1 .eq. b2) js = 4
         if (- d1 .gt. tolfea  .and.  - d2 .gt. tolfea) js = 5
         if (js .eq. 1 ) djtest = - djtest
         if (js .ge. 4 ) djtest =   abs( djtest )
         if (             abs( djtest ) .le. tolopt) key = laltop
         if (js .ne. 4  .and.  djtest   .gt. tolopt) key = lnotop
      else

*        Set key for basic and superbasic variables.

         if (abs(   d1   ) .le. tolfea  .or.
     $       abs(   d2   ) .le. tolfea) key = ldegen
         if (           js .eq. 2       .and.
     $       abs( djtest ) .gt. tolopt) key = lnotop
         if (           d1 .gt. tolfea  .or.
     $                  d2 .gt. tolfea) key = linfea
      end if

*     Select format for printing.

      jstat   = jstate(js + 1)
      if (ondisk) then
               write(isoln,  1000) j,id1,id2,key,jstat,xj,cj,b1,b2,dj,k
      else
         if (b2 .lt. bplus) then
            if (b1 .gt. - bplus) then
               write(iprint, 1200) j,id1,id2,key,jstat,xj,cj,b1,b2,dj,k
            else
               write(iprint, 1300) j,id1,id2,key,jstat,xj,cj,   b2,dj,k
            end if
         else
            if (b1 .gt. - bplus) then
               write(iprint, 1400) j,id1,id2,key,jstat,xj,cj,b1,   dj,k
            else
               write(iprint, 1500) j,id1,id2,key,jstat,xj,cj,      dj,k
            end if
         end if
      end if
      return

 1000 format(i8, 2x, 2a4, 1x, a1, 1x, a3, 1p, e16.6, 4e16.6, i7)
 1200 format(i8, 2x, 2a4, 1x, a1, 1x, a3, 5f16.5, i7)
 1300 format(i8, 2x, 2a4, 1x, a1, 1x, a3, 2f16.5,
     $   '           NONE ', f16.5, f16.5, i7)
 1400 format(i8, 2x, 2a4, 1x, a1, 1x, a3, 2f16.5,
     $   f16.5, '           NONE ', f16.5, i7)
 1500 format(i8, 2x, 2a4, 1x, a1, 1x, a3, 2f16.5,
     $   '           NONE ', '           NONE ', f16.5, i7)

*     end of m4solp
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m4stat( k, istate )

      integer            k
      character*12       istate

*     ------------------------------------------------------------------
*     m4stat loads istate with words describing the current state.
*     ------------------------------------------------------------------

      intrinsic          min
      character*12       c(0:5)
      data               c     /'PROCEEDING  ',
     $                          'OPTIMAL SOLN',
     $                          'INFEASIBLE  ',
     $                          'UNBOUNDED   ',
     $                          'EXCESS ITNS ',
     $                          'ERROR CONDN '/

      j      = min( k, 5 )
      istate = c(j)

*     end of m4stat
      end
