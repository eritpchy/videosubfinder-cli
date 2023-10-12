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
#include <wx/dcmemory.h>
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(CButton, CBitmapButton)
	EVT_SIZE(CButton::OnSize)
END_EVENT_TABLE()

CButton::CButton(wxWindow* parent,
	wxWindowID id,
	wxColour& button_color,
	wxColour& button_color_focused,
	wxColour& button_color_selected,
	wxColour& buttons_border_colour,
	const wxString& label,
	const wxPoint& pos,
	const wxSize& size) : CBitmapButton(parent, id, pos, size)
{
	m_pParent = parent;
	m_pFont = NULL;
	m_p_text_colour = NULL;
	m_p_label = &label;
	m_p_button_color = &button_color;
	m_p_button_color_focused = &button_color_focused;
	m_p_button_color_selected = &button_color_selected;
	m_p_buttons_border_colour = &buttons_border_colour;

	wxSizeEvent event;
	this->OnSize(event);
}

void CButton::FillButtonBitmap(wxBitmap& bmp, wxColour parent_colour, wxColour button_colour, wxColour button_border_colour)
{
	int w, h;
	int cr = 5;
	int bw = 1;

	w = bmp.GetWidth();
	h = bmp.GetHeight();

	wxMemoryDC dc;
	dc.SelectObject(bmp);

	dc.SetPen(wxPen(parent_colour));
	dc.SetBrush(wxBrush(parent_colour));
	dc.DrawRectangle(0, 0, w, h);

	dc.SetPen(wxPen(button_border_colour, bw));
	dc.SetBrush(wxBrush(button_colour));
	dc.DrawCircle(cr, cr, cr);
	dc.DrawCircle(w - cr - 1, cr, cr);
	dc.DrawCircle(cr, h - cr - 1, cr);
	dc.DrawCircle(w - cr - 1, h - cr - 1, cr);

	dc.SetPen(wxPen(button_colour));
	dc.DrawRectangle(cr, 0, w - (2 * cr), h);
	dc.DrawRectangle(0, cr, w, h - (2 * cr));

	dc.SetPen(wxPen(button_border_colour, bw));
	dc.DrawLine(cr, 0, w - cr, 0);
	dc.DrawLine(cr, h - 1, w - cr, h - 1);
	dc.DrawLine(0, cr, 0, h - cr);
	dc.DrawLine(w - 1, cr, w - 1, h - cr);

	if (m_p_label->size() > 0)
	{
		if (m_pFont) dc.SetFont(*m_pFont);		
		if (m_p_text_colour) dc.SetTextForeground(*m_p_text_colour);
		wxSize ts = dc.GetMultiLineTextExtent(*m_p_label);

		dc.DrawText(*m_p_label, (w - ts.x) / 2, (h - ts.y) / 2);
	}
}

void CButton::OnSize(wxSizeEvent& event)
{
	int w, h;

	this->GetClientSize(&w, &h);

	if ((w > 0) && (h > 0))
	{
		wxBitmap bmp(w, h), bmp_focused(w, h), bmp_selected(w, h);
		FillButtonBitmap(bmp, m_pParent->GetBackgroundColour(), *m_p_button_color, *m_p_buttons_border_colour);
		FillButtonBitmap(bmp_focused, m_pParent->GetBackgroundColour(), *m_p_button_color_focused, *m_p_buttons_border_colour);
		FillButtonBitmap(bmp_selected, m_pParent->GetBackgroundColour(), *m_p_button_color_selected, *m_p_buttons_border_colour);
		SetBitmaps(bmp.ConvertToImage(), bmp_focused.ConvertToImage(), bmp_selected.ConvertToImage());

		this->Refresh(true);
	}

	event.Skip();
}

void CButton::SetLabel(const wxString& label)
{
	m_p_label = &label;
	RefreshData();
}

void CButton::SetFont(wxFont& font)
{
	m_pFont = &font;
	RefreshData();
}

void CButton::SetTextColour(wxColour& colour)
{
	m_p_text_colour = &colour;
	wxSizeEvent event;
	this->OnSize(event);
}

void CButton::SetMinSize(wxSize& size)
{
	m_min_size = size;
}

void CButton::RefreshData()
{
	wxSizer* pSizer = GetContainingSizer();
	if (pSizer)
	{
		wxMemoryDC dc;
		if (m_pFont) dc.SetFont(*m_pFont);
		wxSize best_size = dc.GetMultiLineTextExtent(*m_p_label);
		wxSize cur_size = this->GetSize();
		wxSize cur_client_size = this->GetClientSize();
		wxSize opt_size;
		best_size.x += cur_size.x - cur_client_size.x + 20;
		best_size.y += cur_size.y - cur_client_size.y + 10;

		opt_size.x = std::max<int>(best_size.x, m_min_size.x);
		opt_size.y = std::max<int>(best_size.y, m_min_size.y);

		if (opt_size != cur_size)
		{
			bool res = pSizer->SetItemMinSize(this, opt_size);
			pSizer->Layout();
		}
	}

	wxSizeEvent event;
	this->OnSize(event);
}
