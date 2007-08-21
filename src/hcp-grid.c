/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
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

#include <math.h>

#include <hildon/hildon-defines.h>

#include <gtk/gtk.h>

#include "hcp-grid.h"
#include "hcp-app.h"

#define HCP_GRID_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_GRID, HCPGridPrivate))

G_DEFINE_TYPE (HCPGrid, hcp_grid, GTK_TYPE_ICON_VIEW);

#define HCP_GRID_NUM_COLUMNS 2
#define HCP_GRID_ITEM_WIDTH  328 
#define HCP_GRID_X_PADDING   4
#define HCP_GRID_Y_PADDING   2
#define HCP_GRID_NO_FOCUS   -1

struct _HCPGridPrivate 
{
  GtkCellRenderer *text_cell;
  GtkCellRenderer *pixbuf_cell;
  gboolean         can_move_focus;
  gboolean         focused_in;
  HCPIconSize      icon_size;
  gint             row_height;
};

enum
{
  HCP_GRID_FOCUS_FROM_ABOVE,
  HCP_GRID_FOCUS_FROM_BELOW,
  HCP_GRID_FOCUS_FIRST,
  HCP_GRID_FOCUS_LAST
};

static gboolean
hcp_grid_update_icon (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      user_data)
{
  HCPGridPrivate *priv;
  HCPApp *app;
  GtkIconTheme *icon_theme;
  GdkPixbuf *icon_pixbuf;
  gchar *icon = NULL;
  GError *error = NULL;

  g_return_val_if_fail (user_data, TRUE);
  g_return_val_if_fail (HCP_IS_GRID (user_data), TRUE);

  priv = HCP_GRID (user_data)->priv;

  gtk_tree_model_get (GTK_TREE_MODEL (model), iter, 
                      HCP_STORE_APP, &app,
                      -1);

  g_object_get (G_OBJECT (app),
                "icon", &icon,
                NULL);
 
  icon_theme = gtk_icon_theme_get_default ();

  icon_pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                          icon,
                                          priv->icon_size, 
                                          0, 
                                          &error);

  if (icon_pixbuf == NULL) 
  {
    g_warning ("Couldn't load icon \"%s\": %s", icon, error->message);
    g_error_free (error);

    error = NULL;

    icon_pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                            HCP_DEFAULT_ICON_BASENAME,
                                            priv->icon_size, 
                                            0, 
                                            &error);

    if (icon_pixbuf == NULL) 
    {
      g_warning ("Couldn't load default icon: %s", error->message);
      g_error_free (error);
    }
  }

  if (priv->row_height == -1) 
  {
    if (priv->icon_size == HCP_ICON_SIZE_SMALL) 
    {
      priv->row_height = gdk_pixbuf_get_height (icon_pixbuf);
    }
    else if (priv->icon_size == HCP_ICON_SIZE_LARGE) 
    {
      priv->row_height = gdk_pixbuf_get_height (icon_pixbuf);
    }

    priv->row_height += 2 * HCP_GRID_Y_PADDING;
  }

  gtk_list_store_set (GTK_LIST_STORE (model), iter, 
                      HCP_STORE_ICON, icon_pixbuf, 
                      -1);

  g_free (icon);

  return FALSE;
}

static gboolean
hcp_grid_button_pressed (GtkWidget      *widget,
                         GdkEventButton *event)
{
  GtkTreePath *selected_path, *clicked_path;
 
  if (event->type == GDK_2BUTTON_PRESS ||
      event->type == GDK_3BUTTON_PRESS) {
    return FALSE;
  }
 
  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
  {
    gtk_widget_grab_focus (widget);

    selected_path = hcp_grid_get_selected_item (HCP_GRID (widget));

    clicked_path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget), 
                                                  (gint) event->x, 
                                                  (gint) event->y);

    if (clicked_path != NULL && selected_path != NULL) 
    { 
      if (gtk_tree_path_get_indices (clicked_path) [0] ==
          gtk_tree_path_get_indices (selected_path) [0]) 
      {
        gtk_icon_view_item_activated (GTK_ICON_VIEW (widget), clicked_path);
      }
    }

    if (clicked_path != NULL) 
    {
      gtk_icon_view_select_path (GTK_ICON_VIEW (widget), clicked_path);
    } 

    gtk_tree_path_free (clicked_path);
    gtk_tree_path_free (selected_path);

    return TRUE;
  }
  
  return FALSE;
}

