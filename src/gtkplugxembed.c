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

/* By Owen Taylor <otaylor@gtk.org>              98/4/4 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "gtk2-funcs.h"
#include "xembed.h"
#include "gtkplugxembed.h"

#if !USE_WX_TRAY
#ifdef __WXGTK__

#include <gtk/gtk.h>
#include <gtk/gtkprivate.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>


static void            gtk_plug_xembed_class_init            (GtkPlugXEmbedClass *klass);
static void            gtk_plug_xembed_init                  (GtkPlugXEmbed      *plug);
/*static void            gtk_plug_xembed_finalize              (GtkObject          *object);*/
static void            gtk_plug_xembed_realize               (GtkWidget          *widget);
static void            gtk_plug_xembed_unrealize             (GtkWidget          *widget);
static void            gtk_plug_xembed_show                  (GtkWidget          *widget);
static void            gtk_plug_xembed_hide                  (GtkWidget          *widget);
static void            gtk_plug_xembed_map                   (GtkWidget          *widget);
static void            gtk_plug_xembed_unmap                 (GtkWidget          *widget);
static void            gtk_plug_xembed_size_allocate         (GtkWidget          *widget,
							      GtkAllocation      *allocation);
static void            gtk_plug_xembed_size_request          (GtkWidget          *widget,
							      GtkRequisition     *requisition);
static gboolean        gtk_plug_xembed_key_press_event       (GtkWidget          *widget,
							      GdkEventKey        *event);
static gboolean        gtk_plug_xembed_focus_event           (GtkWidget          *widget,
							      GdkEventFocus      *event);
static void            gtk_plug_xembed_set_focus             (GtkWindow          *window,
							      GtkWidget          *focus);
#ifdef PORT_COMPLETE
static gboolean        gtk_plug_xembed_focus                 (GtkWidget          *widget,
							      GtkDirectionType    direction);
#endif
static void            gtk_plug_xembed_check_resize          (GtkContainer       *container);
#ifdef PORT_COMPLETE
static void            gtk_plug_xembed_keys_changed          (GtkWindow          *window);
#endif
static GdkFilterReturn gtk_plug_xembed_filter_func           (GdkXEvent          *gdk_xevent,
							      GdkEvent           *event,
							      gpointer            data);

static void handle_modality_off        (GtkPlugXEmbed       *plug);
static void send_xembed_message        (GtkPlugXEmbed       *plug,
					glong                message,
					glong                detail,
					glong                data1,
					glong                data2,
					guint32              time);
static void xembed_set_info            (GdkWindow           *window,
					unsigned long        flags);


/* From Tk */
#define EMBEDDED_APP_WANTS_FOCUS NotifyNormal+20
  
static GtkWindowClass *parent_class = NULL;
static GtkBinClass *bin_class = NULL;

enum {
  EMBEDDED,
  LAST_SIGNAL
}; 

GtkType
gtk_plug_xembed_get_type ()
{
  static GtkType plug_type = 0;

  if (!plug_type)
    {
      static const GtkTypeInfo plug_info =
      {
	      "GtkPlugXEmbed",
	      sizeof (GtkPlugXEmbed),
	      sizeof (GtkPlugXEmbedClass),
	      (GtkClassInitFunc) gtk_plug_xembed_class_init,
	      (GtkObjectInitFunc) gtk_plug_xembed_init,
	      NULL,
	      NULL,
	      NULL
      };

      plug_type = gtk_type_unique (GTK_TYPE_WINDOW, &plug_info);
    }

  return plug_type;
}

static void
gtk_plug_xembed_class_init (GtkPlugXEmbedClass *class)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass *)class;
  GtkWindowClass *window_class = (GtkWindowClass *)class;
  GtkContainerClass *container_class = (GtkContainerClass *)class;

  parent_class = gtk_type_class (GTK_TYPE_WINDOW);
  bin_class = gtk_type_class (GTK_TYPE_BIN);

  /* breaks gtk2
   *gtk_object_class->finalize = gtk_plug_xembed_finalize;
   */

  widget_class->realize = gtk_plug_xembed_realize;
  widget_class->unrealize = gtk_plug_xembed_unrealize;
  widget_class->key_press_event = gtk_plug_xembed_key_press_event;
  widget_class->focus_in_event = gtk_plug_xembed_focus_event;
  widget_class->focus_out_event = gtk_plug_xembed_focus_event;

  widget_class->show = gtk_plug_xembed_show;
  widget_class->hide = gtk_plug_xembed_hide;
  widget_class->map = gtk_plug_xembed_map;
  widget_class->unmap = gtk_plug_xembed_unmap;
  widget_class->size_allocate = gtk_plug_xembed_size_allocate;
  widget_class->size_request = gtk_plug_xembed_size_request;

