/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.
*/
/** @file
	Mixed structural block decomposition support.

	This API builds a conservative mixed incidence graph using the same
	matrix/block machinery as QRSlv's real-valued block decomposition. It is
	intended for conditional solver analysis before CMSlv starts consuming it
	directly.
*/

#ifndef ASC_DECOMP_H
#define ASC_DECOMP_H

#include <ascend/general/platform.h>
#include <ascend/system/slv_types.h>
#include <ascend/linear/mtx.h>

typedef enum slv_decomp_row_kind {
	slv_decomp_row_rel,
	slv_decomp_row_condrel,
	slv_decomp_row_logrel,
	slv_decomp_row_condlogrel,
	slv_decomp_row_invalid
} slv_decomp_row_kind_t;

typedef enum slv_decomp_col_kind {
	slv_decomp_col_var,
	slv_decomp_col_dvar,
	slv_decomp_col_invalid
} slv_decomp_col_kind_t;

typedef struct slv_decomp_partition {
	int32 nblocks;
	mtx_region_t *blocks;
	int32 rank;
	int32 n_rows;
	int32 n_cols;
	int32 n_rels;
	int32 n_condrels;
	int32 n_logrels;
	int32 n_condlogrels;
	int32 n_vars;
	int32 n_dvars;
	int32 nnz;
	int32 *nz_rows;
	int32 *nz_cols;
	int32 *row_org;
	int32 *col_org;
	int32 *row_cur;
	int32 *col_cur;
} slv_decomp_partition_t;

ASC_DLLSPEC void slv_decomp_init(slv_decomp_partition_t *decomp);
ASC_DLLSPEC void slv_decomp_destroy(slv_decomp_partition_t *decomp);

ASC_DLLSPEC int slv_decomp_partition(slv_system_t sys,
		slv_decomp_partition_t *decomp);
/**<
	Builds a mixed real/logical/conditional BLT partition.

	Rows are solver relations, conditional relations, logical relations, then
	conditional logical relations. Columns are solver variables followed by
	solver discrete variables. Boundary dependencies from SATISFIED terms and
	WHEN selector dependencies are added conservatively.

	@return 0 on success, 2 on out-of-memory, 1 on other failure.
*/

ASC_DLLSPEC int slv_decomp_partition_active(slv_system_t sys,
		slv_decomp_partition_t *decomp);
/**<
	Builds a mixed BLT partition for the current active conditional branch.

	This partition uses only rows whose `included` and `active` flags are both
	true. It does not add WHEN selector-to-row activation edges, because those
	edges have already been consumed when the active CASE rows were selected.
	It is intended as the re-analysis view used after selectors or boundaries
	have resolved part of the conditional structure.

	@return 0 on success, 2 on out-of-memory, 1 on other failure.
*/

ASC_DLLSPEC slv_decomp_row_kind_t slv_decomp_row_kind(
		const slv_decomp_partition_t *decomp, int32 orgrow, int32 *local);
ASC_DLLSPEC slv_decomp_col_kind_t slv_decomp_col_kind(
		const slv_decomp_partition_t *decomp, int32 orgcol, int32 *local);

#endif
