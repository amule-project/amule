//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasFrame Class
///
/// Purpose:      wxCas main frame
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmats from aMule http://www.amule.org
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
#pragma implementation "wxcasframe.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/image.h>
#include <wx/version.h>
#include <wx/config.h>
#include <wx/protocol/ftp.h>
#include <wx/wfstream.h>

#include "wxcasframe.h"
#include "wxcasprint.h"
#include "wxcasprefs.h"
#include "wxcascte.h"

#ifndef __WXMSW__
#include "../pixmaps/wxcas.xpm"
#include "../pixmaps/refresh.xpm"
#include "../pixmaps/stop.xpm"
#include "../pixmaps/save.xpm"
#include "../pixmaps/print.xpm"
#include "../pixmaps/about.xpm"
#include "../pixmaps/prefs.xpm"
#include "../pixmaps/stat.xpm"  // Stat banner
#include "../pixmaps/amule.xpm" // aMule splash
#endif

// Needed for 2.4.2 backward compatibility
#if (wxMINOR_VERSION < 5)
#define wxCLOSE_BOX 0
#endif

// Constructor
WxCasFrame::WxCasFrame (const wxString & title):
    wxFrame ((wxFrame *) NULL, -1, title, wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_FRAME_STYLE & (wxSYSTEM_MENU | wxMINIMIZE_BOX | wxCAPTION | wxCLOSE_BOX ))
{
  // Give it an icon
  SetIcon (wxICON (wxcas));

  // Prefs
  wxConfigBase * prefs = wxConfigBase::Get();

  m_maxLineCount = 0;

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

  // Statistics Static Vertical Box Sizer
  m_sigPanelSBox = new wxStaticBox (m_mainPanel, -1, wxEmptyString);
  m_sigPanelSBoxSizer = new wxStaticBoxSizer (m_sigPanelSBox, wxVERTICAL);

  // Statistic labels
  m_statLine_1 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_2 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_3 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_4 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_5 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_6 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_statLine_7 = new wxStaticText (m_mainPanel, -1, wxEmptyString);

#ifdef __LINUX__		// System monitoring on Linux

  m_sysLine_1 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
  m_sysLine_2 = new wxStaticText (m_mainPanel, -1, wxEmptyString);
#endif

  // Add Online Sig file
  m_aMuleSig = new OnLineSig ();

#ifdef __LINUX__		// System monitoring on Linux

  m_sysMonitor = new LinuxMon ();
#endif

  // Amule Static Bitmap
  wxBitmap amule_bitmap = wxBITMAP (amule);
  m_amuleSBitmap = new wxStaticBitmap (m_mainPanel,-1,amule_bitmap);

  // Toolbar Pixmaps
  m_toolBarBitmaps[0] = wxBITMAP (refresh);
  m_toolBarBitmaps[1] = wxBITMAP (save);
  m_toolBarBitmaps[2] = wxBITMAP (print);
  m_toolBarBitmaps[3] = wxBITMAP (about);
  m_toolBarBitmaps[4] = wxBITMAP (stop);
  m_toolBarBitmaps[5] = wxBITMAP (prefs);

  // Constructing toolbar
  m_toolbar =
    new wxToolBar (this, -1, wxDefaultPosition, wxDefaultSize,
                   wxTB_HORIZONTAL | wxTB_FLAT);

  m_toolbar->SetToolBitmapSize (wxSize (32, 32));

  m_toolbar->AddTool (ID_BAR_REFRESH, wxT("Refresh"), m_toolBarBitmaps[0],
                      _("Stop Auto Refresh"));

  m_toolbar->AddSeparator ();

  m_toolbar->AddTool (ID_BAR_SAVE, wxT("Save"), m_toolBarBitmaps[1],
                      _("Save Online Statistics image"));

  m_toolbar->AddTool (ID_BAR_PRINT, wxT("Print"), m_toolBarBitmaps[2],
                      _("Print Online Statistics image"));

  m_toolbar->AddTool (ID_BAR_PREFS, wxT("Prefs"), m_toolBarBitmaps[5],
                      _("Preferences setting"));

  m_toolbar->AddSeparator ();

  m_toolbar->AddTool (ID_BAR_ABOUT, wxT("About"), m_toolBarBitmaps[3],
                      _("About wxCas"));

  m_toolbar->SetMargins (2, 2);
  m_toolbar->Realize ();

  SetToolBar (m_toolbar);

  // Statistic Panel Layout
  m_sigPanelSBoxSizer->Add (m_statLine_1, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_2, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_3, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_4, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_5, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_6, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_statLine_7, 0, wxALL | wxADJUST_MINSIZE, 5);

#ifdef __LINUX__		// System monitoring on Linux

  m_sigPanelSBoxSizer->Add (m_sysLine_1, 0, wxALL | wxADJUST_MINSIZE, 5);
  m_sigPanelSBoxSizer->Add (m_sysLine_2, 0, wxALL | wxADJUST_MINSIZE, 5);
#endif

  // Main panel Layout
  m_mainPanelVBox->Add (m_staticLine, 0, wxALL | wxGROW);
  m_mainPanelVBox->Add (m_sigPanelSBoxSizer, 0, wxALL | wxADJUST_MINSIZE, 10);
  m_mainPanelVBox->Add (m_amuleSBitmap, 0, wxALL | wxADJUST_MINSIZE, 10);
  m_mainPanel->SetAutoLayout(true);
  m_mainPanel->SetSizer (m_mainPanelVBox);
  m_mainPanelVBox->Fit(m_mainPanel);
  m_mainPanelVBox->SetSizeHints(m_mainPanel);

  // Frame Layout
  m_frameVBox->Add (m_mainPanel, 1, wxALL | wxADJUST_MINSIZE);
  SetAutoLayout (TRUE);
  SetSizer(m_frameVBox);
  m_frameVBox->Fit(this);
  m_frameVBox->SetSizeHints(this);

  // Add refresh timer
  m_refresh_timer = new wxTimer (this, ID_REFRESH_TIMER);
  m_refresh_timer->Start (1000 * prefs->Read (WxCasCte::REFRESH_RATE_KEY, WxCasCte::DEFAULT_REFRESH_RATE));	// s to ms

  // Add FTP update timer
  m_ftp_update_timer = new wxTimer (this, ID_FTP_UPDATE_TIMER);
  m_ftp_update_timer->Start (60000 * prefs->Read (WxCasCte::FTP_UPDATE_RATE_KEY, WxCasCte::DEFAULT_FTP_UPDATE_RATE));	// min to ms

  // Update stats
  UpdateAll ();
}

