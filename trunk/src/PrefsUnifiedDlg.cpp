// This file is part of the aMule Project
//
// Copyright (c) 2004 aMule Project ( http://www.amule-project.net )
// Original author: Emilio Sandoz
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/colordlg.h>
#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>

#ifdef __WXGTK__
	#include <wx/gtk/tooltip.h>
#endif

#include "PrefsUnifiedDlg.h"
#include "Preferences.h"
#include "amule.h"				// Needed for theApp
#include "otherfunctions.h"		// Needed for MakeFoldername
#include "CTypedPtrList.h"		// Needed for CList
#include "EditServerListDlg.h"
#include "amuleDlg.h"
#include "SharedFileList.h"		// Needed for CSharedFileList
#include "StatisticsDlg.h"		// Needed for graph parameters, colors
#include "IPFilter.h"			// Needed for CIPFilter
#include "SearchList.h"
#include "DownloadQueue.h"
#include "ClientList.h"
#include "DirectoryTreeCtrl.h"	// Needed for CDirectoryTreeCtrl
#include "MD5Sum.h"


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



// Static variables
int							PrefsUnifiedDlg::s_ID;
PrefsUnifiedDlg::CFGMap		PrefsUnifiedDlg::s_CfgList;
PrefsUnifiedDlg::CFGList	PrefsUnifiedDlg::s_MiscList;
COLORREF					PrefsUnifiedDlg::s_colors[cntStatColors];
COLORREF					PrefsUnifiedDlg::s_colors_ref[cntStatColors];



