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
#include "TextCtrl.h"

BEGIN_EVENT_TABLE(CTextCtrl, wxTextCtrl)
	EVT_TEXT(wxID_ANY, CTextCtrl::OnTextCtrlEvent)
END_EVENT_TABLE()

CTextCtrl::CTextCtrl(wxWindow* parent,
	wxWindowID id,
	wxString* p_str_val,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxTextCtrl(parent, id, wxEmptyString, pos, size, style)
{
	m_p_str_val = p_str_val;
	ChangeValue(*m_p_str_val);
}

CTextCtrl::CTextCtrl(wxWindow* parent,
	wxWindowID id,
	double* p_f_val,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxTextCtrl(parent, id, wxEmptyString, pos, size, style)
{
	m_p_f_val = p_f_val;
	ChangeValue(wxString::Format(wxT("%f"), *m_p_f_val));
}

void CTextCtrl::OnTextCtrlEvent(wxCommandEvent& evt)
{
	if (m_p_str_val)
	{
		*m_p_str_val = GetValue();
	}
	else if (m_p_f_val)
	{
		double value;
		if (GetValue().ToDouble(&value))
		{
			*m_p_f_val = value;
		}		
	}
}

void CTextCtrl::RefreshData()
{
	if (m_p_str_val)
	{
		ChangeValue(*m_p_str_val);
	}
	else if (m_p_f_val)
	{
		ChangeValue(wxString::Format(wxT("%f"), *m_p_f_val));
	}
}
