/*
 * Copyright (C) 2013 Spreadtrum Communications Inc.
 *
 * Filename : itm_cfg80211.c
 * Abstract : This file is a implementation for cfg80211 subsystem
 *
 * Authors:
 * Leon Liu <leon.liu@spreadtrum.com>
 * Wenjie.Zhang <Wenjie.Zhang@spreadtrum.com>
 * Keguang Zhang <keguang.zhang@spreadtrum.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/spinlock.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>

#ifdef CONFIG_ITM_WIFI_DIRECT
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#endif	/* CONFIG_ITM_WIFI_DIRECT */

#include "sipc.h"
#include "ittiam.h"
#include "cfg80211.h"

/*----------CFG80211 macros and variables----------*/
#define RATETAB_ENT(_rate, _rateid, _flags)				\
{									\
	.bitrate	= (_rate),					\
	.hw_value	= (_rateid),					\
	.flags		= (_flags),					\
}

#define CHAN2G(_channel, _freq, _flags) {				\
	.band			= IEEE80211_BAND_2GHZ,			\
	.center_freq		= (_freq),				\
	.hw_value		= (_channel),				\
	.flags			= (_flags),				\
	.max_antenna_gain	= 0,					\
	.max_power		= 30,					\
}

#define CHAN5G(_channel, _flags) {					\
	.band			= IEEE80211_BAND_5GHZ,			\
	.center_freq		= 5000 + (5 * (_channel)),		\
	.hw_value		= (_channel),				\
	.flags			= (_flags),				\
	.max_antenna_gain	= 0,					\
	.max_power		= 30,					\
}

static struct ieee80211_rate itm_rates[] = {
	RATETAB_ENT(10, 0x1, 0),
	RATETAB_ENT(20, 0x2, 0),
	RATETAB_ENT(55, 0x5, 0),
	RATETAB_ENT(110, 0xb, 0),
	RATETAB_ENT(60, 0x6, 0),
	RATETAB_ENT(90, 0x9, 0),
	RATETAB_ENT(120, 0xc, 0),
	RATETAB_ENT(180, 0x12, 0),
	RATETAB_ENT(240, 0x18, 0),
	RATETAB_ENT(360, 0x24, 0),
	RATETAB_ENT(480, 0x30, 0),
	RATETAB_ENT(540, 0x36, 0),

	RATETAB_ENT(65, 0x80, 0),
	RATETAB_ENT(130, 0x81, 0),
	RATETAB_ENT(195, 0x82, 0),
	RATETAB_ENT(260, 0x83, 0),
	RATETAB_ENT(390, 0x84, 0),
	RATETAB_ENT(520, 0x85, 0),
	RATETAB_ENT(585, 0x86, 0),
	RATETAB_ENT(650, 0x87, 0),
	RATETAB_ENT(130, 0x88, 0),
	RATETAB_ENT(260, 0x89, 0),
	RATETAB_ENT(390, 0x8a, 0),
	RATETAB_ENT(520, 0x8b, 0),
	RATETAB_ENT(780, 0x8c, 0),
	RATETAB_ENT(1040, 0x8d, 0),
	RATETAB_ENT(1170, 0x8e, 0),
	RATETAB_ENT(1300, 0x8f, 0),
};

#define ITM_G_RATE_NUM	28
#define itm_g_rates		(itm_rates)
#define ITM_A_RATE_NUM	24
#define itm_a_rates		(itm_rates + 4)

#define itm_g_htcap (IEEE80211_HT_CAP_SUP_WIDTH_20_40 | \
			IEEE80211_HT_CAP_SGI_20		 | \
			IEEE80211_HT_CAP_SGI_40)

#ifdef CONFIG_ITM_WIFI_DIRECT
#define P2P_IE_ID 221
#define P2P_IE_OUI_BYTE0 0x50
#define P2P_IE_OUI_BYTE1 0x6F
#define P2P_IE_OUI_BYTE2 0x9A
#define P2P_IE_OUI_TYPE 0x09
#endif	/* CONFIG_ITM_WIFI_DIRECT */

static struct ieee80211_channel itm_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

/*static struct ieee80211_channel itm_5ghz_channels[] = {
	CHAN5G(34, 0), CHAN5G(36, 0),
	CHAN5G(38, 0), CHAN5G(40, 0),
	CHAN5G(42, 0), CHAN5G(44, 0),
	CHAN5G(46, 0), CHAN5G(48, 0),
	CHAN5G(52, 0), CHAN5G(56, 0),
	CHAN5G(60, 0), CHAN5G(64, 0),
	CHAN5G(100, 0), CHAN5G(104, 0),
	CHAN5G(108, 0), CHAN5G(112, 0),
	CHAN5G(116, 0), CHAN5G(120, 0),
	CHAN5G(124, 0), CHAN5G(128, 0),
	CHAN5G(132, 0), CHAN5G(136, 0),
	CHAN5G(140, 0), CHAN5G(149, 0),
	CHAN5G(153, 0), CHAN5G(157, 0),
	CHAN5G(161, 0), CHAN5G(165, 0),
	CHAN5G(184, 0), CHAN5G(188, 0),
	CHAN5G(192, 0), CHAN5G(196, 0),
	CHAN5G(200, 0), CHAN5G(204, 0),
	CHAN5G(208, 0), CHAN5G(212, 0),
	CHAN5G(216, 0),
};*/

static struct ieee80211_supported_band itm_band_2ghz = {
	.n_channels = ARRAY_SIZE(itm_2ghz_channels),
	.channels = itm_2ghz_channels,
	.n_bitrates = ITM_G_RATE_NUM,
	.bitrates = itm_g_rates,
	.ht_cap.cap = itm_g_htcap,
	.ht_cap.ht_supported = true,
};

/*static struct ieee80211_supported_band itm_band_5ghz = {
	.n_channels = ARRAY_SIZE(itm_5ghz_channels),
	.channels = itm_5ghz_channels,
	.n_bitrates = ITM_A_RATE_NUM,
	.bitrates = itm_a_rates,
	.ht_cap.cap = itm_g_htcap,
	.ht_cap.ht_supported = true,
};*/

static const u32 itm_cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	WLAN_CIPHER_SUITE_SMS4,
#ifdef BSS_ACCESS_POINT_MODE
	WLAN_CIPHER_SUITE_ITM_CCMP,
	WLAN_CIPHER_SUITE_ITM_TKIP,
#endif
};

/* Supported mgmt frame types to be advertised to cfg80211 */
static const struct ieee80211_txrx_stypes
itm_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_STATION] = {
				    .tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				    BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
				    .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				    BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
				    },
	[NL80211_IFTYPE_AP] = {
			       .tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			       BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
			       .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
			       BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
			       },
	[NL80211_IFTYPE_P2P_CLIENT] = {
				       .tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				       BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
				       .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				       BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
				       },
	[NL80211_IFTYPE_P2P_GO] = {
				   .tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				   BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
				   .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				   BIT(IEEE80211_STYPE_PROBE_REQ >> 4),
				   },
#ifdef CONFIG_ITM_WIFI_DIRECT
/* Supported mgmt frame types for p2p*/
	[NL80211_IFTYPE_ADHOC] = {
				  .tx = 0xffff,
				  .rx = BIT(IEEE80211_STYPE_ACTION >> 4)
				  },
	[NL80211_IFTYPE_STATION] = {
				    .tx = 0xffff,
				    .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				    BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
				    },
	[NL80211_IFTYPE_AP] = {
			       .tx = 0xffff,
			       .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
			       BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
			       BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
			       BIT(IEEE80211_STYPE_DISASSOC >> 4) |
			       BIT(IEEE80211_STYPE_AUTH >> 4) |
			       BIT(IEEE80211_STYPE_DEAUTH >> 4) |
			       BIT(IEEE80211_STYPE_ACTION >> 4)
			       },
	[NL80211_IFTYPE_AP_VLAN] = {
				    /* copy AP */
				    .tx = 0xffff,
				    .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
				    BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
				    BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
				    BIT(IEEE80211_STYPE_DISASSOC >> 4) |
				    BIT(IEEE80211_STYPE_AUTH >> 4) |
				    BIT(IEEE80211_STYPE_DEAUTH >> 4) |
				    BIT(IEEE80211_STYPE_ACTION >> 4)
				    },
	[NL80211_IFTYPE_P2P_CLIENT] = {
				       .tx = 0xffff,
				       .rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
				       BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
				       },
	[NL80211_IFTYPE_P2P_GO] = {
				   .tx = 0xffff,
				   .rx = BIT(IEEE80211_STYPE_ASSOC_REQ >> 4) |
				   BIT(IEEE80211_STYPE_REASSOC_REQ >> 4) |
				   BIT(IEEE80211_STYPE_PROBE_REQ >> 4) |
				   BIT(IEEE80211_STYPE_DISASSOC >> 4) |
				   BIT(IEEE80211_STYPE_AUTH >> 4) |
				   BIT(IEEE80211_STYPE_DEAUTH >> 4) |
				   BIT(IEEE80211_STYPE_ACTION >> 4)
				   },
#endif	/* CONFIG_ITM_WIFI_DIRECT */
};

#ifdef CONFIG_ITM_WIFI_DIRECT
#define P2P_MODE_PATH "/data/misc/wifi/fwpath"

static int get_file_size(struct file *f)
{
	int error = -EBADF;
	struct kstat stat;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	error = vfs_getattr(&f->f_path, &stat);
#else
	error = vfs_getattr(f->f_path.mnt, f->f_path.dentry, &stat);
#endif
	if (error == 0) {
		return stat.size;
	} else {
		pr_err("get conf file stat error\n");
		return error;
	}
}

static char type_name[16][32] = {
	"ASSO REQ",
	"ASSO RESP",
	"REASSO REQ",
	"REASSO RESP",
	"PROBE REQ",
	"PROBE RESP",
	"TIMING ADV",
	"RESERVED",
	"BEACON",
	"ATIM",
	"DISASSO",
	"AUTH",
	"DEAUTH",
	"ACTION",
	"ACTION NO ACK",
	"RESERVED"
};

static char pub_action_name[][32] = {
	"GO Negotiation Req",
	"GO Negotiation Resp",
	"GO Negotiation Conf",
	"P2P Invitation Req",
	"P2P Invitation Resp",
	"Device Discovery Req",
	"Device Discovery Resp",
	"Provision Discovery Req",
	"Provision Discovery Resp",
	"Reserved"
};

static char p2p_action_name[][32] = {
	"Notice of Absence",
	"P2P Precence Req",
	"P2P Precence Resp",
	"GO Discoverability Req",
	"Reserved"
};

#define MAC_LEN			(24)
#define ADDR1_OFFSET	(4)
#define ADDR2_OFFSET	(10)
#define ACTION_TYPE		(13)
#define ACTION_SUBTYPE_OFFSET	(30)
#define PUB_ACTION		(0x4)
#define P2P_ACTION		(0x7f)

