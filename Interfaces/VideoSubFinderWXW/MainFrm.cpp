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
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/regex.h>
#include <chrono>
#include "Control.h"

CMainFrame *g_pMF;

bool g_playback_sound = false;

// const DWORD _MMX_FEATURE_BIT = 0x00800000;
// const DWORD _SSE2_FEATURE_BIT = 0x04000000;

std::vector<CControl*> CControl::m_all_controls;

wxDEFINE_EVENT(VIEW_IMAGE_IN_IMAGE_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_IMAGE_IN_VIDEO_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_GREYSCALE_IMAGE_IN_IMAGE_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_GREYSCALE_IMAGE_IN_VIDEO_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_BGR_IMAGE_IN_IMAGE_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_BGR_IMAGE_IN_VIDEO_BOX, wxThreadEvent);
wxDEFINE_EVENT(VIEW_RGB_IMAGE, wxThreadEvent);

/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32	
int exception_filter(unsigned int code, struct _EXCEPTION_POINTERS *ep, char *det)
{
	g_pMF->SaveError(wxT("Got C Exception: ") + wxString(det));
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

template <typename T>
struct ThreadData
{
	simple_buffer<T> m_Im;
	int m_w;
	int m_h;
};

/////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void send_thread_data(wxEventType eventType, simple_buffer<T>& Im, int w, int h)
{
	if (!(g_pMF->m_blnNoGUI))
	{
		const std::lock_guard<std::mutex> lock(g_pMF->m_mutex);

        auto event = new wxThreadEvent(eventType);

		ThreadData<T> td;
		td.m_Im = Im;
		td.m_w = w;
		td.m_h = h;

		event->SetPayload(td);

    	wxQueueEvent(g_pMF, event);
	}
}

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInImageBox(simple_buffer<int>& Im, int w, int h)
{
	send_thread_data(VIEW_IMAGE_IN_IMAGE_BOX, Im, w, h);	
}

void CMainFrame::OnViewImageInImageBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<int> td = event.GetPayload<ThreadData<int>>();
	g_pMF->m_pImageBox->ViewImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewImageInVideoBox(simple_buffer<int>& Im, int w, int h)
{
	send_thread_data(VIEW_IMAGE_IN_VIDEO_BOX, Im, w, h);	
}

void CMainFrame::OnViewImageInVideoBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<int> td = event.GetPayload<ThreadData<int>>();
	g_pMF->m_pVideoBox->ViewImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewGreyscaleImageInImageBox(simple_buffer<u8>& Im, int w, int h)
{
	send_thread_data(VIEW_GREYSCALE_IMAGE_IN_IMAGE_BOX, Im, w, h);	
}

void CMainFrame::OnViewGreyscaleImageInImageBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<u8> td = event.GetPayload<ThreadData<u8>>();
	g_pMF->m_pImageBox->ViewGrayscaleImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewGreyscaleImageInVideoBox(simple_buffer<u8>& Im, int w, int h)
{
	send_thread_data(VIEW_GREYSCALE_IMAGE_IN_VIDEO_BOX, Im, w, h);	
}

void CMainFrame::OnViewGreyscaleImageInVideoBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<u8> td = event.GetPayload<ThreadData<u8>>();
	g_pMF->m_pVideoBox->ViewGrayscaleImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewBGRImageInImageBox(simple_buffer<u8>& ImBGR, int w, int h)
{
	send_thread_data(VIEW_BGR_IMAGE_IN_IMAGE_BOX, ImBGR, w, h);	
}

void CMainFrame::OnViewBGRImageInImageBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<u8> td = event.GetPayload<ThreadData<u8>>();
	g_pMF->m_pImageBox->ViewBGRImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void  ViewBGRImageInVideoBox(simple_buffer<u8>& ImBGR, int w, int h)
{
	send_thread_data(VIEW_BGR_IMAGE_IN_VIDEO_BOX, ImBGR, w, h);	
}

void CMainFrame::OnViewBGRImageInVideoBox(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<u8> td = event.GetPayload<ThreadData<u8>>();
	g_pMF->m_pVideoBox->ViewBGRImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

void ViewRGBImage(simple_buffer<int> &Im, int w, int h)
{
	send_thread_data(VIEW_RGB_IMAGE, Im, w, h);		
}

void CMainFrame::OnViewRGBImage(wxThreadEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);	
	ThreadData<int> td = event.GetPayload<ThreadData<int>>();
	g_pMF->m_pImageBox->ViewRGBImage(td.m_Im, td.m_w, td.m_h);
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(CMainFrame, wxFrame)
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
	EVT_MOUSEWHEEL(CMainFrame::OnMouseWheel)
END_EVENT_TABLE() 

/////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame(const wxString& title)
		: wxFrame( NULL, wxID_ANY, title,
							wxDefaultPosition, wxDefaultSize,
							wxDEFAULT_FRAME_STYLE | wxFRAME_NO_WINDOW_MENU )
		, m_timer(this, TIMER_ID)
{
	m_WasInited = false;
	m_VIsOpen = false;

	m_pPanel = NULL;
	m_pVideoBox = NULL;
	m_pImageBox = NULL;
	m_pVideo = NULL;

#ifdef WIN32
	// set frame icon
	this->SetIcon(wxIcon("vsf_ico"));
#endif

	g_pV = NULL;

	g_pMF = this;
	g_pViewImage[0] = ViewImageInVideoBox;
	g_pViewImage[1] = ViewImageInImageBox;
	g_pViewBGRImage[0] = ViewBGRImageInVideoBox;
	g_pViewBGRImage[1] = ViewBGRImageInImageBox;
	g_pViewGreyscaleImage[0] = ViewGreyscaleImageInVideoBox;
	g_pViewGreyscaleImage[1] = ViewGreyscaleImageInImageBox;
	g_pViewRGBImage = ViewRGBImage;


	m_blnReopenVideo = false;

	m_FileName = "";
	m_dt = 0;

	m_type = 0;

	m_GeneralSettingsFileName = g_app_dir + wxT("/settings/general.cfg");
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::Init()
{
	int cnt;	

	SaveToReportLog("CMainFrame::Init(): starting...\n");

	m_blnNoGUI = false;

	wxMenuBar *pMenuBar = new wxMenuBar;	

	m_ErrorFileName = g_work_dir + wxT("/error.log");

	SaveToReportLog("CMainFrame::Init(): InitCUDADevice...\n");

	if (!InitCUDADevice())
	{
		g_use_cuda_gpu = false;
	}

	SaveToReportLog("CMainFrame::Init(): Init Menu Bar...\n");

	cnt = pMenuBar->GetMenuCount();

	wxMenu *pMenu5 = new wxMenu;
	pMenu5->Append(ID_SETPRIORITY_HIGH, _T("HIGH"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_ABOVENORMAL, _T("ABOVE NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_NORMAL, _T("NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_BELOWNORMAL, _T("BELOW NORMAL"), _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_IDLE, _T("IDLE"), _T(""), wxITEM_CHECK);

	wxMenu *pMenu1 = new wxMenu;	
	pMenu1->Append(ID_FILE_OPEN_VIDEO_OPENCV, _T("Open Video (OpenCV)"));
	pMenu1->Append(ID_FILE_OPEN_VIDEO_FFMPEG, _T("Open Video (FFMPEG)"));
	pMenu1->Append(ID_FILE_REOPENVIDEO, _T("Reopen Video"));
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
	pMenu3->Append(ID_PLAY_PAUSE, _T("Play/Pause   Space"));
	pMenu3->Append(ID_PLAY_STOP, _T("Stop"));
	pMenuBar->Append(pMenu3, _T("&Play"));

	wxMenu *pMenu4 = new wxMenu;
	pMenu4->Append(ID_APP_ABOUT, _T("&About..."));
	pMenuBar->Append(pMenu4, _T("&Help"));

	cnt = pMenuBar->GetMenuCount();			

	// CSeparatingLine *pHSL = new CSeparatingLine(this, 200, 3, 7, 3, 100, 110, 50, 0);
	// pHSL->m_pos = 0;
	// pHSL->Raise();
	// return;

	wxColor bc(215, 228, 242);

	SaveToReportLog("CMainFrame::Init(): new CImageBox(this)...\n");
	m_pImageBox = new CImageBox(this);

	SaveToReportLog("CMainFrame::Init(): m_pImageBox->Init()...\n");
	m_pImageBox->Init();
	m_pImageBox->SetBackgroundColour(bc);

	int w = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	int h = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
	
	SaveToReportLog("CMainFrame::Init(): new CVideoBox(this)...\n");
	m_pVideoBox = new CVideoBox(this, bc);

	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->Init()...\n");
	m_pVideoBox->Init();
	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->Bind...\n");
	//m_pVideoBox->Bind(wxEVT_CHAR_HOOK, &CVideoBox::OnKeyDown, m_pVideoBox);
	
	SaveToReportLog("CMainFrame::Init(): LoadSettings()...\n");
	LoadSettings();

#ifdef WIN32
	if (m_cfg.process_affinity_mask > 0)
	{
		HANDLE process = GetCurrentProcess();
		DWORD_PTR dwProcessAffinityMask, dwSystemAffinityMask;

		GetProcessAffinityMask(process, &dwProcessAffinityMask, &dwSystemAffinityMask);

		dwProcessAffinityMask = (DWORD_PTR)m_cfg.process_affinity_mask & dwSystemAffinityMask;
		BOOL success = SetProcessAffinityMask(process, dwProcessAffinityMask);

		SaveToReportLog(wxString::Format(wxT("CMainFrame::Init(): SetProcessAffinityMask(%d) == %d\n"), (int)dwProcessAffinityMask, (int)success));
	}
#endif

	bool find_fount_size_lbl = false;
	bool find_fount_size_btn = false;

	if (m_cfg.m_fount_size_lbl == -1)
	{
		m_cfg.m_fount_size_lbl = 8;
		find_fount_size_lbl = true;
	}
	if (m_cfg.m_fount_size_btn == -1)
	{
		m_cfg.m_fount_size_btn = 10;
		find_fount_size_btn = true;
	}

	// https://docs.wxwidgets.org/stable/interface_2wx_2font_8h.html#a0cd7bfd21a4f901245d3c86d8ea0c080
	// wxFONTFAMILY_SWISS / A sans - serif font.

	SaveToReportLog("CMainFrame::Init(): init m_BTNFont...\n");
	m_BTNFont = wxFont(m_cfg.m_fount_size_btn, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);

	SaveToReportLog("CMainFrame::Init(): init m_LBLFont...\n");
	m_LBLFont = wxFont(m_cfg.m_fount_size_lbl, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString, wxFONTENCODING_DEFAULT);

	SaveToReportLog("CMainFrame::Init(): new CSSOWnd(this)...\n");
	m_pPanel = new CSSOWnd(this);

	SaveToReportLog("CMainFrame::Init(): this->SetMenuBar(pMenuBar)...\n");
	this->SetMenuBar(pMenuBar);

	SaveToReportLog("CMainFrame::Init(): this->SetSize(..)...\n");

#ifdef WIN32
	this->SetSize(0, 0, w, h - 50);
#else
	this->SetSize(w/16, h/14, (7*w)/8, (6*h)/7);
#endif

	SaveToReportLog("CMainFrame::Init(): m_pPanel->Init()...\n");
	m_pPanel->Init();

	// Finding Optimal Font Size For Labels
	if (find_fount_size_lbl)
	{
		SaveToReportLog("CMainFrame::Init(): GetOptimalFontSize(..) for Labels...\n");

		int cw, ch;
		m_pPanel->m_pSSPanel->m_plblPixelColor->GetClientSize(&cw, &ch);
		m_cfg.m_fount_size_lbl = GetOptimalFontSize(cw, ch, m_cfg.m_label_pixel_color, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString, wxFONTENCODING_DEFAULT);
		
		if (m_cfg.m_fount_size_lbl > 10)
		{
			m_cfg.m_fount_size_lbl = 10;
		}
	}

	// Finding Optimal Font Size For Buttons
	if (find_fount_size_btn)
	{
		SaveToReportLog("CMainFrame::Init(): GetOptimalFontSize(..) for Buttons...\n");

		int cw, ch;
		m_pPanel->m_pOCRPanel->m_pCSCTI->GetClientSize(&cw, &ch);
		m_cfg.m_fount_size_btn = GetOptimalFontSize(cw, ch, m_cfg.m_ocr_button_cesfcti_text, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);

		if (m_cfg.m_fount_size_btn > 13)
		{
			m_cfg.m_fount_size_btn = 13;
		}
	}

	int cw, ch;
	this->GetClientSize(&cw, &ch);	

	SaveToReportLog("CMainFrame::Init(): m_pImageBox->SetSize(..)...\n");
	m_pImageBox->SetSize(cw / 2 + m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
	SaveToReportLog("CMainFrame::Init(): m_pImageBox->Show(true)...\n");
	m_pImageBox->Show(true);

	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->SetSize(..)...\n");
	m_pVideoBox->SetSize(m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->Show(true)...\n");	
	m_pVideoBox->Show(true);	

	m_WasInited = true;

	if (find_fount_size_lbl || find_fount_size_btn)
	{
		SaveToReportLog("CMainFrame::Init(): reinit m_BTNFont...\n");
		m_BTNFont = wxFont(m_cfg.m_fount_size_btn, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString, wxFONTENCODING_DEFAULT);

		SaveToReportLog("CMainFrame::Init(): reinit m_LBLFont...\n");
		m_LBLFont = wxFont(m_cfg.m_fount_size_lbl, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString, wxFONTENCODING_DEFAULT);
	}
		
	CControl::RefreshAllControlsData();
	this->Refresh();

	this->Bind(VIEW_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewImageInImageBox, this);
	this->Bind(VIEW_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewImageInVideoBox, this);
	this->Bind(VIEW_GREYSCALE_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewGreyscaleImageInImageBox, this);
	this->Bind(VIEW_GREYSCALE_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewGreyscaleImageInVideoBox, this);
	this->Bind(VIEW_BGR_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewBGRImageInImageBox, this);
	this->Bind(VIEW_BGR_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewBGRImageInVideoBox, this);
	this->Bind(VIEW_RGB_IMAGE, &CMainFrame::OnViewRGBImage, this);

#ifndef WIN32
	m_bUpdateSizes = true;
#endif

	SaveToReportLog("CMainFrame::Init(): finished.\n");
}

int CMainFrame::GetOptimalFontSize(int cw, int ch, wxString label, wxFontFamily family, wxFontStyle style, wxFontWeight weight, bool underlined, const wxString& face, wxFontEncoding encoding)
{
	wxStaticText static_text(this, wxID_ANY, label);	
	wxClientDC dc(this);
	wxSize text_size;
	int font_size = 0;

	do
	{
		font_size++;
		wxFont font(font_size, family, style, weight, underlined, face, encoding);
		dc.SetFont(font);
		text_size = dc.GetMultiLineTextExtent(label);
	} while ((text_size.GetWidth() < cw) && (text_size.GetHeight() < ch));
	font_size--;

	if (font_size == 0)
	{
		font_size = 1;
	}

	return font_size;
}

void CMainFrame::OnMouseWheel(wxMouseEvent& event)
{
	if (wxGetKeyState(WXK_CONTROL))
	{
		if (event.m_wheelRotation > 0)
		{
			m_cfg.m_fount_size_btn++;
			m_cfg.m_fount_size_lbl++;
		}
		else
		{
			if ((m_cfg.m_fount_size_btn > 1) && (m_cfg.m_fount_size_lbl > 1))
			{
				m_cfg.m_fount_size_btn--;
				m_cfg.m_fount_size_lbl--;
			}
		}

		m_BTNFont = wxFont(m_cfg.m_fount_size_btn, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
			wxFONTWEIGHT_BOLD, false /* !underlined */,
			wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

		m_LBLFont = wxFont(m_cfg.m_fount_size_lbl, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
			wxFONTWEIGHT_NORMAL, false /* !underlined */,
			wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

		CControl::RefreshAllControlsData();
		this->Refresh();
	}
}

void CMainFrame::OnSize(wxSizeEvent& event)
{
	int cw, ch;
    this->GetClientSize(&cw, &ch);
	m_cw = cw;
	m_ch = ch;

	if (m_pPanel)
	{		
		m_pPanel->SetSize(0, ch - m_ph, cw, m_ph);
#ifdef WIN32		
		m_pPanel->Refresh();
		m_pPanel->Update();
#endif
	}
	
	if (m_bUpdateSizes)
	{		
		m_pImageBox->SetSize(cw / 2 + m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
		m_pVideoBox->SetSize(m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
		m_bUpdateSizes = false;
	}

	//GetClientWindow()->SetSize(0, 0, w, h - m_ph);
}

void CMainFrame::OnFileReOpenVideo(wxCommandEvent& event)
{
	if (m_FileName.size() > 0)
	{
		m_blnReopenVideo = true;
		OnFileOpenVideo(m_type);
	}
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
		// https://en.wikipedia.org/wiki/Video_file_format
		wxString all_video_formats("*.3g2;*.3gp;*.amv;*.asf;*.avi;*.drc;*.flv;*.f4v;*.f4p;*.f4a;*.f4b;*.gif;*.gifv;*.m4p;*.m4v;*.m4v;*.mkv;*.mng;*.mov;*.qt;*.mp4;*.mpg;*.mp2;*.mpeg;*.mpe;*.mpv;*.mpg;*.mpeg;*.m2v;*.mts;*.m2ts;*.ts;*.mxf;*.nsv;*.ogv;*.ogg;*.rm;*.rmvb;*.roq;*.svi;*.viv;*.vob;*.webm;*.wmv;*.yuv;*.avs");

		wxFileDialog fd(this, wxT("Open Video File"),
						wxEmptyString, wxEmptyString, wxString::Format(wxT("Video Files (%s)|%s|All Files (*.*)|*.*"), all_video_formats, all_video_formats), wxFD_OPEN);

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

	if (m_pVideoBox->m_pImage != NULL)
	{
		delete m_pVideoBox->m_pImage;
		m_pVideoBox->m_pImage = NULL;
	}
	m_pVideoBox->ClearScreen();

	if (m_pImageBox->m_pImage != NULL)
	{		
		delete m_pImageBox->m_pImage;
		m_pImageBox->m_pImage = NULL;
	}
	m_pImageBox->ClearScreen();

	m_FileName = csFileName;

	this->Disable();

	if (m_type == 0)
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovie(m_FileName, (void*)m_pVideoBox->m_pVBox, 0);
	}
	else
	{
		m_blnOpenVideoResult = m_pVideo->OpenMovie(m_FileName, (void*)m_pVideoBox->m_pVBox, 0);
	}

	SaveToReportLog(wxString::Format(wxT("Video was opened \"%s\" with log:\n%s\n"), m_FileName, m_pVideo->m_log));

	if (m_blnOpenVideoResult == false) 
	{
		m_VIsOpen = false;
		m_pVideoBox->m_plblVB->SetLabel("VideoBox");
		m_FileName = "";
		m_blnReopenVideo = false;

		this->Enable();

		return;
	}

	if (m_pVideo->m_Duration > 0)
	{
		m_pVideoBox->m_pSB->Enable(true);
		m_pVideoBox->m_pSB->SetScrollPos(0);
		m_pVideoBox->m_pSB->SetScrollRange(0, (int)(m_pVideo->m_Duration));
	}
	else
	{
		m_pVideoBox->m_pSB->Disable();
	}

	m_pVideoBox->m_plblVB->SetLabel("VideoBox \"" + GetFileName(csFileName) + "\"");

	if (m_blnReopenVideo == false) 
	{
		m_BegTime = 0;
		m_EndTime = m_pVideo->m_Duration;
	}

	m_pPanel->m_pSHPanel->m_plblBTA1->SetValue(ConvertVideoTime(m_BegTime));
	m_pPanel->m_pSHPanel->m_plblBTA2->SetValue(ConvertVideoTime(m_EndTime));

	m_w = m_pVideo->m_Width;
	m_h = m_pVideo->m_Height;
	m_BufferSize = m_w*m_h*sizeof(int);
	m_VIsOpen = true;

	wxRect rc, rcP, rVB, rcVW, rcIW, rImB;
	int w, wmax, h, ww, hh, dw, dh, dwi, dhi;

	this->GetClientSize(&w, &h);
	m_cw = w;
	m_ch = h;

	rc.x = rc.y = 0; 
	rc.width = m_cw;
	rc.height = m_ch;

	rcP = m_pPanel->GetRect();

	rVB = m_pVideoBox->GetRect();

	m_pVideoBox->m_pVBox->m_pVideoWnd->GetClientSize(&w, &h);
	rcVW.x = rcVW.y = 0; 
	rcVW.width = w;
	rcVW.height = h;
	
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

	m_EndTimeStr = wxT("/") + ConvertVideoTime(m_pVideo->m_Duration);

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

	m_pVideoBox->SetFocus();
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

void CMainFrame::ReadSettings(wxString file_name, std::map<wxString, wxString>& settings)
{
	wxFileInputStream ffin(file_name);

	if (!ffin.IsOk())
	{
		(void)wxMessageBox(wxT("ERROR: Can't open settings file: ") + file_name);
		exit(1);
	}

	wxTextInputStream fin(ffin, wxT("\x09"), wxConvUTF8);
	wxString name, val, line;
	wxRegEx re(wxT("^[[:space:]]*([^[:space:]]+)[[:space:]]*=[[:space:]]*([^[:space:]].*[^[:space:]]|[^[:space:]])[[:space:]]*$"));
	wxRegEx re_empty(wxT("^[[:space:]]*([^[:space:]]+)[[:space:]]*=[[:space:]]*$"));

	while (ffin.IsOk() && !ffin.Eof())
	{
		line = fin.ReadLine();		

		if (line.size() > 0)
		{
			if (re.Matches(line))
			{
				name = re.GetMatch(line, 1);
				val = re.GetMatch(line, 2);
				settings[name] = val;
			}
			else
			{
				if (re_empty.Matches(line))
				{
					name = re.GetMatch(line, 1);
					val = wxString(wxT(""));
					settings[name] = val;
				}
				else
				{
					(void)wxMessageBox(wxT("Unsupported line format: ") + line + wxT("\nin file: ") + file_name);
					exit(1);
				}
			}
		}
	}
}

void CMainFrame::LoadSettings()
{
	SaveToReportLog("CMainFrame::LoadSettings(): starting...\n");

	SaveToReportLog("CMainFrame::LoadSettings(): ReadSettings(m_GeneralSettingsFileName, m_general_settings)...\n");
	ReadSettings(m_GeneralSettingsFileName, m_general_settings);
	
	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from m_general_settings...\n");

	ReadProperty(m_general_settings, g_DontDeleteUnrecognizedImages1, "dont_delete_unrecognized_images1");
	ReadProperty(m_general_settings, g_DontDeleteUnrecognizedImages2, "dont_delete_unrecognized_images2");

	ReadProperty(m_general_settings, g_generate_cleared_text_images_on_test, "generate_cleared_text_images_on_test");
	ReadProperty(m_general_settings, g_show_results, "dump_debug_images");
	ReadProperty(m_general_settings, g_show_sf_results, "dump_debug_second_filtration_images");
	ReadProperty(m_general_settings, g_clear_test_images_folder, "clear_test_images_folder");
	ReadProperty(m_general_settings, g_show_transformed_images_only, "show_transformed_images_only");
	ReadProperty(m_general_settings, g_use_ocl, "use_ocl");
	ReadProperty(m_general_settings, g_use_cuda_gpu, "use_cuda_gpu");

	ReadProperty(m_general_settings, g_use_filter_color, "use_filter_color");
	ReadProperty(m_general_settings, g_use_outline_filter_color, "use_outline_filter_color");
	ReadProperty(m_general_settings, g_dL_color, "dL_color");
	ReadProperty(m_general_settings, g_dA_color, "dA_color");
	ReadProperty(m_general_settings, g_dB_color, "dB_color");
	ReadProperty(m_general_settings, g_combine_to_single_cluster, "combine_to_single_cluster");

	ReadProperty(m_general_settings, g_cuda_kmeans_initial_loop_iterations, "cuda_kmeans_initial_loop_iterations");
	ReadProperty(m_general_settings, g_cuda_kmeans_loop_iterations, "cuda_kmeans_loop_iterations");		
	ReadProperty(m_general_settings, g_cpu_kmeans_initial_loop_iterations, "cpu_kmeans_initial_loop_iterations");
	ReadProperty(m_general_settings, g_cpu_kmeans_loop_iterations, "cpu_kmeans_loop_iterations");	

	ReadProperty(m_general_settings, g_smthr, "moderate_threshold_for_scaled_image");
	ReadProperty(m_general_settings, g_mthr, "moderate_threshold");
	ReadProperty(m_general_settings, g_mnthr, "moderate_threshold_for_NEdges");
	ReadProperty(m_general_settings, g_segw, "segment_width");
	ReadProperty(m_general_settings, g_segh, "segment_height");
	ReadProperty(m_general_settings, g_msegc, "minimum_segments_count");
	ReadProperty(m_general_settings, g_scd, "min_sum_color_diff");
	ReadProperty(m_general_settings, g_btd, "between_text_distace");
	ReadProperty(m_general_settings, g_to, "text_centre_offset");
	ReadProperty(m_general_settings, g_scale, "image_scale_for_clear_image");
	
	ReadProperty(m_general_settings, g_use_ISA_images_for_get_txt_area, "use_ISA_images");
	ReadProperty(m_general_settings, g_use_ILA_images_for_get_txt_area, "use_ILA_images");

	ReadProperty(m_general_settings, g_use_ILA_images_for_getting_txt_symbols_areas, "use_ILA_images_for_getting_txt_symbols_areas");
	ReadProperty(m_general_settings, g_use_ILA_images_before_clear_txt_images_from_borders, "use_ILA_images_before_clear_txt_images_from_borders");

	ReadProperty(m_general_settings, g_use_gradient_images_for_clear_txt_images, "use_gradient_images_for_clear_txt_images");
	ReadProperty(m_general_settings, g_clear_txt_images_by_main_color, "clear_txt_images_by_main_color");
	ReadProperty(m_general_settings, g_use_ILA_images_for_clear_txt_images, "use_ILA_images_for_clear_txt_images");	

	ReadProperty(m_general_settings, g_mpn, "min_points_number");
	ReadProperty(m_general_settings, g_mpd, "min_points_density");
	ReadProperty(m_general_settings, g_msh, "min_symbol_height");
	ReadProperty(m_general_settings, g_msd, "min_symbol_density");
	ReadProperty(m_general_settings, g_mpned, "min_NEdges_points_density");				

	ReadProperty(m_general_settings, g_clear_txt_folders, "clear_txt_folders");
	ReadProperty(m_general_settings, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	ReadProperty(m_general_settings, g_threads, "threads");
	ReadProperty(m_general_settings, g_ocr_threads, "ocr_threads");
	ReadProperty(m_general_settings, g_DL, "sub_frame_length");
	ReadProperty(m_general_settings, g_tp, "text_percent");
	ReadProperty(m_general_settings, g_mtpl, "min_text_len_in_percent");
	ReadProperty(m_general_settings, g_veple, "vedges_points_line_error");
	ReadProperty(m_general_settings, g_ilaple, "ila_points_line_error");

	ReadProperty(m_general_settings, g_video_contrast, "video_contrast");
	ReadProperty(m_general_settings, g_video_gamma, "video_gamma");
	
	ReadProperty(m_general_settings, g_clear_image_logical, "clear_image_logical");

	ReadProperty(m_general_settings, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	ReadProperty(m_general_settings, g_DefStringForEmptySub, "def_string_for_empty_sub");

	ReadProperty(m_general_settings, m_cfg.m_prefered_locale, "prefered_locale");

	ReadProperty(m_general_settings, m_cfg.process_affinity_mask, "process_affinity_mask");

	ReadProperty(m_general_settings, m_cfg.m_ocr_min_sub_duration, "min_sub_duration");
	ReadProperty(m_general_settings, m_cfg.m_ocr_join_txt_images_split_line, "ocr_join_txt_images_split_line");
	
	ReadProperty(m_general_settings, m_cfg.m_txt_dw, "txt_dw");
	ReadProperty(m_general_settings, m_cfg.m_txt_dy, "txt_dy");

	ReadProperty(m_general_settings, m_cfg.m_fount_size_lbl, "fount_size_lbl");
	ReadProperty(m_general_settings, m_cfg.m_fount_size_btn, "fount_size_btn");

	ReadProperty(m_general_settings, g_use_ISA_images_for_search_subtitles, "use_ISA_images_for_search_subtitles");
	ReadProperty(m_general_settings, g_use_ILA_images_for_search_subtitles, "use_ILA_images_for_search_subtitles");
	ReadProperty(m_general_settings, g_replace_ISA_by_filtered_version, "replace_ISA_by_filtered_version");
	ReadProperty(m_general_settings, g_max_dl_down, "max_dl_down");
	ReadProperty(m_general_settings, g_max_dl_up, "max_dl_up");

	ReadProperty(m_general_settings, g_remove_wide_symbols, "remove_wide_symbols");

	ReadProperty(m_general_settings, g_hw_device, "hw_device");

	if (ReadProperty(m_general_settings, g_filter_descr, "filter_descr"))
	{
		if (g_filter_descr == wxT("none"))
		{
			g_filter_descr = wxT("");
		}
	}

	ReadProperty(m_general_settings, g_save_each_substring_separately, "save_each_substring_separately");
	ReadProperty(m_general_settings, g_save_scaled_images, "save_scaled_images");

	ReadProperty(m_general_settings, g_playback_sound, "playback_sound");

	ReadProperty(m_general_settings, g_border_is_darker, "border_is_darker");

	ReadProperty(m_general_settings, g_text_alignment_string, "text_alignment");

	ReadProperty(m_general_settings, g_extend_by_grey_color, "extend_by_grey_color");
	ReadProperty(m_general_settings, g_allow_min_luminance, "allow_min_luminance");

	double double_val;
	
	if (ReadProperty(m_general_settings, double_val, "bottom_video_image_percent_end"))
	{
		m_pVideoBox->m_pVBox->m_pHSL2->m_pos = 1 - double_val;
	}
	if (ReadProperty(m_general_settings, double_val, "top_video_image_percent_end"))
	{
		m_pVideoBox->m_pVBox->m_pHSL1->m_pos = 1 - double_val;
	}

	ReadProperty(m_general_settings, m_pVideoBox->m_pVBox->m_pVSL1->m_pos, "left_video_image_percent_end");
	ReadProperty(m_general_settings, m_pVideoBox->m_pVBox->m_pVSL2->m_pos, "right_video_image_percent_end");

	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from m_general_settings end.\n");

	//------------------------------------------------

	SaveToReportLog("CMainFrame::LoadSettings(): ReadSettings(.., m_locale_settings)...\n");

	ReadSettings(g_app_dir + wxT("/settings/") + wxString(m_cfg.m_prefered_locale.mb_str()) + wxT("/locale.cfg"), m_locale_settings);

	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from m_locale_settings...\n");

	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_msd_text, "ocr_label_msd_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_join_txt_images_split_line_text, "ocr_label_join_txt_images_split_line_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_jsact_text, "ocr_label_jsact_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_clear_txt_folders, "ocr_label_clear_txt_folders");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_save_each_substring_separately, "ocr_label_save_each_substring_separately");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_label_save_scaled_images, "ocr_label_save_scaled_images");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_ces_text, "ocr_button_ces_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_join_text, "ocr_button_join_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_ccti_text, "ocr_button_ccti_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_csftr_text, "ocr_button_csftr_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_cesfcti_text, "ocr_button_cesfcti_text");
	ReadProperty(m_locale_settings, m_cfg.m_ocr_button_test_text, "ocr_button_test_text");

	ReadProperty(m_locale_settings, m_cfg.m_label_text_alignment, "label_text_alignment");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_hw_device, "ssp_hw_device");
	ReadProperty(m_locale_settings, m_cfg.m_label_filter_descr, "label_filter_descr");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_use_ocl, "ssp_oi_property_use_ocl");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_use_cuda_gpu, "ssp_oi_property_use_cuda_gpu");

	ReadProperty(m_locale_settings, m_cfg.m_label_use_filter_color, "label_use_filter_color");
	ReadProperty(m_locale_settings, m_cfg.m_label_use_outline_filter_color, "label_use_outline_filter_color");
	ReadProperty(m_locale_settings, m_cfg.m_label_dL_color, "label_dL_color");
	ReadProperty(m_locale_settings, m_cfg.m_label_dA_color, "label_dA_color");
	ReadProperty(m_locale_settings, m_cfg.m_label_dB_color, "label_dB_color");

	ReadProperty(m_locale_settings, m_cfg.m_border_is_darker, "label_border_is_darker");
	ReadProperty(m_locale_settings, m_cfg.m_extend_by_grey_color, "label_extend_by_grey_color");
	ReadProperty(m_locale_settings, m_cfg.m_allow_min_luminance, "label_allow_min_luminance");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_ocr_threads, "ssp_ocr_threads");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_image_scale_for_clear_image, "ssp_oi_property_image_scale_for_clear_image");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, "ssp_oi_property_moderate_threshold_for_scaled_image");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, "ssp_oi_property_cuda_kmeans_initial_loop_iterations");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, "ssp_oi_property_cuda_kmeans_loop_iterations");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, "ssp_oi_property_cpu_kmeans_initial_loop_iterations");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, "ssp_oi_property_cpu_kmeans_loop_iterations");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_label_parameters_influencing_image_processing, "ssp_label_parameters_influencing_image_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, "ssp_label_ocl_and_multiframe_image_stream_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_group_global_image_processing_settings, "ssp_oi_group_global_image_processing_settings");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, "ssp_oi_property_generate_cleared_text_images_on_test");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_dump_debug_images, "ssp_oi_property_dump_debug_images");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, "ssp_oi_property_dump_debug_second_filtration_images");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_clear_test_images_folder, "ssp_oi_property_clear_test_images_folder");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_show_transformed_images_only, "ssp_oi_property_show_transformed_images_only");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_group_initial_image_processing, "ssp_oi_group_initial_image_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, "ssp_oi_sub_group_settings_for_sobel_operators");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_moderate_threshold, "ssp_oi_property_moderate_threshold");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_moderate_nedges_threshold, "ssp_oi_property_moderate_nedges_threshold");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, "ssp_oi_sub_group_settings_for_color_filtering");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_segment_width, "ssp_oi_property_segment_width");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_segments_count, "ssp_oi_property_min_segments_count");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_sum_color_difference, "ssp_oi_property_min_sum_color_difference");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_group_secondary_image_processing, "ssp_oi_group_secondary_image_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, "ssp_oi_sub_group_settings_for_linear_filtering");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_line_height, "ssp_oi_property_line_height");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_max_between_text_distance, "ssp_oi_property_max_between_text_distance");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_max_text_center_offset, "ssp_oi_property_max_text_center_offset");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_max_text_center_percent_offset, "ssp_oi_property_max_text_center_percent_offset");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, "ssp_oi_sub_group_settings_for_color_border_points");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_points_number, "ssp_oi_property_min_points_number");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_points_density, "ssp_oi_property_min_points_density");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_symbol_height, "ssp_oi_property_min_symbol_height");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_symbol_density, "ssp_oi_property_min_symbol_density");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_vedges_points_density, "ssp_oi_property_min_vedges_points_density");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_property_min_nedges_points_density, "ssp_oi_property_min_nedges_points_density");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oi_group_tertiary_image_processing, "ssp_oi_group_tertiary_image_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_group_ocr_settings, "ssp_oim_group_ocr_settings");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_clear_images_logical, "ssp_oim_property_clear_images_logical");
	ReadProperty(m_locale_settings, m_cfg.m_label_combine_to_single_cluster, "label_combine_to_single_cluster");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, "ssp_oim_property_clear_rgbimages_after_search_subtitles");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_using_hard_algorithm_for_text_mining, "ssp_oim_property_using_hard_algorithm_for_text_mining");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, "ssp_oim_property_using_isaimages_for_getting_txt_areas");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, "ssp_oim_property_using_ilaimages_for_getting_txt_areas");
	
	ReadProperty(m_locale_settings, m_cfg.m_label_ILA_images_for_getting_txt_symbols_areas, "label_ILA_images_for_getting_txt_symbols_areas");
	ReadProperty(m_locale_settings, m_cfg.m_label_use_ILA_images_before_clear_txt_images_from_borders, "label_use_ILA_images_before_clear_txt_images_from_borders");
	
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, "ssp_oim_property_validate_and_compare_cleared_txt_images");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, "ssp_oim_property_dont_delete_unrecognized_images_first");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, "ssp_oim_property_dont_delete_unrecognized_images_second");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_default_string_for_empty_sub, "ssp_oim_property_default_string_for_empty_sub");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, "ssp_oim_group_settings_for_multiframe_image_processing");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, "ssp_oim_sub_group_settings_for_sub_detection");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_threads, "ssp_oim_property_threads");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_sub_frames_length, "ssp_oim_property_sub_frames_length");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, "ssp_oim_sub_group_settings_for_comparing_subs");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_vedges_points_line_error, "ssp_oim_property_vedges_points_line_error");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_ila_points_line_error, "ssp_oim_property_ila_points_line_error");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, "ssp_oim_sub_group_settings_for_checking_sub");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_text_percent, "ssp_oim_property_text_percent");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_min_text_length, "ssp_oim_property_min_text_length");

	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_sub_group_settings_for_update_video_color, "ssp_oim_sub_group_settings_for_update_video_color");
	ReadProperty(m_locale_settings, m_cfg.m_label_video_contrast, "label_video_contrast");
	ReadProperty(m_locale_settings, m_cfg.m_label_video_gamma, "label_video_gamma");

	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, "ssp_oim_property_use_ISA_images_for_search_subtitles");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, "ssp_oim_property_use_ILA_images_for_search_subtitles");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, "ssp_oim_property_replace_ISA_by_filtered_version");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_max_dl_down, "ssp_oim_property_max_dl_down");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_max_dl_up, "ssp_oim_property_max_dl_up");

	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, "ssp_oim_property_use_gradient_images_for_clear_txt_images");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, "ssp_oim_property_clear_txt_images_by_main_color");
	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, "ssp_oim_property_use_ILA_images_for_clear_txt_images");

	ReadProperty(m_locale_settings, m_cfg.m_ssp_oim_property_remove_wide_symbols, "ssp_oim_property_remove_wide_symbols");

	ReadProperty(m_locale_settings, m_cfg.m_label_settings_file, "label_settings_file");
	ReadProperty(m_locale_settings, m_cfg.m_label_pixel_color, "label_pixel_color");

	ReadProperty(m_locale_settings, m_cfg.m_playback_sound, "label_playback_sound");

	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from m_locale_settings end.\n");

	SaveToReportLog("CMainFrame::LoadSettings(): finished.\n");
}