static void 
hcp_grid_selection_changed (GtkWidget *widget, gpointer user_data)
{
  HCPGridPrivate *priv;
  GtkTreeModel *model;
  GtkTreePath *path;
  gint n_items, item_pos, y, last_row;

  g_return_if_fail (widget);
  g_return_if_fail (HCP_IS_GRID (widget));

  priv = HCP_GRID (widget)->priv;

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));
  n_items = gtk_tree_model_iter_n_children (model, NULL);

  path = hcp_grid_get_selected_item (HCP_GRID (widget));

  if (path == NULL) return;

  item_pos = gtk_tree_path_get_indices (path) [0];
  y = item_pos / HCP_GRID_NUM_COLUMNS;
  last_row = ceil ((double) n_items / HCP_GRID_NUM_COLUMNS) - 1;

  gtk_icon_view_set_cursor (GTK_ICON_VIEW (widget), path, NULL, FALSE);
  
  if ((y == 0 || y == last_row) && !priv->focused_in)
  {
    priv->can_move_focus = FALSE;
  } 
  else if (priv->focused_in)
  {
    priv->focused_in = FALSE;
  } 

  gtk_tree_path_free (path);
}

static GtkWidget *
hcp_grid_get_previous (GtkWidget *grid)
{
  GtkWidget *previous;
  GList *focusable, *focus_chain;

  gtk_container_get_focus_chain (GTK_CONTAINER (grid->parent), &focus_chain);

  focusable = g_list_first (focus_chain);

  while (GTK_WIDGET (focusable->data) != grid)
  {
    focusable = g_list_next (focusable);
  }
  
  if (focusable->prev == NULL)
    return NULL;

  previous = GTK_WIDGET (g_list_previous (focusable)->data);

  g_list_free (focus_chain);
  
  return previous;
}

static GtkWidget *
hcp_grid_get_next (GtkWidget *grid)
{
  GtkWidget *next;
  GList *focusable, *focus_chain;
  
  gtk_container_get_focus_chain (GTK_CONTAINER(grid->parent), &focus_chain);

  focusable = g_list_first (focus_chain);

  while (GTK_WIDGET (focusable->data) != grid)
  {
    focusable = g_list_next (focusable);
  }

  if (focusable->next == NULL)
    return NULL;
  
  next = GTK_WIDGET (g_list_next (focusable)->data);

  g_list_free (focus_chain);

  return next;
}

static void 
hcp_grid_select_from_outside (GtkWidget *widget,
                              gint       direction,
                              gint       x)
{
  GtkTreeModel *model;
  GtkTreePath *path;
  gint n_items, last_row;
  gint item_pos = HCP_GRID_NO_FOCUS;

  if (widget == NULL) return;

  g_return_if_fail (HCP_IS_GRID (widget));

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));
  n_items = gtk_tree_model_iter_n_children (model, NULL);

  switch (direction)
  {
    case HCP_GRID_FOCUS_FROM_ABOVE:
      if (x + 1 > n_items)
        item_pos = n_items - 1;
      else
        item_pos = x;
      break;

    case HCP_GRID_FOCUS_FROM_BELOW:
      last_row = n_items % HCP_GRID_NUM_COLUMNS;

      if (last_row == 0) 
        item_pos = n_items - HCP_GRID_NUM_COLUMNS + x;
      else if (x < last_row)
        item_pos = n_items - last_row + x;
      else 
        item_pos = n_items - 1;
      break;

    case HCP_GRID_FOCUS_FIRST:
      item_pos = 0;
      break;

    case HCP_GRID_FOCUS_LAST:
      item_pos = n_items - 1;
      break;
  }

  if (item_pos != HCP_GRID_NO_FOCUS)
  {
    gtk_widget_grab_focus (widget);
    path = gtk_tree_path_new_from_indices (item_pos, -1);
    gtk_icon_view_select_path (GTK_ICON_VIEW (widget), path);
    gtk_tree_path_free (path);
  }
}

