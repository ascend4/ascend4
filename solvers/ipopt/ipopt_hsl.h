#ifndef ASC_IPOPT_HSL_H
#define ASC_IPOPT_HSL_H

#ifdef __cplusplus
extern "C" {
#endif

/* IPOPT 3.14+: linked HSL routines plus complete, loadable runtime routines.
   A NULL library selects IPOPT's default libhsl shared library. */
unsigned int asc_ipopt_hsl_available(const char *library);

#ifdef __cplusplus
}
#endif
#endif
