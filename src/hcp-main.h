/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Karoliina Salminen <karoliina.t.salminen@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /* dbus_api_subject_to_change */

#ifndef HILDON_CONTROLPANEL_MAIN_H
#define HILDON_CONTROLPANEL_MAIN_H

/* System includes */
#include <libintl.h>
#include <strings.h>

/* Osso includes */
#include <libosso.h>
#include <osso-helplib.h>

/* GLib includes */
#include <glib.h>
#include <glib/gstdio.h>

/* GDK includes */
#include <gdk/gdkkeysyms.h>

/* GTK includes */
#include <gtk/gtk.h>

/* Hildon widgets */
#include <hildon-widgets/hildon-program.h>
#include <hildon-widgets/hildon-note.h>
#include <hildon-widgets/gtk-infoprint.h>

#include <hildon-base-lib/hildon-base-dnotify.h>
#include <hildon-base-lib/hildon-base-types.h>

/* Control Panel includes */
#include "hcp-applist.h"
#include "hcp-item.h"
#include "hcp-grid.h"
#include "hcp-config-keys.h"


G_BEGIN_DECLS


#define _(a) gettext(a)

#define APP_NAME     "controlpanel"
#define APP_VERSION  "0.1"

#define HILDON_CP_SYSTEM_DIR ".osso/hildon-cp"
#define HILDON_CP_CONF_USER_FILENAME "hildon-cp.conf"
#define HILDON_CP_SYSTEM_DIR_ACCESS   0755
#define HILDON_ENVIRONMENT_USER_HOME_DIR "HOME"
#define HILDON_CP_CONF_FILE_FORMAT    \
 "iconsize=%20s\n"
#define HILDON_CP_MAX_FILE_LENGTH 30 /* CONF FILE FORMAT MUST NOT BE LARGER */



#define OSSO_HELP_ID_CONTROL_PANEL "Utilities_controlpanel_mainview"
#define HILDON_CP_ASSUMED_LOCKCODE_MAXLENGTH 5 /* Note that max number of characters is removed from widget specs. */

/* DBus RPC */
#define HCP_RPC_SERVICE                     "com.nokia.controlpanel"
#define HCP_RPC_PATH                        "/com/nokia/controlpanel/rpc"
#define HCP_RPC_INTERFACE                   "com.nokia.controlpanel.rpc"
#define HCP_RPC_METHOD_RUN_APPLET           "run_applet"
#define HCP_RPC_METHOD_SAVE_STATE_APPLET    "save_state_applet"
#define HCP_RPC_METHOD_TOP_APPLICATION      "top_application"
#define HCP_RPC_METHOD_IS_APPLET_RUNNING    "is_applet_running"

typedef struct _HCP HCP;

struct _HCP {
    HildonProgram *program;
    HildonWindow *window;
    HCPAppList *al;
    HCPItem *focused_item;
    GtkWidget *view;
    GtkWidget *large_icons_menu_item;
    GtkWidget *small_icons_menu_item;
    GList *grids;
    osso_context_t *osso;

    /* for state save data */
    gint icon_size;
    gchar *saved_focused_filename;
    gint scroll_value;
    gint execute;
};
 
gboolean hcp_rfs (HCP *hcp,
                  const char *warning,
                  const char *title,
                  const char *script,
                  const char *help_topic);


G_END_DECLS

#endif
