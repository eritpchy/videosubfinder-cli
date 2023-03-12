                              //Video.h//                                
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
#include <fstream>

using namespace std;

class CVideo
{
public:
	CVideo()
	{
		m_MovieName = "";
		m_Inited = false;
		m_Width = 0;
		m_Height = 0;
		m_Duration = 0;
		m_log = "";
		m_Dir = "";
	}

	virtual ~CVideo()
	{
	}
	
public:
	bool		m_play_video;

	wxString	m_MovieName;
	bool		m_Inited;

	long		m_Width;
	long		m_Height;

	s64			m_Duration;
	s64			m_Pos;

	wxString		m_log;
	wxString		m_Dir;
	void	*m_pVideoWindow;

	// VideoWindowSettins
	int m_xmin;
	int m_xmax;
	int m_ymin;
	int m_ymax;
	int m_w;
	int m_h;

public:

	void SetVideoWindowSettins(double dx_min, double dx_max, double dy_min, double dy_max)
	{
		m_xmin = (int)(dx_min*(double)m_Width);
		m_xmax = (int)(dx_max*(double)m_Width) - 1;
		m_ymin = (int)(dy_min*(double)m_Height);
		m_ymax = (int)(dy_max*(double)m_Height) - 1;

		m_w = m_xmax - m_xmin + 1;
		m_h = m_ymax - m_ymin + 1;
	}

	virtual bool OpenMovie(wxString csMovieName, void	*pVideoWindow, int type)
	{
		return false;
	}

	virtual bool SetVideoWindowPlacement(void *pVideoWindow)
	{
		return false;
	}

	virtual bool SetNullRender()
	{
		return false;
	}

	virtual bool CloseMovie()
	{
		return false;
	}

	virtual void SetPos(s64 Pos)
	{
	}

	virtual void SetPos(double pos)
	{
	}

	virtual void SetPosFast(s64 Pos)
	{
	}

	virtual void SetImageGeted(bool ImageGeted)
	{
	}

	virtual void Run()
	{
	}

	virtual void Pause()
	{
	}

	virtual void WaitForCompletion(s64 timeout)
	{		
	}

	virtual void StopFast()
	{
	}

	virtual void RunWithTimeout(s64 timeout)
	{
	}

	virtual void Stop()
	{
	}

    virtual void OneStep()
	{
	}

	virtual s64 OneStepWithTimeout()
	{
		return 0;
	}

    virtual s64  GetPos()
	{
		return 0;
	}

	virtual void GetBGRImage(simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
	{
	}

	virtual int ConvertToBGR(u8* frame_data, simple_buffer<u8>& ImBGR, int xmin, int xmax, int ymin, int ymax)
	{
		return -1;
	}

	virtual int GetFrameDataSize()
	{
		return 0;
	}

	virtual void GetFrameData(simple_buffer<u8>& FrameData)
	{
	}
	
	virtual void SetVideoWindowPosition(int left, int top, int width, int height, void *dc)
	{
	}
};

extern CVideo *g_pV;

extern wxString ConvertVideoTime(s64 pos);
