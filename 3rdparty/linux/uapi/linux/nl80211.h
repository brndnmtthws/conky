// SOURCE:
// https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/plain/include/uapi/linux/nl80211.h

#ifndef __CONKY_LINUX_NL80211_H__
#define __CONKY_LINUX_NL80211_H__

/*
 * 802.11 netlink interface public header
 *
 * Copyright 2006-2010 Johannes Berg <johannes@sipsolutions.net>
 * Copyright 2008 Michael Wu <flamingice@sourmilk.net>
 * Copyright 2008 Luis Carlos Cobo <luisca@cozybit.com>
 * Copyright 2008 Michael Buesch <m@bues.ch>
 * Copyright 2008, 2009 Luis R. Rodriguez <lrodriguez@atheros.com>
 * Copyright 2008 Jouni Malinen <jouni.malinen@atheros.com>
 * Copyright 2008 Colin McCabe <colin@cozybit.com>
 * Copyright 2015-2017	Intel Deutschland GmbH
 * Copyright (C) 2018-2024 Intel Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * This header file defines the userspace API to the wireless stack. Please
 * be careful not to break things - i.e. don't move anything around or so
 * unless you can demonstrate that it breaks neither API nor ABI.
 *
 * Additions to the API should be accompanied by actual implementations in
 * an upstream driver, so that example implementations exist in case there
 * are ever concerns about the precise semantics of the API or changes are
 * needed, and to ensure that code for dead (no longer implemented) API
 * can actually be identified and removed.
 * Nonetheless, semantics should also be documented carefully in this file.
 */

