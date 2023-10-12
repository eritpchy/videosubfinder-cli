                              //VideoBox.cpp//                                
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

#include "VideoBox.h"

BEGIN_EVENT_TABLE(CVideoWnd, wxWindow)
	EVT_PAINT(CVideoWnd::OnPaint)
	EVT_ERASE_BACKGROUND(CVideoWnd::OnEraseBackGround)
	EVT_LEFT_DOWN(CVideoWnd::OnLeftDown)
END_EVENT_TABLE()

CVideoWnd::CVideoWnd(CVideoWindow *pVW)
		:wxWindow(pVW, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS)
{
	m_pVW = pVW;
	m_pVB = pVW->m_pVB;
	m_filter_image = false;
}

CVideoWnd::~CVideoWnd()
{
}

void CVideoWnd::OnEraseBackGround(wxEraseEvent& event)
{
	bool do_erase = true;

	if (m_pVB)
	{
		if (m_pVB->m_pImage || m_pVB->m_pMF->m_VIsOpen)
		{
			do_erase = false;
		}
	}
	
	if (do_erase)
	{
		int w, h;
		this->GetClientSize(&w, &h);
		event.GetDC()->SetBrush(wxBrush(g_cfg.m_video_image_box_background_colour));
		event.GetDC()->DrawRectangle(0, 0, w, h);
	}
}

bool CVideoWnd::CheckFilterImage()
{
	bool filter_image = false;

	if ((m_pVB->m_pMF->m_VIsOpen) || (m_pVB->m_pImage != NULL))
	{

		if (wxGetKeyState(wxKeyCode('U')) || wxGetKeyState(wxKeyCode('u')))
		{
			filter_image = true;
		}
		else 
		{
			if (g_color_ranges.size() > 0)
			{
				if (wxGetKeyState(wxKeyCode('R')) || wxGetKeyState(wxKeyCode('r')) ||
					wxGetKeyState(wxKeyCode('T')) || wxGetKeyState(wxKeyCode('t')) ||
					wxGetKeyState(wxKeyCode('Y')) || wxGetKeyState(wxKeyCode('y')))
				{
					filter_image = true;
				}
			}

			if (!filter_image)
			{
				if (g_outline_color_ranges.size() > 0)
				{
					if (wxGetKeyState(wxKeyCode('R')) || wxGetKeyState(wxKeyCode('r')) ||
						wxGetKeyState(wxKeyCode('T')) || wxGetKeyState(wxKeyCode('t')) ||
						wxGetKeyState(wxKeyCode('I')) || wxGetKeyState(wxKeyCode('i')))
					{
						filter_image = true;
					}
				}
			}
		}		
	}

	return filter_image;
}

