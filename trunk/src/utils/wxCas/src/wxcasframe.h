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

#ifndef _WXCASFRAME_H
#define _WXCASFRAME_H

#ifdef __GNUG__
#pragma interface "wxcasframe.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statline.h>
#include <wx/toolbar.h>
#include <wx/timer.h>

#include "wxcascanvas.h"
#include "onlinesig.h"


#ifdef __LINUX__		// System monitoring on Linux
#include "linuxmon.h"
#endif

// wxCas Frame
class WxCasFrame:public wxFrame
  {
  private:
    wxToolBar *m_toolbar;
    wxBitmap m_toolBarBitmaps[6];

    wxBoxSizer *m_frameVBox;
    wxBoxSizer *m_mainPanelVBox;
    wxBoxSizer *m_sigPanelVBox;

    wxPanel *m_mainPanel;
    wxPanel *m_sigPanel;

    wxStaticLine *m_staticLine;

    wxStaticBox *m_sigPanelSBox;
    wxStaticBoxSizer *m_sigPanelSBoxSizer;

    wxStaticText *m_statLine_1;
    wxStaticText *m_statLine_2;
    wxStaticText *m_statLine_3;
    wxStaticText *m_statLine_4;
    wxStaticText *m_statLine_5;
    wxStaticText *m_statLine_6;
    wxStaticText *m_statLine_7;

    wxTimer * m_timer;
    WxCasCanvas *m_imgPanel;
    OnLineSig *m_aMuleSig;
    wxUint32 m_maxLineCount;

#ifdef __LINUX__		// System monitoring on Linux

    wxStaticText *m_sysLine_1;
    wxStaticText *m_sysLine_2;
    LinuxMon *m_sysMonitor;
#endif

    enum
    {
      ID_BAR_REFRESH = 1000,
      ID_BAR_SAVE,
      ID_BAR_PRINT,
      ID_BAR_PREFS,
      ID_BAR_ABOUT,
      ID_TIMER
    };

  protected:
    void UpdateStatsPanel ();

    void OnBarRefresh (wxCommandEvent & event);
    void OnBarAbout (wxCommandEvent & event);
    void OnBarSave (wxCommandEvent & event);
    void OnBarPrint (wxCommandEvent & event);
    void OnBarPrefs (wxCommandEvent & event);
    void OnTimer (wxTimerEvent & event);

    DECLARE_EVENT_TABLE ();

  public:
    // Constructor
    WxCasFrame (const wxString& title);

    // Destructor
    ~WxCasFrame ();

    // Accessor
    wxImage *GetStatImage () const;
  };

#endif /* _WXCASFRAME_H */
