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


#include <cstdio>
#include <cstdlib>

#include <wx/timer.h>
#include <wx/valgen.h>
#include <wx/tokenzr.h>
#include <wx/control.h>
#include <wx/slider.h>
#include <wx/filename.h>
#include <wx/textfile.h>
#include <wx/config.h>

#ifdef __WXMSW__
	#include <wx/msw/winundef.h>
#endif

#include "amule.h"
#include "config.h"		// Needed for PACKAGE_NAME and PACKAGE_STRING
#include "otherfunctions.h"
#include "opcodes.h"		// Needed for PREFFILE_VERSION
#include "Preferences.h"
#include "CFile.h"
#include "muuli_wdr.h"
#include "StatisticsDlg.h"
#include "MD5Sum.h"


// Static variables
COLORREF		CPreferences::s_colors[cntStatColors];
COLORREF		CPreferences::s_colors_ref[cntStatColors];

CPreferences::CFGMap	CPreferences::s_CfgList;
CPreferences::CFGList	CPreferences::s_MiscList;


wxString	CPreferences::s_nick;
uint16		CPreferences::s_maxupload;
uint16		CPreferences::s_maxdownload;
uint16		CPreferences::s_slotallocation;
uint16		CPreferences::s_port;
uint16		CPreferences::s_udpport;
bool		CPreferences::s_UDPDisable;
uint16		CPreferences::s_maxconnections;
bool		CPreferences::s_reconnect;
bool		CPreferences::s_autoconnect;
bool		CPreferences::s_autoconnectstaticonly;
bool		CPreferences::s_autoserverlist;
bool		CPreferences::s_deadserver;
wxString	CPreferences::s_incomingdir;
wxString	CPreferences::s_tempdir;
bool		CPreferences::s_ICH;
uint16		CPreferences::s_downloadColumnWidths[13];
uint16		CPreferences::s_downloadColumnHidden[13];
uint16		CPreferences::s_downloadColumnOrder[13];
uint16		CPreferences::s_uploadColumnWidths[11];
uint16		CPreferences::s_uploadColumnHidden[11];
uint16		CPreferences::s_uploadColumnOrder[11];
uint16		CPreferences::s_queueColumnWidths[11];
uint16		CPreferences::s_queueColumnHidden[11];
uint16		CPreferences::s_queueColumnOrder[11];
uint16		CPreferences::s_searchColumnWidths[5];
uint16		CPreferences::s_searchColumnHidden[5];
uint16		CPreferences::s_searchColumnOrder[5];
uint16		CPreferences::s_sharedColumnWidths[12];
uint16		CPreferences::s_sharedColumnHidden[12];
uint16		CPreferences::s_sharedColumnOrder[12];
uint16		CPreferences::s_serverColumnWidths[12];
uint16		CPreferences::s_serverColumnHidden[12];
uint16 		CPreferences::s_serverColumnOrder[12];
uint16		CPreferences::s_clientListColumnWidths[8];
uint16		CPreferences::s_clientListColumnHidden[8];
uint16 		CPreferences::s_clientListColumnOrder[8];
uint8		CPreferences::s_depth3D;
uint32		CPreferences::s_tableSortItemDownload;
uint32		CPreferences::s_tableSortItemUpload;
uint32		CPreferences::s_tableSortItemQueue;
uint32		CPreferences::s_tableSortItemSearch;
uint32		CPreferences::s_tableSortItemShared;
uint32		CPreferences::s_tableSortItemServer;
uint32		CPreferences::s_tableSortItemClientList;
bool		CPreferences::s_tableSortAscendingDownload;
bool		CPreferences::s_tableSortAscendingUpload;
bool		CPreferences::s_tableSortAscendingQueue;
bool		CPreferences::s_tableSortAscendingSearch;
bool		CPreferences::s_tableSortAscendingShared;
bool		CPreferences::s_tableSortAscendingServer;
bool		CPreferences::s_tableSortAscendingClientList;
bool		CPreferences::s_scorsystem;
bool		CPreferences::s_mintotray;
bool		CPreferences::s_addnewfilespaused;
bool		CPreferences::s_addserversfromserver;
bool		CPreferences::s_addserversfromclient;
uint16		CPreferences::s_maxsourceperfile;
uint16		CPreferences::s_trafficOMeterInterval;
uint16		CPreferences::s_statsInterval;
uint32		CPreferences::s_maxGraphDownloadRate;
uint32		CPreferences::s_maxGraphUploadRate;
bool		CPreferences::s_confirmExit;
bool		CPreferences::s_splashscreen;
bool		CPreferences::s_filterBadIP;
bool		CPreferences::s_onlineSig;
uint64		CPreferences::s_totalDownloadedBytes;
uint64		CPreferences::s_totalUploadedBytes;
uint16		CPreferences::s_languageID;
bool		CPreferences::s_transferDoubleclick;
uint8		CPreferences::s_iSeeShares;
uint8		CPreferences::s_iToolDelayTime;
uint8		CPreferences::s_splitterbarPosition;
uint16		CPreferences::s_deadserverretries;
uint32		CPreferences::s_dwServerKeepAliveTimeoutMins;
uint8		CPreferences::s_statsMax;
uint8		CPreferences::s_statsAverageMinutes;
bool		CPreferences::s_useDownloadNotifier;
bool		CPreferences::s_useChatNotifier;
bool		CPreferences::s_useLogNotifier;	
bool		CPreferences::s_useSoundInNotifier;
bool		CPreferences::s_sendEmailNotifier;
bool		CPreferences::s_notifierPopsEveryChatMsg;
bool		CPreferences::s_notifierImportantError;
bool		CPreferences::s_notifierNewVersion;
wxString	CPreferences::s_notifierSoundFilePath;
bool		CPreferences::s_bpreviewprio;
bool		CPreferences::s_smartidcheck;
uint8		CPreferences::s_smartidstate;
bool		CPreferences::s_safeServerConnect;
bool		CPreferences::s_startMinimized;
uint16		CPreferences::s_MaxConperFive;
bool		CPreferences::s_checkDiskspace;
uint32		CPreferences::s_uMinFreeDiskSpace;
wxString	CPreferences::s_yourHostname;
bool		CPreferences::s_bVerbose;
bool		CPreferences::s_bupdatequeuelist;
bool		CPreferences::s_bmanualhighprio;
bool		CPreferences::s_btransferfullchunks;
bool		CPreferences::s_bstartnextfile;
bool		CPreferences::s_bshowoverhead;
bool		CPreferences::s_bDAP;
bool		CPreferences::s_bUAP;
bool		CPreferences::s_bDisableKnownClientList;
bool		CPreferences::s_bDisableQueueList;
bool		CPreferences::s_showRatesInTitle;
wxString	CPreferences::s_VideoPlayer;
bool		CPreferences::s_moviePreviewBackup;
bool		CPreferences::s_indicateratings;
bool		CPreferences::s_showAllNotCats;
bool		CPreferences::s_filterserverbyip;
bool		CPreferences::s_bFirstStart;
bool		CPreferences::s_msgonlyfriends;
bool		CPreferences::s_msgsecure;
uint8		CPreferences::s_filterlevel;
uint8		CPreferences::s_iFileBufferSize;
uint8		CPreferences::s_iQueueSize;
uint16		CPreferences::s_maxmsgsessions;
wxString 	CPreferences::s_datetimeformat;
wxString	CPreferences::s_sWebPassword;
wxString	CPreferences::s_sWebLowPassword;
uint16		CPreferences::s_nWebPort;
bool		CPreferences::s_bWebEnabled;
bool		CPreferences::s_bWebUseGzip;
uint32		CPreferences::s_nWebPageRefresh;
bool		CPreferences::s_bWebLowEnabled;
wxString	CPreferences::s_sWebResDir;
wxString	CPreferences::s_sTemplateFile;
bool		CPreferences::s_bIsASCWOP;
bool		CPreferences::s_showCatTabInfos;
bool		CPreferences::s_resumeSameCat;
bool		CPreferences::s_dontRecreateGraphs;
uint32		CPreferences::s_allcatType;
uint32		CPreferences::s_desktopMode;
uint8		CPreferences::s_NoNeededSources;
bool		CPreferences::s_DropFullQueueSources;
bool		CPreferences::s_DropHighQueueRankingSources;
uint32		CPreferences::s_HighQueueRanking;
uint32		CPreferences::s_AutoDropTimer;
bool 		CPreferences::s_AcceptExternalConnections;
bool 		CPreferences::s_ECUseTCPPort;
uint32		CPreferences::s_ECPort;
wxString	CPreferences::s_ECPassword;
bool		CPreferences::s_IPFilterOn;
bool		CPreferences::s_UseSrcSeeds;
bool		CPreferences::s_UseSafeMaxConn;
bool		CPreferences::s_VerbosePacketError;
bool		CPreferences::s_ProgBar;
bool		CPreferences::s_Percent;	
bool		CPreferences::s_SecIdent;
bool		CPreferences::s_ExtractMetaData;
bool		CPreferences::s_AllocFullPart;
bool		CPreferences::s_AllocFullChunk;
uint16		CPreferences::s_Browser;
wxString	CPreferences::s_CustomBrowser;
bool		CPreferences::s_BrowserTab;
wxString	CPreferences::s_OSDirectory;
wxString	CPreferences::s_SkinFile;
bool		CPreferences::s_UseSkinFile;
bool		CPreferences::s_FastED2KLinksHandler;




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

			if ( validator )
				return validator->TransferToWindow();
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
	 * Constructor.
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
	Cfg_Str_Encrypted( const wxString& keyname, wxString& value, const wxString& defaultVal = wxT("") )
	 : Cfg_Str( keyname, value, defaultVal )
	{}

