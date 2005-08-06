# CVS ebuild for Conky
# $Header$

ECVS_SERVER="cvs.sourceforge.net:/cvsroot/conky"
ECVS_MODULE="conky"
inherit cvs

DESCRIPTION="minimalist system monitor for X based on torsmo"
HOMEPAGE="http://conky.rty.ca"

LICENSE="BSD"
SLOT="0"
KEYWORDS="~amd64 ~ppc ~sparc ~x86"
IUSE="truetype seti metar mldonkey mpd"

RDEPEND="virtual/libc
   virtual/x11
   truetype? ( >=media-libs/freetype-2 )
   seti? ( sci-astronomy/setiathome )
   metar? ( dev-libs/mdsplib )"

DEPEND="truetype? ( >=media-libs/freetype-2 )
   metar? ( dev-libs/mdsplib )
   virtual/x11
   >=sys-devel/automake-1.4
   sys-devel/autoconf
   sys-apps/grep
   sys-apps/sed
   sys-devel/gcc
   >=sys-process/procps-3.2.5
   "

S=${WORKDIR}/conky

src_unpack() {
        cvs_src_unpack
        cd ${S}
        NOCONFIGURE=blah ./autogen.sh
}

src_compile() {
   econf \
      $(use_enable truetype xft) \
      $(use_enable metar) \
      $(use_enable mldonkey) \
      $(use_enable mpd) \
      $(use_enable seti) \
      --enable-double-buffer \
      --enable-own-window \
      --enable-proc-uptime \
      || die "econf failed"
   emake || die "compile failed"
}

src_install() {
   emake DESTDIR=${D} install || die "make install failed"
   dodoc ChangeLog AUTHORS README conkyrc.sample
}

pkg_postinst() {
   einfo 'default configuration file is "~/.conkyrc"'
   einfo "you can find a sample configuration file in"
   einfo "/usr/share/doc/${PF}/conkyrc.sample.gz"
   einfo
   einfo "For more info on Conky's new features,"
   einfo "please look at the README and ChangeLog:"
   einfo "/usr/share/doc/${PF}/README.gz"
   einfo "/usr/share/doc/${PF}/ChangeLog.gz"
   einfo
   einfo "Comment out temperature info lines if you have no kernel"
   einfo "support for it."
   einfo
   ewarn "Conky doesn't work with window managers that"
   ewarn "take control over root window such as Gnome's nautilus."
} 
