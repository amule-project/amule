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

#include <wx/wx.h>
#include "Preferences.h"

#include <protocol/ed2k/Constants.h>
#include <common/Constants.h>
#include <common/DataFileVersion.h>

#include <wx/config.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/stopwatch.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>		// Needed for wxBusyCursor

#include "amule.h"
#ifdef HAVE_CONFIG_H
	#include "config.h"		// Needed for PACKAGE_STRING
#endif

#include "CFile.h"
#include <common/MD5Sum.h>
#include "Logger.h"
#include <common/Format.h>		// Needed for CFormat
#include <common/TextFile.h>		// Needed for CTextFile
#include <common/ClientVersion.h>

#include "UserEvents.h"

#ifndef AMULE_DAEMON
#include <wx/valgen.h>
#include "muuli_wdr.h"
#include "StatisticsDlg.h"
#include "MuleColour.h"
#endif

#ifndef CLIENT_GUI
#include "RandomFunctions.h"
#include "PlatformSpecific.h"		// Needed for PlatformSpecific::GetMaxConnections()
#endif

// Needed for IP filtering prefs
#include "ClientList.h"
#include "ServerList.h"
#include "GuiEvents.h"

#define DEFAULT_TCP_PORT 4662
#define DEFAULT_UDP_PORT 4672

// Static variables
unsigned long		CPreferences::s_colors[cntStatColors];
unsigned long		CPreferences::s_colors_ref[cntStatColors];

CPreferences::CFGMap	CPreferences::s_CfgList;
CPreferences::CFGList	CPreferences::s_MiscList;


/* Proxy */
CProxyData	CPreferences::s_ProxyData;

/* The rest, organize it! */
wxString	CPreferences::s_nick;
Cfg_Lang_Base * CPreferences::s_cfgLang;
uint16		CPreferences::s_maxupload;
uint16		CPreferences::s_maxdownload;
uint16		CPreferences::s_slotallocation;
wxString	CPreferences::s_Addr;
uint16		CPreferences::s_port;
uint16		CPreferences::s_udpport;
bool		CPreferences::s_UDPEnable;
uint16		CPreferences::s_maxconnections;
bool		CPreferences::s_reconnect;
bool		CPreferences::s_autoconnect;
bool		CPreferences::s_autoconnectstaticonly;
bool		CPreferences::s_UPnPEnabled;
bool		CPreferences::s_UPnPECEnabled;
bool		CPreferences::s_UPnPWebServerEnabled;
uint16		CPreferences::s_UPnPTCPPort;
bool		CPreferences::s_autoserverlist;
bool		CPreferences::s_deadserver;
CPath		CPreferences::s_incomingdir;
CPath		CPreferences::s_tempdir;
bool		CPreferences::s_ICH;
uint8		CPreferences::s_depth3D;
bool		CPreferences::s_scorsystem;
bool		CPreferences::s_mintotray;
bool		CPreferences::s_trayiconenabled;
bool		CPreferences::s_addnewfilespaused;
bool		CPreferences::s_addserversfromserver;
bool		CPreferences::s_addserversfromclient;
uint16		CPreferences::s_maxsourceperfile;
uint16		CPreferences::s_trafficOMeterInterval;
uint16		CPreferences::s_statsInterval;
uint32		CPreferences::s_maxGraphDownloadRate;
uint32		CPreferences::s_maxGraphUploadRate;
bool		CPreferences::s_confirmExit;
bool		CPreferences::s_filterLanIP;
bool		CPreferences::s_paranoidfilter;
bool		CPreferences::s_IPFilterSys;
bool		CPreferences::s_onlineSig;
uint16		CPreferences::s_OSUpdate;
uint64		CPreferences::s_totalDownloadedBytes;
uint64		CPreferences::s_totalUploadedBytes;
wxString	CPreferences::s_languageID;
uint8		CPreferences::s_iSeeShares;
uint8		CPreferences::s_iToolDelayTime;
uint8		CPreferences::s_splitterbarPosition;
uint16		CPreferences::s_deadserverretries;
uint32		CPreferences::s_dwServerKeepAliveTimeoutMins;
uint8		CPreferences::s_statsMax;
uint8		CPreferences::s_statsAverageMinutes;
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
bool		CPreferences::s_bmanualhighprio;
bool		CPreferences::s_bstartnextfile;
bool		CPreferences::s_bstartnextfilesame;
bool		CPreferences::s_bstartnextfilealpha;
bool		CPreferences::s_bshowoverhead;
bool		CPreferences::s_bDAP;
bool		CPreferences::s_bUAP;
uint8_t		CPreferences::s_showRatesOnTitle;
wxString	CPreferences::s_VideoPlayer;
bool		CPreferences::s_showAllNotCats;
bool		CPreferences::s_msgonlyfriends;
bool		CPreferences::s_msgsecure;
uint8		CPreferences::s_filterlevel;
uint8		CPreferences::s_iFileBufferSize;
uint8		CPreferences::s_iQueueSize;
wxString 	CPreferences::s_datetimeformat;
wxString	CPreferences::s_sWebPath;
wxString	CPreferences::s_sWebPassword;
wxString	CPreferences::s_sWebLowPassword;
uint16		CPreferences::s_nWebPort;
uint16		CPreferences::s_nWebUPnPTCPPort;
bool		CPreferences::s_bWebEnabled;
bool		CPreferences::s_bWebUseGzip;
uint32		CPreferences::s_nWebPageRefresh;
bool		CPreferences::s_bWebLowEnabled;
wxString	CPreferences::s_WebTemplate;
bool		CPreferences::s_showCatTabInfos;
AllCategoryFilter CPreferences::s_allcatFilter;
uint8		CPreferences::s_NoNeededSources;
bool		CPreferences::s_DropFullQueueSources;
bool		CPreferences::s_DropHighQueueRankingSources;
uint32		CPreferences::s_HighQueueRanking;
uint32		CPreferences::s_AutoDropTimer;
bool 		CPreferences::s_AcceptExternalConnections;
wxString	CPreferences::s_ECAddr;
uint32		CPreferences::s_ECPort;
wxString	CPreferences::s_ECPassword;
bool		CPreferences::s_TransmitOnlyUploadingClients;
bool		CPreferences::s_IPFilterClients;
bool		CPreferences::s_IPFilterServers;
bool		CPreferences::s_UseSrcSeeds;
bool		CPreferences::s_ProgBar;
bool		CPreferences::s_Percent;
bool		CPreferences::s_SecIdent;
bool		CPreferences::s_ExtractMetaData;
bool		CPreferences::s_allocFullFile;
wxString	CPreferences::s_CustomBrowser;
bool		CPreferences::s_BrowserTab;
CPath		CPreferences::s_OSDirectory;
wxString	CPreferences::s_Skin;
bool		CPreferences::s_FastED2KLinksHandler;
bool		CPreferences::s_ToolbarOrientation;
bool		CPreferences::s_AICHTrustEveryHash;
wxString 	CPreferences::s_CommentFilterString;
bool		CPreferences::s_IPFilterAutoLoad;
wxString	CPreferences::s_IPFilterURL;
CMD4Hash	CPreferences::s_userhash;
bool		CPreferences::s_MustFilterMessages;
wxString 	CPreferences::s_MessageFilterString;
bool		CPreferences::s_FilterAllMessages;
bool 		CPreferences::s_FilterComments;
bool		CPreferences::s_FilterSomeMessages;
bool		CPreferences::s_ShowMessagesInLog;
bool		CPreferences::s_IsAdvancedSpamfilterEnabled;
bool		CPreferences::s_IsChatCaptchaEnabled;
bool		CPreferences::s_ShareHiddenFiles;
bool		CPreferences::s_AutoSortDownload;
bool		CPreferences::s_NewVersionCheck;
bool		CPreferences::s_ConnectToKad;
bool		CPreferences::s_ConnectToED2K;
unsigned	CPreferences::s_maxClientVersions;
bool		CPreferences::s_DropSlowSources;
bool		CPreferences::s_IsClientCryptLayerSupported;
bool		CPreferences::s_bCryptLayerRequested;
bool		CPreferences::s_IsClientCryptLayerRequired;
uint32		CPreferences::s_dwKadUDPKey;
uint8		CPreferences::s_byCryptTCPPaddingLength;

wxString 	CPreferences::s_Ed2kURL;
wxString 	CPreferences::s_KadURL;
bool	 	CPreferences::s_GeoIPEnabled;
wxString 	CPreferences::s_GeoIPUpdateUrl;
bool		CPreferences::s_preventSleepWhileDownloading;
wxString 	CPreferences::s_StatsServerName;
wxString 	CPreferences::s_StatsServerURL;

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
	
	
	/** Updates the assosiated variable, returning true on success. */
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
	
	/** Updates the assosiated widget, returning true on success. */
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

	/** Sets the default value. */
	void	SetDefault(const TYPE& defaultVal)
	{
		m_default = defaultVal;
	}

protected:
	//! Reference to the associated variable
	TYPE&	m_value;

	//! Default variable value
	TYPE	m_default;
	
	//! Pointer to the widget assigned to the Cfg instance
	wxWindow*	m_widget;
};


