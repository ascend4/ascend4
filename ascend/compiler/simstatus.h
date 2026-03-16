#ifndef ASCEND_COMPILER_SIMSTATUS_H
#define ASCEND_COMPILER_SIMSTATUS_H

#include <ascend/general/platform.h>

#ifdef __cplusplus
extern "C"{
#endif

struct Instance;
struct asc_simstatus;

ASC_DLLSPEC void asc_simstatus_method_enter(struct Instance *inst);
ASC_DLLSPEC void asc_simstatus_method_leave(struct Instance *inst);
ASC_DLLSPEC void asc_simstatus_mark_dirty(struct Instance *inst);
ASC_DLLSPEC void asc_simstatus_mark_clean(struct Instance *inst, struct Instance *target);
ASC_DLLSPEC int asc_simstatus_is_dirty(struct Instance *inst);
ASC_DLLSPEC unsigned asc_simstatus_method_depth(struct Instance *inst);
ASC_DLLSPEC struct Instance *asc_simstatus_get_last_solve_target(struct Instance *inst);
ASC_DLLSPEC void asc_simstatus_destroy(struct Instance *siminst);

#ifdef __cplusplus
}
#endif

#endif
