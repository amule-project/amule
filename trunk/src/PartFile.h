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
#include <wx/dcmemory.h>	// Needed for wxMemoryDC

#include "types.h"		// Needed for uint8
#include "KnownFile.h"		// Needed for CKnownFile
#include "CFile.h"		// Needed for CFile
#include "GetTickCount.h"	// Needed for GetTickCount

#include "updownclient.h"  // temporarily needed for #define DOWNLOADRATE_FILTERED

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

//#define BUFFER_SIZE_LIMIT	500000 // Max bytes before forcing a flush
#define BUFFER_TIME_LIMIT	5000   // Max milliseconds before forcing a flush

#define	STATES_COUNT			13

class CSearchFile;
class CUpDownClient;
class completingThread;
class Requested_Block_Struct;
class CMemFile;
class Gap_Struct;

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
	CPartFile(CString edonkeylink);
	CPartFile(class CED2KFileLink* fileLink);
	void InitializeFromLink(CED2KFileLink* fileLink);
	virtual ~CPartFile();

	void 	SetPartFileStatus(uint8 newstatus);
	bool	CreateFromFile(char* directory,char* filename)	{return false;}// not supported in this class
	bool	LoadFromFile(FILE* file)						{return false;}
	bool	WriteToFile(FILE* file)							{return false;}
	bool	IsPartFile()									{return !(status == PS_COMPLETE);}
	uint32	Process(uint32 reducedownload, uint8 m_icounter);
	uint8	LoadPartFile(LPCTSTR in_directory, LPCTSTR filename, bool getsizeonly=false);
	bool	SavePartFile(bool Initial=false);
	void	PartFileHashFinished(CKnownFile* result);
	bool	HashSinglePart(uint16 partnumber); // true = ok , false = corrupted
	uint64	GetRealFileSize();
	
	// TODO: check files atributes
	//bool	IsNormalFile() const { return (m_dwFileAttributes & (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE)) == 0; }
	bool	IsNormalFile() const { return true; }
	
	void	AddGap(uint32 start, uint32 end);
	void	FillGap(uint32 start, uint32 end);
	void	DrawStatusBar(wxMemoryDC* dc, wxRect rect, bool bFlat);
	void    DrawShareStatusBar(wxMemoryDC* dc, wxRect rect, bool onlygreyrect, bool bFlat);
	bool	IsComplete(uint32 start, uint32 end);
	bool	IsPureGap(uint32 start, uint32 end);
	bool	IsCorruptedPart(uint16 partnumber);
	void	UpdateCompletedInfos();

	bool	GetNextRequestedBlock(CUpDownClient* sender,Requested_Block_Struct** newblocks,uint16* count);
	void	WritePartStatus(CMemFile* file);
	void	WriteCompleteSourcesCount(CMemFile* file);
	bool 	CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped);
	void	AddSources(CMemFile* sources,uint32 serverip, uint16 serverport);
	uint8	GetStatus(bool ignorepause = false);
	void	NewSrcPartsInfo();	
	char*	GetPartMetFileName()							{return m_partmetfilename;}
	uint32	GetTransfered()								{return transfered;}
	char*	GetFullName()								{return fullname;}
	uint16	GetSourceCount();
	uint16	GetSrcA4AFCount()							{return A4AFsrclist.GetCount();}
	uint16	GetTransferingSrcCount()						{return transferingsrc;}
#ifdef DOWNLOADRATE_FILTERED
	float	GetKBpsDown()									{ return kBpsDown; }
#else
	uint32	GetDatarate()								{return datarate;}
#endif
	float	GetPercentCompleted()							{return percentcompleted;}
	uint16  GetNotCurrentSourcesCount();
	int	GetValidSourcesCount();
	uint32	GetNeededSpace();
	bool	IsMovie();
	bool	IsSound();
	bool	IsArchive(); 
	bool	IsCDImage(); 
	bool 	IsImage();
	bool 	IsText();
	
	
	CString CPartFile::getPartfileStatus(); //<<--9/21/02
	sint32	CPartFile::getTimeRemaining(); //<<--9/21/02
	time_t	lastseencomplete;
	int		getPartfileStatusRang();
        CString GetDownloadFileInfo();

	// Barry - Added as replacement for BlockReceived to buffer data before writing to disk
	uint32	WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block);
	void	FlushBuffer(void);
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

	virtual	Packet* CreateSrcInfoPacket(CUpDownClient* forClient);
	//void	AddClientSources(CMemFile* sources);
	void    AddClientSources(CMemFile* sources,uint8 sourceexchangeversion);

	void	PreviewFile();
	bool	PreviewAvailable();
	uint8   GetAvailablePartCount()			{return availablePartsCount;}
	void	UpdateAvailablePartsCount();

	uint32	GetLastAnsweredTime()			{ return m_ClientSrcAnswered; }
	void	SetLastAnsweredTime()			{ m_ClientSrcAnswered = ::GetTickCount(); }
	void	SetLastAnsweredTimeTimeout()		{ m_ClientSrcAnswered = 2 * CONNECTION_LATENCY +
											                        ::GetTickCount() - SOURCECLIENTREASK; }
	uint64	GetLostDueToCorruption()		{return m_iLostDueToCorruption;}
	uint64	GetGainDueToCompression()		{return m_iGainDueToCompression;}
	uint32	TotalPacketsSavedDueToICH()		{return m_iTotalPacketsSavedDueToICH;}
	bool	IsStopped() 				{return stopped;}
	bool	HasComment()				{return hasComment;}
	bool	HasRating()				{return hasRating;}
	bool	HasBadRating()				{return hasBadRating;}
	void	SetHasComment(bool in)			{hasComment=in;}
	void	SetHasRating(bool in)			{hasRating=in;}
	void	UpdateFileRatingCommentAvail();

        wxString GetProgressString(uint16 size);

	int	GetCommonFilePenalty();
	void	UpdateDisplayedInfo(bool force=false);
	time_t	GetLastChangeDatetime(bool forcecheck=false);
	uint8	GetCategory();
	void	SetCategory(uint8 cat)			{m_category=cat;SavePartFile();}

	CFile	m_hpartfile;	//permanent opened handle to avoid write conflicts
	volatile bool m_bPreviewing;
	CTypedPtrList<CPtrList, CUpDownClient*> A4AFsrclist; //<<-- enkeyDEV(Ottavio84) -A4AF-
	void	SetDownPriority(uint8 newDownPriority);
	bool	IsAutoDownPriority()	{ return m_bAutoDownPriority; }
	void	SetAutoDownPriority(bool flag) { m_bAutoDownPriority = flag; }
	void	UpdateAutoDownPriority();
	uint8	GetDownPriority()	{ return m_iDownPriority; }
	completingThread* cthread;
	bool GetInsufficient() { return insufficient; }
		
