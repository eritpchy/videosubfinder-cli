                              //TextBox.cpp//                                
//////////////////////////////////////////////////////////////////////////////////
//							  Version 1.76              						//
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

#include "TextBox.h"

BEGIN_EVENT_TABLE(CTextBox, wxPanel)
	EVT_SIZE(CTextBox::OnSize)
END_EVENT_TABLE()

CTextBox::CTextBox(  wxWindow* parent,
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

	m_pST = new wxStaticText( this, wxID_ANY, label);
	m_text_style = text_style;
}

CTextBox::~CTextBox()
{
}

bool CTextBox::SetFont(const wxFont& font)
{
	return m_pST->SetFont(font);

	wxSizeEvent event;
	OnSize(event);
}

void CTextBox::SetLabel(const wxString& label)
{
	m_pST->SetLabel(label);

	wxSizeEvent event;
	OnSize(event);
}

bool CTextBox::SetBackgroundColour(const wxColour& colour)
{
	return ( wxPanel::SetBackgroundColour(colour) && 
		     m_pST->SetBackgroundColour(colour) );
}

void CTextBox::SetTextColour(const wxColour& colour)
{
	m_pST->SetForegroundColour(colour);
}

void CTextBox::OnSize(wxSizeEvent& event)
{
	int w, h, tw, th, x, y;

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
