/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#include <libosso.h>
#include <osso-log.h>
#include <osso-helplib.h>
#include <hildon-widgets/hildon-window.h>
#include <hildon-widgets/hildon-program.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>

#include "hcp-window.h"
#include "hcp-program.h"
#include "hcp-app-list.h"
#include "hcp-app-view.h"
#include "hcp-app.h"
#include "hcp-rfs.h"
#include "hcp-config-keys.h"

#define HCP_WINDOW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_WINDOW, HCPWindowPrivate))

G_DEFINE_TYPE (HCPWindow, hcp_window, HILDON_TYPE_WINDOW);

struct _HCPWindowPrivate 
{
  HCPApp         *focused_item;
  HCPAppList     *al;
  GtkWidget      *view;
  GtkWidget      *large_icons_menu_item;
  GtkWidget      *small_icons_menu_item;

  /* For state save data */
  gint            icon_size;
  gchar          *saved_focused_filename;
  gint            scroll_value;
};

#define HCP_TITLE             _("copa_ap_cp_name")
#define HCP_MENU_OPEN         _("copa_me_open")
#define HCP_MENU_SUB_VIEW     _("copa_me_view")
#define HCP_MENU_SMALL_ITEMS  _("copa_me_view_small")
#define HCP_MENU_LARGE_ITEMS  _("copa_me_view_large")
#define HCP_MENU_SUB_TOOLS    _("copa_me_tools")
#define HCP_MENU_SETUP_WIZARD _("copa_me_tools_setup_wizard")
#define HCP_MENU_RFS          _("copa_me_tools_rfs")
#define HCP_MENU_CUD          _("copa_me_tools_cud")
#define HCP_MENU_HELP         _("copa_me_tools_help")
#define HCP_MENU_CLOSE        _("copa_me_close")

#define HCP_OSSO_HELP_ID "Utilities_controlpanel_mainview"

#define HCP_STATE_GROUP         "HildonControlPanel"
#define HCP_STATE_FOCUSED       "Focussed"
#define HCP_STATE_SCROLL_VALUE  "ScrollValue"
#define HCP_STATE_EXECUTE       "Execute"

#define HCP_OPERATOR_WIZARD_DBUS_SERVICE "operator_wizard"
#define HCP_OPERATOR_WIZARD_LAUNCH       "launch_operator_wizard"

#define HCP_RFS_WARNING        _("refs_ia_text")
#define HCP_RFS_WARNING_TITLE  _("rfs_ti_restore")
#define HCP_RFS_SCRIPT         "/usr/sbin/osso-app-killer-rfs.sh"
#define HCP_RFS_HELP_TOPIC     "Features_restorefactorysettings_closealldialog"

#define HCP_CUD_WARNING        _("cud_ia_text")
#define HCP_CUD_WARNING_TITLE  _("cud_ti_clear")
#define HCP_CUD_SCRIPT         "/usr/sbin/osso-app-killer-cud.sh"
#define HCP_CUD_HELP_TOPIC     "Features_clearuserdata_dialog"

static void 
hcp_window_enforce_state (HCPWindow *window)
{
    /* Actually enforce the saved state */
    if (window->priv->icon_size == 0)
      hcp_app_view_set_icon_size (window->priv->view,
                                  HCP_APP_VIEW_ICON_SIZE_SMALL);
    else if (window->priv->icon_size == 1)
      hcp_app_view_set_icon_size (window->priv->view,
                                  HCP_APP_VIEW_ICON_SIZE_LARGE);
    else 
        ULOG_ERR("Unknown iconsize");

    /* If the saved focused item filename is defined, try to 
     * focus on the item. */
    if (window->priv->saved_focused_filename)
    {
      GHashTable *apps = NULL;
      HCPApp *app = NULL;

      g_object_get (G_OBJECT (window->priv->al),
                    "apps", &apps,
                    NULL);

      app = g_hash_table_lookup (apps,
                                 window->priv->saved_focused_filename);

      if (app)
        hcp_app_focus (app);

      g_free (window->priv->saved_focused_filename);
      window->priv->saved_focused_filename = NULL;
    }

    /* main() will start the possible plugin in hcp->execute */
}

