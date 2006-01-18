//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef RANGEMAP_H
#define RANGEMAP_H

#include <map>

#include <common/MuleDebug.h>

#include "Types.h"


/**
 * Default helper structure for normal CRangeMap instantations.
 *
 * Specializations should must have the following properties.
 *  - The four value typedefs (see comments for details).
 *  - A template-defined member variable named 'first'.
 *  - A comparison operator that doesn't consider the 'first' field.
 *
 *  The typedefs are used to specify the return values of iterators.
 */
template <typename VALUE, typename KEYTYPE>
struct CRangeMapHelper
{
	//! Typedef specifying the type to use when a non-const pointer is expected.
	typedef VALUE* ValuePtr;
	//! Typedef specifying the type to use when a non-const referenecs is expected.
	typedef VALUE& ValueRef;
	//! Typedef specifying the type to use when a const referenecs is expected.
	typedef const VALUE& ConstValueRef; 
	//! Typedef specifying the type to use when a const pointer is expected.
	typedef const VALUE* ConstValuePtr;

	//! Used internally by CRangeMap to specify the end of a range.
	KEYTYPE first;
	//! Contains the value of a given range.
	VALUE  second;

	//! Compares the user-values of this range with another.
	bool operator==(const CRangeMapHelper<VALUE, KEYTYPE>& o) const {
		return second == o.second;
	}
};


/**
 * Helper structure for CRangeSet (CRangeMap with void as value).
 */
template <typename KEYTYPE>
struct CRangeMapHelper<void, KEYTYPE>
{	
	typedef void ValuePtr;
	typedef void ValueRef;
	typedef void ConstValueRef;
	typedef void ConstValuePtr;
	
	KEYTYPE first;

	bool operator==(const CRangeMapHelper<void, KEYTYPE>&) const {
		return true;
	}
};


/**
 * This class represents a map of non-overlapping ranges.
 *
 * Each range has a user-specified value associated. The map supports quick
 * lookup of which range covers a particular key-value, and will merge or
 * split existing ranges when new ranges are added. 
 *
 * The decision on whenever to split/resize a range or to merge the two ranges
 * involved is based on equality of the user-specified value, using the 
 * equality operator. Thus if two ranges with the same user-value are placed 
 * adjacent to each other or partially overlapping each other, then they will
 * be merged into a single range. If the user-values of the two ranges are
 * different, then the old range will be either resized or split, based on the 
 * position of the new range.
 *
 * In cases where ranges are split into two parts, copies will be made of the
 * user-specified value, such that each new part contains the same user-value.
 *
 * It is currently not possible to manipulate existing ranges by hand, other
 * than by erasing and then re-inserting them.
 *
 * A specialization of this class exists (typedef'd as CRangeSet), which does
 * not assosiate a value with each range.
 *
 * NOTE: KEYTYPE is assumed to be an unsigned integer type!
 */
template <typename VALUE, typename KEYTYPE = uint64>
class CRangeMap
{
	typedef CRangeMapHelper<VALUE, KEYTYPE> HELPER;
private:
	//! The map uses the start-key as key and the User-value and end-key pair as value
	typedef std::map<KEYTYPE, HELPER> RangeMap;
	//! Shortcut for the pair used by the RangeMap.
	typedef std::pair<KEYTYPE, HELPER> RangePair;

	//! Typedefs used to distinguish between our custom iterator and the real ones.
	typedef typename RangeMap::iterator RangeIterator;
	typedef typename RangeMap::const_iterator ConstRangeIterator;
	
	//! The raw map of range values.
	RangeMap	m_ranges;

	/**
	 * This class provides a wrapper around the raw iterators used by CRangeMap.
	 *
	 * It will act as a normal iterator and also give access the the range values.
	 * When used as a per normal, it will return the value specified by the user
	 * for that range.
	 *
	 * Special member-functions are keyStart() and keyEnd().
	 */ 
	template <typename RealIterator, typename ReturnTypeRef, typename ReturnTypePtr>
	class iterator_base {
		friend class CRangeMap<VALUE, KEYTYPE>;
	public:
		iterator_base( const RealIterator& it )
			: m_it( it )
		{
		}

		//! Equality operator
		bool operator==( const iterator_base& other ) const {
			return m_it == other.m_it;
		}

		//! Non-equality operator
		bool operator!=( const iterator_base& other ) const {
			return m_it != other.m_it;
		}

		
		//! Returns the starting point of the range
		KEYTYPE keyStart() const {
			return m_it->first;
		}
		
		//! Returns the end-point of the range
		KEYTYPE keyEnd() const {
			return m_it->second.first;
		}


		//! Prefix increment.
		iterator_base& operator++() {
			++m_it;
			
			return *this;
		}

		//! Postfix increment.
		iterator_base operator++(int) {
			return iterator_base( m_it++ );
		}


		//!  Prefix decrement.
		iterator_base& operator--() {
			--m_it;

			return *this;
		}

		//! Postfix decrement.
		iterator_base operator--(int) {
			return iterator_base( m_it-- );
		}


