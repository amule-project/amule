// This file is part of aMule Project
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


#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "types.h"		// Needed for int8, int16, int32, int64, uint8, uint16, uint32 and uint64

#include "CMD4Hash.h"
#include "MD5Sum.h"
#include "otherfunctions.h"

#include <wx/dynarray.h>

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/string.h>		// Needed for wxString
#if wxCHECK_VERSION(2, 5, 2)
#	include <wx/arrstr.h>	// Needed for wxArrayString
#endif
#include <wx/config.h>
#include <wx/valgen.h>
#include <wx/tokenzr.h>
#include <wx/control.h>

#include <map>
#include <list>

enum EViewSharedFilesAccess{
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

#define DEFAULT_COL_SIZE 65535

// DO NOT EDIT VALUES like making a uint16 to uint32, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct{
	int8	version;
	unsigned char	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()

#pragma pack(1)
struct Category_Struct{
	wxString	incomingpath;
	wxString	title;
	wxString	comment;
	DWORD	color;
	uint8	prio;
};
#pragma pack()

#undef Bool	// Yeah right.
/**
 * Base-class for automatically loading and saving of preferences.
 *
 * The purpose of this class is to perform two tasks:
 * 1) To load and save a variable using wxConfig
 * 2) If nescecarry, to syncronize it with a widget
 *
 * This pure-virtual class servers as the base of all the Cfg types
 * defined below, and exposes the entire interface.
 *
 * Please note that for reasons of simplicity these classes dont provide
 * direct access to the variables they maintain, as there is no real need
 * for this.
 *
 * To create a sub-class you need only provide the Load/Save functionality,
 * as it is given that not all variables have a widget assosiated.
 */
class Cfg_Base
{
public:
	/**
	 * Constructor.
	 *
	 * @param keyname This is the keyname under which the variable is to be saved.
	 */
	Cfg_Base( const wxString& keyname )
	 : m_key( keyname ),
	   m_changed( false )
	{}

	/**
	 * Destructor.
	 */
	virtual ~Cfg_Base() {}

	/**
	 * This function loads the assosiated variable from the provided config object.
	 */
	virtual void LoadFromFile(wxConfigBase* cfg) = 0;
	/**
	 * This function saves the assosiated variable to the provided config object.
	 */
	virtual void SaveToFile(wxConfigBase* cfg) = 0;

	/**
	 * Syncs the variable with the contents of the widget.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferFromWindow()	{ return false; }
	/**
	 * Syncs the widget with the contents of the variable.
	 *
	 * @return True of success, false otherwise.
	 */
	virtual bool TransferToWindow()		{ return false; }

	/**
	 * Connects a widget with the specified ID to the Cfg object.
	 *
	 * @param id The ID of the widget.
	 * @param parent A pointer to the widgets parent, to speed up searches.
	 * @return True on success, false otherwise.
	 *
	 * This function only makes sense for Cfg-classes that have the capability
	 * to interact with a widget, see Cfg_Tmpl::ConnectToWidget().
	 */	 
	virtual	bool ConnectToWidget( int WXUNUSED(id), wxWindow* WXUNUSED(parent) = NULL )	{ return false; }

	/**
	 * Gets the key assosiated with Cfg object.
	 *
	 * @return The config-key of this object.
	 */
	virtual const wxString& GetKey()	{ return m_key; }


	/**
	 * Specifies if the variable has changed since the TransferFromWindow() call.
	 *
	 * @return True if the variable has changed, false otherwise.
	 */
	virtual bool HasChanged() 			{ return m_changed; }


protected:
	/**
	 * Sets the changed status.
	 *
	 * @param changed The new status.
	 */
	virtual void SetChanged( bool changed )
	{ 
		m_changed = changed;
	};
	
private:

	//! The Config-key under which to save the variable
	wxString	m_key;