/**
 * enum nl80211_attrs - nl80211 netlink attributes
 *
 * @NL80211_ATTR_UNSPEC: unspecified attribute to catch errors
 *
 * @NL80211_ATTR_WIPHY: index of wiphy to operate on, cf.
 *	/sys/class/ieee80211/<phyname>/index
 * @NL80211_ATTR_WIPHY_NAME: wiphy name (used for renaming)
 * @NL80211_ATTR_WIPHY_TXQ_PARAMS: a nested array of TX queue parameters
 * @NL80211_ATTR_WIPHY_FREQ: frequency of the selected channel in MHz,
 *	defines the channel together with the (deprecated)
 *	%NL80211_ATTR_WIPHY_CHANNEL_TYPE attribute or the attributes
 *	%NL80211_ATTR_CHANNEL_WIDTH and if needed %NL80211_ATTR_CENTER_FREQ1
 *	and %NL80211_ATTR_CENTER_FREQ2
 * @NL80211_ATTR_CHANNEL_WIDTH: u32 attribute containing one of the values
 *	of &enum nl80211_chan_width, describing the channel width. See the
 *	documentation of the enum for more information.
 * @NL80211_ATTR_CENTER_FREQ1: Center frequency of the first part of the
 *	channel, used for anything but 20 MHz bandwidth. In S1G this is the
 *	operating channel center frequency.
 * @NL80211_ATTR_CENTER_FREQ2: Center frequency of the second part of the
 *	channel, used only for 80+80 MHz bandwidth
 * @NL80211_ATTR_WIPHY_CHANNEL_TYPE: included with NL80211_ATTR_WIPHY_FREQ
 *	if HT20 or HT40 are to be used (i.e., HT disabled if not included):
 *	NL80211_CHAN_NO_HT = HT not allowed (i.e., same as not including
 *		this attribute)
 *	NL80211_CHAN_HT20 = HT20 only
 *	NL80211_CHAN_HT40MINUS = secondary channel is below the primary channel
 *	NL80211_CHAN_HT40PLUS = secondary channel is above the primary channel
 *	This attribute is now deprecated.
 * @NL80211_ATTR_WIPHY_RETRY_SHORT: TX retry limit for frames whose length is
 *	less than or equal to the RTS threshold; allowed range: 1..255;
 *	dot11ShortRetryLimit; u8
 * @NL80211_ATTR_WIPHY_RETRY_LONG: TX retry limit for frames whose length is
 *	greater than the RTS threshold; allowed range: 1..255;
 *	dot11ShortLongLimit; u8
 * @NL80211_ATTR_WIPHY_FRAG_THRESHOLD: fragmentation threshold, i.e., maximum
 *	length in octets for frames; allowed range: 256..8000, disable
 *	fragmentation with (u32)-1; dot11FragmentationThreshold; u32
 * @NL80211_ATTR_WIPHY_RTS_THRESHOLD: RTS threshold (TX frames with length
 *	larger than or equal to this use RTS/CTS handshake); allowed range:
 *	0..65536, disable with (u32)-1; dot11RTSThreshold; u32
 * @NL80211_ATTR_WIPHY_COVERAGE_CLASS: Coverage Class as defined by IEEE 802.11
 *	section 7.3.2.9; dot11CoverageClass; u8
 *
 * @NL80211_ATTR_IFINDEX: network interface index of the device to operate on
 * @NL80211_ATTR_IFNAME: network interface name
 * @NL80211_ATTR_IFTYPE: type of virtual interface, see &enum nl80211_iftype
 *
 * @NL80211_ATTR_WDEV: wireless device identifier, used for pseudo-devices
 *	that don't have a netdev (u64)
 *
 * @NL80211_ATTR_MAC: MAC address (various uses)
 *
 * @NL80211_ATTR_KEY_DATA: (temporal) key data; for TKIP this consists of
 *	16 bytes encryption key followed by 8 bytes each for TX and RX MIC
 *	keys
 * @NL80211_ATTR_KEY_IDX: key ID (u8, 0-3)
 * @NL80211_ATTR_KEY_CIPHER: key cipher suite (u32, as defined by IEEE 802.11
 *	section 7.3.2.25.1, e.g. 0x000FAC04)
 * @NL80211_ATTR_KEY_SEQ: transmit key sequence number (IV/PN) for TKIP and
 *	CCMP keys, each six bytes in little endian
 * @NL80211_ATTR_KEY_DEFAULT: Flag attribute indicating the key is default key
 * @NL80211_ATTR_KEY_DEFAULT_MGMT: Flag attribute indicating the key is the
 *	default management key
 * @NL80211_ATTR_CIPHER_SUITES_PAIRWISE: For crypto settings for connect or
 *	other commands, indicates which pairwise cipher suites are used
 * @NL80211_ATTR_CIPHER_SUITE_GROUP: For crypto settings for connect or
 *	other commands, indicates which group cipher suite is used
 *
 * @NL80211_ATTR_BEACON_INTERVAL: beacon interval in TU
 * @NL80211_ATTR_DTIM_PERIOD: DTIM period for beaconing
 * @NL80211_ATTR_BEACON_HEAD: portion of the beacon before the TIM IE
 * @NL80211_ATTR_BEACON_TAIL: portion of the beacon after the TIM IE
 *
 * @NL80211_ATTR_STA_AID: Association ID for the station (u16)
 * @NL80211_ATTR_STA_FLAGS: flags, nested element with NLA_FLAG attributes of
 *	&enum nl80211_sta_flags (deprecated, use %NL80211_ATTR_STA_FLAGS2)
 * @NL80211_ATTR_STA_LISTEN_INTERVAL: listen interval as defined by
 *	IEEE 802.11 7.3.1.6 (u16).
 * @NL80211_ATTR_STA_SUPPORTED_RATES: supported rates, array of supported
 *	rates as defined by IEEE 802.11 7.3.2.2 but without the length
 *	restriction (at most %NL80211_MAX_SUPP_RATES).
 * @NL80211_ATTR_STA_VLAN: interface index of VLAN interface to move station
 *	to, or the AP interface the station was originally added to.
 * @NL80211_ATTR_STA_INFO: information about a station, part of station info
 *	given for %NL80211_CMD_GET_STATION, nested attribute containing
 *	info as possible, see &enum nl80211_sta_info.
 *
 * @NL80211_ATTR_WIPHY_BANDS: Information about an operating bands,
 *	consisting of a nested array.
 *
 * @NL80211_ATTR_MESH_ID: mesh id (1-32 bytes).
 * @NL80211_ATTR_STA_PLINK_ACTION: action to perform on the mesh peer link
 *	(see &enum nl80211_plink_action).
 * @NL80211_ATTR_MPATH_NEXT_HOP: MAC address of the next hop for a mesh path.
 * @NL80211_ATTR_MPATH_INFO: information about a mesh_path, part of mesh path
 * 	info given for %NL80211_CMD_GET_MPATH, nested attribute described at
 *	&enum nl80211_mpath_info.
 *
 * @NL80211_ATTR_MNTR_FLAGS: flags, nested element with NLA_FLAG attributes of
 *      &enum nl80211_mntr_flags.
 *
 * @NL80211_ATTR_REG_ALPHA2: an ISO-3166-alpha2 country code for which the
 * 	current regulatory domain should be set to or is already set to.
 * 	For example, 'CR', for Costa Rica. This attribute is used by the kernel
 * 	to query the CRDA to retrieve one regulatory domain. This attribute can
 * 	also be used by userspace to query the kernel for the currently set
 * 	regulatory domain. We chose an alpha2 as that is also used by the
 * 	IEEE-802.11 country information element to identify a country.
 * 	Users can also simply ask the wireless core to set regulatory domain
 * 	to a specific alpha2.
 * @NL80211_ATTR_REG_RULES: a nested array of regulatory domain regulatory
 *	rules.
 *
 * @NL80211_ATTR_BSS_CTS_PROT: whether CTS protection is enabled (u8, 0 or 1)
 * @NL80211_ATTR_BSS_SHORT_PREAMBLE: whether short preamble is enabled
 *	(u8, 0 or 1)
 * @NL80211_ATTR_BSS_SHORT_SLOT_TIME: whether short slot time enabled
 *	(u8, 0 or 1)
 * @NL80211_ATTR_BSS_BASIC_RATES: basic rates, array of basic
 *	rates in format defined by IEEE 802.11 7.3.2.2 but without the length
 *	restriction (at most %NL80211_MAX_SUPP_RATES).
 *
 * @NL80211_ATTR_HT_CAPABILITY: HT Capability information element (from
 *	association request when used with NL80211_CMD_NEW_STATION)
 *
 * @NL80211_ATTR_SUPPORTED_IFTYPES: nested attribute containing all
 *	supported interface types, each a flag attribute with the number
 *	of the interface mode.
 *
 * @NL80211_ATTR_MGMT_SUBTYPE: Management frame subtype for
 *	%NL80211_CMD_SET_MGMT_EXTRA_IE.
 *
 * @NL80211_ATTR_IE: Information element(s) data (used, e.g., with
 *	%NL80211_CMD_SET_MGMT_EXTRA_IE).
 *
 * @NL80211_ATTR_MAX_NUM_SCAN_SSIDS: number of SSIDs you can scan with
 *	a single scan request, a wiphy attribute.
 * @NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS: number of SSIDs you can
 *	scan with a single scheduled scan request, a wiphy attribute.
 * @NL80211_ATTR_MAX_SCAN_IE_LEN: maximum length of information elements
 *	that can be added to a scan request
 * @NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN: maximum length of information
 *	elements that can be added to a scheduled scan request
 * @NL80211_ATTR_MAX_MATCH_SETS: maximum number of sets that can be
 *	used with @NL80211_ATTR_SCHED_SCAN_MATCH, a wiphy attribute.
 *
 * @NL80211_ATTR_SCAN_FREQUENCIES: nested attribute with frequencies (in MHz)
 * @NL80211_ATTR_SCAN_SSIDS: nested attribute with SSIDs, leave out for passive
 *	scanning and include a zero-length SSID (wildcard) for wildcard scan
 * @NL80211_ATTR_BSS: scan result BSS
 *
 * @NL80211_ATTR_REG_INITIATOR: indicates who requested the regulatory domain
 * 	currently in effect. This could be any of the %NL80211_REGDOM_SET_BY_*
 * @NL80211_ATTR_REG_TYPE: indicates the type of the regulatory domain currently
 * 	set. This can be one of the nl80211_reg_type (%NL80211_REGDOM_TYPE_*)
 *
 * @NL80211_ATTR_SUPPORTED_COMMANDS: wiphy attribute that specifies
 *	an array of command numbers (i.e. a mapping index to command number)
 *	that the driver for the given wiphy supports.
 *
 * @NL80211_ATTR_FRAME: frame data (binary attribute), including frame header
 *	and body, but not FCS; used, e.g., with NL80211_CMD_AUTHENTICATE and
 *	NL80211_CMD_ASSOCIATE events
 * @NL80211_ATTR_SSID: SSID (binary attribute, 0..32 octets)
 * @NL80211_ATTR_AUTH_TYPE: AuthenticationType, see &enum nl80211_auth_type,
 *	represented as a u32
 * @NL80211_ATTR_REASON_CODE: ReasonCode for %NL80211_CMD_DEAUTHENTICATE and
 *	%NL80211_CMD_DISASSOCIATE, u16
 *
 * @NL80211_ATTR_KEY_TYPE: Key Type, see &enum nl80211_key_type, represented as
 *	a u32
 *
 * @NL80211_ATTR_FREQ_BEFORE: A channel which has suffered a regulatory change
 * 	due to considerations from a beacon hint. This attribute reflects
 * 	the state of the channel _before_ the beacon hint processing. This
 * 	attributes consists of a nested attribute containing
 * 	NL80211_FREQUENCY_ATTR_*
 * @NL80211_ATTR_FREQ_AFTER: A channel which has suffered a regulatory change
 * 	due to considerations from a beacon hint. This attribute reflects
 * 	the state of the channel _after_ the beacon hint processing. This
 * 	attributes consists of a nested attribute containing
 * 	NL80211_FREQUENCY_ATTR_*
 *
 * @NL80211_ATTR_CIPHER_SUITES: a set of u32 values indicating the supported
 *	cipher suites
 *
 * @NL80211_ATTR_FREQ_FIXED: a flag indicating the IBSS should not try to look
 *	for other networks on different channels
 *
 * @NL80211_ATTR_TIMED_OUT: a flag indicating than an operation timed out; this
 *	is used, e.g., with %NL80211_CMD_AUTHENTICATE event
 *
 * @NL80211_ATTR_USE_MFP: Whether management frame protection (IEEE 802.11w) is
 *	used for the association (&enum nl80211_mfp, represented as a u32);
 *	this attribute can be used with %NL80211_CMD_ASSOCIATE and
 *	%NL80211_CMD_CONNECT requests. %NL80211_MFP_OPTIONAL is not allowed for
 *	%NL80211_CMD_ASSOCIATE since user space SME is expected and hence, it
 *	must have decided whether to use management frame protection or not.
 *	Setting %NL80211_MFP_OPTIONAL with a %NL80211_CMD_CONNECT request will
 *	let the driver (or the firmware) decide whether to use MFP or not.
 *
 * @NL80211_ATTR_STA_FLAGS2: Attribute containing a
 *	&struct nl80211_sta_flag_update.
 *
 * @NL80211_ATTR_CONTROL_PORT: A flag indicating whether user space controls
 *	IEEE 802.1X port, i.e., sets/clears %NL80211_STA_FLAG_AUTHORIZED, in
 *	station mode. If the flag is included in %NL80211_CMD_ASSOCIATE
 *	request, the driver will assume that the port is unauthorized until
 *	authorized by user space. Otherwise, port is marked authorized by
 *	default in station mode.
 * @NL80211_ATTR_CONTROL_PORT_ETHERTYPE: A 16-bit value indicating the
 *	ethertype that will be used for key negotiation. It can be
 *	specified with the associate and connect commands. If it is not
 *	specified, the value defaults to 0x888E (PAE, 802.1X). This
 *	attribute is also used as a flag in the wiphy information to
 *	indicate that protocols other than PAE are supported.
 * @NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT: When included along with
 *	%NL80211_ATTR_CONTROL_PORT_ETHERTYPE, indicates that the custom
 *	ethertype frames used for key negotiation must not be encrypted.
 * @NL80211_ATTR_CONTROL_PORT_OVER_NL80211: A flag indicating whether control
 *	port frames (e.g. of type given in %NL80211_ATTR_CONTROL_PORT_ETHERTYPE)
 *	will be sent directly to the network interface or sent via the NL80211
 *	socket.  If this attribute is missing, then legacy behavior of sending
 *	control port frames directly to the network interface is used.  If the
 *	flag is included, then control port frames are sent over NL80211 instead
 *	using %CMD_CONTROL_PORT_FRAME.  If control port routing over NL80211 is
 *	to be used then userspace must also use the %NL80211_ATTR_SOCKET_OWNER
 *	flag. When used with %NL80211_ATTR_CONTROL_PORT_NO_PREAUTH, pre-auth
 *	frames are not forwarded over the control port.
 *
 * @NL80211_ATTR_TESTDATA: Testmode data blob, passed through to the driver.
 *	We recommend using nested, driver-specific attributes within this.
 *
 * @NL80211_ATTR_DISCONNECTED_BY_AP: A flag indicating that the DISCONNECT
 *	event was due to the AP disconnecting the station, and not due to
 *	a local disconnect request.
 * @NL80211_ATTR_STATUS_CODE: StatusCode for the %NL80211_CMD_CONNECT
 *	event (u16)
 * @NL80211_ATTR_PRIVACY: Flag attribute, used with connect(), indicating
 *	that protected APs should be used. This is also used with NEW_BEACON to
 *	indicate that the BSS is to use protection.
 *
 * @NL80211_ATTR_CIPHERS_PAIRWISE: Used with CONNECT, ASSOCIATE, and NEW_BEACON
 *	to indicate which unicast key ciphers will be used with the connection
 *	(an array of u32).
 * @NL80211_ATTR_CIPHER_GROUP: Used with CONNECT, ASSOCIATE, and NEW_BEACON to
 *	indicate which group key cipher will be used with the connection (a
 *	u32).
 * @NL80211_ATTR_WPA_VERSIONS: Used with CONNECT, ASSOCIATE, and NEW_BEACON to
 *	indicate which WPA version(s) the AP we want to associate with is using
 *	(a u32 with flags from &enum nl80211_wpa_versions).
 * @NL80211_ATTR_AKM_SUITES: Used with CONNECT, ASSOCIATE, and NEW_BEACON to
 *	indicate which key management algorithm(s) to use (an array of u32).
 *	This attribute is also sent in response to @NL80211_CMD_GET_WIPHY,
 *	indicating the supported AKM suites, intended for specific drivers which
 *	implement SME and have constraints on which AKMs are supported and also
 *	the cases where an AKM support is offloaded to the driver/firmware.
 *	If there is no such notification from the driver, user space should
 *	assume the driver supports all the AKM suites.
 *
 * @NL80211_ATTR_REQ_IE: (Re)association request information elements as
 *	sent out by the card, for ROAM and successful CONNECT events.
 * @NL80211_ATTR_RESP_IE: (Re)association response information elements as
 *	sent by peer, for ROAM and successful CONNECT events.
 *
 * @NL80211_ATTR_PREV_BSSID: previous BSSID, to be used in ASSOCIATE and CONNECT
 *	commands to specify a request to reassociate within an ESS, i.e., to use
 *	Reassociate Request frame (with the value of this attribute in the
 *	Current AP address field) instead of Association Request frame which is
 *	used for the initial association to an ESS.
 *
 * @NL80211_ATTR_KEY: key information in a nested attribute with
 *	%NL80211_KEY_* sub-attributes
 * @NL80211_ATTR_KEYS: array of keys for static WEP keys for connect()
 *	and join_ibss(), key information is in a nested attribute each
 *	with %NL80211_KEY_* sub-attributes
 *
 * @NL80211_ATTR_PID: Process ID of a network namespace.
 *
 * @NL80211_ATTR_GENERATION: Used to indicate consistent snapshots for
 *	dumps. This number increases whenever the object list being
 *	dumped changes, and as such userspace can verify that it has
 *	obtained a complete and consistent snapshot by verifying that
 *	all dump messages contain the same generation number. If it
 *	changed then the list changed and the dump should be repeated
 *	completely from scratch.
 *
 * @NL80211_ATTR_4ADDR: Use 4-address frames on a virtual interface
 *
 * @NL80211_ATTR_SURVEY_INFO: survey information about a channel, part of
 *      the survey response for %NL80211_CMD_GET_SURVEY, nested attribute
 *      containing info as possible, see &enum survey_info.
 *
 * @NL80211_ATTR_PMKID: PMK material for PMKSA caching.
 * @NL80211_ATTR_MAX_NUM_PMKIDS: maximum number of PMKIDs a firmware can
 *	cache, a wiphy attribute.
 *
 * @NL80211_ATTR_DURATION: Duration of an operation in milliseconds, u32.
 * @NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION: Device attribute that
 *	specifies the maximum duration that can be requested with the
 *	remain-on-channel operation, in milliseconds, u32.
 *
 * @NL80211_ATTR_COOKIE: Generic 64-bit cookie to identify objects.
 *
 * @NL80211_ATTR_TX_RATES: Nested set of attributes
 *	(enum nl80211_tx_rate_attributes) describing TX rates per band. The
 *	enum nl80211_band value is used as the index (nla_type() of the nested
 *	data. If a band is not included, it will be configured to allow all
 *	rates based on negotiated supported rates information. This attribute
 *	is used with %NL80211_CMD_SET_TX_BITRATE_MASK and with starting AP,
 *	and joining mesh networks (not IBSS yet). In the later case, it must
 *	specify just a single bitrate, which is to be used for the beacon.
 *	The driver must also specify support for this with the extended
 *	features NL80211_EXT_FEATURE_BEACON_RATE_LEGACY,
 *	NL80211_EXT_FEATURE_BEACON_RATE_HT,
 *	NL80211_EXT_FEATURE_BEACON_RATE_VHT and
 *	NL80211_EXT_FEATURE_BEACON_RATE_HE.
 *
 * @NL80211_ATTR_FRAME_MATCH: A binary attribute which typically must contain
 *	at least one byte, currently used with @NL80211_CMD_REGISTER_FRAME.
 * @NL80211_ATTR_FRAME_TYPE: A u16 indicating the frame type/subtype for the
 *	@NL80211_CMD_REGISTER_FRAME command.
 * @NL80211_ATTR_TX_FRAME_TYPES: wiphy capability attribute, which is a
 *	nested attribute of %NL80211_ATTR_FRAME_TYPE attributes, containing
 *	information about which frame types can be transmitted with
 *	%NL80211_CMD_FRAME.
 * @NL80211_ATTR_RX_FRAME_TYPES: wiphy capability attribute, which is a
 *	nested attribute of %NL80211_ATTR_FRAME_TYPE attributes, containing
 *	information about which frame types can be registered for RX.
 *
 * @NL80211_ATTR_ACK: Flag attribute indicating that the frame was
 *	acknowledged by the recipient.
 *
 * @NL80211_ATTR_PS_STATE: powersave state, using &enum nl80211_ps_state values.
 *
 * @NL80211_ATTR_CQM: connection quality monitor configuration in a
 *	nested attribute with %NL80211_ATTR_CQM_* sub-attributes.
 *
 * @NL80211_ATTR_LOCAL_STATE_CHANGE: Flag attribute to indicate that a command
 *	is requesting a local authentication/association state change without
 *	invoking actual management frame exchange. This can be used with
 *	NL80211_CMD_AUTHENTICATE, NL80211_CMD_DEAUTHENTICATE,
 *	NL80211_CMD_DISASSOCIATE.
 *
 * @NL80211_ATTR_AP_ISOLATE: (AP mode) Do not forward traffic between stations
 *	connected to this BSS.
 *
 * @NL80211_ATTR_WIPHY_TX_POWER_SETTING: Transmit power setting type. See
 *      &enum nl80211_tx_power_setting for possible values.
 * @NL80211_ATTR_WIPHY_TX_POWER_LEVEL: Transmit power level in signed mBm units.
 *      This is used in association with @NL80211_ATTR_WIPHY_TX_POWER_SETTING
 *      for non-automatic settings.
 *
 * @NL80211_ATTR_SUPPORT_IBSS_RSN: The device supports IBSS RSN, which mostly
 *	means support for per-station GTKs.
 *
 * @NL80211_ATTR_WIPHY_ANTENNA_TX: Bitmap of allowed antennas for transmitting.
 *	This can be used to mask out antennas which are not attached or should
 *	not be used for transmitting. If an antenna is not selected in this
 *	bitmap the hardware is not allowed to transmit on this antenna.
 *
 *	Each bit represents one antenna, starting with antenna 1 at the first
 *	bit. Depending on which antennas are selected in the bitmap, 802.11n
 *	drivers can derive which chainmasks to use (if all antennas belonging to
 *	a particular chain are disabled this chain should be disabled) and if
 *	a chain has diversity antennas whether diversity should be used or not.
 *	HT capabilities (STBC, TX Beamforming, Antenna selection) can be
 *	derived from the available chains after applying the antenna mask.
 *	Non-802.11n drivers can derive whether to use diversity or not.
 *	Drivers may reject configurations or RX/TX mask combinations they cannot
 *	support by returning -EINVAL.
 *
 * @NL80211_ATTR_WIPHY_ANTENNA_RX: Bitmap of allowed antennas for receiving.
 *	This can be used to mask out antennas which are not attached or should
 *	not be used for receiving. If an antenna is not selected in this bitmap
 *	the hardware should not be configured to receive on this antenna.
 *	For a more detailed description see @NL80211_ATTR_WIPHY_ANTENNA_TX.
 *
 * @NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX: Bitmap of antennas which are available
 *	for configuration as TX antennas via the above parameters.
 *
 * @NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX: Bitmap of antennas which are available
 *	for configuration as RX antennas via the above parameters.
 *
 * @NL80211_ATTR_MCAST_RATE: Multicast tx rate (in 100 kbps) for IBSS
 *
 * @NL80211_ATTR_OFFCHANNEL_TX_OK: For management frame TX, the frame may be
 *	transmitted on another channel when the channel given doesn't match
 *	the current channel. If the current channel doesn't match and this
 *	flag isn't set, the frame will be rejected. This is also used as an
 *	nl80211 capability flag.
 *
 * @NL80211_ATTR_BSS_HT_OPMODE: HT operation mode (u16)
 *
 * @NL80211_ATTR_KEY_DEFAULT_TYPES: A nested attribute containing flags
 *	attributes, specifying what a key should be set as default as.
 *	See &enum nl80211_key_default_types.
 *
 * @NL80211_ATTR_MESH_SETUP: Optional mesh setup parameters.  These cannot be
 *	changed once the mesh is active.
 * @NL80211_ATTR_MESH_CONFIG: Mesh configuration parameters, a nested attribute
 *	containing attributes from &enum nl80211_meshconf_params.
 * @NL80211_ATTR_SUPPORT_MESH_AUTH: Currently, this means the underlying driver
 *	allows auth frames in a mesh to be passed to userspace for processing
 via *	the @NL80211_MESH_SETUP_USERSPACE_AUTH flag.
 * @NL80211_ATTR_STA_PLINK_STATE: The state of a mesh peer link as defined in
 *	&enum nl80211_plink_state. Used when userspace is driving the peer link
 *	management state machine.  @NL80211_MESH_SETUP_USERSPACE_AMPE or
 *	@NL80211_MESH_SETUP_USERSPACE_MPM must be enabled.
 *
 * @NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED: indicates, as part of the wiphy
 *	capabilities, the supported WoWLAN triggers
 * @NL80211_ATTR_WOWLAN_TRIGGERS: used by %NL80211_CMD_SET_WOWLAN to
 *	indicate which WoW triggers should be enabled. This is also
 *	used by %NL80211_CMD_GET_WOWLAN to get the currently enabled WoWLAN
 *	triggers.
 *
 * @NL80211_ATTR_SCHED_SCAN_INTERVAL: Interval between scheduled scan
 *	cycles, in msecs.
 *
 * @NL80211_ATTR_SCHED_SCAN_MATCH: Nested attribute with one or more
 *	sets of attributes to match during scheduled scans.  Only BSSs
 *	that match any of the sets will be reported.  These are
 *	pass-thru filter rules.
 *	For a match to succeed, the BSS must match all attributes of a
 *	set.  Since not every hardware supports matching all types of
 *	attributes, there is no guarantee that the reported BSSs are
 *	fully complying with the match sets and userspace needs to be
 *	able to ignore them by itself.
 *	Thus, the implementation is somewhat hardware-dependent, but
 *	this is only an optimization and the userspace application
 *	needs to handle all the non-filtered results anyway.
 *	If the match attributes don't make sense when combined with
 *	the values passed in @NL80211_ATTR_SCAN_SSIDS (eg. if an SSID
 *	is included in the probe request, but the match attributes
 *	will never let it go through), -EINVAL may be returned.
 *	If omitted, no filtering is done.
 *
 * @NL80211_ATTR_INTERFACE_COMBINATIONS: Nested attribute listing the supported
 *	interface combinations. In each nested item, it contains attributes
 *	defined in &enum nl80211_if_combination_attrs.
 * @NL80211_ATTR_SOFTWARE_IFTYPES: Nested attribute (just like
 *	%NL80211_ATTR_SUPPORTED_IFTYPES) containing the interface types that
 *	are managed in software: interfaces of these types aren't subject to
 *	any restrictions in their number or combinations.
 *
 * @NL80211_ATTR_REKEY_DATA: nested attribute containing the information
 *	necessary for GTK rekeying in the device, see &enum nl80211_rekey_data.
 *
 * @NL80211_ATTR_SCAN_SUPP_RATES: rates per to be advertised as supported in
 scan, *	nested array attribute containing an entry for each band, with
 the entry *	being a list of supported rates as defined by IEEE
 802.11 7.3.2.2 but *	without the length restriction (at most
 %NL80211_MAX_SUPP_RATES).
 *
 * @NL80211_ATTR_HIDDEN_SSID: indicates whether SSID is to be hidden from Beacon
 *	and Probe Response (when response to wildcard Probe Request); see
 *	&enum nl80211_hidden_ssid, represented as a u32
 *
 * @NL80211_ATTR_IE_PROBE_RESP: Information element(s) for Probe Response frame.
 *	This is used with %NL80211_CMD_NEW_BEACON and %NL80211_CMD_SET_BEACON to
 *	provide extra IEs (e.g., WPS/P2P IE) into Probe Response frames when the
 *	driver (or firmware) replies to Probe Request frames.
 * @NL80211_ATTR_IE_ASSOC_RESP: Information element(s) for (Re)Association
 *	Response frames. This is used with %NL80211_CMD_NEW_BEACON and
 *	%NL80211_CMD_SET_BEACON to provide extra IEs (e.g., WPS/P2P IE) into
 *	(Re)Association Response frames when the driver (or firmware) replies to
 *	(Re)Association Request frames.
 *
 * @NL80211_ATTR_STA_WME: Nested attribute containing the wme configuration
 *	of the station, see &enum nl80211_sta_wme_attr.
 * @NL80211_ATTR_SUPPORT_AP_UAPSD: the device supports uapsd when working
 *	as AP.
 *
 * @NL80211_ATTR_ROAM_SUPPORT: Indicates whether the firmware is capable of
 *	roaming to another AP in the same ESS if the signal lever is low.
 *
 * @NL80211_ATTR_PMKSA_CANDIDATE: Nested attribute containing the PMKSA caching
 *	candidate information, see &enum nl80211_pmksa_candidate_attr.
 *
 * @NL80211_ATTR_TX_NO_CCK_RATE: Indicates whether to use CCK rate or not
 *	for management frames transmission. In order to avoid p2p probe/action
 *	frames are being transmitted at CCK rate in 2GHz band, the user space
 *	applications use this attribute.
 *	This attribute is used with %NL80211_CMD_TRIGGER_SCAN and
 *	%NL80211_CMD_FRAME commands.
 *
 * @NL80211_ATTR_TDLS_ACTION: Low level TDLS action code (e.g. link setup
 *	request, link setup confirm, link teardown, etc.). Values are
 *	described in the TDLS (802.11z) specification.
 * @NL80211_ATTR_TDLS_DIALOG_TOKEN: Non-zero token for uniquely identifying a
 *	TDLS conversation between two devices.
 * @NL80211_ATTR_TDLS_OPERATION: High level TDLS operation; see
 *	&enum nl80211_tdls_operation, represented as a u8.
 * @NL80211_ATTR_TDLS_SUPPORT: A flag indicating the device can operate
 *	as a TDLS peer sta.
 * @NL80211_ATTR_TDLS_EXTERNAL_SETUP: The TDLS discovery/setup and teardown
 *	procedures should be performed by sending TDLS packets via
 *	%NL80211_CMD_TDLS_MGMT. Otherwise %NL80211_CMD_TDLS_OPER should be
 *	used for asking the driver to perform a TDLS operation.
 *
 * @NL80211_ATTR_DEVICE_AP_SME: This u32 attribute may be listed for devices
 *	that have AP support to indicate that they have the AP SME integrated
 *	with support for the features listed in this attribute, see
 *	&enum nl80211_ap_sme_features.
 *
 * @NL80211_ATTR_DONT_WAIT_FOR_ACK: Used with %NL80211_CMD_FRAME, this tells
 *	the driver to not wait for an acknowledgement. Note that due to this,
 *	it will also not give a status callback nor return a cookie. This is
 *	mostly useful for probe responses to save airtime.
 *
 * @NL80211_ATTR_FEATURE_FLAGS: This u32 attribute contains flags from
 *	&enum nl80211_feature_flags and is advertised in wiphy information.
 * @NL80211_ATTR_PROBE_RESP_OFFLOAD: Indicates that the HW responds to probe
 *	requests while operating in AP-mode.
 *	This attribute holds a bitmap of the supported protocols for
 *	offloading (see &enum nl80211_probe_resp_offload_support_attr).
 *
 * @NL80211_ATTR_PROBE_RESP: Probe Response template data. Contains the entire
 *	probe-response frame. The DA field in the 802.11 header is zero-ed out,
 *	to be filled by the FW.
 * @NL80211_ATTR_DISABLE_HT: Force HT capable interfaces to disable
 *      this feature during association. This is a flag attribute.
 *	Currently only supported in mac80211 drivers.
 * @NL80211_ATTR_DISABLE_VHT: Force VHT capable interfaces to disable
 *      this feature during association. This is a flag attribute.
 *	Currently only supported in mac80211 drivers.
 * @NL80211_ATTR_DISABLE_HE: Force HE capable interfaces to disable
 *      this feature during association. This is a flag attribute.
 *	Currently only supported in mac80211 drivers.
 * @NL80211_ATTR_HT_CAPABILITY_MASK: Specify which bits of the
 *      ATTR_HT_CAPABILITY to which attention should be paid.
 *      Currently, only mac80211 NICs support this feature.
 *      The values that may be configured are:
 *       MCS rates, MAX-AMSDU, HT-20-40 and HT_CAP_SGI_40
 *       AMPDU density and AMPDU factor.
 *      All values are treated as suggestions and may be ignored
 *      by the driver as required.  The actual values may be seen in
 *      the station debugfs ht_caps file.
 *
 * @NL80211_ATTR_DFS_REGION: region for regulatory rules which this country
 *    abides to when initiating radiation on DFS channels. A country maps
 *    to one DFS region.
 *
 * @NL80211_ATTR_NOACK_MAP: This u16 bitmap contains the No Ack Policy of
 *      up to 16 TIDs.
 *
 * @NL80211_ATTR_INACTIVITY_TIMEOUT: timeout value in seconds, this can be
 *	used by the drivers which has MLME in firmware and does not have support
 *	to report per station tx/rx activity to free up the station entry from
 *	the list. This needs to be used when the driver advertises the
 *	capability to timeout the stations.
 *
 * @NL80211_ATTR_RX_SIGNAL_DBM: signal strength in dBm (as a 32-bit int);
 *	this attribute is (depending on the driver capabilities) added to
 *	received frames indicated with %NL80211_CMD_FRAME.
 *
 * @NL80211_ATTR_BG_SCAN_PERIOD: Background scan period in seconds
 *      or 0 to disable background scan.
 *
 * @NL80211_ATTR_USER_REG_HINT_TYPE: type of regulatory hint passed from
 *	userspace. If unset it is assumed the hint comes directly from
 *	a user. If set code could specify exactly what type of source
 *	was used to provide the hint. For the different types of
 *	allowed user regulatory hints see nl80211_user_reg_hint_type.
 *
 * @NL80211_ATTR_CONN_FAILED_REASON: The reason for which AP has rejected
 *	the connection request from a station. nl80211_connect_failed_reason
 *	enum has different reasons of connection failure.
 *
 * @NL80211_ATTR_AUTH_DATA: Fields and elements in Authentication frames.
 *	This contains the authentication frame body (non-IE and IE data),
 *	excluding the Authentication algorithm number, i.e., starting at the
 *	Authentication transaction sequence number field. It is used with
 *	authentication algorithms that need special fields to be added into
 *	the frames (SAE and FILS). Currently, only the SAE cases use the
 *	initial two fields (Authentication transaction sequence number and
 *	Status code). However, those fields are included in the attribute data
 *	for all authentication algorithms to keep the attribute definition
 *	consistent.
 *
 * @NL80211_ATTR_VHT_CAPABILITY: VHT Capability information element (from
 *	association request when used with NL80211_CMD_NEW_STATION)
 *
 * @NL80211_ATTR_SCAN_FLAGS: scan request control flags (u32)
 *
 * @NL80211_ATTR_P2P_CTWINDOW: P2P GO Client Traffic Window (u8), used with
 *	the START_AP and SET_BSS commands
 * @NL80211_ATTR_P2P_OPPPS: P2P GO opportunistic PS (u8), used with the
 *	START_AP and SET_BSS commands. This can have the values 0 or 1;
 *	if not given in START_AP 0 is assumed, if not given in SET_BSS
 *	no change is made.
 *
 * @NL80211_ATTR_LOCAL_MESH_POWER_MODE: local mesh STA link-specific power mode
 *	defined in &enum nl80211_mesh_power_mode.
 *
 * @NL80211_ATTR_ACL_POLICY: ACL policy, see &enum nl80211_acl_policy,
 *	carried in a u32 attribute
 *
 * @NL80211_ATTR_MAC_ADDRS: Array of nested MAC addresses, used for
 *	MAC ACL.
 *
 * @NL80211_ATTR_MAC_ACL_MAX: u32 attribute to advertise the maximum
 *	number of MAC addresses that a device can support for MAC
 *	ACL.
 *
 * @NL80211_ATTR_RADAR_EVENT: Type of radar event for notification to userspace,
 *	contains a value of enum nl80211_radar_event (u32).
 *
 * @NL80211_ATTR_EXT_CAPA: 802.11 extended capabilities that the kernel driver
 *	has and handles. The format is the same as the IE contents. See
 *	802.11-2012 8.4.2.29 for more information.
 * @NL80211_ATTR_EXT_CAPA_MASK: Extended capabilities that the kernel driver
 *	has set in the %NL80211_ATTR_EXT_CAPA value, for multibit fields.
 *
 * @NL80211_ATTR_STA_CAPABILITY: Station capabilities (u16) are advertised to
 *	the driver, e.g., to enable TDLS power save (PU-APSD).
 *
 * @NL80211_ATTR_STA_EXT_CAPABILITY: Station extended capabilities are
 *	advertised to the driver, e.g., to enable TDLS off channel operations
 *	and PU-APSD.
 *
 * @NL80211_ATTR_PROTOCOL_FEATURES: global nl80211 feature flags, see
 *	&enum nl80211_protocol_features, the attribute is a u32.
 *
 * @NL80211_ATTR_SPLIT_WIPHY_DUMP: flag attribute, userspace supports
 *	receiving the data for a single wiphy split across multiple
 *	messages, given with wiphy dump message
 *
 * @NL80211_ATTR_MDID: Mobility Domain Identifier
 *
 * @NL80211_ATTR_IE_RIC: Resource Information Container Information
 *	Element
 *
 * @NL80211_ATTR_CRIT_PROT_ID: critical protocol identifier requiring increased
 *	reliability, see &enum nl80211_crit_proto_id (u16).
 * @NL80211_ATTR_MAX_CRIT_PROT_DURATION: duration in milliseconds in which
 *      the connection should have increased reliability (u16).
 *
 * @NL80211_ATTR_PEER_AID: Association ID for the peer TDLS station (u16).
 *	This is similar to @NL80211_ATTR_STA_AID but with a difference of being
 *	allowed to be used with the first @NL80211_CMD_SET_STATION command to
 *	update a TDLS peer STA entry.
 *
 * @NL80211_ATTR_COALESCE_RULE: Coalesce rule information.
 *
 * @NL80211_ATTR_CH_SWITCH_COUNT: u32 attribute specifying the number of TBTT's
 *	until the channel switch event.
 * @NL80211_ATTR_CH_SWITCH_BLOCK_TX: flag attribute specifying that transmission
 *	must be blocked on the current channel (before the channel switch
 *	operation). Also included in the channel switch started event if quiet
 *	was requested by the AP.
 * @NL80211_ATTR_CSA_IES: Nested set of attributes containing the IE information
 *	for the time while performing a channel switch.
 * @NL80211_ATTR_CNTDWN_OFFS_BEACON: An array of offsets (u16) to the channel
 *	switch or color change counters in the beacons tail
 (%NL80211_ATTR_BEACON_TAIL).
 * @NL80211_ATTR_CNTDWN_OFFS_PRESP: An array of offsets (u16) to the channel
 *	switch or color change counters in the probe response
 (%NL80211_ATTR_PROBE_RESP).
 *
 * @NL80211_ATTR_RXMGMT_FLAGS: flags for nl80211_send_mgmt(), u32.
 *	As specified in the &enum nl80211_rxmgmt_flags.
 *
 * @NL80211_ATTR_STA_SUPPORTED_CHANNELS: array of supported channels.
 *
 * @NL80211_ATTR_STA_SUPPORTED_OPER_CLASSES: array of supported
 *      operating classes.
 *
 * @NL80211_ATTR_HANDLE_DFS: A flag indicating whether user space
 *	controls DFS operation in IBSS mode. If the flag is included in
 *	%NL80211_CMD_JOIN_IBSS request, the driver will allow use of DFS
 *	channels and reports radar events to userspace. Userspace is required
 *	to react to radar events, e.g. initiate a channel switch or leave the
 *	IBSS network.
 *
 * @NL80211_ATTR_SUPPORT_5_MHZ: A flag indicating that the device supports
 *	5 MHz channel bandwidth.
 * @NL80211_ATTR_SUPPORT_10_MHZ: A flag indicating that the device supports
 *	10 MHz channel bandwidth.
 *
 * @NL80211_ATTR_OPMODE_NOTIF: Operating mode field from Operating Mode
 *	Notification Element based on association request when used with
 *	%NL80211_CMD_NEW_STATION or %NL80211_CMD_SET_STATION (only when
 *	%NL80211_FEATURE_FULL_AP_CLIENT_STATE is supported, or with TDLS);
 *	u8 attribute.
 *
 * @NL80211_ATTR_VENDOR_ID: The vendor ID, either a 24-bit OUI or, if
 *	%NL80211_VENDOR_ID_IS_LINUX is set, a special Linux ID (not used yet)
 * @NL80211_ATTR_VENDOR_SUBCMD: vendor sub-command
 * @NL80211_ATTR_VENDOR_DATA: data for the vendor command, if any; this
 *	attribute is also used for vendor command feature advertisement
 * @NL80211_ATTR_VENDOR_EVENTS: used for event list advertising in the wiphy
 *	info, containing a nested array of possible events
 *
 * @NL80211_ATTR_QOS_MAP: IP DSCP mapping for Interworking QoS mapping. This
 *	data is in the format defined for the payload of the QoS Map Set element
 *	in IEEE Std 802.11-2012, 8.4.2.97.
 *
 * @NL80211_ATTR_MAC_HINT: MAC address recommendation as initial BSS
 * @NL80211_ATTR_WIPHY_FREQ_HINT: frequency of the recommended initial BSS
 *
 * @NL80211_ATTR_MAX_AP_ASSOC_STA: Device attribute that indicates how many
 *	associated stations are supported in AP mode (including P2P GO); u32.
 *	Since drivers may not have a fixed limit on the maximum number (e.g.,
 *	other concurrent operations may affect this), drivers are allowed to
 *	advertise values that cannot always be met. In such cases, an attempt
 *	to add a new station entry with @NL80211_CMD_NEW_STATION may fail.
 *
 * @NL80211_ATTR_CSA_C_OFFSETS_TX: An array of csa counter offsets (u16) which
 *	should be updated when the frame is transmitted.
 * @NL80211_ATTR_MAX_CSA_COUNTERS: U8 attribute used to advertise the maximum
 *	supported number of csa counters.
 *
 * @NL80211_ATTR_TDLS_PEER_CAPABILITY: flags for TDLS peer capabilities, u32.
 *	As specified in the &enum nl80211_tdls_peer_capability.
 *
 * @NL80211_ATTR_SOCKET_OWNER: Flag attribute, if set during interface
 *	creation then the new interface will be owned by the netlink socket
 *	that created it and will be destroyed when the socket is closed.
 *	If set during scheduled scan start then the new scan req will be
 *	owned by the netlink socket that created it and the scheduled scan will
 *	be stopped when the socket is closed.
 *	If set during configuration of regulatory indoor operation then the
 *	regulatory indoor configuration would be owned by the netlink socket
 *	that configured the indoor setting, and the indoor operation would be
 *	cleared when the socket is closed.
 *	If set during NAN interface creation, the interface will be destroyed
 *	if the socket is closed just like any other interface. Moreover, NAN
 *	notifications will be sent in unicast to that socket. Without this
 *	attribute, the notifications will be sent to the %NL80211_MCGRP_NAN
 *	multicast group.
 *	If set during %NL80211_CMD_ASSOCIATE or %NL80211_CMD_CONNECT the
 *	station will deauthenticate when the socket is closed.
 *	If set during %NL80211_CMD_JOIN_IBSS the IBSS will be automatically
 *	torn down when the socket is closed.
 *	If set during %NL80211_CMD_JOIN_MESH the mesh setup will be
 *	automatically torn down when the socket is closed.
 *	If set during %NL80211_CMD_START_AP the AP will be automatically
 *	disabled when the socket is closed.
 *
 * @NL80211_ATTR_TDLS_INITIATOR: flag attribute indicating the current end is
 *	the TDLS link initiator.
 *
 * @NL80211_ATTR_USE_RRM: flag for indicating whether the current connection
 *	shall support Radio Resource Measurements (11k). This attribute can be
 *	used with %NL80211_CMD_ASSOCIATE and %NL80211_CMD_CONNECT requests.
 *	User space applications are expected to use this flag only if the
 *	underlying device supports these minimal RRM features:
 *		%NL80211_FEATURE_DS_PARAM_SET_IE_IN_PROBES,
 *		%NL80211_FEATURE_QUIET,
 *	Or, if global RRM is supported, see:
 *		%NL80211_EXT_FEATURE_RRM
 *	If this flag is used, driver must add the Power Capabilities IE to the
 *	association request. In addition, it must also set the RRM capability
 *	flag in the association request's Capability Info field.
 *
 * @NL80211_ATTR_WIPHY_DYN_ACK: flag attribute used to enable ACK timeout
 *	estimation algorithm (dynack). In order to activate dynack
 *	%NL80211_FEATURE_ACKTO_ESTIMATION feature flag must be set by lower
 *	drivers to indicate dynack capability. Dynack is automatically disabled
 *	setting valid value for coverage class.
 *
 * @NL80211_ATTR_TSID: a TSID value (u8 attribute)
 * @NL80211_ATTR_USER_PRIO: user priority value (u8 attribute)
 * @NL80211_ATTR_ADMITTED_TIME: admitted time in units of 32 microseconds
 *	(per second) (u16 attribute)
 *
 * @NL80211_ATTR_SMPS_MODE: SMPS mode to use (ap mode). see
 *	&enum nl80211_smps_mode.
 *
 * @NL80211_ATTR_OPER_CLASS: operating class
 *
 * @NL80211_ATTR_MAC_MASK: MAC address mask
 *
 * @NL80211_ATTR_WIPHY_SELF_MANAGED_REG: flag attribute indicating this device
 *	is self-managing its regulatory information and any regulatory domain
 *	obtained from it is coming from the device's wiphy and not the global
 *	cfg80211 regdomain.
 *
 * @NL80211_ATTR_EXT_FEATURES: extended feature flags contained in a byte
 *	array. The feature flags are identified by their bit index (see &enum
 *	nl80211_ext_feature_index). The bit index is ordered starting at the
 *	least-significant bit of the first byte in the array, ie. bit index 0
 *	is located at bit 0 of byte 0. bit index 25 would be located at bit 1
 *	of byte 3 (u8 array).
 *
 * @NL80211_ATTR_SURVEY_RADIO_STATS: Request overall radio statistics to be
 *	returned along with other survey data. If set, @NL80211_CMD_GET_SURVEY
 *	may return a survey entry without a channel indicating global radio
 *	statistics (only some values are valid and make sense.)
 *	For devices that don't return such an entry even then, the information
 *	should be contained in the result as the sum of the respective counters
 *	over all channels.
 *
 * @NL80211_ATTR_SCHED_SCAN_DELAY: delay before the first cycle of a
 *	scheduled scan is started.  Or the delay before a WoWLAN
 *	net-detect scan is started, counting from the moment the
 *	system is suspended.  This value is a u32, in seconds.

 * @NL80211_ATTR_REG_INDOOR: flag attribute, if set indicates that the device
 *      is operating in an indoor environment.
 *
 * @NL80211_ATTR_MAX_NUM_SCHED_SCAN_PLANS: maximum number of scan plans for
 *	scheduled scan supported by the device (u32), a wiphy attribute.
 * @NL80211_ATTR_MAX_SCAN_PLAN_INTERVAL: maximum interval (in seconds) for
 *	a scan plan (u32), a wiphy attribute.
 * @NL80211_ATTR_MAX_SCAN_PLAN_ITERATIONS: maximum number of iterations in
 *	a scan plan (u32), a wiphy attribute.
 * @NL80211_ATTR_SCHED_SCAN_PLANS: a list of scan plans for scheduled scan.
 *	Each scan plan defines the number of scan iterations and the interval
 *	between scans. The last scan plan will always run infinitely,
 *	thus it must not specify the number of iterations, only the interval
 *	between scans. The scan plans are executed sequentially.
 *	Each scan plan is a nested attribute of &enum nl80211_sched_scan_plan.
 * @NL80211_ATTR_PBSS: flag attribute. If set it means operate
 *	in a PBSS. Specified in %NL80211_CMD_CONNECT to request
 *	connecting to a PCP, and in %NL80211_CMD_START_AP to start
 *	a PCP instead of AP. Relevant for DMG networks only.
 * @NL80211_ATTR_BSS_SELECT: nested attribute for driver supporting the
 *	BSS selection feature. When used with %NL80211_CMD_GET_WIPHY it contains
 *	attributes according &enum nl80211_bss_select_attr to indicate what
 *	BSS selection behaviours are supported. When used with
 %NL80211_CMD_CONNECT *	it contains the behaviour-specific attribute containing
 the parameters for *	BSS selection to be done by driver and/or firmware.
 *
 * @NL80211_ATTR_STA_SUPPORT_P2P_PS: whether P2P PS mechanism supported
 *	or not. u8, one of the values of &enum nl80211_sta_p2p_ps_status
 *
 * @NL80211_ATTR_PAD: attribute used for padding for 64-bit alignment
 *
 * @NL80211_ATTR_IFTYPE_EXT_CAPA: Nested attribute of the following attributes:
 *	%NL80211_ATTR_IFTYPE, %NL80211_ATTR_EXT_CAPA,
 *	%NL80211_ATTR_EXT_CAPA_MASK, to specify the extended capabilities and
 *	other interface-type specific capabilities per interface type. For MLO,
 *	%NL80211_ATTR_EML_CAPABILITY and %NL80211_ATTR_MLD_CAPA_AND_OPS are
 *	present.
 *
 * @NL80211_ATTR_MU_MIMO_GROUP_DATA: array of 24 bytes that defines a MU-MIMO
 *	groupID for monitor mode.
 *	The first 8 bytes are a mask that defines the membership in each
 *	group (there are 64 groups, group 0 and 63 are reserved),
 *	each bit represents a group and set to 1 for being a member in
 *	that group and 0 for not being a member.
 *	The remaining 16 bytes define the position in each group: 2 bits for
 *	each group.
 *	(smaller group numbers represented on most significant bits and bigger
 *	group numbers on least significant bits.)
 *	This attribute is used only if all interfaces are in monitor mode.
 *	Set this attribute in order to monitor packets using the given MU-MIMO
 *	groupID data.
 *	to turn off that feature set all the bits of the groupID to zero.
 * @NL80211_ATTR_MU_MIMO_FOLLOW_MAC_ADDR: mac address for the sniffer to follow
 *	when using MU-MIMO air sniffer.
 *	to turn that feature off set an invalid mac address
 *	(e.g. FF:FF:FF:FF:FF:FF)
 *
 * @NL80211_ATTR_SCAN_START_TIME_TSF: The time at which the scan was actually
 *	started (u64). The time is the TSF of the BSS the interface that
 *	requested the scan is connected to (if available, otherwise this
 *	attribute must not be included).
 * @NL80211_ATTR_SCAN_START_TIME_TSF_BSSID: The BSS according to which
 *	%NL80211_ATTR_SCAN_START_TIME_TSF is set.
 * @NL80211_ATTR_MEASUREMENT_DURATION: measurement duration in TUs (u16). If
 *	%NL80211_ATTR_MEASUREMENT_DURATION_MANDATORY is not set, this is the
 *	maximum measurement duration allowed. This attribute is used with
 *	measurement requests. It can also be used with %NL80211_CMD_TRIGGER_SCAN
 *	if the scan is used for beacon report radio measurement.
 * @NL80211_ATTR_MEASUREMENT_DURATION_MANDATORY: flag attribute that indicates
 *	that the duration specified with %NL80211_ATTR_MEASUREMENT_DURATION is
 *	mandatory. If this flag is not set, the duration is the maximum duration
 *	and the actual measurement duration may be shorter.
 *
 * @NL80211_ATTR_MESH_PEER_AID: Association ID for the mesh peer (u16). This is
 *	used to pull the stored data for mesh peer in power save state.
 *
 * @NL80211_ATTR_NAN_MASTER_PREF: the master preference to be used by
 *	%NL80211_CMD_START_NAN and optionally with
 *	%NL80211_CMD_CHANGE_NAN_CONFIG. Its type is u8 and it can't be 0.
 *	Also, values 1 and 255 are reserved for certification purposes and
 *	should not be used during a normal device operation.
 * @NL80211_ATTR_BANDS: operating bands configuration.  This is a u32
 *	bitmask of BIT(NL80211_BAND_*) as described in %enum
 *	nl80211_band.  For instance, for NL80211_BAND_2GHZ, bit 0
 *	would be set.  This attribute is used with
 *	%NL80211_CMD_START_NAN and %NL80211_CMD_CHANGE_NAN_CONFIG, and
 *	it is optional.  If no bands are set, it means don't-care and
 *	the device will decide what to use.
 * @NL80211_ATTR_NAN_FUNC: a function that can be added to NAN. See
 *	&enum nl80211_nan_func_attributes for description of this nested
 *	attribute.
 * @NL80211_ATTR_NAN_MATCH: used to report a match. This is a nested attribute.
 *	See &enum nl80211_nan_match_attributes.
 * @NL80211_ATTR_FILS_KEK: KEK for FILS (Re)Association Request/Response frame
 *	protection.
 * @NL80211_ATTR_FILS_NONCES: Nonces (part of AAD) for FILS (Re)Association
 *	Request/Response frame protection. This attribute contains the 16 octet
 *	STA Nonce followed by 16 octets of AP Nonce.
 *
 * @NL80211_ATTR_MULTICAST_TO_UNICAST_ENABLED: Indicates whether or not
 multicast *	packets should be send out as unicast to all stations (flag
 attribute).
 *
 * @NL80211_ATTR_BSSID: The BSSID of the AP. Note that %NL80211_ATTR_MAC is also
 *	used in various commands/events for specifying the BSSID.
 *
 * @NL80211_ATTR_SCHED_SCAN_RELATIVE_RSSI: Relative RSSI threshold by which
 *	other BSSs has to be better or slightly worse than the current
 *	connected BSS so that they get reported to user space.
 *	This will give an opportunity to userspace to consider connecting to
 *	other matching BSSs which have better or slightly worse RSSI than
 *	the current connected BSS by using an offloaded operation to avoid
 *	unnecessary wakeups.
 *
 * @NL80211_ATTR_SCHED_SCAN_RSSI_ADJUST: When present the RSSI level for BSSs in
 *	the specified band is to be adjusted before doing
 *	%NL80211_ATTR_SCHED_SCAN_RELATIVE_RSSI based comparison to figure out
 *	better BSSs. The attribute value is a packed structure
 *	value as specified by &struct nl80211_bss_select_rssi_adjust.
 *
 * @NL80211_ATTR_TIMEOUT_REASON: The reason for which an operation timed out.
 *	u32 attribute with an &enum nl80211_timeout_reason value. This is used,
 *	e.g., with %NL80211_CMD_CONNECT event.
 *
 * @NL80211_ATTR_FILS_ERP_USERNAME: EAP Re-authentication Protocol (ERP)
 *	username part of NAI used to refer keys rRK and rIK. This is used with
 *	%NL80211_CMD_CONNECT.
 *
 * @NL80211_ATTR_FILS_ERP_REALM: EAP Re-authentication Protocol (ERP) realm part
 *	of NAI specifying the domain name of the ER server. This is used with
 *	%NL80211_CMD_CONNECT.
 *
 * @NL80211_ATTR_FILS_ERP_NEXT_SEQ_NUM: Unsigned 16-bit ERP next sequence number
 *	to use in ERP messages. This is used in generating the FILS wrapped data
 *	for FILS authentication and is used with %NL80211_CMD_CONNECT.
 *
 * @NL80211_ATTR_FILS_ERP_RRK: ERP re-authentication Root Key (rRK) for the
 *	NAI specified by %NL80211_ATTR_FILS_ERP_USERNAME and
 *	%NL80211_ATTR_FILS_ERP_REALM. This is used for generating rIK and rMSK
 *	from successful FILS authentication and is used with
 *	%NL80211_CMD_CONNECT.
 *
 * @NL80211_ATTR_FILS_CACHE_ID: A 2-octet identifier advertised by a FILS AP
 *	identifying the scope of PMKSAs. This is used with
 *	@NL80211_CMD_SET_PMKSA and @NL80211_CMD_DEL_PMKSA.
 *
 * @NL80211_ATTR_PMK: attribute for passing PMK key material. Used with
 *	%NL80211_CMD_SET_PMKSA for the PMKSA identified by %NL80211_ATTR_PMKID.
 *	For %NL80211_CMD_CONNECT and %NL80211_CMD_START_AP it is used to provide
 *	PSK for offloading 4-way handshake for WPA/WPA2-PSK networks. For 802.1X
 *	authentication it is used with %NL80211_CMD_SET_PMK. For offloaded FT
 *	support this attribute specifies the PMK-R0 if NL80211_ATTR_PMKR0_NAME
 *	is included as well.
 *
 * @NL80211_ATTR_SCHED_SCAN_MULTI: flag attribute which user-space shall use to
 *	indicate that it supports multiple active scheduled scan requests.
 * @NL80211_ATTR_SCHED_SCAN_MAX_REQS: indicates maximum number of scheduled
 *	scan request that may be active for the device (u32).
 *
 * @NL80211_ATTR_WANT_1X_4WAY_HS: flag attribute which user-space can include
 *	in %NL80211_CMD_CONNECT to indicate that for 802.1X authentication it
 *	wants to use the supported offload of the 4-way handshake.
 * @NL80211_ATTR_PMKR0_NAME: PMK-R0 Name for offloaded FT.
 * @NL80211_ATTR_PORT_AUTHORIZED: (reserved)
 *
 * @NL80211_ATTR_EXTERNAL_AUTH_ACTION: Identify the requested external
 *     authentication operation (u32 attribute with an
 *     &enum nl80211_external_auth_action value). This is used with the
 *     %NL80211_CMD_EXTERNAL_AUTH request event.
 * @NL80211_ATTR_EXTERNAL_AUTH_SUPPORT: Flag attribute indicating that the user
 *	space supports external authentication. This attribute shall be used
 *	with %NL80211_CMD_CONNECT and %NL80211_CMD_START_AP request. The driver
 *	may offload authentication processing to user space if this capability
 *	is indicated in the respective requests from the user space. (This flag
 *	attribute deprecated for %NL80211_CMD_START_AP, use
 *	%NL80211_ATTR_AP_SETTINGS_FLAGS)
 *
 * @NL80211_ATTR_NSS: Station's New/updated  RX_NSS value notified using this
 *	u8 attribute. This is used with %NL80211_CMD_STA_OPMODE_CHANGED.
 *
 * @NL80211_ATTR_TXQ_STATS: TXQ statistics (nested attribute, see &enum
 *      nl80211_txq_stats)
 * @NL80211_ATTR_TXQ_LIMIT: Total packet limit for the TXQ queues for this phy.
 *      The smaller of this and the memory limit is enforced.
 * @NL80211_ATTR_TXQ_MEMORY_LIMIT: Total memory limit (in bytes) for the
 *      TXQ queues for this phy. The smaller of this and the packet limit is
 *      enforced.
 * @NL80211_ATTR_TXQ_QUANTUM: TXQ scheduler quantum (bytes). Number of bytes
 *      a flow is assigned on each round of the DRR scheduler.
 * @NL80211_ATTR_HE_CAPABILITY: HE Capability information element (from
 *	association request when used with NL80211_CMD_NEW_STATION). Can be set
 *	only if %NL80211_STA_FLAG_WME is set.
 *
 * @NL80211_ATTR_FTM_RESPONDER: nested attribute which user-space can include
 *	in %NL80211_CMD_START_AP or %NL80211_CMD_SET_BEACON for fine timing
 *	measurement (FTM) responder functionality and containing parameters as
 *	possible, see &enum nl80211_ftm_responder_attr
 *
 * @NL80211_ATTR_FTM_RESPONDER_STATS: Nested attribute with FTM responder
 *	statistics, see &enum nl80211_ftm_responder_stats.
 *
 * @NL80211_ATTR_TIMEOUT: Timeout for the given operation in milliseconds (u32),
 *	if the attribute is not given no timeout is requested. Note that 0 is an
 *	invalid value.
 *
 * @NL80211_ATTR_PEER_MEASUREMENTS: peer measurements request (and result)
 *	data, uses nested attributes specified in
 *	&enum nl80211_peer_measurement_attrs.
 *	This is also used for capability advertisement in the wiphy information,
 *	with the appropriate sub-attributes.
 *
 * @NL80211_ATTR_AIRTIME_WEIGHT: Station's weight when scheduled by the airtime
 *	scheduler.
 *
 * @NL80211_ATTR_STA_TX_POWER_SETTING: Transmit power setting type (u8) for
 *	station associated with the AP. See &enum nl80211_tx_power_setting for
 *	possible values.
 * @NL80211_ATTR_STA_TX_POWER: Transmit power level (s16) in dBm units. This
 *	allows to set Tx power for a station. If this attribute is not included,
 *	the default per-interface tx power setting will be overriding. Driver
 *	should be picking up the lowest tx power, either tx power per-interface
 *	or per-station.
 *
 * @NL80211_ATTR_SAE_PASSWORD: attribute for passing SAE password material. It
 *	is used with %NL80211_CMD_CONNECT to provide password for offloading
 *	SAE authentication for WPA3-Personal networks.
 *
 * @NL80211_ATTR_TWT_RESPONDER: Enable target wait time responder support.
 *
 * @NL80211_ATTR_HE_OBSS_PD: nested attribute for OBSS Packet Detection
 *	functionality.
 *
 * @NL80211_ATTR_WIPHY_EDMG_CHANNELS: bitmap that indicates the 2.16 GHz
 *	channel(s) that are allowed to be used for EDMG transmissions.
 *	Defined by IEEE P802.11ay/D4.0 section 9.4.2.251. (u8 attribute)
 * @NL80211_ATTR_WIPHY_EDMG_BW_CONFIG: Channel BW Configuration subfield encodes
 *	the allowed channel bandwidth configurations. (u8 attribute)
 *	Defined by IEEE P802.11ay/D4.0 section 9.4.2.251, Table 13.
 *
 * @NL80211_ATTR_VLAN_ID: VLAN ID (1..4094) for the station and VLAN group key
 *	(u16).
 *
 * @NL80211_ATTR_HE_BSS_COLOR: nested attribute for BSS Color Settings.
 *
 * @NL80211_ATTR_IFTYPE_AKM_SUITES: nested array attribute, with each entry
 *	using attributes from &enum nl80211_iftype_akm_attributes. This
 *	attribute is sent in a response to %NL80211_CMD_GET_WIPHY indicating
 *	supported AKM suites capability per interface. AKMs advertised in
 *	%NL80211_ATTR_AKM_SUITES are default capabilities if AKM suites not
 *	advertised for a specific interface type.
 *
 * @NL80211_ATTR_TID_CONFIG: TID specific configuration in a
 *	nested attribute with &enum nl80211_tid_config_attr sub-attributes;
 *	on output (in wiphy attributes) it contains only the feature sub-
 *	attributes.
 *
 * @NL80211_ATTR_CONTROL_PORT_NO_PREAUTH: disable preauth frame rx on control
 *	port in order to forward/receive them as ordinary data frames.
 *
 * @NL80211_ATTR_PMK_LIFETIME: Maximum lifetime for PMKSA in seconds (u32,
 *	dot11RSNAConfigPMKReauthThreshold; 0 is not a valid value).
 *	An optional parameter configured through %NL80211_CMD_SET_PMKSA.
 *	Drivers that trigger roaming need to know the lifetime of the
 *	configured PMKSA for triggering the full vs. PMKSA caching based
 *	authentication. This timeout helps authentication methods like SAE,
 *	where PMK gets updated only by going through a full (new SAE)
 *	authentication instead of getting updated during an association for EAP
 *	authentication. No new full authentication within the PMK expiry shall
 *	result in a disassociation at the end of the lifetime.
 *
 * @NL80211_ATTR_PMK_REAUTH_THRESHOLD: Reauthentication threshold time, in
 *	terms of percentage of %NL80211_ATTR_PMK_LIFETIME
 *	(u8, dot11RSNAConfigPMKReauthThreshold, 1..100). This is an optional
 *	parameter configured through %NL80211_CMD_SET_PMKSA. Requests the
 *	driver to trigger a full authentication roam (without PMKSA caching)
 *	after the reauthentication threshold time, but before the PMK lifetime
 *	has expired.
 *
 *	Authentication methods like SAE need to be able to generate a new PMKSA
 *	entry without having to force a disconnection after the PMK timeout. If
 *	no roaming occurs between the reauth threshold and PMK expiration,
 *	disassociation is still forced.
 * @NL80211_ATTR_RECEIVE_MULTICAST: multicast flag for the
 *	%NL80211_CMD_REGISTER_FRAME command, see the description there.
 * @NL80211_ATTR_WIPHY_FREQ_OFFSET: offset of the associated
 *	%NL80211_ATTR_WIPHY_FREQ in positive KHz. Only valid when supplied with
 *	an %NL80211_ATTR_WIPHY_FREQ_OFFSET.
 * @NL80211_ATTR_CENTER_FREQ1_OFFSET: Center frequency offset in KHz for the
 *	first channel segment specified in %NL80211_ATTR_CENTER_FREQ1.
 * @NL80211_ATTR_SCAN_FREQ_KHZ: nested attribute with KHz frequencies
 *
 * @NL80211_ATTR_HE_6GHZ_CAPABILITY: HE 6 GHz Band Capability element (from
 *	association request when used with NL80211_CMD_NEW_STATION).
 *
 * @NL80211_ATTR_FILS_DISCOVERY: Optional parameter to configure FILS
 *	discovery. It is a nested attribute, see
 *	&enum nl80211_fils_discovery_attributes. Userspace should pass an empty
 *	nested attribute to disable this feature and delete the templates.
 *
 * @NL80211_ATTR_UNSOL_BCAST_PROBE_RESP: Optional parameter to configure
 *	unsolicited broadcast probe response. It is a nested attribute, see
 *	&enum nl80211_unsol_bcast_probe_resp_attributes. Userspace should pass
 an empty *	nested attribute to disable this feature and delete the
 templates.
 *
 * @NL80211_ATTR_S1G_CAPABILITY: S1G Capability information element (from
 *	association request when used with NL80211_CMD_NEW_STATION)
 * @NL80211_ATTR_S1G_CAPABILITY_MASK: S1G Capability Information element
 *	override mask. Used with NL80211_ATTR_S1G_CAPABILITY in
 *	NL80211_CMD_ASSOCIATE or NL80211_CMD_CONNECT.
 *
 * @NL80211_ATTR_SAE_PWE: Indicates the mechanism(s) allowed for SAE PWE
 *	derivation in WPA3-Personal networks which are using SAE authentication.
 *	This is a u8 attribute that encapsulates one of the values from
 *	&enum nl80211_sae_pwe_mechanism.
 *
 * @NL80211_ATTR_SAR_SPEC: SAR power limitation specification when
 *	used with %NL80211_CMD_SET_SAR_SPECS. The message contains fields
 *	of %nl80211_sar_attrs which specifies the sar type and related
 *	sar specs. Sar specs contains array of %nl80211_sar_specs_attrs.
 *
 * @NL80211_ATTR_RECONNECT_REQUESTED: flag attribute, used with deauth and
 *	disassoc events to indicate that an immediate reconnect to the AP
 *	is desired.
 *
 * @NL80211_ATTR_OBSS_COLOR_BITMAP: bitmap of the u64 BSS colors for the
 *	%NL80211_CMD_OBSS_COLOR_COLLISION event.
 *
 * @NL80211_ATTR_COLOR_CHANGE_COUNT: u8 attribute specifying the number of
 TBTT's *	until the color switch event.
 * @NL80211_ATTR_COLOR_CHANGE_COLOR: u8 attribute specifying the color that we
 are *	switching to
 * @NL80211_ATTR_COLOR_CHANGE_ELEMS: Nested set of attributes containing the IE
 *	information for the time while performing a color switch.
 *
 * @NL80211_ATTR_MBSSID_CONFIG: Nested attribute for multiple BSSID
 *	advertisements (MBSSID) parameters in AP mode.
 *	Kernel uses this attribute to indicate the driver's support for MBSSID
 *	and enhanced multi-BSSID advertisements (EMA AP) to the userspace.
 *	Userspace should use this attribute to configure per interface MBSSID
 *	parameters.
 *	See &enum nl80211_mbssid_config_attributes for details.
 *
 * @NL80211_ATTR_MBSSID_ELEMS: Nested parameter to pass multiple BSSID elements.
 *	Mandatory parameter for the transmitting interface to enable MBSSID.
 *	Optional for the non-transmitting interfaces.
 *
 * @NL80211_ATTR_RADAR_BACKGROUND: Configure dedicated offchannel chain
 *	available for radar/CAC detection on some hw. This chain can't be used
 *	to transmit or receive frames and it is bounded to a running wdev.
 *	Background radar/CAC detection allows to avoid the CAC downtime
 *	switching on a different channel during CAC detection on the selected
 *	radar channel.
 *
 * @NL80211_ATTR_AP_SETTINGS_FLAGS: u32 attribute contains ap settings flags,
 *	enumerated in &enum nl80211_ap_settings_flags. This attribute shall be
 *	used with %NL80211_CMD_START_AP request.
 *
 * @NL80211_ATTR_EHT_CAPABILITY: EHT Capability information element (from
 *	association request when used with NL80211_CMD_NEW_STATION). Can be set
 *	only if %NL80211_STA_FLAG_WME is set.
 *
 * @NL80211_ATTR_MLO_LINK_ID: A (u8) link ID for use with MLO, to be used with
 *	various commands that need a link ID to operate.
 * @NL80211_ATTR_MLO_LINKS: A nested array of links, each containing some
 *	per-link information and a link ID.
 * @NL80211_ATTR_MLD_ADDR: An MLD address, used with various commands such as
 *	authenticate/associate.
 *
 * @NL80211_ATTR_MLO_SUPPORT: Flag attribute to indicate user space supports MLO
 *	connection. Used with %NL80211_CMD_CONNECT. If this attribute is not
 *	included in NL80211_CMD_CONNECT drivers must not perform MLO connection.
 *
 * @NL80211_ATTR_MAX_NUM_AKM_SUITES: U16 attribute. Indicates maximum number of
 *	AKM suites allowed for %NL80211_CMD_CONNECT, %NL80211_CMD_ASSOCIATE and
 *	%NL80211_CMD_START_AP in %NL80211_CMD_GET_WIPHY response. If this
 *	attribute is not present userspace shall consider maximum number of AKM
 *	suites allowed as %NL80211_MAX_NR_AKM_SUITES which is the legacy maximum
 *	number prior to the introduction of this attribute.
 *
 * @NL80211_ATTR_EML_CAPABILITY: EML Capability information (u16)
 * @NL80211_ATTR_MLD_CAPA_AND_OPS: MLD Capabilities and Operations (u16)
 *
 * @NL80211_ATTR_TX_HW_TIMESTAMP: Hardware timestamp for TX operation in
 *	nanoseconds (u64). This is the device clock timestamp so it will
 *	probably reset when the device is stopped or the firmware is reset.
 *	When used with %NL80211_CMD_FRAME_TX_STATUS, indicates the frame TX
 *	timestamp. When used with %NL80211_CMD_FRAME RX notification, indicates
 *	the ack TX timestamp.
 * @NL80211_ATTR_RX_HW_TIMESTAMP: Hardware timestamp for RX operation in
 *	nanoseconds (u64). This is the device clock timestamp so it will
 *	probably reset when the device is stopped or the firmware is reset.
 *	When used with %NL80211_CMD_FRAME_TX_STATUS, indicates the ack RX
 *	timestamp. When used with %NL80211_CMD_FRAME RX notification, indicates
 *	the incoming frame RX timestamp.
 * @NL80211_ATTR_TD_BITMAP: Transition Disable bitmap, for subsequent
 *	(re)associations.
 *
 * @NL80211_ATTR_PUNCT_BITMAP: (u32) Preamble puncturing bitmap, lowest
 *	bit corresponds to the lowest 20 MHz channel. Each bit set to 1
 *	indicates that the sub-channel is punctured. Higher 16 bits are
 *	reserved.
 *
 * @NL80211_ATTR_MAX_HW_TIMESTAMP_PEERS: Maximum number of peers that HW
 *	timestamping can be enabled for concurrently (u16), a wiphy attribute.
 *	A value of 0xffff indicates setting for all peers (i.e. not specifying
 *	an address with %NL80211_CMD_SET_HW_TIMESTAMP) is supported.
 * @NL80211_ATTR_HW_TIMESTAMP_ENABLED: Indicates whether HW timestamping should
 *	be enabled or not (flag attribute).
 *
 * @NL80211_ATTR_EMA_RNR_ELEMS: Optional nested attribute for
 *	reduced neighbor report (RNR) elements. This attribute can be used
 *	only when NL80211_MBSSID_CONFIG_ATTR_EMA is enabled.
 *	Userspace is responsible for splitting the RNR into multiple
 *	elements such that each element excludes the non-transmitting
 *	profiles already included in the MBSSID element
 *	(%NL80211_ATTR_MBSSID_ELEMS) at the same index. Each EMA beacon
 *	will be generated by adding MBSSID and RNR elements at the same
 *	index. If the userspace includes more RNR elements than number of
 *	MBSSID elements then these will be added in every EMA beacon.
 *
 * @NL80211_ATTR_MLO_LINK_DISABLED: Flag attribute indicating that the link is
 *	disabled.
 *
 * @NL80211_ATTR_BSS_DUMP_INCLUDE_USE_DATA: Include BSS usage data, i.e.
 *	include BSSes that can only be used in restricted scenarios and/or
 *	cannot be used at all.
 *
 * @NL80211_ATTR_MLO_TTLM_DLINK: Binary attribute specifying the downlink TID to
 *      link mapping. The length is 8 * sizeof(u16). For each TID the link
 *      mapping is as defined in section 9.4.2.314 (TID-To-Link Mapping element)
 *      in Draft P802.11be_D4.0.
 * @NL80211_ATTR_MLO_TTLM_ULINK: Binary attribute specifying the uplink TID to
 *      link mapping. The length is 8 * sizeof(u16). For each TID the link
 *      mapping is as defined in section 9.4.2.314 (TID-To-Link Mapping element)
 *      in Draft P802.11be_D4.0.
 *
 * @NL80211_ATTR_ASSOC_SPP_AMSDU: flag attribute used with
 *	%NL80211_CMD_ASSOCIATE indicating the SPP A-MSDUs
 *	are used on this connection
 *
 * @NUM_NL80211_ATTR: total number of nl80211_attrs available
 * @NL80211_ATTR_MAX: highest attribute number currently defined
 * @__NL80211_ATTR_AFTER_LAST: internal use
 */
