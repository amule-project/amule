// GeoIPConfigDlg.h
#ifndef GEOIPCONFIGDLG_H
#define GEOIPCONFIGDLG_H

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

class GeoIPConfigDlg : public wxDialog
{
public:
    GeoIPConfigDlg(wxWindow* parent, 
                  const wxString& title = _("GeoIP Configuration"),
                  const wxString& currentUrl = wxEmptyString);
    
    wxString GetDownloadUrl() const { return m_textCtrl->GetValue(); }
    
private:
    wxTextCtrl* m_textCtrl;
    
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

#endif // GEOIPCONFIGDLG_H