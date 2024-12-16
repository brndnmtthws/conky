/*
 *
 * Copyright (c) 2007 Mikko Sysikaski <mikko.sysikaski@gmail.com>
 *					  Toni Spets <toni.spets@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

#ifndef PRSS_H
#define PRSS_H

#include <libxml/parser.h>
#include <string>

typedef struct PRSS_Item_ {
  char *title;
  char *link;
  char *description;
  char *category;
  char *pubDate;
  char *guid;
} PRSS_Item;

class PRSS {
 public:
  char *version;

  char *title;
  char *link;
  char *description;
  char *language;
  char *generator;
  char *managingEditor;
  char *webMaster;
  char *docs;
  char *lastBuildDate;
  char *pubDate;
  char *copyright;
  char *ttl;

  PRSS_Item *items;
  int item_count;

  explicit PRSS(const std::string &xml_data);
  ~PRSS();
};

#endif /* PRSS_H */
