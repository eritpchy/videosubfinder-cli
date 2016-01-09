                              //MyClosedFigure.h//                                
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

#include "DataTypes.h"
#include <vector>

using namespace std;

class CMyPoint
{
public:
	int m_x;
	int m_y;
	int	m_i;

	CMyPoint() {}
	CMyPoint(int x, int y, int i) {m_x=x; m_y=y; m_i=i;}
}; 

typedef enum {UP, DOWN} POS;

class CMyClosedFigure
{
public:
	CMyPoint *m_PointsArray;
	int m_minX;
	int m_maxX;
	int m_minY;
	int m_maxY;
	int m_w;
	int m_h;
	int m_Square;
	int m_Count;
	int m_Weight;
	int m_DY;
	string m_SymbolValue;
	string m_ParentImageName;

	int m_mY;
	int m_mI;
	int m_mQ;
	int m_mmY;
	int m_mmI;
	int m_mmQ;

	char *m_pImage;
	int m_Imagew;
	int m_Imageh;
	char m_White;
	char m_Black;

	POS m_pos;

	CMyClosedFigure();
	~CMyClosedFigure();

	bool operator>(CMyClosedFigure& other);
	void operator=(CMyClosedFigure& other);
	void operator+=(CMyClosedFigure& other);

	void refresh();
	bool IsNear(CMyClosedFigure &other,int pogreshnost);	
	void CreateImage(int w,int h,char White,char Black);
	double CompareWith(CMyClosedFigure &other,double MaxPercentDeviation);

	//bool IsPointIn(CMyPoint *Point);
	//void AddPoints(CMyClosedFigure *other);	
	
	//void AlignPoints();
};

clock_t SearchClosedFigures(int *Im, int w, int h, int white, CMyClosedFigure* &FiguresArray, int &Number);
clock_t CreateIndexedImage(int *Im, int *ImRES, int w, int h, int white, int &Number);