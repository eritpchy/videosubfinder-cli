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

	vector<wxString> m_SavedFiles;
	simple_buffer<u8> m_ImBGR;
	simple_buffer<u8> m_ImClearedText;
	simple_buffer<u8>* m_pImFF = NULL;
	simple_buffer<u8>* m_pImSF = NULL;
	simple_buffer<u8>* m_pImTF = NULL;
	simple_buffer<u8>* m_pImNE = NULL;
	simple_buffer<u8>* m_pImY = NULL;

	FindTextLinesRes()
	{
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

class ThreadCreateClearedTextImages : public wxThread
{
public:
    ThreadCreateClearedTextImages(CMainFrame *pMF, vector<wxString> &FileNamesVector, wxThreadKind kind = wxTHREAD_DETACHED);

    virtual void *Entry();

public:
    CMainFrame	*m_pMF;
	vector<wxString> m_FileNamesVector;
};

class COCRPanel : public wxPanel
{
public:
	COCRPanel(CSSOWnd* pParent);
	~COCRPanel();

	std::mutex m_mutex;

	wxString m_sub_path;

	CStaticText *m_plblMSD;
	CStaticText	*m_plblJTXTSL;
	CCheckBox	*m_pcbJSACT;
	CCheckBox	*m_pcbCTXTF;
	CCheckBox	*m_pcbSESS;
	CCheckBox	*m_pcbSSI;
	CTextCtrl	*m_pMSD;
	CTextCtrl	*m_pJTXTSL;
	CButton	 *m_pCCTI;
	CButton	 *m_pCES;
	CButton  *m_pJOIN;
	CButton	 *m_pCSCTI;
	CButton	 *m_pCSTXT;
	wxPanel		 *m_pP3;

	wxColour   m_CLOCR;
	wxColour   m_CL1;

	CSSOWnd		*m_pParent;
	CMainFrame	*m_pMF;

	ThreadCreateClearedTextImages *m_pSearchThread;

	void Init();

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

private:
	DECLARE_EVENT_TABLE()
};
