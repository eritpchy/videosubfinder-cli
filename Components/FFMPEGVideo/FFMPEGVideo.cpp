                              //FFMPEGVideo.cpp//                                
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

#include "FFMPEGVideo.h"
#include "IPAlgorithms.h"

#ifdef USE_CUDA
#include "cuda_kernels.h"
#endif

wxString g_hw_device = wxT("cpu");
wxString g_filter_descr;

bool g_use_hw_acceleration = false;
bool g_bln_get_hw_format = true;
bool g_use_filter = false;
AVPixelFormat dest_fmt = AV_PIX_FMT_BGR24;

wxArrayString GetAvailableHWDeviceTypes()
{
	SaveToReportLog("GetAvailableHWDeviceTypes: starting...\n");

	wxArrayString res;
	enum AVHWDeviceType type;

	res.Add(wxT("cpu"));

	type = AV_HWDEVICE_TYPE_NONE;
	while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
	{		
		res.Add(av_hwdevice_get_type_name(type));
	}

	SaveToReportLog("GetAvailableHWDeviceTypes: finished.\n");
	
	return res;
}
/////////////////////////////////////////////////////////////////////////////

FFMPEGVideo::FFMPEGVideo()
{
	m_Inited=false;
	m_IsSetNullRender = false;
    
    m_pBuffer=NULL;
	m_st = 0;

	m_type = 1;

	/*m_pBmp = NULL;
	m_pBmpScaled = NULL;*/

	m_pVideoWindow = NULL;

	m_pThreadRunVideo = NULL;

	m_play_video = false;
}

/////////////////////////////////////////////////////////////////////////////

FFMPEGVideo::~FFMPEGVideo()
{	
	if (input_ctx) {
		CloseMovie();
	}

	/*if (m_pBmp != NULL)
	{
		delete m_pBmp;
		m_pBmp = NULL;
	}

	if (m_pBmpScaled != NULL)
	{
		delete m_pBmpScaled;
		m_pBmpScaled = NULL;
	}*/	
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::ShowFrame(void *dc)
{
	if ((m_show_video) && (dc != NULL))
	{
		int ret;		
		uint8_t* dst_data[4] = { NULL };
		int dst_linesize[4];

		/* buffer is going to be written to rawvideo file, no alignment */
		if ((ret = av_image_alloc(dst_data, dst_linesize,
			m_Width, m_Height, dest_fmt, 1)) < 0) {
			custom_assert(ret > 0, "FFMPEGVideo::ShowFrame\nCould not allocate memory for destination image.");
		}

		if (ret > 0)
		{
			ret = convert_to_dst_format(m_frame_buffer.m_pData, dst_data, dst_linesize, dest_fmt);
		}

		if (ret > 0)
		{
			int wnd_w, wnd_h, img_w = m_Width, img_h = m_Height, num_pixels = img_w * img_h;
			((wxPaintDC*)dc)->GetSize(&wnd_w, &wnd_h);

			if ((wnd_w > 0) && (wnd_h > 0) && (img_w > 0) && (img_h > 0))
			{
				unsigned char* img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage
				for (int i = 0; i < num_pixels; i++)
				{
					img_data[i * 3] = dst_data[0][i * 3 + 2]; //R
					img_data[i * 3 + 1] = dst_data[0][i * 3 + 1]; //G
					img_data[i * 3 + 2] = dst_data[0][i * 3]; //B
				}
				UpdateImageColor(img_data, img_w, img_h);

				((wxPaintDC*)dc)->DrawBitmap(wxImage(img_w, img_h, img_data).Scale(wnd_w, wnd_h), 0, 0);
			}
		}

		if (dst_data[0]) av_freep(&dst_data[0]);
	}
}

/////////////////////////////////////////////////////////////////////////////
// https://ffmpeg.org/doxygen/trunk/hwcontext_8c_source.html
//65 static const char *const hw_type_names[] = {
//66[AV_HWDEVICE_TYPE_CUDA] = "cuda",
//67[AV_HWDEVICE_TYPE_DRM] = "drm",
//68[AV_HWDEVICE_TYPE_DXVA2] = "dxva2",
//69[AV_HWDEVICE_TYPE_D3D11VA] = "d3d11va",
//70[AV_HWDEVICE_TYPE_OPENCL] = "opencl",
//71[AV_HWDEVICE_TYPE_QSV] = "qsv",
//72[AV_HWDEVICE_TYPE_VAAPI] = "vaapi",
//73[AV_HWDEVICE_TYPE_VDPAU] = "vdpau",
//74[AV_HWDEVICE_TYPE_VIDEOTOOLBOX] = "videotoolbox",
//75[AV_HWDEVICE_TYPE_MEDIACODEC] = "mediacodec",
//76 };

static enum AVPixelFormat hw_pix_fmt;
static enum AVPixelFormat get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
	const enum AVPixelFormat *p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	g_bln_get_hw_format = false;
	return AV_PIX_FMT_NONE;
}

