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

	m_pMF->get_StrFN();
	m_cn = g_cfg.m_StrFN.size() - 1;
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
	wxSize p2_min_size = rcP2.GetSize();
	m_pP2->SetMinSize(p2_min_size);
	m_pP2->SetBackgroundColour(g_cfg.m_notebook_panels_colour);	

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB1...\n");
	m_pGB1 = new CStaticBox( m_pP2, wxID_ANY,
		g_cfg.m_ssp_label_parameters_influencing_image_processing, rcGB1.GetPosition(), rcGB1.GetSize() );
	m_pGB1->SetFont(m_pMF->m_LBLFont);
	m_pGB1->SetTextColour(g_cfg.m_main_text_colour);
	m_pGB1->SetBackgroundColour(g_cfg.m_notebook_panels_colour);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB2...\n");
	m_pGB2 = new CStaticBox( m_pP2, wxID_ANY,
		g_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, rcGB2.GetPosition(), rcGB2.GetSize() );
	m_pGB2->SetFont(m_pMF->m_LBLFont);
	m_pGB2->SetTextColour(g_cfg.m_main_text_colour);
	m_pGB2->SetBackgroundColour(g_cfg.m_notebook_panels_colour);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGB3...\n");
	g_cfg.m_ssp_GB3_label = wxT("");
	m_pGB3 = new CStaticBox( m_pP2, wxID_ANY,
		g_cfg.m_ssp_GB3_label, rcGB3.GetPosition(), rcGB3.GetSize() );
	m_pGB3->SetFont(m_pMF->m_LBLFont);
	m_pGB3->SetTextColour(g_cfg.m_main_text_colour);
	m_pGB3->SetBackgroundColour(g_cfg.m_notebook_panels_colour);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pTest...\n");
	m_pTest = new CButton(m_pGB3, ID_TEST, g_cfg.m_main_buttons_colour, g_cfg.m_main_buttons_colour_focused, g_cfg.m_main_buttons_colour_selected, g_cfg.m_main_buttons_border_colour,
		g_cfg.m_button_test_text, wxDefaultPosition, rcTEST.GetSize() );
	rcTEST.GetPosition();
	m_pTest->SetFont(m_pMF->m_BTNFont);
	m_pTest->SetTextColour(g_cfg.m_main_text_colour);
	wxSize test_min_size = rcTEST.GetSize();
	m_pTest->SetMinSize(test_min_size);

	bmp_na = wxImage(g_app_dir + "/bitmaps/left_na.bmp");
	bmp_od = wxImage(g_app_dir + "/bitmaps/left_od.bmp");

	SaveToReportLog("CSettingsPanel::Init(): init m_pLeft...\n");
	m_pLeft = new CBitmapButton(m_pGB3, ID_SP_LEFT,
		bmp_na, bmp_od, rcLeft.GetPosition(), rcLeft.GetSize() );

	bmp_na = wxImage(g_app_dir + "/bitmaps/right_na.bmp");
	bmp_od = wxImage(g_app_dir + "/bitmaps/right_od.bmp");

	SaveToReportLog("CSettingsPanel::Init(): init m_pRight...\n");
	m_pRight = new CBitmapButton(m_pGB3, ID_SP_RIGHT,
		bmp_na, bmp_od, rcRight.GetPosition(), rcRight.GetSize() );

	SaveToReportLog("CSettingsPanel::Init(): init m_plblIF...\n");
	m_plblIF = new CStaticText(m_pGB3, g_cfg.m_StrFN[m_cn], wxID_ANY);
	m_plblIF->SetFont(m_pMF->m_LBLFont);
	m_plblIF->SetTextColour(g_cfg.m_main_text_colour);
	m_plblIF->SetBackgroundColour(g_cfg.m_test_result_label_colour);
	m_plblIF->SetSize(rlIF);

	SaveToReportLog("CSettingsPanel::Init(): init m_pOI...\n");	
	m_pOI = new CDataGrid(m_pGB1, g_cfg.m_grid_col_property_label, g_cfg.m_grid_col_value_label, ID_OI, &(m_pMF->m_LBLFont), &(g_cfg.m_main_text_colour),
                           rcOI.GetPosition(), rcOI.GetSize() );
	m_pOI->SetBackgroundColour(g_cfg.m_notebook_colour);
	m_pOI->SetGridLineColour(g_cfg.m_grid_line_colour);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_global_image_processing_settings...\n");
    m_pOI->AddGroup(g_cfg.m_ssp_oi_group_global_image_processing_settings, g_cfg.m_grid_gropes_colour);	
	
	m_pMF->get_available_text_alignments();
	m_pOI->AddProperty(g_cfg.m_label_text_alignment, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_text_alignment_string, g_cfg.m_available_text_alignments);

	m_pOI->AddProperty(g_cfg.m_label_use_filter_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_filter_color);
	m_pOI->AddProperty(g_cfg.m_label_use_outline_filter_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_outline_filter_color);
	m_pOI->AddProperty(g_cfg.m_label_dL_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_dL_color, 0, 255);
	m_pOI->AddProperty(g_cfg.m_label_dA_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_dA_color, 0, 255);
	m_pOI->AddProperty(g_cfg.m_label_dB_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_dB_color, 0, 255);	
	
	g_cfg.m_available_hw_device_types = GetAvailableHWDeviceTypes();
	m_pOI->AddProperty(g_cfg.m_ssp_hw_device, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_hw_device, g_cfg.m_available_hw_device_types);
	
	m_pOI->AddProperty(g_cfg.m_label_filter_descr, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_filter_descr);

