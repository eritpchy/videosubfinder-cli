                              //MainFrm.h//                                
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

#pragma once

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/mdi.h>
#include <wx/timer.h>
#include <wx/filename.h>
#include <wx/cmdline.h>
#include "DataTypes.h"
#include "MyResource.h"
#include "SSOWnd.h"
#include "VideoBox.h"
#include "ImageBox.h"
#include "OCVVideoLoader.h"
#include "FFMPEGVideoLoader.h"
#include "SSAlgorithms.h"
#include "IPAlgorithms.h"
#include <time.h>

using namespace std;

class Settings;

extern bool g_playback_sound;
extern wxCmdLineParser *g_pParser;
extern Settings	g_cfg;
extern wxString g_GeneralSettingsFileName;

class CSSOWnd;
class CVideoBox;
class CImageBox;

void ReadSettings(wxString file_name, std::map<wxString, wxString>& settings);
void LoadSettings();
void SaveSettings();

class Settings
{
public:
	wxString	m_prefered_locale;

	int			m_txt_dw;
	int			m_txt_dy;

	int			m_fount_size_lbl;
	int			m_fount_size_btn;

	double		m_ocr_min_sub_duration;
	wxString	m_ocr_join_txt_images_split_line;

	int			process_affinity_mask = -1;

	wxString	m_pixel_color_bgr;
	wxString	m_pixel_color_lab;

	wxString	m_ocr_label_msd_text;
	wxString	m_ocr_label_join_txt_images_split_line_text;
	wxString	m_ocr_label_jsact_text;
	wxString	m_ocr_label_clear_txt_folders;
	wxString	m_ocr_label_save_each_substring_separately;
	wxString	m_ocr_label_save_scaled_images;
	wxString	m_ocr_button_ces_text;
	wxString	m_ocr_button_join_text;
	wxString	m_ocr_button_ccti_text;
	wxString	m_ocr_button_csftr_text;
	wxString	m_ocr_button_cesfcti_text;
	wxString	m_ocr_button_test_text;

