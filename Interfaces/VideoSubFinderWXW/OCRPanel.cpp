                              //OCRPanel.cpp//                                
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

#include "OCRPanel.h"

bool g_use_FRD_images = false;

int g_IsCreateClearedTextImages = 0;
int g_RunCreateClearedTextImages = 0;
bool g_ValidateAndCompareTXTImages = false;
bool g_DontDeleteUnrecognizedImages1 = false;
bool g_DontDeleteUnrecognizedImages2 = true;

wxString g_DefStringForEmptySub = "sub duration: %sub_duration%";

bool g_CLEAN_RGB_IMAGES = false;

AssTXTLine::AssTXTLine()
{
	m_TXTStr = string("");
	m_LH = 0;
	m_LY = 0;
	m_LXB = 0;
	m_LXE = 0;
	m_LYB = 0;
	m_LYE = 0;

	m_mY = 0;
	m_mI = 0;
	m_mQ = 0;

	m_BT = 0;
	m_ET = 0;

	m_pAssStyle = NULL;

	m_dX = -1;
	m_dY = -1;
	m_Alignment = -1;
}

AssTXTLine& AssTXTLine::operator=(const AssTXTLine &other)
{
	m_TXTStr = other.m_TXTStr;
	m_LH = other.m_LH;
	m_LY = other.m_LY;
	m_LXB = other.m_LXB;
	m_LXE = other.m_LXE;
	m_LYB = other.m_LYB;
	m_LYE = other.m_LYE;

	m_mY = other.m_mY;
	m_mI = other.m_mI;
	m_mQ = other.m_mQ;

	m_BT = other.m_BT;
	m_ET = other.m_ET;

	m_pAssStyle = other.m_pAssStyle;

	m_dX = other.m_dX;
	m_dY = other.m_dY;
	m_Alignment = other.m_Alignment;

	return *this;
}

AssTXTStyle::AssTXTStyle()
{	
	m_data.clear();

	m_minY = 0;
	m_minI = 0;
	m_minQ = 0;
	
	m_maxY = 0;
	m_maxI = 0;
	m_maxQ = 0;

	m_mY = 0;
	m_mI = 0;
	m_mQ = 0;

	m_minLH = 0;
	m_maxLH = 0;

	m_LH = 0;

	m_Alignment = 2;
	m_MarginL = 10;
	m_MarginR = 10;
	m_MarginV = 10;

	m_Name = string("");
}

void AssTXTStyle::Compute()
{
	int i;
	int size, val1, val2, val3, val4;

	val1 = 0;
	val2 = 0;
	val3 = 0;
	val4 = 0;

	size = (int)m_data.size();

	for (i=0; i<size; i++)
	{
		val1 += m_data[i].m_mY;
		val2 += m_data[i].m_mI;
		val3 += m_data[i].m_mQ;
		val4 += m_data[i].m_LH;
	}
	

	m_mY = val1/size;
	m_mI = val2/size;
	m_mQ = val3/size;
	m_LH = (val4*528*100)/(size*g_H*53);
	m_LH += m_LH%2;
}

BEGIN_EVENT_TABLE(COCRPanel, wxPanel)
	EVT_BUTTON(ID_BTN_CES, COCRPanel::OnBnClickedCreateEmptySub)
	EVT_BUTTON(ID_BTN_CSCTI, COCRPanel::OnBnClickedCreateSubFromClearedTXTImages)
	EVT_BUTTON(ID_BTN_CSTXT, COCRPanel::OnBnClickedCreateSubFromTXTResults)
	EVT_BUTTON(ID_BTN_CCTI, COCRPanel::OnBnClickedCreateClearedTextImages)
	EVT_BUTTON(ID_BTN_TEST, COCRPanel::OnBnClickedTest)
END_EVENT_TABLE()

COCRPanel::COCRPanel(CSSOWnd* pParent)
		:wxPanel( pParent, wxID_ANY )
{
	m_pParent = pParent;
	m_pMF = pParent->m_pMF;
}

COCRPanel::~COCRPanel()
{
}

void COCRPanel::Init()
{
	m_CL1 = wxColour(255, 215, 0);
	m_CLOCR = wxColour(170, 170, 170);

	//"Times New Roman"
	m_BTNFont = wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_BOLD, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);

	//"Microsoft Sans Serif"
	m_LBLFont = wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                    wxFONTWEIGHT_NORMAL, false /* !underlined */,
                    wxEmptyString /* facename */, wxFONTENCODING_DEFAULT);


	wxRect rcCCTI, rcCES, rcP3, rcClP3, rlMSD, reMSD, rcTEST, rcCSCTI, rcCSTXT;
	int w, w2, h, dw, dh;

	w2 = 700;
	w = 400;
	h = 25;

	rcCCTI.x = w2/2 - w/2;
	rcCCTI.y = 20;
	rcCCTI.width = w;
	rcCCTI.height = h;

	rcCSTXT.x = rcCCTI.x;
	rcCSTXT.y = rcCCTI.GetBottom() + 10;
	rcCSTXT.width = w;
	rcCSTXT.height = h;

	rcCSCTI.x = rcCCTI.x;
	rcCSCTI.y = rcCSTXT.GetBottom() + 10;
	rcCSCTI.width = w;
	rcCSCTI.height = h;

	rcCES.x = rcCCTI.x;
	rcCES.y = rcCSCTI.GetBottom() + 10;
	rcCES.width = w;
	rcCES.height = h;

	rcTEST.x = rcCCTI.GetRight() + 30;
	rcTEST.y = rcCCTI.GetBottom() + 5 - h/2;
	rcTEST.width = 100;
	rcTEST.height = h;

	rlMSD.x = 20;
	rlMSD.y = 20;
	rlMSD.width = rcCCTI.GetLeft() - rlMSD.x*2;
	rlMSD.height = 18;

	reMSD.x = rlMSD.x;
	reMSD.y = rlMSD.GetBottom() + 10;
	reMSD.width = rlMSD.width;
	reMSD.height = 18;

	rcP3 = this->GetRect();

	this->GetClientSize(&w, &h);
	rcClP3.x = rcClP3.y = 0; 
	rcClP3.width = w;
	rcClP3.height = h;

	dw = rcP3.width - rcClP3.width;
	dh = rcP3.height - rcClP3.height;

	rcP3.x = 10;	
	rcP3.y = 10;
	rcP3.width = w2 + dw;
	rcP3.height = rcCES.GetBottom() + 20 + dh;

	this->SetSize(rcP3);	

	m_pP3 = new wxPanel( this, wxID_ANY, rcP3.GetPosition(), rcP3.GetSize() );
	m_pP3->SetMinSize(rcP3.GetSize());
	m_pP3->SetBackgroundColour( m_CLOCR );

	m_plblMSD = new wxStaticText( m_pP3, wxID_ANY,
		wxT(" Min Sub Duration:"), rlMSD.GetPosition(), rlMSD.GetSize(), wxALIGN_LEFT | wxST_NO_AUTORESIZE | wxBORDER );
	m_plblMSD->SetFont(m_LBLFont);
	m_plblMSD->SetBackgroundColour( m_CL1 );

	m_pMSD = new wxTextCtrl( m_pP3, wxID_ANY,
		wxT("0.000"), reMSD.GetPosition(), reMSD.GetSize());
	m_pMSD->SetFont(m_LBLFont);

	m_pCES = new wxButton( m_pP3, ID_BTN_CES,
		wxT("Create Empty Sub"), rcCES.GetPosition(), rcCES.GetSize() );
	m_pCES->SetFont(m_BTNFont);

	m_pCCTI = new wxButton( m_pP3, ID_BTN_CCTI,
		wxT("Create Cleared TXT Images"), rcCCTI.GetPosition(), rcCCTI.GetSize() );
	m_pCCTI->SetFont(m_BTNFont);

	m_pCSTXT = new wxButton( m_pP3, ID_BTN_CSTXT,
		wxT("Create Sub From TXT Results"), rcCSTXT.GetPosition(), rcCSTXT.GetSize() );
	m_pCSTXT->SetFont(m_BTNFont);

	m_pCSCTI = new wxButton( m_pP3, ID_BTN_CSCTI,
		wxT("Create Empty Sub From Cleared TXT Images"), rcCSCTI.GetPosition(), rcCSCTI.GetSize() );
	m_pCSCTI->SetFont(m_BTNFont);

	m_pTEST = new wxButton( m_pP3, ID_BTN_TEST,
		wxT("Test"), rcTEST.GetPosition(), rcTEST.GetSize() );
	m_pTEST->SetFont(m_BTNFont);

	wxBoxSizer *top_sizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer *button_sizer = new wxBoxSizer( wxHORIZONTAL );

	button_sizer->Add(m_pP3, 1, wxALIGN_CENTER, 0 );

	top_sizer->Add(button_sizer, 1, wxALIGN_CENTER );

	this->SetSizer(top_sizer);

	m_pTEST->SetFocus();
}