static gboolean 
hcp_grid_focus_in (GtkWidget     *widget, 
                   GdkEventFocus *event)
{
  GList *iter = NULL;
  GList *list = NULL;

  /*
   * Unselect all on other grids 
   */
  list = iter = gtk_container_get_children (GTK_CONTAINER (widget->parent));

  while (iter != 0)
  {
    if (HCP_IS_GRID (iter->data) && widget != iter->data)
    {
      gtk_icon_view_unselect_all (GTK_ICON_VIEW (iter->data));
    }

    iter = iter->next;
  }

  g_list_free (list);

  HCP_GRID (widget)->priv->focused_in = TRUE;
  HCP_GRID (widget)->priv->can_move_focus = TRUE;

  return TRUE;
}

static gint 
hcp_grid_keyboard_listener (GtkWidget   *widget,
                            GdkEventKey *keyevent)
{
  HCPGridPrivate *priv;
  GtkTreeModel *model;
  GtkTreePath *path;
  gint n_items, item_pos, last_row, x, y;
  gboolean result = FALSE;

  g_return_val_if_fail (widget, FALSE);
  g_return_val_if_fail (HCP_IS_GRID (widget), FALSE);

  priv = HCP_GRID (widget)->priv;

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));
  n_items = gtk_tree_model_iter_n_children (model, NULL);

  path = hcp_grid_get_selected_item (HCP_GRID (widget));

  if (path == NULL) 
    return FALSE;

  item_pos = gtk_tree_path_get_indices (path) [0];
  x = item_pos % HCP_GRID_NUM_COLUMNS;
  y = item_pos / HCP_GRID_NUM_COLUMNS;
  last_row = ceil ((double) n_items / HCP_GRID_NUM_COLUMNS) - 1;

  if (keyevent->type == GDK_KEY_RELEASE) 
  {
    switch (keyevent->keyval)
    {
      case HILDON_HARDKEY_UP:
        if (y == 0)
        {
          if (priv->can_move_focus)
          {
            hcp_grid_select_from_outside (hcp_grid_get_previous (widget),
                                          HCP_GRID_FOCUS_FROM_BELOW, x);
            result = TRUE;
          }
          else
            priv->can_move_focus = TRUE;
        }
        break;

      case HILDON_HARDKEY_DOWN:
        if (y == last_row) 
        {
          if (priv->can_move_focus)
          {
            hcp_grid_select_from_outside (hcp_grid_get_next (widget),
                                          HCP_GRID_FOCUS_FROM_ABOVE, x);
            result = TRUE;
          }
          else
            priv->can_move_focus = TRUE;
        } 
        break;

      case HILDON_HARDKEY_LEFT:
      case HILDON_HARDKEY_RIGHT:
        if (y == 0 || y == last_row)
        {
          if (!priv->can_move_focus)
            priv->can_move_focus = TRUE;
        } 
        break;
    }
  }
 
  gtk_tree_path_free (path);

  return result;
}

