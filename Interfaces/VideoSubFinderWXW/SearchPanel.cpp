                              //SearchPanel.cpp//                                
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
#include "SearchPanel.h"
#include <exception>
#include <wx/sound.h>

int g_IsSearching = 0;
int g_IsClose = 0;

wxDEFINE_EVENT(THREAD_SEARCH_SUBTITLES_END, wxCommandEvent);

BEGIN_EVENT_TABLE(CSearchPanel, wxPanel)
	EVT_COMMAND(wxID_ANY, THREAD_SEARCH_SUBTITLES_END, CSearchPanel::ThreadSearchSubtitlesEnd)
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
	SaveToReportLog("CSearchPanel::Init(): starting...\n");

	wxRect rcP1, rcClP1, rcBT1, rcBTA1, rcBT2, rcBTA2, rcClear, rcRun;

	rcBT1.x = 20;
	rcBT1.y = 20;
	rcBT1.width = 90;
	rcBT1.height = 20;
	
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

	SaveToReportLog("CSearchPanel::Init(): init m_pP1...\n");
	m_pP1 = new wxPanel( this, wxID_ANY, rcP1.GetPosition(), rcP1.GetSize() );
	m_pP1->SetMinSize(rcP1.GetSize());
	m_pP1->SetBackgroundColour(g_cfg.m_notebook_panels_colour);

	SaveToReportLog("CSearchPanel::Init(): init m_plblBT1...\n");

	m_plblBT1 = new CStaticText( m_pP1, wxID_ANY, g_cfg.m_label_begin_time);
	m_plblBT1->SetSize(rcBT1);

	SaveToReportLog("CSearchPanel::Init(): init m_plblBT2...\n");
	m_plblBT2 = new CStaticText( m_pP1, wxID_ANY, g_cfg.m_label_end_time);
	m_plblBT2->SetSize(rcBT2);

	SaveToReportLog("CSearchPanel::Init(): init m_plblBTA1...\n");
	m_plblBTA1 = new CTextCtrl(m_pP1, wxID_ANY,
		wxT(""), rcBTA1.GetPosition(), rcBTA1.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER);

	SaveToReportLog("CSearchPanel::Init(): init m_plblBTA2...\n");
	m_plblBTA2 = new CTextCtrl( m_pP1, wxID_ANY,
		wxT(""), rcBTA2.GetPosition(), rcBTA2.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );

	SaveToReportLog("CSearchPanel::Init(): init m_pClear...\n");
	m_pClear = new CButton( m_pP1, ID_BTN_CLEAR, g_cfg.m_main_buttons_colour, g_cfg.m_main_buttons_colour_focused, g_cfg.m_main_buttons_colour_selected, g_cfg.m_main_buttons_border_colour,
		g_cfg.m_button_clear_folders_text, rcClear.GetPosition(), rcClear.GetSize() );

	SaveToReportLog("CSearchPanel::Init(): init m_pRun...\n");
	m_pRun = new CButton( m_pP1, ID_BTN_RUN, g_cfg.m_main_buttons_colour, g_cfg.m_main_buttons_colour_focused, g_cfg.m_main_buttons_colour_selected, g_cfg.m_main_buttons_border_colour,
		g_cfg.m_button_run_search_text, rcRun.GetPosition(), rcRun.GetSize() );
		
	m_plblBT1->SetBackgroundColour(g_cfg.m_main_labels_background_colour);
	m_plblBT2->SetBackgroundColour(g_cfg.m_main_labels_background_colour);
	m_plblBTA1->SetBackgroundColour( g_cfg.m_main_text_ctls_background_colour );
	m_plblBTA2->SetBackgroundColour( g_cfg.m_main_text_ctls_background_colour );

	m_plblBT1->SetFont(m_pMF->m_LBLFont);
	m_plblBT2->SetFont(m_pMF->m_LBLFont);
	m_plblBTA1->SetFont(m_pMF->m_LBLFont);
	m_plblBTA2->SetFont(m_pMF->m_LBLFont);
	m_pClear->SetFont(m_pMF->m_BTNFont);
	m_pRun->SetFont(m_pMF->m_BTNFont);

	m_plblBT1->SetTextColour(g_cfg.m_main_text_colour);
	m_plblBT2->SetTextColour(g_cfg.m_main_text_colour);
	m_plblBTA1->SetTextColour(g_cfg.m_main_text_colour);
	m_plblBTA2->SetTextColour(g_cfg.m_main_text_colour);
	m_pClear->SetTextColour(g_cfg.m_main_text_colour);
	m_pRun->SetTextColour(g_cfg.m_main_text_colour);

	wxBoxSizer *top_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

	button_sizer->Add(m_pP1, 1, wxALIGN_CENTER, 0 );

	top_sizer->Add(button_sizer, 1, wxALIGN_CENTER );

	this->SetSizer(top_sizer);

	SaveToReportLog("CSearchPanel::Init(): finished.\n");
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

		m_pMF->m_pVideoBox->m_pButtonPause->Disable();
		m_pMF->m_pVideoBox->m_pButtonRun->Disable();
		m_pMF->m_pVideoBox->m_pButtonStop->Disable();

		m_pMF->m_pPanel->m_pSSPanel->Disable();
		m_pMF->m_pPanel->m_pOCRPanel->Disable();
		m_pMF->m_pImageBox->ClearScreen();

		m_pMF->m_BegTime = GetVideoTime(wxString(m_plblBTA1->GetValue()));
		m_pMF->m_EndTime = GetVideoTime(wxString(m_plblBTA2->GetValue()));

		if (m_pMF->m_pVideo->SetNullRender())
		{
			m_pClear->Disable();
			m_plblBTA1->SetEditable(false);
			m_plblBTA2->SetEditable(false);
			g_color_ranges = GetColorRanges(g_use_filter_color);
			g_outline_color_ranges = GetColorRanges(g_use_outline_filter_color);

			m_pMF->m_pVideo->SetVideoWindowSettins(m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

			m_pSearchThread = new ThreadSearchSubtitles(m_pMF);
			m_pSearchThread->Create();
			m_pSearchThread->Run();
		}
	}
	else
	{
		if (g_RunSubSearch == 1) 
		{
			m_pRun->Disable();

			m_pMF->m_timer.Stop();
			wxTimerEvent event;
			m_pMF->OnTimer(event);

			g_RunSubSearch = 0;
		}
	}
}

