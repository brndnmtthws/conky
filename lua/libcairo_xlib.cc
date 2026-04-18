/*
 * Lua binding: cairo_xlib (deprecated)
 *
 * Statically maintained replacement for the toluapp-generated binding.
 * Every function emits a one-time deprecation warning directing script
 * authors to the unified conky_surface() API.
 */

#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xlib.h>

#include <cstdio>

#include "tolua++.h"

/* ---- deprecation helper ------------------------------------------------- */

#define DEPRECATION_WARNING(func)                                          \
  do {                                                                     \
    static bool once = false;                                              \
    if (!once) {                                                           \
      once = true;                                                         \
      fprintf(stderr,                                                      \
        "conky: WARNING: " #func " is deprecated and will be "             \
        "removed in a future release. Use conky_surface() instead."        \
        "See: https://github.com/brndnmtthws/conky/wiki/Lua:-Evaluation"   \
        "\n");                                                             \
    }                                                                      \
  } while (0)

/* ---- argument checking helpers ------------------------------------------ */

static bool check_arg(lua_State *L, int idx, const char *type,
                      const char *func) {
  tolua_Error err;
  if (!tolua_isusertype(L, idx, type, 0, &err)) {
    char msg[128];
    snprintf(msg, sizeof(msg), "#ferror in function '%s'.", func);
    tolua_error(L, msg, &err);
    return false;
  }
  return true;
}

static bool check_num(lua_State *L, int idx, const char *func) {
  tolua_Error err;
  if (!tolua_isnumber(L, idx, 0, &err)) {
    char msg[128];
    snprintf(msg, sizeof(msg), "#ferror in function '%s'.", func);
    tolua_error(L, msg, &err);
    return false;
  }
  return true;
}

static bool check_end(lua_State *L, int idx, const char *func) {
  tolua_Error err;
  if (!tolua_isnoobj(L, idx, &err)) {
    char msg[128];
    snprintf(msg, sizeof(msg), "#ferror in function '%s'.", func);
    tolua_error(L, msg, &err);
    return false;
  }
  return true;
}

