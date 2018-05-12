/*
 *
 * Conky, a system monitor, based on torsmo
 *
 * Any original torsmo code is licensed under the BSD license
 *
 * All code written since the fork of torsmo is licensed under the GPL
 *
 * Please see COPYING for details
 *
 * Copyright (c) 2004, Hannu Saransaari and Lauri Hakkarainen
 * Copyright (c) 2005-2018 Brenden Matthews, Philip Kovacs, et. al.
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
#include "conky.h"
#include "core.h"
#include "logging.h"
#include "specials.h"
#include "text_object.h"
#include "x11.h"
#include <vector>

/**
 * Length of a character in bytes.
 * @param c first byte of the character
 */
inline int scroll_character_length(char c) {
#ifdef BUILD_X11
  if (utf8_mode.get(*state)) {
    auto uc = static_cast<unsigned char>(c);
    int len = 0;

    if (c == -1) {
      return 1;
    }

    if ((uc & 0x80) == 0) {
      return 1;
    }

    while (len < 7 && (uc & (0x80 >> len)) != 0) {
      ++len;
    }

    return len;
  }
#endif

  return 1;
}

/**
 * Check if a byte should be skipped when counting characters to scroll text to
 * right.
 */
inline bool scroll_check_skip_byte(char c) {
#ifdef BUILD_X11
  if (utf8_mode.get(*state)) {
    // Check if byte matches UTF-8 continuation byte pattern (0b10xxxxxx)
    if ((c & 0xC0) == 0x80) {
      return true;
    }
  }
#endif

  return SPECIAL_CHAR == c;
}

#define SCROLL_LEFT 1
#define SCROLL_RIGHT 2
#define SCROLL_WAIT 3

struct scroll_data {
  char *text;
  unsigned int show;
  unsigned int step;
  int wait;
  unsigned int wait_arg;
  signed int start;
  long resetcolor;
  int direction;
};

/**
 * Get count of characters to right from (sd->start) position.
 */
static unsigned int scroll_count_characters_to_right(
    struct scroll_data *sd, const std::vector<char> &buf) {
  unsigned int n = 0;
  int offset = sd->start;

  while ('\0' != buf[offset] && offset < buf.size()) {
    offset += scroll_character_length(buf[offset]);
    ++n;
  }

  return n;
}

static void scroll_scroll_left(struct scroll_data *sd,
                               const std::vector<char> &buf,
                               unsigned int amount) {
  for (int i = 0;
       (i < amount) && (buf[sd->start] != '\0') && (sd->start < buf.size());
       ++i) {
    sd->start += scroll_character_length(buf[sd->start]);
  }

  if (buf[sd->start] == 0 || sd->start > strlen(buf.data())) {
    sd->start = 0;
  }
}

static void scroll_scroll_right(struct scroll_data *sd,
                                const std::vector<char> &buf,
                                unsigned int amount) {
  for (int i = 0; i < amount; ++i) {
    if (sd->start <= 0) {
      sd->start = static_cast<int>(strlen(&(buf[0])));
    }

    while (--(sd->start) >= 0) {
      if (!scroll_check_skip_byte(buf[sd->start])) {
        break;
      }
    }
  }
}

void parse_scroll_arg(struct text_object *obj, const char *arg,
                      void *free_at_crash, char *free_at_crash2) {
  struct scroll_data *sd;
  int n1 = 0, n2 = 0;
  char dirarg[6];

  sd = static_cast<struct scroll_data *>(malloc(sizeof(struct scroll_data)));
  memset(sd, 0, sizeof(struct scroll_data));

  sd->resetcolor = get_current_text_color();
  sd->step = 1;
  sd->direction = SCROLL_LEFT;

  if ((arg != nullptr) && sscanf(arg, "%5s %n", dirarg, &n1) == 1) {
    if (strcasecmp(dirarg, "right") == 0 || strcasecmp(dirarg, "r") == 0) {
      sd->direction = SCROLL_RIGHT;
    } else if (strcasecmp(dirarg, "wait") == 0 ||
               strcasecmp(dirarg, "w") == 0) {
      sd->direction = SCROLL_WAIT;
    } else if (strcasecmp(dirarg, "left") != 0 &&
               strcasecmp(dirarg, "l") != 0) {
      n1 = 0;
    }
  }

  if ((arg == nullptr) || sscanf(arg + n1, "%u %n", &sd->show, &n2) <= 0) {
    free(sd);
#ifdef BUILD_X11
    free(obj->next);
#endif
    free(free_at_crash2);
    CRIT_ERR(obj, free_at_crash,
             "scroll needs arguments: [left|right|wait] <length> [<step>] "
             "[interval] <text>");
  }
  n1 += n2;

  if (sscanf(arg + n1, "%u %n", &sd->step, &n2) == 1) {
    n1 += n2;
  } else {
    sd->step = 1;
  }

  if (sscanf(arg + n1, "%u %n", &sd->wait_arg, &n2) == 1) {
    n1 += n2;
    sd->wait = sd->wait_arg;
  } else {
    sd->wait_arg = sd->wait = 0;
  }

  sd->text = static_cast<char *>(malloc(strlen(arg + n1) + sd->show + 1));

  if (strlen(arg) > sd->show && sd->direction != SCROLL_WAIT) {
    for (n2 = 0; static_cast<unsigned int>(n2) < sd->show; n2++) {
      sd->text[n2] = ' ';
    }
    sd->text[n2] = 0;
  } else {
    sd->text[0] = 0;
  }

  strcat(sd->text, arg + n1);
  sd->start = sd->direction == SCROLL_WAIT ? strlen(sd->text) : 0;
  obj->sub =
      static_cast<struct text_object *>(malloc(sizeof(struct text_object)));
  extract_variable_text_internal(obj->sub, sd->text);

  obj->data.opaque = sd;

#ifdef BUILD_X11
  /* add a color object right after scroll to reset any color changes */
#endif /* BUILD_X11 */
}

