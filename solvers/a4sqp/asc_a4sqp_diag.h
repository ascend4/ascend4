/*
 * A4SQP diagnostics.
 */

#ifndef ASC_A4SQP_DIAG_H
#define ASC_A4SQP_DIAG_H

#include <ascend/system/slv_client.h>

void a4sqp_report_progress(slv_parameters_t *params, const char *message);

#endif