#ifndef AMULE_DAEMON
	virtual bool TransferFromWindow()
	{
		// Store current value to see if it has changed
		wxString temp;
	
		// Shakraw: when storing value, store it encrypted here (only if changed in prefs)
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
#endif
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
		cfg->Write( GetKey(), (long)m_real_value );
	}


#ifndef AMULE_DAEMON
	virtual bool TransferFromWindow()
	{
		if ( Cfg_Tmpl<int>::TransferFromWindow() ) { 
			m_real_value = (TYPE)m_temp_value;

			return true;
		} 
		
		return false;
	}

	virtual bool TransferToWindow()
	{
		m_temp_value = (int)m_real_value;

		if ( Cfg_Tmpl<int>::TransferToWindow() ) {
			
			// In order to let us update labels on slider-changes, we trigger a event
   			if ( m_widget->IsKindOf(CLASSINFO(wxSlider)) ) {
				int id = m_widget->GetId();
				int pos = ((wxSlider*)m_widget)->GetValue();
			
				wxScrollEvent evt( wxEVT_SCROLL_THUMBRELEASE, id, pos );

				m_widget->ProcessEvent( evt );
			}

			return true;
		}
		
		return false;
	}
#endif

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
			while ( tokenizer.HasMoreTokens() && ( counter < m_count ) ) {
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



/// new implementation
CPreferences::CPreferences()
{
	srand( wxGetLocalTimeMillis().GetLo() ); // we need random numbers sometimes

	CreateUserHash();

	// load preferences.dat or set standart values
	wxString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	CFile preffile;
	if ( wxFileExists( fullpath ) ) {
		if ( preffile.Open(fullpath, CFile::read) ) {
			Preferences_Ext_Struct prefsExt;
	
			memset( &prefsExt, 0, sizeof(Preferences_Ext_Struct) );
			off_t read = preffile.Read( &prefsExt, sizeof(Preferences_Ext_Struct) );

			if ( read == sizeof(Preferences_Ext_Struct) ) {
				md4cpy(m_userhash, prefsExt.userhash);
			} else {
				SetStandartValues();
			}
			
			preffile.Close();	
		} else {
			SetStandartValues();
		}
	} else {
		SetStandartValues();
	}
	
	LoadPreferences();

	// shared directories
	wxTextFile sdirfile(theApp.ConfigDir + wxT("shareddir.dat"));
	if( sdirfile.Exists() && sdirfile.Open() ) {
		if (sdirfile.GetLineCount()) {
			shareddir_list.Add(sdirfile.GetFirstLine());				
			while (!sdirfile.Eof()) {
				shareddir_list.Add(sdirfile.GetNextLine());
			}
		}
		sdirfile.Close();
	}

	// serverlist adresses
	wxTextFile slistfile(theApp.ConfigDir + wxT("addresses.dat"));
	if ( slistfile.Exists() && slistfile.Open()) {
		if (slistfile.GetLineCount()) {
			for (wxString str = slistfile.GetFirstLine(); !slistfile.Eof(); str = slistfile.GetNextLine() ) {
    				adresses_list.Add(str);
			}
		}
		slistfile.Close();
	}

	m_userhash[5] = 14;
	m_userhash[14] = 111;

	if (!::wxDirExists(GetIncomingDir())) {
		::wxMkdir(GetIncomingDir(),0750);
	}

	if (!::wxDirExists(GetTempDir())) {
		::wxMkdir(GetTempDir(),0750);
	}

	if (m_userhash.IsEmpty()) {
		CreateUserHash();
	}
	
	printf("Userhash loaded: %s\n", unicode2char(m_userhash.Encode()));
}


void CPreferences::BuildItemList( const wxString& appdir )  // gets called at init time
{
	/**
	 * User settings
	 **/
	s_CfgList[IDC_NICK]		= new Cfg_Str(  wxT("/eMule/Nick"), s_nick, wxT("http://www.aMule.org") );
	s_CfgList[IDC_LANGUAGE]		= MkCfg_Int( wxT("/eMule/Language"), s_languageID, 0 );


	/**
	 * Misc
	 **/
	s_CfgList[IDC_FCHECK]		=    MkCfg_Int( wxT("/FakeCheck/Browser"), s_Browser, 0 );
	s_CfgList[IDC_FCHECKTABS]	= new Cfg_Bool( wxT("/FakeCheck/BrowserTab"), s_BrowserTab, true );
	s_CfgList[IDC_FCHECKSELF]	= new Cfg_Str(  wxT("/FakeCheck/CustomBrowser"), s_CustomBrowser, wxEmptyString );
	s_CfgList[IDC_QUEUESIZE]	= MkCfg_Int( wxT("/eMule/QueueSizePref"), s_iQueueSize, 50 );


	/**
	 * Debugging
	 **/
	s_CfgList[IDC_VERBOSE]			= new Cfg_Bool( wxT("/eMule/Verbose"), s_bVerbose, false );
	s_CfgList[IDC_VERBOSEPACKETERROR]	= new Cfg_Bool( wxT("/FakeCheck/VerbosePacketError"), s_VerbosePacketError, false );


	/**
	 * Connection settings
	 **/
	s_CfgList[IDC_MAXUP]			=    MkCfg_Int( wxT("/eMule/MaxUpload"), s_maxupload, 0 );
	s_CfgList[IDC_MAXDOWN]			=    MkCfg_Int( wxT("/eMule/MaxDownload"), s_maxdownload, 0 );
	s_CfgList[IDC_SLOTALLOC]		=    MkCfg_Int( wxT("/eMule/SlotAllocation"), s_slotallocation, 2 );
	s_CfgList[IDC_PORT]				=    MkCfg_Int( wxT("/eMule/Port"), s_port, 4662 );
	s_CfgList[IDC_UDPPORT]			=    MkCfg_Int( wxT("/eMule/UDPPort"), s_udpport, 4672 );
	s_CfgList[IDC_UDPDISABLE]		= new Cfg_Bool( wxT("/eMule/UDPDisable"), s_UDPDisable, false );
	s_CfgList[IDC_AUTOCONNECT]		= new Cfg_Bool( wxT("/eMule/Autoconnect"), s_autoconnect, true );
	s_CfgList[IDC_MAXSOURCEPERFILE]	=    MkCfg_Int( wxT("/eMule/MaxSourcesPerFile"), s_maxsourceperfile, 300 );
	s_CfgList[IDC_MAXCON]			=    MkCfg_Int( wxT("/eMule/MaxConnections"), s_maxconnections, GetRecommendedMaxConnections() );
	s_CfgList[IDC_MAXCON5SEC]		=    MkCfg_Int( wxT("/eMule/MaxConnectionsPerFiveSeconds"), s_MaxConperFive, 20 );
	s_CfgList[IDC_SAFEMAXCONN]		= new Cfg_Bool( wxT("/FakeCheck/SafeMaxConn"), s_UseSafeMaxConn, false );


	/**
	 * Servers
	 **/ 
	s_CfgList[IDC_REMOVEDEAD]		= new Cfg_Bool( wxT("/eMule/RemoveDeadServer"), s_deadserver, 1 );
	s_CfgList[IDC_SERVERRETRIES]	=    MkCfg_Int( wxT("/eMule/DeadServerRetry"), s_deadserverretries, 2 );
	s_CfgList[IDC_SERVERKEEPALIVE]	=    MkCfg_Int( wxT("/eMule/ServerKeepAliveTimeout"), s_dwServerKeepAliveTimeoutMins, 0 );
	s_CfgList[IDC_RECONN]			= new Cfg_Bool( wxT("/eMule/Reconnect"), s_reconnect, true );
	s_CfgList[IDC_SCORE]			= new Cfg_Bool( wxT("/eMule/Scoresystem"), s_scorsystem, true );
	s_CfgList[IDC_AUTOSERVER]		= new Cfg_Bool( wxT("/eMule/Serverlist"), s_autoserverlist, false );
	s_CfgList[IDC_UPDATESERVERCONNECT]	= new Cfg_Bool( wxT("/eMule/AddServersFromServer"), s_addserversfromserver, true);
	s_CfgList[IDC_UPDATESERVERCLIENT]	= new Cfg_Bool( wxT("/eMule/AddServersFromClient"), s_addserversfromclient, true );
	s_CfgList[IDC_SAFESERVERCONNECT]	 = new Cfg_Bool( wxT("/eMule/SafeServerConnect"), s_safeServerConnect, false );
	s_CfgList[IDC_AUTOCONNECTSTATICONLY] = new Cfg_Bool( wxT("/eMule/AutoConnectStaticOnly"), s_autoconnectstaticonly, false ); 
	s_CfgList[IDC_SMARTIDCHECK]		= new Cfg_Bool( wxT("/eMule/SmartIdCheck"), s_smartidcheck, true );


	/**
	 * Files
	 **/
	s_CfgList[IDC_TEMPFILES]		= new Cfg_Str(  wxT("/eMule/TempDir"), s_tempdir,			appdir + wxT("Temp") );
	s_CfgList[IDC_INCFILES]			= new Cfg_Str(  wxT("/eMule/IncomingDir"), s_incomingdir,	appdir + wxT("Incoming") );
	s_CfgList[IDC_ICH]				= new Cfg_Bool( wxT("/eMule/ICH"), s_ICH, true );
	s_CfgList[IDC_METADATA] 		= new Cfg_Bool( wxT("/ExternalConnect/ExtractMetaDataTags"), s_ExtractMetaData, false );
	s_CfgList[IDC_CHUNKALLOC]		= new Cfg_Bool( wxT("/ExternalConnect/FullChunkAlloc"), s_AllocFullChunk, false );
	s_CfgList[IDC_FULLALLOCATE]		= new Cfg_Bool( wxT("/ExternalConnect/FullPartAlloc"), s_AllocFullPart, false );
	s_CfgList[IDC_CHECKDISKSPACE]	= new Cfg_Bool( wxT("/eMule/CheckDiskspace"), s_checkDiskspace, true );
	s_CfgList[IDC_MINDISKSPACE]		=    MkCfg_Int( wxT("/eMule/MinFreeDiskSpace"), s_uMinFreeDiskSpace, 1 );
	s_CfgList[IDC_ADDNEWFILESPAUSED]	= new Cfg_Bool( wxT("/eMule/AddNewFilesPaused"), s_addnewfilespaused, false );
	s_CfgList[IDC_PREVIEWPRIO]		= new Cfg_Bool( wxT("/eMule/PreviewPrio"), s_bpreviewprio, false );
	s_CfgList[IDC_MANUALSERVERHIGHPRIO]	= new Cfg_Bool( wxT("/eMule/ManualHighPrio"), s_bmanualhighprio, false );
	s_CfgList[IDC_FULLCHUNKTRANS] 	= new Cfg_Bool( wxT("/eMule/FullChunkTransfers"), s_btransferfullchunks, true );
	s_CfgList[IDC_STARTNEXTFILE]	= new Cfg_Bool( wxT("/eMule/StartNextFile"), s_bstartnextfile, false );
	s_CfgList[IDC_FILEBUFFERSIZE]	=    MkCfg_Int( wxT("/eMule/FileBufferSizePref"), s_iFileBufferSize, 16 );
	s_CfgList[IDC_DAP]				= new Cfg_Bool( wxT("/eMule/DAPPref"), s_bDAP, true );
	s_CfgList[IDC_UAP]				= new Cfg_Bool( wxT("/eMule/UAPPref"), s_bUAP, true );


	/**
	 * External Connections
	 */
	s_CfgList[IDC_OSDIR]			= new Cfg_Str(  wxT("/eMule/OSDirectory"), s_OSDirectory,	appdir );
	s_CfgList[IDC_ONLINESIG]		= new Cfg_Bool( wxT("/eMule/OnlineSignature"), s_onlineSig, false );
	s_CfgList[IDC_ENABLE_WEB]		= new Cfg_Bool( wxT("/WebServer/Enabled"), s_bWebEnabled, false );
	s_CfgList[IDC_WEB_PASSWD]		= new Cfg_Str_Encrypted( wxT("/WebServer/Password"), s_sWebPassword );
	s_CfgList[IDC_WEB_PASSWD_LOW]	= new Cfg_Str_Encrypted( wxT("/WebServer/PasswordLow"), s_sWebLowPassword );
	s_CfgList[IDC_WEB_PORT]			=    MkCfg_Int( wxT("/WebServer/Port"), s_nWebPort, 4711 );
	s_CfgList[IDC_WEB_GZIP]			= new Cfg_Bool( wxT("/WebServer/UseGzip"), s_bWebUseGzip, true );
	s_CfgList[IDC_ENABLE_WEB_LOW]	= new Cfg_Bool( wxT("/WebServer/UseLowRightsUser"), s_bWebLowEnabled, false );
	s_CfgList[IDC_WEB_REFRESH_TIMEOUT]	=    MkCfg_Int( wxT("/WebServer/PageRefreshTime"), s_nWebPageRefresh, 120 );
	s_CfgList[IDC_EXT_CONN_ACCEPT]	= new Cfg_Bool( wxT("/ExternalConnect/AcceptExternalConnections"), s_AcceptExternalConnections, true );
	s_CfgList[IDC_EXT_CONN_USETCP]	= new Cfg_Bool( wxT("/ExternalConnect/ECUseTCPPort"), s_ECUseTCPPort, false );
	s_CfgList[IDC_EXT_CONN_TCP_PORT]	=    MkCfg_Int( wxT("/ExternalConnect/ECPort"), s_ECPort, 4712 );
	s_CfgList[IDC_EXT_CONN_PASSWD]	= new Cfg_Str_Encrypted( wxT("/ExternalConnect/ECPassword"), s_ECPassword, wxEmptyString );


	/**
	 * GUI behavoir
	 **/
	s_CfgList[IDC_SPLASHON]			= new Cfg_Bool( wxT("/eMule/Splashscreen"), s_splashscreen, true );
	s_CfgList[IDC_MINTRAY]			= new Cfg_Bool( wxT("/eMule/MinToTray"), s_mintotray, false );
	s_CfgList[IDC_EXIT]				= new Cfg_Bool( wxT("/eMule/ConfirmExit"), s_confirmExit, false );
	s_CfgList[IDC_DBLCLICK]			= new Cfg_Bool( wxT("/eMule/TransferDoubleClick"), s_transferDoubleclick, true );
	s_CfgList[IDC_STARTMIN]			= new Cfg_Bool( wxT("/eMule/StartupMinimized"), s_startMinimized, false );


	/**
	 * GUI appearence
	 **/
	s_CfgList[IDC_3DDEPTH]			=    MkCfg_Int( wxT("/eMule/3DDepth"), s_depth3D, 10 );
	s_CfgList[IDC_TOOLTIPDELAY]		=    MkCfg_Int( wxT("/eMule/ToolTipDelay"), s_iToolDelayTime, 1 );
	s_CfgList[IDC_SHOWOVERHEAD]		= new Cfg_Bool( wxT("/eMule/ShowOverhead"), s_bshowoverhead, false );
	s_CfgList[IDC_EXTCATINFO]		= new Cfg_Bool( wxT("/eMule/ShowInfoOnCatTabs"), s_showCatTabInfos, false );
	s_CfgList[IDC_FED2KLH]			= new Cfg_Bool( wxT("/Razor_Preferences/FastED2KLinksHandler"), s_FastED2KLinksHandler, true );
	s_CfgList[IDC_PROGBAR]			= new Cfg_Bool( wxT("/ExternalConnect/ShowProgressBar"),	s_ProgBar, true );
	s_CfgList[IDC_PERCENT]			= new Cfg_Bool( wxT("/ExternalConnect/ShowPercent"), 		s_Percent, false );

	s_CfgList[IDC_USESKIN]			= new Cfg_Bool( wxT("/SkinGUIOptions/UseSkinFile"), s_UseSkinFile, false );
	s_CfgList[IDC_SKINFILE]			= new Cfg_Str(  wxT("/SkinGUIOptions/SkinFile"), s_SkinFile, wxEmptyString );


	/**
	 * External Apps
	 */
	s_CfgList[IDC_VIDEOPLAYER]		= new Cfg_Str(  wxT("/eMule/VideoPlayer"), s_VideoPlayer, wxEmptyString );
	s_CfgList[IDC_VIDEOBACKUP]		= new Cfg_Bool( wxT("/eMule/VideoPreviewBackupped"), s_moviePreviewBackup, true );


	/**
	 * Statistics
	 **/
	s_CfgList[IDC_SLIDER]			=    MkCfg_Int( wxT("/eMule/StatGraphsInterval"), s_trafficOMeterInterval, 3 );
	s_CfgList[IDC_SLIDER2]			=    MkCfg_Int( wxT("/eMule/statsInterval"), s_statsInterval, 30 );
	s_CfgList[IDC_DOWNLOAD_CAP]		=    MkCfg_Int( wxT("/eMule/DownloadCapacity"), s_maxGraphDownloadRate, 3 );
	s_CfgList[IDC_UPLOAD_CAP]		=    MkCfg_Int( wxT("/eMule/UploadCapacity"), s_maxGraphUploadRate, 3 );
	s_CfgList[IDC_SLIDER3] 			=    MkCfg_Int( wxT("/eMule/StatsAverageMinutes"), s_statsAverageMinutes, 5 );
	s_CfgList[IDC_SLIDER4]			=    MkCfg_Int( wxT("/eMule/VariousStatisticsMaxValue"), s_statsMax, 100 );


	/**
	 * Sources
	 **/
	s_CfgList[IDC_ENABLE_AUTO_FQS]	= new Cfg_Bool( wxT("/Razor_Preferences/FullQueueSources"), s_DropFullQueueSources,  false );
	s_CfgList[IDC_ENABLE_AUTO_HQRS]	= new Cfg_Bool( wxT("/Razor_Preferences/HighQueueRankingSources"), s_DropHighQueueRankingSources, false );
	s_CfgList[IDC_HQR_VALUE]		=    MkCfg_Int( wxT("/Razor_Preferences/HighQueueRanking"), s_HighQueueRanking, 1200 );
	s_CfgList[IDC_AUTO_DROP_TIMER]	=    MkCfg_Int( wxT("/Razor_Preferences/AutoDropTimer"), s_AutoDropTimer, 240 );
	s_CfgList[IDC_NNS_HANDLING]		=    MkCfg_Int( wxT("/Razor_Preferences/NoNeededSourcesHandling"), s_NoNeededSources, 2 );
	s_CfgList[IDC_SRCSEEDS]			= new Cfg_Bool( wxT("/ExternalConnect/UseSrcSeeds"),		s_UseSrcSeeds, false );
	 

	/**
	 * Security
	 **/
	s_CfgList[IDC_SEESHARES]		=    MkCfg_Int( wxT("/eMule/SeeShare"),					s_iSeeShares, 2 );
	s_CfgList[IDC_SECIDENT]			= new Cfg_Bool( wxT("/ExternalConnect/UseSecIdent"),	s_SecIdent, true );
	s_CfgList[IDC_IPFONOFF]			= new Cfg_Bool( wxT("/ExternalConnect/IpFilterOn"),		s_IPFilterOn, true );
	s_CfgList[IDC_FILTER]			= new Cfg_Bool( wxT("/eMule/FilterBadIPs"),				s_filterBadIP, true );






	/**
	 * The folowing doesn't have an assosiated widget.
	 **/


	/* window colum widths, no dialog interaction - BEGIN */
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnWidths"),		s_downloadColumnWidths,	ELEMENT_COUNT(s_downloadColumnWidths),		DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnHidden"),		s_downloadColumnHidden,	ELEMENT_COUNT(s_downloadColumnHidden),		0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnOrder"),		s_downloadColumnOrder,	ELEMENT_COUNT(s_downloadColumnOrder),		0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnWidths"),		s_uploadColumnWidths,	ELEMENT_COUNT(s_uploadColumnWidths), 		DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnHidden"),		s_uploadColumnHidden,	ELEMENT_COUNT(s_uploadColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnOrder"),			s_uploadColumnOrder,		ELEMENT_COUNT(s_uploadColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnWidths"),			s_queueColumnWidths,		ELEMENT_COUNT(s_queueColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnHidden"),			s_queueColumnHidden,		ELEMENT_COUNT(s_queueColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnOrder"),			s_queueColumnOrder,		ELEMENT_COUNT(s_queueColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnWidths"),		s_searchColumnWidths,	ELEMENT_COUNT(s_searchColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnHidden"),		s_searchColumnHidden,	ELEMENT_COUNT(s_searchColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnOrder"),			s_searchColumnOrder,		ELEMENT_COUNT(s_searchColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnWidths"),		s_sharedColumnWidths,	ELEMENT_COUNT(s_sharedColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnHidden"),		s_sharedColumnHidden,	ELEMENT_COUNT(s_sharedColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnOrder"),			s_sharedColumnOrder,		ELEMENT_COUNT(s_sharedColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnWidths"),		s_serverColumnWidths,	ELEMENT_COUNT(s_serverColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnHidden"),		s_serverColumnHidden,	ELEMENT_COUNT(s_serverColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnOrder"),			s_serverColumnOrder,		ELEMENT_COUNT(s_serverColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnWidths"),	s_clientListColumnWidths, ELEMENT_COUNT(s_clientListColumnWidths),	DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnHidden"),	s_clientListColumnHidden, ELEMENT_COUNT(s_clientListColumnHidden), 	0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnOrder"),		s_clientListColumnOrder,	ELEMENT_COUNT(s_clientListColumnOrder),		0 ) );
	/*  window colum widths - END */

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemDownload"),		s_tableSortItemDownload,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemUpload"),			s_tableSortItemUpload,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemQueue"),			s_tableSortItemQueue,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemSearch"),			s_tableSortItemSearch,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemShared"),			s_tableSortItemShared,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemServer"),			s_tableSortItemServer,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemClientList"),		s_tableSortItemClientList,		0 ) );
	
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingDownload"),	s_tableSortAscendingDownload,	true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingUpload"),		s_tableSortAscendingUpload,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingQueue"),		s_tableSortAscendingQueue,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingSearch"),		s_tableSortAscendingSearch,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingShared"),		s_tableSortAscendingShared,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingServer"),		s_tableSortAscendingServer,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingClientList"),	s_tableSortAscendingClientList,	true ) );


	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalDownloadedBytes"), s_totalDownloadedBytes ) );
	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalUploadedBytes"),	s_totalUploadedBytes ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/SplitterbarPosition"),			s_splitterbarPosition, 75 ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/FilterServersByIP"),			s_filterserverbyip, false ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/FilterLevel"),					s_filterlevel, 127 ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/YourHostname"),					s_yourHostname, wxEmptyString ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/DateTimeFormat"),				s_datetimeformat, wxT("%A, %x, %X") ) );
	

	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/IndicateRatings"),				s_indicateratings, true ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/AllcatType"),					s_allcatType, 0 ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/ShowAllNotCats"),				s_showAllNotCats, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/ResumeNextFromSameCat"),		s_resumeSameCat, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DontRecreateStatGraphsOnResize"),	s_resumeSameCat, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DisableKnownClientList"),		s_bDisableKnownClientList, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DisableQueueList"),				s_bDisableQueueList, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/MessagesFromFriendsOnly"),		s_msgonlyfriends, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/MessageFromValidSourcesOnly"),	s_msgsecure, true ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/MaxMessageSessions"),			s_maxmsgsessions, 50 ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/WebTemplateFile"),				s_sTemplateFile, wxT("eMule.tmpl") ) );


	s_MiscList.push_back(   MkCfg_Int( wxT("/Statistics/DesktopMode"), s_desktopMode, 4 ) );

