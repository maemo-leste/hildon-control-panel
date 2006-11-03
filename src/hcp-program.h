/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2006 Nokia Corporation.
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

#ifndef HCP_PROGRAM_H
#define HCP_PROGRAM_H

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif /* dbus_api_subject_to_change */

#include <libosso.h>

#include <glib-object.h>

#include "hcp-app-list.h" 
#include "hcp-window.h" 

G_BEGIN_DECLS

typedef struct _HCPProgram HCPProgram;
typedef struct _HCPProgramClass HCPProgramClass;

#define HCP_TYPE_PROGRAM            (hcp_program_get_type ())
#define HCP_PROGRAM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HCP_TYPE_PROGRAM, HCPProgram))
#define HCP_PROGRAM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  HCP_TYPE_PROGRAM, HCPProgramClass))
#define HCP_IS_PROGRAM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HCP_TYPE_PROGRAM))
#define HCP_IS_PROGRAM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  HCP_TYPE_PROGRAM))
#define HCP_PROGRAM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  HCP_TYPE_PROGRAM, HCPProgramClass))

struct _HCPProgram 
{
  GObject gobject;

  GtkWidget      *window;
  HCPAppList     *al;
  osso_context_t *osso;
  gint            execute;
};

struct _HCPProgramClass 
{
  GObjectClass parent_class;
};

GType        hcp_program_get_type       (void);

HCPProgram*  hcp_program_get_instance   (void);

void         hcp_program_run            (HCPProgram *program);

G_END_DECLS

#endif
