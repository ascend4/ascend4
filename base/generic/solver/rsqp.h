/** copyright 1998 Carnegie Mellon University.
 * By Dave Ternet and Ben Allan
 *  Version: $Revision: 1.5 $
 *  Version control file: $RCSfile: rsqp.h,v $
 *  Date last modified: $Date: 1998/04/25 14:05:26 $
 *  Last modified by: $Author: ballan $
 */
#ifndef __RSQP_H_SEEN__
#define __RSQP_H_SEEN__

/** this header expects ascConfig.h */

#if defined(CRAY) || defined(__WIN32__)
#define OPT_ASCEND		OPT_ASCEND
#define SET_OPT_COMMON		SET_OPT_COMMON
#define GET_OPT_COMMON		GET_OPT_COMMON
#define INIT_OPT_COMMON		INIT_OPT_COMMON
#define INIT_OPT_DEFAULTS	SET_OPT_DEFAULTS
#define GET_OPT_OUTPUT		GET_OPT_OUTPUT
#define CHECK_MEM		CHECK_MEM
#endif /** cray, windoze */

#if defined (APOLLO) || defined (_HPUX_SOURCE)
#define OPT_ASCEND		opt_ascend
#define SET_OPT_COMMON		set_opt_common
#define GET_OPT_COMMON		get_opt_common
#define INIT_OPT_COMMON		init_opt_common
#define INIT_OPT_DEFAULTS	set_opt_defaults
#define GET_OPT_OUTPUT		get_opt_output
#define CHECK_MEM		check_mem
#endif /** apollo, hpux9 */

#ifndef OPT_ASCEND
#define OPT_ASCEND		opt_ascend_
#define SET_OPT_COMMON		set_opt_common_
#define GET_OPT_COMMON		get_opt_common_
#define INIT_OPT_COMMON		init_opt_common_
#define INIT_OPT_DEFAULTS	set_opt_defaults_
#define GET_OPT_OUTPUT		get_opt_output_
#define CHECK_MEM		check_mem_
#endif /** everyone else */


/** ALL OPT_ASCEND ints below are expected to be 32 bits (INTEGER*4) */
/** ALL OPT_ASCEND doubles below are expected to be 64 bits (REAL*8) */
extern void OPT_ASCEND (
  int32 *,              /** n */
  int32 *,              /** m */
  int32 *,              /** md */
  real64 *,           /** x */
  real64 *,           /** f */
  real64 *,           /** g */
  real64 *,           /** c */
  real64 *,           /** a */
  int32 *,              /** nz */
  int32 *,              /** nzd */
  int32 *,              /** avar */
  int32 *,              /** acon */
  int32 *,              /** ibnds */
  real64 *,           /** bnds */
  int32 *,              /** inf */
  real64 *,           /** trust */
  int32 *,              /** liw */
  int32 *,              /** iw */
  int32 *,              /** lrw */
  real64 *	      /** rw */
);


/** initialize rsqp internal common blocks to their default values.
 * these are not the control parameters.
 */
extern void INIT_OPT_COMMON(void);

/** have opt set defaults in its common block control parameters. */
extern void INIT_OPT_DEFAULTS(void);

#define ICONTROLSIZE 12
#define RCONTROLSIZE 1
/** get the common block control parameters */
extern void GET_OPT_COMMON(
  real64 *eps,
  int32 *maxit,
  int32 *kprint,
  int32 *idebug,
  int32 *ioption,
  int32 *ichoose,
  int32 *icorr, 
  int32 *i_mult_free,
  int32 *iiexact,
  int32 *ipmethod,
  int32 *ilusolve,
  int32 *iwarm,
  int32 *cnr
);

/** set the common block control parameters */
extern void SET_OPT_COMMON(
  real64 *eps,
  int32 *maxit,
  int32 *kprint,
  int32 *idebug,
  int32 *ioption,
  int32 *ichoose,
  int32 *icorr, 
  int32 *i_mult_free,
  int32 *iiexact,
  int32 *ipmethod,
  int32 *ilusolve,
  int32 *iwarm,
  int32 *cnr
);

/** get common block iteration status information */
extern void GET_OPT_OUTPUT(
  real64 *fout,
  real64 *e1out,
  real64 *e2out,
  real64 *ynout,
  real64 *znout,
  real64 *alfaout,
  int32 *nactout
);

/** get opt to tell us roughly how much space we need to send it. */
extern void CHECK_MEM(
  int32 *inform,
  int32 *iguess,
  int32 *rguess,
  int32 *n,
  int32 *m,
  int32 *nzd,
  int32 *iwsize, /** out */
  int32 *rwsize  /** out */
);

#endif /** __RSQP_H_SEEN__ */
