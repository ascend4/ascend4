/**
	Implementation functions for IDA. Only put things here if they need to 
	be shared between .c files but not visible outside IDA.
*/	
#ifndef ASC_IDA_IMPL_H
#define ASC_IDA_IMPL_H

mtx_matrix_t integrator_ida_dgdya(const IntegratorSystem *sys);

#endif