void CMainFrame::SaveSettings()
{
	wxFFileOutputStream ffout(m_GeneralSettingsFileName);
	wxTextOutputStream fout(ffout);

	m_pPanel->m_pSSPanel->m_pOI->SaveEditControlValue();
	m_pPanel->m_pSSPanel->m_pOIM->SaveEditControlValue();

	WriteProperty(fout, m_cfg.m_prefered_locale, "prefered_locale");

	WriteProperty(fout, m_cfg.process_affinity_mask, "process_affinity_mask");

	WriteProperty(fout, m_cfg.m_fount_size_lbl, "fount_size_lbl");
	WriteProperty(fout, m_cfg.m_fount_size_btn, "fount_size_btn");

	WriteProperty(fout, g_DontDeleteUnrecognizedImages1, "dont_delete_unrecognized_images1");
	WriteProperty(fout, g_DontDeleteUnrecognizedImages2, "dont_delete_unrecognized_images2");

	WriteProperty(fout, g_generate_cleared_text_images_on_test, "generate_cleared_text_images_on_test");
	WriteProperty(fout, g_show_results, "dump_debug_images");
	WriteProperty(fout, g_show_sf_results, "dump_debug_second_filtration_images");
	WriteProperty(fout, g_clear_test_images_folder, "clear_test_images_folder");
	WriteProperty(fout, g_show_transformed_images_only, "show_transformed_images_only");
	WriteProperty(fout, g_use_ocl, "use_ocl");
	WriteProperty(fout, g_use_cuda_gpu, "use_cuda_gpu");
	
	WriteProperty(fout, g_use_filter_color, "use_filter_color");
	WriteProperty(fout, g_use_outline_filter_color, "use_outline_filter_color");
	WriteProperty(fout, g_dL_color, "dL_color");
	WriteProperty(fout, g_dA_color, "dA_color");
	WriteProperty(fout, g_dB_color, "dB_color");
	WriteProperty(fout, g_combine_to_single_cluster, "combine_to_single_cluster");

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
	WriteProperty(fout, g_to, "text_centre_offset");
	WriteProperty(fout, g_scale, "image_scale_for_clear_image");

	WriteProperty(fout, g_use_ISA_images_for_get_txt_area, "use_ISA_images");
	WriteProperty(fout, g_use_ILA_images_for_get_txt_area, "use_ILA_images");

	WriteProperty(fout, g_use_ILA_images_for_getting_txt_symbols_areas, "use_ILA_images_for_getting_txt_symbols_areas");
	WriteProperty(fout, g_use_ILA_images_before_clear_txt_images_from_borders, "use_ILA_images_before_clear_txt_images_from_borders");

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
	WriteProperty(fout, g_tp, "text_percent");
	WriteProperty(fout, g_mtpl, "min_text_len_in_percent");
	WriteProperty(fout, g_veple, "vedges_points_line_error");
	WriteProperty(fout, g_ilaple, "ila_points_line_error");

	WriteProperty(fout, g_video_contrast, "video_contrast");
	WriteProperty(fout, g_video_gamma, "video_gamma");

	WriteProperty(fout, g_clear_txt_folders, "clear_txt_folders");
	WriteProperty(fout, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	WriteProperty(fout, g_clear_image_logical, "clear_image_logical");

	WriteProperty(fout, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	WriteProperty(fout, g_DefStringForEmptySub, "def_string_for_empty_sub");

	WriteProperty(fout, m_cfg.m_ocr_min_sub_duration, "min_sub_duration");
	WriteProperty(fout, m_cfg.m_ocr_join_txt_images_split_line, "ocr_join_txt_images_split_line");

	WriteProperty(fout, m_cfg.m_txt_dw, "txt_dw");
	WriteProperty(fout, m_cfg.m_txt_dy, "txt_dy");

	WriteProperty(fout, g_use_ISA_images_for_search_subtitles, "use_ISA_images_for_search_subtitles");
	WriteProperty(fout, g_use_ILA_images_for_search_subtitles, "use_ILA_images_for_search_subtitles");
	WriteProperty(fout, g_replace_ISA_by_filtered_version, "replace_ISA_by_filtered_version");
	WriteProperty(fout, g_max_dl_down, "max_dl_down");
	WriteProperty(fout, g_max_dl_up, "max_dl_up");

	WriteProperty(fout, g_remove_wide_symbols, "remove_wide_symbols");

	WriteProperty(fout, g_hw_device, "hw_device");

	wxString wxstr_val;
	(g_filter_descr == wxT("")) ? wxstr_val = wxT("none") : wxstr_val = g_filter_descr;
	WriteProperty(fout, wxstr_val, "filter_descr");

	WriteProperty(fout, g_text_alignment_string, "text_alignment");

	WriteProperty(fout, g_save_each_substring_separately, "save_each_substring_separately");
	WriteProperty(fout, g_save_scaled_images, "save_scaled_images");

	WriteProperty(fout, g_playback_sound, "playback_sound");

	WriteProperty(fout, g_border_is_darker, "border_is_darker");

	WriteProperty(fout, g_extend_by_grey_color, "extend_by_grey_color");
	WriteProperty(fout, g_allow_min_luminance, "allow_min_luminance");

	double double_val;

	double_val = 1 - m_pVideoBox->m_pVBox->m_pHSL2->m_pos;
	WriteProperty(fout, double_val, "bottom_video_image_percent_end");
	double_val = 1 - m_pVideoBox->m_pVBox->m_pHSL1->m_pos;
	WriteProperty(fout, double_val, "top_video_image_percent_end");
	
	WriteProperty(fout, m_pVideoBox->m_pVBox->m_pVSL1->m_pos, "left_video_image_percent_end");
	WriteProperty(fout, m_pVideoBox->m_pVBox->m_pVSL2->m_pos, "right_video_image_percent_end");

	fout.Flush();
	ffout.Close();
}

void CMainFrame::SaveError(wxString error)
{
	wxFFileOutputStream ffout(m_ErrorFileName, wxT("ab"));
	wxTextOutputStream fout(ffout);
	fout << error << '\n';
	fout.Flush();
	ffout.Close();
}

void CMainFrame::OnEditSetBeginTime(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		s64 Cur;
	
		Cur = m_pVideo->GetPos();

		m_pPanel->m_pSHPanel->m_plblBTA1->SetValue(ConvertVideoTime(Cur));

		m_BegTime = Cur;
	}
}

void CMainFrame::OnEditSetEndTime(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		s64 Cur;
	
		Cur = m_pVideo->GetPos();

		m_pPanel->m_pSHPanel->m_plblBTA2->SetValue(ConvertVideoTime(Cur));

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
		this->m_pPanel->m_pSSPanel->m_pGSFN->SetLabel(m_GeneralSettingsFileName + wxT(" "));

		LoadSettings();

		CControl::RefreshAllControlsData();
	}
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

		this->m_pPanel->m_pSSPanel->m_pGSFN->SetLabel(m_GeneralSettingsFileName + wxT(" "));

		SaveSettings();
	}
}

