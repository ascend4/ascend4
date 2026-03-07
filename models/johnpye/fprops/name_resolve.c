#include "name_resolve.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

static int domain_matches(unsigned wanted, unsigned have){
	if(wanted == FPROPS_NAME_DOMAIN_NONE || wanted == FPROPS_NAME_DOMAIN_ANY){
		return 1;
	}
	return (wanted & have) != 0;
}

static int source_matches(const char *wanted, const char *have){
	if(wanted == NULL || wanted[0] == '\0'){
		return 1;
	}
	if(have == NULL || have[0] == '\0'){
		return 1;
	}
	return strcmp(wanted, have) == 0;
}

static int token_matches(const char *a, const char *b){
	return a != NULL && b != NULL && strcmp(a, b) == 0;
}

static void normalize_token(const char *src, char *dst, size_t dstlen){
	size_t j = 0;
	if(dstlen == 0){
		return;
	}
	for(size_t i = 0; src != NULL && src[i] != '\0' && j + 1 < dstlen; ++i){
		unsigned char ch = (unsigned char)src[i];
		if(isalnum(ch)){
			dst[j++] = (char)tolower(ch);
		}
	}
	dst[j] = '\0';
}

static int canonicals_differ(const FpropsNameCanonical *a, const FpropsNameCanonical *b){
	if(a == NULL || b == NULL){
		return 0;
	}
	return strcmp(a->canonical, b->canonical) != 0 || strcmp(a->source, b->source) != 0;
}


static int count_matching_canonicals(
	const char *canonical,
	const char *source,
	unsigned domains,
	const FpropsNameCanonical **first
){
	int i;
	int count = 0;
	const FpropsNameCanonical *best = NULL;
	for(i = 0; i < fprops_name_registry.ncanonicals; ++i){
		const FpropsNameCanonical *c = &fprops_name_registry.canonicals[i];
		if(!token_matches(canonical, c->canonical)){
			continue;
		}
		if(!source_matches(source, c->source)){
			continue;
		}
		if(!domain_matches(domains, c->domains)){
			continue;
		}
		if(best == NULL){
			best = c;
			count = 1;
		}else if(canonicals_differ(best, c)){
			count += 1;
		}
	}
	if(first != NULL){
		*first = best;
	}
	return count;
}


static const char *effective_source_filter(const char *requested, const char *alias_source){
	if(alias_source != NULL && alias_source[0] != '\0'){
		return alias_source;
	}
	return requested;
}


static unsigned effective_domain_filter(unsigned requested, unsigned alias_domains){
	if(requested == FPROPS_NAME_DOMAIN_NONE || requested == FPROPS_NAME_DOMAIN_ANY){
		return alias_domains;
	}
	return requested & alias_domains;
}

FpropsNameResolveStatus fprops_name_resolve(
	const char *token,
	unsigned domains,
	const char *source,
	FpropsResolvedName *out
){
	int i;
	int candidate_count;
	int best_priority = -2147483647;
	int matches = 0;
	const FpropsNameAlias *best_alias = NULL;
	const FpropsNameCanonical *best_canonical = NULL;
	char normalized[256];

	if(out != NULL){
		out->canonical = NULL;
		out->alias = NULL;
	}
	if(token == NULL || token[0] == '\0' || out == NULL){
		return FPROPS_NAME_RESOLVE_INVALID;
	}

	for(i = 0; i < fprops_name_registry.ncanonicals; ++i){
		const FpropsNameCanonical *c = &fprops_name_registry.canonicals[i];
		if(!token_matches(token, c->canonical)){
			continue;
		}
		if(!source_matches(source, c->source)){
			continue;
		}
		if(!domain_matches(domains, c->domains)){
			continue;
		}
		if(best_canonical == NULL){
			best_canonical = c;
			matches = 1;
		}else if(strcmp(best_canonical->canonical, c->canonical) != 0 || strcmp(best_canonical->source, c->source) != 0){
			return FPROPS_NAME_RESOLVE_AMBIGUOUS;
		}
	}
	if(best_canonical != NULL){
		out->canonical = best_canonical;
		out->alias = NULL;
		return FPROPS_NAME_RESOLVE_OK;
	}

	for(i = 0; i < fprops_name_registry.naliases; ++i){
		const FpropsNameAlias *a = &fprops_name_registry.aliases[i];
		const FpropsNameCanonical *c;
		const char *candidate_source;
		unsigned candidate_domains;
		if(!token_matches(token, a->alias)){
			continue;
		}
		if(!source_matches(source, a->source)){
			continue;
		}
		if(!domain_matches(domains, a->domains)){
			continue;
		}
		candidate_source = effective_source_filter(source, a->source);
		candidate_domains = effective_domain_filter(domains, a->domains);
		candidate_count = count_matching_canonicals(a->canonical, candidate_source, candidate_domains, &c);
		if(candidate_count == 0 || c == NULL){
			continue;
		}
		if(candidate_count > 1){
			return FPROPS_NAME_RESOLVE_AMBIGUOUS;
		}
		if(a->priority > best_priority){
			best_priority = a->priority;
			best_alias = a;
			best_canonical = c;
			matches = 1;
		}else if(a->priority == best_priority){
			if(best_canonical == NULL || canonicals_differ(best_canonical, c)){
				matches += 1;
			}
		}
	}
	if(matches == 1 && best_canonical != NULL){
		out->canonical = best_canonical;
		out->alias = best_alias;
		return FPROPS_NAME_RESOLVE_OK;
	}
	if(matches > 1){
		return FPROPS_NAME_RESOLVE_AMBIGUOUS;
	}

	normalize_token(token, normalized, sizeof(normalized));
	if(normalized[0] != '\0' && strcmp(normalized, token) != 0){
		best_priority = -2147483647;
		matches = 0;
		best_alias = NULL;
		best_canonical = NULL;
		for(i = 0; i < fprops_name_registry.naliases; ++i){
			const FpropsNameAlias *a = &fprops_name_registry.aliases[i];
			const FpropsNameCanonical *c;
			const char *candidate_source;
			unsigned candidate_domains;
			if(!token_matches(normalized, a->alias)){
				continue;
			}
			if(!source_matches(source, a->source)){
				continue;
			}
			if(!domain_matches(domains, a->domains)){
				continue;
			}
			candidate_source = effective_source_filter(source, a->source);
			candidate_domains = effective_domain_filter(domains, a->domains);
			candidate_count = count_matching_canonicals(a->canonical, candidate_source, candidate_domains, &c);
			if(candidate_count == 0 || c == NULL){
				continue;
			}
			if(candidate_count > 1){
				return FPROPS_NAME_RESOLVE_AMBIGUOUS;
			}
			if(a->priority > best_priority){
				best_priority = a->priority;
				best_alias = a;
				best_canonical = c;
				matches = 1;
			}else if(a->priority == best_priority){
				if(best_canonical == NULL || canonicals_differ(best_canonical, c)){
					matches += 1;
				}
			}
		}
		if(matches == 1 && best_canonical != NULL){
			out->canonical = best_canonical;
			out->alias = best_alias;
			return FPROPS_NAME_RESOLVE_OK;
		}
		if(matches > 1){
			return FPROPS_NAME_RESOLVE_AMBIGUOUS;
		}
	}

	return FPROPS_NAME_RESOLVE_NOT_FOUND;
}
