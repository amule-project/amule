// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Tiku & Patrizio Bassi aka Hetfield <hetfield@email.it>
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

// this file will implement GNOME/KDE compatible system tray icon

#include <libintl.h>
#include <clocale>		// Needed for setlocale(3)
#ifndef __WXMSW__
#ifdef __OPENBSD__
       #include <sys/types.h>
#endif /* __OPENBSD__ */
	#include <sys/socket.h>		//
	#include <net/if.h>		// Needed for struct ifreq
	#include <netinet/in.h>		// Needed for inet_ntoa
	#include <arpa/inet.h>		//
	#include <sys/ioctl.h>		// Needed for SIOCGIFADDR
	#include <gtk/gtk.h>		// Needed for gtk_object_get_data
	#include <X11/Xatom.h>		// Needed for XA_WINDOW
#endif

#include "pixmaps/mule_Tr_grey.ico.xpm"
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "UploadListCtrl.h"	// Needed for CUploadListCtrl
#include "SysTray.h"		// Interface declarations.
#include "GetTickCount.h"	// Needed for GetTickCount
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "server.h"		// Needed for GetListName
#include "otherfunctions.h"	// Needed for EncodeBase16
#include "sockets.h"		// Needed for CServerConnect
#include "SharedFilesCtrl.h"	// Needed for CSharedFilesCtrl
#include "SharedFilesWnd.h"	// Needed for CSharedFilesWnd
#include "ServerListCtrl.h"	// Needed for CServerListCtrl
#include "ServerWnd.h"		// Needed for CServerWnd
#include "DownloadListCtrl.h"	// Needed for CDownloadListCtrl
#include "TransferWnd.h"	// Needed for CTransferWnd
#include "opcodes.h"		// Needed for UNLIMITED
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"		// Needed for theApp
#ifdef __WXGTK__
	#include "eggtrayicon.h"	// Needed for egg_tray_icon_new
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for LOCALEDIR, PACKAGE and VERSION
#endif

#ifdef __WXMSW__

CSysTray::CSysTray(
	wxWindow *parent, int desktopMode, const wxString &title
) {
}

// Stupid? Sure, but lets stay identical to gtktray solution below for now... 
void CSysTray::TraySetToolTip(char* data) {
	SetIcon(c, data);
}

void CSysTray::TraySetIcon(char** data, bool what, int* pVals) {
	SetIcon(wxIcon(data));
	c=wxIcon(data);
}

// Hm? This does the grey-thingie? Need wxDC/wxImage manipulation here then...
bool CSysTray::SetColorLevels(int* pLimits, COLORREF* pColors, int nEntries) {
	return true;
}

#endif // __WXMSW__

#ifdef __WXGTK__ // Only use this code in wxGTK since it uses GTK code.

// PA: use pure gettext instead of wx functions 
#ifdef _
#undef _
#endif
#define _(String) gettext(String)


#ifdef __SAFE_TRAY__

gchar* getIP()
{

 gchar* ip=_("Detection Disabled");

 return ip;

}

#else

gchar* getIP()
{

  gchar* ip=_("Not Found");
  wxString interface;
  int index;
  index=0;
  int   sfd;
  struct ifreq ifr;
  struct sockaddr_in *sin = (struct sockaddr_in *) &ifr.ifr_addr;

  memset(&ifr, 0, sizeof ifr);
  sfd = socket(AF_INET, SOCK_STREAM, 0);

  strcpy(ifr.ifr_name, "ppp0");
  sin->sin_family = AF_INET;
  if (0 == ioctl(sfd, SIOCGIFADDR, &ifr)) {
	ip=inet_ntoa(sin->sin_addr);
	 return ip;
	 }
  else {

//	printf("Not connected at network with ppp0 direct connection\n");
 	 do
  	{
    	interface="eth"+wxString::Format("%d", index);
    	strcpy(ifr.ifr_name, interface);
    	sin->sin_family = AF_INET;
    	if (0 == ioctl(sfd, SIOCGIFADDR, &ifr)) {
      		ip=inet_ntoa(sin->sin_addr);
		index++;
    	}
//    	else printf(wxString("Not connected at network with ")+interface);

  	} while (0 == ioctl(sfd, SIOCGIFADDR, &ifr)) ;
}
 return ip;

}
#endif // __SAFE_TRAY__

