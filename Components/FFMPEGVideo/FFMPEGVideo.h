                              //FFMPEGVideo.h//                                
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

#include "DataTypes.h"
#include "Video.h"
#include "opencv2/opencv.hpp"
#include <wx/wx.h>

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/hwcontext.h>
	#include <libavutil/opt.h>
	#include <libavutil/avassert.h>
	#include <libavutil/imgutils.h>
	#include <libswscale/swscale.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
}

class FFMPEGVideo;

/////////////////////////////////////////////////////////////////////////////

class FFMPEGThreadRunVideo : public wxThread
{
public:
	FFMPEGThreadRunVideo(FFMPEGVideo *pVideo);

	virtual void *Entry();

public:
	FFMPEGVideo	*m_pVideo;
};

/////////////////////////////////////////////////////////////////////////////

class FFMPEGVideo: public CVideo
{
public:
	FFMPEGVideo();
	~FFMPEGVideo();
	
public:		
	bool	m_IsSetNullRender;

    int     *m_pBuffer;
    int     m_BufferSize;
    bool    m_ImageGeted;
    s64     m_st;
	int		m_type; //video open type
	bool	m_show_video;	
	double		m_frameNumbers;
	double		m_fps;
	long		m_origWidth;
	long		m_origHeight;

	FFMPEGThreadRunVideo *m_pThreadRunVideo;
	
	int64_t m_start_pts = 0;

	AVFilterGraph *filter_graph = NULL;
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterContext *buffersrc_ctx = NULL;	

	AVFormatContext *input_ctx = NULL;
	AVBufferRef *hw_device_ctx = NULL;		
	AVCodecContext *decoder_ctx = NULL;		
	AVStream *video = NULL;
#if LIBAVFORMAT_VERSION_MAJOR > 58
	const AVCodec *decoder = NULL;
#else
	 AVCodec *decoder = NULL;
#endif
	AVFrame *frame = NULL;
	AVFrame *sw_frame = NULL;
	AVFrame* filt_frame = NULL;
	int video_stream;
	int64_t m_cur_pts;
	s64	m_dt_search;
	s64	m_dt;

	int m_frame_buffer_size = -1;
	simple_buffer<u8> m_frame_buffer;

	AVPacket packet;	

	bool need_to_read_packet = true;
	bool cuda_memory_is_initialized = false;

	AVPixelFormat src_fmt;

	//DWORD min_dt = 1000;
	//DWORD max_dt = 0;

public:
	void ShowFrame(void *dc = NULL);

	bool OpenMovie(wxString csMovieName, void *pVideoWindow, int type);

	bool SetVideoWindowPlacement(void *pVideoWindow);
	bool SetNullRender();

	bool CloseMovie();
	
	void SetPos(s64 Pos);
	void SetPos(double pos);
	void SetPosFast(s64 Pos);

	void SetImageGeted(bool ImageGeted);

	void Run();
	void Pause();

	void WaitForCompletion(s64 timeout);

	void StopFast();

	void RunWithTimeout(s64 timeout);

	void Stop();
    void OneStep();
	s64  OneStepWithTimeout();
	s64  GetPos();
	void GetBGRImage(simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax);
	int ConvertToBGR(u8* frame_data, simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax);
	inline int convert_to_dst_format(u8* frame_data, uint8_t* const dst_data[], const int dst_linesize[], AVPixelFormat dest_fmt);

	int GetFrameDataSize();
	void GetFrameData(simple_buffer<u8>& FrameData);

	void SetVideoWindowPosition(int left, int top, int width, int height, void *dc);

	void ErrorMessage(wxString str);

	int hw_decoder_init(AVCodecContext *ctx, const enum AVHWDeviceType type);
	int decode_frame(s64& frame_pos);
	int init_filters();
};
