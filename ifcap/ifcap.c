/* ifcap - hostapd-backed nl80211 capability reporter */
#include "utils/includes.h"
#include "utils/common.h"
#include "utils/eloop.h"
#include "utils/os.h"
#include "utils/wpa_debug.h"
#include "common/version.h"
#include "common/ieee802_11_common.h"
#include "drivers/driver.h"
#include "ap/hostapd.h"
#include "ap/ap_drv_ops.h"
#include "ap/hw_features.h"
#include "tool_build_config.h"
#include "ifcap-regulatory.h"

struct bit_name {
	u64		bit;
	const char     *name;
};

static const struct bit_name channel_flags[] = {
	{HOSTAPD_CHAN_DISABLED, "disabled"}, {HOSTAPD_CHAN_NO_IR, "no_ir"},
	{HOSTAPD_CHAN_RADAR, "radar"}, {HOSTAPD_CHAN_HT40PLUS, "ht40_plus"},
	{HOSTAPD_CHAN_HT40MINUS, "ht40_minus"}, {HOSTAPD_CHAN_HT40, "ht40"},
#ifdef CONFIG_IEEE80211BE
	{HOSTAPD_CHAN_VHT_80MHZ_SUBCHANNEL, "vht_80_subchannel"},
	{HOSTAPD_CHAN_VHT_160MHZ_SUBCHANNEL, "vht_160_subchannel"},
#endif
#ifdef CONFIG_IEEE80211BE
	{HOSTAPD_CHAN_EHT_320MHZ_SUBCHANNEL, "eht_320_subchannel"},
#endif
	{HOSTAPD_CHAN_INDOOR_ONLY, "indoor_only"}, {HOSTAPD_CHAN_GO_CONCURRENT, "go_concurrent"},
#ifdef CONFIG_IEEE80211BE
	{HOSTAPD_CHAN_AUTO_BW, "auto_bw"},
#endif
};
static const struct bit_name width_flags[] = {
	{HOSTAPD_CHAN_WIDTH_10, "10"}, {HOSTAPD_CHAN_WIDTH_20, "20"},
	{HOSTAPD_CHAN_WIDTH_40P, "40_plus"}, {HOSTAPD_CHAN_WIDTH_40M, "40_minus"},
	{HOSTAPD_CHAN_WIDTH_80, "80"}, {HOSTAPD_CHAN_WIDTH_160, "160"},
#ifdef CONFIG_IEEE80211BE
	{HOSTAPD_CHAN_WIDTH_320, "320"},
#endif
};
static const struct bit_name key_mgmt[] = {
	{WPA_DRIVER_CAPA_KEY_MGMT_WPA, "wpa"}, {WPA_DRIVER_CAPA_KEY_MGMT_WPA2, "wpa2"},
	{WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK, "wpa_psk"}, {WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK, "wpa2_psk"},
	{WPA_DRIVER_CAPA_KEY_MGMT_FT, "ft"}, {WPA_DRIVER_CAPA_KEY_MGMT_FT_PSK, "ft_psk"},
	{WPA_DRIVER_CAPA_KEY_MGMT_OWE, "owe"}, {WPA_DRIVER_CAPA_KEY_MGMT_DPP, "dpp"},
	{WPA_DRIVER_CAPA_KEY_MGMT_SAE, "sae"}, {WPA_DRIVER_CAPA_KEY_MGMT_FT_SAE, "ft_sae"},
};
static const struct bit_name ciphers[] = {
	{WPA_DRIVER_CAPA_ENC_WEP40, "wep40"}, {WPA_DRIVER_CAPA_ENC_WEP104, "wep104"},
	{WPA_DRIVER_CAPA_ENC_TKIP, "tkip"}, {WPA_DRIVER_CAPA_ENC_CCMP, "ccmp"},
	{WPA_DRIVER_CAPA_ENC_GCMP, "gcmp"}, {WPA_DRIVER_CAPA_ENC_GCMP_256, "gcmp_256"},
	{WPA_DRIVER_CAPA_ENC_CCMP_256, "ccmp_256"}, {WPA_DRIVER_CAPA_ENC_BIP, "bip"},
	{WPA_DRIVER_CAPA_ENC_BIP_GMAC_128, "bip_gmac_128"}, {WPA_DRIVER_CAPA_ENC_BIP_GMAC_256, "bip_gmac_256"},
	{WPA_DRIVER_CAPA_ENC_BIP_CMAC_256, "bip_cmac_256"},
};
static const struct bit_name auth_algs[] = {
	{WPA_DRIVER_AUTH_OPEN, "open"}, {WPA_DRIVER_AUTH_SHARED, "shared"},
	{WPA_DRIVER_AUTH_LEAP, "leap"},
};
static const struct bit_name driver_flags[] = {
	{WPA_DRIVER_FLAGS_DFS_OFFLOAD, "dfs_offload"}, {WPA_DRIVER_FLAGS_AP, "ap"},
	{WPA_DRIVER_FLAGS_HT_2040_COEX, "ht_2040_coex"}, {WPA_DRIVER_FLAGS_RADAR, "radar"},
	{WPA_DRIVER_FLAGS_AP_CSA, "ap_csa"}, {WPA_DRIVER_FLAGS_MESH, "mesh"},
	{WPA_DRIVER_FLAGS_ACS_OFFLOAD, "acs_offload"}, {WPA_DRIVER_FLAGS_KEY_MGMT_OFFLOAD, "key_mgmt_offload"},
	{WPA_DRIVER_FLAGS_SUPPORT_HW_MODE_ANY, "support_hw_mode_any"},
#ifdef CONFIG_IEEE80211BE
	{WPA_DRIVER_FLAGS_HE_CAPABILITIES, "he_capabilities"}, {WPA_DRIVER_FLAGS_BEACON_PROTECTION, "beacon_protection"},
	{WPA_DRIVER_FLAGS_EXTENDED_KEY_ID, "extended_key_id"},
#endif
};
static const struct bit_name driver_flags2[] = {
	{WPA_DRIVER_FLAGS2_OCV, "ocv"},
#ifdef CONFIG_IEEE80211BE
	{WPA_DRIVER_FLAGS2_AP_SME, "ap_sme"},
	{WPA_DRIVER_FLAGS2_RADAR_BACKGROUND, "radar_background"}, {WPA_DRIVER_FLAGS2_MLO, "mlo"},
	{WPA_DRIVER_FLAGS2_SAE_OFFLOAD_AP, "sae_offload_ap"}, {WPA_DRIVER_FLAGS2_OWE_OFFLOAD_AP, "owe_offload_ap"},
	{WPA_DRIVER_FLAGS2_AP_CHANWIDTH_CHANGE, "ap_chanwidth_change"},
#endif
};

