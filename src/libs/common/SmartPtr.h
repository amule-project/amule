//							-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2019 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SMARTPTR_H
#define SMARTPTR_H

// std::auto_ptr is deprecated in C++11 and removed in C++17. We should use
// std::unique_ptr instead, which was introduced in C++11.
//
// Considering the above, we shall use std::unique_ptr if we're using C++11 or
// a later standard, and std::auto_ptr otherwise.

#include <memory>

#if __cplusplus >= 201103L

// It seems like Apple has (or had, hopefully) a configuration where a new
// clang compiler supporting C++11 is accompanied by an old c++ library not
// supporting std::unique_ptr.
// See https://stackoverflow.com/questions/31655462/no-type-named-unique-ptr-in-namespace-std-when-compiling-under-llvm-clang
#	ifdef __clang__
#		if __has_include(<forward_list>)
			// either using libc++ or a libstdc++ that's
			// new enough to have unique_ptr
#			define HAVE_UNIQUE_PTR 1
#		endif
#	else
		// not clang, assume unique_ptr available
#		define HAVE_UNIQUE_PTR 1
#	endif

#endif

#ifdef HAVE_UNIQUE_PTR
template<typename T> using CSmartPtr = std::unique_ptr<T>;
#else
#	define CSmartPtr std::auto_ptr
#	ifndef nullptr
#		define nullptr	 NULL
#	endif
#endif

#undef HAVE_UNIQUE_PTR

#endif /* SMARTPTR_H */
