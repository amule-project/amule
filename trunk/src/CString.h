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

#ifndef CSTRING_H
#define CSTRING_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString
#include <wx/list.h>		// Needed for WX_DECLARE_LIST
#include "position.h"		// Needed for POSITION
#include "otherfunctions.h"

class stringList;

class CString : public wxString {
public:
	CString() : wxString() {};
	CString(const char* ptr) : wxString(char2unicode(ptr)) {};
	CString(const wchar_t* ptr) : wxString(ptr) {};
	const char* GetBuffer() { 
		return (const char*)GetData();
	};
	
	int GetLength() const {
		return Length();
	};
     
	int Find(CString what) const {
		return wxString::Find(what);
	};
       
	CString Mid(int from,int len) {
		return CString(wxString::Mid(from,len).GetData());
	}	
	
	void Format(const wxChar* pszFormat,...) {
		va_list argptr;
		va_start(argptr,pszFormat);
		PrintfV(pszFormat,argptr);
		va_end(argptr);
	};
	
	CString& operator=(wxString& src) {
		if(this!=&src) {
			Clear();
			Append(src.GetData());
		}
    	return *this;
	}

	CString& operator=(wxString src) {
		if(this!=&src) {
			Clear();
			Append(src.GetData());
		}
		return *this;
	}
  
	CString& operator=(const char a[]) {
		Clear();
		Append((wxChar*)a);
		return *this;
	}

	CString& operator+(const char a[]) {
		Append((wxChar*)a);
		return *this;
	}

	CString& operator+(wxString src) {
		if(this!=&src) {
			Append(src.GetData());
		}
		return *this;
	}
	
};

#define CStringA CString

WX_DECLARE_LIST(CString, stringList);

class CStringList : public stringList {
public:
  CStringList(int nblocks=0) { }

  POSITION GetHeadPosition() { return (POSITION)GetFirst(); }

  CString GetNext(POSITION& pos)
      {
        stringList::Node* nod=(stringList::Node*)pos;
        CString retval=*(nod->GetData());
        nod=nod->GetNext();
        pos=(POSITION)nod;
        return retval;
      }

  CString GetAt(POSITION pos)
      {
        stringList::Node* nod=(stringList::Node*)pos;
        return *(nod->GetData());
      }

  void SetAt(POSITION pos, CString* string)
      { ((stringList::Node*)pos)->SetData(string); }
};

#endif // CSTRING_H
