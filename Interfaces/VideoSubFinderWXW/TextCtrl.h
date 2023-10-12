                              //TextCtrl.h//                                
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
#include <wx/textctrl.h>
#include <wx/regex.h>
#include "Control.h"

class CTextCtrl : public wxTextCtrl, public CControl
{
public:
	wxWindow* m_pParent;
	wxString* m_p_str_val = NULL;
	double* m_p_f_val = NULL;
	wxString m_str_val;
	wxString m_re_expr = wxString();
	wxRegEx m_re;
	

	CTextCtrl(wxWindow* parent,
		wxWindowID id,
		wxString str_val = wxString(),
		wxString re_expr = wxString(),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	CTextCtrl(wxWindow* parent, 
		wxWindowID id,
		wxString* p_str_val,
		wxString  re_expr = wxString(),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	CTextCtrl(wxWindow* parent,
		wxWindowID id,
		double* p_f_val,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	void OnText(wxCommandEvent& evt);
	void OnTextEnter(wxCommandEvent& evt);
	void OnKillFocus(wxFocusEvent& event);

	void SetValue(const wxString& value) wxOVERRIDE;

	bool ValidateAndSaveData(wxString val);
	void RefreshData();

	void SetFont(wxFont& font);
	void SetTextColour(wxColour& colour);
	void SetBackgroundColour(wxColour& colour);
	void SetMinSize(wxSize& size);
	
private:
	wxSize m_min_size;
	wxFont* m_pFont = NULL;
	wxColour* m_pTextColour = NULL;
	wxColour* m_pBackgroundColour = NULL;
	DECLARE_EVENT_TABLE()
};