/** Cfg class for wxStrings. */
class Cfg_Str : public Cfg_Tmpl<wxString>
{
public:
	/** Constructor. */
	Cfg_Str( const wxString& keyname, wxString& value, const wxString& defaultVal = EmptyString )
	 : Cfg_Tmpl<wxString>( keyname, value, defaultVal )
	{}

	/** Loads the string, using the specified default value. */
	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		cfg->Read( GetKey(), &m_value, m_default );
	}

	
	/** Saves the string to the specified wxConfig object. */
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
	Cfg_Str_Encrypted( const wxString& keyname, wxString& value, const wxString& defaultVal = EmptyString )
	 : Cfg_Str( keyname, value, defaultVal )
	{}

#ifndef AMULE_DAEMON
	virtual bool TransferFromWindow()
	{
		// Shakraw: when storing value, store it encrypted here (only if changed in prefs)
		if ( Cfg_Str::TransferFromWindow() ) {

			// Only recalucate the hash for new, non-empty passwords
			if ( HasChanged() && !m_value.IsEmpty() ) {
				m_value = MD5Sum( m_value ).GetHash();
			}

			return true;
		}


		return false;
	}
#endif
};


/** Cfg class for CPath. */
class Cfg_Path : public Cfg_Str
{
public:
	/** Constructor. */
	Cfg_Path(const wxString& keyname, CPath& value, const wxString& defaultVal = EmptyString )
	 : Cfg_Str(keyname, m_temp_path, defaultVal)
	 , m_real_path(value)
	{}

	/** @see Cfg_Str::LoadFromFile. */
	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		Cfg_Str::LoadFromFile(cfg);

		m_real_path = CPath::FromUniv(m_temp_path);
	}

	
	/** @see Cfg_Str::SaveToFile. */
	virtual void SaveToFile(wxConfigBase* cfg)
	{
		m_temp_path = CPath::ToUniv(m_real_path);

		Cfg_Str::SaveToFile(cfg);
	}


	/** @see Cfg_Tmpl::TransferToWindow. */
	virtual bool TransferToWindow()
	{
		m_temp_path = m_real_path.GetRaw();

		return Cfg_Str::TransferToWindow();
	}
	
	/** @see Cfg_Tmpl::TransferFromWindow. */
	virtual bool TransferFromWindow()
	{
		if (Cfg_Str::TransferFromWindow()) {
			m_real_path = CPath(m_temp_path);
			return true;
		}

		return false;
	}

private:
	wxString m_temp_path;
	CPath&   m_real_path;
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
			wxSlider *slider = dynamic_cast<wxSlider *>(m_widget);
			if (slider) {
				int id = m_widget->GetId();
				int pos = slider->GetValue();
				wxScrollEvent evt( wxEVT_SCROLL_THUMBRELEASE, id, pos );
				m_widget->GetEventHandler()->ProcessEvent( evt );
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
 * Cfg-class for uint64s, with no associated widgets.
 */
class Cfg_Counter : public Cfg_Base
{
public:
	Cfg_Counter( const wxString& keyname, uint64& value )
	 : Cfg_Base( keyname ),
	   m_value( value )
	{}

	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		wxString buffer;
	
		cfg->Read( GetKey(), &buffer, wxT("0") );

		uint64 tmp = 0;
		for (unsigned int i = 0; i < buffer.Length(); ++i) {
			if ((buffer[i] >= wxChar('0')) &&(buffer[i] <= wxChar('9'))) {
				tmp = tmp * 10 + (buffer[i] - wxChar('0'));
			} else {
				tmp = 0;
				break;
			}
		}
		m_value = tmp;
	}

	virtual void SaveToFile(wxConfigBase* cfg)
	{
		wxString str = CFormat( wxT("%llu") ) % m_value;
	
		cfg->Write( GetKey(), str );
	}

	
protected:
	uint64& m_value;
};


#ifndef AMULE_DAEMON

class Cfg_Colour : public Cfg_Base
{
      public:
	Cfg_Colour(const wxString& key, wxColour& colour)
		: Cfg_Base(key),
		  m_colour(colour),
		  m_default(CMuleColour(colour).GetULong())
	{}

	virtual void LoadFromFile(wxConfigBase* cfg)
	{
		long int rgb;
		cfg->Read(GetKey(), &rgb, m_default);
		m_colour.Set(rgb);
	}

	virtual void SaveToFile(wxConfigBase* cfg)
	{
		cfg->Write(GetKey(), static_cast<long int>(CMuleColour(m_colour).GetULong()));
	}

      private:
	wxColour&	m_colour;
	long int	m_default;
};


typedef struct {
	int	 id;
	bool	 available;
	wxString displayname;
	wxString name;
} LangInfo;


/**
 * The languages aMule has translation for.
 *
 * Add new languages here. 
 * Then activate the test code in Cfg_Lang::UpdateChoice below!
 */
static LangInfo aMuleLanguages[] = {
	{ wxLANGUAGE_DEFAULT,				true,	wxEmptyString,	wxTRANSLATE("System default") },
	{ wxLANGUAGE_ALBANIAN,				false,	wxEmptyString,	wxTRANSLATE("Albanian") },
	{ wxLANGUAGE_ARABIC,				false,	wxEmptyString,	wxTRANSLATE("Arabic") },
	{ wxLANGUAGE_ASTURIAN,				false,	wxEmptyString,	wxTRANSLATE("Asturian") },
	{ wxLANGUAGE_BASQUE,				false,	wxEmptyString,	wxTRANSLATE("Basque") },
	{ wxLANGUAGE_BULGARIAN,				false,	wxEmptyString,	wxTRANSLATE("Bulgarian") },
	{ wxLANGUAGE_CATALAN,				false,	wxEmptyString,	wxTRANSLATE("Catalan") },
	{ wxLANGUAGE_CHINESE_SIMPLIFIED,	false,	wxEmptyString,	wxTRANSLATE("Chinese (Simplified)") },
	{ wxLANGUAGE_CHINESE_TRADITIONAL,	false,	wxEmptyString,	wxTRANSLATE("Chinese (Traditional)") },
	{ wxLANGUAGE_CROATIAN,				false,	wxEmptyString,	wxTRANSLATE("Croatian") },
	{ wxLANGUAGE_CZECH,					false,	wxEmptyString,	wxTRANSLATE("Czech") },
	{ wxLANGUAGE_DANISH,				false,	wxEmptyString,	wxTRANSLATE("Danish") },
	{ wxLANGUAGE_DUTCH,					false,	wxEmptyString,	wxTRANSLATE("Dutch") },
	{ wxLANGUAGE_ENGLISH,				false,	wxEmptyString,	wxTRANSLATE("English (U.K.)") },
	{ wxLANGUAGE_ESTONIAN,				false,	wxEmptyString,	wxTRANSLATE("Estonian") },
	{ wxLANGUAGE_FINNISH,				false,	wxEmptyString,	wxTRANSLATE("Finnish") },
	{ wxLANGUAGE_FRENCH,				false,	wxEmptyString,	wxTRANSLATE("French") },
	{ wxLANGUAGE_GALICIAN,				false,	wxEmptyString,	wxTRANSLATE("Galician") },
	{ wxLANGUAGE_GERMAN,				false,	wxEmptyString,	wxTRANSLATE("German") },
	{ wxLANGUAGE_GREEK,					false,	wxEmptyString,	wxTRANSLATE("Greek") },
	{ wxLANGUAGE_HEBREW,				false,	wxEmptyString,	wxTRANSLATE("Hebrew") },
	{ wxLANGUAGE_HUNGARIAN,				false,	wxEmptyString,	wxTRANSLATE("Hungarian") },
	{ wxLANGUAGE_ITALIAN,				false,	wxEmptyString,	wxTRANSLATE("Italian") },
	{ wxLANGUAGE_ITALIAN_SWISS,			false,	wxEmptyString,	wxTRANSLATE("Italian (Swiss)") },
	{ wxLANGUAGE_JAPANESE,				false,	wxEmptyString,	wxTRANSLATE("Japanese") },
	{ wxLANGUAGE_KOREAN,				false,	wxEmptyString,	wxTRANSLATE("Korean") },
	{ wxLANGUAGE_LITHUANIAN,			false,	wxEmptyString,	wxTRANSLATE("Lithuanian") },
	{ wxLANGUAGE_NORWEGIAN_NYNORSK,		false,	wxEmptyString,	wxTRANSLATE("Norwegian (Nynorsk)") },
	{ wxLANGUAGE_POLISH,				false,	wxEmptyString,	wxTRANSLATE("Polish") },
	{ wxLANGUAGE_PORTUGUESE,			false,	wxEmptyString,	wxTRANSLATE("Portuguese") },
	{ wxLANGUAGE_PORTUGUESE_BRAZILIAN,	false,	wxEmptyString,	wxTRANSLATE("Portuguese (Brazilian)") },
	{ wxLANGUAGE_ROMANIAN,				false,	wxEmptyString,	wxTRANSLATE("Romanian") },
	{ wxLANGUAGE_RUSSIAN,				false,	wxEmptyString,	wxTRANSLATE("Russian") },
	{ wxLANGUAGE_SLOVENIAN,				false,	wxEmptyString,	wxTRANSLATE("Slovenian") },
	{ wxLANGUAGE_SPANISH,				false,	wxEmptyString,	wxTRANSLATE("Spanish") },
	{ wxLANGUAGE_SWEDISH,				false,	wxEmptyString,	wxTRANSLATE("Swedish") },
	{ wxLANGUAGE_TURKISH,				false,	wxEmptyString,	wxTRANSLATE("Turkish") },
	{ wxLANGUAGE_UKRAINIAN,				false,	wxEmptyString,	wxTRANSLATE("Ukrainian") },
};