static void cfg80211_dump_frame_prot_info(struct wiphy *wiphy,
		int send, int freq,
		const unsigned char *buf, int len)
{
	char print_buf[1024];
	int buf_idx = 0;
	int type = ((*buf) & IEEE80211_FCTL_FTYPE) >> 2;
	int subtype = ((*buf) & IEEE80211_FCTL_STYPE) >> 4;
	int action;
	int action_subtype;

	buf_idx += sprintf(print_buf + buf_idx, "[cfg80211] ");

	if (send)
		buf_idx += sprintf(print_buf + buf_idx, "SEND: ");
	else
		buf_idx += sprintf(print_buf + buf_idx, "RECV: ");

	if(type == IEEE80211_FTYPE_MGMT){
		buf_idx += sprintf(print_buf + buf_idx, "%dMHz, %s, ",
			freq, type_name[subtype]);
	}else{
		buf_idx += sprintf(print_buf + buf_idx, "%dMHz, not mgmt frame, type=%d, ",
			freq, type);
	}

	if (subtype == ACTION_TYPE) {
		action = *(buf + MAC_LEN);
		action_subtype = *(buf + ACTION_SUBTYPE_OFFSET);
		if (action == PUB_ACTION)
			buf_idx += sprintf(print_buf + buf_idx, "PUB:%s ",
					pub_action_name[action_subtype]);
		else if (action == P2P_ACTION)
			buf_idx += sprintf(print_buf + buf_idx, "P2P:%s ",
					p2p_action_name[action_subtype]);
		else
			buf_idx += sprintf(print_buf + buf_idx,
					"Unknown ACTION(0x%x)",	action);
	}

	buf_idx += sprintf(print_buf + buf_idx, "%02x:%02x:%02x:%02x:%02x:%02x",
	       *(buf + 4), *(buf + 5), *(buf + 6),
	       *(buf + 7), *(buf + 8), *(buf + 9));
	buf_idx += sprintf(print_buf + buf_idx, "  ");
	buf_idx += sprintf(print_buf + buf_idx, "%02x:%02x:%02x:%02x:%02x:%02x",
	       *(buf + 10), *(buf + 11), *(buf + 12),
	       *(buf + 13), *(buf + 14), *(buf + 15));

	wiphy_info(wiphy, "%s\n", print_buf);
}

int itm_get_p2p_mode_from_file(void)
{
	struct file *fp = 0;
	mm_segment_t fs;
	int size = 0;
	loff_t pos = 0;
	u8 *buf;
	int ret = false;

	fp = filp_open(P2P_MODE_PATH, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		pr_err("open %s file error\n", P2P_MODE_PATH);
		goto end;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);

	size = get_file_size(fp);
	if (size <= 0) {
		pr_err("load file:%s error\n", P2P_MODE_PATH);
		goto error;
	}

	buf = kzalloc(size + 1, GFP_KERNEL);
	vfs_read(fp, buf, size, &pos);

	if (strcmp(buf, "p2p_mode") == 0)
		ret = true;

	kfree(buf);

error:
	filp_close(fp, NULL);
	set_fs(fs);
end:
	return ret;
}

#endif	/*CONFIG_ITM_WIFI_DIRECT */

static bool itm_wlan_cfg80211_ready(struct itm_priv *priv)
{
	return (priv->cp2_status == ITM_READY);
}

#define WLAN_EID_VENDOR_SPECIFIC 221
static bool itm_is_wps_ie(const u8 *pos)
{
	return (pos[0] == WLAN_EID_VENDOR_SPECIFIC &&
		pos[1] >= 4 &&
		pos[2] == 0x00 && pos[3] == 0x50 && pos[4] == 0xf2 &&
		pos[5] == 0x04);
}

static bool itm_find_wpsie(const u8 *ies, size_t ies_len,
			  u8 *buf, size_t *wps_len)
{
	const u8 *pos;
	size_t len = 0;
	bool flags = false;

	/*
	 * Filter out RSN/WPA IE(s)
	 */
	if (ies && ies_len) {
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;

			if (itm_is_wps_ie(pos)) {
				memcpy(buf + len, pos, 2 + pos[1]);
				len += 2 + pos[1];
				flags = true;
			}

			pos += 2 + pos[1];
		}
	}

	*wps_len = len;
	return flags;
}

#ifdef CONFIG_ITM_WIFI_DIRECT
static bool itm_find_p2p_ie(const u8 *ie, size_t ie_len, u8 *p2p_ie,
			    size_t *p2p_ie_len)
{
	bool flags = false;
	u16 index = 0;
/*Find out P2P IE.*/

	if (NULL == ie || ie_len <= 0 || NULL == p2p_ie)
		return flags;

	while (index < ie_len) {
		if (P2P_IE_ID == ie[index]) {
			*p2p_ie_len = ie[index + 1];
			if (ie_len >= *p2p_ie_len &&
			    P2P_IE_OUI_BYTE0 == ie[index + 2] &&
			    P2P_IE_OUI_BYTE1 == ie[index + 3] &&
			    P2P_IE_OUI_BYTE2 == ie[index + 4] &&
			    P2P_IE_OUI_TYPE == ie[index + 5]) {
				memcpy(p2p_ie, ie + index, *p2p_ie_len + 2);
				*p2p_ie_len += 2;
				return true;
			}
		}
		index++;
	}

	return false;
}
#endif				/*CONFIG_ITM_WIFI_DIRECT */

static int itm_wlan_add_cipher_key(struct itm_priv *priv, bool pairwise,
				   u8 key_index, u32 cipher, const u8 *key_seq,
				   const u8 *macaddr)
{
	u8 pn_key[16] = { 0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36,
			  0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36, 0x5c, 0x36};
	int ret;

	if (priv->key_len[pairwise][0] || priv->key_len[pairwise][1] ||
	    priv->key_len[pairwise][2] || priv->key_len[pairwise][3]) {
		/* Only set wep keys if we have at least one of them.
		   pairwise: 0:GTK 1:PTK */
		switch (cipher) {
		case WLAN_CIPHER_SUITE_WEP40:
			priv->cipher_type = WEP40;
			break;
		case WLAN_CIPHER_SUITE_WEP104:
			priv->cipher_type = WEP104;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			priv->cipher_type = TKIP;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			priv->cipher_type = CCMP;
			break;
		case WLAN_CIPHER_SUITE_SMS4:
			priv->cipher_type = WAPI;
			break;
		default:
			wiphy_err(priv->wdev->wiphy,
				  "%s invalid cipher: %d\n", __func__,
				  priv->cipher_type);
			return -EINVAL;
		}
		memcpy(priv->key_txrsc[pairwise], pn_key, sizeof(pn_key));
		ret = itm_wlan_add_key_cmd(priv->wlan_sipc,
					   priv->key[pairwise][key_index],
					   priv->key_len[pairwise][key_index],
					   pairwise, key_index,
					   key_seq, priv->cipher_type, macaddr);
		if (ret < 0) {
			wiphy_err(priv->wdev->wiphy,
				  "%s error %d\n", __func__, ret);
			return ret;
		}
	}
	return 0;
}

/*supposing wlan_sipc cfg80211 privite struct is itm_priv. it should
  be modified later*/
static int itm_wlan_cfg80211_scan(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
				  struct net_device *dev,
#endif
				  struct cfg80211_scan_request *request)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	struct wireless_dev *wdev = priv->wdev;
	struct cfg80211_ssid *ssids = request->ssids;
	struct wlan_sipc_scan_ssid *scan_ssids;
	int scan_ssids_len = 0;
	u8 *sipc_data = NULL;
	unsigned int i, n;
	int ret;
	unsigned long flags;
#ifdef CONFIG_ITM_WIFI_DIRECT
	struct wlan_sipc_scan_channels scan_channels;
	static u32 scan_channels_count = 0;
#endif	/* CONFIG_ITM_WIFI_DIRECT */

	wiphy_info(wiphy, "%s\n", __func__);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	spin_lock_irqsave(&priv->scan_lock, flags);
	if (priv->scan_request) {
		spin_unlock_irqrestore(&priv->scan_lock, flags);
		wiphy_err(wiphy, "Already scanning\n");
		return -EAGAIN;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);

	/* check we are client side */
	switch (wdev->iftype) {
	case NL80211_IFTYPE_AP:
		break;
	case NL80211_IFTYPE_STATION:
		break;
#ifdef CONFIG_ITM_WIFI_DIRECT
	case NL80211_IFTYPE_P2P_CLIENT:
	case NL80211_IFTYPE_P2P_GO:
		break;
#endif	/* CONFIG_ITM_WIFI_DIRECT */
	default:
		return -EOPNOTSUPP;
	}

	/* set wps ie */
	if (request->ie_len > 0) {
		if (request->ie_len > 255) {
			wiphy_err(wiphy,
				  "%s invalid len: %d\n", __func__,
				  request->ie_len);
			return -EOPNOTSUPP;
		}
		ret = itm_wlan_set_wps_ie_cmd(priv->wlan_sipc, WPS_REQ_IE,
					      request->ie, request->ie_len);
		if (ret) {
			wiphy_err(wiphy,
				  "%s failed to set wps ie!\n", __func__);
			return ret;
		}
	}

#ifdef CONFIG_ITM_WIFI_DIRECT
	if(priv->p2p_mode)
	{
		/* set scan channel */
		n = min(request->n_channels, 4U);
		for (i = 0; i < n; i++) {
			int ch = request->channels[i]->hw_value;
			if (ch == 0) {
				wiphy_err(wiphy,
					  "%s unknown frequency: %dMHz\n", __func__,
					request->channels[i]->center_freq);
				continue;
			}
		}

		if (request->n_channels > 4) {
			scan_channels.channel_num = 0;
		} else {
			scan_channels.channel_num = request->n_channels;
		}
		for (i = 0; i < scan_channels.channel_num; i++) {
			scan_channels.channel_freq[i] =
			    request->channels[i]->center_freq;
		}
		scan_channels_count++;
		scan_channels.random_count = scan_channels_count;
		wiphy_info(wiphy, "%s: channel_num=%d[%d, %d, %d, %d], random_count=0x%x\n", __func__,
			scan_channels.channel_num, scan_channels.channel_freq[0], scan_channels.channel_freq[1], 
			scan_channels.channel_freq[2], scan_channels.channel_freq[3], scan_channels.random_count);
		ret = itm_wlan_set_scan_channels_cmd(priv->wlan_sipc,
		   (u8 *)&scan_channels, sizeof(struct wlan_sipc_scan_channels));
		if (ret) {
			wiphy_err(wiphy, "%s failed to set scan channels!\n", __func__);
			return ret;
		}
	}
#endif

	n = min(request->n_ssids, 9);
	if (n) {
		sipc_data = kzalloc(512, GFP_ATOMIC);
		if (!sipc_data) {
			wiphy_err(wiphy,
				  "%s failed to alloc memory for combo ssid!\n",
				  __func__);
			return -2;
		}
		scan_ssids = (struct wlan_sipc_scan_ssid *)sipc_data;
		for (i = 0; i < n; i++) {
			if (!ssids[i].ssid_len)
				continue;
			scan_ssids->len = ssids[i].ssid_len;
			memcpy(scan_ssids->ssid, ssids[i].ssid,
			       ssids[i].ssid_len);
			scan_ssids_len += (ssids[i].ssid_len
					   + sizeof(scan_ssids->len));
			scan_ssids = (struct wlan_sipc_scan_ssid *)
			    (sipc_data + scan_ssids_len);
		}
	} else {
		wiphy_err(wiphy, "%s err param n_ssids is 0\n", __func__);
		return -EINVAL;
	}

	n = min(request->n_channels, 4U);
	for (i = 0; i < n; i++) {
		int ch = request->channels[i]->hw_value;
		if (ch == 0) {
			wiphy_err(wiphy,
				"%s unknown frequency: %dMHz\n", __func__,
				request->channels[i]->center_freq);
			continue;
		}
	}
	spin_lock_irqsave(&priv->scan_lock, flags);
	priv->scan_request = request;
	spin_unlock_irqrestore(&priv->scan_lock, flags);
	ret = itm_wlan_scan_cmd(priv->wlan_sipc, sipc_data, scan_ssids_len);
	kfree(sipc_data);
	if (ret) {
		wiphy_err(wiphy, "%s error %d\n", __func__, ret);
		spin_lock_irqsave(&priv->scan_lock, flags);
		if (priv->scan_request) {
			cfg80211_scan_done(priv->scan_request, true);
			priv->scan_request = NULL;
		}
		spin_unlock_irqrestore(&priv->scan_lock, flags);
		return ret;
	}

	/* Arm scan timeout timer */
	mod_timer(&priv->scan_timeout,
		  jiffies + ITM_SCAN_TIMER_INTERVAL_MS * HZ / 1000);

	return 0;
}

