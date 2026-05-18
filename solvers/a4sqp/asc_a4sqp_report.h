/*
 * ASCEND-facing A4SQP reporting helpers.
 */

#ifndef ASC_A4SQP_REPORT_H
#define ASC_A4SQP_REPORT_H

struct A4SqpSystem;

void asc_a4sqp_report_view(struct A4SqpSystem *sys);
void asc_a4sqp_report_qp(struct A4SqpSystem *sys);
void asc_a4sqp_report_iteration(struct A4SqpSystem *sys);

#endif
