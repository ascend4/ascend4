************************************************************************
*
*     File  mi10unix  fortran.
*-->  Unix version of  mi10mach  fortran
*
*     minoss   minos1   minos2   minos3
*     mifile   mispec   misolv
*     m1clos   m1envt   m1init
*     m1open   m1page   m1time   m1timp   m1cpu
*
*     mi10vms and mi10unix are the same except for
*     minos2   m1cpu
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine minoss( start, m, n, nb, ne, nname,
     $                   nncon, nnobj, nnjac, 
     $                   iobj, objadd, names,
     $                   a, ha, ka, bl, bu, name1, name2,
     $                   hs, xn, pi, rc, 
     $                   inform, mincor, ns, ninf, sinf, obj,
     $                   z, nwcore )

      implicit           double precision (a-h,o-z)
      character*(10)     start
      character*8        names(5)
      integer*4          ha(ne), hs(nb)
      integer            ka(n+1), name1(nname), name2(nname)
      double precision   a(ne), bl(nb), bu(nb)
      double precision   xn(nb), pi(m), rc(nb), z(nwcore)

*     ------------------------------------------------------------------
*     minoss (pronounced minos-s) is the subroutine version of MINOS.
*     It has all the data passed to it as parameters instead of reading
*     it from an MPS file.
*
*     ON ENTRY:
*
*     start   specifies how a starting basis (and certain other items)
*             are to be obtained.
*             start = 'Cold' means that Crash should be used to choose
*                      an initial basis, unless a basis file is given
*                      via Old basis, Insert or Load in the Specs file.
*             start = 'Basis file' means the same (but is more
*                      meaningful in the latter case).
*             start = 'Warm' means that a basis is already defined in hs
*                      (probably from an earlier call).
*             start = 'Hot' or 'Hot FHS' implies a hot start.
*                      hs defines a basis and an earlier call has
*                      defined certain other things that should also be
*                      kept.  The problem dimensions and the array z(*)
*                      must not have changed.
*                      F refers to the LU factors of the basis.
*                      H refers to the approximate reduced Hessian R.
*                      S refers to column and row scales.
*             start = 'Hot H' (for example) means that only the Hessian
*                      is defined.
*
*     m       is the number of general constraints.  For LP problems
*             this means the number of rows in the constraint matrix A.
*             m > 0 in principle, though sometimes m = 0 may be ok.
*             (Strictly speaking, Fortran declarations of the form
*                double precision   pi(m)
*             require m > 0.  In debug mode, compilers will probably
*             enforce m > 0, but optimized code may run ok with m = 0.)
*
*     n       is the number of variables, excluding slacks.
*             For LP problems, this is the number of columns in A.
*             n > 0.
*
*     nb      is n + m.
*
*     ne      is the number of nonzero entries in A (including the
*             Jacobian for any nonlinear constraints).
*             ne > 0 in principle, though again m = 0, ne = 0 may work
*             with some compilers.
*
*     nname   is the number of column and row names provided in the
*             arrays name1 and name2.  If nname = 1, there are NO names.
*             Generic names will be used in the printed solution.
*             Otherwise, nname = nb and all names must be provided.
*
*     nncon   is the number of nonlinear constraints.
*             nncon ge 0.
*
*     nnobj   is the number of nonlinear objective variables.
*             nnobj ge 0.
*
*     nnjac   is the number of nonlinear Jacobian variables.
*             If nncon = 0, nnjac = 0.
*             if nncon > 0, nnjac > 0.
*
*     iobj    says which row of A is a free row containing a linear
*             objective vector  c  (iobj = 0 if none).
*             iobj = 0  or  nncon < iobj le m.
*
*     objadd  is a constant that will be added to the objective.
*             Typically objadd = 0.0d+0.
*
*     names(5)is a set of 8-character names for the problem, the linear
*             objective, the rhs, the ranges and bounds.  (This is a
*             hangover from MPS files.  The names are used in the
*             printed solution and in some of the basis files.)
*
*     a(ne)   is the constraint matrix (Jacobian), stored column-wise.
*
*     ha(ne)  is the list of row indices for each nonzero in a(*).
*
*     ka(n+1) is a set of pointers to the beginning of each column of
*             the constraint matrix within a(*) and ha(*).
*             Must have ka(1) = 1 and ka(n+1) = ne+1.
*
*  NOTES:  1. If the problem has a nonlinear objective,
*             the first nnobj columns of a and ha belong to the
*             nonlinear objective variables.
*             Subroutine funobj deals with these variables.
*          
*          2. If the problem has nonlinear constraints,
*             the first nnjac columns of a and ha belong to the
*             nonlinear Jacobian variables, and
*             the first nncon rows of a and ha belong to the
*             nonlinear constraints.
*             Subroutine funcon deals with these variables and
*             constraints.
*          
*          3. If nnobj > 0 and nnjac > 0, the two sets of
*             nonlinear variables overlap.  The total number of
*             nonlinear variables is nn = max( nnobj, nnjac ).
*          
*          4. The Jacobian forms the top left corner of a and ha.
*             If a Jacobian column j (1 le j le nnjac) contains
*             any entries a(k), ha(k) associated with nonlinear
*             constraints (1 le ha(k) le nncon), those entries must
*             come before any other (linear) entries.
*          
*          5. The row indices ha(k) for a column may be in any order
*             (subject to Jacobian entries appearing first).
*             Subroutine funcon must define Jacobian entries in the
*             same order.
*          
*          6. If column j contains no entries, perhaps
*             ka(j) = ka(j+1) is acceptable.  (Must check this.
*             When MINOS reads an MPS with empty columns,
*             it inserts a dummy entry a(k) = 0.0d+0, ha(k) = 1.
*             This may not be necessary.)
*
*          7. To allocate storage, MINOS needs to know if the Jacobian
*             is dense or sparse.  The default is dense.  If this is
*             not appropriate, define
*                              Jacobian    Sparse
*             in the Specs file, or
*                call miopt ( 'Jacobian    Sparse', 0, 0, inform )
*             before calling minoss.
*                
*     bl(nb)  is the lower bounds on the variables and slacks (x, s).
*
*     bu(nb)  is the upper bounds on (x, s).
*
*     BEWARE: MINOS represents general constraints as Ax + s = 0.
*             Constraints of the form   l <= Ax <= u  
*             therefore mean            l <= -s <= u,
*             so that                  -u <=  s <= -l.
*             The last m components of bl and bu are -u and -l.
*
*     name1(nname), name2(nname) are two integer arrays.
*             If nname =  1, name1 and name2 are not used.  The printed
*             solution will use generic names for the columns and row.
*             If nname = nb, name1(j) and name2(j) should contain the
*             name of the j-th variable in 2a4 format (j = 1, nb).
*             If j = n+i, the j-th variable is the i-th row.
*
*     hs(nb)  sometimes contains a set of initial states for each
*             variable x or for each column and slack (x, s).
*             See the following NOTES.
*
*     xn(nb)  sometimes contains a set of initial values for each
*             variable x or for each column and slack (x, s).
*             See the following NOTES.
*
*  NOTES:  1. If start = 'Cold' or 'Basis file' and a BASIS file
*             of some sort is to be input
*             (an OLD BASIS file, INSERT file or LOAD file),
*             hs and xn need not be set at all.
*
*          2. Otherwise, hs(j) and xn(j), j=1:n, must be defined for a
*             Cold start.  (The values for j=n+1:nb need not be set.)
*             If nothing special is known about the problem, or if
*             there is no wish to provide special information,
*             you may set hs(j) = 0, xn(j) = 0.0d+0 for all j=1:n.
*             All variables will be eligible for the initial basis.
*        
*             Less trivially, to say that variable j will probably
*             be equal to one of its bounds,
*             set hs(j) = 4 and xn(j) = bl(j)
*             or  hs(j) = 5 and xn(j) = bu(j) as appropriate.
*        
*          3. For Cold starts with no basis file, a Crash procedure
*             is used to select an initial basis.  The initial basis
*             matrix will be triangular (ignoring certain small
*             entries in each column).
*             The values hs(j) = 0, 1, 2, 3, 4, 5 have the following
*             meaning:
*                
*             hs(j)    State of variable j during Crash
*        
*             0, 1, 3  Eligible for the basis.  3 is given preference.
*             2, 4, 5  Ignored.
*        
*             After Crash, hs(j) = 2 entries are made superbasic.
*             Other entries not selected for the basis are made
*             nonbasic at the value xn(j) if bl(j) <= xn(j) <= bu(j),
*             or at the value bl(j) or bu(j) closest to xn(j).
*
*          4. For Warm or Hot starts, all of hs(1:nb) is assumed to be
*             set to the values 0, 1, 2 or 3 (probably from some
*             previous call) and all of xn(1:nb) must have values.
*        
*     pi(m)   contains an estimate of the vector of Lagrange multipliers
*             (shadow prices) for the NONLINEAR constraints.  The first
*             nncon components must be defined.  They will be used as
*             lambda in the subproblem objective function for the first
*             major iteration.  If nothing is known about lambda,
*             set pi(i) = 0.0d+0, i = 1 to nncon.
*
*     ns      need not be specified for Cold starts,
*             but should retain its value from a previous call
*             when a Warm or Hot start is used.
*
*
*     ON EXIT:
*
*     hs(nb)  is the final state vector:
*
*                hs(j)    State of variable j    Normal value of xn(j)
*
*                  0      nonbasic               bl(j)
*                  1      nonbasic               bu(j)
*                  2      superbasic             Between bl(j) and bu(j)
*                  3      basic                  ditto
*
*             Very occasionally there may be nonbasic variables for
*             which xn(j) lies strictly between its bounds.
*             If ninf = 0, basic and superbasic variables may be outside
*             their bounds by as much as the Feasibility tolerance.
*             Note that if Scale is specified, the Feasibility tolerance
*             applies to the variables of the SCALED problem. 
*             In this case, the variables of the original problem may be
*             as much as 0.1 outside their bounds, but this is unlikely
*             unless the problem is very badly scaled.
*
*     xn(nb)  is the final variables and slacks (x, s).
*
*     pi(m)   is the vector of Lagrange multipliers (shadow prices)
*             for the general constraints.
*
*     rc(nb)  is a vector of reduced costs: rc = g - (A I)'pi, where g
*             is the gradient of the objective function if xn is feasible
*             (or the gradient of the Phase-1 objective otherwise).
*             If ninf = 0, the last m entries are -pi (negative pi).
*
*     inform  says what happened; see Chapter 6.3 of the User's Guide.
*             A summary of possible values follows:
*
*             inform   Meaning
*
*                0     Optimal solution found.
*                1     The problem is infeasible.
*                2     The problem is unbounded (or badly scaled).
*                3     Too many iterations.
*                4     Apparent stall.  The solution has not changed
*                      for a large number of iterations (e.g. 1000).
*                5     The Superbasics limit is too small.
*                6     Subroutine funobj or funcon requested termination
*                      by returning mode < 0.
*                7     Subroutine funobj seems to be giving incorrect
*                      gradients.
*                8     Subroutine funcon seems to be giving incorrect
*                      gradients.
*                9     The current point cannot be improved.
*               10     Numerical error in trying to satisfy the linear
*                      constraints (or the linearized nonlinear
*                      constraints).  The basis is very ill-conditioned.
*               11     Cannot find a superbasic to replace a basic
*                      variable.
*               12     Basis factorization requested twice in a row.
*                      Should probably be treated as inform = 9.
*               13     Near-optimal solution found.
*                      Should probably be treated as inform = 9.
*
*               20     Not enough storage for the basis factorization.
*               21     Error in basis package.
*               22     The basis is singular after several attempts to
*                      factorize it (and add slacks where necessary).
*
*               30     An OLD BASIS file had dimensions that did not
*                      match the current problem.
*               32     System error.  Wrong number of basic variables.
*
*               40     Fatal errors in the MPS file.
*               41     Not enough storage to read the MPS file.
*               42     Not enough storage to solve the problem.
*
*     mincor  says how much storage is needed to solve the problem.
*             If inform = 42, the work array z(nwcore) was too small.
*             minoss may be called again with nwcore suitably larger
*             than mincor.  (The bigger the better, since it is
*             not certain how much storage the basis factors need.)
*
*     ns      is the final number of superbasics.
*
*     ninf    is the number of infeasibilities.
*
*     sinf    is the sum    of infeasibilities.
*
*     obj     is the value  of the objective function.
*             If ninf = 0, obj includes the nonlinear objective if any.
*             If ninf > 0, obj is just the linear objective if any.
*
*     30 Sep 1991: First version.
*     06 Dec 1991: A few more output parameters.
*     10 Apr 1992: Parameter  objadd added.  Parameters reordered.
*     20 Apr 1992: Parameters nname, name1, name2 added.
*     27 Apr 1992: Parameter  mincor added to allow reentry with more
*                  storage.
*     27 Jun 1992: Parameter  start  implemented.  Passed to misolv.
*     ------------------------------------------------------------------
*     NOTE:
*     In /m7len / and /m8len /, nnobjx, nnconx, nnjacx are normally
*                               nnobj , nncon , nnjac .
*     Here it is better to save those names for the minoss parameters.

      common    /m2len / mrows,mcols,melms
      common    /m2mapz/ maxw  ,maxz
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m7len / fobj  ,fobj2 ,nnobjx,nnobj0
      common    /m8len / njac  ,nnconx,nncon0,nnjacx
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /cycle2/ objtru,suminf,numinf

      character*5        f1
      data               f1 /'(2a4)'/
