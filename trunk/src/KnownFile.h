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

#ifndef KNOWNFILE_H
#define KNOWNFILE_H

#ifdef __CRYPTO_DEBIAN_GENTOO__
	#include <crypto++/config.h>
	#include <crypto++/rsa.h>
#else
	#ifdef __CRYPTO_MDK_SUSE_FC__
		#include <cryptopp/config.h>
		#include <cryptopp/rsa.h>
	#else
		#ifdef __CRYPTO_SOURCE__
			#include <crypto-5.1/config.h>
			#include <crypto-5.1/rsa.h>
		#else //needed for standard path
			#include <cryptopp/config.h>
			#include <cryptopp/rsa.h>
		#endif
	#endif
#endif


#include "CMD4Hash.h"

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dynarray.h>

#include "types.h"		// Needed for int8, uint8, uint16, uint32 and uint64
#include "opcodes.h"		// Needed for PARTSIZE
#include "CTypedPtrList.h"	// Needed for CTypedPtrList


#define	PS_READY			0
#define	PS_EMPTY			1
#define PS_WAITINGFORHASH		2
#define PS_HASHING			3
#define PS_ERROR			4
#define	PS_INSUFFICIENT			5
#define	PS_UNKNOWN			6
#define PS_PAUSED			7
#define PS_COMPLETING			8
#define PS_COMPLETE			9

#define PR_VERYLOW			4 // I Had to change this because it didn't save negative number correctly.. Had to modify the sort function for this change..
#define PR_LOW				0 //*
#define PR_NORMAL			1 // Don't change this - needed for edonkey clients and server!
#define	PR_HIGH				2 //*
#define PR_VERYHIGH			3
#define PR_AUTO				5
#define PR_POWERSHARE                   6 //added for powershare (deltaHF)
#define SRV_PR_LOW			2
#define SRV_PR_NORMAL			0
#define SRV_PR_HIGH			1

class CUpDownClient;
class CFile;
class Packet;
class CTag;

WX_DEFINE_ARRAY_SHORT(uint16, ArrayOfUInts16);

WX_DECLARE_OBJARRAY(CMD4Hash, ArrayOfCMD4Hash);

WX_DECLARE_OBJARRAY(CTag*, ArrayOfCTag);

class CFileStatistic {
	friend class CKnownFile;
public:
	CFileStatistic();
	void	AddRequest();
	void	AddAccepted();
	void    AddTransferred(uint64 bytes);
	uint16	GetRequests() const			{return requested;}
	uint16	GetAccepts() const			{return accepted;}
	uint64  GetTransfered() const			{return transfered;}
	uint16	GetAllTimeRequests() const	{return alltimerequested;}
	uint16	GetAllTimeAccepts() const		{return alltimeaccepted;}
	uint64	GetAllTimeTransfered() const	{return alltimetransferred;}
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
	virtual ~CAbstractFile() {};

	const wxString&	GetFileName() const		{return m_strFileName;}
	const CMD4Hash&	GetFileHash() const	{return m_abyFileHash;}
	uint32	GetFileSize() const			{return m_nFileSize;}
	virtual void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }
	void	SetFileName(const wxString& strmakeFilename);
	
protected:
	wxString	m_strFileName;
	CMD4Hash	m_abyFileHash;
	uint32		m_nFileSize;
	wxString	m_strComment;
	int8		m_iRate;
};

class CKnownFile : public CAbstractFile
{
public:
	CKnownFile();
	virtual ~CKnownFile();

	virtual bool	CreateFromFile(const wxString& directory, const wxString& filename, volatile int const * notify); // create date, hashset and tags from a file
	
	void SetFilePath(const wxString& strFilePath);
	const wxString& GetFilePath() const { return m_strFilePath; }
	
	virtual	bool	IsPartFile() const	{return false;}
	virtual bool	LoadFromFile(const CFile* file);	//load date, hashset and tags from a .met file
	virtual uint8	GetStatus(bool WXUNUSED(ignorepause) = false) const { return PS_COMPLETE; }
	bool	WriteToFile(CFile* file);	
	uint32	GetFileDate() const	{return date;}

		
	virtual void SetFileSize(uint32 nFileSize);

	// local available part hashs
	uint16	GetHashCount() const	{return hashlist.GetCount();}
	const CMD4Hash&	GetPartHash(uint16 part) const;

	// nr. of part hashs according the file size wrt ED2K protocol
	UINT	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

	// nr. of 9MB parts (file data)
	inline uint16 GetPartCount() const { return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }
	
	// file upload priority
	uint8	GetUpPriority()	 const		{return m_iUpPriority;}
	void	SetUpPriority(uint8 newUpPriority, bool bSave=true);
	bool	IsAutoUpPriority() const		{return m_bAutoUpPriority;}
	void	SetAutoUpPriority(bool flag)	{m_bAutoUpPriority = flag;}
	void	UpdateAutoUpPriority();
	void	AddQueuedCount() {m_iQueuedCount++; UpdateAutoUpPriority();};
	void	SubQueuedCount() {if(m_iQueuedCount) m_iQueuedCount--; UpdateAutoUpPriority();}
	uint32	GetQueuedCount() const {return m_iQueuedCount;}

	// shared file view permissions (all, only friends, no one)
	uint8	GetPermissions() const	{ return m_iPermissions; };
	void	SetPermissions(uint8 iNewPermissions) {m_iPermissions = iNewPermissions;};

	bool	LoadHashsetFromFile(const CFile* file, bool checkhash);
	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	void	NewAvailPartsInfo();
	
	// comment 
	const wxString&	GetFileComment() {if (!m_bCommentLoaded) LoadComment(); return m_strComment;} 
	void	SetFileComment(const wxString& strNewComment);
	void	SetFileRate(int8 iNewRate); 
	int8	GetFileRate() 			{if (!m_bCommentLoaded) LoadComment(); return m_iRate;}
	void	SetPublishedED2K( bool val );
	bool	GetPublishedED2K() const	{return m_PublishedED2K;}
	
	// file sharing
	virtual	Packet*	CreateSrcInfoPacket(const CUpDownClient* forClient);
	
	virtual void	UpdatePartsInfo();	

	uint32	date;
	
	CFileStatistic statistic;
	
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
	CTypedPtrList<CPtrList, CUpDownClient*> m_ClientUploadList;	
	ArrayOfUInts16 m_AvailPartFrequency;
	
protected:
	bool	LoadTagsFromFile(const CFile* file);
	bool	LoadDateFromFile(const CFile* file);
	void	CreateHashFromFile(FILE* file, int Length, unsigned char* Output)	{CreateHashFromInput(file,0,Length,Output,0);}
	void	CreateHashFromFile(CFile* file, int Length, unsigned char* Output)	{CreateHashFromInput(0,file,Length,Output,0);}
	void	CreateHashFromString(unsigned char* in_string, int Length, unsigned char* Output)	{CreateHashFromInput(0,0,Length,Output,in_string);}
	void	LoadComment();//comment
	void GetMetaDataTags();
	ArrayOfCMD4Hash hashlist;
	ArrayOfCTag taglist;
	wxString m_strFilePath;	

private:
	void	CreateHashFromInput(FILE* file, CFile* file2, int Length, unsigned char* Output, unsigned char* = 0);
	bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	uint8	m_iPermissions;
	bool	m_bAutoUpPriority;
	uint32	m_iQueuedCount;
	bool	m_PublishedED2K;

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