#ifndef AMULE_DAEMON
	// Colors have been moved from global prefs to CStatisticsDlg
	for ( int i = 0; i < cntStatColors; i++ ) {  
		wxString str = wxString::Format(wxT("/eMule/StatColor%i"),i);
		
		s_MiscList.push_back( MkCfg_Int( str, CStatisticsDlg::acrStat[i], CStatisticsDlg::acrStat[i] ) );
	}
#endif

// These options are currently not used 
#if 0
	////// Notify
	s_CfgList[IDC_CB_TBN_USESOUND]	= new Cfg_Bool( wxT("/eMule/NotifierUseSound"), s_useSoundInNotifier, false );
	s_CfgList[IDC_CB_TBN_ONLOG]		= new Cfg_Bool( wxT("/eMule/NotifyOnLog"), s_useLogNotifier, false );
	s_CfgList[IDC_CB_TBN_ONCHAT]	= new Cfg_Bool( wxT("/eMule/NotifyOnChat"), s_useChatNotifier, false );
	s_CfgList[IDC_CB_TBN_POP_ALWAYS]	= new Cfg_Bool( wxT("/eMule/NotifierPopEveryChatMessage"), s_notifierPopsEveryChatMsg, false );
	s_CfgList[IDC_CB_TBN_ONDOWNLOAD]	= new Cfg_Bool( wxT("/eMule/NotifyOnDownload"), s_useDownloadNotifier, false );
	s_CfgList[IDC_CB_TBN_ONNEWVERSION]	= new Cfg_Bool( wxT("/eMule/NotifierPopNewVersion"), s_notifierNewVersion, false );
	s_CfgList[IDC_CB_TBN_IMPORTATNT]	= new Cfg_Bool( wxT("/eMule/NotifyOnImportantError"), s_notifierImportantError, false );
	s_CfgList[IDC_SENDMAIL]			= new Cfg_Bool( wxT("/eMule/NotifyByMail"), s_sendEmailNotifier, false );
	s_CfgList[IDC_EDIT_TBN_WAVFILE]	= new Cfg_Str(  wxT("/eMule/NotifierSoundPath"), s_notifierSoundFilePath, wxEmptyString );
