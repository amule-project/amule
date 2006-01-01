//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasPrefs Class
///
/// Purpose:      Display user preferences dialog and manage configuration storage system
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
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _WXCASPREFS_H
#define _WXCASPREFS_H

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

/// Preference Dialog
class WxCasPrefs: public wxDialog
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
	wxCheckBox *m_autoStatImgCheck;
	wxBoxSizer *m_autoStatImgHBoxSizer;
	wxTextCtrl *m_autoStatImgTextCtrl;
	wxButton *m_autoStatImgButton;
	wxComboBox *m_autoStatImgCombo;

	wxStaticBox *m_ftpUpdateSBox;
	wxStaticBoxSizer *m_ftpUpdateSBoxSizer;
	wxSpinCtrl *m_ftpUpdateSpinButton;
	wxStaticText *m_ftpUpdateStaticText;
	wxBoxSizer *m_ftpRateHBoxSizer;
	wxCheckBox *m_ftpUpdateCheck;
	wxGridSizer *m_ftpUpdateGridSizer;
	wxTextCtrl *m_ftpUrlTextCtrl;
	wxStaticText *m_ftpUrlStaticText;
	wxTextCtrl *m_ftpPathTextCtrl;
	wxStaticText *m_ftpPathStaticText;
	wxTextCtrl *m_ftpUserTextCtrl;
	wxStaticText *m_ftpUserStaticText;
	wxTextCtrl *m_ftpPasswdTextCtrl;
	wxStaticText *m_ftpPasswdStaticText;

	wxStaticLine *m_staticLine;

	wxBoxSizer *m_buttonHBox;
	wxButton *m_validateButton;
	wxButton *m_cancelButton;

	void EnableAutoStatImgCtrls( bool state );
	void EnableFtpUpdateCtrls( bool state );

	enum
	{
	    ID_OSPATH_BROWSE_BUTTON = 100,
	    ID_AUTOSTATIMG_CHECK,
	    ID_AUTOSTATIMG_COMBO,
	    ID_AUTOSTATIMG_BROWSE_BUTTON,
	    ID_FTP_UPDATE_CHECK,
	    ID_VALIDATE_BUTTON,
	    ID_CANCEL_BUTTON
	};

protected:
	void OnOSPathBrowseButton ( wxCommandEvent & event );
	void OnValidateButton ( wxCommandEvent & event );
	void OnAutoStatImgBrowseButton ( wxCommandEvent & event );
	void OnAutoStatImgCheck ( wxCommandEvent & event );
	void OnFtpUpdateCheck ( wxCommandEvent & event );

	DECLARE_EVENT_TABLE ()

public:

	/// Constructor
	WxCasPrefs ( wxWindow * parent );

	/// Destructor
	~WxCasPrefs ();
};

#endif /* _WXCASPREFS_H */
