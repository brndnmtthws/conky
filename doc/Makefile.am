clean-am: clean-generic mostlyclean-am
	rm -f docs.mxml
	rm -f conky.1

if HAVE_DOCSTUFF

man_MANS = conky.1 ../README docs.html

conky.1.gz: command_options.xml config_settings.xml docs.xml variables.xml 
	${DB2X_XSLPROC} -s man ${srcdir}/docs.xml -o docs.mxml
	${DB2X_MANXML} docs.mxml

docs.html: command_options.xml config_settings.xml docs.xml variables.xml
	${XSLPROC} http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl ${srcdir}/docs.xml > docs.html

../README: conky.1.gz
	man ./conky.1 | col -b > README
	mv README ${top_srcdir}
else

man_MANS = ../README

endif

EXTRA_DIST = conkyrc.sample conky.1.gz docs.html command_options.xml config_settings.xml docgen.sh docs.xml variables.xml