#endif
}


void CPreferences::LoadAllItems(wxConfigBase* cfg)
{
	// Connect the Cfgs with their widgets
	CFGMap::iterator it_a = s_CfgList.begin();
	for ( ; it_a != s_CfgList.end(); ++it_a ) {
		it_a->second->LoadFromFile( cfg );
	}

	CFGList::iterator it_b = s_MiscList.begin();
	for ( ; it_b != s_MiscList.end(); ++it_b ) {
		(*it_b)->LoadFromFile( cfg ); 
	}

	
	// Now do some post-processing / sanity checking on the values we just loaded
	CheckUlDlRatio();
}


void CPreferences::SaveAllItems(wxConfigBase* cfg)
{
	// Connect the Cfgs with their widgets
	CFGMap::iterator it_a = s_CfgList.begin();
	for ( ; it_a != s_CfgList.end(); ++it_a )
		it_a->second->SaveToFile( cfg );


	CFGList::iterator it_b = s_MiscList.begin();
	for ( ; it_b != s_MiscList.end(); ++it_b )
		(*it_b)->SaveToFile( cfg ); 
}

void CPreferences::SetMaxUpload(uint16 in)
{
	if ( s_maxupload != in ) {
		s_maxupload = in;

		// Ensure that the ratio is upheld
		CheckUlDlRatio();
	}
}


