// This file is part of the aMule project.
//
// Copyright (c) 2003, Michael Schikora <schiko@schikos.de>
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

// Ini.h: Schnittstelle für die Klasse CIni.

// Autor: Michael Schikora
// Mail:  schiko@schikos.de
//
// If you found this code useful,
// please let me know
//
// How to use:
//
//
//void CMyClass::UpdateFromIni(bool bFromIni)
//{
//   CIni ini(m_strFileName,m_strSection);
//   ini.SER_GET(bFromIni,m_nValueXY); 
//   ini.SER_GET(bFromIni,m_strValue);
//   ini.SER_ARR(bFromIni,m_arValue,MAX_AR); 
//   ini.SER_ARR(bFromIni,m_ar3D,3);
//   //ore with default values 
//   ini.SER_GETD(bFromIni,m_nValueXY,5); 
//   ini.SER_GETD(bFromIni,m_strValue,"Hello");
//   ini.SER_ARRD(bFromIni,m_arValue,MAX_AR,10); 
//   ini.SER_ARRD(bFromIni,m_ar3D,3,5); 
//}

#ifndef INI2_H
#define INI2_H

#include <wx/string.h>		// Needed for wxString
#include <wx/gdicmn.h>		// Needed for wxPoint
#include "types.h"		// Needed for WORD	
#include "color.h"		// Needed for RGB

#define SER_GET(bGet,value) SerGet(bGet,value,#value)
#define SER_ARR(bGet,value,n) SerGet(bGet,value,n,#value)
#define SER_GETD(bGet,value,default) SerGet(bGet,value,#value,NULL,default)
#define SER_ARRD(bGet,value,n,default) SerGet(bGet,value,n,#value,default)

class CIni {
public:

#ifdef __NEVER_DEFINED__
   // MAKRO: SerGet(bGet,value,#value)
   wxInt16 SER_GET(bool bGet,wxInt16 value);
   // MAKRO: SerGet(bGet,value,n,#value)
   wxInt16 SER_ARR(bGet,wxInt16* value,wxInt32 n);
#endif
   // If the IniFilename contains no path,
   // the module-directory will be add to the FileName,
   // to avoid storing in the windows-directory
   // bModulPath=TRUE: ModulDir, bModulPath=FALSE: CurrentDir
   static void AddModulPath(wxString& strFileName,bool bModulPath = TRUE);
   static wxString GetDefaultSection();
   static wxString GetDefaultIniFile(bool bModulPath = TRUE);

	CIni( bool bModulPath = TRUE);
	CIni(CIni const& Ini, bool bModulPath = TRUE);
	CIni(wxString const& strFileName, bool bModulPath = TRUE);
	CIni(wxString const& strFileName, wxString const& strSection, bool bModulPath = TRUE);
	virtual ~CIni();

	void SetFileName(wxString const& strFileName);
	void SetSection(wxString const& strSection);
	wxString const& GetFileName() const;
	wxString const& GetSection() const;
private:
	void Init(LPCSTR strIniFile, LPCSTR strSection = NULL);
public:
	wxString	GetString(wxString strEntry,	LPCSTR strDefault=NULL,			LPCSTR strSection = NULL);

	double		GetDouble(wxString strEntry,	double fDefault = 0.0,			LPCSTR strSection = NULL);
	float		GetFloat(wxString strEntry,	float fDefault = 0.0,			LPCSTR strSection = NULL);
	sint32	GetInt(wxString strEntry,	sint32 nDefault = 0,			LPCSTR strSection = NULL);
	WORD		GetWORD(wxString strEntry,	WORD nDefault = 0,			LPCSTR strSection = NULL);
	bool		GetBool(wxString strEntry,	bool bDefault = FALSE,			LPCSTR strSection = NULL);
	wxPoint		GetPoint(wxString strEntry,	wxPoint ptDefault = wxPoint(0,0),	LPCSTR strSection = NULL);
	wxRect		GetRect(wxString strEntry,	wxRect rectDefault = wxRect(0,0,0,0),	LPCSTR strSection = NULL);
	COLORREF	GetColRef(wxString strEntry,	COLORREF crDefault = 0x808080,	LPCSTR strSection = NULL);

	void		WriteString(wxString strEntry,wxString		str,		LPCSTR strSection = NULL);
	void		WriteDouble(wxString strEntry,double		f,		LPCSTR strSection = NULL);
	void		WriteFloat(wxString strEntry,float		f,		LPCSTR strSection = NULL);
	void		WriteInt(wxString strEntry,sint32			n,		LPCSTR strSection = NULL);
	void		WriteWORD(wxString strEntry,WORD		n,		LPCSTR strSection = NULL);
	void		WriteBool(wxString strEntry,bool		b,		LPCSTR strSection = NULL);
	void		WritePoint(wxString strEntry,wxPoint		pt,		LPCSTR strSection = NULL);
	void		WriteRect(wxString strEntry,wxRect		rect,		LPCSTR strSection = NULL);
	void		WriteColRef(wxString strEntry,COLORREF		cr,		LPCSTR strSection = NULL);

