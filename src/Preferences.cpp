// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


#ifdef HAVE_CONFIG_H
#include "config.h"		// Needed for PACKAGE_NAME and PACKAGE_STRING
#endif

#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include "otherfunctions.h"	// Needed for atoll
#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif
#include "PrefsUnifiedDlg.h"			// Needed for Rse
#include "opcodes.h"		// Needed for PREFFILE_VERSION
#include "color.h"			// Needed for RGB

#include <wx/config.h>
#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!
#include "amule.h"

WX_DEFINE_OBJARRAY(ArrayOfCategory_Struct);

/// new implementation
CPreferences::CPreferences()
{
	srand((uint32)time(0)); // we need random numbers sometimes

	prefs = new Preferences_Struct;
	memset(prefs,0,sizeof(Preferences_Struct));
	prefsExt=new Preferences_Ext_Struct;
	memset(prefsExt,0,sizeof(Preferences_Ext_Struct));

	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);

	// load preferences.dat or set standart values
	CString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	FILE* preffile = fopen(unicode2char(fullpath),"rb");

	if (!preffile) {
		SetStandartValues();
	} else {
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile)) {
			SetStandartValues();
		}
		memcpy(&prefs->userhash,&prefsExt->userhash,16);
		prefs->EmuleWindowPlacement=prefsExt->EmuleWindowPlacement;
		fclose(preffile);
		memcpy(&userhash,&prefs->userhash,16);
		prefs->smartidstate=0;
	}

	PrefsUnifiedDlg::BuildItemList(prefs, theApp.ConfigDir);

	LoadPreferences();

	// shared directories
	wxTextFile sdirfile(theApp.ConfigDir + wxT("shareddir.dat"));
	if( sdirfile.Exists() && sdirfile.Open() ) {
		if (sdirfile.GetLineCount()) {
			for (wxString str = sdirfile.GetFirstLine(); !sdirfile.Eof(); str = sdirfile.GetNextLine() ) {
    				shareddir_list.Add(str);
			}
		}
		sdirfile.Close();
	}

	// serverlist adresses
	wxTextFile slistfile(theApp.ConfigDir + wxT("addresses.dat"));
	if ( slistfile.Exists() && slistfile.Open()) {
		if (slistfile.GetLineCount()) {
			for (wxString str = slistfile.GetFirstLine(); !slistfile.Eof(); str = slistfile.GetNextLine() ) {
    				adresses_list.Append(new CString(str));
			}
		}
		slistfile.Close();
	}

	userhash[5] = 14;
	userhash[14] = 111;

	if (!::wxDirExists(char2unicode(GetIncomingDir()))) {
		::wxMkdir(char2unicode(GetIncomingDir()),0777);
	}

	if (!::wxDirExists(char2unicode(GetTempDir()))) {
		::wxMkdir(char2unicode(GetTempDir()),0777);
	}

	if (((int*)prefs->userhash)[0] == 0 && ((int*)prefs->userhash)[1] == 0 && ((int*)prefs->userhash)[2] == 0 && ((int*)prefs->userhash)[3] == 0) {
		CreateUserHash();
	}
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();
	md4cpy(&prefs->userhash,&userhash);
	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;
	defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;
	defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	prefs->EmuleWindowPlacement=defaultWPM;
	prefs->versioncheckLastAutomatic=0;
	Save();
}

uint16 CPreferences::GetMaxDownload()
{
	return prefs->maxdownload;
}

bool CPreferences::Save()
{
	CString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	bool error = false;

	FILE* preffile = fopen(unicode2char(fullpath),"wb");
	prefsExt->version = PREFFILE_VERSION;

	if (preffile) {
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=prefs->EmuleWindowPlacement;
		memcpy(&prefsExt->userhash,&prefs->userhash,16);
		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		fclose(preffile);
	} else {
		error = true;
	}

	SavePreferences();

	wxString shareddir(theApp.ConfigDir + wxT("shareddir.dat"));

	wxRemoveFile(shareddir);

	wxTextFile sdirfile(shareddir);

	if(sdirfile.Create()) {
		for(unsigned int ii = 0; ii < shareddir_list.GetCount(); ++ii) {
			sdirfile.AddLine(shareddir_list[ii]);
		}
		sdirfile.Write(),
		sdirfile.Close();
	} else {
		error = true;
	}

	return error;
}

