// This file is part of the aMule project.
//
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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/colour.h>		// Needed before wx/listctrl.h
#include <wx/event.h>		// Needed before wx/listctrl.h
#include <wx/listctrl.h>	// Needed for wxListCtrl
#include <wx/radiobut.h>
#include <wx/intl.h>		// Needed for _

#include "Wizard.h"		// Interface declarations.
#include "PPgTweaks.h"		// Needed for CPPgTweaks
#include "PPgConnection.h"	// Needed for CPPgConnection
#include "PreferencesDlg.h"	// Needed for CPreferencesDlg
#include "StatisticsDlg.h"	// Needed for CStatisticsDlg
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "CamuleAppBase.h"	// Needed for theApp
#include "Preferences.h"	// Needed for CPreferences
#include "muuli_wdr.h"		// Needed for connWizDlg

// Wizard dialog

//IMPLEMENT_DYNAMIC(Wizard, CDialog)

Wizard::Wizard(wxWindow* pParent /*=NULL*/)
: wxDialog(pParent,Wizard::IDD,_("Wizard"),wxDefaultPosition,wxDefaultSize,
wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU)
{
	m_iOS = 0;
	m_iTotalDownload = 0;

	wxSizer* content=connWizDlg(this,TRUE);
	content->Show(this,TRUE);

	Centre();

	m_provider=wxStaticCast(FindWindowById(ID_PROVIDER),wxListCtrl);
}

Wizard::~Wizard()
{
}

BEGIN_EVENT_TABLE(Wizard,wxDialog)
	EVT_BUTTON(ID_APPLY,Wizard::OnBnClickedApply)
	EVT_BUTTON(ID_CANCEL,Wizard::OnBnClickedCancel)
	EVT_RADIOBUTTON(ID_LOWDOWN,Wizard::OnBnClickedWizLowdownloadRadio)
	EVT_RADIOBUTTON(ID_MEDIUMDOWN,Wizard::OnBnClickedWizMediumdownloadRadio)
	EVT_RADIOBUTTON(ID_HIGHDOWN,Wizard::OnBnClickedWizHighdownloadRadio)
	EVT_LIST_ITEM_SELECTED(ID_PROVIDER,Wizard::OnNMClickProviders)
END_EVENT_TABLE()

// Wizard message handlers

#define GetDlgItem(a,b) wxStaticCast(FindWindowById((a)),b)
#define IsDlgButtonChecked(a) GetDlgItem(a,wxRadioButton)->GetValue()

void Wizard::OnBnClickedApply(wxEvent& evt)
{
//	char buffer[510]; commented cause it wasnt used (falso)
	int download = GetDlgItem(ID_TRUEDOWNLOAD,wxSpinCtrl)->GetValue();
	int upload = GetDlgItem(ID_TRUEUPLOAD,wxSpinCtrl)->GetValue();
	if(IsDlgButtonChecked(ID_KBITS)==1) {
		upload/=8;
		download/=8;
	}
	if( upload > 0 && download > 0 ) {
		app_prefs->prefs->maxupload = (uint16)(upload*.8);
		if( upload < 4 && download > upload*3 ) {
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload * 3;
			download = upload*3;
		}
		if( upload < 10 && download > upload*4 ) {
			app_prefs->prefs->maxdownload = app_prefs->prefs->maxupload * 4;
			download = upload*4;
		} else {
			app_prefs->prefs->maxdownload = (uint16)(download*.9);
		}
		app_prefs->prefs->maxGraphDownloadRate = download;
		app_prefs->prefs->maxGraphUploadRate = upload;
		theApp.amuledlg->statisticswnd->SetARange(false,app_prefs->prefs->maxGraphUploadRate);
		theApp.amuledlg->statisticswnd->SetARange(true,app_prefs->prefs->maxGraphDownloadRate);

		if( m_iOS == 1 ) {
			app_prefs->prefs->maxconnections = 50;
		} else {
			if( upload <= 7 ) {
				app_prefs->prefs->maxconnections = 80;
			} else if( upload < 12 ) {
				app_prefs->prefs->maxconnections = 200;
			} else if( upload < 25 ) {
				app_prefs->prefs->maxconnections = 400;
			} else if( upload < 37 ) {
				app_prefs->prefs->maxconnections = 600;
			} else {
				app_prefs->prefs->maxconnections = 800;
			}
		}
		if( m_iOS == 1 ) {
			download = download/2;
		}
		if( download <= 7 ) {
			switch( m_iTotalDownload ) {
				case 0:
					app_prefs->prefs->maxsourceperfile = 100;
					break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 60;
					break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 40;
					break;
			}
		} else if( download < 62 ) {
			switch( m_iTotalDownload ) {
				case 0:
					app_prefs->prefs->maxsourceperfile = 300;
					break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 200;
					break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 100;
					break;
			}
		} else if( download < 187 ) {
			switch( m_iTotalDownload ) {
				case 0:
					app_prefs->prefs->maxsourceperfile = 500;
					break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 400;
					break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 350;
					break;
			}
		} else if( download <= 312 ) {
			switch( m_iTotalDownload ) {
				case 0:
					app_prefs->prefs->maxsourceperfile = 800;
					break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 600;
					break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 400;
					break;
			}
		} else {
			switch( m_iTotalDownload ) {
				case 0:
					app_prefs->prefs->maxsourceperfile = 1000;
					break;
				case 1:
					app_prefs->prefs->maxsourceperfile = 750;
					break;
				case 2:
					app_prefs->prefs->maxsourceperfile = 500;
					break;
			}
		}
	}
#ifndef DISABLE_OLDPREFS
	theApp.amuledlg->preferenceswnd->m_wndConnection->LoadSettings();
	theApp.amuledlg->preferenceswnd->m_wndTweaks->LoadSettings();
#endif
	printf("TODO: TWEAKS missing \n");
	EndModal(0);
}

