AM_CPPFLAGS = \
	$(PANEL_WEB_CFLAGS) \
	-DMX_CACHE=\"$(pkgdatadir)/mx.cache\" \
	-fno-exceptions -fno-rtti \
	-I$(srcdir)/../common \
	$(NULL)

libexec_PROGRAMS = dawati-panel-web

dawati_panel_web_LDADD = \
	$(PANEL_WEB_LIBS) \
	$(builddir)/../common/libcommon.a \
	$(NULL)

dawati_panel_web_SOURCES = \
	dawati-panel-web.cc \
	dawati-netbook-netpanel.cc \
	dawati-netbook-netpanel.h \
	mnb-netpanel-bar.cc \
	mnb-netpanel-bar.h \
	$(NULL)

dawati_panel_web_DEPENDENCIES = \
	$(builddir)/../common/libcommon.a \
	$(NULL)

servicedir = $(datadir)/dbus-1/services
service_in_files = com.dawati.UX.Shell.Panels.internet.service.in
service_DATA = com.dawati.UX.Shell.Panels.internet.service

autostartdir = $(DAWATI_PANELS_DIR)
autostart_DATA = dawati-panel-internet.desktop

@INTLTOOL_DESKTOP_RULE@

desktopfiles_in_files = dawati-panel-internet.desktop.in
desktopfiles_files = $(desktopfiles_in_files:.desktop.in=.desktop)

EXTRA_DIST = \
	dawati-panel-internet.desktop.in.in \
	$(service_in_files) \
	$(NULL)

DISTCLEANFILES = \
	$(autostart_DATA) \
	com.dawati.UX.Shell.Panels.internet.service \
	$(NULL)

include $(top_srcdir)/build/autotools/Makefile.am.panels