*     ------------------------------------------------------------------

*     Initialize timers.

      call m1time( 0,0 )

*     Load the Common variables with various problem dimensions.

      mrows  = m
      mcols  = n
      melms  = ne
      nnconx = nncon
      nnobjx = nnobj
      nnjacx = nnjac

*     Say how much z(*) we've got, in case mispec wasn't called,
*     or nwcore has been altered since mispec was called.
*     This means the Specs file can't set  Workspace (TOTAL) .

      maxz   = nwcore

*     The Specs file has been read (or the options have been
*     otherwise defined).  Check that the options have sensible values.

      call m3dflt( 2 )

*     ------------------------------------------------------------------
*     Determine storage requirements using the
*     following Common variables:
*        (m2len )   mrows, mcols, melms
*        (m3len )   nscl  (determined by lscale)
*        (m5len )   maxr, maxs, nn
*        (m7len )   nnobj
*        (m8len )   njac, nncon, nnjac
*     All have to be known exactly before calling m2core( 4, ... ).
*     The only one in doubt is njac, the number of Jacobian elements.
*     If Jacobian = dense  (nden = 1), m2core sets njac = nncon*nnjac.
*     If Jacobian = sparse (nden = 2), we have to set njac here.
*     ------------------------------------------------------------------

      njac = 0
      if (nncon .gt. 0  .and.  nden .eq. 2) then
         last = ka(nnjac+1) - 1
         if (nncon .eq. m) then
            njac = last
         else
            do 100 k = 1, last
               i     = ha(k)
               if (i .le. nncon) njac = njac + 1
  100       continue         
         end if
      end if

      call m2core( 4, mincor )

      if (mincor .gt. nwcore) then
         inform = 42
         return
      end if

*     ------------------------------------------------------------------
*     Open files needed for this problem.
*     Print the options if iprint > 0, Print level > 0 and iparm(3) > 0.
*     ------------------------------------------------------------------
      call mifile( 2 )
      call m3dflt( 3 )

*     ------------------------------------------------------------------
*     Load names into the MINOS arrays.
*     ------------------------------------------------------------------
      read (names(1), f1) name
      read (names(2), f1) mobj
      read (names(3), f1) mrhs
      read (names(4), f1) mrng
      read (names(5), f1) mbnd
      
*     ------------------------------------------------------------------
*     Solve the problem.
*     ------------------------------------------------------------------
      mimode = 2
      nka    = n + 1
      call misolv( mimode, start, m, n, nb, ne, nka, nname,
     $             iobj, objadd, 
     $             a, ha, ka, bl, bu, name1, name2,
     $             hs, xn, pi, rc, 
     $             inform, ns, z, nwcore )

      ninf   = numinf
      sinf   = suminf
      obj    = objtru

*     Print times for all clocks (if ltime > 0).

      call m1time( 0,2 )

*     end of minoss
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine minos1( z, nwcore )

      implicit           double precision (a-h,o-z)
      double precision   z(nwcore)

