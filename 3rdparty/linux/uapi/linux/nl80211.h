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
 * enum nl80211_commands - supported nl80211 commands
 *
 * @NL80211_CMD_UNSPEC: unspecified command to catch errors
 *
 * @NL80211_CMD_GET_WIPHY: request information about a wiphy or dump request
 *	to get a list of all present wiphys.
 * @NL80211_CMD_SET_WIPHY: set wiphy parameters, needs %NL80211_ATTR_WIPHY or
 *	%NL80211_ATTR_IFINDEX; can be used to set %NL80211_ATTR_WIPHY_NAME,
 *	%NL80211_ATTR_WIPHY_TXQ_PARAMS, %NL80211_ATTR_WIPHY_FREQ,
 *	%NL80211_ATTR_WIPHY_FREQ_OFFSET (and the attributes determining the
 *	channel width; this is used for setting monitor mode channel),
 *	%NL80211_ATTR_WIPHY_RETRY_SHORT, %NL80211_ATTR_WIPHY_RETRY_LONG,
 *	%NL80211_ATTR_WIPHY_FRAG_THRESHOLD, and/or
 *	%NL80211_ATTR_WIPHY_RTS_THRESHOLD.  However, for setting the channel,
 *	see %NL80211_CMD_SET_CHANNEL instead, the support here is for backward
 *	compatibility only.
 * @NL80211_CMD_NEW_WIPHY: Newly created wiphy, response to get request
 *	or rename notification. Has attributes %NL80211_ATTR_WIPHY and
 *	%NL80211_ATTR_WIPHY_NAME.
 * @NL80211_CMD_DEL_WIPHY: Wiphy deleted. Has attributes
 *	%NL80211_ATTR_WIPHY and %NL80211_ATTR_WIPHY_NAME.
 *
 * @NL80211_CMD_GET_INTERFACE: Request an interface's configuration;
 *	either a dump request for all interfaces or a specific get with a
 *	single %NL80211_ATTR_IFINDEX is supported.
 * @NL80211_CMD_SET_INTERFACE: Set type of a virtual interface, requires
 *	%NL80211_ATTR_IFINDEX and %NL80211_ATTR_IFTYPE.
 * @NL80211_CMD_NEW_INTERFACE: Newly created virtual interface or response
 *	to %NL80211_CMD_GET_INTERFACE. Has %NL80211_ATTR_IFINDEX,
 *	%NL80211_ATTR_WIPHY and %NL80211_ATTR_IFTYPE attributes. Can also
 *	be sent from userspace to request creation of a new virtual interface,
 *	then requires attributes %NL80211_ATTR_WIPHY, %NL80211_ATTR_IFTYPE and
 *	%NL80211_ATTR_IFNAME.
 * @NL80211_CMD_DEL_INTERFACE: Virtual interface was deleted, has attributes
 *	%NL80211_ATTR_IFINDEX and %NL80211_ATTR_WIPHY. Can also be sent from
 *	userspace to request deletion of a virtual interface, then requires
 *	attribute %NL80211_ATTR_IFINDEX. If multiple BSSID advertisements are
 *	enabled using %NL80211_ATTR_MBSSID_CONFIG, %NL80211_ATTR_MBSSID_ELEMS,
 *	and if this command is used for the transmitting interface, then all
 *	the non-transmitting interfaces are deleted as well.
 *
 * @NL80211_CMD_GET_KEY: Get sequence counter information for a key specified
 *	by %NL80211_ATTR_KEY_IDX and/or %NL80211_ATTR_MAC. %NL80211_ATTR_MAC
 *	represents peer's MLD address for MLO pairwise key. For MLO group key,
 *	the link is identified by %NL80211_ATTR_MLO_LINK_ID.
 * @NL80211_CMD_SET_KEY: Set key attributes %NL80211_ATTR_KEY_DEFAULT,
 *	%NL80211_ATTR_KEY_DEFAULT_MGMT, or %NL80211_ATTR_KEY_THRESHOLD.
 *	For MLO connection, the link to set default key is identified by
 *	%NL80211_ATTR_MLO_LINK_ID.
 * @NL80211_CMD_NEW_KEY: add a key with given %NL80211_ATTR_KEY_DATA,
 *	%NL80211_ATTR_KEY_IDX, %NL80211_ATTR_MAC, %NL80211_ATTR_KEY_CIPHER,
 *	and %NL80211_ATTR_KEY_SEQ attributes. %NL80211_ATTR_MAC represents
 *	peer's MLD address for MLO pairwise key. The link to add MLO
 *	group key is identified by %NL80211_ATTR_MLO_LINK_ID.
 * @NL80211_CMD_DEL_KEY: delete a key identified by %NL80211_ATTR_KEY_IDX
 *	or %NL80211_ATTR_MAC. %NL80211_ATTR_MAC represents peer's MLD address
 *	for MLO pairwise key. The link to delete group key is identified by
 *	%NL80211_ATTR_MLO_LINK_ID.
 *
 * @NL80211_CMD_GET_BEACON: (not used)
 * @NL80211_CMD_SET_BEACON: change the beacon on an access point interface
 *	using the %NL80211_ATTR_BEACON_HEAD and %NL80211_ATTR_BEACON_TAIL
 *	attributes. For drivers that generate the beacon and probe responses
 *	internally, the following attributes must be provided: %NL80211_ATTR_IE,
 *	%NL80211_ATTR_IE_PROBE_RESP and %NL80211_ATTR_IE_ASSOC_RESP.
 * @NL80211_CMD_START_AP: Start AP operation on an AP interface, parameters
 *	are like for %NL80211_CMD_SET_BEACON, and additionally parameters that
 *	do not change are used, these include %NL80211_ATTR_BEACON_INTERVAL,
 *	%NL80211_ATTR_DTIM_PERIOD, %NL80211_ATTR_SSID,
 *	%NL80211_ATTR_HIDDEN_SSID, %NL80211_ATTR_CIPHERS_PAIRWISE,
 *	%NL80211_ATTR_CIPHER_GROUP, %NL80211_ATTR_WPA_VERSIONS,
 *	%NL80211_ATTR_AKM_SUITES, %NL80211_ATTR_PRIVACY,
 *	%NL80211_ATTR_AUTH_TYPE, %NL80211_ATTR_INACTIVITY_TIMEOUT,
 *	%NL80211_ATTR_ACL_POLICY and %NL80211_ATTR_MAC_ADDRS.
 *	The channel to use can be set on the interface or be given using the
 *	%NL80211_ATTR_WIPHY_FREQ and %NL80211_ATTR_WIPHY_FREQ_OFFSET, and the
 *	attributes determining channel width.
 * @NL80211_CMD_NEW_BEACON: old alias for %NL80211_CMD_START_AP
 * @NL80211_CMD_STOP_AP: Stop AP operation on the given interface
 * @NL80211_CMD_DEL_BEACON: old alias for %NL80211_CMD_STOP_AP
 *
 * @NL80211_CMD_GET_STATION: Get station attributes for station identified by
 *	%NL80211_ATTR_MAC on the interface identified by %NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_SET_STATION: Set station attributes for station identified by
 *	%NL80211_ATTR_MAC on the interface identified by %NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_NEW_STATION: Add a station with given attributes to the
 *	interface identified by %NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_DEL_STATION: Remove a station identified by %NL80211_ATTR_MAC
 *	or, if no MAC address given, all stations, on the interface identified
 *	by %NL80211_ATTR_IFINDEX. For MLD station, MLD address is used in
 *	%NL80211_ATTR_MAC. %NL80211_ATTR_MGMT_SUBTYPE and
 *	%NL80211_ATTR_REASON_CODE can optionally be used to specify which type
 *	of disconnection indication should be sent to the station
 *	(Deauthentication or Disassociation frame and reason code for that
 *	frame). %NL80211_ATTR_MLO_LINK_ID can be used optionally to remove
 *	stations connected and using at least that link as one of its links.
 *
 * @NL80211_CMD_GET_MPATH: Get mesh path attributes for mesh path to
 * 	destination %NL80211_ATTR_MAC on the interface identified by
 * 	%NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_SET_MPATH:  Set mesh path attributes for mesh path to
 * 	destination %NL80211_ATTR_MAC on the interface identified by
 * 	%NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_NEW_MPATH: Create a new mesh path for the destination given by
 *	%NL80211_ATTR_MAC via %NL80211_ATTR_MPATH_NEXT_HOP.
 * @NL80211_CMD_DEL_MPATH: Delete a mesh path to the destination given by
 *	%NL80211_ATTR_MAC.
 * @NL80211_CMD_NEW_PATH: Add a mesh path with given attributes to the
 *	interface identified by %NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_DEL_PATH: Remove a mesh path identified by %NL80211_ATTR_MAC
 *	or, if no MAC address given, all mesh paths, on the interface identified
 *	by %NL80211_ATTR_IFINDEX.
 * @NL80211_CMD_SET_BSS: Set BSS attributes for BSS identified by
 *	%NL80211_ATTR_IFINDEX.
 *
 * @NL80211_CMD_GET_REG: ask the wireless core to send us its currently set
 *	regulatory domain. If %NL80211_ATTR_WIPHY is specified and the device
 *	has a private regulatory domain, it will be returned. Otherwise, the
 *	global regdomain will be returned.
 *	A device will have a private regulatory domain if it uses the
 *	regulatory_hint() API. Even when a private regdomain is used the channel
 *	information will still be mended according to further hints from
 *	the regulatory core to help with compliance. A dump version of this API
 *	is now available which will returns the global regdomain as well as
 *	all private regdomains of present wiphys (for those that have it).
 *	If a wiphy is self-managed (%NL80211_ATTR_WIPHY_SELF_MANAGED_REG), then
 *	its private regdomain is the only valid one for it. The regulatory
 *	core is not used to help with compliance in this case.
 * @NL80211_CMD_SET_REG: Set current regulatory domain. CRDA sends this command
 *	after being queried by the kernel. CRDA replies by sending a regulatory
 *	domain structure which consists of %NL80211_ATTR_REG_ALPHA set to our
 *	current alpha2 if it found a match. It also provides
 * 	NL80211_ATTR_REG_RULE_FLAGS, and a set of regulatory rules. Each
 * 	regulatory rule is a nested set of attributes  given by
 * 	%NL80211_ATTR_REG_RULE_FREQ_[START|END] and
 * 	%NL80211_ATTR_FREQ_RANGE_MAX_BW with an attached power rule given by
 * 	%NL80211_ATTR_REG_RULE_POWER_MAX_ANT_GAIN and
 * 	%NL80211_ATTR_REG_RULE_POWER_MAX_EIRP.
 * @NL80211_CMD_REQ_SET_REG: ask the wireless core to set the regulatory domain
 * 	to the specified ISO/IEC 3166-1 alpha2 country code. The core will
 * 	store this as a valid request and then query userspace for it.
 *
 * @NL80211_CMD_GET_MESH_CONFIG: Get mesh networking properties for the
 *	interface identified by %NL80211_ATTR_IFINDEX
 *
 * @NL80211_CMD_SET_MESH_CONFIG: Set mesh networking properties for the
 *      interface identified by %NL80211_ATTR_IFINDEX
 *
 * @NL80211_CMD_SET_MGMT_EXTRA_IE: Set extra IEs for management frames. The
 *	interface is identified with %NL80211_ATTR_IFINDEX and the management
 *	frame subtype with %NL80211_ATTR_MGMT_SUBTYPE. The extra IE data to be
 *	added to the end of the specified management frame is specified with
 *	%NL80211_ATTR_IE. If the command succeeds, the requested data will be
 *	added to all specified management frames generated by
 *	kernel/firmware/driver.
 *	Note: This command has been removed and it is only reserved at this
 *	point to avoid re-using existing command number. The functionality this
 *	command was planned for has been provided with cleaner design with the
 *	option to specify additional IEs in NL80211_CMD_TRIGGER_SCAN,
 *	NL80211_CMD_AUTHENTICATE, NL80211_CMD_ASSOCIATE,
 *	NL80211_CMD_DEAUTHENTICATE, and NL80211_CMD_DISASSOCIATE.
 *
 * @NL80211_CMD_GET_SCAN: get scan results
 * @NL80211_CMD_TRIGGER_SCAN: trigger a new scan with the given parameters
 *	%NL80211_ATTR_TX_NO_CCK_RATE is used to decide whether to send the
 *	probe requests at CCK rate or not. %NL80211_ATTR_BSSID can be used to
 *	specify a BSSID to scan for; if not included, the wildcard BSSID will
 *	be used.
 * @NL80211_CMD_NEW_SCAN_RESULTS: scan notification (as a reply to
 *	NL80211_CMD_GET_SCAN and on the "scan" multicast group)
 * @NL80211_CMD_SCAN_ABORTED: scan was aborted, for unspecified reasons,
 *	partial scan results may be available
 *
 * @NL80211_CMD_START_SCHED_SCAN: start a scheduled scan at certain
 *	intervals and certain number of cycles, as specified by
 *	%NL80211_ATTR_SCHED_SCAN_PLANS. If %NL80211_ATTR_SCHED_SCAN_PLANS is
 *	not specified and only %NL80211_ATTR_SCHED_SCAN_INTERVAL is specified,
 *	scheduled scan will run in an infinite loop with the specified interval.
 *	These attributes are mutually exclusive,
 *	i.e. NL80211_ATTR_SCHED_SCAN_INTERVAL must not be passed if
 *	NL80211_ATTR_SCHED_SCAN_PLANS is defined.
 *	If for some reason scheduled scan is aborted by the driver, all scan
 *	plans are canceled (including scan plans that did not start yet).
 *	Like with normal scans, if SSIDs (%NL80211_ATTR_SCAN_SSIDS)
 *	are passed, they are used in the probe requests.  For
 *	broadcast, a broadcast SSID must be passed (ie. an empty
 *	string).  If no SSID is passed, no probe requests are sent and
 *	a passive scan is performed.  %NL80211_ATTR_SCAN_FREQUENCIES,
 *	if passed, define which channels should be scanned; if not
 *	passed, all channels allowed for the current regulatory domain
 *	are used.  Extra IEs can also be passed from the userspace by
 *	using the %NL80211_ATTR_IE attribute.  The first cycle of the
 *	scheduled scan can be delayed by %NL80211_ATTR_SCHED_SCAN_DELAY
 *	is supplied. If the device supports multiple concurrent scheduled
 *	scans, it will allow such when the caller provides the flag attribute
 *	%NL80211_ATTR_SCHED_SCAN_MULTI to indicate user-space support for it.
 * @NL80211_CMD_STOP_SCHED_SCAN: stop a scheduled scan. Returns -ENOENT if
 *	scheduled scan is not running. The caller may assume that as soon
 *	as the call returns, it is safe to start a new scheduled scan again.
 * @NL80211_CMD_SCHED_SCAN_RESULTS: indicates that there are scheduled scan
 *	results available.
 * @NL80211_CMD_SCHED_SCAN_STOPPED: indicates that the scheduled scan has
 *	stopped.  The driver may issue this event at any time during a
 *	scheduled scan.  One reason for stopping the scan is if the hardware
 *	does not support starting an association or a normal scan while running
 *	a scheduled scan.  This event is also sent when the
 *	%NL80211_CMD_STOP_SCHED_SCAN command is received or when the interface
 *	is brought down while a scheduled scan was running.
 *
 * @NL80211_CMD_GET_SURVEY: get survey results, e.g. channel occupation
 *      or noise level
 * @NL80211_CMD_NEW_SURVEY_RESULTS: survey data notification (as a reply to
 *	NL80211_CMD_GET_SURVEY and on the "scan" multicast group)
 *
 * @NL80211_CMD_SET_PMKSA: Add a PMKSA cache entry using %NL80211_ATTR_MAC
 *	(for the BSSID), %NL80211_ATTR_PMKID, and optionally %NL80211_ATTR_PMK
 *	(PMK is used for PTKSA derivation in case of FILS shared key offload) or
 *	using %NL80211_ATTR_SSID, %NL80211_ATTR_FILS_CACHE_ID,
 *	%NL80211_ATTR_PMKID, and %NL80211_ATTR_PMK in case of FILS
 *	authentication where %NL80211_ATTR_FILS_CACHE_ID is the identifier
 *	advertised by a FILS capable AP identifying the scope of PMKSA in an
 *	ESS.
 * @NL80211_CMD_DEL_PMKSA: Delete a PMKSA cache entry, using %NL80211_ATTR_MAC
 *	(for the BSSID) and %NL80211_ATTR_PMKID or using %NL80211_ATTR_SSID,
 *	%NL80211_ATTR_FILS_CACHE_ID, and %NL80211_ATTR_PMKID in case of FILS
 *	authentication. Additionally in case of SAE offload and OWE offloads
 *	PMKSA entry can be deleted using %NL80211_ATTR_SSID.
 * @NL80211_CMD_FLUSH_PMKSA: Flush all PMKSA cache entries.
 *
 * @NL80211_CMD_REG_CHANGE: indicates to userspace the regulatory domain
 * 	has been changed and provides details of the request information
 * 	that caused the change such as who initiated the regulatory request
 * 	(%NL80211_ATTR_REG_INITIATOR), the wiphy_idx
 * 	(%NL80211_ATTR_REG_ALPHA2) on which the request was made from if
 * 	the initiator was %NL80211_REGDOM_SET_BY_COUNTRY_IE or
 * 	%NL80211_REGDOM_SET_BY_DRIVER, the type of regulatory domain
 * 	set (%NL80211_ATTR_REG_TYPE), if the type of regulatory domain is
 * 	%NL80211_REG_TYPE_COUNTRY the alpha2 to which we have moved on
 * 	to (%NL80211_ATTR_REG_ALPHA2).
 * @NL80211_CMD_REG_BEACON_HINT: indicates to userspace that an AP beacon
 * 	has been found while world roaming thus enabling active scan or
 * 	any mode of operation that initiates TX (beacons) on a channel
 * 	where we would not have been able to do either before. As an example
 * 	if you are world roaming (regulatory domain set to world or if your
 * 	driver is using a custom world roaming regulatory domain) and while
 * 	doing a passive scan on the 5 GHz band you find an AP there (if not
 * 	on a DFS channel) you will now be able to actively scan for that AP
 * 	or use AP mode on your card on that same channel. Note that this will
 * 	never be used for channels 1-11 on the 2 GHz band as they are always
 * 	enabled world wide. This beacon hint is only sent if your device had
 * 	either disabled active scanning or beaconing on a channel. We send to
 * 	userspace the wiphy on which we removed a restriction from
 * 	(%NL80211_ATTR_WIPHY) and the channel on which this occurred
 * 	before (%NL80211_ATTR_FREQ_BEFORE) and after (%NL80211_ATTR_FREQ_AFTER)
 * 	the beacon hint was processed.
 *
 * @NL80211_CMD_AUTHENTICATE: authentication request and notification.
 *	This command is used both as a command (request to authenticate) and
 *	as an event on the "mlme" multicast group indicating completion of the
 *	authentication process.
 *	When used as a command, %NL80211_ATTR_IFINDEX is used to identify the
 *	interface. %NL80211_ATTR_MAC is used to specify PeerSTAAddress (and
 *	BSSID in case of station mode). %NL80211_ATTR_SSID is used to specify
 *	the SSID (mainly for association, but is included in authentication
 *	request, too, to help BSS selection. %NL80211_ATTR_WIPHY_FREQ +
 *	%NL80211_ATTR_WIPHY_FREQ_OFFSET is used to specify the frequency of the
 *	channel in MHz. %NL80211_ATTR_AUTH_TYPE is used to specify the
 *	authentication type. %NL80211_ATTR_IE is used to define IEs
 *	(VendorSpecificInfo, but also including RSN IE and FT IEs) to be added
 *	to the frame.
 *	When used as an event, this reports reception of an Authentication
 *	frame in station and IBSS modes when the local MLME processed the
 *	frame, i.e., it was for the local STA and was received in correct
 *	state. This is similar to MLME-AUTHENTICATE.confirm primitive in the
 *	MLME SAP interface (kernel providing MLME, userspace SME). The
 *	included %NL80211_ATTR_FRAME attribute contains the management frame
 *	(including both the header and frame body, but not FCS). This event is
 *	also used to indicate if the authentication attempt timed out. In that
 *	case the %NL80211_ATTR_FRAME attribute is replaced with a
 *	%NL80211_ATTR_TIMED_OUT flag (and %NL80211_ATTR_MAC to indicate which
 *	pending authentication timed out).
 * @NL80211_CMD_ASSOCIATE: association request and notification; like
 *	NL80211_CMD_AUTHENTICATE but for Association and Reassociation
 *	(similar to MLME-ASSOCIATE.request, MLME-REASSOCIATE.request,
 *	MLME-ASSOCIATE.confirm or MLME-REASSOCIATE.confirm primitives). The
 *	%NL80211_ATTR_PREV_BSSID attribute is used to specify whether the
 *	request is for the initial association to an ESS (that attribute not
 *	included) or for reassociation within the ESS (that attribute is
 *	included).
 * @NL80211_CMD_DEAUTHENTICATE: deauthentication request and notification; like
 *	NL80211_CMD_AUTHENTICATE but for Deauthentication frames (similar to
 *	MLME-DEAUTHENTICATION.request and MLME-DEAUTHENTICATE.indication
 *	primitives).
 * @NL80211_CMD_DISASSOCIATE: disassociation request and notification; like
 *	NL80211_CMD_AUTHENTICATE but for Disassociation frames (similar to
 *	MLME-DISASSOCIATE.request and MLME-DISASSOCIATE.indication primitives).
 *
 * @NL80211_CMD_MICHAEL_MIC_FAILURE: notification of a locally detected Michael
 *	MIC (part of TKIP) failure; sent on the "mlme" multicast group; the
 *	event includes %NL80211_ATTR_MAC to describe the source MAC address of
 *	the frame with invalid MIC, %NL80211_ATTR_KEY_TYPE to show the key
 *	type, %NL80211_ATTR_KEY_IDX to indicate the key identifier, and
 *	%NL80211_ATTR_KEY_SEQ to indicate the TSC value of the frame; this
 *	event matches with MLME-MICHAELMICFAILURE.indication() primitive
 *
 * @NL80211_CMD_JOIN_IBSS: Join a new IBSS -- given at least an SSID and a
 *	FREQ attribute (for the initial frequency if no peer can be found)
 *	and optionally a MAC (as BSSID) and FREQ_FIXED attribute if those
 *	should be fixed rather than automatically determined. Can only be
 *	executed on a network interface that is UP, and fixed BSSID/FREQ
 *	may be rejected. Another optional parameter is the beacon interval,
 *	given in the %NL80211_ATTR_BEACON_INTERVAL attribute, which if not
 *	given defaults to 100 TU (102.4ms).
 * @NL80211_CMD_LEAVE_IBSS: Leave the IBSS -- no special arguments, the IBSS is
 *	determined by the network interface.
 *
 * @NL80211_CMD_TESTMODE: testmode command, takes a wiphy (or ifindex) attribute
 *	to identify the device, and the TESTDATA blob attribute to pass through
 *	to the driver.
 *
 * @NL80211_CMD_CONNECT: connection request and notification; this command
 *	requests to connect to a specified network but without separating
 *	auth and assoc steps. For this, you need to specify the SSID in a
 *	%NL80211_ATTR_SSID attribute, and can optionally specify the association
 *	IEs in %NL80211_ATTR_IE, %NL80211_ATTR_AUTH_TYPE,
 *	%NL80211_ATTR_USE_MFP, %NL80211_ATTR_MAC, %NL80211_ATTR_WIPHY_FREQ,
 *	%NL80211_ATTR_WIPHY_FREQ_OFFSET, %NL80211_ATTR_CONTROL_PORT,
 *	%NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
 *	%NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT,
 *	%NL80211_ATTR_CONTROL_PORT_OVER_NL80211, %NL80211_ATTR_MAC_HINT, and
 *	%NL80211_ATTR_WIPHY_FREQ_HINT.
 *	If included, %NL80211_ATTR_MAC and %NL80211_ATTR_WIPHY_FREQ are
 *	restrictions on BSS selection, i.e., they effectively prevent roaming
 *	within the ESS. %NL80211_ATTR_MAC_HINT and %NL80211_ATTR_WIPHY_FREQ_HINT
 *	can be included to provide a recommendation of the initial BSS while
 *	allowing the driver to roam to other BSSes within the ESS and also to
 *	ignore this recommendation if the indicated BSS is not ideal. Only one
 *	set of BSSID,frequency parameters is used (i.e., either the enforcing
 *	%NL80211_ATTR_MAC,%NL80211_ATTR_WIPHY_FREQ or the less strict
 *	%NL80211_ATTR_MAC_HINT and %NL80211_ATTR_WIPHY_FREQ_HINT).
 *	Driver shall not modify the IEs specified through %NL80211_ATTR_IE if
 *	%NL80211_ATTR_MAC is included. However, if %NL80211_ATTR_MAC_HINT is
 *	included, these IEs through %NL80211_ATTR_IE are specified by the user
 *	space based on the best possible BSS selected. Thus, if the driver ends
 *	up selecting a different BSS, it can modify these IEs accordingly (e.g.
 *	userspace asks the driver to perform PMKSA caching with BSS1 and the
 *	driver ends up selecting BSS2 with different PMKSA cache entry; RSNIE
 *	has to get updated with the apt PMKID).
 *	%NL80211_ATTR_PREV_BSSID can be used to request a reassociation within
 *	the ESS in case the device is already associated and an association with
 *	a different BSS is desired.
 *	Background scan period can optionally be
 *	specified in %NL80211_ATTR_BG_SCAN_PERIOD,
 *	if not specified default background scan configuration
 *	in driver is used and if period value is 0, bg scan will be disabled.
 *	This attribute is ignored if driver does not support roam scan.
 *	It is also sent as an event, with the BSSID and response IEs when the
 *	connection is established or failed to be established. This can be
 *	determined by the %NL80211_ATTR_STATUS_CODE attribute (0 = success,
 *	non-zero = failure). If %NL80211_ATTR_TIMED_OUT is included in the
 *	event, the connection attempt failed due to not being able to initiate
 *	authentication/association or not receiving a response from the AP.
 *	Non-zero %NL80211_ATTR_STATUS_CODE value is indicated in that case as
 *	well to remain backwards compatible.
 * @NL80211_CMD_ROAM: Notification indicating the card/driver roamed by itself.
 *	When a security association was established on an 802.1X network using
 *	fast transition, this event should be followed by an
 *	%NL80211_CMD_PORT_AUTHORIZED event.
 *	Following a %NL80211_CMD_ROAM event userspace can issue
 *	%NL80211_CMD_GET_SCAN in order to obtain the scan information for the
 *	new BSS the card/driver roamed to.
 * @NL80211_CMD_DISCONNECT: drop a given connection; also used to notify
 *	userspace that a connection was dropped by the AP or due to other
 *	reasons, for this the %NL80211_ATTR_DISCONNECTED_BY_AP and
 *	%NL80211_ATTR_REASON_CODE attributes are used.
 *
 * @NL80211_CMD_SET_WIPHY_NETNS: Set a wiphy's netns. Note that all devices
 *	associated with this wiphy must be down and will follow.
 *
 * @NL80211_CMD_REMAIN_ON_CHANNEL: Request to remain awake on the specified
 *	channel for the specified amount of time. This can be used to do
 *	off-channel operations like transmit a Public Action frame and wait for
 *	a response while being associated to an AP on another channel.
 *	%NL80211_ATTR_IFINDEX is used to specify which interface (and thus
 *	radio) is used. %NL80211_ATTR_WIPHY_FREQ is used to specify the
 *	frequency for the operation.
 *	%NL80211_ATTR_DURATION is used to specify the duration in milliseconds
 *	to remain on the channel. This command is also used as an event to
 *	notify when the requested duration starts (it may take a while for the
 *	driver to schedule this time due to other concurrent needs for the
 *	radio).
 *	When called, this operation returns a cookie (%NL80211_ATTR_COOKIE)
 *	that will be included with any events pertaining to this request;
 *	the cookie is also used to cancel the request.
 * @NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL: This command can be used to cancel a
 *	pending remain-on-channel duration if the desired operation has been
 *	completed prior to expiration of the originally requested duration.
 *	%NL80211_ATTR_WIPHY or %NL80211_ATTR_IFINDEX is used to specify the
 *	radio. The %NL80211_ATTR_COOKIE attribute must be given as well to
 *	uniquely identify the request.
 *	This command is also used as an event to notify when a requested
 *	remain-on-channel duration has expired.
 *
 * @NL80211_CMD_SET_TX_BITRATE_MASK: Set the mask of rates to be used in TX
 *	rate selection. %NL80211_ATTR_IFINDEX is used to specify the interface
 *	and @NL80211_ATTR_TX_RATES the set of allowed rates.
 *
 * @NL80211_CMD_REGISTER_FRAME: Register for receiving certain mgmt frames
 *	(via @NL80211_CMD_FRAME) for processing in userspace. This command
 *	requires an interface index, a frame type attribute (optional for
 *	backward compatibility reasons, if not given assumes action frames)
 *	and a match attribute containing the first few bytes of the frame
 *	that should match, e.g. a single byte for only a category match or
 *	four bytes for vendor frames including the OUI. The registration
 *	cannot be dropped, but is removed automatically when the netlink
 *	socket is closed. Multiple registrations can be made.
 *	The %NL80211_ATTR_RECEIVE_MULTICAST flag attribute can be given if
 *	%NL80211_EXT_FEATURE_MULTICAST_REGISTRATIONS is available, in which
 *	case the registration can also be modified to include/exclude the
 *	flag, rather than requiring unregistration to change it.
 * @NL80211_CMD_REGISTER_ACTION: Alias for @NL80211_CMD_REGISTER_FRAME for
 *	backward compatibility
 * @NL80211_CMD_FRAME: Management frame TX request and RX notification. This
 *	command is used both as a request to transmit a management frame and
 *	as an event indicating reception of a frame that was not processed in
 *	kernel code, but is for us (i.e., which may need to be processed in a
 *	user space application). %NL80211_ATTR_FRAME is used to specify the
 *	frame contents (including header). %NL80211_ATTR_WIPHY_FREQ is used
 *	to indicate on which channel the frame is to be transmitted or was
 *	received. If this channel is not the current channel (remain-on-channel
 *	or the operational channel) the device will switch to the given channel
 *	and transmit the frame, optionally waiting for a response for the time
 *	specified using %NL80211_ATTR_DURATION. When called, this operation
 *	returns a cookie (%NL80211_ATTR_COOKIE) that will be included with the
 *	TX status event pertaining to the TX request.
 *	%NL80211_ATTR_TX_NO_CCK_RATE is used to decide whether to send the
 *	management frames at CCK rate or not in 2GHz band.
 *	%NL80211_ATTR_CSA_C_OFFSETS_TX is an array of offsets to CSA
 *	counters which will be updated to the current value. This attribute
 *	is used during CSA period.
 *	For TX on an MLD, the frequency can be omitted and the link ID be
 *	specified, or if transmitting to a known peer MLD (with MLD addresses
 *	in the frame) both can be omitted and the link will be selected by
 *	lower layers.
 *	For RX notification, %NL80211_ATTR_RX_HW_TIMESTAMP may be included to
 *	indicate the frame RX timestamp and %NL80211_ATTR_TX_HW_TIMESTAMP may
 *	be included to indicate the ack TX timestamp.
 * @NL80211_CMD_FRAME_WAIT_CANCEL: When an off-channel TX was requested, this
 *	command may be used with the corresponding cookie to cancel the wait
 *	time if it is known that it is no longer necessary.  This command is
 *	also sent as an event whenever the driver has completed the off-channel
 *	wait time.
 * @NL80211_CMD_ACTION: Alias for @NL80211_CMD_FRAME for backward compatibility.
 * @NL80211_CMD_FRAME_TX_STATUS: Report TX status of a management frame
 *	transmitted with %NL80211_CMD_FRAME. %NL80211_ATTR_COOKIE identifies
 *	the TX command and %NL80211_ATTR_FRAME includes the contents of the
 *	frame. %NL80211_ATTR_ACK flag is included if the recipient acknowledged
 *	the frame. %NL80211_ATTR_TX_HW_TIMESTAMP may be included to indicate the
 *	tx timestamp and %NL80211_ATTR_RX_HW_TIMESTAMP may be included to
 *	indicate the ack RX timestamp.
 * @NL80211_CMD_ACTION_TX_STATUS: Alias for @NL80211_CMD_FRAME_TX_STATUS for
 *	backward compatibility.
 *
 * @NL80211_CMD_SET_POWER_SAVE: Set powersave, using %NL80211_ATTR_PS_STATE
 * @NL80211_CMD_GET_POWER_SAVE: Get powersave status in %NL80211_ATTR_PS_STATE
 *
 * @NL80211_CMD_SET_CQM: Connection quality monitor configuration. This command
 *	is used to configure connection quality monitoring notification trigger
 *	levels.
 * @NL80211_CMD_NOTIFY_CQM: Connection quality monitor notification. This
 *	command is used as an event to indicate the that a trigger level was
 *	reached.
 * @NL80211_CMD_SET_CHANNEL: Set the channel (using %NL80211_ATTR_WIPHY_FREQ
 *	and the attributes determining channel width) the given interface
 *	(identified by %NL80211_ATTR_IFINDEX) shall operate on.
 *	In case multiple channels are supported by the device, the mechanism
 *	with which it switches channels is implementation-defined.
 *	When a monitor interface is given, it can only switch channel while
 *	no other interfaces are operating to avoid disturbing the operation
 *	of any other interfaces, and other interfaces will again take
 *	precedence when they are used.
 *
 * @NL80211_CMD_SET_WDS_PEER: Set the MAC address of the peer on a WDS interface
 *	(no longer supported).
 *
 * @NL80211_CMD_SET_MULTICAST_TO_UNICAST: Configure if this AP should perform
 *	multicast to unicast conversion. When enabled, all multicast packets
 *	with ethertype ARP, IPv4 or IPv6 (possibly within an 802.1Q header)
 *	will be sent out to each station once with the destination (multicast)
 *	MAC address replaced by the station's MAC address. Note that this may
 *	break certain expectations of the receiver, e.g. the ability to drop
 *	unicast IP packets encapsulated in multicast L2 frames, or the ability
 *	to not send destination unreachable messages in such cases.
 *	This can only be toggled per BSS. Configure this on an interface of
 *	type %NL80211_IFTYPE_AP. It applies to all its VLAN interfaces
 *	(%NL80211_IFTYPE_AP_VLAN), except for those in 4addr (WDS) mode.
 *	If %NL80211_ATTR_MULTICAST_TO_UNICAST_ENABLED is not present with this
 *	command, the feature is disabled.
 *
 * @NL80211_CMD_JOIN_MESH: Join a mesh. The mesh ID must be given, and initial
 *	mesh config parameters may be given.
 * @NL80211_CMD_LEAVE_MESH: Leave the mesh network -- no special arguments, the
 *	network is determined by the network interface.
 *
 * @NL80211_CMD_UNPROT_DEAUTHENTICATE: Unprotected deauthentication frame
 *	notification. This event is used to indicate that an unprotected
 *	deauthentication frame was dropped when MFP is in use.
 * @NL80211_CMD_UNPROT_DISASSOCIATE: Unprotected disassociation frame
 *	notification. This event is used to indicate that an unprotected
 *	disassociation frame was dropped when MFP is in use.
 *
 * @NL80211_CMD_NEW_PEER_CANDIDATE: Notification on the reception of a
 *      beacon or probe response from a compatible mesh peer.  This is only
 *      sent while no station information (sta_info) exists for the new peer
 *      candidate and when @NL80211_MESH_SETUP_USERSPACE_AUTH,
 *      @NL80211_MESH_SETUP_USERSPACE_AMPE, or
 *      @NL80211_MESH_SETUP_USERSPACE_MPM is set.  On reception of this
 *      notification, userspace may decide to create a new station
 *      (@NL80211_CMD_NEW_STATION).  To stop this notification from
 *      reoccurring, the userspace authentication daemon may want to create the
 *      new station with the AUTHENTICATED flag unset and maybe change it later
 *      depending on the authentication result.
 *
 * @NL80211_CMD_GET_WOWLAN: get Wake-on-Wireless-LAN (WoWLAN) settings.
 * @NL80211_CMD_SET_WOWLAN: set Wake-on-Wireless-LAN (WoWLAN) settings.
 *	Since wireless is more complex than wired ethernet, it supports
 *	various triggers. These triggers can be configured through this
 *	command with the %NL80211_ATTR_WOWLAN_TRIGGERS attribute. For
 *	more background information, see
 *	https://wireless.wiki.kernel.org/en/users/Documentation/WoWLAN.
 *	The @NL80211_CMD_SET_WOWLAN command can also be used as a notification
 *	from the driver reporting the wakeup reason. In this case, the
 *	@NL80211_ATTR_WOWLAN_TRIGGERS attribute will contain the reason
 *	for the wakeup, if it was caused by wireless. If it is not present
 *	in the wakeup notification, the wireless device didn't cause the
 *	wakeup but reports that it was woken up.
 *
 * @NL80211_CMD_SET_REKEY_OFFLOAD: This command is used give the driver
 *	the necessary information for supporting GTK rekey offload. This
 *	feature is typically used during WoWLAN. The configuration data
 *	is contained in %NL80211_ATTR_REKEY_DATA (which is nested and
 *	contains the data in sub-attributes). After rekeying happened,
 *	this command may also be sent by the driver as an MLME event to
 *	inform userspace of the new replay counter.
 *
 * @NL80211_CMD_PMKSA_CANDIDATE: This is used as an event to inform userspace
 *	of PMKSA caching candidates.
 *
 * @NL80211_CMD_TDLS_OPER: Perform a high-level TDLS command (e.g. link setup).
 *	In addition, this can be used as an event to request userspace to take
 *	actions on TDLS links (set up a new link or tear down an existing one).
 *	In such events, %NL80211_ATTR_TDLS_OPERATION indicates the requested
 *	operation, %NL80211_ATTR_MAC contains the peer MAC address, and
 *	%NL80211_ATTR_REASON_CODE the reason code to be used (only with
 *	%NL80211_TDLS_TEARDOWN).
 * @NL80211_CMD_TDLS_MGMT: Send a TDLS management frame. The
 *	%NL80211_ATTR_TDLS_ACTION attribute determines the type of frame to be
 *	sent. Public Action codes (802.11-2012 8.1.5.1) will be sent as
 *	802.11 management frames, while TDLS action codes (802.11-2012
 *	8.5.13.1) will be encapsulated and sent as data frames. The currently
 *	supported Public Action code is %WLAN_PUB_ACTION_TDLS_DISCOVER_RES
 *	and the currently supported TDLS actions codes are given in
 *	&enum ieee80211_tdls_actioncode.
 *
 * @NL80211_CMD_UNEXPECTED_FRAME: Used by an application controlling an AP
 *	(or GO) interface (i.e. hostapd) to ask for unexpected frames to
 *	implement sending deauth to stations that send unexpected class 3
 *	frames. Also used as the event sent by the kernel when such a frame
 *	is received.
 *	For the event, the %NL80211_ATTR_MAC attribute carries the TA and
 *	other attributes like the interface index are present.
 *	If used as the command it must have an interface index and you can
 *	only unsubscribe from the event by closing the socket. Subscription
 *	is also for %NL80211_CMD_UNEXPECTED_4ADDR_FRAME events.
 *
 * @NL80211_CMD_UNEXPECTED_4ADDR_FRAME: Sent as an event indicating that the
 *	associated station identified by %NL80211_ATTR_MAC sent a 4addr frame
 *	and wasn't already in a 4-addr VLAN. The event will be sent similarly
 *	to the %NL80211_CMD_UNEXPECTED_FRAME event, to the same listener.
 *
 * @NL80211_CMD_PROBE_CLIENT: Probe an associated station on an AP interface
 *	by sending a null data frame to it and reporting when the frame is
 *	acknowledged. This is used to allow timing out inactive clients. Uses
 *	%NL80211_ATTR_IFINDEX and %NL80211_ATTR_MAC. The command returns a
 *	direct reply with an %NL80211_ATTR_COOKIE that is later used to match
 *	up the event with the request. The event includes the same data and
 *	has %NL80211_ATTR_ACK set if the frame was ACKed.
 *
 * @NL80211_CMD_REGISTER_BEACONS: Register this socket to receive beacons from
 *	other BSSes when any interfaces are in AP mode. This helps implement
 *	OLBC handling in hostapd. Beacons are reported in %NL80211_CMD_FRAME
 *	messages. Note that per PHY only one application may register.
 *
 * @NL80211_CMD_SET_NOACK_MAP: sets a bitmap for the individual TIDs whether
 *      No Acknowledgement Policy should be applied.
 *
 * @NL80211_CMD_CH_SWITCH_NOTIFY: An AP or GO may decide to switch channels
 *	independently of the userspace SME, send this event indicating
 *	%NL80211_ATTR_IFINDEX is now on %NL80211_ATTR_WIPHY_FREQ and the
 *	attributes determining channel width.  This indication may also be
 *	sent when a remotely-initiated switch (e.g., when a STA receives a CSA
 *	from the remote AP) is completed;
 *
 * @NL80211_CMD_CH_SWITCH_STARTED_NOTIFY: Notify that a channel switch
 *	has been started on an interface, regardless of the initiator
 *	(ie. whether it was requested from a remote device or
 *	initiated on our own).  It indicates that
 *	%NL80211_ATTR_IFINDEX will be on %NL80211_ATTR_WIPHY_FREQ
 *	after %NL80211_ATTR_CH_SWITCH_COUNT TBTT's.  The userspace may
 *	decide to react to this indication by requesting other
 *	interfaces to change channel as well.
 *
 * @NL80211_CMD_START_P2P_DEVICE: Start the given P2P Device, identified by
 *	its %NL80211_ATTR_WDEV identifier. It must have been created with
 *	%NL80211_CMD_NEW_INTERFACE previously. After it has been started, the
 *	P2P Device can be used for P2P operations, e.g. remain-on-channel and
 *	public action frame TX.
 * @NL80211_CMD_STOP_P2P_DEVICE: Stop the given P2P Device, identified by
 *	its %NL80211_ATTR_WDEV identifier.
 *
 * @NL80211_CMD_CONN_FAILED: connection request to an AP failed; used to
 *	notify userspace that AP has rejected the connection request from a
 *	station, due to particular reason. %NL80211_ATTR_CONN_FAILED_REASON
 *	is used for this.
 *
 * @NL80211_CMD_SET_MCAST_RATE: Change the rate used to send multicast frames
 *	for IBSS or MESH vif.
 *
 * @NL80211_CMD_SET_MAC_ACL: sets ACL for MAC address based access control.
 *	This is to be used with the drivers advertising the support of MAC
 *	address based access control. List of MAC addresses is passed in
 *	%NL80211_ATTR_MAC_ADDRS and ACL policy is passed in
 *	%NL80211_ATTR_ACL_POLICY. Driver will enable ACL with this list, if it
 *	is not already done. The new list will replace any existing list. Driver
 *	will clear its ACL when the list of MAC addresses passed is empty. This
 *	command is used in AP/P2P GO mode. Driver has to make sure to clear its
 *	ACL list during %NL80211_CMD_STOP_AP.
 *
 * @NL80211_CMD_RADAR_DETECT: Start a Channel availability check (CAC). Once
 *	a radar is detected or the channel availability scan (CAC) has finished
 *	or was aborted, or a radar was detected, usermode will be notified with
 *	this event. This command is also used to notify userspace about radars
 *	while operating on this channel.
 *	%NL80211_ATTR_RADAR_EVENT is used to inform about the type of the
 *	event.
 *
 * @NL80211_CMD_GET_PROTOCOL_FEATURES: Get global nl80211 protocol features,
 *	i.e. features for the nl80211 protocol rather than device features.
 *	Returns the features in the %NL80211_ATTR_PROTOCOL_FEATURES bitmap.
 *
 * @NL80211_CMD_UPDATE_FT_IES: Pass down the most up-to-date Fast Transition
 *	Information Element to the WLAN driver
 *
 * @NL80211_CMD_FT_EVENT: Send a Fast transition event from the WLAN driver
 *	to the supplicant. This will carry the target AP's MAC address along
 *	with the relevant Information Elements. This event is used to report
 *	received FT IEs (MDIE, FTIE, RSN IE, TIE, RICIE).
 *
 * @NL80211_CMD_CRIT_PROTOCOL_START: Indicates user-space will start running
 *	a critical protocol that needs more reliability in the connection to
 *	complete.
 *
 * @NL80211_CMD_CRIT_PROTOCOL_STOP: Indicates the connection reliability can
 *	return back to normal.
 *
 * @NL80211_CMD_GET_COALESCE: Get currently supported coalesce rules.
 * @NL80211_CMD_SET_COALESCE: Configure coalesce rules or clear existing rules.
 *
 * @NL80211_CMD_CHANNEL_SWITCH: Perform a channel switch by announcing the
 *	new channel information (Channel Switch Announcement - CSA)
 *	in the beacon for some time (as defined in the
 *	%NL80211_ATTR_CH_SWITCH_COUNT parameter) and then change to the
 *	new channel. Userspace provides the new channel information (using
 *	%NL80211_ATTR_WIPHY_FREQ and the attributes determining channel
 *	width). %NL80211_ATTR_CH_SWITCH_BLOCK_TX may be supplied to inform
 *	other station that transmission must be blocked until the channel
 *	switch is complete.
 *
 * @NL80211_CMD_VENDOR: Vendor-specified command/event. The command is specified
 *	by the %NL80211_ATTR_VENDOR_ID attribute and a sub-command in
 *	%NL80211_ATTR_VENDOR_SUBCMD. Parameter(s) can be transported in
 *	%NL80211_ATTR_VENDOR_DATA.
 *	For feature advertisement, the %NL80211_ATTR_VENDOR_DATA attribute is
 *	used in the wiphy data as a nested attribute containing descriptions
 *	(&struct nl80211_vendor_cmd_info) of the supported vendor commands.
 *	This may also be sent as an event with the same attributes.
 *
 * @NL80211_CMD_SET_QOS_MAP: Set Interworking QoS mapping for IP DSCP values.
 *	The QoS mapping information is included in %NL80211_ATTR_QOS_MAP. If
 *	that attribute is not included, QoS mapping is disabled. Since this
 *	QoS mapping is relevant for IP packets, it is only valid during an
 *	association. This is cleared on disassociation and AP restart.
 *
 * @NL80211_CMD_ADD_TX_TS: Ask the kernel to add a traffic stream for the given
 *	%NL80211_ATTR_TSID and %NL80211_ATTR_MAC with %NL80211_ATTR_USER_PRIO
 *	and %NL80211_ATTR_ADMITTED_TIME parameters.
 *	Note that the action frame handshake with the AP shall be handled by
 *	userspace via the normal management RX/TX framework, this only sets
 *	up the TX TS in the driver/device.
 *	If the admitted time attribute is not added then the request just checks
 *	if a subsequent setup could be successful, the intent is to use this to
 *	avoid setting up a session with the AP when local restrictions would
 *	make that impossible. However, the subsequent "real" setup may still
 *	fail even if the check was successful.
 * @NL80211_CMD_DEL_TX_TS: Remove an existing TS with the %NL80211_ATTR_TSID
 *	and %NL80211_ATTR_MAC parameters. It isn't necessary to call this
 *	before removing a station entry entirely, or before disassociating
 *	or similar, cleanup will happen in the driver/device in this case.
 *
 * @NL80211_CMD_GET_MPP: Get mesh path attributes for mesh proxy path to
 *	destination %NL80211_ATTR_MAC on the interface identified by
 *	%NL80211_ATTR_IFINDEX.
 *
 * @NL80211_CMD_JOIN_OCB: Join the OCB network. The center frequency and
 *	bandwidth of a channel must be given.
 * @NL80211_CMD_LEAVE_OCB: Leave the OCB network -- no special arguments, the
 *	network is determined by the network interface.
 *
 * @NL80211_CMD_TDLS_CHANNEL_SWITCH: Start channel-switching with a TDLS peer,
 *	identified by the %NL80211_ATTR_MAC parameter. A target channel is
 *	provided via %NL80211_ATTR_WIPHY_FREQ and other attributes determining
 *	channel width/type. The target operating class is given via
 *	%NL80211_ATTR_OPER_CLASS.
 *	The driver is responsible for continually initiating channel-switching
 *	operations and returning to the base channel for communication with the
 *	AP.
 * @NL80211_CMD_TDLS_CANCEL_CHANNEL_SWITCH: Stop channel-switching with a TDLS
 *	peer given by %NL80211_ATTR_MAC. Both peers must be on the base channel
 *	when this command completes.
 *
 * @NL80211_CMD_WIPHY_REG_CHANGE: Similar to %NL80211_CMD_REG_CHANGE, but used
 *	as an event to indicate changes for devices with wiphy-specific regdom
 *	management.
 *
 * @NL80211_CMD_ABORT_SCAN: Stop an ongoing scan. Returns -ENOENT if a scan is
 *	not running. The driver indicates the status of the scan through
 *	cfg80211_scan_done().
 *
 * @NL80211_CMD_START_NAN: Start NAN operation, identified by its
 *	%NL80211_ATTR_WDEV interface. This interface must have been
 *	previously created with %NL80211_CMD_NEW_INTERFACE. After it
 *	has been started, the NAN interface will create or join a
 *	cluster. This command must have a valid
 *	%NL80211_ATTR_NAN_MASTER_PREF attribute and optional
 *	%NL80211_ATTR_BANDS attributes.  If %NL80211_ATTR_BANDS is
 *	omitted or set to 0, it means don't-care and the device will
 *	decide what to use.  After this command NAN functions can be
 *	added.
 * @NL80211_CMD_STOP_NAN: Stop the NAN operation, identified by
 *	its %NL80211_ATTR_WDEV interface.
 * @NL80211_CMD_ADD_NAN_FUNCTION: Add a NAN function. The function is defined
 *	with %NL80211_ATTR_NAN_FUNC nested attribute. When called, this
 *	operation returns the strictly positive and unique instance id
 *	(%NL80211_ATTR_NAN_FUNC_INST_ID) and a cookie (%NL80211_ATTR_COOKIE)
 *	of the function upon success.
 *	Since instance ID's can be re-used, this cookie is the right
 *	way to identify the function. This will avoid races when a termination
 *	event is handled by the user space after it has already added a new
 *	function that got the same instance id from the kernel as the one
 *	which just terminated.
 *	This cookie may be used in NAN events even before the command
 *	returns, so userspace shouldn't process NAN events until it processes
 *	the response to this command.
 *	Look at %NL80211_ATTR_SOCKET_OWNER as well.
 * @NL80211_CMD_DEL_NAN_FUNCTION: Delete a NAN function by cookie.
 *	This command is also used as a notification sent when a NAN function is
 *	terminated. This will contain a %NL80211_ATTR_NAN_FUNC_INST_ID
 *	and %NL80211_ATTR_COOKIE attributes.
 * @NL80211_CMD_CHANGE_NAN_CONFIG: Change current NAN
 *	configuration. NAN must be operational (%NL80211_CMD_START_NAN
 *	was executed).  It must contain at least one of the following
 *	attributes: %NL80211_ATTR_NAN_MASTER_PREF,
 *	%NL80211_ATTR_BANDS.  If %NL80211_ATTR_BANDS is omitted, the
 *	current configuration is not changed.  If it is present but
 *	set to zero, the configuration is changed to don't-care
 *	(i.e. the device can decide what to do).
 * @NL80211_CMD_NAN_FUNC_MATCH: Notification sent when a match is reported.
 *	This will contain a %NL80211_ATTR_NAN_MATCH nested attribute and
 *	%NL80211_ATTR_COOKIE.
 *
 * @NL80211_CMD_UPDATE_CONNECT_PARAMS: Update one or more connect parameters
 *	for subsequent roaming cases if the driver or firmware uses internal
 *	BSS selection. This command can be issued only while connected and it
 *	does not result in a change for the current association. Currently,
 *	only the %NL80211_ATTR_IE data is used and updated with this command.
 *
 * @NL80211_CMD_SET_PMK: For offloaded 4-Way handshake, set the PMK or PMK-R0
 *	for the given authenticator address (specified with %NL80211_ATTR_MAC).
 *	When %NL80211_ATTR_PMKR0_NAME is set, %NL80211_ATTR_PMK specifies the
 *	PMK-R0, otherwise it specifies the PMK.
 * @NL80211_CMD_DEL_PMK: For offloaded 4-Way handshake, delete the previously
 *	configured PMK for the authenticator address identified by
 *	%NL80211_ATTR_MAC.
 * @NL80211_CMD_PORT_AUTHORIZED: An event that indicates port is authorized and
 *	open for regular data traffic. For STA/P2P-client, this event is sent
 *	with AP MAC address and for AP/P2P-GO, the event carries the STA/P2P-
 *	client MAC address.
 *	Drivers that support 4 way handshake offload should send this event for
 *	STA/P2P-client after successful 4-way HS or after 802.1X FT following
 *	NL80211_CMD_CONNECT or NL80211_CMD_ROAM. Drivers using AP/P2P-GO 4-way
 *	handshake offload should send this event on successful completion of
 *	4-way handshake with the peer (STA/P2P-client).
 * @NL80211_CMD_CONTROL_PORT_FRAME: Control Port (e.g. PAE) frame TX request
 *	and RX notification.  This command is used both as a request to transmit
 *	a control port frame and as a notification that a control port frame
 *	has been received. %NL80211_ATTR_FRAME is used to specify the
 *	frame contents.  The frame is the raw EAPoL data, without ethernet or
 *	802.11 headers.
 *	For an MLD transmitter, the %NL80211_ATTR_MLO_LINK_ID may be given and
 *	its effect will depend on the destination: If the destination is known
 *	to be an MLD, this will be used as a hint to select the link to transmit
 *	the frame on. If the destination is not an MLD, this will select both
 *	the link to transmit on and the source address will be set to the link
 *	address of that link.
 *	When used as an event indication %NL80211_ATTR_CONTROL_PORT_ETHERTYPE,
 *	%NL80211_ATTR_CONTROL_PORT_NO_ENCRYPT and %NL80211_ATTR_MAC are added
 *	indicating the protocol type of the received frame; whether the frame
 *	was received unencrypted and the MAC address of the peer respectively.
 *
 * @NL80211_CMD_RELOAD_REGDB: Request that the regdb firmware file is reloaded.
 *
 * @NL80211_CMD_EXTERNAL_AUTH: This interface is exclusively defined for host
 *	drivers that do not define separate commands for authentication and
 *	association, but rely on user space for the authentication to happen.
 *	This interface acts both as the event request (driver to user space)
 *	to trigger the authentication and command response (userspace to
 *	driver) to indicate the authentication status.
 *
 *	User space uses the %NL80211_CMD_CONNECT command to the host driver to
 *	trigger a connection. The host driver selects a BSS and further uses
 *	this interface to offload only the authentication part to the user
 *	space. Authentication frames are passed between the driver and user
 *	space through the %NL80211_CMD_FRAME interface. Host driver proceeds
 *	further with the association after getting successful authentication
 *	status. User space indicates the authentication status through
 *	%NL80211_ATTR_STATUS_CODE attribute in %NL80211_CMD_EXTERNAL_AUTH
 *	command interface.
 *
 *	Host driver sends MLD address of the AP with %NL80211_ATTR_MLD_ADDR in
 *	%NL80211_CMD_EXTERNAL_AUTH event to indicate user space to enable MLO
 *	during the authentication offload in STA mode while connecting to MLD
 *	APs. Host driver should check %NL80211_ATTR_MLO_SUPPORT flag capability
 *	in %NL80211_CMD_CONNECT to know whether the user space supports enabling
 *	MLO during the authentication offload or not.
 *	User space should enable MLO during the authentication only when it
 *	receives the AP MLD address in authentication offload request. User
 *	space shouldn't enable MLO when the authentication offload request
 *	doesn't indicate the AP MLD address even if the AP is MLO capable.
 *	User space should use %NL80211_ATTR_MLD_ADDR as peer's MLD address and
 *	interface address identified by %NL80211_ATTR_IFINDEX as self MLD
 *	address. User space and host driver to use MLD addresses in RA, TA and
 *	BSSID fields of the frames between them, and host driver translates the
 *	MLD addresses to/from link addresses based on the link chosen for the
 *	authentication.
 *
 *	Host driver reports this status on an authentication failure to the
 *	user space through the connect result as the user space would have
 *	initiated the connection through the connect request.
 *
 * @NL80211_CMD_STA_OPMODE_CHANGED: An event that notify station's
 *	ht opmode or vht opmode changes using any of %NL80211_ATTR_SMPS_MODE,
 *	%NL80211_ATTR_CHANNEL_WIDTH,%NL80211_ATTR_NSS attributes with its
 *	address(specified in %NL80211_ATTR_MAC).
 *
 * @NL80211_CMD_GET_FTM_RESPONDER_STATS: Retrieve FTM responder statistics, in
 *	the %NL80211_ATTR_FTM_RESPONDER_STATS attribute.
 *
 * @NL80211_CMD_PEER_MEASUREMENT_START: start a (set of) peer measurement(s)
 *	with the given parameters, which are encapsulated in the nested
 *	%NL80211_ATTR_PEER_MEASUREMENTS attribute. Optionally, MAC address
 *	randomization may be enabled and configured by specifying the
 *	%NL80211_ATTR_MAC and %NL80211_ATTR_MAC_MASK attributes.
 *	If a timeout is requested, use the %NL80211_ATTR_TIMEOUT attribute.
 *	A u64 cookie for further %NL80211_ATTR_COOKIE use is returned in
 *	the netlink extended ack message.
 *
 *	To cancel a measurement, close the socket that requested it.
 *
 *	Measurement results are reported to the socket that requested the
 *	measurement using @NL80211_CMD_PEER_MEASUREMENT_RESULT when they
 *	become available, so applications must ensure a large enough socket
 *	buffer size.
 *
 *	Depending on driver support it may or may not be possible to start
 *	multiple concurrent measurements.
 * @NL80211_CMD_PEER_MEASUREMENT_RESULT: This command number is used for the
 *	result notification from the driver to the requesting socket.
 * @NL80211_CMD_PEER_MEASUREMENT_COMPLETE: Notification only, indicating that
 *	the measurement completed, using the measurement cookie
 *	(%NL80211_ATTR_COOKIE).
 *
 * @NL80211_CMD_NOTIFY_RADAR: Notify the kernel that a radar signal was
 *	detected and reported by a neighboring device on the channel
 *	indicated by %NL80211_ATTR_WIPHY_FREQ and other attributes
 *	determining the width and type.
 *
 * @NL80211_CMD_UPDATE_OWE_INFO: This interface allows the host driver to
 *	offload OWE processing to user space. This intends to support
 *	OWE AKM by the host drivers that implement SME but rely
 *	on the user space for the cryptographic/DH IE processing in AP mode.
 *
 * @NL80211_CMD_PROBE_MESH_LINK: The requirement for mesh link metric
 *	refreshing, is that from one mesh point we be able to send some data
 *	frames to other mesh points which are not currently selected as a
 *	primary traffic path, but which are only 1 hop away. The absence of
 *	the primary path to the chosen node makes it necessary to apply some
 *	form of marking on a chosen packet stream so that the packets can be
 *	properly steered to the selected node for testing, and not by the
 *	regular mesh path lookup. Further, the packets must be of type data
 *	so that the rate control (often embedded in firmware) is used for
 *	rate selection.
 *
 *	Here attribute %NL80211_ATTR_MAC is used to specify connected mesh
 *	peer MAC address and %NL80211_ATTR_FRAME is used to specify the frame
 *	content. The frame is ethernet data.
 *
 * @NL80211_CMD_SET_TID_CONFIG: Data frame TID specific configuration
 *	is passed using %NL80211_ATTR_TID_CONFIG attribute.
 *
 * @NL80211_CMD_UNPROT_BEACON: Unprotected or incorrectly protected Beacon
 *	frame. This event is used to indicate that a received Beacon frame was
 *	dropped because it did not include a valid MME MIC while beacon
 *	protection was enabled (BIGTK configured in station mode).
 *
 * @NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS: Report TX status of a control
 *	port frame transmitted with %NL80211_CMD_CONTROL_PORT_FRAME.
 *	%NL80211_ATTR_COOKIE identifies the TX command and %NL80211_ATTR_FRAME
 *	includes the contents of the frame. %NL80211_ATTR_ACK flag is included
 *	if the recipient acknowledged the frame.
 *
 * @NL80211_CMD_SET_SAR_SPECS: SAR power limitation configuration is
 *	passed using %NL80211_ATTR_SAR_SPEC. %NL80211_ATTR_WIPHY is used to
 *	specify the wiphy index to be applied to.
 *
 * @NL80211_CMD_OBSS_COLOR_COLLISION: This notification is sent out whenever
 *	mac80211/drv detects a bss color collision.
 *
 * @NL80211_CMD_COLOR_CHANGE_REQUEST: This command is used to indicate that
 *	userspace wants to change the BSS color.
 *
 * @NL80211_CMD_COLOR_CHANGE_STARTED: Notify userland, that a color change has
 *	started
 *
 * @NL80211_CMD_COLOR_CHANGE_ABORTED: Notify userland, that the color change has
 *	been aborted
 *
 * @NL80211_CMD_COLOR_CHANGE_COMPLETED: Notify userland that the color change
 *	has completed
 *
 * @NL80211_CMD_SET_FILS_AAD: Set FILS AAD data to the driver using -
 *	&NL80211_ATTR_MAC - for STA MAC address
 *	&NL80211_ATTR_FILS_KEK - for KEK
 *	&NL80211_ATTR_FILS_NONCES - for FILS Nonces
 *		(STA Nonce 16 bytes followed by AP Nonce 16 bytes)
 *
 * @NL80211_CMD_ASSOC_COMEBACK: notification about an association
 *      temporal rejection with comeback. The event includes %NL80211_ATTR_MAC
 *      to describe the BSSID address of the AP and %NL80211_ATTR_TIMEOUT to
 *      specify the timeout value.
 *
 * @NL80211_CMD_ADD_LINK: Add a new link to an interface. The
 *	%NL80211_ATTR_MLO_LINK_ID attribute is used for the new link.
 * @NL80211_CMD_REMOVE_LINK: Remove a link from an interface. This may come
 *	without %NL80211_ATTR_MLO_LINK_ID as an easy way to remove all links
 *	in preparation for e.g. roaming to a regular (non-MLO) AP.
 *
 * @NL80211_CMD_ADD_LINK_STA: Add a link to an MLD station
 * @NL80211_CMD_MODIFY_LINK_STA: Modify a link of an MLD station
 * @NL80211_CMD_REMOVE_LINK_STA: Remove a link of an MLD station
 *
 * @NL80211_CMD_SET_HW_TIMESTAMP: Enable/disable HW timestamping of Timing
 *	measurement and Fine timing measurement frames. If %NL80211_ATTR_MAC
 *	is included, enable/disable HW timestamping only for frames to/from the
 *	specified MAC address. Otherwise enable/disable HW timestamping for
 *	all TM/FTM frames (including ones that were enabled with specific MAC
 *	address). If %NL80211_ATTR_HW_TIMESTAMP_ENABLED is not included, disable
 *	HW timestamping.
 *	The number of peers that HW timestamping can be enabled for concurrently
 *	is indicated by %NL80211_ATTR_MAX_HW_TIMESTAMP_PEERS.
 *
 * @NL80211_CMD_LINKS_REMOVED: Notify userspace about the removal of STA MLD
 *	setup links due to AP MLD removing the corresponding affiliated APs with
 *	Multi-Link reconfiguration. %NL80211_ATTR_MLO_LINKS is used to provide
 *	information about the removed STA MLD setup links.
 *
 * @NL80211_CMD_SET_TID_TO_LINK_MAPPING: Set the TID to Link Mapping for a
 *      non-AP MLD station. The %NL80211_ATTR_MLO_TTLM_DLINK and
 *      %NL80211_ATTR_MLO_TTLM_ULINK attributes are used to specify the
 *      TID to Link mapping for downlink/uplink traffic.
 *
 * @NL80211_CMD_MAX: highest used command number
 * @__NL80211_CMD_AFTER_LAST: internal use
 */
enum nl80211_commands {
  /* don't change the order or add anything between, this is ABI! */
  NL80211_CMD_UNSPEC,

  NL80211_CMD_GET_WIPHY, /* can dump */
  NL80211_CMD_SET_WIPHY,
  NL80211_CMD_NEW_WIPHY,
  NL80211_CMD_DEL_WIPHY,

  NL80211_CMD_GET_INTERFACE, /* can dump */
  NL80211_CMD_SET_INTERFACE,
  NL80211_CMD_NEW_INTERFACE,
  NL80211_CMD_DEL_INTERFACE,

  NL80211_CMD_GET_KEY,
  NL80211_CMD_SET_KEY,
  NL80211_CMD_NEW_KEY,
  NL80211_CMD_DEL_KEY,

  NL80211_CMD_GET_BEACON,
  NL80211_CMD_SET_BEACON,
  NL80211_CMD_START_AP,
  NL80211_CMD_NEW_BEACON = NL80211_CMD_START_AP,
  NL80211_CMD_STOP_AP,
  NL80211_CMD_DEL_BEACON = NL80211_CMD_STOP_AP,

  NL80211_CMD_GET_STATION,
  NL80211_CMD_SET_STATION,
  NL80211_CMD_NEW_STATION,
  NL80211_CMD_DEL_STATION,

  NL80211_CMD_GET_MPATH,
  NL80211_CMD_SET_MPATH,
  NL80211_CMD_NEW_MPATH,
  NL80211_CMD_DEL_MPATH,

  NL80211_CMD_SET_BSS,

  NL80211_CMD_SET_REG,
  NL80211_CMD_REQ_SET_REG,

  NL80211_CMD_GET_MESH_CONFIG,
  NL80211_CMD_SET_MESH_CONFIG,

  NL80211_CMD_SET_MGMT_EXTRA_IE /* reserved; not used */,

  NL80211_CMD_GET_REG,

  NL80211_CMD_GET_SCAN,
  NL80211_CMD_TRIGGER_SCAN,
  NL80211_CMD_NEW_SCAN_RESULTS,
  NL80211_CMD_SCAN_ABORTED,

  NL80211_CMD_REG_CHANGE,

  NL80211_CMD_AUTHENTICATE,
  NL80211_CMD_ASSOCIATE,
  NL80211_CMD_DEAUTHENTICATE,
  NL80211_CMD_DISASSOCIATE,

  NL80211_CMD_MICHAEL_MIC_FAILURE,

  NL80211_CMD_REG_BEACON_HINT,

  NL80211_CMD_JOIN_IBSS,
  NL80211_CMD_LEAVE_IBSS,

  NL80211_CMD_TESTMODE,

  NL80211_CMD_CONNECT,
  NL80211_CMD_ROAM,
  NL80211_CMD_DISCONNECT,

  NL80211_CMD_SET_WIPHY_NETNS,

  NL80211_CMD_GET_SURVEY,
  NL80211_CMD_NEW_SURVEY_RESULTS,

  NL80211_CMD_SET_PMKSA,
  NL80211_CMD_DEL_PMKSA,
  NL80211_CMD_FLUSH_PMKSA,

  NL80211_CMD_REMAIN_ON_CHANNEL,
  NL80211_CMD_CANCEL_REMAIN_ON_CHANNEL,

  NL80211_CMD_SET_TX_BITRATE_MASK,

  NL80211_CMD_REGISTER_FRAME,
  NL80211_CMD_REGISTER_ACTION = NL80211_CMD_REGISTER_FRAME,
  NL80211_CMD_FRAME,
  NL80211_CMD_ACTION = NL80211_CMD_FRAME,
  NL80211_CMD_FRAME_TX_STATUS,
  NL80211_CMD_ACTION_TX_STATUS = NL80211_CMD_FRAME_TX_STATUS,

  NL80211_CMD_SET_POWER_SAVE,
  NL80211_CMD_GET_POWER_SAVE,

  NL80211_CMD_SET_CQM,
  NL80211_CMD_NOTIFY_CQM,

  NL80211_CMD_SET_CHANNEL,
  NL80211_CMD_SET_WDS_PEER,

  NL80211_CMD_FRAME_WAIT_CANCEL,

  NL80211_CMD_JOIN_MESH,
  NL80211_CMD_LEAVE_MESH,

  NL80211_CMD_UNPROT_DEAUTHENTICATE,
  NL80211_CMD_UNPROT_DISASSOCIATE,

  NL80211_CMD_NEW_PEER_CANDIDATE,

  NL80211_CMD_GET_WOWLAN,
  NL80211_CMD_SET_WOWLAN,

  NL80211_CMD_START_SCHED_SCAN,
  NL80211_CMD_STOP_SCHED_SCAN,
  NL80211_CMD_SCHED_SCAN_RESULTS,
  NL80211_CMD_SCHED_SCAN_STOPPED,

  NL80211_CMD_SET_REKEY_OFFLOAD,

  NL80211_CMD_PMKSA_CANDIDATE,

  NL80211_CMD_TDLS_OPER,
  NL80211_CMD_TDLS_MGMT,

  NL80211_CMD_UNEXPECTED_FRAME,

  NL80211_CMD_PROBE_CLIENT,

  NL80211_CMD_REGISTER_BEACONS,

  NL80211_CMD_UNEXPECTED_4ADDR_FRAME,

  NL80211_CMD_SET_NOACK_MAP,

  NL80211_CMD_CH_SWITCH_NOTIFY,

  NL80211_CMD_START_P2P_DEVICE,
  NL80211_CMD_STOP_P2P_DEVICE,

  NL80211_CMD_CONN_FAILED,

  NL80211_CMD_SET_MCAST_RATE,

  NL80211_CMD_SET_MAC_ACL,

  NL80211_CMD_RADAR_DETECT,

  NL80211_CMD_GET_PROTOCOL_FEATURES,

  NL80211_CMD_UPDATE_FT_IES,
  NL80211_CMD_FT_EVENT,

  NL80211_CMD_CRIT_PROTOCOL_START,
  NL80211_CMD_CRIT_PROTOCOL_STOP,

  NL80211_CMD_GET_COALESCE,
  NL80211_CMD_SET_COALESCE,

  NL80211_CMD_CHANNEL_SWITCH,

  NL80211_CMD_VENDOR,

  NL80211_CMD_SET_QOS_MAP,

  NL80211_CMD_ADD_TX_TS,
  NL80211_CMD_DEL_TX_TS,

  NL80211_CMD_GET_MPP,

  NL80211_CMD_JOIN_OCB,
  NL80211_CMD_LEAVE_OCB,

  NL80211_CMD_CH_SWITCH_STARTED_NOTIFY,

  NL80211_CMD_TDLS_CHANNEL_SWITCH,
  NL80211_CMD_TDLS_CANCEL_CHANNEL_SWITCH,

  NL80211_CMD_WIPHY_REG_CHANGE,

  NL80211_CMD_ABORT_SCAN,

  NL80211_CMD_START_NAN,
  NL80211_CMD_STOP_NAN,
  NL80211_CMD_ADD_NAN_FUNCTION,
  NL80211_CMD_DEL_NAN_FUNCTION,
  NL80211_CMD_CHANGE_NAN_CONFIG,
  NL80211_CMD_NAN_MATCH,

  NL80211_CMD_SET_MULTICAST_TO_UNICAST,

  NL80211_CMD_UPDATE_CONNECT_PARAMS,

  NL80211_CMD_SET_PMK,
  NL80211_CMD_DEL_PMK,

  NL80211_CMD_PORT_AUTHORIZED,

  NL80211_CMD_RELOAD_REGDB,

  NL80211_CMD_EXTERNAL_AUTH,

  NL80211_CMD_STA_OPMODE_CHANGED,

  NL80211_CMD_CONTROL_PORT_FRAME,

  NL80211_CMD_GET_FTM_RESPONDER_STATS,

  NL80211_CMD_PEER_MEASUREMENT_START,
  NL80211_CMD_PEER_MEASUREMENT_RESULT,
  NL80211_CMD_PEER_MEASUREMENT_COMPLETE,

  NL80211_CMD_NOTIFY_RADAR,

  NL80211_CMD_UPDATE_OWE_INFO,

  NL80211_CMD_PROBE_MESH_LINK,

  NL80211_CMD_SET_TID_CONFIG,

  NL80211_CMD_UNPROT_BEACON,

  NL80211_CMD_CONTROL_PORT_FRAME_TX_STATUS,

  NL80211_CMD_SET_SAR_SPECS,

  NL80211_CMD_OBSS_COLOR_COLLISION,

  NL80211_CMD_COLOR_CHANGE_REQUEST,

  NL80211_CMD_COLOR_CHANGE_STARTED,
  NL80211_CMD_COLOR_CHANGE_ABORTED,
  NL80211_CMD_COLOR_CHANGE_COMPLETED,

  NL80211_CMD_SET_FILS_AAD,

  NL80211_CMD_ASSOC_COMEBACK,

  NL80211_CMD_ADD_LINK,
  NL80211_CMD_REMOVE_LINK,

  NL80211_CMD_ADD_LINK_STA,
  NL80211_CMD_MODIFY_LINK_STA,
  NL80211_CMD_REMOVE_LINK_STA,

  NL80211_CMD_SET_HW_TIMESTAMP,

  NL80211_CMD_LINKS_REMOVED,

  NL80211_CMD_SET_TID_TO_LINK_MAPPING,

  /* add new commands above here */

  /* used to define NL80211_CMD_MAX below */
  __NL80211_CMD_AFTER_LAST,
  NL80211_CMD_MAX = __NL80211_CMD_AFTER_LAST - 1
};

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