//same check of the connection tab.
void speed_check(){

	if (theApp.glob_prefs->GetMaxGraphDownloadRate() < theApp.glob_prefs->GetMaxDownload()) theApp.glob_prefs->SetDownloadlimit(UNLIMITED);
	if (theApp.glob_prefs->GetMaxGraphUploadRate() < theApp.glob_prefs->GetMaxUpload()) theApp.glob_prefs->SetUploadlimit(UNLIMITED);

    if( theApp.glob_prefs->GetMaxUpload() != UNLIMITED ){
	if( theApp.glob_prefs->GetMaxUpload() < 4 && ( theApp.glob_prefs->GetMaxUpload()*3 < theApp.glob_prefs->GetMaxDownload() ) )
	  theApp.glob_prefs->SetDownloadlimit((theApp.glob_prefs->GetMaxUpload()*3));

	if( theApp.glob_prefs->GetMaxUpload() < 10 && ( theApp.glob_prefs->GetMaxUpload()*4 < theApp.glob_prefs->GetMaxDownload() ) )
	  theApp.glob_prefs->SetDownloadlimit((theApp.glob_prefs->GetMaxUpload()*4)) ;
      }
}

//closes aMule
void close_amule() {

	wxCloseEvent SendCloseEvent;

	theApp.amuledlg->OnClose(SendCloseEvent);

}

void do_hide() {
	theApp.amuledlg->Hide_aMule();
}

void do_show() {
	theApp.amuledlg->Show_aMule();
}

// shows or hides amule...double click automatic selection
void showgui(){

	if (theApp.amuledlg->IsShown()) {
		do_hide();
	} else {
		do_show();
	}
}

//set download and upload speed to max
void set_all_max() {

theApp.glob_prefs->SetUploadlimit(theApp.glob_prefs->GetMaxGraphUploadRate());
theApp.glob_prefs->SetDownloadlimit(theApp.glob_prefs->GetMaxGraphDownloadRate());

}

//set download and upload speed to min
void set_all_min() {

theApp.glob_prefs->SetUploadlimit(2);
theApp.glob_prefs->SetDownloadlimit(2);
speed_check();
}

//connect to a server
void connect_any_server() {

if (theApp.serverconnect->IsConnected()) theApp.serverconnect->Disconnect();
theApp.amuledlg->AddLogLine(true, _("Connecting"));
theApp.serverconnect->ConnectToAnyServer();
theApp.amuledlg->ShowConnectionState(false);

}

//disconnect
void disconnect(){
	if (theApp.serverconnect->IsConnected()) {
		theApp.serverconnect->Disconnect();
	}
	//else printf("Already disconnected!\n");
}

//set download speed
void set_dl_speed(GtkWidget *widget, GdkEventButton *event, gpointer data) {

int temp;
gchar* tempchar=g_strdup_printf("%s", (char*)gtk_object_get_data (GTK_OBJECT(widget), "label") );
temp = atoi(tempchar);
free(tempchar);
if (temp==0) theApp.glob_prefs->SetDownloadlimit(UNLIMITED);
else theApp.glob_prefs->SetDownloadlimit(temp);
speed_check();
}

//set upload speed
void set_up_speed(GtkWidget *widget, GdkEventButton *event, gpointer data) {

int temp;
gchar* tempchar=g_strdup_printf("%s", (char*)gtk_object_get_data (GTK_OBJECT(widget), "label") );
temp = atoi(tempchar);
free(tempchar);
if (temp==0) theApp.glob_prefs->SetUploadlimit(UNLIMITED);
else theApp.glob_prefs->SetUploadlimit(temp);
speed_check();
}