void print_scroll(struct text_object *obj, char *p, int p_max_size) {
  auto *sd = static_cast<struct scroll_data *>(obj->data.opaque);
  unsigned int j, colorchanges = 0, frontcolorchanges = 0,
                  visibcolorchanges = 0, strend;
  unsigned int visiblechars = 0;
  char *pwithcolors;
  std::vector<char> buf(max_user_text.get(*state), static_cast<char>(0));

  if (sd == nullptr) {
    return;
  }

  generate_text_internal(&(buf[0]), max_user_text.get(*state), *obj->sub);
  for (j = 0; buf[j] != 0; j++) {
    switch (buf[j]) {
      case '\n':  // place all the lines behind each other with LINESEPARATOR
                  // between them
#define LINESEPARATOR '|'
        buf[j] = LINESEPARATOR;
        break;
      case SPECIAL_CHAR:
        colorchanges++;
        break;
    }
  }
  // no scrolling necessary if the length of the text to scroll is too short
  if (strlen(&(buf[0])) - colorchanges <= sd->show) {
    snprintf(p, p_max_size, "%s", &(buf[0]));
    return;
  }
  // if length of text changed to shorter so the (sd->start) is already
  // outside of actual text then reset (sd->start)
  if (sd->start >= strlen(&(buf[0]))) {
    sd->start = 0;
  }
  // make sure a colorchange at the front is not part of the string we are going
  // to show
  while (buf[sd->start] == SPECIAL_CHAR) {
    sd->start++;
  }
  // place all chars that should be visible in p, including colorchanges
  for (j = 0, visiblechars = 0; visiblechars < sd->show;) {
    char c = p[j] = buf[sd->start + j];
    if (0 == c) {
      break;
    }

    ++j;

    if (SPECIAL_CHAR == c) {
      ++visibcolorchanges;
    } else {
      int l = scroll_character_length(c);

      while (--l != 0) {
        p[j] = buf[sd->start + j];
        ++j;
      }

      ++visiblechars;
    }
  }
  for (; visiblechars < sd->show; j++, visiblechars++) {
    p[j] = ' ';
  }
  p[j] = 0;
  // count colorchanges in front of the visible part and place that many
  // colorchanges in front of the visible part
  for (j = 0; j < static_cast<unsigned>(sd->start); j++) {
    if (buf[j] == SPECIAL_CHAR) {
      frontcolorchanges++;
    }
  }
  pwithcolors = static_cast<char *>(
      malloc(strlen(p) + 4 + colorchanges - visibcolorchanges));
  for (j = 0; j < frontcolorchanges; j++) {
    pwithcolors[j] = SPECIAL_CHAR;
  }
  pwithcolors[j] = 0;
  strcat(pwithcolors, p);
  strend = strlen(pwithcolors);
  // and place the colorchanges not in front or in the visible part behind the
  // visible part
  for (j = 0; j < colorchanges - frontcolorchanges - visibcolorchanges; j++) {
    pwithcolors[strend + j] = SPECIAL_CHAR;
  }
  pwithcolors[strend + j] = 0;
  strcpy(p, pwithcolors);
  free(pwithcolors);
  // scroll
  if (sd->direction == SCROLL_LEFT) {
    scroll_scroll_left(sd, buf, sd->step);
  } else if (sd->direction == SCROLL_WAIT) {
    unsigned int charsleft = scroll_count_characters_to_right(sd, buf);

    if (sd->show >= charsleft) {
      if ((sd->wait_arg != 0u) && (--sd->wait <= 0 && sd->wait_arg != 1)) {
        sd->wait = sd->wait_arg;
      } else {
        sd->start = 0;
      }
    } else {
      if ((sd->wait_arg == 0u) || sd->wait_arg == 1 ||
          ((sd->wait_arg != 0u) && sd->wait-- <= 0)) {
        sd->wait = 0;

        if (sd->step < charsleft) {
          scroll_scroll_left(sd, buf, sd->step);
        } else {
          scroll_scroll_left(sd, buf, charsleft);
        }
      }
    }
  } else {
    scroll_scroll_right(sd, buf, sd->step);
  }
#ifdef BUILD_X11
  // reset color when scroll is finished
  if (out_to_x.get(*state)) {
    new_special(p + strlen(p), FG)->arg = sd->resetcolor;
  }
#endif
}

void free_scroll(struct text_object *obj) {
  auto *sd = static_cast<struct scroll_data *>(obj->data.opaque);

  if (sd == nullptr) {
    return;
  }

  free_and_zero(sd->text);
  free_text_objects(obj->sub);
  free_and_zero(obj->sub);
  free_and_zero(obj->data.opaque);
}