typedef Cfg_Int<int> Cfg_PureInt;

class Cfg_Lang : public Cfg_PureInt, public Cfg_Lang_Base
{
public:
	Cfg_Lang()
		: Cfg_PureInt( wxEmptyString, m_selection, 0 )
	{
		m_languagesReady = false;
		m_changePos = 0;
	}

	virtual void LoadFromFile(wxConfigBase* WXUNUSED(cfg)) {}
	virtual void SaveToFile(wxConfigBase* WXUNUSED(cfg)) {}


	virtual bool TransferFromWindow()
	{
		if (!m_languagesReady) {
			return true;	// nothing changed, no problem
		}

		if ( Cfg_PureInt::TransferFromWindow() ) { 
			// find wx ID of selected language
			int i = 0;
			while (m_selection > 0) {
				i++;
				if (aMuleLanguages[i].available) {
					m_selection--;
				}
			}
			int id = aMuleLanguages[i].id;

			// save language selection
			thePrefs::SetLanguageID(wxLang2Str(id));

			return true;
		}

		return false;
	}


	virtual bool TransferToWindow()
	{
		m_langSelector = dynamic_cast<wxChoice*>(m_widget);	// doesn't work in ctor!
		if (m_languagesReady) {
			FillChoice();
		} else {
			int wxId = StrLang2wx(thePrefs::GetLanguageID());
			m_langSelector->Clear();
			m_selection = 0;
			for (uint32 i = 0; i < itemsof(aMuleLanguages); i++) {
				if ( aMuleLanguages[i].id == wxId ) {
					m_langSelector->Append(wxString(wxGetTranslation(aMuleLanguages[i].name)) + wxT(" [") + aMuleLanguages[i].name + wxT("]"));
					break;
				}
			}
			m_langSelector->Append(_("Change Language"));
			m_changePos = m_langSelector->GetCount() - 1;
		}

		return Cfg_PureInt::TransferToWindow();
	}

	virtual void UpdateChoice(int pos)
	{
		if (!m_languagesReady && pos == m_changePos) {
			// Find available languages and translate them.
			// This is only done when the user selects "Change Language"
			// Language is changed rarely, and the go-through-all locales takes a considerable
			// time when the settings dialog is opened for the first time.
			wxBusyCursor busyCursor;
			aMuleLanguages[0].displayname = wxGetTranslation(aMuleLanguages[0].name);
			for (unsigned int i = 1; i < itemsof(aMuleLanguages); ++i) {
				if (wxLocale::IsAvailable(aMuleLanguages[i].id)) {
					// This supresses error-messages about invalid locales.
					wxLogNull	logTarget;
					wxLocale 	locale_to_check;

					InitLocale(locale_to_check, aMuleLanguages[i].id);
					if (locale_to_check.IsOk() && locale_to_check.IsLoaded(wxT(PACKAGE))) {
						aMuleLanguages[i].displayname = wxString(wxGetTranslation(aMuleLanguages[i].name)) + wxT(" [") + aMuleLanguages[i].name + wxT("]");
						aMuleLanguages[i].available = true;
#if 0
						// Check for language problems
						// Activate this code temporarily after messing with the languages!
						int wxid = StrLang2wx(wxLang2Str(aMuleLanguages[i].id));
						if (wxid != aMuleLanguages[i].id) {
							AddDebugLogLineN(logGeneral, CFormat(wxT("Language problem for %s : aMule id %d != wx id %d"))
								% aMuleLanguages[i].name % aMuleLanguages[i].id % wxid);
						}
#endif
					}
				}
			}
			// Restore original locale
			wxLocale tmpLocale;
			InitLocale(tmpLocale, theApp->m_locale.GetLanguage());
			FillChoice();
			m_langSelector->SetSelection(m_selection);
			m_languagesReady = true;
		}
	}

protected:
	int	m_selection;

private:
	void FillChoice()
	{
		int wxId = StrLang2wx(thePrefs::GetLanguageID());
		m_langSelector->Clear();
		// Add all available languages and find the index of the selected language.
		for ( unsigned int i = 0, j = 0; i < itemsof(aMuleLanguages); i++) {
			if (aMuleLanguages[i].available) {
				m_langSelector->Append(aMuleLanguages[i].displayname);
				if ( aMuleLanguages[i].id == wxId ) {
					m_selection = j;
				}
				j++;
			}
		}
	}

	bool m_languagesReady;	// true: all translations calculated
	int	 m_changePos;
	wxChoice * m_langSelector;
};

#endif /* ! AMULE_DAEMON */
 
void Cfg_Lang_Base::UpdateChoice(int) {}	// dummy

class Cfg_Skin : public Cfg_Str
{
public:
	Cfg_Skin( const wxString& keyname, wxString& value, const wxString& defaultVal = EmptyString )
		: Cfg_Str( keyname, value, defaultVal ),
		  m_is_skin(false)
	{}

#ifndef AMULE_DAEMON
	virtual bool TransferFromWindow()
	{
		if ( Cfg_Str::TransferFromWindow() ) {
			if (m_is_skin) {
				wxChoice *skinSelector = dynamic_cast<wxChoice*>(m_widget);
				// "- default -" is always the first
				if (skinSelector->GetSelection() == 0) {
					m_value.Clear();
				}
			}
			return true;
		}

		return false;
	}


	virtual bool TransferToWindow()
	{

		wxChoice *skinSelector = dynamic_cast<wxChoice*>(m_widget);
		skinSelector->Clear();

		wxString folder;
		int flags = wxDIR_DIRS;
		wxString filespec;
		wxString defaultSelection = _("- default -");
//#warning there has to be a better way...
		if ( GetKey() == wxT("/SkinGUIOptions/Skin") ) {
			folder = wxT("skins");
			m_is_skin = true;
			flags = wxDIR_FILES;
			filespec = wxT("*.zip");
			skinSelector->Append(defaultSelection);
		} else {
			folder = wxT("webserver");
		}
		wxString dirName(JoinPaths(GetConfigDir(theApp->m_configFile), folder));
		wxString Filename;
		wxDir d;
		
		if (wxDir::Exists(dirName) &&
			d.Open(dirName) &&
			d.GetFirst(& Filename, filespec, flags)
			)
		{
			do
			{
				if (m_is_skin) {
					Filename = wxT("User:") + Filename;
				}
				skinSelector->Append(Filename);
			}
			while (d.GetNext(&Filename));
		}

		wxString dataDir;
		if (m_is_skin) {
			dataDir = wxStandardPaths::Get().GetDataDir();
		} else {
			dataDir = wxStandardPaths::Get().GetResourcesDir();
		}
#if !defined(__WXMSW__) && !defined(__WXMAC__)
		dataDir = dataDir.BeforeLast(wxT('/')) + wxT("/amule");
#endif
		wxString systemDir(JoinPaths(dataDir,folder));

		if (wxDir::Exists(systemDir) &&
			d.Open(systemDir) &&
			d.GetFirst(& Filename, filespec, flags)
			)
		{
			do
			{
				if (m_is_skin) {
					Filename = wxT("System:") +  Filename;
				}
				// avoid duplicates for webserver templates
				if (skinSelector->FindString(Filename) == wxNOT_FOUND) {
					skinSelector->Append(Filename);
				}
			}
			while (d.GetNext(&Filename));
		}

		if ( skinSelector->GetCount() == 0 ) {
			skinSelector->Append(_("no options available"));	
		}

		int id = skinSelector->FindString(m_value);
		if ( id == wxNOT_FOUND ) {
			id = 0;
			if (m_is_skin) {
				m_value = defaultSelection;
			}
		}
		skinSelector->SetSelection(id);

		return Cfg_Str::TransferToWindow();
	}
#endif /* ! AMULE_DAEMON */

      protected:
	bool	m_is_skin;
};


