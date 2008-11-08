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

#ifndef HCP_GRID_H
#define HCP_GRID_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _HCPGridPrivate HCPGridPrivate;

#define HCP_TYPE_GRID			 (hcp_grid_get_type())
#define HCP_GRID(obj) 	 	     (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_GRID, HCPGrid))
#define HCP_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HCP_TYPE_GRID, HCPGridClass))
#define HCP_IS_GRID(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_GRID))
#define HCP_IS_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HCP_TYPE_GRID))
#define HCP_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HCP_TYPE_GRID, HCPGridClass))

#define HCP_DEFAULT_ICON_BASENAME  "qgn_list_gene_unknown_file"

#define HCP_GRID_NUM_COLUMNS       2

typedef struct {
  GtkIconView parent;
  HCPGridPrivate* priv;
} HCPGrid;

typedef struct {
  GtkIconViewClass parent_class;
} HCPGridClass;

GType hcp_grid_get_type (void);

typedef enum {
  HCP_STORE_ICON = 0,
  HCP_STORE_LABEL,
  HCP_STORE_APP,
  HCP_STORE_NUM_COLUMNS
} HCPStoreColumn;

GtkWidget* hcp_grid_new (HildonUIMode);
void hcp_grid_refresh_icons (HCPGrid*);

G_END_DECLS

#endif /* HCP_GRID_H */
