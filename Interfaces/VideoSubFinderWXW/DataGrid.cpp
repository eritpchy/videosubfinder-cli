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

class CGridCellTextEditor: public wxGridCellTextEditor, public CControl
{
public:
	CGridCellTextEditor(int row, int col, CDataGrid* grid, wxString *pstr)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pstr = pstr;
		m_grid->SetCellValue(m_row, m_col, *m_pstr );
	}

	
	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res;

		res = wxGridCellTextEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			UpdateData(newval);
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
		*m_pstr = *newval;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		m_grid->SetCellValue(m_row, m_col, *m_pstr);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;
	wxString *m_pstr;
};

class CGridCellAutoWrapStringEditor : public wxGridCellAutoWrapStringEditor, public CControl
{
public:
	CGridCellAutoWrapStringEditor(int row, int col, CDataGrid* grid, wxArrayString* pstr)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pstr = pstr;

		m_grid->SetCellValue(m_row, m_col, wxJoin(*m_pstr, '\n'));
	}


	bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString* newval) override
	{
		bool res;

		res = wxGridCellTextEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			UpdateData(newval);
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
		*m_pstr = wxSplit(*newval, '\n');
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		m_grid->SetCellValue(m_row, m_col, wxJoin(*m_pstr, '\n'));
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;

	wxArrayString* m_pstr;
};

class CGridCellChoiceEditor : public wxGridCellChoiceEditor, public CControl
{
public:
	CGridCellChoiceEditor(int row, int col, CDataGrid* grid, wxString *pstr, wxArrayString vals) : wxGridCellChoiceEditor(vals)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;
		
		m_pstr = pstr;

		bool found = false;
		for (auto && v : vals)
		{
			if (*m_pstr == v)
			{				
				found = true;
				break;
			}
		}
		if (!found)
		{
			*m_pstr = vals[0];
		}
		grid->SetCellValue(row, col, *m_pstr);
	}

	bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString* newval) override
	{
		bool res;

		res = wxGridCellChoiceEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			UpdateData(newval);
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
		*m_pstr = *newval;
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));
		m_grid->SetCellValue(m_row, m_col, *m_pstr);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;

	wxString *m_pstr;
};

class CGridCellNumberEditor: public wxGridCellNumberEditor, public CControl
{
public:
	CGridCellNumberEditor(int row, int col, CDataGrid* grid, int *pval, int val_min, int val_max)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pval = pval;
		m_vmin = val_min;
		m_vmax = val_max;

		Str << *m_pval;
		grid->SetCellValue( row, col, Str );
	}

	bool EndEdit(int row, int col, const wxGrid *grid, const wxString &oldval, wxString *newval) override
	{
		bool res;
		
		res = wxGridCellNumberEditor::EndEdit(row, col, grid, oldval, newval);

		if (res == true)
		{
			UpdateData(newval);			
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
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
		}	
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));

		wxString Str;
		Str << *m_pval;
		m_grid->SetCellValue(m_row, m_col, Str);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;

	int m_vmin;
	int m_vmax;
	int *m_pval;
};

class CGridCellFloatEditor: public wxGridCellFloatEditor, public CControl
{
public:
	CGridCellFloatEditor(int row, int col, CDataGrid* grid, double *pval, double val_min, double val_max)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

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
			UpdateData(newval);
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
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
		}
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));

		wxString Str;
		Str << *m_pval;
		m_grid->SetCellValue(m_row, m_col, Str);
	}

	int m_row;
	int m_col;
	CDataGrid* m_grid;

	double m_vmin;
	double m_vmax;
	double *m_pval;
};

class CGridCellBoolEditor: public wxGridCellBoolEditor, public CControl
{
public:
	CGridCellBoolEditor(int row, int col, CDataGrid* grid, bool *pbln)
	{
		wxString Str;

		m_row = row;
		m_col = col;
		m_grid = grid;

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
			UpdateData(newval);
		}

		return res;
	}

	void UpdateData(wxString *newval) override
	{
		if (IsTrueValue(*newval))
		{
			*m_pbln = true;
		}
		else
		{
			*m_pbln = false;
		}
	}

	void RefreshData() override
	{
		if (m_grid->m_pFont) m_grid->SetCellFont(m_row, m_col, *(m_grid->m_pFont));

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

	bool *m_pbln;
};

BEGIN_EVENT_TABLE(CDataGrid, wxGrid)
	EVT_SIZE(CDataGrid::OnSize)
	EVT_GRID_CELL_CHANGING(CDataGrid::OnGridCellChanging)
END_EVENT_TABLE()

CDataGrid::CDataGrid( wxWindow* parent,
					  wxWindowID id,
					  wxFont* pFont,
					  const wxPoint& pos,
					  const wxSize& size )				
		:wxGrid( parent, id, pos, size )
{
	m_w = 0;
	m_h = 0;	
	m_pFont = pFont;
	this->CreateGrid( 0, 2 );
	this->SetRowLabelSize(0);

	this->SetColLabelSize(20);
	this->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetColLabelValue(0, " Property");
	this->SetColLabelValue(1, " Value");

	if (m_pFont) this->SetLabelFont(*m_pFont);
}

