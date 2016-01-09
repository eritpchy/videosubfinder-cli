                              //ImageBox.cpp//                                
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

	if ( (m_pIB != NULL) && (m_pIB->m_pBmp != NULL) )
	{
		int w, h;

		this->GetClientSize(&w, &h);

		if ( (w != m_pIB->m_wScaled) || (h != m_pIB->m_hScaled) )
		{
			*m_pIB->m_pBmpScaled = wxBitmap(m_pIB->m_pBmp->ConvertToImage().Scale(w, h));
			m_pIB->m_wScaled = w;
			m_pIB->m_hScaled = h;
		}

		dc.DrawBitmap(*m_pIB->m_pBmpScaled, 0, 0);
	}
}

BEGIN_EVENT_TABLE(CImageBox, wxMDIChildFrame)
	EVT_SIZE(CImageBox::OnSize)
END_EVENT_TABLE()

CImageBox::CImageBox(CMainFrame* pMF)
		: wxMDIChildFrame(pMF, wxID_ANY, "", 
		          wxDefaultPosition, wxDefaultSize, 
				  wxTHICK_FRAME 
				  | wxCLIP_CHILDREN
				  | wxRESIZE_BORDER 
				  | wxCAPTION 
				  | wxWANTS_CHARS )
{
	m_pMF = pMF;

	m_w = 0;
	m_h = 0;
	m_pBmp = NULL;
	m_pBmpScaled = NULL;

	m_WasInited = false;
}

CImageBox::~CImageBox()
{
	if (m_pBmp != NULL)
	{
		delete m_pBmp;
		m_pBmp = NULL;

		delete m_pBmpScaled;
		m_pBmpScaled = NULL;
	}
}

void CImageBox::Init()
{
	string strIBClass;
	string strIBXClass;

	m_IWColor = wxColour(125, 125, 125);

	m_CL1Color = wxColour(255, 255, 225);

	m_LBLFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);


	m_plblIB = new CTextBox( this, ID_LBL_IB, wxT("Image Box") );
	m_IBColor = m_plblIB->GetBackgroundColour();
	m_plblIB->SetSize(0, 0, 390, 30);
	m_plblIB->SetFont(m_LBLFont);
	m_plblIB->SetBackgroundColour( m_CL1Color );

	m_pIW = new CImageWnd(this);
	m_pIW->SetBackgroundColour(m_IWColor);

	this->SetBackgroundColour(m_IBColor);
	this->SetSize(20,20,402,300);

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
	if (m_pBmp != NULL) 
	{
		delete m_pBmp;
		m_pBmp = new wxBitmap(m_w, m_h);

		delete m_pBmpScaled;
		m_pBmpScaled = new wxBitmap(m_wScaled, m_hScaled);		

		m_pIW->Refresh(false);
	}
}

void CImageBox::ViewRGBImage(int *Im, int w, int h)
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

	m_pIW->Refresh(false);
	m_pIW->Update();
}

void CImageBox::ViewGrayscaleImage(int *Im, int w, int h)
{
	int i, x, y;
	u8 color;
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
		color = Im[i];

		dc.SetPen(wxPen(wxColor(color, color, color)));
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

	m_pIW->Refresh(false);
	m_pIW->Update();
}

void CImageBox::ViewImage(int *Im, int w, int h)
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

	wxPen white = wxPen(wxColor(255, 255, 255));
	wxPen black = wxPen(wxColor(0, 0, 0));

	for(y=0, i=0; y<h; y++)
	for(x=0; x<w; x++, i++)
	{
		color = (u8*)(&Im[i]);

		if (Im[i] == 0)
		{
			dc.SetPen(black);
			dc.DrawPoint(x, y);
		}
		else
		{
			dc.SetPen(white);
			dc.DrawPoint(x, y);
		}
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

	m_pIW->Refresh(false);
	m_pIW->Update();
}

