// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Original author: Emilio Sandoz
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


// Unified handling of file I/O and dialog I/O for Preferences
// =================== Under Construction ====================

#ifdef __GNUG__
    #pragma implementation "PrefsUnifiedDlg.cpp"
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif


#include <wx/object.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/colordlg.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "amule.h"			// Needed for theApp
#include "otherfunctions.h"	// Needed for MakeFoldername
#include "PrefsUnifiedDlg.h"
#include "CTypedPtrList.h"	// Needed for CList
#include "EditServerListDlg.h"
#include "amuleDlg.h"
#include "SharedFileList.h"	// Needed for CSharedFileList
#include "StatisticsDlg.h"	// Needed for graph parameters, colors
#include "IPFilter.h"		// Needed for CIPFilter
#include "SearchList.h"
#include "DownloadQueue.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl

// The following classes are used for table-driven access to user-definable settings,
// most of which appear in the Preferences dialog.  All other items in the preference file
// (e.g. window column widths) use the same mechanism and the same table for consistency.
//
// The Rse classes defined below are used only by class PrefsUnifiedDlg; they are here in 
// a .cpp file for now because it facilitates the migration from the exiting code, which 
// is an evolving, interactive process.  Also, most (all) methods are currently defined 
// inline because it speeds up the design process to not have to edit both definition 
// and implementation (most Rse methods are virtual and called from very few places, so 
// there is actually very little price to pay in terms of code size).
//
// Note: currently all prefs are stored in a common struct which gets filled before 
// instances of the classes that use them are created.  The code here does not depend on
// this struct, and eventually it may be more advantageous to move the prefs vars into
// the classes they really belong to, as static members (which do not require an instance).


class Rse: public wxObject {  // Remembered Settings Element - root class
friend class PrefsUnifiedDlg;
friend class RseInt;
public:
	Rse(int ID, const wxString& szIniName, const wxString& szIniSection=wxT("")) 
		: id(ID), strName(szIniName), szSection(szIniSection), pctrl(NULL), wxc(wxcNone), prseLink(NULL)  {}
	Rse(const wxString& szIniName, const wxString& szIniSection=wxT("")) 
		: id(0), strName(szIniName), szSection(szIniSection), pctrl(NULL), wxc(wxcNone), prseLink(NULL)  {}
	virtual ~Rse() {}

	virtual void LoadFromFile(wxConfigBase& ini)	{}
	virtual void SaveToFile(wxConfigBase& ini)  	{}
		
	void TransferToDlg() 					
	{ 
		SetPrevValue();   
		if (id!=0)  
			SetDlgValue(); 
	}
	
	void TransferFromDlg() 					
	{ 
		if (id!=0)  
			StoreDlgValue(); 
	}
	
	virtual void SetDlgValue() 				{}
	virtual void SetPrevValue()				{}
	virtual void RestorePrevValue()			{}
	virtual void StoreDlgValue()			{}
	virtual bool WasChanged()				{ return false; }
	virtual void SetWxControl(wxControl *pc){}
		
	int Id() 								{ return id; }
	virtual int GetMemValue()				{ return 0; }
	virtual wxString GetMemStringValue()	{ return wxT(""); }
	virtual void SetMemValue(int val)		{}
	virtual int GetCtrlValue()				{ return 0; }
	virtual void SetCtrlValue(int val)		{}
	virtual void SetCtrlValue(wxString str)	{}
	virtual void SetCtrlRange(int iMin, int iMax) {}
	virtual int GetDefaultValue()			{ return 0; }
	virtual int GetPrevValue()				{ return 0; }
	virtual int GetCtrlCount()				{ wxASSERT(false); return 0;}

	void Propagate()						{ if (prseLink!=NULL) prseLink->SetCtrlValue(GetCtrlValue()); }
	void SetLink(Rse *prse)					{ prseLink = prse;}
	virtual int IdLinkedTo()				{ return 0; }
		
	virtual void SetEnabled(bool Enable) {
		((wxTextCtrl*)pctrl)->Enable(Enable);		
	}
	
protected:
	enum wxcType { wxcNone, wxcCheck, wxcSlider, wxcSpin, wxcText, wxcChoice, wxcStatic, 
					wxcButton, wxcRadioButton, wxcRadioBox };	

	Rse()  {}

	int			id;				// item ID in the prefs dlg
	wxString	strName;		// name of item in the ini file
	wxString	szSection;		// (optional) section in the ini file
	wxControl	*pctrl;			// pointer to wxControl in dlg
	wxcType		wxc;			// the type of control
	Rse			*prseLink;		// points to a linked dlg item, e.g. a dynamic label
};



class RseBool: public Rse {
// Handles all bool and pseudo-bool (int8 and uint8 used like bool) preference vars
public:
	RseBool(int ID, bool& bSetting, const wxString& szIniName, bool bDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), pbSet(&bSetting), bDef(bDefault)  {}

	virtual void LoadFromFile(wxConfigBase& ini) {
		ini.Read( wxT("/") + szSection + wxT("/") + strName, pbSet, bDef );
	}
	
	virtual void SaveToFile(wxConfigBase& ini) {
		ini.Write( wxT("/") + szSection + wxT("/") + strName, *pbSet );
	}
	
	virtual void SetDlgValue() 				{ SetCtrlValue(*pbSet);  Propagate(); }
	virtual void SetPrevValue()				{ bPrev = *pbSet; }
	virtual void RestorePrevValue() 		{ *pbSet = bPrev; }
	virtual void StoreDlgValue() 			{ *pbSet = GetCtrlValue(); }
	virtual bool WasChanged()				{ return (*pbSet != bPrev); }
	
	virtual void SetWxControl(wxControl *pc)
	{ 
		pctrl=pc;  
		wxASSERT(pctrl->IsKindOf(CLASSINFO(wxCheckBox))); 
		wxc = wxcCheck;	
	}
	
	virtual int GetCtrlValue()				
	{ 
		return ((wxCheckBox *)pctrl)->GetValue(); 
	}
	
	virtual void SetCtrlValue(int val)		
	{ 
		((wxCheckBox *)pctrl)->SetValue( val ); 
	}
	
	virtual int GetMemValue()				{ return *pbSet; }
	virtual void SetMemValue(int val)		{ *pbSet = (val != 0); }
	
	virtual int GetDefaultValue()			{ return bDef; }
	virtual int GetPrevValue()				{ return bPrev; }
	
private:
	bool	*pbSet;
	bool	bDef;
	bool	bPrev;
};



