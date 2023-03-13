                              //Button.h//                                
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
#include <wx/button.h>
#include "Control.h"
#include "BitmapButton.h"

class CButton : public CBitmapButton, public CControl
{
public:
	CButton(wxWindow* parent,
		wxWindowID id,
		wxColour& button_color,
		wxColour& button_color_focused,
		wxColour& button_color_selected,
		wxColour& buttons_border_colour,
		const wxString& label = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	void SetLabel(const wxString& label);
	void RefreshData();
	void SetFont(wxFont& font);
	void SetTextColour(wxColour& colour);

	void OnSize(wxSizeEvent& event);
	
private:
	wxWindow* m_pParent;
	CBitmapButton* m_pButton;
	wxFont* m_pFont;
	wxColour* m_p_button_color;
	wxColour* m_p_button_color_focused;
	wxColour* m_p_button_color_selected;
	wxColour* m_p_buttons_border_colour;
	wxColour* m_p_text_colour;
	wxString m_label;

	void FillButtonBitmap(wxBitmap& bmp, wxColour parent_colour, wxColour button_colour, wxColour button_border_colour);

	DECLARE_EVENT_TABLE()
};
