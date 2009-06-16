/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* mpl-panel-clutter.c */
/*
 * Copyright (c) 2009 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <clutter/x11/clutter-x11.h>
#include <string.h>

#include "mpl-panel-clutter.h"

#define MAX_SUPPORTED_XEMBED_VERSION   1

#define XEMBED_MAPPED          (1 << 0)

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY          0
#define XEMBED_WINDOW_ACTIVATE          1
#define XEMBED_WINDOW_DEACTIVATE        2
#define XEMBED_REQUEST_FOCUS            3
#define XEMBED_FOCUS_IN                 4
#define XEMBED_FOCUS_OUT                5
#define XEMBED_FOCUS_NEXT               6
#define XEMBED_FOCUS_PREV               7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON              10
#define XEMBED_MODALITY_OFF             11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

static void xembed_init (MplPanelClutter *panel);
static void xembed_set_win_info (Display *xdpy, Window xwin, int flags);

G_DEFINE_TYPE (MplPanelClutter, mpl_panel_clutter, MPL_TYPE_PANEL_CLIENT)

#define MPL_PANEL_CLUTTER_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MPL_TYPE_PANEL_CLUTTER, MplPanelClutterPrivate))

static void mpl_panel_clutter_constructed (GObject *self);

enum
{
  PROP_0,
};

struct _MplPanelClutterPrivate
{
  ClutterActor *stage;
  Window        xwindow;
  Window        embedder;
  Atom          Atom_XEMBED;

  ClutterActor *tracked_actor;
  guint         height_notify_cb;
};

static void
mpl_panel_clutter_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_clutter_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
mpl_panel_clutter_dispose (GObject *self)
{
  G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->dispose (self);
}

static void
mpl_panel_clutter_finalize (GObject *object)
{
  G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->finalize (object);
}

static void
mpl_panel_clutter_set_size (MplPanelClient *self, guint width, guint height)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();

  XResizeWindow (xdpy, priv->xwindow, width, height);
  clutter_actor_set_size (priv->stage, width, height);
}

static void
mpl_panel_clutter_set_height (MplPanelClient *panel, guint height)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (panel)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();
  gfloat                  width;

  width = clutter_actor_get_width (priv->stage);

  XResizeWindow (xdpy, priv->xwindow, (guint) width, height);
  clutter_actor_set_height (priv->stage, height);
}

static void
mpl_panel_clutter_class_init (MplPanelClutterClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  MplPanelClientClass *client_class = MPL_PANEL_CLIENT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (MplPanelClutterPrivate));

  object_class->get_property     = mpl_panel_clutter_get_property;
  object_class->set_property     = mpl_panel_clutter_set_property;
  object_class->dispose          = mpl_panel_clutter_dispose;
  object_class->finalize         = mpl_panel_clutter_finalize;
  object_class->constructed      = mpl_panel_clutter_constructed;

  client_class->set_size   = mpl_panel_clutter_set_size;
  client_class->set_height = mpl_panel_clutter_set_height;
}

static void
mpl_panel_clutter_init (MplPanelClutter *self)
{
  MplPanelClutterPrivate *priv;

  priv = self->priv = MPL_PANEL_CLUTTER_GET_PRIVATE (self);
}

ClutterX11FilterReturn
mpl_panel_clutter_xevent_filter (XEvent *xev, ClutterEvent *cev, gpointer data)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (data)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();

  switch (xev->type)
    {
    case MapNotify:
      g_debug ("### got Mapped ###");
      return CLUTTER_X11_FILTER_CONTINUE;
    case ClientMessage:
      if (xev->xclient.message_type == priv->Atom_XEMBED)
	{
	  switch (xev->xclient.data.l[1])
	    {
	    case XEMBED_EMBEDDED_NOTIFY:
	      g_debug ("### got XEMBED_EMBEDDED_NOTIFY ###");

              priv->embedder = xev->xclient.data.l[3];

              /* Map */
              clutter_actor_show (priv->stage);

              /* Signal the embedder we are mapped */
	      xembed_set_win_info (xdpy, priv->xwindow, XEMBED_MAPPED);
	      break;
	    case XEMBED_WINDOW_ACTIVATE:
	      g_debug ("### got XEMBED_WINDOW_ACTIVATE ###");
	      break;
	    case XEMBED_WINDOW_DEACTIVATE:
	      g_debug ("### got XEMBED_WINDOW_DEACTIVATE ###");
	      break;
	    case XEMBED_FOCUS_IN:
	      g_debug ("### got XEMBED_FOCUS_IN ###");
	      break;
            default: ;
	    }

          return CLUTTER_X11_FILTER_REMOVE;
	}
    default: ;
    }

  return CLUTTER_X11_FILTER_CONTINUE;
}