void CPreferences::SetMaxDownload(uint16 in)
{
	if ( s_maxdownload != in ) {
		s_maxdownload = in;

		// Ensure that the ratio is upheld
		CheckUlDlRatio();
	}
}


// Here we slightly limit the users' ability to be a bad citizen: for very low upload rates
// we force a low download rate, so as to discourage this type of leeching.  
// We're Open Source, and whoever wants it can do his own mod to get around this, but the 
// packaged product will try to enforce good behavior. 
//
// Kry note: of course, any leecher mod will be banned asap.
void CPreferences::CheckUlDlRatio()
{
	// Backwards compatibility
	if ( s_maxupload == 0xFFFF )	s_maxupload = 0;

	// Backwards compatibility
	if ( s_maxdownload == 0xFFFF )	s_maxdownload = 0;
		
	// Enforce the limits
	if ( s_maxupload < 4  ) {
		if ( ( s_maxupload * 3 < s_maxdownload ) || ( s_maxdownload == 0 ) )
			s_maxdownload = s_maxupload * 3 ;
	} else if ( s_maxupload < 10  ) {
		if ( ( s_maxupload * 4 < s_maxdownload ) || ( s_maxdownload == 0 ) )
			s_maxdownload = s_maxupload * 4;
	}
}


void CPreferences::SetStandartValues()
{
	CreateUserHash();
	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;
	defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;
	defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	Save();
}

