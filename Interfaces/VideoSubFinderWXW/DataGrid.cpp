                              //DataGrid.cpp//                                
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

#include "DataGrid.h"

class CGridCellTextEditor: public wxGridCellTextEditor
{
public:
	CGridCellTextEditor(int row, int col, wxGrid* grid, wxString *pstr)
	{
		m_row = row;
		m_col = col;
		m_grid = grid;

		m_pstr = pstr;
		grid->SetCellValue( row, col, *m_pstr );
	}

	bool EndEdit(int row, int col, wxGrid* grid)
	{
		bool res;

		res = wxGridCellTextEditor::EndEdit(row, col, grid);

		if (res == true)
		{
			*m_pstr = grid->GetCellValue(row, col);			
		}

		return res;
	}

	int m_row;
	int m_col;
	wxGrid* m_grid;

	wxString *m_pstr;
};

class CGridCellNumberEditor: public wxGridCellNumberEditor
{
public:
	CGridCellNumberEditor(int row, int col, wxGrid* grid, int *pval, int val_min, int val_max)
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

	bool EndEdit(int row, int col, wxGrid* grid)
	{
		bool res;
		wxString Str;
		int val;
		
		res = wxGridCellNumberEditor::EndEdit(row, col, grid);

		if (res == true)
		{
			Str = grid->GetCellValue(row, col);
			val = (int)strtod(Str, NULL);
			
			if ( (val >= m_vmin) && (val <= m_vmax) )
			{
				*m_pval = val;
			}
			else
			{
				Str = "";
				Str << *m_pval;
				grid->SetCellValue( row, col, Str );
			}
		}

		return res;
	}

	int m_row;
	int m_col;
	wxGrid* m_grid;

	int m_vmin;
	int m_vmax;
	int *m_pval;
};

class CGridCellFloatEditor: public wxGridCellFloatEditor
{
public:
	CGridCellFloatEditor(int row, int col, wxGrid* grid, double *pval, double val_min, double val_max)
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

	bool EndEdit(int row, int col, wxGrid* grid)
	{
		bool res;
		wxString Str;
		double val;
		
		res = wxGridCellFloatEditor::EndEdit(row, col, grid);

		if (res == true)
		{
			Str = grid->GetCellValue(row, col);
			val = strtod(Str, NULL);
			
			if ( (val >= m_vmin) && (val <= m_vmax) )
			{
				*m_pval = val;
			}
			else
			{
				Str = "";
				Str << *m_pval;
				grid->SetCellValue( row, col, Str );
			}
		}

		return res;
	}

	int m_row;
	int m_col;
	wxGrid* m_grid;

	double m_vmin;
	double m_vmax;
	double *m_pval;
};

class CGridCellBoolEditor: public wxGridCellBoolEditor
{
public:
	CGridCellBoolEditor(int row, int col, wxGrid* grid, bool *pbln)
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

	bool EndEdit(int row, int col, wxGrid* grid)
	{
		bool res;
		wxString Str;
		
		res = wxGridCellBoolEditor::EndEdit(row, col, grid);

		if (res == true)
		{
			*m_pbln = !(*m_pbln);			
		}

		if (*m_pbln)
		{
			Str = "1";
		}
		else
		{
			Str = "";
		}

		grid->SetCellValue( row, col, Str );

		return res;
	}

	int m_row;
	int m_col;
	wxGrid* m_grid;

	bool *m_pbln;
};

BEGIN_EVENT_TABLE(CDataGrid, wxGrid)
	EVT_SIZE(CDataGrid::OnSize)
END_EVENT_TABLE()

CDataGrid::CDataGrid( wxWindow* parent,
					  wxWindowID id,
					  const wxPoint& pos,
					  const wxSize& size )				
		:wxGrid( parent, id, pos, size )
{
	m_w = 0;
	m_h = 0;
	this->CreateGrid( 0, 2 );
	this->SetRowLabelSize(0);

	this->SetColLabelSize(20);
	this->SetColLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);
	this->SetColLabelValue(0, " Property");
	this->SetColLabelValue(1, " Value");
}

CDataGrid::~CDataGrid()
{
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

void CDataGrid::AddGroup(wxString label, wxColour colour, wxFont font)
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
	this->SetCellFont( index, 0, font );
}

void CDataGrid::AddSubGroup(wxString label, wxColour colour, wxFont font)
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
	this->SetCellFont( index, 0, font );
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 wxFont font1, wxFont font2,
							 wxString *pstr )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	this->SetCellFont( index, 0, font1 );
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellFont( index, 1, font2 );
	this->SetCellBackgroundColour( index, 1, colour2 );
    this->SetCellEditor( index, 1, new CGridCellTextEditor(index, 1, this, pstr));
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 wxFont font1, wxFont font2, 
							 int *pval, int val_min, int val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	this->SetCellFont( index, 0, font1 );
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellFont( index, 1, font2 );
	this->SetCellBackgroundColour( index, 1, colour2 );
	this->SetCellEditor( index, 1, new CGridCellNumberEditor(index, 1, this, pval, val_min, val_max) );
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 wxFont font1, wxFont font2, 
							 double *pval, double val_min, double val_max )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	this->SetCellFont( index, 0, font1 );
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellFont( index, 1, font2 );
	this->SetCellBackgroundColour( index, 1, colour2 );
	this->SetCellEditor( index, 1, new CGridCellFloatEditor(index, 1, this, pval, val_min, val_max) );
}

void CDataGrid::AddProperty( wxString label, 
							 wxColour colour1, wxColour colour2,
							 wxFont font, bool *pbln )
{
	int index;
	wxString Str;
	this->AppendRows(1);

	index = this->GetNumberRows()-1;

	this->SetCellAlignment(index, 0, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellValue( index, 0, label );	
    this->SetReadOnly( index, 0 );
	this->SetCellFont( index, 0, font );
	this->SetCellBackgroundColour( index, 0, colour1 );

	this->SetCellAlignment(index, 1, wxALIGN_LEFT, wxALIGN_CENTRE);	
	this->SetCellBackgroundColour( index, 1, colour2 );
	
	this->SetCellRenderer( index, 1, new wxGridCellBoolRenderer);
    this->SetCellEditor( index, 1, new CGridCellBoolEditor(index, 1, this, pbln));
}
