man_MANS = conky.1.gz

clean-am: clean-generic mostlyclean-am
	rm -f docs.mxml
	rm -f conky.1.gz

if HAVE_DOCSTUFF
conky.1.gz: command_options.xml config_settings.xml docgen.sh docs.xml variables.xml
	 ${XSLPROC} http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl ${srcdir}/docs.xml > docs.html
	${DB2X_XSLPROC} -s man ${srcdir}/docs.xml -o docs.mxml
	${DB2X_MANXML} docs.mxml
	gzip -f conky.1
	man ./conky.1.gz | col -b > README
	mv README ${top_srcdir}
endif

EXTRA_DIST = conkyrc.sample conky.1.gz docs.html command_options.xml config_settings.xml docgen.sh docs.xml variables.xml
