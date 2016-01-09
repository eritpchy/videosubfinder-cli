                              //SeparatingLine.cpp//                                
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

#include "SeparatingLine.h"

BEGIN_EVENT_TABLE(CSeparatingLine, wxWindow)
	EVT_PAINT(CSeparatingLine::OnPaint)
	EVT_ERASE_BACKGROUND(CSeparatingLine::OnEraseBackground)
	EVT_LEFT_DOWN(CSeparatingLine::OnLButtonDown)
	EVT_LEFT_UP(CSeparatingLine::OnLButtonUp)
	EVT_MOTION(CSeparatingLine::OnMouseMove)
END_EVENT_TABLE()

CSeparatingLine::CSeparatingLine(wxWindow *parent, int w, int h, int sw, int sh, int minpos, int maxpos, int offset, int orientation, wxWindowID id)
		: wxWindow( parent, id, wxDefaultPosition, wxDefaultSize, 
							0
							| wxTRANSPARENT_WINDOW )
{
	m_bDown = false;
	m_pParent = parent;

	m_w = w;
	m_h = h;

	m_sw = sw;
	m_sh = sh;

	m_min = minpos;
	m_max = maxpos;

	m_offset = offset;

	m_orientation = orientation;

	m_pos = 0;
	m_pos_min = 0;
	m_pos_max = 1;

	wxRect rc;

	if (m_orientation == 0)
	{
		this->SetCursor( wxCursor( wxCURSOR_SIZENS ) );

		rc.x = m_offset-m_sw;		
		rc.y = m_min-m_h/2-m_sh;
		rc.width = m_w+2*m_sw;
		rc.height = m_h+2*m_sh;
	}	
	else
	{
		this->SetCursor( wxCursor( wxCURSOR_SIZEWE ) );

		rc.x = m_min-m_w/2-m_sw;
		rc.y = m_offset-m_sh;
		rc.width = m_w+2*m_sw;		
		rc.height = m_h+2*m_sh;
	}

	CreateNewRgn();

	this->Raise();
	
	UpdateSL();
}

CSeparatingLine::~CSeparatingLine()
{
}

void CSeparatingLine::CreateNewRgn()
{
	wxPoint ps[20];
	int i = 0;

	if (m_orientation == 0)
	{
		ps[i].x = 0;
		ps[i].y = 0;
		i++;

		ps[i].x = m_sw;
		ps[i].y = m_sh;
		i++;

		ps[i].x = m_w+m_sw;
		ps[i].y = m_sh;
		i++;

		ps[i].x = m_w+2*m_sw;
		ps[i].y = 0;
		i++;

		ps[i].x = m_w+m_sw+1;
		ps[i].y = m_sh+(m_h+1)/2+1;
		i++;

		ps[i].x = m_w+m_sw+1;
		ps[i].y = m_sh+(m_h+1)/2-1;
		i++;

		ps[i].x = m_w+2*m_sw;
		ps[i].y = m_sh+m_h+m_sh;
		i++;

		ps[i].x = m_w+m_sw;
		ps[i].y = m_sh+m_h;
		i++;

		ps[i].x = m_sw;
		ps[i].y = m_sh+m_h;
		i++;

		ps[i].x = 0;
		ps[i].y = m_sh+m_h+m_sh;
		i++;

		ps[i].x = m_sw-1;
		ps[i].y = m_sh+(m_h+1)/2-1;
		i++;

		ps[i].x = m_sw-1;
		ps[i].y = m_sh+(m_h+1)/2+1;
		i++;
	}
	else
	{
		ps[i].x = 0;
		ps[i].y = 0;
		i++;

		ps[i].x = m_sw+1+(m_w+1)/2;
		ps[i].y = m_sh-1;
		i++;

		ps[i].x = m_sw-1+(m_w+1)/2;
		ps[i].y = m_sh-1;
		i++;

		ps[i].x = m_w+2*m_sw;
		ps[i].y = 0;
		i++;

		ps[i].x = m_w+m_sw;
		ps[i].y = m_sh;
		i++;

		ps[i].x = m_w+m_sw;
		ps[i].y = m_sh+m_h;
		i++;

		ps[i].x = m_w+2*m_sw;
		ps[i].y = m_h+2*m_sh;
		i++;

		ps[i].x = m_sw-1+(m_w+1)/2;
		ps[i].y = m_h+m_sh+1;
		i++;

		ps[i].x = m_sw+1+(m_w+1)/2;
		ps[i].y = m_h+m_sh+1;
		i++;

		ps[i].x = 0;
		ps[i].y = m_h+2*m_sh;
		i++;

		ps[i].x = m_sw;
		ps[i].y = m_h+m_sh;
		i++;

		ps[i].x = m_sw;
		ps[i].y = m_sh;
		i++;
	}

	m_rgn = wxRegion( (size_t)i, ps, wxWINDING_RULE );

	//this->SetShape(m_rgn);

	m_old_w = m_w;
	m_old_h = m_w;
}

