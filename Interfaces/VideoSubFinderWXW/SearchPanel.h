                              //SearchPanel.h//                                
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
#include <wx/panel.h>
#include <wx/stattext.h>
#include "SSOWnd.h"
#include "MyResource.h"

class CMainFrame;
class CSSOWnd;

extern int g_IsSearching;
extern int g_IsClose;

class ThreadSearchSubtitles : public wxThread
{
public:
    ThreadSearchSubtitles(CMainFrame *pMF);

    virtual void *Entry();

public:
    CMainFrame	*m_pMF;
};

class CSearchPanel : public wxPanel
{
public:
	CSearchPanel(CSSOWnd* pParent);
	~CSearchPanel();

	wxFont    m_BTNFont;
	wxFont    m_LBLFont;

	wxButton	*m_pClear;
	wxButton	*m_pRun;
	
	wxPanel		*m_pP1;

	wxStaticText  *m_plblBT1;
	wxStaticText  *m_plblBTA1;
	wxStaticText  *m_plblBT2;
	wxStaticText  *m_plblBTA2;
	
	wxColour   m_CLP;
	wxColour   m_CL1;
	wxColour   m_CL2;

	CSSOWnd		*m_pParent;

	CMainFrame	*m_pMF;

	ThreadSearchSubtitles *m_pSearchThread;

	void Init();

public:
	//HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	void OnBnClickedRun(wxCommandEvent& event);
	void OnBnClickedClear(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
};
