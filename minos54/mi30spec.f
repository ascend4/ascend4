*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*     File  mi30spec fortran
*
*     miopt    miopti   mioptr   m3char   m3dflt   m3key
*     opfile   oplook   opnumb   opscan   optokn   opuppr
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine miopt ( buffer, iprint, isumm, inform )

      implicit           double precision (a-h,o-z)
      character*(72)      buffer

*     ------------------------------------------------------------------
*     miopt  decodes the option contained in  buffer.
*
*     The buffer is output to file iprint, minus trailing blanks.
*     Error messages are output to files iprint and isumm.
*     buffer is echoed to iprint but normally not to isumm.
*     It is echoed to isumm before any error msg.
*
*     On entry,
*     iprint is the Print   file.  No output occurs if iprint .le 0.
*     isumm  is the Summary file.  No output occurs if isumm  .le 0.
*     inform is the number of errors so far.
*
*     On exit,
*     inform is the number of errors so far.
*
*     27 Nov 1991: First version.
*     ------------------------------------------------------------------

      character*16       key

      call m3key ( buffer, key, iprint, isumm, inform )

*     end of miopt
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine miopti( buffer, ivalue, iprint, isumm, inform )

      implicit           double precision (a-h,o-z)
      character*(56)     buffer
      integer            ivalue

*     ------------------------------------------------------------------
*     miopti decodes the option contained in  buffer // ivalue.
*     The parameters other than ivalue are as in miopt.
*
*     27 Nov 1991: First version.
*     17 Jan 1992: buff72 needed to comply with f77 standard.
*     ------------------------------------------------------------------

      character*16       key
      character*72       buff72

      write(key, '(i16)') ivalue
      lenbuf = len(buffer)
      buff72 = buffer
      buff72(lenbuf+1:lenbuf+16) = key
      call m3key ( buff72, key, iprint, isumm, inform )

*     end of miopti
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine mioptr( buffer, rvalue, iprint, isumm, inform )

      implicit           double precision (a-h,o-z)
      character*(56)     buffer
      double precision   rvalue

*     ------------------------------------------------------------------
*     mioptr decodes the option contained in  buffer // rvalue.
*     The parameters other than rvalue are as in miopt.
*
*     27 Nov 1991: First version.
*     17 Jan 1992: buff72 needed to comply with f77 standard.
*     ------------------------------------------------------------------

      character*16       key
      character*72       buff72

      write(key, '(1p, e16.8)') rvalue
      lenbuf = len(buffer)
      buff72 = buffer
      buff72(lenbuf+1:lenbuf+16) = key
      call m3key ( buff72, key, iprint, isumm, inform )

*     end of mioptr
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m3char( lname, iname )

      character*4        lname
      integer            iname

*     ------------------------------------------------------------------
*     m3char copies lname into iname.
*     lname contains character data in a4 format.
*     ------------------------------------------------------------------

      read (lname, '(a4)') iname

*     end of m3char
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m3dflt( mode )

      implicit           double precision (a-h,o-z)

*     ------------------------------------------------------------------
*     If mode = 1, m3dflt sets default values for most of the parameters
*                  that can be altered by opfile via the OPTIONS file.
*     If mode = 2, the parameter values are checked and possibly changed
*                  to reasonable values.
*     If mode = 3  and iprint > 0 and iparm(3) > 0, the parameters are
*                  printed.  (In the OPTIONS file,  Suppress parameters
*                  sets iparm(3) = 0.)
*
*     26 Apr 1992: mode 3 added.
*     ------------------------------------------------------------------

      logical            conv,restrt
      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime
      common    /m1word/ nwordr,nwordi,nwordh
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2len / mrows,mcols,melms
      common    /m2lu4 / parmlu(30),luparm(30)
      common    /m2mapz/ maxw,maxz
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m5freq/ kchk,kinv,ksav,klog,ksumm,i1freq,i2freq,msoln
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7cg1 / cgbeta,itncg,msgcg,modcg,restrt
      common    /m7cg2 / lcg1,lcg2,lcg3,lcg4,modtcg,nitncg,nsubsp
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8veri/ jverif(4),lverif(2)
      common    /cyclcm/ cnvtol,jnew,materr,maxcy,nephnt,nphant,nprint

      intrinsic          max, min, sqrt

      parameter         (idummy =  -11111,  rdummy = -11111.0d+0,
     $                   zero   =  0.0d+0,  one    =      1.0d+0)

      logical            lincon, linear, nlncon, nonlin, SYSTEM

      character*4        lblank, lmax, lmin, id(6)
      data               lblank /'    '/,
     $                   lmax   /'Max '/,
     $                   lmin   /'Min '/,
     $                   id     /'No  ', 'Yes ', 'Part',
     $                           'Full', ' Den', 'Spar'/

*     SYSTEM means that MINOS is being used "behind the scenes"
*     by some other system such as GAMS or AMPL or ASCEND.
*     Certain defaults should then be different.

      SYSTEM = GAMS .or. AMPL

*     Set some local machine-dependent constants.

      c3     = max( 1.0d-3, eps4 )
      c4     = max( 1.0d-4, eps3 )
      c5     = max( 1.0d-5, eps2 )
      c6     = max( 1.0d-6, eps2 )
      c7     = max( 1.0d-7, eps2 )

*     Set the Reduced-Gradient tolerances.

      xtol(1) = 0.1
      xtol(2) = c6
      ftol(1) = xtol(1)*0.1
      ftol(2) = xtol(2)**2
      gtol(1) = c3
      gtol(2) = c7
      gdummy  = -111111.0d+0

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        mode = 1.      Set parameters to default or dummy values.
*     
*        For some keywords like LAGRANGIAN, the Options file doesn't
*        allow the user to set illegal values.  Sensible defaults can
*        therefore be set here, and the final values don't need to be
*        checked later.
*     
*        Some of the options with numerical values do need to be checked
*        and sometimes we need to know if the user set values or not.
*        Such options are set to dummy values here.
*        ---------------------------------------------------------------
*     
*        Options needed by GAMS.
*        Things like file numbers have to be initialized.
*        Genuine options are mostly concerned with solving the problem.
      
         iback  =   0
         idump  =   0
         iload  =   0
         inewb  =   0
         imps   =   0
         insrt  =   0
         ioldb  =   0
         ipnch  =   0
         iprob  =   0
         ireprt =   0
         isoln  =   0
      
         idebug =   0
         itnlim = idummy
         kchk   = idummy
         kinv   = idummy
         klog   = idummy
         ksav   = idummy
         ksumm  = idummy
         kdegen = idummy
         lderiv =   3
         lprint =   0
         prnt0  = .true.
         prnt1  = .false.
         summ0  = .true.
         summ1  = .false.
         lscale = idummy
         ltime  = 3
         lverif(1) =  0
         lverif(2) = -2
         maxr   = idummy
         maxs   = idummy
         minimz =   1
         msoln  =   2
         ncom   = idummy
         nlag   =   1
         nmajor =  50
         nminor =  40
         nmulpr =   1
         nparpr = idummy
      
         do 60 i = 1, 4
            jverif(i) = -1
   60    continue
      
*        iparm(1) = Crash option
*        iparm(2) = Linesearch debug -- starting iteration
*        iparm(3) = Suppress parameters
*        iparm(4) = Scale print
*        iparm(5) = Start assigned nonlinears  (GAMS only)
      
         iparm(1) = 3
         iparm(2) = 9999999
         iparm(3) = 1
         iparm(4) = 0
         iparm(5) = 2
      
*        dparm(1) = Unbounded objective
*        dparm(2) = Unbounded step
*        dparm(3) = Function precision
*        dparm(4) = Major damping parameter
*        dparm(5) = Crash tolerance
*        dparm(6) = Minor damping parameter
*        dparm(7) = Penalty parameter
*        dparm(8) = LU swap tolerance
      
         do 10 i = 1, 8
            dparm(i) = rdummy
   10    continue
      
         difint(1) = rdummy
         difint(2) = rdummy
         etarg     = rdummy
         etash     = rdummy
         parmlu(1) = rdummy
         parmlu(2) = rdummy
         parmlu(4) = rdummy
         parmlu(5) = rdummy
         parmlu(8) = rdummy
         penpar    = rdummy
         radius    = rdummy
         rowtol    = rdummy
         scltol    = rdummy
         toldj(3)  = rdummy
         tolpiv    = rdummy
         tolrow    = c4
         tolx      = rdummy
         wtobj     = rdummy

*        Certain defaults should be different within modeling systems.

         if (SYSTEM) then
            lverif(1) = -1
            msoln     = 0
         end if

*        Options not used by GAMS.
*        These are mostly to do with the MPS file.
      
         aijtol    = 1.0d-10
         bstruc(1) = zero
         bstruc(2) = plinfy
         cnvtol    = zero
      
         i1freq =   0
         i2freq =   0
         mrows  =   0
         mcols  =   0
         melms  =   0
         mer    =  10
         mlst   =   0
         modcg  =  -1
         modtcg =   1
         nitncg =   0
         nden   =   1
         nncon  =   0
         nnjac  =   0
         nnobj  =   0
         nprob  =   0
         maxcy  =   1
         nephnt =   0
         nphant =   0
         nprint =   1
      
         call m3char( lblank, iblank )
         do 20 i = 1, 2
            name(i) = iblank
            mobj(i) = iblank
            mrhs(i) = iblank
            mrng(i) = iblank
            mbnd(i) = iblank
   20    continue
      
      else if (mode .eq. 2) then