*     ------------------------------------------------------------------
*     minos1 is used for the stand-alone version of MINOS.
*     It is called by the main program (or equivalent driver).
*     It repeatedly looks for a new problem in the SPECS file
*     and asks for it to be solved, until opfile returns inform gt 1,
*     which means an ENDRUN card was found in the SPECS file,
*     or end-of-file was encountered.
*
*     06 Oct 1985: minos1 calls minos2 to allow reallocation of z(*),
*                  following suggestions from David Gay,
*                  AT&T Bell Laboratories (for Unix).
*     22 Dec 1987: For DEC VAX VMS, newz tells minos2 whether or not to
*                  re-allocate z(*).
*     03 Mar 1988: f77 version calls opfile.
*     01 Oct 1991: mispec and misolv implemented.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m1savz/ nbytes,newz
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2mapz/ maxw  ,maxz
      common    /m8len / njac  ,nncon ,nncon0,nnjac

      external           m3key
      character*30       title

      newz   = 0

*     ------------------------------------------------------------------
*     Define global files (reader, printer, etc.)
*     ------------------------------------------------------------------
      ispecs = 4
      iprint = 9
      isumm  = 6
      call mifile( 1 )

*     ==================================================================
*     Loop through each problem in the SPECS file.
*     ==================================================================
      do 100 loop = 1, 100000
         ncalls = loop
         maxw   = 0
         maxz   = nwcore

*        Initialize timers.
      
         call m1time( 0,0 )
      
*        ---------------------------------------------------------------
*        Define the MINOS title and read the Specs file.
*        ---------------------------------------------------------------
         call m1init( title )
         call opfile( ncalls, ispecs, m3key,
     $                title , iprint, isumm, inform )
      
         if (inform .ge. 2) then
            inform = 100 + inform 
            return
         end if
      
*        ---------------------------------------------------------------
*        Check options.
*        Open files needed for this problem.
*        ---------------------------------------------------------------
         call m3dflt( 2 )
         call mifile( 2 )
      
*        ---------------------------------------------------------------
*        Estimate storage requirements using the
*        following Common variables:
*           (m2len )   mrows, mcols, melms
*           (m3len )   nscl
*           (m5len )   maxr, maxs
*           (m7len )   nnobj
*           (m8len )   njac, nncon, nnjac
*        All except njac have been set by default or by the SPECS file.
*        We haven't read the MPS file yet, so m2core estimates njac.
*        ---------------------------------------------------------------
         call m2core( 1, mincor )
      
*        ---------------------------------------------------------------
*        Solve the problem.
*        ---------------------------------------------------------------
         call minos2( z, nwcore, mincor, inform )
  100 continue
*     ==================================================================
*     End of loop through SPECS file.
*     ==================================================================

*     end of minos1
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine minos2( z, nwcore, mincor, inform )

      implicit           double precision (a-h,o-z)
      double precision   z(nwcore)

*     ------------------------------------------------------------------
*     minos2 asks minos3 to solve the problem just found in the SPECS
*     file.  For standard Fortran installations, that is all.
*
*     For some installations, this is an appropriate place to
*     increase the size of the workspace array  z,  to allow
*     solution of arbitrarily large problems without recompiling
*     the main program, in which z(nwcore) is originally declared.
*
*     z  might have to be in blank common if it is to be expanded.
*     The default size  nwcore  should be large enough to solve
*     reasonably big problems without change (e.g., nwcore = 100000).
*
*     At this stage, the SPECS file has been read and values are known
*     for maxw, maxz and mincor.  The default values for the first two
*     are maxw = 0 and maxz = nwcore, but we allow the user to alter
*     these by means of two cards in the SPECS file of the following
*     form...
*
*        Workspace (user)      10000 words  (This sets  maxw = 10000)
*        Workspace (total)     90000 words  (This sets  maxz = 90000)
*
*     MINOS will use only  Z(maxw+1), ..., Z(maxz).  Hence,
*     z(1), ..., z(maxw)    and possibly   z(maxz+1), ..., z(nwcore)
*     may be used as workspace by the user during solution of this
*     particular problem (e.g., within funobj or funcon).
*
*     If maxz is set to a value less than nwcore, it may serve to
*     reduce paging activity on a machine with virtual memory, by
*     confining MINOS (in particular the basis-factorization routines)
*     to an area of core that is sensible for the current problem.
*     On some systems (e.g., Burroughs), this will allow z(nwcore)
*     to be declared arbitrarily large at compile time.
*
*     mincor contains an estimate of the minimum core requirements,
*     allowing for maxw but ignoring maxz.  z is already large enough
*     if nwcore .ge. max( maxz, mincor ).  Systems that allow z to be
*     re-allocated at run-time should make appropriate use of the
*     logical variable enough.
*
*     22 Dec 1987: VAX dynamic storage added by Steve White, DSIR.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m1savz/ nbytes,newz
      common    /m2mapz/ maxw  ,maxz

      intrinsic          max, min
      logical            enough

      if (iprint .gt. 0) write(iprint, 1100) maxw, mincor
      nwcor2 = max( maxz, mincor )
*-->  enough = nwcore .ge. nwcor2
      enough = .true.

      if ( enough ) then

*        Use the storage already available.

         maxz   = min( maxz, nwcore )
         if (iprint .gt. 0) write(iprint, 1200) maxw, maxz, nwcore
         call minos3( z, nwcore, inform )
      else
*-->
*        Re-allocate  z  (using some non-standard Fortran).
*        newz says if we have to free z from an earlier call.
*        The following is for DEC VAX/VMS.
*
*        nwcor2 = max( nwcore, maxz, mincor )
*        if (newz .ne. 0) then
*           istat = lib$free_vm( nbytes, newz )
*           if (.not. istat) then
*              call lib$signal( %val(istat) )
*           end if
*        end if
*        maxz   = nwcor2
*        nbytes = nwcor2*8
*        istat  = lib$get_vm( nbytes, newz )
*        if (.not. istat) then
*           call lib$signal( %val(istat) )
*        else
*           if (iprint .gt. 0) write(iprint, 1200) maxw, maxz, nwcor2
*           call minos3( %val(newz), nwcor2, inform )
*        end if
      end if
      return

 1100 format(/ ' Reasonable Workspace limits are', i10, ' ...', i8)
 1200 format(  ' Actual     Workspace limits are', i10, ' ...',
     $   i8, ' ...', i8, '  words of  z.')

*     end of minos2
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine minos3( z, nwcore, inform )

      implicit           double precision (a-h,o-z)
      double precision   z(nwcore)

*     ------------------------------------------------------------------
*     minos3 prints the options (sometimes),
*     then reads a problem from an MPS file
*     and asks misolv to solve it.
*
*     01 Oct 1991: misolv implemented.
*     20 Apr 1992: name1, name2, nname added as parameters to misolv.
*     26 Apr 1992: call m3dflt( 3 ) added.
*     ------------------------------------------------------------------

      common    /m2mapa/ ne    ,nka   ,la    ,lha   ,lka
      common    /m3len / m     ,n     ,nb    ,nscl
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m3mps1/ lname1,lname2,lkeynm,nname
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m5lobj/ sinf,wtobj,minimz,ninf,iobj,jobj,kobj
      common    /m5log1/ idebug,ierr  ,lprint
*     ------------------------------------------------------------------

      call m3dflt( 3 )
      call m1time( 1,0 )
      call m3inpt( objadd, z, nwcore )
      call m1time(-1,0 )

      if (ierr .ne. 0) return

      mimode = 1
      nname  = nb
      lrc    = lpi + m
      
      call misolv( mimode, 'Cold', m, n, nb, ne, nka, nname,
     $             iobj  , objadd,
     $             z(la),  z(lha), z(lka), z(lbl), z(lbu),
     $             z(lname1),   z(lname2),
     $             z(lhs), z(lxn), z(lpi), z(lrc), 
     $             inform, ns    , z     , nwcore )

*     Print times for all clocks (if ltime > 0).

      call m1time( 0,2 )

*     end of minos3
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine mifile( mode )