void CSeparatingLine::OnLButtonDown( wxMouseEvent& event )
{
	m_bDown = true;

	this->CaptureMouse();
}

void CSeparatingLine::OnLButtonUp( wxMouseEvent& event )
{
	if (m_bDown == true) 
	{
		m_bDown = false;
		this->ReleaseMouse();		
		
		this->Refresh(true);
	}
}

void CSeparatingLine::OnMouseMove( wxMouseEvent& event )
{
	if (m_bDown == true) 
	{
		wxPoint pt = this->GetPosition();
		wxSize border = this->GetWindowBorderSize();

		MoveSL( wxPoint(pt.x + border.GetWidth() + event.m_x, pt.y + border.GetHeight() + event.m_y) );
	}
}

void CSeparatingLine::MoveSL(wxPoint pt)
{
	int val;

	if (m_orientation == 0)
		val = pt.y;
	else 
		val = pt.x;

	if (val > m_max)
	{
		m_pos = 1;
	}
	else
	{
		if (val < m_min)
		{
			m_pos = 0;
		}
		else
		{
			m_pos = (double)(val-m_min)/(double)(m_max-m_min);
		}
	}

	if (m_pos < m_pos_min) m_pos = m_pos_min;
	if (m_pos > m_pos_max) m_pos = m_pos_max;

	UpdateSL();
}

double CSeparatingLine::CalculateCurPos()
{
	wxRect rc;
	int val;
	double res;

	rc = this->GetRect();

	if (m_orientation == 0)
	{
		val = rc.y+m_h/2+m_sh;
	}
	else
	{
		val = rc.x+m_w/2+m_sw;
	}

	res = (double)(val-m_min)/(double)(m_max-m_min);

	return res;
}

int CSeparatingLine::GetCurPos()
{
	wxRect rc;
	int res;

	rc = this->GetRect();

	if (m_orientation == 0)
	{
		res = rc.y+m_h/2+m_sh;
	}
	else
	{
		res = rc.x+m_w/2+m_sw;
	}

	return res;
}

void CSeparatingLine::UpdateSL()
{
	wxRect rc;
	int pos;

	pos = m_min+(int)(m_pos*(double)(m_max-m_min));

	if ( (m_w != m_old_w) || (m_h != m_old_h) )
	{
		CreateNewRgn();
	}

	if (m_orientation == 0)
	{
		rc.x = m_offset-m_sw;		
		rc.y = pos-m_h/2-m_sh;
		rc.width = m_w+2*m_sw;
		rc.height = m_h+2*m_sh;
	}	
	else
	{
		rc.x = pos-m_w/2-m_sw;
		rc.y = m_offset-m_sh;
		rc.width = m_w+2*m_sw;		
		rc.height = m_h+2*m_sh;
	}

	this->Show(false);

	this->SetSize(rc);

	this->Show(true);

	this->Raise();

	//this->Refresh(true);
	//this->Update();
}

void CSeparatingLine::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);
	int w, h;

	this->GetClientSize(&w, &h);

	wxBrush blackBrush( wxColour(0, 0, 0) );
	wxBrush whiteBrush( wxColour(255, 255, 255) );

	dc.SetBackgroundMode(wxTRANSPARENT);
	dc.DestroyClippingRegion();
	dc.SetClippingRegion(m_rgn);

	dc.SetBrush(blackBrush);
	dc.DrawRectangle(0, 0, w, h);

	dc.SetBrush(whiteBrush);
	if (m_orientation == 0)
	{
		dc.DrawRectangle(m_sw, m_sh, m_w, m_h);
	}
	else
	{
		dc.DrawRectangle(m_sw, m_sh, m_w, m_h);
	}

	//this->Update();
}

void CSeparatingLine::OnEraseBackground(wxEraseEvent &event)
{
	wxDC *pdc = event.GetDC();

	int w, h;

	this->GetClientSize(&w, &h);

	wxBrush blackBrush( wxColour(0, 0, 0) );
	wxBrush whiteBrush( wxColour(255, 255, 255) );

	pdc->SetBackgroundMode(wxTRANSPARENT);
	pdc->DestroyClippingRegion();
	pdc->SetClippingRegion(m_rgn);

	pdc->SetBrush(blackBrush);
	pdc->DrawRectangle(0, 0, w, h);

	pdc->SetBrush(whiteBrush);
	if (m_orientation == 0)
	{
		pdc->DrawRectangle(m_sw, m_sh, m_w, m_h);
	}
	else
	{
		pdc->DrawRectangle(m_sw, m_sh, m_w, m_h);
	}

	//this->Update();
}