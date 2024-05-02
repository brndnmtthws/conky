#include "netlink-conky.h"

#include "logging.h"

#include <array>
#include <initializer_list>
#include <variant>

template <size_t Size>
struct nla_policy_cache {
  std::array<struct nla_policy, Size> value;

  constexpr nla_policy_cache(
      const std::initializer_list<
          std::pair<size_t, std::variant<uint16_t, nla_policy>>> &data) {
    for (const auto &kv : data) {
      if (std::holds_alternative<uint16_t>(kv.second)) {
        value[kv.first] = nla_policy{.type = std::get<uint16_t>(kv.second)};
      } else {
        value[kv.first] = std::get<nla_policy>(kv.second);
      }
    }
  }

  nla_policy *data() { return this->value.data(); }
};

template <typename Data>
nl_task<Data>::nl_task(int family, uint8_t request,
                       std::function<int(struct nl_msg *, Data *)> processor) {
  this->cb = nl_cb_alloc(NL_CB_DEFAULT);
  if (!this->cb) {
    NORM_ERR("unable to allocate netlink callback.");
    return;
  }

  const auto valid_handler = [processor = std::move(processor)](
                                 struct nl_msg *msg, void *arg) {
    return processor(msg, arg);
  };
  const auto finish_handler = [](struct nl_msg *msg, void *arg) {
    *reinterpret_cast<std::atomic<callback_state> *>(arg) =
        callback_state::DONE;
    return NL_SKIP;
  };
  const auto invalid_handler = [](struct nl_msg *msg, void *arg) {
    *reinterpret_cast<std::atomic<callback_state> *>(arg) =
        callback_state::INVALID;
    return NL_SKIP;
  };

  nl_cb_set(this->cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, &this->data);
  nl_cb_set(this->cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &state);
  nl_cb_set(this->cb, NL_CB_INVALID, NL_CB_CUSTOM, invalid_handler, &state);
}

template <typename Data>
nl_task<Data>::~nl_task() {}

void net_device_cache::setup_callbacks() {}

net_device_cache::net_device_cache() {
  this->sock = nl_socket_alloc();
  nl_socket_set_buffer_size(this->sock, 8192, 8192);

  if (!this->sock) {
    NORM_ERR("unable to connect to netlink socket.");
    return;
  }

  if (rtnl_link_alloc_cache(this->sock, AF_UNSPEC, &this->nl_cache) == 0) {
    this->nl_cache_size = nl_cache_nitems(nl_cache);
  } else {
    NORM_ERR("can't allocate netlink device cache.");
    return;
  };

  this->id_nl80211 = genl_ctrl_resolve(this->sock, "nl80211");
  if (this->id_nl80211 < 0) {
    // limited data
    DBGP("nl80211 module not loaded");
    return;
  }

  this->setup_callbacks();
}

net_device_cache::~net_device_cache() {
  if (this->nl_cache != nullptr) { nl_cache_free(this->nl_cache); }
  if (this->sock != nullptr) {
    nl_close(this->sock);
    nl_socket_free(this->sock);
  }
}

struct rtnl_link *net_device_cache::get_link(const nl_link_id &id) {
  if (std::holds_alternative<int>(id)) {
    return rtnl_link_get(this->nl_cache, std::get<int>(id));
  } else if (std::holds_alternative<char *>(id)) {
    return rtnl_link_get_by_name(this->nl_cache, std::get<char *>(id));
  } else {
    return rtnl_link_get_by_name(this->nl_cache,
                                 std::get<std::string>(id).c_str());
  }
}

void net_device_cache::update() {
  if (this->nl_cache != nullptr) {
    nl_cache_refill(this->sock, this->nl_cache);
    this->nl_cache_size = nl_cache_nitems(this->nl_cache);
  }
}