static void 
hcp_window_retrieve_state (HCPWindow *window)
{
  HCPProgram *program = hcp_program_get_instance ();
  osso_state_t state = { 0, };
  GKeyFile *keyfile = NULL;
  osso_return_t ret;
  GError *error = NULL;
  gchar *focused = NULL;
  gint scroll_value;
  gboolean execute;

  ret = osso_state_read (program->osso, &state);

  if (ret != OSSO_OK)
  {
    ULOG_ERR ("An error occured when reading application state");
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
    ULOG_ERR ("An error occured when reading application state: %s",
              error->message);
    goto cleanup;
  }

  focused = g_key_file_get_string (keyfile,
                                   HCP_STATE_GROUP,
                                   HCP_STATE_FOCUSED,
                                   &error);
  
  if (error)
  {
    ULOG_ERR ("An error occured when reading application state: %s",
              error->message);
    goto cleanup;
  }

  g_printerr ("Retrieve State focused: %s\n", focused);

  window->priv->saved_focused_filename = focused;

  scroll_value = g_key_file_get_integer (keyfile,
                                         HCP_STATE_GROUP,
                                         HCP_STATE_SCROLL_VALUE,
                                         &error);

  if (error)
  {
    ULOG_ERR ("An error occured when reading application state: %s",
              error->message);
    goto cleanup;
  }
  
  g_printerr ("Retrieve State scroll value: %d\n", scroll_value);

  window->priv->scroll_value = scroll_value;
  
  execute = g_key_file_get_boolean (keyfile,
                                    HCP_STATE_GROUP,
                                    HCP_STATE_EXECUTE,
                                    &error);

  if (error)
  {
    ULOG_ERR ("An error occured when reading application state: %s",
              error->message);
    goto cleanup;
  }
  
  g_printerr ("State scroll execute: %d\n", execute);

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
  HCPProgram *program = hcp_program_get_instance ();
  HCPWindowPrivate *priv;
  osso_state_t state = { 0, };
  GKeyFile *keyfile = NULL;
  osso_return_t ret;
  gchar *focused = NULL;
  GError *error = NULL;

  priv = window->priv;

  if (clear_state)
  {
    state.state_data = "";
    state.state_size = 1;
    ret = osso_state_write (program->osso, &state);
    if (ret != OSSO_OK)
      ULOG_ERR ("An error occured when clearing application state");

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
                                         &state.state_size,
                                         &error);

  if (error)
    goto cleanup;

  ret = osso_state_write (program->osso, &state);

  if (ret != OSSO_OK)
  {
    ULOG_ERR ("An error occured when reading application state");
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
    ULOG_ERR ("An error occured when reading application state: %s",
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
  gboolean icon_size;

  client = gconf_client_get_default ();

  g_return_if_fail (client);

  icon_size = gconf_client_get_bool (client,
                                     HCP_GCONF_ICON_SIZE_KEY,
                                     &error);

  if (error)
  {
    ULOG_ERR (error->message);
    g_error_free (error);
  }
  else
  {
    window->priv->icon_size = icon_size?1:0;
  }

  g_object_unref (client);
}

/* Save the configuration (large/small icons)  */
static void 
hcp_window_save_configuration (HCPWindow *window)
{
  GConfClient *client = NULL;
  GError *error = NULL;
  gboolean icon_size;

  client = gconf_client_get_default ();

  g_return_if_fail (client);

  icon_size = window->priv->icon_size?TRUE:FALSE;

  gconf_client_set_bool (client,
                         HCP_GCONF_ICON_SIZE_KEY,
                         icon_size,
                         &error);

  if (error)
  {
    ULOG_ERR (error->message);
    g_error_free (error);
  }

  g_object_unref (client);
}

static gint 
hcp_window_keyboard_listener (GtkWidget * widget,
                              GdkEventKey * keyevent, 
		              gpointer data)
{
  HCPWindow *window = HCP_WINDOW (widget);
  HCPWindowPrivate *priv = window->priv;

  if (keyevent->type == GDK_KEY_RELEASE) 
  {
    if (keyevent->keyval == HILDON_HARDKEY_INCREASE) 
    {
      if (priv->icon_size != 1) 
      {
        priv->icon_size = 1;
        gtk_check_menu_item_set_active (
                GTK_CHECK_MENU_ITEM (priv->large_icons_menu_item),
                TRUE);
        hcp_app_view_set_icon_size (priv->view, HCP_APP_VIEW_ICON_SIZE_LARGE);
        hcp_window_save_configuration (window);

        return TRUE;
      }
    } 
    else if (keyevent->keyval == HILDON_HARDKEY_DECREASE)
    {
      if (priv->icon_size != 0)
      {
        priv->icon_size = 0;
        gtk_check_menu_item_set_active (
                GTK_CHECK_MENU_ITEM (priv->small_icons_menu_item),
                TRUE);
        hcp_app_view_set_icon_size (priv->view, HCP_APP_VIEW_ICON_SIZE_SMALL);
        hcp_window_save_configuration (window);

        return TRUE;
      }
    }
  }
  
  return FALSE;
}

static void 
hcp_window_launch_help (GtkWidget *widget, HCPWindow *window)
{
  HCPProgram *program = hcp_program_get_instance (); 
  osso_return_t help_ret;
  
  help_ret = ossohelp_show (program->osso, 
                            HCP_OSSO_HELP_ID, 0);

  switch (help_ret)
  {
     case OSSO_OK:
       break;

     case OSSO_ERROR:
       ULOG_WARN ("HELP: ERROR (No help for such topic ID)\n");
       break;

     case OSSO_RPC_ERROR:
       ULOG_WARN ("HELP: RPC ERROR (RPC failed for HelpApp/Browser)\n");
       break;

     case OSSO_INVALID:
       ULOG_WARN ("HELP: INVALID (invalid argument)\n");
       break;

     default:
       ULOG_WARN ("HELP: Unknown error!\n");
       break;
   }
}

static gboolean 
hcp_window_clear_user_data (GtkWidget *widget, HCPWindow *window)
{
  hcp_rfs (HCP_CUD_WARNING,
           HCP_CUD_WARNING_TITLE,
           HCP_CUD_SCRIPT,
           HCP_CUD_HELP_TOPIC);

  return TRUE;
}

static gboolean 
hcp_window_reset_factory_settings (GtkWidget *widget, HCPWindow *window)
{
  hcp_rfs (HCP_RFS_WARNING,
           HCP_RFS_WARNING_TITLE,
           HCP_RFS_SCRIPT,
           HCP_RFS_HELP_TOPIC);

  return TRUE;
}

static void 
hcp_window_run_operator_wizard (GtkWidget *widget, HCPWindow *window)
{
  HCPProgram *program = hcp_program_get_instance ();
  osso_rpc_t returnvalues;
  osso_return_t returnstatus;

  returnstatus = osso_rpc_run_with_defaults
      (program->osso, HCP_OPERATOR_WIZARD_DBUS_SERVICE,
       HCP_OPERATOR_WIZARD_LAUNCH,
       &returnvalues, 
       DBUS_TYPE_INVALID);

  switch (returnstatus)
  {
    case OSSO_OK:
      break;

    case OSSO_INVALID:
      ULOG_ERR("Invalid parameter in operator_wizard launch");
      break;

    case OSSO_RPC_ERROR:
    case OSSO_ERROR:
    case OSSO_ERROR_NAME:
    case OSSO_ERROR_NO_STATE:
    case OSSO_ERROR_STATE_SIZE:
      if (returnvalues.type == DBUS_TYPE_STRING) 
      {    
        ULOG_ERR("Operator wizard launch failed: %s\n",returnvalues.value.s);
      }
      else
      {
        ULOG_ERR("Operator wizard launch failed, unspecified");
      }
      break;            

    default:
      ULOG_ERR("Unknown error type %d", returnstatus);
  }
  
  osso_rpc_free_val (&returnvalues);
}

static void hcp_window_iconsize (GtkWidget *widget, HCPWindow *window)
{
  if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)))
    return;
  
  if (widget == window->priv->large_icons_menu_item)
  {
    hcp_app_view_set_icon_size (window->priv->view,
                                HCP_APP_VIEW_ICON_SIZE_LARGE);
    window->priv->icon_size = 1;
  }
  else if (widget == window->priv->small_icons_menu_item)
  {
    hcp_app_view_set_icon_size (window->priv->view,
                                HCP_APP_VIEW_ICON_SIZE_SMALL);
    window->priv->icon_size = 0;
  }
  
  hcp_window_save_configuration (window);
}

