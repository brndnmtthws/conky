/* Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2008 Brenden Matthews, Philip Kovacs, et. al.
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

#include "config.h"
#include "text_object.h"
#include "conky.h"
#include "common.h"
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/time.h>
#ifdef X11
#include "x11.h"
#include <X11/Xutil.h>
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef IMLIB2
#include <Imlib2.h>
#endif /* IMLIB2 */
#endif /* X11 */
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <getopt.h>

/* local headers */
#include "build.h"
#include "diskio.h"
#include "fs.h"
#include "logging.h"
#include "mixer.h"
#include "mail.h"
#include "mboxscan.h"
#include "temphelper.h"
#include "top.h"

/* check for OS and include appropriate headers */
#if defined(__linux__)
#include "linux.h"
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include "freebsd.h"
#elif defined(__OpenBSD__)
#include "openbsd.h"
#endif

/* FIXME: apm_getinfo is unused here. maybe it's meant for common.c */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
int apm_getinfo(int fd, apm_info_t aip);
char *get_apm_adapter(void);
char *get_apm_battery_life(void);
char *get_apm_battery_time(void);
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#ifdef CONFIG_OUTPUT
#include "defconfig.h"
#ifdef HAVE_FOPENCOOKIE
#include "conf_cookie.h"
#endif
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(x)   ((x & S_IFMT) == S_IFSOCK)
#endif

#define MAIL_FILE "$MAIL"
#define MAX_IF_BLOCK_DEPTH 5
#define MAX_TAIL_LINES 100

//#define SIGNAL_BLOCKING
#undef SIGNAL_BLOCKING

/* debugging level, used by logging.h */
int global_debug_level = 0;

/* two strings for internal use */
static char *tmpstring1, *tmpstring2;

/* variables holding various config settings */
int short_units;
int cpu_separate;
enum {
	NO_SPACER = 0,
	LEFT_SPACER,
	RIGHT_SPACER
} use_spacer;
int top_cpu, top_mem;
#define TO_X 1
#define TO_STDOUT 2
static int output_methods;
static volatile int g_signal_pending;
/* Update interval */
static double update_interval;


/* prototypes for internally used functions */
static void signal_handler(int);
static void print_version(void) __attribute__((noreturn));
static void reload_config(void);
static void clean_up(void);

static void print_version(void)
{
	printf(PACKAGE_NAME" "VERSION" compiled "BUILD_DATE" for "BUILD_ARCH"\n");

	printf("\nCompiled in features:\n\n"
		   "System config file: "SYSTEM_CONFIG_FILE"\n\n"
#ifdef X11
		   " X11:\n"
# ifdef HAVE_XDAMAGE
		   "  * Xdamage extension\n"
# endif /* HAVE_XDAMAGE */
# ifdef HAVE_XDBE
		   "  * Xdbe extension (double buffer)\n"
# endif /* HAVE_XDBE */
# ifdef XFT
		   "  * xft\n"
# endif /* XFT */
#endif /* X11 */
		   "\n Music detection:\n"
#ifdef AUDACIOUS
		   "  * audacious\n"
#endif /* AUDACIOUS */
#ifdef BMPX
		   "  * bmpx\n"
#endif /* BMPX */
#ifdef MPD
		   "  * mpd\n"
#endif /* MPD */
#ifdef MOC
       "  * moc\n"
#endif /* MOC */
#ifdef XMMS2
		   "  * xmms2\n"
#endif /* XMMS2 */
		   "\n General features:\n"
#ifdef MATH
		   "  * math\n"
#endif /* Math */
#ifdef HDDTEMP
		   "  * hddtemp\n"
#endif /* HDDTEMP */
#ifdef TCP_PORT_MONITOR
		   "  * portmon\n"
#endif /* TCP_PORT_MONITOR */
#ifdef RSS
		   "  * rss\n"
#endif /* RSS */
#ifdef EVE
		   "  * eve\n"
#endif /* EVE */
#ifdef HAVE_IWLIB
		   "  * wireless\n"
#endif /* HAVE_IWLIB */
#ifdef SMAPI
	"  * smapi\n"
#endif /* SMAPI */
#ifdef NVIDIA
	"  * nvidia\n"
#endif
	);

	exit(0);
}

static const char *suffixes[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "" };


#ifdef X11

/* text size */

static int text_start_x, text_start_y;	/* text start position in window */
static int text_width, text_height;

/* alignments */
enum alignment {
	TOP_LEFT = 1,
	TOP_RIGHT,
	TOP_MIDDLE,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	BOTTOM_MIDDLE,
	MIDDLE_LEFT,
	MIDDLE_RIGHT,
	NONE
};

/* for fonts */
struct font_list {

	char name[DEFAULT_TEXT_BUFFER_SIZE];
	int num;
	XFontStruct *font;

#ifdef XFT
	XftFont *xftfont;
	int font_alpha;
#endif
};

static int selected_font = 0;
static int font_count = -1;
struct font_list *fonts = NULL;

#ifdef XFT

#define font_height() (use_xft ? (fonts[selected_font].xftfont->ascent + \
	fonts[selected_font].xftfont->descent) \
	: (fonts[selected_font].font->max_bounds.ascent + \
	fonts[selected_font].font->max_bounds.descent))
#define font_ascent() (use_xft ? fonts[selected_font].xftfont->ascent \
	: fonts[selected_font].font->max_bounds.ascent)
#define font_descent() (use_xft ? fonts[selected_font].xftfont->descent \
	: fonts[selected_font].font->max_bounds.descent)

#else

#define font_height() (fonts[selected_font].font->max_bounds.ascent + \
	fonts[selected_font].font->max_bounds.descent)
#define font_ascent() fonts[selected_font].font->max_bounds.ascent
#define font_descent() fonts[selected_font].font->max_bounds.descent

#endif

#define MAX_FONTS 64 // hmm, no particular reason, just makes sense.

static void set_font(void);

int addfont(const char *data_in)
{
	if (font_count > MAX_FONTS) {
		CRIT_ERR("you don't need that many fonts, sorry.");
	}
	font_count++;
	if (font_count == 0) {
		if (fonts != NULL) {
			free(fonts);
		}
		if ((fonts = (struct font_list *) malloc(sizeof(struct font_list)))
				== NULL) {
			CRIT_ERR("malloc");
		}
		memset(fonts, 0, sizeof(struct font_list));
	}
	fonts = realloc(fonts, (sizeof(struct font_list) * (font_count + 1)));
	memset(&fonts[font_count], 0, sizeof(struct font_list));
	if (fonts == NULL) {
		CRIT_ERR("realloc in addfont");
	}
	// must account for null terminator
	if (strlen(data_in) < DEFAULT_TEXT_BUFFER_SIZE) {
		strncpy(fonts[font_count].name, data_in, DEFAULT_TEXT_BUFFER_SIZE);
#ifdef XFT
		fonts[font_count].font_alpha = 0xffff;
#endif
	} else {
		CRIT_ERR("Oops...looks like something overflowed in addfont().");
	}
	return font_count;
}

void set_first_font(const char *data_in)
{
	if (font_count < 0) {
		if ((fonts = (struct font_list *) malloc(sizeof(struct font_list)))
				== NULL) {
			CRIT_ERR("malloc");
		}
		memset(fonts, 0, sizeof(struct font_list));
		font_count++;
	}
	if (strlen(data_in) > 1) {
		strncpy(fonts[0].name, data_in, DEFAULT_TEXT_BUFFER_SIZE);
#ifdef XFT
		fonts[0].font_alpha = 0xffff;
#endif
	}
}

void free_fonts(void)
{
	int i;

	for (i = 0; i <= font_count; i++) {
#ifdef XFT
		if (use_xft) {
			XftFontClose(display, fonts[i].xftfont);
			fonts[i].xftfont = 0;
		} else
#endif
		{
			XFreeFont(display, fonts[i].font);
			fonts[i].font = 0;
		}
	}
	free(fonts);
	fonts = 0;
	font_count = -1;
	selected_font = 0;
}

static void load_fonts(void)
{
	int i;

	for (i = 0; i <= font_count; i++) {
#ifdef XFT
		/* load Xft font */
		if (use_xft && fonts[i].xftfont) {
			continue;
		} else if (use_xft) {
			/* if (fonts[i].xftfont != NULL && selected_font == 0) {
				XftFontClose(display, fonts[i].xftfont);
			} */
			fonts[i].xftfont = XftFontOpenName(display, screen,
					fonts[i].name);
			if (fonts[i].xftfont != NULL) {
				continue;
			}

			ERR("can't load Xft font '%s'", fonts[i].name);
			if ((fonts[i].xftfont = XftFontOpenName(display, screen,
					"courier-12")) != NULL) {
				continue;
			}

			ERR("can't load Xft font '%s'", "courier-12");

			if ((fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
				CRIT_ERR("can't load font '%s'", "fixed");
			}
			use_xft = 0;

			continue;
		}
#endif
		/* load normal font */
		/* if (fonts[i].font != NULL) {
			XFreeFont(display, fonts[i].font);
		} */

		if (fonts[i].font || (fonts[i].font = XLoadQueryFont(display, fonts[i].name)) == NULL) {
			ERR("can't load font '%s'", fonts[i].name);
			if ((fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
				CRIT_ERR("can't load font '%s'", "fixed");
				printf("loaded fixed?\n");
			}
		}
	}
}

#endif /* X11 */

/* struct that has all info to be shared between
 * instances of the same text object */
struct information info;

/* default config file */
static char *current_config;

/* set to 1 if you want all text to be in uppercase */
static unsigned int stuff_in_upper_case;

/* Run how many times? */
static unsigned long total_run_times;

/* fork? */
static int fork_to_background;

static int cpu_avg_samples, net_avg_samples;

#ifdef X11

static int show_graph_scale;
static int show_graph_range;

/* Position on the screen */
static int text_alignment;
static int gap_x, gap_y;

/* border */
static int draw_borders;
static int draw_graph_borders;
static int stippled_borders;

static int draw_shades, draw_outline;

static int border_margin, border_width;

static long default_fg_color, default_bg_color, default_out_color;

/* create own window or draw stuff to root? */
static int set_transparent = 0;

#ifdef OWN_WINDOW
static int own_window = 0;
static int background_colour = 0;

/* fixed size/pos is set if wm/user changes them */
static int fixed_size = 0, fixed_pos = 0;
#endif

static int minimum_width, minimum_height;
static int maximum_width;

#endif /* X11 */

#ifdef __OpenBSD__
static int sensor_device;
#endif

static long color0, color1, color2, color3, color4, color5, color6, color7,
	color8, color9;

static char *template[10];

/* maximum number of special things, e.g. fonts, offsets, aligns, etc. */
static unsigned int max_specials = MAX_SPECIALS_DEFAULT;

/* maximum size of config TEXT buffer, i.e. below TEXT line. */
static unsigned int max_user_text = MAX_USER_TEXT_DEFAULT;

/* maximum size of individual text buffers, ie $exec buffer size */
unsigned int text_buffer_size = DEFAULT_TEXT_BUFFER_SIZE;

#ifdef HAVE_ICONV
#define CODEPAGE_LENGTH 20
long iconv_selected;
long iconv_count = 0;
char iconv_converting;
static iconv_t **iconv_cd = 0;

int register_iconv(iconv_t *new_iconv)
{
	iconv_cd = realloc(iconv_cd, sizeof(iconv_t *) * (iconv_count + 1));
	if (!iconv_cd) {
		CRIT_ERR("Out of memory");
	}
	iconv_cd[iconv_count] = malloc(sizeof(iconv_t));
	if (!iconv_cd[iconv_count]) {
		CRIT_ERR("Out of memory");
	}
	memcpy(iconv_cd[iconv_count], new_iconv, sizeof(iconv_t));
	iconv_count++;
	return iconv_count;
}

void free_iconv(void)
{
	if (iconv_cd) {
		long i;

		for (i = 0; i < iconv_count; i++) {
			if (iconv_cd[i]) {
				iconv_close(*iconv_cd[i]);
				free(iconv_cd[i]);
			}
		}
		free(iconv_cd);
	}
	iconv_cd = 0;
}

#endif

/* UTF-8 */
int utf8_mode = 0;

/* no buffers in used memory? */
int no_buffers;

/* pad percentages to decimals? */
static int pad_percents = 0;

static char *global_text = 0;
long global_text_lines;

static int total_updates;

int check_contains(char *f, char *s)
{
	int ret = 0;
	FILE *where = fopen(f, "r");

	if (where) {
		char buf1[256], buf2[256];

		while (fgets(buf1, 256, where)) {
			sscanf(buf1, "%255s", buf2);
			if (strstr(buf2, s)) {
				ret = 1;
				break;
			}
		}
		fclose(where);
	} else {
		ERR("Could not open the file");
	}
	return ret;
}

#ifdef X11
static inline int calc_text_width(const char *s, int l)
{
#ifdef XFT
	if (use_xft) {
		XGlyphInfo gi;

		if (utf8_mode) {
			XftTextExtentsUtf8(display, fonts[selected_font].xftfont,
				(const FcChar8 *) s, l, &gi);
		} else {
			XftTextExtents8(display, fonts[selected_font].xftfont,
				(const FcChar8 *) s, l, &gi);
		}
		return gi.xOff;
	} else
#endif
	{
		return XTextWidth(fonts[selected_font].font, s, l);
	}
}
#endif /* X11 */

/* formatted text to render on screen, generated in generate_text(),
 * drawn in draw_stuff() */

static char *text_buffer;

/* special stuff in text_buffer */

#define SPECIAL_CHAR '\x01'

enum special_types {
	HORIZONTAL_LINE,
	STIPPLED_HR,
	BAR,
	FG,
	BG,
	OUTLINE,
	ALIGNR,
	ALIGNC,
	GRAPH,
	OFFSET,
	VOFFSET,
	FONT,
	GOTO,
	TAB,
};

struct special_t {
	int type;
	short height;
	short width;
	long arg;
	double *graph;
	double graph_scale;
	short show_scale;
	int graph_width;
	int scaled;
	unsigned long first_colour;	// for graph gradient
	unsigned long last_colour;
	short font_added;
};

/* create specials array on heap instead of stack with introduction of
 * max_specials */
static struct special_t *specials = NULL;

static unsigned int special_count;

#ifdef X11
static unsigned int special_index;	/* used when drawing */
#endif /* X11 */

/* why 256? cause an array of more then 256 doubles seems excessive,
 * and who needs that kind of precision anyway? */
#define MAX_GRAPH_DEPTH 256

static struct special_t *new_special(char *buf, enum special_types t)
{
	if (special_count >= max_specials) {
		CRIT_ERR("too many special things in text");
	}

	buf[0] = SPECIAL_CHAR;
	buf[1] = '\0';
	specials[special_count].type = t;
	return &specials[special_count++];
}

long fwd_fcharfind(FILE *fp, char val, unsigned int step)
{
#define BUFSZ 0x1000
	long ret = -1;
	unsigned int count = 0;
	static char buf[BUFSZ];
	long orig_pos = ftell(fp);
	long buf_pos = -1;
	long buf_size = BUFSZ;
	char *cur_found = NULL;

	while (count < step) {
		if (cur_found == NULL) {
			buf_size = fread(buf, 1, buf_size, fp);
			buf_pos = 0;
		}
		cur_found = memchr(buf + buf_pos, val, buf_size - buf_pos);
		if (cur_found != NULL) {
			buf_pos = cur_found - buf + 1;
			count++;
		} else {
			if (feof(fp)) {
				break;
			}
		}
	}
	if (count == step) {
		ret = ftell(fp) - buf_size + buf_pos - 1;
	}
	fseek(fp, orig_pos, SEEK_SET);
	return ret;
}

#ifndef HAVE_MEMRCHR
void *memrchr(const void *buffer, char c, size_t n)
{
	const unsigned char *p = buffer;

	for (p += n; n; n--) {
		if (*--p == c) {
			return (void *) p;
		}
	}
	return NULL;
}
#endif

long rev_fcharfind(FILE *fp, char val, unsigned int step)
{
#define BUFSZ 0x1000
	long ret = -1;
	unsigned int count = 0;
	static char buf[BUFSZ];
	long orig_pos = ftell(fp);
	long buf_pos = -1;
	long file_pos = orig_pos;
	long buf_size = BUFSZ;
	char *cur_found;

	while (count < step) {
		if (buf_pos <= 0) {
			if (file_pos > BUFSZ) {
				fseek(fp, file_pos - BUFSZ, SEEK_SET);
			} else {
				buf_size = file_pos;
				fseek(fp, 0, SEEK_SET);
			}
			file_pos = ftell(fp);
			buf_pos = fread(buf, 1, buf_size, fp);
		}
		cur_found = memrchr(buf, val, (size_t) buf_pos);
		if (cur_found != NULL) {
			buf_pos = cur_found - buf;
			count++;
		} else {
			buf_pos = -1;
			if (file_pos == 0) {
				break;
			}
		}
	}
	fseek(fp, orig_pos, SEEK_SET);
	if (count == step) {
		ret = file_pos + buf_pos;
	}
	return ret;
}

static void new_bar(char *buf, int w, int h, int usage)
{
	struct special_t *s = new_special(buf, BAR);

	s->arg = (usage > 255) ? 255 : ((usage < 0) ? 0 : usage);
	s->width = w;
	s->height = h;
}

static const char *scan_bar(const char *args, int *w, int *h)
{
	/* zero width means all space that is available */
	*w = 0;
	*h = 6;
	/* bar's argument is either height or height,width */
	if (args) {
		int n = 0;

		if (sscanf(args, "%d,%d %n", h, w, &n) <= 1) {
			sscanf(args, "%d %n", h, &n);
		}
		args += n;
	}

	return args;
}

static char *scan_font(const char *args)
{
	if (args && *args) {
		return strndup(args, DEFAULT_TEXT_BUFFER_SIZE);
	}

	return NULL;
}

#ifdef X11
static void new_font(char *buf, char *args)
{
	if (args) {
		struct special_t *s = new_special(buf, FONT);

		if (s->font_added > font_count || !s->font_added || (strncmp(args, fonts[s->font_added].name, DEFAULT_TEXT_BUFFER_SIZE) != EQUAL) ) {
			int tmp = selected_font;

			selected_font = s->font_added = addfont(args);
			load_fonts();
			selected_font = tmp;
		}
	} else {
		struct special_t *s = new_special(buf, FONT);
		int tmp = selected_font;

		selected_font = s->font_added = 0;
		selected_font = tmp;
	}
}
#endif
void graph_append(struct special_t *graph, double f, char showaslog)
{
	int i;

	if (showaslog) {
#ifdef MATH
		f = log10(f + 1);
#endif
	}
	
	if (!graph->scaled && f > graph->graph_scale) {
		f = graph->graph_scale;
	}

/* Already happens in new_graph
	if (graph->scaled) {
		graph->graph_scale = 1;
	}
*/
	graph->graph[0] = f;	/* add new data */
	/* shift all the data by 1 */
	for (i = graph->graph_width - 1; i > 0; i--) {
		graph->graph[i] = graph->graph[i - 1];
		if (graph->scaled && graph->graph[i] > graph->graph_scale) {
			/* check if we need to update the scale */
			graph->graph_scale = graph->graph[i];
		}
	}
}

short colour_depth = 0;
void set_up_gradient(void);

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

/* adjust color values depending on color depth */
static unsigned int adjust_colors(unsigned int color)
{
	double r, g, b;

	if (colour_depth == 0) {
		set_up_gradient();
	}
	if (colour_depth == 16) {
		r = (color & 0xff0000) >> 16;
		g = (color & 0xff00) >> 8;
		b =  color & 0xff;
		color  = (int) (r * CONST_8_TO_5_BITS) << 11;
		color |= (int) (g * CONST_8_TO_6_BITS) << 5;
		color |= (int) (b * CONST_8_TO_5_BITS);
	}
	return color;
}

static void new_graph(char *buf, int w, int h, unsigned int first_colour,
		unsigned int second_colour, double i, int scale, int append, char showaslog)
{
	struct special_t *s = new_special(buf, GRAPH);

	s->width = w;
	if (s->graph == NULL) {
		if (s->width > 0 && s->width < MAX_GRAPH_DEPTH) {
			// subtract 2 for the box
			s->graph_width = s->width /* - 2 */;
		} else {
			s->graph_width = MAX_GRAPH_DEPTH - 2;
		}
		s->graph = malloc(s->graph_width * sizeof(double));
		memset(s->graph, 0, s->graph_width * sizeof(double));
		s->graph_scale = 100;
	}
	s->height = h;
	s->first_colour = adjust_colors(first_colour);
	s->last_colour = adjust_colors(second_colour);
	if (scale != 0) {
		s->scaled = 0;
		s->graph_scale = scale;
		s->show_scale = 0;
	} else {
		s->scaled = 1;
		s->graph_scale = 1;
		s->show_scale = 1;
	}
	/* if (s->width) {
		s->graph_width = s->width - 2;	// subtract 2 for rectangle around
	} */
	if (showaslog) {
#ifdef MATH
		s->graph_scale = log10(s->graph_scale + 1);
#endif
	}
	if (append) {
		graph_append(s, i, showaslog);
	}
}

//don't use spaces in LOGGRAPH or NORMGRAPH if you change them
#define LOGGRAPH "log"
#define NORMGRAPH "normal"

static char *scan_graph(const char *args, int *w, int *h,
		unsigned int *first_colour, unsigned int *last_colour,
		unsigned int *scale, char *showaslog)
{
	const char *nographtype;
	char buf[64];
	buf[0] = 0;

	/* zero width means all space that is available */
	*w = 0;
	*h = 25;
	*first_colour = 0;
	*last_colour = 0;
	*scale = 0;
	if (args) {
		//set showaslog and place the rest of the args in nographtype
		if(strcasecmp(args, LOGGRAPH) == EQUAL) {
			*showaslog = TRUE;
			return NULL;
		}else if(strcasecmp(args, NORMGRAPH) == EQUAL) {
			*showaslog = FALSE;
			return NULL;
		}else if(strncasecmp(args, LOGGRAPH" ", strlen(LOGGRAPH) + 1 ) == EQUAL) {
			*showaslog = TRUE;
			nographtype = &args[strlen(LOGGRAPH) + 1];
		}else if(strncasecmp(args, NORMGRAPH" ", strlen(NORMGRAPH) + 1 ) == EQUAL) {
			*showaslog = FALSE;
			nographtype = &args[strlen(NORMGRAPH) + 1];
		}else{
			*showaslog = FALSE;
			nographtype = args;
		}
		//check the rest of the args
		if (sscanf(nographtype, "%d,%d %x %x %u", h, w, first_colour, last_colour, scale) == 5) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%d,%d %x %x", h, w, first_colour, last_colour) == 4) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %d,%d %x %x %u", buf, h, w, first_colour, last_colour, scale) == 6) {
			return strndup(buf, text_buffer_size);
		}
		*scale = 0;
		if (sscanf(nographtype, "%63s %d,%d %x %x", buf, h, w, first_colour, last_colour) == 5) {
			return strndup(buf, text_buffer_size);
		}
		buf[0] = '\0';
		*h = 25;
		*w = 0;
		if (sscanf(nographtype, "%x %x %u", first_colour, last_colour, scale) == 3) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%x %x", first_colour, last_colour) == 2) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %x %x %u", buf, first_colour, last_colour, scale) == 4) {
			return strndup(buf, text_buffer_size);
		}
		*scale = 0;
		if (sscanf(nographtype, "%63s %x %x", buf, first_colour, last_colour) == 3) {
			return strndup(buf, text_buffer_size);
		}
		buf[0] = '\0';
		*first_colour = 0;
		*last_colour = 0;
		if (sscanf(nographtype, "%d,%d %u", h, w, scale) == 3) {
			return NULL;
		}
		*scale = 0;
		if (sscanf(nographtype, "%d,%d", h, w) == 2) {
			return NULL;
		}
		if (sscanf(nographtype, "%63s %d,%d %u", buf, h, w, scale) < 4) {
			*scale = 0;
			//TODO: check the return value and throw an error?
			sscanf(nographtype, "%63s %d,%d", buf, h, w);
		}

		return strndup(buf, text_buffer_size);
	}

	if (buf[0] == '\0') {
		return NULL;
	} else {
		return strndup(buf, text_buffer_size);
	}
}

static inline void new_hr(char *buf, int a)
{
	new_special(buf, HORIZONTAL_LINE)->height = a;
}

static inline void new_stippled_hr(char *buf, int a, int b)
{
	struct special_t *s = new_special(buf, STIPPLED_HR);

	s->height = b;
	s->arg = a;
}

static inline void new_fg(char *buf, long c)
{
	new_special(buf, FG)->arg = c;
}

static inline void new_bg(char *buf, long c)
{
	new_special(buf, BG)->arg = c;
}

static inline void new_outline(char *buf, long c)
{
	new_special(buf, OUTLINE)->arg = c;
}

static inline void new_offset(char *buf, long c)
{
	new_special(buf, OFFSET)->arg = c;
}

static inline void new_voffset(char *buf, long c)
{
	new_special(buf, VOFFSET)->arg = c;
}

static inline void new_alignr(char *buf, long c)
{
	new_special(buf, ALIGNR)->arg = c;
}

// A postive offset pushes the text further left
static inline void new_alignc(char *buf, long c)
{
	new_special(buf, ALIGNC)->arg = c;
}

static inline void new_goto(char *buf, long c)
{
	new_special(buf, GOTO)->arg = c;
}

static inline void new_tab(char *buf, int a, int b)
{
	struct special_t *s = new_special(buf, TAB);

	s->width = a;
	s->arg = b;
}

/* quite boring functions */

static inline void for_each_line(char *b, void f(char *))
{
	char *ps, *pe;

	for (ps = b, pe = b; *pe; pe++) {
		if (*pe == '\n') {
			*pe = '\0';
			f(ps);
			*pe = '\n';
			ps = pe + 1;
		}
	}

	if (ps < pe) {
		f(ps);
	}
}

static void convert_escapes(char *buf)
{
	char *p = buf, *s = buf;

	while (*s) {
		if (*s == '\\') {
			s++;
			if (*s == 'n') {
				*p++ = '\n';
			} else if (*s == '\\') {
				*p++ = '\\';
			}
			s++;
		} else {
			*p++ = *s++;
		}
	}
	*p = '\0';
}

/* Prints anything normally printed with snprintf according to the current value
 * of use_spacer.  Actually slightly more flexible than snprintf, as you can
 * safely specify the destination buffer as one of your inputs.  */
static int spaced_print(char *, int, const char *, int, ...)
		__attribute__((format(printf, 3, 5)));

static int spaced_print(char *buf, int size, const char *format, int width, ...) {
	int len;
	va_list argp;
	char *tempbuf;

	if (size < 1) {
		return 0;
	}
	tempbuf = malloc(size * sizeof(char));

	// Passes the varargs along to vsnprintf
	va_start(argp, width);
	vsnprintf(tempbuf, size, format, argp);
	va_end(argp);

	switch (use_spacer) {
		case NO_SPACER:
			len = snprintf(buf, size, "%s", tempbuf);
			break;
		case LEFT_SPACER:
			len = snprintf(buf, size, "%*s", width - 1, tempbuf);
			break;
		case RIGHT_SPACER:
			len = snprintf(buf, size, "%-*s", width - 1, tempbuf);
			break;
	}
	free(tempbuf);
	return len;
}

/* converts from bytes to human readable format (k, M, G, T) */
static void human_readable(long long num, char *buf, int size)
{
	const char **suffix = suffixes;
	float fnum;
	int precision, len;
	static const int WIDTH = 10, SHORT_WIDTH = 8;
	int width;
	const char *format, *format2;

	if (short_units) {
		width = SHORT_WIDTH;
		format = "%lld%1s";
		format2 = "%.*f%1s";
	} else {
		width = WIDTH;
		format = "%lld%s";
		format2 = "%.*f%s";
	}

	if (llabs(num) < 1024LL) {
		spaced_print(buf, size, format, width, num, *suffix);
		return;
	}

	while (llabs(num / 1024) >= 1024LL && **(suffix + 2)) {
		num /= 1024;
		suffix++;
	}

	suffix++;
	fnum = num / 1024.0;

	precision = 3;
	do {
		precision--;
		if (precision < 0) {
			break;
		}
		len = spaced_print(buf, size, format2, width,
		                   precision, fnum, *suffix);
	} while (len >= MAX(width, size));
}

/* global object list root element */
static struct text_object global_root_object;

static void generate_text_internal(char *p, int p_max_size,
	struct text_object text_object, struct information *cur);

static inline void read_exec(const char *data, char *buf, const int size)
{
	FILE *fp = popen(data, "r");
	int length = fread(buf, 1, size, fp);

	pclose(fp);
	buf[length] = '\0';
	if (length > 0 && buf[length - 1] == '\n') {
		buf[length - 1] = '\0';
	}
}

