                              //SSOWnd.h//                                
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
#include <wx/aui/auibook.h>
#include <wx/artprov.h>
#include "MyResource.h"
#include "SearchPanel.h"
#include "SettingsPanel.h"
#include "OCRPanel.h"
#include "MainFrm.h"
#include "Control.h"

class CMainFrame;
class CSearchPanel;
class CSettingsPanel;
class COCRPanel;

class CTabArt : public wxAuiGenericTabArt
{
public:
	CMainFrame* m_pMF;

	CTabArt(CMainFrame* pMF);

	wxAuiTabArt* Clone() wxOVERRIDE
	{
		return new CTabArt(*this);
	}

	void DrawTab(wxDC& dc,
		wxWindow* wnd,
		const wxAuiNotebookPage& pane,
		const wxRect& inRect,
		int closeButtonState,
		wxRect* outTabRect,
		wxRect* outButtonRect,
		int* xExtent) wxOVERRIDE;
};

class CSSOWnd : public wxAuiNotebook, public CControl
{
public:
	CSSOWnd(CMainFrame* pMF);           // protected constructor used by dynamic creation
	~CSSOWnd();

	//wxAuiNotebook  *m_pAN;
	CSearchPanel   *m_pSHPanel;
	CSettingsPanel *m_pSSPanel;
	COCRPanel	   *m_pOCRPanel;

	bool			m_WasInited;
	CMainFrame	    *m_pMF;
	CTabArt			*m_pTabArt;

public:
	void Init();
	void RefreshData();
};


