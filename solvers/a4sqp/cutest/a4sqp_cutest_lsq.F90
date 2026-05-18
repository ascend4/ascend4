! Experimental CUTEst objective-group accessors for A4SQP least-squares runs.
!
! This file deliberately lives in the CUTEst package adapter, not in liba4sqp.
! It reads CUTEst's internal group/element representation after CUTEST_usetup
! or CUTEST_csetup and exposes only residual-style objective groups to C.
!
! Important: this is intentionally a prototype shim over CUTEst internals. It
! is not a public CUTEst API. The routines below reach directly into:
!
!   CUTEST_data_global
!     n        : number of variables in CUTEst's current problem
!     ng       : number of SIF groups
!     KNDOFC   : maps group -> constraint number; zero means objective group
!                in constrained problems. In unconstrained problems the whole
!                group list is the objective, so KNDOFC is not used as a filter.
!     GXEQX    : true for trivial identity group functions
!     GSCALE   : scale multiplying each group contribution in the objective
!     B        : constant offset in each group argument
!     ISTADA   : row pointers into linear coefficients A / ICNA by group
!     A, ICNA  : sparse linear term coefficients and variable indices
!     ISTADG   : row pointers into nonlinear element uses by group
!     IELING   : nonlinear element number for each group-element use
!     ESCALE   : scale for each group-element use
!     ISTAGV   : row pointers into structural variable incidence by group
!     ISVGRP   : variable indices structurally present in each group
!     ITYPEG, ISTGP, GPVALU
!              : group-function metadata passed back to generated GROUP_r
!     ITYPEE, ISTAEV, IELVAR, INTVAR, ISTADH, ISTEP, EPVALU, INTREP
!              : element metadata passed to generated ELFUN_r/RANGE_r
!
!   CUTEST_work_global(1)
!     ICALCF   : work list of element/group indices to evaluate
!     FUVALS   : element values and element derivative workspace
!     FT       : group argument values before applying GROUP_r
!     GVALS    : group value/first derivative/second derivative workspace
!     W_ws     : sparse group-gradient assembly workspace indexed by variable
!     W_el     : element-gradient temporary workspace
!
! The residual exposed to A4SQP is FT(ig), i.e. the group argument before the
! square/L2 group function is applied. A4SQP's LSQ core minimizes
! 0.5 * weight_i * residual_i^2, while CUTEst's square group contributes
! GSCALE(ig) * FT(ig)^2; therefore weights are exported as 2 * GSCALE(ig).
!
! Keep this shim behind cheap adapter-side gates. The C driver checks the CUTEst
! classification first and only calls these routines for `S...` classifications,
! unconstrained problems, and user-enabled LSQ mode. The routines here then fail
! fast on the first non-square objective group or non-positive scale.

#include "cutest_modules.h"
#include "cutest_routines.h"

SUBROUTINE a4sqp_cutest_lsq_dim( n, nres, max_row_nnz, status ) BIND( C )
! C-facing capability probe.
!
! Scans CUTEST_data_global%KNDOFC to find objective groups
! (KNDOFC(ig) == 0). Every objective group must be a non-trivial square/L2
! group with positive GSCALE, otherwise the LSQ view is rejected and the C
! driver falls back to the ordinary CUTEst NLP callbacks.
!
! max_row_nnz is computed from ISTAGV/ISVGRP, the structural variables present
! in each group argument. It is used by the C side as row workspace capacity for
! a4sqp_lsq_solve's per-row Jacobian callback.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ), VALUE :: n
  INTEGER( C_INT ), INTENT( OUT ) :: nres
  INTEGER( C_INT ), INTENT( OUT ) :: max_row_nnz
  INTEGER( C_INT ), INTENT( OUT ) :: status

  INTEGER( KIND = ip_ ) :: ig, count, row_nnz
  INTEGER( KIND = ip_ ) :: a4sqp_cutest_lsq_group_nnz
  LOGICAL :: a4sqp_cutest_lsq_is_objective_group
  LOGICAL :: a4sqp_cutest_lsq_square_group

  nres = 0
  max_row_nnz = 0
  status = 0
  IF ( .NOT. ALLOCATED( CUTEST_work_global ) ) THEN
    status = 1
    RETURN
  END IF
  IF ( n /= CUTEST_data_global%n ) THEN
    status = 4
    RETURN
  END IF

  count = 0
  DO ig = 1, CUTEST_data_global%ng
    IF ( .NOT. a4sqp_cutest_lsq_is_objective_group( ig ) ) CYCLE
    IF ( .NOT. a4sqp_cutest_lsq_square_group( ig ) ) THEN
      status = 21
      RETURN
    END IF
    IF ( CUTEST_data_global%GSCALE( ig ) <= 0.0_rp_ ) THEN
      status = 22
      RETURN
    END IF
    count = count + 1
    row_nnz = a4sqp_cutest_lsq_group_nnz( ig, n )
    max_row_nnz = MAX( max_row_nnz, INT( row_nnz, C_INT ) )
  END DO

  IF ( count <= 0 ) THEN
    status = 23
    RETURN
  END IF
  nres = INT( count, C_INT )
