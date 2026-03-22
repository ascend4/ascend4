#include <ascend/general/platform.h>
#include <ascend/general/ascMalloc.h>
#include <ascend/general/list.h>
#include <ascend/utilities/error.h>
#include <string.h>

#include "derivinst.h"

#include "createinst.h"
#include "destroyinst.h"
#include "dimen.h"
#include "atomvalue.h"
#include "expr_types.h"
#include "instmacro.h"
#include "instquery.h"
#include "library.h"
#include "link.h"
#include "mathinst.h"
#include "parentchild.h"
#include "relation.h"
#include "relation_util.h"
#include "symtab.h"
#include "type_desc.h"
#include "visitinst.h"

struct derivinst_entry {
  struct Instance *root;
  struct Instance *base;
  struct Instance *inst;
  unsigned int present;
  unsigned int solver_owned;
  unsigned int algebraic_default;
};

struct derivinst_rootinfo {
  struct Instance *root;
  CONST dim_type *indepdim;
  struct Instance *indepinst;
};

static struct gl_list_t *g_derivinst_entries = NULL;
static struct gl_list_t *g_derivinst_scanned_roots = NULL;
static struct gl_list_t *g_derivinst_rootinfo = NULL;

static void derivinst_ensure_root_scanned(struct Instance *root);

static symchar *derivinst_child_name(void){
  return AddSymbol("der");
}

static struct Instance *derivinst_root_for(struct Instance *inst){
  struct Instance *sim;
  if(inst == NULL){
    return NULL;
  }
  sim = FindSimulationInstance(inst);
  if(sim == NULL){
    return NULL;
  }
  return GetSimulationRoot(sim);
}

static int derivinst_can_materialise(struct Instance *base){
  if(base == NULL){
    return 0;
  }
  return InstanceKind(base) == REAL_ATOM_INST;
}

static int derivinst_root_scanned(struct Instance *root){
  unsigned long i, len;
  if(root == NULL || g_derivinst_scanned_roots == NULL){
    return 0;
  }
  len = gl_length(g_derivinst_scanned_roots);
  for(i = 1; i <= len; ++i){
    if((struct Instance *)gl_fetch(g_derivinst_scanned_roots, i) == root){
      return 1;
    }
  }
  return 0;
}

static void derivinst_mark_root_scanned(struct Instance *root){
  if(root == NULL){
    return;
  }
  if(g_derivinst_scanned_roots == NULL){
    g_derivinst_scanned_roots = gl_create(8);
    if(g_derivinst_scanned_roots == NULL){
      return;
    }
  }
  if(!derivinst_root_scanned(root)){
    gl_append_ptr(g_derivinst_scanned_roots, root);
  }
}

static struct derivinst_entry *derivinst_lookup(struct Instance *root, struct Instance *base){
  unsigned long i, len;
  if(root == NULL || base == NULL || g_derivinst_entries == NULL){
    return NULL;
  }
  len = gl_length(g_derivinst_entries);
  for(i = 1; i <= len; ++i){
    struct derivinst_entry *entry = (struct derivinst_entry *)gl_fetch(g_derivinst_entries, i);
    if(entry != NULL && entry->root == root && entry->base == base){
      return entry;
    }
  }
  return NULL;
}

static struct derivinst_entry *derivinst_lookup_by_inst(CONST struct Instance *inst){
  unsigned long i, len;
  if(inst == NULL || g_derivinst_entries == NULL){
    return NULL;
  }
  len = gl_length(g_derivinst_entries);
  for(i = 1; i <= len; ++i){
    struct derivinst_entry *entry = (struct derivinst_entry *)gl_fetch(g_derivinst_entries, i);
    if(entry != NULL && entry->inst == inst){
      return entry;
    }
  }
  return NULL;
}

static void derivinst_destroy_entry(struct derivinst_entry *entry){
  if(entry == NULL){
    return;
  }
  if(entry->inst != NULL && !entry->solver_owned){
    DestroyInstance(entry->inst, NULL);
  }
  ASC_FREE(entry);
}

