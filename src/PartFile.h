//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef PARTFILE_H
#define PARTFILE_H


#include "KnownFile.h"		// Needed for CKnownFile
#include "CFile.h"		// Needed for CFile

#include "OtherStructs.h"	// Needed for Gap_Struct
#include "DeadSourceList.h"	// Needed for CDeadSourceList



class CSearchFile;
class CUpDownClient;
class CMemFile;
class CFileDataIO;
class CED2KFileLink;

//#define BUFFER_SIZE_LIMIT	500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	60000   // Max milliseconds before forcing a flush

#define	STATES_COUNT			13

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

struct PartFileBufferedData
{
	byte *data;						// Barry - This is the data to be written
	uint64 start;					// Barry - This is the start offset of the data
	uint64 end;						// Barry - This is the end offset of the data
	Requested_Block_Struct *block;	// Barry - This is the requested block that this data relates to
};


struct SFileRating
{
	wxString UserName;
	wxString FileName;
	sint16   Rating;
	wxString Comment;
};

typedef std::list<SFileRating*> FileRatingList;

class CPartFile : public CKnownFile {
public:
	typedef std::list<Gap_Struct*> CGapPtrList;
	typedef std::list<Requested_Block_Struct*> CReqBlockPtrList;

	
	CPartFile();
#ifdef CLIENT_GUI
	CPartFile(CEC_PartFile_Tag *tag);
#else 
	virtual void	SetFileName(const wxString& strmakeFilename);
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
	uint8	LoadPartFile(const wxString& in_directory, const wxString& filename, bool from_backup = false, bool getsizeonly = false);
	bool	SavePartFile(bool Initial = false);
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted
	
	bool    CheckShowItemInGivenCat(int inCategory);

	bool	IsComplete(uint64 start, uint64 end);
	
	void	UpdateCompletedInfos();

	bool	GetNextRequestedBlock(CUpDownClient* sender,Requested_Block_Struct** newblocks,uint16* count);
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
	const wxString& GetPartMetFileName() const { return m_partmetfilename; }
	uint64	GetTransfered() const		{ return transfered; }
	const wxString& GetFullName() const	{ return m_fullname; }
	float	GetKBpsDown() const		{ return kBpsDown; }
	double	GetPercentCompleted() const	{ return percentcompleted; }

#ifndef CLIENT_GUI
	uint16	GetSourceCount() const		{ return m_SrcList.size(); }
	uint16	GetSrcA4AFCount() const		{ return m_A4AFsrclist.size(); }
#else
	uint16 m_source_count, m_a4af_source_count;
	uint16	GetSourceCount() const		{ return m_source_count; }
	uint16	GetSrcA4AFCount() const		{ return m_a4af_source_count; }
#endif
	uint16	GetTransferingSrcCount() const	{ return transferingsrc; }
	uint16  GetNotCurrentSourcesCount()	const	{ return m_notCurrentSources; };
	void	SetNotCurrentSourcesCount(uint16 new_count)	{ m_notCurrentSources = new_count; };	
	uint16	GetValidSourcesCount() const	{ return m_validSources; };
	
	uint64	GetNeededSpace();
	
	wxString GetFeedback();
	
