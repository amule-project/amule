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
#pragma implementation "wxcas.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#if !wxUSE_PRINTING_ARCHITECTURE
#error You must set wxUSE_PRINTING_ARCHITECTURE to 1 in setup.h to compile wxCas.
#endif

#include <wx/image.h>

#include "wxcas.h"

// Application implementation
IMPLEMENT_APP (WxCas);

bool
WxCas::OnInit ()
{
#if wxUSE_LIBPNG
  wxImage::AddHandler (new wxPNGHandler);
#endif

#if wxUSE_LIBJPEG
  wxImage::AddHandler (new wxJPEGHandler);
#endif

#ifdef __WXMSW__
  SetPrintMode (wxPRINT_WINDOWS);
#else
  SetPrintMode (wxPRINT_POSTSCRIPT);
#endif

  m_config = new wxConfig(GetAppName());
  m_config->Get(TRUE);

  m_frame = new WxCasFrame (_("wxCas, aMule Online Statistics"));

  // Show all
  m_frame->Show (TRUE);
  SetTopWindow (m_frame);
  return true;
}

int 
WxCas::OnExit()
{
	m_config->Set(NULL);
	delete m_config;
	return 0;
}

WxCasFrame *
WxCas::GetMainFrame ()
{
  return m_frame;
}

wxConfig *
WxCas::GetConfig ()
{
	return m_config;
}
