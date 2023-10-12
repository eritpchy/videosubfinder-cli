                              //DataGrid.cpp//                                
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

#include "DataGrid.h"
#include "Control.h"
#include <wx/dcclient.h>
#include <numeric>

class CGridCellTextEditor: public wxGridCellTextEditor, public CControl
{
public:
	CGridCellTextEditor(int row, int col, CDataGrid* grid, wxString *pstr, wxColour& colour)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;
		
		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_pstr = pstr;
		m_grid->SetCellValue(m_row, m_col, *m_pstr );
	}

	
	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res;

		res = wxGridCellTextEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		*m_pstr = *newval;
		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);
		m_grid->SetCellValue(m_row, m_col, *m_pstr);		
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxString *m_pstr;
	wxColour* m_pBackgroundColour;
};

class CGridCellAutoWrapStringEditor : public wxGridCellAutoWrapStringEditor, public CControl
{
public:
	CGridCellAutoWrapStringEditor(int row, int col, CDataGrid* grid, wxArrayString* pstr, wxColour& colour)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_pstr = pstr;

		m_grid->SetCellValue(m_row, m_col, wxJoin(*m_pstr, '\n'));
	}


	bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString* newval) override
	{
		bool res;

		res = wxGridCellTextEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		*m_pstr = wxSplit(*newval, '\n');
		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);
		m_grid->SetCellValue(m_row, m_col, wxJoin(*m_pstr, '\n'));		
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxColour* m_pBackgroundColour;
	wxArrayString* m_pstr;
};

class CGridCellChoiceEditor : public wxGridCellChoiceEditor, public CControl
{
public:
	CGridCellChoiceEditor(int row, int col, CDataGrid* grid, wxString *pstr, wxArrayString& vals, wxColour& colour) : wxGridCellChoiceEditor(vals)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_p_vals = &vals;
		m_pstr = pstr;

		bool found = false;
		for (auto && v : *m_p_vals)
		{
			if (*m_pstr == v)
			{				
				found = true;
				break;
			}
		}
		if (!found)
		{
			*m_pstr = (*m_p_vals)[0];
		}
		grid->SetCellValue(row, col, *m_pstr);
	}

	bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString* newval) override
	{
		bool res;

		res = wxGridCellChoiceEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		*m_pstr = *newval;
		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		bool found = false;
		int pos;
		for (pos = 0; pos < m_choices.size(); pos++)
		{
			if (*m_pstr == m_choices[pos])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			custom_assert(found, "CGridCellChoiceEditor::EndEdit not: found");
			pos = 0;
		}

		wxString params = std::accumulate(std::begin(*m_p_vals), std::end(*m_p_vals), wxString(),
			[](wxString& ss, wxString& s)
			{
				return ss.empty() ? s : ss + wxT(",") + s;
			});
		this->SetParameters(params);

		*m_pstr = (*m_p_vals)[pos];
		m_grid->SetCellValue(m_row, m_col, *m_pstr);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxColour* m_pBackgroundColour;
	wxString *m_pstr;
	wxArrayString *m_p_vals;
};

class CGridCellNumberEditor: public wxGridCellNumberEditor, public CControl
{
public:
	CGridCellNumberEditor(int row, int col, CDataGrid* grid, int *pval, int val_min, int val_max, wxColour& colour)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_pval = pval;
		m_vmin = val_min;
		m_vmax = val_max;

		Str << *m_pval;
		m_grid->SetCellValue(m_row, m_col, Str );
	}

	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res;
		
		res = wxGridCellNumberEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		wxString Str = *newval;
		int val = (int)strtod(Str, NULL);		
			
		if ( (val >= m_vmin) && (val <= m_vmax) )
		{
			*m_pval = val;
		}
		else
		{
			Str = "";
			Str << *m_pval;
			*newval = Str;
			res = false;
		}

		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		wxString Str;
		Str << *m_pval;
		m_grid->SetCellValue(m_row, m_col, Str);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxColour* m_pBackgroundColour;
	int m_vmin;
	int m_vmax;
	int *m_pval;
};

