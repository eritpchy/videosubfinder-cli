                              //Control.h//                                
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
#include <vector>

class CControl
{
public:
	static std::vector<CControl*> m_all_controls;

	CControl()
	{
		CControl::m_all_controls.push_back(this);
	}

	~CControl()
	{		
		CControl::m_all_controls.erase(find(CControl::m_all_controls.begin(), CControl::m_all_controls.end(), this));
	}

	static void RefreshAllControlsData()
	{
		for (int i = 0; i < m_all_controls.size(); i++)
		{
			m_all_controls[i]->RefreshData();
		}
	}

	static void UpdateAllControlsSize()
	{
		for (int i = m_all_controls.size() - 1; i >= 0; i--)
		{
			m_all_controls[i]->UpdateSize();
		}
	}

	virtual bool UpdateData(wxString* newval) { bool res = true; return res; }
	virtual void UpdateSize(){}

	virtual void RefreshData() = 0;
};