*     ------------------------------------------------------------------
*     mifile  is a (possibly) machine-dependent routine for opening
*     various files.
*     Some systems (e.g. DEC VAX/VMS) open files automatically.
*     Other systems may require explicit OPEN statements.
*
*     19 Jun 1989: Modified to call m1open.
*     11 Nov 1991: mode 1 now assumes that ispecs, iprint, isumm
*                  have been set by minos1 (for MINOS)
*                             or by mispec (for minoss).
*
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt

      integer            iprinx, isummx
      save               iprinx, isummx

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        Mode 1: Open default files (if opens are necessary).
*
*        iread and iprint have the following use:
*        Input  files (MPS, INSERT, LOAD, OLD BASIS)     are rewound
*        after being read,    but not if they are the same as  iread.
*        Output files (DUMP, NEW BASIS, PUNCH, SOLUTION) are rewound
*        after being written, but not if they are the same as iprint.
*     
*        iread  = (conceptually) the CARD READER that can't be rewound.
*                 If there are no such units, use 0.
*                 MINOS does not use this file directly, so there is
*                 no 'open'.
*        iprint = the PRINT file.
*        isumm  = the SUMMARY file.  Sometimes this is the screen.
*                 If so, it may not need to be opened.
*        ispecs = the SPECS file, containing one or more problem specs.
*                 This file is not rewound after use, because it may
*                 contain another SPECS file.
*        iscr   = SCRATCH file -- no longer used.
*
*        Files may be opened directly or by calling m1open, e.g.
*           if (iprint .gt. 0) open( unit=iprint, file='minos.print' )
*        or
*           call m1open( iprint, 'OUT' )
*
*        iread and ispecs remain the same throughout the run.
*        iprint and isumm may be altered by the SPECS file
*        and therefore need to be opened twice.
*        ---------------------------------------------------------------
      
         iread  = 5
         iprinx = iprint
         isummx = isumm
         if (ispecs .gt. 0) open(unit=ispecs, file='minos.specs', 
     *                           status='OLD')
         if ((iprint .gt. 0) .and. (iprint .ne. 6))
     *     open(unit=iprint, file='minos.print', status='UNKNOWN')
         if ((isumm  .gt. 0) .and. (isumm .ne. 6)) 
     *     open(unit=isumm, file='minos.summary', status='UNKNOWN')
      else      
*        ---------------------------------------------------------------
*        Mode 2: Define files mentioned in the SPECS file just read.
*        Input files are opened first.  Only one basis file is needed.
*        ---------------------------------------------------------------
         if (imps  .le. 0     ) imps = ispecs
         if (imps  .ne. ispecs) open(unit=imps, file='minos.mps',
     *                               status='OLD')
      
         if      (ioldb .gt. 0) then
                           open(unit=ioldb, file='minos.oldb',
     *                          status='OLD')
         else if (insrt .gt. 0) then
                           open(unit=insrt, file='minos.insert',
     *                          status='OLD')
         else if (iload .gt. 0) then
                           open(unit=iload, file='minos.load',
     *                          status='OLD')
         end if
         
         if (iback .gt. 0) open(unit=iback, file='minos.back',
     *                          status='UNKNOWN')
         if (idump .gt. 0) open(unit=idump, file='minos.dump',
     *                          status='UNKNOWN')
         if (inewb .gt. 0) open(unit=inewb, file='minos.newb',
     *                          status='UNKNOWN')
         if (ipnch .gt. 0) open(unit=ipnch, file='minos.punch',
     *                          status='UNKNOWN')
         if (isoln .gt. 0) open(unit=isoln, file='minos.soln',
     *                          status='UNKNOWN')

*        Open new Print or Summary files if they were altered
*        by the Specs file.

         if (iprint .ne. iprinx) then
            if (iprint .gt. 0) open(unit=iprint, file='minos.print',
     *                              status='UNKNOWN')
         end if
         if (isumm  .ne. isummx) then
            if (isumm  .gt. 0) open(unit=isumm, file='minos.summary',
     *                              status='UNKNOWN')
         end if
      end if
   
*     end of mifile
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine mispec( ispecx, iprinx, isummx, nwcore, inform )

*     ------------------------------------------------------------------
*     mispec  is called by minoss (not by MINOS) to do the following:
*     1. Open default files (Specs, Print, Summary).
*     2. Initialize title.
*     3. Set options to default values.
*     4. Read the Specs file if any.
*
*     01 Oct 1991: First version.
*     27 Jun 1992: Don't read a Specs file if ispecs <= 0.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm
      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2mapz/ maxw  ,maxz

      external           m3key
      character*30       title

*     Open the specified Specs, Print and Summary files.

      ispecs = ispecx
      iprint = iprinx
      isumm  = isummx
      call mifile( 1 )

      ncalls = 1
      maxw   = 0
      maxz   = nwcore
      inform = 0

*     ------------------------------------------------------------------
*     Define the MINOS title and read the Specs file (if any).
*     minoss will check the options later and maybe print them.
*     ------------------------------------------------------------------
      call m1init( title )
      if (ispecs .gt. 0) then
         call opfile( ncalls, ispecs, m3key,
     $                title , iprint, isumm, inform )
      else
*        Just set options to default values.
         call m3dflt( 1 )
      end if

*     end of mispec
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine misolv( mimode, start,
     $                   mxx, nxx, nbxx, nexx, nkax, nnamex,
     $                   iobjxx, objadd,
     $                   a, ha, ka, bl, bu, name1, name2,
     $                   hs, xn, pi, rc, 
     $                   inform, ns, z, nwcore )

      implicit           double precision (a-h,o-z)
      character*(*)      start
      integer*4          ha(nexx), hs(nbxx)
      integer            ka(nkax), name1(nnamex), name2(nnamex)
      double precision   a(nexx) , bl(nbxx), bu(nbxx)
      double precision   xn(nbxx), pi(mxx) , rc(nxx) , z(nwcore)