class CGridCellFloatEditor: public wxGridCellFloatEditor, public CControl
{
public:
	CGridCellFloatEditor(int row, int col, CDataGrid* grid, double *pval, double val_min, double val_max, wxColour& colour)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_pval = pval;
		m_vmin = val_min;
		m_vmax = val_max;

		Str << *m_pval;
		grid->SetCellValue( row, col, Str );
	}

	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res;
		wxString Str;
		double val;
		
		res = wxGridCellFloatEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		wxString Str = *newval;
		double val = strtod(Str, NULL);
			
		if ( (val >= m_vmin) && (val <= m_vmax) )
		{
			*m_pval = val;
		}
		else
		{
			Str = "";
			Str << *m_pval;
			*newval = Str;
			res = false;
		}

		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		wxString Str;
		Str << *m_pval;
		m_grid->SetCellValue(m_row, m_col, Str);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxColour* m_pBackgroundColour;
	double m_vmin;
	double m_vmax;
	double *m_pval;
};

class CGridCellBoolEditor: public wxGridCellBoolEditor, public CControl
{
public:
	CGridCellBoolEditor(int row, int col, CDataGrid* grid, bool *pbln, wxColour& colour)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pBackgroundColour = &colour;
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		m_pbln = pbln;

		if (*m_pbln)
		{
			Str = "1";
		}
		else
		{
			Str = "";
		}

		grid->SetCellValue( row, col, Str );
	}	

	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res = true;
		wxString Str;
		
		res = wxGridCellBoolEditor::EndEdit(row, col, grid, oldval, newval);
		if (res == true)
		{
			res = UpdateData(newval);
		}

		return res;
	}

	bool UpdateData(wxString *newval) override
	{
		bool res = true;
		if (IsTrueValue(*newval))
		{
			*m_pbln = true;
		}
		else
		{
			*m_pbln = false;
		}
		return res;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		if (m_grid->m_pTextColour) m_grid->SetCellTextColour(m_row, m_col, *(m_grid->m_pTextColour));
		m_grid->SetCellBackgroundColour(m_row, m_col, *m_pBackgroundColour);

		wxString Str;
		
		if (*m_pbln)
		{
			Str = "1";
		}
		else
		{
			Str = "";
		}

		m_grid->SetCellValue(m_row, m_col, Str);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxColour* m_pBackgroundColour;
	bool *m_pbln;
};

BEGIN_EVENT_TABLE(CDataGrid, wxGrid)
	EVT_SIZE(CDataGrid::OnSize)
	EVT_GRID_CELL_CHANGING(CDataGrid::OnGridCellChanging)
END_EVENT_TABLE()

CDataGrid::CDataGrid(	wxWindow* parent,
						wxString& grid_col_property_label,
						wxString& grid_col_value_label,
						wxWindowID id,
						wxFont* pFont,
						wxColour* pTextColour,
						const wxPoint& pos,
						const wxSize& size )				
		:wxGrid( parent, id, pos, size )
{
	m_p_grid_col_property_label = &grid_col_property_label;
	m_p_grid_col_value_label = &grid_col_value_label;
	m_w = 0;
	m_h = 0;	
	m_pFont = pFont;
	m_pTextColour = pTextColour;
	this->CreateGrid( 0, 2 );
	this->SetRowLabelSize(0);

	if (m_pFont) this->SetLabelFont(*m_pFont);	
	if (m_pTextColour)
	{
		wxGrid::SetLabelTextColour(*m_pTextColour);
		wxGrid::SetDefaultCellTextColour(*m_pTextColour);
	}
	
	SetGridColLaberls();
}

CDataGrid::~CDataGrid()
{
}

