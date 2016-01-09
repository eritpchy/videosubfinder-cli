                              //MainFrm.cpp//                                
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

#include "MainFrm.h"

CMainFrame *g_pMF;

const DWORD _MMX_FEATURE_BIT = 0x00800000;
const DWORD _SSE2_FEATURE_BIT = 0x04000000;

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInImageBox(int *Im, int w, int h)
{
	g_pMF->m_pImageBox->ViewImage(Im, w, h);	
}

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInVideoBox(int *Im, int w, int h)
{
	g_pMF->m_pVideoBox->ViewImage(Im, w, h);	
}

/////////////////////////////////////////////////////////////////////////////

void ViewRGBImage(int *Im, int w, int h)
{
	g_pMF->m_pImageBox->ViewRGBImage(Im, w, h);	
}

/////////////////////////////////////////////////////////////////////////////

static bool _IsFeature(DWORD dwRequestFeature)
{
	// This	bit	flag can get set on	calling	cpuid
	// with	register eax set to	1
	DWORD dwFeature	= 0;
	__try {
			_asm {
				mov	eax,1
				cpuid
				mov	dwFeature,edx
			}
	} __except ( EXCEPTION_EXECUTE_HANDLER)	{
			return false;
	}
	if ((dwRequestFeature == _MMX_FEATURE_BIT) &&
		(dwFeature & _MMX_FEATURE_BIT)) {
		__try {
			__asm {
				pxor mm0, mm0
				pmaxsw mm0, mm0
				emms
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return (0);
		}
		return(true);
	}
	else if ((dwRequestFeature == _SSE2_FEATURE_BIT) &&
		(dwFeature & _SSE2_FEATURE_BIT)) {
		__try {
			__asm {
				xorpd xmm0, xmm0
			}
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			return (0);
		}
		return(true);
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////

bool IsMMX_and_SSE()
{
	static bool bMMX = _IsFeature(_MMX_FEATURE_BIT);
	return(bMMX);
}

/////////////////////////////////////////////////////////////////////////////

bool IsSSE2()
{
	static bool bSSE2 = _IsFeature(_SSE2_FEATURE_BIT);
	return(bSSE2);
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CMainFrame, wxMDIParentFrame)
	EVT_SIZE(CMainFrame::OnSize)   
	EVT_MENU(ID_PLAY_PAUSE, CMainFrame::OnPlayPause)
	EVT_MENU(ID_PLAY_STOP, CMainFrame::OnStop)
	EVT_MENU(ID_FILE_REOPENVIDEO, CMainFrame::OnFileReOpenVideo)
	EVT_MENU(ID_FILE_OPENVIDEONORMALLY, CMainFrame::OnFileOpenVideoNormally)
	EVT_MENU(ID_FILE_OPENVIDEOALLDEFAULT, CMainFrame::OnFileOpenVideoAllDefault)
	EVT_MENU(ID_FILE_OPENVIDEOHARD, CMainFrame::OnFileOpenVideoHard)
	EVT_MENU(ID_EDIT_SETBEGINTIME, CMainFrame::OnEditSetBeginTime)
	EVT_MENU(ID_EDIT_SETENDTIME, CMainFrame::OnEditSetEndTime)
	EVT_MENU(ID_FILE_SAVESETTINGS, CMainFrame::OnFileSaveSettings)
	EVT_MENU(ID_FILE_LOADSETTINGS, CMainFrame::OnFileLoadSettings)
	EVT_MENU(ID_FILE_SAVESETTINGSAS, CMainFrame::OnFileSaveSettingsAs)
	EVT_TIMER(TIMER_ID, CMainFrame::OnTimer)
	EVT_CLOSE(CMainFrame::OnClose) 
	EVT_MENU(ID_FILE_EXIT, CMainFrame::OnQuit)
	EVT_MENU(ID_FILE_OPENPREVIOUSVIDEO, CMainFrame::OnFileOpenPreviousVideo)
	EVT_MENU(ID_APP_ABOUT, CMainFrame::OnAppAbout)
	EVT_MENU(ID_SETPRIORITY_IDLE, CMainFrame::OnSetPriorityIdle)
	EVT_MENU(ID_SETPRIORITY_NORMAL, CMainFrame::OnSetPriorityNormal)
	EVT_MENU(ID_SETPRIORITY_BELOWNORMAL, CMainFrame::OnSetPriorityBelownormal)
	EVT_MENU(ID_SETPRIORITY_ABOVENORMAL, CMainFrame::OnSetPriorityAbovenormal)
	EVT_MENU(ID_SETPRIORITY_HIGH, CMainFrame::OnSetPriorityHigh)
END_EVENT_TABLE() 

/////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame(const wxString& title)
		: wxMDIParentFrame( NULL, wxID_ANY, title,
							wxDefaultPosition, wxDefaultSize,
							wxDEFAULT_FRAME_STYLE | wxFRAME_NO_WINDOW_MENU )
		, m_timer(this, TIMER_ID)
{
	wxString Str;

	m_WasInited = false;
	m_VIsOpen = false;

	m_pPanel = NULL;
	m_pVideoBox = NULL;
	m_pImageBox = NULL;
	m_pVideo = NULL;

	// set frame icon
	this->SetIcon(wxIcon("vsf_ico"));

	m_pVideo = GetDSVideoObject();

	Str = wxGetCwd();
	Str.Replace("\\", "/"); 
	m_Dir = Str;

	g_dir = m_Dir;
	m_pVideo->m_Dir = m_Dir;

	g_pMF = this;
	g_pViewImage[0] = ViewImageInVideoBox;
	g_pViewImage[1] = ViewImageInImageBox;
	g_pViewRGBImage = ViewRGBImage;

	m_blnReopenVideo = false;

	m_FileName = "";
	m_dt = 0;

	m_type = 0;
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::Init()
{
	int cnt;

	wxMenuBar *pMenuBar = new wxMenuBar;

	cnt = pMenuBar->GetMenuCount();

	wxMenu *pMenu5 = new wxMenu;
	pMenu5->Append(ID_SETPRIORITY_HIGH, _T("HIGH"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_ABOVENORMAL, _T("ABOVE NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_NORMAL, _T("NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_BELOWNORMAL, _T("BELOW NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_IDLE, _T("IDLE"), _T(""), wxITEM_CHECK);

	wxMenu *pMenu1 = new wxMenu;	
	pMenu1->Append(ID_FILE_OPENVIDEOALLDEFAULT, _T("Open Video All Default"));
	pMenu1->Append(ID_FILE_OPENVIDEONORMALLY, _T("Open Video Normally"));
	pMenu1->Append(ID_FILE_OPENVIDEOHARD, _T("Open Video Hard"));
	pMenu1->Append(ID_FILE_OPENPREVIOUSVIDEO, _T("Open Or Continue Previous Video"));
	pMenu1->AppendSeparator();
	pMenu1->AppendSubMenu( pMenu5, _T("Set Priority"));
	pMenu1->Append(ID_FILE_SAVESETTINGS, _T("Save Settings\tCtrl+S"));
	pMenu1->Append(ID_FILE_SAVESETTINGSAS, _T("Save Settings As..."));
	pMenu1->Append(ID_FILE_LOADSETTINGS, _T("Load Settings..."));
	pMenu1->AppendSeparator();
	pMenu1->Append(ID_FILE_EXIT, _T("E&xit"));
	pMenuBar->Append(pMenu1, _T("&File"));

	wxMenu *pMenu2 = new wxMenu;
	pMenu2->Append(ID_EDIT_SETBEGINTIME, _T("Set Begin Time\tCtrl+Z"));
	pMenu2->Append(ID_EDIT_SETENDTIME, _T("Set End Time\tCtrl+X"));
	pMenuBar->Append(pMenu2, _T("&Edit"));

	wxMenu *pMenu3 = new wxMenu;
	pMenu3->Append(ID_PLAY_PAUSE, _T("Play/Pause\tSpace"));
	pMenu3->Append(ID_PLAY_STOP, _T("Stop"));
	pMenuBar->Append(pMenu3, _T("&Play"));

	wxMenu *pMenu4 = new wxMenu;
	pMenu4->Append(ID_APP_ABOUT, _T("&About..."));
	pMenuBar->Append(pMenu4, _T("&Help"));

	cnt = pMenuBar->GetMenuCount();

	this->SetMenuBar(pMenuBar);

	m_SettingsFileName = m_Dir+string("/settings.cfg");
	LoadSettings(m_SettingsFileName);

	if (IsMMX_and_SSE() == true)
	{
		g_MMX_SSE = true;
	}
	else
	{
		g_MMX_SSE = false;
	}

	m_pPanel = new CSSOWnd(this);
	m_pPanel->Init();

	m_pImageBox = new CImageBox(this);
	m_pImageBox->Init();

	m_pImageBox->SetSize(508, 22, 408, 354);
	m_pImageBox->Show(true);

	m_pVideoBox = new CVideoBox(this);
	m_pVideoBox->Init();

	m_pVideoBox->SetSize(80, 22, 408, 404);
	m_pVideoBox->Show(true);

	this->SetSize(0, 0, 1024, 768-30);

	m_WasInited = true;
}

void CMainFrame::OnSize(wxSizeEvent& event)
{
	int w, h, ph;
    this->GetClientSize(&w, &h);

	ph = 250;

	m_pPanel->SetSize(0, h-ph, w, ph);

    GetClientWindow()->SetSize(0, 0, w, h-ph);
}

void CMainFrame::OnFileReOpenVideo(wxCommandEvent& event)
{
	m_blnReopenVideo = true;
	OnFileOpenVideo(m_type);
}

void CMainFrame::OnFileOpenVideoNormally(wxCommandEvent& event)
{
	OnFileOpenVideo(0);
}

void CMainFrame::OnFileOpenVideoAllDefault(wxCommandEvent& event)
{
	OnFileOpenVideo(1);
}

void CMainFrame::OnFileOpenVideoHard(wxCommandEvent& event)
{
	OnFileOpenVideo(2);
}

void CMainFrame::OnFileOpenVideo(int type)
{
	string csFileName;
	s64 Cur;
	bool was_open_before = false;
	int i;

	if ( m_timer.IsRunning() ) 
	{
		m_timer.Stop();
		was_open_before = true;
	}

	m_type = type;
	csFileName = m_FileName;

	if (m_blnReopenVideo == false)
	{
		wxFileDialog fd(this, _T("Open Video File"),
						wxEmptyString, wxEmptyString, wxT("Video Files (*.avi;*.mp4;*.mpg;*.mpeg;*.mpv;*.m1v;*.dat;*.avs;*.vdr;*.asf;*.asx;*.wmv;*.mkv;*.ogm)|*.avi;*.mp4;*.mpg;*.mpeg;*.mpv;*.m1v;*.dat;*.avs;*.vdr;*.asf;*.asx;*.wmv;*.mkv;*.ogm|All Files (*.*)|*.*"), wxFD_OPEN);

		if(fd.ShowModal() != wxID_OK)
		{
			if (was_open_before) 
			{
				m_ct = -1;
				m_timer.Start(100);
			}

			return;
		}

		csFileName = fd.GetPath();
	}

	m_FileName = csFileName;

	WXHWND hWnd = m_pVideoBox->m_pVBox->m_pVideoWnd->GetHandle();

	this->Disable();

	if (type == 0)
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovieNormally(m_FileName, (void*)&hWnd);
	}
	else if (type == 1)
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovieAllDefault(m_FileName, (void*)&hWnd);
	}
	else if (type == 2)
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovieHard(m_FileName, (void*)&hWnd);
	}

	fstream fout;
	string rpl_path = m_Dir+string("/report.log");
	fout.open(rpl_path.c_str(), ios::out);
	fout <<	m_pVideo->m_log;
	fout.close();

	if (m_blnOpenVideoResult == false) 
	{
		m_VIsOpen = false;
		m_pVideoBox->m_plblVB->SetLabel("VideoBox");
		m_FileName = "";
		m_blnReopenVideo = false;

		this->Enable();

		return;
	}

	InitIPData((int)m_pVideo->m_Width, (int)m_pVideo->m_Height, 1);

	m_pVideoBox->m_pSB->SetScrollPos(0);
	m_pVideoBox->m_pSB->SetScrollRange(0, (int)(m_pVideo->m_Duration/(s64)10000));

	i=csFileName.size()-1;
	while (csFileName[i] != '\\') i--;

	m_pVideoBox->m_plblVB->SetLabel("VideoBox \""+csFileName.substr(i+1, csFileName.size()-i-1)+"\"");

	if (m_blnReopenVideo == false) 
	{
		m_BegTime = 0;
		m_EndTime = m_pVideo->m_Duration;
	}

	m_pPanel->m_pSHPanel->m_plblBTA1->SetLabel(ConvertVideoTime(m_BegTime));
	m_pPanel->m_pSHPanel->m_plblBTA2->SetLabel(ConvertVideoTime(m_EndTime));

	m_w = m_pVideo->m_Width;
	m_h = m_pVideo->m_Height;
	m_BufferSize = m_w*m_h*sizeof(int);
	m_VIsOpen = true;

	wxRect rc, rcP, rcVB, rVB, rcVW, rVW, rcIW, rImB;
	int w, wmax, h, ww, hh, dw, dh, dwi, dhi;

	this->GetClientSize(&w, &h);
	rc.x = rc.y = 0; 
	rc.width = w;
	rc.height = h;

	rcP = m_pPanel->GetRect();

	m_pVideoBox->GetClientSize(&w, &h);
	rcVB.x = rcVB.y = 0; 
	rcVB.width = w;
	rcVB.height = h;

	rVB = m_pVideoBox->GetRect();

	m_pVideoBox->m_pVBox->m_pVideoWnd->GetClientSize(&w, &h);
	rcVW.x = rcVW.y = 0; 
	rcVW.width = w;
	rcVW.height = h;

	rVW = m_pVideoBox->m_pVBox->m_pVideoWnd->GetRect();
	
	dw = rVB.width-rcVW.width;
	dh = rVB.height-rcVW.height;

	ww = (int)((double)rc.width*0.49);
	hh = (int)((double)rcP.y*0.98);

	wmax = ((hh-dh)*m_w)/m_h;
	if ( wmax > ww-dw ) wmax = ww-dw;
	
	w = wmax;
	h = ((w*m_h)/m_w);

	int x = ((rc.width/2-(w+dw))*3)/4, y = 5;
	m_pVideoBox->SetSize(x, y, (w+dw+10), (h+dh+10));
	m_pVideoBox->SetSize(x, y, (w+dw), (h+dh));

	m_pImageBox->m_pIW->GetClientSize(&w, &h);
	rcIW.x = rcIW.y = 0;
	rcIW.width = w;
	rcIW.height = h;

	rImB = m_pImageBox->GetRect();
	
	dwi = rImB.width - rcIW.width;
	dhi = rImB.height - rcIW.height;

	w = wmax;
	h = ((w*m_h)/m_w);

	x = rc.width/2+(rc.width/2-(w+dwi))/4;
	y = 5;	

	m_pImageBox->SetSize(x, y, w+dwi, h+dhi);	

	this->Update();

	m_pVideo->SetImageGeted(false);
	m_pVideo->RunWithTimeout(100);
	m_pVideo->Pause();
	
	m_dt = Cur = m_pVideo->GetPos();

	m_pVideo->SetImageGeted(false);
	m_pVideo->RunWithTimeout(100);
	m_pVideo->Pause();

	Cur = m_pVideo->GetPos();
	m_dt = Cur-m_dt;

	if (m_dt == 0)
	{
		m_dt = Cur;

		m_pVideo->SetImageGeted(false);
		m_pVideo->RunWithTimeout(100);
		m_pVideo->Pause();

		Cur = m_pVideo->GetPos();
		m_dt = Cur-m_dt;
	}

	if ((m_dt >= 10000000/10) || (m_dt <= 0)) m_dt = 10000000/25;

	Cur = m_BegTime;
	m_pVideo->SetPos(Cur);
	
	m_pVideo->SetImageGeted(false);
	m_pVideo->RunWithTimeout(100);
	m_pVideo->Pause();

	m_vs = Pause;
	m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, false);
	m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, true);
	m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, false);

	//m_pPanel->m_pSSPanel->OnBnClickedTest();

	m_EndTimeStr = string("/") + ConvertVideoTime(m_pVideo->m_Duration);

	if ( !m_timer.IsRunning() ) 
	{
		m_ct = -1;

		wxTimerEvent event;
		CMainFrame::OnTimer(event);
		
		m_timer.Start(100);
	}

	if (m_blnReopenVideo == false)
	{
		Cur = m_dt;
		m_pVideo->SetPos(Cur);
	}

	m_blnReopenVideo = false;	

	this->Enable();
}

void CMainFrame::OnPlayPause(wxCommandEvent& event)
{
	if (m_VIsOpen)	
	{
		if (m_vs == Play)
		{
			m_pVideo->Pause();
			m_vs = Pause;

			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, false);
			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, true);
			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, false);
		}
		else
		{
			m_pVideo->Run();
			m_vs = Play;

			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, true);
			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, false);
			m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, false);
		}
	}
}