void PrefsUnifiedDlg::BuildItemList( const wxString& appdir )  // gets called at init time
{
	/**
	 * User settings
	 **/
	s_CfgList[IDC_NICK]				= new Cfg_Str(  wxT("/eMule/Nick"), CPreferences::s_nick, wxT("http://www.aMule.org") );
	s_CfgList[IDC_LANGUAGE]			=    MkCfg_Int( wxT("/eMule/Language"), CPreferences::s_languageID, 0 );


	/**
	 * Misc
	 **/
	s_CfgList[IDC_FCHECK]			=    MkCfg_Int( wxT("/FakeCheck/Browser"), CPreferences::s_Browser, 0 );
	s_CfgList[IDC_FCHECKTABS]		= new Cfg_Bool( wxT("/FakeCheck/BrowserTab"), CPreferences::s_BrowserTab, true );
	s_CfgList[IDC_FCHECKSELF]		= new Cfg_Str(  wxT("/FakeCheck/CustomBrowser"), CPreferences::s_CustomBrowser, wxT("") );
	s_CfgList[IDC_QUEUESIZE]		=    MkCfg_Int( wxT("/eMule/QueueSizePref"), CPreferences::s_iQueueSize, 50 );


	/**
	 * Debugging
	 **/
	s_CfgList[IDC_VERBOSE]			= new Cfg_Bool( wxT("/eMule/Verbose"), CPreferences::s_bVerbose, false );
	s_CfgList[IDC_VERBOSEPACKETERROR]	= new Cfg_Bool( wxT("/FakeCheck/VerbosePacketError"), CPreferences::s_VerbosePacketError, false );


	/**
	 * Connection settings
	 **/
	s_CfgList[IDC_MAXUP]			=    MkCfg_Int( wxT("/eMule/MaxUpload"), CPreferences::s_maxupload, 0 );
	s_CfgList[IDC_MAXDOWN]			=    MkCfg_Int( wxT("/eMule/MaxDownload"), CPreferences::s_maxdownload, 0 );
	s_CfgList[IDC_SLOTALLOC]		=    MkCfg_Int( wxT("/eMule/SlotAllocation"), CPreferences::s_slotallocation, 2 );
	s_CfgList[IDC_PORT]				=    MkCfg_Int( wxT("/eMule/Port"), CPreferences::s_port, 4662 );
	s_CfgList[IDC_UDPPORT]			=    MkCfg_Int( wxT("/eMule/UDPPort"), CPreferences::s_udpport, 4672 );
	s_CfgList[IDC_UDPDISABLE]		= new Cfg_Bool( wxT("/eMule/UDPDisable"), CPreferences::s_UDPDisable, false );
	s_CfgList[IDC_AUTOCONNECT]		= new Cfg_Bool( wxT("/eMule/Autoconnect"), CPreferences::s_autoconnect, true );
	s_CfgList[IDC_MAXSOURCEPERFILE]	=    MkCfg_Int( wxT("/eMule/MaxSourcesPerFile"), CPreferences::s_maxsourceperfile, 300 );
	s_CfgList[IDC_MAXCON]			=    MkCfg_Int( wxT("/eMule/MaxConnections"), CPreferences::s_maxconnections, CPreferences::GetRecommendedMaxConnections() );
	s_CfgList[IDC_MAXCON5SEC]		=    MkCfg_Int( wxT("/eMule/MaxConnectionsPerFiveSeconds"), CPreferences::s_MaxConperFive, 20 );
	s_CfgList[IDC_SAFEMAXCONN]		= new Cfg_Bool( wxT("/FakeCheck/SafeMaxConn"), CPreferences::s_UseSafeMaxConn, false );


	/**
	 * Servers
	 **/ 
	s_CfgList[IDC_REMOVEDEAD]		= new Cfg_Bool( wxT("/eMule/RemoveDeadServer"), CPreferences::s_deadserver, 1 );
	s_CfgList[IDC_SERVERRETRIES]	=    MkCfg_Int( wxT("/eMule/DeadServerRetry"), CPreferences::s_deadserverretries, 2 );
	s_CfgList[IDC_SERVERKEEPALIVE]	=    MkCfg_Int( wxT("/eMule/ServerKeepAliveTimeout"), CPreferences::s_dwServerKeepAliveTimeoutMins, 0 );
	s_CfgList[IDC_RECONN]			= new Cfg_Bool( wxT("/eMule/Reconnect"), CPreferences::s_reconnect, true );
	s_CfgList[IDC_SCORE]			= new Cfg_Bool( wxT("/eMule/Scoresystem"), CPreferences::s_scorsystem, true );
	s_CfgList[IDC_AUTOSERVER]		= new Cfg_Bool( wxT("/eMule/Serverlist"), CPreferences::s_autoserverlist, false );
	s_CfgList[IDC_UPDATESERVERCONNECT]	= new Cfg_Bool( wxT("/eMule/AddServersFromServer"), CPreferences::s_addserversfromserver, true);
	s_CfgList[IDC_UPDATESERVERCLIENT]	= new Cfg_Bool( wxT("/eMule/AddServersFromClient"), CPreferences::s_addserversfromclient, true );
	s_CfgList[IDC_SAFESERVERCONNECT]	 = new Cfg_Bool( wxT("/eMule/SafeServerConnect"), CPreferences::s_safeServerConnect, false );
	s_CfgList[IDC_AUTOCONNECTSTATICONLY] = new Cfg_Bool( wxT("/eMule/AutoConnectStaticOnly"), CPreferences::s_autoconnectstaticonly, false ); 
	s_CfgList[IDC_SMARTIDCHECK]		= new Cfg_Bool( wxT("/eMule/SmartIdCheck"), CPreferences::s_smartidcheck, true );


	/**
	 * Files
	 **/
	s_CfgList[IDC_TEMPFILES]		= new Cfg_Str(  wxT("/eMule/TempDir"), CPreferences::s_tempdir,			appdir + wxT("Temp") );
	s_CfgList[IDC_INCFILES]			= new Cfg_Str(  wxT("/eMule/IncomingDir"), CPreferences::s_incomingdir,	appdir + wxT("Incoming") );
	s_CfgList[IDC_ICH]				= new Cfg_Bool( wxT("/eMule/ICH"), CPreferences::s_ICH, true );
	s_CfgList[IDC_METADATA] 		= new Cfg_Bool( wxT("/ExternalConnect/ExtractMetaDataTags"), CPreferences::s_ExtractMetaData, false );
	s_CfgList[IDC_CHUNKALLOC]		= new Cfg_Bool( wxT("/ExternalConnect/FullChunkAlloc"), CPreferences::s_AllocFullChunk, false );
	s_CfgList[IDC_FULLALLOCATE]		= new Cfg_Bool( wxT("/ExternalConnect/FullPartAlloc"), CPreferences::s_AllocFullPart, false );
	s_CfgList[IDC_CHECKDISKSPACE]	= new Cfg_Bool( wxT("/eMule/CheckDiskspace"), CPreferences::s_checkDiskspace, true );
	s_CfgList[IDC_MINDISKSPACE]		=    MkCfg_Int( wxT("/eMule/MinFreeDiskSpace"), CPreferences::s_uMinFreeDiskSpace, 1 );
	s_CfgList[IDC_ADDNEWFILESPAUSED]	= new Cfg_Bool( wxT("/eMule/AddNewFilesPaused"), CPreferences::s_addnewfilespaused, false );
	s_CfgList[IDC_PREVIEWPRIO]		= new Cfg_Bool( wxT("/eMule/PreviewPrio"), CPreferences::s_bpreviewprio, false );
	s_CfgList[IDC_MANUALSERVERHIGHPRIO]	= new Cfg_Bool( wxT("/eMule/ManualHighPrio"), CPreferences::s_bmanualhighprio, false );
	s_CfgList[IDC_FULLCHUNKTRANS] 	= new Cfg_Bool( wxT("/eMule/FullChunkTransfers"), CPreferences::s_btransferfullchunks, true );
	s_CfgList[IDC_STARTNEXTFILE]	= new Cfg_Bool( wxT("/eMule/StartNextFile"), CPreferences::s_bstartnextfile, false );
	s_CfgList[IDC_FILEBUFFERSIZE]	=    MkCfg_Int( wxT("/eMule/FileBufferSizePref"), CPreferences::s_iFileBufferSize, 16 );
	s_CfgList[IDC_DAP]				= new Cfg_Bool( wxT("/eMule/DAPPref"), CPreferences::s_bDAP, true );
	s_CfgList[IDC_UAP]				= new Cfg_Bool( wxT("/eMule/UAPPref"), CPreferences::s_bUAP, true );


	/**
	 * External Connections
	 */
	s_CfgList[IDC_OSDIR]			= new Cfg_Str(  wxT("/eMule/OSDirectory"), CPreferences::s_OSDirectory,	appdir );
	s_CfgList[IDC_ONLINESIG]		= new Cfg_Bool( wxT("/eMule/OnlineSignature"), CPreferences::s_onlineSig, false );
	s_CfgList[IDC_ENABLE_WEB]		= new Cfg_Bool( wxT("/WebServer/Enabled"), CPreferences::s_bWebEnabled, false );
	s_CfgList[IDC_WEB_PASSWD]		= new Cfg_Str_Encrypted( wxT("/WebServer/Password"), CPreferences::s_sWebPassword );
	s_CfgList[IDC_WEB_PASSWD_LOW]	= new Cfg_Str_Encrypted( wxT("/WebServer/PasswordLow"), CPreferences::s_sWebLowPassword );
	s_CfgList[IDC_WEB_PORT]			=    MkCfg_Int( wxT("/WebServer/Port"), CPreferences::s_nWebPort, 4711 );
	s_CfgList[IDC_WEB_GZIP]			= new Cfg_Bool( wxT("/WebServer/UseGzip"), CPreferences::s_bWebUseGzip, true );
	s_CfgList[IDC_ENABLE_WEB_LOW]	= new Cfg_Bool( wxT("/WebServer/UseLowRightsUser"), CPreferences::s_bWebLowEnabled, false );
	s_CfgList[IDC_WEB_REFRESH_TIMEOUT]	=    MkCfg_Int( wxT("/WebServer/PageRefreshTime"), CPreferences::s_nWebPageRefresh, 120 );
	s_CfgList[IDC_EXT_CONN_ACCEPT]	= new Cfg_Bool( wxT("/ExternalConnect/AcceptExternalConnections"), CPreferences::s_AcceptExternalConnections, true );
	s_CfgList[IDC_EXT_CONN_USETCP]	= new Cfg_Bool( wxT("/ExternalConnect/ECUseTCPPort"), CPreferences::s_ECUseTCPPort, false );
	s_CfgList[IDC_EXT_CONN_TCP_PORT]	=    MkCfg_Int( wxT("/ExternalConnect/ECPort"), CPreferences::s_ECPort, 4712 );
	s_CfgList[IDC_EXT_CONN_PASSWD]	= new Cfg_Str_Encrypted( wxT("/ExternalConnect/ECPassword"), CPreferences::s_ECPassword, wxT("") );


	/**
	 * GUI behavoir
	 **/
	s_CfgList[IDC_SPLASHON]			= new Cfg_Bool( wxT("/eMule/Splashscreen"), CPreferences::s_splashscreen, true );
	s_CfgList[IDC_MINTRAY]			= new Cfg_Bool( wxT("/eMule/MinToTray"), CPreferences::s_mintotray, false );
	s_CfgList[IDC_EXIT]				= new Cfg_Bool( wxT("/eMule/ConfirmExit"), CPreferences::s_confirmExit, false );
	s_CfgList[IDC_DBLCLICK]			= new Cfg_Bool( wxT("/eMule/TransferDoubleClick"), CPreferences::s_transferDoubleclick, true );
	s_CfgList[IDC_STARTMIN]			= new Cfg_Bool( wxT("/eMule/StartupMinimized"), CPreferences::s_startMinimized, false );


	/**
	 * GUI appearence
	 **/
	s_CfgList[IDC_3DDEPTH]			=    MkCfg_Int( wxT("/eMule/3DDepth"), CPreferences::s_depth3D, 10 );
	s_CfgList[IDC_TOOLTIPDELAY]		=    MkCfg_Int( wxT("/eMule/ToolTipDelay"), CPreferences::s_iToolDelayTime, 1 );
	s_CfgList[IDC_SHOWOVERHEAD]		= new Cfg_Bool( wxT("/eMule/ShowOverhead"), CPreferences::s_bshowoverhead, false );
	s_CfgList[IDC_EXTCATINFO]		= new Cfg_Bool( wxT("/eMule/ShowInfoOnCatTabs"), CPreferences::s_showCatTabInfos, false );
	s_CfgList[IDC_FED2KLH]			= new Cfg_Bool( wxT("/Razor_Preferences/FastED2KLinksHandler"), CPreferences::s_FastED2KLinksHandler, true );
	s_CfgList[IDC_PROGBAR]			= new Cfg_Bool( wxT("/ExternalConnect/ShowProgressBar"),	CPreferences::s_ProgBar, true );
	s_CfgList[IDC_PERCENT]			= new Cfg_Bool( wxT("/ExternalConnect/ShowPercent"), 		CPreferences::s_Percent, false );

	s_CfgList[IDC_USESKIN]			= new Cfg_Bool( wxT("/SkinGUIOptions/UseSkinFile"), CPreferences::s_UseSkinFile, false );
	s_CfgList[IDC_SKINFILE]			= new Cfg_Str(  wxT("/SkinGUIOptions/SkinFile"), CPreferences::s_SkinFile, wxT("") );


	/**
	 * External Apps
	 */
	s_CfgList[IDC_VIDEOPLAYER]		= new Cfg_Str(  wxT("/eMule/VideoPlayer"), CPreferences::s_VideoPlayer, wxT("") );
	s_CfgList[IDC_VIDEOBACKUP]		= new Cfg_Bool( wxT("/eMule/VideoPreviewBackupped"), CPreferences::s_moviePreviewBackup, true );


	/**
	 * Statistics
	 **/
	s_CfgList[IDC_SLIDER]			=    MkCfg_Int( wxT("/eMule/StatGraphsInterval"), CPreferences::s_trafficOMeterInterval, 3 );
	s_CfgList[IDC_SLIDER2]			=    MkCfg_Int( wxT("/eMule/statsInterval"), CPreferences::s_statsInterval, 30 );
	s_CfgList[IDC_DOWNLOAD_CAP]		=    MkCfg_Int( wxT("/eMule/DownloadCapacity"), CPreferences::s_maxGraphDownloadRate, 3 );
	s_CfgList[IDC_UPLOAD_CAP]		=    MkCfg_Int( wxT("/eMule/UploadCapacity"), CPreferences::s_maxGraphUploadRate, 3 );
	s_CfgList[IDC_SLIDER3] 			=    MkCfg_Int( wxT("/eMule/StatsAverageMinutes"), CPreferences::s_statsAverageMinutes, 5 );
	s_CfgList[IDC_SLIDER4]			=    MkCfg_Int( wxT("/eMule/VariousStatisticsMaxValue"), CPreferences::s_statsMax, 100 );


	/**
	 * Sources
	 **/
	s_CfgList[IDC_ENABLE_AUTO_FQS]	= new Cfg_Bool( wxT("/Razor_Preferences/FullQueueSources"), CPreferences::s_DropFullQueueSources,  false );
	s_CfgList[IDC_ENABLE_AUTO_HQRS]	= new Cfg_Bool( wxT("/Razor_Preferences/HighQueueRankingSources"), CPreferences::s_DropHighQueueRankingSources, false );
	s_CfgList[IDC_HQR_VALUE]		=    MkCfg_Int( wxT("/Razor_Preferences/HighQueueRanking"), CPreferences::s_HighQueueRanking, 1200 );
	s_CfgList[IDC_AUTO_DROP_TIMER]	=    MkCfg_Int( wxT("/Razor_Preferences/AutoDropTimer"), CPreferences::s_AutoDropTimer, 240 );
	s_CfgList[IDC_NNS_HANDLING]		=    MkCfg_Int( wxT("/Razor_Preferences/NoNeededSourcesHandling"), CPreferences::s_NoNeededSources, 2 );
	s_CfgList[IDC_SRCSEEDS]			= new Cfg_Bool( wxT("/ExternalConnect/UseSrcSeeds"),		CPreferences::s_UseSrcSeeds, false );
	 

	/**
	 * Security
	 **/
	s_CfgList[IDC_SEESHARES]		=    MkCfg_Int( wxT("/eMule/SeeShare"),					CPreferences::s_iSeeShares, 2 );
	s_CfgList[IDC_SECIDENT]			= new Cfg_Bool( wxT("/ExternalConnect/UseSecIdent"),	CPreferences::s_SecIdent, true );
	s_CfgList[IDC_IPFONOFF]			= new Cfg_Bool( wxT("/ExternalConnect/IpFilterOn"),		CPreferences::s_IPFilterOn, true );
	s_CfgList[IDC_FILTER]			= new Cfg_Bool( wxT("/eMule/FilterBadIPs"),				CPreferences::s_filterBadIP, true );






	/**
	 * The folowing doesn't have an assosiated widget.
	 **/


	/* window colum widths, no dialog interaction - BEGIN */
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnWidths"),		CPreferences::s_downloadColumnWidths,	ELEMENT_COUNT(CPreferences::s_downloadColumnWidths),		DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnHidden"),		CPreferences::s_downloadColumnHidden,	ELEMENT_COUNT(CPreferences::s_downloadColumnHidden),		0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/DownloadColumnOrder"),		CPreferences::s_downloadColumnOrder,	ELEMENT_COUNT(CPreferences::s_downloadColumnOrder),		0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnWidths"),		CPreferences::s_uploadColumnWidths,	ELEMENT_COUNT(CPreferences::s_uploadColumnWidths), 		DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnHidden"),		CPreferences::s_uploadColumnHidden,	ELEMENT_COUNT(CPreferences::s_uploadColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/UploadColumnOrder"),			CPreferences::s_uploadColumnOrder,		ELEMENT_COUNT(CPreferences::s_uploadColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnWidths"),			CPreferences::s_queueColumnWidths,		ELEMENT_COUNT(CPreferences::s_queueColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnHidden"),			CPreferences::s_queueColumnHidden,		ELEMENT_COUNT(CPreferences::s_queueColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/QueueColumnOrder"),			CPreferences::s_queueColumnOrder,		ELEMENT_COUNT(CPreferences::s_queueColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnWidths"),		CPreferences::s_searchColumnWidths,	ELEMENT_COUNT(CPreferences::s_searchColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnHidden"),		CPreferences::s_searchColumnHidden,	ELEMENT_COUNT(CPreferences::s_searchColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SearchColumnOrder"),			CPreferences::s_searchColumnOrder,		ELEMENT_COUNT(CPreferences::s_searchColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnWidths"),		CPreferences::s_sharedColumnWidths,	ELEMENT_COUNT(CPreferences::s_sharedColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnHidden"),		CPreferences::s_sharedColumnHidden,	ELEMENT_COUNT(CPreferences::s_sharedColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/SharedColumnOrder"),			CPreferences::s_sharedColumnOrder,		ELEMENT_COUNT(CPreferences::s_sharedColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnWidths"),		CPreferences::s_serverColumnWidths,	ELEMENT_COUNT(CPreferences::s_serverColumnWidths),			DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnHidden"),		CPreferences::s_serverColumnHidden,	ELEMENT_COUNT(CPreferences::s_serverColumnHidden),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ServerColumnOrder"),			CPreferences::s_serverColumnOrder,		ELEMENT_COUNT(CPreferences::s_serverColumnOrder),			0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnWidths"),	CPreferences::s_clientListColumnWidths, ELEMENT_COUNT(CPreferences::s_clientListColumnWidths),	DEFAULT_COL_SIZE ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnHidden"),	CPreferences::s_clientListColumnHidden, ELEMENT_COUNT(CPreferences::s_clientListColumnHidden), 	0 ) );
	s_MiscList.push_back( new Cfg_Columns( wxT("/eMule/ClientListColumnOrder"),		CPreferences::s_clientListColumnOrder,	ELEMENT_COUNT(CPreferences::s_clientListColumnOrder),		0 ) );
	/*  window colum widths - END */

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemDownload"),		CPreferences::s_tableSortItemDownload,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemUpload"),			CPreferences::s_tableSortItemUpload,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemQueue"),			CPreferences::s_tableSortItemQueue,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemSearch"),			CPreferences::s_tableSortItemSearch,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemShared"),			CPreferences::s_tableSortItemShared,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemServer"),			CPreferences::s_tableSortItemServer,			0 ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/TableSortItemClientList"),		CPreferences::s_tableSortItemClientList,		0 ) );
	
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingDownload"),	CPreferences::s_tableSortAscendingDownload,	true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingUpload"),		CPreferences::s_tableSortAscendingUpload,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingQueue"),		CPreferences::s_tableSortAscendingQueue,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingSearch"),		CPreferences::s_tableSortAscendingSearch,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingShared"),		CPreferences::s_tableSortAscendingShared,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingServer"),		CPreferences::s_tableSortAscendingServer,		true ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/TableSortAscendingClientList"),	CPreferences::s_tableSortAscendingClientList,	true ) );


	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalDownloadedBytes"), CPreferences::s_totalDownloadedBytes ) );
	s_MiscList.push_back( new Cfg_Counter( wxT("/Statistics/TotalUploadedBytes"),	CPreferences::s_totalUploadedBytes ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/SplitterbarPosition"),			CPreferences::s_splitterbarPosition, 75 ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/FilterServersByIP"),			CPreferences::s_filterserverbyip, false ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/FilterLevel"),					CPreferences::s_filterlevel, 127 ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/YourHostname"),					CPreferences::s_yourHostname, wxT("") ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/DateTimeFormat"),				CPreferences::s_datetimeformat, wxT("%A, %x, %X") ) );
	

	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/IndicateRatings"),				CPreferences::s_indicateratings, true ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/AllcatType"),					CPreferences::s_allcatType, 0 ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/ShowAllNotCats"),				CPreferences::s_showAllNotCats, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/ResumeNextFromSameCat"),		CPreferences::s_resumeSameCat, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DontRecreateStatGraphsOnResize"),	CPreferences::s_resumeSameCat, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DisableKnownClientList"),		CPreferences::s_bDisableKnownClientList, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/DisableQueueList"),				CPreferences::s_bDisableQueueList, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/MessagesFromFriendsOnly"),		CPreferences::s_msgonlyfriends, false ) );
	s_MiscList.push_back( new Cfg_Bool( wxT("/eMule/MessageFromValidSourcesOnly"),	CPreferences::s_msgsecure, true ) );
	s_MiscList.push_back(    MkCfg_Int( wxT("/eMule/MaxMessageSessions"),			CPreferences::s_maxmsgsessions, 50 ) );
	s_MiscList.push_back( new Cfg_Str(  wxT("/eMule/WebTemplateFile"),				CPreferences::s_sTemplateFile, wxT("eMule.tmpl") ) );


	s_MiscList.push_back(   MkCfg_Int( wxT("/Statistics/DesktopMode"), CPreferences::s_desktopMode, 4 ) );

	// Colors have been moved from global prefs to CStatisticsDlg
	for ( int i = 0; i < cntStatColors; i++ ) {  
		wxString str = wxString::Format(wxT("/eMule/StatColor%i"),i);
		
		s_MiscList.push_back( MkCfg_Int( str, CStatisticsDlg::acrStat[i], CStatisticsDlg::acrStat[i] ) );
	}


