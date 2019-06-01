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
#include "cuda_kernels.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <ppl.h>
#include <ppltasks.h>
using namespace std;

void MergeImagesByIntersectedFigures(custom_buffer<int> &ImInOut, custom_buffer<int> &ImIn2, int w, int h, int white);
void MergeWithClusterImage(custom_buffer<int> &ImInOut, custom_buffer<int> &ImCluser, int w, int h, int white);
void(*g_pViewRGBImage)(custom_buffer<int> &Im, int w, int h);
void(*g_pViewImage[2])(custom_buffer<int> &Im, int w, int h);
void GreyscaleImageToMat(custom_buffer<int> &ImGR, int w, int h, cv::Mat &res);
void GreyscaleMatToImage(cv::Mat &ImGR, int w, int h, custom_buffer<int> &res);
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
int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H);
int ThirdFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H);
int GetImageWithInsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white, int &val1, int &val2, int LH = 0, int LMAXY = 0, int real_im_x_center = 0);

string  g_work_dir;
string  g_app_dir;

string  g_im_save_format = ".jpeg";
//string  g_im_save_format = ".bmp";

double	g_mthr = 0.4;
double	g_mnthr = 0.3;
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

int g_min_alpha_color = 1;

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

int g_scale = 4;

#define STR_SIZE (256 * 2) // 

#define MAX_EDGE_STR 786432 //на сам деле ~ 32*3*255

//int g_LN;

//int g_blnVNE = 0;
//int g_blnHE = 0;

bool g_clear_image_logical = false;

bool g_generate_cleared_text_images_on_test = false;
bool g_show_results = false;
bool g_show_sf_results = false;
bool g_clear_test_images_folder = true;
bool g_show_transformed_images_only = false;

bool g_wxImageHandlersInitialized = false;

bool g_use_cuda_gpu = true;

int g_cuda_kmeans_loop_iterations = 20;

bool InitCUDADevice()
{
	bool res = true;
	int num = GetCUDADeviceCount();
	if (num == 0)
	{
		res = false;
	}
	return res;
}

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

void FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)
{	
	custom_assert(ImIn.size() >= w * h, "FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImHOE.size() >= w * h, "FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h)\nnot: ImHOE.size() >= w*h");

	const int mx = w - 1;
	const int my = h - 1;

	concurrency::parallel_for(1, my, [&](int y)
	{
		int x, val, val1, val2;
		int* pIm, *pImHOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImHOE = &ImHOE[y*w + x]; x < mx; x++, pIm++, pImHOE++)
		{
			val1 = *(pIm - w - 1) + *(pIm - w + 1);
			val2 = *(pIm - w);

			val1 -= *(pIm + w - 1) + *(pIm + w + 1);
			val2 -= *(pIm + w);

			val = 3 * val1 + 10 * val2;

			if (val < 0) val = -val;
			*pImHOE = val;
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

void GetImHE(custom_buffer<int> &ImHE, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)
{
	custom_buffer<int> ImYMOE(w*h, 0), ImUMOE(w*h, 0), ImVMOE(w*h, 0);
	custom_buffer<int> ImRES1(w*h, 0);
	int k, cnt, val, N, mx, my;
	int segh;
	int ww, hh;
	__int64 t1, dt, num_calls;

	EasyBorderClear(ImHE, w, h);

	concurrency::parallel_invoke(
		[&] { FastImprovedSobelHEdge(ImY, ImYMOE, w, h); },
		[&] { FastImprovedSobelHEdge(ImU, ImUMOE, w, h); },
		[&] { FastImprovedSobelHEdge(ImV, ImVMOE, w, h); }
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
				ImHE[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
			}
		}
		ApplyModerateThreshold(ImHE, g_mnthr, w, h);
	}
	);

	CombineTwoImages(ImHE, ImRES1, w, h);
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int GetTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, int w, int h, int W, int H)
{
	custom_buffer<int> LB(h, 0), LE(h, 0), ImY(w*h, 0), ImU(w*h, 0), ImV(w*h, 0), ImHE(w*h, 0);
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

	/*cv::Mat cv_ImRGB(h, w, CV_8UC4), cv_ImLAB, cv_ImLABSplit[3];
	memcpy(cv_ImRGB.data, &ImRGB[0], w*h * sizeof(int));

	cv::cvtColor(cv_ImRGB, cv_ImLAB, cv::COLOR_BGR2Lab);
	cv::split(cv_ImLAB, cv_ImLABSplit);

	GreyscaleMatToImage(cv_ImLABSplit[0], w, h, ImY);
	GreyscaleMatToImage(cv_ImLABSplit[1], w, h, ImU);
	GreyscaleMatToImage(cv_ImLABSplit[2], w, h, ImV);*/
	
	concurrency::parallel_invoke(
		[&] { GetImFF(ImFF, ImSF, ImRGB, ImY, ImU, ImV, LB, LE, N, w, h); },
		[&] { GetImNE(ImNE, ImY, ImU, ImV, w, h); },
		[&] { GetImHE(ImHE, ImY, ImU, ImV, w, h); }
	);	

	if (g_show_results) SaveGreyscaleImage(ImFF, "\\TestImages\\GetTransformedImage_02_1_ImFF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "\\TestImages\\GetTransformedImage_02_2_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "\\TestImages\\GetTransformedImage_02_3_ImNE" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImHE, "\\TestImages\\GetTransformedImage_02_4_ImHE" + g_im_save_format, w, h);
	
	CombineTwoImages(ImNE, ImHE, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "\\TestImages\\GetTransformedImage_02_5_ImNE+HE" + g_im_save_format, w, h);

	res = SecondFiltration(ImSF, ImNE, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImSF, "\\TestImages\\GetTransformedImage_05_ImSFFSecondFiltration" + g_im_save_format, w, h);

	memcpy(&ImTF[0], &ImSF[0], w*h*sizeof(int));
	if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);	
	memcpy(&ImRES1[0], &ImTF[0], w*h * sizeof(int));
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_06_ImTFFThirdFiltration" + g_im_save_format, w, h);

	if (res == 1) res = ClearImageFromSmallSymbols(ImTF, w, h, 255);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_07_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);

	if (res == 1) res = SecondFiltration(ImTF, ImNE, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_08_ImTFFSecondFiltration" + g_im_save_format, w, h);

	if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_09_ImTFFThirdFiltration" + g_im_save_format, w, h);		

	if (res == 1) RestoreStillExistLines(ImTF, ImRES1, w, h);	
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_10_ImTFWithRestoredStillExistLines" + g_im_save_format, w, h);

	if (res == 1) ExtendImFWithDataFromImNF(ImTF, ImFF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImTF, "\\TestImages\\GetTransformedImage_12_ImTFExtByImFF" + g_im_save_format, w, h);

	return res;
}

int FilterImage(custom_buffer<int> &ImF, custom_buffer<int> &ImNE, int w, int h, int W, int H, custom_buffer<int> &LB, custom_buffer<int> &LE, int N)
{
	int res;
	custom_buffer<int> ImRES1(w*h, 0), ImRES2(w*h, 0);
	int diff_found = 1;
	int iter = 0;

	memcpy(&ImRES1[0], &ImF[0], w*h * sizeof(int));

	if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FilterImage_01_ImFOrigin" + g_im_save_format, w, h);

	while (diff_found)
	{
		iter++;
		memcpy(&ImRES2[0], &ImF[0], w*h * sizeof(int));		

		res = SecondFiltration(ImF, ImNE, LB, LE, N, w, h, W, H);
		if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FilterImage_iter" + std::to_string(iter) + "_02_ImSFFSecondFiltration" + g_im_save_format, w, h);

		if (res == 1) res = ThirdFiltration(ImF, LB, LE, N, w, h, W, H);
		if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FilterImage_iter" + std::to_string(iter) + "_03_ImTFFThirdFiltration" + g_im_save_format, w, h);

		if (res == 1) res = ClearImageFromSmallSymbols(ImF, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FilterImage_iter" + std::to_string(iter) + "_04_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);
		
		diff_found = 0;
		for (int i = 0; i < w*h; i++)
		{
			if (ImRES2[i] != ImF[i])
			{
				diff_found = 1;
				break;
			}
		}
	}

	return res;
}

inline bool IsTooRight(int &lb, int &le, const int &dw2, int &w)
{
	return (((lb + le - w) >= dw2 / 2) || (lb >= w / 2));
	//return ((lb + le) > w);
}

