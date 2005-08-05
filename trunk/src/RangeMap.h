//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 Mikkel Schubert ( xaignar@users.sourceforge.net )
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef RANGEMAP_H
#define RANGEMAP_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "RangeMap.h"
// implementation in IPFilter.cpp,
// for remote GUI in BarShader.cpp
#endif

#include <map>

#include "MuleDebug.h"
#include "Types.h"



/**
 * This class represents a map of non-overlapping ranges.
 *
 * The value of ranges are currently hardcoded to uint32s, however each range
 * also has a user-specified value associated. The map supports quick lookup of
 * which range covers a particular key-value, and will merge or split existing
 * ranges when new ranges are added. 
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
 */
template <typename VALUE>
class CRangeMap
{
private:
	//! This pair is used to contain the user-value and the end-key
	typedef std::pair< uint32, VALUE >	RangeItems;
	//! The map uses the start-key as key and the User-value and end-key pair as value
	typedef std::map< uint32, RangeItems >	RangeMap;
	//! Shortcut for the pair used by the RangeMap.
	typedef std::pair< uint32, RangeItems >	RangePair;

	//! Typedef used to distinguish between our custom iterator and the real ones.
	typedef typename RangeMap::iterator RangeIterator;

	//! The raw map of range values.
	RangeMap	m_ranges;


public:
	//! The type used to specify size, ie size().
	typedef typename RangeMap::size_type size_type;
	
	//! The type of user-data saved with each range.
	typedef VALUE value_type;
	
	class const_iterator;
	class iterator;
	

	/**
	 * Default constructor.
	 */
	CRangeMap() {
	}

	/**
	 * Copy-constructor.
	 */
	CRangeMap(const CRangeMap<VALUE>& other)
		: m_ranges( other.m_ranges )
	{	
	}

	/**
	 * Assignment operator.
	 */
	CRangeMap& operator=(const CRangeMap<VALUE>& other) {
		m_ranges = other.m_ranges;

		return *this;
	}

