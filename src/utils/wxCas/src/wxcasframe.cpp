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
#pragma implementation "wxcasframe.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wxcasframe.h"

#ifdef __WXMSW__
#define USE_XPM_BITMAPS 0
#else
#define USE_XPM_BITMAPS 1
#endif

#if USE_XPM_BITMAPS && defined(__WXMSW__) && !wxUSE_XPM_IN_MSW
#error You need to enable XPM support to use XPM bitmaps with toolbar!
#endif // USE_XPM_BITMAPS

#if USE_XPM_BITMAPS
#include "../pixmaps/wxcas.xpm"
#include "../pixmaps/refresh.xpm"
#include "../pixmaps/stop.xpm"
#include "../pixmaps/save.xpm"
#include "../pixmaps/print.xpm"
#include "../pixmaps/about.xpm"
//#include "../pixmaps/green.xpm"
//#include "../pixmaps/red.xpm"
#endif

// Constructor
WxCasFrame::WxCasFrame (const wxChar * title):
wxFrame ((wxFrame *) NULL, -1, title, wxDefaultPosition, wxDefaultSize,
	 wxDEFAULT_FRAME_STYLE & (wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCAPTION))
{
  // Give it an icon
  SetIcon (wxICON (wxcas));

  m_maxLineCount = 0;

  // Status Bar
  CreateStatusBar ();
  SetStatusText (_("Welcome!"));

  // Frame Vertical Sizer
  m_frameVBox = new wxBoxSizer (wxVERTICAL);

  // Add static line to sizer
  m_staticLine = new wxStaticLine (this, -1);
  m_frameVBox->Add (m_staticLine, 0, wxALL | wxEXPAND);

  // Draw and populate panel
  m_sigPanel = new wxPanel (this, -1, wxDefaultPosition,
			    wxDefaultSize, wxTAB_TRAVERSAL, "wxCasPanel");

  // Panel Vertical Sizer
  m_sigPanelSBox = new wxStaticBox (m_sigPanel, -1, "");
  m_sigPanelSBoxSizer = new wxStaticBoxSizer (m_sigPanelSBox, wxVERTICAL);

  // Statistics labels
  m_statLine_1 = new wxStaticText (m_sigPanel, -1, "");
  m_statLine_2 = new wxStaticText (m_sigPanel, -1, "");
  m_statLine_3 = new wxStaticText (m_sigPanel, -1, "");
  m_statLine_4 = new wxStaticText (m_sigPanel, -1, "");
  m_statLine_5 = new wxStaticText (m_sigPanel, -1, "");
  m_statLine_6 = new wxStaticText (m_sigPanel, -1, "");

  // Add Online Sig file
  aMuleSig = new OnLineSig ();
  SetFromAmuleFile ();

  m_sigPanelSBoxSizer->Add (m_statLine_1, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_2, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_3, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_4, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_5, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_6, 0, wxALL | wxADJUST_MINSIZE, 5);

  // Panel Layout
  m_sigPanel->SetSizerAndFit (m_sigPanelSBoxSizer);

  // Add panel to sizer
  m_frameVBox->Add (m_sigPanel, 0, wxALL | wxADJUST_MINSIZE, 10);

  // Display Bitmap
  m_imgPanel = new WxCasCanvas (this, m_sigPanel);

  // Add Bimap to sizer
  m_frameVBox->Add (m_imgPanel, 0, wxALL | wxADJUST_MINSIZE, 10);

// Toolbar Pixmaps
#if USE_XPM_BITMAPS
  m_toolBarBitmaps[0] = new wxBitmap (refresh_xpm);
  m_toolBarBitmaps[1] = new wxBitmap (save_xpm);
  m_toolBarBitmaps[2] = new wxBitmap (print_xpm);
  m_toolBarBitmaps[3] = new wxBitmap (about_xpm);
  m_toolBarBitmaps[4] = new wxBitmap (stop_xpm);
#else
  m_toolBarBitmaps[0] = new wxBITMAP (refresh);
  m_toolBarBitmaps[1] = new wxBITMAP (save);
  m_toolBarBitmaps[2] = new wxBITMAP (print);
  m_toolBarBitmaps[3] = new wxBITMAP (about);
  m_toolBarBitmaps[4] = new wxBITMAP (stop);
#endif

  // Constructing toolbar
  m_toolbar =
    new wxToolBar (this, -1, wxDefaultPosition, wxDefaultSize,
		   wxTB_HORIZONTAL | wxTB_FLAT);
  SetToolBar (m_toolbar);

  m_toolbar->AddTool (ID_BAR_REFRESH, "Refresh", *(m_toolBarBitmaps[0]),
		      _("Stop Auto Refresh"));

  m_toolbar->AddSeparator ();

  m_toolbar->AddTool (ID_BAR_SAVE, "Save", *(m_toolBarBitmaps[1]),
		      _
		      ("Save Online Statistics image (NOT implemented Yet)"));

  m_toolbar->AddTool (ID_BAR_PRINT, "Print", *(m_toolBarBitmaps[2]),
		      _
		      ("Print Online Statistics image (NOT implemented Yet)"));

  m_toolbar->AddSeparator ();

  m_toolbar->AddTool (ID_BAR_ABOUT, "About", *(m_toolBarBitmaps[3]),
		      _("About wxCas"));

  m_toolbar->SetMargins (2, 2);
  m_toolbar->Realize ();

  // Frame Layout
  SetAutoLayout (TRUE);
  SetSizerAndFit (m_frameVBox);

  // Add timer
  m_timer = new wxTimer (this, ID_TIMER);
  m_timer->Start (5000);
}

