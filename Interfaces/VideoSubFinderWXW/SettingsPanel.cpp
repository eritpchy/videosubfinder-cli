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

vector<string> StrFN;

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
	StrFN[0] = string("After First Filtration");
	StrFN[1] = string("After Second Filtration");
	StrFN[2] = string("After Third Filtration");
	StrFN[3] = string("NEdges Points Image");
	StrFN[4] = string("Cleared Text Image");
	m_cn = 4;
	m_W = 0;
	m_H = 0;
}

CSettingsPanel::~CSettingsPanel()
{
}

void CSettingsPanel::Init()
{
	int bbw, bbh, w, h, dw, dh;
	wxRect rcP2, rcClP2, rcGB1, rcGB2, rcGB3; 
	wxRect rcTEST, rcLeft, rcRight, rlIF;
	wxRect rcOI, rcOIM;
	wxBitmap bmp_na, bmp_od;
	//CObjectInspector::CProperty *pProp;

	m_CLDBG = wxColour(200, 200, 200);
	m_CLSP = wxColour(170,170,170);
	m_CL1 = wxColour(255, 215, 0);
	m_CL2 = wxColour(127, 255, 0);
	m_CL3 = wxColour(127, 255, 212);
	m_CL4 = wxColour(255, 255, 0);
	m_CLGG = wxColour(0, 255, 255);

	//"Arial Black"
	m_BTNFont = wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_BOLD, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

	//"Microsoft Sans Serif"
	m_LBLFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);
	
	
	rcP2 = this->GetRect();

	this->GetClientSize(&w, &h);
	rcClP2.x = rcClP2.y = 0; 
	rcClP2.width = w;
	rcClP2.height = h;

	dw = rcP2.width - rcClP2.width;
	dh = rcP2.height - rcClP2.height;

	rcGB1.x = 10;
	rcGB1.y = 2;
	rcGB1.width = 400;
	rcGB1.height = 200;

	rcOI.x = rcGB1.x + 3;
	rcOI.y = rcGB1.y + 15;
	rcOI.width = rcGB1.width - 3*2;
	rcOI.height = rcGB1.height - 15 - 3;

	rcGB2.x = rcGB1.GetRight() + 5;
	rcGB2.y = rcGB1.y;
	rcGB2.width = rcGB1.width;
	rcGB2.height = rcGB1.height;

	rcOIM.x = rcGB2.x + 3;
	rcOIM.y = rcGB2.y + 15;
	rcOIM.width = rcGB2.width - 3*2;
	rcOIM.height = rcGB2.height - 15 - 3;

	rcTEST.x = rcGB2.GetRight() + 70;
	rcTEST.y = 70;
	rcTEST.width = 100;
	rcTEST.height = 30;

	bbw = 23;
	bbh = 22;

	rcLeft.x = rcTEST.x - 50;
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

	rcGB3.x = rcGB2.GetRight() + 5;
	rcGB3.y = rcGB2.y;
	rcGB3.width = rcGB2.GetRight() + 2*(rcTEST.x+rcTEST.width/2-rcGB2.GetRight())-5-rcGB3.x;
	rcGB3.height = rcGB2.height;

	rcP2.x = 10;
	rcP2.y = 10;
	rcP2.width = rcGB3.GetRight() + 10 + dw;
	rcP2.height = (rcGB2.GetBottom() + 5) + dh;

	this->SetSize(rcP2);

	m_pP2 = new wxPanel( this, wxID_ANY, rcP2.GetPosition(), rcP2.GetSize() );
	m_pP2->SetMinSize(rcP2.GetSize());
	m_pP2->SetBackgroundColour( m_CLSP );

	wxBoxSizer *top_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

	button_sizer->Add(m_pP2, 1, wxALIGN_CENTER, 0 );

	top_sizer->Add(button_sizer, 1, wxALIGN_CENTER );

	this->SetSizer(top_sizer);

	m_pGB1 = new wxStaticBox( m_pP2, wxID_ANY,
		m_pMF->m_cfg.m_ssp_label_parameters_influencing_image_processing, rcGB1.GetPosition(), rcGB1.GetSize() );
	m_pGB1->SetFont(m_LBLFont);

	m_pGB2 = new wxStaticBox( m_pP2, wxID_ANY,
		m_pMF->m_cfg.m_ssp_label_ocl_and_multiframe_image_stream_processing, rcGB2.GetPosition(), rcGB2.GetSize() );
	m_pGB2->SetFont(m_LBLFont);

	m_pGB3 = new wxStaticBox( m_pP2, wxID_ANY,
		wxT(""), rcGB3.GetPosition(), rcGB3.GetSize() );
	m_pGB3->SetFont(m_LBLFont);
	
	
	m_pTest = new wxButton( m_pP2, ID_TEST,
		wxT("Test"), rcTEST.GetPosition(), rcTEST.GetSize() );
	m_pTest->SetFont(m_BTNFont);
	
	bmp_na = wxBitmap(wxImage(g_app_dir + "/bitmaps/left_na.bmp"));
	bmp_od = wxBitmap(wxImage(g_app_dir + "/bitmaps/left_od.bmp"));

	m_pLeft = new wxBitmapButton( m_pP2, ID_SP_LEFT,
		bmp_na, rcLeft.GetPosition(), rcLeft.GetSize() );
	m_pLeft->SetBitmapSelected(bmp_od);

	bmp_na = wxBitmap(wxImage(g_app_dir + "/bitmaps/right_na.bmp"));
	bmp_od = wxBitmap(wxImage(g_app_dir + "/bitmaps/right_od.bmp"));

	m_pRight = new wxBitmapButton( m_pP2, ID_SP_RIGHT,
		bmp_na, rcRight.GetPosition(), rcRight.GetSize() );
	m_pRight->SetBitmapSelected(bmp_od);

	m_plblIF = new CTextBox( m_pP2, wxID_ANY, wxString(StrFN[m_cn]));
	m_plblIF->SetFont(m_LBLFont);
	m_plblIF->SetBackgroundColour( m_CL3 );
	m_plblIF->SetSize(rlIF);

	m_pOI = new CDataGrid( m_pP2, ID_OI,
                           rcOI.GetPosition(), rcOI.GetSize() );

    m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_global_image_processing_settings, m_CLGG, m_LBLFont);	
	
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_hw_device, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_hw_device, GetAvailableHWDeviceTypes());

