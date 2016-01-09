                              //VideoBox.cpp//                                
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

#include "VideoBox.h"

BEGIN_EVENT_TABLE(CVideoWnd, wxWindow)
	EVT_PAINT(CVideoWnd::OnPaint)
	EVT_SET_FOCUS(CVideoWnd::OnSetFocus)
END_EVENT_TABLE()

CVideoWnd::CVideoWnd(CVideoWindow *pVW)
		:wxWindow( pVW, wxID_ANY )
{
	this->SetBackgroundColour(wxColor(125, 125, 125));

	m_pVW = pVW;
	m_pVB = pVW->m_pVB;
}

CVideoWnd::~CVideoWnd()
{
}

void CVideoWnd::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);	

	if ( (m_pVB != NULL) && (m_pVB->m_pBmp != NULL) )
	{
		int w, h;

		this->GetClientSize(&w, &h);

		if ( (w != m_pVB->m_wScaled) || (h != m_pVB->m_hScaled) )
		{
			*m_pVB->m_pBmpScaled = wxBitmap(m_pVB->m_pBmp->ConvertToImage().Scale(w, h));
			m_pVB->m_wScaled = w;
			m_pVB->m_hScaled = h;
		}

		dc.DrawBitmap(*m_pVB->m_pBmpScaled, 0, 0);
	}
}

BEGIN_EVENT_TABLE(CVideoWindow, wxPanel)
	EVT_PAINT(CVideoWindow::OnPaint)
	EVT_SIZE(CVideoWindow::OnSize)
END_EVENT_TABLE()

CVideoWindow::CVideoWindow(CVideoBox *pVB)
		:wxPanel( pVB, wxID_ANY )
{
	m_pVB = pVB;
	m_WasInited = false;
}

CVideoWindow::~CVideoWindow()
{
}

void CVideoWindow::Init()
{
	m_pVideoWnd = new CVideoWnd(this);

	m_pHSL1 = new CSeparatingLine(this, 200, 3, 7, 3, 100, 110, 50, 0);
	m_pHSL1->m_pos = 0;
	m_pHSL2 = new CSeparatingLine(this, 200, 3, 7, 3, 140, 150, 50, 0);
	m_pHSL2->m_pos = 1;
	m_pVSL1 = new CSeparatingLine(this, 3, 100, 3, 7, 100, 110, 50, 1);
	m_pVSL1->m_pos = 0;
	m_pVSL2 = new CSeparatingLine(this, 3, 100, 3, 7, 140, 150, 50, 1);
	m_pVSL2->m_pos = 1;

	this->SetSize(50,50,200,200);
}

void CVideoWindow::Update()
{
	m_pHSL1->m_pos_max = m_pHSL2->m_pos-0.05;
	m_pHSL2->m_pos_min = m_pHSL1->m_pos+0.05;
	m_pVSL1->m_pos_max = m_pVSL2->m_pos-0.05;
	m_pVSL2->m_pos_min = m_pVSL1->m_pos+0.05;

	if (m_pVB->m_pMF->m_VIsOpen)
	{
		if (m_pVB->m_pMF->m_vs != CMainFrame::Play)
		{
			s64 Cur;
			Cur = m_pVB->m_pMF->m_pVideo->GetPos();
			m_pVB->m_pMF->m_pVideo->SetPos(Cur);
		}
	}
}

void CVideoWindow::OnPaint( wxPaintEvent &event )
{
	wxPaintDC dc(this);
}

