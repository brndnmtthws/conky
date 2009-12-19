include(Docbook)

if(MAINTAINER_MODE)
	wrap_xsltproc(lua config_settings variables)
	wrap_man(docs)
endif(MAINTAINER_MODE)
