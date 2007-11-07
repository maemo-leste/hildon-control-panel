# This file is part of hildon-control-panel
#
# Copyright (C) 2003, 2004, 2005, 2006 Nokia Corporation.
#
# Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public License
# version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA
#

INCLUDES = \
	$(HILDON_CFLAGS) \
	$(MAEMO_LAUNCHER_CFLAGS) \
	$(OSSO_CFLAGS)\
	$(HILDON_HELP_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(GNOME_VFS_CFLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPREFIXDIR=\"$(prefix)\" \
	-DCONTROLPANEL_ENTRY_DIR=\"$(hildoncpdesktopentrydir)\" \
	-DHCP_PLUGIN_DIR=\"$(hildoncplibdir)\"

hcp-marshalers.h: hcp-marshalers.list
	$(GLIB_GENMARSHAL) $< --header --prefix=hcp_marshal > $@

hcp-marshalers.c: hcp-marshalers.list
	$(GLIB_GENMARSHAL) $< --header --body --prefix=hcp_marshal > $@

bin_PROGRAMS = controlpanel

controlpanel_SOURCES = \
	$(BUILT_SOURCES) \
	hcp-main.c \
	hcp-main.h \
	hcp-program.c \
	hcp-program.h \
	hcp-window.c \
	hcp-window.h \
	hcp-app.c \
	hcp-app.h \
	hcp-app-list.c \
	hcp-app-list.h \
	hcp-app-view.c \
	hcp-app-view.h \
	hcp-grid.h \
	hcp-grid.c \
	hildon-cp-plugin-interface.h

if USE_MAEMO_TOOLS
controlpanel_SOURCES += \
	hcp-rfs.c \
	hcp-rfs.h
endif

BUILT_SOURCES = hcp-marshalers.c \
                hcp-marshalers.h

controlpanel_LDFLAGS = $(MAEMO_LAUNCHER_LDFLAGS)

controlpanel_LDADD = \
	$(HILDON_LIBS) \
	$(OSSO_LIBS) \
	$(HILDON_HELP_LIBS) \
	$(GCONF_LIBS) \
	$(GNOME_VFS_LIBS)

hildon_cp_pluginincludeinstdir=$(includedir)/hildon-cp-plugin
hildon_cp_pluginincludeinst_DATA = hildon-cp-plugin-interface.h

EXTRA_DIST = hcp-marshalers.list

CLEANFILES = *~ $(BUILT_SOURCES)
