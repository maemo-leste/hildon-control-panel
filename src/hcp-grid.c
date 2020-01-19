/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005-2008 Nokia Corporation.
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
#include <hildon/hildon-gtk.h>
#include <hildon/hildon.h>

#define HCP_GRID_X_PADDING   4
#define HCP_GRID_Y_PADDING   2
#define HCP_ICON_SIZE        HILDON_ICON_PIXEL_SIZE_FINGER

struct _HCPGridPrivate {
  GtkCellRenderer *text_cell;
  GtkCellRenderer *pixbuf_cell;
  gboolean         can_move_focus;
  gboolean         focused_in;
/*  gint             row_height;*/
  gint             icon_size;
};

G_DEFINE_TYPE_WITH_CODE (HCPGrid, hcp_grid, GTK_TYPE_ICON_VIEW, G_ADD_PRIVATE(HCPGrid))


static gboolean
hcp_grid_update_icon (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      user_data)
{
  HCPApp *app;
  GtkIconTheme *icon_theme;
  GdkPixbuf *icon_pixbuf;
  gchar *icon = NULL;
  GError *error = NULL;

  g_return_val_if_fail (user_data, TRUE);
  g_return_val_if_fail (HCP_IS_GRID (user_data), TRUE);

/*  priv = HCP_GRID (user_data)->priv; */

  gtk_tree_model_get (GTK_TREE_MODEL (model), iter, 
                      HCP_STORE_APP, &app,
                      -1);

  g_object_get (G_OBJECT (app),
                "icon", &icon,
                NULL);

  icon_theme = gtk_icon_theme_get_default ();

  icon_pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                          icon,
                                         /* priv->icon_size, */
					  HCP_ICON_SIZE,
                                          0, 
                                          &error);

  if (icon_pixbuf == NULL) 
  {
    if (error)
    {
      g_warning ("Couldn't load icon \"%s\": %s", icon, error->message);
      g_error_free (error);
    }

    error = NULL;

    icon_pixbuf = gtk_icon_theme_load_icon (icon_theme,
                                            HCP_DEFAULT_ICON_BASENAME,
                                        /*    priv->icon_size, */
     					    HCP_ICON_SIZE,
                                            0, 
                                            &error);

    if (icon_pixbuf == NULL) 
    {
      g_warning ("Couldn't load default icon: %s", error->message);
      g_error_free (error);
    }
  }

  gtk_list_store_set (GTK_LIST_STORE (model), iter, 
                      HCP_STORE_ICON, icon_pixbuf, 
                      -1);

  g_free (icon);

  return FALSE;
}

static void
hcp_grid_class_init (HCPGridClass *klass)
{

}

static void
hcp_grid_init (HCPGrid *grid)
{
  grid->priv = (HCPGridPrivate*)hcp_grid_get_instance_private(grid);
 
  grid->priv->can_move_focus = FALSE;
  grid->priv->focused_in = FALSE;

  grid->priv->icon_size = HCP_ICON_SIZE;

  grid->priv->pixbuf_cell = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_renderer_set_fixed_size (grid->priv->pixbuf_cell,
									HCP_ICON_SIZE+2*HCP_GRID_X_PADDING , -1);

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
  gtk_cell_renderer_set_fixed_size (grid->priv->text_cell, 300 , 60);

  /* NOTE: it seems that text truncation only works with GtkLabel */
  g_object_set (G_OBJECT(grid->priv->text_cell), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

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

  gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (grid), HILDON_MARGIN_DOUBLE);
  gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (grid), 0);
  gtk_icon_view_set_spacing (GTK_ICON_VIEW (grid), 6);

  gtk_icon_view_set_orientation (GTK_ICON_VIEW (grid), 
                                 GTK_ORIENTATION_HORIZONTAL);

  /* Set default column number (for landscape view) */
  gtk_icon_view_set_columns (GTK_ICON_VIEW (grid), 2);
}

void
hcp_grid_refresh_icons (HCPGrid* grid)
{ 
  GtkTreeModel* model;

  model = gtk_icon_view_get_model (GTK_ICON_VIEW (grid));
  gtk_tree_model_foreach (model, hcp_grid_update_icon, grid);
  GtkRequisition req;
  gtk_widget_size_request (GTK_WIDGET(grid), &req);
  GtkAllocation alloc = {0,0,req.width, req.height};
  gtk_widget_size_allocate (GTK_WIDGET(grid), &alloc);
  gtk_widget_queue_resize (GTK_WIDGET (grid));
}

GtkWidget *
hcp_grid_new (HildonUIMode uimode)
{
  GtkWidget *grid = g_object_new (HCP_TYPE_GRID, NULL);

  return grid;
}