inline void FilterImage(simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImLab, const int w, const int h)
{
	bool bln_color_ranges = (g_color_ranges.size() > 0) ? true : false;
	bool bln_outline_color_ranges = (g_outline_color_ranges.size() > 0) ? true : false;	
	bool bln_r_pressed = (wxGetKeyState(wxKeyCode('R')) || wxGetKeyState(wxKeyCode('r')));
	bool bln_t_pressed = (wxGetKeyState(wxKeyCode('T')) || wxGetKeyState(wxKeyCode('t')));
	bool bln_y_pressed = (wxGetKeyState(wxKeyCode('Y')) || wxGetKeyState(wxKeyCode('y')));
	bool bln_i_pressed = (wxGetKeyState(wxKeyCode('I')) || wxGetKeyState(wxKeyCode('i')));
	int rc = GetBGRColor(ColorName::Red);
	int gc = GetBGRColor(ColorName::Green);
	int yc = GetBGRColor(ColorName::Yellow);
	int bc = 0;
	int pc;

	if (bln_y_pressed)
	{
		if ((!bln_t_pressed) && (!bln_r_pressed))
		{
			if (!bln_color_ranges)
			{
				return;
			}
		}
	}

	if (bln_i_pressed)
	{
		if ((!bln_t_pressed) && (!bln_r_pressed) && (!bln_y_pressed))
		{
			if (!bln_outline_color_ranges)
			{
				return;
			}
		}
	}

	if (bln_color_ranges || bln_outline_color_ranges)
	{
		int num_pixels = w * h;

		if (bln_r_pressed)
		{
			simple_buffer<u8> ImRes(w * h, (u8)255);
			int res = FilterImageByPixelColorIsInRange(ImRes, &ImBGR, &ImLab, w, h, wxT("FilterImage"), (u8)0, true, true);

			ImBGR.set_values(0);
			
			if (res != 0)
			{
				for (int p_id = 0; p_id < num_pixels; p_id++)
				{
					if (ImRes[p_id] != 0)
					{
						SetBGRColor(ImBGR, p_id, rc);
					}
				}
			}
		}
		else
		{
			int offset;			
			bool bln_in_color_ranges;
			bool bln_in_outline_color_ranges;

			if (!bln_t_pressed)
			{
				if (bln_y_pressed)
				{
					if (bln_color_ranges)
					{
						color_range cr = g_color_ranges[0];
						u8 mid_y;
						
						if (cr.m_color_space == ColorSpace::RGB)
						{						
							u8 mid_b, mid_g, mid_r;

							mid_b = (cr.m_min_data[2] + cr.m_max_data[2]) / 2;
							mid_g = (cr.m_min_data[1] + cr.m_max_data[1]) / 2;
							mid_r = (cr.m_min_data[0] + cr.m_max_data[0]) / 2;
							BGRToYUV(mid_b, mid_g, mid_r, &mid_y);
						}
						else
						{
							mid_y = (cr.m_min_data[0] + cr.m_max_data[0]) / 2;
						}

						if (mid_y < 50)
						{
							bc = GetBGRColor(ColorName::White);
						}
					}
				}
				else if (bln_i_pressed)
				{
					if (bln_outline_color_ranges)
					{
						color_range cr = g_outline_color_ranges[0];
						u8 mid_y;

						if (cr.m_color_space == ColorSpace::RGB)
						{
							u8 mid_b, mid_g, mid_r;

							mid_b = (cr.m_min_data[2] + cr.m_max_data[2]) / 2;
							mid_g = (cr.m_min_data[1] + cr.m_max_data[1]) / 2;
							mid_r = (cr.m_min_data[0] + cr.m_max_data[0]) / 2;
							BGRToYUV(mid_b, mid_g, mid_r, &mid_y);
						}
						else
						{
							mid_y = (cr.m_min_data[0] + cr.m_max_data[0]) / 2;
						}

						if (mid_y < 50)
						{
							bc = GetBGRColor(ColorName::White);
						}
					}
				}
			}

			for (int p_id = 0; p_id < num_pixels; p_id++)
			{
				bln_in_color_ranges = false;
				bln_in_outline_color_ranges = false;
				pc = -1;

				if (bln_color_ranges) bln_in_color_ranges = PixelColorIsInRange(g_color_ranges, &ImBGR, &ImLab, w, h, p_id);
				if (bln_outline_color_ranges) bln_in_outline_color_ranges = PixelColorIsInRange(g_outline_color_ranges, &ImBGR, &ImLab, w, h, p_id);

				if (bln_t_pressed)
				{
					if (bln_in_color_ranges)
					{
						if (bln_in_outline_color_ranges)
						{
							pc = yc;
						}
						else
						{
							pc = rc;
						}
					}
					else
					{
						if (bln_in_outline_color_ranges)
						{
							pc = gc;
						}
						else
						{
							pc = bc;
						}
					}
				}
				else if (bln_y_pressed)
				{
					if (!bln_in_color_ranges)
					{
						pc = bc;
					}
				}
				else if (bln_i_pressed)
				{
					if (!bln_in_outline_color_ranges)
					{
						pc = bc;
					}
				}

				if (pc != -1)
				{
					SetBGRColor(ImBGR, p_id, pc);
				}
			}
		}
	}
}

void CVideoWnd::DrawImage(simple_buffer<u8>& ImBGR, const int w, const int h)
{
	wxPaintDC dc(this);
	{
		int num_pixels = w * h;
		u8* img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

		for (int i = 0; i < num_pixels; i++)
		{
			img_data[(i * 3)] = ImBGR[(i * 3) + 2];
			img_data[(i * 3) + 1] = ImBGR[(i * 3) + 1];
			img_data[(i * 3) + 2] = ImBGR[(i * 3)];
		}

		wxImage Im(w, h, img_data);

		int cw, ch;
		this->GetClientSize(&cw, &ch);

		if ((cw != w) || (ch != h))
		{
			dc.DrawBitmap(Im.Scale(cw, ch, wxIMAGE_QUALITY_HIGH), 0, 0);
		}
		else
		{
			dc.DrawBitmap(Im, 0, 0);
		}
	}
}

void wxImageToBGR(wxImage& wxIm, simple_buffer<u8>& ImBGRRes)
{
	int num_pixels = wxIm.GetWidth() * wxIm.GetHeight();
	u8* img_data = wxIm.GetData();

	ImBGRRes.set_size(num_pixels * 3);

	for (int i = 0; i < num_pixels; i++)
	{
		ImBGRRes[(i * 3)] = img_data[(i * 3) + 2];
		ImBGRRes[(i * 3) + 1] = img_data[(i * 3) + 1];
		ImBGRRes[(i * 3) + 2] = img_data[(i * 3)];
	}
}

