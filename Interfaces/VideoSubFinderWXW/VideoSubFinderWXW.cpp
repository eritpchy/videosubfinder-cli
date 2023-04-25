						    //VideoSubFinderWXW.cpp//                                
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

#include "VideoSubFinderWXW.h"
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>

#ifndef WIN32
#include <X11/Xlib.h>
#endif

wxString g_ReportFileName;
wxString g_GeneralSettingsFileName;
wxString g_ErrorFileName;
wxCmdLineParser *g_pParser = NULL;

void SetParserDescription()
{
	std::map<wxString, wxString> general_settings;
	ReadSettings(g_GeneralSettingsFileName, general_settings);

	vector<wxString> keys;
	transform(begin(general_settings), end(general_settings), back_inserter(keys), [](decltype(general_settings)::value_type const& pair) {return pair.first;});
	std::sort(keys.begin(), keys.end());

	g_pParser->AddSwitch("c", "clear_dirs", g_cfg.m_help_desc_for_clear_dirs);
	g_pParser->AddSwitch("r", "run_search", g_cfg.m_help_desc_for_run_search);
	g_pParser->AddSwitch("ccti", "create_cleared_text_images", g_cfg.m_help_desc_for_create_cleared_text_images);
	g_pParser->AddSwitch("ji", "join_images", g_cfg.m_help_desc_for_join_images);
	g_pParser->AddOption("ces", "create_empty_sub", g_cfg.m_help_desc_for_create_empty_sub);
	g_pParser->AddOption("cscti", "create_sub_from_cleared_txt_images", g_cfg.m_help_desc_for_create_sub_from_cleared_txt_images);
	g_pParser->AddOption("cstxt", "create_sub_from_txt_results", g_cfg.m_help_desc_for_create_sub_from_txt_results);
	g_pParser->AddOption("i", "input_video", g_cfg.m_help_desc_for_input_video);
	g_pParser->AddSwitch("ovocv", "open_video_opencv", g_cfg.m_help_desc_for_open_video_opencv);
	g_pParser->AddSwitch("ovffmpeg", "open_video_ffmpeg", g_cfg.m_help_desc_for_open_video_ffmpeg);
	g_pParser->AddSwitch("uc", "use_cuda", g_cfg.m_help_desc_for_use_cuda);
	g_pParser->AddOption("s", "start_time", g_cfg.m_help_desc_for_start_time);
	g_pParser->AddOption("e", "end_time", g_cfg.m_help_desc_for_end_time);
	g_pParser->AddOption("te", "top_video_image_percent_end", g_cfg.m_help_desc_for_top_video_image_percent_end, wxCMD_LINE_VAL_DOUBLE);
	g_pParser->AddOption("be", "bottom_video_image_percent_end", g_cfg.m_help_desc_for_bottom_video_image_percent_end, wxCMD_LINE_VAL_DOUBLE);
	g_pParser->AddOption("le", "left_video_image_percent_end", g_cfg.m_help_desc_for_left_video_image_percent_end, wxCMD_LINE_VAL_DOUBLE);
	g_pParser->AddOption("re", "right_video_image_percent_end", g_cfg.m_help_desc_for_right_video_image_percent_end, wxCMD_LINE_VAL_DOUBLE);
	g_pParser->AddOption("o", "output_dir",  g_cfg.m_help_desc_for_output_dir);
	g_pParser->AddOption("gs", "general_settings",  g_cfg.m_help_desc_for_general_settings);
	g_pParser->AddOption("nthr", "num_threads", g_cfg.m_help_desc_for_num_threads, wxCMD_LINE_VAL_NUMBER);
	g_pParser->AddOption("nocrthr", "num_ocr_threads", g_cfg.m_help_desc_for_num_ocr_threads, wxCMD_LINE_VAL_NUMBER);
	g_pParser->AddSwitch("h", "help", g_cfg.m_help_desc_for_help);

	for (wxString& key : keys)
	{
		g_pParser->AddOption(key);
	}
}

void PlaySound(wxString path)
{
	if (wxFileExists(path))
	{
#ifdef WIN32
		if (PlaySound(path.c_str(), NULL, SND_ASYNC) == TRUE)
		{
			SaveToReportLog("PlaySound: PlaySound(path.c_str(), NULL, SND_ASYNC) == TRUE\n");
		}
		else
		{
			SaveToReportLog("ERROR: PlaySound: PlaySound(path.c_str(), NULL, SND_ASYNC) != TRUE\n");
		}
#else
		// Unfortunately wxWidgets doesn't play sound
		SaveToReportLog("PlaySound: play sound by canberra-gtk-play ...\n");
		system(wxString::Format(wxT("canberra-gtk-play -f \"%s\""), path).c_str());
#endif
	}
	else
	{
		SaveToReportLog(wxString::Format(wxT("ERROR: PlaySound: \"%s\" not found\n"), path));
	}
}

template <typename T>
void test_func(simple_buffer<T>& Im)
{
	for (int i = 0; i < Im.size(); i++)
	{
		for (int j = 0; j < 1920*10; j++)
		{
			Im[i] += Im[j];
		}
	}
}

