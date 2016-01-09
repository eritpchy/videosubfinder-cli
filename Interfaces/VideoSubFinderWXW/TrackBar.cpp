                              //TrackBar.cpp//                                
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

#include "stdafx.h"
#include "VideoSubFinder.h"
#include "TrackBar.h"
#include ".\trackbar.h"

CTBLine::CTBLine()
{
	m_pParent = NULL;
}

CTBLine::~CTBLine()
{
}

BEGIN_MESSAGE_MAP(CTBLine, CWnd)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CTBLine::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pParent!=NULL) 
	{
		m_pParent->m_pMF->PauseVideo();

		m_pParent->MoveBox();

		m_pParent->m_bDown = true;

		m_pParent->m_Box.SetCapture();
	}

	CWnd::OnLButtonDown(nFlags, point);
}

CTBBox::CTBBox()
{
	m_pParent = NULL;
}

CTBBox::~CTBBox()
{
}

BEGIN_MESSAGE_MAP(CTBBox, CWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

void CTBBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pParent!=NULL) 
	{
		m_pParent->m_pMF->PauseVideo();

		m_pParent->MoveBox();

		m_pParent->m_bDown = true;

		SetCapture();
	}

	CWnd::OnLButtonDown(nFlags, point);
}

CTrackBar::CTrackBar()
{
	m_WasInited = false;
	m_bDown = false;
	m_pMF = NULL;
}

CTrackBar::~CTrackBar()
{
}

BEGIN_MESSAGE_MAP(CTrackBar, CWnd)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void CTrackBar::Init(CWnd* pParent)
{
	CString strTBClass;
	CString strLNClass;
	CString strBXClass;

	m_CL1Brush.CreateSolidBrush(RGB(255, 255, 255));
	m_CL2Brush.CreateSolidBrush(RGB(125, 125, 125));

	strTBClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
									LoadCursor(NULL, IDC_ARROW),
									(HBRUSH) (COLOR_WINDOW),
									NULL);

	strLNClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
									LoadCursor(NULL, IDC_ARROW),
									m_CL1Brush,
									NULL);

	strBXClass = AfxRegisterWndClass(CS_VREDRAW | CS_HREDRAW,
									LoadCursor(NULL, IDC_ARROW),
									m_CL2Brush,
									NULL);

	Create( strTBClass, 
			"", 
			WS_CHILD | WS_VISIBLE,
			CRect(0,0,200,50),
			pParent,
			ID_TRACKBAR );
	
	m_Line.Create(strLNClass, 
			"", 
			WS_CLIPSIBLINGS | WS_VISIBLE | WS_BORDER,
			CRect(0,0,200,10),
			this,
			ID_LINE );
	m_Line.m_pParent = this;

	m_Box.Create(strBXClass, 
			"", 
			WS_CLIPSIBLINGS | WS_VISIBLE | WS_BORDER,
			CRect(0,0,10,20),
			this,
			ID_BOX );
	m_Box.m_pParent = this;

	m_Pos = 0.2;
	m_lh = 8;
	m_bw = 9; 
	m_bh = 16;

	ResizeControls();

	m_Box.BringWindowToTop();

	m_WasInited = true;
}

void CTrackBar::ResizeControls()
{
	CRect rcClTB, rcLN, rcBX;

	this->GetClientRect(rcClTB);
	
	rcLN.left = 0;
	rcLN.right = rcClTB.right;
	rcLN.top = (rcClTB.Height() - m_lh)/2;
	rcLN.bottom = rcLN.top + m_lh;

	rcBX.left = (LONG)((double)(rcClTB.right - m_bw)*m_Pos);
	rcBX.right = rcBX.left + m_bw;
	rcBX.top = (rcClTB.Height() - m_bh)/2;
	rcBX.bottom = rcBX.top + m_bh;

	m_Line.MoveWindow(&rcLN);
	m_Box.MoveWindow(&rcBX);
}

void CTrackBar::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (m_WasInited) ResizeControls();
}

void CTrackBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_pMF->PauseVideo();

	MoveBox();

	m_bDown = true;

	m_Box.SetCapture();

	CWnd::OnLButtonDown(nFlags, point);
}

void CTrackBar::MoveBox()
{
	POINT pt;
	CRect rcClTB, rcBX;

	this->GetClientRect(rcClTB);
	rcBX.top = (rcClTB.Height() - m_bh)/2;
	rcBX.bottom = rcBX.top + m_bh;
	
	GetCursorPos(&pt); 
	this->ScreenToClient(&pt);

	if (pt.x > m_bw/2)
	{
		if (pt.x < (rcClTB.right - m_bw/2))
		{
			rcBX.left = (pt.x - m_bw/2);
			rcBX.right = rcBX.left + m_bw;
			m_Box.MoveWindow(&rcBX);

			m_Pos = (double)rcBX.left/(double)(rcClTB.right - m_bw);
		}
		else
		{
			rcBX.right = rcClTB.right;
			rcBX.left = rcBX.right - m_bw;
			m_Box.MoveWindow(&rcBX);

			m_Pos = 1.0;
		}
	}
	else
	{
		rcBX.left = 0;
		rcBX.right = m_bw;
		m_Box.MoveWindow(&rcBX);

		m_Pos = 0.0;
	}
	
	if (m_pMF!=NULL)
	if (m_pMF->m_VIsOpen)
	{
		s64 Pos, endPos;
		s64 Cur;

		endPos = m_pMF->m_pVideo->m_Duration;
		Pos = ((s64)(rcBX.left)*endPos)/(s64)(rcClTB.right - m_bw);
		Cur = m_pMF->m_pVideo->GetPos();

		if (Pos != Cur)
		{
			m_pMF->m_pVideo->SetPosFast(Pos);
		}
	}
}

void CTrackBar::MoveBox(s64 Pos)
{
	CRect rcClTB, rcLN, rcBX;

	this->GetClientRect(rcClTB);

	//всвязи с неправильным округлением добавляем 1
	rcBX.left = (LONG)(1+(Pos*(s64)(rcClTB.right - m_bw))/(m_pMF->m_pVideo->m_Duration));
	if (Pos == 0) rcBX.left = 0;
	if (Pos ==  m_pMF->m_pVideo->m_Duration) rcBX.left = rcClTB.right-m_bw;
	rcBX.right = rcBX.left + m_bw;
	rcBX.top = (rcClTB.Height() - m_bh)/2;
	rcBX.bottom = rcBX.top + m_bh;

	m_Box.MoveWindow(&rcBX);

	m_Pos = (double)rcBX.left/(double)(rcClTB.right - m_bw);
}

void CTBBox::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pParent->m_bDown == true) 
	{
		m_pParent->MoveBox();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CTBBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_pParent->m_bDown == true) 
	{
		m_pParent->m_bDown = false;
		ReleaseCapture();
	}

	CWnd::OnLButtonUp(nFlags, point);
}