void CMainFrame::OnFileSaveSettings(wxCommandEvent& event)
{
	SaveSettings();
}

wxString CMainFrame::ConvertTime(u64 total_milliseconds)
{
	wxString str;
	int hour, min, sec, val = (int)(total_milliseconds/(u64)1000);

	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	str.Printf(wxT("%02dh:%02dm:%02ds"), hour, min, sec);

	return str;
}

void CMainFrame::OnTimer(wxTimerEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_mutex);
	s64 Cur;

	Cur = m_pVideo->GetPos();

	if (Cur != m_ct) 
	{
		if (g_RunSubSearch == 1)
		{
			if (Cur > m_BegTime)
			{
				std::chrono::time_point<std::chrono::high_resolution_clock> cur_time = std::chrono::high_resolution_clock::now();
				u64 run_time = std::chrono::duration_cast<std::chrono::milliseconds>(cur_time - g_StartTimeRunSubSearch).count();
				
				wxString str_progress, str_eta;

				if (m_EndTime >= 0)
				{
					double progress = std::min<double>(((double)(Cur - m_BegTime) / (double)(m_EndTime - m_BegTime)) * 100.0, 100.0);
					u64 eta = (u64)((double)run_time * (100.0 - progress) / progress);
					str_progress.Printf(wxT("%%%2.2f"), progress);
					str_eta = ConvertTime(eta);
				}
				else				
				{
					str_progress = wxT("%%N/A");
					str_eta = wxT("N/A");
				}

				wxString str;
				str.Printf(wxT("progress: %s eta : %s run_time : %s   |   "), str_progress, str_eta, ConvertTime(run_time));

				m_pVideoBox->m_plblTIME->SetLabel(str + ConvertVideoTime(Cur) + m_EndTimeStr + "   ");
			}			
		}
		else
		{
			m_pVideoBox->m_plblTIME->SetLabel(ConvertVideoTime(Cur) + m_EndTimeStr + "   ");
		}

		m_ct = Cur;
	}

	m_pVideoBox->m_pSB->SetScrollPos((int)Cur);

	//m_pVideoBox->m_pVBox->m_pHSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pHSL2->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL1->Refresh(true);
	//m_pVideoBox->m_pVBox->m_pVSL2->Refresh(true);
}

