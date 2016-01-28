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

#include "CommonFunctions.h"

wxSize get_max_wxSize(vector<wxSize> sizes)
{
	wxSize res(0, 0);

	for (int i = 0; i < sizes.size(); i++)
	{
		if (sizes[i].x > res.x) res.x = sizes[i].x;
		if (sizes[i].y > res.y) res.y = sizes[i].y;
	}

	return res;
}