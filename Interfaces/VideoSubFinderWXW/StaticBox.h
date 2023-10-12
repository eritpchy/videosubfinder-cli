                              //StaticBox.h//                                
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
#include <wx/statbox.h>
#include "Control.h"

class CStaticBox : public wxStaticBox, public CControl
{
public:
    CStaticBox(wxWindow* parent, wxWindowID id,
        const wxString& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxStaticBoxNameStr);
    CStaticBox(wxWindow* parent, wxWindowID id,
        const wxString&& label,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = 0,
        const wxString& name = wxStaticBoxNameStr) = delete;

	void RefreshData();
	void SetFont(wxFont& font);
    void SetTextColour(wxColour& colour);
    void SetBackgroundColour(wxColour& colour);
    void SetLabel(const wxString& label);
    void SetLabel(const wxString&& label) = delete;
    void SetMinSize(wxSize& size);
    void UpdateSize();
	
private:
    wxSize m_min_size;
    wxWindow* m_pParent;
	wxFont* m_pFont = NULL;
    wxColour* m_pTextColour = NULL;
    wxColour* m_pBackgroundColour = NULL;
    const wxString* m_p_label;
};
