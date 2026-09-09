#include <assert.h>
#include <string.h>

#include "ifcap-regulatory.h"
#include "drivers/nl80211_copy.h"

size_t os_strlcpy(char *dest, const char *src, size_t size)
{
	size_t len = strlen(src);
	if (size) {
		size_t copy = len < size - 1 ? len : size - 1;
		memcpy(dest, src, copy);
		dest[copy] = '\0';
	}
	return len;
}

int main(void)
{
	struct ifcap_regulatory reg = { 0 };

	reg.dfs_region = NL80211_DFS_ETSI;
	reg.rules[0].start_mhz = 5250;
	reg.rules[0].end_mhz = 5330;
	reg.rules[0].flags = NL80211_RRF_DFS;
	reg.rules[1].start_mhz = 5490;
	reg.rules[1].end_mhz = 5710;
	reg.rules[1].flags = 0;
	reg.num_rules = 2;

	assert(ifcap_regulatory_is_dfs(&reg, 5260));
	assert(!ifcap_regulatory_is_dfs(&reg, 5500));

	reg.num_rules = 0;
	assert(ifcap_regulatory_is_dfs(&reg, 5500));
	assert(!ifcap_regulatory_is_dfs(&reg, 5800));
	return 0;
}
