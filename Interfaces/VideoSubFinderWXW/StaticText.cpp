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
				wxWindowID id,
				const wxString& label,
				long text_style,
				long style,				   
				const wxPoint& pos,
				const wxSize& size,
				const wxString& name )				
		:wxPanel( parent, id, pos, size, style, name)
{
	m_pParent = parent;
	m_pFont = NULL;

	m_pST = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, text_style, wxStaticTextNameStr);

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

void CStaticText::RefreshData()
{
	wxSizeEvent event;
	OnSize(event);
}

void CStaticText::SetLabel(const wxString& label)
{
	m_pST->SetLabel(label);

	wxSizeEvent event;
	OnSize(event);
}

bool CStaticText::SetBackgroundColour(const wxColour& colour)
{
	return ( wxPanel::SetBackgroundColour(colour) && 
		     m_pST->SetBackgroundColour(colour) );
}

void CStaticText::SetTextColour(const wxColour& colour)
{
	m_pST->SetForegroundColour(colour);
}

void CStaticText::OnSize(wxSizeEvent& event)
{
	int w, h, tw, th, x, y;

	if (m_pFont) m_pST->SetFont(*m_pFont);

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