enum nl80211_attrs {
  /* don't change the order or add anything between, this is ABI! */
  NL80211_ATTR_UNSPEC,

  NL80211_ATTR_WIPHY,
  NL80211_ATTR_WIPHY_NAME,

  NL80211_ATTR_IFINDEX,
  NL80211_ATTR_IFNAME,
  NL80211_ATTR_IFTYPE,

  NL80211_ATTR_MAC,

  NL80211_ATTR_KEY_DATA,
  NL80211_ATTR_KEY_IDX,
  NL80211_ATTR_KEY_CIPHER,
  NL80211_ATTR_KEY_SEQ,
  NL80211_ATTR_KEY_DEFAULT,

  NL80211_ATTR_BEACON_INTERVAL,
  NL80211_ATTR_DTIM_PERIOD,
  NL80211_ATTR_BEACON_HEAD,
  NL80211_ATTR_BEACON_TAIL,

  NL80211_ATTR_STA_AID,
  NL80211_ATTR_STA_FLAGS,
  NL80211_ATTR_STA_LISTEN_INTERVAL,
  NL80211_ATTR_STA_SUPPORTED_RATES,
  NL80211_ATTR_STA_VLAN,
  NL80211_ATTR_STA_INFO,

  NL80211_ATTR_WIPHY_BANDS,

