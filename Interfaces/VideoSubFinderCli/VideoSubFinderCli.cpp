#include <wx/regex.h>

#include "VideoSubFinderCli.h"
#include "IPAlgorithms.h"


static const wxCmdLineEntryDesc cmdLineDesc[] =
{
	{ wxCMD_LINE_SWITCH, "c", "clear_dirs", "Clear Folders (remove all images), performed before any other steps" },
	{ wxCMD_LINE_SWITCH, "r", "run_search", "Run Search (find frames with hardcoded text (hardsub) on video)" },
    { wxCMD_LINE_OPTION, "ces", "create_empty_sub", "Create Empty Sub With Provided Output File Name (*.srt)" },
    { wxCMD_LINE_OPTION, "i", "input_video", "input video file" },
	{ wxCMD_LINE_SWITCH, "ovocv", "open_video_opencv", "open video by OpenCV (default)" },
	{ wxCMD_LINE_SWITCH, "ovffmpeg", "open_video_ffmpeg", "open video by FFMPEG" },
	{ wxCMD_LINE_SWITCH, "uc", "use_cuda", "use cuda" },
	{ wxCMD_LINE_SWITCH, "dsi", "dont_save_images", "Don't save images" },
	{ wxCMD_LINE_OPTION, "s", "start_time", "start time, default = 0:00:00:000 (in format hour:min:sec:milisec)" },
	{ wxCMD_LINE_OPTION, "e", "end_time", "end time, default = video length" },
	{ wxCMD_LINE_OPTION, "te", "top_video_image_percent_end", "top video image percent offset from image bottom, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "be", "bottom_video_image_percent_end", "bottom video image percent offset from image bottom, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "le", "left_video_image_percent_end", "left video image percent end, can be in range [0.0,1.0], default = 0.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "re", "right_video_image_percent_end", "right video image percent end, can be in range [0.0,1.0], default = 1.0", wxCMD_LINE_VAL_DOUBLE },
	{ wxCMD_LINE_OPTION, "o", "output_dir",  "output dir (root directory where results will be stored)" },
    { wxCMD_LINE_OPTION, "gs", "general_settings",  "general settings (path to general settings *.cfg file, default = settings/general.cfg)" },
    { wxCMD_LINE_OPTION, "nthr", "num_threads", "number of threads used for Run Search", wxCMD_LINE_VAL_NUMBER },
	{ wxCMD_LINE_SWITCH, "h", "help", "show this help message\n\nExample of usage:\nVideoSubFinderWXW.exe -c -r -ccti -i \"C:\\test_video.mp4\" -cscti \"C:\\test_video.srt\" -o \"C:\\ResultsDir\" -te 0.5 -be 0.1 -le 0.1 -re 0.9 -s 0:00:10:300 -e 0:00:13:100\n" },
	{ wxCMD_LINE_NONE }
};

wxString g_ReportFileName;
wxString g_InputFileName;
wxString g_sub_path;
CVideo* g_pVideo;
double g_DxMin;
double g_DxMax;
double g_DyMin;
double g_DyMax;
s64 g_BegTime;
s64	g_EndTime;

extern int g_IsCreateClearedTextImages;
extern int g_RunCreateClearedTextImages;
extern bool g_ValidateAndCompareTXTImages;
wxString g_DefStringForEmptySub = "sub duration: %sub_duration%";

bool g_join_subs_and_correct_time = true;

void ClearDir(wxString DirName)
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

void CVideoSubFinderApp::OnInitCmdLine(wxCmdLineParser& parser) {
    wxAppConsole::OnInitCmdLine(parser);

    parser.SetDesc(cmdLineDesc);
}

bool CVideoSubFinderApp::OnCmdLineParsed(wxCmdLineParser& parser) {
    wxAppConsole::OnCmdLineParsed(parser);
    wxString Str = wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath();
    Str.Replace("\\", "/");
    g_app_dir = Str;
    g_work_dir = g_app_dir;

    g_ReportFileName = g_app_dir + wxT("/report.log");

    SaveToReportLog("Starting program...\n", wxT("wb"));
    SaveToReportLog("CVideoSubFinderApp::OnInitCmdLine...\n");

    wxString wxStr;
    if (parser.Found("gs", &wxStr))
    {
        wxStr.Replace("\\", "/");
        LoadSettings(wxStr);
    } else {
        LoadSettings(g_app_dir + wxT("/settings/general.cfg"));
    }

    if (parser.FoundSwitch("dsi")) {
        g_save_images = false;
    } else {
        g_save_images = true;
    }
    long threads;
    if (parser.Found("nthr", &threads))
    {
        g_threads = threads;
    }

    if (parser.FoundSwitch("uc"))
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
    parser.Found("i", &g_InputFileName);
    if (parser.Found("o", &wxStr))
    {
        wxStr.Replace("\\", "/");
        g_work_dir = wxStr;
    }
    if (wxCMD_SWITCH_ON == parser.FoundSwitch("c"))
    {
        ClearDir(g_work_dir + "/RGBImages");
        ClearDir(g_work_dir + "/ISAImages");
        ClearDir(g_work_dir + "/ILAImages");
        ClearDir(g_work_dir + "/TXTImages");
        ClearDir(g_work_dir + "/TXTImagesJoined");
        ClearDir(g_work_dir + "/TestImages");
        ClearDir(g_work_dir + "/TXTResults");
    }

    wxFileName::Mkdir(g_work_dir + "/RGBImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/ISAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/ILAImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/TestImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/TXTImages", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/TXTImagesJoined", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxFileName::Mkdir(g_work_dir + "/TXTResults", wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    if (parser.FoundSwitch("ovffmpeg")) {
        g_pVideo = GetFFMPEGVideoObject();
    } else {
        g_pVideo = GetOCVVideoObject();
    }
    g_pVideo->m_Dir = g_work_dir;
    if (!g_pVideo->OpenMovie(g_InputFileName, NULL, 0)) {
        wxLogMessage("ERROR: input video file was not provided\n");
        return false;
    }
    g_BegTime = 0;
    g_EndTime = g_pVideo->m_Duration;

    if (parser.Found("s", &wxStr))
    {
        g_BegTime = GetVideoTime(wxStr);
    }

    if (parser.Found("e", &wxStr))
    {
        g_EndTime = GetVideoTime(wxStr);
    }
    if (!g_pVideo->SetNullRender()) {
        wxLogMessage("ERROR: g_pVideo->SetNullRender() failed\n");
        return false;
    }
    g_DxMin = 0;
    g_DxMax = 1;
    g_DyMin = 1 - 1;
    g_DyMax = 1 - 0;
    if (parser.Found("le", &g_DxMin))
    {
        if ((g_DxMin < 0) || (g_DxMin > 1.0))
        {
            wxLogMessage("ERROR: wrong \"le\" command line option value\n");
            return false;
        }
    }
    if (parser.Found("re", &g_DxMax))
    {
        if ((g_DxMax < 0) || (g_DxMax > 1.0))
        {
            wxLogMessage("ERROR: wrong \"re\" command line option value\n");
            return false;
        }
    }
    if (parser.Found("te", &g_DyMin))
    {
        if ((g_DyMin < 0) || (g_DyMin > 1.0))
        {
            wxLogMessage("ERROR: wrong \"te\" command line option value\n");
            return false;
        }
        g_DyMin = 1 - g_DyMin;
    }
    if (parser.Found("be", &g_DyMax))
    {
        if ((g_DyMax < 0) || (g_DyMax > 1.0))
        {
            wxLogMessage("ERROR: wrong \"be\" command line option value\n");
            return false;
        }
        g_DyMax = 1 - g_DyMax;
    }

    if (parser.Found("ces", &wxStr))
    {
        g_sub_path = wxStr;
    }

    g_color_ranges = GetColorRanges(g_use_filter_color);
    g_outline_color_ranges = GetColorRanges(g_use_outline_filter_color);
	return true;
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

void createEmptySub()
{
	wxString Str, SubStr, hour1, hour2, min1, min2, sec1, sec2, msec1, msec2;
	int i, j, k, sec, msec;
	u64 bt, et, dt, mdt;
	wxString str_int;

	vector<wxString> FileNamesVector = g_file_names_vector;
	vector<u64> BT, ET;
	wxString filename;
	for (i=0; i<(int)FileNamesVector.size()-1; i++)
	for (j=i+1; j<(int)FileNamesVector.size(); j++)
	{
		if (FileNamesVector[i] > FileNamesVector[j])
		{
			Str = FileNamesVector[i];
			FileNamesVector[i] = FileNamesVector[j];
			FileNamesVector[j] = Str;
		}
	}

	mdt = 0; //(s64)(m_pMF->m_cfg.m_ocr_min_sub_duration * (double)1000);

	for(k=0; k<(int)FileNamesVector.size(); k++)
	{
		Str = FileNamesVector[k];

		hour1 = Str.Mid(0,1);
		min1 = Str.Mid(2,2);
		sec1 = Str.Mid(5,2);
		msec1 = Str.Mid(8,3);

		hour2 = Str.Mid(13,1);
		min2 = Str.Mid(15,2);
		sec2 = Str.Mid(18,2);
		msec2 = Str.Mid(21,3);

		bt = (wxAtoi(hour1)*3600 + wxAtoi(min1)*60 + wxAtoi(sec1))*1000 + wxAtoi(msec1);
		et = (wxAtoi(hour2)*3600 + wxAtoi(min2)*60 + wxAtoi(sec2))*1000 + wxAtoi(msec2);

		BT.push_back(bt);
		ET.push_back(et);
	}

	if (g_join_subs_and_correct_time)
	{
		for (k = 0; k < (int)FileNamesVector.size() - 1; k++)
		{
			if (ET[k] - BT[k] < mdt)
			{
				if (BT[k] + mdt < BT[k + 1])
				{
					ET[k] = BT[k] + mdt;
				}
				else
				{
					ET[k] = BT[k + 1] - 1;
				}
			}
		}
	}

	wxString srt_sub;
	for(k=0; k<(int)FileNamesVector.size(); k++)
	{
		bt = BT[k];
		et = ET[k];

		Str = VideoTimeToStr2(bt)+
			  " --> "+
			  VideoTimeToStr2(et);

		dt = et - bt;
		sec = (int)(dt/1000);
		msec = (int)(dt%1000);

		sec1 = wxString::Format(wxT("%i"), sec);

		str_int = wxString::Format(wxT("%i"), msec);
		if (msec < 10) msec1 = wxT("00") + str_int;
		else
		{
			if (msec < 100) msec1 = wxT("0")+ str_int;
			else msec1 = str_int;
		}

        //"def_string_for_empty_sub"
        wxString g_DefStringForEmptySub = "sub duration: %sub_duration%";
		SubStr = g_DefStringForEmptySub;

		if (g_DefStringForEmptySub.Contains("%sub_duration%"))
		{
			SubStr.Replace("%sub_duration%", sec1 + "," + msec1);
		}

		srt_sub << (k+1) << wxT("\n") << Str << wxT("\n") << SubStr << "\n\n";
	}
	wxFFileOutputStream ffout(g_sub_path);
    wxTextOutputStream fout(ffout);
    fout << srt_sub;
    fout.Flush();
    ffout.Close();
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

static void ReadSettings(wxString file_name, std::map<wxString, wxString>& settings)
{
	wxFileInputStream ffin(file_name);

	if (!ffin.IsOk())
	{
		wxLogMessage("ERROR: Can't open settings file: " + file_name);
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
					wxLogMessage(wxT("Unsupported line format: ") + line + wxT("\nin file: ") + file_name);
					exit(1);
				}
			}
		}
	}
}