//create menu linked to the tray icon
static gboolean
tray_menu (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	//sets gtk objects
	GtkWidget *status_menu,*item,*down_speed,*dl_item,*up_speed,*up_item,*info_menu,*info_item;
	wxString label;
	wxString upl_speed,dl_speed;
	gchar* temp,*tempstring;
	int tempspeed;
	uint16 max_dl_speed,max_up_speed;
	uint16 actual_dl_speed, actual_up_speed;

	speed_check();

	actual_up_speed=theApp.glob_prefs->GetMaxUpload();
	actual_dl_speed=theApp.glob_prefs->GetMaxDownload();
	max_up_speed=theApp.glob_prefs->GetMaxGraphUploadRate();
	max_dl_speed=theApp.glob_prefs->GetMaxGraphDownloadRate();

	//what will be shown, very nice!
	if (actual_dl_speed==UNLIMITED || actual_dl_speed==0) dl_speed=_("Unlimited");
	else { temp = g_strdup_printf("%d", actual_dl_speed ); dl_speed=temp;}
	if (actual_up_speed==UNLIMITED || actual_up_speed==0) upl_speed=_("Unlimited");
	else { temp = g_strdup_printf("%d", actual_up_speed ); upl_speed=temp;}

	if (max_dl_speed==UNLIMITED || max_dl_speed==0) max_dl_speed=100;
	if (max_up_speed==UNLIMITED || max_up_speed==0) max_up_speed=100;

	label=_("aMule ")+wxString(VERSION)+"\n"+_("Actual Speed Limits:")+"\n"+_("DL: ")+dl_speed+_(" kb/s ")+_("UP: ")+upl_speed+_(" kb/s");

	//info menu
	info_menu=gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(info_menu),_("aMule Tray Menu Info"));

	if (theApp.glob_prefs->GetUserNick()!=NULL)
	info_item=gtk_menu_item_new_with_label(wxString(_("Nick: "))+theApp.glob_prefs->GetUserNick());
	else info_item=gtk_menu_item_new_with_label(_("Nick: Not Ready"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);


	if (theApp.glob_prefs->GetUserHash()){
		wxString hash;
		hash=EncodeBase16((const unsigned char*)theApp.glob_prefs->GetUserHash(),16);
		info_item=gtk_menu_item_new_with_label(wxString(_("Hash: "))+hash);
	}
	else info_item=gtk_menu_item_new_with_label(_("Hash: Not Ready"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.serverconnect->IsConnected()) {
	info_item=gtk_menu_item_new_with_label(wxString(_("ClientID: "))+wxString::Format("%.0f",(float)theApp.serverconnect->GetClientID()));
	}
	else info_item=gtk_menu_item_new_with_label(_("ID: Not Connected"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);



	tempstring=getIP();
	info_item=gtk_menu_item_new_with_label(wxString(_("IP: "))+tempstring);
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.glob_prefs->GetPort()) {
		tempstring = g_strdup_printf("%d", theApp.glob_prefs->GetPort() );
		info_item=gtk_menu_item_new_with_label(wxString(_("TCP Port: "))+tempstring);
	}
	else info_item=gtk_menu_item_new_with_label(_("TCP Port: Not Ready"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.glob_prefs->GetUDPPort()) {
		tempstring = g_strdup_printf("%d", theApp.glob_prefs->GetUDPPort() );
		info_item=gtk_menu_item_new_with_label(wxString(_("UDP Port: "))+tempstring);
	}
	else info_item=gtk_menu_item_new_with_label(_("UDP Port: Not Ready"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.glob_prefs->IsOnlineSignatureEnabled()) info_item=gtk_menu_item_new_with_label(_("Online Signature: Enabled"));
	else info_item=gtk_menu_item_new_with_label(_("Online Signature: Disabled"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.Start_time>0) {
		tempstring=g_strdup_printf("%s", CastSecondsToHM(theApp.GetUptimeSecs()).GetData() );
		info_item=gtk_menu_item_new_with_label(wxString(_("Uptime: "))+tempstring);
	}
	else info_item=gtk_menu_item_new_with_label(_("Uptime: None"));
 	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	if (theApp.serverconnect->GetCurrentServer()!=NULL) {

	info_item=gtk_menu_item_new_with_label(wxString(_("ServerName: "))+(theApp.serverconnect->GetCurrentServer()->GetListName()));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	tempstring = g_strdup_printf("%d", theApp.serverconnect->GetCurrentServer()->GetPort() );
	info_item=gtk_menu_item_new_with_label(wxString(_("ServerIP: "))+(theApp.serverconnect->GetCurrentServer()->GetFullIP())+wxString(" : ")+tempstring );
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	}
	else {
	info_item=gtk_menu_item_new_with_label(_("ServerName: Not Connected"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);
	info_item=gtk_menu_item_new_with_label(_("ServerIP: Not Connected"));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	}

	tempstring = g_strdup_printf("%d", theApp.sharedfiles->GetCount() );
	info_item=gtk_menu_item_new_with_label(wxString(_("Shared Files: "))+tempstring);
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	tempstring = g_strdup_printf("%d", theApp.uploadqueue->GetWaitingUserCount() );
	info_item=gtk_menu_item_new_with_label(wxString(_("Queued Clients: "))+tempstring);
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	tempstring = g_strdup_printf("%.2f", ((float)(theApp.stat_sessionReceivedBytes+theApp.glob_prefs->GetTotalDownloaded()) / 1073741824) );
	info_item=gtk_menu_item_new_with_label(wxString(_("Total DL: "))+tempstring+wxString(_(" GB")));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);

	tempstring = g_strdup_printf("%.2f", ((float)(theApp.stat_sessionSentBytes+theApp.glob_prefs->GetTotalUploaded()) / 1073741824));
	info_item=gtk_menu_item_new_with_label(wxString(_("Total UP: "))+tempstring+wxString(_(" GB")));
	gtk_container_add (GTK_CONTAINER (info_menu), info_item);


	//main menu
	status_menu = gtk_menu_new();
	gtk_menu_set_title(GTK_MENU(status_menu),_("aMule Tray Menu"));

	//first item, not linked, only to show version and speed
	item=gtk_menu_item_new_with_label(label);
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//personal infos item, not linked, only to show them
	item=gtk_menu_item_new_with_label(_("Personal Infos"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),info_menu);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//download speed submenu
	down_speed=gtk_menu_new();

	temp=_("Unlimited");
	dl_item=gtk_menu_item_new_with_label(temp);
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", 0 , NULL);
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);

	tempspeed=max_dl_speed;
	temp = g_strdup_printf("%d", tempspeed );
	dl_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", temp , NULL);
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);

	tempspeed=(max_dl_speed/5)*4;
	temp = g_strdup_printf("%d", tempspeed );
	dl_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);

	tempspeed=(max_dl_speed/5)*3;
	temp = g_strdup_printf("%d", tempspeed );
	dl_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);

	tempspeed=(max_dl_speed/5)*2;
	temp = g_strdup_printf("%d", tempspeed );
	dl_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);

	tempspeed=(max_dl_speed/5);
	temp = g_strdup_printf("%d", tempspeed );
	dl_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (down_speed), dl_item);
	gtk_object_set_data_full(GTK_OBJECT(dl_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(dl_item), "activate",GTK_SIGNAL_FUNC (set_dl_speed),dl_item);


	//upload speed submenu
	up_speed=gtk_menu_new();

	temp=_("Unlimited");
	up_item=gtk_menu_item_new_with_label(temp);
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", 0 , NULL);
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	tempspeed=max_up_speed;
	temp = g_strdup_printf("%d", tempspeed );
	up_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", temp, NULL);
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	tempspeed=(max_up_speed/5)*4;
	temp = g_strdup_printf("%d", tempspeed );
	up_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	tempspeed=(max_up_speed/5)*3;
	temp = g_strdup_printf("%d", tempspeed );
	up_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	tempspeed=(max_up_speed/5)*2;
	temp = g_strdup_printf("%d", tempspeed );
	up_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	tempspeed=(max_up_speed/5);
	temp = g_strdup_printf("%d", tempspeed );
	up_item=gtk_menu_item_new_with_label(wxString(temp)+_(" kb/s"));
	gtk_container_add (GTK_CONTAINER (up_speed), up_item);
	gtk_object_set_data_full(GTK_OBJECT(up_item), "label", temp, NULL);
	gtk_signal_connect (GTK_OBJECT(up_item), "activate",GTK_SIGNAL_FUNC (set_up_speed),up_item);

	if (theApp.amuledlg->IsShown()) {
		//hide item
		item=gtk_menu_item_new_with_label(_("Hide"));
		gtk_container_add (GTK_CONTAINER (status_menu), item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (do_hide),NULL);
	} else {
		//show item
		item=gtk_menu_item_new_with_label(_("Show"));
		gtk_container_add (GTK_CONTAINER (status_menu), item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (do_show),NULL);
	}

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//sets max speed
	item=gtk_menu_item_new_with_label(_("All To Max Speed"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (set_all_max),NULL);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//sets min speed
	item=gtk_menu_item_new_with_label(_("All To Min Speed"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (set_all_min),NULL);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//Download Speed item
	item=gtk_menu_item_new_with_label(_("Download Limit"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),down_speed);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//Upload Speed item
	item=gtk_menu_item_new_with_label(_("Upload Limit"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),up_speed);

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	if (theApp.serverconnect->IsConnected()) {
		//Disconnection Speed item
		item=gtk_menu_item_new_with_label(_("Disconnect from server"));
		gtk_container_add (GTK_CONTAINER (status_menu), item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (disconnect),NULL);
	} else {
		//Connect item
		item=gtk_menu_item_new_with_label(_("Connect to any server"));
		gtk_container_add (GTK_CONTAINER (status_menu), item);
		gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (connect_any_server),NULL);
	}

	//separator
	item=gtk_menu_item_new();
	gtk_container_add (GTK_CONTAINER (status_menu), item);

	//Exit item
	item=gtk_menu_item_new_with_label(_("Exit"));
	gtk_container_add (GTK_CONTAINER (status_menu), item);
	gtk_signal_connect (GTK_OBJECT (item), "activate",GTK_SIGNAL_FUNC (close_amule),NULL);


	//when the menu is popped-down, you need to destroy it
	gtk_signal_connect (GTK_OBJECT (status_menu), "selection-done",
			  GTK_SIGNAL_FUNC (gtk_widget_destroy), &status_menu);

	//gtk_signal_connect (GTK_OBJECT (info_menu), "leave_notify_event",
	//		  GTK_SIGNAL_FUNC (gtk_widget_destroy), &info_menu);

	//finalization
	gtk_widget_show_all (status_menu);
	gtk_menu_popup (GTK_MENU(status_menu), NULL, NULL,NULL, NULL,event->button, event->time);

	return TRUE;
}


static gboolean tray_clicked (GtkWidget *event_box, GdkEventButton *event, gpointer data) {

	//mouse wheel or middle click + left double click
	if ( (event->button == 1 && event->type == GDK_2BUTTON_PRESS) ||event->button == 2) {
		showgui();
		return true;
	}

	//mouse right click
	if (event->button == 3) {
		return tray_menu (event_box, event, data);
	}

	// Tell calling code that we have not handled this event; pass it on.
	return false;
}

CSysTray::CSysTray(wxWindow* _parent,int _desktopMode, const wxString& title)
{
  static GtkWidget *eventbox;
  gdk_rgb_init();

  /* argH!!! */
  m_sDimensions.cx=16;
  m_sDimensions.cy=16;
  m_nSpacingWidth=1;
  m_nNumBars=1;
  m_nMaxVal=100;
  m_pLimits=NULL;
  m_pColors=NULL;

  parent=_parent;
  desktopMode=_desktopMode;
  if(desktopMode==4) {
    // not wanted, so don't show it.
   return;
  }

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  bool use_legacy=false;

  // case 2 and 3 are KDE/other legacy system
  if(desktopMode==2 || desktopMode==3) {
    use_legacy=true;
  }

  if(use_legacy) {
    status_docklet=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(status_docklet), title);
    gtk_window_set_wmclass(GTK_WINDOW(status_docklet),"amule_StatusDocklet","aMule");
    gtk_widget_set_usize(status_docklet,22,22);
  } else {
    status_docklet=GTK_WIDGET(egg_tray_icon_new(_("aMule for Linux")));
      if(status_docklet==NULL) {
      printf("**** WARNING: Can't create status docklet. Systray will not be created.\n");
      desktopMode=4;
      return;
    }
  }
  gtk_widget_realize(status_docklet);

  gtk_signal_connect(GTK_OBJECT(status_docklet),"destroy", GTK_SIGNAL_FUNC(gtk_widget_destroyed),&status_docklet);


  // set image
  GtkStyle *style;
  style = gtk_widget_get_style(status_docklet);
  GdkBitmap* mask=NULL;
  GdkPixmap* img=gdk_pixmap_create_from_xpm_d(status_docklet->window,&mask,&style->bg[GTK_STATE_NORMAL],mule_Tr_grey_ico);
  status_image=gtk_pixmap_new(img,mask);

  eventbox = gtk_event_box_new ();
  gtk_widget_show (eventbox);
  gtk_container_add (GTK_CONTAINER (eventbox), status_image);
  gtk_container_add (GTK_CONTAINER (status_docklet), eventbox);
  gtk_signal_connect (GTK_OBJECT (eventbox), "button_press_event",GTK_SIGNAL_FUNC (tray_clicked), NULL );
  gtk_signal_connect (GTK_OBJECT(status_image),"destroy",  GTK_SIGNAL_FUNC(gtk_widget_destroyed),&status_image);

  // set tooltips
  status_tooltips=gtk_tooltips_new();
  gtk_tooltips_enable(status_tooltips);
  gtk_tooltips_set_tip(status_tooltips,status_docklet,_("aMule for Linux"),"blind text");

  // finalization
  gtk_widget_show(status_image);
  if(use_legacy) {
    setupProperties();
  }
  gtk_widget_show(GTK_WIDGET(status_docklet));
  gtk_widget_show_all (GTK_WIDGET (status_docklet));
  
}