	//! The changed-status of the variable
	bool		m_changed;
};


/**
 * Template Cfg class for connecting with widgets.
 *
 * This template provides the base functionionality needed to syncronize a 
 * variable with a widget. However, please note that wxGenericValidator only
 * supports a few types (int, wxString, bool and wxArrayInt), so this template 
 * can't always be used directly.
 *
 * Cfg_Str and Cfg_Bool are able to use this template directly, whereas Cfg_Int
 * makes use of serveral workaround to enable it to be used with integers other
 * than int.
 */
template <typename TYPE>
class Cfg_Tmpl : public Cfg_Base
{
public:
	/**
	 * Constructor.
	 *
	 * @param keyname
	 * @param value
	 * @param defaultVal
	 */
	Cfg_Tmpl( const wxString& keyname, TYPE& value, const TYPE& defaultVal )
	 : Cfg_Base( keyname ),
	   m_value( value ),
	   m_default( defaultVal ),
	   m_widget( NULL )
	{}

#ifndef AMULE_DAEMON
	/**
	 * Connects the Cfg to a widget.
	 * 
	 * @param id The ID of the widget to be connected.
	 * @param parent The parent of the widget. Use this to speed up searches.
	 *
	 * This function works by setting the wxValidator of the class. This however
	 * poses some restrictions on which variable types can be used for this
	 * template, as noted above. It also poses some limits on the widget types,
	 * refer to the wx documentation for those.
	 */
	virtual	bool ConnectToWidget( int id, wxWindow* parent = NULL )
	{
		if ( id ) {
			m_widget = wxWindow::FindWindowById( id, parent );
		
			if ( m_widget ) {
				wxGenericValidator validator( &m_value );

				m_widget->SetValidator( validator );
			
				return true;
			}
		} else {
			m_widget = NULL;
		}

		return false;
	}
	
	
	/**
	 * Sets the assosiated variable to the value of the widget.
	 *
	 * @return True on success, false otherwise.
	 */
	virtual bool TransferFromWindow()
	{
		if ( m_widget ) {
			wxValidator* validator = m_widget->GetValidator();

			if ( validator ) {
				TYPE temp = m_value;
			
				if ( validator->TransferFromWindow() ) {
					SetChanged( temp != m_value );

					return true;
				}
			}
		} 
		
		return false;
	}
	
	/**
	 * Sets the assosiated variable to the value of the widget.
	 *
	 * @return True on success, false otherwise.
	 */
	virtual bool TransferToWindow()
	{
		if ( m_widget ) {
			wxValidator* validator = m_widget->GetValidator();

			if ( validator ) {
				return validator->TransferToWindow();
			}
		}

		return false;
	}
#endif

protected:
	//! Reference to the assosiated variable
	TYPE&	m_value;

	//! Default variable value
	TYPE	m_default;
	
	//! Pointer to the widget assigned to the Cfg instance
	wxWindow*	m_widget;
};


/**
 * Cfg class for wxStrings.
 */
class Cfg_Str : public Cfg_Tmpl<wxString>
{
public:
	/**
	 * Constructor
	 *
	 *
	 */
	Cfg_Str( const wxString& keyname, wxString& value, const wxString& defaultVal = wxT("") )
	 : Cfg_Tmpl<wxString>( keyname, value, defaultVal )
	{}

	/**
	 * Saves the string to specified wxConfig.
	 *
	 * @param cfg The wxConfig to save the variable to.
	 */
	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		cfg->Read( GetKey(), &m_value, m_default );
	}
	
	/**
	 * Loads the string to specified wxConfig using the specified default value.
	 *
	 * @param cfg The wxConfig to load the variable from.
	 */
	virtual void SaveToFile(wxConfigBase* cfg)
	{
		cfg->Write( GetKey(), m_value );
	}
};


/**
 * Cfg-class for encrypting strings, for example for passwords.
 */
class Cfg_Str_Encrypted : public Cfg_Str
{
public:
	/**
	 * Constructor.
	 *
	 * @param
	 * @param
	 * @param 
	 */
	Cfg_Str_Encrypted( const wxString& keyname, wxString& value, const wxString& defaultVal = wxT("") )
	 : Cfg_Str( keyname, value, defaultVal )
	{}

	/**
	 *
	 *
	 */
	virtual bool TransferFromWindow()
	{
		// Store current value to see if it has changed
		wxString temp;

		// Shakraw, when storing value, store it encrypted here (only if changed in prefs)
		if ( Cfg_Str::TransferFromWindow() ) {
			if ( temp != m_value ) {
				if ( temp.IsEmpty() ) {
					m_value = temp;
				} else {
					m_value = MD5Sum(temp).GetHash();
				}
			}

			return true;
		}

		return false;
	}
};


/**
 * Cfg class that takes care of integer types.
 *
 * This template is needed since wxValidator only supports normals ints, and 
 * wxConfig for the matter only supports longs, thus some worksarounds are
 * needed. 
 *
 * There are two work-arounds:
 *  1) wxValidator only supports int*, so we need a immediate variable to act
 *     as a storage. Thus we use Cfg_Tmpl<int> as base class. Thus this class
 *     contains a integer which we use to pass the value back and forth 
 *     between the widgets.
 *
 *  2) wxConfig uses longs to save and read values, thus we need an immediate
 *     stage when loading and saving the value.
 */
