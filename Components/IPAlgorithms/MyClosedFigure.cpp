                              //MyClosedFigure.cpp//                                
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

#include "MyClosedFigure.h"

CMyClosedFigure::CMyClosedFigure()
{
	m_PointsArray = NULL;
	m_pImage = NULL;
	m_Square=0;
	m_Count=1;
}

CMyClosedFigure::~CMyClosedFigure()
{
	if (m_PointsArray!=NULL) delete[] m_PointsArray; 
	if (m_pImage!=NULL) delete[] m_pImage;
}

bool CMyClosedFigure::operator>(CMyClosedFigure& other)
{	
	if((m_minY-other.m_minY) >= 0.7*m_DY) return true;
	else 
	{
		if( (m_minX < other.m_minX) && (abs(m_minY-other.m_minY) < 0.7*m_DY) )
		{			
			return true;
		}
		else return false;
	}
}

void CMyClosedFigure::operator=(CMyClosedFigure& other)
{	
	if (m_PointsArray!=NULL) delete[] m_PointsArray;
	m_PointsArray = new CMyPoint[other.m_Square];
	memcpy(m_PointsArray,other.m_PointsArray,4*other.m_Square);
	
	if (other.m_pImage!=NULL)
	{
		if (m_pImage!=NULL) delete[] m_pImage;

		m_Imagew = other.m_Imagew;
		m_Imageh = other.m_Imageh;
		m_White = other.m_White;
		m_Black = other.m_Black;
		m_pImage = new char[m_Imagew*m_Imageh];
		memcpy(m_pImage,other.m_pImage,m_Imagew*m_Imageh);
	}

	m_minX = other.m_minX;
	m_maxX = other.m_maxX;
	m_minY = other.m_minY;
	m_maxY = other.m_maxY;
	m_w = other.m_w;
	m_h = other.m_h;
	m_Square = other.m_Square;
	m_Count = other.m_Count;
	m_DY = other.m_DY;
	m_SymbolValue = other.m_SymbolValue;
	m_ParentImageName = other.m_ParentImageName;
}

void CMyClosedFigure::operator+=(CMyClosedFigure& other)
{	
	int N1, N2;
	CMyPoint *m;

	N1 = m_Square;
	N2 = other.m_Square;

	m = new CMyPoint[N1+N2];
	memcpy(m,m_PointsArray,4*N1);
	memcpy(m+N1,other.m_PointsArray,4*N2);

	delete[] m_PointsArray;
	m_PointsArray = m;
	m_Square = N1+N2;
	m_Count += other.m_Count;

	if (other.m_minX < m_minX) m_minX = other.m_minX;
	if (other.m_maxX > m_maxX) m_maxX = other.m_maxX;
	if (other.m_minY < m_minY) m_minY = other.m_minY;
	if (other.m_maxY > m_maxY) m_maxY = other.m_maxY;
	m_w = m_maxX-m_minX+1;
	m_h = m_maxY-m_minY+1;
}

void CMyClosedFigure::refresh()
{
	int i,N;
	int minX,maxX,minY,maxY;
	CMyPoint *m;
	
	N = m_Square;
	if (N>0)
	{
		m=m_PointsArray;
		minX=maxX=m[0].m_x;
		minY=maxY=m[0].m_y;

		for (i=0; i<N; i++)
		{
			if (m[i].m_x<minX) minX=m[i].m_x;
			if (m[i].m_x>maxX) maxX=m[i].m_x;
			if (m[i].m_y<minY) minY=m[i].m_y;
			if (m[i].m_y>maxY) maxY=m[i].m_y;
		}
		
		m_minX = minX;
		m_maxX = maxX;
		m_minY = minY;
		m_maxY = maxY;
		m_w = maxX-minX+1;
		m_h = maxY-minY+1;
	}
}

