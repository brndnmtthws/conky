/*
 * Conky, a system monitor, based on torsmo
 *
 * This program is licensed under BSD license, read COPYING
 *
 *  $Id$
 */

#include "conky.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <sys/time.h>
#ifdef X11
#include <X11/Xutil.h>
#endif /* X11 */
#include <sys/types.h>
#include <sys/stat.h>

#define CONFIG_FILE "$HOME/.conkyrc"
#define MAIL_FILE "$MAIL"
#define MAX_IF_BLOCK_DEPTH 5

/* #define SIGNAL_BLOCKING */
#undef SIGNAL_BLOCKING

#ifdef X11

/* alignments */
enum alignment {
	TOP_LEFT = 1,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT,
	NONE
};


/* for fonts */
struct font_list {

	char name[TEXT_BUFFER_SIZE];
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

#define font_height() use_xft ? (fonts[selected_font].xftfont->ascent + fonts[selected_font].xftfont->descent) : \
(fonts[selected_font].font->max_bounds.ascent + fonts[selected_font].font->max_bounds.descent)
#define font_ascent() use_xft ? fonts[selected_font].xftfont->ascent : fonts[selected_font].font->max_bounds.ascent
#define font_descent() use_xft ? fonts[selected_font].xftfont->descent : fonts[selected_font].font->max_bounds.descent

#else

#define font_height() (fonts[selected_font].font->max_bounds.ascent + fonts[selected_font].font->max_bounds.descent)
#define font_ascent() fonts[selected_font].font->max_bounds.ascent
#define font_descent() fonts[selected_font].font->max_bounds.descent

#endif

#define MAX_FONTS 64 // hmm, no particular reason, just makes sense.


static void set_font();

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
		if ((fonts = (struct font_list*)malloc(sizeof(struct font_list))) == NULL) {
			CRIT_ERR("malloc");
		}
	}
	fonts = realloc(fonts, (sizeof(struct font_list) * (font_count+1)));
	if (fonts == NULL) {
		CRIT_ERR("realloc in addfont");
	}
	if (strlen(data_in) < TEXT_BUFFER_SIZE) { // must account for null terminator
		strncpy(fonts[font_count].name, data_in, TEXT_BUFFER_SIZE);
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
		if ((fonts = (struct font_list*)malloc(sizeof(struct font_list))) == NULL) {
			CRIT_ERR("malloc");
		}
		font_count++;
	}
	if (strlen(data_in) > 1) {
		strncpy(fonts[0].name, data_in, TEXT_BUFFER_SIZE-1);
#ifdef XFT
		fonts[0].font_alpha = 0xffff;
#endif
	}
}

void free_fonts()
{
	int i;
	for (i=0;i<=font_count;i++) {
#ifdef XFT
		if (use_xft) {
			XftFontClose(display, fonts[i].xftfont);
		} else
#endif
		{
			XFreeFont(display, fonts[i].font);
		}
}
	free(fonts);
	fonts = NULL;
	font_count = -1;
	selected_font = 0;
	set_first_font("6x10");
}


