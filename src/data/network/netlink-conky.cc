#include "netlink-conky.h"

#include "../../logging.h"

#include <array>
#include <initializer_list>
#include <variant>

using namespace conky::netlink;

void nle_err_format(const char *desc, int error) {
  int index = error;
  if (error < 0) { index = -error; }
  if (index > NLE_MAX) {
    NORM_ERR("%s; error %d", desc, index);
  } else {
    NORM_ERR("%s; error %d (%s)", desc, index, NLE_ERROR_MSG[index]);
  }
}

// Has to be a macro because we want to return from outer scope
#define CHECK_RESPONSE(response, msg)               \
  (response < 0) { nle_err_format(msg, response); } \
  if (response < 0)

template <size_t Size>
class nested_attributes {
  std::array<struct nla_policy, Size + 1> value_policies;

 public:
  constexpr nested_attributes(
      const std::initializer_list<
          std::pair<size_t, std::variant<uint16_t, nla_policy>>> &data) {
    for (const auto &kv : data) {
      if (std::holds_alternative<uint16_t>(kv.second)) {
        value_policies[kv.first] =
            nla_policy{.type = std::get<uint16_t>(kv.second)};
      } else if (std::holds_alternative<nla_policy>(kv.second)) {
        value_policies[kv.first] = std::get<nla_policy>(kv.second);
      } else {
        // caused when assignment throws an exception
        // shouldn't happen as nla_policy is a POD type
        NORM_ERR("erroneous nla_policy variant value: %d", kv.first);
      }
    }
  }

  inline nla_policy *policies() { return this->value_policies.data(); }

  class value_index {
    std::array<struct nlattr *, Size + 1> values;

   public:
    inline std::array<struct nlattr *, Size + 1> &attributes() {
      return this->values.data();
    }

    inline struct nlattr *attribute(size_t index) {
      return this->values[index];
    }
  };

  inline value_index parse(struct nlattr *attrib) {
    value_index result;
    nla_parse_nested(result.values, Size, attrib, value_policies.data());
    return result;
  }
};

template <typename T, typename Tuple>
auto push_front(const T &t, const Tuple &tuple) {
  return std::tuple_cat(std::make_tuple(t), tuple);
}
template <typename Tuple, std::size_t... Is>
auto copy_tail_impl(const Tuple &tuple, std::index_sequence<Is...>) {
  return std::make_tuple(std::get<1 + Is>(tuple)...);
}
template <typename Tuple>
auto copy_tail(const Tuple &tuple) {
  return copy_tail_impl(
      tuple, std::make_index_sequence<std::tuple_size<Tuple>::value - 1>());
}

template <typename R, typename... Args>
int nl_task<R, Args...>::valid_handler(struct nl_msg *msg, void *arg) {
  auto *task = static_cast<nl_task<R, Args...> *>(arg);
  nl_task<R, Args...>::processor_args args =
      task->arguments.load(std::memory_order_acquire);
  if (args == nullptr) {
    NORM_ERR("no arguments provided to callback");
    return NL_STOP;
  }
  std::promise<R> promise = std::move(std::get<0>(*args));
  callback_result<R> result = std::apply(
      task->processor, std::tuple_cat(std::make_tuple(msg), copy_tail(*args)));
  DBGP2("SETTING P %lx", &promise);
  promise.set_value(result.value);
  return result.action;
}
template <typename R, typename... Args>
int nl_task<R, Args...>::finish_handler(struct nl_msg *msg, void *arg) {
  // FIXME: DELETE ARGUMENTS
  return NL_SKIP;
}
template <typename R, typename... Args>
int nl_task<R, Args...>::invalid_handler(struct nl_msg *msg, void *arg) {
  // Future will never resolve; prevents us from sending more invalid requests
  return NL_SKIP;
}

template <typename R, typename... Args>
nl_task<R, Args...>::nl_task(int family, uint8_t request,
                             response_proc processor)
    : family(family), request(request), processor(processor) {
  this->cb = nl_cb_alloc(NL_CB_DEFAULT);
  if (!this->cb) {
    NORM_ERR("unable to allocate netlink callback.");
    return;
  }

  nl_cb_set(this->cb, NL_CB_VALID, NL_CB_CUSTOM,
            &nl_task<R, Args...>::valid_handler, static_cast<void *>(this));
  nl_cb_set(this->cb, NL_CB_FINISH, NL_CB_CUSTOM,
            &nl_task<R, Args...>::finish_handler, nullptr);
  nl_cb_set(this->cb, NL_CB_INVALID, NL_CB_CUSTOM,
            &nl_task<R, Args...>::invalid_handler, nullptr);
};