bool CPreferences::Save()
{
	wxString fullpath(theApp.ConfigDir + wxT("preferences.dat"));

	bool error = false;

	CFile preffile;
	
	if ( !wxFileExists( fullpath ) )
		preffile.Create( fullpath );
	
	if ( preffile.Open(fullpath, CFile::read_write) ) {
		printf("Saving userhash: %s\n", unicode2char(m_userhash.Encode()));
		
		Preferences_Ext_Struct prefsExt;
		memset( &prefsExt, 0, sizeof(Preferences_Ext_Struct) );
		
		prefsExt.version = PREFFILE_VERSION;
		md4cpy( prefsExt.userhash, m_userhash.GetHash() );
		
		off_t read = preffile.Write( &prefsExt, sizeof(Preferences_Ext_Struct) );

		error = read != sizeof(Preferences_Ext_Struct);
		
		preffile.Close();
	} else {
		error = true;
	}

	SavePreferences();

	wxString shareddir(theApp.ConfigDir + wxT("shareddir.dat"));

	wxRemoveFile(shareddir);

	wxTextFile sdirfile(shareddir);

	if(sdirfile.Create()) {
		for(unsigned int ii = 0; ii < shareddir_list.GetCount(); ++ii) {
			sdirfile.AddLine(shareddir_list[ii]);
		}
		sdirfile.Write(),
		sdirfile.Close();
	} else {
		error = true;
	}

	return error;
}


void CPreferences::CreateUserHash()
{
	for (int i = 0;i != 8; i++) {
		uint16	random = rand();
		memcpy(&m_userhash[i*2],&random,2);
	}
	// mark as emule client. that will be need in later version
	m_userhash[5] = 14;
	m_userhash[14] = 111;
}


// Macros to check if the element is not out of bounds
// If you add new columns and this triggers, then you need to 
// increase the size of the arrays in question to the correct size.
#define CHECKANDRETURN(array, index) if ( index < ARRSIZE(array) ) { \
		return array[index]; \
	} else { \
		printf("Attempting to read past end of array at line %d in %s\n", __LINE__, __FILE__); \
		return 0; \
	}
#define CHECKANDSET(array, index, value) if ( index < ARRSIZE(array) ) { \
		array[index] = value; \
	} else { \
		printf("Attempting to write past end of array at line %d in %s\n", __LINE__, __FILE__); \
	}


int32 CPreferences::GetColumnWidth(TablePreference t, int index)
{
	switch(t) {
	case TP_Download:
		CHECKANDRETURN( s_downloadColumnWidths,   index );
	case TP_Upload:
		CHECKANDRETURN( s_uploadColumnWidths,     index );
	case TP_Queue:
		CHECKANDRETURN( s_queueColumnWidths,      index );
	case TP_Search:
		CHECKANDRETURN( s_searchColumnWidths,     index );
	case TP_Shared:
		CHECKANDRETURN( s_sharedColumnWidths,     index );
	case TP_Server:
		CHECKANDRETURN( s_serverColumnWidths,     index );
	case TP_ClientList:
		CHECKANDRETURN( s_clientListColumnWidths, index );
	case TP_None:
	default:
		return 0;
	}
}