  NL80211_ATTR_MNTR_FLAGS,

  NL80211_ATTR_MESH_ID,
  NL80211_ATTR_STA_PLINK_ACTION,
  NL80211_ATTR_MPATH_NEXT_HOP,
  NL80211_ATTR_MPATH_INFO,

  NL80211_ATTR_BSS_CTS_PROT,
  NL80211_ATTR_BSS_SHORT_PREAMBLE,
  NL80211_ATTR_BSS_SHORT_SLOT_TIME,

  NL80211_ATTR_HT_CAPABILITY,

  NL80211_ATTR_SUPPORTED_IFTYPES,

  NL80211_ATTR_REG_ALPHA2,
  NL80211_ATTR_REG_RULES,

  NL80211_ATTR_MESH_CONFIG,

  NL80211_ATTR_BSS_BASIC_RATES,

  NL80211_ATTR_WIPHY_TXQ_PARAMS,
  NL80211_ATTR_WIPHY_FREQ,
  NL80211_ATTR_WIPHY_CHANNEL_TYPE,

  NL80211_ATTR_KEY_DEFAULT_MGMT,

  NL80211_ATTR_MGMT_SUBTYPE,
  NL80211_ATTR_IE,

  NL80211_ATTR_MAX_NUM_SCAN_SSIDS,