static struct derivinst_rootinfo *derivinst_rootinfo_lookup(struct Instance *root){
  unsigned long i, len;
  if(root == NULL || g_derivinst_rootinfo == NULL){
    return NULL;
  }
  len = gl_length(g_derivinst_rootinfo);
  for(i = 1; i <= len; ++i){
    struct derivinst_rootinfo *info = (struct derivinst_rootinfo *)gl_fetch(g_derivinst_rootinfo, i);
    if(info != NULL && info->root == root){
      return info;
    }
  }
  return NULL;
}

static struct derivinst_rootinfo *derivinst_rootinfo_get_or_create(struct Instance *root){
  struct derivinst_rootinfo *info;
  if(root == NULL){
    return NULL;
  }
  info = derivinst_rootinfo_lookup(root);
  if(info != NULL){
    return info;
  }
  if(g_derivinst_rootinfo == NULL){
    g_derivinst_rootinfo = gl_create(8);
    if(g_derivinst_rootinfo == NULL){
      return NULL;
    }
  }
  info = ASC_NEW(struct derivinst_rootinfo);
  if(info == NULL){
    return NULL;
  }
  info->root = root;
  info->indepdim = NULL;
  info->indepinst = NULL;
  gl_append_ptr(g_derivinst_rootinfo, info);
  return info;
}

static void derivinst_destroy_rootinfo(struct derivinst_rootinfo *info){
  if(info != NULL){
    ASC_FREE(info);
  }
}

static struct derivinst_entry *derivinst_get_or_create_entry(struct Instance *root, struct Instance *base){
  struct derivinst_entry *entry;
  if(root == NULL || base == NULL){
    return NULL;
  }
  entry = derivinst_lookup(root, base);
  if(entry != NULL){
    return entry;
  }
  if(g_derivinst_entries == NULL){
    g_derivinst_entries = gl_create(8);
    if(g_derivinst_entries == NULL){
      return NULL;
    }
  }
  entry = ASC_NEW(struct derivinst_entry);
  if(entry == NULL){
    return NULL;
  }
  entry->root = root;
  entry->base = base;
  entry->inst = NULL;
  entry->present = 0;
  entry->solver_owned = 0;
  entry->algebraic_default = 1;
  gl_append_ptr(g_derivinst_entries, entry);
  return entry;
}

static struct derivinst_entry *derivinst_lookup_base_entry(struct Instance *base, struct Instance **root_out){
  struct Instance *root;
  struct derivinst_entry *entry;

  if(root_out != NULL){
    *root_out = NULL;
  }
  if(base == NULL || !derivinst_can_materialise(base)){
    return NULL;
  }

  root = derivinst_root_for(base);
  if(root_out != NULL){
    *root_out = root;
  }
  if(root == NULL){
    return NULL;
  }

  entry = derivinst_lookup(root, base);
  if(entry == NULL && !derivinst_root_scanned(root)){
    derivinst_ensure_root_scanned(root);
    entry = derivinst_lookup(root, base);
  }
  return entry;
}

static void derivinst_remove_entries_for_root(struct Instance *root){
  unsigned long i;
  if(root == NULL || g_derivinst_entries == NULL){
    return;
  }
  for(i = gl_length(g_derivinst_entries); i >= 1; --i){
    struct derivinst_entry *entry = (struct derivinst_entry *)gl_fetch(g_derivinst_entries, i);
    if(entry == NULL || entry->root != root){
      if(i == 1){
        break;
      }
      continue;
    }
    gl_delete(g_derivinst_entries, i, 0);
    derivinst_destroy_entry(entry);
    if(i == 1){
      break;
    }
  }
}

static void derivinst_remove_scanned_root(struct Instance *root){
  unsigned long i;
  if(root == NULL || g_derivinst_scanned_roots == NULL){
    return;
  }
  for(i = gl_length(g_derivinst_scanned_roots); i >= 1; --i){
    if((struct Instance *)gl_fetch(g_derivinst_scanned_roots, i) == root){
      gl_delete(g_derivinst_scanned_roots, i, 0);
      if(i == 1){
        break;
      }
    }else if(i == 1){
      break;
    }
  }
}

