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
	if ((!img.empty()) && m_VC.isOpened() && m_show_video && (dc != NULL))
	{
		int wnd_w, wnd_h, img_w = img.cols, img_h = img.rows, num_pixels = img_w*img_h;
		((wxPaintDC*)dc)->GetSize(&wnd_w, &wnd_h);

		if ((wnd_w > 0) && (wnd_h > 0) && (img_w > 0) && (img_h > 0))
		{
			unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

			for (int i = 0; i < num_pixels; i++)
			{
				img_data[i * 3] = img.data[i * 3 + 2];
				img_data[i * 3 + 1] = img.data[i * 3 + 1];
				img_data[i * 3 + 2] = img.data[i * 3];
			}

			((wxPaintDC*)dc)->DrawBitmap(wxImage(img_w, img_h, img_data).Scale(wnd_w, wnd_h), 0, 0);
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

		m_frameNumbers = m_VC.get(cv::CAP_PROP_FRAME_COUNT);

		if (m_frameNumbers < 0)
		{
			double Pos = 0, add_pos = 60000, prev_pos, cur_pos;

			do
			{
				Pos += add_pos;
				prev_pos = m_VC.get(cv::CAP_PROP_POS_MSEC);
				m_VC.set(cv::CAP_PROP_POS_MSEC, Pos);
				cur_pos = m_VC.get(cv::CAP_PROP_POS_MSEC);
			} while (prev_pos != cur_pos);

			m_frameNumbers = cur_pos;
			m_VC.set(cv::CAP_PROP_POS_FRAMES, 0);
			m_VC >> m_cur_frame;			
		}

		m_fps = m_VC.get(cv::CAP_PROP_FPS);
		m_Duration = ((m_frameNumbers * 1000.0) / m_fps);

		m_ImageGeted = true;

		if (m_show_video)
		{
			((wxWindow*)m_pVideoWindow)->Refresh(true);
		}
	}

	return res;
}

/////////////////////////////////////////////////////////////////////////////

bool OCVVideo::SetVideoWindowPlacement(void *pVideoWindow)
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

		double FN = m_VC.get(cv::CAP_PROP_FRAME_COUNT);
		m_VC.set(cv::CAP_PROP_POS_FRAMES, ((double)Pos*FN)/(double)m_Duration);			
		OneStep();
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

		s64 prevPos, curPos;
		s64 prevNumFrameToBeDecoded, curNumFrameToBeDecoded;
		int num_tries = 0;

		do
		{
			prevPos = m_VC.get(cv::CAP_PROP_POS_MSEC);
			prevNumFrameToBeDecoded = m_VC.get(cv::CAP_PROP_POS_FRAMES);
			m_VC >> m_cur_frame;
			curPos = m_VC.get(cv::CAP_PROP_POS_MSEC);
			curNumFrameToBeDecoded = m_VC.get(cv::CAP_PROP_POS_FRAMES);
			num_tries++;

			if ((curPos != prevPos) || (curNumFrameToBeDecoded != prevNumFrameToBeDecoded))
			{
				num_tries = 0;
			}
		} while ((m_cur_frame.empty()) && ((curPos != prevPos) || (curNumFrameToBeDecoded != prevNumFrameToBeDecoded) || (num_tries < 100)));				

		m_Pos = curPos;

		if (!m_cur_frame.empty())
		{	
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

s64 OCVVideo::OneStepWithTimeout()
{
	OneStep();
	return GetPos();
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::ErrorMessage(wxString str)
{
	wxMessageBox(str, "ERROR MESSAGE");
}

/////////////////////////////////////////////////////////////////////////////

s64 OCVVideo::GetPos()
{
	s64 pos = 0;
	
	if (m_VC.isOpened())
	{
		pos = m_Pos;
	}

    return pos;
}

/////////////////////////////////////////////////////////////////////////////

int OCVVideo::ConvertToBGR(u8* frame_data, simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
{
	int w, h, x, y, i, j, di;
	u8* data = frame_data;
	cv::Mat Im;

	if ((m_Width != m_origWidth) || (m_Height != m_origHeight))
	{
		Im = cv::Mat(m_origHeight, m_origWidth, CV_8UC3);
		memcpy(Im.data, frame_data, m_origWidth * m_origHeight * 3);

		//cv::imshow("Im orig size", Im);

		cv::resize(Im, Im, cv::Size(m_Width, m_Height), 0, 0, cv::INTER_LINEAR);
		data = Im.data;

		//cv::imshow("Im resized", Im);		
		//cv::waitKey(0);
	}

	w = xmax - xmin + 1;
	h = ymax - ymin + 1;	

	di = m_Width - w;

	i = ymin * m_Width + xmin;
	j = 0;

	custom_assert(ImBGR.m_size >= w * h * 3, "OCVVideo::ConvertToBGR not: ImBGR.m_size >= w * h * 3");

	if (w == m_Width)
	{
		ImBGR.copy_data(data, j * 3, i * 3, w * h * 3);
	}
	else
	{
		for (y = 0; y < h; y++)
		{
			ImBGR.copy_data(data, j * 3, i * 3, w * 3);
			j += w;
			i += m_Width;
		}
	}

	return 1;
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::GetBGRImage(simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
{
	if (!m_cur_frame.empty())
	{		
		ConvertToBGR(m_cur_frame.data, ImBGR, xmin, xmax, ymin, ymax);
	}
}

/////////////////////////////////////////////////////////////////////////////

int OCVVideo::GetFrameDataSize()
{
	return (m_origWidth * m_origHeight * 3);
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::GetFrameData(simple_buffer<u8>& FrameData)
{
	if (!m_cur_frame.empty())
	{
		custom_assert(FrameData.size() == m_origWidth * m_origHeight * 3, "void OCVVideo::GetFrameData(simple_buffer<u8>& FrameData)\nnot: FrameData.size() == m_origWidth * m_origHeight * 3");
		FrameData.copy_data(m_cur_frame.data, m_origWidth * m_origHeight * 3);
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
		m_pVideo->m_Pos = m_pVideo->m_VC.get(cv::CAP_PROP_POS_MSEC);
		
		((wxWindow*)m_pVideo->m_pVideoWindow)->Refresh(true);
		int dt = (int)(1000.0 / m_pVideo->m_fps) - (int)(clock() - start_t);
		if (dt > 0) wxMilliSleep(dt);
	}

	return 0;
}