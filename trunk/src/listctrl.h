#include <wx/version.h>

#ifdef __WXMAC__
	#include "extern/listctrl.old.h"
#elif wxCHECK_VERSION(2, 5, 4)
	#include "extern/listctrl.254.h"
#elif wxCHECK_VERSION(2, 4, 2)
	#include "extern/listctrl.242.h"
#else
	#error Unsupported version of wxWidgets used!
#endif
