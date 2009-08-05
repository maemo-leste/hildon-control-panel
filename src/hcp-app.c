/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#define _XOPEN_SOURCE
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>

#include "hcp-program.h"
#include "hcp-app.h"

#define HCP_APP_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_APP, HCPAppPrivate))

G_DEFINE_TYPE (HCPApp, hcp_app, G_TYPE_OBJECT);

enum
{
  PROP_NAME = 1,
  PROP_PLUGIN,
  PROP_ICON,
  PROP_CATEGORY,
  PROP_IS_RUNNING,
  PROP_GRID,
  PROP_ITEM_POS,
  PROP_SUGGESTED_POS,
  PROP_TEXT_DOMAIN
};

struct _HCPAppPrivate 
{
    gchar                   *name;
    gchar                   *plugin;
    gchar                   *icon;
    gchar                   *category;
    gboolean                 is_running;
    GtkWidget               *grid;
    gint                     item_pos;
    gint                     sugg_pos;
    gchar                   *text_domain;
    gchar                   *wm_class;
    Window                   xid;
    GPid                     pid;
};

typedef struct _PluginLaunchData
{
  HCPApp     *app;
  gboolean    user_activated;
  char       *hcp_xid;
} PluginLaunchData;

static void
hcp_app_init (HCPApp *app)
{
  app->priv = HCP_APP_GET_PRIVATE (app);

  app->priv->name = NULL;
  app->priv->plugin = NULL;
  app->priv->wm_class = NULL;
  app->priv->icon = NULL;
  app->priv->category = NULL;
  app->priv->is_running = FALSE;
  app->priv->grid = NULL;
  app->priv->item_pos = -1;
  app->priv->text_domain = NULL;
  app->priv->sugg_pos = G_MAXINT;
}

static void
hcp_app_finalize (GObject *object)
{
  HCPApp *app;
  HCPAppPrivate *priv;
  
  g_return_if_fail (object);
  g_return_if_fail (HCP_IS_APP (object));

  app = HCP_APP (object);
  priv = app->priv;

  if (priv->name != NULL) 
  {
    g_free (priv->name);
    priv->name = NULL;
  }

  if (priv->plugin != NULL) 
  {
    g_free (priv->plugin);
    priv->plugin = NULL;
  }

  if (priv->wm_class != NULL) 
  {
    g_free (priv->wm_class);
    priv->wm_class = NULL;
  }

  if (priv->icon != NULL) 
  {
    g_free (priv->icon);
    priv->icon = NULL;
  }

  if (priv->category != NULL) 
  {
    g_free (priv->category);
    priv->category = NULL;
  }

  if (priv->grid != NULL) 
  {
    g_object_unref (priv->grid);
    priv->grid = NULL;
  }

  if (priv->text_domain != NULL) 
  {
    g_free (priv->text_domain);
    priv->text_domain = NULL;
  }

  G_OBJECT_CLASS (hcp_app_parent_class)->finalize (object);
}

