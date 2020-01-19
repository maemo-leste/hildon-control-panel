/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005-2008 Nokia Corporation.
 *
 * Author: Lucas Rocha <lucas.rocha@nokia.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libosso.h>
#include <hildon/hildon-window.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-defines.h>
#include <hildon/hildon-pannable-area.h>
#include <hildon/hildon-button.h>
#include <hildon/hildon-helper.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <dbus/dbus-glib.h>

#include "hcp-window.h"
#include "hcp-program.h"
#include "hcp-app-list.h"
#include "hcp-app-view.h"
#include "hcp-app.h"
#include "hcp-grid.h"
#include "hcp-config-keys.h"

#ifdef MAEMO_TOOLS
#include "hcp-rfs.h"
#endif

#define HCP_TITLE             _("copa_ap_cp_name")
#define HCP_MENU_RFS          _("copa_me_tools_rfs")
#define HCP_MENU_CUD          _("copa_me_tools_cud")
#if HCP_WITH_SIM
#define HCP_MENU_SIM          dgettext("osso-connectivity-ui",\
                                  "conn_ti_enter_sim_unlock_code")
#endif


#define HCP_STATE_GROUP         "HildonControlPanel"
#define HCP_STATE_FOCUSED       "Focussed"
#define HCP_STATE_SCROLL_VALUE  "ScrollValue"
#define HCP_STATE_EXECUTE       "Execute"

#define HCP_OPERATOR_WIZARD_DBUS_SERVICE "operator_wizard"
#define HCP_OPERATOR_WIZARD_LAUNCH       "launch_operator_wizard"

#define HCP_RFS_WARNING        _("refs_ia_text")
#define HCP_RFS_SCRIPT         "/usr/sbin/osso-app-killer-rfs.sh"

#define HCP_CUD_WARNING        _("cud_ia_text")
#define HCP_CUD_SCRIPT         "/usr/sbin/osso-app-killer-cud.sh"

#define HCP_SIM_SCRIPT         "/usr/bin/startup-pin-query"

#define HCP_WITH_ROS           1
#define HCP_WITH_CUD           1
#define HCP_PORTRAIT_DEBUG     0 /* Only for debugging purposes */

#if HCP_WITH_SIM
#define	 HCP_SIMLOCK_NAME          "com.nokia.phone.SIM"
#define	 HCP_SIMLOCK_PATH          "/com/nokia/phone/SIM"
#define	 HCP_SIMLOCK_INTERFACE     "Phone.Sim"
#define  HCP_SIMLOCK_CHECK_METHOD  "read_simlock_status"

enum simlock_restriction_status{
  HCP_SIMLOCK_RESTRICTED = 2,
  HCP_SIMLOCK_RESTRICTION_ON = 3,
  HCP_SIMLOCK_RESTRICTION_PENDING = 4,
  HCP_SIMLOCK_STATE_NOT_INITIALIZED = 5,
  HCP_SIMLOCK_NO_SERVICE = 6,
  HCP_SIMLOCK_NOT_READY = 7,
  HCP_SIMLOCK_ERROR = 8
};
#endif /* if HCP_WITH_SIM */

struct _HCPWindowPrivate 
{
  HCPApp         *focused_item;
  HCPAppList     *al;
  GtkWidget      *view;
  GtkWidget      *align;

  /* For state save data */
  gchar          *saved_focused_filename;
  gint            scroll_value;

#if HCP_WITH_SIM
  GtkWidget      *mi_simlock;
#endif
};

G_DEFINE_TYPE_WITH_CODE (HCPWindow, hcp_window, HILDON_TYPE_STACKABLE_WINDOW, G_ADD_PRIVATE(HCPWindow));


#if HCP_WITH_SIM
static void
hcp_window_check_simlock (HCPWindow *window);
#endif

static void 
hcp_window_enforce_state (HCPWindow *window)
{
  HCPWindowPrivate *priv;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  /* Actually enforce the saved state */
  /* If the saved focused item filename is defined, try to 
   * focus on the item. */
  if (priv->saved_focused_filename)
  {
    GHashTable *apps = NULL;
    HCPApp *app = NULL;

    g_object_get (G_OBJECT (priv->al),
                  "apps", &apps,
                  NULL);

    app = g_hash_table_lookup (apps,
                               priv->saved_focused_filename);
    
    if (app)
 /*     hcp_app_focus (app); */
      priv->focused_item = app;

    g_free (priv->saved_focused_filename);
    priv->saved_focused_filename = NULL;
  }

  /* HCPProgram will start the possible plugin in 
   * program->execute */
}