// These options are currently not used 
#if 0
	////// Notify
	s_CfgList[IDC_CB_TBN_USESOUND]	= new Cfg_Bool( wxT("/eMule/NotifierUseSound"), CPreferences::s_useSoundInNotifier, false );
	s_CfgList[IDC_CB_TBN_ONLOG]		= new Cfg_Bool( wxT("/eMule/NotifyOnLog"), CPreferences::s_useLogNotifier, false );
	s_CfgList[IDC_CB_TBN_ONCHAT]	= new Cfg_Bool( wxT("/eMule/NotifyOnChat"), CPreferences::s_useChatNotifier, false );
	s_CfgList[IDC_CB_TBN_POP_ALWAYS]	= new Cfg_Bool( wxT("/eMule/NotifierPopEveryChatMessage"), CPreferences::s_notifierPopsEveryChatMsg, false );
	s_CfgList[IDC_CB_TBN_ONDOWNLOAD]	= new Cfg_Bool( wxT("/eMule/NotifyOnDownload"), CPreferences::s_useDownloadNotifier, false );
	s_CfgList[IDC_CB_TBN_ONNEWVERSION]	= new Cfg_Bool( wxT("/eMule/NotifierPopNewVersion"), CPreferences::s_notifierNewVersion, false );
	s_CfgList[IDC_CB_TBN_IMPORTATNT]	= new Cfg_Bool( wxT("/eMule/NotifyOnImportantError"), CPreferences::s_notifierImportantError, false );
	s_CfgList[IDC_SENDMAIL]			= new Cfg_Bool( wxT("/eMule/NotifyByMail"), CPreferences::s_sendEmailNotifier, false );
	s_CfgList[IDC_EDIT_TBN_WAVFILE]	= new Cfg_Str(  wxT("/eMule/NotifierSoundPath"), CPreferences::s_notifierSoundFilePath, wxT("") );