#ifdef USE_CUDA
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_use_cuda_gpu, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_cuda_gpu);
#else
	//m_pOI->AddProperty(g_cfg.m_ssp_oi_property_use_cuda_gpu + " (only on x64 is supported)", g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_use_cuda_gpu);
	//m_pOI->SetReadOnly(m_pOI->GetNumberRows() - 1, 1, true);
#endif
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_use_ocl, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ocl);

	m_pOI->AddProperty(g_cfg.m_playback_sound, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_playback_sound);

	// debug settings
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_generate_cleared_text_images_on_test);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_dump_debug_images, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_show_results);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_show_sf_results);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_clear_test_images_folder, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_clear_test_images_folder);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_show_transformed_images_only, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_show_transformed_images_only);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_initial_image_processing...\n");
	m_pOI->AddGroup(g_cfg.m_ssp_oi_group_initial_image_processing, g_cfg.m_grid_gropes_colour);
	m_pOI->AddSubGroup(g_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, g_cfg.m_grid_sub_gropes_colour);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_moderate_threshold, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mthr, 0.0, 1.0);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_moderate_nedges_threshold, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mnthr, 0.0, 1.0);
	m_pOI->AddSubGroup(g_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, g_cfg.m_grid_sub_gropes_colour);	
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_segment_width, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_segw, 4, 50);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_segments_count, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_msegc, 1, 10);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_sum_color_difference, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_scd, 0, 10000);

	m_pOI->AddGroup(g_cfg.m_ssp_oi_group_secondary_image_processing, g_cfg.m_grid_gropes_colour);
	m_pOI->AddSubGroup(g_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, g_cfg.m_grid_sub_gropes_colour);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_line_height, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_segh, 1, 50);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_max_between_text_distance, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_btd, 0.0, 1.0);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_max_text_center_offset, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_to, 0.0, 1.0);	
	m_pOI->AddSubGroup(g_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, g_cfg.m_grid_sub_gropes_colour);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_points_number, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mpn, 0, 10000);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_points_density, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mpd, 0.0, 1.0);	

	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_nedges_points_density, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mpned, 0.0, 1.0);

	SaveToReportLog("CSettingsPanel::Init(): init m_pOI m_ssp_oi_group_tertiary_image_processing...\n");
	m_pOI->AddGroup(g_cfg.m_ssp_oi_group_tertiary_image_processing, g_cfg.m_grid_gropes_colour);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_symbol_height, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_msh, 0.0, 1.0);
	m_pOI->AddProperty(g_cfg.m_ssp_oi_property_min_symbol_density, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_msd, 0.0, 1.0);

	////////////////////////////////////////////////////////////////////////

	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM...\n");
	m_pOIM = new CDataGrid(m_pGB2, g_cfg.m_grid_col_property_label, g_cfg.m_grid_col_value_label, ID_OIM, &(m_pMF->m_LBLFont), &(g_cfg.m_main_text_colour),
                           rcOIM.GetPosition(), rcOIM.GetSize() );
	m_pOIM->SetBackgroundColour(g_cfg.m_notebook_colour);
	m_pOIM->SetGridLineColour(g_cfg.m_grid_line_colour);

	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM m_ssp_oim_group_ocr_settings...\n");
	m_pOIM->AddGroup(g_cfg.m_ssp_oim_group_ocr_settings, g_cfg.m_grid_gropes_colour);
	m_pOIM->AddProperty(g_cfg.m_border_is_darker, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_border_is_darker);
	m_pOIM->AddProperty(g_cfg.m_extend_by_grey_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_extend_by_grey_color);
	m_pOIM->AddProperty(g_cfg.m_allow_min_luminance, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_allow_min_luminance, 0, 255);
	m_pOIM->AddProperty(g_cfg.m_ssp_ocr_threads, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_ocr_threads, -1, 100);
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_image_scale_for_clear_image, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_scale, 1, 4);
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_smthr, 0.0, 1.0);
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_cpu_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_cpu_kmeans_loop_iterations, 1, 1000);
#ifdef USE_CUDA
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_cuda_kmeans_loop_iterations, 1, 1000);
#else
	//m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	//m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
	//m_pOIM->AddProperty(g_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, g_cfg.m_grid_debug_settings_colour, g_cfg.m_grid_debug_settings_colour, &g_cuda_kmeans_loop_iterations, 1, 1000);
	//m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
