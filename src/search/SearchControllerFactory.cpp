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

#include "SearchControllerFactory.h"
#include "ED2KSearchController.h"
#include "KadSearchController.h"

namespace search {

std::unique_ptr<SearchController> SearchControllerFactory::createController(ModernSearchType type)
{
    switch (type) {
	case ModernSearchType::LocalSearch:
	    return std::make_unique<ED2KSearchController>();
	case ModernSearchType::GlobalSearch:
	    return std::make_unique<ED2KSearchController>();
	case ModernSearchType::KadSearch:
	    return std::make_unique<KadSearchController>();
	default:
	    return std::make_unique<ED2KSearchController>();
    }
}

} // namespace search
