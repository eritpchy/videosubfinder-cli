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

#ifdef WIN64
#include "cuda_kernels.h"
#endif

/////////////////////////////////////////////////////////////////////////////

FFMPEGVideo::FFMPEGVideo()
{
	m_Inited=false;
	m_IsSetNullRender = false;
    
    m_pBuffer=NULL;
	m_st = 0;

	m_type = 1;

	m_pBmp = NULL;
	m_pBmpScaled = NULL;

	m_pVideoWindow = NULL;

	m_pThreadRunVideo = NULL;

	m_play_video = false;
}

/////////////////////////////////////////////////////////////////////////////

FFMPEGVideo::~FFMPEGVideo()
{
	if (m_VC.isOpened() && m_play_video)
	{
		Pause();
		m_VC.release();
	}

	if (m_pBmp != NULL)
	{
		delete m_pBmp;
		m_pBmp = NULL;
	}

	if (m_pBmpScaled != NULL)
	{
		delete m_pBmpScaled;
		m_pBmpScaled = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::ShowFrame(void *dc)
{
	if (m_show_video)
	{
		int wnd_w, wnd_h, img_w = m_Width, img_h = m_Height, num_pixels = img_w*img_h;
		((wxWindow*)m_pVideoWindow)->GetClientSize(&wnd_w, &wnd_h);

		if ((wnd_w > 0) && (wnd_h > 0) && (img_w > 0) && (img_h > 0))
		{
			unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

			for (int i = 0; i < num_pixels; i++)
			{
				img_data[i * 3] = dst_data[0][i * 4 + 2]; //R
				img_data[i * 3 + 1] = dst_data[0][i * 4 + 1]; //G
				img_data[i * 3 + 2] = dst_data[0][i * 4]; //B
			}

			if (dc != NULL)
			{
				((wxPaintDC*)dc)->DrawBitmap(wxImage(img_w, img_h, img_data).Scale(wnd_w, wnd_h), 0, 0);
			}
			else
			{
				wxClientDC cdc((wxWindow*)m_pVideoWindow);
				cdc.DrawBitmap(wxImage(img_w, img_h, img_data).Scale(wnd_w, wnd_h), 0, 0);
			}
		}
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

	wxMessageBox("Failed to get HW surface format.", "FFMPEGVideo::get_hw_format");
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

int FFMPEGVideo::decode_frame()
{
	AVFrame *tmp_frame = NULL;
	int size;
	int ret = 0;	

	sw_frame->format = AV_PIX_FMT_NV21;

	ret = avcodec_receive_frame(decoder_ctx, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return 0;
	}
	else if (ret < 0) {
		fprintf(stderr, "Error while decoding\n");
		return ret;
	}

	if (frame->format == hw_pix_fmt) {
		/* retrieve data from GPU to CPU */			
		if ((ret = av_hwframe_transfer_data(sw_frame, frame, 0)) < 0) {
			fprintf(stderr, "Error transferring the data to system memory\n");
			av_frame_unref(frame);
			return ret;
		}
		tmp_frame = sw_frame;
	}
	else
		tmp_frame = frame;

	if (sws_ctx == NULL) {
		sws_ctx = sws_getContext(
			tmp_frame->width,
			tmp_frame->height,
			(AVPixelFormat)tmp_frame->format,
			m_Width,
			m_Height,
			dest_fmt,
			SWS_BILINEAR,
			NULL,
			NULL,
			NULL
		);
	}		

	m_Pos = av_rescale(frame->pts, video->time_base.num * 1000, video->time_base.den);
	if (m_Pos <= 0)
	{
		m_Pos = m_Pos;
	}

	/*if ((tmp_frame->format == AV_PIX_FMT_NV21) && g_use_cuda_gpu)
	{
		ret = -1;
#ifdef WIN64
		ret = NV21_to_BGRA(tmp_frame->data[0], tmp_frame->data[1], tmp_frame->linesize[0],
			dst_data[0], tmp_frame->width, tmp_frame->height);
#endif
		if (ret != 0)
		{
			ret = sws_scale(sws_ctx, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height, dst_data, dst_linesize);
		}

		cv::Mat in_mat_y(m_origHeight, m_origWidth, CV_8U);
		cv::Mat in_mat_uv(m_origHeight/2, m_origWidth/2, CV_8UC2);
		cv::Mat mat_res;

		//custom_assert(m_origWidth == tmp_frame->linesize[0], "m_origWidth == tmp_frame->linesize[0]");
		//custom_assert((m_origWidth / 2) * 2 == tmp_frame->linesize[1], "m_origWidth == tmp_frame->linesize[1]");
		//custom_assert(m_Width * 4 == dst_linesize[0], "m_Width * 4 == dst_linesize[0]");

		memcpy(in_mat_y.data, tmp_frame->data[0], m_origHeight * m_origWidth);
		memcpy(in_mat_uv.data, tmp_frame->data[1], (m_origHeight/2) * (m_origWidth/2) * 2);

		//cv::UMat in_umat_y;
		//cv::UMat in_umat_uv;
		//cv::UMat umat_res;

		//in_mat_y.copyTo(in_umat_y);
		//in_mat_uv.copyTo(in_umat_uv);

		cv::cvtColorTwoPlane(in_mat_y, in_mat_uv, mat_res, cv::COLOR_YUV2BGRA_NV12);
		
		if ((m_origWidth != m_Width) || (m_origHeight != m_Height))
		{
			cv::resize(mat_res, mat_res, cv::Size(m_Width, m_Height));
		}

		//umat_res.copyTo(mat_res);

		memcpy(dst_data[0], mat_res.data, m_Height*m_Width * 4);
	}
	else*/
	{
		/* convert to destination format */
		ret = sws_scale(sws_ctx, tmp_frame->data, tmp_frame->linesize, 0, tmp_frame->height, dst_data, dst_linesize);		
	}
	ret = 1;
	
	if (frame->format == hw_pix_fmt)
	{
		av_frame_unref(sw_frame);
	}
	av_frame_unref(frame);
	
	return ret;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::OneStep()
{
	m_ImageGeted = false;

	DWORD start_time = GetTickCount();

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
						break;
					}

					ret = avcodec_send_packet(decoder_ctx, &packet);
				}
				
				if (ret >= 0) 
				{
					ret = decode_frame();
				}

				if (ret == 1)
				{
					curPos = m_Pos;
					break;
				}
				else
				{
					need_to_read_packet = true;
					av_packet_unref(&packet);
				}
			}
			num_tries++;
		} while (((curPos == prevPos) || (ret != 1)) && (num_tries < 100));

		if (ret == 1)
		{
			m_ImageGeted = true;
			ShowFrame();
		}
	}

	DWORD dt = GetTickCount() - start_time;

	if (dt < min_dt)
	{
		min_dt = dt;
	}

	if (dt > max_dt)
	{
		max_dt = dt;
	}
}