#ifdef PORT_COMPLETE
  widget_class->focus = gtk_plug_xembed_focus;
#endif

  container_class->check_resize = gtk_plug_xembed_check_resize;

  window_class->set_focus = gtk_plug_xembed_set_focus;
#ifdef PORT_COMPLETE
  window_class->keys_changed = gtk_plug_xembed_keys_changed;
#endif

#if SIGNAL_ENABLED
   /* for some reason, this _will_ segfault
      so I removed it. it is not needed anyway */
  plug_signals[EMBEDDED] =
	  gtk_signal_new ("embedded",
			  GTK_RUN_LAST,
			  GTK_OBJECT_CLASS (class),
			  GTK_STRUCT_OFFSET (GtkPlugXEmbedClass, embedded),
			  gtk_marshal_NONE__NONE,
			  GTK_TYPE_NONE, 0);
#endif
}

static void
gtk_plug_xembed_init (GtkPlugXEmbed *plug)
{
  GtkWindow *window;

  window = GTK_WINDOW (plug);

  window->type = GTK_WINDOW_TOPLEVEL;
}

#ifdef PORT_COMPLETE
static void
gtk_plug_xembed_set_is_child (GtkPlugXEmbed  *plug,
		       gboolean  is_child)
{
  g_assert (!GTK_WIDGET (plug)->parent);
      
  if (is_child)
    {
#if PORT_COMPLETE
      if (plug->modality_window)
	handle_modality_off (plug);

      if (plug->modality_group)
	{
	  gtk_window_group_remove_window (plug->modality_group, GTK_WINDOW (plug));
	  gtk_object_unref (plug->modality_group);
	  plug->modality_group = NULL;
	}
#endif

      /* As a toplevel, the MAPPED flag doesn't correspond
       * to whether the widget->window is mapped; we unmap
       * here, but don't bother remapping -- we will get mapped
       * by gtk_widget_set_parent ().
       */
      if (GTK_WIDGET_MAPPED (plug))
	gtk_widget_unmap (GTK_WIDGET (plug));
      
      GTK_WIDGET_UNSET_FLAGS (plug, GTK_TOPLEVEL);
      gtk_container_set_resize_mode (GTK_CONTAINER (plug), GTK_RESIZE_PARENT);

#ifdef PORT_COMPLETE
      _gtk_widget_propagate_hierarchy_changed (GTK_WIDGET (plug), GTK_WIDGET (plug));
#endif
    }
  else
    {
      if (GTK_WINDOW (plug)->focus_widget)
	gtk_window_set_focus (GTK_WINDOW (plug), NULL);
      if (GTK_WINDOW (plug)->default_widget)
	gtk_window_set_default (GTK_WINDOW (plug), NULL);
	  
#ifdef PORT_COMPLETE
      plug->modality_group = gtk_window_group_new ();
      gtk_window_group_add_window (plug->modality_group, GTK_WINDOW (plug));
#endif

      GTK_WIDGET_SET_FLAGS (plug, GTK_TOPLEVEL);
      gtk_container_set_resize_mode (GTK_CONTAINER (plug), GTK_RESIZE_QUEUE);

#ifdef PORT_COMPLETE
      _gtk_widget_propagate_hierarchy_changed (GTK_WIDGET (plug), NULL);
#endif
    }
}
#endif

/**
 * _gtk_plug_xembed_add_to_socket:
 * @plug: a #GtkPlugXEmbed
 * @socket: a #GtkSocket
 * 
 * Adds a plug to a socket within the same application.
 **/
void
_gtk_plug_xembed_add_to_socket (GtkPlugXEmbed   *plug,
				GtkSocket *socket)
{ (void)plug; (void)socket;
#ifdef PORT_COMPLETE /* Only foreign sockets for now */
  GtkWidget *widget;
  gint w, h;
  
  g_return_if_fail (GTK_IS_PLUG_XEMBED (plug));
  g_return_if_fail (GTK_IS_SOCKET (socket));
  g_return_if_fail (GTK_WIDGET_REALIZED (socket));

  widget = GTK_WIDGET (plug);

  gtk_plug_xembed_set_is_child (plug, TRUE);
  plug->same_app = TRUE;
  socket->same_app = TRUE;
  socket->plug_widget = widget;

  plug->socket_window = GTK_WIDGET (socket)->window;

  if (GTK_WIDGET_REALIZED (widget))
    {
      gdk_drawable_get_size (GDK_DRAWABLE (widget->window), &w, &h);
      gdk_window_reparent (widget->window, plug->socket_window, -w, -h);
    }

  gtk_widget_set_parent (widget, GTK_WIDGET (socket));

  g_signal_emit_by_name (G_OBJECT (socket), "plug_added", 0);
#endif
}

