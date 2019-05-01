                              //IPAlgorithms.cpp//                                
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

#include "IPAlgorithms.h"
#include <math.h>
#include <assert.h>
#include <emmintrin.h>
#include <wx/wx.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <ppl.h>
using namespace std;

void MergeWithClusterImage(custom_buffer<int> &ImInOut, custom_buffer<int> &ImCluser, int w, int h, int white);
void(*g_pViewRGBImage)(custom_buffer<int> &Im, int w, int h);
void(*g_pViewImage[2])(custom_buffer<int> &Im, int w, int h);
void GreyscaleImageToMat(custom_buffer<int> &ImGR, int &w, int &h, cv::Mat &res);
void GreyscaleMatToImage(cv::Mat &ImGR, int &w, int &h, custom_buffer<int> &res);
void BinaryImageToMat(custom_buffer<int> &ImBinary, int &w, int &h, cv::Mat &res);
void BinaryMatToImage(cv::Mat &ImBinary, int &w, int &h, custom_buffer<int> &res, int white);
int GetSubParams(custom_buffer<int> &Im, int w, int h, int white, int &LH, int &LMAXY, int &lb, int &le, int min_h, int real_im_x_center);
int ClearImageOpt2(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white);
int ClearImageOpt5(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white);
int ClearImageOptimal(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white);
void CombineFiguresRelatedToEachOther(custom_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h);
void ClearImageFromGarbageBetweenLines(custom_buffer<int> &Im, int w, int h, int yb, int ye, int min_h, int white);
int ClearImageFromSmallSymbols(custom_buffer<int> &Im, int w, int h, int white);
void RestoreStillExistLines(custom_buffer<int> &Im, custom_buffer<int> &ImOrig, int w, int h);
int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImRGB, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H);
int ThirdFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H);
//int ThirdFiltrationForGFTI(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H);

string  g_work_dir;
string  g_app_dir;

string  g_im_save_format = ".jpeg";
//string  g_im_save_format = ".bmp";

double	g_mthr = 0.4;
double	g_mnthr = 0.3;
int		g_hvt = 65;	 
int		g_segw = 8;
int     g_segh = 3;  
int		g_msegc = 2;
int		g_scd = 800;
double	g_btd = 0.05;
double	g_tco = 0.1;

int		g_mpn = 50;
double	g_mpd = 0.3;
double  g_msh = 0.01;
double  g_msd = 0.2;
double	g_mpned = 0.3;

int g_min_color = 5;

int g_dmaxy = 8;

/// min-max point size for resolution ~ 480p=640×480 scaled to x4 == 4-18p (1-4.5p in original size)
double g_minpw = 4.0/640.0;
double g_maxpw = 18.0/640.0;
double g_minph = 4.0/480.0;
double g_maxph = 18.0/480.0;
double g_minpwh = 2.0/3.0;

int g_min_dI = 9;
int g_min_dQ = 9;
int g_min_ddI = 14;
int g_min_ddQ = 14;

#define STR_SIZE (256 * 2) // 

#define MAX_EDGE_STR 786432 //на сам деле ~ 32*3*255

//int g_LN;

int g_scale = 1;

//int g_blnVNE = 0;
//int g_blnHE = 0;

bool g_clear_image_logical = false;

bool g_show_results = false;
bool g_show_sf_results = false;
bool g_clear_test_images_folder = true;
bool g_show_transformed_images_only = false;

bool g_wxImageHandlersInitialized = false;

void ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h)
{	
	const int scd = g_scd, segw = g_segw, msegc = g_msegc, mx = (w - 1) / g_segw;
	__int64 t1, dt, num_calls;

	custom_assert(Im.size() >= w*h, "ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h)\nnot: Im.size() >= w*h");
	custom_assert(LB.size() >= h, "ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h)\nnot: LB.size() >= H");
	custom_assert(LE.size() >= h, "ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h)\nnot: LE.size() >= H");

	custom_buffer<int> line(h, 0);
	
	concurrency::parallel_for(0, h, [&](int y)
	{
		int ib = w * y;
		int cnt = 0;
		int i, ia, nx, mi, dif, rdif, gdif, bdif;
		int r0, g0, b0, r1, g1, b1;
		u8 *color;

		for (nx = 0, ia = ib; nx<mx; nx++, ia += segw)
		{
			color = (u8*)(&Im[ia]);
			r0 = color[2];
			g0 = color[1];
			b0 = color[0];

			mi = ia + segw;
			dif = 0;

			for (i = ia + 1; i <= mi; i++)
			{
				color = (u8*)(&Im[i]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];

				rdif = r1 - r0;
				if (rdif<0) rdif = -rdif;

				gdif = g1 - g0;
				if (gdif<0) gdif = -gdif;

				bdif = b1 - b0;
				if (bdif<0) bdif = -bdif;

				dif += rdif + gdif + bdif;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}

			if (dif >= scd) cnt++;
			else cnt = 0;

			if (cnt == msegc)
			{
				line[y] = 1;
				break;
			}
		}
	});

	custom_buffer<int> lb(h, 0), le(h, 0);
	int n = 0;
	int sbegin = 1; //searching begin
	int y;
	for (y = 0; y<h; y++)
	{
		if (line[y] == 1)
		{
			if (sbegin == 1)
			{
				lb[n] = y;
				sbegin = 0;
			}
		}
		else
		{
			if (sbegin == 0)
			{
				le[n] = y - 1;
				sbegin = 1;
				n++;
			}
		}
	}
	if (sbegin == 0)
	{
		le[n] = y - 1;
		n++;
	}

	if (n == 0)
	{
		N = 0;
		return;
	}


	int dd, bd, md;

	dd = 6;
	bd = 2 * dd + 1;
	md = 2 + dd;

	int k = 0;
	int val = lb[0] - dd;
	if (val<0) val = 0;
	LB[0] = val;

	for (int i = 0; i<n - 1; i++)
	{
		if ((lb[i + 1] - le[i])>bd)
		{
			if ((le[i] - LB[k]) >= md)
			{
				LE[k] = le[i] + dd;
				k++;
				LB[k] = lb[i + 1] - dd;
			}
			else
			{
				LB[k] = lb[i + 1] - dd;
			}
		}
	}
	if ((le[n-1] - LB[k]) >= md)
	{
		val = le[n-1] + dd;
		if (val>h - 1) val = h - 1;
		LE[k] = val;
		k++;
	}

	N = k;
}

void RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)
{
	u8 *color;
	int i, r, g, b, y, u, v;

	custom_assert(ImIn.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImY.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImY.size() >= w*h");
	custom_assert(ImU.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImU.size() >= w*h");
	custom_assert(ImV.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImV.size() >= w*h");

	for (i = 0; i<w*h; i++)
	{
		color = (u8*)(&ImIn[i]);

		r = color[2];
		g = color[1];
		b = color[0];

		//---(0.299*2^16)(0.587*2^16)(0.114*2^16)-----
		y = 19595 * r + 38470 * g + 7471 * b;

		//-(-0.147*2^16)(-0.289*2^16)(0.436*2^16)-----      
		u = -9634 * r - 18940 * g + 28574 * b;

		//---(0.615*2^16)(-0.515*2^16)(-0.100*2^16)---
		v = 40305 * r - 33751 * g - 6554 * b;

		ImY[i] = y >> 16;

		if (u >= 0)
		{
			ImU[i] = u >> 16;
		}
		else
		{
			u = -u;
			ImU[i] = -(u >> 16);
		}

		if (v >= 0)
		{
			ImV[i] = v >> 16;
		}
		else
		{
			v = -v;
			ImV[i] = -(v >> 16);
		}
	}
}

void YIQ_to_RGB(int Y, int I, int Q, int &R, int &G, int &B, int max_val)
{
	R = (int)(1.00000000*(double)Y + 0.95608445*(double)I + 0.62088850*(double)Q);
	G = (int)(1.00000000*(double)Y - 0.27137664*(double)I - 0.64860590*(double)Q);
	B = (int)(1.00000000*(double)Y - 1.10561724*(double)I + 1.70250126*(double)Q);

	if (R < 0)
	{
		R = 0;
	}
	if (G < 0)
	{
		G = 0;
	}
	if (B < 0)
	{
		B = 0;
	}
	
	if (R > max_val)
	{
		R = max_val;
	}
	if (G > max_val)
	{
		G = max_val;
	}
	if (B > max_val)
	{
		B = max_val;
	}
}

void RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h)
{
	custom_assert(ImIn.size() >= w*h, "RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImY.size() >= w*h, "RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h)\nnot: ImY.size() >= w*h");
	custom_assert(ImI.size() >= w*h, "RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h)\nnot: ImI.size() >= w*h");
	custom_assert(ImQ.size() >= w*h, "RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h)\nnot: ImQ.size() >= w*h");

	concurrency::parallel_for(0, h, [&](int y)
	{
		int i = y*w;
		u8 *color;
		int r, g, b, Y, I, Q;

		for (int x = 0; x < w; x++, i++)
		{
			color = (u8*)(&ImIn[i]);

			r = color[2];
			g = color[1];
			b = color[0];

			//---(0.29889531*2^16)(0.58662247*2^16)(0.11448223*2^16)-----
			Y = 19588 * r + 38445 * g + 7503 * b;

			//---(0.59597799*2^16)(-0.27417610*2^16)(-0.32180189*2^16)-----      
			I = 39058 * r - 17968 * g - 21090 * b;

			//---(0.21147017*2^16)(-0.52261711*2^16)(0.31114694*2^16)---
			Q = 13859 * r - 34250 * g + 20391 * b;

			ImY[i] = Y >> 16;

			if (I >= 0)
			{
				ImI[i] = I >> 16;
			}
			else
			{
				I = -I;
				ImI[i] = -(I >> 16);
			}

			if (Q >= 0)
			{
				ImQ[i] = Q >> 16;
			}
			else
			{
				Q = -Q;
				ImQ[i] = -(Q >> 16);
			}
		}
	});
}

void GetGrayscaleImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, int w, int h)
{
	u8 *color;
	int i, r, g, b, wh;

	custom_assert(ImIn.size() >= w*h, "GetGrayscaleImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImY.size() >= w*h, "GetGrayscaleImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, int w, int h)\nnot: ImY.size() >= w*h");

	wh = w*h;

	for(i=0; i<wh; i++)
	{
		color = (u8*)(&ImIn[i]);

		r = color[2];
		g = color[1];
		b = color[0];
		
		ImY[i] = (19595*r + 38470*g + 7471*b) >> 16;
	}
}

void SobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)
{
	int i, x, y, mx, my, val, val1, val2, val3, val4, max;

	custom_assert(ImIn.size() >= w*h, "SobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImMOE.size() >= w*h, "SobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)\nnot: ImMOE.size() >= w*h");

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{		
		//val1 = lt - rb;
		val1 = ImIn[i-w-1] - ImIn[i+w+1];
		//val2 = rt - lb;
		val2 = ImIn[i-w+1] - ImIn[i+w-1];
		//val3 = mt - mb;
		val3 = ImIn[i-w] - ImIn[i+w];
		//val4 = lm - rm;
		val4 = ImIn[i-1] - ImIn[i+1];

		//val = lt + rt - lb - rb + 2*(mt-mb);
		val = val1 + val2 + 2*val3;
		if (val<0) max = -val;
		else max = val;

		//val = lt - rt + lb - rb + 2*(lm-rm);
		val = val1 - val2 + 2*val4;
		if (val<0) val = -val;
		if (max<val) max = val;

		//val = mt + lm - rm - mb + 2*(lt-rb);
		val = val3 + val4 + 2*val1;
		if (val<0) val = -val;
		if (max<val) max = val;

		//val = mt + rm - lm - mb + 2*(rt-lb);
		val = val3 - val4 + 2*val2;
		if (val<0) val = -val;
		if (max<val) max = val;

		ImMOE[i] = max;
	}
}

void ImprovedSobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)
{
	const int mx = w - 1;
	const int my = h - 1;
	__int64 t1, dt, num_calls;

	custom_assert(ImIn.size() >= w*h, "ImprovedSobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImMOE.size() >= w*h, "ImprovedSobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h)\nnot: ImMOE.size() >= w*h");

	concurrency::parallel_for(1, my, [&](int y)
	{
		int x, val, val1, val2, val3, val4, max;
		int* pIm, *pImMOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImMOE = &ImMOE[y*w + x]; x < mx; x++, pIm++, pImMOE++)
		{
			//val1 = lt - rb;
			val1 = *(pIm - w - 1) - *(pIm + w + 1);
			//val2 = rt - lb;
			val2 = *(pIm - w + 1) - *(pIm + w - 1);
			//val3 = mt - mb;
			val3 = *(pIm - w) - *(pIm + w);
			//val4 = lm - rm;
			val4 = *(pIm - 1) - *(pIm + 1);

			//val = lt + rt - lb - rb + 2*(mt-mb);
			val = 3 * (val1 + val2) + 10 * val3;
			if (val < 0) max = -val;
			else max = val;

			//val = lt - rt + lb - rb + 2*(lm-rm);
			val = 3 * (val1 - val2) + 10 * val4;
			if (val < 0) val = -val;
			if (max < val) max = val;

			//val = mt + lm - rm - mb + 2*(lt-rb);
			val = 3 * (val3 + val4) + 10 * val1;
			if (val < 0) val = -val;
			if (max < val) max = val;

			//val = mt + rm - lm - mb + 2*(rt-lb);
			val = 3 * (val3 - val4) + 10 * val2;
			if (val < 0) val = -val;
			if (max < val) max = val;

			*pImMOE = max;
		}
	});
}

void SobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)
{
	int i, ii, x, y, mx, my, val;

	custom_assert(ImIn.size() >= w*h, "SobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImHOE.size() >= w*h, "SobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImHOE.size() >= w*h");

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ii = i - (w+1);
		val = ImIn[ii] + 2*ImIn[ii+1] + ImIn[ii+2];

		ii += w;
		//val += 0*ImIn[ii] + 0*ImIn[ii+1] + 0*ImIn[ii+2];

		ii += w;
		val -= ImIn[ii] + 2*ImIn[ii+1] + ImIn[ii+2];

		if (val<0) ImHOE[i] = -val;
		else ImHOE[i] = val;
	}
}

void FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2;

	custom_assert(ImIn.size() >= w*h, "FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImHOE.size() >= w*h, "FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImHOE.size() >= w*h");

	int* pIm = &ImIn[0];
	int* pImHOE = &ImHOE[0];

	mx = w-1;
	my = h-1;
	pIm += w+1;
	pImHOE += w+1;
	for(y=1; y<my; y++, pIm += 2, pImHOE += 2)
	for(x=1; x<mx; x++, pIm++, pImHOE++)
	{
		val1 = *(pIm - w - 1) + *(pIm - w + 1);
		val2 = *(pIm - w);

		val1 -= *(pIm + w - 1) + *(pIm + w + 1);
		val2 -= *(pIm + w);

		val = 3*val1 + 10*val2;

		if (val < 0) val = -val;
		*pImHOE = val;
	}
}

void FastSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)
{
	int i, ii, x, y, mx, my, val;

	custom_assert(ImIn.size() >= w*h, "FastSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImVOE.size() >= w*h, "FastSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImVOE.size() >= w*h");

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ii = i - (w+1);
		val = ImIn[ii] - ImIn[ii+2];

		ii += w;
		val += 2*(ImIn[ii] - ImIn[ii+2]);

		ii += w;
		val += ImIn[ii] - ImIn[ii+2];

		if (val<0) val = -val;
		ImVOE[i] = val;
	}
}

void FastImprovedSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2, val3;

	custom_assert(ImIn.size() >= w*h, "FastImprovedSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImVOE.size() >= w*h, "FastImprovedSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImVOE.size() >= w*h");

	int* pIm = &ImIn[0];
	int* pImVOE = &ImVOE[0];

	mx = w-1;
	my = h-1;
	pIm += w+1;
	pImVOE += w+1;
	for(y=1; y<my; y++, pIm += 2, pImVOE += 2)
	{		
		val1 = 3*(*(pIm - 1 - w) + *(pIm - 1 + w)) + 10*(*(pIm - 1));
		
		val2 = 3*(*(pIm - w) + *(pIm + w)) + 10*(*pIm);

		for(x=1; x<mx; x++, pIm++, pImVOE++)
		{
			val3 = 3*(*(pIm + 1 - w) + *(pIm + 1 + w)) + 10*(*(pIm + 1));
			
			val = val1 - val3;

			if (val<0) val = -val;
			*pImVOE = val;

			val1 = val2;
			val2 = val3;
		}
	}
}

void FullSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE1, custom_buffer<int> &ImVOE2, int w, int h)
{
	int i, ii, x, y, mx, my, val, size, hvt, rhvt, MX;

	custom_assert(ImIn.size() >= w*h, "FullSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE1, custom_buffer<int> &ImVOE2, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImVOE1.size() >= w*h, "FullSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE1, custom_buffer<int> &ImVOE2, int w, int h)\nnot: ImVOE1.size() >= w*h");
	custom_assert(ImVOE2.size() >= w*h, "FullSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE1, custom_buffer<int> &ImVOE2, int w, int h)\nnot: ImVOE2.size() >= w*h");

	MX = 0;
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ii = i - (w+1);
		val = ImIn[ii] - ImIn[ii+2];

		ii += w;
		val += 2*(ImIn[ii] - ImIn[ii+2]);

		ii += w;
		val += ImIn[ii] - ImIn[ii+2];

		if (val<0) val = -val;
		if (val>MX) MX = val;

		ImVOE1[i] = val;
	}

	hvt = g_hvt;
	rhvt = (hvt*MX)/255;
	size = w*h;

	for(i=0; i<size; i++)
	{			
			val = ImVOE1[i];
			
			if (val >= rhvt) ImVOE2[i] = 255;
			else ImVOE2[i] = 0;

			if (val >= hvt) ImVOE1[i] = 255;
			else ImVOE1[i] = 0;
	}
}

void SobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)
{
	int i, ii, x, y, mx, my, val, size, hvt, rhvt, MX;

	custom_assert(ImIn.size() >= w*h, "SobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImVOE.size() >= w*h, "SobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h)\nnot: ImVOE.size() >= w*h");

	MX = 0;
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ii = i - (w+1);
		val = 3*(ImIn[ii] - ImIn[ii+2]);

		ii += w;
		val += 10*(ImIn[ii] - ImIn[ii+2]);

		ii += w;
		val += 3*(ImIn[ii] - ImIn[ii+2]);

		if (val<0) val = -val;
		if (val>MX) MX = val;

		ImVOE[i] = val;
	}

	hvt = g_hvt;
	rhvt = (hvt*MX)/255;
	size = w*h;

	for(i=0; i<size; i++)
	{			
			if (ImVOE[i] >= rhvt) ImVOE[i] = 255;
			else ImVOE[i] = 0;
	}
}

void SobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)
{
	int i, j, k;
	int	mx, val;
	int lt,mt,rt, lm,mm,rm, lb,mb,rb;
	int blt,bmt,brt, blm,bmm,brm, blb,bmb,brb;

	custom_assert(ImIn.size() >= w*h, "SobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImNOE.size() >= w*h, "SobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)\nnot: ImNOE.size() >= w*h");

	mx = 0;

	k = w + 1;

	// специально несоответствие затем все сдвинется
	blm = ImIn[k - w - 1];
	bmm = ImIn[k - w];
	brm = ImIn[k - w + 1];

	blb = ImIn[k - 1];
	bmb = ImIn[k];
	brb = ImIn[k + 1];

	for(i=1; i<h-1; i++)
	{
		lt = blt = blm;
		mt = bmt = bmm;
		rt = brt = brm;

		lm = blm = blb;
		mm = bmm = bmb;
		rm = brm = brb;

		lb = blb = ImIn[k + w - 1];
		mb = bmb = ImIn[k + w];
		rb = brb = ImIn[k + w + 1];
		
		for (j=1; j<w-1; j++, k++)
		{
			val = mt + lm - rm - mb + 2*(lt-rb);
			if (val<0) ImNOE[k] = -val;
			else ImNOE[k] = val;

			lb = mb;
			lm = mm;
			lt = mt;

			mb = rb;
			mm = rm;
			mt = rt;
			
			rt = ImIn[k - w + 1];
			rm = ImIn[k + 1];
			rb = ImIn[k + w + 1];
		}
		k++;
	}
}

void FastImprovedSobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)
{
	custom_assert(ImIn.size() >= w*h, "FastImprovedSobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImNOE.size() >= w*h, "FastImprovedSobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h)\nnot: ImNOE.size() >= w*h");

	__int64 t1, dt, num_calls;

	const int mx = w - 1;
	const int my = h - 1;

	concurrency::parallel_for(1, my, [&](int y)
	{
		int x, val, val1, val2;
		int* pIm, *pImNOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImNOE = &ImNOE[y*w + x]; x < mx; x++, pIm++, pImNOE++)
		{
			val1 = *(pIm - w);
			val2 = *(pIm - w - 1);

			val1 += *(pIm - 1) - *(pIm + 1);

			val1 -= *(pIm + w);
			val2 -= *(pIm + w + 1);

			val = 3 * val1 + 10 * val2;

			if (val < 0) val = -val;
			*pImNOE = val;
		}
	});
}

void SobelSEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImSOE, int w, int h)
{
	int i, j, k;
	int	mx, val;
	int lt,mt,rt, lm,mm,rm, lb,mb,rb;
	int blt,bmt,brt, blm,bmm,brm, blb,bmb,brb;

	custom_assert(ImIn.size() >= w*h, "SobelSEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImSOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImSOE.size() >= w*h, "SobelSEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImSOE, int w, int h)\nnot: ImSOE.size() >= w*h");

	mx = 0;

	k = w + 1;

	// специально несоответствие затем все сдвинется
	blm = ImIn[k - w - 1];
	bmm = ImIn[k - w];
	brm = ImIn[k - w + 1];

	blb = ImIn[k - 1];
	bmb = ImIn[k];
	brb = ImIn[k + 1];

	for(i=1; i<h-1; i++)
	{
		lt = blt = blm;
		mt = bmt = bmm;
		rt = brt = brm;

		lm = blm = blb;
		mm = bmm = bmb;
		rm = brm = brb;

		lb = blb = ImIn[k + w - 1];
		mb = bmb = ImIn[k + w];
		rb = brb = ImIn[k + w + 1];
		
		for (j=1; j<w-1; j++, k++)
		{
			val = mt + rm - lm - mb + 2*(rt-lb);
			if (val<0) ImSOE[k] = -val;
			else ImSOE[k] = val;

			lb = mb;
			lm = mm;
			lt = mt;

			mb = rb;
			mm = rm;
			mt = rt;
			
			rt = ImIn[k - w + 1];
			rm = ImIn[k + 1];
			rb = ImIn[k + w + 1];
		}
		k++;
	}
}

void IncreaseContrastOperator(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, int w, int h)
{
	int i, ii, x, y, mx, my, val;

	custom_assert(ImIn.size() >= w*h, "IncreaseContrastOperator(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImRES.size() >= w*h, "IncreaseContrastOperator(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, int w, int h)\nnot: ImRES.size() >= w*h");

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ii = i - (w+1);
		val = -ImIn[ii+1];

		ii += w;
		val += 9*ImIn[ii+1] - ImIn[ii] - ImIn[ii+2];

		ii += w;
		val -= ImIn[ii+1];

		val = val/5;
		if (val<0) val = 0;
		if (val>255) val = 255;

		ImRES[i] = val;
	}
}

