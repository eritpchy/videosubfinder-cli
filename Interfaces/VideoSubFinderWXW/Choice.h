                              //Choice.h//                                
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
#include <wx/choice.h>
#include "Control.h"

class CChoice : public wxChoice, public CControl
{
public:
	CChoice(wxWindow* parent,
		wxArrayString& vals,
		wxString* p_str_selection,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);

	CChoice(wxWindow* parent,
		wxArrayString& vals,
		int* p_int_selection,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize);
	
	~CChoice();
	
	wxWindow		*m_pParent;

public:
	void SetMinSize(wxSize& size);
	void SetFont(wxFont& font);
	void SetTextColour(wxColour& colour);
	void SetBackgroundColour(wxColour& colour);	
	wxSize GetOptimalSize(int vgap = 0, int hgap = 0);
	void RefreshData();
	
	void OnChoice(wxCommandEvent& event);

private:
	int m_vgap;
	int m_hgap;
	wxArrayString m_vals;
	wxString* m_p_str_selection = NULL;
	int* m_p_int_selection = NULL;
	wxSize m_min_size;
	wxFont *m_pFont = NULL;
	wxColour* m_pTextColour = NULL;
	wxColour* m_pBackgroundColour = NULL;

	DECLARE_EVENT_TABLE()
};
