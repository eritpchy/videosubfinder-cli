                              //SettingsPanel.cpp//                                
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

#include "MyResource.h"
#include "SettingsPanel.h"
#include "OCRPanel.h"

vector<wxString> StrFN;

BEGIN_EVENT_TABLE(CSettingsPanel, wxPanel)
	EVT_BUTTON(ID_TEST, CSettingsPanel::OnBnClickedTest)
	EVT_BUTTON(ID_SP_LEFT, CSettingsPanel::OnBnClickedLeft)
	EVT_BUTTON(ID_SP_RIGHT, CSettingsPanel::OnBnClickedRight)
	//ON_WM_SETCURSOR()
	//ON_WM_CREATE()
	//ON_WM_PAINT()
END_EVENT_TABLE()

CSettingsPanel::CSettingsPanel(CSSOWnd* pParent)
		:wxPanel( pParent, wxID_ANY )
{
	m_pParent = pParent;
	m_pMF = pParent->m_pMF;

	m_n = 5;
	StrFN.resize(m_n);
	StrFN[0] = wxT("After First Filtration");
	StrFN[1] = wxT("After Second Filtration");
	StrFN[2] = wxT("After Third Filtration");
	StrFN[3] = wxT("NEdges Points Image");
	StrFN[4] = wxT("Cleared Text Image");
	m_cn = 4;
}

CSettingsPanel::~CSettingsPanel()
{
}

