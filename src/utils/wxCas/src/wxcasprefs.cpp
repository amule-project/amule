//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wxCas
// Purpose:    Display aMule Online Statistics
// Author:       ThePolish <thepolish@vipmail.ru>
// Copyright (C) 2004 by ThePolish
//
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
//
// This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "wxcasprefs.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/dirdlg.h>
#include <wx/config.h>

#include "wxcasprefs.h"
#include "wxcascte.h"

// Constructor
WxCasPrefs::WxCasPrefs (wxWindow * parent):wxDialog (parent, -1,
        wxString (_
                  ("Preferences")))
{
  // Prefs
  wxConfigBase * prefs = wxConfigBase::Get();

  // Main vertical Sizer
  m_mainVBox = new wxBoxSizer (wxVERTICAL);

  // Note
  m_noteStaticText =
    new wxStaticText (this, -1,
                      _
                      ("WxCas must be restarted for modifications to take place"),
                      wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE);
  m_noteStaticText->SetForegroundColour (*wxRED);

  m_mainVBox->Add (m_noteStaticText, 0, wxGROW | wxALIGN_CENTER | wxALL, 10);

  // OS Path
  m_osPathSBox =
    new wxStaticBox (this, -1, _("Directory containing amulesig.dat file"));
  m_osPathSBoxSizer = new wxStaticBoxSizer (m_osPathSBox, wxHORIZONTAL);


  m_osPathTextCtrl = new wxTextCtrl (this, -1, wxEmptyString);
  m_osPathBrowseButton =
    new wxButton (this, ID_OSPATH_BROWSE_BUTTON, wxString (_("Browse")));

  wxString str;
  wxInt32 x, y;
  prefs->Read (WxCasCte::AMULESIG_DIR_KEY, &str,
               WxCasCte::DEFAULT_AMULESIG_PATH);
  m_osPathTextCtrl->GetTextExtent ("8", &x, &y);
  m_osPathTextCtrl->SetSize (wxSize (x * (str.Length () + 1), -1));
  m_osPathTextCtrl->SetValue (str);
  m_osPathTextCtrl->
  SetToolTip (_
              ("Enter here the directory where your amulesig.dat file is"));

  m_osPathSBoxSizer->Add (m_osPathTextCtrl, 1, wxALL | wxALIGN_CENTER, 5);
  m_osPathSBoxSizer->Add (m_osPathBrowseButton, 0, wxALL | wxALIGN_CENTER, 5);

  m_mainVBox->Add (m_osPathSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL, 10);

  // Refresh rate
  m_refreshSBox = new wxStaticBox (this, -1, wxEmptyString);
  m_refreshSBoxSizer = new wxStaticBoxSizer (m_refreshSBox, wxHORIZONTAL);


  m_refreshSpinButton = new wxSpinCtrl (this, -1);
  m_refreshSpinButton->SetRange (WxCasCte::MIN_REFRESH_RATE,
                                 WxCasCte::MAX_REFRESH_RATE);
  m_refreshSpinButton->SetValue (prefs->
                                 Read (WxCasCte::REFRESH_RATE_KEY,
                                       WxCasCte::DEFAULT_REFRESH_RATE));
  m_refreshStaticText =
    new wxStaticText (this, -1, _("Refresh rate interval"), wxDefaultPosition,
                      wxDefaultSize, wxALIGN_CENTRE);
  m_refreshSBoxSizer->Add (m_refreshSpinButton, 0, wxALL | wxALIGN_CENTER, 5);
  m_refreshSBoxSizer->Add (m_refreshStaticText, 1, wxALL | wxALIGN_CENTER, 5);

  m_mainVBox->Add (m_refreshSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL,
                   10);

  // Auto generate stat image
  m_autoStatImgSBox = new wxStaticBox (this, -1, wxEmptyString);
  m_autoStatImgSBoxSizer =
    new wxStaticBoxSizer (m_autoStatImgSBox, wxVERTICAL);

  m_autoStatImgCheck =
    new wxCheckBox (this, ID_AUTOSTATIMG_RADIO,
                    _
                    ("Generate a stat image at every refresh event (Eat CPU)"),
                    wxDefaultPosition, wxDefaultSize, wxRB_SINGLE);
  m_autoStatImgSBoxSizer->Add (m_autoStatImgCheck, 0,
                               wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_autoStatImgHBoxSizer = new wxBoxSizer (wxHORIZONTAL);

  wxString strs[] =
    {
      _T ("PNG"), _T ("JPG"), _T ("BMP")};

  m_autoStatImgCombo =
    new wxComboBox (this, ID_AUTOSTATIMG_COMBO,
                    prefs->
                    Read (WxCasCte::AUTOSTATIMG_TYPE_KEY,
                          WxCasCte::DEFAULT_AUTOSTATIMG_TYPE),
                    wxDefaultPosition, wxDefaultSize, 3, strs,
                    wxCB_DROPDOWN | wxCB_READONLY);
  m_autoStatImgCombo->Enable (FALSE);

  m_autoStatImgTextCtrl = new wxTextCtrl (this, -1, wxEmptyString);
  m_autoStatImgTextCtrl->SetValue (prefs->
                                   Read (WxCasCte::AUTOSTATIMG_DIR_KEY,
                                         WxCasCte::DEFAULT_AUTOSTATIMG_PATH));
  m_autoStatImgTextCtrl->
  SetToolTip (_
              ("Enter here the directory where you want to generate the statistic image"));
  m_autoStatImgTextCtrl->Enable (FALSE);

  m_autoStatImgButton =
    new wxButton (this, ID_AUTOSTATIMG_BROWSE_BUTTON, wxString (_("Browse")));
  m_autoStatImgButton->Enable (FALSE);

  m_autoStatImgHBoxSizer->Add (m_autoStatImgCombo, 0, wxALIGN_CENTER | wxALL,
                               5);
  m_autoStatImgHBoxSizer->Add (m_autoStatImgTextCtrl, 1,
                               wxALIGN_CENTER | wxALL, 5);
  m_autoStatImgHBoxSizer->Add (m_autoStatImgButton, 0, wxALIGN_CENTER | wxALL,
                               5);

  m_autoStatImgSBoxSizer->Add (m_autoStatImgHBoxSizer, 0,
                               wxGROW | wxALIGN_CENTER_VERTICAL | wxALL, 5);

  m_mainVBox->Add (m_autoStatImgSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL,
                   5);

  if ((bool)
      (prefs->
       Read (WxCasCte::ENABLE_AUTOSTATIMG_KEY,
             WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED)))
    {
      m_autoStatImgTextCtrl->Enable (TRUE);
      m_autoStatImgButton->Enable (TRUE);
      m_autoStatImgCombo->Enable (TRUE);
      m_autoStatImgCheck->SetValue (TRUE);
    }

  // Separator line
  m_staticLine = new wxStaticLine (this, -1);
  m_mainVBox->Add (m_staticLine, 0, wxGROW | wxALIGN_CENTER | wxALL);

  // Button bar
  m_buttonHBox = new wxBoxSizer (wxHORIZONTAL);
  m_validateButton =
    new wxButton (this, ID_VALIDATE_BUTTON, wxString (_("Validate")));
  m_cancelButton =
    new wxButton (this, ID_CANCEL_BUTTON, wxString (_("Cancel")));

  m_buttonHBox->Add (m_validateButton, 0, wxALIGN_CENTER | wxALL, 5);
  m_buttonHBox->Add (m_cancelButton, 0, wxALIGN_CENTER | wxALL, 5);

  m_mainVBox->Add (m_buttonHBox, 0, wxALIGN_CENTER | wxALL, 10);

  // Layout
  SetAutoLayout (TRUE);
  SetSizerAndFit (m_mainVBox);

  m_validateButton->SetFocus ();
  m_validateButton->SetDefault ();
}

// Destructor
WxCasPrefs::~WxCasPrefs ()
{}

// Events table
BEGIN_EVENT_TABLE (WxCasPrefs, wxDialog)
EVT_BUTTON (ID_OSPATH_BROWSE_BUTTON, WxCasPrefs::OnOSPathBrowseButton)
EVT_BUTTON (ID_AUTOSTATIMG_BROWSE_BUTTON, WxCasPrefs::OnAutoStatImgBrowseButton)
EVT_BUTTON (ID_VALIDATE_BUTTON, WxCasPrefs::OnValidateButton)
EVT_BUTTON (ID_CANCEL_BUTTON, WxCasPrefs::OnCancel)	// Defined in wxDialog
EVT_CHECKBOX (ID_AUTOSTATIMG_RADIO, WxCasPrefs::OnAutoStatImgRadio)
END_EVENT_TABLE ()

// Browse for OS Path
void WxCasPrefs::OnOSPathBrowseButton (wxCommandEvent & event)
{
  const wxString & dir =
    wxDirSelector (_("Folder containing your signature file"),
                   WxCasCte::DEFAULT_AMULESIG_PATH);

  if (!dir.empty ())
    {
      m_osPathTextCtrl->SetValue (dir);
    }
}

// Browse for stat image Path
void
WxCasPrefs::OnAutoStatImgBrowseButton (wxCommandEvent & event)
{
  const wxString & dir =
    wxDirSelector (_("Folder where generating the statistic image"),
                   WxCasCte::DEFAULT_AUTOSTATIMG_PATH);

  if (!dir.empty ())
    {
      m_autoStatImgTextCtrl->SetValue (dir);
    }
}

// Auto Generate Stat Image
void
WxCasPrefs::OnAutoStatImgRadio (wxCommandEvent & event)
{
  if (m_autoStatImgTextCtrl->IsEnabled ())
    {
      m_autoStatImgTextCtrl->Enable (FALSE);
      m_autoStatImgButton->Enable (FALSE);
      m_autoStatImgCombo->Enable (FALSE);
      m_autoStatImgCheck->SetValue (FALSE);
    }
  else
    {
      m_autoStatImgTextCtrl->Enable (TRUE);
      m_autoStatImgButton->Enable (TRUE);
      m_autoStatImgCombo->Enable (TRUE);
      m_autoStatImgCheck->SetValue (TRUE);
    }
}

// Validate Prefs
void
WxCasPrefs::OnValidateButton (wxCommandEvent & event)
{
  // Prefs
  wxConfigBase * prefs = wxConfigBase::Get();

  prefs->Write (WxCasCte::AMULESIG_DIR_KEY,
                m_osPathTextCtrl->GetValue ());
  prefs->Write (WxCasCte::REFRESH_RATE_KEY,
                m_refreshSpinButton->GetValue ());

  prefs->Write (WxCasCte::ENABLE_AUTOSTATIMG_KEY,
                m_autoStatImgCheck->GetValue ());

  if (m_autoStatImgCheck->GetValue ())
    {
      prefs->Write (WxCasCte::AUTOSTATIMG_DIR_KEY,
                    m_autoStatImgTextCtrl->GetValue ());

      prefs->Write (WxCasCte::AUTOSTATIMG_TYPE_KEY,
                    m_autoStatImgCombo->GetValue ());
    }

  // Close window
  this->EndModal (this->GetReturnCode ());
}
