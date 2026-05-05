#ifndef FPROPS_EQM_LINALG_H
#define FPROPS_EQM_LINALG_H

#if defined(__GNUC__)
# define EQM_LINALG_HIDDEN __attribute__((visibility("hidden")))
#else
# define EQM_LINALG_HIDDEN
#endif

EQM_LINALG_HIDDEN int eqm_linalg_dense_solve(double *A, double *b, int n);

#endif /* FPROPS_EQM_LINALG_H */
