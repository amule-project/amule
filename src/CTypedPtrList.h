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

template<class TYPE, class ARG_TYPE = TYPE const&>
  class CList : public wxObject {
  private:
  // Each node of the list
    struct MyNode {
      struct MyNode* next;
      struct MyNode* prev;
      TYPE data;
    };
    typedef MyNode MYNODE;

  protected:
    MYNODE* head;	// Our list
    MYNODE* tail;		// Last node
    int count;			// Number of nodes

    void freeNode(MYNODE* n) { delete n; }
 
    MYNODE* newNode(ARG_TYPE data)
    {
      MYNODE* n = new MYNODE;
      n->prev = n->next = NULL;
      n->data = data;
      return n;
    }

	MYNODE* newNode() 
	{
    	MYNODE* n=new MYNODE;
    	n->prev=n->next=NULL;
    	return n;
  	}

  public:
    CList(int none = 0) { head = NULL; tail = NULL; count = 0; }
    ~CList() { RemoveAll(); }

    int GetCount(void) const { return count; /*g_list_length(head);*/ }
    int GetSize(void) const { return count; /*g_list_length(head);*/ }
    bool IsEmpty(void) const { return (count == 0); /*g_list_length(head)==0;*/ }
  
    TYPE& GetHead(void)
    {
      wxASSERT(head);
      return head->data;
    }
  
    TYPE GetHead(void) const
    {
      wxASSERT(head);
      return head->data;
    }
  
    TYPE& GetTail(void)
    {
      wxASSERT(tail);				
      return tail->data;
    }
  
    TYPE GetTail(void) const
    {
      if(tail)
	return tail->data;
      else
	return (TYPE)NULL;
    }
	
    TYPE RemoveHead(void)
    {
      wxASSERT(head);
      MYNODE* old = head;
      TYPE oldvalue = old->data;

      // advance head
      head = head->next; 
      // if not null, then there are still items in the list
      
      if (head) {
		head->prev = NULL; // and this is the head
	} else {
		tail = NULL; // otherwise.. no more list and tail
	}

      freeNode(old);
      count--;
      return oldvalue;
    }

  TYPE RemoveTail(void)
  {
    if(tail) {
      MYNODE* old = tail;
      TYPE olddata = tail->data;

      // move tail to previous node
      tail = tail->prev;
      // is there list anymore?
      if (tail) {
		tail->next = NULL; // yes. this is the tail
	} else {
		head = NULL; // no. list has vanished
	}
	
      freeNode(old);
      count--;
      return olddata;
    } else {
    	// iterating on an empty list
    	return (TYPE)NULL;
    }
  }
	
  POSITION AddHead(ARG_TYPE uusi)
  {
    MYNODE* newnode = newNode(uusi);
    // we are adding to head!
    newnode->next = head;
    if(head) {
      head->prev = newnode;
    } else {
      tail = newnode;
    }
    head = newnode;
    count++;
    // return position to head
    return (POSITION)head;
  }

  POSITION AddTail(ARG_TYPE uusi)
  {
    MYNODE* newnode = newNode(uusi);
    // and to the tail
    newnode->prev = tail;
    if (tail) {
      tail->next = newnode;
    } else {
      head = newnode;
    }
    tail = newnode;
    count++;
    return (POSITION)tail;
  }

  POSITION Append(ARG_TYPE uusi)
  { 
    // addtail
    return AddTail(uusi);
  }

  void RemoveAll(void)
  {
    MYNODE* first = head;
    MYNODE* n;
    while(first)
    {
      n = first->next;
      freeNode(first);
      first = n;
    }
    head = tail = NULL;
    count = 0;
  }

  POSITION GetHeadPosition(void) const { return (POSITION)head; }
  POSITION GetTailPosition(void) const { return (POSITION)tail; }

  TYPE& GetNext(POSITION& pos)
  {
    MYNODE* n = (MYNODE*)pos;
    wxASSERT(n);
    TYPE& data = n->data;
    n = n->next;
    pos = (POSITION)n;
    return data;
  }

  TYPE GetNext(POSITION& pos) const
  {
    MYNODE* n = (MYNODE*)pos;
    if(n) {
      TYPE data = n->data;
      pos = (POSITION)(n->next);
      return data;
    } else {
      return (TYPE)NULL;
    }
  }

  TYPE& GetPrev(POSITION& pos)
  {
    MYNODE*n = (MYNODE*)pos;
    wxASSERT(n);
    TYPE& data = n->data;
    pos = (POSITION)(n->prev);	  
    return data;
  }

  TYPE GetPrev(POSITION& pos) const
  {
    MYNODE* n = (MYNODE*)pos;
    if (n) {
      TYPE data = n->data;
      pos = (POSITION)(n->prev);
      return data;
    } else {
      return (TYPE)NULL;
    }
  }

  TYPE& GetAt(POSITION pos)
  {
    MYNODE* n = (MYNODE*)pos;
    wxASSERT(n);
    return n->data;
  }

  TYPE GetAt(POSITION pos) const
  {
    MYNODE* n = (MYNODE*)pos;
    if(n) {
      return n->data;
    } else {
    	return (TYPE)NULL;
    }
  }

  POSITION NextAt(POSITION pos)
  {
    MYNODE* n = (MYNODE*)pos;
    return (n == NULL) ? NULL : (POSITION)(n->next);
  }

  POSITION PrevAt(POSITION pos)
  {
    MYNODE* n = (MYNODE*)pos;
    return (n == NULL) ? NULL : (POSITION)(n->prev);
  }

  TYPE* GetDataPtr(POSITION pos) 
  {
    return pos==NULL ? NULL : (TYPE*)&(((MYNODE*)pos)->data);
  };

  POSITION RecycleNodeAsTail(POSITION pos, ARG_TYPE uusi) 
  {
  // RemoveAt plus AddTail, without memory re-allocation
  // returns position of node that followed the one that was removed
    MYNODE *n =(MYNODE*)pos;
	MYNODE *r;
    if (n == NULL)
      return NULL;
    if (n != tail) 
    {
      if(n == head) 	  		// are we removing the head?
        head = n->next;			//  yes: we have a new head
      else
        n->prev->next = n->next;//  no: close backlink gap
      n->next->prev = n->prev;	// close forward gap
      n->prev = tail;			// link new tail to old tail
      tail->next = n;			// link old tail to new
      tail=n;					// we have a new tail
    }
    n->data = uusi;				// refill tail node
    r = n->next;
    n->next = NULL;
    return (POSITION)r;
  };


  POSITION RecycleNodeAsTail(POSITION pos) 
  {
    MYNODE*n = (MYNODE*)pos;
    if (n==NULL || n==tail)
      return (POSITION)n;
    if(n == head) 	  			// are we removing the head?
      head = n->next;			//  yes: then we have a new head
    else
      n->prev->next = n->next;	//  no: close backlink gap
    n->next->prev = n->prev;	// close forward gap
    n->prev = tail;				// link new tail to old tail
    tail->next = n;				// link old tail to new
    tail = n;					// we have a new tail
    /*tail->data = uusi;*/		// tail does not get refilled in this variant
    MYNODE* r = n->next;
    tail->next = NULL;
    return (POSITION)r;
  };


  POSITION RecycleHeadAsTail(ARG_TYPE uusi) 
  {
  // returns new head 
    MYNODE *n = head;
    if (n==NULL  ||  n==tail)
      return n;
    head = n->next;				// we have a new head
    head->prev = NULL;
    n->prev = tail;				// link new tail to old tail
    tail->next = n;				// link old tail to new
    tail = n;					// we have a new tail
    tail->data = uusi;			// refill tail node
    tail->next = NULL;
    return (POSITION)head;
  };


  POSITION RecycleHeadAsTail() 
  {
  // returns new head 
    MYNODE *n = head;
    if (n==NULL  ||  n==tail)
      return (POSITION)n;
    head = n->next;				// we have a new head
    head->prev = NULL;
    n->prev = tail;				// link new tail to old tail
    tail->next = n;				// link old tail to new
    tail = n;					// we have a new tail
    /*tail->data = uusi;*/		// tail does not get refilled in this variant
    tail->next = NULL;
    return (POSITION)head;
  };


  void RemoveAt(POSITION pos)
  {
    MYNODE* n = (MYNODE*)pos;
    if(n) {
      // are we removing the head?
      if(head == n) {
		head = n->next; // yes
	} else {
		n->prev->next = n->next; // no
	}
      // is it tail too?
      if(tail == n) {
		tail = n->prev; // yes
	} else {
		n->next->prev = n->prev; // no
	}
      freeNode(n);
      count--;
    } 
  }

  TYPE& DeAttach(POSITION pos)
  {
    MYNODE* n = (MYNODE*)pos;
    if(n) {
      // are we removing the head?
      if(head == n) {
		head = n->next; // yes
	} else {
		n->prev->next = n->next; // no
	}
      // is it tail too?
      if(tail == n) {
		tail = n->prev; // yes
	} else {
		n->next->prev = n->prev; // no
	}
      count--;  
	 TYPE value = n->data;
	 freeNode(n);
	 return (value);
     } 
	return (NULL);
  }  
  
  
  void InsertAfter(POSITION pos, ARG_TYPE data)
  {
    MYNODE* n = (MYNODE*)pos;
    if(n) {
      MYNODE* newnode = newNode(data);
      newnode->prev = n;
      newnode->next = n->next;
      if(n->next) {
		n->next->prev = newnode;
	} else {
		tail = newnode; // it's tail
	}
      n->next = newnode;
      // increase count here. not after AddTail()
      count++;
    } else {
      pos = AddTail(data); // just append
    }
  }

  POSITION Find(ARG_TYPE searchValue, POSITION startAfter = NULL) const
  {
    MYNODE* n = head;
    if(startAfter) {
      n = ((MYNODE*)startAfter)->next;
    }
    while(n)
    {
      if(n->data == searchValue) {
		return (POSITION)n;
	}
      n = n->next;
    }
    return (POSITION)NULL;
  }

  POSITION FindIndex(int nIndex) const
  {
    MYNODE* n = head;
    for(int i = 0; i < nIndex; i++) {
      if(n) {
		n = n->next;
	} else {
		return (POSITION)NULL; // past the end
	}
    }
    return (POSITION)n;
  }

  // Adding some new M$ CList function
  void SetAt(POSITION pos, ARG_TYPE data) { ((MYNODE *)pos)->data = data; }		

  // FINISHED CLIST CLASS
};

typedef CList<void*, void*> CPtrList;

template<class BASE_CLASS, class TYPE>
  class CTypedPtrList : public CList<TYPE, TYPE> {	// TYPE is a pointer, so use it also as ARG_TYPE.
  public:
    CTypedPtrList(int nBlockSize = 0) { }
  };

#endif // CTYPEDPTRLIST_H