static void
hcp_grid_size_request (GtkWidget *widget, GtkRequisition *req)
{
  HCPGridPrivate *priv;
  GtkTreeModel *store;
  gint num_items, num_rows;
  
  g_return_if_fail (widget);
  g_return_if_fail (HCP_IS_GRID (widget));

  priv = HCP_GRID (widget)->priv;

  store = gtk_icon_view_get_model (GTK_ICON_VIEW (widget));

  num_items = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL);

  num_rows = ceil ((double) num_items / HCP_GRID_NUM_COLUMNS);
  
  if (num_items > 0) 
  {
    req->height = num_rows * priv->row_height;
  }
}

static void
hcp_grid_class_init (HCPGridClass *class)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->size_request = hcp_grid_size_request;
  widget_class->key_release_event = hcp_grid_keyboard_listener;
  widget_class->focus_in_event = hcp_grid_focus_in;
  widget_class->button_press_event = hcp_grid_button_pressed;

  g_type_class_add_private (g_object_class, sizeof (HCPGridPrivate));
}

static void
hcp_grid_init (HCPGrid *grid)
{
  grid->priv = HCP_GRID_GET_PRIVATE (grid);

  grid->priv->can_move_focus = FALSE;
  grid->priv->focused_in = FALSE;
  grid->priv->icon_size = 0;

  grid->priv->pixbuf_cell = gtk_cell_renderer_pixbuf_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (grid), 
		  	      grid->priv->pixbuf_cell, 
			      FALSE);

  g_object_set (grid->priv->pixbuf_cell, 
		"xpad", HCP_GRID_X_PADDING, 
		"ypad", HCP_GRID_Y_PADDING, 
                NULL);

  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (grid),
                                  grid->priv->pixbuf_cell, 
			          "pixbuf", 0,
                                  NULL);

  grid->priv->text_cell = gtk_cell_renderer_text_new ();

  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (grid), 
		  	      grid->priv->text_cell, 
			      FALSE);

  g_object_set (grid->priv->text_cell, 
                "yalign", 0.5,
                NULL);

  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (grid),
                                  grid->priv->text_cell, 
				  "text", 1, 
				  NULL);  

  gtk_icon_view_set_margin (GTK_ICON_VIEW (grid), 0);

  gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (grid), 8);
  gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (grid), 0);
  gtk_icon_view_set_spacing (GTK_ICON_VIEW (grid), 6);

  gtk_icon_view_set_orientation (GTK_ICON_VIEW (grid), 
                                 GTK_ORIENTATION_HORIZONTAL);

  gtk_icon_view_set_columns (GTK_ICON_VIEW (grid),
                             HCP_GRID_NUM_COLUMNS);

  /* FIXME: This should not be hardcoded. It should be defined 
     based on HCPAppView width. */
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (grid), 
		  		HCP_GRID_ITEM_WIDTH);

  g_signal_connect (G_OBJECT (grid), "selection-changed",
                    G_CALLBACK (hcp_grid_selection_changed),
                    NULL);
}

GtkWidget *
hcp_grid_new (void)
{
  GtkWidget *grid = g_object_new (HCP_TYPE_GRID, NULL);

  return grid;
}

GtkTreePath *
hcp_grid_get_selected_item (HCPGrid *grid)
{
  GtkTreePath *path;
  GList *list;

  g_return_val_if_fail (grid, NULL);
  g_return_val_if_fail (HCP_IS_GRID (grid), NULL);

  list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (grid));

  if (list == NULL) 
    return NULL;

  path = gtk_tree_path_copy ((GtkTreePath *) list->data);
  g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
  g_list_free (list);

  return path;
}

void 
hcp_grid_set_icon_size (HCPGrid *grid, HCPIconSize icon_size)
{
  HCPGridPrivate *priv;
  GtkTreeModel *model;

  g_return_if_fail (grid);
  g_return_if_fail (HCP_IS_GRID (grid));

  priv = grid->priv;

  priv->icon_size = icon_size;

  priv->row_height = -1;

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (grid));

  gtk_tree_model_foreach (model, hcp_grid_update_icon, grid);

  gtk_widget_queue_resize (GTK_WIDGET (grid));
}