static void load_fonts()
{
	int i;
	for (i=0;i<=font_count;i++) {
#ifdef XFT
	/* load Xft font */
	if (use_xft) {
	/*if (fonts[i].xftfont != NULL && selected_font == 0) {
			XftFontClose(display, fonts[i].xftfont);
	}*/
		if ((fonts[i].xftfont =
			XftFontOpenName(display, screen, fonts[i].name)) != NULL)
			continue;
		
		ERR("can't load Xft font '%s'", fonts[i].name);
		if ((fonts[i].xftfont =
			XftFontOpenName(display, screen,
					"courier-12")) != NULL)
			continue;
		
		ERR("can't load Xft font '%s'", "courier-12");
		
		if ((fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
			CRIT_ERR("can't load font '%s'", "fixed");
		}
		use_xft = 0;
		
		continue;
	}
#endif
	/* load normal font */
/*	if (fonts[i].font != NULL)
		XFreeFont(display, fonts[i].font);*/
	
	if ((fonts[i].font = XLoadQueryFont(display, fonts[i].name)) == NULL) {
		ERR("can't load font '%s'", fonts[i].name);
		if ((fonts[i].font = XLoadQueryFont(display, "fixed")) == NULL) {
			CRIT_ERR("can't load font '%s'", "fixed");
			printf("loaded fixed?\n");
		}
	}
	}
}

#endif /* X11 */

/* default config file */
static char *current_config;

/* set to 1 if you want all text to be in uppercase */
static unsigned int stuff_in_upper_case;

/* Update interval */
static double update_interval;

/* Run how many times? */
static unsigned long total_run_times;

/* fork? */
static int fork_to_background;

static int cpu_avg_samples, net_avg_samples;

#ifdef X11

/* Always on bottom */
static int on_bottom;

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

/* UTF-8 */
int utf8_mode = 0;

#endif /* X11 */

/* no buffers in used memory? */
int no_buffers;

/* pad percentages to decimals? */
static int pad_percents = 0;

#ifdef TCP_PORT_MONITOR
tcp_port_monitor_collection_args_t 	tcp_port_monitor_collection_args;
tcp_port_monitor_args_t 		tcp_port_monitor_args;
#endif

/* Text that is shown */
static char original_text[] =
    "$nodename - $sysname $kernel on $machine\n"
    "$hr\n"
    "${color grey}Uptime:$color $uptime\n"
    "${color grey}Frequency (in MHz):$color $freq\n"
    "${color grey}Frequency (in GHz):$color $freq_g\n"
    "${color grey}RAM Usage:$color $mem/$memmax - $memperc% ${membar 4}\n"
    "${color grey}Swap Usage:$color $swap/$swapmax - $swapperc% ${swapbar 4}\n"
    "${color grey}CPU Usage:$color $cpu% ${cpubar 4}\n"
    "${color grey}Processes:$color $processes  ${color grey}Running:$color $running_processes\n"
    "$hr\n"
    "${color grey}File systems:\n"
    " / $color${fs_free /}/${fs_size /} ${fs_bar 6 /}\n"
    "${color grey}Networking:\n"
    " Up:$color ${upspeed eth0} k/s${color grey} - Down:$color ${downspeed eth0} k/s\n"
    "$hr\n"
#ifdef SETI
    "${color grey}SETI@Home Statistics:\n"
    "${color grey}Seti Unit Number:$color $seti_credit\n"
    "${color grey}Seti Progress:$color $seti_prog% $seti_progbar\n"
#endif
#ifdef MPD
    "${color grey}MPD: $mpd_status $mpd_artist - $mpd_title from $mpd_album at $mpd_vol\n"
    "Bitrate: $mpd_bitrate\n" "Progress: $mpd_bar\n"
#endif
#ifdef XMMS2
    "${color grey}XMMS2: $xmms2_status $xmms2_artist - $xmms2_title from $xmms2_album\n"
    "Progress: $xmms2_bar\n"
#endif
    "${color grey}Name		PID	CPU%	MEM%\n"
    " ${color lightgrey} ${top name 1} ${top pid 1} ${top cpu 1} ${top mem 1}\n"
    " ${color lightgrey} ${top name 2} ${top pid 2} ${top cpu 2} ${top mem 2}\n"
    " ${color lightgrey} ${top name 3} ${top pid 3} ${top cpu 3} ${top mem 3}\n"
    " ${color lightgrey} ${top name 4} ${top pid 4} ${top cpu 4} ${top mem 4}\n"
    "${tail /var/log/Xorg.0.log 3}";

static char *text = original_text;

static int total_updates;

/* if-blocks */
static int blockdepth = 0;
static int if_jumped = 0;
static int blockstart[MAX_IF_BLOCK_DEPTH];

int check_mount(char *s)
{
#if defined(__linux__)
	int ret = 0;
	FILE *mtab = fopen("/etc/mtab", "r");
	if (mtab) {
		char buf1[256], buf2[128];
		while (fgets(buf1, 256, mtab)) {
			sscanf(buf1, "%*s %128s", buf2);
			if (!strcmp(s, buf2)) {
				ret = 1;
				break;
			}
		}
		fclose(mtab);
	} else {
		ERR("Could not open mtab");
	}
	return ret;
#elif defined(__FreeBSD__)
	struct statfs *mntbuf;
	int i, mntsize;

	mntsize = getmntinfo(&mntbuf, MNT_NOWAIT);
	for (i = mntsize - 1; i >= 0; i--)
		if (strcmp(mntbuf[i].f_mntonname, s) == 0)
			return 1;

	return 0;
#endif
}


#ifdef X11
static inline int calc_text_width(const char *s, int l)
{
#ifdef XFT
	if (use_xft) {
		XGlyphInfo gi;
		if (utf8_mode) {
			XftTextExtentsUtf8(display, fonts[selected_font].xftfont, s, l, &gi);
		} else {
			XftTextExtents8(display, fonts[selected_font].xftfont, s, l, &gi);
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

static char text_buffer[TEXT_BUFFER_SIZE * 4];

/* special stuff in text_buffer */

#define SPECIAL_CHAR '\x01'

enum {
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
};

static struct special_t {
	int type;
	short height;
	short width;
	long arg;
	double *graph;
	double graph_scale;
	int graph_width;
	int scaled;
	short font_added;
	unsigned long first_colour; // for graph gradient
	unsigned long last_colour;
} specials[128];

static int special_count;
#ifdef X11
static int special_index;	/* used when drawing */
#endif /* X11 */

#define MAX_GRAPH_DEPTH 256	/* why 256? cause an array of more then 256 doubles seems excessive, and who needs that kind of precision anyway? */

static struct special_t *new_special(char *buf, int t)
{
	if (special_count >= 512)
		CRIT_ERR("too many special things in text");

	buf[0] = SPECIAL_CHAR;
	buf[1] = '\0';
	specials[special_count].type = t;
	return &specials[special_count++];
}

typedef struct tailstring_list {
	char data[TEXT_BUFFER_SIZE];
	struct tailstring_list *next;
	struct tailstring_list *first;
} tailstring;

void addtail(tailstring ** head, char *data_in)
{
	tailstring *tmp;
	if ((tmp = malloc(sizeof(*tmp))) == NULL) {
		CRIT_ERR("malloc");
	}
	if (*head == NULL) {
		tmp->first = tmp;
	} else {
		tmp->first = (*head)->first;
	}
	strncpy(tmp->data, data_in, TEXT_BUFFER_SIZE);
	tmp->next = *head;
	*head = tmp;
}

void freetail(tailstring * head)
{
	tailstring *tmp;
	while (head != NULL) {
		tmp = head->next;
		free(head);
		head = tmp;
	}
}

void freelasttail(tailstring * head)
{
	tailstring * tmp = head;
	while(tmp != NULL) {
		if (tmp->next == head->first) {
			tmp->next = NULL;
			break;
		}
		tmp = tmp->next;
	}
	free(head->first);
	while(head != NULL && tmp != NULL) {
		head->first = tmp;
		head = head->next;
	}
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
	*w = 0;			/* zero width means all space that is available */
	*h = 6;
	/* bar's argument is either height or height,width */
	if (args) {
		int n = 0;
		if (sscanf(args, "%d,%d %n", h, w, &n) <= 1)
			sscanf(args, "%d %n", h, &n);
		args += n;
	}

	return args;
}

static char *scan_font(const char *args)
{
	if (args && sizeof(args) < 127) {
		return strdup(args);
	}
	else {
		ERR("font scan failed, lets hope it doesn't mess stuff up");
	}
	return NULL;
}

#ifdef X11
static void new_font(char *buf, char * args) {
	struct special_t *s = new_special(buf, FONT);
	if (!s->font_added || strcmp(args, fonts[s->font_added].name)) {
		int tmp = selected_font;
		selected_font = s->font_added = addfont(args);
		load_fonts();
//		set_font();
		selected_font = tmp;
	}
}
#endif

inline void graph_append(struct special_t *graph, double f)
{
	if (!graph->scaled && f > graph->graph_scale) {
		f = graph->graph_scale;
	}
	int i;
	if (graph->scaled) {
		graph->graph_scale = 1;
	}
	graph->graph[0] = f; /* add new data */
	for (i = graph->graph_width - 1; i > 0; i--) { /* shift all the data by 1 */
		graph->graph[i] = graph->graph[i - 1];
		if (graph->scaled && graph->graph[i] > graph->graph_scale) {
			graph->graph_scale = graph->graph[i]; /* check if we need to update the scale */
		}
	}
}

short colour_depth = 0;
void set_up_gradient();

/* precalculated: 31/255, and 63/255 */
#define CONST_8_TO_5_BITS 0.12156862745098
#define CONST_8_TO_6_BITS 0.247058823529412

/* adjust color values depending on color depth*/
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
		color  = (int)(r * CONST_8_TO_5_BITS) << 11;
		color |= (int)(g * CONST_8_TO_6_BITS) << 5;
		color |= (int)(b * CONST_8_TO_5_BITS);
	}
	return color;
}

static void new_graph(char *buf, int w, int h, unsigned int first_colour, unsigned int second_colour, double i, int scale, int append)
{
	struct special_t *s = new_special(buf, GRAPH);
	s->width = w;
	if (s->graph == NULL) {
		if (s->width > 0 && s->width < MAX_GRAPH_DEPTH) {
			s->graph_width = s->width - 3;	// subtract 3 for the box
		} else {
			s->graph_width = MAX_GRAPH_DEPTH - 3;
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
	} else {
		s->scaled = 1;
	}
	/*if (s->width) {
		s->graph_width = s->width - 3;	// subtract 3 for rectangle around
	}*/
	if (s->scaled) {
		s->graph_scale = 1;
	} else {
		s->graph_scale = scale;
	}
	if (append) {
		graph_append(s, i);
	}
}

static const char *scan_graph(const char *args, int *w, int *h, unsigned int *first_colour, unsigned int *last_colour, unsigned int *scale)
{
	*w = 0;			/* zero width means all space that is available */
	*h = 25;
	*first_colour = 0;
	*last_colour = 0;
	/* graph's argument is either height or height,width */
	if (args) {
		if (sscanf(args, "%*s %d,%d %x %x %i", h, w, first_colour, last_colour, scale) < 5) {
			if (sscanf(args, "%*s %d,%d %x %x", h, w, first_colour, last_colour) < 4) {
				*scale = 0;
				if (sscanf(args, "%d,%d %x %x %i", h, w, first_colour, last_colour, scale) < 5) {
					*scale = 0;
					if (sscanf(args, "%d,%d %x %x", h, w, first_colour, last_colour) < 4) {
						*w = 0;
				*h = 25;			
				if (sscanf(args, "%*s %x %x %i", first_colour, last_colour, scale) < 3) {
				*w = 0;
				*h = 25;
				*scale = 0;
				if (sscanf(args, "%*s %x %x", first_colour, last_colour) < 2) {
					*w = 0;
					*h = 25;
					if (sscanf(args, "%x %x %i", first_colour, last_colour, scale) < 3) {
						*first_colour = 0;
						*last_colour = 0;
						*scale = 0;
						if (sscanf(args, "%x %x", first_colour, last_colour) < 2) {
					*first_colour = 0;
					*last_colour = 0;
					if (sscanf(args, "%d,%d %i", h, w, scale) < 3) {
						*first_colour = 0;
						*last_colour = 0;
						*scale = 0;
						if (sscanf(args, "%d,%d", h, w) < 2) {
							*first_colour = 0;
							*last_colour = 0;
							sscanf(args, "%*s %d,%d", h, w);
	}}}}}}}}}}} // haha
	return args;
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

static inline void new_alignc(char *buf, long c)
{
	new_special(buf, ALIGNC)->arg = c;
}

/* quite boring functions */

static inline void for_each_line(char *b, void (*f) (char *))
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

	if (ps < pe)
		f(ps);
}

static void convert_escapes(char *buf)
{
	char *p = buf, *s = buf;

	while (*s) {
		if (*s == '\\') {
			s++;
			if (*s == 'n')
				*p++ = '\n';
			else if (*s == '\\')
				*p++ = '\\';
			s++;
		} else
			*p++ = *s++;
	}
	*p = '\0';
}

/* converts from bytes to human readable format (k, M, G, T) */
static void human_readable(long long a, char *buf, int size)
{
	// Strange conditional due to possible overflows
	if(a / 1024 / 1024 / 1024.0 > 1024.0){
		snprintf(buf, size, "%.2fT", (a / 1024 / 1024 / 1024) / 1024.0);
	}
	else if (a >= 1024 * 1024 * 1024) {
		snprintf(buf, size, "%.2fG", (a / 1024 / 1024) / 1024.0);
	}
	else if (a >= 1024 * 1024) {
		double m = (a / 1024) / 1024.0;
		if (m >= 100.0)
			snprintf(buf, size, "%.0fM", m);
		else
			snprintf(buf, size, "%.1fM", m);
	} else if (a >= 1024)
		snprintf(buf, size, "%Ldk", a / (long long) 1024);
	else
		snprintf(buf, size, "%Ld", a);
}

/* text handling */

enum text_object_type {
	OBJ_acpiacadapter,
	OBJ_adt746xcpu,
	OBJ_adt746xfan,
	OBJ_acpifan,
	OBJ_addr,
	OBJ_linkstatus,
	OBJ_acpitemp,
	OBJ_acpitempf,
	OBJ_battery,
	OBJ_buffers,
	OBJ_cached,
	OBJ_color,
	OBJ_font,
	OBJ_cpu,
	OBJ_cpubar,
	OBJ_cpugraph,
	OBJ_diskio,
	OBJ_diskiograph,
	OBJ_downspeed,
	OBJ_downspeedf,
	OBJ_downspeedgraph,
	OBJ_else,
	OBJ_endif,
	OBJ_exec,
	OBJ_execi,
	OBJ_texeci,
	OBJ_execbar,
	OBJ_execgraph,
	OBJ_execibar,
	OBJ_execigraph,
	OBJ_freq,
	OBJ_freq_g,
	OBJ_freq_dyn,
	OBJ_freq_dyn_g,
	OBJ_fs_bar,
	OBJ_fs_bar_free,
	OBJ_fs_free,
	OBJ_fs_free_perc,
	OBJ_fs_size,
	OBJ_fs_used,
	OBJ_fs_used_perc,
	OBJ_hr,
	OBJ_offset,
	OBJ_voffset,
	OBJ_alignr,
	OBJ_alignc,
	OBJ_i2c,
#if defined(__linux__)
	OBJ_i8k_version,
	OBJ_i8k_bios,
	OBJ_i8k_serial,
	OBJ_i8k_cpu_temp,
	OBJ_i8k_cpu_tempf,
	OBJ_i8k_left_fan_status,
	OBJ_i8k_right_fan_status,
	OBJ_i8k_left_fan_rpm,
	OBJ_i8k_right_fan_rpm,
	OBJ_i8k_ac_status,	
	OBJ_i8k_buttons_status,
#endif /* __linux__ */
	OBJ_if_existing,
	OBJ_if_mounted,
	OBJ_if_running,
	OBJ_top,
	OBJ_top_mem,
	OBJ_tail,
	OBJ_head,
	OBJ_kernel,
	OBJ_loadavg,
	OBJ_machine,
	OBJ_mails,
	OBJ_mem,
	OBJ_membar,
	OBJ_memgraph,
	OBJ_memmax,
	OBJ_memperc,
	OBJ_mixer,
	OBJ_mixerl,
	OBJ_mixerr,
	OBJ_mixerbar,
	OBJ_mixerlbar,
	OBJ_mixerrbar,
	OBJ_new_mails,
	OBJ_nodename,
	OBJ_pre_exec,
#ifdef MLDONKEY
	OBJ_ml_upload_counter,
	OBJ_ml_download_counter,
	OBJ_ml_nshared_files,
	OBJ_ml_shared_counter,
	OBJ_ml_tcp_upload_rate,
	OBJ_ml_tcp_download_rate,
	OBJ_ml_udp_upload_rate,
	OBJ_ml_udp_download_rate,
	OBJ_ml_ndownloaded_files,
	OBJ_ml_ndownloading_files,
#endif
	OBJ_processes,
	OBJ_running_processes,
	OBJ_shadecolor,
	OBJ_outlinecolor,
	OBJ_stippled_hr,
	OBJ_swap,
	OBJ_swapbar,
	OBJ_swapmax,
	OBJ_swapperc,
	OBJ_sysname,
	OBJ_temp1,		/* i2c is used instead in these */
	OBJ_temp2,
	OBJ_text,
	OBJ_time,
	OBJ_utime,
	OBJ_totaldown,
	OBJ_totalup,
	OBJ_updates,
	OBJ_upspeed,
	OBJ_upspeedf,
	OBJ_upspeedgraph,
	OBJ_uptime,
	OBJ_uptime_short,
#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
	OBJ_apm_adapter,
	OBJ_apm_battery_time,
	OBJ_apm_battery_life,
#endif /* __FreeBSD__ */
#ifdef SETI
	OBJ_seti_prog,
	OBJ_seti_progbar,
	OBJ_seti_credit,
#endif
#ifdef MPD
	OBJ_mpd_title,
	OBJ_mpd_artist,
	OBJ_mpd_album,
	OBJ_mpd_random,
	OBJ_mpd_repeat,
	OBJ_mpd_vol,
	OBJ_mpd_bitrate,
	OBJ_mpd_status,
	OBJ_mpd_host,
	OBJ_mpd_port,
	OBJ_mpd_password,
	OBJ_mpd_bar,
	OBJ_mpd_elapsed,
	OBJ_mpd_length,
	OBJ_mpd_track,
	OBJ_mpd_name,
	OBJ_mpd_file,
	OBJ_mpd_percent,
	OBJ_mpd_smart,
#endif
#ifdef XMMS2
	OBJ_xmms2_title,
	OBJ_xmms2_artist,
	OBJ_xmms2_album,
//	OBJ_xmms2_random,
//	OBJ_xmms2_repeat,
//	OBJ_xmms2_vol,
//	OBJ_xmms2_bitrate,
	OBJ_xmms2_status,
	OBJ_xmms2_bar,
	OBJ_xmms2_elapsed,
	OBJ_xmms2_length,
	OBJ_xmms2_track,
//	OBJ_xmms2_name,
	OBJ_xmms2_file,
	OBJ_xmms2_percent,
//	OBJ_xmms2_smart,
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	OBJ_xmms_status,
	OBJ_xmms_title,
	OBJ_xmms_length,
	OBJ_xmms_length_seconds,
	OBJ_xmms_position,
	OBJ_xmms_position_seconds,
	OBJ_xmms_bitrate,
	OBJ_xmms_frequency,
	OBJ_xmms_channels,
	OBJ_xmms_filename,
	OBJ_xmms_playlist_length,
	OBJ_xmms_playlist_position,
	OBJ_xmms_bar,
#endif
#ifdef BMPX
	OBJ_bmpx_title,
	OBJ_bmpx_artist,
	OBJ_bmpx_album,
	OBJ_bmpx_track,
	OBJ_bmpx_uri,
	OBJ_bmpx_bitrate,
#endif
#ifdef TCP_PORT_MONITOR
	OBJ_tcp_portmon,
#endif
};

struct thread_info_s {
	pthread_t thread;
};

struct text_object {
	int type;
	int a, b;
	unsigned int c, d, e;
	float f;
	union {
		char *s;	/* some string */
		int i;		/* some integer */
		long l;		/* some other integer */
		struct net_stat *net;
		struct fs_stat *fs;
		unsigned char loadavg[3];
		unsigned int cpu_index;
		struct {
			struct fs_stat *fs;
			int w, h;
		} fsbar;	/* 3 */

		struct {
			int l;
			int w, h;
		} mixerbar;	/* 3 */

		struct {
			int fd;
			int arg;
			char devtype[256];
			char type[64];
		} i2c;		/* 2 */
		struct {
			int pos;
			char *s;
		} ifblock;
		struct {
			int num;
			int type;
		} top;

		struct {
			int wantedlines;
			int readlines;
			char *logfile;
			double last_update;
			float interval;
			char *buffer;
		} tail;

		struct {
			double last_update;
			float interval;
			char *cmd;
			char *buffer;
			double data;
			int pos;
			struct thread_info_s thread_info;
		} execi;	/* 5 */

		struct {
			int a, b;
		} pair;		/* 2 */
#ifdef TCP_PORT_MONITOR
		struct {
			in_port_t  port_range_begin;  /* starting port to monitor */
			in_port_t  port_range_end;    /* ending port to monitor */
			int        item;              /* enum value from libtcp-portmon.h, e.g. COUNT, REMOTEIP, etc. */
			int        connection_index;  /* 0 to n-1 connections. */
		} tcp_port_monitor;
#endif
	} data;
};

struct text_object_list {
	unsigned int text_object_count;
	struct text_object *text_objects;
};

static unsigned int text_object_count;
static struct text_object *text_objects;
static void generate_text_internal(char *p, int p_max_size, struct text_object *objs, unsigned int object_count, struct information *cur);

#define MAX_THREADS 512 // sure whatever
typedef struct thread_info_s *thread_info;
static thread_info thread_list[MAX_THREADS];
static int thread_count = 0;
static int threads_runnable = 1;

int register_thread(struct thread_info_s *new_thread)
{
	if (thread_count >= MAX_THREADS) {
		CRIT_ERR("Uh oh, tried to register too many threads");
	} else {
		thread_list[thread_count] = new_thread;
		thread_count++;
	}
	return thread_count - 1;
}

void replace_thread(struct thread_info_s *new_thread, int pos) // this isn't even used anymore; oh wells
{
	if (pos >= 0 && pos < MAX_THREADS) {
		thread_list[pos] = new_thread;
	} else {
		ERR("thread position out of bounds");
	}
}

void *threaded_exec(struct text_object *obj) { // pthreads are really beginning to piss me off
	double update_time;
	int run_code = threads_runnable;
	while (threads_runnable == run_code) {
		update_time = get_time();
		char *p2 = obj->data.execi.buffer;
		FILE *fp = popen(obj->data.execi.cmd,"r");
		int n2 = fread(p2, 1, TEXT_BUFFER_SIZE, fp);
		(void) pclose(fp);
		p2[n2] = '\0';
		if (n2 && p2[n2 - 1] == '\n') {
			p2[n2 - 1] = '\0';
		}
		while (*p2) {
			if (*p2 == '\001') {
				*p2 = ' ';
			}
			p2++;
		}
		obj->data.execi.last_update = update_time;
		usleep(100); // prevent race condition
		if (get_time() - obj->data.execi.last_update > obj->data.execi.interval) {
			continue;
		} else {
			unsigned int delay = 1000000.0 * (obj->data.execi.interval -(get_time() - obj->data.execi.last_update));
			if (delay < update_interval * 500000) {
				delay = update_interval * 1000000;
			}
			usleep(delay);
		}
	}
	ERR("exiting thread");
	pthread_exit(NULL);
	return 0;
}

static struct text_object *new_text_object_internal()
{
	struct text_object *obj = malloc(sizeof(struct text_object));
	memset(obj, 0, sizeof(struct text_object));
	return obj;
}

#ifdef MLDONKEY
void ml_cleanup()
{
	if (mlconfig.mldonkey_hostname) {
		free(mlconfig.mldonkey_hostname);
		mlconfig.mldonkey_hostname = NULL;
	}
}
#endif

static void free_text_objects(unsigned int count, struct text_object *objs)
{
	unsigned int i;
	for (i = 0; i < count; i++) {
		switch (objs[i].type) {
			case OBJ_acpitemp:
				close(objs[i].data.i);
				break;
			case OBJ_acpitempf:
				close(objs[i].data.i);
				break;
			case OBJ_i2c:
				close(objs[i].data.i2c.fd);
				break;
			case OBJ_time:
				free(objs[i].data.s);
				break;
			case OBJ_utime:
			case OBJ_if_existing:
			case OBJ_if_mounted:
			case OBJ_if_running:
				free(objs[i].data.ifblock.s);
				break;
			case OBJ_tail:
				free(objs[i].data.tail.logfile);
				free(objs[i].data.tail.buffer);
				break;
			case OBJ_text: case OBJ_font:
				free(objs[i].data.s);
				break;
			case OBJ_exec:
				free(objs[i].data.s);
				break;
			case OBJ_execbar:
				free(objs[i].data.s);
				break;
			case OBJ_execgraph:
				free(objs[i].data.s);
				break;
				/*		case OBJ_execibar:
						free(objs[i].data.s);
						break;
						case OBJ_execigraph:
						free(objs[i].data.s);
						break;*/
#ifdef MPD
			case OBJ_mpd_title:
				if (info.mpd.title) {
					free(info.mpd.title);
					info.mpd.title = 0;
				}
				break;
			case OBJ_mpd_artist:
				if (info.mpd.artist) {
					free(info.mpd.artist);
					info.mpd.artist = 0;
				}
				break;
			case OBJ_mpd_album:
				if (info.mpd.album) {
					free(info.mpd.album);
					info.mpd.album = 0;
				}
				break;
			case OBJ_mpd_random:
				if (info.mpd.random) {
					free(info.mpd.random);
					info.mpd.random = 0;
				}
				break;
			case OBJ_mpd_repeat:
				if (info.mpd.repeat) {
					free(info.mpd.repeat);
					info.mpd.repeat = 0;
				}
				break;
			case OBJ_mpd_track:
				if (info.mpd.track) {
					free(info.mpd.track);
					info.mpd.track = 0;
				}
				break;
			case OBJ_mpd_name:
				if (info.mpd.name) {
					free(info.mpd.name);
					info.mpd.name = 0;
				}
				break;
			case OBJ_mpd_file:
				if (info.mpd.file) {
					free(info.mpd.file);
					info.mpd.file = 0;
				}
				break;
			case OBJ_mpd_status:
				if (info.mpd.status) {
					free(info.mpd.status);
					info.mpd.status = 0;
				}
				break;
			case OBJ_mpd_smart:
				if (info.mpd.artist) {
					free(info.mpd.artist);
					info.mpd.artist = 0;
				}
				if (info.mpd.title) {
					free(info.mpd.title);
					info.mpd.title = 0;
				}
				if (info.mpd.file) {
					free(info.mpd.file);
					info.mpd.file = 0;
				}
				break;
			case OBJ_mpd_host:
#endif
#ifdef XMMS2
			case OBJ_xmms2_title:
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				break;
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
			case OBJ_xmms2_track:
				if (info.xmms2.track) {
					free(info.xmms2.track);
					info.xmms2.track = 0;
				}
				break;
			case OBJ_xmms2_file:
				if (info.xmms2.file) {
					free(info.xmms2.file);
					info.xmms2.file = 0;
				}
				break;
			case OBJ_xmms2_status:
				if (info.xmms2.status) {
					free(info.xmms2.status);
					info.xmms2.status = 0;
				}
				break;
	/*		case OBJ_xmms2_smart:
				if (info.xmms2.artist) {
					free(info.xmms2.artist);
					info.xmms2.artist = 0;
				}
				if (info.xmms2.title) {
					free(info.xmms2.title);
					info.xmms2.title = 0;
				}
				if (info.xmms2.file) {
					free(info.xmms2.file);
					info.xmms2.file = 0;
				}
				break;*/
#endif
#ifdef BMPX
			case OBJ_bmpx_title:
			case OBJ_bmpx_artist:
			case OBJ_bmpx_album:
			case OBJ_bmpx_track:
			case OBJ_bmpx_uri:
			case OBJ_bmpx_bitrate:
#endif
			case OBJ_pre_exec:
			case OBJ_battery:
				free(objs[i].data.s);
				break;

			case OBJ_execi:
				free(objs[i].data.execi.cmd);
				free(objs[i].data.execi.buffer);
				break;
			case OBJ_texeci:
				free(objs[i].data.execi.cmd);
				free(objs[i].data.execi.buffer);
				break;
			case OBJ_top:
				if (info.first_process) {
					free_all_processes();
					info.first_process = NULL;
				}
				break;
			case OBJ_top_mem:
				if (info.first_process) {
					free_all_processes();
					info.first_process = NULL;
				}
				break;
		}
	}
	free(objs);
	//text_objects = NULL;
	//text_object_count = 0;
}

void scan_mixer_bar(const char *arg, int *a, int *w, int *h)
{
	char buf1[64];
	int n;

	if (arg && sscanf(arg, "%63s %n", buf1, &n) >= 1) {
		*a = mixer_init(buf1);
		(void) scan_bar(arg + n, w, h);
	} else {
		*a = mixer_init(0);
		(void) scan_bar(arg, w, h);
	}
}


/* construct_text_object() creates a new text_object */
static struct text_object *construct_text_object(const char *s, const char *arg, unsigned int object_count, struct text_object *text_objects)
{
	//struct text_object *obj = new_text_object();
	struct text_object *obj = new_text_object_internal();

#define OBJ(a, n) if (strcmp(s, #a) == 0) { obj->type = OBJ_##a; need_mask |= (1 << n); {
#define END ; } } else

#ifdef X11	
	if (s[0] == '#') {
		obj->type = OBJ_color;
		obj->data.l = get_x11_color(s);
	} else
#endif /* X11 */
		OBJ(acpitemp, 0) obj->data.i = open_acpi_temperature(arg);
	END OBJ(acpitempf, 0) obj->data.i = open_acpi_temperature(arg);
	END OBJ(acpiacadapter, 0)
		END OBJ(freq, 0);
	END OBJ(freq_g, 0);
	END OBJ(freq_dyn, 0);
	END OBJ(freq_dyn_g, 0);
	END OBJ(acpifan, 0);
	END OBJ(battery, 0);
	char bat[64];
	if (arg)
		sscanf(arg, "%63s", bat);
	else
		strcpy(bat, "BAT0");
	obj->data.s = strdup(bat);
#if defined(__linux__)
	END OBJ(i8k_version, INFO_I8K)
		END OBJ(i8k_bios, INFO_I8K)
		END OBJ(i8k_serial, INFO_I8K)
		END OBJ(i8k_cpu_temp, INFO_I8K)
		END OBJ(i8k_cpu_tempf, INFO_I8K)
		END OBJ(i8k_left_fan_status, INFO_I8K)	
		END OBJ(i8k_right_fan_status, INFO_I8K)
		END OBJ(i8k_left_fan_rpm, INFO_I8K)
		END OBJ(i8k_right_fan_rpm, INFO_I8K)
		END OBJ(i8k_ac_status, INFO_I8K)
		END OBJ(i8k_buttons_status, INFO_I8K)
#endif /* __linux__ */
		END OBJ(buffers, INFO_BUFFERS)
		END OBJ(cached, INFO_BUFFERS)
		END OBJ(cpu, INFO_CPU)
		if (arg) {
			if (strncmp(arg, "cpu", 3) == 0 && isdigit(arg[3])) {
				obj->data.cpu_index = atoi(&arg[3]);
				arg += 4;
			} else {obj->data.cpu_index = 0; }
		} else {
			obj->data.cpu_index = 0;
		}
	END OBJ(cpubar, INFO_CPU)
		if (arg) {
			if (strncmp(arg, "cpu", 3) == 0 && isdigit(arg[3])) {
				obj->data.cpu_index = atoi(&arg[3]);
				arg += 4;
			}
			else {obj->data.cpu_index = 0;}
			(void) scan_bar(arg, &obj->a, &obj->b);
		} else {
			(void) scan_bar(arg, &obj->a, &obj->b);
			obj->data.cpu_index = 0;
		}
	END OBJ(cpugraph, INFO_CPU)
		if (arg) {
			if (strncmp(arg, "cpu", 3) == 0 && isdigit(arg[3])) {
				obj->data.cpu_index = atoi(&arg[3]);
				arg += 4;
			}
			(void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
		} else {
			(void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
			obj->data.cpu_index = 0;
		}
	END OBJ(diskio, INFO_DISKIO)
		END OBJ(diskiograph, INFO_DISKIO) (void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
	END OBJ(color, 0) 
#ifdef X11
		obj->data.l = arg ? get_x11_color(arg) : default_fg_color;
#endif /* X11 */
	END
		OBJ(font, 0)
		obj->data.s = scan_font(arg);
	END
		OBJ(downspeed, INFO_NET) 
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("downspeed needs argument");
		}
	END OBJ(downspeedf, INFO_NET)
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("downspeedf needs argument");
		}
	END OBJ(downspeedgraph, INFO_NET)
		(void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
	char buf[64];
	sscanf(arg, "%63s %*i,%*i %*i", buf);
	obj->data.net = get_net_stat(buf);
	if (sscanf(arg, "%*s %d,%d %*d", &obj->b, &obj->a) <= 1) {
		if (sscanf(arg, "%*s %d,%d", &obj->b, &obj->a) <= 1) {
			obj->a = 0;
			obj->b = 25;
		}
	}
	END OBJ(else, 0)
		if (blockdepth) {
			(text_objects[blockstart[blockdepth - 1]]).data.ifblock.pos = object_count;
			blockstart[blockdepth - 1] = object_count;
			obj->data.ifblock.pos = object_count + 2;
		} else {
			ERR("$else: no matching $if_*");
		}
	END OBJ(endif, 0)
		if (blockdepth) {
			blockdepth--;
			text_objects[blockstart[blockdepth]].data.ifblock.pos = object_count;
		} else {
			ERR("$endif: no matching $if_*");
		}
	END
#ifdef HAVE_POPEN
		OBJ(exec, 0) obj->data.s = strdup(arg ? arg : "");
	END OBJ(execbar, 0) obj->data.s = strdup(arg ? arg : "");
	END OBJ(execgraph, 0) obj->data.s = strdup(arg ? arg : "");
	END OBJ(execibar, 0) unsigned int n;
	if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
		char buf[256];
		ERR("${execibar <interval> command}");
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strdup(buf);
	} else {
		obj->data.execi.cmd = strdup(arg + n);
	}
	END OBJ(execigraph, 0) unsigned int n;
	if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
		char buf[256];
		ERR("${execigraph <interval> command}");
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strdup(buf);
	} else {
		obj->data.execi.cmd = strdup(arg + n);
	}
	END OBJ(execi, 0) unsigned int n;

	if (!arg
			|| sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
		char buf[256];
		ERR("${execi <interval> command}");
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strdup(buf);
	} else {
		obj->data.execi.cmd = strdup(arg + n);
		obj->data.execi.buffer =
			(char *) calloc(1, TEXT_BUFFER_SIZE);
	}
	END OBJ(texeci, 0) unsigned int n;

	if (!arg || sscanf(arg, "%f %n", &obj->data.execi.interval, &n) <= 0) {
		char buf[256];
		ERR("${texeci <interval> command}");
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strdup(buf);
	} else {
		obj->data.execi.cmd = strdup(arg + n);
		obj->data.execi.buffer =
			(char *) calloc(1, TEXT_BUFFER_SIZE);
	}
	obj->data.execi.pos = -1;
	END OBJ(pre_exec, 0) obj->type = OBJ_text;
	if (arg) {
		FILE *fp = popen(arg, "r");
		unsigned int n;
		char buf[2048];

		n = fread(buf, 1, 2048, fp);
		buf[n] = '\0';

		if (n && buf[n - 1] == '\n')
			buf[n - 1] = '\0';

		(void) pclose(fp);

		obj->data.s = strdup(buf);
	} else
		obj->data.s = strdup("");
	END
#endif
		OBJ(fs_bar, INFO_FS) obj->data.fsbar.h = 4;
	arg = scan_bar(arg, &obj->data.fsbar.w, &obj->data.fsbar.h);
	if (arg) {
		while (isspace(*arg))
			arg++;
		if (*arg == '\0')
			arg = "/";
	} else
		arg = "/";
	obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_bar_free, INFO_FS) obj->data.fsbar.h = 4;
	if (arg) {
		unsigned int n;
		if (sscanf(arg, "%d %n", &obj->data.fsbar.h, &n) >= 1)
			arg += n;
	} else
		arg = "/";
	obj->data.fsbar.fs = prepare_fs_stat(arg);
	END OBJ(fs_free, INFO_FS) if (!arg)
		arg = "/";
	obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used_perc, INFO_FS) if (!arg)
		arg = "/";
	obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_free_perc, INFO_FS) if (!arg)
		arg = "/";
	obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_size, INFO_FS) if (!arg)
		arg = "/";
	obj->data.fs = prepare_fs_stat(arg);
	END OBJ(fs_used, INFO_FS) if (!arg)
		arg = "/";
	obj->data.fs = prepare_fs_stat(arg);
	END OBJ(hr, 0) obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(offset, 0) obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(voffset, 0) obj->data.i = arg ? atoi(arg) : 1;
	END OBJ(i2c, INFO_I2C) char buf1[64], buf2[64];
	int n;

	if (!arg) {
		ERR("i2c needs arguments");
		obj->type = OBJ_text;
		//obj->data.s = strdup("${i2c}");
		return NULL;
	}

	if (sscanf(arg, "%63s %63s %d", buf1, buf2, &n) != 3) {
		/* if scanf couldn't read three values, read type and num and use
		 * default device */
		sscanf(arg, "%63s %d", buf2, &n);
		obj->data.i2c.fd =
			open_i2c_sensor(0, buf2, n, &obj->data.i2c.arg,
					obj->data.i2c.devtype);
		strncpy(obj->data.i2c.type, buf2, 63);
	} else {
		obj->data.i2c.fd =
			open_i2c_sensor(buf1, buf2, n, &obj->data.i2c.arg,
					obj->data.i2c.devtype);
		strncpy(obj->data.i2c.type, buf2, 63);
	}

	END OBJ(top, INFO_TOP)
		char buf[64];
	int n;
	if (!arg) {
		ERR("top needs arguments");
		obj->type = OBJ_text;
		//obj->data.s = strdup("${top}");
		return NULL;
	}
	if (sscanf(arg, "%63s %i", buf, &n) == 2) {
		if (strcmp(buf, "name") == 0) {
			obj->data.top.type = TOP_NAME;
		} else if (strcmp(buf, "cpu") == 0) {
			obj->data.top.type = TOP_CPU;
		} else if (strcmp(buf, "pid") == 0) {
			obj->data.top.type = TOP_PID;
		} else if (strcmp(buf, "mem") == 0) {
			obj->data.top.type = TOP_MEM;
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
		obj->data.s = strdup("${top_mem}");
		return NULL;
	}
	if (sscanf(arg, "%63s %i", buf, &n) == 2) {
		if (strcmp(buf, "name") == 0) {
			obj->data.top.type = TOP_NAME;
		} else if (strcmp(buf, "cpu") == 0) {
			obj->data.top.type = TOP_CPU;
		} else if (strcmp(buf, "pid") == 0) {
			obj->data.top.type = TOP_PID;
		} else if (strcmp(buf, "mem") == 0) {
			obj->data.top.type = TOP_MEM;
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
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("addr needs argument");
		}
	END OBJ(linkstatus, INFO_WIFI) 
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("linkstatus needs argument");
		}
	END OBJ(tail, 0)
		char buf[64];
	int n1, n2;
	if (!arg) {
		ERR("tail needs arguments");
		obj->type = OBJ_text;
		obj->data.s = strdup("${tail}");
		return NULL;
	}
	if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 2) {
		if (n1 < 1 || n1 > 30) {
			CRIT_ERR("invalid arg for tail, number of lines must be between 1 and 30");
			return NULL;
		} else {
			FILE *fp = NULL;
			fp = fopen(buf, "r");
			if (fp) {
				obj->data.tail.logfile =
					malloc(TEXT_BUFFER_SIZE);
				strcpy(obj->data.tail.logfile, buf);
				obj->data.tail.wantedlines = n1 - 1;
				obj->data.tail.interval =
					update_interval * 2;
				fclose(fp);
			} else {
				//fclose (fp);
				CRIT_ERR("tail logfile does not exist, or you do not have correct permissions");
			}
		}
	} else if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 3) {
		if (n1 < 1 || n1 > 30) {
			CRIT_ERR
				("invalid arg for tail, number of lines must be between 1 and 30");
			return NULL;
		} else if (n2 < 1 || n2 < update_interval) {
			CRIT_ERR
				("invalid arg for tail, interval must be greater than 0 and Conky's interval");
			return NULL;
		} else {
			FILE *fp;
			fp = fopen(buf, "r");
			if (fp != NULL) {
				obj->data.tail.logfile =
					malloc(TEXT_BUFFER_SIZE);
				strcpy(obj->data.tail.logfile, buf);
				obj->data.tail.wantedlines = n1 - 1;
				obj->data.tail.interval = n2;
				fclose(fp);
			} else {
				//fclose (fp);
				CRIT_ERR("tail logfile does not exist, or you do not have correct permissions");
			}
		}
	}

	else {
		ERR("invalid args given for tail");
		return NULL;
	}
	obj->data.tail.buffer = malloc(TEXT_BUFFER_SIZE * 20); /* asumming all else worked */
	END OBJ(head, 0)
		char buf[64];
	int n1, n2;
	if (!arg) {
		ERR("head needs arguments");
		obj->type = OBJ_text;
		obj->data.s = strdup("${head}");
		return NULL;
	}
	if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 2) {
		if (n1 < 1 || n1 > 30) {
			CRIT_ERR("invalid arg for head, number of lines must be between 1 and 30");
			return NULL;
		} else {
			FILE *fp;
			fp = fopen(buf, "r");
			if (fp != NULL) {
				obj->data.tail.logfile =
					malloc(TEXT_BUFFER_SIZE);
				strcpy(obj->data.tail.logfile, buf);
				obj->data.tail.wantedlines = n1 - 1;
				obj->data.tail.interval =
					update_interval * 2;
				fclose(fp);
			} else {
				//fclose (fp);
				CRIT_ERR("head logfile does not exist, or you do not have correct permissions");
			}
		}
	} else if (sscanf(arg, "%63s %i %i", buf, &n1, &n2) == 3) {
		if (n1 < 1 || n1 > 30) {
			CRIT_ERR
				("invalid arg for head, number of lines must be between 1 and 30");
			return NULL;
		} else if (n2 < 1 || n2 < update_interval) {
			CRIT_ERR
				("invalid arg for head, interval must be greater than 0 and Conky's interval");
			return NULL;
		} else {
			FILE *fp;
			fp = fopen(buf, "r");
			if (fp != NULL) {
				obj->data.tail.logfile =
					malloc(TEXT_BUFFER_SIZE);
				strcpy(obj->data.tail.logfile, buf);
				obj->data.tail.wantedlines = n1 - 1;
				obj->data.tail.interval = n2;
				fclose(fp);
			} else {
				//fclose (fp);
				CRIT_ERR("head logfile does not exist, or you do not have correct permissions");
			}
		}
	}

	else {
		ERR("invalid args given for head");
		return NULL;
	}
	obj->data.tail.buffer = malloc(TEXT_BUFFER_SIZE * 20); /* asumming all else worked */
	END OBJ(loadavg, INFO_LOADAVG) int a = 1, b = 2, c = 3, r = 3;
	if (arg) {
		r = sscanf(arg, "%d %d %d", &a, &b, &c);
		if (r >= 3 && (c < 1 || c > 3))
			r--;
		if (r >= 2 && (b < 1 || b > 3))
			r--, b = c;
		if (r >= 1 && (a < 1 || a > 3))
			r--, a = b, b = c;
	}
	obj->data.loadavg[0] = (r >= 1) ? (unsigned char) a : 0;
	obj->data.loadavg[1] = (r >= 2) ? (unsigned char) b : 0;
	obj->data.loadavg[2] = (r >= 3) ? (unsigned char) c : 0;
	END OBJ(if_existing, 0)
		if (blockdepth >= MAX_IF_BLOCK_DEPTH) {
			CRIT_ERR("MAX_IF_BLOCK_DEPTH exceeded");
		}
	if (!arg) {
		ERR("if_existing needs an argument");
		obj->data.ifblock.s = 0;
	} else
		obj->data.ifblock.s = strdup(arg);
	blockstart[blockdepth] = object_count;
	obj->data.ifblock.pos = object_count + 2;
	blockdepth++;
	END OBJ(if_mounted, 0)
		if (blockdepth >= MAX_IF_BLOCK_DEPTH) {
			CRIT_ERR("MAX_IF_BLOCK_DEPTH exceeded");
		}
	if (!arg) {
		ERR("if_mounted needs an argument");
		obj->data.ifblock.s = 0;
	} else
		obj->data.ifblock.s = strdup(arg);
	blockstart[blockdepth] = object_count;
	obj->data.ifblock.pos = object_count + 2;
	blockdepth++;
	END OBJ(if_running, 0)
		if (blockdepth >= MAX_IF_BLOCK_DEPTH) {
			CRIT_ERR("MAX_IF_BLOCK_DEPTH exceeded");
		}
	if (arg) {
		char buf[256];
		snprintf(buf, 256, "pidof %s >/dev/null", arg);
		obj->data.ifblock.s = strdup(buf);
	} else {
		ERR("if_running needs an argument");
		obj->data.ifblock.s = 0;
	}
	blockstart[blockdepth] = object_count;
	obj->data.ifblock.pos = object_count + 2;
	blockdepth++;
	END OBJ(kernel, 0)
		END OBJ(machine, 0)
		END OBJ(mails, INFO_MAIL)
		END OBJ(mem, INFO_MEM)
		END OBJ(memmax, INFO_MEM)
		END OBJ(memperc, INFO_MEM)
		END OBJ(membar, INFO_MEM)
		(void) scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(memgraph, INFO_MEM)
		(void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
	END OBJ(mixer, INFO_MIXER) obj->data.l = mixer_init(arg);
	END OBJ(mixerl, INFO_MIXER) obj->data.l = mixer_init(arg);
	END OBJ(mixerr, INFO_MIXER) obj->data.l = mixer_init(arg);
	END OBJ(mixerbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l,
				&obj->data.mixerbar.w, &obj->data.mixerbar.h);
	END OBJ(mixerlbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l,
				&obj->data.mixerbar.w, &obj->data.mixerbar.h);
	END OBJ(mixerrbar, INFO_MIXER)
		scan_mixer_bar(arg, &obj->data.mixerbar.l,
				&obj->data.mixerbar.w, &obj->data.mixerbar.h);
	END
#ifdef MLDONKEY
		OBJ(ml_upload_counter, INFO_MLDONKEY)
		END OBJ(ml_download_counter, INFO_MLDONKEY)
		END OBJ(ml_nshared_files, INFO_MLDONKEY)
		END OBJ(ml_shared_counter, INFO_MLDONKEY)
		END OBJ(ml_tcp_upload_rate, INFO_MLDONKEY)
		END OBJ(ml_tcp_download_rate, INFO_MLDONKEY)
		END OBJ(ml_udp_upload_rate, INFO_MLDONKEY)
		END OBJ(ml_udp_download_rate, INFO_MLDONKEY)
		END OBJ(ml_ndownloaded_files, INFO_MLDONKEY)
		END OBJ(ml_ndownloading_files, INFO_MLDONKEY) END
#endif
		OBJ(new_mails, INFO_MAIL)
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
		if (sscanf(arg, "%d %d", &a, &b) != 2)
			sscanf(arg, "%d", &b);
	}
	if (a <= 0)
		a = 1;
	obj->data.pair.a = a;
	obj->data.pair.b = b;