void CVideoWnd::OnPaint(wxPaintEvent& WXUNUSED(event))
{	
	if (m_pVB != NULL) 
	{		
		if (m_pVB->m_pMF->m_VIsOpen)
		{
			bool filter_image = CheckFilterImage();
			
			if (filter_image)
			{
				m_filter_image = true;
				int vw = m_pVB->m_pMF->m_pVideo->m_Width;
				int vh = m_pVB->m_pMF->m_pVideo->m_Height;
				int imw = vw;
				int imh = vh;
				simple_buffer<u8> ImBGR(imw * imh * 3), ImLab;
				m_pVB->m_pMF->m_pVideo->GetBGRImage(ImBGR, 0, imw - 1, 0, imh - 1);

				{
					cv::Mat cv_ImBGR, cv_ImLab;
					BGRImageToMat(ImBGR, imw, imh, cv_ImBGR);
					//cv::resize(cv_ImBGROrig, cv_ImBGR, cv::Size(0, 0), g_scale, g_scale);
					cv::cvtColor(cv_ImBGR, cv_ImLab, cv::COLOR_BGR2Lab);
					//imw *= g_scale;
					//imh *= g_scale;
					//ImBGR.set_size(imw * imh * 3);
					ImLab.set_size(imw * imh * 3);
					//BGRMatToImage(cv_ImBGR, imw, imh, ImBGR);
					BGRMatToImage(cv_ImLab, imw, imh, ImLab);
				}

				if ( !wxGetKeyState(wxKeyCode('U')) && !wxGetKeyState(wxKeyCode('u')) )
				{
					FilterImage(ImBGR, ImLab, imw, imh);
				}
				DrawImage(ImBGR, imw, imh);
			}
			else
			{
				m_filter_image = false;
				wxPaintDC dc(this);
				int cw, ch;
				this->GetClientSize(&cw, &ch);
				m_pVB->m_pMF->m_pVideo->SetVideoWindowPosition(0, 0, cw, ch, &dc);
			}
		}
		else if (m_pVB->m_pImage != NULL)
		{
			bool filter_image = CheckFilterImage();

			if (filter_image)
			{
				m_filter_image = true;
				int w = m_pVB->m_pImage->GetWidth();
				int h = m_pVB->m_pImage->GetHeight();
				simple_buffer<u8> ImBGR, ImLab;
				wxImageToBGR(*(m_pVB->m_pImage), ImBGR);

				{
					cv::Mat cv_ImBGR, cv_ImLab;
					BGRImageToMat(ImBGR, w, h, cv_ImBGR);
					//cv::resize(cv_ImBGROrig, cv_ImBGR, cv::Size(0, 0), g_scale, g_scale);
					cv::cvtColor(cv_ImBGR, cv_ImLab, cv::COLOR_BGR2Lab);
					//w *= g_scale;
					//h *= g_scale;
					//ImBGR.set_size(w * h * 3);
					ImLab.set_size(w * h * 3);
					//BGRMatToImage(cv_ImBGR, w, h, ImBGR);
					BGRMatToImage(cv_ImLab, w, h, ImLab);
				}

				if (!wxGetKeyState(wxKeyCode('U')) && !wxGetKeyState(wxKeyCode('u')))
				{
					FilterImage(ImBGR, ImLab, w, h);
				}
				DrawImage(ImBGR, w, h);
			}
			else
			{
				m_filter_image = false;
				wxPaintDC dc(this);
				int cw, ch;
				this->GetClientSize(&cw, &ch);
				dc.DrawBitmap(m_pVB->m_pImage->Scale(cw, ch), 0, 0);
			}
		}
		else
		{
			wxPaintDC dc(this);
			dc.Clear();
		}
	}
}

void CVideoWnd::OnLeftDown(wxMouseEvent& event)
{
	if (m_pVB != NULL)
	{
		int cw, ch, mx, my;
		this->GetClientSize(&cw, &ch);
		event.GetPosition(&mx, &my);

		if (((mx >= 0) && (mx < cw)) &&
			((my >= 0) && (my < ch)))
		{
			if ((m_pVB->m_pMF->m_VIsOpen) || (m_pVB->m_pImage != NULL))
			{
				wxClientDC dc(this);
				wxColour clr;
				dc.GetPixel(wxPoint(mx, my), &clr);

				u8 bgr[3], lab[3], y;

				bgr[0] = clr.Blue();
				bgr[1] = clr.Green();
				bgr[2] = clr.Red();

				BGRToYUV(bgr[0], bgr[1], bgr[2], &y);
				BGRToLab(bgr[0], bgr[1], bgr[2], &(lab[0]), &(lab[1]), &(lab[2]));

				g_cfg.m_pixel_color_bgr = wxString::Format(wxT("RGB: r:%d g:%d b:%d L:%d"), (int)(bgr[2]), (int)(bgr[1]), (int)(bgr[0]), (int)y);
				g_cfg.m_pixel_color_lab = wxString::Format(wxT("Lab: l:%d a:%d b:%d"), (int)(lab[0]), (int)(lab[1]), (int)(lab[2]));
				m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_pPixelColorRGB->SetValue(g_cfg.m_pixel_color_bgr);
				m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_pPixelColorLab->SetValue(g_cfg.m_pixel_color_lab);
				m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_PixelColorExample = wxColour(bgr[2], bgr[1], bgr[0]);
				m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_pPixelColorExample->SetBackgroundColour(m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_PixelColorExample);
				m_pVB->m_pMF->m_pPanel->m_pSSPanel->m_pPixelColorExample->Refresh();
			}
		}
	}

	event.Skip();
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
	m_pVideoWnd = NULL;
}

