                              //ImageBox.cpp//                                
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

#include "ImageBox.h"

BEGIN_EVENT_TABLE(CImageWnd, wxWindow)
	EVT_PAINT(CImageWnd::OnPaint)
END_EVENT_TABLE()

CImageWnd::CImageWnd(CImageBox *pIB)
			:wxWindow(pIB, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{
	m_pIB = pIB;
}

CImageWnd::~CImageWnd()
{
}

void CImageWnd::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);

	if (m_pIB != NULL)
	{
		if (m_pIB->m_pImage != NULL)
		{
			int w, h;
			this->GetClientSize(&w, &h);
			dc.DrawBitmap(m_pIB->m_pImage->Scale(w, h), 0, 0);
		}
		else
		{
			dc.Clear();
		}
	}
}

BEGIN_EVENT_TABLE(CImageBox, CResizableWindow)
	EVT_SIZE(CImageBox::OnSize)
	EVT_KEY_DOWN(CImageBox::OnKeyDown)
	EVT_KEY_UP(CImageBox::OnKeyUp)
	EVT_MOUSEWHEEL(CImageBox::OnMouseWheel)
	EVT_TIMER(TIMER_ID_IB, CImageBox::OnTimer)
	EVT_RIGHT_DOWN(CImageBox::OnRButtonDown)
END_EVENT_TABLE()

void CImageBox::OnRButtonDown(wxMouseEvent& event)
{
	SetFocus();
	m_pHW->Popup();
}

void CImageBox::OnMouseWheel(wxMouseEvent& event)
{
	if ( (m_pImage != NULL) && (g_IsSearching == 0) && (g_IsCreateClearedTextImages == 0) )
	{
		wxCommandEvent evt;

		if (event.m_wheelRotation > 0)
		{
			m_pMF->m_pPanel->m_pSSPanel->OnBnClickedRight(evt);
		}
		else
		{
			m_pMF->m_pPanel->m_pSSPanel->OnBnClickedLeft(evt);
		}
	}
}

void CImageBox::OnKeyDown(wxKeyEvent& event)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	s64 Cur;
	int key_code = event.GetKeyCode();
	wxCommandEvent evt;

	if (m_pImage != NULL)
	{
		switch (key_code)
		{
		case WXK_RIGHT:
			if ( (g_IsSearching == 0) && (g_IsCreateClearedTextImages == 0) )
			{
				m_pMF->m_pPanel->m_pSSPanel->OnBnClickedRight(evt);
			}
			break;

		case WXK_LEFT:
			if ( (g_IsSearching == 0) && (g_IsCreateClearedTextImages == 0) )
			{
				m_pMF->m_pPanel->m_pSSPanel->OnBnClickedLeft(evt);
			}
			break;

		case 'U':
		case 'u':
			if (!m_timer.IsRunning())
			{
				m_pIW->Reparent(m_pFullScreenWin);

				wxSize cl_size = m_pFullScreenWin->GetClientSize();
				m_pIW->SetSize(0, 0, cl_size.x, cl_size.y);

				m_pFullScreenWin->Show();

				m_timer.Start(100);
			}

			break;
		}
	}

	switch (key_code)
	{
		case 'S':
		case 's':
			if (event.CmdDown())
			{
				m_pMF->OnFileSaveSettings(evt);
			}
			break;
	}
}

void CImageBox::OnKeyUp(wxKeyEvent& event)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	s64 Cur;
	int key_code = event.GetKeyCode();
	wxCommandEvent evt;

	if (m_pImage != NULL)
	{
		switch (key_code)
		{
		case 'U':
		case 'u':
			if (m_timer.IsRunning())
			{
				m_timer.Stop();
				wxTimerEvent te;
				this->OnTimer(te);
			}

			break;
		}
	}
}

// note: this is the hack for resolve lost keybard messages if click image in fullscreen
void CImageBox::OnTimer(wxTimerEvent& event)
{
	if ( (m_pImage != NULL) && (!wxGetKeyState(wxKeyCode('U')) && !wxGetKeyState(wxKeyCode('u'))) )
	{
		if (m_timer.IsRunning())
		{
			m_timer.Stop();
		}

		if (m_pFullScreenWin->IsShown())
		{
			m_pFullScreenWin->Hide();
			m_pIW->Reparent(this);

			m_pHW->Hide();

			wxSizeEvent event;
			OnSize(event);

			this->SetFocus();
		}
	}
}

CImageBox::CImageBox(CMainFrame* pMF)
		: CResizableWindow(pMF,
			wxID_ANY,
			wxDefaultPosition, wxDefaultSize), m_timer(this, TIMER_ID_IB)
{
	m_pMF = pMF;
	m_pImage = NULL;
	m_WasInited = false;
	m_pFullScreenWin = NULL;
}

CImageBox::~CImageBox()
{
	if (m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}

	if (m_pFullScreenWin != NULL)
	{
		m_pFullScreenWin->Destroy();
	}

	if (m_pHW != NULL)
	{
		m_pHW->Destroy();
	}
}

