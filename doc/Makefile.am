if HAVE_DOCSTUFF

man_MANS = conky.1 ../README docs.html

clean-am: clean-generic mostlyclean-am
	rm -f docs.mxml
	rm -f conky.1

conky.1: command_options.xml config_settings.xml docs.xml variables.xml 
	${db2x_xsltproc_cmd} -s man ${srcdir}/docs.xml -o docs.mxml
	${db2x_manxml_cmd} docs.mxml

docs.html: command_options.xml config_settings.xml docs.xml variables.xml
	${xsltproc_cmd} http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl ${srcdir}/docs.xml > docs.html

../README: conky.1
	man ./conky.1 | col -b > README
	mv README ${top_srcdir}
else

man_MANS = ../README

endif

EXTRA_DIST = conkyrc.sample conky.1 docs.html command_options.xml config_settings.xml docgen.sh docs.xml variables.xml
