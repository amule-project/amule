// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#ifndef CTYPEDPTRLIST_H
#define CTYPEDPTRLIST_H

#include "position.h"	// Needed for POSITION


template <class TYPE, class ARG_TYPE = TYPE const&>
class CList
{
private:
	// The node used in the list
	struct MyNode
	{
		struct MyNode* next;
		struct MyNode* prev;
		TYPE data;
	};
	
	typedef MyNode MYNODE;
	
protected:
	MYNODE* head;	// First node
	MYNODE* tail;	// Last node
	int count;		// Number of nodes

	// Deletes a node
	void freeNode( MYNODE* n ) { 
		delete n;
	}

	// Creates a new node with the specified data
	MYNODE* newNode( ARG_TYPE data ) {
		MYNODE* n = new MYNODE;
		n->prev = n->next = NULL;
		n->data = data;
		return n;
	}

	// Creates an empty node
	MYNODE* newNode() {
		MYNODE* n = new MYNODE;
		n->prev = n->next = NULL;
		return n;
	}

public:
	CList( int WXUNUSED(none) = 0 )
	: head(NULL), tail(NULL), count(0) 
	{
	}
	
	
	~CList() {
		RemoveAll();
	}
	

	// Returns the size of the list
	int GetCount() const { 
		return count; 
	}
	
	int GetSize() const { 
		return count;
	}
	
	
	// Returns true if the list is empty
	bool IsEmpty() const {
		return ( count == 0 );
	}

	
	// Returns a reference to the first element of the list. Do not call on an empty list!
	TYPE& GetHead()
	{
		wxASSERT( head );
		return head->data;
	}
	
	const TYPE& GetHead() const
	{
		wxASSERT( head );
		return head->data;
	}

	
	// Returns a reference to the last element of the list. Do not call on an empty list!
	TYPE& GetTail()
	{
		wxASSERT( tail );
		return tail->data;
	}
	
	const TYPE& GetTail() const
	{
		wxASSERT( tail );
		return tail->data;
	}