void CImageBox::Init()
{
	wxString strIBClass;
	wxString strIBXClass;

	int w = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	int h = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);

	m_pFullScreenWin = new wxFrame(m_pMF, wxID_ANY, wxT(""), wxPoint(0, 0), wxSize(w, h), wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT);
	m_pFullScreenWin->ShowFullScreen(true);
	m_pFullScreenWin->Hide();

	m_plblIB = new CStaticText(this, g_cfg.m_image_box_title, ID_LBL_IB);
	m_plblIB->SetSize(0, 0, 390, 30);
	m_plblIB->SetFont(m_pMF->m_LBLFont);
	m_plblIB->SetTextColour(g_cfg.m_main_text_colour);
	m_plblIB->SetBackgroundColour(g_cfg.m_video_image_box_title_colour);

	m_pIW = new CImageWnd(this);
	m_pIW->SetBackgroundColour(g_cfg.m_video_image_box_background_colour);

	this->SetBackgroundColour(g_cfg.m_video_image_box_border_colour);
	this->SetSize(20,20,402,300);

	m_pHW = new CPopupHelpWindow(g_cfg.m_help_desc_hotkeys_for_image_box);

	m_plblIB->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblIB->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblIB->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblIB->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);
	m_plblIB->Bind(wxEVT_RIGHT_DOWN, &CImageBox::OnRButtonDown, this);
	m_plblIB->m_pST->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblIB->m_pST->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblIB->m_pST->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblIB->m_pST->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);
	m_plblIB->m_pST->Bind(wxEVT_RIGHT_DOWN, &CImageBox::OnRButtonDown, this);

	m_pIW->Bind(wxEVT_KEY_DOWN, &CImageBox::OnKeyDown, this);
	m_pIW->Bind(wxEVT_KEY_UP, &CImageBox::OnKeyUp, this);
	m_pIW->Bind(wxEVT_MOUSEWHEEL, &CImageBox::OnMouseWheel, this);
	m_pIW->Bind(wxEVT_RIGHT_DOWN, &CImageBox::OnRButtonDown, this);

	m_WasInited = true;
}

void CImageBox::UpdateSize()
{
	wxSizeEvent event;
	OnSize(event);
}

void CImageBox::RefreshData()
{
	this->SetBackgroundColour(g_cfg.m_video_image_box_border_colour);
	m_pIW->SetBackgroundColour(g_cfg.m_video_image_box_background_colour);
}

void CImageBox::OnSize( wxSizeEvent& event )
{
	int w, h;
	wxRect rcClIB, rlIB, rcIW;
	wxSize lblVB_opt_size = m_pMF->m_pVideoBox ? m_pMF->m_pVideoBox->m_plblVB->GetOptimalSize() : wxSize(0, 0);
	wxSize lblIB_opt_size = m_plblIB->GetOptimalSize();

	this->GetClientSize(&w, &h);
	rcClIB.x = rcClIB.y = 0;
	rcClIB.width = w;
	rcClIB.y = h;
	
	rlIB.x = 0;
	rlIB.y = 0;
	rlIB.width = rcClIB.width;
	rlIB.height = std::max<int>(lblVB_opt_size.y, lblIB_opt_size.y);

	rcIW.x = 9;
	rcIW.y = rlIB.GetBottom() + 9;
	rcIW.width = rcClIB.width - rcIW.x*2;
	rcIW.height = rcClIB.GetBottom() - 9 - rcIW.y;

	m_plblIB->SetSize(rlIB);
	m_pIW->SetSize(rcIW);

	m_pIW->Refresh(false);
	m_pIW->Update();
}

void CImageBox::ClearScreen()
{
	if (m_pImage != NULL)
	{
		int w, h;
		m_pIW->GetClientSize(&w, &h);

		delete m_pImage;
		m_pImage = new wxImage(w, h);		
	}
	m_pIW->Refresh(false);
}

void CImageBox::ViewRGBImage(simple_buffer<int> &Im, int w, int h)
{
	int num_pixels = w*h;
	u8 *color;

	unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

	for (int i = 0; i < num_pixels; i++)
	{
		color = (u8*)(&Im[i]);
		img_data[i * 3] = color[2];
		img_data[i * 3 + 1] = color[1];
		img_data[i * 3 + 2] = color[0];
	}

	{
		std::lock_guard<std::mutex> guard(m_view_mutex);

		if (m_pImage != NULL) delete m_pImage;
		m_pImage = new wxImage(w, h, img_data);

		m_pIW->Refresh(false);
		m_pIW->Update();
	}
}

void CImageBox::ViewGrayscaleImage(simple_buffer<u8> &Im, int w, int h)
{
	int num_pixels = w*h;

	unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

	for (int i = 0; i < num_pixels; i++)
	{
		img_data[i * 3] = Im[i];
		img_data[i * 3 + 1] = Im[i];
		img_data[i * 3 + 2] = Im[i];
	}

	{
		std::lock_guard<std::mutex> guard(m_view_mutex);

		if (m_pImage != NULL) delete m_pImage;
		m_pImage = new wxImage(w, h, img_data);

		m_pIW->Refresh(false);
		m_pIW->Update();
	}
}

void CImageBox::ViewImage(simple_buffer<int> &Im, int w, int h)
{
	int num_pixels = w*h;
	u8 *color;

	unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

	for (int i = 0; i < num_pixels; i++)
	{
		color = (u8*)(&Im[i]);
		img_data[i * 3] = color[0];
		img_data[i * 3 + 1] = color[0];
		img_data[i * 3 + 2] = color[0];
	}

	{
		std::lock_guard<std::mutex> guard(m_view_mutex);

		if (m_pImage != NULL) delete m_pImage;
		m_pImage = new wxImage(w, h, img_data);

		m_pIW->Refresh(false);
		m_pIW->Update();
	}
}

void CImageBox::ViewBGRImage(simple_buffer<u8>& ImBGR, int w, int h)
{
	int num_pixels = w * h;
	unsigned char* img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

	for (int i = 0; i < num_pixels; i++)
	{
		img_data[i * 3] = ImBGR[i * 3 + 2];
		img_data[i * 3 + 1] = ImBGR[i * 3 + 1];
		img_data[i * 3 + 2] = ImBGR[i * 3];
	}

	{
		std::lock_guard<std::mutex> guard(m_view_mutex);

		if (m_pImage != NULL) delete m_pImage;
		m_pImage = new wxImage(w, h, img_data);

		m_pIW->Refresh(false);
		m_pIW->Update();
	}
}

