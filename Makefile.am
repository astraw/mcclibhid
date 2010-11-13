# http://sourceware.org/autobook/autobook/autobook_39.html#SEC39

include_HEADERS = pmd.h usb-1208FS.h

lib_LTLIBRARIES = libmcclibhid.la
libmcclibhid_la_SOURCES = pmd.c usb-1208FS.c
#libmcclibhid_la_LDFLAGS = -version-info 1.21.0

bin_PROGRAMS = testusb1208FS
testusb1208FS_SOURCES = test-usb1208FS.c
testusb1208FS_LDADD = libmcclibhid.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pkgconfig/libmcclibhid.pc

dist_noinst_DATA = \
	debian/changelog \
	debian/compat \
	debian/control \
	debian/copyright \
	debian/rules