	wxString	m_label_text_alignment;
	wxString	m_playback_sound;
	wxString	m_ssp_hw_device;
	wxString	m_label_filter_descr;
	wxString	m_ssp_label_parameters_influencing_image_processing;
	wxString	m_ssp_label_ocl_and_multiframe_image_stream_processing;
	wxString	m_ssp_oi_group_global_image_processing_settings;
	wxString	m_ssp_oi_property_use_ocl;
	wxString	m_ssp_oi_property_use_cuda_gpu;
	wxString	m_label_use_filter_color;
	wxString	m_label_use_outline_filter_color;
	wxString	m_label_dL_color;
	wxString	m_label_dA_color;
	wxString	m_label_dB_color;
	wxString	m_border_is_darker;
	wxString	m_extend_by_grey_color;
	wxString	m_allow_min_luminance;
	wxString	m_ssp_ocr_threads;
	wxString	m_ssp_oi_property_image_scale_for_clear_image;
	wxString	m_ssp_oi_property_moderate_threshold_for_scaled_image;
	wxString	m_ssp_oi_property_cuda_kmeans_initial_loop_iterations;
	wxString	m_ssp_oi_property_cuda_kmeans_loop_iterations;
	wxString	m_ssp_oi_property_cpu_kmeans_initial_loop_iterations;
	wxString	m_ssp_oi_property_cpu_kmeans_loop_iterations;
	wxString	m_ssp_oi_property_generate_cleared_text_images_on_test;
	wxString	m_ssp_oi_property_dump_debug_images;
	wxString	m_ssp_oi_property_dump_debug_second_filtration_images;
	wxString	m_ssp_oi_property_clear_test_images_folder;
	wxString	m_ssp_oi_property_show_transformed_images_only;
	wxString	m_ssp_oi_group_initial_image_processing;
	wxString	m_ssp_oi_sub_group_settings_for_sobel_operators;
	wxString	m_ssp_oi_property_moderate_threshold;
	wxString	m_ssp_oi_property_moderate_nedges_threshold;
	wxString	m_ssp_oi_sub_group_settings_for_color_filtering;
	wxString	m_ssp_oi_property_segment_width;
	wxString	m_ssp_oi_property_min_segments_count;
	wxString	m_ssp_oi_property_min_sum_color_difference;
	wxString	m_ssp_oi_group_secondary_image_processing;
	wxString	m_ssp_oi_sub_group_settings_for_linear_filtering;
	wxString	m_ssp_oi_property_line_height;
	wxString	m_ssp_oi_property_max_between_text_distance;
	wxString	m_ssp_oi_property_max_text_center_offset;
	wxString	m_ssp_oi_property_max_text_center_percent_offset;
	wxString	m_ssp_oi_sub_group_settings_for_color_border_points;
	wxString	m_ssp_oi_property_min_points_number;
	wxString	m_ssp_oi_property_min_points_density;
	wxString	m_ssp_oi_property_min_symbol_height;
	wxString	m_ssp_oi_property_min_symbol_density;
	wxString	m_ssp_oi_property_min_vedges_points_density;
	wxString	m_ssp_oi_property_min_nedges_points_density;
	wxString	m_ssp_oi_group_tertiary_image_processing;	
	wxString	m_ssp_oim_group_ocr_settings;
	wxString	m_ssp_oim_property_clear_images_logical;
	wxString	m_label_combine_to_single_cluster;
	wxString	m_ssp_oim_property_clear_rgbimages_after_search_subtitles;
	wxString	m_ssp_oim_property_using_hard_algorithm_for_text_mining;
	wxString	m_ssp_oim_property_using_isaimages_for_getting_txt_areas;
	wxString	m_ssp_oim_property_using_ilaimages_for_getting_txt_areas;
	wxString	m_label_ILA_images_for_getting_txt_symbols_areas;
	wxString	m_label_use_ILA_images_before_clear_txt_images_from_borders;
	wxString	m_ssp_oim_property_validate_and_compare_cleared_txt_images;
	wxString	m_ssp_oim_property_dont_delete_unrecognized_images_first;
	wxString	m_ssp_oim_property_dont_delete_unrecognized_images_second;
	wxString	m_ssp_oim_property_default_string_for_empty_sub;
	wxString	m_ssp_oim_group_settings_for_multiframe_image_processing;
	wxString	m_ssp_oim_sub_group_settings_for_sub_detection;
	wxString	m_ssp_oim_property_threads;
	wxString	m_ssp_oim_property_sub_frames_length;
	//wxString	m_ssp_oim_property_sub_square_error;
	wxString	m_ssp_oim_sub_group_settings_for_comparing_subs;
	wxString	m_ssp_oim_sub_group_settings_for_update_video_color;
	wxString	m_ssp_oim_property_vedges_points_line_error;
	wxString	m_ssp_oim_property_ila_points_line_error;
	wxString	m_ssp_oim_sub_group_settings_for_checking_sub;
	wxString	m_ssp_oim_property_text_percent;
	wxString	m_ssp_oim_property_min_text_length;
	wxString	m_ssp_oim_property_use_ISA_images_for_search_subtitles;
	wxString	m_ssp_oim_property_use_ILA_images_for_search_subtitles;
	wxString	m_ssp_oim_property_replace_ISA_by_filtered_version;
	wxString	m_ssp_oim_property_max_dl_down;
	wxString	m_ssp_oim_property_max_dl_up;
	wxString	m_ssp_oim_property_use_gradient_images_for_clear_txt_images;
	wxString	m_ssp_oim_property_clear_txt_images_by_main_color;
	wxString	m_ssp_oim_property_remove_wide_symbols;
	wxString	m_ssp_oim_property_use_ILA_images_for_clear_txt_images;	
	wxString	m_label_settings_file;
	wxString	m_label_pixel_color;
	wxString	m_label_video_contrast;
	wxString	m_label_video_gamma;
	double		m_bottom_video_image_percent_end = 0;
	double		m_top_video_image_percent_end = 1;
	double		m_left_video_image_percent_end = 0;
	double		m_right_video_image_percent_end = 1;
	wxString	m_video_box_title;
	wxString	m_image_box_title;
	wxString	m_search_panel_title;
	wxString	m_settings_panel_title;
	wxString	m_ocr_panel_title;
	wxString	m_label_begin_time;
	wxString	m_label_end_time;

	wxString	m_file_dialog_title_open_video_file;
	wxString	m_file_dialog_title_open_video_file_wild_card;
	wxString	m_file_dialog_title_open_settings_file;
	wxString	m_file_dialog_title_open_settings_file_wild_card;
	wxString	m_file_dialog_title_save_settings_file;
	wxString	m_file_dialog_title_save_settings_file_wild_card;
	wxString	m_file_dialog_title_save_subtitle_as;
	wxString	m_file_dialog_title_save_subtitle_as_wild_card;

