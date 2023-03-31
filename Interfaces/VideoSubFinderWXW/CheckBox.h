                              //CheckBox.h//                                
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
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include "Control.h"

class CCheckBox : public wxPanel, public CControl
{
public:
	bool* m_p_val;

	CCheckBox(wxWindow* parent,
		wxWindowID id,
		bool* p_val,
		const wxString& label,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long check_box_style = wxALIGN_RIGHT | wxCHK_2STATE | wxALIGN_CENTER_VERTICAL,
		long text_style = wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL,
		long panel_style = wxTAB_TRAVERSAL | wxBORDER);

	CCheckBox(wxWindow* parent,
		wxWindowID id,
		bool* p_val,
		const wxString&& label,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long check_box_style = wxALIGN_RIGHT | wxCHK_2STATE | wxALIGN_CENTER_VERTICAL,
		long text_style = wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL,
		long panel_style = wxTAB_TRAVERSAL | wxBORDER) = delete;

	void OnSize(wxSizeEvent& event);
	void OnCheckBoxEvent(wxCommandEvent& evt);
	void SetFont(wxFont& font);
	void SetTextColour(wxColour& colour);
	void SetLabel(const wxString& label);
	void SetBackgroundColour(wxColour& colour);	
	void SetMinSize(wxSize& size);
	void RefreshData();
	
	wxWindow*		m_pParent;
	wxStaticText*	m_pST;
	wxCheckBox*		m_pCB;
	long			m_text_style;
	long			m_check_box_style;
	int				m_cb_offset = 2;

private:
	wxSize m_min_size;
	wxFont* m_pFont = NULL;
	wxColour* m_pTextColour = NULL;
	wxColour* m_pBackgroundColour = NULL;
	const wxString* m_p_label;
	DECLARE_EVENT_TABLE()
};