static int itm_wlan_cfg80211_connect(struct wiphy *wiphy,
				     struct net_device *ndev,
				     struct cfg80211_connect_params *sme)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	struct wireless_dev *wdev = priv->wdev;
	int ret;
	u32 cipher = 0;
	u8 key_mgmt = 0;
	int is_wep = (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40) ||
	    (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104);
	int is_wapi = false;
	int auth_type = 0;
	u8 *buf = NULL;
	size_t wps_len = 0;
#ifdef CONFIG_ITM_WIFI_DIRECT
	size_t p2p_len = 0;
#endif	/* CONFIG_ITM_WIFI_DIRECT */

	wiphy_info(wiphy, "%s\n", __func__);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	/* To avoid confused wapi frame */
	priv->cipher_type = NONE;
	/* Get request status, type, bss, ie and so on */
	/* Set appending ie */
	/* Set wps ie */
	if (sme->ie_len > 0) {
		if (sme->ie_len > 255) {
			wiphy_err(wiphy,
				  "%s invalid len: %d\n", __func__,
				  sme->ie_len);
			return -EOPNOTSUPP;
		}
		buf = kmalloc(sme->ie_len, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;

		if (itm_find_wpsie(sme->ie, sme->ie_len, buf, &wps_len) == true) {
			ret = itm_wlan_set_wps_ie_cmd(priv->wlan_sipc,
						      WPS_ASSOC_IE,
						      buf, wps_len);
			if (ret) {
				wiphy_err(wiphy,
					  "%s failed to set wpas ie!\n",
					  __func__);
				kfree(buf);
				return ret;
			}
		}
#ifdef CONFIG_ITM_WIFI_DIRECT
		if (itm_find_p2p_ie(sme->ie, sme->ie_len,
				    buf, &p2p_len) == true) {
			ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
						      P2P_ASSOC_IE,
						      buf, p2p_len);
			if (ret) {
				dev_err(&priv->ndev->dev,
					"itm_wlan_set_p2p_ie failed with ret %d\n",
					ret);
				kfree(buf);
				return ret;
			}
		}
#endif				/*CONFIG_ITM_WIFI_DIRECT */

		kfree(buf);
	}

	/* Set WPA version */
	wiphy_info(wiphy, "%s wpa_versions %#x\n", __func__,
		   sme->crypto.wpa_versions);
	ret =
	    itm_wlan_set_wpa_version_cmd(priv->wlan_sipc,
					 sme->crypto.wpa_versions);
	if (ret < 0) {
		wiphy_err(wiphy, "%s failed to set wpa version!\n", __func__);
		return ret;
	}

	/* Set Auth type */
	wiphy_info(wiphy, "%s auth_type %#x\n", __func__, sme->auth_type);
	/* Set the authorisation */
	if ((sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM) ||
	    ((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && !is_wep))
		auth_type = ITM_AUTH_OPEN;
	else if ((sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY) ||
		 ((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && is_wep))
		auth_type = ITM_AUTH_SHARED;
	ret = itm_wlan_set_auth_type_cmd(priv->wlan_sipc, auth_type);
	if (ret < 0) {
		wiphy_err(wiphy, "%s failed to set auth type!\n", __func__);
		return ret;
	}
	/* Set cipher - pairewise and group */
	if (sme->crypto.n_ciphers_pairwise) {
		switch (sme->crypto.ciphers_pairwise[0]) {
		case WLAN_CIPHER_SUITE_WEP40:
			cipher = WEP40;
			break;
		case WLAN_CIPHER_SUITE_WEP104:
			cipher = WEP104;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			cipher = TKIP;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			cipher = CCMP;
			break;
			/* WAPI cipher is not processed by CP2 */
		case WLAN_CIPHER_SUITE_SMS4:
			cipher = WAPI;
			is_wapi = true;
			break;
		default:
			wiphy_err(wiphy,
				  "%s unsupported pairwise cipher suite: 0x%x\n",
				  __func__, sme->crypto.ciphers_pairwise[0]);
			return -ENOTSUPP;
		}

		if (is_wapi != true) {
			wiphy_info(wiphy, "%s cipher_pairwise %#x\n", __func__,
				   sme->crypto.ciphers_pairwise[0]);
			ret =
			    itm_wlan_set_cipher_cmd(priv->wlan_sipc, cipher,
						    WIFI_CMD_PAIRWISE_CIPHER);
			if (ret < 0) {
				wiphy_err(wiphy,
					  "%s failed to set pairwise cipher!\n",
					  __func__);
				return ret;
			}
		}
	} else {
		/*No pairewise cipher */
		wiphy_dbg(wiphy, "No pairewise cipher\n");
	}

	/* Set group cipher */
	switch (sme->crypto.cipher_group) {
	case NONE:
		cipher = NONE;
		break;
	case WLAN_CIPHER_SUITE_WEP40:
		cipher = WEP40;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		cipher = WEP104;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		cipher = TKIP;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		cipher = CCMP;
		break;
	default:
		wiphy_err(wiphy,
			  "%s unsupported group cipher suite: 0x%x\n", __func__,
			  sme->crypto.cipher_group);
		return -ENOTSUPP;
	}

	if (is_wapi != true) {
		wiphy_info(wiphy, "%s cipher_group %#x\n", __func__,
			   sme->crypto.cipher_group);
		ret =
		    itm_wlan_set_cipher_cmd(priv->wlan_sipc, cipher,
					    WIFI_CMD_GROUP_CIPHER);
		if (ret < 0) {
			wiphy_err(wiphy,
				  "%s failed to set group cipher!\n", __func__);
			return ret;
		}
	}

	/* FIXME */
	/* Set Auth type again because of CP2 process's differece */
	wiphy_info(wiphy, "%s auth_type %#x again\n", __func__, sme->auth_type);
	/* Set the authorisation */
	if ((sme->auth_type == NL80211_AUTHTYPE_OPEN_SYSTEM) ||
	    ((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && !is_wep))
		auth_type = ITM_AUTH_OPEN;
	else if ((sme->auth_type == NL80211_AUTHTYPE_SHARED_KEY) ||
		 ((sme->auth_type == NL80211_AUTHTYPE_AUTOMATIC) && is_wep))
		auth_type = ITM_AUTH_SHARED;
	ret = itm_wlan_set_auth_type_cmd(priv->wlan_sipc, auth_type);
	if (ret < 0) {
		wiphy_err(wiphy, "%s failed to set auth type!\n", __func__);
		return ret;
	}

	/* Set auth key management (akm) */
	if (sme->crypto.n_akm_suites) {
		if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_PSK)
			key_mgmt = AKM_SUITE_PSK;
		else if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_8021X)
			key_mgmt = AKM_SUITE_8021X;
		/* WAPI akm is not processed by CP2 */
		else if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_WAPI_CERT)
			key_mgmt = AKM_SUITE_WAPI;
		else if (sme->crypto.akm_suites[0] == WLAN_AKM_SUITE_WAPI_PSK)
			key_mgmt = AKM_SUITE_WAPI;

		wiphy_info(wiphy, "%s akm_suites %#x\n", __func__,
			   sme->crypto.akm_suites[0]);
		ret =
		    itm_wlan_set_key_management_cmd(priv->wlan_sipc, key_mgmt);
		if (ret < 0) {
			wiphy_err(wiphy,
				  "%s failed to set key management!\n",
				  __func__);
			return ret;
		}
	}

	/* Set PSK */
	if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40 ||
	    sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104 ||
	    sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP40 ||
	    sme->crypto.ciphers_pairwise[0] == WLAN_CIPHER_SUITE_WEP104) {
		priv->key_index[GROUP] = sme->key_idx;
		priv->key_len[GROUP][sme->key_idx] = sme->key_len;
		memcpy(priv->key[GROUP][sme->key_idx], sme->key, sme->key_len);
		ret = itm_wlan_add_cipher_key(priv, 0, sme->key_idx,
					      sme->crypto.ciphers_pairwise[0],
					      NULL, NULL);
		if (ret < 0) {
			wiphy_err(wiphy,
				  "%s failed to add cipher key!\n", __func__);
			return ret;
		}
	} else {
		u8 psk[32];
		int key_len = 0;
		if (wdev->iftype == NL80211_IFTYPE_AP) {
			ret = hostap_conf_load(HOSTAP_CONF_FILE_NAME, psk);
			if (ret) {
				wiphy_err(wiphy,
					  "%s failed to load hostap conf!\n",
					  __func__);
				return ret;
			}
			key_len = sizeof(psk);
		} else {
			if (sme->key_len > 32) {
				wiphy_err(wiphy,
					  "%s invalid key len: %d\n", __func__,
					  sme->key_len);
				return -EINVAL;
			}
			memcpy(psk, sme->key, sme->key_len);
			key_len = sme->key_len;
		}
		wiphy_info(wiphy, "%s psk %s\n", __func__, sme->key);
		ret = itm_wlan_set_psk_cmd(priv->wlan_sipc, psk, key_len);
		if (ret < 0) {
			wiphy_err(wiphy, "%s failed to set psk!\n", __func__);
			return ret;
		}
	}

	/* Auth RX unencrypted EAPOL is not implemented, do nothing */
	/* Set channel */
	if (sme->channel != NULL) {
		wiphy_info(wiphy, "%s channel %d\n", __func__,
			   ieee80211_frequency_to_channel(sme->
							  channel->center_freq));
		ret =
		    itm_wlan_set_channel_cmd(priv->wlan_sipc,
					     ieee80211_frequency_to_channel
					     (sme->channel->center_freq));
		if (ret < 0) {
			wiphy_err(wiphy,
				  "%s failed to set channel!\n", __func__);
			return ret;
		}
	} else {
		wiphy_dbg(wiphy, "Channel is not specified\n");
	}

	/* Set BSSID */
	if (sme->bssid != NULL) {
		ret = itm_wlan_set_bssid_cmd(priv->wlan_sipc, sme->bssid);
		if (ret < 0) {
			wiphy_err(wiphy, "%s failed to set bssid!\n", __func__);
			return ret;
		}
		memcpy(priv->bssid, sme->bssid, 6);
	} else {
		wiphy_dbg(wiphy, "BSSID is not specified\n");
	}

	/* Special process for WEP(WEP key must be set before itm_set_essid) */
	if (sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP40 ||
	    sme->crypto.cipher_group == WLAN_CIPHER_SUITE_WEP104) {
		wiphy_info(wiphy, "%s WEP cipher_group\n", __func__);

		if (sme->key_len <= 0) {
			wiphy_dbg(wiphy, "No key is specified\n");
		} else {
			if (sme->key_len != WLAN_KEY_LEN_WEP104 &&
			    sme->key_len != WLAN_KEY_LEN_WEP40) {
				wiphy_err(wiphy,
					  "%s invalid WEP key length\n",
					  __func__);
				return -EINVAL;
			}

			itm_wlan_set_key_cmd(priv->wlan_sipc, sme->key_idx);
		}
	}

	/* Set ESSID */
	if (sme->ssid != NULL) {
		wiphy_info(wiphy, "%s essid %s\n", __func__, sme->ssid);
		ret = itm_wlan_set_essid_cmd(priv->wlan_sipc, sme->ssid,
					     (int)sme->ssid_len);
		if (ret < 0) {
			wiphy_err(wiphy, "%s failed to set essid!\n", __func__);
			return ret;
		}
		memcpy(priv->ssid, sme->ssid, sme->ssid_len);
		priv->ssid_len = sme->ssid_len;
	}

	priv->connect_status = ITM_CONNECTING;
	return ret;
}

