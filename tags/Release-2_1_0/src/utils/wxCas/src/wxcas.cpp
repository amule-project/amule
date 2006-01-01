//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCas Class
///
/// Purpose:      wxCas application class
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

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for PACKAGE
#endif

#if !wxUSE_PRINTING_ARCHITECTURE
#error You must set wxUSE_PRINTING_ARCHITECTURE to 1 in setup.h to compile wxCas.
#endif

#include <wx/image.h>
#include <wx/config.h>

#include "wxcas.h"

// Application implementation
IMPLEMENT_APP ( WxCas )

bool
WxCas::OnInit ()
{
	// Used to tell wxCas to use aMule catalog
	m_locale.Init();
	m_locale.AddCatalog( wxT( PACKAGE ) );

#if wxUSE_LIBPNG

	wxImage::AddHandler ( new wxPNGHandler );
#endif

#if wxUSE_LIBJPEG

	wxImage::AddHandler ( new wxJPEGHandler );
#endif

#ifdef __WXMSW__

	SetPrintMode ( wxPRINT_WINDOWS );
#else

	SetPrintMode ( wxPRINT_POSTSCRIPT );
#endif

	// Prefs
	wxConfigBase::Get();

	// Main Frame
	m_frame = new WxCasFrame ( _( "wxCas, aMule Online Statistics" ) );

	// Show all
	m_frame->Show ( TRUE );
	SetTopWindow ( m_frame );
	return true;
}

int
WxCas::OnExit()
{
	delete wxConfigBase::Set( ( wxConfigBase * ) NULL );
	return 0;
}

WxCasFrame *
WxCas::GetMainFrame () const
{
	return m_frame;
}

