                              //ScrollBar.cpp//                                
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

#include "ScrollBar.h"

BEGIN_EVENT_TABLE(CScrollBar, wxPanel)
	EVT_PAINT(CScrollBar::OnPaint)
	EVT_LEFT_DOWN(CScrollBar::OnLButtonDown)
	EVT_LEFT_UP(CScrollBar::OnLButtonUp)
	EVT_MOTION(CScrollBar::OnMouseMove)
END_EVENT_TABLE()

CScrollBar::CScrollBar( wxWindow *parent, 
						wxWindowID id,
						const wxPoint& pos,
						const wxSize& size)
		:wxPanel( parent, id, pos, size, 
				  wxTAB_TRAVERSAL | wxBORDER, 
				  "panel" )						
{
	m_pParent = parent;
	
	m_SBLA = wxImage("bitmaps/sb_la.bmp");
	m_SBRA = wxImage("bitmaps/sb_ra.bmp");
	m_SBLC = wxImage("bitmaps/sb_lc.bmp");
	m_SBRC = wxImage("bitmaps/sb_rc.bmp");
	m_SBT = wxImage("bitmaps/sb_t.bmp");

	m_rcSBT.x = -1;
	m_rcSBT.y = -1;

	m_rcSBLA.height = -1;

	m_min_pos = 0;
	m_max_pos = 100;
	m_pos = m_min_pos;
	m_range = m_max_pos - m_min_pos;

	this->SetCursor( wxCursor( wxCURSOR_HAND ) );

	m_ld = false;
}

CScrollBar::~CScrollBar()
{
}

void CScrollBar::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	int w, h, aw, ah, new_aw, new_ah, tw, th, new_tw, new_th, tx, prev_tx = m_rcSBT.x, dr;
	wxBitmap newSBLA, newSBRA, newSBT, newSBLC, newSBRC;
	bool isRes = false;
	this->GetClientSize(&w, &h);

	//wxPaintDC dc( this );
	wxBitmap bmp(w, h);
	wxMemoryDC dc(bmp);	

	aw = m_SBLA.GetWidth();
	ah = m_SBLA.GetHeight();

	new_ah = h;
	new_aw = (int)((double)(aw*new_ah)/(double)ah);

	tw = m_SBT.GetWidth();
	th = m_SBT.GetHeight();

	new_th = h;
	new_tw = (int)((double)(tw*new_th)/(double)th);	

	dr = w - 2*new_aw - new_tw;
	tx = new_aw + (int)((double)((m_pos-m_min_pos)*dr)/(double)m_range);

	m_rcSBLA.x = 0;
	m_rcSBLA.y = 0;
	m_rcSBLA.width = new_aw;
	m_rcSBLA.height = new_ah;

	m_rcSBRA.x = w-new_aw;
	m_rcSBRA.y = 0;
	m_rcSBRA.width = new_aw;
	m_rcSBRA.height = new_ah;

	m_rcSBLC.x = new_aw;
	m_rcSBLC.y = 0;
	m_rcSBLC.width = tx-new_aw;
	m_rcSBLC.height = h;

	m_rcSBT.x = tx;
	m_rcSBT.y = 0;
	m_rcSBT.width = new_tw;
	m_rcSBT.height = new_th;

	m_rcSBRC.x = tx+new_tw;
	m_rcSBRC.y = 0;
	m_rcSBRC.width = w-(tx+new_tw)-new_aw;
	m_rcSBRC.height = h;

	newSBLA = wxBitmap(m_SBLA.Scale(m_rcSBLA.width, m_rcSBLA.height, wxIMAGE_QUALITY_HIGH));	
	if (m_rcSBLC.width > 0)
	{
		newSBLC = wxBitmap(m_SBLC.Scale(m_rcSBLC.width, m_rcSBLC.height, wxIMAGE_QUALITY_HIGH));
	}
	newSBT = wxBitmap(m_SBT.Scale(m_rcSBT.width, m_rcSBT.height, wxIMAGE_QUALITY_HIGH));
	if (m_rcSBRC.width > 0) 
	{
		newSBRC = wxBitmap(m_SBRC.Scale(m_rcSBRC.width, m_rcSBRC.height, wxIMAGE_QUALITY_HIGH));
	}
	newSBRA = wxBitmap(m_SBRA.Scale(m_rcSBRA.width, m_rcSBRA.height, wxIMAGE_QUALITY_HIGH));

	dc.DrawBitmap(newSBLA, m_rcSBLA.x, m_rcSBLA.y);
	if (m_rcSBLC.width > 0)
	{
		dc.DrawBitmap(newSBLC, m_rcSBLC.x, m_rcSBLC.y);
	}
	dc.DrawBitmap(newSBT, m_rcSBT.x, m_rcSBT.y);
	if (m_rcSBRC.width > 0) 
	{
		dc.DrawBitmap(newSBRC, m_rcSBRC.x, m_rcSBRC.y);
	}
	dc.DrawBitmap(newSBRA, m_rcSBRA.x, m_rcSBRA.y);

	wxBufferedPaintDC bpdc(this, bmp);
}

