/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (C) 2018-2022 François Revol et al.
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2021 Brenden Matthews, Philip Kovacs, et. al.
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

#include <config.h>

#include <SDL.h>
#include <SDL_ttf.h>
#ifdef HAVE_SDL_GFXPRIMITIVES_H
#include <SDL_gfxPrimitives.h>
#endif

#ifdef __HAIKU__
#include <FindDirectory.h>
#include <Path.h>
#endif
#ifdef HAVE_FTW_H
#include <ftw.h>
#endif
#include <regex.h>

#include <iostream>
#include <sstream>
#include <unordered_map>

#include "../conky.h"
#include "display-sdl.hh"
#include "gui.h"
#include "../lua/llua.h"
//#include "x11.h"
#include "../lua/fonts.h"
#ifdef BUILD_IMLIB2
#include "../conky-imlib2.h"
#endif /* BUILD_IMLIB2 */

/* TODO: cleanup global namespace */

#ifndef DIRSEP_CHAR
#define DIRSEP_CHAR '/'
//TODO: win

conky::simple_config_setting<bool> out_to_sdl("out_to_sdl", false, false);
#ifndef BUILD_X11
/* Catch up as a generic "out_to_gui" for now */
conky::simple_config_setting<bool> out_to_x("out_to_x", false, false);
#endif

// TODO: cleanup externs (move to conky.h ?)
#ifdef OWN_WINDOW
extern int fixed_size, fixed_pos;
#endif
extern conky::vec2i text_start;  /* text start position in window */
extern conky::vec2i text_offset; /* offset for start position */
extern conky::vec2i
    text_size; /* initially 1 so no zero-sized window is created */
extern double current_update_time, next_update_time, last_update_time;
void update_text();
extern int need_to_update;
int get_border_total();
extern conky::range_config_setting<int> maximum_width;
extern Colour current_color;
int line_width = 1;

SDL_Surface *surface = nullptr;
static std::vector<std::string> font_search_paths;
static unsigned int thefont;

// TODO: wrap the cairo_xlib_surface_create exposed to lua?
// https://www.cairographics.org/SDL/

/* for sdl_fonts */
struct sdl_font_list {
  TTF_Font *font;

  sdl_font_list()
      : font(nullptr)
  {
  }
};

static std::vector<sdl_font_list> sdl_fonts; /* indexed by selected_font */

static void SDL_create_window();

struct _sdl_stuff_s {
#if 0
  Region region;
#ifdef BUILD_XDAMAGE
  Damage damage;
  XserverRegion region2, part;
  int event_base, error_base;
#endif
#endif
} sdl_stuff;


static const char NOT_IN_SDL[] = "Not running in SDL";