void COCRPanel::OnBnClickedCreateEmptySub(wxCommandEvent& event)
{
	wxString Str, SubStr, hour1, hour2, min1, min2, sec1, sec2, msec1, msec2;
	int i, j, k, sec, msec;
	fstream fout;
	u64 bt, et, dt, mdt;
	char str[30];

	wxString dir_path = wxString(m_pMF->m_Dir + string("/RGBImages/"));
	wxDir dir(dir_path);
	vector<wxString> FileNamesVector;
	vector<u64> BT, ET;
	wxString filename;
	bool res;

	res = dir.GetFirst(&filename, "*.jpeg");
    while ( res )
    {
		FileNamesVector.push_back(filename);

        res = dir.GetNext(&filename);
    }
	
	for (i=0; i<(int)FileNamesVector.size()-1; i++)
	for (j=i+1; j<(int)FileNamesVector.size(); j++)
	{
		if (FileNamesVector[i] > FileNamesVector[j])
		{
			Str = FileNamesVector[i];
			FileNamesVector[i] = FileNamesVector[j];
			FileNamesVector[j] = Str;
		}
	}

	fout.open(string(m_pMF->m_Dir+string("/sub.srt")).c_str(), ios::out);

	Str = m_pMSD->GetValue();
	mdt = (s64)atof(Str)*1000;

	for(k=0; k<(int)FileNamesVector.size(); k++)
	{
		Str = FileNamesVector[k];

		hour1 = Str.Mid(0,1);
		min1 = Str.Mid(2,2);
		sec1 = Str.Mid(5,2);
		msec1 = Str.Mid(8,3);

		hour2 = Str.Mid(13,1);
		min2 = Str.Mid(15,2);
		sec2 = Str.Mid(18,2);
		msec2 = Str.Mid(21,3);

		bt = (atoi(hour1)*3600 + atoi(min1)*60 + atoi(sec1))*1000 + atoi(msec1);
		et = (atoi(hour2)*3600 + atoi(min2)*60 + atoi(sec2))*1000 + atoi(msec2);

		BT.push_back(bt);
		ET.push_back(et);
	}

	for(k=0; k<(int)FileNamesVector.size()-1; k++)
	{
		if (ET[k]-BT[k] < mdt)
		{
			if (BT[k]+mdt < BT[k+1]) 
			{
				ET[k] = BT[k]+mdt;
			}
			else
			{
				ET[k] = BT[k+1]-1;
			}
		}
	}

	for(k=0; k<(int)FileNamesVector.size(); k++)
	{
		bt = BT[k];
		et = ET[k];

		Str = VideoTimeToStr2(bt*(u64)10000)+
			  " --> "+
			  VideoTimeToStr2(et*(u64)10000);

		dt = et - bt;
		sec = (int)(dt/1000);
		msec = (int)(dt%1000);
		
		_itoa(sec, str, 10);
		sec1 = str;

		_itoa(msec, str, 10);
		if (msec < 10) msec1 = wxString("00")+str; 
		else
		{
			if (msec < 100) msec1 = wxString("0")+str; 
			else msec1 = str; 
		}

		SubStr = g_DefStringForEmptySub;

		if (g_DefStringForEmptySub.Contains("%sub_duration%"))
		{			
			SubStr.Replace("%sub_duration%", sec1 + "," + msec1);
		}

		fout << (k+1) << "\n" << Str << "\n" << SubStr << "\n\n";
	}

	fout.close();
}

void COCRPanel::OnBnClickedCreateSubFromTXTResults(wxCommandEvent& event)
{
	CreateSubFromTXTResults();
}

