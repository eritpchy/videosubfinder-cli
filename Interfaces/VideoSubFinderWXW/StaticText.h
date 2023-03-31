                              //StaticText.h//                                
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
#include "Control.h"

class CStaticText : public wxPanel, public CControl
{
public:
	CStaticText(wxWindow* parent,
				   const wxString& label,
				   wxWindowID id = wxID_ANY,
				   long text_style = wxALIGN_CENTER,
				   long panel_style = wxTAB_TRAVERSAL | wxBORDER,
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize);
	
	CStaticText(wxWindow* parent,
		const wxString&& label,
		wxWindowID id = wxID_ANY,
		long text_style = wxALIGN_CENTER,
		long panel_style = wxTAB_TRAVERSAL | wxBORDER,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize) = delete;

	~CStaticText();
	
	wxWindow		*m_pParent;
	wxStaticText	*m_pST;
	long			m_text_style;
	bool			m_allow_auto_set_min_width = true;

public:
	void SetMinSize(wxSize& size);
	void SetFont(wxFont& font);
	void SetTextColour(wxColour& colour);
	void SetBackgroundColour(wxColour& colour);
	void SetLabel(const wxString& label);
	void SetLabel(const wxString&& label) = delete;	
	void OnSize(wxSizeEvent& event);
	void RefreshData();
	wxSize GetOptimalSize(int add_gap = 6);

private:
	wxSize m_min_size;
	const wxString* m_p_label;
	wxFont *m_pFont = NULL;
	wxColour* m_pTextColour = NULL;
	wxColour* m_pBackgroundColour = NULL;
	DECLARE_EVENT_TABLE()
};