/**
 * _gtk_plug_xembed_remove_from_socket:
 * @plug: a #GtkPlugXEmbed
 * @socket: a #GtkSocket
 * 
 * Removes a plug from a socket within the same application.
 **/
void
_gtk_plug_xembed_remove_from_socket (GtkPlugXEmbed   *plug,
				     GtkSocket *socket)
{ (void)plug; (void)socket;
#ifdef PORT_COMPLETE /* Only foreign sockets for now */
  GtkWidget *widget;
  GdkEvent event;
  gboolean result;
  gboolean widget_was_visible;

  g_return_if_fail (GTK_IS_PLUG_XEMBED (plug));
  g_return_if_fail (GTK_IS_SOCKET (socket));
  g_return_if_fail (GTK_WIDGET_REALIZED (plug));

  widget = GTK_WIDGET (plug);

  gtk_object_ref (plug);
  gtk_object_ref (socket);

  widget_was_visible = GTK_WIDGET_VISIBLE (plug);
  
  gdk_window_hide (widget->window);
  gdk_window_reparent (widget->window, GDK_ROOT_PARENT (), 0, 0);

  GTK_PRIVATE_SET_FLAG (plug, GTK_IN_REPARENT);
  gtk_widget_unparent (GTK_WIDGET (plug));
  GTK_PRIVATE_UNSET_FLAG (plug, GTK_IN_REPARENT);
  
  socket->plug_widget = NULL;
  gdk_window_unref (socket->plug_window);
  socket->plug_window = NULL;
  
  socket->same_app = FALSE;

  plug->same_app = FALSE;
  plug->socket_window = NULL;

  gtk_plug_xembed_set_is_child (plug, FALSE);
		    
  g_signal_emit_by_name (G_OBJECT (socket), "plug_removed", &result);
  if (!result)
    gtk_widget_destroy (GTK_WIDGET (socket));

  event.any.type = GDK_DELETE;
  event.any.window = gdk_window_ref (widget->window);
  event.any.send_event = FALSE;
  
  if (!gtk_widget_event (widget, &event))
    gtk_widget_destroy (widget);
  
  gdk_window_unref (event.any.window);
  gtk_object_unref (plug);

  if (widget_was_visible && GTK_WIDGET_VISIBLE (socket))
    gtk_widget_queue_resize (GTK_WIDGET (socket));

  gtk_object_unref (socket);
#endif
}

void
gtk_plug_xembed_construct (GtkPlugXEmbed         *plug,
			   GtkPlugXEmbedNativeWindow  socket_id)
{
  if (socket_id)
    {
#ifdef PORT_COMPLETE /* Only foreign windows for now */
      gpointer user_data = NULL;

      plug->socket_window = gdk_window_lookup (socket_id);

      if (plug->socket_window)
	gdk_window_get_user_data (plug->socket_window, &user_data);
      else
	plug->socket_window = gdk_window_foreign_new (socket_id);

      if (user_data)
	{
	  if (GTK_IS_SOCKET (user_data))
	    _gtk_plug_xembed_add_to_socket (plug, user_data);
	  else
	    {
		    g_warning (/*G_STRLOC*/ "Can't create GtkPlugXEmbed as child of non-GtkSocket");
	      plug->socket_window = NULL;
	    }
	}
#if SIGNAL_ENABLED
      if (plug->socket_window)
	g_signal_emit (G_OBJECT (plug), plug_signals[EMBEDDED], 0);
#endif
#else
      plug->socket_window = gdk_window_foreign_new (socket_id);
#endif
      
    }
}

GtkWidget*
gtk_plug_xembed_new (GtkPlugXEmbedNativeWindow socket_id)
{
  GtkPlugXEmbed *plug;

  plug = GTK_PLUG_XEMBED (gtk_type_new (GTK_TYPE_PLUG_XEMBED));
  gtk_plug_xembed_construct (plug, socket_id);
  return GTK_WIDGET (plug);
}

/**
 * gtk_plug_xembed_get_id:
 * @plug: a #GtkPlugXEmbed.
 * 
 * Gets the window ID of a #GtkPlugXEmbed widget, which can then
 * be used to embed this window inside another window, for
 * instance with gtk_socket_add_id().
 * 
 * Return value: the window ID for the plug
 **/
GtkPlugXEmbedNativeWindow
gtk_plug_xembed_get_id (GtkPlugXEmbed *plug)
{
  g_return_val_if_fail (GTK_IS_PLUG_XEMBED (plug), 0);

  if (!GTK_WIDGET_REALIZED (plug))
    gtk_widget_realize (GTK_WIDGET (plug));

  return GDK_WINDOW_XWINDOW (GTK_WIDGET (plug)->window);
}