void CMainFrame::OnStop(wxCommandEvent& event)
{
	if (m_VIsOpen)	
	{
		m_pVideo->StopFast();
		m_vs = Stop;
		
		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, false);
		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, true);
	}
}

void CMainFrame::PauseVideo()
{
	if (m_VIsOpen && (m_vs != Pause))	
	{
		m_pVideo->Pause();
		m_vs = Pause;

		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_RUN, false);
		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_PAUSE, true);
		m_pVideoBox->m_pVBar->ToggleTool(ID_TB_STOP, false);
	}
}

void CMainFrame::LoadSettings(string fname)
{
	ifstream fin;

	fin.open(fname.c_str(), ios::in);
	
	ReadProperty(fin, g_mthr, "moderate_threshold");
	ReadProperty(fin, g_mvthr, "moderate_threshold_for_VEdges");
	ReadProperty(fin, g_mhthr, "moderate_threshold_for_HEdges");
	ReadProperty(fin, g_mnthr, "moderate_threshold_for_NEdges");
	ReadProperty(fin, g_segw, "segment_width");
	ReadProperty(fin, g_segh, "segment_height");
	ReadProperty(fin, g_msegc, "minimum_segments_count");
	ReadProperty(fin, g_scd, "min_sum_color_diff");
	ReadProperty(fin, g_smcd, "min_sum_multiple_color_diff");
	ReadProperty(fin, g_btd, "between_text_distace");
	ReadProperty(fin, g_tco, "text_centre_offset");
	ReadProperty(fin, g_tcpo, "text_centre_percent_offset");

	ReadProperty(fin, g_mpn, "min_points_number");
	ReadProperty(fin, g_mpd, "min_points_density");
	ReadProperty(fin, g_mpvd, "min_VEdges_points_density_(per_full_line)");
	ReadProperty(fin, g_mphd, "min_HEdges_points_density_(per_full_line)");
	ReadProperty(fin, g_mpnd, "min_NEdges_points_density_(per_full_line)");
	ReadProperty(fin, g_mpved, "min_VEdges_points_density");
	ReadProperty(fin, g_mpned, "min_NEdges_points_density");

	ReadProperty(fin, g_DL, "sub_frame_length");
	ReadProperty(fin, g_tp, "text_procent");
	ReadProperty(fin, g_mtpl, "min_text_len_(in_procent)");
	ReadProperty(fin, g_sse, "sub_square_error");
	ReadProperty(fin, g_veple, "vedges_points_line_error");

	ReadProperty(fin, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	ReadProperty(fin, g_DefStringForEmptySub, "def_string_for_empty_sub");
	
	fin.close();

	//m_pPanel->m_pSSPanel->Refresh();
}

void CMainFrame::SaveSettings(string fname)
{
	ofstream fout;

	fout.open(fname.c_str(), ios::out);

	WriteProperty(fout, g_mthr, "moderate_threshold");
	WriteProperty(fout, g_mvthr, "moderate_threshold_for_VEdges");
	WriteProperty(fout, g_mhthr, "moderate_threshold_for_HEdges");
	WriteProperty(fout, g_mnthr, "moderate_threshold_for_NEdges");
	WriteProperty(fout, g_segw, "segment_width");
	WriteProperty(fout, g_segh, "segment_height");
	WriteProperty(fout, g_msegc, "minimum_segments_count");
	WriteProperty(fout, g_scd, "min_sum_color_diff");
	WriteProperty(fout, g_smcd, "min_sum_multiple_color_diff");
	WriteProperty(fout, g_btd, "between_text_distace");
	WriteProperty(fout, g_tco, "text_centre_offset");
	WriteProperty(fout, g_tcpo, "text_centre_percent_offset");

	WriteProperty(fout, g_mpn, "min_points_number");
	WriteProperty(fout, g_mpd, "min_points_density");
	WriteProperty(fout, g_mpvd, "min_VEdges_points_density_(per_full_line)");
	WriteProperty(fout, g_mphd, "min_HEdges_points_density_(per_full_line)");
	WriteProperty(fout, g_mpnd, "min_NEdges_points_density_(per_full_line)");
	WriteProperty(fout, g_mpved, "min_VEdges_points_density");
	WriteProperty(fout, g_mpned, "min_NEdges_points_density");

	WriteProperty(fout, g_DL, "sub_frame_length");
	WriteProperty(fout, g_tp, "text_procent");
	WriteProperty(fout, g_mtpl, "min_text_len_(in_procent)");
	WriteProperty(fout, g_sse, "sub_square_error");
	WriteProperty(fout, g_veple, "vedges_points_line_error");

	WriteProperty(fout, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	WriteProperty(fout, g_DefStringForEmptySub, "def_string_for_empty_sub");

	fout.close();
}

void CMainFrame::OnEditSetBeginTime(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		s64 Cur;
	
		Cur = m_pVideo->GetPos();

		m_pPanel->m_pSHPanel->m_plblBTA1->SetLabel(ConvertVideoTime(Cur));

		m_BegTime = Cur;
	}
}

void CMainFrame::OnEditSetEndTime(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		s64 Cur;
	
		Cur = m_pVideo->GetPos();

		m_pPanel->m_pSHPanel->m_plblBTA2->SetLabel(ConvertVideoTime(Cur));

		m_EndTime = Cur;
	}
}