  NL80211_ATTR_SCAN_FREQUENCIES,
  NL80211_ATTR_SCAN_SSIDS,
  NL80211_ATTR_GENERATION, /* replaces old SCAN_GENERATION */
  NL80211_ATTR_BSS,

  NL80211_ATTR_REG_INITIATOR,
  NL80211_ATTR_REG_TYPE,

  NL80211_ATTR_SUPPORTED_COMMANDS,

  NL80211_ATTR_FRAME,
  NL80211_ATTR_SSID,
  NL80211_ATTR_AUTH_TYPE,
  NL80211_ATTR_REASON_CODE,

  NL80211_ATTR_KEY_TYPE,

  NL80211_ATTR_MAX_SCAN_IE_LEN,
  NL80211_ATTR_CIPHER_SUITES,

  NL80211_ATTR_FREQ_BEFORE,
  NL80211_ATTR_FREQ_AFTER,

  NL80211_ATTR_FREQ_FIXED,

  NL80211_ATTR_WIPHY_RETRY_SHORT,
  NL80211_ATTR_WIPHY_RETRY_LONG,
  NL80211_ATTR_WIPHY_FRAG_THRESHOLD,
  NL80211_ATTR_WIPHY_RTS_THRESHOLD,

  NL80211_ATTR_TIMED_OUT,