		//! Deference operator, returning the user-specified value.
		ReturnTypeRef operator*() const {
			return m_it->second.second;
		}

		//! Member access operator, returning the user-specified value.
		ReturnTypePtr operator->() const {
			return &m_it->second.second;
		}

	protected:
		//! The raw iterator
		RealIterator m_it;
	};

	typedef typename HELPER::ValueRef ValueRef;
	typedef typename HELPER::ValuePtr ValuePtr;
	typedef typename HELPER::ConstValueRef ConstValueRef;
	typedef typename HELPER::ConstValuePtr ConstValuePtr;

public:
	typedef iterator_base<RangeIterator, ValueRef, ValuePtr> iterator;
	typedef iterator_base<ConstRangeIterator, ConstValueRef, ConstValuePtr> const_iterator;
	
	//! The type used to specify size, ie size().
	typedef typename RangeMap::size_type size_type;
	
	//! The type of user-data saved with each range.
	typedef VALUE value_type;
	
	/**
	 * Default constructor.
	 */
	CRangeMap() {
	}

	/**
	 * Copy-constructor.
	 */
	CRangeMap(const CRangeMap<VALUE, KEYTYPE>& other)
		: m_ranges( other.m_ranges )
	{	
	}

	/**
	 * Assignment operator.
	 */
	CRangeMap& operator=(const CRangeMap<VALUE, KEYTYPE>& other) {
		m_ranges = other.m_ranges;

		return *this;
	}

	/**
	 * Equality operator for two ranges.
	 *
	 * @returns True if both ranges contain the same ranges and values.
	 */
	bool operator==( const CRangeMap<VALUE, KEYTYPE>& other ) const {
		// Check if we are comparing with ourselves
		if ( this == &other ) {
			return true;
		}
		
		// Check size, must be the same
		if ( size() != other.size() ) {
			return false;
		}
		
		return (m_ranges == other.m_ranges);
	}
	

	/**
	 * Returns an iterator pointing to the first range.
	 */
	iterator begin() {
		return m_ranges.begin();
	}

	/**
	 * Returns an iterator pointing past the last range.
	 */
	iterator end() {
		return m_ranges.end();
	}

	/**
	 * Returns a const iterator pointing to the first range.
	 */
	const_iterator begin() const {
		return m_ranges.begin();
	}

	/**
	 * Returns a const iterator pointing past the last range.
	 */
	const_iterator end() const {
		return m_ranges.end();
	}


	/**
	 * Erases the specified range and returns the range next to it.
	 *
	 * @param pos The iterator of the range to be erased.
	 * @return The iterator of the range after the erased range.
	 *
	 * Attempting to erase the end() iterator is an invalid operation.
	 */
	iterator erase(iterator pos) {
		MULE_VALIDATE_PARAMS(pos != end(), wxT("Cannot erase 'end'"));
	
		RangeIterator temp = pos.m_it++;

		m_ranges.erase(temp);

		return pos;
	}


	/**
	 * Returns the number of ranges in the map.
	 */
	size_type size() const {
		return m_ranges.size();
	}

	/**
	 * Returns true if the map is empty.
	 */
	bool empty() const {
		return m_ranges.empty();
	}


	/**
	 * Removes all ranges from the map.
	 */
	void clear() {
		m_ranges.clear();
	}


	/**
	 * Returns the range covering the specified key-value.
	 *
	 * @param key A value that may or may not be covered by a range.
	 * @return end() or the iterator of the range covering key.
	 *
	 * A range is considered to cover a value if the value is greather than or
	 * equal to the start-key and less than or equal to the end-key.
	 */
	// Find the range which contains key (it->first <= key <= it->second->first)
	iterator find_range( KEYTYPE key ) {
		if ( !m_ranges.empty() ) {
			// Find first range whose start comes after key
			// Thus: key < it->first, but (--it)->first <= key
			RangeIterator it = m_ranges.upper_bound( key );

			// Our target range must come before the one we found; does it exist?
			if ( it != m_ranges.begin() ) {
				// Go back to the last range which starts at or before key
				it--;

				// Check if this range covers the key
				if ( key <= it->second.first ) {
					return it;
				}
			}
		}

		return end();
	}


	void erase_range(KEYTYPE startPos, KEYTYPE endPos) {
		// Create default initialized entry, which ensures that all fields are initialized.
		HELPER entry = HELPER();
		// Need to set the 'end' field.
		entry.first = endPos;
		
		// Insert without merging, which forces the creation of an entry that 
		// only covers the specified range, which will crop existing ranges.
		erase(do_insert(startPos, entry, false));
	}	
	

