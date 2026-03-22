#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/utilities/error.h>
#include <string.h>

#include "deriv_pending.h"

struct deriv_pending_entry {
  struct Instance *root;
  struct Instance *base;
  int has_fixed;
  int fixed_value;
  int has_assign;
  struct value_t assign_value;
};

static struct gl_list_t *g_deriv_pending = NULL;

static struct deriv_pending_entry *deriv_pending_find(
  struct Instance *root,
  struct Instance *base
){
  unsigned long i, len;
  if(g_deriv_pending == NULL || root == NULL || base == NULL){
    return NULL;
  }
  len = gl_length(g_deriv_pending);
  for(i = 1; i <= len; ++i){
    struct deriv_pending_entry *entry =
      (struct deriv_pending_entry *)gl_fetch(g_deriv_pending, i);
    if(entry != NULL && entry->root == root && entry->base == base){
      return entry;
    }
  }
  return NULL;
}

static struct deriv_pending_entry *deriv_pending_get_or_create(
  struct Instance *root,
  struct Instance *base
){
  struct deriv_pending_entry *entry;
  if(root == NULL || base == NULL){
    return NULL;
  }
  entry = deriv_pending_find(root, base);
  if(entry != NULL){
    return entry;
  }
  if(g_deriv_pending == NULL){
    g_deriv_pending = gl_create(8);
    if(g_deriv_pending == NULL){
      ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory for pending derivative operations.");
      return NULL;
    }
  }
  entry = ASC_NEW(struct deriv_pending_entry);
  if(entry == NULL){
    ERROR_REPORTER_HERE(ASC_PROG_ERR,"Insufficient memory for pending derivative operation.");
    return NULL;
  }
  memset(entry, 0, sizeof(*entry));
  entry->root = root;
  entry->base = base;
  gl_append_ptr(g_deriv_pending, entry);
  return entry;
}

int deriv_pending_record_fixfree(
  struct Instance *root,
  struct Instance *base,
  int fixed
){
  struct deriv_pending_entry *entry = deriv_pending_get_or_create(root, base);
  if(entry == NULL){
    return 1;
  }
  entry->has_fixed = 1;
  entry->fixed_value = fixed ? 1 : 0;
  return 0;
}

int deriv_pending_record_assign(
  struct Instance *root,
  struct Instance *base,
  struct value_t value
){
  struct deriv_pending_entry *entry = deriv_pending_get_or_create(root, base);
  if(entry == NULL){
    return 1;
  }
  if(entry->has_assign){
    DestroyValue(&entry->assign_value);
  }
  entry->assign_value = CopyValue(value);
  entry->has_assign = 1;
  return 0;
}

int deriv_pending_apply(
  struct Instance *root,
  deriv_pending_apply_fn applyfn,
  void *userdata
){
  unsigned long i, len;
  if(root == NULL || applyfn == NULL || g_deriv_pending == NULL){
    return 0;
  }
  len = gl_length(g_deriv_pending);
  for(i = 1; i <= len; ++i){
    struct deriv_pending_entry *entry =
      (struct deriv_pending_entry *)gl_fetch(g_deriv_pending, i);
    if(entry == NULL || entry->root != root){
      continue;
    }
    if(entry->has_assign && applyfn(entry->base, deriv_pending_assign, &entry->assign_value, userdata)){
      return 1;
    }
    if(entry->has_fixed && applyfn(entry->base, entry->fixed_value ? deriv_pending_fix : deriv_pending_free, NULL, userdata)){
      return 1;
    }
  }
  return 0;
}

void deriv_pending_clear_root(struct Instance *root)
{
  unsigned long i;
  if(g_deriv_pending == NULL || root == NULL){
    return;
  }
  for(i = gl_length(g_deriv_pending); i >= 1; --i){
    struct deriv_pending_entry *entry =
      (struct deriv_pending_entry *)gl_fetch(g_deriv_pending, i);
    if(entry == NULL || entry->root != root){
      continue;
    }
    if(entry->has_assign){
      DestroyValue(&entry->assign_value);
    }
    ascfree(entry);
    gl_delete(g_deriv_pending, i, 0);
    if(i == 1){
      break;
    }
  }
  if(gl_length(g_deriv_pending) == 0){
    gl_destroy(g_deriv_pending);
    g_deriv_pending = NULL;
  }
}

void deriv_pending_clear_all(void)
{
  unsigned long i;
  if(g_deriv_pending == NULL){
    return;
  }
  for(i = 1; i <= gl_length(g_deriv_pending); ++i){
    struct deriv_pending_entry *entry =
      (struct deriv_pending_entry *)gl_fetch(g_deriv_pending, i);
    if(entry == NULL){
      continue;
    }
    if(entry->has_assign){
      DestroyValue(&entry->assign_value);
    }
    ascfree(entry);
  }
  gl_destroy(g_deriv_pending);
  g_deriv_pending = NULL;
}