static void 
hcp_window_retrieve_state (HCPWindow *window)
{
  HCPWindowPrivate *priv;
  HCPProgram *program = hcp_program_get_instance ();
  osso_state_t state = { 0, };
  GKeyFile *keyfile = NULL;
  osso_return_t ret;
  GError *error = NULL;
  gchar *focused = NULL;
  gint scroll_value;
  gboolean execute;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  ret = osso_state_read (program->osso, &state);

  if (ret != OSSO_OK)
  {
    g_warning ("An error occured when reading application state");
    return;
  }

  if (state.state_size == 1)
  {
    /* Clean state, return */
    goto cleanup;
  }

  keyfile = g_key_file_new ();

  g_key_file_load_from_data (keyfile,
                             state.state_data,
                             state.state_size,
                             G_KEY_FILE_NONE,
                             &error);

  if (error)
  {
    g_warning ("An error occured when reading application state: %s",
               error->message);
    goto cleanup;
  }

  focused = g_key_file_get_string (keyfile,
                                   HCP_STATE_GROUP,
                                   HCP_STATE_FOCUSED,
                                   &error);

  if (error)
  {
    g_warning ("An error occured when reading application state: %s",
               error->message);
    goto cleanup;
  }

  if (g_str_has_suffix (focused, ".so"))
  {
    priv->saved_focused_filename = focused;
  }
  else
  {
    priv->saved_focused_filename = NULL;
    g_free (focused);
  }
	  
  scroll_value = g_key_file_get_integer (keyfile,
                                         HCP_STATE_GROUP,
                                         HCP_STATE_SCROLL_VALUE,
                                         &error);

  if (error)
  {
    g_warning ("An error occured when reading application state: %s",
               error->message);
    goto cleanup;
  }
  
  priv->scroll_value = scroll_value;
  
  execute = g_key_file_get_boolean (keyfile,
                                    HCP_STATE_GROUP,
                                    HCP_STATE_EXECUTE,
                                    &error);

  if (error)
  {
    g_warning ("An error occured when reading application state: %s",
               error->message);
    goto cleanup;
  }
  
  program->execute = execute;

cleanup:
  if (error)
    g_error_free (error);

  g_free (state.state_data);

  if (keyfile)
    g_key_file_free (keyfile);
}

static void 
hcp_window_save_state (HCPWindow *window, gboolean clear_state)
{
  HCPWindowPrivate *priv;
  HCPProgram *program = hcp_program_get_instance ();
  osso_state_t state = { 0, };
  gsize size;
  GKeyFile *keyfile = NULL;
  osso_return_t ret;
  gchar *focused = NULL;
  GError *error = NULL;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  if (clear_state)
  {
    state.state_data = "";
    state.state_size = 1;

    ret = osso_state_write (program->osso, &state);

    if (ret != OSSO_OK)
      g_warning ("An error occured when clearing application state");

    return;
  }

  keyfile = g_key_file_new ();

  g_object_get (G_OBJECT (priv->focused_item),
                "plugin", &focused,
                NULL);

  g_key_file_set_string (keyfile,
                         HCP_STATE_GROUP,
                         HCP_STATE_FOCUSED,
                         priv->focused_item?focused:"");

  g_free (focused);
  
  g_key_file_set_integer (keyfile,
                          HCP_STATE_GROUP,
                          HCP_STATE_SCROLL_VALUE,
                          priv->scroll_value);

  g_key_file_set_boolean (keyfile,
                          HCP_STATE_GROUP,
                          HCP_STATE_EXECUTE,
                          program->execute);

  state.state_data = g_key_file_to_data (keyfile,
                                         &size,
                                         &error);

  state.state_size = size;

  if (error)
    goto cleanup;

  ret = osso_state_write (program->osso, &state);

  if (ret != OSSO_OK)
  {
    g_warning ("An error occured when writing application state");
  }

  /* If a plugin is running, save its state */
  if (program->execute && priv->focused_item && 
      hcp_app_is_running (priv->focused_item))
  {
    hcp_app_save_state (priv->focused_item);
  }

cleanup:
  if (error)
  {
    g_warning ("An error occured when reading application state: %s",
               error->message);
    g_error_free (error);
  }

  g_free (state.state_data);

  if (keyfile)
    g_key_file_free (keyfile);
}