#endif
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_CLEAN_RGB_IMAGES);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_clear_images_logical, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_clear_image_logical);
	m_pOIM->AddProperty(g_cfg.m_label_combine_to_single_cluster, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_combine_to_single_cluster);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ISA_images_for_get_txt_area);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ILA_images_for_get_txt_area);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_gradient_images_for_clear_txt_images);
	m_pOIM->AddProperty(g_cfg.m_label_ILA_images_for_getting_txt_symbols_areas, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ILA_images_for_getting_txt_symbols_areas);
	m_pOIM->AddProperty(g_cfg.m_label_use_ILA_images_before_clear_txt_images_from_borders, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ILA_images_before_clear_txt_images_from_borders);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ILA_images_for_clear_txt_images);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_clear_txt_images_by_main_color);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_remove_wide_symbols, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_remove_wide_symbols);

	//m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, m_CLSP, m_CLSP, &g_ValidateAndCompareTXTImages);
	//m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);

	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_DontDeleteUnrecognizedImages1);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_DontDeleteUnrecognizedImages2);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_default_string_for_empty_sub, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_DefStringForEmptySub);
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pOIM m_ssp_oim_group_settings_for_multiframe_image_processing...\n");
	m_pOIM->AddGroup(g_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, g_cfg.m_grid_gropes_colour);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_threads, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_threads, -1, 100);
	m_pOIM->AddSubGroup(g_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, g_cfg.m_grid_sub_gropes_colour);		
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_sub_frames_length, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_DL, 1, 100);

	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ILA_images_for_search_subtitles);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_use_ISA_images_for_search_subtitles);	
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_replace_ISA_by_filtered_version);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_max_dl_down, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_max_dl_down, 0, 255);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_max_dl_up, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_max_dl_up, 0, 255);

	m_pOIM->AddSubGroup(g_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, g_cfg.m_grid_sub_gropes_colour);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_vedges_points_line_error, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_veple, 0.0, 10.0);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_ila_points_line_error, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_ilaple, 0.0, 10.0);
	m_pOIM->AddSubGroup(g_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, g_cfg.m_grid_sub_gropes_colour);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_text_percent, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_tp, 0.0, 1.0);
	m_pOIM->AddProperty(g_cfg.m_ssp_oim_property_min_text_length, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_mtpl, 0.0, 1.0);

	m_pOIM->AddSubGroup(g_cfg.m_ssp_oim_sub_group_settings_for_update_video_color, g_cfg.m_grid_sub_gropes_colour);
	m_pOIM->AddProperty(g_cfg.m_label_video_contrast, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_video_contrast, 0.0, 10.0);
	m_pOIM->AddProperty(g_cfg.m_label_video_gamma, g_cfg.m_main_labels_background_colour, g_cfg.m_main_text_ctls_background_colour, &g_video_gamma, 0.0, 10.0);

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
	m_plblGSFN = new CStaticText(m_pGB3, g_cfg.m_label_settings_file, wxID_ANY);
	m_plblGSFN->SetFont(m_pMF->m_LBLFont);
	m_plblGSFN->SetTextColour(g_cfg.m_main_text_colour);
	m_plblGSFN->SetBackgroundColour(g_cfg.m_main_labels_background_colour);
	m_plblGSFN->SetSize(rlGSFN);

	SaveToReportLog("CSettingsPanel::Init(): init m_pGSFN...\n");
	g_cfg.m_ssp_GSFN_label = g_GeneralSettingsFileName + wxT(" ");
	m_pGSFN = new CStaticText(m_pGB3, g_cfg.m_ssp_GSFN_label, wxID_ANY, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
	m_pGSFN->m_allow_auto_set_min_width = false;
	m_pGSFN->SetFont(m_pMF->m_LBLFont);
	m_pGSFN->SetTextColour(g_cfg.m_main_text_colour);
	m_pGSFN->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);
	m_pGSFN->SetSize(rcGSFN);

	SaveToReportLog("CSettingsPanel::Init(): init m_plblPixelColor...\n");
	m_plblPixelColor = new CStaticText(m_pGB3, g_cfg.m_label_pixel_color, wxID_ANY);
	m_plblPixelColor->SetFont(m_pMF->m_LBLFont);
	m_plblPixelColor->SetTextColour(g_cfg.m_main_text_colour);
	m_plblPixelColor->SetBackgroundColour(g_cfg.m_main_labels_background_colour);
	m_plblPixelColor->SetSize(rlPixelColor);

	m_PixelColorExample = wxColour(255, 255, 255);

	u8 bgr[3], lab[3], y;

	bgr[0] = m_PixelColorExample.Blue();
	bgr[1] = m_PixelColorExample.Green();
	bgr[2] = m_PixelColorExample.Red();

	BGRToYUV(bgr[0], bgr[1], bgr[2], &y);
	BGRToLab(bgr[0], bgr[1], bgr[2], &(lab[0]), &(lab[1]), &(lab[2]));

	g_cfg.m_pixel_color_bgr = wxString::Format(wxT("RGB: r:%d g:%d b:%d L:%d"), (int)(bgr[2]), (int)(bgr[1]), (int)(bgr[0]), (int)y);
	g_cfg.m_pixel_color_lab = wxString::Format(wxT("Lab: l:%d a:%d b:%d"), (int)(lab[0]), (int)(lab[1]), (int)(lab[2]));

	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorRGB...\n");
	m_pPixelColorRGB = new CTextCtrl(m_pGB3, wxID_ANY,
		&(g_cfg.m_pixel_color_bgr), wxString(), rcPixelColorRGB.GetPosition(), rcPixelColorRGB.GetSize());
	m_pPixelColorRGB->SetFont(m_pMF->m_LBLFont);
	m_pPixelColorRGB->SetTextColour(g_cfg.m_main_text_colour);
	m_pPixelColorRGB->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);
	m_pPixelColorRGB->SetEditable(false);	
	
	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorLab...\n");
	m_pPixelColorLab = new CTextCtrl(m_pGB3, wxID_ANY,
		&(g_cfg.m_pixel_color_lab), wxString(), rcPixelColorLab.GetPosition(), rcPixelColorLab.GetSize());
	m_pPixelColorLab->SetFont(m_pMF->m_LBLFont);
	m_pPixelColorLab->SetTextColour(g_cfg.m_main_text_colour);
	m_pPixelColorLab->SetBackgroundColour(g_cfg.m_main_text_ctls_background_colour);
	m_pPixelColorLab->SetEditable(false);

	SaveToReportLog("CSettingsPanel::Init(): init m_pPixelColorExample...\n");
	g_cfg.m_pixel_color_example = wxT("");
	m_pPixelColorExample = new CStaticText(m_pGB3, g_cfg.m_pixel_color_example, wxID_ANY);
	m_pPixelColorExample->SetFont(m_pMF->m_LBLFont);
	m_pPixelColorExample->SetTextColour(g_cfg.m_main_text_colour);
	m_pPixelColorExample->SetBackgroundColour(m_PixelColorExample);
	m_pPixelColorExample->SetSize(rcPixelColorExample);
	
	wxSize pce_min_size = rcPixelColorExample.GetSize();
	m_pPixelColorExample->SetMinSize(pce_min_size);

	// m_pP2 location sizer
	{
		SaveToReportLog("CSettingsPanel::Init(): init top_sizer...\n");
		wxBoxSizer* top_sizer = new wxBoxSizer(wxVERTICAL);
		SaveToReportLog("CSettingsPanel::Init(): init button_sizer...\n");
		wxBoxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
		button_sizer->Add(m_pP2, 1, wxEXPAND | wxALL);
		top_sizer->Add(button_sizer, 1, wxEXPAND | wxALL, 4);
		this->SetSizer(top_sizer);
	}

	// m_pP2 elements location sizer
	{
		wxStaticBoxSizer* gb1_sizer = new wxStaticBoxSizer(m_pGB1, wxVERTICAL);
		gb1_sizer->Add(m_pOI, 1, wxEXPAND | wxALL);
		
		wxStaticBoxSizer* gb2_sizer = new wxStaticBoxSizer(m_pGB2, wxVERTICAL);
		gb2_sizer->Add(m_pOIM, 1, wxEXPAND | wxALL);

		wxBoxSizer* gb3_vert_box_sizer = new wxBoxSizer(wxVERTICAL);
		wxBoxSizer* gb3_hor_box_sizer = new wxBoxSizer(wxHORIZONTAL);

		wxBoxSizer* test_result_sizer = new wxBoxSizer(wxHORIZONTAL);
		test_result_sizer->Add(m_pLeft, 0, wxEXPAND | wxSHAPED | wxALL);
		test_result_sizer->Add(m_plblIF, 1, wxEXPAND | wxALL);
		test_result_sizer->Add(m_pRight, 0, wxEXPAND | wxSHAPED | wxALL);

		wxBoxSizer* rgb_lab_sizer = new wxBoxSizer(wxVERTICAL);
		rgb_lab_sizer->Add(m_pPixelColorLab, 0, wxEXPAND | wxALL);
		rgb_lab_sizer->AddSpacer(2);
		rgb_lab_sizer->Add(m_pPixelColorRGB, 0, wxEXPAND | wxALL);

		wxBoxSizer* rgb_lab_color_sizer = new wxBoxSizer(wxHORIZONTAL);
		rgb_lab_color_sizer->Add(rgb_lab_sizer, 1, wxEXPAND | wxALL);
		rgb_lab_color_sizer->AddSpacer(2);
		rgb_lab_color_sizer->Add(m_pPixelColorExample, 0, wxEXPAND | wxALL);		

		wxBoxSizer* vert_right_ctrls_sizer = new wxBoxSizer(wxVERTICAL);
		vert_right_ctrls_sizer->Add(test_result_sizer, 0, wxEXPAND | wxALL);
		vert_right_ctrls_sizer->AddSpacer(4);
		vert_right_ctrls_sizer->Add(m_pTest, 0, wxALIGN_CENTER);
		vert_right_ctrls_sizer->AddSpacer(4);
		vert_right_ctrls_sizer->Add(m_plblGSFN, 0, wxEXPAND | wxALL);
		vert_right_ctrls_sizer->AddSpacer(2);
		vert_right_ctrls_sizer->Add(m_pGSFN, 0, wxEXPAND | wxALL);
		vert_right_ctrls_sizer->AddSpacer(4);
		vert_right_ctrls_sizer->Add(m_plblPixelColor, 0, wxEXPAND | wxALL);	
		vert_right_ctrls_sizer->AddSpacer(2);
		vert_right_ctrls_sizer->Add(rgb_lab_color_sizer, 0, wxEXPAND | wxALL);		

		gb3_hor_box_sizer->Add(vert_right_ctrls_sizer, 0, wxALIGN_CENTER);
		gb3_vert_box_sizer->AddSpacer(1);
		gb3_vert_box_sizer->Add(gb3_hor_box_sizer, 0, wxALIGN_TOP | wxALIGN_CENTER_HORIZONTAL);
		m_pGB3->SetSizer(gb3_vert_box_sizer);

		wxGridSizer* grids_boxs_sizer = new wxGridSizer(1, 2, 0, 4);
		grids_boxs_sizer->Add(gb1_sizer, 1, wxEXPAND | wxALL);
		grids_boxs_sizer->Add(gb2_sizer, 1, wxEXPAND | wxALL);
		
		wxBoxSizer* hor_box_sizer = new wxBoxSizer(wxHORIZONTAL);
		hor_box_sizer->AddSpacer(4);
		hor_box_sizer->Add(grids_boxs_sizer, 1, wxEXPAND | wxALL);
		hor_box_sizer->AddSpacer(4);
		hor_box_sizer->Add(m_pGB3, 0, wxEXPAND | wxALL);
		hor_box_sizer->AddSpacer(4);

		wxBoxSizer* vert_box_sizer = new wxBoxSizer(wxVERTICAL);
		vert_box_sizer->AddSpacer(2);
		vert_box_sizer->Add(hor_box_sizer, 1, wxEXPAND | wxALL);
		vert_box_sizer->AddSpacer(2);
		m_pP2->SetSizer(vert_box_sizer);
	}

	SaveToReportLog("CSettingsPanel::Init(): finished.\n");
}

