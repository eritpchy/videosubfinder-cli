                              //SSOWnd.cpp//                                
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

#define _HAS_STD_BYTE 0
#include "SSOWnd.h"

/////////////////////////////////////////////////////////////////////////////

CTabArt::CTabArt(CMainFrame* pMF) : wxAuiGenericTabArt()
{
	m_pMF = pMF;
}

void CTabArt::DrawTab(wxDC& dc,
	wxWindow* wnd,
	const wxAuiNotebookPage& pane,
	const wxRect& inRect,
	int closeButtonState,
	wxRect* outTabRect,
	wxRect* outButtonRect,
	int* xExtent)
{	
	SetColour(g_cfg.m_notebook_colour);
	SetActiveColour(g_cfg.m_notebook_colour);
	SetNormalFont(m_pMF->m_LBLFont);
	SetSelectedFont(m_pMF->m_LBLFont);
	SetMeasuringFont(m_pMF->m_LBLFont);
	wxAuiGenericTabArt::DrawTab(dc, wnd, pane, inRect, closeButtonState, outTabRect, outButtonRect, xExtent);
}

/////////////////////////////////////////////////////////////////////////////

CSSOWnd::CSSOWnd(CMainFrame* pMF)
		: wxAuiNotebook(pMF, wxID_ANY, wxDefaultPosition, wxSize(400, 300), 
		               wxAUI_NB_TOP | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | 
					   wxAUI_NB_SCROLL_BUTTONS | wxBORDER )
{
	m_pMF = pMF;
	m_WasInited = false;
	m_pTabArt = NULL;
}

/////////////////////////////////////////////////////////////////////////////

CSSOWnd::~CSSOWnd()
{
}

/////////////////////////////////////////////////////////////////////////////

void CSSOWnd::Init()
{
	wxString strPClass;

	SaveToReportLog("CSSOWnd::Init(): starting...\n");

	SetBackgroundColour(g_cfg.m_notebook_colour);

	this->SetFont(m_pMF->m_LBLFont);

	m_pTabArt = new CTabArt(m_pMF);
	SetArtProvider(m_pTabArt);

	SaveToReportLog("CSSOWnd::Init(): new CSearchPanel(this)...\n");
	m_pSHPanel = new CSearchPanel(this);
	SaveToReportLog("CSSOWnd::Init(): new CSettingsPanel(this)...\n");
	m_pSSPanel = new CSettingsPanel(this);
	SaveToReportLog("CSSOWnd::Init(): new COCRPanel(this)...\n");
	m_pOCRPanel = new COCRPanel(this);
	SaveToReportLog("CSSOWnd::Init(): m_pSHPanel->Init()...\n");
	m_pSHPanel->Init();
	SaveToReportLog("CSSOWnd::Init(): m_pSSPanel->Init()...\n");
	m_pSSPanel->Init();
	SaveToReportLog("CSSOWnd::Init(): m_pOCRPanel->Init()...\n");
	m_pOCRPanel->Init();

	wxBitmap page_bmp = wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16));

	SaveToReportLog("CSSOWnd::Init(): AddPage's...\n");

	this->AddPage(m_pSHPanel, g_cfg.m_search_panel_title, false, page_bmp );
	this->AddPage(m_pSSPanel, g_cfg.m_settings_panel_title, false, page_bmp );
	this->AddPage(m_pOCRPanel, g_cfg.m_ocr_panel_title, false, page_bmp );

	SetTabCtrlHeight(-1);

	m_WasInited = true;

	SaveToReportLog("CSSOWnd::Init(): finished.\n");
}

void CSSOWnd::RefreshData()
{
	SetBackgroundColour(g_cfg.m_notebook_colour);

	this->SetPageText(0, g_cfg.m_search_panel_title);
	this->SetPageText(1, g_cfg.m_settings_panel_title);
	this->SetPageText(2, g_cfg.m_ocr_panel_title);
	
	this->SetFont(m_pMF->m_LBLFont);
	SetTabCtrlHeight(-1);
}
