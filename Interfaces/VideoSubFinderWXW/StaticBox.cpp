                              //StaticBox.cpp//                                
//////////////////////////////////////////////////////////////////////////////////
//																				//
// Author:  Simeon Kosnitsky													//
//          skosnits@gmail.com													//
//																				//
// License:																		//
//     This software is released into the public domain.  You are free to use	//
//     it in any way you like, except that you may not sell this source code.	//
//																				//
//     This software is provided "as is" with no expressed or implied warranty.	//
//     I accept no liability for any damage or loss of business that this		//
//     software may cause.														//
//																				//
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "StaticBox.h"

CStaticBox::CStaticBox(wxWindow* parent, wxWindowID id,
    const wxString& label,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name) : wxStaticBox(parent, id, label, pos, size, style, name)
{
}

void CStaticBox::SetFont(wxFont& font)
{
	m_pFont = &font;
	wxStaticBox::SetFont(*m_pFont);
}

void CStaticBox::SetTextColour(wxColour& colour)
{
    m_pTextColour = &colour;
    wxStaticBox::SetForegroundColour(*m_pTextColour);
}

void CStaticBox::RefreshData()
{
	if (m_pFont) wxStaticBox::SetFont(*m_pFont);
    if (m_pTextColour) wxStaticBox::SetForegroundColour(*m_pTextColour);
}