// Destructor
WxCasFrame::~WxCasFrame ()
{
  delete aMuleSig;
}

// Events table
BEGIN_EVENT_TABLE (WxCasFrame, wxFrame)
EVT_TOOL (ID_BAR_REFRESH, WxCasFrame::OnBarRefresh)
EVT_TOOL (ID_BAR_SAVE, WxCasFrame::OnBarSave)
EVT_TOOL (ID_BAR_PRINT, WxCasFrame::OnBarPrint)
EVT_TOOL (ID_BAR_ABOUT, WxCasFrame::OnBarAbout)
EVT_TIMER (ID_TIMER, WxCasFrame::OnTimer) 
END_EVENT_TABLE ()

// Refresh button
void
WxCasFrame::OnBarRefresh (wxCommandEvent & event)
{
  if (m_timer->IsRunning ())
    {
      m_timer->Stop ();
      m_toolbar->DeleteTool (ID_BAR_REFRESH);
      m_toolbar->InsertTool (0, ID_BAR_REFRESH, "Refresh",
			     *(m_toolBarBitmaps[4]), wxNullBitmap,
			     wxITEM_NORMAL, _("Start Auto Refresh"));
      SetStatusText (_("Auto Refresh stopped"));

    }
  else
    {
      m_timer->Start ();
      m_toolbar->DeleteTool (ID_BAR_REFRESH);
      m_toolbar->InsertTool (0, ID_BAR_REFRESH, "Refresh",
			     *(m_toolBarBitmaps[0]), wxNullBitmap,
			     wxITEM_NORMAL, _("Stop Auto Refresh"));
      SetStatusText (_("Auto Refresh started"));
    }
}

// Save button
void
WxCasFrame::OnBarSave (wxCommandEvent & event)
{
  printf ("To be implemented\n");
}

// Print button
void
WxCasFrame::OnBarPrint (wxCommandEvent & event)
{
  printf ("To be implemented\n");
}

// About button
void
WxCasFrame::OnBarAbout (wxCommandEvent & event)
{
  wxMessageBox (_
		("wxCas, aMule OnLigne Signature Statistics\n\n"
		 "(c) 2004 ThePolish <thepolish@vipmail.ru>\n\n"
		 "Based on CAS by Pedro de Oliveira <falso@rdk.homeip.net>\n\n"
		 "Distributed under GPL"),
		_("About wxCas"), wxOK | wxCENTRE | wxICON_INFORMATION);
}

// Timer
void
WxCasFrame::OnTimer (wxTimerEvent & event)
{
  SetFromAmuleFile ();
  m_imgPanel->Update ();
}

// Accessors
void
WxCasFrame::SetFromAmuleFile ()
{
  // Set labels
  aMuleSig->Refresh ();

  Freeze ();

  wxString newline;

  newline = "aMule ";
  newline += aMuleSig->GetVersion ();
  newline += _(" has been running for ");
  newline += aMuleSig->GetRunTime ();

  wxUint32 newMaxLineCount = newline.Length ();

  m_statLine_1->SetLabel (newline);

  newline = aMuleSig->GetUser ();
  newline += _(" is on ");
  newline += aMuleSig->GetServerName ();
  newline += " [";
  newline += aMuleSig->GetServerIP ();
  newline += ":";
  newline += aMuleSig->GetServerPort ();
  newline += _("] with ");
  newline += aMuleSig->GetConnexionIDType ();;

  m_statLine_2->SetLabel (newline);

#ifdef __GNUG__
  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else
  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  newline = _("Total Download: ");
  newline += aMuleSig->GetConvertedTotalDL ();
  newline += _(", Upload: ");
  newline += aMuleSig->GetConvertedTotalUL ();

  m_statLine_3->SetLabel (newline);

#ifdef __GNUG__
  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else
  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  newline = _("Session Download: ");
  newline += aMuleSig->GetConvertedSessionDL ();
  newline += _(", Upload: ");
  newline += aMuleSig->GetConvertedSessionUL ();

  m_statLine_4->SetLabel (newline);

#ifdef __GNUG__
  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else
  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  newline = _("Download: ");
  newline += aMuleSig->GetDLRate ();
  newline += _(" kB/s, Upload: ");
  newline += aMuleSig->GetULRate ();
  newline += _("kB/s");

  m_statLine_5->SetLabel (newline);

#ifdef __GNUG__
  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else
  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  newline = _("Sharing: ");
  newline += aMuleSig->GetSharedFiles ();
  newline += _(" file(s), Clients on queue: ");
  newline += aMuleSig->GetQueue ();

  m_statLine_6->SetLabel (newline);

#ifdef __GNUG__
  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else
  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  // Set status bar
  if (aMuleSig->IsRunning ())
    {
      SetStatusText (_("aMule is running"));
    }
  else
    {
      SetStatusText (_("WARNING: aMule is NOT running"));
    }

  Thaw ();

  // Resize only if needed
  if (m_maxLineCount != newMaxLineCount)
    {
      // Fit to new lable size
      Layout ();
      Fit ();
      m_maxLineCount = newMaxLineCount;
    }
}