void CMainFrame::OnFileLoadSettings(wxCommandEvent& event)
{
	if (m_blnReopenVideo == false)
	{
		wxFileDialog fd(this, _T("Open Settings File"),
						m_Dir, wxEmptyString, wxT("*.cfg"), wxFD_OPEN);

		if(fd.ShowModal() != wxID_OK)
		{
			return;
		}

		m_SettingsFileName = fd.GetPath();
	}

	LoadSettings(m_SettingsFileName);
}

void CMainFrame::OnFileSaveSettingsAs(wxCommandEvent& event)
{
	wxFileDialog fd(this, _T("Save Settings File"),
						m_Dir, wxEmptyString, wxT("Settings Files (*.cfg)|*.cfg"), wxFD_SAVE);

	if (m_blnReopenVideo == false)
	{
		if(fd.ShowModal() != wxID_OK)
		{
			return;
		}

		m_SettingsFileName = fd.GetPath();
	}

	SaveSettings(m_SettingsFileName);
}

void CMainFrame::OnFileSaveSettings(wxCommandEvent& event)
{
	SaveSettings(m_SettingsFileName);
}

void CMainFrame::OnTimer(wxTimerEvent& event)
{
	s64 Cur;
	
	Cur = m_pVideo->GetPos();

	if (Cur != m_ct) 
	{
		m_pVideoBox->m_plblTIME->SetLabel(ConvertVideoTime(Cur) + m_EndTimeStr);
		m_ct = Cur;
	}

	m_pVideoBox->m_pSB->SetScrollPos((int)(Cur/(s64)10000));

	//m_pVideoBox->m_pVBox->m_pHSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pHSL2->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL2->Refresh(true);
}