CDataGrid::~CDataGrid()
{
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
	int dw = 18;

	rcDG = this->GetSize();

	if (rcDG.width != m_w)
	{
		this->SetColSize(0, (2*(rcDG.width-dw))/3);
		this->SetColSize(1, (rcDG.width-dw)/3);

		m_w = rcDG.width;
	}
}

void CDataGrid::RefreshData()
{
	if (m_pFont)
	{
		wxClientDC dc(this);
		dc.SetFont(*m_pFont);
		int max_text_w = 0;
		int cw = GetClientSize().x;
		double max_percent = 0.85;

		for (int index = 0; index < this->GetNumberRows(); index++)
		{
			this->SetCellFont(index, 0, *m_pFont);
			wxString label = this->GetCellValue(index, 0);
			wxSize text_size = dc.GetMultiLineTextExtent(label);
			this->SetRowSize(index, text_size.GetHeight() + 6);

			if (text_size.GetWidth() > max_text_w)
			{
				max_text_w = text_size.GetWidth();
			}
		}
		max_text_w += 6;

		if (max_text_w <= cw * 0.65)
		{
			SetColSize(0, cw * 0.65);
			SetColSize(1, cw * 0.35);
		}
		else if (max_text_w <= cw * max_percent)
		{
			SetColSize(0, max_text_w);
			SetColSize(1, cw - max_text_w);
		}
		else
		{
			SetColSize(0, cw * max_percent);
			SetColSize(1, cw * (1.0 - max_percent));
		}

		this->SetLabelFont(*m_pFont);
	}
}

void CDataGrid::AddGroup(wxString label, wxColour colour)
{
	int index;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;
	m_DataGridGroups.push_back(DataGridGroup(index, index+1, index+1));

	this->SetCellSize(index, 0, 1, 2);
	this->SetCellAlignment(index, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
	
	this->SetCellValue( index, 0, label );
	this->SetCellBackgroundColour( index, 0, colour );
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont( index, 0, *m_pFont);
}

void CDataGrid::AddSubGroup(wxString label, wxColour colour)
{
	int index;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;
	m_DataGridSubGroup.push_back(DataGridGroup(index, index+1, index+1));

	this->SetCellSize(index, 0, 1, 2);
	this->SetCellAlignment(index, 0, wxALIGN_CENTRE, wxALIGN_CENTRE);
	
	this->SetCellValue( index, 0, label );
	this->SetCellBackgroundColour( index, 0, colour );
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,							 
							 wxString *pstr )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	this->SetCellBackgroundColour( index, 1, colour2 );
    this->SetCellEditor( index, 1, new CGridCellTextEditor(index, 1, this, pstr));
}

void CDataGrid::AddProperty(wxString label,
							wxColour colour1, wxColour colour2,							
							wxString *pstr, wxArrayString vals)
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows() - 1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetCellValue(index, 0, label);
	this->SetReadOnly(index, 0);
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour(index, 0, colour1);

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	this->SetCellBackgroundColour(index, 1, colour2);
	this->SetCellEditor(index, 1, new CGridCellChoiceEditor(index, 1, this, pstr, vals));
}

void CDataGrid::AddProperty(wxString label,
	wxColour colour1, wxColour colour2,
	wxArrayString* pstr)
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows() - 1;	

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetCellValue(index, 0, label);
	this->SetReadOnly(index, 0);
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour(index, 0, colour1);

	this->SetRowSize(index, 16 * 3);

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_TOP);
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	this->SetCellBackgroundColour(index, 1, colour2);
	this->SetCellRenderer(index, 1, new wxGridCellAutoWrapStringRenderer);
	this->SetCellEditor(index, 1, new CGridCellAutoWrapStringEditor(index, 1, this, pstr));
}


void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 int *pval, int val_min, int val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	this->SetCellBackgroundColour( index, 1, colour2 );
	this->SetCellEditor( index, 1, new CGridCellNumberEditor(index, 1, this, pval, val_min, val_max) );
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 double *pval, double val_min, double val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	if (m_pFont) this->SetCellFont(index, 1, *m_pFont);
	this->SetCellBackgroundColour( index, 1, colour2 );
	this->SetCellEditor( index, 1, new CGridCellFloatEditor(index, 1, this, pval, val_min, val_max) );
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 bool *pbln )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	if (m_pFont) this->SetCellFont(index, 0, *m_pFont);
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellBackgroundColour( index, 1, colour2 );
	
	this->SetCellRenderer( index, 1, new wxGridCellBoolRenderer);
    this->SetCellEditor( index, 1, new CGridCellBoolEditor(index, 1, this, pbln));
}
