#include <wx/version.h>

#ifdef __WXMAC__
	#include "extern/listctrl.old.cpp"
#elif wxCHECK_VERSION(2, 5, 4)
	#include "extern/listctrl.254.cpp"
#elif wxCHECK_VERSION(2, 4, 2)
	#include "extern/listctrl.242.cpp"
#else
	#error Unsupported version of wxWidgets used!
#endif
