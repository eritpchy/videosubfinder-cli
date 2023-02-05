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

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "c", "clear_dirs", "Clear Folders (remove all images), performed before any other steps" },
	{ wxCMD_LINE_SWITCH, "r", "run_search", "Run Search (find frames with hardcoded text (hardsub) on video)" },	
	{ wxCMD_LINE_SWITCH, "ccti", "create_cleared_text_images", "Create Cleared Text Images" },
	{ wxCMD_LINE_OPTION, "ces", "create_empty_sub", "Create Empty Sub With Provided Output File Name (*.ass or *.srt)" },
	{ wxCMD_LINE_OPTION, "cscti", "create_sub_from_cleared_txt_images", "Create Sub From Cleared TXT Images With Provided Output File Name (*.ass or *.srt)" },
	{ wxCMD_LINE_OPTION, "cstxt", "create_sub_from_txt_results", "Create Sub From TXT Results With Provided Output File Name (*.ass or *.srt)" },
	{ wxCMD_LINE_OPTION, "i", "input_video", "input video file" },
	{ wxCMD_LINE_SWITCH, "ovocv", "open_video_opencv", "open video by OpenCV (default)" },
	{ wxCMD_LINE_SWITCH, "ovffmpeg", "open_video_ffmpeg", "open video by FFMPEG" },
	{ wxCMD_LINE_SWITCH, "uc", "use_cuda", "use cuda" },
	{ wxCMD_LINE_OPTION, "s", "start_time", "start time, default = 0:00:00:000 (in format hour:min:sec:milisec)" },
	{ wxCMD_LINE_OPTION, "e", "end_time", "end time, default = video length" },
	{ wxCMD_LINE_OPTION, "te", "top_video_image_percent_end", "top video image percent offset from image bottom, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "be", "bottom_video_image_percent_end", "bottom video image percent offset from image bottom, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },	
	{ wxCMD_LINE_OPTION, "le", "left_video_image_percent_end", "left video image percent end, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "re", "right_video_image_percent_end", "right video image percent end, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "o", "output_dir",  "output dir (root directory where results will be stored)" },
	{ wxCMD_LINE_OPTION, "gs", "general_settings",  "general settings (path to general settings *.cfg file, default = settings/general.cfg)" },
	{ wxCMD_LINE_OPTION, "nthr", "num_threads", "number of threads used for Run Search", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_OPTION, "nocrthr", "num_ocr_threads", "number of threads used for Create Cleared TXT Images", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_SWITCH, "h", "help", "show this help message\n\nExample of usage:\nVideoSubFinderWXW.exe -c -r -ccti -i \"C:\\test_video.mp4\" -cscti \"C:\\test_video.srt\" -o \"C:\\ResultsDir\" -te 0.5 -be 0.1 -le 0.1 -re 0.9 -s 0:00:10:300 -e 0:00:13:100\n" },
	{ wxCMD_LINE_NONE }
};

wxString g_ReportFileName;
wxCmdLineParser g_parser;