static void	json_string(const char *s){
	const unsigned char *p = (const unsigned char *)s;
	putchar('"');
	for (; *p; p++) {
		if (*p == '"' || *p == '\\')
			printf("\\%c", *p);
		else if (*p < 0x20)
			printf("\\u%04x", *p);
		else
			putchar(*p);
	} putchar('"');
}
static void	json_hex(const u8 * buf, size_t len){
	static const char hex[] = "0123456789abcdef";
	putchar('"');
	for (size_t i = 0; i < len; i++) {
		putchar(hex[buf[i] >> 4]);
		putchar(hex[buf[i] & 15]);
	} putchar('"');
}
static void	json_bits(u64 value, const struct bit_name *names, size_t count){
	size_t		i;
	printf("{\"hex\":\"0x%016llx\",\"names\":[", (unsigned long long)value);
	for (i = 0; i < count; i++)
		if (value & names[i].bit) {
			if (i) {
				size_t		j;
				for (j = 0; j < i; j++)
					if (value & names[j].bit) {
						putchar(',');
						break;
					}
			} json_string(names[i].name);
		}
	printf("]}");
}
static const char *
mode_name(enum hostapd_hw_mode mode)
{
	switch (mode) {
	case HOSTAPD_MODE_IEEE80211A:
		return "a";
	case HOSTAPD_MODE_IEEE80211B:
		return "b";
	case HOSTAPD_MODE_IEEE80211G:
		return "g";
	case HOSTAPD_MODE_IEEE80211AD:
		return "ad";
	default:
		return "unknown";
	}
}
static const char *
opmode_name(int i)
{
	switch (i) {
	case IEEE80211_MODE_INFRA:
		return "infra";
	case IEEE80211_MODE_IBSS:
		return "ibss";
	case IEEE80211_MODE_AP:
		return "ap";
	case IEEE80211_MODE_MESH:
		return "mesh";
	default:
		return "unknown";
	}
}

