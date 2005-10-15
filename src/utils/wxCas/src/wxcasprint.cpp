//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         wxCasPrint Class
///
/// Purpose:      Manage statistics image printing
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

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#include <wx/image.h>

#include "wxcas.h"
#include "wxcasframe.h"
#include "wxcasprint.h"

// Constructor
WxCasPrint::WxCasPrint ( const wxString& title ) : wxPrintout ( title )
{}

// Destructor
WxCasPrint::~WxCasPrint ()
{}

bool
WxCasPrint::OnPrintPage ( int page )
{
	wxDC * dc = GetDC ();
	if ( dc ) {
		if ( page == 1 ) {
			DrawPageOne ( dc );
		}


		dc->SetDeviceOrigin ( 0, 0 );
		dc->SetUserScale ( 1.0, 1.0 );

		return TRUE;
	} else {
		return FALSE;
	}
}

bool
WxCasPrint::OnBeginDocument ( int startPage, int endPage )
{
	if ( !wxPrintout::OnBeginDocument ( startPage, endPage ) ) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void
WxCasPrint::GetPageInfo ( int *minPage, int *maxPage, int *selPageFrom,
                          int *selPageTo )
{
	*minPage = 1;
	*maxPage = 1;
	*selPageFrom = 1;
	*selPageTo = 1;
}

bool
WxCasPrint::HasPage ( int pageNum )
{
	return ( pageNum == 1 );
}

void
WxCasPrint::DrawPageOne ( wxDC * dc )
{
	wxInt32 dc_w, dc_h;

	// Get the size of the DC in pixels
	dc->GetSize ( &dc_w, &dc_h );

	// Get the size of the image in pixels
	wxImage *statImage = wxGetApp ().GetMainFrame () ->GetStatImage ();

	wxUint32 marginX = 50;
	wxUint32 marginY = 50;

	wxUint32 sizeX = statImage->GetWidth () + 2 * marginX;
	wxUint32 sizeY = statImage->GetHeight () + 2 * marginY;

	// Calculate a suitable scaling factor
	float scale = wxMin ( ( float ) ( dc_w ) / sizeX, ( float ) ( dc_h ) / sizeY );

	// Calculate the position on the DC for centring the graphic
	float posX = marginX + ( dc_w - sizeX * scale ) / 2.0;
	float posY = marginY + ( dc_h - sizeY * scale ) / 2.0;

	// Set the scale and origin
	dc->SetUserScale ( scale, scale );
	dc->SetDeviceOrigin ( ( wxCoord ) posX, ( wxCoord ) posY );

	// Draw image
	dc->DrawBitmap ( wxBitmap( statImage ), 0, 0, FALSE );
}