	void		SerGetString(	bool bGet,wxString	& str,	wxString strEntry,	LPCSTR strSection = NULL,	LPCSTR strDefault=NULL);
	void		SerGetDouble(	bool bGet,double	& f,	wxString strEntry,	LPCSTR strSection = NULL,	double fDefault = 0.0);
	void		SerGetFloat(	bool bGet,float		& f,	wxString strEntry,	LPCSTR strSection = NULL,	float fDefault = 0.0);
	void		SerGetInt(	bool bGet,sint32		& n,	wxString strEntry,	LPCSTR strSection = NULL,	sint32 nDefault = 0);
	void		SerGetDWORD(	bool bGet,DWORD		& n,	wxString strEntry,	LPCSTR strSection = NULL,	DWORD nDefault = 0);
	void		SerGetBool(	bool bGet,bool		& b,	wxString strEntry,	LPCSTR strSection = NULL,	bool bDefault = FALSE);
	void		SerGetPoint(	bool bGet,wxPoint	& pt,	wxString strEntry,	LPCSTR strSection = NULL,	wxPoint ptDefault = wxPoint(0,0));
	void		SerGetRect(	bool bGet,wxRect	& rect,	wxString strEntry,	LPCSTR strSection = NULL,	wxRect rectDefault = wxRect(0,0,0,0));
	void		SerGetColRef(	bool bGet,COLORREF	& cr,	wxString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(0,0,0));

	void		SerGet(	bool bGet,wxString	& str,	wxString strEntry,	LPCSTR strSection = NULL,	LPCSTR strDefault=NULL);
	void		SerGet(	bool bGet,double	& f,	wxString strEntry,	LPCSTR strSection = NULL,	double fDefault = 0.0);
	void		SerGet(	bool bGet,float		& f,	wxString strEntry,	LPCSTR strSection = NULL,	float fDefault = 0.0);
	void		SerGet(	bool bGet,sint32		& n,	wxString strEntry,	LPCSTR strSection = NULL,	sint32 nDefault = 0);
	void		SerGet(	bool bGet,sint16		& n,	wxString strEntry,	LPCSTR strSection = NULL,	sint16 nDefault = 0);
	void		SerGet(	bool bGet,DWORD		& n,	wxString strEntry,	LPCSTR strSection = NULL,	DWORD nDefault = 0);
	void		SerGet(	bool bGet,WORD		& n,	wxString strEntry,	LPCSTR strSection = NULL,	WORD nDefault = 0);
//	void		SerGet(	bool bGet,bool		& b,	wxString strEntry,	LPCSTR strSection = NULL,	bool bDefault = FALSE);
	void		SerGet(	bool bGet,wxPoint	& pt,	wxString strEntry,	LPCSTR strSection = NULL,	wxPoint ptDefault = wxPoint(0,0));
	void		SerGet(	bool bGet,wxRect	& rect,	wxString strEntry,	LPCSTR strSection = NULL,	wxRect rectDefault = wxRect(0,0,0,0));
//	void		SerGet(	bool bGet,COLORREF	& cr,	wxString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(0,0,0));
   
//ARRAYs
	void		SerGet(	bool bGet,wxString	* str,	int nCount, wxString strEntry, LPCSTR strSection = NULL, LPCSTR strDefault = NULL);
	void		SerGet(	bool bGet,double	* f,	int nCount, wxString strEntry, LPCSTR strSection = NULL, double fDefault = 0.0);
	void		SerGet(	bool bGet,float		* f,	int nCount, wxString strEntry, LPCSTR strSection = NULL, float fDefault = 0.0);
	void		SerGet(	bool bGet,unsigned char	* n,    int nCount, wxString strEntry, LPCSTR strSection = NULL, unsigned char nDefault = 0);
	void		SerGet(	bool bGet,sint32		* n,	int nCount, wxString strEntry, LPCSTR strSection = NULL, sint32 nDefault = 0);
	void		SerGet(	bool bGet,sint16		* n,	int nCount, wxString strEntry, LPCSTR strSection = NULL, sint16 nDefault = 0);
	void		SerGet(	bool bGet,DWORD		* n,	int nCount, wxString strEntry, LPCSTR strSection = NULL, DWORD nDefault = 0);
	void		SerGet(	bool bGet,WORD		* n,	int nCount, wxString strEntry, LPCSTR strSection = NULL, WORD nDefault = 0);
	void		SerGet(	bool bGet,wxPoint	* pt,	int nCount, wxString strEntry, LPCSTR strSection = NULL, wxPoint ptDefault = wxPoint(0,0));
	void		SerGet(	bool bGet,wxRect	* rect,	int nCount, wxString strEntry, LPCSTR strSection = NULL, wxRect rectDefault = wxRect(0,0,0,0));

	int			Parse(wxString &strIn, int nOffset, wxString &strOut);
   //MAKRO :
   //SERGET(bGet,value) SerGet(bGet,value,#value)

private:
	wxChar* GetLPCSTR(wxString strEntry,const wxChar* strSection,const wxChar* strDefault);
   bool  m_bModulPath;  //TRUE: Filenames without path take the Modulepath
                        //FALSE: Filenames without path take the CurrentDirectory

#define MAX_INI_BUFFER 256
	char	m_chBuffer[MAX_INI_BUFFER];
	wxString m_strFileName;
	wxString m_strSection;
//////////////////////////////////////////////////////////////////////
// statische Methoden
//////////////////////////////////////////////////////////////////////
public:
	static wxString	Read( wxString const& strFileName, wxString const& strSection, wxString const& strEntry, wxString const& strDefault);
	static void		Write(wxString const& strFileName, wxString const& strSection, wxString const& strEntry, wxString const& strValue);
};

#endif // INI2_H
