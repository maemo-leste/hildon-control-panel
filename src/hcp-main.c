/*
 * This file is part of hildon-control-panel
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libosso.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>

#include "hcp-program.h"

int main (int argc, char **argv)
{
  HCPProgram *program = NULL; 

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, LOCALEDIR);

  bind_textdomain_codeset (PACKAGE, "UTF-8");

  textdomain (PACKAGE);

  /* Set application name to "" as we only need 
   * the window title in the title bar */
  g_set_application_name ("");
  
  gtk_init (&argc, &argv);

  gnome_vfs_init ();

  program = hcp_program_get_instance ();

  hcp_program_run (program);

  gtk_main();

  g_object_unref (program);

  return 0;
}