#endif
}







BEGIN_EVENT_TABLE(PrefsUnifiedDlg,wxDialog)
	EVT_CHECKBOX(IDC_UDPDISABLE, PrefsUnifiedDlg::OnCheckBoxChange)
	
	EVT_BUTTON(ID_PREFS_OK_TOP, PrefsUnifiedDlg::OnOk)
	EVT_BUTTON(ID_OK, PrefsUnifiedDlg::OnOk)
	
	EVT_BUTTON(ID_PREFS_CANCEL_TOP, PrefsUnifiedDlg::OnCancel)
	EVT_BUTTON(ID_CANCEL, PrefsUnifiedDlg::OnCancel)
	
	// Browse buttons
	EVT_BUTTON(IDC_SELSKINFILE,  PrefsUnifiedDlg::OnButtonBrowseSkin)
	EVT_BUTTON(IDC_BTN_BROWSE_WAV, PrefsUnifiedDlg::OnButtonBrowseWav)
	EVT_BUTTON(IDC_BROWSEV, PrefsUnifiedDlg::OnButtonBrowseVideoplayer)
	EVT_BUTTON(IDC_SELTEMPDIR, PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELINCDIR,  PrefsUnifiedDlg::OnButtonDir)
	EVT_BUTTON(IDC_SELOSDIR,  PrefsUnifiedDlg::OnButtonDir)
	
	EVT_SPINCTRL( IDC_TOOLTIPDELAY, PrefsUnifiedDlg::OnToolTipDelayChange)

	EVT_BUTTON(IDC_EDITADR, PrefsUnifiedDlg::OnButtonEditAddr)
	EVT_BUTTON(ID_DESKTOPMODE, PrefsUnifiedDlg::OnButtonSystray)
	EVT_BUTTON(IDC_IPFRELOAD, PrefsUnifiedDlg::OnButtonIPFilterReload)
	EVT_BUTTON(IDC_COLOR_BUTTON, PrefsUnifiedDlg::OnButtonColorChange)
	EVT_CHOICE(IDC_COLORSELECTOR, PrefsUnifiedDlg::OnColorCategorySelected)
	EVT_CHOICE(IDC_FCHECK, PrefsUnifiedDlg::OnFakeBrowserChange)
	EVT_LIST_ITEM_SELECTED(ID_PREFSLISTCTRL, PrefsUnifiedDlg::OnPrefsPageChange)

    EVT_INIT_DIALOG(PrefsUnifiedDlg::OnInitDialog)	
