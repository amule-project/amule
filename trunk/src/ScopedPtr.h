//
// This file is part of the aMule Project.
//
// Copyright (c) 2006 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2006 aMule Team ( admin@amule.org / http://www.amule.org )
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
	CScopedPtr(TYPE* ptr);
	
	/** Frees the pointer owned by the instance. */
	~CScopedPtr();
	
	
	//@{
	/** Deference operators. */
	TYPE& operator*() const;
	TYPE* operator->() const;
	//@}
	
	
	/** Returns the actual pointer value. */
	TYPE* get() const;
	
	/** Sets the actual pointer to a different value. The old pointer is freed. */
	void reset(TYPE* ptr = 0);
	
	/** Returns the actual pointer. The scoped-ptr will thereafter contain NULL. */
	TYPE* release();
	
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
	CScopedArray(TYPE* ptr);
	
	/** Frees the array owned by this instance. */
	~CScopedArray();
	
	
	/** Accessor. */
	TYPE& operator[](unsigned i) const;
	
	
	/** @see CScopedPtr::get */
	TYPE* get() const;
	
	/** @see CScopedPtr::reset */
	void reset(TYPE* ptr = 0);
	
	/** @see CScopedPtr::release */
	TYPE* release();
	
private:
	//@{
	//! A scoped array is neither copyable, nor assignable.
	CScopedArray(const CScopedArray<TYPE>&);
	CScopedArray<TYPE>& operator=(const CScopedArray<TYPE>&);
	//@}
	
	TYPE* m_ptr;
};




////////////////////////////////////////////////////////////
// Implementations

template <typename TYPE>
CScopedPtr<TYPE>::CScopedPtr(TYPE* ptr)
	: m_ptr(ptr)
{
}


template <typename TYPE>
CScopedPtr<TYPE>::~CScopedPtr()
{
	delete m_ptr;
}


template <typename TYPE>
TYPE& CScopedPtr<TYPE>::operator*() const
{
	return *m_ptr;
}


template <typename TYPE>
TYPE* CScopedPtr<TYPE>::operator->() const
{
	return m_ptr;
}


template <typename TYPE>
TYPE* CScopedPtr<TYPE>::get() const
{
	return m_ptr;
}


template <typename TYPE>
void CScopedPtr<TYPE>::reset(TYPE* ptr)
{
	delete m_ptr;
	m_ptr = ptr;
}


template <typename TYPE>
TYPE* CScopedPtr<TYPE>::release()
{
	TYPE* ptr = m_ptr;
	m_ptr = 0;
	return ptr;
}



template <typename TYPE>
CScopedArray<TYPE>::CScopedArray(TYPE* ptr)
	: m_ptr(ptr)
{
}

	
template <typename TYPE>
CScopedArray<TYPE>::~CScopedArray()
{
	delete[] m_ptr;
}


template <typename TYPE>
TYPE& CScopedArray<TYPE>::operator[](unsigned i) const
{
	return m_ptr[i];
}
	
	
template <typename TYPE>
TYPE* CScopedArray<TYPE>::get() const
{
	return m_ptr;
}


template <typename TYPE>
void CScopedArray<TYPE>::reset(TYPE* ptr)
{
	delete[] m_ptr;
	m_ptr = ptr;
}


template <typename TYPE>
TYPE* CScopedArray<TYPE>::release()
{
	TYPE* ptr = m_ptr;
	m_ptr = 0;
	return ptr;
}


#endif // SCOPEDPTR_H
