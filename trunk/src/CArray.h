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

#ifndef CARRAY_H
#define CARRAY_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dynarray.h>	// Needed for WX_DEFINE_ARRAY
#include <wx/object.h>		// Needed for wxObject

#include "types.h"		// Needed for uint16

class VoidArray;
WX_DEFINE_ARRAY(void*, VoidArray);

template<class TYPE,class ARG_TYPE>
class CArray : public wxObject {

private:
	VoidArray intArr;

public:
	CArray() {};
	
	~CArray() { 
		intArr.Empty();
	}
	
	void Copy(const CArray& src) {
		RemoveAll();
		size_t newsize = src.GetSize();
		SetSize(newsize); // to avoid too many mallocs.
		for (uint32 i = 0; i < newsize; i++) {
			Add(src[i]);
		}
	}
	
	size_t GetSize() const {
		return intArr.GetCount();   
	};
	
	size_t GetCount() const {
		return intArr.GetCount();
	};
	
	size_t GetUpperBound() const {
		return intArr.GetCount()+1;
	};
	
	void SetSize(size_t newsiz,int growby=-1) {
		intArr.Alloc(newsiz);
		if (newsiz > intArr.GetCount()) {
			intArr.Add(NULL, newsiz - intArr.GetCount());
		}
	};
	
	bool IsEmpty() const {
		return intArr.IsEmpty();
	};
	
	void RemoveAll() {
		intArr.Empty();
	};
	
	void FreeExtra() {
		intArr.Shrink();
	};
	
	void RemoveAt(size_t nIndex) {
		intArr.RemoveAt(nIndex);
	};

	TYPE GetAt(size_t nIndex) const {
		return (TYPE)intArr.Item(nIndex);
	};
	
	void SetAt(size_t nIndex,ARG_TYPE newElem) {
		intArr[nIndex]=(void*)((unsigned long)newElem);
	};
	
	TYPE& ElementAt(size_t nIndex) {
		return (TYPE&)intArr.Item(nIndex);
	};
	
	int Add(ARG_TYPE newElem) {
		intArr.Add((void*)((unsigned long)newElem));
		return 0;
	};
	
	void InsertAt(size_t nIndex,ARG_TYPE newElem) {
		intArr.Insert((void*)((unsigned long)newElem),nIndex,1);
	}

	void Sort(int compareFunction(TYPE first, TYPE second) ) {
		intArr.Sort(compareFunction);
	}
	
	TYPE operator[](size_t nIndex) const {
		return (TYPE)intArr[nIndex];
	};
	
	TYPE& operator[](size_t nIndex) {
		return (TYPE&)intArr[nIndex];
	};
	
//END CARRAY
};

/* Kry - Implementing CQArray */
#ifndef _QuickSortCArrayAndFunctionTemplate_7EB8E364_1A47_11d3_AFD1_0080ADB99E81_
#define _QuickSortCArrayAndFunctionTemplate_7EB8E364_1A47_11d3_AFD1_0080ADB99E81_

template <class TYPE> void QuickSortRecursive(TYPE *pArr, int d, int h, bool bAscending)
{
	int i,j;
	uint16 str;
	
	i = h;
	j = d;

	
	str = *((uint16 *)(&pArr[((int) ((d+h) / 2))]));
	do {

		if (bAscending) {
			// VERY VERY VERY NASTY - we need to somehow define operators < > for TYPE
			while (*((uint16 *)(&pArr[j])) < (str)) j++;
			while (*((uint16 *)(&pArr[i])) > str) i--;
		} else {
			// VERY VERY VERY NASTY - we need to somehow define operators < > for TYPE
			while (*((uint16 *)(&pArr[i])) > str) j++;
			while (*((uint16 *)(&pArr[i])) < str) i--;
		}

		if ( i >= j ) {

			if ( i != j ) {
				TYPE zal;
				zal = pArr[i];
				pArr[i] = pArr[j];
				pArr[j] = zal;

			}

			i--;
			j++;
		}
	} while (j <= i);

	if (d < i) QuickSortRecursive(pArr,d,i,bAscending);
	if (j < h) QuickSortRecursive(pArr,j,h,bAscending);
}

template <class TYPE> bool QuickSort(TYPE *pArr, int iSize, bool bAscending = TRUE)
{
	bool rc = TRUE;

	if (iSize > 1) {

		try {

			int	low = 0,
				high = iSize - 1;

			QuickSortRecursive(pArr,low,high,bAscending);

		} catch (...) {
			rc = FALSE;
		}

	} else {
		rc = FALSE;
	}

	return rc;
}

template <class BASE_CLASS, class TYPE> 
class CQArray : public CArray <BASE_CLASS, TYPE>
{
public:
	// WARNING!!!! IT'LL CRASH IF TYPE <>uint16
	void QuickSort(CQArray* pArr,bool bAscending = TRUE);
};

template <class BASE_CLASS, class TYPE> void CQArray<BASE_CLASS, TYPE>::QuickSort(CQArray* pArr,bool bAscending/* = TRUE*/)
{
	if (this->GetSize() > 1) {
		::QuickSort(pArr,this->GetSize(),bAscending);
	}
}

#endif

#endif // CARRAY_H
