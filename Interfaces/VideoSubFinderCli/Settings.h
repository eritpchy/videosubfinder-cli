static void loadSettings(const fs::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read settings: " + path.string());
    Settings g_general_settings;
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0]=='#') continue;
        auto separator = line.find('=');
        if (separator == std::string::npos) throw std::runtime_error("Invalid settings line: " + line);
        g_general_settings[trim(line.substr(0, separator))] = trim(line.substr(separator+1));
    }
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


	ReadProperty(g_general_settings, g_save_each_substring_separately, "save_each_substring_separately");
	ReadProperty(g_general_settings, g_save_scaled_images, "save_scaled_images");

	ReadProperty(g_general_settings, g_border_is_darker, "border_is_darker");

	int text_alignment = static_cast<int>(g_text_alignment);
	ReadProperty(g_general_settings, text_alignment, "text_alignment");
	if (text_alignment < 0 || text_alignment > 3) throw std::runtime_error("Invalid text alignment");
	g_text_alignment = static_cast<TextAlignment>(text_alignment);

	ReadProperty(g_general_settings, g_extend_by_grey_color, "extend_by_grey_color");
	ReadProperty(g_general_settings, g_allow_min_luminance, "allow_min_luminance");

}
