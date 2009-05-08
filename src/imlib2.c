/* Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2005-2009 Brenden Matthews, et. al.
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
#include "logging.h"

#include <Imlib2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct image_list_s {
	const char *name;
	Imlib_Image image;
	struct image_list_s *prev;
	struct image_list_s *next;
};

struct image_list_s *image_list_start, *image_list_end;

/* areas to update */
Imlib_Updates updates, current_update;
/* our virtual framebuffer image we draw into */
Imlib_Image buffer, image;

#define DEFAULT_CACHE_SIZE 4096 * 1024 /* default cache size for loaded images */

void cimlib_set_cache_size(long size)
{
	if (size > 0) {
		imlib_set_cache_size(size);
	}
}

void cimlib_cleanup(void)
{
	struct image_list_s *cur = image_list_start, *last = NULL;
	while (cur) {
		last = cur;
		cur = last->next;
		free(last);
	}
	image_list_start = image_list_end = NULL;
}

void cimlib_init(Display *display, Window window, Visual *visual, Colormap colourmap)
{
	image_list_start = image_list_end = NULL;
	cimlib_set_cache_size(DEFAULT_CACHE_SIZE);
	/* set the maximum number of colors to allocate for 8bpp and less to 256 */
	imlib_set_color_usage(256);
	/* dither for depths < 24bpp */
	imlib_context_set_dither(1);
	/* set the display , visual, colormap and drawable we are using */
	imlib_context_set_display(display);
	imlib_context_set_visual(visual);
	imlib_context_set_colormap(colourmap);
	imlib_context_set_drawable(window);
}

void cimlib_add_image(const char *name)
{
	struct image_list_s *cur = NULL;
	cur = malloc(sizeof(struct image_list_s));
	memset(cur, 0, sizeof(struct image_list_s));
	cur->name = name;
	if (image_list_end) {
		image_list_end->next = cur;
		cur->prev = image_list_end;
		image_list_end = cur;
	} else {
		image_list_start = image_list_end = cur;
	}
}

static void cimlib_draw_image(struct image_list_s *cur)
{
	image = imlib_load_image(cur->name);
	if (image) {
		int w, h;
		DBGP("Drawing image '%s'", cur->name);
		imlib_context_set_image(image);
		w = imlib_image_get_width();
		h = imlib_image_get_height();
		imlib_context_set_image(buffer);
		imlib_blend_image_onto_image(image, 0, 0, 0, w, h,
				0, 0, w, h);
		imlib_context_set_image(image);
		imlib_free_image();
	} else {
		ERR("Unable to load image '%s'", cur->name);
	}
}

void cimlib_event_start(void)
{
	if (!image_list_start) return; /* are we actually drawing anything? */
	/* init our updates to empty */
	updates = imlib_updates_init();
}

static void cimlib_draw_all(void)
{
	struct image_list_s *cur = image_list_start;
	while (cur) {
		cimlib_draw_image(cur);
		cur = cur->next;
	}
}

void cimlib_event_end(int x, int y, int width, int height)
{
	if (!image_list_start) return; /* are we actually drawing anything? */
	/* take all the little rectangles to redraw and merge them into
	 * something sane for rendering */
	updates = imlib_updates_merge_for_rendering(updates, width, height);
	for (current_update = updates; current_update; current_update = imlib_updates_get_next(current_update)) {
		int up_x, up_y, up_w, up_h;

		/* find out where the first update is */
		imlib_updates_get_coordinates(current_update, 
				&up_x, &up_y, &up_w, &up_h);

		/* create our buffer image for rendering this update */
		buffer = imlib_create_image(up_w, up_h);

		/* we can blend stuff now */
		imlib_context_set_blend(1);

		cimlib_draw_all();

		/* set the buffer image as our current image */
		imlib_context_set_image(buffer);
		/* render the image at 0, 0 */
		imlib_render_image_on_drawable(up_x, up_y);
		/* don't need that temporary buffer image anymore */
		imlib_free_image();
	}
	/* if we had updates - free them */
	if (updates) {
		imlib_updates_free(updates);
	} else {
		/* was likely a timeout, redraw everything */

		/* create our buffer image for rendering this update */
		buffer = imlib_create_image(width, height);

		/* we can blend stuff now */
		imlib_context_set_blend(1);

		cimlib_draw_all();

		/* set the buffer image as our current image */
		imlib_context_set_image(buffer);
		/* render the image at 0, 0 */
		imlib_render_image_on_drawable(x, y);
		/* don't need that temporary buffer image anymore */
		imlib_free_image();
	}
}

void cimlib_event_expose(XEvent *event)
{
	if (!image_list_start) return; /* are we actually drawing anything? */
	updates = imlib_update_append_rect(updates,
			event->xexpose.x, event->xexpose.y,
			event->xexpose.width, event->xexpose.height);
}

