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

#include <glib/gi18n.h>

#include "hcp-app-view.h"
#include "hcp-app-list.h"
#include "hcp-app.h"
#include "hcp-grid.h"
#include "hcp-grid-item.h"
#include "hcp-marshalers.h"

#define HCP_APP_VIEW_GET_PRIVATE(object) \
        (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_APP_VIEW, HCPAppViewPrivate))

G_DEFINE_TYPE (HCPAppView, hcp_app_view, GTK_TYPE_VBOX);

typedef enum
{
  HCP_APP_VIEW_SIGNAL_FOCUS_CHANGED,
  HCP_APP_VIEW_SIGNALS
} HCPAppViewSignals;

static gint hcp_app_view_signals[HCP_APP_VIEW_SIGNALS];

struct _HCPAppViewPrivate 
{
  GtkWidget *first_grid;
};

static GtkWidget*
hcp_app_view_create_grid ()
{
  GtkWidget *grid;

  grid = cp_grid_new ();
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
  gtk_box_pack_start (GTK_BOX(hbox), separator_1, TRUE, TRUE, 5);
  gtk_box_pack_start (GTK_BOX(hbox), label_1, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX(hbox), separator_2, TRUE, TRUE, 5);

  return hbox;
}

static void
hcp_app_view_launch_app (GtkWidget *item_widget, HCPApp *app)
{
  hcp_app_launch (app, TRUE);
}

static gboolean
hcp_app_view_focus (GtkWidget *item_widget, GdkEventFocus *event, HCPApp *app)
{
  GtkWidget *scrolled_window = item_widget->parent->parent->parent->parent;
  GtkWidget *vbox = item_widget->parent->parent;
  GtkAdjustment *adj;
  gint visible_y;

  hcp_app_focus (app);

  g_return_val_if_fail (GTK_IS_SCROLLED_WINDOW (scrolled_window), FALSE);
  
  adj = gtk_scrolled_window_get_vadjustment (
                                GTK_SCROLLED_WINDOW (scrolled_window));

  g_return_val_if_fail ((adj->upper - adj->lower) && vbox->allocation.height,
                        FALSE);

  visible_y = vbox->allocation.y +
     (gint)(vbox->allocation.height * adj->value / (adj->upper - adj->lower));

  if (item_widget->allocation.y < visible_y)
  {
    adj->value = item_widget->allocation.y * (adj->upper - adj->lower)
                                            / vbox->allocation.height;
    gtk_adjustment_value_changed (adj);
  }
  else if (item_widget->allocation.y + item_widget->allocation.height > 
           visible_y + scrolled_window->allocation.height)
  {
    adj->value = (item_widget->allocation.y + item_widget->allocation.height
           - scrolled_window->allocation.height) * (adj->upper - adj->lower)
           / vbox->allocation.height;
    gtk_adjustment_value_changed (adj);
  }

  g_signal_emit (G_OBJECT (item_widget->parent->parent), 
                 hcp_app_view_signals[HCP_APP_VIEW_SIGNAL_FOCUS_CHANGED], 
                 0, app);
  
  return FALSE;
}

static void
hcp_app_view_add_app (HCPApp *app, GtkWidget *grid)
{
  GtkWidget *grid_item;
  gchar *name = NULL;
  gchar *icon = NULL;

  g_object_get (G_OBJECT (app),
                "name", &name,
                "icon", &icon,
                NULL); 

  g_printerr ("Adding app to grid: %s\n", name);

  grid_item = cp_grid_item_new_with_label (icon, _(name));

  g_signal_connect (grid_item, "focus-in-event",
          G_CALLBACK (hcp_app_view_focus),
          app);
  
  g_signal_connect (grid_item, "activate",
          G_CALLBACK (hcp_app_view_launch_app),
          app);

  gtk_container_add (GTK_CONTAINER (grid), grid_item);

  g_object_set (G_OBJECT (app),
                "grid-item", grid_item,
                NULL);

  g_free (name);
  g_free (icon);
}

