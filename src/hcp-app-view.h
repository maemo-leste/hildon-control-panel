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

#ifndef __HCP_APP_VIEW_H__
#define __HCP_APP_VIEW_H__

#include <gtk/gtk.h>

#include "hcp-app-list.h"
#include "hcp-grid.h"
#include "hcp-app.h"

G_BEGIN_DECLS

typedef struct _HCPAppView HCPAppView;
typedef struct _HCPAppViewClass HCPAppViewClass;
typedef struct _HCPAppViewPrivate HCPAppViewPrivate;

#define HCP_TYPE_APP_VIEW            (hcp_app_view_get_type ())
#define HCP_APP_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_APP_VIEW, HCPAppView))
#define HCP_APP_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HCP_TYPE_APP_VIEW, HCPAppViewClass))
#define HCP_IS_APP_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_APP_VIEW))
#define HCP_IS_APP_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HCP_TYPE_APP_VIEW))
#define HCP_APP_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HCP_TYPE_APP_VIEW, HCPAppViewClass))

struct _HCPAppView 
{
  GtkVBox vbox;

  HCPAppViewPrivate *priv;
};

struct _HCPAppViewClass 
{
  GtkVBoxClass parent_class;

  void (*focus_changed) (HCPAppView *view, HCPApp *app, gpointer user_data);
};

GType        hcp_app_view_get_type        (void);

GtkWidget*   hcp_app_view_new             (HCPIconSize icon_size);

void         hcp_app_view_populate        (HCPAppView *view,
                                           HCPAppList *al);

void         hcp_app_view_set_icon_size   (GtkWidget *view,
                                           HCPIconSize size);

G_END_DECLS

#endif
