// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
//
/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* eggtrayicon.h
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef EGGTRAYICON_H
#define EGGTRAYICON_H

#include "gtkplugxembed.h"	// Needed for GtkPlugXEmbed

#ifdef __WXGTK__ // Uses GTK code, don't compile on other platforms.

#include <gdk/gdkx.h>		// Needed for Atom

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define EGG_TYPE_TRAY_ICON		(egg_tray_icon_get_type ())
#define EGG_TRAY_ICON(obj)		(GTK_CHECK_CAST ((obj), EGG_TYPE_TRAY_ICON, EggTrayIcon))
#define EGG_TRAY_ICON_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), EGG_TYPE_TRAY_ICON, EggTrayIconClass))
#define EGG_IS_TRAY_ICON(obj)		(GTK_CHECK_TYPE ((obj), EGG_TYPE_TRAY_ICON))
//#define EGG_IS_TRAY_ICON_CLASS(klass)	(GTK_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_TRAY_ICON))
//#define EGG_TRAY_ICON_GET_CLASS(obj)	(GTK_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_TRAY_ICON, EggTrayIconClass))
	
typedef struct _EggTrayIcon	  EggTrayIcon;
typedef struct _EggTrayIconClass  EggTrayIconClass;

struct _EggTrayIcon
{
  GtkPlugXEmbed parent_instance;

  guint stamp;
  
  Atom selection_atom;
  Atom manager_atom;
  Atom system_tray_opcode_atom;
  Window manager_window;
};

struct _EggTrayIconClass
{
  GtkPlugXEmbedClass parent_class;
};

GtkType      egg_tray_icon_get_type       (void);

EggTrayIcon *
egg_tray_icon_new_for_xscreen (Screen *xscreen, const char *name);
#if EGG_TRAY_ENABLE_MULTIHEAD
EggTrayIcon *egg_tray_icon_new_for_screen (GdkScreen   *screen,
					   const gchar *name);
#endif

EggTrayIcon *egg_tray_icon_new            (const gchar *name);

guint        egg_tray_icon_send_message   (EggTrayIcon *icon,
					   gint         timeout,
					   const char  *message,
					   gint         len);
void         egg_tray_icon_cancel_message (EggTrayIcon *icon,
					   guint        id);


					    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __WXGTK__

#endif // EGGTRAYICON_H