int FFMPEGVideo::hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type)
{
	int err = 0;

	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
		NULL, NULL, 0)) < 0) {
		wxMessageBox("Failed to create specified HW device.", "FFMPEGVideo::hw_decoder_init");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	return err;
}

int FFMPEGVideo::decode_frame(s64 &frame_pos)
{
	AVFrame* cur_frame = NULL;
	int size;
	int ret = 0;	

	ret = avcodec_receive_frame(decoder_ctx, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		av_frame_unref(frame);
		return 0;
	}
	else if (ret < 0) {
		fprintf(stderr, "Error while decoding\n");
		av_frame_unref(frame);
		return ret;
	}

	frame->pts = frame->best_effort_timestamp;

	m_cur_pts = frame->pts;

	frame_pos = av_rescale(m_cur_pts - m_start_pts, video->time_base.num * 1000, video->time_base.den);
	
	if (g_use_hw_acceleration && (frame->format == hw_pix_fmt)) {

		/* retrieve data from GPU to CPU */			
		if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
			fprintf(stderr, "Error transferring the data to system memory\n");
			av_frame_unref(frame);
			return ret;
		}
		cur_frame = sw_frame;		
		av_frame_unref(frame);
	}
	else
		cur_frame = frame;	
	
	if (g_use_filter)
	{
		/* push the decoded frame into the filtergraph */
		if (ret = av_buffersrc_add_frame_flags(buffersrc_ctx, cur_frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
			av_frame_unref(cur_frame);
			return ret;
		}

		/* pull filtered frames from the filtergraph */
		ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
		{
			av_frame_unref(cur_frame);
			return 0;
		}
		if (ret < 0)
		{
			av_frame_unref(cur_frame);
			return ret;
		}
		
		av_frame_unref(cur_frame);
		cur_frame = filt_frame;
	}

	if (m_frame_buffer_size == -1)
	{	
		src_fmt = (AVPixelFormat)cur_frame->format;
		m_frame_buffer_size = av_image_get_buffer_size(src_fmt, cur_frame->width, cur_frame->height, 1);
		m_frame_buffer.set_size(m_frame_buffer_size);
		m_origWidth = cur_frame->width;
		m_origHeight = cur_frame->height;

		m_Width = m_origWidth;
		m_Height = m_origHeight;

		m_start_pts = m_cur_pts;
		frame_pos = 0;
	}
	
	custom_assert(m_origWidth == cur_frame->width, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: m_origWidth == cur_frame->width");
	custom_assert(m_origHeight == cur_frame->height, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: m_origHeight == cur_frame->height");
	custom_assert(src_fmt == (AVPixelFormat)cur_frame->format, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: src_fmt == (AVPixelFormat)cur_frame->format");
	custom_assert(m_frame_buffer_size == av_image_get_buffer_size((AVPixelFormat)cur_frame->format, cur_frame->width, cur_frame->height, 1), "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: m_frame_buffer_size == av_image_get_buffer_size((AVPixelFormat)cur_frame->format, cur_frame->width, cur_frame->height, 1)");

	ret = av_image_copy_to_buffer(&m_frame_buffer[0], m_frame_buffer_size, cur_frame->data, cur_frame->linesize, src_fmt, cur_frame->width, cur_frame->height, 1);
	
	ret = 1;
	
	av_frame_unref(cur_frame);
	
	return ret;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::OneStep()
{
	m_ImageGeted = false;

	if (input_ctx)
	{		
		s64 prevPos = m_Pos, curPos = m_Pos;
		int ret;
		int num_tries = 0;

		do
		{
			ret = 0;
			while (ret >= 0)
			{
				if (need_to_read_packet == true)
				{
					while (need_to_read_packet)
					{
						if ((ret = av_read_frame(input_ctx, &packet)) < 0)
						{
							break;
						}

						if (video_stream != packet.stream_index)
						{
							av_packet_unref(&packet);
						}
						else
						{							
							need_to_read_packet = false;
						}
					}					

					if (ret < 0)
					{
						av_packet_unref(&packet);
						break;
					}

					custom_assert(video_stream == packet.stream_index, "FFMPEGVideo::OneStep(): not: video_stream == packet.stream_index");
					ret = avcodec_send_packet(decoder_ctx, &packet);
					ret = ret;
				}
				
				if (ret >= 0) 
				{
					ret = decode_frame(curPos);
				}

				av_packet_unref(&packet);

				if (ret == 1)
				{
					break;
				}
				else
				{
					need_to_read_packet = true;
				}
			}
			num_tries++;
		} while (((curPos == prevPos) || (ret != 1)) && (num_tries < 100));

		if (ret == 1)
		{
			m_Pos = curPos;
			m_ImageGeted = true;

			if (m_show_video)
			{
				((wxWindow*)m_pVideoWindow)->Refresh(true);
			}
		}
		else
		{
			m_Pos = m_Duration;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

s64 FFMPEGVideo::OneStepWithTimeout()
{
	OneStep();
	return GetPos();
}

int FFMPEGVideo::init_filters()
{
	char args[512];
	int ret = 0;
	const AVFilter* buffersrc = avfilter_get_by_name("buffer");
	const AVFilter* buffersink = avfilter_get_by_name("buffersink");
	AVFilterInOut* outputs = avfilter_inout_alloc();
	AVFilterInOut* inputs = avfilter_inout_alloc();
	AVRational time_base = video->time_base;
	enum AVPixelFormat pix_fmts[] = { dest_fmt, AV_PIX_FMT_NONE };

	filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}

	/* buffer video source: the decoded frames from the decoder will be inserted here. */
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt,
		time_base.num, time_base.den,
		decoder_ctx->sample_aspect_ratio.num, decoder_ctx->sample_aspect_ratio.den);

	ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		args, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
		goto end;
	}

	/* buffer video sink: to terminate the filter chain. */
	ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		NULL, NULL, filter_graph);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
		goto end;
	}

	ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts,
		AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		goto end;
	}

	/*
	 * Set the endpoints for the filter graph. The filter_graph will
	 * be linked to the graph described by filters_descr.
	 */

	 /*
	  * The buffer source output must be connected to the input pad of
	  * the first filter described by filters_descr; since the first
	  * filter input label is not specified, it is set to "in" by
	  * default.
	  */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	/*
	 * The buffer sink input must be connected to the output pad of
	 * the last filter described by filters_descr; since the last
	 * filter output label is not specified, it is set to "out" by
	 * default.
	 */
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if ((ret = avfilter_graph_parse_ptr(filter_graph, g_filter_descr.ToUTF8(),
		&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}

bool FFMPEGVideo::OpenMovie(wxString csMovieName, void *pVideoWindow, int device_type)
{
	bool res = false;
	int ret;
	enum AVHWDeviceType type;
	int i;

	m_start_pts = 0;
	m_frame_buffer_size = -1;
	m_origWidth = 0;
	m_origHeight = 0;
	m_Width = 0;
	m_Height = 0;

	m_dt_search = 1000;
	m_dt = 0;

	if (input_ctx) {
		CloseMovie();
	}	

	SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): starting...\n"));

	if (g_hw_device == wxT("cpu"))
	{	
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): starting cpu device open...\n"));

		g_use_hw_acceleration = false;

		if ((ret = avformat_open_input(&input_ctx, csMovieName.ToUTF8(), NULL, NULL)) < 0) {
			wxString msg = wxT("Cannot open input file: ") + csMovieName;
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): avformat_open_input \"%s\" passed\n"), csMovieName));

		if ((ret = avformat_find_stream_info(input_ctx, NULL)) < 0) {
			wxString msg = wxT("Cannot find input stream information.");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avformat_find_stream_info passed\n"));

		// select the video stream
		ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
		if (ret < 0) {
			wxString msg = wxT("Cannot find a video stream in the input file.");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		video_stream = ret;
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): av_find_best_stream passed\nvideo_stream: #%u\n"), video_stream));

		// create decoding context
		if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
		{
			wxString msg = wxT("avcodec_alloc_context3 faled");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_alloc_context3 passed\n"));

		video = input_ctx->streams[video_stream];

		if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
		{
			wxString msg = wxT("avcodec_parameters_to_context(decoder_ctx, video->codecpar) faled");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_parameters_to_context passed\n"));

		// init the video decoder
		if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
			wxString msg;
			msg.Printf(wxT("Failed to open codec for stream #%u"), video_stream);
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_open2 passed\n"));
	}
	else
	{
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): starting hw acceleration open...\n"));

		g_use_hw_acceleration = true;

		type = av_hwdevice_find_type_by_name(g_hw_device.ToUTF8());
		if (type == AV_HWDEVICE_TYPE_NONE)
		{
			wxString msg;
			msg = "Device type is not supported.\n";
			msg += "Available device types: ";
			type = AV_HWDEVICE_TYPE_NONE;
			while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
				msg += wxString(av_hwdevice_get_type_name(type)) + " ";
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): av_hwdevice_find_type_by_name passed\n"));

		// open the input file
		if (avformat_open_input(&input_ctx, csMovieName.ToUTF8(), NULL, NULL) != 0) {
			wxString msg = wxT("Cannot open input file: ") + csMovieName;
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): avformat_open_input \"%s\" passed\n"), csMovieName));

		if (avformat_find_stream_info(input_ctx, NULL) < 0) {
			wxString msg = wxT("Cannot find input stream information.");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}

		// find the video stream information
		ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
		if (ret < 0) {
			wxString msg = wxT("Cannot find a video stream in the input file.");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		video_stream = ret;
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): av_find_best_stream passed\nvideo_stream: #%u\n"), video_stream));

		hw_pix_fmt = AV_PIX_FMT_NONE;

		for (i = 0;; i++) {
			const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
			if (!config) {
				wxString msg;
				msg.Printf(wxT("Decoder %s does not support device type %s."),
					wxString(decoder->name), wxString(av_hwdevice_get_type_name(type)));
				SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
				wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
				CloseMovie();
				return false;
			}
			if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
				config->device_type == type) {
				hw_pix_fmt = config->pix_fmt;
				break;
			}
		}

		if (hw_pix_fmt == AV_PIX_FMT_NONE) {
			wxString msg;
			msg.Printf(wxT("Failed to find decoder HW config with support of device type %s for current video."), wxString(av_hwdevice_get_type_name(type)));
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): hw_pix_fmt was found\n"));

		if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
		{
			wxString msg;
			msg.Printf(wxT("avcodec_alloc_context3 faled with device type %s"), wxString(av_hwdevice_get_type_name(type)));
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_alloc_context3 passed\n"));

		video = input_ctx->streams[video_stream];
		if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
		{
			wxString msg;
			msg.Printf(wxT("avcodec_parameters_to_context(decoder_ctx, video->codecpar) faled with device type %s"), wxString(av_hwdevice_get_type_name(type)));
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_parameters_to_context passed\n"));

		decoder_ctx->get_format = get_hw_format;

		if (hw_decoder_init(decoder_ctx, type) < 0)
		{
			wxString msg;
			msg.Printf(wxT("hw_decoder_init(decoder_ctx, type) faled with device type %s"), wxString(av_hwdevice_get_type_name(type)));
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): hw_decoder_init passed\n"));

		if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
			wxString msg;
			msg.Printf(wxT("Failed to open codec for stream #%u with device type %s"), video_stream, wxString(av_hwdevice_get_type_name(type)));
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): avcodec_open2 passed\n"));
	}

	if (g_filter_descr.Len() > 0)
	{
		g_use_filter = true;
		if ((ret = init_filters()) < 0) {
			wxString msg;
			msg.Printf(wxT("init_filters \"%s\" failed"), g_filter_descr);
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): init_filters passed\n"));
	}
	else
	{
		g_use_filter = false;
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): no using filters\n"));
	}

	if (!(frame = av_frame_alloc())) {
		wxString msg = wxT("Can not alloc frame");
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
		wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
		CloseMovie();
		return false;
	}
	SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): frame = av_frame_alloc() passed\n"));

	if (g_use_hw_acceleration) {
		if (!(sw_frame = av_frame_alloc())) {
			wxString msg = wxT("Can not alloc sw_frame");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): sw_frame = av_frame_alloc() passed\n"));
	}

	if (g_use_filter) {
		if (!(filt_frame = av_frame_alloc())) {
			wxString msg = wxT("Can not alloc filt_frame");
			SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
			CloseMovie();
			return false;
		}
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): filt_frame = av_frame_alloc() passed\n"));
	}		

	if (video->duration > 0)
	{
		m_Duration = av_rescale(video->duration, video->time_base.num * 1000, video->time_base.den);
	}
	else
	{
		m_Duration = (input_ctx->duration * (s64)1000) / (s64)AV_TIME_BASE;
	}

	m_pVideoWindow = pVideoWindow;
	m_pVideoWindow ? m_show_video = true : m_show_video = false;

	m_Pos = -2;

	g_bln_get_hw_format = true;
	OneStep();

	if (!g_bln_get_hw_format) {
		wxString msg;
		msg.Printf(wxT("Failed to get HW surface format with device type %s"), wxString(av_hwdevice_get_type_name(type)));
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
		wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
		CloseMovie();
		return false;
	}
	SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): get_hw_format passed\n"));

	if (m_ImageGeted) {
		res = true;
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): m_ImageGeted true\n"));
	}
	else {
		SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): ERROR: m_ImageGeted false\n"));
		CloseMovie();
		return false;
	}

	if ((m_origWidth == 0) || (m_origHeight == 0))
	{
		wxString msg = wxString::Format(wxT("ERROR: Video \"%s\" has wrong frame sizes: FRAME_WIDTH: %d, FRAME_HEIGHT: %d\n"), csMovieName, m_origWidth, m_origHeight);
		SaveToReportLog(wxString::Format(wxT("FFMPEGVideo::OpenMovie(): %s\n"), msg));
		wxMessageBox(msg, wxT("FFMPEGVideo::OpenMovie"));
		CloseMovie();
		return false;
	}

	if ((video->avg_frame_rate.num > 0) && (video->avg_frame_rate.den > 0))
	{
		m_fps = (double)video->avg_frame_rate.num / (double)video->avg_frame_rate.den;
		m_dt = 1000 / m_fps;
	}
	else
	{
		s64 cur_pos = m_Pos;
		OneStep();
		m_dt = m_Pos - cur_pos;

		if (m_dt > 0)
		{
			m_fps = 1000.0 / (double)m_dt;
		}
		else
		{
			SaveToReportLog(wxT("FFMPEGVideo::OpenMovie(): ERROR: Failed to get right time for 2-d frame\n"));
			CloseMovie();
			return false;
		}
	}

	m_MovieName = csMovieName;

	return res;
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::CloseMovie()
{
	if (input_ctx && m_play_video)
	{
		Pause();
	}

	if (filter_graph) avfilter_graph_free(&filter_graph);
	if (decoder_ctx) avcodec_free_context(&decoder_ctx);
	if (input_ctx) avformat_close_input(&input_ctx);
	if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);	
	if (frame) av_frame_free(&frame);
	if (sw_frame) av_frame_free(&sw_frame);
	if (filt_frame) av_frame_free(&filt_frame);	
	av_packet_unref(&packet);

	filter_graph = NULL;
	decoder_ctx = NULL;
	input_ctx = NULL;
	hw_device_ctx = NULL;
	frame = NULL;
	sw_frame = NULL;
	filt_frame = NULL;

	m_start_pts = 0;
	m_frame_buffer_size = -1;
	m_origWidth = 0;
	m_origHeight = 0;
	m_Width = 0;
	m_Height = 0;

	m_play_video = false;

	return true;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetPos(s64 Pos)
{
	if (input_ctx)
	{
		if (m_play_video)
		{
			Pause();
		}
		
		s64 setPos, minPos, maxPos;
		int num_tries, res;
		int64_t min_ts, ts, max_ts;		

		num_tries = 0;
		do
		{
			setPos = Pos - m_dt_search;
			minPos = Pos - m_dt_search*10;
			maxPos = Pos - m_dt;

			min_ts = std::max<int64_t>(0, av_rescale(minPos, video->time_base.den, video->time_base.num * 1000) + m_start_pts);
			ts = std::max<int64_t>(0, av_rescale(setPos, video->time_base.den, video->time_base.num * 1000) + m_start_pts);
			max_ts = std::max<int64_t>(0, av_rescale(maxPos, video->time_base.den, video->time_base.num * 1000) + m_start_pts);

			do
			{


				res = avformat_seek_file(input_ctx, video_stream, min_ts, ts, max_ts, AVSEEK_FLAG_FRAME);
				need_to_read_packet = true;
				OneStep();
				num_tries++;
			} while (((m_Pos > Pos) || (Pos < minPos)) && (num_tries < 10));

			if ((m_Pos > Pos) || (Pos < minPos))
			{
				if (m_dt_search < 60000)
				{
					m_dt_search += 1000;
				}
			}
		} while (((m_Pos > Pos) || (Pos < minPos)) && (num_tries < 100));

		while (m_Pos < maxPos)
		{
			OneStep();
		}

		if (m_Pos < Pos - (9*m_dt / 10))
		{
			OneStep();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::SetVideoWindowPlacement(void *pVideoWindow)
{
	m_pVideoWindow = pVideoWindow;
	m_pVideoWindow ? m_show_video = true : m_show_video = false;
	if (m_show_video)
	{
		((wxWindow*)m_pVideoWindow)->Refresh(true);
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::SetNullRender()
{	
	m_show_video = false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetPos(double pos)
{
	SetPos((s64)(pos*1000.0));
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetPosFast(s64 Pos)
{
	SetPos(Pos);
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::ErrorMessage(wxString str)
{
	wxMessageBox(str, "ERROR MESSAGE");
}

/////////////////////////////////////////////////////////////////////////////

s64 FFMPEGVideo::GetPos()
{
	s64 pos = 0;
	
	if (input_ctx)
	{
		pos = m_Pos;
		if (pos < 0)
		{
			pos = 0;
		}
	}

    return pos;
}

/////////////////////////////////////////////////////////////////////////////

inline int FFMPEGVideo::convert_to_dst_format(u8* frame_data, uint8_t* const dst_data[], const int dst_linesize[], AVPixelFormat dest_fmt)
{
	int ret;
	uint8_t* src_data[4] = { NULL };
	int src_linesize[4];

	// Doesn't need memory free according av_image_fill_arrays and av_image_fill_pointers implementation
	// it use pointers from src pointer (in our case from frame_data)
	// https://ffmpeg.org/doxygen/trunk/imgutils_8c_source.html
	ret = av_image_fill_arrays(src_data, src_linesize, frame_data, src_fmt, m_origWidth, m_origHeight, 1);

	if (ret >= 0)
	{
		if ((src_fmt == AV_PIX_FMT_NV12) && g_use_cuda_gpu)
		{
			ret = 0;

#ifdef USE_CUDA
			ret = NV12_to_BGR(src_data[0], src_data[1], src_linesize[0],
				dst_data[0], m_Width, m_Height, m_origWidth, m_origHeight);

			if (ret == 0)
			{
				wxMessageBox("ERROR: CUDA NV12_to_BGRA failed", "FFMPEGVideo::convert_to_dst_format");
			}
#else
			wxMessageBox("ERROR: CUDA NV12_to_BGRA conversion is not supported on x86", "FFMPEGVideo::convert_to_dst_format");
#endif
		}
		else
		{
			// convert to destination format

			SwsContext* sws_ctx = sws_getContext(
				m_origWidth,
				m_origHeight,
				src_fmt,
				m_Width,
				m_Height,
				dest_fmt,
				SWS_BILINEAR,
				NULL,
				NULL,
				NULL
			);			


			if (sws_ctx)
			{
				ret = sws_scale(sws_ctx, src_data, src_linesize, 0, m_origHeight, dst_data, dst_linesize);
				sws_freeContext(sws_ctx);
			}
		}
	}

	return ret;
}

/////////////////////////////////////////////////////////////////////////////

int FFMPEGVideo::ConvertToBGR(u8* frame_data, simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
{
	int ret;
	AVPixelFormat dest_fmt = AV_PIX_FMT_BGR24;
	uint8_t* dst_data[4] = { NULL };
	int dst_linesize[4];

	/* buffer is going to be written to rawvideo file, no alignment */
	if ((ret = av_image_alloc(dst_data, dst_linesize,
		m_Width, m_Height, dest_fmt, 1)) < 0) {
		custom_assert(ret > 0, "FFMPEGVideo::ConvertToBGR\nCould not allocate memory for destination image.");
	}

	if (ret > 0)
	{
		ret = convert_to_dst_format(frame_data, dst_data, dst_linesize, dest_fmt);
	}

	if (ret > 0)
	{
		int w, h, x, y, i, j, di;		

		w = xmax - xmin + 1;
		h = ymax - ymin + 1;

		di = m_Width - w;

		i = ymin * m_Width + xmin;
		j = 0;

		custom_assert(ImBGR.m_size >= w * h * 3, "FFMPEGVideo::ConvertToBGR not: ImBGR.m_size >= w * h * 3");


		if (w == m_Width)
		{
			ImBGR.copy_data(dst_data[0], j * 3, i * 3, w * h * 3);
		}
		else
		{
			for (y = 0; y < h; y++)
			{
				ImBGR.copy_data(dst_data[0], j * 3, i * 3, w * 3);
				j += w;
				i += m_Width;
			}
		}

		UpdateImageColor(ImBGR, w, h);
	}

	if (dst_data[0]) av_freep(&dst_data[0]);

	return ret;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::GetBGRImage(simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
{
	if (input_ctx)
	{
		ConvertToBGR(m_frame_buffer.m_pData, ImBGR, xmin, xmax, ymin, ymax);
	}
}

/////////////////////////////////////////////////////////////////////////////

int FFMPEGVideo::GetFrameDataSize()
{
	return m_frame_buffer_size;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::GetFrameData(simple_buffer<u8>& FrameData)
{
	custom_assert(FrameData.size() == m_frame_buffer_size, "void FFMPEGVideo::GetFrameData(simple_buffer<u8>& FrameData)\nnot: FrameData.size() == m_frame_buffer_size");
	FrameData.copy_data(m_frame_buffer, m_frame_buffer_size);
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetImageGeted(bool ImageGeted)
{
	m_ImageGeted = ImageGeted;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::RunWithTimeout(s64 timeout)
{
	if (input_ctx)
	{
		Run();
		wxMilliSleep(timeout);
		Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::Run()
{
	if (input_ctx)
	{
		if (m_play_video)
		{
			Pause();
		}

		if (m_ImageGeted == false)
		{
			OneStep();
		}
		else
		{
			m_play_video = true;			

			m_pThreadRunVideo = new FFMPEGThreadRunVideo(this);
			m_pThreadRunVideo->Create();
			m_pThreadRunVideo->Run();
			//m_pThreadRunVideo->SetPriority(30); //THREAD_PRIORITY_BELOW_NORMAL
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::Pause()
{
	if (input_ctx && m_play_video)
	{
		m_play_video = false;
		while (!m_pThreadRunVideo->IsDetached()) { wxMilliSleep(30); }
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::Stop()
{
	Pause();
	SetPos((s64)0);
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::StopFast()
{
	Stop();
}


/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::WaitForCompletion(s64 timeout)
{
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetVideoWindowPosition(int left, int top, int width, int height, void *dc)
{
	ShowFrame(dc);
}

FFMPEGThreadRunVideo::FFMPEGThreadRunVideo(FFMPEGVideo *pVideo) : wxThread()
{
	m_pVideo = pVideo;
}

void *FFMPEGThreadRunVideo::Entry()
{
	while (m_pVideo->m_play_video)
	{		
		std::chrono::time_point<std::chrono::high_resolution_clock> start_t = std::chrono::high_resolution_clock::now();
		m_pVideo->OneStep();
		if (!m_pVideo->m_ImageGeted)
		{
			m_pVideo->m_play_video = false;
			break;
		}
		int dt = (int)(1000.0 / m_pVideo->m_fps) - (int)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_t).count());
		if (dt > 0) wxMilliSleep(dt);
	}

	return 0;
}