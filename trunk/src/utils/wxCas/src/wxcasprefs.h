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

#ifndef _WXCASPREFS_H
#define _WXCASPREFS_H

#ifdef __GNUG__
#pragma interface "wxcasprefs.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

// Preference Dialog
class WxCasPrefs:public wxDialog
{
private:
  wxBoxSizer * m_mainVBox;
  wxStaticBox *m_osPathSBox;
  wxStaticBoxSizer *m_osPathSBoxSizer;
  wxTextCtrl *m_osPathTextCtrl;
  wxButton *m_osPathBrowseButton;

  wxStaticBox *m_refreshSBox;
  wxStaticBoxSizer *m_refreshSBoxSizer;
  wxSpinCtrl *m_refreshSpinButton;
  wxStaticText *m_refreshStaticText;

  wxStaticBox *m_autoStatImgSBox;
  wxStaticBoxSizer *m_autoStatImgSBoxSizer;
  wxRadioButton *m_autoStatImgRadio;
  wxBoxSizer *m_autoStatImgHBoxSizer;
  wxTextCtrl *m_autoStatImgTextCtrl;
  wxButton *m_autoStatImgButton;
  
  wxStaticText *m_noteStaticText;

  wxStaticLine *m_staticLine;

  wxBoxSizer *m_buttonVBox;
  wxButton *m_validateButton;
  wxButton *m_cancelButton;

  enum
  {
    ID_OSPATH_BROWSE_BUTTON = 100,
	ID_AUTOSTATIMG_RADIO,
	ID_AUTOSTATIMG_BROWSE_BUTTON,
    ID_VALIDATE_BUTTON,
    ID_CANCEL_BUTTON
  };

protected:
  void OnOSPathBrowseButton (wxCommandEvent & event);
  void OnValidateButton (wxCommandEvent & event);
  void OnAutoStatImgBrowseButton (wxCommandEvent & event);
  void OnAutoStatImgRadio (wxCommandEvent & event);

    DECLARE_EVENT_TABLE ();
	
public:
    WxCasPrefs (wxWindow * parent);
   ~WxCasPrefs ();

};

#endif /* _WXCASPREFS_H */