string VideoTimeToStr2(s64 pos)
{
	string Str;
	static char str[100];
	int hour, min, sec, sec_1000, vl;
	
	vl = (int)(pos/10000000);
	hour = vl/3600;
	vl -= hour*3600;
	min = vl/60;
	vl -= min*60;
	sec = vl;
	
	itoa(hour,str,10);
	Str += string("0")+string(str)+string(":");
	
	itoa(min,str,10);
	if (min<=9)
	{
		Str += string("0")+string(str)+string(":");
	}
	else Str += string(str)+string(":");

	itoa(sec,str,10);
	if (sec<=9)
	{
		Str += string("0")+string(str)+string(",");
	}
	else Str += string(str)+string(",");

	sec_1000 = (int)((pos%10000000)/10000);
	itoa(sec_1000,str,10);
	if (sec_1000<=9)
	{
		Str += string("00")+string(str);
	}
	else 
	{
		if (sec_1000<=99)
		{
			Str += string("0")+string(str);
		}
		else Str += string(str);
	}

	return Str;
}

string VideoTimeToStr3(s64 pos)
{
	static char str[100];
	int hour, min, sec, sec_100, vl;
	
	vl = (int)(pos/10000000);
	hour = vl/3600;
	vl -= hour*3600;
	min = vl/60;
	vl -= min*60;
	sec = vl;

	sec_100 = (int)((pos%10000000)/10000)/10;

	sprintf(str, "%.1d:%.2d:%.2d.%.2d", hour, min, sec, sec_100);

	return string(str);
}