*     ------------------------------------------------------------------
*     misolv solves the current problem.
*
*     On entry,
*     the SPECS file has been read,
*     all data items have been loaded (including a, ha, ka, ...),
*     and workspace has been allocated within z.
*
*     mimode  =  1 if the call is from minos3 (stand-alone MINOS).
*            ge  2 if the call is from minoss.
*
*     On exit,
*     inform  =  0 if an optimal solution was found,
*             =  1 if the problem was infeasible,
*             =  2 if the problem was unbounded,
*             =  3 if the Iteration limit was exceeded,
*            ge  4 if iterations were terminated by some other
*                  error condition (see the MINOS user's guide).
*
*     01 Oct 1991: minoss, mispec and misolv implemented.
*                  minos1, minos2, minos3 reorganized
*                  to facilitate calling MINOS as a subroutine.
*     25 Nov 1991: nname and rc added as parameters of matmod.
*     10 Apr 1992: objadd added as input parameter.
*     20 Apr 1992: Parameter list revised.  nname, name1, name2 added.
*     27 Jun 1992: Cold, Warm, Hot start implemented.
*     09 Jul 1992: ns initialized here only for Cold starts, just to
*                  help debugging.  m4chek always sets it later.
*     ------------------------------------------------------------------
*
*     All common blocks are listed here for reference.
*
*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
*     File mcommon fortran.


*  Machine-dependent items.

      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1file/ iread,iprint,isumm
      common    /m1savz/ nbytes,newz
      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime
      common    /m1word/ nwordr,nwordi,nwordh


*  Files, maps, parameters.

      common    /m2file/ iback,idump,iload,imps,inewb,insrt,
     $                   ioldb,ipnch,iprob,iscr,isoln,ispecs,ireprt
      common    /m2len / mrows,mcols,melms
      common    /m2lu1 / minlu,maxlu,lena,nbelem,ip,iq,lenc,lenr,
     $                   locc,locr,iploc,iqloc,lua,indc,indr
      common    /m2lu2 / factol(5),lamin,nsing1,nsing2
      common    /m2lu3 / lenl,lenu,ncp,lrow,lcol
      common    /m2lu4 / parmlu(30),luparm(30)
      common    /m2mapa/ ne    ,nka   ,la    ,lha   ,lka
      common    /m2mapz/ maxw  ,maxz
      common    /m2parm/ dparm(30),iparm(30)


*  Problem size, MPS names, Scale options.

      common    /m3len / m     ,n     ,nb    ,nscl
      common    /m3loc / lascal,lbl   ,lbu   ,lbbl  ,lbbu  ,
     $                   lhrtyp,lhs   ,lkb
      common    /m3mps1/ lname1,lname2,lkeynm,nname
      common    /m3mps3/ aijtol,bstruc(2),mlst,mer,
     $                   aijmin,aijmax,na0,line,ier(20)
      common    /m3mps4/ name(2),mobj(2),mrhs(2),mrng(2),mbnd(2),minmax
      common    /m3mps5/ aelem(2), id(6), iblank
      common    /m3scal/ sclobj,scltol,lscale


*  LP items.

      common    /m5len / maxr  ,maxs  ,mbs   ,nn    ,nn0   ,nr    ,nx
      common    /m5loc / lpi   ,lpi2  ,lw    ,lw2   ,
     $                   lx    ,lx2   ,ly    ,ly2   ,
     $                   lgsub ,lgsub2,lgrd  ,lgrd2 ,
     $                   lr    ,lrg   ,lrg2  ,lxn
      common    /m5freq/ kchk,kinv,ksav,klog,ksumm,i1freq,i2freq,msoln
      common    /m5inf / prinf, duinf, jprinf, jduinf
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


*  Nonlinear objective.

      logical            conv,restrt
      common    /m7len / fobj  ,fobj2 ,nnobj ,nnobj0
      common    /m7loc / lgobj ,lgobj2
      common    /m7cg1 / cgbeta,itncg,msgcg,modcg,restrt
      common    /m7cg2 / lcg1,lcg2,lcg3,lcg4,modtcg,nitncg,nsubsp
      common    /m7conv/ etash,etarg,lvltol,nfail,conv(4)
      common    /m7phes/ rgmin1,rgnrm1,rgnrm2,jz1,jz2,labz,nfullz,mfullz
      common    /m7tols/ xtol(2),ftol(2),gtol(2),pinorm,rgnorm,tolrg


*  Nonlinear constraints.

      common    /m8len / njac  ,nncon ,nncon0,nnjac
      common    /m8loc / lfcon ,lfcon2,lfdif ,lfdif2,lfold ,
     $                   lblslk,lbuslk,lxlam ,lrhs  ,
     $                   lgcon ,lgcon2,lxdif ,lxold
      common    /m8al1 / penpar,rowtol,ncom,nden,nlag,nmajor,nminor
      common    /m8al2 / radius,rhsmod,modpen,modrhs
      common    /m8diff/ difint(2),gdummy,lderiv,lvldif,knowng(2)
      common    /m8func/ nfcon(4),nfobj(4),nprob,nstat1,nstat2
      common    /m8save/ vimax ,virel ,maxvi ,majits,minits,nssave
      common    /m8veri/ jverif(4),lverif(2)


*  Miscellaneous.

      logical            gotbas,gotfac,gothes,gotscl
      common    /cycle1/ gotbas,gotfac,gothes,gotscl
      common    /cycle2/ objtru,suminf,numinf
      common    /cyclcm/ cnvtol,jnew,materr,maxcy,nephnt,nphant,nprint

***** end of file mcommon fortran.
*     ------------------------------------------------------------------

      character*1        ch1
      logical            finish, lincon, nlncon, nlnobj

*     For minoss we have to copy m, n, etc into common.

      m      = mxx
      n      = nxx
      nb     = nbxx
      ne     = nexx
      nka    = nkax
      nname  = nnamex
      iobj   = iobjxx

*     Initialize a few things.

      lincon = nncon  .eq. 0
      nlncon = nncon  .gt. 0
      nlnobj = nnobj  .gt. 0
      ierr   = 0
      lenl   = 0
      lenu   = 0
      ncycle = 0
      ninf   = 0
      nstat1 = 1
      nstat2 = 1
      sclobj = 1.0d+0
      jobj   = 0
      if (iobj .gt. 0) jobj = n + iobj

*     ------------------------------------------------------------------
*     Decode 'start'.
*     ------------------------------------------------------------------
      gotbas = .false.
      gotfac = .false.
      gothes = .false.
      gotscl = .false.
      ch1    = start(1:1)

      if      (ch1 .eq. 'C'  .or.  ch1 .eq. 'c'  .or.
     $         ch1 .eq. 'B'  .or.  ch1 .eq. 'b') then

*        Cold start  or  Basis file.

         istart = 0
         gotbas = (ioldb + insrt + iload) .gt. 0
         ns     = 0

      else if (ch1 .eq. 'W'  .or.  ch1 .eq. 'w') then

*        Warm start.

         istart = 1
         gotbas = .true.

      else if (ch1 .eq. 'H'  .or.  ch1 .eq. 'h') then

*        Hot start.
*        'Hot' is the same as 'Hot FHS'.
*        Look for 'Hot F', 'Hot FH', etc.

         istart = 2
         gotbas = .true.
         nchar  = len( start )
         if (nchar .lt. 5) then
            gotfac = .true.
            gothes = .true.
            gotscl = .true.
         else
            do 100 j = 5, nchar
               ch1 = start(j:j)
               if (ch1 .eq. 'F'  .or.  ch1 .eq. 'f') gotfac = .true.
               if (ch1 .eq. 'H'  .or.  ch1 .eq. 'h') gothes = .true.
               if (ch1 .eq. 'S'  .or.  ch1 .eq. 's') gotscl = .true.
  100       continue
         end if

         if (iprint .gt. 0) then
            write(iprint, 1020) gotbas, gotfac, gothes, gotscl
         end if
         
      else
         istart = 0
         if (iprint .gt. 0) write(iprint, 1030) start
         if (isumm  .gt. 0) write(isumm , 1030) start
      end if

      nssave = ns

*     ------------------------------------------------------------------
*     1. Fiddle with partial price parameter to avoid foolish values.
*        We reduce nparpr if both the row and column section sizes
*        would be smaller than minprc (= 10 say).
*     2. Change Scale option 1 to 0 if all variables are nonlinear.
*     ------------------------------------------------------------------
      minprc = 10
      npr1   = n / nparpr
      npr2   = m / nparpr
      if (max( npr1, npr2 ) .lt. minprc) then
         maxmn  = max( m, n )
         nparpr = maxmn / min( maxmn, minprc )
         npr1   = n / nparpr
         npr2   = m / nparpr
      end if

      if (lscale .eq. 1  .and.  nn .eq. n) lscale = 0

      if (iprint .gt. 0) write(iprint, 1100) lscale, nparpr, npr1, npr2
      if (isumm  .gt. 0) write(isumm , 1110) lscale, nparpr

*     ------------------------------------------------------------------
*     Set the vector of row types and print the matrix statistics.
*     ------------------------------------------------------------------
      call m2amat( 1, m, n, nb,
     $             ne, nka, a, ha, ka,
     $             bl, bu, z(lhrtyp) )

*     ------------------------------------------------------------------
*     Load Jacobian elements in the MPS file from A into gcon and gcon2.
*     ------------------------------------------------------------------
      if (nlncon) then
         call m8augl( 1, m, n, nb, ns, inform,
     $                ne, nka, a, ha, ka,
     $                hs, bl, bu, xn, z, nwcore )
      end if

*     ------------------------------------------------------------------
*     Input a basis file if one exists, thereby defining hs and xn.
*     (Otherwise, m2crsh will be called later to define hs.)
*     At this stage, ncycle = 0.
*     ------------------------------------------------------------------
      call m1page( 1 )
      if (iprint .gt. 0) then
         write(iprint, 1200)
         if (.not. gotbas) write(iprint, 1210)
      end if

      call m4getb( ncycle, istart, m, mbs, n, nb, nn, nname, nscl,
     $             lcrash, ns,
     $             ne, nka, a, ha, ka,
     $             z(lhrtyp), hs, z(lkb), z(lascal), bl, bu,
     $             pi, xn, z(ly), z(ly2), name1, name2,
     $             z, nwcore )
      if (ierr .ne. 0) go to 900


*     ------------------------------------------------------------------
*                             CYCLE  PROCEDURE
*
*     The following notes are relevant if Cycle limit = 2 or more.
*
*  1. Scaling and/or Crash are controlled on each cycle by the following
*     logical variables:
*
*     If gotscl is true, scales are retained from the previous cycle.
*                        Otherwise, scales are recomputed (if lscale>0).
*
*     If gotbas is true, the basis is retained.  Otherwise, Crash is
*                        called.
*
*
*  2. When m5solv is called, Flying Starts are controlled by the
*     following logical variables:
*
*     If gotfac is true, an LU factorization of the basis is assumed
*                        to be present.  (Ignored if there are any
*                        nonlinear constraints.)
*
*     If gothes is true, z(lr) is assumed to contain a useful
*                        reduced-Hessian approximation.
*
*
*  3. For the next cycle,
*        m4getb sets gotscl and gotbas to be true, and
*        m5solv sets gotfac and gothes to current values (usually true).
*     These values will often be appropriate.  However, the expert user
*     of matmod must set some or all of the logical variables to .false.
*     if the problem data or state have been significantly altered.
*
*     For example, if the Jacobian was used by the scaling routine
*     (Scale option 2) and if the Jacobian could be rather different
*     from its value at the start of the previous cycle, it may be
*     advisable to request new scales by setting gotscl = .false.
*
*     Similarly, if matmod alters some matrix elements in columns that
*     are currently basic, one should set gotfac = .false. to force
*     refactorization.  In particular, if the linear objective row c is
*     altered, gotfac should be set to .false., since c is part of the
*     LU factors.
*     ------------------------------------------------------------------

      finish = .false.
      jnew   = n - nphant
      materr = 0
      nprntd = 0
      nsolvd = ncycle

*     If Cycle limit is more than 1, call matmod with ncycle = 0 in case
*     the user wants a chance to set things up before any solves.

      if (maxcy .gt. 1) then
         if (iprint .gt. 0) write(iprint, 3000) ncycle
         if (isumm  .gt. 0) write(isumm , 3000) ncycle
      
         call matmod( ncycle, nprob, finish,
     $                m, n, nb, ne, nka, ns, nscl, nname,
     $                a, ha, ka, bl, bu,
     $                z(lascal), hs, name1, name2,
     $                xn, pi, rc, z, nwcore )
         if (finish) go to 800
      end if

*     ------------------------------------------------------------------
*     Check gradients if requested, before scaling interferes.
*     m4getb has made sure nonlinear xn(j)s are within their bounds.
*     ------------------------------------------------------------------
      if (nlncon) then
         call m8chkj( nncon, nnjac, njac, nx,
     $                ne, nka, ha, ka,
     $                bl, bu, z(lfcon), z(lfcon2),
     $                z(lgcon), z(lgcon2),
     $                xn, z(ly), z(ly2), z, nwcore )
         if (ierr .gt. 0) go to 900
      end if

      if (nlnobj) then
         call m7chkg( nnobj,
     $                bl, bu, z(lgobj), z(lgobj2),
     $                xn, z(ly), z(ly2), z, nwcore )
         if (ierr .gt. 0) go to 900
      end if

*     ==================================================================
*     Start of the Cycle loop.
*     ==================================================================
  500    ncycle = ncycle + 1
         nsolvd = ncycle
         ierr   = 0
         if (.not. gotbas) then
            gotfac = .false.
            gothes = .false.
         end if
         if (ncycle .gt. 1) then
            call m1page( 1 )
            if (iprint .gt. 0) then
               write(iprint, 2000) ncycle
               write(iprint, 1020) gotbas, gotfac, gothes, gotscl
            end if
            if (isumm  .gt. 0) then
               write(isumm , 2000) ncycle
            end if
         end if
      
*        Make sure the Jacobian variables are inside their bounds.
      
         if (nlncon) then
            call m8augl( 2, m, n, nb, ns, inform,
     $                   ne, nka, a, ha, ka,
     $                   hs, bl, bu, xn, z, nwcore )
         end if
      
*        For the first cycle, the row types have been set by m2amat.
*        Reset them for later cycles in case m2scal or m2crsh are
*        called.
      
         if (ncycle .ge. 2) then
            call m2amat( 2, m, n, nb,
     $                   ne, nka, a, ha, ka,
     $                   bl, bu, z(lhrtyp) )
         end if
      
*        ---------------------------------------------------------------
*        Evaluate the Jacobian and store it in A (unscaled) for the
*        first major iteration.
*        Compute scales from a, bl, bu (except if gotscl is true).
*        Scale a, bl, bu, xn, pi and fcon.
*        Initialize xlam from pi.
*        Call CRASH if a basis file was not supplied
*        (or if gotbas is false).
*        ---------------------------------------------------------------
         call m4getb( ncycle, istart, m, mbs, n, nb, nn, nname, nscl,
     $                lcrash, ns,
     $                ne, nka, a, ha, ka,
     $                z(lhrtyp), hs, z(lkb), z(lascal), bl, bu,
     $                pi, xn, z(ly), z(ly2), name1, name2,
     $                z, nwcore )
         if (ierr .ne. 0) go to 900
      
*        1. Set ns to match hs(*).
*        2. Set kb(m+1) thru kb(m+ns) to define the initial set of
*           superbasics, except if a Hot start
*           (gotbas and gothes are both true).
*        3. Check that nonbasic xn are within bounds.
      
         call m4chek( m, maxs, mbs, n, nb, ns,
     $                hs, z(lkb), bl, bu, xn )
      
*        ---------------------------------------------------------------
*        Solve the current problem.
*        Bail out if there is a fatal error.
*        ---------------------------------------------------------------
         call m1page( 1 )
         if (iprint .gt. 0) write(iprint, 2100)
      
         call m1time( 2,0 )
         call m5solv( m, maxr, maxs, mbs, n, nb, nn, nn0, nr,
     $                lcrash, ns, nscl, nx, objadd,
     $                ne, nka, a, ha, ka,
     $                z(lhrtyp), hs, z(lkb), z(lascal), bl, bu,
     $                z(lbbl), z(lbbu), fsub, z(lgsub),
     $                z(lgrd), z(lgrd2),
     $                pi, z(lr), rc, z(lrg), z(lrg2),
     $                z(lx), xn, z(ly), z(ly2), z, nwcore )
         call m1time(-2,0 )
      
         if (ierr .ge. 30                   ) go to 900
         if (ierr .ge. 20  .and.  itn .eq. 0) go to 900
      
*        ---------------------------------------------------------------
*        Unscale, compute nonlinear constraint violations,
*        save basis files and prepare to print the solution.
*        Clock 3 is "Output time".
*        ---------------------------------------------------------------
         call m1time( 3,0 )
         call m4savb( 1, m, mbs, n, nb, nn, nname, nscl, msoln, ns,
     $                ne, nka, a, ha, ka,
     $                hs, z(lkb), z(lascal), bl, bu,
     $                name1, name2,
     $                pi, rc, xn, z(ly), z, nwcore )
      
*        In some Cycling applications, it may be desirable to suppress
*        the printing of intermediate solutions.  Otherwise if mode = 2,
*        m4savb prints the solution under the control of msoln
*        (which is set by the  Solution  keyword in the SPECS file).
*        The printed solution may or may not be wanted, as follows:
*     
*        msoln  = 0   means      No
*               = 1   means      If optimal, infeasible or unbounded
*               = 2   means      Yes
*               = 3   means      If error condition
*     
*        This call normally prints the solution when there is no
*        Cycling, because the default values are  maxcy = nprint = 1.
      
         if (ncycle .gt. maxcy - nprint) then
            nprntd = nsolvd
            call m4savb( 2, m, mbs, n, nb, nn, nname, nscl, msoln, ns,
     $                ne, nka, a, ha, ka,
     $                hs, z(lkb), z(lascal), bl, bu,
     $                name1, name2,
     $                pi, rc, xn, z(ly), z, nwcore )
         end if
         call m1time(-3,0 )
      
*        ---------------------------------------------------------------
*        Call the functions one last time with  nstate .ge. 2.
*        We have to disable scaling.
*        mode = 0  tells the functions that gradients are not required.
*        ---------------------------------------------------------------
         if (ierr .eq. 6) go to 800
         lssave = lscale
         lscale = 0
         nstat1 = 2 + ierr
         nstat2 = nstat1
         mode   = 0
         if (nlncon) then
            call m6fcon( mode, nncon, nnjac, njac, z(lfcon), z(lgcon2),
     $                   ne, nka, ha, ka,
     $                   xn, z, nwcore )
         end if
         if (nlnobj) then
            call m6fobj( mode, nnobj, fobj, z(lgobj2), xn, z, nwcore )
         end if
         lscale = lssave
         if (mode .lt. 0) go to 800
         nstat1 = 0
         nstat2 = 0
         
*        ---------------------------------------------------------------
*        Terminate Cycles if m5solv gave a serious error.
*        Otherwise, let the user modify the problem for the next Cycle.
*        ---------------------------------------------------------------
         if (ierr   .ge.    20) go to 800
         if (ncycle .ge. maxcy) go to 800
         if (iprint .gt.     0) write(iprint, 3000) ncycle
         if (isumm  .gt.     0) write(isumm , 3000) ncycle
      
         call matmod( ncycle, nprob, finish,
     $                m, n, nb, ne, nka, ns, nscl, nname,
     $                a, ha, ka, bl, bu,
     $                z(lascal), hs, name1, name2,
     $                xn, pi, rc, z, nwcore )
      
         if (.not. finish) go to 500
*     ==================================================================
*     End of the Cycle loop.
*     ==================================================================


*     Print the final solution if it has not already been printed.

  800 if (nprntd .ne. nsolvd) then
         call m1time( 3,0 )
         call m4savb( 2, m, mbs, n, nb, nn, nname, nscl, msoln, ns,
     $                ne, nka, a, ha, ka,
     $                hs, z(lkb), z(lascal), bl, bu,
     $                name1, name2,
     $                pi, rc, xn, z(ly), z, nwcore )
         call m1time(-3,0 )
      end if
*     ------------------------------------------------------------------
*     Exit.
*     ------------------------------------------------------------------

  900 inform = ierr
      return

 1020 format(/ ' gotbas =', l2, 4x, ' gotfac =', l2, 4x,
     $         ' gothes =', l2, 4x, ' gotscl =', l2)
 1030 format(/ ' XXX Start parameter not recognized:  ', a)
 1100 format(  ' Scale option', i3, ',      Partial price', i8
     $       / ' Partial price section size (A) ', i12
     $       / ' Partial price section size (I) ', i12)
 1110 format(/ ' Scale option', i3, ',    Partial price', i4)
 1200 format(  ' Initial basis' / ' -------------')
 1210 format(/ ' No basis file supplied')
 2000 format(/ ' Start of Cycle', i5,
     $       / ' -------------------')
 2100 format(  ' Iterations' / ' ----------')
 3000 format(/ ' matmod called with ncycle =', i5)

*     end of misolv
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1clos( lun )

*     ------------------------------------------------------------------
*     m1clos  closes the file with logical unit number lun.
*     This version is trivial and so far is not even used by MINOS.
*     Perhaps some implementations will need something fancier.
*     ------------------------------------------------------------------

      close( lun )

*     end of m1clos
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1envt( mode )

*     ------------------------------------------------------------------
*     m1envt specifies the environment within which MINOS is being used.
*
*     When mode = 0, the various logicals should be initialized.
*     page1 says whether new pages are ever wanted on file iprint.
*     page2 says whether new pages are ever wanted on file isumm.
*
*     When mode is in the range 1 to 99, each environment does its
*     own thing.
*
*     When mode = 999, MINOS is asking if resource limits have been
*     reached.   To indicate YES, set ierr = 19.
*
*     The various environments are as follows:
*
*     ALONE:
*     This means MINOS is in stand-alone mode---the normal case.
*     Nothing special is done.
*
*     GAMS:
*     When mode = 1, 2, ..., 9 the characters =1, =2, ..., =9
*     are output to the print file.
*     When mode = 999, the resource limits are tested.
*
*     MINT:
*     Since branch-and-bound means a lot of CYCLES, we suppress
*     page ejects.  Otherwise, nothing special as yet.
*
*     AMPL:
*     Nothing special yet, but might want to test resource limits.
*
*     16 Sep 1987  Initial version.
*     24 Apr 1992  AMPL added.
*     ------------------------------------------------------------------

      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1file/ iread,iprint,isumm
      common    /m5log1/ idebug,ierr,lprint

*     GAMS resource info:
*     ARESLM is limit on cpu time.
*     ATIME0 is starting time.
*     ATMGET is a function returning time so far.
*
*X    REAL               ARESLM, ATIME0, ADELT
*X    COMMON    /GAMS00/ ARESLM, ATIME0, ADELT
*X    REAL               ATMGET
*X    EXTERNAL           ATMGET

      if (mode .le. 0) then
*        ---------------------------------------------------------------
*        mode = 0.    Initialize.
*        page1 and page2 should be false for applications involving
*        many Cycles  (e.g. MINT).
*        ---------------------------------------------------------------
         alone  = .true.
         AMPL   = .false.
         GAMS   = .false.
         MINT   = .false.
         page1  = .true.
         page2  = .false.

      else if (mode .lt. 999) then
*        ---------------------------------------------------------------
*        mode = 1 or more.  Do what has to be done in each environment.
*        ---------------------------------------------------------------
         if (GAMS  .and.  iprint .gt. 0) then
            write(iprint, '(a1, i1)') '=', mode
         end if

      else if (mode .eq. 999) then
*        ---------------------------------------------------------------
*        mode = 999.  Test for excess time, etc.
*        ---------------------------------------------------------------
         if (GAMS) then
*X          IF (ATMGET() - ATIME0 .GE. ARESLM) IERR = 19
         else if (AMPL) then
*X
         end if
      end if

*     end of m1envt
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1init( title )

      implicit           double precision (a-h,o-z)
      character*30       title

*     ------------------------------------------------------------------
*     m1init defines certain machine-dependent constants.
*
*     eps    = floating-point precision (e.g., 2.0**(-47) for CDC)
*     nwordr = no. of  reals          per word of  z(*)
*     nwordi = no. of  integers       per word of  z(*)
*     nwordh = no. of  half integers  per word of  z(*)
*        where  z(*)  is the main array of storage.
*
*     Original version:  integer*2  and  nwordh = 4  used throughout for
*                        certain arrays:
*                        ha, hs  in MINOS (and maybe a few others),
*                        indc, indr, ip, iq  in LUSOL.
*                        A quirk in LUSOL limits MINOS to 16383 rows
*                        when the limit should have been  32767.
*
*                        At present, nwordr is not used because there
*                        are no real*4 arrays.
*
*     22 May 1992:       integer*4  and  nwordh = 2  now used to allow
*                        essentially any number of rows.
*     ------------------------------------------------------------------

      common    /m1eps / eps,eps0,eps1,eps2,eps3,eps4,eps5,plinfy
      common    /m1word/ nwordr,nwordi,nwordh

      title  = 'M I N O S    5.4    (Dec 1992)'
*---------------123456789|123456789|123456789|--------------------------

*---+ IEEE standard: eps = 2**(-52) = 2.22e-16
      eps    = 2.0d+0**(-52)
      nwordr = 2
      nwordi = 2
      nwordh = 2

*     Set other machine-precision constants.

      eps0   = eps**(0.8 d+0)
      eps1   = eps**(0.67d+0)
      eps2   = eps**(0.5 d+0)
      eps3   = eps**(0.33d+0)
      eps4   = eps**(0.25d+0)
      eps5   = eps**(0.2 d+0)
      plinfy = 1.0d+20

*     Set the environment (for later use).

      call m1envt( 0 )

*     end of m1init
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1open( lun, state )

      integer            lun
      character*3        state

*     ------------------------------------------------------------------
*     m1open  is a machine-dependent routine.
*     It opens the file with logical unit number lun.
*
*     state   is intended to be input as 'IN ' or 'OUT'.  It may
*     be helpful on some systems.  MINOS uses sequential files only
*     and does not need to read and write to the same file.
*
*     'IN ' refers to an existing input file that will not be altered.
*     'OUT' means that a new file will be output.  If the file already
*     exists, it might be OK to overwrite it, but on some systems it
*     is better to create a new version of the file.  The choice is
*     open (to coin a phrase).
*
*
*     15 Jul 1989: First version, follows some of the advice offered
*                  by David Gay, AT&T.
*     ------------------------------------------------------------------

      if ( state .eq. 'IN ' ) then

*        Open an input file (e.g. MPS, OLD BASIS).
*        'OLD' means there will be an error if the file does not exist.
*        Since some systems position existing files at the end
*        (rather than the beginning), a rewind is performed.
      
         open  ( unit = lun, status = 'OLD' )
         rewind( lun, err=900 )


      else if ( state .eq. 'OUT' ) then

*        Open an output file (e.g. DUMP, SOLUTION).
*        If it is OK to overwrite an existing file, we could do the
*        same as for input:

*---     open  ( lun )
*---     rewind( lun, err=900 )

*        On DEC VAX/VMS systems it is better to let a new generation
*        be created when the first write occurs, so we do nothing:

      end if

  900 return
      
*     end of m1open
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1page( mode )

*     ------------------------------------------------------------------
*     m1page is an installation-dependent routine.  It is called at
*     points where some users might want output to files iprint or isumm
*     to begin on a new page.
*
*     page1 and page2 have already been set by m1envt.
*     If they are true, a page eject and a blank line are output.
*     Otherwise, just a blank line is output.
*
*     If mode = 1, just the page control is relevant.
*     If mode = 2, GAMS wants m1envt to print an =.
*     If mode = 0  and Summary level = 0, we don't want anything output
*                  to the Summary file.  At present, this is so m8setj
*                  will print just one line per major iteration, with
*                  no blank line in between.
*
*     16-Sep-1987:  First version.
*     20-Mar-1988:  mode 2 added.
*     12-Dec-1991:  mode 0 added.
*     ------------------------------------------------------------------

      logical            alone, AMPL, GAMS, MINT, page1, page2
      common    /m1env / alone, AMPL, GAMS, MINT, page1, page2
      common    /m1file/ iread,iprint,isumm
      logical            prnt0 ,prnt1 ,summ0 ,summ1 ,newhed
      common    /m5log4/ prnt0 ,prnt1 ,summ0 ,summ1 ,newhed

      if (iprint .gt. 0) then
         if ( page1 ) write(iprint, 1001)
                      write(iprint, 1002)
      end if

      if (mode   .eq. 2) call m1envt( 1 )

      if (isumm  .gt. 0) then
         if ( page2 ) write(isumm , 1001)
         if ( summ1  .or.  mode .ne. 0 )
     $                write(isumm , 1002)
      end if
      return

 1001 format('1')
 1002 format(' ')

*     end of m1page
      end

*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1time( clock, prtopt )

      implicit           double precision (a-h,o-z)
      integer            clock, prtopt

*     ------------------------------------------------------------------
*     m1time, m1timp and m1cpu are derived from timer, timout and nowcpu
*     written for DEC VAX/VMS systems by Irvin Lustig,
*     Department of Operations Research, Stanford University, 1987.
*
*     MINOS  calls m1time only.  m1time calls m1cpu  and  m1timp.
*     Only m1cpu is intrinsically machine dependent.
*
*     If a timer is available, call it in m1cpu  and arrange that
*     m1cpu  returns the current CPU time in seconds.
*
*     If a timer is not available or not wanted, set time = 0.0 in m1cpu.
*     Timing will be turned off and m1timp will not be called.
*     ------------------------------------------------------------------
*
*     m1time turns on or off a selected clock and optionally prints
*     statistics regarding all clocks or just the clock chosen.
*
*     The value of abs(clock) is which clock to use.
*     If clock = 0 and prtopt = 0, all clocks and statistics are reset.
*     If clock > 0, the clock is reset to start timing at the
*                   current time (determined by calling the
*                   machine-dependent subroutine m1cpu).
*     If clock < 0, the clock is turned off and the statistic is
*                   recorded for the amount of time since the clock
*                   was turned on.
*
*     prtopt is the print option.
*     If ltime < 0, nothing is printed.  Otherwise,
*     prtopt = 0 indicates print nothing,
*            = 1 indicates print last time for this clock,
*                only if clock < 0 (it has just been turned off),
*            = 2 indicates print total time for all clocks,
*            = 3 indicates print mean  time for all clocks.
*
*     The procedure for adding a new timer n is as follows:
*     1)  Change ntime to n in the parameter statement below (and in
*         all other routines referencing common block /m1tim /).
*     2)  Expand the array "label" to length n in subroutine m1timp.
*
*     04 Jun 1989: Irv's VMS/VAXC version of m1cpu installed,
*                  with changes to return time in seconds.
*     10 Jul 1992: More clocks added for use in AMPL (and elsewhere).
*     ------------------------------------------------------------------
*
*        Clock 1 is for input time.
*        Clock 2 is for solve time.
*        Clock 3 is for output time.
*        Clock 4 is for the nonlinear constraint functions.
*        Clock 5 is for the nonlinear objective.
*
*        numt(i)  is the number of times clock i has been turned on.
*        tlast(i) is the time at which clock i was last turned on.
*        tsum(i)  is the total time elapsed while clock i was on.
*        ltime    is the Timing level set in the Specs file.

      parameter        ( ntime = 5 )
      common    /m1tim / tlast(ntime), tsum(ntime), numt(ntime), ltime

      external           m1cpu
      double precision   stat, time
      integer            iclock, ilo, ihi

      if (ltime .eq. 0) return
      iclock = iabs(clock)

      if (clock .eq. 0) then
         if (prtopt .eq. 0) then

