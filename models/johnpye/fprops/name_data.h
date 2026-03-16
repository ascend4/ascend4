#ifndef FPROPS_NAME_DATA_H
#define FPROPS_NAME_DATA_H

/*
	Immutable/generated naming metadata for FPROPS entities.

	This layer is filedata-like metadata only. It exists to support:
	- canonical internal names
	- alias lookup
	- formula lookup
	- source/domain filtering

	Runtime selection, package-local overrides, and prepared lookup caches
	belong elsewhere.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	FPROPS_NAME_DOMAIN_NONE              = 0,
	FPROPS_NAME_DOMAIN_PURE_FLUID        = 1u << 0,
	FPROPS_NAME_DOMAIN_EQM_SPECIES       = 1u << 1,
	FPROPS_NAME_DOMAIN_MIXTURE_COMPONENT = 1u << 2,
	FPROPS_NAME_DOMAIN_ANY               = 0xFFFFFFFFu
} FpropsNameDomain;

typedef enum{
	FPROPS_NAME_ALIAS_AUTO    = 1u << 0,
	FPROPS_NAME_ALIAS_MANUAL  = 1u << 1,
	FPROPS_NAME_ALIAS_FORMULA = 1u << 2,
	FPROPS_NAME_ALIAS_NORM    = 1u << 3
} FpropsNameAliasFlags;

typedef struct{
	const char *canonical;
	const char *formula;
	const char *source;
	unsigned domains;
} FpropsNameCanonical;

typedef struct{
	const char *alias;
	const char *canonical;
	const char *source;
	unsigned domains;
	unsigned flags;
	int priority;
} FpropsNameAlias;

typedef struct{
	const FpropsNameCanonical *canonicals;
	int ncanonicals;
	const FpropsNameAlias *aliases;
	int naliases;
} FpropsNameRegistry;

extern const FpropsNameRegistry fprops_name_registry;

#ifdef __cplusplus
}
#endif

#endif
