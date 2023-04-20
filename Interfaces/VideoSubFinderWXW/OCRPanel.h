                              //OCRPanel.h//                                
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
#include "DataTypes.h"
#include "MyResource.h"
#include "DataGrid.h"
#include "StaticBox.h"
#include "CheckBox.h"
#include "TextCtrl.h"
#include "Button.h"
#include <wx/panel.h>
#include <wx/textctrl.h>
#include <vector>

using namespace std;

extern bool g_use_ISA_images_for_get_txt_area;
extern bool g_use_ILA_images_for_get_txt_area;

extern int g_IsCreateClearedTextImages;
extern int g_RunCreateClearedTextImages;
extern int g_IsJoinTXTImages;
extern bool g_ValidateAndCompareTXTImages;
extern bool g_DontDeleteUnrecognizedImages1;
extern bool g_DontDeleteUnrecognizedImages2;
extern wxString g_DefStringForEmptySub;

extern bool g_CLEAN_RGB_IMAGES;
extern int  g_ocr_threads;

extern bool g_join_subs_and_correct_time;
extern bool g_clear_txt_folders;

class FindTextLinesRes;
class CMainFrame;
class CSSOWnd;
class AssTXTStyle;

void FindTextLines(wxString FileName, FindTextLinesRes& res);

class FindTextLinesRes
{
public:
	int m_res = 0;
	int m_w = 0;
	int m_h = 0;
	int	m_W = 0;
	int	m_H = 0;
	int	m_xmin = 0;
	int	m_ymin = 0;
	int	m_xmax = 0;
	int	m_ymax = 0;

	wxString m_SaveDir;
	wxString m_BaseImgName;
	simple_buffer<u8> m_ImBGR;
	simple_buffer<u8> m_ImClearedTextScaled;
	simple_buffer<u8>* m_pImFF = NULL;
	simple_buffer<u8>* m_pImSF = NULL;
	simple_buffer<u8>* m_pImTF = NULL;
	simple_buffer<u8>* m_pImNE = NULL;
	simple_buffer<u8>* m_pImY = NULL;

	FindTextLinesRes(wxString SaveDir)
	{
		m_SaveDir = SaveDir;
	}
};

class AssTXTLine
{
public:
	AssTXTLine();

	AssTXTLine& operator=(const AssTXTLine& other);

	int		m_W;
	int		m_H;

	wxString	m_TXTStr;
	int		m_LH;
	int		m_LY;
	int		m_LXB;
	int		m_LXE;
	int		m_LYB;
	int		m_LYE;

	int		m_mY; // main color in YIQ color system
	int		m_mI;
	int		m_mQ;

	s64		m_BT;
	s64		m_ET;

	int		m_dX; //смещение pos(m_dX, m_dY)
	int		m_dY;

	int		m_Alignment;

	int			 m_AssStyleIndex;
	AssTXTStyle *m_pAssStyle;
};

struct YIQ_LH_Struct
{
	int		m_mY;
	int		m_mI;
	int		m_mQ;

	int		m_LH;
};

class AssTXTStyle
{
public:
	AssTXTStyle();

	vector<YIQ_LH_Struct> m_data;
	
	void Compute(int W, int H);

	int		m_minY;
	int		m_minI;
	int		m_minQ;
	
	int		m_maxY;
	int		m_maxI;
	int		m_maxQ;

	int		m_mY;
	int		m_mI;
	int		m_mQ;

	int		m_minLH;
	int		m_maxLH;

	int		m_LH;

	int		m_Alignment;
	int		m_MarginL;
	int		m_MarginR;
	int		m_MarginV;
	wxString  m_Name;
};

void CreateClearedTextImages();
void JoinImages();

class COCRPanel : public wxPanel, public CControl
{
public:
	COCRPanel(CSSOWnd* pParent);
	~COCRPanel();

	std::mutex m_mutex;

	wxString m_sub_path;

	CStaticBox	*m_pGB;
	CDataGrid	*m_pDG;
	CButton		*m_pCCTI;
	CButton		*m_pCES;
	CButton		*m_pJOIN;
	CButton		*m_pCSCTI;
	CButton		*m_pCSTXT;
	wxPanel		*m_pP3;	

	CSSOWnd		*m_pParent;
	CMainFrame	*m_pMF;

	wxSizerItem* m_pSpacerBNs;
	wxBoxSizer* m_gb_hor_box_sizer;
	wxBoxSizer* m_vert_box_buttons_sizer;

	bool m_was_sub_save = false;

	const int m_dx = 10;

	vector<wxString> m_FileNamesVector;
	wxString m_SaveDir;

	std::thread m_CCTIThread;
	std::thread m_JoinTXTImagesThread;

	void Init();

	void CreateSubFromJoinTXTResults(wxString join_txt_res_path);
	void CreateSubFromTXTResults();

public:
	void OnUpdateCCTIProgress(wxThreadEvent& event);
	void OnBnClickedCreateEmptySub(wxCommandEvent& event);
	void OnBnClickedCreateSubFromClearedTXTImages(wxCommandEvent& event);
	void OnBnClickedCreateSubFromTXTResults(wxCommandEvent& event);
	void OnBnClickedCreateClearedTextImages(wxCommandEvent& event);
	void OnBnClickedJoinTXTImages(wxCommandEvent& event);
	void SaveSub(wxString srt_sub, wxString ass_sub);
	void ThreadCreateClearedTextImagesEnd(wxCommandEvent& event);
	void ThreadJoinTXTImagesThreadEnd(wxCommandEvent& event);
	void UpdateSize() override;
	void RefreshData() override;

private:
	DECLARE_EVENT_TABLE()
};
