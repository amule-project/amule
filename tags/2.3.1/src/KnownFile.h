//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include <protocol/ed2k/Constants.h>
#include <common/Path.h>

#include "kademlia/kademlia/Indexed.h"
#include <ec/cpp/ECID.h>	// Needed for CECID


#ifdef CLIENT_GUI
#include <ec/cpp/ECSpecialTags.h>
#include "RLE.h"			// Needed for RLE_Data, PartFileEncoderData
#endif

#include "Constants.h"		// Needed for PS_*, PR_*
#include "ClientRef.h"		// Needed for CClientRef

class CFileDataIO;
class CPacket;
class CTag;


namespace Kademlia
{
	class CEntry;
}


typedef vector<CMD4Hash> ArrayOfCMD4Hash;


typedef vector<CTag> ArrayOfCTag;


class CFileStatistic
{
	friend class CKnownFile;
	friend class CKnownFilesRem;

public:
	CFileStatistic();
	void	AddRequest();
	void	AddAccepted();
	void    AddTransferred(uint64 bytes);
	uint16	GetRequests() const			{return requested;}
	uint16	GetAccepts() const			{return accepted;}
	uint64  GetTransferred() const			{return transferred;}
	uint32	GetAllTimeRequests() const		{return alltimerequested;}
	void	SetAllTimeRequests(uint32 new_value)	{ alltimerequested = new_value; };
	uint32	GetAllTimeAccepts() const		{return alltimeaccepted;}
	void	SetAllTimeAccepts(uint32 new_value)	{ alltimeaccepted = new_value; };
	uint64	GetAllTimeTransferred() const		{return alltimetransferred;}
	void	SetAllTimeTransferred(uint64 new_value)	{ alltimetransferred = new_value; };
	CKnownFile* fileParent;

private:
	uint16 requested;
	uint64 transferred;
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
	virtual ~CAbstractFile() {}

	const CPath&	GetFileName() const	{ return m_fileName; }
	const CMD4Hash&	GetFileHash() const	{ return m_abyFileHash; }

	uint64	GetFileSize() const	{ return m_nFileSize;}
	bool	IsLargeFile() const	{ return m_nFileSize > (uint64)OLD_MAX_FILE_SIZE; }

	virtual void SetFileSize(uint64 nFileSize)	{ m_nFileSize = nFileSize; }
	virtual void SetFileName(const CPath& filename);

	/* Tags and Notes handling */
	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(const wxString& tagname) const;
	bool GetIntTagValue(uint8 tagname, uint32& ruValue) const;
	const wxString& GetStrTagValue(uint8 tagname) const;
	const wxString& GetStrTagValue(const wxString& tagname) const;
	const CTag *GetTag(const wxString& tagname) const;
	const CTag *GetTag(const wxString& tagname, uint8 tagtype) const;
	const CTag *GetTag(uint8 tagname) const;
	const CTag *GetTag(uint8 tagname, uint8 tagtype) const;
	void AddTagUnique(const CTag &pTag);
	const ArrayOfCTag& GetTags() const { return m_taglist; }
	void AddNote(Kademlia::CEntry* pEntry);
	const CKadEntryPtrList& getNotes() const { return m_kadNotes; }

	/* Comment and rating */
	virtual const wxString&	GetFileComment() const { return m_strComment; }
	virtual int8	GetFileRating() 		const { return m_iRating; }

	bool	HasComment() const		{ return m_hasComment; }
	bool	HasRating() const		{ return (m_iUserRating != 0); }
	int8	UserRating() const 		{ return m_iUserRating; }

protected:
	//! CAbstractFile is not assignable.
	CAbstractFile& operator=(const CAbstractFile);

	CMD4Hash	m_abyFileHash;
	// comment/rating are read from the config and cached in these variables,
	// so make the mutable to allow GetFileComment() to be a const method
	mutable	wxString	m_strComment;
	mutable	int8		m_iRating;
	bool		m_hasComment;
	int8		m_iUserRating;
	ArrayOfCTag	m_taglist;
	CKadEntryPtrList m_kadNotes;

private:
	uint64		m_nFileSize;
	CPath		m_fileName;
};


class CSearchFile;
class CFile;


class CKnownFile : public CAbstractFile, public CECID
{
friend class CHashingTask;
public:
	CKnownFile();
	CKnownFile(uint32 ecid);
	explicit CKnownFile(const CSearchFile &searchFile);

	virtual ~CKnownFile();

	void SetFilePath(const CPath& filePath);
	const CPath& GetFilePath() const { return m_filePath; }

	// virtual functions for CKnownFile and CPartFile:
	virtual	bool	IsPartFile() const	{return false;}		// true if not completed
	virtual bool	IsCompleted() const	{ return true; }	// true if completed
	virtual bool	IsCPartFile() const	{ return false; }	// true if it's a CPartFile

	virtual bool	LoadFromFile(const CFileDataIO* file);	//load date, hashset and tags from a .met file
	virtual uint8	GetStatus(bool WXUNUSED(ignorepause) = false) const { return PS_COMPLETE; }
	bool	WriteToFile(CFileDataIO* file);
	time_t GetLastChangeDatetime() const { return m_lastDateChanged; }
	void SetLastChangeDatetime(time_t t) { m_lastDateChanged = t; }

	virtual void SetFileSize(uint64 nFileSize);

