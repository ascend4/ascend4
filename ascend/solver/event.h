/*	ASCEND modelling environment
	Copyright (C) 2026

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2, or (at your option)
	any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*//**
	@file
	Draft public API sketch for structured solver/integrator event reporting.

	This is intentionally a first-pass design scaffold rather than a wired
	implementation. The emphasis here is on:

	- what event types exist
	- what minimal identifying data accompanies them
	- where event handlers attach

	The first-pass design deliberately avoids:

	- event severities
	- domain/mask filtering in the event record itself
	- state snapshots
	- old/new value payload capture

	Observed trajectories and other state logging are expected to continue to
	use existing observation/reporting mechanisms.
*/

#ifndef ASC_EVENT_H
#define ASC_EVENT_H

#include <ascend/general/platform.h>
#include <ascend/system/slv_types.h>

/**	@addtogroup solver_api
	@{
*/

struct IntegratorSystemStruct;

/**
	High-level event type.

	The first pass keeps this taxonomy intentionally flat and readable. More
	rich grouping can be layered on later if needed.
*/
typedef enum asc_event_type_enum {
	ASC_EVENT_NONE = 0,

	/* Configuration / lifecycle */
	ASC_EVENT_SOLVER_SELECTED,
	ASC_EVENT_INTEGRATOR_SELECTED,
	ASC_EVENT_OPTION_CHANGED,
	ASC_EVENT_SYSTEM_REANALYSED,

	/* Integrator progress/state */
	ASC_EVENT_INTEGRATION_STARTED,
	ASC_EVENT_INTEGRATION_COMPLETED,
	ASC_EVENT_STEP_ACCEPTED,
	ASC_EVENT_STEP_REJECTED,
	ASC_EVENT_ROOT_FOUND,
	ASC_EVENT_CONSISTENCY_SOLVE_STARTED,
	ASC_EVENT_CONSISTENCY_SOLVE_COMPLETED,
	ASC_EVENT_EVENT_ITERATION_STARTED,
	ASC_EVENT_EVENT_ITERATION_COMPLETED,
	ASC_EVENT_RESTARTED,
	ASC_EVENT_MICROSTATE,
	ASC_EVENT_INTEGRATION_FAILED,

	/* Hybrid/discrete changes */
	ASC_EVENT_GUARD_TRIGGERED,
	ASC_EVENT_WHEN_CASE_CHANGED,
	ASC_EVENT_DISCRETE_CHANGED,
	ASC_EVENT_REINIT_APPLIED,

	/* Generic solver diagnostics */
	ASC_EVENT_NONCONVERGENCE,
	ASC_EVENT_RESIDUAL_EVAL_FAILED,
	ASC_EVENT_LINEAR_SOLVE_FAILED,
	ASC_EVENT_BOUND_VIOLATION,
	ASC_EVENT_INCONSISTENT_SYSTEM,
	ASC_EVENT_ITERATION_LIMIT_EXCEEDED,
	ASC_EVENT_TIME_LIMIT_EXCEEDED,
	ASC_EVENT_ABORTED,

	/* Optimisation / MIP progress */
	ASC_EVENT_PROGRESS,
	ASC_EVENT_INCUMBENT_UPDATED,
	ASC_EVENT_BEST_BOUND_UPDATED,
	ASC_EVENT_GAP_MILESTONE,
	ASC_EVENT_NODE_MILESTONE,
	ASC_EVENT_OBJECTIVE_IMPROVED,
	ASC_EVENT_OPTIMISATION_TERMINATED
} asc_event_type_t;

/**
	Kind of runtime object reference attached to an event.
*/
typedef enum asc_event_ref_kind_enum {
	ASC_EVENT_REF_NONE = 0,
	ASC_EVENT_REF_VARIABLE,
	ASC_EVENT_REF_RELATION,
	ASC_EVENT_REF_WHEN,
	ASC_EVENT_REF_GUARD,
	ASC_EVENT_REF_BOUNDARY,
	ASC_EVENT_REF_STATEMENT,
	ASC_EVENT_REF_BLOCK,
	ASC_EVENT_REF_SOLVER,
	ASC_EVENT_REF_INTEGRATOR
} asc_event_ref_kind_t;

/**
	Reference to a model/runtime object.

	`ptr` is intended for in-process use only. `name` is a best-effort resolved
	name/path suitable for textual review or file output.
*/
typedef struct asc_event_ref_structure {
	asc_event_ref_kind_t kind;
	const void *ptr;
	long index;
	const char *name;
} asc_event_ref_t;

/**
	Minimal common event record.

	The event carries:

	- a type
	- optional time/iteration metadata
	- a short human-readable message
	- up to two object references
	- a few generic integer fields for simple identifiers such as root index,
	  case index, or direction

	This is intentionally lightweight. Rich state capture is deferred.
*/
typedef struct asc_event_structure {
	unsigned long long sequence_no;
	asc_event_type_t type;
	const char *engine_name;

	int have_sim_time;
	double sim_time;

	long iteration;
	long subiteration;

	const char *message;

	asc_event_ref_t subject;
	asc_event_ref_t related;

	long index0;
	long index1;
	int flag0;
	int flag1;
} asc_event_t;

/**
	Event handler callback.

	The initial intent is diagnostic/reporting use. Implementations should not
	let handler errors abort solver/integrator execution by default.
*/
typedef int AscEventHandlerFn(const asc_event_t *event, void *user_data);

/**
	Simple preferences for event handling.

	If `enabled` is false, no events are emitted.
	If `log_path` is NULL and default logging is enabled, the implementation
	should create a temporary file under `/tmp` using a safe creation API.
*/
typedef struct asc_event_prefs_structure {
	int enabled;
	const char *log_path;
} asc_event_prefs_t;

/**
	Attach an event handler to a solver instance.

	Not yet implemented. Intended use is for explicit frontend attachment of a
	callback-based sink.
*/
ASC_DLLSPEC int slv_set_event_handler(slv_system_t sys, AscEventHandlerFn *handler, void *user_data);

/**
	Attach an event handler to an integrator instance.

	Not yet implemented. Intended use is for explicit frontend attachment of a
	callback-based sink.
*/
ASC_DLLSPEC int integrator_set_event_handler(struct IntegratorSystemStruct *sys, AscEventHandlerFn *handler, void *user_data);

/**
	Enable or configure default textual event logging for a solver instance.

	Not yet implemented. Intended default behavior is:

	- if logging is enabled and `prefs->log_path == NULL`, create a temp file
	  under `/tmp`
	- write all emitted events as plain text suitable for modeller review
*/
ASC_DLLSPEC int slv_set_event_prefs(slv_system_t sys, const asc_event_prefs_t *prefs);

/**
	Enable or configure default textual event logging for an integrator
	instance.

	Not yet implemented. See `slv_set_event_prefs`.
*/
ASC_DLLSPEC int integrator_set_event_prefs(struct IntegratorSystemStruct *sys, const asc_event_prefs_t *prefs);

#if 0
/*
	Future extensions intentionally deferred from the first pass:

	- event severity metadata
	- domain/mask filtering on the event record
	- state snapshot attachment
	- old/new scalar payload values
	- binary sink formats
*/
#endif

/** @} */

#endif /* ASC_EVENT_H */
