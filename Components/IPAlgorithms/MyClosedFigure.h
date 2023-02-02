                              //MyClosedFigure.h//                                
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

#include "DataTypes.h"
#include <vector>
#include <wx/wx.h>

using namespace std;

class CMyClosedFigure
{
public:
	simple_buffer<int> m_PointsArray;

	u16 m_minX;
	u16 m_maxX;
	u16 m_minY;
	u16 m_maxY;

	CMyClosedFigure();
	~CMyClosedFigure();

	inline int width() {
		return (int)(m_maxX - m_minX + 1);
	}

	inline int height() {
		return (int)(m_maxY - m_minY + 1);
	}

	void operator=(CMyClosedFigure& other);
	void operator+=(CMyClosedFigure& other);
};

template <class T>
u64 SearchClosedFigures(simple_buffer<T> &Im, int w, int h, T white, custom_buffer<CMyClosedFigure> &FiguresArray, bool  combine_diagonal_points = true);

//-----------------------------------------------------

template <class T>
void GetInfoAboutNearestPoints(simple_buffer<T>& Im, int& x, int& y, int& i, int& w, bool& bln, bool& bln2, int& i1, int& i2, T& white, bool  combine_diagonal_points)
{
	if (combine_diagonal_points)
	{
		//Проверяем есть ли с лева точка фигуры
		if ((x > 0) && (Im[i - 1] == white))//[x-1][y]
		{//есть:
			bln = true;
			i1 = i - 1;

			if (y > 0)
			{
				//Проверяем нет ли c низу с лева точки фигуры
				if (Im[i - w - 1] != white)//[x-1][y-1]
				{//с низу с лева точки фигуры нету:

					//Проверяем есть ли с низу точка фигуры
					if (Im[i - w] == white)//[x][y-1]
					{//есть:
						bln2 = true;
						i2 = i - w;
					}
					else
					{//нету: 

						//Проверяем есть ли с низу с права точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}
				}
				else
				{//с низу с лева точка фигуры есть:

					//Проверяем нет ли с низу точки фигуры
					if (Im[i - w] != white)//[x][y-1]
					{//нет:

						//Проверяем есть ли с низу с права точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}
				}
			}
		}
		else
		{//с лева точки фигуры нету:

			if (y > 0)
			{
				//Проверяем есть ли с низу точка фигуры
				if (Im[i - w] == white)//[x][y-1]
				{//есть:
					bln = true;
					i1 = i - w;
				}
				else
				{//нету:

					//Проверяем есть ли снизу слева точка фигуры
					if ((x > 0) && (Im[i - w - 1] == white))//[x-1][y-1]
					{//есть:
						bln = true;
						i1 = i - w - 1;

						//Проверяем есть ли снизу справа точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{
								//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}
					else
					{//нету: 

						//Проверяем есть ли снизу справа точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{
								//есть:
								bln = true;
								i1 = i - w + 1;
							}
						}
					}
				}
			}
		}
	}
	else
	{
		//Проверяем есть ли с лева точка фигуры
		if ((x > 0) && (Im[i - 1] == white))//[x-1][y]
		{//есть:
			bln = true;
			i1 = i - 1;

			if (y > 0)
			{
				//Проверяем нет ли c низу с лева точки фигуры
				if (Im[i - w - 1] != white)//[x-1][y-1]
				{//с низу с лева точки фигуры нету:

					//Проверяем есть ли с низу точка фигуры
					if (Im[i - w] == white)//[x][y-1]
					{//есть:
						bln2 = true;
						i2 = i - w;
					}
					/*else
					{//нету: 

						//Проверяем есть ли с низу с права точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}*/
				}
				/*else
				{//с низу с лева точка фигуры есть:

					//Проверяем нет ли с низу точки фигуры
					if (Im[i - w] != white)//[x][y-1]
					{//нет:

						//Проверяем есть ли с низу с права точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}
				}*/
			}
		}
		else
		{//с лева точки фигуры нету:

			if (y > 0)
			{
				//Проверяем есть ли с низу точка фигуры
				if (Im[i - w] == white)//[x][y-1]
				{//есть:
					bln = true;
					i1 = i - w;
				}
				/*else
				{//нету:

					//Проверяем есть ли снизу слева точка фигуры
					if ((x > 0) && (Im[i - w - 1] == white))//[x-1][y-1]
					{//есть:
						bln = true;
						i1 = i - w - 1;

						//Проверяем есть ли снизу справа точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{
								//есть:
								bln2 = true;
								i2 = i - w + 1;
							}
						}
					}
					else
					{//нету: 

						//Проверяем есть ли снизу справа точка фигуры
						if (x < w - 1)
						{
							if (Im[i - w + 1] == white)//[x+1][y-1]
							{
								//есть:
								bln = true;
								i1 = i - w + 1;
							}
						}
					}
				}*/
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

template <class T>
u64 SearchClosedFigures(simple_buffer<T>& Im, int w, int h, T white, custom_buffer<CMyClosedFigure>& FiguresArray, bool  combine_diagonal_points)
{
	int* m, * key, * key2, * max_index_in_split;
	const int n_splits = 1;
	const int size = w * h;
	//std::chrono::time_point<std::chrono::high_resolution_clock> start;

	//start = std::chrono::high_resolution_clock::now();

	m = new int[size];
	key = new int[size];
	key2 = new int[size];
	max_index_in_split = new int[n_splits];

	//Finding all closed figures on image

	// doesn't give difference with n_splits == 32 (work a little longer)
	// std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(n_splits), [&](int i_split)
	//{
	for (int i_split = 0; i_split < n_splits; i_split++)
	{
		int i, j, i1, i2, jj, kk;
		int x, y;
		bool bln, bln2;
		custom_assert(n_splits > 0, "SearchClosedFigures: n_splits > 0");
		int yb = i_split * (h / n_splits);
		int ye = (i_split == n_splits - 1) ? (h - 1) : (yb + (h / n_splits) - 1);
		int hh = ye - yb + 1;
		int index = yb * w + 1;

		for (y = 0, i = yb * w; y < hh; y++)
		{
			for (x = 0; x < w; x++, i++)
			{
				if (Im[i] == white)
				{
					bln = false;
					bln2 = false;

					GetInfoAboutNearestPoints(Im, x, y, i, w, bln, bln2, i1, i2, white, combine_diagonal_points);

					if (bln)
					{
						m[i] = m[i1];
					}
					else
					{
						m[i] = index;
						key[index] = index;
						key2[index] = index;
						index++;
					}

					if (bln2)
					{
						if (m[i1] != m[i2])
						{
							jj = max(m[i1], m[i2]);
							kk = min(m[i1], m[i2]);
							while (key[jj] != jj) jj = key[jj];
							while (key[kk] != kk) kk = key[kk];
							key[max(jj, kk)] = min(jj, kk);
						}
					}
				}
			}
		}

		max_index_in_split[i_split] = index;
	}//);

	for (int i_split = 1; i_split < n_splits; i_split++)
	{
		int i, j, i1, i2, jj, kk;
		custom_assert(n_splits > 0, "SearchClosedFigures: n_splits > 0");
		int x, y = i_split * (h / n_splits);
		bool bln, bln2;

		for (x = 0, i = y * w; x < w; x++, i++)
		{
			if (Im[i] == white)
			{
				bln = false;
				bln2 = false;

				GetInfoAboutNearestPoints(Im, x, y, i, w, bln, bln2, i1, i2, white, combine_diagonal_points);

				if (bln)
				{
					if (m[i] != m[i1])
					{
						jj = max(m[i], m[i1]);
						kk = min(m[i], m[i1]);
						while (key[jj] != jj) jj = key[jj];
						while (key[kk] != kk) kk = key[kk];
						key[max(jj, kk)] = min(jj, kk);
					}
				}

				if (bln2)
				{
					if (m[i1] != m[i2])
					{
						jj = max(m[i1], m[i2]);
						kk = min(m[i1], m[i2]);
						while (key[jj] != jj) jj = key[jj];
						while (key[kk] != kk) kk = key[kk];
						key[max(jj, kk)] = min(jj, kk);
					}
				}
			}
		}

	}

	int i, j, x, y, N = 0;

	for (int i_split = 0; i_split < n_splits; i_split++)
	{
		custom_assert(n_splits > 0, "SearchClosedFigures: n_splits > 0");
		int yb = i_split * (h / n_splits);
		int index_min = yb * w + 1;

		for (i = index_min; i < max_index_in_split[i_split]; i++)
		{
			j = i;
			while (key[j] != j) j = key[j];
			key[i] = j;

			if (key[i] == i)
			{
				key2[i] = N;
				N++;
			}
		}
	}

	for (int i_split = 0; i_split < n_splits; i_split++)
	{
		custom_assert(n_splits > 0, "SearchClosedFigures: n_splits > 0");
		int yb = i_split * (h / n_splits);
		int index_min = yb * w + 1;

		for (i = index_min; i < max_index_in_split[i_split]; i++)
		{
			key[i] = key2[key[i]];
		}
	}

	int* NN, * I, * minX, * maxX, * minY, * maxY;

	NN = new int[N];
	I = new int[N];
	minX = new int[N];
	maxX = new int[N];
	minY = new int[N];
	maxY = new int[N];

	for (i = 0; i < N; i++)
	{
		NN[i] = 0;
		I[i] = 0;
		minX[i] = w;
		maxX[i] = 0;
		minY[i] = h;
		maxY[i] = 0;
	}

	for (y = 0, i = 0; y < h; y++)
	{
		for (x = 0; x < w; x++, i++)
		{
			if (Im[i] == white)
			{
				j = key[m[i]];
				NN[j]++;
				maxY[j] = y;
				if (minY[j] > y) minY[j] = y;
				if (maxX[j] < x) maxX[j] = x;
				if (minX[j] > x) minX[j] = x;
			}
		}
	}
	FiguresArray.set_size(N);

	CMyClosedFigure* pf;

	for (i = 0; i < N; i++)
	{
		pf = &(FiguresArray[i]);
		pf->m_PointsArray.set_size(NN[i]);
		pf->m_minX = minX[i];
		pf->m_maxX = maxX[i];
		pf->m_minY = minY[i];
		pf->m_maxY = maxY[i];
	}

	for (y = 0, i = 0; y < h; y++)
	{
		for (x = 0; x < w; x++, i++)
		{
			if (Im[i] == white)
			{
				j = key[m[i]];
				FiguresArray[j].m_PointsArray[I[j]] = i;
				I[j]++;
			}
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
	delete[] max_index_in_split;

	return 1/*(std::chrono::high_resolution_clock::now() - start)*/;
}

