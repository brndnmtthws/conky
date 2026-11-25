#ifndef CONKY_X11_SETTINGS_H
#define CONKY_X11_SETTINGS_H

#include "setting.hh"

// defined in x11.cc

#ifdef BUILD_XFT
extern conky::simple_config_setting<bool> use_xft;
#endif

extern conky::simple_config_setting<bool> use_double_buffer;

#endif /* CONKY_X11_SETTINGS_H */