static void
hcp_app_view_add_category (HCPCategory *category, GtkWidget *view)
{
  GList *focus_chain = NULL;

  /* If a group has items */
  if (category->apps)
  {
    GtkWidget *grid, *separator;

    grid = hcp_app_view_create_grid ();

    g_printerr ("Adding category: %s\n", category->name);

    /* If we are creating a group with a defined name, we use
     * it in the separator */
    separator = hcp_app_view_create_separator (_(category->name));

    /* Pack the separator and the corresponding grid to the vbox */
    gtk_box_pack_start (GTK_BOX (view), separator, FALSE, FALSE, 5);
    gtk_box_pack_start (GTK_BOX (view), grid, FALSE, TRUE, 5);

    /* Why do we explicitely need to do this, shouldn't GTK take
     * care of it? -- Jobi  */
    gtk_container_get_focus_chain (GTK_CONTAINER (view), &focus_chain);
    focus_chain = g_list_append (focus_chain, grid);
    gtk_container_set_focus_chain (GTK_CONTAINER (view), focus_chain);
    g_list_free (focus_chain);

    g_slist_foreach (category->apps,
                     (GFunc) hcp_app_view_add_app,
                     grid);

    if (!HCP_APP_VIEW (view)->priv->first_grid)
      HCP_APP_VIEW (view)->priv->first_grid = g_object_ref (grid);
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
hcp_app_view_finalize (GObject *object)
{
  HCPAppView *view;
  HCPAppViewPrivate *priv;
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (HCP_IS_APP_VIEW (object));

  view = HCP_APP_VIEW (object);
  priv = view->priv;

  if (priv->first_grid) 
  {
    g_object_unref (priv->first_grid);
    priv->first_grid = NULL;
  }
}

static void
hcp_app_view_class_init (HCPAppViewClass *class)
{
  GObjectClass *g_object_class = (GObjectClass *) class;

  g_object_class->finalize = hcp_app_view_finalize;

  hcp_app_view_signals[HCP_APP_VIEW_SIGNAL_FOCUS_CHANGED] =
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
hcp_app_view_new ()
{
  GtkWidget *view = g_object_new (HCP_TYPE_APP_VIEW, NULL);

  return view;
}

void
hcp_app_view_populate (GtkWidget *view, HCPAppList *al)
{
  GSList *categories = NULL;

  g_object_get (G_OBJECT (al),
                "categories", &categories,
                NULL);

  gtk_container_foreach (GTK_CONTAINER (view),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);

  HCP_APP_VIEW (view)->priv->first_grid = NULL;

  gtk_container_set_focus_chain (GTK_CONTAINER (view), NULL);

  g_slist_foreach (categories,
                   (GFunc) hcp_app_view_add_category,
                   view);

  /* Put focus on the first item of the first grid */
  if (HCP_APP_VIEW (view)->priv->first_grid)
    gtk_widget_grab_focus (HCP_APP_VIEW (view)->priv->first_grid);
}

/* FIXME: use a widget better suited for this situation. Grid wants all the
 * space it can get as it is designed to fill the whole area. 
 * Now we have to restrict it's size in this rather ugly way
 */
void 
hcp_app_view_set_icon_size (GtkWidget *view, HCPAppViewIconSize size)
{
  GtkWidget *grid = NULL;
  GList *iter = NULL;
  GList *list = NULL;

  list = iter = gtk_container_get_children (
          GTK_CONTAINER (view));

  /* Iterate through all of them and set their mode */
  while (iter != 0)
  {
    if (CP_OBJECT_IS_GRID (iter->data))
    {
      grid = GTK_WIDGET (iter->data);

      if (size == HCP_APP_VIEW_ICON_SIZE_SMALL) 
      {
        cp_grid_set_style (CP_GRID (grid), "smallicons-cp");
      }

      if (size == HCP_APP_VIEW_ICON_SIZE_LARGE) 
      {
        cp_grid_set_style (CP_GRID (grid), "largeicons-cp");
      }
    }

    iter = iter->next;
  }

  g_list_free (list);
}
