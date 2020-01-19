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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libosso.h>

#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <gio/gio.h>

#include "hcp-app-list.h"
#include "hcp-app.h"
#include "hcp-config-keys.h"


typedef enum
{
  SIGNAL_UPDATED,
  N_SIGNALS
} HCPAppListSignals;

static gint signals[N_SIGNALS];

enum
{
  PROP_0,
  PROP_APPS,
  PROP_CATEGORIES,
};

struct _HCPAppListPrivate 
{
  GHashTable             *apps;
  GSList                 *categories;
  GFileMonitor *monitor;
};

G_DEFINE_TYPE_WITH_CODE (HCPAppList, hcp_app_list, G_TYPE_OBJECT, G_ADD_PRIVATE(HCPAppList));

#define HCP_SEPARATOR_DEFAULT _("copa_ia_extras")

/* Delay to wait from callback to actually reading 
 * the entries in msecs */
#define HCP_DIR_READ_DELAY 500

#define HCP_POS_REL_PATH "apporder/applets.desktop"
static int callback_pending = 0;

static gboolean 
hcp_monitor_reread_desktop_entries (HCPAppList *al)
{
  callback_pending = 0;

  /* Re-read the item list from .desktop files */
  hcp_app_list_update (al);

  g_signal_emit (G_OBJECT (al), 
                 signals[SIGNAL_UPDATED], 
                 0, NULL);

  return FALSE; 
}

static void 
hcp_monitor_callback_f (GFileMonitor *monitor,
                        GFile *file,
                        GFile *other_file,
                        GFileMonitorEvent event_type,
                        HCPAppList *al)
{
  if (!callback_pending) 
  {
    callback_pending = 1;
    g_timeout_add (HCP_DIR_READ_DELAY,
                  (GSourceFunc) hcp_monitor_reread_desktop_entries, al);
  }
}

static int 
hcp_init_monitor (HCPAppList *al, const gchar *path)
{
  GFile *file;

  g_return_val_if_fail (al, -1);
  g_return_val_if_fail (HCP_IS_APP_LIST (al), -1);
  g_return_val_if_fail (path, -1);

  file = g_file_new_for_path(path);

  al->priv->monitor =
      g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL, NULL);

  g_object_unref(file);

  if (al->priv->monitor)
  {
    g_signal_connect (al->priv->monitor, "changed",
                      G_CALLBACK (hcp_monitor_callback_f), al);
    return 0;
  }

  return -1;
}

static void
hcp_app_list_get_configured_categories (HCPAppList *al)
{
  HCPAppListPrivate *priv;
  GConfClient *client = NULL;
  GSList *group_names = NULL;
  GSList *group_names_i = NULL;
  GSList *group_ids = NULL;
  GSList *group_ids_i = NULL;
  GError *error = NULL;

  g_return_if_fail (al);
  g_return_if_fail (HCP_IS_APP_LIST (al));

  priv = al->priv;

  client = gconf_client_get_default ();
  
  if (client)
  {
    /* Get the group names */
    group_names = gconf_client_get_list (client, 
    		                         HCP_GCONF_GROUPS_KEY,
    		                         GCONF_VALUE_STRING, 
                                         &error);

    if (error)
    {
      g_warning ("Error reading categories names from GConf: %s",
                 error->message);

      g_error_free (error);
      g_object_unref (client);

      goto cleanup;
    }

    /* Get the group ids */
    group_ids = gconf_client_get_list (client, 
                                       HCP_GCONF_GROUP_IDS_KEY,
                                       GCONF_VALUE_STRING, &error);
    
    if (error)
    {
      g_warning ("Error reading categories ids from GConf: %s",
                 error->message);

      g_error_free (error);
      g_object_unref (client);

      goto cleanup;
    }

    g_object_unref (client);
  }

  group_names_i = group_names;
  group_ids_i = group_ids;

  while (group_ids_i && group_names_i)
  {
    HCPCategory *category = g_new0 (HCPCategory, 1);

    category->id = (gchar *) group_ids_i->data;
    category->name = (gchar *) group_names_i->data;
    category->apps = NULL;

    priv->categories = g_slist_append (priv->categories, category);

    group_ids_i = g_slist_next (group_ids_i);
    group_names_i = g_slist_next (group_names_i);
  }

cleanup:
  if (group_names)
    g_slist_free (group_names);

  if (group_ids)
    g_slist_free (group_ids);
}

