/////////////////////////////////////////////////////////////////////////////
// Name:        wxcas.cpp
// Purpose:     wxCas App
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2004/04/15
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "wxcas.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wxcas.h"
#include "wxcasframe.h"

// Application implementation
IMPLEMENT_APP (WxCas)
     bool WxCas::OnInit ()
{
	wxFrame *frame = new WxCasFrame (_("wxCas, aMule online statistics"));

	// Show all
	frame->Show (TRUE);
	SetTopWindow (frame);
	return true;
}
