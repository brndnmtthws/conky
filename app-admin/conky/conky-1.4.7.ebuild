# Copyright 1999-2007 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/app-admin/conky/conky-1.4.5.ebuild,v 1.11 2007/07/22 08:19:13 omp Exp $

inherit eutils
# used for epause

DESCRIPTION="Conky is an advanced, highly configurable system monitor for X"
HOMEPAGE="http://conky.sf.net"
SRC_URI="mirror://sourceforge/conky/${P}.tar.bz2"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~alpha amd64 ppc ppc64 sparc x86"
IUSE="truetype X ipv6 audacious bmpx hddtemp mpd vim-syntax"

DEPEND_COMMON="
	virtual/libc
	X? (
		x11-libs/libICE
		x11-libs/libXext
		x11-libs/libX11
		x11-libs/libSM
		x11-libs/libXrender
		x11-libs/libXdamage
		x11-libs/libXft
		truetype? ( >=media-libs/freetype-2 )
		audacious? ( >=media-sound/audacious-0.1 )
		bmpx? ( media-sound/bmpx
				>=sys-apps/dbus-0.35
			)
	)
	!ipv6? ( >=dev-libs/glib-2.0 )"
RDEPEND="${DEPEND_COMMON}
	hddtemp? ( app-admin/hddtemp )
	vim-syntax? ( || ( app-editors/vim
	app-editors/gvim ) )"

DEPEND="
	${DEPEND_COMMON}
	X? (
		x11-libs/libXt
		x11-proto/xextproto
		x11-proto/xproto
	)"

src_compile() {
	local mymake
	if useq ipv6 ; then
		ewarn
		ewarn "You have the ipv6 USE flag enabled.  Please note that using"
		ewarn "the ipv6 USE flag with Conky disables the port monitor."
		epause
	else
		mymake="MPD_NO_IPV6=noipv6"
	fi
	local myconf
	myconf="--enable-proc-uptime"
	if useq X; then
		myconf="${myconf} --enable-x11 --enable-double-buffer --enable-xdamage --enable-own-window"
		myconf="${myconf} $(use_enable truetype xft)"
	else
		myconf="${myconf} --disable-x11 --disable-double-buffer --disable-xdamage --disable-own-window"
		myconf="${myconf} --disable-xft"
	fi
	econf \
		${myconf} \
		$(use_enable audacious) \
		$(use_enable bmpx) \
		$(use_enable hddtemp ) \
		$(use_enable mpd) \
		$(use_enable !ipv6 portmon) || die "econf failed"
	emake ${mymake} || die "compile failed"
}

src_install() {
	emake DESTDIR="${D}" install || die "make install failed"
	dodoc ChangeLog AUTHORS README doc/conkyrc.sample
	dohtml doc/docs.html doc/config_settings.html doc/variables.html

	if use vim-syntax; then
		insinto /usr/share/vim/vimfiles/ftdetect
		doins "${S}"/extras/vim/ftdetect/conkyrc.vim

		insinto /usr/share/vim/vimfiles/syntax
		doins "${S}"/extras/vim/syntax/conkyrc.vim
	fi
}

pkg_postinst() {
	elog 'Default configuration file is "~/.conkyrc"'
	elog "You can find a sample configuration file in"
	elog "/usr/share/doc/${PF}/conkyrc.sample.gz"
	elog
	elog "For more info on Conky's new features,"
	elog "please look at the README and ChangeLog:"
	elog "/usr/share/doc/${PF}/README.gz"
	elog "/usr/share/doc/${PF}/ChangeLog.gz"
	elog "There are also pretty html docs available"
	elog "on Conky's site or in /usr/share/doc/${PF}"
	elog
	elog "Also see http://www.gentoo.org/doc/en/conky-howto.xml"
	elog
	elog "Vim syntax highlighting for conkyrc now enabled with"
	elog "USE=vim-syntax"
	elog
}
