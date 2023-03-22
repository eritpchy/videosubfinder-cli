                              //VideoSubFinderWXW.h//                                
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

#include "wx/wx.h"
#include "DataTypes.h"
#ifdef USE_GUI
#include "MainFrm.h"
#else
#include <execution>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include "OCVVideoLoader.h"
#include "FFMPEGVideoLoader.h"
#include "SSAlgorithms.h"
#include "IPAlgorithms.h"
#endif

class CVideoSubFinderApp : public wxApp
{
public:
	~CVideoSubFinderApp();

#ifdef USE_GUI
	CMainFrame* m_pMainWnd;
#endif

public:
	virtual bool OnInit();
	virtual bool Initialize(int& argc, wxChar **argv);
};

IMPLEMENT_APP(CVideoSubFinderApp)