bool CMyClosedFigure::IsNear(CMyClosedFigure &other,int pogreshnost)
{
	CMyPoint Point,Point2, *m1, *m2;
	int i,j,N1,N2;

	if ( abs((m_maxX+m_minX)-(other.m_maxX+other.m_minX)) < (m_maxX-m_minX)+(other.m_maxX-other.m_minX)+3+2*pogreshnost )
	if ( abs((m_maxY+m_minY)-(other.m_maxY+other.m_minY)) < (m_maxY-m_minY)+(other.m_maxY-other.m_minY)+3+2*pogreshnost )
	{
		N1 = m_Square;
		N2 = other.m_Square;
		m1 = m_PointsArray;
		m2 = other.m_PointsArray;

		for(i=0; i<N1; i++)
		{
			Point = m1[i];
			
			for(j=0; j<N2; j++)
			{
				Point2 = m2[j];
				
				if (abs(Point.m_x-Point2.m_x)<2+pogreshnost) 
				if (Point.m_y==Point2.m_y)
				{
					return true;
				}

				if (abs(Point.m_y-Point2.m_y)<2+pogreshnost)
				if (Point.m_x==Point2.m_x)
				{
					return true;
				}
			}
		}
		return false;
	}
	return false;
}

void CMyClosedFigure::CreateImage(int w,int h,char White,char Black)
{
	int i,x0,y0,x,y;

	m_Imagew = w;
	m_Imageh = h;
	m_White = White;
	m_Black = Black;

	x0=(w-(m_minX+m_maxX))/2;
	y0=(h-(m_minY+m_maxY))/2;

	if (m_pImage!=NULL) delete[] m_pImage;

	m_pImage = new char[w*h];

	for (i=0; i<w*h; i++) m_pImage[i]=White; 
	
	for (i=0; i<m_Square; i++)
	{ 
		x=x0+m_PointsArray[i].m_x;
		y=y0+m_PointsArray[i].m_y;
		
		if(y>=0)
		if(y<h)
		if(x>=0)
		if(x<w)
		{
			m_pImage[y*w+x]=Black;
		}
	}
}

double CMyClosedFigure::CompareWith(CMyClosedFigure &other,double MaxPercentDeviation)
{
	char *m1, *m2;
	char Black, White;
	int i, w, h, dS;
	double value;

	if (m_Count == other.m_Count)
	if ((double)abs(m_w-other.m_w)/(double)m_w <= MaxPercentDeviation)
	if ((double)abs(m_h-other.m_h)/(double)m_h <= MaxPercentDeviation)
	{
		w = m_Imagew;
		h = m_Imageh;
		Black = m_Black;
		White = m_White;
		m1 = m_pImage;
		m2 = other.m_pImage;
		
		dS = 0;
		for (i=0; i<w*h; i++)
		{
			if (m1[i]!=m2[i])
			if ( (m1[i-1]!=m2[i-1]) || (m1[i-1]==White) )
			if ( (m1[i+1]!=m2[i+1]) || (m1[i+1]==White) )
			{
				dS++;
			}
		}

		value = (double)dS/(double)m_Square;
		if (value <= MaxPercentDeviation)
		{
			return value;
		}
	}
	return 1;
}

