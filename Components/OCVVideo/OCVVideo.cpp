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

void OCVVideo::ShowFrame(cv::Mat &img, void *dc, int left, int top, int width, int height)
{
	if ((!img.empty()) && m_VC.isOpened() && m_show_video && (dc != NULL))
	{
		int img_w = img.cols, img_h = img.rows, num_pixels = img_w*img_h;
		int wnd_w, wnd_h;
		((wxPaintDC*)dc)->GetSize(&wnd_w, &wnd_h);

		if (left == -1)
		{
			left = 0;
			top = 0;
			width = wnd_w;
			height = wnd_h;
		}

		if ((width > 0) && (height > 0) && (img_w > 0) && (img_h > 0))
		{
			unsigned char *img_data = (unsigned char*)malloc(num_pixels * 3); // auto released by wxImage

			for (int i = 0; i < num_pixels; i++)
			{
				img_data[i * 3] = img.data[i * 3 + 2];
				img_data[i * 3 + 1] = img.data[i * 3 + 1];
				img_data[i * 3 + 2] = img.data[i * 3];
			}

			UpdateImageColor(img_data, img_w, img_h);

			if ((left != 0) || (top != 0) || (width != wnd_w) || (height != wnd_h))
			{
				wxBitmap bitmap(wnd_w, wnd_h);
				wxMemoryDC temp_dc;
				temp_dc.SelectObject(bitmap);

				temp_dc.DrawBitmap(wxImage(img_w, img_h, img_data).Scale(width, height), left, top);
				((wxPaintDC*)dc)->Blit(0, 0, wnd_w, wnd_h, &temp_dc, 0, 0);

				temp_dc.SelectObject(wxNullBitmap);
			}
			else
			{
				((wxPaintDC*)dc)->DrawBitmap(wxImage(img_w, img_h, img_data).Scale(width, height), left, top);
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

		if ((m_origWidth == 0) || (m_origHeight == 0))
		{
			wxString msg = wxString::Format(wxT("ERROR: Video \"%s\" has wrong frame sizes: FRAME_WIDTH: %d, FRAME_HEIGHT: %d\n"), csMovieName, m_origWidth, m_origHeight);
			SaveToReportLog(wxString::Format(wxT("OCVVideo::OpenMovie(): %s\n"), msg));
			wxMessageBox(msg, wxT("OCVVideo::OpenMovie"));
			CloseMovie();
			return false;
		}

		/*if (m_origWidth > 1280)
		{
			double zoum = (double)1280 / (double)m_origWidth;
			m_Width = 1280;
			m_Height = (double)m_origHeight*zoum;
		}*/

		m_pVideoWindow = pVideoWindow;
		m_pVideoWindow ? m_show_video = true : m_show_video = false;

		m_VC >> m_cur_frame;

		m_Pos = m_VC.get(cv::CAP_PROP_POS_MSEC);
		m_fps = m_VC.get(cv::CAP_PROP_FPS);
		m_frameNumbers = m_VC.get(cv::CAP_PROP_FRAME_COUNT);
		m_Duration = -1;
		
		if (m_frameNumbers > 0)
		{
			m_Duration = ((m_frameNumbers * 1000.0) / m_fps);
		}
		
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

		double dt = 1000.0 / m_fps;

		if ((Pos < m_Pos) && (Pos > (m_Pos - dt * 3 / 2)))
		{
			double pos = m_VC.get(cv::CAP_PROP_POS_FRAMES);
			m_VC.set(cv::CAP_PROP_POS_FRAMES, pos-2);
			OneStep();
			double cur_pos = m_VC.get(cv::CAP_PROP_POS_FRAMES);
			cur_pos = cur_pos;
		}
		else
		{
			if (m_frameNumbers > 0)
			{
				m_VC.set(cv::CAP_PROP_POS_FRAMES, ((double)Pos * m_frameNumbers) / (double)m_Duration);
				OneStep();
			}
		}		
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

	UpdateImageColor(ImBGR, w, h);

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
		std::lock_guard<std::mutex> guard(m_run_mutex);

		if (!m_play_video)
		{
			//Pause();
		//}

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
}

/////////////////////////////////////////////////////////////////////////////

void OCVVideo::Pause()
{
	if (m_VC.isOpened())
	{
		std::lock_guard<std::mutex> guard(m_pause_mutex);

		if (m_play_video)
		{
			
			m_play_video = false;
			while (!m_pThreadRunVideo->IsDetached()) { wxMilliSleep(30); }
		}
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
	ShowFrame(m_cur_frame, dc, left, top, width, height);
}

ThreadRunVideo::ThreadRunVideo(OCVVideo *pVideo) : wxThread()
{
	m_pVideo = pVideo;
}

void *ThreadRunVideo::Entry()
{
	double startPos = m_pVideo->m_VC.get(cv::CAP_PROP_POS_MSEC);
	std::chrono::time_point<std::chrono::high_resolution_clock> start_t = std::chrono::high_resolution_clock::now();

	while (m_pVideo->m_play_video)
	{		
		
		m_pVideo->m_VC >> m_pVideo->m_cur_frame;
		if (m_pVideo->m_cur_frame.empty())
		{
			m_pVideo->m_play_video = false;
			break;
		}
		m_pVideo->m_Pos = m_pVideo->m_VC.get(cv::CAP_PROP_POS_MSEC);
		
		((wxWindow*)m_pVideo->m_pVideoWindow)->Refresh(true);
		int dt = (m_pVideo->m_Pos - startPos) - (int)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_t).count());
		if (dt > 0)
		{
			wxMilliSleep(dt);
		}
	}	

	return 0;
}