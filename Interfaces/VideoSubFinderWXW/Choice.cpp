                              //Choice.cpp//                                
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

#include "Choice.h"
#include <wx/dcmemory.h>
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(CChoice, wxChoice)
	EVT_CHOICE(wxID_ANY, CChoice::OnChoice)
END_EVENT_TABLE()

CChoice::CChoice(wxWindow* parent,
	wxArrayString& vals,
	wxString* p_str_selection,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size)
		:wxChoice(parent, id, pos, size, vals)
{
	m_pParent = parent;
	m_p_str_selection = p_str_selection;
	m_vals = vals;
	
	if (!(this->SetStringSelection(*m_p_str_selection)))
	{
		this->Select(0);
		*m_p_str_selection = this->GetStringSelection();
	}

	wxSize def_best_size = wxChoice::GetBestSize();
	wxSize opt_size = GetOptimalSize();

	m_vgap = def_best_size.y - opt_size.y;
	m_hgap = def_best_size.x - opt_size.x;	
}

CChoice::CChoice(wxWindow* parent,
	wxArrayString& vals,
	int* p_int_selection,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size)
	:wxChoice(parent, id, pos, size, vals)
{
	m_pParent = parent;
	m_p_int_selection = p_int_selection;
	m_vals = vals;

	if (!(this->SetStringSelection(wxString::Format(wxT("%d"), *m_p_int_selection))))
	{
		this->Select(0);
		*m_p_int_selection = wxAtoi(this->GetStringSelection());
	}

	wxSize def_best_size = wxChoice::GetBestSize();
	wxSize opt_size = GetOptimalSize();

	m_vgap = def_best_size.y - opt_size.y;
	m_hgap = def_best_size.x - opt_size.x;
}

CChoice::~CChoice()
{
}

void CChoice::OnChoice(wxCommandEvent& event)
{
	if (m_p_str_selection)
	{
		*m_p_str_selection = this->GetStringSelection();
	}
	else if (m_p_int_selection)
	{
		*m_p_int_selection = wxAtoi(this->GetStringSelection());
	}

	event.Skip();
}

void CChoice::SetFont(wxFont& font)
{
	m_pFont = &font;
	wxChoice::SetFont(*m_pFont);
}

void CChoice::SetTextColour(wxColour& colour)
{
	m_pTextColour = &colour;
	wxChoice::SetForegroundColour(*m_pTextColour);
}

void CChoice::SetBackgroundColour(wxColour& colour)
{
	m_pBackgroundColour = &colour;
	wxChoice::SetBackgroundColour(*m_pBackgroundColour);
}

wxSize CChoice::GetOptimalSize(int vgap, int hgap)
{
	wxMemoryDC dc;
	if (m_pFont) dc.SetFont(*m_pFont);
	wxSize best_size;

	for (const wxString& val : m_vals)
	{
		wxSize size = dc.GetTextExtent(val);
		if (size.x > best_size.x)
		{
			best_size.x = size.x;
		}
		if (size.y > best_size.y)
		{
			best_size.y = size.y;
		}
	}

	wxSize opt_size;
	best_size.x += hgap;
	best_size.y += vgap;

	opt_size.x = std::max<int>(best_size.x, m_min_size.x);
	opt_size.y = std::max<int>(best_size.y, m_min_size.y);

	return opt_size;
}

void CChoice::RefreshData()
{
	if (m_pFont) wxChoice::SetFont(*m_pFont);
	if (m_pTextColour) wxChoice::SetForegroundColour(*m_pTextColour);
	if (m_pBackgroundColour) wxChoice::SetBackgroundColour(*m_pBackgroundColour);

	if (m_p_str_selection)
	{
		if (!(this->SetStringSelection(*m_p_str_selection)))
		{
			this->Select(0);
			*m_p_str_selection = this->GetStringSelection();
		}
	}
	else if (m_p_int_selection)
	{
		if (!(this->SetStringSelection(wxString::Format(wxT("%d"), *m_p_int_selection))))
		{
			this->Select(0);
			*m_p_int_selection = wxAtoi(this->GetStringSelection());
		}
	}

	wxSizer* pSizer = GetContainingSizer();
	if (pSizer)
	{	
		wxSize cur_size = this->GetSize();
		wxSize opt_size = GetOptimalSize(m_vgap, m_hgap);

		if (opt_size != cur_size)
		{
			pSizer->SetItemMinSize(this, opt_size);
			pSizer->Layout();
		}
	}
}

void CChoice::SetMinSize(wxSize& size)
{
	m_min_size = size;
}
