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
#include <wx/fontenum.h>
#include <wx/textwrapper.h>
#include <chrono>
#include "Control.h"

CMainFrame *g_pMF;

bool g_playback_sound = false;
Settings g_cfg;
std::map<wxString, wxString> g_general_settings;
std::map<wxString, wxString> g_locale_settings;

wxString g_text_alignment_string;

wxArrayString g_localizations;
std::map<wxString, int> g_localization_id;
std::map<int, wxString> g_id_localization;

const int g_max_font_size = 80;
const int g_def_main_text_font_size = 10;
const int g_def_buttons_text_font_size = 13;

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

void LoadLocaleSettings(wxString settings_path);
void UpdateSettingsInFile(wxString SettingsFilePath, std::map<wxString, wxString> settings);

/////////////////////////////////////////////////////////////////////////////

wxArrayString GetAvailableLocalizations()
{
	wxArrayString localizations;
	wxString settings_dir = g_app_dir + wxT("/settings/");

	wxDir dir(settings_dir);
	wxString filename;
	bool res;

	res = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
	while (res)
	{
		if (wxFileExists(settings_dir + filename + wxT("/locale.cfg")))
		{
			localizations.Add(filename);
		}		
		res = dir.GetNext(&filename);
	}

	for (int i = 0; i < (int)localizations.size() - 1; i++)
	for (int j = i + 1; j < (int)localizations.size(); j++)
	{
		if (localizations[i] > localizations[j])
		{
			filename = localizations[i];
			localizations[i] = localizations[j];
			localizations[j] = filename;
		}
	}

	for (int i = 0; i < (int)localizations.size(); i++)
	{
		g_localization_id[localizations[i]] = FIRST_ID_FOR_LOCALIZATIONS + i;
		g_id_localization[FIRST_ID_FOR_LOCALIZATIONS + i] = localizations[i];
	}

	return localizations;
}
 
/////////////////////////////////////////////////////////////////////////////

wxString ConvertTextAlignmentToString(TextAlignment val)
{
	wxString res;
	switch (val)
	{
	case TextAlignment::Center:
		res = g_cfg.m_text_alignment_center;
		break;
	case TextAlignment::Left:
		res = g_cfg.m_text_alignment_left;
		break;
	case TextAlignment::Right:
		res = g_cfg.m_text_alignment_right;
		break;
	case TextAlignment::Any:
		res = g_cfg.m_text_alignment_any;
		break;
	}

	return res;
}

/////////////////////////////////////////////////////////////////////////////

TextAlignment ConvertStringToTextAlignment(wxString val)
{
	TextAlignment res;
	if (val == ConvertTextAlignmentToString(TextAlignment::Center))
	{
		res = TextAlignment::Center;
	}
	else if (val == ConvertTextAlignmentToString(TextAlignment::Left))
	{
		res = TextAlignment::Left;
	}
	else if (val == ConvertTextAlignmentToString(TextAlignment::Right))
	{
		res = TextAlignment::Right;
	}
	else if (val == ConvertTextAlignmentToString(TextAlignment::Any))
	{
		res = TextAlignment::Any;
	}
	return res;
}

/////////////////////////////////////////////////////////////////////////////

