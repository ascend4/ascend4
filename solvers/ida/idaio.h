typedef struct IntegratorIdaStatsStruct{
	long nsteps;
	long nrevals;
	long nlinsetups;
	long netfails;
	int qlast, qcur;
	realtype hinused, hlast, hcur;
	realtype tcur;
} IntegratorIdaStats;

/* error handler forward declaration */
void integrator_ida_error(int error_code
		, const char *module, const char *function
		, char *msg, void *eh_data
);


void integrator_ida_write_stats(IntegratorIdaStats *stats);

IntegratorDebugFn integrator_ida_debug;
IntegratorWriteMatrixFn integrator_ida_write_matrix;

