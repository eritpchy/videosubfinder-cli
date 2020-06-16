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

wxString g_hw_device = "none";

wxArrayString GetAvailableHWDeviceTypes()
{
	wxArrayString res;
	enum AVHWDeviceType type;

	type = AV_HWDEVICE_TYPE_NONE;
	while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
	{
		res.Add(av_hwdevice_get_type_name(type));
	}
	if (res.size() == 0)
	{
		res.Add("none");
	}
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
	if (m_show_video)
	{
		convert_to_dst_format(m_frame_buffer.m_pData);

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

int FFMPEGVideo::decode_frame(s64 &frame_pos)
{
	AVFrame* cur_frame = NULL;
	int size;
	int ret = 0;	

	//sw_frame->format = AV_PIX_FMT_NV12;

	ret = avcodec_receive_frame(decoder_ctx, frame);
	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
		return 0;
	}
	else if (ret < 0) {
		fprintf(stderr, "Error while decoding\n");
		return ret;
	}

	frame_pos = av_rescale(frame->pts, video->time_base.num * 1000, video->time_base.den);

	if (frame->format == hw_pix_fmt) {
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
	
	if (m_frame_buffer_size == -1)
	{	
		src_fmt = (AVPixelFormat)cur_frame->format;
		m_frame_buffer_size = av_image_get_buffer_size(src_fmt, cur_frame->width, cur_frame->height, 1);
		m_frame_buffer.set_size(m_frame_buffer_size);
	}
	
	assert(m_origWidth == cur_frame->width, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: m_origWidth == cur_frame->width");
	assert(m_origHeight == cur_frame->height, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: m_origHeight == cur_frame->height");
	assert(src_fmt == (AVPixelFormat)cur_frame->format, "int FFMPEGVideo::decode_frame(s64 &frame_pos)\nnot: src_fmt == (AVPixelFormat)cur_frame->format");

	av_image_copy_to_buffer(&m_frame_buffer[0], m_frame_buffer_size, cur_frame->data, cur_frame->linesize, src_fmt, cur_frame->width, cur_frame->height, 1);
	
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
						break;
					}

					ret = avcodec_send_packet(decoder_ctx, &packet);
					ret = ret;
				}
				
				if (ret >= 0) 
				{
					ret = decode_frame(curPos);
				}

				if (ret == 1)
				{
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
			m_Pos = curPos;
			m_ImageGeted = true;

			if (m_show_video)
			{
				ShowFrame();
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

bool FFMPEGVideo::OpenMovie(wxString csMovieName, void *pVideoWindow, int device_type)
{
	bool res = false;
	int ret;
	enum AVHWDeviceType type;
	int i;

	m_frame_buffer_size = -1;

	if (input_ctx) {
		CloseMovie();
	}

	if (g_hw_device == "none")
	{
		wxMessageBox("No one FFMPEG HW Device found", "FFMPEGVideo::OpenMovie");
		return false;
	}

	type = av_hwdevice_find_type_by_name(g_hw_device.ToUTF8());
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

	OneStep();

	if (m_ImageGeted) {
		res = true;
	}
	else {
		CloseMovie();
		return false;
	}

	m_fps = video->avg_frame_rate.num / (double)video->avg_frame_rate.den;

	return res;
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::CloseMovie()
{
	if (input_ctx && m_play_video)
	{
		Pause();
	}

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

#ifdef WIN64
	if (cuda_memory_is_initialized)
	{
		release_cuda_memory();
		cuda_memory_is_initialized = false;
	}
#endif

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

		int num_tries = 0, res;
		int64_t min_ts, ts, max_ts;
		s64 dPos;
		min_ts = std::max<int64_t>(0, tm - 100000);
		ts = std::max<int64_t>(0, tm);
		max_ts = std::max<int64_t>(0, tm);

		do
		{
			res = avformat_seek_file(input_ctx, video_stream, min_ts, ts, max_ts, AVSEEK_FLAG_FRAME);
			if (!need_to_read_packet)
			{
				av_packet_unref(&packet);
			}
			need_to_read_packet = true;
			OneStep();
			num_tries++;

			dPos = std::abs(Pos - m_Pos);
		}
		while ((dPos > 5000) && (num_tries < 10));

		num_tries = num_tries;

		/*if (num_tries > 1)
		{
			wxMessageBox("num_tries: " + std::to_string(num_tries), "FFMPEGVideo::SetPos");
		}*/
	}
}

/////////////////////////////////////////////////////////////////////////////

bool FFMPEGVideo::SetVideoWindowPlacement(void *pVideoWindow)
{
	m_pVideoWindow = pVideoWindow;
	m_show_video = true;
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

inline int FFMPEGVideo::convert_to_dst_format(u8* frame_data)
{
	int ret;

	// Doesn't need memory free according av_image_fill_arrays and av_image_fill_pointers implementation
	// it use pointers from src pointer (in our case from frame_data)
	// https://ffmpeg.org/doxygen/trunk/imgutils_8c_source.html
	ret = av_image_fill_arrays(src_data, src_linesize, frame_data, src_fmt, m_origWidth, m_origHeight, 1);

	if (ret >= 0)
	{
		if ((src_fmt == AV_PIX_FMT_NV12) && g_use_cuda_gpu)
		{
			ret = 0;

#ifdef WIN64
			if (!cuda_memory_is_initialized)
			{
				init_cuda_memory(m_Width, m_Height, m_origWidth, m_origHeight);
				cuda_memory_is_initialized = true;
			}

			ret = NV12_to_BGRA(src_data[0], src_data[1], src_linesize[0],
				dst_data[0], m_Width, m_Height, m_origWidth, m_origHeight);
#endif
			if (ret == 0)
			{
				wxMessageBox("ERROR: CUDA NV12_to_BGRA failed", "FFMPEGVideo::decode_frame");
			}
		}
		else
		{
			// convert to destination format

			if (sws_ctx == NULL) {
				sws_ctx = sws_getContext(
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
			}

			ret = sws_scale(sws_ctx, src_data, src_linesize, 0, m_origHeight, dst_data, dst_linesize);
		}
	}

	return ret;
}

void FFMPEGVideo::ConvertToRGB(u8* frame_data, custom_buffer<int>& ImRGB, int xmin, int xmax, int ymin, int ymax)
{
	int ret = convert_to_dst_format(frame_data);

	int w, h, x, y, i, j, di;
	u8* color;

	w = xmax - xmin + 1;
	h = ymax - ymin + 1;

	di = m_Width - w;

	i = ymin * m_Width + xmin;
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

/////////////////////////////////////////////////////////////////////////////
// ImRGB in format b:g:r:0
void FFMPEGVideo::GetRGBImage(custom_buffer<int>& ImRGB, int xmin, int xmax, int ymin, int ymax)
{
	if (input_ctx)
	{
		ConvertToRGB(m_frame_buffer.m_pData, ImRGB, xmin, xmax, ymin, ymax);
	}
}

/////////////////////////////////////////////////////////////////////////////

int FFMPEGVideo::GetFrameDataSize()
{
	return m_frame_buffer_size;
}

/////////////////////////////////////////////////////////////////////////////

void FFMPEGVideo::GetFrameData(custom_buffer<u8>& FrameData)
{
	custom_assert(FrameData.size() >= m_frame_buffer_size, "void FFMPEGVideo::GetFrameData(custom_buffer<u8>& FrameData)\nnot: FrameData.size() >= m_frame_buffer_size");
	memcpy(&FrameData[0], &m_frame_buffer[0], m_frame_buffer_size);
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
		clock_t start_t = clock();
		m_pVideo->OneStep();
		if (!m_pVideo->m_ImageGeted)
		{
			break;
		}
		m_pVideo->ShowFrame();
		int dt = (int)(1000.0 / m_pVideo->m_fps) - (int)(clock() - start_t);
		if (dt > 0) wxMilliSleep(dt);
	}

	return 0;
}