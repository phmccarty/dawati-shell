
/*
 * Copyright (c) 2010 Intel Corp.
 *
 * Author: Robert Staudinger <robert.staudinger@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <locale.h>
#include <stdlib.h>
#include <clutter/clutter.h>
#include <clutter/x11/clutter-x11.h>
#include <glib/gi18n.h>
#include <mx/mx.h>
#include <moblin-panel/mpl-panel-clutter.h>
#include <moblin-panel/mpl-panel-common.h>
#include "mpd-shell.h"
#include "mpd-shell-defines.h"
#include "config.h"

static void
_shell_request_hide_cb (MpdShell        *shell,
                        MplPanelClient  *panel)
{
  mpl_panel_client_hide (panel);
}

static void
_stage_width_notify_cb (ClutterActor  *stage,
                        GParamSpec    *pspec,
                        MpdShell      *shell)
{
  guint width = clutter_actor_get_width (stage);

  clutter_actor_set_width (CLUTTER_ACTOR (shell), width);
}

static void
_stage_height_notify_cb (ClutterActor *stage,
                         GParamSpec   *pspec,
                         MpdShell     *shell)
{
  guint height = clutter_actor_get_height (stage);

  clutter_actor_set_height (CLUTTER_ACTOR (shell), height);
}

static void
_panel_set_size_cb (MplPanelClient  *panel,
                    guint            width,
                    guint            height,
                    MpdShell        *shell)
{
  clutter_actor_set_size (CLUTTER_ACTOR (shell), width, height);
}

int
main (int     argc,
      char  **argv)
{
  static gboolean _standalone = FALSE;
  static GOptionEntry _options[] = {
    { "standalone", 's', 0, G_OPTION_ARG_NONE, &_standalone,
      "Do not embed into the mutter-moblin panel", NULL },
    { NULL }
  };

  ClutterActor    *stage;
  ClutterActor    *shell;
  GOptionContext  *context;
  GError          *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  context = g_option_context_new ("- Moblin devices panel");
  g_option_context_add_main_entries (context, _options, GETTEXT_PACKAGE);
  g_option_context_add_group (context, clutter_get_option_group_without_init ());
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical ("%s %s", G_STRLOC, error->message);
      g_critical ("Starting in standalone mode.");
      g_clear_error (&error);
      _standalone = TRUE;
    }
  g_option_context_free (context);

  clutter_init (&argc, &argv);

  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/moblin/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/icons/hicolor/48x48/mx.cache");
  mx_texture_cache_load_cache (mx_texture_cache_get_default (),
    DATADIR "/mutter-moblin/mx.cache");

  mx_texture_cache_load_cache (mx_texture_cache_get_default (), MX_CACHE);
  mx_style_load_from_file (mx_style_get_default (),
                           PKGTHEMEDIR "/panel.css", NULL);

  if (_standalone)
    {
      Window xwin;

      stage = clutter_stage_get_default ();
      clutter_actor_realize (stage);
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      shell = mpd_shell_new ();
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), shell);

      g_signal_connect (stage, "notify::width",
                        G_CALLBACK (_stage_width_notify_cb), shell);
      g_signal_connect (stage, "notify::height",
                        G_CALLBACK (_stage_height_notify_cb), shell);

      clutter_actor_set_size (stage, MPD_SHELL_WIDTH, MPD_SHELL_HEIGHT);
      clutter_actor_show (stage);

    } else {

      /* All button styling goes in mutter-moblin.css for now,
       * don't pass our own stylesheet. */
      MplPanelClient *panel = mpl_panel_clutter_new ("devices",
                                                     _("devices"),
                                                     /*THEMEDIR "/toolbar-button.css" */ NULL,
                                                     "devices-button",
                                                     TRUE);

      mpl_panel_client_set_height_request (panel, MPD_SHELL_HEIGHT);

      stage = mpl_panel_clutter_get_stage (MPL_PANEL_CLUTTER (panel));
      shell = mpd_shell_new ();
      g_signal_connect (shell, "request-hide",
                        G_CALLBACK (_shell_request_hide_cb), panel);
      clutter_container_add_actor (CLUTTER_CONTAINER (stage), shell);

      g_signal_connect (panel, "set-size",
                        G_CALLBACK (_panel_set_size_cb), shell);
    }

  clutter_main ();
  return EXIT_SUCCESS;
}

