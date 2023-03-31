                              //SettingsPanel.h//                                
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
#include "SSOWnd.h"
#include "StaticText.h"
#include "DataGrid.h"
#include "DataTypes.h"
#include "TextCtrl.h"
#include "Button.h"
#include "BitmapButton.h"
#include "StaticBox.h"
#include <wx/panel.h>
#include <wx/bmpbuttn.h>

class CMainFrame;
class CSSOWnd;

class CSettingsPanel : public wxPanel, public CControl
{
public:
	CSettingsPanel(CSSOWnd* pParent);
	~CSettingsPanel();

	CDataGrid	*m_pOI;
	CDataGrid	*m_pOIM;

	CButton		*m_pTest;
	CStaticBox *m_pGB1;
	CStaticBox *m_pGB2;
	CStaticBox	*m_pGB3;
	wxPanel		*m_pP2;

	CBitmapButton *m_pLeft;
	CBitmapButton *m_pRight;
	CStaticText	   *m_plblIF;

	CStaticText* m_plblGSFN;
	CStaticText* m_pGSFN;

	CStaticText* m_plblPixelColor;
	CTextCtrl* m_pPixelColorRGB;
	CTextCtrl* m_pPixelColorLab;
	CStaticText* m_pPixelColorExample;

	wxColour m_PixelColorExample;

	int		m_cn;
	
	int		m_W = 0;
	int		m_H = 0;
	int		m_w = 0;
	int		m_h = 0;
	int		m_xmin = 0;
	int		m_ymin = 0;
	int		m_xmax = 0;
	int		m_ymax = 0;

	//HCURSOR  m_hCursor;

	CSSOWnd		*m_pParent;

	CMainFrame	*m_pMF;

	custom_buffer<simple_buffer<u8>> m_ImF;

	void Init();

public:
	void OnBnClickedTest(wxCommandEvent& event);
	void OnBnClickedLeft(wxCommandEvent& event);
	void OnBnClickedRight(wxCommandEvent& event);
	void ViewCurImF();
	void UpdateSize() override;
	void RefreshData() override;

private:
   DECLARE_EVENT_TABLE()
};


