/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Please see COPYING for details
 *
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

#ifndef _LIBCAIRO_TEXT_HELPER_H_
#define _LIBCAIRO_TEXT_HELPER_H_

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include <fontconfig/fontconfig.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftadvanc.h>
#include <freetype/ftlcdfil.h>
#include <freetype/ftsnames.h>
#include <freetype/tttables.h>

#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-glib.h>

typedef struct _FontData {
  cairo_font_face_t *cairo_ft_face;
  hb_font_t *hb_ft_font;
  hb_face_t *hb_ft_face;
  int font_size;
  /* Internally the following two are pointers */
  /* Stored here so they can be freed later */
  FT_Library ft_library;
  FT_Face ft_face;
}FontData;

FontData *cairo_text_hp_load_font(const char *font, int font_size)
{
  /* Fontconfig will take a font name and return the file with */
  /* the best match as Freetype only works directly with font files */
    
  FcInit();
  FcConfig* config = FcInitLoadConfigAndFonts();
    
  FcPattern* pat = FcNameParse((const FcChar8*)font);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);
    
  FcResult result;
  FcPattern* font_pat = FcFontMatch(config, pat, &result);

  /* Will be freed with font */
  FcChar8* file = NULL; 
  FcPatternGetString(font_pat, FC_FILE, 0, &file);
  
  if (!file) {
    /* FIXME: add error handling */
    return NULL;
  }
  
  FontData *font_data = malloc(sizeof(struct _FontData));
  
  /* Load the font */
  FT_Init_FreeType(&font_data->ft_library);
  FT_Library_SetLcdFilter(font_data->ft_library, FT_LCD_FILTER_DEFAULT);
  FT_New_Face(font_data->ft_library, (char *)file, 0, &font_data->ft_face);
  FT_Set_Char_Size(font_data->ft_face, font_size*64, font_size*64,0,0);
  
  /* Store font data for use later */
  font_data->cairo_ft_face = cairo_font_face_reference(cairo_ft_font_face_create_for_ft_face(font_data->ft_face, 0));
  font_data->hb_ft_font = hb_ft_font_create(font_data->ft_face, NULL);
  font_data->hb_ft_face = hb_ft_face_create(font_data->ft_face, NULL);
  font_data->font_size = font_size;
  
  /* Cleanup font config */
  FcPatternDestroy(font_pat);
  FcPatternDestroy(pat);
  FcConfigDestroy(config);
  /* FIXME: Crashes */
  /*FcFini(); */
  
  return font_data;
}

void cairo_text_hp_destroy_font(FontData *font)
{
  cairo_font_face_destroy(font->cairo_ft_face);
  cairo_font_face_destroy (font->cairo_ft_face);
  hb_font_destroy(font->hb_ft_font);
  hb_face_destroy(font->hb_ft_face);
  
  FT_Done_Face (font->ft_face);
  FT_Done_FreeType(font->ft_library);
  
  free(font);
}

/* 
 * Direction calls hb_direction_from_string example values are LTR and RTL
 *   https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-direction-from-string
 * Script is an ISO 15924 4 character string, "Zyyy" can be used for "Common" and "Zinh"
 *  for "Inherited". 
 *  https://harfbuzz.github.io/harfbuzz-hb-common.html#hb-script-from-string
 * Language is a BCP 47 language tag. eg "en" or "en-US"
 */
void cairo_text_hp_intl_show(cairo_t *cr, int x, int y, const char *text, FontData *font,
                             const char *direction, const char *script, const char *language)
{  
  /* FIXME: Support others */
  hb_direction_t text_direction = hb_direction_from_string(direction, -1);
  if (text_direction == HB_DIRECTION_INVALID) {
    text_direction = HB_DIRECTION_LTR;
  }
  hb_script_t text_script = hb_script_from_string(script, -1);
  if (text_script == HB_SCRIPT_UNKNOWN) {
    text_script = HB_SCRIPT_COMMON;
  }
    
  hb_language_t text_language = hb_language_from_string (language, -1);
  
  /* Draw text */
  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();

  //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_direction(buf, text_direction); /* or LTR */
  hb_buffer_set_script(buf, text_script); /* see hb-unicode.h */
  hb_buffer_set_language(buf, text_language);

  /* Layout the text */
  hb_buffer_add_utf8(buf, text, strlen(text), 0, strlen(text));
  hb_shape(font->hb_ft_font, buf, NULL, 0);

  /* Hand the layout to cairo to render */
  unsigned int         glyph_count;
  hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);
  cairo_glyph_t       *cairo_glyphs = malloc(sizeof(cairo_glyph_t) * glyph_count);

  unsigned int string_width_in_pixels = 0;
  for (int i=0; i < glyph_count; ++i) {
      string_width_in_pixels += glyph_pos[i].x_advance/64;
  }
  int draw_x = x;
  if (HB_DIRECTION_IS_VERTICAL(text_direction)) { 
    /* FIXME */
    //draw_x = width/2 - string_width_in_pixels/2;
  } else if (text_direction == HB_DIRECTION_RTL){
    draw_x = cairo_image_surface_get_width(cairo_get_target(cr)) + x;
  }

  for (int i=0; i < glyph_count; ++i) {
      cairo_glyphs[i].index = glyph_info[i].codepoint;
      cairo_glyphs[i].x = x + (glyph_pos[i].x_offset/64.0);
      cairo_glyphs[i].y = y - (glyph_pos[i].y_offset/64.0);
      x += glyph_pos[i].x_advance/64.0;
      y -= glyph_pos[i].y_advance/64.0;
  }
  
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
  cairo_set_font_face(cr, font->cairo_ft_face);
  cairo_set_font_size(cr, font->font_size);
  cairo_show_glyphs(cr, cairo_glyphs, glyph_count);

  free(cairo_glyphs);
  hb_buffer_destroy(buf);
}

void cairo_text_hp_simple_show(cairo_t *cr, int x, int y, const char *text, FontData *font)
{
  cairo_text_hp_intl_show(cr, x, y, text, font, "LTR", "Zyyy", "en");
}

#endif
