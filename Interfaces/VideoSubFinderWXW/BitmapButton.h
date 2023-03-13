                              //BitmapButton.h//                                
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
#include <wx/image.h>

class CBitmapButton : public wxWindow
{
public:
	CBitmapButton(wxWindow* parent,
		wxWindowID id,
		const wxImage& image,
		const wxImage& image_focused,
		const wxImage& image_selected,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	CBitmapButton(wxWindow* parent,
		wxWindowID id,
		const wxImage& image,
		const wxImage& image_selected,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	CBitmapButton(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	void OnLButtonDown( wxMouseEvent& event );
	void OnLButtonUp( wxMouseEvent& event );
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);	
	void OnPaint(wxPaintEvent& event);
	void SetBitmaps(const wxImage& image,
		const wxImage& image_focused,
		const wxImage& image_selected);

private:
	bool	m_bDown;
	bool	m_bImagesDefined;
	wxImage m_image;
	wxImage m_image_focused;
	wxImage m_image_selected;
	wxWindow* m_parent;

	DECLARE_EVENT_TABLE()
};
