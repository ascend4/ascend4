#ifndef ASC_DERIV_PENDING_H
#define ASC_DERIV_PENDING_H

#include "value_type.h"

struct Instance;

enum deriv_pending_kind {
  deriv_pending_fix,
  deriv_pending_free,
  deriv_pending_assign
};

ASC_DLLSPEC int deriv_pending_record_fixfree(
  struct Instance *root,
  struct Instance *base,
  int fixed
);

ASC_DLLSPEC int deriv_pending_record_assign(
  struct Instance *root,
  struct Instance *base,
  struct value_t value
);

typedef int (*deriv_pending_apply_fn)(
  struct Instance *base,
  enum deriv_pending_kind kind,
  CONST struct value_t *value,
  void *userdata
);

ASC_DLLSPEC int deriv_pending_apply(
  struct Instance *root,
  deriv_pending_apply_fn applyfn,
  void *userdata
);

ASC_DLLSPEC void deriv_pending_clear_root(struct Instance *root);
ASC_DLLSPEC void deriv_pending_clear_all(void);

#endif