void CSysTray::setupProperties()
{
  GdkWindow* window=status_docklet->window;

  glong data[1]; 

  GdkAtom kwm_dockwindow_atom;
  GdkAtom kde_net_system_tray_window_for_atom;
  
  kwm_dockwindow_atom = gdk_atom_intern("KWM_DOCKWINDOW", FALSE);
  kde_net_system_tray_window_for_atom = gdk_atom_intern("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", FALSE);
  
  /* This is the old KDE 1.0 and GNOME 1.2 way... */
  data[0] = TRUE;
  gdk_property_change(window, kwm_dockwindow_atom, 
		      kwm_dockwindow_atom, 32,
		      GDK_PROP_MODE_REPLACE, (guchar *)&data, 1);
  
  /* This is needed to support KDE 2.0 */
  /* can be set to zero or the root win I think */
  data[0] = 0;
  gdk_property_change(window, kde_net_system_tray_window_for_atom, 
		      (GdkAtom)XA_WINDOW, 32,
		      GDK_PROP_MODE_REPLACE, (guchar *)&data, 1);
}

void CSysTray::Show(const wxChar* caption,int nMsgType,DWORD dwTimeToShow,DWORD dwTimeToStay,DWORD dwTimeTOHide)
{
  if(desktopMode==4)
    return;

  if(status_docklet==NULL)
    return;

  /* this isn't exactly true. notifier must be a widget */
  gtk_tooltips_set_tip(status_tooltips,status_docklet,caption,NULL);
}


