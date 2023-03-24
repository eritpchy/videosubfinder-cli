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
	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::SetTextColour(wxColour& colour)
{
	m_pTextColour = &colour;
	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::RefreshData()
{
	m_pST->SetLabel(*m_p_label);
	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::SetLabel(const wxString& label)
{
	m_p_label = &label;
	m_pST->SetLabel(*m_p_label);
	wxSizeEvent event;
	OnSize(event);
}

bool CStaticText::SetBackgroundColour(const wxColour& colour)
{
	return ( wxPanel::SetBackgroundColour(colour) && 
		     m_pST->SetBackgroundColour(colour) );
}

void CStaticText::OnSize(wxSizeEvent& event)
{
	int w, h, tw, th, x, y;
	
	if (m_pFont) m_pST->SetFont(*m_pFont);
	if (m_pTextColour) m_pST->SetForegroundColour(*m_pTextColour);

    this->GetClientSize(&w, &h);
	m_pST->GetSize(&tw, &th);

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
