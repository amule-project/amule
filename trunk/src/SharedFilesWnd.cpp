//this file is part of aMule
//
//Copyright (C) 2003-2004 aMule Project
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


// SharedFilesWnd.cpp : implementation file
//


#include <wx/settings.h>

#include "muuli_wdr.h"		// Needed for ID_SHFILELIST
#include "SharedFilesWnd.h"	// Interface declarations
#include "otherfunctions.h"	// Needed for CastItoXBytes
#include "KnownFileList.h"	// Needed for CKnownFileList
#include "listctrl_gen.h"	// Needed for wxODListCtrl
#include "PartFile.h"		// Needed for CPartFile
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "CamuleAppBase.h"	// Needed for theApp

// CSharedFilesWnd dialog
#define GetDlgItem(X) (wxStaticCast(wxWindow::FindWindowById((X)),wxStaticText))

BEGIN_EVENT_TABLE(CSharedFilesWnd,wxPanel)
	EVT_LIST_ITEM_SELECTED(ID_SHFILELIST,CSharedFilesWnd::OnLvnItemActivateSflist)
	EVT_BUTTON(IDC_RELOADSHAREDFILES, CSharedFilesWnd::OnBnClickedReloadsharedfiles)
END_EVENT_TABLE()

//IMPLEMENT_DYNAMIC(CSharedFilesWnd, CDialog)
CSharedFilesWnd::CSharedFilesWnd(wxWindow* pParent /*=NULL*/)
: wxPanel(pParent,CSharedFilesWnd::IDD)
{
	memset(shownFileHash,0,sizeof shownFileHash);
	wxSizer* content=sharedfilesDlg(this,TRUE);
	content->Show(this,TRUE);

	pop_bar=(wxGauge*)FindWindowByName("popbar");
	pop_baraccept=(wxGauge*)FindWindowByName("popbarAccept");
	pop_bartrans=(wxGauge*)FindWindowByName("popbarTrans");

	// can't do here. 
	//theApp.sharedfiles->SetOutputCtrl((CSharedFilesCtrl*)FindWindowByName("sharedFilesCt"));
	sharedfilesctrl=(CSharedFilesCtrl*)(FindWindowByName("sharedFilesCt"));
}

CSharedFilesWnd::~CSharedFilesWnd()
{
}

// CSharedFilesWnd message handlers
void CSharedFilesWnd::OnBnClickedReloadsharedfiles()
{
	theApp.sharedfiles->Reload(true, false);
}

void CSharedFilesWnd::Check4StatUpdate(CKnownFile* file)
{
	if (!memcmp(file->GetFileHash(),shownFileHash ,16)) ShowDetails(file);
}

void CSharedFilesWnd::OnLvnItemActivateSflist(wxListEvent& evt)
{
	long item=-1;
	#ifdef __WXMSW__
	wxListCtrl* sflist=wxStaticCast(FindWindowById(ID_SHFILELIST),wxListCtrl);
	#else
	wxODListCtrl* sflist=wxStaticCast(FindWindowById(ID_SHFILELIST),wxODListCtrl);
	#endif
	item=sflist->GetNextItem(item,wxLIST_NEXT_ALL,wxLIST_STATE_SELECTED);

	if (item != (-1) ) {
		CKnownFile* cur_file = (CKnownFile*)sflist->GetItemData(item);
		ShowDetails(cur_file);
	}
}


void CSharedFilesWnd::ShowDetails(CKnownFile* cur_file)
{
	char buffer[100];

	//pop_bartrans.SetRange32(0,theApp.knownfiles->transfered/1024);
	pop_bartrans->SetRange(theApp.knownfiles->transfered/1024);
	//pop_bartrans.SetPos(cur_file->statistic.GetTransfered()/1024);
	pop_bartrans->SetValue(cur_file->statistic.GetTransfered()/1024);
	//pop_bartrans.SetShowPercent();				
	GetDlgItem(IDC_STRANSFERED)->SetLabel(CastItoXBytes(cur_file->statistic.GetTransfered()));

	pop_bar->SetRange(theApp.knownfiles->requested);
	pop_bar->SetValue(cur_file->statistic.GetRequests());
	//pop_bar.SetShowPercent();			
	sprintf(buffer,"%u",cur_file->statistic.GetRequests());
	GetDlgItem(IDC_SREQUESTED)->SetLabel(buffer);

	sprintf(buffer,"%u",cur_file->statistic.GetAccepts());
	pop_baraccept->SetRange(theApp.knownfiles->accepted);
	pop_baraccept->SetValue(cur_file->statistic.GetAccepts());
	//pop_baraccept.SetShowPercent();
	GetDlgItem(IDC_SACCEPTED)->SetLabel(buffer);
	
	GetDlgItem(IDC_STRANSFERED2)->SetLabel(CastItoXBytes(cur_file->statistic.GetAllTimeTransfered()));

	sprintf(buffer,"%u",cur_file->statistic.GetAllTimeRequests());
	GetDlgItem(IDC_SREQUESTED2)->SetLabel(buffer);

	sprintf(buffer,"%u",cur_file->statistic.GetAllTimeAccepts());
	GetDlgItem(IDC_SACCEPTED2)->SetLabel(buffer);

	memcpy(shownFileHash,cur_file->GetFileHash(),16);

	//CString title=CString(_("Statistics"))+" ("+ cur_file->GetFileName() +")";
	//GetDlgItem(IDC_FSTATIC1)->SetWindowText( title );
	Layout();
}

void CSharedFilesWnd::Localize()
{
}