void FindAndApplyGlobalThreshold(custom_buffer<int> &Im, int w, int h)
{
	int i, imx, mx, start=3, dx=5;
	int beg, end, val, MX, thr;
	custom_buffer<int> edgeStr(MAX_EDGE_STR, 0);

	custom_assert(Im.size() >= w*h, "FindAndApplyGlobalThreshold(custom_buffer<int> &Im, int w, int h)\nnot: Im.size() >= w*h");

	MX = 0;

	for(i=0; i<w*h; i++)
	{
		val = Im[i];
		
		if (val > MX) 
		{
			MX = val;			
		}

		edgeStr[val]++;
	}

	imx = start;
	mx = edgeStr[imx];
	for(i=start; i<=20; i++)
	{
		if(edgeStr[i] > mx) 
		{
			imx = i;
			mx = edgeStr[i];
		}
	}

	beg = imx-dx;
	if (beg<start) beg = start;
	end = beg+2*dx; 

	val = 0;
	for(i=beg; i<=end; i++)
	{
		val += edgeStr[i]; 
	}
	val /= (end-beg+1);
	val = (9*val)/10;

	i = imx+1;

	while((edgeStr[i] > val) && (i < MX)) i++;
	thr = i;

	for(i=0; i<w*h; i++)
	{
		if (Im[i]<thr) Im[i] = 0;
	}

	memset(&edgeStr[0], 0, (MX+1)*sizeof(int));
}

void FindAndApplyLocalThresholding(custom_buffer<int> &Im, int dw, int dh, int w, int h)
{
	int i, di, ia, da, x, y, nx, ny, mx, my, MX;
	int val, min, max, mid, lmax, rmax, li, ri, thr;
	custom_buffer<int> edgeStr(MAX_EDGE_STR, 0);
	
	custom_assert(Im.size() >= w*h, "FindAndApplyLocalThresholding(custom_buffer<int> &Im, int dw, int dh, int w, int h)\nnot: Im.size() >= w*h");

	MX = 0;

	for(i=0; i<w*h; i++)
	{
		val = Im[i];
		
		if (val > MX) 
		{
			MX = val;			
		}
	}

	mx = w/dw;
	my = h/dh;

	da = dh*w-mx*dw;
	di = w-dw;

	ia = 0;
	for(ny=0; ny<my; ny++, ia+=da)
	for(nx=0; nx<mx; nx++, ia+=dw)
	{
		min = MX;
		max = 0;
		i = ia;
		
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			val = Im[i];
			
			if (val == 0) continue;
			if (val > max) max = val;						
			if (val < min) min = val;

			edgeStr[val]++;			
		}
		mid = (min+max)/2;

		li = min;
		lmax = edgeStr[li];
		for(i=min; i<mid; i++)
		{
			if (edgeStr[i]>lmax)
			{
				li = i;
				lmax = edgeStr[li];
			}
		}

		ri = mid;
		rmax = edgeStr[ri];
		for(i=mid; i<=max; i++)
		{
			if (edgeStr[i]>rmax)
			{
				ri = i;
				rmax = edgeStr[ri];
			}
		}

		if (lmax<rmax) 
		{
			thr = li;
			val = lmax;
		}
		else
		{
			thr = ri;
			val = rmax;
		}

		for(i=li+1; i<ri; i++)
		{
			if (edgeStr[i]<val) 
			{
				thr = i;
				val = edgeStr[i];
			}
		}

		i = ia;
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			if (Im[i]<thr) Im[i] = 0;
		}

		memset(&edgeStr[0], 0, (MX+1)*sizeof(int));
	}

	dh = h%dh;
	if (dh == 0) return;

	ia = (h-dh)* w;
	for(nx=0; nx<mx; nx++, ia+=dw)
	{
		min = MX;
		max = 0;
		i = ia;
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			val = Im[i];
			
			if (val == 0) continue;
			if (val > max) max = val;						
			if (val < min) min = val;
			
			edgeStr[val]++;			
		}
		mid = (min+max)/2;

		li = min;
		lmax = edgeStr[li];
		for(i=min; i<mid; i++)
		{
			if (edgeStr[i]>lmax)
			{
				li = i;
				lmax = edgeStr[li];
			}
		}

		ri = mid;
		rmax = edgeStr[ri];
		for(i=mid; i<=max; i++)
		{
			if (edgeStr[i]>rmax)
			{
				ri = i;
				rmax = edgeStr[ri];
			}
		}

		if (lmax<rmax) 
		{
			thr = li;
			val = lmax;
		}
		else
		{
			thr = ri;
			val = rmax;
		}

		for(i=li+1; i<ri; i++)
		{
			if (edgeStr[i]<val) 
			{
				thr = i;
				val = edgeStr[i];
			}
		}

		i = ia;
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			if (Im[i]<thr) Im[i] = 0;
		}
		
		memset(&edgeStr[0], 0, (MX+1)*sizeof(int));
	}
}

void ApplyModerateThreshold(custom_buffer<int> &Im, double mthr, int w, int h)
{
	int thr;
	int mx = 0;
	int *pIm = &Im[0];
	int *pImMAX = pIm + w*h;
	
	custom_assert(Im.size() >= w*h, "ApplyModerateThreshold(custom_buffer<int> &Im, double mthr, int w, int h)\nnot: Im.size() >= w*h");

	for(; pIm < pImMAX; pIm++)
	{
		if (*pIm > mx) mx = *pIm;
	}

	thr = (int)((double)mx*mthr);

	for(pIm = &Im[0]; pIm < pImMAX; pIm++)
	{
		if (*pIm < thr) *pIm = 0;
		else *pIm = 255;
	}
}

void AplyESS(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)
{
	__int64 t1, dt, num_calls;

	custom_assert(ImIn.size() >= w * h, "AplyESS(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImOut.size() >= w * h, "AplyESS(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)\nnot: ImOut.size() >= w*h");

	const int mx = w - 2;
	const int my = h - 2;

	concurrency::parallel_for(2, my, [&](int y)
	{
		int i, x, val;

		for (x = 2, i = w * y + x; x < mx; x++, i++)
		{
			val = 2 * (ImIn[i - w * 2 - 2] + ImIn[i - w * 2 + 2] + ImIn[i + w * 2 - 2] + ImIn[i + w * 2 + 2]) +
				+4 * (ImIn[i - w * 2 - 1] + ImIn[i - w * 2 + 1] + ImIn[i - w - 2] + ImIn[i - w + 2] + ImIn[i + w - 2] + ImIn[i + w + 2] + ImIn[i + w * 2 - 1] + ImIn[i + w * 2 + 1]) +
				+5 * (ImIn[i - w * 2] + ImIn[i - 2] + ImIn[i + 2] + ImIn[i + w * 2]) +
				+10 * (ImIn[i - w - 1] + ImIn[i - w + 1] + ImIn[i + w - 1] + ImIn[i + w + 1]) +
				+20 * (ImIn[i - w] + ImIn[i - 1] + ImIn[i + 1] + ImIn[i + w]) +
				+40 * ImIn[i];

			ImOut[i] = val / 220;
		}
	});
}

void AplyECP(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)
{	
	custom_assert(ImIn.size() >= w*h, "AplyECP(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImOut.size() >= w*h, "AplyECP(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h)\nnot: ImOut.size() >= w*h");

	__int64 t1, dt, num_calls;

	const int mx = w - 2;
	const int my = h - 2;
	
	concurrency::parallel_for(2, my, [&](int y)
	{
		int i, ii, x, val;

		for (x = 2, i = w * y + x; x < mx; x++, i++)
		{
			if (ImIn[i] == 0)
			{
				ImOut[i] = 0;
				continue;
			}

			/*val = 8*(ImIn[i - w*2 - 2] + ImIn[i - w*2 + 2] + ImIn[i + w*2 - 2] + ImIn[i + w*2 + 2]) +
				+ 5*(ImIn[i - w*2 - 1] + ImIn[i - w*2 + 1] + ImIn[i - w - 2] + ImIn[i - w + 2] + ImIn[i + w - 2] + ImIn[i + w + 2] + ImIn[i + w*2 - 1] + ImIn[i + w*2 + 1]) +
				+ 4*(ImIn[i - w*2] + ImIn[i - 2] + ImIn[i + 2] + ImIn[i + w*2]) +
				+ 2*(ImIn[i - w - 1] + ImIn[i - w + 1] + ImIn[i + w - 1] + ImIn[i + w + 1]) +
				+ 1*(ImIn[i - w] + ImIn[i - 1] + ImIn[i + 1] + ImIn[i + w]) +
				+ 0*ImIn[i];*/

			ii = i - ((w + 1) << 1);
			val = 8 * ImIn[ii] + 5 * ImIn[ii + 1] + 4 * ImIn[ii + 2] + 5 * ImIn[ii + 3] + 8 * ImIn[ii + 4];

			ii += w;
			val += 5 * ImIn[ii] + 2 * ImIn[ii + 1] + ImIn[ii + 2] + 2 * ImIn[ii + 3] + 5 * ImIn[ii + 4];

			ii += w;
			val += 4 * ImIn[ii] + ImIn[ii + 1] + ImIn[ii + 3] + 4 * ImIn[ii + 4];

			ii += w;
			val += 5 * ImIn[ii] + 2 * ImIn[ii + 1] + ImIn[ii + 2] + 2 * ImIn[ii + 3] + 5 * ImIn[ii + 4];

			ii += w;
			val += 8 * ImIn[ii] + 5 * ImIn[ii + 1] + 4 * ImIn[ii + 2] + 5 * ImIn[ii + 3] + 8 * ImIn[ii + 4];

			ImOut[i] = val / 100;
		}
	});
}

void BorderClear(custom_buffer<int> &Im, int dd, int w, int h)
{
	int i, di, x, y;

	custom_assert(Im.size() >= w*h, "BorderClear(custom_buffer<int> &Im, int dd, int w, int h)\nnot: Im.size() >= w*h");

	memset(&Im[0], 0, w*dd*sizeof(int));
	memset(&Im[w*(h-dd)], 0, w*dd*sizeof(int));

	i = 0;
	di = w-dd;
	for(y=0; y<h; y++, i+=di)
	for(x=0; x<dd; x++, i++)
	{
		Im[i] = 0;
	}

	i = w-dd;
	for(y=0; y<h; y++, i+=di)
	for(x=0; x<dd; x++, i++)
	{
		Im[i] = 0;
	}
}

void EasyBorderClear(custom_buffer<int> &Im, int w, int h)
{
	int i, y;

	custom_assert(Im.size() >= w*h, "BorderClear(custom_buffer<int> &Im, int dd, int w, int h)\nnot: Im.size() >= w*h");

	memset(&Im[0], 0, w*sizeof(int));
	memset(&Im[w*(h-1)], 0, w*sizeof(int));

	i = 0;
	for(y=0; y<h; y++, i+=w)
	{
		Im[i] = 0;
	}

	i = w-1;
	for(y=0; y<h; y++, i+=w)
	{
		Im[i] = 0;
	}
}

inline void CombineTwoImages(custom_buffer<int> &ImRes, custom_buffer<int> &Im2, int w, int h, int white = 255)
{
	int i, size;

	size = w*h;
	for(i=0; i<size; i++) 
	{
		if (Im2[i] != 0)
		{
			ImRes[i] = white;
		}
	}
}

inline void IntersectTwoImages(custom_buffer<int> &ImRes, custom_buffer<int> &Im2, int w, int h)
{
	int i, size;

	size = w*h;
	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 0)
		{
			ImRes[i] = 0;
		}
	}
}

void CombineStrengthOfTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int w, int h)
{
	int i, size;

	size = w*h;
	for(i=0; i<size; i++) 
	{
		if (Im2[i] > Im1[i])
		{
			Im1[i] = Im2[i];
		}
	}
}

void GetImCMOEWithThr1(custom_buffer<int> &ImCMOE, custom_buffer<int> &ImYMOE, custom_buffer<int> &ImUMOE, custom_buffer<int> &ImVMOE, int w, int h, custom_buffer<int> &offsets, custom_buffer<int> &dhs, int N)
{
	custom_buffer<int> ImRES2(w*h, 0), ImRES3(w*h, 0);
	int mx, my, i, k, y, x;
	mx = w - 1;
	my = h - 1;
	i = w + 1;

	EasyBorderClear(ImCMOE, w, h);	

	for (y = 1; y < my; y++, i += 2)
	{
		for (x = 1; x < mx; x++, i++)
		{
			ImCMOE[i] = ImYMOE[i] + ImUMOE[i] + ImVMOE[i];
		}
	}

	FindAndApplyGlobalThreshold(ImCMOE, w, h);
	FindAndApplyLocalThresholding(ImCMOE, w, 32, w, h);

	AplyESS(ImCMOE, ImRES2, w, h);
	AplyECP(ImRES2, ImRES3, w, h);	

	BorderClear(ImCMOE, 2, w, h);

	mx = w - 2;
	my = h - 2;
	i = ((w + 1) << 1);
	for (y = 2; y < my; y++, i += 4)
	{
		for (x = 2; x < mx; x++, i++)
		{
			ImCMOE[i] = (ImRES2[i] + ImRES3[i]) / 2;
		}
	}

	for (k = 0; k < N; k++)
	{
		i = offsets[k];
		ApplyModerateThreshold(ImCMOE.get_sub_buffer(i), g_mthr, w, dhs[k]);
	}
}


