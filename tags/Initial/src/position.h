#ifndef POSITION_H
#define POSITION_H

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/list.h>		// Needed for WX_DECLARE_LIST
#include <wx/hashmap.h>		// Needed for WX_DECLARE_HASH_MAP

#ifndef NULL
#define NULL 0
#endif


WX_DECLARE_LIST(void, VoidList);
WX_DECLARE_HASH_MAP(unsigned int, void*, wxIntegerHash, wxIntegerEqual, VoidHash);

struct _POSITION {
  VoidHash::iterator it;
  VoidList::Node* node;
};

typedef _POSITION* POSITION;

#endif // POSITION_H