s64 GetVideoTime(int minute, int sec, int mili_sec)
{
	s64 res;
	res = (s64)((minute*60+sec)*1000+mili_sec)*(s64)10000;
	return res;
}

string ConvertVideoTime(s64 pos)
{
	string Str;
	static char str[100];
	int hour, min, sec, sec_1000, vl;
	
	vl = (int)(pos/10000000);
	hour = vl/3600;
	vl -= hour*3600;
	min = vl/60;
	vl -= min*60;
	sec = vl;
	
	itoa(hour,str,10);
	if (hour<10) Str = "0";
	Str += string(str)+":";
	
	itoa(min,str,10);
	if (min<10)	Str += "0";
	Str += string(str)+":";

	itoa(sec,str,10);
	if (sec<10) Str += "0";
	Str += string(str)+",";

	sec_1000 = (int)((pos%10000000)/10000);
	itoa(sec_1000,str,10);
	if (sec_1000<100) Str += "0";
	if (sec_1000<10) Str += "0";
	Str += string(str);

	return Str;
}

void CMainFrame::OnQuit(wxCommandEvent& event)
{
	Close(true);
}

void CMainFrame::OnClose(wxCloseEvent& WXUNUSED(event))
{
	if ( m_timer.IsRunning() ) 
	{
		m_timer.Stop();
	}

	if (g_IsCreateClearedTextImages == 1)
	{
		g_RunCreateClearedTextImages = 0;
		//SetThreadPriority(m_pPanel->m_OCRPanel.m_hSearchThread, THREAD_PRIORITY_HIGHEST);
	}

	if ( (g_IsSearching == 0) && (m_FileName != string("")) )
	{
		fstream fout;
		string pvi_path = m_Dir+string("/previous_video.inf");

		fout.open(pvi_path.c_str(), ios::out);

		fout <<	m_FileName << '\n';

		fout <<	m_BegTime << '\n';

		fout <<	m_EndTime << '\n';

		fout <<	m_type << '\n';

		fout.close();
	}

	if (g_IsSearching == 1)
	{
		g_IsClose = 1;
		g_RunSubSearch = 0;
		m_pPanel->m_pSHPanel->m_pSearchThread->SetPriority(90); //THREAD_PRIORITY_HIGHEST
	}

	clock_t start_t = clock();

	while( ((clock() - start_t) < 2000) && ( (g_IsSearching == 1) || (g_IsCreateClearedTextImages == 1) ) ){}

	if (g_IsSearching == 1)
	{
		m_pPanel->m_pSHPanel->m_pSearchThread->Delete();
	}

	if (g_IsCreateClearedTextImages == 1)
	{
		//TerminateThread(m_pPanel->m_OCRPanel.m_hSearchThread, 0);
	}

	ReleaseIPData();

	Destroy();
}

