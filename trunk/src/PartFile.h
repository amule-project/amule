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

#ifndef PARTFILE_H
#define PARTFILE_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/thread.h>		// Needed for wxMutex

#include "types.h"		// Needed for uint8
#include "KnownFile.h"		// Needed for CKnownFile
#include "CFile.h"		// Needed for CFile

#include "otherstructs.h"	// Needed for Gap_Struct
#include "CTypedPtrList.h"

#include <set>
#include <list>

class CSearchFile;
class CUpDownClient;
class completingThread;
class CSafeMemFile;
class wxMemoryDC;
class wxRect;


//#define BUFFER_SIZE_LIMIT	500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	60000   // Max milliseconds before forcing a flush

#define	STATES_COUNT			13

// Ok, eMule and aMule are building incompatible backup files because 
// of the different name. aMule was using ".BAK" and eMule ".bak". 
// This should fix it.
#define   PARTMET_BAK_EXT _T(".bak")


struct PartFileBufferedData
{
	BYTE *data;						// Barry - This is the data to be written
	uint32 start;					// Barry - This is the start offset of the data
	uint32 end;						// Barry - This is the end offset of the data
	Requested_Block_Struct *block;	// Barry - This is the requested block that this data relates to
};

class CPartFile : public CKnownFile {
public:
	CPartFile();
	CPartFile(CSearchFile* searchresult);  //used when downloading a new file
	CPartFile(const wxString& edonkeylink);
	CPartFile(class CED2KFileLink* fileLink);
	void InitializeFromLink(CED2KFileLink* fileLink);
	virtual ~CPartFile();
	
	bool	CreateFromFile(wxString WXUNUSED(directory), wxString WXUNUSED(filename), void* WXUNUSED(pvProgressParam)) {return false;}// not supported in this class
	void 	SetPartFileStatus(uint8 newstatus);
	virtual bool LoadFromFile(const CFile* WXUNUSED(file)) const { return false; }
	bool	WriteToFile(CFile* WXUNUSED(file)) const	{ return false; }
	bool	IsPartFile() const		{ return !(status == PS_COMPLETE); }
	uint32	Process(uint32 reducedownload, uint8 m_icounter);
	uint8	LoadPartFile(wxString in_directory, wxString filename, bool from_backup = false, bool getsizeonly = false);
	bool	SavePartFile(bool Initial = false);
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted
	uint64	GetRealFileSize();
	
	bool	IsNormalFile() const 		{ return true; }
	bool    CheckShowItemInGivenCat(int inCategory);
	void	AddGap(uint32 start, uint32 end);
	void	FillGap(uint32 start, uint32 end);

#ifndef AMULE_DAEMON
	void	DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool bFlat);
