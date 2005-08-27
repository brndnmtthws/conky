if HAVE_DOCSTUFF

man_MANS = conky.1

clean-am: clean-generic mostlyclean-am
	rm -f docs.mxml
	rm -f conky.1

conky.1: command_options.xml config_settings.xml docs.xml variables.xml 
	${db2x_xsltproc_cmd} -s man ${srcdir}/docs.xml -o docs.mxml
	${db2x_manxml_cmd} docs.mxml
	${xsltproc_cmd} http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl ${srcdir}/docs.xml > docs.html
	man ./conky.1 | col -b > README
	mv README ${top_srcdir}
	xsltproc variables.xsl variables.xml > variables.html
	xsltproc config_settings.xsl config_settings.xml > config_settings.html

DOCS = conky.1 docs.html variables.html config_settings.html
endif

EXTRA_DIST = $(DOCS) conkyrc.sample command_options.xml config_settings.xml docgen.sh docs.xml variables.xml variables.xsl config_settings.xsl