void *threaded_exec(void *) __attribute__((noreturn));

void *threaded_exec(void *arg)
{
	char *p2;
	struct text_object *obj = (struct text_object *)arg;

	while (1) {
		p2 = obj->data.texeci.buffer;
		timed_thread_lock(obj->data.texeci.p_timed_thread);
		read_exec(obj->data.texeci.cmd, obj->data.texeci.buffer,
			text_buffer_size);
		while (*p2) {
			if (*p2 == '\001') {
				*p2 = ' ';
			}
			p2++;
		}
		timed_thread_unlock(obj->data.texeci.p_timed_thread);
		if (timed_thread_test(obj->data.texeci.p_timed_thread, 0)) {
			timed_thread_exit(obj->data.texeci.p_timed_thread);
		}
	}
	/* never reached */
}

static struct text_object *new_text_object_internal(void)
{
	struct text_object *obj = malloc(sizeof(struct text_object));
	memset(obj, 0, sizeof(struct text_object));
	return obj;
}

/* free the list of text objects root points to */
static void free_text_objects(struct text_object *root)
{
	struct text_object *obj;

	if (!root->prev) {
		return;
	}

#define data obj->data
	for (obj = root->prev; obj; obj = root->prev) {
		root->prev = obj->prev;
		switch (obj->type) {
#ifndef __OpenBSD__
			case OBJ_acpitemp:
				close(data.i);
				break;
			case OBJ_i2c:
			case OBJ_platform:
			case OBJ_hwmon:
				close(data.sysfs.fd);
				break;
#endif /* !__OpenBSD__ */
			case OBJ_time:
			case OBJ_utime:
				free(data.s);
				break;
			case OBJ_tztime:
				free(data.tztime.tz);
				free(data.tztime.fmt);
				break;
			case OBJ_mboxscan:
				free(data.mboxscan.args);
				free(data.mboxscan.output);
				break;
			case OBJ_mails:
			case OBJ_new_mails:
				free(data.local_mail.box);
				break;
			case OBJ_imap:
				free(info.mail);
				break;
			case OBJ_imap_unseen:
				if (!obj->global_mode) {
					free(data.mail);
				}
				break;
			case OBJ_imap_messages:
				if (!obj->global_mode) {
					free(data.mail);
				}
				break;
			case OBJ_pop3:
				free(info.mail);
				break;
			case OBJ_pop3_unseen:
				if (!obj->global_mode) {
					free(data.mail);
				}
				break;
			case OBJ_pop3_used:
				if (!obj->global_mode) {
					free(data.mail);
				}
				break;
			case OBJ_if_empty:
			case OBJ_if_existing:
			case OBJ_if_mounted:
			case OBJ_if_running:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
			case OBJ_tail:
				free(data.tail.logfile);
				free(data.tail.buffer);
				break;
			case OBJ_text:
			case OBJ_font:
			case OBJ_image:
			case OBJ_exec:
			case OBJ_execbar:
			case OBJ_execgraph:
			case OBJ_execp:
				free(data.s);
				break;
#ifdef HAVE_ICONV
			case OBJ_iconv_start:
				free_iconv();
				break;
#endif
#ifdef __LINUX__
			case OBJ_disk_protect:
				free(obj->data.s);
				break;
			case OBJ_if_up:
				free(obj->data.ifblock.s);
				free(obj->data.ifblock.str);
				break;
			case OBJ_if_gw:
				free(obj->data.ifblock.s);
				free(obj->data.ifblock.str);
			case OBJ_gw_iface:
			case OBJ_gw_ip:
				if (info.gw_info.iface) {
					free(info.gw_info.iface);
					info.gw_info.iface = 0;
				}
				if (info.gw_info.ip) {
					free(info.gw_info.ip);
					info.gw_info.ip = 0;
				}
				break;
			case OBJ_ioscheduler:
				if(obj->data.s)
					free(obj->data.s);
				break;
#endif
#ifdef XMMS2
			case OBJ_xmms2_artist:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				break;
			case OBJ_xmms2_album:
				if (info.xmms2.album) {
					free(info.xmms2.album);
					info.xmms2.album = 0;
				}
				break;
			case OBJ_xmms2_title:
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				break;
			case OBJ_xmms2_genre:
				if (info.xmms2.genre) {
					free(info.xmms2.genre);
					info.xmms2.genre = 0;
				}
				break;
			case OBJ_xmms2_comment:
				if (info.xmms2.comment) {
					free(info.xmms2.comment);
					info.xmms2.comment = 0;
				}
				break;
			case OBJ_xmms2_url:
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
			case OBJ_xmms2_date:
				if (info.xmms2.date) {
					free(info.xmms2.date);
					info.xmms2.date = 0;
				}
				break;
			case OBJ_xmms2_status:
				if (info.xmms2.status) {
					free(info.xmms2.status);
					info.xmms2.status = 0;
				}
				break;
			case OBJ_xmms2_playlist:
				if (info.xmms2.playlist) {
					free(info.xmms2.playlist);
					info.xmms2.playlist = 0;
				}
				break;
			case OBJ_xmms2_smart:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				if (info.xmms2.url) {
					free(info.xmms2.url);
					info.xmms2.url = 0;
				}
				break;
#endif
#ifdef BMPX
			case OBJ_bmpx_title:
			case OBJ_bmpx_artist:
			case OBJ_bmpx_album:
			case OBJ_bmpx_track:
			case OBJ_bmpx_uri:
			case OBJ_bmpx_bitrate:
				break;
#endif
#ifdef EVE
			case OBJ_eve:
				break;
#endif
#ifdef RSS
			case OBJ_rss:
				free(data.rss.uri);
				free(data.rss.action);
				break;
#endif
			case OBJ_pre_exec:
				break;
#ifndef __OpenBSD__
			case OBJ_battery:
				free(data.s);
				break;
			case OBJ_battery_time:
				free(data.s);
				break;
#endif /* !__OpenBSD__ */
			case OBJ_execpi:
			case OBJ_execi:
			case OBJ_execibar:
			case OBJ_execigraph:
				free(data.execi.cmd);
				free(data.execi.buffer);
				break;
			case OBJ_texeci:
				free(data.texeci.cmd);
				free(data.texeci.buffer);
				break;
			case OBJ_nameserver:
				free_dns_data();
				break;
			case OBJ_top:
			case OBJ_top_mem:
				if (info.first_process) {
					free_all_processes();
					info.first_process = NULL;
				}
				break;
#ifdef HDDTEMP
			case OBJ_hddtemp:
				free(data.hddtemp.dev);
				free(data.hddtemp.addr);
				if (data.hddtemp.temp)
					free(data.hddtemp.temp);
				break;
#endif
			case OBJ_entropy_avail:
			case OBJ_entropy_poolsize:
			case OBJ_entropy_bar:
				break;
			case OBJ_user_names:
				if (info.users.names) {
					free(info.users.names);
					info.users.names = 0;
				}
				break;
			case OBJ_user_terms:
				if (info.users.terms) {
					free(info.users.terms);
					info.users.terms = 0;
				}
				break;
			case OBJ_user_times:
				if (info.users.times) {
					free(info.users.times);
					info.users.times = 0;
				}
				break;
#ifdef SMAPI
			case OBJ_smapi:
			case OBJ_smapi_bat_perc:
			case OBJ_smapi_bat_temp:
			case OBJ_smapi_bat_power:
				free(data.s);
				break;
			case OBJ_if_smapi_bat_installed:
				free(data.ifblock.s);
				free(data.ifblock.str);
				break;
#endif
#ifdef NVIDIA
			case OBJ_nvidia:
				break;
#endif
#ifdef MPD
			case OBJ_mpd_title:
			case OBJ_mpd_artist:
			case OBJ_mpd_album:
			case OBJ_mpd_random:
			case OBJ_mpd_repeat:
			case OBJ_mpd_vol:
			case OBJ_mpd_bitrate:
			case OBJ_mpd_status:
			case OBJ_mpd_host:
			case OBJ_mpd_port:
			case OBJ_mpd_password:
			case OBJ_mpd_bar:
			case OBJ_mpd_elapsed:
			case OBJ_mpd_length:
			case OBJ_mpd_track:
			case OBJ_mpd_name:
			case OBJ_mpd_file:
			case OBJ_mpd_percent:
			case OBJ_mpd_smart:
			case OBJ_if_mpd_playing:
				free_mpd();
				break;
#endif
#ifdef MOC
    case OBJ_moc_state:
    case OBJ_moc_file:
    case OBJ_moc_title:
    case OBJ_moc_artist: 
    case OBJ_moc_song: 
    case OBJ_moc_album:
    case OBJ_moc_totaltime:
    case OBJ_moc_timeleft:
    case OBJ_moc_curtime:
    case OBJ_moc_bitrate:
    case OBJ_moc_rate:
      free_moc(&info.moc);
      break;
#endif
			case OBJ_scroll:
				free(data.scroll.text);
				break;
		}
		free(obj);
	}
#undef data
}

void scan_mixer_bar(const char *arg, int *a, int *w, int *h)
{
	char buf1[64];
	int n;

	if (arg && sscanf(arg, "%63s %n", buf1, &n) >= 1) {
		*a = mixer_init(buf1);
		scan_bar(arg + n, w, h);
	} else {
		*a = mixer_init(NULL);
		scan_bar(arg, w, h);
	}
}

/* strip a leading /dev/ if any, following symlinks first
 *
 * BEWARE: this function returns a pointer to static content
 *         which gets overwritten in consecutive calls. I.e.:
 *         this function is NOT reentrant.
 */
static const char *dev_name(const char *path)
{
	static char buf[255];	/* should be enough for pathnames */
	ssize_t buflen;

	if (!path)
		return NULL;

#define DEV_NAME(x) \
  x != NULL && strlen(x) > 5 && strncmp(x, "/dev/", 5) == 0 ? x + 5 : x
	if ((buflen = readlink(path, buf, 254)) == -1)
		return DEV_NAME(path);
	buf[buflen] = '\0';
	return DEV_NAME(buf);
#undef DEV_NAME
}

