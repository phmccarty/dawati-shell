/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (c) 2008 Intel Corp.
 *
 * Author: Tomas Frydrych <tf@linux.intel.com>
 *         Thomas Wood <thomas@linux.intel.com>
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

#ifndef MOBLIN_NETBOOK_PANEL_H
#define MOBLIN_NETBOOK_PANEL_H

#include "moblin-netbook.h"

typedef enum
{
  MNBK_CONTROL_UNKNOWN = 0,
  MNBK_CONTROL_MZONE,
  MNBK_CONTROL_STATUS,
  MNBK_CONTROL_SPACES,
  MNBK_CONTROL_INTERNET,
  MNBK_CONTROL_MEDIA,
  MNBK_CONTROL_APPLICATIONS,
  MNBK_CONTROL_PEOPLE,
  MNBK_CONTROL_PASTEBOARD,
} MnbkControl;

ClutterActor *make_panel (MutterPlugin *plugin, gint width);
gboolean      hide_panel (MutterPlugin *plugin);
void          show_panel (MutterPlugin *plugin, gboolean from_keyboard);
void          show_panel_and_control (MutterPlugin *plugin,
                                      MnbkControl   control);

#endif