END SUBROUTINE a4sqp_cutest_lsq_dim

SUBROUTINE a4sqp_cutest_lsq_weights( nres, weights, status ) BIND( C )
! C-facing residual-weight accessor.
!
! Repeats the objective-group ordering used by a4sqp_cutest_lsq_dim and exports
! 2 * CUTEST_data_global%GSCALE(ig). The factor of two matches A4SQP's
! objective convention 0.5 * weight * residual^2 to CUTEst's group convention
! GSCALE * FT^2 for L2 groups.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT, C_DOUBLE
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ), VALUE :: nres
  REAL( C_DOUBLE ), INTENT( OUT ) :: weights( nres )
  INTEGER( C_INT ), INTENT( OUT ) :: status

  INTEGER( KIND = ip_ ) :: ig, row
  LOGICAL :: a4sqp_cutest_lsq_is_objective_group

  status = 0
  row = 0
  DO ig = 1, CUTEST_data_global%ng
    IF ( .NOT. a4sqp_cutest_lsq_is_objective_group( ig ) ) CYCLE
    row = row + 1
    IF ( row > nres ) THEN
      status = 4
      RETURN
    END IF
    weights( row ) = REAL( 2.0_rp_ * CUTEST_data_global%GSCALE( ig ), C_DOUBLE )
  END DO
  IF ( row /= nres ) status = 4
END SUBROUTINE a4sqp_cutest_lsq_weights

SUBROUTINE a4sqp_cutest_lsq_residuals( n, x, nres, residuals, status ) BIND( C )
! C-facing residual evaluator.
!
! First calls a4sqp_cutest_lsq_form_ft to populate CUTEST_work_global(1)%FT
! from CUTEST_data_global's group/element representation. It then copies FT for
! objective groups only. It intentionally does not call GROUP_r here: A4SQP
! wants the pre-square residual argument, not the final scalar objective term.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT, C_DOUBLE
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ), VALUE :: n
  REAL( C_DOUBLE ), INTENT( IN ) :: x( n )
  INTEGER( C_INT ), INTENT( IN ), VALUE :: nres
  REAL( C_DOUBLE ), INTENT( OUT ) :: residuals( nres )
  INTEGER( C_INT ), INTENT( OUT ) :: status

  INTEGER( KIND = ip_ ) :: ig, row
  INTEGER( C_INT ) :: a4sqp_cutest_lsq_form_ft
  LOGICAL :: a4sqp_cutest_lsq_is_objective_group

  IF ( a4sqp_cutest_lsq_form_ft( n, x ) /= 0 ) THEN
    status = 3
    RETURN
  END IF

  row = 0
  DO ig = 1, CUTEST_data_global%ng
    IF ( .NOT. a4sqp_cutest_lsq_is_objective_group( ig ) ) CYCLE
    row = row + 1
    IF ( row > nres ) THEN
      status = 4
      RETURN
    END IF
    residuals( row ) = REAL( CUTEST_work_global( 1 )%FT( ig ), C_DOUBLE )
  END DO
  status = 0
  IF ( row /= nres ) status = 4
END SUBROUTINE a4sqp_cutest_lsq_residuals

SUBROUTINE a4sqp_cutest_lsq_jacobian_row( n, x, row_c, capacity_c, columns,    &
    values, nnz, status ) BIND( C )
