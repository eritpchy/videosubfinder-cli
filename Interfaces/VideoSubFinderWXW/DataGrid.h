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
#include <wx/grid.h>
#include <vector>
#include "Control.h"

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

class CDataGrid : public wxGrid, public CControl
{
public:
	CDataGrid ( wxWindow* parent,				
				wxWindowID id = wxID_ANY,
				wxFont* pFont = NULL,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize );		
	~CDataGrid();

public:
	int m_w;
	int m_h;
	wxFont* m_pFont;
	vector<DataGridGroup> m_DataGridGroups;
	vector<DataGridGroup> m_DataGridSubGroup;

	void AddGroup(wxString label, wxColour colour);
	void AddSubGroup(wxString label, wxColour colour);
	
	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,					  
					  wxString *pstr );

	void AddProperty(wxString label,
		wxColour colour1, wxColour colour2,
		wxString *pstr, wxArrayString vals);

	void AddProperty(wxString label,
		wxColour colour1, wxColour colour2,
		wxArrayString* pstr);

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
					  int *pval, int val_min, int val_max );

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
					  double *pval, double val_min, double val_max );

	void AddProperty( wxString label, 
					  wxColour colour1, wxColour colour2,
		              bool *pbln );

	//void AddSubGroup();
	//bool SetFont(const wxFont& font);
	//void SetLabel(const wxString& label);
	//bool SetBackgroundColour(const wxColour& colour);
	//void SetTextColour(const wxColour& colour);
	void OnGridCellChanging(wxGridEvent& event);
	void OnSize(wxSizeEvent& event);
	void RefreshData();

private:
   DECLARE_EVENT_TABLE()
};
