                              //MyClosedFigure.cpp//                                
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

#include "MyClosedFigure.h"
#include <algorithm>

using namespace std;

CMyClosedFigure::CMyClosedFigure()
{
}

CMyClosedFigure::~CMyClosedFigure()
{
}

void CMyClosedFigure::operator=(CMyClosedFigure& other)
{	
	m_PointsArray = other.m_PointsArray;
	m_minX = other.m_minX;
	m_maxX = other.m_maxX;
	m_minY = other.m_minY;
	m_maxY = other.m_maxY;
}

void CMyClosedFigure::operator+=(CMyClosedFigure& other)
{	
	m_PointsArray += other.m_PointsArray;
	if (other.m_minX < m_minX) m_minX = other.m_minX;
	if (other.m_maxX > m_maxX) m_maxX = other.m_maxX;
	if (other.m_minY < m_minY) m_minY = other.m_minY;
	if (other.m_maxY > m_maxY) m_maxY = other.m_maxY;
}
