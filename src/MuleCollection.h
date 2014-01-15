//
// This file is part of the aMule Project.
//
// Copyright (c) 2007-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2007-2011 Johannes Krampf ( wuischke@amule.org )
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


#ifndef __MULECOLLECTION_H__
#define __MULECOLLECTION_H__

#include <string>
#include <vector>

#include "Types.h"


class CMuleCollection
{
private:
	std::vector<std::string> vCollection;

public:
	CMuleCollection() {};
	~CMuleCollection() {};

	bool Open(const std::string &File);

	size_t 			size() const 			{ return vCollection.size(); }
	std::string&		operator[](size_t index)	{ return vCollection[index]; }
	const std::string&	operator[](size_t index) const	{ return vCollection[index]; }

private:
	bool OpenBinary(const std::string &File);
	bool OpenText(const std::string &File);

	template <typename intType>
	intType ReadInt(std::ifstream& infile);

	std::string ReadString(std::ifstream& infile, int TagType);
};

#endif // __MULECOLLECTION_H__

