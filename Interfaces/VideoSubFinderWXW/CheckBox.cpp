                              //DataGrid.h//                                
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
#include "CheckBox.h"

BEGIN_EVENT_TABLE(CCheckBox, wxCheckBox)
	EVT_CHECKBOX(wxID_ANY, CCheckBox::OnCheckBoxEvent)
END_EVENT_TABLE()

CCheckBox::CCheckBox(wxWindow* parent,
	wxWindowID id,
	bool* p_val,
	const wxString& label,		
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxCheckBox(parent, id, label, pos, size, style)
{
	m_p_val = p_val;
	this->SetValue(*m_p_val);
}

void CCheckBox::OnCheckBoxEvent(wxCommandEvent& evt)
{
	if (evt.IsChecked())
	{
		*m_p_val = true;
	}
	else
	{
		*m_p_val = false;
	}
}
