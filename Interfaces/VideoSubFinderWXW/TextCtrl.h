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
#include "Control.h"

class CTextCtrl : public wxTextCtrl, public CControl
{
public:
	wxString* m_p_str_val = NULL;
	double* m_p_f_val = NULL;
	
	CTextCtrl(wxWindow* parent, wxWindowID id,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxTextCtrlNameStr);

	CTextCtrl(wxWindow* parent, 
		wxWindowID id,
		wxString* p_str_val,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	CTextCtrl(wxWindow* parent,
		wxWindowID id,
		double* p_f_val,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	void OnTextCtrlEvent(wxCommandEvent& evt);
	void RefreshData();

	void SetFont(wxFont& font);
	
private:
	wxFont* m_pFont;
	DECLARE_EVENT_TABLE()
};
