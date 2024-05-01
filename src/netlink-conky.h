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

#include <array>

#include "net_stat.h"

class net_device_cache {
  struct nl_sock *sock;
  struct nl_cache *nl_cache;
  int nl_cache_size;

 public:
  net_device_cache();
  ~net_device_cache();

  void update();
  void populate_interface(struct net_stat *ns, const char *name);
};

#endif /* _CONKY_NETLINK_H_ */
