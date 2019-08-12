                              //MainFrm.cpp//                                
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

#include "MainFrm.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>

CMainFrame *g_pMF;

const DWORD _MMX_FEATURE_BIT = 0x00800000;
const DWORD _SSE2_FEATURE_BIT = 0x04000000;

/////////////////////////////////////////////////////////////////////////////

int exception_filter(unsigned int code, struct _EXCEPTION_POINTERS *ep, char *det)
{
	g_pMF->SaveError(string("Got C Exception: ") + det);
	return EXCEPTION_EXECUTE_HANDLER;
}

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInImageBox(custom_buffer<int> &Im, int w, int h)
{
	if (!(g_pMF->m_blnNoGUI)) g_pMF->m_pImageBox->ViewImage(Im, w, h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInVideoBox(custom_buffer<int> &Im, int w, int h)
{
	if (!(g_pMF->m_blnNoGUI)) g_pMF->m_pVideoBox->ViewImage(Im, w, h);
}

/////////////////////////////////////////////////////////////////////////////

void ViewRGBImage(custom_buffer<int> &Im, int w, int h)
{
	if (!(g_pMF->m_blnNoGUI)) g_pMF->m_pImageBox->ViewRGBImage(Im, w, h);
}

/////////////////////////////////////////////////////////////////////////////

static bool _IsFeature(DWORD dwRequestFeature)
{
#ifndef WIN64
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
#endif

	return false;
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CMainFrame, wxMDIParentFrame)
	EVT_SIZE(CMainFrame::OnSize)   
	EVT_MENU(ID_PLAY_PAUSE, CMainFrame::OnPlayPause)
	EVT_MENU(ID_PLAY_STOP, CMainFrame::OnStop)
	EVT_MENU(ID_FILE_REOPENVIDEO, CMainFrame::OnFileReOpenVideo)
	EVT_MENU(ID_FILE_OPEN_VIDEO_OPENCV, CMainFrame::OnFileOpenVideoOpenCV)
	EVT_MENU(ID_FILE_OPEN_VIDEO_FFMPEG, CMainFrame::OnFileOpenVideoFFMPEG)	
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
	
	Str = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
	Str.Replace("\\", "/");
	g_app_dir = Str.ToStdString();
	g_work_dir = g_app_dir;

	g_pV = NULL;

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

	m_blnNoGUI = false;

	wxMenuBar *pMenuBar = new wxMenuBar;

	m_GeneralSettingsFileName = g_app_dir + string("/settings/general.cfg");

	m_ErrorFileName = g_work_dir + string("/error.log");

	LoadSettings();

	if (!InitCUDADevice())
	{
		g_use_cuda_gpu = false;
	}

	cnt = pMenuBar->GetMenuCount();

	wxMenu *pMenu5 = new wxMenu;
	pMenu5->Append(ID_SETPRIORITY_HIGH, _T("HIGH"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_ABOVENORMAL, _T("ABOVE NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_NORMAL, _T("NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_BELOWNORMAL, _T("BELOW NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_IDLE, _T("IDLE"), _T(""), wxITEM_CHECK);

	wxMenu *pMenu1 = new wxMenu;	
	pMenu1->Append(ID_FILE_OPEN_VIDEO_OPENCV, _T("Open Video (OpenCV)"));
	pMenu1->Append(ID_FILE_OPEN_VIDEO_FFMPEG, _T("Open Video (FFMPEG with GPU Acceleration)"));
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

	m_pPanel = new CSSOWnd(this);
	m_pPanel->Init();

	this->SetMenuBar(pMenuBar);

	m_pImageBox = new CImageBox(this);
	m_pImageBox->Init();

	int w = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	int h = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
	int dx = 20, dy = 20;
	m_ph = 250;
	
	m_pVideoBox = new CVideoBox(this);
	m_pVideoBox->Init();

	this->SetSize(0, 0, w, h - 50);

	int cw, ch;
	this->GetClientSize(&cw, &ch);

	m_pImageBox->SetSize(cw / 2 + dx, dy, cw / 2 - 2 * dx, ch - m_ph - 2 * dy);
	m_pImageBox->Show(true);

	m_pVideoBox->SetSize(dx, dy, cw / 2 - 2 * dx, ch - m_ph - 2 * dy);
	m_pVideoBox->Show(true);

	m_WasInited = true;
}

void CMainFrame::OnSize(wxSizeEvent& event)
{
	int w, h;
    this->GetClientSize(&w, &h);
	m_cw = w;
	m_ch = h;

	m_pPanel->SetSize(0, h - m_ph, w, m_ph);

	GetClientWindow()->SetSize(0, 0, w, h - m_ph);
}

void CMainFrame::OnFileReOpenVideo(wxCommandEvent& event)
{
	m_blnReopenVideo = true;
	OnFileOpenVideo(m_type);
}

void CMainFrame::OnFileOpenVideoOpenCV(wxCommandEvent& event)
{
	OnFileOpenVideo(0);
}

void CMainFrame::OnFileOpenVideoFFMPEG(wxCommandEvent& event)
{
	OnFileOpenVideo(1);
}

void CMainFrame::OnFileOpenVideo(int type)
{
	wxString csFileName;
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

	if (m_type == 0) m_pVideo = GetOCVVideoObject();
	else if (m_type == 1) m_pVideo = GetFFMPEGVideoObject();
	else
	{
		(void)wxMessageBox("ERROR: Unknown video type for OnFileOpenVideo");
		return;
	}	

	m_pVideo->m_Dir = g_work_dir;

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

	this->Disable();

	if (m_type == 0)
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovie(m_FileName, (void*)m_pVideoBox->m_pVBox->m_pVideoWnd, 0);
	}
	else
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovie(m_FileName, (void*)m_pVideoBox->m_pVBox->m_pVideoWnd, 0);
	}

	fstream fout;
	string rpl_path = g_work_dir + string("/report.log");
	fout.open(rpl_path.c_str(), ios::out);
	fout <<	"Filename: " << m_FileName << "\n" << m_pVideo->m_log;
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

	m_pVideoBox->m_pSB->SetScrollPos(0);
	m_pVideoBox->m_pSB->SetScrollRange(0, (int)(m_pVideo->m_Duration));

	m_pVideoBox->m_plblVB->SetLabel("VideoBox \"" + GetFileName(csFileName) + "\"");

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

	rc.x = rc.y = 0; 
	rc.width = m_cw;
	rc.height = m_ch;

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

	if ((m_dt >= 500) || (m_dt <= 0)) m_dt = 1000.0/25.0;

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

void CMainFrame::LoadSettings()
{
	ifstream fin;

	fin.open(m_GeneralSettingsFileName.c_str(), ios::in);
	
	if (fin.bad())
	{
		(void)wxMessageBox("ERROR: Can't open settings file: " + m_GeneralSettingsFileName);
		exit(1);
	}
	
	
	ReadProperty(fin, g_DontDeleteUnrecognizedImages1, "dont_delete_unrecognized_images1");
	ReadProperty(fin, g_DontDeleteUnrecognizedImages2, "dont_delete_unrecognized_images2");

	ReadProperty(fin, g_generate_cleared_text_images_on_test, "generate_cleared_text_images_on_test");
	ReadProperty(fin, g_show_results, "dump_debug_images");
	ReadProperty(fin, g_show_sf_results, "dump_debug_second_filtration_images");
	ReadProperty(fin, g_clear_test_images_folder, "clear_test_images_folder");
	ReadProperty(fin, g_show_transformed_images_only, "show_transformed_images_only");
	ReadProperty(fin, g_use_ocl, "use_ocl");
	ReadProperty(fin, g_use_cuda_gpu, "use_cuda_gpu");
	ReadProperty(fin, g_cuda_kmeans_initial_loop_iterations, "cuda_kmeans_initial_loop_iterations");
	ReadProperty(fin, g_cuda_kmeans_loop_iterations, "cuda_kmeans_loop_iterations");		
	ReadProperty(fin, g_cpu_kmeans_initial_loop_iterations, "cpu_kmeans_initial_loop_iterations");
	ReadProperty(fin, g_cpu_kmeans_loop_iterations, "cpu_kmeans_loop_iterations");	

	ReadProperty(fin, g_smthr, "moderate_threshold_for_scaled_image");
	ReadProperty(fin, g_mthr, "moderate_threshold");
	ReadProperty(fin, g_mnthr, "moderate_threshold_for_NEdges");
	ReadProperty(fin, g_segw, "segment_width");
	ReadProperty(fin, g_segh, "segment_height");
	ReadProperty(fin, g_msegc, "minimum_segments_count");
	ReadProperty(fin, g_scd, "min_sum_color_diff");
	ReadProperty(fin, g_btd, "between_text_distace");
	ReadProperty(fin, g_tco, "text_centre_offset");
	ReadProperty(fin, g_scale, "image_scale_for_clear_image");
	
	ReadProperty(fin, g_use_ISA_images_for_get_txt_area, "use_ISA_images");
	ReadProperty(fin, g_use_ILA_images_for_get_txt_area, "use_ILA_images");

	ReadProperty(fin, g_use_gradient_images_for_clear_txt_images, "use_gradient_images_for_clear_txt_images");
	ReadProperty(fin, g_clear_txt_images_by_main_color, "clear_txt_images_by_main_color");
	ReadProperty(fin, g_use_ILA_images_for_clear_txt_images, "use_ILA_images_for_clear_txt_images");	

	ReadProperty(fin, g_mpn, "min_points_number");
	ReadProperty(fin, g_mpd, "min_points_density");
	ReadProperty(fin, g_msh, "min_symbol_height");
	ReadProperty(fin, g_msd, "min_symbol_density");
	ReadProperty(fin, g_mpned, "min_NEdges_points_density");				

	ReadProperty(fin, g_clear_txt_folders, "clear_txt_folders");
	ReadProperty(fin, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	ReadProperty(fin, g_threads, "threads");
	ReadProperty(fin, g_ocr_threads, "ocr_threads");
	ReadProperty(fin, g_DL, "sub_frame_length");
	ReadProperty(fin, g_tp, "text_procent");
	ReadProperty(fin, g_mtpl, "min_text_len_(in_procent)");
	//ReadProperty(fin, g_sse, "sub_square_error");
	ReadProperty(fin, g_veple, "vedges_points_line_error");	
	
	ReadProperty(fin, g_clear_image_logical, "clear_image_logical");

	ReadProperty(fin, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	ReadProperty(fin, g_DefStringForEmptySub, "def_string_for_empty_sub");

	ReadProperty(fin, m_cfg.m_prefered_locale, "prefered_locale");

	ReadProperty(fin, m_cfg.m_ocr_min_sub_duration, "min_sub_duration");
	
	ReadProperty(fin, m_cfg.m_txt_dw, "txt_dw");
	ReadProperty(fin, m_cfg.m_txt_dy, "txt_dy");

	ReadProperty(fin, m_cfg.m_fount_size_ocr_lbl, "fount_size_ocr_lbl");
	ReadProperty(fin, m_cfg.m_fount_size_ocr_btn, "fount_size_ocr_btn");

	ReadProperty(fin, g_use_ISA_images_for_search_subtitles, "use_ISA_images_for_search_subtitles");
	ReadProperty(fin, g_use_ILA_images_for_search_subtitles, "use_ILA_images_for_search_subtitles");
	ReadProperty(fin, g_replace_ISA_by_filtered_version, "replace_ISA_by_filtered_version");
	ReadProperty(fin, g_max_dl_down, "max_dl_down");
	ReadProperty(fin, g_max_dl_up, "max_dl_up");

	ReadProperty(fin, g_remove_wide_symbols, "remove_wide_symbols");
	

	fin.close();

	fin.open((g_app_dir + string("/settings/") + string(m_cfg.m_prefered_locale.mb_str()) + string("/locale.cfg")).c_str(), ios::in);
	
	ReadProperty(fin, m_cfg.m_ocr_label_msd_text, "ocr_label_msd_text");
	ReadProperty(fin, m_cfg.m_ocr_label_jsact_text, "ocr_label_jsact_text");
	ReadProperty(fin, m_cfg.m_ocr_label_clear_txt_folders, "ocr_label_clear_txt_folders");
	ReadProperty(fin, m_cfg.m_ocr_button_ces_text, "ocr_button_ces_text");
	ReadProperty(fin, m_cfg.m_ocr_button_ccti_text, "ocr_button_ccti_text");
	ReadProperty(fin, m_cfg.m_ocr_button_csftr_text, "ocr_button_csftr_text");
	ReadProperty(fin, m_cfg.m_ocr_button_cesfcti_text, "ocr_button_cesfcti_text");
	ReadProperty(fin, m_cfg.m_ocr_button_test_text, "ocr_button_test_text");
	
	ReadProperty(fin, m_cfg.m_ssp_oi_property_use_ocl, "ssp_oi_property_use_ocl");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_use_cuda_gpu, "ssp_oi_property_use_cuda_gpu");
	ReadProperty(fin, m_cfg.m_ssp_ocr_threads, "ssp_ocr_threads");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_image_scale_for_clear_image, "ssp_oi_property_image_scale_for_clear_image");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, "ssp_oi_property_moderate_threshold_for_scaled_image");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, "ssp_oi_property_cuda_kmeans_initial_loop_iterations");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, "ssp_oi_property_cuda_kmeans_loop_iterations");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, "ssp_oi_property_cpu_kmeans_initial_loop_iterations");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, "ssp_oi_property_cpu_kmeans_loop_iterations");
	ReadProperty(fin, m_cfg.m_ssp_label_parameters_influencing_image_processing, "ssp_label_parameters_influencing_image_processing");
	ReadProperty(fin, m_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, "ssp_label_ocl_and_multiframe_image_stream_processing");
	ReadProperty(fin, m_cfg.m_ssp_oi_group_global_image_processing_settings, "ssp_oi_group_global_image_processing_settings");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, "ssp_oi_property_generate_cleared_text_images_on_test");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_dump_debug_images, "ssp_oi_property_dump_debug_images");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, "ssp_oi_property_dump_debug_second_filtration_images");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_clear_test_images_folder, "ssp_oi_property_clear_test_images_folder");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_show_transformed_images_only, "ssp_oi_property_show_transformed_images_only");
	ReadProperty(fin, m_cfg.m_ssp_oi_group_initial_image_processing, "ssp_oi_group_initial_image_processing");
	ReadProperty(fin, m_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, "ssp_oi_sub_group_settings_for_sobel_operators");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_moderate_threshold, "ssp_oi_property_moderate_threshold");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_moderate_nedges_threshold, "ssp_oi_property_moderate_nedges_threshold");
	ReadProperty(fin, m_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, "ssp_oi_sub_group_settings_for_color_filtering");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_segment_width, "ssp_oi_property_segment_width");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_segments_count, "ssp_oi_property_min_segments_count");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_sum_color_difference, "ssp_oi_property_min_sum_color_difference");
	ReadProperty(fin, m_cfg.m_ssp_oi_group_secondary_image_processing, "ssp_oi_group_secondary_image_processing");
	ReadProperty(fin, m_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, "ssp_oi_sub_group_settings_for_linear_filtering");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_line_height, "ssp_oi_property_line_height");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_max_between_text_distance, "ssp_oi_property_max_between_text_distance");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_max_text_center_offset, "ssp_oi_property_max_text_center_offset");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_max_text_center_percent_offset, "ssp_oi_property_max_text_center_percent_offset");
	ReadProperty(fin, m_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, "ssp_oi_sub_group_settings_for_color_border_points");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_points_number, "ssp_oi_property_min_points_number");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_points_density, "ssp_oi_property_min_points_density");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_symbol_height, "ssp_oi_property_min_symbol_height");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_symbol_density, "ssp_oi_property_min_symbol_density");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_vedges_points_density, "ssp_oi_property_min_vedges_points_density");
	ReadProperty(fin, m_cfg.m_ssp_oi_property_min_nedges_points_density, "ssp_oi_property_min_nedges_points_density");
	ReadProperty(fin, m_cfg.m_ssp_oi_group_tertiary_image_processing, "ssp_oi_group_tertiary_image_processing");
	ReadProperty(fin, m_cfg.m_ssp_oim_group_ocr_settings, "ssp_oim_group_ocr_settings");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_clear_images_logical, "ssp_oim_property_clear_images_logical");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, "ssp_oim_property_clear_rgbimages_after_search_subtitles");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_using_hard_algorithm_for_text_mining, "ssp_oim_property_using_hard_algorithm_for_text_mining");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, "ssp_oim_property_using_isaimages_for_getting_txt_areas");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, "ssp_oim_property_using_ilaimages_for_getting_txt_areas");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, "ssp_oim_property_validate_and_compare_cleared_txt_images");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, "ssp_oim_property_dont_delete_unrecognized_images_first");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, "ssp_oim_property_dont_delete_unrecognized_images_second");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_default_string_for_empty_sub, "ssp_oim_property_default_string_for_empty_sub");
	ReadProperty(fin, m_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, "ssp_oim_group_settings_for_multiframe_image_processing");
	ReadProperty(fin, m_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, "ssp_oim_sub_group_settings_for_sub_detection");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_threads, "ssp_oim_property_threads");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_sub_frames_length, "ssp_oim_property_sub_frames_length");
	//ReadProperty(fin, m_cfg.m_ssp_oim_property_sub_square_error, "ssp_oim_property_sub_square_error");
	ReadProperty(fin, m_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, "ssp_oim_sub_group_settings_for_comparing_subs");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_vedges_points_line_error, "ssp_oim_property_vedges_points_line_error");
	ReadProperty(fin, m_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, "ssp_oim_sub_group_settings_for_checking_sub");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_text_procent, "ssp_oim_property_text_procent");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_min_text_length, "ssp_oim_property_min_text_length");	

	ReadProperty(fin, m_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, "ssp_oim_property_use_ISA_images_for_search_subtitles");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, "ssp_oim_property_use_ILA_images_for_search_subtitles");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, "ssp_oim_property_replace_ISA_by_filtered_version");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_max_dl_down, "ssp_oim_property_max_dl_down");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_max_dl_up, "ssp_oim_property_max_dl_up");

	ReadProperty(fin, m_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, "ssp_oim_property_use_gradient_images_for_clear_txt_images");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, "ssp_oim_property_clear_txt_images_by_main_color");
	ReadProperty(fin, m_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, "ssp_oim_property_use_ILA_images_for_clear_txt_images");

	ReadProperty(fin, m_cfg.m_ssp_oim_property_remove_wide_symbols, "ssp_oim_property_remove_wide_symbols");

	fin.close();
}