void GetImCMOEWithThr2(custom_buffer<int> &ImCMOE, custom_buffer<int> &ImYMOE, custom_buffer<int> &ImUMOE, custom_buffer<int> &ImVMOE, int w, int h, custom_buffer<int> &offsets, custom_buffer<int> &dhs, int N)
{
	custom_buffer<int> ImRES5(w*h, 0), ImRES6(w*h, 0);
	int mx, my, i, k, y, x;
	mx = w - 1;
	my = h - 1;
	i = w + 1;

	EasyBorderClear(ImCMOE, w, h);

	mx = w - 1;
	my = h - 1;
	i = w + 1;
	for (y = 1; y < my; y++, i += 2)
	{
		for (x = 1; x < mx; x++, i++)
		{
			ImCMOE[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
		}
	}

	FindAndApplyGlobalThreshold(ImCMOE, w, h);
	FindAndApplyLocalThresholding(ImCMOE, w, 32, w, h);

	AplyESS(ImCMOE, ImRES5, w, h);
	AplyECP(ImRES5, ImRES6, w, h);

	BorderClear(ImCMOE, 2, w, h);

	mx = w - 2;
	my = h - 2;
	i = ((w + 1) << 1);
	for (y = 2; y < my; y++, i += 4)
		for (x = 2; x < mx; x++, i++)
		{
			ImCMOE[i] = (ImRES5[i] + ImRES6[i]) / 2;
		}

	i = 0;
	for (k = 0; k < N; k++)
	{
		i = offsets[k];
		ApplyModerateThreshold(ImCMOE.get_sub_buffer(i), g_mthr, w, dhs[k]);
	}
}

void GetImFF(custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImRGB, custom_buffer<int> &ImYFull, custom_buffer<int> &ImUFull, custom_buffer<int> &ImVFull, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h)
{	
	custom_buffer<int> offsets(N, 0), cnts(N, 0), dhs(N, 0);
	int i, k, cnt, val;
	int x, y, segh;
	int ww, hh;
	__int64 t1, dt, num_calls;
	
	ww = w;
	hh = 0;
	i = 0;
	for (k = 0; k < N; k++)
	{
		offsets[k] = i;
		dhs[k] = LE[k] - LB[k] + 1;
		cnts[k] = w * dhs[k];		
		i += cnts[k];
		hh += dhs[k];
	}

	custom_buffer<int> ImY(ww*hh, 0), ImU(ww*hh, 0), ImV(ww*hh, 0), ImYMOE(ww*hh, 0), ImUMOE(ww*hh, 0), ImVMOE(ww*hh, 0);
	custom_buffer<int> ImRES1(ww*hh, 0), ImRES4(ww*hh, 0);
	
	for(k=0; k<N; k++)
	{
		i = offsets[k];
		cnt = cnts[k];				
		memcpy(&ImRES1[i], &ImRGB[w*LB[k]], cnt * sizeof(int));
		memcpy(&ImY[i], &ImYFull[w*LB[k]], cnt * sizeof(int));
		memcpy(&ImU[i], &ImUFull[w*LB[k]], cnt * sizeof(int));
		memcpy(&ImV[i], &ImVFull[w*LB[k]], cnt * sizeof(int));
	}
		
	concurrency::parallel_invoke(
		[&] { ImprovedSobelMEdge(ImY, ImYMOE, ww, hh); },
		[&] { ImprovedSobelMEdge(ImU, ImUMOE, ww, hh); },
		[&] { ImprovedSobelMEdge(ImV, ImVMOE, ww, hh); }
	);		
	
	concurrency::parallel_invoke(
		[&] { GetImCMOEWithThr1(ImRES4, ImYMOE, ImUMOE, ImVMOE, ww, hh, offsets, dhs, N); },
		[&] { GetImCMOEWithThr2(ImRES1, ImYMOE, ImUMOE, ImVMOE, ww, hh, offsets, dhs, N); }
	);

	CombineTwoImages(ImRES1, ImRES4, ww, hh);
		
	memset(&ImFF[0], 0, w*h * sizeof(int));

	i = 0;
	for(k=0; k<N; k++)
	{
		i = offsets[k];
		cnt = cnts[k];
		memcpy(&ImFF[w*LB[k]], &ImRES1[i], cnt*sizeof(int));
	}	

	memcpy(&ImSF[0], &ImFF[0], w*h * sizeof(int));

	segh = g_segh;
	for (k = 0; k < N; k++)
	{
		val = LB[k] % segh;
		LB[k] -= val;

		val = LE[k] % segh;
		if (val > 0) val = segh - val;
		if (LE[k] + val < h) LE[k] += val;
	}

	if ((LE[N - 1] + g_segh) > h)
	{
		val = LE[N - 1] - (h - g_segh);
		LE[N - 1] = h - g_segh;

		memset(&ImSF[w*(LE[N - 1] + 1)], 0, w*val * sizeof(int));
	}
}

void GetImNE(custom_buffer<int> &ImNE, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)
{
	custom_buffer<int> ImYMOE(w*h, 0), ImUMOE(w*h, 0), ImVMOE(w*h, 0);
	custom_buffer<int> ImRES1(w*h, 0);
	int k, cnt, val, N, mx, my;
	int segh;
	int ww, hh;
	__int64 t1, dt, num_calls;

	EasyBorderClear(ImNE, w, h);

	concurrency::parallel_invoke(
		[&] { FastImprovedSobelNEdge(ImY, ImYMOE, w, h); },
		[&] { FastImprovedSobelNEdge(ImU, ImUMOE, w, h); },
		[&] { FastImprovedSobelNEdge(ImV, ImVMOE, w, h); }
	);
	
	mx = w - 1;
	my = h - 1;
		
	concurrency::parallel_invoke(
		[&] { 
			int i = w + 1, x, y;
			for (y = 1; y < my; y++, i += 2)
			{
				for (x = 1; x < mx; x++, i++)
				{
					ImRES1[i] = ImYMOE[i] + ImUMOE[i] + ImVMOE[i];
				}
			}
			ApplyModerateThreshold(ImRES1, g_mnthr, w, h);
		},
		[&] {
			int i = w + 1, x, y;
			for (y = 1; y < my; y++, i += 2)
			{
				for (x = 1; x < mx; x++, i++)
				{
					ImNE[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
				}
			}
			ApplyModerateThreshold(ImNE, g_mnthr, w, h);
		}
	);	

	CombineTwoImages(ImNE, ImRES1, w, h);
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int GetTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, int w, int h, int W, int H)
{
	custom_buffer<int> LB(h, 0), LE(h, 0), ImY(w*h, 0), ImU(w*h, 0), ImV(w*h, 0);
	custom_buffer<int> ImRES1(w*h, 0);
	int N;
	int res;	
	__int64 t1, dt, num_calls = 100;

	res = 0;	

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\GetTransformedImage_01_ImRGB" + g_im_save_format, w, h);	

	ColorFiltration(ImRGB, LB, LE, N, w, h);

	if (N == 0)
	{
		return res;
	}		

	RGB_to_YIQ(ImRGB, ImY, ImU, ImV, w, h);	
	
	concurrency::parallel_invoke(
		[&] { GetImFF(ImFF, ImSF, ImRGB, ImY, ImU, ImV, LB, LE, N, w, h); },
		[&] { GetImNE(ImNE, ImY, ImU, ImV, w, h); }
	);	

	if (g_show_results) SaveGreyscaleImage(ImFF, "\\TestImages\\GetTransformedImage_02_1_ImFF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "\\TestImages\\GetTransformedImage_02_2_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "\\TestImages\\GetTransformedImage_02_3_ImNE" + g_im_save_format, w, h);

	res = SecondFiltration(ImSF, ImRGB, ImNE, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImSF, "\\TestImages\\GetTransformedImage_05_ImSFFSecondFiltration" + g_im_save_format, w, h);

	memcpy(&ImTF[0], &ImSF[0], w*h*sizeof(int));
	if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);	
	memcpy(&ImRES1[0], &ImTF[0], w*h * sizeof(int));
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_06_ImTFFThirdFiltration" + g_im_save_format, w, h);

	if (res == 1) res = ClearImageFromSmallSymbols(ImTF, w, h, 255);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_07_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);

	if (res == 1) res = SecondFiltration(ImTF, ImRGB, ImNE, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_08_ImTFFSecondFiltration" + g_im_save_format, w, h);

	if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_09_ImTFFThirdFiltration" + g_im_save_format, w, h);		

	if (res == 1) RestoreStillExistLines(ImTF, ImRES1, w, h);	
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_10_ImTFWithRestoredStillExistLines" + g_im_save_format, w, h);

	if (res == 1) ExtendImFWithDataFromImNF(ImTF, ImFF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_12_ImTFExtByImFF" + g_im_save_format, w, h);

	return res;
}

void FreeImage(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h)
{
	custom_buffer<int> LLB(h, 0), LLE(h, 0);
	int i, j;
	
	if (LB[0] > 0)
	{
		LLB[0] = 0;
		LLE[0] = LB[0]-1;
		
		j = 1;
	}
	else
	{
		j = 0;
	}

	for(i=0; i<N-1; i++)
	{
		if (LB[i+1]-LE[i] > 1)
		{
			LLB[j] = LE[i]+1;
			LLE[j] = LB[i+1]-1;
			
			j++;
		}
	}

	if (LE[N-1] < h-1) 
	{
		LLB[j] = LE[N-1]+1;
		LLE[j] = h-1;
		
		j++;
	}

	for (i=0; i<j; i++)
	{
		memset(&Im[w*LLB[i]], 0, w*(LLE[i]-LLB[i]+1)*sizeof(int));
	}
}

void UnpackImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h)
{
	int i, k, cnt;

	FreeImage(ImRES, LB, LE, LN, w, h);

	i = 0;
	for(k=0; k<LN; k++)
	{
		cnt = w*(LE[k]-LB[k]+1);		
		memcpy(&ImRES[w*LB[k]], &ImIn[i], cnt*sizeof(int));
		i += cnt;
	}		
}

///////////////////////////////////////////////////////////////////////////////
// W - full image include scale (if is) width
// H - full image include scale (if is) height
int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImRGB, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H)
{		
	std::string now;
	if (g_show_sf_results) now = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

	int res = 0;

	const int segw = g_segw;
	const int msegc = g_msegc;
	const int segh = g_segh;
	const int w_2 = w/2;
	const int dw = (int)(g_btd*(double)W);
	const int dw2 = (int)(g_tco*(double)W*2.0);
	const int mpn = g_mpn;
	const double mpd = g_mpd;
	const double mpned = g_mpned;
	const int da = segh*w;

	if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_01_Im" + g_im_save_format, w, h);
	
	for(int k=0; k<N; k++)
	{
		const int ie = (LB[k] + (int)((min(LE[k]+segh,h-1)-LB[k])/segh)*segh)*w;

		// doesn't give difference (work a little longer)
		//concurrency::parallel_for(LB[k] * w, ie, da, [&](int ia)
		//{
		for (int ia = LB[k] * w; ia < ie; ia += da)
		{
			custom_buffer<int> lb(w, 0), le(w, 0);
			int ln, ln_orig;
			int x, y, ib, i, l, ll, val, val1, val2, offset;
			int bln;
			int nNE = 0;
			int S = 0;
			int SS = 0;
			int iter = 0;

			while (1)
			{
				iter++;
				l = 0;
				bln = 0;

				// searching segments
				for (x = 0; x < w; x++)
				{
					for (y = 0, i = ia + x; y < segh; y++, i += w)
					{
						if (Im[i] == 255)
						{
							if (bln == 0)
							{
								lb[l] = le[l] = x;
								bln = 1;
							}
							else
							{
								le[l] = x;
							}
						}
					}

					if (bln == 1)
					{
						if (le[l] != x)
						{
							bln = 0;
							l++;
						}
					}
				}

				if (bln == 1)
				{
					if (le[l] == w - 1)
					{
						l++;
					}
				}
				ln = l;
				ln_orig = ln;

				if (ln == 0)
				{
					break;
				}

				l = ln - 2;
				while ((l >= 0) && (l < (ln - 1)))
				{
					//проверяем расстояние между соседними подстроками
					if ((lb[l + 1] - le[l]) > dw)
					{
						//определяем подстроку наиболее удаленую от центра
						val1 = lb[l] + le[l] - w;
						val2 = lb[l + 1] + le[l + 1] - w;
						if (val1 < 0) val1 = -val1;
						if (val2 < 0) val2 = -val2;

						offset = le[l] + lb[l] - w;
						if (offset < 0) offset = -offset;

						if (((lb[l + 1] + le[l + 1]) > w) ||
							(
							(offset <= dw2) &&
								((le[l + 1] - lb[l + 1]) < ((le[l] - lb[l])))
								)
							)
						{
							ll = l + 1;
						}
						else if (val1 > val2) ll = l;
						else ll = l + 1;

						//удаляем наиболее удаленую подстроку
						val = (le[ll] - lb[ll] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}

						for (i = ll; i < ln - 1; i++)
						{
							lb[i] = lb[i + 1];
							le[i] = le[i + 1];
						}

						ln--;

						if (l == (ln - 1)) l--;

						continue;
					}

					l--;
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia/w) + "_iter" + std::to_string(iter) + "_01_ImFBetweenTextDistace" + g_im_save_format, w, h);

				if (ln == 0)
				{
					break;
				}

				offset = le[ln - 1] + lb[0] - w;
				if (offset < 0) offset = -offset;

				// потенциальный текст слишком сильно сдвинут от центра изображения ?
				if (offset > dw2)
				{
					l = ln - 1;
					bln = 0;
					while (l > 0)
					{
						val1 = le[l - 1] + lb[0] - w;
						if (val1 < 0) val1 = -val1;

						val2 = le[l] + lb[1] - w;
						if (val2 < 0) val2 = -val2;

						if (val1 > val2)
						{
							ll = 0;
							val = (le[ll] - lb[ll] + 1) * sizeof(int);
							for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
							{
								memset(&Im[i], 0, val);
							}

							for (i = 0; i < l; i++)
							{
								lb[i] = lb[i + 1];
								le[i] = le[i + 1];
							}
						}
						else
						{
							ll = l;
							val = (le[ll] - lb[ll] + 1) * sizeof(int);
							for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
							{
								memset(&Im[i], 0, val);
							}
						}

						l--;

						if (lb[0] >= w_2)
						{
							bln = 0;
							break;
						}
						if (le[l] <= w_2)
						{
							bln = 0;
							break;
						}

						offset = le[l] + lb[0] - w;
						if (offset < 0) offset = -offset;
						if (offset <= dw2)
						{
							bln = 1;
							break;
						}
					};

					if (bln == 0)
					{
						val = (le[l] - lb[0] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}

						if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_1_ImFTextCentreOffset" + g_im_save_format, w, h);

						break;
					}

					ln = l + 1;
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_2_ImFTextCentreOffset" + g_im_save_format, w, h);

				// текст состоит всего из 2-х подстрок растояние между которыми больше их размеров ?
				if (ln == 2)
				{
					val1 = le[0] - lb[0] + 1;
					val2 = le[1] - lb[1] + 1;
					if (val1 < val2) val1 = val2;

					val2 = lb[1] - le[0] - 1;

					if (val2 > val1)
					{
						//удаляем эти под строки
						val = (le[1] - lb[0] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}

						if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_03_ImFOnly2SubStrWithBigDist" + g_im_save_format, w, h);

						break;
					}
				}

				bln = 0;
				while ((ln > 0) && (bln == 0))
				{
					S = 0;
					for (ll = 0; ll < ln; ll++)  S += le[ll] - lb[ll] + 1;

					SS = le[ln - 1] - lb[0] + 1;

					if ((double)S / (double)SS < mpd)
					{
						//определяем подстроку наиболее удаленую от центра
						val1 = lb[ln - 1] + le[ln - 1] - w;
						val2 = lb[0] + le[0] - w;
						if (val1 < 0) val1 = -val1;
						if (val2 < 0) val2 = -val2;

						offset = le[0] + lb[0] - w;
						if (offset < 0) offset = -offset;

						if (((lb[ln - 1] + le[ln - 1]) > w) ||
							(
							(offset <= dw2) &&
								((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0])))
								)
							)
						{
							ll = ln - 1;
						}
						else if (val1 > val2) ll = ln - 1;
						else ll = 0;

						//удаляем наиболее удаленую подстроку
						val = (le[ll] - lb[ll] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}

						for (i = ll; i < ln - 1; i++)
						{
							lb[i] = lb[i + 1];
							le[i] = le[i + 1];
						}

						ln--;
					}
					else
					{
						bln = 1;
					}
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_04_ImFMinPointsDensity" + g_im_save_format, w, h);

				if (ln == 0)
				{
					break;
				}

				bln = 0;
				while ((ln > 0) && (bln == 0))
				{
					// определяем число течек в строке толщиной segh
					// а также их плотность
					ib = ia;

					S = 0;
					for (ll = 0; ll < ln; ll++)  S += le[ll] - lb[ll] + 1;
					S *= segh;

					nNE = 0;
					for (y = 0; y < segh; y++, ib += w)
					{
						for (ll = 0; ll < ln; ll++)
						{
							i = ib + lb[ll];
							val = ib + le[ll];

							for (; i <= val; i++)
							{
								if (ImNE[i] == 255) nNE++;
							}
						}
					}

					if (nNE < mpn)
					{
						// removing all sub lines
						val = (le[ln-1] - lb[0] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}
						ln = 0;						

						break;
					}

					if ((double)nNE / (double)S < mpned)
					{
						//определяем подстроку наиболее удаленую от центра
						val1 = lb[ln - 1] + le[ln - 1] - w;
						val2 = lb[0] + le[0] - w;
						if (val1 < 0) val1 = -val1;
						if (val2 < 0) val2 = -val2;

						if (((lb[ln - 1] + le[ln - 1]) > w) ||
							(
							(offset <= dw2) &&
								((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0])))
								)
							)
						{
							ll = ln - 1;
						}
						else if (val1 > val2) ll = ln - 1;
						else ll = 0;

						//удаляем наиболее удаленую подстроку
						val = (le[ll] - lb[ll] + 1) * sizeof(int);
						for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
						{
							memset(&Im[i], 0, val);
						}

						for (i = ll; i < ln - 1; i++)
						{
							lb[i] = lb[i + 1];
							le[i] = le[i + 1];
						}

						ln--;
					}
					else
					{
						bln = 1;
					}
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "\\TestImages\\SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_06_ImFMinNEdgesPointsDensity" + g_im_save_format, w, h);

				if (ln == 0)
				{
					break;
				}

				if (ln == ln_orig)
				{
					if (ln > 0)
					{
						res = 1;
					}
					break;
				}
			}
		}//);
	}

	return res;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int ThirdFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H)
{
	custom_buffer<int> LL(h, 0), LR(h, 0), LLB(h, 0), LLE(h, 0), LW(h, 0), NN(h, 0);
	custom_buffer<custom_buffer<int>> LLLB(h, custom_buffer<int>(3, 0)), LLLE(h, custom_buffer<int>(3, 0));

	int wmin, wmax, nmin, bln, res, x, y, k, l, r, val, val1, val2;
	int i, j, ib, segh, N;
	int dy = H/16;
	int w_2, mw = W/10, yb, ym;

	res = 0;

	segh = g_segh;

	w_2 = w/2;

	N = 0;
	LLB[0] = -1;
	LLE[0] = -1;
	LL[0] = 0;
	LR[0] = 0;
	for (y=0, ib=0; y<h; y++, ib+=w)
	{
		bln = 0;
		l = -1;
		r = -1;
		for(x=0; x<w; x++)
		{
			if (Im[ib+x] == 255) 
			{
				if (LLB[N] == -1)
				{
					LLB[N] = y;
					LLE[N] = y;
				}
				else
				{
					LLE[N] = y;
				}

				if (l == -1) l = x;

				if  (x > r) r = x;
				
				bln = 1;
			}			
		}
		LL[y] = l;
		LR[y] = r;
		LW[y] = r-l+1;
		
		if ((bln == 0) && (LLB[N] != -1))
		{
			N++;
			LLB[N] = -1;
			LLE[N] = -1;
		}
	}
	if (LLE[N] == h-1) N++;

	bln = 1;
	nmin = 0;
	while (bln == 1)
	{
		bln = 0;

		k = nmin;
		for(; k<N; k++)
		{
			NN[k] = 1;
			y = val = yb = LLB[k];
			ym = LLE[k];

			wmin = wmax = LW[yb];

			for (y = yb; y<=ym; y++)
			{
				if (LW[y] > wmax) 
				{
					wmax = LW[y];
					val = y;
				}
			}
			
			y = val;
			while(y<=ym)
			{
				if ((double)LW[y]/(double)wmax > 0.5)
				{ 
					y++;
				}
				else
				{
					break;
				}
			}
			val2 = y-1;

			y = val;
			while(y>=yb)
			{
				if ((double)LW[y]/(double)wmax > 0.5)
				{ 
					y--;
				}
				else
				{
					break;
				}
			}
			val1 = y+1;

			if ((val1 != yb) || (val2 != ym))
			{			
				bln = 1;

				if (val1 == yb)
				{
					LLLB[k][1] = val2+1;
					LLLE[k][1] = ym;

					LLLB[k][0] = val1;
					LLLE[k][0] = val2;

					NN[k]++;
				}
				else if (val2 == ym)
				{
					LLLB[k][1] = yb;
					LLLE[k][1] = val1-1;

					LLLB[k][0] = val1;
					LLLE[k][0] = val2;

					NN[k]++;
				}
				else
				{
					LLLB[k][1] = yb;
					LLLE[k][1] = val1-1;
					LLLB[k][2] = val2+1;
					LLLE[k][2] = ym;

					LLLB[k][0] = val1;
					LLLE[k][0] = val2;

					NN[k]+=2;
				}			
			}
			else
			{
				LLLB[k][0] = yb;
				LLLE[k][0] = ym;
			}
		}

		if (bln == 1)
		{
			for (k=nmin; k<N; k++) 
			{
				LLB[k] = LLLB[k][0];
				LLE[k] = LLLE[k][0];
			}

			i = N;
			for (k=nmin; k<N; k++) 
			{
				for(l=1; l<NN[k]; l++)
				{

					LLB[i] = LLLB[k][l];
					LLE[i] = LLLE[k][l];
					i++;
				}
			}
			nmin = N;
			N = i;
		}
	}

	for (i=0; i<N-1; i++)
	for (j=i+1; j<N; j++)
	{
		k = i;
		val = LLB[k];

		if (LLB[j] < val) 
		{
			k = j;
			val = LLB[k];
		}

		if (k != i)
		{
			val1 = LLB[i];
			val2 = LLE[i];
			LLB[i] = LLB[k];
			LLE[i] = LLE[k];
			LLB[k] = val1;
			LLE[k] = val2;
		}
	}

	for (k=0; k<N; k++)
	{
		yb = LLB[k];
		ym = LLE[k];
		LL[k] = LL[yb];
		LR[k] = LR[yb];

		for (y = yb+1; y<=ym; y++)
		{
			LL[k] += LL[y];
			LR[k] += LR[y];
		}
	}

	for (k=0; k<N; k++)
	{
		LL[k] = LL[k]/(LLE[k]-LLB[k]+1);
		LR[k] = LR[k]/(LLE[k]-LLB[k]+1);

		if ( ((LLE[k]-LLB[k]+1) < segh) || 
			 (LL[k] >= w_2)
			)
		{
			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}		

		res = 1;
	}

	return res;
}

/*
// W - full image include scale (if is) width
// H - full image include scale (if is) height
int ThirdFiltrationForGFTI(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H)
{
	custom_buffer<int> LL(h, 0), LR(h, 0), LLB(h, 0), LLE(h, 0), LW(h, 0), NN(h, 0);
	custom_buffer<custom_buffer<int>> LLLB(h, custom_buffer<int>(3, 0)), LLLE(h, custom_buffer<int>(3, 0));

	int wmin, wmax, nmin, im, vmin, vmax, bln, res, x, y, k, l, r, val, val1, val2;
	int i, j, da, ib, ie, S, segh, YMIN, YMAX, K, N;
	int dy = H / 16;
	int w_2, ww, mw = W / 10, yb, ym_temp, ym, xb, xm, nHE, nNE, nVE;

	res = 0;

	segh = g_segh;

	w_2 = w / 2;

	for (K = 0; K < LN; K++)
	{
		YMIN = LB[K];
		YMAX = LE[K];

		N = 0;
		LLB[0] = -1;
		LLE[0] = -1;
		LL[0] = 0;
		LR[0] = 0;
		for (y = YMIN, ib = YMIN * w; y < YMAX; y++, ib += w)
		{
			bln = 0;
			l = -1;
			r = -1;
			for (x = 0; x < w; x++)
			{
				if (Im[ib + x] == 255)
				{
					if (LLB[N] == -1)
					{
						LLB[N] = y;
						LLE[N] = y;
					}
					else
					{
						LLE[N] = y;
					}

					if (l == -1) l = x;

					if (x > r) r = x;

					bln = 1;
				}
			}
			LL[y] = l;
			LR[y] = r;
			LW[y] = r - l + 1;

			if ((bln == 0) && (LLB[N] != -1))
			{
				N++;
				LLB[N] = -1;
				LLE[N] = -1;
			}
		}
		if (LLE[N] == YMAX - 1) N++;

		bln = 1;
		nmin = 0;
		while (bln == 1)
		{
			bln = 0;

			k = nmin;
			for (; k < N; k++)
			{
				NN[k] = 1;
				y = val = yb = LLB[k];
				ym = LLE[k];

				wmin = wmax = LW[yb];

				for (y = yb; y <= ym; y++)
				{
					if (LW[y] > wmax)
					{
						wmax = LW[y];
						val = y;
					}
				}

				y = val;
				while (y <= ym)
				{
					if ((double)LW[y] / (double)wmax > 0.5)
					{
						y++;
					}
					else
					{
						break;
					}
				}
				val2 = y - 1;

				y = val;
				while (y >= yb)
				{
					if ((double)LW[y] / (double)wmax > 0.5)
					{
						y--;
					}
					else
					{
						break;
					}
				}
				val1 = y + 1;

				if ((val1 != yb) || (val2 != ym))
				{
					bln = 1;

					if (val1 == yb)
					{
						LLLB[k][1] = val2 + 1;
						LLLE[k][1] = ym;

						LLLB[k][0] = val1;
						LLLE[k][0] = val2;

						NN[k]++;
					}
					else if (val2 == ym)
					{
						LLLB[k][1] = yb;
						LLLE[k][1] = val1 - 1;

						LLLB[k][0] = val1;
						LLLE[k][0] = val2;

						NN[k]++;
					}
					else
					{
						LLLB[k][1] = yb;
						LLLE[k][1] = val1 - 1;
						LLLB[k][2] = val2 + 1;
						LLLE[k][2] = ym;

						LLLB[k][0] = val1;
						LLLE[k][0] = val2;

						NN[k] += 2;
					}
				}
				else
				{
					LLLB[k][0] = yb;
					LLLE[k][0] = ym;
				}
			}

			if (bln == 1)
			{
				for (k = nmin; k < N; k++)
				{
					LLB[k] = LLLB[k][0];
					LLE[k] = LLLE[k][0];
				}

				i = N;
				for (k = nmin; k < N; k++)
				{
					for (l = 1; l < NN[k]; l++)
					{

						LLB[i] = LLLB[k][l];
						LLE[i] = LLLE[k][l];
						i++;
					}
				}
				nmin = N;
				N = i;
			}
		}

		for (i = 0; i < N - 1; i++)
			for (j = i + 1; j < N; j++)
			{
				k = i;
				val = LLB[k];

				if (LLB[j] < val)
				{
					k = j;
					val = LLB[k];
				}

				if (k != i)
				{
					val1 = LLB[i];
					val2 = LLE[i];
					LLB[i] = LLB[k];
					LLE[i] = LLE[k];
					LLB[k] = val1;
					LLE[k] = val2;
				}
			}

		for (k = 0; k < N; k++)
		{
			yb = LLB[k];
			ym = LLE[k];
			LL[k] = LL[yb];
			LR[k] = LR[yb];

			for (y = yb + 1; y <= ym; y++)
			{
				LL[k] += LL[y];
				LR[k] += LR[y];
			}
		}

		for (k = 0; k < N; k++)
		{
			LL[k] = LL[k] / (LLE[k] - LLB[k] + 1);
			LR[k] = LR[k] / (LLE[k] - LLB[k] + 1);

			if (((LLE[k] - LLB[k] + 1) < segh) ||
				(LL[k] >= w_2)
				)
			{
				memset(&Im[LLB[k] * w], 0, (LLE[k] - LLB[k] + 1)*w * sizeof(int));
				continue;
			}

			res = 1;
		}
	}

	return res;
}
*/

void GetMainClusterImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImRES, custom_buffer<int> &ImMaskWithBorder, int w, int h, int LH, int LMAXY, int iter, int white)
{
	custom_buffer<int> ImRES4(w*h, 0);
	int x, y, i, j, ddy1, ddy2;
	int color;
	u8 *pClr;
	std::string now;
	if (g_show_results) now = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

	pClr = (u8*)(&color);

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_01_2_ImFF" + g_im_save_format, w, h);

	memset(&ImRES[0], 0, (w*h) * sizeof(int));	

	ddy1 = std::max<int>(4, LMAXY - ((7 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + ((1 * LH) / 4));

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(0, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h - 1, LMAXY + 3);
	}		

	cv::Mat samples(w * h, 3, CV_32F);

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			pClr = (u8*)(&ImRGB[y*w + x]);

			for (int z = 0; z < 3; z++)
			{
				samples.at<float>(y + x * h, z) = pClr[z];
			}
		}
	}

	int clusterCount = 4;
	custom_buffer<int> cluster_cnt(clusterCount, 0);
	cv::Mat labels;
	cv::Mat centers;
	cv::kmeans(samples, clusterCount, labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 10, 1.0), 10, cv::KMEANS_PP_CENTERS, centers);

	if (g_show_results)
	{
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				ImRES[y*w + x] = 0;
				pClr = (u8*)(&ImRES[y*w + x]);
				int cluster_idx = labels.at<int>(y + x * h, 0);
				pClr[0] = centers.at<float>(cluster_idx, 0);
				pClr[1] = centers.at<float>(cluster_idx, 1);
				pClr[2] = centers.at<float>(cluster_idx, 2);
			}
		}
		SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_02_ImRES4Clusters" + g_im_save_format, w, h);
	}

	// searching main TXT cluster which intersect with ImFF (ImFF) for save it in ImRES
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			if (ImFF[y * w + x] != 0)
			{
				int cluster_idx = labels.at<int>(y + x * h, 0);
				cluster_cnt[cluster_idx]++;
			}
		}
	}

	j = 0;
	int val1 = cluster_cnt[j];
	for (i = 0; i < clusterCount; i++)
	{
		if (cluster_cnt[i] > val1)
		{
			j = i;
			val1 = cluster_cnt[j];
		}
	}

	// ImRES - MainTXTCluster
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int cluster_idx = labels.at<int>(y + x * h, 0);

			if (cluster_idx == j)
			{
				ImRES[y*w + x] = white;
			}
			else
			{
				ImRES[y*w + x] = 0;
			}
		}
	}

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_05_ImRES_MainTXTClusterIn4Clasters" + g_im_save_format, w, h);

	clusterCount = 6;
	labels.release();
	centers.release();
	cv::kmeans(samples, clusterCount, labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 10, 1.0), 10, cv::KMEANS_PP_CENTERS, centers);

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			int cluster_idx = labels.at<int>(y + x * h, 0);
			ImRES4[y*w + x] = (255 / clusterCount)*(cluster_idx + 1);
		}
	}

	if (g_show_results) SaveGreyscaleImage(ImRES4, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_06_ImRES4_6Clusters" + g_im_save_format, w, h);


	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_07_1_ImRES3_MainTXTClusterIn4Clasters" + g_im_save_format, w, h);
	for (int cluster_idx = 0; cluster_idx < clusterCount; cluster_idx++)
	{
		custom_buffer<CMyClosedFigure> pFigures;
		SearchClosedFigures(ImRES4, w, h, (255 / clusterCount)*(cluster_idx + 1), pFigures);
		int N = pFigures.size(), l, ii;
		CMyClosedFigure *pFigure;
		CMyPoint *PA;

		for (i = 0; i < N; i++)
		{
			pFigure = &(pFigures[i]);

			if ((pFigure->m_minY < ddy1) ||
				(pFigure->m_maxY > ddy2)
				/*if ((pFigure->m_minY == 0) ||
					(pFigure->m_maxY == h - 1)*/
				)
			{
				PA = pFigure->m_PointsArray;

				for (l = 0; l < pFigure->m_Square; l++)
				{
					ii = PA[l].m_i;
					ImRES[ii] = 0;
					ImMaskWithBorder[ii] = 0;
				}
			}
		}
	}
	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_07_2_ImRES3_MainTXTClusterIn4ClastersFilteredBy6ClutersFromTopAndBottom" + g_im_save_format, w, h);

	cluster_cnt = custom_buffer<int>(clusterCount, 0);

	// searching main TXT cluster which intersect with ImFF (ImFF) for save it in ImRES
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			if ((ImFF[y * w + x] != 0) && (ImRES[y * w + x] != 0))
			{
				int cluster_idx = labels.at<int>(y + x * h, 0);
				cluster_cnt[cluster_idx]++;
			}
		}
	}

	j = 0;
	val1 = cluster_cnt[j];
	for (i = 0; i < clusterCount; i++)
	{
		if (cluster_cnt[i] > val1)
		{
			j = i;
			val1 = cluster_cnt[j];
		}
	}

	if (g_show_results)
	{
		// ImRES4 - MainTXTCluster with clusterCount == 6
		for (int y = 0; y < h; y++)
		{
			for (int x = 0; x < w; x++)
			{
				int cluster_idx = labels.at<int>(y + x * h, 0);

				if (cluster_idx == j)
				{
					ImRES4[y*w + x] = white;
				}
				else
				{
					ImRES4[y*w + x] = 0;
				}
			}
		}

		SaveRGBImage(ImRES4, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_08_1_ImRES4_MainTXTClusterIn6Clasters" + g_im_save_format, w, h);
	}

	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(ImRES, w, h, white, pFigures);
	int N = pFigures.size(), l, ii;
	CMyClosedFigure *pFigure;
	CMyPoint *PA;

	for (i = 0; i < N; i++)
	{
		pFigure = &(pFigures[i]);
		PA = pFigure->m_PointsArray;

		int cnt = 0;
		int min_x = w, max_x = 0, min_y = h, max_y = 0, cw = 0, ch = 0;
		for (l = 0; l < pFigure->m_Square; l++)
		{
			ii = PA[l].m_i;
			y = ii / w;
			x = ii % w;
			int cluster_idx = labels.at<int>(y + x * h, 0);

			if (cluster_idx == j)
			{
				cnt++;
				if (x < min_x)
				{
					min_x = x;
				}
				if (x > max_x)
				{
					max_x = x;
				}
				if (y < min_y)
				{
					min_y = y;
				}
				if (y > max_y)
				{
					max_y = y;
				}
			}
		}

		if (cnt > 0)
		{
			ch = max_y - min_y + 1;
			cw = max_x - min_x + 1;
		}

		if ((cnt == 0) ||
			((pFigure->m_h < pFigure->m_w / 4) && (cw < pFigure->m_w / 2)) ||
			((pFigure->m_w < pFigure->m_h / 4) && (ch < pFigure->m_h / 2)) ||
			(((pFigure->m_h >= 16) || (pFigure->m_w >= 16)) &&
			(pFigure->m_h >= pFigure->m_w / 4) &&
				(pFigure->m_w >= pFigure->m_h / 4) &&
				((double)(ch*cw) / (pFigure->m_h*pFigure->m_w) < 0.5))
			)
		{
			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImRES[ii] = 0;
			}
		}
	}

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_08_2_ImRES3_MainTXTClusterIn4ClastersFilteredByMainTXTClusterIn6Clasters" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\GetMainClusterImage_iter" + std::to_string(iter) + "_" + now + "_08_3_ImMaskWithBorder" + g_im_save_format, w, h);
}