	wxString	m_menu_file;
	wxString	m_menu_edit;
	wxString	m_menu_view;
	wxString	m_menu_play;
	wxString	m_menu_help;
	wxString	m_menu_setpriority;
	wxString	m_menu_setpriority_high;
	wxString	m_menu_setpriority_abovenormal;
	wxString	m_menu_setpriority_normal;
	wxString	m_menu_setpriority_belownormal;
	wxString	m_menu_setpriority_idle;
	wxString	m_menu_file_open_video_opencv;
	wxString	m_menu_file_open_video_ffmpeg;
	wxString	m_menu_file_reopenvideo;
	wxString	m_menu_file_openpreviousvideo;
	wxString	m_menu_file_savesettings;
	wxString	m_menu_file_savesettingsas;
	wxString	m_menu_file_loadsettings;
	wxString	m_menu_file_exit;
	wxString	m_menu_edit_setbegintime;
	wxString	m_menu_edit_setendtime;
	wxString	m_menu_scale_text_size_inc;
	wxString	m_menu_scale_text_size_dec;
	wxString	m_menu_play_pause;
	wxString	m_menu_play_stop;
	wxString	m_menu_app_about;

	wxString	m_help_desc_app_about;
	wxString	m_help_desc_for_clear_dirs;
	wxString	m_help_desc_for_run_search;
	wxString	m_help_desc_for_create_cleared_text_images;
	wxString	m_help_desc_for_create_empty_sub;
	wxString	m_help_desc_for_create_sub_from_cleared_txt_images;
	wxString	m_help_desc_for_create_sub_from_txt_results;
	wxString	m_help_desc_for_input_video;
	wxString	m_help_desc_for_open_video_opencv;
	wxString	m_help_desc_for_open_video_ffmpeg;
	wxString	m_help_desc_for_use_cuda;
	wxString	m_help_desc_for_start_time;
	wxString	m_help_desc_for_end_time;
	wxString	m_help_desc_for_top_video_image_percent_end;
	wxString	m_help_desc_for_bottom_video_image_percent_end;
	wxString	m_help_desc_for_left_video_image_percent_end;
	wxString	m_help_desc_for_right_video_image_percent_end;
	wxString	m_help_desc_for_output_dir;
	wxString	m_help_desc_for_general_settings;
	wxString	m_help_desc_for_num_threads;
	wxString	m_help_desc_for_num_ocr_threads;
	wxString	m_help_desc_for_help;

	wxString	m_button_clear_folders_text;
	wxString	m_button_run_search_text;
	wxString	m_button_test_text;
	wxString	m_test_result_after_first_filtration_label;
	wxString	m_test_result_after_second_filtration_label;
	wxString	m_test_result_after_third_filtration_label;
	wxString	m_test_result_nedges_points_image_label;
	wxString	m_test_result_cleared_text_image_label;
	wxString	m_grid_col_property_label;
	wxString	m_grid_col_value_label;

	wxColour	m_main_text_colour = wxColour(0, 0, 0);
	wxColour	m_main_text_ctls_background_colour = wxColour(255, 255, 255);
	wxColour	m_main_buttons_background_colour = wxColour(225, 225, 225);
	wxColour	m_main_labels_background_colour = wxColour(127, 255, 0);
	wxColour	m_main_frame_background_colour = wxColour(171, 171, 171);
	wxColour	m_notebook_colour = wxColour(240, 240, 240);
	wxColour	m_notebook_panels_colour = wxColour(170, 170, 170);	
	wxColour	m_grid_line_colour = wxColour(0, 0, 0);
	wxColour	m_grid_gropes_colour = wxColour(0, 255, 255);
	wxColour	m_grid_sub_gropes_colour = wxColour(255, 215, 0);
	wxColour	m_grid_debug_settings_colour = wxColour(200, 200, 200);
	wxColour	m_test_result_label_colour = wxColour(127, 255, 212);
	wxColour	m_video_image_box_background_colour = wxColour(125, 125, 125);
	wxColour	m_video_image_box_border_colour = wxColour(215, 228, 242);
	wxColour	m_video_image_box_title_colour = wxColour(255, 255, 225);
	wxColour	m_video_box_time_colour = wxColour(0, 0, 0);
	wxColour	m_video_box_time_text_colour = wxColour(255, 255, 255);
	wxColour	m_video_box_separating_line_colour = wxColour(255, 255, 255);
	wxColour	m_video_box_separating_line_border_colour = wxColour(0, 0, 0);	
	wxColour	m_toolbar_bitmaps_border_colour = wxColour(192, 192, 192);
};

class CMainFrame : public wxFrame
{
public:
	CMainFrame(const wxString& title);
	~CMainFrame();

public:	
	bool		m_WasInited;
	
	bool		m_blnNoGUI;

	CSSOWnd		*m_pPanel;
	CVideoBox	*m_pVideoBox;
	CImageBox	*m_pImageBox;

	CVideo		*m_pVideo;

	bool	    m_VIsOpen;
	//CDocManager m_DocManager;
	wxString	m_FileName;

	wxString		m_EndTimeStr;

	wxTimer		m_timer;
	
	enum {Play, Pause, Stop}	m_vs;
	std::mutex					m_play_mutex;