! C-facing residual-Jacobian row accessor.
!
! Maps A4SQP's zero-based residual row to the corresponding objective group in
! CUTEST_data_global%KNDOFC ordering, then assembles the sparse gradient of the
! group argument FT(ig). Returned column indices are zero-based for the A4SQP C
! side, even though CUTEst's internal arrays use Fortran one-based indices.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT, C_DOUBLE
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ), VALUE :: n
  REAL( C_DOUBLE ), INTENT( IN ) :: x( n )
  INTEGER( C_INT ), INTENT( IN ), VALUE :: row_c
  INTEGER( C_INT ), INTENT( IN ), VALUE :: capacity_c
  INTEGER( C_INT ), INTENT( OUT ) :: columns( capacity_c )
  REAL( C_DOUBLE ), INTENT( OUT ) :: values( capacity_c )
  INTEGER( C_INT ), INTENT( OUT ) :: nnz
  INTEGER( C_INT ), INTENT( OUT ) :: status

  INTEGER( KIND = ip_ ) :: ig, target, found
  LOGICAL :: a4sqp_cutest_lsq_is_objective_group

  nnz = 0
  status = 0
  target = INT( row_c, KIND = ip_ ) + 1
  IF ( target <= 0 ) THEN
    status = 4
    RETURN
  END IF

  found = 0
  DO ig = 1, CUTEST_data_global%ng
    IF ( .NOT. a4sqp_cutest_lsq_is_objective_group( ig ) ) CYCLE
    found = found + 1
    IF ( found == target ) THEN
      CALL a4sqp_cutest_lsq_group_grad( n, x, ig, capacity_c, columns, values, &
                                        nnz, status )
      RETURN
    END IF
  END DO
  status = 4
END SUBROUTINE a4sqp_cutest_lsq_jacobian_row

LOGICAL FUNCTION a4sqp_cutest_lsq_is_objective_group( ig )
! Internal objective-group predicate.
!
! CUTEst's constrained routines use KNDOFC(ig) to map a group to a constraint;
! KNDOFC(ig) == 0 is then an objective group. Unconstrained problems are stored
! as objective-only group lists, and the uofg/ush code processes every group, so
! this shim must do the same when CUTEST_data_global%numcon == 0.
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( KIND = ip_ ), INTENT( IN ) :: ig

  IF ( CUTEST_data_global%numcon == 0 ) THEN
    a4sqp_cutest_lsq_is_objective_group = .TRUE.
  ELSE
    a4sqp_cutest_lsq_is_objective_group = CUTEST_data_global%KNDOFC( ig ) == 0
  END IF
END FUNCTION a4sqp_cutest_lsq_is_objective_group