END_EVENT_TABLE()



/**
 * This struct provides a general way to represent config-tabs.
 */
struct PrefsPage
{
	//! The title of the page, used on the listctrl.
	wxString	m_title;
	//! Function pointer to the wxDesigner function creating the dialog.
	wxSizer*	(*m_function)(wxWindow*, bool = TRUE, bool = TRUE );
	//! The index of the image used on the list.
	int 		m_imageidx;
	//! The actual widget, to be set later.
	wxPanel*	m_widget;
};



PrefsPage	pages[] =
{
	{ _("General"),				PreferencesGeneralTab,			13,	NULL },
	{ _("Connection"),			PreferencesConnectionTab,		14, NULL },
	{ _("Remote Controls"),		PreferencesRemoteControlsTab,	11, NULL },
	{ _("Online Signature"),	PreferencesOnlineSigTab,		0,	NULL },
	{ _("Server"),				PreferencesServerTab,			15, NULL },
	{ _("Files"),				PreferencesFilesTab,			16, NULL },
	{ _("Sources Dropping"),	PreferencesSourcesDroppingTab,	20, NULL },
	{ _("Directories"),			PreferencesDirectoriesTab,		17, NULL },
	{ _("Statistics"),			PreferencesStatisticsTab,		10, NULL },
	{ _("Security"),			PreferencesSecurityTab,			0,	NULL },

//	Notications are disabled since they havent been implemented
//	{ _("Notifications"),		PreferencesNotifyTab,			18, NULL },
	{ _("Core Tweaks"),			PreferencesaMuleTweaksTab,		12, NULL },
	{ _("Gui Tweaks"),			PreferencesGuiTweaksTab,		19, NULL }
};