static void
gtk_plug_xembed_unrealize (GtkWidget *widget)
{
  GtkPlugXEmbed *plug;

  g_return_if_fail (GTK_IS_PLUG_XEMBED (widget));

  plug = GTK_PLUG_XEMBED (widget);

  if (plug->socket_window != NULL)
    {
      gdk_window_set_user_data (plug->socket_window, NULL);
      gdk_window_unref (plug->socket_window);
      plug->socket_window = NULL;
    }

#ifdef PORT_COMPLETE
  if (!plug->same_app)
    {
      if (plug->modality_window)
	handle_modality_off (plug);

      gtk_window_group_remove_window (plug->modality_group, GTK_WINDOW (plug));
      gtk_object_unref (plug->modality_group);
    }
#endif
  
  if (GTK_WIDGET_CLASS (parent_class)->unrealize)
    (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

static void
gtk_plug_xembed_realize (GtkWidget *widget)
{
  GtkWindow *window;
  GtkPlugXEmbed *plug;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (GTK_IS_PLUG_XEMBED (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  window = GTK_WINDOW (widget);
  plug = GTK_PLUG_XEMBED (widget);

  attributes.window_type = GDK_WINDOW_CHILD;	/* XXX GDK_WINDOW_PLUG ? */
  attributes.title = window->title;
  attributes.wmclass_name = window->wmclass_name;
  attributes.wmclass_class = window->wmclass_class;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;

  /* this isn't right - we should match our parent's visual/colormap.
   * though that will require handling "foreign" colormaps */
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK |
			    GDK_KEY_PRESS_MASK |
			    GDK_KEY_RELEASE_MASK |
			    GDK_ENTER_NOTIFY_MASK |
			    GDK_LEAVE_NOTIFY_MASK |
			    GDK_FOCUS_CHANGE_MASK |
			    GDK_STRUCTURE_MASK);

  attributes_mask = GDK_WA_VISUAL | GDK_WA_COLORMAP;
  attributes_mask |= (window->title ? GDK_WA_TITLE : 0);
  attributes_mask |= (window->wmclass_name ? GDK_WA_WMCLASS : 0);

  if (GTK_WIDGET_TOPLEVEL (widget))
    {
      attributes.window_type = GDK_WINDOW_TOPLEVEL;

      gdk_error_trap_push ();
      widget->window = gdk_window_new (plug->socket_window, 
				       &attributes, attributes_mask);
      gdk_flush ();
      if (gdk_error_trap_pop ()) /* Uh-oh */
	{
	  gdk_error_trap_push ();
	  gdk_window_destroy (widget->window);
	  gdk_flush ();
	  gdk_error_trap_pop ();
	  widget->window = gdk_window_new (NULL, &attributes, attributes_mask);
	}
      
      gdk_window_add_filter (widget->window, gtk_plug_xembed_filter_func, widget);

#ifdef PORT_COMPLETE
      plug->modality_group = gtk_window_group_new ();
      gtk_window_group_add_window (plug->modality_group, window);
#endif
      
      xembed_set_info (widget->window, 0);
    }
  else
    widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);      
  
  gdk_window_set_user_data (widget->window, window);

  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
gtk_plug_xembed_show (GtkWidget *widget)
{
  if (GTK_WIDGET_TOPLEVEL (widget))
    GTK_WIDGET_CLASS (parent_class)->show (widget);
  else
    GTK_WIDGET_CLASS (bin_class)->show (widget);
}

static void
gtk_plug_xembed_hide (GtkWidget *widget)
{
  if (GTK_WIDGET_TOPLEVEL (widget))
    GTK_WIDGET_CLASS (parent_class)->hide (widget);
  else
    GTK_WIDGET_CLASS (bin_class)->hide (widget);
}

static void
gtk_plug_xembed_map (GtkWidget *widget)
{
  if (GTK_WIDGET_TOPLEVEL (widget)) {
	  GtkBin *bin = GTK_BIN (widget);
	  
	  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
	  
	  if (bin->child &&
	      GTK_WIDGET_VISIBLE (bin->child) &&
	      !GTK_WIDGET_MAPPED (bin->child))
		  gtk_widget_map (bin->child);
	  xembed_set_info (widget->window, XEMBED_MAPPED);
	  if (!GTK_WIDGET_NO_WINDOW (widget))
		  gdk_window_show (widget->window);

#ifdef PORT_COMPLETE
	  gdk_synthesize_window_state (widget->window,
				       GDK_WINDOW_STATE_WITHDRAWN,
				       0);
#endif
  } else {
	  GTK_WIDGET_CLASS (bin_class)->map (widget);
  }
}

static void
gtk_plug_xembed_unmap (GtkWidget *widget)
{
  if (GTK_WIDGET_TOPLEVEL (widget))
    {
      GTK_WIDGET_UNSET_FLAGS (widget, GTK_MAPPED);

      gdk_window_hide (widget->window);
      xembed_set_info (widget->window, 0);

#ifdef PORT_COMPLETE      
      gdk_synthesize_window_state (widget->window,
				   0,
				   GDK_WINDOW_STATE_WITHDRAWN);
#endif
    }
  else
    GTK_WIDGET_CLASS (bin_class)->unmap (widget);
}

static void
gtk_plug_xembed_size_request (GtkWidget     *widget,
			      GtkRequisition *requisition)
{ (void)widget;
  requisition->width = 22;
  requisition->height = 22;
}

static void
gtk_plug_xembed_size_allocate (GtkWidget     *widget,
			       GtkAllocation *allocation)
{
  if (GTK_WIDGET_TOPLEVEL (widget))
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);
  else
    {
      GtkBin *bin = GTK_BIN (widget);

      widget->allocation = *allocation;

      if (GTK_WIDGET_REALIZED (widget))
	gdk_window_move_resize (widget->window,
				allocation->x, allocation->y,
				allocation->width, allocation->height);

      if (bin->child && GTK_WIDGET_VISIBLE (bin->child))
	{
	  GtkAllocation child_allocation;
	  
	  child_allocation.x = child_allocation.y = GTK_CONTAINER (widget)->border_width;
	  child_allocation.width =
	    MAX (1, (gint)allocation->width - child_allocation.x * 2);
	  child_allocation.height =
	    MAX (1, (gint)allocation->height - child_allocation.y * 2);
	  
	  gtk_widget_size_allocate (bin->child, &child_allocation);
	}
      
    }
}

static gboolean
gtk_plug_xembed_key_press_event (GtkWidget   *widget,
			  GdkEventKey *event)
{
  if (GTK_WIDGET_TOPLEVEL (widget))
    return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event);
  else
    return FALSE;
}

static gboolean
gtk_plug_xembed_focus_event (GtkWidget      *widget,
		      GdkEventFocus  *event)
{ (void)widget; (void)event;
  /* We eat focus-in events and focus-out events, since they
   * can be generated by something like a keyboard grab on
   * a child of the plug.
   */
  return FALSE;
}

static void
gtk_plug_xembed_set_focus (GtkWindow *window,
			   GtkWidget *focus)
{
  GtkPlugXEmbed *plug = GTK_PLUG_XEMBED (window);

  GTK_WINDOW_CLASS (parent_class)->set_focus (window, focus);
  
  /* Ask for focus from embedder
   */
#ifdef __GTK2__
  if (focus && !window->has_focus)
#elif GTK_CHECK_VERSION(1,2,9)
  if (focus && !window->window_has_focus)
#else
  if (focus)
#endif
    {
      send_xembed_message (plug, XEMBED_REQUEST_FOCUS, 0, 0, 0,
			   gtk2_get_current_event_time ());
    }
}

#ifdef PORT_COMPLETE
typedef struct
{
  guint			 accelerator_key;
  GdkModifierType	 accelerator_mods;
} GrabbedKey;

static guint
grabbed_key_hash (gconstpointer a)
{
  const GrabbedKey *key = a;
  guint h;
  
  h = key->accelerator_key << 16;
  h ^= key->accelerator_key >> 16;
  h ^= key->accelerator_mods;

  return h;
}

static gboolean
grabbed_key_equal (gconstpointer a, gconstpointer b)
{
  const GrabbedKey *keya = a;
  const GrabbedKey *keyb = b;

  return (keya->accelerator_key == keyb->accelerator_key &&
	  keya->accelerator_mods == keyb->accelerator_mods);
}

static void
add_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  GtkPlugXEmbed *plug = data;

  if (!plug->grabbed_keys ||
      !g_hash_table_lookup (plug->grabbed_keys, grabbed_key))
    {
      send_xembed_message (plug, XEMBED_GTK_GRAB_KEY, 0, 
			   grabbed_key->accelerator_key, grabbed_key->accelerator_mods,
			   gtk_get_current_event_time ());
    }
}

