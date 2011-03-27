//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2003-2011 Patrizio Bassi ( hetfield@amule.org )
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include <wx/app.h>

#include "MuleTrayIcon.h"

#include <common/ClientVersion.h>
#include <common/Constants.h>

#include "pixmaps/mule_TrayIcon_big.ico.xpm"
#include "pixmaps/mule_Tr_yellow_big.ico.xpm"
#include "pixmaps/mule_Tr_grey_big.ico.xpm"

#include <wx/menu.h>

#include "amule.h" 			// Needed for theApp
#include "amuleDlg.h" 		// Needed for IsShown
#include "Preferences.h"	// Needed for thePrefs
#include "ServerConnect.h"	// Needed for CServerConnect
#include "Server.h"			// Needed for CServer
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg::getColors()
#include "Statistics.h"		// Needed for theStats
#include <common/Format.h>	// Needed for CFormat
#include "Logger.h"
#include <common/MenuIDs.h>	// Needed to access menu item constants

/****************************************************/
/******************* Event Table ********************/
/****************************************************/

BEGIN_EVENT_TABLE(CMuleTrayIcon, wxTaskBarIcon)
	EVT_TASKBAR_LEFT_DCLICK(CMuleTrayIcon::SwitchShow)
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
	label.Replace(_("kB/s"),wxT(""),TRUE);
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
				if (item->GetItemLabelText()==(_("Unlimited"))) {
					temp=UNLIMITED;
				}
				else {
					temp=GetSpeedFromString(item->GetItemLabelText());
				}
				thePrefs::SetMaxUpload(temp);

				#ifdef CLIENT_GUI
				// Send preferences to core.
				theApp->glob_prefs->SendToRemote();
				#endif
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
				if (item->GetItemLabelText()==(_("Unlimited"))) {
					temp=UNLIMITED;
				}
				else {
					temp=GetSpeedFromString(item->GetItemLabelText());
				}
				thePrefs::SetMaxDownload(temp);

				#ifdef CLIENT_GUI
				// Send preferences to core.
				theApp->glob_prefs->SendToRemote();
				#endif
			}
		}
	}
}


void CMuleTrayIcon::ServerConnection(wxCommandEvent& WXUNUSED(event))
{	
	wxCommandEvent evt;
	theApp->amuledlg->OnBnConnect(evt);
}


void CMuleTrayIcon::ShowHide(wxCommandEvent& WXUNUSED(event))
{
	theApp->amuledlg->DoIconize(theApp->amuledlg->IsShown());
}


void CMuleTrayIcon::Close(wxCommandEvent& WXUNUSED(event))
{
	if (theApp->amuledlg->IsEnabled()) {
		theApp->amuledlg->Close();
	}
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
}

/****************************************************/
/***************** Public Functions *****************/
/****************************************************/

void CMuleTrayIcon::SetTrayIcon(int Icon, uint32 percent)
{
	int Bar_ySize = 0;

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
			wxFAIL;
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
					wxFAIL;
			}
		}

		Old_Icon = Icon;
		Old_SpeedSize = NewSize;
		
		// Do whatever to the icon before drawing it (percent)
		
		wxBitmap TempBMP;
		TempBMP.CopyFromIcon(CurrentIcon);
		
		TempBMP.SetMask(NULL);

		IconWithSpeed.SelectObject(TempBMP);

		
		// Speed bar is: centered, taking 80% of the icon heigh, and 
		// right-justified taking a 10% of the icon width.
		
		// X
		int Bar_xSize = 4; 
		int Bar_xPos = CurrentIcon.GetWidth() - 5; 
		
		IconWithSpeed.SetBrush(*(wxTheBrushList->FindOrCreateBrush(CStatisticsDlg::getColors(11))));
		IconWithSpeed.SetPen(*wxTRANSPARENT_PEN);
		
		IconWithSpeed.DrawRectangle(Bar_xPos + 1, Bar_ySize - NewSize, Bar_xSize -2 , NewSize);
		
		// Unselect the icon.
		IconWithSpeed.SelectObject(wxNullBitmap);
		
		// Do transparency

		// Set a new mask with transparency set to red.
		wxMask* new_mask = new wxMask(TempBMP, wxColour(0xFF, 0x00, 0x00));
		
		TempBMP.SetMask(new_mask);
		CurrentIcon.CopyFromBitmap(TempBMP);

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