__attribute__((weak)) void print_monitor(struct text_object *obj, char *p,
                                         unsigned int p_max_size) {
  (void)obj;

  if (!out_to_sdl.get(*state)) {
    strncpy(p, NOT_IN_SDL, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_monitor_number(struct text_object *obj,
                                                char *p,
                                                unsigned int p_max_size) {
  (void)obj;

  if (!out_to_sdl.get(*state)) {
    strncpy(p, NOT_IN_SDL, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop(struct text_object *obj, char *p,
                                         unsigned int p_max_size) {
  (void)obj;

  if (!out_to_sdl.get(*state)) {
    strncpy(p, NOT_IN_SDL, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop_number(struct text_object *obj,
                                                char *p,
                                                unsigned int p_max_size) {
  (void)obj;

  if (!out_to_sdl.get(*state)) {
    strncpy(p, NOT_IN_SDL, p_max_size);
    return;
  }
  snprintf(p, p_max_size, "%d", -1);
}

__attribute__((weak)) void print_desktop_name(struct text_object *obj, char *p,
                                              unsigned int p_max_size) {
  (void)obj;

  if (!out_to_sdl.get(*state)) {
    strncpy(p, NOT_IN_SDL, p_max_size);
  } else {
    strncpy(p, "NYI", p_max_size);
  }
}

#endif /* BUILD_SDL */

namespace conky {
namespace {

conky::display_output_sdl sdl_output;

}  // namespace
extern void init_sdl_output() {}

namespace priv {}  // namespace priv

#ifdef BUILD_SDL

const SDL_Color to_sdl(const Colour& c) {
  const SDL_Color sdlc = {c.red, c.green, c.blue, c.alpha};
  return sdlc;
  // We know for a fact those strucs have the same fields in the same order.
  //return *(reinterpret_cast<const SDL_Color *>(&c));
}

const Uint32 to_gfx(const Colour& c) {
  return c.red << 24 | c.green << 16 | c.blue << 8 | c.alpha;
}

template <>
void register_output<output_t::SDL>(display_outputs_t &outputs) {
  outputs.push_back(&sdl_output);
}

display_output_sdl::display_output_sdl() : display_output_base("sdl") {
  is_graphical = true;
}

bool display_output_sdl::detect() {
  if (out_to_sdl.get(*state)) {
    DBGP2("Display output '%s' enabled in config.", name.c_str());
    return true;
  }
#ifndef BUILD_X11
  if (out_to_x.get(*state)) {
    DBGP2("Display output '%s' enabled in config as fallback of 'out_to_x'.",
          name.c_str());
    return true;
  }
#endif
  return false;
}

bool display_output_sdl::initialize() {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1) {
    NORM_ERR("SDL_Init(): %s", SDL_GetError());
    return false;
  }
  TTF_Init();

  int b = border_inner_margin.get(*state) + border_width.get(*state) +
          border_outer_margin.get(*state);
  int flags = SDL_SWSURFACE | SDL_RESIZABLE;
  int bpp = 32;
  surface = SDL_SetVideoMode(b, b, bpp, flags);
  if (surface == NULL) {
    NORM_ERR("SDL_SetVideoMode: %s", SDL_GetError());
    return false;
  }

  SDL_WM_SetCaption("Conky", "Conky");

  setup_fonts();
  load_fonts(utf8_mode.get(*state));
  update_text_area(); /* to position text/window on screen */

  draw_stuff();

  selected_font = 0;
  update_text_area(); /* to get initial size of the window */

  return true;
}

bool display_output_sdl::shutdown() {
  TTF_Quit();
  SDL_Quit();
  return false;
}

static Uint32 SDL_timer_callback(Uint32, void *param) {
  SDL_PushEvent(static_cast<SDL_Event *>(param));
  return 0;
}

bool display_output_sdl::main_loop_wait(double t) {
  /* wait for SDL event or timeout */
  /* SDL_WaitEventTimeout is SDL2 so we must set a timer */
  SDL_TimerID timer;
  static const SDL_UserEvent timeout = {SDL_USEREVENT, 1, NULL, NULL};
  SDL_Event ev;
  int r;

  t = std::min(std::max(t, 0.0), active_update_interval());
  timer = SDL_AddTimer(static_cast<int>(t * 1000.0), SDL_timer_callback,
                       (void *)&timeout);

  SDL_ClearError();

  r = SDL_WaitEvent(NULL);

  SDL_RemoveTimer(timer);

  if (r == 0) {
    /* error */
    NORM_ERR("Error in SDL_WaitEvent(): %s", SDL_GetError());
  }

  if (need_to_update != 0) {
    need_to_update = 0;
    selected_font = 0;
    update_text_area();

    int changed = 0;
    int border_total = get_border_total();

    /* resize window if it isn't right size */
    if (/*(fixed_size == 0) &&*/
        (text_size.x() + 2 * border_total != surface->w ||
         text_size.y() + 2 * border_total != surface->h)) {
      int width = text_size.x() + 2 * border_total;
      int height = text_size.y() + 2 * border_total;
      int flags = SDL_SWSURFACE | SDL_RESIZABLE;
      int bpp = 32;
      printf("resize %dx%d\n", width, height);
      surface = SDL_SetVideoMode(width, height, bpp, flags);
      if (surface == NULL) {
        NORM_ERR("SDL_SetVideoMode: %s", SDL_GetError());
        return false;
      }
      draw_stuff(); /* redraw everything in our newly sized window */

      /* swap buffers */
      if (surface && (surface->flags & SDL_HWSURFACE)) SDL_Flip(surface);
      SDL_UpdateRect(surface, 0, 0, surface->w, surface->h);

      changed++;
      /* update lua window globals */
      llua_update_window_table(conky::rect<int>(text_start, text_size));
    }

    clear_text(1);
  }

  /* handle events */
  while (SDL_PollEvent(&ev) != 0) {
    //printf("ev %d\n", ev.type);

    switch (ev.type) {
      case SDL_ACTIVEEVENT:
        need_to_update = 1;
        printf("gap: %d x %d\n", gap_x.get(*state), gap_y.get(*state));
        break;
      case SDL_VIDEORESIZE:
        // TODO
        break;
      case SDL_QUIT:
        // FIXME
        g_sigterm_pending = 1;
        break;
      case SDL_USEREVENT:
        /* currently only the timeout */
        update_text();
        draw_stuff();
        SDL_UpdateRect(surface, 0, 0, surface->w, surface->h);
        //need_to_update = 1;
        break;
      default:
        break;
    }
  }

  // if (surface && (surface->flags & SDL_HWSURFACE))
  //  TODO: clear back buffer?

  // handled
  return true;
}

void display_output_sdl::sigterm_cleanup() {}

void display_output_sdl::cleanup() {
  // XXX:shutdown();
  free_fonts(utf8_mode.get(*state));
}

void display_output_sdl::set_foreground_color(Colour c) {
  current_color = c;
}

int display_output_sdl::calc_text_width(const char *s) {
  size_t slen = strlen(s);
  // TODO
  // return XTextWidth(x_fonts[selected_font].font, s, slen);
  int w, h;
  // TODO: check utf8_mode.get(*state) ?
  if (sdl_fonts[thefont].font == nullptr || TTF_SizeUTF8(sdl_fonts[thefont].font, s, &w, &h) < 0)
    return slen * 10;
  return w;
}

void display_output_sdl::draw_string_at(int x, int y, const char *s, int w) {
  if (sdl_fonts[thefont].font == nullptr)
    return;
  SDL_Color c = to_sdl(current_color);
  SDL_Surface *text = TTF_RenderText_Blended(sdl_fonts[thefont].font, s, c);
  y -= text->h;
  SDL_Rect sr = { 0, 0, (Uint16)(text->w), (Uint16)(text->h)};
  SDL_Rect dr = { (Sint16)x, (Sint16)y, (Uint16)(text->w), (Uint16)(text->h)};
  SDL_BlitSurface(text, &sr, surface, &dr);
  SDL_FreeSurface(text);
  // TODO
}

void display_output_sdl::set_line_style(int w, bool solid) {
  // TODO
  line_width = w;
}

void display_output_sdl::set_dashes(char *s) {
  // TODO
}

void display_output_sdl::draw_line(int x1, int y1, int x2, int y2) {
  printf("draw_line(%d, %d, %d, %d)\n", x1, y1, x2, y2);
  //SDL_Color c = to_sdl(current_color);
  // TODO
#ifdef HAVE_SDL_GFXPRIMITIVES_H
  Colour c = current_color;
  thickLineRGBA(surface, x1, y1, x2, y2, line_width, c.red, c.green, c.blue, c.alpha);
#endif
}

void display_output_sdl::draw_rect(int x, int y, int w, int h) {
  printf("draw_rect(%d, %d, %d, %d)\n", x, y, w, h);
#ifdef HAVE_SDL_GFXPRIMITIVES_H
  Colour c = current_color;
  rectangleRGBA(surface, x, y, x + w, y + h, c.red, c.green, c.blue, c.alpha);
#else
  SDL_Color c = to_sdl(current_color);
  SDL_Rect r = {(Sint16)x, (Sint16)y, (Uint16)w, 1};
  SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, c.r, c.g, c.b));
  r.y += h/* - 1*/;
  SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, c.r, c.g, c.b));
  r.y -= h/* - 1*/;
  r.h = h + 1;
  r.w = 1;
  SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, c.r, c.g, c.b));
  r.x += w/* - 1*/;
  SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, c.r, c.g, c.b));
#endif
}

void display_output_sdl::fill_rect(int x, int y, int w, int h) {
  printf("fill_rect(%d, %d, %d, %d)\n", x, y, w, h);
#ifdef HAVE_SDL_GFXPRIMITIVES_H
  Colour c = current_color;
  boxRGBA(surface, x, y, x + w, y + h, c.red, c.green, c.blue, c.alpha);
#else
  SDL_Color c = to_sdl(current_color);
  SDL_Rect r = {(Sint16)x, (Sint16)y, (Uint16)w, (Uint16)h};
  // TODO: fill with fg color
  SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, c.r, c.g, c.b));