CVideoWindow::~CVideoWindow()
{
}

void CVideoWindow::Init()
{
	m_pVideoWnd = new CVideoWnd(this);

	m_pHSL1 = new CSeparatingLine(this, 200, 3, 7, 3, 100, 110, 50, 0, g_cfg.m_video_box_separating_line_colour, g_cfg.m_video_box_separating_line_border_colour);
	m_pHSL1->m_pos = 0;
	m_pHSL2 = new CSeparatingLine(this, 200, 3, 7, 3, 140, 150, 50, 0, g_cfg.m_video_box_separating_line_colour, g_cfg.m_video_box_separating_line_border_colour);
	m_pHSL2->m_pos = 1;
	m_pVSL1 = new CSeparatingLine(this, 3, 100, 3, 7, 100, 110, 50, 1, g_cfg.m_video_box_separating_line_colour, g_cfg.m_video_box_separating_line_border_colour);
	m_pVSL1->m_pos = 0;
	m_pVSL2 = new CSeparatingLine(this, 3, 100, 3, 7, 140, 150, 50, 1, g_cfg.m_video_box_separating_line_colour, g_cfg.m_video_box_separating_line_border_colour);
	m_pVSL2->m_pos = 1;

	m_pHSL1->Raise();
	m_pHSL2->Raise();
	m_pVSL1->Raise();
	m_pVSL2->Raise();

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

void CVideoWindow::Refresh(bool eraseBackground,
	const wxRect* rect)
{
	bool need_to_refresh = true;

	if (m_pVideoWnd)
	{
		if (m_pVideoWnd->GetParent() != this)
		{
			m_pVideoWnd->Refresh(true);
			need_to_refresh = false;
		}
	}
	
	if (need_to_refresh)
	{
		wxWindow::Refresh(eraseBackground, rect);
	}
}

void CVideoWindow::OnPaint( wxPaintEvent &event )
{
	wxPaintDC dc(this);	
}

void CVideoWindow::OnSize(wxSizeEvent& event)
{
	int w, h;
	wxRect rcCL, rcVWND;

	this->GetClientSize(&w, &h);
	
	rcVWND.x = 9;
	rcVWND.y = 9;
	rcVWND.width = w - rcVWND.x*2;
	rcVWND.height = h - rcVWND.y*2;

	m_pVideoWnd->SetSize(rcVWND);

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

BEGIN_EVENT_TABLE(CVideoBox, CResizableWindow)
	EVT_SIZE(CVideoBox::OnSize)
	EVT_BUTTON(ID_TB_RUN, CVideoBox::OnBnClickedRun)
	EVT_BUTTON(ID_TB_PAUSE, CVideoBox::OnBnClickedPause)
	EVT_BUTTON(ID_TB_STOP, CVideoBox::OnBnClickedStop)
	EVT_KEY_DOWN(CVideoBox::OnKeyDown)
	EVT_KEY_UP(CVideoBox::OnKeyUp)
	EVT_MOUSEWHEEL(CVideoBox::OnMouseWheel)
	EVT_SCROLL_THUMBTRACK(CVideoBox::OnHScroll)
	EVT_TIMER(TIMER_ID_VB, CVideoBox::OnTimer)
	EVT_RIGHT_DOWN(CVideoBox::OnRButtonDown)
END_EVENT_TABLE()

void CVideoBox::OnRButtonDown(wxMouseEvent& event)
{
	SetFocus();
	m_pHW->Popup();
}

CVideoBox::CVideoBox(CMainFrame* pMF)
		: CResizableWindow(pMF,
				  wxID_ANY,
		          wxDefaultPosition, wxDefaultSize), m_timer(this, TIMER_ID_VB)
{
	m_pImage = NULL;
	m_pMF = pMF;
	m_WasInited = false;
	m_pFullScreenWin = NULL;
}

CVideoBox::~CVideoBox()
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

void ReplaceColour(wxBitmap& bmp, wxColour orig_colour, wxColour repl_colour)
{
	if (bmp.IsOk())
	{
		int w = bmp.GetWidth();
		int h = bmp.GetHeight();
		wxColour Colour;

		wxMemoryDC dc;
		dc.SelectObject(bmp);
		dc.SetPen(wxPen(repl_colour));

		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				dc.GetPixel(x, y, &Colour);

				if (Colour == orig_colour)
				{
					dc.DrawPoint(x, y);
				}
			}
		}
	}
}