	/**
	 * Equality operator for two ranges.
	 *
	 * @returns True if both ranges contain the same ranges and values.
	 */
	bool operator==( const CRangeMap<VALUE>& other ) const {
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
	iterator find_range( uint32 key )
	{
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


	/**
	 * Inserts a new range into the map, potentially erasing/changing old ranges.
	 *
	 * @param start The start position of the range, also considered part of the range.
	 * @param end The end position of the range, also considered part of the range.
	 * @param object The user-data to be assosiated with the range.
	 * @return An iterator pointing to the resulting range.
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
	iterator insert(uint32 start, uint32 end, const VALUE& object) {
		MULE_VALIDATE_PARAMS(start <= end, wxT("Not a valid range."));
	
		if ( m_ranges.empty() ) {
			return m_ranges.insert( m_ranges.end(), RangePair( start, RangeItems( end, object ) ) );
		}

        // Find first range whose start comes after the new start. We check
		// the last element first, since sequential insertions are commen
		RangeIterator it = --m_ranges.end();


		// The start-key of the last element must be smaller than our start-key
		// Otherwise there is the possibility that we can merge with the one before that
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


		while ( it != m_ranges.end() ) {
			// Begins before the current span
			if ( start <= it->first ) {
				// Never touches the current span, it follows that start < it->first
				// (it->first) is used to avoid checking against (uint32)-1 by accident
				if ( end < it->first - 1 && it->first ) {
					break;
				}

				// Stops just before the current span, it follows that start < it->first
				// (it->first) is used to avoid checking against (uint32)-1 by accident
				else if ( end == it->first - 1 && it->first ) {
					// If same type: Merge
					if ( object == it->second.second ) {
						end = it->second.first;
						m_ranges.erase( it++ );
					}

					break;
				}

				// Covers part of the span
				else if ( end < it->second.first ) {
					// Same type, merge
					if ( object == it->second.second ) {
						end = it->second.first;
						m_ranges.erase( it++ );
					} else {
						// Resize the partially covered span and get the next one
						it = ++resize( end + 1, it->second.first, it );
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
					if ( end < it->second.first ) {
						// Adding a span with same type inside a existing span is fruitless
						if ( object == it->second.second ) {
							return it;
						}

						// Create a new span to cover the second block
						VALUE item( it->second.second );

						// Insert the new span
						m_ranges.insert( it, RangePair( end + 1, RangeItems( it->second.first, item ) ) );
						
						// Resize the current span to fit before the new span
						it->second.first = start - 1;
		
						break;
					} else {
						// Ends past the current span, resize or merge
						if ( object == it->second.second ) {
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
						if ( object == it->second.second ) {
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

		return m_ranges.insert( it, RangePair( start, RangeItems( end, object ) ) );
	}

	
	/**
	 * This class provides a wrapper around the raw iterator used by CRangeMap.
	 *
	 * It will act as a normal iterator and also give access the the range values.
	 * When used as a per normal, it will return the value specified by the user
	 * for that range.
	 *
	 * Special member-functions are keyStart() and keyEnd().
	 */ 
	class iterator {
		friend class CRangeMap<VALUE>;
		typedef typename CRangeMap<VALUE>::RangeMap::iterator RealIterator;

	public:
		iterator( const RealIterator& it )
			: m_it( it )
		{
		}

		//! Equality operator
		bool operator==( const iterator& other ) const {
			return m_it == other.m_it;
		}

		//! Non-equality operator
		bool operator!=( const iterator& other ) const {
			return m_it != other.m_it;
		}

		
		//! Returns the starting point of the range
		uint32 keyStart() const {
			return m_it->first;
		}
		
		//! Returns the end-point of the range
		uint32 keyEnd() const {
			return m_it->second.first;
		}


		//! Prefix increment.
		iterator& operator++() {
			++m_it;
			
			return *this;
		}

		//! Postfix increment.
		iterator operator++(int) {
			iterator tmp( *this );

			++m_it;

			return tmp;
		}


		//!  Prefix decrement.
		iterator& operator--() {
			--m_it;

			return *this;
		}

		//! Postfix decrement.
		iterator operator--(int) {
			iterator tmp( *this );

			--m_it;

			return tmp;
		}


		//! Deference operator, returning the user-specified value.
		VALUE& operator*() const {
			return m_it->second.second;
		}

		//! Member access operator, returning the user-specified value.
		VALUE* operator->() const {
			return &m_it->second.second;
		}

	private:
		//! The raw iterator
		RealIterator m_it;
	};


	/**
	 * This class provides a wrapper around the raw const_iterator used by CRangeMap.
	 *
	 * It will act as a normal iterator and also give access the the range values.
	 * When used as a per normal, it will return the value specified by the user
	 * for that range.
	 *
	 * Special member-functions are keyStart() and keyEnd().
	 */ 
	class const_iterator {
		typedef typename CRangeMap<VALUE>::RangeMap::const_iterator RealIterator;

	public:
		const_iterator( const RealIterator& it )
			: m_it( it )
		{
		}

		//! Equality operator
		bool operator==( const const_iterator& other ) const {
			return m_it == other.m_it;
		}

		//! Non-equality operator
		bool operator!=( const const_iterator& other ) const {
			return m_it != other.m_it;
		}

		
		//! Returns the starting point of the range
		uint32 keyStart() const {
			return m_it->first;
		}
		
		//! Returns the end-point of the range
		uint32 keyEnd() const {
			return m_it->second.first;
		}


		//! Prefix increment.
		const_iterator& operator++() {
			++m_it;
			
			return *this;
		}

		//! Postfix increment.
		const_iterator operator++(int) {
			const_iterator tmp( *this );

			++m_it;

			return tmp;
		}


		//!  Prefix decrement.
		const_iterator& operator--() {
			--m_it;

			return *this;
		}

		//! Postfix decrement.
		const_iterator operator--(int) {
			const_iterator tmp( *this );

			--m_it;

			return tmp;
		}


		//! Deference operator, returning the user-specified value.
		const VALUE& operator*() const {
			return m_it->second.second;
		}

		//! Member access operator, returning the user-specified value.
		const VALUE* operator->() const {
			return &m_it->second.second;
		}

	private:
		//! The raw iterator
		RealIterator m_it;
	};

private:
	//! Helper function that resizes an existing range to the specified size.
	RangeIterator resize( uint32 start, uint32 end, RangeIterator it ) {
		VALUE item( it->second.second );

		m_ranges.erase( it++ );

		return m_ranges.insert( it, RangePair( start, RangeItems( end, item ) ) );		
	}

	//! The iterator-wrapper needs access to the RangeMap typedef
	friend class iterator;
	//! The const-iterator needs access to the RangeMap typedef
	friend class const_iterator;
};

#endif