void CVideoSubFinderApp::LoadSettings(wxString file_name)
{
	std::map<wxString, wxString> g_general_settings;
	ReadSettings(file_name, g_general_settings);

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

	ReadProperty(g_general_settings, g_join_subs_and_correct_time, "join_subs_and_correct_time");

	ReadProperty(g_general_settings, g_threads, "threads");
	ReadProperty(g_general_settings, g_DL, "sub_frame_length");
	ReadProperty(g_general_settings, g_tp, "text_percent");
	ReadProperty(g_general_settings, g_mtpl, "min_text_len_in_percent");
	ReadProperty(g_general_settings, g_veple, "vedges_points_line_error");
	ReadProperty(g_general_settings, g_ilaple, "ila_points_line_error");

	ReadProperty(g_general_settings, g_video_contrast, "video_contrast");
	ReadProperty(g_general_settings, g_video_gamma, "video_gamma");

	ReadProperty(g_general_settings, g_clear_image_logical, "clear_image_logical");

	ReadProperty(g_general_settings, g_DefStringForEmptySub, "def_string_for_empty_sub");

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

	ReadProperty(g_general_settings, g_border_is_darker, "border_is_darker");

	int text_alignment;
	ReadProperty(g_general_settings, text_alignment, "text_alignment");
	g_text_alignment = (TextAlignment)text_alignment;

	ReadProperty(g_general_settings, g_extend_by_grey_color, "extend_by_grey_color");
	ReadProperty(g_general_settings, g_allow_min_luminance, "allow_min_luminance");
}


static void ViewImageInNull(simple_buffer<int>& Im, int w, int h) {
}
static void ViewBGRImageInNull(simple_buffer<u8>& ImBGR, int w, int h) {
}
static void ViewGreyscaleImageInNull(simple_buffer<u8>& Im, int w, int h) {
}
int CVideoSubFinderApp::OnRun()
{
	g_pViewImage[0] = ViewImageInNull;
	g_pViewImage[1] = ViewImageInNull;
	g_pViewBGRImage[0] = ViewBGRImageInNull;
	g_pViewBGRImage[1] = ViewBGRImageInNull;
	g_pViewGreyscaleImage[0] = ViewGreyscaleImageInNull;
	g_pViewGreyscaleImage[1] = ViewGreyscaleImageInNull;
	g_pViewRGBImage = ViewImageInNull;

    g_pVideo->SetVideoWindowSettins(g_DxMin, g_DxMax, g_DyMin, g_DyMax);
    g_RunSubSearch = 1;
    FastSearchSubtitles(g_pVideo, g_BegTime, g_EndTime);
    if (g_sub_path.size() > 0) {
        createEmptySub();
    }
    wxLogMessage("FINISH");
    return false;
}

CVideoSubFinderApp::~CVideoSubFinderApp()
{
}