*        ---------------------------------------------------------------
*        mode = 2.   Check parameters and assign default values.
*        ---------------------------------------------------------------
      
*        Options and variables needed by GAMS.
      
         if (nncon  .eq. 0) nnjac = 0
         if (nnjac  .eq. 0) nncon = 0
         nn     = max( nnjac, nnobj )
         nncon0 = max( nncon,   1   )
         lincon = nncon .eq. 0
         linear = nn    .eq. 0
         nlncon = nncon .gt. 0
         nonlin = nn    .gt. 0

*        Set unspecified frequencies or silly values to defaults.
      
         if (kchk   .eq. idummy) kchk   =     60
         if (kinv   .le.    0  ) kinv   =    100
         if (klog   .eq. idummy) klog   =    100
         if (ksav   .eq. idummy) ksav   =    100
         if (ksumm  .eq. idummy) ksumm  =    100
         if (kdegen .eq. idummy) kdegen =  10000
      
*        Sometimes, frequency 0 means "almost never".
      
         if (kchk   .le. 0) kchk   = 99999999
         if (klog   .le. 0) klog   = 99999999
         if (ksav   .le. 0) ksav   = 99999999
         if (ksumm  .le. 0) ksumm  = 99999999
         if (kdegen .le. 0) kdegen = 99999999
      
         prnt0  = lprint .eq. 0
         prnt1  = lprint .gt. 0
         if (iprint .le. 0) then
            prnt0  = .false.
            prnt1  = .false.
         end if
         if (isumm  .le. 0) then
            summ0  = .false.
            summ1  = .false.
         end if
      
*        Check Hessian dimension maxr and Superbasics limit maxs.
      
         if ( nonlin ) then
            if (maxr   .gt. 0  .and.  maxs .lt. 0) maxs = maxr
            if (maxs   .gt. 0  .and.  maxr .lt. 0) maxr = maxs
            if (.not. GAMS) then
               if (maxs .lt. 0) maxs = 50
               if (maxr .lt. 0) maxr = 50
            end if
         end if
         if (.not. GAMS) then
            if (maxs   .le. 0   ) maxs = 1
            if (maxr   .lt. 0   ) maxr = 0
            if (maxs   .lt. maxr) maxs = maxr
         end if
         maxr   = min( maxr, maxs )
      
*        Check other options.
      
         if (lscale .lt. 0) then
                        lscale = 2
            if (nonlin) lscale = 1
         end if
         if (ncom   .lt. 0) then
                        ncom   = 1
            if (nlncon) ncom   = 0
         end if
         if (nparpr .le. 0) then
                        nparpr = 10
            if (nonlin) nparpr = 1
         end if
      
*        If the Optimality tolerance was not specified, it should not
*        be smaller than the sqrt of the Function precision.
      
         if (toldj(3) .le. zero) then
            toldj(3) = c6
            if (dparm(3) .gt. zero) toldj(3) = sqrt( dparm(3) )
         end if
      
*        See the list of dparms above.
      
         if (dparm(1) .le. zero) dparm(1) = plinfy
         if (dparm(2) .le. zero) dparm(2) = 1.0d+10
         if (dparm(3) .le. zero) dparm(3) = eps0
         if (dparm(4) .le. zero) dparm(4) = 2.0
         if (dparm(6) .le. zero) dparm(6) = 2.0
         if (penpar   .lt. zero) penpar   = one
         dparm(7) = penpar
         if (dparm(8) .le. zero) dparm(8) = eps4
      
         if (dparm(5) .lt. zero  .or.  dparm(5) .ge. one) dparm(5)= 0.1
         if (etarg    .le. zero  .or.  etarg    .gt. one) etarg   = 0.5
         if (etash    .lt. zero  .or.  etash    .gt. one) etash   = 0.1
      
         if (difint(1).le. zero) difint(1) = sqrt( dparm(3) )
         if (difint(2).le. zero) difint(2) = dparm(3) ** 0.333333
         if (parmlu(1).lt. one ) parmlu(1) = 100.0
         if (parmlu(2).lt. one ) parmlu(2) = 10.0
         if (parmlu(4).le. zero) parmlu(4) = eps1
         if (parmlu(5).le. zero) parmlu(5) = eps1
         if (parmlu(8).le. zero) parmlu(8) = 0.5
         if (radius   .le. eps2) radius    = 0.01
         if (rowtol   .le. eps ) rowtol    = c6
         if (scltol   .le. zero) scltol    = 0.90
         if (tolpiv   .le. zero) tolpiv    = eps1
         if (tolx     .le. zero) tolx      = c6
         if (wtobj    .lt. zero) wtobj     = zero
      
*        Check the Start and Stop column numbers for gradient checking.
      
         if (jverif(1) .lt. 0) jverif(1) = 1
         if (jverif(2) .lt. 0) jverif(2) = nnobj
         if (jverif(3) .lt. 0) jverif(3) = 1
         if (jverif(4) .lt. 0) jverif(4) = nnjac
      

*        Options not used by GAMS.
      
         if (bstruc(1) .gt. bstruc(2)) then
             t          =   bstruc(1)
             bstruc(1)  =   bstruc(2)
             bstruc(2)  =   t
         end if
         if (minimz .gt. 0     ) call m3char( lmin, minmax )
         if (minimz .le. 0     ) call m3char( lmax, minmax )
         if (iback  .eq. inewb ) iback  = 0
         if (mrows  .le. 0     ) mrows  = 100
         if (mcols  .le. 0     ) mcols  = 3*mrows
         if (melms  .le. 0     ) melms  = 5*mcols
         if (itnlim .lt. 0     ) itnlim = 3*mrows + 10*nn
         maxs   = min( maxs, mcols + mrows + 1 )

      else if (mode .eq. 3  .and.  iprint .gt. 0) then
*        ---------------------------------------------------------------
*        mode = 3.  Print the parameters if
*                   Print level > 0 and iparm(3) > 0.
*        ---------------------------------------------------------------
         if (prnt1  .and.  iparm(3) .gt. 0) then
            call m1page( 1 )
            write(iprint, 1000)

            if (SYSTEM) then