#endif /* X11 */
	END OBJ(swap, INFO_MEM)
		END OBJ(swapmax, INFO_MEM)
		END OBJ(swapperc, INFO_MEM)
		END OBJ(swapbar, INFO_MEM)
		(void) scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(sysname, 0) END OBJ(temp1, INFO_I2C) obj->type = OBJ_i2c;
	obj->data.i2c.fd =
		open_i2c_sensor(0, "temp", 1, &obj->data.i2c.arg,
				obj->data.i2c.devtype);
	END OBJ(temp2, INFO_I2C) obj->type = OBJ_i2c;
	obj->data.i2c.fd =
		open_i2c_sensor(0, "temp", 2, &obj->data.i2c.arg,
				obj->data.i2c.devtype);
	END OBJ(time, 0) obj->data.s = strdup(arg ? arg : "%F %T");
	END OBJ(utime, 0) obj->data.s = strdup(arg ? arg : "%F %T");
	END OBJ(totaldown, INFO_NET)
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("totaldown needs argument");
		}
	END OBJ(totalup, INFO_NET) obj->data.net = get_net_stat(arg);
	if(arg) {
		obj->data.net = get_net_stat(arg);
	}
	else {
		CRIT_ERR("totalup needs argument");
	}
	END OBJ(updates, 0)
		END OBJ(alignr, 0) obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(alignc, 0) obj->data.i = arg ? atoi(arg) : 0;
	END OBJ(upspeed, INFO_NET)
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("upspeed needs argument");
		}
	END OBJ(upspeedf, INFO_NET) 
		if(arg) {
			obj->data.net = get_net_stat(arg);
		}
		else {
			CRIT_ERR("upspeedf needs argument");
		}

	END OBJ(upspeedgraph, INFO_NET)
		(void) scan_graph(arg, &obj->a, &obj->b, &obj->c, &obj->d, &obj->e);
	char buf[64];
	sscanf(arg, "%63s %*i,%*i %*i", buf);
	obj->data.net = get_net_stat(buf);
	if (sscanf(arg, "%*s %d,%d %*d", &obj->b, &obj->a) <= 1) {
		if (sscanf(arg, "%*s %d,%d", &obj->a, &obj->a) <= 1) {
			obj->a = 0;
			obj->b = 25;
		}
	}
	END OBJ(uptime_short, INFO_UPTIME) END OBJ(uptime, INFO_UPTIME) END
		OBJ(adt746xcpu, 0) END OBJ(adt746xfan, 0) END
#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
		OBJ(apm_adapter, 0) END
		OBJ(apm_battery_life, 0) END
		OBJ(apm_battery_time, 0) END
#endif /* __FreeBSD__ */
#ifdef SETI
		OBJ(seti_prog, INFO_SETI) END OBJ(seti_progbar, INFO_SETI)
		(void) scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
	END OBJ(seti_credit, INFO_SETI) END
#endif
#ifdef MPD
		OBJ(mpd_artist, INFO_MPD)
		END OBJ(mpd_title, INFO_MPD)
		END OBJ(mpd_random, INFO_MPD)
		END OBJ(mpd_repeat, INFO_MPD)
		END OBJ(mpd_elapsed, INFO_MPD)
		END OBJ(mpd_length, INFO_MPD)
		END OBJ(mpd_track, INFO_MPD)
		END OBJ(mpd_name, INFO_MPD)
		END OBJ(mpd_file, INFO_MPD)
		END OBJ(mpd_percent, INFO_MPD)
		END OBJ(mpd_album, INFO_MPD)
		END OBJ(mpd_vol, INFO_MPD)
		END OBJ(mpd_bitrate, INFO_MPD)
		END OBJ(mpd_status, INFO_MPD)
		END OBJ(mpd_bar, INFO_MPD)
		(void) scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
		END OBJ(mpd_smart, INFO_MPD) END
#endif
#ifdef XMMS2
		OBJ(xmms2_artist, INFO_XMMS2)
		END OBJ(xmms2_title, INFO_XMMS2)
		END OBJ(xmms2_length, INFO_XMMS2)
		END OBJ(xmms2_track, INFO_XMMS2)
		END OBJ(xmms2_file, INFO_XMMS2)
		END OBJ(xmms2_album, INFO_XMMS2)
		END OBJ(xmms2_status, INFO_XMMS2)
		END OBJ(xmms2_elapsed, INFO_XMMS2)
		END OBJ(xmms2_length, INFO_XMMS2)
		END OBJ(xmms2_percent, INFO_XMMS2)
		END OBJ(xmms2_bar, INFO_XMMS2)
		(void) scan_bar(arg, &obj->data.pair.a, &obj->data.pair.b);
		END
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
		OBJ(xmms_status, INFO_XMMS) END
		OBJ(xmms_title, INFO_XMMS) END
		OBJ(xmms_length, INFO_XMMS) END
		OBJ(xmms_length_seconds, INFO_XMMS) END
		OBJ(xmms_position, INFO_XMMS) END
		OBJ(xmms_position_seconds, INFO_XMMS) END
		OBJ(xmms_bitrate, INFO_XMMS) END
		OBJ(xmms_frequency, INFO_XMMS) END
		OBJ(xmms_channels, INFO_XMMS) END
		OBJ(xmms_filename, INFO_XMMS) END
		OBJ(xmms_playlist_length, INFO_XMMS) END
		OBJ(xmms_playlist_position, INFO_XMMS) END
		OBJ(xmms_bar, INFO_XMMS)
		(void) scan_bar(arg, &obj->a, &obj->b);
	END
