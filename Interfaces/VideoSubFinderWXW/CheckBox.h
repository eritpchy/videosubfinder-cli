                              //DataGrid.h//                                
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
#include <wx/checkbox.h>

class CCheckBox : public wxCheckBox
{
public:
	bool* m_p_val;

	CCheckBox(wxWindow* parent,
		wxWindowID id,
		bool* p_val,
		const wxString& label,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	void OnCheckBoxEvent(wxCommandEvent& evt);
	
private:
   DECLARE_EVENT_TABLE()
};
