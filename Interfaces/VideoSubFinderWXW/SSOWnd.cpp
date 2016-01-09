                              //SSOWnd.cpp//                                
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

#include "SSOWnd.h"

BEGIN_EVENT_TABLE(CSSOWnd, wxAuiNotebook)
//	ON_NOTIFY(TCN_SELCHANGE, ID_TAB, OnTcnSelchangeTab)
//	ON_WM_SIZE()
END_EVENT_TABLE()

/////////////////////////////////////////////////////////////////////////////

CSSOWnd::CSSOWnd(CMainFrame* pMF)
		: wxAuiNotebook(pMF, wxID_ANY, wxDefaultPosition, wxSize(400, 300), 
		               wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | 
					   wxAUI_NB_SCROLL_BUTTONS | wxBORDER )
{
	m_pMF = pMF;
	m_WasInited = false;
}

/////////////////////////////////////////////////////////////////////////////

CSSOWnd::~CSSOWnd()
{
}

/////////////////////////////////////////////////////////////////////////////

void CSSOWnd::Init()
{
	string strPClass;

	/*m_pAN = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxSize(400, 300), 
		               wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | 
					   wxAUI_NB_SCROLL_BUTTONS);*/

	//m_LBLFont.CreateFont(
	//	14,                        // nHeight
	//	0,                         // nWidth
	//	0,                         // nEscapement
	//	0,                         // nOrientation
	//	FW_NORMAL,                 // nWeight
	//	FALSE,                     // bItalic
	//	FALSE,                     // bUnderline
	//	0,                         // cStrikeOut
	//	ANSI_CHARSET,              // nCharSet
	//	OUT_DEFAULT_PRECIS,        // nOutPrecision
	//	CLIP_DEFAULT_PRECIS,       // nClipPrecision
	//	DEFAULT_QUALITY,           // nQuality
	//	DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
	//	"Microsoft Sans Serif");   // lpszFacename


	m_pSHPanel = new CSearchPanel(this);
	m_pSSPanel = new CSettingsPanel(this);
	m_pOCRPanel = new COCRPanel(this);
	m_pSHPanel->Init();
	m_pSSPanel->Init();
	m_pOCRPanel->Init();

	wxBitmap page_bmp = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16));

	this->AddPage(m_pSHPanel, wxT("Search"), false, page_bmp );
	this->AddPage(m_pSSPanel, wxT("Settings"), false, page_bmp );
	this->AddPage(m_pOCRPanel, wxT("OCR"), false, page_bmp );

	//this->SetMinSize(wxSize(400,300));
	//m_pAN->SetMinSize(wxSize(400,300));

	//this->SetSize(0, 0, 400, 300);

	//TabCtrlItem.pszText = "Search";
	//m_Tab.InsertItem( 0, &TabCtrlItem );
	//
	//TabCtrlItem.pszText = "Settings";
	//m_Tab.InsertItem( 1, &TabCtrlItem );
	//
	//TabCtrlItem.pszText = "OCR";
	//m_Tab.InsertItem( 2, &TabCtrlItem );

	//m_Tab.SetFont(&m_LBLFont);

	//m_SHPanel.Init(this);
	//m_SSPanel.Init(this);
	//m_OCRPanel.Init(this);

	//m_SSPanel.ShowWindow(false);
	//m_OCRPanel.ShowWindow(false);

	//ResizeControls();

	m_WasInited = true;
}

void CSSOWnd::ResizeControls()
{
	/*CRect rcTab, rcP1, rcP2, rcP3, rcBK, rcCl;
	int w, h;

	GetClientRect(&rcCl);
	m_SHPanel.GetWindowRect(&rcP1);
	m_SSPanel.GetWindowRect(&rcP2);
	m_OCRPanel.GetWindowRect(&rcP3);

	w = rcP1.Width();
	h = rcP1.Height();

	rcTab.left = 0;
	rcTab.right = rcCl.right;
	rcTab.top = 0;
	rcTab.bottom = 250;
	
	rcP1.left = (rcTab.Width() - w)/2;
	rcP1.right = rcP1.left + w;
	rcP1.bottom = rcTab.bottom - 60;
	rcP1.top = rcP1.bottom - h;

	w = rcP2.Width();
	h = rcP2.Height();

	rcP2.left = (rcTab.Width() - w)/2;
	rcP2.right = rcP2.left + w;
	rcP2.bottom = rcTab.bottom - 4;
	rcP2.top = rcP2.bottom - h;

	w = rcP3.Width();
	h = rcP3.Height();

	rcP3.left = (rcTab.Width() - w)/2;
	rcP3.right = rcP3.left + w;
	rcP3.top = rcTab.top + (rcTab.Height() - h)/2;
	rcP3.bottom = rcP3.top + h;
	
	m_Tab.MoveWindow(&rcTab, 1);
	
	switch(m_Tab.GetCurSel())
	{
		case 0:
			m_SHPanel.MoveWindow(&rcP1, 1);
			break;

		case 1:
			m_SSPanel.MoveWindow(&rcP2, 1);
			break;

		case 2:
			m_OCRPanel.MoveWindow(&rcP3, 1);
			break;
	}*/	
}

void CSSOWnd::OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult)
{
	/*switch(m_Tab.GetCurSel())
	{
		case 0:
			ResizeControls();
			m_SSPanel.ShowWindow(false);
			m_OCRPanel.ShowWindow(false);
			m_SHPanel.ShowWindow(true);
			break;

		case 1:
			ResizeControls();
			m_SHPanel.ShowWindow(false);
			m_OCRPanel.ShowWindow(false);
			m_SSPanel.ShowWindow(true);
			break;

		case 2:
			ResizeControls();
			m_SHPanel.ShowWindow(false);
			m_SSPanel.ShowWindow(false);
			m_OCRPanel.ShowWindow(true);
			break;
	}

	*pResult = 0;
	pNMHDR = NULL;*/
}

void CSSOWnd::OnSize(UINT nType, int cx, int cy)
{
	/*CMDIChildWnd::OnSize(nType, cx, cy);

	if (m_WasInited) ResizeControls();*/
}