static void derivinst_remove_rootinfo(struct Instance *root){
  unsigned long i;
  if(root == NULL || g_derivinst_rootinfo == NULL){
    return;
  }
  for(i = gl_length(g_derivinst_rootinfo); i >= 1; --i){
    struct derivinst_rootinfo *info = (struct derivinst_rootinfo *)gl_fetch(g_derivinst_rootinfo, i);
    if(info != NULL && info->root == root){
      gl_delete(g_derivinst_rootinfo, i, 0);
      derivinst_destroy_rootinfo(info);
      if(i == 1){
        break;
      }
    }else if(i == 1){
      break;
    }
  }
}

static CONST dim_type *derivinst_independent_dimensions(struct Instance *root){
  struct gl_list_t *links;
  CONST struct gl_list_t *instances;
  CONST dim_type *dim = NULL;
  struct derivinst_rootinfo *info;
  symchar *independent_key;
  unsigned long i;

  if(root == NULL){
    return NULL;
  }

  info = derivinst_rootinfo_get_or_create(root);
  if(info != NULL && info->indepdim != NULL){
    return info->indepdim;
  }

  independent_key = AddSymbol("independent");
  links = getLinks(root, independent_key, 0);
  if(links == NULL){
    return NULL;
  }

  for(i = 1; i <= gl_length(links); ++i){
    struct link_entry_t *entry = (struct link_entry_t *)gl_fetch(links, i);
    if(entry == NULL){
      continue;
    }
    instances = getLinkInstances(root, entry, 0);
    if(instances == NULL || gl_length((struct gl_list_t *)instances) != 1){
      dim = NULL;
      break;
    }
    {
      struct Instance *indep = (struct Instance *)gl_fetch((struct gl_list_t *)instances, 1);
      if(indep != NULL && InstanceKind(indep) == REAL_ATOM_INST){
        if(info != NULL){
          info->indepinst = indep;
        }
        dim = RealAtomDims(indep);
      }else{
        if(info != NULL){
          info->indepinst = NULL;
        }
        dim = NULL;
      }
    }
    break;
  }

  gl_destroy(links);
  if(info != NULL){
    info->indepdim = dim;
  }
  return dim;
}

static struct Instance *derivinst_independent_instance(struct Instance *root){
  struct derivinst_rootinfo *info;
  if(root == NULL){
    return NULL;
  }
  info = derivinst_rootinfo_get_or_create(root);
  if(info != NULL && info->indepinst != NULL){
    return info->indepinst;
  }
  (void)derivinst_independent_dimensions(root);
  return info != NULL ? info->indepinst : NULL;
}

static void derivinst_set_dimensions(struct Instance *inst, struct Instance *base, struct Instance *root){
  CONST dim_type *basedim, *indepdim, *derdim;
  if(inst == NULL || base == NULL){
    return;
  }
  if(InstanceKind(base) != REAL_ATOM_INST){
    SetRealAtomDims(inst, WildDimension());
    return;
  }
  basedim = RealAtomDims(base);
  indepdim = derivinst_independent_dimensions(root);
  if(basedim == NULL){
    derdim = WildDimension();
  }else if(indepdim == NULL){
    derdim = basedim;
  }else{
    derdim = DiffDimensions(basedim, indepdim, 0);
    if(derdim == NULL){
      derdim = WildDimension();
    }
  }
  SetRealAtomDims(inst, derdim);
}

static struct Instance *derivinst_create_instance(struct Instance *base, struct Instance *root){
  struct TypeDescription *desc;
  struct Instance *inst;

  desc = FindType(AddSymbol("solver_var"));
  if(desc == NULL && base != NULL){
    desc = InstanceTypeDesc(base);
  }
  if(desc == NULL){
    return NULL;
  }

  inst = CreateRealInstance(desc);
  if(inst == NULL){
    return NULL;
  }
  SetRealAtomValue(inst, 0.0, 0U);
  derivinst_set_dimensions(inst, base, root);
  return inst;
}