void CVideoWindow::OnSize(wxSizeEvent& event)
{
	int w, h, vwnd_w, vwnd_h;
	wxRect rcCL, rcVWND;

	this->GetClientSize(&w, &h);
	
	rcVWND.x = 9;
	rcVWND.y = 9;
	rcVWND.width = w - rcVWND.x*2;
	rcVWND.height = h - rcVWND.y*2;

	m_pVideoWnd->SetSize(rcVWND);

	if (m_pVB->m_pMF->m_VIsOpen) 
	{
		m_pVideoWnd->GetClientSize(&vwnd_w, &vwnd_h);
		m_pVB->m_pMF->m_pVideo->SetVideoWindowPosition(0, 0, vwnd_w, vwnd_h);
	}

	m_pHSL1->m_offset = rcVWND.x - 2;
	m_pHSL1->m_w = rcVWND.width + 4;
	m_pHSL1->m_min = rcVWND.y;
	m_pHSL1->m_max = m_pHSL1->m_min + rcVWND.height;
	//m_pHSL1->m_pos = 0;

	m_pHSL2->m_offset = m_pHSL1->m_offset;
	m_pHSL2->m_w = m_pHSL1->m_w;
	m_pHSL2->m_min = rcVWND.y;
	m_pHSL2->m_max = m_pHSL2->m_min + rcVWND.height;
	//m_pHSL2->m_pos = 1;

	m_pVSL1->m_offset = rcVWND.y - 2;
	m_pVSL1->m_h = rcVWND.height + 4;
	m_pVSL1->m_min = rcVWND.x;
	m_pVSL1->m_max = m_pVSL1->m_min + rcVWND.width;
	//m_pVSL1->m_pos = 0;

	m_pVSL2->m_offset = m_pVSL1->m_offset;
	m_pVSL2->m_h = m_pVSL1->m_h;
	m_pVSL2->m_min = rcVWND.x;
	m_pVSL2->m_max = m_pVSL2->m_min + rcVWND.width;
	//m_VSL2->m_pos = 1;

	m_pHSL1->UpdateSL();
	m_pHSL2->UpdateSL();
	m_pVSL1->UpdateSL();
	m_pVSL2->UpdateSL();
}

BEGIN_EVENT_TABLE(CVideoBox, wxMDIChildFrame)
	EVT_SIZE(CVideoBox::OnSize)
	EVT_MENU(ID_TB_RUN, CVideoBox::OnBnClickedRun)
	EVT_MENU(ID_TB_PAUSE, CVideoBox::OnBnClickedPause)
	EVT_MENU(ID_TB_STOP, CVideoBox::OnBnClickedStop)
	EVT_KEY_DOWN(CVideoBox::OnKeyDown)
	EVT_MOUSEWHEEL(CVideoBox::OnMouseWheel)
	EVT_SCROLL_THUMBTRACK(CVideoBox::OnHScroll)
END_EVENT_TABLE()

CVideoBox::CVideoBox(CMainFrame* pMF)
		: wxMDIChildFrame(pMF, wxID_ANY, "", 
		          wxDefaultPosition, wxDefaultSize, 
				  wxTHICK_FRAME 
				  | wxCLIP_CHILDREN
				  | wxRESIZE_BORDER 
				  | wxCAPTION 
				  | wxWANTS_CHARS )
{
	m_w = 0;
	m_h = 0;
	m_pBmp = NULL;
	m_pBmpScaled = NULL;

	m_pMF = pMF;

	m_WasInited = false;
}

CVideoBox::~CVideoBox()
{
	if (m_pBmp != NULL)
	{
		delete m_pBmp;
		m_pBmp = NULL;

		delete m_pBmpScaled;
		m_pBmpScaled = NULL;
	}
}