  NL80211_ATTR_USE_MFP,

  NL80211_ATTR_STA_FLAGS2,

  NL80211_ATTR_CONTROL_PORT,

  NL80211_ATTR_TESTDATA,

  NL80211_ATTR_PRIVACY,

  NL80211_ATTR_DISCONNECTED_BY_AP,
  NL80211_ATTR_STATUS_CODE,

  NL80211_ATTR_CIPHER_SUITES_PAIRWISE,
  NL80211_ATTR_CIPHER_SUITE_GROUP,
  NL80211_ATTR_WPA_VERSIONS,
  NL80211_ATTR_AKM_SUITES,

  NL80211_ATTR_REQ_IE,
  NL80211_ATTR_RESP_IE,

  NL80211_ATTR_PREV_BSSID,

  NL80211_ATTR_KEY,
  NL80211_ATTR_KEYS,

  NL80211_ATTR_PID,

  NL80211_ATTR_4ADDR,

  NL80211_ATTR_SURVEY_INFO,

  NL80211_ATTR_PMKID,
  NL80211_ATTR_MAX_NUM_PMKIDS,

  NL80211_ATTR_DURATION,

  NL80211_ATTR_COOKIE,

  NL80211_ATTR_WIPHY_COVERAGE_CLASS,

  NL80211_ATTR_TX_RATES,

  NL80211_ATTR_FRAME_MATCH,

  NL80211_ATTR_ACK,

  NL80211_ATTR_PS_STATE,

  NL80211_ATTR_CQM,

  NL80211_ATTR_LOCAL_STATE_CHANGE,

  NL80211_ATTR_AP_ISOLATE,

  NL80211_ATTR_WIPHY_TX_POWER_SETTING,
  NL80211_ATTR_WIPHY_TX_POWER_LEVEL,

  NL80211_ATTR_TX_FRAME_TYPES,
  NL80211_ATTR_RX_FRAME_TYPES,
  NL80211_ATTR_FRAME_TYPE,

  NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
  NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT,

  NL80211_ATTR_SUPPORT_IBSS_RSN,

  NL80211_ATTR_WIPHY_ANTENNA_TX,
  NL80211_ATTR_WIPHY_ANTENNA_RX,

  NL80211_ATTR_MCAST_RATE,

  NL80211_ATTR_OFFCHANNEL_TX_OK,

  NL80211_ATTR_BSS_HT_OPMODE,

  NL80211_ATTR_KEY_DEFAULT_TYPES,

  NL80211_ATTR_MAX_REMAIN_ON_CHANNEL_DURATION,

  NL80211_ATTR_MESH_SETUP,

  NL80211_ATTR_WIPHY_ANTENNA_AVAIL_TX,
  NL80211_ATTR_WIPHY_ANTENNA_AVAIL_RX,

  NL80211_ATTR_SUPPORT_MESH_AUTH,
  NL80211_ATTR_STA_PLINK_STATE,

  NL80211_ATTR_WOWLAN_TRIGGERS,
  NL80211_ATTR_WOWLAN_TRIGGERS_SUPPORTED,

  NL80211_ATTR_SCHED_SCAN_INTERVAL,

  NL80211_ATTR_INTERFACE_COMBINATIONS,
  NL80211_ATTR_SOFTWARE_IFTYPES,

  NL80211_ATTR_REKEY_DATA,

  NL80211_ATTR_MAX_NUM_SCHED_SCAN_SSIDS,
  NL80211_ATTR_MAX_SCHED_SCAN_IE_LEN,

  NL80211_ATTR_SCAN_SUPP_RATES,

  NL80211_ATTR_HIDDEN_SSID,

  NL80211_ATTR_IE_PROBE_RESP,
  NL80211_ATTR_IE_ASSOC_RESP,

  NL80211_ATTR_STA_WME,
  NL80211_ATTR_SUPPORT_AP_UAPSD,

  NL80211_ATTR_ROAM_SUPPORT,

  NL80211_ATTR_SCHED_SCAN_MATCH,
  NL80211_ATTR_MAX_MATCH_SETS,

  NL80211_ATTR_PMKSA_CANDIDATE,

  NL80211_ATTR_TX_NO_CCK_RATE,

  NL80211_ATTR_TDLS_ACTION,
  NL80211_ATTR_TDLS_DIALOG_TOKEN,
  NL80211_ATTR_TDLS_OPERATION,
  NL80211_ATTR_TDLS_SUPPORT,
  NL80211_ATTR_TDLS_EXTERNAL_SETUP,

  NL80211_ATTR_DEVICE_AP_SME,