struct derivinst_scan_data {
  struct Instance *root;
};

static void derivinst_scan_instance(struct Instance *inst, VOIDPTR userdata){
  struct derivinst_scan_data *data = (struct derivinst_scan_data *)userdata;
  struct relation *rel;
  unsigned long i;

  if(data == NULL || inst == NULL || InstanceKind(inst) != REL_INST){
    return;
  }
  if(GetInstanceRelationType(inst) != e_token){
    return;
  }

  rel = (struct relation *)GetInstanceRelationOnly(inst);
  if(rel == NULL){
    return;
  }

  if(RTOKEN(rel).lhs != NULL){
    for(i = 0; i < RTOKEN(rel).lhs_len; ++i){
      struct relation_term *term = A_TERM(&(RTOKEN(rel).lhs[i]));
      if(term != NULL && term->t == e_der){
        struct Instance *base = RelationVariable(rel, TermVarNumber(term));
        if(base != NULL){
          DerivativeInstancesMarkPresent(data->root, base);
        }
      }
    }
  }
  if(RTOKEN(rel).rhs != NULL){
    for(i = 0; i < RTOKEN(rel).rhs_len; ++i){
      struct relation_term *term = A_TERM(&(RTOKEN(rel).rhs[i]));
      if(term != NULL && term->t == e_der){
        struct Instance *base = RelationVariable(rel, TermVarNumber(term));
        if(base != NULL){
          DerivativeInstancesMarkPresent(data->root, base);
        }
      }
    }
  }
}

static void derivinst_ensure_root_scanned(struct Instance *root){
  struct derivinst_scan_data data;
  if(root == NULL || derivinst_root_scanned(root)){
    return;
  }
  data.root = root;
  VisitInstanceTreeTwo(root, derivinst_scan_instance, 0, 0, &data);
  derivinst_mark_root_scanned(root);
}

void DerivativeInstancesPrepareRoot(struct Instance *root){
  (void)derivinst_independent_dimensions(root);
  derivinst_ensure_root_scanned(root);
}

int DerivativeInstancesMarkPresent(struct Instance *root, struct Instance *base){
  struct derivinst_entry *entry;
  if(root == NULL || base == NULL || !derivinst_can_materialise(base)){
    return 1;
  }
  entry = derivinst_get_or_create_entry(root, base);
  if(entry == NULL){
    return 1;
  }
  entry->present = 1;
  return 0;
}

int InstanceHasDerivative(struct Instance *base){
  struct derivinst_entry *entry;
  entry = derivinst_lookup_base_entry(base, NULL);
  return entry != NULL && entry->present;
}

struct Instance *InstanceGetDerivative(struct Instance *base){
  struct derivinst_entry *entry;
  entry = derivinst_lookup_base_entry(base, NULL);
  if(entry == NULL || !entry->present){
    return NULL;
  }
  return entry->inst;
}

struct Instance *InstanceEnsureDerivative(struct Instance *base){
  struct Instance *root;
  struct derivinst_entry *entry;
  entry = derivinst_lookup_base_entry(base, &root);
  if(entry == NULL || !entry->present){
    return NULL;
  }
  if(entry->inst == NULL){
    entry->inst = derivinst_create_instance(base, root);
    entry->solver_owned = 0;
  }
  return entry->inst;
}

unsigned long InstanceDynamicChildCount(struct Instance *inst){
  return InstanceHasDerivative(inst) ? 1UL : 0UL;
}

struct Instance *InstanceDynamicChild(struct Instance *inst, unsigned long n){
  if(n != 1){
    return NULL;
  }
  return InstanceEnsureDerivative(inst);
}

symchar *InstanceDynamicChildName(struct Instance *inst, unsigned long n){
  if(n != 1 || !InstanceHasDerivative(inst)){
    return NULL;
  }
  return derivinst_child_name();
}

struct Instance *InstanceDynamicChildByChar(struct Instance *inst, symchar *name){
  if(name == NULL || strcmp(SCP(name), SCP(derivinst_child_name())) != 0){
    return NULL;
  }
  return InstanceEnsureDerivative(inst);
}