	wxString getPartfileStatus() const; //<<--9/21/02
	sint32	getTimeRemaining() const; //<<--9/21/02
	time_t	lastseencomplete;
	int	getPartfileStatusRang() const;

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, byte *data, uint64 start, uint64 end, Requested_Block_Struct *block);
	void	FlushBuffer(bool forcewait=false, bool bForceICH = false, bool bNoAICH = false);	

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
	uint8	GetAvailablePartCount() const	{ return m_availablePartsCount; }
	uint32	GetLastAnsweredTime() const	{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime();
	void	SetLastAnsweredTimeTimeout();
	uint64	GetLostDueToCorruption() const	{ return m_iLostDueToCorruption; }
	uint64	GetGainDueToCompression() const	{ return m_iGainDueToCompression; }
	uint32	TotalPacketsSavedDueToICH()const{ return m_iTotalPacketsSavedDueToICH; }
	bool	IsStopped() const		{ return m_stopped; }
	bool	IsPaused() const		{ return m_paused; }
	void	UpdateFileRatingCommentAvail();

	int	GetCommonFilePenalty();
	void	UpdateDisplayedInfo(bool force = false);
	
	const wxDateTime& GetLastChangeDatetime() const { return m_lastDateChanged; }
	uint8	GetCategory() const { return m_category; }
	void	SetCategory(uint8 cat);

	CFile	m_hpartfile;	//permanent opened handle to avoid write conflicts
	volatile bool m_bPreviewing;
	void	SetDownPriority(uint8 newDownPriority, bool bSave = true, bool bRefresh = true);
	bool	IsAutoDownPriority() const	{ return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool flag)	{ m_bAutoDownPriority = flag; }
	void	UpdateAutoDownPriority();
	uint8	GetDownPriority() const		{ return m_iDownPriority; }
	void	SetActive(bool bActive);
	uint32	GetDlActiveTime() const;
	bool	GetInsufficient() const		{ return m_insufficient; }
	
	void	CompleteFileEnded(bool errorOccured, const wxString& newname);	

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

	const CGapPtrList		GetGapList() const	{ return m_gaplist; }
	const CReqBlockPtrList	GetRequestedBlockList() const { return m_requestedblocks_list; }


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

	void GetRatingAndComments(FileRatingList& list);

private:
	//! A local list of sources that are invalid for this file.
#ifndef CLIENT_GUI
	CDeadSourceList	m_deadSources;
#endif

	uint16	m_notCurrentSources;

	bool	m_showSources;
	
	uint32	m_validSources;

	void	AddGap(uint64 start, uint64 end);
	void	FillGap(uint64 start, uint64 end);
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result);
	bool	IsAlreadyRequested(uint64 start, uint64 end);
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();

	bool	CheckFreeDiskSpace( uint32 neededSpace = 0 );
	
	bool	IsCorruptedPart(uint16 partnumber);
	
	uint64	GetTotalGapSizeInPart(uint32 uPart) const;

	uint64	GetTotalGapSizeInRange(uint64 uRangeStart, uint64 uRangeEnd) const;	
	
	uint32	m_iLastPausePurge;
	uint16	m_count;
	uint16	m_anStates[STATES_COUNT];
	uint16	transferingsrc;
	uint64  completedsize;
	uint64	transfered;
	
	uint64	m_iLostDueToCorruption;
	uint64	m_iGainDueToCompression;
	uint32  m_iTotalPacketsSavedDueToICH;
	float 	kBpsDown;
	wxString m_fullname;
	wxString m_partmetfilename;
	bool	m_paused;
	bool	m_stopped;
	bool	m_insufficient;
	uint8   m_iDownPriority;
	bool    m_bAutoDownPriority;
	uint8	status;
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CGapPtrList m_gaplist;
	CReqBlockPtrList m_requestedblocks_list;
	double	percentcompleted;
	std::list<uint16> m_corrupted_list;
	uint8	m_availablePartsCount;
	uint32	m_ClientSrcAnswered;
	bool	m_bPercentUpdated;

	void	PerformFileComplete();

	uint32		m_lastRefreshedDLDisplay;
	wxDateTime	m_lastDateChanged;

	// Barry - Buffered data to be written
	std::list<PartFileBufferedData*> m_BufferedData_list;
	
	uint32 m_nTotalBufferData;
	uint32 m_nLastBufferFlushTime;

	uint8	m_category;
	uint32	m_nDlActiveTime;
	time_t  m_tActivated;
	bool	m_is_A4AF_auto;

	uint32	m_LastSourceDropTime;

	SourceSet	m_SrcList;
	SourceSet	m_A4AFsrclist;
	bool		m_hashsetneeded;
	uint32		m_lastsearchtime;
	bool		m_localSrcReqQueued;
	
public:
	bool IsHashSetNeeded() const				{ return m_hashsetneeded; }
	void SetHashSetNeeded(bool value)			{ m_hashsetneeded = value; }
	
	uint64  GetCompletedSize() const			{ return completedsize; }
	void	SetCompletedSize(uint64 size)		{ completedsize = size; }	

	bool IsLocalSrcRequestQueued() const		{ return m_localSrcReqQueued; }
	void SetLocalSrcRequestQueued(bool value) 	{ m_localSrcReqQueued = value; }

	void AddA4AFSource(CUpDownClient* src)		{ m_A4AFsrclist.insert(src); }
	bool RemoveA4AFSource(CUpDownClient* src)	{ return m_A4AFsrclist.erase(src); }

	uint32 GetLastSearchTime() const			{ return m_lastsearchtime; }
	void SetLastSearchTime(uint32 time)			{ m_lastsearchtime = time; }
	

	void CleanUpSources( bool noNeeded, bool fullQueue = false, bool highQueue = false );

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
	
	CUpDownClient* GetSlowerDownloadingClient(uint32 speed, CUpDownClient* caller) ;

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