static int itm_wlan_cfg80211_disconnect(struct wiphy *wiphy,
					struct net_device *ndev,
					u16 reason_code)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s %s\n", __func__, priv->ssid);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	ret = itm_wlan_disconnect_cmd(priv->wlan_sipc, reason_code);
	if (ret < 0) {
		wiphy_err(wiphy, "%s failed disconnect!\n", __func__);
	}
	memset(priv->ssid, 0, sizeof(priv->ssid));

	return ret;
}

static int itm_wlan_cfg80211_add_key(struct wiphy *wiphy,
				     struct net_device *netdev, u8 idx,
				     bool pairwise, const u8 *mac_addr,
				     struct key_params *params)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s with cipher %d\n", __func__, params->cipher);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

#ifdef CONFIG_ITM_WIFI_DIRECT
	if ((priv->mode == ITM_AP_MODE) && (priv->p2p_mode == false)) {
#else	/* CONFIG_ITM_WIFI_DIRECT */
	if (priv->mode == ITM_AP_MODE) {
#endif	/* CONFIG_ITM_WIFI_DIRECT */
		u8 key[32];
		memset(key, 0, sizeof(key));
		ret = hostap_conf_load(HOSTAP_CONF_FILE_NAME, key);
		if (ret != 0) {
			wiphy_err(wiphy, "%s failed to load hostapd conf!\n",
				  __func__);
			return ret;
		}
		ret = itm_wlan_set_psk_cmd(priv->wlan_sipc, key, sizeof(key));
		if (ret < 0) {
			wiphy_err(wiphy, "%s failed to set psk!\n", __func__);
			return ret;
		}
#ifdef CONFIG_ITM_WIFI_DIRECT
	} else if ((priv->mode == ITM_STATION_MODE) ||
		   priv->mode == ITM_P2P_CLIENT_MODE ||
		   priv->mode == ITM_P2P_GO_MODE) {
#else	/* CONFIG_ITM_WIFI_DIRECT */
	} else if (priv->mode == ITM_STATION_MODE) {
#endif	/* CONFIG_ITM_WIFI_DIRECT */
		priv->key_index[pairwise] = idx;
		priv->key_len[pairwise][idx] = params->key_len;
		memcpy(priv->key[pairwise][idx], params->key, params->key_len);
		ret = itm_wlan_add_cipher_key(priv, pairwise, idx,
					      params->cipher,
					      params->seq, mac_addr);
		if (ret < 0) {
			wiphy_err(wiphy,
				  "%s failed to add cipher key!\n", __func__);
			return ret;
		}
	}

	return 0;
}

static int itm_wlan_cfg80211_del_key(struct wiphy *wiphy,
				     struct net_device *ndev,
				     u8 key_index, bool pairwise,
				     const u8 *mac_addr)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	wiphy_info(wiphy, "%s key_index=%d, pairwise=%d\n", __func__, key_index, pairwise);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	if (key_index > WLAN_MAX_KEY_INDEX) {
		wiphy_err(wiphy, "%s key index %d out of bounds\n", __func__,
			  key_index);
		return -ENOENT;
	}

	if (!priv->key_len[pairwise][key_index]) {
		wiphy_err(wiphy, "%s key index %d is empty\n", __func__,
			  key_index);
		return 0;
	}

	priv->key_len[pairwise][key_index] = 0;
	priv->cipher_type = NONE;

	return itm_wlan_del_key_cmd(priv->wlan_sipc, key_index, mac_addr);
}

static int itm_wlan_cfg80211_set_default_key(struct wiphy *wiphy,
					     struct net_device *ndev,
					     u8 key_index, bool unicast,
					     bool multicast)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	if (key_index > 3) {
		wiphy_err(wiphy, "%s invalid key index %d\n", __func__,
			  key_index);
		return -EINVAL;
	}

	ret = itm_wlan_set_key_cmd(priv->wlan_sipc, key_index);
	if (ret < 0) {
		wiphy_err(wiphy, "%s failed to set key\n", __func__);
		return ret;
	}

	return 0;
}

static int itm_wlan_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	if (changed & WIPHY_PARAM_RTS_THRESHOLD) {
		ret = itm_wlan_set_rts_cmd(priv->wlan_sipc,
					   wiphy->rts_threshold);
		if (ret != 0) {
			wiphy_err(wiphy, "%s failed to set rts\n", __func__);
			return -EIO;
		}
	}

	if (changed & WIPHY_PARAM_FRAG_THRESHOLD) {
		ret =
		    itm_wlan_set_frag_cmd(priv->wlan_sipc,
					  wiphy->frag_threshold);
		if (ret != 0) {
			wiphy_err(wiphy, "%s failed to set frag\n", __func__);
			return -EIO;
		}
	}
	return 0;
}

static int itm_wlan_cfg80211_get_station(struct wiphy *wiphy,
					 struct net_device *dev, u8 *mac,
					 struct station_info *sinfo)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	s8 signal, noise;
	s32 rate;
	int ret;
	size_t i;

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	sinfo->filled |= STATION_INFO_TX_BYTES |
			 STATION_INFO_TX_PACKETS |
			 STATION_INFO_RX_BYTES |
			 STATION_INFO_RX_PACKETS;
	sinfo->tx_bytes = priv->ndev->stats.tx_bytes;
	sinfo->tx_packets = priv->ndev->stats.tx_packets;
	sinfo->rx_bytes = priv->ndev->stats.rx_bytes;
	sinfo->rx_packets = priv->ndev->stats.rx_packets;

	/* Get current RSSI */
	ret = itm_wlan_get_rssi_cmd(priv->wlan_sipc, &signal, &noise);
	if (ret == 0) {
		sinfo->signal = signal;
		sinfo->filled |= STATION_INFO_SIGNAL;
	} else {
		wiphy_err(wiphy, "%s failed to get rssi!\n", __func__);
		return -EIO;
	}

	ret = itm_wlan_get_txrate_cmd(priv->wlan_sipc, &rate);
	if (ret == 0) {
		sinfo->filled |= STATION_INFO_TX_BITRATE;
	} else {
		wiphy_err(wiphy, "%s failed to get txrate!\n", __func__);
		return -EIO;
	}

	/* Convert got rate from hw_value to NL80211 value */
	if (!(rate & 0x7f)) {
		wiphy_info(wiphy, "%s rate %d\n", __func__, (rate & 0x7f));
		sinfo->txrate.legacy = 10;
	} else {
		for (i = 0; i < ARRAY_SIZE(itm_rates); i++) {
			if (rate == itm_rates[i].hw_value) {
				sinfo->txrate.legacy = itm_rates[i].bitrate;
				if (rate & 0x80)
					sinfo->txrate.mcs =
					    itm_rates[i].hw_value;
				break;
			}
		}

		if (i >= ARRAY_SIZE(itm_rates))
			sinfo->txrate.legacy = 10;
	}

	wiphy_info(wiphy, "%s signal %d txrate %d\n", __func__, sinfo->signal,
		   sinfo->txrate.legacy);

	return 0;
}

static int itm_wlan_cfg80211_set_pmksa(struct wiphy *wiphy,
				       struct net_device *netdev,
				       struct cfg80211_pmksa *pmksa)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s\n", __func__);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	ret = itm_wlan_pmksa_cmd(priv->wlan_sipc, pmksa->bssid,
				 pmksa->pmkid, CMD_TYPE_SET);

	return ret;
}

static int itm_wlan_cfg80211_del_pmksa(struct wiphy *wiphy,
				       struct net_device *netdev,
				       struct cfg80211_pmksa *pmksa)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s\n", __func__);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	ret = itm_wlan_pmksa_cmd(priv->wlan_sipc, pmksa->bssid,
				 pmksa->pmkid, CMD_TYPE_DEL);

	return ret;
}

static int itm_wlan_cfg80211_flush_pmksa(struct wiphy *wiphy,
					 struct net_device *netdev)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s\n", __func__);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	ret = itm_wlan_pmksa_cmd(priv->wlan_sipc, priv->bssid,
				 NULL, CMD_TYPE_FLUSH);

	return ret;
}

