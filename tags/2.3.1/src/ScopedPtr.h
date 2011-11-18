//							-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2006-2011 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2006-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SCOPEDPTR_H
#define SCOPEDPTR_H

#include "OtherFunctions.h"		// Needed for DeleteContents()

/**
 * CScopedPtr is a simple smart pointer.
 *
 * This class is a replacement for std::auto_ptr, with simpler
 * copying schematics, in that it doesn't allow copying or 
 * assignment, compared to auto_ptr, which allows only one 
 * instance to own a pointer (swapping at assignment).
 */
template <typename TYPE>
class CScopedPtr
{
public:
	/** Constructor. Note that CScopedPtr takes ownership of the pointer. */
	CScopedPtr(TYPE* ptr)
		: m_ptr(ptr)
	{}

	CScopedPtr()
	{
		m_ptr = new TYPE;
	}

	/** Frees the pointer owned by the instance. */
	~CScopedPtr()			{ delete m_ptr; }

	//@{
	/** Deference operators. */
	TYPE& operator*() const		{ return *m_ptr; }
	TYPE* operator->() const	{ return m_ptr; }
	//@}


	/** Returns the actual pointer value. */
	TYPE* get() const		{ return m_ptr; }

	/** Sets the actual pointer to a different value. The old pointer is freed. */
	void reset(TYPE* ptr = 0)	{ delete m_ptr; m_ptr = ptr; }

	/** Returns the actual pointer. The scoped-ptr will thereafter contain NULL. */
	TYPE* release()
	{
		TYPE* ptr = m_ptr;
		m_ptr = 0;
		return ptr;
	}

private:
	//@{
	//! A scoped pointer is neither copyable, nor assignable.
	CScopedPtr(const CScopedPtr<TYPE>&);
	CScopedPtr<TYPE>& operator=(const CScopedPtr<TYPE>&);
	//@}

	TYPE* m_ptr;
};


/**
 * Similar to CScopedPtr, except that an array is expected.
 *
 * @see CScopedPtr
 */
template <typename TYPE>
class CScopedArray
{
public:
	/** Constructor. Note that CScopedArray takes ownership of the array. */
	CScopedArray(TYPE* ptr)
		: m_ptr(ptr)
	{}

	/** Constructor, allocating nr elements. */
	CScopedArray(size_t nr)			{ m_ptr = new TYPE[nr]; }

	/** Frees the array owned by this instance. */
	~CScopedArray()				{ delete[] m_ptr; }


	/** Accessor. */
	TYPE& operator[](unsigned i) const	{ return m_ptr[i]; }


	/** @see CScopedPtr::get */
	TYPE* get() const			{ return m_ptr; }

	/** @see CScopedPtr::reset */
	void reset(TYPE* ptr = 0)		{ delete[] m_ptr; m_ptr = ptr; }

	/** free the existing array and allocate a new one with nr elements */
	void reset(size_t nr)			{ delete[] m_ptr; m_ptr = new TYPE[nr]; }

	/** @see CScopedPtr::release */
	TYPE* release()
	{
		TYPE* ptr = m_ptr;
		m_ptr = 0;
		return ptr;
	}


private:
	//@{
	//! A scoped array is neither copyable, nor assignable.
	CScopedArray(const CScopedArray<TYPE>&);
	CScopedArray<TYPE>& operator=(const CScopedArray<TYPE>&);
	//@}

	TYPE* m_ptr;
};


/**
 * Similar to CScopedPtr, except that a STL container of pointers is expected
 * which has to be freed with DeleteContents.
 *
 * @see CScopedPtr
 */
template <typename STL_CONTAINER>
class CScopedContainer
{
public:
	/** Constructor. Note that CScopedContainer takes ownership of the array. */
	CScopedContainer(STL_CONTAINER* ptr)
		: m_ptr(ptr)
	{}

	CScopedContainer()
	{
		m_ptr = new STL_CONTAINER;
	}

	~CScopedContainer()	
	{
		if (m_ptr) {
			DeleteContents(*m_ptr);
			delete m_ptr;
		}
	}

	//@{
	/** Deference operators. */
	STL_CONTAINER& operator*() const	{ return *m_ptr; }
	//@}


	/** Returns the actual pointer value. */
	STL_CONTAINER* get() const			{ return m_ptr; }

private:
	//@{
	//! A scoped container is neither copyable, nor assignable.
	CScopedContainer(const CScopedContainer<STL_CONTAINER>&);
	CScopedContainer<STL_CONTAINER>& operator=(const CScopedContainer<STL_CONTAINER>&);
	//@}

	STL_CONTAINER* m_ptr;
};

#endif // SCOPEDPTR_H
// File_checked_for_headers