static void
add_grabbed_key_always (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  GtkPlugXEmbed *plug = data;

  send_xembed_message (plug, XEMBED_GTK_GRAB_KEY, 0, 
		       grabbed_key->accelerator_key, grabbed_key->accelerator_mods,
		       gtk_get_current_event_time ());
}

static void
remove_grabbed_key (gpointer key, gpointer val, gpointer data)
{
  GrabbedKey *grabbed_key = key;
  GtkPlugXEmbed *plug = data;

  if (!plug->grabbed_keys ||
      !g_hash_table_lookup (plug->grabbed_keys, grabbed_key))
    {
      send_xembed_message (plug, XEMBED_GTK_UNGRAB_KEY, 0, 
			   grabbed_key->accelerator_key, grabbed_key->accelerator_mods,
			   gtk_get_current_event_time ());
    }
}

static void
keys_foreach (GtkWindow      *window,
	      guint           keyval,
	      GdkModifierType modifiers,
	      gboolean        is_mnemonic,
	      gpointer        data)
{
  GHashTable *new_grabbed_keys = data;
  GrabbedKey *key = g_new (GrabbedKey, 1);

  key->accelerator_key = keyval;
  key->accelerator_mods = modifiers;
  
  g_hash_table_replace (new_grabbed_keys, key, key);
}