void CSettingsPanel::UpdateSize()
{
	wxPoint  gb1_pos = m_pGB1->GetScreenPosition();
	wxPoint  oi_pos = m_pOI->GetScreenPosition();

	m_pGB3->GetSizer()->GetItem((size_t)0)->AssignSpacer(0, oi_pos.y - gb1_pos.y);

	this->GetSizer()->Layout();
}

void CSettingsPanel::RefreshData()
{
	m_pP2->SetBackgroundColour(g_cfg.m_notebook_panels_colour);
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

		m_pMF->m_pVideo->SetVideoWindowSettins(
			std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos),
			std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos),
			std::min<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos),
			std::max<double>(g_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, g_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos));

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

		SaveBGRImage(ImBGR, wxT("/TestImages/RGBImages/") + BaseImgName + wxT("_") + FormatImInfoAddData(m_W, m_H, m_xmin, m_ymin, m_w, m_h) + g_im_save_format, m_w, m_h);

		if (ImBGR.size() == 0)
		{
			return;
		}		

		m_ImF = custom_buffer<simple_buffer<u8>>(g_cfg.m_StrFN.size(), simple_buffer<u8>(m_w * m_h, (u8)0));

		if (g_clear_test_images_folder) m_pMF->ClearDir(g_work_dir + "/DebugImages");

		S = GetTransformedImage(ImBGR, m_ImF[0], m_ImF[1], m_ImF[2], m_ImF[3], m_ImF[4], m_w, m_h, m_W, m_H, 0, m_w - 1);

		if ((g_generate_cleared_text_images_on_test) && (!g_show_transformed_images_only))
		{
			simple_buffer<u8> ImIL;

			m_ImF[4] = simple_buffer<u8>(m_w * g_scale * m_h * g_scale, (u8)0);
			FindTextLines(ImBGR, m_ImF[4], m_ImF[2], m_ImF[0], m_ImF[3], ImIL, wxString(wxT("/TestImages/TXTImages/")), BaseImgName, m_w, m_h, m_W, m_H, m_xmin, m_ymin);
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

			g_cfg.m_on_test_image_name = ImgName;
			m_pMF->get_video_box_lblVB_on_test_title();
			m_pMF->m_pVideoBox->m_plblVB->SetLabel(g_cfg.m_video_box_lblVB_title);

			if (g_clear_test_images_folder) m_pMF->ClearDir(g_work_dir + "/DebugImages");

			FindTextLinesRes res(wxT("/TestImages/TXTImages/"));
			m_ImF = custom_buffer<simple_buffer<u8>>(g_cfg.m_StrFN.size(), simple_buffer<u8>(m_w * m_h, (u8)0));
			res.m_pImFF = &m_ImF[0];
			res.m_pImSF = &m_ImF[1];
			res.m_pImTF = &m_ImF[2];
			res.m_pImNE = &m_ImF[3];
			res.m_pImY = &m_ImF[4];
			FindTextLines(filepath, res);
			
			if ((g_generate_cleared_text_images_on_test) && (!g_show_transformed_images_only))
			{
				m_ImF[4] = res.m_ImClearedTextScaled;
			}
		}
		else
		{
			return;
		}
	}	
	
	ViewCurImF();

	m_pMF->m_pPanel->Enable();
}