void CVideoBox::Init()
{
	//string strVBClass;
	//string strVBXClass;
	wxBitmap bmp;

	m_VBX = wxColour(125, 125, 125);
	m_CL1 = wxColour(255, 255, 225);
	m_CL2 = wxColour(0, 0, 0);

	m_LBLFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);
	
	m_pVBar = new wxToolBar(this, wxID_ANY,
                               wxDefaultPosition, wxSize(380, 30), 
							   wxTB_HORIZONTAL | wxTB_BOTTOM | 
							   /*wxTB_NO_TOOLTIPS |*/ wxTB_FLAT );
	m_pVBar->SetMargins(4, 4);
	m_CLVBar = m_pVBar->GetBackgroundColour();

	LoadToolBarImage(bmp, "bitmaps/tb_run.bmp", m_CLVBar);
	m_pVBar->AddTool(ID_TB_RUN, _T(""), bmp, wxNullBitmap, wxITEM_CHECK);

	LoadToolBarImage(bmp, "bitmaps/tb_pause.bmp", m_CLVBar);
	m_pVBar->AddTool(ID_TB_PAUSE, _T(""), bmp, wxNullBitmap, wxITEM_CHECK);
	
	LoadToolBarImage(bmp, "bitmaps/tb_stop.bmp", m_CLVBar);
	m_pVBar->AddTool(ID_TB_STOP, _T(""), bmp, wxNullBitmap, wxITEM_CHECK);

	m_pVBar->Realize();

	m_plblVB = new CTextBox( this, ID_LBL_VB, wxT("Video Box") );
	m_plblVB->SetSize(0, 0, 390, 30);
	m_plblVB->SetFont(m_LBLFont);
	m_plblVB->SetBackgroundColour( m_CL1 );
	
	m_plblTIME = new CTextBox( m_pVBar, ID_LBL_TIME, 
									wxT("00:00:00,000/00:00:00,000") );
	m_plblTIME->SetSize(200, 242, 190, 26);
	m_plblTIME->SetFont(m_LBLFont);
	m_plblTIME->SetTextColour(*wxWHITE);
	m_plblTIME->SetBackgroundColour( m_CL2 );

	m_pVBox = new CVideoWindow(this);
	m_pVBox->Init();

	this->SetBackgroundColour(m_CLVBar);

	m_pSB = new CScrollBar(this, ID_TRACKBAR);
	m_pSB->SetScrollRange(0, 255);

	this->SetSize(20,20,402,300);

	m_WasInited = true;
}

void CVideoBox::OnSize(wxSizeEvent& event)
{
	int w, h, sbw, sbh, csbw, csbh;
	wxRect rlVB, rlTIME, rcSB, rcVBOX, rcVBAR;

	this->GetClientSize(&w, &h);
	
	rlVB.x = 0;
	rlVB.width = w;
	rlVB.y = 0;
	rlVB.height = 28;

	rcVBAR.width = w;
	rcVBAR.height = 30;
	rcVBAR.x = 0;
	rcVBAR.y = h - rcVBAR.height+6;

	m_pSB->GetClientSize(&csbw, &csbh);
	m_pSB->GetSize(&sbw, &sbh);

	rcSB.x = 2;
	rcSB.width = w - rcSB.x*2;
	rcSB.height = (sbh-csbh) + m_pSB->m_SBLA.GetHeight();
	rcSB.y = rcVBAR.y - rcSB.height - 1;	
	
	rcVBOX.x = 0;
	rcVBOX.width = w;
	rcVBOX.y = rlVB.GetBottom() + 1;
	rcVBOX.height = rcSB.y - rcVBOX.y - 2;

	m_plblVB->SetSize(rlVB);
	m_pVBox->SetSize(rcVBOX);
	m_pSB->SetSize(rcSB);
	m_pVBar->SetSize(rcVBAR);

	m_pVBar->GetClientSize(&w, &h);

	rlTIME.width = 150;
	rlTIME.height = 22;
	rlTIME.x = w - rlTIME.width;
	rlTIME.y = (h - rlTIME.height)/2-2;

	m_plblTIME->SetSize(rlTIME);

	this->Refresh(false);
	this->Update();

	event.Skip();
}

void CVideoBox::OnBnClickedRun(wxCommandEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		m_pVBar->ToggleTool(ID_TB_RUN, true);
		m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pVBar->ToggleTool(ID_TB_STOP, false);

		m_pMF->m_pVideo->Run();
		m_pMF->m_vs = m_pMF->Play;
	}
	else
	{
		m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pVBar->ToggleTool(ID_TB_STOP, false);
	}
}

void CVideoBox::OnBnClickedPause(wxCommandEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVBar->ToggleTool(ID_TB_PAUSE, true);
		m_pVBar->ToggleTool(ID_TB_STOP, false);

		m_pMF->m_pVideo->Pause();		
		m_pMF->m_vs = m_pMF->Pause;
	}
	else
	{
		m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pVBar->ToggleTool(ID_TB_STOP, false);
	}
}

void CVideoBox::OnBnClickedStop(wxCommandEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		wxCommandEvent cmd_event;
		m_pMF->OnStop(cmd_event);
	}
	else
	{
		m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pVBar->ToggleTool(ID_TB_STOP, false);
	}
}

