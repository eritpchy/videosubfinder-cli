                              //TrackBar.h//                                
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
#include "myresource.h"
#include "DataTypes.h"
#include "MainFrm.h"

class CTrackBar;
class CMainFrame;

class CTBLine : public CWnd
{
public:
	CTBLine();
	virtual ~CTBLine();
	
	CTrackBar *m_pParent;

protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

class CTBBox : public CWnd
{
public:
	CTBBox();
	virtual ~CTBBox();

	CTrackBar *m_pParent;
	
protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
};

class CTrackBar : public CWnd
{

public:
	CTrackBar();
	virtual ~CTrackBar();

	CTBLine  m_Line;
	CTBBox   m_Box;
	
	CMainFrame* m_pMF;

	CBrush   m_CL1Brush;
	CBrush   m_CL2Brush;

	double   m_Pos;
	int      m_lh;
	int      m_bw; 
	int      m_bh;

	bool     m_WasInited;

	bool	m_bDown;

public:
	void Init(CWnd* pParent);
	void ResizeControls();
	void MoveBox(s64 Pos);
	void MoveBox();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

