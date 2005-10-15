//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         OnLineSig Class
///
/// Purpose:      Monitor aMule Online Statistics by reading amulesig.dat file
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
///
/// Pixmats from aMule http://www.amule.org
///
/// This program is free software; you can redistribute it and/or modify
///  it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
 #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
 #include "wx/wx.h"
#endif

#include "onlinesig.h"
#include "wxcascte.h"

#include <wx/txtstrm.h>
#include <wx/wfstream.h>

// Constructor
OnLineSig::OnLineSig ( const wxFileName& file,
                       const double absoluteMaxDL,
                       const wxDateTime absoluteMaxDlDate )
{
	m_amulesig = file;

	m_sessionMaxDL = 0.0;
	m_sessionMaxDLDate = wxDateTime::Now();

	m_absoluteMaxDL = absoluteMaxDL;
	m_absoluteMaxDlDate = absoluteMaxDlDate;
	
	m_isSessionMaxDlReseted = false;
	m_isAbsoluteMaxDlReseted = false;

	Refresh ();
}

// Destructor
OnLineSig::~OnLineSig ()
{}

// Accessors
void
OnLineSig::SetAmuleSig ( const wxFileName& file )
{
	m_amulesig = file;
	Refresh ();
}

void
OnLineSig::Refresh ()
{
	wxFileInputStream input ( m_amulesig.GetFullPath () );

	wxTextInputStream text ( input );
	text.SetStringSeparators ( wxT( "\n" ) );

	text >> m_amuleState;
	text >> m_serverName;
	text >> m_serverIP;
	text >> m_serverPort;
	text >> m_connexionID;
	text >> m_DLRate;
	text >> m_ULRate;
	text >> m_queue;
	text >> m_sharedFiles;
	text >> m_user;
	text >> m_totalDL;
	text >> m_totalUL;
	text >> m_version;
	text >> m_sessionDL;
	text >> m_sessionUL;
	text >> m_runTimeS;

	double dl;
	m_DLRate.ToDouble ( &dl );

	if ( dl > m_sessionMaxDL || m_isSessionMaxDlReseted ) {
		m_sessionMaxDL = dl;
		m_sessionMaxDLDate = wxDateTime::Now();
		m_isSessionMaxDlChanged = true;
		m_isSessionMaxDlReseted = false;
	}
	else {
		m_isSessionMaxDlChanged = false;
	}

	if ( dl > m_absoluteMaxDL || m_isAbsoluteMaxDlReseted ) {
		m_absoluteMaxDL = dl;
		m_absoluteMaxDlDate = wxDateTime::Now();
		m_isAbsoluteMaxDlChanged = true;
		m_isAbsoluteMaxDlReseted = false;
	}
	else {
		m_isAbsoluteMaxDlChanged = false;
	}
}

int OnLineSig::GetAmuleState() const
{
	if ( m_amuleState >= 0 && m_amuleState <= 2 ) {
		return ( m_amuleState );
	} else {
		return ( -1 );
	}
}

wxString OnLineSig::GetServerName () const
{
	return m_serverName;
}

wxString OnLineSig::GetServerIP () const
{
	return m_serverIP;
}

wxString OnLineSig::GetServerPort () const
{
	return m_serverPort;
}

wxString OnLineSig::GetConnexionID () const
{
	return m_connexionID;
}

wxString OnLineSig::GetULRate () const
{
	return m_ULRate;
}

wxString OnLineSig::GetDLRate () const
{
	return m_DLRate;
}

wxString OnLineSig::GetQueue () const
{
	return m_queue;
}

wxString OnLineSig::GetSharedFiles () const
{
	return m_sharedFiles;
}

wxString OnLineSig::GetUser () const
{
	return m_user;
}

wxString OnLineSig::GetTotalUL () const
{
	return m_totalUL;
}


wxString OnLineSig::GetTotalDL () const
{
	return m_totalDL;
}

wxString OnLineSig::GetVersion () const
{
	return m_version;
}

wxString OnLineSig::GetSessionUL () const
{
	return m_sessionUL;
}