template <typename T>
void test_func_fast(simple_buffer<T>& Im)
{
	for (int i = 0; i < Im.size(); i++)
	{
		for (int j = 0; j < 1920; j++)
		{
			Im[i] += 1;
		}
	}
}

void test1()
{
	int w = 1920;
	int h = 1080;
	simple_buffer<u8> Im(w * h, (u8)1);

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(h), [&](int y)
	{
		for (int i = w * y; i < w * (y + 1); i++)
		{
			for (int j = 0; j < 1920 * 10; j++)
			{
				Im[i] += Im[j];
			}
		}
	});
}

void test2()
{
	simple_buffer<u8> Im1(1920 * 1080, (u8)1);
	simple_buffer<u8> Im2(1920 * 1080, (u8)2);
	simple_buffer<u8> Im3(1920 * 1080, (u8)3);
	simple_buffer<u8> Im4(1920 * 1080, (u8)4);
	simple_buffer<u8> Im5(1920 * 1080, (u8)5);

	run_in_parallel(
		[&] { test_func(Im1); },
		[&] { test_func(Im2); },
		[&] { test_func(Im3); },
		[&] { test_func(Im4); },
		[&] { test_func(Im5); }
	);
}

void test3()
{
	int N = 64;
	vector<shared_custom_task> thrs;
	thrs = vector<shared_custom_task>(N, shared_custom_task([] {}));
	wait_all(begin(thrs), end(thrs));

	simple_buffer<u32> Im1(1920 * 1080, (u32)1);
	simple_buffer<u32> Im2(1920 * 1080, (u32)2);

	thrs[0] = shared_custom_task([&] { test_func_fast(Im1); });
	thrs[1] = shared_custom_task([&] { test_func_fast(Im2); });

	wait_all(begin(thrs), end(thrs));

	N = N;
}

void test()
{
	//test1();
	//test2();

	test3();
}

bool CVideoSubFinderApp::Initialize(int& argc, wxChar **argv)
{
#ifndef WIN32
	XInitThreads();
#endif

	wxString Str = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
	Str.Replace("\\", "/");
	g_app_dir = Str;
	g_work_dir = g_app_dir;
	g_ReportFileName = g_app_dir + wxT("/report.log");
	g_ErrorFileName = g_app_dir + wxT("/error.log");
	g_GeneralSettingsFileName = g_app_dir + wxT("/settings/general.cfg");
	g_prev_data_path = g_app_dir + wxT("/previous_data.inf");

	SaveToReportLog("Starting program...\n", wxT("wb"));
	SaveToReportLog("CVideoSubFinderApp::Initialize...\n");

	SaveToReportLog("CVideoSubFinderApp::Initial LoadSettings...\n");
	LoadSettings();

	//test();

	g_pParser = new wxCmdLineParser();
	SetParserDescription();
	g_pParser->SetCmdLine(argc, argv);
	if (g_pParser->Parse() != 0)
	{
		return false;
	}

	SaveToReportLog("CVideoSubFinderApp::LoadSettings again with affect of command line...\n");
	LoadSettings();

	if (g_pParser->Found("gs", &Str))
	{
		Str.Replace("\\", "/");
		g_GeneralSettingsFileName = Str;

		SaveToReportLog("CVideoSubFinderApp::LoadSettings from other path...\n");
		LoadSettings();

		delete g_pParser;
		g_pParser = new wxCmdLineParser();
		SetParserDescription();
		g_pParser->SetCmdLine(argc, argv);
		if (g_pParser->Parse() != 0)
		{
			return false;
		}
	}

	SaveToReportLog("wxApp::Initialize...\n");
	return wxApp::Initialize(argc, argv);
}