#ifdef WIN64
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_cuda_gpu, m_CL2, m_CL4, m_LBLFont, &g_use_cuda_gpu);
#else
	//m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_cuda_gpu + " (only on x64 is supported)", m_CLSP, m_CLSP, m_LBLFont, &g_use_cuda_gpu);
	//m_pOI->SetReadOnly(m_pOI->GetNumberRows() - 1, 1, true);
#endif
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_use_ocl, m_CL2, m_CL4, m_LBLFont, &g_use_ocl);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_generate_cleared_text_images_on_test, m_CLDBG, m_CLDBG, m_LBLFont, &g_generate_cleared_text_images_on_test);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_dump_debug_images, m_CLDBG, m_CLDBG, m_LBLFont, &g_show_results);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_dump_debug_second_filtration_images, m_CLDBG, m_CLDBG, m_LBLFont, &g_show_sf_results);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_clear_test_images_folder, m_CLDBG, m_CLDBG, m_LBLFont, &g_clear_test_images_folder);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_show_transformed_images_only, m_CLDBG, m_CLDBG, m_LBLFont, &g_show_transformed_images_only);
	
	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_initial_image_processing, m_CLGG, m_LBLFont);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_sobel_operators, m_CL1, m_LBLFont);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_threshold, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mthr, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_nedges_threshold, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mnthr, 0.0, 1.0);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_color_filtering, m_CL1, m_LBLFont);	
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_segment_width, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_segw, 4, 50);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_segments_count, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_msegc, 1, 10);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_sum_color_difference, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_scd, 0, 10000);

	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_secondary_image_processing, m_CLGG, m_LBLFont);
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_linear_filtering, m_CL1, m_LBLFont);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_line_height, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_segh, 1, 50);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_max_between_text_distance, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_btd, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_max_text_center_offset, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_tco, 0.0, 1.0);	
	m_pOI->AddSubGroup(m_pMF->m_cfg.m_ssp_oi_sub_group_settings_for_color_border_points, m_CL1, m_LBLFont);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_points_number, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mpn, 0, 10000);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_points_density, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mpd, 0.0, 1.0);	

	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_nedges_points_density, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mpned, 0.0, 1.0);

	m_pOI->AddGroup(m_pMF->m_cfg.m_ssp_oi_group_tertiary_image_processing, m_CLGG, m_LBLFont);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_symbol_height, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_msh, 0.0, 1.0);
	m_pOI->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_min_symbol_density, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_msd, 0.0, 1.0);

	m_pOI->SetColSize(0, m_pOI->GetClientSize().x*0.75);
	m_pOI->SetColSize(1, m_pOI->GetClientSize().x*0.25);

	////////////////////////////////////////////////////////////////////////

	m_pOIM = new CDataGrid( m_pP2, ID_OIM,
                           rcOIM.GetPosition(), rcOIM.GetSize() );

	m_pOIM->AddGroup(m_pMF->m_cfg.m_ssp_oim_group_ocr_settings, m_CLGG, m_LBLFont);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_ocr_threads, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_ocr_threads, 1, 100);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_image_scale_for_clear_image, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_scale, 1, 4);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_moderate_threshold_for_scaled_image, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_smthr, 0.0, 1.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cpu_kmeans_initial_loop_iterations, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_cpu_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cpu_kmeans_loop_iterations, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_cpu_kmeans_loop_iterations, 1, 1000);