LOGICAL FUNCTION a4sqp_cutest_lsq_square_group( ig )
! Internal group-type verifier.
!
! CUTEst stores the group type as integer metadata plus generated GROUP_r code;
! the symbolic type name "L2" is not directly exposed here. Rather than depend
! on a numeric ITYPEG convention, this probes the generated group function for a
! representative argument t. A compatible LSQ group must evaluate as:
!
!   GVALS(ig,1) = t^2
!   GVALS(ig,2) = 2t
!   GVALS(ig,3) = 2
!
! The routine temporarily overwrites CUTEST_work_global(1)%FT(ig) and
! ICALCF(1), calls GROUP_r, then restores those work entries before returning.
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( KIND = ip_ ), INTENT( IN ) :: ig
  REAL( KIND = rp_ ) :: old_ft
  INTEGER( KIND = ip_ ) :: old_icalc
  INTEGER( KIND = ip_ ) :: igstat
  REAL( KIND = rp_ ) :: t
  REAL( KIND = rp_ ) :: tol

  EXTERNAL :: GROUP_r

  a4sqp_cutest_lsq_square_group = .FALSE.
  IF ( CUTEST_data_global%GXEQX( ig ) ) RETURN

  old_ft = CUTEST_work_global( 1 )%FT( ig )
  old_icalc = CUTEST_work_global( 1 )%ICALCF( 1 )
  t = 0.37_rp_
  tol = 1.0E-8_rp_

  CUTEST_work_global( 1 )%FT( ig ) = t
  CUTEST_work_global( 1 )%ICALCF( 1 ) = ig
  CALL GROUP_r( CUTEST_work_global( 1 )%GVALS, CUTEST_data_global%ng,          &
                CUTEST_work_global( 1 )%FT, CUTEST_data_global%GPVALU,         &
                1_ip_, CUTEST_data_global%ITYPEG, CUTEST_data_global%ISTGP,    &
                CUTEST_work_global( 1 )%ICALCF, CUTEST_data_global%ltypeg,     &
                CUTEST_data_global%lstgp, CUTEST_data_global%lcalcf,           &
                CUTEST_data_global%lcalcg, CUTEST_data_global%lgpvlu,          &
                .FALSE., igstat )
  IF ( igstat == 0 ) THEN
    CALL GROUP_r( CUTEST_work_global( 1 )%GVALS, CUTEST_data_global%ng,        &
                  CUTEST_work_global( 1 )%FT, CUTEST_data_global%GPVALU,       &
                  1_ip_, CUTEST_data_global%ITYPEG, CUTEST_data_global%ISTGP,  &
                  CUTEST_work_global( 1 )%ICALCF, CUTEST_data_global%ltypeg,   &
                  CUTEST_data_global%lstgp, CUTEST_data_global%lcalcf,         &
                  CUTEST_data_global%lcalcg, CUTEST_data_global%lgpvlu,        &
                  .TRUE., igstat )
  END IF
  IF ( igstat == 0 ) THEN
    a4sqp_cutest_lsq_square_group =                                           &
      ABS( CUTEST_work_global( 1 )%GVALS( ig, 1 ) - t * t ) <= tol .AND.       &
      ABS( CUTEST_work_global( 1 )%GVALS( ig, 2 ) - 2.0_rp_ * t ) <= tol .AND. &
      ABS( CUTEST_work_global( 1 )%GVALS( ig, 3 ) - 2.0_rp_ ) <= tol
  END IF

  CUTEST_work_global( 1 )%FT( ig ) = old_ft
  CUTEST_work_global( 1 )%ICALCF( 1 ) = old_icalc
END FUNCTION a4sqp_cutest_lsq_square_group

INTEGER( KIND = ip_ ) FUNCTION a4sqp_cutest_lsq_group_nnz( ig, n )
! Internal structural row-size helper.
!
! Counts variables in CUTEST_data_global%ISVGRP for group ig, using ISTAGV as
! the row pointer. This is structural incidence, not numerical pruning: a
! zero-valued derivative at the current x still occupies a row entry if CUTEst
! marks that variable structurally present in the group argument.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( KIND = ip_ ), INTENT( IN ) :: ig
  INTEGER( C_INT ), INTENT( IN ) :: n
  INTEGER( KIND = ip_ ) :: i, ll

  a4sqp_cutest_lsq_group_nnz = 0
  DO i = CUTEST_data_global%ISTAGV( ig ), CUTEST_data_global%ISTAGV( ig + 1 ) - 1
    ll = CUTEST_data_global%ISVGRP( i )
    IF ( ll <= n ) a4sqp_cutest_lsq_group_nnz = a4sqp_cutest_lsq_group_nnz + 1
  END DO
END FUNCTION a4sqp_cutest_lsq_group_nnz