void CSettingsPanel::ViewCurImF()
{
	m_plblIF->SetLabel(g_cfg.m_StrFN[m_cn]);

	if (m_ImF[m_cn].m_size == m_w * m_h)
	{
		simple_buffer<u8> ImTMP_F(m_W * m_H);
		ImToNativeSize(m_ImF[m_cn], ImTMP_F, m_w, m_h, m_W, m_H, m_xmin, m_xmax, m_ymin, m_ymax);
		m_pMF->m_pImageBox->ViewGrayscaleImage(ImTMP_F, m_W, m_H);
	}
	else if (m_ImF[m_cn].m_size == m_w * g_scale * m_h * g_scale)
	{
		simple_buffer<u8> ImTMP_F(m_W * g_scale * m_H * g_scale);
		ImToNativeSize(m_ImF[m_cn], ImTMP_F, m_w * g_scale, m_h * g_scale, m_W * g_scale, m_H * g_scale, m_xmin * g_scale, m_xmax * g_scale, m_ymin * g_scale, m_ymax * g_scale);
		m_pMF->m_pImageBox->ViewGrayscaleImage(ImTMP_F, m_W * g_scale, m_H * g_scale);
	}
}


void CSettingsPanel::OnBnClickedLeft(wxCommandEvent& event)
{
	m_cn--;
	if (m_cn < 0) m_cn = g_cfg.m_StrFN.size() - 1;
	
	if (m_ImF.m_size > 0)
	{
		ViewCurImF();
	}
}

void CSettingsPanel::OnBnClickedRight(wxCommandEvent& event)
{
	m_cn++;
	if (m_cn > g_cfg.m_StrFN.size() - 1) m_cn = 0;

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
