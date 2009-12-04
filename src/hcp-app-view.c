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

#include <glib/gi18n.h>

#include "hcp-app-view.h"
#include "hcp-app-list.h"
#include "hcp-app.h"
#include "hcp-grid.h"
#include "hcp-marshalers.h"
#include <hildon/hildon-gtk.h>
#include <hildon/hildon-helper.h>

#define HCP_APP_VIEW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_APP_VIEW, HCPAppViewPrivate))

#define DEF_SCREEN      gdk_display_get_default_screen (gdk_display_get_default ())

G_DEFINE_TYPE (HCPAppView, hcp_app_view, GTK_TYPE_VBOX);

typedef enum
{
  SIGNAL_FOCUS_CHANGED,
  N_SIGNALS
} HCPAppViewSignals;

static gint signals[N_SIGNALS];

enum
{
  PROP_ICON_SIZE = 1
};

struct _HCPAppViewPrivate 
{
  GtkWidget   *first_grid;
};

static GtkListStore*
hcp_app_view_create_store ()
{
  GtkListStore *store;

  store = gtk_list_store_new (HCP_STORE_NUM_COLUMNS, 
                              GDK_TYPE_PIXBUF, 
                              G_TYPE_STRING,
                              G_TYPE_OBJECT); 

  return store;
}

static GtkWidget*
hcp_app_view_create_grid ()
{
  GtkWidget *grid;

  grid = hcp_grid_new (HILDON_UI_MODE_NORMAL);
  gtk_widget_set_name (grid, "hildon-control-panel-grid");

  return grid;
}

static GtkWidget*
hcp_app_view_create_separator (const gchar *label)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 5);
  GtkWidget *separator_1 = gtk_hseparator_new ();
  GtkWidget *separator_2 = gtk_hseparator_new ();
  GtkWidget *label_1 = gtk_label_new (label);

  gtk_widget_set_name (separator_1, "hildon-control-panel-separator");
  gtk_widget_set_name (separator_2, "hildon-control-panel-separator");
  gtk_box_pack_start (GTK_BOX(hbox), separator_1, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label_1, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(hbox), separator_2, TRUE, TRUE, 0);

  return hbox;
}

static HCPApp *
hcp_app_view_get_selected_app (GtkWidget *widget, GtkTreePath *path)
{
  GtkTreeModel *model;
  HCPApp *app = NULL;
  GtkTreeIter iter;
  gint item_pos;

  g_return_val_if_fail (widget, NULL);
  g_return_val_if_fail (GTK_IS_ICON_VIEW (widget), NULL);
  g_return_val_if_fail (path, NULL);

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));

  if (path == NULL) return NULL;

  item_pos = gtk_tree_path_get_indices (path) [0];

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (model), 
                                     &iter, NULL, item_pos)) {
    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                        HCP_STORE_APP, &app,
                        -1);
  }

  return app;
}

static void
hcp_app_view_launch_app (GtkWidget *widget, 
                         GtkTreePath *path, 
                         gpointer user_data)
{

  HCPApp *app = hcp_app_view_get_selected_app (widget, path);

  /* important for state saving of executed app */
  g_signal_emit (G_OBJECT (widget->parent), 
                 signals[SIGNAL_FOCUS_CHANGED], 
                 0, app);

  if (app != NULL)
    hcp_app_launch (app, TRUE);
}

static void
hcp_app_view_add_app (HCPApp *app, HCPGrid *grid)
{
  GtkTreeModel *store;
  GtkTreePath *path;
  GtkTreeIter iter;
  gchar *name = NULL;
  gchar *text_domain = NULL;

  store = gtk_icon_view_get_model (GTK_ICON_VIEW (grid));

  g_object_get (G_OBJECT (app),
                "name", &name,
                "text-domain", &text_domain,
                NULL); 

  gtk_list_store_append (GTK_LIST_STORE (store), &iter);

  gtk_list_store_set (GTK_LIST_STORE (store), &iter, 
                      HCP_STORE_LABEL, ((text_domain && *text_domain) ? 
			                dgettext(text_domain, name) : _(name)),
                      HCP_STORE_APP, app,
                      -1);

  path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);

  g_object_set (G_OBJECT (app),
                "grid", grid,
                "item-pos", gtk_tree_path_get_indices (path) [0],
                NULL);

  gtk_tree_path_free (path);

  g_free (name);
}