*              relax
            else
               write(iprint, 2000) mrows , mlst  , bstruc(1),
     $                             mcols , mer   , bstruc(2),
     $                             melms , nephnt, aijtol
               write(iprint, 2100) imps  , ioldb , iread ,
     $                             isoln , inewb , iprint,
     $                             insrt , iback , ispecs,
     $                             ipnch , iload , idump
            end if                      
         
            write(iprint, 2200) klog     , kchk     , ksav     ,
     $                          ksumm    , kinv     , kdegen
            write(iprint, 2300) lscale   , tolx     , itnlim   ,
     $                          scltol   , toldj(3) , nparpr   ,
     $                          iparm(1) , tolpiv   , nmulpr   ,
     $                          dparm(5) , wtobj
            write(iprint, 2400) nncon    , maxr     , dparm(3) ,
     $                          nnjac    , maxs     , difint(1),
     $                          nnobj    , etash    , difint(2),
     $                          nprob    , etarg    , lderiv   ,
     $                          dparm(1) , dparm(2) , lverif(1)
            write(iprint, 2450) id(4+nden),nmajor   , radius   ,
     $                          id(1+nlag),nminor   , rowtol   ,
     $                          penpar   , id(3+ncom),lprint   ,
     $                          dparm(4) , dparm(6)
            write(iprint, 2500) parmlu(1), maxw     , idebug   ,
     $                          parmlu(2), maxz     , iparm(2) ,
     $                          parmlu(4), eps,    nwordr,nwordi,nwordh,
     $                          dparm(8) , ltime
         end if
      end if
      return

 1000 format(' Parameters' / ' ----------')
 2000 format(' MPS INPUT DATA.'
     $/ ' Row limit..............', i10, 6x,
     $  ' List limit.............', i10, 6x,
     $  ' Lower bound default....', 1p,  e10.2
     $/ ' Column limit...........', i10, 6x,
     $  ' Error message limit....', i10, 6x,
     $  ' Upper bound default....', e10.2
     $/ ' Elements limit ........', i10, 6x,
     $  ' Phantom elements.......', i10, 6x,
     $  ' Aij tolerance..........', e10.2)
 2100 format(/ ' FILES.'
     $/ ' MPS file ..............', i10, 6x,
     $  ' Old basis file ........', i10, 6x,
     $  ' (Card reader)..........', i10
     $/ ' Solution file..........', i10, 6x,
     $  ' New basis file ........', i10, 6x,
     $  ' (Printer)..............', i10
     $/ ' Insert file............', i10, 6x,
     $  ' Backup basis file......', i10, 6x,
     $  ' (Specs file)...........', i10
     $/ ' Punch file.............', i10, 6x,
     $  ' Load file..............', i10, 6x,
     $  ' Dump file..............', i10)
 2200 format(/ ' FREQUENCIES.'
     $/ ' Log frequency..........', i10, 6x,
     $  ' Check row error........', i10, 6x,
     $  ' Save new basis map.....', i10
     $/ ' Summary frequency......', i10, 6x,
     $  ' Factorize basis........', i10, 6x,
     $  ' Expand frequency.......', i10)
 2300 format(/ ' LP PARAMETERS.'
     $/ ' Scale option...........', i10,       6x,
     $  ' Feasibility tolerance..', 1p, e10.2, 6x,
     $  ' Iteration limit........', i10
     $/ ' Scale tolerance........', 0p, f10.3, 6x,
     $  ' Optimality tolerance...', e10.2,     6x,
     $  ' Partial  price.........', i10
     $/ ' Crash option...........', i10,       6x,
     $  ' Pivot tolerance........', 1p, e10.2, 6x,
     $  ' Multiple price.........', i10 
     $/ ' Crash tolerance........', 0p, f10.3, 6x,
     $  ' Weight on objective....', e10.2)
 2400 format(/ ' NONLINEAR PROBLEMS.'
     $/ ' Nonlinear constraints..', i10,       6x,
     $  ' Hessian dimension......', i10,       6x,
     $  ' Function precision.....', 1p, e10.2
     $/ ' Nonlinear Jacobian vars', i10,       6x,
     $  ' Superbasics limit......', i10,       6x,
     $  ' Difference interval....', e10.2
     $/ ' Nonlinear objectiv vars', i10,       6x,
     $  ' Linesearch tolerance...', 0p, f10.5, 6x,
     $  ' Central difference int.', 1p, e10.2
     $/ ' Problem number.........', i10,       6x,
     $  ' Subspace tolerance.....', 0p, f10.5, 6x,
     $  ' Derivative level.......', i10
     $/ ' Unbounded objective val', 1p, e10.2, 6x,
     $  ' Unbounded step size....', e10.2,     6x,
     $  ' Verify level...........', i10)
 2450 format(/ ' AUGMENTED LAGRANGIAN.'
     $/ ' Jacobian...............', 4x, a4, 'se', 6x,
     $  ' Major iterations limit.', i10, 6x,
     $  ' Radius of convergence..', 1p, e10.2
     $/ ' Lagrangian.............', 7x, a3,    6x,
     $  ' Minor iterations limit.', i10,       6x,
     $  ' Row tolerance..........', e10.2
     $/ ' Penalty parameter......', e10.2,     6x,
     $  ' Completion.............', 6x, a4,    6x,
     $  ' Print level..(JFLXB)...', i10
     $/ ' Major damping parameter', e10.2,     6x,
     $  ' Minor damping parameter', e10.2)
 2500 format(/ ' MISCELLANEOUS.'
     $/ ' LU factor tolerance....', f10.2,     6x,
     $  ' Workspace (user).......', i10,       6x,
     $  ' Debug level............', i10
     $/ ' LU update tolerance....', f10.2,     6x,
     $  ' Workspace (total)......', i10,       6x,
     $  ' Linesearch debug after.', i10
     $/ ' LU singularity tol.....', 1p, e10.2, 6x,
     $  ' eps (machine precision)', e10.2,     6x,
     $  ' nwordr, nwordi, nwordh.', 1x, 3i3
     $/ ' LU swap tolerance......', e10.2,     6x,
     $  ' Timing level...........', i10)

*     end of m3dflt
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m3key ( buffer, key, lprnt, lsumm, inform )

      implicit           double precision (a-h,o-z)
      character*(*)      buffer
      character*16       key

************************************************************************
*     m3key  decodes the option contained in  buffer  in order to set
*     a parameter value in the relevant common block.
*
*     The buffer is output to file iprint, minus trailing blanks.
*     Error messages are output to files iprint and isumm.
*     buffer is echoed to iprint but normally not to isumm.
*     It is echoed to isumm before any error msg.
*
*     On entry,
*     lprnt  is iprint as given to opfile.
*     lsumm  is isumm  as given to opfile.
*     inform is the number of errors so far.
*
*     On exit,
*     key    is the first keyword contained in buffer.
*     inform is the number of errors so far.
*
*     m3key  calls opnumb and the subprograms
*                 lookup, scannr, tokens, upcase
*     (now called oplook, opscan, optokn, opuppr)
*     supplied by Sterling Software, Palo Alto, California.
*
*     Systems Optimization Laboratory, Stanford University.
*     22 Mar 1988: First MINOS version.
*     10 Nov 1991: inform added to count errors and allow for no output.
************************************************************************

      parameter         (mxparm = 30)
      logical            conv,restrt
      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1file/ iread,iprint,isumm
      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2len / mrows,mcols,melms
      common    /m2lu4 / parmlu(30),luparm(30)
      common    /m2mapz/ maxw,maxz
      common    /m2parm/ dparm(30),iparm(30)
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3scal/ sclobj,scltol,lscale
      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m5freq/ kchk,kinv,ksav,klog,ksumm,i1freq,i2freq,msoln
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr,lprint
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5lp1 / itn,itnlim,nphs,kmodlu,kmodpi
      common    /m5prc / nparpr,nmulpr,kprc,newsb
      common    /m5step/ featol, tolx0,tolinc,kdegen,ndegen,
     $                   itnfix, nfix(2)
      common    /m5tols/ toldj(3),tolx,tolpiv,tolrow,rowerr,xnorm
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7cg1 / cgbeta,itncg,msgcg,modcg,restrt
      common    /m7cg2 / lcg1,lcg2,lcg3,lcg4,modtcg,nitncg,nsubsp
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8veri/ jverif(4),lverif(2)
      common    /cyclcm/ cnvtol,jnew,materr,maxcy,nephnt,nphant,nprint
*-----------------------------------------------------------------------

      external           opnumb
      intrinsic          abs
      logical            more  , number, opnumb, sorted

      parameter         (     maxkey = 38,  maxtie = 40,   maxtok = 10)
      character*16       keys(maxkey), ties(maxtie), token(maxtok)

*     Next 2 lines not needed by GAMS
      parameter         (     mxmkey = 35)
      character*16       mkey(mxmkey)

      character*16       key2, k2save, value

      parameter         (idummy =    -11111,   rdummy = -11111.0d+0,
     $                   sorted =    .true.,
     $                   maxint = 100000000,   zero   =  0.0d+0    )

*     maxint above should be larger than any expected integer value.

*     GAMS recognizes the following keywords.

      data   keys
     $ / 'CHECK           ', 'COMPLETION      ', 'CRASH           ',
     $   'DEBUG           ', 'DEFAULTS        ', 'EXPAND          ',
     $   'FACTORIZATION   ', 'FEASIBILITY     ', 'HESSIAN         ',
     $   'IPARM           ',
     $   'ITERATIONS      ', 'ITERS:ITERATIONS', 'ITNS :ITERATIONS',
     $   'LAGRANGIAN      ', 'LINESEARCH      ', 'LOG             ',
     $   'LU              ', 'MAJOR           ', 'MINOR           ',
     $   'MULTIPLE        ', 'OPTIMALITY      ', 'PARTIAL         ',
     $   'PENALTY         ', 'PIVOT           ', 'PRINT           ',
     $   'RADIUS          ', 'ROWS            ', 'RPARM           ',
     $   'SCALE           ', 'SOLUTION        ', 'START           ',
     $   'SUBSPACE        ', 'SUMMARY         ', 'SUPERBASICS     ',
     $   'TIMING          ', 'UNBOUNDED       ',
     $   'VERIFY          ', 'WEIGHT          '/

      data   ties
     $ / '(TOTAL)         ', '(USER)          ',
     $   'ALL             ', 'BASIC           ', 'COLUMNS         ',
     $   'CONSTRAINTS     ', 'DAMPING         ', 'DEBUG           ',
     $   'DENSE           ', 'DENSITY         ',
     $   'ELEMENTS        ', 'ELIGIBLE        ',
     $   'FACTORIZATION   ', 'FILE            ', 'FREQUENCY       ',
     $   'FULL            ', 'GRADIENTS       ',
     $   'ITERATIONS      ', 'ITERS:ITERATIONS', 'ITNS :ITERATIONS',
     $   'JACOBIAN        ', 'LEVEL           ', 'LIMIT           ',
     $   'LINEAR          ',
     $   'NO              ', 'NONBASIC        ', 'NONLINEAR       ',
     $   'OBJECTIVE       ', 'OPTION          ', 'PARTIAL         ',
     $   'PRINT           ', 'SINGULARITY     ', 'SPARSE          ',
     $   'STEP            ', 'SUPERBASIC      ', 'SWAP            ',
     $   'TOLERANCE       ', 'UPDATE          ',
     $   'VARIABLES       ', 'YES             '/

