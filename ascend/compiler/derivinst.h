#ifndef ASC_DERIVINST_H
#define ASC_DERIVINST_H

#include <ascend/general/platform.h>
#include "symtab.h"

struct Instance;

/* public instance-centric API */
ASC_DLLSPEC int InstanceHasDerivative(struct Instance *base);
ASC_DLLSPEC struct Instance *InstanceGetDerivative(struct Instance *base);
ASC_DLLSPEC struct Instance *InstanceEnsureDerivative(struct Instance *base);
ASC_DLLSPEC int IsDerivativeInstance(CONST struct Instance *inst);
ASC_DLLSPEC struct Instance *DerivativeInstanceBase(CONST struct Instance *inst);
ASC_DLLSPEC unsigned long DerivativeInstanceOrder(CONST struct Instance *inst);
ASC_DLLSPEC struct Instance *DerivativeInstanceIndependent(CONST struct Instance *inst);
ASC_DLLSPEC int DerivativeInstanceUsesAlgebraicDefault(CONST struct Instance *inst);
ASC_DLLSPEC void DerivativeInstanceClearAlgebraicDefault(struct Instance *inst);
ASC_DLLSPEC void DerivativeInstanceNoteMutation(struct Instance *inst);

/* browser / interactive dynamic-child API */
ASC_DLLSPEC unsigned long InstanceDynamicChildCount(struct Instance *inst);
ASC_DLLSPEC struct Instance *InstanceDynamicChild(struct Instance *inst, unsigned long n);
ASC_DLLSPEC symchar *InstanceDynamicChildName(struct Instance *inst, unsigned long n);
ASC_DLLSPEC struct Instance *InstanceDynamicChildByChar(struct Instance *inst, symchar *name);
ASC_DLLSPEC unsigned long InstanceDynamicChildIndex(struct Instance *inst, struct Instance *child);

/* derivative runtime management API */
ASC_DLLSPEC int DerivativeInstancesMarkPresent(struct Instance *root, struct Instance *base);
ASC_DLLSPEC void DerivativeInstancesPrepareRoot(struct Instance *root);
ASC_DLLSPEC void DerivativeInstanceMarkSolverOwned(struct Instance *inst);
ASC_DLLSPEC void DerivativeInstanceDetach(struct Instance *inst);
ASC_DLLSPEC void DerivativeInstancesClearRoot(struct Instance *root);
ASC_DLLSPEC void DerivativeInstancesClearAll(void);

#endif