void CPreferences::CreateUserHash()
{
	for (int i = 0;i != 8; i++) {
		uint16	random = rand();
		memcpy(&userhash[i*2],&random,2);
	}
	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int32 CPreferences::GetColumnWidth(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnWidths[index];
	case tableUpload:
		return prefs->uploadColumnWidths[index];
	case tableQueue:
		return prefs->queueColumnWidths[index];
	case tableSearch:
		return prefs->searchColumnWidths[index];
	case tableShared:
		return prefs->sharedColumnWidths[index];
	case tableServer:
		return prefs->serverColumnWidths[index];
	case tableClientList:
		return prefs->clientListColumnWidths[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnWidth(Table t, int index, int32 width)
{
	switch(t) {
	case tableDownload:
		prefs->downloadColumnWidths[index] = width;
		break;
	case tableUpload:
		prefs->uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		prefs->queueColumnWidths[index] = width;
		break;
	case tableSearch:
		prefs->searchColumnWidths[index] = width;
		break;
	case tableShared:
		prefs->sharedColumnWidths[index] = width;
		break;
	case tableServer:
		prefs->serverColumnWidths[index] = width;
		break;
	case tableClientList:
		prefs->clientListColumnWidths[index] = width;
		break;
	case tableNone:
	default:
		break;
	}
}

bool CPreferences::GetColumnHidden(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnHidden[index];
	case tableUpload:
		return prefs->uploadColumnHidden[index];
	case tableQueue:
		return prefs->queueColumnHidden[index];
	case tableSearch:
		return prefs->searchColumnHidden[index];
	case tableShared:
		return prefs->sharedColumnHidden[index];
	case tableServer:
		return prefs->serverColumnHidden[index];
	case tableClientList:
		return prefs->clientListColumnHidden[index];
	case tableNone:
	default:
		return FALSE;
	}
}

void CPreferences::SetColumnHidden(Table t, int index, bool bHidden)
{
	switch(t) {
	case tableDownload:
		prefs->downloadColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		prefs->uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		prefs->queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		prefs->searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		prefs->sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		prefs->serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		prefs->clientListColumnHidden[index] = bHidden;
		break;
	case tableNone:
	default:
		break;
	}
}

int32 CPreferences::GetColumnOrder(Table t, int index) const
{
	switch(t) {
	case tableDownload:
		return prefs->downloadColumnOrder[index];
	case tableUpload:
		return prefs->uploadColumnOrder[index];
	case tableQueue:
		return prefs->queueColumnOrder[index];
	case tableSearch:
		return prefs->searchColumnOrder[index];
	case tableShared:
		return prefs->sharedColumnOrder[index];
	case tableServer:
		return prefs->serverColumnOrder[index];
	case tableClientList:
		return prefs->clientListColumnOrder[index];
	case tableNone:
	default:
		return 0;
	}
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder)
{
	switch(t) {
	case tableDownload:
		memcpy(prefs->downloadColumnOrder, piOrder, sizeof(prefs->downloadColumnOrder));
		break;
	case tableUpload:
		memcpy(prefs->uploadColumnOrder, piOrder, sizeof(prefs->uploadColumnOrder));
		break;
	case tableQueue:
		memcpy(prefs->queueColumnOrder, piOrder, sizeof(prefs->queueColumnOrder));
		break;
	case tableSearch:
		memcpy(prefs->searchColumnOrder, piOrder, sizeof(prefs->searchColumnOrder));
		break;
	case tableShared:
		memcpy(prefs->sharedColumnOrder, piOrder, sizeof(prefs->sharedColumnOrder));
		break;
	case tableServer:
		memcpy(prefs->serverColumnOrder, piOrder, sizeof(prefs->serverColumnOrder));
		break;
	case tableClientList:
		memcpy(prefs->clientListColumnOrder, piOrder, sizeof(prefs->clientListColumnOrder));
		break;
	case tableNone:
	default:
		break;
	}
}

CPreferences::~CPreferences()
{

	Category_Struct* delcat;
	while (!catMap.IsEmpty()) {
		delcat=catMap[0];
		catMap.RemoveAt(0);
		delete delcat;
	}

	catMap.Clear();

	if (adresses_list.GetCount()>0) {
		// the 'true' tells wxList to do 'delete' on the nodes.
		adresses_list.DeleteContents(true);
	}

	delete prefs;
	delete prefsExt;
}

int32 CPreferences::GetRecommendedMaxConnections()
{
	int iRealMax = ::GetMaxConnections();
	if(iRealMax == -1 || iRealMax > 520) {
		return 500;
	}
	if(iRealMax < 20) {
		return iRealMax;
	}
	if(iRealMax <= 256) {
		return iRealMax - 10;
	}
	return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
	wxConfigBase* cfg = wxConfig::Get();

	cfg->Write( wxT("/eMule/AppVersion"), wxT(PACKAGE_STRING) );

	PrefsUnifiedDlg::SaveAllItems((*cfg));

	// Ensure that the changes are saved to disk.
	cfg->Flush();
}

void CPreferences::SaveCats()
{
	if ( GetCatCount() ) {
		wxConfigBase* cfg = wxConfig::Get();

		// The first category is the default one and should not be counte

		cfg->Write( wxT("/General/Count"), (long)(catMap.GetCount() - 1) );

		for ( size_t i = 1; i < catMap.GetCount(); i++ ) {
			cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

			cfg->Write( wxT("Title"), catMap[i]->title );
			cfg->Write( wxT("Incoming"), catMap[i]->incomingpath );
			cfg->Write( wxT("Comment"), catMap[i]->comment );
			cfg->Write( wxT("Color"), wxString::Format(wxT("%lu"), catMap[i]->color) );
			cfg->Write( wxT("Priority"), catMap[i]->prio );
		}
	}
}

void CPreferences::LoadPreferences()
{
	wxConfigBase* cfg = wxConfig::Get();

	PrefsUnifiedDlg::LoadAllItems( (*cfg) );

	LoadCats();
}

void CPreferences::LoadCats() {
	// default cat ... Meow! =(^.^)=
	Category_Struct* newcat=new Category_Struct;
	newcat->title[0] = 0;
	newcat->incomingpath[0] = 0;
	newcat->comment[0] = 0;
	newcat->prio=0;
	newcat->color=0;
	AddCat( newcat );

	wxConfigBase* cfg = wxConfig::Get();

	long max = cfg->Read( wxT("/General/Count"), 0l );

	for ( int i = 1; i <= max ; i++ ) {
		cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

		Category_Struct* newcat = new Category_Struct;


		sprintf(newcat->title, "%s", unicode2char( cfg->Read( wxT("Title"), wxT("") )));
		sprintf(newcat->incomingpath, "%s", unicode2char( cfg->Read( wxT("Incoming"), wxT("") )));

		MakeFoldername(newcat->incomingpath);
		sprintf(newcat->comment, "%s", unicode2char( cfg->Read( wxT("Comment"), wxT("") )));

		newcat->prio = cfg->Read( wxT("Priority"), 0l );

		wxString color = cfg->Read( wxT("Color"), wxT("0") );
		newcat->color = atoll( unicode2char(color) );

		AddCat(newcat);
		if (!wxFileName::DirExists(char2unicode(newcat->incomingpath))) {
			wxFileName::Mkdir(char2unicode(newcat->incomingpath),0777);
		}
	}
}

uint16 CPreferences::GetDefaultMaxConperFive()
{
	return MAXCONPER5SEC;
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
int32 CPreferences::GetColumnSortItem(Table t) const
{
	switch(t) {
		case tableDownload:
			return prefs->tableSortItemDownload;
		case tableUpload:
			return prefs->tableSortItemUpload;
		case tableQueue:
			return prefs->tableSortItemQueue;
		case tableSearch:
			return prefs->tableSortItemSearch;
		case tableShared:
			return prefs->tableSortItemShared;
		case tableServer:
			return prefs->tableSortItemServer;
		case tableClientList:
			return prefs->tableSortItemClientList;
		case tableNone:
		default:
			return 0;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t) const
{
	switch(t) {
		case tableDownload:
			return prefs->tableSortAscendingDownload;
		case tableUpload:
			return prefs->tableSortAscendingUpload;
		case tableQueue:
			return prefs->tableSortAscendingQueue;
		case tableSearch:
			return prefs->tableSortAscendingSearch;
		case tableShared:
			return prefs->tableSortAscendingShared;
		case tableServer:
			return prefs->tableSortAscendingServer;
		case tableClientList:
			return prefs->tableSortAscendingClientList;
		case tableNone:
		default:
			return true;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(Table t, int32 sortItem)
{
	switch(t) {
		case tableDownload:
			prefs->tableSortItemDownload = sortItem;
			break;
		case tableUpload:
			prefs->tableSortItemUpload = sortItem;
			break;
		case tableQueue:
			prefs->tableSortItemQueue = sortItem;
			break;
		case tableSearch:
			prefs->tableSortItemSearch = sortItem;
			break;
		case tableShared:
			prefs->tableSortItemShared = sortItem;
			break;
		case tableServer:
			prefs->tableSortItemServer = sortItem;
			break;
		case tableClientList:
			prefs->tableSortItemClientList = sortItem;
			break;
		case tableNone:
		default:
			break;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(Table t, bool sortAscending)
{
	switch(t) {
		case tableDownload:
			prefs->tableSortAscendingDownload = sortAscending;
			break;
		case tableUpload:
			prefs->tableSortAscendingUpload = sortAscending;
			break;
		case tableQueue:
			prefs->tableSortAscendingQueue = sortAscending;
			break;
		case tableSearch:
			prefs->tableSortAscendingSearch = sortAscending;
			break;
		case tableShared:
			prefs->tableSortAscendingShared = sortAscending;
			break;
		case tableServer:
			prefs->tableSortAscendingServer = sortAscending;
			break;
		case tableClientList:
			prefs->tableSortAscendingClientList = sortAscending;
			break;
		case tableNone:
		default:
			break;
	}
}

void CPreferences::RemoveCat(size_t index)
{
	if (index < catMap.GetCount()) {
		catMap.RemoveAt(index);
	}
}

wxString CPreferences::GetBrowser()
{
	wxString cmd;
	switch ( prefs->Browser ) {
		case 0: cmd = wxT("konqueror '%s'"); break;
		case 1: cmd = wxT("xterm -e sh -c 'mozilla %s'"); break;
		case 2: cmd = wxT("firefox '%s'"); break;
		case 3:	cmd = wxT("firebird '%s'"); break;
		case 4:	cmd = wxT("opera '%s'"); break;
		case 5: cmd = wxT("netscape '%s'"); break;
		case 6: cmd = wxT("galeon '%s'"); break;
		case 7: cmd = wxT("epiphany '%s'"); break;
		case 8: cmd = char2unicode(prefs->CustomBrowser); break;
		default:
			printf("Unable to determine selected browser!\n");
	}

	return cmd;
}
