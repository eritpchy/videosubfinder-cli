                              //SSOWnd.h//                                
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
#include <wx/aui/auibook.h>
#include <wx/artprov.h>
#include "MyResource.h"
#include "SearchPanel.h"
#include "SettingsPanel.h"
#include "OCRPanel.h"
#include "MainFrm.h"

class CMainFrame;
class CSearchPanel;
class CSettingsPanel;
class COCRPanel;

class CSSOWnd : public wxAuiNotebook
{
public:
	CSSOWnd(CMainFrame* pMF);           // protected constructor used by dynamic creation
	~CSSOWnd();

	wxFont			m_LBLFont;
	//wxAuiNotebook  *m_pAN;
	CSearchPanel   *m_pSHPanel;
	CSettingsPanel *m_pSSPanel;
	COCRPanel	   *m_pOCRPanel;

	bool			m_WasInited;
	CMainFrame	   *m_pMF;

public:
	void Init();
	void ResizeControls();
	void OnSize(UINT nType, int cx, int cy);
	void OnTcnSelchangeTab(NMHDR *pNMHDR, LRESULT *pResult);

private:
   DECLARE_EVENT_TABLE()
};


