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
/// it under the terms of the GNU General Public License as published by
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// the Free Software Foundation; either version 2 of the License, or
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

#ifndef WX_PRECOMP
#include <wx/filedlg.h>
#include <wx/textfile.h>
#include <wx/file.h>
#include <wx/timer.h>
#include <wx/listbox.h>
#include <wx/url.h>
#include <wx/filename.h>
#endif

#include "md4.h"
#include "ed2khash.h"
#include "alcframe.h"
#include "alcpix.h"

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

  // Input Parameters
  m_inputSBox =
    new wxStaticBox (m_mainPanel, -1, _("Input parameters"));
  m_inputSBoxSizer = new wxStaticBoxSizer (m_inputSBox, wxHORIZONTAL);

  // Input Grid
  m_inputFlexSizer = new wxFlexGridSizer (6, 2, 5, 10);

  // Left col is growable
  m_inputFlexSizer->AddGrowableCol (0);

  // Static texts
  m_inputFileStaticText=new wxStaticText(m_mainPanel, -1,
                                         _("File to Hash"),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE);

  m_inputAddStaticText=new wxStaticText(m_mainPanel, -1,
                                        _("Add Optional URLs for this file"),wxDefaultPosition,wxDefaultSize,wxALIGN_CENTRE);

  // Text ctrls
  m_inputFileTextCtrl = new wxTextCtrl (m_mainPanel,-1,wxEmptyString,
                                        wxDefaultPosition, wxSize(300,-1));
  m_inputFileTextCtrl->
  SetToolTip (_
              ("Enter here the file you want to compute the Ed2k link"));

  m_inputAddTextCtrl = new wxTextCtrl (m_mainPanel,-1,wxEmptyString,
                                       wxDefaultPosition, wxSize(300,-1));
  m_inputAddTextCtrl->
  SetToolTip (_
              ("Enter here the URL you want to add to the Ed2k link: "
               "Add / at the end to let aLinkCreator append the current file name"));

  // List box
  m_inputUrlListBox = new wxListBox(m_mainPanel, -1, wxDefaultPosition,
                                    wxDefaultSize, 0, NULL, wxLB_SINGLE | wxLB_NEEDED_SB | wxLB_HSCROLL);

  // Buttons
  m_inputFileBrowseButton =
    new wxButton (m_mainPanel, ID_BROWSE_BUTTON, wxString (_("Browse")));

  m_inputAddButton =
    new wxButton (m_mainPanel, ID_ADD_BUTTON, wxString (_("Add")));

  // Button bar
  m_buttonUrlVBox = new wxBoxSizer (wxVERTICAL);
  m_removeButton =
    new wxButton (m_mainPanel, ID_REMOVE_BUTTON, wxString (_("Remove")));
  m_clearButton =
    new wxButton (m_mainPanel, ID_CLEAR_BUTTON, wxString (_("Clear")));

  m_buttonUrlVBox->Add (m_removeButton, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 5);
  m_buttonUrlVBox->Add (m_clearButton, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 5);

  // Check button
  m_parthashesCheck =
    new wxCheckBox (m_mainPanel, ID_PARTHASHES_CHECK,
                    _
                    ("Create link with part-hashes"));

  m_parthashesCheck->SetValue(FALSE);

  m_parthashesCheck->
  SetToolTip (_
              ("Help to spread new and rare files faster, at the cost of an increased link size"));

  // Add to sizers
  m_inputFlexSizer->Add (m_inputFileStaticText, 1, wxGROW | wxALIGN_BOTTOM | wxTOP, 10);
  m_inputFlexSizer->Add (1,1);

  m_inputFlexSizer->Add (m_inputFileTextCtrl, 1, wxGROW | wxALIGN_TOP , 0);
  m_inputFlexSizer->Add (m_inputFileBrowseButton, 0, wxGROW | wxALIGN_TOP , 0);

  m_inputFlexSizer->Add (m_inputAddStaticText, 1, wxGROW | wxALIGN_BOTTOM | wxTOP, 10);
  m_inputFlexSizer->Add (1,1);

  m_inputFlexSizer->Add (m_inputAddTextCtrl, 1, wxGROW | wxALIGN_TOP , 0);
  m_inputFlexSizer->Add (m_inputAddButton, 0, wxGROW | wxALIGN_TOP , 0);

  m_inputFlexSizer->Add (m_inputUrlListBox, 0, wxGROW | wxALIGN_CENTER , 0);
  m_inputFlexSizer->Add (m_buttonUrlVBox, 0, wxGROW | wxALIGN_CENTER , 0);

  m_inputFlexSizer->Add (m_parthashesCheck, 0, wxGROW | wxALIGN_CENTER | wxTOP, 10);
  m_inputFlexSizer->Add (1,1);

  m_inputSBoxSizer->Add (m_inputFlexSizer, 1, wxALL | wxALIGN_CENTER | wxALL, 10);
  m_mainPanelVBox->Add (m_inputSBoxSizer, 0, wxGROW | wxALIGN_CENTER | wxALL, 10);