void itm_cfg80211_report_connect_result(struct itm_priv *priv)
{
	u8 *req_ie_ptr, *resp_ie_ptr, *bssid_ptr, *pos, *value_ptr;
	u8 status_code = 0;
	u16 bssid_len, status_len;
	u8 req_ie_len, resp_ie_len;
	u32 event_len;
	int left;
	unsigned long flags;

	event_len = priv->wlan_sipc->wlan_sipc_event_len;
	/* status_len 2 + status_code 1 = 3 bytes */
	if (event_len < 3) {
		wiphy_err(priv->wdev->wiphy,
			  "%s invalid event len %d!\n", __func__, event_len);
		goto out;
	}
	pos = kmalloc(event_len, GFP_ATOMIC);
	if (pos == NULL) {
		goto out;
	}

	/* The first byte of event data is status and len */
	memcpy(pos, priv->wlan_sipc->event_buf->u.event.variable, event_len);
	memcpy(&status_len, pos, 2);
	if (status_len != 1) {
		wiphy_err(priv->wdev->wiphy,
			  "%s erro status len(%d)\n", __func__, status_len);
		goto freepos;
	}
	memcpy(&status_code, pos + 2, status_len);
	/* FIXME later the status code should be reported by CP2 */
	if (status_code != 0) {
		wiphy_err(priv->wdev->wiphy,
			  "%s failled to connect(%d)\n", __func__, status_code);
		goto freepos;
	}

	value_ptr = pos + 2 + status_len;
	left = event_len - 2 - status_len;
	/* BSSID is 6 + len is 2 = 8 */
	if (left < 8) {
		wiphy_err(priv->wdev->wiphy, "%s invaild bssid!\n", __func__);
		goto freepos;
	}
	memcpy(&bssid_len, value_ptr, 2);
	left -= 2;
	bssid_ptr = value_ptr + 2;
	left -= bssid_len;

	if (!left) {
		wiphy_err(priv->wdev->wiphy, "%s no req_ie frame!\n", __func__);
		goto freepos;
	}
	req_ie_len = *(u8 *) (bssid_ptr + bssid_len);
	left -= 1;
	req_ie_ptr = bssid_ptr + bssid_len + 1;
	left -= req_ie_len;
	if (!left) {
		wiphy_err(priv->wdev->wiphy, "%s no resp_ie frame!\n",
			  __func__);
		goto freepos;
	}
	resp_ie_len = *(u8 *) (req_ie_ptr + req_ie_len);
	resp_ie_ptr = req_ie_ptr + req_ie_len + 1;

	if (priv->connect_status == ITM_CONNECTING) {
		/* inform connect result to cfg80211 */
		priv->connect_status = ITM_CONNECTED;
		cfg80211_connect_result(priv->ndev,
					bssid_ptr, req_ie_ptr, req_ie_len,
					resp_ie_ptr, resp_ie_len,
					WLAN_STATUS_SUCCESS, GFP_KERNEL);
		wiphy_info(priv->wdev->wiphy, "%s %s success!\n", __func__,
			   priv->ssid);

		if (!netif_carrier_ok(priv->ndev)) {
			wiphy_dbg(priv->wdev->wiphy,
				  "%s netif_carrier_on, ssid:%s\n", __func__,
				  priv->ssid);
			netif_carrier_on(priv->ndev);
			netif_wake_queue(priv->ndev);
		}
	} else {
		wiphy_err(priv->wdev->wiphy,
			  "%s wrong previous connect status!\n", __func__);
		goto freepos;
	}

	kfree(pos);
	return;

freepos:
	kfree(pos);
out:
	if (timer_pending(&priv->scan_timeout))
		del_timer_sync(&priv->scan_timeout);
	spin_lock_irqsave(&priv->scan_lock, flags);
	if (priv->scan_request) {
		cfg80211_scan_done(priv->scan_request, true);
		priv->scan_request = NULL;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);
	if (priv->connect_status == ITM_CONNECTING) {
		cfg80211_connect_result(priv->ndev,
					priv->bssid, NULL, 0,
					NULL, 0,
					WLAN_STATUS_UNSPECIFIED_FAILURE,
					GFP_KERNEL);
		wiphy_info(priv->wdev->wiphy, "%s %s fail!\n", __func__,
			   priv->ssid);
	} else if (priv->connect_status == ITM_CONNECTED) {
		cfg80211_disconnected(priv->ndev, status_code,
				      NULL, 0, GFP_KERNEL);
	}
	return;
}

void itm_cfg80211_report_disconnect_done(struct itm_priv *priv)
{
	u16 reason_code = 0;
	unsigned long flags;

	/* This should filled if disconnect reason is not only one */
	memcpy(&reason_code, priv->wlan_sipc->event_buf->u.event.variable, 2);
	if (timer_pending(&priv->scan_timeout))
		del_timer_sync(&priv->scan_timeout);
	spin_lock_irqsave(&priv->scan_lock, flags);
	if (priv->scan_request) {
		cfg80211_scan_done(priv->scan_request, true);
		priv->scan_request = NULL;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);
	if (priv->connect_status == ITM_CONNECTING) {
		cfg80211_connect_result(priv->ndev,
					priv->bssid, NULL, 0,
					NULL, 0,
					WLAN_STATUS_UNSPECIFIED_FAILURE,
					GFP_KERNEL);
	} else if (priv->connect_status == ITM_CONNECTED) {
		cfg80211_disconnected(priv->ndev, reason_code,
				      NULL, 0, GFP_KERNEL);
		wiphy_info(priv->wdev->wiphy, "%s %s\n", __func__, priv->ssid);
	}

	priv->connect_status = ITM_DISCONNECTED;
	if (netif_carrier_ok(priv->ndev)) {
		wiphy_dbg(priv->wdev->wiphy, "%s netif_carrier_off\n",
			  __func__);
		netif_carrier_off(priv->ndev);
		netif_stop_queue(priv->ndev);
	}
	return;
}

static void itm_cfg80211_scan_timeout(unsigned long data)
{
	struct itm_priv *priv = (struct itm_priv *)data;
	unsigned long flags;

	spin_lock_irqsave(&priv->scan_lock, flags);
	if (priv->scan_request) {
		wiphy_err(priv->wdev->wiphy, "%s\n", __func__);
		cfg80211_scan_done(priv->scan_request, true);
		priv->scan_request = NULL;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);

	return;
}

void itm_cfg80211_report_scan_done(struct itm_priv *priv, bool aborted)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	struct ieee80211_supported_band *band;
	struct ieee80211_channel *channel;
	struct ieee80211_mgmt *mgmt = NULL;
	struct cfg80211_bss *bss = NULL;
	u32 i = 0, left = priv->wlan_sipc->wlan_sipc_event_len;
	u8 *pos = priv->wlan_sipc->event_buf->u.event.variable;
	u16 channel_num, channel_len;
	u32 freq;
	s16 rssi, rssi_len;
	u32 mgmt_len = 0;
	u64 tsf;
	u16 capability, beacon_interval;
	u8 *ie;
	size_t ielen;
	s32 signal;
	unsigned long flags;

	spin_lock_irqsave(&priv->scan_lock, flags);
	if (!priv->scan_request) {
		spin_unlock_irqrestore(&priv->scan_lock, flags);
		wiphy_err(wiphy, "%s null scan_request!\n", __func__);
		return;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);

	if (left < 10 || aborted) {
		wiphy_err(wiphy, "%s invalid event len %d!\n", __func__, left);
		aborted = true;
		goto out;
	}

	while (left >= 10) {
		/* must use memcpy to protect unaligned */
		/* The formate of frame is len(two bytes) + data */
		memcpy(&channel_len, pos, 2);
		pos += 2;
		left -= 2;
		memcpy(&channel_num, pos, channel_len);
		pos += channel_len;
		left -= channel_len;
		/* FIXME only support 2GHZ */
		band = wiphy->bands[IEEE80211_BAND_2GHZ];
		freq = ieee80211_channel_to_frequency(channel_num, band->band);
		channel = ieee80211_get_channel(wiphy, freq);
		if (!channel) {
			wiphy_err(wiphy, "%s invalid freq\n", __func__);
			continue;
		}

		/* The second two value of frame is rssi */
		memcpy(&rssi_len, pos, 2);
		pos += 2;
		left -= 2;
		memcpy(&rssi, pos, rssi_len);
		pos += rssi_len;
		left -= rssi_len;
		signal = rssi * 100;

		/* The third two value of frame is following data len */
		memcpy(&mgmt_len, pos, 2);
		pos += 2;
		left -= 2;
		if (mgmt_len > left) {
			wiphy_err(wiphy,
				  "%s mgmt_len(0x%08x) > left(0x%08x)!\n",
				  __func__, mgmt_len, left);
			aborted = true;
			goto out;
		}
		/* The following is real data */
		mgmt = (struct ieee80211_mgmt *)pos;
		pos += mgmt_len;
		left -= mgmt_len;
		ie = mgmt->u.probe_resp.variable;
		ielen = mgmt_len - offsetof(struct ieee80211_mgmt,
					    u.probe_resp.variable);
		tsf = le64_to_cpu(mgmt->u.probe_resp.timestamp);
		beacon_interval = le16_to_cpu(mgmt->u.probe_resp.beacon_int);
		capability = le16_to_cpu(mgmt->u.probe_resp.capab_info);
		wiphy_dbg(wiphy,
			  "%s %s, %02x:%02x:%02x:%02x:%02x:%02x, tsf %llu\n",
			  __func__, ieee80211_is_probe_resp(mgmt->frame_control)
			  ? "proberesp" : "beacon   ",
			  mgmt->bssid[0], mgmt->bssid[1], mgmt->bssid[2],
			  mgmt->bssid[3], mgmt->bssid[4], mgmt->bssid[5], tsf);

		bss = cfg80211_inform_bss(wiphy, channel, mgmt->bssid,
					  tsf, capability, beacon_interval, ie,
					  ielen, signal, GFP_KERNEL);

		if (unlikely(!bss))
			wiphy_err(wiphy,
				  "%s failed to inform bss frame!\n", __func__);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		cfg80211_put_bss(wiphy, bss);
#else
		cfg80211_put_bss(bss);
#endif
		i++;
	}

	if (left) {
		wiphy_err(wiphy, "%s wrong event len %d!\n", __func__, left);
		aborted = true;
		goto out;
	}
	wiphy_info(wiphy, "%s got %d BSSes\n", __func__, i);

out:
	if (timer_pending(&priv->scan_timeout))
		del_timer_sync(&priv->scan_timeout);
	spin_lock_irqsave(&priv->scan_lock, flags);
	if (priv->scan_request) {
		cfg80211_scan_done(priv->scan_request, aborted);
		priv->scan_request = NULL;
	}
	spin_unlock_irqrestore(&priv->scan_lock, flags);

	return;
}

void itm_cfg80211_report_ready(struct itm_priv *priv)
{
	priv->cp2_status = ITM_READY;
}

void itm_cfg80211_report_tx_busy(struct itm_priv *priv)
{
	u8 busy_flag = 0;

	/* busy_flag is 1 mean stop tx otherwise wake tx */
	memcpy(&busy_flag, priv->wlan_sipc->event_buf->u.event.variable, 1);
	if (busy_flag) {
		atomic_set(&priv->stopped, 1);
		netif_stop_queue(priv->ndev);
		wiphy_dbg(priv->wdev->wiphy, "tx busy event, stop queue\n");
	} else {
		atomic_set(&priv->stopped, 0);
		netif_wake_queue(priv->ndev);
		wiphy_dbg(priv->wdev->wiphy, "tx ok event, wake up queue\n");
	};

	return;
}

void itm_cfg80211_report_softap(struct itm_priv *priv)
{
	struct wlan_softap_event *event =
	    (struct wlan_softap_event *)priv->wlan_sipc->event_buf->u.event.
	    variable;
	struct station_info sinfo;
	memset(&sinfo, 0, sizeof(sinfo));
#ifdef CONFIG_ITM_WIFI_DIRECT
	if (event->req_ie_len > 0) {
		sinfo.assoc_req_ies = event->ie;
		sinfo.assoc_req_ies_len = event->req_ie_len;
		sinfo.filled |= STATION_INFO_ASSOC_REQ_IES;
	}
#endif	/* CONFIG_ITM_WIFI_DIRECT */

	if (event->connected) {
		cfg80211_new_sta(priv->ndev, (u8 const *)&event->mac, &sinfo,
				 GFP_KERNEL);
		wiphy_dbg(priv->wdev->wiphy, "hotspot station is connected\n");
	} else {
		cfg80211_del_sta(priv->ndev, (u8 const *)&event->mac,
				 GFP_ATOMIC);
		wiphy_dbg(priv->wdev->wiphy,
			  "hotspot station is disconnected\n");
	}
}

static int itm_wlan_cfg80211_mgmt_tx(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
				     struct wireless_dev *wdev,
#else
				     struct net_device *ndev,
#endif
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 38)) || \
	defined(COMPAT_KERNEL_RELEASE))
				     struct ieee80211_channel *chan,
				     bool offchan,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
				     enum nl80211_channel_type channel_type,
				     bool channel_type_valid,
#endif
				     unsigned int wait,
#else	/*(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)) */
				     struct ieee80211_channel *chan,
				     enum nl80211_channel_type channel_type,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || \
	defined(COMPAT_KERNEL_RELEASE)
				     bool channel_type_valid,
#endif
#endif	/*(LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,38)) */
				     const u8 *buf, size_t len,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
				     bool no_cck,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 3, 0))
				     bool dont_wait_for_ack,
