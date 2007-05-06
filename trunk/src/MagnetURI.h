//                                                       -*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2007 Dévai Tamás ( gonosztopi@amule.org )
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

#ifndef MAGNETURI_H
#define MAGNETURI_H

#ifdef USE_STD_STRING
#	include <string>
#	define STRING	std::string
#else
#	include <wx/string.h>
#	define STRING	wxString
#endif

#include <list>		// Needed for std::list
#include <utility>	// Needed for std::pair

class CMagnetURI
{
      public:
	typedef	std::list<STRING>	Value_List;

	CMagnetURI() {}
	CMagnetURI(const STRING& uri);

	void		AddField(const STRING& name, const STRING& value)	{ m_fields.push_back(Field_Type(name, value)); }
	Value_List	GetField(const STRING& name) const;
	void		Clear() 						{ m_fields.clear(); }
	STRING		GetLink() const;
	operator STRING() const							{ return GetLink(); }

      protected:
	typedef std::pair<STRING, STRING>	Field_Type;
	typedef std::list<Field_Type>		List_Type;

	List_Type	m_fields;
};

class CMagnetED2KConverter : private CMagnetURI
{
      public:
	CMagnetED2KConverter(const STRING& uri)
		: CMagnetURI(uri)
		{}

	bool	CanConvertToED2K() const;
	STRING	GetED2KLink() const;

	operator STRING() const		{ return GetED2KLink(); }
};

#endif /* MAGNETURI_H */