/* //for algoritm IsNear
int minX, maxX, minY, maxY, w, h, val, val1, val2, x0, y0, x, y;
char **m, White, Black;
vector<int> DXVector,DYVector;
int *DX, *DY, DN;

val1 = pogreshnost+1;
val2 = pogreshnost+2;
val = val1*val1+1;
for (i=-val1; i<val2; i++)
for (j=-val1; j<val2; j++)
{
	if (i*i+j*j < val) 
	{
		DXVector.push_back(i);
		DYVector.push_back(j);
	}
}

DN = DXVector.size();
DX = new int[DN];
DY = new int[DN];
for (i=0; i<DN; i++) 
{
	DX[i] = DXVector[i];
	DY[i] = DYVector[i];
}

DXVector.clear();
DYVector.clear();

White = 0;
Black = 1;

minX = min(m_minX,other.m_minX);
maxX = max(m_maxX,other.m_maxX);
minY = min(m_minY,other.m_minY);
maxY = max(m_maxY,other.m_maxY);
w = maxX-minX+1;
h = maxY-minY+1;

m = new char*[w];
for (i=0; i<w; i++) 
{
	m[i] = new char[h];
	for (j=0; j<h; j++) m[i][j] = White;
}

if (m_Square <= other.m_Square)
{
	N1 = m_Square;
	N2 = other.m_Square;
	m1 = m_PointsArray;
	m2 = other.m_PointsArray;
}
else
{
	N2 = m_Square;
	N1 = other.m_Square;
	m2 = m_PointsArray;
	m1 = other.m_PointsArray;
}

for (i=0; i<N1; i++)
{
	x0 = m1[i].m_x - minX;
	y0 = m1[i].m_y - minY;

	for (j=0; j<DN; j++)
	{
		x = x0+DX[j];
		y = y0+DY[j];
		if (x>=0)
		if (x<w)
		if (y>=0)
		if (y<h)
		{
			m[x][y] = Black;
		}
	}
}

for (i=0; i<N2; i++)
if (m[m2[i].m_x-minX][m2[i].m_y-minY] == Black)
{
	for (i=0; i<w; i++) 
	{
		delete[] m[i];
	}
	delete[] m;
	delete[] DX;
	delete[] DY;
	
	return true;
}

for (i=0; i<w; i++) 
{
	delete[] m[i];
}
delete[] m;
delete[] DX;
delete[] DY;*/
/*
bool CMyClosedFigure::IsPointIn(CMyPoint *Point)
{
	if( (Point->m_x >= m_minX) && (Point->m_x <= m_maxX) &&
		(Point->m_y >= m_minY) && (Point->m_y <= m_maxY) )
	{
		if( m_PointsList.Find(*Point)!=NULL ) return true;
		else return false;
	}
	else return false;
	return false;
}

void CMyClosedFigure::AddPoints(CMyClosedFigure *other)
{
	m_PointsList.AddTail(&other->m_PointsList);

	if (other->m_minX < m_minX) m_minX = other->m_minX;
	if (other->m_maxX > m_maxX) m_maxX = other->m_maxX;
	if (other->m_minY < m_minY) m_minY = other->m_minY;
	if (other->m_maxY > m_maxY) m_maxY = other->m_maxY;
	m_Square+=other->m_Square;
}

void CMyClosedFigure::AlignPoints()
{
	POSITION pos;
	CMyPoint *pPoint;
	int i;

	pos = m_PointsList.GetHeadPosition();
	for(i=0;i < m_PointsList.GetCount();i++)
	{
		pPoint = &m_PointsList.GetNext(pos);
		pPoint->m_x-=m_minX+1;
		pPoint->m_y-=m_minY;
	}

	m_maxX-=m_minX;
	m_maxY-=m_minY;
	m_minX=0;
	m_minY=0;
}
*/

/////////////////////////////////////////////////////////////////////////////

