#include "utils/includes.h"
#include "utils/common.h"
#include "ifcap-regulatory.h"
#include "drivers/nl80211_copy.h"
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

struct reg_query_ctx {
	struct ifcap_regulatory *reg;
};

static int
reg_valid_cb(struct nl_msg *msg, void *arg)
{
	struct reg_query_ctx *ctx = arg;
	struct genlmsghdr *gh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr  *tb[NL80211_ATTR_MAX + 1];
	struct nlattr  *rule;
	int		rem;
	static struct nla_policy policy[NL80211_ATTR_MAX + 1] = {
		[NL80211_ATTR_REG_ALPHA2] = {.type = NLA_STRING},
		[NL80211_ATTR_DFS_REGION] = {.type = NLA_U8},
		[NL80211_ATTR_REG_RULES] = {.type = NLA_NESTED},
	};
	static struct nla_policy rule_policy[NL80211_REG_RULE_ATTR_MAX + 1] = {
		[NL80211_ATTR_REG_RULE_FLAGS] = {.type = NLA_U32},
		[NL80211_ATTR_FREQ_RANGE_START] = {.type = NLA_U32},
		[NL80211_ATTR_FREQ_RANGE_END] = {.type = NLA_U32},
	};

	if (nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gh, 0),
		      genlmsg_attrlen(gh, 0), policy))
		return NL_OK;
	if (tb[NL80211_ATTR_REG_ALPHA2])
		os_strlcpy(ctx->reg->alpha2, nla_get_string(tb[NL80211_ATTR_REG_ALPHA2]),
			   sizeof(ctx->reg->alpha2));
	if (tb[NL80211_ATTR_DFS_REGION])
		ctx->reg->dfs_region = nla_get_u8(tb[NL80211_ATTR_DFS_REGION]);
	if (!tb[NL80211_ATTR_REG_RULES])
		return NL_OK;

	nla_for_each_nested(rule, tb[NL80211_ATTR_REG_RULES], rem) {
		struct nlattr  *rtb[NL80211_REG_RULE_ATTR_MAX + 1];
		struct ifcap_regulatory_rule *dst;
		if (ctx->reg->num_rules >= IFCAP_MAX_REG_RULES)
			break;
		if (nla_parse_nested(rtb, NL80211_REG_RULE_ATTR_MAX, rule,
				     rule_policy) ||
		    !rtb[NL80211_ATTR_FREQ_RANGE_START] ||
		    !rtb[NL80211_ATTR_FREQ_RANGE_END])
			continue;
		dst = &ctx->reg->rules[ctx->reg->num_rules++];
		dst->start_mhz = nla_get_u32(rtb[NL80211_ATTR_FREQ_RANGE_START]) / 1000;
		dst->end_mhz = nla_get_u32(rtb[NL80211_ATTR_FREQ_RANGE_END]) / 1000;
		dst->flags = rtb[NL80211_ATTR_REG_RULE_FLAGS] ?
			nla_get_u32(rtb[NL80211_ATTR_REG_RULE_FLAGS]) : 0;
	}
	return NL_OK;
}

int
ifcap_regulatory_query(struct ifcap_regulatory *reg)
{
	struct nl_sock *sock = NULL;
	struct nl_msg  *msg = NULL;
	struct reg_query_ctx ctx;
	int		family, ret = -1;

	os_memset(reg, 0, sizeof(*reg));
	sock = nl_socket_alloc();
	if (!sock || genl_connect(sock) < 0)
		goto out;
	family = genl_ctrl_resolve(sock, "nl80211");
	if (family < 0)
		goto out;
	msg = nlmsg_alloc();
	if (!msg || !genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family, 0,
				 NLM_F_REQUEST, NL80211_CMD_GET_REG, 0))
		goto out;
	ctx.reg = reg;
	nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, reg_valid_cb, &ctx);
	if (nl_send_auto(sock, msg) < 0 || nl_recvmsgs_default(sock) < 0)
		goto out;
	/*
	 * A kernel may return the active alpha2 without rule details (for
	 * example while regulatory state is being refreshed). Preserve that
	 * metadata rather than treating the whole query as unusable.
	 */
	ret = (reg->num_rules || reg->alpha2[0]) ? 0 : -1;
out:
	nlmsg_free(msg);
	nl_socket_free(sock);
	return ret;
}

int
ifcap_regulatory_is_dfs(const struct ifcap_regulatory *reg, int freq_mhz)
{
	unsigned int	i;
	for (i = 0; i < reg->num_rules; i++) {
		const struct ifcap_regulatory_rule *r = &reg->rules[i];
		if (freq_mhz >= (int)r->start_mhz && freq_mhz <= (int)r->end_mhz &&
		    (r->flags & NL80211_RRF_DFS))
			return 1;
	}
	/* Some kernel/hwsim combinations expose the ETSI DFS region but omit
	 * DFS bits from individual GET_REG rules. Hostapd still treats the
	 * standard mid-band 5 GHz channels as radar in that region. */
	if (reg->dfs_region == NL80211_DFS_ETSI && freq_mhz >= 5260 &&
	    freq_mhz <= 5720)
		return 1;
	return 0;
}