template <typename R, typename... Args>
nl_task<R, Args...>::~nl_task() {
  nl_cb_put(this->cb);
};

template <typename R, typename... Args>
std::optional<R> nl_task<R, Args...>::send_message(struct nl_sock *sock,
                                                   msg_cfg configure,
                                                   Args &...args) {
  if (this->previous_request.has_value()) {
    std::future<R> &prev = this->previous_request.value();
    if (prev.valid() &&
        prev.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      return prev.get();
    }
  }

  std::promise<R> result_promise;
  auto result_future = result_promise.get_future();

  auto value = std::make_tuple(std::move(result_promise),
                               Args(args)...);  // FIXME: leaked
  this->arguments.store(&value, std::memory_order_release);

  struct nl_msg *msg = nlmsg_alloc();
  if (!msg) {
    NORM_ERR("failed to allocate netlink message.");
    return std::nullopt;
  }

  // if (w->ifindex < 0) { return -1; }

  genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, this->family, 0, NLM_F_DUMP,
              this->request, 0);
  configure(msg);

  nl_send_auto(sock, msg);
  nlmsg_free(msg);

  nl_recvmsgs(sock, this->cb);

  this->previous_request = std::move(result_future);
  return std::nullopt;
}

int ieee80211_frequency_to_channel(int freq) {
  /* see 802.11-2007 17.3.8.3.2 and Annex J */
  if (freq == 2484) return 14;
  /* see 802.11ax D6.1 27.3.23.2 and Annex E */
  else if (freq == 5935)
    return 2;
  else if (freq < 2484)
    return (freq - 2407) / 5;
  else if (freq >= 4910 && freq <= 4980)
    return (freq - 4000) / 5;
  else if (freq < 5950)
    return (freq - 5000) / 5;
  else if (freq <= 45000) /* DMG band lower limit */
    /* see 802.11ax D6.1 27.3.23.2 */
    return (freq - 5950) / 5;
  else if (freq >= 58320 && freq <= 70200)
    return (freq - 56160) / 2160;
  else
    return 0;
}

std::string mac_addr_to_str(const uint8_t *addr) {
  static const size_t ETH_ALEN = 6;

  std::string result;
  result.reserve(ETH_ALEN * 2 + (ETH_ALEN - 1));

  char buffer[4];
  snprintf(buffer, sizeof(buffer), "%02x", addr[0]);
  result += buffer;
  for (size_t i = 1; i < ETH_ALEN; i++) {
    snprintf(buffer, sizeof(buffer), ":%02x", addr[i]);
    result += buffer;
  }

  return result;
}

std::string ssid_to_utf8(const uint8_t len, const uint8_t *data) {
  std::string result;
  result.reserve(len);

  for (int i = 0; i < len; i++) {
    auto curr = std::string(result.begin(), result.end());
    if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\') {
      result.push_back(static_cast<char>(data[i]));
    } else if (data[i] == ' ' && (i != 0 && i != len - 1)) {
      result.push_back(' ');
    } else {
      char buffer[5];
      snprintf(buffer, sizeof(buffer), "\\x%.2x", data[i]);
      result += buffer;
    }
  }

  return result;
}

callback_result<interface_result> interface_callback(struct nl_msg *msg,
                                                     net_stat *ns) {
  struct genlmsghdr *gnlh =
      static_cast<struct genlmsghdr *>(nlmsg_data(nlmsg_hdr(msg)));
  struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
  const char *indent = "";
  nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
            genlmsg_attrlen(gnlh, 0), NULL);
  DBGP2("INTERFACE %d", nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]));
  DBGP2("B");
  if (tb_msg[NL80211_ATTR_MAC]) {
    DBGP2("By");
    ns->ap = mac_addr_to_str(
        static_cast<uint8_t *>(nla_data(tb_msg[NL80211_ATTR_MAC])));
  }
  DBGP2("C");

  if (tb_msg[NL80211_ATTR_SSID]) {
    ns->essid = "temp";
    ns->essid = ssid_to_utf8(
        nla_len(tb_msg[NL80211_ATTR_SSID]),
        static_cast<uint8_t *>(nla_data(tb_msg[NL80211_ATTR_SSID])));
    DBGP2("Cd %s", ns->essid.c_str());
  }

  DBGP2("D");
  if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
    DBGP2("Dy");
    uint32_t freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);

    snprintf(&ns->freq[0], 16, "%d MHz", freq);
    ns->channel = ieee80211_frequency_to_channel(freq);
  }
  DBGP2("E");

  return callback_result<interface_result>{.action = NL_SKIP,
                                           .value = interface_result{}};
}