void CMainFrame::SaveSettings()
{
	ofstream fout;

	g_clear_txt_folders = m_pPanel->m_pOCRPanel->m_pcbCTXTF->GetValue();
	g_join_subs_and_correct_time = m_pPanel->m_pOCRPanel->m_pcbJSACT->GetValue();

	m_pPanel->m_pSSPanel->m_pOI->SaveEditControlValue();
	m_pPanel->m_pSSPanel->m_pOIM->SaveEditControlValue();

	fout.open(m_GeneralSettingsFileName.c_str(), ios::out);	

	WriteProperty(fout, m_cfg.m_prefered_locale, "prefered_locale");

	WriteProperty(fout, g_DontDeleteUnrecognizedImages1, "dont_delete_unrecognized_images1");
	WriteProperty(fout, g_DontDeleteUnrecognizedImages2, "dont_delete_unrecognized_images2");

	WriteProperty(fout, g_generate_cleared_text_images_on_test, "generate_cleared_text_images_on_test");
	WriteProperty(fout, g_show_results, "dump_debug_images");
	WriteProperty(fout, g_show_sf_results, "dump_debug_second_filtration_images");
	WriteProperty(fout, g_clear_test_images_folder, "clear_test_images_folder");
	WriteProperty(fout, g_show_transformed_images_only, "show_transformed_images_only");
	WriteProperty(fout, g_use_ocl, "use_ocl");
	WriteProperty(fout, g_use_cuda_gpu, "use_cuda_gpu");
	WriteProperty(fout, g_cuda_kmeans_initial_loop_iterations, "cuda_kmeans_initial_loop_iterations");
	WriteProperty(fout, g_cuda_kmeans_loop_iterations, "cuda_kmeans_loop_iterations");	
	WriteProperty(fout, g_cpu_kmeans_initial_loop_iterations, "cpu_kmeans_initial_loop_iterations");
	WriteProperty(fout, g_cpu_kmeans_loop_iterations, "cpu_kmeans_loop_iterations");

	WriteProperty(fout, g_smthr, "moderate_threshold_for_scaled_image");
	WriteProperty(fout, g_mthr, "moderate_threshold");
	WriteProperty(fout, g_mnthr, "moderate_threshold_for_NEdges");
	WriteProperty(fout, g_segw, "segment_width");
	WriteProperty(fout, g_segh, "segment_height");
	WriteProperty(fout, g_msegc, "minimum_segments_count");
	WriteProperty(fout, g_scd, "min_sum_color_diff");
	WriteProperty(fout, g_btd, "between_text_distace");
	WriteProperty(fout, g_tco, "text_centre_offset");
	WriteProperty(fout, g_scale, "image_scale_for_clear_image");

	WriteProperty(fout, g_use_ISA_images_for_get_txt_area, "use_ISA_images");
	WriteProperty(fout, g_use_ILA_images_for_get_txt_area, "use_ILA_images");

	WriteProperty(fout, g_use_gradient_images_for_clear_txt_images, "use_gradient_images_for_clear_txt_images");
	WriteProperty(fout, g_clear_txt_images_by_main_color, "clear_txt_images_by_main_color");
	WriteProperty(fout, g_use_ILA_images_for_clear_txt_images, "use_ILA_images_for_clear_txt_images");

	WriteProperty(fout, g_mpn, "min_points_number");
	WriteProperty(fout, g_mpd, "min_points_density");
	WriteProperty(fout, g_msh, "min_symbol_height");
	WriteProperty(fout, g_msd, "min_symbol_density");
	WriteProperty(fout, g_mpned, "min_NEdges_points_density");

	WriteProperty(fout, g_threads, "threads");
	WriteProperty(fout, g_ocr_threads, "ocr_threads");
	WriteProperty(fout, g_DL, "sub_frame_length");
	WriteProperty(fout, g_tp, "text_procent");
	WriteProperty(fout, g_mtpl, "min_text_len_(in_procent)");
	//WriteProperty(fout, g_sse, "sub_square_error");
	WriteProperty(fout, g_veple, "vedges_points_line_error");

	WriteProperty(fout, g_clear_txt_folders, "clear_txt_folders");
	WriteProperty(fout, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	WriteProperty(fout, g_clear_image_logical, "clear_image_logical");

	WriteProperty(fout, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	WriteProperty(fout, g_DefStringForEmptySub, "def_string_for_empty_sub");

	WriteProperty(fout, m_cfg.m_ocr_min_sub_duration, "min_sub_duration");

	WriteProperty(fout, m_cfg.m_txt_dw, "txt_dw");
	WriteProperty(fout, m_cfg.m_txt_dy, "txt_dy");

	WriteProperty(fout, m_cfg.m_fount_size_ocr_lbl, "fount_size_ocr_lbl");
	WriteProperty(fout, m_cfg.m_fount_size_ocr_btn, "fount_size_ocr_btn");

	WriteProperty(fout, g_use_ISA_images_for_search_subtitles, "use_ISA_images_for_search_subtitles");
	WriteProperty(fout, g_use_ILA_images_for_search_subtitles, "use_ILA_images_for_search_subtitles");
	WriteProperty(fout, g_replace_ISA_by_filtered_version, "replace_ISA_by_filtered_version");
	WriteProperty(fout, g_max_dl_down, "max_dl_down");
	WriteProperty(fout, g_max_dl_up, "max_dl_up");

	WriteProperty(fout, g_remove_wide_symbols, "remove_wide_symbols");

	fout.close();
}

void CMainFrame::SaveError(std::string error)
{
	ofstream fout;
	fout.open(m_ErrorFileName.c_str(), ios::out | ios::app);
	fout << error << '\n';
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
			g_work_dir, wxEmptyString, wxT("*.cfg"), wxFD_OPEN);

		if(fd.ShowModal() != wxID_OK)
		{
			return;
		}

		m_GeneralSettingsFileName = fd.GetPath();
	}

	LoadSettings();
}