#endif
				     u64 *cookie)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
#ifdef CONFIG_ITM_WIFI_DIRECT
	int ret = 0;
#endif	/* CONFIG_ITM_WIFI_DIRECT */

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}
#ifdef CONFIG_ITM_WIFI_DIRECT
	cfg80211_dump_frame_prot_info(wiphy, 1, chan->center_freq, buf, len);

	/* send tx mgmt */
	if (len > 0) {
		ret = itm_wlan_set_tx_mgmt_cmd(priv->wlan_sipc, chan,
					       wait, buf, len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_tx_mgmt_cmd failed with ret %d\n",
				ret);
			return ret;
		}
	}

	cfg80211_mgmt_tx_status(priv->wdev, *cookie, buf, len, 1, GFP_KERNEL);

#endif				/*CONFIG_ITM_WIFI_DIRECT */

	return 0;
}

#ifdef CONFIG_ITM_WIFI_DIRECT
static void register_frame_work_fun(struct work_struct *work)
{
	struct itm_priv *priv =
	    container_of(work, struct itm_priv, work);
	struct wlan_sipc *wlan_sipc = priv->wlan_sipc;
	struct wlan_sipc_data *send_buf = wlan_sipc->send_buf;
	struct wlan_sipc_register_frame *data =
	    (struct wlan_sipc_register_frame *)send_buf->u.cmd.variable;
	int ret = 0;

	mutex_lock(&wlan_sipc->cmd_lock);

	data->type = priv->frame_type;
	data->reg = priv->reg ? 1 : 0;
	priv->frame_type = 0xffff;
	priv->reg = 0;

	wlan_sipc->wlan_sipc_send_len =
	    ITM_WLAN_CMD_HDR_SIZE + sizeof(struct wlan_sipc_register_frame);
	wlan_sipc->wlan_sipc_recv_len = ITM_WLAN_CMD_RESP_HDR_SIZE;

	ret =
	    itm_wlan_cmd_send_recv(wlan_sipc, CMD_TYPE_SET,
				   WIFI_CMD_REGISTER_FRAME);

	if (ret)
		pr_err("return wrong status code is %d\n", ret);

	mutex_unlock(&wlan_sipc->cmd_lock);
}

void init_register_frame_param(struct itm_priv *priv)
{
	if (priv == NULL)
		return;

	priv->frame_type = 0xffff;
	priv->reg = 0;
	INIT_WORK(&priv->work, register_frame_work_fun);
}

static int itm_mac_register_frame(struct itm_priv *priv, u16 frame_type,
				  bool reg)
{
	priv->frame_type = frame_type;
	priv->reg = reg;
	schedule_work(&priv->work);
	return 0;
}

#endif

static void itm_wlan_cfg80211_mgmt_frame_register(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
						  struct wireless_dev *wdev,
#else
						  struct net_device *ndev,
#endif
						  u16 frame_type, bool reg)
{
#ifdef CONFIG_ITM_WIFI_DIRECT
	struct itm_priv *priv = *(struct itm_priv **)wiphy_priv(wiphy);

	if(priv->p2p_mode)
		itm_mac_register_frame(priv, frame_type, reg);
#endif
}
static int itm_wlan_change_beacon(struct itm_priv *priv,
				  struct cfg80211_beacon_data *beacon)
{
	int ret = 0;

#ifdef CONFIG_ITM_WIFI_DIRECT
	u16 ie_len;
	u8 *ie_ptr;

	if(priv->p2p_mode == false)
	{
		return ret;
	}
	/* send beacon extra ies */
	if (beacon->head != NULL) {
		ie_len = beacon->head_len;

		ie_ptr = kmalloc(ie_len, GFP_ATOMIC);
		if (ie_ptr == NULL)
			return -EINVAL;

		memcpy(ie_ptr, beacon->head, ie_len);
		pr_debug("begin send beacon head ies\n");

		ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
					      P2P_BEACON_IE_HEAD, ie_ptr,
					      ie_len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_p2p_ie beacon_ies head failed with ret %d\n",
				ret);
		} else {
			pr_debug("send beacon head ies successfully\n");
		}

		kfree(ie_ptr);
	}

	/* send beacon extra ies */
	if (beacon->tail != NULL) {
		ie_len = beacon->tail_len;

		ie_ptr = kmalloc(ie_len, GFP_ATOMIC);
		if (ie_ptr == NULL)
			return -EINVAL;

		memcpy(ie_ptr, beacon->tail, ie_len);
		pr_debug("begin send beacon tail ies\n");

		ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
					      P2P_BEACON_IE_TAIL, ie_ptr,
					      ie_len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_p2p_ie beacon_ies tail failed with ret %d\n",
				ret);
		} else {
			pr_debug("send beacon tail ies successfully\n");
		}

		kfree(ie_ptr);
	}

	/* send probe response ies */

	/* send beacon extra ies */
	if (beacon->beacon_ies != NULL) {
		ie_len = beacon->beacon_ies_len;

		ie_ptr = kmalloc(ie_len, GFP_ATOMIC);
		if (ie_ptr == NULL)
			return -EINVAL;

		memcpy(ie_ptr, beacon->beacon_ies, ie_len);
		pr_debug("begin send beacon extra ies\n");

		ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
					      P2P_BEACON_IE, ie_ptr, ie_len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_p2p_ie beacon_ies failed with ret %d\n",
				ret);
		} else {
			pr_debug("send beacon extra ies successfully\n");
		}

		kfree(ie_ptr);
	}

	/* send probe response ies */

	if (beacon->proberesp_ies != NULL) {
		ie_len = beacon->proberesp_ies_len;

		ie_ptr = kmalloc(ie_len, GFP_ATOMIC);
		if (ie_ptr == NULL)
			return -EINVAL;

		memcpy(ie_ptr, beacon->proberesp_ies, ie_len);
		pr_debug("begin send probe response extra ies\n");

		ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
					      P2P_PROBERESP_IE, ie_ptr, ie_len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_p2p_ie proberesp_ies failed with ret %d\n",
				ret);
		} else {
			pr_debug("send probe response ies successfully\n");
		}

		kfree(ie_ptr);
	}

	/* send associate response ies */

	if (beacon->assocresp_ies != NULL) {
		ie_len = beacon->assocresp_ies_len;

		ie_ptr = kmalloc(ie_len, GFP_ATOMIC);
		if (ie_ptr == NULL)
			return -EINVAL;

		memcpy(ie_ptr, beacon->assocresp_ies, ie_len);
		pr_debug("begin send associate response extra ies\n");

		ret = itm_wlan_set_p2p_ie_cmd(priv->wlan_sipc,
					      P2P_ASSOCRESP_IE, ie_ptr, ie_len);
		if (ret) {
			dev_err(&priv->ndev->dev,
				"itm_wlan_set_p2p_ie assocresp_ies failed with ret %d\n",
				ret);
		} else {
			pr_debug("send associate response ies successfully\n");
		}

		kfree(ie_ptr);
	}
#endif				/*CONFIG_ITM_WIFI_DIRECT */
	return ret;
}

static int itm_wlan_start_ap(struct itm_priv *priv,
			     struct cfg80211_beacon_data *beacon)
{
	struct ieee80211_mgmt *mgmt;
	u16 mgmt_len;
	int ret;

	/* TODO:
	 * info->interval
	 * info->dtim_period
	 */
	itm_wlan_change_beacon(priv, beacon);

	if (beacon->head == NULL)
		return -EINVAL;

	mgmt_len = beacon->head_len;
	if (beacon->tail)
		mgmt_len += beacon->tail_len;

	mgmt = kmalloc(mgmt_len, GFP_ATOMIC);
	if (mgmt == NULL)
		return -EINVAL;

	memcpy((u8 *)mgmt, beacon->head, beacon->head_len);
	if (beacon->tail)
		memcpy((u8 *)mgmt + beacon->head_len,
		       beacon->tail, beacon->tail_len);

	/* auth type */
	/* akm suites */
	/* ciphers_pairwise */
	/* ciphers_group */
	/* ssid */

	ret = itm_wlan_start_ap_cmd(priv->wlan_sipc, (u8 *)mgmt, mgmt_len);
	kfree(mgmt);
	if (ret != 0)
		wiphy_err(priv->wdev->wiphy, "itm_wlan_start_ap_cmd failed\n");

	return ret;
}

static int itm_wlan_cfg80211_start_ap(struct wiphy *wiphy,
				      struct net_device *ndev,
				      struct cfg80211_ap_settings *info)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	wiphy_info(wiphy, "%s: mode = %d\n", __func__, 
			priv->mode);

	if (info->ssid == NULL) {
		wiphy_err(wiphy, "%s null ssid!\n", __func__);
		return -EINVAL;
	}

	memcpy(priv->ssid, info->ssid, info->ssid_len);
	priv->ssid_len = info->ssid_len;

#ifdef CONFIG_ITM_WIFI_DIRECT
	if (!netif_carrier_ok(priv->ndev)) {
		netif_carrier_on(priv->ndev);
		netif_wake_queue(priv->ndev);
	}
#endif	/* CONFIG_ITM_WIFI_DIRECT */
	return itm_wlan_start_ap(priv, &info->beacon);
}

static int itm_wlan_cfg80211_stop_ap(struct wiphy *wiphy,
				     struct net_device *ndev)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	wiphy_info(wiphy, "%s\n", __func__);
#ifdef CONFIG_ITM_WIFI_DIRECT
	if (netif_carrier_ok(priv->ndev)) {
		netif_carrier_off(priv->ndev);
		netif_stop_queue(priv->ndev);
	}
#endif	/* CONFIG_ITM_WIFI_DIRECT */
	ret = itm_wlan_mac_close_cmd(priv->wlan_sipc, priv->mode);

	return ret;
}

static int itm_wlan_cfg80211_change_beacon(struct wiphy *wiphy,
					   struct net_device *ndev,
					   struct cfg80211_beacon_data *beacon)
{
#ifdef CONFIG_ITM_WIFI_DIRECT
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	wiphy_info(wiphy, "[cfg80211] \t ==>>>%s\n", __func__);
	return itm_wlan_change_beacon(priv, beacon);
#endif	/* CONFIG_ITM_WIFI_DIRECT */
	return 0;
}

static int itm_wlan_change_mode(struct itm_priv *priv, enum nl80211_iftype type)
{
	int mode;
	int ret;

	switch (type) {
	case NL80211_IFTYPE_STATION:
#ifdef CONFIG_ITM_WIFI_DIRECT
		mode = priv->p2p_mode ? ITM_P2P_CLIENT_MODE : ITM_STATION_MODE;
#else	/* CONFIG_ITM_WIFI_DIRECT */
		mode = ITM_STATION_MODE;
#endif	/* CONFIG_ITM_WIFI_DIRECT */
		break;
	case NL80211_IFTYPE_AP:
#ifdef CONFIG_ITM_WIFI_DIRECT
		mode = priv->p2p_mode ? ITM_P2P_GO_MODE : ITM_AP_MODE;
#else	/* CONFIG_ITM_WIFI_DIRECT */
		mode = ITM_AP_MODE;
#endif	/* CONFIG_ITM_WIFI_DIRECT */
		break;
#ifdef CONFIG_ITM_WIFI_DIRECT
	case NL80211_IFTYPE_P2P_CLIENT:
		mode = ITM_P2P_CLIENT_MODE;
		break;
	case NL80211_IFTYPE_P2P_GO:
		mode = ITM_P2P_GO_MODE;
		break;
#endif	/* CONFIG_ITM_WIFI_DIRECT */
	default:
		wiphy_err(priv->wdev->wiphy, "invalid interface type %u\n",
			  type);
		return -EOPNOTSUPP;
	}
#ifdef CONFIG_ITM_WIFI_DIRECT
	priv->wdev->iftype = type;
#endif	/* CONFIG_ITM_WIFI_DIRECT */

	wiphy_info(priv->wdev->wiphy, "%s mode %d\n", __func__, mode);
	if (mode == priv->mode)
		return 0;

	ret = itm_wlan_mac_open_cmd(priv->wlan_sipc,
				    mode, priv->ndev->dev_addr);
	if (ret != 0) {
		wiphy_err(priv->wdev->wiphy, "%s failed to open mac!\n",
			  __func__);
		return -EIO;
	}

	priv->wdev->iftype = type;
	priv->mode = mode;

	return 0;
}

