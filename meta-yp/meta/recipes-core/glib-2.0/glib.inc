SUMMARY = "A general-purpose utility library"
DESCRIPTION = "GLib is a general-purpose utility library, which provides many useful data types, macros, type conversions, string utilities, file utilities, a main loop abstraction, and so on."
HOMEPAGE = "https://developer.gnome.org/glib/"

# pcre is under BSD;
# docs/reference/COPYING is with a 'public domai'-like license!
LICENSE = "LGPLv2+ & BSD & PD"
LIC_FILES_CHKSUM = "file://COPYING;md5=3bf50002aefd002f49e7bb854063f7e7 \
                    file://glib/glib.h;beginline=4;endline=17;md5=b4f0f4a399c19e5ebb20c31b79d6bc32 \
                    file://gmodule/COPYING;md5=3bf50002aefd002f49e7bb854063f7e7 \
                    file://gmodule/gmodule.h;beginline=4;endline=17;md5=b4f0f4a399c19e5ebb20c31b79d6bc32 \
                    file://glib/pcre/COPYING;md5=266ebc3ff74ee9ce6fad65577667c0f4 \
                    file://glib/pcre/pcre.h;beginline=11;endline=35;md5=de27f2bf633d20a2b7af0b1983423283 \
                    file://docs/reference/COPYING;md5=f51a5100c17af6bae00735cd791e1fcc"
BUGTRACKER = "http://bugzilla.gnome.org"
SECTION = "libs"

CVE_PRODUCT = "glib"

BBCLASSEXTEND = "native nativesdk"

DEPENDS = "virtual/libiconv libffi zlib glib-2.0-native"
DEPENDS_append = " ${@bb.utils.contains('PTEST_ENABLED', '1', ' dbus', '', d)}"
DEPENDS_class-native = "pkgconfig-native gettext-native libffi-native zlib-native"
DEPENDS_class-nativesdk = "nativesdk-libtool nativesdk-gettext nativesdk-libffi nativesdk-zlib glib-2.0-native"

PACKAGES += "${PN}-codegen ${PN}-utils"

LEAD_SONAME = "libglib-2.0.*"

inherit autotools gettext gtk-doc pkgconfig ptest-gnome upstream-version-is-even bash-completion gio-module-cache python3native manpages

S = "${WORKDIR}/glib-${PV}"

PACKAGECONFIG ??= "system-pcre libmount"
# To use the system pcre it must be configured with --enable-unicode-properties
PACKAGECONFIG[system-pcre] = "--with-pcre=system,--with-pcre=internal,libpcre"
PACKAGECONFIG[libmount] = "--enable-libmount,--disable-libmount,util-linux"
PACKAGECONFIG[manpages] = "--enable-man --with-xml-catalog=${STAGING_ETCDIR_NATIVE}/xml/catalog.xml, --disable-man, libxslt-native xmlto-native"

CORECONF = "--disable-dtrace --disable-fam --disable-libelf --disable-systemtap"

PRINTF = "--enable-included-printf=no"
PRINTF_darwin = "--enable-included-printf=yes"
PRINTF_mingw32 = "--enable-included-printf=yes"
EXTRA_OECONF = "${PRINTF} ${CORECONF}"
EXTRA_OECONF_class-native = "${CORECONF} --disable-selinux"

# Tell configure that we'll have dbus-daemon on the target for the tests
EXTRA_OECONF_class-target_append = " ${@bb.utils.contains('PTEST_ENABLED', '1', ' ac_cv_prog_DBUS_DAEMON=dbus-daemon', '', d)}"

do_configure_prepend() {
	sed -i -e '1s,#!.*,#!${USRBINPATH}/env python3,' ${S}/gio/gdbus-2.0/codegen/gdbus-codegen.in
}

FILES_${PN} = "${libdir}/lib*${SOLIBS} \
               ${libdir}/gio \
               ${libexecdir}/*gio-querymodules \
               ${datadir}/glib-2.0/schemas"