*           clock = 0, prtopt = 0.  Reset everything.

            call m1cpu ( 1, time )
            call m1cpu ( 0, time )
            do 100 i = 1, ntime
               tlast(i) = time
               tsum(i)  = 0.0
               numt(i)  = 0
  100       continue

*           If the m1cpu( 0, time ) gave time = 0.0, we assume that
*           the clock is a dummy.  Turn off future timing.

            if (time .le. 0.0) ltime = 0
         end if

      else
         call m1cpu ( 0, time )
         if (clock .gt. 0) then
            tlast(iclock) = time
         else
            stat         = time - tlast(iclock)
            tsum(iclock) = tsum(iclock) + stat
            numt(iclock) = numt(iclock) + 1
         end if
      end if

*     Now deal with print options.

      if (prtopt .eq. 0  .or.  ltime .lt. 0) then

*        Do nothing.

      else if (prtopt .eq. 1) then

*        Print statistic for last clock if just turned off.
      
         if (clock .lt. 0) then
            call m1timp( iclock, 'Last time', stat )
         end if

      else

*        prtopt >= 2.  Print all statistics if clock = 0,
*        or print statistic for individual clock.
      
         if (clock .eq. 0) then
            call m1cpu ( -1, time )
            ilo   = 1
            ihi   = ntime
         else
            ilo   = iclock
            ihi   = iclock
         end if
      
         do 400 i = ilo, ihi
            stat  = tsum(i)
            if (prtopt .eq. 2) then
               call m1timp( i, 'Time', stat )
            else if (prtopt .eq. 3) then
               istat = numt(i)
               if (istat .gt. 0) stat = stat / istat
               call m1timp( i, 'Mean time', stat )
            end if
  400    continue
      end if

