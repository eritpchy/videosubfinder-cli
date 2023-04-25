                              //TextCtrl.cpp//                                
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
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(CTextCtrl, wxTextCtrl)
	EVT_TEXT(wxID_ANY, CTextCtrl::OnText)
	EVT_TEXT_ENTER(wxID_ANY, CTextCtrl::OnTextEnter)
	EVT_KILL_FOCUS(CTextCtrl::OnKillFocus)
END_EVENT_TABLE()

void CTextCtrl::OnKillFocus(wxFocusEvent& event)
{
	if (m_re_expr.size() > 0)
	{
		wxString val = GetValue();
		ValidateAndSaveData(val);
	}
	event.Skip();
}

CTextCtrl::CTextCtrl(wxWindow* parent,
	wxWindowID id,
	wxString str_val,
	wxString  re_expr,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxTextCtrl(parent, id, wxEmptyString, pos, size, style | wxTE_PROCESS_ENTER), m_re(re_expr)
{
	m_pParent = parent;
	m_str_val = str_val;
	m_re_expr = re_expr;
	ChangeValue(str_val);
}

CTextCtrl::CTextCtrl(wxWindow* parent,
	wxWindowID id,
	wxString* p_str_val,
	wxString  re_expr,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxTextCtrl(parent, id, wxEmptyString, pos, size, style | wxTE_PROCESS_ENTER), m_re(re_expr)
{
	m_pParent = parent;
	m_p_str_val = p_str_val;
	m_re_expr = re_expr;
	ChangeValue(*m_p_str_val);
}

CTextCtrl::CTextCtrl(wxWindow* parent,
	wxWindowID id,
	double* p_f_val,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxTextCtrl(parent, id, wxEmptyString, pos, size, style)
{
	m_pParent = parent;
	m_p_f_val = p_f_val;
	ChangeValue(wxString::Format(wxT("%f"), *m_p_f_val));
}

void CTextCtrl::OnTextEnter(wxCommandEvent& evt)
{
	if (m_re_expr.size() > 0)
	{
		wxString val = GetValue();
		ValidateAndSaveData(val);
	}
}

bool CTextCtrl::ValidateAndSaveData(wxString val)
{
	bool res = true;

	if (m_re_expr.size() > 0)
	{
		if (m_p_str_val)
		{
			if (m_re.Matches(val))
			{
				*m_p_str_val = val;
			}
			else
			{
				res = false;
				ChangeValue(*m_p_str_val);
			}
		}
		else
		{
			if (m_re.Matches(val))
			{
				m_str_val = val;
			}
			else
			{
				res = false;
				ChangeValue(m_str_val);
			}
		}
	}

	return res;
}

void CTextCtrl::OnText(wxCommandEvent& evt)
{
	if (m_re_expr.size() == 0)
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
}

void CTextCtrl::SetValue(const wxString& value)
{
	if (ValidateAndSaveData(value))
	{
		ChangeValue(value);
	}
}

void CTextCtrl::SetFont(wxFont& font)
{
	m_pFont = &font;
	wxTextCtrl::SetFont(*m_pFont);
}

void CTextCtrl::SetTextColour(wxColour& colour)
{
	m_pTextColour = &colour;
	wxTextCtrl::SetForegroundColour(*m_pTextColour);
}

void CTextCtrl::SetBackgroundColour(wxColour& colour)
{
	m_pBackgroundColour = &colour;
	wxTextCtrl::SetBackgroundColour(*m_pBackgroundColour);
}

void CTextCtrl::SetMinSize(wxSize& size)
{
	m_min_size = size;
}

void CTextCtrl::RefreshData()
{
	if (m_pFont) wxTextCtrl::SetFont(*m_pFont);
	if (m_pTextColour) wxTextCtrl::SetForegroundColour(*m_pTextColour);
	if (m_pBackgroundColour) wxTextCtrl::SetBackgroundColour(*m_pBackgroundColour);

	if (m_p_str_val)
	{
		ChangeValue(*m_p_str_val);
	}
	else if (m_p_f_val)
	{
		ChangeValue(wxString::Format(wxT("%f"), *m_p_f_val));
	}

	wxSizer* pSizer = GetContainingSizer();
	if (pSizer)
	{
		wxSize text_size = this->GetTextExtent(this->GetValue());
		wxSize best_size = this->GetSizeFromTextSize(text_size);
		wxSize cur_size = this->GetSize();
		wxSize cur_client_size = this->GetClientSize();
		wxSize opt_size;

		opt_size.x = std::max<int>(best_size.x, m_min_size.x);
		opt_size.y = std::max<int>(best_size.y, m_min_size.y);

		if (opt_size != cur_size)
		{
			pSizer->SetItemMinSize(this, opt_size);
			pSizer->Layout();
		}
	}
}