template <typename TYPE>
class Cfg_Int : public Cfg_Tmpl<int>
{
public:
	Cfg_Int( const wxString& keyname, TYPE& value, int defaultVal = 0 )
	 : Cfg_Tmpl<int>( keyname, m_temp_value, defaultVal ),
	   m_real_value( value ),
	   m_temp_value( value )
	{}


	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		long tmp = 0;
		cfg->Read( GetKey(), &tmp, m_default ); 
			
		// Set the temp value
		m_temp_value = (int)tmp;
		// Set the actual value
		m_real_value = (TYPE)tmp;
	}

	virtual void SaveToFile(wxConfigBase* cfg)
	{
		long tmp = (long)m_real_value;
		
		cfg->Write( GetKey(), tmp );
	}
	

	virtual bool TransferFromWindow()
	{
		Cfg_Tmpl<int>::TransferFromWindow(); 

		m_real_value = (TYPE)m_temp_value;

		return true;
	}
	
	virtual bool TransferToWindow()
	{
		m_temp_value = (int)m_real_value;
	
		Cfg_Tmpl<int>::TransferToWindow();

		return true;
	}


protected:

	TYPE&	m_real_value;
	int		m_temp_value;
};


/**
 * Helper function for creating new Cfg_Ints.
 *
 * @param keyname The cfg-key under which the item should be saved.
 * @param value The variable to syncronize. The type of this variable defines the type used to create the Cfg_Int.
 * @param defaultVal The default value if the key isn't found when loading the value.
 * @return A pointer to the new Cfg_Int object. The caller is responsible for deleting it.
 *
 * This template-function returns a Cfg_Int of the appropriate type for the 
 * variable used as argument and should be used to avoid having to specify
 * the integer type when adding a new Cfg_Int, since that's just increases
 * the maintainence burden.
 */
template <class TYPE>
Cfg_Base* MkCfg_Int( const wxString& keyname, TYPE& value, int defaultVal )
{
	return new Cfg_Int<TYPE>( keyname, value, defaultVal );
}


/**
 * Cfg-class for bools.
 */
class Cfg_Bool : public Cfg_Tmpl<bool>
{
public:
	Cfg_Bool( const wxString& keyname, bool& value, bool defaultVal )
	 : Cfg_Tmpl<bool>( keyname, value, defaultVal )
	{}

	
	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		cfg->Read( GetKey(), &m_value, m_default );
	}
	
	virtual void SaveToFile(wxConfigBase* cfg)
	{
		cfg->Write( GetKey(), m_value );
	}
};


/**
 * Cfg-class for uint64s, with no assisiated widgets.
 */
class Cfg_Counter : public Cfg_Base
{
public:
	Cfg_Counter( const wxString& keyname, uint64& value )
	 : Cfg_Base( keyname ),
	   m_value( value )
	{

	}

	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		wxString buffer;
		
		cfg->Read( GetKey(), &buffer, wxT("0") );
		
		m_value = atoll(unicode2char(buffer));
	}
	
	virtual void SaveToFile(wxConfigBase* cfg)
	{
		wxString str = wxString::Format(wxT("%llu"),(unsigned long long)m_value);
		
		cfg->Write( GetKey(), str );
	}
	
protected:
	uint64& m_value;
};


/**
 * Cfg-class for arrays of uint16, with no assisiated widgets.
 */
class Cfg_Columns : public Cfg_Base
{
public:
	Cfg_Columns( const wxString& keyname, uint16* array, int count, int defaultVal )
	 : Cfg_Base( keyname ),
	   m_array( array ),
	   m_default_val( defaultVal ),
	   m_count( count )
	{
	}


	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		// Set default values
		for ( int i = 0; i < m_count; i++ )
			m_array[i] = m_default_val;
			
		wxString buffer;
		if ( cfg->Read( GetKey(), &buffer, wxT("") ) ) {
			int counter = 0;
			
			wxStringTokenizer tokenizer( buffer, wxT(",") );
			while ( tokenizer.HasMoreTokens() && ( counter < m_count ) ) 
			{
				m_array[counter++] = atoi( unicode2char( tokenizer.GetNextToken() ) );
			}
		}
	}
	
	
	virtual void SaveToFile(wxConfigBase* cfg)
	{
		wxString buffer;

		for ( int i = 0; i < m_count; i++ ) {
			if ( i ) buffer << wxT(",");

			buffer << m_array[i];
		}
	
		cfg->Write( GetKey(), buffer );
	}


