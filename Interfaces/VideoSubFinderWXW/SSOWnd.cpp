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
	wxString strPClass;

	SaveToReportLog("CSSOWnd::Init(): starting...\n");

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

	this->AddPage(m_pSHPanel, wxT("Search"), false, page_bmp );
	this->AddPage(m_pSSPanel, wxT("Settings"), false, page_bmp );
	this->AddPage(m_pOCRPanel, wxT("OCR"), false, page_bmp );

	this->SetFont(m_pMF->m_LBLFont);
	GetActiveTabCtrl()->SetFont(m_pMF->m_LBLFont);

	m_WasInited = true;

	SaveToReportLog("CSSOWnd::Init(): finished.\n");
}

void CSSOWnd::RefreshData()
{
	this->SetFont(m_pMF->m_LBLFont);
	GetActiveTabCtrl()->SetFont(m_pMF->m_LBLFont);
}
