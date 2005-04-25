#include <wx/version.h>

#if wxCHECK_VERSION(2, 6, 0)
	#include "extern/listctrl.260.cpp"
#elif wxCHECK_VERSION(2, 4, 2)
	#include "extern/listctrl.242.cpp"
#else
	#error Unsupported version of wxWidgets used!
#endif