void GetToolBarImages(wxImage& image, wxImage& image_focused, wxImage& image_selected, wxImage& origImage,
						wxColour border_colour, wxColour border_colour_focused, wxColour border_colour_selected)
{
	wxBitmap bmp = wxBitmap(origImage);
	wxBitmap bmp_focused = bmp;
	wxBitmap bmp_selected = bmp;

	ReplaceColour(bmp, g_cfg.m_toolbar_bitmaps_transparent_colour, border_colour);
	ReplaceColour(bmp_focused, g_cfg.m_toolbar_bitmaps_transparent_colour, border_colour_focused);
	ReplaceColour(bmp_selected, g_cfg.m_toolbar_bitmaps_transparent_colour, border_colour_selected);

	image = bmp.ConvertToImage();
	image_focused = bmp_focused.ConvertToImage();
	image_selected = bmp_selected.ConvertToImage();	
}

void CVideoBox::Init()
{
	//wxString strVBClass;
	//wxString strVBXClass;
	wxImage image, image_focused, image_selected;

	int w = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	int h = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);

	m_pFullScreenWin = new wxFrame(m_pMF, wxID_ANY, wxT(""), wxPoint(0,0), wxSize(w, h), wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT);
	m_pFullScreenWin->ShowFullScreen(true);
	m_pFullScreenWin->Hide();

	this->SetBackgroundColour(g_cfg.m_video_image_box_border_colour);
	m_prevBackgroundColour = g_cfg.m_video_image_box_border_colour;

	wxColour border_colour_focused(((int)g_cfg.m_video_image_box_border_colour.Red()) * 9 / 10, ((int)g_cfg.m_video_image_box_border_colour.Green()) * 9 / 10, ((int)g_cfg.m_video_image_box_border_colour.Blue()) * 9 / 10);
	wxColour border_colour_selected(((int)g_cfg.m_video_image_box_border_colour.Red()) * 2 / 3, ((int)g_cfg.m_video_image_box_border_colour.Green()) * 2 / 3, ((int)g_cfg.m_video_image_box_border_colour.Blue()) * 2 / 3);

	m_tb_run_origImage = wxImage(g_app_dir + "/bitmaps/tb_run.bmp");
	GetToolBarImages(image, image_focused, image_selected, m_tb_run_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
	m_pButtonRun = new CBitmapButton(this, ID_TB_RUN, image, image_focused, image_selected, wxDefaultPosition, image.GetSize());

	m_tb_pause_origImage = wxImage(g_app_dir + "/bitmaps/tb_pause.bmp");
	GetToolBarImages(image, image_focused, image_selected, m_tb_pause_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
	m_pButtonPause = new CBitmapButton(this, ID_TB_PAUSE, image, image_focused, image_selected, wxDefaultPosition, image.GetSize());

	m_tb_stop_origImage = wxImage(g_app_dir + "/bitmaps/tb_stop.bmp");
	GetToolBarImages(image, image_focused, image_selected, m_tb_stop_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
	m_pButtonStop = new CBitmapButton(this, ID_TB_STOP, image, image_focused, image_selected, wxDefaultPosition, image.GetSize());

	m_pButtonPause->Disable();
	m_pButtonRun->Disable();
	m_pButtonStop->Disable();

	m_plblVB = new CStaticText(this, g_cfg.m_video_box_title, ID_LBL_VB);
	m_plblVB->SetSize(0, 0, 390, 30);
	m_plblVB->SetFont(m_pMF->m_LBLFont);
	m_plblVB->SetTextColour(g_cfg.m_main_text_colour);
	m_plblVB->SetBackgroundColour(g_cfg.m_video_image_box_title_colour);
	
	m_plblTIME = new CStaticText(this, g_cfg.m_video_box_lblTIME_label, ID_LBL_TIME, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL | wxTB_BOTTOM);
	m_plblTIME->SetSize(200, 242, 190, 26);
	m_plblTIME->SetFont(m_pMF->m_LBLFont);
	m_plblTIME->SetTextColour(g_cfg.m_video_box_time_text_colour);
	m_plblTIME->SetBackgroundColour(g_cfg.m_video_box_time_colour);

	m_pVBox = new CVideoWindow(this);
	m_pVBox->Init();

	m_pVBox->m_pVideoWnd->SetBackgroundColour(g_cfg.m_video_image_box_background_colour);

	m_pSB = new CScrollBar(this, ID_TRACKBAR);
	m_pSB->SetScrollRange(0, 255);

	this->SetSize(20,20,402,300);

	m_pHW = new CPopupHelpWindow(g_cfg.m_help_desc_hotkeys_for_video_box);

	m_plblVB->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblVB->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblVB->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblVB->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);
	m_plblVB->Bind(wxEVT_RIGHT_DOWN, &CVideoBox::OnRButtonDown, this);
	m_plblVB->m_pST->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblVB->m_pST->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblVB->m_pST->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblVB->m_pST->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);
	m_plblVB->m_pST->Bind(wxEVT_RIGHT_DOWN, &CVideoBox::OnRButtonDown, this);

	m_plblTIME->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblTIME->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblTIME->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblTIME->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);
	m_plblTIME->m_pST->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblTIME->m_pST->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblTIME->m_pST->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblTIME->m_pST->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);

	m_pVBox->m_pHSL1->m_pos = 1 - g_cfg.m_top_video_image_percent_end;
	m_pVBox->m_pHSL2->m_pos = 1 - g_cfg.m_bottom_video_image_percent_end;
	m_pVBox->m_pVSL1->m_pos = g_cfg.m_left_video_image_percent_end;
	m_pVBox->m_pVSL2->m_pos = g_cfg.m_right_video_image_percent_end;

	m_pVBox->Bind(wxEVT_RIGHT_DOWN, &CVideoBox::OnRButtonDown, this);

	m_pVBox->m_pVideoWnd->Bind(wxEVT_MOUSEWHEEL, &CVideoBox::OnMouseWheel, this);
	m_pVBox->m_pVideoWnd->Bind(wxEVT_RIGHT_DOWN, &CVideoBox::OnRButtonDown, this);
	m_pVBox->m_pVideoWnd->Bind(wxEVT_KEY_DOWN, &CVideoBox::OnKeyDown, this);
	m_pVBox->m_pVideoWnd->Bind(wxEVT_KEY_UP, &CVideoBox::OnKeyUp, this);

	m_WasInited = true;
}

