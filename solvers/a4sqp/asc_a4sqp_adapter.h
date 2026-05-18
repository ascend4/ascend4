/*
 * A4SQP slv_system_t adapter.
 */

#ifndef ASC_A4SQP_ASCEND_H
#define ASC_A4SQP_ASCEND_H

#include <ascend/system/slv_client.h>

struct A4SqpSystem;
struct A4SqpView;

int asc_a4sqp_build_view(struct A4SqpSystem *sys, slv_system_t server);
int a4sqp_view_build(struct A4SqpView *view, slv_system_t server, int safe, const char *scaleopt);

#endif