INTEGER( C_INT ) FUNCTION a4sqp_cutest_lsq_form_ft( n, x )
! Internal group-argument evaluator.
!
! Mirrors the FT-building section of CUTEst_uofg_threadsafe_r, but stops before
! GROUP_r is applied. The calculation is:
!
!   FT(ig) = -B(ig)
!            + sum_{linear terms k in group ig} A(k) * x(ICNA(k))
!            + sum_{element uses k in group ig} ESCALE(k) * FUVALS(IELING(k))
!
! ELFUN_r is called with derivative flag 1 to fill CUTEST_work_global(1)%FUVALS
! with nonlinear element values needed by the group arguments.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT, C_DOUBLE
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ) :: n
  REAL( C_DOUBLE ), INTENT( IN ) :: x( n )

  INTEGER( KIND = ip_ ) :: i, j, ig, ifstat
  REAL( KIND = rp_ ) :: ftt

  EXTERNAL :: ELFUN_r

  DO i = 1, MAX( CUTEST_data_global%nel, CUTEST_data_global%ng )
    CUTEST_work_global( 1 )%ICALCF( i ) = i
  END DO

  CALL ELFUN_r( CUTEST_work_global( 1 )%FUVALS, x, CUTEST_data_global%EPVALU,  &
                CUTEST_data_global%nel, CUTEST_data_global%ITYPEE,             &
                CUTEST_data_global%ISTAEV, CUTEST_data_global%IELVAR,          &
                CUTEST_data_global%INTVAR, CUTEST_data_global%ISTADH,          &
                CUTEST_data_global%ISTEP, CUTEST_work_global( 1 )%ICALCF,      &
                CUTEST_data_global%ltypee, CUTEST_data_global%lstaev,          &
                CUTEST_data_global%lelvar, CUTEST_data_global%lntvar,          &
                CUTEST_data_global%lstadh, CUTEST_data_global%lstep,           &
                CUTEST_data_global%lcalcf, CUTEST_data_global%lfuval,          &
                CUTEST_data_global%lvscal, CUTEST_data_global%lepvlu,          &
                1, ifstat )
  IF ( ifstat /= 0 ) THEN
    a4sqp_cutest_lsq_form_ft = 3
    RETURN
  END IF

  DO ig = 1, CUTEST_data_global%ng
    ftt = - CUTEST_data_global%B( ig )
    DO i = CUTEST_data_global%ISTADA( ig ), CUTEST_data_global%ISTADA( ig + 1 ) - 1
      j = CUTEST_data_global%ICNA( i )
      IF ( j <= n ) ftt = ftt + CUTEST_data_global%A( i ) * x( j )
    END DO
    DO i = CUTEST_data_global%ISTADG( ig ), CUTEST_data_global%ISTADG( ig + 1 ) - 1
      ftt = ftt + CUTEST_data_global%ESCALE( i ) *                            &
            CUTEST_work_global( 1 )%FUVALS( CUTEST_data_global%IELING( i ) )
    END DO
    CUTEST_work_global( 1 )%FT( ig ) = ftt
  END DO

  a4sqp_cutest_lsq_form_ft = 0
END FUNCTION a4sqp_cutest_lsq_form_ft

SUBROUTINE a4sqp_cutest_lsq_group_grad( n, x, ig, capacity_c, columns, values, &
    nnz, status )