class RseInt: public Rse {
// Handles all signed and unsigned integer preference vars
public:
	RseInt(int ID, sint8& iSetting, const wxString& szIniName, sint8 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	RseInt(int ID, sint16& iSetting, const wxString& szIniName, sint16 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	RseInt(int ID, sint32& iSetting, const wxString& szIniName, sint32 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	RseInt(int ID, uint8& iSetting, const wxString& szIniName, uint8 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	RseInt(int ID, uint16& iSetting, const wxString& szIniName, uint16 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	RseInt(int ID, uint32& iSetting, const wxString& szIniName, uint32 iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}
#ifdef __WXMSW__
	RseInt(int ID, COLORREF& iSetting, const wxString& szIniName, COLORREF iDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}
#endif
//	RseInt(int ID, int& iSetting, const wxString& szIniName, int32 iDefault, const wxString& szIniSection)
//		: Rse(ID, szIniName, szIniSection), cb(sizeof(iSetting)), piSet(&iSetting), iDef(iDefault)  {}

	virtual void LoadFromFile(wxConfigBase& ini) {
		WriteMem( ini.Read( wxT("/") + szSection + wxT("/") + strName, iDef ) );
	}
	
	virtual void SaveToFile(wxConfigBase& ini) {
		ini.Write( wxT("/") + szSection + wxT("/") + strName, (long)ReadMem() );
	}
	
	virtual void RestorePrevValue() 		{ WriteMem(iPrev); }
	virtual void SetPrevValue()				{ iPrev = ReadMem(); }
	virtual void SetDlgValue() 				{ SetCtrlValue(ReadMem());  Propagate(); }
	virtual void StoreDlgValue() 			{ WriteMem(GetCtrlValue()); }
	virtual bool WasChanged()				{ return (ReadMem() != iPrev); }
	virtual int GetMemValue()				{ return ReadMem(); }
	virtual void SetMemValue(int val)		{ WriteMem(val); }
	virtual int GetDefaultValue()			{ return iDef; }
	virtual int GetPrevValue()				{ return iPrev; }
	
	virtual int GetCtrlValue()
	{
		switch (wxc) {
			case wxcChoice:		return ((wxChoice*)pctrl)->GetSelection();
			case wxcSlider:		return ((wxSlider *)pctrl)->GetValue(); 
			case wxcSpin:		return ((wxSpinCtrl *)pctrl)->GetValue(); 
			case wxcRadioBox:	return ((wxRadioBox *)pctrl)->GetSelection();
			case wxcRadioButton: {
				int val = 0;
				for (Rse* prse=this; prse!=NULL; prse=prse->prseLink) {
					if (((wxRadioButton*)(prse->pctrl))->GetValue()) {
						return val;
					} else {
						val++;
					}
				}
				return -1;  // should never get here if the buttons are set up correctly
			}
			default:	wxASSERT(false);		
		}
		return 0;  // stop the compiler from complaining about missing return from function
	}
	
	virtual void SetCtrlValue(int val) 
	{
		switch (wxc) {
			case wxcChoice:	
				((wxChoice*)pctrl)->SetSelection((val < ((wxChoice*)pctrl)->GetCount()) ? val : 1);
				break;
			case wxcSlider:	
				((wxSlider *)pctrl)->SetValue(val); 
				break;
			case wxcSpin:	
				((wxSpinCtrl*)pctrl)->SetValue(val);
				break;
			case wxcRadioBox:
				((wxRadioBox*)pctrl)->SetSelection(val);
				break;				
			case wxcRadioButton: 	
				for (Rse* prse=this; prse!=NULL; prse=prse->prseLink)
					((wxRadioButton*)(prse->pctrl))->SetValue((val-- == 0));
				break;
			default:
				wxASSERT(false);
				break;
		}
}
	
	virtual void SetCtrlRange(int iMin, int iMax) 
	{
		if (wxc==wxcSlider)
			((wxSlider *)pctrl)->SetRange(iMin, iMax); 
		else if (wxc==wxcSpin)
			((wxSpinCtrl*)pctrl)->SetRange(iMin, iMax);
		else
			wxASSERT(false);
	}
	
	virtual void SetWxControl(wxControl *pc)	
	{ 
		pctrl=pc;  
		if (pctrl->IsKindOf(CLASSINFO(wxSlider))) 
			wxc = wxcSlider; 
		else if (pctrl->IsKindOf(CLASSINFO(wxSpinCtrl))) 
			wxc = wxcSpin;
		else if (pctrl->IsKindOf(CLASSINFO(wxRadioButton))) 
			wxc = wxcRadioButton;
		else if (pctrl->IsKindOf(CLASSINFO(wxChoice))) 
			wxc = wxcChoice;
		else if (pctrl->IsKindOf(CLASSINFO(wxRadioBox)))
			wxc = wxcRadioBox;
		wxASSERT(wxc != wxcNone);
	}

	int GetCtrlCount() {
		if (wxc==wxcChoice) {
			return ((wxChoice *)pctrl)->GetCount();
		} else {
			wxASSERT(false);
			return 0;
		}
	}
	
private:
	void WriteMem(int32 i)
	{
		switch (cb) {
			case sizeof(int32): *(int32*)piSet = i;  break;
			case sizeof(int16): *(int16*)piSet = i;  break;
			case sizeof(int8):	*(int8*)piSet = i;   break;
			default:			wxASSERT(false);	 break;  
				// if this assert is triggered by a "int" on 64bit system, then
				// it's probably time to reduce the size of the underlying pref var
		}
	}

	int32 ReadMem() 
	{
		switch (cb) {
			case sizeof(int32):  return *(int32*)piSet;
			case sizeof(int16):  return *(int16*)piSet;
			case sizeof(int8):	 return *(int8*)piSet;
			default:			 wxASSERT(false);	  return 0;
		}
	}

	int			cb;
	void		*piSet;
	int32		iDef;
	int32		iPrev;
};



class RseDynLabel: public Rse {
// A special item type that creates a dynamic label for a slider or similar control.  No file I/O.
// It can appear anywhere in listRse, but it is recommended to put it immediately after
// the control it is linked to, for reasons of code maintainability.

public:
	RseDynLabel(int ID_label, 		// dlg ID of wxStaticText
			  int ID_ctrl, 			// dlg ID of linked control
			  int iMultiplier, 		// ctrl val * iMultiplier = output in string
			  wxString strSeveral, 	// format string to use for val*multiplier > 1
			  wxString strOne,	 	//							ditto == 1 ("" if not used)
			  wxString strZero)		//							ditto == 0 ("" if not used)
		: Rse(ID_label, wxT("label")), idLinkedTo(ID_ctrl), iMult(iMultiplier), 
			strSev(strSeveral), str1(strOne), str0(strZero)  {}
				
	virtual void SetWxControl(wxControl *pc)
	{ 
		pctrl=pc;  
		wxASSERT(pctrl->IsKindOf(CLASSINFO(wxStaticText))); 
		wxc = wxcStatic;	
	}

	virtual void SetCtrlValue(int val)		
	{
		int i = val * iMult;
		wxString str;
		
		if (i == 0) 
			str = (str0.IsEmpty()) ? str0 : strSev;
		else if (i == 1)
			str = (str1.IsEmpty()) ? str1 : strSev;
		else
			str = strSev;
		
		((wxStaticText*)pctrl)->SetLabel(wxString::Format(str, i));
	}
	
	virtual int IdLinkedTo()	{ return idLinkedTo; }
	
private:
	int	idLinkedTo;
	int iMult;
	wxString strSev;
	wxString str1;
	wxString str0;
};



class RseCounter: public Rse {
// Used for long-term statistics counters, e.g. for total bytes received; no dlg use
public:
	RseCounter(uint64& iSetting, const wxString&  szIniName, const wxString& szIniSection)
		: Rse(szIniName, szIniSection), piSet(&iSetting)  {}

	virtual void LoadFromFile(wxConfigBase& ini) { 
		wxString buffer;

		buffer = ini.Read( wxT("/") + szSection + wxT("/") + strName, wxT("0"));
		
		*piSet = atoll(unicode2char(buffer));
	}
	virtual void SaveToFile(wxConfigBase& ini)		{ 
		wxString str = wxString::Format(wxT("%llu"),(unsigned long long)*piSet);
		
		ini.Write( wxT("/") + szSection + wxT("/") + strName, str);
	}
	
private:
	uint64	*piSet;
};



class RseString: public Rse {
// all settable strings, e.g. name of video viewer
public:
	RseString(int ID, wxString& pchSetting, const wxString& szIniName, const wxString& szDefault, const wxString& szIniSection)
		: Rse(ID, szIniName, szIniSection), m_setting(pchSetting), m_default(szDefault) {}

	virtual void LoadFromFile(wxConfigBase& ini) {
		ini.Read( wxT("/") + szSection + wxT("/") + strName, &m_setting, m_default );
	}
	
	virtual void SaveToFile(wxConfigBase& ini) {
		ini.Write( wxT("/") + szSection + wxT("/") + strName, m_setting );
	}

	virtual void SetDlgValue() 				
	{
		bWasChanged = false;
		if (wxc==wxcText)
			((wxTextCtrl*)pctrl)->SetValue(m_setting);
	}
	
	virtual void RestorePrevValue()			
	{
#ifdef __DEBUG__
		printf("restore on Cancel not yet supported in RseString\n");
#endif
	}
		
	virtual void StoreDlgValue()			
	{
		wxString test = ((wxTextCtrl*)pctrl)->GetValue();
		if (wxc==wxcText) {
			bWasChanged = test != m_setting;
			m_setting = test;
		}
	}
		
	virtual bool WasChanged()					{ return bWasChanged; }
	
	virtual void SetCtrlValue(wxString str)		{ ((wxTextCtrl*)pctrl)->SetValue(str); }

	virtual void SetWxControl(wxControl *pc)
	{ 
		pctrl=pc;  
		if (pctrl->IsKindOf(CLASSINFO(wxTextCtrl)))
			wxc = wxcText; 
	}
	
	wxString GetMemStringValue() { return m_setting; }
	
protected:
	wxString&	m_setting;
	wxString	m_default;

private:
	bool	bWasChanged;
};

	

class RseStringEncrypted: public RseString {
// used for passwords, which should not appear in clear text on disk - no default value
// the password will be encrypted on disk, in the clear in memory.
public:
	RseStringEncrypted(int ID, wxString& pchSetting, const wxString&  szIniName, const wxString& szIniSection)
		: RseString(ID, pchSetting, szIniName, wxT(""), szIniSection)  {}

	//shakraw, when storing value, store it encrypted here (only if changed in prefs)
	virtual void StoreDlgValue()
	{
		wxString temp = ((wxTextCtrl*)pctrl)->GetValue();
		bWasChanged = m_setting != temp;
		if ((wxc==wxcText) && (bWasChanged))
			m_setting = MD5Sum(temp).GetHash();
	}
	
private:
	bool	bWasChanged;
};



class RseDirAssured: public RseString {
// A special string: the name of a directory which will be created if it does not already exist
// (used for Temp and Incoming directories) the name gets prepended with the app dir
public:
	RseDirAssured(int ID, wxString& pchSetting, const wxString&  szAppDir, const wxString&  szIniName, const wxString& szDefault, const wxString& szIniSection)
		: RseString(ID, pchSetting, szIniName, szDefault, szIniSection), strAppDir(szAppDir)  {}
		
	virtual void LoadFromFile(wxConfigBase& ini)  {
		ini.Read( wxT("/") + szSection + wxT("/") + strName, &m_setting, strAppDir + m_default );

		m_setting = MakeFoldername(m_setting);
	}
	
	void SelectDir()
	{
		wxString str = wxDirSelector(_("Choose a folder for ")+m_default, wxT(""));
		if (!str.IsEmpty())
			SetCtrlValue(str);
	}

private:
	wxString	strAppDir;
};	



class RseColumns: public Rse {  
// Window column widths / order / hidden state; used for .ini file only, no dlg interface.
// Note: the individual values here are declared as "int16", but the character sequence written
// to the .ini file for "default column width" is "65535", NOT "-1" as one would expect - this
// behavior appears to rely on a bug in CIni::SerGet, which now is a feature - we need to keep 
// doing it because we have always done it that way, and eMule does it that way.
public:
	RseColumns(int16 *piSetting, int count, const wxString&  szIniName, int16 iDefault, const wxString&  szIniSection)
		: Rse(szIniName, szIniSection), piSet(piSetting), iDef(iDefault), cnt(count)  {}

	virtual void LoadFromFile(wxConfigBase& ini) {
		wxString buffer;

		buffer = ini.Read( wxT("/") + szSection + wxT("/") + strName, wxT(""));
		
		for ( int i = 0; i < cnt; i++ )
			piSet[i] = iDef;
		
		
		int counter = 0;	
		wxStringTokenizer tokenizer( buffer, wxT(",") );
		while ( tokenizer.HasMoreTokens() && ( counter < cnt ) )
		{
    		wxString token = tokenizer.GetNextToken();

			piSet[counter] = atoi( unicode2char(token) );
			
			counter++;
		}

	}
	
	virtual void SaveToFile(wxConfigBase& ini) {
		wxString buffer;

		for ( int i = 0; i < cnt; i++ ) {
			if ( i ) 
				buffer << wxT(",");

			buffer << piSet[i];
		}
	
		
		ini.Write( wxT("/") + szSection + wxT("/") + strName, buffer );
	}
	
private:
	int16	*piSet;  
	int16	iDef;
	int		cnt;
};



//========================================================================================
//
// The following list drives loading/saving preferences in the ini file as well as getting
// values in and out of the new Preferences dialog.  The order in the list determines the 
// order in the ini file.  New items are normally added to the end of the list.
//
// When you decide to make variable XYZ in class ABC user-settable (i.e. making it a 
// "preference"), this is the recommended way of going about it:
//
// (1) make class PrefsUnifiedDlg a friend of class ABC to give it direct access;
// (2) make XYZ a "static" member of class ABC and put it in the "protected" section 
//     of ABC so that a friend class has access to it;
// (3) define XYZ in the class definition file and give it the desired default value;
// (4) use wxDesigner (or ask a team member to do this for you) to create a dialog control 
//     that will allow changing XYZ;  name the control something like IDC_XYZ and write 
//     down that ID for the next step;  generate the new muuli_wdr.cpp/.h pair;
// (5) add an entry of the appropriate type to the list below, using the ID from step (4) -
//     specify the value of the variable itself as the default value (so that the 
//     initialized value of the static member is the default)
//
// The basic idea is that "preferences" are just like other variables: even though we 
// make them persistent and give the user access to them, the principles of information
// hiding in object-oriented design still apply - variables should be defined where 
// they are normally used.  If you follow these recommendations then some day the aMule
// code will no longer be littered with references of the type "theApp.glob_prefs->s_xyz" ...
// [Emilio]
//
// WARNING:  do not, I repeat, DO NOT use casts like "(int32&)prefs->s_xyz" in the statements
// creating new table entries below.  Preventing the compiler from recognizing a variable
// according to its true type here may result in corruption of memory neighboring "prefs->s_xyz"!!!


// The list needs to be accessible before the dialog constructor gets called because we load 
// the preferences from file before setting up the rest of the app (so we use static methods).

//static 	CTypedPtrList<CPtrList, Rse*> listRse;

WX_DECLARE_LIST(Rse, ListOfRse);
#include <wx/listimpl.cpp>
WX_DEFINE_LIST(ListOfRse);

ListOfRse listRse;

Rse*	prseMaxUp;		// these pointers are needed before 
Rse*	prseMaxDown;	// PrefsUnifiedDlg constructor gets called
Rse*	aprseColor[cntStatColors];  // this array helps in accessing stat colors through RseInt's
    


void PrefsUnifiedDlg::BuildItemList(CPreferences *prefs, const wxString& appdir)  // gets called at init time
{
	listRse.Append(new Rse(wxT("Missing ID of dlg item in listRse")));  // LEAVE AT HEAD OF LIST - handles missing dlg IDs gracefully
	
	listRse.Append(new RseDirAssured(IDC_TEMPFILES, prefs->s_tempdir, appdir, wxT("TempDir"), wxT("Temp"), wxT("eMule")));
	listRse.Append(new RseString(IDC_NICK, prefs->s_nick, wxT("Nick"), wxT("http://www.aMule.org"), wxT("eMule")));
	listRse.Append(new RseDirAssured(IDC_INCFILES,prefs->s_incomingdir, appdir, wxT("IncomingDir"), wxT("Incoming"),wxT("eMule")));

	listRse.Append(prseMaxUp = new RseInt(IDC_MAXUP, prefs->s_maxupload, wxT("MaxUpload"), UNLIMITED, wxT("eMule"))); // see note in ForceUlDlRateCorrelation
	listRse.Append(prseMaxDown = new RseInt(IDC_MAXDOWN, prefs->s_maxdownload, wxT("MaxDownload"), UNLIMITED, wxT("eMule"))); // ditto

	listRse.Append(new RseInt(IDC_SLOTALLOC, prefs->s_slotallocation, wxT("SlotAllocation"), 2, wxT("eMule")));
	listRse.Append(new RseInt(IDC_MAXCON, prefs->s_maxconnections, wxT("MaxConnections"), CPreferences::GetRecommendedMaxConnections(), wxT("eMule")));
	listRse.Append(new RseBool(IDC_REMOVEDEAD, prefs->s_deadserver,wxT("RemoveDeadServer"), 1, wxT("eMule")));
	listRse.Append(new RseInt(IDC_PORT, prefs->s_port, wxT("Port"), 4662, wxT("eMule")));
	listRse.Append(new RseInt(IDC_UDPPORT, prefs->s_udpport, wxT("UDPPort"), 4672, wxT("eMule")));
	listRse.Append(new RseBool(IDC_UDPDISABLE, prefs->s_UDPDisable, wxT("UDPDisable"), false, wxT("eMule"))); 		
	listRse.Append(new RseInt(IDC_MAXSOURCEPERFILE, prefs->s_maxsourceperfile, wxT("MaxSourcesPerFile"), 300,wxT("eMule")));
	listRse.Append(new RseInt(IDC_LANGUAGE, prefs->s_languageID, wxT("Language"), 0,wxT("eMule")));

	listRse.Append(new RseInt(IDC_SEESHARES, prefs->s_iSeeShares, wxT("SeeShare"), 2,wxT("eMule")));
	listRse.Append(new RseInt(IDC_TOOLTIPDELAY, prefs->s_iToolDelayTime, wxT("ToolTipDelay"), 1, wxT("eMule")));

	listRse.Append(new RseInt(IDC_SLIDER, prefs->s_trafficOMeterInterval, wxT("StatGraphsInterval"), 3, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_SLIDERINFO, IDC_SLIDER, 1, _("Update period: %i secs"), _("Update period: %i sec"), _("Update: Disabled")));
	listRse.Append(new RseInt(IDC_SLIDER2, prefs->s_statsInterval, wxT("statsInterval"), 30, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_SLIDERINFO2, IDC_SLIDER2, 1, _("Update period: %i secs"), _("Update period: %i sec"), _("Update: Disabled")));

	listRse.Append(new RseInt(IDC_DOWNLOAD_CAP, prefs->s_maxGraphDownloadRate, wxT("DownloadCapacity"), 3, wxT("eMule"))); // see note in ForceUlDlRateCorrelation
	listRse.Append(new RseInt(IDC_UPLOAD_CAP, prefs->s_maxGraphUploadRate, wxT("UploadCapacity"), 3, wxT("eMule"))); // ditto
	listRse.Append(new RseInt(IDC_SERVERRETRIES, prefs->s_deadserverretries, wxT("DeadServerRetry"), 2, wxT("eMule")));

	listRse.Append(new RseInt(IDC_SERVERKEEPALIVE, prefs->s_dwServerKeepAliveTimeoutMins, wxT("ServerKeepAliveTimeout"), 0, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_SERVERKEEPALIVE_LABEL, IDC_SERVERKEEPALIVE, 1,
		_("Server connection refresh interval %i mins"), _("Server connection refresh interval %i min"), _("Server connection refresh interval: Disabled")));

	listRse.Append(new RseInt(0, prefs->s_splitterbarPosition, wxT("SplitterbarPosition"), 75, wxT("eMule")));	// no GUI needed (window layout)


	listRse.Append(new RseInt(IDC_SLIDER4, prefs->s_statsMax, wxT("VariousStatisticsMaxValue"), 100, wxT("eMule")));	
	listRse.Append(new RseDynLabel(IDC_SLIDERINFO4, IDC_SLIDER4, 1, _("Connections Graph Scale: %i"), wxT(""), wxT("")));

	listRse.Append(new RseInt(IDC_SLIDER3, prefs->s_statsAverageMinutes, wxT("StatsAverageMinutes"), 5, wxT("eMule"))); 
	listRse.Append(new RseDynLabel(IDC_SLIDERINFO3, IDC_SLIDER3, 1, _("Time for running averages: %i mins"), wxT(""), wxT("")));

	listRse.Append(new RseInt(IDC_MAXCON5SEC, prefs->s_MaxConperFive, wxT("MaxConnectionsPerFiveSeconds"),prefs->s_MaxConperFive, wxT("eMule")));

	listRse.Append(new RseBool(IDC_RECONN, prefs->s_reconnect, wxT("Reconnect"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SCORE, prefs->s_scorsystem, wxT("Scoresystem"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_ICH, prefs->s_ICH, wxT("ICH"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_AUTOSERVER, prefs->s_autoserverlist, wxT("Serverlist"), false, wxT("eMule")));

	listRse.Append(new RseBool(IDC_CHECK4UPDATE, prefs->s_updatenotify, wxT("UpdateNotify"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_MINTRAY, prefs->s_mintotray, wxT("MinToTray"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_UPDATESERVERCONNECT, prefs->s_addserversfromserver, wxT("AddServersFromServer"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_UPDATESERVERCLIENT, prefs->s_addserversfromclient, wxT("AddServersFromClient"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SPLASHON, prefs->s_splashscreen, wxT("Splashscreen"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_BRINGTOFOREGROUND, prefs->s_bringtoforeground, wxT("BringToFront"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_DBLCLICK, prefs->s_transferDoubleclick, wxT("TransferDoubleClick"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_BEEPER, prefs->s_beepOnError, wxT("BeepOnError"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_EXIT, prefs->s_confirmExit, wxT("ConfirmExit"),false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_FILTER, prefs->s_filterBadIP, wxT("FilterBadIPs"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_AUTOCONNECT, prefs->s_autoconnect, wxT("Autoconnect"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SHOWRATEONTITLE, prefs->s_showRatesInTitle, wxT("ShowRatesOnTitle"), false, wxT("eMule")));

	listRse.Append(new RseBool(IDC_ONLINESIG, prefs->s_onlineSig, wxT("OnlineSignature"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_STARTMIN, prefs->s_startMinimized, wxT("StartupMinimized"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SAFESERVERCONNECT, prefs->s_safeServerConnect, wxT("SafeServerConnect"), false, wxT("eMule")));

	listRse.Append(new RseBool(0, prefs->s_filterserverbyip, wxT("FilterServersByIP"), false, wxT("eMule")));	// no GUI yet
	listRse.Append(new RseInt(0, prefs->s_filterlevel, wxT("FilterLevel"), 127, wxT("eMule")));					// no GUI yet
	listRse.Append(new RseBool(IDC_CHECKDISKSPACE, prefs->s_checkDiskspace, wxT("CheckDiskspace"), true, wxT("eMule")));			// no GUI yet
	listRse.Append(new RseInt(IDC_MINDISKSPACE, prefs->s_uMinFreeDiskSpace, wxT("MinFreeDiskSpace"), 1, wxT("eMule")));		// no GUI yet
	listRse.Append(new RseString(0, prefs->s_yourHostname, wxT("YourHostname"), wxT(""), wxT("eMule"))); // no GUI yet

	listRse.Append(new RseBool(IDC_AUTOCONNECTSTATICONLY, prefs->s_autoconnectstaticonly, wxT("AutoConnectStaticOnly"), false, wxT("eMule"))); 
	listRse.Append(new RseBool(IDC_AUTOTAKEED2KLINKS, prefs->s_autotakeed2klinks, wxT("AutoTakeED2KLinks"), true, wxT("eMule"))); 
	listRse.Append(new RseBool(IDC_ADDNEWFILESPAUSED, prefs->s_addnewfilespaused, wxT("AddNewFilesPaused"), false, wxT("eMule"))); 
	listRse.Append(new RseInt(IDC_3DDEPTH, prefs->s_depth3D, wxT("3DDepth"), 10, wxT("eMule")));

	listRse.Append(new RseBool(IDC_CB_TBN_USESOUND, prefs->s_useSoundInNotifier, wxT("NotifierUseSound"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_ONLOG, prefs->s_useLogNotifier, wxT("NotifyOnLog"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_ONCHAT, prefs->s_useChatNotifier, wxT("NotifyOnChat"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_POP_ALWAYS, prefs->s_notifierPopsEveryChatMsg, wxT("NotifierPopEveryChatMessage"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_ONDOWNLOAD, prefs->s_useDownloadNotifier, wxT("NotifyOnDownload"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_ONNEWVERSION, prefs->s_notifierNewVersion, wxT("NotifierPopNewVersion"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_CB_TBN_IMPORTATNT, prefs->s_notifierImportantError, wxT("NotifyOnImportantError"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SENDMAIL, prefs->s_sendEmailNotifier, wxT("NotifyByMail"), false, wxT("eMule")));
	listRse.Append(new RseString(IDC_EDIT_TBN_WAVFILE, prefs->s_notifierSoundFilePath, wxT("NotifierSoundPath"),wxT(""), wxT("eMule")));

	listRse.Append(new RseString(0, prefs->s_datetimeformat, wxT("DateTimeFormat"),wxT("%A, %x, %X"), wxT("eMule")));    // no GUI yet

	listRse.Append(new RseBool(IDC_SMARTIDCHECK, prefs->s_smartidcheck, wxT("SmartIdCheck"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_VERBOSE, prefs->s_bVerbose, wxT("Verbose"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_PREVIEWPRIO, prefs->s_bpreviewprio, wxT("PreviewPrio"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_MANUALSERVERHIGHPRIO, prefs->s_bmanualhighprio, wxT("ManualHighPrio"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_FULLCHUNKTRANS, prefs->s_btransferfullchunks, wxT("FullChunkTransfers"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_STARTNEXTFILE, prefs->s_bstartnextfile, wxT("StartNextFile"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_SHOWOVERHEAD, prefs->s_bshowoverhead, wxT("ShowOverhead"), false, wxT("eMule")));
	listRse.Append(new RseBool(IDC_VIDEOBACKUP, prefs->s_moviePreviewBackup, wxT("VideoPreviewBackupped"), true, wxT("eMule")));
	listRse.Append(new RseInt(IDC_FILEBUFFERSIZE, prefs->s_iFileBufferSize, wxT("FileBufferSizePref"), 16, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_FILEBUFFERSIZE_STATIC, IDC_FILEBUFFERSIZE, 15000, _("File Buffer Size %i bytes"), wxT(""), wxT("")));
	
	listRse.Append(new RseInt(IDC_QUEUESIZE, prefs->s_iQueueSize, wxT("QueueSizePref"), 50, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_QUEUESIZE_STATIC, IDC_QUEUESIZE, 100, _("Upload Queue Size %i clients"), wxT(""), wxT("")));
	listRse.Append(new RseInt(IDC_CHECKDAYS, prefs->s_versioncheckdays, wxT("Check4NewVersionDelay"), 5, wxT("eMule")));
	listRse.Append(new RseDynLabel(IDC_DAYS, IDC_CHECKDAYS, 1, _("%i days"), _("%i day"), wxT("")));
	listRse.Append(new RseBool(IDC_DAP, prefs->s_bDAP, wxT("DAPPref"), true, wxT("eMule")));
	listRse.Append(new RseBool(IDC_UAP, prefs->s_bUAP, wxT("UAPPref"), true, wxT("eMule")));

/* No traces of evidence for theses in old GUI prefs handling :-) */
	listRse.Append(new RseBool(0, prefs->s_indicateratings, wxT("IndicateRatings"), true, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_allcatType, wxT("AllcatType"), 0, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_showAllNotCats, wxT("ShowAllNotCats"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_watchclipboard, wxT("WatchClipboard4ED2kFilelinks"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_resumeSameCat, wxT("ResumeNextFromSameCat"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_resumeSameCat, wxT("DontRecreateStatGraphsOnResize"), false, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_versioncheckLastAutomatic, wxT("VersionCheckLastAutomatic"), 0, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_bDisableKnownClientList, wxT("DisableKnownClientList"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_bDisableQueueList, wxT("DisableQueueList"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_bCreditSystem, wxT("UseCreditSystem"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_msgonlyfriends, wxT("MessagesFromFriendsOnly"), false, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_msgsecure, wxT("MessageFromValidSourcesOnly"), true, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_maxmsgsessions, wxT("MaxMessageSessions"), 50, wxT("eMule")));
	listRse.Append(new RseString(0, prefs->s_sTemplateFile, wxT("WebTemplateFile"), wxT("eMule.tmpl"), wxT("eMule")));

	listRse.Append(new RseString(IDC_VIDEOPLAYER, prefs->s_VideoPlayer, wxT("VideoPlayer"), wxT(""), wxT("eMule")));
	listRse.Append(new RseBool(IDC_EXTCATINFO, prefs->s_showCatTabInfos, wxT("ShowInfoOnCatTabs"), false, wxT("eMule")));
	
/* window colum widths, no dialog interaction - BEGIN */
	listRse.Append(new RseColumns(prefs->s_downloadColumnWidths, ELEMENT_COUNT(prefs->s_downloadColumnWidths), wxT("DownloadColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_downloadColumnHidden, ELEMENT_COUNT(prefs->s_downloadColumnHidden), wxT("DownloadColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_downloadColumnOrder, ELEMENT_COUNT(prefs->s_downloadColumnOrder), wxT("DownloadColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_uploadColumnWidths, ELEMENT_COUNT(prefs->s_uploadColumnWidths), wxT("UploadColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_uploadColumnHidden, ELEMENT_COUNT(prefs->s_uploadColumnHidden), wxT("UploadColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_uploadColumnOrder, ELEMENT_COUNT(prefs->s_uploadColumnOrder), wxT("UploadColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_queueColumnWidths, ELEMENT_COUNT(prefs->s_queueColumnWidths), wxT("QueueColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_queueColumnHidden, ELEMENT_COUNT(prefs->s_queueColumnHidden), wxT("QueueColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_queueColumnOrder, ELEMENT_COUNT(prefs->s_queueColumnOrder), wxT("QueueColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_searchColumnWidths, ELEMENT_COUNT(prefs->s_searchColumnWidths), wxT("SearchColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_searchColumnHidden, ELEMENT_COUNT(prefs->s_searchColumnHidden), wxT("SearchColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_searchColumnOrder, ELEMENT_COUNT(prefs->s_searchColumnOrder), wxT("SearchColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_sharedColumnWidths, ELEMENT_COUNT(prefs->s_sharedColumnWidths), wxT("SharedColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_sharedColumnHidden, ELEMENT_COUNT(prefs->s_sharedColumnHidden), wxT("SharedColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_sharedColumnOrder, ELEMENT_COUNT(prefs->s_sharedColumnOrder), wxT("SharedColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_serverColumnWidths, ELEMENT_COUNT(prefs->s_serverColumnWidths), wxT("ServerColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_serverColumnHidden, ELEMENT_COUNT(prefs->s_serverColumnHidden), wxT("ServerColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_serverColumnOrder, ELEMENT_COUNT(prefs->s_serverColumnOrder), wxT("ServerColumnOrder"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_clientListColumnWidths, ELEMENT_COUNT(prefs->s_clientListColumnWidths), wxT("ClientListColumnWidths"), DEFAULT_COL_SIZE, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_clientListColumnHidden, ELEMENT_COUNT(prefs->s_clientListColumnHidden), wxT("ClientListColumnHidden"),0, wxT("eMule")));
	listRse.Append(new RseColumns(prefs->s_clientListColumnOrder, ELEMENT_COUNT(prefs->s_clientListColumnOrder), wxT("ClientListColumnOrder"),0, wxT("eMule")));
/*  window colum widths - END */

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	listRse.Append(new RseInt(0, prefs->s_tableSortItemDownload, wxT("TableSortItemDownload"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemUpload, wxT("TableSortItemUpload"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemQueue, wxT("TableSortItemQueue"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemSearch, wxT("TableSortItemSearch"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemShared, wxT("TableSortItemShared"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemServer, wxT("TableSortItemServer"), 0, wxT("eMule")));
	listRse.Append(new RseInt(0, prefs->s_tableSortItemClientList, wxT("TableSortItemClientList"), 0, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingDownload, wxT("TableSortAscendingDownload"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingUpload, wxT("TableSortAscendingUpload"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingQueue, wxT("TableSortAscendingQueue"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingSearch, wxT("TableSortAscendingSearch"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingShared, wxT("TableSortAscendingShared"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingServer, wxT("TableSortAscendingServer"), true, wxT("eMule")));
	listRse.Append(new RseBool(0, prefs->s_tableSortAscendingClientList, wxT("TableSortAscendingClientList"), true, wxT("eMule")));

	for (int i=0; i<cntStatColors; i++) {  // colors have been moved from global prefs to CStatisticsDlg
		wxString str = wxString::Format(wxT("StatColor%i"),i);
		listRse.Append(aprseColor[i] = new RseInt(0, CStatisticsDlg::acrStat[i], str, CStatisticsDlg::acrStat[i], wxT("eMule")));
	}

	listRse.Append(new RseCounter(prefs->s_totalDownloadedBytes, wxT("TotalDownloadedBytes"), wxT("Statistics"))); // no GUI needed
	listRse.Append(new RseCounter(prefs->s_totalUploadedBytes, wxT("TotalUploadedBytes"), wxT("Statistics")));		// no GUI needed
	
	listRse.Append(new RseInt(0, prefs->s_desktopMode, wxT("DesktopMode"), 4, wxT("Statistics")));

	listRse.Append(new RseStringEncrypted(IDC_WEB_PASSWD, prefs->s_sWebPassword, wxT("Password"), wxT("WebServer")));
	listRse.Append(new RseStringEncrypted(IDC_WEB_PASSWD_LOW, prefs->s_sWebLowPassword, wxT("PasswordLow"), wxT("WebServer")));
	listRse.Append(new RseInt(IDC_WEB_PORT, prefs->s_nWebPort, wxT("Port"), 4711, wxT("WebServer")));
	listRse.Append(new RseBool(IDC_ENABLE_WEB, prefs->s_bWebEnabled, wxT("Enabled"), false, wxT("WebServer")));
	listRse.Append(new RseBool(IDC_WEB_GZIP, prefs->s_bWebUseGzip, wxT("UseGzip"), true, wxT("WebServer")));
	listRse.Append(new RseBool(IDC_ENABLE_WEB_LOW, prefs->s_bWebLowEnabled, wxT("UseLowRightsUser"), false, wxT("WebServer")));
	listRse.Append(new RseInt(IDC_WEB_REFRESH_TIMEOUT, prefs->s_nWebPageRefresh, wxT("PageRefreshTime"), 120, wxT("WebServer")));

	listRse.Append(new RseInt(IDC_NNS_HANDLING, prefs->s_NoNeededSources, wxT("NoNeededSourcesHandling"), 2, wxT("Razor_Preferences")));
	
	listRse.Append(new RseBool(IDC_ENABLE_AUTO_FQS, prefs->s_DropFullQueueSources, wxT("FullQueueSources"), false, wxT("Razor_Preferences")));
	listRse.Append(new RseBool(IDC_ENABLE_AUTO_HQRS, prefs->s_DropHighQueueRankingSources, wxT("HighQueueRankingSources"), false, wxT("Razor_Preferences")));
	listRse.Append(new RseInt(IDC_HQR_VALUE, prefs->s_HighQueueRanking, wxT("HighQueueRanking"), 1200, wxT("Razor_Preferences")));
	listRse.Append(new RseInt(IDC_AUTO_DROP_TIMER, prefs->s_AutoDropTimer, wxT("AutoDropTimer"), 240, wxT("Razor_Preferences")));
	listRse.Append(new RseBool(IDC_FED2KLH, prefs->s_FastED2KLinksHandler, wxT("FastED2KLinksHandler"), true, wxT("Razor_Preferences")));

	listRse.Append(new RseBool(IDC_EXT_CONN_ACCEPT, prefs->s_AcceptExternalConnections, wxT("AcceptExternalConnections"), true,wxT("ExternalConnect")));
	listRse.Append(new RseBool(IDC_EXT_CONN_USETCP, prefs->s_ECUseTCPPort, wxT("ECUseTCPPort"), false,wxT("ExternalConnect")));
	listRse.Append(new RseInt(IDC_EXT_CONN_TCP_PORT, prefs->s_ECPort, wxT("ECPort"), 4712, wxT("ExternalConnect")));
	listRse.Append(new RseStringEncrypted(IDC_EXT_CONN_PASSWD, prefs->s_ECPassword, wxT("ECPassword"), wxT("ExternalConnect")));
	listRse.Append(new RseBool(IDC_NEWSTYLETABS, prefs->s_bDlgTabsOnTop, wxT("DlgTabsOnTop"), false,wxT("ExternalConnect")));  

	// Kry
	listRse.Append(new RseBool(IDC_SECIDENT, prefs->s_SecIdent, wxT("UseSecIdent"), true,wxT("ExternalConnect")));
	listRse.Append(new RseBool(IDC_IPFONOFF, prefs->s_IPFilterOn, wxT("IpFilterOn"), true,wxT("ExternalConnect"))); 	 
	listRse.Append(new RseBool(IDC_SRCSEEDS, prefs->s_UseSrcSeeds, wxT("UseSrcSeeds"), false,wxT("ExternalConnect"))); 	 
	listRse.Append(new RseBool(IDC_PROGBAR, prefs->s_ProgBar, wxT("ShowProgressBar"), true,wxT("ExternalConnect"))); 	 
	listRse.Append(new RseBool(IDC_PERCENT, prefs->s_Percent, wxT("ShowPercent"), false,wxT("ExternalConnect"))); 	
	listRse.Append(new RseBool(IDC_METADATA, prefs->s_ExtractMetaData, wxT("ExtractMetaDataTags"), false,wxT("ExternalConnect"))); 	
	listRse.Append(new RseBool(IDC_CHUNKALLOC, prefs->s_AllocFullChunk, wxT("FullChunkAlloc"), false,wxT("ExternalConnect"))); 		
	listRse.Append(new RseBool(IDC_FULLALLOCATE, prefs->s_AllocFullPart, wxT("FullPartAlloc"), false,wxT("ExternalConnect"))); 		
	listRse.Append(new RseBool(IDC_FCHECKTABS, prefs->s_BrowserTab, wxT("BrowserTab"), true, wxT("FakeCheck")));
	listRse.Append(new RseString(IDC_FCHECKSELF, prefs->s_CustomBrowser, wxT("CustomBrowser"), wxT(""), wxT("FakeCheck")));
	listRse.Append(new RseInt(IDC_FCHECK, prefs->s_Browser, wxT("Browser"), 0,wxT("FakeCheck")));	
	listRse.Append(new RseBool(IDC_SAFEMAXCONN, prefs->s_UseSafeMaxConn, wxT("SafeMaxConn"), false, wxT("FakeCheck"))); 		
	listRse.Append(new RseBool(IDC_VERBOSEPACKETERROR, prefs->s_VerbosePacketError, wxT("VerbosePacketError"), false, wxT("FakeCheck"))); 
	listRse.Append(new RseDirAssured(IDC_OSDIR, prefs->s_OSDirectory, appdir, wxT("OSDirectory"), wxT(""), wxT("FakeCheck")));	
	listRse.Append(new RseBool(IDC_USESKIN, prefs->s_UseSkinFile, wxT("UseSkinFile"), false, wxT("SkinGUIOptions"))); 
	listRse.Append(new RseDirAssured(IDC_SKINFILE, prefs->s_SkinFile, appdir, wxT("SkinFile"), wxT(""), wxT("SkinGUIOptions")));	
}

//==============================================================================
//
//  Dialog implementation

// WDR: class implementations

//----------------------------------------------------------------------------
// PrefsUnifiedDlg
//----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(PrefsUnifiedDlg,wxDialog)

// WDR: event table for PrefsUnifiedDlg

BEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	EVT_SCROLL(PrefsUnifiedDlg::OnScroll)
	EVT_SPINCTRL(IDC_MAXUP, PrefsUnifiedDlg::OnSpinMaxDLR)
	EVT_SPINCTRL(IDC_MAXDOWN, PrefsUnifiedDlg::OnSpinMaxDLR)
	EVT_CHECKBOX(IDC_UDPDISABLE, PrefsUnifiedDlg::OnCheckBoxChange)
	EVT_CHECKBOX(IDC_USESKIN, PrefsUnifiedDlg::OnCheckBoxChange)
	
	EVT_BUTTON(ID_PREFS_OK_TOP, PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_PREFS_OK_LEFT, PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_OK, PrefsUnifiedDlg::OnOk)
	
	EVT_BUTTON(ID_PREFS_CANCEL_TOP, PrefsUnifiedDlg::OnCancel)
	EVT_BUTTON(ID_PREFS_CANCEL_LEFT, PrefsUnifiedDlg::OnCancel)
	EVT_BUTTON(ID_CANCEL, PrefsUnifiedDlg::OnCancel)
	
	EVT_BUTTON(IDC_SELTEMPDIR, PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELINCDIR,  PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELOSDIR,  PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELSKINFILE,  PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV, PrefsUnifiedDlg::OnButtonBrowseWav)
	EVT_BUTTON(IDC_BROWSEV, PrefsUnifiedDlg::OnButtonBrowseVideoplayer)
	EVT_BUTTON(IDC_EDITADR, PrefsUnifiedDlg::OnButtonEditAddr)
	EVT_BUTTON(ID_DESKTOPMODE, PrefsUnifiedDlg::OnButtonSystray)
	//EVT_BUTTON(IDC_WIZARD, PrefsUnifiedDlg::OnButtonWizard)
	EVT_BUTTON(IDC_IPFRELOAD, PrefsUnifiedDlg::OnButtonIPFilterReload)
	EVT_BUTTON(IDC_COLOR_BUTTON, PrefsUnifiedDlg::OnButtonColorChange)
	EVT_CHOICE(IDC_COLORSELECTOR, PrefsUnifiedDlg::OnColorCategorySelected)
	EVT_CHOICE(IDC_FCHECK, PrefsUnifiedDlg::OnFakeBrowserChange)
END_EVENT_TABLE()


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow *parent)
	: wxDialog(parent,9990, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxSYSTEM_MENU | wxRESIZE_BORDER)
{
	Rse *prse;
	int id;

	if (theApp.glob_prefs->BDlgTabsOnTop()) {
		preferencesDlgTop( this, TRUE ); 
	} else {
    		preferencesDlgLeft( this, TRUE ); 
	}
	
	CentreOnParent();
	pdtcShareSelector = ((CDirectoryTreeCtrl*)FindWindowById(IDC_SHARESELECTOR, this));
	pbuttonColor = (wxButton*)FindWindowById(IDC_COLOR_BUTTON, this);
	pchoiceColor = (wxChoice*)FindWindowById(IDC_COLORSELECTOR, this);

	// Link up the dlg items and listRse
	// First pass: get wxControl pointers and identify range of IDs
	idMax = 0;
	idMin = 0x7fffffff;
	
	wxListOfRseNode *pos;
	
	pos = listRse.GetFirst();
	while (pos) 	{		
		
		prse = pos->GetData();
		
		if ((id=prse->Id()) != 0) {
			if (id < idMin)
				idMin = id;
			if (id > idMax)
				idMax = id;
			// if the code crashes on the following line after you've added a new item to
			// listRse, then the ID used in the new list entry is probably wrong - wxWindows
			// does not seem to handle a non-existent ID gracefully inside FindWindowById().
			wxControl *pctrl = (wxControl *)FindWindowById(id, this);
			prse->SetWxControl(pctrl);
		}
		
		pos = pos->GetNext();
	}			
	wxASSERT(idMin<=idMax);
	// Second pass: build an array of pointers into listRse for fast access
	// [Note: the wxControl class lacks a "backpointer" facility - as a kludge
	// one could in principle use SetName / GetName with bent pointers, but that
	// would be very, very ugly and might break on future versions of wxWindows]
	trse = new Rse*[idMax-idMin+1];
	Rse** pprse;
	int i;
	for (pprse=trse, i=idMax-idMin+1;  i>0;  pprse++, i--) {
		*pprse = listRse.GetFirst()->GetData();  // dummy used for non-consecutive IDs
	}
	
	pos = listRse.GetFirst();
	while (pos) 	{		
		
		prse = pos->GetData();
	
		if ((id = prse->Id()) > 0)  {
			wxASSERT(id>=idMin && id<=idMax);
			trse[id-idMin] = prse;
		}
		pos = pos->GetNext();
	}
	
	// Third pass: establish links between items, e.g. dynamic labels of sliders
	pos = listRse.GetFirst();
	while (pos) 	{		
		
		prse = pos->GetData();
		if ((id = prse->IdLinkedTo()) != 0)
			Prse(id)->SetLink(prse);	
		pos = pos->GetNext();
	}
	
	wxASSERT(prseMaxUp == Prse(IDC_MAXUP));
	wxASSERT(prseMaxDown = Prse(IDC_MAXDOWN));
	
}



PrefsUnifiedDlg::~PrefsUnifiedDlg()
{
	delete[] trse;
	listRse.DeleteContents(true);
	listRse.Clear();
	
	/*		
	for (POSITION pos=listRse.GetHeadPosition();  pos!=NULL;  pos=listRse.NextAt(pos))
		delete listRse.GetAt(pos);
	listRse.RemoveAll();
	*/
}



Rse* PrefsUnifiedDlg::Prse(int id)	// returns the Rse* corresponding to an item ID
{ 
	wxASSERT(id>=idMin && id<=idMax);
	return trse[id-idMin]; 
}	


bool PrefsUnifiedDlg::Validate()
{
    return TRUE;
}



bool PrefsUnifiedDlg::TransferDataToWindow()
{	
	wxListOfRseNode *pos;
	
	pos = listRse.GetFirst();
	while (pos) 	{	
		(pos->GetData())->TransferToDlg();	
		pos = pos->GetNext();
	}		

	pdtcShareSelector->SetSharedDirectories(&theApp.glob_prefs->shareddir_list);
	CheckRateUnlimited(prseMaxUp);
	CheckRateUnlimited(prseMaxDown);
    return TRUE;
}



bool PrefsUnifiedDlg::TransferDataFromWindow()
{
	wxListOfRseNode *pos;
	pos = listRse.GetFirst();
	
	while (pos) 	{	
		(pos->GetData())->TransferFromDlg();
		pos = pos->GetNext();
	}		
	
	
	theApp.glob_prefs->shareddir_list.Clear();
	pdtcShareSelector->GetSharedDirectories(&theApp.glob_prefs->shareddir_list);
    return TRUE;
}


void PrefsUnifiedDlg::CheckRateUnlimited(Rse* prse)
{
	if (prse->GetMemValue() == 0)
		prse->SetMemValue(UNLIMITED);
	if (prse->GetMemValue() == UNLIMITED)
		prse->SetCtrlValue(0);
}



void PrefsUnifiedDlg::ForceUlDlRateCorrelation(int id)
// Here we slightly limit the users' ability to be a bad citizen: for very low upload rates
// we force a low download rate, so as to discourage this type of leeching.  
// We're Open Source, and whoever wants it can do his own mod to get around this, but the 
// packaged product will try to enforce good behavior. 
//
// Kry note: of course, any leecher mod will be banned asap.
//
// Also please note: Upload & Download Line "Capacities" are misnamed in the dlg right now: 
// as the eMule FAQ explains (and our code follows eMule in this respect), these values 
// are used only for scaling the statistics graphs and have no influence on network 
// I/O speeds, which are set set separately.  The old prefs limited the max line speeds to
// the values set as "capacities", the new prefs no longer do this for the following reason:
// My actual line capacity may be very high, say 256kB/s, but my DL rates may be much lower
// most of the time; if I want the graph to display with a useful scale, I will then set
// the "capacity" to the limit of actual experienced DL rates, say 60kB/s, yet I may not 
// want to limit my DLs to that rate, if I'm lucky enough to be sent data at a high rate 
// occasionally.  The confusion will go away when we move the spin controls for graph scaling 
// into the Statistics window, together with the sliders for update rate and averaging time.
{
	if (id == 0) {  // at init time no dlg exists yet, but check values loaded from file
		if (prseMaxUp->GetMemValue() < 4  && prseMaxUp->GetMemValue()*3 < prseMaxDown->GetMemValue())
			prseMaxDown->SetMemValue(prseMaxUp->GetMemValue()*3);
		else if (prseMaxUp->GetMemValue() < 10  && prseMaxUp->GetMemValue()*4 < prseMaxDown->GetMemValue())
			prseMaxDown->SetMemValue(prseMaxUp->GetMemValue()*4);
	} else {
		int kBpsUpMax = prseMaxUp->GetCtrlValue();
		int kBpsDownMax = prseMaxDown->GetCtrlValue();

		if (kBpsUpMax == 0  ||  kBpsUpMax >= 10) {
			prseMaxDown->SetCtrlRange(0, 19375 /* a magic number ?*/);
		} else if (kBpsUpMax < 4) {
			prseMaxDown->SetCtrlRange(1, kBpsUpMax*3);
			if (kBpsDownMax==0  ||  kBpsDownMax>kBpsUpMax*3)
				prseMaxDown->SetCtrlValue(kBpsUpMax*3);
		} else {
			prseMaxDown->SetCtrlRange(1, kBpsUpMax*4);
			if (kBpsDownMax==0  ||  kBpsDownMax>kBpsUpMax*4)
				prseMaxDown->SetCtrlValue(kBpsUpMax*4);
		}
	}
}



void PrefsUnifiedDlg::OnSpinMaxDLR(wxSpinEvent &event)
{
	ForceUlDlRateCorrelation(((wxControl*)event.GetEventObject())->GetId());
}



void PrefsUnifiedDlg::OnScroll(wxScrollEvent &event)
{
	wxControl*	pctrl = (wxControl*)event.GetEventObject();
	int			id = pctrl->GetId();
	Rse*		prse = Prse(id);

     prse->Propagate();

	
	// dynamic interactions
	switch (id) {
		case IDC_SLIDER:	prse->TransferFromDlg();
					 		theApp.amuledlg->statisticswnd->SetUpdatePeriod();	
							break;
		case IDC_SLIDER2:	prse->TransferFromDlg();	
							// tree update time needs no special function call
							break;
		case IDC_SLIDER3:	prse->TransferFromDlg();
							theApp.amuledlg->statisticswnd->ResetAveragingTime();
							break;
		case IDC_SLIDER4:	prse->TransferFromDlg();
							break;

		default:	break;
	}
}



void PrefsUnifiedDlg::OnOk(wxCommandEvent& WXUNUSED(event))
{
	TransferDataFromWindow();
	
	// do sanity checking, special processing, and user notifications here
	ForceUlDlRateCorrelation(0);
	CheckRateUnlimited(prseMaxUp);
	CheckRateUnlimited(prseMaxDown);
	Prse(IDC_UDPDISABLE)->SetCtrlValue(!Prse(IDC_UDPPORT)->GetMemValue());
	Prse(IDC_FCHECKSELF)->SetEnabled(Prse(IDC_FCHECK)->GetMemValue()==(Prse(IDC_FCHECK)->GetCtrlCount() -1));
	// save the preferences on ok
	if (theApp.glob_prefs) {
		theApp.glob_prefs->Save(); }
	
	if (Prse(IDC_FED2KLH)->WasChanged()) 
		theApp.amuledlg->ToggleFastED2KLinksHandler();
	if (Prse(IDC_LANGUAGE)->WasChanged())
		wxMessageBox(wxString::wxString(_("Language change will not be applied until aMule is restarted.")));

	if (Prse(IDC_INCFILES)->WasChanged() || Prse(IDC_TEMPFILES)->WasChanged() || pdtcShareSelector->HasChanged) {
		theApp.sharedfiles->Reload(true, false);
	}
	
	if (Prse(IDC_PERCENT)->WasChanged() || Prse(IDC_PROGBAR)->WasChanged()) {		
		// Force upload of the donwload queue 
		theApp.downloadqueue->UpdateDisplayedInfo(true);
	}

	if (Prse(IDC_OSDIR)->WasChanged()) {		
		// Build the filenames for the two OS files
		theApp.SetOSFiles(Prse(IDC_OSDIR)->GetMemStringValue());
	}
	
    EndModal(ID_PREFS_OK_LEFT);
}


void PrefsUnifiedDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// undo interactive changes here, e.g. stats graphs parameter settings
	Prse(IDC_SLIDER)->RestorePrevValue();
	Prse(IDC_SLIDER3)->RestorePrevValue();
	Prse(IDC_SLIDER2)->RestorePrevValue();
	Prse(IDC_SLIDER4)->RestorePrevValue();
	Prse(IDC_COLOR_BUTTON)->RestorePrevValue();
	theApp.amuledlg->statisticswnd->SetUpdatePeriod();	
	theApp.amuledlg->statisticswnd->ResetAveragingTime();
	for (int i=0; i<cntStatColors; i++) {
		aprseColor[i]->RestorePrevValue();
		theApp.amuledlg->statisticswnd->ApplyStatsColor(i);
	}
	
    EndModal(ID_PREFS_CANCEL_LEFT);
}



void PrefsUnifiedDlg::OnCheckBoxChange(wxCommandEvent& event)
{
	wxCheckBox*	pbox = (wxCheckBox*)event.GetEventObject();
	int			id = pbox->GetId();
	bool		bIsChecked = pbox->GetValue();

	switch (id) {
		case IDC_UDPDISABLE: {
			Rse* prse = Prse(IDC_UDPPORT);
			if (bIsChecked) {
				prse->SetCtrlRange(0,0);
				prse->SetCtrlValue(0);
			} else {
				prse->SetCtrlRange(1025,65535);
				int iPrev = prse->GetPrevValue();
				prse->SetCtrlValue(iPrev ? iPrev : Prse(IDC_PORT)->GetCtrlValue()+10);
			}
			break;
		}
		case IDC_USESKIN: {
			Prse(IDC_SKINFILE)->SetEnabled(bIsChecked);
		}
		
		default:	
			break;
	}
}

void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = GetColorIndex();
	wxColour col = WxColourFromCr(aprseColor[index]->GetMemValue());
	col = wxGetColourFromUser(this, col);
	if(col.Ok()) {
		pbuttonColor->SetBackgroundColour(col);
		aprseColor[index]->SetMemValue(CrFromWxColour(col));
		theApp.amuledlg->statisticswnd->ApplyStatsColor(index);
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	pbuttonColor->SetBackgroundColour(WxColourFromCr(aprseColor[GetColorIndex()]->GetMemValue()));
}


void PrefsUnifiedDlg::OnFakeBrowserChange(wxCommandEvent& WXUNUSED(evt))
{
	Prse(IDC_FCHECK)->StoreDlgValue();
	Prse(IDC_FCHECKSELF)->SetEnabled(Prse(IDC_FCHECK)->GetMemValue()==(Prse(IDC_FCHECK)->GetCtrlCount() -1));
}

void PrefsUnifiedDlg::OnButtonSystray(wxCommandEvent& WXUNUSED(evt))
{
	theApp.amuledlg->changeDesktopMode();
}



void PrefsUnifiedDlg::OnButtonDir(wxCommandEvent& event)
{
	wxControl*	pctrl = (wxControl*)event.GetEventObject();
	int			idButton = pctrl->GetId();
	Rse*		prse;
	
	if (idButton == IDC_SELTEMPDIR)
		prse = Prse(IDC_TEMPFILES);
	else if (idButton == IDC_SELINCDIR)
		prse = Prse(IDC_INCFILES);
	else if (idButton == IDC_SELOSDIR)
		prse = Prse(IDC_OSDIR);
	else if (idButton == IDC_SELSKINFILE)
		prse = Prse(IDC_SKINFILE);
	else
		wxASSERT(false);
	((RseDirAssured*)prse)->SelectDir();
}



void PrefsUnifiedDlg::OnButtonBrowseWav(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector(_("Browse wav"),wxT(""),wxT(""),wxT("*.wav"),_("File wav (*.wav)|*.wav||"));
	Prse(IDC_EDIT_TBN_WAVFILE)->SetCtrlValue(str);
}



void PrefsUnifiedDlg::OnButtonBrowseVideoplayer(wxCommandEvent& WXUNUSED(e))
{
	wxString str=wxFileSelector(_("Browse for videoplayer"),wxT(""),wxT(""),wxT(""),_("Executable (*)|*||"));
	if(!str.IsEmpty()) 
		Prse(IDC_VIDEOPLAYER)->SetCtrlValue(str);
}



void PrefsUnifiedDlg::OnButtonEditAddr(wxCommandEvent& WXUNUSED(evt))
{
	wxString fullpath(theApp.ConfigDir + wxT("addresses.dat"));
	
	EditServerListDlg* test=new EditServerListDlg(this, _("Edit Serverlist"),
	_("Add here URL's to download server.met files.\nOnly one url on each line."), fullpath);
	test->ShowModal();
  
	delete test;
}

void PrefsUnifiedDlg::OnButtonIPFilterReload(wxCommandEvent& WXUNUSED(event)) {
	theApp.ipfilter->Reload();
}	

void PrefsUnifiedDlg::LoadAllItems(wxConfigBase& ini)
{
	wxListOfRseNode *pos;
	
	pos = listRse.GetFirst();
	while (pos) 	{	
		(pos->GetData())->LoadFromFile(ini);
	
		pos = pos->GetNext();
	}
	
	// Now do some post-processing / sanity checking on the values we just loaded
	ForceUlDlRateCorrelation(0);
	
}



void PrefsUnifiedDlg::SaveAllItems(wxConfigBase& ini)
{
	wxListOfRseNode *pos;
	
	pos = listRse.GetFirst();
	while (pos) 	{	
		(pos->GetData())->SaveToFile(ini);
		pos = pos->GetNext();
	}
}
