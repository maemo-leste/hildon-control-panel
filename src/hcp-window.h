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

#ifndef HCP_WINDOW_H
#define HCP_WINDOW_H

#include <hildon/hildon-window.h>

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _HCPWindow HCPWindow;
typedef struct _HCPWindowClass HCPWindowClass;
typedef struct _HCPWindowPrivate HCPWindowPrivate;

#define HCP_TYPE_WINDOW            (hcp_window_get_type ())
#define HCP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_WINDOW, HCPWindow))
#define HCP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HCP_TYPE_WINDOW, HCPWindowClass))
#define HCP_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_WINDOW))
#define HCP_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HCP_TYPE_WINDOW))
#define HCP_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HCP_TYPE_WINDOW, HCPWindowClass))

struct _HCPWindow 
{
  HildonWindow win;

  HCPWindowPrivate *priv;
};

struct _HCPWindowClass 
{
  HildonWindowClass parent_class;
};

GType        hcp_window_get_type    (void);

GtkWidget*   hcp_window_new         (void);

void         hcp_window_close       (HCPWindow *window);

G_END_DECLS

#endif