#endif
}

void display_output_sdl::draw_arc(int x, int y, int w, int h, int a1, int a2) {
  printf("draw_arc(%d, %d, %d, %d, %d, %d)\n", x, y, w, h, a1, a2);
#ifdef HAVE_SDL_GFXPRIMITIVES_H
  // FIXME: does not seem to support stretched arcs
  Colour c = current_color;
  arcRGBA(surface, x, y, w/2, a1, a2, c.red, c.green, c.blue, c.alpha);
#endif
}

void display_output_sdl::move_win(int x, int y) {
  /* not possible in SDL 1.2 */
}

int display_output_sdl::dpi_scale(int value) { return value; }

void display_output_sdl::end_draw_stuff() {
  if (surface && (surface->flags & SDL_HWSURFACE)) SDL_Flip(surface);
}

void display_output_sdl::clear_text(int exposures) {
  if (surface != nullptr) {
    /* there is some extra space for borders and outlines */
    int border_total = get_border_total();

    SDL_Rect r = {(Sint16)(text_start.x() - border_total),
                  (Sint16)(text_start.y() - border_total),
                  (Uint16)(text_size.x() + 2 * border_total),
                  (Uint16)(text_size.y() + 2 * border_total)};
    // TODO: fill with bg color
    SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, 0, 0, 0));
  }
}