void CPreferences::SetColumnWidth(TablePreference t, int index, int32 width)
{
	switch(t) {
	case TP_Download:
		CHECKANDSET( s_downloadColumnWidths,   index, width );
		break;
	case TP_Upload:
		CHECKANDSET( s_uploadColumnWidths,     index, width );
		break;
	case TP_Queue:
		CHECKANDSET( s_queueColumnWidths,      index, width );
		break;
	case TP_Search:
		CHECKANDSET( s_searchColumnWidths,     index, width );
		break;
	case TP_Shared:
		CHECKANDSET( s_sharedColumnWidths,     index, width );
		break;
	case TP_Server:
		CHECKANDSET( s_serverColumnWidths,     index, width );
		break;
	case TP_ClientList:
		CHECKANDSET( s_clientListColumnWidths, index, width );
		break;
	case TP_None:
	default:
		break;
	}
}


bool CPreferences::GetColumnHidden(TablePreference t, int index)
{
	switch(t) {
	case TP_Download:
		CHECKANDRETURN( s_downloadColumnHidden,   index );
	case TP_Upload:
		CHECKANDRETURN( s_uploadColumnHidden,     index );
	case TP_Queue:
		CHECKANDRETURN( s_queueColumnHidden,      index );
	case TP_Search:
		CHECKANDRETURN( s_searchColumnHidden,     index );
	case TP_Shared:
		CHECKANDRETURN( s_sharedColumnHidden,     index );
	case TP_Server:
		CHECKANDRETURN( s_serverColumnHidden,     index );
	case TP_ClientList:
		CHECKANDRETURN( s_clientListColumnHidden, index );
	case TP_None:
	default:
		return FALSE;
	}
}


void CPreferences::SetColumnHidden(TablePreference t, int index, bool bHidden)
{
	switch(t) {
	case TP_Download:
		CHECKANDSET( s_downloadColumnHidden,   index, bHidden );
		break;
	case TP_Upload:
		CHECKANDSET( s_uploadColumnHidden,     index, bHidden );
		break;
	case TP_Queue:
		CHECKANDSET( s_queueColumnHidden,      index, bHidden );
		break;
	case TP_Search:
		CHECKANDSET( s_searchColumnHidden,     index, bHidden );
		break;
	case TP_Shared:
		CHECKANDSET( s_sharedColumnHidden,     index, bHidden );
		break;
	case TP_Server:
		CHECKANDSET( s_serverColumnHidden,     index, bHidden );
		break;
	case TP_ClientList:
		CHECKANDSET( s_clientListColumnHidden, index, bHidden );
		break;
	case TP_None:
	default:
		break;
	}
}


int32 CPreferences::GetColumnOrder(TablePreference t, int index)
{
	switch(t) {
	case TP_Download:
		CHECKANDRETURN( s_downloadColumnOrder,   index );
	case TP_Upload:
		CHECKANDRETURN( s_uploadColumnOrder,     index );
	case TP_Queue:
		CHECKANDRETURN( s_queueColumnOrder,      index );
	case TP_Search:
		CHECKANDRETURN( s_searchColumnOrder,     index );
	case TP_Shared:
		CHECKANDRETURN( s_sharedColumnOrder,     index );
	case TP_Server:
		CHECKANDRETURN( s_serverColumnOrder,     index );
	case TP_ClientList:
		CHECKANDRETURN( s_clientListColumnOrder, index );
	case TP_None:
	default:
		return 0;
	}
}


void CPreferences::SetColumnOrder(TablePreference t, INT *piOrder)
{
	switch(t) {
	case TP_Download:
		memcpy(s_downloadColumnOrder, piOrder, sizeof(s_downloadColumnOrder));
		break;
	case TP_Upload:
		memcpy(s_uploadColumnOrder, piOrder, sizeof(s_uploadColumnOrder));
		break;
	case TP_Queue:
		memcpy(s_queueColumnOrder, piOrder, sizeof(s_queueColumnOrder));
		break;
	case TP_Search:
		memcpy(s_searchColumnOrder, piOrder, sizeof(s_searchColumnOrder));
		break;
	case TP_Shared:
		memcpy(s_sharedColumnOrder, piOrder, sizeof(s_sharedColumnOrder));
		break;
	case TP_Server:
		memcpy(s_serverColumnOrder, piOrder, sizeof(s_serverColumnOrder));
		break;
	case TP_ClientList:
		memcpy(s_clientListColumnOrder, piOrder, sizeof(s_clientListColumnOrder));
		break;
	case TP_None:
	default:
		break;
	}
}


CPreferences::~CPreferences()
{
	while ( !m_CatList.empty() ) {
		delete m_CatList.front();
		m_CatList.erase( m_CatList.begin() );
	}

	m_CatList.clear();
}


int32 CPreferences::GetRecommendedMaxConnections()
{
	int iRealMax = ::GetMaxConnections();
	if(iRealMax == -1 || iRealMax > 520) {
		return 500;
	}
	if(iRealMax < 20) {
		return iRealMax;
	}
	if(iRealMax <= 256) {
		return iRealMax - 10;
	}
	return iRealMax - 20;
}


void CPreferences::SavePreferences()
{
	wxConfigBase* cfg = wxConfig::Get();

	cfg->Write( wxT("/eMule/AppVersion"), wxT(PACKAGE_STRING) );

	// Ensure that the changes are saved to disk.
	cfg->Flush();
}


void CPreferences::SaveCats()
{
	if ( GetCatCount() ) {
		wxConfigBase* cfg = wxConfig::Get();

		// The first category is the default one and should not be counte

		cfg->Write( wxT("/General/Count"), (long)(m_CatList.size() - 1) );

		for ( size_t i = 1; i < m_CatList.size(); i++ ) {
			cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

			cfg->Write( wxT("Title"),		m_CatList[i]->title );
			cfg->Write( wxT("Incoming"),	m_CatList[i]->incomingpath );
			cfg->Write( wxT("Comment"),		m_CatList[i]->comment );
			cfg->Write( wxT("Color"),		wxString::Format(wxT("%u"), m_CatList[i]->color) );
			cfg->Write( wxT("Priority"),	m_CatList[i]->prio );
		}
	}
}


void CPreferences::LoadPreferences()
{
	LoadCats();
}


