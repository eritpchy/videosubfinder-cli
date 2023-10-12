
//ResizableWindow.cpp//                                
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

#include "ResizableWindow.h"

BEGIN_EVENT_TABLE(CResizableWindow, wxWindow)
EVT_LEFT_DOWN(CResizableWindow::OnLButtonDown)
EVT_LEFT_UP(CResizableWindow::OnLButtonUp)
EVT_MOTION(CResizableWindow::OnMouseMove)
EVT_LEAVE_WINDOW(CResizableWindow::OnMouseLeave)
EVT_MOUSE_CAPTURE_LOST(CResizableWindow::OnMouseCaptureLost)
END_EVENT_TABLE()


CResizableWindow::CResizableWindow(wxWindow* parent, wxWindowID id,
	const wxPoint& pos,
	const wxSize& size)
	: wxWindow(parent, id, pos, size, wxWANTS_CHARS)
{
}

CResizableWindow::~CResizableWindow()
{
}

void CResizableWindow::OnLButtonDown(wxMouseEvent& event)
{
	wxPoint clao = GetClientAreaOrigin();
	wxPoint mp = wxGetMousePosition() - GetScreenPosition() - clao;
	event.m_x = mp.x;
	event.m_y = mp.y;

	if (!this->HasFocus())
	{
		this->SetFocus();
	}

	int x = event.m_x, y = event.m_y, w, h;
	this->GetClientSize(&w, &h);

	if ((x < m_border_size) ||
		(x > w - m_border_size - 1) ||
		(y < m_border_size) ||
		(y > h - m_border_size - 1))
	{
		m_bDownResize = true;

		UpdateCursor(event.m_x, event.m_y);
		
		if (x < m_border_size)
			m_bDownFromLeft = true;
		else
			m_bDownFromLeft = false;

		if (x > w - m_border_size - 1)
			m_bDownFromRight = true;
		else
			m_bDownFromRight = false;

		if (y < m_border_size)
			m_bDownFromTop = true;
		else
			m_bDownFromTop = false;

		if (y > h - m_border_size - 1)
			m_bDownFromBottom = true;
		else
			m_bDownFromBottom = false;

		this->Raise();
		this->CaptureMouse();		
	}
	else if (y < m_move_border_size)
	{
		m_bDownMove = true;
		m_dm_orig_point = wxPoint(event.m_x, event.m_y);
		//UpdateCursor(event.m_x, event.m_y);
		m_cur_cursor = wxCURSOR_HAND;
		this->SetCursor(wxCursor(m_cur_cursor));
		this->Raise();
		this->CaptureMouse();
	}
}

void CResizableWindow::OnLButtonUp(wxMouseEvent& event)
{
	if ((m_bDownResize == true) || (m_bDownMove == true))
	{
		m_bDownResize = false;
		m_bDownMove = false;
		m_cur_cursor = wxCURSOR_ARROW;
		this->SetCursor(wxCursor(m_cur_cursor));
		this->ReleaseMouse();		
		this->Refresh(true);
	}
}

void CResizableWindow::UpdateCursor(int x, int y)
{
	wxStockCursor set_cursor = wxCURSOR_ARROW;

	int w, h;
	this->GetClientSize(&w, &h);

	if ((x < m_border_size) ||
		(x > w - m_border_size - 1) ||
		(y < m_border_size) ||
		(y > h - m_border_size - 1))
	{
		if (((x < m_border_size) && (y < m_border_size)) ||
			((x > w - m_border_size - 1) && (y > h - m_border_size - 1)))
		{
			set_cursor = wxCURSOR_SIZENWSE;
		}
		else if (((x < m_border_size) && (y > h - m_border_size - 1)) ||
			((x > w - m_border_size - 1) && (y < m_border_size)))
		{
			set_cursor = wxCURSOR_SIZENESW;
		}
		else if ((x < m_border_size) ||
			(x > w - m_border_size - 1))
		{
			set_cursor = wxCURSOR_SIZEWE;
		}
		else
		{
			set_cursor = wxCURSOR_SIZENS;
		}
	}

	if (m_cur_cursor != set_cursor)
	{
		m_cur_cursor = set_cursor;
		this->SetCursor(wxCursor(m_cur_cursor));
	}
}

void CResizableWindow::OnMouseLeave(wxMouseEvent& event)
{
	if ((m_bDownResize == false) && (m_bDownMove == false))
	{
		m_cur_cursor = wxCURSOR_ARROW;
		this->SetCursor(wxCursor(m_cur_cursor));
	}
}

void CResizableWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	if ((m_bDownResize == true) || (m_bDownMove == true))
	{
		m_bDownResize = false;
		m_bDownMove = false;
		
		m_cur_cursor = wxCURSOR_ARROW;
		this->SetCursor(wxCursor(m_cur_cursor));
	}
}

void CResizableWindow::OnMouseMove(wxMouseEvent& event)
{
	wxPoint clao = GetClientAreaOrigin();
	wxPoint mp = wxGetMousePosition() - GetScreenPosition() - clao;
	event.m_x = mp.x;
	event.m_y = mp.y;

	wxPoint pt = this->GetPosition();
	wxSize border = this->GetWindowBorderSize();
	int x = pt.x + border.GetWidth() + event.m_x, y = pt.y + border.GetHeight() + event.m_y, w, h, val;
	wxSize ps = this->GetParent()->GetClientSize();

	wxRect rc = this->GetRect();
	wxRect orig_rc = rc;
	w = rc.width;
	h = rc.height;

	if (m_bDownResize == true)
	{
		if (m_bDownFromLeft)
		{
			val = rc.x + w - x;
			if ((val > 2 * m_border_size + 10) && (x >= 0))
			{
				rc.width = val;
				rc.x = x;
			}
		}

		if (m_bDownFromRight)
		{
			val = x - rc.x;
			if ((val > 2 * m_border_size + 10) && (x < ps.x))
			{
				rc.width = val;
			}
		}

		if (m_bDownFromTop)
		{
			val = rc.y + h - y;
			if ((val > 2 * m_border_size + 10) && (y >= 0))
			{
				rc.height = val;
				rc.y = y;
			}
		}

		if (m_bDownFromBottom)
		{
			val = y - rc.y;
			if ((val > 2 * m_border_size + 10) && (y < ps.y))
			{
				rc.height = val;
			}
		}

		if (rc != orig_rc)
		{
			//this->Show(false);
			this->SetSize(rc);
			//this->Show(true);
			//this->Raise();
			//this->Refresh(true);
		}
	}
	else if (m_bDownMove == true)
	{
		int dx = event.m_x - m_dm_orig_point.x;
		int dy = event.m_y - m_dm_orig_point.y;

		if ((rc.x + w + dx > m_move_border_size) && (rc.x + dx < ps.x - m_move_border_size))
			rc.x += dx;
		
		if ((rc.y + dy >= 0) && (rc.y + dy < ps.y - m_move_border_size))
			rc.y += dy;

		if (rc != orig_rc)
		{
			//this->Show(false);
			this->SetSize(rc);
			//this->Show(true);
			//this->Raise();
			//this->Refresh(true);
		}
	}
	else
	{
		UpdateCursor(event.m_x, event.m_y);
	}
}