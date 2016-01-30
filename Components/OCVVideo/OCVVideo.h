                              //OCVVideo.h//                                
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

class OCVVideo;

/////////////////////////////////////////////////////////////////////////////

class ThreadRunVideo : public wxThread
{
public:
	ThreadRunVideo(OCVVideo *pVideo);

	virtual void *Entry();

public:
	OCVVideo	*m_pVideo;
};

/////////////////////////////////////////////////////////////////////////////

class OCVVideo: public CVideo
{
public:
	OCVVideo();
	~OCVVideo();
	
public:		
	bool			m_IsSetNullRender;

	cv::VideoCapture m_VC;

    int     *m_pBuffer;
    int     m_BufferSize;
    bool    m_ImageGeted;
    s64     m_st;
	int		m_type; //video open type
	bool	m_show_video;
	bool	m_play_video;
	wxBitmap	*m_pBmp;	
	wxBitmap	*m_pBmpScaled;
	double m_frameNumbers;
	double m_fps;
	cv::Mat m_cur_frame;

	ThreadRunVideo *m_pThreadRunVideo;

public:
	void ShowFrame(cv::Mat &img, void *dc = NULL);

	bool OpenMovie(string csMovieName, void *pVideoWindow, int type);

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
	void GetRGBImage(custom_buffer<int> &ImRGB, int xmin, int xmax, int ymin, int ymax);

	void SetVideoWindowPosition(int left, int top, int width, int height, void *dc);

	void ErrorMessage(string str);
};
