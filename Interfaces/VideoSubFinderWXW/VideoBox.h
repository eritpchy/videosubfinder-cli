                              //VideoBox.h//                                
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
#include <wx/toolbar.h>
#include <wx/mdi.h>
#include <wx/image.h>
#include "MyResource.h"
#include "MainFrm.h"
#include "TextBox.h"
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

public:
	void	OnPaint( wxPaintEvent &event );
	void	OnSetFocus( wxFocusEvent &event );

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

public:
	void OnSize( wxSizeEvent& event );
	void OnPaint( wxPaintEvent &event );

private:
   DECLARE_EVENT_TABLE()
};

class CVideoBox : public wxMDIChildFrame
{
public:
	CVideoBox(CMainFrame* pMF);
	~CVideoBox();

	wxFont    m_LBLFont;

	wxToolBar		*m_pVBar;
	CTextBox		*m_plblVB;
	CTextBox		*m_plblTIME;
	CVideoWindow	*m_pVBox;
	CScrollBar		*m_pSB;

	wxColour	m_VBX;
	wxColour	m_CL1;
	wxColour	m_CL2;
	wxColour	m_CLVBar;

	wxBitmap	*m_pBmp;
	wxBitmap	*m_pBmpScaled;
	int			m_w;
	int			m_h;
	int			m_wScaled;
	int			m_hScaled;

	CMainFrame	*m_pMF;
	bool		m_WasInited;

public:
	void Init();
	void ViewImage(int *Im, int w, int h);

public:
	void OnSize(wxSizeEvent& event);	
	void OnBnClickedRun(wxCommandEvent& event);
	void OnBnClickedPause(wxCommandEvent& event);
	void OnBnClickedStop(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnMouseWheel(wxMouseEvent& event);
	void OnHScroll(wxScrollEvent& event);

private:
   DECLARE_EVENT_TABLE()
};