/* Retrieve the configuration (large/small icons)  */
static void 
hcp_window_retrieve_configuration (HCPWindow *window)
{
  GConfClient *client = NULL;
  GError *error = NULL;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  client = gconf_client_get_default ();

  g_return_if_fail (client);

  if (error)
  {
    g_warning ("Error reading window settings from GConf: %s", error->message);
    g_error_free (error);
  }

  g_object_unref (client);
}

static gint 
hcp_window_keyboard_listener (GtkWidget * widget,
                              GdkEventKey * keyevent, 
		              gpointer data)
{
  g_return_val_if_fail (widget, FALSE);
  g_return_val_if_fail (HCP_IS_WINDOW (widget), FALSE);

  if (keyevent->type == GDK_KEY_RELEASE) 
  {
    switch (keyevent->keyval)
    {
        default:
          break;
    }
  }
  
  return FALSE;
}

#ifdef MAEMO_TOOLS
#if HCP_WITH_CUD
static gboolean 
hcp_window_clear_user_data (GtkWidget *widget, HCPWindow *window)
{
  textdomain(PACKAGE);
  hcp_rfs (HCP_CUD_WARNING,
           HCP_CUD_SCRIPT);

  return TRUE;
}
#endif
#if HCP_WITH_ROS
static gboolean 
hcp_window_reset_factory_settings (GtkWidget *widget, HCPWindow *window)
{
  textdomain(PACKAGE);
  hcp_rfs (HCP_RFS_WARNING,
           HCP_RFS_SCRIPT);

  return TRUE;
}
#endif
#if HCP_WITH_SIM
static gboolean 
hcp_window_sim_unlock (GtkWidget *widget, HCPWindow *window)
{
  textdomain(PACKAGE);
  hcp_rfs_simlock ();
  return TRUE;
}
#endif
#if HCP_PORTRAIT_DEBUG
static gboolean
hcp_portrait_debug (GtkWidget *widget, HCPWindow *window)
{
    gint   flag = 0;
    static gboolean portrait = FALSE;

    portrait = ! portrait;

    if (portrait)
        flag = HILDON_PORTRAIT_MODE_REQUEST;

    g_debug ("Portrait mode = %s", portrait ? "true" : "false");
    hildon_gtk_window_set_portrait_flags (GTK_WINDOW (window), flag);

    return TRUE;
}
#endif
#endif /* ifdef MAEMO_TOOLS */

static void 
hcp_window_topmost_status_change (GObject *gobject, 
		                  GParamSpec *arg1,
			          HCPWindow *window)
{
  HCPWindowPrivate *priv;
  HildonProgram *program = HILDON_PROGRAM (gobject);

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  if (hildon_program_get_is_topmost (program)) {
    hildon_program_set_can_hibernate (program, FALSE);
  } else {
    /* Do not set ourselves as background killable if we are
     * running an applet which doesn't implement state-saving */
    if (!priv->focused_item ||
        (!hcp_app_is_running (priv->focused_item) || 
         hcp_app_can_save_state (priv->focused_item)))
    {
        hcp_window_save_state (window, FALSE);
        hildon_program_set_can_hibernate (program, TRUE);
    }
  }
}

static void 
hcp_window_app_view_focus_cb (HCPAppView *view,
                              HCPApp     *app, 
                              HCPWindow  *window)
{
  HCPWindowPrivate *priv;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  if (priv->focused_item != NULL)
    g_object_unref (priv->focused_item);

  /* Increment reference count to avoid object
   * destruction on HCPAppList update. */
  priv->focused_item = g_object_ref (app);
}

static void 
hcp_window_app_list_updated_cb (HCPAppList *al, HCPWindow *window)
{
  HCPWindowPrivate *priv;
  GHashTable *apps = NULL;
  HCPApp *app = NULL;
  gchar *focused = NULL;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  /* Update the view */
  hcp_app_view_populate (HCP_APP_VIEW (priv->view), al);

  gtk_widget_show_all (priv->view);
    
  if (priv->focused_item == NULL)
    return;
	  
  g_object_get (G_OBJECT (priv->focused_item),
                "plugin", &focused,
                NULL);

  g_object_unref (priv->focused_item);
  window->priv->focused_item = NULL;

  g_object_get (G_OBJECT (priv->al),
                "apps", &apps,
                NULL);

  app = g_hash_table_lookup (apps,
                             focused);

  g_free (focused);

  if (app)
    hcp_app_focus (app);

  hcp_window_enforce_state (window);
}

