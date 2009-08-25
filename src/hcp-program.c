/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2006 Nokia Corporation.
 *
 * Author: Lucas Rocha <lucas.rocha@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "hcp-program.h"
#include "hcp-window.h"
#include "hcp-app-list.h"
#include "hcp-app.h"

G_DEFINE_TYPE (HCPProgram, hcp_program, G_TYPE_OBJECT);

#define HCP_APP_NAME     "controlpanel"
#define HCP_APP_VERSION  "0.1"

#define HCP_RPC_SERVICE                     "com.nokia.controlpanel"
#define HCP_RPC_PATH                        "/com/nokia/controlpanel/rpc"
#define HCP_RPC_INTERFACE                   "com.nokia.controlpanel.rpc"
#define HCP_RPC_METHOD_RUN_APPLET           "run_applet"
#define HCP_RPC_METHOD_SAVE_STATE_APPLET    "save_state_applet"
#define HCP_RPC_METHOD_TOP_APPLICATION      "top_application"
#define HCP_RPC_METHOD_IS_APPLET_RUNNING    "is_applet_running"

static void
hcp_program_show_window (HCPProgram *program)
{
  program->window = hcp_window_new ();

  gtk_widget_show_all (program->window);
}

static gint 
hcp_program_rpc_handler (const gchar *interface,
                         const gchar *method,
                         GArray *arguments,
                         HCPProgram *program,
                         osso_rpc_t *retval)
{
  g_return_val_if_fail (method, OSSO_ERROR);

  if ((!strcmp (method, HCP_RPC_METHOD_RUN_APPLET)))
  {
    osso_rpc_t applet, user_activated;
    GHashTable *apps = NULL;
    HCPApp *app = NULL;

    if (arguments->len != 2)
      goto error;

    applet = g_array_index (arguments, osso_rpc_t, 0);
    user_activated = g_array_index (arguments, osso_rpc_t, 1);

    if (applet.type != DBUS_TYPE_STRING)
      goto error;
    
    if (user_activated.type != DBUS_TYPE_BOOLEAN)
      goto error;

    g_object_get (G_OBJECT (program->al),
                  "apps", &apps,
                  NULL);

    app = g_hash_table_lookup (apps, applet.value.s);

    if (app)
    {
      if (!hcp_app_is_running (app))
      {
          hcp_app_launch (app, user_activated.value.b);
      }

      if (GTK_IS_WINDOW (program->window))
          gtk_window_present (GTK_WINDOW (program->window));

      retval->type = DBUS_TYPE_INT32;
      retval->value.i = 0;

      return OSSO_OK;
    }
  }
  else if ((!strcmp (method, HCP_RPC_METHOD_SAVE_STATE_APPLET)))
  {
    osso_rpc_t applet;
    GHashTable *apps = NULL;
    HCPApp *app = NULL;
    
    if (arguments->len != 1)
      goto error;

    applet = g_array_index (arguments, osso_rpc_t, 0);

    if (applet.type != DBUS_TYPE_STRING)
      goto error;
    
    g_object_get (G_OBJECT (program->al),
                  "apps", &apps,
                  NULL);

    app = g_hash_table_lookup (apps, applet.value.s);

    if (app)
    {
      if (hcp_app_is_running (app))
      {
        hcp_app_save_state (app);
      }

      retval->type = DBUS_TYPE_INT32;
      retval->value.i = 0;

      return OSSO_OK;
    }
  }
  else if ((!strcmp (method, HCP_RPC_METHOD_IS_APPLET_RUNNING)))
  {
    osso_rpc_t applet;
    GHashTable *apps = NULL;
    HCPApp *app = NULL;
    
    if (arguments->len != 1)
      goto error;

    applet = g_array_index (arguments, osso_rpc_t, 0);

    if (applet.type != DBUS_TYPE_STRING)
      goto error;
    
    g_object_get (G_OBJECT (program->al),
                  "apps", &apps,
                  NULL);

    app = g_hash_table_lookup (apps, applet.value.s);

    retval->type = DBUS_TYPE_BOOLEAN;
    retval->value.b = (app && hcp_app_is_running (app))?
                            TRUE:
                            FALSE;

    return OSSO_OK;
  }
  else if ((!strcmp (method, HCP_RPC_METHOD_TOP_APPLICATION)))
  {
    if (!program->window)
      hcp_program_show_window (program);
    else
      gtk_window_present (GTK_WINDOW (program->window));

    retval->type = DBUS_TYPE_INT32;
    retval->value.i = 0;
    return OSSO_OK;
  }

error:
  retval->type = DBUS_TYPE_INT32;
  retval->value.i = -1;

  return OSSO_ERROR;
}

static void 
hcp_program_init_rpc (HCPProgram *program)
{
  g_return_if_fail (program);
  g_return_if_fail (HCP_IS_PROGRAM (program));

  program->osso = osso_initialize (HCP_APP_NAME, HCP_APP_VERSION, TRUE, NULL);
  
  if (!program->osso)
  {
    g_warning ("Error initializing osso -- check that D-BUS is running");
    exit(-1);
  }

  osso_rpc_set_default_cb_f (program->osso,
                     (osso_rpc_cb_f *) hcp_program_rpc_handler,
                     program);
}

static void 
hcp_program_hw_signal_cb (osso_hw_state_t *state, HCPProgram *program)
{
  g_return_if_fail (program);
  g_return_if_fail (HCP_IS_PROGRAM (program));

  if (state != NULL) 
  {
    if (state->shutdown_ind)
    {
      if (program->window)
      {
        hcp_window_close (HCP_WINDOW (program->window));
      }
    }
  }
}

static void
hcp_program_init (HCPProgram *program)
{
  program->execute = 0;

  program->al = (HCPAppList *) hcp_app_list_new ();
  hcp_app_list_update (program->al);

  hcp_program_init_rpc (program);

  osso_hw_set_event_cb (program->osso,
                        NULL,
                        (osso_hw_cb_f *) hcp_program_hw_signal_cb,
                        program);
}

static void
hcp_program_finalize (GObject *object)
{
  HCPProgram *program;
  
  g_return_if_fail (object);
  g_return_if_fail (HCP_IS_PROGRAM (object));

  program = HCP_PROGRAM (object);

  if (program->al != NULL) 
  {
    g_object_unref (program->al);
    program->al = NULL;
  }

  if (program->osso)
  {
      osso_deinitialize (program->osso);
  }

  G_OBJECT_CLASS (hcp_program_parent_class)->finalize (object);
}

static void
hcp_program_class_init (HCPProgramClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hcp_program_finalize;
}

HCPProgram *
hcp_program_get_instance (void)
{
  static HCPProgram *instance;
  
  if (!instance) 
  {
    instance = HCP_PROGRAM (g_object_new (HCP_TYPE_PROGRAM, NULL));
  }
  
  return instance;
}

void
hcp_program_run (HCPProgram *program)
{
  gboolean dbus_activated;

  g_return_if_fail (program);
  g_return_if_fail (HCP_IS_PROGRAM (program));

  dbus_activated = g_getenv ("DBUS_STARTER_BUS_TYPE")?TRUE:FALSE;

#if 0
  if (!dbus_activated)
  {
    /* When started from the command line we show the UI as default
     * behavior. When dbus activated, we wait to see if we got
     * top_application or run_applet method call */
    hcp_program_show_window (hcp);
  }
#endif

  /* Always start the user interface for now */
  hcp_program_show_window (program);
}
