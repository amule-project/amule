// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#ifndef GTKPLUGXEMBED_H
#define GTKPLUGXEMBED_H

#include "MuleTrayIcon.h"

#if !USE_WX_TRAY

#ifdef __WXGTK__

#include <gdk/gdk.h>
#include <gtk/gtksocket.h>
#include <gtk/gtkwindow.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_TYPE_PLUG_XEMBED            (gtk_plug_xembed_get_type ())
#define GTK_PLUG_XEMBED(obj)            (GTK_CHECK_CAST ((obj), GTK_TYPE_PLUG_XEMBED, GtkPlugXEmbed))
#define GTK_PLUG_XEMBED_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_PLUG_XEMBED, GtkPlugXEmbedClass))
#define GTK_IS_PLUG_XEMBED(obj)         (GTK_CHECK_TYPE ((obj), GTK_TYPE_PLUG_XEMBED))
#define GTK_IS_PLUG_XEMBED_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_PLUG_XEMBED))
#define GTK_PLUG_XEMBED_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GTK_TYPE_PLUG_XEMBED, GtkPlugXEmbedClass))


typedef struct _GtkPlugXEmbed        GtkPlugXEmbed;
typedef struct _GtkPlugXEmbedClass   GtkPlugXEmbedClass;

typedef guint32 GtkPlugXEmbedNativeWindow;

struct _GtkPlugXEmbed
{
  GtkWindow window;

  GdkWindow *socket_window;
#ifdef PORT_COMPLETE
  GtkWidget *modality_window;
  GtkWindowGroup *modality_group;
#endif
  GHashTable *grabbed_keys;

  guint same_app : 1;
};

struct _GtkPlugXEmbedClass
{
  GtkWindowClass parent_class;

  void (*embedded) (GtkPlugXEmbed *plug);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GtkType    gtk_plug_xembed_get_type  (void) G_GNUC_CONST;
void       gtk_plug_xembed_construct (GtkPlugXEmbed *plug, GtkPlugXEmbedNativeWindow socket_id);

GtkWidget*      gtk_plug_xembed_new    (GtkPlugXEmbedNativeWindow  socket_id);
GtkPlugXEmbedNativeWindow gtk_plug_xembed_get_id (GtkPlugXEmbed         *plug);

void _gtk_plug_xembed_add_to_socket      (GtkPlugXEmbed   *plug,
					  GtkSocket *socket);
void _gtk_plug_xembed_remove_from_socket (GtkPlugXEmbed   *plug,
					  GtkSocket *socket);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __WXGTK__

#endif //USE_WX_TRAY

#endif // GTKPLUGXEMBED_H
