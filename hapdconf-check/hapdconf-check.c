/* hapdconf-check - validate a hostapd configuration without driver startup */
#include "utils/includes.h"
#include "utils/common.h"
#include "utils/os.h"
#include "utils/wpa_debug.h"
#include "ap/ap_config.h"
#include "config_file.h"
#include "tool_build_config.h"

struct feature_gate {
	const char *directive;
	const char *config_option;
	int prefix;
	int enabled;
};

/*
 * hostapd reports a compile-time-gated item as simply unknown. Keep this
 * deliberately small and explicit: it is a usability layer over hostapd's
 * parser, not a second parser that must track every configuration item.
 */
static const struct feature_gate feature_gates[] = {
#ifdef EAP_SERVER
	{ "eap_server", "CONFIG_EAP", 0, 1 },
#else
	{ "eap_server", "CONFIG_EAP", 0, 0 },
	{ "eap_user_file", "CONFIG_EAP", 0, 0 },
	{ "ca_cert", "CONFIG_EAP", 0, 0 },
	{ "server_cert", "CONFIG_EAP", 0, 0 },
	{ "private_key", "CONFIG_EAP", 0, 0 },
	{ "private_key_passwd", "CONFIG_EAP", 0, 0 },
#endif
#ifdef CONFIG_IEEE80211AC
	{ "ieee80211ac", "CONFIG_IEEE80211AC", 0, 1 },
	{ "vht_", "CONFIG_IEEE80211AC", 1, 1 },
#else
	{ "ieee80211ac", "CONFIG_IEEE80211AC", 0, 0 },
	{ "vht_", "CONFIG_IEEE80211AC", 1, 0 },
#endif
#ifdef CONFIG_IEEE80211AX
	{ "ieee80211ax", "CONFIG_IEEE80211AX", 0, 1 },
	{ "he_", "CONFIG_IEEE80211AX", 1, 1 },
	{ "require_he", "CONFIG_IEEE80211AX", 0, 1 },
#else
	{ "ieee80211ax", "CONFIG_IEEE80211AX", 0, 0 },
	{ "he_", "CONFIG_IEEE80211AX", 1, 0 },
	{ "require_he", "CONFIG_IEEE80211AX", 0, 0 },
#endif
#ifdef CONFIG_IEEE80211BE
	{ "ieee80211be", "CONFIG_IEEE80211BE", 0, 1 },
	{ "eht_", "CONFIG_IEEE80211BE", 1, 1 },
	{ "mld_", "CONFIG_IEEE80211BE", 1, 1 },
	{ "punct_", "CONFIG_IEEE80211BE", 1, 1 },
	{ "disable_11be", "CONFIG_IEEE80211BE", 0, 1 },
	{ "require_eht", "CONFIG_IEEE80211BE", 0, 1 },
	{ "bss_require_eht", "CONFIG_IEEE80211BE", 0, 1 },
#else
	{ "ieee80211be", "CONFIG_IEEE80211BE", 0, 0 },
	{ "eht_", "CONFIG_IEEE80211BE", 1, 0 },
	{ "mld_", "CONFIG_IEEE80211BE", 1, 0 },
	{ "punct_", "CONFIG_IEEE80211BE", 1, 0 },
	{ "disable_11be", "CONFIG_IEEE80211BE", 0, 0 },
	{ "require_eht", "CONFIG_IEEE80211BE", 0, 0 },
	{ "bss_require_eht", "CONFIG_IEEE80211BE", 0, 0 },
#endif
#ifdef CONFIG_IEEE80211R
	{ "ieee80211r", "CONFIG_IEEE80211R", 0, 1 },
#else
	{ "ieee80211r", "CONFIG_IEEE80211R", 0, 0 },
#endif
#ifdef CONFIG_WPS
	{ "wps_", "CONFIG_WPS", 1, 1 },
#else
	{ "wps_", "CONFIG_WPS", 1, 0 },
#endif
#ifdef CONFIG_SAE
	{ "sae_", "CONFIG_SAE", 1, 1 },
#else
	{ "sae_", "CONFIG_SAE", 1, 0 },
#endif
#ifdef CONFIG_DPP
	{ "dpp_", "CONFIG_DPP", 1, 1 },
#else
	{ "dpp_", "CONFIG_DPP", 1, 0 },
#endif
#ifdef CONFIG_OCV
	{ "ocv", "CONFIG_OCV", 0, 1 },
#else
	{ "ocv", "CONFIG_OCV", 0, 0 },
#endif
};

static int check_feature_gates(const char *path)
{
	FILE *f;
	char line[1024];
	int errors = 0;

	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "hapdconf-check: failed to read '%s'\n", path);
		return 0; /* Let hostapd print its usual file-open diagnostic. */
	}
	while (fgets(line, sizeof(line), f)) {
		char *key = line;
		char *end;
		size_t i;

		while (*key == ' ' || *key == '\t')
			key++;
		if (*key == '#' || *key == '\n' || !*key)
			continue;
		end = os_strchr(key, '=');
		if (!end)
			continue;
		*end = '\0';
		for (i = 0; i < ARRAY_SIZE(feature_gates); i++) {
			const struct feature_gate *gate = &feature_gates[i];
			if (gate->enabled)
				continue;
			if (gate->prefix ? os_strncmp(key, gate->directive,
						     os_strlen(gate->directive)) != 0 :
			    os_strcmp(key, gate->directive) != 0)
				continue;
			fprintf(stderr, "%s requires %s,\n"
				"which is not enabled in this hapdconf-check build.\n"
				"Run 'hapdconf-check --build-config' to inspect compiled features.\n",
				key, gate->config_option);
			errors++;
			break;
		}
	}
	fclose(f);
	return errors;
}

int main(int argc, char **argv)
{
	struct hostapd_config *conf;
	int ret = 1;

	if (argc == 2 && os_strcmp(argv[1], "--build-config") == 0) {
		fputs(tool_build_config, stdout);
		return 0;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: %s [--build-config|CONFIG_FILE]\n", argv[0]);
		return 2;
	}
	if (check_feature_gates(argv[1]))
		return 1;

	if (os_program_init()) {
		fprintf(stderr, "hapdconf-check: hostapd runtime initialization failed\n");
		return 1;
	}

	/* Keep hostapd's exact parse and semantic diagnostics off stdout. */
	wpa_debug_open_file("/dev/stderr");
	conf = hostapd_config_read(argv[1]);
	if (conf) {
		hostapd_config_free(conf);
		ret = 0;
	}
	wpa_debug_close_file();
	os_program_deinit();
	return ret;
}
