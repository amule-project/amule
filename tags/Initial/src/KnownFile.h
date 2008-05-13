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

#ifndef KNOWNFILE_H
#define KNOWNFILE_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dc.h>		// Needed for wxDC
#include <wx/gdicmn.h>		// Needed for wxRect

#include "types.h"		// Needed for int8, uint8, uint16, uint32 and uint64
#include "CString.h"		// Needed for CString
#include "opcodes.h"		// Needed for PARTSIZE
#include "CArray.h"		// Needed for CArray
#include "CTypedPtrList.h"	// Needed for CTypedPtrList

class CUpDownClient;
class CFile;
class Packet;
class CTag;
class CBarShader;

class CFileStatistic {
	friend class CKnownFile;
public:
	CFileStatistic()					{requested = transfered = accepted = alltimerequested= alltimetransferred = alltimeaccepted = 0;}
	void	AddRequest();
	void	AddAccepted();
	void    AddTransferred(uint64 bytes);
	uint16	GetRequests()				{return requested;}
	uint16	GetAccepts()				{return accepted;}
	uint64  GetTransfered()				{return transfered;}
	uint16	GetAllTimeRequests()		{return alltimerequested;}
	uint16	GetAllTimeAccepts()			{return alltimeaccepted;}
	uint64	GetAllTimeTransfered()		{return alltimetransferred;}
	CKnownFile* fileParent;

private:
	uint16 requested;
	uint64 transfered;
	uint16 accepted;
	uint32 alltimerequested;
	uint64 alltimetransferred;
	uint32 alltimeaccepted;
};

/*
					   CPartFile
					 /
		  CKnownFile
		/
CAbstractFile 
		\ 
		  CSearchFile
*/
class CAbstractFile
{
public:
	CAbstractFile();
	virtual ~CAbstractFile()
	{
//		if( m_strFileName != NULL )
//			delete[] m_strFileName;
	}

//	const char*	GetFileName()			{return m_strFileName;}
	const wxString&	GetFileName()			{return m_strFileName;}
//	unsigned char*	GetFileHash()			{return m_abyFileHash;}
	unsigned char*	GetFileHash()			{return m_abyFileHash;}
	uint32	GetFileSize()			{return m_nFileSize;}
	void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }
	uint32*	GetFileTypePtr()		{return &m_iFileType;}
	uint32	GetFileType()			{return m_iFileType;}
	void	SetFileName(LPCTSTR pszFilename, bool bReplaceInvalidFileSystemChars = false); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!
	
protected:
//	char*	m_strFileName;
	wxString	m_strFileName;
	unsigned char	m_abyFileHash[16];
	uint32	m_nFileSize;
	uint32	m_iFileType;
	CString m_strComment;
	int8	m_iRate; //for rate 
};

class CKnownFile : public CAbstractFile
{
public:
	CKnownFile();
	~CKnownFile();

	virtual bool	CreateFromFile(char* directory,char* filename, volatile int const * notify); // create date, hashset and tags from a file
	uint32*	GetFileTypePtr()		{return &m_iFileType;}
	char*	GetPath()				{return directory;}
	void	SetPath(LPCTSTR path);
	void SetFilePath(LPCTSTR pszFilePath);	
	const CString& GetFilePath() const { return m_strFilePath; }
	
	virtual	bool	IsPartFile()	{return false;}
	virtual bool	LoadFromFile(CFile* file);	//load date, hashset and tags from a .met file
	bool	WriteToFile(CFile* file);	
	uint32	GetFileDate()	{return date;}
	time_t*	GetCFileDate()	{return (time_t*)&date;}
	uint32	GetCrFileDate()	{return dateC;}
	time_t*	GetCrCFileDate()	{return (time_t*)&dateC; }
	
	void SetFileSize(uint32 nFileSize);

	// local available part hashs
	uint16	GetHashCount() const	{return hashlist.GetCount();}
	uchar*	GetPartHash(uint16 part) const;

	// nr. of part hashs according the file size wrt ED2K protocol
	UINT	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

	// nr. of 9MB parts (file data)
	__inline uint16 GetPartCount() const { return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	__inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }

	uint32	date;
	// Kry - 0.30 import
	uint32	dateC;
	
	
	// file upload priority
	uint8	GetUpPriority()			{return m_iUpPriority;}
	void	SetUpPriority(uint8 newUpPriority, bool bSave=true);
	bool	IsAutoUpPriority()		{return m_bAutoUpPriority;}
	void	SetAutoUpPriority(bool flag)	{m_bAutoUpPriority = flag;}
	void	UpdateAutoUpPriority();
	void	AddQueuedCount() {m_iQueuedCount++; UpdateAutoUpPriority();};
	void	SubQueuedCount() {if(m_iQueuedCount) m_iQueuedCount--; UpdateAutoUpPriority();}
	uint32	GetQueuedCount() {return m_iQueuedCount;}

	// shared file view permissions (all, only friends, no one)
	uint8	GetPermissions(void)	{ return m_iPermissions; };
	void	SetPermissions(uint8 iNewPermissions) {m_iPermissions = iNewPermissions;};

	bool	LoadHashsetFromFile(CFile* file, bool checkhash);
	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	void	NewAvailPartsInfo();
	void	DrawShareStatusBar(wxDC* dc, wxRect rect, bool onlygreyrect, bool bFlat);

	CFileStatistic statistic;
	// comment 
	CString	GetFileComment()		{if (!m_bCommentLoaded) LoadComment(); return m_strComment;} 
	void	SetFileComment(CString strNewComment);
	void	SetFileRate(int8 iNewRate); 
	int8	GetFileRate()			{if (!m_bCommentLoaded) LoadComment(); return m_iRate;}
	void	SetPublishedED2K( bool val );
	bool	GetPublishedED2K()	{return m_PublishedED2K;}
	
	// file sharing
	virtual	Packet*	CreateSrcInfoPacket(CUpDownClient* forClient);
	
	#ifdef RELEASE_GROUP_MODE
	uint32	totalupload;
	#endif	
	
protected:
	bool	LoadTagsFromFile(CFile* file);
	bool	LoadDateFromFile(CFile* file);
	void	CreateHashFromFile(FILE* file, int Length, unsigned char* Output)	{CreateHashFromInput(file,0,Length,Output,0);}
	void	CreateHashFromFile(CFile* file, int Length, unsigned char* Output)	{CreateHashFromInput(0,file,Length,Output,0);}
	void	CreateHashFromString(unsigned char* in_string, int Length, unsigned char* Output)	{CreateHashFromInput(0,0,Length,Output,in_string);}
	void	LoadComment();//comment
	CArray<unsigned char*,unsigned char*> hashlist;
	CArray<CTag*,CTag*> taglist;
	char*	directory;
	CString m_strFilePath;	

private:
	void	CreateHashFromInput(FILE* file,CFile* file2, int Length, unsigned char* Output, unsigned char* = 0);
	bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	uint8	m_iPermissions;
	bool	m_bAutoUpPriority;
	uint32	m_iQueuedCount;
	static	CBarShader s_ShareStatusBar;
	bool	m_PublishedED2K;

	CArray<uint16,uint16> m_AvailPartFrequency;

/* Creteil Begin */
public:
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
/* Creteil End */
	CTypedPtrList<CPtrList, CUpDownClient*> m_ClientUploadList;
};

// permission values for shared files
#define PERM_ALL		0
#define PERM_FRIENDS	1
#define PERM_NOONE		2

// constants for MD4Transform
#define S11 3
#define S12 7
#define S13 11
#define S14 19
#define S21 3
#define S22 5
#define S23 9
#define S24 13
#define S31 3
#define S32 9
#define S33 11
#define S34 15

// basic MD4 functions
#define MD4_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD4_G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define MD4_H(x, y, z) ((x) ^ (y) ^ (z))

// rotates x left n bits
#define MD4_ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// partial transformations
#define MD4_FF(a, b, c, d, x, s) \
{ \
  (a) += MD4_F((b), (c), (d)) + (x); \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

#define MD4_GG(a, b, c, d, x, s) \
{ \
  (a) += MD4_G((b), (c), (d)) + (x) + (uint32)0x5A827999; \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

#define MD4_HH(a, b, c, d, x, s) \
{ \
  (a) += MD4_H((b), (c), (d)) + (x) + (uint32)0x6ED9EBA1; \
  (a) = MD4_ROTATE_LEFT((a), (s)); \
}

#endif // KNOWNFILE_H