//
// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 Patrizio Bassi (Hetfield) ( hetfield@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma implementation "MuleTrayIcon.h"
#endif

#include "MuleTrayIcon.h"

#if USE_WX_TRAY 

#include "pixmaps/mule_TrayIcon_big.ico.xpm"
#include "pixmaps/mule_Tr_yellow_big.ico.xpm"
#include "pixmaps/mule_Tr_grey_big.ico.xpm"

#include <wx/menu.h>
#include <wx/string.h>
#include <wx/intl.h>

#include "OPCodes.h" 			// Needed for MOD_VERSION_LONG
#include "amule.h" 			// Needed for theApp
#include "amuleDlg.h" 			// Needed for IsShown
#include "Preferences.h"		// Needed for glod_prefs
#include "DownloadQueue.h" 		// Needed for GetKbps
#include "UploadQueue.h" 		// Needed for GetKbps
#include "ServerConnect.h"			// Needed for CServerConnect
#include "OtherFunctions.h"		// Needed for CastSecondsToHM
#include "Server.h"			// Needed for CServer
#include "NetworkFunctions.h"		// Needed for Uint32toStringIP
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "Statistics.h"
#include "Logger.h"

using namespace otherfunctions;

// Pop-up menu clickable entries
enum {
	TRAY_MENU_INFO = 0,
	TRAY_MENU_CLIENTINFO=0,
	TRAY_MENU_CLIENTINFO_ITEM = 13007,
	TRAY_MENU_DISCONNECT,
	TRAY_MENU_CONNECT,
	TRAY_MENU_HIDE,
	TRAY_MENU_SHOW,
	TRAY_MENU_EXIT,
	UPLOAD_ITEM1=12340,
	UPLOAD_ITEM2=12341,
	UPLOAD_ITEM3=12342,
	UPLOAD_ITEM4=12343,
	UPLOAD_ITEM5=12344,
	UPLOAD_ITEM6=12345,
	DOWNLOAD_ITEM1=54320,
	DOWNLOAD_ITEM2=54321,
	DOWNLOAD_ITEM3=54322,
	DOWNLOAD_ITEM4=54323,
	DOWNLOAD_ITEM5=54324,
	DOWNLOAD_ITEM6=54325,
};

/****************************************************/
/******************* Event Table ********************/
/****************************************************/

