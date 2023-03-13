                              //BitmapButton.cpp//                                
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
#include "BitmapButton.h"
#include <wx/dcclient.h>

BEGIN_EVENT_TABLE(CBitmapButton, wxWindow)
	EVT_PAINT(CBitmapButton::OnPaint)
	EVT_LEFT_DOWN(CBitmapButton::OnLButtonDown)
	EVT_LEFT_UP(CBitmapButton::OnLButtonUp)
	EVT_ENTER_WINDOW(CBitmapButton::OnMouseEnter)
	EVT_LEAVE_WINDOW(CBitmapButton::OnMouseLeave)
	EVT_MOUSE_CAPTURE_LOST(CBitmapButton::OnMouseCaptureLost)
END_EVENT_TABLE()

CBitmapButton::CBitmapButton(wxWindow* parent,
	wxWindowID id,
	const wxImage& image,
	const wxImage& image_focused,
	const wxImage& image_selected,
	const wxPoint& pos,
	const wxSize& size) : wxWindow(parent, id, pos, size, wxCLIP_CHILDREN | wxWANTS_CHARS)
{
	m_parent = parent;
	m_bDown = false;
	m_image = image;
	m_image_focused = image_focused;
	m_image_selected = image_selected;
	m_bImagesDefined = true;
}

CBitmapButton::CBitmapButton(wxWindow* parent,
	wxWindowID id,
	const wxImage& image,
	const wxImage& image_selected,
	const wxPoint& pos,
	const wxSize& size) : CBitmapButton(parent, id, image, image, image_selected, pos, size)
{
}

CBitmapButton::CBitmapButton(wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size) : wxWindow(parent, id, pos, size, wxCLIP_CHILDREN | wxWANTS_CHARS)
{
	m_parent = parent;
	m_bDown = false;
	m_bImagesDefined = false;
}

void CBitmapButton::SetBitmaps(const wxImage& image,
	const wxImage& image_focused,
	const wxImage& image_selected)
{
	m_image = image;
	m_image_focused = image_focused;
	m_image_selected = image_selected;
	m_bImagesDefined = true;
	this->Refresh(true);
}

void CBitmapButton::OnLButtonDown( wxMouseEvent& event )
{
	m_bDown = true;
	this->CaptureMouse();
	this->Refresh(true);
	
	wxCommandEvent bnev(wxEVT_BUTTON, this->GetId());
	wxPostEvent(m_parent, bnev);
}

void CBitmapButton::OnLButtonUp( wxMouseEvent& event )
{
	if (m_bDown == true) 
	{
		m_bDown = false;
		this->ReleaseMouse();		
		this->Refresh(true);
	}
}

void CBitmapButton::OnMouseEnter(wxMouseEvent& event)
{
	this->Refresh(true);
}

void CBitmapButton::OnMouseLeave(wxMouseEvent& event)
{
	this->Refresh(true);
}

void CBitmapButton::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if (m_bDown == true) 
	{
		m_bDown = false;
		this->Refresh(true);
	}
}

void CBitmapButton::OnPaint(wxPaintEvent& event)
{
	if (m_bImagesDefined)
	{
		wxPaintDC dc(this);
		int cw, ch;
		this->GetClientSize(&cw, &ch);

		if (m_bDown)
		{
			dc.DrawBitmap(wxBitmap(m_image_selected.Scale(cw, ch)), 0, 0);
		}
		else
		{
			wxPoint clao = GetClientAreaOrigin();
			wxPoint mp = wxGetMousePosition() - GetScreenPosition() - clao;
			int w, h;
			this->GetClientSize(&w, &h);

			if ((mp.x >= 0) &&
				(mp.x < w) &&
				(mp.y >= 0) &&
				(mp.y < h))
			{
				dc.DrawBitmap(wxBitmap(m_image_focused.Scale(cw, ch)), 0, 0);
			}
			else
			{
				dc.DrawBitmap(wxBitmap(m_image.Scale(cw, ch)), 0, 0);
			}
		}
	}
}