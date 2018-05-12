/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2012 Brenden Matthews, Philip Kovacs, et. al.
 *	(see AUTHORS)
 * All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "fonts.h"
#include "conky.h"
#include "logging.h"

int selected_font = 0;
std::vector<font_list> fonts;
char fontloaded = 0;

void font_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && out_to_x.get(*state)) {
    if (fonts.size() == 0) fonts.resize(1);
    fonts[0].name = do_convert(l, -1).first;
  }

  ++s;
}

font_setting font;

#ifdef BUILD_XFT
namespace {
class xftalpha_setting : public conky::simple_config_setting<float> {
  typedef conky::simple_config_setting<float> Base;

 protected:
  virtual void lua_setter(lua::state &l, bool init) {
    lua::stack_sentry s(l, -2);

    Base::lua_setter(l, init);

    if (init && out_to_x.get(*state)) {
      fonts[0].font_alpha = do_convert(l, -1).first * 0xffff;
    }

    ++s;
  }

 public:
  xftalpha_setting() : Base("xftalpha", 1.0, false) {}
};

xftalpha_setting xftalpha;
}  // namespace
#endif /* BUILD_XFT */

void set_font(void) {
#ifdef BUILD_XFT
  if (use_xft.get(*state)) return;
#endif /* BUILD_XFT */
  if (fonts[selected_font].font) {
    XSetFont(display, window.gc, fonts[selected_font].font->fid);
  }
}

void setup_fonts(void) {
  if (not out_to_x.get(*state)) {
    return;
  }
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    if (window.xftdraw) {
      XftDrawDestroy(window.xftdraw);
      window.xftdraw = 0;
    }
    window.xftdraw = XftDrawCreate(display, window.drawable, window.visual,
                                   window.colourmap);
  }
#endif /* BUILD_XFT */
  set_font();
}

int add_font(const char *data_in) {
  if (not out_to_x.get(*state)) {
    return 0;
  }
  fonts.push_back(font_list());
  fonts.rbegin()->name = data_in;

  return fonts.size() - 1;
}

void free_fonts(bool utf8) {
  if (not out_to_x.get(*state)) {
    return;
  }
  for (size_t i = 0; i < fonts.size(); i++) {
#ifdef BUILD_XFT
    if (use_xft.get(*state)) {
      /*
       * Do we not need to close fonts with Xft? Unsure.  Not freeing the
       * fonts seems to incur a slight memory leak, but it also prevents
       * a crash.
       *
       * XftFontClose(display, fonts[i].xftfont);
       */
    } else
#endif /* BUILD_XFT */
    {
      if (fonts[i].font) {
        XFreeFont(display, fonts[i].font);
      }
      if (utf8 && fonts[i].fontset) {
        XFreeFontSet(display, fonts[i].fontset);
      }
    }
  }
  fonts.clear();
  selected_font = 0;
#ifdef BUILD_XFT
  if (window.xftdraw) {
    XftDrawDestroy(window.xftdraw);
    window.xftdraw = 0;
  }
#endif /* BUILD_XFT */
}

void load_fonts(bool utf8) {
  if (not out_to_x.get(*state)) return;
  for (size_t i = 0; i < fonts.size(); i++) {
#ifdef BUILD_XFT
    /* load Xft font */
    if (use_xft.get(*state)) {
      if (not fonts[i].xftfont)
        fonts[i].xftfont =
            XftFontOpenName(display, screen, fonts[i].name.c_str());

      if (fonts[i].xftfont) {
        continue;
      }

      NORM_ERR("can't load Xft font '%s'", fonts[i].name.c_str());
      if ((fonts[i].xftfont = XftFontOpenName(display, screen, "courier-12")) !=
          NULL) {
        continue;
      }

      CRIT_ERR(NULL, NULL, "can't load Xft font '%s'", "courier-12");

      continue;
    }
#endif
    if (utf8 && fonts[i].fontset == NULL) {
      char **missing;
      int missingnum;
      char *missingdrawn;
      fonts[i].fontset = XCreateFontSet(display, fonts[i].name.c_str(),
                                        &missing, &missingnum, &missingdrawn);
      XFreeStringList(missing);
      if (fonts[i].fontset == NULL) {
        NORM_ERR("can't load font '%s'", fonts[i].name.c_str());
        fonts[i].fontset = XCreateFontSet(display, "fixed", &missing,
                                          &missingnum, &missingdrawn);
        if (fonts[i].fontset == NULL) {
          CRIT_ERR(NULL, NULL, "can't load font '%s'", "fixed");
        }
      }
    }
    /* load normal font */
    if (!fonts[i].font && (fonts[i].font = XLoadQueryFont(
                               display, fonts[i].name.c_str())) == NULL) {
      NORM_ERR("can't load font '%s'", fonts[i].name.c_str());
      if ((fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
        CRIT_ERR(NULL, NULL, "can't load font '%s'", "fixed");
      }
    }
  }
}