void COCRPanel::OnBnClickedCreateSubFromClearedTXTImages(wxCommandEvent& event)
{
	wxString Str, SubStr, Name, hour1, hour2, min1, min2, sec1, sec2, msec1, msec2;
	int i, j, k, kb, sec, msec;
	fstream fout;
	char str[30];
	u64 bt, et, dt, mdt;

	wxString dir_path = wxString(m_pMF->m_Dir + string("/TXTImages/"));
	wxDir dir(dir_path);
	vector<wxString> FileNamesVector;
	vector<u64> BT, ET;
	wxString filename;
	bool res;

	res = dir.GetFirst(&filename, "*.jpeg");
    while ( res )
    {
		FileNamesVector.push_back(filename);

        res = dir.GetNext(&filename);
    }

	for (i=0; i<(int)FileNamesVector.size()-1; i++)
	for (j=i+1; j<(int)FileNamesVector.size(); j++)
	{
		if (FileNamesVector[i] > FileNamesVector[j])
		{
			Str = FileNamesVector[i];
			FileNamesVector[i] = FileNamesVector[j];
			FileNamesVector[j] = Str;
		}
	}

	Str = m_pMSD->GetValue();
	mdt = (s64)atof(Str)*1000;

	k = 0;
	while (k < (int)FileNamesVector.size())
	{
		kb = k;
		i = 0;
		while( (k < (int)FileNamesVector.size()) &&
			   (FileNamesVector[kb].Mid(0, 11) == FileNamesVector[k].Mid(0, 11))
			 )
		{
			k++;
		}

		Str = FileNamesVector[kb];

		hour1 = Str.Mid(0,1);
		min1 = Str.Mid(2,2);
		sec1 = Str.Mid(5,2);
		msec1 = Str.Mid(8,3);

		hour2 = Str.Mid(13,1);
		min2 = Str.Mid(15,2);
		sec2 = Str.Mid(18,2);
		msec2 = Str.Mid(21,3);

		bt = (atoi(hour1)*3600 + atoi(min1)*60 + atoi(sec1))*1000 + atoi(msec1);
		et = (atoi(hour2)*3600 + atoi(min2)*60 + atoi(sec2))*1000 + atoi(msec2);

		BT.push_back(bt);
		ET.push_back(et);
	}

	for(k=0; k<(int)BT.size()-1; k++)
	{
		if (ET[k]-BT[k] < mdt)
		{
			if (BT[k]+mdt < BT[k+1]) 
			{
				ET[k] = BT[k]+mdt;
			}
			else
			{
				ET[k] = BT[k+1]-1;
			}
		}
	}

	fout.open(string(m_pMF->m_Dir+"/sub.srt").c_str(), ios::out);

	for(k=0; k<(int)BT.size(); k++)
	{
		bt = BT[k];
		et = ET[k];

		Str = VideoTimeToStr2(bt*(u64)10000)+
			  " --> "+
			  VideoTimeToStr2(et*(u64)10000);

		dt = et - bt;
		sec = (int)(dt/1000);
		msec = (int)(dt%1000);
		
		_itoa(sec, str, 10);
		sec1 = str;

		_itoa(msec, str, 10);
		if (msec < 10) msec1 = wxString("00")+str; 
		else
		{
			if (msec < 100) msec1 = wxString("0")+str; 
			else msec1 = str; 
		}

		SubStr = g_DefStringForEmptySub;

		if (g_DefStringForEmptySub.Contains("%sub_duration%"))
		{			
			SubStr.Replace("%sub_duration%", sec1 + "," + msec1);
		}

		fout << (k+1) << "\n" << Str << "\n" << SubStr << "\n\n";
	}

	fout.close();
}