static int itm_wlan_cfg80211_change_iface(struct wiphy *wiphy,
					  struct net_device *ndev,
					  enum nl80211_iftype type, u32 *flags,
					  struct vif_params *params)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	wiphy_info(wiphy, "%s type %d\n", __func__, type);

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}
#ifdef CONFIG_ITM_WIFI_DIRECT
	priv->p2p_mode = itm_get_p2p_mode_from_file();
	wiphy_info(wiphy, "[cfg80211] %s: p2p_mode: %d\n",
			__func__, priv->p2p_mode);
#endif				/* CONFIG_ITM_WIFI_DIRECT */

	return itm_wlan_change_mode(priv, type);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
static int itm_wlan_cfg80211_set_channel(struct wiphy *wiphy,
					 struct net_device *ndev,
					 struct ieee80211_channel *channel)
#else
static int itm_wlan_cfg80211_set_channel(struct wiphy *wiphy,
					 struct net_device *ndev,
					 struct ieee80211_channel *channel,
					 enum nl80211_channel_type channel_type)
#endif
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret = -ENOTSUPP;

	if (!itm_wlan_cfg80211_ready(priv)) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EIO;
	}

	/*
	 * FIXME: To be handled properly when monitor mode is supported.
	 */
	ret =
	    itm_wlan_set_channel_cmd(priv->wlan_sipc,
				     ieee80211_frequency_to_channel
				     (channel->center_freq));
	if (ret < 0) {
		wiphy_err(wiphy,
			  "itm_wlan_set_channel_cmd failed with ret %d\n", ret);
		return ret;
	}

	return 0;
}

/*
static int itm_wlan_cfg80211_set_power_mgmt(struct wiphy *wiphy,
					  struct net_device *dev,
					  bool pmgmt, int timeout)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;

	if (priv->cp2_status != ITM_READY) {
		wiphy_err(wiphy, "CP2 not ready!\n");
		return -EAGAIN;
	}

	if (pmgmt) {
		ret = itm_wlan_pm_enter_ps_cmd(priv->wlan_sipc);
		if (ret < 0) {
			wiphy_err(wiphy,
				"itm_wlan_pm_enter_ps_cmd failed(%d)\n", ret);
			return ret;
		}
	} else {
		ret = itm_wlan_pm_exit_ps_cmd(priv->wlan_sipc);
		if (ret < 0) {
			wiphy_err(wiphy,
				"itm_wlan_pm_exit_ps_cmd failed(%d)\n", ret);
			return ret;
		}
	}

	return 0;
}
*/
#ifdef CONFIG_ITM_WIFI_DIRECT

void itm_cfg80211_p2p_prob_request(struct itm_priv *priv)
{
	u8 *req_ptr, *index;
	u16 req_len, channel_len;
	u8 channel;
	u32 event_len;
	int freq, left;
	pr_debug("enter itm_cfg80211_p2p_prob_request\n");

	event_len = priv->wlan_sipc->wlan_sipc_event_len;

	index = priv->wlan_sipc->event_buf->u.event.variable;
	left = event_len;

	/* The first byte of event data is chan */
	memcpy(&channel_len, index, 2);
	index += 2;
	left -= 2;

	if (channel_len > 1) {
		dev_err(&priv->ndev->dev, "channel len bigger than 1\n");
		return;
	}

	memcpy(&channel, index, channel_len);
	index += channel_len;
	left -= channel_len;

	if (!left) {
		dev_err(&priv->ndev->dev, "There is no req frame!\n");
		return;
	}

	/* The second event data is probe request */
	memcpy(&req_len, index, 2);
	index += 2;
	left -= 2;

	req_ptr = index;
	left -= req_len;

	freq = ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);

	cfg80211_rx_mgmt(priv->wdev, freq, 0, req_ptr, req_len - FCS_LEN,
			 GFP_ATOMIC);

	pr_debug("proccess probe request event completed\n");
}

void itm_cfg80211_mgmt_deauth(struct itm_priv *priv)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	u8 *mac_ptr, *index;
	u16 mac_len;

	index = priv->wlan_sipc->event_buf->u.event.variable;
	memcpy(&mac_len, index, 2);
	index += 2;
	mac_ptr = index;

	cfg80211_dump_frame_prot_info(wiphy, 0, 0, mac_ptr, mac_len);

	cfg80211_send_deauth(priv->ndev, mac_ptr, mac_len);
}

void itm_mac_event_report_frame(struct itm_priv *priv)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	u16 mac_len;
	u8 *mac_ptr = NULL;
	u8 channel = 0, type = 0;
	int freq;

	struct wlan_sipc_event_report_frame *report_frame = NULL;

	report_frame =
	    (struct wlan_sipc_event_report_frame *)priv->wlan_sipc->event_buf->
	    u.event.variable;
	channel = report_frame->channel;
	type = report_frame->frame_type;
	freq = ieee80211_channel_to_frequency(channel, IEEE80211_BAND_2GHZ);
	mac_ptr = (u8 *)(report_frame + 1);
	mac_len = report_frame->frame_len;

	cfg80211_dump_frame_prot_info(wiphy, 0, freq, mac_ptr, mac_len);

	cfg80211_rx_mgmt(priv->wdev, freq, 0, mac_ptr, mac_len - FCS_LEN,
			 GFP_ATOMIC);
}

void itm_cfg80211_mgmt_disassoc(struct itm_priv *priv)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	u8 *mac_ptr, *index;
	u16 mac_len;

	index = priv->wlan_sipc->event_buf->u.event.variable;
	memcpy(&mac_len, index, 2);
	index += 2;
	mac_ptr = index;

	cfg80211_dump_frame_prot_info(wiphy, 0, 0, mac_ptr, mac_len);

	cfg80211_send_disassoc(priv->ndev, mac_ptr, mac_len);
}

void itm_cfg80211_remain_on_channel_expired(struct itm_priv *priv)
{
	struct wiphy *wiphy = priv->wdev->wiphy;
	u8 *index = NULL;
	u16 type_len;
	u8 chan_type;
	u32 event_len;
	int left;

	wiphy_info(wiphy, "[cfg80211] %s.\n", __func__);

	event_len = priv->wlan_sipc->wlan_sipc_event_len;

	index = priv->wlan_sipc->event_buf->u.event.variable;
	left = event_len;

	/* The first byte of event data is cookie */
	memcpy(&type_len, index, 2);
	index += 2;
	left -= 2;

	if (type_len > 1) {
		dev_err(&priv->ndev->dev, "type len error\n");
		return;
	}

	memcpy(&chan_type, index, type_len);
	index += type_len;
	left -= type_len;

	cfg80211_remain_on_channel_expired(priv->wdev, priv->listen_cookie,
					   &priv->listen_channel, GFP_KERNEL);
}

static int itm_wlan_cfg80211_remain_on_channel(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
					       struct wireless_dev *dev,
#else
					       struct net_device *dev,
#endif
					       struct ieee80211_channel
					       *channel,
					       unsigned int duration,
					       u64 *cookie)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	int ret;
	enum nl80211_channel_type channel_type = 0;

	wiphy_info(wiphy, "[cfg80211] %s %d for %dms. cookie=0x%llx\n",
		__func__, channel->center_freq, duration, *cookie);
	if (priv->cp2_status != ITM_READY) {
		dev_err(&priv->ndev->dev, "CP2 not ready!\n");
		return -EAGAIN;
	}

	memcpy(&priv->listen_channel, channel, sizeof(struct ieee80211_channel));
	priv->listen_cookie = *cookie;

	/* send remain chan */

	ret = itm_wlan_remain_chan_cmd(priv->wlan_sipc, channel,
				       channel_type, duration, cookie);
	if (ret) {
		dev_err(&priv->ndev->dev,
			"itm_wlan_remain_chan_cmd failed with ret %d\n", ret);
		return ret;
	}
	/* report remain chan */
	cfg80211_ready_on_channel(dev, *cookie, channel, duration, GFP_KERNEL);
	return 0;
}

static int itm_wlan_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
						      struct wireless_dev *dev,
#else
						      struct net_device *dev,
#endif
						      u64 cookie)
{
	int ret;
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	wiphy_info(wiphy, "[cfg80211] %s cookie = 0x%llx.\n",
		__func__, cookie);

	if (priv->cp2_status != ITM_READY) {
		dev_err(&priv->ndev->dev, "CP2 not ready!\n");
		return -EAGAIN;
	}

	ret = itm_wlan_cancel_remain_chan_cmd(priv->wlan_sipc, cookie);
	if (ret) {
		dev_err(&priv->ndev->dev,
			"itm_wlan_cancel_remain_chan_cmd failed with ret %d.\n",
			ret);
		return ret;
	}

	return 0;
}

static int itm_wlan_cfg80211_del_station(struct wiphy *wiphy,
					 struct net_device *ndev, u8 *mac)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;

	if (priv->cp2_status != ITM_READY) {
		dev_err(&priv->ndev->dev, "CP2 not ready!\n");
		return -EAGAIN;
	}

	return 0;
}
#endif				/*CONFIG_ITM_WIFI_DIRECT */

static struct cfg80211_ops itm_cfg80211_ops = {
/*#ifdef CONFIG_PM
	.suspend = itm_wlan_cfg80211_suspend,
	.resume = itm_wlan_cfg80211_resume,
#endif*/
	/*.add_virtual_intf = itm_wlan_cfg80211_add_iface, */
	/*.del_virtual_intf = itm_wlan_cfg80211_del_iface, */
	.change_virtual_intf = itm_wlan_cfg80211_change_iface,
	.scan = itm_wlan_cfg80211_scan,
	.connect = itm_wlan_cfg80211_connect,
	.disconnect = itm_wlan_cfg80211_disconnect,
	.add_key = itm_wlan_cfg80211_add_key,
	/*.get_key = itm_wlan_cfg80211_get_key, */
	.del_key = itm_wlan_cfg80211_del_key,
	.set_default_key = itm_wlan_cfg80211_set_default_key,
	.set_wiphy_params = itm_wlan_cfg80211_set_wiphy_params,
	/*.set_tx_power = itm_wlan_cfg80211_set_txpower, */
	/*.get_tx_power = itm_wlan_cfg80211_get_txpower, */
	/*.set_power_mgmt = itm_wlan_cfg80211_set_power_mgmt, */
	/*.join_ibss = itm_wlan_cfg80211_join_ibss, */
	/*.leave_ibss = itm_wlan_cfg80211_leave_ibss, */
	.get_station = itm_wlan_cfg80211_get_station,
	.set_pmksa = itm_wlan_cfg80211_set_pmksa,
	.del_pmksa = itm_wlan_cfg80211_del_pmksa,
	.flush_pmksa = itm_wlan_cfg80211_flush_pmksa,
/*      CFG80211_TESTMODE_CMD(itm_wlan_tm_cmd)*/
	/* AP mode */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)) && \
	!defined(COMPAT_KERNEL_RELEASE)
	.add_beacon = itm_wlan_cfg80211_add_beacon,
	.set_beacon = itm_wlan_cfg80211_set_beacon,
	.del_beacon = itm_wlan_cfg80211_del_beacon,