static void hcp_window_quit (GtkWidget *widget, HCPWindow *window)
{
  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  /* we can only close the window, when no applets are running */
  /**@TODO review this */
  HCPProgram *program = hcp_program_get_instance ();
  program->execute = 0;

  hcp_window_save_state (window, FALSE);

  gtk_widget_destroy (GTK_WIDGET (window));

  gtk_main_quit ();
}

static gboolean
hcp_take_screenshot (gpointer data)
{
  g_debug("taking screenshot");
  hildon_gtk_window_take_screenshot(GTK_WINDOW(data), TRUE);
  return FALSE;
}

static gboolean
_expose_cb (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  HCPProgram *program = hcp_program_get_instance ();
  
  g_timeout_add (80, hcp_take_screenshot, program->window);

  /* we only need to call this once */
  g_signal_handler_disconnect(G_OBJECT(data), program->handler_id);
  return FALSE;
}
#if HCP_WITH_SIM
static void
_simlock_cb (DBusGProxy* proxy, DBusGProxyCall* call, HCPWindow *window)
{
  gint ret;
  GError *err = NULL;

  HCPWindowPrivate *priv;
  GtkWidget *mi;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  mi = priv->mi_simlock;
  g_return_if_fail (mi);
  g_return_if_fail (GTK_IS_WIDGET (mi));

  if (!dbus_g_proxy_end_call (proxy, call,
                          &err, G_TYPE_INT,
                          &ret, G_TYPE_INVALID)) {
    g_warning ("unlock - communication error: %s\n", err->message);
    g_error_free (err);
  } else {
    if ( ret != HCP_SIMLOCK_STATE_NOT_INITIALIZED &&
         ret != HCP_SIMLOCK_NO_SERVICE &&
         ret != HCP_SIMLOCK_NOT_READY &&
         ret != HCP_SIMLOCK_ERROR ) {
      /* show unlock button */
      gtk_widget_set_no_show_all (mi, FALSE);
      gtk_widget_show_all (mi);
    } else {
      /* hide unlock button */
      gtk_widget_hide (mi);
      gtk_widget_set_no_show_all (mi, TRUE);
    }
  }
}

static void
hcp_window_check_simlock (HCPWindow* window)
{
  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  DBusGConnection *conn;
  GError *err = NULL;

  conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &err);
  if (!conn) {
    g_warning ("Could not connect to dbus: %s\n", err->message);
    g_error_free (err);
  } else {
    DBusGProxy *proxy;
    proxy = dbus_g_proxy_new_for_name (conn,
                                       HCP_SIMLOCK_NAME,
                                       HCP_SIMLOCK_PATH,
                                       HCP_SIMLOCK_INTERFACE);

    dbus_g_proxy_begin_call (proxy,
                             HCP_SIMLOCK_CHECK_METHOD,
                             (DBusGProxyCallNotify)_simlock_cb,
                             window,
                             NULL,
                             G_TYPE_INVALID);
  }
}
#endif /* if HCP_WITH_SIM */
static void 
hcp_window_construct_ui (HCPWindow *window)
{
  HCPWindowPrivate *priv;

  HildonProgram *program;
  
  HildonAppMenu *menu = NULL;
  GtkWidget *mi = NULL;
  GtkWidget *scrolled_window = NULL;

  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  /* Why is this not read from the gtkrc?? -- Jobi */
#if 0 
  /* Control Panel Grid */
  gtk_rc_parse_string ("  style \"hildon-control-panel-grid\" {"
              "    CPGrid::n_columns = 2"
          "    CPGrid::label_pos = 1"
	      "    GtkWidget::hildon-mode = 1"
          "  }"
          " widget \"*.hildon-control-panel-grid\" "
          "    style \"hildon-control-panel-grid\"");
  /* Separators style */
  gtk_rc_parse_string ("  style \"hildon-control-panel-separator\" {"
          "    GtkSeparator::hildonlike-drawing = 1"
                      "  }"
          " widget \"*.hildon-control-panel-separator\" "
                      "    style \"hildon-control-panel-separator\"");
#endif
  
  program = HILDON_PROGRAM (hildon_program_get_instance ());

  hildon_program_add_window (program, HILDON_WINDOW (window));
  
  gtk_window_set_title (GTK_WINDOW (window),
                        HCP_TITLE);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (hcp_window_quit), window);

  g_signal_connect(G_OBJECT (program), "notify::is-topmost",
                   G_CALLBACK (hcp_window_topmost_status_change), window);

  menu = HILDON_APP_MENU (hildon_app_menu_new ());

  hildon_stackable_window_set_main_menu (HILDON_STACKABLE_WINDOW (window), menu);

