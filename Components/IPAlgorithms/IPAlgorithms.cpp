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
#ifdef WIN64
#include "cuda_kernels.h"
#endif
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <ppl.h>
#include <ppltasks.h>
using namespace std;

void ClearMainClusterImage(custom_buffer<int> &ImMainCluster, custom_buffer<int> ImMASK, custom_buffer<int> &ImIL, int w, int h, int W, int H, int LH, std::string iter_det);
void GetBinaryImageFromImageByColor(custom_buffer<int> &ImRES, custom_buffer<int> &ImGR, int white, int w, int h);
int CheckOnSubPresence(custom_buffer<int> &ImMASK, custom_buffer<int> &ImNE, custom_buffer<int> &ImFRes, int w, int h, int W, int H, int XB, int YB, std::string iter_det);
void SaveImageWithLinesInfo(custom_buffer<int> &Im, string name, int lb1, int le1, int lb2, int le2, int w, int h);
void ExtendByInsideFigures(custom_buffer<int> &ImRES, int w, int h, int white, bool simple = true);
void SaveImageWithSubParams(custom_buffer<int> &Im, string name, int lb, int le, int LH, int LMAXY, int real_im_x_center, int w, int h);
int ClearImageByTextDistance(custom_buffer<int> &Im, int w, int h, int W, int H, int real_im_x_center, int white, std::string iter_det);
void ClearImageByBigLines(custom_buffer<int> &Im, int w, int h, int W, int H);
int ClearImageOptimal2(custom_buffer<int> &Im, int w, int h, int min_x, int max_x, int min_y, int max_y, int white);
void GetMainColorImage(custom_buffer<int> &ImRES, custom_buffer<int> *pImMainCluster, custom_buffer<int> &ImMASK, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h, std::string iter_det, int min_x, int max_x);
void GreyscaleImageToBinary(custom_buffer<int> &ImRES, custom_buffer<int> &ImGR, int w, int h, int white);
int GetImageWithOutsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white);
void MergeImagesByIntersectedFigures(custom_buffer<int> &ImInOut, custom_buffer<int> &ImIn2, int w, int h, int white);
void MergeWithClusterImage(custom_buffer<int> &ImInOut, custom_buffer<int> &ImCluser, int w, int h, int white);
void(*g_pViewRGBImage)(custom_buffer<int> &Im, int w, int h);
void(*g_pViewImage[2])(custom_buffer<int> &Im, int w, int h);
int GetSubParams(custom_buffer<int> &Im, int w, int h, int white, int &LH, int &LMAXY, int &lb, int &le, int min_h, int real_im_x_center, int yb, int ye, std::string iter_det, bool combine_figures_related_to_each_other = true);
int ClearImageFromMainSymbols(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int white, std::string iter_det);
int ClearImageOpt2(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, int white, std::string iter_det);
int ClearImageOpt3(custom_buffer<int> &Im, int w, int h, int real_im_x_center, int white);
int ClearImageOpt4(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, int white);
int ClearImageOpt5(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int real_im_x_center, int white);
int ClearImageByMask(custom_buffer<int> &Im, custom_buffer<int> &ImMASK, int w, int h, int white, double thr = 0.2);
int ClearImageOptimal(custom_buffer<int> &Im, int w, int h, int white);
void CombineFiguresRelatedToEachOther(custom_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h, std::string iter_det);
void ClearImageFromGarbageBetweenLines(custom_buffer<int> &Im, int w, int h, int yb, int ye, int min_h, int white, std::string iter_det);
int ClearImageFromSmallSymbols(custom_buffer<int> &Im, int w, int h, int W, int H, int white);
int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H);
//int ThirdFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h, int W, int H);
int GetImageWithInsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white, bool simple = false);
void InvertBinaryImage(custom_buffer<int> &Im, int w, int h);

void GreyscaleImageToMat(custom_buffer<int> &ImGR, int w, int h, cv::Mat &res);
void GreyscaleMatToImage(cv::Mat &ImGR, int w, int h, custom_buffer<int> &res);
void BinaryImageToMat(custom_buffer<int> &ImBinary, int w, int h, cv::Mat &res);
void BinaryMatToImage(cv::Mat &ImBinary, int w, int h, custom_buffer<int> &res, int white);
void GreyscaleImageToMat(custom_buffer<int> &ImGR, int w, int h, cv::UMat &res);
void GreyscaleMatToImage(cv::UMat &ImGR, int w, int h, custom_buffer<int> &res);
void BinaryImageToMat(custom_buffer<int> &ImBinary, int w, int h, cv::UMat &res);
void BinaryMatToImage(cv::UMat &ImBinary, int w, int h, custom_buffer<int> &res, int white);
void RGBMatToImage(cv::UMat &ImRGB, int w, int h, custom_buffer<int> &res);
void RGBImageToMat(custom_buffer<int> &ImRGB, int w, int h, cv::UMat &res);
void InvertBinaryMat(cv::UMat &ImBinary, int w, int h, cv::UMat &res);

string  g_work_dir;
string  g_app_dir;

string  g_im_save_format = ".jpeg";
//string  g_im_save_format = ".bmp";

double	g_mthr = 0.4;
double	g_smthr = 0.25;
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

int g_use_simple = false;

#define STR_SIZE (256 * 2) // 

#define MAX_EDGE_STR 786432 //на сам деле ~ 32*3*255

//int g_LN;

//int g_blnVNE = 0;
//int g_blnHE = 0;

bool g_use_ILA_images_for_clear_txt_images = true;
bool g_use_gradient_images_for_clear_txt_images = true;
bool g_clear_txt_images_by_main_color = true;

bool g_clear_image_logical = false;

bool g_generate_cleared_text_images_on_test = false;
bool g_show_results = false;
bool g_show_sf_results = false;
bool g_clear_test_images_folder = true;
bool g_show_transformed_images_only = false;

bool g_wxImageHandlersInitialized = false;

bool g_use_ocl = true;

bool g_use_cuda_gpu = true;

int g_cuda_kmeans_initial_loop_iterations = 10;
int g_cuda_kmeans_loop_iterations = 30;
int g_cpu_kmeans_initial_loop_iterations = 10;
int g_cpu_kmeans_loop_iterations = 10;

double g_min_h = 12.0 / 720.0; // ~ min sub height in percents to image height

bool g_remove_wide_symbols = true;

bool g_disable_save_images = false;

bool InitCUDADevice()
{
	bool res = false;
#ifdef WIN64
	int num = GetCUDADeviceCount();
	if (num > 0)
	{
		res = true;
	}
#endif
	return res;
}

inline int GetClusterColor(int clusterCount, int cluster_id)
{
	int white = (255 / clusterCount)*(cluster_id + 1);
	return white;
}