/* construct_text_object() creates a new text_object */
static struct text_object *construct_text_object(const char *s,
		const char *arg, long line, char allow_threaded)
{
	// struct text_object *obj = new_text_object();
	struct text_object *obj = new_text_object_internal();

	obj->line = line;

#define OBJ(a, n) if (strcmp(s, #a) == 0) { \
	obj->type = OBJ_##a; need_mask |= (1 << n); {
#define OBJ_THREAD(a, n) if (strcmp(s, #a) == 0 && allow_threaded) { \
	obj->type = OBJ_##a; need_mask |= (1 << n); {
#define END } } else

#ifdef X11
	if (s[0] == '#') {
		obj->type = OBJ_color;
		obj->data.l = get_x11_color(s);
	} else
#endif /* X11 */
#ifdef __OpenBSD__
	OBJ(freq, INFO_FREQ)
#else
	OBJ(acpitemp, 0)
		obj->data.i = open_acpi_temperature(arg);
	END OBJ(acpiacadapter, 0)
	END OBJ(freq, INFO_FREQ)
#endif /* !__OpenBSD__ */
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* ERR("freq: Invalid CPU number or you don't have that many CPUs! "
				"Displaying the clock for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
	END OBJ(freq_g, INFO_FREQ)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* ERR("freq_g: Invalid CPU number or you don't have that many "
				"CPUs! Displaying the clock for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
#if defined(__linux__)
	END OBJ(voltage_mv, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* ERR("voltage_mv: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;
	END OBJ(voltage_v, 0)
		get_cpu_count();
		if (!arg || !isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) == 0
				|| (unsigned int) atoi(&arg[0]) > info.cpu_count) {
			obj->data.cpu_index = 1;
			/* ERR("voltage_v: Invalid CPU number or you don't have that many "
				"CPUs! Displaying voltage for CPU 1."); */
		} else {
			obj->data.cpu_index = atoi(&arg[0]);
		}
		obj->a = 1;

#ifdef HAVE_IWLIB
	END OBJ(wireless_essid, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_essid: needs an argument");
		}
	END OBJ(wireless_mode, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_mode: needs an argument");
		}
	END OBJ(wireless_bitrate, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_bitrate: needs an argument");
		}
	END OBJ(wireless_ap, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_ap: needs an argument");
		}
	END OBJ(wireless_link_qual, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_link_qual: needs an argument");
		}
	END OBJ(wireless_link_qual_max, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_link_qual_max: needs an argument");
		}
	END OBJ(wireless_link_qual_perc, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_link_qual_perc: needs an argument");
		}
	END OBJ(wireless_link_bar, INFO_NET)
		if (arg) {
			arg = scan_bar(arg, &obj->a, &obj->b);
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("wireless_link_bar: needs an argument");
		}
#endif /* HAVE_IWLIB */

#endif /* __linux__ */

#ifndef __OpenBSD__
	END OBJ(acpifan, 0)
	END OBJ(battery, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_time, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_percent, 0)
		char bat[64];

		if (arg) {
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
	END OBJ(battery_bar, 0)
		char bat[64];
		obj->b = 6;
		if (arg) {
			arg = scan_bar(arg, &obj->a, &obj->b);
			sscanf(arg, "%63s", bat);
		} else {
			strcpy(bat, "BAT0");
		}
		obj->data.s = strndup(bat, text_buffer_size);
#endif /* !__OpenBSD__ */

#if defined(__linux__)
	END OBJ(disk_protect, 0)
		if (arg)
			obj->data.s = strndup(dev_name(arg), text_buffer_size);
		else
			CRIT_ERR("disk_protect needs an argument");
	END OBJ(i8k_version, INFO_I8K)
	END OBJ(i8k_bios, INFO_I8K)
	END OBJ(i8k_serial, INFO_I8K)
	END OBJ(i8k_cpu_temp, INFO_I8K)
	END OBJ(i8k_left_fan_status, INFO_I8K)
	END OBJ(i8k_right_fan_status, INFO_I8K)
	END OBJ(i8k_left_fan_rpm, INFO_I8K)
	END OBJ(i8k_right_fan_rpm, INFO_I8K)
	END OBJ(i8k_ac_status, INFO_I8K)
	END OBJ(i8k_buttons_status, INFO_I8K)
	END OBJ(ibm_fan, 0)
	END OBJ(ibm_temps, 0)
		if (!arg) {
			CRIT_ERR("ibm_temps: needs an argument");
		}
		if (!isdigit(arg[0]) || strlen(arg) >= 2 || atoi(&arg[0]) >= 8) {
			obj->data.sensor = 0;
			ERR("Invalid temperature sensor! Sensor number must be 0 to 7. "
				"Using 0 (CPU temp sensor).");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(ibm_volume, 0)
	END OBJ(ibm_brightness, 0)
	END OBJ(if_up, 0)
		if (!arg) {
			ERR("if_up needs an argument");
			obj->data.ifblock.s = 0;
		} else
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		obj_be_ifblock_if(obj);
	END OBJ(if_gw, 0)
		obj_be_ifblock_if(obj);
	END OBJ(ioscheduler, 0)
		if (!arg) {
			CRIT_ERR("get_ioscheduler needs an argument (e.g. hda)");
			obj->data.s = 0;
		} else
			obj->data.s = strndup(dev_name(arg), text_buffer_size);
	END OBJ(laptop_mode, 0)
	END OBJ(pb_battery, 0)
		if (arg && strcmp(arg, "status") == EQUAL) {
			obj->data.i = PB_BATT_STATUS;
		} else if (arg && strcmp(arg, "percent") == EQUAL) {
			obj->data.i = PB_BATT_PERCENT;
		} else if (arg && strcmp(arg, "time") == EQUAL) {
			obj->data.i = PB_BATT_TIME;
		} else {
			ERR("pb_battery: needs one argument: status, percent or time");
			free(obj);
			return NULL;
		}

#endif /* __linux__ */
#if defined(__OpenBSD__)
	END OBJ(obsd_sensors_temp, 0)
		if (!arg) {
			CRIT_ERR("obsd_sensors_temp: needs an argument");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			ERR("Invalid temperature sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_sensors_fan, 0)
		if (!arg) {
			CRIT_ERR("obsd_sensors_fan: needs 2 arguments (device and sensor "
				"number)");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			ERR("Invalid fan sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_sensors_volt, 0)
		if (!arg) {
			CRIT_ERR("obsd_sensors_volt: needs 2 arguments (device and sensor "
				"number)");
		}
		if (!isdigit(arg[0]) || atoi(&arg[0]) < 0
				|| atoi(&arg[0]) > OBSD_MAX_SENSORS - 1) {
			obj->data.sensor = 0;
			ERR("Invalid voltage sensor number!");
		}
		obj->data.sensor = atoi(&arg[0]);
	END OBJ(obsd_vendor, 0)
	END OBJ(obsd_product, 0)
#endif /* __OpenBSD__ */
	END OBJ(buffers, INFO_BUFFERS)
	END OBJ(cached, INFO_BUFFERS)
	END OBJ(cpu, INFO_CPU)
		if (arg) {
			if (strncmp(arg, "cpu", 3) == EQUAL && isdigit(arg[3])) {
				obj->data.cpu_index = atoi(&arg[3]);
				arg += 4;
			} else {
				obj->data.cpu_index = 0;
			}
			DBGP2("Adding $cpu for CPU %d", obj->data.cpu_index);
		} else {
			obj->data.cpu_index = 0;
		}
	END OBJ(cpubar, INFO_CPU)
		if (arg) {
			if (strncmp(arg, "cpu", 3) == EQUAL && isdigit(arg[3])) {
				obj->data.cpu_index = atoi(&arg[3]);
				arg += 4;
			} else {
				obj->data.cpu_index = 0;
			}
			scan_bar(arg, &obj->a, &obj->b);
		} else {
			scan_bar(arg, &obj->a, &obj->b);
			obj->data.cpu_index = 0;
		}
		DBGP2("Adding $cpubar for CPU %d", obj->data.cpu_index);
	END OBJ(cpugraph, INFO_CPU)
		char *buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		if (buf) {
			if (strncmp(buf, "cpu", 3) == EQUAL && isdigit(buf[3])) {
				obj->data.cpu_index = atoi(&buf[3]);
			} else {
				obj->data.cpu_index = 0;
			}
			free(buf);
		}
		DBGP2("Adding $cpugraph for CPU %d", obj->data.cpu_index);
	END OBJ(loadgraph, INFO_LOADAVG)
		char *buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
				&obj->e, &obj->showaslog);
		if (buf) {
			int a = 1, r = 3;
			if (arg) {
				r = sscanf(arg, "%d", &a);
			}
			obj->data.loadavg[0] = (r >= 1) ? (unsigned char) a : 0;
			free(buf);
		}
#if defined(__linux__)
	END OBJ(diskio, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
	END OBJ(diskio_read, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
	END OBJ(diskio_write, INFO_DISKIO)
		obj->data.diskio = prepare_diskio_stat(dev_name(arg));
	END OBJ(diskiograph, INFO_DISKIO)
		char *buf = scan_graph(dev_name(arg), &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		obj->data.diskio = prepare_diskio_stat(buf);
		if (buf)
			free(buf);
	END OBJ(diskiograph_read, INFO_DISKIO)
		char *buf = scan_graph(dev_name(arg), &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		obj->data.diskio = prepare_diskio_stat(buf);
		if (buf)
			free(buf);
	END OBJ(diskiograph_write, INFO_DISKIO)
		char *buf = scan_graph(dev_name(arg), &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		obj->data.diskio = prepare_diskio_stat(buf);
		if (buf)
			free(buf);
#endif
	END OBJ(color, 0)
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_fg_color;
#endif /* X11 */
	END OBJ(color0, 0)
		obj->data.l = color0;
	END OBJ(color1, 0)
		obj->data.l = color1;
	END OBJ(color2, 0)
		obj->data.l = color2;
	END OBJ(color3, 0)
		obj->data.l = color3;
	END OBJ(color4, 0)
		obj->data.l = color4;
	END OBJ(color5, 0)
		obj->data.l = color5;
	END OBJ(color6, 0)
		obj->data.l = color6;
	END OBJ(color7, 0)
		obj->data.l = color7;
	END OBJ(color8, 0)
		obj->data.l = color8;
	END OBJ(color9, 0)
		obj->data.l = color9;
	END OBJ(font, 0)
		obj->data.s = scan_font(arg);
	END OBJ(conky_version, 0)
	END OBJ(conky_build_date, 0)
	END OBJ(conky_build_arch, 0)
	END OBJ(downspeed, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("downspeed needs argument");
		}
	END OBJ(downspeedf, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("downspeedf needs argument");
		}
	END OBJ(downspeedgraph, INFO_NET)
		char *buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		// default to DEFAULTNETDEV
		buf = strndup(buf ? buf : "DEFAULTNETDEV", text_buffer_size);
		obj->data.net = get_net_stat(buf);
		free(buf);
	END OBJ(else, 0)
		obj_be_ifblock_else(obj);
	END OBJ(endif, 0)
		obj_be_ifblock_endif(obj);
	END OBJ(image, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
#ifdef HAVE_POPEN
	END OBJ(exec, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execp, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execbar, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execgraph, 0)
		obj->data.s = strndup(arg ? arg : "", text_buffer_size);
	END OBJ(execibar, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			ERR("${execibar <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
		}
	END OBJ(execigraph, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			ERR("${execigraph <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
		}
	END OBJ(execi, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			ERR("${execi <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
			obj->data.execi.buffer = malloc(text_buffer_size);
		}
	END OBJ(execpi, 0)
		int n;

		if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
			char buf[256];

			ERR("${execi <interval> command}");
			obj->type = OBJ_text;
			snprintf(buf, 256, "${%s}", s);
			obj->data.s = strndup(buf, text_buffer_size);
		} else {
			obj->data.execi.cmd = strndup(arg + n, text_buffer_size);
			obj->data.execi.buffer = malloc(text_buffer_size);
		}
	END OBJ_THREAD(texeci, 0)
			int n;

			if (!arg || sscanf(arg, "%f %n", &obj->data.texeci.interval, &n) <= 0) {
				char buf[256];

				ERR("${texeci <interval> command}");
				obj->type = OBJ_text;
				snprintf(buf, 256, "${%s}", s);
				obj->data.s = strndup(buf, text_buffer_size);
			} else {
				obj->data.texeci.cmd = strndup(arg + n, text_buffer_size);
				obj->data.texeci.buffer = malloc(text_buffer_size);
			}
			obj->data.texeci.p_timed_thread = NULL;
	END	OBJ(pre_exec, 0)
		obj->type = OBJ_text;
	if (arg) {
		char buf[2048];

		read_exec(arg, buf, sizeof(buf));
		obj->data.s = strndup(buf, text_buffer_size);
	} else {
		obj->data.s = strndup("", text_buffer_size);
	}
#endif
	END OBJ(fs_bar, INFO_FS)
		arg = scan_bar(arg, &obj->data.fsbar.w, &obj->data.fsbar.h);
	if (arg) {
			while (isspace(*arg)) {
				arg++;
			}
			if (*arg == '\0') {
				arg = "/";
			}
		} else {
			arg = "/";
		}
		obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_bar_free, INFO_FS)
		arg = scan_bar(arg, &obj->data.fsbar.w, &obj->data.fsbar.h);
		if (arg) {
			while (isspace(*arg)) {
				arg++;
			}
			if (*arg == '\0') {
				arg = "/";
			}
		} else {
			arg = "/";
		}

		obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_free, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used_perc, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_free_perc, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_size, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_type, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used, INFO_FS)
		if (!arg) {
			arg = "/";
		}
		obj->data.fs = prepare_fs_stat(arg);
	END OBJ(hr, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(nameserver, INFO_DNS)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(offset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(voffset, 0)
		obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(goto, 0)

		if (!arg) {
			ERR("goto needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${goto}", text_buffer_size);
			return NULL;
		}

		obj->data.i = atoi(arg);

	END OBJ(tab, 0)
		int a = 10, b = 0;

		if (arg) {
			if (sscanf(arg, "%d %d", &a, &b) != 2) {
				sscanf(arg, "%d", &b);
			}
		}
		if (a <= 0) {
			a = 1;
		}
		obj->data.pair.a = a;
		obj->data.pair.b = b;

#ifndef __OpenBSD__
	END OBJ(i2c, INFO_SYSFS)
		char buf1[64], buf2[64];
		int n;

		if (!arg) {
			ERR("i2c needs arguments");
			obj->type = OBJ_text;
			// obj->data.s = strndup("${i2c}", text_buffer_size);
			return NULL;
		}

		if (sscanf(arg, "%63s %63s %d", buf1, buf2, &n) != 3) {
			/* if scanf couldn't read three values, read type and num and use
			 * default device */
			sscanf(arg, "%63s %d", buf2, &n);
			obj->data.sysfs.fd = open_i2c_sensor(0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		} else {
			obj->data.sysfs.fd = open_i2c_sensor(buf1, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		}

	END OBJ(platform, INFO_SYSFS)
		char buf1[64], buf2[64];
		int n;

		if (!arg) {
			ERR("platform needs arguments");
			obj->type = OBJ_text;
			return NULL;
		}

		if (sscanf(arg, "%63s %63s %d", buf1, buf2, &n) != 3) {
			/* if scanf couldn't read three values, read type and num and use
			 * default device */
			sscanf(arg, "%63s %d", buf2, &n);
			obj->data.sysfs.fd = open_platform_sensor(0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		} else {
			obj->data.sysfs.fd = open_platform_sensor(buf1, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		}

	END OBJ(hwmon, INFO_SYSFS)
		char buf1[64], buf2[64];
		int n;

		if (!arg) {
			ERR("hwmon needs argumanets");
			obj->type = OBJ_text;
			return NULL;
		}

		if (sscanf(arg, "%63s %63s %d", buf1, buf2, &n) != 3) {
			/* if scanf couldn't read three values, read type and num and use
			 * default device */
			sscanf(arg, "%63s %d", buf2, &n);
			obj->data.sysfs.fd = open_hwmon_sensor(0, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		} else {
			obj->data.sysfs.fd = open_hwmon_sensor(buf1, buf2, n,
				&obj->data.sysfs.arg, obj->data.sysfs.devtype);
			strncpy(obj->data.sysfs.type, buf2, 63);
		}
#endif /* !__OpenBSD__ */

	END OBJ(top, INFO_TOP)
		char buf[64];
		int n;

		if (!arg) {
			ERR("top needs arguments");
			obj->type = OBJ_text;
			// obj->data.s = strndup("${top}", text_buffer_size);
			return NULL;
		}
		if (sscanf(arg, "%63s %i", buf, &n) == 2) {
			if (strcmp(buf, "name") == EQUAL) {
				obj->data.top.type = TOP_NAME;
			} else if (strcmp(buf, "cpu") == EQUAL) {
				obj->data.top.type = TOP_CPU;
			} else if (strcmp(buf, "pid") == EQUAL) {
				obj->data.top.type = TOP_PID;
			} else if (strcmp(buf, "mem") == EQUAL) {
				obj->data.top.type = TOP_MEM;
			} else if (strcmp(buf, "time") == EQUAL) {
				obj->data.top.type = TOP_TIME;
			} else if (strcmp(buf, "mem_res") == EQUAL) {
				obj->data.top.type = TOP_MEM_RES;
			} else if (strcmp(buf, "mem_vsize") == EQUAL) {
				obj->data.top.type = TOP_MEM_VSIZE;
			} else {
				ERR("invalid arg for top");
				return NULL;
			}
			if (n < 1 || n > 10) {
				CRIT_ERR("invalid arg for top");
				return NULL;
			} else {
				obj->data.top.num = n - 1;
				top_cpu = 1;
			}
		} else {
			ERR("invalid args given for top");
			return NULL;
		}
	END OBJ(top_mem, INFO_TOP)
		char buf[64];
		int n;

		if (!arg) {
			ERR("top_mem needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${top_mem}", text_buffer_size);
			return NULL;
		}
		if (sscanf(arg, "%63s %i", buf, &n) == 2) {
			if (strcmp(buf, "name") == EQUAL) {
				obj->data.top.type = TOP_NAME;
			} else if (strcmp(buf, "cpu") == EQUAL) {
				obj->data.top.type = TOP_CPU;
			} else if (strcmp(buf, "pid") == EQUAL) {
				obj->data.top.type = TOP_PID;
			} else if (strcmp(buf, "mem") == EQUAL) {
				obj->data.top.type = TOP_MEM;
			} else if (strcmp(buf, "time") == EQUAL) {
				obj->data.top.type = TOP_TIME;
			} else if (strcmp(buf, "mem_res") == EQUAL) {
				obj->data.top.type = TOP_MEM_RES;
			} else if (strcmp(buf, "mem_vsize") == EQUAL) {
				obj->data.top.type = TOP_MEM_VSIZE;
			} else {
				ERR("invalid arg for top");
				return NULL;
			}
			if (n < 1 || n > 10) {
					CRIT_ERR("invalid arg for top");
				return NULL;
			} else {
				obj->data.top.num = n - 1;
				top_mem = 1;
			}
		} else {
			ERR("invalid args given for top");
			return NULL;
		}
	END OBJ(addr, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("addr needs argument");
		}
#if defined(__linux__)
	END OBJ(addrs, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("addrs needs argument");
		}
#endif /* __linux__ */
	END OBJ(tail, 0)
		char buf[64];
		int n1, n2;
		struct stat st;

		if (!arg) {
			ERR("tail needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${tail}", text_buffer_size);
			return NULL;
		}
		if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 2) {
			if (n1 < 1 || n1 > MAX_TAIL_LINES) {
				CRIT_ERR("invalid arg for tail, number of lines must be "
						"between 1 and %i", MAX_TAIL_LINES);
				return NULL;
			} else {
				FILE *fp = NULL;
				int fd;

				obj->data.tail.fd = -1;

				if (stat(buf, &st) == 0) {
					if (S_ISFIFO(st.st_mode)) {
						fd = open(buf, O_RDONLY | O_NONBLOCK);

						if (fd == -1) {
							CRIT_ERR("tail logfile does not exist, or you do "
								"not have correct permissions");
						}

						obj->data.tail.fd = fd;
					} else {
						fp = fopen(buf, "r");
					}
				}

				if (fp || obj->data.tail.fd != -1) {
					obj->data.tail.logfile = malloc(text_buffer_size);
					strcpy(obj->data.tail.logfile, buf);
					obj->data.tail.wantedlines = n1;
					obj->data.tail.interval = update_interval * 2;

					if (obj->data.tail.fd == -1) {
						fclose(fp);
					}
				} else {
					// fclose(fp);
					CRIT_ERR("tail logfile does not exist, or you do not have "
						"correct permissions");
				}
			}
		} else if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 3) {
			if (n1 < 1 || n1 > MAX_TAIL_LINES) {
				CRIT_ERR("invalid arg for tail, number of lines must be "
					"between 1 and %i", MAX_TAIL_LINES);
				return NULL;
			} else if (n2 < 1 || n2 < update_interval) {
				CRIT_ERR("invalid arg for tail, interval must be greater than "
					"0 and "PACKAGE_NAME"'s interval");
				return NULL;
			} else {
				FILE *fp = 0;
				int fd;

				obj->data.tail.fd = -1;

				if (stat(buf, &st) == 0) {
					if (S_ISFIFO(st.st_mode)) {
						fd = open(buf, O_RDONLY | O_NONBLOCK);

						if (fd == -1) {
							CRIT_ERR("tail logfile does not exist, or you do "
								"not have correct permissions");
						}

						obj->data.tail.fd = fd;
					} else {
						fp = fopen(buf, "r");
					}
				}

				if (fp || obj->data.tail.fd != -1) {
					obj->data.tail.logfile = malloc(text_buffer_size);
					strcpy(obj->data.tail.logfile, buf);
					obj->data.tail.wantedlines = n1;
					obj->data.tail.interval = n2;

					if (obj->data.tail.fd == -1) {
						fclose(fp);
					}
				} else {
					// fclose(fp);
					CRIT_ERR("tail logfile does not exist, or you do not have "
						"correct permissions");
				}
			}
		} else {
			ERR("invalid args given for tail");
			return NULL;
		}
		/* asumming all else worked */
		obj->data.tail.buffer = malloc(text_buffer_size * 20);
	END OBJ(head, 0)
		char buf[64];
		int n1, n2;

		if (!arg) {
			ERR("head needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${head}", text_buffer_size);
			return NULL;
		}
		if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 2) {
			if (n1 < 1 || n1 > MAX_TAIL_LINES) {
				CRIT_ERR("invalid arg for head, number of lines must be "
					"between 1 and %i", MAX_TAIL_LINES);
				return NULL;
			} else {
				FILE *fp;

				fp = fopen(buf, "r");
				if (fp != NULL) {
					obj->data.tail.logfile = malloc(text_buffer_size);
					strcpy(obj->data.tail.logfile, buf);
					obj->data.tail.wantedlines = n1;
					obj->data.tail.interval = update_interval * 2;
					fclose(fp);
				} else {
					// fclose(fp);
					CRIT_ERR("head logfile does not exist, or you do not have "
						"correct permissions");
				}
			}
		} else if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 3) {
			if (n1 < 1 || n1 > MAX_TAIL_LINES) {
				CRIT_ERR("invalid arg for head, number of lines must be "
					"between 1 and %i", MAX_TAIL_LINES);
				return NULL;
			} else if (n2 < 1 || n2 < update_interval) {
				CRIT_ERR("invalid arg for head, interval must be greater than "
					"0 and "PACKAGE_NAME"'s interval");
				return NULL;
			} else {
				FILE *fp;

				fp = fopen(buf, "r");
				if (fp != NULL) {
					obj->data.tail.logfile = malloc(text_buffer_size);
					strcpy(obj->data.tail.logfile, buf);
					obj->data.tail.wantedlines = n1;
					obj->data.tail.interval = n2;
					fclose(fp);
				} else {
					// fclose(fp);
					CRIT_ERR("head logfile does not exist, or you do not have "
						"correct permissions");
				}
			}
		} else {
			ERR("invalid args given for head");
			return NULL;
		}
		/* asumming all else worked */
		obj->data.tail.buffer = malloc(text_buffer_size * 20);
	END OBJ(lines, 0)
		if (arg) {
			obj->data.s = strdup(arg);
		}else{
			CRIT_ERR("lines needs a argument");
		}
	END OBJ(words, 0)
		if (arg) {
			obj->data.s = strdup(arg);
		}else{
			CRIT_ERR("words needs a argument");
		}
	END OBJ(loadavg, INFO_LOADAVG)
		int a = 1, b = 2, c = 3, r = 3;

		if (arg) {
			r = sscanf(arg, "%d %d %d", &a, &b, &c);
			if (r >= 3 && (c < 1 || c > 3)) {
				r--;
			}
			if (r >= 2 && (b < 1 || b > 3)) {
				r--, b = c;
			}
			if (r >= 1 && (a < 1 || a > 3)) {
				r--, a = b, b = c;
			}
		}
		obj->data.loadavg[0] = (r >= 1) ? (unsigned char) a : 0;
		obj->data.loadavg[1] = (r >= 2) ? (unsigned char) b : 0;
		obj->data.loadavg[2] = (r >= 3) ? (unsigned char) c : 0;
	END OBJ(if_empty, 0)
		if (!arg) {
			ERR("if_empty needs an argument");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		}
		obj_be_ifblock_if(obj);
	END OBJ(if_existing, 0)
		if (!arg) {
			ERR("if_existing needs an argument or two");
			obj->data.ifblock.s = NULL;
			obj->data.ifblock.str = NULL;
		} else {
			char buf1[256], buf2[256];
			int r = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

			if (r == 1) {
				obj->data.ifblock.s = strndup(buf1, text_buffer_size);
				obj->data.ifblock.str = NULL;
			} else {
				obj->data.ifblock.s = strndup(buf1, text_buffer_size);
				obj->data.ifblock.str = strndup(buf2, text_buffer_size);
			}
		}
		DBGP("if_existing: '%s' '%s'", obj->data.ifblock.s, obj->data.ifblock.str);
		obj_be_ifblock_if(obj);
	END OBJ(if_mounted, 0)
		if (!arg) {
			ERR("if_mounted needs an argument");
			obj->data.ifblock.s = 0;
		} else {
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		}
		obj_be_ifblock_if(obj);
	END OBJ(if_running, 0)
		if (arg) {
			char buf[256];

			snprintf(buf, 256, "pidof %s >/dev/null", arg);
			obj->data.ifblock.s = strndup(buf, text_buffer_size);
		} else {
			ERR("if_running needs an argument");
			obj->data.ifblock.s = 0;
		}
		obj_be_ifblock_if(obj);
	END OBJ(kernel, 0)
	END OBJ(machine, 0)
	END OBJ(mails, 0)
		float n1;
		char box[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			/* Kapil: Changed from MAIL_FILE to
			   current_mail_spool since the latter
			   is a copy of the former if undefined
			   but the latter should take precedence
			   if defined */
			strncpy(box, current_mail_spool, sizeof(box));
		} else {
			if (sscanf(arg, "%s %f", box, &n1) != 2) {
				n1 = 9.5;
				strncpy(box, arg, sizeof(box));
			}
		}

		variable_substitute(box, dst, sizeof(dst));
		obj->data.local_mail.box = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(mboxscan, 0)
		obj->data.mboxscan.args = (char *) malloc(text_buffer_size);
		obj->data.mboxscan.output = (char *) malloc(text_buffer_size);
		/* if '1' (in mboxscan.c) then there was SIGUSR1, hmm */
		obj->data.mboxscan.output[0] = 1;
		strncpy(obj->data.mboxscan.args, arg, text_buffer_size);
	END OBJ(mem, INFO_MEM)
	END OBJ(memeasyfree, INFO_MEM)
	END OBJ(memfree, INFO_MEM)
	END OBJ(memmax, INFO_MEM)
	END OBJ(memperc, INFO_MEM)
	END OBJ(membar, INFO_MEM)
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(memgraph, INFO_MEM)
		char *buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		if (buf) {
			free(buf);
		}
	END OBJ(mixer, INFO_MIXER)
		obj->data.l = mixer_init(arg);
	END OBJ(mixerl, INFO_MIXER)
		obj->data.l = mixer_init(arg);
	END OBJ(mixerr, INFO_MIXER)
		obj->data.l = mixer_init(arg);
	END OBJ(mixerbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
	END OBJ(mixerlbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
	END OBJ(mixerrbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l, &obj->data.mixerbar.w,
			&obj->data.mixerbar.h);
#ifdef X11
	END OBJ(monitor, INFO_X11)
	END OBJ(monitor_number, INFO_X11)
#endif
	END OBJ(new_mails, 0)
		float n1;
		char box[256], dst[256];

		if (!arg) {
			n1 = 9.5;
			/* Kapil: Changed from MAIL_FILE to
			   current_mail_spool since the latter
			   is a copy of the former if undefined
			   but the latter should take precedence
			   if defined */
			strncpy(box, current_mail_spool, sizeof(box));
		} else {
			if (sscanf(arg, "%s %f", box, &n1) != 2) {
				n1 = 9.5;
				strncpy(box, arg, sizeof(box));
			}
		}

		variable_substitute(box, dst, sizeof(dst));
		obj->data.local_mail.box = strndup(dst, text_buffer_size);
		obj->data.local_mail.interval = n1;
	END OBJ(nodename, 0)
	END OBJ(processes, INFO_PROCS)
	END OBJ(running_processes, INFO_RUN_PROCS)
	END OBJ(shadecolor, 0)
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_bg_color;
#endif /* X11 */
	END OBJ(outlinecolor, 0)
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_out_color;
#endif /* X11 */
	END OBJ(stippled_hr, 0)
#ifdef X11
		int a = stippled_borders, b = 1;

		if (arg) {
			if (sscanf(arg, "%d %d", &a, &b) != 2) {
				sscanf(arg, "%d", &b);
			}
		}
		if (a <= 0) {
			a = 1;
		}
		obj->data.pair.a = a;
		obj->data.pair.b = b;
#endif /* X11 */
	END OBJ(swap, INFO_MEM)
	END OBJ(swapmax, INFO_MEM)
	END OBJ(swapperc, INFO_MEM)
	END OBJ(swapbar, INFO_MEM)
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(sysname, 0)
#ifndef __OpenBSD__
	END OBJ(temp1, INFO_SYSFS)
		obj->type = OBJ_i2c;
		obj->data.sysfs.fd = open_i2c_sensor(0, "temp", 1,
			&obj->data.sysfs.arg, obj->data.sysfs.devtype);
	END OBJ(temp2, INFO_SYSFS)
		obj->type = OBJ_i2c;
		obj->data.sysfs.fd = open_i2c_sensor(0, "temp", 2,
			&obj->data.sysfs.arg, obj->data.sysfs.devtype);
#endif
	END OBJ(time, 0)
		obj->data.s = strndup(arg ? arg : "%F %T", text_buffer_size);
	END OBJ(utime, 0)
		obj->data.s = strndup(arg ? arg : "%F %T", text_buffer_size);
	END OBJ(tztime, 0)
		char buf1[256], buf2[256], *fmt, *tz;

		fmt = tz = NULL;
		if (arg) {
			int nArgs = sscanf(arg, "%255s %255[^\n]", buf1, buf2);

			switch (nArgs) {
				case 2:
					tz = buf1;
				case 1:
					fmt = buf2;
			}
		}

		obj->data.tztime.fmt = strndup(fmt ? fmt : "%F %T", text_buffer_size);
		obj->data.tztime.tz = tz ? strndup(tz, text_buffer_size) : NULL;
#ifdef HAVE_ICONV
	END OBJ(iconv_start, 0)
		if (iconv_converting) {
			CRIT_ERR("You must stop your last iconv conversion before "
				"starting another");
		}
		if (arg) {
			char iconv_from[CODEPAGE_LENGTH];
			char iconv_to[CODEPAGE_LENGTH];

			if (sscanf(arg, "%s %s", iconv_from, iconv_to) != 2) {
				CRIT_ERR("Invalid arguments for iconv_start");
			} else {
				iconv_t new_iconv;

				new_iconv = iconv_open(iconv_to, iconv_from);
				if (new_iconv == (iconv_t) (-1)) {
					ERR("Can't convert from %s to %s.", iconv_from, iconv_to);
				} else {
					obj->a = register_iconv(&new_iconv);
					iconv_converting = 1;
				}
			}
		} else {
			CRIT_ERR("Iconv requires arguments");
		}
	END OBJ(iconv_stop, 0)
		iconv_converting = 0;

#endif
	END OBJ(totaldown, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("totaldown needs argument");
		}
	END OBJ(totalup, INFO_NET)
		obj->data.net = get_net_stat(arg);
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("totalup needs argument");
		}
	END OBJ(updates, 0)
	END OBJ(alignr, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(alignc, 0)
		obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(upspeed, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("upspeed needs argument");
		}
	END OBJ(upspeedf, INFO_NET)
		if (arg) {
			obj->data.net = get_net_stat(arg);
		} else {
			CRIT_ERR("upspeedf needs argument");
		}

	END OBJ(upspeedgraph, INFO_NET)
		char *buf = scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d,
			&obj->e, &obj->showaslog);

		// default to DEFAULTNETDEV
		buf = strndup(buf ? buf : "DEFAULTNETDEV", text_buffer_size);
		obj->data.net = get_net_stat(buf);
		free(buf);
	END OBJ(uptime_short, INFO_UPTIME)
	END OBJ(uptime, INFO_UPTIME)
	END OBJ(user_names, INFO_USERS)
	END OBJ(user_times, INFO_USERS)
	END OBJ(user_terms, INFO_USERS)
	END OBJ(user_number, INFO_USERS)
#if defined(__linux__)
	END OBJ(gw_iface, INFO_GW)
	END OBJ(gw_ip, INFO_GW)
	END OBJ(if_gw, INFO_GW)
#endif /* !__linux__ */
#ifndef __OpenBSD__
	END OBJ(adt746xcpu, 0)
	END OBJ(adt746xfan, 0)
#endif /* !__OpenBSD__ */
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
	END OBJ(apm_adapter, 0)
	END OBJ(apm_battery_life, 0)
	END OBJ(apm_battery_time, 0)
#endif /* __FreeBSD__ */
	END OBJ_THREAD(imap_unseen, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(IMAP_TYPE, arg);
			obj->global_mode = 0;
		} else {
			obj->global_mode = 1;
		}
	END OBJ_THREAD(imap_messages, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(IMAP_TYPE, arg);
			obj->global_mode = 0;
		} else {
			obj->global_mode = 1;
		}
	END OBJ_THREAD(pop3_unseen, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(POP3_TYPE, arg);
			obj->global_mode = 0;
		} else {
			obj->global_mode = 1;
		}
	END OBJ_THREAD(pop3_used, 0)
		if (arg) {
			// proccss
			obj->data.mail = parse_mail_args(POP3_TYPE, arg);
			obj->global_mode = 0;
		} else {
			obj->global_mode = 1;
		}
#ifdef SMAPI
	END OBJ(smapi, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			ERR("smapi needs an argument");
	END OBJ(if_smapi_bat_installed, 0)
		if (!arg) {
			ERR("if_smapi_bat_installed needs an argument");
			obj->data.ifblock.s = 0;
		} else
			obj->data.ifblock.s = strndup(arg, text_buffer_size);
		obj_be_ifblock_if(obj);
	END OBJ(smapi_bat_perc, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			ERR("smapi_bat_perc needs an argument");
	END OBJ(smapi_bat_temp, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			ERR("smapi_bat_temp needs an argument");
	END OBJ(smapi_bat_power, 0)
		if (arg)
			obj->data.s = strndup(arg, text_buffer_size);
		else
			ERR("smapi_bat_power needs an argument");
	END OBJ(smapi_bat_bar, 0)
		if(arg) {
			int cnt;
			if(sscanf(arg, "%i %n", &obj->data.i, &cnt) <= 0) {
				ERR("first argument to smapi_bat_bar must be an integer value");
				obj->data.i = -1;
			} else {
				obj->b = 4;
				arg = scan_bar(arg + cnt, &obj->a, &obj->b);
			}
		} else
			ERR("smapi_bat_bar needs an argument");
#endif /* SMAPI */
#ifdef MPD
#define mpd_set_maxlen(name) \
		if (arg) { \
			int i; \
			sscanf(arg, "%d", &i); \
			if (i > 0) \
				obj->data.i = i + 1; \
			else \
				ERR(#name ": invalid length argument"); \
		}
	END OBJ_THREAD(mpd_artist, INFO_MPD)
		mpd_set_maxlen(mpd_artist);
		init_mpd();
	END OBJ_THREAD(mpd_title, INFO_MPD)
		mpd_set_maxlen(mpd_title);
		init_mpd();
	END OBJ_THREAD(mpd_random, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_repeat, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_elapsed, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_length, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_track, INFO_MPD)
		mpd_set_maxlen(mpd_track);
		init_mpd();
	END OBJ_THREAD(mpd_name, INFO_MPD)
		mpd_set_maxlen(mpd_name);
		init_mpd();
	END OBJ_THREAD(mpd_file, INFO_MPD)
		mpd_set_maxlen(mpd_file);
		init_mpd();
	END OBJ_THREAD(mpd_percent, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_album, INFO_MPD)
		mpd_set_maxlen(mpd_album);
		init_mpd();
	END OBJ_THREAD(mpd_vol, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_bitrate, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_status, INFO_MPD) init_mpd();
	END OBJ_THREAD(mpd_bar, INFO_MPD)
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
		init_mpd();
	END OBJ_THREAD(mpd_smart, INFO_MPD)
		mpd_set_maxlen(mpd_smart);
		init_mpd();
	END OBJ_THREAD(if_mpd_playing, INFO_MPD)
		obj_be_ifblock_if(obj);
		init_mpd();
#undef mpd_set_maxlen
#endif /* MPD */
#ifdef MOC
  END OBJ_THREAD(moc_state, INFO_MOC)
  END OBJ_THREAD(moc_file, INFO_MOC)
  END OBJ_THREAD(moc_title, INFO_MOC)
  END OBJ_THREAD(moc_artist, INFO_MOC)
  END OBJ_THREAD(moc_song, INFO_MOC)
  END OBJ_THREAD(moc_album, INFO_MOC)
  END OBJ_THREAD(moc_totaltime, INFO_MOC)
  END OBJ_THREAD(moc_timeleft, INFO_MOC)
  END OBJ_THREAD(moc_curtime, INFO_MOC)
  END OBJ_THREAD(moc_bitrate, INFO_MOC)
  END OBJ_THREAD(moc_rate, INFO_MOC)
#endif /* MOC */
#ifdef XMMS2
	END OBJ(xmms2_artist, INFO_XMMS2)
	END OBJ(xmms2_album, INFO_XMMS2)
	END OBJ(xmms2_title, INFO_XMMS2)
	END OBJ(xmms2_genre, INFO_XMMS2)
	END OBJ(xmms2_comment, INFO_XMMS2)
	END OBJ(xmms2_url, INFO_XMMS2)
	END OBJ(xmms2_tracknr, INFO_XMMS2)
	END OBJ(xmms2_bitrate, INFO_XMMS2)
	END OBJ(xmms2_date, INFO_XMMS2)
	END OBJ(xmms2_id, INFO_XMMS2)
	END OBJ(xmms2_duration, INFO_XMMS2)
	END OBJ(xmms2_elapsed, INFO_XMMS2)
	END OBJ(xmms2_size, INFO_XMMS2)
	END OBJ(xmms2_status, INFO_XMMS2)
	END OBJ(xmms2_percent, INFO_XMMS2)
	END OBJ(xmms2_bar, INFO_XMMS2)
		scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(xmms2_smart, INFO_XMMS2)
	END OBJ(xmms2_playlist, INFO_XMMS2)
	END OBJ(xmms2_timesplayed, INFO_XMMS2)
	END OBJ(if_xmms2_connected, INFO_XMMS2)
		obj_be_ifblock_if(obj);
#endif
#ifdef AUDACIOUS
	END OBJ(audacious_status, INFO_AUDACIOUS)
	END OBJ(audacious_title, INFO_AUDACIOUS)
		if (arg) {
			sscanf(arg, "%d", &info.audacious.max_title_len);
			if (info.audacious.max_title_len > 0) {
				info.audacious.max_title_len++;
			} else {
				CRIT_ERR("audacious_title: invalid length argument");
			}
		}
	END OBJ(audacious_length, INFO_AUDACIOUS)
	END OBJ(audacious_length_seconds, INFO_AUDACIOUS)
	END OBJ(audacious_position, INFO_AUDACIOUS)
	END OBJ(audacious_position_seconds, INFO_AUDACIOUS)
	END OBJ(audacious_bitrate, INFO_AUDACIOUS)
	END OBJ(audacious_frequency, INFO_AUDACIOUS)
	END OBJ(audacious_channels, INFO_AUDACIOUS)
	END OBJ(audacious_filename, INFO_AUDACIOUS)
	END OBJ(audacious_playlist_length, INFO_AUDACIOUS)
	END OBJ(audacious_playlist_position, INFO_AUDACIOUS)
	END OBJ(audacious_main_volume, INFO_AUDACIOUS)
	END OBJ(audacious_bar, INFO_AUDACIOUS)
		scan_bar(arg, &obj->a, &obj->b);
#endif
#ifdef BMPX
	END OBJ(bmpx_title, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_artist, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_album, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_track, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_uri, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END OBJ(bmpx_bitrate, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
#endif
#ifdef EVE
	END OBJ(eve, 0)
		if(arg) {
			int argc;
			char *userid = (char *) malloc(20 * sizeof(char));
			char *apikey = (char *) malloc(64 * sizeof(char));
			char *charid = (char *) malloc(20 * sizeof(char));

			argc = sscanf(arg, "%20s %64s %20s", userid, apikey, charid);
			obj->data.eve.charid = charid;
			obj->data.eve.userid = userid;
			obj->data.eve.apikey = apikey;

			init_eve();
		} else {
			CRIT_ERR("eve needs arguments: <userid> <apikey> <characterid>");
		}
#endif
#ifdef RSS
	END OBJ(rss, 0)
		if (arg) {
			int argc, delay, act_par;
			char *uri = (char *) malloc(128 * sizeof(char));
			char *action = (char *) malloc(64 * sizeof(char));

			argc = sscanf(arg, "%127s %d %63s %d", uri, &delay, action,
					&act_par);
			obj->data.rss.uri = uri;
			obj->data.rss.delay = delay;
			obj->data.rss.action = action;
			obj->data.rss.act_par = act_par;

			init_rss_info();
		} else {
			CRIT_ERR("rss needs arguments: <uri> <delay in minutes> <action> "
					"[act_par]");
		}
#endif
#ifdef HDDTEMP
	END OBJ(hddtemp, 0)
		if (scan_hddtemp(arg, &obj->data.hddtemp.dev,
		                 &obj->data.hddtemp.addr, &obj->data.hddtemp.port)) {
			ERR("hddtemp needs arguments");
			obj->type = OBJ_text;
			obj->data.s = strndup("${hddtemp}", text_buffer_size);
			obj->data.hddtemp.update_time = 0;
		} else
			obj->data.hddtemp.temp = NULL;
#endif
#ifdef TCP_PORT_MONITOR
	END OBJ(tcp_portmon, INFO_TCP_PORT_MONITOR)
		tcp_portmon_init(arg, &obj->data.tcp_port_monitor);
#endif
	END OBJ(entropy_avail, INFO_ENTROPY)
		END OBJ(entropy_poolsize, INFO_ENTROPY)
		END OBJ(entropy_bar, INFO_ENTROPY)
		scan_bar(arg, &obj->a, &obj->b);
	END OBJ(scroll, 0)
		int n1, n2;

		obj->data.scroll.step = 1;
		if (arg && sscanf(arg, "%u %n", &obj->data.scroll.show, &n1) > 0) {
			if (sscanf(arg + n1, "%u %n", &obj->data.scroll.step, &n2) > 0)
				n1 += n2;
			obj->data.scroll.text = strndup(arg + n1, text_buffer_size);
			obj->data.scroll.start = 0;
		} else {
			CRIT_ERR("scroll needs arguments: <length> [<step>] <text>");
		}
#ifdef NVIDIA
	END OBJ(nvidia, 0)
		if (!arg) {
			CRIT_ERR("nvidia needs an argument\n");
		} else if (set_nvidia_type(&obj->data.nvidia, arg)) {
			CRIT_ERR("nvidia: invalid argument"
				 " specified: '%s'\n", arg);
		}
#endif /* NVIDIA */
	END {
		char buf[256];

		ERR("unknown variable %s", s);
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strndup(buf, text_buffer_size);
	}
#undef OBJ

	return obj;
}

static struct text_object *create_plain_text(const char *s)
{
	struct text_object *obj;

	if (s == NULL || *s == '\0') {
		return NULL;
	}

	obj = new_text_object_internal();

	obj->type = OBJ_text;
	obj->data.s = strndup(s, text_buffer_size);
	return obj;
}

/* backslash_escape - do the actual substitution task for template objects
 *
 * The field templates is used for substituting the \N occurences. Set it to
 * NULL to leave them as they are.
 */
static char *backslash_escape(const char *src, char **templates, unsigned int template_count)
{
	char *src_dup;
	const char *p;
	unsigned int dup_idx = 0, dup_len;

	dup_len = strlen(src);
	src_dup = malloc(dup_len * sizeof(char));

	p = src;
	while (*p) {
		switch (*p) {
		case '\\':
			if (!*(p + 1))
				break;
			if (*(p + 1) == '\\') {
				src_dup[dup_idx++] = '\\';
				p++;
			} else if (*(p + 1) == ' ') {
				src_dup[dup_idx++] = ' ';
				p++;
			} else if (*(p + 1) == 'n') {
				src_dup[dup_idx++] = '\n';
				p++;
			} else if (templates) {
				unsigned int tmpl_num;
				int digits;
				if ((sscanf(p + 1, "%u%n", &tmpl_num, &digits) <= 0) ||
				    (tmpl_num > template_count))
					break;
				dup_len += strlen(templates[tmpl_num - 1]);
				src_dup = realloc(src_dup, dup_len * sizeof(char));
				sprintf(src_dup + dup_idx, "%s", templates[tmpl_num - 1]);
				dup_idx += strlen(templates[tmpl_num - 1]);
				p += digits;
			}
			break;
		default:
			src_dup[dup_idx++] = *p;
			break;
		}
		p++;
	}
	src_dup[dup_idx] = '\0';
	src_dup = realloc(src_dup, (strlen(src_dup) + 1) * sizeof(char));
	return src_dup;
}

/* handle_template_object - core logic of the template object
 *
 * use config variables like this:
 * template1 = "$\1\2"
 * template2 = "\1: ${fs_bar 4,100 \2} ${fs_used \2} / ${fs_size \2}"
 *
 * and use them like this:
 * ${template1 node name}
 * ${template2 root /}
 * ${template2 cdrom /mnt/cdrom}
 */
static char *handle_template(const char *tmpl, const char *args)
{
	char *args_dup, *p, *p_old;
	char **argsp = NULL;
	unsigned int argcnt = 0, template_idx, i;
	char *eval_text;

	if ((sscanf(tmpl, "template%u", &template_idx) != 1) ||
	    (template_idx > 9))
		return NULL;

	args_dup = strdup(args);
	p = args_dup;
	while (*p) {
		while (*p && (*p == ' ' && *(p - 1) != '\\'))
			p++;
		if (*(p - 1) == '\\')
			p--;
		p_old = p;
		while (*p && (*p != ' ' || *(p - 1) == '\\'))
			p++;
		if (*p) {
			(*p) = '\0';
			p++;
		}
		argsp = realloc(argsp, ++argcnt * sizeof(char *));
		argsp[argcnt - 1] = p_old;
	}
	for (i = 0; i < argcnt; i++) {
		char *tmp;
		tmp = backslash_escape(argsp[i], NULL, 0);
		DBGP2("%s: substituted arg '%s' to '%s'", tmpl, argsp[i], tmp);
		argsp[i] = tmp;
	}

	eval_text = backslash_escape(template[template_idx], argsp, argcnt);
	DBGP("substituted %s, output is '%s'", tmpl, eval_text);
	free(args_dup);
	for (i = 0; i < argcnt; i++)
		free(argsp[i]);
	free(argsp);
	return eval_text;
}

static char *find_and_replace_templates(const char *inbuf)
{
	char *outbuf, *indup, *p, *o, *templ, *args, *tmpl_out;
	int stack, outlen;

	outlen = strlen(inbuf) + 1;
	o = outbuf = calloc(outlen, sizeof(char));
	memset(outbuf, 0, outlen * sizeof(char));

	p = indup = strdup(inbuf);
	while (*p) {
		while (*p && *p != '$')
			*(o++) = *(p++);

		if (!(*p))
			break;

		if (strncmp(p, "$template", 9) && strncmp(p, "${template", 10)) {
			*(o++) = *(p++);
			continue;
		}

		if (*(p + 1) == '{') {
			templ = p + 2;
			while (*p != ' ')
				p++;
			*(p++) = '\0';
			args = p;
			stack = 1;
			while (stack > 0) {
				p++;
				if (*p == '{')
					stack++;
				else if (*p == '}')
					stack--;
			}
			*(p++) = '\0';
		} else {
			templ = p + 1;
			while (*p != ' ')
				p++;
			*(p++) = '\0';
			args = NULL;
		}
		tmpl_out = handle_template(templ, args);
		if (tmpl_out) {
			outlen += strlen(tmpl_out);
			outbuf = realloc(outbuf, outlen * sizeof(char));
			strcat (outbuf, tmpl_out);
			free(tmpl_out);
			o = outbuf + strlen(outbuf);
		} else {
			ERR("failed to handle template '%s' with args '%s'", templ, args);
		}
	}
	*o = '\0';
	outbuf = realloc(outbuf, (strlen(outbuf) + 1) * sizeof(char));
	free(indup);
	return outbuf;
}

static int text_contains_templates(const char *text)
{
	if (strcasestr(text, "${template") != NULL)
		return 1;
	if (strcasestr(text, "$template") != NULL)
		return 1;
	return 0;
}

static int extract_variable_text_internal(struct text_object *retval, const char *const_p, char allow_threaded)
{
	struct text_object *obj;
	char *p, *s, *orig_p;
	long line;

	p = strndup(const_p, max_user_text - 1);
	while (text_contains_templates(p)) {
		char *tmp;
		tmp = find_and_replace_templates(p);
		free(p);
		p = tmp;
	}
	s = orig_p = p;

	if (strcmp(p, const_p)) {
		DBGP("replaced all templates in text: input is\n'%s'\noutput is\n'%s'", const_p, p);
	} else {
		DBGP("no templates to replace");
	}

	memset(retval, 0, sizeof(struct text_object));

	line = global_text_lines;

	while (*p) {
		if (*p == '\n') {
			line++;
		}
		if (*p == '$') {
			*p = '\0';
			obj = create_plain_text(s);
			if (obj != NULL) {
				append_object(retval, obj);
			}
			*p = '$';
			p++;
			s = p;

			if (*p != '$') {
				char buf[256];
				const char *var;
				unsigned int len;

				/* variable is either $foo or ${foo} */
				if (*p == '{') {
					unsigned int brl = 1, brr = 0;

					p++;
					s = p;
					while (*p && brl != brr) {
						if (*p == '{') {
							brl++;
						}
						if (*p == '}') {
							brr++;
						}
						p++;
					}
					p--;
				} else {
					s = p;
					if (*p == '#') {
						p++;
					}
					while (*p && (isalnum((int) *p) || *p == '_')) {
						p++;
					}
				}

				/* copy variable to buffer */
				len = (p - s > 255) ? 255 : (p - s);
				strncpy(buf, s, len);
				buf[len] = '\0';

				if (*p == '}') {
					p++;
				}
				s = p;

				var = getenv(buf);

				/* if variable wasn't found in environment, use some special */
				if (!var) {
					char *tmp_p;
					char *arg = 0;

					/* split arg */
					if (strchr(buf, ' ')) {
						arg = strchr(buf, ' ');
						*arg = '\0';
						arg++;
						while (isspace((int) *arg)) {
							arg++;
						}
						if (!*arg) {
							arg = 0;
						}
					}

					/* lowercase variable name */
					tmp_p = buf;
					while (*tmp_p) {
						*tmp_p = tolower(*tmp_p);
						tmp_p++;
					}

					obj = construct_text_object(buf, arg, line, allow_threaded);
					if (obj != NULL) {
						append_object(retval, obj);
					}
				}
				continue;
			} else {
				obj = create_plain_text("$");
				if (obj != NULL) {
					append_object(retval, obj);
				}
			}
		}
		p++;
	}
	obj = create_plain_text(s);
	if (obj != NULL) {
		append_object(retval, obj);
	}

	if (!ifblock_stack_empty()) {
		ERR("one or more $endif's are missing");
	}

	free(orig_p);
	return 0;
}

static void extract_variable_text(const char *p)
{
	free_text_objects(&global_root_object);
	if (tmpstring1) {
		free(tmpstring1);
		tmpstring1 = 0;
	}
	if (tmpstring2) {
		free(tmpstring2);
		tmpstring2 = 0;
	}
	if (text_buffer) {
		free(text_buffer);
		text_buffer = 0;
	}

	extract_variable_text_internal(&global_root_object, p, 1);
}

int parse_conky_vars(struct text_object *root, char *txt, char *p, struct information *cur)
{
	extract_variable_text_internal(root, txt, 0);
	generate_text_internal(p, max_user_text, *root, cur);
	return 0;
}

/* Allows reading from a FIFO (i.e., /dev/xconsole).
 * The file descriptor is set to non-blocking which makes this possible.
 *
 * FIXME: Since lseek cannot seek a file descriptor long lines will break. */
static void tail_pipe(struct text_object *obj, char *dst, size_t dst_size)
{
#define TAIL_PIPE_BUFSIZE	4096
	int lines = 0;
	int line_len = 0;
	int last_line = 0;
	int fd = obj->data.tail.fd;

	while (1) {
		char buf[TAIL_PIPE_BUFSIZE];
		ssize_t len = read(fd, buf, sizeof(buf));
		int i;

		if (len == -1) {
			if (errno != EAGAIN) {
				strcpy(obj->data.tail.buffer, "Logfile Read Error");
				snprintf(dst, dst_size, "Logfile Read Error");
			}

			break;
		} else if (len == 0) {
			strcpy(obj->data.tail.buffer, "Logfile Empty");
			snprintf(dst, dst_size, "Logfile Empty");
			break;
		}

		for (line_len = 0, i = 0; i < len; i++) {
			int pos = 0;
			char *p;

			if (buf[i] == '\n') {
				lines++;

				if (obj->data.tail.readlines > 0) {
					int n;
					int olines = 0;
					int first_line = 0;

					for (n = 0; obj->data.tail.buffer[n]; n++) {
						if (obj->data.tail.buffer[n] == '\n') {
							if (!first_line) {
								first_line = n + 1;
							}

							if (++olines < obj->data.tail.wantedlines) {
								pos = n + 1;
								continue;
							}

							n++;
							p = obj->data.tail.buffer + first_line;
							pos = n - first_line;
							memmove(obj->data.tail.buffer,
									obj->data.tail.buffer + first_line, strlen(p));
							obj->data.tail.buffer[pos] = 0;
							break;
						}
					}
				}

				p = buf + last_line;
				line_len++;
				memcpy(&(obj->data.tail.buffer[pos]), p, line_len);
				obj->data.tail.buffer[pos + line_len] = 0;
				last_line = i + 1;
				line_len = 0;
				obj->data.tail.readlines = lines;
				continue;
			}

			line_len++;
		}
	}

	snprintf(dst, dst_size, "%s", obj->data.tail.buffer);
}

static inline struct mail_s *ensure_mail_thread(struct text_object *obj,
		void *thread(void *), const char *text)
{
	if (obj->global_mode && info.mail) {
		// this means we use info
		if (!info.mail->p_timed_thread) {
			info.mail->p_timed_thread =
				timed_thread_create(thread,
						(void *) info.mail, info.mail->interval * 1000000);
			if (!info.mail->p_timed_thread) {
				ERR("Error creating %s timed thread", text);
			}
			timed_thread_register(info.mail->p_timed_thread,
					&info.mail->p_timed_thread);
			if (timed_thread_run(info.mail->p_timed_thread)) {
				ERR("Error running %s timed thread", text);
			}
		}
		return info.mail;
	} else if (obj->data.mail) {
		// this means we use obj
		if (!obj->data.mail->p_timed_thread) {
			obj->data.mail->p_timed_thread =
				timed_thread_create(thread,
						(void *) obj->data.mail,
						obj->data.mail->interval * 1000000);
			if (!obj->data.mail->p_timed_thread) {
				ERR("Error creating %s timed thread", text);
			}
			timed_thread_register(obj->data.mail->p_timed_thread,
					&obj->data.mail->p_timed_thread);
			if (timed_thread_run(obj->data.mail->p_timed_thread)) {
				ERR("Error running %s timed thread", text);
			}
		}
		return obj->data.mail;
	} else if (!obj->a) {
		// something is wrong, warn once then stop
		ERR("Theres a problem with your %s_unseen settings.  "
				"Check that the global %s settings are defined "
				"properly (line %li).", global_text, global_text, obj->line);
		obj->a++;
	}
	return NULL;
}

char *format_time(unsigned long timeval, const int width)
{
	char buf[10];
	unsigned long nt;	// narrow time, for speed on 32-bit
	unsigned cc;		// centiseconds
	unsigned nn;		// multi-purpose whatever

	nt = timeval;
	cc = nt % 100;		// centiseconds past second
	nt /= 100;			// total seconds
	nn = nt % 60;		// seconds past the minute
	nt /= 60;			// total minutes
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u.%02u",
				nt, nn, cc)) {
		return strndup(buf, text_buffer_size);
	}
	if (width >= snprintf(buf, sizeof buf, "%lu:%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt % 60;		// minutes past the hour
	nt /= 60;			// total hours
	if (width >= snprintf(buf, sizeof buf, "%lu,%02u", nt, nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn = nt;			// now also hours
	if (width >= snprintf(buf, sizeof buf, "%uh", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 24;			// now days
	if (width >= snprintf(buf, sizeof buf, "%ud", nn)) {
		return strndup(buf, text_buffer_size);
	}
	nn /= 7;			// now weeks
	if (width >= snprintf(buf, sizeof buf, "%uw", nn)) {
		return strndup(buf, text_buffer_size);
	}
	// well shoot, this outta' fit...
	return strndup("<inf>", text_buffer_size);
}

//remove backspaced chars, example: "dog^H^H^Hcat" becomes "cat"
//string has to end with \0 and it's length should fit in a int
#define BACKSPACE 8
void remove_deleted_chars(char *string){
	int i = 0;
	while(string[i] != 0){
		if(string[i] == BACKSPACE){
			if(i != 0){
				strcpy( &(string[i-1]), &(string[i+1]) );
				i--;
			}else strcpy( &(string[i]), &(string[i+1]) ); //necessary for ^H's at the start of a string
		}else i++;
	}
}

static inline void format_media_player_time(char *buf, const int size,
		int seconds)
{
	int days, hours, minutes;

	days = seconds / (24 * 60 * 60);
	seconds %= (24 * 60 * 60);
	hours = seconds / (60 * 60);
	seconds %= (60 * 60);
	minutes = seconds / 60;
	seconds %= 60;

	if (days > 0) {
		snprintf(buf, size, "%i days %i:%02i:%02i", days,
				hours, minutes, seconds);
	} else if (hours > 0) {
		snprintf(buf, size, "%i:%02i:%02i", hours, minutes,
				seconds);
	} else {
		snprintf(buf, size, "%i:%02i", minutes, seconds);
	}
}

static inline double get_barnum(char *buf)
{
	char *c = buf;
	double barnum;

	while (*c) {
		if (*c == '\001') {
			*c = ' ';
		}
		c++;
	}

	if (sscanf(buf, "%lf", &barnum) == 0) {
		ERR("reading execbar value failed (perhaps it's not the "
				"correct format?)");
		return -1;
	}
	if (barnum > 100.0 || barnum < 0.0) {
		ERR("your execbar value is not between 0 and 100, "
				"therefore it will be ignored");
		return -1;
	}
	return barnum;
}

static void generate_text_internal(char *p, int p_max_size,
		struct text_object root, struct information *cur)
{
	struct text_object *obj;

#ifdef HAVE_ICONV
	char buff_in[p_max_size];
	buff_in[0] = 0;
	iconv_converting = 0;
#endif

	p[0] = 0;
	for (obj = root.next; obj && p_max_size > 0; obj = obj->next) {

/* IFBLOCK jumping algorithm
 *
 * This is easier as it looks like:
 * - each IF checks it's condition
 *   - on FALSE: call DO_JUMP
 *   - on TRUE: don't care
 * - each ELSE calls DO_JUMP unconditionally
 * - each ENDIF is silently being ignored
 *
 * Why this works:
 * DO_JUMP overwrites the "obj" variable of the loop and sets it to the target
 * (i.e. the corresponding ELSE or ENDIF). After that, processing for the given
 * object can continue, free()ing stuff e.g., then the for-loop does the rest: as
 * regularly, "obj" is being updated to point to obj->next, so object parsing
 * continues right after the corresponding ELSE or ENDIF. This means that if we
 * find an ELSE, it's corresponding IF must not have jumped, so we need to jump
 * always. If we encounter an ENDIF, it's corresponding IF or ELSE has not
 * jumped, and there is nothing to do.
 */
#define DO_JUMP { \
	DBGP2("jumping"); \
	obj = obj->data.ifblock.next; \
}

#define OBJ(a) break; case OBJ_##a:

		switch (obj->type) {
			default:
				ERR("not implemented obj type %d", obj->type);
#ifndef __OpenBSD__
			OBJ(acpitemp) {
				temp_print(p, p_max_size, get_acpi_temperature(obj->data.i), TEMP_CELSIUS);
			}
#endif /* !__OpenBSD__ */
			OBJ(freq) {
				if (obj->a) {
					obj->a = get_freq(p, p_max_size, "%.0f", 1,
							obj->data.cpu_index);
				}
			}
			OBJ(freq_g) {
				if (obj->a) {
#ifndef __OpenBSD__
					obj->a = get_freq(p, p_max_size, "%'.2f", 1000,
							obj->data.cpu_index);
#else
					/* OpenBSD has no such flag (SUSv2) */
					obj->a = get_freq(p, p_max_size, "%.2f", 1000,
							obj->data.cpu_index);
#endif
				}
			}
#if defined(__linux__)
			OBJ(voltage_mv) {
				if (obj->a) {
					obj->a = get_voltage(p, p_max_size, "%.0f", 1,
							obj->data.cpu_index);
				}
			}
			OBJ(voltage_v) {
				if (obj->a) {
					obj->a = get_voltage(p, p_max_size, "%'.3f", 1000,
							obj->data.cpu_index);
				}
			}

#ifdef HAVE_IWLIB
			OBJ(wireless_essid) {
				snprintf(p, p_max_size, "%s", obj->data.net->essid);
			}
			OBJ(wireless_mode) {
				snprintf(p, p_max_size, "%s", obj->data.net->mode);
			}
			OBJ(wireless_bitrate) {
				snprintf(p, p_max_size, "%s", obj->data.net->bitrate);
			}
			OBJ(wireless_ap) {
				snprintf(p, p_max_size, "%s", obj->data.net->ap);
			}
			OBJ(wireless_link_qual) {
				spaced_print(p, p_max_size, "%d", 4,
						obj->data.net->link_qual);
			}
			OBJ(wireless_link_qual_max) {
				spaced_print(p, p_max_size, "%d", 4,
						obj->data.net->link_qual_max);
			}
			OBJ(wireless_link_qual_perc) {
				if (obj->data.net->link_qual_max > 0) {
					spaced_print(p, p_max_size, "%.0f", 5,
							(double) obj->data.net->link_qual /
							obj->data.net->link_qual_max * 100);
				} else {
					spaced_print(p, p_max_size, "unk", 5);
				}
			}
			OBJ(wireless_link_bar) {
				new_bar(p, obj->a, obj->b, ((double) obj->data.net->link_qual /
							obj->data.net->link_qual_max) * 255.0);
			}
#endif /* HAVE_IWLIB */

#endif /* __linux__ */

#ifndef __OpenBSD__
			OBJ(adt746xcpu) {
				get_adt746x_cpu(p, p_max_size);
			}
			OBJ(adt746xfan) {
				get_adt746x_fan(p, p_max_size);
			}
			OBJ(acpifan) {
				get_acpi_fan(p, p_max_size);
			}
			OBJ(acpiacadapter) {
				get_acpi_ac_adapter(p, p_max_size);
			}
			OBJ(battery) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_STATUS);
			}
			OBJ(battery_time) {
				get_battery_stuff(p, p_max_size, obj->data.s, BATTERY_TIME);
			}
			OBJ(battery_percent) {
				spaced_print(p, p_max_size, "%*d", 4,
						pad_percents, get_battery_perct(obj->data.s));
			}
			OBJ(battery_bar) {
				new_bar(p, obj->a, obj->b, get_battery_perct_bar(obj->data.s));
			}
#endif /* __OpenBSD__ */

			OBJ(buffers) {
				human_readable(cur->buffers * 1024, p, 255);
			}
			OBJ(cached) {
				human_readable(cur->cached * 1024, p, 255);
			}
			OBJ(cpu) {
				if (obj->data.cpu_index > info.cpu_count) {
					printf("obj->data.cpu_index %i info.cpu_count %i",
							obj->data.cpu_index, info.cpu_count);
					CRIT_ERR("attempting to use more CPUs than you have!");
				}
				spaced_print(p, p_max_size, "%*d", 4, pad_percents,
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 100.0));
			}
			OBJ(cpubar) {
				new_bar(p, obj->a, obj->b,
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 255.0));
			}
			OBJ(cpugraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d, (unsigned int)
						round_to_int(cur->cpu_usage[obj->data.cpu_index] * 100),
						100, 1, obj->showaslog);
			}
			OBJ(loadgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d, cur->loadavg[0],
						obj->e, 1, obj->showaslog);
			}
			OBJ(color) {
				new_fg(p, obj->data.l);
			}
			OBJ(color0) {
				new_fg(p, color0);
			}
			OBJ(color1) {
				new_fg(p, color1);
			}
			OBJ(color2) {
				new_fg(p, color2);
			}
			OBJ(color3) {
				new_fg(p, color3);
			}
			OBJ(color4) {
				new_fg(p, color4);
			}
			OBJ(color5) {
				new_fg(p, color5);
			}
			OBJ(color6) {
				new_fg(p, color6);
			}
			OBJ(color7) {
				new_fg(p, color7);
			}
			OBJ(color8) {
				new_fg(p, color8);
			}
			OBJ(color9) {
				new_fg(p, color9);
			}
			OBJ(conky_version) {
				snprintf(p, p_max_size, "%s", VERSION);
			}
			OBJ(conky_build_date) {
				snprintf(p, p_max_size, "%s", BUILD_DATE);
			}
			OBJ(conky_build_arch) {
				snprintf(p, p_max_size, "%s", BUILD_ARCH);
			}
#if defined(__linux__)
			OBJ(disk_protect) {
				snprintf(p, p_max_size, "%s",
						get_disk_protect_queue(obj->data.s));
			}
			OBJ(i8k_version) {
				snprintf(p, p_max_size, "%s", i8k.version);
			}
			OBJ(i8k_bios) {
				snprintf(p, p_max_size, "%s", i8k.bios);
			}
			OBJ(i8k_serial) {
				snprintf(p, p_max_size, "%s", i8k.serial);
			}
			OBJ(i8k_cpu_temp) {
				int cpu_temp;

				sscanf(i8k.cpu_temp, "%d", &cpu_temp);
				temp_print(p, p_max_size, (double)cpu_temp, TEMP_CELSIUS);
			}
			OBJ(i8k_left_fan_status) {
				int left_fan_status;

				sscanf(i8k.left_fan_status, "%d", &left_fan_status);
				if (left_fan_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (left_fan_status == 1) {
					snprintf(p, p_max_size, "low");
				}
				if (left_fan_status == 2) {
					snprintf(p, p_max_size, "high");
				}
			}
			OBJ(i8k_right_fan_status) {
				int right_fan_status;

				sscanf(i8k.right_fan_status, "%d", &right_fan_status);
				if (right_fan_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (right_fan_status == 1) {
					snprintf(p, p_max_size, "low");
				}
				if (right_fan_status == 2) {
					snprintf(p, p_max_size, "high");
				}
			}
			OBJ(i8k_left_fan_rpm) {
				snprintf(p, p_max_size, "%s", i8k.left_fan_rpm);
			}
			OBJ(i8k_right_fan_rpm) {
				snprintf(p, p_max_size, "%s", i8k.right_fan_rpm);
			}
			OBJ(i8k_ac_status) {
				int ac_status;

				sscanf(i8k.ac_status, "%d", &ac_status);
				if (ac_status == -1) {
					snprintf(p, p_max_size, "disabled (read i8k docs)");
				}
				if (ac_status == 0) {
					snprintf(p, p_max_size, "off");
				}
				if (ac_status == 1) {
					snprintf(p, p_max_size, "on");
				}
			}
			OBJ(i8k_buttons_status) {
				snprintf(p, p_max_size, "%s", i8k.buttons_status);
			}
			OBJ(ibm_fan) {
				get_ibm_acpi_fan(p, p_max_size);
			}
			OBJ(ibm_temps) {
				get_ibm_acpi_temps();
				temp_print(p, p_max_size,
				           ibm_acpi.temps[obj->data.sensor], TEMP_CELSIUS);
			}
			OBJ(ibm_volume) {
				get_ibm_acpi_volume(p, p_max_size);
			}
			OBJ(ibm_brightness) {
				get_ibm_acpi_brightness(p, p_max_size);
			}
			OBJ(if_up) {
				if ((obj->data.ifblock.s)
						&& (!interface_up(obj->data.ifblock.s))) {
					DO_JUMP;
				}
			}
			OBJ(if_gw) {
				if (!cur->gw_info.count) {
					DO_JUMP;
				}
			}
			OBJ(gw_iface) {
				snprintf(p, p_max_size, "%s", cur->gw_info.iface);
			}
			OBJ(gw_ip) {
				snprintf(p, p_max_size, "%s", cur->gw_info.ip);
			}
			OBJ(laptop_mode) {
				snprintf(p, p_max_size, "%d", get_laptop_mode());
			}
			OBJ(pb_battery) {
				get_powerbook_batt_info(p, p_max_size, obj->data.i);
			}
#endif /* __linux__ */

#ifdef __OpenBSD__
			OBJ(obsd_sensors_temp) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				temp_print(p, p_max_size,
				           obsd_sensors.temp[obsd_sensors.device][obj->data.sensor],
					   TEMP_CELSIUS);
			}
			OBJ(obsd_sensors_fan) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				snprintf(p, p_max_size, "%d",
						obsd_sensors.fan[obsd_sensors.device][obj->data.sensor]);
			}
			OBJ(obsd_sensors_volt) {
				obsd_sensors.device = sensor_device;
				update_obsd_sensors();
				snprintf(p, p_max_size, "%.2f",
						obsd_sensors.volt[obsd_sensors.device][obj->data.sensor]);
			}
			OBJ(obsd_vendor) {
				get_obsd_vendor(p, p_max_size);
			}
			OBJ(obsd_product) {
				get_obsd_product(p, p_max_size);
			}
#endif /* __OpenBSD__ */

#ifdef X11
			OBJ(font) {
				new_font(p, obj->data.s);
			}
#endif
			/* TODO: move this correction from kB to kB/s elsewhere
			 * (or get rid of it??) */
			OBJ(diskio) {
				human_readable((obj->data.diskio->current / update_interval) * 1024LL,
						p, p_max_size);
			}
			OBJ(diskio_write) {
				human_readable((obj->data.diskio->current / update_interval) * 1024LL,
						p, p_max_size);
			}
			OBJ(diskio_read) {
				human_readable((obj->data.diskio->current / update_interval) * 1024LL,
						p, p_max_size);
			}
			OBJ(diskiograph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current, obj->e, 1, obj->showaslog);
			}
			OBJ(diskiograph_read) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current_read, obj->e, 1, obj->showaslog);
			}
			OBJ(diskiograph_write) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
				          obj->data.diskio->current_write, obj->e, 1, obj->showaslog);
			}
			OBJ(downspeed) {
				spaced_print(p, p_max_size, "%d", 6,
						round_to_int(obj->data.net->recv_speed / 1024));
			}
			OBJ(downspeedf) {
				spaced_print(p, p_max_size, "%.1f", 8,
						obj->data.net->recv_speed / 1024.0);
			}
			OBJ(downspeedgraph) {
			new_graph(p, obj->a, obj->b, obj->c, obj->d,
				obj->data.net->recv_speed / 1024.0, obj->e, 1, obj->showaslog);
			}
			OBJ(else) {
				/* Since we see you, you're if has not jumped.
				 * Do Ninja jump here: without leaving traces.
				 * This is to prevent us from stale jumped flags.
				 */
				obj = obj->data.ifblock.next;
				continue;
			}
			OBJ(endif) {
				/* harmless object, just ignore */
			}
#ifdef HAVE_POPEN
			OBJ(addr) {
				if ((obj->data.net->addr.sa_data[2] & 255) == 0
						&& (obj->data.net->addr.sa_data[3] & 255) == 0
						&& (obj->data.net->addr.sa_data[4] & 255) == 0
						&& (obj->data.net->addr.sa_data[5] & 255) == 0) {
					snprintf(p, p_max_size, "No Address");
				} else {
					snprintf(p, p_max_size, "%u.%u.%u.%u",
						obj->data.net->addr.sa_data[2] & 255,
						obj->data.net->addr.sa_data[3] & 255,
						obj->data.net->addr.sa_data[4] & 255,
						obj->data.net->addr.sa_data[5] & 255);
				}
			}
#if defined(__linux__)
			OBJ(addrs) {
				if(NULL != obj->data.net->addrs && strlen(obj->data.net->addrs) > 2)
				{
					obj->data.net->addrs[strlen(obj->data.net->addrs) - 2] = 0; /* remove ", " from end of string */
					strcpy(p, obj->data.net->addrs);
				}
				else
                                        strcpy(p, "0.0.0.0");
           }
#endif /* __linux__ */

#if defined(IMLIB2) && defined(X11)
			OBJ(image) {
				if (obj->a < 1) {
					obj->a++;
				} else {
					Imlib_Image image, buffer;

					image = imlib_load_image(obj->data.s);
					imlib_context_set_image(image);
					if (image) {
						int w, h;

						w = imlib_image_get_width();
						h = imlib_image_get_height();
						buffer = imlib_create_image(w, h);
						imlib_context_set_display(display);
						imlib_context_set_drawable(window.drawable);
						imlib_context_set_colormap(DefaultColormap(display,
							screen));
						imlib_context_set_visual(DefaultVisual(display,
							screen));
						imlib_context_set_image(buffer);
						imlib_blend_image_onto_image(image, 0, 0, 0, w, h,
							text_start_x, text_start_y, w, h);
						imlib_render_image_on_drawable(text_start_x,
							text_start_y);
						imlib_free_image();
						imlib_context_set_image(image);
						imlib_free_image();
					}
				}
			}
#endif /* IMLIB2 */

			OBJ(exec) {
				read_exec(obj->data.s, p, text_buffer_size);
				remove_deleted_chars(p);
			}
			OBJ(execp) {
				struct information *tmp_info;
				struct text_object subroot;

				read_exec(obj->data.s, p, text_buffer_size);

				tmp_info = malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				parse_conky_vars(&subroot, p, p, tmp_info);

				free_text_objects(&subroot);
				free(tmp_info);
			}
			OBJ(execbar) {
				double barnum;

				read_exec(obj->data.s, p, text_buffer_size);
				barnum = get_barnum(p);

				if (barnum >= 0.0) {
					barnum /= 100;
					new_bar(p, 0, 4, round_to_int(barnum * 255.0));
				}
			}
			OBJ(execgraph) {
				char showaslog = FALSE;
				double barnum;

				if(strncasecmp(obj->data.s, LOGGRAPH" ", strlen(LOGGRAPH" ")) == EQUAL) {
					showaslog = TRUE;
					read_exec(obj->data.s + strlen(LOGGRAPH" ") * sizeof(char), p, text_buffer_size);
				} else if(strncasecmp(obj->data.s, NORMGRAPH" ", strlen(NORMGRAPH" ")) == EQUAL) {
					read_exec(obj->data.s + strlen(NORMGRAPH" ") * sizeof(char), p, text_buffer_size);
				} else {
					read_exec(obj->data.s, p, text_buffer_size);
				}
				barnum = get_barnum(p);

				if (barnum >= 0.0) {
					new_graph(p, 0, 25, obj->c, obj->d, round_to_int(barnum),
						100, 1, showaslog);
				}
			}
			OBJ(execibar) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval) {
					double barnum;

					read_exec(obj->data.execi.cmd, p, text_buffer_size);
					barnum = get_barnum(p);

					if (barnum >= 0.0) {
						obj->f = 255 * barnum / 100.0;
					}
					obj->data.execi.last_update = current_update_time;
				}
				new_bar(p, 0, 4, round_to_int(obj->f));
			}
			OBJ(execigraph) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval) {
					double barnum;

					read_exec(obj->data.execi.cmd, p, text_buffer_size);
					barnum = get_barnum(p);

					if (barnum >= 0.0) {
						obj->f = barnum;
					}
					obj->data.execi.last_update = current_update_time;
				}
				new_graph(p, 0, 25, obj->c, obj->d, (int) (obj->f), 100, 1, FALSE);
			}
			OBJ(execi) {
				if (current_update_time - obj->data.execi.last_update
						>= obj->data.execi.interval
						&& obj->data.execi.interval != 0) {
					read_exec(obj->data.execi.cmd, obj->data.execi.buffer,
						text_buffer_size);
					obj->data.execi.last_update = current_update_time;
				}
				snprintf(p, text_buffer_size, "%s", obj->data.execi.buffer);
			}
			OBJ(execpi) {
				struct text_object subroot;
				struct information *tmp_info =
					malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));

				if (current_update_time - obj->data.execi.last_update
						< obj->data.execi.interval
						|| obj->data.execi.interval == 0) {
					parse_conky_vars(&subroot, obj->data.execi.buffer, p, tmp_info);
				} else {
					char *output = obj->data.execi.buffer;
					FILE *fp = popen(obj->data.execi.cmd, "r");
					int length = fread(output, 1, text_buffer_size, fp);

					pclose(fp);

					output[length] = '\0';
					if (length > 0 && output[length - 1] == '\n') {
						output[length - 1] = '\0';
					}

					parse_conky_vars(&subroot, obj->data.execi.buffer, p, tmp_info);
					obj->data.execi.last_update = current_update_time;
				}
				free_text_objects(&subroot);
				free(tmp_info);
			}
			OBJ(texeci) {
				if (!obj->data.texeci.p_timed_thread) {
					obj->data.texeci.p_timed_thread =
						timed_thread_create(&threaded_exec,
						(void *) obj, obj->data.texeci.interval * 1000000);
					if (!obj->data.texeci.p_timed_thread) {
						ERR("Error creating texeci timed thread");
					}
					timed_thread_register(obj->data.texeci.p_timed_thread,
						&obj->data.texeci.p_timed_thread);
					if (timed_thread_run(obj->data.texeci.p_timed_thread)) {
						ERR("Error running texeci timed thread");
					}
				}
				timed_thread_lock(obj->data.texeci.p_timed_thread);
				snprintf(p, text_buffer_size, "%s", obj->data.texeci.buffer);
				timed_thread_unlock(obj->data.texeci.p_timed_thread);
			}
#endif /* HAVE_POPEN */
			OBJ(imap_unseen) {
				struct mail_s *mail = ensure_mail_thread(obj, imap_thread, "imap");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->unseen);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(imap_messages) {
				struct mail_s *mail = ensure_mail_thread(obj, imap_thread, "imap");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->messages);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(pop3_unseen) {
				struct mail_s *mail = ensure_mail_thread(obj, pop3_thread, "pop3");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%lu", mail->unseen);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(pop3_used) {
				struct mail_s *mail = ensure_mail_thread(obj, pop3_thread, "pop3");

				if (mail && mail->p_timed_thread) {
					timed_thread_lock(mail->p_timed_thread);
					snprintf(p, p_max_size, "%.1f",
						mail->used / 1024.0 / 1024.0);
					timed_thread_unlock(mail->p_timed_thread);
				}
			}
			OBJ(fs_bar) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0) {
						new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h, 255);
					} else {
						new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h,
							(int) (255 - obj->data.fsbar.fs->avail * 255 /
							obj->data.fs->size));
					}
				}
			}
			OBJ(fs_free) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->avail, p, 255);
				}
			}
			OBJ(fs_free_perc) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size) {
						spaced_print(p, p_max_size, "%*d", 4,
								pad_percents, (int) ((obj->data.fs->avail * 100) /
									obj->data.fs->size));
					} else {
						snprintf(p, p_max_size, "0");
					}
				}
			}
			OBJ(fs_size) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->size, p, 255);
				}
			}
			OBJ(fs_type) {
				if (obj->data.fs != NULL)
					snprintf(p, p_max_size, "%s", obj->data.fs->type);
			}
			OBJ(fs_used) {
				if (obj->data.fs != NULL) {
					human_readable(obj->data.fs->size - (obj->data.fs->free
						? obj->data.fs->free : obj->data.fs->avail), p, 255);
				}
			}
			OBJ(fs_bar_free) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0) {
						new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h, 255);
					} else {
						new_bar(p, obj->data.fsbar.w, obj->data.fsbar.h,
							(int) (obj->data.fsbar.fs->avail * 255 /
							obj->data.fs->size));
					}
				}
			}
			OBJ(fs_used_perc) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size) {
						spaced_print(p, 4, "%*d", 4,
								pad_percents, 100 - ((int) ((obj->data.fs->avail * 100) /
										obj->data.fs->size)));
					} else {
						snprintf(p, p_max_size, "0");
					}
				}
			}
			OBJ(loadavg) {
				float *v = info.loadavg;

				if (obj->data.loadavg[2]) {
					snprintf(p, p_max_size, "%.2f %.2f %.2f",
						v[obj->data.loadavg[0] - 1],
						v[obj->data.loadavg[1] - 1],
						v[obj->data.loadavg[2] - 1]);
				} else if (obj->data.loadavg[1]) {
					snprintf(p, p_max_size, "%.2f %.2f",
						v[obj->data.loadavg[0] - 1],
						v[obj->data.loadavg[1] - 1]);
				} else if (obj->data.loadavg[0]) {
					snprintf(p, p_max_size, "%.2f",
						v[obj->data.loadavg[0] - 1]);
				}
			}
			OBJ(goto) {
				new_goto(p, obj->data.i);
			}
			OBJ(tab) {
				new_tab(p, obj->data.pair.a, obj->data.pair.b);
			}
			OBJ(hr) {
				new_hr(p, obj->data.i);
			}
			OBJ(nameserver) {
				if (cur->nameserver_info.nscount > obj->data.i)
					snprintf(p, p_max_size, "%s",
							cur->nameserver_info.ns_list[obj->data.i]);
			}
