// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
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

#ifndef GTK2_FUNCS_H
#define GTK2_FUNCS_H

#include <wx/version.h>

#if wxCHECK_VERSION(2, 5, 3)
 #define USE_WX_TRAY 1
#endif

#if !USE_WX_TRAY

#ifdef __WXGTK__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern guint32 gtk2_get_current_event_time (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __WXGTK__
#endif // USE_WX_TRAY
#endif // GTK2_FUNCS_H