/////////////////////////////////////////////////////////////////////////////

s64 FFMPEGVideo::OneStepWithTimeout()
{
	OneStep();
	return GetPos();
}

bool FFMPEGVideo::OpenMovie(wxString csMovieName, void *pVideoWindow, int device_type)
{
	bool res = false;
	int ret;
	enum AVHWDeviceType type;
	int i;

	if (input_ctx) {
		CloseMovie();
	}	

	//type = av_hwdevice_find_type_by_name("dxva2");
	type = av_hwdevice_find_type_by_name("cuda");
	if (type == AV_HWDEVICE_TYPE_NONE)
	{
		std::string msg;
		msg = "Device type is not supported.\n";
		msg += "Available device types: ";
		type = AV_HWDEVICE_TYPE_NONE;
		while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
			msg += std::string(av_hwdevice_get_type_name(type)) + " ";
		wxMessageBox(msg, "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	// open the input file
	if (avformat_open_input(&input_ctx, csMovieName.ToUTF8(), NULL, NULL) != 0) {
		wxMessageBox(std::string("Cannot open input file: ") + csMovieName.ToUTF8(), "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	if (avformat_find_stream_info(input_ctx, NULL) < 0) {
		wxMessageBox("Cannot find input stream information.", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	// find the video stream information
	ret = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
	if (ret < 0) {
		wxMessageBox("Cannot find a video stream in the input file.", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}
	video_stream = ret;

	for (i = 0;; i++) {
		const AVCodecHWConfig *config = avcodec_get_hw_config(decoder, i);
		if (!config) {
			char msg[500];
			sprintf(msg, "Decoder %s does not support device type %s.",
				decoder->name, av_hwdevice_get_type_name(type));
			wxMessageBox(msg, "FFMPEGVideo::OpenMovie");
			CloseMovie();
			return false;
		}
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			config->device_type == type) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}

	if (!(decoder_ctx = avcodec_alloc_context3(decoder)))
	{
		wxMessageBox("avcodec_alloc_context3 faled", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	video = input_ctx->streams[video_stream];
	if (avcodec_parameters_to_context(decoder_ctx, video->codecpar) < 0)
	{
		wxMessageBox("avcodec_parameters_to_context(decoder_ctx, video->codecpar) faled", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	decoder_ctx->get_format = get_hw_format;

	if (hw_decoder_init(decoder_ctx, type) < 0)
	{
		wxMessageBox("hw_decoder_init(decoder_ctx, type) faled", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	if ((ret = avcodec_open2(decoder_ctx, decoder, NULL)) < 0) {
		char msg[500];
		sprintf(msg, "Failed to open codec for stream #%u\n", video_stream);
		wxMessageBox(msg, "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}

	m_origWidth = video->codecpar->width;
	m_origHeight = video->codecpar->height;

	m_Width = m_origWidth;
	m_Height = m_origHeight;

	if (m_origWidth > 1280)
	{
		double zoum = (double)1280 / (double)m_origWidth;
		m_Width = 1280;
		m_Height = (double)m_origHeight*zoum;
	}

	/* buffer is going to be written to rawvideo file, no alignment */
	if ((ret = av_image_alloc(dst_data, dst_linesize,
		m_Width, m_Height, dest_fmt, 1)) < 0) {
		wxMessageBox("Could not allocate memory for destination image.", "FFMPEGVideo::OpenMovie");
		CloseMovie();
		return false;
	}
	dst_bufsize = ret;

	m_Duration = input_ctx->duration / (AV_TIME_BASE / 1000);

	m_pVideoWindow = pVideoWindow;
	m_pVideoWindow ? m_show_video = true : m_show_video = false;	

	if (!(frame = av_frame_alloc()) || !(sw_frame = av_frame_alloc())) {
		wxMessageBox("Can not alloc frame");
		CloseMovie();
		return false;
	}

	m_Pos = -2;

	tmp_buffer.set_size(m_origWidth*m_origHeight);

	OneStep();

	if (m_ImageGeted) {
		res = true;
	}
	else {
		CloseMovie();
	}		

	return res;
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::CloseMovie()
{
	if (decoder_ctx) avcodec_free_context(&decoder_ctx);
	if (input_ctx) avformat_close_input(&input_ctx);
	if (hw_device_ctx) av_buffer_unref(&hw_device_ctx);
	if (dst_data[0]) av_freep(&dst_data[0]);
	if (sws_ctx) sws_freeContext(sws_ctx);
	if (frame) av_frame_free(&frame);
	if (sw_frame) av_frame_free(&sw_frame);
	av_packet_unref(&packet);

	decoder_ctx = NULL;
	input_ctx = NULL;
	hw_device_ctx = NULL;
	dst_data[0] = NULL;
	sws_ctx = NULL;	
	frame = NULL;
	sw_frame = NULL;

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

		s64 tm = av_rescale(Pos, video->time_base.den, video->time_base.num * 1000);

		s64 CurPos = m_Pos;
		int num_tries = 0;

		do
		{
			avformat_seek_file(input_ctx, video_stream, INT64_MIN, tm, tm, AVSEEK_FLAG_FRAME);
			if (!need_to_read_packet)
			{
				av_packet_unref(&packet);
			}
			need_to_read_packet = true;
			OneStep();
			num_tries++;
		}
		while ((std::abs<s64>(CurPos - m_Pos) > 60000) && num_tries < 10);
	}
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::SetVideoWindowPlacement(void *pVideoWindow)
{
	m_pVideoWindow = pVideoWindow;
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

void FFMPEGVideo::ErrorMessage(string str)
{
	wxMessageBox(str.c_str(), "ERROR MESSAGE");
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
// ImRGB in format b:g:r:0
void FFMPEGVideo::GetRGBImage(custom_buffer<int> &ImRGB, int xmin, int xmax, int ymin, int ymax)
{
	if (input_ctx)
	{
		int w, h, x, y, i, j, di;
		u8 *color;

		w = xmax - xmin + 1;
		h = ymax - ymin + 1;

		di = m_Width - w;

		i = ymin*m_Width + xmin;
		j = 0;

		if (w == m_Width)
		{
			memcpy(&(ImRGB.m_pData[j]), &dst_data[0][i * 4], w * h * 4);
		}
		else
		{
			for (y = 0; y < h; y++)
			{
				memcpy(&(ImRGB.m_pData[j]), &dst_data[0][i * 4], w * 4);
				j += w;
				i += m_Width;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::SetImageGeted(bool ImageGeted)
{
	m_ImageGeted = m_ImageGeted;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::RunWithTimeout(s64 timeout)
{
	if (m_VC.isOpened())
	{
		Run();
		wxMilliSleep(timeout);
		Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::Run()
{
	if (m_VC.isOpened())
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
	if (m_VC.isOpened() && m_play_video)
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
		clock_t start_t = clock();
		m_pVideo->m_VC >> m_pVideo->m_cur_frame;
		if (m_pVideo->m_cur_frame.empty())
		{
			break;
		}
		if ((m_pVideo->m_Width != m_pVideo->m_origWidth) || (m_pVideo->m_Height != m_pVideo->m_origHeight)) cv::resize(m_pVideo->m_cur_frame, m_pVideo->m_cur_frame, cv::Size(m_pVideo->m_Width, m_pVideo->m_Height), 0, 0, cv::INTER_LINEAR);
		m_pVideo->ShowFrame();
		int dt = (int)(1000.0 / m_pVideo->m_fps) - (int)(clock() - start_t);
		if (dt > 0) wxMilliSleep(dt);
	}

	return 0;
}