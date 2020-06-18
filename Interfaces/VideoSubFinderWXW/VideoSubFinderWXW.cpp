						    //VideoSubFinder.h//                                
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

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "c", "clear_dirs", "Clear Folders (remove all images), performed before any other steps" },
	{ wxCMD_LINE_SWITCH, "r", "run_search", "Run Search (find frames with hardcoded text (hardsub) on video)" },	
	{ wxCMD_LINE_SWITCH, "ccti", "create_cleared_text_images", "Create Cleared Text Images" },
	{ wxCMD_LINE_SWITCH, "ces", "create_empty_sub", "Create Empty Sub" },
	{ wxCMD_LINE_SWITCH, "cscti", "create_sub_from_cleared_txt_images", "Create Sub From Cleared TXT Images" },
	{ wxCMD_LINE_SWITCH, "cstxt", "create_sub_from_txt_results", "Create Sub From TXT Results" },
	{ wxCMD_LINE_OPTION, "i", "input_video", "input video file" },
	{ wxCMD_LINE_SWITCH, "ovocv", "open_video_opencv", "open video by OpenCV (default)" },
	{ wxCMD_LINE_SWITCH, "ovffmpeg", "open_video_ffmpeg", "open video by FFMPEG with GPU Acceleration" },
	{ wxCMD_LINE_SWITCH, "uc", "use_cuda", "use cuda" },
	{ wxCMD_LINE_OPTION, "s", "start_time", "start time, default = 0:00:00:000 (in format hour:min:sec:milisec)" },
	{ wxCMD_LINE_OPTION, "e", "end_time", "end time, default = video length" },
	{ wxCMD_LINE_OPTION, "be", "bottom_video_image_percent_end", "bottom video image percent end, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "te", "top_video_image_percent_end", "top video image percent end, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },	
	{ wxCMD_LINE_OPTION, "le", "left_video_image_percent_end", "left video image percent end, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "re", "right_video_image_percent_end", "right video image percent end, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "o", "output_dir",  "output dir (root directory where results will be stored)" },
	{ wxCMD_LINE_OPTION, "nthr", "num_threads", "number of threads used for Run Search", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_OPTION, "nocrthr", "num_ocr_threads", "number of threads used for Create Cleared TXT Images", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_SWITCH, "h", "help", "show this help message\n\n\nExample of usage:\nVideoSubFinderWXW.exe -c -r -ccti -cscti -i \"C:\\test_video.mp4\" -o \"C:\\ResultsDir\" -be 0.1 -te 0.5 -le 0.1 -re 0.9 -s 0:00:10:300 -e 0:00:13:100\n" },
	{ wxCMD_LINE_NONE }
};

bool CVideoSubFinderApp::Initialize(int& argc, wxChar **argv)
{
	m_pMainWnd = new CMainFrame("VideoSubFinder " VSF_VERSION " Version");

	m_pMainWnd->m_parser.SetDesc(cmdLineDesc);
	m_pMainWnd->m_parser.SetCmdLine(argc, argv);
	
	if (m_pMainWnd->m_parser.Parse() != 0)
	{
		return false;
	}

	return wxApp::Initialize(argc, argv);
}

bool CVideoSubFinderApp::OnInit() 
{
	wxString wxStr;
	bool blnNeedToExit = false;	
	
	m_pMainWnd->Init();	

	long threads;
	if (m_pMainWnd->m_parser.Found("nthr", &threads))
	{
		g_threads = threads;
	}

	long ocr_threads;
	if (m_pMainWnd->m_parser.Found("nocrthr", &ocr_threads))
	{
		g_ocr_threads = ocr_threads;
	}	

	if (m_pMainWnd->m_parser.FoundSwitch("ovocv"))
	{
		m_pMainWnd->m_type = 0;
	}
	else if (m_pMainWnd->m_parser.FoundSwitch("ovffmpeg"))
	{
		m_pMainWnd->m_type = 1;
	}

	if (m_pMainWnd->m_parser.FoundSwitch("uc"))
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

	bool blnI = m_pMainWnd->m_parser.Found("i", &(m_pMainWnd->m_FileName));
	if (m_pMainWnd->m_parser.Found("o", &wxStr))
	{
		wxStr.Replace("\\", "/");
		g_work_dir = wxStr;		
	}

	wxFileName::Mkdir(g_work_dir + "/RGBImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ISAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/ILAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TestImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFileName::Mkdir(g_work_dir + "/TXTResults", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("c"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CLEAR);
		m_pMainWnd->m_pPanel->m_pSHPanel->OnBnClickedClear(bn_event);
	}	

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("r"))
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

				if (m_pMainWnd->m_parser.Found("s", &wxStr))
				{
					m_pMainWnd->m_BegTime = GetVideoTime(wxStr.ToStdString());
				}

				if (m_pMainWnd->m_parser.Found("e", &wxStr))
				{
					m_pMainWnd->m_EndTime = GetVideoTime(wxStr.ToStdString());
				}

				double double_val;				

				if (m_pMainWnd->m_parser.Found("be", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						printf("ERROR: wrong \"be\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL2->m_pos = 1 - double_val;
				}

				if (m_pMainWnd->m_parser.Found("te", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						printf("ERROR: wrong \"te\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pHSL1->m_pos = 1 - double_val;
				}


				if (m_pMainWnd->m_parser.Found("le", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						printf("ERROR: wrong \"le\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL1->m_pos = double_val;
				}

				if (m_pMainWnd->m_parser.Found("re", &double_val))
				{
					if ((double_val < 0) || (double_val > 1.0))
					{
						printf("ERROR: wrong \"re\" command line option value\n");
						return false;
					}
					m_pMainWnd->m_pVideoBox->m_pVBox->m_pVSL2->m_pos = double_val;
				}

				m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread = new ThreadSearchSubtitles(m_pMainWnd, wxTHREAD_JOINABLE);
				m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Create();
				m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Run();				
				m_pMainWnd->m_pPanel->m_pSHPanel->m_pSearchThread->Wait();
			}
		}
		else
		{
			printf("ERROR: input video file was not provided\n");
			return false;
		}

		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("ccti"))
	{		
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CCTI);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateClearedTextImages(bn_event);
		m_pMainWnd->m_pPanel->m_pOCRPanel->m_pSearchThread->Wait();
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("ces"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CES);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateEmptySub(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("cscti"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSCTI);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromClearedTXTImages(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("cstxt"))
	{
		wxCommandEvent bn_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_BTN_CSTXT);
		m_pMainWnd->m_blnNoGUI = true;
		m_pMainWnd->m_pPanel->m_pOCRPanel->OnBnClickedCreateSubFromTXTResults(bn_event);
		blnNeedToExit = true;
	}

	if (wxCMD_SWITCH_ON == m_pMainWnd->m_parser.FoundSwitch("h"))
	{		
		cout << m_pMainWnd->m_parser.GetUsageString();
		m_pMainWnd->OnAppAbout(wxCommandEvent());
		//m_pMainWnd->m_parser.Usage();
		blnNeedToExit = true;
	}

	if (blnNeedToExit) return false;

	m_pMainWnd->Show(true);

	return true;
}

CVideoSubFinderApp::~CVideoSubFinderApp()
{
}