/// new implementation
CPreferences::CPreferences()
{
	srand( wxGetLocalTimeMillis().GetLo() ); // we need random numbers sometimes

	// load preferences.dat or set standard values
	wxString fullpath(theApp->ConfigDir + wxT("preferences.dat"));
	CFile preffile;
	if (wxFileExists(fullpath)) {
		if (preffile.Open(fullpath, CFile::read)) {
			try {
				preffile.ReadUInt8(); // Version. Value is not used.
				s_userhash = preffile.ReadHash();
			} catch (const CSafeIOException& e) {
				AddDebugLogLineC(logGeneral,
					wxT("Error while reading userhash: ") + e.what());
			}
		}
	}

	if (s_userhash.IsEmpty()) {
		for (int i = 0; i < 8; i++) {
			RawPokeUInt16(s_userhash.GetHash() + (i * 2), rand());
		}

		Save();
	}
	
	// Mark hash as an eMule-type hash
	// See also CUpDownClient::GetHashType
	s_userhash[5] = 14;
	s_userhash[14] = 111;
	
#ifndef CLIENT_GUI
	LoadPreferences();
	ReloadSharedFolders();

	// serverlist adresses
	CTextFile slistfile;
	if (slistfile.Open(theApp->ConfigDir + wxT("addresses.dat"), CTextFile::read)) {
		adresses_list = slistfile.ReadLines();
	}
#endif
}

