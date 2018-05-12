/*
 */

#if defined(BUILD_NCURSES) && !defined(CONKY_NC_H)
#define CONKY_NC_H

#include <ncurses.h>

#include "setting.hh"

#ifdef LEAKFREE_NCURSES
extern "C" {
void _nc_free_and_exit(int);
}
#endif

namespace priv {
class out_to_ncurses_setting : public conky::simple_config_setting<bool> {
  typedef conky::simple_config_setting<bool> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init);
  virtual void cleanup(lua::state &l);

 public:
  out_to_ncurses_setting() : Base("out_to_ncurses", false, false) {}
};
}  // namespace priv

extern priv::out_to_ncurses_setting out_to_ncurses;

#endif /* CONKY_NC_H */