#ifdef MAEMO_TOOLS

  /* Reset Factory Settings */
#if HCP_WITH_ROS
  mi = hildon_button_new (HILDON_SIZE_AUTO_WIDTH |
						  HILDON_SIZE_FINGER_HEIGHT,
  						  HILDON_BUTTON_ARRANGEMENT_VERTICAL);

  GtkWidget *ros_label = gtk_label_new (HCP_MENU_RFS);
  gtk_label_set_justify (GTK_LABEL(ros_label), GTK_JUSTIFY_CENTER );

  gtk_container_add (GTK_CONTAINER(mi), ros_label);
 
  hildon_helper_set_logical_font (mi, "SmallSystemFont");
 
  hildon_app_menu_append (menu, GTK_BUTTON(mi));

  g_signal_connect (mi, "clicked",
                    G_CALLBACK (hcp_window_reset_factory_settings), window);
#endif
  /* Clear User Data */
#if HCP_WITH_CUD
  mi = hildon_button_new_with_text (HILDON_SIZE_AUTO_WIDTH |
									HILDON_SIZE_FINGER_HEIGHT, 
									HILDON_BUTTON_ARRANGEMENT_VERTICAL,
									HCP_MENU_CUD, NULL);

  hildon_app_menu_append (menu, GTK_BUTTON(mi));

  g_signal_connect (mi, "clicked",
                    G_CALLBACK (hcp_window_clear_user_data), window);
#endif
  /* unlock device */
#if HCP_WITH_SIM
  mi = hildon_button_new (HILDON_SIZE_AUTO_WIDTH |
						  HILDON_SIZE_FINGER_HEIGHT,
  						  HILDON_BUTTON_ARRANGEMENT_VERTICAL);

  GtkWidget *sim_label = gtk_label_new (HCP_MENU_SIM);
  gtk_label_set_justify (GTK_LABEL(sim_label), GTK_JUSTIFY_CENTER );

  gtk_container_add (GTK_CONTAINER(mi), sim_label);

  hildon_helper_set_logical_font (mi, "SmallSystemFont");

  hildon_app_menu_append (menu, GTK_BUTTON(mi));

  g_signal_connect (mi, "clicked",
                    G_CALLBACK (hcp_window_sim_unlock), window);

  gtk_widget_set_no_show_all(mi,TRUE);
  priv->mi_simlock = mi;
  hcp_window_check_simlock (window);
#endif /* if HCP_WITH_SIM */

#if HCP_PORTRAIT_DEBUG
  mi = hildon_button_new_with_text (HILDON_SIZE_AUTO_WIDTH |
									HILDON_SIZE_FINGER_HEIGHT, 
									HILDON_BUTTON_ARRANGEMENT_VERTICAL,
									"Portrait debug", NULL);

  hildon_app_menu_append (menu, GTK_BUTTON (mi));

  g_signal_connect (mi, "clicked",
                    G_CALLBACK (hcp_portrait_debug), window);
#endif

#endif /* ifdef MAEMO_TOOLS */
  
  gtk_widget_show_all (GTK_WIDGET (menu));

  /* Set the keyboard listening callback */
  gtk_widget_add_events (GTK_WIDGET(window),
                         GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (window), "key_release_event",
                    G_CALLBACK (hcp_window_keyboard_listener), NULL);

  scrolled_window = g_object_new (HILDON_TYPE_PANNABLE_AREA, NULL);

  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  window->priv->align = gtk_alignment_new (0,0,0,0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (window->priv->align),
                             HILDON_MARGIN_DOUBLE, 
                             0,
                             HILDON_MARGIN_DOUBLE,
                             HILDON_MARGIN_DOUBLE);

  gtk_container_add (GTK_CONTAINER(window->priv->align),
                     GTK_WIDGET(priv->view));

  hildon_pannable_area_add_with_viewport (
          HILDON_PANNABLE_AREA (scrolled_window),
          window->priv->align);
}