void CDataGrid::SetGridColLaberls()
{
	if (m_pFont) this->SetLabelFont(*m_pFont);
	if (m_pTextColour) this->SetLabelTextColour(*m_pTextColour);

	wxClientDC dc(this);
	if (m_pFont) dc.SetFont(*m_pFont);
	if (m_pTextColour) dc.SetTextForeground(*m_pTextColour);
	
	wxString label_first = wxString(wxT(" ")) + *m_p_grid_col_property_label;
	wxString label_second = wxString(wxT(" ")) + *m_p_grid_col_value_label;
	int label_height = std::max<int>(dc.GetMultiLineTextExtent(label_first).y, dc.GetMultiLineTextExtent(label_second).y) + 6;

	this->SetColLabelSize(label_height);
	this->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetColLabelValue(0, label_first);
	this->SetColLabelValue(1, label_second);
}

void CDataGrid::SetGridLineColour(wxColour& col)
{
	m_pGridLineColour = &col;
	wxGrid::SetGridLineColour(*m_pGridLineColour);
}

void CDataGrid::SetBackgroundColour(wxColour& col)
{
	m_pBackgroundColour = &col;
	wxGrid::SetBackgroundColour(*m_pBackgroundColour);
	wxGrid::SetDefaultCellBackgroundColour(*m_pBackgroundColour);
	wxGrid::SetLabelBackgroundColour(*m_pBackgroundColour);
}

void CDataGrid::OnGridCellChanging(wxGridEvent& event)
{
	int row = event.GetRow(), col = event.GetCol();
	wxString updateval, newval;
	newval = event.GetString();
	updateval = newval;
	wxGridCellEditor *cel_editr = this->GetCellEditor(row, col);	
	(dynamic_cast<CControl*>(cel_editr))->UpdateData(&updateval);
	cel_editr->DecRef();
	if (updateval != newval)
	{
		this->SetCellValue(row, col, updateval);
	}
}

void CDataGrid::OnSize(wxSizeEvent& event)
{
	wxRect rcDG;
	rcDG = this->GetSize();

	if (rcDG.width != m_w)
	{
		UpdateSize();
		m_w = rcDG.width;
	}
}

void CDataGrid::RefreshData()
{
	if (m_pFont) this->SetLabelFont(*m_pFont);
	if (m_pTextColour)
	{
		wxGrid::SetLabelTextColour(*m_pTextColour);
		wxGrid::SetDefaultCellTextColour(*m_pTextColour);
	}
	if (m_pGridLineColour) wxGrid::SetGridLineColour(*m_pGridLineColour);

	if (m_pBackgroundColour)
	{
		wxGrid::SetBackgroundColour(*m_pBackgroundColour);
		wxGrid::SetDefaultCellBackgroundColour(*m_pBackgroundColour);
		wxGrid::SetLabelBackgroundColour(*m_pBackgroundColour);
	}

	SetGridColLaberls();
}

wxSize CDataGrid::GetOptimalSize()
{
	wxClientDC dc(this);
	if (m_pFont) dc.SetFont(*m_pFont);
	int max_text_w = 0;
	int opt_cw;

	for (int index = 0; index < this->GetNumberRows(); index++)
	{
		wxString label = this->GetCellValue(index, 0);
		wxSize text_size = dc.GetMultiLineTextExtent(label);
		this->SetRowSize(index, text_size.GetHeight() + 6);

		int rows, cols;
		this->GetCellSize(index, 0, &rows, &cols);

		if (cols == 1)
		{
			if (text_size.GetWidth() > max_text_w)
			{
				max_text_w = text_size.GetWidth();
			}
		}
	}
	max_text_w += 6;

	opt_cw = (double)max_text_w / 0.65;

	wxSize cur_size = this->GetSize();
	wxSize cur_client_size = this->GetClientSize();
	wxSize opt_size(10, 10);
	opt_size.x = opt_cw + cur_size.x - cur_client_size.x;

	return opt_size;
}