/* Validate single-arg surface getters. */
#define CHECK_SURFACE_ONLY(func)                                     \
  if (!check_arg(L, 1, "cairo_surface_t", #func) ||                 \
      !check_end(L, 2, #func))                                      \
    return 0

/* ---- helpers for repetitive getter patterns ----------------------------- */

/* surface -> pointer getter (Display*, Screen*, Visual*) */
#define DEFINE_SURFACE_PTR_GETTER(cfunc, tolua_type)                \
  static int l_##cfunc(lua_State *L) {                              \
    DEPRECATION_WARNING(cfunc);                                     \
    CHECK_SURFACE_ONLY(cfunc);                                      \
    auto *s = (cairo_surface_t *)tolua_tousertype(L, 1, 0);        \
    tolua_pushusertype(L, (void *)cfunc(s), tolua_type);            \
    return 1;                                                       \
  }

/* surface -> int getter (depth, width, height) */
#define DEFINE_SURFACE_INT_GETTER(cfunc)                            \
  static int l_##cfunc(lua_State *L) {                              \
    DEPRECATION_WARNING(cfunc);                                     \
    CHECK_SURFACE_ONLY(cfunc);                                      \
    auto *s = (cairo_surface_t *)tolua_tousertype(L, 1, 0);        \
    tolua_pushnumber(L, (lua_Number)cfunc(s));                      \
    return 1;                                                       \
  }

/* ---- binding functions -------------------------------------------------- */

static int l_cairo_xlib_surface_create(lua_State *L) {
  const char *fn = "cairo_xlib_surface_create";
  DEPRECATION_WARNING(cairo_xlib_surface_create);
  if (!check_arg(L, 1, "Display", fn) ||
      !check_arg(L, 2, "Drawable", fn) ||
      !check_arg(L, 3, "Visual", fn) ||
      !check_num(L, 4, fn) ||
      !check_num(L, 5, fn) ||
      !check_end(L, 6, fn))
    return 0;

  auto *dpy = (Display *)tolua_tousertype(L, 1, 0);
  Drawable drawable = *(Drawable *)tolua_tousertype(L, 2, 0);
  auto *visual = (Visual *)tolua_tousertype(L, 3, 0);
  int w = (int)tolua_tonumber(L, 4, 0);
  int h = (int)tolua_tonumber(L, 5, 0);
  tolua_pushusertype(
      L, cairo_xlib_surface_create(dpy, drawable, visual, w, h),
      "cairo_surface_t");
  return 1;
}

static int l_cairo_xlib_surface_create_for_bitmap(lua_State *L) {
  const char *fn = "cairo_xlib_surface_create_for_bitmap";
  DEPRECATION_WARNING(cairo_xlib_surface_create_for_bitmap);
  if (!check_arg(L, 1, "Display", fn) ||
      !check_arg(L, 2, "Pixmap", fn) ||
      !check_arg(L, 3, "Screen", fn) ||
      !check_num(L, 4, fn) ||
      !check_num(L, 5, fn) ||
      !check_end(L, 6, fn))
    return 0;

  auto *dpy = (Display *)tolua_tousertype(L, 1, 0);
  Pixmap bitmap = *(Pixmap *)tolua_tousertype(L, 2, 0);
  auto *screen = (Screen *)tolua_tousertype(L, 3, 0);
  int w = (int)tolua_tonumber(L, 4, 0);
  int h = (int)tolua_tonumber(L, 5, 0);
  tolua_pushusertype(
      L, cairo_xlib_surface_create_for_bitmap(dpy, bitmap, screen, w, h),
      "cairo_surface_t");
  return 1;
}

static int l_cairo_xlib_surface_set_size(lua_State *L) {
  const char *fn = "cairo_xlib_surface_set_size";
  DEPRECATION_WARNING(cairo_xlib_surface_set_size);
  if (!check_arg(L, 1, "cairo_surface_t", fn) ||
      !check_num(L, 2, fn) ||
      !check_num(L, 3, fn) ||
      !check_end(L, 4, fn))
    return 0;

  auto *s = (cairo_surface_t *)tolua_tousertype(L, 1, 0);
  cairo_xlib_surface_set_size(s, (int)tolua_tonumber(L, 2, 0),
                              (int)tolua_tonumber(L, 3, 0));
  return 0;
}

static int l_cairo_xlib_surface_set_drawable(lua_State *L) {
  const char *fn = "cairo_xlib_surface_set_drawable";
  DEPRECATION_WARNING(cairo_xlib_surface_set_drawable);
  if (!check_arg(L, 1, "cairo_surface_t", fn) ||
      !check_arg(L, 2, "Drawable", fn) ||
      !check_num(L, 3, fn) ||
      !check_num(L, 4, fn) ||
      !check_end(L, 5, fn))
    return 0;

  auto *s = (cairo_surface_t *)tolua_tousertype(L, 1, 0);
  Drawable drawable = *(Drawable *)tolua_tousertype(L, 2, 0);
  cairo_xlib_surface_set_drawable(s, drawable, (int)tolua_tonumber(L, 3, 0),
                                  (int)tolua_tonumber(L, 4, 0));
  return 0;
}

DEFINE_SURFACE_PTR_GETTER(cairo_xlib_surface_get_display, "Display")
DEFINE_SURFACE_PTR_GETTER(cairo_xlib_surface_get_screen, "Screen")
DEFINE_SURFACE_PTR_GETTER(cairo_xlib_surface_get_visual, "Visual")

static int l_cairo_xlib_surface_get_drawable(lua_State *L) {
  DEPRECATION_WARNING(cairo_xlib_surface_get_drawable);
  CHECK_SURFACE_ONLY(cairo_xlib_surface_get_drawable);

  auto *s = (cairo_surface_t *)tolua_tousertype(L, 1, 0);
  auto *obj = Mtolua_new(Drawable(cairo_xlib_surface_get_drawable(s)));
  tolua_pushusertype(L, obj, "Drawable");
  tolua_register_gc(L, lua_gettop(L));
  return 1;
}

DEFINE_SURFACE_INT_GETTER(cairo_xlib_surface_get_depth)
DEFINE_SURFACE_INT_GETTER(cairo_xlib_surface_get_width)
DEFINE_SURFACE_INT_GETTER(cairo_xlib_surface_get_height)

/* ---- module registration ------------------------------------------------ */

static void reg_types(lua_State *L) {
  tolua_usertype(L, "cairo_surface_t");
  tolua_usertype(L, "Visual");
  tolua_usertype(L, "Screen");
  tolua_usertype(L, "Pixmap");
  tolua_usertype(L, "Drawable");
  tolua_usertype(L, "Display");
}

TOLUA_API int tolua_cairo_xlib_open(lua_State *L) {
  tolua_open(L);
  reg_types(L);
  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);

#define REG(name) tolua_function(L, #name, l_##name)
  REG(cairo_xlib_surface_create);
  REG(cairo_xlib_surface_create_for_bitmap);
  REG(cairo_xlib_surface_set_size);
  REG(cairo_xlib_surface_set_drawable);
  REG(cairo_xlib_surface_get_display);
  REG(cairo_xlib_surface_get_drawable);
  REG(cairo_xlib_surface_get_screen);
  REG(cairo_xlib_surface_get_visual);
  REG(cairo_xlib_surface_get_depth);
  REG(cairo_xlib_surface_get_width);
  REG(cairo_xlib_surface_get_height);
#undef REG

  tolua_endmodule(L);
  return 1;
}

TOLUA_API int luaopen_cairo_xlib(lua_State *L) {
  return tolua_cairo_xlib_open(L);
}