#ifdef WANT_MD4SUM
  // MD4 Hash Vertical Box Sizer
  m_md4HashSBox = new wxStaticBox (m_mainPanel, -1, _("MD4 File Hash"));
  m_md4HashSBoxSizer = new wxStaticBoxSizer (m_md4HashSBox, wxHORIZONTAL);

  // MD4 Hash results
  m_md4HashTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, wxTE_READONLY );

  m_md4HashSBoxSizer->Add (m_md4HashTextCtrl, 1, wxALL | wxALIGN_CENTER, 5);
  m_mainPanelVBox->Add( m_md4HashSBoxSizer, 0, wxALL | wxGROW, 10 );
#endif

  // Hash Vertical Box Sizer
  m_e2kHashSBox = new wxStaticBox (m_mainPanel, -1, _("Ed2k File Hash"));
  m_e2kHashSBoxSizer = new wxStaticBoxSizer (m_e2kHashSBox, wxHORIZONTAL);

  // Hash results
  m_e2kHashTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, wxTE_READONLY );

  m_e2kHashSBoxSizer->Add (m_e2kHashTextCtrl, 1, wxALL | wxALIGN_CENTER, 5);
  m_mainPanelVBox->Add( m_e2kHashSBoxSizer, 0, wxALL | wxGROW, 10 );

  // Ed2k Vertical Box Sizer
  m_ed2kSBox = new wxStaticBox (m_mainPanel, -1, _("Ed2k link"));
  m_ed2kSBoxSizer = new wxStaticBoxSizer (m_ed2kSBox, wxVERTICAL);

  // Ed2k results
  m_ed2kTextCtrl = new wxTextCtrl( m_mainPanel, -1, wxEmptyString, wxDefaultPosition,
                                   wxSize(-1,60), wxTE_MULTILINE|wxTE_READONLY|wxVSCROLL );

  m_ed2kSBoxSizer->Add (m_ed2kTextCtrl, 1, wxALL | wxGROW, 5);
  m_mainPanelVBox->Add( m_ed2kSBoxSizer, 1, wxALL | wxGROW, 10 );

  // Activitybar
  m_activityBar = new ActivityBar(m_mainPanel, -1, 100, 10, wxDefaultPosition, wxSize(-1,10));
  m_mainPanelVBox->Add( m_activityBar, 0, wxLEFT | wxRIGHT | wxGROW, 10 );

  // Button bar
  m_buttonHBox = new wxBoxSizer (wxHORIZONTAL);
  m_startButton =
    new wxButton (m_mainPanel, ID_START_BUTTON, wxString (_("Start")));
  m_saveButton =
    new wxButton (m_mainPanel, ID_SAVEAS_BUTTON, wxString (_("Save")));
  m_closeButton =
    new wxButton (m_mainPanel, ID_EXIT_BUTTON, wxString (_("Exit")));

  m_buttonHBox->Add (m_startButton, 0, wxALIGN_CENTER | wxALL, 5);
  m_buttonHBox->Add (m_saveButton, 0, wxALIGN_CENTER | wxALL, 5);
  m_buttonHBox->Add (m_closeButton, 0, wxALIGN_CENTER | wxALL, 5);

  m_mainPanelVBox->Add (m_buttonHBox, 0,  wxALIGN_CENTER | wxALL, 5);

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
                      _("About aLinkCreator"));

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

  m_startButton->SetFocus();
}

/// Destructor
AlcFrame::~AlcFrame ()
{}

/// Events table
BEGIN_EVENT_TABLE (AlcFrame, wxFrame)
EVT_TOOL (ID_BAR_OPEN, AlcFrame::OnBarOpen)
EVT_TOOL (ID_BAR_SAVEAS, AlcFrame::OnBarSaveAs)
EVT_TOOL (ID_BAR_ABOUT, AlcFrame::OnBarAbout)
EVT_BUTTON (ID_START_BUTTON, AlcFrame::OnStartButton)
EVT_BUTTON (ID_EXIT_BUTTON, AlcFrame::OnCloseButton)
EVT_BUTTON (ID_SAVEAS_BUTTON, AlcFrame::OnSaveAsButton)
EVT_BUTTON (ID_BROWSE_BUTTON, AlcFrame::OnBrowseButton)
EVT_BUTTON (ID_ADD_BUTTON, AlcFrame::OnAddUrlButton)
EVT_BUTTON (ID_REMOVE_BUTTON, AlcFrame::OnRemoveUrlButton)
EVT_BUTTON (ID_CLEAR_BUTTON, AlcFrame::OnClearUrlButton)
END_EVENT_TABLE ()

/// Toolbar Open button
void
AlcFrame::OnBarOpen (wxCommandEvent & event)
{
  SetFileToHash();
}

/// Browse button to select file to hash
void
AlcFrame::OnBrowseButton (wxCommandEvent & event)
{
  SetFileToHash();
}

/// Set File to hash in wxTextCtrl
void
AlcFrame::SetFileToHash()
{
  const wxString & filename =
    wxFileSelector (_("Select the file you want to compute the ed2k link"),
                    wxFileName::GetHomeDir(),wxEmptyString, wxEmptyString, wxT("*.*"),
                    wxOPEN | wxFILE_MUST_EXIST );

  if (!filename.empty ())
    {
      m_inputFileTextCtrl->SetValue(filename);
    }
}

