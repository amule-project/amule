#include <wx/version.h>

#if wxCHECK_VERSION(2, 6, 0)
	#if defined(__WIN32__)
		#include <wx/imaglist.h>
	#endif
#include "extern/listctrl.cvs.h"
#else
	#error Unsupported version of wxWidgets used!
#endif