clock_t SearchClosedFigures(int *Im, int w, int h, int white, CMyClosedFigure* &FiguresArray, int &Number)
{
	int N;
	int *m, *key, *key2, *NN, *I, *minX, *maxX, *minY, *maxY;
	int i, j, i1, i2, jj, kk, index;
	int x, y, size;
	bool bln, bln2;
	clock_t start;
		
	start = clock();
	size = w*h;

	m = new int[size];
	key = new int[size];
	key2 = new int[size];
	index=0;

	for(i=0; i<size; i++)
	{
		m[i]=-1;
		key[i]=i;
		key2[i]=i;
	}

	//Ќаходим все замкнутые фигуры в изображении
	for(i=0; i<size; i++)
	{
		if(Im[i] == white)
		{
			y = i/w;
			x = i - y*w;
			
			bln=false;
			bln2=false;
			//ѕровер€ем есть ли с лева точка фигуры
			if ( (x>0) && (Im[i-1]==white) )//[x-1][y]
			{//есть:
				bln=true;
				i1=i-1;
				
				if (y>0)
				{
					//ѕровер€ем нет ли c низу с лева точки фигуры
					if (Im[i-w-1]!=white)//[x-1][y-1]
					{//с низу с лева точки фигуры нету:

						//ѕровер€ем есть ли с низу точка фигуры
						if (Im[i-w]==white)//[x][y-1]
						{//есть:
							bln2=true;
							i2=i-w;
						}

						else 
						{//нету: 
							
							//ѕровер€ем есть ли с низу с права точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
					}
					else
					{//с низу с лева точка фигуры есть:
						
						//ѕровер€ем нет ли с низу точки фигуры
						if (Im[i-w]!=white)//[x][y-1]
						{//нет:
							
							//ѕровер€ем есть ли с низу с права точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
					}
				}
			}
			else 
			{//с лева точки фигуры нету:

				if (y>0)
				{
					//ѕровер€ем есть ли с низу точка фигуры
					if (Im[i-w]==white)//[x][y-1]
					{//есть:
						bln=true;
						i1=i-w;
					}

					else
					{//нету:
						
						//ѕровер€ем есть ли снизу слева точка фигуры
						if( (x>0) && (Im[i-w-1]==white) )//[x-1][y-1]
						{//есть:
							bln=true;
							i1=i-w-1;
							
							//ѕровер€ем есть ли снизу справа точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{
							//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
						
						else 
						{//нету: 
							
							//ѕровер€ем есть ли снизу справа точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{
							//есть:
								bln=true;
								i1=i-w+1;
							}
						}
					}
				}
			}

			if (bln)
			{
				m[i]=key[m[i1]];
			}
			else
			{
				m[i]=index;
				index++;
			}

			if (bln2)
			if (m[i1]!=m[i2])
			{
				jj=max(m[i1],m[i2]);
				kk=min(m[i1],m[i2]);
				while (key[jj]!=jj) jj=key[jj];
				while (key[kk]!=kk) kk=key[kk];
				key[max(jj,kk)]=min(jj,kk);
			}

		}
	}

	N=0;
	for (i=0; i<index; i++)
	{
		j=i;
		while (key[j]!=j) j=key[j];
		key[i]=j;

		if (key[i]==i)
		{
			key2[i]=N;
			N++;
		}
	}

	Number = N;
	NN = new int[N];
	I = new int[N];
	minX = new int[N];
	maxX = new int[N];
	minY = new int[N];
	maxY = new int[N];

	for(i=0; i<N; i++)
	{
		NN[i]=0;
		I[i]=0;
		minX[i]=w;
		maxX[i]=0;
		minY[i]=h;
		maxY[i]=0;
	}

	for(i=0; i<w*h; i++)
	{
		if (Im[i]==white)
		{
			y = i/w;
			x = i - y*w;

			j=key2[key[m[i]]];
			NN[j]++;
			maxY[j]=y;
			if (minY[j]>y) minY[j]=y;
			if (maxX[j]<x) maxX[j]=x;
			if (minX[j]>x) minX[j]=x;
		}
	}

	FiguresArray = new CMyClosedFigure[N];

	CMyClosedFigure* pfa = FiguresArray, *pf;
	
	for(i=0; i<N; i++)
	{
		pf = pfa+i;
		pf->m_PointsArray = new CMyPoint[NN[i]];
		pf->m_Square =  NN[i];
		pf->m_minX = minX[i];
		pf->m_maxX = maxX[i];
		pf->m_minY = minY[i];
		pf->m_maxY = maxY[i];
		pf->m_w = maxX[i]-minX[i]+1;
		pf->m_h = maxY[i]-minY[i]+1;
	}

	for(y=0, i=0; y<h; y++)
	for(x=0; x<w; x++, i++)
	{
		if (Im[i]==white)
		{
			j = key2[key[m[i]]];
			pfa[j].m_PointsArray[I[j]]=CMyPoint(x, y, i);
			I[j]++;
		}
	}

	delete[] m;
	delete[] key;
	delete[] key2;
	delete[] NN;
	delete[] I;
	delete[] minX;
	delete[] maxX;
	delete[] minY;
	delete[] maxY;
	
	return (clock()-start);
}

clock_t CreateIndexedImage(int *Im, int *ImRES, int w, int h, int white, int &Number)
{
	int N;
	int *m, *key, *key2;
	int i, j, i1, i2, jj, kk, index;
	int x, y, size;
	bool bln, bln2;
	clock_t start;
		
	start = clock();
	size = w*h;

	m = ImRES;
	key = new int[size];
	key2 = new int[size];
	index=0;

	for(i=0; i<size; i++)
	{
		m[i]=-1;
		key[i]=i;
		key2[i]=i;
	}

	//Ќаходим все замкнутые фигуры в изображении
	for(i=0; i<size; i++)
	{
		if(Im[i] == white)
		{
			y = i/w;
			x = i - y*w;
			
			bln=false;
			bln2=false;
			//ѕровер€ем есть ли с лева точка фигуры
			if ( (x>0) && (Im[i-1]==white) )//[x-1][y]
			{//есть:
				bln=true;
				i1=i-1;
				
				if (y>0)
				{
					//ѕровер€ем нет ли c низу с лева точки фигуры
					if (Im[i-w-1]!=white)//[x-1][y-1]
					{//с низу с лева точки фигуры нету:

						//ѕровер€ем есть ли с низу точка фигуры
						if (Im[i-w]==white)//[x][y-1]
						{//есть:
							bln2=true;
							i2=i-w;
						}

						else 
						{//нету: 
							
							//ѕровер€ем есть ли с низу с права точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
					}
					else
					{//с низу с лева точка фигуры есть:
						
						//ѕровер€ем нет ли с низу точки фигуры
						if (Im[i-w]!=white)//[x][y-1]
						{//нет:
							
							//ѕровер€ем есть ли с низу с права точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
					}
				}
			}
			else 
			{//с лева точки фигуры нету:

				if (y>0)
				{
					//ѕровер€ем есть ли с низу точка фигуры
					if (Im[i-w]==white)//[x][y-1]
					{//есть:
						bln=true;
						i1=i-w;
					}

					else
					{//нету:
						
						//ѕровер€ем есть ли снизу слева точка фигуры
						if( (x>0) && (Im[i-w-1]==white) )//[x-1][y-1]
						{//есть:
							bln=true;
							i1=i-w-1;
							
							//ѕровер€ем есть ли снизу справа точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{
							//есть:
								bln2=true;
								i2=i-w+1;
							}
						}
						
						else 
						{//нету: 
							
							//ѕровер€ем есть ли снизу справа точка фигуры
							if (x<w-1) 
							if (Im[i-w+1]==white)//[x+1][y-1]
							{
							//есть:
								bln=true;
								i1=i-w+1;
							}
						}
					}
				}
			}

			if (bln)
			{
				m[i]=key[m[i1]];
			}
			else
			{
				m[i]=index;
				index++;
			}

			if (bln2)
			if (m[i1]!=m[i2])
			{
				jj=max(m[i1],m[i2]);
				kk=min(m[i1],m[i2]);
				while (key[jj]!=jj) jj=key[jj];
				while (key[kk]!=kk) kk=key[kk];
				key[max(jj,kk)]=min(jj,kk);
			}

		}
	}

	N=0;
	for (i=0; i<index; i++)
	{
		j=i;
		while (key[j]!=j) j=key[j];
		key[i]=j;

		if (key[i]==i)
		{
			key2[i]=N;
			N++;
		}
	}

	for(i=0; i<size; i++)
	{
		if (Im[i] == white)
		{
			y = i/w;
			x = i - y*w;

			j=key2[key[m[i]]];
			m[i] = j;
		}
	}

	Number = N;

	delete[] key;
	delete[] key2;
	
	return (clock()-start);
}