void CMainFrame::OnFileOpenPreviousVideo(wxCommandEvent& event)
{
	char str[300];
	fstream fin;
	string pvi_path = m_Dir+string("/previous_video.inf");

	fin.open(pvi_path.c_str(), ios::in);
	
	fin.getline(str, 300);
	m_FileName = string(str);

	fin.getline(str, 300);
	m_BegTime = (s64)strtod(str, NULL);

	fin.getline(str, 300);
	m_EndTime = (s64)strtod(str, NULL);

	fin.getline(str, 300);
	m_type = (int)strtod(str, NULL);

	fin.close();

	m_blnReopenVideo = true;

	OnFileOpenVideo(m_type);
}

void CMainFrame::ClearDir(string DirName)
{
	wxString dir_path = wxString(m_Dir + string("/") + DirName + string("/"));
	wxDir dir(dir_path);
	vector<wxString> FileNamesVector;
	wxString filename;
	bool res;

	res = dir.GetFirst(&filename);
    while ( res )
    {
        if ( (filename != wxString(".")) && 
			 (filename != wxString("..")) )
		{
			FileNamesVector.push_back(filename);
		}

        res = dir.GetNext(&filename);
    }

	for(int i=0; i<(int)FileNamesVector.size(); i++)
	{		
		res = wxRemoveFile(dir_path + FileNamesVector[i]);
	}
	
	FileNamesVector.clear();
}

