//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef PARTFILE_H
#define PARTFILE_H


#include "KnownFile.h"		// Needed for CKnownFile
#include "FileAutoClose.h"	// Needed for CFileAutoClose

#include "OtherStructs.h"	// Needed for Gap_Struct
#include "DeadSourceList.h"	// Needed for CDeadSourceList
#include "GapList.h"


class CSearchFile;
class CUpDownClient;
class CMemFile;
class CFileDataIO;
class CED2KFileLink;

//#define BUFFER_SIZE_LIMIT	500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	60000   // Max milliseconds before forcing a flush

// Ok, eMule and aMule are building incompatible backup files because 
// of the different name. aMule was using ".BAK" and eMule ".bak". 
// This should fix it.
#define   PARTMET_BAK_EXT wxT(".bak")

enum EPartFileFormat {
	PMT_UNKNOWN	= 0,
	PMT_DEFAULTOLD,
	PMT_SPLITTED,
	PMT_NEWOLD,
	PMT_SHAREAZA,
	PMT_BADFORMAT
};


class SFileRating
{
public:
	wxString UserName;
	wxString FileName;
	sint16   Rating;
	wxString Comment;
public:
	SFileRating(const wxString &u, const wxString &f, sint16 r, const wxString &c);
	SFileRating(const SFileRating &fr);
	SFileRating(const CUpDownClient &client);
	~SFileRating();
};

typedef std::list<SFileRating> FileRatingList;

class SourcenameItem
{
public:
	wxString	name;
	int			count;
public:
	SourcenameItem(const wxString &n = EmptyString, int c = 0)
	:
	name(n), count(c) {}
};

typedef std::map<uint32,SourcenameItem> SourcenameItemMap;

class CPartFile : public CKnownFile {
public:
	typedef std::list<Requested_Block_Struct*> CReqBlockPtrList;
	
	CPartFile();
#ifdef CLIENT_GUI
	CPartFile(CEC_PartFile_Tag *tag);
#else 
	virtual void	SetFileName(const CPath& filename);
#endif
	CPartFile(CSearchFile* searchresult);  //used when downloading a new file
	CPartFile(const CED2KFileLink* fileLink);
	virtual ~CPartFile();
	
	bool	CreateFromFile(wxString WXUNUSED(directory), wxString WXUNUSED(filename), void* WXUNUSED(pvProgressParam)) {return false;}// not supported in this class
	void 	SetPartFileStatus(uint8 newstatus);
	virtual bool LoadFromFile(const CFileDataIO* WXUNUSED(file)) { return false; }
	bool	WriteToFile(CFileDataIO* WXUNUSED(file))	{ return false; }
	bool	IsPartFile() const		{ return !(status == PS_COMPLETE); }
	uint32	Process(uint32 reducedownload, uint8 m_icounter);
	uint8	LoadPartFile(const CPath& in_directory, const CPath& filename, bool from_backup = false, bool getsizeonly = false);
	bool	SavePartFile(bool Initial = false);
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted
	
	bool    CheckShowItemInGivenCat(int inCategory);

	bool	IsComplete(uint64 start, uint64 end)	{ return m_gaplist.IsComplete(start, end); }
	bool	IsComplete(uint16 part)					{ return m_gaplist.IsComplete(part); }
	
	void	UpdateCompletedInfos();

	bool	GetNextRequestedBlock(CUpDownClient* sender, std::vector<Requested_Block_Struct*>& toadd, uint16& count);
	void	WritePartStatus(CMemFile* file);
	void	WriteCompleteSourcesCount(CMemFile* file);
	static bool 	CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped = NULL, bool ed2kID = true);
	void	AddSources(CMemFile& sources, uint32 serverip, uint16 serverport, unsigned origin, bool bWithObfuscationAndHash);
#ifdef CLIENT_GUI
	uint8	GetStatus() const { return status; }
	uint8	GetStatus(bool /*ignorepause = false*/) const { return status; }
#else
	uint8	GetStatus(bool ignorepause = false) const;
#endif
	virtual void	UpdatePartsInfo();
	const CPath& GetPartMetFileName() const { return m_partmetfilename; }
	uint16	GetPartMetNumber() const;
	uint64	GetTransferred() const		{ return transferred; }
	const CPath& GetFullName() const	{ return m_fullname; }
	float	GetKBpsDown() const			{ return kBpsDown; }
	double	GetPercentCompleted() const	{ return percentcompleted; }

