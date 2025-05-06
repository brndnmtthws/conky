#include "config.h"

#ifndef _CONKY_NETLINK_H_
#define _CONKY_NETLINK_H_

extern "C" {
#include <netlink/attr.h>
#include <netlink/cache.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/route/link.h>

#include <linux/ieee80211.h>
#include <uapi/linux/nl80211.h>
}

#include "net_stat.h"

#include <atomic>
#include <functional>
#include <future>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace conky::netlink {

using nl_interface_id = std::variant<int, char *, std::string>;

template <typename R>
struct callback_result {
  int action;
  R value;
};

using msg_cfg = std::function<void(struct nl_msg *)>;

template <typename R, typename... Args>
struct nl_task {
  using response_proc =
      std::function<callback_result<R>(struct nl_msg *, Args...)>;
  using processor_args = std::tuple<std::promise<R>, Args...> *;

 private:
  struct nl_cb *cb;
  std::atomic<processor_args> arguments;

  int family;
  uint8_t request;
  std::optional<std::future<R>> previous_request;
  response_proc processor;

  static int valid_handler(struct nl_msg *msg, void *arg);
  static int finish_handler(struct nl_msg *msg, void *arg);
  static int invalid_handler(struct nl_msg *msg, void *arg);

 public:
  nl_task(int family, uint8_t request, response_proc processor);
  ~nl_task();

  std::optional<R> send_message(struct nl_sock *sock, msg_cfg configure,
                                Args &...pargs);
};

struct interface_result {
  std::optional<std::string> mac;
  std::optional<std::string> ssid;
  std::optional<std::string> freq;
  std::optional<std::string> channel;
};
struct station_result {};

class net_device_cache {
  struct nl_sock *socket_route;
  struct nl_sock *socket_genl;

  struct nl_cache *nl_cache;
  int nl_cache_size;

  int id_nl80211;

  nl_task<interface_result, net_stat *> *interface_data_cb;
  nl_task<station_result, net_stat *> *station_data_cb;

  void setup_callbacks();

 public:
  net_device_cache();
  ~net_device_cache();

  /// @brief Update link cache and device information.
  void update();

  struct rtnl_link *get_link(const nl_interface_id &id);

  /// @brief Populate `net_stat` interface from netlink cache.
  ///
  /// @param ns interface stats struct to populate
  /// @param link index or name of link
  void populate_interface(struct net_stat *ns, const nl_interface_id &link);
};

static const std::array<const char *, NLE_MAX + 1> NLE_ERROR_MSG{
    "success",
    "unspecific failure",
    "interrupted system call",
    "bad socket",
    "try again",
    "out of memory",
    "object exists",
    "invalid input data or parameter",
    "input data out of range",
    "message size not sufficient",
    "operation not supported",
    "address family not supported",
    "object not found",
    "attribute not available",
    "missing attribute",
    "address family mismatch",
    "message sequence number mismatch",
    "kernel reported message overflow",
    "kernel reported truncated message",
    "invalid address for specified address family",
    "source based routing not supported",
    "netlink message is too short",
    "netlink message type is not supported",
    "object type does not match cache",
    "unknown or invalid cache type",
    "object busy",
    "protocol mismatch",
    "no access",
    "operation not permitted",
    "unable to open packet location file",
    "unable to parse object",
    "no such device",
    "immutable attribute",
    "dump inconsistency detected, interrupted",
    "attribute max length exceeded",
};
}  // namespace conky::netlink

#endif /* _CONKY_NETLINK_H_ */