#ifdef EVE
			OBJ(eve) {
				char *skill = eve(obj->data.eve.userid, obj->data.eve.apikey, obj->data.eve.charid);
				snprintf(p, p_max_size, "%s", skill);
			}
#endif
#ifdef RSS
			OBJ(rss) {
				PRSS *data = get_rss_info(obj->data.rss.uri,
					obj->data.rss.delay);
				char *str;

				if (data == NULL) {
					snprintf(p, p_max_size, "prss: Error reading RSS data\n");
				} else {
					if (strcmp(obj->data.rss.action, "feed_title") == EQUAL) {
						str = data->title;
						// remove trailing new line if one exists
						if (str[strlen(str) - 1] == '\n') {
							str[strlen(str) - 1] = 0;
						}
						snprintf(p, p_max_size, "%s", str);
					} else if (strcmp(obj->data.rss.action, "item_title") == EQUAL) {
						if (obj->data.rss.act_par < data->item_count) {
							str = data->items[obj->data.rss.act_par].title;
							// remove trailing new line if one exists
							if (str[strlen(str) - 1] == '\n') {
								str[strlen(str) - 1] = 0;
							}
							snprintf(p, p_max_size, "%s", str);
						}
					} else if (strcmp(obj->data.rss.action, "item_desc") == EQUAL) {
						if (obj->data.rss.act_par < data->item_count) {
							str =
								data->items[obj->data.rss.act_par].description;
							// remove trailing new line if one exists
							if (str[strlen(str) - 1] == '\n') {
								str[strlen(str) - 1] = 0;
							}
							snprintf(p, p_max_size, "%s", str);
						}
					} else if (strcmp(obj->data.rss.action, "item_titles") == EQUAL) {
						if (data->item_count > 0) {
							int itmp;
							int show;

							p[0] = 0;

							if (obj->data.rss.act_par > data->item_count) {
								show = data->item_count;
							} else {
								show = obj->data.rss.act_par;
							}
							for (itmp = 0; itmp < show; itmp++) {
								PRSS_Item *item = &data->items[itmp];

								str = item->title;
								if (str) {
									// don't add new line before first item
									if (itmp > 0) {
										strncat(p, "\n", p_max_size);
									}
									/* remove trailing new line if one exists,
									 * we have our own */
									if (str[strlen(str) - 1] == '\n') {
										str[strlen(str) - 1] = 0;
									}
									strncat(p, str, p_max_size);
								}
							}
						}
					}
				}
			}
