// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2004 Carlo Wood ( carlo@alinoe.com )
//

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

typedef struct  {
  VoidHash::iterator it;
  VoidList::Node* node;
} _POSITION;

typedef _POSITION* POSITION;

#endif // POSITION_H