void CSettingsPanel::Init()
{
	SaveToReportLog("CSettingsPanel::Init(): starting...\n");

	int bbw, bbh, w, h, dw, dh, cw, ch;
	wxRect rcP2, rcClP2, rcGB1, rcGB2, rcGB3; 
	wxRect rcTEST, rcLeft, rcRight, rlIF;
	wxRect rcOI, rcOIM;
	wxImage bmp_na, bmp_od;
	const int GBH = 238;
	const int GB3W = 300;
	const int GB12OPTW = 580;
	const int TESTW = 170;
	const int dx = 5;
	int GB12W;

	m_CLDBG = wxColour(200, 200, 200);
	m_CLSP = wxColour(170,170,170);
	m_CL1 = wxColour(255, 215, 0);
	m_CL2 = wxColour(127, 255, 0);
	m_CL3 = wxColour(127, 255, 212);
	m_CL4 = wxColour(255, 255, 0);
	m_CLGG = wxColour(0, 255, 255);

	rcP2 = this->GetRect();

	this->GetClientSize(&w, &h);
	rcClP2.x = rcClP2.y = 0; 
	rcClP2.width = w;
	rcClP2.height = h;

	dw = rcP2.width - rcClP2.width;
	dh = rcP2.height - rcClP2.height;

	m_pParent->GetClientSize(&cw, &ch);

	GB12W = std::min<int>(GB12OPTW, (cw - GB3W - (dx * 6) - dw) / 2);

	rcGB1.x = dx * 2;
	rcGB1.y = 2;
	rcGB1.width = GB12W;
	rcGB1.height = GBH;

	rcGB2.x = rcGB1.GetRight() + dx;
	rcGB2.y = rcGB1.y;
	rcGB2.width = GB12W;
	rcGB2.height = GBH;

	rcGB3.x = rcGB2.GetRight() + dx;
	rcGB3.y = rcGB2.y;
	rcGB3.width = GB3W;
	rcGB3.height = GBH;

	rcP2.x = 0;
	rcP2.y = 0;
	rcP2.width = rcGB3.GetRight() + (dx * 2) + dw;
	rcP2.height = (rcGB2.GetBottom() + 5) + dh;

	rcOI.x = rcGB1.x + 3;
	rcOI.y = rcGB1.y + 15;
	rcOI.width = rcGB1.width - 3*2;
	rcOI.height = rcGB1.height - 15 - 3;

	rcOIM.x = rcGB2.x + 3;
	rcOIM.y = rcGB2.y + 15;
	rcOIM.width = rcGB2.width - 3*2;
	rcOIM.height = rcGB2.height - 15 - 3;

	rcTEST.x = rcGB3.x + (GB3W - TESTW)/2;
	rcTEST.y = 50;
	rcTEST.width = TESTW;
	rcTEST.height = 30;

	bbw = 23;
	bbh = 22;

	rcLeft.x = rcTEST.x - 56;
	rcLeft.y = rcTEST.y - 30;
	rcLeft.width = bbw;
	rcLeft.height = bbh;

	rcRight.x = rcTEST.GetRight() + (rcTEST.x - rcLeft.GetRight());
	rcRight.y = rcLeft.y;
	rcRight.width = bbw;
	rcRight.height = bbh;

	rlIF.x = rcLeft.GetRight() + 2;
	rlIF.y = rcLeft.y;
	rlIF.width = rcRight.x - 2 - rlIF.x;
	rlIF.height = bbh;

	SaveToReportLog("CSettingsPanel::Init(): init m_pP2...\n");
	m_pP2 = new wxPanel( this, wxID_ANY, rcP2.GetPosition(), rcP2.GetSize() );
	m_pP2->SetMinSize(rcP2.GetSize());
	m_pP2->SetBackgroundColour( m_CLSP );

	SaveToReportLog("CSettingsPanel::Init(): init top_sizer...\n");
	wxBoxSizer *top_sizer = new wxBoxSizer( wxVERTICAL );

	SaveToReportLog("CSettingsPanel::Init(): init button_sizer...\n");
	wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

	button_sizer->Add(m_pP2, 1, wxALIGN_CENTER, 0 );

	top_sizer->Add(button_sizer, 1, wxALIGN_CENTER );
	
	this->SetSizer(top_sizer);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB1...\n");
	m_pGB1 = new CStaticBox( m_pP2, wxID_ANY,
		m_pMF->m_cfg.m_ssp_label_parameters_influencing_image_processing, rcGB1.GetPosition(), rcGB1.GetSize() );
	m_pGB1->SetFont(m_pMF->m_LBLFont);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB2...\n");
	m_pGB2 = new CStaticBox( m_pP2, wxID_ANY,
		m_pMF->m_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, rcGB2.GetPosition(), rcGB2.GetSize() );
	m_pGB2->SetFont(m_pMF->m_LBLFont);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB3...\n");
	m_pGB3 = new CStaticBox( m_pP2, wxID_ANY,
		wxT(""), rcGB3.GetPosition(), rcGB3.GetSize() );
	m_pGB3->SetFont(m_pMF->m_LBLFont);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pTest...\n");
	m_pTest = new CButton( m_pP2, ID_TEST,
		wxT("Test"), rcTEST.GetPosition(), rcTEST.GetSize() );
	m_pTest->SetFont(m_pMF->m_BTNFont);
	
	bmp_na = wxImage(g_app_dir + "/bitmaps/left_na.bmp");
	bmp_od = wxImage(g_app_dir + "/bitmaps/left_od.bmp");

	SaveToReportLog("CSettingsPanel::Init(): init m_pLeft...\n");
	m_pLeft = new CBitmapButton( m_pP2, ID_SP_LEFT,
		bmp_na, bmp_od, rcLeft.GetPosition(), rcLeft.GetSize() );

	bmp_na = wxImage(g_app_dir + "/bitmaps/right_na.bmp");
	bmp_od = wxImage(g_app_dir + "/bitmaps/right_od.bmp");

	SaveToReportLog("CSettingsPanel::Init(): init m_pRight...\n");
	m_pRight = new CBitmapButton( m_pP2, ID_SP_RIGHT,
		bmp_na, bmp_od, rcRight.GetPosition(), rcRight.GetSize() );

	SaveToReportLog("CSettingsPanel::Init(): init m_plblIF...\n");
	m_plblIF = new CStaticText( m_pP2, wxID_ANY, wxString(StrFN[m_cn]));
	m_plblIF->SetFont(m_pMF->m_LBLFont);
	m_plblIF->SetBackgroundColour( m_CL3 );
	m_plblIF->SetSize(rlIF);

	SaveToReportLog("CSettingsPanel::Init(): init m_pOI...\n");
	m_pOI = new CDataGrid( m_pP2, ID_OI, &(m_pMF->m_LBLFont),
                           rcOI.GetPosition(), rcOI.GetSize() );

	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_global_image_processing_settings...\n");
    m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_global_image_processing_settings, m_CLGG);	
	
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_text_alignment, m_CL2, m_CL4, &g_text_alignment_string, GetAvailableTextAlignments());

	m_pOI->AddProperty(m_pMF->m_cfg.m_label_use_filter_color, m_CL2, m_CL4, &g_use_filter_color);
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_use_outline_filter_color, m_CL2, m_CL4, &g_use_outline_filter_color);
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_dL_color, m_CL2, m_CL4, &g_dL_color, 0, 255);
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_dA_color, m_CL2, m_CL4, &g_dA_color, 0, 255);
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_dB_color, m_CL2, m_CL4, &g_dB_color, 0, 255);	
	
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_hw_device, m_CL2, m_CL4, &g_hw_device, GetAvailableHWDeviceTypes());
	
	m_pOI->AddProperty(m_pMF->m_cfg.m_label_filter_descr, m_CL2, m_CL4, &g_filter_descr);

