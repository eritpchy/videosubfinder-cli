                              //VideoSubFinder.h//                                
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

#include "wx/wx.h"
#include "DataTypes.h"
#include "MainFrm.h"

class CVideoSubFinderApp : public wxApp 
{
public:
	~CVideoSubFinderApp();

	CMainFrame* m_pMainWnd;

public:
	virtual bool OnInit();
};

IMPLEMENT_APP(CVideoSubFinderApp)
