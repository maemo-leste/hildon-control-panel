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

SUBDIRS = data src 

CLEANFILES = *~

EXTRA_DIST = debian/changelog debian/control debian/copyright debian/rules \
	debian/hildon-control-panel.install \
	debian/hildon-control-panel-dev.install 

deb: dist
	 -mkdir $(top_builddir)/debian-build
	cd $(top_builddir)/debian-build && tar zxf ../$(top_builddir)/$(PACKAGE)-$(VERSION).tar.gz
	cd $(top_builddir)/debian-build/$(PACKAGE)-$(VERSION) && dpkg-buildpackage -rfakeroot 
