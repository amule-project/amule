#include <wx/version.h>

#if defined(__WXMAC__ ) and !wxCHECK_VERSION(2,5,5)
	#include "extern/listctrl.old.h"
#elif wxCHECK_VERSION(2, 5, 4)
	#include "extern/listctrl.254.h"
#elif wxCHECK_VERSION(2, 4, 2)
	#include "extern/listctrl.242.h"
#else
	#error Unsupported version of wxWidgets used!
#endif
