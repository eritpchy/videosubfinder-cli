                              //SearchPanel.cpp//                                
//////////////////////////////////////////////////////////////////////////////////
//							  Version 1.80              						//
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

#include "SearchPanel.h"

int g_IsSearching = 0;
int g_IsClose = 0;

BEGIN_EVENT_TABLE(CSearchPanel, wxPanel)
	EVT_BUTTON(ID_BTN_CLEAR, CSearchPanel::OnBnClickedClear)
	EVT_BUTTON(ID_BTN_RUN, CSearchPanel::OnBnClickedRun)
END_EVENT_TABLE()

CSearchPanel::CSearchPanel(CSSOWnd* pParent)
		:wxPanel( pParent, wxID_ANY )
{
	m_pParent = pParent;
	m_pMF = pParent->m_pMF;
}

CSearchPanel::~CSearchPanel()
{
}

void CSearchPanel::Init()
{
	m_CLP = wxColour(125,125,125);
	m_CL1 = wxColour(255, 215, 0);
	m_CL2 = wxColour(127, 255, 0);
	
	m_BTNFont = wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_BOLD, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

	m_LBLFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

	wxRect rcP1, rcClP1, rcBT1, rcBTA1, rcBT2, rcBTA2, rcClear, rcRun;

	rcBT1.x = 20;
	rcBT1.y = 20;
	rcBT1.width = 80;
	rcBT1.height = 16;
	
	rcBTA1.x = rcBT1.GetRight()+4;
	rcBTA1.y = rcBT1.y;
	rcBTA1.width = 260;
	rcBTA1.height = rcBT1.height;

	rcBT2.x = rcBT1.x;
	rcBT2.y = rcBT1.GetBottom() + 6;
	rcBT2.width = rcBT1.width;
	rcBT2.height = rcBT1.height;
	
	rcBTA2.x = rcBTA1.x;
	rcBTA2.y = rcBT2.y;
	rcBTA2.width = rcBTA1.width;
	rcBTA2.height = rcBT1.height;

	rcClear.x = rcBT2.x + 8;
	rcClear.y = rcBT2.GetBottom() + 10;
	rcClear.width = 150;	
	rcClear.height = 30;

	rcRun.width = rcClear.width;	
	rcRun.height = rcClear.height;
	rcRun.x = rcBTA2.GetRight() - 8 - rcRun.width;	
	rcRun.y = rcClear.y;
	
	rcP1.x = 10;
	rcP1.y = 10;
	rcP1.width = rcBTA1.GetRight() + rcBT1.x;
	rcP1.height = rcRun.GetBottom() + rcBT1.y;

	m_pP1 = new wxPanel( this, wxID_ANY, rcP1.GetPosition(), rcP1.GetSize() );
	m_pP1->SetMinSize(rcP1.GetSize());

	m_plblBT1 = new wxStaticText( m_pP1, wxID_ANY,
		wxT("  Begin Time:"), rcBT1.GetPosition(), rcBT1.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );

	m_plblBT2 = new wxStaticText( m_pP1, wxID_ANY,
		wxT("  End Time:"), rcBT2.GetPosition(), rcBT2.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );

	m_plblBTA1 = new wxStaticText( m_pP1, wxID_ANY,
		wxT(""), rcBTA1.GetPosition(), rcBTA1.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );

	m_plblBTA2 = new wxStaticText( m_pP1, wxID_ANY,
		wxT(""), rcBTA2.GetPosition(), rcBTA2.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );

	m_pClear = new wxButton( m_pP1, ID_BTN_CLEAR,
		wxT("Clear Folders"), rcClear.GetPosition(), rcClear.GetSize() );

	m_pRun = new wxButton( m_pP1, ID_BTN_RUN,
		wxT("Run Search"), rcRun.GetPosition(), rcRun.GetSize() );
	
	m_pP1->SetBackgroundColour( m_CLP );
	m_plblBT1->SetBackgroundColour( m_CL2 );
	m_plblBT2->SetBackgroundColour( m_CL2 );
	m_plblBTA1->SetBackgroundColour( m_CL1 );
	m_plblBTA2->SetBackgroundColour( m_CL1 );

	m_plblBT1->SetFont(m_LBLFont);
	m_plblBT2->SetFont(m_LBLFont);
	m_plblBTA1->SetFont(m_LBLFont);
	m_plblBTA2->SetFont(m_LBLFont);
	m_pClear->SetFont(m_BTNFont);
	m_pRun->SetFont(m_BTNFont);

	wxBoxSizer *top_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

	button_sizer->Add(m_pP1, 1, wxALIGN_CENTER, 0 );

	top_sizer->Add(button_sizer, 1, wxALIGN_CENTER );

	this->SetSizer(top_sizer);
}