#endif
#ifdef BMPX
		OBJ(bmpx_title, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
		OBJ(bmpx_artist, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
		OBJ(bmpx_album, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
		OBJ(bmpx_track, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
		OBJ(bmpx_uri, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
		OBJ(bmpx_bitrate, INFO_BMPX)
		memset(&(info.bmpx), 0, sizeof(struct bmpx_s));
	END
#endif
#ifdef TCP_PORT_MONITOR
		OBJ(tcp_portmon, INFO_TCP_PORT_MONITOR) 
		int argc, port_begin, port_end, item, connection_index;
	char itembuf[32];
	memset(itembuf,0,sizeof(itembuf));
	connection_index=0;
	/* massive argument checking */
	if (!arg) {
		CRIT_ERR("tcp_portmon: needs arguments");
	}
	argc=sscanf(arg, "%d %d %31s %d", &port_begin, &port_end, itembuf, &connection_index);
	if ( (argc != 3) && (argc != 4) ) 
	{
		CRIT_ERR("tcp_portmon: requires 3 or 4 arguments");
	}
	if ( (port_begin<1) || (port_begin>65535) || (port_end<1) || (port_end>65535) )
	{
		CRIT_ERR("tcp_portmon: port values must be from 1 to 65535");
	}
	if ( port_begin > port_end )
	{
		CRIT_ERR("tcp_portmon: starting port must be <= ending port");
	}
	if ( strncmp(itembuf,"count",31) == 0 )
		item=COUNT;
	else if ( strncmp(itembuf,"rip",31) == 0 )
		item=REMOTEIP;
	else if ( strncmp(itembuf,"rhost",31) == 0 )
		item=REMOTEHOST;
	else if ( strncmp(itembuf,"rport",31) == 0 )
		item=REMOTEPORT;
	else if ( strncmp(itembuf,"rservice",31) == 0 )
		item=REMOTESERVICE;
	else if ( strncmp(itembuf,"lip",31) == 0 )
		item=LOCALIP;
	else if ( strncmp(itembuf,"lhost",31) == 0 )
		item=LOCALHOST;
	else if ( strncmp(itembuf,"lport",31) == 0 )
		item=LOCALPORT;
	else if ( strncmp(itembuf,"lservice",31) == 0 )
		item=LOCALSERVICE;
	else
	{
		CRIT_ERR("tcp_portmon: invalid item specified"); 
	}
	if ( (argc==3) && (item!=COUNT) )
	{
		CRIT_ERR("tcp_portmon: 3 argument form valid only for \"count\" item");
	}
	if ( (argc==4) && (connection_index<0) )
	{
		CRIT_ERR("tcp_portmon: connection index must be non-negative");
	}
	/* ok, args looks good. save the text object data */
	obj->data.tcp_port_monitor.port_range_begin = (in_addr_t)port_begin;
	obj->data.tcp_port_monitor.port_range_end = (in_addr_t)port_end;
	obj->data.tcp_port_monitor.item = item;
	obj->data.tcp_port_monitor.connection_index = connection_index;

	/* if the port monitor collection hasn't been created, we must create it */
	if ( !info.p_tcp_port_monitor_collection )
	{
		info.p_tcp_port_monitor_collection = 
			create_tcp_port_monitor_collection( &tcp_port_monitor_collection_args );
		if ( !info.p_tcp_port_monitor_collection )
		{
			CRIT_ERR("tcp_portmon: unable to create port monitor collection");
		}
	}

	/* if a port monitor for this port does not exist, create one and add it to the collection */
	if ( find_tcp_port_monitor( info.p_tcp_port_monitor_collection, port_begin, port_end ) == NULL )
	{
		tcp_port_monitor_t * p_monitor = 
			create_tcp_port_monitor( port_begin, port_end, &tcp_port_monitor_args );
		if ( !p_monitor )
		{
			CRIT_ERR("tcp_portmon: unable to create port monitor");
		}
		/* add the newly created monitor to the collection */
		if ( insert_tcp_port_monitor_into_collection( info.p_tcp_port_monitor_collection,
					p_monitor ) != 0 )
		{
			CRIT_ERR("tcp_portmon: unable to add port monitor to collection");
		}
	}
	END
#endif
	{
		char buf[256];
		ERR("unknown variable %s", s);
		obj->type = OBJ_text;
		snprintf(buf, 256, "${%s}", s);
		obj->data.s = strdup(buf);
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
	obj->data.s = strdup(s);
	return obj;
}

static struct text_object_list *extract_variable_text_internal(const char *p)
{
	struct text_object_list *retval;
	struct text_object *obj;
	const char *s = p;

	retval = malloc(sizeof(struct text_object_list));
	memset(retval, 0, sizeof(struct text_object_list));
	retval->text_object_count = 0;

	while (*p) {
		if (*p == '$') {
			*(char *) p = '\0';
			obj = create_plain_text(s);
			if(obj != NULL) {
				// allocate memory for the object
				retval->text_objects = realloc(retval->text_objects, 
						sizeof(struct text_object) * (retval->text_object_count+1));
				// assign the new object to the end of the list.
				memcpy(&retval->text_objects[retval->text_object_count++],
						obj, sizeof(struct text_object));
				free(obj);
			}
			*(char *) p = '$';
			p++;
			s = p;

			if (*p != '$') {
				char buf[256];
				const char *var;
				unsigned int len;

				/* variable is either $foo or ${foo} */
				if (*p == '{') {
					p++;
					s = p;
					while (*p && *p != '}')
						p++;
				} else {
					s = p;
					if (*p == '#')
						p++;
					while (*p && (isalnum((int) *p)
								|| *p == '_'))
						p++;
				}

				/* copy variable to buffer */
				len = (p - s > 255) ? 255 : (p - s);
				strncpy(buf, s, len);
				buf[len] = '\0';

				if (*p == '}')
					p++;
				s = p;

				var = getenv(buf);

				/* if variable wasn't found from environment, use some special */
				if (!var) {
					char *p;
					char *arg = 0;

					/* split arg */
					if (strchr(buf, ' ')) {
						arg = strchr(buf, ' ');
						*arg = '\0';
						arg++;
						while (isspace((int) *arg))
							arg++;
						if (!*arg)
							arg = 0;
					}

					/* lowercase variable name */
					p = buf;
					while (*p) {
						*p = tolower(*p);
						p++;
					}

					// create new object
					obj = construct_text_object(buf, arg, retval->text_object_count, retval->text_objects);
					if(obj != NULL) {
						// allocate memory for the object
						retval->text_objects = realloc(retval->text_objects, 
								sizeof(struct text_object) * (retval->text_object_count+1));
						// assign the new object to the end of the list.
						memcpy(&retval->text_objects[retval->text_object_count++],
								obj, sizeof(struct text_object));
						free(obj);
					}
				}
				continue;
			} else {
				obj = create_plain_text("$");
				if(obj != NULL) {
					// allocate memory for the object
					retval->text_objects = realloc(retval->text_objects, 
							sizeof(struct text_object) * (retval->text_object_count+1));
					// assign the new object to the end of the list.
					memcpy(&retval->text_objects[retval->text_object_count++],
							obj, sizeof(struct text_object));
					free(obj);
				}
			}
		}
		p++;
	}
	obj = create_plain_text(s);
	if(obj != NULL) {
		// allocate memory for the object
		retval->text_objects = realloc(retval->text_objects,
				sizeof(struct text_object) * (retval->text_object_count+1));
		// assign the new object to the end of the list.
		memcpy(&retval->text_objects[retval->text_object_count++],
				obj, sizeof(struct text_object));
		free(obj);
	}

	if (blockdepth) {
		ERR("one or more $endif's are missing");
	}

	return retval;
}

static void extract_variable_text(const char *p)
{
	struct text_object_list *list;

	free_text_objects(text_object_count, text_objects);
	text_object_count = 0;
	text_objects = NULL;

#ifdef MLDONKEY	
	ml_cleanup();
#endif /* MLDONKEY */

	list = extract_variable_text_internal(p);
	text_objects = list->text_objects;
	text_object_count = list->text_object_count;

	free(list);

	return;
}

void parse_conky_vars(char * text, char * p, struct information *cur) { 
	struct text_object_list *object_list = extract_variable_text_internal(text);
	generate_text_internal(p, P_MAX_SIZE, object_list->text_objects, object_list->text_object_count, cur);
	free(object_list);
}

static void generate_text_internal(char *p, int p_max_size, struct text_object *objs, unsigned int object_count, struct information *cur)
{
	unsigned int i;
	for (i = 0; i < object_count; i++) {
		struct text_object *obj = &objs[i];

#define OBJ(a) break; case OBJ_##a:

		switch (obj->type) {
			default:
				{
					ERR("not implemented obj type %d",
							obj->type);
				}
				OBJ(acpitemp) {
					/* does anyone have decimals in acpi temperature? */
					if (!use_spacer)
						snprintf(p, p_max_size, "%d", (int)
								get_acpi_temperature(obj->
									data.
									i));
					else
						snprintf(p, 5, "%d    ", (int)
								get_acpi_temperature(obj->
									data.
									i));
				}
				OBJ(acpitempf) {
					/* does anyone have decimals in acpi temperature? */
					if (!use_spacer)
						snprintf(p, p_max_size, "%d", (int)
								((get_acpi_temperature(obj->
										       data.
										       i)+ 40) * 9.0 / 5 - 40));
					else
						snprintf(p, 5, "%d    ", (int)
								((get_acpi_temperature(obj->
										       data.
										       i)+ 40) * 9.0 / 5 - 40));
				}
				OBJ(freq) {
					get_freq(p, p_max_size, "%.0f", 1); /* pk */
				}
				OBJ(freq_g) {
					get_freq(p, p_max_size, "%'.2f", 1000); /* pk */
				}
				OBJ(freq_dyn) {
					if (use_spacer) {
						get_freq_dynamic(p, 6, "%.0f     ", 1 ); /* pk */
					} else {
						get_freq_dynamic(p, p_max_size, "%.0f", 1 ); /* pk */
					}
				}
				OBJ(freq_dyn_g) {
					if (use_spacer) {
						get_freq_dynamic(p, 6, "%'.2f     ", 1000); /* pk */
					} else {
						get_freq_dynamic(p, p_max_size, "%'.2f", 1000); /* pk */
					}
				}
				OBJ(adt746xcpu) {
					get_adt746x_cpu(p, p_max_size); /* pk */
				}
				OBJ(adt746xfan) {
					get_adt746x_fan(p, p_max_size); /* pk */
				}
				OBJ(acpifan) {
					get_acpi_fan(p, p_max_size);  /* pk */
				}
				OBJ(acpiacadapter) {
					get_acpi_ac_adapter(p, p_max_size); /* pk */
				}
				OBJ(battery) {
					get_battery_stuff(p, p_max_size, obj->data.s);
				}
				OBJ(buffers) {
					human_readable(cur->buffers * 1024, p, 255);
				}
				OBJ(cached) {
					human_readable(cur->cached * 1024, p, 255);
				}
				OBJ(cpu) {
					if (obj->data.cpu_index > info.cpu_count) {
						printf("obj->data.cpu_index %i info.cpu_count %i", obj->data.cpu_index, info.cpu_count);
						CRIT_ERR("attempting to use more CPUs then you have!");
					}
					if (!use_spacer)
						snprintf(p, p_max_size, "%*d", pad_percents,
								(int) round_to_int(cur->cpu_usage[obj->data.cpu_index] *
										   100.0));
					else
						snprintf(p, 4, "%*d    ",
								pad_percents,
								(int) round_to_int(cur->cpu_usage[obj->data.cpu_index] *
										   100.0));
				}
				OBJ(cpubar) {
					new_bar(p, obj->a,
							obj->b,
							(int) round_to_int(cur->cpu_usage[obj->data.cpu_index] * 255.0));
				}
				OBJ(cpugraph) {
					new_graph(p, obj->a,
							obj->b, obj->c, obj->d,
							(unsigned int) round_to_int(cur->cpu_usage[obj->data.cpu_index] *
										    100), 100, 1);
				}
				OBJ(color) {
					new_fg(p, obj->data.l);
				}
#if defined(__linux__)
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
					snprintf(p, p_max_size, "%s", i8k.cpu_temp);
				}
				OBJ(i8k_cpu_tempf) { 
					int cpu_temp;
					sscanf(i8k.cpu_temp, "%d", &cpu_temp);
					snprintf(p, p_max_size, "%.1f", cpu_temp*(9.0/5.0)+32.0);
				}
				OBJ(i8k_left_fan_status) { 
					int left_fan_status;
					sscanf(i8k.left_fan_status, "%d", &left_fan_status);
					if(left_fan_status == 0) {
						snprintf(p, p_max_size,"off");
					} if(left_fan_status == 1) {
						snprintf(p, p_max_size, "low");
					}	if(left_fan_status == 2) {
						snprintf(p, p_max_size, "high");
					}

				}
				OBJ(i8k_right_fan_status) { 
					int right_fan_status;
					sscanf(i8k.right_fan_status, "%d", &right_fan_status);
					if(right_fan_status == 0) {
						snprintf(p, p_max_size,"off");
					} if(right_fan_status == 1) {
						snprintf(p, p_max_size, "low");
					}	if(right_fan_status == 2) {
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
					if(ac_status == -1) {
						snprintf(p, p_max_size,"disabled (read i8k docs)");
					} if(ac_status == 0) {
						snprintf(p, p_max_size, "off");
					}	if(ac_status == 1) {
						snprintf(p, p_max_size, "on");
					}
				}
				OBJ(i8k_buttons_status) {
					snprintf(p, p_max_size, "%s", i8k.buttons_status); 

				}
#endif /* __linux__ */

#ifdef X11
				OBJ(font) {
					new_font(p, obj->data.s);
				}
#endif /* X11 */
				OBJ(diskio) {
					if (!use_spacer) {
						if (diskio_value > 1024*1024) {
							snprintf(p, p_max_size, "%.1fG",
									(double)diskio_value/1024/1024);
						} else if (diskio_value > 1024) {
							snprintf(p, p_max_size, "%.1fM",
									(double)diskio_value/1024);
						} else if (diskio_value > 0) {
							snprintf(p, p_max_size, "%dK", diskio_value);
						} else {
							snprintf(p, p_max_size, "%d", diskio_value);
						}
					} else {
						if (diskio_value > 1024*1024) {
							snprintf(p, 6, "%.1fG   ",
									(double)diskio_value/1024/1024);
						} else if (diskio_value > 1024) {
							snprintf(p, 6, "%.1fM   ",
									(double)diskio_value/1024);
						} else if (diskio_value > 0) {
							snprintf(p, 6, "%dK ", diskio_value);
						} else {
							snprintf(p, 6, "%d     ", diskio_value);
						}
					}
				}
				OBJ(diskiograph) {
					new_graph(p, obj->a,
							obj->b, obj->c, obj->d,
							diskio_value, obj->e, 1);
				}

				OBJ(downspeed) {
					if (!use_spacer) {
						snprintf(p, p_max_size, "%d",
								(int) (obj->data.net->
								       recv_speed /
								       1024));
					} else
						snprintf(p, 6, "%d     ",
								(int) (obj->data.net->
								       recv_speed /
								       1024));
				}
				OBJ(downspeedf) {
					if (!use_spacer)
						snprintf(p, p_max_size, "%.1f",
								obj->data.net->
								recv_speed / 1024.0);
					else
						snprintf(p, 8, "%.1f       ",
								obj->data.net->
								recv_speed / 1024.0);
				}
				OBJ(downspeedgraph) {
					if (obj->data.net->recv_speed == 0)	// this is just to make the ugliness at start go away
						obj->data.net->recv_speed = 0.01;
					new_graph(p, obj->a, obj->b, obj->c, obj->d,
							(obj->data.net->recv_speed /
							 1024.0), obj->e, 1);
				}
				OBJ(
						else
				   ) {
					if (!if_jumped) {
						i = obj->data.ifblock.pos - 2;
					} else {
						if_jumped = 0;
					}
				}
				OBJ(endif) {
					if_jumped = 0;
				}
#ifdef HAVE_POPEN
				OBJ(addr) {
					snprintf(p, p_max_size, "%u.%u.%u.%u",
							obj->data.net->addr.
							sa_data[2] & 255,
							obj->data.net->addr.
							sa_data[3] & 255,
							obj->data.net->addr.
							sa_data[4] & 255,
							obj->data.net->addr.
							sa_data[5] & 255);

				}
				OBJ(linkstatus) {
					snprintf(p, p_max_size, "%d",
							obj->data.net->linkstatus);
				}

				OBJ(exec) {
					FILE *fp = popen(obj->data.s, "r");
					int length = fread(p, 1, p_max_size, fp);
					(void) pclose(fp);

					/*output[length] = '\0';
					  if (length > 0 && output[length - 1] == '\n') {
					  output[length - 1] = '\0';
					  }*/
					p[length] = '\0';
					if (length > 0 && p[length - 1] == '\n') {
						p[length - 1] = '\0';
					}

					//parse_conky_vars(output, p, cur);
				}
				OBJ(execbar) {
					char *p2 = p;
					FILE *fp = popen(obj->data.s, "r");
					int n2 = fread(p, 1, p_max_size, fp);
					(void) pclose(fp);

					p[n2] = '\0';
					if (n2 && p[n2 - 1] == '\n')
						p[n2 - 1] = '\0';

					while (*p2) {
						if (*p2 == '\001')
							*p2 = ' ';
						p2++;
					}
					double barnum;
					if (sscanf(p, "%lf", &barnum) == 0) {
						ERR("reading execbar value failed (perhaps it's not the correct format?)");
					}
					if (barnum > 100 || barnum < 0) {
						ERR("your execbar value is not between 0 and 100, therefore it will be ignored");
					} else {
						barnum = barnum / 100.0;
						new_bar(p, 0, 4, (int) (barnum * 255.0));
					}

				}
				OBJ(execgraph) {
					char *p2 = p;
					FILE *fp = popen(obj->data.s, "r");
					int n2 = fread(p, 1, p_max_size, fp);
					(void) pclose(fp);

					p[n2] = '\0';
					if (n2 && p[n2 - 1] == '\n')
						p[n2 - 1] = '\0';

					while (*p2) {
						if (*p2 == '\001')
							*p2 = ' ';
						p2++;
					}
					double barnum;
					if (sscanf(p, "%lf", &barnum) == 0) {
						ERR("reading execgraph value failed (perhaps it's not the correct format?)");
					}
					if (barnum > 100 || barnum < 0) {
						ERR("your execgraph value is not between 0 and 100, therefore it will be ignored");
					} else {
						new_graph(p, 0, 25, obj->c, obj->d, (int) (barnum), obj->e, 1);
					}

				}
				OBJ(execibar) {
					if (current_update_time - obj->data.execi.last_update < obj->data.execi.interval) {
						new_bar(p, 0, 4, (int) obj->f);
					} else {
						char *p2 = p;
						FILE *fp = popen(obj->data.execi.cmd, "r");
						int n2 = fread(p, 1, p_max_size, fp);
						(void) pclose(fp);
						p[n2] = '\0';
						if (n2 && p[n2 - 1] == '\n')
							p[n2 - 1] = '\0';

						while (*p2) {
							if (*p2 == '\001')
								*p2 = ' ';
							p2++;
						}
						float barnum;
						if (sscanf(p, "%f", &barnum) == 0) {
							ERR("reading execibar value failed (perhaps it's not the correct format?)");
						}
						if (barnum > 100 || barnum < 0) {
							ERR("your execibar value is not between 0 and 100, therefore it will be ignored");
						} else {
							obj->f = 255 * barnum / 100.0;
							new_bar(p, 0, 4, (int) obj->f);
						}
						obj->data.execi.last_update =
							current_update_time;
					}
				}
				OBJ(execigraph) {
					if (current_update_time - obj->data.execi.last_update < obj->data.execi.interval) {
						new_graph(p, 0,	25, obj->c, obj->d, (int) (obj->f), 100, 0);
					} else {
						char *p2 = p;
						FILE *fp = popen(obj->data.execi.cmd, "r");
						int n2 = fread(p, 1, p_max_size, fp);
						(void) pclose(fp);
						p[n2] = '\0';
						if (n2 && p[n2 - 1] == '\n')
							p[n2 - 1] = '\0';

						while (*p2) {
							if (*p2 == '\001')
								*p2 = ' ';
							p2++;
						}
						float barnum;
						if (sscanf(p, "%f", &barnum) == 0) {
							ERR("reading execigraph value failed (perhaps it's not the correct format?)");
						}
						if (barnum > 100 || barnum < 0) {
							ERR("your execigraph value is not between 0 and 100, therefore it will be ignored");
						} else {
							obj->f = barnum;
							new_graph(p, 0,	25, obj->c, obj->d, (int) (obj->f), 100, 1);
						}
						obj->data.execi.last_update = current_update_time;

					}

				}
				OBJ(execi) {
					if (current_update_time - obj->data.execi.last_update < obj->data.execi.interval || obj->data.execi.interval == 0) {
						snprintf(p, p_max_size, "%s", obj->data.execi.buffer);
					} else {
						char *output = obj->data.execi.buffer;
						FILE *fp = popen(obj->data.execi.cmd, "r");
						//int length = fread(output, 1, TEXT_BUFFER_SIZE, fp);
						int length = fread(output, 1, TEXT_BUFFER_SIZE, fp);
						(void) pclose(fp);

						output[length] = '\0';
						if (length > 0 && output[length - 1] == '\n') {
							output[length - 1] = '\0';
						}
						obj->data.execi.last_update = current_update_time;
						snprintf(p, p_max_size, "%s", output);
					}
					//parse_conky_vars(output, p, cur);
				}
				OBJ(texeci) {
					if (obj->data.execi.pos < 0) {
						obj->data.execi.last_update = current_update_time;
						if (pthread_create(&(obj->data.execi.thread_info.thread), NULL, (void*)threaded_exec, (void*) obj)) {
							ERR("Error starting thread");
						}
						obj->data.execi.pos = register_thread(&(obj->data.execi.thread_info));
					}
					snprintf(p, p_max_size, "%s", obj->data.execi.buffer);
				}
#endif
			OBJ(fs_bar) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0)
						new_bar(p,
							obj->data.fsbar.w,
							obj->data.fsbar.h,
							255);
					else
						new_bar(p,
							obj->data.fsbar.w,
							obj->data.fsbar.h,
							(int) (255 -
							       obj->data.
							       fsbar.fs->
							       avail *
							       255 /
							       obj->data.
							       fs->size));
				}
			}
			OBJ(fs_free) {
				if (obj->data.fs != NULL)
					human_readable(obj->data.fs->avail,
						       p, 255);
			}
			OBJ(fs_free_perc) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size)
						snprintf(p, p_max_size, "%*d",
							 pad_percents,
							 (int) ((obj->data.
								 fs->
								 avail *
								 100) /
								obj->data.
								fs->size));
					else
						snprintf(p, p_max_size, "0");
				}
			}
			OBJ(fs_size) {
				if (obj->data.fs != NULL)
					human_readable(obj->data.fs->size,
						       p, 255);
			}
			OBJ(fs_used) {
				if (obj->data.fs != NULL)
					human_readable(obj->data.fs->size -
						       (obj->data.fs->free ? obj->data.fs->free :obj->data.fs->avail),
						       p, 255);
			}
			OBJ(fs_bar_free) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size == 0)
						new_bar(p,
							obj->data.fsbar.w,
							obj->data.fsbar.h,
							255);
					else
						new_bar(p,
							obj->data.fsbar.w,
							obj->data.fsbar.h,
							(int) (obj->data.
							       fsbar.fs->
							       avail *
							       255 /
							       obj->data.
							       fs->size));
				}
			}
			OBJ(fs_used_perc) {
				if (obj->data.fs != NULL) {
					if (obj->data.fs->size)
						snprintf(p, 4, "%d",
							 100 - ((int)
								((obj->
								  data.fs->
								  avail *
								  100) /
								 obj->data.
								 fs->
								 size)));
					else
						snprintf(p, p_max_size, "0");
				}
			}
			OBJ(loadavg) {
				float *v = info.loadavg;

				if (obj->data.loadavg[2])
					snprintf(p, p_max_size, "%.2f %.2f %.2f",
						 v[obj->data.loadavg[0] -
						   1],
						 v[obj->data.loadavg[1] -
						   1],
						 v[obj->data.loadavg[2] -
						   1]);
				else if (obj->data.loadavg[1])
					snprintf(p, p_max_size, "%.2f %.2f",
						 v[obj->data.loadavg[0] -
						   1],
						 v[obj->data.loadavg[1] -
						   1]);
				else if (obj->data.loadavg[0])
					snprintf(p, p_max_size, "%.2f",
						 v[obj->data.loadavg[0] -
						   1]);
			}
			OBJ(hr) {
				new_hr(p, obj->data.i);
			}
			OBJ(offset) {
				new_offset(p, obj->data.i);
			}
			OBJ(voffset) {
				new_voffset(p, obj->data.i);
			}
			OBJ(i2c) {
				double r;

				r = get_i2c_info(&obj->data.i2c.fd,
						 obj->data.i2c.arg,
						 obj->data.i2c.devtype,
						 obj->data.i2c.type);

				if (r >= 100.0 || r == 0)
					snprintf(p, p_max_size, "%d", (int) r);
				else
					snprintf(p, p_max_size, "%.1f", r);
			}
			OBJ(alignr) {
				new_alignr(p, obj->data.i);
			}
			OBJ(alignc) {
				new_alignc(p, obj->data.i);
			}
			OBJ(if_existing) {
				struct stat tmp;
				if ((obj->data.ifblock.s)
				    && (stat(obj->data.ifblock.s, &tmp) ==
					-1)) {
					i = obj->data.ifblock.pos;
					if_jumped = 1;
				} else {
					if_jumped = 0;
				}
			}
			OBJ(if_mounted) {
				if ((obj->data.ifblock.s)
				    && (!check_mount(obj->data.ifblock.s))) {
					i = obj->data.ifblock.pos;
					if_jumped = 1;
				} else {
					if_jumped = 0;
				}
			}
			OBJ(if_running) {
				if ((obj->data.ifblock.s)
				    && system(obj->data.ifblock.s)) {
					i = obj->data.ifblock.pos;
					if_jumped = 1;
				} else {
					if_jumped = 0;
				}
			}
			OBJ(kernel) {
				snprintf(p, p_max_size, "%s", cur->uname_s.release);
			}
			OBJ(machine) {
				snprintf(p, p_max_size, "%s", cur->uname_s.machine);
			}

			/* memory stuff */
			OBJ(mem) {
				human_readable(cur->mem * 1024, p, 6);
			}
			OBJ(memmax) {
				human_readable(cur->memmax * 1024, p, 255);
			}
			OBJ(memperc) {
				if (cur->memmax) {
					if (!use_spacer)
						snprintf(p, p_max_size, "%*lu",
							 pad_percents,
							 (cur->mem * 100) /
							 (cur->memmax));
					else
						snprintf(p, 4, "%*lu   ",
							 pad_percents,
							 (cur->mem * 100) /
							 (cur->memmax));
				}
			}
			OBJ(membar) {
				new_bar(p, obj->data.pair.a,
					obj->data.pair.b,
					cur->memmax ? (cur->mem * 255) /
					(cur->memmax) : 0);
			}

			OBJ(memgraph) {
				new_graph(p, obj->a,
				obj->b, obj->c, obj->d,
				cur->memmax ? (cur->mem * 100.0) /
						(cur->memmax) : 0.0, 100, 1);
			}
			/* mixer stuff */
			OBJ(mixer) {
				snprintf(p, p_max_size, "%d",
					 mixer_get_avg(obj->data.l));
			}
			OBJ(mixerl) {
				snprintf(p, p_max_size, "%d",
					 mixer_get_left(obj->data.l));
			}
			OBJ(mixerr) {
				snprintf(p, p_max_size, "%d",
					 mixer_get_right(obj->data.l));
			}
			OBJ(mixerbar) {
				new_bar(p, obj->data.mixerbar.w,
					obj->data.mixerbar.h,
					mixer_get_avg(obj->data.mixerbar.
						      l) * 255 / 100);
			}
			OBJ(mixerlbar) {
				new_bar(p, obj->data.mixerbar.w,
					obj->data.mixerbar.h,
					mixer_get_left(obj->data.mixerbar.
						       l) * 255 / 100);
			}
			OBJ(mixerrbar) {
				new_bar(p, obj->data.mixerbar.w,
					obj->data.mixerbar.h,
					mixer_get_right(obj->data.mixerbar.
							l) * 255 / 100);
			}

			/* mail stuff */
			OBJ(mails) {
				snprintf(p, p_max_size, "%d", cur->mail_count);
			}
			OBJ(new_mails) {
				snprintf(p, p_max_size, "%d", cur->new_mail_count);
			}
