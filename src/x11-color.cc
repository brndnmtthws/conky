/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "colours.h"

#include <X11/Xlib.h>

#include <string.h>
#include <strings.h>

unsigned long Colour::to_x11_color(Display *display, int screen,
                                   bool transparency, bool premultiply) {
  static std::unordered_map<Colour, unsigned long, Colour::Hash> x11_pixels;

  if (display == nullptr) {
    /* cannot work if display is not open */
    return 0;
  }

  unsigned long pixel;

  /* Either get a cached X11 pixel or allocate one */
  if (auto pixel_iter = x11_pixels.find(*this);
      pixel_iter != x11_pixels.end()) {
    pixel = pixel_iter->second;
  } else {
    XColor xcolor{};
    xcolor.red = this->red * 257;
    xcolor.green = this->green * 257;
    xcolor.blue = this->blue * 257;
    if (XAllocColor(display, DefaultColormap(display, screen), &xcolor) == 0) {
      // NORM_ERR("can't allocate X color");
      return 0;
    }

    /* Save pixel value in the cache to avoid reallocating it */
    x11_pixels[*this] = xcolor.pixel;
    pixel = static_cast<unsigned long>(xcolor.pixel);
  }

  pixel &= 0xffffff;
#ifdef BUILD_ARGB
  if (transparency) {
    if (premultiply)
      pixel = (red * alpha / 255) << 16 | (green * alpha / 255) << 8 |
              (blue * alpha / 255);
    pixel |= ((unsigned long)alpha << 24);
  }
#endif /* BUILD_ARGB */
  return pixel;
}