#else
	.start_ap = itm_wlan_cfg80211_start_ap,
	.change_beacon = itm_wlan_cfg80211_change_beacon,
	.stop_ap = itm_wlan_cfg80211_stop_ap,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 37)) || \
	defined(COMPAT_KERNEL_RELEASE)
	.mgmt_tx = itm_wlan_cfg80211_mgmt_tx,
	.mgmt_frame_register = itm_wlan_cfg80211_mgmt_frame_register,
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34) && \
	LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 35))
	.action = itm_wlan_cfg80211_mgmt_tx,
#endif
#ifdef CONFIG_ITM_WIFI_DIRECT
	.remain_on_channel = itm_wlan_cfg80211_remain_on_channel,
	.cancel_remain_on_channel = itm_wlan_cfg80211_cancel_remain_on_channel,
	.del_station = itm_wlan_cfg80211_del_station,
#endif				/*CONFIG_ITM_WIFI_DIRECT */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	.libertas_set_mesh_channel = itm_wlan_cfg80211_set_channel,
#else
	.set_channel = itm_wlan_cfg80211_set_channel,
#endif
};

#define ENG_MAC_ADDR_PATH "/data/misc/wifi/wifimac.txt"
int itm_get_mac_from_cfg(struct itm_priv *priv)
{
	struct file *fp = 0;
	mm_segment_t fs;
	loff_t *pos;
	u8 file_data[64] = { 0 };
	u8 mac_addr[18] = { 0 };
	u8 *tmp_p = NULL;

	fp = filp_open(ENG_MAC_ADDR_PATH, O_RDONLY, 0);
	if (IS_ERR(fp))
		return -ENOENT;

	fs = get_fs();
	set_fs(KERNEL_DS);

	pos = &(fp->f_pos);
	vfs_read(fp, file_data, sizeof(file_data), pos);
	tmp_p = file_data;
	if (tmp_p != NULL) {
		memcpy(mac_addr, tmp_p, 18);
		sscanf(mac_addr, "%02x:%02x:%02x:%02x:%02x:%02x",
		       (unsigned int *)&(priv->ndev->dev_addr[0]),
		       (unsigned int *)&(priv->ndev->dev_addr[1]),
		       (unsigned int *)&(priv->ndev->dev_addr[2]),
		       (unsigned int *)&(priv->ndev->dev_addr[3]),
		       (unsigned int *)&(priv->ndev->dev_addr[4]),
		       (unsigned int *)&(priv->ndev->dev_addr[5]));
	}

	filp_close(fp, NULL);
	set_fs(fs);

	return 0;
}

static void itm_cfg80211_reg_notify(struct wiphy *wiphy,
				    struct regulatory_request *request)
{
	struct itm_priv **priv_ptr = wiphy_priv(wiphy);
	struct itm_priv *priv = *priv_ptr;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	const struct ieee80211_freq_range *freq_range;
	const struct ieee80211_reg_rule *reg_rule;
	struct itm_ieee80211_regdomain *rd = NULL;
	u32 band, channel, i;
	u32 last_start_freq;
	u32 n_rules = 0, rd_size;

	wiphy_info(wiphy, "%s %c%c initiator %d hint_type %d\n", __func__,
		   request->alpha2[0], request->alpha2[1],
		   request->initiator, request->user_reg_hint_type);

	/* Figure out the actual rule number */
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			reg_rule =
			    freq_reg_info(wiphy, MHZ_TO_KHZ(chan->center_freq));
			if (IS_ERR(reg_rule))
				continue;

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz) {
				last_start_freq = freq_range->start_freq_khz;
				n_rules++;
			}
		}
	}

	rd_size = sizeof(struct itm_ieee80211_regdomain) +
	    n_rules * sizeof(struct ieee80211_reg_rule);

	rd = kzalloc(rd_size, GFP_KERNEL);
	if (!rd) {
		wiphy_err(wiphy,
			  "Failed to allocate itm_ieee80211_regdomain\n");
		return;
	}

	/* Fill regulatory domain */
	rd->n_reg_rules = n_rules;
	memcpy(rd->alpha2, request->alpha2, ARRAY_SIZE(rd->alpha2));
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		sband = wiphy->bands[band];
		if (!sband)
			continue;

		last_start_freq = 0;
		for (channel = i = 0; channel < sband->n_channels; channel++) {
			chan = &sband->channels[channel];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

			reg_rule =
			    freq_reg_info(wiphy, MHZ_TO_KHZ(chan->center_freq));
			if (IS_ERR(reg_rule))
				continue;

			freq_range = &reg_rule->freq_range;
			if (last_start_freq != freq_range->start_freq_khz
			    && i < n_rules) {
				last_start_freq = freq_range->start_freq_khz;
				memcpy(&rd->reg_rules[i], reg_rule,
				       sizeof(struct ieee80211_reg_rule));
				i++;
				wiphy_dbg(wiphy,
					  "%s %d KHz - %d KHz @ %d KHz flags %#x\n",
					  __func__, freq_range->start_freq_khz,
					  freq_range->end_freq_khz,
					  freq_range->max_bandwidth_khz,
					  reg_rule->flags);
			}
		}
	}

	if (itm_wlan_set_regdom_cmd(priv->wlan_sipc, (u8 *)rd, rd_size))
		wiphy_err(wiphy, "%s failed to set regdomain!\n", __func__);
	kfree(rd);
}

/*Init wiphy parameters*/
static void init_wiphy_parameters(struct itm_priv *priv, struct wiphy *wiphy)
{
	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;
	wiphy->mgmt_stypes = itm_mgmt_stypes;

	wiphy->max_scan_ssids = MAX_SITES_FOR_SCAN;
	wiphy->max_scan_ie_len = SCAN_IE_LEN_MAX;
	wiphy->max_num_pmkids = MAX_NUM_PMKIDS;

	/*TODO:consider AP mode */

/*#ifdef IBSS_BSS_STATION_MODE*/
	wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION)
	    | BIT(NL80211_IFTYPE_AP)
	    | BIT(NL80211_IFTYPE_ADHOC); /*fix the problem with ifconfig */
/*#endif
#ifdef BSS_ACCESS_POINT_MODE
	wiphy->interface_modes = BIT(NL80211_IFTYPE_AP);
#endif*/
#ifdef CONFIG_ITM_WIFI_DIRECT
	wiphy->interface_modes |=
	    BIT(NL80211_IFTYPE_P2P_CLIENT) | BIT(NL80211_IFTYPE_P2P_GO);
	wiphy->max_remain_on_channel_duration = 5000;
	wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
	/* set AP SME flag, also needed by STA mode? */
	wiphy->flags |= WIPHY_FLAG_HAVE_AP_SME;
	wiphy->ap_sme_capa = 1;
#endif	/*CONFIG_ITM_WIFI_DIRECT */
	/*Attach cipher suites */
	wiphy->cipher_suites = itm_cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(itm_cipher_suites);
	/*Attach bands */
	wiphy->bands[IEEE80211_BAND_2GHZ] = &itm_band_2ghz;
	/*wiphy->bands[IEEE80211_BAND_5GHZ] = &itm_band_5ghz;*/

	/*Default not in powersave state */
	wiphy->flags &= ~WIPHY_FLAG_PS_ON_BY_DEFAULT;

#if defined(CONFIG_PM) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
	/*Set WoWLAN flags */
	wiphy->wowlan.flags = WIPHY_WOWLAN_ANY | WIPHY_WOWLAN_DISCONNECT;
#endif
	wiphy->reg_notifier = itm_cfg80211_reg_notify;
}

int itm_register_wdev(struct itm_priv *priv, struct device *dev)
{
	int ret = 0;
	struct wireless_dev *wdev;
	struct net_device *ndev;

	if (priv == NULL || priv->ndev == NULL)
		return -ENODEV;

	ndev = priv->ndev;

	/*Allocate wireless_dev */
	wdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);

	if (wdev == NULL) {
		dev_err(&priv->ndev->dev, "Failed to allocate wireless_dev\n");
		return -ENOMEM;
	}

	ret = itm_wlan_sipc_alloc(priv);
	if (ret) {
		dev_err(&priv->ndev->dev, "Cannot allocate spic device\n");
		ret = -ENOMEM;
		goto out_free_wdev;
	}

	/*Allocate wiphy */
	wdev->wiphy = wiphy_new(&itm_cfg80211_ops, sizeof(struct itm_priv *));

	if (wdev->wiphy == NULL) {
		dev_err(&priv->ndev->dev, "Failed to allocate wiphy\n");
		ret = -ENOMEM;
		goto out_free_sipc;
	}

	*(struct itm_priv **)wiphy_priv(wdev->wiphy) = priv;
	set_wiphy_dev(wdev->wiphy, dev);

	priv->wdev = wdev;
	ndev->ieee80211_ptr = wdev;

	/*Init wdev_priv */
	priv->scan_request = NULL;
	priv->connect_status = ITM_DISCONNECTED;
	memset(priv->bssid, 0, sizeof(priv->bssid));
	priv->mode = ITM_NONE_MODE;
	/* FIXME it will be modify when cp2 code ready */
	priv->cp2_status = ITM_READY;

	/* Init scan_timeout timer */
	init_timer(&priv->scan_timeout);
	priv->scan_timeout.data = (unsigned long)priv;
	priv->scan_timeout.function = itm_cfg80211_scan_timeout;

	wdev->netdev = ndev;

	/*FIXME make sure change_virtual_intf happen */
	wdev->iftype = NL80211_IFTYPE_ADHOC;

	/*Init wiphy parameters */
	init_wiphy_parameters(priv, wdev->wiphy);

	/*register wiphy */
	ret = wiphy_register(wdev->wiphy);

	if (ret < 0) {
		dev_err(&priv->ndev->dev,
			"Failed to regitster wiphy (%d)\n", ret);
		goto out_free_wiphy;
	}

	SET_NETDEV_DEV(ndev, wiphy_dev(wdev->wiphy));

	return 0;

out_free_wiphy:
	wiphy_free(wdev->wiphy);
out_free_sipc:
	itm_wlan_sipc_free(priv);
out_free_wdev:
	kfree(wdev);
	return ret;
}

void itm_unregister_wdev(struct itm_priv *priv)
{
	if (priv->wdev == NULL)
		return;

	if (priv->mode != ITM_NONE_MODE) {
		itm_wlan_mac_close_cmd(priv->wlan_sipc, priv->mode);
	}

	wiphy_unregister(priv->wdev->wiphy);
	wiphy_free(priv->wdev->wiphy);

	itm_wlan_sipc_free(priv);

	kfree(priv->wdev);
}
