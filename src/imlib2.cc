/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2019 Brenden Matthews, et. al.
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

#include "imlib2.h"
#include "config.h"
#include "conky.h"
#include "logging.h"
#include "text_object.h"

#include <Imlib2.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "x11.h"

struct image_list_s {
  char name[1024];
  Imlib_Image image;
  int x, y, w, h;
  int wh_set;
  char no_cache;
  int flush_interval;
  struct image_list_s *next;
};

struct image_list_s *image_list_start, *image_list_end;

/* areas to update */
Imlib_Updates updates, current_update;
/* our virtual framebuffer image we draw into */
Imlib_Image buffer, image;

namespace {
Imlib_Context context;

conky::range_config_setting<unsigned int> imlib_cache_flush_interval(
    "imlib_cache_flush_interval", 0, std::numeric_limits<unsigned int>::max(),
    0, true);

unsigned int cimlib_cache_flush_last = 0;

conky::simple_config_setting<bool> draw_blended(
    "draw_blended", true, true);
}  // namespace

void imlib_cache_size_setting::lua_setter(lua::state &l, bool init) {
  lua::stack_sentry s(l, -2);

  Base::lua_setter(l, init);

  if (init && out_to_x.get(l)) {
    image_list_start = image_list_end = nullptr;
    context = imlib_context_new();
    imlib_context_push(context);
    imlib_set_cache_size(do_convert(l, -1).first);
    /* set the maximum number of colors to allocate for 8bpp and less to 256 */
    imlib_set_color_usage(256);
    /* dither for depths < 24bpp */
    imlib_context_set_dither(1);
    /* set the display , visual, colormap and drawable we are using */
    imlib_context_set_display(display);
    imlib_context_set_visual(window.visual);
    imlib_context_set_colormap(window.colourmap);
    imlib_context_set_drawable(window.drawable);
  }

  ++s;
}

void imlib_cache_size_setting::cleanup(lua::state &l) {
  lua::stack_sentry s(l, -1);

  if (out_to_x.get(l)) {
    cimlib_cleanup();
    imlib_context_disconnect_display();
    imlib_context_pop();
    imlib_context_free(context);
  }
}

void cimlib_cleanup() {
  struct image_list_s *cur = image_list_start, *last = nullptr;
  while (cur != nullptr) {
    last = cur;
    cur = last->next;
    free(last);
  }
  image_list_start = image_list_end = nullptr;
}

void cimlib_add_image(const char *args) {
  struct image_list_s *cur = nullptr;
  const char *tmp;

  cur = new struct image_list_s[sizeof(struct image_list_s)];
  memset(cur, 0, sizeof(struct image_list_s));

  if (sscanf(args, "%1023s", cur->name) == 0) {
    NORM_ERR(
        "Invalid args for $image.  Format is: '<path to image> (-p"
        "x,y) (-s WxH) (-n) (-f interval)' (got '%s')",
        args);
    delete [] cur;
    return;
  }
  strncpy(cur->name, to_real_path(cur->name).c_str(), 1024);
  cur->name[1023] = 0;
  //
  // now we check for optional args
  tmp = strstr(args, "-p ");
  if (tmp != nullptr) {
    tmp += 3;
    sscanf(tmp, "%i,%i", &cur->x, &cur->y);
  }
  tmp = strstr(args, "-s ");
  if (tmp != nullptr) {
    tmp += 3;
    if (sscanf(tmp, "%ix%i", &cur->w, &cur->h) != 0) {
      cur->wh_set = 1;
    }
  }

  tmp = strstr(args, "-n");
  if (tmp != nullptr) {
    cur->no_cache = 1;
  }

  tmp = strstr(args, "-f ");
  if (tmp != nullptr) {
    tmp += 3;
    if (sscanf(tmp, "%d", &cur->flush_interval) != 0) {
      cur->no_cache = 0;
    }
  }
  if (cur->flush_interval < 0) {
    NORM_ERR("Imlib2: flush interval should be >= 0");
    cur->flush_interval = 0;
  }

  if (image_list_end != nullptr) {
    image_list_end->next = cur;
    image_list_end = cur;
  } else {
    image_list_start = image_list_end = cur;
  }
}

