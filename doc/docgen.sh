#!/bin/sh
# $Id$

xsltproc http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl docs.xml > docs.html

db2x_xsltproc -s man docs.xml -o docs.mxml
db2x_manxml docs.mxml

gzip -f conky.1
man ./conky.1.gz | col -b > README
mv README ../
