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

#include <wx/config.h>
#include <wx/string.h>
#include <wx/window.h>

#include "ini2.h"		// Interface declarations.
#include "color.h"		// Needed for GetRValue, GetBValue and GetGValue

// If the IniFilename contains no path,
// the module-directory will be add to the FileName,
// to avoid storing in the windows-directory
/*static*/ void CIni::AddModulPath(wxString& strFileName,bool bModulPath /*= TRUE*/)
{
#if 0
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

   _splitpath( strFileName, drive, dir, fname, ext );
   if( ! drive[0]  )
   {
      //PathCanonicalize(..) doesn't work with for all Plattforms !
      wxString strModule;
      if( bModulPath )
      {
         GetModuleFileName(NULL,strModule.GetBuffer(MAX_INI_BUFFER),MAX_INI_BUFFER);
         strModule.ReleaseBuffer();
      }
      else
      {
         GetCurrentDirectory(MAX_INI_BUFFER,strModule.GetBuffer(MAX_INI_BUFFER));
         strModule.ReleaseBuffer();
         // fix by "cpp@world-online.no"
         strModule.TrimRight('\\');
         strModule.TrimRight('/');
         strModule += "\\";
      }
      strModule.ReleaseBuffer();
      _splitpath( strModule, drive, dir, fname, ext );
      strModule = drive;
      strModule+= dir;
      strModule+= strFileName;
      strFileName = strModule;
   }
#endif
}
/*static*/ wxString CIni::GetDefaultSection()
{
  return wxString("eMule");
  //   return AfxGetAppName();
}
/*static*/ wxString CIni::GetDefaultIniFile(bool bModulPath /*= TRUE*/)
{
#if 0
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];
   wxString strTemp;
   wxString strApplName;
   GetModuleFileName(NULL,strTemp.GetBuffer(MAX_INI_BUFFER),MAX_INI_BUFFER);
   strTemp.ReleaseBuffer();
   _splitpath( strTemp, drive, dir, fname, ext );
   strTemp = fname; //"ApplName"
   strTemp += ".ini";  //"ApplName.ini"
   if( bModulPath )
   {
      strApplName  = drive;
      strApplName += dir;
      strApplName += strTemp;
   }
   else
   {
      GetCurrentDirectory(MAX_INI_BUFFER,strApplName.GetBuffer(MAX_INI_BUFFER));
      strApplName.ReleaseBuffer();
      strApplName.TrimRight('\\');
      strApplName.TrimRight('/');
      strApplName += "\\";
      strApplName += strTemp;
   }
   return strApplName;
#endif
   return "perse";
}
//////////////////////////////////////////////////////////////////////
// Konstruktion/Destruktion
//////////////////////////////////////////////////////////////////////
// Creates/Use file : "Drive:\ApplPath\ApplName.ini"
CIni::CIni(bool bModulPath /*= TRUE*/):
   m_bModulPath(bModulPath)
{
   m_strFileName = GetDefaultIniFile(m_bModulPath);
   m_strSection  = GetDefaultSection();
}
CIni::CIni(CIni const& Ini, bool bModulPath /*= TRUE*/):
	m_bModulPath(Ini.m_bModulPath),
	m_strFileName(Ini.m_strFileName),
	m_strSection(Ini.m_strSection)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,m_bModulPath);
   if(m_strSection.IsEmpty())
      m_strSection = GetDefaultSection();
}
CIni::CIni(wxString const& strFileName, bool bModulPath /*= TRUE*/):
	m_bModulPath(bModulPath),
	m_strFileName(strFileName)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,bModulPath);
   m_strSection = GetDefaultSection();
}
CIni::CIni(wxString const& strFileName, wxString const& strSection, bool bModulPath /*= TRUE*/):
	m_bModulPath(bModulPath),
	m_strFileName(strFileName),
	m_strSection(strSection)
{
   if(m_strFileName.IsEmpty())
      m_strFileName = GetDefaultIniFile(m_bModulPath);
   AddModulPath(m_strFileName,bModulPath);
   if(m_strSection.IsEmpty())
      m_strSection = GetDefaultSection();
}

CIni::~CIni()
{
}
//////////////////////////////////////////////////////////////////////
// Zugriff auf Quelle/Ziel von IO-Operationen
//////////////////////////////////////////////////////////////////////
void CIni::SetFileName(wxString const& strFileName)
{
	m_strFileName = strFileName;
   AddModulPath(m_strFileName);
}
void CIni::SetSection(wxString const& strSection)
{
	m_strSection = strSection;
}