BEGIN_EVENT_TABLE(CMuleTrayIcon, wxTaskBarIcon)
	EVT_TASKBAR_LEFT_DOWN(CMuleTrayIcon::SwitchShow)
	EVT_MENU( TRAY_MENU_EXIT, CMuleTrayIcon::Close)
	EVT_MENU( TRAY_MENU_CONNECT, CMuleTrayIcon::ServerConnection)
	EVT_MENU( TRAY_MENU_DISCONNECT, CMuleTrayIcon::ServerConnection)
	EVT_MENU( TRAY_MENU_HIDE, CMuleTrayIcon::ShowHide)
	EVT_MENU( TRAY_MENU_SHOW, CMuleTrayIcon::ShowHide)
	EVT_MENU( UPLOAD_ITEM1, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM2, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM3, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM4, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM5, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( UPLOAD_ITEM6, CMuleTrayIcon::SetUploadSpeed)
	EVT_MENU( DOWNLOAD_ITEM1, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM2, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM3, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM4, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM5, CMuleTrayIcon::SetDownloadSpeed)
	EVT_MENU( DOWNLOAD_ITEM6, CMuleTrayIcon::SetDownloadSpeed)
END_EVENT_TABLE()

/****************************************************/
/************ Constructor / Destructor **************/
/****************************************************/

long GetSpeedFromString(wxString label){
	long temp;
	label.Replace(wxT("kB/s"),wxT(""),TRUE);
	label.Trim(FALSE);
	label.Trim(TRUE);
	label.ToLong(&temp);
	return temp;
}

void CMuleTrayIcon::SetUploadSpeed(wxCommandEvent& event){

	wxObject* obj=event.GetEventObject();
	if (obj!=NULL) {
		wxMenu *menu = dynamic_cast<wxMenu *>(obj);
		if (menu) {
			wxMenuItem* item=menu->FindItem(event.GetId());
			if (item!=NULL) {
				long temp;
				if (item->GetLabel()==(_("Unlimited"))) {
					temp=UNLIMITED;
				}
				else {
					temp=GetSpeedFromString(item->GetLabel());
				}
				thePrefs::SetMaxUpload(temp);
			}
		}
	}
}

void CMuleTrayIcon::SetDownloadSpeed(wxCommandEvent& event){
	
	wxObject* obj=event.GetEventObject();
	if (obj!=NULL) {
		wxMenu *menu = dynamic_cast<wxMenu *>(obj);
		if (menu) {
			wxMenuItem* item=menu->FindItem(event.GetId());
			if (item!=NULL) {
				long temp;
				if (item->GetLabel()==(_("Unlimited"))) {
					temp=UNLIMITED;
				}
				else {
					temp=GetSpeedFromString(item->GetLabel());
				}
				thePrefs::SetMaxDownload(temp);
			}
		}
	}
}

void CMuleTrayIcon::ServerConnection(wxCommandEvent& event){
	
	if (event.GetId()==TRAY_MENU_CONNECT) {
		if ( theApp.serverconnect->IsConnected() ) {
			theApp.serverconnect->Disconnect();
		} else if ( !theApp.serverconnect->IsConnecting() ) {
			AddLogLineM(true, _("Connecting"));
			theApp.serverconnect->ConnectToAnyServer();
			theApp.amuledlg->ShowConnectionState(false);
		}
	}
	if (event.GetId()==TRAY_MENU_DISCONNECT) {
		if ( theApp.serverconnect->IsConnected() ) {
			theApp.serverconnect->Disconnect();
		}
	}

}
void CMuleTrayIcon::ShowHide(wxCommandEvent& WXUNUSED(event)){

	if ( theApp.amuledlg->IsShown() ) {
		theApp.amuledlg->Hide_aMule();
	}
	else {
		theApp.amuledlg->Show_aMule();
	}
}

void  CMuleTrayIcon::Close(wxCommandEvent& WXUNUSED(event)){
	wxCloseEvent SendCloseEvent;
	theApp.amuledlg->OnClose(SendCloseEvent);
}

CMuleTrayIcon::CMuleTrayIcon()
{
	Old_Icon = -1;
	Old_SpeedSize = 0xFFFF; // must be > any possible one.
	// Create the background icons (speed improvement)
	HighId_Icon_size = wxIcon(mule_TrayIcon_big_ico_xpm).GetHeight();
	LowId_Icon_size = wxIcon(mule_Tr_yellow_big_ico_xpm).GetHeight();
	Disconnected_Icon_size = wxIcon(mule_Tr_grey_big_ico_xpm).GetHeight();
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
	int Bar_ySize;

	switch (Icon) {
		case TRAY_ICON_HIGHID:
			// Most likely case, test first
			Bar_ySize = HighId_Icon_size; 
			break;
		case TRAY_ICON_LOWID:
			Bar_ySize = LowId_Icon_size; 
			break;
		case TRAY_ICON_DISCONNECTED:
			Bar_ySize = Disconnected_Icon_size; 
			break;
		default:
			wxASSERT(0);
	}
	// Lookup this values for speed improvement: don't draw if not needed
	int NewSize = (Bar_ySize * percent) / 100;
	
	if ((Old_Icon != Icon) || (Old_SpeedSize != NewSize)) {

		if ((Old_SpeedSize > NewSize) || (Old_Icon != Icon)) {
			// We have to rebuild the icon, because bar is lower now.
			switch (Icon) {
				case TRAY_ICON_HIGHID:
					// Most likely case, test first
					CurrentIcon = wxIcon(mule_TrayIcon_big_ico_xpm);
					break;
				case TRAY_ICON_LOWID:
					CurrentIcon = wxIcon(mule_Tr_yellow_big_ico_xpm);
					break;
				case TRAY_ICON_DISCONNECTED:
					CurrentIcon = wxIcon(mule_Tr_grey_big_ico_xpm);
					break;
				default:
					wxASSERT(0);
			}
		}

		Old_Icon = Icon;
		Old_SpeedSize = NewSize;
		
		// Do whatever to the icon before drawing it (percent)
		CurrentIcon.SetMask(NULL);
		
		IconWithSpeed.SelectObject(CurrentIcon);
		
		// Speed bar is: centered, taking 80% of the icon heigh, and 
		// right-justified taking a 10% of the icon width.
		
		// X
		int Bar_xSize = (CurrentIcon.GetWidth() / 4); 
		int Bar_xPos = CurrentIcon.GetWidth() - Bar_xSize -1; 
			
		IconWithSpeed.SetBrush(*wxBLACK_BRUSH);
		IconWithSpeed.SetPen(*wxTRANSPARENT_PEN);
		
		IconWithSpeed.DrawRectangle(Bar_xPos + 1, Bar_ySize - NewSize, Bar_xSize -2 , NewSize);
		
		// Unselect the icon.
		IconWithSpeed.SelectObject(wxNullBitmap);
		
		// Do transparency
		
		// Set a new mask with transparency set to red.
		wxMask* new_mask = new wxMask(CurrentIcon, wxColour(0xFF, 0x00, 0x00));
		
		CurrentIcon.SetMask(new_mask);

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
	}
	else { 
		label += wxString::Format( _("UL: %u, "), max_upload);
	}

	// Check for download limits
	unsigned int max_download = thePrefs::GetMaxDownload();
	if ( max_download == UNLIMITED ) {
		label += wxString::Format( _("DL: None"));
	}
	else {
		label += wxString::Format( _("DL: %u"), max_download);
	}

	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();

	// Client Info
	wxMenu* ClientInfoMenu = new wxMenu();
	ClientInfoMenu->SetTitle(_("Client Information"));

	// User nick-name
	{
		wxString temp = _("Nickname: ");
		if ( thePrefs::GetUserNick().IsEmpty() ) {
			temp += _("No Nickname Selected!");
		}
		else {
			temp += thePrefs::GetUserNick();
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Client ID
	{
		wxString temp = _("ClientID: ");
		
		if (theApp.serverconnect->IsConnected()) {
			unsigned long id = theApp.serverconnect->GetClientID();
			temp += wxString::Format(wxT("%lu"), id);
		} else {
			temp += _("Not Connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Current Server and Server IP
	{
		wxString temp_name = _("ServerName: ");
		wxString temp_ip   = _("ServerIP: ");
		
		if ( theApp.serverconnect->GetCurrentServer() ) {
			temp_name += theApp.serverconnect->GetCurrentServer()->GetListName();
			temp_ip   += theApp.serverconnect->GetCurrentServer()->GetFullIP();
		} else {
			temp_name += _("Not Connected");
			temp_ip   += _("Not Connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_name);
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_ip);
	}
	
	// IP Address
	{
		wxString temp = _("IP: ");
		if ( theApp.GetPublicIP() ) {
			temp += Uint32toStringIP(theApp.GetPublicIP()); 
		} else {
			temp += _("Unknown");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// TCP PORT
	{
		wxString temp;
		if (thePrefs::GetPort()) {
			temp = wxString::Format(wxT("%s%d"), _("TCP Port: "), thePrefs::GetPort());
		} else {
			temp=_("TCP Port: Not Ready");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// UDP PORT
	{
		wxString temp;
		if (thePrefs::GetUDPPort()) {
			temp = wxString::Format(wxT("%s%d"), _("UDP Port: "), thePrefs::GetUDPPort());	
		} else {
			temp=_("UDP Port: Not Ready");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Online Signature
	{
		wxString temp;
		if (thePrefs::IsOnlineSignatureEnabled()) {
			temp=_("Online Signature: Enabled");
		}
		else {
			temp=_("Online Signature: Disabled");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Uptime
	{
		wxString temp = _("Uptime: ") + CastSecondsToHM(theApp.statistics->GetUptimeSecs());
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of shared files
	{
		wxString temp = wxString::Format(wxT("%s%d"), _("Shared Files: "), theApp.sharedfiles->GetCount());
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of queued clients
	{
		wxString temp = wxString::Format(wxT("%s%d"), _("Queued Clients: "), theApp.uploadqueue->GetWaitingUserCount() );
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Total Downloaded
	{
		wxString temp = CastItoXBytes( theApp.statistics->GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded() );
		temp = wxString(_("Total DL: ")) + temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Total Uploaded
	{
		wxString temp = CastItoXBytes( theApp.statistics->GetSessionSentBytes() + thePrefs::GetTotalUploaded() );
		temp = wxString(_("Total UL: ")) + temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	traymenu->Append(TRAY_MENU_CLIENTINFO,ClientInfoMenu->GetTitle(),ClientInfoMenu);
	
	// Separator
	traymenu->AppendSeparator();
	
	// Upload Speed sub-menu
	wxMenu* UploadSpeedMenu = new wxMenu();
	UploadSpeedMenu->SetTitle(_("Upload Limit"));
	
	// Download Speed sub-menu
	wxMenu* DownloadSpeedMenu = new wxMenu();
	DownloadSpeedMenu->SetTitle(_("Download Limit"));
	
	// Upload Speed sub-menu
	{
		wxString temp=wxString(_("Unlimited"));
		UploadSpeedMenu->Append(UPLOAD_ITEM1,temp);

		uint32 max_ul_speed = thePrefs::GetMaxGraphUploadRate();
		
		if ( max_ul_speed == UNLIMITED ) {
			max_ul_speed = 100;
		}
		else if ( max_ul_speed < 10 ) {
			max_ul_speed = 10;
		}
			
		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_ul_speed / 5) * (5 - i);
			wxString temp = wxString::Format(wxT("%u%s "), tempspeed, wxT("kB/s"));
			UploadSpeedMenu->Append((int)UPLOAD_ITEM1+i+1,temp);
		}
	}
	traymenu->Append(0,UploadSpeedMenu->GetTitle(),UploadSpeedMenu);
	
	// Download Speed sub-menu
	{ 
		wxString temp=wxString(_("Unlimited"));
		
		DownloadSpeedMenu->Append(DOWNLOAD_ITEM1,temp);

		uint32 max_dl_speed = thePrefs::GetMaxGraphDownloadRate();
		
		if ( max_dl_speed == UNLIMITED ) {
			max_dl_speed = 100;
		}
		else if ( max_dl_speed < 10 ) {
			max_dl_speed = 10;
		}
	
		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_dl_speed / 5) * (5 - i);
			wxString temp = wxString::Format(wxT("%d%s "), tempspeed, wxT("kB/s"));
			DownloadSpeedMenu->Append((int)DOWNLOAD_ITEM1+i+1,temp);
		}
	}

	traymenu->Append(0,DownloadSpeedMenu->GetTitle(),DownloadSpeedMenu);
	// Separator
	traymenu->AppendSeparator();
	
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