void CSysTray::TraySetToolTip(char* data)
{
  if(desktopMode==4)
    return;

  if(status_docklet==NULL)
    return;

  gtk_tooltips_set_tip(status_tooltips,status_docklet,data,NULL);
}

COLORREF CSysTray::GetMeterColor(int nLevel)
// it the nLevel is greater than the values defined in m_pLimits the last value in the array is used
{// begin GetMeterColor

	for(int i = 0;i < m_nEntries;i++)
	{
		if(nLevel <= m_pLimits[i])
		{
			return m_pColors[i];
		}
	}
	// default to the last entry

	return m_pColors[m_nEntries-1];

}// end GetMeterColor

void CSysTray::DrawIconMeter(GdkPixmap* pix,GdkBitmap* mask,int nLevel,int nPos)
{
  GdkGC* gc=gdk_gc_new(pix);

  gdk_rgb_gc_set_background(gc,0);
  // border color is black :)
  gdk_rgb_gc_set_foreground(gc,GetMeterColor(nLevel));
  gdk_draw_rectangle(pix,gc,1,((m_sDimensions.cx-1)/m_nNumBars)*nPos+m_nSpacingWidth,m_sDimensions.cy-((nLevel*(m_sDimensions.cy-1)/m_nMaxVal)+1),((m_sDimensions.cx-1)/m_nNumBars)*(nPos+1)+1,m_sDimensions.cy);
  // then draw to mask (look! it must be initialised even if it is not used!)
  GdkGC* newgc=gdk_gc_new(mask);
  gdk_rgb_gc_set_foreground(newgc,0x0);
  gdk_draw_rectangle(mask,newgc,TRUE,0,0,22,22);

  if(nLevel>0) {
    gdk_rgb_gc_set_foreground(newgc,0xffffff);
    gdk_draw_rectangle(mask,newgc,1,m_sDimensions.cx-2,m_sDimensions.cy-((nLevel*(m_sDimensions.cy-1)/m_nMaxVal)+1),
		       m_sDimensions.cx,m_sDimensions.cy);    
  } 
  gdk_gc_unref(newgc);
  gdk_gc_unref(gc);
}

