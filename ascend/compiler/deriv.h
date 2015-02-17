#include "instance_enum.h"
#include "name.h"

extern
void SetDerInfo(struct Instance *deriv, struct Instance *state, struct Instance *indep);
/**<
  *  Creates pointers between the instances. All the instances should be of
  *  type REAL_ATOM_INST.
  */

ASC_DLLSPEC
struct Instance *FindDerByArgs(struct Instance *state, struct Instance *indep);
/**<
 *  Finds a derivative given the state and independent variables. All the instances
 *  should be of type REAL_ATOM_INST.
 */

ASC_DLLSPEC
struct Instance *FindIndepByDer(struct Instance *deriv);
/**<
 *  Finds an independent variable given a derivative. The parameter instance should
 *  be of type REAL_ATOM_INST.
 */

extern
void ModifyIderivPointers(struct Instance *deriv,
                          struct gl_list_t *indlist,
                          CONST struct Instance *old,
                          CONST struct Instance *new);
/**<
 *  Change the independent variable in a derivative from old to new.
 */

extern
void ModifyStatePointers(struct Instance *state,
                         struct gl_list_t *derlist,
                         CONST struct Instance *old,
                         CONST struct Instance *new);
/**<
 *  Change the derivative in the list contained by the state variable from old to new.
 */

extern
void ModifyIndepPointers(struct Instance *deriv,
                          struct gl_list_t *indlist,
                          CONST struct Instance *old,
                          CONST struct Instance *new);
/**<
 *  Change the derivative in the list contained by the independent variable from old to new.
 */

extern
void ModifySderivPointers(struct Instance *deriv,
                          struct gl_list_t *stlist,
                          CONST struct Instance *old,
                          CONST struct Instance *new);
/**<
 *  Change the state variable in a derivative from old to new.
 */

ASC_DLLSPEC
void WriteDerInfo(FILE *f, struct Instance *inst);
/**<
 *  Output the lists of derivative, independent and state variable
 *  instances connected with inst.
 */

ASC_DLLSPEC
int IsDeriv(struct Instance *inst);
/**<
 *  If the instance in a derivative, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
int IsState(struct Instance *inst);
/**<
 *  If the instance in a state variable, return 1. Otherwise return 0.
 */

ASC_DLLSPEC
int IsIndep(struct Instance *inst);
/**<
 *  If the instance in an independent variable, return 1. Otherwise return 0.
 */