// Destructor
WxCasFrame::~WxCasFrame ()
{
  delete m_aMuleSig;

#ifdef __LINUX__		// System monitoring on Linux

  delete m_sysMonitor;
#endif
}

// Events table
BEGIN_EVENT_TABLE (WxCasFrame, wxFrame)
EVT_TOOL (ID_BAR_REFRESH, WxCasFrame::OnBarRefresh)
EVT_TOOL (ID_BAR_SAVE, WxCasFrame::OnBarSave)
EVT_TOOL (ID_BAR_PRINT, WxCasFrame::OnBarPrint)
EVT_TOOL (ID_BAR_PREFS, WxCasFrame::OnBarPrefs)
EVT_TOOL (ID_BAR_ABOUT, WxCasFrame::OnBarAbout)
EVT_TIMER (ID_REFRESH_TIMER, WxCasFrame::OnRefreshTimer)
EVT_TIMER (ID_FTP_UPDATE_TIMER, WxCasFrame::OnFtpUpdateTimer)
END_EVENT_TABLE ()

// Get Stat Bitmap
wxImage *
WxCasFrame::GetStatImage () const
  {
    wxBitmap
    statBitmap = wxBITMAP (stat);

    wxMemoryDC memdc;
    memdc.SelectObject (statBitmap);

#ifdef __WXMSW__

    memdc.
    SetFont (wxFont::wxFont (10, wxSWISS, wxNORMAL, wxBOLD));
#else

    memdc.
    SetFont (wxFont::wxFont (12, wxSWISS, wxNORMAL, wxBOLD));
#endif

    memdc.
    SetTextForeground (*wxWHITE);
    memdc.
    DrawText (m_statLine_1->GetLabel (), 25, 8);
    memdc.
    DrawText (m_statLine_2->GetLabel (), 25, 26);
    memdc.
    DrawText (m_statLine_3->GetLabel (), 25, 43);
    memdc.
    DrawText (m_statLine_4->GetLabel (), 25, 60);
    memdc.
    DrawText (m_statLine_5->GetLabel (), 25, 77);
    memdc.
    DrawText (m_statLine_6->GetLabel (), 25, 94);
    memdc.
    SelectObject (wxNullBitmap);

    wxImage *
    statImage = new wxImage (statBitmap.ConvertToImage());

    return
      (statImage);
  }