PrefsUnifiedDlg::PrefsUnifiedDlg(wxWindow* parent)
	: wxDialog(parent, -1, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	SetPrefsID( GetId() );

	preferencesDlgTop( this, FALSE );
	wxListCtrl* PrefsIcons = (wxListCtrl*) FindWindowById(ID_PREFSLISTCTRL,this);
	
	PrefsIcons->SetSize(wxSize(150,-1));
	
	wxImageList* icon_list = new wxImageList(16, 16);
	PrefsIcons->AssignImageList( icon_list, wxIMAGE_LIST_SMALL);

	// Add the single column used
	PrefsIcons->InsertColumn(0, wxT(""), wxLIST_FORMAT_LEFT, PrefsIcons->GetSize().GetWidth()-5);


	// Temp variables for finding the smallest height and width needed 
	int width = 0;
	int height = 0;
	
	// Create and add each page
	for ( unsigned int i = 0; i < ELEMENT_COUNT(pages); i++ ) {
		// Add the icon and label assosiated with the page
		icon_list->Add( amuleSpecial(pages[i].m_imageidx) );
		PrefsIcons->InsertItem(i, pages[i].m_title, i);
		
		// Create a container widget and the contents of the page 
		pages[i].m_widget = new wxPanel( this, -1 );	
		pages[i].m_function( pages[i].m_widget, true );

		// Add it to the sizer
		prefs_sizer->Add( pages[i].m_widget, 0, wxGROW|wxEXPAND );

		// Align and resize the page
		Fit();
		Layout();

		
		// Find the greatest sizes
		wxSize size = prefs_sizer->GetSize();
		if ( size.GetWidth() > width ) 
			width = size.GetWidth();
	
		if ( size.GetHeight() > height )
			height = size.GetHeight();


		// Hide it for now
		prefs_sizer->Remove( pages[i].m_widget );
		pages[i].m_widget->Show( false );
	}

	// Default to the General tab
	m_CurrentPanel = pages[0].m_widget;
	prefs_sizer->Add( pages[0].m_widget, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );

	// We now have the needed minimum height and width
	prefs_sizer->SetMinSize( width, height );


	// Store some often used pointers
	m_ShareSelector = ((CDirectoryTreeCtrl*)FindWindowById(IDC_SHARESELECTOR, this));
	m_buttonColor = (wxButton*)FindWindowById(IDC_COLOR_BUTTON, this);
	m_choiceColor = (wxChoice*)FindWindowById(IDC_COLORSELECTOR, this);


	// Connect the Cfgs with their widgets
	CFGMap::iterator it = s_CfgList.begin();
	for ( ; it != s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->ConnectToWidget( it->first, this ) ) {
			printf("Failed to connect Cfg to widget with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));
		}
	}

	Fit();

	// Place the window centrally
	CentreOnScreen();
}


