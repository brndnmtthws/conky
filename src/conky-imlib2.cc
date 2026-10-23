/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2024 Brenden Matthews, et. al.
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

#include "conky-imlib2.h"

#include "common.h"
#include "conky.h"
#include "content/text_object.h"
#include "logging.h"
#include "output/display-output.hh"

#include <Imlib2.h>
#ifdef BUILD_WAYLAND
#include <cairo.h>
#endif
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

#include "lua/x11-settings.h"
#ifdef BUILD_X11
#include "output/x11.h"
#endif

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
std::array<std::array<int, 2>, 100> saved_coordinates;

/* areas to update */
Imlib_Updates updates, current_update;
/* our virtual framebuffer image we draw into */
Imlib_Image buffer, image;

conky::range_config_setting<unsigned int> imlib_cache_flush_interval(
    "imlib_cache_flush_interval", 0, std::numeric_limits<unsigned int>::max(),
    0, true);

conky::simple_config_setting<bool> imlib_draw_blended("draw_blended", true,
                                                      true);

conky::range_config_setting<unsigned long> imlib_cache_size(
    "imlib_cache_size", 0, std::numeric_limits<unsigned long>::max(),
    4096 * 1024, true);

namespace {
Imlib_Context context;

unsigned int cimlib_cache_flush_last = 0;

/// Creates and pushes the imlib context shared by all backends.  Binding it to
/// an X11 drawable is left to the caller, since only backends rendering
/// through an X11 drawable have one.
void cimlib_create_context() {
  image_list_start = image_list_end = nullptr;
  context = imlib_context_new();
  imlib_context_push(context);
  imlib_set_cache_size(imlib_cache_size.get(*state));
  /* set the maximum number of colors to allocate for 8bpp and less to 256 */
  imlib_set_color_usage(256);
  /* dither for depths < 24bpp */
  imlib_context_set_dither(1);
}
}  // namespace

#ifdef BUILD_X11
void cimlib_init() {
  if (display == nullptr || window.visual == nullptr) { return; }

  if (context == nullptr) { cimlib_create_context(); }
  /* set the display , visual, colormap and drawable we are using */
  imlib_context_set_display(display);
  imlib_context_set_visual(window.visual);
  imlib_context_set_colormap(window.colourmap);
  imlib_context_set_drawable(window.drawable);
}

#endif

void cimlib_init_cpu() {
  if (context != nullptr) { return; }

  cimlib_create_context();
}

void cimlib_deinit() {
  if (context == nullptr) { return; }
  cimlib_cleanup();
#ifdef BUILD_X11
  if (imlib_context_get_display() != nullptr) {
    imlib_context_disconnect_display();
  }
#endif
  imlib_context_pop();
  imlib_context_free(context);
  context = nullptr;
}

void cimlib_cleanup() {
  struct image_list_s *cur = image_list_start, *last = nullptr;
  while (cur != nullptr) {
    last = cur;
    cur = last->next;
    delete last;
  }
  image_list_start = image_list_end = nullptr;
}

void cimlib_add_image(const char *args) {
  struct image_list_s *cur = nullptr;
  const char *tmp;

  cur = new image_list_s{};

  if (sscanf(args, "%1023s", cur->name) == 0) {
    LOG_ERROR(
        "invalid args for $image, format is '<path to image> (-p x,y) (-s WxH) "
        "(-n) (-f interval)' (got '{}')",
        args);
    delete cur;
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
    cur->x = dpi_scale(cur->x);
    cur->y = dpi_scale(cur->y);
  }
  tmp = strstr(args, "-s ");
  if (tmp != nullptr) {
    tmp += 3;
    if (sscanf(tmp, "%ix%i", &cur->w, &cur->h) != 0) { cur->wh_set = 1; }
    cur->w = dpi_scale(cur->w);
    cur->h = dpi_scale(cur->h);
  }

  tmp = strstr(args, "-n");
  if (tmp != nullptr) { cur->no_cache = 1; }

  tmp = strstr(args, "-f ");
  if (tmp != nullptr) {
    tmp += 3;
    if (sscanf(tmp, "%d", &cur->flush_interval) != 0) { cur->no_cache = 0; }
  }
  tmp = strstr(args, "-i ");
  if (tmp != nullptr) {
    tmp += 3;
    int i;
    if (sscanf(tmp, "%d", &i) == 1) {
      const auto &coordinates = saved_coordinates.at(static_cast<size_t>(i));
      cur->x = coordinates[0];
      cur->y = coordinates[1];
    }
  }
  if (cur->flush_interval < 0) {
    LOG_WARNING("flush interval should be >= 0, got {}", cur->flush_interval);
    cur->flush_interval = 0;
  }

  if (image_list_end != nullptr) {
    image_list_end->next = cur;
    image_list_end = cur;
  } else {
    image_list_start = image_list_end = cur;
  }
}