int display_output_sdl::font_height(unsigned int f) {
  assert(f < sdl_fonts.size());
  //  return sdl_fonts[f].font->max_bounds.ascent +
  //         sdl_fonts[f].font->max_bounds.descent;
  if (sdl_fonts[f].font == nullptr)
    return 10;
  return TTF_FontHeight(sdl_fonts[f].font);
}

int display_output_sdl::font_ascent(unsigned int f) {
  assert(f < sdl_fonts.size());
  if (sdl_fonts[f].font == nullptr)
    return 0;
  return TTF_FontAscent(sdl_fonts[f].font);
  // return x_fonts[f].font->max_bounds.ascent;
}

int display_output_sdl::font_descent(unsigned int f) {
  assert(f < sdl_fonts.size());
  // return sdl_fonts[f].font->max_bounds.descent;
  if (sdl_fonts[f].font == nullptr)
    return 0;
  return std::abs(TTF_FontDescent(sdl_fonts[f].font));
}

void display_output_sdl::setup_fonts(void) {
  /* Build up the font search paths */
#ifdef __HAIKU__
  directory_which dirs[] = {
    B_USER_NONPACKAGED_FONTS_DIRECTORY,
    B_USER_FONTS_DIRECTORY,
    B_SYSTEM_NONPACKAGED_FONTS_DIRECTORY,
    B_SYSTEM_FONTS_DIRECTORY
  };

  for (int i = 0; i < 4; i++) {
    char p[B_PATH_NAME_LENGTHrm];
    if (find_directory(dirs[i], -1, false, p, sizeof(p)) == B_OK)
      font_search_paths.push_back(p);
  }
#endif
  const char *e;
  e = getenv("HOME");
  if (e) {
    std::string s(e);
    s.append("/.fonts");
    font_search_paths.push_back(s.c_str());
  }
  // TODO: depends on the OS
  // maybe use XDG_DATA_DIRS
  // also check local user dirs
  // some fallbacks
  font_search_paths.push_back("/usr/share/fonts");
}

void display_output_sdl::set_font(unsigned int f) {
  assert(f < sdl_fonts.size());
  thefont = f;
}

void display_output_sdl::free_fonts(bool utf8) {
  for (auto &font : sdl_fonts) {
    if (font.font)
      TTF_CloseFont(font.font);
  }
  sdl_fonts.clear();
}

/*
 * We try to locate TTF files by comparing with the font name,
 * after removing spaces and other separator characters.
 */
static const char *font_name_filtered_chars = " _-";
static std::string searched_font;
static const char *fallback_fonts[] = {
  "DejaVuSans.ttf",
  nullptr
};

static int font_finder(const char *fpath, const struct stat *sb, int typeflag) {
  //printf("%s %d\n", fpath, typeflag);

  if (typeflag != FTW_F)
    return 0;

  const char *p = strrchr(fpath, DIRSEP_CHAR);
  if (p == nullptr)
    return 0;
  p++;

  std::string name;
  for (unsigned int i = 0; p[i]; i++) {
    if (strchr(font_name_filtered_chars, p[i]))
      continue;
    name += p[i];
  }
  //printf("%s\n", name.c_str());
  if (name.compare(searched_font))
    return 0;

  searched_font = fpath;
  return 1;
}

static bool find_font() {
#ifdef HAVE_FTW_H
  int found = 0;
  for (unsigned int i = 0; i < font_search_paths.size(); i++) {
    found = ftw(font_search_paths[i].c_str(), &font_finder, 20);
    //printf("err %d\nF: %s\n", found, searched_font.c_str());
    if (found == 1)
      return true;
  }
  return false;
#else
#error WRITEME
#endif
}

void display_output_sdl::load_fonts(bool utf8) {
  sdl_fonts.resize(fonts.size());

  // TODO: should we cache them?


  for (unsigned int f = 0; f < fonts.size(); f++) {
    auto &font = fonts[f];
    auto &sdlfont = sdl_fonts[f];
    sdlfont.font = nullptr;
    const char *p, *q;
    int size = 1;

    searched_font = "";
    p = font.name.c_str();
    q = strstr(p, ":size=");
    if (q)
      size = (int)strtol(q + 6, nullptr, 10);

    for (unsigned int i = 0; p[i] && (&p[i] != q); i++) {
      if (strchr(font_name_filtered_chars, p[i]))
        continue;
      searched_font += p[i];
    }
    searched_font += ".ttf";

    bool found = find_font();

    if (found) {
      sdlfont.font = TTF_OpenFont(searched_font.c_str(), size);
    } else {
      NORM_ERR("can't load font '%s' %d", searched_font.c_str(), size);
      for (unsigned int i = 0; fallback_fonts[i]; i++) {
        searched_font = fallback_fonts[i];
        found = find_font();
      }
      if (!found)
        CRIT_ERR("can't load fallback font");
    }
  }
}

#endif /* BUILD_SDL */

}  // namespace conky