/// Toolbar Save As button
void
AlcFrame::OnBarSaveAs (wxCommandEvent & event)
{
  SaveEd2kLinkToFile();
}

/// Save As button
void
AlcFrame::OnSaveAsButton(wxCommandEvent & event)
{
  SaveEd2kLinkToFile();
}

/// Save computed Ed2k link to file
void
AlcFrame::SaveEd2kLinkToFile()
{
  wxString link(m_ed2kTextCtrl->GetValue());

  if (!link.IsEmpty())
    {
      const wxString & filename =
        wxFileSelector (_("Select the file to your computed ed2k link"),
                        wxFileName::GetHomeDir(),wxT("my_ed2k_link"),
                        wxT("txt"), wxT("*.txt"), wxSAVE );

      if (!filename.empty ())
        {
          // Open file and let wxFile destructor close the file
          // Closing it explicitly may crash on Win32 ...
          wxFile file(filename,wxFile::write_append);
          if (! file.IsOpened())
            {
              SetStatusText (_("Unable to open ") + filename);
              return;
            }
          file.Write(link + wxTextFile::GetEOL());
        }
      else
        {
          SetStatusText (_("Please, enter a non empty file name"));
        }
    }
  else
    {
      SetStatusText (_("Nothing to save for now !"));
    }
}

/// Toolbar About button
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

/// Close Button
void AlcFrame::OnCloseButton (wxCommandEvent & event)
{
  Close (FALSE);
}

/// Hook into MD4/ED2K routine
static bool alc_frame_hook(int percent);
bool alc_frame_hook(int percent)
{
	::wxSafeYield();

	return true;
}

/// Compute Hashes on Start Button
void AlcFrame::OnStartButton (wxCommandEvent & event)
{
  int i;
  wxString filename = m_inputFileTextCtrl->GetValue();

  if (!filename.empty ())
    {
      // Chrono
      wxStopWatch chrono;

      // wxFileName needed for base name
      wxFileName fileToHash(filename);

      m_e2kHashTextCtrl->SetValue(_("Hashing..."));
      m_ed2kTextCtrl->SetValue(_("Hashing..."));

#ifdef WANT_MD4SUM

      m_md4HashTextCtrl->SetValue(_("Hashing..."));
#endif

      m_activityBar->Start();

      // Compute ed2k Hash
      Ed2kHash hash;
      
      // Test the return value to see if was aborted.
      bool finished = hash.SetED2KHashFromFile(filename, alc_frame_hook);
      if (!finished) return;
      
      wxArrayString ed2kHash (hash.GetED2KHash());

      // Get URLs
      wxArrayString arrayOfUrls;
      wxString url;
      for (i=0;i < m_inputUrlListBox->GetCount();i++)
        {
          url=m_inputUrlListBox->GetString(i);
          if (url.Right(1) == wxT("/"))
            {
              url += fileToHash.GetFullName();
            }
          arrayOfUrls.Add(wxURL::ConvertToValidURI(url));
        }
      arrayOfUrls.Shrink();

#ifdef WANT_MD4SUM
      // Md4 hash
      MD4 md4;
      m_md4HashTextCtrl->SetValue(md4.calcMd4FromFile(filename));
#endif
      // Ed2k hash
      m_e2kHashTextCtrl->SetValue(ed2kHash.Last());

      // Ed2k link
      m_ed2kTextCtrl->SetValue(hash.GetED2KLink(arrayOfUrls,m_parthashesCheck->IsChecked()));

      m_activityBar->Stop();

      SetStatusText (wxString::Format(_("Done in %.2f s"),
                                      chrono.Time()*.001));
    }
  else
    {
      SetStatusText (_("Please, enter a non empty file name"));
    }
}

/// Add an URL to the URL list box
void
AlcFrame::OnAddUrlButton (wxCommandEvent & event)
{
  wxString url(m_inputAddTextCtrl->GetValue());

  if (!url.IsEmpty())
    {
      // Check if the URL already exist in list
      int i;
      bool UrlNotExists = TRUE;
      for (i=0;i < m_inputUrlListBox->GetCount();i++)
        {
          if (url == m_inputUrlListBox->GetString(i))
            {
              UrlNotExists =FALSE;
              break;
            }
        }

      // Add only a not already existant URL
      if (UrlNotExists)
        {
          m_inputUrlListBox->Append(wxURL::ConvertToValidURI(url));
          m_inputAddTextCtrl->SetValue(wxEmptyString);
        }
      else
        {
          wxLogError(_("You have already added this URL !"));
        }
    }
  else
    {
      SetStatusText (_("Please, enter a non empty URL"));
    }
}

/// Remove the selected URL from the URL list box
void
AlcFrame::OnRemoveUrlButton (wxCommandEvent & event)
{
  m_inputUrlListBox->Delete(m_inputUrlListBox->GetSelection());
}

/// Clear the URL list box
void
AlcFrame::OnClearUrlButton (wxCommandEvent & event)
{
  m_inputUrlListBox->Clear();
}
