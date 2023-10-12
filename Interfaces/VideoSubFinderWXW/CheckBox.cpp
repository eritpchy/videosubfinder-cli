                              //CheckBox.cpp//                                
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
#include <wx/dcmemory.h>
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(CCheckBox, wxCheckBox)
	EVT_SIZE(CCheckBox::OnSize)
END_EVENT_TABLE()

CCheckBox::CCheckBox(wxWindow* parent,
	wxWindowID id,
	bool* p_val,
	const wxString& label,
	const wxPoint& pos,
	const wxSize& size,
	long check_box_style,
	long text_style,
	long panel_style) : wxPanel(parent, id, pos, size, panel_style, wxT(""))
{
	m_pParent = parent;
	m_p_val = p_val;
	m_p_label = &label;

	m_text_style = text_style;
	m_check_box_style = check_box_style;

	m_pST = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, m_text_style, wxStaticTextNameStr);
	m_pCB = new wxCheckBox(this, id, wxT(""), wxDefaultPosition, wxDefaultSize, ((m_check_box_style ^ wxALIGN_RIGHT) ^ wxALIGN_LEFT));
	
	m_pCB->SetValue(*m_p_val);

	m_pCB->Bind(wxEVT_CHECKBOX, &CCheckBox::OnCheckBoxEvent, this);	
}

void CCheckBox::SetFont(wxFont& font)
{
	m_pFont = &font;
	if (m_pFont) m_pST->SetFont(*m_pFont);

	wxSizeEvent event;
	OnSize(event);
}

void CCheckBox::SetTextColour(wxColour& colour)
{
	m_pTextColour = &colour;
	m_pST->SetForegroundColour(*m_pTextColour);
}

void CCheckBox::SetBackgroundColour(wxColour& colour)
{
	m_pBackgroundColour = &colour;
	wxPanel::SetBackgroundColour(*m_pBackgroundColour);
	m_pST->SetBackgroundColour(*m_pBackgroundColour);
}

void CCheckBox::SetLabel(const wxString& label)
{
	m_p_label = &label;
	m_pST->SetLabel(*m_p_label);
	wxSizeEvent event;
	OnSize(event);
}

void CCheckBox::SetMinSize(wxSize& size)
{
	m_min_size = size;
}

void CCheckBox::RefreshData()
{	
	m_pST->SetLabel(*m_p_label);
	m_pCB->SetValue(*m_p_val);
	if (m_pFont) m_pST->SetFont(*m_pFont);
	if (m_pTextColour) m_pST->SetForegroundColour(*m_pTextColour);
	if (m_pBackgroundColour)
	{
		wxPanel::SetBackgroundColour(*m_pBackgroundColour);
		m_pST->SetBackgroundColour(*m_pBackgroundColour);
	}

	wxSizer* pSizer = GetContainingSizer();
	if (pSizer)
	{
		wxMemoryDC dc;
		if (m_pFont) dc.SetFont(*m_pFont);
		wxSize best_size = dc.GetMultiLineTextExtent(*m_p_label);
		wxSize cur_size = this->GetSize();
		wxSize cur_client_size = this->GetClientSize();
		wxSize cb_size = m_pCB->GetSize();
		wxSize opt_size;
		best_size.x += cur_size.x - cur_client_size.x + 6 + (m_cb_offset * 2) + cb_size.x;
		best_size.y += cur_size.y - cur_client_size.y + 6;

		opt_size.x = std::max<int>(best_size.x, m_min_size.x);
		opt_size.y = std::max<int>({best_size.y, m_min_size.y, cb_size.y});

		if (opt_size != cur_size)
		{
			pSizer->SetItemMinSize(this, opt_size);
			pSizer->Layout();
		}
	}

	wxSizeEvent event;
	OnSize(event);
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

	evt.Skip();
}

void CCheckBox::OnSize(wxSizeEvent& event)
{
	int w, h, stw, sth, cbw, cbh, x, y, stp_beg_x, stp_end_x, stp_w, cb_x, cb_y = m_cb_offset;

	this->GetClientSize(&w, &h);

	if ((w > 0) && (h > 0))
	{
		{
			wxMemoryDC dc;
			if (m_pFont) dc.SetFont(*m_pFont);
			wxSize text_size = dc.GetMultiLineTextExtent(*m_p_label);
			wxSize st_cur_size = m_pST->GetSize();
			wxSize st_cur_client_size = m_pST->GetClientSize();
			stw = text_size.x + st_cur_size.x - st_cur_client_size.x;
			sth = text_size.y + st_cur_size.y - st_cur_client_size.y;
		}

		m_pCB->GetSize(&cbw, &cbh);

		if (m_check_box_style & wxALIGN_RIGHT)
		{
			stp_beg_x = 0;
			stp_end_x = w - cbw - 1 - (m_cb_offset * 2);
			cb_x = stp_end_x + 1 + m_cb_offset;
		}
		else // left aligned
		{
			stp_beg_x = cbw + (m_cb_offset * 2) + 1; // add +1 due to text become too close to check box
			stp_end_x = w - 1;
			cb_x = m_cb_offset;
		}

		stp_w = stp_end_x - stp_beg_x + 1 - 1; // add -1 due to text become too close to check box

		if (m_check_box_style & wxALIGN_CENTER_VERTICAL)
		{
			cb_y = (h - sth) / 2;
		}

		if (m_text_style & wxALIGN_CENTER_HORIZONTAL)
		{
			x = stp_beg_x + (stp_w - stw) / 2;
		}
		else if (m_text_style & wxALIGN_RIGHT)
		{
			x = stp_beg_x + stp_w - stw;
		}
		else
		{
			x = stp_beg_x;
		}

		if (m_text_style & wxALIGN_CENTER_VERTICAL)
		{
			y = (h - sth) / 2;
		}
		else if (m_text_style & wxALIGN_BOTTOM)
		{
			y = h - sth;
		}
		else
		{
			y = 0;
		}

		m_pST->SetSize(x, y, stw, sth);
		m_pCB->SetPosition(wxPoint(cb_x, cb_y));
	}

	event.Skip();
}