static void
hcp_app_list_init (HCPAppList *al)
{
  al->priv = (HCPAppListPrivate*)hcp_app_list_get_instance_private(al);

  HCPCategory *extras_category = g_new0 (HCPCategory, 1);

  al->priv->apps = g_hash_table_new (g_str_hash, g_str_equal);

  hcp_app_list_get_configured_categories (al);
  
  /* Add the default category as the last one */
  extras_category->id   = g_strdup ("");
  extras_category->name = g_strdup (HCP_SEPARATOR_DEFAULT);
  extras_category->apps = NULL;

  al->priv->categories = g_slist_append (al->priv->categories, extras_category);

  al->priv->monitor = NULL;
  
  hcp_init_monitor (al, CONTROLPANEL_ENTRY_DIR);
}

static void
hcp_app_list_free_category (HCPCategory* category)
{
  g_slist_free (category->apps);
  category->apps = NULL;

  if (category->id)
    g_free (category->id);

  if (category->name)
    g_free (category->name);

  g_free (category);
}

static void
hcp_app_list_empty_category (HCPCategory* category)
{
  if (category->apps)
  {
    g_slist_free (category->apps);
    category->apps = NULL;
  }
}

static gboolean
hcp_app_list_free_app (gchar *plugin, HCPApp *app)
{
  g_free (plugin);

  if (app)
    g_object_unref (app);

  return TRUE;
}

static void
hcp_app_list_finalize (GObject *object)
{
  HCPAppListPrivate *priv;
  
  g_return_if_fail (object);
  g_return_if_fail (HCP_IS_APP_LIST (object));

  priv = HCP_APP_LIST (object)->priv;

  if (priv->apps != NULL) 
  {
    g_hash_table_foreach_remove (priv->apps, (GHRFunc) hcp_app_list_free_app, NULL);
    g_hash_table_destroy (priv->apps);
  }

  if (priv->categories != NULL) 
  {
    g_slist_foreach (priv->categories, (GFunc) hcp_app_list_free_category, NULL);
    g_slist_free (priv->categories);
  }

  if (priv->monitor)
  {
    g_object_unref (priv->monitor);
  }
    
  G_OBJECT_CLASS (hcp_app_list_parent_class)->finalize (object);
}