wxString VideoTimeToStr2(s64 pos)
{
	wxString str;
	int hour, min, sec, msec, val;

	val = (int)(pos / 1000); // seconds
	msec = pos - (s64)val * 1000;
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	str.Printf(wxT("%02d:%02d:%02d,%03d"), hour, min, sec, msec);

	return str;
}

wxString VideoTimeToStr3(s64 pos)
{
	wxString str;
	int hour, min, sec, sec_100, val;

	val = (int)(pos / 1000); // seconds
	sec_100 = (pos - (s64)val * 1000)/10;
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	str.Printf(wxT("%.1d:%.2d:%.2d.%.2d"), hour, min, sec, sec_100);

	return str;
}

wxString ConvertVideoTime(s64 pos)
{
	wxString str;
	int hour, min, sec, msec, val;

	if (pos >= 0)
	{
		val = (int)(pos / 1000); // seconds
		msec = pos - (s64)val * 1000;
		hour = val / 3600;
		val -= hour * 3600;
		min = val / 60;
		val -= min * 60;
		sec = val;

		str.Printf(wxT("%02d:%02d:%02d:%03d"), hour, min, sec, msec);
	}
	else
	{
		str = wxT("N/A");
	}

	return str;
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

	if ( (g_IsSearching == 0) && (m_FileName.size() > 0) )
	{
		wxString pvi_path = g_work_dir + wxT("/previous_video.inf");
		wxFFileOutputStream ffout(pvi_path);
		wxTextOutputStream fout(ffout);
		fout <<	m_FileName << '\n';

		fout <<	(int)m_BegTime << '\n';

		fout <<	(int)m_EndTime << '\n';

		fout <<	m_type << '\n';

		fout.Flush();
		ffout.Close();
	}

	if (g_IsSearching == 1)
	{
		g_IsClose = 1;
		g_RunSubSearch = 0;
		//m_pPanel->m_pSHPanel->m_pSearchThread->SetPriority(90); //THREAD_PRIORITY_HIGHEST
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> start_t = std::chrono::high_resolution_clock::now();

	while( ((int)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_t).count()) < 2000) && ( (g_IsSearching == 1) || (g_IsCreateClearedTextImages == 1) ) ){}

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
	wxString str;
	wxString pvi_path = g_work_dir + wxT("/previous_video.inf");
	wxFileInputStream ffin(pvi_path);

	if (ffin.IsOk())
	{
		wxTextInputStream fin(ffin, wxT("\x09"), wxConvUTF8);
		
		m_FileName = fin.ReadLine();

		str = fin.ReadLine();
		m_BegTime = (s64)wxAtoi(str);

		str = fin.ReadLine();
		m_EndTime = (s64)wxAtoi(str);

		str = fin.ReadLine();
		m_type = (s64)wxAtoi(str);

		m_blnReopenVideo = true;

		OnFileOpenVideo(m_type);
	}
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
        if ( (filename != wxT(".")) && 
			 (filename != wxT("..")) )
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

