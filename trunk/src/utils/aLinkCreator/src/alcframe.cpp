//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         AlcFrame Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Pixmaps from http://www.everaldo.com and http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
///  it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "alcframe.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/filedlg.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include "alcframe.h"
#include "alchash.h"
#include "alcpix.h"

// Needed for 2.4.2 backward compatibility
#if (wxMINOR_VERSION < 5)
#define wxCLOSE_BOX 0
#endif

/// Constructor
AlcFrame::AlcFrame (const wxString & title):
    wxFrame ((wxFrame *) NULL, -1, title)
{
  // Give it an icon
#if defined(__WXMSW__)
  wxIcon icon("alc");
#else

  wxIcon icon;
  icon.CopyFromBitmap(AlcPix::getPixmap(wxT("Alc")));
  SetIcon (icon);
#endif

  // Status Bar
  CreateStatusBar ();
  SetStatusText (_("Welcome!"));

  // Frame Vertical sizer
  m_frameVBox = new wxBoxSizer (wxVERTICAL);

  // Add Main panel to frame (needed by win32 for padding sub panels)
  m_mainPanel = new wxPanel (this, -1);

  // Main Panel Vertical Sizer
  m_mainPanelVBox = new wxBoxSizer (wxVERTICAL);

  // Main Panel static line
  m_staticLine = new wxStaticLine (m_mainPanel, -1);
  m_mainPanelVBox->Add (m_staticLine, 0, wxALL | wxGROW);

  // MD4 Hash Vertical Box Sizer
  m_md4HashSBox = new wxStaticBox (m_mainPanel, -1, _("MD4 File Hash"));
  m_md4HashSBoxSizer = new wxStaticBoxSizer (m_md4HashSBox, wxHORIZONTAL);

  // MD4 Hash results
  m_md4HashTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, wxTE_READONLY );


  wxInt32 x, y;
  m_md4HashTextCtrl->GetTextExtent (wxT("8"), &x, &y);
  m_md4HashTextCtrl->SetSize (wxSize (33 * x, -1)); // 32 + 1 characters
  
  m_md4HashSBoxSizer->Add (m_md4HashTextCtrl, 1, wxALL | wxALIGN_CENTER, 5);
  m_mainPanelVBox->Add( m_md4HashSBoxSizer, 0, wxALL | wxGROW, 10 );

  // Hash Vertical Box Sizer
  m_hashSBox = new wxStaticBox (m_mainPanel, -1, _("Ed2k File Hash"));
  m_hashSBoxSizer = new wxStaticBoxSizer (m_hashSBox, wxHORIZONTAL);

  // Hash results
  m_hashTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                   wxDefaultSize, wxTE_READONLY );


  m_hashSBoxSizer->Add (m_hashTextCtrl, 1, wxALL | wxALIGN_CENTER, 5);
  m_mainPanelVBox->Add( m_hashSBoxSizer, 0, wxALL | wxGROW, 10 );

  // Ed2k Vertical Box Sizer
  m_ed2kSBox = new wxStaticBox (m_mainPanel, -1, _("Ed2k link"));
  m_ed2kSBoxSizer = new wxStaticBoxSizer (m_ed2kSBox, wxVERTICAL);

  // Ed2k results
  m_ed2kTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                   wxSize(-1,40), wxTE_MULTILINE|wxTE_READONLY|wxVSCROLL );



  m_ed2kSBoxSizer->Add (m_ed2kTextCtrl, 1, wxALL | wxGROW, 5);
  m_mainPanelVBox->Add( m_ed2kSBoxSizer, 1, wxALL | wxGROW, 10 );

  // Toolbar Pixmaps
  m_toolBarBitmaps[0] = AlcPix::getPixmap(wxT("open"));
  m_toolBarBitmaps[1] = AlcPix::getPixmap(wxT("saveas"));
  m_toolBarBitmaps[2] = AlcPix::getPixmap(wxT("about"));

  // Constructing toolbar
  m_toolbar =
    new wxToolBar (this, -1, wxDefaultPosition, wxDefaultSize,
                   wxTB_HORIZONTAL | wxTB_FLAT);

  m_toolbar->SetToolBitmapSize (wxSize (32, 32));

  m_toolbar->AddTool (ID_BAR_OPEN, wxT("Open"), m_toolBarBitmaps[0],
                      _("Open a file to compute its ed2k link"));

  m_toolbar->AddTool (ID_BAR_SAVEAS, wxT("Save as"), m_toolBarBitmaps[1],
                      _("Save computed ed2k link to file"));

  m_toolbar->AddSeparator ();

  m_toolbar->AddTool (ID_BAR_ABOUT, wxT("About"), m_toolBarBitmaps[2],
                      _("About Alc"));

  m_toolbar->SetMargins (2, 2);
  m_toolbar->Realize ();

  SetToolBar (m_toolbar);

  // Main panel Layout
  m_mainPanel->SetAutoLayout(true);
  m_mainPanel->SetSizerAndFit (m_mainPanelVBox);

  // Frame Layout
  m_frameVBox->Add (m_mainPanel, 1, wxALL | wxGROW, 0);
  SetAutoLayout (TRUE);
  SetSizerAndFit (m_frameVBox);
}

// Destructor
AlcFrame::~AlcFrame ()
{}

// Events table
BEGIN_EVENT_TABLE (AlcFrame, wxFrame)
EVT_TOOL (ID_BAR_OPEN, AlcFrame::OnBarOpen)
EVT_TOOL (ID_BAR_SAVEAS, AlcFrame::OnBarSaveAs)
EVT_TOOL (ID_BAR_ABOUT, AlcFrame::OnBarAbout)
END_EVENT_TABLE ()

// Open button
void
AlcFrame::OnBarOpen (wxCommandEvent & event)
{
  const wxString & filename =
    wxFileSelector (_("Select the file you want to compute the ed2k link"),
                    wxFileName::GetHomeDir(),wxEmptyString, wxEmptyString, wxT("*.*"),
                    wxOPEN | wxFILE_MUST_EXIST );

  if (!filename.empty ())
    {
      AlcHash hash (filename);
      m_md4HashTextCtrl->SetValue(hash.GetMD4Hash());
    }
  SetStatusText (_("Ed2k computed from ")+filename);
}

// Save as button
void
AlcFrame::OnBarSaveAs (wxCommandEvent & event)
{
  const wxString & filename =
    wxFileSelector (_("Select the file to your computed ed2k link"),
                    wxFileName::GetHomeDir(),wxT("my_ed2k_link"), wxT("txt"), wxT("*.txt"),
                    wxSAVE );

  if (!filename.empty ())
    {
      // Openning streams to write cleaned file
      wxFileOutputStream outputStream( filename );
      wxTextOutputStream txtOutStream( outputStream );

      txtOutStream << m_ed2kTextCtrl->GetValue() <<endl;
    }

  SetStatusText (_("Ed2k link saved to ")+filename);
}

// About button
void
AlcFrame::OnBarAbout (wxCommandEvent & event)
{
  wxMessageBox (_
                ("aLinkCreator, the aMule ed2k link creator\n\n"
                 "(c) 2004 ThePolish <thepolish@vipmail.ru>\n\n"
                 "Pixmaps from http://www.everaldo.com and http://www.amule.org\n\n"
                 "Distributed under GPL"),
                _("About aLinkCreator"), wxOK | wxCENTRE | wxICON_INFORMATION);
}