	/**
	 * Inserts a new range into the map, potentially erasing/changing old ranges.
	 *
	 * @param startPos The start position of the range, also considered part of the range.
	 * @param endPos The end position of the range, also considered part of the range.
	 * @param object The user-data to be assosiated with the range.
	 * @return An iterator pointing to the resulting range, covering at least the specified range.
	 *
	 * This function inserts the specified range into the map, while overwriting
	 * or resizing existing ranges if there is any conflict. Ranges might also 
	 * be merged, if the object of each evaluates to being equal, in which case
	 * the old range will be removed and the new extended to include the old
	 * range. This also includes ranges placed directly after or in front of each
	 * other, which will also be merged if their type is the same.
	 *
	 * This has the result that the iterator returned can point to a range quite
	 * different from what was originally specified. If this is not desired, then
	 * the VALUE type should simply be made to return false on all equality tests.
	 * Otherwise, the only promise that is made is that the resulting range has 
	 * the same user-data (based on the equality operator) as the what was specified.
	 *
	 * Note that the start position must be smaller than or equal to the end-position.
	 */
	//@{
	iterator insert(KEYTYPE startPos, KEYTYPE endPos) {
		HELPER entry = {endPos};
		return do_insert(startPos, entry);
	}
	template <typename TYPE>
	iterator insert(KEYTYPE startPos, KEYTYPE endPos, const TYPE& value) {
		HELPER entry = {endPos, value};
		return do_insert(startPos, entry);
	}
	//@}

protected:
	/**
	 * Inserts the specified range.
	 *
	 * @param start The starting position of the range.
	 * @param entry A helper-struct, containing the end position and possibly user-data.
	 * @param merge Specifies if ranges should be merged when possible.
	 * @return An iterator pointing to the range covering at least the specified range.
	 */
	iterator do_insert(KEYTYPE start, HELPER entry, bool merge = true) {
		MULE_VALIDATE_PARAMS(start <= entry.first, wxT("Not a valid range."));
		
		RangeIterator it = get_insert_it(start);
		while ( it != m_ranges.end() ) {
			// Begins before the current span
			if ( start <= it->first ) {
				// Never touches the current span, it follows that start < it->first
				// (it->first) is used to avoid checking against (uintXX)-1 by accident
				if ( entry.first < it->first - 1 && it->first ) {
					break;
				}

				// Stops just before the current span, it follows that start < it->first
				// (it->first) is used to avoid checking against (uintXX)-1 by accident
				else if ( entry.first == it->first - 1 && it->first ) {
					// If same type: Merge
					if (merge and (entry == it->second)) {
						entry.first = it->second.first;
						m_ranges.erase( it++ );
					}

					break;
				}

				// Covers part of the span
				else if ( entry.first < it->second.first ) {
					// Same type, merge
					if (merge and (entry == it->second)) {
						entry.first = it->second.first;
						m_ranges.erase( it++ );
					} else {
						// Resize the partially covered span and get the next one
						it = ++resize( entry.first + 1, it->second.first, it );
					}

					break;
				} else {
					// It covers the entire span
					m_ranges.erase( it++ );
				}
			}
			
			// Starts inside the current span or after the current span
			else {
				// Starts inside the current span
				if ( start <= it->second.first ) {
					// Ends inside the current span
					if ( entry.first < it->second.first ) {
						// Adding a span with same type inside a existing span is fruitless
						if (merge and (entry == it->second)) {
							return it;
						}

						// Insert the new span
						m_ranges.insert(it, RangePair(entry.first + 1, it->second));
						
						// Resize the current span to fit before the new span
						it->second.first = start - 1;
		
						break;
					} else {
						// Ends past the current span, resize or merge
						if (merge and (entry == it->second)) {
							start = it->first;
							m_ranges.erase( it++ );
						} else {
							// Resize the partially covered span and get the next one
							it = ++resize( it->first, start - 1, it );
						}
					}
				} else {
					// Start past the current span
					if ( start == it->second.first + 1 ) {
						// Touches the current span
						if (merge and (entry == it->second)) {
							start = it->first;
							m_ranges.erase( it++ );
						} else {
							++it;
						}
					} else {
						// Starts after the current span, nothing to do
						++it;
					}
				}
			}
		}

		return m_ranges.insert(it, RangePair(start, entry));
	}

	
	/**
	 * Finds the optimal location to start looking for insertion points.
	 *
	 * This is the first range whose start comes after the new start. We check
	 * the last element first, since sequential insertions are commen.
	 */
	RangeIterator get_insert_it(KEYTYPE start)
	{
		if ( m_ranges.empty() ) {
			return m_ranges.end();
		}

		// The start-key of the last element must be smaller than our start-key
		// Otherwise there is the possibility that we can merge with the one before that
		RangeIterator it = --m_ranges.end();
		if ( start <= it->first ) {
			// If the two starts are equal, then we only need to go back another
			// step to see if the range prior to this one is mergeable
			if ( start != it->first ) {
				it = m_ranges.lower_bound( start );
			}

			if ( it != m_ranges.begin() ) {
				// Go back to the last range which starts at or before key
				--it;
			}
		}

		return it;
	}
	

	//! Helper function that resizes an existing range to the specified size.
	RangeIterator resize( KEYTYPE startPos, KEYTYPE endPos, RangeIterator it ) {
		HELPER item = it->second;
		item.first = endPos;

		m_ranges.erase( it++ );

		return m_ranges.insert(it, RangePair(startPos, item));
	}
};


//! CRangeSet is simply a partial specialization of CRangeMap
typedef CRangeMap<void> CRangeSet;


#endif
