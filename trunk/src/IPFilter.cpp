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


#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/intl.h>		// Needed for _
#include <wx/textfile.h>

#include "IPFilter.h"		// Interface declarations.
#include "amuleDlg.h"		// Needed for CamuleDlg
#include "Preferences.h"	// Needed for CPreferences
#include "amule.h"			// Needed for theApp

#include <wx/arrimpl.cpp> // this is a magic incantation which must be done!

WX_DEFINE_OBJARRAY(ArrayOfIPRange_Struct);

CIPFilter::CIPFilter(){
	lasthit=wxT("");
	LoadFromFile();
}

CIPFilter::~CIPFilter(){
	RemoveAllIPs();
}

void CIPFilter::Reload(){
	wxMutexLocker lock(s_IPfilter_Data_mutex);
	RemoveAllIPs();
	lasthit=wxT("");
	LoadFromFile();
}

void CIPFilter::AddBannedIPRange(uint32 IPfrom,uint32 IPto,uint8 filter, const wxString& desc){
	IPRange_Struct* newFilter=new IPRange_Struct();
	IPRange_Struct* search;
	bool inserted=false;

	newFilter->IPstart=IPfrom;
	newFilter->IPend=IPto;
	newFilter->filter=filter;
	newFilter->description=desc;

	if (iplist.GetCount()==0) iplist.Insert(newFilter,0); else {
		for (size_t i=0;i<iplist.GetCount();i++) {
			search=iplist[i];
			if (search->IPstart>IPfrom) {
				iplist.Insert(newFilter,i);
				inserted=true;
				break;
			}
		}
		if (!inserted) iplist.Add(newFilter);
	}
}

#if 0 // incomplete function
wxString Tokenize(wxString input,wxString token,int startat)
{
  wxString tmp=input.Right(input.Length()-startat);
  int pos=tmp.Find(token);
  if(pos>=0) {

  }
}
#endif

int CIPFilter::LoadFromFile(){
	wxString sbuffer2,sbuffer3,sbuffer4;
	int pos,filtercounter;
	uint32 ip1,ip2;
	uint8 filter;
	filtercounter=0;

	RemoveAllIPs();

	wxTextFile readFile(theApp.ConfigDir + wxT("ipfilter.dat"));
	if ( readFile.Exists() && readFile.Open()) {
		for (wxString sbuffer = readFile.GetFirstLine(); !readFile.Eof(); sbuffer = readFile.GetNextLine() ) {

			// ignore comments & too short lines
			if (sbuffer.GetChar(0) == '#' || sbuffer.GetChar(0) == '/' || sbuffer.Length()<5)
				continue;

			// get & test & process IP range
			pos=sbuffer.Find(wxT(","));
			if (pos==-1) continue;
			sbuffer2=sbuffer.Left(pos).Trim();
			pos=sbuffer2.Find(wxT("-"));
			if (pos==-1) continue;
			sbuffer3=sbuffer2.Left(pos).Trim();
			sbuffer4=sbuffer2.Right(sbuffer2.Length()-pos-1).Trim();

			ip1=ip2=0;
			wxString temp;
			bool skip=false;

			unsigned int s3[4];
			unsigned int s4[4];
			memset(s3,0,sizeof(s3));
			memset(s4,0,sizeof(s4));
			sscanf(unicode2char(sbuffer3.GetData()),"%d.%d.%d.%d",&s3[0],&s3[1],&s3[2],&s3[3]);
			sscanf(unicode2char(sbuffer4.GetData()),"%d.%d.%d.%d",&s4[0],&s4[1],&s4[2],&s4[3]);

			if ((s3[0]==s3[1]==s3[2]==s3[3]==0) || (s4[0]==s4[1]==s4[2]==s4[3]==0)) {
				skip=true;
			}

			for(int i=0;i<4;i++) {
				ip1|=(uint32)(s3[i]<<(8*(3-i)));
				ip2|=(uint32)(s4[i]<<(8*(3-i)));
			}

			// filter
			bool found=false;
			for(unsigned int m = pos + 1; m < sbuffer.Length(); ++m) {
			  if(sbuffer.GetChar(m)==',') {
			    pos=m;
			    found=true;
			    break;
			  }
			}
			if(!found) pos=-1;
			int pos2 = (-1);
			found=false;
			for(unsigned int m = pos + 1; m < sbuffer.Length(); ++m) {
			  if(sbuffer.GetChar(m)==',') {
			    pos2=m;
			    found=true;
			    break;
			  }
			}

			if (pos2==-1) continue;
			sbuffer2=sbuffer.Mid(pos+1,pos2-pos-1).Trim();
			filter=atoi(unicode2char(sbuffer2));

			sbuffer2=sbuffer.Right(sbuffer.Length()-pos2-1);
			if (sbuffer2.GetChar(sbuffer2.Length()-1)==10) sbuffer2.Remove(sbuffer2.Length()-1);

			// add a filter
			AddBannedIPRange(ip1,ip2,filter,sbuffer2);
			filtercounter++;

		}
		readFile.Close();
	}

	AddLogLineM(true,wxString::Format(_("Loaded ipfilter with %d IP addresses."),filtercounter));

	return filtercounter;
}

void CIPFilter::SaveToFile(){
}

void CIPFilter::RemoveAllIPs(){
	IPRange_Struct* search;
	while (iplist.GetCount()>0) {
        search=iplist[0];
		iplist.RemoveAt(0);
		delete search;
	}
}

bool CIPFilter::IsFiltered(uint32 IP2test){
	if ((iplist.GetCount()==0) || (!theApp.glob_prefs->GetIPFilterOn()) ) return false;

	//CSingleLock(&m_Mutex,TRUE); // will be unlocked on exit

	IPRange_Struct* search;
	uint32 IP2test_=(uint8)(IP2test>>24) | (uint8)(IP2test>>16)<<8 | (uint8)(IP2test>>8)<<16 | (uint8)(IP2test)<<24 ;

	uint16 lo=0;
	uint16 hi=iplist.GetCount()-1;
	uint16 mi;

	while (true) {
		mi=((hi-lo)/2) +lo;
		search=iplist[mi];
		if (search->IPstart<=IP2test_ && search->IPend>=IP2test_ ) {
			if (search->filter<theApp.glob_prefs->GetIPFilterLevel() ) {
				lasthit=search->description;
				return true;
			}
			return false;
		}
		if (lo==hi) return false;
		if (IP2test_<search->IPstart) hi=mi;
			else lo=mi+1;
	}
	return false;
}
