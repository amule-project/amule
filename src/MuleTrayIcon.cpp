 // This file is part of the aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include "MuleTrayIcon.h"

#ifdef USE_WX_TRAY 

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/intl.h>		

#include "opcodes.h" 			// Needed for MOD_VERSION_LONG
#include "amule.h" 				// Needed for theApp
#include "amuleDlg.h" 			// Needed for IsShown
#include "Preferences.h"		// Needed for glod_prefs
#include "DownloadQueue.h" 	// Needed for GetKbps
#include "UploadQueue.h" 		// Needed for GetKbps


/****************************************************/
/******************* Event Table ********************/
/****************************************************/


BEGIN_EVENT_TABLE(CMuleTrayIcon, wxTaskBarIcon)
	EVT_TASKBAR_LEFT_DOWN(CMuleTrayIcon::SwitchShow)
END_EVENT_TABLE()


/****************************************************/
/************ Constructor / Destructor **************/
/****************************************************/


CMuleTrayIcon::CMuleTrayIcon()
{
}


CMuleTrayIcon::~CMuleTrayIcon() 
{
	// If there's an icon set, remove it
	if (IsIconInstalled()) {
		RemoveIcon();
	}

}

/****************************************************/
/***************** Public Functions *****************/
/****************************************************/


void CMuleTrayIcon::SetTrayIcon(wxIcon& Icon, uint32 percent) 
{
	CurrentIcon = Icon;
	// Do whatever to the icon before drawing it (percent)
	
	
	IconWithSpeed.SelectObject(CurrentIcon);

//	wxColour temp;
//	IconWithSpeed.GetPixel(0,0, &temp);
	
	// Set the colour for the traffic bar.
	IconWithSpeed.SetBrush(*wxTRANSPARENT_BRUSH);
	IconWithSpeed.SetPen(*wxCYAN_PEN);
	
	// Speed bar is: centered, taking 80% of the icon heigh, and 
	// right-justified taking a 10% of the icon width.
	// X
	int Bar_xSize = (Icon.GetWidth() / 4); 
	int Bar_xPos = Bar_xSize*2; 
	// Y
	int Bar_yPos = (Icon.GetHeight() / 10); // 0 + 10% = start ;)
	int Bar_ySize = Icon.GetHeight() - (Bar_yPos * 2); // 10% * 8 = 80% ;)
	 
	IconWithSpeed.DrawRectangle(Bar_xPos, Bar_yPos, Bar_xSize, Bar_ySize);
	
	IconWithSpeed.SetBrush(*wxBLUE_BRUSH);
	IconWithSpeed.SetPen(*wxTRANSPARENT_PEN);
	
	int NewSize = ((Bar_ySize -2) * percent) / 100;
	
	IconWithSpeed.DrawRectangle(Bar_xPos + 1, Bar_yPos + (Bar_ySize - NewSize) + 1, Bar_xSize -2 , NewSize);
	
	// Unselect the icon.
	IconWithSpeed.SelectObject(wxNullBitmap);	
	
//	wxMask* new_mask = new wxMask(CurrentIcon, temp);
	
//	CurrentIcon.SetMask(new_mask);
	
	UpdateTray();
}
		
void CMuleTrayIcon::SetTrayToolTip(const wxString& Tip)
{
	CurrentTip = Tip;
	UpdateTray();
}


/****************************************************/
/**************** Private Functions *****************/
/****************************************************/

void CMuleTrayIcon::UpdateTray() {
	// Icon update and Tip update
	SetIcon(CurrentIcon, CurrentTip);
}

enum {
	TRAY_MENU_INFO = 10317,
	TRAY_MENU_CLIENTINFO
};

wxMenu* CMuleTrayIcon::CreatePopupMenu() 
{
   // Creates dinamically the menu to show the user.
	wxMenu *traymenu = new wxMenu();
	traymenu->SetTitle(_("aMule Tray Menu"));
	
	// Build the Top string name
	wxString label = MOD_VERSION_LONG wxT(":\n");
	label += wxString::Format(_("Download Speed: %.1f\n"), theApp.downloadqueue->GetKBps());
	label += wxString::Format(_("Upload Speed: %.1f\n"), theApp.uploadqueue->GetKBps());
	label += _("\nSpeed Limits:\n");

	// Check for upload limits
	unsigned int max_upload = thePrefs::GetMaxUpload();
	if ( max_upload == UNLIMITED ) {
		label += wxString::Format( _("UL: None, "));
	} else {
		label += wxString::Format( _("UL: %u, "), max_upload);
	}

	// Check for download limits
	unsigned int max_download = thePrefs::GetMaxDownload();
	if ( max_download == UNLIMITED ) {
		label += wxString::Format( _("DL: None"));
	} else {
		label += wxString::Format( _("DL: %u"), max_download);
	}
	
	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();
	
	
	return traymenu;
}		

void CMuleTrayIcon::SwitchShow(wxTaskBarIconEvent&) {
	if ( theApp.amuledlg->IsShown() ) {
		theApp.amuledlg->Hide_aMule();
	} else {
		theApp.amuledlg->Show_aMule();
	}	
}


#endif // USE_WX_TRAY