*    More keywords for MINOS --- not needed by GAMS.

      data   mkey
     $ / 'AIJ             ', 'BACKUP          ', 'BOUNDS          ',
     $   'CENTRAL         ', 'COEFFICIENTS    ', 'COLUMNS         ',
     $   'CYCLE           ', 'DERIVATIVE      ', 'DIFFERENCE      ',
     $   'DUMP            ', 'ELEMENTS        ', 'ERROR           ',
     $   'FUNCTION        ', 'INSERT          ', 'JACOBIAN        ',
     $   'LIST            ', 'LOAD            ', 'LOWER           ',
     $   'MAXIMIZE        ', 'MINIMIZE        ', 'MPS             ',
     $   'NEW             ', 'NONLINEAR       ', 'OBJECTIVE       ',
     $   'OLD             ', 'PHANTOM         ', 'PROBLEM         ',
     $   'PUNCH           ', 'RANGES          ', 'REPORT          ',
     $   'RHS             ', 'SAVE            ',
     $   'STOP            ', 'UPPER           ', 'WORKSPACE       '/
*-----------------------------------------------------------------------

*     iparm(1) = Crash option
*     iparm(2) = Linesearch debug -- starting iteration
*     iparm(3) = Suppress parameters
*     iparm(4) = Scale print
*     iparm(5) = Start assigned nonlinears

*     dparm(1) = Unbounded objective
*     dparm(2) = Unbounded step
*     dparm(3) = Function precision
*     dparm(4) = Major damping parameter
*     dparm(5) = Crash tolerance
*     dparm(6) = Minor damping parameter
*     dparm(7) = Penalty parameter

*     Set lenbuf = length of buffer without trailing blanks.
*     Echo to the print file.

      lenbuf  = 1
      do 10 j = 1, len(buffer)
         if (buffer(j:j) .ne. ' ') lenbuf = j
   10 continue

      if (lprnt .gt. 0) then
         write(lprnt, '(6x, a)') buffer(1:lenbuf)
      end if

*     Set lenb = length of buffer without trailing comments.
*     Eliminate comments and empty lines.
*     A '*' appearing anywhere in buffer terminates the string.

      i      = index( buffer(1:lenbuf), '*' )
      if (i .eq. 0) then
         lenb = lenbuf
      else
         lenb = i - 1
      end if
      if (lenb .le. 0) then
         key = '*'
         go to 900
      end if

*     ------------------------------------------------------------------
*     Extract up to maxtok tokens from the record.
*     ntoken returns how many were actually found.
*     key, key2, are the first tokens if any, otherwise blank.
*     For some values of key (BOUNDS, OBJECTIVE, RANGES, RHS)
*     we have to save key2 before oplook gets a chance to alter it.
*     For example, if the data is     Objective = OBJ
*     oplook will change obj to objective.
*     ------------------------------------------------------------------
      ntoken = maxtok
      call optokn( buffer(1:lenb), ntoken, token )
      key    = token(1)
      key2   = token(2)
      k2save = key2

*     Certain keywords require no action.

      if (key .eq. '   ') go to 900
      if (key .eq. 'END') go to 900

*     Most keywords will have an associated integer or real value,
*     so look for it no matter what the keyword.

      i      = 1
      number = .false.

   50 if (i .lt. ntoken  .and.  .not. number) then
         i      = i + 1
         value  = token(i)
         number = opnumb( value )
         go to 50
      end if

      ivalue = 0
      rvalue = zero
      if ( number ) then
         read  (value, '(bn, e16.0)') rvalue
         if (abs(rvalue) .lt. maxint) ivalue = rvalue
      end if

*     Convert the keywords to their most fundamental form
*     (upper case, no abbreviations).
*     sorted says whether the dictionaries are in alphabetic order.
*     loci   says where the keywords are in the dictionaries.
*     loci = 0 signals that the keyword wasn't there.

      call oplook( maxkey, keys, sorted, key , loc1 )
      call oplook( maxtie, ties, sorted, key2, loc2 )

*     ------------------------------------------------------------------
*     Decide what to do about each keyword.
*     The second keyword (if any) might be needed to break ties.
*     Some seemingly redundant testing of more is used
*     to avoid compiler limits on the number of consecutive else if's.
*     ------------------------------------------------------------------
      more   = .true.
      if (more) then
         more   = .false.
         if      (key .eq. 'CHECK       ') then
            kchk   = ivalue
         else if (key .eq. 'COMPLETION  ') then
              if (key2.eq. 'PARTIAL     ') ncom   = 0
              if (key2.eq. 'FULL        ') ncom   = 1
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'CRASH       ') then
              if (key2.eq. 'OPTION      ') iparm(1) = ivalue
              if (key2.eq. 'TOLERANCE   ') dparm(5) = rvalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'DEBUG       ') then
            idebug = ivalue
         else if (key .eq. 'DEFAULTS    ') then
            call m3dflt( 1 )
         else if (key .eq. 'EXPAND      ') then
            kdegen = ivalue
         else if (key .eq. 'FACTORIZATION') then
            kinv   = ivalue
         else if (key .eq. 'FEASIBILITY ') then
            tolx   = rvalue
         else if (key .eq. 'HESSIAN     ') then
            maxr   = ivalue
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'IPARM       ') then
*           Allow things like  Iparm 21 = 100  to set iparm(21) = 100
            key2   = token(3)
            if (ivalue .ge. 1  .and. ivalue .le. mxparm) then
               read (key2, '(bn, i16)') iparm(ivalue)
            else
               go to 880
            end if
         else if (key .eq. 'ITERATIONS  ') then
            itnlim = ivalue
         else if (key .eq. 'LAGRANGIAN  ') then
              if (key2.eq. 'YES         ') nlag   = 1
              if (key2.eq. 'NO          ') nlag   = 0
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'LINESEARCH  ') then
              if (key2.eq. 'TOLERANCE   ') etash    = rvalue
              if (key2.eq. 'DEBUG       ') iparm(2) = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'LOG         ') then
            klog   = ivalue
         else if (key .eq. 'LU          ') then
              if (key2.eq. 'FACTORIZATION')parmlu(1) = rvalue
              if (key2.eq. 'UPDATE      ') parmlu(2) = rvalue
              if (key2.eq. 'DENSITY     ') parmlu(8) = rvalue
              if (key2.eq. 'SINGULARITY ') then
                 parmlu(4) = rvalue
                 parmlu(5) = rvalue
              end if
              if (key2.eq. 'SWAP        ') dparm(8)  = rvalue
              if (loc2.eq.  0            ) go to 820
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'MAJOR       ') then
              if (key2.eq. 'DAMPING     ') dparm(4) = rvalue
              if (key2.eq. 'ITERATIONS  ') nmajor   = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'MINOR       ') then
              if (key2.eq. 'DAMPING     ') dparm(6) = rvalue
              if (key2.eq. 'ITERATIONS  ') nminor   = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'MULTIPLE    ') then
            nmulpr = ivalue
         else if (key .eq. 'OPTIMALITY  ') then
            toldj(3) = rvalue
         else if (key .eq. 'PARTIAL     ') then
            nparpr = ivalue
         else if (key .eq. 'PENALTY     ') then
            dparm(7) = rvalue
            penpar   = rvalue
         else if (key .eq. 'PIVOT       ') then
            tolpiv = rvalue
         else if (key .eq. 'PRINT       ') then
              if (key2.eq. 'FILE        ') iprint   = ivalue
              if (key2.eq. 'FREQUENCY   ') klog     = ivalue
              if (key2.eq. 'LEVEL       ') then
                 lprint   = ivalue
              end if
              if (loc2.eq.  0            ) go to 820
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'RADIUS      ') then
            radius = rvalue
         else if (key .eq. 'ROWS        ') then
*             GAMS and AMPL should recognize Row tolerance
*             but not just          Rows
              if (key2.eq. 'TOLERANCE   ') then
                 rowtol = rvalue
              else
                 if ( .not. (GAMS .or. AMPL) ) mrows = ivalue
              end if
         else if (key .eq. 'RPARM       ') then