static void	write_he(const struct he_capabilities *h){
	printf("{\"supported\":%s,\"mac_cap\":", h->he_supported ? "true" : "false");
	json_hex(h->mac_cap, sizeof(h->mac_cap));
	printf(",\"phy_cap\":");
	json_hex(h->phy_cap, sizeof(h->phy_cap));
	printf(",\"mcs\":");
	json_hex(h->mcs, sizeof(h->mcs));
	printf(",\"ppet\":");
	json_hex(h->ppet, sizeof(h->ppet));
	printf(",\"he_6ghz_cap\":%u}", h->he_6ghz_capa);
}
#ifdef CONFIG_IEEE80211BE
static void	write_eht(const struct eht_capabilities *e){
	printf("{\"supported\":%s,\"mac_cap\":%u,\"phy_cap\":", e->eht_supported ? "true" : "false", e->mac_cap);
	json_hex(e->phy_cap, sizeof(e->phy_cap));
	printf(",\"mcs\":");
	json_hex(e->mcs, sizeof(e->mcs));
	printf(",\"ppet\":");
	json_hex(e->ppet, sizeof(e->ppet));
	putchar('}');
}
#endif
static int	chan_radar(const struct ifcap_regulatory *reg, int freq){
	return ifcap_regulatory_is_dfs(reg, freq);
}
static void	json_cac(unsigned int ms){
	if (ms)
		printf("%u", ms);
	else
		printf("null");
}
static int
chan_dfs_available(const struct hostapd_channel_data *c)
{
	return (c->flag & HOSTAPD_CHAN_DFS_MASK) == HOSTAPD_CHAN_DFS_AVAILABLE;
}
#ifdef CONFIG_IEEE80211BE
static void	write_block(const struct hostapd_hw_modes *m, int start, int count, const struct ifcap_regulatory *reg, int dfs_capable){
	int		i, radar = 0, regulatory_radar = 0, dfs_ready = 1, usable = 1;
	unsigned int	cac = 0;
	u32		width_flag = count == 4 ? HOSTAPD_CHAN_WIDTH_80 : count == 8 ? HOSTAPD_CHAN_WIDTH_160 : HOSTAPD_CHAN_WIDTH_320;
	int		center = (m->channels[start].freq + m->channels[start + count - 1].freq) / 2;
	printf("{\"width_mhz\":%d,\"center_frequency_mhz\":%d,\"channels\":[", count * 20, center);
	for (i = 0; i < count; i++) {
		const struct hostapd_channel_data *c = &m->channels[start + i];
		int		this_regulatory_radar = chan_radar(reg, c->freq);
		int		this_radar = this_regulatory_radar || (c->flag & HOSTAPD_CHAN_RADAR);
		if (i)
			putchar(',');
		printf("%d", c->chan);
		regulatory_radar |= this_regulatory_radar;
		radar |= this_radar;
		if (this_radar && !chan_dfs_available(c))
			dfs_ready = 0;
		if ((c->flag & (HOSTAPD_CHAN_DISABLED | HOSTAPD_CHAN_NO_IR)) || !(c->allowed_bw & width_flag) || (this_radar && (!dfs_capable || !chan_dfs_available(c))))
			usable = 0;
		if (c->dfs_cac_ms > cac)
			cac = c->dfs_cac_ms;
	}
	printf("],\"regulatory_radar\":%s,\"ap_usable\":%s,\"requires_dfs_cac\":%s,\"dfs_cac_ms\":", regulatory_radar ? "true" : "false", usable ? "true" : "false", radar && !dfs_ready ? "true" : "false");
	json_cac(cac);
	putchar('}');
}
#endif
#ifdef CONFIG_IEEE80211BE
static int	valid_block_center(const struct hostapd_hw_modes *m, int start, int count){
	int		center;
	if (count == 16)
		return 1;
	center = ieee80211_get_center_freq(m->channels[start].freq, count == 4 ? BW80 : BW160);
	return center && center == (m->channels[start].freq + m->channels[start + count - 1].freq) / 2;
}
static void	write_blocks(const struct hostapd_hw_modes *m, const struct ifcap_regulatory *reg, int dfs_capable){
	int		width, start, i, contiguous;
	printf("\"channel_blocks\":[");
	int		first = 1;
	for (width = 4; width <= 16; width *= 2)
		for (start = 0; start + width <= m->num_channels; start++) {
			contiguous = 1;
			for (i = 1; i < width; i++)
				if (m->channels[start + i].freq != m->channels[start].freq + i * 20)
					contiguous = 0;
			if (!contiguous || !valid_block_center(m, start, width))
				continue;
			if (!first)
				putchar(',');
			first = 0;
			write_block(m, start, width, reg, dfs_capable);
		}
	printf("]");
}
#endif
static void	write_modes(const struct hostapd_hw_modes *modes, u16 num, const struct ifcap_regulatory *reg, int dfs_capable){
	u16		i;
	int		ops[] = {IEEE80211_MODE_INFRA, IEEE80211_MODE_IBSS, IEEE80211_MODE_AP, IEEE80211_MODE_MESH};
	printf("\"hardware_modes\":[");
	for (i = 0; i < num; i++) {
		const struct hostapd_hw_modes *m = &modes[i];
		int		j;
		if (i)
			putchar(',');
		printf("{\"mode\":");
		json_string(mode_name(m->mode));
#ifdef CONFIG_IEEE80211BE
		printf(",\"is_6ghz\":%s,\"flags\":", m->is_6ghz ? "true" : "false");
#else
		printf(",\"flags\":");
#endif
		json_bits(m->flags, NULL, 0);
		printf(",\"ht\":{\"capabilities\":%u,\"mcs\":", m->ht_capab);
		json_hex(m->mcs_set, sizeof(m->mcs_set));
		printf(",\"ampdu_params\":%u},\"vht\":{\"capabilities\":%u,\"mcs\":", m->a_mpdu_params, m->vht_capab);
		json_hex(m->vht_mcs_set, sizeof(m->vht_mcs_set));
		printf("},\"he\":{");
		for (j = 0; j < 4; j++) {
			if (j)
				putchar(',');
			json_string(opmode_name(ops[j]));
			putchar(':');
			write_he(&m->he_capab[ops[j]]);
		}
#ifdef CONFIG_IEEE80211BE
	printf("},\"eht\":{");
		for (j = 0; j < 4; j++) {
			if (j)
				putchar(',');
			json_string(opmode_name(ops[j]));
			putchar(':');
			write_eht(&m->eht_capab[ops[j]]);
		}
	printf("},\"rates_100kbps\":[");
#else
	printf("},\"rates_100kbps\":[");
#endif
		for (j = 0; j < m->num_rates; j++) {
			if (j)
				putchar(',');
			printf("%d", m->rates[j]);
		}
	printf("],\"channels\":[");
		for (j = 0; j < m->num_channels; j++) {
			const struct hostapd_channel_data *c = &m->channels[j];
			int		regulatory_radar = chan_radar(reg, c->freq);
			int		radar = regulatory_radar || (c->flag & HOSTAPD_CHAN_RADAR);
			int		dfs_ready = !radar || chan_dfs_available(c);
			int		usable = !(c->flag & (HOSTAPD_CHAN_DISABLED | HOSTAPD_CHAN_NO_IR)) && (!radar || (dfs_capable && dfs_ready));
			if (j)
				putchar(',');
			printf("{\"number\":%d,\"frequency_mhz\":%d,\"max_tx_power_dbm\":%u,\"flags\":", c->chan, c->freq, c->max_tx_power);
			json_bits(c->flag, channel_flags, ARRAY_SIZE(channel_flags));
			printf(",\"allowed_widths\":");
			json_bits(c->allowed_bw, width_flags, ARRAY_SIZE(width_flags));
#ifdef CONFIG_IEEE80211BE
			printf(",\"dfs_cac_ms\":%u,\"puncturing_bitmap\":%u,\"regulatory_radar\":%s,\"dfs_state\":", c->dfs_cac_ms, c->punct_bitmap, regulatory_radar ? "true" : "false");
#else
			printf(",\"dfs_cac_ms\":%u,\"regulatory_radar\":%s,\"dfs_state\":", c->dfs_cac_ms, regulatory_radar ? "true" : "false");
#endif
			json_string(chan_dfs_available(c) ? "available" : (c->flag & HOSTAPD_CHAN_DFS_MASK) == HOSTAPD_CHAN_DFS_USABLE ? "usable" : (c->flag & HOSTAPD_CHAN_DFS_MASK) == HOSTAPD_CHAN_DFS_UNAVAILABLE ? "unavailable" : "unknown");
			printf(",\"ap\":{\"usable\":%s,\"requires_dfs_cac\":%s,\"dfs_cac_ms\":", usable ? "true" : "false", radar && !dfs_ready ? "true" : "false");
			json_cac(c->dfs_cac_ms);
			printf("}}");
		}
#ifdef CONFIG_IEEE80211BE
	printf("],");
	write_blocks(m, reg, dfs_capable);
#else
	printf("]");
#endif
		printf("}");
	}
	printf("]");
}
static void	write_capa(const struct wpa_driver_capa *c){
	printf("\"driver_capabilities\":{\"key_management\":");
	json_bits(c->key_mgmt, key_mgmt, ARRAY_SIZE(key_mgmt));
	printf(",\"ciphers\":");
	json_bits(c->enc, ciphers, ARRAY_SIZE(ciphers));
	printf(",\"authentication_algorithms\":");
	json_bits(c->auth, auth_algs, ARRAY_SIZE(auth_algs));
	printf(",\"flags\":");
	json_bits(c->flags, driver_flags, ARRAY_SIZE(driver_flags));
	printf(",\"flags2\":");
	json_bits(c->flags2, driver_flags2, ARRAY_SIZE(driver_flags2));
#ifdef CONFIG_IEEE80211BE
	printf(",\"max_stations\":%u,\"max_scan_ssids\":%d,\"max_remain_on_channel_ms\":%u,\"max_csa_counters\":%u,\"mbssid_max_interfaces\":%u,\"ema_max_periodicity\":%u,\"extended_capabilities\":", c->max_stations, c->max_scan_ssids, c->max_remain_on_chan, c->max_csa_counters, c->mbssid_max_interfaces, c->ema_max_periodicity);
#else
	printf(",\"max_stations\":%u,\"max_scan_ssids\":%d,\"max_remain_on_channel_ms\":%u,\"max_csa_counters\":%u,\"extended_capabilities\":", c->max_stations, c->max_scan_ssids, c->max_remain_on_chan, c->max_csa_counters);
#endif
	if (c->extended_capa && c->extended_capa_len)
		json_hex(c->extended_capa, c->extended_capa_len);
	else
		printf("null");
	printf(",\"extended_capabilities_mask\":");
	if (c->extended_capa_mask && c->extended_capa_len)
		json_hex(c->extended_capa_mask, c->extended_capa_len);
	else
		printf("null");
	putchar('}');
}

