                              //Button.cpp//                                
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
#include "Button.h"

CButton::CButton(wxWindow* parent,
	wxWindowID id,
	const wxString& label,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& validator,
	const wxString& name) : wxButton(parent, id, label, pos, size, style, validator, name)
{
	m_pFont = NULL;
}

void CButton::SetFont(wxFont& font)
{
	m_pFont = &font;
	wxButton::SetFont(*m_pFont);
}

void CButton::RefreshData()
{
	if (m_pFont) wxButton::SetFont(*m_pFont);
}