void CMuleTrayIcon::UpdateTray()
{
	// Icon update and Tip update
#ifndef __WXCOCOA__
	if (IsOk()) 
#endif
	{
		SetIcon(CurrentIcon, CurrentTip);
	}	
}


wxMenu* CMuleTrayIcon::CreatePopupMenu() 
{
	// Creates dinamically the menu to show the user.
	wxMenu *traymenu = new wxMenu();
	traymenu->SetTitle(_("aMule Tray Menu"));
	
	// Build the Top string name
	wxString label = MOD_VERSION_LONG;
	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();
	label = wxString(_("Speed limits:")) + wxT(" ");

	// Check for upload limits
	unsigned int max_upload = thePrefs::GetMaxUpload();
	if ( max_upload == UNLIMITED ) {
		label += _("UL: None");
	}
	else { 
		label += CFormat(_("UL: %u")) % max_upload;
	}
	label += wxT(", ");

	// Check for download limits
	unsigned int max_download = thePrefs::GetMaxDownload();
	if ( max_download == UNLIMITED ) {
		label += _("DL: None");
	}
	else {
		label += CFormat(_("DL: %u")) % max_download;
	}

	traymenu->Append(TRAY_MENU_INFO, label);
	label = CFormat(_("Download speed: %.1f")) % (theStats::GetDownloadRate() / 1024.0);
	traymenu->Append(TRAY_MENU_INFO, label);
	label = CFormat(_("Upload speed: %.1f")) % (theStats::GetUploadRate() / 1024.0);
	traymenu->Append(TRAY_MENU_INFO, label);
	traymenu->AppendSeparator();

	// Client Info
	wxMenu* ClientInfoMenu = new wxMenu();
	ClientInfoMenu->SetTitle(_("Client Information"));

	// User nick-name
	{
		wxString temp = CFormat(_("Nickname: %s")) % ( thePrefs::GetUserNick().IsEmpty() ? wxString(_("No Nickname Selected!")) : thePrefs::GetUserNick() );

		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Client ID
	{
		wxString temp = _("ClientID: ");
		
		if (theApp->IsConnectedED2K()) {
			temp += CFormat(wxT("%u")) % theApp->GetED2KID();
		} else {
			temp += _("Not connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Current Server and Server IP
	{
		wxString temp_name = _("ServerName: ");
		wxString temp_ip   = _("ServerIP: ");
		
		if ( theApp->serverconnect->GetCurrentServer() ) {
			temp_name += theApp->serverconnect->GetCurrentServer()->GetListName();
			temp_ip   += theApp->serverconnect->GetCurrentServer()->GetFullIP();
		} else {
			temp_name += _("Not connected");
			temp_ip   += _("Not Connected");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_name);
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp_ip);
	}
	
	// IP Address
	{
		wxString temp = CFormat(_("IP: %s")) % ( (theApp->GetPublicIP()) ? Uint32toStringIP(theApp->GetPublicIP()) : wxString(_("Unknown")) );

		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// TCP PORT
	{
		wxString temp;
		if (thePrefs::GetPort()) {
			temp = CFormat(_("TCP port: %d")) % thePrefs::GetPort();
		} else {
			temp=_("TCP port: Not ready");
		}
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// UDP PORT
	{
		wxString temp;
		if (thePrefs::GetEffectiveUDPPort()) {
			temp = CFormat(_("UDP port: %d")) % thePrefs::GetEffectiveUDPPort();
		} else {
			temp=_("UDP port: Not ready");
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
		wxString temp = CFormat(_("Uptime: %s")) % CastSecondsToHM(theStats::GetUptimeSeconds());
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of shared files
	{
		wxString temp = CFormat(_("Shared files: %d")) % theStats::GetSharedFileCount();
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	// Number of queued clients
	{
		wxString temp = CFormat(_("Queued clients: %d")) % theStats::GetWaitingUserCount();
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Total Downloaded
	{
		wxString temp = CastItoXBytes( theStats::GetSessionReceivedBytes() + thePrefs::GetTotalDownloaded() );
		temp = CFormat(_("Total DL: %s")) % temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}
	
	// Total Uploaded
	{
		wxString temp = CastItoXBytes( theStats::GetSessionSentBytes() + thePrefs::GetTotalUploaded() );
		temp = CFormat(_("Total UL: %s")) % temp;
		ClientInfoMenu->Append(TRAY_MENU_CLIENTINFO_ITEM,temp);
	}

	traymenu->Append(TRAY_MENU_CLIENTINFO,ClientInfoMenu->GetTitle(),ClientInfoMenu);
	
	// Separator
	traymenu->AppendSeparator();
	
	// Upload Speed sub-menu
	wxMenu* UploadSpeedMenu = new wxMenu();
	UploadSpeedMenu->SetTitle(_("Upload limit"));
	
	// Download Speed sub-menu
	wxMenu* DownloadSpeedMenu = new wxMenu();
	DownloadSpeedMenu->SetTitle(_("Download limit"));
	
	// Upload Speed sub-menu
	{
		UploadSpeedMenu->Append(UPLOAD_ITEM1, _("Unlimited"));

		uint32 max_ul_speed = thePrefs::GetMaxGraphUploadRate();
		
		if ( max_ul_speed == UNLIMITED ) {
			max_ul_speed = 100;
		}
		else if ( max_ul_speed < 10 ) {
			max_ul_speed = 10;
		}
			
		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_ul_speed / 5) * (5 - i);
			wxString temp = CFormat(wxT("%u %s")) % tempspeed % _("kB/s");
			UploadSpeedMenu->Append((int)UPLOAD_ITEM1+i+1,temp);
		}
	}
	traymenu->Append(0,UploadSpeedMenu->GetTitle(),UploadSpeedMenu);
	
	// Download Speed sub-menu
	{ 
		DownloadSpeedMenu->Append(DOWNLOAD_ITEM1, _("Unlimited"));

		uint32 max_dl_speed = thePrefs::GetMaxGraphDownloadRate();
		
		if ( max_dl_speed == UNLIMITED ) {
			max_dl_speed = 100;
		}
		else if ( max_dl_speed < 10 ) {
			max_dl_speed = 10;
		}
	
		for ( int i = 0; i < 5; i++ ) {
			unsigned int tempspeed = (unsigned int)((double)max_dl_speed / 5) * (5 - i);
			wxString temp = CFormat(wxT("%d %s")) % tempspeed % _("kB/s");
			DownloadSpeedMenu->Append((int)DOWNLOAD_ITEM1+i+1,temp);
		}
	}

	traymenu->Append(0,DownloadSpeedMenu->GetTitle(),DownloadSpeedMenu);
	// Separator
	traymenu->AppendSeparator();
	
	if (theApp->IsConnected()) {
		//Disconnection Speed item
		traymenu->Append(TRAY_MENU_DISCONNECT, _("Disconnect"));
	} else {
		//Connect item
		traymenu->Append(TRAY_MENU_CONNECT, _("Connect"));
	}
	
	// Separator
	traymenu->AppendSeparator();
	
	if (theApp->amuledlg->IsShown()) {
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

void CMuleTrayIcon::SwitchShow(wxTaskBarIconEvent&)
{
	theApp->amuledlg->DoIconize(theApp->amuledlg->IsShown());
}
// File_checked_for_headers