// Refresh button
void
WxCasFrame::OnBarRefresh (wxCommandEvent & event)
{
  if (m_refresh_timer->IsRunning ())
    {
      m_refresh_timer->Stop ();
      m_ftp_update_timer->Stop ();
      m_toolbar->DeleteTool (ID_BAR_REFRESH);
      m_toolbar->InsertTool (0, ID_BAR_REFRESH, wxT("Refresh"),
                             m_toolBarBitmaps[4], wxNullBitmap,
                             wxITEM_NORMAL, _("Start Auto Refresh"));
      m_toolbar->Realize ();
      SetStatusText (_("Auto Refresh stopped"));
    }
  else
    {
      m_refresh_timer->Start ();
      m_ftp_update_timer->Start ();
      m_toolbar->DeleteTool (ID_BAR_REFRESH);
      m_toolbar->InsertTool (0, ID_BAR_REFRESH, wxT("Refresh"),
                             m_toolBarBitmaps[0], wxNullBitmap,
                             wxITEM_NORMAL, _("Stop Auto Refresh"));
      m_toolbar->Realize ();
      SetStatusText (_("Auto Refresh started"));
    }
}

// Save button
void
WxCasFrame::OnBarSave (wxCommandEvent & event)
{
  wxImage *statImage = GetStatImage ();

  wxString saveFileName = wxFileSelector (_("Save Statistics Image"),
                                          wxFileName::GetHomeDir (),
                                          WxCasCte::AMULESIG_IMG_NAME,
                                          (const wxChar *) NULL,
                                          wxT ("PNG files (*.png)|*.png|")
                                          wxT ("JPEG files (*.jpg)|*.jpg|")
                                          wxT ("BMP files (*.bmp)|*.bmp|"),
                                          wxSAVE);

  if (!saveFileName.empty ())
    {
      // This one guesses image format from filename extension
      // (it may fail if the extension is not recognized):

      if (!statImage->SaveFile (saveFileName))
        {
          wxMessageBox (_("No handler for this file type."),
                        _("File was not saved"), wxOK | wxCENTRE, this);
        }
    }
  delete statImage;
}

// Print button
void
WxCasFrame::OnBarPrint (wxCommandEvent & event)
{
  wxPrinter printer;
  WxCasPrint printout (_("aMule Online Statistics"));
  if (!printer.Print (this, &printout, TRUE))
    {
      if (wxPrinter::GetLastError () == wxPRINTER_ERROR)
        {
          wxMessageBox (_
                        ("There was a problem printing.\nPerhaps your current printer is not set correctly?"),
                        _("Printing"), wxOK);
        }
    }
}

// Prefs button
void
WxCasFrame::OnBarPrefs (wxCommandEvent & event)
{
  WxCasPrefs dlg (this);
  dlg.ShowModal ();
}

// About button
void
WxCasFrame::OnBarAbout (wxCommandEvent & event)
{
  wxMessageBox (_
                ("wxCas, aMule OnLine Signature Statistics\n\n"
                 "(c) 2004 ThePolish <thepolish@vipmail.ru>\n\n"
                 "Based on CAS by Pedro de Oliveira <falso@rdk.homeip.net>\n\n"
                 "Distributed under GPL"),
                _("About wxCas"), wxOK | wxCENTRE | wxICON_INFORMATION);
}

// Refresh timer
void
WxCasFrame::OnRefreshTimer (wxTimerEvent & event)
{
  // Prefs
  wxConfigBase * prefs = wxConfigBase::Get();

  UpdateAll ();

  // Generate stat image if asked in config
  if ((bool)
      (prefs->
       Read (WxCasCte::ENABLE_AUTOSTATIMG_KEY,
             WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED)))
    {
      wxImage *statImage = GetStatImage ();

      wxFileName fileName (prefs->
                           Read (WxCasCte::AUTOSTATIMG_DIR_KEY,
                                 WxCasCte::DEFAULT_AUTOSTATIMG_PATH),
                           WxCasCte::AMULESIG_IMG_NAME,
                           prefs->
                           Read (WxCasCte::AUTOSTATIMG_TYPE_KEY,
                                 WxCasCte::DEFAULT_AUTOSTATIMG_TYPE).
                           Lower ());

      if (!statImage->SaveFile (fileName.GetFullPath ()))
        {
          wxLogError (wxT("No handler for this file type. File was not saved"));
        }
      delete statImage;
    }
}

