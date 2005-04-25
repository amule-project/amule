#include <wx/version.h>

#ifdef __WXMSW__
	#warning This shouldnt be built for win32
#elif wxCHECK_VERSION(2, 6, 0)
	#include "extern/listctrl.260.cpp"
#elif wxCHECK_VERSION(2, 4, 2)
	#include "extern/listctrl.242.cpp"
#else
	#error Unsupported version of wxWidgets used!
#endif