static void
mpl_panel_clutter_constructed (GObject *self)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (self)->priv;
  ClutterActor           *stage = NULL;
  Window                  xwin = None;
  XSetWindowAttributes    attr;
  Display                *xdpy;
  gint                    screen;

  if (G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed)
    G_OBJECT_CLASS (mpl_panel_clutter_parent_class)->constructed (self);

  /*
   * TODO
   *
   * create stage in an override redirect window.
   */

  xdpy   = clutter_x11_get_default_display ();
  screen = clutter_x11_get_default_screen ();

  attr.override_redirect = True;

  priv->xwindow = xwin = XCreateWindow (xdpy,
                                        RootWindow (xdpy, screen),
                                        0, 0, 100, 100, 0,
                                        CopyFromParent, InputOutput,
                                        CopyFromParent,
                                        CWOverrideRedirect, &attr);

  xembed_init (MPL_PANEL_CLUTTER (self));

  stage = clutter_stage_get_default ();

  clutter_x11_set_stage_foreign (CLUTTER_STAGE (stage), xwin);

  priv->stage = stage;

  clutter_x11_add_filter (mpl_panel_clutter_xevent_filter, self);

  g_object_set (self, "xid",
                clutter_x11_get_stage_window (CLUTTER_STAGE (stage)), NULL);
}

MplPanelClient *
mpl_panel_clutter_new (const gchar *name,
                       const gchar *tooltip,
                       const gchar *stylesheet,
                       const gchar *button_style,
                       gboolean     with_toolbar_service)
{
  MplPanelClient *panel = g_object_new (MPL_TYPE_PANEL_CLUTTER,
                                        "name",            name,
                                        "tooltip",         tooltip,
                                        "stylesheet",      stylesheet,
                                        "button-style",    button_style,
                                        "toolbar-service", with_toolbar_service,
                                        NULL);

  return panel;
}

ClutterActor *
mpl_panel_clutter_get_stage (MplPanelClutter *panel)
{
  return panel->priv->stage;
}

static void
mpl_panel_clutter_actor_height_notify_cb (GObject    *gobject,
                                          GParamSpec *pspec,
                                          gpointer    data)
{
  ClutterActor   *actor = CLUTTER_ACTOR (gobject);
  MplPanelClient *panel = MPL_PANEL_CLIENT (data);
  guint           height;

  height = (guint) clutter_actor_get_height (actor);
  mpl_panel_client_set_height (panel, height);
}

/*
 * Sets up the panel for dynamically matching its height to that of the
 * supplied actor (e.g., the top-level panel widget).
 *
 * Passing NULL for actor on a subsequent call we terminated the height
 * tracking.
 */
void
mpl_panel_clutter_track_actor_height (MplPanelClutter *panel,
                                      ClutterActor    *actor)
{
  MplPanelClutterPrivate *priv = panel->priv;

  if (priv->tracked_actor && priv->height_notify_cb)
    {
      g_signal_handler_disconnect (priv->tracked_actor,
                                   priv->height_notify_cb);

      priv->height_notify_cb = 0;
      priv->tracked_actor = NULL;
    }

  if (actor)
    {
      priv->tracked_actor = actor;

      priv->height_notify_cb =
        g_signal_connect (actor, "notify::height",
                          G_CALLBACK (mpl_panel_clutter_actor_height_notify_cb),
                          panel);
    }
}


/*
 * The XEmbed stuff based on Matchbox keyboard.
 */
static void
xembed_set_win_info (Display *xdpy, Window xwin, int flags)
{
   guint32 list[2];

   Atom atom_ATOM_XEMBED_INFO;

   atom_ATOM_XEMBED_INFO
     = XInternAtom(xdpy, "_XEMBED_INFO", False);


   list[0] = MAX_SUPPORTED_XEMBED_VERSION;
   list[1] = flags;
   XChangeProperty (xdpy,
		    xwin,
		    atom_ATOM_XEMBED_INFO,
		    atom_ATOM_XEMBED_INFO, 32,
		    PropModeReplace, (unsigned char *) list, 2);
}

static void
xembed_init (MplPanelClutter *panel)
{
  MplPanelClutterPrivate *priv = panel->priv;
  Display                *xdpy = clutter_x11_get_default_display ();

  priv->Atom_XEMBED = XInternAtom(xdpy, "_XEMBED", False);

  xembed_set_win_info (xdpy, priv->xwindow, 0);
}

#if 0
/*
 * TODO -- currently not needed, probalby remove.
 */
static Bool
xembed_send_message (MplPanelClutter *panel,
                     Window          *w,
                     long             message,
                     long             detail,
                     long             data1,
                     long             data2)
{
  MplPanelClutterPrivate *priv = MPL_PANEL_CLUTTER (data)->priv;
  Display                *xdpy = clutter_x11_get_default_display ();
  XEvent                  ev;

  memset(&ev, 0, sizeof(ev));

  ev.xclient.type = ClientMessage;
  ev.xclient.window = w;
  ev.xclient.message_type = Atom_XEMBED;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime; /* FIXME: Is this correct */
  ev.xclient.data.l[1] = message;
  ev.xclient.data.l[2] = detail;
  ev.xclient.data.l[3] = data1;
  ev.xclient.data.l[4] = data2;

  clutter_x11_trap_x_errors ();

  XSendEvent(xdpy, w, False, NoEventMask, &ev);
  XSync(xdpy, False);

  if (clutter_x11_untrap_x_errors ())
    return False;

  return True;
}
#endif
