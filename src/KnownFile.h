//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef KNOWNFILE_H
#define KNOWNFILE_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "KnownFile.h"
#endif

#include "CMD4Hash.h"
#include "SHAHashSet.h"
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dynarray.h>

#include <set>

#include "Types.h"		// Needed for int8, uint8, uint16, uint32 and uint64
#include "OPCodes.h"		// Needed for PARTSIZE

#ifdef CLIENT_GUI
#include "ECSpecialTags.h"
#endif

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

class CUpDownClient;
class CFile;
class CPacket;
class CTag;

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
friend class CAddFileThread;
public:
	CKnownFile();
#ifdef CLIENT_GUI
	CKnownFile(CEC_SharedFile_Tag *);
#endif

	virtual ~CKnownFile();

	void SetFilePath(const wxString& strFilePath);
	const wxString& GetFilePath() const { return m_strFilePath; }
	
	virtual	bool	IsPartFile() const	{return false;}
	virtual bool	LoadFromFile(const CFileDataIO* file);	//load date, hashset and tags from a .met file
	virtual uint8	GetStatus(bool WXUNUSED(ignorepause) = false) const { return PS_COMPLETE; }
	bool	WriteToFile(CFileDataIO* file);	
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

	bool	LoadHashsetFromFile(const CFileDataIO* file, bool checkhash);
	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	void	NewAvailPartsInfo();
	
	// comment 
#ifdef CLIENT_GUI
	const wxString&	GetFileComment(){ return m_strComment; }
	int8	GetFileRate() 		{ return m_iRate; }
#else
	const wxString&	GetFileComment(){ if (!m_bCommentLoaded) LoadComment(); return m_strComment; } 
	int8	GetFileRate() 		{ if (!m_bCommentLoaded) LoadComment(); return m_iRate; }
#endif
	void	SetFileComment(const wxString& strNewComment);
	void	SetFileRate(int8 iNewRate); 
	void	SetPublishedED2K( bool val );
	bool	GetPublishedED2K() const	{return m_PublishedED2K;}
	
	// file sharing
	virtual	CPacket*	CreateSrcInfoPacket(const CUpDownClient* forClient);
	
	virtual void	UpdatePartsInfo();	

	uint32	date;
	
	CFileStatistic statistic;
	
	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;
	
	// Maybe find a common place for this typedef?
	typedef std::set<CUpDownClient*> SourceSet;
	SourceSet m_ClientUploadList;
	ArrayOfUInts16 m_AvailPartFrequency;
	
	bool	CreateAICHHashSetOnly();
	// aich
	CAICHHashSet*	GetAICHHashset() const							{return m_pAICHHashSet;}
	void			SetAICHHashset(CAICHHashSet* val)				{m_pAICHHashSet = val;}		

	/**
	 * Updates the requency of uploading parts from with the data the client provides.
	 *
	 * @param client The clients whoose uploading parts should be considered.
	 * @param increment If true, the counts are incremented, otherwise they are decremented.
	 *
	 * This functions updates the frequency list of file-upparts, using the clients 
	 * upparts-status. This function should be called by clients every time they update their
	 * upparts-status, or when they are added or removed from the file.
	 */
	void UpdateUpPartsFrequency( CUpDownClient* client, bool increment );

	void	CreateHashFromString(uchar* in_string, uint32 Length, uchar* Output, CAICHHashTree* pShaHashOut = NULL)	{CreateHashFromInput(NULL, Length,Output,in_string,pShaHashOut);}
protected:
	bool	LoadTagsFromFile(const CFileDataIO* file);
	bool	LoadDateFromFile(const CFileDataIO* file);
	void	CreateHashFromFile(CFile* file, uint32 Length, uchar* Output, CAICHHashTree* pShaHashOut = NULL) const { CreateHashFromInput(file, Length, Output, NULL, pShaHashOut); }	
	void	LoadComment();//comment
	ArrayOfCMD4Hash hashlist;
	ArrayOfCTag taglist;
	wxString m_strFilePath;	
	CAICHHashSet*			m_pAICHHashSet;

	void	CreateHashFromInput(CFile* file, uint32 Length, uchar* Output, uchar* in_string, CAICHHashTree* pShaHashOut) const;
	bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	uint32	m_iQueuedCount;
	bool	m_PublishedED2K;

};

#endif // KNOWNFILE_H