void SaveToReportLog(wxString msg, wxString mode)
{
	wxFFileOutputStream ffout(g_ReportFileName, mode);
	wxTextOutputStream fout(ffout);
	fout << msg;
	fout.Flush();
	ffout.Close();
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
	simple_buffer<u8> Im(w * h, 1);

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
	simple_buffer<u8> Im1(1920 * 1080, 1);
	simple_buffer<u8> Im2(1920 * 1080, 2);
	simple_buffer<u8> Im3(1920 * 1080, 3);
	simple_buffer<u8> Im4(1920 * 1080, 4);
	simple_buffer<u8> Im5(1920 * 1080, 5);

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
	vector<custom_task> thrs;
	thrs = vector<custom_task>(N, create_custom_task([] {}));
	wait_all(begin(thrs), end(thrs));

	simple_buffer<u32> Im1(1920 * 1080, 1);
	simple_buffer<u32> Im2(1920 * 1080, 2);

	thrs[0] = create_custom_task([&] { test_func_fast(Im1); });
	thrs[1] = create_custom_task([&] { test_func_fast(Im2); });

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

	SaveToReportLog("Starting program...\n", wxT("wb"));
	SaveToReportLog("CVideoSubFinderApp::Initialize...\n");

	//test();

	g_parser.SetDesc(cmdLineDesc);
	g_parser.SetCmdLine(argc, argv);
	
	if (g_parser.Parse() != 0)
	{
		return false;
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
	m_pMainWnd = new CMainFrame("VideoSubFinder " VSF_VERSION " Version");
	SaveToReportLog("CMainFrame was created.\n");

	wxString wxStr;
	bool blnNeedToExit = false;	
	
	if (g_parser.Found("gs", &wxStr))
	{
		wxStr.Replace("\\", "/");
		m_pMainWnd->m_GeneralSettingsFileName = wxStr;
	}

	SaveToReportLog("m_pMainWnd->Init...\n");
	m_pMainWnd->Init();	
	SaveToReportLog("m_pMainWnd->Init was finished.\n");

	long threads;
	if (g_parser.Found("nthr", &threads))
	{
		g_threads = threads;
	}

	long ocr_threads;
	if (g_parser.Found("nocrthr", &ocr_threads))
	{
		g_ocr_threads = ocr_threads;
	}	

	if (g_parser.FoundSwitch("ovocv"))
	{
		m_pMainWnd->m_type = 0;
	}
	else if (g_parser.FoundSwitch("ovffmpeg"))
	{
		m_pMainWnd->m_type = 1;
	}

	if (g_parser.FoundSwitch("uc"))
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

	bool blnI = g_parser.Found("i", &(m_pMainWnd->m_FileName));
	if (g_parser.Found("o", &wxStr))
	{
		wxStr.Replace("\\", "/");
		g_work_dir = wxStr;		
	}

	wxFileName::Mkdir(g_work_dir + "/RGBImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ISAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ILAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TestImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTImagesJoined", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTResults", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	if (wxCMD_SWITCH_ON == g_parser.FoundSwitch("c"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CLEAR);
		m_pMainWnd->m_pPanel->m_pSHPanel->OnBnClickedClear(bn_event);
	}	

	if (wxCMD_SWITCH_ON == g_parser.FoundSwitch("r"))
	{
		if (blnI)
		{
			m_pMainWnd->m_blnNoGUI = true;

			if (m_pMainWnd->m_type == 0) m_pMainWnd->m_pVideo = GetOCVVideoObject();
			else if (m_pMainWnd->m_type == 1) m_pMainWnd->m_pVideo = GetFFMPEGVideoObject();

			if (m_pMainWnd->m_pVideo->OpenMovie(m_pMainWnd->m_FileName, NULL, 0))
			{
				m_pMainWnd->m_BegTime = 0;
				m_pMainWnd->m_EndTime = m_pMainWnd->m_pVideo->m_Duration;

				if (g_parser.Found("s", &wxStr))
				{
					m_pMainWnd->m_BegTime = GetVideoTime(wxStr);
				}

				if (g_parser.Found("e", &wxStr))
				{
					m_pMainWnd->m_EndTime = GetVideoTime(wxStr);
				}

				double double_val;				

				if (g_parser.Found("be", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"be\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL2->m_pos = 1 - double_val;
				}

				if (g_parser.Found("te", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"te\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL1->m_pos = 1 - double_val;
				}


				if (g_parser.Found("le", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						wxMessageBox("ERROR: wrong \"le\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL1->m_pos = double_val;
				}

				if (g_parser.Found("re", &double_val))
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

					m_pMainWnd->m_pVideo->SetVideoWindowSettins(m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL1->m_pos,
						m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL2->m_pos,
						m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL1->m_pos,
						m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

					m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread = new ThreadSearchSubtitles(m_pMainWnd, wxTHREAD_JOINABLE);
					m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Create();
					m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Run();
					m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Wait();
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

	if (wxCMD_SWITCH_ON == g_parser.FoundSwitch("ccti"))
	{		
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CCTI);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateClearedTextImages(bn_event);
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_pSearchThread->Wait();
		blnNeedToExit = true;
	}

	if (g_parser.Found("ces", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CES);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateEmptySub(bn_event);
		blnNeedToExit = true;
	}

	if (g_parser.Found("cscti", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSCTI);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromClearedTXTImages(bn_event);
		blnNeedToExit = true;
	}

	if (g_parser.Found("cstxt", &wxStr))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSTXT);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_sub_path = wxStr;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromTXTResults(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == g_parser.FoundSwitch("h"))
	{		
		cout << g_parser.GetUsageString();
		wxCommandEvent send_event;
		m_pMainWnd->OnAppAbout(send_event);
		//g_parser.Usage();
		blnNeedToExit = true;
	}

	if (blnNeedToExit) return false;

	m_pMainWnd->Show(true);

	return true;
}

CVideoSubFinderApp::~CVideoSubFinderApp()
{
}