PrefsUnifiedDlg::~PrefsUnifiedDlg()
{
	// Un-Connect the Cfgs
	CFGMap::iterator it = s_CfgList.begin();
	for ( ; it != s_CfgList.end(); ++it ) {
		// Checking for failures
		it->second->ConnectToWidget( 0 );
	}
}


Cfg_Base* PrefsUnifiedDlg::GetCfg(int id)
{
	CFGMap::iterator it = s_CfgList.find( id );

	if ( it != s_CfgList.end() )
		return it->second;

	return NULL;
}	


bool PrefsUnifiedDlg::TransferToWindow()
{
	// Connect the Cfgs with their widgets
	CFGMap::iterator it = s_CfgList.begin();
	for ( ; it != s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferToWindow() ) {
			printf("Failed to transfer data from Cfg to Widget with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));
		}
	}
	
	m_ShareSelector->SetSharedDirectories(&theApp.glob_prefs->shareddir_list);
	
	
	for ( int i = 0; i < cntStatColors; i++ ) {
		s_colors[i] = CStatisticsDlg::acrStat[i];
		s_colors_ref[i] = CStatisticsDlg::acrStat[i];
	}


	// Enable/Disable some controls
	FindWindow( IDC_FCHECKSELF )->Enable( ((wxChoice*)FindWindow( IDC_FCHECK ))->GetSelection() == 8 );
	
    return true;
}


bool PrefsUnifiedDlg::TransferFromWindow()
{
	// Connect the Cfgs with their widgets
	CFGMap::iterator it = s_CfgList.begin();
	for ( ; it != s_CfgList.end(); ++it ) {
		// Checking for failures
		if ( !it->second->TransferFromWindow() ) {
			printf("Failed to transfer data from Widget to Cfg with the ID %d and key %s\n", it->first, unicode2char(it->second->GetKey()));		 
		}
	}

	theApp.glob_prefs->shareddir_list.Clear();
	m_ShareSelector->GetSharedDirectories(&theApp.glob_prefs->shareddir_list);

	for ( int i = 0; i < cntStatColors; i++ ) {
		if ( s_colors[i] != s_colors_ref[i] ) {
			CStatisticsDlg::acrStat[i] = s_colors[i];

			theApp.amuledlg->statisticswnd->ApplyStatsColor(i);	
		}
			
	}

	return true;
}


bool PrefsUnifiedDlg::CfgChanged(int ID)
{
	Cfg_Base* cfg = GetCfg(ID);

	if ( cfg )
		return cfg->HasChanged();

	return false;
}


void PrefsUnifiedDlg::OnOk(wxCommandEvent& WXUNUSED(event))
{
	TransferFromWindow();
	
	// do sanity checking, special processing, and user notifications here
	theApp.glob_prefs->CheckUlDlRatio();
	
	// save the preferences on ok
	theApp.glob_prefs->Save();
	
	
	if ( CfgChanged(IDC_FED2KLH) ) 
		theApp.amuledlg->ToggleFastED2KLinksHandler();
	
	
	if ( CfgChanged(IDC_LANGUAGE) )
		wxMessageBox(wxString::wxString(_("Language change will not be applied until aMule is restarted.")));


	if ( CfgChanged(IDC_INCFILES) || CfgChanged(IDC_TEMPFILES) || m_ShareSelector->HasChanged )
		theApp.sharedfiles->Reload(true, false);


	if ( CfgChanged(IDC_PERCENT) || CfgChanged(IDC_PROGBAR) ) {		
		// Force upload of the donwload queue 
		theApp.downloadqueue->UpdateDisplayedInfo( true );
	}

	if ( CfgChanged(IDC_OSDIR) ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_OSDIR );
	
		// Build the filenames for the two OS files
		theApp.SetOSFiles( widget->GetValue() );
	}

	
	if ( theApp.glob_prefs->GetIPFilterOn() )
		theApp.clientlist->FilterQueues();
	
	
	// Final actions:
	// Reset the ID so that a new dialog can be created
	SetPrefsID( 0 );

	// Hide the dialog since Destroy isn't instant
	Show( false );

	// Destory the dialog
	Destroy();
}


