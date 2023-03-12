//ResizableWindow.h//                                
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
#include <wx/window.h>

class CResizableWindow : public wxWindow
{
public:
	CResizableWindow(wxWindow* parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	~CResizableWindow();
	
	void	OnLButtonDown(wxMouseEvent& event);
	void	OnLButtonUp(wxMouseEvent& event);
	void	OnMouseMove(wxMouseEvent& event);
	void	OnMouseLeave(wxMouseEvent& event);
	void	OnMouseCaptureLost(wxMouseCaptureLostEvent& event);
	
	void UpdateCursor(int x, int y);

	int m_border_size = 4;
	int m_move_border_size = 40;
	bool m_bDownResize = false;
	bool m_bDownFromLeft = false;
	bool m_bDownFromRight = false;
	bool m_bDownFromTop = false;
	bool m_bDownFromBottom = false;
	bool m_bDownMove = false;
	wxPoint m_dm_orig_point;
	wxStockCursor m_cur_cursor = wxCURSOR_ARROW;

private:
	DECLARE_EVENT_TABLE()
};