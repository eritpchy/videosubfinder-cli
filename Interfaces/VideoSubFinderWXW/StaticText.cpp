                              //StaticText.cpp//                                
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

#include "StaticText.h"
#include <wx/dcmemory.h>
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(CStaticText, wxPanel)
	EVT_SIZE(CStaticText::OnSize)
END_EVENT_TABLE()

CStaticText::CStaticText(  wxWindow* parent,
				const wxString& label,
				wxWindowID id,				
				long text_style,
				long panel_style,
				const wxPoint& pos,
				const wxSize& size)				
		:wxPanel( parent, id, pos, size, panel_style, wxT(""))
{
	m_pParent = parent;
	m_p_label = &label;

	m_pST = new wxStaticText(this, wxID_ANY, *m_p_label, wxDefaultPosition, wxDefaultSize, text_style, wxStaticTextNameStr);

	m_text_style = text_style;	
}

CStaticText::~CStaticText()
{
}

void CStaticText::SetFont(wxFont& font)
{
	m_pFont = &font;
	m_pST->SetFont(*m_pFont);

	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::SetTextColour(wxColour& colour)
{
	m_pTextColour = &colour;
	m_pST->SetForegroundColour(*m_pTextColour);
}

void CStaticText::SetBackgroundColour(wxColour& colour)
{
	m_pBackgroundColour = &colour;
	wxPanel::SetBackgroundColour(*m_pBackgroundColour);
	m_pST->SetBackgroundColour(*m_pBackgroundColour);
}

wxSize CStaticText::GetOptimalSize(int add_gap)
{
	wxMemoryDC dc;
	if (m_pFont) dc.SetFont(*m_pFont);
	wxSize best_size = dc.GetMultiLineTextExtent(*m_p_label);
	wxSize cur_size = this->GetSize();
	wxSize cur_client_size = this->GetClientSize();
	wxSize opt_size = cur_size;
	best_size.x += cur_size.x - cur_client_size.x + add_gap;
	best_size.y += cur_size.y - cur_client_size.y + add_gap;

	if (m_allow_auto_set_min_width)
	{
		opt_size.x = std::max<int>(best_size.x, m_min_size.x);
	}
	else
	{
		opt_size.x = 10;
	}

	opt_size.y = std::max<int>(best_size.y, m_min_size.y);
	
	return opt_size;
}

void CStaticText::RefreshData()
{
	m_pST->SetLabel(*m_p_label);

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
		wxSize cur_size = this->GetSize();
		wxSize opt_size = GetOptimalSize();

		if (opt_size != cur_size)
		{
			pSizer->SetItemMinSize(this, opt_size);
			pSizer->Layout();
		}
	}

	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::SetMinSize(wxSize& size)
{
	m_min_size = size;
}

void CStaticText::SetLabel(const wxString& label)
{
	m_p_label = &label;
	m_pST->SetLabel(*m_p_label);
	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::OnSize(wxSizeEvent& event)
{
	int w, h, tw, th, x, y;
	
    this->GetClientSize(&w, &h);

	{
		wxMemoryDC dc;
		if (m_pFont) dc.SetFont(*m_pFont);
		wxSize st_best_size = dc.GetMultiLineTextExtent(*m_p_label);
		wxSize st_cur_size = m_pST->GetSize();
		wxSize st_cur_client_size = m_pST->GetClientSize();
		st_best_size.x += st_cur_size.x - st_cur_client_size.x;
		st_best_size.y += st_cur_size.y - st_cur_client_size.y;

		tw = st_best_size.x;
		th = st_best_size.y;
	}

	if ( m_text_style & wxALIGN_CENTER_HORIZONTAL )
	{
		x = (w - tw)/2;
	}
	else if ( m_text_style & wxALIGN_RIGHT )
	{
		x = w - tw;
	}
	else
	{
		x = 0;
	}

	if ( m_text_style & wxALIGN_CENTER_VERTICAL )
	{
		y = (h - th)/2;
	}
	else if ( m_text_style & wxALIGN_BOTTOM )
	{
		y = h - th;
	}
	else
	{
		y = 0;
	}

	m_pST->SetSize(x, y, tw, th);

    event.Skip();
}
