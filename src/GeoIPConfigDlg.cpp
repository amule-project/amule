// GeoIPConfigDlg.cpp
#include "GeoIPConfigDlg.h"
#include "geoip/IP2CountryManager.h"
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include "Logger.h"  // For AddLogLineN

wxBEGIN_EVENT_TABLE(GeoIPConfigDlg, wxDialog)
    EVT_BUTTON(wxID_OK, GeoIPConfigDlg::OnOK)
    EVT_BUTTON(wxID_CANCEL, GeoIPConfigDlg::OnCancel)
    EVT_BUTTON(wxID_DEFAULT, GeoIPConfigDlg::OnDefault)
wxEND_EVENT_TABLE()

GeoIPConfigDlg::GeoIPConfigDlg(wxWindow* parent, 
                             const wxString& title,
                             const wxString& currentUrl)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, 
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // Check for obsolete GeoIP.dat.gz URLs and replace with default
    wxString displayUrl = currentUrl;
    if (currentUrl.Contains("geolite.maxmind.com") || 
        currentUrl.Contains("GeoIP.dat.gz")) {
        displayUrl = "https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb";
    }
    
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Information text
    wxStaticText* infoText = new wxStaticText(this, wxID_ANY,
        _("Configure the download URL for GeoIP database:\n\n"
          "Supported formats:\n"
          "• https://example.com/database.mmdb.gz (gzip compressed)\n"
          "• https://example.com/database.mmdb (raw database)\n"
          "• file:///path/to/local/database.mmdb (local file)\n\n"
          "Note: Old GeoIP.dat.gz URLs are automatically updated to modern format.\n"
          "Download runs in background - you can continue using aMule while downloading."));
    
    mainSizer->Add(infoText, 0, wxALL | wxEXPAND, 10);
    
    // URL input
    wxStaticText* urlLabel = new wxStaticText(this, wxID_ANY, _("Download URL:"));
    mainSizer->Add(urlLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    
    m_textCtrl = new wxTextCtrl(this, wxID_ANY, displayUrl, 
                               wxDefaultPosition, wxSize(400, -1));
    mainSizer->Add(m_textCtrl, 0, wxALL | wxEXPAND, 10);
    
    // Show warning if URL was updated from obsolete format
    if (displayUrl != currentUrl) {
        wxStaticText* warningText = new wxStaticText(this, wxID_ANY,
            _("WARNING: Obsolete GeoIP URL detected and automatically updated to modern format."));
        warningText->SetForegroundColour(wxColour(255, 140, 0)); // Orange color for warning
        mainSizer->Add(warningText, 0, wxALL | wxEXPAND, 10);
    }
    
    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* defaultButton = new wxButton(this, wxID_DEFAULT, _("&Default"));
    wxButton* okButton = new wxButton(this, wxID_OK, _("&OK"));
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, _("&Cancel"));
    
    buttonSizer->Add(defaultButton, 0, wxRIGHT, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    buttonSizer->Add(cancelButton, 0);
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizerAndFit(mainSizer);
    Center();
}

void GeoIPConfigDlg::OnOK(wxCommandEvent& event)
{
    wxString url = m_textCtrl->GetValue().Trim();
    
    // Basic validation
    if (url.IsEmpty()) {
        wxMessageBox(_("Please enter a download URL"), _("Error"), 
                    wxOK | wxICON_ERROR, this);
        return;
    }
    
    if (!url.StartsWith("http://") && 
        !url.StartsWith("https://") && 
        !url.StartsWith("file://")) {
        if (wxMessageBox(
            _("The URL doesn't start with http://, https://, or file://.\n"
              "Do you want to use it anyway?"),
            _("Warning"), wxYES_NO | wxICON_WARNING, this) == wxNO) {
            return;
        }
    }
    
    event.Skip(); // Allow dialog to close
    
    // Download in background using UpdateScheduler
    // Set custom URL in manager
    IP2CountryManager::GetInstance().SetDatabaseDownloadUrl(url);
    
    // Log download started instead of showing dialog
    AddLogLineN(_("GeoIP database download started in background. Check the log for progress and completion status."));
    
    // Start async download
    IP2CountryManager::GetInstance().CheckForUpdates();
}

void GeoIPConfigDlg::OnCancel(wxCommandEvent& event)
{
    event.Skip();
}

void GeoIPConfigDlg::OnDefault(wxCommandEvent& event)
{
    m_textCtrl->SetValue("https://cdn.jsdelivr.net/gh/8bitsaver/maxmind-geoip@release/GeoLite2-Country.mmdb");
}