static void
hcp_app_view_add_category (HCPCategory *category, HCPAppView *view)
{
  /* If a group has items */
  if (category->apps)
  {
    GtkWidget *grid, *separator;
    GtkListStore *store;
    GList *focus_chain = NULL;

    grid = hcp_app_view_create_grid ();
    store = hcp_app_view_create_store ();

    g_signal_connect (grid, "item-activated",
                      G_CALLBACK (hcp_app_view_launch_app),
                      NULL);
  
    /* If we are creating a group with a defined name, we use
     * it in the separator */
    separator = hcp_app_view_create_separator (_(category->name));

    gtk_box_pack_start (GTK_BOX (view), separator, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (view), GTK_WIDGET(grid), FALSE, FALSE, 0);

    gtk_container_get_focus_chain (GTK_CONTAINER (view), &focus_chain);
    focus_chain = g_list_append (focus_chain, grid);
    gtk_container_set_focus_chain (GTK_CONTAINER (view), focus_chain);
    g_list_free (focus_chain);

    gtk_icon_view_set_model (GTK_ICON_VIEW (grid), 
                             GTK_TREE_MODEL (store));

    g_slist_foreach (category->apps,
                     (GFunc) hcp_app_view_add_app,
                     grid);

    hcp_grid_refresh_icons (HCP_GRID(grid));

    /* first group */
    if (!view->priv->first_grid)
      view->priv->first_grid = grid;
  }
}

static void
hcp_app_view_set_n_columns  (GtkWidget      *widget,
                             gpointer        data)
{
    gint n_columns;
/* 4px is the HCP_GRID_X_PADDING */
#define PORTRAIT_WIDTH   480      - 3 * HILDON_MARGIN_DOUBLE
#define LANDSCAPE_WIDTH (800 / 2) - 2 * HILDON_MARGIN_DOUBLE

    if (! HCP_IS_GRID (widget))
        return;

    n_columns = GPOINTER_TO_INT (data);

/*
 * g_debug ("WIDTH = %d", (n_columns == 1) ? PORTRAIT_WIDTH : LANDSCAPE_WIDTH);
 */

    /* grid view, set proper no. of colunms */
    gtk_icon_view_set_columns (GTK_ICON_VIEW (widget), n_columns);
    gtk_icon_view_set_item_width (GTK_ICON_VIEW (widget),
                                  (n_columns == 1) ? 
                                   PORTRAIT_WIDTH : 
                                   LANDSCAPE_WIDTH);
    hcp_grid_refresh_icons (HCP_GRID (widget));
}

static void
hcp_app_view_size_changed   (GdkScreen          *screen,
                             HCPAppView         *view)
{
    gint    columns = 2;
    if (gdk_screen_get_width (screen) < 800)
        columns = 1;

    gtk_container_foreach (GTK_CONTAINER (view),
                           hcp_app_view_set_n_columns, 
                           GINT_TO_POINTER (columns));
}

static void
hcp_app_view_init (HCPAppView *view)
{
  view->priv = HCP_APP_VIEW_GET_PRIVATE (view);

  view->priv->first_grid = NULL;

  /* Connect to screen size changes in order to receive
   * the orientation changes */
  g_signal_connect (DEF_SCREEN,
                    "size-changed",
                    G_CALLBACK (hcp_app_view_size_changed),
                    view);

  g_object_set (G_OBJECT (view), 
                "homogeneous", FALSE,
                "spacing", 6, 
                NULL);
}

static void
hcp_app_view_get_property (GObject    *gobject,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  HCPAppViewPrivate *priv;

  priv = HCP_APP_VIEW (gobject)->priv;

  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hcp_app_view_set_property (GObject      *gobject,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  HCPAppViewPrivate *priv;

  priv = HCP_APP_VIEW (gobject)->priv;
  
  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
hcp_app_view_class_init (HCPAppViewClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;

  g_object_class->get_property = hcp_app_view_get_property;
  g_object_class->set_property = hcp_app_view_set_property;

  signals[SIGNAL_FOCUS_CHANGED] =
        g_signal_new ("focus-changed",
                      G_OBJECT_CLASS_TYPE (g_object_class),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (HCPAppViewClass, focus_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      HCP_TYPE_APP);

  g_type_class_add_private (g_object_class, sizeof (HCPAppViewPrivate));
}

GtkWidget *
hcp_app_view_new (void)
{
  GtkWidget *view = g_object_new (HCP_TYPE_APP_VIEW, NULL);

  return view;
}

void
hcp_app_view_populate (HCPAppView *view, HCPAppList *al)
{
  HCPAppViewPrivate *priv;
  GSList *categories = NULL;

  g_return_if_fail (view);
  g_return_if_fail (HCP_IS_APP_VIEW (view));
  g_return_if_fail (al);
  g_return_if_fail (HCP_IS_APP_LIST (al));

  priv = view->priv;

  g_object_get (G_OBJECT (al),
                "categories", &categories,
                NULL);

  gtk_container_foreach (GTK_CONTAINER (view),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  priv->first_grid = NULL;

  gtk_container_set_focus_chain (GTK_CONTAINER (view), NULL);

  g_slist_foreach (categories,
                   (GFunc) hcp_app_view_add_category,
                   view);

  /*
   * Init columns in grids with the proper number
   * (accoording to the current orientation)
   */
  hcp_app_view_size_changed (DEF_SCREEN, view);
}

