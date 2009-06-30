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

#define HCP_GRID_GET_PRIVATE(object) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((object), HCP_TYPE_GRID, HCPGridPrivate))

G_DEFINE_TYPE (HCPGrid, hcp_grid, GTK_TYPE_TABLE)

#define HCP_GRID_ITEM_WIDTH  372
#define HCP_GRID_X_PADDING   4
#define HCP_GRID_Y_PADDING   2
#define HCP_ICON_SIZE        HILDON_ICON_PIXEL_SIZE_FINGER
typedef enum
{
  SIGNAL_ITEM_ACTIVATED,
  N_SIGNALS
} HCPGridSignals;

static gint signals[N_SIGNALS];

struct _HCPGridPrivate {
  gboolean         can_move_focus;
  gboolean         focused_in;
  gint             icon_size;
  GtkTreeModel     *model;
};

static void
hcp_grid_button_clicked (GtkButton* button,
                         gpointer pos)
{
  g_return_if_fail (button);
  g_return_if_fail (GTK_IS_BUTTON(button));

  GtkWidget* grid = gtk_widget_get_parent (GTK_WIDGET(button));

  g_return_if_fail (grid);
  g_return_if_fail (HCP_IS_GRID(grid));
  g_signal_emit (grid, signals[SIGNAL_ITEM_ACTIVATED], 0, pos);
}

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

  /* refreshing buttons */
  gchar *l;
  gtk_tree_model_get (GTK_TREE_MODEL (model), iter, 
                      HCP_STORE_LABEL, &l,
                      -1);

  GtkWidget *label = gtk_label_new(l);

  GtkWidget *image = gtk_image_new_from_pixbuf (icon_pixbuf);

  GtkWidget *hbox = gtk_hbox_new (FALSE, HILDON_MARGIN_DEFAULT);

  GtkWidget *align = gtk_alignment_new (0,0.5,0,0);
  gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, 0, 0, HILDON_MARGIN_DOUBLE);

  gtk_widget_set_size_request (label, 290, -1);
  gtk_misc_set_alignment (GTK_MISC(label), 0,0.5);

  gtk_box_pack_start (GTK_BOX(hbox), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_label_set_line_wrap (label, FALSE);

  gtk_container_add (GTK_CONTAINER(align), hbox);

  GtkWidget *button = hildon_gtk_button_new (HILDON_SIZE_FINGER_HEIGHT);
  gtk_widget_set_size_request (button, HCP_GRID_ITEM_WIDTH, -1);

  gtk_container_add (GTK_CONTAINER(button), align);

  gint *i;
  i = gtk_tree_path_get_indices (path);
  gtk_table_attach_defaults (GTK_TABLE(user_data), button, *i%2, *i%2+1, *i/2, *i/2+1);

  g_signal_connect (button, "clicked",
                      G_CALLBACK (hcp_grid_button_clicked),
                      GINT_TO_POINTER(*i));

  g_free (icon);

  return FALSE;
}

static void
hcp_grid_class_init (HCPGridClass *klass)
{
  signals[SIGNAL_ITEM_ACTIVATED] =
        g_signal_new ("item-activated",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_FIRST,
                      0,
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1,
                      G_TYPE_POINTER);

  g_type_class_add_private (klass, sizeof (HCPGridPrivate));

}

static void
hcp_grid_init (HCPGrid *grid)
{
  grid->priv = HCP_GRID_GET_PRIVATE (grid);

  grid->priv->can_move_focus = FALSE;
  grid->priv->focused_in = FALSE;

  grid->priv->icon_size = HCP_ICON_SIZE;

  g_object_set (grid, 
		"n-rows", 1, 
		"n-columns", 2, 
		"homogeneous", TRUE,
                NULL);

/*  gtk_table_set_col_spacings (GTK_TABLE(grid), 8); */

}

GtkTreeModel *
hcp_grid_get_model (HCPGrid *grid)
{
  g_return_val_if_fail (HCP_GRID(grid), NULL);

  return grid->priv->model;
}

void
hcp_grid_set_model (HCPGrid *grid, GtkTreeModel *model)
{
  g_return_if_fail (HCP_GRID(grid));
  g_return_if_fail (model == NULL || GTK_IS_TREE_MODEL (model));

  if (grid->priv->model == model)
    return;

/* TODO free up previous model. disconnect handlers? */

  grid->priv->model = model;
}


void
hcp_grid_refresh_icons (HCPGrid* grid)
{
  GtkTreeModel* model;

  model = hcp_grid_get_model (HCP_GRID(grid));

  gtk_tree_model_foreach (model, hcp_grid_update_icon, grid);

  GtkRequisition req;
  gtk_widget_size_request (GTK_WIDGET(grid), &req);
  GtkAllocation alloc = {0,0,req.width, req.height};
  gtk_widget_size_allocate (GTK_WIDGET(grid), &alloc);
  gtk_widget_queue_resize (GTK_WIDGET (grid));
}

GtkWidget *
hcp_grid_new (void)
{
  GtkWidget *grid = g_object_new (HCP_TYPE_GRID, NULL);

  return grid;
}