static void
hcp_app_list_get_property (GObject    *gobject,
                           guint      prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  HCPAppListPrivate *priv;

  priv = HCP_APP_LIST (gobject)->priv;

  switch (prop_id)
  {
    case PROP_APPS:
      g_value_set_pointer (value, priv->apps);
      break;

    case PROP_CATEGORIES:
      g_value_set_pointer (value, priv->categories);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hcp_app_list_class_init (HCPAppListClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hcp_app_list_finalize;

  g_object_class->get_property = hcp_app_list_get_property;

  signals[SIGNAL_UPDATED] =
        g_signal_new ("updated",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (HCPAppListClass,updated),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

  g_object_class_install_property (g_object_class,
                                   PROP_APPS,
                                   g_param_spec_pointer ("apps",
                                                         "Apps",
                                                         "App List",
                                                         G_PARAM_READABLE));

  g_object_class_install_property (g_object_class,
                                   PROP_CATEGORIES,
                                   g_param_spec_pointer ("categories",
                                                         "Categories",
                                                         "Categories List",
                                                         G_PARAM_READABLE));
}

/* for fmtx pp-bit handling */
#define SYSINFO_SERVICE_DBUS "com.nokia.SystemInfo"
#define SYSINFO_PATH_DBUS "/com/nokia/SystemInfo"
#define SYSINFO_INTERFACE_DBUS "com.nokia.SystemInfo"
#define SYSINFO_METHOD "GetConfigValue"
#define SYSINFO_KEY_FMTX "/certs/ccc/pp/fmtx-raw"

/*
 * Nasty hack in HCP... 
 * i think this should be somewhere else...
 * but well on the devices which has disabled FMTX
 * there is no sense to show this applet ...
 */
static gboolean
hcp_app_is_fmtx_enabled ()
{
    DBusGConnection  *connection;
    DBusGProxy       *proxy = NULL;

    GError           *error = NULL;
    GArray           *array = NULL;
    gboolean          result = FALSE;
    unsigned char     type;

    connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);

    if (connection == NULL || error)
        return FALSE;

    proxy = dbus_g_proxy_new_for_name (connection,
                                       SYSINFO_SERVICE_DBUS,
                                       SYSINFO_PATH_DBUS,
                                       SYSINFO_INTERFACE_DBUS);
    if (proxy == NULL)
    {
        g_warning ("hcp_app_is_fmtx_disabled: "
                   "Couldn't create the proxy object\n");
        return FALSE;
    }

    result = dbus_g_proxy_call (proxy, SYSINFO_METHOD, &error,
                                G_TYPE_STRING,
                                SYSINFO_KEY_FMTX,
                                G_TYPE_INVALID,
                                dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR),
                                &array, G_TYPE_INVALID);
    if (!result || error)
    {
        g_warning ("hcp_app_is_fmtx_disabled: "
                   "Unable to get stored fmtx settings\n");
        g_clear_error (&error);
        return FALSE;
    }
    else
    {
        type = g_array_index (array, unsigned char, 0);
        g_array_free (array, TRUE);
    }

    g_object_unref (proxy);

    if (type == 1) /* fmtx disabled */
        return FALSE;

    return TRUE;
}

static void 
hcp_app_list_read_desktop_entries (HCPAppList *al, const gchar *dir_path)
{
  HCPAppListPrivate *priv;
  GDir *dir;
  GError *error = NULL;
  const char *filename;
  GKeyFile *keyfile, *pos_keyfile;
  gchar *pos_path = NULL;
  gboolean use_pos = TRUE;

  g_return_if_fail (al);
  g_return_if_fail (HCP_IS_APP_LIST (al));
  g_return_if_fail (dir_path);

  priv = al->priv;

  dir = g_dir_open(dir_path, 0, &error);

  if (!dir)
  {
    g_warning ("Error reading desktop files directory: %s", error->message);
    g_error_free (error);
    return;
  }

  keyfile = g_key_file_new ();

  /* try to open keyfile with app positions */
  pos_keyfile = g_key_file_new ();
  pos_path = g_strdup_printf ( "%s/"HCP_POS_REL_PATH, dir_path );
  if (!g_key_file_load_from_file (pos_keyfile, pos_path, G_KEY_FILE_NONE, NULL))
  {
    g_debug ("no keyfile found or there was a problem while parsing %s", pos_path);
    use_pos = FALSE;
  }

  while ((filename = g_dir_read_name (dir)))
  {
    GObject *app = NULL;
    error = NULL;
    gchar *desktop_path = NULL;
    gchar *name = NULL;
    gchar *plugin = NULL;
    gchar *icon = NULL;
    gchar *category = NULL;
    gchar *text_domain = NULL;
    gint pos = 0;

    /* Only consider .desktop files */
    if (!g_str_has_suffix (filename, ".desktop"))
      continue;

    /* Do not load fmtx applet when its disabled */
    if (g_strrstr (filename, "cpfmtx") &&
        ! hcp_app_is_fmtx_enabled ())
      continue;

    desktop_path = g_build_filename (dir_path, filename, NULL);

    g_key_file_load_from_file (keyfile,
                               desktop_path,
                               G_KEY_FILE_NONE,
                               &error);

    g_free (desktop_path);

    if (error)
    {
      g_warning ("Error reading applet desktop file: %s", error->message);
      g_error_free (error);
      continue;
    }

    name = g_key_file_get_locale_string (keyfile,
                                         HCP_DESKTOP_GROUP,
                                         HCP_DESKTOP_KEY_NAME,
                                         NULL /* current locale */,
                                         &error);

    if (error)
    {
      g_warning ("Error reading applet desktop file: %s", error->message);
      g_error_free (error);
      continue;
    }

    plugin = g_key_file_get_string (keyfile,
                                    HCP_DESKTOP_GROUP,
                                    HCP_DESKTOP_KEY_PLUGIN,
                                    &error);

    if (error)
    {
      g_warning ("Error reading applet desktop file: %s", error->message);
      g_error_free (error);
      continue;
    }

    icon = g_key_file_get_string (keyfile,
                                  HCP_DESKTOP_GROUP,
                                  HCP_DESKTOP_KEY_ICON,
                                  &error);
    if (error)
    {
      g_error_free (error);
      error = NULL;
    }

    category = g_key_file_get_string (keyfile,
                                      HCP_DESKTOP_GROUP,
                                      HCP_DESKTOP_KEY_CATEGORY,
                                      &error);

    if (error)
    {
      g_error_free (error);
      error = NULL;
    }

    text_domain = g_key_file_get_string (keyfile,
                                         HCP_DESKTOP_GROUP,
                                         HCP_DESKTOP_KEY_TEXT_DOMAIN,
                                         &error);

    if (error)
    {
      g_error_free (error);
      error = NULL;
    }

    /* try to read position from global .desktop file */
    if (use_pos && category)
    {
      gchar* group_title = g_ascii_strdown (category, -1);
      pos = g_key_file_get_integer (pos_keyfile,
                                    group_title,
                                    filename,
                                    NULL);
      g_free(group_title);
    }

    app = hcp_app_new ();

    g_object_set (G_OBJECT (app),
                  "name", name,
                  "plugin", plugin,
                  "icon", icon,
                  NULL); 

    if (category != NULL)
      g_object_set (G_OBJECT (app),
                    "category", category,
                    NULL); 

    if (text_domain != NULL)
    {
      g_object_set (G_OBJECT (app),
                    "text-domain", text_domain,
                    NULL);
    }

    if (pos)
    {
      g_object_set (G_OBJECT (app),
                    "suggested-pos", pos,
                    NULL);
    }

    g_hash_table_insert (priv->apps, g_strdup (plugin), app);

    g_free (name);
    g_free (plugin);
    g_free (icon);
    g_free (category);
    g_free (text_domain);
  }

  g_key_file_free (keyfile);
  g_key_file_free (pos_keyfile);
  g_dir_close (dir);
  g_free (pos_path);
}

static gint
hcp_app_list_find_category (gpointer a, gpointer b)
{
  HCPCategory *category = (HCPCategory *) a;
  HCPApp *app = (HCPApp *) b;
  gchar *category_id = NULL;

  g_object_get (G_OBJECT (app),
                "category", &category_id,
                NULL);

  if (category_id && category->id &&
      !g_ascii_strcasecmp (category_id, category->id))
  {
    g_free (category_id);
    return 0;
  }

  g_free (category_id);

  return 1;
}

static void
hcp_app_list_sort_by_category (gpointer key, gpointer value, gpointer user_data)
{
  HCPAppListPrivate *priv;
  HCPAppList *al = (HCPAppList *) user_data;
  HCPApp *app = (HCPApp *) value;
  HCPCategory *category; 
  GSList *category_item = NULL;

  g_return_if_fail (al);
  g_return_if_fail (HCP_IS_APP_LIST (al));
  g_return_if_fail (app);
  g_return_if_fail (HCP_IS_APP (app));

  priv = al->priv;

  /* Find a category for this applet */
  category_item = g_slist_find_custom (priv->categories,
                                       app,
                                       (GCompareFunc) hcp_app_list_find_category);

  if (category_item)
  {
    category = (HCPCategory *) category_item->data;
  }
  else
  {
    /* If category doesn't exist or wasn't matched,
     * add to the default one (Extra) */
    category = g_slist_last (priv->categories)->data;
  }

  category->apps = g_slist_insert_sorted (
                                  category->apps,
                                  app,
                                  (GCompareFunc) hcp_app_sort_func);
}

GObject *
hcp_app_list_new ()
{
  GObject *al = g_object_new (HCP_TYPE_APP_LIST, NULL);

  return al;
}

void
hcp_app_list_update (HCPAppList *al)
{
  HCPAppListPrivate *priv;

  g_return_if_fail (al);
  g_return_if_fail (HCP_IS_APP_LIST (al));

  priv = al->priv;

  /* Clean the previous list */
  g_hash_table_foreach_remove (priv->apps, (GHRFunc) hcp_app_list_free_app, NULL);

  /* Read all the entries */
  hcp_app_list_read_desktop_entries (al, CONTROLPANEL_ENTRY_DIR);

  /* Place them is the relevant category */
  g_slist_foreach (priv->categories, (GFunc) hcp_app_list_empty_category, NULL);

  g_hash_table_foreach (priv->apps,
                        (GHFunc) hcp_app_list_sort_by_category,
                        al);
}
