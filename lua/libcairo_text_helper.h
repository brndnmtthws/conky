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

typedef enum _cairo_text_alignment {
  CAIRO_TEXT_ALIGN_LEFT = 0,
  CAIRO_TEXT_ALIGN_RIGHT,
  CAIRO_TEXT_ALIGN_CENTER
} cairo_text_alignment_t;

FontData *cairo_text_hp_load_font(const char *font, int font_size)
{
  /* Fontconfig will take a font name and return the file with */
  /* the best match as Freetype only works directly with font files */
    
  FcInit();
  FcConfig* config = FcInitLoadConfigAndFonts();

  // crash here
  if (!font) {
    printf("Error: cairo_text font not set.\n"); 
    return NULL;
  }   
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
    printf("Error: cairo_text couldn't find font.\n"); 
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
//void cairo_text_hp_intl_show(cairo_t *cr, int x, int y, cairo_text_alignment_t alignment, const char *text, FontData *font,
//                             const char *direction, const char *script, const char *language)
void cairo_text_hp_show(cairo_t *cr, int x, int y, const char *text, FontData *font, 
                   cairo_text_alignment_t alignment, const char *language, const char *script, const char *direction)
{
  /* It seems that lua may just pass NULL for an empty string */
  if (text == NULL) {
    printf("Error: CairoTextHelper: Null string\n");
    return;
  }
  if (font == NULL) {
    printf("Error: CairoTextHelper: Null FontData");
    return;
  }

  double x1,x2,y1,y2;

  cairo_clip_extents(cr, &x1, &y1, &x2, &y2);

  int width = x2-x1;
  int height = y2-y1;

  hb_language_t text_language;
  hb_direction_t text_direction;
  if (language != NULL) {
    text_language = hb_language_from_string (language, -1);
  }
  else {
    /* Use en as the default as if you are sharing configs with other */
    /* people, you don't want them to break if they use a different lang */
    hb_language_from_string("en", -1);
  }

  hb_script_t text_script = hb_script_from_string(script, -1);
  if (text_script == HB_SCRIPT_UNKNOWN) {
    text_script = HB_SCRIPT_COMMON;
  }

  if (direction != NULL) {
    text_direction = hb_direction_from_string(direction, -1);
  }
  else {
    text_direction = hb_script_get_horizontal_direction(text_script);
  }
  /* Note Direction can be invalid if user passes something invalid */
  /* or if the script can be Vertical or Horizontal */
  if (text_direction == HB_DIRECTION_INVALID) {
    text_direction = HB_DIRECTION_LTR;
  }
  
  /* Draw text */
  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();

  //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_language(buf, text_language);
  if (script != NULL) {
    hb_buffer_set_script(buf, text_script); /* see hb-unicode.h */
  }
  else {
    hb_buffer_guess_segment_properties(buf);
  }
  hb_buffer_set_direction(buf, text_direction); /* or LTR */

  /* Layout the text */
  hb_buffer_add_utf8(buf, text, strlen(text), 0, strlen(text));
  hb_shape(font->hb_ft_font, buf, NULL, 0);

  /* Need to calculate the Baseline for drawing on the y axis */
  hb_font_extents_t font_extents;
  hb_font_get_extents_for_direction(font->hb_ft_font, text_direction, &font_extents);

  /* Note Line Gap was always 0 in my testing */
  int baseline_offset = font_extents.ascender/64 + 0.5 * font_extents.line_gap/64 + 1;

  /* Hand the layout to cairo to render */
  unsigned int         glyph_count;
  hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);
  cairo_glyph_t       *cairo_glyphs = malloc(sizeof(cairo_glyph_t) * glyph_count);

  /* RTL positioning seems to be slightly off and characters don't link as they should */
  /* This hack gets it significantly closer to correct but is not 100% for all fonts and sizes */
  int rtl_fix = font->font_size/10;

  unsigned int string_width_in_pixels = 0;
  for (int i=0; i < glyph_count; ++i) {
      string_width_in_pixels += glyph_pos[i].x_advance/64;
      if (text_direction == HB_DIRECTION_RTL) {
        string_width_in_pixels -= rtl_fix;
      }
  }
  /* More RTL Hacks */
  if (text_direction == HB_DIRECTION_RTL) {
    string_width_in_pixels += 2;
  }
  int draw_x = x;
  int draw_y = y;

  if (HB_DIRECTION_IS_VERTICAL(text_direction)) { 
    draw_x = width/2 - string_width_in_pixels/2 + x;
  }
  else {
    draw_y = baseline_offset + y;
    if (alignment == CAIRO_TEXT_ALIGN_LEFT) {
      if (text_direction == HB_DIRECTION_RTL) {
        draw_x = width - x - string_width_in_pixels;
      }
      // LTR handled as default.
    }
    else if (alignment == CAIRO_TEXT_ALIGN_RIGHT) {
      if (text_direction == HB_DIRECTION_RTL) {
        draw_x = x;
      }
      else {
        draw_x = width - x - string_width_in_pixels;
      }
    }
    else if (alignment == CAIRO_TEXT_ALIGN_CENTER) {
      draw_x = width/2 - string_width_in_pixels/2 + x;
    }
  }

  // Reset x/y now that draw_x is set
  x = 0;
  y = 0;

  for (int i=0; i < glyph_count; ++i) {
      cairo_glyphs[i].index = glyph_info[i].codepoint;
      cairo_glyphs[i].x = x + draw_x + (glyph_pos[i].x_offset/64.0);
      cairo_glyphs[i].y = y + draw_y - (glyph_pos[i].y_offset/64.0);
      x += glyph_pos[i].x_advance/64.0;
      if (text_direction == HB_DIRECTION_RTL) {
        x -= rtl_fix;
      }
      y -= glyph_pos[i].y_advance/64.0;
  }

  cairo_set_font_face(cr, font->cairo_ft_face);
  cairo_set_font_size(cr, font->font_size);
  cairo_show_glyphs(cr, cairo_glyphs, glyph_count);

  free(cairo_glyphs);
  hb_buffer_destroy(buf);
}