// Ftp update timer
void
WxCasFrame::OnFtpUpdateTimer (wxTimerEvent & event)
{
  // Prefs
  wxConfigBase * prefs = wxConfigBase::Get();

  // Image must be autogenerated to be uploaded
  if ((bool)(prefs->Read (WxCasCte::ENABLE_AUTOSTATIMG_KEY,
                          WxCasCte::DEFAULT_AUTOSTATIMG_ISENABLED)) &&
      (bool)(prefs->Read (WxCasCte::ENABLE_FTP_UPDATE_KEY,
                          WxCasCte::DEFAULT_FTP_UPDATE_ISENABLED)))
    {
      // Get image file
      wxFileName fileName (prefs->
                           Read (WxCasCte::AUTOSTATIMG_DIR_KEY,
                                 WxCasCte::DEFAULT_AUTOSTATIMG_PATH),
                           WxCasCte::AMULESIG_IMG_NAME,
                           prefs->
                           Read (WxCasCte::AUTOSTATIMG_TYPE_KEY,
                                 WxCasCte::DEFAULT_AUTOSTATIMG_TYPE).
                           Lower ());

      // If img doenst exist, return
      if (! fileName.FileExists(fileName.GetFullPath ()))
        {
          wxLogError(wxT("Image file ") + fileName.GetFullPath () + wxT(" doesn't exist"));
          return;
        }

      // Connect to ftp
      wxFTP ftp;

      ftp.SetUser(prefs->Read (WxCasCte::FTP_USER_KEY,
                               WxCasCte::DEFAULT_FTP_USER));
      ftp.SetPassword(prefs->Read (WxCasCte::FTP_PASSWD_KEY,
                                   WxCasCte::DEFAULT_FTP_PASSWD));

      if (! ftp.Connect(prefs->Read (WxCasCte::FTP_URL_KEY,
                                     WxCasCte::DEFAULT_FTP_URL)) )
        {
          wxLogError(wxT("Cannot connect to FTP server ")+prefs->Read (WxCasCte::FTP_URL_KEY,
                     WxCasCte::DEFAULT_FTP_URL));
          return;
        }

      // Chdir
      if (! ftp.ChDir(prefs->Read (WxCasCte::FTP_PATH_KEY,
                                   WxCasCte::DEFAULT_FTP_PATH)))
        {
          wxLogError(wxT("Cannot chdir to ")+prefs->Read (WxCasCte::FTP_PATH_KEY,
                     WxCasCte::DEFAULT_FTP_PATH));
          ftp.Close();
          return;
        }

      // Upload image
      ftp.SetBinary();
      wxFileInputStream in(fileName.GetFullPath ());
      if (in.Ok())
        {
          wxOutputStream *out = ftp.GetOutputStream(fileName.GetFullName ());
          if ( out )
            {
              out->Write(in);
              delete out;
            }
          else
            {
              wxLogError(wxT("Cannot open FTP upload stream"));
            }
        }
      else
        {
          wxLogError(wxT("Cannot open file stream to read image file"));
        }

      // Close connexion
      ftp.Close();
    }
}

// Update all panels and frame, call Fit if needed
void
WxCasFrame::UpdateAll ()
{
  bool needFit = UpdateStatsPanel ();

  if (needFit)
    {
      m_mainPanel->SetAutoLayout(true);
      m_mainPanelVBox->Fit(m_mainPanel);

      wxInt32 ph, pw;
      m_sigPanelSBox->GetClientSize (&pw, &ph);
      AdjustSplashWidth(pw);

      m_mainPanelVBox->SetSizeHints(m_mainPanel);

      // Frame Layout
      SetAutoLayout (TRUE);
      m_frameVBox->Fit(this);
      m_frameVBox->SetSizeHints(this);
    }
}