*           Allow things like  Rparm 21 = 2  to set dparm(21) = 2.0
            key2   = token(3)
            if (ivalue .ge. 1  .and. ivalue .le. mxparm) then
               read (key2, '(bn, e16.0)') dparm(ivalue)
            else
               go to 880
            end if
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'SCALE       ') then
              if (key2.eq. 'OPTION      ') then
                 lscale = ivalue
              else
                 if (rvalue .gt. zero       ) scltol   = rvalue
                 if (key2.eq. 'PRINT       ') iparm(4) =  1
                 if (key2.eq. 'ALL         ') lscale   =  2
                 if (key2.eq. 'NONLINEAR   ') lscale   =  2
                 if (key2.eq. 'LINEAR      ') lscale   =  1
                 if (key2.eq. 'NO          ') lscale   =  0
                 if (key2.eq. 'YES         ') lscale   = -1
                 if (key2.eq. '            ') lscale   = -1
                 if (key2.eq. '            ') loc2     =  1
                 if (loc2.eq.  0            ) go to 820
              end if
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'SOLUTION    ') then
              if (key2.eq. 'YES         ') msoln = 2
              if (key2.eq. 'NO          ') msoln = 0
              if (key2.eq. 'FILE        ') isoln = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'START       ') then
              key2   = token(4)
              call oplook( maxtie, ties, sorted, key2, loc2 )
              if (key2.eq. 'SUPERBASIC  ') iparm(5) = 2
              if (key2.eq. 'BASIC       ') iparm(5) = 3
              if (key2.eq. 'NONBASIC    ') iparm(5) = 4
              if (key2.eq. 'ELIGIBLE    ') iparm(5) = 1
              if (key2.eq. 'OBJECTIVE   ') jverif(1) = ivalue
              if (key2.eq. 'CONSTRAINTS ') jverif(3) = ivalue
              if (loc2.eq.  0            ) go to 840
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'SUBSPACE    ') then
            etarg  = rvalue
         else if (key .eq. 'SUPERBASICS ') then
            maxs   = ivalue
         else if (key .eq. 'SUMMARY     ') then
              if (key2.eq. 'FILE        ') isumm    = ivalue
              if (key2.eq. 'FREQUENCY   ') ksumm    = ivalue
              if (key2.eq. 'LEVEL       ') then
                 summ0    = ivalue .eq. 0
                 summ1    = ivalue .gt. 0
              end if
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'TIMING      ') then
            ltime    = ivalue
         else if (key .eq. 'UNBOUNDED   ') then
              if (key2.eq. 'OBJECTIVE   ') dparm(1) = rvalue
              if (key2.eq. 'STEP        ') dparm(2) = rvalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'VERIFY      ') then
              if (key2.eq. 'OBJECTIVE   ') lverif(1) = 1
              if (key2.eq. 'CONSTRAINTS ') lverif(1) = 2
              if (key2.eq. 'GRADIENTS   ') lverif(1) = 3
              if (key2.eq. 'YES         ') lverif(1) = 3
              if (key2.eq. 'NO          ') lverif(1) = 0
              if (key2.eq. 'LEVEL       ') lverif(1) = ivalue
              if (key2.eq. '            ') lverif(1) = 3
              if (key2.eq. '            ') loc2      = 1
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'WEIGHT      ') then
            wtobj  = rvalue
         else
            more   = .true.
         end if
      end if

      if (.not. more) go to 900
      if (      GAMS) go to 800

*     ------------------------------------------------------------------
*     The following keywords are not recognized by GAMS.
*     ------------------------------------------------------------------
      call oplook( mxmkey, mkey, sorted, key , loc1 )

      if (more) then
         more   = .false.
         if      (key .eq. 'AIJ         ') then
            aijtol = rvalue
         else if (key .eq. 'BACKUP      ') then
            iback  = ivalue
         else if (key .eq. 'BOUNDS      ') then
           call m3char( k2save(1:4), mbnd(1) )
           call m3char( k2save(5:8), mbnd(2) )
         else if (key .eq. 'CENTRAL     ') then
            difint(2) = rvalue
         else if (key .eq. 'COEFFICIENTS') then
            melms  = ivalue
         else if (key .eq. 'COLUMNS     ') then
            mcols  = ivalue
         else if (key .eq. 'CYCLE       ') then
              if (key2.eq. 'LIMIT       ') maxcy  = ivalue
              if (key2.eq. 'PRINT       ') nprint = ivalue
              if (key2.eq. 'TOLERANCE   ') cnvtol = rvalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'DERIVATIVE  ') then
            lderiv = ivalue
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'DIFFERENCE  ') then
            difint(1) = rvalue
         else if (key .eq. 'DUMP        ') then
            idump  = ivalue
         else if (key .eq. 'ELEMENTS    ') then
            melms  = ivalue
         else if (key .eq. 'ERROR       ') then
            mer    = ivalue
         else if (key .eq. 'FUNCTION    ') then
            dparm(3) = rvalue
         else if (key .eq. 'INSERT      ') then
            insrt  = ivalue
         else if (key .eq. 'JACOBIAN    ') then
              if (key2.eq. 'DENSE       ') nden = 1
              if (key2.eq. 'SPARSE      ') nden = 2
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'LIST        ') then
            mlst   = ivalue
         else if (key .eq. 'LOAD        ') then
            iload  = ivalue
         else if (key .eq. 'LOWER       ') then
            bstruc(1) = rvalue
         else if (key .eq. 'MAXIMIZE    ') then
            minimz = -1
         else if (key .eq. 'MINIMIZE    ') then
            minimz =  1
         else if (key .eq. 'MPS         ') then
            imps   = ivalue
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'NEW         ') then
            inewb  = ivalue
         else if (key .eq. 'NONLINEAR   ') then
              if (key2.eq. 'CONSTRAINTS ') nncon = ivalue
              if (key2.eq. 'OBJECTIVE   ') nnobj = ivalue
              if (key2.eq. 'JACOBIAN    ') nnjac = ivalue
              if (key2.eq. 'VARIABLES   ') then
                  nnobj = ivalue
                  nnjac = ivalue
              end if
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'OBJECTIVE   ') then
            call m3char( k2save(1:4), mobj(1) )
            call m3char( k2save(5:8), mobj(2) )
         else if (key .eq. 'OLD         ') then
            ioldb  = ivalue
         else if (key .eq. 'PHANTOM     ') then
              if (key2.eq. 'COLUMNS     ') nphant = ivalue
              if (key2.eq. 'ELEMENTS    ') nephnt = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'PROBLEM     ') then
              if (key2.eq. 'FILE        ') ifile  = ivalue
              if (key2.ne. 'FILE        ') nprob  = ivalue
         else if (key .eq. 'PUNCH       ') then
            ipnch  = ivalue
         else if (key .eq. 'RANGES      ') then
            call m3char( k2save(1:4), mrng(1) )
            call m3char( k2save(5:8), mrng(2) )
         else if (key .eq. 'REPORT      ') then
            ireprt = ivalue
         else if (key .eq. 'RHS         ') then
            call m3char( k2save(1:4), mrhs(1) )
            call m3char( k2save(5:8), mrhs(2) )
         else
            more   = .true.
         end if
      end if

      if (more) then
         more   = .false.
         if      (key .eq. 'SAVE        ') then
            ksav   = ivalue
         else if (key .eq. 'STOP        ') then
              if (key2.eq. 'OBJECTIVE   ') jverif(2) = ivalue
              if (key2.eq. 'CONSTRAINTS ') jverif(4) = ivalue
              if (loc2.eq.  0            ) go to 820
         else if (key .eq. 'UPPER       ') then
            bstruc(2) = rvalue
         else if (key .eq. 'WORKSPACE   ') then
              if (key2.eq. '(USER)      ') maxw = ivalue
              if (key2.eq. '(TOTAL)     ') maxz = ivalue
              if (loc2.eq.  0            ) go to 820
         else
            more   = .true.
         end if
      end if

      if (.not. more) go to 900

*     ------------------------------------------------------------------
*     Error messages.
*     ------------------------------------------------------------------
  800 inform = inform + 1
      if (lprnt .gt. 0) then
         write (lprnt, 2300) key
      end if
      if (lsumm .gt. 0) then
         write (lsumm, '(1x, a )') buffer
         write (lsumm, 2300) key
      end if
      return

  820 inform = inform + 1
      if (lprnt .gt. 0) then
         write (lprnt, 2320) key2
      end if
      if (lsumm .gt. 0) then
         write (lsumm, '(1x, a )') buffer
         write (lsumm, 2320) key2
      end if
      return

  840 inform = inform + 1
      if (lprnt .gt. 0) then
         write (lprnt, 2340) key2
      end if
      if (lsumm .gt. 0) then
         write (lsumm, '(1x, a )') buffer
         write (lsumm, 2340) key2
      end if
      return

  880 inform = inform + 1
      if (lprnt .gt. 0) then
         write (lprnt, 2380) ivalue
      end if
      if (lsumm .gt. 0) then
         write (lsumm, '(1x, a )') buffer
         write (lsumm, 2380) ivalue
      end if

  900 return

 2300 format(' XXX  Keyword not recognized:         ', a)
 2320 format(' XXX  Second keyword not recognized:  ', a)
*2330 format(' XXX  Third  keyword not recognized:  ', a)
 2340 format(' XXX  Fourth keyword not recognized:  ', a)
 2380 format(' XXX  The parm subscript is out of range:', i10)

*     end of m3key
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine opfile( ncalls, ioptns, opkey,
     $                   title , iprint, isumm, inform )

      integer            ncalls, ioptns, iprint, isumm, inform
      character*(*)      title
      external           opkey

************************************************************************
*     opfile  reads the options file from unit  ioptns  and loads the
*     relevant options, using opkey to process each line.
*
*     On exit, inform says how many errors were encountered.
*
*     Systems Optimization Laboratory, Stanford University.
*     18-Dec-1985: Original version.
*     20-Mar-1988: First MINOS version -- title added, prnt deleted.
*     10 Nov 1991: Provision made for no output to iprint and isumm.
*     11 Nov 1991: opfile now calls m3dflt( 1 ) to initialize options
*                  only after an Options file has been found.
************************************************************************

      character*16       key   , token(1)
      character*72       buffer
      character*30       dashes
      data               dashes /'=============================='/

*     lprnt and lsumm and local copies of iprint and isumm
*     (which might get changed by m3key).

      lprnt  = iprint
      lsumm  = isumm
      inform = 0

*     Return if the unit number is out of range.

      if (ioptns .lt. 0  .or.  ioptns .gt. 99) then
         inform = 1
         return
      end if

