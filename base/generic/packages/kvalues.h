#ifndef kvalues_h_seen
#define kvalues_h_seen
extern int KValues_Init (void);
extern int kvalues_preslv(struct Slv_Interp *slv_interp,
		   struct Instance *data,
		   struct gl_list_t *arglist);

extern int kvalues_fex(struct Slv_Interp *slv_interp,
		int ninputs, int noutputs,
		double *inputs, double *outputs,
		double *jacobian);
#endif /*kvalues_h_seen */
