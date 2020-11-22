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
#include <wx/mdi.h>
#include <wx/image.h>
#include <mutex>
#include "MyResource.h"
#include "MainFrm.h"
#include "StaticText.h"
#include "ScrollBar.h"
#include "SeparatingLine.h"

class CVideoBox;
class CVideoWindow;

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
	void OnSetFocus( wxFocusEvent &event );
	void OnEraseBackGround(wxEraseEvent& event);
	void OnLeftDown(wxMouseEvent& event);
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

class CVideoBox : public wxMDIChildFrame
{
public:
	CVideoBox(CMainFrame* pMF);
	~CVideoBox();

	wxToolBar		*m_pVBar;
	CStaticText		*m_plblVB;
	CStaticText		*m_plblTIME;
	CVideoWindow	*m_pVBox;
	CScrollBar		*m_pSB;

	wxColour	m_VBX;
	wxColour	m_CL1;
	wxColour	m_CL2;
	wxColour	m_CLVBar;

	wxImage	*m_pImage;	

	CMainFrame	*m_pMF;
	bool		m_WasInited;

public:
	void Init();
	void ViewImage(simple_buffer<int> &Im, int w, int h);
	void ViewGrayscaleImage(simple_buffer<u8>& Im, int w, int h);
	void ViewBGRImage(simple_buffer<u8>& ImBGR, int w, int h);
	void ClearScreen();

public:
	void OnSize(wxSizeEvent& event);	
	void OnBnClickedRun(wxCommandEvent& event);
	void OnBnClickedPause(wxCommandEvent& event);
	void OnBnClickedStop(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnHScroll(wxScrollEvent& event);

private:
	std::mutex m_view_mutex;

	DECLARE_EVENT_TABLE()
};


