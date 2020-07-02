                              //IPAlgorithms.h//                                
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

#include "SSAlgorithms.h"
#include "DataTypes.h"
#include "MyClosedFigure.h"
#include <string>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs/legacy/constants_c.h>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

using namespace std;

#include <stdio.h>

extern void     (*g_pViewRGBImage)(simple_buffer<int> &Im, int w, int h);
extern void     (*g_pViewImage[2])(simple_buffer<int> &Im, int w, int h);
extern void     (*g_pViewGreyscaleImage[2])(simple_buffer<u8>& ImGR, int w, int h);
extern void     (*g_pViewBGRImage[2])(simple_buffer<u8>& ImBGR, int w, int h);

extern string   g_work_dir;
extern string   g_app_dir;
extern string   g_im_save_format;

extern double	g_smthr;  //moderate threshold for scaled image
extern double	g_mthr;  //moderate threshold
extern double	g_mnthr; //moderate threshold for NEdges
extern int		g_segw;  //segment width
extern int		g_segh;  //segment height
extern int		g_msegc; //minimum segments count
extern int		g_scd;   //min sum color diff
extern double	g_btd;   //between text distace
extern double	g_tco;   //text centre offset

extern int		g_mpn;	 //min points number
extern double	g_mpd;   //min points density
extern double   g_msh;	 //min symbol height in percents to full image height
extern double   g_msd;   //min symbol density (percent of its pixels / w*h of symbol)

extern double	g_mpned; //min NEdges points density

extern int		g_scale;

extern bool		g_clear_image_logical;

extern bool		g_generate_cleared_text_images_on_test;
extern bool		g_show_results;
extern bool		g_show_sf_results;
extern bool		g_clear_test_images_folder;
extern bool		g_show_transformed_images_only;

extern int		g_dmaxy;

extern bool		g_use_ocl;
extern bool		g_use_cuda_gpu;
extern int		g_cuda_kmeans_initial_loop_iterations;
extern int		g_cuda_kmeans_loop_iterations;

extern int		g_cpu_kmeans_initial_loop_iterations;
extern int		g_cpu_kmeans_loop_iterations;

extern int		g_min_alpha_color;

extern bool		g_use_gradient_images_for_clear_txt_images;
extern bool		g_clear_txt_images_by_main_color;
extern bool		g_use_ILA_images_for_clear_txt_images;

// Settings for logical filtering
extern bool g_remove_wide_symbols;

extern bool		g_disable_save_images;

extern bool     g_save_each_substring_separately;
extern bool     g_save_scaled_images;

void BGR_to_YUV(simple_buffer<u8> &ImInBGR, simple_buffer<u8> &ImY, simple_buffer<u8> &ImU, simple_buffer<u8> &ImV, int w, int h);
void YIQ_to_RGB(int Y, int I, int Q, int &R, int &G, int &B, int max_val);
void RGB_to_YIQ(simple_buffer<int> &ImIn, simple_buffer<int> &ImY, simple_buffer<int> &ImI, simple_buffer<int> &ImQ, int w, int h);
void GetGrayscaleImage(simple_buffer<int> &ImIn, simple_buffer<int> &ImY, int w, int h);

void SobelMEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImMOE, int w, int h);
void ImprovedSobelMEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImMOE, int w, int h);
void SobelHEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImHOE, int w, int h);
void FastImprovedSobelHEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImHOE, int w, int h);
void FastSobelVEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImVOE, int w, int h);
void FastImprovedSobelVEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImVOE, int w, int h);
void SobelNEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImNOE, int w, int h);
void FastImprovedSobelNEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImNOE, int w, int h);
void SobelSEdge(simple_buffer<int> &ImIn, simple_buffer<int> &ImSOE, int w, int h);

void IncreaseContrastOperator(simple_buffer<int> &ImIn, simple_buffer<int> &ImRES, int w, int h);

void FindAndApplyLocalThresholding(simple_buffer<int> &Im, int dw, int dh, int w, int h);
void ApplyModerateThresholdBySplit(simple_buffer<int> &Im, double mthr, int w, int h, int W, int H);
void ApplyModerateThreshold(simple_buffer<int> &Im, double mthr, int w, int h);

void AplyESS(simple_buffer<int> &ImIn, simple_buffer<int> &ImOut, int w, int h);
void AplyECP(simple_buffer<int> &ImIn, simple_buffer<int> &ImOut, int w, int h);

void ColorFiltration(simple_buffer<u8> &ImBGR, simple_buffer<int> &LB, simple_buffer<int> &LE, int &N, int w, int h);

template <class T>
void BorderClear(simple_buffer<T> &Im, int dd, int w, int h);
template <class T>
void EasyBorderClear(simple_buffer<T> &Im, int w, int h);

int GetTransformedImage(simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImFF, simple_buffer<u8> &ImSF, simple_buffer<u8> &ImTF, simple_buffer<u8> &ImNE, simple_buffer<u8> &ImY, int w, int h, int W, int H);
int FilterTransformedImage(simple_buffer<u8> &ImFF, simple_buffer<u8> &ImSF, simple_buffer<u8> &ImTF, simple_buffer<u8> &ImNE, simple_buffer<int> &LB, simple_buffer<int> &LE, int N, int w, int h, int W, int H, std::string iter_det);
int FilterImage(simple_buffer<u8> &ImF, simple_buffer<u8> &ImNE, int w, int h, int W, int H, simple_buffer<int> &LB, simple_buffer<int> &LE, int N);

