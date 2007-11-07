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

EXTRA_DIST = hildon-control-panel.pc.in \
	com.nokia.controlpanel.service.in \
	hildon-control-panel.desktop.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = hildon-control-panel.pc

servicefiledir=$(prefix)/share/dbus-1/services
servicefile_DATA=com.nokia.controlpanel.service

gconfschemasdir=$(sysconfdir)/gconf/schemas
gconfschemas_DATA=hildon-control-panel.schemas

hildondesktopentry_DATA = hildon-control-panel.desktop

hildon_othersmenudir = $(sysconfdir)/others-menu

CLEANFILES = *~
