//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
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

#ifndef DOWNLOADLISTCTRL_H
#define DOWNLOADLISTCTRL_H

#include <utility>		// Needed for std::pair
#include <map>			// Needed for std::multimap
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/bitmap.h>		// Needed for wxBitmap
#include <wx/imaglist.h>        // Needed for wxImageList

#include "types.h"		// Needed for uint8
#include "MuleListCtrl.h"	// Needed for CMuleListCtrl
#include "CTypedPtrList.h"	// Needed for CTypedPtrList
 
class CPartFile;
class CUpDownClient;

enum ItemType {FILE_TYPE=1,AVAILABLE_SOURCE=2,UNAVAILABLE_SOURCE=3};
struct CtrlItem_Struct{
	ItemType			type;
	CPartFile*			owner;
	void*				value;
	CtrlItem_Struct*		parent;
	DWORD				dwUpdated;
	wxBitmap*			status;

	~CtrlItem_Struct() { delete status; }
};

// CDownloadListCtrl

class CDownloadListCtrl : public CMuleListCtrl
{
DECLARE_DYNAMIC_CLASS(CDownloadListCtrl)

public:
	CDownloadListCtrl();
	CDownloadListCtrl(wxWindow*& parent,int id,const wxPoint& pos,wxSize siz,int flags);
	virtual	~CDownloadListCtrl();
	uint8 curTab;
	void UpdateItem(void* toupdate);
	void Init();
	void InitSort();
	void AddFile(CPartFile* toadd);
	void AddSource(CPartFile* owner,CUpDownClient* source,bool notavailable);
	void RemoveSource(CUpDownClient* source,CPartFile* owner);
	void RemoveFile(CPartFile* toremove);
	void ClearCompleted();
	void SetStyle();
	void CreateMenues();
	void HideSources(CPartFile* toCollapse,bool isShift = false,bool isCtrl = false,bool isAlt = false);
	void ShowFilesCount();
	void ChangeCategory(int newsel);
	wxString getTextList();
	void ShowSelectedFileDetails();
	void HideFile(CPartFile* tohide);
	void ShowFile(CPartFile* tohide);
	
	// lagloose
	bool isShift;
	void OnKeyUp(wxKeyEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	// end lagloose

public:
	virtual void OnDrawItem(int item,wxDC* dc,const wxRect& rect,const wxRect& rectHL,bool highlighted);
	
protected:
	void OnColResize(wxListEvent& evt);

	CPreferences::Table TablePrefs()	{ return CPreferences::tableDownload; }
	void DrawFileItem(wxDC* dc,int nColumn,LPRECT lpRect,CtrlItem_Struct* lpCtrlItem);
	void DrawSourceItem(wxDC* dc,int nColumn,LPRECT lpRect,CtrlItem_Struct* lpCtrlItem);
	void OnColumnClick(wxListEvent& evt);
	static int wxCALLBACK SortProc(long lp1,long lp2,long lpSort);
	static int Compare(CPartFile* file1, CPartFile* file2, long lParamSort);
	static int Compare(CUpDownClient* client1, CUpDownClient* client2, long lParamSort,int sortMod);
	void OnLvnItemActivate(wxListEvent& evt);
	void OnNMRclick(wxListEvent& evt);
	void OnPriLow(wxCommandEvent& evt);
	void OnPriNormal(wxCommandEvent& evt);
	void OnPriHigh(wxCommandEvent& evt);
	void OnMCancel(wxCommandEvent& evt);
	bool ProcessEvent(wxEvent& evt);
	void setPri(int newpri);
	void collectSelections(CTypedPtrList<CPtrList,CPartFile*>* selectedList);
	DECLARE_EVENT_TABLE()

private:
	bool ShowItemInCurrentCat(CPartFile* file,int newsel);
	bool 	this_is_the_moment();
	int	last_moment;


	typedef std::pair<void*,CtrlItem_Struct*> ListItemsPair;
	typedef std::multimap<void*,CtrlItem_Struct*> ListItems;
	ListItems	m_ListItems;
	wxImageList	m_ImageList;
	wxMenu*		m_FileMenu;
	wxMenu*		m_ClientMenu;
	wxMenu*		m_PrioMenu;
	wxBrush*	m_hilightBrush,*m_hilightUnfocusBrush;
};

#endif // DOWNLOADLISTCTRL_H