void CSysTray::drawMeters(GdkPixmap* pix,GdkBitmap* mask,int* pBarData)
{
  if(pBarData==NULL)
    return;

  for(int i=0;i<m_nNumBars;i++) {
    DrawIconMeter(pix,mask,pBarData[i],i);
  }
}

void CSysTray::TraySetIcon(char** data,bool what,int* pVals) 
{
  if(desktopMode==4)
    return;

  if(status_image==NULL)
    return; // nothing you can do..

  GdkPixmap* oldpix,*oldbit;
  GdkPixmap* newpix,*newbit;

  /* set new */
  gtk_pixmap_get(GTK_PIXMAP(status_image),&oldpix,&oldbit);
  newpix=gdk_pixmap_create_from_xpm_d(status_docklet->window,&newbit,NULL,data);
  /* create pixmap for meters */
  GdkPixmap *meterpix=gdk_pixmap_new(status_docklet->window,22,22,-1);
  GdkBitmap* meterbit=gdk_pixmap_new(status_docklet->window,22,22,1);
  /* draw meters */
  drawMeters(meterpix,meterbit,pVals);
  /* then draw meters onto main pix */
  GdkGC* mygc=gdk_gc_new(newpix);
  gdk_gc_set_clip_mask(mygc,meterbit);
  gdk_draw_pixmap(newpix,mygc,meterpix,0,0,0,0,22,22);
  gdk_gc_set_clip_mask(mygc,NULL);
  gdk_gc_unref(mygc);
  /* finally combine masks */
  mygc=gdk_gc_new(newbit);
  gdk_gc_set_function(mygc,GDK_OR);
  gdk_draw_pixmap(newbit,mygc,meterbit,0,0,0,0,22,22);
  gdk_gc_unref(mygc);
  gdk_pixmap_unref(meterpix);
  gdk_bitmap_unref(meterbit);
  gtk_pixmap_set(GTK_PIXMAP(status_image),newpix,newbit);
 
  /* free old */
  gdk_pixmap_unref(oldpix);
  gdk_bitmap_unref(oldbit);
  /* and force repaint */
  gtk_widget_draw(status_docklet,NULL);
}

bool CSysTray::SetColorLevels(int *pLimits, COLORREF *pColors,int nEntries)
// pLimits is an array of int that contain the upper limit for the corresponding color
{// begin SetColorLevels
	// free exsisting memory
	if(m_pLimits)
		delete []m_pLimits;
	if(m_pColors)
		delete []m_pColors;
	// allocate new memory
	m_pLimits = new int[nEntries];
	m_pColors = new COLORREF[nEntries];
	// copy values
	for(int i = 0;i < nEntries;i++)
	{// begin copy
		m_pLimits[i] = pLimits[i];
		m_pColors[i] = pColors[i];
	}// end copy
	m_nEntries = nEntries;
	return true;
}// end SetColorLevels

CSysTray::~CSysTray() {

	if(m_pLimits)
		delete []m_pLimits;
	if(m_pColors)
		delete []m_pColors;
}

#endif // __WXGTK__