//
// Gets called at init time
// 
void CPreferences::BuildItemList( const wxString& appdir )
{
#ifndef AMULE_DAEMON
	#define NewCfgItem(ID, COMMAND)	s_CfgList[ID] = COMMAND
#else
	int current_id = 0;
	#define NewCfgItem(ID, COMMAND)	s_CfgList[++current_id] = COMMAND
#endif /* AMULE_DAEMON */
	
	/**
	 * User settings
	 **/
	NewCfgItem(IDC_NICK,		(new Cfg_Str(  wxT("/eMule/Nick"), s_nick, wxT("http://www.aMule.org") )));
#ifndef AMULE_DAEMON
	Cfg_Lang * cfgLang = new Cfg_Lang();
	s_cfgLang = cfgLang;
	NewCfgItem(IDC_LANGUAGE,	cfgLang);
#endif

	/**
	 * Browser options
	 **/
	#ifdef __WXMAC__
		wxString	customBrowser = wxT("/usr/bin/open");
	#else 
		wxString	customBrowser; // left empty
	#endif

	NewCfgItem(IDC_BROWSERTABS,	(new Cfg_Bool( wxT("/Browser/OpenPageInTab"), s_BrowserTab, true )));
	NewCfgItem(IDC_BROWSERSELF,	(new Cfg_Str(  wxT("/Browser/CustomBrowserString"), s_CustomBrowser, customBrowser )));


	/**
	 * Misc
	 **/
	NewCfgItem(IDC_QUEUESIZE,	(MkCfg_Int( wxT("/eMule/QueueSizePref"), s_iQueueSize, 50 )));


#ifdef __DEBUG__
	/**
	 * Debugging
	 **/
	NewCfgItem(ID_VERBOSEDEBUG, (new Cfg_Bool( wxT("/eMule/VerboseDebug"), s_bVerbose, false )));
#endif

	/**
	 * Connection settings
	 **/
	NewCfgItem(IDC_MAXUP,		(MkCfg_Int( wxT("/eMule/MaxUpload"), s_maxupload, 0 )));
	NewCfgItem(IDC_MAXDOWN,		(MkCfg_Int( wxT("/eMule/MaxDownload"), s_maxdownload, 0 )));
	NewCfgItem(IDC_SLOTALLOC,	(MkCfg_Int( wxT("/eMule/SlotAllocation"), s_slotallocation, 2 )));
	NewCfgItem(IDC_PORT,		(MkCfg_Int( wxT("/eMule/Port"), s_port, DEFAULT_TCP_PORT )));
	NewCfgItem(IDC_UDPPORT,		(MkCfg_Int( wxT("/eMule/UDPPort"), s_udpport, DEFAULT_UDP_PORT )));
	NewCfgItem(IDC_UDPENABLE,	(new Cfg_Bool( wxT("/eMule/UDPEnable"), s_UDPEnable, true )));
	NewCfgItem(IDC_ADDRESS,		(new Cfg_Str( wxT("/eMule/Address"), s_Addr, wxEmptyString)));
	NewCfgItem(IDC_AUTOCONNECT,	(new Cfg_Bool( wxT("/eMule/Autoconnect"), s_autoconnect, true )));
	NewCfgItem(IDC_MAXSOURCEPERFILE,	(MkCfg_Int( wxT("/eMule/MaxSourcesPerFile"), s_maxsourceperfile, 300 )));
	NewCfgItem(IDC_MAXCON,		(MkCfg_Int( wxT("/eMule/MaxConnections"), s_maxconnections, GetRecommendedMaxConnections() )));
	NewCfgItem(IDC_MAXCON5SEC,	(MkCfg_Int( wxT("/eMule/MaxConnectionsPerFiveSeconds"), s_MaxConperFive, 20 )));

	/**
	 * Proxy
	 **/
	NewCfgItem(ID_PROXY_ENABLE_PROXY,	(new Cfg_Bool( wxT("/Proxy/ProxyEnableProxy"), s_ProxyData.m_proxyEnable, false )));
	NewCfgItem(ID_PROXY_TYPE,		(MkCfg_Int( wxT("/Proxy/ProxyType"), s_ProxyData.m_proxyType, 0 )));
	NewCfgItem(ID_PROXY_NAME,		(new Cfg_Str( wxT("/Proxy/ProxyName"), s_ProxyData.m_proxyHostName, wxEmptyString )));
	NewCfgItem(ID_PROXY_PORT,		(MkCfg_Int( wxT("/Proxy/ProxyPort"), s_ProxyData.m_proxyPort, 1080 )));
	NewCfgItem(ID_PROXY_ENABLE_PASSWORD,	(new Cfg_Bool( wxT("/Proxy/ProxyEnablePassword"), s_ProxyData.m_enablePassword, false )));
	NewCfgItem(ID_PROXY_USER,		(new Cfg_Str( wxT("/Proxy/ProxyUser"), s_ProxyData.m_userName, wxEmptyString )));
	NewCfgItem(ID_PROXY_PASSWORD,		(new Cfg_Str( wxT("/Proxy/ProxyPassword"), s_ProxyData.m_password, wxEmptyString )));
// These were copied from eMule config file, maybe someone with windows can complete this?
//	NewCfgItem(ID_PROXY_AUTO_SERVER_CONNECT_WITHOUT_PROXY,	(new Cfg_Bool( wxT("/Proxy/Proxy????"), s_Proxy????, false )));
	
	/**
	 * Servers
	 **/ 
	NewCfgItem(IDC_REMOVEDEAD,	(new Cfg_Bool( wxT("/eMule/RemoveDeadServer"), s_deadserver, 1 )));
	NewCfgItem(IDC_SERVERRETRIES,	(MkCfg_Int( wxT("/eMule/DeadServerRetry"), s_deadserverretries, 3 )));
	NewCfgItem(IDC_SERVERKEEPALIVE,	(MkCfg_Int( wxT("/eMule/ServerKeepAliveTimeout"), s_dwServerKeepAliveTimeoutMins, 0 )));
	NewCfgItem(IDC_RECONN,		(new Cfg_Bool( wxT("/eMule/Reconnect"), s_reconnect, true )));
	NewCfgItem(IDC_SCORE,		(new Cfg_Bool( wxT("/eMule/Scoresystem"), s_scorsystem, true )));
	NewCfgItem(IDC_AUTOSERVER,	(new Cfg_Bool( wxT("/eMule/Serverlist"), s_autoserverlist, false )));
	NewCfgItem(IDC_UPDATESERVERCONNECT,	(new Cfg_Bool( wxT("/eMule/AddServerListFromServer"), s_addserversfromserver, false)));
	NewCfgItem(IDC_UPDATESERVERCLIENT,	(new Cfg_Bool( wxT("/eMule/AddServerListFromClient"), s_addserversfromclient, false )));
	NewCfgItem(IDC_SAFESERVERCONNECT,	(new Cfg_Bool( wxT("/eMule/SafeServerConnect"), s_safeServerConnect, false )));
	NewCfgItem(IDC_AUTOCONNECTSTATICONLY,	(new Cfg_Bool( wxT("/eMule/AutoConnectStaticOnly"), s_autoconnectstaticonly, false )));
	NewCfgItem(IDC_UPNP_ENABLED,	(new Cfg_Bool( wxT("/eMule/UPnPEnabled"), s_UPnPEnabled, false )));
	NewCfgItem(IDC_UPNPTCPPORT,	(MkCfg_Int( wxT("/eMule/UPnPTCPPort"), s_UPnPTCPPort, 50000 )));
	NewCfgItem(IDC_SMARTIDCHECK,	(new Cfg_Bool( wxT("/eMule/SmartIdCheck"), s_smartidcheck, true )));
	// Enabled networks
	NewCfgItem( IDC_NETWORKKAD, (new Cfg_Bool( wxT("/eMule/ConnectToKad"),	s_ConnectToKad, true )) );
	NewCfgItem( IDC_NETWORKED2K, ( new Cfg_Bool( wxT("/eMule/ConnectToED2K"),	s_ConnectToED2K, true ) ));


	/**
	 * Files
	 **/
	NewCfgItem(IDC_TEMPFILES,	(new Cfg_Path(  wxT("/eMule/TempDir"), 	s_tempdir, appdir + wxT("Temp") )));
	
	#if defined(__WXMAC__) || defined(__WXMSW__)
		wxString incpath = wxStandardPaths::Get().GetDocumentsDir();
		if (incpath.IsEmpty()) {
			// There is a built-in possibility for this call to fail, though I can't imagine a reason for that.
			incpath = appdir + wxT("Incoming");
		} else {
			incpath = JoinPaths(incpath, wxT("aMule Downloads"));
		}
	#else 
		wxString incpath = appdir + wxT("Incoming");
	#endif
	NewCfgItem(IDC_INCFILES,	(new Cfg_Path(  wxT("/eMule/IncomingDir"), s_incomingdir, incpath )));
	
	NewCfgItem(IDC_ICH,		(new Cfg_Bool( wxT("/eMule/ICH"), s_ICH, true )));
	NewCfgItem(IDC_AICHTRUST,	(new Cfg_Bool( wxT("/eMule/AICHTrust"), s_AICHTrustEveryHash, false )));
	NewCfgItem(IDC_CHECKDISKSPACE,	(new Cfg_Bool( wxT("/eMule/CheckDiskspace"), s_checkDiskspace, true )));
	NewCfgItem(IDC_MINDISKSPACE,	(MkCfg_Int( wxT("/eMule/MinFreeDiskSpace"), s_uMinFreeDiskSpace, 1 )));
	NewCfgItem(IDC_ADDNEWFILESPAUSED,	(new Cfg_Bool( wxT("/eMule/AddNewFilesPaused"), s_addnewfilespaused, false )));
	NewCfgItem(IDC_PREVIEWPRIO,	(new Cfg_Bool( wxT("/eMule/PreviewPrio"), s_bpreviewprio, false )));
	NewCfgItem(IDC_MANUALSERVERHIGHPRIO,	(new Cfg_Bool( wxT("/eMule/ManualHighPrio"), s_bmanualhighprio, false )));
	NewCfgItem(IDC_STARTNEXTFILE,	(new Cfg_Bool( wxT("/eMule/StartNextFile"), s_bstartnextfile, false )));
	NewCfgItem(IDC_STARTNEXTFILE_SAME,	(new Cfg_Bool( wxT("/eMule/StartNextFileSameCat"), s_bstartnextfilesame, false )));
	NewCfgItem(IDC_STARTNEXTFILE_ALPHA,	(new Cfg_Bool( wxT("/eMule/StartNextFileAlpha"), s_bstartnextfilealpha, false )));
	NewCfgItem(IDC_SRCSEEDS,	(new Cfg_Bool( wxT("/ExternalConnect/UseSrcSeeds"), s_UseSrcSeeds, false )));
	NewCfgItem(IDC_FILEBUFFERSIZE,	(MkCfg_Int( wxT("/eMule/FileBufferSizePref"), s_iFileBufferSize, 16 )));
	NewCfgItem(IDC_DAP,		(new Cfg_Bool( wxT("/eMule/DAPPref"), s_bDAP, true )));
	NewCfgItem(IDC_UAP,		(new Cfg_Bool( wxT("/eMule/UAPPref"), s_bUAP, true )));
	NewCfgItem(IDC_ALLOCFULLFILE,	(new Cfg_Bool( wxT("/eMule/AllocateFullFile"), s_allocFullFile, false )));

	/**
	 * Web Server
	 */
	NewCfgItem(IDC_OSDIR,		(new Cfg_Path(  wxT("/eMule/OSDirectory"), s_OSDirectory,	appdir )));
	NewCfgItem(IDC_ONLINESIG,	(new Cfg_Bool( wxT("/eMule/OnlineSignature"), s_onlineSig, false )));
	NewCfgItem(IDC_OSUPDATE,	(MkCfg_Int( wxT("/eMule/OnlineSignatureUpdate"), s_OSUpdate, 5 )));
	NewCfgItem(IDC_ENABLE_WEB,	(new Cfg_Bool( wxT("/WebServer/Enabled"), s_bWebEnabled, false )));
	NewCfgItem(IDC_WEB_PASSWD,	(new Cfg_Str_Encrypted( wxT("/WebServer/Password"), s_sWebPassword )));
	NewCfgItem(IDC_WEB_PASSWD_LOW,	(new Cfg_Str_Encrypted( wxT("/WebServer/PasswordLow"), s_sWebLowPassword )));
	NewCfgItem(IDC_WEB_PORT,	(MkCfg_Int( wxT("/WebServer/Port"), s_nWebPort, 4711 )));
	NewCfgItem(IDC_WEBUPNPTCPPORT,	(MkCfg_Int( wxT("/WebServer/WebUPnPTCPPort"), s_nWebUPnPTCPPort, 50001 )));
	NewCfgItem(IDC_UPNP_WEBSERVER_ENABLED,
					(new Cfg_Bool( wxT("/WebServer/UPnPWebServerEnabled"), s_UPnPWebServerEnabled, false )));
	NewCfgItem(IDC_WEB_GZIP,	(new Cfg_Bool( wxT("/WebServer/UseGzip"), s_bWebUseGzip, true )));
	NewCfgItem(IDC_ENABLE_WEB_LOW,	(new Cfg_Bool( wxT("/WebServer/UseLowRightsUser"), s_bWebLowEnabled, false )));
	NewCfgItem(IDC_WEB_REFRESH_TIMEOUT,	(MkCfg_Int( wxT("/WebServer/PageRefreshTime"), s_nWebPageRefresh, 120 )));
	NewCfgItem(IDC_WEBTEMPLATE,	(new Cfg_Skin( wxT("/WebServer/Template"), s_WebTemplate, wxEmptyString )));

	/**
	 * External Connections
	 */
	NewCfgItem(IDC_EXT_CONN_ACCEPT,	(new Cfg_Bool( wxT("/ExternalConnect/AcceptExternalConnections"), s_AcceptExternalConnections, false )));
	NewCfgItem(IDC_EXT_CONN_IP,	(new Cfg_Str( wxT("/ExternalConnect/ECAddress"), s_ECAddr, wxEmptyString )));
	NewCfgItem(IDC_EXT_CONN_TCP_PORT,	(MkCfg_Int( wxT("/ExternalConnect/ECPort"), s_ECPort, 4712 )));
	NewCfgItem(IDC_EXT_CONN_PASSWD,	(new Cfg_Str_Encrypted( wxT("/ExternalConnect/ECPassword"), s_ECPassword, wxEmptyString )));
	NewCfgItem(IDC_UPNP_EC_ENABLED,	(new Cfg_Bool( wxT("/ExternalConnect/UPnPECEnabled"), s_UPnPECEnabled, false )));

	/**
	 * GUI behavior
	 **/
	NewCfgItem(IDC_ENABLETRAYICON,	(new Cfg_Bool( wxT("/eMule/EnableTrayIcon"), s_trayiconenabled, false )));
	NewCfgItem(IDC_MINTRAY,		(new Cfg_Bool( wxT("/eMule/MinToTray"), s_mintotray, false )));
	NewCfgItem(IDC_EXIT,		(new Cfg_Bool( wxT("/eMule/ConfirmExit"), s_confirmExit, true )));
	NewCfgItem(IDC_STARTMIN,	(new Cfg_Bool( wxT("/eMule/StartupMinimized"), s_startMinimized, false )));

	/**
	 * GUI appearence
	 **/
	NewCfgItem(IDC_3DDEPTH,		(MkCfg_Int( wxT("/eMule/3DDepth"), s_depth3D, 10 )));
	NewCfgItem(IDC_TOOLTIPDELAY,	(MkCfg_Int( wxT("/eMule/ToolTipDelay"), s_iToolDelayTime, 1 )));
	NewCfgItem(IDC_SHOWOVERHEAD,	(new Cfg_Bool( wxT("/eMule/ShowOverhead"), s_bshowoverhead, false )));
	NewCfgItem(IDC_EXTCATINFO,	(new Cfg_Bool( wxT("/eMule/ShowInfoOnCatTabs"), s_showCatTabInfos, true )));
	NewCfgItem(IDC_FED2KLH,		(new Cfg_Bool( wxT("/Razor_Preferences/FastED2KLinksHandler"), s_FastED2KLinksHandler, true )));
	NewCfgItem(IDC_PROGBAR,		(new Cfg_Bool( wxT("/ExternalConnect/ShowProgressBar"), s_ProgBar, true )));
	NewCfgItem(IDC_PERCENT,		(new Cfg_Bool( wxT("/ExternalConnect/ShowPercent"), s_Percent, true )));
	NewCfgItem(IDC_SKIN,		(new Cfg_Skin(  wxT("/SkinGUIOptions/Skin"), s_Skin, wxEmptyString )));
	NewCfgItem(IDC_VERTTOOLBAR,	(new Cfg_Bool( wxT("/eMule/VerticalToolbar"), s_ToolbarOrientation, false )));
	NewCfgItem(IDC_SHOW_COUNTRY_FLAGS,	(new Cfg_Bool( wxT("/eMule/GeoIPEnabled"), s_GeoIPEnabled, true )));
	
	/**
	 * External Apps
	 */
	NewCfgItem(IDC_VIDEOPLAYER,	(new Cfg_Str(  wxT("/eMule/VideoPlayer"), s_VideoPlayer, wxEmptyString )));

	/**
	 * Statistics
	 **/
	NewCfgItem(IDC_SLIDER,		(MkCfg_Int( wxT("/eMule/StatGraphsInterval"), s_trafficOMeterInterval, 3 )));
	NewCfgItem(IDC_SLIDER2,		(MkCfg_Int( wxT("/eMule/statsInterval"), s_statsInterval, 30 )));
	NewCfgItem(IDC_DOWNLOAD_CAP,	(MkCfg_Int( wxT("/eMule/DownloadCapacity"), s_maxGraphDownloadRate, 300 )));
	NewCfgItem(IDC_UPLOAD_CAP,	(MkCfg_Int( wxT("/eMule/UploadCapacity"), s_maxGraphUploadRate, 100 )));
	NewCfgItem(IDC_SLIDER3,		(MkCfg_Int( wxT("/eMule/StatsAverageMinutes"), s_statsAverageMinutes, 5 )));
	NewCfgItem(IDC_SLIDER4,		(MkCfg_Int( wxT("/eMule/VariousStatisticsMaxValue"), s_statsMax, 100 )));
	NewCfgItem(IDC_CLIENTVERSIONS,	(MkCfg_Int( wxT("/Statistics/MaxClientVersions"), s_maxClientVersions, 0 )));

	/**
	 * Security
	 **/
	NewCfgItem(IDC_SEESHARES,	(MkCfg_Int( wxT("/eMule/SeeShare"),	s_iSeeShares, 2 )));
	NewCfgItem(IDC_SECIDENT,        (new Cfg_Bool( wxT("/ExternalConnect/UseSecIdent"), s_SecIdent, true )));
	NewCfgItem(IDC_IPFCLIENTS,	(new Cfg_Bool( wxT("/ExternalConnect/IpFilterClients"), s_IPFilterClients, true )));
	NewCfgItem(IDC_IPFSERVERS,	(new Cfg_Bool( wxT("/ExternalConnect/IpFilterServers"), s_IPFilterServers, true )));
	NewCfgItem(IDC_FILTERLAN,		(new Cfg_Bool( wxT("/eMule/FilterLanIPs"), s_filterLanIP, true )));
	NewCfgItem(IDC_PARANOID,		(new Cfg_Bool( wxT("/eMule/ParanoidFiltering"), s_paranoidfilter, true )));
	NewCfgItem(IDC_AUTOIPFILTER,	(new Cfg_Bool( wxT("/eMule/IPFilterAutoLoad"), s_IPFilterAutoLoad, true )));
	NewCfgItem(IDC_IPFILTERURL,	(new Cfg_Str(  wxT("/eMule/IPFilterURL"), s_IPFilterURL, wxEmptyString )));
	NewCfgItem(ID_IPFILTERLEVEL,	(MkCfg_Int( wxT("/eMule/FilterLevel"), s_filterlevel, 127 )));
	NewCfgItem(IDC_IPFILTERSYS,	(new Cfg_Bool( wxT("/eMule/IPFilterSystem"), s_IPFilterSys, false )));
		
	/** 
	 * Message Filter 
	 **/
	NewCfgItem(IDC_MSGFILTER,	(new Cfg_Bool( wxT("/eMule/FilterMessages"), s_MustFilterMessages, true )));
	NewCfgItem(IDC_MSGFILTER_ALL,	(new Cfg_Bool( wxT("/eMule/FilterAllMessages"), s_FilterAllMessages, false )));
	NewCfgItem(IDC_MSGFILTER_NONFRIENDS,	(new Cfg_Bool( wxT("/eMule/MessagesFromFriendsOnly"),	s_msgonlyfriends, false )));
	NewCfgItem(IDC_MSGFILTER_NONSECURE,	(new Cfg_Bool( wxT("/eMule/MessageFromValidSourcesOnly"),	s_msgsecure, true )));
	NewCfgItem(IDC_MSGFILTER_WORD,	(new Cfg_Bool( wxT("/eMule/FilterWordMessages"), s_FilterSomeMessages, false )));
	NewCfgItem(IDC_MSGWORD,		(new Cfg_Str(  wxT("/eMule/MessageFilter"), s_MessageFilterString, wxEmptyString )));
	NewCfgItem(IDC_MSGLOG,	(new Cfg_Bool( wxT("/eMule/ShowMessagesInLog"), s_ShowMessagesInLog, true )));
	//Todo NewCfgItem(IDC_MSGADVSPAM,	(new Cfg_Bool( wxT("/eMule/AdvancedSpamFilter"), s_IsAdvancedSpamfilterEnabled, true )));
	//Todo NewCfgItem(IDC_MSGCAPTCHA,	(new Cfg_Bool( wxT("/eMule/MessageUseCaptchas"), s_IsChatCaptchaEnabled, true )));
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/AdvancedSpamFilter"), s_IsAdvancedSpamfilterEnabled, true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/MessageUseCaptchas"), s_IsChatCaptchaEnabled, true ) );

	NewCfgItem(IDC_FILTERCOMMENTS,	(new Cfg_Bool( wxT("/eMule/FilterComments"), s_FilterComments, false )));
	NewCfgItem(IDC_COMMENTWORD,		(new Cfg_Str(  wxT("/eMule/CommentFilter"), s_CommentFilterString, wxEmptyString )));
	 
	/**
	 * Hidden files sharing
	 **/	  
	NewCfgItem(IDC_SHAREHIDDENFILES,	(new Cfg_Bool( wxT("/eMule/ShareHiddenFiles"), s_ShareHiddenFiles, false )));

	/**
	 * Auto-Sorting of downloads
	 **/
	 NewCfgItem(IDC_AUTOSORT,	 (new Cfg_Bool( wxT("/eMule/AutoSortDownloads"), s_AutoSortDownload, false )));

	/**
	 * Version check
	 **/
	 NewCfgItem(IDC_NEWVERSION,	(new Cfg_Bool( wxT("/eMule/NewVersionCheck"), s_NewVersionCheck, true )));
	 
	 /**
	  * Obfuscation
	  **/
	NewCfgItem( IDC_SUPPORT_PO, ( new Cfg_Bool( wxT("/Obfuscation/IsClientCryptLayerSupported"), s_IsClientCryptLayerSupported, true )));
	NewCfgItem( IDC_ENABLE_PO_OUTGOING, ( new Cfg_Bool( wxT("/Obfuscation/IsCryptLayerRequested"), s_bCryptLayerRequested, true )));
	NewCfgItem( IDC_ENFORCE_PO_INCOMING, ( new Cfg_Bool( wxT("/Obfuscation/IsClientCryptLayerRequired"), s_IsClientCryptLayerRequired, false )));
