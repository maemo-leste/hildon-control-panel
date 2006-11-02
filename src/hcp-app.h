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

#ifndef HCP_APP_H
#define HCP_APP_H

#include <libosso.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _HCPApp HCPApp;
typedef struct _HCPAppClass HCPAppClass;
typedef struct _HCPAppPrivate HCPAppPrivate;

#define HCP_TYPE_APP            (hcp_app_get_type ())
#define HCP_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_APP, HCPApp))
#define HCP_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HCP_TYPE_APP, HCPAppClass))
#define HCP_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_APP))
#define HCP_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HCP_TYPE_APP))
#define HCP_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HCP_TYPE_APP, HCPAppClass))

typedef osso_return_t (hcp_plugin_exec_f) (
                       osso_context_t * osso,
                       gpointer data,
                       gboolean user_activated);

typedef osso_return_t (hcp_plugin_save_state_f) (
                       osso_context_t * osso,
                       gpointer data);

struct _HCPApp 
{
  GObject gobject;

  HCPAppPrivate *priv;
};

struct _HCPAppClass 
{
  GObjectClass parent_class;
};

GType        hcp_app_get_type       (void);

GObject*     hcp_app_new            (void);

void         hcp_app_launch         (HCPApp   *app, 
                                     gboolean  user_activated);

void         hcp_app_focus          (HCPApp   *app); 

void         hcp_app_save_state     (HCPApp   *app);

gboolean     hcp_app_is_running     (HCPApp   *app);

gboolean     hcp_app_can_save_state (HCPApp   *app);

gint         hcp_app_sort_func      (const HCPApp *a, 
                                     const HCPApp *b);

G_END_DECLS

#endif