#ifdef WIN64
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_cuda_kmeans_loop_iterations, 1, 1000);
#else
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_initial_loop_iterations, m_CLSP, m_CLSP, m_LBLFont, m_LBLFont, &g_cuda_kmeans_initial_loop_iterations, 1, 1000);
	m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oi_property_cuda_kmeans_loop_iterations, m_CLSP, m_CLSP, m_LBLFont, m_LBLFont, &g_cuda_kmeans_loop_iterations, 1, 1000);
	m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
#endif
	//m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_images_logical, m_CL2, m_CL4, m_LBLFont, &g_clear_image_logical);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_rgbimages_after_search_subtitles, m_CL2, m_CL4, m_LBLFont, &g_CLEAN_RGB_IMAGES);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_using_isaimages_for_getting_txt_areas, m_CL2, m_CL4, m_LBLFont, &g_use_ISA_images_for_get_txt_area);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_using_ilaimages_for_getting_txt_areas, m_CL2, m_CL4, m_LBLFont, &g_use_ILA_images_for_get_txt_area);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_validate_and_compare_cleared_txt_images, m_CLSP, m_CLSP, m_LBLFont, &g_ValidateAndCompareTXTImages);
	m_pOIM->SetReadOnly(m_pOIM->GetNumberRows() - 1, 1, true);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_first, m_CL2, m_CL4, m_LBLFont, &g_DontDeleteUnrecognizedImages1);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_dont_delete_unrecognized_images_second, m_CL2, m_CL4, m_LBLFont, &g_DontDeleteUnrecognizedImages2);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_default_string_for_empty_sub, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_DefStringForEmptySub);
	
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_gradient_images_for_clear_txt_images, m_CL2, m_CL4, m_LBLFont, &g_use_gradient_images_for_clear_txt_images);	
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ILA_images_for_clear_txt_images, m_CL2, m_CL4, m_LBLFont, &g_use_ILA_images_for_clear_txt_images);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_clear_txt_images_by_main_color, m_CL2, m_CL4, m_LBLFont, &g_clear_txt_images_by_main_color);

	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_remove_wide_symbols, m_CL2, m_CL4, m_LBLFont, &g_remove_wide_symbols);

	m_pOIM->AddGroup(m_pMF->m_cfg.m_ssp_oim_group_settings_for_multiframe_image_processing, m_CLGG, m_LBLFont);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_threads, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_threads, -1, 100);
	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_sub_detection, m_CL1, m_LBLFont);		
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_sub_frames_length, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_DL, 1, 100);

	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ILA_images_for_search_subtitles, m_CL2, m_CL4, m_LBLFont, &g_use_ILA_images_for_search_subtitles);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_use_ISA_images_for_search_subtitles, m_CL2, m_CL4, m_LBLFont, &g_use_ISA_images_for_search_subtitles);	
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_replace_ISA_by_filtered_version, m_CL2, m_CL4, m_LBLFont, &g_replace_ISA_by_filtered_version);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_max_dl_down, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_max_dl_down, 0, 255);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_max_dl_up, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_max_dl_up, 0, 255);

	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_comparing_subs, m_CL1, m_LBLFont);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_vedges_points_line_error, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_veple, 0.0, 1.0);
	m_pOIM->AddSubGroup(m_pMF->m_cfg.m_ssp_oim_sub_group_settings_for_checking_sub, m_CL1, m_LBLFont);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_text_procent, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_tp, 0.0, 1.0);
	m_pOIM->AddProperty(m_pMF->m_cfg.m_ssp_oim_property_min_text_length, m_CL2, m_CL4, m_LBLFont, m_LBLFont, &g_mtpl, 0.0, 1.0);

	m_pOIM->SetColSize(0, m_pOIM->GetClientSize().x*0.75);
	m_pOIM->SetColSize(1, m_pOIM->GetClientSize().x*0.25);
}

