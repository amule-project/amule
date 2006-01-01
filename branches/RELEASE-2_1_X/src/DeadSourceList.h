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

#ifndef DEADSOURCELIST_H
#define DEADSOURCELIST_H


#include <map>

#include "Types.h"


class CUpDownClient;


/**
 * This class keeps track of "invalid" sources.
 *
 * A dead source is a source that has been evaluated as being useles
 * which can be due to serveral reasons, such as not responding to
 * queries. This list then allows for those sources to be ignored
 * for an set ammount of time in order to avoid the overhead of
 * trying to connect to them.
 *
 * This is important, since these sources would be removed and readded
 * repeatedly, causing extra overhead with no gain.
 */
class CDeadSourceList 
{
public:
	/**
	 * Constructor.
	 * 
	 * @param isGlobal Specifies if the list is global or not, used for debugging.
	 */
	CDeadSourceList(bool isGlobal = false);

	
	/**
	 * Adds a client to the list of dead sources.
	 */
	void		AddDeadSource(const CUpDownClient* client);

	/**
	 * Returns true if the client object is a dead source.
	 */
	bool		IsDeadSource(const CUpDownClient* client);
	

	/**
	 * Returns the number of sources.
	 */
	uint32		GetDeadSourcesCount() const;

private:
	/**
	 * Removes too old entries from the list.
	 */
	void		CleanUp();


	/**
	 * Record of dead source.
	 */
	class CDeadSource
	{
	public:
		/**
		 * Constructor.
		 *
		 * @param ID The IP/ID of the recorded client.
		 * @param Port The TCP port of the recorded client.
		 * @param ServerIP The ip of the connected server.
		 * @param KadPort The Kad port used by the client.
		 *
		 * Notes: 
		 *  * ID must be specified.
		 *  * Either KadPort or Port must be specified.
		 *  * For lowid sources, ServerIP must be specified.
		 *
		 */
		CDeadSource(uint32 ID, uint16 Port, uint32 ServerIP, uint16 KadPort);

		
		/**
		 * Equality operator.
		 */
		bool operator==(const CDeadSource& other) const;

		
		/**
		 * Sets the timestamp for the time where this entry will expire.
		 */
		void	SetTimeout( uint32 t );

		/**
		 * Returns the timestamp of this entry.
		 */
		uint32	GetTimeout() const;
		
	private:
		//! The ID/IP of the client.
		uint32			m_ID;
		//! The TCP port of the client
		uint16			m_Port;
		//! The Kad port of the client.
		uint16			m_KadPort;
		//! The IP of the server the client is connected to.
		uint32			m_ServerIP;
		//! The timestamp of DOOM!
		uint32			m_TimeStamp;
	};

	
	typedef std::multimap< uint32, CDeadSource > DeadSourceMap;
	typedef DeadSourceMap::iterator DeadSourceIterator;
	typedef std::pair<DeadSourceIterator, DeadSourceIterator> DeadSourcePair;
	//! List of currently dead sources.
	DeadSourceMap m_sources;

	
	//! The timestamp of when the last cleanup was performed.
	uint32	m_dwLastCleanUp;
	//! Specifies if the list is global or not.
	bool	m_bGlobalList;
};

#endif