protected:
	uint16*	m_array;
	int		m_default_val;
	int 	m_count;
};


WX_DECLARE_OBJARRAY(Category_Struct*, ArrayOfCategory_Struct);

const int cntStatColors = 13;


class CPreferences{
public:
	enum Table { tableDownload, tableUpload, tableQueue, tableSearch,
		tableShared, tableServer, tableClientList, tableNone };

	friend class PrefsUnifiedDlg;

	CPreferences();
	~CPreferences();

	bool	Save();
	void	SaveCats();

	static bool		Score() {return s_scorsystem;}
	static bool		Reconnect() {return s_reconnect;}
	static bool		DeadServer() {return s_deadserver;}
	static const	wxString&	GetUserNick() {return s_nick;}

	static uint16	GetPort() {return s_port;}
	static uint16	GetUDPPort() {return s_udpport;}
	static const wxString&	GetIncomingDir() {return s_incomingdir;}
	static const wxString&	GetTempDir() {return s_tempdir;}
	const CMD4Hash&	GetUserHash() {return m_userhash;}
	static uint16	GetMaxUpload() {return	s_maxupload;}
	static uint16	GetSlotAllocation() {return	s_slotallocation;}
	static bool		IsICHEnabled() {return s_ICH;}
	static bool		AutoServerlist() {return s_autoserverlist;}
	static bool		DoMinToTray() {return s_mintotray;}
	static bool		DoAutoConnect() {return s_autoconnect;}
	static void		SetAutoConnect( bool inautoconnect) {s_autoconnect = inautoconnect;}
	static bool		AddServersFromServer() {return s_addserversfromserver;}
	static bool		AddServersFromClient() {return s_addserversfromclient;}
	static uint16	GetTrafficOMeterInterval() { return s_trafficOMeterInterval;}
	static void		SetTrafficOMeterInterval(uint16 in) { s_trafficOMeterInterval=in;}
	static uint16	GetStatsInterval() { return s_statsInterval;}
	static void		SetStatsInterval(uint16 in) { s_statsInterval=in;}
	static void		Add2TotalDownloaded(uint64 in) {s_totalDownloadedBytes+=in;}
	static void		Add2TotalUploaded(uint64 in) {s_totalUploadedBytes+=in;}
	static uint64	GetTotalDownloaded() {return s_totalDownloadedBytes;}
	static uint64	GetTotalUploaded() {return s_totalUploadedBytes;}
	static bool		IsConfirmExitEnabled() {return s_confirmExit;}
	static bool		UseSplashScreen() {return s_splashscreen;}
	static bool		FilterBadIPs() {return s_filterBadIP;}
	static bool		IsOnlineSignatureEnabled()  {return s_onlineSig;}
	static uint32	GetMaxGraphUploadRate()  {return s_maxGraphUploadRate;}
	static uint32	GetMaxGraphDownloadRate()  {return s_maxGraphDownloadRate;}
	static void		SetMaxGraphUploadRate(uint32 in) {s_maxGraphUploadRate=in;}
	static void		SetMaxGraphDownloadRate(uint32 in) {s_maxGraphDownloadRate=in;}

	static uint16	GetMaxDownload()  { return s_maxdownload; }
	static uint16	GetMaxConnections() {return	s_maxconnections;}
	static uint16	GetMaxSourcePerFile() {return s_maxsourceperfile;}
	static uint16	GetMaxSourcePerFileSoft() {uint16 temp = (uint16)(s_maxsourceperfile*0.9);
						if( temp > 1000 ) return 1000;
                                            	return temp;}
	static uint16	GetMaxSourcePerFileUDP() {uint16 temp = (uint16)(s_maxsourceperfile*0.75);
						if( temp > 100 ) return 100;
                                            	return temp;}
	static uint16	GetDeadserverRetries() {return s_deadserverretries;}
	static DWORD	GetServerKeepAliveTimeout()  {return s_dwServerKeepAliveTimeoutMins*60000;}
	
	static int32	GetColumnWidth (Table t, int index);
	static bool		GetColumnHidden(Table t, int index);
	static int32	GetColumnOrder (Table t, int index);
	static void		SetColumnWidth (Table t, int index, int32 width);
	static void		SetColumnHidden(Table t, int index, bool bHidden);
	static void		SetColumnOrder (Table t, INT *piOrder);

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	static int32	GetColumnSortItem 	(Table t);
	static bool		GetColumnSortAscending 	(Table t);
	static void		SetColumnSortItem 	(Table t, int32 sortItem);
	static void		SetColumnSortAscending 	(Table t, bool sortAscending);