static void 
hcp_window_open (GtkWidget *widget, HCPWindow *window)
{
  hcp_app_launch (window->priv->focused_item, TRUE);
}

static void 
hcp_window_topmost_status_change (GObject *gobject, 
		                  GParamSpec *arg1,
			          HCPWindow *window)
{
  HildonProgram *program = HILDON_PROGRAM (gobject);
  HCPWindowPrivate *priv = window->priv;

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
hcp_window_app_list_updated_cb (HCPAppList *al, HCPWindow *window)
{
  GHashTable *apps = NULL;
  HCPApp *app = NULL;
  gchar *focused = NULL;

  g_object_get (G_OBJECT (window->priv->focused_item),
                "plugin", &focused,
                NULL);

  g_object_get (G_OBJECT (window->priv->al),
                "apps", &apps,
                NULL);

  /* Update the view */
  hcp_app_view_populate (window->priv->view, al);
  gtk_widget_show_all (window->priv->view);

  app = g_hash_table_lookup (apps,
                             focused);

  g_free (focused);

  if (app)
    hcp_app_focus (app);

  hcp_window_enforce_state (window);
}

static void hcp_window_quit (GtkWidget *widget, HCPWindow *window)
{
  hcp_window_save_state (window, TRUE);

  gtk_main_quit ();
}

static void 
hcp_window_construct_ui (HCPWindow *window)
{
  HCPWindowPrivate *priv;

  HildonProgram *program;
  
  GtkMenu *menu = NULL;
  GtkWidget *sub_view = NULL;
  GtkWidget *sub_tools = NULL;
  GtkWidget *mi = NULL;
  GtkWidget *scrolled_window = NULL;
  GSList *menugroup = NULL;

  g_return_if_fail (window != NULL);
  g_return_if_fail (HCP_IS_WINDOW (window));

  priv = window->priv;

  /* Why is this not read from the gtkrc?? -- Jobi */
  /* Control Panel Grid */
  gtk_rc_parse_string ("  style \"hildon-control-panel-grid\" {"
              "    CPGrid::n_columns = 2"
          "    CPGrid::label_pos = 1"
          "  }"
          " widget \"*.hildon-control-panel-grid\" "
          "    style \"hildon-control-panel-grid\"");

  /* Separators style */
  gtk_rc_parse_string ("  style \"hildon-control-panel-separator\" {"
          "    GtkSeparator::hildonlike-drawing = 1"
                      "  }"
          " widget \"*.hildon-control-panel-separator\" "
                      "    style \"hildon-control-panel-separator\"");

  program = HILDON_PROGRAM (hildon_program_get_instance ());

  hildon_program_add_window (program, HILDON_WINDOW (window));

  gtk_window_set_title (GTK_WINDOW (window),
                        HCP_TITLE);

  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (hcp_window_quit), window);

  g_signal_connect(G_OBJECT (program), "notify::is-topmost",
                   G_CALLBACK (hcp_window_topmost_status_change), window);

  menu = GTK_MENU (gtk_menu_new ());

  hildon_window_set_menu (HILDON_WINDOW (window), menu);

  mi = gtk_menu_item_new_with_label (HCP_MENU_OPEN);

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (hcp_window_open), window);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  /* View submenu */
  sub_view = gtk_menu_new ();

  mi = gtk_menu_item_new_with_label (HCP_MENU_SUB_VIEW);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (mi), sub_view);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  /* Small icon size */
  mi = gtk_radio_menu_item_new_with_label
      (menugroup, HCP_MENU_SMALL_ITEMS);
  menugroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mi));
  priv->small_icons_menu_item = mi;

  if (priv->icon_size == 0) {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(mi), TRUE);
  }

  gtk_menu_shell_append (GTK_MENU_SHELL(sub_view), mi);

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (hcp_window_iconsize), window);

  /* Large icon size */
  mi = gtk_radio_menu_item_new_with_label
      (menugroup, HCP_MENU_LARGE_ITEMS);
  priv->large_icons_menu_item = mi;

  if (priv->icon_size == 1) {
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (mi), TRUE);
  }
  menugroup = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (mi));

  gtk_menu_shell_append (GTK_MENU_SHELL (sub_view), mi);

  g_signal_connect (G_OBJECT (mi), "activate", 
                    G_CALLBACK (hcp_window_iconsize), window);

  /* Tools submenu */
  sub_tools = gtk_menu_new ();

  mi = gtk_menu_item_new_with_label (HCP_MENU_SUB_TOOLS);

  gtk_menu_item_set_submenu (GTK_MENU_ITEM(mi), sub_tools);

  gtk_menu_shell_append (GTK_MENU_SHELL(menu), mi);

  /* Run operator wizard */
  mi = gtk_menu_item_new_with_label
      (HCP_MENU_SETUP_WIZARD);

  gtk_menu_shell_append (GTK_MENU_SHELL(sub_tools), mi);

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (hcp_window_run_operator_wizard), window);

  /* Reset Factory Settings */
  mi = gtk_menu_item_new_with_label (HCP_MENU_RFS);

  gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (hcp_window_reset_factory_settings), window);

  /* Clean User Data */
  mi = gtk_menu_item_new_with_label (HCP_MENU_CUD);

  gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

  g_signal_connect (G_OBJECT (mi), "activate",
                    G_CALLBACK (hcp_window_clear_user_data), window);

  /* Help! */
  mi = gtk_menu_item_new_with_label (HCP_MENU_HELP);

  gtk_menu_shell_append (GTK_MENU_SHELL (sub_tools), mi);

  g_signal_connect(G_OBJECT (mi), "activate",
                   G_CALLBACK (hcp_window_launch_help), window);

  /* Close */
  mi = gtk_menu_item_new_with_label (HCP_MENU_CLOSE);

  g_signal_connect (GTK_OBJECT(mi), "activate",
                    G_CALLBACK(hcp_window_quit), window);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  gtk_widget_show_all (GTK_WIDGET (menu));

  /* What is this for? -- Jobi */
