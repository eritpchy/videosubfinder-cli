                              //SeparatingLine.h//                                
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
#include <wx/window.h>
#include <wx/region.h>
#include <wx/dcclient.h>
//#include <wx/toplevel.h>
//#include <wx/minifram.h>
//#include <wx/frame.h>
//#include <wx/mdi.h>
//#include <wx/button.h>

class CSeparatingLine : public wxWindow
{
public:
	CSeparatingLine(wxWindow *parent, int w, int h, int sw, int sh, int minpos, int maxpos, int offset, int orientation, wxWindowID id = wxID_ANY);
	~CSeparatingLine();

	int m_w;
	int m_old_w;
	int m_h;
	int m_old_h;

	int m_sw;
	int m_sh;

	int m_min;
	int m_max;
	int	m_offset;

	int	m_orientation; // 0 horizontal, 1 vertical
	
	double	m_pos;
	double	m_pos_min;
	double	m_pos_max;

	wxWindow* m_pParent;
	wxRegion m_rgn;

	bool	m_bDown;

public:
	void CreateNewRgn();
	void MoveSL(wxPoint pt);
	void UpdateSL();
	double CalculateCurPos();
	int GetCurPos();

public:
	void	OnPaint( wxPaintEvent &event );
	void	OnEraseBackground(wxEraseEvent &event);
	void	OnLButtonDown( wxMouseEvent& event );
	void	OnLButtonUp( wxMouseEvent& event );
	void	OnMouseMove( wxMouseEvent& event );

private:
   DECLARE_EVENT_TABLE()
};