	static WORD		GetLanguageID() {return s_languageID;}
	static void		SetLanguageID(WORD new_id) {s_languageID = new_id;}	
	static uint8	CanSeeShares() {return s_iSeeShares;}

	static uint8	GetStatsMax()  {return s_statsMax;}
	static bool		UseFlatBar()  {return (s_depth3D==0);}
	static uint8	GetStatsAverageMinutes()  {return s_statsAverageMinutes;}
	static void		SetStatsAverageMinutes(uint8 in) {s_statsAverageMinutes=in;}

	static bool		GetNotifierPopOnImportantError() {return s_notifierImportantError;}

	static bool		GetStartMinimized() {return s_startMinimized;}
	static void		SetStartMinimized( bool instartMinimized) {s_startMinimized = instartMinimized;}
	static bool		GetSmartIdCheck()	 {return s_smartidcheck;}
	static void		SetSmartIdCheck( bool in_smartidcheck ) {s_smartidcheck = in_smartidcheck;}
	static uint8	GetSmartIdState() {return s_smartidstate;}
	static void		SetSmartIdState( uint8 in_smartidstate ) { s_smartidstate = in_smartidstate;}
	static bool		GetVerbose() {return s_bVerbose;}
	static bool		GetPreviewPrio() {return s_bpreviewprio;}
	static void		SetPreviewPrio(bool in) {s_bpreviewprio=in;}
	static bool		GetUpdateQueueList()  {return s_bupdatequeuelist;}
	static void		SetUpdateQueueList(bool new_state) {s_bupdatequeuelist = new_state;}
	static bool		TransferFullChunks()  {return s_btransferfullchunks;}
	static void		SetTransferFullChunks( bool m_bintransferfullchunks ) {s_btransferfullchunks = m_bintransferfullchunks;}
	static bool		StartNextFile() {return s_bstartnextfile;}
	static bool		ShowOverhead() {return s_bshowoverhead;}
	static void		SetNewAutoUp(bool m_bInUAP) {s_bUAP = m_bInUAP;}
	static bool		GetNewAutoUp() {return s_bUAP;}
	static void		SetNewAutoDown(bool m_bInDAP) {s_bDAP = m_bInDAP;}
	static bool		GetNewAutoDown() {return s_bDAP;}

	static const wxString&	GetVideoPlayer() { return s_VideoPlayer;}

	static uint32	GetFileBufferSize() {return s_iFileBufferSize*15000;}
	static uint32	GetQueueSize() {return s_iQueueSize*100;}

	// Barry
	static uint16	Get3DDepth() { return s_depth3D;}
	static bool		AddNewFilesPaused() {return s_addnewfilespaused;}

	static void		SetMaxConsPerFive(int in) {s_MaxConperFive=in;}

	static uint16	GetMaxConperFive() {return s_MaxConperFive;}
	static uint16	GetDefaultMaxConperFive();

	static bool		IsSafeServerConnectEnabled() {return s_safeServerConnect;}
	static bool		IsMoviePreviewBackup() {return s_moviePreviewBackup;}
	
	static bool		IsCheckDiskspaceEnabled() {return s_checkDiskspace != 0;}
	static uint32	GetMinFreeDiskSpace() {return s_uMinFreeDiskSpace;}

	static const wxString&	GetYourHostname() {return s_yourHostname;}

	// quick-speed changer [xrmb]
	static void		SetMaxUpload(uint16 in);
	static void		SetMaxDownload(uint16 in);
	static void		SetSlotAllocation(uint16 in) {s_slotallocation=in; };

	wxArrayString shareddir_list;
	wxArrayString adresses_list;

	static void 	SetLanguage();
	static bool 	AutoConnectStaticOnly() {return s_autoconnectstaticonly;}	
	void			LoadCats();
	static const wxString&	GetDateTimeFormat() { return s_datetimeformat;}
	// Download Categories (Ornis)
	int32			AddCat(Category_Struct* cat) { catMap.Add(cat); return catMap.GetCount()-1;}
	void			RemoveCat(size_t index);
	uint32			GetCatCount() { return catMap.GetCount();}
	Category_Struct* GetCategory(size_t index) { if (index<catMap.GetCount()) return catMap[index]; else return NULL;}

	const wxString&	GetCatPath(uint8 index) { return catMap[index]->incomingpath;}

	DWORD			GetCatColor(size_t index) { if (index<catMap.GetCount()) return catMap[index]->color; else return 0;}