FILES_${PN}-dev += "${libdir}/glib-2.0/include \
                    ${libdir}/gio/modules/lib*${SOLIBSDEV} \
                    ${libdir}/gio/modules/*.la \
                    ${bindir}/glib-genmarshal \
                    ${bindir}/glib-gettextize \
                    ${bindir}/glib-mkenums \
                    ${bindir}/glib-compile-resources \
                    ${datadir}/glib-2.0/gettext/po/Makefile.in.in \
                    ${datadir}/glib-2.0/schemas/gschema.dtd \
                    ${datadir}/glib-2.0/valgrind/glib.supp \
                    ${datadir}/gettext/its"
FILES_${PN}-dbg += "${datadir}/glib-2.0/gdb ${datadir}/gdb"
FILES_${PN}-codegen = "${datadir}/glib-2.0/codegen/*.py \
                       ${bindir}/gdbus-codegen"
FILES_${PN}-utils = "${bindir}/*"

RRECOMMENDS_${PN} += "shared-mime-info"
# When cross compiling for Windows we don't want to include this
RRECOMMENDS_${PN}_remove_mingw32 = "shared-mime-info"

ARM_INSTRUCTION_SET_armv4 = "arm"
ARM_INSTRUCTION_SET_armv5 = "arm"
# Valgrind runtime detection works using hand-written assembly, which
# doesn't support mips16e
CPPFLAGS_append_class-target_mips16e = " -DNVALGRIND=1"

USE_NLS = "yes"

do_install_append () {
	if [ -f ${D}${bindir}/gtester-report ]; then
		sed ${D}${bindir}/gtester-report -i -e '1s|^#!.*|#!/usr/bin/env python3|'
	fi

	# Remove some unpackaged files
	rm -rf ${D}${datadir}/glib-2.0/codegen/__pycache__
	rm -f ${D}${datadir}/glib-2.0/codegen/*.pyc
	rm -f ${D}${datadir}/glib-2.0/codegen/*.pyo

	# Some distros have both /bin/perl and /usr/bin/perl, but we set perl location
	# for target as /usr/bin/perl, so fix it to /usr/bin/perl.
	if [ -f ${D}${bindir}/glib-mkenums ]; then
		sed -i -e '1s,#!.*perl,#! ${USRBINPATH}/env perl,' ${D}${bindir}/glib-mkenums
	fi

	if [ -e ${D}${libdir}/charset.alias ]; then
		rm -f ${D}${libdir}/charset.alias
	fi
}

do_install_append_class-target () {
	# Tests are only installed on targets, not native builds.  Separating this out
	# keeps glib-2.0-native from depending on ${DISTRO_FEATURES}
	if [ -f ${D}${datadir}/installed-tests/glib/gdbus-serialization.test ]; then
		if ${@bb.utils.contains("DISTRO_FEATURES", "x11", "false", "true", d)}; then
			rm ${D}${datadir}/installed-tests/glib/gdbus-serialization.test
		fi
	fi

	# Make sure gio-querymodules is unique among multilibs
	if test "x${MLPREFIX}" != "x"; then
		mv ${D}${libexecdir}/gio-querymodules ${D}${libexecdir}/${MLPREFIX}gio-querymodules
	fi
}

RDEPENDS_${PN}-ptest += "\
            dbus \
            gnome-desktop-testing \
            tzdata \
            tzdata-americas \
            tzdata-asia \
            tzdata-europe \
            tzdata-posix \
            shared-mime-info \
           "

RDEPENDS_${PN}-ptest_append_libc-glibc = "\
            glibc-gconv-utf-16 \
            glibc-charmap-utf-8 \
            glibc-gconv-cp1255 \
            glibc-charmap-cp1255 \
            glibc-gconv-utf-32 \
            glibc-gconv-utf-7 \
            glibc-gconv-euc-jp \
            glibc-gconv-iso8859-1 \
            glibc-gconv-iso8859-15 \
            glibc-charmap-invariant \
            glibc-localedata-translit-cjk-variants \
           "

INSANE_SKIP_${PN}-ptest += "libdir"