#ifdef WIN32	
int exception_filter(unsigned int code, struct _EXCEPTION_POINTERS *ep, char *det)
{
	SaveError(wxT("Got C Exception: ") + wxString(det) + wxT("\n"));
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
	EVT_MENU(ID_APP_CMD_ARGS_INFO, CMainFrame::OnAppCMDArgsInfo)
	EVT_MENU(ID_APP_USAGE_DOCS, CMainFrame::OnAppUsageDocs)
	EVT_MENU(ID_APP_WEBSITE, CMainFrame::OnAppOpenLink)
	EVT_MENU(ID_APP_FORUM, CMainFrame::OnAppOpenLink)
	EVT_MENU(ID_APP_BUG_TRACKER, CMainFrame::OnAppOpenLink)
	EVT_MENU(ID_APP_ABOUT, CMainFrame::OnAppAbout)	
	EVT_MENU(ID_FONTS, CMainFrame::OnFonts)
	EVT_MENU(ID_SCALE_TEXT_SIZE_INC, CMainFrame::OnScaleTextSizeInc)
	EVT_MENU(ID_SCALE_TEXT_SIZE_DEC, CMainFrame::OnScaleTextSizeDec)
	EVT_MENU(ID_NEXT_FRAME, CMainFrame::OnNextFrame)
	EVT_MENU(ID_PREVIOUS_FRAME, CMainFrame::OnPreviousFrame)
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
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::SetFonts()
{
	if (g_cfg.m_main_text_font_size == -1)
	{
		g_cfg.m_main_text_font_size = g_def_main_text_font_size;
	}
	if (g_cfg.m_buttons_text_font_size == -1)
	{
		g_cfg.m_buttons_text_font_size = g_def_buttons_text_font_size;
	}

	// https://docs.wxwidgets.org/stable/interface_2wx_2font_8h.html#a0cd7bfd21a4f901245d3c86d8ea0c080
	// wxFONTFAMILY_SWISS / A sans - serif font.

	if (g_cfg.m_main_text_font == wxT("default"))
	{
		SaveToReportLog("CMainFrame::Init(): init m_LBLFont...\n");
		m_LBLFont = wxFont(g_cfg.m_main_text_font_size,
			wxFONTFAMILY_SWISS,
			g_cfg.m_main_text_font_italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
			g_cfg.m_main_text_font_bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
			g_cfg.m_main_text_font_underline,
			wxEmptyString,
			wxFONTENCODING_DEFAULT);
	}
	else
	{
		m_LBLFont = wxFont(wxFontInfo(g_cfg.m_main_text_font_size).FaceName(g_cfg.m_main_text_font).Italic(g_cfg.m_main_text_font_italic).Bold(g_cfg.m_main_text_font_bold).Underlined(g_cfg.m_main_text_font_underline));
	}

	if (g_cfg.m_buttons_text_font == wxT("default"))
	{
		SaveToReportLog("CMainFrame::Init(): init m_BTNFont...\n");
		m_BTNFont = wxFont(g_cfg.m_buttons_text_font_size,
			wxFONTFAMILY_SWISS,
			g_cfg.m_buttons_text_font_italic ? wxFONTSTYLE_ITALIC : wxFONTSTYLE_NORMAL,
			g_cfg.m_buttons_text_font_bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL,
			g_cfg.m_buttons_text_font_underline,
			wxEmptyString,
			wxFONTENCODING_DEFAULT);
	}
	else
	{
		m_BTNFont = wxFont(wxFontInfo(g_cfg.m_buttons_text_font_size).FaceName(g_cfg.m_buttons_text_font).Italic(g_cfg.m_buttons_text_font_italic).Bold(g_cfg.m_buttons_text_font_bold).Underlined(g_cfg.m_buttons_text_font_underline));
	}
}

wxMenuBar* CMainFrame::CreateMenuBar()
{
	SaveToReportLog("CMainFrame::CreateMenuBar(): starting ...\n");

	wxMenuBar* pMenuBar = new wxMenuBar;	

	wxMenu* pMenu5 = new wxMenu;

	pMenu5->Append(ID_SETPRIORITY_HIGH, g_cfg.m_menu_setpriority_high, _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_ABOVENORMAL, g_cfg.m_menu_setpriority_abovenormal, _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_NORMAL, g_cfg.m_menu_setpriority_normal, _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_BELOWNORMAL, g_cfg.m_menu_setpriority_belownormal, _T(""), wxITEM_CHECK);
	pMenu5->Append(ID_SETPRIORITY_IDLE, g_cfg.m_menu_setpriority_idle, _T(""), wxITEM_CHECK);
	pMenu5->Check(ID_SETPRIORITY_NORMAL, true);

	wxMenu* pMenu1 = new wxMenu;
	pMenu1->Append(ID_FILE_OPEN_VIDEO_OPENCV, g_cfg.m_menu_file_open_video_opencv);
	pMenu1->Append(ID_FILE_OPEN_VIDEO_FFMPEG, g_cfg.m_menu_file_open_video_ffmpeg);
	pMenu1->Append(ID_FILE_REOPENVIDEO, g_cfg.m_menu_file_reopenvideo);
	pMenu1->Append(ID_FILE_OPENPREVIOUSVIDEO, g_cfg.m_menu_file_openpreviousvideo);
	pMenu1->AppendSeparator();
	pMenu1->AppendSubMenu(pMenu5, g_cfg.m_menu_setpriority);
	pMenu1->AppendSeparator();
	pMenu1->Append(ID_FILE_SAVESETTINGS, g_cfg.m_menu_file_savesettings + wxT("\tCtrl+S"));
	pMenu1->Append(ID_FILE_SAVESETTINGSAS, g_cfg.m_menu_file_savesettingsas);
	pMenu1->Append(ID_FILE_LOADSETTINGS, g_cfg.m_menu_file_loadsettings);
	pMenu1->AppendSeparator();
	pMenu1->Append(ID_FILE_EXIT, g_cfg.m_menu_file_exit);
	pMenuBar->Append(pMenu1, g_cfg.m_menu_file);

	wxMenu* pMenu2 = new wxMenu;
	pMenu2->Append(ID_EDIT_SETBEGINTIME, g_cfg.m_menu_edit_setbegintime + wxT("\tCtrl+Z"));
	pMenu2->Append(ID_EDIT_SETENDTIME, g_cfg.m_menu_edit_setendtime + wxT("\tCtrl+X"));
	pMenuBar->Append(pMenu2, g_cfg.m_menu_edit);

	wxMenu* pMenuLocalization = new wxMenu;
	g_localizations = GetAvailableLocalizations();

	for (wxString localization : g_localizations)
	{
		pMenuLocalization->Append(g_localization_id[localization], localization, _T(""), wxITEM_CHECK);
		this->Bind(wxEVT_MENU, &CMainFrame::OnLocalization, this, g_localization_id[localization]);
	}
	pMenuLocalization->Check(g_localization_id[g_cfg.m_prefered_locale], true);

	wxMenu* pMenuView = new wxMenu;
	pMenuView->AppendSubMenu(pMenuLocalization, g_cfg.m_menu_localization);
	pMenuView->AppendSeparator();
	pMenuView->Append(ID_FONTS, g_cfg.m_menu_fonts);
	pMenuView->AppendSeparator();
	pMenuView->Append(ID_SCALE_TEXT_SIZE_INC, g_cfg.m_menu_scale_text_size_inc + wxT("   Ctrl+Mouse Wheel"));
	pMenuView->Append(ID_SCALE_TEXT_SIZE_DEC, g_cfg.m_menu_scale_text_size_dec + wxT("   Ctrl+Mouse Wheel"));
	pMenuBar->Append(pMenuView, g_cfg.m_menu_view);

	wxMenu* pMenu3 = new wxMenu;

	pMenu3->Append(ID_PLAY_PAUSE, g_cfg.m_menu_play_pause + wxT("   Space"));
	pMenu3->Append(ID_PLAY_STOP, g_cfg.m_menu_play_stop);
	pMenu3->Append(ID_NEXT_FRAME, g_cfg.m_menu_next_frame + wxT("   Mouse Wheel / Right"));
	pMenu3->Append(ID_PREVIOUS_FRAME, g_cfg.m_menu_previous_frame + wxT("   Mouse Wheel / Left"));
	pMenuBar->Append(pMenu3, g_cfg.m_menu_play);

	wxMenu* pMenu4 = new wxMenu;
	pMenu4->Append(ID_APP_CMD_ARGS_INFO, g_cfg.m_menu_app_cmd_args_info);
	pMenu4->AppendSeparator();
	pMenu4->Append(ID_APP_USAGE_DOCS, g_cfg.m_menu_app_usage_docs);
	pMenu4->AppendSeparator();
	pMenu4->Append(ID_APP_WEBSITE, g_cfg.m_menu_app_website);
	pMenu4->Append(ID_APP_FORUM, g_cfg.m_menu_app_forum);
	pMenu4->Append(ID_APP_BUG_TRACKER, g_cfg.m_menu_app_bug_tracker);
	pMenu4->AppendSeparator();
	pMenu4->Append(ID_APP_ABOUT, g_cfg.m_menu_app_about + wxT("\tF1"));
	pMenuBar->Append(pMenu4, g_cfg.m_menu_help);

	pMenuBar->SetFont(m_LBLFont);

	for (wxString localization : g_localizations)
	{
		int loc_id = g_localization_id[localization];

		if (g_cfg.m_prefered_locale == localization)
		{
			pMenuBar->Check(loc_id, true);
		}
		else
		{
			pMenuBar->Check(loc_id, false);
		}
	}

	SaveToReportLog("CMainFrame::CreateMenuBar(): end.\n");

	return pMenuBar;
}

void CMainFrame::Init()
{
	SaveToReportLog("CMainFrame::Init(): starting...\n");

	m_blnNoGUI = false;

	SaveToReportLog("CMainFrame::Init(): InitCUDADevice...\n");

	if (!InitCUDADevice())
	{
		g_use_cuda_gpu = false;
	}	

	// CSeparatingLine *pHSL = new CSeparatingLine(this, 200, 3, 7, 3, 100, 110, 50, 0);
	// pHSL->m_pos = 0;
	// pHSL->Raise();
	// return;	

	this->SetBackgroundColour(g_cfg.m_main_frame_background_colour);

	SetFonts();

	SaveToReportLog("CMainFrame::Init(): CreateMenuBar() ...\n");
	wxMenuBar* pMenuBar = CreateMenuBar();

	SaveToReportLog("CMainFrame::Init(): this->SetMenuBar(pMenuBar)...\n");
	this->SetMenuBar(pMenuBar);

	SaveToReportLog("CMainFrame::Init(): new CImageBox(this)...\n");
	m_pImageBox = new CImageBox(this);

	SaveToReportLog("CMainFrame::Init(): m_pImageBox->Init()...\n");
	m_pImageBox->Init();

	int w = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
	int h = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
	
	SaveToReportLog("CMainFrame::Init(): new CVideoBox(this)...\n");
	m_pVideoBox = new CVideoBox(this);

	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->Init()...\n");
	m_pVideoBox->Init();
	SaveToReportLog("CMainFrame::Init(): m_pVideoBox->Bind...\n");

#ifdef WIN32
	if (g_cfg.process_affinity_mask > 0)
	{
		HANDLE process = GetCurrentProcess();
		DWORD_PTR dwProcessAffinityMask, dwSystemAffinityMask;

		GetProcessAffinityMask(process, &dwProcessAffinityMask, &dwSystemAffinityMask);

		dwProcessAffinityMask = (DWORD_PTR)g_cfg.process_affinity_mask & dwSystemAffinityMask;
		BOOL success = SetProcessAffinityMask(process, dwProcessAffinityMask);

		SaveToReportLog(wxString::Format(wxT("CMainFrame::Init(): SetProcessAffinityMask(%d) == %d\n"), (int)dwProcessAffinityMask, (int)success));
	}
#endif

	SaveToReportLog("CMainFrame::Init(): new CSSOWnd(this)...\n");
	m_pPanel = new CSSOWnd(this);	

	SaveToReportLog("CMainFrame::Init(): this->SetSize(..)...\n");

#ifdef WIN32
	this->SetSize(0, 0, w, h - 50);
#else
	this->SetSize(w / 16, h / 14, (7 * w) / 8, (6 * h) / 7);
#endif

	if (g_cfg.m_vsf_is_maximized)
	{
		this->Maximize();
	}
	else if ((g_cfg.m_vsf_x >= 0) &&
		(g_cfg.m_vsf_y >= 0) &&
		(g_cfg.m_vsf_w > 0) &&
		(g_cfg.m_vsf_h > 0) && 
		(g_cfg.m_vsf_x + g_cfg.m_vsf_w <= w) &&
		(g_cfg.m_vsf_y + g_cfg.m_vsf_h <= h) )
	{
		this->SetSize(g_cfg.m_vsf_x, g_cfg.m_vsf_y, g_cfg.m_vsf_w, g_cfg.m_vsf_h);
	}

	SaveToReportLog("CMainFrame::Init(): m_pPanel->Init()...\n");
	m_pPanel->Init();

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
		
	UpdateDynamicSettings();
	CControl::RefreshAllControlsData();
	CControl::UpdateAllControlsSize();
	this->Refresh();

	this->Bind(VIEW_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewImageInImageBox, this);
	this->Bind(VIEW_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewImageInVideoBox, this);
	this->Bind(VIEW_GREYSCALE_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewGreyscaleImageInImageBox, this);
	this->Bind(VIEW_GREYSCALE_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewGreyscaleImageInVideoBox, this);
	this->Bind(VIEW_BGR_IMAGE_IN_IMAGE_BOX, &CMainFrame::OnViewBGRImageInImageBox, this);
	this->Bind(VIEW_BGR_IMAGE_IN_VIDEO_BOX, &CMainFrame::OnViewBGRImageInVideoBox, this);
	this->Bind(VIEW_RGB_IMAGE, &CMainFrame::OnViewRGBImage, this);

	if (wxFileExists(g_prev_data_path))
	{
		std::map<wxString, wxString> previous_vido_settings;
		ReadSettings(g_prev_data_path, previous_vido_settings);
		ReadProperty(previous_vido_settings, m_last_video_file_path, "last_video_file_path");
		ReadProperty(previous_vido_settings, m_last_video_begin_time, "last_video_begin_time");
		ReadProperty(previous_vido_settings, m_last_video_end_time, "last_video_end_time");
		ReadProperty(previous_vido_settings, m_last_video_open_type, "last_video_open_type");
		ReadProperty(previous_vido_settings, m_last_saved_sub_file_path, "last_saved_sub_file_path");
		ReadProperty(previous_vido_settings, m_last_specified_settings_file_path, "last_specified_settings_file_path");
	}

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

void CMainFrame::OnScaleTextSizeInc(wxCommandEvent& event)
{
	UpdateTextSizes(1);
}

void CMainFrame::OnScaleTextSizeDec(wxCommandEvent& event)
{
	UpdateTextSizes(-1);
}

void CMainFrame::OnMouseWheel(wxMouseEvent& event)
{
	if (wxGetKeyState(WXK_CONTROL))
	{
		if (event.m_wheelRotation > 0)
		{
			UpdateTextSizes(1);
		}
		else
		{
			UpdateTextSizes(-1);
		}
	}
}

void CMainFrame::ScaleTextSize(int& size, int dsize)
{
	size += dsize;

	if (size > g_max_font_size)
	{
		size = g_max_font_size;
	}

	if (size < 1)
	{
		size = 1;
	}
}

void CMainFrame::UpdateTextSizes(int dsize)
{
	if (dsize != 0)
	{
		ScaleTextSize(g_cfg.m_main_text_font_size, dsize);
		ScaleTextSize(g_cfg.m_buttons_text_font_size, dsize);
	}

	SetFonts();
	
	SaveToReportLog("CMainFrame::UpdateTextSizes(): CreateMenuBar() ...\n");
	wxMenuBar* pMenuBar = CreateMenuBar();	

	SaveToReportLog("CMainFrame::UpdateTextSizes(): this->SetMenuBar(pMenuBar) ...\n");
	this->SetMenuBar(pMenuBar);

	CControl::RefreshAllControlsData();
	CControl::UpdateAllControlsSize();
	this->Refresh();
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
	
	if (m_pImageBox)
	{
		m_pImageBox->SetSize(cw / 2 + m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
		m_pImageBox->Raise();
		m_pImageBox->Refresh();
	}

	if (m_pVideoBox)
	{
		m_pVideoBox->SetSize(m_dx, m_dy, cw / 2 - 2 * m_dx, ch - m_ph - 2 * m_dy);
		m_pVideoBox->Raise();
		m_pVideoBox->Refresh();
	}

	CControl::UpdateAllControlsSize();
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

void CMainFrame::get_video_box_lblTIME_run_search_label()
{
	if (g_cfg.m_run_search_str_progress.size() > 0)
	{
		wxString str;
		str.Printf(g_cfg.m_run_search_progress_format_string, g_cfg.m_run_search_str_progress, g_cfg.m_run_search_str_eta, g_cfg.m_run_search_run_time, g_cfg.m_run_search_cur_time, m_EndTimeStr);
		g_cfg.m_video_box_lblTIME_label = str + wxT("   ");
	}
	else
	{
		g_cfg.m_video_box_lblTIME_label = wxT("00:00:00,000/00:00:00,000   ");
	}
}

void CMainFrame::get_video_box_lblVB_open_video_title()
{
	if (m_FileName.size() > 0)
	{
		g_cfg.m_video_box_lblVB_title = g_cfg.m_video_box_title + wxT(" \"") + GetFileName(m_FileName) + wxT("\"");
	}
	else
	{
		g_cfg.m_video_box_lblVB_title = wxT("");
	}
}

void CMainFrame::get_video_box_lblVB_on_test_title()
{
	if (g_cfg.m_on_test_image_name.size() > 0)
	{
		g_cfg.m_video_box_lblVB_title = g_cfg.m_video_box_title + wxT(" \"") + g_cfg.m_on_test_image_name + wxT("\"");
	}
	else
	{
		g_cfg.m_video_box_lblVB_title = wxT("");
	}
}

void CMainFrame::get_available_text_alignments()
{
	wxArrayString res;
	res.Add(ConvertTextAlignmentToString(TextAlignment::Center));
	res.Add(ConvertTextAlignmentToString(TextAlignment::Left));
	res.Add(ConvertTextAlignmentToString(TextAlignment::Right));
	res.Add(ConvertTextAlignmentToString(TextAlignment::Any));
	g_cfg.m_available_text_alignments = res;
}

void CMainFrame::get_StrFN()
{
	g_cfg.m_StrFN.resize(5);
	g_cfg.m_StrFN[0] = g_cfg.m_test_result_after_first_filtration_label;
	g_cfg.m_StrFN[1] = g_cfg.m_test_result_after_second_filtration_label;
	g_cfg.m_StrFN[2] = g_cfg.m_test_result_after_third_filtration_label;
	g_cfg.m_StrFN[3] = g_cfg.m_test_result_nedges_points_image_label;
	g_cfg.m_StrFN[4] = g_cfg.m_test_result_cleared_text_image_label;
}

void CMainFrame::UpdateDynamicSettings()
{
	get_video_box_lblTIME_run_search_label();
	get_video_box_lblVB_open_video_title();
	get_video_box_lblVB_on_test_title();
	get_available_text_alignments();
	get_StrFN();
	if (g_pParser != NULL)
	{
		delete g_pParser;
	}
	g_pParser = new wxCmdLineParser();
	SetParserDescription();
	g_pParser->SetCmdLine(wxTheApp->argc, wxTheApp->argv);
	if (g_pParser->Parse() != 0)
	{
		exit(0);
	}
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

		wxString video_file_dir = (m_FileName.size() > 0) ? GetFileDir(m_FileName) : ((m_last_video_file_path.size() > 0) ? GetFileDir(m_last_video_file_path) : wxString(wxEmptyString));
		wxString video_file_name = (m_FileName.size() > 0) ? GetFileNameWithExtension(m_FileName) : ((m_last_video_file_path.size() > 0) ? GetFileNameWithExtension(m_last_video_file_path) : wxString(wxEmptyString));

		wxFileDialog fd(this, g_cfg.m_file_dialog_title_open_video_file,
			video_file_dir, video_file_name, wxString::Format(g_cfg.m_file_dialog_title_open_video_file_wild_card, all_video_formats, all_video_formats), wxFD_OPEN);

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
		m_pVideoBox->m_plblVB->SetLabel(g_cfg.m_video_box_title);
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

	get_video_box_lblVB_open_video_title();
	m_pVideoBox->m_plblVB->SetLabel(g_cfg.m_video_box_lblVB_title);

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
	m_pVideoBox->m_pButtonPause->Enable();
	m_pVideoBox->m_pButtonRun->Enable();
	m_pVideoBox->m_pButtonStop->Enable();
	m_pVideoBox->m_pButtonPause->Hide();
	m_pVideoBox->m_pButtonRun->Show();

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
	const std::lock_guard<std::mutex> lock(m_play_mutex);

	if (m_VIsOpen)	
	{
		if (m_vs == Play)
		{
			m_vs = Pause;
			m_pVideo->Pause();

			m_pVideoBox->m_pButtonPause->Hide();
			m_pVideoBox->m_pButtonRun->Show();
		}
		else
		{
			m_vs = Play;
			m_pVideo->Run();
			m_pVideoBox->m_pButtonRun->Hide();
			m_pVideoBox->m_pButtonPause->Show();
		}
	}
}

void CMainFrame::OnStop(wxCommandEvent& event)
{
	const std::lock_guard<std::mutex> lock(m_play_mutex);

	if (m_VIsOpen)	
	{
		if (m_vs != Stop)
		{
			m_vs = Stop;
			m_pVideo->StopFast();
			m_pVideoBox->m_pButtonPause->Hide();
			m_pVideoBox->m_pButtonRun->Show();
		}
	}
}

void CMainFrame::OnLocalization(wxCommandEvent& event)
{
	SaveToReportLog("CMainFrame::OnLocalization(): starting ...\n");

	int selected_loc_id = event.GetId();
	g_cfg.m_prefered_locale = g_id_localization[selected_loc_id];

	SaveToReportLog(wxString::Format(wxT("CMainFrame::OnLocalization(): initializing locale \"%s\" ...\n"), g_cfg.m_prefered_locale));

	LoadLocaleSettings(g_app_dir + wxT("/settings/") + g_cfg.m_prefered_locale + wxT("/locale.cfg"));

	//Not required, SetMenuBar automaticly remove previous
	//SaveToReportLog("CMainFrame::OnLocalization(): deleting current Menu Bar ...\n");
	//this->GetMenuBar()->Destroy();	

	SaveToReportLog("CMainFrame::OnLocalization(): CreateMenuBar() ...\n");
	wxMenuBar* pMenuBar = CreateMenuBar();	

	SaveToReportLog("CMainFrame::OnLocalization(): this->SetMenuBar(pMenuBar) ...\n");
	this->SetMenuBar(pMenuBar);

	UpdateDynamicSettings();
	CControl::RefreshAllControlsData();
	CControl::UpdateAllControlsSize();
	this->Refresh();

	UpdateSettingsInFile(g_GeneralSettingsFileName, std::map<wxString, wxString>{ {wxString(wxT("prefered_locale")), g_cfg.m_prefered_locale} });
}

void CMainFrame::OnNextFrame(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		PauseVideo();
		m_pVideo->OneStep();
	}
}

void CMainFrame::OnPreviousFrame(wxCommandEvent& event)
{
	if (m_VIsOpen)
	{
		s64 Cur;
		PauseVideo();
		Cur = m_pVideo->GetPos();
		Cur -= m_dt;
		if (Cur < 0) Cur = 0;
		m_pVideo->SetPosFast(Cur);
	}
}

void CMainFrame::PauseVideo()
{
	const std::lock_guard<std::mutex> lock(m_play_mutex);

	if (m_VIsOpen && (m_vs != Pause))	
	{
		m_vs = Pause;
		m_pVideo->Pause();
		m_pVideoBox->m_pButtonPause->Hide();
		m_pVideoBox->m_pButtonRun->Show();
	}
}

void ReadSettings(wxString file_name, std::map<wxString, wxString>& settings)
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

void LoadSettings()
{
	SaveToReportLog("CMainFrame::LoadSettings(): starting...\n");

	SaveToReportLog("CMainFrame::LoadSettings(): ReadSettings(g_GeneralSettingsFileName, g_general_settings)...\n");
	ReadSettings(g_GeneralSettingsFileName, g_general_settings);
	
	if (g_pParser != NULL)
	{
		SaveToReportLog("CMainFrame::LoadSettings(): Updating g_general_settings according command line options...\n");

		wxString val;

		for (const std::map<wxString, wxString>::value_type& pair : g_general_settings)
		{
			if (g_pParser->Found(pair.first, &val))
			{
				g_general_settings[pair.first] = val;
			}
		}
	}

	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from g_general_settings...\n");

	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_join_rgb_images, "ocr_join_images_join_rgb_images");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_use_txt_images_data_for_join_rgb_images, "ocr_join_images_use_txt_images_data_for_join_rgb_images");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_clear_dir, "ocr_join_images_clear_dir");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_split_line, "ocr_join_images_split_line");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_split_line_font_size, "ocr_join_images_split_line_font_size");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_split_line_font_bold, "ocr_join_images_split_line_font_bold");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_sub_id_format, "ocr_join_images_sub_id_format");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_sub_search_by_id_format, "ocr_join_images_sub_search_by_id_format");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_scale, "ocr_join_images_scale");
	ReadProperty(g_general_settings, g_cfg.m_ocr_join_images_max_number, "ocr_join_images_max_number");

	ReadProperty(g_general_settings, g_cfg.m_vsf_is_maximized, "vsf_is_maximized");
	ReadProperty(g_general_settings, g_cfg.m_vsf_x, "vsf_x");
	ReadProperty(g_general_settings, g_cfg.m_vsf_y, "vsf_y");
	ReadProperty(g_general_settings, g_cfg.m_vsf_w, "vsf_w");
	ReadProperty(g_general_settings, g_cfg.m_vsf_h, "vsf_h");

	ReadProperty(g_general_settings, g_cfg.m_main_text_colour, "main_text_colour");
	ReadProperty(g_general_settings, g_cfg.m_main_text_ctls_background_colour, "main_text_ctls_background_colour");
	ReadProperty(g_general_settings, g_cfg.m_main_buttons_colour, "main_buttons_colour");
	ReadProperty(g_general_settings, g_cfg.m_main_buttons_colour_focused, "main_buttons_colour_focused");
	ReadProperty(g_general_settings, g_cfg.m_main_buttons_colour_selected, "main_buttons_colour_selected");
	ReadProperty(g_general_settings, g_cfg.m_main_buttons_border_colour, "main_buttons_border_colour");
	ReadProperty(g_general_settings, g_cfg.m_main_labels_background_colour, "main_labels_background_colour");
	ReadProperty(g_general_settings, g_cfg.m_main_frame_background_colour, "main_frame_background_colour");
	ReadProperty(g_general_settings, g_cfg.m_notebook_colour, "notebook_colour");
	ReadProperty(g_general_settings, g_cfg.m_notebook_panels_colour, "notebook_panels_colour");
	ReadProperty(g_general_settings, g_cfg.m_grid_line_colour, "grid_line_colour");
	ReadProperty(g_general_settings, g_cfg.m_grid_gropes_colour, "grid_gropes_colour");
	ReadProperty(g_general_settings, g_cfg.m_grid_sub_gropes_colour, "grid_sub_gropes_colour");
	ReadProperty(g_general_settings, g_cfg.m_grid_debug_settings_colour, "grid_debug_settings_colour");
	ReadProperty(g_general_settings, g_cfg.m_test_result_label_colour, "test_result_label_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_image_box_background_colour, "video_image_box_background_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_image_box_border_colour, "video_image_box_border_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_image_box_title_colour, "video_image_box_title_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_box_time_colour, "video_box_time_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_box_time_text_colour, "video_box_time_text_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_box_separating_line_colour, "video_box_separating_line_colour");
	ReadProperty(g_general_settings, g_cfg.m_video_box_separating_line_border_colour, "video_box_separating_line_border_colour");
	ReadProperty(g_general_settings, g_cfg.m_toolbar_bitmaps_transparent_colour, "toolbar_bitmaps_transparent_colour");

	ReadProperty(g_general_settings, g_DontDeleteUnrecognizedImages1, "dont_delete_unrecognized_images1");
	ReadProperty(g_general_settings, g_DontDeleteUnrecognizedImages2, "dont_delete_unrecognized_images2");

	ReadProperty(g_general_settings, g_generate_cleared_text_images_on_test, "generate_cleared_text_images_on_test");
	ReadProperty(g_general_settings, g_show_results, "dump_debug_images");
	ReadProperty(g_general_settings, g_show_sf_results, "dump_debug_second_filtration_images");
	ReadProperty(g_general_settings, g_clear_test_images_folder, "clear_test_images_folder");
	ReadProperty(g_general_settings, g_show_transformed_images_only, "show_transformed_images_only");
	ReadProperty(g_general_settings, g_use_ocl, "use_ocl");
	ReadProperty(g_general_settings, g_use_cuda_gpu, "use_cuda_gpu");

	ReadProperty(g_general_settings, g_use_filter_color, "use_filter_color");
	ReadProperty(g_general_settings, g_use_outline_filter_color, "use_outline_filter_color");
	ReadProperty(g_general_settings, g_dL_color, "dL_color");
	ReadProperty(g_general_settings, g_dA_color, "dA_color");
	ReadProperty(g_general_settings, g_dB_color, "dB_color");
	ReadProperty(g_general_settings, g_combine_to_single_cluster, "combine_to_single_cluster");

	ReadProperty(g_general_settings, g_cuda_kmeans_initial_loop_iterations, "cuda_kmeans_initial_loop_iterations");
	ReadProperty(g_general_settings, g_cuda_kmeans_loop_iterations, "cuda_kmeans_loop_iterations");
	ReadProperty(g_general_settings, g_cpu_kmeans_initial_loop_iterations, "cpu_kmeans_initial_loop_iterations");
	ReadProperty(g_general_settings, g_cpu_kmeans_loop_iterations, "cpu_kmeans_loop_iterations");

	ReadProperty(g_general_settings, g_smthr, "moderate_threshold_for_scaled_image");
	ReadProperty(g_general_settings, g_mthr, "moderate_threshold");
	ReadProperty(g_general_settings, g_mnthr, "moderate_threshold_for_NEdges");
	ReadProperty(g_general_settings, g_segw, "segment_width");
	ReadProperty(g_general_settings, g_segh, "segment_height");
	ReadProperty(g_general_settings, g_msegc, "minimum_segments_count");
	ReadProperty(g_general_settings, g_scd, "min_sum_color_diff");
	ReadProperty(g_general_settings, g_btd, "between_text_distace");
	ReadProperty(g_general_settings, g_to, "text_centre_offset");
	ReadProperty(g_general_settings, g_scale, "image_scale_for_clear_image");

	ReadProperty(g_general_settings, g_use_ISA_images_for_get_txt_area, "use_ISA_images");
	ReadProperty(g_general_settings, g_use_ILA_images_for_get_txt_area, "use_ILA_images");

	ReadProperty(g_general_settings, g_use_ILA_images_for_getting_txt_symbols_areas, "use_ILA_images_for_getting_txt_symbols_areas");
	ReadProperty(g_general_settings, g_use_ILA_images_before_clear_txt_images_from_borders, "use_ILA_images_before_clear_txt_images_from_borders");

	ReadProperty(g_general_settings, g_use_gradient_images_for_clear_txt_images, "use_gradient_images_for_clear_txt_images");
	ReadProperty(g_general_settings, g_clear_txt_images_by_main_color, "clear_txt_images_by_main_color");
	ReadProperty(g_general_settings, g_use_ILA_images_for_clear_txt_images, "use_ILA_images_for_clear_txt_images");

	ReadProperty(g_general_settings, g_mpn, "min_points_number");
	ReadProperty(g_general_settings, g_mpd, "min_points_density");
	ReadProperty(g_general_settings, g_msh, "min_symbol_height");
	ReadProperty(g_general_settings, g_msd, "min_symbol_density");
	ReadProperty(g_general_settings, g_mpned, "min_NEdges_points_density");

	ReadProperty(g_general_settings, g_clear_txt_folders, "clear_txt_folders");
	ReadProperty(g_general_settings, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	ReadProperty(g_general_settings, g_threads, "threads");
	ReadProperty(g_general_settings, g_ocr_threads, "ocr_threads");
	ReadProperty(g_general_settings, g_DL, "sub_frame_length");
	ReadProperty(g_general_settings, g_tp, "text_percent");
	ReadProperty(g_general_settings, g_mtpl, "min_text_len_in_percent");
	ReadProperty(g_general_settings, g_veple, "vedges_points_line_error");
	ReadProperty(g_general_settings, g_ilaple, "ila_points_line_error");

	ReadProperty(g_general_settings, g_video_contrast, "video_contrast");
	ReadProperty(g_general_settings, g_video_gamma, "video_gamma");

	ReadProperty(g_general_settings, g_clear_image_logical, "clear_image_logical");

	ReadProperty(g_general_settings, g_CLEAN_RGB_IMAGES, "clean_rgb_images_after_run");

	ReadProperty(g_general_settings, g_DefStringForEmptySub, "def_string_for_empty_sub");

	ReadProperty(g_general_settings, g_cfg.m_prefered_locale, "prefered_locale");

	ReadProperty(g_general_settings, g_cfg.process_affinity_mask, "process_affinity_mask");

	ReadProperty(g_general_settings, g_cfg.m_ocr_min_sub_duration, "min_sub_duration");	

	ReadProperty(g_general_settings, g_cfg.m_txt_dw, "txt_dw");
	ReadProperty(g_general_settings, g_cfg.m_txt_dy, "txt_dy");

	ReadProperty(g_general_settings, g_cfg.m_main_text_font, "main_text_font");
	ReadProperty(g_general_settings, g_cfg.m_main_text_font_bold, "main_text_font_bold");
	ReadProperty(g_general_settings, g_cfg.m_main_text_font_italic, "main_text_font_italic");
	ReadProperty(g_general_settings, g_cfg.m_main_text_font_underline, "main_text_font_underline");
	ReadProperty(g_general_settings, g_cfg.m_main_text_font_size, "main_text_font_size");

	ReadProperty(g_general_settings, g_cfg.m_buttons_text_font, "buttons_text_font");
	ReadProperty(g_general_settings, g_cfg.m_buttons_text_font_bold, "buttons_text_font_bold");
	ReadProperty(g_general_settings, g_cfg.m_buttons_text_font_italic, "buttons_text_font_italic");
	ReadProperty(g_general_settings, g_cfg.m_buttons_text_font_underline, "buttons_text_font_underline");
	ReadProperty(g_general_settings, g_cfg.m_buttons_text_font_size, "buttons_text_font_size");

	ReadProperty(g_general_settings, g_use_ISA_images_for_search_subtitles, "use_ISA_images_for_search_subtitles");
	ReadProperty(g_general_settings, g_use_ILA_images_for_search_subtitles, "use_ILA_images_for_search_subtitles");
	ReadProperty(g_general_settings, g_replace_ISA_by_filtered_version, "replace_ISA_by_filtered_version");
	ReadProperty(g_general_settings, g_max_dl_down, "max_dl_down");
	ReadProperty(g_general_settings, g_max_dl_up, "max_dl_up");

	ReadProperty(g_general_settings, g_remove_wide_symbols, "remove_wide_symbols");

	ReadProperty(g_general_settings, g_hw_device, "hw_device");

	if (ReadProperty(g_general_settings, g_filter_descr, "filter_descr"))
	{
		if (g_filter_descr == wxT("none"))
		{
			g_filter_descr = wxT("");
		}
	}

	ReadProperty(g_general_settings, g_save_each_substring_separately, "save_each_substring_separately");
	ReadProperty(g_general_settings, g_save_scaled_images, "save_scaled_images");

	ReadProperty(g_general_settings, g_playback_sound, "playback_sound");

	ReadProperty(g_general_settings, g_border_is_darker, "border_is_darker");

	int text_alignment;
	ReadProperty(g_general_settings, text_alignment, "text_alignment");
	g_text_alignment = (TextAlignment)text_alignment;	

	ReadProperty(g_general_settings, g_extend_by_grey_color, "extend_by_grey_color");
	ReadProperty(g_general_settings, g_allow_min_luminance, "allow_min_luminance");

	ReadProperty(g_general_settings, g_cfg.m_bottom_video_image_percent_end, "bottom_video_image_percent_end");
	ReadProperty(g_general_settings, g_cfg.m_top_video_image_percent_end, "top_video_image_percent_end");
	ReadProperty(g_general_settings, g_cfg.m_left_video_image_percent_end, "left_video_image_percent_end");
	ReadProperty(g_general_settings, g_cfg.m_right_video_image_percent_end, "right_video_image_percent_end");

	SaveToReportLog("CMainFrame::LoadSettings(): reading properties from g_general_settings end.\n");

	//------------------------------------------------

	SaveToReportLog("CMainFrame::LoadSettings(): LoadLocaleSettings(.. g_cfg.m_prefered_locale ..)...\n");

	LoadLocaleSettings(g_app_dir + wxT("/settings/") + wxString(g_cfg.m_prefered_locale) + wxT("/locale.cfg"));

	g_text_alignment_string = ConvertTextAlignmentToString(g_text_alignment);
}