	static uint32	GetAllcatType() { return s_allcatType;}
	static void		SetAllcatType(uint32 in) { s_allcatType=in; }
	static bool		ShowAllNotCats() { return s_showAllNotCats;}

	// WebServer
	static uint16	GetWSPort() { return s_nWebPort; }
	static void		SetWSPort(uint16 uPort) { s_nWebPort=uPort; }
	static const	wxString&	GetWSPass() { return s_sWebPassword; }
	static bool		GetWSIsEnabled() { return s_bWebEnabled; }
	static void		SetWSIsEnabled(bool bEnable) { s_bWebEnabled=bEnable; }
	static bool		GetWebUseGzip() { return s_bWebUseGzip; }
	static void		SetWebUseGzip(bool bUse) { s_bWebUseGzip=bUse; }
	static uint32	GetWebPageRefresh() { return s_nWebPageRefresh; }
	static void		SetWebPageRefresh(uint32 nRefresh) { s_nWebPageRefresh=nRefresh; }
	static bool		GetWSIsLowUserEnabled() { return s_bWebLowEnabled; }
	static void		SetWSIsLowUserEnabled(bool in) { s_bWebLowEnabled=in; }
	static const wxString&	GetWSLowPass() { return s_sWebLowPassword; }

	static void		SetMaxSourcesPerFile(uint16 in) { s_maxsourceperfile=in;}
	static void		SetMaxConnections(uint16 in) { s_maxconnections =in;}
	
	static bool		MsgOnlyFriends() { return s_msgonlyfriends;}
	static bool		MsgOnlySecure() { return s_msgsecure;}


	static uint32 	GetDesktopMode() {return s_desktopMode;}
	static void 	SetDesktopMode(uint32 mode) {s_desktopMode=mode;}

	static bool		ShowCatTabInfos() { return s_showCatTabInfos;}
	static void		ShowCatTabInfos(bool in) { s_showCatTabInfos=in;}
	
	// Madcat - Sources Dropping Tweaks
	static bool		DropNoNeededSources() { return s_NoNeededSources > 0; }
	static bool		SwapNoNeededSources() { return s_NoNeededSources == 2; }
	static bool		DropFullQueueSources() { return s_DropFullQueueSources; }
	static bool		DropHighQueueRankingSources() { return s_DropHighQueueRankingSources; }
	static uint32	HighQueueRanking() { return s_HighQueueRanking; }
	static uint32	GetAutoDropTimer() { return s_AutoDropTimer; }
	
	// Kry - External Connections
	static bool 	AcceptExternalConnections()		{ return s_AcceptExternalConnections; }
	static bool 	ECUseTCPPort()			{ return s_ECUseTCPPort; }
	static uint32 	ECPort()				{ return s_ECPort; }
	static const wxString&	ECPassword()		{ return s_ECPassword; }
	// Madcat - Fast ED2K Links Handler Toggling
	static bool 	GetFED2KLH()			{ return s_FastED2KLinksHandler; }

	// Kry - Ip filter On/Off
	static bool		GetIPFilterOn()			{ return s_IPFilterOn; }
	static void		SetIPFilterOn(bool val)		{ s_IPFilterOn = val; }
	static uint8	GetIPFilterLevel()			{ return s_filterlevel;}
	static void		SetIPFilterLevel(uint8 level)	{ s_filterlevel = level;}

	// Kry - Source seeds On/Off
	static bool		GetSrcSeedsOn() 			{ return s_UseSrcSeeds; }
	
	// Kry - Safe Max Connections
	static bool		GetSafeMaxConn()			{ return s_UseSafeMaxConn; }
	
	static bool		GetVerbosePacketError()		{ return s_VerbosePacketError; }
	
	static bool		IsSecureIdentEnabled()		{ return s_SecIdent; }
	
	static bool		GetExtractMetaData()		{ return s_ExtractMetaData; }
	
	static bool		ShowProgBar()			{ return s_ProgBar; }
	static bool		ShowPercent()			{ return s_Percent; }	
	
	static bool		GetAllocFullPart()			{ return s_AllocFullPart; };
	static bool		GetAllocFullChunk()			{ return s_AllocFullChunk; };

	static wxString GetBrowser();
	
	static const wxString& GetSkinFile()		{ return s_SkinFile; }
	
	static bool		UseSkin()				{ return s_UseSkinFile; }
	
	static const wxString& GetOSDir()		{ return s_OSDirectory; }

	static uint8	GetToolTipDelay()		{ return s_iToolDelayTime; }