*     ------------------------------------------------------------------
*     Look for  Begin, Endrun  or  Skip.
*     ------------------------------------------------------------------
      nread  = 0
   50    read (ioptns, '(a72)', end = 930) buffer
         nread = nread + 1
         nkey  = 1
         call optokn( buffer, nkey, token )
         key   = token(1)
         if (key .eq. 'ENDRUN') go to 940
         if (key .ne. 'BEGIN' ) then
            if (nread .eq. 1  .and.  key .ne. 'SKIP') then
               inform = inform + 1
               if (lprnt .gt. 0) write (lprnt, 2000) ioptns, buffer
               if (lsumm .gt. 0) write (lsumm, 2000) ioptns, buffer
            end if
            go to 50
         end if

*     ------------------------------------------------------------------
*     BEGIN found.
*     This is taken to be the first line of an OPTIONS file.
*     It is printed without the trailing blanks.
*     ------------------------------------------------------------------
      call m1page(1)
      do 10 j = 1, len(buffer)
         if (buffer(j:j) .ne. ' ') lenbuf = j
   10 continue

      if (lprnt .gt. 0) then
         write (lprnt, '(  9x, a)') ' ', dashes, title, dashes
         write (lprnt, '(/ 6x, a)') buffer(1:lenbuf)
      end if
      if (lsumm .gt. 0) then
         write (lsumm, '(  1x, a)') ' ', dashes, title, dashes
         write (lsumm, '(/ 1x, a)') buffer(1:lenbuf)
      end if

*     Set options to default values.

      call m3dflt( 1 )

*     ------------------------------------------------------------------
*     Read the rest of the file.
*     ------------------------------------------------------------------
*+    while (key .ne. 'END') loop
  100 if    (key .ne. 'END') then
         read  (ioptns, '(a72)', end = 920) buffer
         call opkey ( buffer, key, lprnt, lsumm, inform )
         go to 100
      end if
