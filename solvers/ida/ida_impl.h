/**
	Implementation functions for IDA. Only put things here if they need to 
	be shared between .c files but not visible outside IDA.
*/	
#ifndef ASC_IDA_IMPL_H
#define ASC_IDA_IMPL_H

/* forward dec needed for IntegratorIdaPrecFreeFn */
struct IntegratorIdaDataStruct;

/* functions for allocating storage for and freeing preconditioner data */
typedef void IntegratorIdaPrecCreateFn(IntegratorSystem *integ);
typedef void IntegratorIdaPrecFreeFn(struct IntegratorIdaDataStruct *enginedata);

/**
	Struct containing any stuff that IDA needs that doesn't fit into the
	common IntegratorSystem struct.
*/
typedef struct IntegratorIdaDataStruct{

	struct rel_relation **rellist;   /**< NULL terminated list of ACTIVE rels */
	int nrels; /* number of ACTIVE rels */

	struct bnd_boundary **bndlist;	 /**< NULL-terminated list of boundaries, for use in the root-finding  code */
	int nbnds; /* number of boundaries */

	int safeeval;                    /**< whether to pass the 'safe' flag to relman_eval */
	var_filter_t vfilter;
	rel_filter_t rfilter;            /**< Used to filter relations from solver's rellist (@TODO needs work) */
	void *precdata;                  /**< For use by the preconditioner */
	IntegratorIdaPrecFreeFn *pfree;	 /**< Store instructions here on how to free precdata */

} IntegratorIdaData;


mtx_matrix_t integrator_ida_dgdya(const IntegratorSystem *sys);

int integrator_ida_debug(const IntegratorSystem *sys, FILE *fp);

IntegratorIdaData *integrator_ida_enginedata(IntegratorSystem *integ);

void integrator_ida_write_incidence(IntegratorSystem *integ);

#endif