  NL80211_ATTR_DONT_WAIT_FOR_ACK,

  NL80211_ATTR_FEATURE_FLAGS,

  NL80211_ATTR_PROBE_RESP_OFFLOAD,

  NL80211_ATTR_PROBE_RESP,

  NL80211_ATTR_DFS_REGION,

  NL80211_ATTR_DISABLE_HT,
  NL80211_ATTR_HT_CAPABILITY_MASK,

  NL80211_ATTR_NOACK_MAP,

  NL80211_ATTR_INACTIVITY_TIMEOUT,

  NL80211_ATTR_RX_SIGNAL_DBM,

  NL80211_ATTR_BG_SCAN_PERIOD,

  NL80211_ATTR_WDEV,

  NL80211_ATTR_USER_REG_HINT_TYPE,

  NL80211_ATTR_CONN_FAILED_REASON,

  NL80211_ATTR_AUTH_DATA,

  NL80211_ATTR_VHT_CAPABILITY,

  NL80211_ATTR_SCAN_FLAGS,

  NL80211_ATTR_CHANNEL_WIDTH,
  NL80211_ATTR_CENTER_FREQ1,
  NL80211_ATTR_CENTER_FREQ2,

  NL80211_ATTR_P2P_CTWINDOW,
  NL80211_ATTR_P2P_OPPPS,

  NL80211_ATTR_LOCAL_MESH_POWER_MODE,

  NL80211_ATTR_ACL_POLICY,

  NL80211_ATTR_MAC_ADDRS,

  NL80211_ATTR_MAC_ACL_MAX,

  NL80211_ATTR_RADAR_EVENT,

  NL80211_ATTR_EXT_CAPA,
  NL80211_ATTR_EXT_CAPA_MASK,

  NL80211_ATTR_STA_CAPABILITY,
  NL80211_ATTR_STA_EXT_CAPABILITY,

  NL80211_ATTR_PROTOCOL_FEATURES,
  NL80211_ATTR_SPLIT_WIPHY_DUMP,

  NL80211_ATTR_DISABLE_VHT,
  NL80211_ATTR_VHT_CAPABILITY_MASK,

  NL80211_ATTR_MDID,
  NL80211_ATTR_IE_RIC,

  NL80211_ATTR_CRIT_PROT_ID,
  NL80211_ATTR_MAX_CRIT_PROT_DURATION,

  NL80211_ATTR_PEER_AID,

  NL80211_ATTR_COALESCE_RULE,

  NL80211_ATTR_CH_SWITCH_COUNT,
  NL80211_ATTR_CH_SWITCH_BLOCK_TX,
  NL80211_ATTR_CSA_IES,
  NL80211_ATTR_CNTDWN_OFFS_BEACON,
  NL80211_ATTR_CNTDWN_OFFS_PRESP,

  NL80211_ATTR_RXMGMT_FLAGS,

  NL80211_ATTR_STA_SUPPORTED_CHANNELS,

  NL80211_ATTR_STA_SUPPORTED_OPER_CLASSES,

  NL80211_ATTR_HANDLE_DFS,

  NL80211_ATTR_SUPPORT_5_MHZ,
  NL80211_ATTR_SUPPORT_10_MHZ,

  NL80211_ATTR_OPMODE_NOTIF,

  NL80211_ATTR_VENDOR_ID,
  NL80211_ATTR_VENDOR_SUBCMD,
  NL80211_ATTR_VENDOR_DATA,
  NL80211_ATTR_VENDOR_EVENTS,

  NL80211_ATTR_QOS_MAP,

  NL80211_ATTR_MAC_HINT,
  NL80211_ATTR_WIPHY_FREQ_HINT,

  NL80211_ATTR_MAX_AP_ASSOC_STA,

  NL80211_ATTR_TDLS_PEER_CAPABILITY,

  NL80211_ATTR_SOCKET_OWNER,

  NL80211_ATTR_CSA_C_OFFSETS_TX,
  NL80211_ATTR_MAX_CSA_COUNTERS,

  NL80211_ATTR_TDLS_INITIATOR,

  NL80211_ATTR_USE_RRM,

  NL80211_ATTR_WIPHY_DYN_ACK,

  NL80211_ATTR_TSID,
  NL80211_ATTR_USER_PRIO,
  NL80211_ATTR_ADMITTED_TIME,

  NL80211_ATTR_SMPS_MODE,

  NL80211_ATTR_OPER_CLASS,

  NL80211_ATTR_MAC_MASK,

  NL80211_ATTR_WIPHY_SELF_MANAGED_REG,

  NL80211_ATTR_EXT_FEATURES,

  NL80211_ATTR_SURVEY_RADIO_STATS,

  NL80211_ATTR_NETNS_FD,

  NL80211_ATTR_SCHED_SCAN_DELAY,

  NL80211_ATTR_REG_INDOOR,

  NL80211_ATTR_MAX_NUM_SCHED_SCAN_PLANS,
  NL80211_ATTR_MAX_SCAN_PLAN_INTERVAL,
  NL80211_ATTR_MAX_SCAN_PLAN_ITERATIONS,
  NL80211_ATTR_SCHED_SCAN_PLANS,

  NL80211_ATTR_PBSS,

  NL80211_ATTR_BSS_SELECT,

  NL80211_ATTR_STA_SUPPORT_P2P_PS,

  NL80211_ATTR_PAD,

  NL80211_ATTR_IFTYPE_EXT_CAPA,

  NL80211_ATTR_MU_MIMO_GROUP_DATA,
  NL80211_ATTR_MU_MIMO_FOLLOW_MAC_ADDR,

  NL80211_ATTR_SCAN_START_TIME_TSF,
  NL80211_ATTR_SCAN_START_TIME_TSF_BSSID,
  NL80211_ATTR_MEASUREMENT_DURATION,
  NL80211_ATTR_MEASUREMENT_DURATION_MANDATORY,

  NL80211_ATTR_MESH_PEER_AID,

  NL80211_ATTR_NAN_MASTER_PREF,
  NL80211_ATTR_BANDS,
  NL80211_ATTR_NAN_FUNC,
  NL80211_ATTR_NAN_MATCH,

  NL80211_ATTR_FILS_KEK,
  NL80211_ATTR_FILS_NONCES,

  NL80211_ATTR_MULTICAST_TO_UNICAST_ENABLED,

  NL80211_ATTR_BSSID,

  NL80211_ATTR_SCHED_SCAN_RELATIVE_RSSI,
  NL80211_ATTR_SCHED_SCAN_RSSI_ADJUST,

  NL80211_ATTR_TIMEOUT_REASON,

  NL80211_ATTR_FILS_ERP_USERNAME,
  NL80211_ATTR_FILS_ERP_REALM,
  NL80211_ATTR_FILS_ERP_NEXT_SEQ_NUM,
  NL80211_ATTR_FILS_ERP_RRK,
  NL80211_ATTR_FILS_CACHE_ID,

  NL80211_ATTR_PMK,

  NL80211_ATTR_SCHED_SCAN_MULTI,
  NL80211_ATTR_SCHED_SCAN_MAX_REQS,

  NL80211_ATTR_WANT_1X_4WAY_HS,
  NL80211_ATTR_PMKR0_NAME,
  NL80211_ATTR_PORT_AUTHORIZED,

  NL80211_ATTR_EXTERNAL_AUTH_ACTION,
  NL80211_ATTR_EXTERNAL_AUTH_SUPPORT,

  NL80211_ATTR_NSS,
  NL80211_ATTR_ACK_SIGNAL,

  NL80211_ATTR_CONTROL_PORT_OVER_NL80211,

  NL80211_ATTR_TXQ_STATS,
  NL80211_ATTR_TXQ_LIMIT,
  NL80211_ATTR_TXQ_MEMORY_LIMIT,
  NL80211_ATTR_TXQ_QUANTUM,

  NL80211_ATTR_HE_CAPABILITY,

  NL80211_ATTR_FTM_RESPONDER,

  NL80211_ATTR_FTM_RESPONDER_STATS,

  NL80211_ATTR_TIMEOUT,

  NL80211_ATTR_PEER_MEASUREMENTS,

  NL80211_ATTR_AIRTIME_WEIGHT,
  NL80211_ATTR_STA_TX_POWER_SETTING,
  NL80211_ATTR_STA_TX_POWER,

  NL80211_ATTR_SAE_PASSWORD,

  NL80211_ATTR_TWT_RESPONDER,

  NL80211_ATTR_HE_OBSS_PD,

  NL80211_ATTR_WIPHY_EDMG_CHANNELS,
  NL80211_ATTR_WIPHY_EDMG_BW_CONFIG,

  NL80211_ATTR_VLAN_ID,

  NL80211_ATTR_HE_BSS_COLOR,

  NL80211_ATTR_IFTYPE_AKM_SUITES,

  NL80211_ATTR_TID_CONFIG,

  NL80211_ATTR_CONTROL_PORT_NO_PREAUTH,

  NL80211_ATTR_PMK_LIFETIME,
  NL80211_ATTR_PMK_REAUTH_THRESHOLD,

  NL80211_ATTR_RECEIVE_MULTICAST,
  NL80211_ATTR_WIPHY_FREQ_OFFSET,
  NL80211_ATTR_CENTER_FREQ1_OFFSET,
  NL80211_ATTR_SCAN_FREQ_KHZ,

  NL80211_ATTR_HE_6GHZ_CAPABILITY,

  NL80211_ATTR_FILS_DISCOVERY,

  NL80211_ATTR_UNSOL_BCAST_PROBE_RESP,

  NL80211_ATTR_S1G_CAPABILITY,
  NL80211_ATTR_S1G_CAPABILITY_MASK,

  NL80211_ATTR_SAE_PWE,

  NL80211_ATTR_RECONNECT_REQUESTED,

  NL80211_ATTR_SAR_SPEC,

  NL80211_ATTR_DISABLE_HE,

  NL80211_ATTR_OBSS_COLOR_BITMAP,

  NL80211_ATTR_COLOR_CHANGE_COUNT,
  NL80211_ATTR_COLOR_CHANGE_COLOR,
  NL80211_ATTR_COLOR_CHANGE_ELEMS,

  NL80211_ATTR_MBSSID_CONFIG,
  NL80211_ATTR_MBSSID_ELEMS,

  NL80211_ATTR_RADAR_BACKGROUND,

  NL80211_ATTR_AP_SETTINGS_FLAGS,

  NL80211_ATTR_EHT_CAPABILITY,

  NL80211_ATTR_DISABLE_EHT,

  NL80211_ATTR_MLO_LINKS,
  NL80211_ATTR_MLO_LINK_ID,
  NL80211_ATTR_MLD_ADDR,

  NL80211_ATTR_MLO_SUPPORT,

  NL80211_ATTR_MAX_NUM_AKM_SUITES,

  NL80211_ATTR_EML_CAPABILITY,
  NL80211_ATTR_MLD_CAPA_AND_OPS,

  NL80211_ATTR_TX_HW_TIMESTAMP,
  NL80211_ATTR_RX_HW_TIMESTAMP,
  NL80211_ATTR_TD_BITMAP,

  NL80211_ATTR_PUNCT_BITMAP,

  NL80211_ATTR_MAX_HW_TIMESTAMP_PEERS,
  NL80211_ATTR_HW_TIMESTAMP_ENABLED,

  NL80211_ATTR_EMA_RNR_ELEMS,

  NL80211_ATTR_MLO_LINK_DISABLED,

  NL80211_ATTR_BSS_DUMP_INCLUDE_USE_DATA,

  NL80211_ATTR_MLO_TTLM_DLINK,
  NL80211_ATTR_MLO_TTLM_ULINK,

  NL80211_ATTR_ASSOC_SPP_AMSDU,

  /* add attributes here, update the policy in nl80211.c */

  __NL80211_ATTR_AFTER_LAST,
  NUM_NL80211_ATTR = __NL80211_ATTR_AFTER_LAST,
  NL80211_ATTR_MAX = __NL80211_ATTR_AFTER_LAST - 1
};

/**
 * enum nl80211_rate_info - bitrate information
 *
 * These attribute types are used with %NL80211_STA_INFO_TXRATE
 * when getting information about the bitrate of a station.
 * There are 2 attributes for bitrate, a legacy one that represents
 * a 16-bit value, and new one that represents a 32-bit value.
 * If the rate value fits into 16 bit, both attributes are reported
 * with the same value. If the rate is too high to fit into 16 bits
 * (>6.5535Gbps) only 32-bit attribute is included.
 * User space tools encouraged to use the 32-bit attribute and fall
 * back to the 16-bit one for compatibility with older kernels.
 *
 * @__NL80211_RATE_INFO_INVALID: attribute number 0 is reserved
 * @NL80211_RATE_INFO_BITRATE: total bitrate (u16, 100kbit/s)
 * @NL80211_RATE_INFO_MCS: mcs index for 802.11n (u8)
 * @NL80211_RATE_INFO_40_MHZ_WIDTH: 40 MHz dualchannel bitrate
 * @NL80211_RATE_INFO_SHORT_GI: 400ns guard interval
 * @NL80211_RATE_INFO_BITRATE32: total bitrate (u32, 100kbit/s)
 * @NL80211_RATE_INFO_MAX: highest rate_info number currently defined
 * @NL80211_RATE_INFO_VHT_MCS: MCS index for VHT (u8)
 * @NL80211_RATE_INFO_VHT_NSS: number of streams in VHT (u8)
 * @NL80211_RATE_INFO_80_MHZ_WIDTH: 80 MHz VHT rate
 * @NL80211_RATE_INFO_80P80_MHZ_WIDTH: unused - 80+80 is treated the
 *	same as 160 for purposes of the bitrates
 * @NL80211_RATE_INFO_160_MHZ_WIDTH: 160 MHz VHT rate
 * @NL80211_RATE_INFO_10_MHZ_WIDTH: 10 MHz width - note that this is
 *	a legacy rate and will be reported as the actual bitrate, i.e.
 *	half the base (20 MHz) rate
 * @NL80211_RATE_INFO_5_MHZ_WIDTH: 5 MHz width - note that this is
 *	a legacy rate and will be reported as the actual bitrate, i.e.
 *	a quarter of the base (20 MHz) rate
 * @NL80211_RATE_INFO_HE_MCS: HE MCS index (u8, 0-11)
 * @NL80211_RATE_INFO_HE_NSS: HE NSS value (u8, 1-8)
 * @NL80211_RATE_INFO_HE_GI: HE guard interval identifier
 *	(u8, see &enum nl80211_he_gi)
 * @NL80211_RATE_INFO_HE_DCM: HE DCM value (u8, 0/1)
 * @NL80211_RATE_INFO_RU_ALLOC: HE RU allocation, if not present then
 *	non-OFDMA was used (u8, see &enum nl80211_he_ru_alloc)
 * @NL80211_RATE_INFO_320_MHZ_WIDTH: 320 MHz bitrate
 * @NL80211_RATE_INFO_EHT_MCS: EHT MCS index (u8, 0-15)
 * @NL80211_RATE_INFO_EHT_NSS: EHT NSS value (u8, 1-8)
 * @NL80211_RATE_INFO_EHT_GI: EHT guard interval identifier
 *	(u8, see &enum nl80211_eht_gi)
 * @NL80211_RATE_INFO_EHT_RU_ALLOC: EHT RU allocation, if not present then
 *	non-OFDMA was used (u8, see &enum nl80211_eht_ru_alloc)
 * @NL80211_RATE_INFO_S1G_MCS: S1G MCS index (u8, 0-10)
 * @NL80211_RATE_INFO_S1G_NSS: S1G NSS value (u8, 1-4)
 * @NL80211_RATE_INFO_1_MHZ_WIDTH: 1 MHz S1G rate
 * @NL80211_RATE_INFO_2_MHZ_WIDTH: 2 MHz S1G rate
 * @NL80211_RATE_INFO_4_MHZ_WIDTH: 4 MHz S1G rate
 * @NL80211_RATE_INFO_8_MHZ_WIDTH: 8 MHz S1G rate
 * @NL80211_RATE_INFO_16_MHZ_WIDTH: 16 MHz S1G rate
 * @__NL80211_RATE_INFO_AFTER_LAST: internal use
 */
