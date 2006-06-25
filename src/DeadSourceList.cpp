//
// This file is part of the aMule Project.
//
// Copyright (C) 2005-2006aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004 Merkur ( devs@users.sourceforge.net / http://www.emule-project.net )
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

#include "DeadSourceList.h"

#include <include/common/Macros.h>

#include "updownclient.h"

#define	CLEANUPTIME			MIN2MS(60)

#define BLOCKTIME		(::GetTickCount() + (m_bGlobalList ? MIN2MS(30) : MIN2MS(45)))
#define BLOCKTIMEFW		(::GetTickCount() + (m_bGlobalList ? MIN2MS(45) : MIN2MS(60)))

///////////////////////////////////////////////////////////////////////////////////////
//// CDeadSource


CDeadSourceList::CDeadSource::CDeadSource(uint32 ID, uint16 Port, uint32 ServerIP, uint16 KadPort)
{
	m_ID = ID;
	m_Port = Port;
	m_KadPort = KadPort;
	m_ServerIP = ServerIP;
	m_TimeStamp = 0;
}


void CDeadSourceList::CDeadSource::SetTimeout( uint32 t )
{
	m_TimeStamp = t;
}


uint32 CDeadSourceList::CDeadSource::GetTimeout() const
{
	return m_TimeStamp;
}


bool CDeadSourceList::CDeadSource::operator==(const CDeadSource& other) const
{
	if ( m_ID == other.m_ID ) {
		if ( m_Port == other.m_Port || m_KadPort == other.m_KadPort ) {
			if ( IsLowID( m_ID ) ) {
				return m_ServerIP == other.m_ServerIP;
			} else {
				return true;
			}
		}
	}

	return false;
}


///////////////////////////////////////////////////////////////////////////////////////
//// CDeadSourceList

CDeadSourceList::CDeadSourceList(bool isGlobal)
{
	m_dwLastCleanUp = ::GetTickCount();
	m_bGlobalList = isGlobal;
}


uint32 CDeadSourceList::GetDeadSourcesCount() const
{
	return m_sources.size();
}


bool CDeadSourceList::IsDeadSource(const CUpDownClient* client)
{
	CDeadSource source(
		client->GetUserIDHybrid(),
		client->GetUserPort(),
		client->GetServerIP(),
		client->GetKadPort()
	);
	
	
	DeadSourcePair range = m_sources.equal_range( client->GetUserIDHybrid() );
	for ( ; range.first != range.second; range.first++ ) {
		if ( range.first->second == source ) {
			// Check if the entry is still valid
			if ( range.first->second.GetTimeout() > GetTickCount() ) {
				return true;
			} 

			// The source is no longer dead, so remove it to reduce the size of the list
			m_sources.erase( range.first );
			break;
		}
	}

	return false;
}


void CDeadSourceList::AddDeadSource( const CUpDownClient* client )
{
	CDeadSource source(
		client->GetUserIDHybrid(),
		client->GetUserPort(),
		client->GetServerIP(),
		client->GetKadPort()
	);

	// Set the timeout for the new source
	source.SetTimeout( client->HasLowID() ? BLOCKTIMEFW : BLOCKTIME );
	
	// Check if the source is already listed
	DeadSourcePair range = m_sources.equal_range( client->GetUserIDHybrid() );
	for ( ; range.first != range.second; range.first++ ) {
		if ( range.first->second == source ) {
			range.first->second = source;			
			return;
		}
	}

	m_sources.insert( DeadSourceMap::value_type( client->GetUserIDHybrid(), source ) );

	// Check if we should cleanup the list. This is
	// done to avoid a buildup of stale entries.
	if ( GetTickCount() - m_dwLastCleanUp > CLEANUPTIME ) {
		CleanUp();
	}
}


void CDeadSourceList::CleanUp()
{
	m_dwLastCleanUp = ::GetTickCount();

	DeadSourceIterator it = m_sources.begin();
	for ( ; it != m_sources.end(); ) {
		if ( it->second.GetTimeout() < m_dwLastCleanUp ) {
			m_sources.erase( it++ );
		} else {
			++it;
		}
	}
}
// File_checked_for_headers
