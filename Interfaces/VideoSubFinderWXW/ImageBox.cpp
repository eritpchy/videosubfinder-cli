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
			:wxWindow( pIB, wxID_ANY )
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
END_EVENT_TABLE()

CImageBox::CImageBox(CMainFrame* pMF)
		: CResizableWindow(pMF,//->GetClientWindow(),
			wxID_ANY,
			wxDefaultPosition, wxDefaultSize)
{
	m_pMF = pMF;
	m_pImage = NULL;
	m_WasInited = false;
}

CImageBox::~CImageBox()
{
	if (m_pImage != NULL)
	{
		delete m_pImage;
		m_pImage = NULL;
	}
}

void CImageBox::Init()
{
	wxString strIBClass;
	wxString strIBXClass;

	m_IWColor = wxColour(125, 125, 125);

	m_CL1Color = wxColour(255, 255, 225);

	m_plblIB = new CStaticText( this, ID_LBL_IB, wxT("Image Box") );
	m_IBColor = m_plblIB->GetBackgroundColour();
	m_plblIB->SetSize(0, 0, 390, 30);
	m_plblIB->SetFont(m_pMF->m_LBLFont);
	m_plblIB->SetBackgroundColour( m_CL1Color );

	m_pIW = new CImageWnd(this);
	m_pIW->SetBackgroundColour(m_IWColor);

	this->SetBackgroundColour(m_IBColor);
	this->SetSize(20,20,402,300);

	m_plblIB->Bind(wxEVT_MOTION, &CResizableWindow::OnMouseMove, this);
	m_plblIB->Bind(wxEVT_LEAVE_WINDOW, &CResizableWindow::OnMouseLeave, this);
	m_plblIB->Bind(wxEVT_LEFT_DOWN, &CResizableWindow::OnLButtonDown, this);
	m_plblIB->Bind(wxEVT_LEFT_UP, &CResizableWindow::OnLButtonUp, this);

	m_WasInited = true;
}

void CImageBox::OnSize( wxSizeEvent& event )
{
	int w, h;
	wxRect rcClIB, rlIB, rcIW;

	this->GetClientSize(&w, &h);
	rcClIB.x = rcClIB.y = 0;
	rcClIB.width = w;
	rcClIB.y = h;
	
	rlIB.x = 0;
	rlIB.y = 0;
	rlIB.width = rcClIB.width;
	rlIB.height = 28;

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

