// This file is part of the aMule project.
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


#include <wx/slider.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/textctrl.h>

#include "PPgGuiTweaks.h"	// Interface declarations
#include "muuli_wdr.h"		// Needed for PreferencesGuiTweaksTab

IMPLEMENT_DYNAMIC_CLASS(CPPgGuiTweaks,wxPanel)


CPPgGuiTweaks::CPPgGuiTweaks(wxWindow* parent)
: wxPanel(parent,CPPgGuiTweaks::IDD)
{
	//wxNotebook* book= (wxNotebook*) parent;
	PreferencesGuiTweaksTab(this,TRUE);
	//book->AddPage(this,_("GUI Tweaks"));
}

CPPgGuiTweaks::~CPPgGuiTweaks()
{
}

void CPPgGuiTweaks::Localize(void)
{
}

void CPPgGuiTweaks::LoadSettings(void)
{
}

void CPPgGuiTweaks::OnApply()
{
}
