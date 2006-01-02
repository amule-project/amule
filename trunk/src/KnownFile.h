//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef KNOWNFILE_H
#define KNOWNFILE_H

#include "MD4Hash.h"
#include "SHAHashSet.h"
#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/dynarray.h>

#include <set>

#include "CTypedPtrList.h"
#include "Types.h"		// Needed for int8, uint8, uint16, uint32 and uint64
#include "OPCodes.h"		// Needed for PARTSIZE

#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/Indexed.h"

#ifdef CLIENT_GUI
#include <ec/ECSpecialTags.h>
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
class CFileDataIO;
class CPacket;
class CTag;

namespace Kademlia{
	class CEntry;
};

WX_DECLARE_OBJARRAY(CMD4Hash, ArrayOfCMD4Hash);

WX_DECLARE_OBJARRAY(CTag*, ArrayOfCTag);

class CFileStatistic {
	friend class CKnownFile;
	friend class CSharedFilesRem;
public:
	CFileStatistic();
	void	AddRequest();
	void	AddAccepted();
	void    AddTransferred(uint64 bytes);
	uint16	GetRequests() const			{return requested;}
	uint16	GetAccepts() const			{return accepted;}
	uint64  GetTransfered() const			{return transfered;}
	uint32	GetAllTimeRequests() const	{return alltimerequested;}
	void	SetAllTimeRequests(uint32 new_value) { alltimerequested = new_value; };
	uint32	GetAllTimeAccepts() const		{return alltimeaccepted;}
	void	SetAllTimeAccepts(uint32 new_value) { alltimeaccepted = new_value; };	
	uint64	GetAllTimeTransfered() const	{return alltimetransferred;}
	void	SetAllTimeTransfered(uint64 new_value) { alltimetransferred = new_value; };
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
	explicit CAbstractFile(const CAbstractFile& other);
	virtual ~CAbstractFile();

	virtual const wxString&	GetFileName() const		{return m_strFileName;}
	const CMD4Hash&	GetFileHash() const	{return m_abyFileHash;}
	uint32	GetFileSize() const			{return m_nFileSize;}
	virtual void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }
	
	virtual void	SetFileName(const wxString& strmakeFilename);

	/* Tags and Notes handling */
	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(const char* tagname) const;
	bool GetIntTagValue(uint8 tagname, uint32& ruValue) const;
	const wxString& GetStrTagValue(uint8 tagname) const;
	const wxString& GetStrTagValue(const char* tagname) const;
	CTag* GetTag(const char* tagname) const;	
	CTag* GetTag(const char* tagname, uint8 tagtype) const;
	CTag* GetTag(uint8 tagname) const;
	CTag* GetTag(uint8 tagname, uint8 tagtype) const;	
	void AddTagUnique(CTag* pTag);
	const ArrayOfCTag& GetTags() const { return taglist; }
	void AddNote(Kademlia::CEntry* pEntry);
	const CKadEntryPtrList& getNotes() const { return m_kadNotes; }

	/* Comment and rating */	
	virtual const wxString&	GetFileComment() const { return m_strComment; }
	virtual int8	GetFileRating() 		const { return m_iRating; }	
	
	bool	HasComment() const		{ return m_hasComment; }
	bool	HasRating() const		{ return m_iUserRating; }
	bool	HasBadRating() const		{ return (m_iUserRating == 1); }
	int8	UserRating() const 		{ return m_iUserRating; }
	void	UpdateFileRatingCommentAvail();

protected:
	//! CAbstractFile is not assignable.
	CAbstractFile& operator=(const CAbstractFile);
	
	wxString	m_strFileName;
	CMD4Hash	m_abyFileHash;
	uint32		m_nFileSize;
	wxString	m_strComment;
	int8		m_iRating;
	bool		m_hasComment;
	int8		m_iUserRating;
	ArrayOfCTag taglist;
	CKadEntryPtrList m_kadNotes;
};


class CKnownFile : public CAbstractFile
{
friend class CAddFileThread;
public:
	CKnownFile();
#ifdef CLIENT_GUI
	CKnownFile(CEC_SharedFile_Tag *);
	friend class CSharedFilesRem;
#endif

	virtual ~CKnownFile();

	#ifndef CLIENT_GUI	
	virtual void SetFileName(const wxString& strmakeFilename);
	#endif
		
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
	uint32	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

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
	uint32	GetQueuedCount() const {return m_ClientUploadList.size();}

	bool	LoadHashsetFromFile(const CFileDataIO* file, bool checkhash);
	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);
	
	// comment 
	const wxString&	GetFileComment() { if (!m_bCommentLoaded) LoadComment(); return m_strComment; } 
	int8	GetFileRating() 		{ if (!m_bCommentLoaded) LoadComment(); return m_iRating; }

	void	SetFileComment(const wxString& strNewComment);
	void	SetFileRating(int8 iNewRating); 
	void	SetPublishedED2K( bool val );
	bool	GetPublishedED2K() const	{return m_PublishedED2K;}

	/* Kad stuff */ 
	uint32	GetKadFileSearchID() const { return kadFileSearchID; }
	// KAD TODO: This must be used on KadSearchListCtrl too once imported
	void	SetKadFileSearchID(uint32 id) { kadFileSearchID = id; } // John - Don't use this unless you know what your are DOING!! (Hopefully I do.. :)
	const Kademlia::WordList& GetKadKeywords() const { return wordlist; }
	// KAD TODO: If we add the proper column to SharedFilesCtrl, this is the funtion.
	uint32	GetLastPublishTimeKadSrc() const { return m_lastPublishTimeKadSrc; }	
	void	SetLastPublishTimeKadSrc(uint32 time, uint32 buddyip) { m_lastPublishTimeKadSrc = time; m_lastBuddyIP = buddyip;}
	// Another unused function, useful for the shared files control column
	uint32	GetLastPublishBuddy() const { return m_lastBuddyIP; }
	void	SetLastPublishTimeKadNotes(uint32 time) {m_lastPublishTimeKadNotes = time;}
	uint32	GetLastPublishTimeKadNotes() const { return m_lastPublishTimeKadNotes; }	
	
	bool	PublishSrc();
	bool	PublishNotes();	
	
	// TODO: This must be implemented if we ever want to have metadata.
	uint32	GetMetaDataVer() const { return /*m_uMetaDataVer*/ 0; }
	
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

	void	CreateHashFromString(byte* in_string, uint32 Length, byte* Output, CAICHHashTree* pShaHashOut = NULL)	{CreateHashFromInput(NULL, Length,Output,in_string,pShaHashOut);}
	
	void	ClearPriority();
	
protected:
	bool	LoadTagsFromFile(const CFileDataIO* file);
	bool	LoadDateFromFile(const CFileDataIO* file);
	void	CreateHashFromFile(CFileDataIO* file, uint32 Length, byte* Output, CAICHHashTree* pShaHashOut = NULL) const { CreateHashFromInput(file, Length, Output, NULL, pShaHashOut); }	
	void	LoadComment();//comment
	ArrayOfCMD4Hash hashlist;
	wxString m_strFilePath;	
	CAICHHashSet*			m_pAICHHashSet;

	void	CreateHashFromInput(CFileDataIO* file, uint32 Length, byte* Output, byte* in_string, CAICHHashTree* pShaHashOut) const;
	bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	bool	m_PublishedED2K;
	
	/* Kad stuff */
	Kademlia::WordList wordlist;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_lastPublishTimeKadNotes;
	uint32	m_lastBuddyIP;

};

#endif // KNOWNFILE_H
