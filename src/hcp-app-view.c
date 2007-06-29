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

#define HCP_APP_VIEW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_APP_VIEW, HCPAppViewPrivate))

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
  HCPIconSize  icon_size;
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

  grid = hcp_grid_new ();
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
hcp_app_view_get_selected_app (GtkWidget *widget)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  HCPApp *app = NULL;
  GtkTreeIter iter;
  gint item_pos;

  g_return_val_if_fail (widget, NULL);
  g_return_val_if_fail (GTK_IS_ICON_VIEW (widget), NULL);

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));

  path = hcp_grid_get_selected_item (HCP_GRID (widget));

  if (path == NULL) return NULL;

  item_pos = gtk_tree_path_get_indices (path) [0];

  if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (model), 
                                     &iter, NULL, item_pos)) {
    gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 
                        HCP_STORE_APP, &app,
                        -1);
  }

  gtk_tree_path_free (path);

  return app;
}

static void
hcp_app_view_launch_app (GtkWidget *widget, 
                         GtkTreePath *path, 
                         gpointer user_data)
{
  HCPApp *app = hcp_app_view_get_selected_app (widget);

  if (app != NULL)
    hcp_app_launch (app, TRUE);
}

static void 
hcp_app_view_grid_selection_changed (GtkWidget *widget, gpointer user_data)
{
  GtkWidget *view = GTK_WIDGET (user_data);
  GtkWidget *scrolled_window = view->parent->parent;
  HCPApp *app = hcp_app_view_get_selected_app (widget);
  GtkAdjustment *adj;
  gint visible_y;

  if (app == NULL) return;

  g_return_if_fail (GTK_IS_SCROLLED_WINDOW (scrolled_window));

  adj = gtk_scrolled_window_get_vadjustment (
                                GTK_SCROLLED_WINDOW (scrolled_window));

  g_return_if_fail ((adj->upper - adj->lower) && view->allocation.height);

  visible_y = view->allocation.y +
     (gint)(view->allocation.height * adj->value / (adj->upper - adj->lower));

  if (widget->allocation.y < visible_y)
  {
    adj->value = widget->allocation.y * (adj->upper - adj->lower)
                                        / view->allocation.height;
    gtk_adjustment_value_changed (adj);
  }
  else if (widget->allocation.y + widget->allocation.height > 
           visible_y + scrolled_window->allocation.height)
  {
    adj->value = (widget->allocation.y + widget->allocation.height
           - scrolled_window->allocation.height) * (adj->upper - adj->lower)
           / view->allocation.height;
    gtk_adjustment_value_changed (adj);
  }

  g_signal_emit (G_OBJECT (widget->parent), 
                 signals[SIGNAL_FOCUS_CHANGED], 
                 0, app);
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
 
    g_signal_connect (grid, "selection-changed",
                      G_CALLBACK (hcp_app_view_grid_selection_changed),
                      view);

    g_signal_connect (grid, "item-activated",
                      G_CALLBACK (hcp_app_view_launch_app),
                      NULL);
  
    /* If we are creating a group with a defined name, we use
     * it in the separator */
    separator = hcp_app_view_create_separator (_(category->name));

    /* Pack the separator and the corresponding grid to the vbox */
    gtk_box_pack_start (GTK_BOX (view), separator, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (view), grid, FALSE, TRUE, 0);

    gtk_container_get_focus_chain (GTK_CONTAINER (view), &focus_chain);
    focus_chain = g_list_append (focus_chain, grid);
    gtk_container_set_focus_chain (GTK_CONTAINER (view), focus_chain);
    g_list_free (focus_chain);

    gtk_icon_view_set_model (GTK_ICON_VIEW (grid), 
                             GTK_TREE_MODEL (store));

    g_slist_foreach (category->apps,
                     (GFunc) hcp_app_view_add_app,
                     grid);

    if (!view->priv->first_grid)
      view->priv->first_grid = grid;
  }
}

static void
hcp_app_view_init (HCPAppView *view)
{
  view->priv = HCP_APP_VIEW_GET_PRIVATE (view);

  view->priv->first_grid = NULL;

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
    case PROP_ICON_SIZE:
      g_value_set_int (value, priv->icon_size);
      break;

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
    case PROP_ICON_SIZE:
      priv->icon_size = g_value_get_int (value);
      break;

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

  g_object_class_install_property (g_object_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     "Icon Size",
                                                     "Set view's icon size",
                                                     HCP_ICON_SIZE_SMALL,
                                                     HCP_ICON_SIZE_LARGE,
                                                     HCP_ICON_SIZE_SMALL,
                                                     (G_PARAM_READABLE | 
                                                      G_PARAM_WRITABLE | 
                                                      G_PARAM_CONSTRUCT)));

  g_type_class_add_private (g_object_class, sizeof (HCPAppViewPrivate));
}

GtkWidget *
hcp_app_view_new (HCPIconSize icon_size)
{
  GtkWidget *view = g_object_new (HCP_TYPE_APP_VIEW, 
                                  "icon-size", icon_size,
                                  NULL);

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

  hcp_app_view_set_icon_size (GTK_WIDGET (view), priv->icon_size);

  /* Put focus on the first item of the first grid */
  if (view->priv->first_grid) 
  {
    gtk_widget_grab_focus (priv->first_grid);
    gtk_icon_view_select_path (GTK_ICON_VIEW (priv->first_grid),
                               gtk_tree_path_new_first ());
  }
}

void 
hcp_app_view_set_icon_size (GtkWidget *view, HCPIconSize size)
{
  GtkWidget *grid = NULL;
  GList *iter = NULL;
  GList *list = NULL;

  g_return_if_fail (view);
  g_return_if_fail (HCP_IS_APP_VIEW (view));

  list = iter = gtk_container_get_children (GTK_CONTAINER (view));

  /* Iterate through all of them and set their mode */
  while (iter != 0)
  {
    if (HCP_IS_GRID (iter->data))
    {
      grid = GTK_WIDGET (iter->data);

      if (size == HCP_ICON_SIZE_SMALL) 
      {
        hcp_grid_set_icon_size (HCP_GRID (grid), HCP_ICON_SIZE_SMALL);
      }

      if (size == HCP_ICON_SIZE_LARGE) 
      {
        hcp_grid_set_icon_size (HCP_GRID (grid), HCP_ICON_SIZE_LARGE);
      }
    }

    iter = iter->next;
  }

  g_list_free (list);
}