enum nl80211_rate_info {
  __NL80211_RATE_INFO_INVALID,
  NL80211_RATE_INFO_BITRATE,
  NL80211_RATE_INFO_MCS,
  NL80211_RATE_INFO_40_MHZ_WIDTH,
  NL80211_RATE_INFO_SHORT_GI,
  NL80211_RATE_INFO_BITRATE32,
  NL80211_RATE_INFO_VHT_MCS,
  NL80211_RATE_INFO_VHT_NSS,
  NL80211_RATE_INFO_80_MHZ_WIDTH,
  NL80211_RATE_INFO_80P80_MHZ_WIDTH,
  NL80211_RATE_INFO_160_MHZ_WIDTH,
  NL80211_RATE_INFO_10_MHZ_WIDTH,
  NL80211_RATE_INFO_5_MHZ_WIDTH,
  NL80211_RATE_INFO_HE_MCS,
  NL80211_RATE_INFO_HE_NSS,
  NL80211_RATE_INFO_HE_GI,
  NL80211_RATE_INFO_HE_DCM,
  NL80211_RATE_INFO_HE_RU_ALLOC,
  NL80211_RATE_INFO_320_MHZ_WIDTH,
  NL80211_RATE_INFO_EHT_MCS,
  NL80211_RATE_INFO_EHT_NSS,
  NL80211_RATE_INFO_EHT_GI,
  NL80211_RATE_INFO_EHT_RU_ALLOC,
  NL80211_RATE_INFO_S1G_MCS,
  NL80211_RATE_INFO_S1G_NSS,
  NL80211_RATE_INFO_1_MHZ_WIDTH,
  NL80211_RATE_INFO_2_MHZ_WIDTH,
  NL80211_RATE_INFO_4_MHZ_WIDTH,
  NL80211_RATE_INFO_8_MHZ_WIDTH,
  NL80211_RATE_INFO_16_MHZ_WIDTH,

  /* keep last */
  __NL80211_RATE_INFO_AFTER_LAST,
  NL80211_RATE_INFO_MAX = __NL80211_RATE_INFO_AFTER_LAST - 1
};

/**
 * enum nl80211_sta_info - station information
 *
 * These attribute types are used with %NL80211_ATTR_STA_INFO
 * when getting information about a station.
 *
 * @__NL80211_STA_INFO_INVALID: attribute number 0 is reserved
 * @NL80211_STA_INFO_INACTIVE_TIME: time since last activity (u32, msecs)
 * @NL80211_STA_INFO_RX_BYTES: total received bytes (MPDU length)
 *	(u32, from this station)
 * @NL80211_STA_INFO_TX_BYTES: total transmitted bytes (MPDU length)
 *	(u32, to this station)
 * @NL80211_STA_INFO_RX_BYTES64: total received bytes (MPDU length)
 *	(u64, from this station)
 * @NL80211_STA_INFO_TX_BYTES64: total transmitted bytes (MPDU length)
 *	(u64, to this station)
 * @NL80211_STA_INFO_SIGNAL: signal strength of last received PPDU (u8, dBm)
 * @NL80211_STA_INFO_TX_BITRATE: current unicast tx rate, nested attribute
 * 	containing info as possible, see &enum nl80211_rate_info
 * @NL80211_STA_INFO_RX_PACKETS: total received packet (MSDUs and MMPDUs)
 *	(u32, from this station)
 * @NL80211_STA_INFO_TX_PACKETS: total transmitted packets (MSDUs and MMPDUs)
 *	(u32, to this station)
 * @NL80211_STA_INFO_TX_RETRIES: total retries (MPDUs) (u32, to this station)
 * @NL80211_STA_INFO_TX_FAILED: total failed packets (MPDUs)
 *	(u32, to this station)
 * @NL80211_STA_INFO_SIGNAL_AVG: signal strength average (u8, dBm)
 * @NL80211_STA_INFO_LLID: the station's mesh LLID
 * @NL80211_STA_INFO_PLID: the station's mesh PLID
 * @NL80211_STA_INFO_PLINK_STATE: peer link state for the station
 *	(see %enum nl80211_plink_state)
 * @NL80211_STA_INFO_RX_BITRATE: last unicast data frame rx rate, nested
 *	attribute, like NL80211_STA_INFO_TX_BITRATE.
 * @NL80211_STA_INFO_BSS_PARAM: current station's view of BSS, nested attribute
 *     containing info as possible, see &enum nl80211_sta_bss_param
 * @NL80211_STA_INFO_CONNECTED_TIME: time since the station is last connected
 * @NL80211_STA_INFO_STA_FLAGS: Contains a struct nl80211_sta_flag_update.
 * @NL80211_STA_INFO_BEACON_LOSS: count of times beacon loss was detected (u32)
 * @NL80211_STA_INFO_T_OFFSET: timing offset with respect to this STA (s64)
 * @NL80211_STA_INFO_LOCAL_PM: local mesh STA link-specific power mode
 * @NL80211_STA_INFO_PEER_PM: peer mesh STA link-specific power mode
 * @NL80211_STA_INFO_NONPEER_PM: neighbor mesh STA power save mode towards
 *	non-peer STA
 * @NL80211_STA_INFO_CHAIN_SIGNAL: per-chain signal strength of last PPDU
 *	Contains a nested array of signal strength attributes (u8, dBm)
 * @NL80211_STA_INFO_CHAIN_SIGNAL_AVG: per-chain signal strength average
 *	Same format as NL80211_STA_INFO_CHAIN_SIGNAL.
 * @NL80211_STA_EXPECTED_THROUGHPUT: expected throughput considering also the
 *	802.11 header (u32, kbps)
 * @NL80211_STA_INFO_RX_DROP_MISC: RX packets dropped for unspecified reasons
 *	(u64)
 * @NL80211_STA_INFO_BEACON_RX: number of beacons received from this peer (u64)
 * @NL80211_STA_INFO_BEACON_SIGNAL_AVG: signal strength average
 *	for beacons only (u8, dBm)
 * @NL80211_STA_INFO_TID_STATS: per-TID statistics (see &enum nl80211_tid_stats)
 *	This is a nested attribute where each the inner attribute number is the
 *	TID+1 and the special TID 16 (i.e. value 17) is used for non-QoS frames;
 *	each one of those is again nested with &enum nl80211_tid_stats
 *	attributes carrying the actual values.
 * @NL80211_STA_INFO_RX_DURATION: aggregate PPDU duration for all frames
 *	received from the station (u64, usec)
 * @NL80211_STA_INFO_PAD: attribute used for padding for 64-bit alignment
 * @NL80211_STA_INFO_ACK_SIGNAL: signal strength of the last ACK frame(u8, dBm)
 * @NL80211_STA_INFO_ACK_SIGNAL_AVG: avg signal strength of ACK frames (s8, dBm)
 * @NL80211_STA_INFO_RX_MPDUS: total number of received packets (MPDUs)
 *	(u32, from this station)
 * @NL80211_STA_INFO_FCS_ERROR_COUNT: total number of packets (MPDUs) received
 *	with an FCS error (u32, from this station). This count may not include
 *	some packets with an FCS error due to TA corruption. Hence this counter
 *	might not be fully accurate.
 * @NL80211_STA_INFO_CONNECTED_TO_GATE: set to true if STA has a path to a
 *	mesh gate (u8, 0 or 1)
 * @NL80211_STA_INFO_TX_DURATION: aggregate PPDU duration for all frames
 *	sent to the station (u64, usec)
 * @NL80211_STA_INFO_AIRTIME_WEIGHT: current airtime weight for station (u16)
 * @NL80211_STA_INFO_AIRTIME_LINK_METRIC: airtime link metric for mesh station
 * @NL80211_STA_INFO_ASSOC_AT_BOOTTIME: Timestamp (CLOCK_BOOTTIME, nanoseconds)
 *	of STA's association
 * @NL80211_STA_INFO_CONNECTED_TO_AS: set to true if STA has a path to a
 *	authentication server (u8, 0 or 1)
 * @__NL80211_STA_INFO_AFTER_LAST: internal
 * @NL80211_STA_INFO_MAX: highest possible station info attribute
 */
enum nl80211_sta_info {
  __NL80211_STA_INFO_INVALID,
  NL80211_STA_INFO_INACTIVE_TIME,
  NL80211_STA_INFO_RX_BYTES,
  NL80211_STA_INFO_TX_BYTES,
  NL80211_STA_INFO_LLID,
  NL80211_STA_INFO_PLID,
  NL80211_STA_INFO_PLINK_STATE,
  NL80211_STA_INFO_SIGNAL,
  NL80211_STA_INFO_TX_BITRATE,
  NL80211_STA_INFO_RX_PACKETS,
  NL80211_STA_INFO_TX_PACKETS,
  NL80211_STA_INFO_TX_RETRIES,
  NL80211_STA_INFO_TX_FAILED,
  NL80211_STA_INFO_SIGNAL_AVG,
  NL80211_STA_INFO_RX_BITRATE,
  NL80211_STA_INFO_BSS_PARAM,
  NL80211_STA_INFO_CONNECTED_TIME,
  NL80211_STA_INFO_STA_FLAGS,
  NL80211_STA_INFO_BEACON_LOSS,
  NL80211_STA_INFO_T_OFFSET,
  NL80211_STA_INFO_LOCAL_PM,
  NL80211_STA_INFO_PEER_PM,
  NL80211_STA_INFO_NONPEER_PM,
  NL80211_STA_INFO_RX_BYTES64,
  NL80211_STA_INFO_TX_BYTES64,
  NL80211_STA_INFO_CHAIN_SIGNAL,
  NL80211_STA_INFO_CHAIN_SIGNAL_AVG,
  NL80211_STA_INFO_EXPECTED_THROUGHPUT,
  NL80211_STA_INFO_RX_DROP_MISC,
  NL80211_STA_INFO_BEACON_RX,
  NL80211_STA_INFO_BEACON_SIGNAL_AVG,
  NL80211_STA_INFO_TID_STATS,
  NL80211_STA_INFO_RX_DURATION,
  NL80211_STA_INFO_PAD,
  NL80211_STA_INFO_ACK_SIGNAL,
  NL80211_STA_INFO_ACK_SIGNAL_AVG,
  NL80211_STA_INFO_RX_MPDUS,
  NL80211_STA_INFO_FCS_ERROR_COUNT,
  NL80211_STA_INFO_CONNECTED_TO_GATE,
  NL80211_STA_INFO_TX_DURATION,
  NL80211_STA_INFO_AIRTIME_WEIGHT,
  NL80211_STA_INFO_AIRTIME_LINK_METRIC,
  NL80211_STA_INFO_ASSOC_AT_BOOTTIME,
  NL80211_STA_INFO_CONNECTED_TO_AS,

  /* keep last */
  __NL80211_STA_INFO_AFTER_LAST,
  NL80211_STA_INFO_MAX = __NL80211_STA_INFO_AFTER_LAST - 1
};

/**
 * enum nl80211_sta_bss_param - BSS information collected by STA
 *
 * These attribute types are used with %NL80211_STA_INFO_BSS_PARAM
 * when getting information about the bitrate of a station.
 *
 * @__NL80211_STA_BSS_PARAM_INVALID: attribute number 0 is reserved
 * @NL80211_STA_BSS_PARAM_CTS_PROT: whether CTS protection is enabled (flag)
 * @NL80211_STA_BSS_PARAM_SHORT_PREAMBLE:  whether short preamble is enabled
 *	(flag)
 * @NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME:  whether short slot time is enabled
 *	(flag)
 * @NL80211_STA_BSS_PARAM_DTIM_PERIOD: DTIM period for beaconing (u8)
 * @NL80211_STA_BSS_PARAM_BEACON_INTERVAL: Beacon interval (u16)
 * @NL80211_STA_BSS_PARAM_MAX: highest sta_bss_param number currently defined
 * @__NL80211_STA_BSS_PARAM_AFTER_LAST: internal use
 */
enum nl80211_sta_bss_param {
  __NL80211_STA_BSS_PARAM_INVALID,
  NL80211_STA_BSS_PARAM_CTS_PROT,
  NL80211_STA_BSS_PARAM_SHORT_PREAMBLE,
  NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME,
  NL80211_STA_BSS_PARAM_DTIM_PERIOD,
  NL80211_STA_BSS_PARAM_BEACON_INTERVAL,

  /* keep last */
  __NL80211_STA_BSS_PARAM_AFTER_LAST,
  NL80211_STA_BSS_PARAM_MAX = __NL80211_STA_BSS_PARAM_AFTER_LAST - 1
};

#endif /* __CONKY_LINUX_NL80211_H__ */