#ifndef CLIENT_GUI
	uint16	GetSourceCount() const		{ return (uint16)m_SrcList.size(); }
	uint16	GetSrcA4AFCount() const		{ return (uint16)m_A4AFsrclist.size(); }
#else
	uint16	m_source_count, m_a4af_source_count;
	uint16	GetSourceCount() const		{ return m_source_count; }
	uint16	GetSrcA4AFCount() const		{ return m_a4af_source_count; }
#endif
	uint16	GetTransferingSrcCount() const	{ return transferingsrc; }
	uint16  GetNotCurrentSourcesCount()	const	{ return m_notCurrentSources; };
	void	SetNotCurrentSourcesCount(uint16 new_count)	{ m_notCurrentSources = new_count; };	
	uint16	GetValidSourcesCount() const	{ return m_validSources; };
	
	uint64	GetNeededSpace();
	
	virtual wxString GetFeedback() const;
	
	wxString getPartfileStatus() const; //<<--9/21/02
	sint32	getTimeRemaining() const; //<<--9/21/02
	time_t	lastseencomplete;
	int	getPartfileStatusRang() const;

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, byte *data, uint64 start, uint64 end, Requested_Block_Struct *block, const CUpDownClient* client);
	void	FlushBuffer(bool fromAICHRecoveryDataAvailable = false);	

	// Barry - Is archive recovery in progress
	volatile bool m_bRecoveringArchive;

	// Barry - Added to prevent list containing deleted blocks on shutdown
	void	RemoveAllRequestedBlocks(void);

	void	RemoveBlockFromList(uint64 start,uint64 end);
	void	RemoveAllSources(bool bTryToSwap);
	void	Delete();
	void	StopFile(bool bCancel = false);
	void	PauseFile(bool bInsufficient = false);
	void	ResumeFile();

	virtual	CPacket* CreateSrcInfoPacket(const CUpDownClient* forClient, uint8 byRequestedVersion, uint16 nRequestedOptions);
	void    AddClientSources(CMemFile* sources, unsigned nSourceFrom, uint8 uClientSXVersion, bool bSourceExchange2, const CUpDownClient* pClient = NULL);

	bool	PreviewAvailable();
	uint16	GetAvailablePartCount() const	{ return m_availablePartsCount; }
	uint32	GetLastAnsweredTime() const	{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime();
	void	SetLastAnsweredTimeTimeout();
	uint64	GetLostDueToCorruption() const	{ return m_iLostDueToCorruption; }
	uint64	GetGainDueToCompression() const	{ return m_iGainDueToCompression; }
	uint32	TotalPacketsSavedDueToICH()const{ return m_iTotalPacketsSavedDueToICH; }
	bool	IsStopped() const		{ return this ? m_stopped : true; }
	bool	IsPaused() const		{ return m_paused; }
	void	UpdateFileRatingCommentAvail();

	int	GetCommonFilePenalty();
	void	UpdateDisplayedInfo(bool force = false);
	
	uint8	GetCategory() const { return m_category; }
	void	SetCategory(uint8 cat);

	volatile bool m_bPreviewing;
	void	SetDownPriority(uint8 newDownPriority, bool bSave = true, bool bRefresh = true);
	bool	IsAutoDownPriority() const	{ return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool flag)	{ m_bAutoDownPriority = flag; }
	void	UpdateAutoDownPriority();
	uint8	GetDownPriority() const		{ return m_iDownPriority; }
	void	SetActive(bool bActive);
	uint32	GetDlActiveTime() const;
	bool	GetInsufficient() const		{ return m_insufficient; }
	
	void	CompleteFileEnded(bool errorOccured, const CPath& newname);	

	bool	RemoveSource(CUpDownClient* toremove, bool updatewindow = true, bool bDoStatsUpdate = true);

	void	RequestAICHRecovery(uint16 nPart);
	void	AICHRecoveryDataAvailable(uint16 nPart);	

	/**
	 * This function is used to update source-counts.
	 *
	 * @param oldState The old state of the client, or -1 to ignore.
	 * @param newState The new state of the client, or -1 to ignore.
	 *
	 * Call this function for a client belonging to this file, which has changed 
	 * its state. The value -1 can be used to make the function ignore one of 
	 * the two states.
	 *
	 * AddSource and DelSource takes care of calling this function when a source is 
	 * removed, so there's no need to call this function when calling either of those.
	 */
	void	ClientStateChanged( int oldState, int newState );

	bool	AddSource( CUpDownClient* client );
	bool	DelSource( CUpDownClient* client );

	/**
	 * Updates the requency of avilable parts from with the data the client provides.
	 *
	 * @param client The clients whoose available parts should be considered.
	 * @param increment If true, the counts are incremented, otherwise they are decremented.
	 *
	 * This functions updates the frequency list of file-parts, using the clients 
	 * parts-status. This function should be called by clients every time they update their
	 * parts-status, or when they are added or removed from the file.
	 */
	void	UpdatePartsFrequency( CUpDownClient* client, bool increment );

	ArrayOfUInts16	m_SrcpartFrequency;

	void	SetShowSources( bool val )	{ m_showSources = val; }
	bool	ShowSources()				const { return m_showSources; }

	typedef std::set<CUpDownClient*> SourceSet;
	
	const SourceSet& GetSourceList()	const { return m_SrcList; }
	const SourceSet& GetA4AFList()		const { return m_A4AFsrclist; }

	const CReqBlockPtrList	GetRequestedBlockList() const { return m_requestedblocks_list; }

	// LEGACY - to be removed when possible
	class CGapPtrList {
	public:
		void Init(const CGapList * gaplist) { m_gaplist = gaplist; }
		class const_iterator {
			CGapList::const_iterator m_it;
			Gap_Struct m_gap;
		public:
			const_iterator() {};
			const_iterator(const CGapList::const_iterator& it) { m_it = it; };
			bool operator != (const const_iterator& it) { return m_it != it.m_it; }
			const_iterator& operator ++ () { ++ m_it; return *this; }
			Gap_Struct * operator * () { 
				m_gap.start = m_it.start();
				m_gap.end = m_it.end();
				return & m_gap; 
			}
		};
		const_iterator begin() const { return const_iterator(m_gaplist->begin()); }
		const_iterator end() const { return const_iterator(m_gaplist->end()); }
		bool empty() const { return m_gaplist->IsComplete(); }
		uint32 size() const { return m_gaplist->size(); }

	private:
		const CGapList * m_gaplist;
	};
	// this function must stay, but return the m_gaplist instead
	const CGapPtrList& GetGapList() const { return m_gapptrlist; }
	// meanwhile use
	const CGapList& GetNewGapList() const { return m_gaplist; }
	private:
	CGapPtrList m_gapptrlist;
	public:
	// END LEGACY

	/**
	 * Adds a source to the list of dead sources.
	 *
	 * @param client The source to be recorded as dead for this file.
	 */
	void		AddDeadSource(const CUpDownClient* client);

	/**
	 * Checks if a source is recorded as being dead for this file.
	 *
	 * @param client The client to evaluate.
	 * @return True if dead, false otherwise.
	 *
	 * Sources that are dead are not to be considered valid
	 * sources and should not be added to the partfile.
	 */
	bool		IsDeadSource(const CUpDownClient* client);
	
	/* Kad Stuff */
	uint16	GetMaxSources() const;
	uint16	GetMaxSourcePerFileSoft() const;
	uint16	GetMaxSourcePerFileUDP() const;		 

	void GetRatingAndComments(FileRatingList & list) const;

	void	AllocationFinished();
private:
#ifndef CLIENT_GUI
	// partfile handle (opened on demand)
	CFileAutoClose	m_hpartfile;
	//! A local list of sources that are invalid for this file.
	CDeadSourceList	m_deadSources;

	class CCorruptionBlackBox* m_CorruptionBlackBox;
#endif

	uint16	m_notCurrentSources;

	bool	m_showSources;
	
	uint32	m_validSources;

	void	AddGap(uint64 start, uint64 end);
	void	AddGap(uint16 part);
	void	FillGap(uint64 start, uint64 end);
	void	FillGap(uint16 part);
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result);
	bool	IsAlreadyRequested(uint64 start, uint64 end);
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();

	bool	CheckFreeDiskSpace( uint64 neededSpace = 0 );
	
	bool	IsCorruptedPart(uint16 partnumber);
	
	uint32	m_iLastPausePurge;
	uint16	m_count;
	uint16	transferingsrc;
	uint64  completedsize;
	uint64	transferred;
	
	uint64	m_iLostDueToCorruption;
	uint64	m_iGainDueToCompression;
	uint32  m_iTotalPacketsSavedDueToICH;
	float 	kBpsDown;
	CPath	m_fullname;			// path/name of the met file
	CPath	m_partmetfilename;	// name of the met file
	CPath 	m_PartPath; 		// path/name of the partfile
	bool	m_paused;
	bool	m_stopped;
	bool	m_insufficient;
	uint8   m_iDownPriority;
	bool    m_bAutoDownPriority;
	uint8	status;
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CGapList m_gaplist;
	CReqBlockPtrList m_requestedblocks_list;
	double	percentcompleted;
	std::list<uint16> m_corrupted_list;
	uint16	m_availablePartsCount;
	uint32	m_ClientSrcAnswered;
	bool	m_bPercentUpdated;

	void	PerformFileComplete();

	uint32		m_lastRefreshedDLDisplay;

	// Buffered data to be written
	std::list<class PartFileBufferedData*> m_BufferedData_list;
	
	uint32 m_nTotalBufferData;
	uint32 m_nLastBufferFlushTime;

	uint8	m_category;
	uint32	m_nDlActiveTime;
	time_t  m_tActivated;
	bool	m_is_A4AF_auto;

	SourceSet	m_SrcList;
	SourceSet	m_A4AFsrclist;
	bool		m_hashsetneeded;
	uint32		m_lastsearchtime;
	bool		m_localSrcReqQueued;