void PrefsUnifiedDlg::OnCancel(wxCommandEvent& WXUNUSED(event))
{
	// Final actions:
	// Reset the ID so that a new dialog can be created
	SetPrefsID( 0 );

	// Hide the dialog since Destroy isn't instant
	Show( false );

	// Destory the dialog
	Destroy();
}


void PrefsUnifiedDlg::OnCheckBoxChange(wxCommandEvent& event)
{
	bool		value = event.IsChecked();
	wxWindow*	widget = NULL;

	widget = FindWindow( IDC_UDPPORT );
	if ( widget ) 
		widget->Enable( !value );
}


void PrefsUnifiedDlg::OnButtonColorChange(wxCommandEvent& WXUNUSED(event))
{
	int index = m_choiceColor->GetSelection();
	wxColour col = WxColourFromCr( s_colors[index] );
	col = wxGetColourFromUser( this, col );
	if ( col.Ok() ) {
		m_buttonColor->SetBackgroundColour( col );
		s_colors[index] = CrFromWxColour(col);
	}
}


void PrefsUnifiedDlg::OnColorCategorySelected(wxCommandEvent& WXUNUSED(evt))
{
	m_buttonColor->SetBackgroundColour( WxColourFromCr( s_colors[ m_choiceColor->GetSelection() ] ) );
}


void PrefsUnifiedDlg::OnFakeBrowserChange( wxCommandEvent& evt )
{
	wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_FCHECKSELF );

	if ( widget )
		widget->Enable( evt.GetSelection() == 8 );
}


void PrefsUnifiedDlg::OnButtonSystray(wxCommandEvent& WXUNUSED(evt))
{
	theApp.amuledlg->changeDesktopMode();
	
	// Ensure that the dialog is still visible afterwards
	Raise();
	SetFocus();
}


void PrefsUnifiedDlg::OnButtonDir(wxCommandEvent& event)
{
	wxString type = _("Choose a folder for ");
	
	int id = 0;
	switch ( event.GetId() ) {
		case IDC_SELTEMPDIR:
			id = IDC_TEMPFILES;
			type += _("Temporary files");			
			break;
			
		case IDC_SELINCDIR:
			id = IDC_INCFILES;
			type += _("Incomming files");			
			break;
			
		case IDC_SELOSDIR:
			id = IDC_OSDIR;
			type += _("Online Signatures");			
			break;
		
		default:
			wxASSERT( false );
			return;
	}

	wxTextCtrl* widget	= (wxTextCtrl*)FindWindow( id );
	wxString dir		= widget->GetValue();
	
	wxString str = wxDirSelector( type, dir );

	if ( !str.IsEmpty() ) {
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseWav(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( _("Browse wav"), wxT(""), wxT(""), wxT("*.wav"), _("File wav (*.wav)|*.wav||") );
	
	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_EDIT_TBN_WAVFILE );
		
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseSkin(wxCommandEvent& WXUNUSED(evt))
{
	wxString str = wxFileSelector( _("Browse skin file"), wxT(""), wxT(""), wxT("*") );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_SKINFILE );
		
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonBrowseVideoplayer(wxCommandEvent& WXUNUSED(e))
{
	wxString str = wxFileSelector( _("Browse for videoplayer"), wxT(""), wxT(""), wxT(""), _("Executable (*)|*||") );

	if ( !str.IsEmpty() ) {
		wxTextCtrl* widget = (wxTextCtrl*)FindWindow( IDC_VIDEOPLAYER );
		
		widget->SetValue( str );
	}
}


void PrefsUnifiedDlg::OnButtonEditAddr(wxCommandEvent& WXUNUSED(evt))
{
	wxString fullpath( theApp.ConfigDir + wxT("addresses.dat") );
	
	EditServerListDlg* test = new EditServerListDlg(this, _("Edit Serverlist"), _("Add here URL's to download server.met files.\nOnly one url on each line."), fullpath );
	
	test->ShowModal();
  
	delete test;
}


void PrefsUnifiedDlg::OnButtonIPFilterReload(wxCommandEvent& WXUNUSED(event))
{
	theApp.ipfilter->Reload();
}	


void PrefsUnifiedDlg::LoadAllItems(wxConfigBase* cfg)
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
	theApp.glob_prefs->CheckUlDlRatio();
}


void PrefsUnifiedDlg::SaveAllItems(wxConfigBase* cfg)
{
	// Connect the Cfgs with their widgets
	CFGMap::iterator it_a = s_CfgList.begin();
	for ( ; it_a != s_CfgList.end(); ++it_a )
		it_a->second->SaveToFile( cfg );


	CFGList::iterator it_b = s_MiscList.begin();
	for ( ; it_b != s_MiscList.end(); ++it_b )
		(*it_b)->SaveToFile( cfg ); 
}


void PrefsUnifiedDlg::OnPrefsPageChange(wxListEvent& event)
{
	prefs_sizer->Remove( m_CurrentPanel );
	m_CurrentPanel->Show( false );
	
	m_CurrentPanel = pages[ event.GetIndex() ].m_widget;
	
	prefs_sizer->Add( m_CurrentPanel, 0, wxGROW|wxEXPAND );
	m_CurrentPanel->Show( true );
	
	Layout();
}

void PrefsUnifiedDlg::OnToolTipDelayChange(wxSpinEvent& event)
{
	#ifdef __WXGTK__
		wxToolTip::SetDelay( event.GetPosition() * 1000 );
	#else
		#warning NO TOOLTIPS FOR NON-GTK!
	#endif
}


void PrefsUnifiedDlg::OnInitDialog( wxInitDialogEvent& WXUNUSED(evt) )
{
}
