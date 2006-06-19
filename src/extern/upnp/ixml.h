///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// Modified by Angel Vidal Veiga (kry@amule.org) for the aMule project.
// Really crippled version of the original header. Keep just what we need.

#ifndef _IXML_H_
#define _IXML_H_

typedef int BOOL;

#define DOMString   char *

#ifndef IN
#define IN
#endif

/**@name DOM Interfaces 
 * The Document Object Model consists of a set of objects and interfaces
 * for accessing and manipulating documents.  IXML does not implement all
 * the interfaces documented in the DOM2-Core recommendation but defines
 * a subset of the most useful interfaces.  A description of the supported
 * interfaces and methods is presented in this section.
 *
 * For a complete discussion on the object model, the object hierarchy,
 * etc., refer to section 1.1 of the DOM2-Core recommendation.
 */

//@{


/*================================================================
*
*   DOM node type 
*
*
*=================================================================*/
typedef enum
{
    eINVALID_NODE                   = 0,
    eELEMENT_NODE                   = 1,
    eATTRIBUTE_NODE                 = 2,
    eTEXT_NODE                      = 3,
    eCDATA_SECTION_NODE             = 4,
    eENTITY_REFERENCE_NODE          = 5,
    eENTITY_NODE                    = 6,                
    ePROCESSING_INSTRUCTION_NODE    = 7,
    eCOMMENT_NODE                   = 8,
    eDOCUMENT_NODE                  = 9,
    eDOCUMENT_TYPE_NODE             = 10,
    eDOCUMENT_FRAGMENT_NODE         = 11,
    eNOTATION_NODE                  = 12,

}   IXML_NODE_TYPE;

/*================================================================
*
*   DOM data structures
*
*
*=================================================================*/

typedef struct _IXML_Document *Docptr;

typedef struct _IXML_Node    *Nodeptr;

typedef struct _IXML_Node
{
    DOMString       nodeName;
    DOMString       nodeValue;
    IXML_NODE_TYPE  nodeType;
    DOMString       namespaceURI;
    DOMString       prefix;
    DOMString       localName;
    BOOL            readOnly;

    Nodeptr         parentNode;
    Nodeptr         firstChild;
    Nodeptr         prevSibling;
    Nodeptr         nextSibling;
    Nodeptr         firstAttr;
    Docptr          ownerDocument;

} IXML_Node;

typedef struct _IXML_Document
{
    IXML_Node    n;
} IXML_Document;

typedef struct _IXML_Element
{
    IXML_Node   n;
    DOMString   tagName;

} IXML_Element;

typedef struct _IXML_NamedNodeMap
{
    IXML_Node                 *nodeItem;
    struct _IXML_NamedNodeMap *next;
} IXML_NamedNodeMap;

//@} 

#endif  // _IXML_H_
// File_checked_for_headers