wxString const& CIni::GetFileName() const
{
	return m_strFileName;
}
wxString const& CIni::GetSection() const
{
	return m_strSection;
}
//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////


void CIni::Init( LPCSTR strFileName, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL)
		m_strSection = strSection;
	if(strFileName != NULL)
		m_strFileName = strFileName;
}
wxString CIni::GetString(wxString strEntry,LPCSTR strDefault/*=NULL*/,LPCSTR strSection/* = NULL*/)
{
	if(strDefault == NULL)
		return wxString(GetLPCSTR(strEntry,strSection,""));
	else
		return wxString(GetLPCSTR(strEntry,strSection,strDefault));
}
double CIni::GetDouble(wxString strEntry,double fDefault/* = 0.0*/,LPCSTR strSection/* = NULL*/)
{
	wxString strDefault;
	//strDefault.Format("%g",fDefault);
	strDefault=wxString::Format("%g",fDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return atof(m_chBuffer);
}
float CIni::GetFloat(wxString strEntry,float fDefault/* = 0.0*/, LPCSTR strSection/* = NULL*/)
{
	wxString strDefault;
	//strDefault.Format("%g",fDefault);
	strDefault=wxString::Format("%g",fDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return (float)atof(m_chBuffer);
}
sint32 CIni::GetInt(wxString strEntry,sint32 nDefault/* = 0*/,LPCSTR strSection/* = NULL*/)
{
	wxString strDefault;
	//strDefault.Format("%d",nDefault);
	strDefault=wxString::Format("%d",nDefault);
	GetLPCSTR(strEntry,strSection,strDefault.GetData());
	return atoi(m_chBuffer);
}
WORD CIni::GetWORD(wxString strEntry,WORD nDefault/* = 0*/,LPCSTR strSection/* = NULL*/)
{
	wxString strDefault;
	//strDefault.Format("%u",nDefault);
	strDefault=wxString::Format("%u",nDefault);
	GetLPCSTR(strEntry,strSection,strDefault);
	return (WORD)atoi(m_chBuffer);
}
bool CIni::GetBool(wxString strEntry,bool bDefault/* = FALSE*/,LPCSTR strSection/* = NULL*/)
{
	wxString strDefault;
	//strDefault.Format("%d",bDefault);
	strDefault=wxString::Format("%d", bDefault ? 1 : 0);
	GetLPCSTR(strEntry,strSection,strDefault);
	return ( atoi(m_chBuffer) != 0 );
}
wxPoint CIni::GetPoint(wxString strEntry,	wxPoint ptDefault, LPCSTR strSection)
{
	wxPoint ptReturn=ptDefault;

	wxString strDefault;
	strDefault=wxString::Format("(%d,%d)",ptDefault.x,ptDefault.y);
	//strDefault.Format("(%d,%d)",ptDefault.x, ptDefault.y);

	wxString strPoint = GetString(strEntry,(char*)strDefault.GetData());
	sscanf(strPoint,"(%d,%d)", &ptReturn.x, &ptReturn.y);

	return ptReturn;
}
wxRect CIni::GetRect(wxString strEntry, wxRect rectDefault, LPCSTR strSection)
{
#if 0
	wxRect rectReturn=rectDefault;

	wxString strDefault;
	//old version :strDefault.Format("(%d,%d,%d,%d)",rectDefault.top,rectDefault.left,rectDefault.bottom,rectDefault.right);
	strDefault.Format("%d,%d,%d,%d",rectDefault.left,rectDefault.top,rectDefault.right,rectDefault.bottom);

	wxString strRect = GetString(strEntry,strDefault);
	int nRead = 0;
   //new Version found
   if( 4==sscanf(strRect,"%d,%d,%d,%d",&rectDefault.left,&rectDefault.top,&rectDefault.right,&rectDefault.bottom))
	   return rectReturn;
   //old Version found
   sscanf(strRect,"(%d,%d,%d,%d)", &rectReturn.top,&rectReturn.left,&rectReturn.bottom,&rectReturn.right);
	return rectReturn;
#endif
	return wxRect(0,0,0,0);
}
COLORREF CIni::GetColRef(wxString strEntry, COLORREF crDefault, LPCSTR strSection)
{
	int32 temp[3]={	GetRValue(crDefault),
					GetGValue(crDefault),
					GetBValue(crDefault)};

	wxString strDefault;
	strDefault=wxString::Format("RGB(%hd,%hd,%hd)",temp[0],temp[1],temp[2]);

	wxString strColRef = GetString(strEntry,(char*)strDefault.GetData());
	sscanf(strColRef,"RGB(%d,%d,%d)", temp, temp+1, temp+2);

	return RGB(temp[0],temp[1],temp[2]);
}
	
void WritePrivateProfileString(const char* sec,const char* entry,const char*jottae ,const char* d)
{
  char buffer[512];
  sprintf(buffer,"%s/%s",sec,entry);
  wxConfigBase* cb=wxConfigBase::Get(TRUE);
  cb->Write(buffer,jottae);
  cb->Flush();
}

void CIni::WriteString(wxString strEntry,wxString	str, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	WritePrivateProfileString(m_strSection,strEntry,str,m_strFileName);
}
void CIni::WriteDouble(wxString strEntry,double f, LPCSTR strSection/*= NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("%g",f);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteFloat(wxString strEntry,float f, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("%g",f);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteInt(wxString strEntry,sint32 n, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("%d",n);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteWORD(wxString strEntry,WORD n, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("%u",n);
		WritePrivateProfileString(m_strSection,strEntry,strBuffer,m_strFileName);
}
void CIni::WriteBool(wxString strEntry,bool b, LPCSTR strSection/* = NULL*/)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("%d",b ? 1 : 0);
		WritePrivateProfileString(m_strSection, strEntry, strBuffer, m_strFileName);
}
void CIni::WritePoint(wxString strEntry,wxPoint pt, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("(%d,%d)",pt.x,pt.y);
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
void CIni::WriteRect(wxString strEntry,wxRect rect, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	//strBuffer.Format("(%d,%d,%d,%d)",rect.top,rect.left,rect.bottom,rect.right);
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
void CIni::WriteColRef(wxString strEntry,COLORREF cr, LPCSTR strSection)
{
	if(strSection != NULL) 
		m_strSection = strSection;
	wxString strBuffer;
	strBuffer=wxString::Format("RGB(%d,%d,%d)",GetRValue(cr), GetGValue(cr), GetBValue(cr));
	Write(m_strFileName,m_strSection,strEntry,strBuffer);
}
wxChar* CIni::GetLPCSTR(wxString strEntry, const wxChar* strSection, const wxChar* strDefault)
{
  wxConfigBase* cb=wxConfigBase::Get(TRUE);
  char buffer[512];
  if(strSection)
    m_strSection=wxString(strSection);

  sprintf(buffer,"%s/%s",m_strSection.GetData(),strEntry.GetData());

  wxString stringu=cb->Read(wxString(buffer),wxString(strDefault));
  cb->Flush();
  memcpy(m_chBuffer,stringu.GetData(),stringu.Length()+1);
  return m_chBuffer;
#if 0
	// evtl Section neu setzen
	if(strSection != NULL)
		m_strSection = strSection;

	wxString temp;
	if(strDefault == NULL)
		temp = Read(m_strFileName,m_strSection,strEntry,wxString());
	else
		temp = Read(m_strFileName,m_strSection,strEntry,strDefault);

	return (wxChar*)memcpy(m_chBuffer,(LPCTSTR)temp.GetData(),temp.Length()+1);// '+1' damit die Null am Ende mit kopiert wird
#endif

}
void CIni::SerGetString(	bool bGet,wxString &	str,wxString strEntry,LPCSTR strSection,LPCSTR strDefault)
{
	if(bGet)
		str = GetString(strEntry,strDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteString(strEntry,str, strSection/* = NULL*/);
}
void CIni::SerGetDouble(	bool bGet,double&	f,	wxString strEntry,LPCSTR strSection/* = NULL*/,double fDefault/* = 0.0*/)
{
	if(bGet)
		f = GetDouble(strEntry,fDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteDouble(strEntry,f, strSection/* = NULL*/);
}
void CIni::SerGetFloat(		bool bGet,float	&	f,	wxString strEntry, LPCSTR strSection/* = NULL*/,float fDefault/* = 0.0*/)
{
	if(bGet)
		f = GetFloat(strEntry,fDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteFloat(strEntry,f, strSection/* = NULL*/);
}
void CIni::SerGetInt(		bool bGet,sint32	&	n,	wxString strEntry,LPCSTR strSection/* = NULL*/,sint32 nDefault/* = 0*/)
{
	if(bGet)
		n = GetInt(strEntry,nDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteInt(strEntry,n, strSection/* = NULL*/);
}
void CIni::SerGetDWORD(		bool bGet,DWORD	&	n,	wxString strEntry,LPCSTR strSection/* = NULL*/,DWORD nDefault/* = 0*/)
{
	if(bGet)
		n = (DWORD)GetInt(strEntry,nDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteInt(strEntry,n, strSection/* = NULL*/);
}
void CIni::SerGetBool(		bool bGet,bool	&	b,	wxString strEntry,LPCSTR strSection/* = NULL*/,bool bDefault/* = FALSE*/)
{
	if(bGet)
		b = GetBool(strEntry,bDefault/*=NULL*/,strSection/* = NULL*/);
	else
		WriteBool(strEntry,b, strSection/* = NULL*/);
}

void CIni::SerGetPoint(	bool bGet,wxPoint	& pt,	wxString strEntry,	LPCSTR strSection,	wxPoint ptDefault)
{
	if(bGet)
		pt = GetPoint(strEntry,ptDefault,strSection);
	else
		WritePoint(strEntry,pt, strSection);
}
void CIni::SerGetRect(		bool bGet,wxRect		& rect,	wxString strEntry,	LPCSTR strSection,	wxRect rectDefault)
{
	if(bGet)
		rect = GetRect(strEntry,rectDefault,strSection);
	else
		WriteRect(strEntry,rect, strSection);
}
void CIni::SerGetColRef(	bool bGet,COLORREF	& cr,	wxString strEntry,	LPCSTR strSection,	COLORREF crDefault)
{
	if(bGet)
		cr = GetColRef(strEntry,crDefault,strSection);
	else
		WriteColRef(strEntry,cr, strSection);
}
// Überladene Methoden //////////////////////////////////////////////////////////////////////////////////////////////////77
// Einfache Typen /////////////////////////////////////////////////////////////////////////////////////////////////////////
void		CIni::SerGet(	bool bGet,wxString	& str,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	LPCSTR strDefault/*= NULL*/)
{
   SerGetString(bGet,str,strEntry,strSection,strDefault);
}
void		CIni::SerGet(	bool bGet,double	& f,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	double fDefault/* = 0.0*/)
{
   SerGetDouble(bGet,f,strEntry,strSection,fDefault);
}
void		CIni::SerGet(	bool bGet,float		& f,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	float fDefault/* = 0.0*/)
{
   SerGetFloat(bGet,f,strEntry,strSection,fDefault);
}
void		CIni::SerGet(	bool bGet,sint32		& n,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	sint32 nDefault/* = 0*/)
{
   SerGetInt(bGet,n,strEntry,strSection,nDefault);
}
void		CIni::SerGet(	bool bGet,sint16		& n,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	sint16 nDefault/* = 0*/)
{
   int nTemp = n;
   SerGetInt(bGet,nTemp,strEntry,strSection,nDefault);
   n = nTemp;
}
void		CIni::SerGet(	bool bGet,DWORD		& n,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	DWORD nDefault/* = 0*/)
{
   SerGetDWORD(bGet,n,strEntry,strSection,nDefault);
}
void		CIni::SerGet(	bool bGet,WORD		& n,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	WORD nDefault/* = 0*/)
{
   DWORD dwTemp = n;
   SerGetDWORD(bGet,dwTemp,strEntry,strSection,nDefault);
   n = dwTemp;
}
//	void		SerGet(	bool bGet,bool		& b,	wxString strEntry,	LPCSTR strSection = NULL,	bool bDefault = FALSE);
void		CIni::SerGet(	bool bGet,wxPoint	& pt,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	wxPoint ptDefault/* = wxPoint(0,0)*/)
{
   SerGetPoint(bGet,pt,strEntry,strSection,ptDefault);
}
void		CIni::SerGet(	bool bGet,wxRect		& rect,	wxString strEntry,	LPCSTR strSection/*= NULL*/,	wxRect rectDefault/* = wxRect(0,0,0,0)*/)
{
   SerGetRect(bGet,rect,strEntry,strSection,rectDefault);
}
//	void		SerGet(	bool bGet,COLORREF	& cr,	wxString strEntry,	LPCSTR strSection = NULL,	COLORREF crDefault = RGB(128,128,128));

// Überladene Methoden ////////////////////////////////////////////////////////////////////////////////////////////
// ARRAYS /////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entries werden durch Unterstrich + Index ergenzt////////////////////////////////////////////////////////////////
void CIni::SerGet(bool bGet, wxString *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, LPCSTR Default/*=NULL*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, ar[i]);
				if(ar[i].Length() == 0)
					ar[i] = Default;
			}

		} else {
			strBuffer = ar[0];
			for(int i = 1; i < nCount; i++) {
			  strBuffer=strBuffer+","; //.AppendChar(',');
			  strBuffer.Append(ar[i]);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}

void CIni::SerGet(bool bGet, double *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, double Default/* = 0.0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = atof(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%g", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%g", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, float *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, float Default/* = 0.0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = (float)atof(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%g", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%g", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, sint32 *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, sint32 Default/* = 0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = atoi(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%d", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, unsigned char *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, unsigned char Default/* = 0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = (unsigned char)atoi(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%d", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, sint16 *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, sint16 Default/* = 0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = (short)atoi(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%d", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, DWORD *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, DWORD Default/* = 0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = (DWORD)atoi(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%u", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%u", ar[i]);
				//strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void CIni::SerGet(bool bGet, WORD *ar, int nCount, wxString strEntry, LPCSTR strSection/*=NULL*/, WORD Default/* = 0*/)
{
	if(nCount > 0) {
		wxString strBuffer;
		if(bGet) {
			strBuffer = GetString(strEntry, "", strSection);
			wxString strTemp;
			int nOffset = 0;
			for(int i = 0; i < nCount; i++) {
				nOffset = Parse(strBuffer, nOffset, strTemp);
				if(strTemp.Length() == 0)
					ar[i] = Default;
				else
					ar[i] = (WORD)atoi(strTemp);
			}

		} else {
			wxString strTemp;
			strBuffer=wxString::Format("%d", ar[0]);
			for(int i = 1; i < nCount; i++) {
				strTemp=wxString::Format("%d", ar[i]);
				//				strBuffer.AppendChar(',');
				strBuffer=strBuffer+",";
				strBuffer.Append(strTemp);
			}
			WriteString(strEntry, strBuffer, strSection);
		}
	}
}
void		CIni::SerGet(	bool bGet,wxPoint	* ar,	   int nCount, wxString strEntry,	LPCSTR strSection/*=NULL*/,	wxPoint Default/* = wxPoint(0,0)*/)
{
   wxString strBuffer;
   for( int i=0 ; i<nCount ; i++)
   {
      strBuffer=wxString::Format("_%i",i);
      strBuffer = strEntry + strBuffer;
      SerGet(bGet,ar[i],strBuffer,strSection,Default);
   }
}
void		CIni::SerGet(	bool bGet,wxRect	* ar,	   int nCount, wxString strEntry,	LPCSTR strSection/*=NULL*/,	wxRect Default/* = wxRect(0,0,0,0)*/)
{
   wxString strBuffer;
   for( int i=0 ; i<nCount ; i++)
   {
      strBuffer=wxString::Format("_%i",i);
      strBuffer = strEntry + strBuffer;
      SerGet(bGet,ar[i],strBuffer,strSection,Default);
   }
}

int			CIni::Parse(wxString &strIn, int nOffset, wxString &strOut) {
	strOut.Clear();
	int nLength = strIn.Length();

	if(nOffset < nLength) {
		if(nOffset != 0 && strIn.GetChar(nOffset) == ',')
			nOffset++;

		while(nOffset < nLength) {
			if(!isspace(strIn.GetChar(nOffset)))
				break;

			nOffset++;
		}

		while(nOffset < nLength) {
			strOut += strIn.GetChar(nOffset);

			if(strIn.GetChar(++nOffset) == ',')
				break;
		}

		strOut.Trim();
	}
	return nOffset;
}

void GetPrivateProfileString(const wxChar*a0,const wxChar* a,const wxChar* b,wxChar* ret,long len,const wxChar* name)
{
  printf("todo. gpps\n");
}

//////////////////////////////////////////////////////////////////////
// statische Methoden
//////////////////////////////////////////////////////////////////////
wxString CIni::Read(wxString const& strFileName, wxString const& strSection, wxString const& strEntry, wxString const& strDefault)
{
	wxString strReturn;
	GetPrivateProfileString(strSection,
							strEntry,
							strDefault,
				(wxChar*)strReturn.GetData(), //.GetBufferSetLength(MAX_INI_BUFFER),
							MAX_INI_BUFFER,
							strFileName);
	//strReturn.ReleaseBuffer();
	return strReturn;
}
void CIni::Write(wxString const& strFileName, wxString const& strSection, wxString const& strEntry, wxString const& strValue)
{
	WritePrivateProfileString(strSection,
							strEntry,
							strValue,
							strFileName);
}