void CVideoBox::UpdateSize()
{
	wxSizeEvent event;
	OnSize(event);
	m_pVBox->OnSize(event);
}

void CVideoBox::RefreshData()
{
	this->SetBackgroundColour(g_cfg.m_video_image_box_border_colour);
	m_pVBox->m_pVideoWnd->SetBackgroundColour(g_cfg.m_video_image_box_background_colour);

	if (g_cfg.m_video_image_box_background_colour != m_prevBackgroundColour)
	{
		wxColour border_colour_focused(((int)g_cfg.m_video_image_box_border_colour.Red()) * 9 / 10, ((int)g_cfg.m_video_image_box_border_colour.Green()) * 9 / 10, ((int)g_cfg.m_video_image_box_border_colour.Blue()) * 9 / 10);
		wxColour border_colour_selected(((int)g_cfg.m_video_image_box_border_colour.Red()) * 2 / 3, ((int)g_cfg.m_video_image_box_border_colour.Green()) * 2 / 3, ((int)g_cfg.m_video_image_box_border_colour.Blue()) * 2 / 3);

		GetToolBarImages(m_pButtonRun->m_image, m_pButtonRun->m_image_focused, m_pButtonRun->m_image_selected, m_tb_run_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
		m_pButtonRun->Refresh();

		GetToolBarImages(m_pButtonPause->m_image, m_pButtonPause->m_image_focused, m_pButtonPause->m_image_selected, m_tb_pause_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
		m_pButtonPause->Refresh();

		GetToolBarImages(m_pButtonStop->m_image, m_pButtonStop->m_image_focused, m_pButtonStop->m_image_selected, m_tb_stop_origImage, g_cfg.m_video_image_box_border_colour, border_colour_focused, border_colour_selected);
		m_pButtonStop->Refresh();
	}

	m_prevBackgroundColour = g_cfg.m_video_image_box_border_colour;

	m_pVBox->m_pHSL1->m_pos = 1 - g_cfg.m_top_video_image_percent_end;
	m_pVBox->m_pHSL2->m_pos = 1 - g_cfg.m_bottom_video_image_percent_end;
	m_pVBox->m_pVSL1->m_pos = g_cfg.m_left_video_image_percent_end;
	m_pVBox->m_pVSL2->m_pos = g_cfg.m_right_video_image_percent_end;
}

void CVideoBox::OnSize(wxSizeEvent& event)
{
	int w, h, sbw, sbh, csbw, csbh;
	wxRect rlVB, rlTIME, rcSB, rcVBOX, rcButtonRunPause, rcButtonStop;
	wxSize lblVB_opt_size = m_plblVB->GetOptimalSize();
	wxSize lblIB_opt_size = m_pMF->m_pImageBox ? m_pMF->m_pImageBox->m_plblIB->GetOptimalSize() : wxSize(0, 0);
	wxSize lblTIME_opt_size = m_plblTIME->GetOptimalSize(0);

	this->GetClientSize(&w, &h);
	
	rlVB.x = 0;
	rlVB.width = w;
	rlVB.y = 0;
	rlVB.height = std::max<int>(lblVB_opt_size.y, lblIB_opt_size.y);

	int bw, bh;
	wxSize opt_bsize = m_pButtonRun->GetOptimalSize(0, lblTIME_opt_size.y);
	bw = opt_bsize.x;
	bh = opt_bsize.y;

	rcButtonRunPause.width = bw;
	rcButtonRunPause.height = bh;
	rcButtonRunPause.x = 4;
	rcButtonRunPause.y = h - bh - rcButtonRunPause.x;

	rcButtonStop = rcButtonRunPause;
	rcButtonStop.x = rcButtonRunPause.x + rcButtonRunPause.width + 2;

	m_pSB->GetClientSize(&csbw, &csbh);
	m_pSB->GetSize(&sbw, &sbh);

	rcSB.x = rcButtonRunPause.x;
	rcSB.width = w - rcSB.x*2;
	rcSB.height = (sbh-csbh) + m_pSB->m_SBLA.GetHeight();
	rcSB.y = rcButtonRunPause.y - rcSB.height - 1;
	
	rcVBOX.x = rcButtonRunPause.x;
	rcVBOX.width = w - rcVBOX.x*2;
	rcVBOX.y = rlVB.GetBottom() + 1;
	rcVBOX.height = rcSB.y - rcVBOX.y - 2;

	m_plblVB->SetSize(rlVB);
	m_pVBox->SetSize(rcVBOX);
	m_pSB->SetSize(rcSB);	
	m_pButtonPause->SetSize(rcButtonRunPause);
	m_pButtonPause->Hide();
	m_pButtonRun->SetSize(rcButtonRunPause);
	m_pButtonStop->SetSize(rcButtonStop);
	
	rlTIME.x = rcButtonStop.x + rcButtonStop.width + 2;
	rlTIME.y = rcButtonStop.y;
	rlTIME.width = w - rlTIME.x - rcButtonRunPause.x;
	rlTIME.height = rcButtonStop.height;

	m_plblTIME->SetSize(rlTIME);

	this->Refresh(false);
	this->Update();

	event.Skip();
}

void CVideoBox::OnBnClickedRun(wxCommandEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_pMF->m_play_mutex);

	if (m_pMF->m_VIsOpen)
	{
		if (m_pMF->m_vs != m_pMF->Play)
		{
			m_pMF->m_vs = m_pMF->Play;
			m_pButtonRun->Hide();
			m_pButtonPause->Show();
			m_pMF->m_pVideo->Run();
		}
	}
	else
	{
		m_pButtonPause->Disable();
		m_pButtonRun->Disable();
		m_pButtonStop->Disable();
	}
}

void CVideoBox::OnBnClickedPause(wxCommandEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_pMF->m_play_mutex);

	if (m_pMF->m_VIsOpen)
	{
		if (m_pMF->m_vs != m_pMF->Pause)
		{
			m_pMF->m_vs = m_pMF->Pause;			
			m_pButtonPause->Hide();
			m_pButtonRun->Show();
			m_pMF->m_pVideo->Pause();
		}
	}
	else
	{
		m_pButtonPause->Disable();
		m_pButtonRun->Disable();
		m_pButtonStop->Disable();
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
		m_pButtonPause->Disable();
		m_pButtonRun->Disable();
		m_pButtonStop->Disable();
	}
}