#ifdef MLDONKEY
			OBJ(ml_upload_counter) {
				snprintf(p, p_max_size, "%lld",
					 mlinfo.upload_counter / 1048576);
			}
			OBJ(ml_download_counter) {
				snprintf(p, p_max_size, "%lld",
					 mlinfo.download_counter /
					 1048576);
			}
			OBJ(ml_nshared_files) {
				snprintf(p, p_max_size, "%i", mlinfo.nshared_files);
			}
			OBJ(ml_shared_counter) {
				snprintf(p, p_max_size, "%lld",
					 mlinfo.shared_counter / 1048576);
			}
			OBJ(ml_tcp_upload_rate) {
				snprintf(p, p_max_size, "%.2f",
					 (float) mlinfo.tcp_upload_rate /
					 1024);
			}
			OBJ(ml_tcp_download_rate) {
				snprintf(p, p_max_size, "%.2f",
					 (float) mlinfo.tcp_download_rate /
					 1024);
			}
			OBJ(ml_udp_upload_rate) {
				snprintf(p, p_max_size, "%.2f",
					 (float) mlinfo.udp_upload_rate /
					 1024);
			}
			OBJ(ml_udp_download_rate) {
				snprintf(p, p_max_size, "%.2f",
					 (float) mlinfo.udp_download_rate /
					 1024);
			}
			OBJ(ml_ndownloaded_files) {
				snprintf(p, p_max_size, "%i",
					 mlinfo.ndownloaded_files);
			}
			OBJ(ml_ndownloading_files) {
				snprintf(p, p_max_size, "%i",
					 mlinfo.ndownloading_files);
			}
#endif

			OBJ(nodename) {
				snprintf(p, p_max_size, "%s",
					 cur->uname_s.nodename);
			}
			OBJ(outlinecolor) {
				new_outline(p, obj->data.l);
			}
			OBJ(processes) {
				if (!use_spacer)
					snprintf(p, p_max_size, "%hu", cur->procs);
				else
					snprintf(p, 5, "%hu    ",
						 cur->procs);
			}
			OBJ(running_processes) {
				if (!use_spacer)
					snprintf(p, p_max_size, "%hu",
						 cur->run_procs);
				else
					snprintf(p, 3, "%hu     ",
						 cur->run_procs);
			}
			OBJ(text) {
				snprintf(p, p_max_size, "%s", obj->data.s);
			}
			OBJ(shadecolor) {
				new_bg(p, obj->data.l);
			}
			OBJ(stippled_hr) {
				new_stippled_hr(p, obj->data.pair.a,
						obj->data.pair.b);
			}
			OBJ(swap) {
				human_readable(cur->swap * 1024, p, 255);
			}
			OBJ(swapmax) {
				human_readable(cur->swapmax * 1024, p,
					       255);
			}
			OBJ(swapperc) {
				if (cur->swapmax == 0) {
					strncpy(p, "No swap", 255);
				} else {
					if (!use_spacer)
						snprintf(p, 255, "%*lu",
							 pad_percents,
							 (cur->swap *
							  100) /
							 cur->swapmax);
					else
						snprintf(p, 4, "%*lu   ",
							 pad_percents,
							 (cur->swap *
							  100) /
							 cur->swapmax);
				}
			}
			OBJ(swapbar) {
				new_bar(p, obj->data.pair.a,
					obj->data.pair.b,
					cur->swapmax ? (cur->swap * 255) /
					(cur->swapmax) : 0);
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
			OBJ(totaldown) {
				human_readable(obj->data.net->recv, p,
					       255);
			}
			OBJ(totalup) {
				human_readable(obj->data.net->trans, p,
					       255);
			}
			OBJ(updates) {
				snprintf(p, p_max_size, "%d", total_updates);
			}
			OBJ(upspeed) {
				if (!use_spacer)
					snprintf(p, p_max_size, "%d",
						 (int) (obj->data.net->
							trans_speed /
							1024));
				else
					snprintf(p, 5, "%d     ",
						 (int) (obj->data.net->
							trans_speed /
							1024));
			}
			OBJ(upspeedf) {
				if (!use_spacer)
					snprintf(p, p_max_size, "%.1f",
						 obj->data.net->
						 trans_speed / 1024.0);
				else
					snprintf(p, 8, "%.1f       ",
						 obj->data.net->
						 trans_speed / 1024.0);
			}
			OBJ(upspeedgraph) {
				if (obj->data.net->trans_speed == 0)	// this is just to make the ugliness at start go away
					obj->data.net->trans_speed = 0.01;
				new_graph(p, obj->a, obj->b, obj->c, obj->d,
					  (obj->data.net->trans_speed /
				1024.0), obj->e, 1);
			}
			OBJ(uptime_short) {
				format_seconds_short(p, p_max_size,
						     (int) cur->uptime);
			}
			OBJ(uptime) {
				format_seconds(p, p_max_size, (int) cur->uptime);
			}

#if defined(__FreeBSD__) && (defined(i386) || defined(__i386__))
			OBJ(apm_adapter) {
				snprintf(p, p_max_size, "%s", get_apm_adapter());
			}
			OBJ(apm_battery_life) {
				char    *msg;
				msg = get_apm_battery_life();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
			OBJ(apm_battery_time) {
				char    *msg;
				msg = get_apm_battery_time();
				snprintf(p, p_max_size, "%s", msg);
				free(msg);
			}
#endif /* __FreeBSD__ */
#ifdef SETI
			OBJ(seti_prog) {
				snprintf(p, p_max_size, "%.2f",
					 cur->seti_prog * 100.0f);
			}
			OBJ(seti_progbar) {
				new_bar(p, obj->data.pair.a,
					obj->data.pair.b,
					(int) (cur->seti_prog * 255.0f));
			}
			OBJ(seti_credit) {
				snprintf(p, p_max_size, "%.0f", cur->seti_credit);
			}
#endif

#ifdef MPD
			OBJ(mpd_title) {
				snprintf(p, p_max_size, "%s", cur->mpd.title);
			}
			OBJ(mpd_artist) {
				snprintf(p, p_max_size, "%s", cur->mpd.artist);
			}
			OBJ(mpd_album) {
				snprintf(p, p_max_size, "%s", cur->mpd.album);
			}
			OBJ(mpd_random) {
				snprintf(p, p_max_size, "%s", cur->mpd.random);
			}
			OBJ(mpd_repeat) {
				snprintf(p, p_max_size, "%s", cur->mpd.repeat);
			}
			OBJ(mpd_track) {
				snprintf(p, p_max_size, "%s", cur->mpd.track);
			}
			OBJ(mpd_name) {
				snprintf(p, p_max_size, "%s", cur->mpd.name);
			}
			OBJ(mpd_file) {
				snprintf(p, p_max_size, "%s", cur->mpd.file);
			}
			OBJ(mpd_vol) {
				snprintf(p, p_max_size, "%i", cur->mpd.volume);
			}
			OBJ(mpd_bitrate) {
				snprintf(p, p_max_size, "%i", cur->mpd.bitrate);
			}
			OBJ(mpd_status) {
				snprintf(p, p_max_size, "%s", cur->mpd.status);
			}
			OBJ(mpd_elapsed) {
				int days = 0, hours = 0, minutes =
				    0, seconds = 0;
				int tmp = cur->mpd.elapsed;
				while (tmp >= 86400) {
					tmp -= 86400;
					days++;
				}
				while (tmp >= 3600) {
					tmp -= 3600;
					hours++;
				}
				while (tmp >= 60) {
					tmp -= 60;
					minutes++;
				}
				seconds = tmp;
				if (days > 0)
					snprintf(p, p_max_size, "%i days %i:%02i:%02i",
						 days, hours, minutes,
						 seconds);
				else if (hours > 0)
					snprintf(p, p_max_size, "%i:%02i:%02i", hours,
						 minutes, seconds);
				else
					snprintf(p, p_max_size, "%i:%02i", minutes,
						 seconds);
			}
			OBJ(mpd_length) {
				int days = 0, hours = 0, minutes =
				    0, seconds = 0;
				int tmp = cur->mpd.length;
				while (tmp >= 86400) {
					tmp -= 86400;
					days++;
				}
				while (tmp >= 3600) {
					tmp -= 3600;
					hours++;
				}
				while (tmp >= 60) {
					tmp -= 60;
					minutes++;
				}
				seconds = tmp;
				if (days > 0)
					snprintf(p, p_max_size,
						 "%i days %i:%02i:%02i",
						 days, hours, minutes,
						 seconds);
				else if (hours > 0)
					snprintf(p, p_max_size, "%i:%02i:%02i", hours,
						 minutes, seconds);
				else
					snprintf(p, p_max_size, "%i:%02i", minutes,
						 seconds);
			}
			OBJ(mpd_percent) {
				snprintf(p, p_max_size, "%2.0f",
					 cur->mpd.progress * 100);
			}
			OBJ(mpd_bar) {
				new_bar(p, obj->data.pair.a,
					obj->data.pair.b,
					(int) (cur->mpd.progress *
					       255.0f));
			}
			OBJ(mpd_smart) {
				if (strlen(cur->mpd.title) < 2 && strlen(cur->mpd.title) < 2) {
					snprintf(p, p_max_size, "%s", cur->mpd.file);
				} else {
					snprintf(p, p_max_size, "%s - %s", cur->mpd.artist, cur->mpd.title);
				}
			}
#endif
#ifdef XMMS2
			OBJ(xmms2_title) {
				snprintf(p, p_max_size, "%s", cur->xmms2.title);
			}
			OBJ(xmms2_artist) {
				snprintf(p, p_max_size, "%s", cur->xmms2.artist);
			}
			OBJ(xmms2_album) {
				snprintf(p, p_max_size, "%s", cur->xmms2.album);
			}
			OBJ(xmms2_track) {
				snprintf(p, p_max_size, "%i", cur->xmms2.track);
			}
			OBJ(xmms2_file) {
				snprintf(p, p_max_size, "%s", cur->xmms2.file);
			}
			OBJ(xmms2_status) {			
				snprintf(p, p_max_size, "%s", cur->xmms2.status);
			}
			OBJ(xmms2_elapsed) {
				int tmp = cur->xmms2.elapsed;
				snprintf(p, p_max_size, "%02d:%02d", 
				    tmp / 60000, (tmp / 1000) % 60);
			}
			OBJ(xmms2_length) {
				int tmp = cur->xmms2.length;
				snprintf(p, p_max_size, "%02d:%02d", 
				    tmp / 60000, (tmp / 1000) % 60);
			}
			OBJ(xmms2_percent) {
				snprintf(p, p_max_size, "%2.0f",
					 cur->xmms2.progress * 100);
			}
			OBJ(xmms2_bar) {
				new_bar(p, obj->data.pair.a,
					obj->data.pair.b,
					(int) (cur->xmms2.progress *
					       255.0f));
			}
			
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
			OBJ(xmms_status) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_STATUS]);
			}
        		OBJ(xmms_title) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_TITLE]);
			}
        		OBJ(xmms_length) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_LENGTH]);
			}
        		OBJ(xmms_length_seconds) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_LENGTH_SECONDS]);
			}
        		OBJ(xmms_position) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_POSITION]);
			}
        		OBJ(xmms_position_seconds) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_POSITION_SECONDS]);
			}
        		OBJ(xmms_bitrate) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_BITRATE]);
			}
        		OBJ(xmms_frequency) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_FREQUENCY]);
			}
        		OBJ(xmms_channels) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_CHANNELS]);
			}
        		OBJ(xmms_filename) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_FILENAME]);
			}
        		OBJ(xmms_playlist_length) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_PLAYLIST_LENGTH]);
			}
        		OBJ(xmms_playlist_position) {
			    snprintf(p, p_max_size, "%s", cur->xmms.items[XMMS_PLAYLIST_POSITION]);
			}
        		OBJ(xmms_bar) {
                            double progress;
                            progress= atof(cur->xmms.items[XMMS_POSITION_SECONDS]) /
                                      atof(cur->xmms.items[XMMS_LENGTH_SECONDS]);
                            new_bar(p,obj->a,obj->b,(int)(progress*255.0f));
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
				if (obj->data.top.type == TOP_NAME
				    && obj->data.top.num >= 0
				    && obj->data.top.num < 10) {
					// if we limit the buffer and add a bunch of space after, it stops the thing from
					// moving other shit around, which is really fucking annoying
					snprintf(p, 17, "%s                              ", cur->cpu[obj->data.top.num]->name);
				} else if (obj->data.top.type == TOP_CPU
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 7, "%3.2f      ",
						 cur->cpu[obj->data.top.
							  num]->amount);
				} else if (obj->data.top.type == TOP_PID
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 8, "%i           ",
						 cur->cpu[obj->data.top.
							  num]->pid);
				} else if (obj->data.top.type == TOP_MEM
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 7, "%3.2f       ",
						 cur->cpu[obj->data.top.
							  num]->totalmem);
				}
			}
			OBJ(top_mem) {
				if (obj->data.top.type == TOP_NAME
				    && obj->data.top.num >= 0
				    && obj->data.top.num < 10) {
					// if we limit the buffer and add a bunch of space after, it stops the thing from
					// moving other shit around, which is really fucking annoying
					snprintf(p, 17,
						 "%s                              ",
						 cur->memu[obj->data.top.
							   num]->name);
				} else if (obj->data.top.type == TOP_CPU
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 7, "%3.2f      ",
						 cur->memu[obj->data.top.
							   num]->amount);
				} else if (obj->data.top.type == TOP_PID
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 8, "%i           ",
						 cur->memu[obj->data.top.
							   num]->pid);
				} else if (obj->data.top.type == TOP_MEM
					   && obj->data.top.num >= 0
					   && obj->data.top.num < 10) {
					snprintf(p, 7, "%3.2f       ",
						 cur->memu[obj->data.top.
							   num]->totalmem);
				}
			}


			OBJ(tail) {
				if (current_update_time -obj->data.tail.last_update < obj->data.tail.interval) {
							snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
				} else {
					obj->data.tail.last_update = current_update_time;
					FILE *fp;
					int i;
					int added = 0;
					tailstring *head = NULL;
					tailstring *headtmp = NULL;
					tailstring *freetmp = NULL;
					fp = fopen(obj->data.tail.logfile, "rt");
					if (fp == NULL) {
						ERR("tail logfile failed to open");
					}
					else {
						obj->data.tail.readlines = 0;

						while (fgets(obj->data.tail.buffer, TEXT_BUFFER_SIZE*20, fp) != NULL) {
							if (added >= 30) {
								freelasttail(head);
							}
							else {
								added++;
							}
							addtail(&head, obj->data.tail.buffer);
							obj->data.tail.readlines++;
						}

						fclose(fp);
						freetmp = head;

						if (obj->data.tail.readlines > 0) {
							for (i = 0;i < obj->data.tail.wantedlines + 1 && i < obj->data.tail.readlines; i++) {
								addtail(&headtmp, head->data);
								head = head->next;
							}
							freetail(freetmp);
							freetmp = headtmp;
							strcpy(obj->data.tail.buffer, headtmp->data);
							headtmp = headtmp->next;
							for (i = 1;i < obj->data.tail.wantedlines + 1 && i < obj->data.tail.readlines; i++) {
								if (headtmp) {
									strncat(obj->data.tail.buffer, headtmp->data, (TEXT_BUFFER_SIZE * 20) - strlen(obj->data.tail.buffer)); /* without strlen() at the end this becomes a possible */
									headtmp = headtmp->next;
								}
							}

							/* get rid of any ugly newlines at the end */
							if (obj->data.tail.buffer[strlen(obj->data.tail.buffer)-1] == '\n') {
								obj->data.tail.buffer[strlen(obj->data.tail.buffer)-1] = '\0';
							}
							snprintf(p, p_max_size, "%s", obj->data.tail.buffer);

							freetail(freetmp);
						} else {
							strcpy(obj->data.tail.buffer, "Logfile Empty");
							snprintf(p, p_max_size, "Logfile Empty");
						}  /* if readlines */
					} /*  fp == NULL  */
				} /* if cur_upd_time >= */
	
				//parse_conky_vars(obj->data.tail.buffer, p, cur);

			}
			OBJ(head) {
				if (current_update_time -obj->data.tail.last_update < obj->data.tail.interval) {
							snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
				} else {
					obj->data.tail.last_update = current_update_time;
					FILE *fp;
					tailstring *head = NULL;
					tailstring *headtmp = NULL;
					tailstring *freetmp = NULL;
					fp = fopen(obj->data.tail.logfile, "rt");
					if (fp == NULL) {
						ERR("head logfile failed to open");
					} else {
						obj->data.tail.readlines = 0;
						while (fgets(obj->data.tail.buffer, TEXT_BUFFER_SIZE*20, fp) != NULL && obj->data.tail.readlines <= obj->data.tail.wantedlines) {
							addtail(&head, obj->data.tail.buffer);
							obj->data.tail.readlines++;
						}
						fclose(fp);
						freetmp = head;
						if (obj->data.tail.readlines > 0) {
							while (head) {
								addtail(&headtmp, head->data);
								head = head->next;
							}
							freetail(freetmp);
							freetmp = headtmp;
							strcpy(obj->data.tail.buffer, headtmp->data);
							headtmp = headtmp->next;
							while (headtmp) {
								strncat(obj->data.tail.buffer, headtmp->data, (TEXT_BUFFER_SIZE * 20) - strlen(obj->data.tail.buffer)); /* without strlen() at the end this becomes a possible */
								headtmp = headtmp->next;
							}
							freetail(freetmp);
							/* get rid of any ugly newlines at the end */
							if (obj->data.tail.buffer[strlen(obj->data.tail.buffer)-1] == '\n') {
								obj->data.tail.buffer[strlen(obj->data.tail.buffer)-1] = '\0';
							}
							snprintf(p, p_max_size, "%s", obj->data.tail.buffer);
						} else {
							strcpy(obj->data.tail.buffer, "Logfile Empty");
							snprintf(p, p_max_size, "Logfile Empty");
						} /* if readlines > 0 */
					} /* if fp == null */
				} /* cur_upd_time >= */

				//parse_conky_vars(obj->data.tail.buffer, p, cur);

			}