#endif
	
	bool	IsComplete(uint32 start, uint32 end);
	bool	IsPureGap(uint32 start, uint32 end);
	bool	IsCorruptedPart(uint16 partnumber);
	uint32	GetTotalGapSizeInRange(uint32 uRangeStart, uint32 uRangeEnd) const;	
	uint32	GetTotalGapSizeInPart(UINT uPart) const;
	void	UpdateCompletedInfos();

	bool	GetNextRequestedBlock(CUpDownClient* sender,Requested_Block_Struct** newblocks,uint16* count);
	void	WritePartStatus(CSafeMemFile* file);
	void	WriteCompleteSourcesCount(CSafeMemFile* file);
	bool 	CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped);
	void	AddSources(CSafeMemFile& sources,uint32 serverip, uint16 serverport);
	uint8	GetStatus(bool ignorepause = false) const;
	virtual void	UpdatePartsInfo();
	const wxString& GetPartMetFileName() const { return m_partmetfilename; }
	uint32	GetTransfered() const		{ return transfered; }
	const wxString& GetFullName() const	{ return m_fullname; }
	uint16	GetSourceCount() const		{ return m_SrcList.size(); }
	uint16	GetSrcA4AFCount() const		{ return A4AFsrclist.size(); }
	uint16	GetTransferingSrcCount() const	{ return transferingsrc; }
	float	GetKBpsDown() const		{ return kBpsDown; }
	double	GetPercentCompleted() const	{ return percentcompleted; }
	uint16  GetNotCurrentSourcesCount()	const	{ return m_notCurrentSources; };
	int	GetValidSourcesCount()			const	{ return m_validSources; };
	uint32	GetNeededSpace();
	
	wxString getPartfileStatus() const; //<<--9/21/02
	sint32	getTimeRemaining() const; //<<--9/21/02
	time_t	lastseencomplete;
	int	getPartfileStatusRang() const;

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block);
	void	FlushBuffer(bool forcewait=false, bool bForceICH = false, bool bNoAICH = false);	
	// Barry - This will invert the gap list, up to caller to delete gaps when done
	// 'Gaps' returned are really the filled areas, and guaranteed to be in order
	void	GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled);

	// Barry - Is archive recovery in progress
	volatile bool m_bRecoveringArchive;

	// Barry - Added to prevent list containing deleted blocks on shutdown
	void	RemoveAllRequestedBlocks(void);

	void	RemoveBlockFromList(uint32 start,uint32 end);
	void	RemoveAllSources(bool bTryToSwap);
	void	Delete();
	void	StopFile(bool bCancel = false);
	void	PauseFile(bool bInsufficient = false);
	void	ResumeFile();
	void	PauseFileInsufficient();
	void	ResumeFileInsufficient();

	virtual	Packet* CreateSrcInfoPacket(const CUpDownClient* forClient);
	//void	AddClientSources(CMemFile* sources);
	void    AddClientSources(CSafeMemFile* sources,uint8 sourceexchangeversion);

	void	PreviewFile();
	bool	PreviewAvailable();
	uint8	GetAvailablePartCount() const	{ return m_availablePartsCount; }
	uint32	GetLastAnsweredTime() const	{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime();
	void	SetLastAnsweredTimeTimeout();
	uint64	GetLostDueToCorruption() const	{ return m_iLostDueToCorruption; }
	uint64	GetGainDueToCompression() const	{ return m_iGainDueToCompression; }
	uint32	TotalPacketsSavedDueToICH()const{ return m_iTotalPacketsSavedDueToICH; }
	bool	IsStopped() const		{ return stopped; }
	bool	IsPaused() const		{ return paused; }
	bool	HasComment() const		{ return hasComment; }
	bool	HasRating() const		{ return hasRating; }
	bool	HasBadRating() const		{ return hasBadRating; }
	void	SetHasComment(bool in)		{ hasComment = in; }
	void	SetHasRating(bool in)		{ hasRating = in; }
	void	UpdateFileRatingCommentAvail();

        wxString GetProgressString(uint16 size);

	int	GetCommonFilePenalty();
	void	UpdateDisplayedInfo(bool force = false);
	time_t	GetLastChangeDatetime(bool forcecheck = false);
	uint8	GetCategory();
	void	SetCategory(uint8 cat)		{ m_category = cat; SavePartFile(); }

	CFile	m_hpartfile;	//permanent opened handle to avoid write conflicts
	volatile bool m_bPreviewing;
	void	SetDownPriority(uint8 newDownPriority, bool bSave = true, bool bRefresh = true);
	bool	IsAutoDownPriority() const	{ return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool flag)	{ m_bAutoDownPriority = flag; }
	void	UpdateAutoDownPriority();
	uint8	GetDownPriority() const		{ return m_iDownPriority; }
	completingThread* cthread;
	bool	GetInsufficient() const		{ return insufficient; }
	
	void	CompleteFileEnded(int completing_result, wxString* newname);	

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
protected:
	uint32	m_validSources;
	uint32	m_notCurrentSources;
	
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result);
	bool	IsAlreadyRequested(uint32 start, uint32 end);
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();
private:
	uint32	m_iLastPausePurge;
	uint16	count;
	uint16	m_anStates[STATES_COUNT];
	uint16	transferingsrc;
	uint32  completedsize;
	uint64	m_iLostDueToCorruption;
	uint64	m_iGainDueToCompression;
	uint32  m_iTotalPacketsSavedDueToICH;
	float 	kBpsDown;
	wxString m_fullname;
	wxString m_partmetfilename;
	uint32	transfered;
	bool	paused;
	bool	stopped;
	bool	insufficient;
	uint8   m_iDownPriority;
	bool    m_bAutoDownPriority;
	uint8	status;
	bool	newdate;	// indicates if there was a writeaccess to the .part file
	uint32	lastpurgetime;
	uint32	m_LastNoNeededCheck;
	CTypedPtrList<CPtrList, Gap_Struct*> gaplist;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> requestedblocks_list;
	double	percentcompleted;
	CList<uint16, uint16> corrupted_list;
	uint8	m_availablePartsCount;
	uint32	m_ClientSrcAnswered;
	uint32	m_nSavedReduceDownload;
	bool	m_bPercentUpdated;

	bool	hasRating;
	bool	hasBadRating;
	bool	hasComment;
	uint8 	PerformFileComplete(); // Lord KiRon
	//static unsigned int CompleteThreadProc(CPartFile* pFile); // Lord KiRon - Used as separate thread to complete file
	void    CharFillRange(wxString* buffer,uint32 start, uint32 end, char color);

	DWORD	m_lastRefreshedDLDisplay;
	DWORD   m_lastdatetimecheck;
	time_t	m_lastdatecheckvalue;

	// Barry - Buffered data to be written
	CTypedPtrList<CPtrList, PartFileBufferedData*> m_BufferedData_list;
	uint32 m_nTotalBufferData;
	uint32 m_nLastBufferFlushTime;
	uint8	m_category;
	bool	m_is_A4AF_auto;

	DWORD	m_LastSourceDropTime;