void CVideoBox::OnKeyDown(wxKeyEvent& event)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	s64 Cur;
	int key_code = event.GetKeyCode();
	wxCommandEvent evt;

	switch (key_code)
	{
		case 'R':
		case 'r':
		case 'T':
		case 't':
		case 'Y':
		case 'y':
		case 'U':
		case 'u':
		case 'I':
		case 'i':
			// This fix issue in case of Run Search and redefining g_color_ranges and g_outline_color_ranges
			{
				std::vector<color_range> color_ranges = GetColorRanges(g_use_filter_color);
				std::vector<color_range> outline_color_ranges = GetColorRanges(g_use_outline_filter_color);

				if (g_color_ranges != color_ranges)
				{
					g_color_ranges = color_ranges;
				}

				if (g_outline_color_ranges != outline_color_ranges)
				{
					g_outline_color_ranges = outline_color_ranges;
				}
			}

			if (m_pVBox->m_pVideoWnd->CheckFilterImage())
			{
				if (!m_timer.IsRunning())
				{
					m_pVBox->m_pVideoWnd->Reparent(m_pFullScreenWin);

					wxSize cl_size = m_pFullScreenWin->GetClientSize();
					m_pVBox->m_pVideoWnd->SetSize(0, 0, cl_size.x, cl_size.y);

					m_pFullScreenWin->Show();

					m_timer.Start(100);
				}

				break;
			}
			break;
	}

	if (m_pMF->m_VIsOpen)
	{
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

			case WXK_SPACE:				
 				m_pMF->OnPlayPause(evt);
				break;

			case 'Z':
			case 'z':
				if (event.CmdDown())
				{
					m_pMF->OnEditSetBeginTime(evt);
				}
				break;
			
			case 'X':
			case 'x':
				if (event.CmdDown())
				{
					m_pMF->OnEditSetEndTime(evt);
				}
				break;
		}
	}

	switch ( key_code )
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