static void cimlib_draw_image(struct image_list_s *cur, int *clip_x,
                              int *clip_y, int *clip_x2, int *clip_y2) {
  int w, h;
  time_t now = time(nullptr);
  static int rep = 0;

  if (imlib_context_get_drawable() != window.drawable) {
    imlib_context_set_drawable(window.drawable);
  }

  image = imlib_load_image(cur->name);
  if (image == nullptr) {
    if (rep == 0) {
      NORM_ERR("Unable to load image '%s'", cur->name);
    }
    rep = 1;
    return;
  }
  rep = 0; /* reset so disappearing images are reported */

  DBGP(
      "Drawing image '%s' at (%i,%i) scaled to %ix%i, "
      "caching interval set to %i (with -n opt %i)",
      cur->name, cur->x, cur->y, cur->w, cur->h, cur->flush_interval,
      cur->no_cache);

  imlib_context_set_image(image);
  /* turn alpha channel on */
  imlib_image_set_has_alpha(1);
  w = imlib_image_get_width();
  h = imlib_image_get_height();
  if (cur->wh_set == 0) {
    cur->w = w;
    cur->h = h;
  }
  imlib_context_set_image(buffer);
  imlib_blend_image_onto_image(image, 1, 0, 0, w, h, cur->x, cur->y, cur->w,
                               cur->h);
  imlib_context_set_image(image);
  if ((cur->no_cache != 0) ||
      ((cur->flush_interval != 0) && now % cur->flush_interval == 0)) {
    imlib_free_image_and_decache();
  } else {
    imlib_free_image();
  }
  if (cur->x < *clip_x) {
    *clip_x = cur->x;
  }
  if (cur->y < *clip_y) {
    *clip_y = cur->y;
  }
  if (cur->x + cur->w > *clip_x2) {
    *clip_x2 = cur->x + cur->w;
  }
  if (cur->y + cur->h > *clip_y2) {
    *clip_y2 = cur->y + cur->h;
  }
}

static void cimlib_draw_all(int *clip_x, int *clip_y, int *clip_x2,
                            int *clip_y2) {
  struct image_list_s *cur = image_list_start;
  while (cur != nullptr) {
    cimlib_draw_image(cur, clip_x, clip_y, clip_x2, clip_y2);
    cur = cur->next;
  }
}

void cimlib_render(int x, int y, int width, int height) {
  int clip_x = INT_MAX, clip_y = INT_MAX;
  int clip_x2 = 0, clip_y2 = 0;
  time_t now;

  if (image_list_start == nullptr) {
    return; /* are we actually drawing anything? */
  }

  /* cheque if it's time to flush our cache */
  now = time(nullptr);
  if ((imlib_cache_flush_interval.get(*state) != 0u) &&
      now - imlib_cache_flush_interval.get(*state) > cimlib_cache_flush_last) {
    int size = imlib_get_cache_size();
    imlib_set_cache_size(0);
    imlib_set_cache_size(size);
    cimlib_cache_flush_last = now;
    DBGP("Flushing Imlib2 cache (%li)\n", now);
  }

  /* take all the little rectangles to redraw and merge them into
   * something sane for rendering */
  buffer = imlib_create_image(width, height);
  /* clear our buffer */
  imlib_context_set_image(buffer);
  imlib_image_clear();

  /* check if we should blend when rendering */
  if (draw_blended.get(*state)) {
    /* we can blend stuff now */
    imlib_context_set_blend(1);
  } else {
    imlib_context_set_blend(0);
  }

  /* turn alpha channel on */
  imlib_image_set_has_alpha(1);

  cimlib_draw_all(&clip_x, &clip_y, &clip_x2, &clip_y2);

  /* set the buffer image as our current image */
  imlib_context_set_image(buffer);

  /* setup our clip rect */
  if (clip_x == INT_MAX) {
    clip_x = 0;
  }
  if (clip_y == INT_MAX) {
    clip_y = 0;
  }

  /* render the image at 0, 0 */
  imlib_render_image_part_on_drawable_at_size(
      clip_x, clip_y, clip_x2 - clip_x, clip_y2 - clip_y, x + clip_x,
      y + clip_y, clip_x2 - clip_x, clip_y2 - clip_y);
  /* don't need that temporary buffer image anymore */
  imlib_free_image();
}

void print_image_callback(struct text_object *obj, char *,
                          unsigned int) {
  cimlib_add_image(obj->data.s);
}