void CPreferences::LoadCats() {
	// default cat ... Meow! =(^.^)=
	Category_Struct* newcat=new Category_Struct;
	newcat->title = wxEmptyString;
	newcat->incomingpath = wxEmptyString;
	newcat->comment = wxEmptyString;
	newcat->prio=0;
	newcat->color=0;
	AddCat( newcat );

	wxConfigBase* cfg = wxConfig::Get();

	long max = cfg->Read( wxT("/General/Count"), 0l );

	for ( int i = 1; i <= max ; i++ ) {
		cfg->SetPath( wxString::Format(wxT("/Cat#%i"), i) );

		Category_Struct* newcat = new Category_Struct;


		newcat->title = cfg->Read( wxT("Title"), wxEmptyString );
		newcat->incomingpath = cfg->Read( wxT("Incoming"), wxEmptyString );

		newcat->incomingpath = MakeFoldername(newcat->incomingpath);
		newcat->comment = cfg->Read( wxT("Comment"), wxEmptyString );

		newcat->prio = cfg->Read( wxT("Priority"), 0l );

		wxString color = cfg->Read( wxT("Color"), wxT("0") );
		newcat->color = StrToULong(color);

		AddCat(newcat);
		if (!wxFileName::DirExists(newcat->incomingpath)) {
			wxFileName::Mkdir(newcat->incomingpath);
		}
	}
}


uint16 CPreferences::GetDefaultMaxConperFive()
{
	return MAXCONPER5SEC;
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
int32 CPreferences::GetColumnSortItem(TablePreference t)
{
	switch(t) {
		case TP_Download:
			return s_tableSortItemDownload;
		case TP_Upload:
			return s_tableSortItemUpload;
		case TP_Queue:
			return s_tableSortItemQueue;
		case TP_Search:
			return s_tableSortItemSearch;
		case TP_Shared:
			return s_tableSortItemShared;
		case TP_Server:
			return s_tableSortItemServer;
		case TP_ClientList:
			return s_tableSortItemClientList;
		default:
			return 0;
	}
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(TablePreference t)
{
	switch(t) {
		case TP_Download:
			return s_tableSortAscendingDownload;
		case TP_Upload:
			return s_tableSortAscendingUpload;
		case TP_Queue:
			return s_tableSortAscendingQueue;
		case TP_Search:
			return s_tableSortAscendingSearch;
		case TP_Shared:
			return s_tableSortAscendingShared;
		case TP_Server:
			return s_tableSortAscendingServer;
		case TP_ClientList:
			return s_tableSortAscendingClientList;
		default:
			return true;
	}
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(TablePreference t, int32 sortItem)
{
	switch(t) {
		case TP_Download:
			s_tableSortItemDownload = sortItem;
			break;
		case TP_Upload:
			s_tableSortItemUpload = sortItem;
			break;
		case TP_Queue:
			s_tableSortItemQueue = sortItem;
			break;
		case TP_Search:
			s_tableSortItemSearch = sortItem;
			break;
		case TP_Shared:
			s_tableSortItemShared = sortItem;
			break;
		case TP_Server:
			s_tableSortItemServer = sortItem;
			break;
		case TP_ClientList:
			s_tableSortItemClientList = sortItem;
			break;
		default:
			break;
	}
}


// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(TablePreference t, bool sortAscending)
{
	switch(t) {
		case TP_Download:
			s_tableSortAscendingDownload = sortAscending;
			break;
		case TP_Upload:
			s_tableSortAscendingUpload = sortAscending;
			break;
		case TP_Queue:
			s_tableSortAscendingQueue = sortAscending;
			break;
		case TP_Search:
			s_tableSortAscendingSearch = sortAscending;
			break;
		case TP_Shared:
			s_tableSortAscendingShared = sortAscending;
			break;
		case TP_Server:
			s_tableSortAscendingServer = sortAscending;
			break;
		case TP_ClientList:
			s_tableSortAscendingClientList = sortAscending;
			break;
		default:
			break;
	}
}


uint32 CPreferences::AddCat(Category_Struct* cat)
{
	m_CatList.push_back( cat );
	
	return m_CatList.size() - 1;
}


void CPreferences::RemoveCat(size_t index)
{
	if ( index < m_CatList.size() ) {
		CatList::iterator it = m_CatList.begin() + index;
	
		delete *it;
		
		m_CatList.erase( it );
	}
}


uint32 CPreferences::GetCatCount()
{
	return m_CatList.size();
}


Category_Struct* CPreferences::GetCategory(size_t index)
{
	wxASSERT( index < m_CatList.size() );

	return m_CatList[index];
}


const wxString&	CPreferences::GetCatPath(uint8 index)
{
	wxASSERT( index < m_CatList.size() );
	
	return m_CatList[index]->incomingpath;
}

DWORD CPreferences::CPreferences::GetCatColor(size_t index)
{
	wxASSERT( index < m_CatList.size() );

	return m_CatList[index]->color;
}



// Jacobo221 - Several issues on the browsers:
// netscape is named Netscape on some systems
// MozillaFirebird is named mozilla-firebird and also firebird on some systems
// Niether Galeon tabs nor epiphany tabs have been tested
// Konqueror alternatives is (Open on current window, fails if no konq already open):
//	dcop `dcop konqueror-* | head -n1` konqueror-mainwindow#1 openURL '%s'
wxString CPreferences::GetBrowser()
{
	wxString cmd;
	if( s_BrowserTab )
                switch ( s_Browser ) {
                        case 0: cmd = wxT("kfmclient exec '%s'"); break;
                        case 1: cmd = wxT("sh -c \"if ! mozilla -remote 'openURL(%s, new-tab)'; then mozilla '%s'; fi\""); break;
                        case 2: cmd = wxT("sh -c \"if ! firefox -remote 'openURL(%s, new-tab)'; then firefox '%s'; fi\""); break;
                        case 3: cmd = wxT("sh -c \"if ! MozillaFirebird -remote 'openURL(%s, new-tab)'; then MozillaFirebird '%s'; fi\""); break;
                        case 4: cmd = wxT("opera --newpage '%s'"); break;
                        case 5: cmd = wxT("sh -c \"if ! netscape -remote 'openURLs(%s,new-tab)'; then netscape '%s'; fi\""); break;
                        case 6: cmd = wxT("galeon -n '%s'"); break;
                        case 7: cmd = wxT("epiphany -n '%s'"); break;
                        case 8: cmd = s_CustomBrowser; break;
                        default:
                                printf("Unable to determine selected browser!\n");
                }
        else
		switch ( s_Browser ) {
			case 0: cmd = wxT("konqueror '%s'"); break;
			case 1: cmd = wxT("sh -c 'mozilla %s'"); break;
			case 2: cmd = wxT("firefox '%s'"); break;
			case 3:	cmd = wxT("MozillaFirebird '%s'"); break;
			case 4:	cmd = wxT("opera '%s'"); break;
			case 5: cmd = wxT("netscape '%s'"); break;
			case 6: cmd = wxT("galeon '%s'"); break;
			case 7: cmd = wxT("epiphany '%s'"); break;
			case 8: cmd = s_CustomBrowser; break;
			default:
				printf("Unable to determine selected browser!\n");
		}
	
	return cmd;
}

