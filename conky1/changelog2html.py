#!/usr/bin/env python

import sys

if len(sys.argv) <= 1:
  print >> sys.stderr, "Usage: ./changelog2html.py [changelog file]"
  sys.exit(1)

f = sys.argv[1]

blah = 0

for i in open(f).read().splitlines():
  # ignore empty lines

  if i and i[0].isspace():
    if not '*' in i:
      print '      ' + i.strip()
    else:
      s = i.split('*', 1)[1].strip()
      print '  <LI>' + s.replace('<', '&lt;').replace('>', '&gt;')
  else:
    if blah:
      print '</UL>'
    print '<H3>%s</H3>' % i.strip()
    print '<UL>'
    blah = 1

if blah:
  print '</UL>'