void LoadLocaleSettings(wxString settings_path)
{
	SaveToReportLog(wxString::Format(wxT("CMainFrame::LoadLocaleSettings(): starting for \"%s\" ...\n"), settings_path));

	wxString main_up_to_date_locale_path = g_app_dir + wxT("/settings/eng/locale.cfg");
	if ( (settings_path != main_up_to_date_locale_path) && wxFileExists(main_up_to_date_locale_path) )
	{
		SaveToReportLog(wxString::Format(wxT("CMainFrame::LoadLocaleSettings(): loading main up to date locale data from \"%s\" ...\n"), main_up_to_date_locale_path));
		LoadLocaleSettings(main_up_to_date_locale_path);
	}

	SaveToReportLog(wxString::Format(wxT("CMainFrame::LoadLocaleSettings(): reading properties from \"%s\" ...\n"), settings_path));

	ReadSettings(settings_path, g_locale_settings);

	ReadProperty(g_locale_settings, g_cfg.m_help_desc_hotkeys_for_video_box, "help_desc_hotkeys_for_video_box");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_hotkeys_for_image_box, "help_desc_hotkeys_for_image_box");

	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_join_rgb_images, "ocr_label_join_images_join_rgb_images");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_use_txt_images_data_for_join_rgb_images, "ocr_label_join_images_use_txt_images_data_for_join_rgb_images");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_clear_dir, "ocr_label_join_images_clear_dir");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_dg_sub_group_settings_for_create_txt_images, "ocr_dg_sub_group_settings_for_create_txt_images");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_dg_sub_group_settings_for_join_images, "ocr_dg_sub_group_settings_for_join_images");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_dg_sub_group_settings_for_create_sub, "ocr_dg_sub_group_settings_for_create_sub");

	ReadProperty(g_locale_settings, g_cfg.m_fd_main_font_gb_label, "fd_main_font_gb_label");
	ReadProperty(g_locale_settings, g_cfg.m_fd_buttons_font_gb_label, "fd_buttons_font_gb_label");
	ReadProperty(g_locale_settings, g_cfg.m_fd_font_size, "fd_font_size");
	ReadProperty(g_locale_settings, g_cfg.m_fd_font_name, "fd_font_name");
	ReadProperty(g_locale_settings, g_cfg.m_fd_font_bold, "fd_font_bold");
	ReadProperty(g_locale_settings, g_cfg.m_fd_font_italic, "fd_font_italic");
	ReadProperty(g_locale_settings, g_cfg.m_fd_font_underline, "fd_font_underline");

	ReadProperty(g_locale_settings, g_cfg.m_text_alignment_center, "text_alignment_center");
	ReadProperty(g_locale_settings, g_cfg.m_text_alignment_left, "text_alignment_left");
	ReadProperty(g_locale_settings, g_cfg.m_text_alignment_right, "text_alignment_right");
	ReadProperty(g_locale_settings, g_cfg.m_text_alignment_any, "text_alignment_any");

	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_open_video_file, "file_dialog_title_open_video_file");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_open_video_file_wild_card, "file_dialog_title_open_video_file_wild_card");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_open_settings_file, "file_dialog_title_open_settings_file");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_open_settings_file_wild_card, "file_dialog_title_open_settings_file_wild_card");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_save_settings_file, "file_dialog_title_save_settings_file");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_save_settings_file_wild_card, "file_dialog_title_save_settings_file_wild_card");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_save_subtitle_as, "file_dialog_title_save_subtitle_as");
	ReadProperty(g_locale_settings, g_cfg.m_file_dialog_title_save_subtitle_as_wild_card, "file_dialog_title_save_subtitle_as_wild_card");

	ReadProperty(g_locale_settings, g_cfg.m_menu_file, "menu_file");
	ReadProperty(g_locale_settings, g_cfg.m_menu_edit, "menu_edit");
	ReadProperty(g_locale_settings, g_cfg.m_menu_view, "menu_view");
	ReadProperty(g_locale_settings, g_cfg.m_menu_play, "menu_play");
	ReadProperty(g_locale_settings, g_cfg.m_menu_help, "menu_help");
	ReadProperty(g_locale_settings, g_cfg.m_menu_localization, "menu_localization");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority, "menu_setpriority");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority_high, "menu_setpriority_high");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority_abovenormal, "menu_setpriority_abovenormal");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority_normal, "menu_setpriority_normal");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority_belownormal, "menu_setpriority_belownormal");
	ReadProperty(g_locale_settings, g_cfg.m_menu_setpriority_idle, "menu_setpriority_idle");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_open_video_opencv, "menu_file_open_video_opencv");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_open_video_ffmpeg, "menu_file_open_video_ffmpeg");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_reopenvideo, "menu_file_reopenvideo");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_openpreviousvideo, "menu_file_openpreviousvideo");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_savesettings, "menu_file_savesettings");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_savesettingsas, "menu_file_savesettingsas");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_loadsettings, "menu_file_loadsettings");
	ReadProperty(g_locale_settings, g_cfg.m_menu_file_exit, "menu_file_exit");
	ReadProperty(g_locale_settings, g_cfg.m_menu_edit_setbegintime, "menu_edit_setbegintime");
	ReadProperty(g_locale_settings, g_cfg.m_menu_edit_setendtime, "menu_edit_setendtime");
	ReadProperty(g_locale_settings, g_cfg.m_menu_fonts, "menu_fonts");
	ReadProperty(g_locale_settings, g_cfg.m_menu_scale_text_size_inc, "menu_scale_text_size_inc");
	ReadProperty(g_locale_settings, g_cfg.m_menu_scale_text_size_dec, "menu_scale_text_size_dec");
	ReadProperty(g_locale_settings, g_cfg.m_menu_play_pause, "menu_play_pause");
	ReadProperty(g_locale_settings, g_cfg.m_menu_play_stop, "menu_play_stop");
	ReadProperty(g_locale_settings, g_cfg.m_menu_next_frame, "menu_next_frame");
	ReadProperty(g_locale_settings, g_cfg.m_menu_previous_frame, "menu_previous_frame");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_cmd_args_info, "menu_app_cmd_args_info");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_usage_docs, "menu_app_usage_docs");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_website, "menu_app_website");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_forum, "menu_app_forum");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_bug_tracker, "menu_app_bug_tracker");
	ReadProperty(g_locale_settings, g_cfg.m_menu_app_about, "menu_app_about");

	ReadProperty(g_locale_settings, g_cfg.m_help_desc_app_about, "help_desc_app_about");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_app_about_dev_email, "help_desc_app_about_dev_email");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_app_about_dev_contact, "help_desc_app_about_dev_contact");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_app_about_dev_donate, "help_desc_app_about_dev_donate");

	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_clear_dirs, "help_desc_for_clear_dirs");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_run_search, "help_desc_for_run_search");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_join_images, "help_desc_for_join_images");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_create_cleared_text_images, "help_desc_for_create_cleared_text_images");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_create_empty_sub, "help_desc_for_create_empty_sub");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_create_sub_from_cleared_txt_images, "help_desc_for_create_sub_from_cleared_txt_images");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_create_sub_from_txt_results, "help_desc_for_create_sub_from_txt_results");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_input_video, "help_desc_for_input_video");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_open_video_opencv, "help_desc_for_open_video_opencv");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_open_video_ffmpeg, "help_desc_for_open_video_ffmpeg");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_use_cuda, "help_desc_for_use_cuda");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_start_time, "help_desc_for_start_time");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_end_time, "help_desc_for_end_time");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_top_video_image_percent_end, "help_desc_for_top_video_image_percent_end");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_bottom_video_image_percent_end, "help_desc_for_bottom_video_image_percent_end");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_left_video_image_percent_end, "help_desc_for_left_video_image_percent_end");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_right_video_image_percent_end, "help_desc_for_right_video_image_percent_end");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_output_dir, "help_desc_for_output_dir");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_general_settings, "help_desc_for_general_settings");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_num_threads, "help_desc_for_num_threads");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_num_ocr_threads, "help_desc_for_num_ocr_threads");
	ReadProperty(g_locale_settings, g_cfg.m_help_desc_for_help, "help_desc_for_help");

	ReadProperty(g_locale_settings, g_cfg.m_button_clear_folders_text, "button_clear_folders_text");
	ReadProperty(g_locale_settings, g_cfg.m_button_run_search_text, "button_run_search_text");
	ReadProperty(g_locale_settings, g_cfg.m_button_run_search_stop_text, "button_run_search_stop_text");
	ReadProperty(g_locale_settings, g_cfg.m_button_test_text, "button_test_text");
	ReadProperty(g_locale_settings, g_cfg.m_test_result_after_first_filtration_label, "test_result_after_first_filtration_label");
	ReadProperty(g_locale_settings, g_cfg.m_test_result_after_second_filtration_label, "test_result_after_second_filtration_label");
	ReadProperty(g_locale_settings, g_cfg.m_test_result_after_third_filtration_label, "test_result_after_third_filtration_label");
	ReadProperty(g_locale_settings, g_cfg.m_test_result_nedges_points_image_label, "test_result_nedges_points_image_label");
	ReadProperty(g_locale_settings, g_cfg.m_test_result_cleared_text_image_label, "test_result_cleared_text_image_label");
	ReadProperty(g_locale_settings, g_cfg.m_grid_col_property_label, "grid_col_property_label");
	ReadProperty(g_locale_settings, g_cfg.m_grid_col_value_label, "grid_col_value_label");
	ReadProperty(g_locale_settings, g_cfg.m_label_begin_time, "label_begin_time");
	ReadProperty(g_locale_settings, g_cfg.m_label_end_time, "label_end_time");
	ReadProperty(g_locale_settings, g_cfg.m_run_search_progress_format_string, "run_search_progress_format_string");
	ReadProperty(g_locale_settings, g_cfg.m_ccti_start_progress_format_string, "ccti_start_progress_format_string");
	ReadProperty(g_locale_settings, g_cfg.m_ccti_progress_format_string, "ccti_progress_format_string");	
	ReadProperty(g_locale_settings, g_cfg.m_video_box_title, "video_box_title");
	ReadProperty(g_locale_settings, g_cfg.m_image_box_title, "image_box_title");
	ReadProperty(g_locale_settings, g_cfg.m_search_panel_title, "search_panel_title");
	ReadProperty(g_locale_settings, g_cfg.m_settings_panel_title, "settings_panel_title");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_panel_title, "ocr_panel_title");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_msd_text, "ocr_label_msd_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_split_line_text, "ocr_label_join_images_split_line_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_jsact_text, "ocr_label_jsact_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_clear_txt_folders, "ocr_label_clear_txt_folders");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_save_each_substring_separately, "ocr_label_save_each_substring_separately");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_save_scaled_images, "ocr_label_save_scaled_images");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_ces_text, "ocr_button_ces_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_join_text, "ocr_button_join_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_join_stop_text, "ocr_button_join_stop_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_ccti_text, "ocr_button_ccti_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_ccti_stop_text, "ocr_button_ccti_stop_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_csftr_text, "ocr_button_csftr_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_cesfcti_text, "ocr_button_cesfcti_text");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_button_test_text, "ocr_button_test_text");

	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_split_line_font_size, "ocr_label_join_images_split_line_font_size");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_split_line_font_bold, "ocr_label_join_images_split_line_font_bold");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_sub_id_format, "ocr_label_join_images_sub_id_format");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_sub_search_by_id_format, "ocr_label_join_images_sub_search_by_id_format");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_scale, "ocr_label_join_images_scale");
	ReadProperty(g_locale_settings, g_cfg.m_ocr_label_join_images_max_number, "ocr_label_join_images_max_number");

	ReadProperty(g_locale_settings, g_cfg.m_label_text_alignment, "label_text_alignment");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_hw_device, "ssp_hw_device");
	ReadProperty(g_locale_settings, g_cfg.m_label_filter_descr, "label_filter_descr");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_use_ocl, "ssp_oi_property_use_ocl");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_use_cuda_gpu, "ssp_oi_property_use_cuda_gpu");

	ReadProperty(g_locale_settings, g_cfg.m_label_use_filter_color, "label_use_filter_color");
	ReadProperty(g_locale_settings, g_cfg.m_label_use_outline_filter_color, "label_use_outline_filter_color");
	ReadProperty(g_locale_settings, g_cfg.m_label_dL_color, "label_dL_color");
	ReadProperty(g_locale_settings, g_cfg.m_label_dA_color, "label_dA_color");
	ReadProperty(g_locale_settings, g_cfg.m_label_dB_color, "label_dB_color");

	ReadProperty(g_locale_settings, g_cfg.m_border_is_darker, "label_border_is_darker");
	ReadProperty(g_locale_settings, g_cfg.m_extend_by_grey_color, "label_extend_by_grey_color");
	ReadProperty(g_locale_settings, g_cfg.m_allow_min_luminance, "label_allow_min_luminance");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_ocr_threads, "ssp_ocr_threads");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_image_scale_for_clear_image, "ssp_oi_property_image_scale_for_clear_image");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, "ssp_oi_property_moderate_threshold_for_scaled_image");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, "ssp_oi_property_cuda_kmeans_initial_loop_iterations");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, "ssp_oi_property_cuda_kmeans_loop_iterations");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, "ssp_oi_property_cpu_kmeans_initial_loop_iterations");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, "ssp_oi_property_cpu_kmeans_loop_iterations");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_label_parameters_influencing_image_processing, "ssp_label_parameters_influencing_image_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, "ssp_label_ocl_and_multiframe_image_stream_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_group_global_image_processing_settings, "ssp_oi_group_global_image_processing_settings");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, "ssp_oi_property_generate_cleared_text_images_on_test");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_dump_debug_images, "ssp_oi_property_dump_debug_images");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, "ssp_oi_property_dump_debug_second_filtration_images");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_clear_test_images_folder, "ssp_oi_property_clear_test_images_folder");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_show_transformed_images_only, "ssp_oi_property_show_transformed_images_only");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_group_initial_image_processing, "ssp_oi_group_initial_image_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, "ssp_oi_sub_group_settings_for_sobel_operators");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_moderate_threshold, "ssp_oi_property_moderate_threshold");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_moderate_nedges_threshold, "ssp_oi_property_moderate_nedges_threshold");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, "ssp_oi_sub_group_settings_for_color_filtering");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_segment_width, "ssp_oi_property_segment_width");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_segments_count, "ssp_oi_property_min_segments_count");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_sum_color_difference, "ssp_oi_property_min_sum_color_difference");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_group_secondary_image_processing, "ssp_oi_group_secondary_image_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, "ssp_oi_sub_group_settings_for_linear_filtering");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_line_height, "ssp_oi_property_line_height");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_max_between_text_distance, "ssp_oi_property_max_between_text_distance");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_max_text_center_offset, "ssp_oi_property_max_text_center_offset");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_max_text_center_percent_offset, "ssp_oi_property_max_text_center_percent_offset");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, "ssp_oi_sub_group_settings_for_color_border_points");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_points_number, "ssp_oi_property_min_points_number");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_points_density, "ssp_oi_property_min_points_density");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_symbol_height, "ssp_oi_property_min_symbol_height");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_symbol_density, "ssp_oi_property_min_symbol_density");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_vedges_points_density, "ssp_oi_property_min_vedges_points_density");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_property_min_nedges_points_density, "ssp_oi_property_min_nedges_points_density");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oi_group_tertiary_image_processing, "ssp_oi_group_tertiary_image_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_group_ocr_settings, "ssp_oim_group_ocr_settings");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_clear_images_logical, "ssp_oim_property_clear_images_logical");
	ReadProperty(g_locale_settings, g_cfg.m_label_combine_to_single_cluster, "label_combine_to_single_cluster");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, "ssp_oim_property_clear_rgbimages_after_search_subtitles");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_using_hard_algorithm_for_text_mining, "ssp_oim_property_using_hard_algorithm_for_text_mining");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, "ssp_oim_property_using_isaimages_for_getting_txt_areas");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, "ssp_oim_property_using_ilaimages_for_getting_txt_areas");
	
	ReadProperty(g_locale_settings, g_cfg.m_label_ILA_images_for_getting_txt_symbols_areas, "label_ILA_images_for_getting_txt_symbols_areas");
	ReadProperty(g_locale_settings, g_cfg.m_label_use_ILA_images_before_clear_txt_images_from_borders, "label_use_ILA_images_before_clear_txt_images_from_borders");
	
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, "ssp_oim_property_validate_and_compare_cleared_txt_images");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, "ssp_oim_property_dont_delete_unrecognized_images_first");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, "ssp_oim_property_dont_delete_unrecognized_images_second");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_default_string_for_empty_sub, "ssp_oim_property_default_string_for_empty_sub");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, "ssp_oim_group_settings_for_multiframe_image_processing");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, "ssp_oim_sub_group_settings_for_sub_detection");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_threads, "ssp_oim_property_threads");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_sub_frames_length, "ssp_oim_property_sub_frames_length");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, "ssp_oim_sub_group_settings_for_comparing_subs");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_vedges_points_line_error, "ssp_oim_property_vedges_points_line_error");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_ila_points_line_error, "ssp_oim_property_ila_points_line_error");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, "ssp_oim_sub_group_settings_for_checking_sub");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_text_percent, "ssp_oim_property_text_percent");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_min_text_length, "ssp_oim_property_min_text_length");

	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_sub_group_settings_for_update_video_color, "ssp_oim_sub_group_settings_for_update_video_color");
	ReadProperty(g_locale_settings, g_cfg.m_label_video_contrast, "label_video_contrast");
	ReadProperty(g_locale_settings, g_cfg.m_label_video_gamma, "label_video_gamma");

	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, "ssp_oim_property_use_ISA_images_for_search_subtitles");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, "ssp_oim_property_use_ILA_images_for_search_subtitles");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, "ssp_oim_property_replace_ISA_by_filtered_version");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_max_dl_down, "ssp_oim_property_max_dl_down");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_max_dl_up, "ssp_oim_property_max_dl_up");

	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, "ssp_oim_property_use_gradient_images_for_clear_txt_images");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, "ssp_oim_property_clear_txt_images_by_main_color");
	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, "ssp_oim_property_use_ILA_images_for_clear_txt_images");

	ReadProperty(g_locale_settings, g_cfg.m_ssp_oim_property_remove_wide_symbols, "ssp_oim_property_remove_wide_symbols");

	ReadProperty(g_locale_settings, g_cfg.m_label_settings_file, "label_settings_file");
	ReadProperty(g_locale_settings, g_cfg.m_label_pixel_color, "label_pixel_color");

	ReadProperty(g_locale_settings, g_cfg.m_playback_sound, "label_playback_sound");

	SaveToReportLog("CMainFrame::LoadLocaleSettings(): finished.\n");
}