void CDataGrid::UpdateSize()
{
	wxClientDC dc(this);
	if (m_pFont) dc.SetFont(*m_pFont);
	int max_text_w = 0;
	int cw = GetClientSize().x;
	int first_col_size;
	int dw = 4;
	double max_percent = 0.85;

	for (int index = 0; index < this->GetNumberRows(); index++)
	{
		wxString label = this->GetCellValue(index, 0);
		wxSize text_size = dc.GetMultiLineTextExtent(label);
		this->SetRowSize(index, text_size.GetHeight() + 6);

		int rows, cols;
		this->GetCellSize(index, 0, &rows, &cols);

		if (cols == 1)
		{
			if (text_size.GetWidth() > max_text_w)
			{
				max_text_w = text_size.GetWidth();
			}
		}
	}
	max_text_w += 6;

	// Note: fix for appeared horizontal scroll bar
	SetColSize(0, cw / 10);
	SetColSize(1, cw / 10);

	if (max_text_w <= cw * 0.65)
	{
		first_col_size = cw * 0.65;
		SetColSize(0, first_col_size);
		SetColSize(1, cw - first_col_size - dw);
	}
	else if (max_text_w <= cw * max_percent)
	{
		SetColSize(0, max_text_w);
		SetColSize(1, cw - max_text_w - dw);
	}
	else
	{
		first_col_size = cw * max_percent;
		SetColSize(0, first_col_size);
		SetColSize(1, cw - first_col_size - dw);
	}
}

void CDataGrid::AddGroup(wxString& label, wxColour& colour)
{
	int index;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellSize(index, 0, 1, 2);
	this->SetCellAlignment(index, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour));
}

void CDataGrid::AddSubGroup(wxString& label, wxColour& colour)
{
	int index;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellSize(index, 0, 1, 2);
	this->SetCellAlignment(index, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour));
}

void CDataGrid::AddProperty(wxString& label, 
							 wxColour& colour1, wxColour& colour2,							 
							 wxString *pstr )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	if (m_pTextColour) this->SetCellTextColour(index, 1, *m_pTextColour);
    this->SetCellEditor( index, 1, new CGridCellTextEditor(index, 1, this, pstr, colour2));
}

void CDataGrid::AddProperty(wxString& label,
							wxColour& colour1, wxColour& colour2,							
							wxString *pstr, wxArrayString& vals)
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows() - 1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetReadOnly(index, 0);
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	if (m_pTextColour) this->SetCellTextColour(index, 1, *m_pTextColour);
	this->SetCellEditor(index, 1, new CGridCellChoiceEditor(index, 1, this, pstr, vals, colour2));
}

void CDataGrid::AddProperty(wxString& label,
	wxColour& colour1, wxColour& colour2,
	wxArrayString* pstr)
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows() - 1;	

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetReadOnly(index, 0);
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetRowSize(index, 16 * 3);

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_TOP);
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	if (m_pTextColour) this->SetCellTextColour(index, 1, *m_pTextColour);
	this->SetCellRenderer(index, 1, new wxGridCellAutoWrapStringRenderer);
	this->SetCellEditor(index, 1, new CGridCellAutoWrapStringEditor(index, 1, this, pstr, colour2));
}


void CDataGrid::AddProperty(wxString& label,
							 wxColour& colour1, wxColour& colour2,
							 int *pval, int val_min, int val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	if (m_pTextColour) this->SetCellTextColour(index, 1, *m_pTextColour);
	this->SetCellEditor( index, 1, new CGridCellNumberEditor(index, 1, this, pval, val_min, val_max, colour2) );
}

void CDataGrid::AddProperty(wxString& label,
							 wxColour& colour1, wxColour& colour2,
							 double *pval, double val_min, double val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	if (m_pTextColour) this->SetCellTextColour(index, 1, *m_pTextColour);
	this->SetCellEditor( index, 1, new CGridCellFloatEditor(index, 1, this, pval, val_min, val_max, colour2) );
}

void CDataGrid::AddProperty(wxString& label,
							 wxColour& colour1, wxColour& colour2,
							 bool *pbln )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
    this->SetReadOnly( index, 0 );
	this->SetCellEditor(index, 0, new CGridCellTextEditor(index, 0, this, &label, colour1));

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellRenderer( index, 1, new wxGridCellBoolRenderer);
    this->SetCellEditor( index, 1, new CGridCellBoolEditor(index, 1, this, pbln, colour2));
}
