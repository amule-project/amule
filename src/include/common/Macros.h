//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef COMMONMACROS_H
#define COMMONMACROS_H

#define	SEC2MS(sec)		((sec)*1000)
#define	MIN2MS(min)		SEC2MS((min)*60)
#define	HR2MS(hr)		MIN2MS((hr)*60)
#define	DAY2MS(day)		HR2MS((day)*24)
#define	SEC(sec)		(sec)
#define	MIN2S(min)		((min)*60)
#define	HR2S(hr)		MIN2S((hr)*60)
#define	DAY2S(day)		HR2S((day)*24)

#define COMPILE_DATE				__DATE__
#define COMPILE_TIME				__TIME__

#endif // COMMONMACROS_H
