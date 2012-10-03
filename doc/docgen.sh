#!/bin/sh

xsltproc http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl docs.xml > docs.html && \
db2x_xsltproc -s man docs.xml -o docs.mxml && \
db2x_manxml docs.mxml && \
{ echo ".TH CONKY 1 \"August 2005\" \"conky compiled August 2005\" \"User Commands\""; sed 1d < conky.1; } > conky.2 && \
mv conky.2 conky.1 && \
man ./conky.1 | col -b > README && \
mv README ../