void Wizard::OnBnClickedCancel(wxEvent& evt)
{
	EndModal(1);
}

void Wizard::OnBnClickedWizRadioOsNtxp(wxEvent& evt)
{
	m_iOS = 0;
}

void Wizard::OnBnClickedWizRadioUs98me(wxEvent& evt)
{
	m_iOS = 1;
}

void Wizard::OnBnClickedWizLowdownloadRadio(wxEvent& evt)
{
	m_iTotalDownload = 0;
}

void Wizard::OnBnClickedWizMediumdownloadRadio(wxEvent& evt)
{
	m_iTotalDownload = 1;
}

void Wizard::OnBnClickedWizHighdownloadRadio(wxEvent& evt)
{
	m_iTotalDownload = 2;
}

void Wizard::OnBnClickedWizResetButton(wxEvent& evt)
{
	CString strBuffer;
	strBuffer.Format("%i", 0);
	GetDlgItem(ID_TRUEDOWNLOAD,wxSpinCtrl)->SetValue(strBuffer); 
	GetDlgItem(ID_TRUEUPLOAD,wxSpinCtrl)->SetValue(strBuffer); 
}

#define CheckDlgButton(a,b) GetDlgItem(a,wxRadioButton)->SetValue(b)

bool Wizard::OnInitDialog()
{
	CheckDlgButton(ID_LOWDOWN,1);
	CheckDlgButton(ID_KBITS,1);

	GetDlgItem(ID_TRUEDOWNLOAD,wxSpinCtrl)->SetValue(app_prefs->prefs->maxGraphDownloadRate);
	GetDlgItem(ID_TRUEUPLOAD,wxSpinCtrl)->SetValue(app_prefs->prefs->maxGraphUploadRate);

	m_provider->InsertColumn(0,_("Connection"),wxLIST_FORMAT_LEFT, 160);
	m_provider->InsertColumn(1,_("Down (kbit/s)"),wxLIST_FORMAT_LEFT, 90);
	m_provider->InsertColumn(2,_("Up (kbit/s)"),wxLIST_FORMAT_LEFT, 90);

	int i=0;
	i=m_provider->InsertItem(0,_("Custom"));m_provider->SetItem(0,1,_("(enter below!)"));m_provider->SetItem(0,2,_("(enter below!)"));
	i=m_provider->InsertItem(1,_("56-k Modem"));m_provider->SetItem(1,1,"56");m_provider->SetItem(1,2,"56");
	i=m_provider->InsertItem(2,_("ISDN"));m_provider->SetItem(2,1,"64");m_provider->SetItem(2,2,"64");
	i=m_provider->InsertItem(3,_("ISDN 2x"));m_provider->SetItem(3,1,"128");m_provider->SetItem(3,2,"128");
	i=m_provider->InsertItem(4 ,_("xDSL"));m_provider->SetItem(4,1,"256");m_provider->SetItem(4,2,"128");
	i=m_provider->InsertItem(5,_("xDSL"));m_provider->SetItem(5,1,"384");m_provider->SetItem(5,2,"90");
	i=m_provider->InsertItem(6,_("xDSL"));m_provider->SetItem(6,1,"512");m_provider->SetItem(6,2,"90");
	i=m_provider->InsertItem(7 ,_("xDSL"));m_provider->SetItem(7,1,"512");m_provider->SetItem(7,2,"128");
	i=m_provider->InsertItem(8,_("xDSL"));m_provider->SetItem(8,1,"640");m_provider->SetItem(8,2,"90");
	i=m_provider->InsertItem(9,_("xDSL"));m_provider->SetItem(9,1,"768");m_provider->SetItem(9,2,"128");
	i=m_provider->InsertItem(10,_("xDSL"));m_provider->SetItem(10,1,"1024");m_provider->SetItem(10,2,"128");
	i=m_provider->InsertItem(11,_("xDSL"));m_provider->SetItem(11,1,"1024");m_provider->SetItem(11,2,"256");
	i=m_provider->InsertItem(12,_("xDSL"));m_provider->SetItem(12,1,"1500");m_provider->SetItem(12,2,"192");
	i=m_provider->InsertItem(13,_("xDSL"));m_provider->SetItem(13,1,"1600");m_provider->SetItem(13,2,"90");
	i=m_provider->InsertItem(14,_("xDSL"));m_provider->SetItem(14,1,"2000");m_provider->SetItem(14,2,"300");
	i=m_provider->InsertItem(15,_("Cable"));m_provider->SetItem(15,1,"187");m_provider->SetItem(15,2,"32");
	i=m_provider->InsertItem(16,_("Cable"));m_provider->SetItem(16,1,"187");m_provider->SetItem(16,2,"64");
	i=m_provider->InsertItem(17,_("T1"));m_provider->SetItem(17,1,"1500");m_provider->SetItem(17,2,"1500");
	i=m_provider->InsertItem(18,_("T3+"));m_provider->SetItem(18,1,"44 Mbps");m_provider->SetItem(18,2,"44 Mbps");
	i=m_provider->InsertItem(19,_("100 Mbits"));m_provider->SetItem(19,1,"100 Mbps");m_provider->SetItem(19,2,"100 Mbps");
	i=m_provider->InsertItem(20,_("155 Mbits (ATM)"));m_provider->SetItem(20,1,"155 Mbps");m_provider->SetItem(20,2,"155 Mbps");
  
	return TRUE;
}

