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
#include <wx/string.h>
#include <wx/wfstream.h>
#include <fstream>
#include <execution>
#include <algorithm>
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

extern wxString   g_work_dir;
extern wxString   g_app_dir;
extern wxString   g_prev_data_path;
extern wxString   g_im_save_format;

extern double	g_smthr;  //moderate threshold for scaled image
extern double	g_mthr;  //moderate threshold
extern double	g_mnthr; //moderate threshold for NEdges
extern int		g_segw;  //segment width
extern int		g_segh;  //segment height
extern int		g_msegc; //minimum segments count
extern int		g_scd;   //min sum color diff
extern double	g_btd;   //between text distace
extern double	g_to;   //text offset

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

extern wxArrayString g_use_filter_color;
extern wxArrayString g_use_outline_filter_color;
extern std::vector<color_range> g_color_ranges;
extern std::vector<color_range> g_outline_color_ranges;

extern int g_dL_color;
extern int g_dA_color;
extern int g_dB_color;

extern bool g_combine_to_single_cluster;

extern int		g_cuda_kmeans_initial_loop_iterations;
extern int		g_cuda_kmeans_loop_iterations;

extern int		g_cpu_kmeans_initial_loop_iterations;
extern int		g_cpu_kmeans_loop_iterations;

extern int		g_min_alpha_color;

extern bool		g_use_gradient_images_for_clear_txt_images;
extern bool		g_clear_txt_images_by_main_color;
extern bool		g_use_ILA_images_for_clear_txt_images;
extern bool		g_use_ILA_images_before_clear_txt_images_from_borders;
extern bool		g_use_ILA_images_for_getting_txt_symbols_areas;

// Settings for logical filtering
extern bool g_remove_wide_symbols;

extern bool		g_disable_save_images;

extern bool     g_save_each_substring_separately;
extern bool     g_save_scaled_images;
extern bool		g_border_is_darker;

extern TextAlignment g_text_alignment;

extern bool g_extend_by_grey_color;
extern int g_allow_min_luminance;

std::vector<color_range> GetColorRanges(wxArrayString& filter_colors);

bool PixelColorIsInRange(std::vector<color_range> &color_ranges, simple_buffer<u8>* pImBGR, simple_buffer<u8>* pImLab, int w, int h, int p_id);

void BGRToYUV(u8 b, u8 g, u8 r, u8* py, u8* pu = NULL, u8* pv = NULL);
void YUVToBGR(u8 y, u8 u, u8 v, u8& b, u8& g, u8& r);

void BGRToLab(u8 b, u8 g, u8 r, u8* p_l_lab, u8* p_a_lab, u8* p_b_lab);
void LabToBGR(u8 l_lab, u8 a_lab, u8 b_lab, u8& b, u8& g, u8& r);

template <class T>
void FindAndApplyLocalThresholding(simple_buffer<T>& Im, int dw, int dh, int w, int h);

template <class T>
void ApplyModerateThreshold(simple_buffer<T> &Im, double mthr, int w, int h);

void AplyESS(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h);
void AplyECP(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h);

void ColorFiltration(simple_buffer<u8> &ImBGR, simple_buffer<int> &LB, simple_buffer<int> &LE, int &N, int w, int h);

template <class T>
void BorderClear(simple_buffer<T> &Im, int dd, int w, int h);
template <class T>
void EasyBorderClear(simple_buffer<T> &Im, int w, int h);

int GetTransformedImage(simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImFF, simple_buffer<u8> &ImSF, simple_buffer<u8> &ImTF, simple_buffer<u8> &ImNE, simple_buffer<u8> &ImY, int w, int h, int W, int H, int min_x, int max_x);
int FilterTransformedImage(simple_buffer<u8> &ImFF, simple_buffer<u8> &ImSF, simple_buffer<u8> &ImTF, simple_buffer<u8> &ImNE, simple_buffer<int> &LB, simple_buffer<int> &LE, int N, int w, int h, int W, int H, int min_x, int max_x, wxString iter_det);
int FilterImage(simple_buffer<u8> &ImF, simple_buffer<u8> &ImNE, int w, int h, int W, int H, int min_x, int max_x, simple_buffer<int> &LB, simple_buffer<int> &LE, int N);

wxString FormatImInfoAddData(int W, int H, int xmin, int ymin, int w, int h, int ln = 0);