inline int GetClusterId(int clusterCount, int white)
{
	int cluster_id = (white / (255 / clusterCount)) - 1;
	return cluster_id;
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
	custom_assert(ImIn.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImY.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImY.size() >= w*h");
	custom_assert(ImU.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImU.size() >= w*h");
	custom_assert(ImV.size() >= w*h, "RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h)\nnot: ImV.size() >= w*h");

	concurrency::parallel_for(0, h, [&](int y)
	{
		int i = y * w;
		u8 *color;
		int r, g, b, u, v;

		for (int x = 0; x < w; x++, i++)
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
	});
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

void ApplyModerateThresholdBySplit(custom_buffer<int> &Im, double mthr, int w, int h, int W, int H)
{	
	int nx = 12; // std::max<int>(1, std::round((double)w / 64.0));
	int ny = 12; // std::max<int>(1, std::round((double)h / ((double)H / 12.0)));
	int dh = h / ny;
	int dw = w / nx;

	for (int iy = 0; iy < ny; iy++)
	{
		int y_min = dh * iy;
		int hh = dh;
		if (iy == ny - 1)
		{
			hh = h - dh * iy;
		}
		int y_max = y_min + hh - 1;

		for (int ix = 0; ix < nx; ix++)
		{
			int x_min = dw * ix;
			int ww = dw;
			if (ix == nx - 1)
			{
				ww = w - dw * ix;
			}
			int x_max = x_min + ww - 1;

			int mx = 0;

			for (int y = y_min; y <= y_max; y++)
			{
				for (int x = x_min; x <= x_max; x++)
				{
					if (Im[(y * w) + x] > mx) mx = Im[(y * w) + x];
				}
			}

			int thr = std::max<int>(1, std::round((double)mx*mthr));

			for (int y = y_min; y <= y_max; y++)
			{
				for (int x = x_min; x <= x_max; x++)
				{
					if (Im[(y * w) + x] < thr) Im[(y * w) + x] = 0;
					else Im[(y * w) + x] = 255;
				}
			}
		}
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
		if (ImRes[i] == 0)
		{
			if (Im2[i] != 0)
			{
				ImRes[i] = white;
			}
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

void GetImCMOEWithThr1(custom_buffer<int> &ImCMOE, custom_buffer<int> &ImYMOE, custom_buffer<int> &ImUMOE, custom_buffer<int> &ImVMOE, int w, int h, int W, int H, custom_buffer<int> &offsets, custom_buffer<int> &dhs, int N, double mthr)
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

	//FindAndApplyGlobalThreshold(ImCMOE, w, h);
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
		ApplyModerateThreshold(ImCMOE.get_sub_buffer(i), mthr, w, dhs[k]);
	}
}


void GetImCMOEWithThr2(custom_buffer<int> &ImCMOE, custom_buffer<int> &ImYMOE, custom_buffer<int> &ImUMOE, custom_buffer<int> &ImVMOE, int w, int h, int W, int H, custom_buffer<int> &offsets, custom_buffer<int> &dhs, int N, double mthr)
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

	//FindAndApplyGlobalThreshold(ImCMOE, w, h);
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
		ApplyModerateThreshold(ImCMOE.get_sub_buffer(i), mthr, w, dhs[k]);
	}
}

void GetImFF(custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImRGB, custom_buffer<int> &ImYFull, custom_buffer<int> &ImUFull, custom_buffer<int> &ImVFull, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w,int h, int W, int H, double mthr)
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
		[&] { GetImCMOEWithThr1(ImRES4, ImYMOE, ImUMOE, ImVMOE, ww, hh, W, H, offsets, dhs, N, mthr); },
		[&] { GetImCMOEWithThr2(ImRES1, ImYMOE, ImUMOE, ImVMOE, ww, hh, W, H, offsets, dhs, N, mthr); }
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

void ClearImagesFromTooBigElements(custom_buffer<int> &ImSF, custom_buffer<int> &ImNE, int w, int h, int W, int H, int white)
{
	custom_buffer<int> Im(w*h, 0);

	memcpy(&Im[0], &ImNE[0], w*h * sizeof(int));
	IntersectTwoImages(Im, ImSF, w, h);

	CMyClosedFigure *pFigure;
	int i, l, ii, N, res = 0, x, y, y2;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	if (g_show_results) SaveGreyscaleImage(Im, "/TestImages/ClearImagesFromTooBigElements_01_01_Im" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/ClearImagesFromTooBigElements_01_02_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/TestImages/ClearImagesFromTooBigElements_01_03_ImNE" + g_im_save_format, w, h);

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_h >= 0.3*H) &&
			 (pFigure->m_w < pFigure->m_h) )
		{
			PA = pFigure->m_PointsArray;

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImSF[ii] = 0;
				ImNE[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}

	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/ClearImagesFromTooBigElements_02_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/TestImages/ClearImagesFromTooBigElements_02_ImNE" + g_im_save_format, w, h);
}

int FilterTransformedImage(custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H, std::string iter_det = "main")
{
	custom_buffer<int> ImRES1(w*h, 0), ImRES2(w*h, 0);
	int res = 0;

	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/FilterTransformedImage_" + iter_det + "_01_01_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/TestImages/FilterTransformedImage_" + iter_det + "_01_02_ImNE" + g_im_save_format, w, h);

	cv::Mat cv_im_gr;
	BinaryImageToMat(ImNE, w, h, cv_im_gr);
	cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), (g_min_h * H)/2);
	BinaryMatToImage(cv_im_gr, w, h, ImRES1, 255);
	IntersectTwoImages(ImSF, ImRES1, w, h);

	if (g_show_results) SaveGreyscaleImage(ImRES1, "/TestImages/FilterTransformedImage_" + iter_det + "_02_01_ImNEDilate" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/FilterTransformedImage_" + iter_det + "_02_02_ImSFMergeImDilate" + g_im_save_format, w, h);

	memcpy(&ImRES1[0], &ImSF[0], w*h * sizeof(int));

	/*cv::Mat cv_ImRGB(h, w, CV_8UC4), cv_ImLAB, cv_ImLABSplit[3];
	memcpy(cv_ImRGB.data, &ImRGB[0], w*h * sizeof(int));

	cv::cvtColor(cv_ImRGB, cv_ImLAB, cv::COLOR_BGR2Lab);
	cv::split(cv_ImLAB, cv_ImLABSplit);

	GreyscaleMatToImage(cv_ImLABSplit[0], w, h, ImY);
	GreyscaleMatToImage(cv_ImLABSplit[1], w, h, ImU);
	GreyscaleMatToImage(cv_ImLABSplit[2], w, h, ImV);*/

	//ClearImagesFromTooBigElements(ImSF, ImNE, w, h, W, H, 255);
	//memcpy(&ImFF[0], &ImSF[0], w*h * sizeof(int));
	
	res = SecondFiltration(ImSF, ImNE, LB, LE, N, w, h, W, H);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/FilterTransformedImage_" + iter_det + "_03_01_ImSFFSecondFiltration" + g_im_save_format, w, h);

	if (res == 1) RestoreStillExistLines(ImSF, ImRES1, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/FilterTransformedImage_" + iter_det + "_03_02_ImSFWithRestoredStillExistLines" + g_im_save_format, w, h);

	memcpy(&ImTF[0], &ImSF[0], w*h * sizeof(int));
	//if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);
	memcpy(&ImRES2[0], &ImTF[0], w*h * sizeof(int));
	if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_04_ImTFFThirdFiltration" + g_im_save_format, w, h);

	if (res == 1) res = ClearImageFromSmallSymbols(ImTF, w, h, W, H, 255);
	if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_05_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);

	//if (res == 1) res = SecondFiltration(ImTF, ImNE, LB, LE, N, w, h, W, H);
	//if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_06_ImTFFSecondFiltration" + g_im_save_format, w, h);

	//if (res == 1) res = ThirdFiltration(ImTF, LB, LE, N, w, h, W, H);
	//if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_07_ImTFFThirdFiltration" + g_im_save_format, w, h);

	if (res == 1) RestoreStillExistLines(ImTF, ImRES2, w, h);
	if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_08_ImTFWithRestoredStillExistLines" + g_im_save_format, w, h);

	if (res == 1) ExtendImFWithDataFromImNF(ImTF, ImRES1, w, h);
	if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FilterTransformedImage_" + iter_det + "_09_ImTFExtByImFF" + g_im_save_format, w, h);

	return res;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int GetTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, custom_buffer<int> &ImY, int w, int h, int W, int H)
{
	custom_buffer<int> LB(h, 0), LE(h, 0), ImU(w*h, 0), ImV(w*h, 0), ImHE(w*h, 0);	
	int N;
	int res = 0;
	__int64 t1, dt, num_calls = 100;

	if (g_show_results) SaveRGBImage(ImRGB, "/TestImages/GetTransformedImage_01_ImRGB" + g_im_save_format, w, h);	

	memset(&ImFF[0], 0, w*h * sizeof(int));
	memset(&ImSF[0], 0, w*h * sizeof(int));
	memset(&ImTF[0], 0, w*h * sizeof(int));
	memset(&ImNE[0], 0, w*h * sizeof(int));

	ColorFiltration(ImRGB, LB, LE, N, w, h);

	if (N == 0)
	{
		return res;
	}		

	RGB_to_YUV(ImRGB, ImY, ImU, ImV, w, h);	

	/*cv::Mat cv_ImRGB(h, w, CV_8UC4), cv_ImLAB, cv_ImLABSplit[3];
	memcpy(cv_ImRGB.data, &ImRGB[0], w*h * sizeof(int));

	cv::cvtColor(cv_ImRGB, cv_ImLAB, cv::COLOR_BGR2Lab);
	cv::split(cv_ImLAB, cv_ImLABSplit);

	GreyscaleMatToImage(cv_ImLABSplit[0], w, h, ImY);
	GreyscaleMatToImage(cv_ImLABSplit[1], w, h, ImU);
	GreyscaleMatToImage(cv_ImLABSplit[2], w, h, ImV);*/
	
	concurrency::parallel_invoke(
		[&] { GetImFF(ImFF, ImSF, ImRGB, ImY, ImU, ImV, LB, LE, N, w, h, W, H, g_mthr); },
		[&] { GetImNE(ImNE, ImY, ImU, ImV, w, h); },
		[&] { GetImHE(ImHE, ImY, ImU, ImV, w, h); }
	);	

	if (g_show_results) SaveGreyscaleImage(ImFF, "/TestImages/GetTransformedImage_02_1_ImFF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/TestImages/GetTransformedImage_02_2_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/TestImages/GetTransformedImage_02_3_ImNE" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImHE, "/TestImages/GetTransformedImage_02_4_ImHE" + g_im_save_format, w, h);
	
	CombineTwoImages(ImNE, ImHE, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/TestImages/GetTransformedImage_02_5_ImNE+HE" + g_im_save_format, w, h);

	res = FilterTransformedImage(ImFF, ImSF, ImTF, ImNE, LB, LE, N, w, h, W, H);

	return res;
}

int FilterImage(custom_buffer<int> &ImF, custom_buffer<int> &ImNE, int w, int h, int W, int H, custom_buffer<int> &LB, custom_buffer<int> &LE, int N)
{
	int res;
	custom_buffer<int> ImRES1(w*h, 0), ImRES2(w*h, 0);
	int diff_found = 1;
	int iter = 0;

	memcpy(&ImRES1[0], &ImF[0], w*h * sizeof(int));

	if (g_show_results) SaveGreyscaleImage(ImF, "/TestImages/FilterImage_01_ImFOrigin" + g_im_save_format, w, h);

	while (diff_found)
	{
		iter++;
		memcpy(&ImRES2[0], &ImF[0], w*h * sizeof(int));		

		res = SecondFiltration(ImF, ImNE, LB, LE, N, w, h, W, H);
		if (g_show_results) SaveGreyscaleImage(ImF, "/TestImages/FilterImage_iter" + std::to_string(iter) + "_02_ImSFFSecondFiltration" + g_im_save_format, w, h);

		//if (res == 1) res = ThirdFiltration(ImF, LB, LE, N, w, h, W, H);
		//if (g_show_results) SaveGreyscaleImage(ImF, "/TestImages/FilterImage_iter" + std::to_string(iter) + "_03_ImTFFThirdFiltration" + g_im_save_format, w, h);

		if (res == 1) res = ClearImageFromSmallSymbols(ImF, w, h, W, H, 255);
		if (g_show_results) SaveGreyscaleImage(ImF, "/TestImages/FilterImage_iter" + std::to_string(iter) + "_04_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);
		
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

inline bool IsTooRight(int lb, int le, const int dw2, int real_im_x_center)
{
	return (((lb + le - real_im_x_center*2) >= dw2 / 2) || (lb >= real_im_x_center));
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

	if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_01_Im" + g_im_save_format, w, h);
	
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

				/*
#ifdef DEBUG
				if (g_show_sf_results)
				{
					y = (ia / w);
					if ((y == 630) &&
						(iter == 1))
					{
						y = y;
					}
				}
#endif				
				*/

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

				int ln_start;

				do
				{
					ln_start = ln;
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

							if (IsTooRight(lb[l + 1], le[l + 1], dw2, w/2) ||
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
				} while (ln != ln_start);

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia/w) + "_iter" + std::to_string(iter) + "_01_ImFBetweenTextDistace" + g_im_save_format, w, h);

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

						if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_1_ImFTextCentreOffset" + g_im_save_format, w, h);

						break;
					}

					ln = l + 1;
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_2_ImFTextCentreOffset" + g_im_save_format, w, h);

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

						if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_03_ImFOnly2SubStrWithBigDist" + g_im_save_format, w, h);

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

						if (IsTooRight(lb[ln - 1], le[ln - 1], dw2, w/2) ||
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

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_04_ImFMinPointsDensity" + g_im_save_format, w, h);

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

						if (IsTooRight(lb[ln - 1], le[ln - 1], dw2, w/2) ||
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

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_06_ImFMinNEdgesPointsDensity" + g_im_save_format, w, h);				

				if (ln == 0)
				{
					break;
				}

				if (lb[0] >= w_2)
				{
					// removing all sub lines
					val = (le[ln - 1] - lb[0] + 1) * sizeof(int);
					for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
					{
						memset(&Im[i], 0, val);
					}
					ln = 0;

					if (g_show_sf_results) SaveGreyscaleImage(Im, "/TestImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_07_ImFAlignment" + g_im_save_format, w, h);

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

/*
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
}*/

inline void GetDDY(int &h, int &LH, int &LMAXY, int &ddy1, int &ddy2)
{
	ddy1 = std::max<int>(4, LMAXY - ((8 * LH) / 5));
	ddy2 = std::min<int>((h - 1) - 4, LMAXY + LH); // Arabic symbols can be too low

	if (ddy1 > LMAXY - LH - 3)
	{
		ddy1 = std::max<int>(1, LMAXY - LH - 3);
	}

	if (ddy2 < LMAXY + 3)
	{
		ddy2 = std::min<int>(h - 2, LMAXY + 3);
	}
}

int cuda_kmeans(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &labels, int w, int h, int numClusters, int loop_iterations, int initial_loop_iterations, int &min_x, int &max_x, int &min_y, int &max_y, bool mask_only = false, float threshold = 0.001)
{	
	int res = 0;
#ifdef WIN64	
	int numObjs = w * h, numObjsFF;
	float **clusters;
	custom_buffer<int> ImRGBFF(numObjs, 0), labelsFF(numObjs, 0);
	custom_buffer<char> color_cluster_id(1 << 24, -1);

	numObjsFF = 0;
	for (int i = 0; i < w*h; i++)
	{
		if (ImFF[i] != 0)
		{
			labelsFF[numObjsFF] = -1;
			ImRGBFF[numObjsFF] = ImRGB[i];
			numObjsFF++;
		}
	}

	if (numObjsFF < g_mpn)
	{
		return res;
	}

	if (numObjsFF > 0)
	{
		clusters = cuda_kmeans_img((unsigned char*)(ImRGBFF.m_pData), numObjsFF, numClusters, threshold, labelsFF.m_pData, initial_loop_iterations);
		free(clusters[0]);
		free(clusters);
	}

	if (numObjsFF > 0)
	{
		numObjsFF = 0;
		for (int i = 0; i < w*h; i++)
		{
			if (ImFF[i] != 0)
			{
				color_cluster_id[ImRGB[i] >> 8] = labelsFF[numObjsFF];
				numObjsFF++;
			}
		}
	}

	if (!mask_only)
	{
		if (numObjsFF > 0)
		{
			for (int i = 0; i < w*h; i++)
			{
				if (ImFF[i] != 0)
				{
					labels[i] = color_cluster_id[ImRGB[i] >> 8];
				}
				else
				{
					labels[i] = -1;
				}
			}
		}
		min_x = 0;
		max_x = w - 1;
		min_y = 0;
		max_y = h - 1;

		clusters = cuda_kmeans_img((unsigned char*)(ImRGB.m_pData), numObjs, numClusters, threshold, labels.m_pData, loop_iterations);
		free(clusters[0]);
		free(clusters);
	}
	else
	{
		if (numObjsFF == 0)
		{
			min_x = 0;
			max_x = w - 1;
			min_y = 0;
			max_y = h - 1;

			clusters = cuda_kmeans_img((unsigned char*)(ImRGB.m_pData), numObjs, numClusters, threshold, labels.m_pData, loop_iterations);
			free(clusters[0]);
			free(clusters);
		}
		else
		{
			int i, j, x, y, ww, hh;

			min_x = w-1;
			max_x = 0;
			min_y = h-1;
			max_y = 0;

			for (y = 0, i = 0; y<h; y++)
			{
				for (x = 0; x < w; x++, i++)
				{
					if (ImFF[i] != 0)
					{
						if (x < min_x) min_x = x;
						if (x > max_x) max_x = x;
						if (y < min_y) min_y = y;
						if (y > max_y) max_y = y;
					}
				}
			}
			min_x = std::max<int>(min_x - 2, 0);
			max_x = std::min<int>(max_x + 2, w-1);
			min_y = std::max<int>(min_y - 2, 0);
			max_y = std::min<int>(max_y + 2, h - 1);

			ww = max_x - min_x + 1;
			hh = max_y - min_y + 1;

			custom_buffer<int> ImRGBMASK(ww*hh, 0), labelsMASK(ww*hh, 0);
			int numObjsMASK = ww*hh;

			for (y = min_y, j = 0; y <= max_y; y++)
			{
				for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
				{
					ImRGBMASK[j] = ImRGB[i];

					if (ImFF[i] != 0)
					{
						labelsMASK[j] = color_cluster_id[ImRGBMASK[j] >> 8];
					}
					else
					{
						labelsMASK[j] = -1;
					}
				}
			}

			clusters = cuda_kmeans_img((unsigned char*)(ImRGBMASK.m_pData), numObjsMASK, numClusters, threshold, labelsMASK.m_pData, loop_iterations);
			free(clusters[0]);
			free(clusters);

			for (i = 0; i < ww*hh; i++)
			{
				color_cluster_id[ImRGBMASK[i] >> 8] = labelsMASK[i];
			}
			
			for (int i = 0; i < w*h; i++)
			{
				labels[i] = color_cluster_id[ImRGB[i] >> 8];
			}
		}
	}
#endif

	res = 1;
	return res;
}

int opencv_kmeans(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &labels, int w, int h, int numClusters, int loop_iterations, int initial_loop_iterations, int &min_x, int &max_x, int &min_y, int &max_y, bool mask_only = false)
{
	//custom_buffer<int> ImRGBFF(numObjs, 0), labelsFF(numObjs, 0);
	custom_buffer<char> color_cluster_id(1 << 24, 0);
	int numObjsFF;
	int i, j, color, res = 0;
	u8 *pClr;

	numObjsFF = 0;
	for (i = 0; i < w*h; i++)
	{
		if (ImFF[i] != 0)
		{
			numObjsFF++;
		}
	}

	if (numObjsFF < g_mpn)
	{
		return res;
	}
	
	cv::Mat cv_samplesFF(numObjsFF, 3, CV_32F);
	cv::Mat cv_labelsFF(numObjsFF, 1, CV_32S);	
	cv::Mat centers;

	j = 0;
	for (i = 0; i < w*h; i++)
	{
		if (ImFF[i] != 0)
		{
			pClr = (u8*)(&ImRGB[i]);

			for (int z = 0; z < 3; z++)
			{
				cv_samplesFF.at<float>(j, z) = pClr[z];
			}
			j++;
		}
	}

	if (numObjsFF > 0)
	{
		cv::theRNG().state = 0;
		cv::kmeans(cv_samplesFF, numClusters, cv_labelsFF, cv::TermCriteria(cv::TermCriteria::MAX_ITER, initial_loop_iterations, 1.0), 1, cv::KMEANS_PP_CENTERS, centers);
	}

	if (numObjsFF > 0)
	{
		j = 0;
		for (i = 0; i < w*h; i++)
		{
			if (ImFF[i] != 0)
			{
				color_cluster_id[ImRGB[i] >> 8] = cv_labelsFF.at<int>(j, 0);
				j++;
			}
		}
	}

	if (!mask_only)
	{
		cv::Mat cv_samples(w*h, 3, CV_32F);
		cv::Mat cv_labels(w*h, 1, CV_32S);

		for (i = 0; i < w*h; i++)
		{
			pClr = (u8*)(&ImRGB[i]);

			for (int z = 0; z < 3; z++)
			{
				cv_samples.at<float>(i, z) = pClr[z];
			}
		}

		if (numObjsFF > 0)
		{
			for (i = 0; i < w*h; i++)
			{
				cv_labels.at<int>(i, 0) = color_cluster_id[ImRGB[i] >> 8];				
			}
		}
		min_x = 0;
		max_x = w - 1;
		min_y = 0;
		max_y = h - 1;

		cv::theRNG().state = 0;
		cv::kmeans(cv_samples, numClusters, cv_labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER, loop_iterations, 1.0), 1, cv::KMEANS_USE_INITIAL_LABELS, centers);
		memcpy(&labels[0], cv_labels.data, w*h * sizeof(int));
	}
	else
	{
		if (numObjsFF == 0)
		{
			min_x = 0;
			max_x = w - 1;
			min_y = 0;
			max_y = h - 1;

			cv::Mat cv_samples(w*h, 3, CV_32F);
			cv::Mat cv_labels(w*h, 1, CV_32S);

			for (i = 0; i < w*h; i++)
			{
				pClr = (u8*)(&ImRGB[i]);

				for (int z = 0; z < 3; z++)
				{
					cv_samples.at<float>(i, z) = pClr[z];
				}
			}

			cv::theRNG().state = 0;
			cv::kmeans(cv_samples, numClusters, cv_labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER, loop_iterations, 1.0), 1, cv::KMEANS_PP_CENTERS, centers);
			memcpy(&labels[0], cv_labels.data, w*h * sizeof(int));
		}
		else
		{
			int i, j, x, y, ww, hh;

			min_x = w - 1;
			max_x = 0;
			min_y = h - 1;
			max_y = 0;

			for (y = 0, i = 0; y < h; y++)
			{
				for (x = 0; x < w; x++, i++)
				{
					if (ImFF[i] != 0)
					{
						if (x < min_x) min_x = x;
						if (x > max_x) max_x = x;
						if (y < min_y) min_y = y;
						if (y > max_y) max_y = y;
					}
				}
			}
			min_x = std::max<int>(min_x - 2, 0);
			max_x = std::min<int>(max_x + 2, w - 1);
			min_y = std::max<int>(min_y - 2, 0);
			max_y = std::min<int>(max_y + 2, h - 1);

			ww = max_x - min_x + 1;
			hh = max_y - min_y + 1;

			cv::Mat cv_samplesMASK(ww*hh, 3, CV_32F);
			cv::Mat cv_labelsMASK(ww*hh, 1, CV_32S);

			int numObjsMASK = ww * hh;

			for (y = min_y, j = 0; y <= max_y; y++)
			{
				for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
				{
					pClr = (u8*)(&ImRGB[i]);

					for (int z = 0; z < 3; z++)
					{
						cv_samplesMASK.at<float>(j, z) = pClr[z];
					}

					cv_labelsMASK.at<int>(j, 0) = color_cluster_id[ImRGB[i] >> 8];
				}
			}

			cv::theRNG().state = 0;
			cv::kmeans(cv_samplesMASK, numClusters, cv_labelsMASK, cv::TermCriteria(cv::TermCriteria::MAX_ITER, loop_iterations, 1.0), 1, cv::KMEANS_USE_INITIAL_LABELS, centers);

			for (y = min_y, j = 0; y <= max_y; y++)
			{
				for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
				{
					color_cluster_id[ImRGB[i] >> 8] = cv_labelsMASK.at<int>(j, 0);
				}
			}

			for (int i = 0; i < w*h; i++)
			{
				labels[i] = color_cluster_id[ImRGB[i] >> 8];
			}
		}
	}

	res = 1;
	return res;
}

void GetClustersImage(custom_buffer<int> &ImRES, custom_buffer<int> &labels, int clusterCount, int w, int h)
{
	for (int i = 0; i < w*h; i++)
	{
		int cluster_id = labels[i];

		if (cluster_id != -1)
		{
			ImRES[i] = GetClusterColor(clusterCount, cluster_id);
		}
		else
		{
			ImRES[i] = 0;
		}
	}
}

void SortClustersData(custom_buffer<int> &cluster_id, custom_buffer<int> &cluster_cnt, int clusterCount)
{
	int i, j, val;

	for (i = 0; i < clusterCount; i++)
	{
		cluster_id[i] = i;
	}

	for (i = 0; i < clusterCount - 1; i++)
	{
		for (j = i + 1; j < clusterCount; j++)
		{
			if (cluster_cnt[j] > cluster_cnt[i])
			{
				val = cluster_cnt[i];
				cluster_cnt[i] = cluster_cnt[j];
				cluster_cnt[j] = val;

				val = cluster_id[i];
				cluster_id[i] = cluster_id[j];
				cluster_id[j] = val;
			}
		}
	}
}

void SortClusters(custom_buffer<int> &labels, custom_buffer<int> &cluster_id, custom_buffer<int> &cluster_cnt, custom_buffer<int> &ImMASK, int clusterCount, int w, int h, int min_x, int max_x, int min_y, int max_y)
{
	int x, y, i, j, val;	

	for (i = 0; i < clusterCount; i++)
	{
		cluster_cnt[i] = 0;
	}

	for (y = min_y; y <= max_y; y++)
	{
		for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++)
		{
			if (ImMASK[i] != 0)
			{
				int cluster_idx = labels[i];

				if (cluster_idx != -1)
				{
					cluster_cnt[cluster_idx]++;
				}
			}
		}
	}

	SortClustersData(cluster_id, cluster_cnt, clusterCount);	
}

void GetInfoByLocation(custom_buffer<int> &Im, int &len, int &ww, int &max_section, int &cnts, int w, int h, int max_x, int white, std::string iter_det)
{
	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(Im, w, h, white, pFigures);
	int N = pFigures.size(), i, x, y, cnt;
	CMyClosedFigure *pFigure;
	custom_buffer<int> x_range(w, 0);

	int _min_x = w - 1;
	int _max_x = -1;
	len = 0;
	ww = 0;
	max_section = 0;
	cnts = 0;

	for (i = 0; i < N; i++)
	{
		pFigure = &(pFigures[i]);

		if (pFigure->m_minX <= max_x)
		{
			for (int x = pFigure->m_minX; x <= pFigure->m_maxX; x++) x_range[x] = 1;
			if (pFigure->m_minX < _min_x) _min_x = pFigure->m_minX;
			if (pFigure->m_maxX > _max_x) _max_x = pFigure->m_maxX;
			cnts += pFigure->m_Square;
		}
	}

	if (_max_x != -1)
	{
		len = std::min<int>(_max_x, max_x) - _min_x + 1;

		for (int x = _min_x; x <= std::min<int>(_max_x, max_x); x++)
		{
			if (x_range[x] == 1) ww++;
		}

		for (y = 0; y < h; y++)
		{
			cnt = 0;
			for (x = 0, i = y * w; x < std::min<int>(w, max_x + 1); x++, i++)
			{
				if (Im[i] == white)
				{
					cnt++;
				}
			}
			if (cnt > max_section) max_section = cnt;
		}
	}
}

void GetClustersInfoByLocation(custom_buffer<int> &ImMASKClusters, custom_buffer<int> &len, custom_buffer<int> &ww, custom_buffer<int> &max_section, custom_buffer<int> &cnts, int clusterCount, int w, int h, int max_x, std::string iter_det)
{
	//custom_buffer<int> len(clusterCount, 0), ww(clusterCount, 0), max_section(clusterCount, 0);

	concurrency::parallel_for(0, clusterCount, [&ImMASKClusters, &len, &ww, &max_section, &cnts, clusterCount, w, h, max_x, iter_det](int cluster_id)
	{
		int white = GetClusterColor(clusterCount, cluster_id);
		GetInfoByLocation(ImMASKClusters, len[cluster_id], ww[cluster_id], max_section[cluster_id], cnts[cluster_id], w, h, max_x, white, iter_det);
	});
}

void GetClustersInfoByPlacement(custom_buffer<int> &cluster_cnt, custom_buffer<int> &ImMASKClusters, int clusterCount, int w, int h, int min_x, int max_x, int min_y, int max_y, std::string iter_det)
{
	int x, y, i, j, val;	

	if (g_show_results) SaveGreyscaleImage(ImMASKClusters, "/TestImages/GetClustersInfoByPlacement_" + iter_det + "_ImMASKClusters" + g_im_save_format, w, h);

	for (i = 0; i < clusterCount; i++)
	{
		cluster_cnt[i] = 0;
	}

	if ((max_x < min_x) || (max_y < min_y)) return;

	concurrency::parallel_for(0, clusterCount, [&ImMASKClusters, &cluster_cnt, clusterCount, w, h, min_x, max_x, min_y, max_y, iter_det](int cluster_id)
	{		
		int i, j, l, ii, x, y, N, fmin_x, fmax_x, fmin_y, fmax_y;
		custom_buffer<int> ImCluster(w*h, 0), ImTMP(w*h, 0);
		int white = GetClusterColor(clusterCount, cluster_id);

		for (i = 0; i < w*h; i++)
		{
			if (ImMASKClusters[i] == white)
			{
				ImTMP[i] = 255;
			}
		}

		N = GetImageWithOutsideFigures(ImTMP, ImCluster, w, h, 255);		

		if (N > 0)
		{
			for (y = min_y; y <= max_y; y++)
			{
				for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++)
				{
					if (ImCluster[i] == 0)
					{
						cluster_cnt[cluster_id]++;
					}
				}
			}
		}

		if (g_show_results)
		{
			if (N > 0)
			{
				InvertBinaryImage(ImCluster, w, h);
			}

			SaveBinaryImage(ImCluster, "/TestImages/GetClustersInfoByPlacement_" + iter_det + "_id" + std::to_string(cluster_id) + "_ImClusterPlacement" + g_im_save_format, w, h);
			memset(&ImCluster[0], 0, w*h * sizeof(int));
			for (y = min_y; y <= max_y; y++)
			{
				for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++)
				{
					if (ImMASKClusters[i] == white)
					{
						ImCluster[i] = white;
					}
				}
			}
			SaveGreyscaleImage(ImCluster, "/TestImages/GetClustersInfoByPlacement_" + iter_det + "_id" + std::to_string(cluster_id) + "_ImCluster" + g_im_save_format, w, h);
		}
	});	
}

void GetBinaryImageByClusterId(custom_buffer<int> &ImRES, custom_buffer<int> &labels, int req_cluster_id, int w, int h)
{
	for (int i = 0; i < w*h; i++)
	{
		int cluster_id = labels[i];

		if (req_cluster_id == cluster_id)
		{
			ImRES[i] = 255;
		}
		else
		{
			ImRES[i] = 0;
		}
	}
}

void GetBinaryImageFromClustersImageByClusterId(custom_buffer<int> &ImRES, custom_buffer<int> &ImClusters, int cluster_id, int clusterCount, int w, int h)
{
	int white = GetClusterColor(clusterCount, cluster_id);

	for (int i = 0; i < w*h; i++)
	{
		if (ImClusters[i] == white)
		{
			ImRES[i] = 255;
		}
		else
		{
			ImRES[i] = 0;
		}
	}
}

void GetBinaryImageFromImageByColor(custom_buffer<int> &ImRES, custom_buffer<int> &ImGR, int white, int w, int h)
{
	for (int i = 0; i < w*h; i++)
	{
		if (ImGR[i] == white)
		{
			ImRES[i] = 255;
		}
		else
		{
			ImRES[i] = 0;
		}
	}
}

void RestoreImMask(custom_buffer<int> &ImClusters, custom_buffer<int> &ImMASKF, int clusterCount, int w, int h, std::string iter_det)
{	
	concurrency::parallel_for(0, clusterCount, [&ImClusters, &ImMASKF, clusterCount, w, h](int cluster_id)
	{
		custom_buffer<int> ImCluster(w*h, 0);
		custom_buffer<int> ImTMP(ImMASKF);

		GetBinaryImageFromClustersImageByClusterId(ImCluster, ImClusters, cluster_id, clusterCount, w, h);
		IntersectTwoImages(ImTMP, ImCluster, w, h);
		MergeImagesByIntersectedFigures(ImTMP, ImCluster, w, h, 255);
		CombineTwoImages(ImMASKF, ImTMP, w, h, 255);
	});
}

int FilterImMask(custom_buffer<int> &ImClusters, custom_buffer<int> &ImMASKF, custom_buffer<int> &labels, int clusterCount, int w, int h, int max_x, int &deleted_by_location, std::string iter_det)
{
	custom_buffer<int> ImRES4(w*h, 0);
	int val, i, j;
	double ww_thr = 0.5;
	double ww_in_thr = 0.7;
	double max_section_thr = 0.5;	
	double cnt_thr = 0.33;
	//double len_thr = 0.0;
	//double placement_cnt_thr = 0.2;
	int num_main_clusters;
	custom_buffer<int> ImInside(w*h, 0), ImInsideClose(w*h, 0), ImInside2(w*h, 0);
	custom_buffer<int> len(clusterCount, 0), ww(clusterCount, 0), max_section(clusterCount, 0), cluster_ids(clusterCount, 0), cnts(clusterCount, 0), placement_cnts(clusterCount, 0);
	int max_ww, max_max_section, max_len, max_cnt;

	cv::ocl::setUseOpenCL(g_use_ocl);

	deleted_by_location = 0;

	memcpy(&ImRES4[0], &ImClusters[0], w * h * sizeof(int));
	IntersectTwoImages(ImRES4, ImMASKF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImRES4, "/TestImages/FilterImMask_" + iter_det + "_01_ImClustersInImMASKF" + g_im_save_format, w, h);

	/*concurrency::parallel_invoke(
		[&ImRES4, &len, &ww, &max_section, &cnts, w, h, max_x, clusterCount, iter_det] {*/
			GetClustersInfoByLocation(ImRES4, len, ww, max_section, cnts, clusterCount, w, h, max_x, iter_det);
		/*},
		[&ImRES4, &placement_cnts, w, h, max_x, clusterCount, iter_det] {
			GetClustersInfoByPlacement(placement_cnts, ImRES4, clusterCount, w, h, 0, max_x, 0, h - 1, iter_det);
		}
	);*/

	for (int cluster_id = 0; cluster_id < clusterCount; cluster_id++)
	{
		cluster_ids[cluster_id] = cluster_id;
	}

	num_main_clusters = clusterCount;
	
	i = 0;
	while (i < num_main_clusters)
	{
		if (num_main_clusters == 1)
		{
			break;
		}

		int cluster_id = cluster_ids[i];
		int white = GetClusterColor(clusterCount, cluster_id);
		int len_in, ww_in, max_section_in, cnt_in;

		if (GetImageWithInsideFigures(ImRES4, ImInside, w, h, white, false) > 0)
		{
			custom_buffer<int> ImCluster(w*h, 0);
			GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_id, clusterCount, w, h);

			if (g_show_results)
			{
				SaveGreyscaleImage(ImRES4, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_01_ImClustersInImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_02_ImClusterInImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImInside, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_03_ImClusterInsideFigures" + g_im_save_format, w, h);
			}

			{
				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImCluster, w, h, cv_im_gr);
				cv::Mat kernel = cv::Mat::ones(3, 3, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 1);
				BinaryMatToImage(cv_im_gr, w, h, ImCluster, 255);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_04_ImClusterClose" + g_im_save_format, w, h);
			}

			if (GetImageWithInsideFigures(ImCluster, ImInsideClose, w, h, 255, false) > 0)
			{
				if (g_show_results) SaveGreyscaleImage(ImInsideClose, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_05_ImClusterCloseInsideFigures" + g_im_save_format, w, h);
				CombineTwoImages(ImInside, ImInsideClose, w, h, white);
				if (g_show_results) SaveGreyscaleImage(ImInside, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_06_ImClusterInsideFiguresCombine" + g_im_save_format, w, h);
			}

			IntersectTwoImages(ImInside, ImRES4, w, h);
			if (g_show_results) SaveGreyscaleImage(ImInside, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_07_01_ImClusterInsideFiguresIntImMASK" + g_im_save_format, w, h);

			GetInfoByLocation(ImInside, len_in, ww_in, max_section_in, cnt_in, w, h, max_x, white, iter_det);							

			if (ww_in >= ww[i] * ww_in_thr)
			{
				int rem_i = i;
				int cnt1 = 0, cnt2 = 0, cnt3 = 0;
				if (GetImageWithInsideFigures(ImInside, ImInside2, w, h, white, false) > 0)
				{
					IntersectTwoImages(ImInside2, ImRES4, w, h);
					if (g_show_results) SaveGreyscaleImage(ImInside2, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_07_02_ImClusterInside2FiguresIntImMASK" + g_im_save_format, w, h);

					for (j = 0; j < w*h; j++)
					{
						if (ImInside2[j] != 0)
						{
							cnt1++;

							if (ImRES4[j] == white)
							{
								cnt2++;
							}
						}

						if (ImInside[j] != 0)
						{
							cnt3++;
						}
					}
				}

				if ((cnt2 > cnt1*0.5) && (cnt2 > cnt3*0.5))
				{
					custom_buffer<int> cluster_ids_rem(clusterCount, 0);
					custom_buffer<int> cluster_cnt(clusterCount, 0);
					SortClusters(labels, cluster_ids_rem, cluster_cnt, ImInside, clusterCount, w, h, 0, max_x, 0, h - 1);

					rem_i = num_main_clusters;
					for (j = 0; j < num_main_clusters; j++)
					{
						if (cluster_ids[j] == cluster_ids_rem[0])
						{
							rem_i = j;
							break;
						}
					}
					
				}

				if (rem_i < num_main_clusters)
				{
					if (g_show_results)
					{
						GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[rem_i], clusterCount, w, h);
						SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_ids[rem_i]) + "_08_RemovingCluster" + g_im_save_format, w, h);
					}

					for (j=rem_i; j<num_main_clusters - 1; j++)
					{
						ww[j] = ww[j+1];
						max_section[j] = max_section[j+1];
						len[j] = len[j+1];
						cluster_ids[j] = cluster_ids[j+1];
						cnts[j] = cnts[j+1];
						placement_cnts[j] = placement_cnts[j+1];
					}					
					num_main_clusters--;

					if (rem_i > i)
					{
						i++;
					}

					continue;
				}
			}
		}		

		i++;
	}

	max_ww = ww.get_max_value(num_main_clusters);

	if (max_ww == 0)
	{
		return 0;
	}

	max_max_section = max_section.get_max_value(num_main_clusters);

	i = 0;
	while (i < num_main_clusters)
	{
		if ((ww[i] < max_ww * ww_thr)/* && (max_section[i] < max_max_section * std::max<double>(max_section_thr, ww_thr))*/)
		{
			if (g_show_results)
			{
				custom_buffer<int> ImCluster(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_03_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
			}

			if (ww[i] > 0) deleted_by_location++;
			if (i < num_main_clusters - 1)
			{
				ww[i] = ww[num_main_clusters - 1];
				max_section[i] = max_section[num_main_clusters - 1];
				len[i] = len[num_main_clusters - 1];
				cluster_ids[i] = cluster_ids[num_main_clusters - 1];
				cnts[i] = cnts[num_main_clusters - 1];
				placement_cnts[i] = placement_cnts[num_main_clusters - 1];
			}
			num_main_clusters--;
			continue;
		}
		i++;
	}	

	max_max_section = max_section.get_max_value(num_main_clusters);

	i = 0;
	while (i < num_main_clusters)
	{
		if (max_section[i] < max_max_section * max_section_thr)
		{
			if (g_show_results)
			{
				custom_buffer<int> ImCluster(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_04_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
			}

			if (ww[i] > 0) deleted_by_location++;
			if (i < num_main_clusters - 1)
			{
				ww[i] = ww[num_main_clusters - 1];
				max_section[i] = max_section[num_main_clusters - 1];
				len[i] = len[num_main_clusters - 1];
				cluster_ids[i] = cluster_ids[num_main_clusters - 1];
				cnts[i] = cnts[num_main_clusters - 1];
				placement_cnts[i] = placement_cnts[num_main_clusters - 1];
			}
			num_main_clusters--;
			continue;
		}
		i++;
	}

	/*max_len = len.get_max_value(num_main_clusters);
	i = 0;
	while (i < num_main_clusters)
	{
		if (len[i] < max_len * len_thr)
		{
			if (ww[i] > 0) deleted_by_location++;
			if (i < num_main_clusters - 1)
			{
				ww[i] = ww[num_main_clusters - 1];
				max_section[i] = max_section[num_main_clusters - 1];
				len[i] = len[num_main_clusters - 1];
				cluster_ids[i] = cluster_ids[num_main_clusters - 1];
				cnts[i] = cnts[num_main_clusters - 1];
				placement_cnts[i] = placement_cnts[num_main_clusters - 1];
			}
			num_main_clusters--;
			continue;
		}
		i++;
	}*/

	max_cnt = cnts.get_max_value(num_main_clusters);
	i = 0;
	while (i < num_main_clusters)
	{
		if (cnts[i] < max_cnt * cnt_thr)
		{
			if (g_show_results)
			{
				custom_buffer<int> ImCluster(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_05_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
			}

			if (ww[i] > 0) deleted_by_location++;
			if (i < num_main_clusters - 1)
			{
				ww[i] = ww[num_main_clusters - 1];
				max_section[i] = max_section[num_main_clusters - 1];
				len[i] = len[num_main_clusters - 1];
				cluster_ids[i] = cluster_ids[num_main_clusters - 1];
				cnts[i] = cnts[num_main_clusters - 1];
				placement_cnts[i] = placement_cnts[num_main_clusters - 1];
			}
			num_main_clusters--;
			continue;
		}
		i++;
	}

	/*max_cnt = placement_cnts.get_max_value(num_main_clusters);
	i = 0;
	while (i < num_main_clusters)
	{
		if (placement_cnts[i] < max_cnt * placement_cnt_thr)
		{
			if (g_show_results)
			{
				custom_buffer<int> ImCluster(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/TestImages/FilterImMask_" + iter_det + "_05_id" + std::to_string(cluster_ids[i]) + "_02_RemovingCluster" + g_im_save_format, w, h);
			}

			if (ww[i] > 0) deleted_by_location++;
			if (i < num_main_clusters - 1)
			{
				ww[i] = ww[num_main_clusters - 1];
				max_section[i] = max_section[num_main_clusters - 1];
				len[i] = len[num_main_clusters - 1];
				cluster_ids[i] = cluster_ids[num_main_clusters - 1];
				cnts[i] = cnts[num_main_clusters - 1];
				placement_cnts[i] = placement_cnts[num_main_clusters - 1];
			}
			num_main_clusters--;
			continue;
		}
		i++;
	}	*/

	for (i = 0; i < w*h; i++)
	{
		if (ImRES4[i] != 0)
		{
			int cluster_id = labels[i];
			bool is_main_cluster = false;

			for (j = 0; j < num_main_clusters; j++)
			{
				if (cluster_id == cluster_ids[j])
				{
					is_main_cluster = true;
					break;
				}
			}

			if (!is_main_cluster)
			{
				ImRES4[i] = 0;
			}
		}
	}

	GreyscaleImageToBinary(ImMASKF, ImRES4, w, h, 255);
	if (g_show_results)
	{
		SaveGreyscaleImage(ImMASKF, "/TestImages/FilterImMask_" + iter_det + "_06_01_ImMASKFIntMainClustersByLocation" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImRES4, "/TestImages/FilterImMask_" + iter_det + "_06_02_ImClustersInImMASKF" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImClusters, "/TestImages/FilterImMask_" + iter_det + "_06_03_ImClustersOrig" + g_im_save_format, w, h);
	}

	/*if (num_main_clusters == 1)
	{
		return num_main_clusters;
	}

	int num_main_clusters_orig = num_main_clusters;

	if (num_main_clusters_orig != num_main_clusters)
	{
		for (i = 0; i < w*h; i++)
		{
			if (ImRES4[i] != 0)
			{
				int cluster_id = labels[i];
				bool is_main_cluster = false;

				for (j = 0; j < num_main_clusters; j++)
				{
					if (cluster_id == cluster_ids[j])
					{
						is_main_cluster = true;
						break;
					}
				}

				if (!is_main_cluster)
				{
					ImRES4[i] = 0;
				}
			}
		}

		GreyscaleImageToBinary(ImMASKF, ImRES4, w, h, 255);
		if (g_show_results)
		{
			SaveGreyscaleImage(ImMASKF, "/TestImages/FilterImMask_" + iter_det + "_06_ImMASKFIntMainClustersByLocation" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImRES4, "/TestImages/FilterImMask_" + iter_det + "_07_ImClustersInImMASKF" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImClusters, "/TestImages/FilterImMask_" + iter_det + "_08_ImClustersOrig" + g_im_save_format, w, h);
		}
	}*/

	return num_main_clusters;
}

void DistanceTransformImage(custom_buffer<int> &ImGR, custom_buffer<int> &ImRES, int w, int h)
{
	cv::ocl::setUseOpenCL(g_use_ocl);

	cvMAT cv_im_gr;
	GreyscaleImageToMat(ImGR, w, h, cv_im_gr);

	cv::distanceTransform(cv_im_gr, cv_im_gr, cv::DIST_C, 5);

	cv::normalize(cv_im_gr, cv_im_gr, 0, 255, cv::NORM_MINMAX, CV_8UC1);
	//cv::imshow("Distance Transform Image: " + iter_det, dist);

	cv::threshold(cv_im_gr, cv_im_gr, 30, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
	//cv::imshow("Distance Transform Image Thresh: " + iter_det, dist);
	//cv::waitKey(0);

	BinaryMatToImage(cv_im_gr, w, h, ImRES, 255);
	IntersectTwoImages(ImRES, ImGR, w, h);
}

int GetFirstFilteredClusters(custom_buffer<int> &ImRGB, custom_buffer<int> &ImMASK, custom_buffer<int> &ImMASK2, custom_buffer<int> &ImMaskWithBorder, custom_buffer<int> &ImMaskWithBorderF, custom_buffer<int> &ImClusters2, custom_buffer<int> &labels2, int clusterCount2, int w, int h, std::string iter_det)
{
	int min_x, max_x, min_y, max_y;
	int res = 0;

	if (g_use_cuda_gpu)
	{
		res = cuda_kmeans(ImRGB, ImMASK, labels2, w, h, clusterCount2, g_cuda_kmeans_loop_iterations, g_cuda_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, false);
	}
	else
	{
		res = opencv_kmeans(ImRGB, ImMASK, labels2, w, h, clusterCount2, g_cpu_kmeans_loop_iterations, g_cpu_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, false);
	}

	if (res == 0)
	{
		return res;
	}

	GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
	if (g_show_results) SaveGreyscaleImage(ImClusters2, "/TestImages/GetFirstFilteredClusters_" + iter_det + "_01_ImClusters" + g_im_save_format, w, h);

	{
		concurrency::parallel_for(0, clusterCount2, [&ImClusters2, clusterCount2, w, h](int cluster_idx)
		{
			ClearImageOptimal(ImClusters2, w, h, GetClusterColor(clusterCount2, cluster_idx));
		});

		GreyscaleImageToBinary(ImMaskWithBorder, ImClusters2, w, h, 255);

		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/TestImages/GetFirstFilteredClusters_" + iter_det + "_02_ImMaskWithBorder" + g_im_save_format, w, h);
	}

	memcpy(&ImMaskWithBorderF[0], &ImMaskWithBorder[0], w * h * sizeof(int));

	{
		concurrency::parallel_for(0, clusterCount2, [&ImMASK2, &ImClusters2, clusterCount2, w, h](int cluster_idx)
		{
			ClearImageByMask(ImClusters2, ImMASK2, w, h, GetClusterColor(clusterCount2, cluster_idx));
		});

		GreyscaleImageToBinary(ImMaskWithBorderF, ImClusters2, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorderF, "/TestImages/GetFirstFilteredClusters_" + iter_det + "_03_ImMaskWithBorderFByMASK2" + g_im_save_format, w, h);
	}

	res = 1;
	return res;
}

void ClearMainClusterImage(custom_buffer<int> &ImMainCluster, custom_buffer<int> ImMASK, custom_buffer<int> &ImIL, int w, int h, int W, int H, int LH, std::string iter_det)
{
	cv::ocl::setUseOpenCL(g_use_ocl);

	if (g_use_ILA_images_for_clear_txt_images && (!g_use_gradient_images_for_clear_txt_images) && (ImIL[0] != -1))
	{
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/ClearMainClusterImage_" + iter_det + "_01_01_ImMainCluster" + g_im_save_format, w, h);
		ClearImageByMask(ImMainCluster, ImIL, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/ClearMainClusterImage_" + iter_det + "_01_02_ImMainClusterFWithImIL" + g_im_save_format, w, h);		
	}
	else if (g_use_gradient_images_for_clear_txt_images)
	{
		
		{
			custom_buffer<int> ImTMP(w*h, 0);
			cvMAT cv_im_gr;
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_02_01_ImMASK" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/ClearMainClusterImage_" + iter_det + "_02_02_ImMainCluster" + g_im_save_format, w, h);
			BinaryImageToMat(ImMainCluster, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), std::max<int>(1, LH/10));
			BinaryMatToImage(cv_im_gr, w, h, ImTMP, 255);
			if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/ClearMainClusterImage_" + iter_det + "_02_03_ImMainClusterDilate" + g_im_save_format, w, h);			
			IntersectTwoImages(ImMASK, ImTMP, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_02_04_ImMASKIntImMainCluster" + g_im_save_format, w, h);
		}

		if ((g_use_ILA_images_for_clear_txt_images) && (ImIL[0] != -1))
		{
			custom_buffer<int> ImTMP(w*h, 0);
			cvMAT cv_im_gr;
			if (g_show_results) SaveGreyscaleImage(ImIL, "/TestImages/ClearMainClusterImage_" + iter_det + "_03_01_ImIL" + g_im_save_format, w, h);
			BinaryImageToMat(ImIL, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), std::max<int>(1, LH / 10));
			BinaryMatToImage(cv_im_gr, w, h, ImTMP, 255);
			if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/ClearMainClusterImage_" + iter_det + "_03_02_ImILDilate" + g_im_save_format, w, h);
			IntersectTwoImages(ImMASK, ImTMP, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_03_03_ImMASKIntImIL" + g_im_save_format, w, h);
		}

		{
			cvMAT cv_im_gr;
			GreyscaleImageToMat(ImMASK, w, h, cv_im_gr);
			cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
			cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
			BinaryMatToImage(cv_im_gr, w, h, ImMASK, 255);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_04_ImMASKClose" + g_im_save_format, w, h);
		}

		ExtendByInsideFigures(ImMASK, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_05_ImMASKExtendByInsideFigures" + g_im_save_format, w, h);

		IntersectTwoImages(ImMASK, ImMainCluster, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/ClearMainClusterImage_" + iter_det + "_06_ImMASKIntImMainCluster" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/ClearMainClusterImage_" + iter_det + "_07_01_ImMainCluster" + g_im_save_format, w, h);
		ClearImageByMask(ImMainCluster, ImMASK, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/ClearMainClusterImage_" + iter_det + "_07_02_ImMainClusterFWithImMASK" + g_im_save_format, w, h);
	}
}

int GetMainClusterImage(custom_buffer<int> &ImRGB, custom_buffer<int> ImMASK, custom_buffer<int> ImMASK2, custom_buffer<int> &ImIL, custom_buffer<int> &ImRES, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, custom_buffer<int> &ImMaskWithBorder, custom_buffer<int> &ImMaskWithBorderF, custom_buffer<int> &ImClusters2, custom_buffer<int> &labels2, int w, int h, int W, int H, std::string iter_det, int real_im_x_center, int min_h)
{
	custom_buffer<int> ImRES1(w*h, 0), ImRES2(w*h, 0), ImRES3(w*h, 0), ImRES4(w*h, 0), ImMainCluster(w*h, 0), ImMainCluster2(w*h, 0);
	int x, y, i, j, j2, val1, val2;
	int clusterCount2 = 6;
	custom_buffer<int> cluster_cnt2(clusterCount2, 0);
	custom_buffer<int> cluster_id2(clusterCount2, 0);
	int clusterCount3 = 4;
	custom_buffer<int> cluster_cnt3(clusterCount3, 0);
	custom_buffer<int> cluster_id3(clusterCount3, 0);
	DWORD  start_time;
	int ddy1 = 1, ddy2 = h - 2;
	int LH, LMAXY, lb, le, val, res = 0;

	cv::ocl::setUseOpenCL(g_use_ocl);

	if (g_show_results) SaveRGBImage(ImRGB, "/TestImages/GetMainClusterImage_" + iter_det + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_01_2_ImMASK" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_01_3_ImMASK2" + g_im_save_format, w, h);

	memset(&ImRES[0], 0, (w*h) * sizeof(int));	

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_02_04_02_ImMASK" + g_im_save_format, w, h);
	IntersectTwoImages(ImMASK, ImMaskWithBorderF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_02_04_03_ImMASKIntImMaskWithBorderF" + g_im_save_format, w, h);

	{				
		concurrency::parallel_for(0, clusterCount2, [&ImClusters2, clusterCount2, w, h, W, H, min_h, real_im_x_center, &ImMASK, iter_det](int cluster_idx)
		//for (int cluster_idx=0; cluster_idx<clusterCount2; cluster_idx++)
		{
			int i, val, LH, LMAXY, lb, le, white = GetClusterColor(clusterCount2, cluster_idx);
			custom_buffer<int> ImTMP(ImMASK);

			for (i=0; i<w*h; i++)
			{
				if (ImClusters2[i] != white)
				{
					ImTMP[i] = 0;
				}
			}

			if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_01_ImMASKIntCluster" + g_im_save_format, w, h);

			val = GetSubParams(ImTMP, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);
			if (val == 1)
			{
				if (g_show_results) SaveImageWithSubParams(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_02_ImMASKIntCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt2(ImClusters2, w, h, W, H, LH, LMAXY, real_im_x_center, white, iter_det + "_02_05_01_id" + std::to_string(cluster_idx));
				if (g_show_results)
				{
					GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters2, cluster_idx, clusterCount2, w, h);
					IntersectTwoImages(ImTMP, ImMASK, w, h);
					SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_03_ImMASKIntClusterFOpt2" + g_im_save_format, w, h);
				}
			}			
		});

		GreyscaleImageToBinary(ImMaskWithBorderF, ImClusters2, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorderF, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_02_ImMaskWithBorderFByOpt2Res" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_03_ImMASK" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK, ImMaskWithBorderF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_02_05_04_ImMASKIntImMaskWithBorder" + g_im_save_format, w, h);
	}	

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_01_ImMASK" + g_im_save_format, w, h);

	RestoreImMask(ImClusters2, ImMASK, clusterCount2, w, h, iter_det + "_call1");	
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_04_01_ImMASKRestore" + g_im_save_format, w, h);	

	ClearImageByTextDistance(ImMASK, w, h, W, H, real_im_x_center, 255, iter_det);
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_04_02_ImMASKByTextDistance" + g_im_save_format, w, h);

	if (g_show_results)
	{
		SaveGreyscaleImage(ImClusters2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_05_ImClusters2" + g_im_save_format, w, h);
		memcpy(&ImRES4[0], &ImClusters2[0], w * h * sizeof(int));
		IntersectTwoImages(ImRES4, ImMASK, w, h);
		SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_06_ImClusters2InImMASK" + g_im_save_format, w, h);
	}
	
	int num_main_clusters = 0;
	int id_last_main_cluster = 0;
	custom_buffer<int> cnts(clusterCount2, 0);

	for (i = 0; i < w*h; i++)
	{
		if ((ImMASK[i] != 0) && (ImClusters2[i] != 0))
		{
			int cluster_id = GetClusterId(clusterCount2, ImClusters2[i]);
			cnts[cluster_id]++;
		}
	}

	for (i = 0; i < clusterCount2; i++)
	{
		if (cnts[i] > 0)
		{
			num_main_clusters++;
			id_last_main_cluster = i;
		}
	}
	
	if (num_main_clusters == 0)
	{
		memset(&ImRES[0], 0, (w*h) * sizeof(int));
		memset(&ImMASK[0], 0, (w*h) * sizeof(int));
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_01_01_07_ImMASKRes" + g_im_save_format, w, h);
		return res;
	}	

	// for save punctuation marks and on top symbols we need to get image with decreased number of clusters
	{
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_01_ImMASK" + g_im_save_format, w, h);
		custom_buffer<int> labels3(w*h, 0);
		custom_buffer<int> ImClusters3(w*h, 0);
		custom_buffer<int> ImMASKR(w*h, 0), ImMASKR2(w*h, 0);
		int min_x, max_x, min_y, max_y;

		if (g_use_cuda_gpu)
		{
			res = cuda_kmeans(ImRGB, ImMASK, labels3, w, h, clusterCount3, g_cuda_kmeans_loop_iterations, g_cuda_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, true);
		}
		else
		{
			res = opencv_kmeans(ImRGB, ImMASK, labels3, w, h, clusterCount3, g_cpu_kmeans_loop_iterations, g_cpu_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, true);
		}

		if (res == 0)
		{
			return res;
		}

		GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_02_ImClusters3" + g_im_save_format, w, h);

		concurrency::parallel_for(0, clusterCount3, [&ImClusters3, clusterCount3, w, h, min_h, real_im_x_center, min_x, max_x, min_y, max_y, iter_det](int cluster_idx)
		{
			int white = GetClusterColor(clusterCount3, cluster_idx);
			if (g_show_results)
			{
				custom_buffer<int> ImTMP(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
				SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_03_id" + std::to_string(cluster_idx) + "_01_ImCluster" + g_im_save_format, w, h);
			}
			ClearImageOptimal2(ImClusters3, w, h, min_x, max_x, min_y, max_y, white);
			if (g_show_results)
			{
				custom_buffer<int> ImTMP(w*h, 0);
				GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
				SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_03_id" + std::to_string(cluster_idx) + "_02_ImClusterFOptimal2" + g_im_save_format, w, h);
			}
		});
		if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_01_ImClusters3FOptimal2Res" + g_im_save_format, w, h);

		//CombineTwoImages(ImMaskWithBorder, ImClusters3, w, h, 255);
		//if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_02_ImMaskWithBorderCombinedWithImClusters3" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_02_ImMASK2" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK2, ImMaskWithBorder, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_03_ImMASK2IntImMaskWithBorder" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK2, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_04_ImMASK2IntImClusters3" + g_im_save_format, w, h);

		{
			cvMAT cv_im_gr;
			GreyscaleImageToMat(ImMASK2, w, h, cv_im_gr);
			cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
			cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
			BinaryMatToImage(cv_im_gr, w, h, ImMASK2, 255);
			if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_05_ImMASK2Close" + g_im_save_format, w, h);
		}

		ExtendByInsideFigures(ImMASK2, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_06_ImMASK2ExtendByInsideFigures" + g_im_save_format, w, h);

		IntersectTwoImages(ImMASK2, ImMaskWithBorder, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_07_ImMASK2IntImMaskWithBorder" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK2, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_08_ImMASK2IntImClusters3" + g_im_save_format, w, h);

		if (ImIL[0] != -1)
		{
			IntersectTwoImages(ImMASK2, ImIL, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_04_09_ImMASK2IntImIL" + g_im_save_format, w, h);
		}

		{
			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_05_01_ImClusters3" + g_im_save_format, w, h);

			concurrency::parallel_for(0, clusterCount3, [&ImMASK2, &ImClusters3, clusterCount3, w, h](int cluster_idx)
			{
				ClearImageByMask(ImClusters3, ImMASK2, w, h, GetClusterColor(clusterCount3, cluster_idx));
			});

			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_05_02_ImClusters3FWithImMASK2" + g_im_save_format, w, h);
		}

		//IntersectTwoImages(ImClusters3, ImMaskWithBorder, w, h);
		//if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_05_ImClusters3IntImMaskWithBorder" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_06_01_ImMASK" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_06_02_ImMASKIntImClusters3" + g_im_save_format, w, h);

		ClearImageByTextDistance(ImMASK, w, h, W, H, real_im_x_center, 255, iter_det);
		if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_07_ImMASKByTextDistance" + g_im_save_format, w, h);

		//RestoreImMask(ImClusters3, ImMASK, clusterCount3, w, h, iter_det + "_call2");
		//if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_02_08_ImMASKRestore" + g_im_save_format, w, h);

		custom_buffer<int> ImClusters3FR(ImClusters3);

		/*concurrency::parallel_for(0, clusterCount3, [&ImClusters3, clusterCount3, w, h](int cluster_idx)
		{
			int white = GetClusterColor(clusterCount3, cluster_idx);
			ExtendByInsideFigures(ImClusters3, w, h, white, false);
		});
		if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_01_ImClusters3ExtendByInsideFigures" + g_im_save_format, w, h);

		GreyscaleImageToBinary(ImMASK, ImClusters3, w, h, 255);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_02_ImMASKRes" + g_im_save_format, w, h);*/

		int sub_iter = 0;

		while (1)
		{
			sub_iter++;

			IntersectTwoImages(ImClusters3, ImMASK, w, h);
			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_01_01_ImClusters3IntImMASK" + g_im_save_format, w, h);

			if (sub_iter == 1)
			{
				val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);

				if (val == 1)
				{
					if (g_show_results) SaveImageWithSubParams(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_01_02_ImMASK_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);

					concurrency::parallel_for(0, clusterCount3, [&ImClusters3, clusterCount3, w, h, W, H, lb, le, LH, LMAXY, min_h, real_im_x_center, iter_det, sub_iter](int cluster_idx)
					{
						int white = GetClusterColor(clusterCount3, cluster_idx);
						if (g_show_results)
						{
							custom_buffer<int> ImTMP(w*h, 0);
							GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
							SaveImageWithSubParams(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_01_ImCluster" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
						}
						ClearImageOpt4(ImClusters3, w, h, W, H, LH, LMAXY, real_im_x_center, white);
						if (g_show_results)
						{
							custom_buffer<int> ImTMP(w*h, 0);
							GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
							SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_02_ImClusterFOptimal4" + g_im_save_format, w, h);
						}

						int val, LH2, LMAXY2, lb2, le2;
						val = GetSubParams(ImClusters3, w, h, white, LH2, LMAXY2, lb2, le2, min_h, real_im_x_center, 0, h - 1, iter_det);

						if (val == 1)
						{
							if (g_show_results)
							{
								custom_buffer<int> ImTMP(w*h, 0);
								GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
								SaveImageWithSubParams(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_03_ImCluster_WSP" + g_im_save_format, lb2, le2, LH2, LMAXY2, real_im_x_center, w, h);
							}

							ClearImageOpt4(ImClusters3, w, h, W, H, LH2, LMAXY2, real_im_x_center, white);
							if (g_show_results)
							{
								custom_buffer<int> ImTMP(w*h, 0);
								GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
								SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_04_ImClusterFOptimal4Second" + g_im_save_format, w, h);
							}
						}
						else
						{
							// remove entire cluster
							if (g_show_results)
							{
								custom_buffer<int> ImTMP(w*h, 0);
								GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
								SaveGreyscaleImage(ImTMP, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_05_ImClusterRemove" + g_im_save_format, w, h);
							}

							for (int i = 0; i < w*h; i++)
							{
								if (ImClusters3[i] == white)
								{
									ImClusters3[i] = 0;
								}
							}

						}
					});
					if (g_show_results) SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_04_ImClusters3FOptimal4Res" + g_im_save_format, w, h);

					if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_05_01_ImMASK" + g_im_save_format, w, h);
					IntersectTwoImages(ImMASK, ImClusters3, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_05_02_ImMASKOpt4" + g_im_save_format, w, h);
				}

				memcpy(&ImMASKR2[0], &ImMASK[0], w * h * sizeof(int));
			}

			if (g_use_simple)
			{
				num_main_clusters = 1;
				break;
			}

			val = num_main_clusters;
			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, w - 1, deleted_by_location, iter_det + "_call1_sub_iter" + std::to_string(sub_iter));

			if ((num_main_clusters > 1) && (deleted_by_location > 0))
			{
				num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, w - 1, deleted_by_location, iter_det + "_call2_sub_iter" + std::to_string(sub_iter));
			}

			if (g_show_results)
			{
				SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_03_ImMASKIntImClusters3FByLocation" + g_im_save_format, w, h);
				memcpy(&ImRES4[0], &ImClusters3[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES4, ImMASK, w, h);
				SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_04_ImClusters3InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImClusters3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_05_01_ImClusters3Orig" + g_im_save_format, w, h);
			}

			if (num_main_clusters < 2)
			{
				break;
			}
			else
			{
				if (num_main_clusters > 0)
				{
					memcpy(&ImMASKR[0], &ImMASK[0], w * h * sizeof(int));
				}

				if (sub_iter == 3)
				{
					break;
				}

				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImMASK, w, h, cv_im_gr);

				cv::Mat kernel = cv::Mat::ones(5, 5, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

				if (g_show_results)
				{
					BinaryMatToImage(cv_im_gr, w, h, ImRES1, 255);
					if (g_show_results) SaveGreyscaleImage(ImRES1, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_16_ImMASKClose" + g_im_save_format, w, h);
				}

				int erosion_size = 1, erosion_type = 0;
				cv::Mat element = cv::getStructuringElement(erosion_type,
					cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
					cv::Point(erosion_size, erosion_size));

				cv::erode(cv_im_gr, cv_im_gr, element, cv::Point(-1, -1), 2);
				//cv::imshow("ImErode: " + iter_det, cv_im_gr);

				if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_17_ImMASK" + g_im_save_format, w, h);
				BinaryMatToImage(cv_im_gr, w, h, ImRES1, 255);
				IntersectTwoImages(ImMASK, ImRES1, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_18_ImMASKErode" + g_im_save_format, w, h);
			}
		}

		if (num_main_clusters > 1)
		{
			memcpy(&ImMASK[0], &ImMASKR[0], w * h * sizeof(int));
			memcpy(&ImClusters3[0], &ImClusters3FR[0], w * h * sizeof(int));
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_01_ImMASK" + g_im_save_format, w, h);
			RestoreImMask(ImClusters3, ImMASK, clusterCount3, w, h, iter_det + "_call3");
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_02_01_ImMASKRestore" + g_im_save_format, w, h);
			IntersectTwoImages(ImMASK, ImMASKR2, w, h);
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_02_02_ImMASKFImMASKR2" + g_im_save_format, w, h);

			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, w - 1, deleted_by_location, iter_det + "_call3");

			if ((num_main_clusters > 1) && (deleted_by_location > 0))
			{
				num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, w - 1, deleted_by_location, iter_det + "_call4");
			}

			if (g_show_results)
			{
				SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_03_ImMASKIntImClusters3FByLocation" + g_im_save_format, w, h);
				memcpy(&ImRES4[0], &ImClusters3[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES4, ImMASK, w, h);
				SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_04_ImClusters3InImMASK" + g_im_save_format, w, h);
			}
		}

		if (num_main_clusters > 1)
		{
			SortClusters(labels2, cluster_id2, cluster_cnt2, ImMASK, clusterCount2, w, h, 0, std::min<int>(w, real_im_x_center) - 1, 0, h - 1);
			GetBinaryImageByClusterId(ImMainCluster, labels2, cluster_id2[0], w, h);
			GetBinaryImageByClusterId(ImMainCluster2, labels2, cluster_id2[1], w, h);

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_11_02_ImMainClusterInImClusters2" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_11_03_ImMainCluster2InImClusters2" + g_im_save_format, w, h);

			memcpy(&ImRES2[0], &ImMainCluster[0], w * h * sizeof(int));
			CombineTwoImages(ImRES2, ImMainCluster2, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImRES2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_12_01_Im2MainClustersInImClusters2" + g_im_save_format, w, h);

			memcpy(&ImRES4[0], &ImRES2[0], w * h * sizeof(int));
			IntersectTwoImages(ImRES4, ImMASK, w, h);
			if (g_show_results) SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_12_02_Im2MainClustersInImClusters2IntWithImMASK" + g_im_save_format, w, h);
			{
				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImRES4, w, h, cv_im_gr);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 8);
				BinaryMatToImage(cv_im_gr, w, h, ImRES4, 255);
				if (g_show_results) SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_12_03_Im2MainClustersInImClusters2Dilate" + g_im_save_format, w, h);
			}
			IntersectTwoImages(ImRES4, ImRES2, w, h);
			GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
			IntersectTwoImages(ImClusters2, ImRES4, w, h);

			if (g_show_results)
			{
				SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_12_04_Im2MainClustersInImClusters2IntOrig2MainClustersInImClusters2" + g_im_save_format, w, h);
				memcpy(&ImRES3[0], &ImClusters2[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES3, ImRES4, w, h);
				SaveGreyscaleImage(ImRES3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_13_ImClusters2InIm2MainClustersInImClusters2IntOrig2MainClustersInImClusters2" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImClusters2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_14_ImClusters2" + g_im_save_format, w, h);
			}

			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters2, ImRES4, labels2, clusterCount2, w, h, w - 1, deleted_by_location, iter_det + "_call5");

			if (num_main_clusters < 2)
			{
				IntersectTwoImages(ImMASK, ImRES4, w, h);
			}
			else
			{
				//IntersectTwoImages(ImMASK, ImMainCluster, w, h);
				//num_main_clusters = 1;

				//memcpy(&ImMASK[0], &ImRES4[0], w * h * sizeof(int));

				int len1 = 0, len2 = 0;
				int cnt1 = cluster_cnt2[0], cnt2 = cluster_cnt2[1];

				memcpy(&ImRES2[0], &ImMainCluster[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES2, ImRES4, w, h);
				memcpy(&ImRES3[0], &ImMainCluster2[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES3, ImRES4, w, h);

				if (g_show_results) SaveGreyscaleImage(ImRES2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_15_01_ImMainClusterInImClusters2IntImMASK" + g_im_save_format, w, h);
				if (g_show_results) SaveGreyscaleImage(ImRES3, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_01_15_02_ImMainCluster2InImClusters2IntImMASK" + g_im_save_format, w, h);

				{
					cvMAT cv_im;
					vector<vector<cv::Point> > contours;
					vector<cv::Vec4i> hierarchy;

					BinaryImageToMat(ImRES2, w, h, cv_im);
					cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
					for (j = 0; j < contours.size(); j++) len1 += cv::arcLength(contours[j], true);
					//len1 = cv::arcLength(contours, true);
				}

				{
					cvMAT cv_im;
					vector<vector<cv::Point> > contours;
					vector<cv::Vec4i> hierarchy;

					BinaryImageToMat(ImRES3, w, h, cv_im);
					cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
					for (j = 0; j < contours.size(); j++) len2 += cv::arcLength(contours[j], true);
					//len2 = cv::arcLength(contours, true);
				}

				if ((len1 > len2*1.2) && (cnt2 >= cnt1 * 0.4))
				{
					//memcpy(&ImMainCluster[0], &ImMainCluster2[0], w * h * sizeof(int));
					IntersectTwoImages(ImMASK, ImRES3, w, h);
					num_main_clusters = 1;
				}
				else
				{
					IntersectTwoImages(ImMASK, ImRES2, w, h);
					num_main_clusters = 1;
				}
			}
		}
		
		if (num_main_clusters == 1)
		{
			//memcpy(&ImRES1[0], &ImMASK[0], w * h * sizeof(int));
			//GetMainColorImage(ImMASK, NULL, ImRES1, ImY, ImU, ImV, w, h, iter_det + "_call1_01", real_im_x_center);			

			if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_02_01_ImMASK" + g_im_save_format, w, h);

			SortClusters(labels3, cluster_id3, cluster_cnt3, ImMASK, clusterCount3, w, h, 0, std::min<int>(w, real_im_x_center) - 1, 0, h - 1);
			GetBinaryImageByClusterId(ImMainCluster, labels3, cluster_id3[0], w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_02_02_ImMainClusterInImClusters3" + g_im_save_format, w, h);

			/*val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);
			if (val == 1)
			{
				if (g_show_results) SaveImageWithSubParams(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_03_ImMASK_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt4(ImMASK, w, h, W, H, LH, LMAXY, real_im_x_center, 255);
				if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_04_04_ImMASKOpt4" + g_im_save_format, w, h);
			}*/

			SortClusters(labels2, cluster_id2, cluster_cnt2, ImMASK, clusterCount2, w, h, 0, std::min<int>(w, real_im_x_center) - 1, 0, h - 1);
			GetBinaryImageByClusterId(ImMainCluster2, labels2, cluster_id2[0], w, h);

			if (g_show_results)
			{
				SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_03_08_ImMASK" + g_im_save_format, w, h);
				memcpy(&ImRES4[0], &ImClusters2[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES4, ImMASK, w, h);
				SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_03_09_01_ImClusters2InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_09_02_ImMainClusterInImClusters2" + g_im_save_format, w, h);
			}

			concurrency::parallel_invoke(
				[&ImMainCluster, &ImMaskWithBorder, w, h, iter_det] {
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_01_ImMainCluster" + g_im_save_format, w, h);
				IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_02_ImMainClusterFByMaskWithBorder" + g_im_save_format, w, h);
				ClearImageOptimal(ImMainCluster, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_03_ImMainClusterFOptimal" + g_im_save_format, w, h);
			},
				[&ImMainCluster2, &ImMaskWithBorder, w, h, iter_det] {
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_04_ImMainCluster2" + g_im_save_format, w, h);
				IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_05_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);
				ClearImageOptimal(ImMainCluster2, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_10_06_ImMainCluster2FOptimal" + g_im_save_format, w, h);
			}
			);
		}
		else
		{
			val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);

			concurrency::parallel_invoke(
				[&ImMainCluster, &ImMaskWithBorder, w, h, LH, LMAXY, lb, le, real_im_x_center, iter_det] {
				//if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_01_ImMainCluster" + g_im_save_format, w, h);
				IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_02_ImMainClusterFByMaskWithBorder" + g_im_save_format, w, h);
				//ClearImageOpt5(ImMainCluster, w, h, LH, LMAXY, real_im_x_center, 255);
				//if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_03_ImMainClusterFOpt5" + g_im_save_format, w, h);				
			},
				[&ImMainCluster2, &ImMaskWithBorder, w, h, LH, LMAXY, lb, le, real_im_x_center, iter_det] {
				//if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_04_ImMainCluster2" + g_im_save_format, w, h);
				IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_05_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);				
				//ClearImageOpt5(ImMainCluster2, w, h, LH, LMAXY, real_im_x_center, 255);
				//if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_03_13_06_ImMainCluster2FOpt5" + g_im_save_format, w, h);
			}
			);			

			int len1 = 0, len2 = 0;
			int cnt1 = cluster_cnt2[0], cnt2 = cluster_cnt2[1];

			{
				cvMAT cv_im;
				vector<vector<cv::Point> > contours;
				vector<cv::Vec4i> hierarchy;

				BinaryImageToMat(ImMainCluster, w, h, cv_im);				
				cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));				
				for (j=0; j<contours.size(); j++) len1 += cv::arcLength(contours[j], true);
				//len1 = cv::arcLength(contours, true);
			}
			
			{
				cvMAT cv_im;
				vector<vector<cv::Point> > contours;
				vector<cv::Vec4i> hierarchy;

				BinaryImageToMat(ImMainCluster2, w, h, cv_im);
				cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
				for (j = 0; j < contours.size(); j++) len2 += cv::arcLength(contours[j], true);
				//len2 = cv::arcLength(contours, true);
			}

			if ( (len1 > len2*1.2) && (cnt2 >= cnt1*0.4) )
			{
				memcpy(&ImMainCluster[0], &ImMainCluster2[0], w * h * sizeof(int));				
			}			

			//CombineTwoImages(ImMainCluster, ImMainCluster2, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainClusterImage_" + iter_det + "_04_03_ImMainCluster" + g_im_save_format, w, h);

			IntersectTwoImages(ImMASK, ImMainCluster, w, h);

			/*val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);
			if (val == 1)
			{
				if (g_show_results) SaveImageWithSubParams(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_04_02_ImMASK_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt4(ImMASK, w, h, W, H, LH, LMAXY, real_im_x_center, 255);
				if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_04_03_mMASKFOpt4" + g_im_save_format, w, h);
			}*/

			SortClusters(labels3, cluster_id3, cluster_cnt3, ImMASK, clusterCount3, w, h, 0, std::min<int>(w, real_im_x_center) - 1, 0, h - 1);
			GetBinaryImageByClusterId(ImMainCluster2, labels3, cluster_id3[0], w, h);

			if (g_show_results)
			{
				SaveGreyscaleImage(ImMASK, "/TestImages/GetMainClusterImage_" + iter_det + "_04_04_ImMASK" + g_im_save_format, w, h);
				memcpy(&ImRES4[0], &ImClusters2[0], w * h * sizeof(int));
				IntersectTwoImages(ImRES4, ImMASK, w, h);
				SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_04_05_ImClusters2InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_04_06_ImMainClusterInImClusters3" + g_im_save_format, w, h);
			}

			IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_04_06_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);
			ClearImageOptimal(ImMainCluster2, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/TestImages/GetMainClusterImage_" + iter_det + "_04_08_ImMainCluster2FOptimal" + g_im_save_format, w, h);
		}
	}

	memcpy(&ImRES[0], &ImMainCluster[0], w * h * sizeof(int));
	memcpy(&ImRES4[0], &ImMainCluster2[0], w * h * sizeof(int));

	IntersectTwoImages(ImRES4, ImRES, w, h);
	if (g_show_results) SaveGreyscaleImage(ImRES4, "/TestImages/GetMainClusterImage_" + iter_det + "_07_ImMainClusterIntImMainCluster2" + g_im_save_format, w, h);

	val = GetSubParams(ImRES4, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);

	CombineTwoImages(ImRES, ImMainCluster2, w, h, 255);
	if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainClusterImage_" + iter_det + "_08_4_ImMainClusteCombinedWithImMainCluster2" + g_im_save_format, w, h);

	if (val == 0)
	{
		val = GetSubParams(ImRES, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, 0, h - 1, iter_det);
	}

	if (val == 1)
	{
		if (g_show_results) SaveImageWithSubParams(ImRES, "/TestImages/GetMainClusterImage_" + iter_det + "_08_5_ImMainCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
		ClearImageOpt5(ImRES, w, h, LH, LMAXY, real_im_x_center, 255);
		if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainClusterImage_" + iter_det + "_08_6_ImMainClusterFOpt5" + g_im_save_format, w, h);
	}

	ClearImageByTextDistance(ImRES, w, h, W, H, real_im_x_center, 255, iter_det);
	if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainClusterImage_" + iter_det + "_08_7_ImMainClusterFByTextDistance" + g_im_save_format, w, h);

	start_time = GetTickCount();

	res = 1;
	return res;
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

	if (g_show_results) SaveRGBImage(ImRGB, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_01_1_ImRGB" + g_im_save_format, w, h);
	if (g_show_results) SaveRGBImage(ImFF, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_01_2_ImFF" + g_im_save_format, w, h);

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

		SaveRGBImage(ImRES1, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_02_ImRGBWithSplitInfo" + g_im_save_format, w, h);
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

		//GetMainClusterImage(ImRES1, ImRES2, ImRES3, ImRES5, tw, h, LH, LMAXY, iter_det + "_splititer" + std::to_string(ti + 1), white);

		for (y = 0, i = TL[ti], j = 0; y < h; y++, i += w, j += tw)
		{
			memcpy(&ImRES[i], &ImRES3[j], tw * sizeof(int));
			memcpy(&ImMaskWithBorder[i], &ImRES5[j], tw * sizeof(int));
		}
	});

	if (g_show_results) SaveRGBImage(ImRES, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_03_1_ImRES_MainCluster" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(ImRES, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_03_2_ImRES_MainClusterFilteredByFiguresOnSplitEdges" + g_im_save_format, w, h);
	*/

	if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/TestImages/GetMainClusterImageBySplit" + iter_det + "_03_3_ImMaskWithBorder" + g_im_save_format, w, h);
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

void GetMainColorImage(custom_buffer<int> &ImRES, custom_buffer<int> *pImMainCluster, custom_buffer<int> &ImMASK, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h, std::string iter_det, int min_x, int max_x)
{
	custom_buffer<int> ImRES1(w*h, 0);
	custom_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	int delta, val1, val2, val3, val4, NN, ys1;
	int i, j, j1, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int xb, xe, yb, ye, x, y;

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/GetMainColorImage_" + iter_det + "_01_01_ImMASK" + g_im_save_format, w, h);		

	yb = 0;
	ye = h-1;
	xb = 0;
	xe = w-1;	

	memcpy(&ImRES[0], &ImMASK[0], w*h * sizeof(int));

	if (!((min_x <= max_x) &&
		(max_x < w) &&
		(max_x >= 0) &&
		(min_x < w) &&
		(min_x >= 0)
		))
	{
		memset(&ImRES[0], 0, w*h * sizeof(int));
		if (pImMainCluster != NULL) memset(pImMainCluster->m_pData, 0, w*h * sizeof(int));
		return;
	}

	if (max_x < w)
	{
		for (y = 0; y < h; y++)
		{
			if (min_x > 0)
			{
				for (x = 0, i = (w*y); x < min_x; x++, i++)
				{
					ImRES[i] = 0;
				}
			}

			for (x = max_x + 1, i = (w*y) + max_x + 1; x < w; x++, i++)
			{
				ImRES[i] = 0;
			}
		}
	}

	delta = 100;
	StrAnalyseImage(ImRES, ImY, GRStr, w, h, xb, xe, yb, ye, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
	FindMaxStr(smax, smaxi, j1, ys1, NN);
	j1_min = j1;
	j1_max = j1_min + delta - 1;

	if (NN == 0)
	{
		memset(&ImRES[0], 0, w*h * sizeof(int));
		if (pImMainCluster != NULL) memset(pImMainCluster->m_pData, 0, w*h * sizeof(int));
		return;
	}

	for (i = 0; i < w*h; i++)
	{
		val1 = ImY[i];

		if ((ImRES[i] != 0) && 
			!((val1 >= j1_min) && (val1 <= j1_max))
			)
		{
			ImRES[i] = 0;
		}
	}
	if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainColorImage_" + iter_det + "_01_02_ImMASKFilteredByY" + g_im_save_format, w, h);

	int j2, ys2, j3, ys3, min_delta;	

	delta = 40;
	StrAnalyseImage(ImRES, ImU, GRStr, w, h, xb, xe, yb, ye, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
	FindMaxStr(smax, smaxi, j2, ys2, NN);
	j2_min = j2;
	j2_max = j2_min + delta - 1;

	if (NN == 0)
	{
		memset(&ImRES[0], 0, w*h * sizeof(int));
		if (pImMainCluster != NULL) memset(pImMainCluster->m_pData, 0, w*h * sizeof(int));
		return;
	}

	for (i = 0; i < w*h; i++)
	{
		val2 = ImU[i];

		if ((ImRES[i] != 0) &&
			!((val2 >= j2_min) && (val2 <= j2_max))
			)
		{
			ImRES[i] = 0;
		}
	}
	if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainColorImage_" + iter_det + "_01_03_ImMASKFilteredByYU" + g_im_save_format, w, h);
	
	delta = 40;
	StrAnalyseImage(ImRES, ImV, GRStr, w, h, xb, xe, yb, ye, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
	FindMaxStr(smax, smaxi, j3, ys3, NN);
	j3_min = j3;
	j3_max = j3_min + delta - 1;

	if (NN == 0)
	{
		memset(&ImRES[0], 0, w*h * sizeof(int));
		if (pImMainCluster != NULL) memset(pImMainCluster->m_pData, 0, w*h * sizeof(int));
		return;
	}

	for (i = 0; i < w*h; i++)
	{
		val3 = ImV[i];

		if ((ImRES[i] != 0) &&
			!((val3 >= j3_min) && (val3 <= j3_max))
			)
		{
			ImRES[i] = 0;
		}
	}
	if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/GetMainColorImage_" + iter_det + "_01_04_ImMASKFilteredByYUV" + g_im_save_format, w, h);	
	
	j1_min = 1000;
	j1_max = -1000;

	j2_min = 1000;
	j2_max = -1000;

	j3_min = 1000;
	j3_max = -1000;

	for (i = 0; i < w*h; i++)
	{
		if (ImRES[i] != 0)
		{
			if (ImY[i] < j1_min) j1_min = ImY[i];
			if (ImY[i] > j1_max) j1_max = ImY[i];

			if (ImU[i] < j2_min) j2_min = ImU[i];
			if (ImU[i] > j2_max) j2_max = ImU[i];

			if (ImV[i] < j3_min) j3_min = ImV[i];
			if (ImV[i] > j3_max) j3_max = ImV[i];
		}
	}

	if (pImMainCluster != NULL)
	{
		custom_buffer<int> &ImMainCluster = *pImMainCluster;

		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainColorImage_" + iter_det + "_02_01_MainTXTCluster" + g_im_save_format, w, h);

		for (i = 0; i < w*h; i++)
		{
			val1 = ImY[i];
			val2 = ImU[i];
			val3 = ImV[i];

			if ((ImMainCluster[i] != 0) &&
				!((val1 >= j1_min) && (val1 <= j1_max) &&
				(val2 >= j2_min) && (val2 <= j2_max) &&
					(val3 >= j3_min) && (val3 <= j3_max))
				)
			{
				ImMainCluster[i] = 0;
			}
		}

		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/GetMainColorImage_" + iter_det + "_02_02_MainTXTClusterFilteredByColor" + g_im_save_format, w, h);
	}
}

void ExtendByInsideFigures(custom_buffer<int> &ImRES, int w, int h, int white, bool simple)
{
	custom_buffer<int> ImTMP(w * h, 0);
	int val, val1, val2;

	// Extend ImSNF with Internal Figures
	val = GetImageWithInsideFigures(ImRES, ImTMP, w, h, white, simple);
	if (val > 0)
	{
		CombineTwoImages(ImRES, ImTMP, w, h, white);
	}
}

int CheckOnSubPresence(custom_buffer<int> &ImMASK, custom_buffer<int> &ImNE, custom_buffer<int> &ImFRes, int w, int h, int W, int H, int XB, int YB, std::string iter_det)
{
	custom_buffer<int> ImTMP(w*h, 0);
	int i, j, x, y, ww, hh, res = 0;

	cv::ocl::setUseOpenCL(g_use_ocl);

	memset(&ImFRes[0], 0, w*h * sizeof(int));

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/TestImages/CheckOnSubPresence_" + iter_det + "_01_01_ImFF" + g_im_save_format, w, h);

	{
		cvMAT cv_im_gr, cv_im_gr_resize;
		BinaryImageToMat(ImMASK, w, h, cv_im_gr);
		cv::resize(cv_im_gr, cv_im_gr_resize, cv::Size(0, 0), 1.0 / g_scale, 1.0 / g_scale);
		ww = cv_im_gr_resize.cols;
		hh = cv_im_gr_resize.rows;
		cv::dilate(cv_im_gr_resize, cv_im_gr_resize, cv::Mat(), cv::Point(-1, -1), 1);
		BinaryMatToImage(cv_im_gr_resize, ww, hh, ImTMP, 255);

		if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/CheckOnSubPresence_" + iter_det + "_01_02_ImFFDilate" + g_im_save_format, ww, hh);
	}

	custom_buffer<int> ImFFD(W*hh, 0), ImSF(W*hh, 0), ImTF(W*hh, 0);

	for (y = 0, i = XB, j = 0; y < hh; y++, i += W, j += ww)
	{
		memcpy(&ImFFD[i], &ImTMP[j], ww * sizeof(int));
	}

	if (g_show_results) SaveGreyscaleImage(ImFFD, "/TestImages/CheckOnSubPresence_" + iter_det + "_02_ImFFAligned" + g_im_save_format, W, hh);

	memcpy(&ImSF[0], &ImFFD[0], W*hh * sizeof(int));

	custom_buffer<int> LB(1, 0), LE(1, 0);
	LB[0] = 0;
	LE[0] = hh - 1;
	res = FilterTransformedImage(ImFFD, ImSF, ImTF, ImNE.get_sub_buffer(YB*W), LB, LE, 1, W, hh, W, H, iter_det);

	if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/CheckOnSubPresence_" + iter_det + "_03_ImFF_F" + g_im_save_format, W, hh);

	if (res == 0)
	{		
		return res;
	}

	for (y = 0, i = XB, j = 0; y < hh; y++, i += W, j += ww)
	{
		memcpy(&ImTMP[j], &ImTF[i], ww * sizeof(int));
	}

	if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/CheckOnSubPresence_" + iter_det + "_04_ImFF_F_Aligned" + g_im_save_format, ww, hh);

	{
		cvMAT cv_im_gr, cv_im_gr_resize;
		BinaryImageToMat(ImTMP, ww, hh, cv_im_gr);
		cv::resize(cv_im_gr, cv_im_gr_resize, cv::Size(0, 0), g_scale, g_scale);
		BinaryMatToImage(cv_im_gr_resize, w, h, ImFRes, 255);
	}

	if (g_show_results) SaveGreyscaleImage(ImFRes, "/TestImages/CheckOnSubPresence_" + iter_det + "_05_ImFF_F" + g_im_save_format, w, h);

	IntersectTwoImages(ImFRes, ImMASK, w, h);
	if (g_show_results) SaveGreyscaleImage(ImFRes, "/TestImages/CheckOnSubPresence_" + iter_det + "_06_ImFF_F_IntImMASK" + g_im_save_format, w, h);

	MergeImagesByIntersectedFigures(ImFRes, ImMASK, w, h, 255);
	if (g_show_results) SaveGreyscaleImage(ImFRes, "/TestImages/CheckOnSubPresence_" + iter_det + "_07_ImMASKMergeImFF_F" + g_im_save_format, w, h);

	return res;
}

FindTextRes FindText(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, custom_buffer<int> &ImNE, custom_buffer<int> &FullImIL, custom_buffer<int> &FullImY, string SaveName, std::string iter_det, int N, const int k, custom_buffer<int> LL, custom_buffer<int> LR, custom_buffer<int> LLB, custom_buffer<int> LLE, int W, int H)
{
	int i, j, l, r, x, y, ib, bln, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, w_orig, h_orig, ww, hh, cnt;
	int XB, XE, YB, YE, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, lb, le, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, cnt3, cnt4, cnt5, NN, ys1, ys2, ys3, val_min, val_max;
	int j1, j2, j3, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int mY, mI, mQ;
	int LH, LMAXY;
	custom_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	FindTextRes res;
	int color, rc, gc, bc, yc, cc, wc, min_h;
	u8 *pClr;
	DWORD start_time;
	const int mpn = g_mpn;

	cv::ocl::setUseOpenCL(g_use_ocl);

	start_time = GetTickCount();

	min_h = g_min_h * H;

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
	val = (3 * min_h) / 2; // 3/2 * ~ min sub height # (536-520+1)/576
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
	YE = std::min<int>(y + 2, H - 1);

	if (YE - YB < (2 * min_h) / 3)
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

	w_orig = w;
	h_orig = h;

	custom_buffer<int> ImRGBOrig(w*h, 0), ImRGBScaled((int)(w*g_scale)*(int)(h*g_scale), 0), ImSNFOrig((int)(w*g_scale)*(int)(h*g_scale), 0), ImSNF((int)(w*g_scale)*(int)(h*g_scale), 0), ImFF((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImY((int)(w*g_scale)*(int)(h*g_scale), 0), ImU((int)(w*g_scale)*(int)(h*g_scale), 0), ImV((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImL((int)(w*g_scale)*(int)(h*g_scale), 0), ImA((int)(w*g_scale)*(int)(h*g_scale), 0), ImB((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImIL((int)(w*g_scale)*(int)(h*g_scale), 0);
	custom_buffer<int> ImTHR((int)(w*g_scale)*(int)(h*g_scale), 0), ImTHRInside((w*g_scale)*(int)(h*g_scale), 0), ImTHRMerge((w*g_scale)*(int)(h*g_scale), 0);

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

		SaveRGBImage(ImTMP, "/TestImages/FindTextLines_" + iter_det + "_03_1_ImRGB_RGBWithLinesInfo" + g_im_save_format, W, H);
		SaveRGBImage(ImTMP, "/TestImages/" + SaveName + "_FindTextLines_" + iter_det + "_03_1_ImRGB_RGBWithLinesInfo" + g_im_save_format, W, H);
	}

	{
		cv::Mat cv_ImRGB;

		concurrency::parallel_invoke(
			[&ImRGB, &ImRGBOrig, &ImRGBScaled, &cv_ImRGB, YB, YE, XB, W, w, h, iter_det] {
			int y, i, j;
			for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
			{
				memcpy(&ImRGBOrig[j], &ImRGB[i], w * sizeof(int));
			}

			if (g_show_results) SaveRGBImage(ImRGBOrig, "/TestImages/FindTextLines_" + iter_det + "_03_2_1_ImRES1_RGB" + g_im_save_format, w, h);

			cv::Mat cv_ImRGBOrig(h, w, CV_8UC4);
			memcpy(cv_ImRGBOrig.data, &ImRGBOrig[0], w*h * sizeof(int));
			cv::resize(cv_ImRGBOrig, cv_ImRGB, cv::Size(0, 0), g_scale, g_scale);
			memcpy(&ImRGBScaled[0], cv_ImRGB.data, (int)(w * g_scale) * (int)(h * g_scale) * sizeof(int));

			if (g_show_results) SaveRGBImage(ImRGBScaled, "/TestImages/FindTextLines_" + iter_det + "_03_2_2_Im_RGBScaled4x4" + g_im_save_format, w * g_scale, h * g_scale);
		},
			[&ImNF, &ImSNFOrig, YB, YE, XB, W, w, h, iter_det] {
			custom_buffer<int> ImTMP(w * h, 0);
			int y, i, j;

			for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
			{
				memcpy(&ImTMP[j], &ImNF[i], w * sizeof(int));
			}

			cv::Mat cv_ImGROrig, cv_ImGR;
			GreyscaleImageToMat(ImTMP, w, h, cv_ImGROrig);
			cv::resize(cv_ImGROrig, cv_ImGR, cv::Size(0, 0), g_scale, g_scale);
			BinaryMatToImage(cv_ImGR, w * g_scale, h * g_scale, ImSNFOrig, 255);
			if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/TestImages/FindTextLines_" + iter_det + "_03_2_3_ImSNFOrig" + g_im_save_format, w * g_scale, h * g_scale);
		},
			[&FullImIL, &ImIL, YB, YE, XB, W, w, h, iter_det] {
			if (FullImIL[0] != -1)
			{
				custom_buffer<int> ImTMP(w * h, 0);
				int y, i, j;

				for (y = YB, i = YB * W + XB, j = 0; y <= YE; y++, i += W, j += w)
				{
					memcpy(&ImTMP[j], &FullImIL[i], w * sizeof(int));
				}

				cv::Mat cv_ImGROrig, cv_ImGR;
				GreyscaleImageToMat(ImTMP, w, h, cv_ImGROrig);
				cv::resize(cv_ImGROrig, cv_ImGR, cv::Size(0, 0), g_scale, g_scale);
				BinaryMatToImage(cv_ImGR, w * g_scale, h * g_scale, ImIL, 255);
				if (g_show_results) SaveGreyscaleImage(ImIL, "/TestImages/FindTextLines_" + iter_det + "_03_2_4_ImIL" + g_im_save_format, w * g_scale, h * g_scale);
			}
			else
			{
				ImIL[0] = -1;
			}
		}
		);

		concurrency::parallel_invoke(
			[&cv_ImRGB, &ImY, &ImU, &ImV, w, h, iter_det] {
			cv::Mat cv_Im, cv_ImSplit[3];
			cv::cvtColor(cv_ImRGB, cv_Im, cv::COLOR_BGR2YUV);
			cv::split(cv_Im, cv_ImSplit);
			GreyscaleMatToImage(cv_ImSplit[0], w * g_scale, h * g_scale, ImY);
			GreyscaleMatToImage(cv_ImSplit[1], w * g_scale, h * g_scale, ImU);
			GreyscaleMatToImage(cv_ImSplit[2], w * g_scale, h * g_scale, ImV);
			if (g_show_results) SaveGreyscaleImage(ImY, "/TestImages/FindTextLines_" + iter_det + "_03_4_1_ImY" + g_im_save_format, w * g_scale, h * g_scale);
			if (g_show_results) SaveGreyscaleImage(ImU, "/TestImages/FindTextLines_" + iter_det + "_03_4_2_ImU" + g_im_save_format, w * g_scale, h * g_scale);
			if (g_show_results) SaveGreyscaleImage(ImV, "/TestImages/FindTextLines_" + iter_det + "_03_4_3_ImV" + g_im_save_format, w * g_scale, h * g_scale);
		},
			[&cv_ImRGB, &ImL, &ImA, &ImB, w, h, iter_det] {
			cv::Mat cv_Im, cv_ImSplit[3];
			cv::cvtColor(cv_ImRGB, cv_Im, cv::COLOR_BGR2Lab);
			cv::split(cv_Im, cv_ImSplit);
			GreyscaleMatToImage(cv_ImSplit[0], w * g_scale, h * g_scale, ImL);
			GreyscaleMatToImage(cv_ImSplit[1], w * g_scale, h * g_scale, ImA);
			GreyscaleMatToImage(cv_ImSplit[2], w * g_scale, h * g_scale, ImB);

			if (g_show_results) SaveGreyscaleImage(ImL, "/TestImages/FindTextLines_" + iter_det + "_03_5_1_ImL" + g_im_save_format, w * g_scale, h * g_scale);
			if (g_show_results) SaveGreyscaleImage(ImA, "/TestImages/FindTextLines_" + iter_det + "_03_5_2_ImA" + g_im_save_format, w * g_scale, h * g_scale);
			if (g_show_results) SaveGreyscaleImage(ImB, "/TestImages/FindTextLines_" + iter_det + "_03_5_3_ImB" + g_im_save_format, w * g_scale, h * g_scale);
		}
		);
	}

	w *= g_scale;
	h *= g_scale;

	int real_im_x_center = ((W / 2) - XB) * g_scale;

	{
		custom_buffer<int> ImTMP(w*h, 0);
		custom_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = h - 1;
		GetImFF(ImSNF, ImTMP, ImRGBScaled, ImY, ImU, ImV, LB, LE, 1, w, h, (W*g_scale), (H*g_scale), g_smthr);
		if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_1_ImSNF" + g_im_save_format, w, h);
	}

	/*ClearImageByBigLines(ImSNF, w, h, (W*g_scale), (H*g_scale));
	if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_2_ImSNF_F" + g_im_save_format, w, h);*/

	// note: ImSNF can be too broken due to color filtration, don't merge it with ImSNF before extending internal figures

	custom_buffer<int> ImSNFS(ImSNF);

	concurrency::parallel_invoke(
		[&ImSNF, w, h, iter_det] {
			{
				cv::ocl::setUseOpenCL(g_use_ocl);

				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImSNF, w, h, cv_im_gr);
				cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_im_gr, w, h, ImSNF, 255);
				if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_3_1_ImSNFClose" + g_im_save_format, w, h);
			}

			ExtendByInsideFigures(ImSNF, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_3_2_ImSNFExtInternalFigures" + g_im_save_format, w, h);
	},
		[&ImSNFOrig, w, h, iter_det] {
			{
				cv::ocl::setUseOpenCL(g_use_ocl);

				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImSNFOrig, w, h, cv_im_gr);
				cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_im_gr, w, h, ImSNFOrig, 255);
				if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/TestImages/FindTextLines_" + iter_det + "_06_3_3_ImSNFOrigClose" + g_im_save_format, w, h);
			}

			ExtendByInsideFigures(ImSNFOrig, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/TestImages/FindTextLines_" + iter_det + "_06_3_4_ImSNFOrigExtInternalFigures" + g_im_save_format, w, h);
	});

	IntersectTwoImages(ImSNF, ImSNFOrig, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_5_01_ImSNFIntImSNFOrig" + g_im_save_format, w, h);

	//IntersectTwoImages(ImSNFS, ImSNFOrig, w, h);
	//if (g_show_results) SaveGreyscaleImage(ImSNFS, "/TestImages/FindTextLines_" + iter_det + "_06_5_02_ImSNFSIntImSNFOrig" + g_im_save_format, w, h);

	//ExtendByInsideFigures(ImSNF, w, h, 255);
	//if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_6_ImSNFExtInternalFigures" + g_im_save_format, w, h);

	if (ImIL[0] != -1)
	{
		IntersectTwoImages(ImSNF, ImIL, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_06_7_01_SNFIntImIL" + g_im_save_format, w, h);
	}

	//dont use: cv::threshold(cv_bw, cv_bw, 40, 255, cv::THRESH_BINARY | cv::THRESH_OTSU) produce on some images (0_00_48_648__0_00_50_115.jpeg) too bad results (half of image black (half of subs losed))
	//cv_bw = cv_ImY;

	/*cvMAT cv_bw;

	GreyscaleImageToMat(ImY, w, h, cv_bw);
	cv::medianBlur(cv_bw, cv_bw, 5);
	cv::adaptiveThreshold(cv_bw, cv_bw, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);

	GreyscaleMatToImage(cv_bw, w, h, ImTHR);
	if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_07_1_ImTHR" + g_im_save_format, w, h);

	for (i = 0; i < w*h; i++)
	{
		if (ImTHR[i] == 0)
		{
			ImTHRInside[i] = 255;
		}
		else
		{
			ImTHRInside[i] = 0;
		}
	}
	if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_07_2_ImTHRInsideFigures" + g_im_save_format, w, h);

	{
		// noise removal
		cv::Mat kernel_open = cv::Mat::ones(3, 3, CV_8U);

		concurrency::parallel_invoke(
			[&ImTHR, &ImSNF, &cv_bw, &kernel_open, w, h, iter_det] {
				cv::ocl::setUseOpenCL(g_use_ocl);

				custom_buffer<int> ImTMP(ImTHR);
				cvMAT cv_opening;

				if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_09_1_ImTHR" + g_im_save_format, w, h);
				ClearImageFromBorders(ImTMP, w, h, 1, h - 2, 255);
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/FindTextLines_" + iter_det + "_09_2_ImTHRClearedFromBorders" + g_im_save_format, w, h);
				cv::morphologyEx(cv_bw, cv_opening, cv::MORPH_OPEN, kernel_open, cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_opening, w, h, ImTHR, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_09_3_ImTHROpen" + g_im_save_format, w, h);
				ClearImageFromBorders(ImTHR, w, h, 1, h - 2, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_09_4_ImTHROpenClearedFromBorders" + g_im_save_format, w, h);
				CombineTwoImages(ImTHR, ImTMP, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_09_5_ImTHRCombinedClearedFromBorders" + g_im_save_format, w, h);
				if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_09_6_ImSNF" + g_im_save_format, w, h);
				IntersectTwoImages(ImTHR, ImSNF, w, h);
				if (g_show_results) SaveGreyscaleImage(ImTHR, "/TestImages/FindTextLines_" + iter_det + "_09_7_ImTHRIntImSNF" + g_im_save_format, w, h);
			},
			[&ImTHRInside, &ImSNF, &kernel_open, w, h, iter_det] {
				cv::ocl::setUseOpenCL(g_use_ocl);

				custom_buffer<int> ImTMP(ImTHRInside);
				cvMAT cv_inside;

				if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_10_1_ImTHRInsideFigures" + g_im_save_format, w, h);
				ClearImageFromBorders(ImTMP, w, h, 1, h - 2, 255);
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/TestImages/FindTextLines_" + iter_det + "_10_2_ImTHRInsideFiguresClearedFromBorders" + g_im_save_format, w, h);
				GreyscaleImageToMat(ImTHRInside, w, h, cv_inside);
				cv::morphologyEx(cv_inside, cv_inside, cv::MORPH_OPEN, kernel_open, cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_inside, w, h, ImTHRInside, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_10_3_ImTHRInsideFiguresOpen" + g_im_save_format, w, h);
				ClearImageFromBorders(ImTHRInside, w, h, 1, h - 2, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_10_4_ImTHRInsideFiguresOpenClearedFromBorders" + g_im_save_format, w, h);
				CombineTwoImages(ImTHRInside, ImTMP, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_10_5_ImTHRInsideFiguresCombinedClearedFromBorders" + g_im_save_format, w, h);
				if (g_show_results) SaveGreyscaleImage(ImSNF, "/TestImages/FindTextLines_" + iter_det + "_10_6_ImSNF" + g_im_save_format, w, h);
				IntersectTwoImages(ImTHRInside, ImSNF, w, h);
				if (g_show_results) SaveGreyscaleImage(ImTHRInside, "/TestImages/FindTextLines_" + iter_det + "_10_7_ImTHRInsideIntImSNF" + g_im_save_format, w, h);
			}
		);
	}

	memcpy(&ImTHRMerge[0], &ImTHR[0], w * h * sizeof(int));
	CombineTwoImages(ImTHRMerge, ImTHRInside, w, h);
	if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/TestImages/FindTextLines_" + iter_det + "_10_07_ImTHRMerge" + g_im_save_format, w, h);*/

	memcpy(&ImTHRMerge[0], &ImSNF[0], w * h * sizeof(int));

	ClearImageByTextDistance(ImTHRMerge, w, h, (W*g_scale), (H*g_scale), real_im_x_center, 255, iter_det);
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/TestImages/FindTextLines_" + iter_det + "_10_08_ImTHRMergeFByTextDistance" + g_im_save_format, w, h);

	custom_buffer<int> ImMaskWithBorder(w*h, 0), ImMaskWithBorderF(w*h, 0);
	custom_buffer<int> ImClusters2(w*h, 0), labels2(w*h, 0);	

	{
		custom_buffer<int> Im1(w * h, 0), ImTHRMergeF(ImTHRMerge), ImTHRMergeF2(w * h, 0);

		vector<custom_buffer<int>*> ImagesForCheck;				
		int clusterCount2 = 6;
		custom_buffer<int> cluster_id2(clusterCount2, 0);
		custom_buffer<int> cluster_cnt2(clusterCount2, 0);				

		val = GetFirstFilteredClusters(ImRGBScaled, ImTHRMerge, ImSNF, ImMaskWithBorder, ImMaskWithBorderF, ImClusters2, labels2, clusterCount2, w, h, iter_det + "_call1");

		if (val == 0)
		{
			return res;
		}
		
		if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/TestImages/FindTextLines_" + iter_det + "_11_02_02_ImTHRMergeOrig" + g_im_save_format, w, h);
		IntersectTwoImages(ImTHRMerge, ImMaskWithBorder, w, h);
		if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/TestImages/FindTextLines_" + iter_det + "_11_02_03_ImTHRMergeOrigIntImMaskWithBorder" + g_im_save_format, w, h);

		val = CheckOnSubPresence(ImTHRMerge, ImNE, ImTHRMergeF2, w, h, W, H, XB, YB, iter_det + "_call1");		
		if (g_show_results) SaveGreyscaleImage(ImTHRMergeF2, "/TestImages/FindTextLines_" + iter_det + "_11_02_04_ImTHRMergeOrigFWithCheckOnSubPresence" + g_im_save_format, w, h);

		if (val == 0)
		{
			return res;
		}
		else
		{
			memcpy(&ImTHRMerge[0], &ImTHRMergeF2[0], w * h * sizeof(int));
		}

		if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/TestImages/FindTextLines_" + iter_det + "_11_03_01_ImTHRMergeF" + g_im_save_format, w, h);
		IntersectTwoImages(ImTHRMergeF, ImTHRMerge, w, h);
		if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/TestImages/FindTextLines_" + iter_det + "_11_03_02_ImTHRMergeFIntImTHRMergeOrig" + g_im_save_format, w, h);
		IntersectTwoImages(ImTHRMergeF, ImMaskWithBorderF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/TestImages/FindTextLines_" + iter_det + "_11_03_03_ImTHRMergeFIntImMaskWithBorderF" + g_im_save_format, w, h);
		
		SortClusters(labels2, cluster_id2, cluster_cnt2, ImTHRMergeF, clusterCount2, w, h, 0, std::min<int>(w, real_im_x_center) - 1, 0, h - 1);
		GetBinaryImageByClusterId(Im1, labels2, cluster_id2[0], w, h);		
		custom_buffer<int> Im2(Im1);		

		concurrency::parallel_invoke(
			[&Im1, &ImTHRMerge, w, h, W, H, real_im_x_center, iter_det] {
				if (g_show_results) SaveGreyscaleImage(Im1, "/TestImages/FindTextLines_" + iter_det + "_11_04_01_01_ImTHRMergeFMainCluster1" + g_im_save_format, w, h);
				ExtendByInsideFigures(Im1, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(Im1, "/TestImages/FindTextLines_" + iter_det + "_11_04_01_02_ImTHRMergeFMainCluster1ExtInternalFigures" + g_im_save_format, w, h);

				IntersectTwoImages(Im1, ImTHRMerge, w, h);
				if (g_show_results) SaveGreyscaleImage(Im1, "/TestImages/FindTextLines_" + iter_det + "_11_04_02_ImTHRMergeFMainCluster1IntImTHRMergeOrig" + g_im_save_format, w, h);

				ClearImageByTextDistance(Im1, w, h, (W*g_scale), (H*g_scale), real_im_x_center, 255, iter_det);
				if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im1, "/TestImages/FindTextLines_" + iter_det + "_11_04_03_ImTHRMergeFMainCluster1FByTextDistance" + g_im_save_format, w, h);

				ClearImageOpt3(Im1, w, h, real_im_x_center, 255);
				if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im1, "/TestImages/FindTextLines_" + iter_det + "_11_04_04_ImTHRMergeFMainCluster1FByOpt3" + g_im_save_format, w, h);
			},
			[&Im2, &ImTHRMerge, w, h, W, H, real_im_x_center, iter_det] {
				// note: needed for: 0_01_05_295__0_01_05_594.jpeg (in other case line with two symbols can be too thin)		
				// also needed for: 0_14_28_040__0_14_30_399.jpeg in other case first cluster is too thin
				{
					cv::ocl::setUseOpenCL(g_use_ocl);

					cvMAT cv_im_gr;
					GreyscaleImageToMat(Im2, w, h, cv_im_gr);
					cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
					cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
					cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 2);
					BinaryMatToImage(cv_im_gr, w, h, Im2, 255);
					if (g_show_results) SaveGreyscaleImage(Im2, "/TestImages/FindTextLines_" + iter_det + "_11_05_ImTHRMergeFMainCluster1CloseDilate" + g_im_save_format, w, h);
				}
				ExtendByInsideFigures(Im2, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(Im2, "/TestImages/FindTextLines_" + iter_det + "_11_06_ImTHRMergeFMainCluster1ExtInternalFigures" + g_im_save_format, w, h);
				IntersectTwoImages(Im2, ImTHRMerge, w, h);
				if (g_show_results) SaveGreyscaleImage(Im2, "/TestImages/FindTextLines_" + iter_det + "_11_07_ImTHRMergeFMainCluster1IntImTHRMergeOrig" + g_im_save_format, w, h);

				ClearImageByTextDistance(Im2, w, h, (W*g_scale), (H*g_scale), real_im_x_center, 255, iter_det);
				if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im2, "/TestImages/FindTextLines_" + iter_det + "_11_08_ImTHRMergeFMainCluster1FByTextDistance" + g_im_save_format, w, h);

				ClearImageOpt3(Im2, w, h, real_im_x_center, 255);
				if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im2, "/TestImages/FindTextLines_" + iter_det + "_11_09_ImTHRMergeFMainCluster1FByOpt3" + g_im_save_format, w, h);
			}
		);

		ImagesForCheck.push_back(&Im2);
		ImagesForCheck.push_back(&Im1);
		ImagesForCheck.push_back(&ImTHRMerge);		

		for (int i_im = 0; i_im < ImagesForCheck.size(); i_im++)
		{
			custom_buffer<int> &ImThrRef = *(ImagesForCheck[i_im]);

			if (g_show_results) SaveGreyscaleImage(ImThrRef, "/TestImages/FindTextLines_" + iter_det + "_12_" + std::to_string(i_im) + "_1_ImTHR" + std::to_string(i_im) + g_im_save_format, w, h);

			yb = (orig_LLBk - YB) * g_scale;
			ye = (orig_LLEk - YB) * g_scale;
			xb = (orig_LLk - XB) * g_scale;
			xe = std::min<int>((int)(((W / 2) - XB) * g_scale), (orig_LRk - XB) * g_scale); //sometimes frome the right can be additional not centered text lines which intersect with two centered lines

			// Checking ImTHRMain on presence more then one line (if yes spliting on two and repeat)
			//-------------------------------		
			val = GetSubParams(ImThrRef, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale, yb, ye, iter_det);

			if (val == 1)
			{
				if (g_show_results) SaveImageWithSubParams(ImThrRef, "/TestImages/FindTextLines_" + iter_det + "_12_" + std::to_string(i_im) + "_2_1_ImTHR" + std::to_string(i_im) + "_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
			}

			if ((val == 1) && (std::max<int>((LLE[k] - LLB[k] + 1) * g_scale, le - lb + 1) > 1.5*(double)LH))
			{
				bln = 0;

				custom_buffer<int> lw(h, 0);
				int max_txt_w, max_txt_y, min_y1, max_y1, min_y2, max_y2, new_txt_y = 0, new_txt_w, max_txt2_w, max_txt2_y;

				custom_buffer<int> ImTHRF(ImThrRef);
				ClearImageFromMainSymbols(ImTHRF, w, h, W*g_scale, H*g_scale, LH, LMAXY, 255, iter_det);
				if (g_show_results) SaveGreyscaleImage(ImTHRF, "/TestImages/FindTextLines_" + iter_det + "_12_" + std::to_string(i_im) + "_2_2_ImTHR" + std::to_string(i_im) + "FFromMainSymbols" + g_im_save_format, w, h);

				for (y = yb; y <= ye; y++)
				{
					for (x = xb; x < xe; x++)
					{
						if ((y >= LMAXY - LH + 1) && (y <= LMAXY))
						{
							if (ImThrRef[y*w + x] != 0)
							{
								lw[y]++;
							}
						}
						else
						{
							if (ImTHRF[y*w + x] != 0)
							{
								lw[y]++;
							}
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


				min_y1 = yb;
				max_y1 = std::max<int>(LMAXY - LH * 1.2, 0);
				max_y2 = std::min<int>(LMAXY - LH * 0.8, max_txt_y - 1);

				int y1, y2;

				if (max_y2 - min_y1 + 1 >= std::max<int>(LH * 0.6, min_h * g_scale))
				{
					for (y1 = max_y1; y1 >= min_y1; y1--)
					{
						if (lw[y1] >= (min_h * g_scale * 2) / 3) // bigger then 2/3 min sub height
						{
							for (y2 = max_y2; y2 >= y1 + 1; y2--)
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
							for (y2 = max_y2; y2 >= y1 + 1; y2--)
							{
								if (lw[y2] < lw[new_txt_y])
								{
									new_txt_y = y2;
									new_txt_w = lw[new_txt_y];
								}
							}

							y2 = new_txt_y;
							while (lw[new_txt_y] == 0) new_txt_y--;
							new_txt_y = (y2 + new_txt_y) / 2;
							new_txt_w = lw[new_txt_y];

							max_txt2_y = y1;
							max_txt2_w = lw[max_txt2_y];

							if (new_txt_y - yb + 1 < min_h * g_scale)
							{
								new_txt_y = 0;
							}

							break;
						}
					}
				}

				if (new_txt_y == 0)
				{
					min_y1 = std::min<int>(LMAXY + LH * 0.2, h-1);
					min_y2 = std::max<int>(LMAXY - LH * 0.2, max_txt_y + 1);
					max_y1 = ye;					

					if (max_y1 - min_y2 + 1 >= std::max<int>(LH * 0.6, min_h * g_scale))
					{
						for (y1 = min_y1; y1 <= max_y1; y1++)
						{
							if (lw[y1] >= (min_h * g_scale * 2) / 3) // bigger then 2/3 min sub height
							{
								for (y2 = min_y2; y2 <= y1 - 1; y2++)
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
								for (y2 = min_y2; y2 <= y1 - 1; y2++)
								{
									if (lw[y2] < lw[new_txt_y])
									{
										new_txt_y = y2;
										new_txt_w = lw[new_txt_y];
									}
								}

								y2 = new_txt_y;
								while (lw[new_txt_y] == 0) new_txt_y++;
								new_txt_y = (y2 + new_txt_y) / 2;
								new_txt_w = lw[new_txt_y];

								max_txt2_y = y1;
								max_txt2_w = lw[max_txt2_y];

								if (ye - new_txt_y + 1 < min_h * g_scale)
								{
									new_txt_y = 0;
								}

								break;
							}
						}
					}
				}

				if ((new_txt_y > 0) &&
					(((YB + (new_txt_y / g_scale)) - orig_LLBk + 1) >= g_scale) &&
					((orig_LLEk - (YB + (new_txt_y / g_scale) + 1) + 1) >= g_scale)
					)
				{
					cnt1 = 0;
					cnt2 = 0;
					for (y = yb; y <= ye; y++)
					{
						for (x = xb; x < xe; x++)
						{
							if (ImThrRef[y*w + x] != 0)
							{
								if (y <= new_txt_y) cnt1++;
								else cnt2++;
							}
						}
					}

					if ((cnt1 >= mpn) && (cnt2 >= mpn))
					{
						int LH1, LMAXY1, lb1, le1;
						int LH2, LMAXY2, lb2, le2;
						val1 = GetSubParams(ImThrRef, w, h, 255, LH1, LMAXY1, lb1, le1, min_h * g_scale, ((W / 2) - XB) * g_scale, yb, new_txt_y, iter_det);
						val2 = GetSubParams(ImThrRef, w, h, 255, LH2, LMAXY2, lb2, le2, min_h * g_scale, ((W / 2) - XB) * g_scale, new_txt_y + 1, ye, iter_det);

						val = 0;
						if ((val1 == 1) && (val2 == 1))
						{
							if (std::min<int>(LH1, LH2) >= std::max<int>(LH1, LH2) / 2)
							{
								val = 1;
							}
						}

						if (val == 1)
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

							if (g_show_results) SaveImageWithLinesInfo(ImThrRef, "/TestImages/FindTextLines_" + iter_det + "_12_" + std::to_string(i_im) + "_3_ImTHR" + std::to_string(i_im) + "_WithSplitInfo" + g_im_save_format, (LLB[k] - YB)*g_scale, (LLE[k] - YB)*g_scale, (LLB[k + 1] - YB)*g_scale, (LLE[k + 1] - YB)*g_scale, w, h);

							return res;
						}
					}
				}
			}
			//----------------------------	
		}
	}

	{
		custom_buffer<int> ImMainMASK(w*h, 0);
		
		yb = (LLB[k] - YB) * g_scale;
		ye = (LLE[k] - YB) * g_scale;
		xb = (LL[k] - XB) * g_scale;
		xe = (LR[k] - XB) * g_scale;

		{
			custom_buffer<int> ImMainMASKF(w*h, 0), ImMainCluster(w*h, 0), ImMainClusterF(w*h, 0);					

			val = GetMainClusterImage(ImRGBScaled, ImTHRMerge, ImSNFS, ImIL, ImMainCluster, ImL, ImA, ImB, ImMaskWithBorder, ImMaskWithBorderF, ImClusters2, labels2, w, h, W * g_scale, H * g_scale, iter_det, ((W / 2) - XB) * g_scale, min_h * g_scale);
			if (val == 0)
			{
				return res;
			}

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/FindTextLines_" + iter_det + "_13_02_ImMainCluster" + g_im_save_format, w, h);			

			val = GetSubParams(ImMainCluster, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale, yb, ye, iter_det);
			if (val == 0)
			{
				return res;
			}

			ClearMainClusterImage(ImMainCluster, ImSNFS, ImIL, w, h, W * g_scale, H * g_scale, LH, iter_det + "_call1");
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/FindTextLines_" + iter_det + "_13_03_ImMainClusterF" + g_im_save_format, w, h);

			if (g_clear_txt_images_by_main_color)
			{
				memcpy(&ImMainMASK[0], &ImSNF[0], w*h * sizeof(int));
				IntersectTwoImages(ImMainMASK, ImMainCluster, w, h);

				memcpy(&ImMainClusterF[0], &ImMainCluster[0], w * h * sizeof(int));
				GetMainColorImage(ImMainMASKF, &ImMainClusterF, ImMainMASK, ImL, ImA, ImB, w, h, iter_det + "_call2", 0, w - 1);

				if (g_show_results) SaveGreyscaleImage(ImMainClusterF, "/TestImages/FindTextLines_" + iter_det + "_14_ImMainClusterFByColor" + g_im_save_format, w, h);

				ClearImageByMask(ImMainCluster, ImMainClusterF, w, h, 255, 2.0 / 3.0);

				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/FindTextLines_" + iter_det + "_15_ImMainClusterClearByMASKImMainClusterFByColor" + g_im_save_format, w, h);
			}

			val = GetSubParams(ImMainCluster, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale, yb, ye, iter_det);
			if (val == 0)
			{
				val = GetSubParams(ImMainMASKF, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, ((W / 2) - XB) * g_scale, yb, ye, iter_det);
			}
			if (val != 0)
			{
				if (g_show_results) SaveImageWithSubParams(ImMainCluster, "/TestImages/FindTextLines_" + iter_det + "_16_ImMainCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt5(ImMainCluster, w, h, LH, LMAXY, real_im_x_center, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/TestImages/FindTextLines_" + iter_det + "_17_ImMainClusterFOpt5" + g_im_save_format, w, h);
			}

			memcpy(&ImFF[0], &ImMainCluster[0], w*h * sizeof(int));
		}		
	}

	ww = W * g_scale;
	hh = h;

	custom_buffer<int> ImSubForSave(ww*hh, 0);
	for (i = 0; i < ww*hh; i++) ImSubForSave[i] = wc;

	cnt = 0;
	for (y = 0, i = 0; y < h; y++)
	{
		for (x = 0; x < w; x++, i++)
		{
			if (ImFF[i] != 0)
			{
				cnt++;
				ImSubForSave[y*ww + (int)(XB * g_scale) + x] = 0;
			}
		}
	}

	if (cnt > 0)
	{
		res.m_YB = YB;
		res.m_im_h = h / g_scale;

		custom_buffer<int> ImFFD(W*res.m_im_h, 0), ImSF(W*res.m_im_h, 0), ImTF(W*res.m_im_h, 0);

		{
			cv::Mat cv_ImRGBOrig(hh, ww, CV_8UC4), cv_ImDilate(res.m_im_h, W, CV_8UC1);
			memcpy(cv_ImRGBOrig.data, &ImSubForSave[0], hh*ww * sizeof(int));
			cv::resize(cv_ImRGBOrig, res.m_cv_ImRGB, cv::Size(0, 0), 1.0/g_scale, 1.0/g_scale);

			for (i = 0; i < res.m_im_h*W; i++)
			{
				if (((int*)(res.m_cv_ImRGB.data))[i] == 0)
				{
					cv_ImDilate.data[i] = 255;
				}
				else
				{
					cv_ImDilate.data[i] = 0;
				}
			}
			cv::dilate(cv_ImDilate, cv_ImDilate, cv::Mat(), cv::Point(-1, -1), 2);

			ImFFD = custom_buffer<int>(W*res.m_im_h, 0);
			for (i = 0; i < res.m_im_h*W; i++)
			{
				if (cv_ImDilate.data[i] != 0)
				{
					ImFFD[i] = 255;
				}
			}

			if (g_show_results) SaveGreyscaleImage(ImFFD, "/TestImages/FindTextLines_" + iter_det + "_78_01_ImTXTDilate" + g_im_save_format, W, res.m_im_h);
		}
		
		memcpy(&ImSF[0], &ImFFD[0], W*res.m_im_h * sizeof(int));

		custom_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = res.m_im_h - 1;
		res.m_res = FilterTransformedImage(ImFFD, ImSF, ImTF, ImNE.get_sub_buffer(res.m_YB*W), LB, LE, 1, W, res.m_im_h, W, H, iter_det);

		if (g_show_results) SaveGreyscaleImage(ImTF, "/TestImages/FindTextLines_" + iter_det + "_78_02_ImTXTF" + g_im_save_format, W, res.m_im_h);

		if (res.m_res == 1)
		{
			GetTextLineParameters(ImFF, ImY, ImU, ImV, w, h, LH, LMAXY, DXB, DXE, DYB, DYE, mY, mI, mQ, 255);

			res.m_LH = LH / g_scale;
			res.m_LY = YB + LMAXY / g_scale;
			res.m_LXB = XB + DXB / g_scale;
			res.m_LXE = XB + DXE / g_scale;
			res.m_LYB = YB + DYB / g_scale;
			res.m_LYE = YB + DYE / g_scale;
			res.m_mY = mY;
			res.m_mI = mI;
			res.m_mQ = mQ;

			char str[10];
			string FullName;

			FullName = string("/TXTImages/");
			FullName += SaveName;
			FullName += string("_");
			sprintf(str, "%.5d", res.m_LY);
			FullName += str;
			FullName += g_im_save_format;

			res.m_ImageName = FullName;

			SaveRGBImage(ImSubForSave, FullName, ww, hh);
		}
	}

	return res;
}

inline concurrency::task<FindTextRes> TaskFindText(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, custom_buffer<int> &ImNE, custom_buffer<int> &ImIL, custom_buffer<int> &FullImY, string &SaveName, std::string &iter_det, int N, int k, custom_buffer<int> &LL, custom_buffer<int> &LR, custom_buffer<int> &LLB, custom_buffer<int> &LLE, int W, int H)
{	
	return concurrency::create_task([&ImRGB, &ImF, &ImNF, &ImNE, &ImIL, &FullImY, SaveName, iter_det, N, k, LL, LR, LLB, LLE, W, H]	{
			return FindText(ImRGB, ImF, ImNF, ImNE, ImIL, FullImY, SaveName, iter_det, N, k, LL, LR, LLB, LLE, W, H);
		}
	);
}

int FindTextLines(custom_buffer<int> &ImRGB, custom_buffer<int> &ImClearedText, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, custom_buffer<int> &ImNE, custom_buffer<int> &ImIL, vector<wxString> &SavedFiles, int W, int H)
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
	int min_h = g_min_h * H;
	
	//cv::String info;
	//cv::ocl::Device device = cv::ocl::Device::getDefault();
	//info = "Device name: " + device.name() + "\n";

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

	//g_pViewImage[0](ImRGB, W, H);	

	if (g_show_results) SaveRGBImage(ImRGB, "/TestImages/FindTextLines_01_1_ImRGB" + g_im_save_format, W, H);
	if (g_show_results) SaveRGBImage(ImRGB, "/TestImages/" + SaveName + g_im_save_format, W, H);

	if (g_show_results) SaveGreyscaleImage(ImF, "/TestImages/FindTextLines_01_2_ImF" + g_im_save_format, W, H);
	if (g_show_results) SaveGreyscaleImage(ImNF, "/TestImages/FindTextLines_01_3_ImNF" + g_im_save_format, W, H);

	{
		cv::Mat cv_FullImRGB(H, W, CV_8UC4), cv_FullImY;
		memcpy(cv_FullImRGB.data, &ImRGB[0], W*H * sizeof(int));
		cv::cvtColor(cv_FullImRGB, cv_FullImY, cv::COLOR_BGR2GRAY);
		GreyscaleMatToImage(cv_FullImY, W, H, FullImY);
		if (g_show_results) SaveGreyscaleImage(FullImY, "/TestImages/FindTextLines_01_4_FullImY" + g_im_save_format, W, H);
	}

	if (g_show_results && (ImIL[0] != -1)) SaveGreyscaleImage(ImIL, "/TestImages/FindTextLines_01_5_FullImIL" + g_im_save_format, W, H);	

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
		
		if (LLE[k] - LLB[k] < (2 * min_h) / 3)
		{
			for (i = k; i < N - 1; i++)
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
		FindTextTasks.emplace_back(TaskFindText(ImRGB, ImF, ImNF, ImNE, ImIL, FullImY, SaveName, "l" + std::to_string(k + 1), N, k, LL, LR, LLB, LLE, W, H));
	}
	
	k = 0;
	while (k < FindTextTasks.size())
	{
		FindTextRes ft_res = FindTextTasks[k].get();

		if (ft_res.m_LL.m_size > 0) // ned to split text on 2 parts
		{
			FindTextTasks.emplace_back(TaskFindText(ImRGB, ImF, ImNF, ImNE, ImIL, FullImY, SaveName, ft_res.m_iter_det + "_sl1", ft_res.m_N, ft_res.m_k, ft_res.m_LL, ft_res.m_LR, ft_res.m_LLB, ft_res.m_LLE, W, H));
			FindTextTasks.emplace_back(TaskFindText(ImRGB, ImF, ImNF, ImNE, ImIL, FullImY, SaveName, ft_res.m_iter_det + "_sl2", ft_res.m_N, ft_res.m_k + 1, ft_res.m_LL, ft_res.m_LR, ft_res.m_LLB, ft_res.m_LLE, W, H));
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

				//g_pViewRGBImage(ImClearedText, W, H);

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

void ClearImageByBigLines(custom_buffer<int> &Im, int w, int h, int W, int H)
{
	// Removing all big lines on image Im
	int i, x, y, lb, le, max_line_len = std::min<int>(0.25*W, h * 1.5);
	for (y = 0, i = 0; y < h; y++)
	{
		lb = -1;
		for (x = 0; x < w; x++, i++)
		{
			if (Im[i] != 0)
			{
				if (lb == -1)
				{
					lb = x;
				}
				le = x;
			}
			else
			{
				if (lb != -1)
				{
					if (le - lb + 1 >= max_line_len)
					{
						for (int xx = lb, j = (y*w) + xx; xx <= le; xx++, j++)
						{
							Im[j] = 0;
						}
					}
				}

				lb = -1;
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

int ClearImageOptimal(custom_buffer<int> &Im, int w, int h, int white)
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
		
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];
		
		if (
			(pFigure->m_minY < 1) ||
			(pFigure->m_maxY > h-2) ||
			((pFigure->m_minX <= 4) || (pFigure->m_maxX >= (w - 1) - 4)) ||
			(g_remove_wide_symbols && (pFigure->m_w >= h * 2))
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

	return 1;
}

int ClearImageOptimal2(custom_buffer<int> &Im, int w, int h, int min_x, int max_x, int min_y, int max_y, int white)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, N /*num of closed figures*/, NNY, min_h;
	int val = 0, val1, val2, ddy1, ddy2;
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

		if (
			(pFigure->m_minY <= min_y) ||
			(pFigure->m_maxY >= max_y) ||
			((pFigure->m_minX <= min_x) || (pFigure->m_maxX >= max_x)) ||
			(g_remove_wide_symbols && (pFigure->m_w >= h * 2)) ||
			((pFigure->m_w < 3) || (pFigure->m_h < 3)) //|| bad for multiple points with main color like: ......
			//( (pFigure->m_Square >= 0.95*((M_PI*pFigure->m_w*pFigure->m_h) / 4.0)) && //is like ellipse or rectangle on 95% 
//			  (std::max<int>(pFigure->m_h, pFigure->m_w) <= 2 * std::min<int>(pFigure->m_h, pFigure->m_w)) )
			)
		{
			/*if((pFigure->m_Square >= 0.95*((M_PI*pFigure->m_w*pFigure->m_h) / 4.0)) && //is like ellipse or rectangle on 95% 
				(std::max<int>(pFigure->m_h, pFigure->m_w) <= 2 * std::min<int>(pFigure->m_h, pFigure->m_w)))
			{
				i = i;
			}*/

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

	if (N == 0)
	{
		return 0;
	}

	return 1;
}

void CombineFiguresRelatedToEachOther(custom_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h, std::string iter_det)
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

				if ( (i != j) && 
					 (pFigureJ->m_w < 5*min_h)
					)
				{
					int val1 = (pFigureI->m_maxX + pFigureI->m_minX) - (pFigureJ->m_maxX + pFigureJ->m_minX);
					if (val1 < 0) val1 = -val1;
					val1 = ((pFigureI->m_maxX - pFigureI->m_minX) + (pFigureJ->m_maxX - pFigureJ->m_minX) + 2 - val1) / 2;

					int my = (pFigureI->m_maxY + pFigureI->m_minY) / 2;

					if ((pFigureI->m_h >= min_h / 3) && // ImI is not too small
						(pFigureI->m_w <= 1.5*(double)pFigureJ->m_w) && // ImI is not too wide
						(pFigureI->m_minY < pFigureJ->m_minY) && // ImI is on top of ImJ						
						( ( (pFigureI->m_maxY < pFigureJ->m_minY + (pFigureJ->m_h / 2)) &&
						    (pFigureI->m_w >= 1.5*(double)pFigureI->m_h) && 
							(pFigureI->m_h <= pFigureJ->m_h) ) || // ImI is wider then its height (if on top half or higher) and not too high
							( (pFigureI->m_maxY < pFigureJ->m_maxY) && // ImI max is located on first half of ImJ
							  (pFigureI->m_maxY >= pFigureJ->m_minY + (pFigureJ->m_h / 2)) ) ) &&
						( 
						(val1 >= 0.75*(double)pFigureI->m_w) && // ImI intersect with ImJ by x more then 0.75*ImI_width
						((pFigureJ->m_minY - pFigureI->m_maxY - 1) <= std::min<int>((std::min<int>(pFigureI->m_h, pFigureJ->m_h) / 3), std::max<int>(pFigureI->m_h, pFigureJ->m_h) / 4)) // distance between ImI and ImJ is not bigger then Images_height / 4
						))
					{
						/*
#ifdef DEBUG
						if (iter_det == "l2_sl2")
						{
							iter_det = iter_det;
						}
#endif						
						*/

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

int GetSubParams(custom_buffer<int> &Im, int w, int h, int white, int &LH, int &LMAXY, int &lb, int &le, int min_h, int real_im_x_center, int yb, int ye, std::string iter_det, bool combine_figures_related_to_each_other)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, val, N, minN, H, delta, NNN, jY, jI, jQ;
	custom_buffer<int> GRStr(256 * 2, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	custom_buffer<int> ImRR(w*h, 0);
	int val1, val2, val3;
	int NNY;
	CMyPoint *PA;
	clock_t t;

	LMAXY = 0;
	LH = 0;
	lb = h - 1;
	le = 0;

	if (real_im_x_center <= 0) return 0;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();	

	if (N == 0)	return 0;

	custom_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	custom_buffer<int> NN(N, 0), NY(N, 0), NL(N, 0), NR(N, 0);
	custom_buffer<CMyClosedFigure*> good_figures(N);
	
	/*
#ifdef DEBUG
	if (iter_det == "l2_sl2")
	{
		iter_det = iter_det;
	}
#endif
	*/

	CombineFiguresRelatedToEachOther(ppFigures, N, min_h, iter_det);

	for (i = 0, j = 0; i < N; i++)
	{
		pFigure = ppFigures[i];		

		if ( (pFigure->m_h >= min_h) &&
			 ((pFigure->m_minY + pFigure->m_maxY)/2 > yb) &&
			((pFigure->m_minY + pFigure->m_maxY)/2 < ye) &&
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
		if ((NN[i] > NN[j]) && (NL[i] < real_im_x_center))
		{
			j = i;
		}
	}
	for (i = 0; i < k; i++)
	{
		//if ((NY[i] > NY[j]) && ((NN[i] >= 2) || (NN[i] >= NN[j])) && (NL[i] < real_im_x_center))
		if ((NR[i] - NL[i] > NR[j] - NL[j]) && (NN[i] >= NN[j]) && (NL[i] < real_im_x_center))
		{
			j = i;
		}
	}
	LMAXY = NY[j];

	custom_buffer<int> arMinY(N, 0);

	for (i = 0, k = 0; i < NNY; i++)
	{
		pFigure = good_figures[i];

		if ( //(pFigure->m_minX < real_im_x_center) &&
			 (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY - std::max<int>(g_dmaxy, LH / 8))
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

int ClearImageByMask(custom_buffer<int> &Im, custom_buffer<int> &ImMASK, int w, int h, int white, double thr)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
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

	//Removing all elements which are going outside ImMASK

	i = 0;
	while (i < N)
	{
		int cnt = 0;
		pFigure = ppFigures[i];

		PA = pFigure->m_PointsArray;

		for (l = 0; l < pFigure->m_Square; l++)
		{
			ii = (PA[l].m_y*w) + PA[l].m_x;

			if (ImMASK[ii] == 0)
			{
				cnt++;
			}
		}

		if (cnt >= std::max<int>(thr*pFigure->m_Square, 1))
		{
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

int ClearImageFromMainSymbols(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int white, std::string iter_det)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	CMyPoint *PA;
	clock_t t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, 255, pFigures);
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

		int val1 = (pFigure->m_maxY + pFigure->m_minY) - (LMAXY + LMAXY - LH + 1);
		if (val1 < 0) val1 = -val1;
		val1 = ((pFigure->m_maxY - pFigure->m_minY + 1) + LH - val1) / 2;

		if ((val1 >= 0.25*(double)LH) // Im intersect with [LMAXY, LMAXY - LH + 1] more then on 25%
			)
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
}

int ClearImageOpt2(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, int white, std::string iter_det)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	CMyPoint *PA;
	clock_t t;
	int max_len = std::min<int>(0.25*W, h * 2);

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
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (6 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (1 * LH) / 5) || // do agressive even Arabic
			(pFigure->m_maxY < LMAXY - LH) ||
			(g_remove_wide_symbols && (pFigure->m_w >= LH * 3)) ||
			(g_remove_wide_symbols && (pFigure->m_w >= max_len)) //||
			//((pFigure->m_w < 3) || (pFigure->m_h < 3)) || image can be too broken in 6 clusters
			//(pFigure->m_Square >= (LH * LH)/2) //remove good symbols
			)
		{
			PA = pFigure->m_PointsArray;

			/*
#ifdef DEBUG
			if (iter_det == "l2_02_05_01_id4")
			{
				i = i;
			}
#endif			
			*/

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

	if (N == 0) return N;

	/*int min_x = ppFigures[0]->m_minX;

	for (i = 1; i < N; i++)
	{
		if (ppFigures[i]->m_minX < min_x)
		{
			min_x = ppFigures[i]->m_minX;
		}
	}

	int max_x = real_im_x_center;
	if (min_x < real_im_x_center)
	{
		max_x = real_im_x_center + (real_im_x_center - min_x);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_minX > max_x)
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
	}*/

	return N;
}

int ClearImageOpt3(custom_buffer<int> &Im, int w, int h, int real_im_x_center, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
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

	int min_x = ppFigures[0]->m_minX;

	for (i = 1; i < N; i++)
	{
		if (ppFigures[i]->m_minX < min_x)
		{
			min_x = ppFigures[i]->m_minX;
		}
	}

	int max_x = real_im_x_center;
	if (min_x < real_im_x_center)
	{
		max_x = real_im_x_center + (real_im_x_center - min_x);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_minX > max_x)
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

int ClearImageOpt4(custom_buffer<int> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	CMyPoint *PA;
	clock_t t;
	int min_h = H * g_min_h;

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
			(g_remove_wide_symbols && (pFigure->m_w >= LH * 3)) ||
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (6 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (1 * LH) / 5) || // do agressive even Arabic
			((pFigure->m_w < 3) || (pFigure->m_h < 3)) ||
			( ((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) <= (LH/8))  &&
			  ((pFigure->m_maxY - LMAXY) >= (LH / 16)) ) //||
			//(pFigure->m_maxY < LMAXY - (LH / 2)) || // only main characters are saved
			//(pFigure->m_minY > LMAXY - LH / 5) ||
			//(pFigure->m_Square >= (LH * LH) / 2) //|| remove good symbols
			//(pFigure->m_Square <= 0.1*(pFigure->m_w*pFigure->m_h)) ||
			//( (pFigure->m_Square >= 0.7*(pFigure->m_w*pFigure->m_h)) &&
			// (pFigure->m_h >= LH/2) && (pFigure->m_w >= pFigure->m_h/3) )
			)
		{
			PA = pFigure->m_PointsArray;

			/*
#ifdef DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif			
			*/

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

	/*if (N == 0) return N;

	int min_x = ppFigures[0]->m_minX;

	for (i = 1; i < N; i++)
	{
		if (ppFigures[i]->m_minX < min_x)
		{
			min_x = ppFigures[i]->m_minX;
		}
	}

	int max_x = real_im_x_center;	
	if (min_x < real_im_x_center)
	{
		max_x = real_im_x_center + (real_im_x_center - min_x);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_minX > max_x)
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
	}*/

	return N;
}

int ClearImageOpt5(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int real_im_x_center, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
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
			(g_remove_wide_symbols && (pFigure->m_w >= LH * 3)) ||
			(g_remove_wide_symbols && (pFigure->m_w >= h * 2)) ||
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (7 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (3 * LH) / 5) || // final clear: Arabic symbols can have dots below line
			((pFigure->m_w < 3) || (pFigure->m_h < 3))
			)
		{
			PA = pFigure->m_PointsArray;

			/*
#ifdef DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif
			*/

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

	/*if (N == 0) return N;

	int min_x = ppFigures[0]->m_minX;

	for (i = 1; i < N; i++)
	{
		if (ppFigures[i]->m_minX < min_x)
		{
			min_x = ppFigures[i]->m_minX;
		}
	}

	int max_x = real_im_x_center;
	if (min_x < real_im_x_center)
	{
		max_x = real_im_x_center + (real_im_x_center - min_x);
	}

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_minX > max_x)
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
	}*/

	return N;
}

void CombineFiguresRelatedToEachOther2(custom_buffer<CMyClosedFigure*> &ppFigures, int &N)
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

					if ((pFigureI->m_minY < pFigureJ->m_minY) && // ImI is on top of ImJ
						(val1 >= 1) && // ImI intersect with ImJ
						((pFigureJ->m_minY - pFigureI->m_maxY - 1) <= 2 * g_segh) // distance between ImI and ImJ is not bigger then 2*g_segh
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

int ClearImageByTextDistance(custom_buffer<int> &Im, int w, int h, int W, int H, int real_im_x_center, int white, std::string iter_det)
{
	const int dw = (int)(g_btd*(double)W);
	const int dw2 = (int)(g_tco*(double)W*2.0);

	CMyClosedFigure *pFigure;
	int i, j, k, val1, val2, l, ii, N, res = 0, x, y, y2;
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

	for (i = 0; i < N-1; i++)
	{
		for (j = i + 1; j < N; j++)
		{
			if (ppFigures[j]->m_minX < ppFigures[i]->m_minX)
			{
				pFigure = ppFigures[i];
				ppFigures[i] = ppFigures[j];
				ppFigures[j] = pFigure;
			}
		}
	}

	custom_buffer<int> max_x(N, 0);
	for (j = 0, val1 = ppFigures[0]->m_maxX; j < N; j++)
	{
		if (ppFigures[j]->m_maxX > val1)
		{
			val1 = ppFigures[j]->m_maxX;
		}
		max_x[j] = val1;
	}

	i = N - 2;
	while ((i >= 0) && (i < (N - 1)))
	{
		if (ppFigures[i+1]->m_minX - max_x[i] > dw)
		{
			/*if (iter_det == "l4_sl1")
			{
				iter_det = iter_det;
			}*/

			if (ppFigures[i + 1]->m_minX > real_im_x_center)
			{
				// removing from right side				
				for (j = i + 1; j < N; j++)
				{
					pFigure = ppFigures[j];
					PA = pFigure->m_PointsArray;
					
					for (l = 0; l < pFigure->m_Square; l++)
					{
						ii = PA[l].m_i;
						Im[ii] = 0;
					}
				}

				N = i + 1;				
				i--;
				continue;
			}
			else
			{
				// removing from left side				
				for (j = 0; j <= i; j++)
				{
					pFigure = ppFigures[j];
					PA = pFigure->m_PointsArray;

					for (l = 0; l < pFigure->m_Square; l++)
					{
						ii = PA[l].m_i;
						Im[ii] = 0;
					}
				}
				for (j = i + 1, k = 0; j < N; j++, k++)
				{
					ppFigures[k] = ppFigures[j];
				}

				break;				
			}
		}
		i--;
	}
}

int ClearImageFromSmallSymbols(custom_buffer<int> &Im, int w, int h, int W, int H, int white)
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

	//CombineFiguresRelatedToEachOther2(ppFigures, N);

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->m_h < g_msh*(double)H) ||
			(pFigure->m_w <= g_segh) ||
			(pFigure->m_h <= g_segh) //||
			//(pFigure->m_Square < g_msd*(pFigure->m_h*pFigure->m_w))
			)
		{
			PA = pFigure->m_PointsArray;

			/*
#ifdef DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif			
			*/

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

	if (g_show_results) SaveGreyscaleImage(Im, "/TestImages/ClearImageFromSmallSymbols_01" + g_im_save_format, w, h);

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

	if (g_show_results) SaveGreyscaleImage(Im, "/TestImages/RestoreStillExistLines_01" + g_im_save_format, w, h);
}

int GetAllInsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, custom_buffer<CMyClosedFigure> &pFigures, custom_buffer<CMyClosedFigure*> &ppFigures, int &N, int w, int h, int white)
{
	CMyClosedFigure *pFigure;
	int i, j, l, x, y, ii, cnt;
	CMyPoint *PA;

	memset(&ImRes[0], 0, w*h * sizeof(int));

	{
		custom_buffer<int> ImTMP(w * h, 0);

		for (i = 0; i < w*h; i++)
		{
			if (Im[i] == white)
			{
				ImTMP[i] = white;
			}
		}

		SearchClosedFigures(ImTMP, w, h, 0, pFigures);
		N = pFigures.size();
	}

	if (N == 0)	return 0;

	ppFigures.set_size(N);

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
			(pFigure->m_maxY == h - 1)
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

	return N;
}

void ConvertCMyClosedFigureToSubImage(CMyClosedFigure *pFigure, custom_buffer<int> &ImRes, int w, int h, int ww, int hh, int min_x, int min_y, int white)
{
	int ii, l, x, y;
	CMyPoint *PA = pFigure->m_PointsArray;

	for (l = 0; l < pFigure->m_Square; l++)
	{
		x = PA[l].m_x;
		y = PA[l].m_y;

		ii = (y - min_y)*ww + x - min_x;
		ImRes[ii] = white;
	}
}

void AddSubImageToImage(custom_buffer<int> &ImRes, custom_buffer<int> &ImSub, int w, int h, int ww, int hh, int min_x, int min_y, int white)
{
	int i, ii, x, y;

	for (y = 0, i = 0; y < hh; y++)
	{
		for (x = 0; x < ww; x++, i++)
		{
			if (ImSub[i] != 0)
			{
				ii = (y + min_y)*w + x + min_x;
				ImRes[ii] = white;
			}
		}
	}
}

int GetImageWithInsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white, bool simple)
{
	CMyClosedFigure *pFigure;
	int i, j, l, x, y, ii, cnt, N;
	CMyPoint *PA;
	custom_buffer<CMyClosedFigure> pFigures;
	custom_buffer<CMyClosedFigure*> ppFigures;
	custom_buffer<int> ImIntRes(w * h, 0);

	GetAllInsideFigures(Im, ImRes, pFigures, ppFigures, N, w, h, white);		

	if (simple)
	{
		return N;
	}

	// Removing all inside figures which are inside others

	concurrency::parallel_for(0, N, [&ImIntRes, &ppFigures, N, w, h, white](int i)
	//for (i = 0; i < N; i++)
	{
		bool found = false;

		for (int j = 0; j < N; j++)
		{
			if (i != j)
			{
				if ((ppFigures[i]->m_minX < ppFigures[j]->m_minX) &&
					(ppFigures[i]->m_maxX > ppFigures[j]->m_maxX) &&
					(ppFigures[i]->m_minY < ppFigures[j]->m_minY) &&
					(ppFigures[i]->m_maxY > ppFigures[j]->m_maxY))
				{
					found = true;
					break;
				}
			}
		}
		
		if (found)
		{
			int N1, ww = ppFigures[i]->m_w, hh = ppFigures[i]->m_h, x, y, min_x = ppFigures[i]->m_minX, min_y = ppFigures[i]->m_minY;
			custom_buffer<int> Im1(ww * hh, 0), ImInt1(ww * hh, 0);
			custom_buffer<CMyClosedFigure> pFigures1;
			custom_buffer<CMyClosedFigure*> ppFigures1;

			ConvertCMyClosedFigureToSubImage(ppFigures[i], Im1, w, h, ww, hh, min_x, min_y, white);
			if (GetAllInsideFigures(Im1, ImInt1, pFigures1, ppFigures1, N1, ww, hh, white) > 0)
			{
				AddSubImageToImage(ImIntRes, ImInt1, w, h, ww, hh, min_x, min_y, white);
			}
		}
	});

	/*if (g_show_results)
	{
		custom_buffer<int> ImTMP(w * h, 0);
		GetBinaryImageFromImageByColor(ImTMP, Im, white, w, h);
		SaveGreyscaleImage(ImTMP, "/TestImages/GetImageWithInsideFigures_01_Im" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImRes, "/TestImages/GetImageWithInsideFigures_02_ImInsideFigures" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImIntRes, "/TestImages/GetImageWithInsideFigures_03_ImInsideInsideFigures" + g_im_save_format, w, h);
	}*/

	for (i = 0; i < w*h; i++)
	{
		if (ImRes[i] != 0)
		{
			if (ImIntRes[i] != 0)
			{
				ImRes[i] = 0;
			}
		}
	}
	//if (g_show_results) SaveGreyscaleImage(ImRes, "/TestImages/GetImageWithInsideFigures_04_ImInsideRes" + g_im_save_format, w, h);

	return N;
}

void InvertBinaryImage(custom_buffer<int> &Im, int w, int h)
{
	for (int i = 0; i < w*h; i++)
	{
		if (Im[i] != 0)
		{
			Im[i] = 0;
		}
		else
		{
			Im[i] = 255;
		}
	}
}

int GetImageWithOutsideFigures(custom_buffer<int> &Im, custom_buffer<int> &ImRes, int w, int h, int white)
{
	CMyClosedFigure *pFigure;
	int i, l, x, y, ii, cnt, N;
	CMyPoint *PA;

	memset(&ImRes[0], 0, w*h * sizeof(int));

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
			(pFigure->m_maxY == h - 1)
			)
		{
			PA = pFigure->m_PointsArray;

			for (l = 0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				ImRes[ii] = white;
			}
		}
		else
		{
			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}	

	return N;
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

// note: both images should be binary type of images (not greyscale) (should have 0 or white values of pixels)
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

void ClearImageFromGarbageBetweenLines(custom_buffer<int> &Im, int w, int h, int yb, int ye, int min_h, int white, std::string iter_det)
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

	CombineFiguresRelatedToEachOther(ppFigures, N, min_h, iter_det);

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
	
	fname = g_work_dir + string("/text_lines.info");
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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_01_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_02_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_03_Im" + g_im_save_format, w, h);

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
	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_04_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_05_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_06_Im" + g_im_save_format, w, h);

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

	if (g_show_results) SaveRGBImage(Im, "/TestImages/ClearImageLogical_07_Im" + g_im_save_format, w, h);

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
void LoadRGBImage(custom_buffer<int> &Im, string name, int w, int h)
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
	if (g_disable_save_images) return;

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
	if (g_disable_save_images) return;

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

void SaveBinaryImage(custom_buffer<int> &Im, string name, int w, int h, int quality, int dpi)
{
	if (g_disable_save_images) return;

	cv::Mat im(h, w, CV_8UC3);
	u8 color;

	for (int i = 0; i < w*h; i++)
	{
		if (Im[i] == 0)
		{
			color = 0;
		}
		else
		{
			color = 255;
		}
		im.data[i * 3] = color;
		im.data[i * 3 + 1] = color;
		im.data[i * 3 + 2] = color;
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

void SaveImageWithLinesInfo(custom_buffer<int> &Im, string name, int lb1, int le1, int lb2, int le2, int w, int h)
{
	if (g_disable_save_images) return;

	custom_buffer<int> ImTMP(w*h, 0);
	int x, y;
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


	for (int i = 0; i < w*h; i++)
	{
		if (Im[i] != 0)
		{
			ImTMP[i] = wc;
		}
	}

	for (x = 0; x < w; x++)
	{
		if ((lb1 >= 0) && (lb1 < h)) ImTMP[lb1 * w + x] = cc;
		if ((le1 >= 0) && (le1 < h)) ImTMP[le1 * w + x] = cc;

		if ((lb2 >= 0) && (lb2 < h)) ImTMP[lb2 * w + x] = gc;
		if ((le2 >= 0) && (le2 < h)) ImTMP[le2 * w + x] = gc;
	}

	SaveRGBImage(ImTMP, name, w, h);
}

void SaveImageWithSubParams(custom_buffer<int> &Im, string name, int lb, int le, int LH, int LMAXY, int real_im_x_center, int w, int h)
{
	if (g_disable_save_images) return;

	custom_buffer<int> ImTMP(w*h, 0);
	int x, y;
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


	for (int i = 0; i < w*h; i++)
	{
		if (Im[i] != 0)
		{
			ImTMP[i] = wc;
		}
	}

	for (x = 0; x < w; x++)
	{
		ImTMP[lb * w + x] = cc;
		ImTMP[le * w + x] = cc;

		ImTMP[(LMAXY - LH + 1) * w + x] = gc;
		ImTMP[LMAXY * w + x] = gc;
	}

	if (real_im_x_center < 0) real_im_x_center = 0;
	if (real_im_x_center >= w) real_im_x_center = w - 1;

	for (y = 0; y < h; y++)
	{
		ImTMP[y * w + real_im_x_center] = rc;
	}

	SaveRGBImage(ImTMP, name, w, h);
}

void LoadGreyscaleImage(custom_buffer<int> &Im, string name, int &w, int &h)
{
	cv::Mat im = cv::imread(name, cv::IMREAD_COLOR); // load in BGR format
	w = im.cols;
	h = im.rows;

	custom_assert(Im.m_size >= w * h, "Im.m_size < w*h");

	for (int i = 0; i < w*h; i++)
	{
		if (im.data[i * 3] != 0)
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

void GreyscaleImageToMat(custom_buffer<int> &ImGR, int w, int h, cv::UMat &res)
{
	cv::Mat im(h, w, CV_8UC1);

	for (int i = 0; i < w*h; i++)
	{
		im.data[i] = ImGR[i];
	}

	im.copyTo(res);
}


void GreyscaleMatToImage(cv::Mat &ImGR, int w, int h, custom_buffer<int> &res)
{
	for (int i = 0; i < w*h; i++)
	{
		res[i] = ImGR.data[i];
	}
}

void GreyscaleMatToImage(cv::UMat &ImGR, int w, int h, custom_buffer<int> &res)
{
	cv::Mat im;
	ImGR.copyTo(im);

	for (int i = 0; i < w*h; i++)
	{
		res[i] = im.data[i];
	}
}

// ImBinary - 0 or some_color!=0 (like 0 and 1) 
void BinaryImageToMat(custom_buffer<int> &ImBinary, int w, int h, cv::Mat &res)
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

// ImBinary - 0 or some_color!=0 (like 0 and 1) 
void BinaryImageToMat(custom_buffer<int> &ImBinary, int w, int h, cv::UMat &res)
{
	cv::Mat im(h, w, CV_8UC1);

	for (int i = 0; i < w*h; i++)
	{
		if (ImBinary[i] != 0)
		{
			im.data[i] = 255;
		}
		else
		{
			im.data[i] = 0;
		}
	}

	im.copyTo(res);
}

// ImBinary - 0 or some_color!=0 (like 0 and 1 or 0 and 255) 
void BinaryMatToImage(cv::Mat &ImBinary, int w, int h, custom_buffer<int> &res, int white)
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

// ImBinary - 0 or some_color!=0 (like 0 and 1 or 0 and 255) 
void BinaryMatToImage(cv::UMat &ImBinary, int w, int h, custom_buffer<int> &res, int white)
{
	cv::Mat im;
	ImBinary.copyTo(im);

	for (int i = 0; i < w*h; i++)
	{
		if (im.data[i] != 0)
		{
			res[i] = white;
		}
		else
		{
			res[i] = 0;
		}
	}
}

void RGBMatToImage(cv::UMat &ImRGB, int w, int h, custom_buffer<int> &res)
{
	cv::Mat im;
	ImRGB.copyTo(im);
	memcpy(&res[0], im.data, w*h * sizeof(int));
}

void RGBImageToMat(custom_buffer<int> &ImRGB, int w, int h, cv::UMat &res)
{
	cv::Mat cv_ImRGB(h, w, CV_8UC4);	
	memcpy(cv_ImRGB.data, &ImRGB[0], w*h * sizeof(int));
	cv_ImRGB.copyTo(res);
}

void InvertBinaryMat(cv::UMat &ImBinary, int w, int h, cv::UMat &res)
{
	cv::Mat im;
	ImBinary.copyTo(im);

	for (int i = 0; i < w*h; i++)
	{
		if (im.data[i] == 0)
		{
			im.data[i] = 255;
		}
		else
		{
			im.data[i] = 0;
		}
	}

	im.copyTo(res);
}

void GreyscaleImageToBinary(custom_buffer<int> &ImRES, custom_buffer<int> &ImGR, int w, int h, int white)
{
	for (int i = 0; i < w*h; i++)
	{
		if (ImGR[i] != 0)
		{
			ImRES[i] = white;
		}
		else
		{
			ImRES[i] = 0;
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


void NV12ToBGRA()
{
	/*cv::Mat in_mat_y(m_origHeight, m_origWidth, CV_8U);
	cv::Mat in_mat_uv(m_origHeight/2, m_origWidth/2, CV_8UC2);
	cv::Mat mat_res;

	//custom_assert(m_origWidth == tmp_frame->linesize[0], "m_origWidth == tmp_frame->linesize[0]");
	//custom_assert((m_origWidth / 2) * 2 == tmp_frame->linesize[1], "m_origWidth == tmp_frame->linesize[1]");
	//custom_assert(m_Width * 4 == dst_linesize[0], "m_Width * 4 == dst_linesize[0]");

	memcpy(in_mat_y.data, tmp_frame->data[0], m_origHeight * m_origWidth);
	memcpy(in_mat_uv.data, tmp_frame->data[1], (m_origHeight/2) * (m_origWidth/2) * 2);

	//cv::UMat in_umat_y;
	//cv::UMat in_umat_uv;
	//cv::UMat umat_res;

	//in_mat_y.copyTo(in_umat_y);
	//in_mat_uv.copyTo(in_umat_uv);

	cv::cvtColorTwoPlane(in_mat_y, in_mat_uv, mat_res, cv::COLOR_YUV2BGRA_NV12);

	if ((m_origWidth != m_Width) || (m_origHeight != m_Height))
	{
		cv::resize(mat_res, mat_res, cv::Size(m_Width, m_Height));
	}

	//umat_res.copyTo(mat_res);

	memcpy(dst_data[0], mat_res.data, m_Height*m_Width * 4);*/
}