void CMainFrame::OnFileSaveSettingsAs(wxCommandEvent& event)
{
	wxFileDialog fd(this, _T("Save Settings File"),
		g_work_dir, wxEmptyString, wxT("Settings Files (*.cfg)|*.cfg"), wxFD_SAVE);

	if (m_blnReopenVideo == false)
	{
		if(fd.ShowModal() != wxID_OK)
		{
			return;
		}

		m_GeneralSettingsFileName = fd.GetPath();
	}

	SaveSettings();
}

void CMainFrame::OnFileSaveSettings(wxCommandEvent& event)
{
	SaveSettings();
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

	m_pVideoBox->m_pSB->SetScrollPos((int)Cur);

	//m_pVideoBox->m_pVBox->m_pHSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pHSL2->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL2->Refresh(true);
}

string VideoTimeToStr2(s64 pos)
{
	static char str[100];
	int hour, min, sec, msec, val;

	val = (int)(pos / 1000); // seconds
	msec = pos - (s64)val * 1000;
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	sprintf(str, "%02d:%02d:%02d,%03d", hour, min, sec, msec);

	return string(str);
}

string VideoTimeToStr3(s64 pos)
{
	static char str[100];
	int hour, min, sec, sec_100, val;

	val = (int)(pos / 1000); // seconds
	sec_100 = (pos - (s64)val * 1000)/10;
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	sprintf(str, "%.1d:%.2d:%.2d.%.2d", hour, min, sec, sec_100);

	return string(str);
}