void CSearchPanel::OnBnClickedClear(wxCommandEvent& event)
{
	m_pMF->ClearDir(g_work_dir + "/RGBImages");
	m_pMF->ClearDir(g_work_dir + "/ISAImages");
	m_pMF->ClearDir(g_work_dir + "/ILAImages");
	m_pMF->ClearDir(g_work_dir + "/TXTImages");
	m_pMF->ClearDir(g_work_dir + "/TXTImagesJoined");
	m_pMF->ClearDir(g_work_dir + "/TestImages");
	m_pMF->ClearDir(g_work_dir + "/TXTResults");
}

ThreadSearchSubtitles::ThreadSearchSubtitles(CMainFrame *pMF, wxThreadKind kind)
        : wxThread(kind)
{
	g_pMF = pMF;
}

void ThreadSearchSubtitlesRun(wxThread *pThr);

void *ThreadSearchSubtitles::Entry()
{
	try
	{
		ThreadSearchSubtitlesRun(this);
	}
	catch (const exception& e)
	{
		g_pMF->SaveError(wxT("Got C++ Exception: got error in ThreadSearchSubtitles::Entry():ThreadSearchSubtitlesRun() ") + wxString(e.what()));
	}
	
	if (!(g_pMF->m_blnNoGUI))
	{
		SaveToReportLog("ThreadSearchSubtitles::Entry: wxPostEvent THREAD_SEARCH_SUBTITLES_END ...\n");
		wxCommandEvent event(THREAD_SEARCH_SUBTITLES_END); // No specific id
		wxPostEvent(g_pMF->m_pPanel->m_pSHPanel, event);
	}
	else
	{
		g_IsSearching = 0;
		g_RunSubSearch = 0;
	}

	return 0;
}

void ThreadSearchSubtitlesRun(wxThread *pThr)
{
	g_IsSearching = 1;

	try
	{
		g_pMF->m_BegTime = FastSearchSubtitles(pThr, g_pMF->m_pVideo, g_pMF->m_BegTime, g_pMF->m_EndTime);
	}
	catch (const exception& e)
	{
		g_pMF->SaveError(wxT("Got C++ Exception: got error in ThreadSearchSubtitlesRun: ") + wxString(e.what()));
	}
}

void CSearchPanel::ThreadSearchSubtitlesEnd(wxCommandEvent& event)
{
	if (g_IsClose == 1) 
	{
		g_IsSearching = 0;
		m_pMF->Close();
		return;
	}

	if (!(m_pMF->m_blnNoGUI))
	{
		if (g_RunSubSearch == 1)
		{
			m_pMF->m_timer.Stop();
			wxTimerEvent event;
			m_pMF->OnTimer(event);
		}
		else
		{
			wxCommandEvent  menu_event(wxEVT_COMMAND_MENU_SELECTED, ID_FILE_REOPENVIDEO);
			m_pMF->OnFileReOpenVideo(menu_event);
		}

		m_pMF->m_pPanel->m_pSHPanel->m_pRun->SetLabel("Run Search");

		m_pMF->m_pPanel->m_pSSPanel->Enable();
		m_pMF->m_pPanel->m_pOCRPanel->Enable();

		if ((g_RunSubSearch == 1) && (g_CLEAN_RGB_IMAGES == true))
		{
			wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CCTI);
			m_pMF->m_pPanel->m_pOCRPanel->OnBnClickedCreateClearedTextImages(bn_event);
		}
		else if ((g_RunSubSearch == 1) && g_playback_sound)
		{
			SaveToReportLog("ThreadSearchSubtitlesEnd: trying to play sound ...\n");
			wxString Str = g_app_dir + wxT("/finished.wav");
			PlaySound(Str);
		}

		m_pClear->Enable();
		m_plblBTA1->SetEditable(true);
		m_plblBTA2->SetEditable(true);
		m_pRun->Enable();
	}

	g_IsSearching = 0;
	g_RunSubSearch = 0;

	return;
}