#ifndef CLIENT_GUI	 
	// There is no need for GUI items for this two.
	s_MiscList.push_back( MkCfg_Int( wxT("/Obfuscation/CryptoPaddingLenght"), s_byCryptTCPPaddingLength, 254 ) );
	s_MiscList.push_back( MkCfg_Int( wxT("/Obfuscation/CryptoKadUDPKey"), s_dwKadUDPKey, GetRandomUint32() ) );	 
#endif

	/**
	 * Power management
	 **/
	NewCfgItem( IDC_PREVENT_SLEEP, ( new Cfg_Bool( wxT("/PowerManagement/PreventSleepWhileDownloading"), s_preventSleepWhileDownloading, false )));

	/**
	 * The following doesn't have an associated widget or section
	 **/
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/Language"),			s_languageID ) );
	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalDownloadedBytes"), s_totalDownloadedBytes ) );
	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalUploadedBytes"),	s_totalUploadedBytes ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/SplitterbarPosition"),		s_splitterbarPosition, 75 ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/YourHostname"),			s_yourHostname, wxEmptyString ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/DateTimeFormat"),		s_datetimeformat, wxT("%A, %x, %X") ) );

	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/AllcatType"),			s_allcatFilter, 0 ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/ShowAllNotCats"),		s_showAllNotCats, false ) );

	s_MiscList.push_back( MkCfg_Int( wxT("/eMule/SmartIdState"), s_smartidstate, 0 ) );
	
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DropSlowSources"), 		s_DropSlowSources, false ) );
		
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/KadNodesUrl"),			s_KadURL, wxT("http://download.tuxfamily.org/technosalad/utils/nodes.dat") ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/Ed2kServersUrl"),		s_Ed2kURL, wxT("http://gruk.org/server.met.gz") ) );
	s_MiscList.push_back( MkCfg_Int( wxT("/eMule/ShowRatesOnTitle"),		s_showRatesOnTitle, 0 ));

	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/GeoLiteCountryUpdateUrl"),		s_GeoIPUpdateUrl, wxT("http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/GeoIP.dat.gz") ) );
	wxConfigBase::Get()->DeleteEntry(wxT("/eMule/GeoIPUpdateUrl")); // get rid of the old one for a while

	s_MiscList.push_back( new Cfg_Str( wxT("/WebServer/Path"),				s_sWebPath, wxT("amuleweb") ) );

	s_MiscList.push_back( new Cfg_Str( wxT("/eMule/StatsServerName"),		s_StatsServerName,	wxT("Shorty's ED2K stats") ) );
	s_MiscList.push_back( new Cfg_Str( wxT("/eMule/StatsServerURL"),		s_StatsServerURL,	wxT("http://ed2k.shortypower.dyndns.org/?hash=") ) );

	s_MiscList.push_back( new Cfg_Bool( wxT("/ExternalConnect/TransmitOnlyUploadingClients"),	s_TransmitOnlyUploadingClients, false ) );