#ifdef PORT_COMPLETE
static void
gtk_plug_xembed_keys_changed (GtkWindow *window)
{
  GHashTable *new_grabbed_keys, *old_grabbed_keys;
  GtkPlugXEmbed *plug = GTK_PLUG_XEMBED (window);

  new_grabbed_keys = g_hash_table_new_full (grabbed_key_hash, grabbed_key_equal, (GDestroyNotify)g_free, NULL);
  _gtk_window_keys_foreach (window, keys_foreach, new_grabbed_keys);

  if (plug->socket_window)
    g_hash_table_foreach (new_grabbed_keys, add_grabbed_key, plug);

  old_grabbed_keys = plug->grabbed_keys;
  plug->grabbed_keys = new_grabbed_keys;

  if (old_grabbed_keys)
    {
      if (plug->socket_window)
	g_hash_table_foreach (old_grabbed_keys, remove_grabbed_key, plug);
      g_hash_table_destroy (old_grabbed_keys);
    }
}
#endif
#endif

#ifdef PORT_COMPLETE
static gboolean
gtk_plug_xembed_focus (GtkWidget        *widget,
		       GtkDirectionType  direction)
{
  GtkBin *bin = GTK_BIN (widget);
  GtkPlugXEmbed *plug = GTK_PLUG_XEMBED (widget);
  GtkWindow *window = GTK_WINDOW (widget);
  GtkContainer *container = GTK_CONTAINER (widget);
  GtkWidget *old_focus_child = container->focus_child;
  GtkWidget *parent;

  /* We override GtkWindow's behavior, since we don't want wrapping here.
   */
  if (old_focus_child)
    {
      if (gtk_widget_child_focus (old_focus_child, direction))
	return TRUE;

      if (window->focus_widget)
	{
	  /* Wrapped off the end, clear the focus setting for the toplevel */
	  parent = window->focus_widget->parent;
	  while (parent)
	    {
	      gtk_container_set_focus_child (GTK_CONTAINER (parent), NULL);
	      parent = GTK_WIDGET (parent)->parent;
	    }
	  
	  gtk_window_set_focus (GTK_WINDOW (container), NULL);

	  if (!GTK_CONTAINER (window)->focus_child)
	    {
	      gint message = -1;

	      switch (direction)
		{
		case GTK_DIR_UP:
		case GTK_DIR_LEFT:
		case GTK_DIR_TAB_BACKWARD:
		  message = XEMBED_FOCUS_PREV;
		  break;
		case GTK_DIR_DOWN:
		case GTK_DIR_RIGHT:
		case GTK_DIR_TAB_FORWARD:
		  message = XEMBED_FOCUS_NEXT;
		  break;
		}
	      
	      send_xembed_message (plug, message, 0, 0, 0,
				   gtk_get_current_event_time ());
	    }
	}

      return FALSE;
    }
  else
    {
      /* Try to focus the first widget in the window */
      
      if (bin->child && gtk_widget_child_focus (bin->child, direction))
        return TRUE;
    }

  return FALSE;
}
#endif

static void
gtk_plug_xembed_check_resize (GtkContainer *container)
{
  if (GTK_WIDGET_TOPLEVEL (container))
    GTK_CONTAINER_CLASS (parent_class)->check_resize (container);
  else
    GTK_CONTAINER_CLASS (bin_class)->check_resize (container);
}

static void
send_xembed_message (GtkPlugXEmbed *plug,
		     glong      message,
		     glong      detail,
		     glong      data1,
		     glong      data2,
		     guint32    time)
{
  if (plug->socket_window)
    {
      XEvent xevent;

      GTK_NOTE(PLUGSOCKET,
	       g_message ("GtkPlugXEmbed: Sending XEMBED message of type %ld", message));

      xevent.xclient.window = GDK_WINDOW_XWINDOW (plug->socket_window);
      xevent.xclient.type = ClientMessage;
      xevent.xclient.message_type = (Atom)gdk_atom_intern ("_XEMBED", FALSE);
      xevent.xclient.format = 32;
      xevent.xclient.data.l[0] = time;
      xevent.xclient.data.l[1] = message;
      xevent.xclient.data.l[2] = detail;
      xevent.xclient.data.l[3] = data1;
      xevent.xclient.data.l[4] = data2;

      gdk_error_trap_push ();
      XSendEvent (GDK_DISPLAY (),
		  GDK_WINDOW_XWINDOW (plug->socket_window),
		  False, NoEventMask, &xevent);
      gdk_flush ();
      gdk_error_trap_pop ();
    }
}