void COCRPanel::CreateSubFromTXTResults()
{
	wxString Str, Name, hour1, hour2, min1, min2, sec1, sec2, msec1, msec2;
	int i, j, k, kb, sec, msec, max_mY_dif, max_mI_dif, max_mQ_dif, max_posY_dif;
	int val1, val2, val3, val4, val5, val6, val7, val8;
	string fname, image_name;
	fstream fout, txt_info;
	u64 bt, et, dt, mdt;
	char str[1000];
	double max_LH_dif;
	int bln;

	vector<wxString> FileNamesVector;
	vector<wxString> TXTVector;
	AssTXTLine *AssTXTVector;
	AssTXTStyle *AssTXTStyleVector; 
	vector<u64> BT, ET;
	AssTXTLine AssLine;
	AssTXTStyle AssStyle;
	YIQ_LH_Struct AssStyleDatum;
	int mR, mG, mB, NT, NS;
	string BaseStyleName;
	
	BaseStyleName = string("Base");
	max_mY_dif = 16;
	max_mI_dif = 10;
	max_mQ_dif = 10;
	max_posY_dif = 5;
	max_LH_dif = 0.20;

	wxString dir_path = wxString(m_pMF->m_Dir + string("/TXTResults/"));
	wxDir dir(dir_path);
	wxString filename;
	bool res;

	res = dir.GetFirst(&filename, "*.txt");
    while ( res )
    {
		FileNamesVector.push_back(filename);

        res = dir.GetNext(&filename);
    }

	for (i=0; i<(int)FileNamesVector.size()-1; i++)
	for (j=i+1; j<(int)FileNamesVector.size(); j++)
	{
		if (FileNamesVector[i] > FileNamesVector[j])
		{
			Str = FileNamesVector[i];
			FileNamesVector[i] = FileNamesVector[j];
			FileNamesVector[j] = Str;
		}
	}

	Str = m_pMSD->GetValue();
	mdt = (s64)atof(Str)*1000;

	str[0] = '\0';

	fname = g_dir + string("/text_lines.info");
	txt_info.open(fname.c_str(), ios::in);

	NT = 0;
	AssTXTVector = new AssTXTLine[(int)FileNamesVector.size()];

	NS = 0;
	AssTXTStyleVector = new AssTXTStyle[(int)FileNamesVector.size()];	
	
    //--------------

    image_name = g_dir+string("/RGBImages/")+string(FileNamesVector[0]).substr(0, 24) + string(".jpeg");
    GetImageSize(image_name, g_W, g_H);    
	InitIPData(g_W, g_H, 1);

    //--------------
    
	k = 0;
	while (k < (int)FileNamesVector.size())
	{
		kb = k;

		Str = FileNamesVector[kb];

		hour1 = Str.Mid(0,1);
		min1 = Str.Mid(2,2);
		sec1 = Str.Mid(5,2);
		msec1 = Str.Mid(8,3);

		hour2 = Str.Mid(13,1);
		min2 = Str.Mid(15,2);
		sec2 = Str.Mid(18,2);
		msec2 = Str.Mid(21,3);

		bt = (atoi(hour1)*3600 + atoi(min1)*60 + atoi(sec1))*1000 + atoi(msec1);
		et = (atoi(hour2)*3600 + atoi(min2)*60 + atoi(sec2))*1000 + atoi(msec2);

		BT.push_back(bt);
		ET.push_back(et);

		Str = "";
		i = 0;
		while( (k < (int)FileNamesVector.size()) &&
			   (FileNamesVector[kb].Mid(0, 11) == FileNamesVector[k].Mid(0, 11))
			 )
		{
			Name = m_pMF->m_Dir+"/TXTResults/"+FileNamesVector[k];

			FILE *fin = fopen(Name, "r");
			
			while (!feof(fin))
			{
				str[0] = '\0';
				fgets(str, 1000, fin);
				if (str[0] != '\0')
				{
					if (i > 0) Str += "\n";
					Str += str;
					i++;
				}
			}

			fclose(fin);

			AssLine.m_TXTStr = string(str);
			AssLine.m_BT = bt;
			AssLine.m_ET = et;			

			image_name = string("/TXTImages/") + string(FileNamesVector[k]).substr(0, 27) + string(".jpeg");
			
            fname = string("");
			do
			{
                if ( txt_info.eof() )                
                {
                    break;
                }

				txt_info >> fname; // file name
				
				txt_info >> str; // "="
				txt_info >> str; // "YB"
				txt_info >> str; // YB value
				txt_info >> str; // "LH"
				txt_info >>	AssLine.m_LH;
				txt_info >> str; // "LY"
				txt_info >>	AssLine.m_LY;
				txt_info >> str; // "LXB"
				txt_info >>	AssLine.m_LXB;
				txt_info >> str; // "LXE"
				txt_info >>	AssLine.m_LXE;
				txt_info >> str; // "LYB"
				txt_info >>	AssLine.m_LYB;
				txt_info >> str; // "LYE"
				txt_info >>	AssLine.m_LYE;
				txt_info >> str; // "YIQ"
				txt_info >>	AssLine.m_mY;
				txt_info >>	AssLine.m_mI;
				txt_info >>	AssLine.m_mQ;
			}
			while (fname != image_name);
			
			if (fname != image_name)
			{
				txt_info.close();
				Str = wxString("There is not info about \"") + wxString(image_name.c_str()) 
                    + wxString("\" in \"text_lines.info\" file.\n") 
                    + wxString("Please run \"Create Cleared Text Images\" again.");

				#ifdef WIN32
				::MessageBox(NULL, Str, "CreateSubFromTXTResults", MB_ICONERROR);			
				#endif

				return;
			}

			AssTXTVector[NT] = AssLine;
			NT++;

			k++;
		}

		TXTVector.push_back(Str);
	}

	txt_info.close();

	// סמחהאול srt subtitle

	k=0;
	while(k < (int)TXTVector.size()-1)
	{
		if (TXTVector[k] == "")
		{
			if (g_DontDeleteUnrecognizedImages2 == false)
			{
				for(i=k; i<(int)TXTVector.size()-1; i++)
				{
					BT[i] = BT[i+1];
					ET[i] = ET[i+1];
					TXTVector[i] = TXTVector[i+1];
				}
				BT.pop_back();
				ET.pop_back();
				TXTVector.pop_back();

				continue;
			}
			else
			{
				TXTVector[k] = wxString("#unrecognized text#");
			}
		}

		if (BT[k+1]-ET[k] <= 333)
		{
			if (TXTVector[k+1] == TXTVector[k])
			{
				ET[k] = ET[k+1];

				for(i=k+1; i<(int)TXTVector.size()-1; i++)
				{
					BT[i] = BT[i+1];
					ET[i] = ET[i+1];
					TXTVector[i] = TXTVector[i+1];
				}
				BT.pop_back();
				ET.pop_back();
				TXTVector.pop_back();

				continue;
			}
		}

		k++;
	}

	for(k=0; k<(int)TXTVector.size()-1; k++)
	{
		if (ET[k]-BT[k] < mdt)
		{
			if (BT[k]+mdt < BT[k+1]) 
			{
				ET[k] = BT[k]+mdt;
			}
			else
			{
				ET[k] = BT[k+1]-1;
			}
		}
	}

	fout.open(string(m_pMF->m_Dir+"/sub.srt").c_str(), ios::out);

	for(k=0; k<(int)TXTVector.size(); k++)
	{
		bt = BT[k];
		et = ET[k];

		Str = VideoTimeToStr2(bt*(u64)10000)+
			  " --> "+
			  VideoTimeToStr2(et*(u64)10000);

		dt = et - bt;
		sec = (int)(dt/1000);
		msec = (int)(dt%1000);
		
		_itoa(sec, str, 10);
		sec1 = str;

		_itoa(msec, str, 10);
		if (msec < 10) msec1 = wxString("00")+str; 
		else
		{
			if (msec < 100) msec1 = wxString("0")+str; 
			else msec1 = str; 
		}

		fout << (k+1) << "\n" << Str << "\n" << TXTVector[k] << "\n\n";
	}

	fout.close();

	// סמחהאול ass subtitle

	for(i=0; i < NT; i++)
	{
		if (AssTXTVector[i].m_TXTStr != string(""))
		{
			if (AssTXTVector[i].m_LYE - AssTXTVector[i].m_LY <= g_dmaxy/4)
			{
				AssTXTVector[i].m_LYE = max(AssTXTVector[i].m_LYE, AssTXTVector[i].m_LY + (AssTXTVector[i].m_LH*7)/22);
			}
		}
	}

	for(i=0; i < NT-1; i++)
	{
		if (AssTXTVector[i+1].m_BT == AssTXTVector[i].m_BT)
		{
			val1 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2 - g_H/2;
			val2 = AssTXTVector[i+1].m_LY - AssTXTVector[i+1].m_LH/2 - g_H/2;

			if ( (val1 >= AssTXTVector[i].m_LH/2) &&
				 (val2 >= AssTXTVector[i+1].m_LH/2) )
			{
				AssLine = AssTXTVector[i+1];
				AssTXTVector[i+1] = AssTXTVector[i];
				AssTXTVector[i] = AssLine;
			}
		}
	}

	k = 0;
	while(k < NT)
	{
		if (AssTXTVector[k].m_TXTStr == string(""))
		{
			if (g_DontDeleteUnrecognizedImages2 == false)
			{
				for(i=k; i<NT-1; i++)
				{
					AssTXTVector[i] = AssTXTVector[i+1];
				}
				NT--;

				continue;
			}
			else
			{
				AssTXTVector[k].m_TXTStr = string("#unrecognized text#");
				AssTXTVector[k].m_LH = 14;
				AssTXTVector[k].m_mY = 0;
				AssTXTVector[k].m_mI = 0;
				AssTXTVector[k].m_mQ = 0;
				/*AssTXTVector[k].m_LXB = AssTXTVector[k].m_LXE = g_W/2;
				AssTXTVector[k].m_LY = AssTXTVector[k].m_LYE = g_H - 20;				
				AssTXTVector[k].m_LYB = AssTXTVector[k].m_LYE - AssTXTVector[k].m_LH + 1;
				*/
			}
		}

		j = k+1;

		while ( (j < NT) &&
			    (AssTXTVector[j].m_BT-AssTXTVector[k].m_ET <= 333) )
		{
			if ( (AssTXTVector[j].m_TXTStr == AssTXTVector[k].m_TXTStr) &&				
				 (abs(AssTXTVector[j].m_LY - AssTXTVector[k].m_LY) <= max_posY_dif) &&
				 (abs(AssTXTVector[j].m_mY - AssTXTVector[k].m_mY) <= max_mY_dif) &&
				 (abs(AssTXTVector[j].m_mI - AssTXTVector[k].m_mI) <= max_mI_dif) &&
				 (abs(AssTXTVector[j].m_mQ - AssTXTVector[k].m_mQ) <= max_mQ_dif) &&
				 ((double)abs(AssTXTVector[j].m_LH - AssTXTVector[k].m_LH)/(double)AssTXTVector[k].m_LH <= max_LH_dif) )
			{
				AssTXTVector[k].m_ET = AssTXTVector[j].m_ET;

				for(i=j; i<NT-1; i++)
				{
					AssTXTVector[i] = AssTXTVector[i+1];
				}
				NT--;

				continue;
			}
			else
			{
				j++;
			}
		}

		k++;
	}

	for(k=0; k<NT; k++)
	{
		if (AssTXTVector[k].m_ET-AssTXTVector[k].m_BT < (s64)mdt)
		{
			AssTXTVector[k].m_ET = AssTXTVector[k].m_BT + mdt;
		}
	}

	for(i=0; i<NT; i++)
	{
		bln = 0;

		for(j=0; j<NS; j++)
		{
			val1 = AssTXTStyleVector[j].m_minY;
			val2 = AssTXTStyleVector[j].m_maxY;

			if (AssTXTVector[i].m_mY < val1) val1 = AssTXTVector[i].m_mY;
			if (AssTXTVector[i].m_mY > val2) val2 = AssTXTVector[i].m_mY;

			val3 = AssTXTStyleVector[j].m_minI;
			val4 = AssTXTStyleVector[j].m_maxI;

			if (AssTXTVector[i].m_mI < val3) val3 = AssTXTVector[i].m_mI;
			if (AssTXTVector[i].m_mI > val4) val4 = AssTXTVector[i].m_mI;

			val5 = AssTXTStyleVector[j].m_minQ;
			val6 = AssTXTStyleVector[j].m_maxQ;

			if (AssTXTVector[i].m_mQ < val5) val5 = AssTXTVector[i].m_mQ;
			if (AssTXTVector[i].m_mQ > val6) val6 = AssTXTVector[i].m_mQ;

			val7 = AssTXTStyleVector[j].m_minLH;
			val8 = AssTXTStyleVector[j].m_maxLH;

			if (AssTXTVector[i].m_LH < val7) val7 = AssTXTVector[i].m_LH;
			if (AssTXTVector[i].m_LH > val8) val8 = AssTXTVector[i].m_LH;

			if ( ((val2 - val1) <= max_mY_dif) &&
			     ((val4 - val3) <= max_mI_dif) &&
			     ((val6 - val5) <= max_mQ_dif) &&
			     ((double)(val8 - val7)/(double)val7 <= max_LH_dif) )
			{
				bln = 1;

				AssTXTStyleVector[j].m_minY = val1;
				AssTXTStyleVector[j].m_maxY = val2;

				AssTXTStyleVector[j].m_minI = val3;
				AssTXTStyleVector[j].m_maxI = val4;

				AssTXTStyleVector[j].m_minQ = val5;
				AssTXTStyleVector[j].m_maxQ = val6;

				AssTXTStyleVector[j].m_minLH = val7;
				AssTXTStyleVector[j].m_maxLH = val8;

				AssStyleDatum.m_mY = AssTXTVector[i].m_mY;
				AssStyleDatum.m_mI = AssTXTVector[i].m_mI;
				AssStyleDatum.m_mQ = AssTXTVector[i].m_mQ;
				AssStyleDatum.m_LH = AssTXTVector[i].m_LH;

				AssTXTStyleVector[j].m_data.push_back(AssStyleDatum);

				AssTXTVector[i].m_AssStyleIndex = j;
				//AssTXTVector[i].m_pAssStyle = &(AssTXTStyleVector[j]);
			}

			if (bln == 1)
			{
				break;
			}
		}

		if (bln == 0)
		{
			AssStyleDatum.m_mY = AssTXTVector[i].m_mY;
			AssStyleDatum.m_mI = AssTXTVector[i].m_mI;
			AssStyleDatum.m_mQ = AssTXTVector[i].m_mQ;
			AssStyleDatum.m_LH = AssTXTVector[i].m_LH;

			AssStyle.m_data.clear();
			AssStyle.m_data.push_back(AssStyleDatum);

			AssStyle.m_minY = AssStyleDatum.m_mY;
			AssStyle.m_maxY = AssStyleDatum.m_mY;

			AssStyle.m_minI = AssStyleDatum.m_mI;
			AssStyle.m_maxI = AssStyleDatum.m_mI;

			AssStyle.m_minQ = AssStyleDatum.m_mQ;
			AssStyle.m_maxQ = AssStyleDatum.m_mQ;

			AssStyle.m_minLH = AssStyleDatum.m_LH;
			AssStyle.m_maxLH = AssStyleDatum.m_LH;

			val1 = (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2;
			val2 = val1 - g_W/2;

			AssStyle.m_Alignment = 2;

			if ((double)abs(val2)/(g_W/2) < 0.3)
			{
				AssStyle.m_MarginL = -1;
				AssStyle.m_MarginR = -1;
			}
			else
			{
				if (val2 < 0)
				{
					val3 = AssTXTVector[i].m_LXB - AssTXTVector[i].m_LH/10;
					if (val3 < 0) val3 = 0;
					
					AssStyle.m_MarginL = val3;
					AssStyle.m_MarginR = val3;
				}
				else
				{
					val3 = g_W-(AssTXTVector[i].m_LXE + AssTXTVector[i].m_LH/10);
					if (val3 < 0) val3 = 0;

					AssStyle.m_MarginL = val3;
					AssStyle.m_MarginR = val3;
				}
			}

			val1 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2;
			val2 = val1 - g_H/2;

			if (abs(val2) < AssTXTVector[i].m_LH/2)
			{
				AssStyle.m_Alignment += 3;

				AssStyle.m_MarginV = 0;
			}
			else
			{
				if (val2 < 0)
				{
					AssStyle.m_Alignment += 6;

					val3 = AssTXTVector[i].m_LYB;

					if (val3 > 0)
					{
						AssStyle.m_MarginV = val3;
					}
					else
					{
						AssStyle.m_MarginV = 0;
					}
				}
				else
				{
					val3 = g_H - AssTXTVector[i].m_LYE;

					if (val3 > 0)
					{
						AssStyle.m_MarginV = val3;
					}
					else
					{
						AssStyle.m_MarginV = 0;
					}
				}
			}

			sprintf(str, "%.2d", NS+1);
			AssStyle.m_Name = BaseStyleName + string(str);

			AssTXTStyleVector[NS] = AssStyle;
			NS++;

			AssTXTVector[i].m_AssStyleIndex = NS-1;
			//AssTXTVector[i].m_pAssStyle = &(AssTXTStyleVector[AssTXTVector[i].m_AssStyleIndex]);
		}
	}
	
	for(i=0; i<NS; i++)
	{
		AssTXTStyleVector[i].Compute();
	}

	for(i=0; i<NT; i++)
	{
		AssTXTVector[i].m_pAssStyle = &(AssTXTStyleVector[AssTXTVector[i].m_AssStyleIndex]);
	}

	for(i=0; i<NT; i++)
	{
		AssTXTVector[i].m_dX = -1;
		AssTXTVector[i].m_dY = -1;
		AssTXTVector[i].m_Alignment = -1;

		int ho = (AssTXTVector[i].m_pAssStyle->m_Alignment - 1)%3 + 1;
		int vo = (AssTXTVector[i].m_pAssStyle->m_Alignment - 1)/3 + 1;

		val1 = (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2;
		val2 = val1 - g_W/2;

		val3 = AssTXTVector[i].m_LXB - AssTXTVector[i].m_LH/10;
		if (val3 < 0) val3 = 0;

		if (abs(val2) >= AssTXTVector[i].m_LH*2)
		{
			if (AssTXTVector[i].m_pAssStyle->m_MarginL == -1)
			{
				AssTXTVector[i].m_pAssStyle->m_MarginL = val3;
				AssTXTVector[i].m_pAssStyle->m_MarginR = val3;				
			}
			
			if ( abs(AssTXTVector[i].m_pAssStyle->m_MarginL - val3) < AssTXTVector[i].m_LH/2 )
			{
				if (val2 < 0)
				{
					AssTXTVector[i].m_Alignment = 1;
				}
				else
				{
					AssTXTVector[i].m_Alignment = 3;
				}
			}
			else
			{
				val1 = (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2;
				val2 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2;

				AssTXTVector[i].m_Alignment = 5;
				AssTXTVector[i].m_dX = val1;
				AssTXTVector[i].m_dY = val2;

				continue;
			}
		}

		val1 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2;
		val2 = val1 - g_H/2;

		if ((double)abs(val2)/(g_H/2) < 0.05)
		{
			if (vo != 2)
			{
				if (AssTXTVector[i].m_Alignment == -1)
				{
					AssTXTVector[i].m_Alignment = 5;
				}
				else
				{
					AssTXTVector[i].m_Alignment += 3;
				}
			}
		}
		else
		{
			if (val2 < 0)
			{
				val3 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH - AssTXTVector[i].m_LH/5;

				if ( abs(AssTXTVector[i].m_pAssStyle->m_MarginV - val3) < AssTXTVector[i].m_LH/2 )
				{
					if (vo != 3)
					{
						if (AssTXTVector[i].m_Alignment == -1)
						{
							AssTXTVector[i].m_Alignment = 8;
						}
						else
						{
							AssTXTVector[i].m_Alignment += 6;
						}
					}
				}
				else
				{
					val1 = (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2;
					val2 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2;

					AssTXTVector[i].m_Alignment = 5;
					AssTXTVector[i].m_dX = val1;
					AssTXTVector[i].m_dY = val2;

					continue;
				}
			}
			else
			{
				val3 = (g_H - (AssTXTVector[i].m_LY + AssTXTVector[i].m_LH/10));

				if ( abs(AssTXTVector[i].m_pAssStyle->m_MarginV - val3) < AssTXTVector[i].m_LH/2 )
				{
					if (vo != 1)
					{
						if (AssTXTVector[i].m_Alignment == -1)
						{
							AssTXTVector[i].m_Alignment = 2;
						}
					}
				}
				else
				{
					val1 = (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2;
					val2 = AssTXTVector[i].m_LY - AssTXTVector[i].m_LH/2;

					AssTXTVector[i].m_Alignment = 5;
					AssTXTVector[i].m_dX = val1;
					AssTXTVector[i].m_dY = val2;

					continue;
				}
			}
		}
	}

	// מבתוהטםÿול מבתוהטםÿולûו סאבû
	i=0;
	while(i<NT-1)
	{
		if( (AssTXTVector[i+1].m_AssStyleIndex == AssTXTVector[i].m_AssStyleIndex) &&
			(AssTXTVector[i+1].m_BT == AssTXTVector[i].m_BT) &&
			(AssTXTVector[i+1].m_ET == AssTXTVector[i].m_ET) )
		{
			val1 = abs((AssTXTVector[i+1].m_LXB + AssTXTVector[i+1].m_LXE) - (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE))/2;
			val2 = abs((AssTXTVector[i+1].m_LYB + AssTXTVector[i+1].m_LYE) - (AssTXTVector[i].m_LYB + AssTXTVector[i].m_LYE))/2 - 
				   (AssTXTVector[i+1].m_LYE - AssTXTVector[i+1].m_LYB + AssTXTVector[i].m_LYE - AssTXTVector[i].m_LYB)/2;
			val3 = AssTXTVector[i].m_LH;

			if ( (val1 < val3*2) &&
				 (val2 < val3) )
			{
				if (AssTXTVector[i+1].m_LYE > AssTXTVector[i].m_LYE)
				{
					AssTXTVector[i].m_TXTStr = AssTXTVector[i].m_TXTStr + string("\\N") + AssTXTVector[i+1].m_TXTStr;
				}
				else
				{
					AssTXTVector[i].m_TXTStr = AssTXTVector[i+1].m_TXTStr + string("\\N") + AssTXTVector[i].m_TXTStr;
				}

				if (AssTXTVector[i].m_dX != -1)
				{
					val1 = ((AssTXTVector[i+1].m_LXB + AssTXTVector[i+1].m_LXE)/2 + (AssTXTVector[i].m_LXB + AssTXTVector[i].m_LXE)/2)/2;
					val2 = ((AssTXTVector[i+1].m_LYB + AssTXTVector[i+1].m_LYE)/2 + (AssTXTVector[i].m_LYB + AssTXTVector[i].m_LYE)/2)/2;

					AssTXTVector[i].m_dX = val1;
					AssTXTVector[i].m_dY = val2;
				}

				for(j=i+1; j<NT-1; j++)
				{
					AssTXTVector[j] = AssTXTVector[j+1];
				}
				NT--;
			}
			else
			{
				i = i;
			}
		}

		i++;
	}

	fout.open(string(m_pMF->m_Dir+"/sub.ass").c_str(), ios::out);

	fout << "Title: Default Aegisub file\n";
	fout << "ScriptType: v4.00+\n";
	fout << "PlayResX: " << g_W << "\n";
	fout << "PlayResY: " << g_H << "\n";
	fout << "PlayDepth: 16\n";
	fout << "Timer: 100,0000\n";
	fout << "WrapStyle: 1\n";
	fout << "\n";
	fout << "\n";
	fout << "[V4+ Styles]\n";
	fout << "Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding\n";

	for(i=0; i<NS; i++)
	{
		if (AssTXTStyleVector[i].m_MarginL == -1)
		{
			AssTXTStyleVector[i].m_MarginL = 10;
			AssTXTStyleVector[i].m_MarginR = 10;
		}

		YIQ_to_RGB( AssTXTStyleVector[i].m_mY, 
					AssTXTStyleVector[i].m_mI, 
					AssTXTStyleVector[i].m_mQ, 
					mR, mG, mB, 255 );

		sprintf(str, "&H00%.2X%.2X%.2X", mB, mG, mR);

		fout << "Style: " << AssTXTStyleVector[i].m_Name << ",";
		fout << "Arial Narrow," << AssTXTStyleVector[i].m_LH << ",";
		fout << str << ",";
		fout << "&H0000FFFF,&H00000000,&H80000000,-1,0,0,0,100,100,0,0,1,2,0,";
		fout << AssTXTStyleVector[i].m_Alignment << ",";
		fout << AssTXTStyleVector[i].m_MarginL << ",";
		fout << AssTXTStyleVector[i].m_MarginR << ",";
		fout << AssTXTStyleVector[i].m_MarginV << ",";
		fout << "204\n";
	}

	fout << "\n";
	fout << "[Events]\n";
	fout << "Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text";

	for(i=0; i<NT; i++)
	{
		fout << "\n";

		fout << "Dialogue: 0,";
		fout << VideoTimeToStr3(AssTXTVector[i].m_BT*(u64)10000) << ",";
		fout << VideoTimeToStr3(AssTXTVector[i].m_ET*(u64)10000) << ",";
		fout << AssTXTVector[i].m_pAssStyle->m_Name << ",";
		fout << ",0000,0000,0000,,";

		if ( (AssTXTVector[i].m_Alignment != -1) ||
			 (AssTXTVector[i].m_dX != -1) )
		{
			fout << "{";

			if (AssTXTVector[i].m_Alignment != -1)
			{
				fout << "\\an" << AssTXTVector[i].m_Alignment;
			}

			if (AssTXTVector[i].m_dX != -1)
			{
				fout << "\\pos(" << AssTXTVector[i].m_dX << "," << AssTXTVector[i].m_dY << ")";
			}

			fout << "}";
		}

		fout << AssTXTVector[i].m_TXTStr;
	}

	fout.close();

	delete[] AssTXTVector;
	delete[] AssTXTStyleVector;
}

void COCRPanel::OnBnClickedTest(wxCommandEvent& event)
{
	int w, h, S, i, j;
	vector<string> SavedFiles;
	wxString Str;
	s64 CurPos;

	if (m_pMF->m_VIsOpen == false) return;
	
	SetVideoWindowSettins(m_pMF->m_pVideo, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pVSL2->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL1->m_pos, 
                                  m_pMF->m_pVideoBox->m_pVBox->m_pHSL2->m_pos);

	w = g_w;
	h = g_h;

	InitIPData((int)m_pMF->m_pVideo->m_Width, (int)m_pMF->m_pVideo->m_Height, 3);
	
	S = GetAndConvertImage(g_ImRGB, g_ImF[3], g_ImF[4], g_ImF[5], g_ImF[0], g_ImF[1], g_ImF[2], m_pMF->m_pVideo, w, h);

	SavedFiles.clear();

	CurPos = m_pMF->m_pVideo->GetPos();

	Str = m_pMF->m_pVideo->m_MovieName.c_str();

	j = Str.length()-1;
	while (Str[j] != '.') j--;
	i = j;
	while ((Str[i] != '\\') && (Str[i] != '/')) i--;

	Str = Str.Mid(i+1, j-i-1);

	Str += wxString(" -- ") + VideoTimeToStr(CurPos).c_str();

	SavedFiles.push_back(string(Str));

	g_show_results = 1;

	if (g_debug == 0) m_pMF->ClearDir("TestImages");

	FindTextLines(g_ImRGB, g_ImF[5], g_ImF[3], SavedFiles, w, h);
}

void COCRPanel::OnBnClickedCreateClearedTextImages(wxCommandEvent& event)
{
	if (g_IsCreateClearedTextImages == 0)
	{
		g_IsCreateClearedTextImages = 1;
		g_RunCreateClearedTextImages = 1;

		m_pCCTI->SetLabel("Stop CCTXTImages");

		m_pSearchThread = new ThreadCreateClearedTextImages(m_pMF);
		m_pSearchThread->Create();
		m_pSearchThread->Run();
		m_pSearchThread->SetPriority(30); //THREAD_PRIORITY_BELOW_NORMAL
	}
	else
	{
		m_pCCTI->SetLabel("Create Cleared Text Images");
		g_RunCreateClearedTextImages = 0;
	}
}

ThreadCreateClearedTextImages::ThreadCreateClearedTextImages(CMainFrame *pMF)
        : wxThread()
{
    m_pMF = pMF;
}

void *ThreadCreateClearedTextImages::Entry()
{
	g_IsCreateClearedTextImages = 1;

	if (g_debug == 0) g_show_results = 0;
	else g_show_results = 1;

	wxString Str, dStr;
	string fname;
	ofstream fout;
	char str[30];
	int i, j, k, w, h, val;
	
	int w1, h1, w2, h2, YB1, YB2, bln;
	wxString hour1, hour2, min1, min2, sec1, sec2, msec1, msec2;
	u64 bt1, et1, bt2, et2;

	int *ImRES1 = NULL;
	int *ImRES2 = NULL;

	int res;	

	m_pMF->ClearDir("TXTImages");
	m_pMF->ClearDir("TXTResults");

	// מקטשאול פאיכ text_lines.info
	fname = g_dir + string("/text_lines.info");
	fout.open(fname.c_str(), ios::out);
	fout << "";
	fout.close();

	wxString dir_path = wxString(m_pMF->m_Dir + string("/RGBImages/"));
	wxDir dir(dir_path);
	vector<wxString> FileNamesVector;
	vector<string> SavedFiles, prevSavedFiles;
	vector<u64> BT, ET;
	wxString filename;
	bool bres;

	bres = dir.GetFirst(&filename, "*.jpeg");
    while ( bres )
    {
		FileNamesVector.push_back(filename);

        bres = dir.GetNext(&filename);
    }

	for (i=0; i<(int)FileNamesVector.size()-1; i++)
	for (j=i+1; j<(int)FileNamesVector.size(); j++)
	{
		if (FileNamesVector[i] > FileNamesVector[j])
		{
			Str = FileNamesVector[i];
			FileNamesVector[i] = FileNamesVector[j];
			FileNamesVector[j] = Str;
		}
	}

	val = (int)FileNamesVector.size();
	sprintf(str, "%.4d", val);
	dStr = wxString(" : ") + wxString(str);

    g_W = -1;
    g_H = -1;
    w = 0;
    h = 0;

	for (k=0; k<(int)FileNamesVector.size(); k++)
	{
		if (g_RunCreateClearedTextImages == 0) break;

		Str = m_pMF->m_Dir+"/RGBImages/"+FileNamesVector[k];
		
        GetImageSize(string(Str), w, h);
        
        if ( (g_W != w) || (g_H != h) )
        {
            g_W = w;
	        g_H = h;

            g_xmin = 0;
	        g_xmax = w-1;
	        g_ymin = 0;
	        g_ymax = h-1;
            
	        InitIPData(w, h, 3);
        }

		LoadRGBImage(g_ImRGB, string(Str), w, h);		
		//m_pMF->m_pVideoBox->ViewImage(ImRGB, w, h);		

		GetTransformedImage(g_ImRGB, g_ImF[3], g_ImF[4], g_ImF[5], g_ImF[0], g_ImF[1], g_ImF[2], w, h);

		if (g_use_FRD_images == true) 
		{
			Str = FileNamesVector[k];
			Str = Str.Mid(0, Str.length()-5);
			Str = m_pMF->m_Dir+"/FRDImages/"+Str+"!.jpeg";
			LoadImage(g_ImF[5], string(Str), w, h);		
			//m_pMF->m_pImageBox->ViewImage(ImSF, w, h);
		}
		
		Str = FileNamesVector[k];
		Str = Str.Mid(0, Str.length()-5);
		SavedFiles.clear();
		SavedFiles.push_back(string(Str));

		val = k+1;
		sprintf(str, "%.4d", val);
		m_pMF->m_pVideoBox->m_plblTIME->SetLabel(wxString(str) + dStr);

		res = FindTextLines(g_ImRGB, g_ImF[5], g_ImF[3], SavedFiles, w, h);

		if ( (res == 0) && (g_DontDeleteUnrecognizedImages1 == true) )
		{
			Str = FileNamesVector[k];
			Str = Str.Mid(0, Str.length()-5);
			Str = wxString("/TXTImages/") + Str + wxString("_01.jpeg");

			memset(g_ImRES1, 0, ((w*4)*(h/4))*sizeof(int));

			SaveImage(g_ImRES1, string(Str), w*4, h/4);
			
			val = 14; //LH
			SaveTextLineParameters(	string(Str), 0, 
								val, (h+val)/2, 
								w/2, w/2,
								(h-val)/2 + 1, (h+val)/2,
								0, 0, 0 );
		}

		if ( (k>1) && (res == 1) && (g_ValidateAndCompareTXTImages == true) && (prevSavedFiles.size() == SavedFiles.size()) )
		{
			Str = prevSavedFiles[i].c_str();
			i = Str.length()-1;
			while ((Str[i] != '\\') && (Str[i] != '/')) i--;
			Str = Str.Mid(i+1);

			hour1 = Str.Mid(0,1);
			min1 = Str.Mid(2,2);
			sec1 = Str.Mid(5,2);
			msec1 = Str.Mid(8,3);

			hour2 = Str.Mid(13,1);
			min2 = Str.Mid(15,2);
			sec2 = Str.Mid(18,2);
			msec2 = Str.Mid(21,3);

			bt1 = (atoi(hour1)*3600 + atoi(min1)*60 + atoi(sec1))*1000 + atoi(msec1);
			et1 = (atoi(hour2)*3600 + atoi(min2)*60 + atoi(sec2))*1000 + atoi(msec2);

			Str = SavedFiles[i].c_str();
			i = Str.length()-1;
			while ((Str[i] != '\\') && (Str[i] != '/')) i--;
			Str = Str.Mid(i+1);

			hour1 = Str.Mid(0,1);
			min1 = Str.Mid(2,2);
			sec1 = Str.Mid(5,2);
			msec1 = Str.Mid(8,3);

			hour2 = Str.Mid(13,1);
			min2 = Str.Mid(15,2);
			sec2 = Str.Mid(18,2);
			msec2 = Str.Mid(21,3);

			bt2 = (atoi(hour1)*3600 + atoi(min1)*60 + atoi(sec1))*1000 + atoi(msec1);
			et2 = (atoi(hour2)*3600 + atoi(min2)*60 + atoi(sec2))*1000 + atoi(msec2);
			
			if (bt2-et1 < 300)
			{
				bln = 1;
				for (i=0; i<(int)SavedFiles.size(); i++)
				{
					LoadImage(g_ImF[0], prevSavedFiles[i], w1, h1);
					LoadImage(g_ImF[1], SavedFiles[i], w2, h2);

					Str = prevSavedFiles[i].c_str();
					i = Str.length()-1;
					while (Str[i] != '_') i--;
					j = i;
					i--;
					while (Str[i] != '_') i--;
					Str = Str.Mid(i+1, j-i-1);
					YB1 = atoi(Str);

					Str = SavedFiles[i].c_str();
					i = Str.length()-1;
					while (Str[i] != '_') i--;
					j = i;
					i--;
					while (Str[i] != '_') i--;
					Str = Str.Mid(i+1, j-i-1);
					YB2 = atoi(Str);

					bln = CompareTXTImages(g_ImF[0], g_ImF[1], w1, h1, w2, h2, YB1, YB2);
					if (bln == 0) break;
				}

				if (bln == 1)
				{
					for (i=0; i<(int)SavedFiles.size(); i++)
					{
						DeleteFile(prevSavedFiles[i].c_str());
						
						Str = prevSavedFiles[i].c_str();
						i = Str.length()-1;
						while ((Str[i] != '\\') && (Str[i] != '/')) i--;
						Str = Str.Mid(0,i+1+11)+wxString(SavedFiles[i].c_str()).Mid(i+1+11);

						MoveFile(SavedFiles[i].c_str(), Str);
					}
				}
			}
		}

		prevSavedFiles = SavedFiles;
	}

	//if (ImRES1 != NULL) delete[] ImRES1;
	//if (ImRES2 != NULL) delete[] ImRES2;

	m_pMF->m_pVideoBox->m_plblTIME->SetLabel("00:00:00,000/00:00:00,000");

	m_pMF->m_pPanel->m_pOCRPanel->m_pCCTI->SetLabel("Create Cleared TXT Images");

	g_IsCreateClearedTextImages = 0;
	
	return 0;
}