*     end of m1time
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1timp( iclock, lstat, stat )

      integer            iclock
      character*(*)      lstat
      double precision   stat

*     ------------------------------------------------------------------
*     m1timp  prints CPU time for m1time on file iprint and/or isumm.
*     It is not intrinsically machine dependent.
*
*     iclock  selects the correct label.
*     lstat   is a string to print to tell which type of statistic.
*     stat    is the statistic to print out.
*             If it is zero, we figure it was never timed, so no print.
*
*     12 Jul 1992: Array of labels avoids multiple formats.
*     ------------------------------------------------------------------

      common    /m1file/ iread,iprint,isumm

      character*24       label(5)
      data               label
     $                 / 'for MPS input',          
     $                   'for solving problem',    
     $                   'for solution output',    
     $                   'for constraint functions',
     $                   'for objective function' /

      if (iclock .eq. 1) then
         if (iprint .gt. 0) write(iprint, 1000)
         if (isumm  .gt. 0) write(isumm , 1000)
      end if

      if (stat .eq. 0.0) return

      if (iprint .gt. 0) write(iprint, 1000) lstat, label(iclock), stat
      if (isumm  .gt. 0) write(isumm , 1000) lstat, label(iclock), stat
      return

 1000 format( 1x, a, 1x, a, t38, f13.2,' seconds')