	static void		CheckUlDlRatio();
	
	static void BuildItemList( const wxString& appdir );
	static void LoadAllItems(wxConfigBase* cfg);
	static void SaveAllItems(wxConfigBase* cfg);
	
	static int  GetPrefsID() { return s_ID; }
protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static int32 GetRecommendedMaxConnections();
	
	static void SetPrefsID(int ID)	{ s_ID = ID; }
	//! Contains the ID of the current window or zero if no preferences window has been created.
	static int s_ID;

	//! Temporary storage for statistic-colors.
	static COLORREF	s_colors[cntStatColors];
	//! Reference for checking if the colors has changed.
	static COLORREF	s_colors_ref[cntStatColors];
	 
	typedef std::list<Cfg_Base*>		CFGList;
	typedef std::map<int, Cfg_Base*>	CFGMap;

	static CFGMap	s_CfgList;
	static CFGList	s_MiscList;

private:
	Preferences_Ext_Struct* prefsExt;

	CMD4Hash m_userhash;
	WORD m_wWinVer;

	void LoadPreferences();
	void SavePreferences();

	ArrayOfCategory_Struct catMap;

	// deadlake PROXYSUPPORT
	bool m_UseProxyListenPort;
	uint16	ListenPort;

protected:
	// Preference vars:
	typedef uint16 Bool;	// an ugly way of appearing to be bool but being writeable to file as integer

////////////// USER
	static wxString	s_nick;

////////////// CONNECTION
	static uint16	s_maxupload;
	static uint16	s_maxdownload;
	static uint16	s_slotallocation;
	static uint16	s_port;
	static uint16	s_udpport;
	static bool		s_UDPDisable;
	static uint16	s_maxconnections;
	static bool		s_reconnect;
	static bool		s_autoconnect;
	static bool		s_autoconnectstaticonly;

////////////// SERVERS
	static bool		s_autoserverlist;
	static bool		s_deadserver;

////////////// FILES
	static wxString	s_incomingdir;
	static wxString	s_tempdir;
	static bool		s_ICH;
	
////////////// GUI
	static uint16	s_downloadColumnWidths[13];
	static Bool		s_downloadColumnHidden[13];
	static uint16	s_downloadColumnOrder[13];
	static uint16	s_uploadColumnWidths[11];
	static Bool		s_uploadColumnHidden[11];
	static uint16	s_uploadColumnOrder[11];
	static uint16	s_queueColumnWidths[11];
	static Bool		s_queueColumnHidden[11];
	static uint16	s_queueColumnOrder[11];
	static uint16	s_searchColumnWidths[5];
	static Bool		s_searchColumnHidden[5];
	static uint16	s_searchColumnOrder[5];
	static uint16	s_sharedColumnWidths[12];
	static Bool		s_sharedColumnHidden[12];
	static uint16	s_sharedColumnOrder[12];
	static uint16	s_serverColumnWidths[12];
	static Bool		s_serverColumnHidden[12];
	static uint16 	s_serverColumnOrder[12];
	static uint16	s_clientListColumnWidths[8];
	static Bool		s_clientListColumnHidden[8];
	static uint16 	s_clientListColumnOrder[8];

	static uint8	s_depth3D;
	
	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	static uint32	s_tableSortItemDownload;
	static uint32	s_tableSortItemUpload;
	static uint32	s_tableSortItemQueue;
	static uint32	s_tableSortItemSearch;
	static uint32	s_tableSortItemShared;
	static uint32	s_tableSortItemServer;
	static uint32	s_tableSortItemClientList;
	static bool		s_tableSortAscendingDownload;
	static bool		s_tableSortAscendingUpload;
	static bool		s_tableSortAscendingQueue;
	static bool		s_tableSortAscendingSearch;
	static bool		s_tableSortAscendingShared;
	static bool		s_tableSortAscendingServer;
	static bool		s_tableSortAscendingClientList;


	static bool		s_scorsystem;
	static bool		s_mintotray;
	static bool		s_addnewfilespaused;
	static bool		s_addserversfromserver;
	static bool		s_addserversfromclient;
	static uint16	s_maxsourceperfile;
	static uint16	s_trafficOMeterInterval;
	static uint16	s_statsInterval;
	static uint32	s_maxGraphDownloadRate;
	static uint32	s_maxGraphUploadRate;
	static bool		s_confirmExit;


	static bool		s_splashscreen;
	static bool		s_filterBadIP;
	static bool		s_onlineSig;

