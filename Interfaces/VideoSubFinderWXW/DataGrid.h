                              //DataGrid.h//                                
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
#include <wx/grid.h>
#include <vector>

using namespace std;

class DataGridGroup
{
public:
	DataGridGroup(int i, int rb, int re)
	{
		m_i = i;
		m_rb = rb;
		m_re = re;
	}

	int m_i;
	int m_rb;
	int m_re;
};

class CDataGrid : public wxGrid
{
public:
	CDataGrid ( wxWindow* parent,
				wxWindowID id = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize );		
	~CDataGrid();

public:
	int m_w;
	int m_h;
	vector<DataGridGroup> m_DataGridGroups;
	vector<DataGridGroup> m_DataGridSubGroup;

	void AddGroup(wxString label, wxColour colour, wxFont font);
	void AddSubGroup(wxString label, wxColour colour, wxFont font);
	
	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
					  wxFont font1, wxFont font2,
					  wxString *pstr );

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
		              wxFont font1, wxFont font2, 
					  int *pval, int val_min, int val_max );

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
		              wxFont font1, wxFont font2, 
					  double *pval, double val_min, double val_max );

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
		              wxFont font, bool *pbln );
	//void AddSubGroup();
	//bool SetFont(const wxFont& font);
	//void SetLabel(const wxString& label);
	//bool SetBackgroundColour(const wxColour& colour);
	//void SetTextColour(const wxColour& colour);
	void OnSize(wxSizeEvent& event);

private:
   DECLARE_EVENT_TABLE()
};
