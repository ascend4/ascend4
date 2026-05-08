/*
 * A4SQP slv_system_t adapter.
 */

#ifndef ASC_A4SQP_ASCEND_H
#define ASC_A4SQP_ASCEND_H

#include <ascend/system/slv_client.h>

struct A4SqpSystem;

int a4sqp_ascend_build_view(struct A4SqpSystem *sys, slv_system_t server);

#endif
