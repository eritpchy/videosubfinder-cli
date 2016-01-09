                              //TextBox.h//                                
//////////////////////////////////////////////////////////////////////////////////
//							  Version 1.76              						//
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

class CTextBox : public wxPanel
{
public:
	CTextBox (wxWindow* parent,
				   wxWindowID id = wxID_ANY,				   
				   const wxString& label = "",
				   long text_style = wxALIGN_CENTER,
				   long style = wxTAB_TRAVERSAL | wxBORDER,				   
				   const wxPoint& pos = wxDefaultPosition,
				   const wxSize& size = wxDefaultSize,
				   const wxString& name = "panel");		
	~CTextBox();
	
	wxWindow		*m_pParent;
	wxStaticText	*m_pST;
	long			m_text_style;

public:
	bool SetFont(const wxFont& font);
	void SetLabel(const wxString& label);
	bool SetBackgroundColour(const wxColour& colour);
	void SetTextColour(const wxColour& colour);
	void OnSize(wxSizeEvent& event);

private:
   DECLARE_EVENT_TABLE()
};
