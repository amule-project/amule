#include "ModernLogging.h"
#include "Logger.h"
#include "PerformanceUtils.h"

#ifdef USE_CPP20
void modern_log::Log(std::string_view msg, bool critical, std::source_location loc) {
    // Convert to traditional wxString for compatibility
    wxString wxmsg(msg.data(), wxConvUTF8, msg.size());
    #ifdef __DEBUG__
    theLogger.AddLogLine(
        wxString::FromUTF8(loc.file_name()), 
        loc.line(), 
        critical, 
        logStandard, 
        wxmsg
    );
    #else
    theLogger.AddLogLine(wxEmptyString, 0, critical, logStandard, wxmsg);
    #endif
}
#else
void modern_log::Log(const wxString& msg, bool critical) {
    theLogger.AddLogLine(wxEmptyString, 0, critical, logStandard, msg);
}
#endif