void CVideoBox::OnKeyUp(wxKeyEvent& event)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	int key_code = event.GetKeyCode();

	switch (key_code)
	{
		case 'R':
		case 'r':
		case 'T':
		case 't':
		case 'Y':
		case 'y':
		case 'U':
		case 'u':
		case 'I':
		case 'i':
			if (m_pVBox->m_pVideoWnd->CheckFilterImage())
			{
				m_pVBox->m_pVideoWnd->Refresh(false);
				m_pVBox->m_pVideoWnd->Update();
			}
			else
			{
				if (m_timer.IsRunning())
				{
					m_timer.Stop();
					wxTimerEvent te;
					this->OnTimer(te);
				}
			}
			break;
	}
}

// note: this is the hack for resolve lost keybard messages if click video image in fullscreen
void CVideoBox::OnTimer(wxTimerEvent& event)
{
	if (!m_pVBox->m_pVideoWnd->CheckFilterImage())
	{
		if (m_timer.IsRunning())
		{
			m_timer.Stop();
		}

		if (m_pFullScreenWin->IsShown())
		{
			// note: this is the hack for bring separating lines to top of video window
			m_pFullScreenWin->Hide();
			m_pVBox->m_pHSL1->Reparent(m_pFullScreenWin);
			m_pVBox->m_pHSL2->Reparent(m_pFullScreenWin);
			m_pVBox->m_pVSL1->Reparent(m_pFullScreenWin);
			m_pVBox->m_pVSL2->Reparent(m_pFullScreenWin);

			m_pVBox->m_pVideoWnd->Reparent(m_pVBox);
			m_pVBox->m_pHSL1->Reparent(m_pVBox);
			m_pVBox->m_pHSL2->Reparent(m_pVBox);
			m_pVBox->m_pVSL1->Reparent(m_pVBox);
			m_pVBox->m_pVSL2->Reparent(m_pVBox);

			m_pHW->Hide();

			wxSizeEvent event;
			m_pVBox->OnSize(event);

			this->SetFocus();
		}
	}
}

void CVideoBox::OnMouseWheel(wxMouseEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		wxCommandEvent cmd_event;

		if (event.m_wheelRotation > 0)
		{
			m_pMF->OnNextFrame(cmd_event);
		}
		else
		{
			m_pMF->OnPreviousFrame(cmd_event);
		}
	}
	else
	{
		m_pMF->OnMouseWheel(event);
	}
}

void CVideoBox::ViewImage(simple_buffer<int> &Im, int w, int h)
{
	int num_pixels = w*h;
	u8 *color;

	unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

	for (int i = 0; i < num_pixels; i++)
	{
		color = (u8*)(&Im[i]);
		img_data[i * 3] = color[2]; //r
		img_data[i * 3 + 1] = color[1]; //g
		img_data[i * 3 + 2] = color[0]; //b
	}

	{
		std::lock_guard<std::mutex> guard(m_view_mutex);

		if (m_pImage != NULL) delete m_pImage;
		m_pImage = new wxImage(w, h, img_data);

		if (m_timer.IsRunning())
		{
			m_pVBox->m_pVideoWnd->Refresh(false);
		}
		else
		{
			m_pVBox->Refresh(false);
		}
	}
}

void CVideoBox::ViewGrayscaleImage(simple_buffer<u8>& Im, int w, int h)
{
	int num_pixels = w * h;

	unsigned char* img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

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

		if (m_timer.IsRunning())
		{
			m_pVBox->m_pVideoWnd->Refresh(false);
		}
		else
		{
			m_pVBox->Refresh(false);
		}
	}
}

void CVideoBox::ViewBGRImage(simple_buffer<u8>& ImBGR, int w, int h)
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

		if (m_timer.IsRunning())
		{
			m_pVBox->m_pVideoWnd->Refresh(false);
		}
		else
		{
			m_pVBox->Refresh(false);
		}
	}
}

void CVideoBox::ClearScreen()
{
	if (m_pImage != NULL)
	{
		int w, h;
		m_pVBox->m_pVideoWnd->GetClientSize(&w, &h);

		delete m_pImage;
		m_pImage = new wxImage(w, h);
	}
	
	if (m_timer.IsRunning())
	{
		m_pVBox->m_pVideoWnd->Refresh(false);
	}
	else
	{
		m_pVBox->Refresh(false);
	}
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
			
			Pos = SP;

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