void CSearchPanel::OnBnClickedRun(wxCommandEvent& event)
{
	if (m_pMF->m_VIsOpen)
	{
		wxCommandEvent event;
		m_pMF->OnStop(event);

		m_pMF->m_VIsOpen = false;

		if ( m_pMF->m_timer.IsRunning() ) 
		{
			m_pMF->m_timer.Stop();
		}

		m_pMF->m_ct = -1;

		
		m_pMF->m_timer.Start(1000);

		m_pRun->SetLabel("Stop Search");

		m_pMF->m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pMF->m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pMF->m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, false);

		m_pMF->m_pPanel->m_pSSPanel->Disable();
		m_pMF->m_pPanel->m_pOCRPanel->Disable();
		m_pMF->m_pImageBox->ClearScreen();

		InitIPData((int)m_pMF->m_pVideo->m_Width, (int)m_pMF->m_pVideo->m_Height, 1);

		m_pSearchThread = new ThreadSearchSubtitles(m_pMF);
		m_pSearchThread->Create();
		m_pSearchThread->Run();
		m_pSearchThread->SetPriority(30); //THREAD_PRIORITY_BELOW_NORMAL
	}
	else
	{
		if (g_RunSubSearch == 1) 
		{
			g_RunSubSearch = 0;
		}
	}
}

void CSearchPanel::OnBnClickedClear(wxCommandEvent& event)
{
	m_pMF->ClearDir("RGBImages");
	m_pMF->ClearDir("FRDImages");
	m_pMF->ClearDir("TSTImages");
	m_pMF->ClearDir("TXTImages");
	m_pMF->ClearDir("TestImages");
	m_pMF->ClearDir("TXTResults");	
}

ThreadSearchSubtitles::ThreadSearchSubtitles(CMainFrame *pMF)
        : wxThread()
{
    m_pMF = pMF;
}

void *ThreadSearchSubtitles::Entry()
{
	wxEvtHandler *handler;

	g_IsSearching = 1;

	if (g_fast_search == true)
	{
        if ( m_pMF->m_pVideo->SetNullRender() )
        {
	        SetVideoWindowSettins(m_pMF->m_pVideo, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

            m_pMF->m_BegTime = FastSearchSubtitles( m_pMF->m_pVideo, m_pMF->m_BegTime, m_pMF->m_EndTime );
        }
	}
	else
	{
        if ( m_pMF->m_pVideo->SetNullRender() )
        {
	        SetVideoWindowSettins(m_pMF->m_pVideo, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

		    m_pMF->m_BegTime = SearchSubtitles( m_pMF->m_pVideo, m_pMF->m_BegTime, m_pMF->m_EndTime );
        }
	}

	if (g_IsClose == 1) 
	{
		g_IsSearching = 0;
		m_pMF->Close();
		return 0;
	}

	m_pMF->m_pPanel->m_pSHPanel->m_pRun->SetLabel("Run Search");

	if (m_pMF->m_pVideoBox->m_pBmp != NULL)
	{
		delete m_pMF->m_pVideoBox->m_pBmp;
		m_pMF->m_pVideoBox->m_pBmp = NULL;
	}

	m_pMF->m_pPanel->m_pSSPanel->Enable();
	m_pMF->m_pPanel->m_pOCRPanel->Enable();
	
	wxCommandEvent  menu_event(wxEVT_COMMAND_MENU_SELECTED, ID_FILE_REOPENVIDEO);
	handler = m_pMF->GetEventHandler(); 
	wxPostEvent(handler, menu_event);

	if ( (g_RunSubSearch == 1) && (g_CLEAN_RGB_IMAGES == true) )
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CCTI);
		handler = m_pMF->m_pPanel->m_pOCRPanel->GetEventHandler(); 
		wxPostEvent(handler, bn_event);
		//m_pMF->m_pPanel->m_pOCRPanel->OnBnClickedCreateClearedTextImages(bn_event);
	}

	g_IsSearching = 0;
	g_RunSubSearch = 0;

	return 0;
}

