#ifndef IFCAP_REGULATORY_H
#define IFCAP_REGULATORY_H

#include "utils/includes.h"

struct ifcap_regulatory_rule {
	unsigned int start_mhz;
	unsigned int end_mhz;
	unsigned int flags;
};

#define IFCAP_MAX_REG_RULES 128
struct ifcap_regulatory {
	char alpha2[3];
	unsigned int dfs_region;
	struct ifcap_regulatory_rule rules[IFCAP_MAX_REG_RULES];
	unsigned int num_rules;
};

int ifcap_regulatory_query(struct ifcap_regulatory *reg);
int ifcap_regulatory_is_dfs(const struct ifcap_regulatory *reg, int freq_mhz);

#endif
