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
 * Copyright (c) 2005-2019 Brenden Matthews, Philip Kovacs, et. al.
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
#include "logging.h"

unsigned int selected_font = 0;
std::vector<font_list> fonts;
char fontloaded = 0;

void font_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && out_to_x.get(*state)) {
    if (fonts.empty()) { fonts.resize(1); }
    fonts[0].name = do_convert(l, -1).first;
  }

  ++s;
}

font_setting font;

conky::simple_config_setting<std::string> font_template[10] = {
    {"font0", ""}, {"font1", ""}, {"font2", ""}, {"font3", ""}, {"font4", ""},
    {"font5", ""}, {"font6", ""}, {"font7", ""}, {"font8", ""}, {"font9", ""}};

#ifdef BUILD_XFT
namespace {
class xftalpha_setting : public conky::simple_config_setting<float> {
  using Base = conky::simple_config_setting<float>;

 protected:
  void lua_setter(lua::state &l, bool init) override {
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

void set_font() {
#ifdef BUILD_XFT
  if (use_xft.get(*state)) { return; }
#endif /* BUILD_XFT */
  if (fonts.size() > selected_font && fonts[selected_font].font != nullptr &&
      window.gc != nullptr) {
    XSetFont(display, window.gc, fonts[selected_font].font->fid);
  }
}

void setup_fonts() {
  DBGP2("setting up fonts");
  if (!out_to_x.get(*state)) { return; }
#ifdef BUILD_XFT
  if (use_xft.get(*state)) {
    if (window.xftdraw != nullptr) {
      XftDrawDestroy(window.xftdraw);
      window.xftdraw = nullptr;
    }
    window.xftdraw = XftDrawCreate(display, window.drawable, window.visual,
                                   window.colourmap);
  }
#endif /* BUILD_XFT */
  set_font();
}

int add_font(const char *data_in) {
  if (!out_to_x.get(*state)) { return 0; }
  fonts.emplace_back();
  fonts.rbegin()->name = data_in;

  return fonts.size() - 1;
}

void free_fonts(bool utf8) {
  if (!out_to_x.get(*state)) { return; }
  for (auto &font : fonts) {
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
      if (font.font != nullptr) { XFreeFont(display, font.font); }
      if (utf8 && (font.fontset != nullptr)) {
        XFreeFontSet(display, font.fontset);
      }
    }
  }
  fonts.clear();
  selected_font = 0;
#ifdef BUILD_XFT
  if (window.xftdraw != nullptr) {
    XftDrawDestroy(window.xftdraw);
    window.xftdraw = nullptr;
  }
#endif /* BUILD_XFT */
}

void load_fonts(bool utf8) {
  DBGP2("loading fonts");
  if (!out_to_x.get(*state)) { return; }
  for (auto &font : fonts) {
#ifdef BUILD_XFT
    /* load Xft font */
    if (use_xft.get(*state)) {
      if (font.xftfont == nullptr) {
        font.xftfont = XftFontOpenName(display, screen, font.name.c_str());
      }

      if (font.xftfont != nullptr) { continue; }

      NORM_ERR("can't load Xft font '%s'", font.name.c_str());
      if ((font.xftfont = XftFontOpenName(display, screen, "courier-12")) !=
          nullptr) {
        continue;
      }

      CRIT_ERR(nullptr, nullptr, "can't load Xft font '%s'", "courier-12");

      continue;
    }
#endif
    if (utf8 && font.fontset == nullptr) {
      char **missing;
      int missingnum;
      char *missingdrawn;
      font.fontset = XCreateFontSet(display, font.name.c_str(), &missing,
                                    &missingnum, &missingdrawn);
      XFreeStringList(missing);
      if (font.fontset == nullptr) {
        NORM_ERR("can't load font '%s'", font.name.c_str());
        font.fontset = XCreateFontSet(display, "fixed", &missing, &missingnum,
                                      &missingdrawn);
        if (font.fontset == nullptr) {
          CRIT_ERR(nullptr, nullptr, "can't load font '%s'", "fixed");
        }
      }
    }
    /* load normal font */
    if ((font.font == nullptr) &&
        (font.font = XLoadQueryFont(display, font.name.c_str())) == nullptr) {
      NORM_ERR("can't load font '%s'", font.name.c_str());
      if ((font.font = XLoadQueryFont(display, "fixed")) == nullptr) {
        CRIT_ERR(nullptr, nullptr, "can't load font '%s'", "fixed");
      }
    }
  }
}

#ifdef BUILD_XFT

int font_height() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  if (use_xft.get(*state)) {
    return fonts[selected_font].xftfont->ascent +
           fonts[selected_font].xftfont->descent;
  } else {
    return fonts[selected_font].font->max_bounds.ascent +
           fonts[selected_font].font->max_bounds.descent;
  }
}

int font_ascent() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  if (use_xft.get(*state)) {
    return fonts[selected_font].xftfont->ascent;
  } else {
    return fonts[selected_font].font->max_bounds.ascent;
  }
}

int font_descent() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  if (use_xft.get(*state)) {
    return fonts[selected_font].xftfont->descent;
  } else {
    return fonts[selected_font].font->max_bounds.descent;
  }
}

#else

int font_height() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  return fonts[selected_font].font->max_bounds.ascent +
         fonts[selected_font].font->max_bounds.descent;
}

int font_ascent() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  return fonts[selected_font].font->max_bounds.ascent;
}

int font_descent() {
  if (!out_to_x.get(*state)) { return 0; }
  assert(selected_font < fonts.size());
  return fonts[selected_font].font->max_bounds.descent;
}

#endif