#ifndef AMULE_DAEMON
	// Colors have been moved from global prefs to CStatisticsDlg
	for ( int i = 0; i < cntStatColors; i++ ) {  
		wxString str = CFormat(wxT("/eMule/StatColor%i")) % i;
		s_MiscList.push_back( new Cfg_Colour( str, CStatisticsDlg::acrStat[i] ) );
	}
#endif

	// User events
	for (unsigned int i = 0; i < CUserEvents::GetCount(); ++i) {
		// We can't use NewCfgItem here, because we need to find these items
		// later, which would be impossible in amuled with NewCfgItem.
		// The IDs we assign here are high enough to not cause any collision
		// even on the daemon.
		s_CfgList[USEREVENTS_FIRST_ID + i * USEREVENTS_IDS_PER_EVENT + 1] = new Cfg_Bool(wxT("/UserEvents/") + CUserEvents::GetKey(i) + wxT("/CoreEnabled"), CUserEvents::GetCoreEnableVar(i), false);
		s_CfgList[USEREVENTS_FIRST_ID + i * USEREVENTS_IDS_PER_EVENT + 2] = new Cfg_Str(wxT("/UserEvents/") + CUserEvents::GetKey(i) + wxT("/CoreCommand"), CUserEvents::GetCoreCommandVar(i), wxEmptyString);
		s_CfgList[USEREVENTS_FIRST_ID + i * USEREVENTS_IDS_PER_EVENT + 3] = new Cfg_Bool(wxT("/UserEvents/") + CUserEvents::GetKey(i) + wxT("/GUIEnabled"), CUserEvents::GetGUIEnableVar(i), false);
		s_CfgList[USEREVENTS_FIRST_ID + i * USEREVENTS_IDS_PER_EVENT + 4] = new Cfg_Str(wxT("/UserEvents/") + CUserEvents::GetKey(i) + wxT("/GUICommand"), CUserEvents::GetGUICommandVar(i), wxEmptyString);
	}
}


void CPreferences::EraseItemList()
{
	while ( s_CfgList.begin() != s_CfgList.end() ) {
		delete s_CfgList.begin()->second;
		s_CfgList.erase( s_CfgList.begin() );
	}
	
	CFGList::iterator it = s_MiscList.begin();
	for ( ; it != s_MiscList.end();  ) {
		delete *it;
		it = s_MiscList.erase( it );
	}
}


void CPreferences::LoadAllItems(wxConfigBase* cfg)
{
#ifndef CLIENT_GUI
	// Preserve values from old config. The global config object may not be set yet
	// when BuildItemList() is called, so we need to provide defaults later - here.
	if (cfg->HasEntry(wxT("/eMule/ExecOnCompletion"))) {
		bool ExecOnCompletion;
		cfg->Read(wxT("/eMule/ExecOnCompletion"), &ExecOnCompletion, false);
		// Assign to core command, that's the most likely it was.
		static_cast<Cfg_Bool*>(s_CfgList[USEREVENTS_FIRST_ID + CUserEvents::DownloadCompleted * USEREVENTS_IDS_PER_EVENT + 1])->SetDefault(ExecOnCompletion);
		cfg->DeleteEntry(wxT("/eMule/ExecOnCompletion"));
	}
	if (cfg->HasEntry(wxT("/eMule/ExecOnCompletionCommand"))) {
		wxString ExecOnCompletionCommand;
		cfg->Read(wxT("/eMule/ExecOnCompletionCommand"), &ExecOnCompletionCommand, wxEmptyString);
		static_cast<Cfg_Str*>(s_CfgList[USEREVENTS_FIRST_ID + CUserEvents::DownloadCompleted * USEREVENTS_IDS_PER_EVENT + 2])->SetDefault(ExecOnCompletionCommand);
		cfg->DeleteEntry(wxT("/eMule/ExecOnCompletionCommand"));
	}
#endif
	CFGMap::iterator it_a = s_CfgList.begin();
	for ( ; it_a != s_CfgList.end(); ++it_a ) {
		it_a->second->LoadFromFile( cfg );
	}

	CFGList::iterator it_b = s_MiscList.begin();
	for ( ; it_b != s_MiscList.end(); ++it_b ) {
		(*it_b)->LoadFromFile( cfg ); 
	}

	// Preserve old value of UDPDisable
	if (cfg->HasEntry(wxT("/eMule/UDPDisable"))) {
		bool UDPDisable;
		cfg->Read(wxT("/eMule/UDPDisable"), &UDPDisable, false);
		SetUDPDisable(UDPDisable);
		cfg->DeleteEntry(wxT("/eMule/UDPDisable"));
	}

	// Preserve old value of UseSkinFiles
	if (cfg->HasEntry(wxT("/SkinGUIOptions/UseSkinFiles"))) {
		bool UseSkinFiles;
		cfg->Read(wxT("/SkinGUIOptions/UseSkinFiles"), &UseSkinFiles, false);
		if (!UseSkinFiles) {
			s_Skin.Clear();
		}
		cfg->DeleteEntry(wxT("/SkinGUIOptions/UseSkinFiles"));
	}

#ifdef __DEBUG__
	// Load debug-categories
	int count = theLogger.GetDebugCategoryCount();

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = theLogger.GetDebugCategory( i );
		
		bool enabled = false;
		cfg->Read( wxT("/Debug/Cat_") + cat.GetName(), &enabled );

		theLogger.SetEnabled( cat.GetType(), enabled );
	}	
#endif
	
	// Now do some post-processing / sanity checking on the values we just loaded
#ifndef CLIENT_GUI
	CheckUlDlRatio();
	SetPort(s_port);
	if (s_byCryptTCPPaddingLength > 254) {
		s_byCryptTCPPaddingLength = GetRandomUint8() % 254;
	}
	SetSlotAllocation(s_slotallocation);
#endif
}