#ifdef WIN64
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_cuda_gpu, m_CL2, m_CL4, &g_use_cuda_gpu);
#else
	//m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_cuda_gpu + " (only on x64 is supported)", m_CLSP, m_CLSP, &g_use_cuda_gpu);
	//m_pOI->SetReadOnly(m_pOI->GetNumberRows() - 1, 1, true);
#endif
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_ocl, m_CL2, m_CL4, &g_use_ocl);

	m_pOI->AddProperty(m_pMF->m_cfg.m_playback_sound, m_CL2, m_CL4, &g_playback_sound);

	// debug settings
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, m_CLDBG, m_CLDBG, &g_generate_cleared_text_images_on_test);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_dump_debug_images, m_CLDBG, m_CLDBG, &g_show_results);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, m_CLDBG, m_CLDBG, &g_show_sf_results);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_clear_test_images_folder, m_CLDBG, m_CLDBG, &g_clear_test_images_folder);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_show_transformed_images_only, m_CLDBG, m_CLDBG, &g_show_transformed_images_only);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_initial_image_processing...\n");
	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_initial_image_processing, m_CLGG);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, m_CL1);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_threshold, m_CL2, m_CL4, &g_mthr, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_nedges_threshold, m_CL2, m_CL4, &g_mnthr, 0.0, 1.0);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, m_CL1);	
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_segment_width, m_CL2, m_CL4, &g_segw, 4, 50);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_segments_count, m_CL2, m_CL4, &g_msegc, 1, 10);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_sum_color_difference, m_CL2, m_CL4, &g_scd, 0, 10000);

	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_secondary_image_processing, m_CLGG);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, m_CL1);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_line_height, m_CL2, m_CL4, &g_segh, 1, 50);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_max_between_text_distance, m_CL2, m_CL4, &g_btd, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_max_text_center_offset, m_CL2, m_CL4, &g_to, 0.0, 1.0);	
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, m_CL1);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_points_number, m_CL2, m_CL4, &g_mpn, 0, 10000);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_points_density, m_CL2, m_CL4, &g_mpd, 0.0, 1.0);	

	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_nedges_points_density, m_CL2, m_CL4, &g_mpned, 0.0, 1.0);

	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_tertiary_image_processing...\n");
	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_tertiary_image_processing, m_CLGG);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_symbol_height, m_CL2, m_CL4, &g_msh, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_symbol_density, m_CL2, m_CL4, &g_msd, 0.0, 1.0);

	m_pOI->SetColSize(0, m_pOI->GetClientSize().x*0.65);
	m_pOI->SetColSize(1, m_pOI->GetClientSize().x*0.35);

	////////////////////////////////////////////////////////////////////////

	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM...\n");
	m_pOIM = new CDataGrid( m_pP2, ID_OIM, &(m_pMF->m_LBLFont),
                           rcOIM.GetPosition(), rcOIM.GetSize() );

	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM m_ssp_oim_group_ocr_settings...\n");
	m_pOIM->AddGroup(m_pMF->m_cfg.m_ssp_oim_group_ocr_settings, m_CLGG);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_border_is_darker, m_CL2, m_CL4, &g_border_is_darker);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_extend_by_grey_color, m_CL2, m_CL4, &g_extend_by_grey_color);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_allow_min_luminance, m_CL2, m_CL4, &g_allow_min_luminance, 0, 255);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_ocr_threads, m_CL2, m_CL4, &g_ocr_threads, -1, 100);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_image_scale_for_clear_image, m_CL2, m_CL4, &g_scale, 1, 4);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, m_CL2, m_CL4, &g_smthr, 0.0, 1.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, m_CL2, m_CL4, &g_cpu_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, m_CL2, m_CL4, &g_cpu_kmeans_loop_iterations, 1, 1000);