int FindTextLines(simple_buffer<u8>& ImBGR, simple_buffer<u8> &ImClearedText, simple_buffer<u8>& ImF, simple_buffer<u8>& ImNF, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImIL, vector<wxString>& SavedFiles, int W, int H);

void FindMaxStrDistribution(simple_buffer<int> &GRStr, int delta, simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &N, int offset);
void FindMaxStr(simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &max_i, int &max_val, int N);

void ExtendImFWithDataFromImNF(simple_buffer<u8> &ImF, simple_buffer<u8> &ImNF, int w, int h);

void StrAnalyseImage(simple_buffer<u8>& Im, simple_buffer<u8>& ImGR, simple_buffer<int>& GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);

void ClearImage4x4(simple_buffer<int> &Im, int w, int h, int white);
void ClearImageSpecific1(simple_buffer<int> &Im, int w, int h, int yb, int ye, int xb, int xe, int white);
int ClearImageFromBorders(simple_buffer<int> &Im, int w, int h, int ddy1, int ddy2, int white);

int ClearImageLogical(simple_buffer<int> &Im, int w, int h, int LH, int LMAXY, int xb, int xe, int white, int W, int H, int real_im_x_center);

void SaveTextLineParameters(string ImageName, int YB, int LH, int LY, int LXB, int LXE, int LYB, int LYE, int mY, int mI, int mQ, int W, int H);
void GetSymbolAvgColor(CMyClosedFigure *pFigure, simple_buffer<u8> &ImY, simple_buffer<u8> &ImI, simple_buffer<u8> &ImQ);
void GetTextLineParameters(simple_buffer<u8>& Im, simple_buffer<u8>& ImY, simple_buffer<u8>& ImI, simple_buffer<u8>& ImQ, int w, int h, int& LH, int& LMAXY, int& XB, int& XE, int& YB, int& YE, int& mY, int& mI, int& mQ, u8 white);

void ResizeImage4x(simple_buffer<int> &Im, simple_buffer<int> &ImRES, int w, int h);
void SimpleResizeImage4x(simple_buffer<int> &Im, simple_buffer<int> &ImRES, int w, int h);
void ResizeGrayscaleImage4x(simple_buffer<int> &Im, simple_buffer<int> &ImRES, int w, int h);

int CompareTXTImages(simple_buffer<int> &Im1, simple_buffer<int> &Im2, int w1, int h1, int w2, int h2, int YB1, int YB2);

void GetImageSize(string name, int &w, int &h);
void SaveBGRImage(simple_buffer<u8>& ImBGR, string name, int w, int h);
void LoadBGRImage(simple_buffer<u8>& ImBGR, string name);
void LoadRGBImage(simple_buffer<int> &Im, string name, int w, int h);
void SaveGreyscaleImage(simple_buffer<u8>& Im, string name, int w, int h, int add = 0, double scale = 1.0, int quality = -1, int dpi = -1);
void SaveGreyscaleImage(simple_buffer<int>& Im, string name, int w, int h, int add = 0, double scale = 1.0, int quality = -1, int dpi = -1);
template <class T>
void LoadBinaryImage(simple_buffer<T>& Im, string name, int& w, int& h, T white = 255);
template <class T>
void SaveBinaryImage(simple_buffer<T> &Im, string name, int w, int h, int quality = -1, int dpi = -1);
template <class T1, class T2>
void IntersectTwoImages(simple_buffer<T1> &ImRes, simple_buffer<T2> &Im2, int w, int h);
template <class T1, class T2>
void IntersectImages(simple_buffer<T1>& ImRes, simple_buffer<simple_buffer<T2>*>& ImIn, int min_id_im_in, int max_id_im_in, int w, int h);

bool InitCUDADevice();

void RestoreStillExistLines(simple_buffer<u8> &Im, simple_buffer<u8> &ImOrig, int w, int h);

//-----------------------------------------------------------

template <class T1, class T2>
void IntersectTwoImages(simple_buffer<T1>& ImRes, simple_buffer<T2>& Im2, int w, int h)
{
	int i, size;

	size = w * h;
	for (i = 0; i < size; i++)
	{
		if (Im2[i] == 0)
		{
			ImRes[i] = 0;
		}
	}
}

template <class T1, class T2>
void IntersectImages(simple_buffer<T1>& ImRes, simple_buffer<simple_buffer<T2>*>& ImIn, int min_id_im_in, int max_id_im_in, int w, int h)
{
	int i, size, im_id;

	size = w * h;
	for (i = 0; i < size; i++)
	{
		for (im_id = min_id_im_in; (im_id <= max_id_im_in) && ImRes[i]; im_id++)
		{
			if ((*(ImIn[im_id]))[i] == 0)
			{
				ImRes[i] = 0;
			}
		}
	}
}

template <class T>
void SaveBinaryImage(simple_buffer<T>& Im, string name, int w, int h, int quality, int dpi)
{
	if (g_disable_save_images) return;

	cv::Mat im(h, w, CV_8UC3);
	u8 color;

	for (int i = 0; i < w * h; i++)
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

template <class T>
void LoadBinaryImage(simple_buffer<T>& Im, string name, int& w, int& h, T white)
{
	cv::Mat im = cv::imread(name, cv::IMREAD_COLOR); // load in BGR format
	w = im.cols;
	h = im.rows;

	custom_assert(Im.m_size >= w * h, "LoadBinaryImage(simple_buffer<T>& Im, string name, int& w, int& h)\nnot: Im.m_size >= w * h");

	for (int i = 0; i < w * h; i++)
	{
		if (im.data[i * 3] != 0)
		{
			Im[i] = white;
		}
		else
		{
			Im[i] = 0;
		}
	}
}