static void
focus_first_last (GtkPlugXEmbed          *plug,
		  GtkDirectionType  direction)
{ (void)direction;
  GtkWindow *window = GTK_WINDOW (plug);
  GtkWidget *parent;
  
  if (window->focus_widget)
    {
      parent = window->focus_widget->parent;
      while (parent)
	{
	  gtk_container_set_focus_child (GTK_CONTAINER (parent), NULL);
	  parent = GTK_WIDGET (parent)->parent;
	}
      
      gtk_window_set_focus (GTK_WINDOW (plug), NULL);
    }

#ifdef PORT_COMPLETE
  gtk_widget_child_focus (GTK_WIDGET (plug), direction);
#endif
}

static void
handle_modality_on (GtkPlugXEmbed *plug)
{ (void)plug;
#ifdef PORT_COMPLETE
  if (!plug->modality_window)
    {
      plug->modality_window = gtk_window_new (GTK_WINDOW_POPUP);
      gtk_widget_realize (plug->modality_window);
      gtk_window_group_add_window (plug->modality_group, GTK_WINDOW (plug->modality_window));
      gtk_grab_add (plug->modality_window);
    }
#else
  /* g_print("Modality On for plug %p\n", plug); */
#endif
}

static void
handle_modality_off (GtkPlugXEmbed *plug)
{ (void)plug;
#ifdef PORT_COMPLETE
  if (plug->modality_window)
    {
      gtk_widget_destroy (plug->modality_window);
      plug->modality_window = NULL;
    }
#else
  /* g_print("Modality Off for plug %p\n", plug); */
#endif
}

static void
xembed_set_info (GdkWindow     *gdk_window,
		 unsigned long  flags)
{
  Display *display = GDK_WINDOW_XDISPLAY (gdk_window);
  Window window = GDK_WINDOW_XWINDOW (gdk_window);
  unsigned long buffer[2];
  
  Atom xembed_info_atom = (Atom)gdk_atom_intern ("_XEMBED_INFO", FALSE);
                          
  buffer[1] = 0;		/* Protocol version */
  buffer[1] = flags;

  XChangeProperty (display, window,
		   xembed_info_atom, xembed_info_atom, 32,
		   PropModeReplace,
		   (unsigned char *)buffer, 2);
}

static void
handle_xembed_message (GtkPlugXEmbed   *plug,
		       glong      message,
		       glong      detail,
		       glong      data1,
		       glong      data2,
		       guint32    time)
{ (void)data1; (void)data2; (void)time;
  GTK_NOTE (PLUGSOCKET,
	    g_message ("GtkPlugXEmbed: Message of type %ld received\n", message));
  
  switch (message)
    {
    case XEMBED_EMBEDDED_NOTIFY:
      break;
    case XEMBED_WINDOW_ACTIVATE:
      GTK_NOTE(PLUGSOCKET,
	       g_message ("GtkPlugXEmbed: ACTIVATE received"));
      break;
    case XEMBED_WINDOW_DEACTIVATE:
      GTK_NOTE(PLUGSOCKET,
	       g_message ("GtkPlugXEmbed: DEACTIVATE received"));
      break;
      
    case XEMBED_MODALITY_ON:
      handle_modality_on (plug);
      break;
    case XEMBED_MODALITY_OFF:
      handle_modality_off (plug);
      break;

    case XEMBED_FOCUS_IN:
      switch (detail)
	{
	case XEMBED_FOCUS_FIRST:
	  focus_first_last (plug, GTK_DIR_TAB_FORWARD);
	  break;
	case XEMBED_FOCUS_LAST:
	  focus_first_last (plug, GTK_DIR_TAB_BACKWARD);
	  break;
	case XEMBED_FOCUS_CURRENT:
	  /* fall through */;
	}
      
    case XEMBED_FOCUS_OUT:
      {
	GtkWidget *widget = GTK_WIDGET (plug);
	GdkEvent event;

	event.focus_change.type = GDK_FOCUS_CHANGE;
	event.focus_change.window = widget->window;
	event.focus_change.send_event = TRUE;

	if (message == XEMBED_FOCUS_IN)
	  {
	    event.focus_change.in = TRUE;
	    GTK_WIDGET_CLASS (parent_class)->focus_in_event (widget, (GdkEventFocus *)&event);
	  }
	else
	  {
	    event.focus_change.in = FALSE;
	    GTK_WIDGET_CLASS (parent_class)->focus_out_event (widget, (GdkEventFocus *)&event);
	  }
	
	break;
      }
      
    case XEMBED_GRAB_KEY:
    case XEMBED_UNGRAB_KEY:
    case XEMBED_GTK_GRAB_KEY:
    case XEMBED_GTK_UNGRAB_KEY:
    case XEMBED_REQUEST_FOCUS:
    case XEMBED_FOCUS_NEXT:
    case XEMBED_FOCUS_PREV:
      g_warning ("GtkPlugXEmbed: Invalid _XEMBED message of type %ld received", message);
      break;
      
    default:
      GTK_NOTE(PLUGSOCKET,
	       g_message ("GtkPlugXEmbed: Ignoring unknown _XEMBED message of type %ld", message));
      break;
    }
}

