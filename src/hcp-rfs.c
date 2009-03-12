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

#include <crypt.h>
#include <strings.h>

#include <hildon/hildon-code-dialog.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-banner.h>
#include <codelockui.h>
#include <libosso.h>

#include <glib/gi18n.h>

#include "hcp-rfs.h"
#include "hcp-program.h"

#define HCP_RFS_INFOBANNER_OK      _("rfs_bd_ok")
#define HCP_RFS_INFOBANNER_CANCEL  _("rfs_bd_cancel")
#define HCP_RFS_IB_WRONG_LOCKCODE  dgettext("osso-system-lock", "secu_info_incorrectcode")

#define HCP_RFC_WARNING_DIALOG_WIDTH 450

/*
 * Asks the user for confirmation, returns TRUE if confirmed
 */
static gboolean hcp_rfs_display_warning (HCPProgram  *program,
                                         const gchar *warning,
                                         const gchar *title)
{
  GtkWidget *confirm_dialog;
  gint ret;

  confirm_dialog = hildon_note_new_confirmation (NULL, warning);
  gtk_window_set_transient_for (GTK_WINDOW(confirm_dialog), GTK_WINDOW(program->window));

  gtk_widget_show_all  (confirm_dialog);
  ret = gtk_dialog_run (GTK_DIALOG (confirm_dialog));

  gtk_widget_destroy (GTK_WIDGET (confirm_dialog));

  if (ret == GTK_RESPONSE_OK) {
    return TRUE;
  }

  return FALSE;
}

/*
 * Prompts the user for the lock password.
 * Returns TRUE if correct password, FALSE if cancelled
 */
/* NOTE: implementation now uses libcodelockui */
static gboolean 
hcp_rfs_check_lock_code_dialog (HCPProgram *program)
{
  GtkWidget *dialog; 
  gint ret;
  gint password_correct = FALSE;
  CodeLockUI clui;

  if (!codelockui_init (program->osso))
  {
    g_warning ("codelockui init error!");
    return FALSE;
  }

  dialog = codelock_create_dialog (&clui, TIMEOUT_FOOBAR, FALSE);

  gtk_widget_show_all (dialog);

  gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                GTK_WINDOW (program->window));

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_NONE);

  while (!password_correct)
  {
    gtk_widget_set_sensitive (dialog, TRUE);

    ret = gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_set_sensitive (dialog, FALSE);

    if (ret == GTK_RESPONSE_CANCEL ||
      ret == GTK_RESPONSE_DELETE_EVENT) {
      codelock_destroy_dialog (&clui);

      return FALSE;
    }

    password_correct = codelock_is_passwd_correct (
            codelock_get_code(&clui));

    if (!password_correct)
    {
	   
      hildon_banner_show_information (NULL,
                                      NULL,
                                      HCP_RFS_IB_WRONG_LOCKCODE);

      codelock_clear_code (&clui);
    }
  }

  codelock_destroy_dialog (&clui);

  if (password_correct == -1)
  {
    /* An error occured in the lock code verification query */
    return FALSE;
  }

  return TRUE;
}

static void 
hcp_rfs_launch_script (const gchar *script)
{
  GError *error = NULL;

  if (!g_spawn_command_line_async (script, &error))
  {
    g_warning ("Call to RFS or CUD script failed");

    if (error)
    {
      g_warning (error->message);
      g_error_free (error);
    }
  }
}


gboolean 
hcp_rfs (const gchar *warning, const gchar *title,
         const gchar *script)
{
  if (warning)
  {
    if (!hcp_rfs_display_warning (hcp_program_get_instance (), 
                                  warning, title))
    {
      /* User canceled, return */
      return TRUE;
    }
  }

  if (hcp_rfs_check_lock_code_dialog (hcp_program_get_instance ()))
  {
    /* Password is correct, proceed */
    hcp_rfs_launch_script (script);
    return FALSE;
  }
  else
  {
    /* User cancelled or an error occured. Exit */
    return TRUE;
  }
}