void GetMainClusterImageBySplit(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImRES, custom_buffer<int> &ImMaskWithBorder, int w, int h, int LH, int LMAXY, int iter, int white)
{
	custom_buffer<int> ImRES1(w*h, 0), ImRES2(w*h, 0), ImRES3(w*h, 0), ImRES4(w*h, 0), ImRES5(w*h, 0);
	custom_buffer<int> TL(w, 0), TR(w, 0);
	int TN, bln1, bln2, x, y, i, j, ddy1, ddy2;
	int color, rc, gc;
	u8 *pClr;

	pClr = (u8*)(&color);

	color = 0;
	pClr[2] = 255;
	rc = color;

	color = 0;
	pClr[1] = 255;
	gc = color;

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_01_2_ImFF" + g_im_save_format, w, h);

	memset(&ImRES[0], 0, (w*h) * sizeof(int));

	ddy1 = std::max<int>(4, LMAXY - ((7 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + ((1 * LH) / 4));

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(0, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h - 1, LMAXY + 3);
	}

	TN = 0;
	bln1 = 0;
	for (x = 0; x < w; x++)
	{
		bln2 = 0;
		for (y = 0; y < h; y++)
		{
			if (ImFF[y * w + x] != 0)
			{
				bln2 = 1;
				break;
			}
		}

		if (bln2 == 1)
		{
			if (bln1 == 0)
			{
				TL[TN] = x;
				TR[TN] = x;
				bln1 = 1;
			}
			else
			{
				TR[TN] = x;
			}
		}
		else
		{
			if (bln1 == 1)
			{
				TN++;
				bln1 = 0;
			}
		}
	}

	if (bln1 == 1)
	{
		TN++;
	}

	if (TN == 0)
	{
		return;
	}

	i = 0;
	while (i < TN - 1)
	{
		int tw = TR[i] - TL[i] + 1;
		int dt = TL[i + 1] - TR[i] - 1;

		if ((tw < LH / 2) || (dt < LH / 12))
		{
			TR[i] = TR[i + 1];

			for (j = i + 1; j < TN - 1; j++)
			{
				TL[j] = TL[j + 1];
				TR[j] = TR[j + 1];
			}
			TN--;

			continue;
		}

		i++;
	}

	for (i = 0; i < TN - 1; i++)
	{
		int dt = TL[i + 1] - TR[i] - 1;

		TR[i] += (dt / 2);
		TL[i + 1] -= dt - (dt / 2);
	}

	TL[0] = 0;
	TR[TN - 1] = w - 1;

	if (g_show_results)
	{
		memcpy(&ImRES1[0], &ImRGB[0], (w*h) * sizeof(int));

		for (i = 0; i < TN; i++)
		{
			for (y = 0; y < h; y++)
			{
				ImRES1[y * w + TL[i]] = rc;
				ImRES1[y * w + TR[i]] = gc;
			}
		}

		SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_02_ImRGBWithSplitInfo" + g_im_save_format, w, h);
	}

	for (int ti = 0; ti < TN; ti++)
	{
		int tw = TR[ti] - TL[ti] + 1;

		// ImRES1 - RGB image with size tw:h (segment of original image)
		// ImRES2 - ImFF image with size tw:h (segment of original image)
		for (y = 0, i = TL[ti], j = 0; y < h; y++, i += w, j += tw)
		{
			memcpy(&ImRES1[j], &ImRGB[i], tw * sizeof(int));
			memcpy(&ImRES2[j], &ImFF[i], tw * sizeof(int));
			memcpy(&ImRES5[j], &ImMaskWithBorder[i], tw * sizeof(int));
		}

		GetMainClusterImage(ImRES1, ImRES2, ImRES3, ImRES5, tw, h, LH, LMAXY, iter, white);

		for (y = 0, i = TL[ti], j = 0; y < h; y++, i += w, j += tw)
		{
			memcpy(&ImRES[i], &ImRES3[j], tw * sizeof(int));
			memcpy(&ImMaskWithBorder[i], &ImRES5[j], tw * sizeof(int));
		}
	}

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_03_1_ImRES_MainCluster" + g_im_save_format, w, h);

	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(ImRES, w, h, white, pFigures);
	int N = pFigures.size(), l, ii;
	CMyClosedFigure *pFigure;
	CMyPoint *PA;

	for (i = 0; i < N; i++)
	{
		pFigure = &(pFigures[i]);
		PA = pFigure->m_PointsArray;
		int found = 0;

		//removing all figures which are located only on one side of edge of image split
		for (int ti = 0; ti < TN; ti++)
		{
			if ( ((pFigure->m_maxX == TR[ti]) && (ti < TN - 1)) ||
				 ((pFigure->m_minX == TL[ti]) && (ti > 0))
				)
			{
				found = 1;
				break;
			}
		}

		if (found == 1)
		{
			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImRES[ii] = 0;
			}
		}
	}

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_03_2_ImRES_MainClusterFilteredByFiguresOnSplitEdges" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\GetMainClusterImageBySplit_iter" + std::to_string(iter) + "_03_3_ImMaskWithBorder" + g_im_save_format, w, h);
}