	// Removes the first element and returns the value it contained
	TYPE RemoveHead()
	{
		wxASSERT( head );
		TYPE oldvalue = head->data;

		RemoveAt((POSITION)head);
		
		return oldvalue;
	}

	
	// Removes the last element and returns the value it contained
	TYPE RemoveTail()
	{
		wxASSERT( tail );
		TYPE olddata = tail->data;
		
		RemoveAt((POSITION)tail);
		
		return olddata;
	}

	
	// Inserts a new element at the first position
	POSITION AddHead( ARG_TYPE newElement )
	{
		MYNODE* newnode = newNode( newElement );
		// we are adding to head!
		newnode->next = head;
		if ( head ) {
			head->prev = newnode;
		} else {
			tail = newnode;
		}
		head = newnode;
		count++;
		// return position to head
		return (POSITION)head;
	}

	
	// Appends an element to the list
	POSITION AddTail( ARG_TYPE newElement )
	{
		MYNODE* newnode = newNode( newElement );
		// and to the tail
		newnode->prev = tail;
		if ( tail ) {
			tail->next = newnode;
		} else {
			head = newnode;
		}
		tail = newnode;
		count++;
		return (POSITION)tail;
	}

	
	// Removes all elements
	void RemoveAll()
	{
		MYNODE * first = head;
		MYNODE* n;
		while ( first ) {
			n = first->next;
			freeNode( first );
			first = n;
		}
		head = tail = NULL;
		count = 0;
	}

	
	// Returns an iterator to the first element
	POSITION GetHeadPosition() const { 
		return (POSITION)head; 
	}
	
	
	// Returns an iterator to the last element
	POSITION GetTailPosition() const {
		return (POSITION)tail;
	}

	
	// Increments an iterator and returns a reference to the value at its original position
	TYPE& GetNext( POSITION& pos )
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		TYPE& data = n->data;
		n = n->next;
		pos = (POSITION)n;
		return data;
	}
	
	const TYPE& GetNext( POSITION& pos ) const
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		TYPE data = n->data;
		pos = (POSITION)(n->next);
		return data;
	}

	
	// Increments an iterator and returns a reference to the value at its original position
	TYPE& GetPrev( POSITION& pos )
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		TYPE& data = n->data;
		pos = (POSITION)(n->prev);
		return data;
	}
	
	const TYPE& GetPrev( POSITION& pos ) const
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		TYPE data = n->data;
		pos = (POSITION)(n->prev);
		return data;
	}

	
	// Returns a reference to the value at iterator pos
	TYPE& GetAt( POSITION pos )
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		return n->data;
	}
	
	const TYPE& GetAt( POSITION pos ) const
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		return n->data;
	}

	
	// Increments an iterator
	POSITION NextAt( POSITION pos )
	{
		MYNODE* n = (MYNODE*)pos;
		return ( n == NULL ) ? NULL : (POSITION)(n->next);
	}

	
	// Decrements an iterator
	POSITION PrevAt( POSITION pos )
	{
		MYNODE* n = (MYNODE*)pos;
		return ( n == NULL ) ? NULL : (POSITION)(n->prev);
	}
	

	// RemoveAt plus AddTail, without memory re-allocation.
	// Returns position of node that followed the one that was removed
	POSITION RecycleNodeAsTail( POSITION pos )
	{
		MYNODE* n = (MYNODE*)pos;
		
		if ( n == NULL || n == tail )
			return (POSITION)n;
			
		if ( n == head )  	  			// are we removing the head?
			head = n->next;				//  yes: then we have a new head
		else
			n->prev->next = n->next;	//  no: close backlink gap
			
		n->next->prev = n->prev;		// close forward gap
		n->prev = tail;					// link new tail to old tail
		tail->next = n;					// link old tail to new
		tail = n;						// we have a new tail

		MYNODE* r = n->next;
		tail->next = NULL;
		return (POSITION)r;
	};


	void RemoveAt( POSITION pos )
	{
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		
		// are we removing the head?
		if ( head == n ) {
			head = n->next; // yes
		} else {
			n->prev->next = n->next; // no
		}
		// is it tail too?
		if ( tail == n ) {
			tail = n->prev; // yes
		} else {
			n->next->prev = n->prev; // no
		}
		freeNode( n );
		count--;
	}


	// Inserts the value after the specified position
	// and update the iterator to contain the position of the new node
	void InsertAfter( POSITION pos, ARG_TYPE data )
	{
		MYNODE* n = (MYNODE*)pos;
		if ( n ) {
			MYNODE* newnode = newNode( data );
			newnode->prev = n;
			newnode->next = n->next;
			if (n->next) {
				n->next->prev = newnode;
			} else {
				tail = newnode; // it's tail
			}
			n->next = newnode;
			// increase count here. not after AddTail()
			count++;
		} else {
			pos = AddTail( data ); // just append
		}
	}

	
	// Returns the iterator which contains the searchValue or NULL if it isn't in the list
	POSITION Find( ARG_TYPE searchValue, POSITION startAfter = NULL ) const
	{
		if ( head ) {
			MYNODE* n = head;
			if ( startAfter ) {
				n = ((MYNODE*)startAfter)->next;
			}
			while ( n ) {
				if ( n->data == searchValue ) {
					return (POSITION)n;
				}
				n = n->next;
			}
		}
		return (POSITION)NULL;
	}

	
	// Returns the iterator for the nIndex'th item
	POSITION FindIndex( int nIndex ) const
	{
		MYNODE* n = head;
		for ( int i = 0; i < nIndex; i++ ) {
			if ( n ) {
				n = n->next;
			} else {
				return (POSITION)NULL; // past the end
			}
		}
		return (POSITION)n;
	}

	// Sets the value at "pos"
	void SetAt( POSITION& pos, ARG_TYPE data ) {
		MYNODE* n = (MYNODE*)pos;
		wxASSERT( n );
		n->data = data; 
	}
};

typedef CList<void*, void*> CPtrList;

template <class BASE_CLASS, class TYPE>
class CTypedPtrList : public CList<TYPE, TYPE>
{	// TYPE is a pointer, so use it also as ARG_TYPE.
public:
	CTypedPtrList( int WXUNUSED(nBlockSize) = 0 ) { }
};

#endif // CTYPEDPTRLIST_H
