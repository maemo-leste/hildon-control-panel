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

#ifndef HCP_APP_LIST_H
#define HCP_APP_LIST_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _HCPAppList HCPAppList;
typedef struct _HCPAppListClass HCPAppListClass;
typedef struct _HCPAppListPrivate HCPAppListPrivate;

#define HCP_TYPE_APP_LIST            (hcp_app_list_get_type ())
#define HCP_APP_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_APP_LIST, HCPAppList))
#define HCP_APP_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HCP_TYPE_APP_LIST, HCPAppListClass))
#define HCP_IS_APP_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_APP_LIST))
#define HCP_IS_APP_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HCP_TYPE_APP_LIST))
#define HCP_APP_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HCP_TYPE_APP_LIST, HCPAppListClass))

struct _HCPAppList 
{
  GObject gobject;

  HCPAppListPrivate *priv;
};

struct _HCPAppListClass 
{
  GObjectClass parent_class;

  void (*updated) (HCPAppList *al, gpointer user_data);
};

/* .desktop keys */
#define HCP_DESKTOP_GROUP               "Desktop Entry"
#define HCP_DESKTOP_KEY_NAME            "Name"
#define HCP_DESKTOP_KEY_ICON            "Icon"
#define HCP_DESKTOP_KEY_CATEGORY        "Categories"
#define HCP_DESKTOP_KEY_PLUGIN          "X-control-panel-plugin"
#define HCP_DESKTOP_KEY_TEXT_DOMAIN     "X-Text-Domain"

typedef struct _HCPCategory {
  gchar   *id;
  gchar   *name;
  GSList  *apps;
} HCPCategory;

GType        hcp_app_list_get_type    (void);

GObject*     hcp_app_list_new         (void);

gboolean     hcp_app_list_focus_item  (HCPAppList  *al, 
                                       const gchar *entryname);

void         hcp_app_list_update      (HCPAppList  *al);

G_END_DECLS

#endif