void CScrollBar::SetScrollRange(int min_pos, int max_pos)
{
	double dpos = (double)(m_pos-m_min_pos)/(double)(m_range);

	m_min_pos = min_pos;
	m_max_pos = max_pos;
	m_range = m_max_pos - m_min_pos;	

	m_pos = m_min_pos + (int)((double)m_range*(double)dpos);

	this->Refresh(false);
}

void CScrollBar::SetScrollPos(int pos)
{
	if (pos <= m_min_pos)
	{
		m_pos = m_min_pos;
	}
	else if (pos <= m_max_pos)
	{
		m_pos = pos;
	}
	else
	{
		m_pos = m_max_pos;
	}

	this->Refresh(false);
}

void CScrollBar::OnLButtonDown( wxMouseEvent& event )
{
	int x = event.m_x, y = event.m_y, w, h;
	this->GetClientSize(&w, &h);

	if ( m_rcSBT.Contains(x, y) || m_rcSBLC.Contains(x, y) || m_rcSBRC.Contains(x, y) )
	{
		m_ld = true;
		this->CaptureMouse();

		m_pos = m_min_pos + ((double)((x - m_rcSBT.width/2 - m_rcSBLA.width)*m_range)/(double)(w - 2*m_rcSBLA.width - m_rcSBT.width));
		if (m_pos < m_min_pos) m_pos = m_min_pos;
		if (m_pos > m_max_pos) m_pos = m_max_pos;
		
		this->Refresh(false);

		wxScrollEvent scroll_event(wxEVT_SCROLL_THUMBTRACK, 0, m_pos, 0);
		wxEvtHandler *handler = m_pParent->GetEventHandler(); 
		handler->ProcessEvent(scroll_event);
	}
	else if ( m_rcSBLA.Contains(x, y) )
	{
		this->ScrollLeft();
	}
	else if ( m_rcSBRA.Contains(x, y) )
	{
		this->ScrollRight();
	}
}

void CScrollBar::OnLButtonUp( wxMouseEvent& event )
{
	if ( m_ld )	
	{
		m_ld = false;
		this->ReleaseMouse();		
	}
}

void CScrollBar::OnMouseMove( wxMouseEvent& event )
{ 
	if ( m_ld )	
	{
		int x = event.m_x, y = event.m_y, w, h;
		this->GetClientSize(&w, &h);

		m_pos = m_min_pos + ((double)((x - m_rcSBT.width/2 - m_rcSBLA.width)*m_range)/(double)(w - 2*m_rcSBLA.width - m_rcSBT.width));
		if (m_pos < m_min_pos) m_pos = m_min_pos;
		if (m_pos > m_max_pos) m_pos = m_max_pos;
		
		this->Refresh(false);

		wxScrollEvent scroll_event(wxEVT_SCROLL_THUMBTRACK, 0, m_pos, 0);
		wxEvtHandler *handler = m_pParent->GetEventHandler(); 
		handler->ProcessEvent(scroll_event);
	}
}

void CScrollBar::ScrollLeft()
{
	if (m_pos > m_min_pos)
	{
		m_pos--;
		this->Refresh(false);

		wxScrollEvent scroll_event(wxEVT_SCROLL_THUMBTRACK, 0, -2, 0);
		wxEvtHandler *handler = m_pParent->GetEventHandler(); 
		handler->ProcessEvent(scroll_event);
	}
}

void CScrollBar::ScrollRight()
{
	if (m_pos < m_max_pos)
	{
		m_pos++;
		this->Refresh(false);

		wxScrollEvent scroll_event(wxEVT_SCROLL_THUMBTRACK, 0, -1, 0);
		wxEvtHandler *handler = m_pParent->GetEventHandler(); 
		handler->ProcessEvent(scroll_event);
	}
}
