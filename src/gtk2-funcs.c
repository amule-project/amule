#include "gtk2-funcs.h"

#ifdef __WXGTK__

#include <gtk/gtk.h>
#include <gtk/gtkprivate.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>

extern guint32 gtk2_get_current_event_time (void);

guint32
gtk2_get_current_event_time (void)
{
	GdkEvent *ev = gtk_get_current_event();
	if ( ev ) {
		guint32 result;
		result = gdk_event_get_time(ev);
		gdk_event_free(ev);
		return result;
	} else {
		return GDK_CURRENT_TIME;
	}

}

#endif // __WXGTK__