void CVideoBox::OnKeyDown(wxKeyEvent& event)
{
	s64 Cur;

	if ( !m_pMF->m_VIsOpen )
	{
		event.Skip();
	}
	else
	{
		int key_code = event.GetKeyCode();

		switch ( key_code )
		{
			case WXK_RIGHT:
				m_pMF->PauseVideo();
				m_pMF->m_pVideo->OneStep();
				break;

			case WXK_UP:
				m_pMF->PauseVideo();
				m_pMF->m_pVideo->OneStep();
				break;

			case WXK_LEFT:
				m_pMF->PauseVideo();
				Cur = m_pMF->m_pVideo->GetPos();
				Cur -= m_pMF->m_dt;
				if (Cur < 0) Cur = 0;
				m_pMF->m_pVideo->SetPosFast(Cur);
				break;
			
			case WXK_DOWN:
				m_pMF->PauseVideo();
				Cur = m_pMF->m_pVideo->GetPos();
				Cur -= m_pMF->m_dt;
				if (Cur < 0) Cur = 0;
				m_pMF->m_pVideo->SetPosFast(Cur);
				break;

			default:
				event.Skip();
		}
	}
}

void CVideoBox::OnMouseWheel(wxMouseEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		s64 Cur;

		if (event.m_wheelRotation > 0)
		{
			m_pMF->PauseVideo();
			m_pMF->m_pVideo->OneStep();
		}
		else
		{
			m_pMF->PauseVideo();
			Cur = m_pMF->m_pVideo->GetPos();
			Cur -= m_pMF->m_dt;
			if (Cur < 0) Cur = 0;
			m_pMF->m_pVideo->SetPosFast(Cur);
		}
	}
}

void CVideoBox::ViewImage(int *Im, int w, int h)
{
	int i, x, y;
	u8 *color;
	wxMemoryDC dc;

	if (m_pBmp == NULL) 
	{
		m_pBmp = new wxBitmap(w, h);
		m_w = w;
		m_h = h;
	}
	else
	{
		if ((m_w!=w) || (m_h!=h))
		{
			delete m_pBmp;
			
			m_pBmp = new wxBitmap(w, h);
			m_w = w;
			m_h = h;
		}
	}

	dc.SelectObject(*m_pBmp);

	for(y=0, i=0; y<h; y++)
	for(x=0; x<w; x++, i++)
	{
		color = (u8*)(&Im[i]);

		dc.SetPen(wxPen(wxColor(color[2], color[1], color[0])));
		dc.DrawPoint(x, y);
	}

	if (m_pBmpScaled == NULL)
	{
		m_pBmpScaled = new wxBitmap(*m_pBmp);
	}
	else
	{
		*m_pBmpScaled = *m_pBmp;
	}
	m_wScaled = w;
	m_hScaled = h;

	m_pVBox->m_pVideoWnd->Refresh(false);
	m_pVBox->m_pVideoWnd->Update();
}

void CVideoBox::OnHScroll(wxScrollEvent& event)
{
	s64 SP = event.GetPosition();

	if (m_pMF->m_VIsOpen) 
	{
		s64 Cur, Pos, endPos;
		
		if (SP >= 0)
		{
			m_pMF->PauseVideo();
			
			Pos = SP*(s64)10000;

			endPos = m_pMF->m_pVideo->m_Duration;
			Cur = m_pMF->m_pVideo->GetPos();

			if (Pos != Cur)
			{
				m_pMF->m_pVideo->SetPosFast(Pos);
			}
		}
		else
		{
			if (SP == -1)
			{
				m_pMF->PauseVideo();
				m_pMF->m_pVideo->OneStep();
			}
			else
			{
				m_pMF->PauseVideo();
				Cur = m_pMF->m_pVideo->GetPos();
				Cur -= m_pMF->m_dt;
				if (Cur < 0) Cur = 0;
				m_pMF->m_pVideo->SetPosFast(Cur);				
			}
		}
	}
}

void CVideoWnd::OnSetFocus( wxFocusEvent &event )
{
	m_pVB->SetFocus();
}