#endif
#ifdef HDDTEMP
			OBJ(hddtemp) {
				char *endptr, unit;
				long val;
				if (obj->data.hddtemp.update_time < current_update_time - 30) {
					if (obj->data.hddtemp.temp)
						free(obj->data.hddtemp.temp);
					obj->data.hddtemp.temp = get_hddtemp_info(obj->data.hddtemp.dev,
							obj->data.hddtemp.addr, obj->data.hddtemp.port);
					obj->data.hddtemp.update_time = current_update_time;
				}
				if (!obj->data.hddtemp.temp) {
					snprintf(p, p_max_size, "N/A");
				} else {
					val = strtol(obj->data.hddtemp.temp + 1, &endptr, 10);
					unit = obj->data.hddtemp.temp[0];

					if (*endptr != '\0')
						snprintf(p, p_max_size, "N/A");
					else if (unit == 'C')
						temp_print(p, p_max_size, (double)val, TEMP_CELSIUS);
					else if (unit == 'F')
						temp_print(p, p_max_size, (double)val, TEMP_FAHRENHEIT);
					else
						snprintf(p, p_max_size, "N/A");
				}
			}
#endif
			OBJ(offset) {
				new_offset(p, obj->data.i);
			}
			OBJ(voffset) {
				new_voffset(p, obj->data.i);
			}
#ifndef __OpenBSD__
			OBJ(i2c) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
			OBJ(platform) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
			OBJ(hwmon) {
				double r;

				r = get_sysfs_info(&obj->data.sysfs.fd, obj->data.sysfs.arg,
					obj->data.sysfs.devtype, obj->data.sysfs.type);

				if (!strncmp(obj->data.sysfs.type, "temp", 4)) {
					temp_print(p, p_max_size, r, TEMP_CELSIUS);
				} else if (r >= 100.0 || r == 0) {
					snprintf(p, p_max_size, "%d", (int) r);
				} else {
					snprintf(p, p_max_size, "%.1f", r);
				}
			}
#endif /* !__OpenBSD__ */
			OBJ(alignr) {
				new_alignr(p, obj->data.i);
			}
			OBJ(alignc) {
				new_alignc(p, obj->data.i);
			}
			OBJ(if_empty) {
				struct text_object subroot;
				struct information *tmp_info =
					malloc(sizeof(struct information));
				memcpy(tmp_info, cur, sizeof(struct information));
				parse_conky_vars(&subroot, obj->data.ifblock.s, p, tmp_info);

				if (strlen(p) != 0) {
					DO_JUMP;
				}
				p[0] = '\0';
				free_text_objects(&subroot);
				free(tmp_info);
			}
			OBJ(if_existing) {
				if (obj->data.ifblock.str
				    && !check_contains(obj->data.ifblock.s,
				                       obj->data.ifblock.str)) {
					DO_JUMP;
				} else if (obj->data.ifblock.s
				           && access(obj->data.ifblock.s, F_OK)) {
					DO_JUMP;
				}
			}
			OBJ(if_mounted) {
				if ((obj->data.ifblock.s)
						&& (!check_mount(obj->data.ifblock.s))) {
					DO_JUMP;
				}
			}
			OBJ(if_running) {
				if ((obj->data.ifblock.s) && system(obj->data.ifblock.s)) {
					DO_JUMP;
				}
			}
#if defined(__linux__)
			OBJ(ioscheduler) {
				snprintf(p, p_max_size, "%s", get_ioscheduler(obj->data.s));
			}
#endif
			OBJ(kernel) {
				snprintf(p, p_max_size, "%s", cur->uname_s.release);
			}
			OBJ(machine) {
				snprintf(p, p_max_size, "%s", cur->uname_s.machine);
			}

			/* memory stuff */
			OBJ(mem) {
				human_readable(cur->mem * 1024, p, 255);
			}
			OBJ(memeasyfree) {
				human_readable(cur->memeasyfree * 1024, p, 255);
			}
			OBJ(memfree) {
				human_readable(cur->memfree * 1024, p, 255);
			}
			OBJ(memmax) {
				human_readable(cur->memmax * 1024, p, 255);
			}
			OBJ(memperc) {
				if (cur->memmax) {
					spaced_print(p, p_max_size, "%*llu", 4,
						pad_percents, cur->mem * 100 / cur->memmax);
				}
			}
			OBJ(membar) {
				new_bar(p, obj->data.pair.a, obj->data.pair.b,
					cur->memmax ? (cur->mem * 255) / (cur->memmax) : 0);
			}
			OBJ(memgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					cur->memmax ? (cur->mem * 100.0) / (cur->memmax) : 0.0,
					100, 1, obj->showaslog);
			}

			/* mixer stuff */
			OBJ(mixer) {
				snprintf(p, p_max_size, "%d", mixer_get_avg(obj->data.l));
			}
			OBJ(mixerl) {
				snprintf(p, p_max_size, "%d", mixer_get_left(obj->data.l));
			}
			OBJ(mixerr) {
				snprintf(p, p_max_size, "%d", mixer_get_right(obj->data.l));
			}
			OBJ(mixerbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_get_avg(obj->data.mixerbar.l) * 255 / 100);
			}
			OBJ(mixerlbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_get_left(obj->data.mixerbar.l) * 255 / 100);
			}
			OBJ(mixerrbar) {
				new_bar(p, obj->data.mixerbar.w, obj->data.mixerbar.h,
					mixer_get_right(obj->data.mixerbar.l) * 255 / 100);
			}
#ifdef X11
			OBJ(monitor) {
				snprintf(p, p_max_size, "%d", cur->x11.monitor.current);
			}
			OBJ(monitor_number) {
				snprintf(p, p_max_size, "%d", cur->x11.monitor.number);
			}
#endif

			/* mail stuff */
			OBJ(mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d", obj->data.local_mail.mail_count);
			}
			OBJ(mboxscan) {
				mbox_scan(obj->data.mboxscan.args, obj->data.mboxscan.output,
					text_buffer_size);
				snprintf(p, p_max_size, "%s", obj->data.mboxscan.output);
			}
			OBJ(new_mails) {
				update_mail_count(&obj->data.local_mail);
				snprintf(p, p_max_size, "%d",
					obj->data.local_mail.new_mail_count);
			}
			OBJ(nodename) {
				snprintf(p, p_max_size, "%s", cur->uname_s.nodename);
			}
			OBJ(outlinecolor) {
				new_outline(p, obj->data.l);
			}
			OBJ(processes) {
				spaced_print(p, p_max_size, "%hu", 5, cur->procs);
			}
			OBJ(running_processes) {
				spaced_print(p, p_max_size, "%hu", 3, cur->run_procs);
			}
			OBJ(text) {
				snprintf(p, p_max_size, "%s", obj->data.s);
			}
			OBJ(shadecolor) {
				new_bg(p, obj->data.l);
			}
			OBJ(stippled_hr) {
				new_stippled_hr(p, obj->data.pair.a, obj->data.pair.b);
			}
			OBJ(swap) {
				human_readable(cur->swap * 1024, p, 255);
			}
			OBJ(swapmax) {
				human_readable(cur->swapmax * 1024, p, 255);
			}
			OBJ(swapperc) {
				if (cur->swapmax == 0) {
					strncpy(p, "No swap", p_max_size);
				} else {
					spaced_print(p, p_max_size, "%*llu", 4,
						pad_percents, cur->swap * 100 / cur->swapmax);
				}
			}
			OBJ(swapbar) {
				new_bar(p, obj->data.pair.a, obj->data.pair.b,
					cur->swapmax ? (cur->swap * 255) / (cur->swapmax) : 0);
			}
			OBJ(sysname) {
				snprintf(p, p_max_size, "%s", cur->uname_s.sysname);
			}
			OBJ(time) {
				time_t t = time(NULL);
				struct tm *tm = localtime(&t);

				setlocale(LC_TIME, "");
				strftime(p, p_max_size, obj->data.s, tm);
			}
			OBJ(utime) {
				time_t t = time(NULL);
				struct tm *tm = gmtime(&t);

				strftime(p, p_max_size, obj->data.s, tm);
			}
			OBJ(tztime) {
				char *oldTZ = NULL;
				time_t t;
				struct tm *tm;

				if (obj->data.tztime.tz) {
					oldTZ = getenv("TZ");
					setenv("TZ", obj->data.tztime.tz, 1);
					tzset();
				}
				t = time(NULL);
				tm = localtime(&t);

				setlocale(LC_TIME, "");
				strftime(p, p_max_size, obj->data.tztime.fmt, tm);
				if (oldTZ) {
					setenv("TZ", oldTZ, 1);
					tzset();
				} else {
					unsetenv("TZ");
				}
				// Needless to free oldTZ since getenv gives ptr to static data
			}
			OBJ(totaldown) {
				human_readable(obj->data.net->recv, p, 255);
			}
			OBJ(totalup) {
				human_readable(obj->data.net->trans, p, 255);
			}
			OBJ(updates) {
				snprintf(p, p_max_size, "%d", total_updates);
			}
			OBJ(upspeed) {
				spaced_print(p, p_max_size, "%d", 6,
					round_to_int(obj->data.net->trans_speed / 1024));
			}
			OBJ(upspeedf) {
				spaced_print(p, p_max_size, "%.1f", 8,
					obj->data.net->trans_speed / 1024.0);
			}
			OBJ(upspeedgraph) {
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					obj->data.net->trans_speed / 1024.0, obj->e, 1, obj->showaslog);
			}
			OBJ(uptime_short) {
				format_seconds_short(p, p_max_size, (int) cur->uptime);
			}
			OBJ(uptime) {
				format_seconds(p, p_max_size, (int) cur->uptime);
			}
			OBJ(user_names) {
				snprintf(p, p_max_size, "%s", cur->users.names);
			}
			OBJ(user_terms) {
				snprintf(p, p_max_size, "%s", cur->users.terms);
			}
			OBJ(user_times) {
				snprintf(p, p_max_size, "%s", cur->users.times);
			}
			OBJ(user_number) {
				snprintf(p, p_max_size, "%d", cur->users.number);
			}
#if (defined(__FreeBSD__) || defined(__FreeBSD_kernel__) \
		|| defined(__OpenBSD__)) && (defined(i386) || defined(__i386__))
			OBJ(apm_adapter) {
				char *msg;

				msg = get_apm_adapter();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_life) {
				char *msg;

				msg = get_apm_battery_life();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_time) {
				char *msg;

				msg = get_apm_battery_time();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
#endif /* __FreeBSD__ __OpenBSD__ */

#ifdef MPD
#define mpd_printf(fmt, val) \
	snprintf(p, p_max_size, fmt, mpd_get_info()->val)
#define mpd_sprintf(val) { \
	if (!obj->data.i || obj->data.i > p_max_size) \
		mpd_printf("%s", val); \
	else \
		snprintf(p, obj->data.i, "%s", mpd_get_info()->val); \
}
			OBJ(mpd_title)
				mpd_sprintf(title);
			OBJ(mpd_artist)
				mpd_sprintf(artist);
			OBJ(mpd_album)
				mpd_sprintf(album);
			OBJ(mpd_random)
				mpd_printf("%s", random);
			OBJ(mpd_repeat)
				mpd_printf("%s", repeat);
			OBJ(mpd_track)
				mpd_sprintf(track);
			OBJ(mpd_name)
				mpd_sprintf(name);
			OBJ(mpd_file)
				mpd_sprintf(file);
			OBJ(mpd_vol)
				mpd_printf("%d", volume);
			OBJ(mpd_bitrate)
				mpd_printf("%d", bitrate);
			OBJ(mpd_status)
				mpd_printf("%s", status);
			OBJ(mpd_elapsed) {
				format_media_player_time(p, p_max_size, mpd_get_info()->elapsed);
			}
			OBJ(mpd_length) {
				format_media_player_time(p, p_max_size, mpd_get_info()->length);
			}
			OBJ(mpd_percent) {
				spaced_print(p, p_max_size, "%*d", 4,
						pad_percents, (int) (mpd_get_info()->progress * 100));
			}
			OBJ(mpd_bar) {
				new_bar(p, obj->data.pair.a, obj->data.pair.b,
					(int) (mpd_get_info()->progress * 255.0f));
			}
			OBJ(mpd_smart) {
				struct mpd_s *mpd = mpd_get_info();
				int len = obj->data.i;
				if (len == 0 || len > p_max_size)
					len = p_max_size;

				memset(p, 0, p_max_size);
				if (mpd->artist && *mpd->artist &&
				    mpd->title && *mpd->title) {
					snprintf(p, len, "%s - %s", mpd->artist,
						mpd->title);
				} else if (mpd->title && *mpd->title) {
					snprintf(p, len, "%s", mpd->title);
				} else if (mpd->artist && *mpd->artist) {
					snprintf(p, len, "%s", mpd->artist);
				} else if (mpd->file && *mpd->file) {
					snprintf(p, len, "%s", mpd->file);
				} else {
					*p = 0;
				}
			}
			OBJ(if_mpd_playing) {
				if (!mpd_get_info()->is_playing) {
					DO_JUMP;
				}
			}
#undef mpd_sprintf
#undef mpd_printf
#endif

#ifdef MOC
			OBJ(moc_state) {
				snprintf(p, p_max_size, "%s", (cur->moc.state ? cur->moc.state : "??"));
			}
			OBJ(moc_file) {
				snprintf(p, p_max_size, "%s", (cur->moc.file ? cur->moc.file : "no file"));
			}
			OBJ(moc_title) {
				snprintf(p, p_max_size, "%s", (cur->moc.title ? cur->moc.title : "no title"));
			}
			OBJ(moc_artist) {
				snprintf(p, p_max_size, "%s", (cur->moc.artist ? cur->moc.artist : "no artist"));
			}
			OBJ(moc_song) {
				snprintf(p, p_max_size, "%s", (cur->moc.song ? cur->moc.song : "no song"));
			}
			OBJ(moc_album) {
				snprintf(p, p_max_size, "%s", (cur->moc.album ? cur->moc.album : "no album"));
			}
			OBJ(moc_totaltime) {
				snprintf(p, p_max_size, "%s", (cur->moc.totaltime ? cur->moc.totaltime : "0:00"));
			}
			OBJ(moc_timeleft) {
				snprintf(p, p_max_size, "%s", (cur->moc.timeleft ? cur->moc.timeleft : "0:00"));
			}
			OBJ(moc_curtime) {
				snprintf(p, p_max_size, "%s", (cur->moc.curtime ? cur->moc.curtime : "0:00"));
			}
			OBJ(moc_bitrate) {
				snprintf(p, p_max_size, "%s", (cur->moc.bitrate ? cur->moc.bitrate : "0Kbps"));
			}
			OBJ(moc_rate) {
        snprintf(p, p_max_size, "%s", (cur->moc.rate ? cur->moc.rate : "0KHz"));
			}
#endif /* MOC */
#ifdef XMMS2
			OBJ(xmms2_artist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.artist);
			}
			OBJ(xmms2_album) {
				snprintf(p, p_max_size, "%s", cur->xmms2.album);
			}
			OBJ(xmms2_title) {
				snprintf(p, p_max_size, "%s", cur->xmms2.title);
			}
			OBJ(xmms2_genre) {
				snprintf(p, p_max_size, "%s", cur->xmms2.genre);
			}
			OBJ(xmms2_comment) {
				snprintf(p, p_max_size, "%s", cur->xmms2.comment);
			}
			OBJ(xmms2_url) {
				snprintf(p, p_max_size, "%s", cur->xmms2.url);
			}
			OBJ(xmms2_status) {
				snprintf(p, p_max_size, "%s", cur->xmms2.status);
			}
			OBJ(xmms2_date) {
				snprintf(p, p_max_size, "%s", cur->xmms2.date);
			}
			OBJ(xmms2_tracknr) {
				if (cur->xmms2.tracknr != -1) {
					snprintf(p, p_max_size, "%i", cur->xmms2.tracknr);
				}
			}
			OBJ(xmms2_bitrate) {
				snprintf(p, p_max_size, "%i", cur->xmms2.bitrate);
			}
			OBJ(xmms2_id) {
				snprintf(p, p_max_size, "%u", cur->xmms2.id);
			}
			OBJ(xmms2_size) {
				snprintf(p, p_max_size, "%2.1f", cur->xmms2.size);
			}
			OBJ(xmms2_elapsed) {
				snprintf(p, p_max_size, "%02d:%02d", cur->xmms2.elapsed / 60000,
					(cur->xmms2.elapsed / 1000) % 60);
			}
			OBJ(xmms2_duration) {
				snprintf(p, p_max_size, "%02d:%02d",
					cur->xmms2.duration / 60000,
					(cur->xmms2.duration / 1000) % 60);
			}
			OBJ(xmms2_percent) {
				snprintf(p, p_max_size, "%2.0f", cur->xmms2.progress * 100);
			}
			OBJ(xmms2_bar) {
				new_bar(p, obj->data.pair.a, obj->data.pair.b,
					(int) (cur->xmms2.progress * 255.0f));
			}
			OBJ(xmms2_playlist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.playlist);
			}
			OBJ(xmms2_timesplayed) {
				snprintf(p, p_max_size, "%i", cur->xmms2.timesplayed);
			}
			OBJ(xmms2_smart) {
				if (strlen(cur->xmms2.title) < 2
						&& strlen(cur->xmms2.title) < 2) {
					snprintf(p, p_max_size, "%s", cur->xmms2.url);
				} else {
					snprintf(p, p_max_size, "%s - %s", cur->xmms2.artist,
						cur->xmms2.title);
				}
			}
			OBJ(if_xmms2_connected) {
				if (cur->xmms2_conn_state != 1) {
					DO_JUMP;
				}
			}
#endif
#ifdef AUDACIOUS
			OBJ(audacious_status) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_STATUS]);
			}
			OBJ(audacious_title) {
				snprintf(p, cur->audacious.max_title_len > 0
					? cur->audacious.max_title_len : p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_TITLE]);
			}
			OBJ(audacious_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH]);
			}
			OBJ(audacious_length_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
			}
			OBJ(audacious_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION]);
			}
			OBJ(audacious_position_seconds) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_POSITION_SECONDS]);
			}
			OBJ(audacious_bitrate) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_BITRATE]);
			}
			OBJ(audacious_frequency) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FREQUENCY]);
			}
			OBJ(audacious_channels) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_CHANNELS]);
			}
			OBJ(audacious_filename) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_FILENAME]);
			}
			OBJ(audacious_playlist_length) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_LENGTH]);
			}
			OBJ(audacious_playlist_position) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_PLAYLIST_POSITION]);
			}
			OBJ(audacious_main_volume) {
				snprintf(p, p_max_size, "%s",
					cur->audacious.items[AUDACIOUS_MAIN_VOLUME]);
			}
			OBJ(audacious_bar) {
				double progress;

				progress =
					atof(cur->audacious.items[AUDACIOUS_POSITION_SECONDS]) /
					atof(cur->audacious.items[AUDACIOUS_LENGTH_SECONDS]);
				new_bar(p, obj->a, obj->b, (int) (progress * 255.0f));
			}
