//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCas Class
///
/// Purpose:      wxCas application class
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmaps from aMule http://www.amule.org
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

#ifndef _WXCAS_H
#define _WXCAS_H


#include <wx/app.h>

#include "wxcasframe.h"

/// Application
class WxCas: public wxApp
{
private:
	WxCasFrame * m_frame;

protected:
	wxLocale m_locale; // Used to tell wxCas to use aMule catalog

public:

	/// Application initialisation
	virtual bool OnInit ();

	/// Cleaning on exit
	virtual int OnExit();

	/// Get application main frame
	WxCasFrame *GetMainFrame () const;
};

DECLARE_APP ( WxCas )

#endif /* _WXCAS_H */
// File_checked_for_headers