public:
	typedef std::set<CUpDownClient*> SourceSet;
	SourceSet m_SrcList;
	SourceSet A4AFsrclist; //<<-- enkeyDEV(Ottavio84) -A4AF-
	
	// Kry - Avoid counting again and again (mayor cpu leak)
	bool	IsCountDirty;
	uint16	CleanCount;

	bool	srcarevisible;		// used for downloadlistctrl
	bool	m_bShowOnlyDownloading;	// used for downloadlistctrl
	bool	hashsetneeded;
	uint32  GetCompletedSize() const	{ return completedsize; }

	uint32	lastsearchtime;
	bool	m_bLocalSrcReqQueued;

	/* CleanUpSources function */
	void CleanUpSources( bool noNeeded, bool fullQueue = false, bool highQueue = false );

	/* AddDownloadingSource function */
	void AddDownloadingSource(CUpDownClient* client);
          
	/* RemoveDownloadingSource function */
	void RemoveDownloadingSource(CUpDownClient* client);
	void SetStatus(uint8 in);
	void StopPausedFile();

	// void SetA4AFAuto(bool A4AFauto)
	// [sivka / Tarod] Imported from eMule 0.30c (Creteil) ... 
	void SetA4AFAuto(bool in)		{ m_is_A4AF_auto = in; }
	bool IsA4AFAuto() const			{ return m_is_A4AF_auto; }
	
	// Kry -Sources seeds
	void SaveSourceSeeds();
	void LoadSourceSeeds();

private:
	/* downloading sources list */
	std::list<CUpDownClient *> m_downloadingSourcesList;
	static	wxMutex m_FileCompleteMutex;

friend class CPartFile_Encoder;
friend class completingThread;
};

class completingThread : public wxThread
{
public:
	~completingThread();
	completingThread(wxString FileName, wxString fullname, uint32 Category, CPartFile* caller);
	completingThread() { };

private:
	virtual	bool InitInstance() { return true; }
	virtual wxThread::ExitCode Entry();
	uint8 completing_result;
	uint32 Completing_Category;
	wxString Completing_FileName;
	wxString Completing_Fullname;
	wxString* newname;
	CPartFile* completing;
	virtual void OnExit();
};

#endif // PARTFILE_H