void CMainFrame::OnAppAbout(wxCommandEvent& event)
{
	(void)wxMessageBox("This program was developed and \nimplemented by Simeon Kosnitsky. \nPublished under GPL license.", "VideoSubFinder Version 1.80 beta");
}

void CMainFrame::OnSetPriorityIdle(wxCommandEvent& event)
{
	wxMenuBar* pMenuBar = this->GetMenuBar();

	pMenuBar->Check(ID_SETPRIORITY_IDLE, true);
	pMenuBar->Check(ID_SETPRIORITY_BELOWNORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_NORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_ABOVENORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_HIGH, false);

#ifdef WIN32
	HANDLE m_hCurrentProcess = GetCurrentProcess();
	BOOL res = SetPriorityClass(m_hCurrentProcess, IDLE_PRIORITY_CLASS);
#endif
}

void CMainFrame::OnSetPriorityBelownormal(wxCommandEvent& event)
{
	wxMenuBar* pMenuBar = this->GetMenuBar();

	pMenuBar->Check(ID_SETPRIORITY_IDLE, false);
	pMenuBar->Check(ID_SETPRIORITY_BELOWNORMAL, true);
	pMenuBar->Check(ID_SETPRIORITY_NORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_ABOVENORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_HIGH, false);

#ifdef WIN32
	HANDLE m_hCurrentProcess = GetCurrentProcess();
	BOOL res = SetPriorityClass(m_hCurrentProcess, BELOW_NORMAL_PRIORITY_CLASS);
#endif
}