protected:
	bool	GetNextEmptyBlockInPart(uint16 partnumber,Requested_Block_Struct* result);
	bool	IsAlreadyRequested(uint32 start, uint32 end);
	void	CompleteFile(bool hashingdone);
	void	CreatePartFile();
	void	Init();
	wxMutex 	m_FileCompleteMutex; // Lord KiRon - Mutex for file completion
private:
	uint32	m_iLastPausePurge;
	uint16	count;
	uint16	m_anStates[STATES_COUNT];
	uint16	transferingsrc;
	uint32  completedsize;
	uint64	m_iLostDueToCorruption;
	uint64	m_iGainDueToCompression;
	uint32  m_iTotalPacketsSavedDueToICH;
#ifdef DOWNLOADRATE_FILTERED
	float 	kBpsDown;
#else
	uint32	datarate;
#endif
	char*	fullname;
	char*	m_partmetfilename;
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
	ArrayOfUInts16	m_SrcpartFrequency;
	float	percentcompleted;
	CList<uint16, uint16> corrupted_list;
	uint8	availablePartsCount;
	uint32	m_ClientSrcAnswered;
	uint32	m_nSavedReduceDownload;
	bool	m_bPercentUpdated;
	static	CBarShader s_LoadBar;
	static	CBarShader s_ChunkBar;
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
	CTypedPtrList<CPtrList, CUpDownClient*> srclists[SOURCESSLOTS];
	
	// Kry - Avoid counting again and again (mayor cpu leak)
	bool	IsCountDirty;
	uint16	CleanCount;

	bool	srcarevisible;		// used for downloadlistctrl
	bool	m_bShowOnlyDownloading;	// used for downloadlistctrl
	bool	hashsetneeded;
	uint32  GetCompletedSize()   {return completedsize;}

	uint32	lastsearchtime;
	bool	m_bLocalSrcReqQueued;

	/* Razor 1a - Modif by MikaelB */

          /* RemoveNoNeededSources function */
          void	RemoveNoNeededSources();

          /* RemoveFullQueueSources function */
          void	RemoveFullQueueSources();

          /* RemoveHighQueueRatingSources function */
          void	RemoveHighQueueRatingSources();

          /* CleanUpSources function */
          void	CleanUpSources();

          /* AddDownloadingSource function */
          void AddDownloadingSource(CUpDownClient* client);
          
          /* RemoveDownloadingSource function */
          void RemoveDownloadingSource(CUpDownClient* client);
          void	SetStatus(uint8 in);
          void	StopPausedFile();

          /* A4AF sources list */
          CTypedPtrList<CPtrList, CUpDownClient*> A4AFSourcesList;

	// void SetA4AFAuto(bool A4AFauto)
	void	SetA4AFAuto(bool in)			{m_is_A4AF_auto = in;} // [sivka / Tarod] Imported from eMule 0.30c (Creteil) ...
	bool	IsA4AFAuto()				{return m_is_A4AF_auto;} // [sivka / Tarod] Imported from eMule 0.30c (Creteil) ...
	
	// Kry -Sources seeds
	void SaveSourceSeeds();
	void LoadSourceSeeds();

private:

          /* downloading sources list */
          CTypedPtrList<CPtrList, CUpDownClient*> m_downloadingSourcesList;


/* End modif */
friend class completingThread;
};

class completingThread : public wxThread
{
  private:
  void* Entry();
  int result;
  CPartFile* completing;

  public:

  ~completingThread();
  completingThread::completingThread(CPartFile*);
  completingThread();

  void setFile(CPartFile*);
  void OnExit();

};

#endif // PARTFILE_H