int FindTextLines(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, vector<string> &SavedFiles, int W, int H)
{
	custom_buffer<int> LL(H, 0), LR(H, 0), LLB(H, 0), LLE(H, 0), LW(H, 0);
	custom_buffer<int> ImRES1(W*H * 16, 0), ImRES2(W*H * 16, 0), ImRES3(W*H * 16, 0), ImRES4(W*H * 16, 0);		
	custom_buffer<int> ImRES5(W*H * 16, 0), ImRES6(W*H * 16, 0), ImRES7(W*H * 16, 0), ImRES8(W*H * 16, 0), ImRES(W*H * 16, 0);
	custom_buffer<int> Im(W*H * 16, 0), ImSF(W*H * 16, 0), ImSNF(W*H * 16, 0), ImFF(W*H * 16, 0), ImSFIntTHRF(W*H * 16, 0), ImNSFIntTHRF(W*H * 16, 0);
	custom_buffer<int> FullImY(W*H, 0), ImY(W*H * 16, 0), ImU(W*H * 16, 0), ImV(W*H * 16, 0), ImI(W*H * 16, 0);
	custom_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);

	int i, j, k, l, r, x, y, ib, bln, N, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, ww, hh, cnt;
	int XB, XE, YB, YE, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, lb, le, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, NN, ys1, ys2, ys3, val_min, val_max;	
	int j1, j2, j3, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int mY, mI, mQ;
	int LH, LMAXY;
	double mthr;
	string SaveName, FullName, Str;
	char str[30];
	int res;
	int iter = 0;

	res = 0;

	SaveName = SavedFiles[0];
	SavedFiles.clear();

	mthr = 0.3;
	segh = g_segh;
	int color, rc, gc, bc, cc, wc;
	u8 *pClr;

	pClr = (u8*)(&color);

	color = 0;
	pClr[2] = 255;
	rc = color;

	color = 0;
	pClr[1] = 255;
	gc = color;

	color = 0;
	pClr[0] = 255;
	bc = color;

	color = 0;
	pClr[2] = 128;
	pClr[0] = 128;
	cc = color;

	color = 0;
	pClr[0] = 255;
	pClr[1] = 255;
	pClr[2] = 255;
	wc = color;

	for (i = 0; i < W*H*16; i++) ImRES[i] = wc;

	g_pViewImage[0](ImRGB, W, H);

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\FindTextLines_01_1_ImRGB" + g_im_save_format, W, H);
	if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FindTextLines_01_2_ImF" + g_im_save_format, W, H);
	if (g_show_results) SaveGreyscaleImage(ImNF, "\\TestImages\\FindTextLines_01_3_ImNF" + g_im_save_format, W, H);

	cv::Mat cv_FullImRGB(H, W, CV_8UC4), cv_FullImY;
	memcpy(cv_FullImRGB.data, &ImRGB[0], W*H * sizeof(int));
	cv::cvtColor(cv_FullImRGB, cv_FullImY, cv::COLOR_BGR2GRAY);
	GreyscaleMatToImage(cv_FullImY, W, H, FullImY);
	if (g_show_results) SaveGreyscaleImage(FullImY, "\\TestImages\\FindTextLines_01_4_FullImY" + g_im_save_format, W, H);

	// Getting info about text lines position in ImF
	// -----------------------------------------------------
	// LW[y] contain number of text pixels in 'y' of ImF image
	N = 0; // number of lines
	LLB[0] = -1; // line 'i' begin by 'y'
	LLE[0] = -1; // line 'i' end by 'y'
	LL[0] = W-1; // line 'i' begin by 'x'
	LR[0] = 0; // line 'i' end by 'x'

	for (y=0, ib=0; y<H; y++, ib+=W)
	{
		bln = 0;
		cnt = 0;
		for(x=0; x<W; x++)
		{
			if (ImF[ib+x] == 255) 
			{
				if (LLB[N] == -1)
				{
					LLB[N] = y;
					LLE[N] = y;
				}
				else
				{
					LLE[N] = y;
				}

				if (bln == 0) 
				{
					l = x;
					bln = 1;
				}

				r = x;
				cnt++;
			}			
		}		
		
		if ((bln == 0) && (LLB[N] != -1))
		{
			N++;
			LLB[N] = -1;
			LLE[N] = -1;
			LL[N] = W-1;
			LR[N] = 0;
		}

		if (bln == 1)
		{
			if (LL[N]>l) LL[N] = l;
			if (LR[N]<r) LR[N] = r;
		}
		
		LW[y] = cnt;
	}
	if (LLE[N] == H-1) N++;
	// -----------------------------------------------------

	k = 0;
	while (k < N-1)
	{
		if (LLB[k + 1] - LLE[k] <= g_segh)
		{
			LLE[k] = LLE[k + 1];
			LL[k] = std::min<int>(LL[k], LL[k + 1]);
			LR[k] = std::max<int>(LR[k], LR[k + 1]);

			for (i = k+1; i < N-1; i++)
			{
				LL[i] = LL[i + 1];
				LR[i] = LR[i + 1];
				LLB[i] = LLB[i + 1];
				LLE[i] = LLE[i + 1];
			}
			N--;
			continue;
		}
		k++;		
	}

	k = 0;
	while (k<N)
	{
		iter++;
		int orig_LLBk = LLB[k];
		int orig_LLEk = LLE[k];
		int orig_LLk = LL[k];
		int orig_LRk = LR[k];
		int min_h;

		XB = LL[k];
		XE = LR[k];
		w = XE-XB+1;
		val = (int)((double)w*0.15);
		if (val<40) val = 40;
		XB -= val;
		XE += val;
		if (XB < 0) XB = 0;
		if (XE > W-1) XE = W-1;
		w = XE-XB+1;

		YB = LLB[k];
		YE = LLE[k];
		
		// getting h of sub
		h = YE-YB+1;
		val = (3*((int)(0.03*(double)H)+1))/2; // 3/2 * ~ min sub height # (536-520+1)/576
		if (h < val)
		{
			if (N == 1)
			{
				LLB[k] -= (val - h) / 2;
				LLE[k] = LLB[k] + val - 1;
			}
			else if (k == 0)
			{
				LLE[k] += std::min<int>((val - h) / 2, (LLB[k+1] - LLE[k])/2);
				LLB[k] = LLE[k] - val + 1;
			}
			else if (k == N-1)
			{
				LLB[k] -= std::min<int>((val - h) / 2, (LLB[k] - LLE[k-1]) / 2);
				LLE[k] = LLB[k] + val - 1;
			}
			else
			{
				LLB[k] -= std::min<int>((val - h) / 2, (LLB[k] - LLE[k - 1]) / 2);
				LLE[k] += std::min<int>((val - h) / 2, (LLB[k + 1] - LLE[k]) / 2);				
			}

			if (LLB[k] < 0) LLB[k] = 0;
			if (LLE[k] > H - 1) LLE[k] = H - 1;

			YB = LLB[k];
			YE = LLE[k];
			h = YE-YB+1;
		}
		
		YB -= h/2;
		YE += h/2;
		if (YB < 0) YB = 0;
		if (YE > H-1) YE = H-1;

		if (k>0)
		{
			if (YB < LLE[k-1]-2) YB = std::max<int>(0, LLE[k-1]-2);
		}
		if (k<N-1)
		{
			val = (LLB[k+1]*5+LLE[k+1])/6;
			if (YE > val) YE = val;
		}		

		int max_diff = 20;
		for (y = YB; y < YE; y++)
		{
			bln = 0;
			val1 = FullImY[y*W];
			val2 = val1;
			for (x = 0; x < W; x++)
			{
				if (FullImY[y*W + x] < val1)
				{
					val1 = FullImY[y*W + x];
				}

				if (FullImY[y*W + x] > val2)
				{
					val2 = FullImY[y*W + x];
				}

				if (val2 - val1 > max_diff)
				{
					bln = 1;
					break;
				}
			}

			if (bln == 1)
			{
				break;
			}
		}
		YB = y;

		for (y = YE; y > YB; y--)
		{
			bln = 0;
			val1 = FullImY[y*W];
			val2 = val1;
			for (x = 0; x < W; x++)
			{
				if (FullImY[y*W + x] < val1)
				{
					val1 = FullImY[y*W + x];
				}

				if (FullImY[y*W + x] > val2)
				{
					val2 = FullImY[y*W + x];
				}

				if (val2 - val1 > max_diff)
				{
					bln = 1;
					break;
				}
			}

			if (bln == 1)
			{
				break;
			}
		}
		YE = y;

		if (YB > orig_LLBk - 4)
		{			
			YB = std::max<int>(0, LLB[k] - 4);
		}

		if (YE < orig_LLEk + 4)
		{
			YE = std::min<int>(H-1, orig_LLEk + 4);
		}

		if (YB == YE)
		{
			k++;
			continue;
		}

		if (LLB[k] < YB) LLB[k] = YB;
		if (LLE[k] > YE) LLE[k] = YE;

		if (g_show_results)
		{
			memcpy(&ImRES1[0], &ImRGB[0], W * H * sizeof(int));

			for (i = 0; i < N; i++)
			{
				for (x = 0; x < W; x++)
				{
					ImRES1[LLB[i] * W + x] = rc;
					ImRES1[LLE[i] * W + x] = gc;
				}
			}

			for (x = 0; x < W; x++)
			{
				ImRES1[YB * W + x] = bc;
				ImRES1[YE * W + x] = bc;
			}

			SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_03_1_ImRGB_RGBWithLinesInfo" + g_im_save_format, W, H);
		}
		
		for (y=YB, i=YB*W+XB, j=0; y<=YE; y++, i+=W, j+=w)
		{
			memcpy(&ImRES1[j], &ImRGB[i], w * sizeof(int));
		}
		h = YE - YB + 1;

		if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_03_2_ImRES1_RGB" + g_im_save_format, w, h);

		cv::Mat cv_ImRGBOrig(h, w, CV_8UC4), cv_ImRGB, cv_ImY, cv_ImLAB, cv_ImLABSplit[3], cv_ImHSV, cv_ImHSVSplit[3], cv_ImL, cv_ImA, cv_ImB, cv_bw, cv_bw_gaus;
		memcpy(cv_ImRGBOrig.data, &ImRES1[0], w*h*sizeof(int));
		cv::resize(cv_ImRGBOrig, cv_ImRGB, cv::Size(0, 0), 4, 4);

		memcpy(&Im[0], cv_ImRGB.data, w*h*4*4*sizeof(int));
		if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_03_3_Im_RGBScaled4x4" + g_im_save_format, w*4, h*4);

		for(y=YB, i=YB*W+XB, j=0; y<=YE; y++, i+=W, j+=w)
		{
			memcpy(&ImRES2[j], &ImF[i], w*sizeof(int));
		}
		SimpleResizeImage4x(ImRES2, ImSF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_04_1_SF" + g_im_save_format, w*4, h*4);

		for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
		{
			memcpy(&ImRES2[j], &ImNF[i], w * sizeof(int));
		}
		SimpleResizeImage4x(ImRES2, ImSNF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSNF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_04_2_SNF" + g_im_save_format, w * 4, h * 4);
		
		w *= 4;
		h *= 4;

		cv::cvtColor(cv_ImRGB, cv_ImY, cv::COLOR_BGR2GRAY);
		cv::cvtColor(cv_ImRGB, cv_ImLAB, cv::COLOR_BGR2Lab);		
		cv::cvtColor(cv_ImRGB, cv_ImHSV, cv::COLOR_BGR2HSV);
		cv::split(cv_ImLAB, cv_ImLABSplit);
		cv::split(cv_ImHSV, cv_ImHSVSplit);

		GreyscaleMatToImage(cv_ImY, w, h, ImY);
		GreyscaleMatToImage(cv_ImLABSplit[1], w, h, ImU);
		GreyscaleMatToImage(cv_ImLABSplit[2], w, h, ImV);
		GreyscaleMatToImage(cv_ImHSVSplit[1], w, h, ImI);

		if (g_show_results) SaveGreyscaleImage(ImU, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_06_1_ImU" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImV, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_06_2_ImV" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImI, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_06_3_ImI" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImY, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_06_5_ImY" + g_im_save_format, w, h);

		cv_bw = cv_ImY;
		cv::medianBlur(cv_bw, cv_bw, 5);
		//cv::imshow("medianBlur", cv_bw);
		cv::adaptiveThreshold(cv_bw, cv_bw_gaus, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
		//cv::imshow("Binary Image With Thr Gaus", cv_bw_gaus);

			// Create a kernel that we will use to sharpen our image
		cv::Mat kernel_sharp = (cv::Mat_<float>(3, 3) <<
			1, 1, 1,
			1, -8, 1,
			1, 1, 1); // an approximation of second derivative, a quite strong kernel
		// do the laplacian filtering as it is
		// well, we need to convert everything in something more deeper then CV_8U
		// because the kernel has some negative values,
		// and we can expect in general to have a Laplacian image with negative values
		// BUT a 8bits unsigned int (the one we are working with) can contain values from 0 to 255
		// so the possible negative number will be truncated
		cv::Mat imgLaplacian;
		cv::filter2D(cv_ImRGB, imgLaplacian, CV_32F, kernel_sharp);
		cv::Mat sharp;
		cv_ImRGB.convertTo(sharp, CV_32F);
		cv::Mat imgResult = sharp - imgLaplacian;
		// convert back to 8bits gray scale
		imgResult.convertTo(imgResult, CV_8UC3);
		imgLaplacian.convertTo(imgLaplacian, CV_8UC3);
		// imshow( "Laplace Filtered Image", imgLaplacian );
		//cv::imshow("New Sharped Image", imgResult);
		// Create binary image from source image
		cv::cvtColor(imgResult, cv_bw, cv::COLOR_BGR2GRAY);
		cv::threshold(cv_bw, cv_bw, 40, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
		//cv::imshow("Binary Image", cv_bw);

		for (int i = 0; i < w*h; i++)
		{
			if (cv_bw_gaus.data[i] == 0)
			{
				cv_bw.data[i] = 0;
			}
		}
		//cv::imshow("Binary Image With Thr Otsu Int Thr Gaus", cv_bw);

		custom_buffer<int> ImTHR(w*h, 0);
		GreyscaleMatToImage(cv_bw, w, h, ImTHR);
		if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_07_ImTHR" + g_im_save_format, w, h);

		// noise removal
		cv::Mat kernel_open = cv::Mat::ones(3, 3, CV_8U);
		cv::Mat opening;
		//cv::imshow("cv_bw", cv_bw);
		cv::morphologyEx(cv_bw, opening, cv::MORPH_OPEN, kernel_open, cv::Point(-1, -1), 2);
		//cv::imshow("opening", opening);		
		//cv::waitKey(0);		
		GreyscaleMatToImage(opening, w, h, ImSFIntTHRF);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_09_1_ImTHRF" + g_im_save_format, w, h);

		ClearImageFromBorders(ImSFIntTHRF, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_09_2_ImTHRFClearedFromBorders" + g_im_save_format, w, h);		

		IntersectTwoImages(ImSFIntTHRF, ImSNF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_09_3_ImNSFIntTHRF" + g_im_save_format, w, h);		

		val = (int)(0.03*(double)H) + 1; // ~ min sub height # (536-520+1)/576
		min_h = (int)(0.4*(double)min<int>(val, orig_LLEk - orig_LLBk + 1));

		ClearImageFromGarbageBetweenLines(ImSFIntTHRF, w, h, (LLB[k] - YB) * 4, (LLE[k] - YB) * 4, min_h * 4, 255);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_09_4_ImNSFIntTHRFClearedFromGarbageBetweenLines" + g_im_save_format, w, h);

		memcpy(&ImNSFIntTHRF[0], &ImSFIntTHRF[0], (w*h) * sizeof(int));

		// Checking ImSFIntTHRF on presence more then one line (if yes spliting on two and repeat)
		//-------------------------------		
		val = GetSubParams(ImSFIntTHRF, w, h, 255, LH, LMAXY, lb, le, min_h*4, ((W / 2) - XB) * 4);

		yb = (orig_LLBk - YB) * 4;
		ye = (orig_LLEk - YB) * 4;
		xb = (orig_LLk - XB) * 4;
		xe = std::min<int>(((W / 2) - XB) * 4 + (((W / 2) - XB) * 4 - xb), (orig_LRk - XB) * 4); //sometimes frome the right can be additional not centered text lines which intersect with two centered lines

		if ((val == 1) && (std::max<int>((LLE[k] - LLB[k] + 1)*4, le - lb + 1) > 1.7*(double)LH))
		{
			bln = 0;

			custom_buffer<int> lw(h, 0);
			int max_txt_w, max_txt_y, min_y, max_y, new_txt_y = 0, new_txt_w, max_txt2_w, max_txt2_y;

			for (y = yb; y <= ye; y++)
			{
				for (x = xb; x < xe; x++, i++)
				{
					if (ImSFIntTHRF[y*w+x] != 0)
					{
						lw[y]++;
					}
				}
			}			

			// searching max text location by y for line [LMAXY-LH+1, LMAXY] -- max_txt_y
			for (y = LMAXY - LH + 1, max_txt_w = lw[y], max_txt_y = y; y <= LMAXY; y++)
			{
				if (lw[y] > max_txt_w)
				{
					max_txt_w = lw[y];
					max_txt_y = y;
				}
			}

			if (max_txt_w == 0)
			{
				k++;
				continue;
			}

			min_y = yb;
			max_y = LMAXY - LH;

			int y1, y2;

			for (y1 = max_y - 1; y1 >= min_y; y1--)
			{					
				if (lw[y1] >= min_h * 4) // bigger then min sub height
				{
					for (y2 = max_y; y2 >= y1 + 1; y2--)
					{
						if ((double)lw[y2] / max_txt_w <= 0.3)
						{
							if ((double)lw[y2] / lw[y1] <= 0.3)
							{
								new_txt_y = y2;
								new_txt_w = lw[new_txt_y];
								break;
							}
						}
					}
				}
				if (new_txt_y > 0)
				{
					for (y2 = max_y; y2 >= y1 + 1; y2--)
					{
						if (lw[y2] < lw[new_txt_y])
						{
							new_txt_y = y2;
							new_txt_w = lw[new_txt_y];
						}
					}

					max_txt2_y = y1;
					max_txt2_w = lw[max_txt2_y];

					/*for (y1 = min_y, max_txt2_y=y1, max_txt2_w=lw[y1]; y1 < new_txt_y; y1++)
					{
						if (lw[y1] > max_txt2_w)
						{
							max_txt2_y = y1;
							max_txt2_w = lw[max_txt2_y];
						}
					}*/

					break;
				}
			}

			if ( (new_txt_y > 0) && 
				 (((YB + (new_txt_y/4)) - orig_LLBk + 1) >= 4) &&
				 ((orig_LLEk - (YB + (new_txt_y / 4) + 1) + 1) >= 4)
				)
			{
				for (i = N; i > k + 1; i--)
				{
					LL[i] = LL[i - 1];
					LR[i] = LR[i - 1];
					LLB[i] = LLB[i - 1];
					LLE[i] = LLE[i - 1];
				}

				LL[k + 1] = orig_LLk;
				LR[k + 1] = orig_LRk;
				LLB[k + 1] = YB + (new_txt_y / 4) + 1;
				LLE[k + 1] = orig_LLEk;

				LL[k] = orig_LLk;
				LR[k] = orig_LRk;
				LLB[k] = orig_LLBk;
				LLE[k] = YB + (new_txt_y / 4);

				N++;
				continue;
			}
		}
		//----------------------------
		
		while (1)
		{
			IntersectTwoImages(ImSFIntTHRF, ImSF, w, h);
			if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_09_5_ImSFIntTHRF" + g_im_save_format, w, h);		

			delta = 40;

			val1 = (int)((double)((LLE[k] - LLB[k] + 1) * 4)*0.3);
			val2 = (int)((double)((LR[k] - LL[k] + 1) * 4)*0.1);
			yb = (LLB[k] - YB) * 4 + val1;
			ye = (LLE[k] - YB) * 4 - val1;
			xb = (LL[k] - XB) * 4 + val2;
			xe = (LR[k] - XB) * 4 - val2;

			StrAnalyseImage(ImSFIntTHRF, ImY, GRStr, w, h, xb, xe, yb, ye, 0);
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
			FindMaxStr(smax, smaxi, j1, ys1, NN);

			if (NN == 0)
			{
				memset(&ImFF[0], 0, (w*h) * sizeof(int));
				break;
			}

			j1_min = 1000;
			j1_max = -1000;

			j2_min = 1000;
			j2_max = -1000;

			j3_min = 1000;
			j3_max = -1000;
		
			j4_min = 1000;
			j4_max = -1000;
		
			j5_min = 1000;
			j5_max = -1000;

			custom_buffer<int> GRStr2(STR_SIZE, 0), GRStr3(STR_SIZE, 0), GRStr4(STR_SIZE, 0);
			int j2, ys2, j3, ys3, j4, ys4, min_delta;

			memset(&GRStr2[0], 0, 256 * sizeof(int));
			memset(&GRStr3[0], 0, 256 * sizeof(int));

			for (i = 0; i < w*h; i++)
			{
				val1 = ImY[i];

				if ((ImSFIntTHRF[i] != 0) && (ImY[i] >= j1) && (ImY[i] <= j1 + delta - 1))
				{
					GRStr2[ImU[i]]++;
					GRStr3[ImV[i]]++;
					GRStr4[ImI[i]]++;

					if (ImU[i] < j2_min) j2_min = ImU[i];
					if (ImU[i] > j2_max) j2_max = ImU[i];

					if (ImV[i] < j3_min) j3_min = ImV[i];
					if (ImV[i] > j3_max) j3_max = ImV[i];

					if (ImI[i] < j4_min) j4_min = ImI[i];
					if (ImI[i] > j4_max) j4_max = ImI[i];
				}
			}

			if (j2_min == 1000)
			{
				memset(&ImFF[0], 0, (w*h) * sizeof(int));
				break;
			}

			delta = 80;
			StrAnalyseImage(ImSFIntTHRF, ImY, GRStr, w, h, xb, xe, yb, ye, 0);
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
			FindMaxStr(smax, smaxi, j1, ys1, NN);
			j1_min = j1;
			j1_max = j1_min + delta - 1;

			min_delta = 20;
			delta = j2_max - j2_min + 1;
			if (delta < min_delta)
			{
				delta = delta / 2;
				FindMaxStrDistribution(GRStr2, delta, smax, smaxi, NN, 0);
				FindMaxStr(smax, smaxi, j2, ys2, NN);
				j2_min = std::max<int>(0, std::min<int>(j2_min, j2 + (delta / 2) - (min_delta/2)));
				j2_max = std::min<int>(255, std::max<int>(j2_max, j2 + (delta / 2) + (min_delta/2) - 1));
			}

			min_delta = 20;
			delta = j3_max - j3_min + 1;
			if (delta < min_delta)
			{
				delta = delta / 2;
				FindMaxStrDistribution(GRStr3, delta, smax, smaxi, NN, 0);
				FindMaxStr(smax, smaxi, j3, ys3, NN);
				j3_min = std::max<int>(0, std::min<int>(j3_min, j3 + (delta / 2) - (min_delta / 2)));
				j3_max = std::min<int>(255, std::max<int>(j3_max, j3 + (delta / 2) + (min_delta / 2) - 1));
			}

			min_delta = 20;
			delta = j4_max - j4_min + 1;
			if (delta < min_delta)
			{
				delta = delta / 2;
				FindMaxStrDistribution(GRStr4, delta, smax, smaxi, NN, 0);
				FindMaxStr(smax, smaxi, j4, ys4, NN);
				j4_min = std::max<int>(0, std::min<int>(j4_min, j4 + (delta / 2) - (min_delta / 2)));
				j4_max = std::min<int>(255, std::max<int>(j4_max, j4 + (delta / 2) + (min_delta / 2) - 1));
			}
				
			for (i=0; i<w*h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];
				val4 = ImI[i];

				if (( val1 >= j1_min ) && (val1 <= j1_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max)
					) 
				{
					ImRES1[i] = rc;
				}
				else
				{
					ImRES1[i] = 0;
				}
			}

			val_min = j1_min - 20;
			val_max = j1_max;			
			for (i = 0; i < w*h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];
				val4 = ImI[i];

				if ((val1 >= val_min) && (val1 <= val_max) &&
					(val2 >= j2_min) && (val2 <= j2_max) &&
					(val3 >= j3_min) && (val3 <= j3_max) &&
					(val4 >= j4_min) && (val4 <= j4_max)
					)
				{
					ImRES3[i] = rc;
				}
				else
				{
					ImRES3[i] = 0;
				}
			}

			val_min = j1_min+20;
			val_max = j1_max+20;
			for (i = 0; i < w*h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];
				val4 = ImI[i];

				if ((val1 >= val_min) && (val1 <= val_max) &&
					(val2 >= j2_min) && (val2 <= j2_max) &&
					(val3 >= j3_min) && (val3 <= j3_max) &&
					(val4 >= j4_min) && (val4 <= j4_max)
					)
				{
					ImRES4[i] = rc;
				}
				else
				{
					ImRES4[i] = 0;
				}
			}

			val_min = j1_min;
			val_max = j1_max+20;
			for (i=0; i<w*h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];
				val4 = ImI[i];

				if (( val1 >= val_min ) && (val1 <= val_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max)
					) 
				{
					ImRES5[i] = rc;
				}
				else
				{
					ImRES5[i] = 0;
				}
			}

			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_42_1_ImRES1" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES1, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_42_2_ImRES1IntImTHR" + g_im_save_format, w, h);
			
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_44_1_ImRES3" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES3, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_44_2_ImRES3IntImTHR" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_45_1_ImRES4" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES4, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_45_2_ImRES4IntImTHR" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_46_1_ImRES5" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES5, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_46_2_ImRES5IntImTHR" + g_im_save_format, w, h);


			yb = (LLB[k]-YB)*4;
			ye = (LLE[k]-YB)*4;
			xb = (LL[k]-XB)*4;
			xe = (LR[k]-XB)*4;

			memcpy(&ImRES7[0], &ImRES4[0], (w*h)*sizeof(int));
			memcpy(&ImFF[0], &ImRES1[0], (w*h)*sizeof(int));

			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_48_1_ImRES1" + g_im_save_format, w, h);
			IntersectTwoImages(ImNSFIntTHRF, ImRES1, w, h);
			if (g_show_results) SaveGreyscaleImage(ImNSFIntTHRF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_48_2_ImNSFIntTHRFIntImRES1" + g_im_save_format, w, h);

			val = (int)(0.03*(double)H) + 1; // ~ min sub height # (536-520+1)/576
			min_h = (int)(0.4*(double)max<int>(val, orig_LLEk - orig_LLBk + 1));
			val = GetSubParams(ImNSFIntTHRF, w, h, 255, LH, LMAXY, lb, le, min_h * 4, ((W / 2) - XB) * 4);
			if (val == 0)
			{
				memset(&ImFF[0], 0, (w*h) * sizeof(int));
				break;
			}

			N1 = ClearImageOptimal(ImRES1, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_48_3_ImRES1F" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_50_1_ImRES3" + g_im_save_format, w, h);
			N3 = ClearImageOptimal(ImRES3, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_50_2_ImRES3F" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_51_1_ImRES4" + g_im_save_format, w, h);
			N4 = ClearImageOptimal(ImRES4, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_51_2_ImRES4F" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_52_1_ImRES5" + g_im_save_format, w, h);
			N5 = ClearImageOptimal(ImRES5, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_52_2_ImRES5F" + g_im_save_format, w, h);
			
			minN = N5/2;

			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (ImRES1[i] != 0) ) ||
					 ( (N3 >= minN) && (ImRES3[i] != 0) ) ||
					 ( (N4 >= minN) && (ImRES4[i] != 0) ) ||
					 ( (N5 >= minN) && (ImRES5[i] != 0) )
				   )
				{
					ImRES8[i] = rc;
				}
				else
				{
					ImRES8[i] = 0;
				}
			}
			if (g_show_results) SaveRGBImage(ImRES8, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_54_1_ImRES8(ImRES1+3+4+5)" + g_im_save_format, w, h);

			val = ClearImageOpt2(ImRES8, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES8, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_54_2_ImRES8ClearImageOpt2" + g_im_save_format, w, h);

			if (val == 0)
			{
				memset(&ImFF[0], 0, (w*h)*sizeof(int));
				break;
			}

			CombineTwoImages(ImFF, ImRES8, w, h, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_57_1_ImFF(ImRES1)WithImRES8" + g_im_save_format, w, h);			
			
			ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_57_2_ImFF_F" + g_im_save_format, w, h);

			memcpy(&ImRES8[0], &ImFF[0], (w*h)*sizeof(int));

			if (g_show_results) SaveRGBImage(ImRES7, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_58_1_ImRES7(ImRES4)" + g_im_save_format, w, h);
			N7 = ClearImageOpt5(ImRES7, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES7, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_58_2_ImRES7(ImRES4)_F" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_60_1_ImFF" + g_im_save_format, w, h);
			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (ImRES1[i] != 0) ) ||
					 ( (N5 >= minN) && (ImRES5[i] != 0) ) ||
					 ( (N7 >= minN) && (ImRES7[i] != 0) )
					)
				{
					ImFF[i] = rc;
				}
			}
			
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_60_2_ImFFWithImRES1+5+7" + g_im_save_format, w, h);
			ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_60_3_ImFF_F" + g_im_save_format, w, h);				

			if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_68_1_ImTHR" + g_im_save_format, w, h);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_68_2_ImFF" + g_im_save_format, w, h);						
			IntersectTwoImages(ImFF, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_68_3_ImFFIntImTHR" + g_im_save_format, w, h);

			ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_69_4_ImFFIntImTHR_F" + g_im_save_format, w, h);

			if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_69_5_ImRGB" + g_im_save_format, w, h);			

			int min_x = w-1, max_x = 0, min_y = h-1, max_y = 0, ww, hh;
			for (y = 0, ib = 0; y < h; y++, ib += w)
			{
				for (x = 0; x < w; x++)
				{
					if (ImFF[ib + x] != 0)
					{
						if (x < min_x) min_x = x;
						if (x > max_x) max_x = x;
						if (y < min_y) min_y = y;
						if (y > max_y) max_y = y;
					}
				}
			}

			if (max_x == 0)
			{
				memset(&ImFF[0], 0, (w*h) * sizeof(int));
				break;
			}

			min_x -= 16;
			if (min_x < 0) min_x = 0;
			max_x += 16;
			if (max_x > w-1) max_x = w-1;
			
			min_y -= 16;
			if (min_y < 0) min_y = 0;
			max_y += 16;
			if (max_y > h - 1) max_y = h - 1;

			ww = max_x - min_x + 1;
			hh = max_y - min_y + 1;

			// ImRES1 - RGB image with size ww:hh (segment of original image)
			// ImRES2 - ImFF image with size ww:hh (segment of original image)
			for (y = min_y, i = min_y * w + min_x, j = 0; y <= max_y; y++, i += w, j += ww)
			{
				memcpy(&ImRES1[j], &Im[i], ww * sizeof(int));
				memcpy(&ImRES2[j], &ImFF[i], ww * sizeof(int));
				memcpy(&ImRES6[j], &ImSFIntTHRF[i], ww * sizeof(int));
			}			

			IntersectTwoImages(ImRES6, ImRES2, ww, hh);

			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_1_ImRES1_ImRGB" + g_im_save_format, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_2_ImRES2_ImFF" + g_im_save_format, ww, hh);
			if (g_show_results) SaveGreyscaleImage(ImRES6, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_3_ImRES6_ImFFIntImSFIntTHRF" + g_im_save_format, ww, hh);

			u8 *color;
			cv::Mat cv_ImGR(hh, ww, CV_8UC1), cv_ImGRRes(hh, ww, CV_8UC1);
			custom_buffer<int> ImMaskWithBorder(ww*hh, 255);

			BinaryImageToMat(ImRES6, ww, hh, cv_ImGR);
			cv::erode(cv_ImGR, cv_ImGRRes, cv::Mat());
			BinaryMatToImage(cv_ImGRRes, ww, hh, ImRES6, rc);			
			if (g_show_results) SaveRGBImage(ImRES6, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_4_ImRES6_ImFFIntImSFIntTHRFWithErode" + g_im_save_format, ww, hh);

			GetMainClusterImageBySplit(ImRES1, ImRES6, ImRES4, ImMaskWithBorder, ww, hh, LH, LMAXY - min_y, iter, rc);
			GetMainClusterImage(ImRES1, ImRES6, ImRES3, ImMaskWithBorder, ww, hh, LH, LMAXY - min_y, iter, rc);
			
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_5_ImRES4_MainTXTCluster" + g_im_save_format, ww, hh);						
			ClearImageOpt5(ImRES4, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_71_6_ImRES4_MainTXTClusterF" + g_im_save_format, ww, hh);			

			//// Morphological Transformations
			//// https://docs.opencv.org/3.0-beta/doc/py_tutorials/py_imgproc/py_morphological_ops/py_morphological_ops.html
			//// https://docs.opencv.org/4.1.0/db/df6/tutorial_erosion_dilatation.html
			//BinaryImageToMat(ImRES3, ww, hh, cv_ImGR);
			//cv::morphologyEx(cv_ImGR, cv_ImGRRes, cv::MORPH_OPEN, kernel_open, cv::Point(-1, -1), 2);
			//BinaryMatToImage(cv_ImGRRes, ww, hh, ImRES5, rc);			
			
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_73_1_ImRES4_MainTXTClusterF" + g_im_save_format, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_73_2_ImRES3_MainTXTCluster2" + g_im_save_format, ww, hh);
			ClearImageOpt5(ImRES3, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_73_3_ImRES3_MainTXTCluster2F" + g_im_save_format, ww, hh);			

			/*if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_73_4_ImRES5_MainTXTCluster2WithOpen" + g_im_save_format, ww, hh);
			ClearImageOpt5(ImRES5, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_73_5_ImRES5_MainTXTCluster2WithOpenF" + g_im_save_format, ww, hh);*/

			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_1_ImRES4_MainTXTClusterF" + g_im_save_format, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_2_ImRES3_MainTXTCluster2F" + g_im_save_format, ww, hh);
			//if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_3_ImRES5_MainTXTCluster2WithOpenF" + g_im_save_format, ww, hh);
			MergeWithClusterImage(ImRES3, ImRES4, ww, hh, rc);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_4_ImRES3_MainTXTClusterFMerge1+2" + g_im_save_format, ww, hh);
			if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_5_ImMaskWithBorder" + g_im_save_format, ww, hh);
			IntersectTwoImages(ImRES3, ImMaskWithBorder, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_74_6_ImRES3_MainTXTClusterFilteredByImMaskWithBorder" + g_im_save_format, ww, hh);

			if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_75_1_ImMaskWithBorder" + g_im_save_format, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_75_2_ImRES2_ImFF" + g_im_save_format, ww, hh);			
			IntersectTwoImages(ImRES2, ImMaskWithBorder, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_75_3_ImRES2_ImFFFilteredByImMaskWithBorder" + g_im_save_format, ww, hh);

			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_76_1_ImRES3_MainTXTCluster" + g_im_save_format, ww, hh);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_76_2_ImRES2_ImFF" + g_im_save_format, ww, hh);
			MergeWithClusterImage(ImRES2, ImRES3, ww, hh, rc);
			//CombineTwoImages(ImRES2, ImRES3, ww, hh, rc);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_76_3_ImRES2_ImFFMergeWithMainTXTCluster" + g_im_save_format, ww, hh);
			ClearImageOpt5(ImRES2, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_76_4_ImRES2_ImFF_F" + g_im_save_format, ww, hh);

			for (y = min_y, i = min_y * w + min_x, j = 0; y <= max_y; y++, i += w, j += ww)
			{
				memcpy(&ImFF[i], &ImRES2[j], ww * sizeof(int));
			}			

			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_77_1_ImFF" + g_im_save_format, w, h);

			if (g_clear_image_logical == true)
			{
				val = ClearImageLogical(ImFF, w, h, LH, LMAXY, xb, xe, rc, W * 4, H * 4, ((W / 2) - XB) * 4);
				if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_77_2_ImFFWithClearImageLogical" + g_im_save_format, w, h);
			}

			if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_iter" + std::to_string(iter) + "_77_2_ImRGB" + g_im_save_format, w, h);

			LLE[k] = YB + (LMAXY/4);

			break;
		}        

		ww = W*4;
		hh = h;

		for (i = 0; i < ww*hh; i++) ImRES1[i] = 255;

		for(y=0, i=0; y<h; y++)
		for(x=0; x<w; x++, i++)
		{
			if (ImFF[i] != 0)
			{
				ImRES1[y*ww + (XB * 4) + x] = 0;
				ImRES[(YB*4 + y)*ww + (XB * 4) + x] = 0;
			}
		}

		g_pViewRGBImage(ImRES, W*4, H*4);

		GetTextLineParameters(ImFF, ImY, ImU, ImV, w, h, LH, LMAXY, DXB, DXE, DYB, DYE, mY, mI, mQ, rc);

        FullName = string("/TXTImages/");
		FullName += SaveName;

		sprintf(str, "%.2d", (int)SavedFiles.size() + 1);
		FullName += string("_");
        FullName += string(str);

		SaveTextLineParameters(	FullName, YB, 
								LH/4, YB + LMAXY/4, 
								XB + DXB/4, XB + DXE/4,
								YB + DYB/4, YB + DYE/4,
								mY, mI, mQ );

		SavedFiles.push_back(FullName);

		SaveGreyscaleImage(ImRES1, FullName + g_im_save_format, ww, hh, 0, 1.0, -1, 300);

		res = 1;
		k++;
	}

	return res;
}

//Extending ImF with data from ImNF
//adding all figures from ImNF which intersect with minY-maxY figures position in ImF
void ExtendImFWithDataFromImNF(custom_buffer<int> &ImF, custom_buffer<int> &ImNF, int w, int h)
{
	// Getting info about text position in ImF
	int x, y, i, ib, k, bln, l, r, N;
	custom_buffer<int> LL(h, 0), LR(h, 0), LLB(h, 0), LLE(h, 0);

	// Getting info about text lines position in ImF
	// -----------------------------------------------------
	N = 0; // number of lines
	LLB[0] = -1; // line 'i' begin by 'y'
	LLE[0] = -1; // line 'i' end by 'y'
	LL[0] = w - 1; // line 'i' begin by 'x'
	LR[0] = 0; // line 'i' end by 'x'

	for (y = 0, ib = 0; y < h; y++, ib += w)
	{
		bln = 0;
		for (x = 0; x < w; x++)
		{
			if (ImF[ib + x] != 0)
			{
				if (LLB[N] == -1)
				{
					LLB[N] = y;
					LLE[N] = y;
				}
				else
				{
					LLE[N] = y;
				}

				if (bln == 0)
				{
					l = x;
					bln = 1;
				}

				r = x;
			}
		}

		if ((bln == 0) && (LLB[N] != -1))
		{
			N++;
			LLB[N] = -1;
			LLE[N] = -1;
			LL[N] = w - 1;
			LR[N] = 0;
		}

		if (bln == 1)
		{
			if (LL[N] > l) LL[N] = l;
			if (LR[N] < r) LR[N] = r;
		}
	}
	if (LLE[N] == h - 1) N++;

	k = N-2;
	while (k >= 0)
	{
		if ( (LLB[k + 1] - LLE[k] - 1 <= std::min<int>(LLE[k] - LLB[k] + 1, LLE[k + 1] - LLB[k + 1] + 1)) &&
			 (LLB[k + 1] - LLE[k] - 1 <= std::min<int>(LR[k] - LL[k] + 1, LR[k + 1] - LL[k + 1] + 1))
			)
		{
			LLE[k] = LLE[k + 1];
			LL[k] = std::min<int>(LL[k], LL[k + 1]);
			LR[k] = std::max<int>(LR[k], LR[k + 1]);

			for (i = k + 1; i < N - 1; i++)
			{
				LL[i] = LL[i + 1];
				LR[i] = LR[i + 1];
				LLB[i] = LLB[i + 1];
				LLE[i] = LLE[i + 1];
			}
			N--;
			if (k == N - 1)
			{
				k--;
			}
			continue;
		}
		k--;
	}

	for (k = 0; k < N; k++)
	{
		for (y = LLB[k]; y <= LLE[k]; y++)
		{
			for (x = LL[k]; x <= LR[k]; x++)
			{
				if (ImNF[y*w + x] != 0)
				{
					ImF[y*w + x] = 255;
				}
			}
		}
	}
}

void ClearImage4x4(custom_buffer<int> &Im, int w, int h, int white)
{
	int i, j, l, ib, x, y;

	for (y=0, ib=0; y<h; y++, ib+=w)
	{
		x=0;
		i=ib;
		while(x < w)
		{
			if (Im[i] == white)
			{
				j = i;
				while( (Im[j] == white) && (x<w) ) 
				{ 
					j++; 
					x++;
				}

				if (j-i < 4)
				{
					for(l=i; l<j; l++) Im[l] = 0;
				}
				
				i = j;
			}
			else
			{
				x++;
				i++;
			}
		}
	}

	for (x=0; x<w; x++)
	{
		y=0;
		i=x;
		while(y < h)
		{
			if (Im[i] == white)
			{
				j = i;
				while( (Im[j] == white) && (y<h) ) 
				{ 
					j+=w; 
					y++;
				}

				if (j-i < 4*w)
				{
					for(l=i; l<j; l+=w) Im[l] = 0;
				}
				
				i = j;
			}
			else
			{
				y++;
				i+=w;
			}
		}
	}
}

int ClearImageFromBorders(custom_buffer<int> &Im, int w, int h, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0) return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_minX <= 4) ||
			(pFigure->m_maxX >= (w - 1) - 4) ||
			(pFigure->m_minY <= 0) ||
			(pFigure->m_maxY >= (h - 1))
			)
		{
			PA = pFigure->m_PointsArray;

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = (PA[l].m_y*w) + PA[l].m_x;
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;

			continue;
		}

		i++;
	}

	return N;
}