#ifdef TCP_PORT_MONITOR
			OBJ(tcp_portmon)
			{
				/* grab a pointer to this port monitor */
				tcp_port_monitor_t * p_monitor = 
					find_tcp_port_monitor( info.p_tcp_port_monitor_collection,
							        obj->data.tcp_port_monitor.port_range_begin,
								obj->data.tcp_port_monitor.port_range_end );
				if ( !p_monitor ) {
					snprintf(p, p_max_size, "monitor not found");
					break;
				}

				/* now grab the text of interest */
				if ( peek_tcp_port_monitor( p_monitor, 
				      	    		    obj->data.tcp_port_monitor.item, 
				            		    obj->data.tcp_port_monitor.connection_index,
				                            p, p_max_size ) != 0 )
				{
					snprintf(p, p_max_size, "monitor peek error");
					break;
				}
			}
#endif

			break;
		}

		{
			unsigned int a = strlen(p);
			p += a;
			p_max_size -= a;
		}
	}
}


double current_update_time, last_update_time;

static void generate_text()
{
	struct information *cur = &info;
	char *p;

	special_count = 0;

	/* update info */

	current_update_time = get_time();

	update_stuff(cur);

	/* add things to the buffer */

	/* generate text */

	p = text_buffer;

	generate_text_internal(p, P_MAX_SIZE, text_objects, text_object_count, cur);

	if (stuff_in_upper_case) {
		char *p;

		p = text_buffer;
		while (*p) {
			*p = toupper(*p);
			p++;
		}
	}

	last_update_time = current_update_time;
	total_updates++;
	//free(p);
}


#ifdef X11
static void set_font()
{
#ifdef XFT
	if (use_xft) {
			if (window.xftdraw != NULL) {
				XftDrawDestroy(window.xftdraw);
			}
			window.xftdraw = XftDrawCreate(display, window.drawable,
					DefaultVisual(display,
							screen),
					DefaultColormap(display,
							screen));
		} else
#endif
{
	XSetFont(display, window.gc, fonts[selected_font].font->fid);
}
}


/*
 * text size
 */

static int text_start_x, text_start_y;	/* text start position in window */
static int text_width, text_height;

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
	if (!s) {
		return 0;
	}
#ifdef X11
	char *p, *final;
	p = strdup(s);
	final = p;
	int index = 1;
	int width = 0;
	unsigned int i;
	while (*p) {
		if (*p == SPECIAL_CHAR) {
			/* shift everything over by 1 so that the special char doesn't mess up the size calculation */
			for (i = 0; i < strlen(p); i++) {
				*(p + i) = *(p + i + 1);
			}
			if (specials[special_index+index].type == GRAPH || specials[special_index+index].type == BAR) {
				width += specials[special_index+index].width;
			}
			index++;
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
	return strlen(s);
#endif /* X11 */
}

int fontchange = 0;

#ifdef X11
static void text_size_updater(char *s)
{
	int w = 0;
	char *p;
	int h = font_height();
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
				if (specials[special_index].height > h) {
					h = specials[special_index].height;
					h += font_ascent();
				}
			}
			
			else if (specials[special_index].type == OFFSET) {
				w += specials[special_index].arg + get_string_width("a"); /* filthy, but works */
			}
			else if (specials[special_index].type == VOFFSET) {
				h += specials[special_index].arg;
			}
			else if (specials[special_index].type == FONT) {
				fontchange = specials[special_index].font_added;
				selected_font = specials[special_index].font_added;
				h = font_height();
			}

			
			special_index++;
			s = p + 1;
		}
		p++;
	}
		w += get_string_width(s);
	if (w > text_width)
		text_width = w;
	if (text_width > maximum_width && maximum_width)
		text_width = maximum_width;

	text_height += h;
	if (fontchange) {
		selected_font = 0;
	}
}
#endif /* X11 */


#ifdef X11
static void update_text_area()
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
		for_each_line(text_buffer, text_size_updater);
		text_width += 1;
		if (text_height < minimum_height)
			text_height = minimum_height;
		if (text_width > maximum_width && maximum_width > 0)
			text_width = maximum_width;
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

	default:
	case BOTTOM_LEFT:
		x = gap_x;
		y = workarea[3] - text_height - gap_y;
		break;

	case BOTTOM_RIGHT:
		x = workarea[2] - text_width - gap_x;
		y = workarea[3] - text_height - gap_y;
		break;
	
#ifdef OWN_WINDOW
	case NONE: // Let the WM manage the window
		x = window.x;
		y = window.y;

		fixed_pos  = 1;
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
		/* If window size doesn't match to workarea's size, then window
		 * probably includes panels (gnome).
		 * Blah, doesn't work on KDE. */
		if (workarea[2] != window.width
		    || workarea[3] != window.height) {
			y += workarea[1];
			x += workarea[0];
		}

		text_start_x = x;
		text_start_y = y;
	}
}

/*
 * drawing stuff
 */

static int cur_x, cur_y;	/* current x and y for drawing */
static int draw_mode;		/* FG, BG or OUTLINE */
static long current_color;

static inline void set_foreground_color(long c)
{
	current_color = c;
	XSetForeground(display, window.gc, c);
}
#endif /* X11 */

static void draw_string(const char *s)
{
	if (s[0] == '\0')
		return;
	int i, i2, pos, width_of_s;
	int max=0;
	int added;
	width_of_s = get_string_width(s);
	if (out_to_console) {
		printf("%s\n", s);
		fflush(stdout);   /* output immediately, don't buffer */
	}
	/* daemon_run(s);  the daemon can be called here, but we need to have a buffer in daemon_run() and we need to tell it when everything is ready to be sent */
	memset(tmpstring1,0,TEXT_BUFFER_SIZE);
	memset(tmpstring2,0,TEXT_BUFFER_SIZE);
	strncpy(tmpstring1, s, TEXT_BUFFER_SIZE-1);
	pos = 0;
	added = 0;
	char space[2];
	snprintf(space, 2, " ");
#ifdef X11
	max = ((text_width - width_of_s) / get_string_width(space));
#endif /* X11 */
	/*
	 * This code looks for tabs in the text and coverts them to spaces.
	 * The trick is getting the correct number of spaces,
	 * and not going over the window's size without forcing
	 * the window larger.
	 */
	for (i = 0; i < TEXT_BUFFER_SIZE; i++) {
		if (tmpstring1[i] == '\t')	// 9 is ascii tab
		{
			i2 = 0;
			for (i2 = 0;
			     i2 < (8 - (1 + pos) % 8) && added <= max;
			     i2++) {
				/*
				if ( pos + i2 > TEXT_BUFFER_SIZE-1 )
					fprintf(stderr,"buffer overrun detected\n");
				*/
				tmpstring2[ MIN(pos + i2, TEXT_BUFFER_SIZE-1) ] = ' '; /* guard against overrun */
				added++;
			}
			pos += i2;
		} else {
			if (tmpstring1[i] != 9) {
				/*
				if ( pos > TEXT_BUFFER_SIZE-1 )
					 fprintf(stderr,"buffer overrun detected\n");
				*/
				tmpstring2[ MIN(pos, TEXT_BUFFER_SIZE-1) ] = tmpstring1[i]; /* guard against overrun */
				pos++;
			}
		}
	}
#ifdef X11
	if (text_width == maximum_width) {
		/* this means the text is probably pushing the limit, so we'll chop it */
		while (cur_x + get_string_width(tmpstring2) - text_start_x > maximum_width && strlen(tmpstring2) > 0) {
			tmpstring2[strlen(tmpstring2)-1] = '\0';
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
					  cur_x, cur_y, (XftChar8 *) s,
					  strlen(s));
		} else {
			XftDrawString8(window.xftdraw, &c2, fonts[selected_font].xftfont,
				       cur_x, cur_y, (XftChar8 *) s,
				       strlen(s));
		}
	} else
#endif
	{
		XDrawString(display, window.drawable, window.gc,
			    cur_x, cur_y, s, strlen(s));
	}
	cur_x += width_of_s;
#endif /* X11 */
	memcpy(tmpstring1, s, TEXT_BUFFER_SIZE);
}

long redmask, greenmask, bluemask;

void set_up_gradient()
{
#ifdef X11
	colour_depth = DisplayPlanes(display, screen);
#else
	colour_depth = 16;
#endif /* X11 */
	if (colour_depth != 24 && colour_depth != 16) {
		ERR("using non-standard colour depth, gradients may look like a lolly-pop");
	}
	int i;
	redmask = 0;
	greenmask = 0;
	bluemask = 0;
	for(i = (colour_depth / 3)-1; i>=0; i--) {
		redmask |= 1 << i;
		greenmask |= 1 << i;
		bluemask |= 1 << i;
	}
	if (colour_depth%3 == 1) {
		greenmask |= 1 << (colour_depth / 3);
	}
	redmask = redmask << (2*colour_depth / 3 + colour_depth%3);
	greenmask = greenmask << (colour_depth / 3);
}

inline unsigned long do_gradient(unsigned long first_colour, unsigned long last_colour) { /* this function returns the next colour between two colours for a gradient */
	int tmp_color = 0;
	int red1, green1, blue1; // first colour
	int red2, green2, blue2; // second colour
	int red3 = 0, green3 = 0, blue3 = 0; // difference
	short redshift = (2*colour_depth / 3 + colour_depth%3);
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

inline unsigned long gradient_max(unsigned long first_colour, unsigned long last_colour) { /* this function returns the max diff for a gradient */
	if (colour_depth == 0) {
		set_up_gradient();
	}
	int red1, green1, blue1; // first colour
	int red2, green2, blue2; // second colour
	long redshift = (2*colour_depth / 3 + colour_depth%3);
	long greenshift = (colour_depth / 3);
	int red3 = 0, green3 = 0, blue3 = 0; // difference
	red1 = (first_colour & redmask) >> redshift;
	green1 = (first_colour & greenmask) >> greenshift;
	blue1 = first_colour & bluemask;
	red2 = (last_colour & redmask) >> redshift;
	green2 = (last_colour & greenmask) >> greenshift;
	blue2 = last_colour & bluemask;
	red3 = abs(red1 - red2);
	green3 = abs(green1 - green2);
	blue3 = abs(blue1 - blue2);
	int max = red3;
	if (green3 > max)
		max = green3;
	if (blue3 > max)
		max = blue3;
	return max;
}

static void draw_line(char *s)
{
#ifdef X11
	char *p;
	cur_x = text_start_x;
	cur_y += font_ascent();
	int cur_y_add = 0;
	short font_h = font_height();

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
					int h =
					    specials[special_index].height;
					int mid = font_ascent() / 2;
					w = text_start_x + text_width -
					    cur_x;

					XSetLineAttributes(display,
							   window.gc, h,
							   LineSolid,
							   CapButt,
							   JoinMiter);
					XDrawLine(display, window.drawable,
						  window.gc, cur_x,
						  cur_y - mid / 2,
						  cur_x + w,
						  cur_y - mid / 2);
				}
				break;

			case STIPPLED_HR:
				{
					int h =
					    specials[special_index].height;
					int s =
					    specials[special_index].arg;
					int mid = font_ascent() / 2;
					char ss[2] = { s, s };
					w = text_start_x + text_width -
					    cur_x - 1;

					XSetLineAttributes(display,
							   window.gc, h,
							   LineOnOffDash,
							   CapButt,
							   JoinMiter);
					XSetDashes(display, window.gc, 0,
						   ss, 2);
					XDrawLine(display, window.drawable,
						  window.gc, cur_x,
						  cur_y - mid / 2,
						  cur_x + w,
						  cur_y - mid / 2);
				}
				break;

			case BAR:
				{
					if (cur_x - text_start_x > maximum_width && maximum_width > 0) {
						break;
					}
					int h =
					    specials[special_index].height;
					int bar_usage =
					    specials[special_index].arg;
					int by;

#ifdef XFT
					if (use_xft) {
						by = cur_y - (font_ascent() + h) / 2 - 1;
					} else 
#endif
					{
						by = cur_y - (font_ascent()/2) - 1;
					}
					if (h < (font_height())) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0)
						w = text_start_x +
						    text_width - cur_x - 1;
					if (w < 0)
						w = 0;

					XSetLineAttributes(display,
							   window.gc, 1,
							   LineSolid,
							   CapButt,
							   JoinMiter);

					XDrawRectangle(display,
						       window.drawable,
						       window.gc, cur_x,
						       by, w, h);
					XFillRectangle(display,
						       window.drawable,
						       window.gc, cur_x,
						       by,
						       w * bar_usage / 255,
						       h);
					if (specials[special_index].
					    height > cur_y_add
					    && specials[special_index].
					    height > font_h) {
						cur_y_add =
						    specials
						    [special_index].height;
					}
				}
				break;

			case GRAPH:
			{
					if (cur_x - text_start_x > maximum_width && maximum_width > 0) {
						break;
					}
					int h =
					    specials[special_index].height;
					int by;
					unsigned long last_colour = current_color;
#ifdef XFT
					if (use_xft) {
                                            by = cur_y - (font_ascent() + h) / 2 - 1;
					} else
#endif
					{
						by = cur_y - (font_ascent()/2) - 1;
					}
					if (h < (font_height())) {
						by -= h / 2 - 1;
					}
					w = specials[special_index].width;
					if (w == 0)
						w = text_start_x + text_width - cur_x - 1;
					if (w < 0)
						w = 0;
					if (draw_graph_borders) {
						XSetLineAttributes(display, window.gc, 1, LineSolid, CapButt, JoinMiter);
						XDrawRectangle(display,window.drawable, window.gc, cur_x, by, w, h);
					}
					XSetLineAttributes(display,
							   window.gc, 1,
							   LineSolid,
							   CapButt,
							   JoinMiter);
	int i;
	int j = 0;
	int gradient_size = 0;
	float gradient_factor = 0;
	float gradient_update = 0;
	unsigned long tmpcolour = current_color;
	if (specials[special_index].last_colour != specials[special_index].first_colour) {
		tmpcolour = specials[special_index].last_colour;
		gradient_size = gradient_max(specials[special_index].last_colour, specials[special_index].first_colour);
		gradient_factor = (float)gradient_size / (w - 3);
	}
	for (i = w - 3; i > 0; i--) {
		if (specials[special_index].last_colour != specials[special_index].first_colour) {
			XSetForeground(display, window.gc, tmpcolour);
			gradient_update += gradient_factor;
			while (gradient_update > 0) {
				tmpcolour = do_gradient(tmpcolour, specials[special_index].first_colour);
				gradient_update--;
			}
		}
		if ((w - 3 - i) / ((float) (w - 3) / (specials[special_index].graph_width)) > j) {
			j++;
		}
						XDrawLine(display,  window.drawable, window.gc, cur_x + i + 2, by + h, cur_x + i + 2, by + h - specials[special_index].graph[j] * (h - 1) / specials[special_index].graph_scale);	/* this is mugfugly, but it works */
					}
					if (specials[special_index].
					    height > cur_y_add
					    && specials[special_index].
					    height > font_h) {
						cur_y_add =
						    specials
						    [special_index].height;
					}
/*				if (draw_mode == BG) {
					set_foreground_color(default_bg_color);
				}
				else if (draw_mode == OUTLINE) {
					set_foreground_color(default_out_color);
				} else {
					set_foreground_color(default_fg_color);
				}*/
				set_foreground_color(last_colour);
			}
				break;
			
				case FONT:
				if (fontchange) {
					cur_y -= font_ascent();
					selected_font = specials[special_index].font_added;
					cur_y += font_ascent();
#ifdef XFT
					if (!use_xft || use_xdbe)
#endif
					{
						set_font();
					}
				}
				break;
			case FG:
				if (draw_mode == FG)
					set_foreground_color(specials
							     [special_index].
							     arg);
				break;

			case BG:
				if (draw_mode == BG)
					set_foreground_color(specials
							     [special_index].
							     arg);
				break;

			case OUTLINE:
				if (draw_mode == OUTLINE)
					set_foreground_color(specials
							     [special_index].
							     arg);
				break;

				case OFFSET:
				{
					w += specials[special_index].arg;
				}
				break;
				case VOFFSET:
				{
					cur_y += specials[special_index].arg;
				}
				break;

			case ALIGNR:
				{
					int pos_x = text_start_x + text_width - get_string_width_special(s) /*+ border_margin*/;
					/*printf("pos_x %i text_start_x %i text_width %i cur_x %i get_string_width(p) %i gap_x %i specials[special_index].arg %i border_margin %i border_width %i\n", pos_x, text_start_x, text_width, cur_x, get_string_width_special(s), gap_x, specials[special_index].arg, border_margin, border_width);*/
					if (pos_x > specials[special_index].arg && pos_x > cur_x) {
						cur_x = pos_x - specials[special_index].arg;
				}
				}
				break;

			case ALIGNC:
				{
					int pos_x = (text_width)/2 - get_string_width_special(s)/2 - (cur_x - text_start_x);
					/*int pos_x = text_start_x + text_width/2 - get_string_width_special(s)/2;*/
					/*printf("pos_x %i text_start_x %i text_width %i cur_x %i get_string_width(p) %i gap_x %i specials[special_index].arg %i\n", pos_x, text_start_x, text_width, cur_x, get_string_width(s), gap_x, specials[special_index].arg);*/
					if (pos_x >
					    specials[special_index].arg)
						w = pos_x -
						    specials
						    [special_index].arg;
				}
				break;

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
	if (fontchange) {
		selected_font = 0;
	}