void CPreferences::SaveAllItems(wxConfigBase* cfg)
{
	// Save the Cfg values
	CFGMap::iterator it_a = s_CfgList.begin();
	for ( ; it_a != s_CfgList.end(); ++it_a )
		it_a->second->SaveToFile( cfg );

	CFGList::iterator it_b = s_MiscList.begin();
	for ( ; it_b != s_MiscList.end(); ++it_b )
		(*it_b)->SaveToFile( cfg ); 


// Save debug-categories
#ifdef __DEBUG__
	int count = theLogger.GetDebugCategoryCount();

	for ( int i = 0; i < count; i++ ) {
		const CDebugCategory& cat = theLogger.GetDebugCategory( i );

		cfg->Write( wxT("/Debug/Cat_") + cat.GetName(), cat.IsEnabled() );
	}	
#endif
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


void CPreferences::UnsetAutoServerStart()
{
	s_autoserverlist = false;
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
	if ( s_maxupload == 0xFFFF )
		s_maxupload = UNLIMITED;

	// Backwards compatibility
	if ( s_maxdownload == 0xFFFF )
		s_maxdownload = UNLIMITED;
		
	if ( s_maxupload == UNLIMITED )
		return;
	
	// Enforce the limits
	if ( s_maxupload < 4  ) {
		if ( ( s_maxupload * 3 < s_maxdownload ) || ( s_maxdownload == 0 ) )
			s_maxdownload = s_maxupload * 3 ;
	} else if ( s_maxupload < 10  ) {
		if ( ( s_maxupload * 4 < s_maxdownload ) || ( s_maxdownload == 0 ) )
			s_maxdownload = s_maxupload * 4;
	}
}


void CPreferences::Save()
{
	wxString fullpath(theApp->ConfigDir + wxT("preferences.dat"));

	CFile preffile;
	if (!wxFileExists(fullpath)) {
		preffile.Create(fullpath);
	}
	
	if (preffile.Open(fullpath, CFile::read_write)) {
		try {
			preffile.WriteUInt8(PREFFILE_VERSION);
			preffile.WriteHash(s_userhash);
		} catch (const CIOFailureException& e) {
			AddDebugLogLineC(logGeneral, wxT("IO failure while saving user-hash: ") + e.what());
		}
	}

	SavePreferences();

	#ifndef CLIENT_GUI
	CTextFile sdirfile;
	if (sdirfile.Open(theApp->ConfigDir + wxT("shareddir.dat"), CTextFile::write)) {
		for (size_t i = 0; i < shareddir_list.size(); ++i) {
			sdirfile.WriteLine(CPath::ToUniv(shareddir_list[i]), wxConvUTF8);
		}

	}
	#endif
}


CPreferences::~CPreferences()
{
	DeleteContents(m_CatList);
}


int32 CPreferences::GetRecommendedMaxConnections()
{
#ifndef CLIENT_GUI
	int iRealMax = PlatformSpecific::GetMaxConnections();
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
#else
	return 500;
#endif
}


void CPreferences::SavePreferences()
{
	wxConfigBase* cfg = wxConfigBase::Get();

	cfg->Write( wxT("/eMule/AppVersion"), wxT(VERSION) );

	// Save the options
	SaveAllItems( cfg );

	// Ensure that the changes are saved to disk.
	cfg->Flush();
}


void CPreferences::SaveCats()
{
	if ( GetCatCount() ) {
		wxConfigBase* cfg = wxConfigBase::Get();

		// Save the main cat.
		cfg->Write( wxT("/eMule/AllcatType"), (int)s_allcatFilter);
		
		// The first category is the default one and should not be counted

		cfg->Write( wxT("/General/Count"), (long)(m_CatList.size() - 1) );

		uint32 maxcat = m_CatList.size();
		for (uint32 i = 1; i < maxcat; i++) {
			cfg->SetPath(CFormat(wxT("/Cat#%i")) % i);

			cfg->Write( wxT("Title"),	m_CatList[i]->title );
			cfg->Write( wxT("Incoming"),	CPath::ToUniv(m_CatList[i]->path) );
			cfg->Write( wxT("Comment"),	m_CatList[i]->comment );
			cfg->Write( wxT("Color"),	wxString(CFormat(wxT("%u")) % m_CatList[i]->color));
			cfg->Write( wxT("Priority"),	(int)m_CatList[i]->prio );
		}
		// remove deleted cats from config
		while (cfg->DeleteGroup(CFormat(wxT("/Cat#%i")) % maxcat++)) {}
		
		cfg->Flush();
	}
}


void CPreferences::LoadPreferences()
{
	LoadCats();
}


void CPreferences::LoadCats()
{
	// default cat ... Meow! =(^.^)=
	Category_Struct* defaultcat = new Category_Struct;
	defaultcat->prio = 0;
	defaultcat->color = 0;

	AddCat( defaultcat );

	wxConfigBase* cfg = wxConfigBase::Get();

	long max = cfg->Read( wxT("/General/Count"), 0l );

	for ( int i = 1; i <= max ; i++ ) {
		cfg->SetPath(CFormat(wxT("/Cat#%i")) % i);

		Category_Struct* newcat = new Category_Struct;

		newcat->title = cfg->Read( wxT("Title"), wxEmptyString );
		newcat->path  = CPath::FromUniv(cfg->Read(wxT("Incoming"), wxEmptyString));

		// Some sanity checking
		if ( newcat->title.IsEmpty() || !newcat->path.IsOk() ) {
			AddLogLineN(_("Invalid category found, skipping"));
			
			delete newcat;
			continue;
		}

		newcat->comment = cfg->Read( wxT("Comment"), wxEmptyString );
		newcat->prio = cfg->Read( wxT("Priority"), 0l );
		newcat->color = StrToULong(cfg->Read(wxT("Color"), wxT("0")));

		AddCat(newcat);
		
		if (!newcat->path.DirExists()) {
			CPath::MakeDir(newcat->path);
		}
	}
}


uint16 CPreferences::GetDefaultMaxConperFive()
{
	return MAXCONPER5SEC;
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


const CPath& CPreferences::GetCatPath(uint8 index)
{
	wxASSERT( index < m_CatList.size() );
	
	return m_CatList[index]->path;
}


uint32 CPreferences::GetCatColor(size_t index)
{
	wxASSERT( index < m_CatList.size() );

	return m_CatList[index]->color;
}

bool CPreferences::CreateCategory(
	Category_Struct *& category,
	const wxString& name,
	const CPath& path,
	const wxString& comment,
	uint32 color,
	uint8 prio)
{
	category = new Category_Struct();
	category->path = thePrefs::GetIncomingDir();	// set a default in case path is invalid
	uint32 cat = AddCat(category);
	return UpdateCategory(cat, name, path, comment, color, prio);
}

bool CPreferences::UpdateCategory(
	uint8 cat, 
	const wxString& name,
	const CPath& path,
	const wxString& comment,
	uint32 color,
	uint8 prio)
{
	Category_Struct *category = m_CatList[cat];

	// return true if path is ok, false if not
	bool ret = true;
	if (!path.IsOk() || (!path.DirExists() && !CPath::MakeDir(path))) {
		ret = false;
		// keep path as it was
	} else {
		category->path		= path;
	}
	category->title			= name;
	category->comment		= comment;
	category->color			= color;
	category->prio			= prio;
	
	SaveCats();
	return ret;
}


wxString CPreferences::GetBrowser()
{
	wxString cmd(s_CustomBrowser);
#ifndef __WXMSW__
	if( s_BrowserTab ) {
		// This is certainly not the best way to do it, but I'm lazy
		if ((wxT("mozilla") == cmd.Right(7)) || (wxT("firefox") == cmd.Right(7))
			|| (wxT("MozillaFirebird") == cmd.Right(15))) {
			cmd += wxT(" -remote 'openURL(%s, new-tab)'");
		}
		if ((wxT("galeon") == cmd.Right(6)) || (wxT("epiphany") == cmd.Right(8))) {
			cmd += wxT(" -n '%s'");
		}
		if (wxT("opera") == cmd.Right(5)) {
			cmd += wxT(" --newpage '%s'");
		}
		if (wxT("netscape") == cmd.Right(8)) {
			cmd += wxT(" -remote 'openURLs(%s,new-tab)'");
		}
	}
#endif /* !__WXMSW__ */
	return cmd;
}

void CPreferences::SetFilteringClients(bool val)
{
	if (val != s_IPFilterClients) {
		s_IPFilterClients = val;
		if (val) {
			theApp->clientlist->FilterQueues();
		}
	}
}

void CPreferences::SetFilteringServers(bool val)
{
	if (val != s_IPFilterServers) {
		s_IPFilterServers = val;
		if (val) {
			theApp->serverlist->FilterServers();
		}
	}
}

void CPreferences::SetIPFilterLevel(uint8 level)
{
	if (level != s_filterlevel) {
		// Set the new access-level
		s_filterlevel = level;
#ifndef CLIENT_GUI
		// and reload the filter
		NotifyAlways_IPFilter_Reload();
#endif
	}
}

void CPreferences::SetPort(uint16 val)
{ 
	// Warning: Check for +3, because server UDP is TCP+3
	
	if (val +3 > 65535) {
		AddLogLineC(_("TCP port can't be higher than 65532 due to server UDP socket being TCP+3"));
		AddLogLineN(CFormat(_("Default port will be used (%d)")) % DEFAULT_TCP_PORT);
		s_port = DEFAULT_TCP_PORT;
	} else {
		s_port = val;
	}
}


void CPreferences::ReloadSharedFolders()
{
#ifndef CLIENT_GUI
	shareddir_list.clear();

	CTextFile file;
	if (file.Open(theApp->ConfigDir + wxT("shareddir.dat"), CTextFile::read)) {
		wxArrayString lines = file.ReadLines(txtReadDefault, wxConvUTF8);

		for (size_t i = 0; i < lines.size(); ++i) {
			CPath path = CPath::FromUniv(lines[i]);

			if (path.DirExists()) {
				shareddir_list.push_back(path);
			} else {
				AddLogLineN(CFormat(_("Dropping non-existing shared directory: %s")) % path.GetRaw());
			}
		}
	}
#endif
}


bool CPreferences::IsMessageFiltered(const wxString& message)
{ 
	if (s_FilterAllMessages) { 
		return true;
	} else {
		if (s_FilterSomeMessages) {
			if (s_MessageFilterString.IsSameAs(wxT("*"))){  
				// Filter anything
				return true;
			} else {
				wxStringTokenizer tokenizer( s_MessageFilterString, wxT(",") );
				while (tokenizer.HasMoreTokens()) {
					if ( message.Lower().Trim(false).Trim(true).Contains(
							tokenizer.GetNextToken().Lower().Trim(false).Trim(true))) {
						return true;
					}
				}
				return false;
			}			
		} else {
			return false;
		}
	}
}


bool CPreferences::IsCommentFiltered(const wxString& comment)
{ 
	if (s_FilterComments) { 
		wxStringTokenizer tokenizer( s_CommentFilterString, wxT(",") );
		while (tokenizer.HasMoreTokens()) {
			if ( comment.Lower().Trim(false).Trim(true).Contains(
					tokenizer.GetNextToken().Lower().Trim(false).Trim(true))) {
				return true;
			}
		}
	}
	return false;
}

wxString CPreferences::GetLastHTTPDownloadURL(uint8 t)
{
	wxConfigBase* cfg = wxConfigBase::Get();
	wxString key = CFormat(wxT("/HTTPDownload/URL_%d")) % t;
	return cfg->Read(key, wxEmptyString);
}

void CPreferences::SetLastHTTPDownloadURL(uint8 t, const wxString& val)
{
	wxConfigBase* cfg = wxConfigBase::Get();
	wxString key = CFormat(wxT("/HTTPDownload/URL_%d")) % t;
	cfg->Write(key, val);
}

// File_checked_for_headers