int ClearImageOptimal(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, N /*num of closed figures*/, NNY, min_h;
	int val=0, val1, val2, ddy1, ddy2;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();	

	if (N == 0)	return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}
	
	min_h = std::max<int>(2, (int)((double)LH*0.4));

	ddy1 = std::max<int>(4, LMAXY - ((7 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + ((1 * LH) / 4));

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(0, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h - 1, LMAXY + 3);
	}
	
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];
		
		if ((pFigure->m_h < min_h) ||
			(pFigure->m_minX <= 4) ||
			(pFigure->m_maxX >= (w - 1) - 4) ||
			(pFigure->m_minY < ddy1) ||
			(pFigure->m_maxY > ddy2) ||
			(pFigure->m_w >= LH * 2) ||			
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (6 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (1 * LH) / 5) ||
			((pFigure->m_w < 3) || (pFigure->m_h < 3))
			)
		{		
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = (PA[l].m_y*w)+PA[l].m_x;
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N-1];
			N--;

			continue;
		}

		i++;
	}

	if (N == 0)
	{
		return 0;
	}

	min_h = (int)((double)LH*0.6);
	val1 = LMAXY - LH / 2 - (int)((double)LH*0.2);
	val2 = LMAXY - LH / 2 + (int)((double)LH*0.2);

	l = 0;
	for (i = 0; i < N; i++)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_h >= min_h) &&
			(pFigure->m_minY < val1) &&
			(pFigure->m_maxY > val2))
		{
			l++;
		}
	}

	return l;
}

void CombineFiguresRelatedToEachOther(custom_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h)
{
	int i, j, k;
	CMyClosedFigure *pFigureI, *pFigureJ;
	bool found = true;

	while (found)
	{
		found = false;

		for (i = 0; i < N; i++)
		{
			pFigureI = ppFigures[i];

			for (j = 0; j < N; j++)
			{
				pFigureJ = ppFigures[j];

				if ((i != j))
				{
					int val1 = (pFigureI->m_maxX + pFigureI->m_minX) - (pFigureJ->m_maxX + pFigureJ->m_minX);
					if (val1 < 0) val1 = -val1;
					val1 = ((pFigureI->m_maxX - pFigureI->m_minX) + (pFigureJ->m_maxX - pFigureJ->m_minX) + 2 - val1) / 2;

					int my = (pFigureI->m_maxY + pFigureI->m_minY) / 2;

					if ((pFigureI->m_h >= min_h / 3) && // ImI is not too small
						(pFigureI->m_w >= 1.5*(double)pFigureI->m_h) && // ImI is wider then its height
						(pFigureI->m_w <= 1.5*(double)pFigureJ->m_w) && // ImI is not too wide
						( (pFigureI->m_minY < pFigureJ->m_minY) &&
						  (pFigureI->m_maxY < pFigureJ->m_minY + (pFigureJ->m_h/2)) ) && // ImI is on top of ImJ
						(val1 >= 0.75*(double)pFigureI->m_w) && // ImI intersect with ImJ by x more then 0.75*ImI_width
						((pFigureJ->m_minY - pFigureI->m_maxY - 1) <= std::min<int>(pFigureI->m_h, pFigureJ->m_h) / 4) // distance between ImI and ImJ is not bigger then Images_height / 5
						)
					{
						// combine two images
						*pFigureI += *pFigureJ;
						for (k = j; k < N - 1; k++)
						{
							ppFigures[k] = ppFigures[k + 1];
						}
						found = true;
					}
				}

				if (found) break;
			}
			if (found) break;
		}

		if (found)
		{
			N--;
		}
	}
}

int GetSubParams(custom_buffer<int> &Im, int w, int h, int white, int &LH, int &LMAXY, int &lb, int &le, int min_h, int real_im_x_center)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, val, N, minN, H, delta, NNN, jY, jI, jQ;
	custom_buffer<int> GRStr(256 * 2, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	custom_buffer<int> ImRR(w*h, 0);
	int val1, val2, val3;
	int NNY;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	LMAXY = 0;
	LH = 0;
	lb = h - 1;
	le = 0;

	if (N == 0)	return 0;

	if (N < 2)
	{
		return 0;
	}

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	custom_buffer<int> NN(N, 0), NY(N, 0), NL(N, 0), NR(N, 0);
	custom_buffer<CMyClosedFigure*> good_figures(N);
	
	CombineFiguresRelatedToEachOther(ppFigures, N, min_h);

	for (i = 0, j = 0; i < N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_h >= min_h) &&
			 (pFigure->m_w < pFigure->m_h * 2) &&
			(!((pFigure->m_minX <= 4) ||
				(pFigure->m_maxX >= (w - 1) - 4) ||
				(pFigure->m_minY <= 0) ||
				(pFigure->m_maxY >= (h - 1))
				))
			)
		{
			good_figures[j] = pFigure;
			if (pFigure->m_h > LH)
			{
				LH = pFigure->m_h;
			}
			if (pFigure->m_minY < lb)
			{
				lb = pFigure->m_minY;
			}
			if (pFigure->m_maxY > le)
			{
				le = pFigure->m_maxY;
			}
			j++;
		}
	}
	NNY = j;	

	if (NNY == 0)
	{
		return 0;
	}

	for (i = 0; i < NNY - 1; i++)
	{
		for (j = i + 1; j < NNY; j++)
		{
			if (good_figures[j]->m_maxY > good_figures[i]->m_maxY)
			{
				pFigure = good_figures[i];
				good_figures[i] = good_figures[j];
				good_figures[j] = pFigure;
			}
		}
	}

	j = 0;
	k = 0;
	i = 0;
	NL[k] = good_figures[j]->m_minX;
	NR[k] = good_figures[j]->m_maxX;
	while (i < NNY)
	{
		if ((good_figures[j]->m_maxY - good_figures[i]->m_maxY) > std::max<int>(g_dmaxy, LH / 8))
		{
			NN[k] = i - j;
			NY[k] = good_figures[j]->m_maxY;			
			
			if (j < NNY - 1)
			{
				k++;
				l = j + 1;
				while ((good_figures[l]->m_maxY == good_figures[j]->m_maxY) && (l < NNY - 1)) l++;

				j = i = l;
				NL[k] = good_figures[j]->m_minX;
				NR[k] = good_figures[j]->m_maxX;
			}
		}
		else
		{
			if (good_figures[i]->m_minX < NL[k])
			{
				NL[k] = good_figures[i]->m_minX;
			}
			if (good_figures[i]->m_maxX < NR[k])
			{
				NR[k] = good_figures[i]->m_maxX;
			}
		}

		i++;
	}
	NN[k] = i - j;
	NY[k] = good_figures[j]->m_maxY;
	k++;

	j = k-1;
	for (i = 0; i < k; i++)
	{
		if ((NN[i] >= NN[j]) && (NL[i] < real_im_x_center))
		{
			j = i;
		}
	}
	for (i = 0; i < k; i++)
	{
		if ((NY[i] > NY[j]) && ((NN[i] >= 2) || (NN[i] >= NN[j])) && (NL[i] < real_im_x_center))
		{
			j = i;
		}
	}
	LMAXY = NY[j];

	if (LMAXY < 2)
	{
		return 0;
	}

	custom_buffer<int> arH(N, 0);

	for (i = 0, k = 0; i < NNY; i++)
	{
		pFigure = good_figures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LH / 8))
			)
		{
			arH[k] = pFigure->m_h;
			k++;
		}
	}

	if (k < 1)
	{
		return 0;
	}

	for (i = 0; i < k - 1; i++)
	{
		for (j = i + 1; j < k; j++)
		{
			if (arH[j] > arH[i])
			{
				val = arH[i];
				arH[i] = arH[j];
				arH[j] = val;
			}
		}
	}

	i = 0;
	H = 0;
	while (i < k)
	{
		if ((arH[0] - arH[i]) > arH[0]/5) //sometimes CombineFiguresRelatedToEachOther add garbage, to avoid this using more average height
		{
			break;
		}
		else
		{
			H += arH[i];
			i++;
		}
	}

	LH = H / i;

	return 1;
}

int ClearImageOpt2(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, val, H, N;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	//custom_buffer<int> maxY(N, 0), NN(N, 0), NY(N, 0);	

	H = 0;
	k = 0;
	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_maxY < LMAXY - (LH / 2))
		{
			PA = pFigure->m_PointsArray;

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}
			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}		

		i++;
	}

	return N;
}

int ClearImageFromSmallSymbols(custom_buffer<int> &Im, int w, int h, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N, res = 0, x, y, y2;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_h < g_msh*(double)h) ||
			(pFigure->m_w <= g_segh) ||
			(pFigure->m_h <= g_segh) ||
			(pFigure->m_Square < g_msd*(pFigure->m_h*pFigure->m_w))
			)
		{
			PA = pFigure->m_PointsArray;

			// for debug
			/*if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}*/

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}

	if (g_show_results) SaveGreyscaleImage(Im, "\\TestImages\\ClearImageFromSmallSymbols_01" + g_im_save_format, w, h);

	if (N > 0)
	{
		res = 1;
	}

	return res;
}

void RestoreStillExistLines(custom_buffer<int> &Im, custom_buffer<int> &ImOrig, int w, int h)
{
	int i, x, y, y2;
	
	int dy = 2 * g_segh;
	custom_buffer<int> lines_info(h, 0);

	for (y = 0; y < h; y++)
	{
		for (x = 0, i = y * w; x < w; x++, i++)
		{
			if (Im[i] != 0)
			{
				lines_info[y] = 1;
				break;
			}
		}
	}

	for (y = 0; y < h; y++)
	{
		int found = 0;
		for (y2 = std::max<int>(0, y - dy); y2 <= std::min<int>(y + dy, h - 1); y2++)
		{
			if (lines_info[y2] == 1)
			{
				found = 1;
				break;
			}
		}

		if (found == 1)
		{
			memcpy(&Im[y * w], &ImOrig[y * w], w * sizeof(int));
		}
	}

	if (g_show_results) SaveGreyscaleImage(Im, "\\TestImages\\ClearImageFromSmallSymbols_02" + g_im_save_format, w, h);
}

int ClearImageOpt5(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	custom_buffer<int> GRStr(256 * 2, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	ddy1 = std::max<int>(4, LMAXY - ((7 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + ((1 * LH) / 4));

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(0, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h-1, LMAXY + 3);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_minX <= 4) ||
			(pFigure->m_maxX >= (w - 1) - 4) ||
			(pFigure->m_minY < ddy1) ||
			(pFigure->m_maxY > ddy2) ||
			(pFigure->m_w >= LH * 2) ||				
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY)/2)) > (6*LH)/5) ||
			((((pFigure->m_minY + pFigure->m_maxY)/2) - LMAXY) > (1*LH)/5) ||
			((pFigure->m_w < 3) || (pFigure->m_h < 3))
			)
		{
			PA = pFigure->m_PointsArray;

			// for debug
			/*if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}*/

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}

	int min_x = w - 1, max_x = 0;

	// getting sub min max position by x
	for (i = 0; i < N; i++)
	{
		pFigure = ppFigures[i];
		if ((pFigure->m_maxY <= LMAXY + LH / 8) && (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LH / 8)))
		{
			if (pFigure->m_minX < min_x) min_x = pFigure->m_minX;
			if (pFigure->m_maxX > max_x) max_x = pFigure->m_maxX;
		}
	}

	int min_h = (int)((double)LH*0.6);
	val1 = LMAXY - LH / 2 - (int)((double)LH*0.2);
	val2 = LMAXY - LH / 2 + (int)((double)LH*0.2);

	l = 0;
	for (i = 0; i < N; i++)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_h >= min_h) &&
			(pFigure->m_minY < val1) &&
			(pFigure->m_maxY > val2))
		{
			l++;
		}
	}

	return l;
}

void ClearImageSpecific1(custom_buffer<int> &Im, int w, int h, int yb, int ye, int xb, int xe, int white)
{
	CMyClosedFigure *pFigure;
	CMyPoint *PA;
	clock_t t;
	int bln, dh, i, l, N;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();
	
	if (N == 0)	return;

	dh = (ye-yb+1)/2;

	bln = 0;
	for(i=0; i<N; i++)
	{
		pFigure = &(pFigures[i]);

		if (pFigure->m_h >= dh)
		{
			bln = 1;
			break;
		}
	}
	
	if (bln == 1)
	{
		for(i=0; i<N; i++)
		{
			pFigure = &(pFigures[i]);

			if (pFigure->m_h < dh)
			{
				PA = pFigure->m_PointsArray;
				
				for(l=0; l < pFigure->m_Square; l++)
				{
					Im[PA[l].m_i] = 0;
				}
			}
		}
	}
}

void MergeWithClusterImage(custom_buffer<int> &ImInOut, custom_buffer<int> &ImCluser, int w, int h, int white)
{
	CMyClosedFigure *pFigure1, *pFigure2;
	int i, j, k, l, ii, N1, N2;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures1;
	t = SearchClosedFigures(ImInOut, w, h, white, pFigures1);
	N1 = pFigures1.size();

	for (i = 0; i < N1; i++)
	{
		pFigure1 = &(pFigures1[i]);
		PA = pFigure1->m_PointsArray;

		bool found = false;
		for (l = 0; l < pFigure1->m_Square; l++)
		{
			ii = PA[l].m_i;
			if (ImCluser[ii] != 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure1->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImInOut[ii] = 0;
			}
		}
	}
	
	CombineTwoImages(ImInOut, ImCluser, w, h, white);

	//for (i = 0; i < N2; i++)
	//{
	//	pFigure2 = &(pFigures2[i]);
	//	PA = pFigure2->m_PointsArray;

	//	bool found = false;
	//	for (l = 0; l < pFigure2->m_Square; l++)
	//	{
	//		ii = PA[l].m_i;
	//		if (ImInOut[ii] != 0)
	//		{
	//			found = true;
	//			break;
	//		}
	//	}

	//	// Check, that possibly on ImInOut was fully removed all symbols in same range of x: [pFigure2->m_minX, pFigure2->m_maxX]
	//	if (!found)
	//	{
	//		int bln = 0;
	//		for (int x = pFigure2->m_minX; x <= pFigure2->m_maxX; x++)
	//		{
	//			for (int y = 0; y < h; y++)
	//			{
	//				if (ImInOut[y * w + x] != 0)
	//				{
	//					bln = 1;
	//					break;
	//				}
	//			}

	//			if (bln == 1)
	//			{
	//				break;
	//			}
	//		}

	//		if (bln == 0)
	//		{
	//			found = true;
	//		}
	//	}

	//	if (found)
	//	{
	//		for (l = 0; l < pFigure2->m_Square; l++)
	//		{
	//			ii = PA[l].m_i;
	//			ImInOut[ii] = white;
	//		}
	//	}
	//}	
}

void ClearImageFromGarbageBetweenLines(custom_buffer<int> &Im, int w, int h, int yb, int ye, int min_h, int white)
{
	CMyClosedFigure *pFigureI, *pFigureJ, *pFigureK;
	int i, j, k, l, N;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0) return;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	CombineFiguresRelatedToEachOther(ppFigures, N, min_h);

	bool found = true;

	while (found)
	{
		found = false;

		for (i=0; i < N; i++)
		{
			pFigureI = ppFigures[i];

			for (j=0; j < N; j++)
			{
				pFigureJ = ppFigures[j];
				
				for (k=0; k < N; k++)
				{
					pFigureK = ppFigures[k];

					if ((i != j) && (j != k) && (i != k))
					{
						int val1 = (pFigureI->m_maxX + pFigureI->m_minX) - (pFigureJ->m_maxX + pFigureJ->m_minX);
						if (val1 < 0) val1 = -val1;
						val1 = (pFigureI->m_maxX - pFigureI->m_minX) + (pFigureJ->m_maxX - pFigureJ->m_minX) + 2 - val1;

						int val2 = (pFigureI->m_maxX + pFigureI->m_minX) - (pFigureK->m_maxX + pFigureK->m_minX);
						if (val2 < 0) val2 = -val2;
						val2 = (pFigureI->m_maxX - pFigureI->m_minX) + (pFigureK->m_maxX - pFigureK->m_minX) + 2 - val2;

						int my = (pFigureI->m_maxY + pFigureI->m_minY)/2;
						int miny = (yb + ye) / 2 - (ye - yb + 1) / 8;
						int maxy = (yb + ye) / 2 + (ye - yb + 1) / 8;

						if ( (pFigureI->m_maxY + pFigureI->m_minY > pFigureJ->m_maxY + pFigureJ->m_minY) && // ImI middle is higher then ImJ
							 (pFigureI->m_maxY + pFigureI->m_minY < pFigureK->m_maxY + pFigureK->m_minY) && // ImI middle is lower then ImK	
							 (pFigureI->m_minY > pFigureJ->m_minY) && // ImI bottom is higher then ImJ
							 (pFigureI->m_maxY < pFigureK->m_maxY) && // ImI top is lower then ImK	
							 (val1 > 2) && // ImI intersect with ImJ by x
							 (val2 > 2) && // ImI intersect with ImK by x
							 ((my >= miny) && (my <= maxy)) && // ImI intersect with 1/4 middle of sub (yb + ye)/2
							 ( (pFigureI->m_maxY - pFigureI->m_minY < pFigureJ->m_maxY - pFigureJ->m_minY) || // ImI is smoler then ImJ or ImK	
							 (pFigureI->m_maxY - pFigureI->m_minY < pFigureK->m_maxY - pFigureK->m_minY) )
							)
						{
							PA = pFigureI->m_PointsArray;

							for (l = 0; l < pFigureI->m_Square; l++)
							{
								Im[PA[l].m_i] = 0;
							}
							ppFigures[i] = ppFigures[N - 1];
							N--;
							break;
						}
					}
				}

				if (found) break;
			}
			if (found) break;
		}
	}
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int IsPoint(CMyClosedFigure *pFigure, int LMAXY, int LLH, int W, int H)
{
	int ret;
	double dval;
	
	ret = 0;

	if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
	else dval = (double)pFigure->m_w/pFigure->m_h;

	if ( (pFigure->m_w >= g_minpw * W) && (pFigure->m_w <= g_maxpw * W) &&
	     (pFigure->m_h >= g_minph * H) && (pFigure->m_h <= g_maxph * H) &&
	     (dval >= g_minpwh) ) 
	{
		if ( ( (pFigure->m_maxY <= LMAXY + 4) && 
			   (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LLH / 8)) ) ||
			 ( (pFigure->m_maxY <= LMAXY-LLH) && 
			   (pFigure->m_maxY >= LMAXY-LLH*1.25) )
			)
		{
			ret = 1;
		}
	}

	return ret;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int IsComma(CMyClosedFigure *pFigure, int LMAXY, int LLH, int W, int H)
{
	int ret;
	double dval;
	
	ret = 0;

	if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
	else dval = (double)pFigure->m_w/pFigure->m_h;

	if ( (pFigure->m_w >= g_minpw * W) && (pFigure->m_w <= 2 * g_maxpw * W) &&
	     (dval <= (2.0/3.0)) && (pFigure->m_h <= (int)((double)LLH*0.8)) )
	{
		if ( ( (pFigure->m_minY <= LMAXY-LLH) &&
		       (pFigure->m_maxY >= LMAXY-LLH-8) ) || 
	         ( (pFigure->m_maxY > LMAXY) && 
		       (pFigure->m_minY < LMAXY) )
		   )
		{
			ret = 1;
		}
	}

	return ret;
}

