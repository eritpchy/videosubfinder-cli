                              //OCVVideo.cpp//                                
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

#include "OCVVideo.h"
#include <mutex>

std::mutex ocv_mutex;

/////////////////////////////////////////////////////////////////////////////

OCVVideo::OCVVideo()
{
	m_Inited=false;
	m_IsSetNullRender = false;
    
    m_pBuffer=NULL;
	m_st = 0;

	m_type = 0;

	m_pBmp = NULL;
	m_pBmpScaled = NULL;

	m_pVideoWindow = NULL;

	m_pThreadRunVideo = NULL;

	m_play_video = false;
}

/////////////////////////////////////////////////////////////////////////////

OCVVideo::~OCVVideo()
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

void OCVVideo::ShowFrame(cv::Mat &img, void *dc)
{
	if (m_VC.isOpened() && m_show_video)
	{
		int wnd_w, wnd_h, img_w = img.cols, img_h = img.rows, num_pixels = img_w*img_h;
		((wxWindow*)m_pVideoWindow)->GetClientSize(&wnd_w, &wnd_h);

		if ((wnd_w > 0) && (wnd_h > 0) && (img_w > 0) && (img_h > 0))
		{
			unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

			for (int i = 0; i < num_pixels; i++)
			{
				img_data[i * 3] = img.data[i * 3 + 2];
				img_data[i * 3 + 1] = img.data[i * 3 + 1];
				img_data[i * 3 + 2] = img.data[i * 3];
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

bool OCVVideo::OpenMovie(wxString csMovieName, void *pVideoWindow, int type)
{ 
	cv::String movie_name(csMovieName.ToUTF8());
	m_VC.open(movie_name);
	bool res = m_VC.isOpened();

	if (res)
	{
		m_MovieName = csMovieName;

		m_origWidth = m_VC.get(cv::CAP_PROP_FRAME_WIDTH);
		m_origHeight = m_VC.get(cv::CAP_PROP_FRAME_HEIGHT);

		m_Width = m_origWidth;
		m_Height = m_origHeight;

		if (m_origWidth > 1280)
		{
			double zoum = (double)1280 / (double)m_origWidth;
			m_Width = 1280;
			m_Height = (double)m_origHeight*zoum;
		}

		m_pVideoWindow = pVideoWindow;
		m_pVideoWindow ? m_show_video = true : m_show_video = false;

		m_VC >> m_cur_frame;
		if ((m_Width != m_origWidth) || (m_Height != m_origHeight)) cv::resize(m_cur_frame, m_cur_frame, cv::Size(m_Width, m_Height), 0, 0, cv::INTER_LINEAR);

		m_frameNumbers = m_VC.get(cv::CAP_PROP_FRAME_COUNT);
		m_fps = m_VC.get(cv::CAP_PROP_FPS);
		m_Duration = ((m_frameNumbers * 1000.0) / m_fps);

		m_ImageGeted = true;

		ShowFrame(m_cur_frame);		
	}

	return res;
}

/////////////////////////////////////////////////////////////////////////////

bool OCVVideo::SetVideoWindowPlacement(void *pVideoWindow)
{	
	m_pVideoWindow = pVideoWindow;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool OCVVideo::CloseMovie()
{
	if (m_VC.isOpened())
	{
		if (m_play_video)
		{
			Pause();
		}
	}

	m_VC.release();
	m_play_video = false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

bool OCVVideo::SetNullRender()
{	
	m_show_video = false;
	return true;
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::SetPos(s64 Pos)
{
	if (m_VC.isOpened())
	{
		if (m_play_video)
		{
			Pause();
		}
		m_VC.set(cv::CAP_PROP_POS_MSEC, Pos);
		if (Pos == 0) m_VC >> m_cur_frame;
		else m_VC.retrieve(m_cur_frame);
		if ((m_Width != m_origWidth) || (m_Height != m_origHeight)) cv::resize(m_cur_frame, m_cur_frame, cv::Size(m_Width, m_Height), 0, 0, cv::INTER_LINEAR);
		m_ImageGeted = true;
		ShowFrame(m_cur_frame);
	}
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::SetPos(double pos)
{
	SetPos((s64)(pos*1000.0));
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::SetPosFast(s64 Pos)
{
	SetPos(Pos);
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::OneStep()
{
	if (m_VC.isOpened())
	{
		if (m_play_video)
		{
			Pause();
		}

		m_VC >> m_cur_frame;
		if ((m_Width != m_origWidth) || (m_Height != m_origHeight)) cv::resize(m_cur_frame, m_cur_frame, cv::Size(m_Width, m_Height), 0, 0, cv::INTER_LINEAR);
		m_ImageGeted = true;
		ShowFrame(m_cur_frame);
	}
}

/////////////////////////////////////////////////////////////////////////////

s64 OCVVideo::OneStepWithTimeout()
{
	OneStep();
	return GetPos();
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::ErrorMessage(string str)
{
	wxMessageBox(str.c_str(), "ERROR MESSAGE");
}

/////////////////////////////////////////////////////////////////////////////

s64 OCVVideo::GetPos()
{
	s64 pos = -1;
	
	if (m_VC.isOpened())
	{
		pos = m_VC.get(cv::CAP_PROP_POS_MSEC);

		if ((m_cur_frame.empty()) && (pos < m_Duration))
		{
			pos = m_Duration;
		}
	}

    return pos;
}

/////////////////////////////////////////////////////////////////////////////
// ImRGB in format b:g:r:0
void OCVVideo::GetRGBImage(custom_buffer<int> &ImRGB, int xmin, int xmax, int ymin, int ymax)
{
	if (m_VC.isOpened() && (!m_cur_frame.empty()))
	{
		int w, h, x, y, i, j, di;
		u8 *color, *img_data = m_cur_frame.data;

		w = xmax - xmin + 1;
		h = ymax - ymin + 1;

		di = m_Width - w;

		i = ymin*m_Width + xmin;
		j = 0;

		for (y = 0; y < h; y++)
		{
			for (x = 0; x < w; x++)
			{
				color = (u8*)(ImRGB.m_pData+j);

				color[3] = 0;
				color[2] = img_data[i * 3 + 2]; //r
				color[1] = img_data[i * 3 + 1]; //g
				color[0] = img_data[i * 3]; //b

				i++;
				j++;
			}
			i += di;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::SetImageGeted(bool ImageGeted)
{
	m_ImageGeted = ImageGeted;
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::RunWithTimeout(s64 timeout)
{
	if (m_VC.isOpened())
	{
		Run();
		wxMilliSleep(timeout);
		Pause();
	}
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::Run()
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

			m_pThreadRunVideo = new ThreadRunVideo(this);
			m_pThreadRunVideo->Create();
			m_pThreadRunVideo->Run();
			//m_pThreadRunVideo->SetPriority(30); //THREAD_PRIORITY_BELOW_NORMAL
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::Pause()
{
	if (m_VC.isOpened() && m_play_video)
	{
		m_play_video = false;
		while (!m_pThreadRunVideo->IsDetached()) { wxMilliSleep(30); }
	}
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::Stop()
{
	Pause();
	SetPos((s64)0);
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::StopFast()
{
	Stop();
}


/////////////////////////////////////////////////////////////////////////////

void OCVVideo::WaitForCompletion(s64 timeout)
{
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::SetVideoWindowPosition(int left, int top, int width, int height, void *dc)
{
	ShowFrame(m_cur_frame, dc);
}

ThreadRunVideo::ThreadRunVideo(OCVVideo *pVideo) : wxThread()
{
	m_pVideo = pVideo;
}

void *ThreadRunVideo::Entry()
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
		m_pVideo->ShowFrame(m_pVideo->m_cur_frame);
		int dt = (int)(1000.0 / m_pVideo->m_fps) - (int)(clock() - start_t);
		if (dt > 0) wxMilliSleep(dt);
	}

	return 0;
}