static void
hcp_app_get_property (GObject    *gobject,
                      guint      prop_id,
                      GValue     *value,
                      GParamSpec *pspec)
{
  HCPAppPrivate *priv;

  priv = HCP_APP (gobject)->priv;

  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_PLUGIN:
      g_value_set_string (value, priv->plugin);
      break;

    case PROP_ICON:
      g_value_set_string (value, priv->icon);
      break;

    case PROP_CATEGORY:
      g_value_set_string (value, priv->category);
      break;

    case PROP_IS_RUNNING:
      g_value_set_boolean (value, priv->is_running);
      break;

    case PROP_GRID:
      g_value_set_object (value, priv->grid);
      break;

    case PROP_ITEM_POS:
      g_value_set_int (value, priv->item_pos);
      break;

    case PROP_SUGGESTED_POS:
      g_value_set_int (value, priv->sugg_pos);
      break;

    case PROP_TEXT_DOMAIN:
      g_value_set_string (value, priv->text_domain);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static gchar *
wm_class_from_so_name (const gchar* so_name)
{
  char* ret = g_path_get_basename (so_name);
  int i, len = strlen (ret);
  for (i = 0; i < len; i++)
  {
    if (!isalpha (ret[i]))
      ret[i] = '_';
    else
      ret[i] = toupper (ret[i]);
  }
  return (gchar*) ret;
}

static void
hcp_app_set_property (GObject      *gobject,
                      guint        prop_id,
                      const GValue *value,
                      GParamSpec   *pspec)
{
  HCPAppPrivate *priv;

  priv = HCP_APP (gobject)->priv;
  
  switch (prop_id)
  {
    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_strdup (g_value_get_string (value));
      break;

    case PROP_PLUGIN:
      g_free (priv->plugin);
      priv->plugin = g_strdup (g_value_get_string (value));
      g_free (priv->wm_class);
      priv->wm_class =wm_class_from_so_name (priv->plugin);
      break;

    case PROP_ICON:
      g_free (priv->icon);
      priv->icon = g_strdup (g_value_get_string (value));
      break;

    case PROP_CATEGORY:
      g_free (priv->category);
      priv->category = g_strdup (g_value_get_string (value));
      break;

    case PROP_IS_RUNNING:
      priv->is_running = g_value_get_boolean (value);
      break;

    case PROP_GRID:
      if (priv->grid)
        g_object_unref (priv->grid);
      priv->grid = g_object_ref (g_value_get_object (value));
      break;

    case PROP_ITEM_POS:
      priv->item_pos = g_value_get_int (value);
      break;

    case PROP_SUGGESTED_POS:
      priv->sugg_pos = g_value_get_int (value);
      break;

    case PROP_TEXT_DOMAIN:
      g_free (priv->text_domain);
      priv->text_domain = g_strdup (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hcp_app_class_init (HCPAppClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;
  
  g_object_class->finalize = hcp_app_finalize;

  g_object_class->get_property = hcp_app_get_property;
  g_object_class->set_property = hcp_app_set_property;
 
  g_object_class_install_property (g_object_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "Set app's name",
                                                        NULL,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));
 
  g_object_class_install_property (g_object_class,
                                   PROP_PLUGIN,
                                   g_param_spec_string ("plugin",
                                                        "Plugin",
                                                        "Set app's plugin path",
                                                        NULL,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));
 
  g_object_class_install_property (g_object_class,
                                   PROP_ICON,
                                   g_param_spec_string ("icon",
                                                        "Icon",
                                                        "Set app's icon",
                                                        NULL,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_CATEGORY,
                                   g_param_spec_string ("category",
                                                        "Category",
                                                        "Set app's category",
                                                        NULL,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_IS_RUNNING,
                                   g_param_spec_boolean ("is-running",
                                                        "Running",
                                                        "Whether the application is running or not",
                                                        FALSE,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_GRID,
                                   g_param_spec_object ("grid",
                                                        "Grid",
                                                        "The grid associated with this application",
                                                        GTK_TYPE_WIDGET,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_ITEM_POS,
                                   g_param_spec_int ("item-pos",
                                                     "Item position",
                                                     "Application position inside the grid",
                                                     -1,
                                                     100,
                                                     -1,
                                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_SUGGESTED_POS,
                                   g_param_spec_int ("suggested-pos",
                                                     "Suggested item position",
                                                     "Application position defined in the .desktop file",
                                                     0,
                                                     G_MAXINT,
                                                     G_MAXINT,
                                                      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

  g_object_class_install_property (g_object_class,
                                   PROP_TEXT_DOMAIN,
                                   g_param_spec_string ("text-domain",
                                                        "Text Domain",
                                                        "Set app's text domain",
                                                        NULL,
                                                        (G_PARAM_READABLE | G_PARAM_WRITABLE)));
 
  g_type_class_add_private (g_object_class, sizeof (HCPAppPrivate));
}

GObject *
hcp_app_new ()
{
  GObject *app = g_object_new (HCP_TYPE_APP, NULL);

  return app;
}

static void
search_window_r (gchar *wm_class, 
                 Atom           atom, 
		 Window         w, 
		 GSList       **result)
{
  unsigned long  nItems;
  unsigned long  bytesAfter;
  unsigned char *prop = NULL;
  Atom           type;
  int            format;
  Window         root;
  Window         parent;
  Window        *children;
  unsigned int   count;
  unsigned int   i;

  /* to avoid HCP exiting on X error ... */
  gdk_error_trap_push ();

  /* Get the _NET_WM_PID property ... */
  if (XGetWindowProperty (GDK_DISPLAY (), w, atom,
                          0, 200, False, XA_STRING, &type, &format, &nItems,
			  &bytesAfter, &prop) == Success)
  {
    /* in case of hit, prepend to result list  */
    if (prop)
    {
      if (strcasecmp ((char*) prop, wm_class) == 0)
        *result = g_slist_append (*result, GUINT_TO_POINTER (w));
      XFree (prop);
    }
  }

  if (gdk_error_trap_pop () == BadWindow)
    return; /* Current 'w' Window is closed meanwhile ... */

  /* Recursion to child windows ... */
  if (XQueryTree(GDK_DISPLAY (), w, &root, &parent, &children, &count))
  {
    for (i = 0; i < count; i++)
      search_window_r (wm_class, atom, children[i], result);
  }
}

static Window
get_xid_by_wm_class (gchar *wm_class)
{
  GdkAtom atom_pid;
  GSList *results = NULL;
  Window  ret;
  atom_pid = gdk_atom_intern ("WM_CLASS", FALSE);
  search_window_r (wm_class, gdk_x11_atom_to_xatom (atom_pid),
                   GDK_ROOT_WINDOW (), &results);

  if (!results)
    ret = None;
  else
  {
   /* Return the topmost window xid */
    ret = GPOINTER_TO_UINT (results->data);
    g_slist_free (results);
  }

  return ret; 
}

static gboolean
try_focus (HCPApp *app)
{
  GdkWindow   *applet;
  HCPProgram  *program = hcp_program_get_instance ();
  g_return_val_if_fail (hcp_app_is_running (app), FALSE);

  if (app->priv->xid == None)
    app->priv->xid = get_xid_by_wm_class (app->priv->wm_class);

  if (app->priv->xid == None)
    return FALSE; /* applet closed meanwhile ... */

  applet = gdk_window_foreign_new ((GdkNativeWindow) app->priv->xid);
  if (applet == NULL)
    return FALSE; /* applet closed meanwhile ... */

  gdk_window_focus (applet, GDK_CURRENT_TIME);

  /* move to the end of the list (to save proper ordering...) */
  program->running_applets = g_list_remove (program->running_applets,
                                            (gconstpointer) app);
  program->running_applets = g_list_append (program->running_applets,
                                            (gpointer) app);

  g_object_unref (applet);

  return TRUE;
}

static void
cpa_child_watch (GPid pid,
                 gint status,
		 PluginLaunchData *d)
{
  HCPApp      *app = d->app;
  HCPProgram  *program = hcp_program_get_instance ();

  app->priv->is_running = FALSE;
  app->priv->xid = None;
  program->running_applets = g_list_remove (program->running_applets,
                                            (gconstpointer) app);

  g_debug ("CPA process exited with status = '%d'", status);

  g_object_unref (app);
  g_free (d->hcp_xid);
  g_free (d);
  return;
}

void
hcp_app_launch (HCPApp *app, gboolean user_activated)
{
  g_return_if_fail (app);
  g_return_if_fail (HCP_IS_APP (app));

  char             *argv[6];
  PluginLaunchData *d;
  HCPProgram       *program = hcp_program_get_instance ();

  if (hcp_app_is_running (app))
  {
    try_focus (app);
    return;
  }

  program->running_applets = g_list_append (program->running_applets,
                                            (gpointer) app);

  d = g_new0 (PluginLaunchData, 1);

  d->user_activated = user_activated;
  d->app = g_object_ref (app);
  d->hcp_xid = g_strdup_printf ("%lu", (unsigned long) GDK_WINDOW_XID 
                                       (GTK_WIDGET (program->window)->window));

  app->priv->is_running = TRUE;
  app->priv->xid = None;

  argv[0] = "/usr/bin/cpa_loader";
  argv[1] = app->priv->plugin;
  argv[2] = app->priv->wm_class;
  argv[3] = ( d->user_activated ? "1" : "0" );
  argv[4] = d->hcp_xid;
  argv[5] = NULL;

  g_spawn_async (NULL, (char**) argv, NULL,
                 (GSpawnFlags) G_SPAWN_DO_NOT_REAP_CHILD,
                 NULL, NULL, &app->priv->pid, NULL);

  /* For watching applet exiting ... */
  g_child_watch_add (app->priv->pid, 
                     (GChildWatchFunc) cpa_child_watch, d);
}


void
hcp_app_focus (HCPApp *app)
{
  HCPAppPrivate *priv;

  g_return_if_fail (app);
  g_return_if_fail (HCP_IS_APP (app));

  priv = app->priv;

  if (priv->grid) 
  {
    GtkTreePath *path;

    gtk_widget_grab_focus (priv->grid);
    path = gtk_tree_path_new_from_indices (priv->item_pos, -1);
    gtk_icon_view_select_path (GTK_ICON_VIEW (priv->grid), path);
    gtk_tree_path_free (path);
  }
}

void
hcp_app_save_state (HCPApp *app)
{
  g_return_if_fail (app);
  g_return_if_fail (HCP_IS_APP (app));
  g_return_if_fail (hcp_app_is_running (app));

  if (app->priv->pid > 0)
  {
    /* Kill SIGTERM to plugin (to save state ... ) */
    kill ((pid_t) app->priv->pid, 15);
/*    g_debug ("hcp_app_save_state ('%s')", app->priv->name); */
  }
}

gboolean
hcp_app_is_running (HCPApp *app)
{
  HCPAppPrivate *priv;

  g_return_val_if_fail (app, FALSE);
  g_return_val_if_fail (HCP_IS_APP (app), FALSE);

  priv = app->priv;

  return priv->is_running;
}

gboolean
hcp_app_can_save_state (HCPApp *app)
{
  /* Lets cpa_loader to decide applet can save itself state */
  return TRUE;
}

gint
hcp_app_sort_func (const HCPApp *a, const HCPApp *b)
{
  g_return_val_if_fail (a && b, 0);

  /* sort by position or translated name (if position is equal) */
  gint ret = a->priv->sugg_pos != b->priv->sugg_pos ? \
             (a->priv->sugg_pos < b->priv->sugg_pos ? -1 : 1) : \
             strcmp (_(a->priv->name), _(b->priv->name));

  return ret;
}

gchar *
hcp_app_get_plugin (HCPApp *app)
{
  g_return_val_if_fail (app, NULL);
  g_return_val_if_fail (HCP_IS_APP (app), NULL);

  return app->priv->plugin;
}