#endif

#ifdef BMPX
			OBJ(bmpx_title) {
				snprintf(p, p_max_size, "%s", cur->bmpx.title);
			}
			OBJ(bmpx_artist) {
				snprintf(p, p_max_size, "%s", cur->bmpx.artist);
			}
			OBJ(bmpx_album) {
				snprintf(p, p_max_size, "%s", cur->bmpx.album);
			}
			OBJ(bmpx_uri) {
				snprintf(p, p_max_size, "%s", cur->bmpx.uri);
			}
			OBJ(bmpx_track) {
				snprintf(p, p_max_size, "%i", cur->bmpx.track);
			}
			OBJ(bmpx_bitrate) {
				snprintf(p, p_max_size, "%i", cur->bmpx.bitrate);
			}
#endif
			OBJ(top) {
				if (obj->data.top.num >= 0 && obj->data.top.num < 10) {
					char *timeval;

					switch (obj->data.top.type) {
						case TOP_NAME:
							snprintf(p, 16, "%-15s",
								cur->cpu[obj->data.top.num]->name);
							break;
						case TOP_CPU:
							snprintf(p, 7, "%6.2f",
								cur->cpu[obj->data.top.num]->amount);
							break;
						case TOP_PID:
							snprintf(p, 6, "%5i",
								cur->cpu[obj->data.top.num]->pid);
							break;
						case TOP_MEM:
							snprintf(p, 7, "%6.2f",
								cur->cpu[obj->data.top.num]->totalmem);
							break;
						case TOP_TIME:
							timeval = format_time(
								cur->cpu[obj->data.top.num]->total_cpu_time, 9);
							snprintf(p, 10, "%9s", timeval);
							free(timeval);
							break;
						case TOP_MEM_RES:
							human_readable(cur->cpu[obj->data.top.num]->rss,
									p, 255);
							break;
						case TOP_MEM_VSIZE:
							human_readable(cur->cpu[obj->data.top.num]->vsize,
									p, 255);
							break;
						default:
							ERR("Unhandled top data type: %d\n",
								obj->data.top.type);
					}
				} else {
					ERR("Top index < 0 or > 10: %d\n", obj->data.top.num);
				}
			}
			OBJ(top_mem) {
				if (obj->data.top.num >= 0 && obj->data.top.num < 10) {
					char *timeval;

					switch (obj->data.top.type) {
						case TOP_NAME:
							snprintf(p, 16, "%-15s",
								cur->memu[obj->data.top.num]->name);
							break;
						case TOP_CPU:
							snprintf(p, 7, "%6.2f",
								cur->memu[obj->data.top.num]->amount);
							break;
						case TOP_PID:
							snprintf(p, 6, "%5i",
								cur->memu[obj->data.top.num]->pid);
							break;
						case TOP_MEM:
							snprintf(p, 7, "%6.2f",
								cur->memu[obj->data.top.num]->totalmem);
							break;
						case TOP_TIME:
							timeval = format_time(
								cur->memu[obj->data.top.num]->total_cpu_time,
								9);
							snprintf(p, 10, "%9s", timeval);
							free(timeval);
							break;
						case TOP_MEM_RES:
							human_readable(cur->cpu[obj->data.top.num]->rss,
									p, 255);
							break;
						case TOP_MEM_VSIZE:
							human_readable(cur->cpu[obj->data.top.num]->vsize,
									p, 255);
							break;
						default:
							ERR("Unhandled top data type: %d\n",
								obj->data.top.type);
					}
				} else {
					ERR("Top index < 0 or > 10: %d\n", obj->data.top.num);
				}
			}
			OBJ(tail) {
				if (current_update_time - obj->data.tail.last_update
						< obj->data.tail.interval) {
					snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
				} else {
					FILE *fp;
					long nl = 0, bsize;
					int iter;

					obj->data.tail.last_update = current_update_time;

					if (obj->data.tail.fd != -1) {
						tail_pipe(obj, p, p_max_size);
						goto head;
					}

					fp = fopen(obj->data.tail.logfile, "rt");

					if (fp == NULL) {
						/* Send one message, but do not consistently spam
						 * on missing logfiles. */
						if (obj->data.tail.readlines != 0) {
							ERR("tail logfile failed to open");
							strcpy(obj->data.tail.buffer, "Logfile Missing");
						}
						obj->data.tail.readlines = 0;
						snprintf(p, p_max_size, "Logfile Missing");
					} else {
						obj->data.tail.readlines = 0;
						/* -1 instead of 0 to avoid counting a trailing
						 * newline */
						fseek(fp, -1, SEEK_END);
						bsize = ftell(fp) + 1;
						for (iter = obj->data.tail.wantedlines; iter > 0;
								iter--) {
							nl = rev_fcharfind(fp, '\n', iter);
							if (nl >= 0) {
								break;
							}
						}
						obj->data.tail.readlines = iter;
						if (obj->data.tail.readlines
								< obj->data.tail.wantedlines) {
							fseek(fp, 0, SEEK_SET);
						} else {
							fseek(fp, nl + 1, SEEK_SET);
							bsize -= ftell(fp);
						}
						/* Make sure bsize is at least 1 byte smaller than the
						 * buffer max size. */
						if (bsize > (long) ((text_buffer_size * 20) - 1)) {
							fseek(fp, bsize - text_buffer_size * 20 - 1,
								SEEK_CUR);
							bsize = text_buffer_size * 20 - 1;
						}
						bsize = fread(obj->data.tail.buffer, 1, bsize, fp);
						fclose(fp);
						if (bsize > 0) {
							/* Clean up trailing newline, make sure the
							 * buffer is null terminated. */
							if (obj->data.tail.buffer[bsize - 1] == '\n') {
								obj->data.tail.buffer[bsize - 1] = '\0';
							} else {
								obj->data.tail.buffer[bsize] = '\0';
							}
							snprintf(p, p_max_size, "%s",
								obj->data.tail.buffer);
						} else {
							strcpy(obj->data.tail.buffer, "Logfile Empty");
							snprintf(p, p_max_size, "Logfile Empty");
						}	/* bsize > 0 */
					}		/* fp == NULL */
				}			/* if cur_upd_time >= */
			}
head:
			OBJ(head) {
				if (current_update_time - obj->data.tail.last_update
						< obj->data.tail.interval) {
					snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
				} else {
					FILE *fp;
					long nl = 0;
					int iter;

					obj->data.tail.last_update = current_update_time;

					fp = fopen(obj->data.tail.logfile, "rt");
					if (fp == NULL) {
						/* Send one message, but do not consistently spam
						 * on missing logfiles. */
						if (obj->data.tail.readlines != 0) {
							ERR("head logfile failed to open");
							strcpy(obj->data.tail.buffer, "Logfile Missing");
						}
						obj->data.tail.readlines = 0;
						snprintf(p, p_max_size, "Logfile Missing");
					} else {
						obj->data.tail.readlines = 0;
						for (iter = obj->data.tail.wantedlines; iter > 0;
								iter--) {
							nl = fwd_fcharfind(fp, '\n', iter);
							if (nl >= 0) {
								break;
							}
						}
						obj->data.tail.readlines = iter;
						/* Make sure nl is at least 1 byte smaller than the
						 * buffer max size. */
						if (nl > (long) ((text_buffer_size * 20) - 1)) {
							nl = text_buffer_size * 20 - 1;
						}
						nl = fread(obj->data.tail.buffer, 1, nl, fp);
						fclose(fp);
						if (nl > 0) {
							/* Clean up trailing newline, make sure the buffer
							 * is null terminated. */
							if (obj->data.tail.buffer[nl - 1] == '\n') {
								obj->data.tail.buffer[nl - 1] = '\0';
							} else {
								obj->data.tail.buffer[nl] = '\0';
							}
							snprintf(p, p_max_size, "%s",
								obj->data.tail.buffer);
						} else {
							strcpy(obj->data.tail.buffer, "Logfile Empty");
							snprintf(p, p_max_size, "Logfile Empty");
						}	/* nl > 0 */
					}		/* if fp == null */
				}			/* cur_upd_time >= */
			}

			OBJ(lines) {
				FILE *fp = fopen(obj->data.s,"r");

				if(fp != NULL) {
					char buf[BUFSZ];
					int j, lines;

					lines = 0;
					while(fgets(buf, BUFSZ, fp) != NULL){
						for(j = 0; buf[j] != 0; j++) {
							if(buf[j] == '\n') {
								lines++;
							}
						}
					}
					sprintf(p, "%d", lines);
					fclose(fp);
				} else {
					sprintf(p, "File Unreadable");
				}
			}

			OBJ(words) {
				FILE *fp = fopen(obj->data.s,"r");

				if(fp != NULL) {
					char buf[BUFSZ];
					int j, words;
					char inword = FALSE;

					words = 0;
					while(fgets(buf, BUFSZ, fp) != NULL){
						for(j = 0; buf[j] != 0; j++) {
							if(!isspace(buf[j])) {
								if(inword == FALSE) {
									words++;
									inword = TRUE;
								}
							} else {
								inword = FALSE;
							}
						}
					}
					sprintf(p, "%d", words);
					fclose(fp);
				} else {
					sprintf(p, "File Unreadable");
				}
			}
#ifdef TCP_PORT_MONITOR
			OBJ(tcp_portmon) {
				tcp_portmon_action(p, p_max_size,
				                   &obj->data.tcp_port_monitor);
			}
#endif

#ifdef HAVE_ICONV
			OBJ(iconv_start) {
				iconv_converting = 1;
				iconv_selected = obj->a;
			}
			OBJ(iconv_stop) {
				iconv_converting = 0;
				iconv_selected = 0;
			}
#endif

			OBJ(entropy_avail) {
				snprintf(p, p_max_size, "%d", cur->entropy.entropy_avail);
			}
			OBJ(entropy_poolsize) {
				snprintf(p, p_max_size, "%d", cur->entropy.poolsize);
			}
			OBJ(entropy_bar) {
				double entropy_perc;

				entropy_perc = (double) cur->entropy.entropy_avail /
					(double) cur->entropy.poolsize;
				new_bar(p, obj->a, obj->b, (int) (entropy_perc * 255.0f));
			}
#ifdef SMAPI
			OBJ(smapi) {
				char *s;
				if(obj->data.s) {
					s = smapi_get_val(obj->data.s);
					snprintf(p, p_max_size, "%s", s);
					free(s);
				}
			}
			OBJ(if_smapi_bat_installed) {
				int idx;
				if(obj->data.ifblock.s && sscanf(obj->data.ifblock.s, "%i", &idx) == 1) {
					if(!smapi_bat_installed(idx)) {
						DO_JUMP;
					}
				} else
					ERR("argument to if_smapi_bat_installed must be an integer");
			}
			OBJ(smapi_bat_perc) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "remaining_percent") : 0;
					spaced_print(p, p_max_size, "%*d", 4, pad_percents, val);
				} else
					ERR("argument to smapi_bat_perc must be an integer");
			}
			OBJ(smapi_bat_temp) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "temperature") : 0;
					/* temperature is in milli degree celsius */
					temp_print(p, p_max_size, val / 1000, TEMP_CELSIUS);
				} else
					ERR("argument to smapi_bat_temp must be an integer");
			}
			OBJ(smapi_bat_power) {
				int idx, val;
				if(obj->data.s && sscanf(obj->data.s, "%i", &idx) == 1) {
					val = smapi_bat_installed(idx) ?
						smapi_get_bat_int(idx, "power_now") : 0;
					/* power_now is in mW, set to W with one digit precision */
					snprintf(p, p_max_size, "%.1f", ((double)val / 1000));
				} else
					ERR("argument to smapi_bat_power must be an integer");
			}
			OBJ(smapi_bat_bar) {
				if(obj->data.i >= 0 && smapi_bat_installed(obj->data.i))
					new_bar(p, obj->a, obj->b, (int)
							(255 * smapi_get_bat_int(obj->data.i, "remaining_percent") / 100));
				else
					new_bar(p, obj->a, obj->b, 0);
			}
#endif /* SMAPI */
			OBJ(scroll) {
				unsigned int j;
				char *tmp;
				struct text_object subroot;
				parse_conky_vars(&subroot, obj->data.scroll.text, p, cur);

				if (strlen(p) <= obj->data.scroll.show) {
					break;
				}
#define LINESEPARATOR '|'
				//place all the lines behind each other with LINESEPARATOR between them
				for(j = 0; p[j] != 0; j++) {
					if(p[j]=='\n') {
						p[j]=LINESEPARATOR;
					}
				}
				//scroll the output obj->data.scroll.start places by copying that many chars from
				//the front of the string to tmp, scrolling the rest to the front and placing tmp
				//at the back of the string
				tmp = calloc(obj->data.scroll.start + 1, sizeof(char));
				strncpy(tmp, p, obj->data.scroll.start); tmp[obj->data.scroll.start] = 0;
				for(j = obj->data.scroll.start; p[j] != 0; j++){
					p[j - obj->data.scroll.start] = p[j];
				}
				strcpy(&p[j - obj->data.scroll.start], tmp);
				free(tmp);
				//only show the requested number of chars
				if(obj->data.scroll.show < j) {
					p[obj->data.scroll.show] = 0;
				}
				//next time, scroll a place more or reset scrolling if we are at the end
				obj->data.scroll.start += obj->data.scroll.step;
				if(obj->data.scroll.start >= j){
					 obj->data.scroll.start = 0;
				}
			}
#ifdef NVIDIA
			OBJ(nvidia) {
				int value = get_nvidia_value(obj->data.nvidia.type, display);
				if(value == -1)
					snprintf(p, p_max_size, "N/A");
				else if (obj->data.nvidia.type == NV_TEMP)
					temp_print(p, p_max_size, (double)value, TEMP_CELSIUS);
				else if (obj->data.nvidia.print_as_float &&
						value > 0 && value < 100)
					snprintf(p, p_max_size, "%.1f", (float)value);
				else
					snprintf(p, p_max_size, "%d", value);
			}
#endif /* NVIDIA */

			break;
		}
#undef DO_JUMP


		{
			unsigned int a = strlen(p);

#ifdef HAVE_ICONV
			if (a > 0 && iconv_converting && iconv_selected > 0
					&& (iconv_cd[iconv_selected - 1] != (iconv_t) (-1))) {
				int bytes;
				size_t dummy1, dummy2;
				char *ptr = buff_in;
				char *outptr = p;

				dummy1 = dummy2 = a;

				strncpy(buff_in, p, p_max_size);

				iconv(*iconv_cd[iconv_selected - 1], NULL, NULL, NULL, NULL);
				while (dummy1 > 0) {
					bytes = iconv(*iconv_cd[iconv_selected - 1], &ptr, &dummy1,
							&outptr, &dummy2);
					if (bytes == -1) {
						ERR("Iconv codeset conversion failed");
						break;
					}
				}

				/* It is nessecary when we are converting from multibyte to
				 * singlebyte codepage */
				a = outptr - p;
			}
#endif /* HAVE_ICONV */
			p += a;
			p_max_size -= a;
		}
	}
}

double current_update_time, next_update_time, last_update_time;

static void generate_text(void)
{
	struct information *cur = &info;
	char *p;

	special_count = 0;

	/* update info */

	current_update_time = get_time();

	update_stuff();

	/* add things to the buffer */

	/* generate text */

	p = text_buffer;

	generate_text_internal(p, max_user_text, global_root_object, cur);

	if (stuff_in_upper_case) {
		char *tmp_p;

		tmp_p = text_buffer;
		while (*tmp_p) {
			*tmp_p = toupper(*tmp_p);
			tmp_p++;
		}
	}

	next_update_time += update_interval;
	if (next_update_time < get_time()) {
		next_update_time = get_time() + update_interval;
	} else if (next_update_time > get_time() + update_interval) {
		next_update_time = get_time() + update_interval;
	}
	last_update_time = current_update_time;
	total_updates++;
	// free(p);
}

#ifdef X11
static void set_font(void)
{
#ifdef XFT
	if (use_xft) {
		if (window.xftdraw != NULL) {
			XftDrawDestroy(window.xftdraw);
		}
		window.xftdraw = XftDrawCreate(display, window.drawable,
			DefaultVisual(display, screen), DefaultColormap(display, screen));
	} else
#endif
	{
		XSetFont(display, window.gc, fonts[selected_font].font->fid);
	}
}

#endif /* X11 */

static inline int get_string_width(const char *s)
{
#ifdef X11
	return *s ? calc_text_width(s, strlen(s)) : 0;
#else
	return strlen(s);
#endif /* X11 */
}

static inline int get_string_width_special(char *s)
{
#ifdef X11
	char *p, *final;
	int idx = 1;
	int width = 0;
	unsigned int i;

	if (!s) {
		return 0;
	}

	p = strndup(s, text_buffer_size);
	final = p;

	while (*p) {
		if (*p == SPECIAL_CHAR) {
			/* shift everything over by 1 so that the special char
			 * doesn't mess up the size calculation */
			for (i = 0; i < strlen(p); i++) {
				*(p + i) = *(p + i + 1);
			}
			if (specials[special_index + idx].type == GRAPH
					|| specials[special_index + idx].type == BAR) {
				width += specials[special_index + idx].width;
			}
			idx++;
		} else {
			p++;
		}
	}
	if (strlen(final) > 1) {
		width += calc_text_width(final, strlen(final));
	}
	free(final);
	return width;
#else
	return (s) ? strlen(s) : 0;
#endif /* X11 */
}

#ifdef X11
static void text_size_updater(char *s);

int last_font_height;
static void update_text_area(void)
{
	int x, y;

	/* update text size if it isn't fixed */
#ifdef OWN_WINDOW
	if (!fixed_size)
#endif
	{
		text_width = minimum_width;
		text_height = 0;
		special_index = 0;
		last_font_height = font_height();
		for_each_line(text_buffer, text_size_updater);
		text_width += 1;
		if (text_height < minimum_height) {
			text_height = minimum_height;
		}
		if (text_width > maximum_width && maximum_width > 0) {
			text_width = maximum_width;
		}
	}

	/* get text position on workarea */
	switch (text_alignment) {
		case TOP_LEFT:
			x = gap_x;
			y = gap_y;
			break;

		case TOP_RIGHT:
			x = workarea[2] - text_width - gap_x;
			y = gap_y;
			break;

		case TOP_MIDDLE:
			x = workarea[2] / 2 - text_width / 2 - gap_x;
			y = gap_y;
			break;

		default:
		case BOTTOM_LEFT:
			x = gap_x;
			y = workarea[3] - text_height - gap_y;
			break;

		case BOTTOM_RIGHT:
			x = workarea[2] - text_width - gap_x;
			y = workarea[3] - text_height - gap_y;
			break;

		case BOTTOM_MIDDLE:
			x = workarea[2] / 2 - text_width / 2 - gap_x;
			y = workarea[3] - text_height - gap_y;
			break;

		case MIDDLE_LEFT:
			x = gap_x;
			y = workarea[3] / 2 - text_height / 2 - gap_y;
			break;

		case MIDDLE_RIGHT:
			x = workarea[2] - text_width - gap_x;
			y = workarea[3] / 2 - text_height / 2 - gap_y;
			break;

#ifdef OWN_WINDOW
		case NONE:	// Let the WM manage the window
			x = window.x;
			y = window.y;

			fixed_pos = 1;
			fixed_size = 1;
			break;
#endif
	}
#ifdef OWN_WINDOW

	if (own_window && !fixed_pos) {
		x += workarea[0];
		y += workarea[1];
		text_start_x = border_margin + 1;
		text_start_y = border_margin + 1;
		window.x = x - border_margin - 1;
		window.y = y - border_margin - 1;
	} else
#endif
	{
		/* If window size doesn't match to workarea's size,
		 * then window probably includes panels (gnome).
		 * Blah, doesn't work on KDE. */
		if (workarea[2] != window.width || workarea[3] != window.height) {
			y += workarea[1];
			x += workarea[0];
		}

		text_start_x = x;
		text_start_y = y;
	}
}

/* drawing stuff */

static int cur_x, cur_y;	/* current x and y for drawing */
#endif
//draw_mode also without X11 because we only need to print to stdout with FG
static int draw_mode;		/* FG, BG or OUTLINE */
#ifdef X11
static long current_color;

#ifdef X11
static void text_size_updater(char *s)
{
	int w = 0;
	char *p;

	/* get string widths and skip specials */
	p = s;
	while (*p) {
		if (*p == SPECIAL_CHAR) {
			*p = '\0';
			w += get_string_width(s);
			*p = SPECIAL_CHAR;

			if (specials[special_index].type == BAR
					|| specials[special_index].type == GRAPH) {
				w += specials[special_index].width;
				if (specials[special_index].height > last_font_height) {
					last_font_height = specials[special_index].height;
					last_font_height += font_ascent();
				}
			} else if (specials[special_index].type == OFFSET) {
				if (specials[special_index].arg > 0) {
					w += specials[special_index].arg;
				}
			} else if (specials[special_index].type == VOFFSET) {
				last_font_height += specials[special_index].arg;
			} else if (specials[special_index].type == GOTO) {
				if (specials[special_index].arg > cur_x) {
					w = (int) specials[special_index].arg;
				}
			} else if (specials[special_index].type == TAB) {
				int start = specials[special_index].arg;
				int step = specials[special_index].width;

				if (!step || step < 0) {
					step = 10;
				}
				w += step - (cur_x - text_start_x - start) % step;
			} else if (specials[special_index].type == FONT) {
				selected_font = specials[special_index].font_added;
				if (font_height() > last_font_height) {
					last_font_height = font_height();
				}
			}

			special_index++;
			s = p + 1;
		}
		p++;
	}
	w += get_string_width(s);
	if (w > text_width) {
		text_width = w;
	}
	if (text_width > maximum_width && maximum_width) {
		text_width = maximum_width;
	}

	text_height += last_font_height;
	last_font_height = font_height();
}
#endif /* X11 */

static inline void set_foreground_color(long c)
{
	current_color = c;
	XSetForeground(display, window.gc, c);
}
#endif /* X11 */

static void draw_string(const char *s)
{
	int i, i2, pos, width_of_s;
	int max = 0;
	int added;

	if (s[0] == '\0') {
		return;
	}

	width_of_s = get_string_width(s);
	if ((output_methods & TO_STDOUT) && draw_mode == FG) {
		printf("%s\n", s);
		fflush(stdout);	/* output immediately, don't buffer */
	}
	memset(tmpstring1, 0, text_buffer_size);
	memset(tmpstring2, 0, text_buffer_size);
	strncpy(tmpstring1, s, text_buffer_size - 1);
	pos = 0;
	added = 0;

#ifdef X11
	max = ((text_width - width_of_s) / get_string_width(" "));
#endif /* X11 */
	/* This code looks for tabs in the text and coverts them to spaces.
	 * The trick is getting the correct number of spaces, and not going
	 * over the window's size without forcing the window larger. */
	for (i = 0; i < (int) text_buffer_size; i++) {
		if (tmpstring1[i] == '\t') {
			i2 = 0;
			for (i2 = 0; i2 < (8 - (1 + pos) % 8) && added <= max; i2++) {
				/* guard against overrun */
				tmpstring2[MIN(pos + i2, (int)text_buffer_size - 1)] = ' ';
				added++;
			}
			pos += i2;
		} else {
			/* guard against overrun */
			tmpstring2[MIN(pos, (int) text_buffer_size - 1)] = tmpstring1[i];
			pos++;
		}
	}
#ifdef X11
	if (text_width == maximum_width) {
		/* this means the text is probably pushing the limit,
		 * so we'll chop it */
		while (cur_x + get_string_width(tmpstring2) - text_start_x
				> maximum_width && strlen(tmpstring2) > 0) {
			tmpstring2[strlen(tmpstring2) - 1] = '\0';
		}
	}
#endif /* X11 */
	s = tmpstring2;
#ifdef X11
#ifdef XFT
	if (use_xft) {
		XColor c;
		XftColor c2;

		c.pixel = current_color;
		XQueryColor(display, DefaultColormap(display, screen), &c);

		c2.pixel = c.pixel;
		c2.color.red = c.red;
		c2.color.green = c.green;
		c2.color.blue = c.blue;
		c2.color.alpha = fonts[selected_font].font_alpha;
		if (utf8_mode) {
			XftDrawStringUtf8(window.xftdraw, &c2, fonts[selected_font].xftfont,
				cur_x, cur_y, (const XftChar8 *) s, strlen(s));
		} else {
			XftDrawString8(window.xftdraw, &c2, fonts[selected_font].xftfont,
				cur_x, cur_y, (const XftChar8 *) s, strlen(s));
		}
	} else
#endif
	{
		XDrawString(display, window.drawable, window.gc, cur_x, cur_y, s,
			strlen(s));
	}
	cur_x += width_of_s;
#endif /* X11 */
	memcpy(tmpstring1, s, text_buffer_size);
}

long redmask, greenmask, bluemask;

void set_up_gradient(void)
{
	int i;
#ifdef X11
	colour_depth = DisplayPlanes(display, screen);
#else
	colour_depth = 16;
#endif /* X11 */
	if (colour_depth != 24 && colour_depth != 16) {
		ERR("using non-standard colour depth, gradients may look like a "
			"lolly-pop");
	}

	redmask = 0;
	greenmask = 0;
	bluemask = 0;
	for (i = (colour_depth / 3) - 1; i >= 0; i--) {
		redmask |= 1 << i;
		greenmask |= 1 << i;
		bluemask |= 1 << i;
	}
	if (colour_depth % 3 == 1) {
		greenmask |= 1 << (colour_depth / 3);
	}
	redmask = redmask << (2 * colour_depth / 3 + colour_depth % 3);
	greenmask = greenmask << (colour_depth / 3);
}

/* this function returns the next colour between two colours for a gradient */
unsigned long do_gradient(unsigned long first_colour,
		unsigned long last_colour)
{
	int tmp_color = 0;
	int red1, green1, blue1;				// first colour
	int red2, green2, blue2;				// second colour
	int red3 = 0, green3 = 0, blue3 = 0;	// difference
	short redshift = (2 * colour_depth / 3 + colour_depth % 3);
	short greenshift = (colour_depth / 3);

	red1 = (first_colour & redmask) >> redshift;
	green1 = (first_colour & greenmask) >> greenshift;
	blue1 = first_colour & bluemask;
	red2 = (last_colour & redmask) >> redshift;
	green2 = (last_colour & greenmask) >> greenshift;
	blue2 = last_colour & bluemask;
	if (red1 > red2) {
		red3 = -1;
	}
	if (red1 < red2) {
		red3 = 1;
	}
	if (green1 > green2) {
		green3 = -1;
	}
	if (green1 < green2) {
		green3 = 1;
	}
	if (blue1 > blue2) {
		blue3 = -1;
	}
	if (blue1 < blue2) {
		blue3 = 1;
	}
	red1 += red3;
	green1 += green3;
	blue1 += blue3;
	if (red1 < 0) {
		red1 = 0;
	}
	if (green1 < 0) {
		green1 = 0;
	}
	if (blue1 < 0) {
		blue1 = 0;
	}
	if (red1 > bluemask) {
		red1 = bluemask;
	}
	if (green1 > bluemask) {
		green1 = bluemask;
	}
	if (blue1 > bluemask) {
		blue1 = bluemask;
	}
	tmp_color = (red1 << redshift) | (green1 << greenshift) | blue1;
	return tmp_color;
}

/* this function returns the max diff for a gradient */
unsigned long gradient_max(unsigned long first_colour,
		unsigned long last_colour)
{
	int red1, green1, blue1;				// first colour
	int red2, green2, blue2;				// second colour
	int red3 = 0, green3 = 0, blue3 = 0;			// difference
	long redshift, greenshift;
	int max;

	if (colour_depth == 0) {
		set_up_gradient();
	}
	redshift = (2 * colour_depth / 3 + colour_depth % 3);
	greenshift = (colour_depth / 3);

	red1 = (first_colour & redmask) >> redshift;
	green1 = (first_colour & greenmask) >> greenshift;
	blue1 = first_colour & bluemask;
	red2 = (last_colour & redmask) >> redshift;
	green2 = (last_colour & greenmask) >> greenshift;
	blue2 = last_colour & bluemask;
	red3 = abs(red1 - red2);
	green3 = abs(green1 - green2);
	blue3 = abs(blue1 - blue2);
	max = red3;

	if (green3 > max) {
		max = green3;
	}
	if (blue3 > max) {
		max = blue3;
	}
	return max;
}