int cairo_text_hp_text_size( const char *text, FontData *font, 
                             const char *language, const char *script, const char *direction, int *width, int *height)
{
  hb_language_t text_language;
  hb_direction_t text_direction;
  if (language != NULL) {
    text_language = hb_language_from_string (language, -1);
  }
  else {
    /* Use en as the default as if you are sharing configs with other */
    /* people, you don't want them to break if they use a different lang */
    hb_language_from_string("en", -1);
  }

  hb_script_t text_script = hb_script_from_string(script, -1);
  if (text_script == HB_SCRIPT_UNKNOWN) {
    text_script = HB_SCRIPT_COMMON;
  }

  if (direction != NULL) {
    text_direction = hb_direction_from_string(direction, -1);
  }
  else {
    text_direction = hb_script_get_horizontal_direction(text_script);
  }
  /* Note Direction can be invalid if user passes something invalid */
  /* or if the script can be Vertical or Horizontal */
  if (text_direction == HB_DIRECTION_INVALID) {
    text_direction = HB_DIRECTION_LTR;
  }
  
  /* Draw text */
  /* Create a buffer for harfbuzz to use */
  hb_buffer_t *buf = hb_buffer_create();

  //alternatively you can use hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_unicode_funcs(buf, hb_glib_get_unicode_funcs());
  hb_buffer_set_direction(buf, text_direction); /* or LTR */
  if (script != NULL) {
    hb_buffer_set_script(buf, text_script); /* see hb-unicode.h */
  }
  else {
    hb_buffer_guess_segment_properties(buf);
  }  hb_buffer_set_language(buf, text_language);

  /* Layout the text */
  hb_buffer_add_utf8(buf, text, strlen(text), 0, strlen(text));
  hb_shape(font->hb_ft_font, buf, NULL, 0);

  hb_font_extents_t font_extents;
  hb_font_get_extents_for_direction(font->hb_ft_font, text_direction, &font_extents);

  /* Hand the layout to cairo to render */
  unsigned int         glyph_count;
  hb_glyph_info_t     *glyph_info   = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_pos    = hb_buffer_get_glyph_positions(buf, &glyph_count);
  cairo_glyph_t       *cairo_glyphs = malloc(sizeof(cairo_glyph_t) * glyph_count);

  unsigned int string_width_in_pixels = 0;
  unsigned int string_height_in_pixels = 0;

  /* Temp variable used for each glyph */
  hb_glyph_extents_t glyph_extents;
  if (text_direction == HB_DIRECTION_LTR ||
      text_direction == HB_DIRECTION_RTL)
  {
    /* Width */
    for (int i=0; i < glyph_count; ++i) {
        string_width_in_pixels += glyph_pos[i].x_advance/64+glyph_pos[i].x_offset/64;
        hb_font_get_glyph_extents (font->hb_ft_font, glyph_info[i].codepoint,
                                   &glyph_extents);
        int h = ((glyph_extents.height/64)*-1)+(glyph_extents.y_bearing/64);
        if (h > string_height_in_pixels) {
          string_height_in_pixels = h;
        }
    }
    /* Height */
    //string_height_in_pixels = font_extents.ascender/64 - font_extents.descender/64;
  } else {
    /* Width */
    string_width_in_pixels = font_extents.ascender/64 - font_extents.descender/64;
    /* Height */
    for (int i=0; i < glyph_count; ++i) {
        string_height_in_pixels += glyph_pos[i].y_advance/64+glyph_pos[i].y_offset/64;
    }
  }

  *width = string_width_in_pixels;
  *height = string_height_in_pixels;
}

void cairo_text_hp_simple_text_size(const char *text, FontData *font, int *width, int *height) {
  cairo_text_hp_text_size(text, font, "LTR", "Zyyy", "en",  width, height);
}
#endif
