                              //Video.h//                                
//////////////////////////////////////////////////////////////////////////////////
//							  Version 1.76              						//
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
#include <Windows.h>
#include <fstream>

using namespace std;

/////////////////////////////////////////////////////////////////////////////

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
	string		m_MovieName;
	bool		m_Inited;

	long		m_Width;
	long		m_Height;

	s64			m_Duration;

	string		m_log;
	string		m_Dir;

public:

	virtual bool OpenMovieNormally(string csMovieName, void *pHWnd)
	{
		return false;

	}
	virtual bool OpenMovieAllDefault(string csMovieName, void *pHWnd)
	{
		return false;
	}

	virtual bool OpenMovieHard(string csMovieName, void *pHWnd)
	{
		return false;
	}

	virtual bool SetVideoWindowPlacement(void *pHWnd)
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

    virtual void GetRGBImage(int *ImRGB, int xmin, int xmax, int ymin, int ymax)
	{
	}

	virtual void SetVideoWindowPosition(int left, int top, int width, int height)
	{
	}
};
