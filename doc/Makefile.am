# -*- mode: Makefile; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: t -*-
#
if HAVE_DOCSTUFF

DOCS = conky.1 docs.html variables.html config_settings.html lua.html

CLEANFILES = docs.mxml conky.1 variable.html config_settings.html lua.html

conky.1: command_options.xml config_settings.xml docs.xml variables.xml lua.xml
	${db2x_xsltproc_cmd} -s man ${srcdir}/docs.xml -o docs.mxml
	${db2x_manxml_cmd} docs.mxml
	${xsltproc_cmd} http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl ${srcdir}/docs.xml > docs.html
	man ./conky.1 > README
	sed -i "s/[[:cntrl:]]\[[0-9]*m//g" README
	mv README ${top_srcdir}
	xsltproc ${srcdir}/variables.xsl ${srcdir}/variables.xml > variables.html
	xsltproc ${srcdir}/config_settings.xsl ${srcdir}/config_settings.xml > config_settings.html
	xsltproc ${srcdir}/lua.xsl ${srcdir}/lua.xml > lua.html

else
conky.1:

endif

man_MANS = conky.1

EXTRA_DIST = $(DOCS) command_options.xml config_settings.xml docgen.sh docs.xml variables.xml variables.xsl config_settings.xsl lua.xml lua.xsl

# vi:set ts=4 sw=4 noet ai nocindent syntax=automake:
