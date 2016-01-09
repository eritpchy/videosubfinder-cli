                              //ScrollBar.h//                                
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
#include <wx/image.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/scrolbar.h>

class CScrollBar : public wxPanel
{
public:
	CScrollBar( wxWindow *parent, 
				wxWindowID id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize );
	~CScrollBar();

public:
	wxWindow	*m_pParent;

	wxBitmap	m_Bmp;
	wxBitmap	m_PrevBmp;
	wxImage		m_SBLA;
	wxImage		m_SBRA;
	wxImage		m_SBLC;
	wxImage		m_SBRC;
	wxImage		m_SBT;

	wxRect		m_rcSBLA; 
	wxRect		m_rcSBRA;
	wxRect		m_rcSBT;
	wxRect		m_rcSBLC;
	wxRect		m_rcSBRC;

	bool		m_ld;

	int			m_pos;
	int			m_min_pos;
	int			m_max_pos;
	int			m_range;

	int		GetScrollPos() { return m_pos; }
	void	SetScrollPos (int pos);
	void	SetScrollRange (int min_pos, int max_pos);
	
	void	OnPaint( wxPaintEvent &event );
	void	OnLButtonDown( wxMouseEvent& event );
	void	OnLButtonUp( wxMouseEvent& event );
	void	OnMouseMove( wxMouseEvent& event );

	void	ScrollLeft();
	void	ScrollRight();	

private:
   DECLARE_EVENT_TABLE()
};
