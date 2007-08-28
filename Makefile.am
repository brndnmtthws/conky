SUBDIRS = 					\
	src 					\
	doc

AUTOMAKE_OPTIONS = 				\
	1.8					\
	dist-bzip2

EXTRA_DIST = 					\
	autogen.sh				\
	configure.ac.in				\
	config.rpath				\
	extras/nano/README			\
	extras/nano/conky.nanorc		\
	extras/vim/README			\
	extras/vim/ftdetect/conkyrc.vim		\
	extras/vim/syntax/conkyrc.vim

ACLOCAL_AMFLAGS =				\
	-I m4

DISTCLEANFILES =				\
	configure.ac

distclean-local:
	/bin/rm -rf *.cache *~

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