int		main(int argc, char **argv){
	struct hostapd_data hapd;
	struct wpa_init_params params;
	struct wpa_driver_capa capa;
	struct hostapd_hw_modes *modes = NULL;
	struct ifcap_regulatory reg;
	void	       *global = NULL;
	u16		num_modes = 0, hw_flags = 0;
	u8		dfs_domain = 0;
	int		ret = 1;
	if (argc == 2 && os_strcmp(argv[1], "--build-config") == 0) {
		fputs(tool_build_config, stdout);
		return 0;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: %s [--build-config|INTERFACE]\n", argv[0]);
		return 2;
	}
	if (os_program_init() || eloop_init()) {
		fprintf(stderr, "ifcap: hostapd runtime initialization failed\n");
		return 1;
	}
	wpa_debug_open_file("/dev/stderr");
	os_memset(&hapd, 0, sizeof(hapd));
	os_memset(&params, 0, sizeof(params));
	hapd.driver = &wpa_driver_nl80211_ops;
	global = hapd.driver->global_init(NULL);
	if (!global) {
		fprintf(stderr, "ifcap: nl80211 global initialization failed\n");
		goto out;
	}
	params.global_priv = global;
	params.ifname = argv[1];
	params.own_addr = hapd.own_addr;
	hapd.drv_priv = hapd.driver->hapd_init(&hapd, &params);
	if (!hapd.drv_priv) {
		fprintf(stderr, "ifcap: could not initialize nl80211 for %s\n", argv[1]);
		goto out_global;
	}
	os_memset(&capa, 0, sizeof(capa));
	if (hapd.driver->get_capa(hapd.drv_priv, &capa) ||
	    !(modes = hostapd_get_hw_feature_data(&hapd, &num_modes,
                                           &hw_flags, &dfs_domain))) {
		fprintf(stderr, "ifcap: hostapd capability query failed for %s\n", argv[1]);
		goto out_driver;
	}
	if (ifcap_regulatory_query(&reg))
		wpa_printf(MSG_WARNING, "ifcap: regulatory DFS query unavailable");
	printf("{\"schema_version\":\"1.2\",\"generator\":{\"name\":\"ifcap\",\"hostapd_version\":");
	json_string(VERSION_STR);
	printf("},\"interface\":{\"name\":");
	json_string(argv[1]);
	printf(",\"driver\":\"nl80211\"},\"regulatory\":{\"alpha2\":");
	json_string(reg.alpha2[0] ? reg.alpha2 : "");
	printf(",\"dfs_domain\":%u,\"nl80211_dfs_region\":%u,\"hardware_flags\":", dfs_domain, reg.dfs_region);
	json_bits(hw_flags, NULL, 0);
	printf("},");
	write_capa(&capa);
	putchar(',');
	write_modes(modes, num_modes, &reg, !!(capa.flags & WPA_DRIVER_FLAGS_RADAR));
	puts("}");
	ret = 0;
	hostapd_free_hw_features(modes, num_modes);
out_driver:hapd.driver->hapd_deinit(hapd.drv_priv);
out_global:hapd.driver->global_deinit(global);
out:	wpa_debug_close_file();
	eloop_destroy();
	os_program_deinit();
	return ret;
}