void GetImInfo(wxString FileName, int img_real_w, int img_real_h, int* pW = NULL, int* pH = NULL, int* pmin_x = NULL, int* pmax_x = NULL, int* pmin_y = NULL, int* pmax_y = NULL, wxString* pBaseName = NULL, int* pScale = NULL);
bool DecodeImData(wxString FileName, int* pW = NULL, int* pH = NULL, int* pmin_x = NULL, int* pmin_y = NULL, int* p_w = NULL, int* p_h = NULL, int* p_ln = NULL, wxString* pBaseName = NULL);

int FindTextLines(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImClearedTextScaled, simple_buffer<u8>& ImF, simple_buffer<u8>& ImNF, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImIL, wxString SaveDir, wxString SaveName, const int w_orig, const int h_orig, const int W_orig, const int H_orig, const int xmin_orig, const int ymin_orig);

void FindMaxStrDistribution(simple_buffer<int> &GRStr, int delta, simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &N, int offset);
void FindMaxStr(simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &max_i, int &max_val, int N);

void ExtendImFWithDataFromImNF(simple_buffer<u8> &ImF, simple_buffer<u8> &ImNF, int w, int h);

void StrAnalyseImage(simple_buffer<u8>& Im, simple_buffer<u8>& ImGR, simple_buffer<int>& GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);

void GetSymbolAvgColor(CMyClosedFigure* pFigure, simple_buffer<u8>& ImY, simple_buffer<u8>& ImI, simple_buffer<u8>& ImQ, int& mY, int& mI, int& mQ, int& weight, int W, int H);
void GetTextLineParameters(simple_buffer<u8>& Im, simple_buffer<u8>& ImY, simple_buffer<u8>& ImI, simple_buffer<u8>& ImQ, int w, int h, int& LH, int& LMAXY, int& XB, int& XE, int& YB, int& YE, int& mY, int& mI, int& mQ, u8 white);

void GetImageSize(wxString name, int &w, int &h);
void SaveBGRImage(simple_buffer<u8>& ImBGR, wxString name, int w, int h);
void LoadBGRImage(simple_buffer<u8>& ImBGR, wxString name);
void LoadBGRImage(simple_buffer<u8>& ImBGR, wxString name, int& w, int& h, bool allocate_im_size);
void SaveGreyscaleImage(simple_buffer<u8>& Im, wxString name, int w, int h, int add = 0, double scale = 1.0, int quality = -1, int dpi = -1);
void SaveGreyscaleImage(simple_buffer<int>& Im, wxString name, int w, int h, int add = 0, double scale = 1.0, int quality = -1, int dpi = -1);
template <class T>
void LoadBinaryImage(simple_buffer<T>& Im, wxString name, int& w, int& h, T white = 255, bool allocate_im_size = false);
template <class T>
void CombineTwoImages(simple_buffer<T>& ImRes, simple_buffer<T>& Im2, int w, int h, T white = 255);
template <class T1, class T2>
void IntersectTwoImages(simple_buffer<T1> &ImRes, simple_buffer<T2> &Im2, int w, int h, T1 zero_val = 0);
template <class T1, class T2>
void IntersectImages(simple_buffer<T1>& ImRes, simple_buffer<simple_buffer<T2>*>& ImIn, int min_id_im_in, int max_id_im_in, int w, int h);

bool InitCUDADevice();

void RestoreStillExistLines(simple_buffer<u8> &Im, simple_buffer<u8> &ImOrig, int w, int h, int W, int H);

void ReadFile(cv::Mat& res_data, wxString name);
void WriteFile(std::vector<uchar>& write_data, wxString name);

void GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::Mat& res);
void GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::UMat& res);

void GreyscaleMatToImage(cv::Mat& ImGR, int w, int h, simple_buffer<u8>& res);
void GreyscaleMatToImage(cv::UMat& ImGR, int w, int h, simple_buffer<u8>& res);

void BGRImageToMat(simple_buffer<u8>& ImBGR, int w, int h, cv::Mat& res);
void BGRImageToMat(simple_buffer<u8>& ImBGR, int w, int h, cv::UMat& res);

void BGRMatToImage(cv::Mat& ImBGR, int w, int h, simple_buffer<u8>& res);

void SaveGreyscaleImageWithLinesInfo(simple_buffer<u8>& Im, wxString name, int w, int h, std::vector<std::pair<int, int>> HLPC, std::vector<std::pair<int, int>> VLPC);
void SaveBGRImageWithLinesInfo(simple_buffer<u8>& Im, wxString name, int w, int h, std::vector<std::pair<int, int>> HLPC, std::vector<std::pair<int, int>> VLPC);