#endif /* X11 */
}

static void draw_text()
{
#ifdef X11
	cur_y = text_start_y;

	/* draw borders */
	if (draw_borders && border_width > 0) {
		unsigned int b = (border_width + 1) / 2;

		if (stippled_borders) {
			char ss[2] =
			    { stippled_borders, stippled_borders };
			XSetLineAttributes(display, window.gc,
					   border_width, LineOnOffDash,
					   CapButt, JoinMiter);
			XSetDashes(display, window.gc, 0, ss, 2);
		} else {
			XSetLineAttributes(display, window.gc,
					   border_width, LineSolid,
					   CapButt, JoinMiter);
		}

		XDrawRectangle(display, window.drawable, window.gc,
			       text_start_x - border_margin + b,
			       text_start_y - border_margin + b,
			       text_width + border_margin * 2 - 1 - b * 2,
			       text_height + border_margin * 2 - 1 -
			       b * 2);
	}

	/* draw text */
	special_index = 0;
#endif /* X11 */
	for_each_line(text_buffer, draw_line);
}

static void draw_stuff()
{
#ifdef X11
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
		for (i = -1; i < 2; i++)
			for (j = -1; j < 2; j++) {
				if (i == 0 && j == 0)
					continue;
				text_start_x += i;
				text_start_y += j;
				set_foreground_color(default_out_color);
				draw_mode = OUTLINE;
				draw_text();
				text_start_x -= i;
				text_start_y -= j;
			}
	}

	set_foreground_color(default_fg_color);
	draw_mode = FG;
#endif /* X11 */
	draw_text();
#ifdef X11
#ifdef XDBE
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
#ifdef XDBE
	if (use_xdbe) {
		return;		/* The swap action is XdbeBackground, which clears */
	} else
#endif
	{
	/* there is some extra space for borders and outlines */
	XClearArea(display, window.drawable,
		   text_start_x - border_margin - 1,
		   text_start_y - border_margin - 1,
		   text_width + border_margin * 2 + 2,
		   text_height + border_margin * 2 + 2,
		   exposures ? True : 0);
	}
}
#endif /* X11 */

static int need_to_update;

/* update_text() generates new text and clears old text area */
static void update_text()
{
	generate_text();
#ifdef X11
	clear_text(1);
#endif /* X11 */
	need_to_update = 1;
}

static void main_loop()
{
#ifdef SIGNAL_BLOCKING
	sigset_t  newmask, oldmask;

	sigemptyset(&newmask);
	sigaddset(&newmask,SIGINT);
	sigaddset(&newmask,SIGTERM);
	sigaddset(&newmask,SIGUSR1);
#endif

#ifdef X11
	Region region = XCreateRegion();
#endif /* X11 */

	info.looped = 0;
	while (total_run_times == 0 || info.looped < total_run_times - 1) {
		info.looped++;

#ifdef SIGNAL_BLOCKING
		/* block signals.  we will inspect for pending signals later */
		if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0)
			CRIT_ERR("unable to sigprocmask()");
#endif

#ifdef X11
		XFlush(display);

		/* wait for X event or timeout */

		if (!XPending(display)) {
			fd_set fdsr;
			struct timeval tv;
			int s;
			double t =
			    update_interval - (get_time() -
					       last_update_time);

			if (t < 0)
				t = 0;

			tv.tv_sec = (long) t;
			tv.tv_usec = (long) (t * 1000000) % 1000000;
			FD_ZERO(&fdsr);
			FD_SET(ConnectionNumber(display), &fdsr);


			s = select(ConnectionNumber(display) + 1, &fdsr, 0,
				   0, &tv);
#else
			usleep(update_interval*1000000); /* FIXME just sleep for the interval time if no X11 */
#endif /* X11 */
#ifdef X11
			if (s == -1) {
				if (errno != EINTR)
					ERR("can't select(): %s",
					    strerror(errno));
			} else {
				/* timeout */
				if (s == 0)
#endif /* X11 */
					update_text();
#ifdef X11
			}
		}
		
		if (need_to_update) {
#ifdef OWN_WINDOW
			int wx = window.x, wy = window.y;
#endif

			need_to_update = 0;

			update_text_area();
#ifdef OWN_WINDOW
			if (own_window) {
				/* resize window if it isn't right size */
				if (!fixed_size &&
				    (text_width + border_margin * 2 + 1 !=
				     window.width
				     || text_height + border_margin * 2 + 1 !=
				     window.height)) {
					window.width =
					    text_width +
					    border_margin * 2 + 1;
					window.height =
					    text_height +
					    border_margin * 2 + 1;
					XResizeWindow(display,
						      window.window,
						      window.width,
						      window.height);
			if (own_window) {
				set_transparent_background(window.window);
			}
				     }

				/* move window if it isn't in right position */
				if (!fixed_pos
				    && (window.x != wx
					|| window.y != wy)) {
					XMoveWindow(display, window.window,
						    window.x, window.y);
				}
			}
#endif

			clear_text(1);

#ifdef XDBE
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
					XUnionRectWithRegion(&r, region,
							     region);
				}
				break;

#ifdef OWN_WINDOW
			case ReparentNotify:
				/* set background to ParentRelative for all parents */
				if (own_window) {
					set_transparent_background(window.
					window);
				}
				break;

			case ConfigureNotify:
				if (own_window) {
					/* if window size isn't what expected, set fixed size */
					if (ev.xconfigure.width !=
					    window.width
					    || ev.xconfigure.height !=
					    window.height) {
						if (window.width != 0
						    && window.height != 0)
							fixed_size = 1;

						/* clear old stuff before screwing up size and pos */
						clear_text(1);

						{
							XWindowAttributes
							    attrs;
							if (XGetWindowAttributes(display, window.window, &attrs)) {
								window.
								    width =
								    attrs.
								    width;
								window.
								    height
								    =
								    attrs.
								    height;
							}
						}

						text_width =
						    window.width -
						    border_margin * 2 - 1;
						text_height =
						    window.height -
						    border_margin * 2 - 1;
						if (text_width > maximum_width && maximum_width > 0)
							text_width = maximum_width;
					}

					/* if position isn't what expected, set fixed pos, total_updates
					 * avoids setting fixed_pos when window is set to weird locations
					 * when started */
					/*if (total_updates >= 2 this is broken
					    && !fixed_pos
					    && (window.x != ev.xconfigure.x
						|| window.y !=
						ev.xconfigure.y)
					    && (ev.xconfigure.x != 0
						|| ev.xconfigure.y != 0)) {
						fixed_pos = 1;
				}*/
					set_font();
				}
				break;

			case ButtonPress:
				if (own_window)
				{
				    /* forward the click to the desktop window */
				    XUngrabPointer(display, ev.xbutton.time);
				    ev.xbutton.window = window.desktop;
				    XSendEvent(display, ev.xbutton.window, False, ButtonPressMask, &ev);
				}
				break;

 			case ButtonRelease:
                                if (own_window)
                                {
                                    /* forward the release to the desktop window */
                                    ev.xbutton.window = window.desktop;
                                    XSendEvent(display, ev.xbutton.window, False, ButtonReleaseMask, &ev);
                                }
                                break;

#endif

			default:
				break;
			}
		}

		/* XDBE doesn't seem to provide a way to clear the back buffer without
		 * interfering with the front buffer, other than passing XdbeBackground
		 * to XdbeSwapBuffers. That means that if we're using XDBE, we need to
		 * redraw the text even if it wasn't part of the exposed area. OTOH,
		 * if we're not going to call draw_stuff at all, then no swap happens
		 * and we can safely do nothing.
		 */

		if (!XEmptyRegion(region)) {
#ifdef XDBE
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
		if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0)
			CRIT_ERR("unable to sigprocmask()");
#endif

		switch(g_signal_pending) {
		case SIGUSR1:
			{
			ERR("received SIGUSR1. reloading the config file.");
			reload_config();
			break;
			}
		case SIGINT:
		case SIGTERM:
			{
			ERR("received SIGINT or SIGTERM to terminate. bye!");
			clean_up();
#ifdef X11
			XDestroyRegion(region);
			region = NULL;
#endif /* X11 */
			return;  /* return from main_loop */
			/*break;*/
			}
		default:
			{
			/* Reaching here means someone set a signal( SIGXXXX, signal_handler )
			 * but didn't write any code to deal with it.   if you don't want to
			 * handle a signal, don't set a handler on it in the first place. */
			if (g_signal_pending)
				ERR("ignoring signal (%d)", g_signal_pending);
			break;
			}
		}
		g_signal_pending=0;
	
	}
#ifdef X11
	XDestroyRegion(region);
	region = NULL;
#endif /* X11 */
}

static void load_config_file(const char *);

/* reload the config file */
void reload_config(void)
{
	//lock_all_threads();
	threads_runnable++;
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
        if (info.xmms.thread) {
		if (destroy_xmms_thread()!=0)
		{
			ERR("error destroying xmms_player thread");
		}
	}
#endif
#ifdef TCP_PORT_MONITOR
	destroy_tcp_port_monitor_collection( info.p_tcp_port_monitor_collection );
#endif

	if (current_config) {
		clear_fs_stats();
		load_config_file(current_config);
#ifdef X11
		load_fonts();
		set_font();
		XClearWindow(display, RootWindow(display, screen)); // clear the window first

#endif /* X11 */
#ifdef TCP_PORT_MONITOR
		info.p_tcp_port_monitor_collection = NULL; 
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
                if ( (!info.xmms.thread) && (info.xmms.current_project > PROJECT_NONE) && 
		     (create_xmms_thread() !=0) )
                    {
                        CRIT_ERR("unable to create xmms_player thread!");
                    }
#endif
		extract_variable_text(text);
		free(text);
		text = NULL;
		update_text();
	}
	thread_count = 0;
}

void clean_up(void)
{
	//lock_all_threads();
	threads_runnable++;
#ifdef X11
#ifdef XDBE
	if (use_xdbe) {
		XdbeDeallocateBackBufferName(display, window.back_buffer);
	}
#endif
#ifdef OWN_WINDOW
	if (own_window) 
	{
		XDestroyWindow(display, window.window);
		XClearWindow(display, RootWindow(display, screen));
		XFlush(display);
	}
	else
#endif
	{
		XClearWindow(display, RootWindow(display, screen));
		clear_text(1);
		XFlush(display);
	}

	XFreeGC(display, window.gc);
#endif /* X11 */


	/* it is really pointless to free() memory at the end of program but ak|ra
	 * wants me to do this */

	free_text_objects(text_object_count, text_objects);
	text_object_count = 0;
	text_objects = NULL;

#ifdef MLDONKEY	
	ml_cleanup();
#endif /* MLDONKEY */

	if (text != original_text)
		free(text);

	free(current_config);
	free(current_mail_spool);
#ifdef SETI
	free(seti_dir);
#endif
#ifdef TCP_PORT_MONITOR
	destroy_tcp_port_monitor_collection( info.p_tcp_port_monitor_collection );
	info.p_tcp_port_monitor_collection = NULL;
#endif
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
        if ( info.xmms.thread && (destroy_xmms_thread()!=0) )
        {
            ERR("error destroying xmms_player thread");
        }
#endif
}

static int string_to_bool(const char *s)
{
	if (!s)
		return 1;
	if (strcasecmp(s, "yes") == 0)
		return 1;
	if (strcasecmp(s, "true") == 0)
		return 1;
	if (strcasecmp(s, "1") == 0)
		return 1;
	return 0;
}
#ifdef X11
static enum alignment string_to_alignment(const char *s)
{
	if (strcasecmp(s, "top_left") == 0)
		return TOP_LEFT;
	else if (strcasecmp(s, "top_right") == 0)
		return TOP_RIGHT;
	else if (strcasecmp(s, "bottom_left") == 0)
		return BOTTOM_LEFT;
	else if (strcasecmp(s, "bottom_right") == 0)
		return BOTTOM_RIGHT;
	else if (strcasecmp(s, "tl") == 0)
		return TOP_LEFT;
	else if (strcasecmp(s, "tr") == 0)
		return TOP_RIGHT;
	else if (strcasecmp(s, "bl") == 0)
		return BOTTOM_LEFT;
	else if (strcasecmp(s, "br") == 0)
		return BOTTOM_RIGHT;
	else if (strcasecmp(s, "none") == 0)
		return NONE;
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
	top_mem = 0;
#ifdef MPD
	strcpy(info.mpd.host, "localhost");
	info.mpd.port = 6600;
	info.mpd.status = NULL;
	info.mpd.artist = NULL;
	info.mpd.album = NULL;
	info.mpd.title = NULL;
	info.mpd.random = NULL;
	info.mpd.track = NULL;
	info.mpd.name = NULL;
	info.mpd.file = NULL;
#endif
#ifdef XMMS2
	info.xmms2.status = NULL;
	info.xmms2.artist = NULL;
	info.xmms2.album = NULL;
	info.xmms2.title = NULL;
//	info.xmms2.random = NULL;
//	info.xmms2.track = NULL;
//	info.xmms2.name = NULL;
	info.xmms2.file = NULL;
//	info.xmms2.connection = NULL;
#endif
	use_spacer = 0;
#ifdef X11
	out_to_console = 0;
#else
	out_to_console = 1;
#endif
#ifdef X11
	default_fg_color = WhitePixel(display, screen);
	default_bg_color = BlackPixel(display, screen);
	default_out_color = BlackPixel(display, screen);
	draw_shades = 1;
	draw_borders = 0;
	draw_graph_borders = 1;
	draw_outline = 0;
	set_first_font("6x10");
	gap_x = 5;
	gap_y = 5;
	minimum_width = 5;
	minimum_height = 5;
	maximum_width = 0;
#ifdef OWN_WINDOW
	own_window = 0;
	window.type=TYPE_NORMAL;
	window.hints=0;
     	strcpy(window.wm_class_name, "conky");	
#endif
	stippled_borders = 0;
	border_margin = 3;
	border_width = 1;
	text_alignment = BOTTOM_LEFT;
	on_bottom = 1;
#endif /* X11 */

	free(current_mail_spool);
	{
		char buf[256];
		variable_substitute(MAIL_FILE, buf, 256);
		if (buf[0] != '\0')
			current_mail_spool = strdup(buf);
	}

	no_buffers = 1;
	update_interval = 10.0;
	stuff_in_upper_case = 0;
#ifdef MLDONKEY
	mlconfig.mldonkey_hostname = strdup("127.0.0.1");
	mlconfig.mldonkey_port = 4001;
	mlconfig.mldonkey_login = NULL;
	mlconfig.mldonkey_password = NULL;
#endif

#ifdef TCP_PORT_MONITOR
	tcp_port_monitor_collection_args.min_port_monitors = MIN_PORT_MONITORS_DEFAULT;
	tcp_port_monitor_args.min_port_monitor_connections = MIN_PORT_MONITOR_CONNECTIONS_DEFAULT;
#endif

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	info.xmms.current_project=PROJECT_NONE;
#endif
}