void SaveSettings()
{
	wxFFileOutputStream ffout(g_GeneralSettingsFileName);
	wxTextOutputStream fout(ffout);

	g_pMF->m_pPanel->m_pSSPanel->m_pOI->SaveEditControlValue();
	g_pMF->m_pPanel->m_pSSPanel->m_pOIM->SaveEditControlValue();

	WriteProperty(fout, g_cfg.m_prefered_locale, "prefered_locale");

	WriteProperty(fout, g_pMF->IsMaximized(), "vsf_is_maximized");
	wxRect rcMF = g_pMF->GetRect();
	WriteProperty(fout, rcMF.x, "vsf_x");
	WriteProperty(fout, rcMF.y, "vsf_y");
	WriteProperty(fout, rcMF.width, "vsf_w");
	WriteProperty(fout, rcMF.height, "vsf_h");

	WriteProperty(fout, g_cfg.process_affinity_mask, "process_affinity_mask");

	WriteProperty(fout, g_cfg.m_main_text_font, "main_text_font");
	WriteProperty(fout, g_cfg.m_main_text_font_bold, "main_text_font_bold");
	WriteProperty(fout, g_cfg.m_main_text_font_italic, "main_text_font_italic");
	WriteProperty(fout, g_cfg.m_main_text_font_underline, "main_text_font_underline");
	WriteProperty(fout, (g_cfg.m_main_text_font_size == g_def_main_text_font_size) ? -1 : g_cfg.m_main_text_font_size, "main_text_font_size");

	WriteProperty(fout, g_cfg.m_buttons_text_font, "buttons_text_font");
	WriteProperty(fout, g_cfg.m_buttons_text_font_bold, "buttons_text_font_bold");
	WriteProperty(fout, g_cfg.m_buttons_text_font_italic, "buttons_text_font_italic");
	WriteProperty(fout, g_cfg.m_buttons_text_font_underline, "buttons_text_font_underline");
	WriteProperty(fout, (g_cfg.m_buttons_text_font_size == g_def_buttons_text_font_size) ? -1: g_cfg.m_buttons_text_font_size, "buttons_text_font_size");
	
	WriteProperty(fout, g_cfg.m_main_text_colour, "main_text_colour");
	WriteProperty(fout, g_cfg.m_main_text_ctls_background_colour, "main_text_ctls_background_colour");
	WriteProperty(fout, g_cfg.m_main_buttons_colour, "main_buttons_colour");
	WriteProperty(fout, g_cfg.m_main_buttons_colour_focused, "main_buttons_colour_focused");
	WriteProperty(fout, g_cfg.m_main_buttons_colour_selected, "main_buttons_colour_selected");
	WriteProperty(fout, g_cfg.m_main_buttons_border_colour, "main_buttons_border_colour");
	WriteProperty(fout, g_cfg.m_main_labels_background_colour, "main_labels_background_colour");
	WriteProperty(fout, g_cfg.m_main_frame_background_colour, "main_frame_background_colour");
	WriteProperty(fout, g_cfg.m_notebook_colour, "notebook_colour");
	WriteProperty(fout, g_cfg.m_notebook_panels_colour, "notebook_panels_colour");
	WriteProperty(fout, g_cfg.m_grid_line_colour, "grid_line_colour");
	WriteProperty(fout, g_cfg.m_grid_gropes_colour, "grid_gropes_colour");
	WriteProperty(fout, g_cfg.m_grid_sub_gropes_colour, "grid_sub_gropes_colour");
	WriteProperty(fout, g_cfg.m_grid_debug_settings_colour, "grid_debug_settings_colour");
	WriteProperty(fout, g_cfg.m_test_result_label_colour, "test_result_label_colour");
	WriteProperty(fout, g_cfg.m_video_image_box_background_colour, "video_image_box_background_colour");
	WriteProperty(fout, g_cfg.m_video_image_box_border_colour, "video_image_box_border_colour");
	WriteProperty(fout, g_cfg.m_video_image_box_title_colour, "video_image_box_title_colour");
	WriteProperty(fout, g_cfg.m_video_box_time_colour, "video_box_time_colour");
	WriteProperty(fout, g_cfg.m_video_box_time_text_colour, "video_box_time_text_colour");
	WriteProperty(fout, g_cfg.m_video_box_separating_line_colour, "video_box_separating_line_colour");
	WriteProperty(fout, g_cfg.m_video_box_separating_line_border_colour, "video_box_separating_line_border_colour");	

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

	WriteProperty(fout, g_cfg.m_ocr_min_sub_duration, "min_sub_duration");

	WriteProperty(fout, g_DefStringForEmptySub, "def_string_for_empty_sub");	

	WriteProperty(fout, g_cfg.m_txt_dw, "txt_dw");
	WriteProperty(fout, g_cfg.m_txt_dy, "txt_dy");

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

	g_text_alignment = ConvertStringToTextAlignment(g_text_alignment_string);
	WriteProperty(fout, (int)g_text_alignment, "text_alignment");

	WriteProperty(fout, g_save_each_substring_separately, "save_each_substring_separately");
	WriteProperty(fout, g_save_scaled_images, "save_scaled_images");

	WriteProperty(fout, g_playback_sound, "playback_sound");

	WriteProperty(fout, g_border_is_darker, "border_is_darker");

	WriteProperty(fout, g_extend_by_grey_color, "extend_by_grey_color");
	WriteProperty(fout, g_allow_min_luminance, "allow_min_luminance");

	double double_val;

	double_val = 1 - std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);
	WriteProperty(fout, double_val, "bottom_video_image_percent_end");
	double_val = 1 - std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);
	WriteProperty(fout, double_val, "top_video_image_percent_end");
	
	WriteProperty(fout, std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos), "left_video_image_percent_end");
	WriteProperty(fout, std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos), "right_video_image_percent_end");

	WriteProperty(fout, g_cfg.m_toolbar_bitmaps_transparent_colour, "toolbar_bitmaps_transparent_colour");

	WriteProperty(fout, g_cfg.m_ocr_join_images_join_rgb_images, "ocr_join_images_join_rgb_images");
	WriteProperty(fout, g_cfg.m_ocr_join_images_use_txt_images_data_for_join_rgb_images, "ocr_join_images_use_txt_images_data_for_join_rgb_images");
	WriteProperty(fout, g_cfg.m_ocr_join_images_clear_dir, "ocr_join_images_clear_dir");
	WriteProperty(fout, g_cfg.m_ocr_join_images_split_line, "ocr_join_images_split_line");
	WriteProperty(fout, g_cfg.m_ocr_join_images_split_line_font_size, "ocr_join_images_split_line_font_size");
	WriteProperty(fout, g_cfg.m_ocr_join_images_split_line_font_bold, "ocr_join_images_split_line_font_bold");
	WriteProperty(fout, g_cfg.m_ocr_join_images_sub_id_format, "ocr_join_images_sub_id_format");
	WriteProperty(fout, g_cfg.m_ocr_join_images_sub_search_by_id_format, "ocr_join_images_sub_search_by_id_format");	
	WriteProperty(fout, g_cfg.m_ocr_join_images_scale, "ocr_join_images_scale");
	WriteProperty(fout, g_cfg.m_ocr_join_images_max_number, "ocr_join_images_max_number");

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
	SaveToReportLog("CMainFrame::OnFileLoadSettings(): starting ...\n");

	wxString settings_file_dir = (m_last_specified_settings_file_path.size() > 0) ? GetFileDir(m_last_specified_settings_file_path) : g_app_dir;
	wxString settings_file_name = (m_last_specified_settings_file_path.size() > 0) ? GetFileNameWithExtension(m_last_specified_settings_file_path) : wxString(wxEmptyString);

	wxFileDialog fd(this, g_cfg.m_file_dialog_title_open_settings_file,
		settings_file_dir, settings_file_name, g_cfg.m_file_dialog_title_open_settings_file_wild_card, wxFD_OPEN);

	if(fd.ShowModal() != wxID_OK)
	{
		return;
	}

	g_GeneralSettingsFileName = fd.GetPath();
	m_last_specified_settings_file_path = g_GeneralSettingsFileName;
	g_cfg.m_ssp_GSFN_label = g_GeneralSettingsFileName + wxT(" ");
	this->m_pPanel->m_pSSPanel->m_pGSFN->SetLabel(g_cfg.m_ssp_GSFN_label);

	LoadSettings();

	this->SetBackgroundColour(g_cfg.m_main_frame_background_colour);

	SetFonts();	
	
	SaveToReportLog("CMainFrame::OnFileLoadSettings(): CreateMenuBar() ...\n");
	wxMenuBar* pMenuBar = CreateMenuBar();

	SaveToReportLog("CMainFrame::OnFileLoadSettings(): this->SetMenuBar(pMenuBar) ...\n");
	this->SetMenuBar(pMenuBar);

	for (wxString localization : g_localizations)
	{
		int loc_id = g_localization_id[localization];

		if (localization == g_cfg.m_prefered_locale)
		{
			pMenuBar->Check(loc_id, true);
		}
		else
		{
			pMenuBar->Check(loc_id, false);
		}
	}

	UpdateDynamicSettings();
	CControl::RefreshAllControlsData();
	CControl::UpdateAllControlsSize();
	this->Refresh();

	SaveToReportLog("CMainFrame::OnFileLoadSettings(): end.\n");
}