static void
hcp_window_size_request (HCPWindow          *window,
                         GtkRequisition     *requisition,
                         gpointer            user_data)
{
    if (G_UNLIKELY (! window->priv->view))
        return;

    gtk_widget_set_size_request (window->priv->align,
                                 requisition->width - HILDON_MARGIN_DEFAULT,
                                 -1);
}                  

static void
hcp_window_init (HCPWindow *window)
{
  HCPProgram *program = hcp_program_get_instance ();
  HCPWindowPrivate *priv = NULL;

  window->priv = (HCPWindowPrivate*)hcp_window_get_instance_private(window);

  priv = window->priv;

  priv->focused_item = NULL;
  priv->saved_focused_filename = NULL;
  priv->scroll_value = 0;
#if HCP_WITH_SIM
  priv->mi_simlock = NULL;
#endif
  priv->al = g_object_ref (program->al);

  hcp_window_retrieve_configuration (window);

  /* Turn on portrait mode support flag */
  /* TODO FIXME XXX Turn on this flag when applets ready... */
  /* Most applets support portrait mode, flag enabled ~MohammadAG */
  hildon_gtk_window_set_portrait_flags (GTK_WINDOW (window),
                                        HILDON_PORTRAIT_MODE_SUPPORT);

  priv->view = hcp_app_view_new ();
  g_signal_connect (G_OBJECT (priv->view), "focus-changed",
                    G_CALLBACK (hcp_window_app_view_focus_cb), window);

  g_signal_connect (G_OBJECT (priv->al), "updated",
                    G_CALLBACK (hcp_window_app_list_updated_cb), window);

  g_signal_connect (G_OBJECT (window), "size-request",
                    G_CALLBACK (hcp_window_size_request), NULL);

  hcp_window_retrieve_state (window);

  hcp_window_construct_ui (window);

  hcp_app_view_populate (HCP_APP_VIEW (priv->view), priv->al);

  program->handler_id = g_signal_connect_after (G_OBJECT (priv->view), "expose-event",
                          G_CALLBACK (_expose_cb), priv->view);
}

static void
hcp_window_finalize (GObject *object)
{
  HCPWindowPrivate *priv;
  
  g_return_if_fail (object);
  g_return_if_fail (HCP_IS_WINDOW (object));

  priv = HCP_WINDOW (object)->priv;

  if (priv->al != NULL) 
  {
    g_object_unref (priv->al);
    priv->al = NULL;
  }

  if (priv->focused_item != NULL) 
  {
    g_object_unref (priv->focused_item);
    priv->focused_item = NULL;
  }

  if (priv->saved_focused_filename)
  {
    g_free (priv->saved_focused_filename);
    priv->saved_focused_filename = NULL;
  }

  G_OBJECT_CLASS (hcp_window_parent_class)->finalize (object);
}

static void
hcp_window_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
/* Buggy applets can crash controlpanel, so disabled for now */
  static gboolean enforce_state = FALSE;
	
  GTK_WIDGET_CLASS (hcp_window_parent_class)->size_allocate (widget, allocation);

  if (enforce_state)
  {
    HCPProgram *program = hcp_program_get_instance ();
    HCPWindow *window = HCP_WINDOW (widget);

    hcp_window_enforce_state (HCP_WINDOW (widget));

    if (program->execute == 1 && window->priv->focused_item) 
    {
      program->execute = 0;
      hcp_app_launch (window->priv->focused_item, FALSE);
    }
    enforce_state = FALSE;
  }
}

static void
hcp_window_class_init (HCPWindowClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

  g_object_class->finalize = hcp_window_finalize;

  widget_class->size_allocate = hcp_window_size_allocate;
}
 
GtkWidget *
hcp_window_new ()
{
  GtkWidget *window = g_object_new (HCP_TYPE_WINDOW, NULL);

  return window;
}

void
hcp_window_close (HCPWindow *window)
{
  g_return_if_fail (window);
  g_return_if_fail (HCP_IS_WINDOW (window));

  hcp_window_quit (NULL, window);
}

