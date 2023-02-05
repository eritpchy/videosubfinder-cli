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
	CStaticText (wxWindow* parent,
				   wxWindowID id = wxID_ANY,				   
				   const wxString& label = "",
				   long text_style = wxALIGN_CENTER,
				   long panel_style = wxTAB_TRAVERSAL | wxBORDER,				   
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize);		
	~CStaticText();
	
	wxWindow		*m_pParent;
	wxStaticText	*m_pST;
	long			m_text_style;

public:
	void SetFont(wxFont& font);
	void SetLabel(const wxString& label);
	bool SetBackgroundColour(const wxColour& colour);
	void SetTextColour(const wxColour& colour);
	void OnSize(wxSizeEvent& event);
	void RefreshData();

private:
	wxFont *m_pFont;
	DECLARE_EVENT_TABLE()
};