#ifdef WIN64
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, m_CL2, m_CL4, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, m_CL2, m_CL4, &g_cuda_kmeans_loop_iterations, 1, 1000);
#else
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, m_CLSP, m_CLSP, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, m_CLSP, m_CLSP, &g_cuda_kmeans_loop_iterations, 1, 1000);
	m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
#endif
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, m_CL2, m_CL4, &g_CLEAN_RGB_IMAGES);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_images_logical, m_CL2, m_CL4, &g_clear_image_logical);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_label_combine_to_single_cluster, m_CL2, m_CL4, &g_combine_to_single_cluster);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, m_CL2, m_CL4, &g_use_ISA_images_for_get_txt_area);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, m_CL2, m_CL4, &g_use_ILA_images_for_get_txt_area);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, m_CL2, m_CL4, &g_use_gradient_images_for_clear_txt_images);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_label_ILA_images_for_getting_txt_symbols_areas, m_CL2, m_CL4, &g_use_ILA_images_for_getting_txt_symbols_areas);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_label_use_ILA_images_before_clear_txt_images_from_borders, m_CL2, m_CL4, &g_use_ILA_images_before_clear_txt_images_from_borders);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, m_CL2, m_CL4, &g_use_ILA_images_for_clear_txt_images);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, m_CL2, m_CL4, &g_clear_txt_images_by_main_color);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_remove_wide_symbols, m_CL2, m_CL4, &g_remove_wide_symbols);

	//m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, m_CLSP, m_CLSP, &g_ValidateAndCompareTXTImages);
	//m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);

	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, m_CL2, m_CL4, &g_DontDeleteUnrecognizedImages1);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, m_CL2, m_CL4, &g_DontDeleteUnrecognizedImages2);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_default_string_for_empty_sub, m_CL2, m_CL4, &g_DefStringForEmptySub);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM m_ssp_oim_group_settings_for_multiframe_image_processing...\n");
	m_pOIM->AddGroup(m_pMF->m_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, m_CLGG);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_threads, m_CL2, m_CL4, &g_threads, -1, 100);
	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, m_CL1);		
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_sub_frames_length, m_CL2, m_CL4, &g_DL, 1, 100);

	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, m_CL2, m_CL4, &g_use_ILA_images_for_search_subtitles);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, m_CL2, m_CL4, &g_use_ISA_images_for_search_subtitles);	
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, m_CL2, m_CL4, &g_replace_ISA_by_filtered_version);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_max_dl_down, m_CL2, m_CL4, &g_max_dl_down, 0, 255);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_max_dl_up, m_CL2, m_CL4, &g_max_dl_up, 0, 255);

	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, m_CL1);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_vedges_points_line_error, m_CL2, m_CL4, &g_veple, 0.0, 10.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_ila_points_line_error, m_CL2, m_CL4, &g_ilaple, 0.0, 10.0);
	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, m_CL1);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_text_percent, m_CL2, m_CL4, &g_tp, 0.0, 1.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_min_text_length, m_CL2, m_CL4, &g_mtpl, 0.0, 1.0);

	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_update_video_color, m_CL1);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_label_video_contrast, m_CL2, m_CL4, &g_video_contrast, 0.0, 10.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_label_video_gamma, m_CL2, m_CL4, &g_video_gamma, 0.0, 10.0);

	m_pOIM->SetColSize(0, m_pOIM->GetClientSize().x * 0.65);
	m_pOIM->SetColSize(1, m_pOIM->GetClientSize().x * 0.35);

	wxRect rlGSFN, rcGSFN, rlPixelColor, rcPixelColorRGB, rcPixelColorLab, rcPixelColorExample;
	rlGSFN.x = rcLeft.x;
	rlGSFN.y = rcTEST.GetBottom() + 8;
	rlGSFN.height = rlIF.height;
	rlGSFN.width = rcRight.GetRight() - rcLeft.GetLeft() + 1;	

	rcGSFN = rlGSFN;
	rcGSFN.y = rlGSFN.GetBottom() + 4;

	rlPixelColor = rlGSFN;
	rlPixelColor.y = rcGSFN.GetBottom() + 8;
	rlPixelColor.height *= 2;

	rcPixelColorLab = rlGSFN;
	rcPixelColorLab.width = ((2 * rlGSFN.width) / 3) - 1;
	rcPixelColorLab.y = rlPixelColor.GetBottom() + 4;

	rcPixelColorRGB = rcPixelColorLab;
	rcPixelColorRGB.y = rcPixelColorLab.GetBottom() + 4;
	
	rcPixelColorExample.x = rcPixelColorLab.GetRight() + 2;
	rcPixelColorExample.y = rcPixelColorLab.y;
	rcPixelColorExample.width = (rlGSFN.width / 3) - 1;
	rcPixelColorExample.height = rcPixelColorRGB.GetBottom() - rcPixelColorLab.y + 1;	

	SaveToReportLog("CSettingsPanel::Init(): init m_plblGSFN...\n");
	m_plblGSFN = new CStaticText(m_pP2, wxID_ANY, m_pMF->m_cfg.m_label_settings_file);
	m_plblGSFN->SetFont(m_pMF->m_LBLFont);
	m_plblGSFN->SetBackgroundColour(m_CL1);
	m_plblGSFN->SetSize(rlGSFN);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGSFN...\n");
	m_pGSFN = new CStaticText(m_pP2, wxID_ANY, m_pMF->m_GeneralSettingsFileName + wxT(" "), wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);	
	m_pGSFN->SetFont(m_pMF->m_LBLFont);
	m_pGSFN->SetBackgroundColour(wxColour(255, 255, 255));
	m_pGSFN->SetSize(rcGSFN);

	SaveToReportLog("CSettingsPanel::Init(): init m_plblPixelColor...\n");
	m_plblPixelColor = new CStaticText(m_pP2, wxID_ANY, m_pMF->m_cfg.m_label_pixel_color);
	m_plblPixelColor->SetFont(m_pMF->m_LBLFont);
	m_plblPixelColor->SetBackgroundColour(m_CL1);
	m_plblPixelColor->SetSize(rlPixelColor);

	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorRGB...\n");
	m_pPixelColorRGB = new CTextCtrl(m_pP2, wxID_ANY,
		&(m_pMF->m_cfg.m_pixel_color_bgr), rcPixelColorRGB.GetPosition(), rcPixelColorRGB.GetSize());
	m_pPixelColorRGB->SetFont(m_pMF->m_LBLFont);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorLab...\n");
	m_pPixelColorLab = new CTextCtrl(m_pP2, wxID_ANY,
		&(m_pMF->m_cfg.m_pixel_color_lab), rcPixelColorLab.GetPosition(), rcPixelColorLab.GetSize());
	m_pPixelColorLab->SetFont(m_pMF->m_LBLFont);

	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorExample...\n");
	m_pPixelColorExample = new CStaticText(m_pP2, wxID_ANY, wxT(""));
	m_pPixelColorExample->SetFont(m_pMF->m_LBLFont);
	m_pPixelColorExample->SetBackgroundColour(wxColour(255, 255, 255));
	m_pPixelColorExample->SetSize(rcPixelColorExample);

	SaveToReportLog("CSettingsPanel::Init(): finished.\n");
}

