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

#include "pixmaps/mule_TrayIcon_big.ico.xpm"
#include "pixmaps/mule_Tr_yellow_big.ico.xpm"
#include "pixmaps/mule_Tr_grey_big.ico.xpm"

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/intl.h>		

#include "opcodes.h" 			// Needed for MOD_VERSION_LONG
#include "amule.h" 				// Needed for theApp
#include "amuleDlg.h" 			// Needed for IsShown
#include "Preferences.h"		// Needed for glod_prefs
#include "DownloadQueue.h" 	// Needed for GetKbps
#include "UploadQueue.h" 		// Needed for GetKbps
#include "sockets.h"			// Needed for CServerConnect

// Pop-up menu clickable entries

enum {
	TRAY_MENU_INFO = 10317,
	TRAY_MENU_CLIENTINFO,
	TRAY_MENU_DISCONNECT,
	TRAY_MENU_CONNECT,
	TRAY_MENU_HIDE,
	TRAY_MENU_SHOW,
	TRAY_MENU_EXIT
};


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
	Old_Icon = -1;
	Old_SpeedSize = -1;
	// Create the background icons (speed improvement)
	HighId_Icon 			= wxIcon(mule_TrayIcon_big_ico_xpm);
	LowId_Icon 			= wxIcon(mule_Tr_yellow_big_ico_xpm);
	Disconnected_Icon	= wxIcon(mule_Tr_grey_big_ico_xpm);
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


void CMuleTrayIcon::SetTrayIcon(int Icon, uint32 percent) 
{

	switch (Icon) {
		case TRAY_ICON_HIGHID:
			// Most likely case, test first
			CurrentIcon = HighId_Icon;
			break;
		case TRAY_ICON_LOWID:
			CurrentIcon = LowId_Icon;
			break;
		case TRAY_ICON_DISCONNECTED:
			CurrentIcon = Disconnected_Icon;
			break;
		default:
			// W00T?
			wxASSERT(0);			
	}

	// Lookup this values for speed improvement: don't draw if not needed
	
	int Bar_ySize = CurrentIcon.GetHeight()-2; 
	int NewSize = ((Bar_ySize -2) * percent) / 100;
	
	if ((Old_Icon != Icon) || (Old_SpeedSize != NewSize)) {

		
		Old_Icon = Icon;
		Old_SpeedSize = NewSize;
		
		// Do whatever to the icon before drawing it (percent)
	
//		wxColour temp;
		
//		IconWithSpeed.SelectObject(CurrentIcon);
	
		// Get the transparency colour.
//		IconWithSpeed.GetPixel(0,0, &temp);
	
//		IconWithSpeed.SelectObject(wxNullBitmap);
		
		// Set a new mask with transparency removed
//		wxMask* new_mask = new wxMask(CurrentIcon, temp);
		
//		CurrentIcon.SetMask(new_mask);
		
		IconWithSpeed.SelectObject(CurrentIcon);
		
		// Get the solid background.
//		IconWithSpeed.GetPixel(0,0, &temp);
		
		// Set the colour for the traffic bar.
		IconWithSpeed.SetBrush(*wxTRANSPARENT_BRUSH);
		IconWithSpeed.SetPen(*wxCYAN_PEN);
		
		// Speed bar is: centered, taking 80% of the icon heigh, and 
		// right-justified taking a 10% of the icon width.
		// X
	
		int Bar_xSize = (CurrentIcon.GetWidth() / 4); 
		int Bar_xPos = CurrentIcon.GetWidth() - Bar_xSize -1; 
			
		// Y
		int Bar_yPos = 0;
		 
		IconWithSpeed.DrawRectangle(Bar_xPos, Bar_yPos, Bar_xSize, Bar_ySize);
		
		IconWithSpeed.SetBrush(*wxBLUE_BRUSH);
		IconWithSpeed.SetPen(*wxTRANSPARENT_PEN);
		
		IconWithSpeed.DrawRectangle(Bar_xPos + 1, (Bar_yPos + 1) + ((Bar_ySize -2) - NewSize), Bar_xSize -2 , NewSize);
	
		// Unselect the icon.
		IconWithSpeed.SelectObject(wxNullBitmap);	
		
	
//		new_mask = new wxMask(CurrentIcon, temp);
		
//		CurrentIcon.SetMask(new_mask);

		UpdateTray();
	}
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
//actually adds too many separator only!
/* 	
	// Mule info
	wxMenu* aMuleInfoMenu = new wxMenu();
	aMuleInfoMenu->SetTitle(_("aMule Tray Menu Info"));

	// Client Info
	wxMenu* ClientInfoMenu = new wxMenu();
	ClientInfoMenu->SetTitle(_("Client Information"));
	
	// Separator
	traymenu->AppendSeparator();
	
	// Upload Speed sub-menu
	wxMenu* UploadSpeedMenu = new wxMenu();
	UploadSpeedMenu->SetTitle(_("Upload Limit"));
	
	// Download Speed sub-menu
	wxMenu* DownloadSpeedMenu = new wxMenu();
	DownloadSpeedMenu->SetTitle(_("Download Limit"));
	
	// Separator
	traymenu->AppendSeparator();
*/	
	if (theApp.serverconnect->IsConnected()) {
		//Disconnection Speed item
		traymenu->Append(TRAY_MENU_DISCONNECT, _("Disconnect from server"));
	} else {
		//Connect item
		traymenu->Append(TRAY_MENU_CONNECT, _("Connect to any server"));
	}
	
	// Separator
	traymenu->AppendSeparator();
	
	if (theApp.amuledlg->IsShown()) {
		//hide item
		traymenu->Append(TRAY_MENU_HIDE, _("Hide aMule"));
	} else {
		//show item
		traymenu->Append(TRAY_MENU_SHOW, _("Show aMule"));
	}
	
	// Separator
	traymenu->AppendSeparator();

	// Exit item
	traymenu->Append(TRAY_MENU_EXIT, _("Exit"));
	
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
