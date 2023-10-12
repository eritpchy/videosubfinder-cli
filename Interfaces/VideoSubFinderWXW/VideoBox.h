                              //VideoBox.h//                                
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
#include <wx/toolbar.h>
#include <wx/minifram.h>
#include <wx/image.h>
#include <mutex>
#include "MyResource.h"
#include "MainFrm.h"
#include "StaticText.h"
#include "ScrollBar.h"
#include "SeparatingLine.h"
#include "ResizableWindow.h"
#include "BitmapButton.h"
#include "Control.h"

class CVideoBox;
class CVideoWindow;
class CPopupHelpWindow;

class CVideoWnd : public wxWindow
{
public:
	CVideoWnd(CVideoWindow *pVW);
	~CVideoWnd();

	CVideoWindow	*m_pVW;
	CVideoBox		*m_pVB;

	bool			m_filter_image;	

public:
	void OnPaint( wxPaintEvent &event );
	void OnEraseBackGround(wxEraseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
	//void OnKeyUp(wxKeyEvent& event);
	bool CheckFilterImage();
	void DrawImage(simple_buffer<u8>& ImBGR, const int w, const int h);	

private:
   DECLARE_EVENT_TABLE()
};

class CVideoWindow: public wxPanel
{
public:
	CVideoWindow(CVideoBox *pVB);
	~CVideoWindow();

	CVideoWnd		*m_pVideoWnd;
	CSeparatingLine *m_pHSL1;
	CSeparatingLine *m_pHSL2;
	CSeparatingLine *m_pVSL1;
	CSeparatingLine *m_pVSL2;

	bool			m_WasInited;
	CVideoBox		*m_pVB;

public:
	void Init();
	void Update();
	void Refresh(bool eraseBackground = true,
		const wxRect* rect = (const wxRect*)NULL);

public:
	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent &event);	

private:
   DECLARE_EVENT_TABLE()
};

class CVideoBox : public CResizableWindow, public CControl
{
public:
	CVideoBox(CMainFrame* pMF);
	~CVideoBox();

	std::mutex m_mutex;

	CBitmapButton	*m_pButtonRun;
	CBitmapButton	*m_pButtonPause;
	CBitmapButton	*m_pButtonStop;
	CStaticText		*m_plblVB;
	CStaticText		*m_plblTIME;
	CVideoWindow	*m_pVBox;
	CScrollBar		*m_pSB;

	wxImage	m_tb_run_origImage;
	wxImage	m_tb_pause_origImage;
	wxImage	m_tb_stop_origImage;
	wxColour m_prevBackgroundColour;

	wxImage	*m_pImage;
	wxFrame *m_pFullScreenWin;

	CMainFrame	*m_pMF;
	bool		m_WasInited;

	wxTimer		m_timer;

	CPopupHelpWindow* m_pHW;

public:
	void Init();
	void ViewImage(simple_buffer<int> &Im, int w, int h);
	void ViewGrayscaleImage(simple_buffer<u8>& Im, int w, int h);
	void ViewBGRImage(simple_buffer<u8>& ImBGR, int w, int h);
	void ClearScreen();
	void UpdateSize() override;
	void RefreshData() override;

public:
	void OnSize(wxSizeEvent& event);	
	void OnBnClickedRun(wxCommandEvent& event);
	void OnBnClickedPause(wxCommandEvent& event);
	void OnBnClickedStop(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnHScroll(wxScrollEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnRButtonDown(wxMouseEvent& event);

private:
	std::mutex m_view_mutex;

	DECLARE_EVENT_TABLE()
};