void CMainFrame::OnFileSaveSettingsAs(wxCommandEvent& event)
{
	wxString settings_file_dir = (m_last_specified_settings_file_path.size() > 0) ? GetFileDir(m_last_specified_settings_file_path) : g_app_dir;
	wxString settings_file_name = (m_last_specified_settings_file_path.size() > 0) ? GetFileNameWithExtension(m_last_specified_settings_file_path) : wxString(wxEmptyString);

	wxFileDialog fd(this, g_cfg.m_file_dialog_title_save_settings_file,
		settings_file_dir, settings_file_name, g_cfg.m_file_dialog_title_save_settings_file_wild_card, wxFD_SAVE);

	if (m_blnReopenVideo == false)
	{
		if(fd.ShowModal() != wxID_OK)
		{
			return;
		}

		g_GeneralSettingsFileName = fd.GetPath();

		wxString ext = GetFileExtension(g_GeneralSettingsFileName);

		if (ext != wxT("cfg"))
		{
			g_GeneralSettingsFileName += wxT(".cfg");
		}

		m_last_specified_settings_file_path = g_GeneralSettingsFileName;

		g_cfg.m_ssp_GSFN_label = g_GeneralSettingsFileName + wxT(" ");
		this->m_pPanel->m_pSSPanel->m_pGSFN->SetLabel(g_cfg.m_ssp_GSFN_label);

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
								
				if (m_EndTime >= 0)
				{
					double progress = std::min<double>(((double)(Cur - m_BegTime) / (double)(m_EndTime - m_BegTime)) * 100.0, 100.0);
					u64 eta = (u64)((double)run_time * (100.0 - progress) / progress);
					g_cfg.m_run_search_str_progress.Printf(wxT("%%%2.2f"), progress);
					g_cfg.m_run_search_str_eta = ConvertTime(eta);
				}
				else				
				{
					g_cfg.m_run_search_str_progress = wxT("%%N/A");
					g_cfg.m_run_search_str_eta = wxT("N/A");
				}

				g_cfg.m_run_search_run_time = ConvertTime(run_time);
				g_cfg.m_run_search_cur_time = ConvertVideoTime(Cur);

				get_video_box_lblTIME_run_search_label();				
				m_pVideoBox->m_plblTIME->SetLabel(g_cfg.m_video_box_lblTIME_label);
			}			
		}
		else
		{
			g_cfg.m_video_box_lblTIME_label = ConvertVideoTime(Cur) + m_EndTimeStr + wxT("   ");
			m_pVideoBox->m_plblTIME->SetLabel(g_cfg.m_video_box_lblTIME_label);
		}

		m_ct = Cur;
	}
	else
	{
		if (g_RunSubSearch == 0)
		{
			if ((m_vs == Play) && (m_pVideo->m_play_video == false))
			{
				m_vs = Pause;
				m_pVideo->Pause();
				m_pVideoBox->m_pButtonPause->Hide();
				m_pVideoBox->m_pButtonRun->Show();
			}
		}
	}

	m_pVideoBox->m_pSB->SetScrollPos((int)Cur);
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
	
	{
		wxFFileOutputStream ffout(g_prev_data_path);
		wxTextOutputStream fout(ffout);

		if (m_FileName.size() > 0)
		{
			m_last_video_file_path = m_FileName;
			m_last_video_begin_time = m_BegTime;
			m_last_video_end_time = m_EndTime;
			m_last_video_open_type = m_type;
		}

		if (m_last_video_file_path.size() > 0)
		{
			WriteProperty(fout, m_last_video_file_path, "last_video_file_path");
			WriteProperty(fout, m_last_video_begin_time, "last_video_begin_time");
			WriteProperty(fout, m_last_video_end_time, "last_video_end_time");
			WriteProperty(fout, m_last_video_open_type, "last_video_open_type");
		}

		if (m_last_saved_sub_file_path.size() > 0)
		{
			WriteProperty(fout, m_last_saved_sub_file_path, "last_saved_sub_file_path");
		}

		if (m_last_specified_settings_file_path.size() > 0)
		{
			WriteProperty(fout, m_last_specified_settings_file_path, "last_specified_settings_file_path");
		}

		fout.Flush();
		ffout.Close();
	}

	if (!m_blnNoGUI)
	{
		wxRect rcMF = g_pMF->GetRect();
		wxRect rcVB = g_pMF->m_pVideoBox->GetRect();
		wxRect rcIB = g_pMF->m_pImageBox->GetRect();

		UpdateSettingsInFile(g_GeneralSettingsFileName, std::map<wxString, wxString>{
			{ wxT("vsf_is_maximized"), wxString::Format(wxString(wxT("%d")), g_pMF->IsMaximized() ? 1 : 0) },
			{ wxT("vsf_x"), wxString::Format(wxString(wxT("%d")), rcMF.x) },
			{ wxT("vsf_y"), wxString::Format(wxString(wxT("%d")), rcMF.y) },
			{ wxT("vsf_w"), wxString::Format(wxString(wxT("%d")), rcMF.width) },
			{ wxT("vsf_h"), wxString::Format(wxString(wxT("%d")), rcMF.height) }
		});
	}

	{
		std::unique_lock<std::mutex> lock(m_pPanel->m_pSHPanel->m_rs_mutex);
		if (g_IsSearching == 1)
		{
			g_IsClose = 1;
			g_RunSubSearch = 0;
			if (m_pPanel->m_pSHPanel->m_SearchThread.joinable())
			{
				m_pPanel->m_pSHPanel->m_SearchThread.join();
			}
		}
	}

	{
		std::unique_lock<std::mutex> lock(m_pPanel->m_pOCRPanel->m_mutex);
		if (g_IsCreateClearedTextImages == 1)
		{
			g_RunCreateClearedTextImages = 0;
			if (m_pPanel->m_pOCRPanel->m_CCTIThread.joinable())
			{
				m_pPanel->m_pOCRPanel->m_CCTIThread.join();
			}
		}
	}

	{
		std::unique_lock<std::mutex> lock(m_pPanel->m_pOCRPanel->m_mutex);
		if (g_IsJoinTXTImages == 1)
		{
			if (m_pPanel->m_pOCRPanel->m_JoinTXTImagesThread.joinable())
			{
				m_pPanel->m_pOCRPanel->m_JoinTXTImagesThread.join();
			}
		}
	}

	Destroy();
}