s64 GetVideoTime(string time)
{
	s64 res;
	int hour, min, sec, msec;
	wxASSERT_MSG(sscanf(time.c_str(), "%d:%d:%d:%d", &hour, &min, &sec, &msec) == 4, wxString::Format(wxT("Wrong video time format '%s'"), time.c_str()));
	res = (s64)(((hour * 60 + min) * 60 + sec) * 1000 + msec);
	return res;
}

s64 GetVideoTime(int minute, int sec, int mili_sec)
{
	s64 res;
	res = (s64)((minute*60+sec)*1000+mili_sec);
	return res;
}

string ConvertVideoTime(s64 pos)
{
	static char str[100];
	int hour, min, sec, msec, val;
	
	val = (int)(pos / 1000); // seconds
	msec = pos - (s64)val * 1000;
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	sprintf(str, "%02d:%02d:%02d:%03d", hour, min, sec, msec);
	
	return string(str);
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

	if ( (g_IsSearching == 0) && (m_FileName != wxString("")) )
	{
		fstream fout;
		string pvi_path = g_work_dir + string("/previous_video.inf");

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
		//m_pPanel->m_pSHPanel->m_pSearchThread->SetPriority(90); //THREAD_PRIORITY_HIGHEST
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

	Destroy();
}

void CMainFrame::OnFileOpenPreviousVideo(wxCommandEvent& event)
{
	char str[300];
	fstream fin;
	string pvi_path = g_work_dir + string("/previous_video.inf");

	fin.open(pvi_path.c_str(), ios::in);
	
	fin.getline(str, 300);
	m_FileName = wxString::FromUTF8(str);

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

void CMainFrame::ClearDir(wxString DirName)
{
	wxDir dir(DirName);
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
		res = wxRemoveFile(dir.GetName() + "/" + FileNamesVector[i]);
	}
	
	FileNamesVector.clear();
}

void CMainFrame::OnAppAbout(wxCommandEvent& event)
{
	(void)wxMessageBox("This program was written by Simeon Kosnitsky. \nPublished under public domain license.", "VideoSubFinder " VSF_VERSION " Version");
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

	fin.clear();
	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i < strlen(str); i++)
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

	fin.clear();
	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i < strlen(str); i++)
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

	fin.clear();
	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';
		
		fin.getline(str, 100);
		for(int i=0; i < strlen(str); i++)
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

	fin.clear();
	fin.seekg(0);
	do
	{
		fin >> name;
		fin >> str;
		str[0] = '\0';

		fin.getline(str, 100);
		for(int i=0; i < strlen(str); i++)
		{
			str[i] = str[i+1];
		}
	} while((Name != string(name)) && !fin.eof());
	
	if (!fin.eof()) 
	{
		val = wxString(str);
	}
}