void Wizard::Localize(void){
}

void Wizard::SetCustomItemsActivation()
{
	bool active=(m_provider->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED)<1);
	GetDlgItem(ID_TRUEUPLOAD,wxSpinCtrl)->Enable(active);
	GetDlgItem(ID_TRUEDOWNLOAD,wxSpinCtrl)->Enable(active);
	GetDlgItem(ID_KBITS,wxRadioButton )->Enable(active);
	GetDlgItem(ID_KBYTES,wxRadioButton )->Enable(active);
}

void Wizard::OnNMClickProviders(wxListEvent& evt)
{
	SetCustomItemsActivation();
	int up,down;
	int cursel=m_provider->GetNextItem(-1,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);
	switch (cursel) {
		case 1 : down=56;up=56; break;
		case 2 : down=64;up=64; break;
		case 3 : down=128;up=128; break;
		case 4 : down=256;up=128; break;
		case 5 : down=384;up=90; break;
		case 6 : down=512;up=90; break;
		case 7 : down=512;up=128; break;
		case 8 : down=640;up=90; break;
		case 9 : down=768;up=128; break;
		case 10 : down=1024;up=128; break;
		case 11 : down=1024;up=256; break;
		case 12 : down=1500;up=192; break;
		case 13: down=1600;up=90; break;
		case 14: down=2000;up=300; break;
		case 15: down=187;up=32; break;
		case 16: down=187;up=64; break;
		case 17: down=1500;up=1500; break;
		case 18: down=44000;up=44000; break;
		case 19: down=100000;up=100000; break;
		case 20: down=155000;up=155000; break;
    
		default: return;
	}
	int tempdown = down;
	GetDlgItem(ID_TRUEDOWNLOAD,wxSpinCtrl)->SetValue(tempdown);
	int tempup = up;
	GetDlgItem(ID_TRUEUPLOAD,wxSpinCtrl)->SetValue(tempup);
	CheckDlgButton(ID_KBITS,1);
	CheckDlgButton(ID_KBYTES,0);
}