*     end of m1timp
      end

*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

      subroutine m1cpu ( mode, time )

      integer            mode
      double precision   time

*     ------------------------------------------------------------------
*     m1cpu is a machine-dependent routine to return time = cpu time
*     in seconds, so that 2 consecutive calls will indicate the
*     time difference of operations between the 2 calls.
*     The parameter 'mode' indicates what function should be done
*     to the timer.  This allows necessary initialization for certain
*     machines.
*     mode =  1  indicates initialization,
*     mode =  0  indicates normal use,
*     mode = -1  indicates stop the timer.
*
*     For the MicroVax II under VMS, we need to call the correct library
*     routine to get the timer statistics.  These statistics are
*     found by using the times() function in the VaX C Runtime library.
*     To use this version of m1cpu, one must create an options file
*     called  vmsc.opt  with the line
*        SYS$LIBRARY:VAXCRTL/SHARE
*     in it.   Then link using the usual command and append   ,vmsc/opt
*     to the end of the line.  The name vmsc can be anything you desire.
*     ------------------------------------------------------------------

*-->  DEC VAX/VMS
*-->  integer            itimad(4)

*-->  Unix (DECstation)
      real               tarray(2)

      if (mode .eq. 1) then
*        ---------------------------------------------------------------
*        Initialize.
*        ---------------------------------------------------------------
         time   = 0.0

      else if (mode .eq. 0) then
*        ---------------------------------------------------------------
*        Normal call.
*        Return current timer value here.
*        ---------------------------------------------------------------
*-->     On VAX/VMS, itimad(1) returns the number of  centiseconds.
*        call times ( itimad )
*        time   = itimad(1)
*        time   = time * 0.01d+0

*-->     On Unix (DECstation MIPS RISC F77), etime returns seconds.
*        time   = etime ( tarray )

*-->     On other machines, to forget about timing, just say
         time   = 0.0

      else if (mode .eq. -1) then
*        ---------------------------------------------------------------
*        Stop the clock.
*        ---------------------------------------------------------------
         time   = 0.0
      end if

*     end of m1cpu
      end
