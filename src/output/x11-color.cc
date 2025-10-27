#include "../content/colours.hh"

#include <X11/Xlib.h>

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