#ifdef BUILD_X11
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
    if (rep == 0) { LOG_ERROR("unable to load image '{}'", cur->name); }
    rep = 1;
    return;
  }
  rep = 0; /* reset so disappearing images are reported */

  LOG_DEBUG(
      "drawing image '{}' at ({},{}) scaled to {}x{}, cache interval {} "
      "(no_cache {})",
      cur->name, cur->x, cur->y, cur->w, cur->h, cur->flush_interval,
      cur->no_cache);

  imlib_context_set_image(image);
  /* turn alpha channel on */
  imlib_image_set_has_alpha(1);
  w = imlib_image_get_width();
  h = imlib_image_get_height();
  if (cur->wh_set == 0) {
    cur->w = dpi_scale(w);
    cur->h = dpi_scale(h);
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
  if (cur->x < *clip_x) { *clip_x = cur->x; }
  if (cur->y < *clip_y) { *clip_y = cur->y; }
  if (cur->x + cur->w > *clip_x2) { *clip_x2 = cur->x + cur->w; }
  if (cur->y + cur->h > *clip_y2) { *clip_y2 = cur->y + cur->h; }
}

static void cimlib_draw_all(int *clip_x, int *clip_y, int *clip_x2,
                            int *clip_y2) {
  struct image_list_s *cur = image_list_start;
  while (cur != nullptr) {
    cimlib_draw_image(cur, clip_x, clip_y, clip_x2, clip_y2);
    cur = cur->next;
  }
}

#endif /* BUILD_X11 */

#ifdef BUILD_WAYLAND
namespace {
/// Loads @a cur and composites it onto @a cr, offset by (@a x, @a y) and scaled
/// to the size requested through `${image <file> -s WxH}'.
///
/// This is the cairo counterpart of cimlib_draw_image(): imlib2 cannot render
/// to anything but an X11 drawable, so the pixels are handed over to cairo
/// instead.  imlib2 stores them unpremultiplied while cairo's ARGB32 requires
/// premultiplied alpha, hence the conversion buffer.
void cimlib_draw_image_cairo(cairo_t *cr, struct image_list_s *cur, int x,
                             int y, bool draw_blended) {
  static int rep = 0;
  time_t now = time(nullptr);

  Imlib_Image img = imlib_load_image(cur->name);
  if (img == nullptr) {
    if (rep == 0) { LOG_ERROR("unable to load image '{}'", cur->name); }
    rep = 1;
    return;
  }
  rep = 0; /* reset so disappearing images are reported */

  imlib_context_set_image(img);
  const bool has_alpha = imlib_image_has_alpha() != 0;
  const int w = imlib_image_get_width();
  const int h = imlib_image_get_height();
  if (cur->wh_set == 0) {
    cur->w = dpi_scale(w);
    cur->h = dpi_scale(h);
  }

  LOG_DEBUG(
      "drawing image '{}' via cairo at ({},{}) scaled to {}x{}, cache interval "
      "{} (no_cache {})",
      cur->name, cur->x, cur->y, cur->w, cur->h, cur->flush_interval,
      cur->no_cache);

  if (w > 0 && h > 0 && cur->w > 0 && cur->h > 0) {
    const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    const DATA32 *data = imlib_image_get_data_for_reading_only();
    if (stride < 0 || data == nullptr) {
      LOG_ERROR("unable to read image '{}' for cairo", cur->name);
      imlib_free_image();
      return;
    }
    std::vector<unsigned char> premultiplied(static_cast<size_t>(stride) * h);
    for (int row = 0; row < h; row++) {
      auto *dst = reinterpret_cast<DATA32 *>(premultiplied.data() +
                                             static_cast<size_t>(row) * stride);
      const DATA32 *src = data + static_cast<size_t>(row) * w;
      for (int col = 0; col < w; col++) {
        const DATA32 pixel = src[col];
        /* imlib2 keeps alpha in the most significant byte */
        const uint32_t alpha =
            draw_blended && has_alpha ? (pixel >> 24) : 0xffU;
        const auto red =
            static_cast<DATA32>(((pixel >> 16) & 0xffU) * alpha / 0xffU);
        const auto green =
            static_cast<DATA32>(((pixel >> 8) & 0xffU) * alpha / 0xffU);
        const auto blue = static_cast<DATA32>((pixel & 0xffU) * alpha / 0xffU);
        dst[col] = (static_cast<DATA32>(alpha) << 24) | (red << 16) |
                   (green << 8) | blue;
      }
    }

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        premultiplied.data(), CAIRO_FORMAT_ARGB32, w, h, stride);
    if (cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS) {
      cairo_pattern_t *pattern = cairo_pattern_create_for_surface(surface);
      cairo_pattern_set_filter(pattern, CAIRO_FILTER_GOOD);

      cairo_save(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
      cairo_translate(cr, x + cur->x, y + cur->y);
      cairo_rectangle(cr, 0, 0, cur->w, cur->h);
      cairo_clip(cr);
      cairo_scale(cr, static_cast<double>(cur->w) / w,
                  static_cast<double>(cur->h) / h);
      cairo_set_source(cr, pattern);
      cairo_paint(cr);
      cairo_restore(cr);

      cairo_pattern_destroy(pattern);
    } else {
      LOG_ERROR("unable to create cairo surface for image '{}': {}", cur->name,
                cairo_status_to_string(cairo_surface_status(surface)));
    }
    cairo_surface_destroy(surface);
  }

  if ((cur->no_cache != 0) ||
      ((cur->flush_interval != 0) && now % cur->flush_interval == 0)) {
    imlib_free_image_and_decache();
  } else {
    imlib_free_image();
  }
}
}  // namespace