! Internal group-argument gradient assembler.
!
! Mirrors the per-group gradient assembly in CUTEst_ccfsg_threadsafe_r, but
! deliberately omits group-function derivative and GSCALE factors. For LSQ we
! need grad(FT(ig)), not grad(GSCALE * GROUP(FT(ig))).
!
! The routine reaches into:
!   - ELFUN_r with derivative flag 2 to fill FUVALS with element derivatives;
!   - RANGE_r when an element uses CUTEst's internal representation;
!   - ISTADG/IELING/ESCALE for nonlinear element contributions;
!   - ISTADA/A/ICNA for linear terms;
!   - ISTAGV/ISVGRP for sparse row structure.
!
! CUTEST_work_global(1)%W_ws is used as a variable-indexed accumulation buffer.
! Returned columns are converted from CUTEst's one-based indices to zero-based
! C/A4SQP indices.
  USE, INTRINSIC :: ISO_C_BINDING, ONLY : C_INT, C_DOUBLE
  USE CUTEST_KINDS_precision
  USE CUTEST_precision
  IMPLICIT NONE

  INTEGER( C_INT ), INTENT( IN ) :: n
  REAL( C_DOUBLE ), INTENT( IN ) :: x( n )
  INTEGER( KIND = ip_ ), INTENT( IN ) :: ig
  INTEGER( C_INT ), INTENT( IN ) :: capacity_c
  INTEGER( C_INT ), INTENT( OUT ) :: columns( capacity_c )
  REAL( C_DOUBLE ), INTENT( OUT ) :: values( capacity_c )
  INTEGER( C_INT ), INTENT( OUT ) :: nnz
  INTEGER( C_INT ), INTENT( OUT ) :: status

  INTEGER( KIND = ip_ ) :: i, ii, iel, ifstat, ig1, istrgv, iendgv
  INTEGER( KIND = ip_ ) :: k, l, ll, nvarel, nelow, nelup, nin, j
  REAL( KIND = rp_ ) :: scalee

  EXTERNAL :: ELFUN_r
  EXTERNAL :: RANGE_r

  nnz = 0
  status = 0

  DO i = 1, MAX( CUTEST_data_global%nel, CUTEST_data_global%ng )
    CUTEST_work_global( 1 )%ICALCF( i ) = i
  END DO

  CALL ELFUN_r( CUTEST_work_global( 1 )%FUVALS, x, CUTEST_data_global%EPVALU,  &
                CUTEST_data_global%nel, CUTEST_data_global%ITYPEE,             &
                CUTEST_data_global%ISTAEV, CUTEST_data_global%IELVAR,          &
                CUTEST_data_global%INTVAR, CUTEST_data_global%ISTADH,          &
                CUTEST_data_global%ISTEP, CUTEST_work_global( 1 )%ICALCF,      &
                CUTEST_data_global%ltypee, CUTEST_data_global%lstaev,          &
                CUTEST_data_global%lelvar, CUTEST_data_global%lntvar,          &
                CUTEST_data_global%lstadh, CUTEST_data_global%lstep,           &
                CUTEST_data_global%lcalcf, CUTEST_data_global%lfuval,          &
                CUTEST_data_global%lvscal, CUTEST_data_global%lepvlu,          &
                2, ifstat )
  IF ( ifstat /= 0 ) THEN
    status = 3
    RETURN
  END IF

  ig1 = ig + 1
  istrgv = CUTEST_data_global%ISTAGV( ig )
  iendgv = CUTEST_data_global%ISTAGV( ig1 ) - 1
  nelow = CUTEST_data_global%ISTADG( ig )
  nelup = CUTEST_data_global%ISTADG( ig1 ) - 1

  CUTEST_work_global( 1 )%W_ws( CUTEST_data_global%ISVGRP( istrgv : iendgv ) ) = 0.0_rp_

  DO ii = nelow, nelup
    iel = CUTEST_data_global%IELING( ii )
    k = CUTEST_data_global%INTVAR( iel )
    l = CUTEST_data_global%ISTAEV( iel )
    nvarel = CUTEST_data_global%ISTAEV( iel + 1 ) - l
    scalee = CUTEST_data_global%ESCALE( ii )
    IF ( CUTEST_data_global%INTREP( iel ) ) THEN
      nin = CUTEST_data_global%INTVAR( iel + 1 ) - k
      CALL RANGE_r( iel, .TRUE., CUTEST_work_global( 1 )%FUVALS( k ),          &
                    CUTEST_work_global( 1 )%W_el, nvarel, nin,                 &
                    CUTEST_data_global%ITYPEE( iel ), nin, nvarel )
      DO i = 1, nvarel
        j = CUTEST_data_global%IELVAR( l )
        CUTEST_work_global( 1 )%W_ws( j ) = CUTEST_work_global( 1 )%W_ws( j ) + &
                                           scalee * CUTEST_work_global( 1 )%W_el( i )
        l = l + 1
      END DO
    ELSE
      DO i = 1, nvarel
        j = CUTEST_data_global%IELVAR( l )
        CUTEST_work_global( 1 )%W_ws( j ) = CUTEST_work_global( 1 )%W_ws( j ) + &
                                           scalee * CUTEST_work_global( 1 )%FUVALS( k )
        k = k + 1
        l = l + 1
      END DO
    END IF
  END DO

  DO k = CUTEST_data_global%ISTADA( ig ), CUTEST_data_global%ISTADA( ig1 ) - 1
    j = CUTEST_data_global%ICNA( k )
    CUTEST_work_global( 1 )%W_ws( j ) = CUTEST_work_global( 1 )%W_ws( j ) +    &
                                       CUTEST_data_global%A( k )
  END DO

  DO i = istrgv, iendgv
    ll = CUTEST_data_global%ISVGRP( i )
    IF ( ll <= n ) THEN
      nnz = nnz + 1
      IF ( nnz > capacity_c ) THEN
        status = 4
        RETURN
      END IF
      columns( nnz ) = INT( ll - 1, C_INT )
      values( nnz ) = REAL( CUTEST_work_global( 1 )%W_ws( ll ), C_DOUBLE )
    END IF
  END DO
END SUBROUTINE a4sqp_cutest_lsq_group_grad
