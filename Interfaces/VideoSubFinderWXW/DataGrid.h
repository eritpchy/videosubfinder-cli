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
#include "DataTypes.h"

using namespace std;

class CDataGrid : public wxGrid, public CControl
{
public:
	CDataGrid ( wxWindow* parent,
				wxString& grid_col_property_label,
				wxString& grid_col_value_label,
				wxWindowID id = wxID_ANY,
				wxFont* pFont = NULL,
				wxColour* pTextColour = NULL,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize );		
	~CDataGrid();

public:
	int m_w;
	int m_h;
	wxFont* m_pFont = NULL;
	wxColour* m_pTextColour = NULL;
	wxColour* m_pGridLineColour = NULL;
	wxColour* m_pBackgroundColour = NULL;
	wxString* m_p_grid_col_property_label;
	wxString* m_p_grid_col_value_label;

	void AddGroup(wxString& label, wxColour& colour);
	void AddGroup(wxString&& label, wxColour& colour) = delete;

	void AddSubGroup(wxString& label, wxColour& colour);
	void AddSubGroup(wxString&& label, wxColour& colour) = delete;
	
	void AddProperty(wxString& label,
					wxColour& colour1, wxColour& colour2,
					wxString *pstr);
	void AddProperty(wxString&& label,
					wxColour& colour1, wxColour& colour2,
					wxString* pstr) = delete;

	void AddProperty(wxString& label,
					wxColour& colour1, wxColour& colour2,
					wxString *pstr, wxArrayString& vals);
	void AddProperty(wxString&& label,
					wxColour& colour1, wxColour& colour2,
					wxString* pstr, wxArrayString&& vals) = delete;

	void AddProperty(wxString& label,
					wxColour& colour1, wxColour& colour2,
					wxArrayString* pstr);
	void AddProperty(wxString&& label,
					wxColour& colour1, wxColour& colour2,
					wxArrayString* pstr) = delete;

	void AddProperty(wxString& label, 
					wxColour& colour1, wxColour& colour2,
					int *pval, int val_min, int val_max);
	void AddProperty(wxString&& label,
					wxColour& colour1, wxColour& colour2,
					int* pval, int val_min, int val_max) = delete;

	void AddProperty(wxString& label, 
					  wxColour& colour1, wxColour& colour2,
					  double *pval, double val_min, double val_max);
	void AddProperty(wxString&& label,
		wxColour& colour1, wxColour& colour2,
		double* pval, double val_min, double val_max) = delete;

	void AddProperty(wxString &label,
					wxColour& colour1, wxColour& colour2,
					bool *pbln);
	void AddProperty(wxString&& label,
					wxColour& colour1, wxColour& colour2,
					bool* pbln) = delete;

	//void AddSubGroup();
	//bool SetFont(const wxFont& font);
	//void SetLabel(const wxString& label);
	void SetBackgroundColour(wxColour& col);
	//void SetTextColour(const wxColour& colour);
	void OnGridCellChanging(wxGridEvent& event);
	void OnSize(wxSizeEvent& event);
	void SetGridColLaberls();
	void SetGridLineColour(wxColour& col);
	wxSize GetOptimalSize();
	void UpdateSize() override;
	void RefreshData() override;

private:
   DECLARE_EVENT_TABLE()
};