void CMainFrame::OnSetPriorityNormal(wxCommandEvent& event)
{
	wxMenuBar* pMenuBar = this->GetMenuBar();

	pMenuBar->Check(ID_SETPRIORITY_IDLE, false);
	pMenuBar->Check(ID_SETPRIORITY_BELOWNORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_NORMAL, true);
	pMenuBar->Check(ID_SETPRIORITY_ABOVENORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_HIGH, false);

#ifdef WIN32
	HANDLE m_hCurrentProcess = GetCurrentProcess();
	BOOL res = SetPriorityClass(m_hCurrentProcess, NORMAL_PRIORITY_CLASS);
#endif
}

void CMainFrame::OnSetPriorityAbovenormal(wxCommandEvent& event)
{
	wxMenuBar* pMenuBar = this->GetMenuBar();

	pMenuBar->Check(ID_SETPRIORITY_IDLE, false);
	pMenuBar->Check(ID_SETPRIORITY_BELOWNORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_NORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_ABOVENORMAL, true);
	pMenuBar->Check(ID_SETPRIORITY_HIGH, false);

#ifdef WIN32
	HANDLE m_hCurrentProcess = GetCurrentProcess();
	BOOL res = SetPriorityClass(m_hCurrentProcess, ABOVE_NORMAL_PRIORITY_CLASS);
#endif
}

void CMainFrame::OnSetPriorityHigh(wxCommandEvent& event)
{
	wxMenuBar* pMenuBar = this->GetMenuBar();

	pMenuBar->Check(ID_SETPRIORITY_IDLE, false);
	pMenuBar->Check(ID_SETPRIORITY_BELOWNORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_NORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_ABOVENORMAL, false);
	pMenuBar->Check(ID_SETPRIORITY_HIGH, true);

#ifdef WIN32
	HANDLE m_hCurrentProcess = GetCurrentProcess();
	BOOL res = SetPriorityClass(m_hCurrentProcess, HIGH_PRIORITY_CLASS);
#endif
}

void LoadToolBarImage(wxBitmap& bmp, const wxString& path, const wxColor& BColor)
{
	bmp = wxBitmap(wxImage(path));
	const wxColor ColorDef(192, 192, 192);

    if ( bmp.IsOk() )
    {
		int w = bmp.GetWidth();
		int h = bmp.GetHeight();
		wxColor Color;

		wxMemoryDC dc;
        dc.SelectObject(bmp);
        dc.SetPen(wxPen(BColor));

		for (int y=0; y < h; y++)
		for (int x=0; x < w; x++)
		{
			dc.GetPixel(x, y, &Color);

			if (Color == ColorDef)
			{
				dc.DrawPoint(x, y);
			}
		}
    }
}

void WriteProperty(ofstream &fout, int val, string Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(ofstream &fout, bool val, string Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(ofstream &fout, double val, string Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(ofstream &fout, wxString val, string Name)
{
	fout << Name << " = " << val << '\n';
}

void ReadProperty(ifstream &fin, int &val, string Name)
{
	char name[100], str[100];

	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i<(100-1); i++)
		{
			str[i] = str[i+1];
		}
	} while((Name != string(name)) && !fin.eof());
	
	if (!fin.eof()) 
	{
		val = (int)strtod(str, NULL);
	}
}

void ReadProperty(ifstream &fin, bool &val, string Name)
{
	char name[100], str[100];

	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i<(100-1); i++)
		{
			str[i] = str[i+1];
		}
	} while((Name != string(name)) && !fin.eof());
	
	if (!fin.eof()) 
	{
		int get_val = (int)strtod(str, NULL);

		if (get_val != 0)
		{
			val = true;
		}
		else
		{
			val = false;
		}
	}
}

void ReadProperty(ifstream &fin, double &val, string Name)
{
	char name[100], str[100];

	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';
		
		fin.getline(str, 100);
		for(int i=0; i<(100-1); i++)
		{
			str[i] = str[i+1];
		}
	} while((Name != string(name)) && !fin.eof());
	
	if (!fin.eof()) 
	{
		val = strtod(str, NULL);
	}
}

void ReadProperty(ifstream &fin, wxString &val, string Name)
{
	char name[100], str[100];

	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i<(100-1); i++)
		{
			str[i] = str[i+1];
		}
	} while((Name != string(name)) && !fin.eof());
	
	if (!fin.eof()) 
	{
		val = wxString(str);
	}
}