// Update stat panel
bool
WxCasFrame::UpdateStatsPanel ()
{
  // Set labels
  m_aMuleSig->Refresh ();

#ifdef __LINUX__		// System monitoring on Linux

  m_sysMonitor->Refresh ();
#endif

  wxString newline;

  Freeze ();

  // Stat line 1
  newline = _("aMule ");
  newline += m_aMuleSig->GetVersion ();
  newline += _(" has been running for ");
  newline += m_aMuleSig->GetRunTime ();

  wxUint32 newMaxLineCount = newline.Length ();

  m_statLine_1->SetLabel (newline);

  // Stat line 2
  newline = m_aMuleSig->GetUser ();
  newline += _(" is on ");
  newline += m_aMuleSig->GetServerName ();
  newline += _(" [");
  newline += m_aMuleSig->GetServerIP ();
  newline += _(":");
  newline += m_aMuleSig->GetServerPort ();
  newline += _("] with ");
  newline += m_aMuleSig->GetConnexionIDType ();

  m_statLine_2->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  // Stat line 3
  newline = _("Total Download: ");
  newline += m_aMuleSig->GetConvertedTotalDL ();
  newline += _(", Upload: ");
  newline += m_aMuleSig->GetConvertedTotalUL ();

  m_statLine_3->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  // Stat line 4
  newline = _("Session Download: ");
  newline += m_aMuleSig->GetConvertedSessionDL ();
  newline += _(", Upload: ");
  newline += m_aMuleSig->GetConvertedSessionUL ();

  m_statLine_4->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  // Stat line 5
  newline = _("Download: ");
  newline += m_aMuleSig->GetDLRate ();
  newline += _(" kB/s, Upload: ");
  newline += m_aMuleSig->GetULRate ();
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

  // Stat line 6
  newline = _("Sharing: ");
  newline += m_aMuleSig->GetSharedFiles ();
  newline += _(" file(s), Clients on queue: ");
  newline += m_aMuleSig->GetQueue ();

  m_statLine_6->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

  // Stat line 7
  newline = _("Maximum DL rate since wxCas is running: ");
  newline += m_aMuleSig->GetMaxDL ();

  m_statLine_7->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif

#ifdef __LINUX__		// System monitoring on Linux
  newline = _("System Load Average (1-5-15 min): ");
  newline +=
    newline.Format (wxT("%.2f - %.2f - %.2f"), m_sysMonitor->GetSysLoad_1 (),
                    m_sysMonitor->GetSysLoad_5 (),
                    m_sysMonitor->GetSysLoad_15 ());

  m_sysLine_1->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif
#endif

#ifdef __LINUX__		// System monitoring on Linux
  newline = _("System uptime: ");
  newline += m_sysMonitor->GetUptime ();

  m_sysLine_2->SetLabel (newline);

#ifdef __GNUG__

  newMaxLineCount = newMaxLineCount >? newline.Length ();
#else

  if (newline.Length () > newMaxLineCount)
    {
      newMaxLineCount = newline.Length ();
    }
#endif
#endif

  Thaw ();

  // Set status bar
  if (m_aMuleSig->IsRunning ())
    {
      SetStatusText (_("aMule is running"));
    }
  else
    {
      SetStatusText (_("WARNING: aMule is NOT running"));
    }

  // Resize only if needed
  if (m_maxLineCount != newMaxLineCount)
    {
      m_maxLineCount = newMaxLineCount;
      return (TRUE);
    }
  else
    {
      return (FALSE);
    }
}

// Refresh period changing
bool
WxCasFrame::ChangeRefreshPeriod(wxInt32 newPeriod)
{
  // As the user can stop it, we must let it in the same state
  // it was before changing period
  bool wasRunning=FALSE;

  if (m_refresh_timer->IsRunning())
    {
      wasRunning=TRUE;
    }

  bool ok=m_refresh_timer->Start(newPeriod);

  if (! wasRunning)
    {
      m_refresh_timer->Stop();
    }

  return (ok);
}

// Ftp update period changing
bool
WxCasFrame::ChangeFtpUpdatePeriod(wxInt32 newPeriod)
{
  // As the user can stop it, we must let it in the same state
  // it was before changing period
  bool wasRunning=FALSE;

  if (m_ftp_update_timer->IsRunning())
    {
      wasRunning=TRUE;
    }

  bool ok=m_ftp_update_timer->Start(newPeriod);

  if (! wasRunning)
    {
      m_ftp_update_timer->Stop();
    }

  return (ok);
}

// Adjust splash bitmap width
void
WxCasFrame::AdjustSplashWidth(wxInt32 width)
{
  wxInt32 h =
    (wxInt32) ((double) (m_amuleSBitmap->GetBitmap().GetHeight ()) / m_amuleSBitmap->GetBitmap().GetWidth () *
               width);

  wxBitmap bitmap (wxImage (m_amuleSBitmap->GetBitmap().ConvertToImage ()).
                   Scale (width, h));

  m_amuleSBitmap->SetBitmap(bitmap);
}

// Set amulesig.dat file
void
WxCasFrame::SetAmuleSigFile(wxFileName *file)
{
  m_aMuleSig->SetAmuleSig (file);
}