	static uint64  	s_totalDownloadedBytes;
	static uint64	s_totalUploadedBytes;
	static uint16	s_languageID;
	static bool		s_transferDoubleclick;
	static uint8	s_iSeeShares;		// 0=everybody 1=friends only 2=noone
	static uint8	s_iToolDelayTime;	// tooltip delay time in seconds
	static uint8	s_splitterbarPosition;
	static uint16	s_deadserverretries;
	static uint32	s_dwServerKeepAliveTimeoutMins;

	static uint8	s_statsMax;
	static uint8	s_statsAverageMinutes;

	static bool		s_useDownloadNotifier;
	static bool		s_useChatNotifier;
	static bool		s_useLogNotifier;	
	static bool		s_useSoundInNotifier;
	static bool		s_sendEmailNotifier;
	static bool		s_notifierPopsEveryChatMsg;
	static bool		s_notifierImportantError;
	static bool		s_notifierNewVersion;
	static wxString	s_notifierSoundFilePath;

	static bool		s_bpreviewprio;
	static bool		s_smartidcheck;
	static uint8	s_smartidstate;
	static bool		s_safeServerConnect;
	static bool		s_startMinimized;
	static uint16	s_MaxConperFive;
	static bool		s_checkDiskspace;
	static uint32 	s_uMinFreeDiskSpace;
	static wxString	s_yourHostname;
	static bool		s_bVerbose;
	static bool		s_bupdatequeuelist;
	static bool		s_bmanualhighprio;
	static bool		s_btransferfullchunks;
	static bool		s_bstartnextfile;
	static bool		s_bshowoverhead;
	static bool		s_bDAP;
	static bool		s_bUAP;
	static bool		s_bDisableKnownClientList;
	static bool		s_bDisableQueueList;


	static bool		s_showRatesInTitle;

	static wxString	s_VideoPlayer;
	static bool		s_moviePreviewBackup;
	static bool		s_indicateratings;
	static bool		s_showAllNotCats;
	static bool		s_filterserverbyip;
	static bool		s_bFirstStart;

	static bool		s_msgonlyfriends;
	static bool		s_msgsecure;

	static uint8	s_iFileBufferSize;
	static uint8	s_iQueueSize;

	static uint16	s_maxmsgsessions;
	static wxString	s_datetimeformat;

	// Web Server [kuchin]
	static wxString	s_sWebPassword;
	static wxString	s_sWebLowPassword;
	static uint16	s_nWebPort;
	static bool	s_bWebEnabled;
	static bool	s_bWebUseGzip;
	static uint32	s_nWebPageRefresh;
	static bool	s_bWebLowEnabled;
	static wxString	s_sWebResDir;

	static wxString	s_sTemplateFile;
	static bool	s_bIsASCWOP;

	static bool	s_showCatTabInfos;
	static bool	s_resumeSameCat;
	static bool	s_dontRecreateGraphs;
	static uint32	s_allcatType;
	
	static uint32 	s_desktopMode;
	
	// Madcat - Sources Dropping Tweaks
	static uint8	s_NoNeededSources; // 0: Keep, 1: Drop, 2:Swap
	static bool	s_DropFullQueueSources;
	static bool	s_DropHighQueueRankingSources;
	static uint32	s_HighQueueRanking;
	static uint32	s_AutoDropTimer;
	
	// Kry - external connections
	static bool 	s_AcceptExternalConnections;
	static bool 	s_ECUseTCPPort;
	static uint32	s_ECPort;
	static wxString	s_ECPassword;
	
	// Kry - IPFilter On/Off
	static bool	s_IPFilterOn;
	static uint8	s_filterlevel;
	
	// Kry - Source seeds on/off
	static bool	s_UseSrcSeeds;
	
	// Kry - Safe Max Connections
	static bool	s_UseSafeMaxConn;
	
	static bool	s_VerbosePacketError;

	static bool	s_ProgBar;
	static bool	s_Percent;	
	
	static bool	s_SecIdent;
	
	static bool	s_ExtractMetaData;
	
	static bool	s_AllocFullPart;
	static bool	s_AllocFullChunk;
	
	static uint16	s_Browser;
	static wxString	s_CustomBrowser;
	static bool	s_BrowserTab;     // Jacobo221 - Open in tabs if possible
	
	static wxString	s_OSDirectory;
	
	static wxString	s_SkinFile;
	
	static bool	s_UseSkinFile;
	
	static bool	s_FastED2KLinksHandler;	// Madcat - Toggle Fast ED2K Links Handler
};

#endif // PREFERENCES_H