callback_result<station_result> station_callack(struct nl_msg *msg,
                                                net_stat *ns) {
  /*
  ns->bitrate NL80211_STA_INFO_TX_BITRATE
  ns->link_qual NL80211_STA_INFO_SIGNAL
  ns->link_qual_max NL80211_STA_INFO_SIGNAL_AVG
  */
  return callback_result<station_result>{.action = NL_SKIP,
                                         .value = station_result{}};
}

void net_device_cache::setup_callbacks() {
  this->interface_data_cb = new nl_task<interface_result, net_stat *>(
      this->id_nl80211, NL80211_CMD_GET_INTERFACE, interface_callback);
  this->station_data_cb = new nl_task<station_result, net_stat *>(
      this->id_nl80211, NL80211_CMD_GET_STATION, station_callack);
}

net_device_cache::net_device_cache() {
  this->socket_route = nl_socket_alloc();
  if (this->socket_route == nullptr) {
    NORM_ERR("unable to allocate routing netlink socket");
    return;
  }

  nl_socket_set_buffer_size(this->socket_route, 8192, 8192);
  int resp = nl_connect(this->socket_route, NETLINK_ROUTE);
  if CHECK_RESPONSE (resp, "routing socket already connected") return;

  resp = rtnl_link_alloc_cache(this->socket_route, AF_UNSPEC, &this->nl_cache);
  if CHECK_RESPONSE (resp, "can't allocate netlink device cache") return;

  this->nl_cache_size = nl_cache_nitems(nl_cache);

  this->socket_genl = nl_socket_alloc();
  if (this->socket_genl == nullptr) {
    NORM_ERR("unable to allocate generic link netlink socket");
    return;
  }

  nl_socket_set_buffer_size(this->socket_genl, 8192, 8192);
  resp = nl_connect(this->socket_genl, NETLINK_GENERIC);
  if CHECK_RESPONSE (resp, "generic link socket already connected") return;

  this->id_nl80211 = genl_ctrl_resolve(this->socket_genl, "nl80211");
  if CHECK_RESPONSE (this->id_nl80211, "nl80211 module not loaded") return;

  this->setup_callbacks();
}

net_device_cache::~net_device_cache() {
  if (this->station_data_cb != nullptr) { delete this->station_data_cb; }
  if (this->interface_data_cb != nullptr) { delete this->interface_data_cb; }
  if (this->nl_cache != nullptr) { nl_cache_free(this->nl_cache); }
  if (this->socket_route != nullptr) {
    nl_close(this->socket_route);
    nl_socket_free(this->socket_route);
  }
  if (this->socket_genl != nullptr) {
    nl_close(this->socket_genl);
    nl_socket_free(this->socket_genl);
  }
}

struct rtnl_link *net_device_cache::get_link(const nl_interface_id &id) {
  if (this->nl_cache == nullptr) { return nullptr; }
  if (std::holds_alternative<int>(id)) {
    return rtnl_link_get(this->nl_cache, std::get<int>(id));
  } else if (std::holds_alternative<char *>(id)) {
    return rtnl_link_get_by_name(this->nl_cache, std::get<char *>(id));
  } else if (std::holds_alternative<std::string>(id)) {
    return rtnl_link_get_by_name(this->nl_cache,
                                 std::get<std::string>(id).c_str());
  } else {
    DBGP("invalid nl_interface_id variant");
    return nullptr;
  }
}

void net_device_cache::update() {
  if (this->nl_cache != nullptr) {
    nl_cache_refill(this->socket_route, this->nl_cache);
    this->nl_cache_size = nl_cache_nitems(this->nl_cache);
  }
}

/*
void parse_rate_info(struct nlattr *bitrate_attr, char *buf, int buflen) {
  int rate = 0;
  char *pos = buf;
  struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
  static auto rate_policy = nla_policy_cache<NL80211_RATE_INFO_MAX + 1>{
      {{NL80211_RATE_INFO_BITRATE, NLA_U16},
       {NL80211_RATE_INFO_BITRATE32, NLA_U32},
       {NL80211_RATE_INFO_MCS, NLA_U8},
       {NL80211_RATE_INFO_40_MHZ_WIDTH, NLA_FLAG},
       {NL80211_RATE_INFO_SHORT_GI, NLA_FLAG}}};

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
*/

void net_device_cache::populate_interface(struct net_stat *ns,
                                          const nl_interface_id &link) {
  struct rtnl_link *nl_link = this->get_link(link);
  if (nl_link == nullptr) return;

  uint32_t id = rtnl_link_get_ifindex(nl_link);

  // The following properties are already in cache and cheap to access:
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

  this->interface_data_cb->send_message(
      this->socket_genl,
      [&](struct nl_msg *msg) { nla_put_u32(msg, NL80211_ATTR_IFINDEX, id); },
      ns);
}