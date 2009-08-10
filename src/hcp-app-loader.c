#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _XOPEN_SOURCE 2009
#include <unistd.h>
#include <libosso.h>
#include <dlfcn.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <hildon/hildon.h>

typedef osso_return_t (hcp_plugin_exec_f) (
                       osso_context_t * osso,
                       gpointer data,
                       gboolean user_activated);

typedef osso_return_t (hcp_plugin_save_state_f) (
                       osso_context_t * osso,
                       gpointer data);

typedef struct {
  gchar                   *soname;
  gchar                   *name;
  gboolean                 user_activated;
  void                    *handle;
  hcp_plugin_exec_f       *exec;
  hcp_plugin_save_state_f *save_state;
  osso_context_t          *osso;
  GtkWidget               *parent;
} hcp_app_data;

/* Its ugly, but i cannot pass this parameter to sighandler ...  */
hcp_app_data *plugin_data = NULL;

static void
save_state (int signal)
{
/*  g_debug ("%s : save_state ()", plugin_data->soname); */
  if (plugin_data->save_state)
    plugin_data->save_state (plugin_data->osso, NULL);
  /* State saved, immediatly exiting ... */
  exit (0);
}

static gboolean
open_plugin (hcp_app_data *plugin)
{
  gchar* full_path = NULL;
  if (*plugin->soname == G_DIR_SEPARATOR)
    full_path = g_strdup (plugin->soname); /* we has the full path :-) */
  else
    full_path = g_build_filename (HCP_PLUGIN_DIR, plugin->soname, NULL);

  if (!(plugin->handle = dlopen (full_path, RTLD_LAZY | RTLD_LOCAL)))
  {
    g_warning ("Could not load the control-panel applet: %s: %s",
               plugin->soname, dlerror ());
    return FALSE;
  }

  if (!(plugin->exec = dlsym (plugin->handle, "execute" )))
  {
    g_warning ("Could not find \"execute\" symbol in "
               "control-panel applet %s: %s", plugin->soname, dlerror ());
    return FALSE;
  }

  plugin->save_state = dlsym (plugin->handle, "save_state");

  return TRUE;
}

static void
close_plugin (hcp_app_data* plugin)
{
  if (!plugin->handle)
    return;

  if (dlclose (plugin->handle))
    g_warning ("An error occured on unloading control-panel applet %s: %s",
               plugin->soname, dlerror ());

  return;
}

static gboolean
execute_plugin (hcp_app_data *plugin)
{
  if (open_plugin (plugin))
    plugin->exec (plugin->osso, plugin->parent, plugin->user_activated);
  
  close_plugin (plugin);

  gtk_main_quit ();

  return FALSE;
}

int
main (int argc, char **argv)
{
  hcp_app_data *plugin;
  Window        hcp;

  if (argc != 5)
  {
  /*
   * argv[0] "cpa_laucher"
   * argv[1] "/somewhere/plugin.so"
   * argv[2] "Plugin name"
   * argv[3] "0" / "1" -> user_activated
   * argv[4] "%lu" -> CPA_parent xid
   */
    g_debug ("Parameters are: %s plugin.so plugin-name"
             " 0/1 [user_activated] xwindow-id", argv[0]);
    return 1;
  }
  
  if (!g_thread_supported ()) g_thread_init (NULL);
  
  hildon_gtk_init (&argc, &argv);

  gnome_vfs_init ();

  plugin = g_new0 (hcp_app_data, 1); 

  plugin->soname = g_strdup (argv[1]);
  plugin->name = g_strdup (argv[2]);
  /* Set WM_CLASS ... */
/*  g_debug ("Set WM_CLASS to '%s'", argv[2]); */
  g_set_prgname (plugin->name);
  gdk_set_program_class (plugin->name);

  g_set_application_name ("");

  plugin->user_activated = (argv[3][0] == '1');

  sscanf (argv[4], "%lu", (unsigned long*) &hcp);

  /* Hack begin ... */
  plugin->parent = gtk_widget_new (GTK_TYPE_WINDOW, "type", GTK_WINDOW_TOPLEVEL, NULL);
  gtk_widget_realize (plugin->parent);
  /* Some applets are trying to query the parent window size ... */
  gtk_widget_set_size_request (plugin->parent, 800, 480);
  plugin->parent->window = gdk_window_foreign_new ((GdkNativeWindow) hcp);
  /* ... hack end */

  plugin->osso = osso_initialize (plugin->name, "1.0",  FALSE, NULL);

  /* To save stating, we should handle SIGTERM */
  plugin_data = plugin;
  signal (15, save_state);

  g_idle_add ((GSourceFunc) execute_plugin, plugin);

  gtk_main ();

  /* Clean up ... */
  osso_deinitialize (plugin->osso);
  g_free (plugin->soname);
  g_free (plugin->name);
  g_free (plugin);

  return 0;
}