wxString OnLineSig::GetSessionDL () const
{
	return m_sessionDL;
}

wxString OnLineSig::GetRunTime ()
{
	unsigned int seconds = m_runTimeS;
	unsigned int days = PullCount( &seconds, 86400 );
	unsigned int hours = PullCount( &seconds, 3600 );
	unsigned int minutes = PullCount( &seconds, 60 );

	if ( days > 0 ) {
		return ( wxString::Format ( _( "%02uD %02uh %02umin %02us" ), days, hours, minutes, seconds ) );
	} else if ( hours > 0 ) {
		return ( wxString::Format ( _( "%02uh %02umin %02us" ), hours, minutes, seconds ) );
	} else if ( minutes > 0 ) {
		return ( wxString::Format ( _( "%02umin %02us" ), minutes, seconds ) );
	} else {
		return ( wxString::Format ( _( "%02us" ), seconds ) );
	}
}

wxString OnLineSig::GetConvertedTotalUL ()
{
	return ( BytesConvertion ( m_totalUL ) );
}

wxString OnLineSig::GetConvertedTotalDL ()
{
	return ( BytesConvertion ( m_totalDL ) );
}

wxString OnLineSig::GetConvertedSessionUL ()
{
	return ( BytesConvertion ( m_sessionUL ) );
}

wxString OnLineSig::GetConvertedSessionDL ()
{
	return ( BytesConvertion ( m_sessionDL ) );
}

wxString OnLineSig::GetConnexionIDType () const
{
	if ( m_connexionID == wxT( "H" ) ) {
		return ( wxString ( _( "HighID" ) ) );
	} else {
		return ( wxString ( _( "LowID" ) ) );
	}
}

double OnLineSig::GetSessionMaxDL () const
{
	return ( m_sessionMaxDL );
}

wxDateTime OnLineSig::GetSessionMaxDlDate () const
{
	return ( m_sessionMaxDLDate );
}

void OnLineSig::ResetSessionMaxDL ()
{
	m_sessionMaxDL = 0.0;
	m_sessionMaxDLDate = wxDateTime::Now();
	m_isSessionMaxDlReseted = true;
}

bool OnLineSig::IsSessionMaxDlChanged() const
{
	return ( m_isSessionMaxDlChanged );
}

double OnLineSig::GetAbsoluteMaxDL () const
{
	return ( m_absoluteMaxDL );
}

wxDateTime OnLineSig::GetAbsoluteMaxDlDate () const
{
	return ( m_absoluteMaxDlDate );
}

void OnLineSig::ResetAbsoluteMaxDL ()
{
	m_absoluteMaxDL = 0.0;
	m_absoluteMaxDlDate = wxDateTime::Now();
	m_isAbsoluteMaxDlReseted = true;
}

bool OnLineSig::IsAbsoluteMaxDlChanged() const
{
	return ( m_isAbsoluteMaxDlChanged );
}

// Private use
wxString OnLineSig::BytesConvertion ( const wxString & bytes )
{
	double
	d_bytes;
	wxString
	c_bytes;

	bytes.ToDouble ( &d_bytes );

	int
	i = 0;
	while ( d_bytes > 1024 ) {
		d_bytes /= 1024;
		i++;
	}

	switch ( i ) {
	case 0:
		c_bytes = wxString::Format ( _( "%.0f B" ), d_bytes );
		break;
	case 1:
		c_bytes = wxString::Format ( _( "%.2f KB" ), d_bytes );
		break;
	case 2:
		c_bytes = wxString::Format ( _( "%.2f MB" ), d_bytes );
		break;
	case 3:
		c_bytes = wxString::Format ( _( "%.2f GB" ), d_bytes );
		break;
	default:
		c_bytes = wxString::Format ( _( "%.2f TB" ), d_bytes );
		break;
	}
	return c_bytes;
}

unsigned int OnLineSig::PullCount ( unsigned int *runtime, const unsigned int count )
{
	unsigned int answer = *runtime / count;
	*runtime -= answer * count;
	return answer;
}