///////////////////////////////////////////////////////////////////////////////
// W - full image include scale (if is) width
// H - full image include scale (if is) height
int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H)
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

				// for debug
				if (g_show_sf_results)
				{
					y = (ia / w);
					if ((y == 630) &&
						(iter == 1))
					{
						y = y;
					}
				}

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

						if (IsTooRight(lb[l + 1], le[l + 1], dw2, w) ||
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

						if (IsTooRight(lb[ln - 1], le[ln - 1], dw2, w) ||
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

						if (IsTooRight(lb[ln - 1], le[ln - 1], dw2, w) ||
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

inline void GetDDY(int &h, int &LH, int &LMAXY, int &ddy1, int &ddy2)
{
	ddy1 = std::max<int>(4, LMAXY - ((7 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + ((2 * LH) / 5));

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(1, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h - 2, LMAXY + 3);
	}
}

void cuda_kmeans(custom_buffer<int> &ImRGB, custom_buffer<int> &labels, int w, int h, int numClusters, int loop_iterations, float threshold = 0.001)
{
	int numObjs=w*h;
	float **clusters;
	
	clusters = cuda_kmeans_img((unsigned char*)(ImRGB.m_pData), numObjs, numClusters, threshold, labels.m_pData, loop_iterations);
	free(clusters[0]);
	free(clusters);
}

void opencv_kmeans(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &labels, int w, int h, int numClusters, int loop_iterations)
{
	int i, x, y;
	cv::Mat samples(w * h, 3, CV_32F);
	cv::Mat cv_labels(w * h, 1, CV_32S);	
	cv::Mat centers;
	int color, rc, gc;
	u8 *pClr;
	pClr = (u8*)(&color);

	for (i = 0; i < w*h; i++)
	{
		pClr = (u8*)(&ImRGB[i]);

		for (int z = 0; z < 3; z++)
		{
			samples.at<float>(i, z) = pClr[z];
		}

		if (ImFF[i] == 0)
		{
			cv_labels.at<int>(i, 0) = 0;
		}
		else
		{
			cv_labels.at<int>(i, 0) = 1;
		}
	}

	cv::theRNG().state = 0;
	cv::kmeans(samples, numClusters, cv_labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER + cv::TermCriteria::EPS, 10, 1.0), loop_iterations, cv::KMEANS_USE_INITIAL_LABELS, centers);
	memcpy(&labels[0], cv_labels.data, w*h * sizeof(int));
}

void GetMainClusterImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImRES, custom_buffer<int> &ImMaskWithBorder, int w, int h, int LH, int LMAXY, std::string iter_det, int white, bool clear_from_left_and_right_borders = false, bool check_on_characters_edging = false, int real_im_x_center = 0)
{
	custom_buffer<int> ImRES1(w*h, 0), ImRES4(w*h, 0), labels1(w*h, 0), labels2(w*h, 0);
	int x, y, i, j, ddy1, ddy2, val1, val2, clusterCount;
	int color, rc, gc;
	u8 *pClr;
	custom_buffer<int> cluster_cnt;
	DWORD  start_time;

	start_time = GetTickCount();

	pClr = (u8*)(&color);

	color = 0;
	pClr[2] = 255;
	rc = color;

	color = 0;
	pClr[1] = 255;
	gc = color;

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\GetMainClusterImage_" + iter_det + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImFF, "\\TestImages\\GetMainClusterImage_" + iter_det + "_01_2_ImFF" + g_im_save_format, w, h);

	memset(&ImRES[0], 0, (w*h) * sizeof(int));

	GetDDY(h, LH, LMAXY, ddy1, ddy2);

	concurrency::parallel_invoke(
		[&ImRGB, &ImFF, &labels1, w, h] {
			int clusterCount = 4;
			if (g_use_cuda_gpu)
			{
				cuda_kmeans(ImRGB, labels1, w, h, clusterCount, g_cuda_kmeans_loop_iterations);
			}
			else
			{
				opencv_kmeans(ImRGB, ImFF, labels1, w, h, clusterCount, 10);
			}
		},	
		[&ImRGB, &ImFF, &labels2, w, h] {
			int clusterCount = 6;
			if (g_use_cuda_gpu)
			{
				cuda_kmeans(ImRGB, labels2, w, h, clusterCount, g_cuda_kmeans_loop_iterations);
			}
			else
			{
				opencv_kmeans(ImRGB, ImFF, labels2, w, h, clusterCount, 10);
			}
		}
	);

	//wxMessageBox("GetMainClusterImage_dt1_cc4_cc6: " + std::to_string(GetTickCount() - start_time));
	start_time = GetTickCount();

	clusterCount = 4;

	if (g_show_results)
	{
		for (i = 0; i < w*h; i++)
		{
			int cluster_idx = labels1[i];
			ImRES[i] = (255 / clusterCount)*(cluster_idx + 1);
		}
		SaveGreyscaleImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_02_ImRES4Clusters" + g_im_save_format, w, h);
	}

	cluster_cnt = custom_buffer<int>(clusterCount, 0);

	// searching main TXT cluster which intersect with ImFF (ImFF) for save it in ImRES
	for (i = 0; i < w*h; i++)
	{
		if (ImFF[i] != 0)
		{
			int cluster_idx = labels1[i];
			cluster_cnt[cluster_idx]++;
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

	// ImRES - MainTXTCluster
	for (i = 0; i < w*h; i++)
	{
		int cluster_idx = labels1[i];

		if (cluster_idx == j)
		{
			ImRES[i] = white;
		}
		else
		{
			ImRES[i] = 0;
		}
	}

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_01_ImRES_MainTXTClusterIn4Clasters" + g_im_save_format, w, h);

	if (check_on_characters_edging)
	{
		custom_buffer<CMyClosedFigure> pFigures1, pFigures2;
		custom_buffer<int> x_range1(w, 0), x_range2(w, 0);
		int l, ii, min_x1, max_x1, min_x2, max_x2, ww1, ww2;

		memcpy(&ImRES1[0], &ImRES[0], w*h * sizeof(int));

		ClearImageFromBorders(ImRES1, w, h, ddy1, ddy2, white);
		if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_02_ImRES1_MainTXTClusterIn4ClastersWithClearImageFromBorders" + g_im_save_format, w, h);

		GetImageWithInsideFigures(ImRES1, ImRES4, w, h, white, val1, val2, LH, LMAXY, real_im_x_center);
		if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_03_ImRES4_MainTXTClusterIn4ClastersInsideFigures" + g_im_save_format, w, h);

		auto add_info_about_figure = [&real_im_x_center, &LMAXY, &LH, &white](custom_buffer<int> &ImRES, CMyClosedFigure *pFigure, custom_buffer<int> &x_range, int &min_x, int &max_x) {
			if ((pFigure->m_minX < real_im_x_center) &&
				((pFigure->m_minY + pFigure->m_maxY) / 2 <= LMAXY - (LH / 10) + 1) &&
				((pFigure->m_minY + pFigure->m_maxY) / 2 > LMAXY - LH + 1) &&
				!((pFigure->m_Square >= 0.95*((M_PI*pFigure->m_w*pFigure->m_h) / 4.0)) && //is not like ellipse or rectangle on 95% 
				(pFigure->m_h <= 2 * pFigure->m_w))
				)
			{
				for (int x = pFigure->m_minX; x <= pFigure->m_maxX; x++) x_range[x] = 1;
				if (pFigure->m_minX < min_x) min_x = pFigure->m_minX;
				if (pFigure->m_maxX > max_x) max_x = pFigure->m_maxX;

				if (g_show_results)
				{
					CMyPoint *PA = pFigure->m_PointsArray;
					for (int l = 0; l < pFigure->m_Square; l++)
					{
						int ii = PA[l].m_i;
						ImRES[ii] = white;
					}
				}
			}
		};

		concurrency::parallel_invoke(
			[&] {
				ClearImageOpt5(ImRES1, w, h, LH, LMAXY, white);
				if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_04_ImRES1_MainTXTClusterIn4ClastersClearImageOpt5" + g_im_save_format, w, h);
				SearchClosedFigures(ImRES1, w, h, white, pFigures1);

				if (g_show_results)
				{
					for (int x_ = 0; x_ < w; x_++)
					{
						ImRES1[LMAXY*w + x_] = rc;
						ImRES1[(LMAXY - (LH / 10) + 1)*w + x_] = rc;
						ImRES1[(LMAXY - LH + 1)*w + x_] = rc;
					}

					if (real_im_x_center < w)
					{
						for (int y_ = 0; y_ < h; y_++)
						{
							ImRES1[y_*w + real_im_x_center] = rc;
						}
					}

					if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_06_ImRES1_MainTXTClusterIn4ClastersClearImageOpt5WithLinesInfo" + g_im_save_format, w, h);

					memset(&ImRES1[0], 0, w*h * sizeof(int));
				}

				min_x1 = w - 1;
				max_x1 = 0;
				for (int k = 0; k < pFigures1.size(); k++)
				{
					add_info_about_figure(ImRES1, &(pFigures1[k]), x_range1, min_x1, max_x1);
				}
				if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_08_ImRES1_MainTXTClusterIn4ClastersF" + g_im_save_format, w, h);

				ww1 = 0;
				for (int x_ = 0; x_ < w; x_++)
				{
					if (x_range1[x_] == 1) ww1++;
				}
			},
			[&] {
				ClearImageOpt5(ImRES4, w, h, LH, LMAXY, white);
				if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_05_ImRES4_MainTXTClusterIn4ClastersInsideFiguresClearImageOpt5" + g_im_save_format, w, h);
				SearchClosedFigures(ImRES4, w, h, white, pFigures2);

				if (g_show_results)
				{
					for (int x_ = 0; x_ < w; x_++)
					{
						ImRES4[LMAXY*w + x_] = rc;
						ImRES4[(LMAXY - (LH / 10) + 1)*w + x_] = rc;
						ImRES4[(LMAXY - LH + 1)*w + x_] = rc;
					}

					if (real_im_x_center < w)
					{
						for (int y_ = 0; y_ < h; y_++)
						{
							ImRES4[y_*w + real_im_x_center] = rc;
						}
					}

					if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_07_ImRES4_MainTXTClusterIn4ClastersInsideFiguresClearImageOpt5WithLinesInfo" + g_im_save_format, w, h);

					memset(&ImRES4[0], 0, w*h * sizeof(int));
				}

				min_x2 = w - 1;
				max_x2 = 0;
				for (int k = 0; k < pFigures2.size(); k++)
				{
					add_info_about_figure(ImRES4, &(pFigures2[k]), x_range2, min_x2, max_x2);
				}
				if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_09_ImRES4_MainTXTClusterIn4ClastersInsideFiguresF" + g_im_save_format, w, h);

				ww2 = 0;
				for (int x_ = 0; x_ < w; x_++)
				{
					if (x_range2[x_] == 1) ww2++;
				}
			}
		);

		// checking on wrong detected main claster (characters edging instead of characters themselves)
		if ( (ww2 >=(2*ww1)/3) &&
			 ((max_x2 - min_x2 + 1) >= 0.8 * (max_x1 - min_x1 + 1) )
			)
		{
			//replacing MainTXTClusterIn4Clasters by claster wich more intersect with MainTXTClusterIn4ClastersInsideFigures

			cluster_cnt = custom_buffer<int>(clusterCount, 0);

			// searching main TXT cluster which intersect with MainTXTClusterIn4ClastersInsideFigures for save it in ImRES
			for (i = 0; i < w*h; i++)
			{
				if (ImRES4[i] != 0)
				{
					int cluster_idx = labels1[i];
					cluster_cnt[cluster_idx]++;
				}
			}

			j = 0;
			val2 = cluster_cnt[j];
			for (i = 0; i < clusterCount; i++)
			{
				if (cluster_cnt[i] > val2)
				{
					j = i;
					val2 = cluster_cnt[j];
				}
			}

			val2 = 0;
			for (i = 0; i < w*h; i++)
			{
				int cluster_idx = labels1[i];
				if (cluster_idx == j)
				{
					ImRES1[i] = white;
					if (ImRES4[i] != 0)
					{
						val2++;
					}
				}
				else
				{
					ImRES1[i] = 0;
				}
			}

			memcpy(&ImRES[0], &ImRES1[0], w*h * sizeof(int));
			if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_05_10_ImRES_MainTXTClusterIn4ClastersReplaced" + g_im_save_format, w, h);
		}
	}

	//wxMessageBox("GetMainClusterImage_dt2: " + std::to_string(GetTickCount() - start_time));
	start_time = GetTickCount();	

	clusterCount = 6;

	for (i = 0; i < w*h; i++)
	{
		int cluster_idx = labels2[i];
		ImRES4[i] = (255 / clusterCount)*(cluster_idx + 1);
	}

	if (g_show_results) SaveGreyscaleImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_06_ImRES4_6Clusters" + g_im_save_format, w, h);

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_07_1_ImRES_MainTXTClusterIn4Clasters" + g_im_save_format, w, h);

	concurrency::parallel_for(0, clusterCount, [&ImRES, &ImMaskWithBorder, &ImRES4, clusterCount, w, h, ddy1, ddy2, clear_from_left_and_right_borders](int cluster_idx)
	{
		custom_buffer<CMyClosedFigure> pFigures;
		SearchClosedFigures(ImRES4, w, h, (255 / clusterCount)*(cluster_idx + 1), pFigures);
		int N = pFigures.size(), l, ii;
		CMyClosedFigure *pFigure;
		CMyPoint *PA;

		for (int i = 0; i < N; i++)
		{
			pFigure = &(pFigures[i]);

			if ( (pFigure->m_minY < ddy1) || 
				 (pFigure->m_maxY > ddy2) ||
				 ( clear_from_left_and_right_borders &&
				  ( (pFigure->m_minX <= 4) || (pFigure->m_maxX >= (w - 1) - 4) ) )
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
	});

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_07_2_ImRES_MainTXTClusterIn4ClastersFilteredBy6ClutersFromBorders" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\GetMainClusterImage_" + iter_det + "_07_3_ImMaskWithBorder" + g_im_save_format, w, h);

	if (g_show_results) SaveGreyscaleImage(ImFF, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_1_ImRES1_ImFF" + g_im_save_format, w, h);
	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_2_ImRES_ImMainTXTClusterIn4Clasters" + g_im_save_format, w, h);
	memcpy(&ImRES1[0], &ImFF[0], (w*h) * sizeof(int));
	IntersectTwoImages(ImRES1, ImRES, w, h);
	if (g_show_results) SaveGreyscaleImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_3_ImRES1_ImFFIntImMainTXTClusterIn4Clasters" + g_im_save_format, w, h);

	// bad in case of split and only one simbol like '.' on splited image
	/*{
		custom_buffer<CMyClosedFigure> pFigures;
		CMyClosedFigure *pFigure;
		CMyPoint *PA;
		int l, ii, min_x1, max_x1, min_x2, max_x2, ww1, ww2;

		SearchClosedFigures(ImRES1, w, h, 255, pFigures);

		min_x1 = w - 1;
		max_x1 = 0;
		for (i = 0; i < pFigures.size(); i++)
		{
			pFigure = &(pFigures[i]);
			PA = pFigure->m_PointsArray;

			if ( ( (pFigure->m_Square >= 0.95*((M_PI*pFigure->m_w*pFigure->m_h) / 4.0)) && //is not like ellipse or rectangle on 95% 
				   (pFigure->m_h <= 2*pFigure->m_w) ) ||
				 ((pFigure->m_h <= 4*g_scale) && (pFigure->m_w <= 4 * g_scale)) )
			{
				for (int l = 0; l < pFigure->m_Square; l++)
				{
					int ii = PA[l].m_i;
					ImRES1[ii] = 0;
				}
			}
		}

		if (g_show_results) SaveGreyscaleImage(ImRES1, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_4_ImRES1_ImFFIntImMainTXTClusterIn4ClastersF" + g_im_save_format, w, h);
	}*/

	if (g_show_results) SaveGreyscaleImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_5_ImRES4_6Clusters" + g_im_save_format, w, h);

	cluster_cnt = custom_buffer<int>(clusterCount, 0);
	custom_buffer<int> cluster_id(clusterCount, 0);

	// searching main TXT clusters which intersect with ImRES1 (ImFFIntImMaskWithBorder)
	for (i = 0; i < w*h; i++)
	{
		if (ImRES1[i] != 0)
		{
			int cluster_idx = labels2[i];
			cluster_cnt[cluster_idx]++;
		}
	}

	for (i = 0; i < clusterCount; i++)
	{
		cluster_id[i] = i;
	}

	for (i = 0; i < clusterCount-1; i++)
	{
		for (j = i + 1; j < clusterCount; j++)
		{
			if (cluster_cnt[j] > cluster_cnt[i])
			{
				val1 = cluster_cnt[i];
				cluster_cnt[i] = cluster_cnt[j];
				cluster_cnt[j] = val1;

				val1 = cluster_id[i];
				cluster_id[i] = cluster_id[j];
				cluster_id[j] = val1;
			}
		}
	}

	int num_main_clasters = 2;

	// ImRES4 - MainTXTClusters with clusterCount == 6
	for (i = 0; i < w*h; i++)
	{
		int cluster_idx = labels2[i];

		bool is_main = false;
		for (j = 0; j < num_main_clasters; j++)
		{
			if (cluster_id[j] == cluster_idx)
			{
				is_main = true;
				break;
			}
		}

		if (is_main)
		{
			ImRES4[i] = 255;
		}
		else
		{
			ImRES4[i] = 0;
		}
	}

	if (g_show_results) SaveGreyscaleImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_6_ImRES4_MainTXTClustersIn6Clasters" + g_im_save_format, w, h);
	IntersectTwoImages(ImRES4, ImMaskWithBorder, w, h);
	if (g_show_results) SaveGreyscaleImage(ImRES4, "\\TestImages\\GetMainClusterImage_" + iter_det + "_08_7_ImRES4_MainTXTClustersIn6ClastersIntImMaskWithBorder" + g_im_save_format, w, h);	

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_09_1_ImRES_MainTXTClusterIn4Clasters" + g_im_save_format, w, h);
	

	if (w <= 3 * h)
	{
		IntersectTwoImages(ImRES, ImRES4, w, h);
		if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_09_2_ImRES3_MainTXTClusterIn4ClastersIntMainTXTClusterIn6Clasters" + g_im_save_format, w, h);
	}
	else
	{
		custom_buffer<CMyClosedFigure> pFigures;
		SearchClosedFigures(ImRES, w, h, white, pFigures);
		int N = pFigures.size();

		concurrency::parallel_for(0, N, [&ImRES, &ImRES4, &pFigures, w, h](int i)
		{
			CMyClosedFigure *pFigure;
			CMyPoint *PA;
			int l, ii, x, y;
			pFigure = &(pFigures[i]);
			PA = pFigure->m_PointsArray;

			int cnt = 0;
			int min_x = w, max_x = 0, min_y = h, max_y = 0, cw = 0, ch = 0;
			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				y = ii / w;
				x = ii % w;

				if (ImRES4[ii] != 0)
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

			if ((cnt < 0.5*pFigure->m_Square) ||
				((pFigure->m_h < pFigure->m_w / 4) && (cw < pFigure->m_w / 2)) ||
				((pFigure->m_w < pFigure->m_h / 4) && (ch < pFigure->m_h / 2)) ||
				(((pFigure->m_h >= 4 * g_scale) || (pFigure->m_w >= 4 * g_scale)) && // figure is bigger then 4x4 in original size
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
		});

		if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImage_" + iter_det + "_09_2_ImRES3_MainTXTClusterIn4ClastersFilteredByMainTXTClusterIn6Clasters" + g_im_save_format, w, h);
	}	

	//wxMessageBox("GetMainClusterImage_dt4: " + std::to_string(GetTickCount() - start_time));
	start_time = GetTickCount();
}

void GetMainClusterImageBySplit(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImRES, custom_buffer<int> &ImMaskWithBorder, int w, int h, int LH, int LMAXY, std::string iter_det, int white)
{	
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

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_01_2_ImFF" + g_im_save_format, w, h);

	memset(&ImRES[0], 0, (w*h) * sizeof(int));

	GetDDY(h, LH, LMAXY, ddy1, ddy2);

	TN = 0;
	bln1 = 0;
	for (x = 0; x < w; x++)
	{
		bln2 = 0;
		for (y = LMAXY - LH + 1; y <= LMAXY; y++)
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
		custom_buffer<int> ImRES1(w*h, 0);
		memcpy(&ImRES1[0], &ImRGB[0], (w*h) * sizeof(int));

		for (i = 0; i < TN; i++)
		{
			for (y = 0; y < h; y++)
			{
				ImRES1[y * w + TL[i]] = rc;
				ImRES1[y * w + TR[i]] = gc;
			}
		}

		SaveRGBImage(ImRES1, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_02_ImRGBWithSplitInfo" + g_im_save_format, w, h);
	}

	concurrency::parallel_for(0, TN, [&TR, &TL, &ImRGB, &ImFF, &ImMaskWithBorder, &ImRES, &h, &w, &LH, &LMAXY, &iter_det, &white](int ti)
	{
		int y, i, j, tw = TR[ti] - TL[ti] + 1;
		custom_buffer<int> ImRES1(tw*h, 0), ImRES2(tw*h, 0), ImRES3(tw*h, 0), ImRES5(tw*h, 0);

		// ImRES1 - RGB image with size tw:h (segment of original image)
		// ImRES2 - ImFF image with size tw:h (segment of original image)
		for (y = 0, i = TL[ti], j = 0; y < h; y++, i += w, j += tw)
		{
			memcpy(&ImRES1[j], &ImRGB[i], tw * sizeof(int));
			memcpy(&ImRES2[j], &ImFF[i], tw * sizeof(int));
			memcpy(&ImRES5[j], &ImMaskWithBorder[i], tw * sizeof(int));
		}

		GetMainClusterImage(ImRES1, ImRES2, ImRES3, ImRES5, tw, h, LH, LMAXY, iter_det + "_splititer" + std::to_string(ti + 1), white);

		for (y = 0, i = TL[ti], j = 0; y < h; y++, i += w, j += tw)
		{
			memcpy(&ImRES[i], &ImRES3[j], tw * sizeof(int));
			memcpy(&ImMaskWithBorder[i], &ImRES5[j], tw * sizeof(int));
		}
	});

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_03_1_ImRES_MainCluster" + g_im_save_format, w, h);

	// can simply remove right symbols: in 0_01_03_001__0_01_03_002.jpeg for example
	/*
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

		//removing all figures which are located only on one side of edge of image split and not intersect with middles of splits
		for (int ti = 1; ti < TN-1; ti++)
		{
			if (
				( (2 * pFigure->m_minX > TL[ti] + TR[ti]) ||
				  (2 * pFigure->m_maxX < TL[ti] + TR[ti]) )
				&&
				( ((pFigure->m_maxX == TR[ti]) && (ti < TN - 1)) ||
				  ((pFigure->m_minX == TL[ti]) && (ti > 0)) )
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

	if (g_show_results) SaveRGBImage(ImRES, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_03_2_ImRES_MainClusterFilteredByFiguresOnSplitEdges" + g_im_save_format, w, h);
	*/

	if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\GetMainClusterImageBySplit" + iter_det + "_03_3_ImMaskWithBorder" + g_im_save_format, w, h);
}

class FindTextRes
{
public:
	int m_res;
	cv::Mat m_cv_ImRGB;
	int m_im_h;
	string m_ImageName;
	int m_YB;
	int m_LH;
	int m_LY;
	int m_LXB;
	int m_LXE;
	int m_LYB;
	int m_LYE;
	int m_mY;
	int m_mI;
	int m_mQ;
	custom_buffer<int> m_LL;
	custom_buffer<int> m_LR;
	custom_buffer<int> m_LLB;
	custom_buffer<int> m_LLE;
	int m_N;
	int m_k;
	string m_iter_det;

	FindTextRes()
	{
		m_res = 0;
	}
};

FindTextRes FindText(custom_buffer<int> &ImRGB, custom_buffer<int> &ImNF, custom_buffer<int> &FullImY, string SaveName, std::string iter_det, int N, int k, custom_buffer<int> LL, custom_buffer<int> LR, custom_buffer<int> LLB, custom_buffer<int> LLE, int W, int H)
{
	int i, j, l, r, x, y, ib, bln, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, ww, hh, cnt;
	int XB, XE, YB, YE, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, lb, le, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, NN, ys1, ys2, ys3, val_min, val_max;
	int j1, j2, j3, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int mY, mI, mQ;
	int LH, LMAXY;
	custom_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	FindTextRes res;		
	int color, rc, gc, bc, yc, cc, wc;
	u8 *pClr;
	DWORD start_time;

	start_time = GetTickCount();

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
	pClr[1] = 255;
	pClr[2] = 255;
	yc = color;

	color = 0;
	pClr[2] = 128;
	pClr[0] = 128;
	cc = color;

	color = 0;
	pClr[0] = 255;
	pClr[1] = 255;
	pClr[2] = 255;
	wc = color;

	int orig_LLBk = LLB[k];
	int orig_LLEk = LLE[k];
	int orig_LLk = LL[k];
	int orig_LRk = LR[k];
	int min_h;

	XB = LL[k];
	XE = LR[k];
	w = XE - XB + 1;
	val = (int)((double)w*0.15);
	if (val < 40) val = 40;
	XB -= val;
	XE += val;
	if (XB < 0) XB = 0;
	if (XE > W - 1) XE = W - 1;
	w = XE - XB + 1;

	YB = LLB[k];
	YE = LLE[k];

	// getting h of sub
	h = YE - YB + 1;
	val = (3 * ((int)(0.03*(double)H) + 1)) / 2; // 3/2 * ~ min sub height # (536-520+1)/576
	if (h < val)
	{
		if (N == 1)
		{
			LLB[k] -= (val - h) / 2;
			LLE[k] = LLB[k] + val - 1;
		}
		else if (k == 0)
		{
			LLE[k] += std::min<int>((val - h) / 2, (LLB[k + 1] - LLE[k]) / 2);
			LLB[k] = LLE[k] - val + 1;
		}
		else if (k == N - 1)
		{
			LLB[k] -= std::min<int>((val - h) / 2, (LLB[k] - LLE[k - 1]) / 2);
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
		h = YE - YB + 1;
	}

	YB -= h / 2;
	YE += h / 2;
	if (YB < 0) YB = 0;
	if (YE > H - 1) YE = H - 1;

	if (k > 0)
	{
		if (YB < LLE[k - 1] - 2) YB = std::max<int>(0, LLE[k - 1] - 2);
	}
	if (k < N - 1)
	{
		val = (LLB[k + 1] * 5 + LLE[k + 1]) / 6;
		if (YE > val) YE = val;
	}

	int max_diff = 20;
	for (y = YB; y < YE; y++)
	{
		bln = 0;
		for (x = std::max<int>(XB, W / 4), val1 = val2 = FullImY[y*W + x]; x <= std::min<int>(XE, (3 * W) / 4); x++)
		{
			i = y * W + x;
			if (FullImY[i] < val1)
			{
				val1 = FullImY[i];
			}

			if (FullImY[i] > val2)
			{
				val2 = FullImY[i];
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
		for (x = std::max<int>(XB, W / 4), val1 = val2 = FullImY[y*W + x]; x <= std::min<int>(XE, (3 * W) / 4); x++)
		{
			i = y * W + x;
			if (FullImY[i] < val1)
			{
				val1 = FullImY[i];
			}

			if (FullImY[i] > val2)
			{
				val2 = FullImY[i];
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

	if (YB >= YE)
	{
		return res;
	}

	if (YB > orig_LLBk - 2)
	{
		orig_LLBk = YB;
		YB = std::max<int>(0, YB - 2);
	}

	if (YE < orig_LLEk + 2)
	{
		orig_LLEk = YE;
		YE = std::min<int>(H - 1, YE + 2);
	}

	if (LLB[k] < YB) LLB[k] = YB;
	if (LLE[k] > YE) LLE[k] = YE;

	h = YE - YB + 1;

	custom_buffer<int> ImRES1((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES2((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES3((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES4((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImRES5((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES6((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES7((int)(w*g_scale)*(int)(h*g_scale), 0), ImRES8((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> Im((int)(w*g_scale)*(int)(h*g_scale), 0), ImSF((int)(w*g_scale)*(int)(h*g_scale), 0), ImSNF((int)(w*g_scale)*(int)(h*g_scale), 0), ImFF((int)(w*g_scale)*(int)(h*g_scale), 0), ImSFIntTHRF((int)(w*g_scale)*(int)(h*g_scale), 0), ImSNFIntTHRF((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImY((int)(w*g_scale)*(int)(h*g_scale), 0), ImU((int)(w*g_scale)*(int)(h*g_scale), 0), ImV((int)(w*g_scale)*(int)(h*g_scale), 0), ImI((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImTHR((int)(w*g_scale)*(int)(h*g_scale), 0);

	if (g_show_results)
	{
		custom_buffer<int> ImTMP(W * H, 0);
		memcpy(&ImTMP[0], &ImRGB[0], W * H * sizeof(int));

		for (i = 0; i < N; i++)
		{
			for (x = 0; x < W; x++)
			{
				ImTMP[LLB[i] * W + x] = rc;
				ImTMP[LLE[i] * W + x] = gc;
			}
		}

		for (x = 0; x < W; x++)
		{
			ImTMP[YB * W + x] = bc;
			ImTMP[YE * W + x] = bc;
		}

		SaveRGBImage(ImTMP, "\\TestImages\\FindTextLines_" + iter_det + "_03_1_ImRGB_RGBWithLinesInfo" + g_im_save_format, W, H);
		SaveRGBImage(ImTMP, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_03_1_ImRGB_RGBWithLinesInfo" + g_im_save_format, W, H);
	}

	for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
	{
		memcpy(&ImRES1[j], &ImRGB[i], w * sizeof(int));
	}

	if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_03_2_ImRES1_RGB" + g_im_save_format, w, h);

	//wxMessageBox("dt1: " + std::to_string(GetTickCount() - start_time));
	start_time = GetTickCount();

	{
		cv::Mat cv_ImRGB, cv_ImY, cv_ImLAB, cv_ImLABSplit[3], cv_ImHSV, cv_ImHSVSplit[3], cv_ImL, cv_ImA, cv_ImB, cv_bw, cv_bw_gaus;

		{
			cv::Mat cv_ImRGBOrig(h, w, CV_8UC4);
			memcpy(cv_ImRGBOrig.data, &ImRES1[0], w*h * sizeof(int));
			cv::resize(cv_ImRGBOrig, cv_ImRGB, cv::Size(0, 0), g_scale, g_scale);
			memcpy(&Im[0], cv_ImRGB.data, (int)(w * g_scale) * (int)(h * g_scale) * sizeof(int));
		}
		
		if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_" + iter_det + "_03_3_Im_RGBScaled4x4" + g_im_save_format, w * g_scale, h * g_scale);
		if (g_show_results) SaveRGBImage(Im, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_03_3_Im_RGBScaled4x4" + g_im_save_format, w * g_scale, h * g_scale);

		for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
		{
			memcpy(&ImRES2[j], &ImNF[i], w * sizeof(int));
		}

		{
			cv::Mat cv_ImGROrig, cv_ImGR;
			GreyscaleImageToMat(ImRES2, w, h, cv_ImGROrig);
			cv::resize(cv_ImGROrig, cv_ImGR, cv::Size(0, 0), g_scale, g_scale);
			GreyscaleMatToImage(cv_ImGR, w * g_scale , h * g_scale, ImSNF);
		}
		//SimpleResizeImage4x(ImRES2, ImSNF, w, h);

		if (g_show_results) SaveGreyscaleImage(ImSNF, "\\TestImages\\FindTextLines_" + iter_det + "_04_2_ImSNF" + g_im_save_format, w * g_scale, h * g_scale);

		w *= g_scale;
		h *= g_scale;

		{
			// extend ImSNF with Internal Figures
			val = GetImageWithInsideFigures(ImSNF, ImRES1, w, h, 255, val1, val2);
			if (val == 0)
			{
				return res;
			}
			if (val2 > 0)
			{
				CombineTwoImages(ImSNF, ImRES1, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImSNF, "\\TestImages\\FindTextLines_" + iter_det + "_04_4_SNFExtInternalFigures" + g_im_save_format, w, h);
			}
		}

		cv::cvtColor(cv_ImRGB, cv_ImY, cv::COLOR_BGR2GRAY);
		cv::cvtColor(cv_ImRGB, cv_ImLAB, cv::COLOR_BGR2Lab);
		cv::cvtColor(cv_ImRGB, cv_ImHSV, cv::COLOR_BGR2HSV);
		cv::split(cv_ImLAB, cv_ImLABSplit);
		cv::split(cv_ImHSV, cv_ImHSVSplit);

		GreyscaleMatToImage(cv_ImY, w, h, ImY);
		GreyscaleMatToImage(cv_ImLABSplit[1], w, h, ImU);
		GreyscaleMatToImage(cv_ImLABSplit[2], w, h, ImV);
		GreyscaleMatToImage(cv_ImHSVSplit[1], w, h, ImI);

		if (g_show_results) SaveGreyscaleImage(ImU, "\\TestImages\\FindTextLines_" + iter_det + "_06_1_ImU" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImV, "\\TestImages\\FindTextLines_" + iter_det + "_06_2_ImV" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImI, "\\TestImages\\FindTextLines_" + iter_det + "_06_3_ImI" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImY, "\\TestImages\\FindTextLines_" + iter_det + "_06_5_ImY" + g_im_save_format, w, h);

		cv_bw = cv_ImY;
		cv::medianBlur(cv_bw, cv_bw, 5);
		cv::adaptiveThreshold(cv_bw, cv_bw_gaus, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

		if (g_show_results)
		{
			GreyscaleMatToImage(cv_bw_gaus, w, h, ImRES1);
			SaveGreyscaleImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_07_1_ImTHRGaus" + g_im_save_format, w, h);
		}

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
		// Create binary image from source image
		cv::cvtColor(imgResult, cv_bw, cv::COLOR_BGR2GRAY);
		cv::threshold(cv_bw, cv_bw, 40, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

		if (g_show_results)
		{
			GreyscaleMatToImage(cv_bw, w, h, ImRES1);
			SaveGreyscaleImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_07_2_ImTHRBinOtsu" + g_im_save_format, w, h);
		}

		for (int i = 0; i < w*h; i++)
		{
			if (cv_bw_gaus.data[i] == 0)
			{
				cv_bw.data[i] = 0;
			}
		}

		GreyscaleMatToImage(cv_bw, w, h, ImTHR);
		if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\FindTextLines_" + iter_det + "_07_3_ImTHRMerge" + g_im_save_format, w, h);

		if (g_scale >= 4.0)
		{
			// noise removal
			cv::Mat kernel_open = cv::Mat::ones(3, 3, CV_8U);
			cv::Mat opening;
			cv::morphologyEx(cv_bw, opening, cv::MORPH_OPEN, kernel_open, cv::Point(-1, -1), 2);
			//cv::imshow("opening", opening);		
			//cv::waitKey(0);		
			GreyscaleMatToImage(opening, w, h, ImSNFIntTHRF);
		}
		else
		{
			GreyscaleMatToImage(cv_bw, w, h, ImSNFIntTHRF);
		}

		if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_09_1_ImTHROpening" + g_im_save_format, w, h);
	}

	//wxMessageBox("dt2: " + std::to_string(GetTickCount() - start_time));
	start_time = GetTickCount();

	ClearImageFromBorders(ImSNFIntTHRF, w, h, 1, h - 2, 255);
	if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_09_2_ImTHRClearedFromBorders" + g_im_save_format, w, h);

	IntersectTwoImages(ImSNFIntTHRF, ImSNF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_09_3_ImSNFIntTHRF" + g_im_save_format, w, h);

	{
		// extend ImSNFIntTHRF with Internal Figures
		val = GetImageWithInsideFigures(ImSNFIntTHRF, ImRES1, w, h, 255, val1, val2);
		if (val == 0)
		{
			return res;
		}
		if (val2 > 0)
		{
			CombineTwoImages(ImSNFIntTHRF, ImRES1, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_10_1_ImSNFIntTHRFExtInternalFigures" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_10_1_ImSNFIntTHRFExtInternalFigures" + g_im_save_format, w, h);
		}

		// extend ImTHR with Internal Figures
		val = GetImageWithInsideFigures(ImTHR, ImRES1, w, h, 255, val1, val2);
		if (val == 0)
		{
			return res;
		}
		if (val2 > 0)
		{
			CombineTwoImages(ImTHR, ImRES1, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\FindTextLines_" + iter_det + "_10_2_ImTHRExtInternalFigures" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_10_2_ImTHRExtInternalFigures" + g_im_save_format, w, h);
		}
	}

	val = (int)(0.03*(double)H) + 1; // ~ min sub height # (536-520+1)/576
	min_h = (int)(0.4*(double)min<int>(val, orig_LLEk - orig_LLBk + 1));

	ClearImageFromGarbageBetweenLines(ImSNFIntTHRF, w, h, (LLB[k] - YB) * g_scale, (LLE[k] - YB) * g_scale, min_h * g_scale, 255);
	if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_11_1_ImSNFIntTHRFClearedFromGarbageBetweenLines" + g_im_save_format, w, h);

	// Checking ImSNFIntTHRF on presence more then one line (if yes spliting on two and repeat)
	//-------------------------------		
	val = GetSubParams(ImSNFIntTHRF, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale);

	if (val == 1)
	{
		if (g_show_results)
		{
			memcpy(&ImRES1[0], &ImSNFIntTHRF[0], w * h * sizeof(int));

			for (x = 0; x < w; x++)
			{
				ImRES1[lb * w + x] = cc;
				ImRES1[le * w + x] = cc;

				ImRES1[(LMAXY - LH + 1) * w + x] = gc;
				ImRES1[LMAXY * w + x] = gc;
			}

			SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_11_2_ImSNFIntTHRFWithSubParams" + g_im_save_format, w, h);
			SaveRGBImage(ImRES1, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_11_2_ImSNFIntTHRFWithSubParams" + g_im_save_format, w, h);
		}
	}

	yb = (orig_LLBk - YB) * g_scale;
	ye = (orig_LLEk - YB) * g_scale;
	xb = (orig_LLk - XB) * g_scale;
	xe = std::min<int>((int)(((W / 2) - XB) * g_scale) + ((int)(((W / 2) - XB) * g_scale) - xb), (orig_LRk - XB) * g_scale); //sometimes frome the right can be additional not centered text lines which intersect with two centered lines

	if ((val == 1) && (std::max<int>((LLE[k] - LLB[k] + 1) * g_scale, le - lb + 1) > 1.7*(double)LH))
	{
		bln = 0;

		custom_buffer<int> lw(h, 0);
		int max_txt_w, max_txt_y, min_y, max_y, new_txt_y = 0, new_txt_w, max_txt2_w, max_txt2_y;

		for (y = yb; y <= ye; y++)
		{
			for (x = xb; x < xe; x++, i++)
			{
				if (ImSNFIntTHRF[y*w + x] != 0)
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
			return res;
		}

		min_y = yb;
		max_y = LMAXY - LH;

		int y1, y2;

		for (y1 = max_y - 1; y1 >= min_y; y1--)
		{
			if (lw[y1] >= min_h * g_scale) // bigger then min sub height
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

				break;
			}
		}

		if ((new_txt_y > 0) &&
			(((YB + (new_txt_y / g_scale)) - orig_LLBk + 1) >= g_scale) &&
			((orig_LLEk - (YB + (new_txt_y / g_scale) + 1) + 1) >= g_scale)
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
			LLB[k + 1] = YB + (new_txt_y / g_scale) + 1;
			LLE[k + 1] = orig_LLEk;

			LL[k] = orig_LLk;
			LR[k] = orig_LRk;
			LLB[k] = orig_LLBk;
			LLE[k] = YB + (new_txt_y / g_scale);

			N++;

			res.m_LL = LL;
			res.m_LR = LR;
			res.m_LLB = LLB;
			res.m_LLE = LLE;
			res.m_N = N;
			res.m_k = k;
			res.m_iter_det = iter_det;
			return res;
		}
	}
	//----------------------------	

	while (1)
	{
		cv::Mat cv_ImGR, cv_ImGRRes;
		custom_buffer<int> ImMaskWithBorder, ImMaskWithBorderBySplit;

		if (LMAXY == 0)
		{
			LMAXY = h - 1;
		}
		if (LH == 0)
		{
			LH = LMAXY;
		}

		ImMaskWithBorder = custom_buffer<int>(w*h, 255);

		//wxMessageBox("dt3: " + std::to_string(GetTickCount() - start_time));
		start_time = GetTickCount();

		GetMainClusterImage(Im, ImSNFIntTHRF, ImSFIntTHRF, ImMaskWithBorder, w, h, LH, LMAXY, "" + iter_det + "_call1", 255, true, true, ((W / 2) - XB) * g_scale);
		
		//wxMessageBox("dt4: " + std::to_string(GetTickCount() - start_time));
		start_time = GetTickCount();
		
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_12_2_ImSFIntTHRF_MainTXTCluster" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_12_2_ImSFIntTHRF_MainTXTCluster" + g_im_save_format, w, h);
		IntersectTwoImages(ImSFIntTHRF, ImSNFIntTHRF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_12_3_ImSFIntTHRF_MainTXTClusterIntImSNFIntTHRF" + g_im_save_format, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSFIntTHRF, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_12_3_ImSFIntTHRF_MainTXTClusterIntImSNFIntTHRF" + g_im_save_format, w, h);		

		delta = 40;

		val1 = (int)((double)((LLE[k] - LLB[k] + 1) * g_scale)*0.3);
		val2 = (int)((double)((LR[k] - LL[k] + 1) * g_scale)*0.1);
		yb = (LLB[k] - YB) * g_scale + val1;
		ye = (LLE[k] - YB) * g_scale - val1;
		xb = (LL[k] - XB) * g_scale + val2;
		xe = (LR[k] - XB) * g_scale - val2;

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
			j2_min = std::max<int>(0, std::min<int>(j2_min, j2 + (delta / 2) - (min_delta / 2)));
			j2_max = std::min<int>(255, std::max<int>(j2_max, j2 + (delta / 2) + (min_delta / 2) - 1));
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

		concurrency::parallel_invoke(
			[&ImY, &ImU, &ImV, &ImI, &ImRES1, &ImTHR, &ImFF, &ImSNFIntTHRF, rc, w, h, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, iter_det] {
			int i, val1, val2, val3, val4;

			for (i = 0; i < w*h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];
				val4 = ImI[i];

				if ((val1 >= j1_min) && (val1 <= j1_max) &&
					(val2 >= j2_min) && (val2 <= j2_max) &&
					(val3 >= j3_min) && (val3 <= j3_max) &&
					(val4 >= j4_min) && (val4 <= j4_max)
					)
				{
					ImRES1[i] = rc;
				}
				else
				{
					ImRES1[i] = 0;
				}
			}

			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_42_1_ImRES1" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES1, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_42_2_ImRES1IntImTHR" + g_im_save_format, w, h);

			memcpy(&ImFF[0], &ImRES1[0], (w*h) * sizeof(int));

			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_42_3_ImRES1" + g_im_save_format, w, h);
			IntersectTwoImages(ImSNFIntTHRF, ImRES1, w, h);
			if (g_show_results) SaveGreyscaleImage(ImSNFIntTHRF, "\\TestImages\\FindTextLines_" + iter_det + "_42_4_ImSNFIntTHRFIntImRES1" + g_im_save_format, w, h);			
		},
			[&ImY, &ImU, &ImV, &ImI, &ImRES3, &ImTHR, rc, w, h, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, iter_det] {
			int i, val1, val2, val3, val4, val_min, val_max;

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

			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_44_1_ImRES3" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES3, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_44_2_ImRES3IntImTHR" + g_im_save_format, w, h);
		},
			[&ImY, &ImU, &ImV, &ImI, &ImRES4, &ImTHR, &ImRES7, rc, w, h, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, iter_det] {
			int i, val1, val2, val3, val4, val_min, val_max;

			val_min = j1_min + 20;
			val_max = j1_max + 20;
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

			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_45_1_ImRES4" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES4, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_45_2_ImRES4IntImTHR" + g_im_save_format, w, h);

			memcpy(&ImRES7[0], &ImRES4[0], (w*h) * sizeof(int));
		},
			[&ImY, &ImU, &ImV, &ImI, &ImRES5, &ImTHR, rc, w, h, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, iter_det] {
			int i, val1, val2, val3, val4, val_min, val_max;

			val_min = j1_min;
			val_max = j1_max + 20;
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
					ImRES5[i] = rc;
				}
				else
				{
					ImRES5[i] = 0;
				}
			}

			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_" + iter_det + "_46_1_ImRES5" + g_im_save_format, w, h);
			IntersectTwoImages(ImRES5, ImTHR, w, h);
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_" + iter_det + "_46_2_ImRES5IntImTHR" + g_im_save_format, w, h);
		}
		);

		yb = (LLB[k] - YB) * g_scale;
		ye = (LLE[k] - YB) * g_scale;
		xb = (LL[k] - XB) * g_scale;
		xe = (LR[k] - XB) * g_scale;

		// note: need to use ImSNFIntTHRF even if some symbols was removed in other case ImFF can have to mach garbage
		val = (int)(0.03*(double)H) + 1; // ~ min sub height # (536-520+1)/576
		min_h = (int)(0.4*(double)max<int>(val, orig_LLEk - orig_LLBk + 1));
		val = GetSubParams(ImSNFIntTHRF, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale);
		if (val == 0)
		{
			memset(&ImFF[0], 0, (w*h) * sizeof(int));
			break;
		}

		if (g_show_results)
		{
			memcpy(&ImRES2[0], &ImSNFIntTHRF[0], w * h * sizeof(int));

			int ddy1, ddy2;
			GetDDY(h, LH, LMAXY, ddy1, ddy2);

			for (x = 0; x < w; x++)
			{
				ImRES2[lb * w + x] = cc;
				ImRES2[le * w + x] = cc;

				ImRES2[(LMAXY - LH + 1) * w + x] = gc;
				ImRES2[LMAXY * w + x] = gc;

				ImRES2[std::max<int>(0, (LMAXY - ((6 * LH) / 5) + 1)) * w + x] = yc;
				ImRES2[std::min<int>(h - 1, (LMAXY + (LH / 5))) * w + x] = yc;

				ImRES2[ddy1 * w + x] = wc;
				ImRES2[ddy2 * w + x] = wc;
			}

			SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_48_3_ImSNFIntTHRFIntImRES1WithSubParams" + g_im_save_format, w, h);
			SaveRGBImage(ImRES2, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_48_3_ImSNFIntTHRFIntImRES1WithSubParams" + g_im_save_format, w, h);
		}

		concurrency::parallel_invoke(
			[&ImRES1, &N1, w, h, LH, LMAXY, iter_det, rc] {
			N1 = ClearImageOptimal(ImRES1, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_48_4_ImRES1F" + g_im_save_format, w, h);
		},
			[&ImRES3, &N3, w, h, LH, LMAXY, iter_det, rc] {
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_50_1_ImRES3" + g_im_save_format, w, h);
			N3 = ClearImageOptimal(ImRES3, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_50_2_ImRES3F" + g_im_save_format, w, h);
		},
			[&ImRES4, &N4, w, h, LH, LMAXY, iter_det, rc] {
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_51_1_ImRES4" + g_im_save_format, w, h);
			N4 = ClearImageOptimal(ImRES4, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_51_2_ImRES4F" + g_im_save_format, w, h);
		},
			[&ImRES5, &N5, w, h, LH, LMAXY, iter_det, rc] {
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_" + iter_det + "_52_1_ImRES5" + g_im_save_format, w, h);
			N5 = ClearImageOptimal(ImRES5, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES5, "\\TestImages\\FindTextLines_" + iter_det + "_52_2_ImRES5F" + g_im_save_format, w, h);
		}
		);

		minN = N5 / 2;
		for (i = 0; i < w*h; i++)
		{
			if (((N1 >= minN) && (ImRES1[i] != 0)) ||
				((N3 >= minN) && (ImRES3[i] != 0)) ||
				((N4 >= minN) && (ImRES4[i] != 0)) ||
				((N5 >= minN) && (ImRES5[i] != 0))
				)
			{
				ImRES8[i] = rc;
			}
			else
			{
				ImRES8[i] = 0;
			}
		}
		if (g_show_results) SaveRGBImage(ImRES8, "\\TestImages\\FindTextLines_" + iter_det + "_54_1_ImRES8(ImRES1+3+4+5)" + g_im_save_format, w, h);

		val = ClearImageOpt2(ImRES8, w, h, LH, LMAXY, rc);
		if (g_show_results) SaveRGBImage(ImRES8, "\\TestImages\\FindTextLines_" + iter_det + "_54_2_ImRES8ClearImageOpt2" + g_im_save_format, w, h);

		if (val == 0)
		{
			memset(&ImFF[0], 0, (w*h) * sizeof(int));
			break;
		}

		concurrency::parallel_invoke(
			[&ImRES8, &ImFF, w, h, LH, LMAXY, iter_det, rc] {
			CombineTwoImages(ImFF, ImRES8, w, h, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_57_1_ImFF(ImRES1)WithImRES8" + g_im_save_format, w, h);

			ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_57_2_ImFF_F" + g_im_save_format, w, h);

			memcpy(&ImRES8[0], &ImFF[0], (w*h) * sizeof(int));
		},
			[&ImRES7, &N7, w, h, LH, LMAXY, iter_det, rc] {
			if (g_show_results) SaveRGBImage(ImRES7, "\\TestImages\\FindTextLines_" + iter_det + "_58_1_ImRES7(ImRES4)" + g_im_save_format, w, h);
			N7 = ClearImageOpt5(ImRES7, w, h, LH, LMAXY, rc);
			if (g_show_results) SaveRGBImage(ImRES7, "\\TestImages\\FindTextLines_" + iter_det + "_58_2_ImRES7(ImRES4)_F" + g_im_save_format, w, h);
		}
		);

		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_60_1_ImFF" + g_im_save_format, w, h);
		for (i = 0; i < w*h; i++)
		{
			if (((N1 >= minN) && (ImRES1[i] != 0)) ||
				((N5 >= minN) && (ImRES5[i] != 0)) ||
				((N7 >= minN) && (ImRES7[i] != 0))
				)
			{
				ImFF[i] = rc;
			}
		}

		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_60_2_ImFFWithImRES1+5+7" + g_im_save_format, w, h);
		ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_60_3_ImFF_F" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImTHR, "\\TestImages\\FindTextLines_" + iter_det + "_68_1_ImTHR" + g_im_save_format, w, h);
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_68_2_ImFF" + g_im_save_format, w, h);
		IntersectTwoImages(ImFF, ImTHR, w, h);
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_68_3_ImFFIntImTHR" + g_im_save_format, w, h);
		ClearImageOpt5(ImFF, w, h, LH, LMAXY, rc);
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_68_4_ImFFIntImTHR_F" + g_im_save_format, w, h);

		// original ImMaskWithBorder can remove some symbols in case of 0_21_28_720__0_22_21_001.jpeg so better to don't use it
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_69_4_ImFF_F" + g_im_save_format, w, h);
		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_69_4_ImFF_F" + g_im_save_format, w, h);

		if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_" + iter_det + "_69_5_ImRGB" + g_im_save_format, w, h);		

		int min_x = w - 1, max_x = 0, min_y = h - 1, max_y = 0, ww, hh;
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
		if (max_x > w - 1) max_x = w - 1;

		min_y -= 16;
		if (min_y < 0) min_y = 0;
		max_y += 16;
		if (max_y > h - 1) max_y = h - 1;

		ww = max_x - min_x + 1;
		hh = max_y - min_y + 1;

		if (LMAXY - min_y > hh - 1)
		{
			LMAXY = min_y + hh - 1;
		}

		if (LMAXY - min_y - LH < 0)
		{
			LH = LMAXY - min_y;
		}

		// ImRES1 - RGB image with size ww:hh (segment of original image)
		// ImRES2 - ImFF image with size ww:hh (segment of original image)
		for (y = min_y, i = min_y * w + min_x, j = 0; y <= max_y; y++, i += w, j += ww)
		{
			memcpy(&ImRES1[j], &Im[i], ww * sizeof(int));
			memcpy(&ImRES2[j], &ImFF[i], ww * sizeof(int));
			//memcpy(&ImRES6[j], &ImSFIntTHRF[i], ww * sizeof(int));
			//memcpy(&ImRES7[j], &ImMaskWithBorder[i], ww * sizeof(int));
		}
		//memcpy(&ImMaskWithBorder[0], &ImRES7[0], (ww*hh) * sizeof(int));

		if (g_show_results) SaveRGBImage(ImRES1, "\\TestImages\\FindTextLines_" + iter_det + "_70_1_ImRES1_ImRGB" + g_im_save_format, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_70_2_ImRES2_ImFF" + g_im_save_format, ww, hh);
		
		// note: original ImMaskWithBorder can remove some symbols in case of 0_21_28_720__0_22_21_001.jpeg so better to don't use it
		//if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\FindTextLines_" + iter_det + "_70_3_ImMaskWithBorder" + g_im_save_format, ww, hh);		

		// note: ImSFIntTHRF can have missed symbols by clear from bolder, bad to use
		//if (g_show_results) SaveGreyscaleImage(ImRES6, "\\TestImages\\FindTextLines_" + iter_det + "_70_4_ImRES6_ImSFIntTHRF" + g_im_save_format, ww, hh);
		//IntersectTwoImages(ImRES6, ImRES2, ww, hh);
		//if (g_show_results) SaveGreyscaleImage(ImRES6, "\\TestImages\\FindTextLines_" + iter_det + "_70_5_ImRES6_ImFFIntImSFIntTHRF" + g_im_save_format, ww, hh);
		
		for (i = 0; i < ww*hh; i++)
		{
			if (ImRES2[i] != 0)
			{
				ImRES6[i] = 255;
			}
			else
			{
				ImRES6[i] = 0;
			}
		}

		ImMaskWithBorder = custom_buffer<int>(ww*hh, 255);
		ImMaskWithBorderBySplit = custom_buffer<int>(ww*hh, 255);

		//wxMessageBox("dt5: " + std::to_string(GetTickCount() - start_time));
		start_time = GetTickCount();

		concurrency::parallel_invoke(
			[&] {
			GetMainClusterImage(ImRES1, ImRES6, ImRES3, ImMaskWithBorder, ww, hh, LH, LMAXY - min_y, "" + iter_det + "_call2", rc, true);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_71_1_ImRES3_MainTXTCluster" + g_im_save_format, ww, hh);
			ClearImageOpt5(ImRES3, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_71_2_ImRES3_MainTXTClusterF" + g_im_save_format, ww, hh);
		},
			[&] {
			GetMainClusterImageBySplit(ImRES1, ImRES6, ImRES4, ImMaskWithBorderBySplit, ww, hh, LH, LMAXY - min_y, "" + iter_det + "_call3", rc);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_73_1_ImRES4_MainTXTClusterSplit" + g_im_save_format, ww, hh);
			ClearImageOpt5(ImRES4, ww, hh, LH, LMAXY - min_y, rc);
			if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_73_2_ImRES4_MainTXTClusterSplitF" + g_im_save_format, ww, hh);
		}
		);

		//wxMessageBox("dt6: " + std::to_string(GetTickCount() - start_time));
		start_time = GetTickCount();

		IntersectTwoImages(ImMaskWithBorder, ImMaskWithBorderBySplit, ww, hh);

		if (g_show_results) SaveRGBImage(ImRES4, "\\TestImages\\FindTextLines_" + iter_det + "_74_1_ImRES4_MainTXTClusterSplitF" + g_im_save_format, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_74_2_ImRES3_MainTXTClusterF" + g_im_save_format, ww, hh);
		MergeImagesByIntersectedFigures(ImRES3, ImRES4, ww, hh, rc);
		if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_74_3_ImRES3_MainTXTClusterFMerge1+2" + g_im_save_format, ww, hh);
		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\FindTextLines_" + iter_det + "_74_4_ImMaskWithBorder" + g_im_save_format, ww, hh);
		IntersectTwoImages(ImRES3, ImMaskWithBorder, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_74_5_ImRES3_MainTXTClusterFilteredByImMaskWithBorder" + g_im_save_format, ww, hh);

		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "\\TestImages\\FindTextLines_" + iter_det + "_75_1_ImMaskWithBorder" + g_im_save_format, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_75_2_ImRES2_ImFF" + g_im_save_format, ww, hh);
		IntersectTwoImages(ImRES2, ImMaskWithBorder, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_75_3_ImRES2_ImFFFilteredByImMaskWithBorder" + g_im_save_format, ww, hh);

		if (g_show_results) SaveRGBImage(ImRES3, "\\TestImages\\FindTextLines_" + iter_det + "_76_1_ImRES3_MainTXTCluster" + g_im_save_format, ww, hh);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_76_2_ImRES2_ImFF" + g_im_save_format, ww, hh);
		MergeWithClusterImage(ImRES2, ImRES3, ww, hh, rc);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_76_3_ImRES2_ImFFMergeWithMainTXTCluster" + g_im_save_format, ww, hh);
		ClearImageOpt5(ImRES2, ww, hh, LH, LMAXY - min_y, rc);
		if (g_show_results) SaveRGBImage(ImRES2, "\\TestImages\\FindTextLines_" + iter_det + "_76_4_ImRES2_ImFF_F" + g_im_save_format, ww, hh);

		for (y = min_y, i = min_y * w + min_x, j = 0; y <= max_y; y++, i += w, j += ww)
		{
			memcpy(&ImFF[i], &ImRES2[j], ww * sizeof(int));
		}

		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_77_1_ImFF" + g_im_save_format, w, h);

		if (g_clear_image_logical == true)
		{
			val = ClearImageLogical(ImFF, w, h, LH, LMAXY, xb, xe, rc, W * g_scale, H * g_scale, ((W / 2) - XB) * g_scale);
			if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\FindTextLines_" + iter_det + "_77_2_ImFFWithClearImageLogical" + g_im_save_format, w, h);
		}

		if (g_show_results) SaveRGBImage(ImFF, "\\TestImages\\" + SaveName + "_FindTextLines_" + iter_det + "_77_3_ImFF" + g_im_save_format, w, h);

		if (g_show_results) SaveRGBImage(Im, "\\TestImages\\FindTextLines_" + iter_det + "_77_2_ImRGB" + g_im_save_format, w, h);

		//wxMessageBox("dt7: " + std::to_string(GetTickCount() - start_time));
		start_time = GetTickCount();

		//LLE[k] = YB + (LMAXY / g_scale);

		break;
	}

	ww = W * g_scale;
	hh = h;

	ImRES1 = custom_buffer<int>(ww*hh, 0);
	for (i = 0; i < ww*hh; i++) ImRES1[i] = wc;

	cnt = 0;
	for (y = 0, i = 0; y < h; y++)
	{
		for (x = 0; x < w; x++, i++)
		{
			if (ImFF[i] != 0)
			{
				cnt++;
				ImRES1[y*ww + (int)(XB * g_scale) + x] = 0;
			}
		}
	}

	if (cnt > 0)
	{
		{
			cv::Mat cv_ImRGBOrig(h, ww, CV_8UC4);
			memcpy(cv_ImRGBOrig.data, &ImRES1[0], h*ww * sizeof(int));
			cv::resize(cv_ImRGBOrig, res.m_cv_ImRGB, cv::Size(0, 0), 1.0/g_scale, 1.0/g_scale);
		}

		GetTextLineParameters(ImFF, ImY, ImU, ImV, w, h, LH, LMAXY, DXB, DXE, DYB, DYE, mY, mI, mQ, rc);		
		
		res.m_YB = YB;
		res.m_LH = LH / g_scale;
		res.m_LY = YB + LMAXY / g_scale;
		res.m_LXB = XB + DXB / g_scale;
		res.m_LXE = XB + DXE / g_scale;
		res.m_LYB = YB + DYB / g_scale;
		res.m_LYE = YB + DYE / g_scale;
		res.m_mY = mY;
		res.m_mI = mI;
		res.m_mQ = mQ;
		res.m_im_h = h / g_scale;

		char str[10];
		string FullName;		

		FullName = string("/TXTImages/");
		FullName += SaveName;
		FullName += string("_");
		sprintf(str, "%.5d", res.m_LY);
		FullName += str;
		FullName += g_im_save_format;

		res.m_ImageName = FullName;

		SaveRGBImage(ImRES1, FullName, ww, hh);

		res.m_res = 1;
	}

	return res;
}

inline concurrency::task<FindTextRes> TaskFindText(custom_buffer<int> &ImRGB, custom_buffer<int> &ImNF, custom_buffer<int> &FullImY, string &SaveName, std::string &iter_det, int N, int k, custom_buffer<int> &LL, custom_buffer<int> &LR, custom_buffer<int> &LLB, custom_buffer<int> &LLE, int W, int H)
{	
	return concurrency::create_task([&ImRGB, &ImNF, &FullImY, SaveName, iter_det, N, k, LL, LR, LLB, LLE, W, H]	{
			return FindText(ImRGB, ImNF, FullImY, SaveName, iter_det, N, k, LL, LR, LLB, LLE, W, H);
		}
	);
}

int FindTextLines(custom_buffer<int> &ImRGB, custom_buffer<int> &ImClearedText, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, vector<string> &SavedFiles, int W, int H)
{
	custom_buffer<int> LL(H, 0), LR(H, 0), LLB(H, 0), LLE(H, 0), LW(H, 0);
	custom_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	custom_buffer<int> FullImY(W*H, 0);

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
	int res = 0;
	int iter = 0;	
	DWORD  start_time;

	//start_time = GetTickCount();

	SaveName = SavedFiles[0];
	SavedFiles.clear();

	mthr = 0.3;
	segh = g_segh;
	int color, rc, gc, bc, yc, cc, wc;
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
	pClr[1] = 255;
	pClr[2] = 255;
	yc = color;

	color = 0;
	pClr[2] = 128;
	pClr[0] = 128;
	cc = color;

	color = 0;
	pClr[0] = 255;
	pClr[1] = 255;
	pClr[2] = 255;
	wc = color;

	for (i = 0; i < W*H; i++) ImClearedText[i] = wc;

	g_pViewImage[0](ImRGB, W, H);	

	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\FindTextLines_01_1_ImRGB" + g_im_save_format, W, H);
	if (g_show_results) SaveRGBImage(ImRGB, "\\TestImages\\" + SaveName + g_im_save_format, W, H);

	if (g_show_results) SaveGreyscaleImage(ImF, "\\TestImages\\FindTextLines_01_2_ImF" + g_im_save_format, W, H);
	if (g_show_results) SaveGreyscaleImage(ImNF, "\\TestImages\\FindTextLines_01_3_ImNF" + g_im_save_format, W, H);

	{
		cv::Mat cv_FullImRGB(H, W, CV_8UC4), cv_FullImY;
		memcpy(cv_FullImRGB.data, &ImRGB[0], W*H * sizeof(int));
		cv::cvtColor(cv_FullImRGB, cv_FullImY, cv::COLOR_BGR2GRAY);
		GreyscaleMatToImage(cv_FullImY, W, H, FullImY);
		if (g_show_results) SaveGreyscaleImage(FullImY, "\\TestImages\\FindTextLines_01_4_FullImY" + g_im_save_format, W, H);
	}

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

	//wxMessageBox("dt0: " + std::to_string(GetTickCount() - start_time));
	//start_time = GetTickCount();

	vector<concurrency::task<FindTextRes>> FindTextTasks;
	
	for (k = 0; k < N; k++)
	{
		FindTextTasks.emplace_back(TaskFindText(ImRGB, ImNF, FullImY, SaveName, "l" + std::to_string(k + 1), N, k, LL, LR, LLB, LLE, W, H));
	}
	
	k = 0;
	while (k < FindTextTasks.size())
	{
		FindTextRes ft_res = FindTextTasks[k].get();

		if (ft_res.m_LL.m_size > 0) // ned to split text on 2 parts
		{
			FindTextTasks.emplace_back(TaskFindText(ImRGB, ImNF, FullImY, SaveName, ft_res.m_iter_det + "_sl1", ft_res.m_N, ft_res.m_k, ft_res.m_LL, ft_res.m_LR, ft_res.m_LLB, ft_res.m_LLE, W, H));
			FindTextTasks.emplace_back(TaskFindText(ImRGB, ImNF, FullImY, SaveName, ft_res.m_iter_det + "_sl2", ft_res.m_N, ft_res.m_k + 1, ft_res.m_LL, ft_res.m_LR, ft_res.m_LLB, ft_res.m_LLE, W, H));
		}
		else
		{
			if (ft_res.m_res == 1)
			{
				SavedFiles.push_back(ft_res.m_ImageName);
				//SaveTextLineParameters(ft_res.m_ImageName, ft_res.m_YB, ft_res.m_LH, ft_res.m_LY, ft_res.m_LXB, ft_res.m_LXE, ft_res.m_LYB, ft_res.m_LYE, ft_res.m_mY, ft_res.m_mI, ft_res.m_mQ, W, H);

				for (y = 0, i = 0; y < ft_res.m_im_h; y++)
				{
					for (x = 0; x < W; x++, i++)
					{
						if (((int*)(ft_res.m_cv_ImRGB.data))[i] == 0)
						{
							ImClearedText[ft_res.m_YB*W + i] = 0;
						}
					}
				}

				g_pViewRGBImage(ImClearedText, W, H);

				res = 1;
			}
		}

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

int ClearImageFromBorders(custom_buffer<int> &Im, int w, int h, int ddy1, int ddy2, int white)
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
			(pFigure->m_minY < ddy1) ||
			(pFigure->m_maxY > ddy2)
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

	GetDDY(h, LH, LMAXY, ddy1, ddy2);
	
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];
		
		if ((pFigure->m_h < min_h) ||
			(pFigure->m_minX <= g_scale) ||
			(pFigure->m_maxX >= (w - 1) - g_scale) ||
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
			// (pFigure->m_w < pFigure->m_h * 2) &&
			(!((pFigure->m_minX <= 4) ||
				(pFigure->m_maxX >= (w - 1) - 4) ||
				(pFigure->m_minY <= 0) ||
				(pFigure->m_maxY >= (h - 1))
				))
			)
		{
			good_figures[j] = pFigure;
			if ((pFigure->m_h > LH) && (pFigure->m_minX < real_im_x_center))
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

	if (LH == 0)
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

	custom_buffer<int> arMinY(N, 0);

	for (i = 0, k = 0; i < NNY; i++)
	{
		pFigure = good_figures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LH / 2))
			)
		{
			arMinY[k] = pFigure->m_minY;
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
			if (arMinY[j] < arMinY[i])
			{
				val = arMinY[i];
				arMinY[i] = arMinY[j];
				arMinY[j] = val;
			}
		}
	}

	i = 0;
	H = 0;
	while (i < k)
	{
		if ((arMinY[i] - arMinY[0]) > (LMAXY - arMinY[0] + 1)/6)
		{
			break;
		}
		else
		{
			H += (LMAXY - arMinY[i] + 1);
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

int GetImageWithInsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white, int &val1, int &val2, int LH, int LMAXY, int real_im_x_center)
{
	CMyClosedFigure *pFigure;
	int i, l, x, y, ii, cnt, N;
	CMyPoint *PA;

	memset(&ImRes[0], 0, w*h * sizeof(int));
	val1 = 0;
	val2 = 0;

	if (LH == 0) LH = h;
	if (LMAXY == 0) LMAXY = h-1;
	if (real_im_x_center == 0) real_im_x_center = w-1;	

	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(Im, w, h, 0, pFigures);
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

		if ((pFigure->m_minX == 0) ||
			(pFigure->m_maxX == w - 1) ||
			(pFigure->m_minY == 0) ||
			(pFigure->m_maxY == h-1)
			)
		{
			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		else
		{
			PA = pFigure->m_PointsArray;

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImRes[ii] = white;
			}
		}
		i++;
	}

	for (y = LMAXY - LH + 1; y <= LMAXY; y++)
	{
		for (x = 0; x < std::min<int>(w, real_im_x_center); x++)
		{
			i = y * w + x;
			if (Im[i] != 0)
			{
				val1++;
			}
			if (ImRes[i] != 0)
			{
				val2++;
			}
		}
	}

	return 1;
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

	GetDDY(h, LH, LMAXY, ddy1, ddy2);

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_minX <= g_scale) ||
			(pFigure->m_maxX >= (w - 1) - g_scale) ||
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
		if ((pFigure->m_maxY <= LMAXY + LH / 8) && (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LH / 3)))
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
}

void MergeImagesByIntersectedFigures(custom_buffer<int> &ImInOut, custom_buffer<int> &ImIn2, int w, int h, int white)
{
	custom_buffer<int> Im2(ImIn2);
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;

	t = SearchClosedFigures(ImInOut, w, h, white, pFigures);
	for (i = 0; i < pFigures.size(); i++)
	{
		pFigure = &(pFigures[i]);
		PA = pFigure->m_PointsArray;

		bool found = false;
		for (l = 0; l < pFigure->m_Square; l++)
		{
			ii = PA[l].m_i;
			if (Im2[ii] != 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImInOut[ii] = 0;
			}
		}
	}

	t = SearchClosedFigures(Im2, w, h, white, pFigures);
	for (i = 0; i < pFigures.size(); i++)
	{
		pFigure = &(pFigures[i]);
		PA = pFigure->m_PointsArray;

		bool found = false;
		for (l = 0; l < pFigure->m_Square; l++)
		{
			ii = PA[l].m_i;
			if (ImInOut[ii] != 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im2[ii] = 0;
			}
		}
	}

	CombineTwoImages(ImInOut, Im2, w, h, white);
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
		if ( ( (pFigure->m_maxY <= LMAXY + (LLH/4)) &&
			   (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LLH / 3)) ) ||
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
		if (((pFigure->m_maxY <= LMAXY + (LLH / 4)) &&
			(pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LLH / 3))) ||
			((pFigure->m_maxY <= LMAXY - LLH) &&
			(pFigure->m_maxY >= LMAXY - LLH * 1.25))
			)
		{
			ret = 1;
		}
	}

	return ret;
}

void SaveTextLineParameters(string ImageName, int YB, int LH, int LY, int LXB, int LXE, int LYB, int LYE, int mY, int mI, int mQ, int W, int H)
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

	sprintf(str, "%d", W);
	PropString += string(" W ");
	PropString += string(str);

	sprintf(str, "%d", H);
	PropString += string(" H ");
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
		if ((maxY[j]-maxY[i]) > std::max<int>(g_dmaxy, LH / 8))
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
		val = maxY[NNY-1] + std::max<int>(g_dmaxy, LH / 8);
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
			 (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 8)) &&
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
			(pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 8)) &&
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
int ClearImageLogical(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int xb, int xe, int white, int W, int H, int real_im_x_center)
{
	CMyClosedFigure *pFigure = NULL, *pFigure2 = NULL;
	int i, ib, i1, i2, i3, j, k, l, x, y, val, val1, N, bln, bln1, bln2, bln3, LMINY, LM1, LM2;
	int res, is_point, is_comma;
	CMyPoint *PA = NULL, *PA1 = NULL, *PA2 = NULL;
	clock_t t;
	double minpw, maxpw, minph, maxph;
	double minpwh;
	double dval;

	int NNY;

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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_01_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_02_Im" + g_im_save_format, w, h);

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
			
			if ( (  ( (pFigure2->m_maxY <= LMAXY) && (pFigure2->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) ) ||
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
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) ) ||
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
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) ) ||
					( (pFigure->m_minY*2 < 2*LMAXY-LH) && (pFigure->m_maxY > LMAXY) )
				 ) &&
				 (pFigure->m_h >= 0.6*LH)
				)
			{
				if ( !( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) ) &&
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

				bln1 = ( (pFigure->m_maxY <= LMAXY + 4) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) ) ||
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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_03_Im" + g_im_save_format, w, h);

	int min_h = (int)((double)LH/4);

	//-----очищаем с левого края-----//
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY < LMAXY- std::max<int>(g_dmaxy, LH / 3)*1.5) ||
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
				 (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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
	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_04_Im" + g_im_save_format, w, h);

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
			   !( (pFigure->m_maxY <= LMAXY + 4) && (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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
				 (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_05_Im" + g_im_save_format, w, h);

	//--определяем минимальную высоту нормальных символов--//
	custom_buffer<int> NH(N, 0);

	k = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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
	int LLH = NH[val];

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
				 (pFigure2->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_06_Im" + g_im_save_format, w, h);

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
				 (pFigure2->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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
						  (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) )
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

	if (g_show_results) SaveRGBImage(Im, "\\TestImages\\ClearImageLogical_07_Im" + g_im_save_format, w, h);

	//////////////////////////////
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		if ( !( (pFigure->m_maxY <= LMAXY) && 
			    (pFigure->m_maxY >= LMAXY- std::max<int>(g_dmaxy, LH / 3)) &&
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

void GreyscaleImageToMat(custom_buffer<int> &ImGR, int w, int h, cv::Mat &res)
{
	res = cv::Mat(h, w, CV_8UC1);

	for (int i = 0; i < w*h; i++)
	{
		res.data[i] = ImGR[i];
	}
}

void GreyscaleMatToImage(cv::Mat &ImGR, int w, int h, custom_buffer<int> &res)
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
