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
#pragma implementation "wxcascte.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include <wx/filename.h>

#include "wxcascte.h"

const wxString
WxCasCte::AMULESIG_NAME ("amulesig.dat");

// Refresh rate limits
const wxUint32
  WxCasCte::MIN_REFRESH_RATE = 1;
const
  wxUint32
  WxCasCte::MAX_REFRESH_RATE = 3600;

// Key config names
const
  wxString
WxCasCte::AMULESIG_KEY ("OSDirectory");
const wxString
WxCasCte::REFRESH_RATE_KEY ("RefreshRate");

// Default config parameters
const wxString
WxCasCte::AMULESIG_DEFAULT_PATH (wxFileName::GetHomeDir () +
				 wxFileName::GetPathSeparator () + ".aMule");
const wxUint32
  WxCasCte::DEFAULT_REFRESH_RATE = 5;