#ifdef CLIENT_GUI
	FileRatingList m_FileRatingList;
	const FileRatingList &GetFileRatingList() { return m_FileRatingList; }
	void ClearFileRatingList() { m_FileRatingList.clear(); }
	void AddFileRatingList(const wxString & u, const wxString & f, sint16 r, const wxString & c) { 
	       m_FileRatingList.push_back(SFileRating(u, f, r, c)); }

	uint32 	m_kbpsDown;
	uint8   m_iDownPriorityEC;
	SourcenameItemMap m_SourcenameItemMap;
public:
	SourcenameItemMap &GetSourcenameItemMap() { return m_SourcenameItemMap; }
#endif
public:
	bool IsHashSetNeeded() const				{ return m_hashsetneeded; }
	void SetHashSetNeeded(bool value)			{ m_hashsetneeded = value; }
	
	uint64  GetCompletedSize() const			{ return completedsize; }
	void	SetCompletedSize(uint64 size)		{ completedsize = size; }	

	bool IsLocalSrcRequestQueued() const		{ return m_localSrcReqQueued; }
	void SetLocalSrcRequestQueued(bool value) 	{ m_localSrcReqQueued = value; }

	void AddA4AFSource(CUpDownClient* src)		{ m_A4AFsrclist.insert(src); }
	bool RemoveA4AFSource(CUpDownClient* src)	{ return (m_A4AFsrclist.erase(src) > 0); }

	uint32 GetLastSearchTime() const			{ return m_lastsearchtime; }
	void SetLastSearchTime(uint32 time)			{ m_lastsearchtime = time; }
	
//	void CleanUpSources( bool noNeeded, bool fullQueue = false, bool highQueue = false );

	void AddDownloadingSource(CUpDownClient* client);
          
	void RemoveDownloadingSource(CUpDownClient* client);
	void SetStatus(uint8 in);
	void StopPausedFile();

	// [sivka / Tarod] Imported from eMule 0.30c (Creteil) ... 
	void SetA4AFAuto(bool in)		{ m_is_A4AF_auto = in; }
	bool IsA4AFAuto() const			{ return m_is_A4AF_auto; }
	
	// Kry -Sources seeds
	void SaveSourceSeeds();
	void LoadSourceSeeds();
	
	// Dropping slow sources
	CUpDownClient* GetSlowerDownloadingClient(uint32 speed, CUpDownClient* caller);

  // Read data for sharing
	bool ReadData(class CFileArea & area, uint64 offset, uint32 toread);

private:
	/* downloading sources list */
	CClientPtrList m_downloadingSourcesList;

	/* Kad Stuff */
	uint32	m_LastSearchTimeKad;
	uint8	m_TotalSearchesKad;

friend class CDownQueueRem;
friend class CPartFileConvert;
};

#endif // PARTFILE_H
// File_checked_for_headers
