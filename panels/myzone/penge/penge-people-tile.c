/*
 * Copyright (C) 2008 - 2009 Intel Corporation.
 *
 * Author: Rob Bradford <rob@linux.intel.com>
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

#include <moblin-panel/mpl-utils.h>
#include <mojito-client/mojito-client.h>

#include "penge-people-tile.h"
#include "penge-utils.h"
#include "penge-magic-texture.h"
#include "penge-people-pane.h"

G_DEFINE_TYPE (PengePeopleTile, penge_people_tile, PENGE_TYPE_INTERESTING_TILE)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), PENGE_TYPE_PEOPLE_TILE, PengePeopleTilePrivate))

typedef struct _PengePeopleTilePrivate PengePeopleTilePrivate;

struct _PengePeopleTilePrivate {
  MojitoItem *item;
  gboolean double_size;
};

enum
{
  PROP_0,
  PROP_ITEM
};

#define DEFAULT_AVATAR_PATH THEMEDIR "/default-avatar-icon.png"
#define DEFAULT_ALBUM_ARTWORK THEMEDIR "/default-album-artwork.png"

static void
penge_people_tile_set_item (PengePeopleTile *tile,
                            MojitoItem      *item);

static void
penge_people_tile_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);

  switch (property_id) {
    case PROP_ITEM:
      g_value_set_boxed (value, priv->item);
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
    case PROP_ITEM:
      penge_people_tile_set_item ((PengePeopleTile *)object,
                                  g_value_get_boxed (value));
      break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
penge_people_tile_dispose (GObject *object)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (object);

  if (priv->item)
  {
    mojito_item_unref (priv->item);
    priv->item = NULL;
  }

  G_OBJECT_CLASS (penge_people_tile_parent_class)->dispose (object);
}

static void
penge_people_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (penge_people_tile_parent_class)->finalize (object);
}

static void
penge_people_tile_class_init (PengePeopleTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *pspec;

  g_type_class_add_private (klass, sizeof (PengePeopleTilePrivate));

  object_class->get_property = penge_people_tile_get_property;
  object_class->set_property = penge_people_tile_set_property;
  object_class->dispose = penge_people_tile_dispose;
  object_class->finalize = penge_people_tile_finalize;

  pspec = g_param_spec_boxed ("item",
                              "Item",
                              "The item to show",
                              MOJITO_TYPE_ITEM,
                              G_PARAM_READWRITE);
  g_object_class_install_property (object_class, PROP_ITEM, pspec);
}

static void
_remove_clicked_cb (PengeInterestingTile *tile,
                    gpointer              userdata)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);
  MojitoClient *client;

  client = penge_people_pane_dup_mojito_client_singleton ();
  mojito_client_hide_item (client, priv->item);
  g_object_unref (client);
}

static void
_clicked_cb (MxButton *button,
             gpointer  userdata)
{
  PengePeopleTile *tile = PENGE_PEOPLE_TILE (button);
  PengePeopleTilePrivate *priv = GET_PRIVATE (button);

  penge_people_tile_activate (tile, priv->item);
}

static void
penge_people_tile_init (PengePeopleTile *self)
{
  g_signal_connect (self,
                    "remove-clicked",
                    (GCallback)_remove_clicked_cb,
                    self);
  g_signal_connect (self,
                    "clicked",
                    (GCallback)_clicked_cb,
                    self);
}

void
penge_people_tile_activate (PengePeopleTile *tile,
                            MojitoItem      *item)
{
  const gchar *url;

  url = g_hash_table_lookup (item->props,
                             "url");

  if (!penge_utils_launch_for_uri ((ClutterActor *)tile, url))
  {
    g_warning (G_STRLOC ": Error launching uri: %s",
               url);
  } else {
    penge_utils_signal_activated ((ClutterActor *)tile);
  }
}

static void
penge_people_tile_set_item (PengePeopleTile *tile,
                            MojitoItem      *item)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);
  ClutterActor *body, *tmp_text;
  ClutterActor *label;
  const gchar *content, *thumbnail;
  gchar *date;
  GError *error = NULL;
  ClutterActor *avatar = NULL;
  ClutterActor *avatar_bin;
  const gchar *author_icon;

  if (priv->item == item)
    return;

  if (priv->item)
    mojito_item_unref (priv->item);

  if (item)
    priv->item = mojito_item_ref (item);
  else
    priv->item = NULL;

  if (!priv->item)
    return;

  priv->double_size = FALSE;

  if (mojito_item_has_key (item, "thumbnail"))
  {
    thumbnail = mojito_item_get_value (item, "thumbnail");
    body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

    if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                       thumbnail, 
                                       &error))
    {
      g_object_set (tile,
                    "body",
                    body,
                    NULL);
    } else {
      g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                  error->message);
      g_clear_error (&error);
    }
  } else if (mojito_item_has_key (item, "content")) {
    content = mojito_item_get_value (item, "content");
    body = mx_table_new ();

    mx_table_set_col_spacing (MX_TABLE (body), 6);
    mx_widget_set_style_class_name (MX_WIDGET (body),
                                    "PengePeopleTileContentBackground");

    author_icon = mojito_item_get_value (item, "authoricon");
    avatar = clutter_texture_new_from_file (author_icon, NULL);
    avatar_bin = mx_bin_new ();
    mx_bin_set_child (MX_BIN (avatar_bin), avatar);
    mx_bin_set_fill (MX_BIN (avatar_bin), TRUE, TRUE);
    mx_widget_set_style_class_name (MX_WIDGET (avatar_bin),
                                    "PengePeopleTileAvatarBackground");
    mx_table_add_actor_with_properties (MX_TABLE (body),
                                        avatar_bin,
                                        0, 0,
                                        "x-expand", FALSE,
                                        "y-expand", TRUE,
                                        "x-fill", FALSE,
                                        "y-fill", FALSE,
                                        "x-align", 0.5,
                                        "y-align", 0.5,
                                        NULL);
    clutter_actor_set_size (avatar_bin, 48, 48);

    label = mx_label_new (content);
    mx_widget_set_style_class_name (MX_WIDGET (label), "PengePeopleTileContentLabel");
    mx_table_add_actor_with_properties (MX_TABLE (body),
                                        label,
                                        0, 1,
                                        "x-expand", TRUE,
                                        "y-expand", TRUE,
                                        "x-fill", TRUE,
                                        "y-fill", TRUE,
                                        "x-align", 0.0,
                                        "y-align", 0.0,
                                        NULL);

    tmp_text = mx_label_get_clutter_text (MX_LABEL (label));
    clutter_text_set_line_wrap (CLUTTER_TEXT (tmp_text), TRUE);
    clutter_text_set_line_wrap_mode (CLUTTER_TEXT (tmp_text),
                                     PANGO_WRAP_WORD_CHAR);
    clutter_text_set_ellipsize (CLUTTER_TEXT (tmp_text),
                                PANGO_ELLIPSIZE_END);
    clutter_text_set_line_alignment (CLUTTER_TEXT (tmp_text),
                                     PANGO_ALIGN_LEFT);
    clutter_text_set_use_markup (CLUTTER_TEXT (tmp_text),
                                 TRUE);
    g_object_set (tile,
                  "body", body,
                  NULL);
    priv->double_size = TRUE;
  } else {
    if (g_str_equal (item->service, "lastfm"))
    {
      body = g_object_new (PENGE_TYPE_MAGIC_TEXTURE, NULL);

      if (clutter_texture_set_from_file (CLUTTER_TEXTURE (body),
                                         DEFAULT_ALBUM_ARTWORK,
                                         &error))
      {
        g_object_set (tile,
                      "body",
                      body,
                      NULL);
      } else {
        g_critical (G_STRLOC ": Loading thumbnail failed: %s",
                    error->message);
        g_clear_error (&error);
      }
    } else {
      g_assert_not_reached ();
    }
  }

  if (mojito_item_has_key (item, "title"))
  {
    if (mojito_item_has_key (item, "author"))
    {
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "title"),
                    "secondary-text", mojito_item_get_value (item, "author"),
                    NULL);
    } else {
      date = mx_utils_format_time (&(item->date));
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "title"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
    }
  } else if (mojito_item_has_key (item, "author")) {
      date = mx_utils_format_time (&(item->date));
      g_object_set (tile,
                    "primary-text", mojito_item_get_value (item, "author"),
                    "secondary-text", date,
                    NULL);
      g_free (date);
  } else {
    g_assert_not_reached ();
  }

  if (!avatar)
  {
    g_object_set (tile,
                  "icon-path", mojito_item_get_value (item, "authoricon"),
                  NULL);
  } else {
    g_object_set (tile,
                  "icon-path", NULL,
                  NULL);
  }
}

gboolean
penge_people_tile_is_double_size (PengePeopleTile *tile)
{
  PengePeopleTilePrivate *priv = GET_PRIVATE (tile);

  return priv->double_size;
}
