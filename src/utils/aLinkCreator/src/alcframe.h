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

#ifndef _ALCFRAME_H
#define _ALCFRAME_H

#ifdef __GNUG__
#pragma interface "alcframe.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/statline.h>

/// Main Alc Frame
class AlcFrame:public wxFrame
  {
  private:
    wxToolBar *m_toolbar;
    wxBitmap m_toolBarBitmaps[3];

    wxBoxSizer *m_frameVBox;

    wxPanel *m_mainPanel;
    wxBoxSizer *m_mainPanelVBox;

    wxStaticLine *m_staticLine;

    wxStaticBox *m_md4HashSBox;
    wxStaticBoxSizer* m_md4HashSBoxSizer;
    wxTextCtrl *m_md4HashTextCtrl;

    wxStaticBox *m_hashSBox;
    wxStaticBoxSizer* m_hashSBoxSizer;
    wxTextCtrl *m_hashTextCtrl;

    wxStaticBox *m_ed2kSBox;
    wxStaticBoxSizer* m_ed2kSBoxSizer;
    wxTextCtrl *m_ed2kTextCtrl;

    enum
    {
      ID_BAR_OPEN = 1000,
      ID_BAR_SAVEAS,
      ID_BAR_ABOUT,
    };

  protected:

    void OnBarOpen (wxCommandEvent & event);
    void OnBarSaveAs (wxCommandEvent & event);
    void OnBarAbout (wxCommandEvent & event);

    DECLARE_EVENT_TABLE ();

  public:
    /// Constructor
    AlcFrame (const wxString& title);

    /// Destructor
    ~AlcFrame ();
  };


#endif /* _ALCFRAME_H */