bool CVideoSubFinderApp::OnInit() 
{
    //if ( !wxApp::OnInit() )
    //    return false;

#ifdef __WXGTK__
    // Many version of wxGTK generate spurious diagnostic messages when
    // destroying wxNotebook (or removing pages from it), allow wxWidgets to
    // suppress them.
    GTKAllowDiagnosticsControl();	
#endif // __WXGTK__

	SaveToReportLog("new CMainFrame...\n");
	m_pMainWnd = new CMainFrame(wxT("VideoSubFinder " VSF_VERSION));
	SaveToReportLog("CMainFrame was created.\n");

	wxString wxStr;
	bool blnNeedToExit = false;	

	SaveToReportLog("m_pMainWnd->Init...\n");
	m_pMainWnd->Init();	
	SaveToReportLog("m_pMainWnd->Init was finished.\n");

	long threads;
	if (g_pParser->Found("nthr", &threads))
	{
		g_threads = threads;
	}

	long ocr_threads;
	if (g_pParser->Found("nocrthr", &ocr_threads))
	{
		g_ocr_threads = ocr_threads;
	}	

	if (g_pParser->FoundSwitch("ovocv"))
	{
		m_pMainWnd->m_type = 0;
	}
	else if (g_pParser->FoundSwitch("ovffmpeg"))
	{
		m_pMainWnd->m_type = 1;
	}

	if (g_pParser->FoundSwitch("uc"))
	{
		if (g_use_cuda_gpu == false)
		{
			g_use_cuda_gpu = true;

			if (!InitCUDADevice())
			{
				g_use_cuda_gpu = false;
			}
		}
	}

	bool blnI = g_pParser->Found("i", &(m_pMainWnd->m_FileName));
	if (g_pParser->Found("o", &wxStr))
	{
		wxStr.Replace("\\", "/");
		g_work_dir = wxStr;		
	}

	wxFileName::Mkdir(g_work_dir + "/RGBImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ISAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ILAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/DebugImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ImagesJoined", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTResults", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TestImages/RGBImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TestImages/TXTImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	if (wxCMD_SWITCH_ON == g_pParser->FoundSwitch("c"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CLEAR);
		m_pMainWnd->m_pPanel->m_pSHPanel->OnBnClickedClear(bn_event);
	}	

	if (wxCMD_SWITCH_ON == g_pParser->FoundSwitch("r"))
	{
		if (blnI)
		{
			m_pMainWnd->m_blnNoGUI = true;
			SaveToReportLog("setting: m_pMainWnd->m_blnNoGUI = true\n");

			if (m_pMainWnd->m_type == 0) m_pMainWnd->m_pVideo = GetOCVVideoObject();
			else if (m_pMainWnd->m_type == 1) m_pMainWnd->m_pVideo = GetFFMPEGVideoObject();

			if (m_pMainWnd->m_pVideo->OpenMovie(m_pMainWnd->m_FileName, NULL, 0))
			{
				m_pMainWnd->m_BegTime = 0;
				m_pMainWnd->m_EndTime = m_pMainWnd->m_pVideo->m_Duration;

				if (g_pParser->Found("s", &wxStr))
				{
					m_pMainWnd->m_BegTime = GetVideoTime(wxStr);
				}

				if (g_pParser->Found("e", &wxStr))
				{
					m_pMainWnd->m_EndTime = GetVideoTime(wxStr);
				}

				double double_val;				

				if (g_pParser->Found("be", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"be\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL2->m_pos = 1 - double_val;
				}

				if (g_pParser->Found("te", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"te\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL1->m_pos = 1 - double_val;
				}


				if (g_pParser->Found("le", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"le\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL1->m_pos = double_val;
				}

				if (g_pParser->Found("re", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"re\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL2->m_pos = double_val;
				}

				if (m_pMainWnd->m_pVideo->SetNullRender())
				{
					g_color_ranges = GetColorRanges(g_use_filter_color);
					g_outline_color_ranges = GetColorRanges(g_use_outline_filter_color);

					m_pMainWnd->m_pVideo->SetVideoWindowSettins(
						std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos),
						std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos),
						std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos),
						std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos));

					g_IsSearching = 1;
					g_RunSubSearch = 1;
					m_pMainWnd->m_pPanel->m_pSHPanel->m_SearchThread = std::thread(ThreadSearchSubtitles);
					m_pMainWnd->m_pPanel->m_pSHPanel->m_SearchThread.join();
				}
			}
		}
		else
		{
			wxMessageBox("ERROR: input video file was not provided\n");
			return false;
		}

		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == g_pParser->FoundSwitch("ccti"))
	{		
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CCTI);
		m_pMainWnd->m_blnNoGUI = true;
		SaveToReportLog("setting: m_pMainWnd->m_blnNoGUI = true\n");
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateClearedTextImages(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == g_pParser->FoundSwitch("ji"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_JOIN);
		m_pMainWnd->m_blnNoGUI = true;
		SaveToReportLog("setting: m_pMainWnd->m_blnNoGUI = true\n");
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedJoinTXTImages(bn_event);
		blnNeedToExit = true;
	}

	if (g_pParser->Found("ces", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CES);
		m_pMainWnd->m_blnNoGUI = true;
		SaveToReportLog("setting: m_pMainWnd->m_blnNoGUI = true\n");
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateEmptySub(bn_event);
		blnNeedToExit = true;
	}

	if (g_pParser->Found("cscti", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSCTI);
		m_pMainWnd->m_blnNoGUI = true;
		SaveToReportLog("setting: m_pMainWnd->m_blnNoGUI = true\n");
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromClearedTXTImages(bn_event);
		blnNeedToExit = true;
	}

	if (g_pParser->Found("cstxt", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSTXT);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromTXTResults(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == g_pParser->FoundSwitch("h"))
	{		
		#ifdef WIN32
				HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
				DWORD fileType = GetFileType(out);

				AttachConsole(ATTACH_PARENT_PROCESS);

				bool is_redirected = ((fileType == FILE_TYPE_DISK) || (fileType == FILE_TYPE_PIPE));

				if (!is_redirected)
				{
					freopen("CONOUT$", "w", stdout);
				}
		#endif

		cout << g_pParser->GetUsageString() << endl;

		blnNeedToExit = true;
	}

	if (blnNeedToExit) return false;

	m_pMainWnd->Show(true);

	return true;
}

CVideoSubFinderApp::~CVideoSubFinderApp()
{
}