static GdkFilterReturn
gtk_plug_xembed_filter_func (GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{ (void)event;
  GtkPlugXEmbed *plug = GTK_PLUG_XEMBED (data);
  XEvent *xevent = (XEvent *)gdk_xevent;

  GdkFilterReturn return_val;
  
  return_val = GDK_FILTER_CONTINUE;

  switch (xevent->type)
    {
    case ClientMessage:
      if (xevent->xclient.message_type == (Atom)gdk_atom_intern ("_XEMBED", FALSE))
	{
	  handle_xembed_message (plug,
				 xevent->xclient.data.l[1],
				 xevent->xclient.data.l[2],
				 xevent->xclient.data.l[3],
				 xevent->xclient.data.l[4],
				 xevent->xclient.data.l[0]);
				 

	  return GDK_FILTER_REMOVE;
	}
      else if (xevent->xclient.message_type == (Atom)gdk_atom_intern ("WM_DELETE_WINDOW", FALSE))
	{
	  /* We filter these out because we take being reparented back to the
	   * root window as the reliable end of the embedding protocol
	   */

	  return GDK_FILTER_REMOVE;
	}
      break;
    case ReparentNotify:
    {
	XReparentEvent *xre = &xevent->xreparent;
	gboolean was_embedded = plug->socket_window != NULL;

	return_val = GDK_FILTER_REMOVE;
	
	gtk_object_ref (GTK_OBJECT(plug));
	
	if (was_embedded)
	  {
	    /* End of embedding protocol for previous socket */
	    
	    /* FIXME: race if we remove from another socket and
	     * then add to a local window before we get notification
	     * Probably need check in _gtk_plug_xembed_add_to_socket
	     */
	    
	    if (xre->parent != GDK_WINDOW_XWINDOW (plug->socket_window))
	      {
		GtkWidget *widget = GTK_WIDGET (plug);

		gdk_window_set_user_data (plug->socket_window, NULL);
		gdk_window_unref (plug->socket_window);
		plug->socket_window = NULL;

		/* Emit a delete window, as if the user attempted
		 * to close the toplevel. Simple as to how we
		 * handle WM_DELETE_WINDOW, if it isn't handled
		 * we destroy the widget. BUt only do this if
		 * we are being reparented to the root window.
		 * Moving from one embedder to another should
		 * be invisible to the app.
		 */

		if (xre->parent == GDK_ROOT_WINDOW())
		  {
		    GdkEvent event;
		    
		    event.any.type = GDK_DELETE;
		    event.any.window = gdk_window_ref (widget->window);
		    event.any.send_event = FALSE;
		    
		    if (!gtk_widget_event (widget, &event))
		      gtk_widget_destroy (widget);
		    
		    gdk_window_unref (event.any.window);
		  }
	      }
	    else
	      goto done;
	  }

	if (xre->parent != GDK_ROOT_WINDOW ())
	  {
	    /* Start of embedding protocol */

	    plug->socket_window = gdk_window_lookup (xre->parent);
	    if (plug->socket_window)
	      {
		gpointer user_data = NULL;
		gdk_window_get_user_data (plug->socket_window, &user_data);

		if (user_data)
		  {
		    g_warning (/*G_STRLOC*/ "Plug reparented unexpectedly into window in the same process");
		    plug->socket_window = NULL;
		    break;
		  }
		
		gdk_window_ref (plug->socket_window);
	      }
	    else
	      {
		plug->socket_window = gdk_window_foreign_new (xre->parent);
		if (!plug->socket_window) /* Already gone */
		  break;
	      }

#ifdef PORT_COMPLETE
	    if (plug->grabbed_keys)
	      g_hash_table_foreach (plug->grabbed_keys, add_grabbed_key_always, plug);
#endif

#if SIGNAL_ENABLED
	    if (!was_embedded)
	      gtk_signal_emit (GTK_OBJECT (plug), plug_signals[EMBEDDED], 0);
#endif

	  }
      done:
	gtk_object_unref (GTK_OBJECT(plug));
	
	break;
      }
    }

  return GDK_FILTER_CONTINUE;
}

#endif /* __WXGTK__ */

#endif /* USE_WX_TRAY */