static void draw_line(char *s)
{
#ifdef X11
	char *p;
	int cur_y_add = 0;
	short font_h;
	char *tmp_str;

	cur_x = text_start_x;
	cur_y += font_ascent();
	font_h = font_height();

	/* find specials and draw stuff */
	p = s;
	while (*p) {
		if (*p == SPECIAL_CHAR) {
			int w = 0;

			/* draw string before special */
			*p = '\0';
			draw_string(s);
			*p = SPECIAL_CHAR;
			s = p + 1;

			/* draw special */
			switch (specials[special_index].type) {
				case HORIZONTAL_LINE:
				{
					int h = specials[special_index].height;
					int mid = font_ascent() / 2;

					w = text_start_x + text_width - cur_x;

					XSetLineAttributes(display, window.gc, h, LineSolid,
						CapButt, JoinMiter);
					XDrawLine(display, window.drawable, window.gc, cur_x,
						cur_y - mid / 2, cur_x + w, cur_y - mid / 2);
					break;
				}

				case STIPPLED_HR:
				{
					int h = specials[special_index].height;
					int tmp_s = specials[special_index].arg;
					int mid = font_ascent() / 2;
					char ss[2] = { tmp_s, tmp_s };

					w = text_start_x + text_width - cur_x - 1;
					XSetLineAttributes(display, window.gc, h, LineOnOffDash,
						CapButt, JoinMiter);
					XSetDashes(display, window.gc, 0, ss, 2);
					XDrawLine(display, window.drawable, window.gc, cur_x,
						cur_y - mid / 2, cur_x + w, cur_y - mid / 2);
					break;
				}

				case BAR:
				{
					int h, bar_usage, by;
					if (cur_x - text_start_x > maximum_width
							&& maximum_width > 0) {
						break;
					}
					h = specials[special_index].height;
					bar_usage = specials[special_index].arg;
					by = cur_y - (font_ascent() / 2) - 1;

					if (h < font_height()) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0) {
						w = text_start_x + text_width - cur_x - 1;
					}
					if (w < 0) {
						w = 0;
					}

					XSetLineAttributes(display, window.gc, 1, LineSolid,
						CapButt, JoinMiter);

					XDrawRectangle(display, window.drawable, window.gc, cur_x,
						by, w, h);
					XFillRectangle(display, window.drawable, window.gc, cur_x,
						by, w * bar_usage / 255, h);
					if (specials[special_index].height > cur_y_add
							&& specials[special_index].height > font_h) {
						cur_y_add = specials[special_index].height;
					}
					break;
				}

				case GRAPH:
				{
					int h, by, i, j = 0;
					int gradient_size = 0;
					float gradient_factor = 0;
					float gradient_update = 0;
					unsigned long last_colour = current_color;
					unsigned long tmpcolour = current_color;
					if (cur_x - text_start_x > maximum_width
							&& maximum_width > 0) {
						break;
					}
					h = specials[special_index].height;
					by = cur_y - (font_ascent() / 2) - 1;

					if (h < font_height()) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0) {
						w = text_start_x + text_width - cur_x - 1;
					}
					if (w < 0) {
						w = 0;
					}
					if (draw_graph_borders) {
						XSetLineAttributes(display, window.gc, 1, LineSolid,
							CapButt, JoinMiter);
						XDrawRectangle(display, window.drawable, window.gc,
							cur_x, by, w, h);
					}
					XSetLineAttributes(display, window.gc, 1, LineSolid,
						CapButt, JoinMiter);

					if (specials[special_index].last_colour != 0
							|| specials[special_index].first_colour != 0) {
						tmpcolour = specials[special_index].last_colour;
						gradient_size =
							gradient_max(specials[special_index].last_colour,
							specials[special_index].first_colour);
						gradient_factor = (float) gradient_size / (w - 2);
					}
					for (i = w - 2; i > -1; i--) {
						if (specials[special_index].last_colour != 0
								|| specials[special_index].first_colour != 0) {
							XSetForeground(display, window.gc, tmpcolour);
							gradient_update += gradient_factor;
							while (gradient_update > 0) {
								tmpcolour = do_gradient(tmpcolour,
									specials[special_index].first_colour);
								gradient_update--;
							}
						}
						if ((w - i) / ((float) (w - 2) /
								(specials[special_index].graph_width)) > j
								&& j < MAX_GRAPH_DEPTH - 3) {
							j++;
						}
						/* this is mugfugly, but it works */
						XDrawLine(display, window.drawable, window.gc,
							cur_x + i + 1, by + h, cur_x + i + 1,
							by + h - specials[special_index].graph[j] *
							(h - 1) / specials[special_index].graph_scale);
					}
					if (specials[special_index].height > cur_y_add
							&& specials[special_index].height > font_h) {
						cur_y_add = specials[special_index].height;
					}
					/* if (draw_mode == BG) {
						set_foreground_color(default_bg_color);
					} else if (draw_mode == OUTLINE) {
						set_foreground_color(default_out_color);
					} else {
						set_foreground_color(default_fg_color);
					} */
					if (show_graph_range) {
						int tmp_x = cur_x;
						int tmp_y = cur_y;
						unsigned short int seconds = update_interval * w;
						char *tmp_day_str;
						char *tmp_hour_str;
						char *tmp_min_str;
						char *tmp_sec_str;
						unsigned short int timeunits;
						if(seconds!=0){
							timeunits = seconds / 86400; seconds %= 86400;
							if( timeunits > 0 ) {
								asprintf(&tmp_day_str, "%dd", timeunits);
							}else{
								tmp_day_str = strdup("");
							}
							timeunits = seconds / 3600; seconds %= 3600;
							if( timeunits > 0 ) {
								asprintf(&tmp_hour_str, "%dh", timeunits);
							}else{
								tmp_hour_str = strdup("");
							}
							timeunits = seconds / 60; seconds %= 60;
							if(timeunits > 0) {
								asprintf(&tmp_min_str, "%dm", timeunits);
							}else{
								tmp_min_str = strdup("");
							}
							if(seconds > 0) {
								asprintf(&tmp_sec_str, "%ds", seconds);
							}else{
								tmp_sec_str = strdup("");
							}
							asprintf(&tmp_str, "%s%s%s%s", tmp_day_str, tmp_hour_str, tmp_min_str, tmp_sec_str);
							free(tmp_day_str); free(tmp_hour_str); free(tmp_min_str); free(tmp_sec_str);
						}else{
							asprintf(&tmp_str, "Range not possible"); //should never happen, but better safe then sorry
						}
						cur_x += (w / 2) - (font_ascent() * (strlen(tmp_str) / 2));
						cur_y += font_height() / 2;
						draw_string(tmp_str);
						free(tmp_str);
						cur_x = tmp_x;
						cur_y = tmp_y;
					}
#ifdef MATH
					if (show_graph_scale && (specials[special_index].show_scale == 1)) {
						int tmp_x = cur_x;
						int tmp_y = cur_y;
						cur_x += font_ascent() / 2;
						cur_y += font_height() / 2;
						tmp_str = (char *)
							calloc(log10(floor(specials[special_index].graph_scale)) + 4,
									sizeof(char));
						sprintf(tmp_str, "%.1f", specials[special_index].graph_scale);
						draw_string(tmp_str);
						free(tmp_str);
						cur_x = tmp_x;
						cur_y = tmp_y;
					}
#endif
					set_foreground_color(last_colour);
					break;
				}

				case FONT:
				{
					int old = font_ascent();

					cur_y -= font_ascent();
					selected_font = specials[special_index].font_added;
					if (cur_y + font_ascent() < cur_y + old) {
						cur_y += old;
					} else {
						cur_y += font_ascent();
					}
					set_font();
					break;
				}
				case FG:
					if (draw_mode == FG) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

				case BG:
					if (draw_mode == BG) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

				case OUTLINE:
					if (draw_mode == OUTLINE) {
						set_foreground_color(specials[special_index].arg);
					}
					break;

				case OFFSET:
					w += specials[special_index].arg;
					break;

				case VOFFSET:
					cur_y += specials[special_index].arg;
					break;

				case GOTO:
					if (specials[special_index].arg >= 0) {
						cur_x = (int) specials[special_index].arg;
					}
					break;

				case TAB:
				{
					int start = specials[special_index].arg;
					int step = specials[special_index].width;

					if (!step || step < 0) {
						step = 10;
					}
					w = step - (cur_x - text_start_x - start) % step;
					break;
				}

				case ALIGNR:
				{
					/* TODO: add back in "+ border_margin" to the end of
					 * this line? */
					int pos_x = text_start_x + text_width -
						get_string_width_special(s);

					/* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
						"get_string_width(p) %i gap_x %i "
						"specials[special_index].arg %i border_margin %i "
						"border_width %i\n", pos_x, text_start_x, text_width,
						cur_x, get_string_width_special(s), gap_x,
						specials[special_index].arg, border_margin,
						border_width); */
					if (pos_x > specials[special_index].arg && pos_x > cur_x) {
						cur_x = pos_x - specials[special_index].arg;
					}
					break;
				}

				case ALIGNC:
				{
					int pos_x = (text_width) / 2 - get_string_width_special(s) /
						2 - (cur_x - text_start_x);
					/* int pos_x = text_start_x + text_width / 2 -
						get_string_width_special(s) / 2; */

					/* printf("pos_x %i text_start_x %i text_width %i cur_x %i "
						"get_string_width(p) %i gap_x %i "
						"specials[special_index].arg %i\n", pos_x, text_start_x,
						text_width, cur_x, get_string_width(s), gap_x,
						specials[special_index].arg); */
					if (pos_x > specials[special_index].arg) {
						w = pos_x - specials[special_index].arg;
					}
					break;
				}
			}

			cur_x += w;

			special_index++;
		}

		p++;
	}
#else
	draw_string(s);
#endif
#ifdef X11
	if (cur_y_add > 0) {
		cur_y += cur_y_add;
		cur_y -= font_descent();
	}

	draw_string(s);

	cur_y += font_descent();
#endif /* X11 */
}

static void draw_text(void)
{
#ifdef X11
	cur_y = text_start_y;

	/* draw borders */
	if (draw_borders && border_width > 0) {
		unsigned int b = (border_width + 1) / 2;

		if (stippled_borders) {
			char ss[2] = { stippled_borders, stippled_borders };
			XSetLineAttributes(display, window.gc, border_width, LineOnOffDash,
				CapButt, JoinMiter);
			XSetDashes(display, window.gc, 0, ss, 2);
		} else {
			XSetLineAttributes(display, window.gc, border_width, LineSolid,
				CapButt, JoinMiter);
		}

		XDrawRectangle(display, window.drawable, window.gc,
			text_start_x - border_margin + b, text_start_y - border_margin + b,
			text_width + border_margin * 2 - 1 - b * 2,
			text_height + border_margin * 2 - 1 - b * 2);
	}

	/* draw text */
	special_index = 0;
#endif /* X11 */
	for_each_line(text_buffer, draw_line);
}

static void draw_stuff(void)
{
#ifdef X11
	selected_font = 0;
	if (draw_shades && !draw_outline) {
		text_start_x++;
		text_start_y++;
		set_foreground_color(default_bg_color);
		draw_mode = BG;
		draw_text();
		text_start_x--;
		text_start_y--;
	}

	if (draw_outline) {
		int i, j;
		selected_font = 0;

		for (i = -1; i < 2; i++) {
			for (j = -1; j < 2; j++) {
				if (i == 0 && j == 0) {
					continue;
				}
				text_start_x += i;
				text_start_y += j;
				set_foreground_color(default_out_color);
				draw_mode = OUTLINE;
				draw_text();
				text_start_x -= i;
				text_start_y -= j;
			}
		}
	}

	set_foreground_color(default_fg_color);
#endif /* X11 */
	draw_mode = FG;
	draw_text();
#ifdef X11
#ifdef HAVE_XDBE
	if (use_xdbe) {
		XdbeSwapInfo swap;

		swap.swap_window = window.window;
		swap.swap_action = XdbeBackground;
		XdbeSwapBuffers(display, &swap, 1);
	}
#endif
#endif /* X11 */
}

#ifdef X11
static void clear_text(int exposures)
{
#ifdef HAVE_XDBE
	if (use_xdbe) {
		/* The swap action is XdbeBackground, which clears */
		return;
	} else
#endif
	{
		/* there is some extra space for borders and outlines */
		XClearArea(display, window.window, text_start_x - border_margin - 1,
			text_start_y - border_margin - 1,
			text_width + border_margin * 2 + 2,
			text_height + border_margin * 2 + 2, exposures ? True : 0);
	}
}
#endif /* X11 */

static int need_to_update;

/* update_text() generates new text and clears old text area */
static void update_text(void)
{
	generate_text();
#ifdef X11
	clear_text(1);
#endif /* X11 */
	need_to_update = 1;
}

static void main_loop(void)
{
#ifdef SIGNAL_BLOCKING
	sigset_t newmask, oldmask;
#endif
	double t;
#ifdef X11
	Region region = XCreateRegion();

#ifdef HAVE_XDAMAGE
	Damage damage;
	XserverRegion region2, part;
	int event_base, error_base;

	if (!XDamageQueryExtension(display, &event_base, &error_base)) {
		ERR("Xdamage extension unavailable");
	}
	damage = XDamageCreate(display, window.window, XDamageReportNonEmpty);
	region2 = XFixesCreateRegionFromWindow(display, window.window, 0);
	part = XFixesCreateRegionFromWindow(display, window.window, 0);
#endif /* HAVE_XDAMAGE */
#endif /* X11 */

#ifdef SIGNAL_BLOCKING
	sigemptyset(&newmask);
	sigaddset(&newmask, SIGINT);
	sigaddset(&newmask, SIGTERM);
	sigaddset(&newmask, SIGUSR1);
#endif

	next_update_time = last_update_time = get_time();
	info.looped = 0;
	while (total_run_times == 0 || info.looped < total_run_times) {
		info.looped++;

#ifdef SIGNAL_BLOCKING
		/* block signals.  we will inspect for pending signals later */
		if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
			CRIT_ERR("unable to sigprocmask()");
		}
#endif

#ifdef X11
		XFlush(display);

		/* wait for X event or timeout */

		if (!XPending(display)) {
			fd_set fdsr;
			struct timeval tv;
			int s;
			t = next_update_time - get_time();

			if (t < 0) {
				t = 0;
			} else if (t > update_interval) {
				t = update_interval;
			}

			tv.tv_sec = (long) t;
			tv.tv_usec = (long) (t * 1000000) % 1000000;
			FD_ZERO(&fdsr);
			FD_SET(ConnectionNumber(display), &fdsr);

			s = select(ConnectionNumber(display) + 1, &fdsr, 0, 0, &tv);
			if (s == -1) {
				if (errno != EINTR) {
					ERR("can't select(): %s", strerror(errno));
				}
			} else {
				/* timeout */
				if (s == 0) {
#else
		t = (next_update_time - get_time()) * 1000000;
		usleep((useconds_t)t);
#endif /* X11 */
					update_text();
#ifdef X11
				}
			}
		}

		if (need_to_update) {
#ifdef OWN_WINDOW
			int wx = window.x, wy = window.y;
#endif

			need_to_update = 0;
			selected_font = 0;
			update_text_area();
#ifdef OWN_WINDOW
			if (own_window) {
				/* resize window if it isn't right size */
				if (!fixed_size
						&& (text_width + border_margin * 2 + 1 != window.width
						|| text_height + border_margin * 2 + 1 != window.height)) {
					window.width = text_width + border_margin * 2 + 1;
					window.height = text_height + border_margin * 2 + 1;
					XResizeWindow(display, window.window, window.width,
						window.height);
					if (own_window) {
						set_transparent_background(window.window);
					}
				}

				/* move window if it isn't in right position */
				if (!fixed_pos && (window.x != wx || window.y != wy)) {
					XMoveWindow(display, window.window, window.x, window.y);
				}
			}
#endif

			clear_text(1);

#ifdef HAVE_XDBE
			if (use_xdbe) {
				XRectangle r;

				r.x = text_start_x - border_margin;
				r.y = text_start_y - border_margin;
				r.width = text_width + border_margin * 2;
				r.height = text_height + border_margin * 2;
				XUnionRectWithRegion(&r, region, region);
			}
#endif
		}

		/* handle X events */
		while (XPending(display)) {
			XEvent ev;

			XNextEvent(display, &ev);
			switch (ev.type) {
				case Expose:
				{
					XRectangle r;

					r.x = ev.xexpose.x;
					r.y = ev.xexpose.y;
					r.width = ev.xexpose.width;
					r.height = ev.xexpose.height;
					XUnionRectWithRegion(&r, region, region);
					break;
				}

#ifdef OWN_WINDOW
				case ReparentNotify:
					/* set background to ParentRelative for all parents */
					if (own_window) {
						set_transparent_background(window.window);
					}
					break;

				case ConfigureNotify:
					if (own_window) {
						/* if window size isn't what expected, set fixed size */
						if (ev.xconfigure.width != window.width
								|| ev.xconfigure.height != window.height) {
							if (window.width != 0 && window.height != 0) {
								fixed_size = 1;
							}

							/* clear old stuff before screwing up
							 * size and pos */
							clear_text(1);

							{
								XWindowAttributes attrs;

								if (XGetWindowAttributes(display,
										window.window, &attrs)) {
									window.width = attrs.width;
									window.height = attrs.height;
								}
							}

							text_width = window.width - border_margin * 2 - 1;
							text_height = window.height - border_margin * 2 - 1;
							if (text_width > maximum_width
									&& maximum_width > 0) {
								text_width = maximum_width;
							}
						}

						/* if position isn't what expected, set fixed pos
						 * total_updates avoids setting fixed_pos when window
						 * is set to weird locations when started */
						/* // this is broken
						if (total_updates >= 2 && !fixed_pos
								&& (window.x != ev.xconfigure.x
								|| window.y != ev.xconfigure.y)
								&& (ev.xconfigure.x != 0
								|| ev.xconfigure.y != 0)) {
							fixed_pos = 1;
						} */
						set_font();
					}
					break;

				case ButtonPress:
					if (own_window) {
						/* if an ordinary window with decorations */
						if ((window.type == TYPE_NORMAL)
								&& (!TEST_HINT(window.hints,
								HINT_UNDECORATED))) {
							/* allow conky to hold input focus. */
							break;
						} else {
							/* forward the click to the desktop window */
							XUngrabPointer(display, ev.xbutton.time);
							ev.xbutton.window = window.desktop;
							XSendEvent(display, ev.xbutton.window, False,
								ButtonPressMask, &ev);
							XSetInputFocus(display, ev.xbutton.window,
								RevertToParent, ev.xbutton.time);
						}
					}
					break;

				case ButtonRelease:
					if (own_window) {
						/* if an ordinary window with decorations */
						if ((window.type == TYPE_NORMAL)
								&& (!TEST_HINT(window.hints,
								HINT_UNDECORATED))) {
							/* allow conky to hold input focus. */
							break;
						} else {
							/* forward the release to the desktop window */
							ev.xbutton.window = window.desktop;
							XSendEvent(display, ev.xbutton.window, False,
								ButtonReleaseMask, &ev);
						}
					}
					break;

#endif

				default:
#ifdef HAVE_XDAMAGE
					if (ev.type == event_base + XDamageNotify) {
						XDamageNotifyEvent *dev = (XDamageNotifyEvent *) &ev;

						XFixesSetRegion(display, part, &dev->area, 1);
						XFixesUnionRegion(display, region2, region2, part);
					}
#endif /* HAVE_XDAMAGE */
					break;
			}
		}

#ifdef HAVE_XDAMAGE
		XDamageSubtract(display, damage, region2, None);
		XFixesSetRegion(display, region2, 0, 0);
#endif /* HAVE_XDAMAGE */

		/* XDBE doesn't seem to provide a way to clear the back buffer without
		 * interfering with the front buffer, other than passing XdbeBackground
		 * to XdbeSwapBuffers. That means that if we're using XDBE, we need to
		 * redraw the text even if it wasn't part of the exposed area. OTOH,
		 * if we're not going to call draw_stuff at all, then no swap happens
		 * and we can safely do nothing. */

		if (!XEmptyRegion(region)) {
#ifdef HAVE_XDBE
			if (use_xdbe) {
				XRectangle r;

				r.x = text_start_x - border_margin;
				r.y = text_start_y - border_margin;
				r.width = text_width + border_margin * 2;
				r.height = text_height + border_margin * 2;
				XUnionRectWithRegion(&r, region, region);
			}
#endif
			XSetRegion(display, window.gc, region);
#ifdef XFT
			if (use_xft) {
				XftDrawSetClip(window.xftdraw, region);
			}
#endif
#endif /* X11 */
			draw_stuff();
#ifdef X11
			XDestroyRegion(region);
			region = XCreateRegion();
		}
#endif /* X11 */

#ifdef SIGNAL_BLOCKING
		/* unblock signals of interest and let handler fly */
		if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
			CRIT_ERR("unable to sigprocmask()");
		}
#endif

		switch (g_signal_pending) {
			case SIGHUP:
			case SIGUSR1:
				ERR("received SIGHUP or SIGUSR1. reloading the config file.");
				reload_config();
				break;
			case SIGINT:
			case SIGTERM:
				ERR("received SIGINT or SIGTERM to terminate. bye!");
				clean_up();
#ifdef X11
				XDestroyRegion(region);
				region = NULL;
#ifdef HAVE_XDAMAGE
				XDamageDestroy(display, damage);
				XFixesDestroyRegion(display, region2);
				XFixesDestroyRegion(display, part);
#endif /* HAVE_XDAMAGE */
#endif /* X11 */
				return;	/* return from main_loop */
				/* break; */
			default:
				/* Reaching here means someone set a signal
				 * (SIGXXXX, signal_handler), but didn't write any code
				 * to deal with it.
				 * If you don't want to handle a signal, don't set a handler on
				 * it in the first place. */
				if (g_signal_pending) {
					ERR("ignoring signal (%d)", g_signal_pending);
				}
				break;
		}
		g_signal_pending = 0;
	}

#if defined(X11) && defined(HAVE_XDAMAGE)
	XDamageDestroy(display, damage);
	XFixesDestroyRegion(display, region2);
	XFixesDestroyRegion(display, part);
	XDestroyRegion(region);
	region = NULL;
#endif /* X11 && HAVE_XDAMAGE */
}

static void load_config_file(const char *);

/* reload the config file */
static void reload_config(void)
{
	timed_thread_destroy_registered_threads();

	if (info.cpu_usage) {
		free(info.cpu_usage);
		info.cpu_usage = NULL;
	}

	if (info.mail) {
		free(info.mail);
	}

#ifdef MOC
  free_moc(&info.moc);
#endif

#ifdef X11
	free_fonts();
#endif /* X11 */

#ifdef TCP_PORT_MONITOR
	tcp_portmon_clear();
#endif

	if (current_config) {
		clear_fs_stats();
		load_config_file(current_config);

		/* re-init specials array */
		if ((specials = realloc((void *) specials,
				sizeof(struct special_t) * max_specials)) == 0) {
			ERR("failed to realloc specials array");
		}

#ifdef X11
		load_fonts();
		set_font();
		// clear the window first
		XClearWindow(display, RootWindow(display, screen));

#endif /* X11 */
		extract_variable_text(global_text);
		free(global_text);
		global_text = NULL;
		if (tmpstring1) {
			free(tmpstring1);
		}
		tmpstring1 = malloc(text_buffer_size);
		memset(tmpstring1, 0, text_buffer_size);
		if (tmpstring2) {
			free(tmpstring2);
		}
		tmpstring2 = malloc(text_buffer_size);
		memset(tmpstring2, 0, text_buffer_size);
		if (text_buffer) {
			free(text_buffer);
		}
		text_buffer = malloc(max_user_text);
		memset(text_buffer, 0, max_user_text);
		update_text();
	}
}

static void clean_up(void)
{
	timed_thread_destroy_registered_threads();

	if (info.cpu_usage) {
		free(info.cpu_usage);
		info.cpu_usage = NULL;
	}
#ifdef X11
#ifdef HAVE_XDBE
	if (use_xdbe) {
		XdbeDeallocateBackBufferName(display, window.back_buffer);
	}
#endif
#ifdef OWN_WINDOW
	if (own_window) {
		XDestroyWindow(display, window.window);
		XClearWindow(display, RootWindow(display, screen));
		XFlush(display);
	} else
#endif
	{
		XClearWindow(display, RootWindow(display, screen));
		clear_text(1);
		XFlush(display);
	}

	XFreeGC(display, window.gc);
	free_fonts();
#endif /* X11 */

	free_text_objects(&global_root_object);
	if (tmpstring1) {
		free(tmpstring1);
		tmpstring1 = 0;
	}
	if (tmpstring2) {
		free(tmpstring2);
		tmpstring2 = 0;
	}
	if (text_buffer) {
		free(text_buffer);
		text_buffer = 0;
	}

	if (global_text) {
		free(global_text);
		global_text = 0;
	}

	free(current_config);

#ifdef TCP_PORT_MONITOR
	tcp_portmon_clear();
#endif
#ifdef RSS
	free_rss_info();
#endif

	if (specials) {
		unsigned int i;

		for (i = 0; i < special_count; i++) {
			if (specials[i].type == GRAPH) {
				free(specials[i].graph);
			}
		}
		free(specials);
		specials = NULL;
	}

	clear_diskio_stats();
}

static int string_to_bool(const char *s)
{
	if (!s) {
		// Assumes an option without a true/false means true
		return 1;
	} else if (strcasecmp(s, "yes") == EQUAL) {
		return 1;
	} else if (strcasecmp(s, "true") == EQUAL) {
		return 1;
	} else if (strcasecmp(s, "1") == EQUAL) {
		return 1;
	}
	return 0;
}

#ifdef X11
static enum alignment string_to_alignment(const char *s)
{
	if (strcasecmp(s, "top_left") == EQUAL) {
		return TOP_LEFT;
	} else if (strcasecmp(s, "top_right") == EQUAL) {
		return TOP_RIGHT;
	} else if (strcasecmp(s, "top_middle") == EQUAL) {
		return TOP_MIDDLE;
	} else if (strcasecmp(s, "bottom_left") == EQUAL) {
		return BOTTOM_LEFT;
	} else if (strcasecmp(s, "bottom_right") == EQUAL) {
		return BOTTOM_RIGHT;
	} else if (strcasecmp(s, "bottom_middle") == EQUAL) {
		return BOTTOM_MIDDLE;
	} else if (strcasecmp(s, "middle_left") == EQUAL) {
		return MIDDLE_LEFT;
	} else if (strcasecmp(s, "middle_right") == EQUAL) {
		return MIDDLE_RIGHT;
	} else if (strcasecmp(s, "tl") == EQUAL) {
		return TOP_LEFT;
	} else if (strcasecmp(s, "tr") == EQUAL) {
		return TOP_RIGHT;
	} else if (strcasecmp(s, "tm") == EQUAL) {
		return TOP_MIDDLE;
	} else if (strcasecmp(s, "bl") == EQUAL) {
		return BOTTOM_LEFT;
	} else if (strcasecmp(s, "br") == EQUAL) {
		return BOTTOM_RIGHT;
	} else if (strcasecmp(s, "bm") == EQUAL) {
		return BOTTOM_MIDDLE;
	} else if (strcasecmp(s, "ml") == EQUAL) {
		return MIDDLE_LEFT;
	} else if (strcasecmp(s, "mr") == EQUAL) {
		return MIDDLE_RIGHT;
	} else if (strcasecmp(s, "none") == EQUAL) {
		return NONE;
	}
	return TOP_LEFT;
}
#endif /* X11 */