static void load_config_file(const char *f)
{
#define CONF_ERR ERR("%s: %d: config file error", f, line)
	int line = 0;
	FILE *fp;

	set_default_configurations();
	fp = fopen(f, "r");
	if (!fp)
		return;

	while (!feof(fp)) {
		char buf[256], *p, *p2, *name, *value;
		line++;
		if (fgets(buf, 256, fp) == NULL)
			break;

		p = buf;

		/* break at comment */
		p2 = strchr(p, '#');
		if (p2)
			*p2 = '\0';

		/* skip spaces */
		while (*p && isspace((int) *p))
			p++;
		if (*p == '\0')
			continue;	/* empty line */

		name = p;

		/* skip name */
		p2 = p;
		while (*p2 && !isspace((int) *p2))
			p2++;
		if (*p2 != '\0') {
			*p2 = '\0';	/* break at name's end */
			p2++;
		}

		/* get value */
		if (*p2) {
			p = p2;
			while (*p && isspace((int) *p))
				p++;

			value = p;

			p2 = value + strlen(value);
			while (isspace((int) *(p2 - 1)))
				*--p2 = '\0';
		} else {
			value = 0;
		}

#define CONF2(a) if (strcasecmp(name, a) == 0)
#define CONF(a) else CONF2(a)
#define CONF3(a,b) \
else if (strcasecmp(name, a) == 0 || strcasecmp(name, b) == 0)


#ifdef X11
		CONF2("alignment") {
	if (value) {
		int a = string_to_alignment(value);
		if (a <= 0)
			CONF_ERR;
		else
			text_alignment = a;
	} else
		CONF_ERR;
		}
		CONF("on_bottom") {
			if(value)
				ERR("on_bottom is deprecated.  use own_window_hints below");
				on_bottom = string_to_bool(value);
				if (on_bottom)
				    SET_HINT(window.hints,HINT_BELOW);

			else
				CONF_ERR;
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
		CONF("border_margin") {
			if (value)
				border_margin = strtol(value, 0, 0);
			else
				CONF_ERR;
		}
		CONF("border_width") {
			if (value)
				border_width = strtol(value, 0, 0);
			else
				CONF_ERR;
		}
		CONF("default_color") {
			if (value)
				default_fg_color = get_x11_color(value);
			else
				CONF_ERR;
		}
		CONF3("default_shade_color", "default_shadecolor") {
			if (value)
				default_bg_color = get_x11_color(value);
			else
				CONF_ERR;
		}
		CONF3("default_outline_color", "default_outlinecolor") {
			if (value)
				default_out_color = get_x11_color(value);
			else
				CONF_ERR;
		}
#endif /* X11 */
#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
		CONF("xmms_player") {
			if (value) {
				if (strncmp(value,"none",4)==0)
				    info.xmms.current_project=PROJECT_NONE;
				else if (strncmp(value,"xmms",4)==0)
				    info.xmms.current_project=PROJECT_XMMS;
				else if (strncmp(value,"bmp",3)==0)
				    info.xmms.current_project=PROJECT_BMP;
				else if (strncmp(value,"audacious",9)==0)
				    info.xmms.current_project=PROJECT_AUDACIOUS;
				else if  (strncmp(value,"infopipe",8)==0)
				    info.xmms.current_project=PROJECT_INFOPIPE;
				else
					CONF_ERR;
			}
			else
				CONF_ERR;
		}
#endif
#ifdef MPD
		CONF("mpd_host") {
			if (value)
				strncpy(info.mpd.host, value, 127);
			else
				CONF_ERR;
		}
		CONF("mpd_port") {
			if (value) {
				info.mpd.port = strtol(value, 0, 0);
				if (info.mpd.port < 1
				    || info.mpd.port > 0xffff)
					CONF_ERR;
			}
		}
		CONF("mpd_password") {
			if (value)
				strncpy(info.mpd.password, value, 127);
			else
				CONF_ERR;
		}
#endif
		CONF("cpu_avg_samples") {
			if (value) {
				cpu_avg_samples = strtol(value, 0, 0);
				if (cpu_avg_samples < 1
				    || cpu_avg_samples > 14)
					CONF_ERR;
				else
					info.
					    cpu_avg_samples
					    = cpu_avg_samples;
			} else
				CONF_ERR;
		}
		CONF("net_avg_samples") {
			if (value) {
				net_avg_samples = strtol(value, 0, 0);
				if (net_avg_samples < 1
				    || net_avg_samples > 14)
					CONF_ERR;
				else
					info.
					    net_avg_samples
					    = net_avg_samples;
			} else
				CONF_ERR;
		}






#ifdef XDBE
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
			out_to_console = string_to_bool(value);
		}
		CONF("use_spacer") {
			use_spacer = string_to_bool(value);
		}
#ifdef X11
#ifdef XFT
		CONF("use_xft") {
			use_xft = string_to_bool(value);
		}
		CONF("font") {
			if (value) {
				set_first_font(value);
			} else
				CONF_ERR;
		}
		CONF("xftalpha") {
			if (value && font_count >= 0)
				fonts[0].font_alpha = atof(value)
				    * 65535.0;
			else
				CONF_ERR;
		}
		CONF("xftfont") {
			if (use_xft) {
#else
		CONF("use_xft") {
			if (string_to_bool(value))
				ERR("Xft not enabled");
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
				} else
					CONF_ERR;
#ifdef XFT
			}
#endif
		}
		CONF("gap_x") {
			if (value)
				gap_x = atoi(value);
			else
				CONF_ERR;
		}
		CONF("gap_y") {
			if (value)
				gap_y = atoi(value);
			else
				CONF_ERR;
		}
#endif /* X11 */
		CONF("mail_spool") {
			if (value) {
				char buf[256];
				variable_substitute(value, buf, 256);

				if (buf[0]
				    != '\0') {
					if (current_mail_spool)
						free(current_mail_spool);
					current_mail_spool = strdup(buf);
				}
			} else
				CONF_ERR;
		}
#ifdef X11
		CONF("minimum_size") {
	if (value) {
		if (sscanf
				  (value, "%d %d", &minimum_width,
				   &minimum_height) != 2)
			if (sscanf
						 (value, "%d",
						   &minimum_width) != 1)
				CONF_ERR;
	} else
		CONF_ERR;
		}
		CONF("maximum_width") {
			if (value) {
				if (sscanf(value, "%d",	&maximum_width) != 1)
					CONF_ERR;
			} else
				CONF_ERR;
		}
#endif /* X11 */
		CONF("no_buffers") {
			no_buffers = string_to_bool(value);
		}
#ifdef MLDONKEY
		CONF("mldonkey_hostname") {
			if (value) {
				if (mlconfig.mldonkey_hostname != NULL) {
					free(mlconfig.mldonkey_hostname);
				}
			mlconfig.mldonkey_hostname = strdup(value);
			}
			else
				CONF_ERR;
		}
		CONF("mldonkey_port") {
			if (value)
				mlconfig.mldonkey_port = atoi(value);
			else
				CONF_ERR;
		}
		CONF("mldonkey_login") {
			if (value) {
				if (mlconfig.mldonkey_login != NULL) {
					free(mlconfig.mldonkey_login);
				}
				mlconfig.mldonkey_login = strdup(value);
			}
			else
				CONF_ERR;
		}
		CONF("mldonkey_password") {
			if (value) {
				if (mlconfig.mldonkey_password != NULL) {
					free(mlconfig.mldonkey_password);
				}
				mlconfig.mldonkey_password = strdup(value);
			}
			else
				CONF_ERR;
		}
#endif
		CONF("pad_percents") {
	pad_percents = atoi(value);
		}
#ifdef X11
#ifdef OWN_WINDOW
		CONF("own_window") {
			own_window = string_to_bool(value);
		}
		CONF("wm_class_name") {
			memset(window.wm_class_name,0,sizeof(window.wm_class_name));
			strncpy(window.wm_class_name, value, sizeof(window.wm_class_name)-1);
		}
		CONF("own_window_transparent") {
			set_transparent = string_to_bool(value);
		}
		CONF("own_window_colour") {
			if (value) {
				background_colour = get_x11_color(value);
			} else {
				ERR("Invalid colour for own_winder_colour (try omitting the '#' for hex colours");
			}
		}
		CONF("own_window_hints") {
			if (value) {
				char *p_hint, *p_save;
				char delim[] = ", ";

				/* tokenize the value into individual hints */
				if ((p_hint=strtok_r(value, delim, &p_save)) != NULL)
				do {
					 /*fprintf(stderr, "hint [%s] parsed\n", p_hint);*/
					 if (strncmp(p_hint,"undecorate",10) == 0)
					     SET_HINT(window.hints,HINT_UNDECORATED);
					 else if (strncmp(p_hint,"below",5) == 0)
					     SET_HINT(window.hints,HINT_BELOW);
					 else if (strncmp(p_hint,"above",5) == 0)
					     SET_HINT(window.hints,HINT_ABOVE);
					 else if (strncmp(p_hint,"sticky",6) == 0)
					     SET_HINT(window.hints,HINT_STICKY);
					 else if (strncmp(p_hint,"skip_taskbar",12) == 0)
					     SET_HINT(window.hints,HINT_SKIP_TASKBAR);
					 else if (strncmp(p_hint,"skip_pager",10) == 0)
					     SET_HINT(window.hints,HINT_SKIP_PAGER);
					 else
					     CONF_ERR;

					 p_hint=strtok_r(NULL, delim, &p_save);
				}
				while (p_hint!=NULL);
			}
		}
		CONF("own_window_type") {
			if (value) {
				if (strncmp(value,"normal",6)==0)
					window.type = TYPE_NORMAL;
				else if  (strncmp(value,"desktop",7)==0)
					window.type = TYPE_DESKTOP;
				else if (strncmp(value,"override",8)==0)
					window.type = TYPE_OVERRIDE;
				else
				    	CONF_ERR;
			}
		}
#endif
		CONF("stippled_borders") {
			if (value)
				stippled_borders = strtol(value, 0, 0);
			else
				stippled_borders = 4;
		}
#endif /* X11 */
		CONF("temp1") {
			ERR("temp1 configuration is obsolete, use ${i2c <i2c device here> temp 1}");
		}
		CONF("temp1") {
			ERR("temp2 configuration is obsolete, use ${i2c <i2c device here> temp 2}");
		}
		CONF("update_interval") {
			if (value)
				update_interval = strtod(value, 0);
			else
				CONF_ERR;
		}
		CONF("total_run_times") {
			if (value)
				total_run_times = strtod(value, 0);
			else
				CONF_ERR;
		}
		CONF("uppercase") {
			stuff_in_upper_case = string_to_bool(value);
		}
#ifdef SETI
		CONF("seti_dir") {
			seti_dir = (char *)
			    malloc(strlen(value)
				   + 1);
			strcpy(seti_dir, value);
		}
#endif
		CONF("text") {
			if (text != original_text)
				free(text);

			text = (char *)
			    malloc(1);
			text[0]
			    = '\0';

			while (!feof(fp)) {
				unsigned
				int l = strlen(text);
				if (fgets(buf, 256, fp) == NULL)
					break;
				text = (char *)
				    realloc(text, l + strlen(buf)
					    + 1);
				strcat(text, buf);

				if (strlen(text) > 1024 * 8)
					break;
			}
			fclose(fp);
			return;
		}
#ifdef TCP_PORT_MONITOR
		CONF("min_port_monitors") 
		{
			if ( !value || 
			     (sscanf(value, "%d", &tcp_port_monitor_collection_args.min_port_monitors) != 1) || 
			     tcp_port_monitor_collection_args.min_port_monitors < 0 )
			{
				/* an error. use default, warn and continue. */
				tcp_port_monitor_collection_args.min_port_monitors = MIN_PORT_MONITORS_DEFAULT;
				CONF_ERR;
			}
			else if ( tcp_port_monitor_collection_args.min_port_monitors == 0 )
			{
				/* no error, just use default */
				tcp_port_monitor_collection_args.min_port_monitors = MIN_PORT_MONITORS_DEFAULT;
			}
			/* else tcp_port_monitor_collection_args.min_port_monitors > 0 as per config */
		}
		CONF("min_port_monitor_connections") 
		{
			if ( !value || 
			     (sscanf(value, "%d", &tcp_port_monitor_args.min_port_monitor_connections) != 1) 
			     || tcp_port_monitor_args.min_port_monitor_connections < 0 )
			{
				/* an error. use default, warni and continue. */
				tcp_port_monitor_args.min_port_monitor_connections = MIN_PORT_MONITOR_CONNECTIONS_DEFAULT;
				CONF_ERR;
			}
			else if ( tcp_port_monitor_args.min_port_monitor_connections == 0 )
			{
				/* no error, just use default */
				tcp_port_monitor_args.min_port_monitor_connections = MIN_PORT_MONITOR_CONNECTIONS_DEFAULT;
			}
			/* else tcp_port_monitor_args.min_port_monitor_connections > 0 as per config */
		}
#endif
		else
		ERR("%s: %d: no such configuration: '%s'", f, line, name);

#undef CONF
#undef CONF2
	}

	fclose(fp);
#undef CONF_ERR
}

																							/* : means that character before that takes an argument */
static const char *getopt_string = "vVdt:f:u:i:hc:w:x:y:a:"
#ifdef X11
		"x:y:w:a:f:"
#ifdef OWN_WINDOW
    "o"
#endif
#ifdef XDBE
    "b"
#endif
#endif /* X11 */
    ;


int main(int argc, char **argv)
{
	struct sigaction act, oact;

	g_signal_pending=0;
	memset(&info, 0, sizeof(info) );

#ifdef TCP_PORT_MONITOR
	tcp_port_monitor_collection_args.min_port_monitors = MIN_PORT_MONITORS_DEFAULT;
	tcp_port_monitor_args.min_port_monitor_connections = MIN_PORT_MONITOR_CONNECTIONS_DEFAULT;
#endif

#if defined(XMMS)
	SET_XMMS_PROJECT_AVAILABLE(info.xmms.project_mask, PROJECT_XMMS);
#endif
#if defined(BMP)
	SET_XMMS_PROJECT_AVAILABLE(info.xmms.project_mask, PROJECT_BMP);
#endif
#if defined(AUDACIOUS)
	SET_XMMS_PROJECT_AVAILABLE(info.xmms.project_mask, PROJECT_AUDACIOUS);
#endif
#if defined(INFOPIPE)
	SET_XMMS_PROJECT_AVAILABLE(info.xmms.project_mask, PROJECT_INFOPIPE);
#endif
		
	/* handle command line parameters that don't change configs */
#ifdef X11
	char *s;
	char temp[10];
	unsigned int x;

	if (((s = getenv("LC_ALL")) && *s) || ((s = getenv("LC_CTYPE")) && 
		     *s) || ((s = getenv("LANG")) && *s)) {
		strcpy(temp, s);
		for(x = 0; x < strlen(s) ; x++) {
			temp[x] = tolower(s[x]);
		}
		if (strstr(temp, "utf-8") || strstr(temp, "utf8")) {
			utf8_mode = 1;
		}
	}
	if (!setlocale(LC_CTYPE, "")) {
		ERR("Can't set the specified locale!\nCheck LANG, LC_CTYPE, LC_ALL.");
	}
#endif /* X11 */
	while (1) {
		int c = getopt(argc,
			       argv,
			       getopt_string);
		if (c == -1)
			break;

		switch (c) {
		case 'v':
		case 'V':
			printf
			    ("Conky " VERSION " compiled " __DATE__ "\n");
			return 0;

		case 'c':
			/* if current_config is set to a strdup of CONFIG_FILE, free it (even
			 * though free() does the NULL check itself;), then load optarg value */
			if (current_config)
				free(current_config);
			current_config = strdup(optarg);
			break;

		case 'h':
			printf
					("Usage: %s [OPTION]...\n"
					"Conky is a system monitor that renders text on desktop or to own transparent\n"
					"window. Command line options will override configurations defined in config\n"
					"file.\n"
					"   -V            version\n"
					"   -c FILE       config file to load instead of "
					CONFIG_FILE
					"\n"
					"   -d            daemonize, fork to background\n"
					"   -h            help\n"
#ifdef X11
					"   -a ALIGNMENT  text alignment on screen, {top,bottom}_{left,right}\n"
					"   -f FONT       font to use\n"
#ifdef OWN_WINDOW
					"   -o            create own window to draw\n"
#endif
#ifdef XDBE
					"   -b            double buffer (prevents flickering)\n"
#endif
					"   -w WIN_ID     window id to draw\n"
					"   -x X          x position\n"
					"   -y Y          y position\n"
#endif /* X11 */
					"   -t TEXT       text to render, remember single quotes, like -t '$uptime'\n"
					"   -u SECS       update interval\n"
					"   -i NUM        number of times to update Conky\n", argv[0]);
			return 0;
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
	/* initalize X BEFORE we load config. (we need to so that 'screen' is set) */
	init_X11();
#endif /* X11 */

	/* load current_config or CONFIG_FILE */

#ifdef CONFIG_FILE
	if (current_config == NULL) {
		/* load default config file */
		char buf[256];

		variable_substitute(CONFIG_FILE, buf, 256);

		if (buf[0] != '\0')
			current_config = strdup(buf);
	}
#endif

	if (current_config != NULL && fopen((const char *)current_config, (const char *)"r"))
		load_config_file(current_config);
	else { 
		set_default_configurations();
	}

#ifdef MAIL_FILE
	if (current_mail_spool == NULL) {
		char buf[256];
		variable_substitute(MAIL_FILE, buf, 256);

		if (buf[0] != '\0')
			current_mail_spool = strdup(buf);
	}
#endif

	/* handle other command line arguments */

#if defined(__FreeBSD__) || defined (__OpenBSD__) || defined(__NetBSD__)
	optind = optreset = 1;
#else
	optind = 0;
#endif

#if defined(__FreeBSD__)
	if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null",
			O_RDONLY, "kvm_open")) == NULL)
		CRIT_ERR( "cannot read kvm");
#endif
	
	while (1) {
		int c = getopt(argc,
			       argv,
			       getopt_string);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			fork_to_background = 1;
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
#ifdef XDBE
		case 'b':
				use_xdbe = 1;
			break;
#endif
#endif /* X11 */
		case 't':
			if (text != original_text)
				free(text);
			text = strdup(optarg);
			convert_escapes(text);
			break;

		case 'u':
			update_interval = strtod(optarg, 0);
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
	extract_variable_text(text);
	if (text != original_text) {
		free(text);
	}
	text = NULL;
	/* fork */
	if (fork_to_background) {
		int pid = fork();
		switch (pid) {
		case -1:
			ERR("can't fork() to background: %s",
			    strerror(errno));
			break;

		case 0:
			/* child process */
			sleep(1);
			fprintf(stderr,"\n");fflush(stderr);
			break;

		default:
			/* parent process */
			fprintf(stderr,"Conky: forked to background, pid is %d\n",pid);
			fflush(stderr);
			return 0;
		}
	}

	update_uname();

	generate_text();
#ifdef X11
	update_text_area();	/* to get initial size of the window */

	init_window
		(own_window,
		 text_width + border_margin * 2 + 1,
		 text_height + border_margin * 2 + 1,
		 set_transparent, background_colour, info.uname_s.nodename, argv, argc);
	
	update_text_area();	/* to position text/window on screen */
#endif /* X11 */

/*#ifdef CAIRO
// why the fuck not?
//do_it();
#endif*/

#ifdef X11
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

	if ( sigaction(SIGINT,&act,&oact) < 0 ||
	     sigaction(SIGUSR1,&act,&oact) < 0 ||
	     sigaction(SIGTERM,&act,&oact) < 0 )
	{
		ERR("error setting signal handler: %s", strerror(errno) );
	}

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	if ( (info.xmms.current_project > PROJECT_NONE) && (create_xmms_thread() !=0) )
	{
	    CRIT_ERR("unable to create xmms_player thread!");
	}
#endif

	main_loop();

#if defined(XMMS) || defined(BMP) || defined(AUDACIOUS) || defined(INFOPIPE)
	if ( info.xmms.thread && (destroy_xmms_thread()!=0) )
        {
            ERR("error destroying xmms_player thread");
        }
#endif	

#if defined(__FreeBSD__)
	kvm_close(kd);
#endif
	
	return 0;
}

void signal_handler(int sig)
{
	/* signal handler is light as a feather, as it should be. 
	 * we will poll g_signal_pending with each loop of conky
	 * and do any signal processing there, NOT here.  pkovacs. */
	g_signal_pending=sig;
}