unsigned long InstanceDynamicChildIndex(struct Instance *inst, struct Instance *child){
  struct Instance *deriv;
  if(inst == NULL || child == NULL){
    return 0;
  }
  deriv = InstanceGetDerivative(inst);
  if(deriv != NULL && deriv == child){
    return 1UL;
  }
  return 0;
}

int IsDerivativeInstance(CONST struct Instance *inst){
  return derivinst_lookup_by_inst(inst) != NULL;
}

struct Instance *DerivativeInstanceBase(CONST struct Instance *inst){
  struct derivinst_entry *entry = derivinst_lookup_by_inst(inst);
  return entry != NULL ? entry->base : NULL;
}

unsigned long DerivativeInstanceOrder(CONST struct Instance *inst){
  return IsDerivativeInstance(inst) ? 1UL : 0UL;
}

struct Instance *DerivativeInstanceIndependent(CONST struct Instance *inst){
  struct Instance *base;
  struct Instance *root;
  base = DerivativeInstanceBase(inst);
  if(base == NULL){
    return NULL;
  }
  root = derivinst_root_for(base);
  if(root == NULL){
    return NULL;
  }
  return derivinst_independent_instance(root);
}

int DerivativeInstanceUsesAlgebraicDefault(CONST struct Instance *inst){
  struct derivinst_entry *entry = derivinst_lookup_by_inst(inst);
  return entry != NULL ? (entry->algebraic_default != 0U) : 0;
}

void DerivativeInstanceClearAlgebraicDefault(struct Instance *inst){
  struct derivinst_entry *entry = derivinst_lookup_by_inst(inst);
  if(entry != NULL){
    entry->algebraic_default = 0U;
  }
}

void DerivativeInstanceNoteMutation(struct Instance *inst){
  struct Instance *parent;
  if(inst == NULL){
    return;
  }
  if(IsDerivativeInstance(inst)){
    DerivativeInstanceClearAlgebraicDefault(inst);
    return;
  }
  parent = InstanceParent(inst, 1);
  if(parent != NULL && IsDerivativeInstance(parent)){
    DerivativeInstanceClearAlgebraicDefault(parent);
  }
}

void DerivativeInstanceMarkSolverOwned(struct Instance *inst){
  struct derivinst_entry *entry = derivinst_lookup_by_inst(inst);
  if(entry != NULL){
    entry->solver_owned = 1;
  }
}

void DerivativeInstanceDetach(struct Instance *inst){
  struct derivinst_entry *entry = derivinst_lookup_by_inst(inst);
  if(entry != NULL){
    entry->inst = NULL;
    entry->solver_owned = 0;
  }
}

void DerivativeInstancesClearRoot(struct Instance *root){
  if(root == NULL){
    return;
  }
  derivinst_remove_entries_for_root(root);
  derivinst_remove_scanned_root(root);
  derivinst_remove_rootinfo(root);
}

void DerivativeInstancesClearAll(void){
  unsigned long i;
  if(g_derivinst_entries != NULL){
    for(i = gl_length(g_derivinst_entries); i >= 1; --i){
      struct derivinst_entry *entry = (struct derivinst_entry *)gl_fetch(g_derivinst_entries, i);
      derivinst_destroy_entry(entry);
      if(i == 1){
        break;
      }
    }
    gl_destroy(g_derivinst_entries);
    g_derivinst_entries = NULL;
  }
  if(g_derivinst_scanned_roots != NULL){
    gl_destroy(g_derivinst_scanned_roots);
    g_derivinst_scanned_roots = NULL;
  }
  if(g_derivinst_rootinfo != NULL){
    for(i = gl_length(g_derivinst_rootinfo); i >= 1; --i){
      struct derivinst_rootinfo *info = (struct derivinst_rootinfo *)gl_fetch(g_derivinst_rootinfo, i);
      derivinst_destroy_rootinfo(info);
      if(i == 1){
        break;
      }
    }
    gl_destroy(g_derivinst_rootinfo);
    g_derivinst_rootinfo = NULL;
  }
}