static void set_default_configurations(void)
{
	fork_to_background = 0;
	total_run_times = 0;
	info.cpu_avg_samples = 2;
	info.net_avg_samples = 2;
	info.memmax = 0;
	top_cpu = 0;
	cpu_separate = 0;
	short_units = 0;
	top_mem = 0;
#ifdef MPD
	mpd_set_host("localhost");
	mpd_set_port("6600");
#endif
#ifdef MOC
  init_moc(&info.moc);
#endif
#ifdef XMMS2
	info.xmms2.artist = NULL;
	info.xmms2.album = NULL;
	info.xmms2.title = NULL;
	info.xmms2.genre = NULL;
	info.xmms2.comment = NULL;
	info.xmms2.url = NULL;
	info.xmms2.status = NULL;
	info.xmms2.playlist = NULL;
#endif
	use_spacer = NO_SPACER;
#ifdef X11
	output_methods = TO_X;
#else
	output_methods = TO_STDOUT;
#endif
#ifdef X11
	show_graph_scale = 0;
	show_graph_range = 0;
	default_fg_color = WhitePixel(display, screen);
	default_bg_color = BlackPixel(display, screen);
	default_out_color = BlackPixel(display, screen);
	color0 = default_fg_color;
	color1 = default_fg_color;
	color2 = default_fg_color;
	color3 = default_fg_color;
	color4 = default_fg_color;
	color5 = default_fg_color;
	color6 = default_fg_color;
	color7 = default_fg_color;
	color8 = default_fg_color;
	color9 = default_fg_color;
	template[0] = strdup("");
	template[1] = strdup("");
	template[2] = strdup("");
	template[3] = strdup("");
	template[4] = strdup("");
	template[5] = strdup("");
	template[6] = strdup("");
	template[7] = strdup("");
	template[8] = strdup("");
	template[9] = strdup("");
	draw_shades = 1;
	draw_borders = 0;
	draw_graph_borders = 1;
	draw_outline = 0;
	set_first_font("6x10");
	gap_x = 5;
	gap_y = 60;
	minimum_width = 5;
	minimum_height = 5;
	maximum_width = 0;
#ifdef OWN_WINDOW
	own_window = 0;
	window.type = TYPE_NORMAL;
	window.hints = 0;
	strcpy(window.class_name, PACKAGE_NAME);
	update_uname();
	sprintf(window.title, PACKAGE_NAME" (%s)", info.uname_s.nodename);
#endif
	stippled_borders = 0;
	border_margin = 3;
	border_width = 1;
	text_alignment = BOTTOM_LEFT;
	info.x11.monitor.number = 1;
	info.x11.monitor.current = 0;
#endif /* X11 */

	free(current_mail_spool);
	{
		char buf[256];

		variable_substitute(MAIL_FILE, buf, 256);
		if (buf[0] != '\0') {
			current_mail_spool = strndup(buf, text_buffer_size);
		}
	}

	no_buffers = 1;
	update_interval = 3.0;
	info.music_player_interval = 1.0;
	stuff_in_upper_case = 0;
	info.users.number = 1;

#ifdef TCP_PORT_MONITOR
	/* set default connection limit */
	tcp_portmon_set_max_connections(0);
#endif
}

static void load_config_file(const char *f)
{
#define CONF_ERR ERR("%s: %d: config file error", f, line)
	int line = 0;
	FILE *fp;

	set_default_configurations();
#ifdef CONFIG_OUTPUT
	if (!strcmp(f, "==builtin==")) {
#ifdef HAVE_FOPENCOOKIE
		fp = fopencookie(NULL, "r", conf_cookie);
#endif
	} else
#endif /* CONFIG_OUTPUT */
		fp = fopen(f, "r");

	if (!fp) {
		return;
	}
	DBGP("reading contents from config file '%s'", f);

	while (!feof(fp)) {
		char buf[256], *p, *p2, *name, *value;

		line++;
		if (fgets(buf, 256, fp) == NULL) {
			break;
		}

		p = buf;

		/* break at comment */
		p2 = strchr(p, '#');
		if (p2) {
			*p2 = '\0';
		}

		/* skip spaces */
		while (*p && isspace((int) *p)) {
			p++;
		}
		if (*p == '\0') {
			continue;	/* empty line */
		}

		name = p;

		/* skip name */
		p2 = p;
		while (*p2 && !isspace((int) *p2)) {
			p2++;
		}
		if (*p2 != '\0') {
			*p2 = '\0';	/* break at name's end */
			p2++;
		}

		/* get value */
		if (*p2) {
			p = p2;
			while (*p && isspace((int) *p)) {
				p++;
			}

			value = p;

			p2 = value + strlen(value);
			while (isspace((int) *(p2 - 1))) {
				*--p2 = '\0';
			}
		} else {
			value = 0;
		}

#define CONF2(a) if (strcasecmp(name, a) == 0)
#define CONF(a) else CONF2(a)
#define CONF3(a, b) else if (strcasecmp(name, a) == 0 \
		|| strcasecmp(name, b) == 0)

#ifdef X11
		CONF2("alignment") {
			if (window.type == TYPE_DOCK)
				;
			else if (value) {
				int a = string_to_alignment(value);

				if (a <= 0) {
					CONF_ERR;
				} else {
					text_alignment = a;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("background") {
			fork_to_background = string_to_bool(value);
		}
#else
		CONF2("background") {
			fork_to_background = string_to_bool(value);
		}
#endif /* X11 */
#ifdef X11
		CONF("show_graph_scale") {
			show_graph_scale = string_to_bool(value);
		}
		CONF("show_graph_range") {
			show_graph_range = string_to_bool(value);
		}
		CONF("border_margin") {
			if (value) {
				border_margin = strtol(value, 0, 0);
			} else {
				CONF_ERR;
			}
		}
		CONF("border_width") {
			if (value) {
				border_width = strtol(value, 0, 0);
			} else {
				CONF_ERR;
			}
		}
		CONF("color0") {
			if (value) {
				color0 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color1") {
			if (value) {
				color1 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color2") {
			if (value) {
				color2 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color3") {
			if (value) {
				color3 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color4") {
			if (value) {
				color4 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color5") {
			if (value) {
				color5 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color6") {
			if (value) {
				color6 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color7") {
			if (value) {
				color7 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color8") {
			if (value) {
				color8 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("color9") {
			if (value) {
				color9 = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
#define TEMPLATE_CONF(n) \
		CONF("template"#n) { \
			if (value) { \
				free(template[n]); \
				template[n] = strdup(value); \
			} else { \
				CONF_ERR; \
			} \
		}
		TEMPLATE_CONF(0)
		TEMPLATE_CONF(1)
		TEMPLATE_CONF(2)
		TEMPLATE_CONF(3)
		TEMPLATE_CONF(4)
		TEMPLATE_CONF(5)
		TEMPLATE_CONF(6)
		TEMPLATE_CONF(7)
		TEMPLATE_CONF(8)
		TEMPLATE_CONF(9)
		CONF("default_color") {
			if (value) {
				default_fg_color = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF3("default_shade_color", "default_shadecolor") {
			if (value) {
				default_bg_color = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
		CONF3("default_outline_color", "default_outlinecolor") {
			if (value) {
				default_out_color = get_x11_color(value);
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
		CONF("imap") {
			if (value) {
				info.mail = parse_mail_args(IMAP_TYPE, value);
			} else {
				CONF_ERR;
			}
		}
		CONF("pop3") {
			if (value) {
				info.mail = parse_mail_args(POP3_TYPE, value);
			} else {
				CONF_ERR;
			}
		}
#ifdef MPD
		CONF("mpd_host") {
			if (value) {
				mpd_set_host(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("mpd_port") {
			if (value && mpd_set_port(value)) {
				CONF_ERR;
			}
		}
		CONF("mpd_password") {
			if (value) {
				mpd_set_password(value);
			} else {
				CONF_ERR;
			}
		}
#endif
		CONF("music_player_interval") {
			if (value) {
				info.music_player_interval = strtod(value, 0);
			} else {
				CONF_ERR;
			}
		}
#ifdef __OpenBSD__
		CONF("sensor_device") {
			if (value) {
				sensor_device = strtol(value, 0, 0);
			} else {
				CONF_ERR;
			}
		}
#endif
		CONF("cpu_avg_samples") {
			if (value) {
				cpu_avg_samples = strtol(value, 0, 0);
				if (cpu_avg_samples < 1 || cpu_avg_samples > 14) {
					CONF_ERR;
				} else {
					info.cpu_avg_samples = cpu_avg_samples;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("net_avg_samples") {
			if (value) {
				net_avg_samples = strtol(value, 0, 0);
				if (net_avg_samples < 1 || net_avg_samples > 14) {
					CONF_ERR;
				} else {
					info.net_avg_samples = net_avg_samples;
				}
			} else {
				CONF_ERR;
			}
		}

#ifdef HAVE_XDBE
		CONF("double_buffer") {
			use_xdbe = string_to_bool(value);
		}
#endif
#ifdef X11
		CONF("override_utf8_locale") {
			utf8_mode = string_to_bool(value);
		}
		CONF("draw_borders") {
			draw_borders = string_to_bool(value);
		}
		CONF("draw_graph_borders") {
			draw_graph_borders = string_to_bool(value);
		}
		CONF("draw_shades") {
			draw_shades = string_to_bool(value);
		}
		CONF("draw_outline") {
			draw_outline = string_to_bool(value);
		}
#endif /* X11 */
		CONF("out_to_console") {
			if(string_to_bool(value)) output_methods |= TO_STDOUT;
		}
		CONF("use_spacer") {
			if (value) {
				if (strcasecmp(value, "left") == EQUAL) {
					use_spacer = LEFT_SPACER;
				} else if (strcasecmp(value, "right") == EQUAL) {
					use_spacer = RIGHT_SPACER;
				} else if (strcasecmp(value, "none") == EQUAL) {
					use_spacer = NO_SPACER;
				} else {
					use_spacer = string_to_bool(value);
					ERR("use_spacer should have an argument of left, right, or"
						" none.  '%s' seems to be some form of '%s', so"
						" defaulting to %s.", value,
						use_spacer ? "true" : "false",
						use_spacer ? "right" : "none");
					if (use_spacer) {
						use_spacer = RIGHT_SPACER;
					} else {
						use_spacer = NO_SPACER;
					}
				}
			} else {
				ERR("use_spacer should have an argument. Defaulting to right.");
				use_spacer = RIGHT_SPACER;
			}
		}
#ifdef X11
#ifdef XFT
		CONF("use_xft") {
			use_xft = string_to_bool(value);
		}
		CONF("font") {
			if (value) {
				set_first_font(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("xftalpha") {
			if (value && font_count >= 0) {
				fonts[0].font_alpha = atof(value) * 65535.0;
			} else {
				CONF_ERR;
			}
		}
		CONF("xftfont") {
			if (use_xft) {
#else
		CONF("use_xft") {
			if (string_to_bool(value)) {
				ERR("Xft not enabled");
			}
		}
		CONF("xftfont") {
			/* xftfont silently ignored when no Xft */
		}
		CONF("xftalpha") {
			/* xftalpha is silently ignored when no Xft */
		}
		CONF("font") {
#endif
				if (value) {
					set_first_font(value);
				} else {
					CONF_ERR;
				}
#ifdef XFT
			}
#endif
		}
		CONF("gap_x") {
			if (value) {
				gap_x = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("gap_y") {
			if (value) {
				gap_y = atoi(value);
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
		CONF("mail_spool") {
			if (value) {
				char buffer[256];

				variable_substitute(value, buffer, 256);

				if (buffer[0] != '\0') {
					if (current_mail_spool) {
						free(current_mail_spool);
					}
					current_mail_spool = strndup(buffer, text_buffer_size);
				}
			} else {
				CONF_ERR;
			}
		}
#ifdef X11
		CONF("minimum_size") {
			if (value) {
				if (sscanf(value, "%d %d", &minimum_width, &minimum_height)
						!= 2) {
					if (sscanf(value, "%d", &minimum_width) != 1) {
						CONF_ERR;
					}
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("maximum_width") {
			if (value) {
				if (sscanf(value, "%d", &maximum_width) != 1) {
					CONF_ERR;
				}
			} else {
				CONF_ERR;
			}
		}
#endif /* X11 */
		CONF("no_buffers") {
			no_buffers = string_to_bool(value);
		}
		CONF("top_cpu_separate") {
			cpu_separate = string_to_bool(value);
		}
		CONF("short_units") {
			short_units = string_to_bool(value);
		}
		CONF("pad_percents") {
			pad_percents = atoi(value);
		}
#ifdef X11
#ifdef OWN_WINDOW
		CONF("own_window") {
			if (value) {
				own_window = string_to_bool(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_class") {
			if (value) {
				memset(window.class_name, 0, sizeof(window.class_name));
				strncpy(window.class_name, value,
					sizeof(window.class_name) - 1);
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_title") {
			if (value) {
				memset(window.title, 0, sizeof(window.title));
				strncpy(window.title, value, sizeof(window.title) - 1);
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_transparent") {
			if (value) {
				set_transparent = string_to_bool(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_colour") {
			if (value) {
				background_colour = get_x11_color(value);
			} else {
				ERR("Invalid colour for own_window_colour (try omitting the "
					"'#' for hex colours");
			}
		}
		CONF("own_window_hints") {
			if (value) {
				char *p_hint, *p_save;
				char delim[] = ", ";

				/* tokenize the value into individual hints */
				if ((p_hint = strtok_r(value, delim, &p_save)) != NULL) {
					do {
						/* fprintf(stderr, "hint [%s] parsed\n", p_hint); */
						if (strncmp(p_hint, "undecorate", 10) == EQUAL) {
							SET_HINT(window.hints, HINT_UNDECORATED);
						} else if (strncmp(p_hint, "below", 5) == EQUAL) {
							SET_HINT(window.hints, HINT_BELOW);
						} else if (strncmp(p_hint, "above", 5) == EQUAL) {
							SET_HINT(window.hints, HINT_ABOVE);
						} else if (strncmp(p_hint, "sticky", 6) == EQUAL) {
							SET_HINT(window.hints, HINT_STICKY);
						} else if (strncmp(p_hint, "skip_taskbar", 12) == EQUAL) {
							SET_HINT(window.hints, HINT_SKIP_TASKBAR);
						} else if (strncmp(p_hint, "skip_pager", 10) == EQUAL) {
							SET_HINT(window.hints, HINT_SKIP_PAGER);
						} else {
							CONF_ERR;
						}

						p_hint = strtok_r(NULL, delim, &p_save);
					} while (p_hint != NULL);
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("own_window_type") {
			if (value) {
				if (strncmp(value, "normal", 6) == EQUAL) {
					window.type = TYPE_NORMAL;
				} else if (strncmp(value, "desktop", 7) == EQUAL) {
					window.type = TYPE_DESKTOP;
				} else if (strncmp(value, "dock", 7) == EQUAL) {
					window.type = TYPE_DOCK;
					text_alignment = TOP_LEFT;
				} else if (strncmp(value, "override", 8) == EQUAL) {
					window.type = TYPE_OVERRIDE;
				} else {
					CONF_ERR;
				}
			} else {
				CONF_ERR;
			}
		}
#endif
		CONF("stippled_borders") {
			if (value) {
				stippled_borders = strtol(value, 0, 0);
			} else {
				stippled_borders = 4;
			}
		}
#endif /* X11 */
		CONF("temp1") {
			ERR("temp1 configuration is obsolete, use ${i2c <i2c device here> "
				"temp 1}");
		}
		CONF("temp2") {
			ERR("temp2 configuration is obsolete, use ${i2c <i2c device here> "
				"temp 2}");
		}
		CONF("update_interval") {
			if (value) {
				update_interval = strtod(value, 0);
			} else {
				CONF_ERR;
			}
			if (info.music_player_interval == 0) {
				// default to update_interval
				info.music_player_interval = update_interval;
			}
		}
		CONF("total_run_times") {
			if (value) {
				total_run_times = strtod(value, 0);
			} else {
				CONF_ERR;
			}
		}
		CONF("uppercase") {
			stuff_in_upper_case = string_to_bool(value);
		}
		CONF("max_specials") {
			if (value) {
				max_specials = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("max_user_text") {
			if (value) {
				max_user_text = atoi(value);
			} else {
				CONF_ERR;
			}
		}
		CONF("text_buffer_size") {
			if (value) {
				text_buffer_size = atoi(value);
				if (text_buffer_size < DEFAULT_TEXT_BUFFER_SIZE) {
					ERR("text_buffer_size must be >=%i bytes", DEFAULT_TEXT_BUFFER_SIZE);
					text_buffer_size = DEFAULT_TEXT_BUFFER_SIZE;
				}
			} else {
				CONF_ERR;
			}
		}
		CONF("text") {
			if (global_text) {
				free(global_text);
				global_text = 0;
			}

			global_text = (char *) malloc(1);
			global_text[0] = '\0';

			while (!feof(fp)) {
				unsigned int l = strlen(global_text);
				unsigned int bl;

				if (fgets(buf, 256, fp) == NULL) {
					break;
				}

				/* Remove \\-\n. */
				bl = strlen(buf);
				if (bl >= 2 && buf[bl-2] == '\\' && buf[bl-1] == '\n') {
					buf[bl-2] = '\0';
					bl -= 2;
					if (bl == 0) {
						continue;
					}
				}

				/* Check for continuation of \\-\n. */
				if (l > 0 && buf[0] == '\n' && global_text[l-1] == '\\') {
					global_text[l-1] = '\0';
					continue;
				}

				global_text = (char *) realloc(global_text, l + bl + 1);
				strcat(global_text, buf);

				if (strlen(global_text) > max_user_text) {
					break;
				}
			}
			fclose(fp);
			if (strlen(global_text) < 1) {
				CRIT_ERR("no text supplied in configuration; exiting");
			}
			global_text_lines = line + 1;
			return;
		}
#ifdef TCP_PORT_MONITOR
		CONF("max_port_monitor_connections") {
			int max;
			if (!value || (sscanf(value, "%d", &max) != 1)) {
				/* an error. use default, warn and continue. */
				tcp_portmon_set_max_connections(0);
				CONF_ERR;
			} else if (tcp_portmon_set_max_connections(max)) {
				/* max is < 0, default has been set*/
				CONF_ERR;
			}
		}
#endif
		CONF("if_up_strictness") {
			if (!value) {
				ERR("incorrect if_up_strictness value, defaulting to 'up'");
				ifup_strictness = IFUP_UP;
			} else if (strcasecmp(value, "up") == EQUAL) {
				ifup_strictness = IFUP_UP;
			} else if (strcasecmp(value, "link") == EQUAL) {
				ifup_strictness = IFUP_LINK;
			} else if (strcasecmp(value, "address") == EQUAL) {
				ifup_strictness = IFUP_ADDR;
			} else {
				ERR("incorrect if_up_strictness value, defaulting to 'up'");
				ifup_strictness = IFUP_UP;
			}
		}

		CONF("temperature_unit") {
			if (!value) {
				ERR("config option 'temperature_unit' needs an argument");
			} else if (set_temp_output_unit(value)) {
				ERR("temperature_unit: incorrect argument");
			}
		}

		else {
			ERR("%s: %d: no such configuration: '%s'", f, line, name);
		}

#undef CONF
#undef CONF2
	}

	fclose(fp);

#undef CONF_ERR

	if (info.music_player_interval == 0) {
		// default to update_interval
		info.music_player_interval = update_interval;
	}
	if (!global_text) { // didn't supply any text
		CRIT_ERR("missing text block in configuration; exiting");
	}
}

static void print_help(const char *prog_name) {
	printf("Usage: %s [OPTION]...\n"
			PACKAGE_NAME" is a system monitor that renders text on desktop or to own transparent\n"
			"window. Command line options will override configurations defined in config\n"
			"file.\n"
			"   -v, --version             version\n"
			"   -q, --quiet               quiet mode\n"
			"   -D, --debug               increase debugging output\n"
			"   -c, --config=FILE         config file to load\n"
			"   -d, --daemonize           daemonize, fork to background\n"
			"   -h, --help                help\n"
#ifdef X11
			"   -a, --alignment=ALIGNMENT text alignment on screen, {top,bottom,middle}_{left,right,middle}\n"
			"   -f, --font=FONT           font to use\n"
#ifdef OWN_WINDOW
			"   -o, --own-window          create own window to draw\n"
#endif
#ifdef HAVE_XDBE
			"   -b, --double-buffer       double buffer (prevents flickering)\n"
#endif
			"   -w, --window-id=WIN_ID    window id to draw\n"
			"   -x X                      x position\n"
			"   -y Y                      y position\n"
#endif /* X11 */
			"   -t, --text=TEXT           text to render, remember single quotes, like -t '$uptime'\n"
			"   -u, --interval=SECS       update interval\n"
			"   -i COUNT                  number of times to update "PACKAGE_NAME" (and quit)\n",
			prog_name
	);
}

/* : means that character before that takes an argument */
static const char *getopt_string = "vVqdDt:u:i:hc:"
#ifdef X11
	"x:y:w:a:f:"
#ifdef OWN_WINDOW
	"o"
#endif
#ifdef HAVE_XDBE
	"b"
#endif
#ifdef CONFIG_OUTPUT
	"C"
#endif
#endif /* X11 */
	;

static const struct option longopts[] = {
	{ "help", 0, NULL, 'h' },
	{ "version", 0, NULL, 'V' },
	{ "debug", 0, NULL, 'D' },
	{ "config", 1, NULL, 'c' },
#ifdef CONFIG_OUTPUT
	{ "print-config", 0, NULL, 'C' },
#endif
	{ "daemonize", 0, NULL, 'd' },
#ifdef X11
	{ "alignment", 1, NULL, 'a' },
	{ "font", 1, NULL, 'f' },
#ifdef OWN_WINDOW
	{ "own-window", 0, NULL, 'o' },
#endif
#ifdef HAVE_XDBE
	{ "double-buffer", 0, NULL, 'b' },
#endif
	{ "window-id", 1, NULL, 'w' },
#endif /* X11 */
	{ "text", 1, NULL, 't' },
	{ "interval", 0, NULL, 'u' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
#ifdef X11
	char *s, *temp;
	unsigned int x;
#endif
	struct sigaction act, oact;

	g_signal_pending = 0;
	memset(&info, 0, sizeof(info));
	clear_net_stats();

#ifdef TCP_PORT_MONITOR
	/* set default connection limit */
	tcp_portmon_set_max_connections(0);
#endif

	/* handle command line parameters that don't change configs */
#ifdef X11
	if (((s = getenv("LC_ALL")) && *s) || ((s = getenv("LC_CTYPE")) && *s)
			|| ((s = getenv("LANG")) && *s)) {
		temp = (char *) malloc((strlen(s) + 1) * sizeof(char));
		if (temp == NULL) {
			ERR("malloc failed");
		}
		for (x = 0; x < strlen(s); x++) {
			temp[x] = tolower(s[x]);
		}
		temp[x] = 0;
		if (strstr(temp, "utf-8") || strstr(temp, "utf8")) {
			utf8_mode = 1;
		}

		free(temp);
	}
	if (!setlocale(LC_CTYPE, "")) {
		ERR("Can't set the specified locale!\nCheck LANG, LC_CTYPE, LC_ALL.");
	}
#endif /* X11 */
	while (1) {
		int c = getopt_long(argc, argv, getopt_string, longopts, NULL);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'v':
			case 'V':
				print_version();
			case 'c':
				if (current_config) {
					free(current_config);
				}
				current_config = strndup(optarg, max_user_text);
				break;
			case 'q':
				freopen("/dev/null", "w", stderr);
				break;
			case 'h':
				print_help(argv[0]);
				return 0;
#ifdef CONFIG_OUTPUT
			case 'C':
				print_defconfig();
				return 0;
#endif
#ifdef X11
			case 'w':
				window.window = strtol(optarg, 0, 0);
				break;
#endif /* X11 */

			case '?':
				exit(EXIT_FAILURE);
		}
	}
#ifdef X11
	/* initalize X BEFORE we load config.
	 * (we need to so that 'screen' is set) */
	init_X11();
#endif /* X11 */

	/* check if specified config file is valid */
	if (current_config) {
		struct stat sb;
		if (stat(current_config, &sb) ||
				(!S_ISREG(sb.st_mode) && !S_ISLNK(sb.st_mode))) {
			ERR("invalid configuration file '%s'\n", current_config);
			free(current_config);
			current_config = 0;
		}
	}

	/* load current_config, CONFIG_FILE or SYSTEM_CONFIG_FILE */

	if (!current_config) {
		/* load default config file */
		char buf[256];
		FILE *fp;

		/* Try to use personal config file first */
		variable_substitute(CONFIG_FILE, buf, sizeof(buf));
		if (buf[0] && (fp = fopen(buf, "r"))) {
			current_config = strndup(buf, max_user_text);
			fclose(fp);
		}

		/* Try to use system config file if personal config not readable */
		if (!current_config && (fp = fopen(SYSTEM_CONFIG_FILE, "r"))) {
			current_config = strndup(SYSTEM_CONFIG_FILE, max_user_text);
			fclose(fp);
		}

		/* No readable config found */
		if (!current_config) {
#ifdef CONFIG_OUTPUT
			current_config = strdup("==builtin==");
			ERR("no readable personal or system-wide config file found,"
					" using builtin default");
#else
			CRIT_ERR("no readable personal or system-wide config file found");
#endif /* ! CONF_OUTPUT */
		}
	}

	load_config_file(current_config);

	/* init specials array */
	if ((specials = calloc(sizeof(struct special_t), max_specials)) == 0) {
		ERR("failed to create specials array");
	}

#ifdef MAIL_FILE
	if (current_mail_spool == NULL) {
		char buf[256];

		variable_substitute(MAIL_FILE, buf, 256);

		if (buf[0] != '\0') {
			current_mail_spool = strndup(buf, text_buffer_size);
		}
	}
#endif

	/* handle other command line arguments */

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) \
		|| defined(__NetBSD__)
	optind = optreset = 1;
#else
	optind = 0;
#endif

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", O_RDONLY,
			"kvm_open")) == NULL) {
		CRIT_ERR("cannot read kvm");
	}
#endif

	while (1) {
		int c = getopt_long(argc, argv, getopt_string, longopts, NULL);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'd':
				fork_to_background = 1;
				break;
			case 'D':
				global_debug_level++;
				break;
#ifdef X11
			case 'f':
				set_first_font(optarg);
				break;
			case 'a':
				text_alignment = string_to_alignment(optarg);
				break;

#ifdef OWN_WINDOW
			case 'o':
				own_window = 1;
				break;
#endif
#ifdef HAVE_XDBE
			case 'b':
				use_xdbe = 1;
				break;
#endif
#endif /* X11 */
			case 't':
				if (global_text) {
					free(global_text);
					global_text = 0;
				}
				global_text = strndup(optarg, max_user_text);
				convert_escapes(global_text);
				break;

			case 'u':
				update_interval = strtod(optarg, 0);
				if (info.music_player_interval == 0) {
					// default to update_interval
					info.music_player_interval = update_interval;
				}
				break;

			case 'i':
				total_run_times = strtod(optarg, 0);
				break;
#ifdef X11
			case 'x':
				gap_x = atoi(optarg);
				break;

			case 'y':
				gap_y = atoi(optarg);
				break;
#endif /* X11 */

			case '?':
				exit(EXIT_FAILURE);
		}
	}

#ifdef X11
	/* load font */
	load_fonts();
#endif /* X11 */

	/* generate text and get initial size */
	extract_variable_text(global_text);
	if (global_text) {
		free(global_text);
		global_text = 0;
	}
	global_text = NULL;
	/* fork */
	if (fork_to_background) {
		int pid = fork();

		switch (pid) {
			case -1:
				ERR(PACKAGE_NAME": couldn't fork() to background: %s",
					strerror(errno));
				break;

			case 0:
				/* child process */
				usleep(25000);
				fprintf(stderr, "\n");
				fflush(stderr);
				break;

			default:
				/* parent process */
				fprintf(stderr, PACKAGE_NAME": forked to background, pid is %d\n",
					pid);
				fflush(stderr);
				return 0;
		}
	}

	text_buffer = malloc(max_user_text);
	memset(text_buffer, 0, max_user_text);
	tmpstring1 = malloc(text_buffer_size);
	memset(tmpstring1, 0, text_buffer_size);
	tmpstring2 = malloc(text_buffer_size);
	memset(tmpstring2, 0, text_buffer_size);

#ifdef X11
	selected_font = 0;
	update_text_area();	/* to get initial size of the window */

#ifdef OWN_WINDOW
	init_window(own_window, text_width + border_margin * 2 + 1,
		text_height + border_margin * 2 + 1, set_transparent, background_colour,
		argv, argc);
#else /* OWN_WINDOW */
	init_window(0, text_width + border_margin * 2 + 1,
		text_height + border_margin * 2 + 1, set_transparent, 0,
		argv, argc);
#endif /* OWN_WINDOW */

	selected_font = 0;
	update_text_area();	/* to position text/window on screen */

#ifdef OWN_WINDOW
	if (own_window && !fixed_pos) {
		XMoveWindow(display, window.window, window.x, window.y);
	}
	if (own_window) {
		set_transparent_background(window.window);
	}
#endif

	create_gc();

	set_font();
	draw_stuff();
#endif /* X11 */

	/* Set signal handlers */
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
#ifdef SA_RESTART
	act.sa_flags |= SA_RESTART;
#endif

	if (		sigaction(SIGINT,  &act, &oact) < 0
			||	sigaction(SIGUSR1, &act, &oact) < 0
			||	sigaction(SIGHUP,  &act, &oact) < 0
			||	sigaction(SIGTERM, &act, &oact) < 0) {
		ERR("error setting signal handler: %s", strerror(errno));
	}

	main_loop();

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
	kvm_close(kd);
#endif

	return 0;
}

static void signal_handler(int sig)
{
	/* signal handler is light as a feather, as it should be.
	 * we will poll g_signal_pending with each loop of conky
	 * and do any signal processing there, NOT here. */
	g_signal_pending = sig;
}