*+    end while

      return

  920 if (lprnt .gt. 0) write (lprnt, 2200) ioptns
      if (lsumm .gt. 0) write (lsumm, 2200) ioptns
      inform = 2
      return

  930 if (ncalls .le. 1) then
         if (lprnt .gt. 0) write (lprnt, 2300) ioptns
         if (lsumm .gt. 0) write (lsumm, 2300) ioptns
      else
         if (lprnt .gt. 0) write (lprnt, '(a)') ' Endrun'
         if (lsumm .gt. 0) write (lsumm, '(a)') ' Endrun'
      end if
      inform = 3
      return

  940 if (lprnt .gt. 0) write (lprnt, '(/ 6x, a)') buffer
      if (lsumm .gt. 0) write (lsumm, '(/ 1x, a)') buffer
      inform = 4
      return

 2000 format(
     $ //' XXX  Error while looking for an OPTIONS file on unit', I7
     $ / ' XXX  The file should start with Begin, Skip or Endrun'
     $ / ' XXX  but the first record found was the following:'
     $ //' ---->', a
     $ //' XXX  Continuing to look for OPTIONS file...')
 2200 format(//' XXX  End-of-file encountered while processing',
     $         ' an OPTIONS file on unit', I6)
 2300 format(//' XXX  End-of-file encountered while looking for',
     $         ' an OPTIONS file on unit', I6)

*     end of opfile
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
C
      SUBROUTINE OPLOOK (NDICT, DICTRY, ALPHA, KEY, ENTRY)
C
C
C Description and usage:
C
C       Performs dictionary lookups.  A pointer is returned if a
C    match is found between the input key and the corresponding
C    initial characters of one of the elements of the dictionary.
C    If a "synonym" has been provided for an entry, the search is
C    continued until a match to a primary dictionary entry is found.
C    Cases of no match, or multiple matches, are also provided for.
C
C     Dictionary entries must be left-justified, and may be alphabetized
C    for faster searches.  Secondary entries, if any, are composed of
C    two words separated by one or more characters such as blank, tab,
C    comma, colon, or equal sign which are treated as non-significant
C    by OPSCAN.  The first entry of each such pair serves as a synonym
C    for the second, more fundamental keyword.
C
C       The ordered search stops after the section of the dictionary
C    having the same first letters as the key has been checked, or
C    after a specified number of entries have been examined.  A special
C    dictionary entry, the vertical bar '|', will also terminate the
C    search.  This will speed things up if an appropriate dictionary
C    length parameter cannot be determined.  Both types of search are
C    sequential.  See "Notes" below for some suggestions if efficiency
C    is an issue.
C
C
C Parameters:
C
C    Name    Dimension  Type  I/O/S  Description
C    NDICT               I    I      Number of dictionary entries to be
C                                    examined.
C    DICTRY  NDICT       C    I      Array of dictionary entries,
C                                    left-justified in their fields.
C                                    May be alphabetized for efficiency,
C                                    in which case ALPHA should be
C                                    .TRUE.  Entries with synonyms are
C                                    of the form
C                                    'ENTRY : SYNONYM', where 'SYNONYM'
C                                    is a more fundamental entry in the
C                                    same dictionary.  NOTE: Don't build
C                                    "circular" dictionaries!
C    ALPHA               L    I      Indicates whether the dictionary
C                                    is in alphabetical order, in which
C                                    case the search can be terminated
C                                    sooner.
C    KEY                 C    I/O    String to be compared against the
C                                    dictionary.  Abbreviations are OK
C                                    if they correspond to a unique
C                                    entry in the dictionary.  KEY is
C                                    replaced on termination by its most
C                                    fundamental equivalent dictionary
C                                    entry (uppercase, left-justified)
C                                    if a match was found.
C    ENTRY               I      O    Dictionary pointer.  If > 0, it
C                                    indicates which entry matched KEY.
C                                    In case of trouble, a negative
C                                    value means that a UNIQUE match
C                                    was not found - the absolute value
C                                    of ENTRY points to the second
C                                    dictionary entry that matched KEY.
C                                    Zero means that NO match could be
C                                    found.  ENTRY always refers to the
C                                    last search performed -
C                                    in searching a chain of synonyms,
C                                    a non-positive value will be
C                                    returned if there is any break,
C                                    even if the original input key
C                                    was found.
C
C
C External references:
C
C    Name    Description
C    OPSCAN  Finds first and last significant characters.
C
C
C Environment:  Digital VAX-11/780 VMS FORTRAN (FORTRAN 77).
C               Appears to satisfy the ANSI Fortran 77 standard.
C
C
C Notes:
C
C    (1)  IMPLICIT NONE is non-standard.  (Has been commented out.)
C
C    (2)  We have assumed that the dictionary is not too big.  If
C         many searches are to be done or if the dictionary has more
C         than a dozen or so entries, it may be advantageous to build
C         an index array of pointers to the beginning of the section
C         of the dictionary containing each letter, then pass in the
C         portion of the dictionary beginning with DICTRY (INDEX).
C         (This won't generally work for dictionaries with synonyms.)
C         For very large problems, a completely different approach may
C         be advisable, e.g. a binary search for ordered dictionaries.
C
C    (3)  OPLOOK is case sensitive.  In most applications it will be
C         necessary to use an uppercase dictionary, and to convert the
C         input key to uppercase before calling OPLOOK.  Companion
C         routines OPTOKN and PAIRS, available from the author, already
C         take care of this.
C
C    (4)  The key need not be left-justified.  Any leading (or
C         trailing) characters which are "non-significant" to OPSCAN
C         will be ignored.  These include blanks, horizontal tabs,
C         commas, colons, and equal signs.  See OPSCAN for details.
C
C    (5)  The ASCII collating sequence for character data is assumed.
C         (N.B. This means the numerals precede the alphabet, unlike
C         common practice!)  This should not cause trouble on EBCDIC
C         machines if DICTRY just contains alphabetic keywords.
C         Otherwise it may be necessary to use the FORTRAN lexical
C         library routines to force use of the ASCII sequence.
C
C    (6)  Parameter NUMSIG sets a limit on the length of significant
C         dictionary entries.  Special applications may require that
C         this be increased.  (It is 16 in the present version.)
C
C    (7)  No protection against "circular" dictionaries is provided:
C         don't claim that A is B, and that B is A.  All synonym chains
C         must terminate!  Other potential errors not checked for
C         include duplicate or mis-ordered entries.
C
C    (8)  The handling of ambiguities introduces some ambiguity:
C
C            ALPHA = .TRUE.  A potential problem, when one entry
C                            looks like an abbreviation for another
C                            (eg. does 'A' match 'A' or 'AB'?) was
C                            resolved by dropping out of the search
C                            immediately when an "exact" match is found.
C
C            ALPHA = .FALSE. The programmer must ensure that the above
C                            situation does not arise: each dictionary
C                            entry must be recognizable, at least when
C                            specified to full length.  Otherwise, the
C                            result of a search will depend on the
C                            order of entries.
C
C
C Author:  Robert Kennelly, Informatics General Corporation.
C
C
C Development history:
C
C    24 Feb. 1984  RAK/DAS  Initial design and coding.
C    25 Feb. 1984    RAK    Combined the two searches by suitable
C                           choice of terminator FLAG.
C    28 Feb. 1984    RAK    Optional synonyms in dictionary, no
C                           longer update KEY.
C    29 Mar. 1984    RAK    Put back replacement of KEY by its
C                           corresponding entry.
C    21 June 1984    RAK    Corrected bug in error handling for cases
C                           where no match was found.
C    23 Apr. 1985    RAK    Introduced test for exact matches, which
C                           permits use of dictionary entries which
C                           would appear to be ambiguous (for ordered
C                           case).  Return -I to point to the entry
C                           which appeared ambiguous (had been -1).
C                           Repaired loop termination - had to use
C                           equal length strings or risk quitting too
C                           soon when one entry is an abbreviation
C                           for another.  Eliminated HIT, reduced
C                           NUMSIG to 16.
C    15 Nov. 1985    MAS    Loop 20 now tests .LT. FLAG, not .LE. FLAG.
C                           If ALPHA is false, FLAG is now '|', not '{'.
C    26 Jan. 1986    PEG    Declaration of FLAG and TARGET modified to
C                           conform to ANSI-77 standard.
C-----------------------------------------------------------------------


C     Variable declarations.
C     ----------------------

*     IMPLICIT NONE

C     Parameters.

      INTEGER
     $   NUMSIG
      CHARACTER
     $   BLANK, VBAR
      PARAMETER
     $   (BLANK = ' ', VBAR = '|', NUMSIG = 16)

C     Variables.

      LOGICAL
     $   ALPHA
      INTEGER
     $   ENTRY, FIRST, I, LAST, LENGTH, MARK, NDICT
*     CHARACTER
*    $   DICTRY (NDICT) * (*), FLAG * (NUMSIG),
*    $   KEY * (*), TARGET * (NUMSIG)
      CHARACTER
     $   DICTRY (NDICT) * (*), FLAG * 16,
     $   KEY * (*), TARGET * 16

C     Procedures.

      EXTERNAL
     $   OPSCAN


C     Executable statements.
C     ----------------------

      ENTRY = 0

C     Isolate the significant portion of the input key (if any).

      FIRST = 1
      LAST  = MIN( LEN(KEY), NUMSIG )
      CALL OPSCAN (KEY, FIRST, LAST, MARK)

      IF (MARK .GT. 0) THEN
         TARGET = KEY (FIRST:MARK)

C        Look up TARGET in the dictionary.

   10    CONTINUE
            LENGTH = MARK - FIRST + 1

C           Select search strategy by cunning choice of termination test
C           flag.  The vertical bar is just about last in both the
C           ASCII and EBCDIC collating sequences.

            IF (ALPHA) THEN
               FLAG = TARGET
            ELSE
               FLAG = VBAR
            END IF


C           Perform search.
C           ---------------

            I = 0
   20       CONTINUE
               I = I + 1
               IF (TARGET (1:LENGTH) .EQ. DICTRY (I) (1:LENGTH)) THEN
                  IF (ENTRY .EQ. 0) THEN

C                    First "hit" - must still guard against ambiguities
C                    by searching until we've gone beyond the key
C                    (ordered dictionary) or until the end-of-dictionary
C                    mark is reached (exhaustive search).

                     ENTRY = I

C                    Special handling if match is exact - terminate
C                    search.  We thus avoid confusion if one dictionary
C                    entry looks like an abbreviation of another.
C                    This fix won't generally work for un-ordered
C                    dictionaries!

                     FIRST = 1
                     LAST = NUMSIG
                     CALL OPSCAN (DICTRY (ENTRY), FIRST, LAST, MARK)
                     IF (MARK .EQ. LENGTH) I = NDICT
                  ELSE


C                    Oops - two hits!  Abnormal termination.
C                    ---------------------------------------

                     ENTRY = -I
                     RETURN
                  END IF
               END IF

C           Check whether we've gone past the appropriate section of the
C           dictionary.  The test on the index provides insurance and an
C           optional means for limiting the extent of the search.

            IF (DICTRY (I) (1:LENGTH) .LT. FLAG  .AND.  I .LT. NDICT)
     $         GO TO 20


C           Check for a synonym.
C           --------------------

            IF (ENTRY .GT. 0) THEN

C              Look for a second entry "behind" the first entry.  FIRST
C              and MARK were determined above when the hit was detected.

               FIRST = MARK + 2
               CALL OPSCAN (DICTRY (ENTRY), FIRST, LAST, MARK)
               IF (MARK .GT. 0) THEN

C                 Re-set target and dictionary pointer, then repeat the
C                 search for the synonym instead of the original key.

                  TARGET = DICTRY (ENTRY) (FIRST:MARK)
                  ENTRY = 0
                  GO TO 10

               END IF
            END IF

      END IF
      IF (ENTRY .GT. 0) KEY = DICTRY (ENTRY)


C     Normal termination.
C     -------------------

      RETURN

C     End of OPLOOK
      END
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      FUNCTION OPNUMB( STRING )

      LOGICAL          OPNUMB
      CHARACTER*(*)    STRING

************************************************************************
*     Description and usage:
*
*        A simple(-minded) test for numeric data is implemented by
*        searching an input string for legitimate characters:
*                digits 0 to 9, D, E, -, + and .
*        Insurance is provided by requiring that a numeric string
*        have at least one digit, at most one D, E or .
*        and at most two -s or +s.  Note that a few ambiguities remain:
*
*           (a)  A string might have the form of numeric data but be
*                intended as text.  No general test can hope to detect
*                such cases.
*
*           (b)  There is no check for correctness of the data format.
*                For example a meaningless string such as 'E1.+2-'
*                will be accepted as numeric.
*
*        Despite these weaknesses, the method should work in the
*        majority of cases.
*
*
*     Parameters:
*
*        Name    Dimension  Type  I/O/S  Description
*        OPNUMB              L      O    Set .TRUE. if STRING appears
*                                        to be numerical data.
*        STRING              C    I      Input data to be tested.
*
*
*     Environment:  ANSI FORTRAN 77.
*
*
*     Notes:
*
*        (1)  It is assumed that STRING is a token extracted by
*             OPTOKN, which will have converted any lower-case
*             characters to upper-case.
*
*        (2)  OPTOKN pads STRING with blanks, so that a genuine
*             number is of the form  '1234        '.
*             Hence, the scan of STRING stops at the first blank.
*
*        (3)  COMPLEX data with parentheses will not look numeric.
*
*
*     Systems Optimization Laboratory, Stanford University.
*     12 Nov  1985    Initial design and coding, starting from the
*                     routine ALPHA from Informatics General, Inc.
************************************************************************

      LOGICAL         NUMBER
      INTEGER         J, LENGTH, NDIGIT, NEXP, NMINUS, NPLUS, NPOINT
      CHARACTER*1     ATOM

      NDIGIT = 0
      NEXP   = 0
      NMINUS = 0
      NPLUS  = 0
      NPOINT = 0
      NUMBER = .TRUE.
      LENGTH = LEN (STRING)
      J      = 0

   10    J    = J + 1
         ATOM = STRING (J:J)
         IF      (ATOM .GE. '0'  .AND.  ATOM .LE. '9') THEN
            NDIGIT = NDIGIT + 1
         ELSE IF (ATOM .EQ. 'D'  .OR.   ATOM .EQ. 'E') THEN
            NEXP   = NEXP   + 1
         ELSE IF (ATOM .EQ. '-') THEN
            NMINUS = NMINUS + 1
         ELSE IF (ATOM .EQ. '+') THEN
            NPLUS  = NPLUS  + 1
         ELSE IF (ATOM .EQ. '.') THEN
            NPOINT = NPOINT + 1
         ELSE IF (ATOM .EQ. ' ') THEN
            J      = LENGTH
         ELSE
            NUMBER = .FALSE.
         END IF

         IF (NUMBER  .AND.  J .LT. LENGTH) GO TO 10

      OPNUMB = NUMBER
     $         .AND.  NDIGIT .GE. 1
     $         .AND.  NEXP   .LE. 1
     $         .AND.  NMINUS .LE. 2
     $         .AND.  NPLUS  .LE. 2
     $         .AND.  NPOINT .LE. 1

      RETURN

*     End of OPNUMB
      END
C+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
C
      SUBROUTINE OPSCAN (STRING, FIRST, LAST, MARK)
C
C
C Description and usage:
C
C       Looks for non-blank fields ("tokens") in a string, where the
C    fields are of arbitrary length, separated by blanks, tabs, commas,
C    colons, or equal signs.  The position of the end of the 1st token
C    is also returned, so this routine may be conveniently used within
C    a loop to process an entire line of text.
C
C       The procedure examines a substring, STRING (FIRST : LAST), which
C    may of course be the entire string (in which case just call OPSCAN
C    with FIRST <= 1 and LAST >= LEN (STRING) ).  The indices returned
C    are relative to STRING itself, not the substring.
C
C
C Parameters:
C
C    Name    Dimension  Type  I/O/S  Description
C    STRING              C    I      Text string containing data to be
C                                    scanned.
C    FIRST               I    I/O    Index of beginning of substring.
C                                    If <= 1, the search begins with 1.
C                                    Output is index of beginning of
C                                    first non-blank field, or 0 if no
C                                    token was found.
C    LAST                I    I/O    Index of end of substring.
C                                    If >= LEN (STRING), the search
C                                    begins with LEN (STRING).  Output
C                                    is index of end of last non-blank
C                                    field, or 0 if no token was found.
C    MARK                I      O    Points to end of first non-blank
C                                    field in the specified substring.
C                                    Set to 0 if no token was found.
C
C
C Environment:  Digital VAX-11/780 VMS FORTRAN (FORTRAN 77).
C               ANSI Fortran 77, except for the tab character HT.
C
C Notes:
C
C    (1)  IMPLICIT NONE is non-standard.  Constant HT (Tab) is defined
C         in a non-standard way:  the CHAR function is not permitted
C         in a PARAMETER declaration (OK on VAX, though).  For Absoft
C         FORTRAN 77 on 68000 machines, use HT = 9.  In other cases, it
C         may be best to declare HT as a variable and assign
C         HT = CHAR(9) on ASCII machines, or CHAR(5) for EBCDIC.
C
C    (2)  The pseudo-recursive structure was chosen for fun.  It is
C         equivalent to three DO loops with embedded GO TOs in sequence.
C
C    (3)  The variety of separators recognized limits the usefulness of
C         this routine somewhat.  The intent is to facilitate handling
C         such tokens as keywords or numerical values.  In other
C         applications, it may be necessary for ALL printing characters
C         to be significant.  A simple modification to statement
C         function SOLID will do the trick.
C
C
C Author:  Robert Kennelly, Informatics General Corporation.
C
C
C Development history:
C
C    29 Dec. 1984    RAK    Initial design and coding, (very) loosely
C                           based on SCAN_STRING by Ralph Carmichael.
C    25 Feb. 1984    RAK    Added ':' and '=' to list of separators.
C    16 Apr. 1985    RAK    Defined SOLID in terms of variable DUMMY
C                           (previous re-use of STRING was ambiguous).
C
C-----------------------------------------------------------------------


C     Variable declarations.
C     ----------------------

*     IMPLICIT NONE

C     Parameters.

      CHARACTER
     $   BLANK, EQUAL, COLON, COMMA, HT
      PARAMETER
     $   (BLANK = ' ', EQUAL = '=', COLON = ':', COMMA = ',')

C     Variables.

      LOGICAL
     $   SOLID
      INTEGER
     $   BEGIN, END, FIRST, LAST, LENGTH, MARK
      CHARACTER
     $   DUMMY, STRING * (*)

C     Statement functions.

      SOLID (DUMMY) = (DUMMY .NE. BLANK) .AND.
     $                (DUMMY .NE. COLON) .AND.
     $                (DUMMY .NE. COMMA) .AND.
     $                (DUMMY .NE. EQUAL) .AND.
     $                (DUMMY .NE. HT)


C     Executable statements.
C     ----------------------

****  HT     = CHAR(9) for ASCII machines, CHAR(5) for EBCDIC.
      HT     = CHAR(9)
      MARK   = 0
      LENGTH = LEN (STRING)
      BEGIN  = MAX (FIRST, 1)
      END    = MIN (LENGTH, LAST)

C     Find the first significant character ...

      DO 30 FIRST = BEGIN, END, +1
         IF (SOLID (STRING (FIRST : FIRST))) THEN

C           ... then the end of the first token ...

            DO 20 MARK = FIRST, END - 1, +1
               IF (.NOT.SOLID (STRING (MARK + 1 : MARK + 1))) THEN

C                 ... and finally the last significant character.

                  DO 10 LAST = END, MARK, -1
                     IF (SOLID (STRING (LAST : LAST))) THEN
                        RETURN
                     END IF
   10             CONTINUE

C                 Everything past the first token was a separator.

                  LAST = LAST + 1
                  RETURN
               END IF
   20       CONTINUE

C           There was nothing past the first token.

            LAST = MARK
            RETURN
         END IF
   30 CONTINUE

C     Whoops - the entire substring STRING (BEGIN : END) was composed of
C     separators !

      FIRST = 0
      MARK = 0
      LAST = 0
      RETURN

C     End of OPSCAN
      END
C+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
C
      SUBROUTINE OPTOKN (STRING, NUMBER, LIST)
C
C
C Description and usage:
C
C       An aid to parsing input data.  The individual "tokens" in a
C    character string are isolated, converted to uppercase, and stored
C    in an array.  Here, a token is a group of significant, contiguous
C    characters.  The following are NON-significant, and hence may
C    serve as separators:  blanks, horizontal tabs, commas, colons,
C    and equal signs.  See OPSCAN for details.  Processing continues
C    until the requested number of tokens have been found or the end
C    of the input string is reached.
C
C
C Parameters:
C
C    Name    Dimension  Type  I/O/S  Description
C    STRING              C    I      Input string to be analyzed.
C    NUMBER              I    I/O    Number of tokens requested (input)
C                                    and found (output).
C    LIST    NUMBER      C      O    Array of tokens, changed to upper
C                                    case.
C
C
C External references:
C
C    Name    Description
C    OPSCAN  Finds positions of first and last significant characters.
C    OPUPPR  Converts a string to uppercase.
C
C
C Environment:  Digital VAX-11/780 VMS FORTRAN (FORTRAN 77).
C               Appears to satisfy the ANSI Fortran 77 standard.
C
C
C Notes:
C
C    (1)  IMPLICIT NONE is non-standard.  (Has been commented out.)
C
C
C Author:  Robert Kennelly, Informatics General Corporation.
C
C
C Development history:
C
C    16 Jan. 1984    RAK    Initial design and coding.
C    16 Mar. 1984    RAK    Revised header to reflect full list of
C                           separators, repaired faulty WHILE clause
C                           in "10" loop.
C    18 Sep. 1984    RAK    Change elements of LIST to uppercase one
C                           at a time, leaving STRING unchanged.
C
C-----------------------------------------------------------------------


C     Variable declarations.
C     ----------------------

*     IMPLICIT NONE

C     Parameters.

      CHARACTER
     $   BLANK
      PARAMETER
     $   (BLANK = ' ')

C     Variables.

      INTEGER
     $   COUNT, FIRST, I, LAST, MARK, NUMBER
      CHARACTER
     $   STRING * (*), LIST (NUMBER) * (*)

C     Procedures.

      EXTERNAL
     $   OPUPPR, OPSCAN


C     Executable statements.
C     ----------------------

C     WHILE there are tokens to find, loop UNTIL enough have been found.

      FIRST = 1
      LAST = LEN (STRING)

      COUNT = 0
   10 CONTINUE

C        Get delimiting indices of next token, if any.

         CALL OPSCAN (STRING, FIRST, LAST, MARK)
         IF (LAST .GT. 0) THEN
            COUNT = COUNT + 1

C           Pass token to output string array, then change case.

            LIST (COUNT) = STRING (FIRST : MARK)
            CALL OPUPPR (LIST (COUNT))
            FIRST = MARK + 2
            IF (COUNT .LT. NUMBER) GO TO 10

         END IF


C     Fill the rest of LIST with blanks and set NUMBER for output.

      DO 20 I = COUNT + 1, NUMBER
         LIST (I) = BLANK
   20 CONTINUE

      NUMBER = COUNT


C     Termination.
C     ------------

      RETURN

C     End of OPTOKN
      END
C+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
C
      SUBROUTINE OPUPPR(STRING)
C
C ACRONYM:  UPper CASE
C
C PURPOSE:  This subroutine changes all lower case letters in the
C           character string to upper case.
C
C METHOD:   Each character in STRING is treated in turn.  The intrinsic
C           function INDEX effectively allows a table lookup, with
C           the local strings LOW and UPP acting as two tables.
C           This method avoids the use of CHAR and ICHAR, which appear
C           be different on ASCII and EBCDIC machines.
C
C ARGUMENTS
C    ARG       DIM     TYPE I/O/S DESCRIPTION
C  STRING       *       C   I/O   Character string possibly containing
C                                 some lower-case letters on input;
C                                 strictly upper-case letters on output
C                                 with no change to any non-alphabetic
C                                 characters.
C
C EXTERNAL REFERENCES:
C  LEN    - Returns the declared length of a CHARACTER variable.
C  INDEX  - Returns the position of second string within first.
C
C ENVIRONMENT:  ANSI FORTRAN 77
C
C DEVELOPMENT HISTORY:
C     DATE  INITIALS  DESCRIPTION
C   06/28/83   CLH    Initial design.
C   01/03/84   RAK    Eliminated NCHAR input.
C   06/14/84   RAK    Used integer PARAMETERs in comparison.
C   04/21/85   RAK    Eliminated DO/END DO in favor of standard code.
C   09/10/85   MAS    Eliminated CHAR,ICHAR in favor of LOW, UPP, INDEX.
C
C AUTHOR: Charles Hooper, Informatics General, Palo Alto, CA.
C
C-----------------------------------------------------------------------

      CHARACTER      STRING * (*)
      INTEGER        I, J
      character*1    C
      character*26   LOW, UPP
      data           LOW /'abcdefghijklmnopqrstuvwxyz'/,
     $               UPP /'ABCDEFGHIJKLMNOPQRSTUVWXYZ'/

      DO 10 J = 1, LEN(STRING)
         C    = STRING(J:J)
         IF (C .GE. 'a'  .AND.  C .LE. 'z') THEN
            I           = INDEX( LOW, C )
            IF (I .GT. 0) STRING(J:J) = UPP(I:I)
         END IF
   10 CONTINUE

*     End of OPUPPR
      END