void SaveTextLineParameters(string ImageName, int YB, int LH, int LY, int LXB, int LXE, int LYB, int LYE, int mY, int mI, int mQ)
{
	char str[100];
	string PropString, fname;
	ofstream fout;

	sprintf(str, "%.4d", YB);
	PropString = string("YB ");
    PropString += string(str);
	
	sprintf(str, "%.4d", LH);
	PropString += string(" LH ");
    PropString += string(str);

	sprintf(str, "%.4d", LY);
	PropString += string(" LY ");
    PropString += string(str);
	
	sprintf(str, "%.4d", LXB);
	PropString += string(" LXB ");
    PropString += string(str);

	sprintf(str, "%.4d", LXE);
	PropString += string(" LXE ");
    PropString += string(str);

	sprintf(str, "%.4d", LYB);
	PropString += string(" LYB ");
    PropString += string(str);

	sprintf(str, "%.4d", LYE);
	PropString += string(" LYE ");
    PropString += string(str);

	sprintf(str, "%.3d %.3d %.3d", mY, mI, mQ);
	PropString += string(" YIQ ");
    PropString += string(str);		
	
	fname = g_work_dir + string("\\text_lines.info");
	fout.open(fname.c_str(), ios::out | ios::app);

	fout << ImageName << " = " << PropString << '\n';

	fout.close();
}

void GetSymbolAvgColor(CMyClosedFigure *pFigure, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ)
{
	CMyPoint *PA;
	int i, ii, j, w, h, x, y, xx, yy, val;
	int r, min_x, max_x, min_y, max_y, mY, mI, mQ, weight;

	w = pFigure->m_maxX - pFigure->m_minX + 1;
	h = pFigure->m_maxY - pFigure->m_minY + 1;

	r = max(8, h/6);

	int SIZE = w * h;

	custom_buffer<int> pImage(SIZE, 0), pImageY(SIZE, 0), pImageI(SIZE, 0), pImageQ(SIZE, 0);

	PA = pFigure->m_PointsArray;

	for(i=0; i < pFigure->m_Square; i++)
	{
		ii = PA[i].m_i;
		j = (PA[i].m_y - pFigure->m_minY)*w + (PA[i].m_x - pFigure->m_minX);

		pImage[j] = 1;
		pImageY[j] = ImY[ii];
		pImageI[j] = ImI[ii];
		pImageQ[j] = ImQ[ii];
	}

	//находим все точки границы
	for(y=0, i=0; y<h; y++)
	{
		for(x=0; x<w; x++, i++)
		{
			if (pImage[i] == 1)
			{
				if ( (x==0) || (x==w-1) ||
				  	 (y==0) || (y==h-1) )
				{
					pImage[i] = -1;
				}
				else
				{
					if ( (pImage[i-1] == 0) ||
						 (pImage[i+1] == 0) ||
						 (pImage[i-w] == 0) ||
						 (pImage[i+w] == 0) ||
						 (pImage[i-1-w] == 0) ||
						 (pImage[i+1-w] == 0) ||
						 (pImage[i-1+w] == 0) ||
						 (pImage[i+1+w] == 0) )
					{
						pImage[i] = -1;
					}
				}
			}
		}
	}

	do
	{
		//помечаем все точки символа отстоящие от границы не более чем на r
		for(y=0, i=0; y<h; y++)
		{
			for(x=0; x<w; x++, i++)
			{
				if (pImage[i] == -1)
				{
					min_x = max(0, x-r);
					max_x = min(w-1, x+r);
					min_y = max(0, y-r);
					max_y = min(h-1, y+r);

					for (yy=min_y; yy<max_y; yy++)
					for (xx=min_x; xx<max_x; xx++)
					{
						j = yy*w + xx;

						if (pImage[j] == 1)
						{
							val = (yy-y)*(yy-y) + (xx-x)*(xx-x);

							if (val <= r*r)
							{
								pImage[j] = -2;	
							}
						}					
					}
				}
			}
		}

		weight = 0;
		mY = 0;
		mI = 0;
		mQ = 0;

		for(y=0, i=0; y<h; y++)
		{
			for(x=0; x<w; x++, i++)
			{
				if (pImage[i] == 1)
				{
					mY += pImageY[i];
					mI += pImageI[i];
					mQ += pImageQ[i];
					weight++;
				}
			}
		}

		if (weight < pFigure->m_Square/5)
		{
			if (r == 0)
			{
				if (weight == 0)
				{
					for(y=0, i=0; y<h; y++)
					{
						for(x=0; x<w; x++, i++)
						{
							if (pImage[i] == -1)
							{
								mY += pImageY[i];
								mI += pImageI[i];
								mQ += pImageQ[i];
								weight++;
							}
						}
					}
				}

				break;
			}

			for(y=0, i=0; y<h; y++)
			{
				for(x=0; x<w; x++, i++)
				{
					if (pImage[i] == -2)
					{
						pImage[i] = 1;
					}
				}
			}
			r = (r*3)/4;
		}
	} while (weight < pFigure->m_Square/5);

	mY = mY/weight;
	mI = mI/weight;
	mQ = mQ/weight;

	pFigure->m_mY = mY;
	pFigure->m_mI = mI;
	pFigure->m_mQ = mQ;
	pFigure->m_Weight = weight;
}

void GetTextLineParameters(custom_buffer<int> &Im, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h, int &LH, int &LMAXY, int &XB, int &XE, int &YB, int &YE, int &mY, int &mI, int &mQ, int white)
{
	CMyClosedFigure *pFigure = NULL;
	CMyPoint *PA = NULL;
	int i, j, k, l, N, val, val1, val2, val3, val4;
	int *NH = NULL, NNY, min_h, min_w, prev_min_w;
	clock_t t;

	LH = 14*4;
	XB = w/2;
	XE = w/2;
	YB = h/2-LH/2;
	YE = YB + LH - 1;	
	LMAXY = YE;
	mY = 0;
	mI = 0;
	mQ = 0;

	min_h = (int)(0.6*(double)LH);

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0) return;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	// определяем цвет текста

	k = 0;
	min_w = prev_min_w = min_h;

	while (k == 0)
	{
		for(i=0; i<N; i++)
		{
			pFigure = ppFigures[i];

			if ( (pFigure->m_h >= min_h) && (pFigure->m_w >= min_w) )
			{				
				k++;
			}
		}

		if (k == 0)
		{
			prev_min_w = min_w;
			min_w = (min_w*2)/3;
		}

		if (prev_min_w == 0)
		{
			return;
		}
	}

	val = 0;
	val1 = 0;
	val2 = 0;
	val3 = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];
		
		if ( (pFigure->m_h >= min_h) && (pFigure->m_w >= min_w) )
		{		
			GetSymbolAvgColor(pFigure, ImY, ImI, ImQ);

			val += pFigure->m_Weight;
			val1 += pFigure->m_mY*pFigure->m_Weight;
			val2 += pFigure->m_mI*pFigure->m_Weight;
			val3 += pFigure->m_mQ*pFigure->m_Weight;
		}
	}

	mY = val1/val;
	mI = val2/val;
	mQ = val3/val;

	// определяем размеры символов и расположение текста по высоте

	custom_buffer<int> maxY(N, 0), NN(N, 0), NY(N, 0);

	for(i=0, j=0; i < N; i++)
	{
		if (ppFigures[i]->m_h >= min_h)
		{
			maxY[j] = ppFigures[i]->m_maxY;
			j++;
		}
	}
	NNY = j;

	for(i=0; i<NNY-1; i++)
	{
		for(j=i+1; j<NNY; j++)
		{
			if(maxY[j] > maxY[i])
			{
				val = maxY[i];
				maxY[i] = maxY[j];
				maxY[j] = val;
			}
		}
	}

	// отыскиваем группы символов, чья высота различается не более чем на g_dmaxy по всевозможным высотам
	// (такие группы могут частично содержать одни и теже символы)
	j=0;
	k=0;
	i=0;
	while(i < NNY)
	{
		if ((maxY[j]-maxY[i]) > g_dmaxy)
		{
			NN[k] = i-j;
			NY[k] = maxY[j];
			k++;
			
			l = j+1;
			while(maxY[l] == maxY[j]) l++;

			j = i = l;
		}

		i++;
	}
	NN[k] = i-j;
	NY[k] = maxY[j];
	k++;

	val = NN[0];
	j = 0;
	for(i=0; i<k; i++)
	{
		if(NN[i] > val)
		{
			val = NN[i];
			j = i;
		}
	}

	if (val > 1)
	{
		LMAXY = NY[j];
	}
	else if (val == 1)
	{
		val = maxY[NNY-1] + g_dmaxy;
		j = NNY-2;

		while ((j >= 0) && (maxY[j] <= val)) j--;
		j++;
		
		LMAXY = NY[j];
	}

	if (val == 0)
	{
		return;
	}
	
	XB = w-1;
	XE = 0;
	YB = h-1;
	YE = 0;
	val1 = 0;
	val2 = h-1;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_minX < XB)
		{
			XB = pFigure->m_minX;
		}

		if (pFigure->m_maxX > XE)
		{
			XE = pFigure->m_maxX;
		}

		if (pFigure->m_minY < YB)
		{
			YB = pFigure->m_minY;
		}

		if (pFigure->m_maxY > YE)
		{
			YE = pFigure->m_maxY;
		}

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
             (pFigure->m_h >= min_h) )
		{
			if (pFigure->m_minY > val1)
			{
				val1 = pFigure->m_minY;
			}

			if (pFigure->m_minY < val2)
			{
				val2 = pFigure->m_minY;
			}
		}
	}

	val3 = (val1*2 + val2)/3;
	val4 = val2 + (int)((double)(LMAXY-val2+1)*0.12) + 1;

	val1 = 0;
	val2 = 0;
	j = 0;
	k = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
			(pFigure->m_maxY >= LMAXY-g_dmaxy) &&
			(pFigure->m_h >= min_h) )
		{
			if (pFigure->m_minY >= val3)
			{
				val1 += pFigure->m_minY;
				j++;
			}
			if (pFigure->m_minY >= val4)
			{
				val2 += pFigure->m_minY;
				k++;
			}
		}
	}

	if (j > 2)
	{
		val = val1/j;
	}
	else
	{
		if (k > 0)
		{
			val = val2/k;
		}
		else
		{
			if (j > 0)
			{
				val = val1/j;
			}
			else
			{				
				val = LMAXY + 1 - LH;
			}
		}
	}

	LH = LMAXY - val + 1;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int ClearImageLogical(custom_buffer<int> &Im, int w, int h, int &LH, int &LMAXY, int xb, int xe, int white, int W, int H, int real_im_x_center)
{
	CMyClosedFigure *pFigure = NULL, *pFigure2 = NULL;
	int i, ib, i1, i2, i3, j, k, l, x, y, val, val1, N, bln, bln1, bln2, bln3, LMINY, LM1, LM2;
	int res, is_point, is_comma, LLH;
	CMyPoint *PA = NULL, *PA1 = NULL, *PA2 = NULL;
	clock_t t;
	double minpw, maxpw, minph, maxph;
	double minpwh;
	double dval;

	int NNY, min_h;

	res = 0;

	minpw = g_minpw;
	maxpw = g_maxpw;
	minph = g_minph;
	maxph = g_maxph;
	minpwh = g_minpwh;

	// Увеличиваем область основных текстовых симоволов
	// если её длина меньше 120 пикселов на оригинальном изображении

	val = 120*4 - (xe-xb+1);
	if (val > 0)
	{
		val =  val/2;
		xb -= val;
		xe += val;

		if (xb < 0) xb = 0;
		if (xe > w-1) xe = w-1;
	}

	// Убиваем все горизонтальные отрезки чья длина <= 2
	// (что означает что в оригинале их длина меньше 1-го пиксела)

	for (y=0, ib=0; y<h; y++, ib+=w)
	{
		x=0;
		i=ib;
		while(x < w)
		{
			if (Im[i] == white)
			{
				j = i;
				while( (Im[j] == white) && (x<w) ) 
				{ 
					j++; 
					x++;
				}

				if (j-i < 2)
				{
					for(l=i; l<j; l++) Im[l] = 0;
				}
				
				i = j;
			}
			else
			{
				x++;
				i++;
			}
		}
	}

	// Убиваем все вертикальные отрезки чья длина <= 2
	// (что означает что в оригинале их длина меньше 1-го пиксела)

	for (x=0; x<w; x++)
	{
		y=0;
		i=x;
		while(y < h)
		{
			if (Im[i] == white)
			{
				j = i;
				while( (Im[j] == white) && (y<h) ) 
				{ 
					j+=w; 
					y++;
				}

				if (j-i < 2*w)
				{
					for(l=i; l<j; l+=w) Im[l] = 0;
				}
				
				i = j;
			}
			else
			{
				y++;
				i+=w;
			}
		}
	}

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_Im" + g_im_save_format, w, h);

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return res;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	custom_buffer<CMyClosedFigure*> ppFgs(N);
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	for (i=0; i<N-1; i++)
	{
		for (j=i+1; j<N; j++)
		{
			if (ppFigures[j]->m_minX < ppFigures[i]->m_minX)
			{
				pFigure = ppFigures[i];
				ppFigures[i] = ppFigures[j];
				ppFigures[j] = pFigure;
			}
		}
	}

	//--------------------
	custom_buffer<int> maxY(N, 0), NN(N, 0), NY(N, 0);

	min_h = (int)(0.6*(double)LH);

	for(i=0, j=0; i < N; i++)
	{
		val = (ppFigures[i]->m_minX+ppFigures[i]->m_maxX)/2;

		if ( (ppFigures[i]->m_h >= min_h) && 
			 (val > xb) &&
			 (val < xe) )
		{
			maxY[j] = ppFigures[i]->m_maxY;
			j++;
		}
	}
	NNY = j;

	for(i=0; i<NNY-1; i++)
	{
		for(j=i+1; j<NNY; j++)
		{
			if(maxY[j] > maxY[i])
			{
				val = maxY[i];
				maxY[i] = maxY[j];
				maxY[j] = val;
			}
		}
	}

	// отыскиваем группы символов, чья высота различается не более чем на g_dmaxy по всевозможным высотам
	// (такие группы могут частично содержать одни и теже символы)
	j=0;
	k=0;
	i=0;
	while(i < NNY)
	{
		if ((maxY[j]-maxY[i]) > g_dmaxy)
		{
			NN[k] = i-j;
			NY[k] = maxY[j];
			k++;
			
			l = j+1;
			while(maxY[l] == maxY[j]) l++;

			j = i = l;
		}

		i++;
	}
	NN[k] = i-j;
	NY[k] = maxY[j];
	k++;

	val = NN[0];
	j = 0;
	for(i=0; i<k; i++)
	{
		if(NN[i] > val)
		{
			val = NN[i];
			j = i;
		}
	}

	if (val > 1)
	{
		LMAXY = NY[j];
	}
	else if (val == 1)
	{
		for(i=0, j=0; i < N; i++)
		{
			val1 = (ppFigures[i]->m_minX+ppFigures[i]->m_maxX)/2;

			if ( (ppFigures[i]->m_h >= min_h) && 
				(val1 > xb) &&
				(val1 < xe) )
			{
				LMAXY = ppFigures[i]->m_maxY;
				break;
			}
		}
	}

	if (val == 0)
	{
		return res;
	}
	//--------------------

	//--------------------
	LH = 0;
	j = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
             (pFigure->m_h >= 0.6*LH) )
		{
			LH += pFigure->m_h;
			j++;
		}
	}

	if (j == 0) return res;

	LH = LH/j;
	//--------------------

	LMINY = LMAXY - (int)((double)LH*1.25);
	LM1 = LMAXY - LH*2;
	LM2 = LMAXY - LH/2;
	
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		is_point = IsPoint(pFigure, LMAXY, LH, W, H);
		is_comma = IsComma(pFigure, LMAXY, LH, W, H);
	
		// add 'Й' support for not filter

		if ( ( (pFigure->m_maxY < LMAXY-LH) && 
			   (pFigure->m_minY < LMAXY - (LH*1.5)) &&
			   (is_point == 0) && (is_comma == 0) ) ||
			 ( pFigure->m_maxY >= LMAXY+((3*LH)/4) ) )
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;

			continue;	
		}

		i++;
	}

	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		is_point = IsPoint(pFigure, LMAXY, LH, W, H);
		is_comma = IsComma(pFigure, LMAXY, LH, W, H);

		if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
		else dval = (double)pFigure->m_w/pFigure->m_h;
		
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (  ( (pFigure2->m_maxY <= LMAXY) && (pFigure2->m_maxY >= LMAXY-g_dmaxy) ) ||
					( (pFigure2->m_minY*2 < 2*LMAXY-LH) && (pFigure2->m_maxY > LMAXY) )
				 ) &&
				 (pFigure2->m_h >= 0.6*LH)
				)
			{
				val = (pFigure->m_maxX+pFigure->m_minX)-(pFigure2->m_maxX+pFigure2->m_minX);
				if (val < 0) val = -val;
				val = (pFigure->m_maxX-pFigure->m_minX)+(pFigure2->m_maxX-pFigure2->m_minX)+2 - val;

				if (val >= 2)//наезжают друг на друга минимум на 2 пиксела
				{
					ppFgs[k] = pFigure2;
					k++;
				}
			}
		}
		
		if (k >= 2) 
		{
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-g_dmaxy) ) ||
					( (pFigure->m_minY*2 < 2*LMAXY-LH) && (pFigure->m_maxY > LMAXY) )
				 ) &&
				 (pFigure->m_h >= 0.6*LH)
				)
			{
				if (k == 4)
				{
					bln = 1;
				}

				if ( (bln == 0) &&
					 (ppFgs[0]->m_minY <= LMAXY-0.9*LH) && 
					 (ppFgs[1]->m_minY <= LMAXY-0.9*LH) && 
					 (pFigure->m_minY < ppFgs[0]->m_minY) &&
					 (pFigure->m_minY < ppFgs[1]->m_minY)
					)
				{
					PA = pFigure->m_PointsArray;
			
					if (ppFgs[1]->m_minX < ppFgs[0]->m_minX)
					{
						pFigure2 = ppFgs[0];
						ppFgs[0] = ppFgs[1];
						ppFgs[1] = pFigure2;
					}

					bln1 = 0;
					for(l=0; l < pFigure->m_Square; l++)
					{
						if ( (PA[l].m_x < ppFgs[0]->m_maxX) &&
							 (PA[l].m_y < ppFgs[0]->m_minY) )
						{
							bln1 = 1;
							break;
						}
					}

					if (bln1 == 1)
					{
						bln1 = 0;

						for(l=0; l < pFigure->m_Square; l++)
						{
							if ( (PA[l].m_x > ppFgs[1]->m_minX) &&
								(PA[l].m_y < ppFgs[1]->m_minY) )
							{
								bln1 = 1;
								break;
							}
						}

						if (bln1 == 1)
						{
							bln = 1;
						}
					}
				}
			}
			else
			{
				if ( !( (pFigure->m_minY >= LM1) && (pFigure->m_maxY <= LM2) && (k==2) && (pFigure->m_w <= maxpw * W) ) )
				{
					bln = 1;
				}
			}
		}
		else if(k == 1)
		{						
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-g_dmaxy) ) ||
					( (pFigure->m_minY*2 < 2*LMAXY-LH) && (pFigure->m_maxY > LMAXY) )
				 ) &&
				 (pFigure->m_h >= 0.6*LH)
				)
			{
				if ( !( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-g_dmaxy) ) &&
					 (pFigure->m_minY < ppFgs[0]->m_minY) && (pFigure->m_maxY > ppFgs[0]->m_maxY) &&
					 (pFigure->m_minX <= ppFgs[0]->m_minX) && (pFigure->m_maxX >= ppFgs[0]->m_maxX) )
				{
					PA = pFigure->m_PointsArray;

					y = ppFgs[0]->m_maxY;
					for (x = ppFgs[0]->m_minX; x<=ppFgs[0]->m_maxX; x++)
					{
						bln1 = 0;

						for(j=0; j < pFigure->m_Square; j++)
						{
							if ( (PA[j].m_x == x) && (PA[j].m_y >= y) )
							{
								bln1 = 1;
								break;
							}
						}

						if (bln1 == 1) break;
					}

					if (bln1 == 1)
					{
						bln = 1;
					}
				}
			}
			else
			{
				val = (pFigure->m_maxX+pFigure->m_minX)-(ppFgs[0]->m_maxX+ppFgs[0]->m_minX);
				if (val < 0) val = -val;				
				val = (pFigure->m_maxX-pFigure->m_minX)+(ppFgs[0]->m_maxX-ppFgs[0]->m_minX)+2 - val;

				bln1 = ( (pFigure->m_maxY <= LMAXY + 4) && (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
						 (pFigure->m_w >= minpw *W) && (pFigure->m_w <= maxpw * W) &&
					     (pFigure->m_h >= minph * H) && (pFigure->m_h <= maxph * H) &&
				         (dval >= minpwh) );

				if ( ( (pFigure->m_minY >= ppFgs[0]->m_minY) && (pFigure->m_maxY <= ppFgs[0]->m_maxY) && (bln1 == 0) ) ||
					 ( (pFigure->m_minY < ppFgs[0]->m_minY) && (pFigure->m_maxY > ppFgs[0]->m_maxY) ) ||
					 (pFigure->m_minY > LMAXY) ||
					 (pFigure->m_maxY < LMINY) ||
					 ( (pFigure->m_h >= 0.6*LH) && !( (pFigure->m_maxY < LMAXY) && (pFigure->m_minY <= LMAXY-LH) && (pFigure->m_h <= LH*1.5) && (pFigure->m_maxX > ppFgs[0]->m_maxX) ) ) ||
					 ( (pFigure->m_h < pFigure->m_w) && 
					   ( (pFigure->m_maxY > LMAXY) || (pFigure->m_minY > LMAXY-0.25*LH) ) &&
					   !( (pFigure->m_w >= minpw * W) && (pFigure->m_w <= maxpw * W) &&
					      (pFigure->m_h >= minph * H) && (pFigure->m_h <= maxph * H) &&
				          (dval >= minpwh) && ((double)val/ppFgs[0]->m_w < 0.25)
						)
					 )
					)
				{
					bln = 1;
				}
			}
		}
		else
		{
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-g_dmaxy) ) ||
					( (pFigure->m_minY*2 < 2*LMAXY-LH) && (pFigure->m_maxY > LMAXY) )
				 ) &&
				 (pFigure->m_h >= 0.6*LH)
				)
			{
			}
			else
			{
				if ( (pFigure->m_minY > LMAXY) || 
					 (pFigure->m_maxY < LMINY) ||
					 ( (pFigure->m_h >= 0.6*LH) && (pFigure->m_maxY > LMAXY) && 
					   !( (pFigure->m_h < 0.8*LH) && (is_comma == 1) ) ) ||
					 ( (pFigure->m_h < pFigure->m_w) && 
					   ( (pFigure->m_maxY > LMAXY) || (pFigure->m_minY > LMAXY-0.2*LH) ) &&
					   !( (pFigure->m_w >= minpw * W) && (pFigure->m_w <= maxpw * W) &&
				          (pFigure->m_h >= minph * H) && (pFigure->m_h <= maxph * H) &&
				          (dval >= minpwh) 
						)
					 )
					)
				{
					bln = 1;
				}
			}
		}

		if ( (pFigure->m_h < minph * H) || ( (pFigure->m_w < minpw * W) && ( pFigure->m_h < 2*pFigure->m_w) ) ||
			 ( (pFigure->m_w < minpw * W) &&
			   ( !( (pFigure->m_maxY < LMAXY-0.2*LH) && ((pFigure->m_minY + pFigure->m_maxY)/2 >= LMINY) ) && 
			     !( (pFigure->m_maxY > LMAXY) && (pFigure->m_minY < LMAXY) ) 
			   ) 
			 )
		   )
		{
			bln = 1;
		}

		if (bln == 1)
		{		
			PA1 = pFigure->m_PointsArray;

			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA1[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;

			continue;
		}

		i++;
	}

	min_h = (int)((double)LH*0.6);

	//-----очищаем с левого края-----//
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY < LMAXY-g_dmaxy*1.5) ||
			 (pFigure->m_h < min_h) ||
			 (pFigure->m_h > LH*2)
		   )
		{
			PA = pFigure->m_PointsArray;
					
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;
		}
		else
		{
			if ( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
				 (pFigure->m_h >= min_h) )
			{
				break;
			}
			else 
			{
				i++;
			}
		}
	}

	//-----очищаем с правого края-----//
	i=N-1;
	while(i >= 0)
	{
		pFigure = ppFigures[i];
		is_comma = IsComma(pFigure, LMAXY, LH, W, H);

		if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
		else dval = (double)pFigure->m_w/pFigure->m_h;

		if ( (pFigure->m_minY < LMAXY - 2*LH) ||
		     ( (pFigure->m_h < min_h) && 
			   !( (pFigure->m_maxY <= LMAXY + 4) && (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
				  (pFigure->m_w >= minpw * W) && (pFigure->m_w <= maxpw * W) &&
				  (pFigure->m_h >= minph * H) && (pFigure->m_h <= maxph * H) &&
				  (dval >= minpwh) 
				 ) &&
			   (is_comma == 0)
			 )
		   )
		{
			PA = pFigure->m_PointsArray;
					
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;

			i--;
		}
		else
		{
			if ( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
				 (pFigure->m_h >= min_h) )
			{
				break;
			}
			else 
			{
				i--;
			}
		}
	}

	//--определяем минимальную высоту нормальных символов--//
	custom_buffer<int> NH(N, 0);

	k = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
				 (pFigure->m_h >= min_h) )
		{
			NH[k] = LMAXY - pFigure->m_minY;
			k++;
		}
	}

	for(i=0; i<k-1; i++)
	{
		for(j=i+1; j<k; j++)
		{
			if(NH[j] > NH[i])
			{
				val = NH[i];
				NH[i] = NH[j];
				NH[j] = val;
			}
		}
	}
	val = (int)((double)k*0.8)-1;
	if (val < 0) val = k-1;
	LLH = NH[val];

	//-----финальная упорядоченная очистка-----//
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];
		PA1 = pFigure->m_PointsArray;

		is_point = IsPoint(pFigure, LMAXY, LLH, W, H);
		is_comma = IsComma(pFigure, LMAXY, LLH, W, H);
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (pFigure2->m_maxY <= LMAXY) && 
				 (pFigure2->m_maxY >= LMAXY-g_dmaxy) &&
				 (pFigure2->m_h >= min_h)
			   )
			{
				val = (pFigure->m_maxX+pFigure->m_minX)-(pFigure2->m_maxX+pFigure2->m_minX);
				if (val < 0) val = -val;
				val = (pFigure->m_maxX-pFigure->m_minX)+(pFigure2->m_maxX-pFigure2->m_minX)+1 - val;

				if (val >= 1)//наезжают друг на друга минимум на 1 пиксела
				{
					ppFgs[k] = pFigure2;
					k++;
				}
			}
		}

		if (k > 0)
		{
			if (bln == 0)
			{
				for(i1=0; i1<k; i1++)
				{
					PA2 = ppFgs[i1]->m_PointsArray;

					for (i2=0; i2<pFigure->m_Square; i2++)
					{
						bln1 = 0;
						bln2 = 0;
						bln3 = 0;

						for (i3=0; i3<ppFgs[i1]->m_Square; i3++)
						{
							if ( (PA1[i2].m_x == PA2[i3].m_x) &&
								(PA1[i2].m_y > PA2[i3].m_y) )
							{
								bln1 = 1;
							}
							
							if ( (PA1[i2].m_x > PA2[i3].m_x) &&
								(PA1[i2].m_y == PA2[i3].m_y) )
							{
								bln2 = 1;
							}

							if ( (PA1[i2].m_x < PA2[i3].m_x) &&
								(PA1[i2].m_y == PA2[i3].m_y) )
							{
								bln3 = 1;
							}
						}
						bln = bln1 & bln2 & bln3;

						if (bln == 1) break;
					}

					if (bln == 1) break;
				}
			}
		}

		if (bln == 1)
		{					
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA1[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;

			continue;
		}

		i++;
	}

	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];
		PA1 = pFigure->m_PointsArray;

		is_point = IsPoint(pFigure, LMAXY, LLH, W, H);
		is_comma = IsComma(pFigure, LMAXY, LLH, W, H);
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (pFigure2->m_maxY <= LMAXY) && 
				 (pFigure2->m_maxY >= LMAXY-g_dmaxy) &&
				 (pFigure2->m_h >= min_h)
			   )
			{
				val = (pFigure->m_maxX+pFigure->m_minX)-(pFigure2->m_maxX+pFigure2->m_minX);
				if (val < 0) val = -val;
				val = (pFigure->m_maxX-pFigure->m_minX)+(pFigure2->m_maxX-pFigure2->m_minX)+1 - val;

				if ( (pFigure->m_minY <= LMAXY-LLH) &&
					 (pFigure->m_maxY >= LMAXY) )
				{
					if (val >= 5)//наезжают друг на друга минимум на 2 пиксела
					{
						ppFgs[k] = pFigure2;
						k++;
					}
				}
				else if ( (pFigure->m_minY <= LMAXY-LLH) &&
						  (pFigure->m_maxY <= LMAXY) &&
						  (pFigure->m_maxY >= LMAXY-g_dmaxy) )
				{
					//if (val >= 3)//наезжают друг на друга минимум на 3 пиксела
					if (val*2 >= pFigure2->m_maxX - pFigure2->m_minX + 1)
					{
						ppFgs[k] = pFigure2;
						k++;
					}
				}
				else if (val >= 1)//наезжают друг на друга минимум на 1 пиксела
				{
					ppFgs[k] = pFigure2;
					k++;
				}
			}
		}

		if (k > 0)
		{
			if ( (is_point == 0) &&
				    (is_comma == 0) )
			{
				for(i1=0; i1<k; i1++)
				{
					if (ppFgs[i1]->m_minX > pFigure->m_minX)
					{
						PA2 = ppFgs[i1]->m_PointsArray;

						for (i2=0; i2<pFigure->m_Square; i2++)
						{
							for (i3=0; i3<ppFgs[i1]->m_Square; i3++)
							{
								if ( (PA1[i2].m_x == PA2[i3].m_x) &&
									(PA1[i2].m_y > PA2[i3].m_y) )
								{
									bln = 1;
									break;
								}
							}

							if (bln == 1) break;
						}
					}

					if (bln == 1) break;
				}
				
				if (bln == 0)
				{
					for(i1=0; i1<k; i1++)
					{
						if (ppFgs[i1]->m_maxX < pFigure->m_maxX)
						{
							PA2 = ppFgs[i1]->m_PointsArray;

							for (i2=0; i2<pFigure->m_Square; i2++)
							{
								for (i3=0; i3<ppFgs[i1]->m_Square; i3++)
								{
									if ( (PA1[i2].m_x == PA2[i3].m_x) &&
										(PA1[i2].m_y < PA2[i3].m_y) )
									{
										bln = 1;
										break;
									}
								}

								if (bln == 1) break;
							}
						}

						if (bln == 1) break;
					}
				}
			}
		}

		if (bln == 1)
		{					
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA1[l].m_i] = 0;
			}

			for(j=i; j<N-1; j++) ppFigures[j] = ppFigures[j+1];
			N--;

			continue;
		}

		i++;
	}

	//////////////////////////////
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		if ( !( (pFigure->m_maxY <= LMAXY) && 
			    (pFigure->m_maxY >= LMAXY-g_dmaxy) &&
			    (pFigure->m_h >= 0.6*LH) ) )
		{
			ppFigures[i] = ppFigures[N-1];
			N--;
			continue;
		}

		i++;
	}

	if (N >= 2)
	{
		val = w;

		for (i=0; i<N-1; i++)
		{
			for(j=i+1; j<N; j++)
			{
				val1 = ((ppFigures[j]->m_maxX + ppFigures[j]->m_minX) - (ppFigures[i]->m_maxX + ppFigures[i]->m_minX))/2;
				if (val1 < 0) val1 = -val1;
				
				if (val1 < val) val = val1;				
			}
		}

		if (val < min(120*4, w/2)) res = 1;
	}
	else if (N == 1)
	{
		pFigure = ppFigures[0];

		if ( pFigure->m_minX < real_im_x_center )
		{
			res = 1;
		}
	}

	// checking that on image present normal symbols which are not like big point '.'
	if ((res == 1) && (N >= 2))
	{
		res = 0;
		i = 0;
		while (i < N)
		{
			pFigure = ppFigures[i];
			if (((double)pFigure->m_Square / (double)(pFigure->m_w * pFigure->m_h)) <= 0.6)
			{
				res = 1;
				break;
			}
			i++;
		}
	}
	//////////////////////////////

	return res;
}

void StrAnalyseImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset)
{
	int i, ib, ie, x, y, val;

	memset(&GRStr[0], 0, (256+offset)*sizeof(int));

	ib = yb*w;
	ie = ye*w;
	for (y=yb; y<=ye; y++, ib+=w)
	{
		for (x=xb; x<=xe; x++)
		{
			i = ib + x; 
			
			if ( Im[i] != 0 )
			{
				val = ImGR[i]+offset; 
				if (val<0)
				{
					assert(false);
				}
				if (val >= 256+offset)
				{
					assert(false);
				}
				GRStr[val]++;
			}
		}
	}
}

void StrAnalyseImage(CMyClosedFigure *pFigure, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int offset)
{
	int l, val;
	CMyPoint *PA;

	memset(&GRStr[0], 0, STR_SIZE*sizeof(int));

	PA = pFigure->m_PointsArray;
		
	for(l=0; l < pFigure->m_Square; l++)
	{
		val = ImGR[PA[l].m_i]+offset; 
		GRStr[val]++;
	}
}

void FindMaxStrDistribution(custom_buffer<int> &GRStr, int delta, custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &N, int offset)
{
	int i, imax, ys, ys1, ys2, val, NN;

	ys = 0;
	for (i=0; i<delta; i++) ys += GRStr[i];

	ys1 = ys2 = ys;
	NN = 0;
	memset(&smax[0], 0, STR_SIZE*sizeof(int));
	memset(&smaxi[0], 0, STR_SIZE*sizeof(int));

	i = 1;
	imax = (256+offset)-delta;
	val = -1;
	while( i <= imax )
	{
		ys = ys-GRStr[i-1]+GRStr[i+delta-1];

		if (i == imax)
		{
			if (ys > ys2) 
			{
				smax[NN] = ys;
				smaxi[NN] = i;

				NN++;
			}
			else if (ys == ys2)
			{
				if (ys2 > ys1)
				{
					smax[NN] = ys;

					if (val == -1) 
					{
						smaxi[NN] = i;
					}
					else
					{
						smaxi[NN] = (val+i)/2;
					}

					NN++;
				}
			}
			else if ((ys < ys2) && (ys1 <= ys2)) 
			{
				smax[NN] = ys2;
				
				if (val == -1) 
				{
					smaxi[NN] = i-1;
				}
				else
				{
					smaxi[NN] = (val+i-1)/2;
				}

				val = -1;
				NN++;
			}

			break;
		}

		if (ys == ys2)
		{
			if (val == -1)
			{
				val = i;
			}
			i++;
			continue;
		}
		else if ((ys < ys2) && (ys1 <= ys2)) 
		{
			smax[NN] = ys2;
			
			if (val == -1) 
			{
				smaxi[NN] = i-1;
			}
			else
			{
				smaxi[NN] = (val+i-1)/2;
			}

			val = -1;
			NN++;
		}
		else
		{
			val = -1;
		}

		ys1 = ys2;
		ys2 = ys;
		i++;
	}

	N = NN;
}

void FindMaxStr(custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &max_i, int &max_val, int N)
{
	int i, j, ys;

	ys = smax[0];
	j = smaxi[0];
	for(i=0; i<N; i++)
	{
		if (smax[i] > ys) 
		{
			ys = smax[i];
			j = smaxi[i];
		}
	}

	max_i = j;
	max_val = ys;
}

void ResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h)
{
	custom_buffer<int> ImRES2(w*h*16, 0);
	int i, j, x, y;
	int r0, g0, b0, r1, g1, b1;
	u8 *color, *clr;
	int clr_res;

	clr_res = 0;
	clr = (u8*)(&clr_res);

	for(y=0, i=0, j=0; y<h; y++)
	{
		color = (u8*)(&Im[i]);
		r0 = color[2];
		g0 = color[1];
		b0 = color[0];	

		for(x=0; x<w; x++, i++, j+=4)
		{
			if (x == w-1)
			{
				ImRES2[j]   = Im[i];
				ImRES2[j+1] = Im[i];
				ImRES2[j+2] = Im[i];
				ImRES2[j+3] = Im[i];
			}
			else
			{
				color = (u8*)(&Im[i+1]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	

				ImRES2[j] = Im[i];
				
				clr[2] = (r0*3+r1)/4;
				clr[1] = (g0*3+g1)/4;
				clr[0] = (b0*3+b1)/4;
				
				ImRES2[j+1] = clr_res;

				clr[2] = (r0+r1)/2;
				clr[1] = (g0+g1)/2;
				clr[0] = (b0+b1)/2;
				
				ImRES2[j+2] = clr_res;

				clr[2] = (r0+r1*3)/4;
				clr[1] = (g0+g1*3)/4;
				clr[0] = (b0+b1*3)/4;
				
				ImRES2[j+3] = clr_res;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}
		}
	}

	for(x=0; x<4*w; x++)
	{
		i = y*4*w+x;
		color = (u8*)(&ImRES2[i]);
		r0 = color[2];
		g0 = color[1];
		b0 = color[0];	

		for(y=0; y<h; y++)		
		{
			i = y*4*w+x;
			j = 4*y*4*w+x;

			if (y == h-1)
			{
				ImRES[j]   = ImRES2[i];
				ImRES[j+1*4*w] = ImRES2[i];
				ImRES[j+2*4*w] = ImRES2[i];
				ImRES[j+3*4*w] = ImRES2[i];
			}
			else
			{
				color = (u8*)(&ImRES2[i+4*w]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	

				ImRES[j] = ImRES2[i];
				
				clr[2] = (r0*3+r1)/4;
				clr[1] = (g0*3+g1)/4;
				clr[0] = (b0*3+b1)/4;
				
				ImRES[j+1*4*w] = clr_res;

				clr[2] = (r0+r1)/2;
				clr[1] = (g0+g1)/2;
				clr[0] = (b0+b1)/2;
				
				ImRES[j+2*4*w] = clr_res;

				clr[2] = (r0+r1*3)/4;
				clr[1] = (g0+g1*3)/4;
				clr[0] = (b0+b1*3)/4;
				
				ImRES[j+3*4*w] = clr_res;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}
		}
	}
}

void SimpleResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h)
{
	int i, j, x, y, xx, yy, clr;

	for (y=0, i=0; y<h; y++)
	for (x=0; x<w; x++, i++)
	{
		clr = Im[i];
		
		for (yy=4*y; yy<4*(y+1); yy++)
		for (xx = 4*x; xx<4*(x+1); xx++)
		{
			j = yy*4*w+xx;
			ImRES[j] = Im[i];
		}
	}
}

void ResizeGrayscaleImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h)
{
	custom_buffer<int> ImRES2(w*h*16, 0);
	int i, j, x, y;
	int val0, val1;

	for(y=0, i=0, j=0; y<h; y++)
	{
		val0 = Im[i];

		for(x=0; x<w; x++, i++, j+=4)
		{
			if (x == w-1)
			{
				ImRES2[j]   = val0;
				ImRES2[j+1] = val0;
				ImRES2[j+2] = val0;
				ImRES2[j+3] = val0;
			}
			else
			{
				val1 = Im[i+1];

				ImRES2[j] = val0;
				
				ImRES2[j+1] = (val0*3+val1)/4;
				
				ImRES2[j+2] = (val0+val1)/2;
				
				ImRES2[j+3] = (val0+val1*3)/4;

				val0 = val1;
			}
		}
	}

	for(x=0; x<4*w; x++)
	{
		i = y*4*w+x;
		val0 = ImRES2[i];

		for(y=0; y<h; y++)		
		{
			i = y*4*w+x;
			j = 4*y*4*w+x;

			if (y == h-1)
			{
				ImRES[j]   = val0;
				ImRES[j+1*4*w] = val0;
				ImRES[j+2*4*w] = val0;
				ImRES[j+3*4*w] = val0;
			}
			else
			{
				val1 = ImRES2[i+4*w];

				ImRES[j] = val0;
				
				ImRES[j+1*4*w] = (val0*3+val1)/4;
				
				ImRES[j+2*4*w] = (val0+val1)/2;
				
				ImRES[j+3*4*w] = (val0+val1*3)/4;

				val0 = val1;
			}
		}
	}
}

int CompareTXTImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int w1, int h1, int w2, int h2, int YB1, int YB2)
{
	return 0;
}

void GetImageSize(string name, int &w, int &h)
{
	cv::Mat im = cv::imread(name, 1);
	w = im.cols;
	h = im.rows;
}

// Im (ImRGB) in format b:g:r:0
void LoadRGBImage(custom_buffer<int> &Im, string name, int &w, int &h)
{
	cv::Mat im = cv::imread(name, cv::IMREAD_COLOR); // load in BGR format
	u8 *color;
	w = im.cols;
	h = im.rows;

	for (int i = 0; i < w*h; i++)
	{		
		color = (u8*)(&Im[i]);
		color[0] = im.data[i * 3];
		color[1] = im.data[i * 3 + 1];
		color[2] = im.data[i * 3 + 2];
		color[3] = 0;
	}
}

void SaveRGBImage(custom_buffer<int> &Im, string name, int w, int h)
{
	cv::Mat im(h, w, CV_8UC4);

	memcpy(im.data, &Im[0], w*h * 4);

	cv::cvtColor(im, im, cv::COLOR_BGRA2BGR);

	vector<int> compression_params;

	if (g_im_save_format == ".jpeg")
	{
		compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
		compression_params.push_back(100);
	}
	else if (g_im_save_format == ".bmp")
	{
		compression_params.push_back(CV_IMWRITE_PAM_FORMAT_RGB);
	}

	try {
		cv::imwrite(g_work_dir + name, im, compression_params);
	}
	catch (runtime_error& ex) {
		char msg[500];
		sprintf(msg, "Exception saving image to %s format: %s\n", g_im_save_format.c_str(), ex.what());
		wxMessageBox(msg, "ERROR: SaveRGBImage");
	}
}

void SaveGreyscaleImage(custom_buffer<int> &Im, string name, int w, int h, int add, double scale, int quality, int dpi)
{
	cv::Mat im(h, w, CV_8UC3);
	u8 *color;

	if ((add == 0) && (scale == 1.0))
	{
		for (int i = 0; i < w*h; i++)
		{
			color = (u8*)(&Im[i]);
			im.data[i * 3] = color[0];
			im.data[i * 3 + 1] = color[0];
			im.data[i * 3 + 2] = color[0];
		}
	}
	else
	{
		for (int i = 0; i < w*h; i++)
		{
			color = (u8*)(&Im[i]);
			int val = (double)(color[0] + add)*scale;
			im.data[i * 3] = val;
			im.data[i * 3 + 1] = val;
			im.data[i * 3 + 2] = val;
		}
	}

	vector<int> compression_params;

	if (g_im_save_format == ".jpeg")
	{
		compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
		compression_params.push_back(100);
	}
	else if (g_im_save_format == ".bmp")
	{
		compression_params.push_back(CV_IMWRITE_PAM_FORMAT_RGB);
	}

	try {
		cv::imwrite(g_work_dir + name, im, compression_params);
	}
	catch (runtime_error& ex) {
		char msg[500];
		sprintf(msg, "Exception saving image to %s format: %s\n", g_im_save_format.c_str(), ex.what());
		wxMessageBox(msg, "ERROR: SaveRGBImage");
	}
}

void LoadGreyscaleImage(custom_buffer<int> &Im, string name, int &w, int &h)
{
	cv::Mat im = cv::imread(name, 1);
	w = im.cols;
	h = im.rows;

	for (int i = 0; i < w*h; i++)
	{
		if (im.data[i * 4] != 0)
		{
			Im[i] = 255;
		}
		else
		{ 
			Im[i] = 0; 
		}
	}	
}

void GreyscaleImageToMat(custom_buffer<int> &ImGR, int &w, int &h, cv::Mat &res)
{
	res = cv::Mat(h, w, CV_8UC1);

	for (int i = 0; i < w*h; i++)
	{
		res.data[i] = ImGR[i];
	}
}

void GreyscaleMatToImage(cv::Mat &ImGR, int &w, int &h, custom_buffer<int> &res)
{
	for (int i = 0; i < w*h; i++)
	{
		res[i] = ImGR.data[i];
	}
}

// ImBinary - 0 or some_color!=0 (like 0 and 1) 
void BinaryImageToMat(custom_buffer<int> &ImBinary, int &w, int &h, cv::Mat &res)
{
	res = cv::Mat(h, w, CV_8UC1);

	for (int i = 0; i < w*h; i++)
	{
		if (ImBinary[i] != 0)
		{
			res.data[i] = 255;
		}
		else
		{
			res.data[i] = 0;
		}
	}
}

// ImBinary - 0 or some_color!=0 (like 0 and 1 or 0 and 255) 
void BinaryMatToImage(cv::Mat &ImBinary, int &w, int &h, custom_buffer<int> &res, int white)
{
	for (int i = 0; i < w*h; i++)
	{
		if (ImBinary.data[i] != 0)
		{
			res[i] = white;
		}
		else
		{
			res[i] = 0;
		}
	}
}

inline wxString get_add_info()
{
	wxString msg;
	if (g_pV != NULL)
	{
		msg += "\nMovieName: " + g_pV->m_MovieName;
		msg += "\nCurPos: " + VideoTimeToStr(g_pV->GetPos());
		msg += "\nDuration: " + VideoTimeToStr(g_pV->m_Duration);
		msg += "\nWidth: " + wxString::Format(wxT("%i"), (int)g_pV->m_Width);
		msg += "\nHeight: " + wxString::Format(wxT("%i"), (int)g_pV->m_Height);
	}

	return msg;
}