void CSettingsPanel::OnBnClickedTest(wxCommandEvent& event)
{
	simple_buffer<u8> ImBGR;
	int i, j, k, S=0;	
	char str[30];
	std::chrono::time_point<std::chrono::high_resolution_clock> t;
	wxString BaseImgName;	

	g_color_ranges = GetColorRanges(g_use_filter_color);
	g_outline_color_ranges = GetColorRanges(g_use_outline_filter_color);

	g_text_alignment = ConvertStringToTextAlignment(g_text_alignment_string);

	if (m_pMF->m_VIsOpen)
	{
		m_pMF->m_pPanel->Disable();

		m_pMF->m_pVideo->SetVideoWindowSettins(m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

		m_w = m_pMF->m_pVideo->m_w;
		m_h = m_pMF->m_pVideo->m_h;
		m_W = m_pMF->m_pVideo->m_Width;
		m_H = m_pMF->m_pVideo->m_Height;
		m_xmin = m_pMF->m_pVideo->m_xmin;
		m_ymin = m_pMF->m_pVideo->m_ymin;
		m_xmax = m_pMF->m_pVideo->m_xmax;
		m_ymax = m_pMF->m_pVideo->m_ymax;

		ImBGR.set_size(m_w*m_h*3);
		m_pMF->m_pVideo->GetBGRImage(ImBGR, m_xmin, m_xmax, m_ymin, m_ymax);		

		s64 CurPos = m_pMF->m_pVideo->GetPos();
		BaseImgName = GetFileName(m_pMF->m_pVideo->m_MovieName);
		BaseImgName += " -- " + VideoTimeToStr(CurPos);

		SaveBGRImage(ImBGR, wxT("/RGBImages/") + BaseImgName + wxT("_") + FormatImInfoAddData(m_W, m_H, m_xmin, m_ymin, m_w, m_h) + g_im_save_format, m_w, m_h);

		if (ImBGR.size() == 0)
		{
			return;
		}		

		m_ImF = custom_buffer<simple_buffer<u8>>(m_n, simple_buffer<u8>(m_w * m_h, 0));

		if (g_clear_test_images_folder) m_pMF->ClearDir(g_work_dir + "/TestImages");

		S = GetTransformedImage(ImBGR, m_ImF[0], m_ImF[1], m_ImF[2], m_ImF[3], m_ImF[4], m_w, m_h, m_W, m_H, 0, m_w - 1);

		if ((g_generate_cleared_text_images_on_test) && (!g_show_transformed_images_only))
		{
			vector<wxString> SavedFiles;
			SavedFiles.push_back(BaseImgName);
			simple_buffer<u8> ImIL;

			FindTextLines(ImBGR, m_ImF[4], m_ImF[2], m_ImF[0], m_ImF[3], ImIL, SavedFiles, m_w, m_h, m_W, m_H, m_xmin, m_ymin);
		}
	}
	else
	{
		wxDir dir(g_work_dir + "/RGBImages");
		wxString filename, ImgName;

		if (dir.GetFirst(&filename))
		{
			m_pMF->m_pPanel->Disable();

			ImgName = GetFileName(filename);

			wxString filepath = g_work_dir + "/RGBImages/" + filename;			

			GetImageSize(filepath, m_w, m_h);
			GetImInfo(ImgName, m_w, m_h, &m_W, &m_H, &m_xmin, &m_xmax, &m_ymin, &m_ymax, &BaseImgName);
			ImBGR.set_size(m_w * m_h * 3);
			LoadBGRImage(ImBGR, filepath);

			{
				simple_buffer<u8> ImTMP_BGR(m_W * m_H * 3);				
				ImBGRToNativeSize(ImBGR, ImTMP_BGR, m_w, m_h, m_W, m_H, m_xmin, m_xmax, m_ymin, m_ymax);
				g_pViewBGRImage[0](ImTMP_BGR, m_W, m_H);
			}

			m_pMF->m_pVideoBox->m_plblVB->SetLabel("VideoBox \"" + ImgName + "\"");

			if (g_clear_test_images_folder) m_pMF->ClearDir(g_work_dir + "/TestImages");

			FindTextLinesRes res;
			m_ImF = custom_buffer<simple_buffer<u8>>(m_n, simple_buffer<u8>(m_w * m_h, 0));
			res.m_pImFF = &m_ImF[0];
			res.m_pImSF = &m_ImF[1];
			res.m_pImTF = &m_ImF[2];
			res.m_pImNE = &m_ImF[3];
			res.m_pImY = &m_ImF[4];
			FindTextLines(filepath, res);
			
			if ((g_generate_cleared_text_images_on_test) && (!g_show_transformed_images_only))
			{
				m_ImF[4].copy_data(res.m_ImClearedText, res.m_ImClearedText.m_size);
			}
		}
	}	
	
	ViewCurImF();

	m_pMF->m_pPanel->Enable();
}

void CSettingsPanel::ViewCurImF()
{
	m_plblIF->SetLabel(StrFN[m_cn]);

	simple_buffer<u8> ImTMP_F(m_W * m_H);
	ImToNativeSize(m_ImF[m_cn], ImTMP_F, m_w, m_h, m_W, m_H, m_xmin, m_xmax, m_ymin, m_ymax);
	m_pMF->m_pImageBox->ViewGrayscaleImage(ImTMP_F, m_W, m_H);
}


void CSettingsPanel::OnBnClickedLeft(wxCommandEvent& event)
{
	m_cn--;
	if (m_cn < 0) m_cn = m_n-1;
	
	if (m_ImF.m_size > 0)
	{
		ViewCurImF();
	}
}

void CSettingsPanel::OnBnClickedRight(wxCommandEvent& event)
{
	m_cn++;
	if (m_cn > m_n-1) m_cn = 0;

	if (m_ImF.m_size > 0)
	{
		ViewCurImF();
	}
}

//BOOL CSettingsPanel::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
//{
//	if (pWnd == &m_Left)
//	{
//		::SetCursor(m_hCursor);	
//		return TRUE;
//	}
//	else if(pWnd == &m_Right)
//	{
//		::SetCursor(m_hCursor);	
//		return TRUE;
//	}
//	else
//	{
//		return CWnd::OnSetCursor(pWnd, nHitTest, message);
//	}
//}
//
//int CSettingsPanel::OnCreate(LPCREATESTRUCT lpCreateStruct)
//{
//	if (CWnd::OnCreate(lpCreateStruct) == -1)
//		return -1;
//
//	return 0;
//}
//
//void CSettingsPanel::OnPaint()
//{
//	CPaintDC dc(this); // device context for painting
//	// TODO: Add your message handler code here
//	// Do not call CWnd::OnPaint() for painting messages
//
//	m_OI.Invalidate(0);
//	m_OIM.Invalidate(0);
//}