void cimlib_render_cairo(cairo_t *cr, int x, int y, uint32_t flush_interval,
                         bool draw_blended) {
  if (cr == nullptr) { return; }

  if (image_list_start == nullptr) {
    return; /* are we actually drawing anything? */
  }

  /* cheque if it's time to flush our cache */
  time_t now = time(nullptr);
  if ((flush_interval != 0u) &&
      now - flush_interval > cimlib_cache_flush_last) {
    int size = imlib_get_cache_size();
    imlib_set_cache_size(0);
    imlib_set_cache_size(size);
    cimlib_cache_flush_last = now;
    LOG_DEBUG("flushing imlib2 cache ({})", now);
  }

  /* Unlike cimlib_render() there is no damage rectangle to compute here:
   * cairo clips every operation to the extents of the target surface. */
  struct image_list_s *cur = image_list_start;
  while (cur != nullptr) {
    cimlib_draw_image_cairo(cr, cur, x, y, draw_blended);
    cur = cur->next;
  }
}

#endif /* BUILD_WAYLAND */

#ifdef BUILD_X11
void cimlib_render(int x, int y, int width, int height, uint32_t flush_interval,
                   bool draw_blended) {
  int clip_x = INT_MAX, clip_y = INT_MAX;
  int clip_x2 = 0, clip_y2 = 0;
  time_t now;

  /* the input shape only counts images drawn in this frame */
  x11_image_alpha_reset();

  if (image_list_start == nullptr) {
    return; /* are we actually drawing anything? */
  }

  /* cheque if it's time to flush our cache */
  now = time(nullptr);
  if ((flush_interval != 0u) &&
      now - flush_interval > cimlib_cache_flush_last) {
    int size = imlib_get_cache_size();
    imlib_set_cache_size(0);
    imlib_set_cache_size(size);
    cimlib_cache_flush_last = now;
    LOG_DEBUG("flushing imlib2 cache ({})", now);
  }

  /* take all the little rectangles to redraw and merge them into
   * something sane for rendering */
  buffer = imlib_create_image(width, height);
  /* clear our buffer */
  imlib_context_set_image(buffer);
  imlib_image_clear();

  /* check if we should blend when rendering */
  if (draw_blended) {
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
  if (clip_x == INT_MAX) { clip_x = 0; }
  if (clip_y == INT_MAX) { clip_y = 0; }

  /* imlib2 writes only RGB to the drawable, so tell the input shaping code
   * which pixels of this buffer are not transparent */
  {
    const int bw = imlib_image_get_width();
    const int bh = imlib_image_get_height();
    const int sx0 = std::max(clip_x, 0), sy0 = std::max(clip_y, 0);
    const int sx1 = std::min(clip_x2, bw), sy1 = std::min(clip_y2, bh);
    const DATA32 *data = imlib_image_get_data_for_reading_only();
    if (data != nullptr && sx1 > sx0 && sy1 > sy0) {
      x11_image_alpha_add(x + sx0, y + sy0, sx1 - sx0, sy1 - sy0, bw,
                          reinterpret_cast<const uint32_t *>(data) +
                              static_cast<size_t>(sy0) * bw + sx0);
    }
  }

  /* render the image at 0, 0 */
  imlib_render_image_part_on_drawable_at_size(
      clip_x, clip_y, clip_x2 - clip_x, clip_y2 - clip_y, x + clip_x,
      y + clip_y, clip_x2 - clip_x, clip_y2 - clip_y);
  /* don't need that temporary buffer image anymore */
  imlib_free_image();
}

#endif /* BUILD_X11 */

void print_image_callback(struct text_object *obj, char *, unsigned int) {
  cimlib_add_image(obj->data.s);
}
