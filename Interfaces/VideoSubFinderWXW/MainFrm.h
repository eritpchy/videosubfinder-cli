                              //MainFrm.h//                                
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

#include <wx/wx.h>
#include <wx/dir.h>
#include <wx/mdi.h>
#include <wx/timer.h>
#include <wx/filename.h>
#include "DataTypes.h"
#include "MyResource.h"
#include "SSOWnd.h"
#include "VideoBox.h"
#include "ImageBox.h"
#include "OCVVideoLoader.h"
#include "SSAlgorithms.h"
#include "IPAlgorithms.h"
#include <time.h>
#include <fstream>

using namespace std;

class CSSOWnd;
class CVideoBox;
class CImageBox;

class Settings
{
public:
	wxString	m_prefered_locale;

	int			m_txt_dw;
	int			m_txt_dy;

	int			m_fount_size_ocr_lbl;
	int			m_fount_size_ocr_btn;

	double		m_ocr_min_sub_duration;

	wxString	m_ocr_label_msd_text;
	wxString	m_ocr_button_ces_text;
	wxString	m_ocr_button_ccti_text;
	wxString	m_ocr_button_csftr_text;
	wxString	m_ocr_button_cesfcti_text;
	wxString	m_ocr_button_test_text;
};

class CMainFrame : public wxMDIParentFrame
{
public:
	CMainFrame(const wxString& title);
	~CMainFrame();

public:
	string		m_Dir;

	bool		m_WasInited;

	wxSizer		*m_pSizer;

	CSSOWnd		*m_pPanel;
	CVideoBox	*m_pVideoBox;
	CImageBox	*m_pImageBox;

	CVideo		*m_pVideo;

	bool	    m_VIsOpen;
	//CDocManager m_DocManager;
	wxString	m_FileName;

	string		m_EndTimeStr;

	wxTimer		m_timer;

	enum {Play, Pause, Stop} m_vs;

	int			m_BufferSize;
	int			m_w; //video width
	int			m_h; //video height

	int			m_cw; //client width
	int			m_ch; //client height

	int			m_ph; //panel height

	s64			m_dt;
	s64         m_ct;

	s64			m_BegTime;
	s64			m_EndTime;

	string		m_GeneralSettingsFileName;

	bool		m_blnReopenVideo;

	int			m_type;

	bool		m_blnOpenVideoThreadStateFlag;
	bool		m_blnOpenVideoResult;	

	Settings	m_cfg;

public:
	void Init();

	void PauseVideo();
	void LoadSettings();
	void SaveSettings();
	void OnFileOpenVideo(int type);
	void ClearDir(string DirName);

public:
	void OnSize(wxSizeEvent& event);
	void OnPlayPause(wxCommandEvent& event);
	void OnStop(wxCommandEvent& event);
	void OnFileReOpenVideo(wxCommandEvent& event);
	void OnFileOpenVideoOpenCV(wxCommandEvent& event);
	void OnEditSetBeginTime(wxCommandEvent& event);
	void OnEditSetEndTime(wxCommandEvent& event);
	void OnFileSaveSettings(wxCommandEvent& event);
	void OnFileLoadSettings(wxCommandEvent& event);
	void OnFileSaveSettingsAs(wxCommandEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnQuit(wxCommandEvent& event);
	void OnFileOpenPreviousVideo(wxCommandEvent& event);
	void OnAppAbout(wxCommandEvent& event);
	void OnSetPriorityIdle(wxCommandEvent& event);
	void OnSetPriorityNormal(wxCommandEvent& event);
	void OnSetPriorityBelownormal(wxCommandEvent& event);
	void OnSetPriorityAbovenormal(wxCommandEvent& event);
	void OnSetPriorityHigh(wxCommandEvent& event);

private:
   DECLARE_EVENT_TABLE()
};

s64 GetVideoTime(int minute, int sec, int mili_sec);
string ConvertVideoTime(s64 pos);
string VideoTimeToStr2(s64 pos);
string VideoTimeToStr3(s64 pos);

void WriteProperty(ofstream &fout, int val, string Name);
void WriteProperty(ofstream &fout, bool val, string Name);
void WriteProperty(ofstream &fout, double val, string Name);
void WriteProperty(ofstream &fout, wxString val, string Name);
void ReadProperty(ifstream &fin, int &val, string Name);
void ReadProperty(ifstream &fin, bool &val, string Name);
void ReadProperty(ifstream &fin, double &val, string Name);
void ReadProperty(ifstream &fin, wxString &val, string Name);

bool IsMMX_and_SSE();
bool IsSSE2();

void LoadToolBarImage(wxBitmap& bmp, const wxString& path, const wxColor& BColor);