void parse_rate_info(struct nlattr *bitrate_attr, char *buf, int buflen) {
  int rate = 0;
  char *pos = buf;
  struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
  static auto rate_policy = nla_policy_cache<NL80211_RATE_INFO_MAX + 1>{
      {NL80211_RATE_INFO_BITRATE, NLA_U16},
      {NL80211_RATE_INFO_BITRATE32, NLA_U32},
      {NL80211_RATE_INFO_MCS, NLA_U8},
      {NL80211_RATE_INFO_40_MHZ_WIDTH, NLA_FLAG},
      {NL80211_RATE_INFO_SHORT_GI, NLA_FLAG}};

  if (nla_parse_nested(rinfo, NL80211_RATE_INFO_MAX, bitrate_attr,
                       rate_policy.data())) {
    snprintf(buf, buflen, "failed to parse nested rate attributes!");
    return;
  }

  if (rinfo[NL80211_RATE_INFO_BITRATE32])
    rate = nla_get_u32(rinfo[NL80211_RATE_INFO_BITRATE32]);
  else if (rinfo[NL80211_RATE_INFO_BITRATE])
    rate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);
  if (rate > 0)
    pos += snprintf(pos, buflen - (pos - buf), "%d.%d MBit/s", rate / 10,
                    rate % 10);
  else
    pos += snprintf(pos, buflen - (pos - buf), "(unknown)");

  if (rinfo[NL80211_RATE_INFO_MCS])
    pos += snprintf(pos, buflen - (pos - buf), " MCS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_MCS]));
  if (rinfo[NL80211_RATE_INFO_VHT_MCS])
    pos += snprintf(pos, buflen - (pos - buf), " VHT-MCS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_VHT_MCS]));
  if (rinfo[NL80211_RATE_INFO_40_MHZ_WIDTH])
    pos += snprintf(pos, buflen - (pos - buf), " 40MHz");
  if (rinfo[NL80211_RATE_INFO_80_MHZ_WIDTH])
    pos += snprintf(pos, buflen - (pos - buf), " 80MHz");
  if (rinfo[NL80211_RATE_INFO_80P80_MHZ_WIDTH])
    pos += snprintf(pos, buflen - (pos - buf), " 80P80MHz");
  if (rinfo[NL80211_RATE_INFO_160_MHZ_WIDTH])
    pos += snprintf(pos, buflen - (pos - buf), " 160MHz");
  if (rinfo[NL80211_RATE_INFO_320_MHZ_WIDTH])
    pos += snprintf(pos, buflen - (pos - buf), " 320MHz");
  if (rinfo[NL80211_RATE_INFO_SHORT_GI])
    pos += snprintf(pos, buflen - (pos - buf), " short GI");
  if (rinfo[NL80211_RATE_INFO_VHT_NSS])
    pos += snprintf(pos, buflen - (pos - buf), " VHT-NSS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_VHT_NSS]));
  if (rinfo[NL80211_RATE_INFO_HE_MCS])
    pos += snprintf(pos, buflen - (pos - buf), " HE-MCS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_HE_MCS]));
  if (rinfo[NL80211_RATE_INFO_HE_NSS])
    pos += snprintf(pos, buflen - (pos - buf), " HE-NSS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_HE_NSS]));
  if (rinfo[NL80211_RATE_INFO_HE_GI])
    pos += snprintf(pos, buflen - (pos - buf), " HE-GI %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_HE_GI]));
  if (rinfo[NL80211_RATE_INFO_HE_DCM])
    pos += snprintf(pos, buflen - (pos - buf), " HE-DCM %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_HE_DCM]));
  if (rinfo[NL80211_RATE_INFO_HE_RU_ALLOC])
    pos += snprintf(pos, buflen - (pos - buf), " HE-RU-ALLOC %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_HE_RU_ALLOC]));
  if (rinfo[NL80211_RATE_INFO_EHT_MCS])
    pos += snprintf(pos, buflen - (pos - buf), " EHT-MCS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_EHT_MCS]));
  if (rinfo[NL80211_RATE_INFO_EHT_NSS])
    pos += snprintf(pos, buflen - (pos - buf), " EHT-NSS %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_EHT_NSS]));
  if (rinfo[NL80211_RATE_INFO_EHT_GI])
    pos += snprintf(pos, buflen - (pos - buf), " EHT-GI %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_EHT_GI]));
  if (rinfo[NL80211_RATE_INFO_EHT_RU_ALLOC])
    pos += snprintf(pos, buflen - (pos - buf), " EHT-RU-ALLOC %d",
                    nla_get_u8(rinfo[NL80211_RATE_INFO_EHT_RU_ALLOC]));
}

void net_device_cache::populate_interface(struct net_stat *ns,
                                          const nl_link_id &link) {
  struct rtnl_link *nl_link = this->get_link(link);
  if (nl_link == nullptr) return;

  // See: http://www.infradead.org/~tgr/libnl/doc/route.html#link_object
  // clang-format off
  // uint32_t link_group = rtnl_link_get_group(nl_link);
  // struct nl_addr *link_layer_addr = rtnl_link_get_addr(nl_link);
  // struct nl_addr *broadcast_addr = rtnl_link_get_broadcast(nl_link);
  // unsigned int max_trasmission_unit = rtnl_link_get_mtu(nl_link);
  // unsigned int trasmission_queue_length = rtnl_link_get_txqlen(nl_link);
  // uint8_t operational_status = rtnl_link_get_operstate(nl_link); // up/down/dormant/etc.
  // - to string: char *rtnl_link_operstate2str(uint8_t state, char *buf, size_t size);
  // uint8_t mode = rtnl_link_get_linkmode(nl_link); // default/dormant
  // - to string: char *rtnl_link_mode2str(uint8_t mode, char *buf, size_t len);
  // char *if_alias = rtnl_link_get_ifalias(nl_link); // SNMP IfAlias.
  // unsigned int hardware_type = rtnl_link_get_arptype(nl_link);
  // - to string: char *nl_llproto2str(int arptype, char *buf, size_t len);
  // char *queueing_discipline = rtnl_link_get_qdisc(nl_link);
  // uint32_t promiscuity = rtnl_link_get_promiscuity(nl_link);
  // uint32_t tx_queues = rtnl_link_get_num_tx_queues(nl_link);
  // uint32_t rx_queues = rtnl_link_get_num_rx_queues(nl_link);
  // clang-format on

  auto modes = rtnl_link_get_flags(nl_link);
  rtnl_link_flags2str(modes, ns->mode, 64);

  /*
  TODO:
  ns->bitrate
  ns->link_qual
  ns->link_qual_max
  ns->ap
  ns->essid
  ns->channel
  ns->freq
  ns->channel
  ns->freq[0]
  */

  //   struct nlattr *tb[NL80211_ATTR_MAX + 1];
  //   struct genlmsghdr *gnlh = nullptr;  // FIXME: nlmsg_data(nlmsg_hdr(msg));
  //   struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
  //   struct nlattr *binfo[NL80211_STA_BSS_PARAM_MAX + 1];

  //   static auto stats_policy = nla_policy_cache<NL80211_STA_INFO_MAX + 1>{
  //       {NL80211_STA_INFO_INACTIVE_TIME, NLA_U32},
  //       {NL80211_STA_INFO_RX_BYTES, NLA_U32},
  //       {NL80211_STA_INFO_TX_BYTES, NLA_U32},
  //       {NL80211_STA_INFO_RX_PACKETS, NLA_U32},
  //       {NL80211_STA_INFO_TX_PACKETS, NLA_U32},
  //       {NL80211_STA_INFO_SIGNAL, NLA_U8},
  //       {NL80211_STA_INFO_RX_BITRATE, NLA_NESTED},
  //       {NL80211_STA_INFO_TX_BITRATE, NLA_NESTED},
  //       {NL80211_STA_INFO_LLID, NLA_U16},
  //       {NL80211_STA_INFO_PLID, NLA_U16},
  //       {NL80211_STA_INFO_PLINK_STATE, NLA_U8}};
  //   static auto bss_policy = nla_policy_cache<NL80211_STA_BSS_PARAM_MAX + 1>{
  //       {NL80211_STA_BSS_PARAM_CTS_PROT, NLA_FLAG},
  //       {NL80211_STA_BSS_PARAM_SHORT_PREAMBLE, NLA_FLAG},
  //       {NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME, NLA_FLAG},
  //       {NL80211_STA_BSS_PARAM_DTIM_PERIOD, NLA_U8},
  //       {NL80211_STA_BSS_PARAM_BEACON_INTERVAL, NLA_U16}};

  //   nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
  //             genlmsg_attrlen(gnlh, 0), NULL);

  //   if (!tb[NL80211_ATTR_STA_INFO]) {
  //     fprintf(stderr, "sta stats missing!\n");
  //     return;
  //   }
  //   if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
  //   tb[NL80211_ATTR_STA_INFO],
  //                        stats_policy.data())) {
  //     fprintf(stderr, "failed to parse nested attributes!\n");
  //     return;
  //   }

  //   if (sinfo[NL80211_STA_INFO_RX_BYTES] &&
  //   sinfo[NL80211_STA_INFO_RX_PACKETS])
  //     printf("\tRX: %u bytes (%u packets)\n",
  //            nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]),
  //            nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]));
  //   if (sinfo[NL80211_STA_INFO_TX_BYTES] &&
  //   sinfo[NL80211_STA_INFO_TX_PACKETS])
  //     printf("\tTX: %u bytes (%u packets)\n",
  //            nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]),
  //            nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]));
  //   if (sinfo[NL80211_STA_INFO_SIGNAL])
  //     printf("\tsignal: %d dBm\n",
  //            (int8_t)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]));

  //   if (sinfo[NL80211_STA_INFO_RX_BITRATE]) {
  //     char buf[100];

  //     parse_rate_info(sinfo[NL80211_STA_INFO_RX_BITRATE], buf, sizeof(buf));
  //     printf("\trx bitrate: %s\n", buf);
  //   }
  //   if (sinfo[NL80211_STA_INFO_TX_BITRATE]) {
  //     char buf[100];

  //     parse_rate_info(sinfo[NL80211_STA_INFO_TX_BITRATE], buf, sizeof(buf));
  //     printf("\ttx bitrate: %s\n", buf);
  //   }

  //   if (sinfo[NL80211_STA_INFO_BSS_PARAM]) {
  //     if (nla_parse_nested(binfo, NL80211_STA_BSS_PARAM_MAX,
  //                          sinfo[NL80211_STA_INFO_BSS_PARAM],
  //                          bss_policy.data())) {
  //       fprintf(stderr, "failed to parse nested bss parameters!\n");
  //     } else {
  //       printf("\n\tbss flags:\t");
  //       if (binfo[NL80211_STA_BSS_PARAM_CTS_PROT]) {
  //       printf("CTS-protection"); } if
  //       (binfo[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE]) {
  //         printf("short-preamble");
  //       }
  //       if (binfo[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME])
  //         printf("short-slot-time");
  //       printf("\n\tdtim period:\t%d",
  //              nla_get_u8(binfo[NL80211_STA_BSS_PARAM_DTIM_PERIOD]));
  //       printf("\n\tbeacon int:\t%d",
  //              nla_get_u16(binfo[NL80211_STA_BSS_PARAM_BEACON_INTERVAL]));
  //       printf("\n");
  //     }
  //   }

  // int skfd = iw_sockets_open();
  // if (iw_get_basic_config(skfd, s, &(winfo->b)) > -1) {
  //   // set present winfo variables
  //   if (iw_get_range_info(skfd, s, &(winfo->range)) >= 0) {
  //     winfo->has_range = 1;
  //   }
  //   if (iw_get_stats(skfd, s, &(winfo->stats), &winfo->range,
  //                    winfo->has_range) >= 0) {
  //     winfo->has_stats = 1;
  //   }
  //   if (iw_get_ext(skfd, s, SIOCGIWAP, &wrq) >= 0) {
  //     winfo->has_ap_addr = 1;
  //     memcpy(&(winfo->ap_addr), &(wrq.u.ap_addr), sizeof(sockaddr));
  //   }

  //   // get bitrate
  //   if (iw_get_ext(skfd, s, SIOCGIWRATE, &wrq) >= 0) {
  //     memcpy(&(winfo->bitrate), &(wrq.u.bitrate), sizeof(iwparam));
  //     iw_print_bitrate(ns->bitrate, 16, winfo->bitrate.value);
  //   }

  //   // get link quality
  //   if (winfo->has_range && winfo->has_stats) {
  //     bool has_qual_level = (winfo->stats.qual.level != 0) ||
  //                           (winfo->stats.qual.updated & IW_QUAL_DBM);

  //     if (has_qual_level &&
  //         !(winfo->stats.qual.updated & IW_QUAL_QUAL_INVALID)) {
  //       ns->link_qual = winfo->stats.qual.qual;

  //       if (winfo->range.max_qual.qual > 0) {
  //         ns->link_qual_max = winfo->range.max_qual.qual;
  //       }
  //     }
  //   }

  //   // get ap mac
  //   if (winfo->has_ap_addr) { iw_sawap_ntop(&winfo->ap_addr, ns->ap); }

  //   // get essid
  //   if (winfo->b.has_essid) {
  //     if (winfo->b.essid_on) {
  //       snprintf(ns->essid, 34, "%s", winfo->b.essid);
  //     } else {
  //       snprintf(ns->essid, 34, "%s", "off/any");
  //     }
  //   }

  //   // get channel and freq
  //   if (winfo->b.has_freq) {
  //     if (winfo->has_range == 1) {
  //       ns->channel = iw_freq_to_channel(winfo->b.freq, &(winfo->range));
  //       iw_print_freq_value(ns->freq, 16, winfo->b.freq);
  //     } else {
  //       ns->channel = 0;
  //       ns->freq[0] = 0;
  //     }
  //   }
  // }

  // iw_sockets_close(skfd);
  // free(winfo);
}