#if 0 
  hcp->grids = hildon_cp_applist_get_grids();
  grids = hcp->grids;
  
  while (grids != 0)
  {
      grid = grids->data;
      {
          GValue val = { 0, };
          g_value_init(&val, G_TYPE_STRING);
          g_value_set_string(&val, "");
          g_object_set_property(G_OBJECT(grid), "empty_label", &val);
          g_value_unset(&val);
      }

      grids = grids->next;
  }
#endif

  /* Set the keyboard listening callback */
  gtk_widget_add_events (GTK_WIDGET(window),
                         GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (window), "key_release_event",
                    G_CALLBACK (hcp_window_keyboard_listener), NULL);

  scrolled_window = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                  "vscrollbar-policy", GTK_POLICY_ALWAYS,
                                  "hscrollbar-policy", GTK_POLICY_NEVER,
                                  NULL);

  gtk_container_add (GTK_CONTAINER (window), scrolled_window);

  gtk_scrolled_window_add_with_viewport (
          GTK_SCROLLED_WINDOW (scrolled_window),
          priv->view);
}

static void
hcp_window_init (HCPWindow *window)
{
  HCPProgram *program = hcp_program_get_instance ();
  HCPWindowPrivate *priv = NULL;

  window->priv = HCP_WINDOW_GET_PRIVATE (window);

  priv = window->priv;

  priv->icon_size = 1;
  priv->focused_item = NULL;
  priv->saved_focused_filename = NULL;
  priv->scroll_value = 0;

  priv->al = g_object_ref (program->al);

  priv->view = hcp_app_view_new ();
  hcp_app_view_populate (priv->view, priv->al);

  g_signal_connect (G_OBJECT (priv->al), "updated",
                    G_CALLBACK (hcp_window_app_list_updated_cb), NULL);

  hcp_window_retrieve_configuration (window);

  hcp_window_retrieve_state (window);

  if (priv->saved_focused_filename)
  {
    GHashTable *apps = NULL;

    g_object_get (G_OBJECT (priv->al),
                  "apps", &apps,
                  NULL);

    priv->focused_item = g_hash_table_lookup (apps,
                                              priv->saved_focused_filename);
  }

  hcp_window_construct_ui (window);

  hcp_window_enforce_state (window);
  
/* FIXME
  if (hcp->execute == 1 && hcp->focused_item) {
      priv->execute = FALSE;
      hcp_app_launch (priv->focused_item, FALSE);
  }
*/
}

static void
hcp_window_finalize (GObject *object)
{
  HCPWindow *window;
  HCPWindowPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HCP_IS_WINDOW (object));

  window = HCP_WINDOW (object);
  priv = window->priv;

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
hcp_window_class_init (HCPWindowClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;

  g_object_class->finalize = hcp_window_finalize;

  g_type_class_add_private (g_object_class, sizeof (HCPWindowPrivate));
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
  hcp_window_quit (NULL, window);
}