void CMainFrame::ShowErrorMessage(wxString msg)
{
	wxMessageBox(msg, wxT("Error Info"), wxOK | wxICON_ERROR);
}

void CMainFrame::OnAppAbout(wxCommandEvent& event)
{
	wxSize cl_size = this->GetClientSize();
	wxSize msg_size(600, 600);
	MyMessageBox msg_dlg(this,
		"This program was written by Simeon Kosnitsky. \nPublished under public domain license.\n\nSupported command line options:\n" + g_parser.GetUsageString(),
		"VideoSubFinder " VSF_VERSION " Version",
		wxPoint((cl_size.x - msg_size.x) / 2, (cl_size.y - msg_size.y) / 2),
		msg_size);
	msg_dlg.ShowModal();
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

void WriteProperty(wxTextOutputStream& fout, int val, wxString Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(wxTextOutputStream& fout, bool val, wxString Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(wxTextOutputStream& fout, double val, wxString Name)
{
	fout << Name << " = " << val << '\n';
}

void WriteProperty(wxTextOutputStream& fout, wxString val, wxString Name)
{
	val = wxJoin(wxSplit(val, '\n'), ';');
	fout << Name << " = " << val << '\n';
}

void WriteProperty(wxTextOutputStream& fout, wxArrayString val, wxString Name)
{
	if (val.size() > 0)
	{
		fout << Name << " = " << wxJoin(val, ';') << '\n';
	}
	else
	{
		fout << Name << " = none\n";
	}
}

bool ReadProperty(std::map<wxString, wxString>& settings, int& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		wxString _val = search->second;
		val = wxAtoi(_val);
		res = true;
	}

	return res;
}

bool ReadProperty(std::map<wxString, wxString>& settings, bool& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		wxString _val = search->second;
		int get_val = wxAtoi(_val);
		if (get_val != 0)
		{
			val = true;
		}
		else
		{
			val = false;
		}
		res = true;
	}

	return res;
}

bool ReadProperty(std::map<wxString, wxString>& settings, double& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		wxString _val = search->second;
		_val.ToDouble(&val);
		res = true;
	}

	return res;
}

bool ReadProperty(std::map<wxString, wxString>& settings, wxString& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		val = search->second;
		val = wxJoin(wxSplit(val, ';'), '\n');		
		res = true;
	}

	return res;
}

bool ReadProperty(std::map<wxString, wxString>& settings, wxArrayString& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		if (search->second == "none") {
			val.clear();
		}
		else {
			val = wxSplit(search->second, ';');
		}
		res = true;
	}

	return res;
}