void CMainFrame::OnFileOpenPreviousVideo(wxCommandEvent& event)
{
	int int_val;

	if (m_last_video_file_path.size() > 0)
	{
		m_FileName = m_last_video_file_path;
		m_BegTime = m_last_video_begin_time;
		m_EndTime = m_last_video_end_time;
		m_type = m_last_video_open_type;

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

class MyMessageBox : public wxDialog, public CControl
{
public:
	CMainFrame* m_pMF;
	wxTextCtrl* m_pDialogText;
	wxColour*	m_p_background_colour;

	MyMessageBox(CMainFrame* pMF, wxString message, const wxString& caption,
		wxColour* p_background_colour = NULL,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~MyMessageBox()
	{
	}

	void OnTextUrl(wxTextUrlEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void UpdateSize() override;
	void RefreshData() override;

	int m_w;
};

MyMessageBox::MyMessageBox(CMainFrame* pMF, wxString message, const wxString& caption,
	wxColour* p_background_colour,
	const wxPoint& pos,
	const wxSize& size) : wxDialog(pMF, wxID_ANY, caption, pos, size)
{
	m_pMF = pMF;
	m_p_background_colour = p_background_colour;
	m_w = size.x;
	m_pDialogText = new wxTextCtrl(this, wxID_ANY, message, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_AUTO_URL | wxTE_READONLY | wxBORDER_NONE);
	m_pDialogText->SetFont(m_pMF->m_LBLFont);
	m_pDialogText->SetForegroundColour(g_cfg.m_main_text_colour);
	if (m_p_background_colour) m_pDialogText->SetBackgroundColour(*m_p_background_colour);
	if (m_p_background_colour) SetBackgroundColour(*m_p_background_colour);
	Bind(wxEVT_TEXT_URL, &MyMessageBox::OnTextUrl, this);
	Bind(wxEVT_CHAR_HOOK, &MyMessageBox::OnKeyDown, this);
	Bind(wxEVT_MOUSEWHEEL, &CMainFrame::OnMouseWheel, m_pMF);
	m_pDialogText->SetInsertionPoint(0);

	wxBoxSizer* vert_box_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hor_box_sizer = new wxBoxSizer(wxHORIZONTAL);

	vert_box_sizer->Add(m_pDialogText, 0, wxALIGN_TOP);
	hor_box_sizer->AddSpacer(5);
	hor_box_sizer->Add(vert_box_sizer, 1, wxALIGN_LEFT);
	this->SetSizer(hor_box_sizer);

	UpdateSize();
}

void MyMessageBox::OnTextUrl(wxTextUrlEvent& event)
{
	if (event.GetMouseEvent().LeftIsDown())
	{
		wxTextCtrl* pTextCtrl = (wxTextCtrl*)event.GetEventObject();
		wxLaunchDefaultBrowser(pTextCtrl->GetRange(event.GetURLStart(), event.GetURLEnd()));
	}
}

void MyMessageBox::OnKeyDown(wxKeyEvent& event)
{	
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		EndModal(0);
	}
	else
	{
		switch (event.GetKeyCode())
		{
			case 'C':
			case 'c':
				if (wxGetKeyState(WXK_CONTROL))
				{
					wxCommandEvent evt(wxEVT_KEY_DOWN, wxID_ANY);
					m_pDialogText->OnCopy(evt);
				}
				break;
		}
	}
}

void MyMessageBox::RefreshData()
{
	m_pDialogText->SetFont(m_pMF->m_LBLFont);
	m_pDialogText->SetForegroundColour(g_cfg.m_main_text_colour);
	if (m_p_background_colour) m_pDialogText->SetBackgroundColour(*m_p_background_colour);
	if (m_p_background_colour) SetBackgroundColour(*m_p_background_colour);
	m_pDialogText->Refresh(true);
}

wxString WrapText(wxWindow* win, const wxString& text, int widthMax, int &ln)
{
	class HardBreakWrapper : public wxTextWrapper
	{
	public:
		int m_ln = 1;

		HardBreakWrapper(wxWindow* win, const wxString& text, int widthMax)
		{
			Wrap(win, text, widthMax);
		}
		wxString const& GetWrapped() const { return m_wrapped; }
	protected:
		virtual void OnOutputLine(const wxString& line)
		{
			m_wrapped += line;
		}
		virtual void OnNewLine()
		{
			m_wrapped += '\n';
			m_ln++;
		}
	private:
		wxString m_wrapped;
	};
	HardBreakWrapper wrapper(win, text, widthMax);
	ln = wrapper.m_ln;
	return wrapper.GetWrapped();
}

void MyMessageBox::UpdateSize()
{
	wxMemoryDC dc;	
	dc.SetFont(m_pMF->m_LBLFont);

	wxSize cur_size = GetSize();
	wxSize cur_client_size = GetClientSize();
	wxSize mf_cl_size = m_pMF->GetClientSize();

	wxString orig_val = m_pDialogText->GetValue();
	int ln;
	wxString wrap_val = WrapText(m_pDialogText, orig_val, m_w - 15 - (cur_size.x - cur_client_size.x), ln);
	wxSize text_size = dc.GetMultiLineTextExtent(wrap_val);
	
	wxSize best_size;

	best_size = m_pDialogText->GetSizeFromTextSize(text_size);
	best_size.x += cur_size.x - cur_client_size.x + 15;
	best_size.y += cur_size.y - cur_client_size.y + 10;
	this->SetSize(std::max<int>((mf_cl_size.x - best_size.x) / 2, 0),
		std::max<int>((mf_cl_size.y - best_size.y) / 2, 10),
		std::min<int>(mf_cl_size.x, best_size.x),
		std::min<int>(mf_cl_size.y, best_size.y));

	// drowing txt data size in wxTextCtrl is not same as if drow in dc in case of "chn" locale and Windows
#ifdef WIN32
	cur_size = GetSize();
	cur_client_size = GetClientSize();
	wrap_val = WrapText(m_pDialogText, orig_val, cur_client_size.x - 15, ln);

	while ( (m_pDialogText->XYToPosition(0, ln) == -1) && (ln > 0) )
	{
		ln--;
	}

	if (ln > 1)
	{
		// return wxPoint(0, 0) on Ubuntu
		wxPoint pos1 = m_pDialogText->PositionToCoords(m_pDialogText->XYToPosition(0, 1));
		wxPoint pos2 = m_pDialogText->PositionToCoords(m_pDialogText->XYToPosition(0, ln));

		text_size.y = pos2.y + pos1.y;

		best_size = m_pDialogText->GetSizeFromTextSize(text_size);
		best_size.x += cur_size.x - cur_client_size.x + 15;
		best_size.y += cur_size.y - cur_client_size.y + 10;
		this->SetSize(std::max<int>((mf_cl_size.x - best_size.x) / 2, 0),
			std::max<int>((mf_cl_size.y - best_size.y) / 2, 10),
			std::min<int>(mf_cl_size.x, best_size.x),
			std::min<int>(mf_cl_size.y, best_size.y));
	}
#endif

	wxSize dt_size = GetClientSize();
	dt_size.x -= 5;

	this->GetSizer()->SetItemMinSize(m_pDialogText, dt_size);
	this->GetSizer()->Layout();
}

void CMainFrame::OnAppCMDArgsInfo(wxCommandEvent& event)
{
	wxSize cl_size = this->GetClientSize();
	wxSize msg_size((9*cl_size.x/10), (9*cl_size.y/10));
	MyMessageBox msg_dlg(this, g_pParser->GetUsageString(),
		wxT("VideoSubFinder " VSF_VERSION),
		&(g_cfg.m_notebook_colour),
		wxPoint((cl_size.x - msg_size.x) / 2, (cl_size.y - msg_size.y) / 2),
		msg_size);
	msg_dlg.ShowModal();
}

void CMainFrame::OnAppUsageDocs(wxCommandEvent& event)
{
	wxSize cl_size = this->GetClientSize();
	wxSize msg_size((9*cl_size.x/10), (9*cl_size.y/10));
	wxString docs_sub_parth = wxString(wxT("/Docs/readme_")) + g_cfg.m_prefered_locale + wxT(".txt");
	wxString docs_full_parth = g_app_dir + docs_sub_parth;

	if (wxFileExists(docs_full_parth))
	{
		wxString str;

		{
			wxFileInputStream ffin(docs_full_parth);
			wxTextInputStream fin(ffin, wxT("\x09"), wxConvUTF8);

			while (ffin.IsOk() && !ffin.Eof())
			{
				str += fin.ReadLine();
				if (ffin.IsOk() && !ffin.Eof())
				{
					str += wxT("\n");
				}
			}
		}

		MyMessageBox msg_dlg(this, g_cfg.m_menu_app_usage_docs + wxT(": ") + docs_sub_parth + wxT("\n\n") + str,
			wxT("VideoSubFinder " VSF_VERSION),
			&(g_cfg.m_main_text_ctls_background_colour),
			wxPoint((cl_size.x - msg_size.x) / 2, (cl_size.y - msg_size.y) / 2),
			msg_size);
		msg_dlg.ShowModal();
	}
}

void CMainFrame::OnAppOpenLink(wxCommandEvent& event)
{
	wxString link;
	int id = event.GetId();

	switch (id)
	{
		case ID_APP_WEBSITE:
			link = wxString(wxT("https://sourceforge.net/projects/videosubfinder/"));
			break;

		case ID_APP_FORUM:
			link = wxString(wxT("https://sourceforge.net/p/videosubfinder/discussion/684990/"));
			break;

		case ID_APP_BUG_TRACKER:
			link = wxString(wxT("https://sourceforge.net/p/videosubfinder/bugs/"));
			break;
	}

	wxLaunchDefaultBrowser(link);
}

void CMainFrame::OnAppAbout(wxCommandEvent& event)
{
	wxSize cl_size = this->GetClientSize();
	wxSize msg_size(830, 220);
	MyMessageBox msg_dlg(this,
		wxString(wxT("\n")) + 
		g_cfg.m_help_desc_app_about + 
		wxString(wxT("\n\n")) +
		g_cfg.m_help_desc_app_about_dev_email + wxString(wxT(" mailto:skosnits@gmail.com")) +
		wxString(wxT("\n\n")) +
		g_cfg.m_help_desc_app_about_dev_contact + wxString(wxT(" https://vk.com/skosnits")) +
		wxString(wxT("\n\n")) +
		g_cfg.m_help_desc_app_about_dev_donate + wxString(wxT(" https://sourceforge.net/projects/videosubfinder/donate")) +
		wxString(wxT("\n")),
		wxT("VideoSubFinder " VSF_VERSION),
		&(g_cfg.m_notebook_colour),
		wxPoint((cl_size.x - msg_size.x) / 2, (cl_size.y - msg_size.y) / 2),
		msg_size);
	msg_dlg.ShowModal();
}

class CFontsDialog : public wxDialog, public CControl
{
public:
	CMainFrame* m_pMF;

	CChoice* m_pMainFontNameChoice;
	CChoice* m_pMainFontSizeChoice;
	CStaticBox* m_pGBMainFont;
	CStaticText* m_plblMainFontSize;
	CStaticText* m_plblMainFontName;
	CCheckBox* m_pcbMainFontBold;
	CCheckBox* m_pcbMainFontItalic;
	CCheckBox* m_pcbMainFontUnderline;

	CChoice* m_pButtonsFontNameChoice;
	CChoice* m_pButtonsFontSizeChoice;
	CStaticBox* m_pGBButtonsFont;
	CStaticText* m_plblButtonsFontSize;
	CStaticText* m_plblButtonsFontName;
	CCheckBox* m_pcbButtonsFontBold;
	CCheckBox* m_pcbButtonsFontItalic;
	CCheckBox* m_pcbButtonsFontUnderline;

	wxStaticBoxSizer* m_p_main_font_gb_sizer;
	wxStaticBoxSizer* m_p_buttons_font_gb_sizer;

	CFontsDialog(CMainFrame* pMF, const wxString& caption,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	~CFontsDialog()
	{
	}

	void OnKeyDown(wxKeyEvent& event);
	void OnChangesEvent(wxCommandEvent& evt);

	void UpdateSize() override;
	void RefreshData() override;
};

CFontsDialog::CFontsDialog(CMainFrame* pMF, const wxString& caption,
	const wxPoint& pos,
	const wxSize& size) : wxDialog(pMF, wxID_ANY, caption, pos, size)
{
	m_pMF = pMF;

	this->SetFont(m_pMF->m_LBLFont);
	this->SetForegroundColour(g_cfg.m_main_text_colour);
	this->SetBackgroundColour(g_cfg.m_notebook_colour);

	wxArrayString validFaceNames;
	{
		wxFontEnumerator fontEnumerator;
		fontEnumerator.EnumerateFacenames(wxFONTENCODING_DEFAULT, false);
		wxArrayString sysFaceNames = fontEnumerator.GetFacenames();

		validFaceNames.Add(wxT("default"));
		for (const wxString& faceName : sysFaceNames)
		{
			if (wxFontEnumerator::IsValidFacename(faceName))
			{
				validFaceNames.Add(faceName);
			}
		}
	}

	wxArrayString fontSizes;
	{		
		for (int font_size = 1; font_size <= g_max_font_size; font_size++)
		{
			fontSizes.Add(wxString::Format(wxT("%d"), font_size));
		}
	}

	//------------------------------
	
	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pGBMainFont...\n");
	m_pGBMainFont = new CStaticBox(this, wxID_ANY, g_cfg.m_fd_main_font_gb_label);
	m_pGBMainFont->SetFont(m_pMF->m_LBLFont);
	m_pGBMainFont->SetTextColour(g_cfg.m_main_text_colour);
	m_pGBMainFont->SetBackgroundColour(g_cfg.m_notebook_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_plblMainFontSize...\n");
	m_plblMainFontSize = new CStaticText(m_pGBMainFont, g_cfg.m_fd_font_size, wxID_ANY);
	m_plblMainFontSize->SetFont(m_pMF->m_LBLFont);
	m_plblMainFontSize->SetTextColour(g_cfg.m_main_text_colour);
	m_plblMainFontSize->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_plblMainFontName...\n");
	m_plblMainFontName = new CStaticText(m_pGBMainFont, g_cfg.m_fd_font_name, wxID_ANY);
	m_plblMainFontName->SetFont(m_pMF->m_LBLFont);
	m_plblMainFontName->SetTextColour(g_cfg.m_main_text_colour);
	m_plblMainFontName->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbMainFontBold...\n");
	m_pcbMainFontBold = new CCheckBox(m_pGBMainFont, wxID_ANY, &g_cfg.m_main_text_font_bold, g_cfg.m_fd_font_bold);
	m_pcbMainFontBold->SetFont(m_pMF->m_LBLFont);
	m_pcbMainFontBold->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbMainFontBold->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbMainFontItalic...\n");
	m_pcbMainFontItalic = new CCheckBox(m_pGBMainFont, wxID_ANY, &g_cfg.m_main_text_font_italic, g_cfg.m_fd_font_italic);
	m_pcbMainFontItalic->SetFont(m_pMF->m_LBLFont);
	m_pcbMainFontItalic->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbMainFontItalic->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbMainFontUnderline...\n");
	m_pcbMainFontUnderline = new CCheckBox(m_pGBMainFont, wxID_ANY, &g_cfg.m_main_text_font_underline, g_cfg.m_fd_font_underline);
	m_pcbMainFontUnderline->SetFont(m_pMF->m_LBLFont);
	m_pcbMainFontUnderline->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbMainFontUnderline->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	m_pMainFontNameChoice = new CChoice(m_pGBMainFont, validFaceNames, &g_cfg.m_main_text_font);
	m_pMainFontNameChoice->SetFont(m_pMF->m_LBLFont);
	m_pMainFontNameChoice->SetTextColour(g_cfg.m_main_text_colour);
	m_pMainFontNameChoice->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);

	m_pMainFontSizeChoice = new CChoice(m_pGBMainFont, fontSizes, &g_cfg.m_main_text_font_size);
	m_pMainFontSizeChoice->SetFont(m_pMF->m_LBLFont);
	m_pMainFontSizeChoice->SetTextColour(g_cfg.m_main_text_colour);
	m_pMainFontSizeChoice->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);

	//------------------------------

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pGBButtonsFont...\n");
	m_pGBButtonsFont = new CStaticBox(this, wxID_ANY, g_cfg.m_fd_buttons_font_gb_label);
	m_pGBButtonsFont->SetFont(m_pMF->m_LBLFont);
	m_pGBButtonsFont->SetTextColour(g_cfg.m_main_text_colour);
	m_pGBButtonsFont->SetBackgroundColour(g_cfg.m_notebook_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_plblButtonsFontSize...\n");
	m_plblButtonsFontSize = new CStaticText(m_pGBButtonsFont, g_cfg.m_fd_font_size, wxID_ANY);
	m_plblButtonsFontSize->SetFont(m_pMF->m_LBLFont);
	m_plblButtonsFontSize->SetTextColour(g_cfg.m_main_text_colour);
	m_plblButtonsFontSize->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_plblButtonsFontName...\n");
	m_plblButtonsFontName = new CStaticText(m_pGBButtonsFont, g_cfg.m_fd_font_name, wxID_ANY);
	m_plblButtonsFontName->SetFont(m_pMF->m_LBLFont);
	m_plblButtonsFontName->SetTextColour(g_cfg.m_main_text_colour);
	m_plblButtonsFontName->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbButtonsFontBold...\n");
	m_pcbButtonsFontBold = new CCheckBox(m_pGBButtonsFont, wxID_ANY, &g_cfg.m_buttons_text_font_bold, g_cfg.m_fd_font_bold);
	m_pcbButtonsFontBold->SetFont(m_pMF->m_LBLFont);
	m_pcbButtonsFontBold->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbButtonsFontBold->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbButtonsFontItalic...\n");
	m_pcbButtonsFontItalic = new CCheckBox(m_pGBButtonsFont, wxID_ANY, &g_cfg.m_buttons_text_font_italic, g_cfg.m_fd_font_italic);
	m_pcbButtonsFontItalic->SetFont(m_pMF->m_LBLFont);
	m_pcbButtonsFontItalic->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbButtonsFontItalic->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	SaveToReportLog("CFontsDialog::CFontsDialog(): init m_pcbButtonsFontUnderline...\n");
	m_pcbButtonsFontUnderline = new CCheckBox(m_pGBButtonsFont, wxID_ANY, &g_cfg.m_buttons_text_font_underline, g_cfg.m_fd_font_underline);
	m_pcbButtonsFontUnderline->SetFont(m_pMF->m_LBLFont);
	m_pcbButtonsFontUnderline->SetTextColour(g_cfg.m_main_text_colour);
	m_pcbButtonsFontUnderline->SetBackgroundColour(g_cfg.m_main_labels_background_colour);

	m_pButtonsFontNameChoice = new CChoice(m_pGBButtonsFont, validFaceNames, &g_cfg.m_buttons_text_font);
	m_pButtonsFontNameChoice->SetFont(m_pMF->m_LBLFont);
	m_pButtonsFontNameChoice->SetTextColour(g_cfg.m_main_text_colour);
	m_pButtonsFontNameChoice->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);

	m_pButtonsFontSizeChoice = new CChoice(m_pGBButtonsFont, fontSizes, &g_cfg.m_buttons_text_font_size);
	m_pButtonsFontSizeChoice->SetFont(m_pMF->m_LBLFont);
	m_pButtonsFontSizeChoice->SetTextColour(g_cfg.m_main_text_colour);
	m_pButtonsFontSizeChoice->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);
	
	//------------------------------

	int gb_y_offset = std::max<int>(m_pGBMainFont->GetTextExtent(m_pGBMainFont->GetLabel()).GetHeight(), m_pGBButtonsFont->GetTextExtent(m_pGBButtonsFont->GetLabel()).GetHeight()) + 2;

	wxFlexGridSizer* main_font_hor_box_sizer_ctrls_1 = new wxFlexGridSizer(1, 4, 0, 2);
	main_font_hor_box_sizer_ctrls_1->Add(m_plblMainFontName, 0, wxEXPAND | wxALL);
	main_font_hor_box_sizer_ctrls_1->Add(m_pMainFontNameChoice, 0, wxEXPAND | wxALL);
	main_font_hor_box_sizer_ctrls_1->Add(m_plblMainFontSize, 0, wxEXPAND | wxALL);
	main_font_hor_box_sizer_ctrls_1->Add(m_pMainFontSizeChoice, 0, wxEXPAND | wxALL);
	
	wxGridSizer* main_font_hor_box_sizer_ctrls_2 = new wxGridSizer(1, 3, 0, 2);
	main_font_hor_box_sizer_ctrls_2->Add(m_pcbMainFontBold, 0, wxEXPAND | wxALL);
	main_font_hor_box_sizer_ctrls_2->Add(m_pcbMainFontItalic, 0, wxEXPAND | wxALL);
	main_font_hor_box_sizer_ctrls_2->Add(m_pcbMainFontUnderline, 0, wxEXPAND | wxALL);

	wxFlexGridSizer* main_font_vert_box_sizer_all_ctrls = new wxFlexGridSizer(2, 1, 6, 0);
	main_font_vert_box_sizer_all_ctrls->Add(main_font_hor_box_sizer_ctrls_1, 0, wxEXPAND | wxALL);
	main_font_vert_box_sizer_all_ctrls->Add(main_font_hor_box_sizer_ctrls_2, 0, wxEXPAND | wxALL);

	m_p_main_font_gb_sizer = new wxStaticBoxSizer(m_pGBMainFont, wxVERTICAL);
	m_p_main_font_gb_sizer->Add(main_font_vert_box_sizer_all_ctrls, 0, wxEXPAND | wxALL);

	//------------------------------

	wxBoxSizer* buttons_font_hor_box_sizer_ctrls_1 = new wxBoxSizer(wxHORIZONTAL);
	buttons_font_hor_box_sizer_ctrls_1->Add(m_plblButtonsFontName, 0, wxEXPAND | wxALL);
	buttons_font_hor_box_sizer_ctrls_1->AddSpacer(2);
	buttons_font_hor_box_sizer_ctrls_1->Add(m_pButtonsFontNameChoice, 0, wxEXPAND | wxALL);
	buttons_font_hor_box_sizer_ctrls_1->AddSpacer(6);
	buttons_font_hor_box_sizer_ctrls_1->Add(m_plblButtonsFontSize, 0, wxEXPAND | wxALL);
	buttons_font_hor_box_sizer_ctrls_1->AddSpacer(2);
	buttons_font_hor_box_sizer_ctrls_1->Add(m_pButtonsFontSizeChoice, 0, wxEXPAND | wxALL);

	wxGridSizer* buttons_font_hor_box_sizer_ctrls_2 = new wxGridSizer(1, 3, 0, 2);
	buttons_font_hor_box_sizer_ctrls_2->Add(m_pcbButtonsFontBold, 0, wxEXPAND | wxALL);
	buttons_font_hor_box_sizer_ctrls_2->Add(m_pcbButtonsFontItalic, 0, wxEXPAND | wxALL);
	buttons_font_hor_box_sizer_ctrls_2->Add(m_pcbButtonsFontUnderline, 0, wxEXPAND | wxALL);

	wxFlexGridSizer* buttons_font_vert_box_sizer_all_ctrls = new wxFlexGridSizer(2, 1, 6, 0);
	buttons_font_vert_box_sizer_all_ctrls->Add(buttons_font_hor_box_sizer_ctrls_1, 0, wxEXPAND | wxALL);
	buttons_font_vert_box_sizer_all_ctrls->Add(buttons_font_hor_box_sizer_ctrls_2, 0, wxEXPAND | wxALL);

	m_p_buttons_font_gb_sizer = new wxStaticBoxSizer(m_pGBButtonsFont, wxVERTICAL);
	m_p_buttons_font_gb_sizer->Add(buttons_font_vert_box_sizer_all_ctrls, 0, wxEXPAND | wxALL);

	wxBoxSizer* vert_box_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hor_box_sizer = new wxBoxSizer(wxHORIZONTAL);

	vert_box_sizer->Add(m_p_main_font_gb_sizer, 0, wxALIGN_CENTER);
	vert_box_sizer->AddSpacer(20);
	vert_box_sizer->Add(m_p_buttons_font_gb_sizer, 0, wxALIGN_CENTER);

	hor_box_sizer->Add(vert_box_sizer, 1, wxALIGN_CENTER);

	this->SetSizer(hor_box_sizer);

	CControl::RefreshAllControlsData();
	CControl::UpdateAllControlsSize();

	//------------------------------

	Bind(wxEVT_CHAR_HOOK, &CFontsDialog::OnKeyDown, this);
	Bind(wxEVT_CHECKBOX, &CFontsDialog::OnChangesEvent, this);
	Bind(wxEVT_CHOICE, &CFontsDialog::OnChangesEvent, this);
	Bind(wxEVT_MOUSEWHEEL, &CMainFrame::OnMouseWheel, m_pMF);
}

void CFontsDialog::OnKeyDown(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		EndModal(0);
	}
}

void CFontsDialog::OnChangesEvent(wxCommandEvent& evt)
{
	m_pMF->UpdateTextSizes();
}

void CFontsDialog::UpdateSize()
{
	wxSize best_size = this->GetSizer()->GetMinSize();
	wxSize cur_size = GetSize();
	wxSize cur_client_size = GetClientSize();
	best_size.x += cur_size.x - cur_client_size.x + 20;
	best_size.y += cur_size.y - cur_client_size.y + 20;

	this->SetSize(best_size);

	this->GetSizer()->Layout();
}

void CFontsDialog::RefreshData()
{
	this->SetFont(m_pMF->m_LBLFont);
	this->SetForegroundColour(g_cfg.m_main_text_colour);
	this->SetBackgroundColour(g_cfg.m_notebook_colour);

	Refresh();
}

void CMainFrame::OnFonts(wxCommandEvent& event)
{
	wxSize cl_size = this->GetClientSize();
	wxSize dlg_size(750, 300);

	CFontsDialog dlg(this,
		wxT("VideoSubFinder " VSF_VERSION),
		wxPoint((cl_size.x - dlg_size.x) / 2, (cl_size.y - dlg_size.y) / 2),
		dlg_size);

	dlg.ShowModal();
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

void UpdateSettingsInFile(wxString SettingsFilePath, std::map<wxString, wxString> settings)
{
	custom_buffer<char> data;
	{
		wxFileInputStream ffin(SettingsFilePath);
		size_t size = ffin.GetSize();
		data.set_size(size + 1);
		ffin.ReadAll(data.m_pData, size);
		data.m_pData[size] = '\0';
	}

	wxString str = wxString(data.m_pData, wxConvUTF8);
	str.Replace(wxString(wxT("\r")), wxString(), true);

	for (std::map<wxString, wxString>::const_reference val : settings)
	{
		wxString search = wxString::Format(wxT("(^|\n)(%s[[:space:]]*=[^\n]+)"), val.first);
		wxRegEx re(search);
		if (re.Matches(str))
		{
			re.Replace(&str, re.GetMatch(str, 1) + wxString::Format(wxT("%s = %s"), val.first, val.second), 1);
		}
	}

	wxFFileOutputStream ffout(SettingsFilePath);
	wxTextOutputStream fout(ffout);
	fout << str;
	fout.Flush();
	ffout.Close();
}

template<typename T>
void WriteProperty(wxTextOutputStream& fout, T val, wxString Name)
{
	fout << Name << " = " << val << '\n';
}

template<>
void WriteProperty<>(wxTextOutputStream& fout, s64 val, wxString Name)
{
	fout << Name << " = " << (wxLongLong)val << '\n';
}

template<>
void WriteProperty<>(wxTextOutputStream& fout, wxString val, wxString Name)
{
	val = wxJoin(wxSplit(val, '\n'), ';');
	fout << Name << " = " << val << '\n';
}

template<>
void WriteProperty<>(wxTextOutputStream& fout, wxArrayString val, wxString Name)
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

template<>
void WriteProperty<>(wxTextOutputStream& fout, wxColour val, wxString Name)
{
	fout << Name << " = " << wxString::Format(wxT("%d,%d,%d"), (int)val.Red(), (int)val.Green(), (int)val.Blue()) << '\n';
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

bool ReadProperty(std::map<wxString, wxString>& settings, s64& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		wxString _val = search->second;
		wxInt64 llval;
		_val.ToLongLong(&llval);
		val = llval;
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

bool ReadProperty(std::map<wxString, wxString>& settings, wxColour& val, wxString Name)
{
	bool res = false;
	auto search = settings.find(Name);

	if (search != settings.end()) {
		wxString data = search->second;
		
		wxRegEx re(wxT("^[[:space:]]*([[:digit:]]+)[[:space:]]*,[[:space:]]*([[:digit:]]+)[[:space:]]*,[[:space:]]*([[:digit:]]+)[[:space:]]*$"));
		if (re.Matches(data))
		{
			val = wxColour(wxAtoi(re.GetMatch(data, 1)), wxAtoi(re.GetMatch(data, 2)), wxAtoi(re.GetMatch(data, 3)));
			res = true;
		}		
	}

	return res;
}

BEGIN_EVENT_TABLE(CPopupHelpWindow, wxFrame)
	EVT_KILL_FOCUS(CPopupHelpWindow::OnKillFocus)
	EVT_MOUSE_CAPTURE_LOST(CPopupHelpWindow::OnMouseCaptureLost)
	EVT_KEY_DOWN(CPopupHelpWindow::OnKeyDown)
END_EVENT_TABLE()

CPopupHelpWindow::CPopupHelpWindow(const wxString& help_msg) : wxFrame(NULL, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, wxFRAME_NO_TASKBAR), m_help_msg(help_msg)
{
	m_pST = new CStaticText(this, m_help_msg, wxID_ANY);
	m_pST->SetFont(g_pMF->m_LBLFont);
	m_pST->SetTextColour(g_cfg.m_main_text_colour);
	m_pST->SetBackgroundColour(g_cfg.m_notebook_colour);

	wxBoxSizer* vert_box_sizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* hor_box_sizer = new wxBoxSizer(wxHORIZONTAL);

	vert_box_sizer->Add(m_pST, 0, wxALIGN_CENTER, 0);
	hor_box_sizer->Add(vert_box_sizer, 1, wxALIGN_CENTER);
	this->SetSizer(hor_box_sizer);

	m_pST->Bind(wxEVT_KILL_FOCUS, &CPopupHelpWindow::OnKillFocus, this);
	m_pST->Bind(wxEVT_MOUSE_CAPTURE_LOST, &CPopupHelpWindow::OnMouseCaptureLost, this);
	m_pST->Bind(wxEVT_KEY_DOWN, &CPopupHelpWindow::OnKeyDown, this);
}

void CPopupHelpWindow::Popup()
{
	this->Hide();

	this->SetBackgroundColour(g_cfg.m_notebook_colour);
	
	m_pST->RefreshData();

	wxSize cur_size = this->GetSize();
	wxSize cur_client_size = this->GetClientSize();
	wxSize best_size = m_pST->GetOptimalSize();

	best_size.x += cur_size.x - cur_client_size.x;
	best_size.y += cur_size.x - cur_client_size.x;

	wxPoint mp = wxGetMousePosition();

	// if dont do SetClientSize initial SetSize work incorrectly on Linux
	this->SetClientSize(best_size);
	this->SetSize(best_size);

	this->GetSizer()->Layout();

	this->Show();

	// set pos work correctly only after show on linux
	this->SetPosition(mp);	
}

void CPopupHelpWindow::OnKillFocus(wxFocusEvent& event)
{
	this->Hide();
}

void CPopupHelpWindow::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	this->Hide();
}

void CPopupHelpWindow::OnKeyDown(wxKeyEvent& event)
{
	if (event.GetKeyCode() == WXK_ESCAPE)
	{
		this->Hide();
	}
}