	// local available part hashs
	size_t	GetHashCount() const	{return m_hashlist.size();}
	const CMD4Hash&	GetPartHash(uint16 part) const;

	// nr. of part hashs according the file size wrt ED2K protocol
	uint32	GetED2KPartHashCount() const { return m_iED2KPartHashCount; }

	// nr. of 9MB parts (file data)
	inline uint16 GetPartCount() const { return m_iPartCount; }

	// nr. of 9MB parts according the file size wrt ED2K protocol (OP_FILESTATUS)
	inline uint16 GetED2KPartCount() const { return m_iED2KPartCount; }

	// size of a certain part, last is different, all others are PARTSIZE
	uint32 GetPartSize(uint16 part) const { return part == m_iPartCount - 1 ? m_sizeLastPart : PARTSIZE; }

	// file upload priority
	uint8	GetUpPriority()	 const		{return m_iUpPriority;}
	void	SetUpPriority(uint8 newUpPriority, bool bSave=true);
	bool	IsAutoUpPriority() const		{return m_bAutoUpPriority;}
	void	SetAutoUpPriority(bool flag)	{m_bAutoUpPriority = flag;}
	void	UpdateAutoUpPriority();
#ifdef CLIENT_GUI
	uint16	GetQueuedCount() const { return m_queuedCount; }
#else
	uint16	GetQueuedCount() const { return (uint16) m_ClientUploadList.size(); }
#endif

	bool	LoadHashsetFromFile(const CFileDataIO* file, bool checkhash);
	void	AddUploadingClient(CUpDownClient* client);
	void	RemoveUploadingClient(CUpDownClient* client);

	// comment
	const wxString&	GetFileComment()	const	{ if (!m_bCommentLoaded) LoadComment(); return m_strComment; }
	int8	GetFileRating() 			const	{ if (!m_bCommentLoaded) LoadComment(); return m_iRating; }

	void	SetFileCommentRating(const wxString& strNewComment, int8 iNewRating);
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
	virtual	CPacket*	CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions);
	void	CreateOfferedFilePacket(CMemFile* files, class CServer* pServer, CUpDownClient* pClient);

	virtual void	UpdatePartsInfo();


	CFileStatistic statistic;

	time_t m_nCompleteSourcesTime;
	uint16 m_nCompleteSourcesCount;
	uint16 m_nCompleteSourcesCountLo;
	uint16 m_nCompleteSourcesCountHi;

	// Common for part and known files.
	typedef std::set<CClientRef> SourceSet;
	SourceSet m_ClientUploadList;
	ArrayOfUInts16 m_AvailPartFrequency;

	/**
 	 * Returns a base-16 encoding of the master hash, or
 	 * an empty string if no such hash exists.
 	 */
	wxString GetAICHMasterHash() const;
	/** Returns true if the AICH-Hashset is valid, and verified or complete. */
	bool HasProperAICHHashSet() const;

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

	static void CreateHashFromHashlist(const ArrayOfCMD4Hash& hashes, CMD4Hash* Output);

	void	ClearPriority();

	time_t	m_lastDateChanged;

	virtual wxString GetFeedback() const;

	void	SetShowSources( bool val )	{ m_showSources = val; }
	bool	ShowSources() const			{ return m_showSources; }
	void	SetShowPeers( bool val )	{ m_showPeers = val; }
	bool	ShowPeers()	const			{ return m_showPeers; }

#ifdef CLIENT_GUI
	CKnownFile(CEC_SharedFile_Tag *);
	friend class CKnownFilesRem;
	RLE_Data m_partStatus;

private:
	uint8	m_iUpPriorityEC;
	uint16	m_queuedCount;

protected:
	//! The AICH master-hash, if it is known.
	wxString	m_AICHMasterHash;
#else
	virtual void SetFileName(const CPath& filename);

	// AICH
	CAICHHashSet* GetAICHHashset() const		{ return m_pAICHHashSet; }
	void SetAICHHashset(CAICHHashSet* val)		{ m_pAICHHashSet = val; }

protected:
	CAICHHashSet*	m_pAICHHashSet;
#endif

	bool	LoadTagsFromFile(const CFileDataIO* file);
	bool	LoadDateFromFile(const CFileDataIO* file);
	void	LoadComment() const;
	ArrayOfCMD4Hash m_hashlist;
	CPath	m_filePath;

	static void CreateHashFromFile(class CFileAutoClose& file, uint64 offset, uint32 Length, CMD4Hash* Output, CAICHHashTree* pShaHashOut);
	static void CreateHashFromInput(const byte* input, uint32 Length, CMD4Hash* Output, CAICHHashTree* pShaHashOut);

	mutable bool	m_bCommentLoaded;
	uint16	m_iPartCount;
	uint16  m_iED2KPartCount;
	uint16	m_iED2KPartHashCount;
	uint32	m_sizeLastPart;			// size of the last part
	uint8	m_iUpPriority;
	bool	m_bAutoUpPriority;
	bool	m_PublishedED2K;

	/* Kad stuff */
	Kademlia::WordList wordlist;
	uint32	kadFileSearchID;
	uint32	m_lastPublishTimeKadSrc;
	uint32	m_lastPublishTimeKadNotes;
	uint32	m_lastBuddyIP;

	bool	m_showSources;
	bool	m_showPeers;
private:
	/** Common initializations for constructors. */
	void Init();
};

#endif // KNOWNFILE_H
// File_checked_for_headers