	int			m_BufferSize;
	int			m_w; //video width
	int			m_h; //video height

	int			m_cw; //client width
	int			m_ch; //client height

	bool	    m_bUpdateSizes = false;
	int 		m_dx = 20;
	int 		m_dy = 20;
	int			m_ph = 288; //panel height

	s64			m_dt;
	s64         m_ct;

	s64			m_BegTime;
	s64			m_EndTime;

	wxString		m_ErrorFileName;	

	bool		m_blnReopenVideo;

	int			m_type;

	bool		m_blnOpenVideoThreadStateFlag;
	bool		m_blnOpenVideoResult;	

	wxFont    m_BTNFont;
	wxFont    m_LBLFont;	

	std::mutex m_mutex;

public:
	void Init();

	void PauseVideo();
	void SaveError(wxString error);
	void OnFileOpenVideo(int type);
	void ClearDir(wxString DirName);

public:
	void OnViewImageInImageBox(wxThreadEvent& event);
	void OnViewImageInVideoBox(wxThreadEvent& event);
	void OnViewGreyscaleImageInImageBox(wxThreadEvent& event);
	void OnViewGreyscaleImageInVideoBox(wxThreadEvent& event);
	void OnViewBGRImageInImageBox(wxThreadEvent& event);
	void OnViewBGRImageInVideoBox(wxThreadEvent& event);
	void OnViewRGBImage(wxThreadEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnPlayPause(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnFileReOpenVideo(wxCommandEvent& event);
	void OnFileOpenVideoOpenCV(wxCommandEvent& event);
	void OnFileOpenVideoFFMPEG(wxCommandEvent& event);
	void OnEditSetBeginTime(wxCommandEvent& event);
	void OnEditSetEndTime(wxCommandEvent& event);
	void OnFileSaveSettings(wxCommandEvent& event);
	void OnFileLoadSettings(wxCommandEvent& event);
	void OnFileSaveSettingsAs(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnFileOpenPreviousVideo(wxCommandEvent& event);
	void OnAppAbout(wxCommandEvent& event);
	void OnScaleTextSizeInc(wxCommandEvent& event);
	void OnScaleTextSizeDec(wxCommandEvent& event);
	void ScaleTextSize(int dsize);
	void OnSetPriorityIdle(wxCommandEvent& event);
	void OnSetPriorityNormal(wxCommandEvent& event);
	void OnSetPriorityBelownormal(wxCommandEvent& event);
	void OnSetPriorityAbovenormal(wxCommandEvent& event);
	void OnSetPriorityHigh(wxCommandEvent& event);
	wxString ConvertTime(u64 total_milliseconds);	
	void ShowErrorMessage(wxString msg);
	int GetOptimalFontSize(int cw, int ch, wxString label, wxFontFamily family, wxFontStyle style, wxFontWeight weight, bool underlined = false, const wxString& face = wxEmptyString, wxFontEncoding encoding = wxFONTENCODING_DEFAULT);

private:
   DECLARE_EVENT_TABLE()
};

class MyMessageBox : public wxDialog
{
public:
	CMainFrame* m_pMF;
	wxTextCtrl* m_pDialogText;

	MyMessageBox(CMainFrame* pMF, const wxString& message, const wxString& caption,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize) : wxDialog(pMF, -1, caption, pos, size)
	{
		m_pMF = pMF;
		m_pDialogText = new wxTextCtrl(this, -1, message, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxBORDER_NONE);
		m_pDialogText->SetFont(m_pMF->m_LBLFont);
		m_pDialogText->SetForegroundColour(g_cfg.m_main_text_colour);
		m_pDialogText->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);
	}

	~MyMessageBox()
	{
	}	
};

wxString ConvertVideoTime(s64 pos);
wxString VideoTimeToStr2(s64 pos);
wxString VideoTimeToStr3(s64 pos);

void WriteProperty(wxTextOutputStream& fout, int val, wxString Name);
void WriteProperty(wxTextOutputStream& fout, bool val, wxString Name);
void WriteProperty(wxTextOutputStream& fout, double val, wxString Name);
void WriteProperty(wxTextOutputStream& fout, wxString val, wxString Name);
void WriteProperty(wxTextOutputStream& fout, wxArrayString val, wxString Name);
void WriteProperty(wxTextOutputStream& fout, wxColour val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, int& val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, bool& val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, double& val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, wxString& val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, wxArrayString& val, wxString Name);
bool ReadProperty(std::map<wxString, wxString>& settings, wxColour& val, wxString Name);

bool IsMMX_and_SSE();
bool IsSSE2();

void LoadToolBarImage(wxBitmap& bmp, const wxString& path, const wxColour& BColor);

extern CMainFrame *g_pMF;
