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

#if !defined( __DEBUGSTUFF_H__ )
#define __DEBUGSTUFF_H__

#define MAGIC_1 1234567890
#define MAGIC_2 1357902468

#if defined( __DEBUG__ )
	int debugprintf(int verbose, char *fmt, ...);
#else // __DEBUG__
	static inline int debugprintf(int verbose, char *fmt, ...) {}
#endif // __DEBUG__

#endif // __DEBUGSTUFF_H__