enum ColorName { Red, Green, Blue, Yellow, Purple, White };

int GetBGRColor(ColorName cn);

void SetBGRColor(simple_buffer<u8>& ImBGR, int pixel_id, int bgra_color);

//-----------------------------------------------------------

template <class T>
void CombineTwoImages(simple_buffer<T>& ImRes, simple_buffer<T>& Im2, int w, int h, T white)
{
	int i, size;

	size = w * h;
	for (i = 0; i < size; i++)
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

template <class T1, class T2>
void IntersectTwoImages(simple_buffer<T1>& ImRes, simple_buffer<T2>& Im2, int w, int h, T1 zero_val)
{
	int i, size;

	size = w * h;
	for (i = 0; i < size; i++)
	{
		if (Im2[i] == 0)
		{
			ImRes[i] = zero_val;
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
void SaveBinaryImage(simple_buffer<T>& Im, wxString name, int w, int h, int quality = -1, int dpi = -1, T zero_value = 0)
{
	if (g_disable_save_images) return;

	cv::Mat im(h, w, CV_8UC3);
	u8 color;

	for (int i = 0; i < w * h; i++)
	{
		if (Im[i] == zero_value)
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
		std::vector<uchar> write_data;
		cv::imencode(cv::String(g_im_save_format), im, write_data, compression_params);
		WriteFile(write_data, g_work_dir + name);
	}
	catch (runtime_error& ex) {
		wxString msg;
		msg.Printf(wxT("Exception saving image to %s format: %s\n"), g_im_save_format, wxString(ex.what()));
		wxMessageBox(msg, "ERROR: SaveBinaryImage");
	}
}

template <class T>
void SaveBinaryImage(simple_buffer<T>&& Im, wxString name, int w, int h, int quality = -1, int dpi = -1, T zero_value = 0)
{
	SaveBinaryImage(Im, name, w, h, quality, dpi, zero_value);
}

template <class T>
void LoadBinaryImage(simple_buffer<T>& Im, wxString name, int& w, int& h, T white, bool allocate_im_size)
{
	cv::Mat data;
	ReadFile(data, name);
	cv::Mat im = cv::imdecode(data, cv::IMREAD_COLOR); // load in BGR format
	w = im.cols;
	h = im.rows;

	if (allocate_im_size)
	{
		Im.set_size(w * h);
	}
	else
	{
		custom_assert(Im.m_size >= w * h, "LoadBinaryImage(simple_buffer<T>& Im, wxString name, int& w, int& h)\nnot: Im.m_size >= w * h");
	}

	for (int i = 0; i < w * h; i++)
	{
		if (im.data[i * 3] > 10)
		{
			Im[i] = white;
		}
		else
		{
			Im[i] = 0;
		}
	}
}

template <class T>
void BinaryImageToMat(simple_buffer<T>& ImBinary, int w, int h, cv::Mat& res, T white = 255);
template <class T>
void BinaryImageToMat(simple_buffer<T>& ImBinary, int w, int h, cv::UMat& res, T white = 255);

// ImBinary - 0 or some_color!=0 (like 0 and 1) 
template <class T>
void BinaryImageToMat(simple_buffer<T>& ImBinary, int w, int h, cv::Mat& res, T white)
{
	res = cv::Mat(h, w, CV_8UC1);

	for (int i = 0; i < w * h; i++)
	{
		if (ImBinary[i] != 0)
		{
			res.data[i] = white;
		}
		else
		{
			res.data[i] = 0;
		}
	}
}

// ImBinary - 0 or some_color!=0 (like 0 and 1) 
template <class T>
void BinaryImageToMat(simple_buffer<T>& ImBinary, int w, int h, cv::UMat& res, T white)
{
	cv::Mat im(h, w, CV_8UC1);

	for (int i = 0; i < w * h; i++)
	{
		if (ImBinary[i] != 0)
		{
			im.data[i] = white;
		}
		else
		{
			im.data[i] = 0;
		}
	}

	im.copyTo(res);
}

// ImBinary - 0 or some_color!=0 (like 0 and 1 or 0 and 255)
template <class T>
void BinaryMatToImage(cv::Mat& ImBinary, int w, int h, simple_buffer<T>& res, T white)
{
	for (int i = 0; i < w * h; i++)
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
template <class T>
void BinaryMatToImage(cv::UMat& ImBinary, int w, int h, simple_buffer<T>& res, T white)
{
	cv::Mat im;
	ImBinary.copyTo(im);

	for (int i = 0; i < w * h; i++)
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

template <class T>
int GetAllInsideFigures(simple_buffer<T>& Im, simple_buffer<T>& ImRes, custom_buffer<CMyClosedFigure>& pFigures, simple_buffer<CMyClosedFigure*>& ppFigures, int& N, int w, int h, T white, bool im_is_white)
{
	CMyClosedFigure* pFigure;
	int i, j, l, x, y, ii, cnt;

	ImRes.set_values(0, w * h);

	if (im_is_white)
	{
		SearchClosedFigures(Im, w, h, (T)0, pFigures, false);
		N = pFigures.size();
	}
	else
	{
		simple_buffer<T> ImTMP(w * h, (T)0);

		for (i = 0; i < w * h; i++)
		{
			if (Im[i] == white)
			{
				ImTMP[i] = white;
			}
		}

		SearchClosedFigures(ImTMP, w, h, (T)0, pFigures, false);
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

		if ((pFigure->m_minX == 0) || (pFigure->m_maxX == w - 1) || (pFigure->m_minY == 0) || (pFigure->m_maxY == h - 1))
		{
			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		else
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				ImRes[ii] = white;
			}
		}
		i++;
	}

	return N;
}

template <class T>
void ConvertCMyClosedFigureToSubImage(CMyClosedFigure* pFigure, simple_buffer<T>& ImRes, int w, int h, int ww, int hh, int min_x, int min_y, T white)
{
	int ii, l, x, y;

	for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
	{
		custom_assert(w > 0, "ConvertCMyClosedFigureToSubImage: w > 0");
		y = pFigure->m_PointsArray[l] / w;
		x = pFigure->m_PointsArray[l] - (y * w);

		ii = (y - min_y) * ww + x - min_x;
		ImRes[ii] = white;
	}
}

template <class T>
void AddSubImageToImage(simple_buffer<T>& ImRes, simple_buffer<T>& ImSub, int w, int h, int ww, int hh, int min_x, int min_y, T white)
{
	int i, ii, x, y;

	for (y = 0, i = 0; y < hh; y++)
	{
		for (x = 0; x < ww; x++, i++)
		{
			if (ImSub[i] != 0)
			{
				ii = (y + min_y) * w + x + min_x;
				ImRes[ii] = white;
			}
		}
	}
}

template <class T>
int GetImageWithInsideFigures(simple_buffer<T>& Im, simple_buffer<T>& ImRes, int w, int h, T white, bool simple, bool im_is_white = false)
{
	CMyClosedFigure* pFigure;
	int i, j, l, x, y, ii, cnt, N;
	custom_buffer<CMyClosedFigure> pFigures;
	simple_buffer<CMyClosedFigure*> ppFigures;	

	GetAllInsideFigures(Im, ImRes, pFigures, ppFigures, N, w, h, white, im_is_white);

	if (simple)
	{
		return N;
	}

	// Removing all inside figures which are inside others

	simple_buffer<T> ImIntRes(w * h, (T)0);

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(N), [&ImIntRes, &ppFigures, N, w, h, white, im_is_white](int i)
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
				// 'j' is inside 'i'

				int N1, ww = ppFigures[i]->width(), hh = ppFigures[i]->height(), x, y, min_x = ppFigures[i]->m_minX, min_y = ppFigures[i]->m_minY;
				simple_buffer<T> Im1(ww * hh, (T)0), ImInt1(ww * hh, (T)0);
				custom_buffer<CMyClosedFigure> pFigures1;
				simple_buffer<CMyClosedFigure*> ppFigures1;

				ConvertCMyClosedFigureToSubImage(ppFigures[i], Im1, w, h, ww, hh, min_x, min_y, white);
				if (GetAllInsideFigures(Im1, ImInt1, pFigures1, ppFigures1, N1, ww, hh, white, true) > 0)
				{
					AddSubImageToImage(ImIntRes, ImInt1, w, h, ww, hh, min_x, min_y, white);
				}
			}
		});

	for (i = 0; i < w * h; i++)
	{
		if (ImRes[i] != 0)
		{
			if (ImIntRes[i] != 0)
			{
				ImRes[i] = 0;
			}
		}
	}

	return N;
}

template <class T>
void GetMaskByPixelColorIsInRange(std::vector<color_range>& color_ranges, simple_buffer<T>& ImRes, simple_buffer<u8>* pImBGR, simple_buffer<u8>* pImLab, int w, int h, T white = 255, T black = 0)
{
	if (color_ranges.size() > 0)
	{
		for (int i = 0; i < w * h; i++)
		{
			if (PixelColorIsInRange(color_ranges, pImBGR, pImLab, w, h, i))
			{
				ImRes[i] = white;
			}
			else
			{
				ImRes[i] = black;
			}
		}
	}
}

template <class T>
int FilterImageByPixelColorIsInRange(simple_buffer<T>& ImRes, simple_buffer<u8>* pImBGR, simple_buffer<u8>* pImLab, int w, int h, wxString iter_det = wxT(""), T zero_value = 0, bool extend_inline = false, bool extend_outline = false)
{
	int res = 1;
	bool show_results = g_show_results && (iter_det.size() > 0);

	if (g_outline_color_ranges.size() > 0)
	{
		simple_buffer<u8> ImMASK(w * h), ImInside(w * h);
		GetMaskByPixelColorIsInRange(g_outline_color_ranges, ImMASK, pImBGR, pImLab, w, h);

		if (show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_01_01_ImMASK" + g_im_save_format, w, h);

		if (extend_outline)
		{
			CMyClosedFigure* pFigure;
			custom_buffer<CMyClosedFigure> pFigures;

			SearchClosedFigures(ImMASK, w, h, (u8)255, pFigures);
			int N = pFigures.size();						

			if (N > 0)
			{

				for (int i = 0; i < N; i++)
				{
					pFigure = &(pFigures[i]);

					for (int y = pFigure->m_minY; y <= pFigure->m_maxY; y++)
					{
						for (int x = pFigure->m_minX; x <= pFigure->m_maxX; x++)
						{
							ImMASK[(y * w) + x] = 255;
						}
					}
				}

				if (show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_01_02_ImMASKExt" + g_im_save_format, w, h);
				if (show_results) SaveBinaryImage(ImRes, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_01_03_ImRes" + g_im_save_format, w, h, -1, -1, zero_value);
				IntersectTwoImages(ImRes, ImMASK, w, h, zero_value);
				if (show_results) SaveBinaryImage(ImRes, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_01_04_ImResIntImMSK" + g_im_save_format, w, h, -1, -1, zero_value);
			}
			else
			{
				ImRes.set_values(zero_value, w * h);
				res = 0;
			}			
		}
		else
		{
			simple_buffer<u8> ImInside(w * h);

			if (GetImageWithInsideFigures(ImMASK, ImInside, w, h, (u8)255, true, true) > 0)
			{
				if (show_results) SaveGreyscaleImage(ImInside, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_02_ImInside" + g_im_save_format, w, h);
				if (show_results) SaveBinaryImage(ImRes, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_03_ImRes" + g_im_save_format, w, h, -1, -1, zero_value);
				IntersectTwoImages(ImRes, ImInside, w, h, zero_value);
				if (show_results) SaveBinaryImage(ImRes, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_04_ImResIntImInside" + g_im_save_format, w, h, -1, -1, zero_value);
			}
			else
			{
				ImRes.set_values(zero_value, w * h);
				res = 0;
			}
		}
	}

	if (res != 0)
	{
		if (g_color_ranges.size() > 0)
		{
			simple_buffer<T> ImTMP, *pIm = &ImRes;

			if (extend_inline)
			{
				ImTMP = ImRes;
				pIm = &ImTMP;
			}

			int cnt = 0;
			for (int i = 0; i < w * h; i++)
			{
				if ((*pIm)[i] != zero_value)
				{
					if (PixelColorIsInRange(g_color_ranges, pImBGR, pImLab, w, h, i))
					{
						cnt++;
					}
					else
					{
						(*pIm)[i] = zero_value;
					}
				}
			}
			if (show_results) SaveBinaryImage(*pIm, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_05_ImRes" + g_im_save_format, w, h, -1, -1, zero_value);

			if (extend_inline)
			{
				cvMAT cv_im_gr;
				BinaryImageToMat(ImTMP, w, h, cv_im_gr);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_im_gr, w, h, ImTMP, (T)255);
				IntersectTwoImages(ImRes, ImTMP, w, h);
				if (show_results) SaveBinaryImage(ImRes, "/DebugImages/FilterImageByPixelColorIsInRange_" + iter_det + "_06_ImResDilate" + g_im_save_format, w, h, -1, -1, zero_value);
			}

			if (cnt == 0)
			{
				res = 0;
			}
		}
	}

	return res;
}