void CSettingsPanel::OnBnClickedTest(wxCommandEvent& event)
{
	simple_buffer<u8> ImBGR;
	int i, j, k, w, h, W, H, xmin, xmax, ymin, ymax, S=0;	
	char str[30];
	clock_t t;
	wxString ImgName;
		
	if (m_pMF->m_VIsOpen)
	{
		m_pMF->m_pVideo->SetVideoWindowSettins(m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos,
			m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

		w = m_pMF->m_pVideo->m_w;
		h = m_pMF->m_pVideo->m_h;
		m_W = W = m_pMF->m_pVideo->m_Width;
		m_H = H = m_pMF->m_pVideo->m_Height;
		xmin = m_pMF->m_pVideo->m_xmin;
		xmax = m_pMF->m_pVideo->m_xmax;
		ymin = m_pMF->m_pVideo->m_ymin;
		ymax = m_pMF->m_pVideo->m_ymax;

		ImBGR.set_size(w*h*3);

		m_pMF->m_pVideo->GetBGRImage(ImBGR, xmin, xmax, ymin, ymax);

		s64 CurPos = m_pMF->m_pVideo->GetPos();
		ImgName = GetFileName(m_pMF->m_pVideo->m_MovieName);
		ImgName += " -- " + VideoTimeToStr(CurPos);
	}
	else
	{
		wxDir dir(g_work_dir + "/RGBImages");
		wxString filename;

		if (dir.GetFirst(&filename))
		{
			string filepath = g_work_dir + "/RGBImages/" + filename;

			GetImageSize(filepath, w, h);
			
			m_W = W = w;
			m_H = H = h;

			xmin = 0;
			xmax = w - 1;
			ymin = 0;
			ymax = h - 1;

			ImBGR.set_size(W * H * 3);			
			LoadBGRImage(ImBGR, filepath);

			g_pViewBGRImage[0](ImBGR, W, H);

			ImgName = GetFileName(filename);
		}
	}

	if (ImBGR.size() == 0)
	{
		return;
	}
	
	m_pMF->m_pPanel->Disable();	

	m_ImF = custom_buffer<simple_buffer<u8>> (m_n, simple_buffer<u8>(W*H, 0));
	
	if (g_clear_test_images_folder) m_pMF->ClearDir(g_work_dir + "/TestImages");

	S = GetTransformedImage(ImBGR, m_ImF[0], m_ImF[1], m_ImF[2], m_ImF[3], m_ImF[4], w, h, W, H);
	
	if ((g_generate_cleared_text_images_on_test) && (!g_show_transformed_images_only))
	{
		vector<wxString> SavedFiles;
		SavedFiles.push_back(ImgName);
		simple_buffer<u8> ImIL;

		FindTextLines(ImBGR, m_ImF[4], m_ImF[2], m_ImF[0], m_ImF[3], ImIL, SavedFiles, w, h);
	}

	if (S > 0)
	{
		if ((w != W) || (h != H))
		{
			for(k=0; k<m_n; k++)
			{
				ImToNativeSize(m_ImF[k], w, h, W, H, xmin, xmax, ymin, ymax);
			}
		}
	}

	m_plblIF->SetLabel(StrFN[m_cn]);
	m_pMF->m_pImageBox->ViewGrayscaleImage(m_ImF[m_cn], W, H);

	m_pMF->m_pPanel->Enable();
}

void CSettingsPanel::OnBnClickedLeft(wxCommandEvent& event)
{
	m_cn--;
	if (m_cn < 0) m_cn = m_n-1;
	
	if (m_ImF.m_size > 0)
	{
		m_plblIF->SetLabel(StrFN[m_cn]);	
		m_pMF->m_pImageBox->ViewGrayscaleImage(m_ImF[m_cn], m_W, m_H);
	}
}

void CSettingsPanel::OnBnClickedRight(wxCommandEvent& event)
{
	m_cn++;
	if (m_cn > m_n-1) m_cn = 0;

	if (m_ImF.m_size > 0)
	{
		m_plblIF->SetLabel(StrFN[m_cn]);
		m_pMF->m_pImageBox->ViewGrayscaleImage(m_ImF[m_cn], m_W, m_H);
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
