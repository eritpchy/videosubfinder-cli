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
#include <emmintrin.h>
#include <wx/wx.h>
#include <wx/regex.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#ifdef USE_CUDA
#include "cuda_kernels.h"
#endif

using namespace std;

void ClearMainClusterImage(simple_buffer<u8>& ImMainCluster, simple_buffer<u8> ImMASK, simple_buffer<u8>& ImIL, int w, int h, int W, int H, int LH, wxString iter_det);
int CheckOnSubPresence(simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImFRes, int w_scaled, int h_scaled, int w_orig, int h_orig, int W_orig, int H_orig, int xb, int yb, wxString iter_det);
void SaveBinaryImageWithLinesInfo(simple_buffer<u8> &Im, wxString name, int lb1, int le1, int lb2, int le2, int w, int h);

template <class T>
void ExtendByInsideFigures(simple_buffer<T> &ImRES, int w, int h, T white = 255, bool simple = true);
void SaveGreyscaleImageWithSubParams(simple_buffer<u8>& Im, wxString name, int lb, int le, int LH, int LMAXY, int real_im_x_center, int w, int h);

template <class T>
int ClearImageByTextDistance(simple_buffer<T>& Im, int w, int h, int W, int H, int real_im_x_center, T white, wxString iter_det);

int ClearImageOptimal2(simple_buffer<u8> &Im, int w, int h, int min_x, int max_x, int min_y, int max_y, u8 white);
void GetMainColorImage(simple_buffer<u8>& ImRES, simple_buffer<u8>* pImMainCluster, simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImY, simple_buffer<u8>& ImU, simple_buffer<u8>& ImV, int w, int h, wxString iter_det, int min_x, int max_x, int min_y, int max_y);
template <class T>
void GreyscaleImageToBinary(simple_buffer<T> &ImRES, simple_buffer<T> &ImGR, int w, int h, T white);
template <class T>
int GetImageWithOutsideFigures(simple_buffer<T>& Im, simple_buffer<T>& ImRes, int w, int h, T white);
template <class T>
void MergeImagesByIntersectedFigures(simple_buffer<T> &ImInOut, simple_buffer<T> &ImIn2, int w, int h, T white);
void(*g_pViewRGBImage)(simple_buffer<int> &Im, int w, int h);
void(*g_pViewImage[2])(simple_buffer<int> &Im, int w, int h);
void(*g_pViewGreyscaleImage[2])(simple_buffer<u8>& ImGR, int w, int h);
void(*g_pViewBGRImage[2])(simple_buffer<u8>& ImBGR, int w, int h);
int GetSubParams(simple_buffer<u8>& Im, int w, int h, u8 white, int& LH, int& LMAXY, int& lb, int& le, int min_h, int real_im_x_center, int yb, int ye, wxString iter_det, bool combine_figures_related_to_each_other = true);
int ClearImageFromMainSymbols(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, u8 white, wxString iter_det);
int ClearImageOpt2(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, u8 white, wxString iter_det);
int ClearImageOpt3(simple_buffer<u8> &Im, int w, int h, int real_im_x_center, u8 white);
int ClearImageOpt4(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, u8 white);
int ClearImageOpt5(simple_buffer<u8> &Im, int w, int h, int LH, int LMAXY, int real_im_x_center, u8 white);
int ClearImageByMask(simple_buffer<u8> &Im, simple_buffer<u8> &ImMASK, int w, int h, u8 white, double thr = 0.2);
int ClearImageOptimal(simple_buffer<u8> &Im, int w, int h, u8 white);
void CombineFiguresRelatedToEachOther(simple_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h, wxString iter_det);
int ClearImageFromSmallSymbols(simple_buffer<u8> &Im, int w, int h, int W, int H, u8 white);
int SecondFiltration(simple_buffer<u8> &Im, simple_buffer<u8> &ImNE, simple_buffer<int> &LB, simple_buffer<int> &LE, int N, int w, int h, int W, int H, int min_x, int max_x);

template <class T>
void InvertBinaryImage(simple_buffer<T> &Im, int w, int h);

void GreyscaleImageToBGR(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImGR, int w, int h);

void GetClustersImage(simple_buffer<u8>& ImRES, simple_buffer<char>& labels, int clusterCount, int w, int h);

void SaveBGRImageWithLinesInfo(simple_buffer<u8>& ImBGR, wxString name, simple_buffer<int>& LB, simple_buffer<int>& LE, int& N, int w, int h, int k = -1);

wxString  g_work_dir;
wxString  g_app_dir;
wxString  g_prev_data_path;

wxString  g_im_save_format = ".jpeg";
//wxString  g_im_save_format = ".bmp";

double	g_mthr = 0.4;
double	g_smthr = 0.25;
double	g_mnthr = 0.3;
int		g_segw = 8;
int     g_segh = 3;  
int		g_msegc = 2;
int		g_scd = 800;
double	g_btd = 0.05;
double	g_to = 0.1; // Max Text Offset

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

#define STR_SIZE (256 * 2)

#define MAX_EDGE_STR (11 * 16 * 256)

bool g_use_ILA_images_for_getting_txt_symbols_areas = false;
bool g_use_ILA_images_before_clear_txt_images_from_borders = false;
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

wxArrayString g_use_filter_color;
wxArrayString g_use_outline_filter_color;
std::vector<color_range> g_color_ranges;
std::vector<color_range> g_outline_color_ranges;

int g_dL_color = 40;
int g_dA_color = 30;
int g_dB_color = 30;

bool g_combine_to_single_cluster = false;

int g_cuda_kmeans_initial_loop_iterations = 10;
int g_cuda_kmeans_loop_iterations = 30;
int g_cpu_kmeans_initial_loop_iterations = 10;
int g_cpu_kmeans_loop_iterations = 10;

double g_min_h = 12.0 / 720.0; // ~ min sub height in percents to image height

bool g_remove_wide_symbols = true;

bool g_disable_save_images = false;

bool g_save_each_substring_separately = true;
bool g_save_scaled_images = true;

bool g_border_is_darker  = true;

bool g_extend_by_grey_color = false;
int g_allow_min_luminance = 100;

TextAlignment g_text_alignment = TextAlignment::Center;

int ClearImageLogical(simple_buffer<u8>& Im, int w, int h, int W, int H, int real_im_x_center, int yb, int ye, u8 white, wxString iter_det)
{
	CMyClosedFigure *pFigure, *pFigure2;
	int i, j, l, ii, N;
	int val, lb, le, LH, LMAXY;
	int min_h = g_min_h * H;
	bool bln;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/ClearImageLogical_" + iter_det + "_1_Im" + g_im_save_format, w, h);

	simple_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	for (i = 0; i < N - 1; i++)
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

	i = 1;
	while (i < N)
	{		
		bln = false;

		for (j = 0; j < i; j++)
		{
			// figure 'i' is inside figure 'j'
			if ((ppFigures[i]->m_maxX <= ppFigures[j]->m_maxX) &&
				(ppFigures[i]->m_minX > ppFigures[j]->m_minX) &&
				(ppFigures[i]->m_maxY < ppFigures[j]->m_maxY) &&
				(ppFigures[i]->m_minY > ppFigures[j]->m_minY))
			{				
				bln = true;
				break;
			}
		}		

		if (bln)
		{
			//removing figure 'i'
			pFigure = ppFigures[i];
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im[ii] = 0;
			}

			for (l = i; l < N - 1; l++)
			{
				ppFigures[l] = ppFigures[l + 1];
			}
			
			N--;
		}
		else
		{
			i++;
		}
	}
	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/ClearImageLogical_" + iter_det + "_2_ImFByInsideFigures" + g_im_save_format, w, h);

	simple_buffer<CMyClosedFigure*> ImInfo(w * h, (CMyClosedFigure*)NULL);

	for (i = 0; i < N; i++)
	{
		pFigure = ppFigures[i];
		for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
		{
			ii = pFigure->m_PointsArray[l];
			ImInfo[ii] = pFigure;
		}
	}

	i = 1;
	while (i < N)
	{
		bln = false;

		for (j = 0; j < i; j++)
		{
			bool bln_fi = false;
			bool bln_fj = false;

			// checking is not 'i' figure are partially on top of 'j' from right side and partially insdide of 'j' at same time

			if ((ppFigures[i]->m_minX <= ppFigures[j]->m_maxX) &&
				(ppFigures[i]->m_minX > ppFigures[j]->m_minX) &&
				(ppFigures[i]->m_maxY < ppFigures[j]->m_maxY) &&
				(ppFigures[i]->m_maxY > ppFigures[j]->m_minY))
			{
				for (int x = ppFigures[i]->m_minX; x <= std::min<int>(ppFigures[i]->m_maxX, ppFigures[j]->m_maxX); x++)
				{
					bln_fi = false;
					bln_fj = false;

					for (int y = ppFigures[j]->m_maxY; y >= std::max<int>(ppFigures[i]->m_minY, ppFigures[j]->m_minY); y--)
					{
						ii = (y * w) + x;

						if (ImInfo[ii] == ppFigures[i])
						{
							bln_fi = true;
							break;
						}

						if (ImInfo[ii] == ppFigures[j])
						{
							bln_fj = true;
						}
					}

					if (bln_fi == true)
					{
						break;
					}
				}
			}

			if (bln_fi && bln_fj)
			{
				bln = true;
				break;
			}
		}

		if (bln)
		{
			//removing figure 'i'
			pFigure = ppFigures[i];
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im[ii] = 0;
			}

			for (l = i; l < N - 1; l++)
			{
				ppFigures[l] = ppFigures[l + 1];
			}

			N--;
		}
		else
		{
			i++;
		}
	}
	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/ClearImageLogical_" + iter_det + "_3_ImFByPartiallyTopInsideFigures" + g_im_save_format, w, h);


	i = 1;
	while (i < N)
	{
		bln = false;

		for (j = 0; j < N; j++)
		{
			bool bln_fi = false;
			bool bln_fj = false;

			if (i == j)
			{
				continue;
			}

			// checking is not 'i' figure are partially below of 'j' from left side and partially insdide of 'j' at same time

			if ((ppFigures[i]->m_maxX < ppFigures[j]->m_maxX) &&
				(ppFigures[i]->m_maxX >= ppFigures[j]->m_minX) &&
				(ppFigures[i]->m_minY > ppFigures[j]->m_minY) &&
				(ppFigures[i]->m_minY < ppFigures[j]->m_maxY))
			{
				for (int x = ppFigures[i]->m_maxX; x >= std::max<int>(ppFigures[i]->m_minX, ppFigures[j]->m_minX); x--)
				{
					bln_fi = false;
					bln_fj = false;

					for (int y = ppFigures[j]->m_minY; y <= std::min<int>(ppFigures[i]->m_maxY, ppFigures[j]->m_maxY); y++)
					{
						ii = (y * w) + x;

						if (ImInfo[ii] == ppFigures[i])
						{
							bln_fi = true;
							break;
						}

						if (ImInfo[ii] == ppFigures[j])
						{
							bln_fj = true;
						}
					}

					if (bln_fi == true)
					{
						break;
					}
				}
			}

			if (bln_fi && bln_fj)
			{
				bln = true;
				break;
			}
		}

		if (bln)
		{
			//removing figure 'i'
			pFigure = ppFigures[i];
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im[ii] = 0;
			}

			for (l = i; l < N - 1; l++)
			{
				ppFigures[l] = ppFigures[l + 1];
			}

			N--;
		}
		else
		{
			i++;
		}
	}
	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/ClearImageLogical_" + iter_det + "_4_ImFByPartiallyBelowInsideFigures" + g_im_save_format, w, h);

	/*val = GetSubParams(Im, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

	if (val == 1)
	{
		if (g_show_results) SaveGreyscaleImageWithSubParams(Im, "/DebugImages/ClearImageLogical_" + iter_det + "_2_Im_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
	}*/

	return N;
}

void ConvertToRange(wxString& val, int& res_min, int& res_max)
{
	wxRegEx re1(wxT("^([[:digit:]]+)$"));
	if (re1.Matches(val))
	{
		res_min = res_max = wxAtoi(re1.GetMatch(val, 1));
	}
	else
	{
		wxRegEx re2(wxT("^([[:digit:]]+)-([[:digit:]]+)$"));
		if (re2.Matches(val))
		{
			res_min = wxAtoi(re2.GetMatch(val, 1));
			res_max = wxAtoi(re2.GetMatch(val, 2));
		}
	}
}

void BGRToYUV(u8 b, u8 g, u8 r, u8 *py, u8 *pu, u8 *pv)
{
	cv::Mat3b bgr(cv::Vec3b(b, g, r));
	cv::Mat3b yuv;
	cv::cvtColor(bgr, yuv, cv::COLOR_BGR2YUV);

	if (py != NULL) *py = yuv.data[0];
	if (pu != NULL) *pu = yuv.data[1];
	if (pv != NULL) *pv = yuv.data[2];
}

void YUVToBGR(u8 y, u8 u, u8 v, u8& b, u8& g, u8& r)
{
	cv::Mat3b yuv(cv::Vec3b(y, u, v));
	cv::Mat3b bgr;
	cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR);
	b = bgr.data[0];
	g = bgr.data[1];
	r = bgr.data[2];
}

void BGRToLab(u8 b, u8 g, u8 r, u8* p_l_lab, u8* p_a_lab, u8* p_b_lab)
{
	cv::Mat3b bgr(cv::Vec3b(b, g, r));
	cv::Mat3b lab;
	cv::cvtColor(bgr, lab, cv::COLOR_BGR2Lab);

	if (p_l_lab != NULL) *p_l_lab = lab.data[0];
	if (p_a_lab != NULL) *p_a_lab = lab.data[1];
	if (p_b_lab != NULL) *p_b_lab = lab.data[2];
}

void LabToBGR(u8 l_lab, u8 a_lab, u8 b_lab, u8& b, u8& g, u8& r)
{
	cv::Mat3b lab(cv::Vec3b(l_lab, a_lab, b_lab));
	cv::Mat3b bgr;
	cv::cvtColor(lab, bgr, cv::COLOR_Lab2BGR);
	b = bgr.data[0];
	g = bgr.data[1];
	r = bgr.data[2];
}

std::vector<color_range> GetColorRanges(wxArrayString& filter_colors)
{
	std::vector<color_range> color_ranges;

	for (int ci = 0; ci < filter_colors.size(); ci++)
	{
		wxString filter_color = filter_colors[ci];
		color_range cr;

		//Examples:
		//example 1: r:3 g:253 b:252 L:178
		//example 2: r:3 g:253 b:252 L:170-190
		//example 3: r:3 g:253 b:252
		//example 4: r:0-20 g:230-260 b:240-270

		wxRegEx space(wxT("[[:space:]]"));
		space.ReplaceAll(&filter_color, wxT(""));

		if (filter_color.size() == 0)
		{
			continue;
		}

		wxRegEx reRGB(wxT("^RGB:r:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)g:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)b:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)(L:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+))?$"));
		if (reRGB.Matches(filter_color))
		{
			wxString strR = reRGB.GetMatch(filter_color, 1);
			wxString strG = reRGB.GetMatch(filter_color, 2);
			wxString strB = reRGB.GetMatch(filter_color, 3);
			u8 mid_b, mid_g, mid_r, mid_y;

			cr.m_color_space = ColorSpace::RGB;

			ConvertToRange(strR, cr.m_min_data[0], cr.m_max_data[0]);
			ConvertToRange(strG, cr.m_min_data[1], cr.m_max_data[1]);
			ConvertToRange(strB, cr.m_min_data[2], cr.m_max_data[2]);
			mid_b = (cr.m_min_data[2] + cr.m_max_data[2]) / 2;
			mid_g = (cr.m_min_data[1] + cr.m_max_data[1]) / 2;
			mid_r = (cr.m_min_data[0] + cr.m_max_data[0]) / 2;
			BGRToYUV(mid_b, mid_g, mid_r, &mid_y);

			int min_y, max_y;

			wxString l = reRGB.GetMatch(filter_color, 5);

			if (l.size() > 0)
			{				
				ConvertToRange(l, min_y, max_y);

				if (min_y == max_y)
				{
					int y = min_y;
					min_y = std::max<int>(0, (int)y - g_dL_color);
					max_y = std::min<int>(255, (int)y + g_dL_color);
				}				
			}
			else if ((cr.m_min_data[0] == cr.m_max_data[0]) &&
				(cr.m_min_data[1] == cr.m_max_data[1]) &&
				(cr.m_min_data[2] == cr.m_max_data[2]))
			{
				min_y = std::max<int>(0, (int)mid_y - g_dL_color);
				max_y = std::min<int>(255, (int)mid_y + g_dL_color);
			}
			else
			{
				min_y = max_y = -1;
			}

			if (min_y != max_y)
			{
				{
					u8 y, u, v, b, g, r;
					BGRToYUV(cr.m_min_data[2], cr.m_min_data[1], cr.m_min_data[0], &y, &u, &v);
					y = std::min<u8>(y, min_y);
					YUVToBGR(y, u, v, b, g, r);
					cr.m_min_data[2] = std::min<uchar>(std::min<uchar>(cr.m_min_data[2], b), (u8)std::max<int>(0, (int)mid_b - (int)(mid_y - min_y)));
					cr.m_min_data[1] = std::min<uchar>(std::min<uchar>(cr.m_min_data[1], g), (u8)std::max<int>(0, (int)mid_g - (int)(mid_y - min_y)));
					cr.m_min_data[0] = std::min<uchar>(std::min<uchar>(cr.m_min_data[0], r), (u8)std::max<int>(0, (int)mid_r - (int)(mid_y - min_y)));
				}
				{
					u8 y, u, v, b, g, r;
					BGRToYUV(cr.m_max_data[2], cr.m_max_data[1], cr.m_max_data[0], &y, &u, &v);
					y = std::max<u8>(y, max_y);
					YUVToBGR(y, u, v, b, g, r);
					cr.m_max_data[2] = std::max<uchar>(std::max<uchar>(cr.m_max_data[2], b), (u8)std::min<int>(255, (int)mid_b + (int)(max_y - mid_y)));
					cr.m_max_data[1] = std::max<uchar>(std::max<uchar>(cr.m_max_data[1], g), (u8)std::min<int>(255, (int)mid_g + (int)(max_y - mid_y)));
					cr.m_max_data[0] = std::max<uchar>(std::max<uchar>(cr.m_max_data[0], r), (u8)std::min<int>(255, (int)mid_r + (int)(max_y - mid_y)));
				}
			}

			color_ranges.push_back(cr);
		}
		else
		{
			wxRegEx reLab(wxT("^Lab:l:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)a:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)b:([[:digit:]]+|[[:digit:]]+-[[:digit:]]+)$"));
			if (reLab.Matches(filter_color))
			{
				wxString strL = reLab.GetMatch(filter_color, 1);
				wxString strA = reLab.GetMatch(filter_color, 2);
				wxString strB = reLab.GetMatch(filter_color, 3);

				cr.m_color_space = ColorSpace::Lab;

				ConvertToRange(strL, cr.m_min_data[0], cr.m_max_data[0]);
				ConvertToRange(strA, cr.m_min_data[1], cr.m_max_data[1]);
				ConvertToRange(strB, cr.m_min_data[2], cr.m_max_data[2]);

				if (cr.m_min_data[0] == cr.m_max_data[0])
				{
					int l = cr.m_min_data[0];
					cr.m_min_data[0] = std::max<int>(0, (int)l - g_dL_color);
					cr.m_max_data[0] = std::min<int>(255, (int)l + g_dL_color);
				}

				if (cr.m_min_data[1] == cr.m_max_data[1])
				{
					int a = cr.m_min_data[1];
					cr.m_min_data[1] = std::max<int>(0, (int)a - g_dA_color);
					cr.m_max_data[1] = std::min<int>(255, (int)a + g_dA_color);
				}

				if (cr.m_min_data[2] == cr.m_max_data[2])
				{
					int b = cr.m_min_data[2];
					cr.m_min_data[2] = std::max<int>(0, (int)b - g_dB_color);
					cr.m_max_data[2] = std::min<int>(255, (int)b + g_dB_color);
				}

				color_ranges.push_back(cr);
			}
			else
			{
				wxMessageBox(wxString::Format(wxT("ERROR: Wrong Color Range Format: %s"), filter_colors[ci]), wxT("GetColorRanges"));
			}
		}
	}

	return color_ranges;
}

bool PixelColorIsInRange(std::vector<color_range>& color_ranges, simple_buffer<u8>* pImBGR, simple_buffer<u8>* pImLab, int w, int h, int p_id)
{
	int offset = p_id * 3;
	bool res = false;

	for (int cr_id = 0; cr_id < color_ranges.size(); cr_id++)
	{
		bool bln_lab = false;

		if (color_ranges[cr_id].m_color_space == ColorSpace::RGB)
		{
			custom_assert(pImBGR != NULL, "PixelColorIsInRange: not: pImBGR != NULL");

			if (((*pImBGR)[offset] >= color_ranges[cr_id].m_min_data[2]) && ((*pImBGR)[offset] <= color_ranges[cr_id].m_max_data[2]) &&
				((*pImBGR)[offset + 1] >= color_ranges[cr_id].m_min_data[1]) && ((*pImBGR)[offset + 1] <= color_ranges[cr_id].m_max_data[1]) &&
				((*pImBGR)[offset + 2] >= color_ranges[cr_id].m_min_data[0]) && ((*pImBGR)[offset + 2] <= color_ranges[cr_id].m_max_data[0]))
			{
				res = true;
				break;
			}
		}
		else
		{
			custom_assert(pImLab != NULL, "PixelColorIsInRange: not: pImLab != NULL");

			if (((*pImLab)[offset] >= color_ranges[cr_id].m_min_data[0]) && ((*pImLab)[offset] <= color_ranges[cr_id].m_max_data[0]) &&
				((*pImLab)[offset + 1] >= color_ranges[cr_id].m_min_data[1]) && ((*pImLab)[offset + 1] <= color_ranges[cr_id].m_max_data[1]) &&
				((*pImLab)[offset + 2] >= color_ranges[cr_id].m_min_data[2]) && ((*pImLab)[offset + 2] <= color_ranges[cr_id].m_max_data[2]))
			{
				res = true;
				break;
			}
		}
	}

	return res;
}

void SetBGRColor(simple_buffer<u8>& ImBGR, int pixel_id, int bgra_color)
{
	custom_assert((pixel_id * 3) + 2 <= ImBGR.m_size - 1, "SetBGRColor: not: (pixel_id * 3) + 2 <= ImBGR.m_size - 1");
	memcpy(ImBGR.m_pData + (pixel_id * 3), &bgra_color, 3);
}

inline void SetBGRColor(simple_buffer<u8>& ImBGROut, int pixel_id_out, simple_buffer<u8>& ImBGRIn, int pixel_id_in)
{
	custom_assert((pixel_id_out * 3) + 2 <= ImBGROut.m_size - 1, "SetBGRColor: not: (pixel_id_out * 3) + 2 <= ImBGROut.m_size - 1");
	custom_assert((pixel_id_in * 3) + 2 <= ImBGRIn.m_size - 1, "SetBGRColor: not: (pixel_id_in * 3) + 2 <= ImBGRIn.m_size - 1");
	memcpy(ImBGROut.m_pData + (pixel_id_out * 3), ImBGRIn.m_pData + (pixel_id_in * 3), 3);
}

inline int GetBGRColor(simple_buffer<u8>& ImBGR, int pixel_id)
{
	int bgra_color = 0;	
	custom_assert((pixel_id * 3) + 2 <= ImBGR.m_size - 1, "GetBGRColor: not: (pixel_id * 3) + 2 <= ImBGR.m_size - 1");
	memcpy((u8*)(&bgra_color), ImBGR.m_pData + (pixel_id * 3), 3);

	custom_assert(bgra_color < (int)(1 << 24), "GetBGRColor: not: bgra_color < (int)(1 << 24)");

	return bgra_color;
}

int GetBGRColor(ColorName cn)
{
	int color = 0;
	u8* pClr = (u8*)(&color);

	// https://www.rapidtables.com/web/color/RGB_Color.html

	switch (cn)
	{
		case Red: 
			pClr[2] = 255;
			break;
		case Green: 
			pClr[1] = 255;
			break;
		case Blue: 
			pClr[0] = 255;
			break;
		case Yellow:
			pClr[1] = 255;
			pClr[2] = 255;
			break;
		case Purple:
			pClr[2] = 128;
			pClr[0] = 128;
			break;
		case White:
			pClr[0] = 255;
			pClr[1] = 255;
			pClr[2] = 255;
			break;
	}
	
	return color;
}

bool InitCUDADevice()
{
	bool res = false;
#ifdef USE_CUDA
	int num = GetCUDADeviceCount();
	if (num > 0)
	{
		res = true;
	}
#endif
	return res;
}

inline u8 GetClusterColor(int clusterCount, int cluster_id)
{
	custom_assert(clusterCount > 0, "GetClusterColor: clusterCount > 0");
	u8 white = (255 / clusterCount)*(cluster_id + 1);
	return white;
}

inline int GetClusterId(int clusterCount, u8 color)
{
	custom_assert((clusterCount <= 255) && (clusterCount > 0), "GetClusterId: (clusterCount <= 255) && (clusterCount > 0)");
	int cluster_id = (color / (255 / clusterCount)) - 1;
	return cluster_id;
}

void ColorFiltration(simple_buffer<u8>& ImBGR, simple_buffer<int>& LB, simple_buffer<int>& LE, int& N, int w, int h)
{	
	custom_assert(g_segw > 0, "ColorFiltration: g_segw > 0");
	const int scd = g_scd, segw = g_segw, msegc = g_msegc, mx = (w - 1) / g_segw;
	s64 t1, dt, num_calls;

	custom_assert(ImBGR.size() >= w * h * 3, "ColorFiltration(...)\nnot: ImBGR.size() >= w*h*3");
	custom_assert(LB.size() >= h, "ColorFiltration(...)\nnot: LB.size() >= H");
	custom_assert(LE.size() >= h, "ColorFiltration(...)\nnot: LE.size() >= H");

	simple_buffer<int> line(h, 0);
	
	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(h), [&](int y)
	{
		int ib = w * y;
		int cnt = 0;
		int i, ia, nx, mi, dif, rdif, gdif, bdif;
		int r0, g0, b0, r1, g1, b1;

		for (nx = 0, ia = ib; nx<mx; nx++, ia += segw)
		{			
			b0 = ImBGR[(ia * 3)];
			g0 = ImBGR[(ia * 3) + 1];
			r0 = ImBGR[(ia * 3) + 2];

			mi = ia + segw;
			dif = 0;

			for (i = ia + 1; i <= mi; i++)
			{
				b1 = ImBGR[(i * 3)];
				g1 = ImBGR[(i * 3) + 1];
				r1 = ImBGR[(i * 3) + 2];

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

	simple_buffer<int> lb(h, 0), le(h, 0);
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
	
	if (g_show_results) SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/ColorFiltration_01_ImBGRWithLinesInfo" + g_im_save_format, lb, le, n, w, h);

	int dd, bd, md;

	dd = 12;
	bd = 2 * dd;
	md = g_min_h * h;

	int k = 0;
	int val = lb[0] - dd;
	if (val<0) val = 0;
	LB[0] = val;

	for (int i = 0; i<n - 1; i++)
	{
		if ((lb[i + 1] - le[i] - 1) >= bd)
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

	if ((N > 0) && (g_show_results)) SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/ColorFiltration_02_ImBGRWithLinesInfoUpdated" + g_im_save_format, LB, LE, N, w, h);
}

void ImprovedSobelMEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImMOE, int w, int h)
{
	const int mx = w - 1;
	const int my = h - 1;
	s64 t1, dt, num_calls;

	custom_assert(ImIn.size() >= w*h, "ImprovedSobelMEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImMOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImMOE.size() >= w*h, "ImprovedSobelMEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImMOE, int w, int h)\nnot: ImMOE.size() >= w*h");

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(1), ForwardIteratorForDefineRange<int>(my), [&](int y)
	{
		int x;
		short val, val1, val2, val3, val4, max;
		u8* pIm;
		u16* pImMOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImMOE = &ImMOE[y*w + x]; x < mx; x++, pIm++, pImMOE++)
		{
			// val1 in range [-255, 255]
			// val2 in range [-255, 255]
			// val3 in range [-255, 255]
			// val4 in range [-255, 255]
			// => val and max in range mod([-16*255(=4080), 16*255]) = [0, 16*255] < max(u16)
			// short [-32768, 32767] : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019

			//val1 = lt - rb;
			val1 = (short)(*(pIm - w - 1)) - (short)(*(pIm + w + 1));
			//val2 = rt - lb;
			val2 = (short)(*(pIm - w + 1)) - (short)(*(pIm + w - 1));
			//val3 = mt - mb;
			val3 = (short)(*(pIm - w)) - (short)(*(pIm + w));
			//val4 = lm - rm;
			val4 = (short)(*(pIm - 1)) - (short)(*(pIm + 1));

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

			*pImMOE = (u16)max;
		}
	});
}

void FastImprovedSobelNEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImNOE, int w, int h)
{
	custom_assert(ImIn.size() >= w*h, "FastImprovedSobelNEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImNOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImNOE.size() >= w*h, "FastImprovedSobelNEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImNOE, int w, int h)\nnot: ImNOE.size() >= w*h");

	s64 t1, dt, num_calls;

	const int mx = w - 1;
	const int my = h - 1;

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(1), ForwardIteratorForDefineRange<int>(my), [&](int y)
	{
		int x;
		short val, val1, val2;
		u8* pIm;
		u16* pImNOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImNOE = &ImNOE[y*w + x]; x < mx; x++, pIm++, pImNOE++)
		{
			// val1 in range [-255*2, 255*2]
			// val2 in range [-255, 255]
			// => val in range mod([-16*255(=4080), 16*255]) = [0, 16*255] < max(u16)
			// short [-32768, 32767] : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019

			val1 = (short)(*(pIm - w));
			val2 = (short)(*(pIm - w - 1));

			val1 += (short)(*(pIm - 1)) - (short)(*(pIm + 1));

			val1 -= (short)(*(pIm + w));
			val2 -= (short)(*(pIm + w + 1));

			val = 3 * val1 + 10 * val2;

			if (val < 0) val = -val;
			*pImNOE = (u16)val;
		}
	});
}

void FastImprovedSobelHEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImHOE, int w, int h)
{	
	custom_assert(ImIn.size() >= w * h, "FastImprovedSobelHEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImHOE, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImHOE.size() >= w * h, "FastImprovedSobelHEdge(simple_buffer<u8> &ImIn, simple_buffer<u16> &ImHOE, int w, int h)\nnot: ImHOE.size() >= w*h");

	const int mx = w - 1;
	const int my = h - 1;

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(1), ForwardIteratorForDefineRange<int>(my), [&](int y)
	{
		int x;
		short val, val1, val2;
		u8* pIm;
		u16* pImHOE;

		for (x = 1, pIm = &ImIn[y*w + x], pImHOE = &ImHOE[y*w + x]; x < mx; x++, pIm++, pImHOE++)
		{
			// val1 in range [-255*2, 255*2]
			// val2 in range [-255, 255]
			// => val in range mod([-16*255(=4080), 16*255]) = [0, 16*255] < max(u16)
			// short [-32768, 32767] : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019

			val1 = (short)(*(pIm - w - 1)) + (short)(*(pIm - w + 1));
			val2 = (short)(*(pIm - w));

			val1 -= (short)(*(pIm + w - 1)) + (short)(*(pIm + w + 1));
			val2 -= (short)(*(pIm + w));

			val = 3 * val1 + 10 * val2;

			if (val < 0) val = -val;
			*pImHOE = (u16)val;
		}
	});
}

template <class T>
void FindAndApplyLocalThresholding(simple_buffer<T>& Im, int dw, int dh, int w, int h)
{
	int i, di, ia, da, x, y, nx, ny, mx, my, MX;
	int val, min, max, mid, lmax, rmax, li, ri, thr;
	simple_buffer<int> edgeStr(MAX_EDGE_STR, 0);
	
	custom_assert(Im.size() >= w*h, "FindAndApplyLocalThresholding(simple_buffer<T> &Im, int dw, int dh, int w, int h)\nnot: Im.size() >= w*h");

	MX = 0;

	for(i=0; i<w*h; i++)
	{
		val = Im[i];
		
		if (val > MX) 
		{
			MX = val;			
		}
	}

	custom_assert(dw > 0, "FindAndApplyLocalThresholding: dw > 0");
	custom_assert(dh > 0, "FindAndApplyLocalThresholding: dh > 0");

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

		edgeStr.set_values(0, (MX + 1));
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
		
		edgeStr.set_values(0, (MX + 1));
	}
}

template <class T>
void ApplyModerateThreshold(simple_buffer<T> &Im, double mthr, int w, int h)
{
	T thr;
	T mx = 0;
	T *pIm = &Im[0];
	T *pImMAX = pIm + (w * h);
	
	custom_assert(Im.size() >= w*h, "ApplyModerateThreshold(simple_buffer<T> &Im, double mthr, int w, int h)\nnot: Im.size() >= w*h");

	for(; pIm < pImMAX; pIm++)
	{
		if (*pIm > mx) mx = *pIm;
	}

	thr = (T)((double)mx*mthr);

	for(pIm = &Im[0]; pIm < pImMAX; pIm++)
	{
		if (*pIm < thr) *pIm = 0;
		else *pIm = (T)255;
	}
}

void AplyESS(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)
{
	s64 t1, dt, num_calls;

	custom_assert(ImIn.size() >= w * h, "AplyESS(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImOut.size() >= w * h, "AplyESS(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)\nnot: ImOut.size() >= w*h");

	const int mx = w - 2;
	const int my = h - 2;	

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(2), ForwardIteratorForDefineRange<int>(my), [&](int y)
	{
		int i, x, val;
		
		// max_ImOut == max_ImIn * ((2*4) + (4*8) + (5*4) + (10*4) + (20*4) + 40 == 220)/220 == max_ImIn == MAX_EDGE_STR-1 (11 * 16 * 256 - 1)
		// max_val  == max_ImIn * ((2*4) + (4*8) + (5*4) + (10*4) + (20*4) + 40 == 220) == 220 * max_ImIn == 220 * MAX_EDGE_STR-1 (11 * 16 * 256 - 1) < max(s32)
		for (x = 2, i = w * y + x; x < mx; x++, i++)
		{
			val = 2 * ((int)ImIn[i - w * 2 - 2] + (int)ImIn[i - w * 2 + 2] + (int)ImIn[i + w * 2 - 2] + (int)ImIn[i + w * 2 + 2]) +
				+4 * ((int)ImIn[i - w * 2 - 1] + (int)ImIn[i - w * 2 + 1] + (int)ImIn[i - w - 2] + (int)ImIn[i - w + 2] + (int)ImIn[i + w - 2] + (int)ImIn[i + w + 2] + (int)ImIn[i + w * 2 - 1] + (int)ImIn[i + w * 2 + 1]) +
				+5 * ((int)ImIn[i - w * 2] + (int)ImIn[i - 2] + (int)ImIn[i + 2] + (int)ImIn[i + w * 2]) +
				+10 * ((int)ImIn[i - w - 1] + (int)ImIn[i - w + 1] + (int)ImIn[i + w - 1] + (int)ImIn[i + w + 1]) +
				+20 * ((int)ImIn[i - w] + (int)ImIn[i - 1] + (int)ImIn[i + 1] + (int)ImIn[i + w]) +
				+40 * (int)ImIn[i];

			ImOut[i] = (u16)(val / 220);
		}
	});
}

void AplyECP(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)
{	
	custom_assert(ImIn.size() >= w*h, "AplyECP(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)\nnot: ImIn.size() >= w*h");
	custom_assert(ImOut.size() >= w*h, "AplyECP(simple_buffer<u16> &ImIn, simple_buffer<u16> &ImOut, int w, int h)\nnot: ImOut.size() >= w*h");

	s64 t1, dt, num_calls;

	const int mx = w - 2;
	const int my = h - 2;
	
	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(2), ForwardIteratorForDefineRange<int>(my), [&](int y)
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

			// max_ImOut == max_ImIn * ((8+5+4+5+8)+(5+2+1+2+5)+(4+1+1+4)+(5+2+1+2+5)+(8+5+4+5+8) == 100)/100 == max_ImIn == MAX_EDGE_STR-1 (11 * 16 * 256 - 1)
			// max_val  == max_ImIn * ((2*4) + (4*8) + (5*4) + (10*4) + (20*4) + 40 == 100) == 100 * max_ImIn == 100 * MAX_EDGE_STR-1 (11 * 16 * 256 - 1) < max(s32)

			ii = i - ((w + 1) << 1);
			val = 8 * (int)ImIn[ii] + 5 * (int)ImIn[ii + 1] + 4 * (int)ImIn[ii + 2] + 5 * (int)ImIn[ii + 3] + 8 * (int)ImIn[ii + 4];

			ii += w;
			val += 5 * (int)ImIn[ii] + 2 * (int)ImIn[ii + 1] + (int)ImIn[ii + 2] + 2 * (int)ImIn[ii + 3] + 5 * (int)ImIn[ii + 4];

			ii += w;
			val += 4 * (int)ImIn[ii] + (int)ImIn[ii + 1] + (int)ImIn[ii + 3] + 4 * (int)ImIn[ii + 4];

			ii += w;
			val += 5 * (int)ImIn[ii] + 2 * (int)ImIn[ii + 1] + (int)ImIn[ii + 2] + 2 * (int)ImIn[ii + 3] + 5 * (int)ImIn[ii + 4];

			ii += w;
			val += 8 * (int)ImIn[ii] + 5 * (int)ImIn[ii + 1] + 4 * (int)ImIn[ii + 2] + 5 * (int)ImIn[ii + 3] + 8 * (int)ImIn[ii + 4];

			ImOut[i] = (u16)(val / 100);
		}
	});
}

template <class T>
void BorderClear(simple_buffer<T>& Im, int dd, int w, int h)
{
	int i, di, x, y;

	custom_assert(Im.size() >= w*h, "BorderClear(simple_buffer<T>& Im, int dd, int w, int h)\nnot: Im.size() >= w*h");

	memset(&Im[0], 0, w*dd*sizeof(T));
	memset(&Im[w*(h-dd)], 0, w*dd*sizeof(T));

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

template <class T>
void EasyBorderClear(simple_buffer<T> &Im, int w, int h)
{
	int i, y;

	custom_assert(Im.size() >= w*h, "EasyBorderClear(simple_buffer<T> &Im, int w, int h)\nnot: Im.size() >= w*h");

	memset(&Im[0], 0, w*sizeof(T));
	memset(&Im[w*(h-1)], 0, w*sizeof(T));

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

void GetImCMOEWithThr1(simple_buffer<u16> &ImCMOE, simple_buffer<u16> &ImYMOE, simple_buffer<u16> &ImUMOE, simple_buffer<u16> &ImVMOE, int w, int h, int W, int H, simple_buffer<int> &offsets, simple_buffer<int> &dhs, int N, double mthr)
{
	simple_buffer<u16> ImRES2(w*h), ImRES3(w*h);
	int mx, my, i, k, y, x;
	mx = w - 1;
	my = h - 1;
	i = w + 1;

	EasyBorderClear(ImCMOE, w, h);	

	for (y = 1; y < my; y++, i += 2)
	{
		for (x = 1; x < mx; x++, i++)
		{
			// affects on MAX_EDGE_STR
			// max_ImCMOE = (3 * 16 * 256 - 1)
			ImCMOE[i] = ImYMOE[i] + ImUMOE[i] + ImVMOE[i];
		}
	}

	FindAndApplyLocalThresholding(ImCMOE, w, 32, w, h);	
	
	// max_ImRES2 == max_ImCMOE = (3 * 16 * 256 - 1)
	AplyESS(ImCMOE, ImRES2, w, h);

	BorderClear(ImRES2, 2, w, h);
	// max_ImRES3 == max_ImRES2 == max_ImCMOE = (3 * 16 * 256 - 1)
	AplyECP(ImRES2, ImRES3, w, h);	

	BorderClear(ImCMOE, 2, w, h);

	mx = w - 2;
	my = h - 2;
	i = ((w + 1) << 1);
	for (y = 2; y < my; y++, i += 4)
	{
		for (x = 2; x < mx; x++, i++)
		{
			// max_ImCMOE == (2 * 3 * 16 * 256 - 1 < u16) / 2
			ImCMOE[i] = (ImRES2[i] + ImRES3[i]) / 2;
		}
	}

	for (k = 0; k < N; k++)
	{
		i = offsets[k];
		simple_buffer<u16> ImCMOE_sub_buffer(ImCMOE, i);
		ApplyModerateThreshold(ImCMOE_sub_buffer, mthr, w, dhs[k]);
	}
}


void GetImCMOEWithThr2(simple_buffer<u16> &ImCMOE, simple_buffer<u16> &ImYMOE, simple_buffer<u16> &ImUMOE, simple_buffer<u16> &ImVMOE, int w, int h, int W, int H, simple_buffer<int> &offsets, simple_buffer<int> &dhs, int N, double mthr)
{
	simple_buffer<u16> ImRES5(w*h), ImRES6(w*h);
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
			// affects on MAX_EDGE_STR (11 * 16 * 256)
			// max_ImCMOE = (11 * 16 * 256 - 1)
			ImCMOE[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
		}
	}

	FindAndApplyLocalThresholding(ImCMOE, w, 32, w, h);

	// max_ImRES5 == max_ImCMOE = (11 * 16 * 256 - 1)
	AplyESS(ImCMOE, ImRES5, w, h);

	BorderClear(ImRES5, 2, w, h);
	// max_ImRES6 == max_ImRES5 == max_ImCMOE = (11 * 16 * 256 - 1)
	AplyECP(ImRES5, ImRES6, w, h);

	BorderClear(ImCMOE, 2, w, h);

	mx = w - 2;
	my = h - 2;
	i = ((w + 1) << 1);
	for (y = 2; y < my; y++, i += 4)
	{
		for (x = 2; x < mx; x++, i++)
		{
			// max_ImCMOE == (2 * 11 * 16 * 256 - 1 > u16) / 2
			ImCMOE[i] = (u16)(((int)ImRES5[i] + (int)ImRES6[i]) / 2);
		}
	}

	for (k = 0; k < N; k++)
	{
		i = offsets[k];
		simple_buffer<u16> ImCMOE_sub_buffer(ImCMOE, i);
		ApplyModerateThreshold(ImCMOE_sub_buffer, mthr, w, dhs[k]);
	}
}

void GetImFF(simple_buffer<u8> &ImFF, simple_buffer<u8> &ImSF, simple_buffer<u8> &ImYFull, simple_buffer<u8> &ImUFull, simple_buffer<u8> &ImVFull, simple_buffer<int> &LB, simple_buffer<int> &LE, int N, int w,int h, int W, int H, double mthr)
{	
	simple_buffer<int> offsets(N), cnts(N), dhs(N);
	int i, j, k, cnt, val;
	int x, y, segh;
	int ww, hh;
	s64 t1, dt, num_calls;
	
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

	{
		simple_buffer<u16> ImYMOE(ww * hh), ImUMOE(ww * hh), ImVMOE(ww * hh);

		{
			simple_buffer<u8> ImY(ww * hh), ImU(ww * hh), ImV(ww * hh);

			for (k = 0; k < N; k++)
			{
				i = offsets[k];
				cnt = cnts[k];
				ImY.copy_data(ImYFull, i, w * LB[k], cnt);
				ImU.copy_data(ImUFull, i, w * LB[k], cnt);
				ImV.copy_data(ImVFull, i, w * LB[k], cnt);
			}

			run_in_parallel(
				[&] { ImprovedSobelMEdge(ImY, ImYMOE, ww, hh); },
				[&] { ImprovedSobelMEdge(ImU, ImUMOE, ww, hh); },
				[&] { ImprovedSobelMEdge(ImV, ImVMOE, ww, hh); }
			);
		}

		{
			simple_buffer<u16> ImRES1(ww * hh), ImRES4(ww * hh);

			run_in_parallel(
				[&] { GetImCMOEWithThr1(ImRES4, ImYMOE, ImUMOE, ImVMOE, ww, hh, W, H, offsets, dhs, N, mthr); },
				[&] { GetImCMOEWithThr2(ImRES1, ImYMOE, ImUMOE, ImVMOE, ww, hh, W, H, offsets, dhs, N, mthr); }
			);

			ImFF.set_values(0, w * h);

			i = 0;
			for (k = 0; k < N; k++)
			{
				i = offsets[k];
				cnt = cnts[k];

				for (j = 0; j < cnt; j++)
				{
					if (ImRES1[i + j] || ImRES4[i + j])
					{
						ImFF[(w * LB[k]) + j] = 255;
					}
					else
					{
						ImFF[(w * LB[k]) + j] = 0;
					}
				}
			}
		}
	}

	ImSF.copy_data(ImFF, w * h);	

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
		ImSF.set_values(0, w * (LE[N - 1] + 1), w * val);
	}
}

void GetImNE(simple_buffer<u8> &ImNE, simple_buffer<u8> &ImY, simple_buffer<u8> &ImU, simple_buffer<u8> &ImV, int w, int h)
{	
	simple_buffer<u16> ImRES1(w*h), ImRES2(w*h);
	int k, cnt, val, N, mx, my;
	int segh;
	int ww, hh;
	s64 t1, dt, num_calls;

	EasyBorderClear(ImNE, w, h);
	EasyBorderClear(ImRES1, w, h);
	EasyBorderClear(ImRES2, w, h);

	{
		simple_buffer<u16> ImYMOE(w * h), ImUMOE(w * h), ImVMOE(w * h);

		run_in_parallel(
			[&] { FastImprovedSobelNEdge(ImY, ImYMOE, w, h); },
			[&] { FastImprovedSobelNEdge(ImU, ImUMOE, w, h); },
			[&] { FastImprovedSobelNEdge(ImV, ImVMOE, w, h); }
		);

		mx = w - 1;
		my = h - 1;

		// ImRES1 values in range [0, 3*16*255] < max(u16) : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019
		// ImRES2 values in range [0, 11*16*255] < max(u16) : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019
		run_in_parallel(
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
						ImRES2[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
					}
				}
				ApplyModerateThreshold(ImRES2, g_mnthr, w, h);
			}
		);
	}

	// CombineTwoImages ImRES1 and ImRES2 to ImNE
	{
		int i = w + 1, x, y;
		for (y = 1; y < my; y++, i += 2)
		{
			for (x = 1; x < mx; x++, i++)
			{
				if (ImRES1[i] || ImRES2[i])
				{
					ImNE[i] = 255;
				}
				else
				{
					ImNE[i] = 0;
				}
			}
		}
	}
}

void GetImHE(simple_buffer<u8> &ImHE, simple_buffer<u8> &ImY, simple_buffer<u8> &ImU, simple_buffer<u8> &ImV, int w, int h)
{	
	simple_buffer<u16> ImRES1(w*h), ImRES2(w*h);
	int k, cnt, val, N, mx, my;
	int segh;
	int ww, hh;
	s64 t1, dt, num_calls;

	EasyBorderClear(ImHE, w, h);
	EasyBorderClear(ImRES1, w, h);
	EasyBorderClear(ImRES2, w, h);

	{
		simple_buffer<u16> ImYMOE(w * h), ImUMOE(w * h), ImVMOE(w * h);

		run_in_parallel(
			[&] { FastImprovedSobelHEdge(ImY, ImYMOE, w, h); },
			[&] { FastImprovedSobelHEdge(ImU, ImUMOE, w, h); },
			[&] { FastImprovedSobelHEdge(ImV, ImVMOE, w, h); }
		);

		mx = w - 1;
		my = h - 1;

		// ImRES1 values in range [0, 3*16*255] < max(u16) : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019
		// ImRES2 values in range [0, 11*16*255] < max(u16) : https://docs.microsoft.com/en-us/cpp/cpp/data-type-ranges?view=vs-2019
		run_in_parallel(
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
						ImRES2[i] = ImYMOE[i] + (ImUMOE[i] + ImVMOE[i]) * 5;
					}
				}
				ApplyModerateThreshold(ImRES2, g_mnthr, w, h);
			}
		);
	}

	// CombineTwoImages ImRES1 and ImRES2 to ImHE
	{
		int i = w + 1, x, y;
		for (y = 1; y < my; y++, i += 2)
		{
			for (x = 1; x < mx; x++, i++)
			{
				if (ImRES1[i] || ImRES2[i])
				{
					ImHE[i] = 255;
				}
				else
				{
					ImHE[i] = 0;
				}
			}
		}
	}
}

void FilterImageByNotIntersectedFiguresWithImMask(simple_buffer<u8>& ImInOut, simple_buffer<u8>& ImMASK, int w, int h, u8 white)
{
	CMyClosedFigure* pFigure;
	int i, j, k, l, ii;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;

	t = SearchClosedFigures(ImInOut, w, h, white, pFigures);
	for (i = 0; i < pFigures.size(); i++)
	{
		pFigure = &(pFigures[i]);

		bool found = false;
		for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
		{
			ii = pFigure->m_PointsArray[l];
			if (ImMASK[ii])
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				ImInOut[ii] = 0;
			}
		}
	}
}

int FilterTransformedImage(simple_buffer<u8>& ImFF, simple_buffer<u8>& ImSF, simple_buffer<u8>& ImTF, simple_buffer<u8>& ImNE, simple_buffer<int>& LB, simple_buffer<int>& LE, int N, int w, int h, int W, int H, int min_x, int max_x, wxString iter_det = "main")
{
	simple_buffer<u8> ImRES1(w * h), ImRES2(w * h);
	int res = 0;

	if (g_show_results) SaveGreyscaleImage(ImSF, "/DebugImages/FilterTransformedImage_" + iter_det + "_01_01_ImSF" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImNE, "/DebugImages/FilterTransformedImage_" + iter_det + "_01_02_ImNE" + g_im_save_format, w, h);

	{
		cv::Mat cv_im_gr;
		BinaryImageToMat(ImNE, w, h, cv_im_gr);
		cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), (g_min_h * H) / 2);
		BinaryMatToImage(cv_im_gr, w, h, ImRES1, (u8)255);
		IntersectTwoImages(ImSF, ImRES1, w, h);
	}

	if (g_show_results) SaveGreyscaleImage(ImRES1, "/DebugImages/FilterTransformedImage_" + iter_det + "_02_01_ImNEDilate" + g_im_save_format, w, h);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/DebugImages/FilterTransformedImage_" + iter_det + "_02_02_ImSFMergeImDilate" + g_im_save_format, w, h);

	ImRES1.copy_data(ImSF, w * h);

	res = SecondFiltration(ImSF, ImNE, LB, LE, N, w, h, W, H, min_x, max_x);
	if (g_show_results) SaveGreyscaleImage(ImSF, "/DebugImages/FilterTransformedImage_" + iter_det + "_03_ImSFFSecondFiltration" + g_im_save_format, w, h);

	if (res == 1)
	{
		ImTF.copy_data(ImSF, w * h);
		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_04_01_ImTF" + g_im_save_format, w, h);

		RestoreStillExistLines(ImTF, ImRES1, w, h, W, H);
		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_04_02_ImTFWithRestoredStillExistLines" + g_im_save_format, w, h);

		FilterImageByNotIntersectedFiguresWithImMask(ImTF, ImSF, w, h, (u8)255);
		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_04_03_ImTFFilterImageByNotIntersectedFiguresWithImMask" + g_im_save_format, w, h);

		ImRES2.copy_data(ImTF, w * h);

		res = ClearImageFromSmallSymbols(ImTF, w, h, W, H, 255);
		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_05_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);

		if (res == 1)
		{
			RestoreStillExistLines(ImTF, ImRES2, w, h, W, H);
			if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_08_ImTFWithRestoredStillExistLines" + g_im_save_format, w, h);

			ExtendImFWithDataFromImNF(ImTF, ImRES1, w, h);
			if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FilterTransformedImage_" + iter_det + "_09_ImTFExtByImFF" + g_im_save_format, w, h);
		}
	}

	return res;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int GetTransformedImage(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImFF, simple_buffer<u8>& ImSF, simple_buffer<u8>& ImTF, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImY, int w, int h, int W, int H, int min_x, int max_x)
{
	simple_buffer<int> LB(h, 0), LE(h, 0);
	int N;
	int res = 0;
	s64 t1, dt, num_calls = 100;

	if (g_show_results) SaveBGRImage(ImBGR, "/DebugImages/GetTransformedImage_01_01_ImRGB" + g_im_save_format, w, h);

	ColorFiltration(ImBGR, LB, LE, N, w, h);

	if (N == 0)
	{
		return res;
	}

	if (g_show_results)	SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/GetTransformedImage_01_02_ImRGBWithLinesInfo" + g_im_save_format, LB, LE, N, w, h);

	{
		simple_buffer<u8> ImHE(w * h);

		{
			simple_buffer<u8> ImU(w * h), ImV(w * h);

			{
				cv::Mat cv_Im, cv_ImSplit[3], cv_ImBGR;
				BGRImageToMat(ImBGR, w, h, cv_ImBGR);
				cv::cvtColor(cv_ImBGR, cv_Im, cv::COLOR_BGR2YUV);
				cv::split(cv_Im, cv_ImSplit);
				GreyscaleMatToImage(cv_ImSplit[0], w, h, ImY);
				GreyscaleMatToImage(cv_ImSplit[1], w, h, ImU);
				GreyscaleMatToImage(cv_ImSplit[2], w, h, ImV);
			}

			if (g_show_results)
			{
				SaveGreyscaleImage(ImY, "/DebugImages/GetTransformedImage_01_03_ImY" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImU, "/DebugImages/GetTransformedImage_01_04_ImU" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImV, "/DebugImages/GetTransformedImage_01_05_ImV" + g_im_save_format, w, h);
			}

			run_in_parallel(
				[&] { GetImFF(ImFF, ImSF, ImY, ImU, ImV, LB, LE, N, w, h, W, H, g_mthr); },
				[&] { GetImNE(ImNE, ImY, ImU, ImV, w, h); },
				[&] { GetImHE(ImHE, ImY, ImU, ImV, w, h); }
			);
		}

		if (g_show_results)
		{
			SaveGreyscaleImage(ImFF, "/DebugImages/GetTransformedImage_02_1_ImFF" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImSF, "/DebugImages/GetTransformedImage_02_2_ImSF" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImNE, "/DebugImages/GetTransformedImage_02_3_ImNE" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImHE, "/DebugImages/GetTransformedImage_02_4_ImHE" + g_im_save_format, w, h);
		}

		CombineTwoImages(ImNE, ImHE, w, h);
		if (g_show_results) SaveGreyscaleImage(ImNE, "/DebugImages/GetTransformedImage_02_5_ImNE+HE" + g_im_save_format, w, h);
	}

	res = FilterTransformedImage(ImFF, ImSF, ImTF, ImNE, LB, LE, N, w, h, W, H, min_x, max_x);

	return res;
}

int FilterImage(simple_buffer<u8>& ImF, simple_buffer<u8>& ImNE, int w, int h, int W, int H, int min_x, int max_x, simple_buffer<int>& LB, simple_buffer<int>& LE, int N)
{
	int res;
	simple_buffer<u8> ImRES1(w * h), ImRES2(w * h);
	int diff_found = 1;
	int iter = 0;

	ImRES1.copy_data(ImF, w * h);

	if (g_show_results) SaveGreyscaleImage(ImF, "/DebugImages/FilterImage_01_ImFOrigin" + g_im_save_format, w, h);

	while (diff_found)
	{
		iter++;
		ImRES2.copy_data(ImF, w * h);

		res = SecondFiltration(ImF, ImNE, LB, LE, N, w, h, W, H, min_x, max_x);
		if (g_show_results) SaveGreyscaleImage(ImF, "/DebugImages/FilterImage_iter" + std::to_string(iter) + "_02_ImSFFSecondFiltration" + g_im_save_format, w, h);

		if (res == 1) res = ClearImageFromSmallSymbols(ImF, w, h, W, H, 255);
		if (g_show_results) SaveGreyscaleImage(ImF, "/DebugImages/FilterImage_iter" + std::to_string(iter) + "_04_ImTFClearedFromSmallSymbols" + g_im_save_format, w, h);
		
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

inline bool IsTooRight(int lb, int le, const int to_max2, int real_im_x_center)
{
	return (((lb + le - (real_im_x_center*2))*2 >= to_max2) || (lb >= real_im_x_center));
}

///////////////////////////////////////////////////////////////////////////////
// W - full image include scale (if is) width
// H - full image include scale (if is) height
int SecondFiltration(simple_buffer<u8>& Im, simple_buffer<u8>& ImNE, simple_buffer<int>& LB, simple_buffer<int>& LE, int N, int w, int h, int W, int H, int min_x, int max_x)
{		
	wxString now;
	if (g_show_sf_results) now = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());

	int res = 0;

	const int segw = g_segw;
	const int msegc = g_msegc;
	const int segh = g_segh;
	const int btd_max = (int)(g_btd * (double)W);
	const int to_max = (int)(g_to * (double)W);
	const int to_max2 = 2 * to_max;	
	const int real_im_x_center2 = min_x + max_x;
	const int real_im_x_center = (min_x + max_x) / 2;
	const int mpn = g_mpn;
	const double mpd = g_mpd;
	const double mpned = g_mpned;
	const int da = segh*w;

	if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_01_Im" + g_im_save_format, w, h);
	
	for(int k=0; k<N; k++)
	{
		custom_assert(segh > 0, "SecondFiltration: segh > 0");

		const int ie = (LB[k] + (int)((min(LE[k]+segh,h-1)-LB[k])/segh)*segh)*w;
		int ia = 0;

		// doesn't give difference (work a little longer)
		// std::for_each(std::execution::par, ForwardIteratorForDefineRangeWithStep<int>(LB[k] * w, da), ForwardIteratorForDefineRangeWithStep<int>(ie, da), [&](int ia)
		//{
		for (ia = LB[k] * w; ia < ie; ia += da)
		{
			simple_buffer<int> lb(w, 0), le(w, 0);
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
#ifdef CUSTOM_DEBUG
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

				if (g_text_alignment != TextAlignment::Any)
				{
					do
					{
						ln_start = ln;
						l = ln - 2;
						while ((l >= 0) && (l < (ln - 1)))
						{
							//проверяем расстояние между соседними подстроками
							if ((lb[l + 1] - le[l]) > btd_max)
							{
								if (g_text_alignment == TextAlignment::Center)
								{
									//определяем подстроку наиболее удаленую от центра
									val1 = lb[l] + le[l] - real_im_x_center2;
									val2 = lb[l + 1] + le[l + 1] - real_im_x_center2;
									if (val1 < 0) val1 = -val1;
									if (val2 < 0) val2 = -val2;

									offset = le[l] + lb[l] - real_im_x_center2;
									if (offset < 0) offset = -offset;

									if (IsTooRight(lb[l + 1], le[l + 1], to_max2, real_im_x_center) ||
										(
											(offset <= to_max2) &&
											((le[l + 1] - lb[l + 1]) < ((le[l] - lb[l])))
											)
										)
									{
										ll = l + 1;
									}
									else if (val1 > val2) ll = l;
									else ll = l + 1;
								}
								else if (g_text_alignment == TextAlignment::Left)
								{
									/*if ((lb[l + 1] <= to_max) &&
										((le[l] - lb[l]) < ((le[l + 1] - lb[l + 1]))))
									{
										ll = l;
									}
									else*/
									{
										ll = l + 1;
									}
								}
								else
								{
									/*if ((w - le[l] <= to_max) &&
										((le[l + 1] - lb[l + 1]) < ((le[l] - lb[l]))))
									{
										ll = l + 1;
									}
									else*/
									{
										ll = l;
									}
								}

								//удаляем наиболее удаленую подстроку
								for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
								{
									Im.set_values(0, i, (le[ll] - lb[ll] + 1));
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

					if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_01_ImFBetweenTextDistace" + g_im_save_format, w, h);
				}

				if (ln == 0)
				{
					break;
				}

				if (g_text_alignment == TextAlignment::Center)
				{
					offset = le[ln - 1] + lb[0] - real_im_x_center2;
					if (offset < 0) offset = -offset;

					// потенциальный текст слишком сильно сдвинут от центра изображения ?
					if (offset > to_max2)
					{
						l = ln - 1;
						bln = 0;
						while (l > 0)
						{
							val1 = le[l - 1] + lb[0] - real_im_x_center2;
							if (val1 < 0) val1 = -val1;

							val2 = le[l] + lb[1] - real_im_x_center2;
							if (val2 < 0) val2 = -val2;

							if (val1 > val2)
							{
								ll = 0;
								for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
								{
									Im.set_values(0, i, (le[ll] - lb[ll] + 1));
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
								for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
								{
									Im.set_values(0, i, (le[ll] - lb[ll] + 1));
								}
							}

							l--;

							if (lb[0] >= real_im_x_center)
							{
								bln = 0;
								break;
							}
							if (le[l] <= real_im_x_center)
							{
								bln = 0;
								break;
							}

							offset = le[l] + lb[0] - real_im_x_center2;
							if (offset < 0) offset = -offset;
							if (offset <= to_max2)
							{
								bln = 1;
								break;
							}
						};

						if (bln == 0)
						{
							for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
							{
								Im.set_values(0, i, (le[l] - lb[0] + 1));
							}

							if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_1_1_ImFTextCentreOffset" + g_im_save_format, w, h);

							break;
						}

						ln = l + 1;
					}
				}
				else if(g_text_alignment != TextAlignment::Any)
				{
					if (g_text_alignment == TextAlignment::Left)
					{
						offset = lb[0] - min_x;
					}
					else
					{
						offset = max_x - le[ln - 1];
					}
					// потенциальный текст слишком сильно сдвинут
					if (offset > to_max)
					{
						for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
						{
							Im.set_values(0, i, (le[ln - 1] - lb[0] + 1));
						}

						if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_1_2_ImFTextOffset" + g_im_save_format, w, h);

						break;
					}
				}

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_02_2_ImFTextOffset" + g_im_save_format, w, h);

				if (g_text_alignment != TextAlignment::Any)
				{
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
							for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
							{
								Im.set_values(0, i, (le[1] - lb[0] + 1));
							}

							if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_03_ImFOnly2SubStrWithBigDist" + g_im_save_format, w, h);

							break;
						}
					}

					bln = 0;
					while ((ln > 1) && (bln == 0))
					{
						S = 0;
						for (ll = 0; ll < ln; ll++)  S += le[ll] - lb[ll] + 1;

						SS = le[ln - 1] - lb[0] + 1;

						if ((double)S < mpd * (double)SS)
						{
							if (g_text_alignment == TextAlignment::Center)
							{
								//определяем подстроку наиболее удаленую от центра
								val1 = lb[ln - 1] + le[ln - 1] - real_im_x_center2;
								val2 = lb[0] + le[0] - real_im_x_center2;
								if (val1 < 0) val1 = -val1;
								if (val2 < 0) val2 = -val2;

								offset = le[0] + lb[0] - real_im_x_center2;
								if (offset < 0) offset = -offset;

								if (IsTooRight(lb[ln - 1], le[ln - 1], to_max2, real_im_x_center) ||
									(
										(offset <= to_max2) &&
										((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0])))
										)
									)
								{
									ll = ln - 1;
								}
								else if (val1 > val2) ll = ln - 1;
								else ll = 0;
							}
							else if (g_text_alignment == TextAlignment::Left)
							{
								/*if ((lb[1] <= to_max) &&
									((le[0] - lb[0]) < ((le[ln - 1] - lb[ln - 1]))))
								{
									ll = 0;
								}
								else*/
								{
									ll = ln - 1;
								}
							}
							else
							{
								/*if ((w - le[ln - 2] <= to_max) &&
									((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0]))))
								{
									ll = ln - 1;
								}
								else*/
								{
									ll = 0;
								}
							}

							//удаляем наиболее удаленую подстроку
							for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
							{
								Im.set_values(0, i, (le[ll] - lb[ll] + 1));
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

					if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_04_ImFMinPointsDensity" + g_im_save_format, w, h);
				}

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
						for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
						{
							Im.set_values(0, i, (le[ln - 1] - lb[0] + 1));
						}
						ln = 0;						

						break;
					}

					if ( ((double)nNE < mpned * (double)S) && (g_text_alignment != TextAlignment::Any) )
					{
						if (ln > 1)
						{
							if (g_text_alignment == TextAlignment::Center)
							{
								//определяем подстроку наиболее удаленую от центра
								val1 = lb[ln - 1] + le[ln - 1] - real_im_x_center2;
								val2 = lb[0] + le[0] - real_im_x_center2;
								if (val1 < 0) val1 = -val1;
								if (val2 < 0) val2 = -val2;

								offset = le[0] + lb[0] - real_im_x_center2;
								if (offset < 0) offset = -offset;


								if (IsTooRight(lb[ln - 1], le[ln - 1], to_max2, real_im_x_center) ||
									(
										(offset <= to_max2) &&
										((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0])))
										)
									)
								{
									ll = ln - 1;
								}
								else if (val1 > val2) ll = ln - 1;
								else ll = 0;
							}
							else if (g_text_alignment == TextAlignment::Left)
							{
								/*if ((lb[1] <= to_max) &&
									((le[0] - lb[0]) < ((le[ln - 1] - lb[ln - 1]))))
								{
									ll = 0;
								}
								else*/
								{
									ll = ln - 1;
								}
							}
							else
							{
								/*if ((w - le[ln - 2] <= to_max) &&
									((le[ln - 1] - lb[ln - 1]) < ((le[0] - lb[0]))))
								{
									ll = ln - 1;
								}
								else*/
								{
									ll = 0;
								}
							}
						}
						else
						{
							ll = 0;
						}

						//удаляем наиболее удаленую подстроку
						for (y = 0, i = ia + lb[ll]; y < segh; y++, i += w)
						{
							Im.set_values(0, i, (le[ll] - lb[ll] + 1));
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

				if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_06_ImFMinNEdges" + g_im_save_format, w, h);				

				if (ln == 0)
				{
					break;
				}

				if ( (g_text_alignment == TextAlignment::Center) && (lb[0] >= real_im_x_center) )
				{
					// removing all sub lines
					for (y = 0, i = ia + lb[0]; y < segh; y++, i += w)
					{
						Im.set_values(0, i, (le[ln - 1] - lb[0] + 1));
					}
					ln = 0;

					if (g_show_sf_results) SaveGreyscaleImage(Im, "/DebugImages/SecondFiltration_" + now + "_y" + std::to_string(ia / w) + "_iter" + std::to_string(iter) + "_07_ImFAlignment" + g_im_save_format, w, h);

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

		int cur_y = ia / w;
		
		if (cur_y <= LE[k])
		{
			Im.set_values(0, ia, (LE[k] - cur_y + 1) * w);
		}
	}

	return res;
}

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

int cuda_kmeans(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImFF, simple_buffer<char>& labels, int w, int h, int numClusters, int loop_iterations, int initial_loop_iterations, int& min_x, int& max_x, int& min_y, int& max_y, bool mask_only = false, float threshold = 0.001)
{	
	int res = 0;
#ifdef USE_CUDA	
	int numObjs = w * h, i, j, numObjsFF;
	float **clusters;	
	simple_buffer<char> color_cluster_id(1 << 24, (char)-1);

	{
		simple_buffer<u8> ImBGRFF(numObjs * 3);
		simple_buffer<int> labelsFF(numObjs);

		numObjsFF = 0;
		for (i = 0; i < w * h; i++)
		{
			if (ImFF[i] != 0)
			{
				labelsFF[numObjsFF] = -1;
				SetBGRColor(ImBGRFF, numObjsFF, ImBGR, i);
				numObjsFF++;
			}
		}

		if (numObjsFF < g_mpn)
		{
			return res;
		}

		clusters = cuda_kmeans_img(ImBGRFF.m_pData, numObjsFF, numClusters, threshold, labelsFF.m_pData, initial_loop_iterations);
		if (clusters == NULL)
		{
			custom_assert(clusters != NULL, "cuda_kmeans crashed, not enough CUDA memory");
			return res;
		}

		free(clusters[0]);
		free(clusters);

		j = 0;
		for (i = 0; i < w * h; i++)
		{
			if (ImFF[i] != 0)
			{
				color_cluster_id[GetBGRColor(ImBGR, i)] = labelsFF[j];
				j++;
			}
		}
	}

	if (!mask_only)
	{
		simple_buffer<int> labelsMASK(w * h);

		for (i = 0; i < w*h; i++)
		{
			if (ImFF[i] != 0)
			{
				labelsMASK[i] = color_cluster_id[GetBGRColor(ImBGR, i)];
			}
			else
			{
				labelsMASK[i] = -1;
			}
		}
		// free memory for reduce usage
		color_cluster_id.set_size(0);

		min_x = 0;
		max_x = w - 1;
		min_y = 0;
		max_y = h - 1;

		clusters = cuda_kmeans_img(ImBGR.m_pData, numObjs, numClusters, threshold, labelsMASK.m_pData, loop_iterations);
		if (clusters == NULL)
		{
			custom_assert(clusters != NULL, "cuda_kmeans crashed, not enough CUDA memory");
			return res;
		}
		free(clusters[0]);
		free(clusters);

		for (i = 0; i < w * h; i++)
		{
			labels[i] = labelsMASK[i];
		}
	}
	else
	{		
		int x, y, ww, hh;

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
		/*min_x = std::max<int>(min_x - 2, 0);
		max_x = std::min<int>(max_x + 2, w-1);
		min_y = std::max<int>(min_y - 2, 0);
		max_y = std::min<int>(max_y + 2, h - 1);*/

		ww = max_x - min_x + 1;
		hh = max_y - min_y + 1;

		simple_buffer<u8> ImBGRMASK(ww * hh * 3);
		simple_buffer<int> labelsMASK(ww*hh);
		int numObjsMASK = ww*hh;

		for (y = min_y, j = 0; y <= max_y; y++)
		{
			for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
			{
				SetBGRColor(ImBGRMASK, j, ImBGR, i);

				if (ImFF[i] != 0)
				{
					labelsMASK[j] = color_cluster_id[GetBGRColor(ImBGRMASK, j)];
				}
				else
				{
					labelsMASK[j] = -1;
				}
			}
		}

		clusters = cuda_kmeans_img(ImBGRMASK.m_pData, numObjsMASK, numClusters, threshold, labelsMASK.m_pData, loop_iterations);
		if (clusters == NULL)
		{
			custom_assert(clusters != NULL, "cuda_kmeans crashed, not enough CUDA memory");
			return res;
		}
		free(clusters[0]);
		free(clusters);

		for (i = 0; i < ww*hh; i++)
		{
			color_cluster_id[GetBGRColor(ImBGRMASK, i)] = labelsMASK[i];
		}
			
		for (i = 0; i < w*h; i++)
		{
			labels[i] = color_cluster_id[GetBGRColor(ImBGR, i)];
		}
	}
#endif

	res = 1;
	return res;
}

int opencv_kmeans(simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImFF, simple_buffer<char> &labels, int w, int h, int numClusters, int loop_iterations, int initial_loop_iterations, int &min_x, int &max_x, int &min_y, int &max_y, wxString iter_det, bool mask_only = false)
{
	simple_buffer<char> color_cluster_id(1 << 24, (char)0);
	int numObjsFF;
	int i, j, color, res = 0;

	if (g_show_results)
	{
		SaveBGRImage(ImBGR, "/DebugImages/opencv_kmeans_" + iter_det + "_01_01_ImBGR" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImFF, "/DebugImages/opencv_kmeans_" + iter_det + "_01_02_ImMASK" + g_im_save_format, w, h);
	}

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
	
	cv::Mat centers;

	{
		cv::Mat cv_samplesFF(numObjsFF, 3, CV_32F);
		cv::Mat cv_labelsFF(numObjsFF, 1, CV_32S);

		j = 0;
		for (i = 0; i < w * h; i++)
		{
			if (ImFF[i] != 0)
			{
				for (int z = 0; z < 3; z++)
				{
					cv_samplesFF.at<float>(j, z) = ImBGR[(i * 3) + z];
				}
				j++;
			}
		}

		cv::theRNG().state = 0;
		cv::kmeans(cv_samplesFF, numClusters, cv_labelsFF, cv::TermCriteria(cv::TermCriteria::MAX_ITER, initial_loop_iterations, 1.0), 1, cv::KMEANS_PP_CENTERS, centers);

		if (g_show_results)
		{
			simple_buffer<u8> ImClusters_tmp(w * h);
			simple_buffer<char> labels_tmp(w * h, (char)-1);

			j = 0;
			for (i = 0; i < w * h; i++)
			{
				if (ImFF[i] != 0)
				{
					labels_tmp[i] = cv_labelsFF.at<int>(j, 0);
					j++;
				}
			}

			GetClustersImage(ImClusters_tmp, labels_tmp, numClusters, w, h);
			SaveGreyscaleImage(ImClusters_tmp, "/DebugImages/opencv_kmeans_" + iter_det + "_01_03_ImClustersInitial" + g_im_save_format, w, h);
		}

		j = 0;
		for (i = 0; i < w * h; i++)
		{
			if (ImFF[i] != 0)
			{
				color_cluster_id[GetBGRColor(ImBGR, i)] = cv_labelsFF.at<int>(j, 0);
				j++;
			}
		}
	}

	if (!mask_only)
	{
		cv::Mat cv_labels(w * h, 1, CV_32S);
		
		for (i = 0; i < w * h; i++)
		{
			cv_labels.at<int>(i, 0) = color_cluster_id[GetBGRColor(ImBGR, i)];
		}
		// free memory for reduce usage
		color_cluster_id.set_size(0);

		cv::Mat cv_samples(w*h, 3, CV_32F);		

		for (i = 0; i < w*h; i++)
		{
			for (int z = 0; z < 3; z++)
			{
				cv_samples.at<float>(i, z) = ImBGR[(i * 3) + z];
			}
		}

		min_x = 0;
		max_x = w - 1;
		min_y = 0;
		max_y = h - 1;

		cv::theRNG().state = 0;
		cv::kmeans(cv_samples, numClusters, cv_labels, cv::TermCriteria(cv::TermCriteria::MAX_ITER, loop_iterations, 1.0), 1, cv::KMEANS_USE_INITIAL_LABELS, centers);
		
		for (i = 0; i < w * h; i++)
		{
			labels[i] = cv_labels.at<int>(i, 0);
		}

		if (g_show_results)
		{
			simple_buffer<u8> ImClusters_tmp(w * h);
			GetClustersImage(ImClusters_tmp, labels, numClusters, w, h);
			SaveGreyscaleImage(ImClusters_tmp, "/DebugImages/opencv_kmeans_" + iter_det + "_01_04_ImClustersFinal" + g_im_save_format, w, h);
		}
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
		/*min_x = std::max<int>(min_x - 2, 0);
		max_x = std::min<int>(max_x + 2, w - 1);
		min_y = std::max<int>(min_y - 2, 0);
		max_y = std::min<int>(max_y + 2, h - 1);*/

		ww = max_x - min_x + 1;
		hh = max_y - min_y + 1;

		cv::Mat cv_samplesMASK(ww*hh, 3, CV_32F);
		cv::Mat cv_labelsMASK(ww*hh, 1, CV_32S);

		int numObjsMASK = ww * hh;

		for (y = min_y, j = 0; y <= max_y; y++)
		{
			for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
			{
				for (int z = 0; z < 3; z++)
				{
					cv_samplesMASK.at<float>(j, z) = ImBGR[(i * 3) + z];
				}

				cv_labelsMASK.at<int>(j, 0) = color_cluster_id[GetBGRColor(ImBGR, i)];
			}
		}

		cv::theRNG().state = 0;
		cv::kmeans(cv_samplesMASK, numClusters, cv_labelsMASK, cv::TermCriteria(cv::TermCriteria::MAX_ITER, loop_iterations, 1.0), 1, cv::KMEANS_USE_INITIAL_LABELS, centers);

		for (y = min_y, j = 0; y <= max_y; y++)
		{
			for (x = min_x, i = y * w + min_x; x <= max_x; x++, i++, j++)
			{
				color_cluster_id[GetBGRColor(ImBGR, i)] = cv_labelsMASK.at<int>(j, 0);
			}
		}

		for (int i = 0; i < w*h; i++)
		{
			labels[i] = color_cluster_id[GetBGRColor(ImBGR, i)];
		}
	}

	res = 1;
	return res;
}

void GetClustersImage(simple_buffer<u8> &ImRES, simple_buffer<char> &labels, int clusterCount, int w, int h)
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

void SortClustersData(simple_buffer<int> &cluster_id, simple_buffer<int> &cluster_cnt, int clusterCount)
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

void GetClustersData(simple_buffer<char>& labels, simple_buffer<int>& cluster_id, simple_buffer<int>& cluster_cnt, simple_buffer<u8>& ImMASK, const int clusterCount, const int w, const int h, const int min_x, const int max_x, const int min_y, const int max_y)
{
	int x, y, i, j, val;

	cluster_cnt.set_values(0, clusterCount);

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
}

void SortClusters(simple_buffer<char> &labels, simple_buffer<int> &cluster_id, simple_buffer<int> &cluster_cnt, simple_buffer<u8> &ImMASK, const int clusterCount, const int w, const int h, const int min_x, const int max_x, const int min_y, const int max_y)
{
	GetClustersData(labels, cluster_id, cluster_cnt, ImMASK, clusterCount, w, h, min_x, max_x, min_y, max_y);

	SortClustersData(cluster_id, cluster_cnt, clusterCount);	
}

void GetSubGreyscaleImage(simple_buffer<u8>& ImIn, simple_buffer<u8>& ImOut, const int w, const int h, const int min_x, const int max_x, const int min_y, const int max_y)
{
	custom_assert((max_x <= w - 1) && (max_x >= 0), "GetSubGreyscaleImage: not: (max_x <= w - 1) && (max_x >= 0)");
	custom_assert((min_x <= max_x) && (min_x >= 0), "GetSubGreyscaleImage: not: (min_x <= max_x) && (min_x >= 0)");
	
	custom_assert((max_y <= h - 1) && (max_y >= 0), "GetSubGreyscaleImage: not: (max_y <= h - 1) && (max_y >= 0)");
	custom_assert((min_y <= max_y) && (min_y >= 0), "GetSubGreyscaleImage: not: (min_y <= max_y) && (min_y >= 0)");

	int ww = max_x - min_x + 1;
	int hh = max_y - min_y + 1;

	custom_assert(ImOut.m_size >= ww * hh, "GetSubGreyscaleImage: not: ImOut.m_size >= ww * hh");

	int y_in, y_out;
	for (y_in = min_y, y_out = 0; y_in <= max_y; y_in++, y_out++)
	{
		ImOut.copy_data(ImIn, y_out * ww, (y_in * w) + min_x, ww);
	}
}

void GetInfoByLocation(simple_buffer<u8> &Im, int &len, int &ww, int &max_section, int &cnts, const int w, const int h, const int min_x, const int max_x, const int min_y, const int max_y, u8 white, wxString iter_det)
{
	const int w_sub = max_x - min_x + 1;
	const int h_sub = max_y - min_y + 1;

	simple_buffer<u8> ImSub(w_sub * h_sub);
	GetSubGreyscaleImage(Im, ImSub, w, h, min_x, max_x, min_y, max_y);

	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(ImSub, w_sub, h_sub, white, pFigures);
	int N = pFigures.size(), i, x, y, cnt;
	CMyClosedFigure *pFigure;
	simple_buffer<int> x_range(w_sub, 0);

	int _min_x = w_sub - 1;
	int _max_x = -1;
	len = 0;
	ww = 0;
	max_section = 0;
	cnts = 0;

	for (i = 0; i < N; i++)
	{
		pFigure = &(pFigures[i]);

		for (int x = pFigure->m_minX; x <= pFigure->m_maxX; x++) x_range[x] = 1;
		if (pFigure->m_minX < _min_x) _min_x = pFigure->m_minX;
		if (pFigure->m_maxX > _max_x) _max_x = pFigure->m_maxX;
		cnts += pFigure->m_PointsArray.m_size;
	}

	if (_max_x != -1)
	{
		len = _max_x - _min_x + 1;

		for (int x = _min_x; x <= _max_x; x++)
		{
			if (x_range[x] == 1) ww++;
		}

		for (y = 0; y < h_sub; y++)
		{
			cnt = 0;
			for (x = _min_x, i = y * w_sub + _min_x; x <= _max_x; x++, i++)
			{
				if (ImSub[i] == white)
				{
					cnt++;
				}
			}
			if (cnt > max_section) max_section = cnt;
		}
	}
}

void GetClustersInfoByLocation(simple_buffer<u8> &ImMASKClusters, simple_buffer<int> &len, simple_buffer<int> &ww, simple_buffer<int> &max_section, simple_buffer<int> &cnts, int clusterCount, int w, int h, int min_x, int max_x, int min_y, int max_y, wxString iter_det)
{
	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount), [&ImMASKClusters, &len, &ww, &max_section, &cnts, clusterCount, w, h, min_x, max_x, min_y, max_y, iter_det](int cluster_id)
	{
		int white = GetClusterColor(clusterCount, cluster_id);
		GetInfoByLocation(ImMASKClusters, len[cluster_id], ww[cluster_id], max_section[cluster_id], cnts[cluster_id], w, h, min_x, max_x, min_y, max_y, white, iter_det);
	});
}

void GetBinaryImageByClusterId(simple_buffer<u8> &ImRES, simple_buffer<char> &labels, char req_cluster_id, int w, int h, u8 white = 255, bool erase = true)
{
	if (erase)
	{
		for (int i = 0; i < w * h; i++)
		{
			char cluster_id = labels[i];

			if (req_cluster_id == cluster_id)
			{
				ImRES[i] = white;
			}
			else
			{
				ImRES[i] = 0;
			}
		}
	}
	else
	{
		for (int i = 0; i < w * h; i++)
		{
			char cluster_id = labels[i];

			if (req_cluster_id == cluster_id)
			{
				ImRES[i] = white;
			}
		}
	}
}

void GetBinaryImageFromClustersImageByClusterId(simple_buffer<u8> &ImRES, simple_buffer<u8> &ImClusters, int cluster_id, int clusterCount, int w, int h)
{
	u8 white = GetClusterColor(clusterCount, cluster_id);

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

void RestoreImMask(simple_buffer<u8> &ImClusters, simple_buffer<u8> &ImMASKF, int clusterCount, int w, int h, wxString iter_det)
{	
	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount), [&ImClusters, &ImMASKF, clusterCount, w, h](int cluster_id)
	{
		simple_buffer<u8> ImCluster(w * h);
		simple_buffer<u8> ImTMP(ImMASKF);

		GetBinaryImageFromClustersImageByClusterId(ImCluster, ImClusters, cluster_id, clusterCount, w, h);
		IntersectTwoImages(ImTMP, ImCluster, w, h);
		MergeImagesByIntersectedFigures(ImTMP, ImCluster, w, h, (u8)255);
		CombineTwoImages(ImMASKF, ImTMP, w, h, (u8)255);
	});
}

int FilterImMask(simple_buffer<u8> &ImClusters, simple_buffer<u8> &ImMASKF, simple_buffer<char> &labels, const int clusterCount, const int w, const int h, const int min_x, const int max_x, const int min_y, const int max_y, int &deleted_by_location, wxString iter_det, const int real_im_x_center)
{
	simple_buffer<u8> ImRES4(w * h);
	int val, i, j;
	double ww_thr = 0.5;
	double ww_in_thr = 0.7;
	double max_section_thr = 0.5;	
	double cnt_thr = 0.33;
	int num_main_clusters;
	simple_buffer<int> len(clusterCount, 0), ww(clusterCount, 0), max_section(clusterCount, 0), cluster_ids(clusterCount, 0), cnts(clusterCount, 0), placement_cnts(clusterCount, 0);
	int max_ww, max_max_section, max_len, max_cnt;

	cv::ocl::setUseOpenCL(g_use_ocl);

	deleted_by_location = 0;

	ImRES4.copy_data(ImClusters, w * h);
	IntersectTwoImages(ImRES4, ImMASKF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImRES4, "/DebugImages/FilterImMask_" + iter_det + "_01_ImClustersInImMASKF" + g_im_save_format, w, h);

	GetClustersInfoByLocation(ImRES4, len, ww, max_section, cnts, clusterCount, w, h, min_x, max_x, min_y, max_y, iter_det);

	for (int cluster_id = 0; cluster_id < clusterCount; cluster_id++)
	{
		cluster_ids[cluster_id] = cluster_id;
	}

	num_main_clusters = clusterCount;

	//Removing all clusters which not exist in left or right part of image
	if (g_text_alignment != TextAlignment::Any)
	{
		simple_buffer<int> tmp_cluster_ids(clusterCount, 0);
		simple_buffer<int> tmp_cluster_cnts(clusterCount, 0);

		if ( (g_text_alignment == TextAlignment::Center) ||
			 (g_text_alignment == TextAlignment::Left) )
		{
			GetClustersData(labels, tmp_cluster_ids, tmp_cluster_cnts, ImMASKF, clusterCount, w, h, min_x, std::min<int>(max_x, real_im_x_center - 1), min_y, max_y);
		}
		else if (g_text_alignment == TextAlignment::Right)
		{
			GetClustersData(labels, tmp_cluster_ids, tmp_cluster_cnts, ImMASKF, clusterCount, w, h, std::max<int>(min_x, real_im_x_center + 1), max_x, min_y, max_y);
		}
		else
		{
			GetClustersData(labels, tmp_cluster_ids, tmp_cluster_cnts, ImMASKF, clusterCount, w, h, min_x, max_x, min_y, max_y);
		}

		i = 0;
		while (i < num_main_clusters)
		{			
			if (tmp_cluster_cnts[cluster_ids[i]] == 0)
			{
				if (g_show_results)
				{
					simple_buffer<u8> ImCluster(w * h);
					GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
					if (g_show_results) SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_01_01_id" + std::to_string(cluster_ids[i]) + "_RemovingClusterDueToNotExistLeftPart" + g_im_save_format, w, h);
				}

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
	}
	
	{
		simple_buffer<u8> ImInside(w * h), ImInsideClose(w * h), ImInside2(w * h);

		i = 0;
		while (i < num_main_clusters)
		{
			if (num_main_clusters == 1)
			{
				break;
			}

			int cluster_id = cluster_ids[i];
			u8 white = GetClusterColor(clusterCount, cluster_id);
			int len_in, ww_in, max_section_in, cnt_in;

			if (GetImageWithInsideFigures(ImRES4, ImInside, w, h, white, false) > 0)
			{
				simple_buffer<u8> ImCluster(w * h);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_id, clusterCount, w, h);

				if (g_show_results)
				{
					SaveGreyscaleImage(ImRES4, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_01_ImClustersInImMASK" + g_im_save_format, w, h);
					SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_02_ImClusterInImMASK" + g_im_save_format, w, h);
					SaveGreyscaleImage(ImInside, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_03_ImClusterInsideFigures" + g_im_save_format, w, h);
				}

				{
					cvMAT cv_im_gr;
					GreyscaleImageToMat(ImCluster, w, h, cv_im_gr);
					cv::Mat kernel = cv::Mat::ones(3, 3, CV_8U);
					cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 1);
					cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 1);
					BinaryMatToImage(cv_im_gr, w, h, ImCluster, (u8)255);
					if (g_show_results) SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_04_ImClusterClose" + g_im_save_format, w, h);
				}

				if (GetImageWithInsideFigures(ImCluster, ImInsideClose, w, h, (u8)255, false) > 0)
				{
					if (g_show_results) SaveGreyscaleImage(ImInsideClose, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_05_ImClusterCloseInsideFigures" + g_im_save_format, w, h);
					CombineTwoImages(ImInside, ImInsideClose, w, h, white);
					if (g_show_results) SaveGreyscaleImage(ImInside, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_06_ImClusterInsideFiguresCombine" + g_im_save_format, w, h);
				}

				IntersectTwoImages(ImInside, ImRES4, w, h);
				if (g_show_results) SaveGreyscaleImage(ImInside, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_07_01_ImClusterInsideFiguresIntImMASK" + g_im_save_format, w, h);

				GetInfoByLocation(ImInside, len_in, ww_in, max_section_in, cnt_in, w, h, min_x, max_x, min_y, max_y, white, iter_det);

				if (ww_in >= ww[i] * ww_in_thr)
				{
					int rem_i = i;
					int cnt1 = 0, cnt2 = 0, cnt3 = 0;
					if (GetImageWithInsideFigures(ImInside, ImInside2, w, h, white, false) > 0)
					{
						IntersectTwoImages(ImInside2, ImRES4, w, h);
						if (g_show_results) SaveGreyscaleImage(ImInside2, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_id) + "_07_02_ImClusterInside2FiguresIntImMASK" + g_im_save_format, w, h);

						for (j = 0; j < w * h; j++)
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

					if ((cnt2 > cnt1 * 0.5) && (cnt2 > cnt3 * 0.5))
					{
						simple_buffer<int> cluster_ids_rem(clusterCount, 0);
						simple_buffer<int> cluster_cnt(clusterCount, 0);
						SortClusters(labels, cluster_ids_rem, cluster_cnt, ImInside, clusterCount, w, h, min_x, max_x, min_y, max_y);

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
							SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_02_id" + std::to_string(cluster_ids[rem_i]) + "_08_RemovingCluster" + g_im_save_format, w, h);
						}

						for (j = rem_i; j < num_main_clusters - 1; j++)
						{
							ww[j] = ww[j + 1];
							max_section[j] = max_section[j + 1];
							len[j] = len[j + 1];
							cluster_ids[j] = cluster_ids[j + 1];
							cnts[j] = cnts[j + 1];
							placement_cnts[j] = placement_cnts[j + 1];
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
		if (ww[i] < max_ww * ww_thr)
		{
			if (g_show_results)
			{
				simple_buffer<u8> ImCluster(w * h);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_03_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
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
				simple_buffer<u8> ImCluster(w * h);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_04_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
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

	max_cnt = cnts.get_max_value(num_main_clusters);
	i = 0;
	while (i < num_main_clusters)
	{
		if (cnts[i] < max_cnt * cnt_thr)
		{
			if (g_show_results)
			{
				simple_buffer<u8> ImCluster(w * h);
				GetBinaryImageFromClustersImageByClusterId(ImCluster, ImRES4, cluster_ids[i], clusterCount, w, h);
				if (g_show_results) SaveGreyscaleImage(ImCluster, "/DebugImages/FilterImMask_" + iter_det + "_05_id" + std::to_string(cluster_ids[i]) + "_01_RemovingCluster" + g_im_save_format, w, h);
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

	GreyscaleImageToBinary(ImMASKF, ImRES4, w, h, (u8)255);
	if (g_show_results)
	{
		SaveGreyscaleImage(ImMASKF, "/DebugImages/FilterImMask_" + iter_det + "_06_01_ImMASKFIntMainClustersByLocation" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImRES4, "/DebugImages/FilterImMask_" + iter_det + "_06_02_ImClustersInImMASKF" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImClusters, "/DebugImages/FilterImMask_" + iter_det + "_06_03_ImClustersOrig" + g_im_save_format, w, h);
	}

	return num_main_clusters;
}

int GetFirstFilteredClusters(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImLab, simple_buffer<u8>& ImIL, simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImMASK2, simple_buffer<u8>& ImMaskWithBorder, simple_buffer<u8>& ImMaskWithBorderF, simple_buffer<u8>& ImClusters2, simple_buffer<char>& labels2, int clusterCount2, int w, int h, wxString iter_det)
{
	int min_x, max_x, min_y, max_y;
	int res = 0;

	if (g_show_results)
	{
		SaveBGRImage(ImBGR, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_01_ImBGR" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImMASK2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_02_ImMASK2" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImMASK, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_03_01_ImMASK" + g_im_save_format, w, h);
		
	}

	if (g_use_cuda_gpu)
	{
		res = cuda_kmeans(ImBGR, ImMASK, labels2, w, h, clusterCount2, g_cuda_kmeans_loop_iterations, g_cuda_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, false);
	}
	else
	{
		res = opencv_kmeans(ImBGR, ImMASK, labels2, w, h, clusterCount2, g_cpu_kmeans_loop_iterations, g_cpu_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, iter_det, false);
	}

	if (res == 0)
	{
		return res;
	}

	if (g_show_results)
	{
		GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
		SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_04_ImClustersOrig" + g_im_save_format, w, h);
	}

	if (g_use_ILA_images_before_clear_txt_images_from_borders)
	{		
		if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
		{
			if (ImIL.m_size)
			{
				IntersectTwoImages(labels2, ImIL, w, h, (char)-1);

				if (g_show_results)
				{
					GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
					SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_05_ImClustersIntImIL" + g_im_save_format, w, h);
				}
			}

			FilterImageByPixelColorIsInRange(labels2, &ImBGR, &ImLab, w, h, iter_det + "_GetFirstFilteredClusters_line" + wxString::Format(wxT("%i"), __LINE__), (char)-1);

			if (g_show_results)
			{
				GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
				if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_06_ImClustersFByPixelColorIsInRange" + g_im_save_format, w, h);
			}
		}
	}	

	GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
	if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_01_07_ImClustersRes" + g_im_save_format, w, h);

	{
		std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount2), [&ImClusters2, clusterCount2, w, h](int cluster_idx)
		{
			ClearImageOptimal(ImClusters2, w, h, GetClusterColor(clusterCount2, cluster_idx));
		});

		GreyscaleImageToBinary(ImMaskWithBorder, ImClusters2, w, h, (u8)255);

		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_02_ImMaskWithBorder" + g_im_save_format, w, h);
	}

	if (g_use_ILA_images_for_getting_txt_symbols_areas)
	{
		if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
		{
			if (ImIL.m_size)
			{
				simple_buffer<u8> ImTMP(ImIL);
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_01_ImIL" + g_im_save_format, w, h);
				ClearImageOptimal(ImTMP, w, h, (u8)255);
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_02_ImILClearImageOptimal" + g_im_save_format, w, h);
				FilterImageByPixelColorIsInRange(ImTMP, &ImBGR, &ImLab, w, h, iter_det + "_GetFirstFilteredClusters_line" + wxString::Format(wxT("%i"), __LINE__));
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_03_ImILFByPixelColorIsInRange" + g_im_save_format, w, h);

				if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_04_ImMaskWithBorder" + g_im_save_format, w, h);
				IntersectTwoImages(ImMaskWithBorder, ImTMP, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_05_ImMaskWithBorderIntImILF" + g_im_save_format, w, h);
				ClearImageByMask(ImTMP, ImMaskWithBorder, w, h, (u8)255);
				if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_06_ImILFClearImageByMaskImMaskWithBorder" + g_im_save_format, w, h);
				IntersectTwoImages(ImMaskWithBorder, ImTMP, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_07_ImMaskWithBorderIntImILCleared" + g_im_save_format, w, h);
				IntersectTwoImages(ImClusters2, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_03_08_ImClustersF" + g_im_save_format, w, h);
			}
		}
	}

	if (!g_use_ILA_images_before_clear_txt_images_from_borders)
	{		
		if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
		{
			if (ImIL.m_size)
			{
				IntersectTwoImages(ImMaskWithBorder, ImIL, w, h);

				if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_04_01_ImMaskWithBorderIntImIL" + g_im_save_format, w, h);
			}

			FilterImageByPixelColorIsInRange(ImMaskWithBorder, &ImBGR, &ImLab, w, h, iter_det + "_GetFirstFilteredClusters_line" + wxString::Format(wxT("%i"), __LINE__));
			if (g_show_results) SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_04_02_ImMaskWithBorderFByPixelColorIsInRange" + g_im_save_format, w, h);

			IntersectTwoImages(ImClusters2, ImMaskWithBorder, w, h);
			if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_04_03_ImClustersF" + g_im_save_format, w, h);
		}
	}	

	ImMaskWithBorderF.copy_data(ImMaskWithBorder, w * h);

	{
		std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount2), [&ImMASK2, &ImClusters2, clusterCount2, w, h](int cluster_idx)
		{
			ClearImageByMask(ImClusters2, ImMASK2, w, h, GetClusterColor(clusterCount2, cluster_idx));
		});

		GreyscaleImageToBinary(ImMaskWithBorderF, ImClusters2, w, h, (u8)255);
		if (g_show_results)
		{
			SaveGreyscaleImage(ImMASK2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_05_01_ImMASK2" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImMaskWithBorderF, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_05_02_ImMaskWithBorderFByMASK2" + g_im_save_format, w, h);
		}
	}

	if (g_combine_to_single_cluster)
	{
		if (g_color_ranges.size() > 0)
		{
			IntersectTwoImages(labels2, ImMaskWithBorder, w, h, (char)-1);
			for (int i = 0; i < w * h; i++)
			{
				if (labels2[i] != -1)
				{
					labels2[i] = clusterCount2 - 1;
				}
			}
			if (g_show_results)
			{
				simple_buffer<u8> ImTMP(w * h);
				GetClustersImage(ImTMP, labels2, clusterCount2, w, h);
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_06_01_ImClustersCombined" + g_im_save_format, w, h);
			}

			if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_06_02_ImClustersF" + g_im_save_format, w, h);
			GreyscaleImageToBinary(ImClusters2, ImClusters2, w, h, GetClusterColor(clusterCount2, clusterCount2-1));
			if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetFirstFilteredClusters_" + iter_det + "_06_03_ImClustersFCombined" + g_im_save_format, w, h);			
		}
	}

	res = 1;
	return res;
}

void ClearMainClusterImage(simple_buffer<u8>& ImMainCluster, simple_buffer<u8> ImMASK, simple_buffer<u8>& ImIL, int w, int h, int W, int H, int LH, wxString iter_det)
{
	cv::ocl::setUseOpenCL(g_use_ocl);

	if (g_use_ILA_images_for_clear_txt_images && (!g_use_gradient_images_for_clear_txt_images) && ImIL.m_size)
	{
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/ClearMainClusterImage_" + iter_det + "_01_01_ImMainCluster" + g_im_save_format, w, h);
		ClearImageByMask(ImMainCluster, ImIL, w, h, (u8)255);
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/ClearMainClusterImage_" + iter_det + "_01_02_ImMainClusterFWithImIL" + g_im_save_format, w, h);		
	}
	else if (g_use_gradient_images_for_clear_txt_images)
	{
		
		{
			simple_buffer<u8> ImTMP(w * h);
			cvMAT cv_im_gr;
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_02_01_ImMASK" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/ClearMainClusterImage_" + iter_det + "_02_02_ImMainCluster" + g_im_save_format, w, h);
			BinaryImageToMat(ImMainCluster, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), std::max<int>(1, LH/10));
			BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/ClearMainClusterImage_" + iter_det + "_02_03_ImMainClusterDilate" + g_im_save_format, w, h);			
			IntersectTwoImages(ImMASK, ImTMP, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_02_04_ImMASKIntImMainClusterDilate" + g_im_save_format, w, h);
		}

		int dilate_size = std::max<int>(1, LH / 10);

		if ((g_use_ILA_images_for_clear_txt_images) && ImIL.m_size)
		{
			simple_buffer<u8> ImTMP(w * h);
			cvMAT cv_im_gr;
			if (g_show_results) SaveGreyscaleImage(ImIL, "/DebugImages/ClearMainClusterImage_" + iter_det + "_03_01_ImIL" + g_im_save_format, w, h);
			BinaryImageToMat(ImIL, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), dilate_size);
			BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/ClearMainClusterImage_" + iter_det + "_03_02_ImILDilate" + g_im_save_format, w, h);
			IntersectTwoImages(ImMASK, ImTMP, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_03_03_ImMASKIntImILDilate" + g_im_save_format, w, h);
		}		

		{
			cvMAT cv_im_gr;
			cv::Mat kernel;
			GreyscaleImageToMat(ImMASK, w, h, cv_im_gr);
			kernel = cv::Mat::ones(7, 7, CV_8U);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 1);
			cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);			
			BinaryMatToImage(cv_im_gr, w, h, ImMASK, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_04_ImMASKDilate1Close2" + g_im_save_format, w, h);
		}

		ExtendByInsideFigures(ImMASK, w, h, (u8)255);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_05_01_ImMASKExtendByInsideFigures" + g_im_save_format, w, h);

		if (dilate_size > 1)
		{
			cvMAT cv_im_gr;
			GreyscaleImageToMat(ImMASK, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), dilate_size-1);
			BinaryMatToImage(cv_im_gr, w, h, ImMASK, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_05_02_ImMASKDilate" + wxString::Format(wxT("%i"), (int)(dilate_size - 1)) + g_im_save_format, w, h);
		}

		IntersectTwoImages(ImMASK, ImMainCluster, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/ClearMainClusterImage_" + iter_det + "_06_ImMASKIntImMainCluster" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/ClearMainClusterImage_" + iter_det + "_07_01_ImMainCluster" + g_im_save_format, w, h);
		ClearImageByMask(ImMainCluster, ImMASK, w, h, (u8)255);
		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/ClearMainClusterImage_" + iter_det + "_07_02_ImMainClusterFWithImMASK" + g_im_save_format, w, h);
	}
}

int GetMainTwoClustersByAverageLuminanceDifference(simple_buffer<u8>& ImMainCluster, simple_buffer<u8>& ImMainCluster2, int& num_main_clusters, simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImY, simple_buffer<char>& labels, const int clusterCount, const int w, const int h, wxString iter_det, const int min_x, const int max_x, const int min_y, const int max_y)
{
	int res = 0;
	int i, x, y;
	simple_buffer<int> cluster_cnt(clusterCount, 0);
	simple_buffer<int> cluster_id(clusterCount, 0);	
	int color, yc;
	u8* pClr = (u8*)(&color);

	color = 0;
	pClr[1] = 255;
	pClr[2] = 255;
	yc = color;

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASK" + g_im_save_format, w, h);	

	if (g_show_results)
	{
		simple_buffer<u8> ImTMP(w * h);
		GetClustersImage(ImTMP, labels, clusterCount, w, h);
		IntersectTwoImages(ImTMP, ImMASK, w, h);
		SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClustersIntImMASK_WSP" + g_im_save_format, w, h,
			{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
			{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
	}

	SortClusters(labels, cluster_id, cluster_cnt, ImMASK, clusterCount, w, h, min_x, max_x, min_y, max_y);

	int id1 = cluster_id[0];
	int id2 = cluster_id[1];

	if (cluster_cnt[0] == 0)
	{
		num_main_clusters = 0;
		res = 0;
	}
	else if (cluster_cnt[1] == 0)
	{
		GetBinaryImageByClusterId(ImMainCluster, labels, id1, w, h);
		if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
			{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
			{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
		num_main_clusters = 1;
		res = 1;
	}
	else
	{
		simple_buffer<int> cluster_mY3(clusterCount);

		if (g_show_results)
		{
			simple_buffer<u8> ImTMP(w * h, (u8)0);
			GetBinaryImageByClusterId(ImTMP, labels, id1, w, h, GetClusterColor(clusterCount, id1), false);
			GetBinaryImageByClusterId(ImTMP, labels, id2, w, h, GetClusterColor(clusterCount, id2), false);
			SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainTwoClusters_WSP" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
			IntersectTwoImages(ImTMP, ImMASK, w, h);
			SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainTwoClustersIntImMASK_WSP" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });

			GetBinaryImageByClusterId(ImTMP, labels, id1, w, h, GetClusterColor(clusterCount, id1));
			IntersectTwoImages(ImTMP, ImMASK, w, h);
			SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster1InImClustersIntImMASK_WSP" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });

			GetBinaryImageByClusterId(ImTMP, labels, id2, w, h, GetClusterColor(clusterCount, id2));
			IntersectTwoImages(ImTMP, ImMASK, w, h);
			SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2InImClustersIntImMASK_WSP" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
		}

		cluster_mY3.set_values(0, clusterCount);
		cluster_cnt.set_values(0, clusterCount);

		for (x = min_x; x <= max_x; x++)
		{
			for (y = min_y; y <= max_y; y++)
			{
				i = (y * w) + x;

				if (ImMASK[i])
				{
					int id = labels[i];
					cluster_mY3[id] += (int)ImY[i];
					cluster_cnt[id]++;
				}
			}
		}

		if ((cluster_cnt[id1] == 0) && (cluster_cnt[id2] == 0))
		{
			num_main_clusters = 0;
			res = 0;
		}
		else if (cluster_cnt[id2] == 0)
		{
			GetBinaryImageByClusterId(ImMainCluster, labels, id1, w, h);
			if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
			num_main_clusters = 1;
			res = 1;
		}
		else if (cluster_cnt[id1] == 0)
		{
			GetBinaryImageByClusterId(ImMainCluster, labels, id2, w, h);
			if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
				{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
			num_main_clusters = 1;
			res = 1;
		}
		else if ((cluster_cnt[id1] > 0) && (cluster_cnt[id2] > 0))
		{
			int mY1 = cluster_mY3[id1] / cluster_cnt[id1];
			int mY2 = cluster_mY3[id2] / cluster_cnt[id2];
			int max_mY = std::max<int>(mY1, mY2);
			int min_mY = std::min<int>(mY1, mY2);

			if (max_mY - min_mY >= 10)
			{
				if ((g_border_is_darker && (mY1 > mY2)) || // first claster looks brighter
					((!g_border_is_darker) && (mY1 < mY2)))  // first claster looks darker
				{
					GetBinaryImageByClusterId(ImMainCluster, labels, id1, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
					GetBinaryImageByClusterId(ImMainCluster2, labels, id2, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster2, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster2" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });						
				}
				else
				{
					GetBinaryImageByClusterId(ImMainCluster, labels, id2, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
					GetBinaryImageByClusterId(ImMainCluster2, labels, id1, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster2, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster2" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
				}

				num_main_clusters = 2;
				res = 1;
			}
			else
			{
				if (cluster_cnt[id1] > cluster_cnt[id2])
				{
					GetBinaryImageByClusterId(ImMainCluster, labels, id1, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
					GetBinaryImageByClusterId(ImMainCluster2, labels, id2, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster2, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster2" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
				}
				else
				{
					GetBinaryImageByClusterId(ImMainCluster, labels, id2, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster1" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
					GetBinaryImageByClusterId(ImMainCluster2, labels, id1, w, h);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster2, "/DebugImages/GetMainTwoClustersByAverageLuminanceDifference_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClaster2" + g_im_save_format, w, h,
						{ std::pair<int, int>{min_y, yc}, std::pair<int, int>{max_y, yc} },
						{ std::pair<int, int>{min_x, yc}, std::pair<int, int>{max_x, yc} });
				}

				num_main_clusters = 2;
				res = 1;
			}
		}
	}

	return res;
}

void RemoveTooLongHorizontalSubLines(simple_buffer<u8> &ImMASK2, const int w, const int h, const int W, const int H, wxString iter_det, const int xb, const int xe, const int yb, const int ye)
{
	int max_ww = W / 20; // 5% from W
	int max_hh = 4;
	
	// removing all horizontal too long sub lines
	for (int y = 0; y < h; y++)
	{
		int cnt = 0;

		int x = 0;
		while (x < w)
		{
			if (ImMASK2[(y * w) + x])
			{
				cnt++;
			}
			else
			{
				cnt = 0;
			}

			if (cnt == max_ww)
			{
				int x_start = x - cnt + 1;

				int hh = 1;
				int top = -1;
				int bottom = -1;

				for (int y2 = y - 1; y2 >= std::max(0, y - max_hh); y2--)
				{
					if (ImMASK2[(y2 * w) + x_start])
					{
						hh++;
					}
					else
					{
						top = y2;
						for (int x_top = x_start; x_top <= x; x_top++)
						{
							if (ImMASK2[(y2 * w) + x_top])
							{
								hh++;
								top = -1;
								break;
							}
						}

						if (top != -1)
						{
							break;
						}
					}
				}

				if ((hh <= max_hh) && (top != -1))
				{
					for (int y2 = y + 1; y2 <= std::min(h - 1, y + max_hh); y2++)
					{
						if (ImMASK2[(y2 * w) + x_start])
						{
							hh++;
						}
						else
						{
							bottom = y2;
							for (int x_bottom = x_start; x_bottom <= x; x_bottom++)
							{
								if (ImMASK2[(y2 * w) + x_bottom])
								{
									hh++;
									bottom = -1;
									break;
								}
							}

							if (bottom != -1)
							{
								break;
							}
						}

						if (hh > max_hh)
						{
							break;
						}
					}
				}

				if ((hh <= max_hh) && (top != -1) && (bottom != -1))
				{
					while (x + 1 < w)
					{
						int orig_top = top;
						int orig_bottom = bottom;

						while ((ImMASK2[(top * w) + x + 1]) && (hh < max_hh) && (top > 0))
						{
							hh++;
							top--;
						}

						while ((ImMASK2[(bottom * w) + x + 1]) && (hh < max_hh) && (bottom < h - 1))
						{
							hh++;
							bottom++;
						}

						if ( (ImMASK2[(top * w) + x + 1]) || (ImMASK2[(bottom * w) + x + 1]) )
						{
							top = orig_top;
							bottom = orig_bottom;
							break;
						}
						
						x++;
						cnt++;
					}

					//removing horizontal too long sub line from [x_start, x], [top, bottom]
					for (int y2 = top; y2 <= bottom; y2++)
					{
						ImMASK2.set_values(0, (y2 * w) + x_start, cnt);
					}

					cnt = 0;
				}
				else
				{
					cnt--;
				}
			}

			x++;
		}
	}
}

void ClearImageFromHThinFigures(simple_buffer<u8>& Im, simple_buffer<CMyClosedFigure*>& ppFigures, int& N)
{
	CMyClosedFigure* pFigure;
	int i, l, ii;

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if (pFigure->height() <= 4)
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}
}

void MorphCloseAndExtByInsideAllFigures(simple_buffer<u8>& Im, simple_buffer<CMyClosedFigure*>& ppFigures, int& N, const int w, const int h, const int morph_close_size, wxString iter_det)
{
	//if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/MorphCloseAndExtByInsideAllFigures_" + iter_det + "_01_FigureIm" + g_im_save_format, w, h);

	std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(N), [&Im, &ppFigures, w, h, morph_close_size, iter_det](int f_i)
	//for (int f_i = 0; f_i < N; f_i++)
		{
			CMyClosedFigure* pFigure = ppFigures[f_i];

			int dx = morph_close_size;
			int dy = morph_close_size;
			int ww = pFigure->width() + (2 * dx);
			int hh = pFigure->height() + (2 * dy);
			simple_buffer<u8> FigureIm(ww * hh, (u8)0);

			for (int l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				int ii = pFigure->m_PointsArray[l];
				int y = ii / w;
				int x = ii - (y * w);
				FigureIm[((dy + y - pFigure->m_minY) * ww) + (dx + x - pFigure->m_minX)] = (u8)255;
			}

			//if (g_show_results) SaveGreyscaleImage(FigureIm, "/DebugImages/MorphCloseAndExtByInsideAllFigures_" + iter_det + "_02_fi" + wxString::Format(wxT("%i"), f_i) + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_FigureIm" + g_im_save_format, ww, hh);

			{
				cvMAT cv_im_gr;
				GreyscaleImageToMat(FigureIm, ww, hh, cv_im_gr);
				cv::Mat kernel = cv::Mat::ones(morph_close_size, morph_close_size, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
				BinaryMatToImage(cv_im_gr, ww, hh, FigureIm, (u8)255);
			}

			//if (g_show_results) SaveGreyscaleImage(FigureIm, "/DebugImages/MorphCloseAndExtByInsideAllFigures_" + iter_det + "_02_fi" + wxString::Format(wxT("%i"), f_i) + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_FigureImMorphClose" + g_im_save_format, ww, hh);

			ExtendByInsideFigures(FigureIm, ww, hh, (u8)255);

			//if (g_show_results) SaveGreyscaleImage(FigureIm, "/DebugImages/MorphCloseAndExtByInsideAllFigures_" + iter_det + "_02_fi" + wxString::Format(wxT("%i"), f_i) + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_FigureImExtendByInsideFigures" + g_im_save_format, ww, hh);

			for (int y = 0, i = 0; y < hh; y++)
			{
				for (int x = 0; x < ww; x++, i++)
				{
					if (FigureIm[i])
					{
						int y2 = y + pFigure->m_minY - dy;
						int x2 = x + pFigure->m_minX - dx;

						if ((y2 >= 0) && (y2 < h) &&
							(x2 >= 0) && (x2 < w))
						{
							Im[(y2 * w) + x2] = (u8)255;
						}
					}
				}
			}
		}
	);

	//if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/MorphCloseAndExtByInsideAllFigures_" + iter_det + "_03_FigureImResult" + g_im_save_format, w, h);
}

void ExtendAndFilterImFF(simple_buffer<u8>& ImFF, const int w, const int h, const int W, const int H, wxString iter_det, const int xb, const int xe, const int yb, const int ye)
{
	if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/ExtendAndFilterImFF_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImFF" + g_im_save_format, w, h);

	RemoveTooLongHorizontalSubLines(ImFF, w, h, W, H, iter_det, xb, xe, yb, ye);
	if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/ExtendAndFilterImFF_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImFFFByTooLongHSubLines" + g_im_save_format, w, h);

	{
		cvMAT cv_im_gr;
		int morph_close_size = 3;
		GreyscaleImageToMat(ImFF, w, h, cv_im_gr);
		cv::Mat kernel = cv::Mat::ones(morph_close_size, morph_close_size, CV_8U);
		cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
		BinaryMatToImage(cv_im_gr, w, h, ImFF, (u8)255);

		if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/ExtendAndFilterImFF_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImFFMorphClose" + g_im_save_format, w, h);
	}

	{
		CMyClosedFigure* pFigure;
		int i, l, ii, N;
		u64 t;

		custom_buffer<CMyClosedFigure> pFigures;
		t = SearchClosedFigures(ImFF, w, h, (u8)255, pFigures);
		N = pFigures.size();

		if (N > 0)
		{
			simple_buffer<CMyClosedFigure*> ppFigures(N);
			for (i = 0; i < N; i++)
			{
				ppFigures[i] = &(pFigures[i]);
			}

			ClearImageFromHThinFigures(ImFF, ppFigures, N);
			if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/ExtendAndFilterImFF_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImFFFByHThinFigures" + g_im_save_format, w, h);

			if (N > 0)
			{
				MorphCloseAndExtByInsideAllFigures(ImFF, ppFigures, N, w, h, 7, iter_det);
				if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/ExtendAndFilterImFF_" + iter_det + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImFFMorphCloseAndExtByInsideAllFigures" + g_im_save_format, w, h);
			}
		}
	}
}

int GetMainClusterImage(simple_buffer<u8> &ImBGR, simple_buffer<u8>& ImLab, simple_buffer<u8> ImMASK, simple_buffer<u8> ImMASK2, simple_buffer<u8> &ImIL, simple_buffer<u8> &ImRES, simple_buffer<u8> &ImY, simple_buffer<u8> &ImU, simple_buffer<u8> &ImV, simple_buffer<u8> &ImMaskWithBorder, simple_buffer<u8> &ImMaskWithBorderF, simple_buffer<u8> &ImClusters2, simple_buffer<char> &labels2, int clusterCount2, const int w, const int h, const int W, const int H, wxString iter_det, const int real_im_x_center, const int min_h, const int xb, const int xe, const int yb, const int ye)
{
	int x, y, i, j, j2, val1, val2;
	simple_buffer<int> cluster_cnt2(clusterCount2, 0);
	simple_buffer<int> cluster_id2(clusterCount2, 0);
	int clusterCount3 = 4;
	simple_buffer<int> cluster_cnt3(clusterCount3, 0);
	simple_buffer<int> cluster_id3(clusterCount3, 0);
	//DWORD  start_time;
	int ddy1 = 1, ddy2 = h - 2;
	int LH, LMAXY, lb, le, val, res = 0;
	int minX, maxX;

	if ((g_text_alignment == TextAlignment::Center) ||
		(g_text_alignment == TextAlignment::Left))
	{
		minX = xb;
		maxX = std::min<int>(xe, real_im_x_center - 1);
	}
	else if (g_text_alignment == TextAlignment::Right)
	{
		minX = std::max<int>(xb, real_im_x_center + 1);
		maxX = xe;
	}
	else
	{
		minX = xb;
		maxX = xe;
	}

	cv::ocl::setUseOpenCL(g_use_ocl);

	if (g_show_results)
	{
		SaveBGRImage(ImBGR, "/DebugImages/GetMainClusterImage_" + iter_det + "_01_1_ImRGB" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_01_2_ImMASK" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_01_3_ImMASK2" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImMaskWithBorder, "/DebugImages/GetMainClusterImage_" + iter_det + "_01_4_ImMaskWithBorder" + g_im_save_format, w, h);
	}

	ImRES.set_values(0, w * h);

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_04_02_ImMASK" + g_im_save_format, w, h);
	IntersectTwoImages(ImMASK, ImMaskWithBorderF, w, h);
	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_04_03_ImMASKIntImMaskWithBorderF" + g_im_save_format, w, h);

	if (g_show_results) SaveGreyscaleImage(ImClusters2, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_04_04_ImClusters2" + g_im_save_format, w, h);

	{				
		std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount2), [&ImClusters2, clusterCount2, w, h, W, H, min_h, real_im_x_center, yb, ye, &ImMASK, iter_det](int cluster_idx)
		{
			int i, val, LH, LMAXY, lb, le;
			u8 white = GetClusterColor(clusterCount2, cluster_idx);
			simple_buffer<u8> ImTMP(ImMASK);

			for (i=0; i<w*h; i++)
			{
				if (ImClusters2[i] != white)
				{
					ImTMP[i] = 0;
				}
			}

			if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_01_ImMASKIntCluster" + g_im_save_format, w, h);

			val = GetSubParams(ImTMP, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);
			if (val == 1)
			{
				if (g_show_results) SaveGreyscaleImageWithSubParams(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_02_ImMASKIntCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt2(ImClusters2, w, h, W, H, LH, LMAXY, real_im_x_center, white, iter_det + "_02_05_01_id" + std::to_string(cluster_idx));
				if (g_show_results)
				{
					GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters2, cluster_idx, clusterCount2, w, h);
					IntersectTwoImages(ImTMP, ImMASK, w, h);
					SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_01_id" + std::to_string(cluster_idx) + "_03_ImMASKIntClusterFOpt2" + g_im_save_format, w, h);
				}
			}			
		});

		GreyscaleImageToBinary(ImMaskWithBorderF, ImClusters2, w, h, (u8)255);
		if (g_show_results) SaveGreyscaleImage(ImMaskWithBorderF, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_02_ImMaskWithBorderFByOpt2Res" + g_im_save_format, w, h);

		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_03_ImMASK" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK, ImMaskWithBorderF, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_02_05_04_ImMASKIntImMaskWithBorder" + g_im_save_format, w, h);
	}	

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_01_ImMASK" + g_im_save_format, w, h);

	RestoreImMask(ImClusters2, ImMASK, clusterCount2, w, h, iter_det + "_call1");	
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_04_01_ImMASKRestore" + g_im_save_format, w, h);	

	ClearImageByTextDistance(ImMASK, w, h, W, H, real_im_x_center, (u8)255, iter_det + wxT("_01"));
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_04_02_ImMASKByTextDistance" + g_im_save_format, w, h);

	if (g_show_results)
	{
		simple_buffer<u8> ImTMP(w * h);
		SaveGreyscaleImage(ImClusters2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_05_ImClusters2" + g_im_save_format, w, h);
		ImTMP.copy_data(ImClusters2, w * h);
		IntersectTwoImages(ImTMP, ImMASK, w, h);
		SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_06_ImClusters2InImMASK" + g_im_save_format, w, h);
	}
	
	int num_main_clusters = 0;
	int id_last_main_cluster = 0;
	simple_buffer<int> cnts(clusterCount2, 0);

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
		if (g_show_results)
		{
			ImMASK.set_values(0, w * h);
			SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_01_01_07_ImMASKRes" + g_im_save_format, w, h);
		}
		return 0;
	}	

	simple_buffer<u8> ImMainCluster(w * h), ImMainCluster2(w * h);

	// for save punctuation marks and on top symbols we need to get image with decreased number of clusters
	{
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_01_ImMASK" + g_im_save_format, w, h);
		simple_buffer<char> labels3(w * h);
		simple_buffer<u8> ImClusters3(w * h);
		simple_buffer<u8> ImMASKR(w * h, (u8)0), ImMASKR2(w * h, (u8)0);

		{
			int min_x, max_x, min_y, max_y;

			if (g_use_cuda_gpu)
			{
				res = cuda_kmeans(ImBGR, ImMASK, labels3, w, h, clusterCount3, g_cuda_kmeans_loop_iterations, g_cuda_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, true);
			}
			else
			{
				res = opencv_kmeans(ImBGR, ImMASK, labels3, w, h, clusterCount3, g_cpu_kmeans_loop_iterations, g_cpu_kmeans_initial_loop_iterations, min_x, max_x, min_y, max_y, iter_det, true);
			}

			if (res == 0)
			{
				return 0;
			}

			min_x = std::max<int>(0, min_x - 8);
			max_x = std::min<int>(w - 1, max_x + 8);
			min_y = std::max<int>(0, min_y - 8);
			max_y = std::min<int>(h - 1, max_y + 8);

			int rc = GetBGRColor(Red);

			if (g_show_results)
			{
				GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
				SaveGreyscaleImageWithLinesInfo(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_02_01_ImClusters3Orig" + g_im_save_format, w, h,
					{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
					{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });
			}

			if (g_use_ILA_images_before_clear_txt_images_from_borders)
			{				
				if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
				{
					if (ImIL.m_size)
					{
						IntersectTwoImages(labels3, ImIL, w, h, (char)-1);

						if (g_show_results)
						{
							GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
							SaveGreyscaleImageWithLinesInfo(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_02_02_ImClusters3IntImIL" + g_im_save_format, w, h,
								{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
								{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });
						}
					}

					FilterImageByPixelColorIsInRange(labels3, &ImBGR, &ImLab, w, h, iter_det + "_GetMainClusterImage_" + wxString::Format(wxT("%i"), __LINE__), (char)-1);

					if (g_show_results)
					{
						GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
						if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_02_03_ImClusters3FByPixelColorIsInRange" + g_im_save_format, w, h,
							{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
							{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });
					}
				}
			}			

			GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
			if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_02_03_ImClusters3Res" + g_im_save_format, w, h,
				{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
				{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });

			std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount3), [&ImClusters3, clusterCount3, w, h, min_h, real_im_x_center, min_x, max_x, min_y, max_y, rc, iter_det](int cluster_idx)
				{					
					int white = GetClusterColor(clusterCount3, cluster_idx);
					if (g_show_results)
					{
						simple_buffer<u8> ImTMP(w * h);
						GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
						SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_03_id" + std::to_string(cluster_idx) + "_01_ImCluster" + g_im_save_format, w, h,
							{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
							{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });
					}
					ClearImageOptimal2(ImClusters3, w, h, min_x, max_x, min_y, max_y, white);
					if (g_show_results)
					{						
						simple_buffer<u8> ImTMP(w * h);
						GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
						SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_03_id" + std::to_string(cluster_idx) + "_02_ImClusterFOptimal2" + g_im_save_format, w, h,
							{ std::pair<int, int>{min_y, rc}, std::pair<int, int>{max_y, rc} },
							{ std::pair<int, int>{min_x, rc}, std::pair<int, int>{max_x, rc} });
					}
				});

			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_01_ImClusters3FOptimal2" + g_im_save_format, w, h);

			if (!g_use_ILA_images_before_clear_txt_images_from_borders)
			{
				if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
				{
					if (ImIL.m_size)
					{
						IntersectTwoImages(ImClusters3, ImIL, w, h);
						if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_02_ImClusters3IntImIL" + g_im_save_format, w, h);
					}

					FilterImageByPixelColorIsInRange(ImClusters3, &ImBGR, &ImLab, w, h, iter_det + "_GetMainClusterImage_" + wxString::Format(wxT("%i"), __LINE__));
					if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_03_ImClusters3FByPixelColorIsInRange" + g_im_save_format, w, h);
				}
			}

			if (g_combine_to_single_cluster)
			{
				if (g_color_ranges.size() > 0)
				{
					IntersectTwoImages(labels3, ImClusters3, w, h, (char)-1);
					for (int i = 0; i < w * h; i++)
					{
						if (labels3[i] != -1)
						{
							labels3[i] = clusterCount3 - 1;
						}
					}					
					GetClustersImage(ImClusters3, labels3, clusterCount3, w, h);
					if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_01_04_ImClusters3Combined" + g_im_save_format, w, h);
				}
			}
		}

		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_02_01_ImMASK2" + g_im_save_format, w, h);
		
		{
			cvMAT cv_im_gr;
			GreyscaleImageToMat(ImMASK2, w, h, cv_im_gr);
			cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 2);			
			BinaryMatToImage(cv_im_gr, w, h, ImMASK2, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_02_02_ImMASK2Dilate" + g_im_save_format, w, h);
		}

		IntersectTwoImages(ImMASK2, ImMaskWithBorder, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_03_ImMASK2IntImMaskWithBorder" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK2, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_04_ImMASK2IntImClusters3" + g_im_save_format, w, h);

		ExtendAndFilterImFF(ImMASK2, w, h, W, H, iter_det, xb, xe, yb, ye);
		
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_06_ImMASK2FByExtendAndFilterImFF" + g_im_save_format, w, h);

		IntersectTwoImages(ImMASK2, ImMaskWithBorder, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_07_ImMASK2IntImMaskWithBorder" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK2, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_08_ImMASK2IntImClusters3" + g_im_save_format, w, h);

		if (ImIL.m_size)
		{
			IntersectTwoImages(ImMASK2, ImIL, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMASK2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_04_09_ImMASK2IntImIL" + g_im_save_format, w, h);
		}

		{
			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_05_01_ImClusters3" + g_im_save_format, w, h);

			std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount3), [&ImMASK2, &ImClusters3, clusterCount3, w, h](int cluster_idx)
			{
				ClearImageByMask(ImClusters3, ImMASK2, w, h, GetClusterColor(clusterCount3, cluster_idx));
			});

			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_05_02_ImClusters3FWithImMASK2" + g_im_save_format, w, h);
		}

		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_06_01_ImMASK" + g_im_save_format, w, h);
		IntersectTwoImages(ImMASK, ImClusters3, w, h);
		if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_06_02_ImMASKIntImClusters3" + g_im_save_format, w, h);

		ClearImageByTextDistance(ImMASK, w, h, W, H, real_im_x_center, (u8)255, iter_det + wxT("_02"));
		if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_02_07_ImMASKByTextDistance" + g_im_save_format, w, h);

		simple_buffer<u8> ImClusters3FR(ImClusters3);

		int sub_iter = 0;

		while (1)
		{
			sub_iter++;

			IntersectTwoImages(ImClusters3, ImMASK, w, h);
			if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_01_01_ImClusters3IntImMASK" + g_im_save_format, w, h);

			if (sub_iter == 1)
			{
				val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

				if (val == 1)
				{
					if (g_show_results) SaveGreyscaleImageWithSubParams(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_01_02_ImMASK_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);

					std::for_each(std::execution::par, ForwardIteratorForDefineRange<int>(0), ForwardIteratorForDefineRange<int>(clusterCount3), [&ImClusters3, clusterCount3, w, h, W, H, lb, le, LH, LMAXY, min_h, real_im_x_center, iter_det, sub_iter, yb, ye](int cluster_idx)
					{
						u8 white = GetClusterColor(clusterCount3, cluster_idx);
						if (g_show_results)
						{
							simple_buffer<u8> ImTMP(w * h);
							GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
							SaveGreyscaleImageWithSubParams(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_01_ImCluster" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
						}
						ClearImageOpt4(ImClusters3, w, h, W, H, LH, LMAXY, real_im_x_center, white);
						if (g_show_results)
						{
							simple_buffer<u8> ImTMP(w * h);
							GetBinaryImageFromClustersImageByClusterId(ImTMP, ImClusters3, cluster_idx, clusterCount3, w, h);
							SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_03_id" + std::to_string(cluster_idx) + "_02_ImClusterFOptimal4" + g_im_save_format, w, h);
						}						
					});
					if (g_show_results) SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_04_ImClusters3FOptimal4Res" + g_im_save_format, w, h);

					if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_05_01_ImMASK" + g_im_save_format, w, h);
					IntersectTwoImages(ImMASK, ImClusters3, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_05_02_ImMASKOpt4" + g_im_save_format, w, h);

					ClearImageByTextDistance(ImMASK, w, h, W, H, real_im_x_center, (u8)255, iter_det + wxT("_03"));
					if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_01_05_03_ImMASKOpt4FByTextDistance" + g_im_save_format, w, h);
				}

				ImMASKR2.copy_data(ImMASK, w * h);
			}

			val = num_main_clusters;
			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, xb, xe, yb, ye, deleted_by_location, iter_det + "_call1_sub_iter" + std::to_string(sub_iter), real_im_x_center);

			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_02_01_ImMASKIntImClusters3FByFilterImMask" + g_im_save_format, w, h);

			if ((num_main_clusters > 1) && (deleted_by_location > 0))
			{
				num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, xb, xe, yb, ye, deleted_by_location, iter_det + "_call2_sub_iter" + std::to_string(sub_iter), real_im_x_center);

				if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_02_02_ImMASKIntImClusters3FByFilterImMask" + g_im_save_format, w, h);
			}

			if (g_show_results)
			{
				simple_buffer<u8> ImTMP(w * h);
				ImTMP.copy_data(ImClusters3, w * h);
				IntersectTwoImages(ImTMP, ImMASK, w, h);
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_03_ImClusters3InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImClusters3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_04_ImClusters3Orig" + g_im_save_format, w, h);
			}

			if (num_main_clusters <= 2)
			{
				break;
			}
			else
			{
				if (num_main_clusters > 0)
				{
					ImMASKR.copy_data(ImMASK, w* h);
				}

				if (sub_iter == 3)
				{
					break;
				}

				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImMASK, w, h, cv_im_gr);

				cv::Mat kernel = cv::Mat::ones(5, 5, CV_8U);
				cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);

				{
					simple_buffer<u8> ImTMP(w * h);

					if (g_show_results)
					{
						BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);
						if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_16_ImMASKClose" + g_im_save_format, w, h);
					}

					int erosion_size = 1, erosion_type = 0;
					cv::Mat element = cv::getStructuringElement(erosion_type,
						cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
						cv::Point(erosion_size, erosion_size));

					cv::erode(cv_im_gr, cv_im_gr, element, cv::Point(-1, -1), 2);
					//cv::imshow("ImErode: " + iter_det, cv_im_gr);

					if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_17_ImMASK" + g_im_save_format, w, h);
					BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);
					IntersectTwoImages(ImMASK, ImTMP, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_03_02_04_sub_iter" + std::to_string(sub_iter) + "_18_ImMASKErode" + g_im_save_format, w, h);
				}
			}
		}

		if (num_main_clusters > 1)
		{
			if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
			{
				num_main_clusters = 1;
			}
			else
			{
				int min_y = yb, max_y = ye;

				val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

				if (val == 1)
				{
					max_y = LMAXY;
					min_y = LMAXY - LH + 1;
				}

				val = GetMainTwoClustersByAverageLuminanceDifference(ImMainCluster, ImMainCluster2, num_main_clusters, ImMASK, ImY, labels3, clusterCount3, w, h, iter_det + "_clusters3", minX, maxX, min_y, max_y);

				if ((val == 0) && (num_main_clusters > 1))
				{
					val = GetMainTwoClustersByAverageLuminanceDifference(ImMainCluster, ImMainCluster2, num_main_clusters, ImMASK, ImY, labels2, clusterCount2, w, h, iter_det + "_clusters2", minX, maxX, min_y, max_y);
				}

				if (val == 1)
				{
					num_main_clusters = 1;
					IntersectTwoImages(ImMASK, ImMainCluster, w, h);
					if (g_show_results) SaveGreyscaleImageWithSubParams(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASKFByMainClaster1" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				}
			}
		}

		if (num_main_clusters > 1)
		{
			ImMASK.copy_data(ImMASKR, w * h);
			ImClusters3.copy_data(ImClusters3FR, w * h);
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASK" + g_im_save_format, w, h);
			RestoreImMask(ImClusters3, ImMASK, clusterCount3, w, h, iter_det + "_call3");
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASKRestore" + g_im_save_format, w, h);
			IntersectTwoImages(ImMASK, ImMASKR2, w, h);
			if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASKFImMASKR2" + g_im_save_format, w, h);

			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, xb, xe, yb, ye, deleted_by_location, iter_det + "_call3", real_im_x_center);

			if ((num_main_clusters > 1) && (deleted_by_location > 0))
			{
				num_main_clusters = FilterImMask(ImClusters3, ImMASK, labels3, clusterCount3, w, h, xb, xe, yb, ye, deleted_by_location, iter_det + "_call4", real_im_x_center);
			}

			if (g_show_results)
			{
				simple_buffer<u8> ImTMP(w* h);
				SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASKIntImClusters3FByLocation" + g_im_save_format, w, h);
				ImTMP.copy_data(ImClusters3, w* h);
				IntersectTwoImages(ImTMP, ImMASK, w, h);
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters3InImMASK" + g_im_save_format, w, h);
			}
		}

		if (num_main_clusters > 1)
		{
			simple_buffer<u8> ImRES2(w * h), ImRES3(w * h), ImRES4(w * h);			

			SortClusters(labels2, cluster_id2, cluster_cnt2, ImMASK, clusterCount2, w, h, minX, maxX, yb, ye);
			GetBinaryImageByClusterId(ImMainCluster, labels2, cluster_id2[0], w, h);
			GetBinaryImageByClusterId(ImMainCluster2, labels2, cluster_id2[1], w, h);

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterInImClusters2" + g_im_save_format, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2InImClusters2" + g_im_save_format, w, h);

			ImRES2.copy_data(ImMainCluster, w * h);
			CombineTwoImages(ImRES2, ImMainCluster2, w, h, (u8)255);
			if (g_show_results) SaveGreyscaleImage(ImRES2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_Im2MainClustersInImClusters2" + g_im_save_format, w, h);

			ImRES4.copy_data(ImRES2, w * h);
			IntersectTwoImages(ImRES4, ImMASK, w, h);
			if (g_show_results) SaveGreyscaleImage(ImRES4, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_Im2MainClustersInImClusters2IntWithImMASK" + g_im_save_format, w, h);
			{
				cvMAT cv_im_gr;
				GreyscaleImageToMat(ImRES4, w, h, cv_im_gr);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 8);
				BinaryMatToImage(cv_im_gr, w, h, ImRES4, (u8)255);
				if (g_show_results) SaveGreyscaleImage(ImRES4, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_Im2MainClustersInImClusters2Dilate" + g_im_save_format, w, h);
			}
			IntersectTwoImages(ImRES4, ImRES2, w, h);
			GetClustersImage(ImClusters2, labels2, clusterCount2, w, h);
			IntersectTwoImages(ImClusters2, ImRES4, w, h);

			if (g_show_results)
			{
				SaveGreyscaleImage(ImRES4, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_Im2MainClustersInImClusters2IntOrig2MainClustersInImClusters2" + g_im_save_format, w, h);
				ImRES3.copy_data(ImClusters2, w* h);
				IntersectTwoImages(ImRES3, ImRES4, w, h);
				SaveGreyscaleImage(ImRES3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters2InIm2MainClustersInImClusters2IntOrig2MainClustersInImClusters2" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImClusters2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters2" + g_im_save_format, w, h);
			}

			int deleted_by_location = 0;
			num_main_clusters = FilterImMask(ImClusters2, ImRES4, labels2, clusterCount2, w, h, xb, xe, yb, ye, deleted_by_location, iter_det + "_call5", real_im_x_center);

			if (num_main_clusters < 2)
			{
				IntersectTwoImages(ImMASK, ImRES4, w, h);
			}
			else
			{
				int len1 = 0, len2 = 0;
				int cnt1 = cluster_cnt2[0], cnt2 = cluster_cnt2[1];

				ImRES2.copy_data(ImMainCluster, w* h);
				IntersectTwoImages(ImRES2, ImRES4, w, h);
				ImRES3.copy_data(ImMainCluster2, w* h);
				IntersectTwoImages(ImRES3, ImRES4, w, h);

				if (g_show_results) SaveGreyscaleImage(ImRES2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterInImClusters2IntImMASK" + g_im_save_format, w, h);
				if (g_show_results) SaveGreyscaleImage(ImRES3, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2InImClusters2IntImMASK" + g_im_save_format, w, h);

				{
					cvMAT cv_im;
					vector<vector<cv::Point> > contours;
					vector<cv::Vec4i> hierarchy;

					BinaryImageToMat(ImRES2, w, h, cv_im);
					cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
					for (j = 0; j < contours.size(); j++) len1 += cv::arcLength(contours[j], true);
				}

				{
					cvMAT cv_im;
					vector<vector<cv::Point> > contours;
					vector<cv::Vec4i> hierarchy;

					BinaryImageToMat(ImRES3, w, h, cv_im);
					cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
					for (j = 0; j < contours.size(); j++) len2 += cv::arcLength(contours[j], true);
				}

				if ((len1 > len2*1.2) && (cnt2 >= cnt1 * 0.4))
				{
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
			if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASK" + g_im_save_format, w, h);

			SortClusters(labels3, cluster_id3, cluster_cnt3, ImMASK, clusterCount3, w, h, minX, maxX, yb, ye);
			GetBinaryImageByClusterId(ImMainCluster, labels3, cluster_id3[0], w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterInImClusters3" + g_im_save_format, w, h);

			SortClusters(labels2, cluster_id2, cluster_cnt2, ImMASK, clusterCount2, w, h, minX, maxX, yb, ye);
			GetBinaryImageByClusterId(ImMainCluster2, labels2, cluster_id2[0], w, h);

			if (g_show_results)
			{
				simple_buffer<u8> ImTMP(w * h);
				SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASK" + g_im_save_format, w, h);
				ImTMP.copy_data(ImClusters2, w* h);
				IntersectTwoImages(ImTMP, ImMASK, w, h);
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters2InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterInImClusters2" + g_im_save_format, w, h);
			}

			run_in_parallel(
				[&ImMainCluster, &ImMaskWithBorder, &ImIL, &ImBGR, &ImLab, w, h, iter_det] {
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster" + g_im_save_format, w, h);
				if (g_use_ILA_images_before_clear_txt_images_from_borders)
				{
					if (ImIL.m_size)
					{
						IntersectTwoImages(ImMainCluster, ImIL, w, h);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterIntImIL" + g_im_save_format, w, h);
					}
				}

				ClearImageOptimal(ImMainCluster, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterFOptimal" + g_im_save_format, w, h);				

				if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
				{
					simple_buffer<u8> ImMainClusterFOptimal(ImMainCluster);
					FilterImageByPixelColorIsInRange(ImMainCluster, &ImBGR, &ImLab, w, h, iter_det + "_GetMainClusterImage_" + wxString::Format(wxT("%i"), __LINE__));
					if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterFByPixelColorIsInRange" + g_im_save_format, w, h);
					simple_buffer<u8> ImTMP(ImMainCluster);
					IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterIntImMaskWithBorder" + g_im_save_format, w, h);
					ClearImageByMask(ImTMP, ImMainCluster, w, h, (u8)255);					
					if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterFClearImageByMask" + g_im_save_format, w, h);
					IntersectTwoImages(ImMainCluster, ImTMP, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterIntImMainClusterFClearImageByMask" + g_im_save_format, w, h);
					MergeImagesByIntersectedFigures(ImMainCluster, ImMainClusterFOptimal, w, h, (u8)255);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterRestored" + g_im_save_format, w, h);
				}
				else
				{
					IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterFByMaskWithBorder" + g_im_save_format, w, h);
				}
			},
				[&ImMainCluster2, &ImMaskWithBorder, &ImIL, &ImBGR, &ImLab, w, h, iter_det] {
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2" + g_im_save_format, w, h);				
				if (g_use_ILA_images_before_clear_txt_images_from_borders)
				{
					if (ImIL.m_size)
					{
						IntersectTwoImages(ImMainCluster2, ImIL, w, h);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2IntImIL" + g_im_save_format, w, h);
					}
				}

				ClearImageOptimal(ImMainCluster2, w, h, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2FOptimal" + g_im_save_format, w, h);
				
				if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
				{
					simple_buffer<u8> ImMainCluster2FOptimal(ImMainCluster2);
					FilterImageByPixelColorIsInRange(ImMainCluster2, &ImBGR, &ImLab, w, h, iter_det + "_GetMainClusterImage_" + wxString::Format(wxT("%i"), __LINE__));
					if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2FByPixelColorIsInRange" + g_im_save_format, w, h);
					simple_buffer<u8> ImTMP(ImMainCluster2);
					IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2IntImMaskWithBorder" + g_im_save_format, w, h);
					ClearImageByMask(ImTMP, ImMainCluster2, w, h, (u8)255);
					if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2FClearImageByMask" + g_im_save_format, w, h);
					IntersectTwoImages(ImMainCluster2, ImTMP, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2IntImMainCluster2FClearImageByMask" + g_im_save_format, w, h);
					MergeImagesByIntersectedFigures(ImMainCluster2, ImMainCluster2FOptimal, w, h, (u8)255);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2Restored" + g_im_save_format, w, h);
				}
				else
				{
					IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
					if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);
				}
			}
			);
		}
		else if (num_main_clusters > 1)
		{
			val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

			run_in_parallel(
				[&ImMainCluster, &ImMaskWithBorder, w, h, LH, LMAXY, lb, le, real_im_x_center, iter_det] {
				IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainClusterFByMaskWithBorder" + g_im_save_format, w, h);
			},
				[&ImMainCluster2, &ImMaskWithBorder, w, h, LH, LMAXY, lb, le, real_im_x_center, iter_det] {
				IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);				
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
			}
			
			{
				cvMAT cv_im;
				vector<vector<cv::Point> > contours;
				vector<cv::Vec4i> hierarchy;

				BinaryImageToMat(ImMainCluster2, w, h, cv_im);
				cv::findContours(cv_im, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
				for (j = 0; j < contours.size(); j++) len2 += cv::arcLength(contours[j], true);
			}

			if ( (len1 > len2*1.2) && (cnt2 >= cnt1*0.4) )
			{
				ImMainCluster.copy_data(ImMainCluster2, w* h);
			}			

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_03_ImMainCluster" + g_im_save_format, w, h);

			IntersectTwoImages(ImMASK, ImMainCluster, w, h);

			SortClusters(labels3, cluster_id3, cluster_cnt3, ImMASK, clusterCount3, w, h, minX, maxX, yb, ye);
			GetBinaryImageByClusterId(ImMainCluster2, labels3, cluster_id3[0], w, h);

			if (g_show_results)
			{
				simple_buffer<u8> ImTMP(w* h);
				SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_04_ImMASK" + g_im_save_format, w, h);
				ImTMP.copy_data(ImClusters2, w* h);
				IntersectTwoImages(ImTMP, ImMASK, w, h);
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_05_ImClusters2InImMASK" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_06_ImMainClusterInImClusters3" + g_im_save_format, w, h);
			}

			IntersectTwoImages(ImMainCluster2, ImMaskWithBorder, w, h);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_06_ImMainCluster2FByMaskWithBorder" + g_im_save_format, w, h);
			ClearImageOptimal(ImMainCluster2, w, h, 255);
			if (g_show_results) SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_04_08_ImMainCluster2FOptimal" + g_im_save_format, w, h);
		}
		else // num_main_clusters == 0
		{
			if (g_show_results)
			{
				ImMASK.set_values(0, w* h);
				SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_03_04_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImMASKRes" + g_im_save_format, w, h);
			}
			return 0;
		}
	}

	ImRES.copy_data(ImMainCluster, w* h);
	val = GetSubParams(ImMASK, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

	if (g_show_results)
	{
		if (val == 1)
		{
			SaveGreyscaleImageWithSubParams(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_01_ImMASK_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
			SaveGreyscaleImageWithSubParams(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_02_ImMainCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
			SaveGreyscaleImageWithSubParams(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_03_ImMainCluster2_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
		}
		else
		{
			SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_01_ImMASK" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_02_ImMainCluster" + g_im_save_format, w, h);
			SaveGreyscaleImage(ImMainCluster2, "/DebugImages/GetMainClusterImage_" + iter_det + "_05_03_ImMainCluster2" + g_im_save_format, w, h);			
		}
	}

	if (val == 0)
	{		
		simple_buffer<u8> ImTMP(ImMainCluster2, 0, w * h);
		IntersectTwoImages(ImTMP, ImRES, w, h);		

		val = GetSubParams(ImTMP, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);

		if (g_show_results)
		{
			if (val == 1)
			{
				SaveGreyscaleImageWithSubParams(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_06_ImMainClusterIntImMainCluster2_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
			}
			else
			{
				SaveGreyscaleImage(ImTMP, "/DebugImages/GetMainClusterImage_" + iter_det + "_06_ImMainClusterIntImMainCluster2" + g_im_save_format, w, h);
			}
		}
	}

	CombineTwoImages(ImRES, ImMainCluster2, w, h, (u8)255);
	if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainClusterImage_" + iter_det + "_07_1_ImMainClusteCombinedWithImMainCluster2" + g_im_save_format, w, h);

	if (!g_extend_by_grey_color)
	{
		if (val == 0)
		{
			val = GetSubParams(ImRES, w, h, 255, LH, LMAXY, lb, le, min_h, real_im_x_center, yb, ye, iter_det);
		}

		if (val == 1)
		{
			if (g_show_results) SaveGreyscaleImageWithSubParams(ImRES, "/DebugImages/GetMainClusterImage_" + iter_det + "_07_2_ImMainCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
			ClearImageOpt5(ImRES, w, h, LH, LMAXY, real_im_x_center, 255);
			if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainClusterImage_" + iter_det + "_07_3_ImMainClusterFOpt5" + g_im_save_format, w, h);
		}
	}

	ClearImageByTextDistance(ImRES, w, h, W, H, real_im_x_center, (u8)255, iter_det + wxT("_04"));
	if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainClusterImage_" + iter_det + "_07_4_ImMainClusterFByTextDistance" + g_im_save_format, w, h);

	//start_time = GetTickCount();

	res = 1;
	return res;
}

class FindTextRes
{
public:
	int m_res;
	cv::Mat m_cv_ImClearedText;
	cv::Mat m_cv_ImClearedTextScaled;
	int m_im_h;
	wxString m_ImageName;
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
	simple_buffer<int> m_LL;
	simple_buffer<int> m_LR;
	simple_buffer<int> m_LLB;
	simple_buffer<int> m_LLE;
	int m_N;
	int m_k;
	wxString m_iter_det;

	FindTextRes()
	{
		m_res = 0;
	}
};

void GetMainColorImage(simple_buffer<u8>& ImRES, simple_buffer<u8>* pImMainCluster, simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImY, simple_buffer<u8>& ImU, simple_buffer<u8>& ImV, int w, int h, wxString iter_det, int min_x, int max_x, int min_y, int max_y)
{
	simple_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	int delta, val1, val2, val3, val4, NN, ys1;
	int i, j, j1, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int xb, xe, yb, ye, x, y;

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/GetMainColorImage_" + iter_det + "_01_01_ImMASK" + g_im_save_format, w, h);		

	yb = min_y;
	ye = max_y;
	xb = min_x;
	xe = max_x;	

	ImRES.copy_data(ImMASK, w * h);	

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
		ImRES.set_values(0, w * h);
		if (pImMainCluster != NULL)
		{
			pImMainCluster->set_values(0, w * h);			
		}
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
	if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainColorImage_" + iter_det + "_01_02_ImMASKFilteredByY" + g_im_save_format, w, h);

	int j2, ys2, j3, ys3, min_delta;	

	delta = 40;
	StrAnalyseImage(ImRES, ImU, GRStr, w, h, xb, xe, yb, ye, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
	FindMaxStr(smax, smaxi, j2, ys2, NN);
	j2_min = j2;
	j2_max = j2_min + delta - 1;

	if (NN == 0)
	{
		ImRES.set_values(0, w * h);
		if (pImMainCluster != NULL)
		{
			pImMainCluster->set_values(0, w * h);
		}		
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
	if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainColorImage_" + iter_det + "_01_03_ImMASKFilteredByYU" + g_im_save_format, w, h);
	
	delta = 40;
	StrAnalyseImage(ImRES, ImV, GRStr, w, h, xb, xe, yb, ye, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
	FindMaxStr(smax, smaxi, j3, ys3, NN);
	j3_min = j3;
	j3_max = j3_min + delta - 1;

	if (NN == 0)
	{
		ImRES.set_values(0, w * h);
		if (pImMainCluster != NULL)
		{
			pImMainCluster->set_values(0, w * h);
		}		
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
	if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/GetMainColorImage_" + iter_det + "_01_04_ImMASKFilteredByYUV" + g_im_save_format, w, h);	
	
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
		simple_buffer<u8> &ImMainCluster = *pImMainCluster;

		if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainColorImage_" + iter_det + "_02_01_MainTXTCluster" + g_im_save_format, w, h);

		if (!g_extend_by_grey_color)
		{
			for (i = 0; i < w * h; i++)
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

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainColorImage_" + iter_det + "_02_02_MainTXTClusterFilteredByColor" + g_im_save_format, w, h);
		}
		else
		{
			simple_buffer<u8> ImMainClusterOrig(ImMainCluster);

			j1_min = std::min<int>(g_allow_min_luminance, j1_min);

			for (i = 0; i < w * h; i++)
			{
				val1 = ImY[i];
				val2 = ImU[i];
				val3 = ImV[i];

				if ( (val1 >= j1_min) && (val1 <= j1_max) &&
					 (val2 >= j2_min) && (val2 <= j2_max) &&
					 (val3 >= j3_min) && (val3 <= j3_max) )
				{
					ImMainCluster[i] = 255;
				}
				else
				{
					ImMainCluster[i] = 0;
				}
			}

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainColorImage_" + iter_det + "_02_03_MainColor" + g_im_save_format, w, h);

			MergeImagesByIntersectedFigures(ImMainCluster, ImMainClusterOrig, w, h, (u8)255);

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/GetMainColorImage_" + iter_det + "_02_04_MainTXTClusterExtByMainColor" + g_im_save_format, w, h);
		}
	}
}

template <class T>
void ExtendByInsideFigures(simple_buffer<T>& ImRES, int w, int h, T white, bool simple)
{
	simple_buffer<T> ImTMP(w * h);
	int val, val1, val2;

	// Extend ImSNF with Internal Figures
	val = GetImageWithInsideFigures(ImRES, ImTMP, w, h, white, simple);
	if (val > 0)
	{
		CombineTwoImages(ImRES, ImTMP, w, h, white);
	}
}

int CheckOnSubPresence(simple_buffer<u8>& ImMASK, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImFRes, int w_scaled, int h_scaled, int w_orig, int h_orig, int W_orig, int H_orig, int xb, int yb, wxString iter_det)
{
	simple_buffer<u8> ImTMP;
	int i, j, x, y, ww, hh, res = 0;

	cv::ocl::setUseOpenCL(g_use_ocl);

	ImFRes.set_values(0, w_scaled * h_scaled);

	if (g_show_results) SaveGreyscaleImage(ImMASK, "/DebugImages/CheckOnSubPresence_" + iter_det + "_01_01_ImFF" + g_im_save_format, w_scaled, h_scaled);

	{
		cvMAT cv_im_gr, cv_im_gr_resize;
		BinaryImageToMat(ImMASK, w_scaled, h_scaled, cv_im_gr);
		custom_assert(g_scale > 0, "CheckOnSubPresence: g_scale > 0");
		cv::resize(cv_im_gr, cv_im_gr_resize, cv::Size(0, 0), 1.0 / g_scale, 1.0 / g_scale);
		ww = cv_im_gr_resize.cols;
		hh = cv_im_gr_resize.rows;
		cv::dilate(cv_im_gr_resize, cv_im_gr_resize, cv::Mat(), cv::Point(-1, -1), 1);
		ImTMP.set_size(ww * hh);
		BinaryMatToImage(cv_im_gr_resize, ww, hh, ImTMP, (u8)255);

		if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/CheckOnSubPresence_" + iter_det + "_01_02_ImFFDilate" + g_im_save_format, ww, hh);
	}

	{
		simple_buffer<u8> ImFFD(w_orig * hh, (u8)0), ImSF(w_orig * hh), ImTF(w_orig * hh);

		for (y = 0, i = xb, j = 0; y < hh; y++, i += w_orig, j += ww)
		{
			ImFFD.copy_data(ImTMP, i, j, ww);
		}

		if (g_show_results) SaveGreyscaleImage(ImFFD, "/DebugImages/CheckOnSubPresence_" + iter_det + "_02_ImFFAligned" + g_im_save_format, w_orig, hh);

		ImSF.copy_data(ImFFD, w_orig * hh);

		simple_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = hh - 1;
		simple_buffer<u8> ImNE_sub_buffer(ImNE, yb * w_orig);
		res = FilterTransformedImage(ImFFD, ImSF, ImTF, ImNE_sub_buffer, LB, LE, 1, w_orig, hh, W_orig, H_orig, 0, w_orig - 1, iter_det);

		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/CheckOnSubPresence_" + iter_det + "_03_ImFF_F" + g_im_save_format, w_orig, hh);

		if (res == 0)
		{
			return res;
		}

		for (y = 0, i = xb, j = 0; y < hh; y++, i += w_orig, j += ww)
		{
			ImTMP.copy_data(ImTF, j, i, ww);
		}
	}

	if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/CheckOnSubPresence_" + iter_det + "_04_ImFF_F_Aligned" + g_im_save_format, ww, hh);

	{
		cvMAT cv_im_gr, cv_im_gr_resize;
		BinaryImageToMat(ImTMP, ww, hh, cv_im_gr);
		cv::resize(cv_im_gr, cv_im_gr_resize, cv::Size(0, 0), g_scale, g_scale);
		BinaryMatToImage(cv_im_gr_resize, w_scaled, h_scaled, ImFRes, (u8)255);
	}

	if (g_show_results) SaveGreyscaleImage(ImFRes, "/DebugImages/CheckOnSubPresence_" + iter_det + "_05_ImFF_F" + g_im_save_format, w_scaled, h_scaled);

	IntersectTwoImages(ImFRes, ImMASK, w_scaled, h_scaled);
	if (g_show_results) SaveGreyscaleImage(ImFRes, "/DebugImages/CheckOnSubPresence_" + iter_det + "_06_ImFF_F_IntImMASK" + g_im_save_format, w_scaled, h_scaled);

	MergeImagesByIntersectedFigures(ImFRes, ImMASK, w_scaled, h_scaled, (u8)255);
	if (g_show_results) SaveGreyscaleImage(ImFRes, "/DebugImages/CheckOnSubPresence_" + iter_det + "_07_ImMASKMergeImFF_F" + g_im_save_format, w_scaled, h_scaled);

	return res;
}

wxString FormatImInfoAddData(int W, int H, int xmin, int ymin, int w, int h, int ln)
{
	return wxString::Format(wxT("%.1d%.4d%.4d%.4d%.4d%.4d%.4d"), ln, xmin, ymin, w, h, W, H);
}

bool DecodeImData(wxString FileName, int* pW, int* pH, int* pmin_x, int* pmin_y, int* p_w, int* p_h, int* p_ln, wxString* pBaseName)
{
	bool res = false;

	wxString str_ymin, str_xmin, str_H, str_W, str_w, str_h, str_ln;

	wxRegEx re(wxT("^(.+)_([[:digit:]]{1})([[:digit:]]{4})([[:digit:]]{4})([[:digit:]]{4})([[:digit:]]{4})([[:digit:]]{4})([[:digit:]]{4})$"));
	if (re.Matches(FileName))
	{
		if (pBaseName) *pBaseName = re.GetMatch(FileName, 1);
		str_ln = re.GetMatch(FileName, 2);
		str_xmin = re.GetMatch(FileName, 3);
		str_ymin = re.GetMatch(FileName, 4);		
		str_w = re.GetMatch(FileName, 5);
		str_h = re.GetMatch(FileName, 6);
		str_W = re.GetMatch(FileName, 7);
		str_H = re.GetMatch(FileName, 8);
		
		
		if (p_ln) *p_ln = wxAtoi(str_ln);
		if (pmin_x) *pmin_x = wxAtoi(str_xmin);
		if (pmin_y) *pmin_y = wxAtoi(str_ymin);
		if (p_w) *p_w = wxAtoi(str_w);
		if (p_h) *p_h = wxAtoi(str_h);
		if (pW) *pW = wxAtoi(str_W);
		if (pH) *pH = wxAtoi(str_H);

		res = true;
	}

	return res;
}

void GetImInfo(wxString FileName, int img_real_w, int img_real_h, int *pW, int* pH, int* pmin_x, int* pmax_x, int* pmin_y, int* pmax_y, wxString* pBaseName, int* pScale)
{
	int w, h, ln, min_x, min_y, W, H;

	if (DecodeImData(FileName, &W, &H, &min_x, &min_y, &w, &h, &ln, pBaseName))
	{		
		int scale = img_real_h/h;

		if (pScale) *pScale = scale;
		if (pW) *pW = scale * W;
		if (pH) *pH = scale * H;
		if (pmin_x) *pmin_x = scale * min_x;
		if (pmin_y) *pmin_y = scale * min_y;
		if (pmax_x) *pmax_x = (scale * min_x) + img_real_w - 1;
		if (pmax_y) *pmax_y = (scale * min_y) + img_real_h - 1;
	}
	else
	{
		if (pBaseName) *pBaseName = FileName;
		if (pScale) *pScale = 1;
		if (pW) *pW = img_real_w;
		if (pH) *pH = img_real_h;
		if (pmin_x) *pmin_x = 0;
		if (pmin_y) *pmin_y = 0;
		if (pmax_x) *pmax_x = img_real_w - 1;
		if (pmax_y) *pmax_y = img_real_h - 1;
	}
}

void FindText(FindTextRes &res, simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImF, simple_buffer<u8> &ImNF, simple_buffer<u8> &ImNE, simple_buffer<u8> &FullImIL, simple_buffer<u8> &FullImY, wxString SaveDir, wxString SaveName, wxString iter_det, int N, const int k, simple_buffer<int> LL, simple_buffer<int> LR, simple_buffer<int> LLB, simple_buffer<int> LLE, const int w_orig, const int h_orig, const int W_orig, const int H_orig, const int xmin_orig, const int ymin_orig)
{
	int i, j, l, r, x, y, ib, bln, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, ww, hh, cnt;
	int XB, XE, YB, YE, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, lb, le, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, cnt3, cnt4, cnt5, NN, ys1, ys2, ys3, val_min, val_max;
	int j1, j2, j3, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int mY, mI, mQ;
	int LH, LMAXY;
	int real_im_x_center;
	simple_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	int color, rc, gc, bc, yc, cc, wc, min_h;
	u8 *pClr;
	//DWORD start_time;
	const int mpn = g_mpn;

	cv::ocl::setUseOpenCL(g_use_ocl);

	//start_time = GetTickCount();

	if (g_show_results)	SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/FindText_" + iter_det + "_01_ImBGRWithLinesInfo" + g_im_save_format, LLB, LLE, N, w_orig, h_orig, k);

	min_h = g_min_h * H_orig;

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
		if (LLE[k] > h_orig - 1) LLE[k] = h_orig - 1;

		YB = LLB[k];
		YE = LLE[k];
		h = YE - YB + 1;
	}

	YB -= h / 2;
	YE += h / 2;
	if (YB < 0) YB = 0;
	if (YE > h_orig - 1) YE = h_orig - 1;

	if (k > 0)
	{
		val = (LLB[k - 1] + LLE[k - 1]) / 2;
		if (YB < val) YB = val;
	}
	if (k < N - 1)
	{
		val = (LLB[k + 1] + LLE[k + 1]) / 2;
		if (YE > val) YE = val;
	}

	{
		int max_diff = 20;

		for (y = YB; y < YE; y++)
		{
			bln = 0;
			for (x = LL[k], val1 = val2 = FullImY[y * w_orig + x]; x <= LR[k]; x++)
			{
				i = y * w_orig + x;
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
			for (x = LL[k], val1 = val2 = FullImY[y * w_orig + x]; x <= LR[k]; x++)
			{
				i = y * w_orig + x;
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
		YE = std::min<int>(y + 2, h_orig - 1);
	}

	if (YE - YB < (2 * min_h) / 3)
	{
		return;
	}

	if (YB > orig_LLBk - 2)
	{
		orig_LLBk = YB;
		YB = std::max<int>(0, YB - 2);
	}

	if (YE < orig_LLEk + 2)
	{
		orig_LLEk = YE;
		YE = std::min<int>(h_orig - 1, YE + 2);
	}

	if (LLB[k] < YB) LLB[k] = YB;
	if (LLE[k] > YE) LLE[k] = YE;

	h = YE - YB + 1;

	real_im_x_center = w_orig/2;

	XB = LL[k];
	XE = LR[k];
	
	w = XE - XB + 1;	
	val = (int)((double)w * 0.15);
	if (val < 40) val = 40;
	XB -= val;
	XE += val;
	if (XB < 0) XB = 0;
	if (XE > w_orig - 1) XE = w_orig - 1;	
	
	w = XE - XB + 1;

	if ( (g_text_alignment == TextAlignment::Center) ||
		 (g_text_alignment == TextAlignment::Left) )
	{
		if (real_im_x_center <= XB)
		{
			return;
		}
	}

	simple_buffer<u8> ImBGRScaled((int)(w * g_scale) * (int)(h * g_scale) * 3);
	simple_buffer<u8> ImLab((int)(w* g_scale)* (int)(h* g_scale) * 3);
	simple_buffer<u8> ImSNF((int)(w*g_scale)*(int)(h*g_scale)), ImFF((int)(w*g_scale)*(int)(h*g_scale));
	simple_buffer<u8> ImY((int)(w*g_scale)*(int)(h*g_scale)), ImU((int)(w*g_scale)*(int)(h*g_scale)), ImV((int)(w*g_scale)*(int)(h*g_scale));
	simple_buffer<u8> ImL((int)(w*g_scale)*(int)(h*g_scale)), ImA((int)(w*g_scale)*(int)(h*g_scale)), ImB((int)(w*g_scale)*(int)(h*g_scale));	
	simple_buffer<u8> ImIL((int)(w*g_scale)*(int)(h*g_scale));
	simple_buffer<u8> ImTHRMerge((w*g_scale)*(int)(h*g_scale));	
	simple_buffer<u8> ImSNFS;	

	if (g_show_results)
	{
		std::vector<std::pair<int, int>> HLPC, VLPC;

		for (i = 0; i < N; i++)
		{
			HLPC.push_back(std::pair<int, int>{LLB[i], rc});
			HLPC.push_back(std::pair<int, int>{LLE[i], gc});
		}
		
		HLPC.push_back(std::pair<int, int>{YB, bc});
		HLPC.push_back(std::pair<int, int>{YE, bc});
		VLPC.push_back(std::pair<int, int>{XB, bc});
		VLPC.push_back(std::pair<int, int>{XE, bc});

		HLPC.push_back(std::pair<int, int>{orig_LLBk, yc});
		HLPC.push_back(std::pair<int, int>{orig_LLEk, yc});
		VLPC.push_back(std::pair<int, int>{orig_LLk, yc});
		VLPC.push_back(std::pair<int, int>{orig_LRk, yc});

		SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/FindText_" + iter_det + "_03_1_ImBGR_WithLinesInfo" + g_im_save_format, w_orig, h_orig, HLPC, VLPC);
	}

	{
		simple_buffer<u8> ImSNFOrig((int)(w * g_scale) * (int)(h * g_scale));

		{
			cv::Mat cv_ImBGR;

			run_in_parallel(
				[&ImBGR, &ImBGRScaled, &cv_ImBGR, YB, YE, XB, w_orig, w, h, iter_det] {
					int y, i, j;
					simple_buffer<u8> ImBGROrig(w * h * 3);

					for (y = YB, i = YB * w_orig + XB, j = 0; y <= YE; y++, i += w_orig, j += w)
					{
						ImBGROrig.copy_data(ImBGR, j * 3, i * 3, w * 3);
					}

					if (g_show_results) SaveBGRImage(ImBGROrig, "/DebugImages/FindText_" + iter_det + "_03_2_1_ImRES1_BGR" + g_im_save_format, w, h);

					cv::Mat cv_ImBGROrig;
					BGRImageToMat(ImBGROrig, w, h, cv_ImBGROrig);
					cv::resize(cv_ImBGROrig, cv_ImBGR, cv::Size(0, 0), g_scale, g_scale);
					ImBGRScaled.copy_data(cv_ImBGR.data, (int)(w * g_scale) * (int)(h * g_scale) * 3);

					if (g_show_results) SaveBGRImage(ImBGRScaled, "/DebugImages/FindText_" + iter_det + "_03_2_2_Im_BGRScaled4x4" + g_im_save_format, w * g_scale, h * g_scale);
				},
				[&ImNF, &ImSNFOrig, YB, YE, XB, w_orig, w, h, iter_det] {
					simple_buffer<u8> ImTMP(w * h);
					int y, i, j;

					for (y = YB, i = YB * w_orig + XB, j = 0; y <= YE; y++, i += w_orig, j += w)
					{
						ImTMP.copy_data(ImNF, j, i, w);
					}

					cv::Mat cv_ImGROrig, cv_ImGR;
					GreyscaleImageToMat(ImTMP, w, h, cv_ImGROrig);
					cv::resize(cv_ImGROrig, cv_ImGR, cv::Size(0, 0), g_scale, g_scale);
					BinaryMatToImage(cv_ImGR, w * g_scale, h * g_scale, ImSNFOrig, (u8)255);
					if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/DebugImages/FindText_" + iter_det + "_03_2_3_ImSNFOrig" + g_im_save_format, w * g_scale, h * g_scale);
				},
					[&FullImIL, &ImIL, YB, YE, XB, w_orig, w, h, iter_det] {
					if (FullImIL.m_size)
					{
						simple_buffer<u8> ImTMP(w * h);
						int y, i, j;

						for (y = YB, i = YB * w_orig + XB, j = 0; y <= YE; y++, i += w_orig, j += w)
						{
							ImTMP.copy_data(FullImIL, j, i, w);
						}

						if (g_show_results) SaveGreyscaleImage(ImTMP, "/DebugImages/FindText_" + iter_det + "_03_2_4_1_ImILOrig" + g_im_save_format, w, h);
						cv::Mat cv_ImGROrig, cv_ImGR;
						GreyscaleImageToMat(ImTMP, w, h, cv_ImGROrig);
						cv::resize(cv_ImGROrig, cv_ImGR, cv::Size(0, 0), g_scale, g_scale);
						BinaryMatToImage(cv_ImGR, w * g_scale, h * g_scale, ImIL, (u8)255);
						if (g_show_results) SaveGreyscaleImage(ImIL, "/DebugImages/FindText_" + iter_det + "_03_2_4_2_ImILScaled" + g_im_save_format, w * g_scale, h * g_scale);				
					}
					else
					{
						ImIL.set_size(0);
					}
				}
				);

			run_in_parallel(
				[&cv_ImBGR, &ImY, &ImU, &ImV, w, h, iter_det] {
					cv::Mat cv_Im, cv_ImSplit[3];
					cv::cvtColor(cv_ImBGR, cv_Im, cv::COLOR_BGR2YUV);
					cv::split(cv_Im, cv_ImSplit);
					GreyscaleMatToImage(cv_ImSplit[0], w * g_scale, h * g_scale, ImY);
					GreyscaleMatToImage(cv_ImSplit[1], w * g_scale, h * g_scale, ImU);
					GreyscaleMatToImage(cv_ImSplit[2], w * g_scale, h * g_scale, ImV);
					if (g_show_results) SaveGreyscaleImage(ImY, "/DebugImages/FindText_" + iter_det + "_03_4_1_ImY" + g_im_save_format, w * g_scale, h * g_scale);
					if (g_show_results) SaveGreyscaleImage(ImU, "/DebugImages/FindText_" + iter_det + "_03_4_2_ImU" + g_im_save_format, w * g_scale, h * g_scale);
					if (g_show_results) SaveGreyscaleImage(ImV, "/DebugImages/FindText_" + iter_det + "_03_4_3_ImV" + g_im_save_format, w * g_scale, h * g_scale);
				},
				[&cv_ImBGR, &ImLab, &ImL, &ImA, &ImB, w, h, iter_det] {
					cv::Mat cv_Im, cv_ImSplit[3];
					cv::cvtColor(cv_ImBGR, cv_Im, cv::COLOR_BGR2Lab);
					ImLab.copy_data(cv_Im.data, (int)(w* g_scale)* (int)(h* g_scale) * 3);
					cv::split(cv_Im, cv_ImSplit);
					GreyscaleMatToImage(cv_ImSplit[0], w * g_scale, h * g_scale, ImL);
					GreyscaleMatToImage(cv_ImSplit[1], w * g_scale, h * g_scale, ImA);
					GreyscaleMatToImage(cv_ImSplit[2], w * g_scale, h * g_scale, ImB);

					if (g_show_results) SaveGreyscaleImage(ImL, "/DebugImages/FindText_" + iter_det + "_03_5_1_ImL" + g_im_save_format, w * g_scale, h * g_scale);
					if (g_show_results) SaveGreyscaleImage(ImA, "/DebugImages/FindText_" + iter_det + "_03_5_2_ImA" + g_im_save_format, w * g_scale, h * g_scale);
					if (g_show_results) SaveGreyscaleImage(ImB, "/DebugImages/FindText_" + iter_det + "_03_5_3_ImB" + g_im_save_format, w * g_scale, h * g_scale);
				}
				);

			if (ImIL.m_size)
			{
				if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
				{
					if (g_show_results) SaveGreyscaleImage(ImIL, "/DebugImages/FindText_" + iter_det + "_03_6_1_ImIL" + g_im_save_format, w * g_scale, h * g_scale);
					FilterImageByPixelColorIsInRange(ImIL, &ImBGRScaled, &ImLab, w* g_scale, h* g_scale, iter_det + "_FindText_" + wxString::Format(wxT("%i"), __LINE__));
					if (g_show_results) SaveGreyscaleImage(ImIL, "/DebugImages/FindText_" + iter_det + "_03_6_2_ImILFByPixelColorIsInRange" + g_im_save_format, w * g_scale, h * g_scale);
				}
			}
		}

		w *= g_scale;
		h *= g_scale;

		real_im_x_center = (real_im_x_center - XB) * g_scale;

		yb = (orig_LLBk - YB) * g_scale;
		ye = (orig_LLEk - YB) * g_scale;
		xb = (orig_LLk - XB) * g_scale;
		xe = (orig_LRk - XB) * g_scale;

		{
			simple_buffer<u8> ImTMP(w * h);
			simple_buffer<int> LB(1, 0), LE(1, 0);
			LB[0] = 0;
			LE[0] = h - 1;
			GetImFF(ImSNF, ImTMP, ImY, ImU, ImV, LB, LE, 1, w, h, (W_orig * g_scale), (H_orig * g_scale), g_smthr);			
		}

		// note: ImSNF can be too broken due to color filtration, don't merge it with ImSNFOrig before extending internal figures

		ImSNFS = ImSNF;

		run_in_parallel(
			[&ImSNF, w, h, iter_det, W_orig, H_orig, xb, xe, yb, ye] {
				if (g_show_results) SaveGreyscaleImage(ImSNF, "/DebugImages/FindText_" + iter_det + "_06_1_1_ImSNF" + g_im_save_format, w, h);
				ExtendAndFilterImFF(ImSNF, w, h, (W_orig* g_scale), (H_orig* g_scale), iter_det + "_ImSNF", xb, xe, yb, ye);
				if (g_show_results) SaveGreyscaleImage(ImSNF, "/DebugImages/FindText_" + iter_det + "_06_1_2_ImSNFFByExtendAndFilterImFF" + g_im_save_format, w, h);
			},
			[&ImSNFOrig, w, h, iter_det, W_orig, H_orig, xb, xe, yb, ye] {
				if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/DebugImages/FindText_" + iter_det + "_06_2_1_ImSNFOrig" + g_im_save_format, w, h);
				ExtendAndFilterImFF(ImSNFOrig, w, h, (W_orig* g_scale), (H_orig* g_scale), iter_det + "_ImSNFOrig", xb, xe, yb, ye);
				if (g_show_results) SaveGreyscaleImage(ImSNFOrig, "/DebugImages/FindText_" + iter_det + "_06_2_2_ImSNFOrigFByExtendAndFilterImFF" + g_im_save_format, w, h);
			});

		IntersectTwoImages(ImSNF, ImSNFOrig, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSNF, "/DebugImages/FindText_" + iter_det + "_06_5_01_ImSNFIntImSNFOrig" + g_im_save_format, w, h);
	}

	if (ImIL.m_size)
	{
		IntersectTwoImages(ImSNF, ImIL, w, h);
		if (g_show_results) SaveGreyscaleImage(ImSNF, "/DebugImages/FindText_" + iter_det + "_06_7_01_SNFIntImIL" + g_im_save_format, w, h);
	}

	ImTHRMerge.copy_data(ImSNF, w * h);

	ClearImageByTextDistance(ImTHRMerge, w, h, (W_orig* g_scale), (H_orig* g_scale), real_im_x_center, (u8)255, iter_det);
	if (g_show_results) if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/DebugImages/FindText_" + iter_det + "_10_08_ImTHRMergeFByTextDistance" + g_im_save_format, w, h);

	yb = (orig_LLBk - YB) * g_scale;
	ye = (orig_LLEk - YB) * g_scale;
	xb = (orig_LLk - XB) * g_scale;
	if ((g_text_alignment == TextAlignment::Center) ||
		(g_text_alignment == TextAlignment::Left))
	{
		xe = std::min<int>(real_im_x_center, (orig_LRk - XB) * g_scale); //sometimes frome the right can be additional not centered text lines which intersect with two centered lines
	}
	else
	{
		xe = (orig_LRk - XB) * g_scale;
	}

	{
		simple_buffer<u8> ImMaskWithBorder(w * h), ImMaskWithBorderF(w * h);
		simple_buffer<u8> ImClusters2(w * h);
		simple_buffer<char> labels2(w * h);
		int clusterCount2 = 6;

		{
			simple_buffer<u8> Im1(w * h), Im2(w* h), ImTHRMergeF(ImTHRMerge), ImTHRMergeF2(w * h);

			vector<simple_buffer<u8>*> ImagesForCheck;			

			val = GetFirstFilteredClusters(ImBGRScaled, ImLab, ImIL, ImTHRMerge, ImSNF, ImMaskWithBorder, ImMaskWithBorderF, ImClusters2, labels2, clusterCount2, w, h, iter_det + "_call1");

			if (val == 0)
			{
				return;
			}

			if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/DebugImages/FindText_" + iter_det + "_11_02_02_ImTHRMergeOrig" + g_im_save_format, w, h);
			IntersectTwoImages(ImTHRMerge, ImMaskWithBorder, w, h);
			if (g_show_results) SaveGreyscaleImage(ImTHRMerge, "/DebugImages/FindText_" + iter_det + "_11_02_03_ImTHRMergeOrigIntImMaskWithBorder" + g_im_save_format, w, h);

			val = CheckOnSubPresence(ImTHRMerge, ImNE, ImTHRMergeF2, w, h, w_orig, h_orig, W_orig, H_orig, XB, YB, iter_det + "_call1");

			if (g_show_results) SaveGreyscaleImage(ImTHRMergeF2, "/DebugImages/FindText_" + iter_det + "_11_02_04_ImTHRMergeOrigFWithCheckOnSubPresence" + g_im_save_format, w, h);

			if (val == 0)
			{
				return;
			}
			else
			{
				ImTHRMerge.copy_data(ImTHRMergeF2, w * h);
			}

			if (!((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0)))
			{
				if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/DebugImages/FindText_" + iter_det + "_11_03_01_ImTHRMergeF" + g_im_save_format, w, h);
				IntersectTwoImages(ImTHRMergeF, ImTHRMerge, w, h);
				if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/DebugImages/FindText_" + iter_det + "_11_03_02_ImTHRMergeFIntImTHRMergeOrig" + g_im_save_format, w, h);
				IntersectTwoImages(ImTHRMergeF, ImMaskWithBorderF, w, h);
				if (g_show_results) SaveGreyscaleImage(ImTHRMergeF, "/DebugImages/FindText_" + iter_det + "_11_03_03_ImTHRMergeFIntImMaskWithBorderF" + g_im_save_format, w, h);

				int num_main_clusters = clusterCount2;
				simple_buffer<u8> ImTMP(ImSNFS);

				{
					cvMAT cv_im_gr;
					GreyscaleImageToMat(ImTMP, w, h, cv_im_gr);
					cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 2);
					BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);
					if (g_show_results)
					{
						SaveGreyscaleImage(ImSNFS, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImSNFS" + g_im_save_format, w, h);
						SaveGreyscaleImage(ImTMP, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImSNFSDilate" + g_im_save_format, w, h);
					}
				}

				IntersectTwoImages(ImTMP, ImTHRMergeF, w, h);
				if (g_show_results)
				{
					SaveGreyscaleImage(ImTHRMergeF, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImTHRMergeF" + g_im_save_format, w, h);
					SaveGreyscaleImage(ImTMP, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImTHRMergeFIntImSNFSDialte" + g_im_save_format, w, h);

					SaveGreyscaleImageWithLinesInfo(ImTMP, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImTHRMergeFIntImSNFS_WSI" + g_im_save_format, w, h,
						{ std::pair<int, int>{yb, yc}, std::pair<int, int>{ye, yc} },
						{ std::pair<int, int>{xb, yc}, std::pair<int, int>{xe, yc} });

					simple_buffer<u8> ImTMP2(w * h);
					GetClustersImage(ImTMP2, labels2, clusterCount2, w, h);
					SaveGreyscaleImageWithLinesInfo(ImTMP2, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters2_WSI" + g_im_save_format, w, h,
						{ std::pair<int, int>{yb, yc}, std::pair<int, int>{ye, yc} },
						{ std::pair<int, int>{xb, yc}, std::pair<int, int>{xe, yc} });
					IntersectTwoImages(ImTMP2, ImTMP, w, h);
					SaveGreyscaleImageWithLinesInfo(ImTMP2, "/DebugImages/FindText_" + iter_det + "_11_03_04" + "_line" + wxString::Format(wxT("%i"), __LINE__) + "_ImClusters2InImTHRMergeFIntImSNFS_WSI" + g_im_save_format, w, h,
						{ std::pair<int, int>{yb, yc}, std::pair<int, int>{ye, yc} },
						{ std::pair<int, int>{xb, yc}, std::pair<int, int>{xe, yc} });
				}

				val = GetMainTwoClustersByAverageLuminanceDifference(Im1, Im2, num_main_clusters, ImTMP, ImY, labels2, clusterCount2, w, h, iter_det + "_initial_clusters2", xb, xe, yb, ye);

				if (val == 0)
				{
					return;
				}

				Im2.copy_data(Im1, w * h);

				run_in_parallel(
					[&Im1, &ImTHRMerge, w, h, W_orig, H_orig, real_im_x_center, iter_det] {
						if (g_show_results) SaveGreyscaleImage(Im1, "/DebugImages/FindText_" + iter_det + "_11_04_01_01_ImMainCluster1" + g_im_save_format, w, h);
						ExtendByInsideFigures(Im1, w, h, (u8)255);
						if (g_show_results) SaveGreyscaleImage(Im1, "/DebugImages/FindText_" + iter_det + "_11_04_01_02_ImMainCluster1ExtInternalFigures" + g_im_save_format, w, h);

						IntersectTwoImages(Im1, ImTHRMerge, w, h);
						if (g_show_results) SaveGreyscaleImage(Im1, "/DebugImages/FindText_" + iter_det + "_11_04_02_ImMainCluster1IntImTHRMergeOrig" + g_im_save_format, w, h);

						ClearImageByTextDistance(Im1, w, h, (W_orig * g_scale), (H_orig * g_scale), real_im_x_center, (u8)255, iter_det);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im1, "/DebugImages/FindText_" + iter_det + "_11_04_03_ImMainCluster1FByTextDistance" + g_im_save_format, w, h);

						ClearImageOpt3(Im1, w, h, real_im_x_center, 255);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im1, "/DebugImages/FindText_" + iter_det + "_11_04_04_ImMainCluster1FByOpt3" + g_im_save_format, w, h);
					},
					[&Im2, &ImTHRMerge, w, h, W_orig, H_orig, real_im_x_center, iter_det] {
						// note: needed for: 0_01_05_295__0_01_05_594.jpeg (in other case line with two symbols can be too thin)		
						// also needed for: 0_14_28_040__0_14_30_399.jpeg in other case first cluster is too thin
						{
							cv::ocl::setUseOpenCL(g_use_ocl);

							cvMAT cv_im_gr;
							GreyscaleImageToMat(Im2, w, h, cv_im_gr);
							cv::Mat kernel = cv::Mat::ones(7, 7, CV_8U);
							cv::morphologyEx(cv_im_gr, cv_im_gr, cv::MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
							cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 2);
							BinaryMatToImage(cv_im_gr, w, h, Im2, (u8)255);
							if (g_show_results) SaveGreyscaleImage(Im2, "/DebugImages/FindText_" + iter_det + "_11_05_ImMainCluster1CloseDilate" + g_im_save_format, w, h);
						}
						ExtendByInsideFigures(Im2, w, h, (u8)255);
						if (g_show_results) SaveGreyscaleImage(Im2, "/DebugImages/FindText_" + iter_det + "_11_06_ImMainCluster1ExtInternalFigures" + g_im_save_format, w, h);
						IntersectTwoImages(Im2, ImTHRMerge, w, h);
						if (g_show_results) SaveGreyscaleImage(Im2, "/DebugImages/FindText_" + iter_det + "_11_07_ImMainCluster1IntImTHRMergeOrig" + g_im_save_format, w, h);

						ClearImageByTextDistance(Im2, w, h, (W_orig * g_scale), (H_orig * g_scale), real_im_x_center, (u8)255, iter_det);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im2, "/DebugImages/FindText_" + iter_det + "_11_08_ImMainCluster1FByTextDistance" + g_im_save_format, w, h);

						ClearImageOpt3(Im2, w, h, real_im_x_center, 255);
						if (g_show_results) if (g_show_results) SaveGreyscaleImage(Im2, "/DebugImages/FindText_" + iter_det + "_11_09_ImMainCluster1FByOpt3" + g_im_save_format, w, h);
					}
					);

				ImagesForCheck.push_back(&Im2);
				ImagesForCheck.push_back(&Im1);
			}

			ImagesForCheck.push_back(&ImTHRMerge);

			for (int i_im = 0; i_im < ImagesForCheck.size(); i_im++)
			{
				simple_buffer<u8>& ImThrRef = *(ImagesForCheck[i_im]);

				if (g_show_results) SaveGreyscaleImage(ImThrRef, "/DebugImages/FindText_" + iter_det + "_12_" + std::to_string(i_im) + "_1_ImTHR" + std::to_string(i_im) + g_im_save_format, w, h);

				// Checking ImTHRMain on presence more then one line (if yes spliting on two and repeat)
				//-------------------------------		
				val = GetSubParams(ImThrRef, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);

				if (val == 1)
				{
					if (g_show_results) SaveGreyscaleImageWithSubParams(ImThrRef, "/DebugImages/FindText_" + iter_det + "_12_" + std::to_string(i_im) + "_2_1_ImTHR" + std::to_string(i_im) + "_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				}

				if ((val == 1) && (std::max<int>((LLE[k] - LLB[k] + 1) * g_scale, le - lb + 1) > 1.5 * (double)LH))
				{
					bln = 0;

					simple_buffer<int> lw(h, 0);
					int max_txt_w, max_txt_y, min_y1, max_y1, min_y2, max_y2, new_txt_y = 0, new_txt_w, max_txt2_w, max_txt2_y;

					simple_buffer<u8> ImTHRF(ImThrRef);
					ClearImageFromMainSymbols(ImTHRF, w, h, W_orig* g_scale, H_orig* g_scale, LH, LMAXY, 255, iter_det);
					if (g_show_results) SaveGreyscaleImage(ImTHRF, "/DebugImages/FindText_" + iter_det + "_12_" + std::to_string(i_im) + "_2_2_ImTHR" + std::to_string(i_im) + "FFromMainSymbols" + g_im_save_format, w, h);

					for (y = yb; y <= ye; y++)
					{
						for (x = xb; x < xe; x++)
						{
							if ((y >= LMAXY - LH + 1) && (y <= LMAXY))
							{
								if (ImThrRef[y * w + x] != 0)
								{
									lw[y]++;
								}
							}
							else
							{
								if (ImTHRF[y * w + x] != 0)
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
						return;
					}

					auto check_split = [&res, &ImThrRef, &LL, &LR, &LLB, &LLE, &N, 
										orig_LLk, orig_LRk, orig_LLBk, orig_LLEk, YB, k, w, h, xb, xe, yb, ye,
										mpn, min_h, real_im_x_center, iter_det, i_im](int new_txt_y)
					{
						bool need_to_split = false;

						custom_assert(g_scale > 0, "FindText: g_scale > 0");
						if ((new_txt_y > 0) &&
							(((YB + (new_txt_y / g_scale)) - orig_LLBk + 1) >= g_scale) &&
							((orig_LLEk - (YB + (new_txt_y / g_scale) + 1) + 1) >= g_scale)
							)
						{
							int cnt1 = 0;
							int cnt2 = 0;
							for (int y = yb; y <= ye; y++)
							{
								for (int x = xb; x < xe; x++)
								{
									if (ImThrRef[y * w + x] != 0)
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
								int val1 = GetSubParams(ImThrRef, w, h, 255, LH1, LMAXY1, lb1, le1, min_h * g_scale, real_im_x_center, yb, new_txt_y, iter_det);
								int val2 = GetSubParams(ImThrRef, w, h, 255, LH2, LMAXY2, lb2, le2, min_h * g_scale, real_im_x_center, new_txt_y + 1, ye, iter_det);

								int val = 0;
								if ((val1 == 1) && (val2 == 1))
								{
									if (std::min<int>(LH1, LH2) >= std::max<int>(LH1, LH2) / 2)
									{
										val = 1;
									}
								}

								if (val == 1)
								{
									for (int i = N; i > k + 1; i--)
									{
										LL[i] = LL[i - 1];
										LR[i] = LR[i - 1];
										LLB[i] = LLB[i - 1];
										LLE[i] = LLE[i - 1];
									}

									LL[k + 1] = orig_LLk;
									LR[k + 1] = orig_LRk;
									custom_assert(g_scale > 0, "FindText: g_scale > 0");
									LLB[k + 1] = YB + (new_txt_y / g_scale) + 1;
									LLE[k + 1] = orig_LLEk;

									LL[k] = orig_LLk;
									LR[k] = orig_LRk;
									LLB[k] = orig_LLBk;
									custom_assert(g_scale > 0, "FindText: g_scale > 0");
									LLE[k] = YB + (new_txt_y / g_scale);

									N++;

									res.m_LL = LL; // setting m_LL which leads to split to two new lines if its size > 0
									res.m_LR = LR;
									res.m_LLB = LLB;
									res.m_LLE = LLE;
									res.m_N = N;
									res.m_k = k;
									res.m_iter_det = iter_det;

									if (g_show_results) SaveBinaryImageWithLinesInfo(ImThrRef, "/DebugImages/FindText_" + iter_det + "_12_" + std::to_string(i_im) + "_3_ImTHR" + std::to_string(i_im) + "_WithSplitInfo" + g_im_save_format, (LLB[k] - YB) * g_scale, (LLE[k] - YB) * g_scale, (LLB[k + 1] - YB) * g_scale, (LLE[k + 1] - YB) * g_scale, w, h);

									need_to_split = true;
								}
							}
						}

						return need_to_split;
					};

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
									custom_assert(max_txt_w > 0, "FindText: max_txt_w > 0");
									if ((double)lw[y2] <= 0.3 * (double)max_txt_w)
									{
										custom_assert(lw[y1] > 0, "FindText: lw[y1] > 0");
										if ((double)lw[y2] <= 0.3 * (double)lw[y1])
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
								
								if (new_txt_y > LMAXY - LH)
								{
									new_txt_y = std::max<int>(LMAXY - LH, 0);
								}

								y2 = new_txt_y;
								while ((lw[new_txt_y] == 0) && (new_txt_y > yb)) new_txt_y--;
								if (new_txt_y > yb) new_txt_y = (y2 + new_txt_y) / 2;
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

					if (check_split(new_txt_y))
					{
						return;
					}

					min_y1 = std::min<int>(LMAXY + LH * 0.2, h - 1);
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
									custom_assert(max_txt_w > 0, "FindText: max_txt_w > 0");
									if ((double)lw[y2] <= 0.3 * (double)max_txt_w)
									{
										custom_assert(lw[y1] > 0, "FindText: lw[y1] > 0");
										if ((double)lw[y2] <= 0.3 * (double)lw[y1])
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

								if (new_txt_y < LMAXY)
								{
									new_txt_y = LMAXY;
								}

								y2 = new_txt_y;
								while ((lw[new_txt_y] == 0) && (new_txt_y < ye)) new_txt_y++;
								if (new_txt_y < ye) new_txt_y = (y2 + new_txt_y) / 2;
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

					if (check_split(new_txt_y))
					{
						return;
					}
				}
				//----------------------------	
			}
		}

		{
			simple_buffer<u8> ImMainCluster(w * h);

			yb = (orig_LLBk - YB) * g_scale;
			ye = (orig_LLEk - YB) * g_scale;
			xb = (orig_LLk - XB) * g_scale;
			xe = (orig_LRk - XB) * g_scale;

			val = GetMainClusterImage(ImBGRScaled, ImLab, ImTHRMerge, ImSNFS, ImIL, ImMainCluster, ImL, ImA, ImB, ImMaskWithBorder, ImMaskWithBorderF, ImClusters2, labels2, clusterCount2, w, h, W_orig * g_scale, H_orig * g_scale, iter_det, real_im_x_center, min_h * g_scale, xb, xe, yb, ye);
			if (val == 0)
			{
				return;
			}

			if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_13_02_ImMainCluster" + g_im_save_format, w, h);

			val = GetSubParams(ImMainCluster, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);
			if (val == 0)
			{
				val = GetSubParams(ImTHRMerge, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);
				if (val == 0)
				{
					return;
				}
			}

			ClearMainClusterImage(ImMainCluster, ImSNFS, ImIL, w, h, W_orig* g_scale, H_orig* g_scale, LH, iter_det + "_call1");
			if (g_show_results)
			{				
				SaveGreyscaleImage(ImSNFS, "/DebugImages/FindText_" + iter_det + "_13_03_ImSNFS" + g_im_save_format, w, h);
				if (ImIL.m_size) SaveGreyscaleImage(ImIL, "/DebugImages/FindText_" + iter_det + "_13_04_ImIL" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_13_05_ImMainClusterFByImSNFSAndImIL" + g_im_save_format, w, h);				
			}

			if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
			{
				IntersectTwoImages(ImMainCluster, ImMaskWithBorder, w, h);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_13_06_ImMainClusterIntImMaskWithBorder" + g_im_save_format, w, h);
			}

			{
				simple_buffer<u8> ImMainMASKF(w * h);

				if (g_clear_txt_images_by_main_color || g_extend_by_grey_color)
				{
					simple_buffer<u8> ImMainMASK(w * h), ImMainClusterF(w * h);

					ImMainMASK.copy_data(ImSNF, w * h);
					IntersectTwoImages(ImMainMASK, ImMainCluster, w, h);
					
					if (g_show_results) SaveGreyscaleImage(ImMainMASK, "/DebugImages/FindText_" + iter_det + "_14_01_ImMainMASKByImSNF" + g_im_save_format, w, h);
					ClearImageOpt4(ImMainMASK, w, h, W_orig* g_scale, H_orig* g_scale, LH, LMAXY, real_im_x_center, (u8)255);
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainMASK, "/DebugImages/FindText_" + iter_det + "_14_02_ImMainMASKFByClearImageOpt4" + g_im_save_format, w, h,
						{ std::pair<int, int>{yb, yc}, std::pair<int, int>{ye, yc} },
						{ std::pair<int, int>{xb, yc}, std::pair<int, int>{xe, yc} });
					if (g_show_results) SaveGreyscaleImageWithLinesInfo(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_14_03_ImMainCluster" + g_im_save_format, w, h,
						{ std::pair<int, int>{yb, yc}, std::pair<int, int>{ye, yc} },
						{ std::pair<int, int>{xb, yc}, std::pair<int, int>{xe, yc} });

					ImMainClusterF.copy_data(ImMainCluster, w * h);
					GetMainColorImage(ImMainMASKF, &ImMainClusterF, ImMainMASK, ImL, ImA, ImB, w, h, iter_det + "_call2", xb, xe, yb, ye);

					if (g_show_results) SaveGreyscaleImage(ImMainClusterF, "/DebugImages/FindText_" + iter_det + "_14_04_ImMainClusterFByColor" + g_im_save_format, w, h);

					if (!g_extend_by_grey_color)
					{
						ClearImageByMask(ImMainCluster, ImMainClusterF, w, h, 255, 2.0 / 3.0);
						if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_15_ImMainClusterClearByMASKImMainClusterFByColor" + g_im_save_format, w, h);
					}
					else
					{
						ImFF.copy_data(ImMainCluster, w* h);
						ImMainCluster.copy_data(ImMainClusterF, w* h);
					}
				}

				val = GetSubParams(ImMainCluster, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);
				if (val == 0)
				{
					if (g_extend_by_grey_color)
					{
						val = GetSubParams(ImFF, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);
					}

					if (val == 0)
					{
						val = GetSubParams(ImMainMASKF, w, h, 255, LH, LMAXY, lb, le, min_h * g_scale, real_im_x_center, yb, ye, iter_det);
					}
				}
			}

			if (val != 0)
			{
				if (g_show_results) SaveGreyscaleImageWithSubParams(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_16_ImMainCluster_WSP" + g_im_save_format, lb, le, LH, LMAXY, real_im_x_center, w, h);
				ClearImageOpt5(ImMainCluster, w, h, LH, LMAXY, real_im_x_center, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_17_ImMainClusterFOpt5" + g_im_save_format, w, h);
			}

			if ((val != 0) && g_extend_by_grey_color)
			{
				if (g_show_results) SaveGreyscaleImage(ImFF, "/DebugImages/FindText_" + iter_det + "_18_ImMainClusterOrig" + g_im_save_format, w, h);
				CombineTwoImages(ImMainCluster, ImFF, w, h, (u8)255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_19_ImMainClusterRestoredByImMainClusterOrig" + g_im_save_format, w, h);
				ClearImageOpt5(ImMainCluster, w, h, LH, LMAXY, real_im_x_center, 255);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_20_ImMainClusterFOpt5" + g_im_save_format, w, h);
			}

			if (g_clear_image_logical)
			{
				ClearImageLogical(ImMainCluster, w, h, W_orig * g_scale, H_orig * g_scale, real_im_x_center, yb, ye, (u8)255, iter_det);
				if (g_show_results) SaveGreyscaleImage(ImMainCluster, "/DebugImages/FindText_" + iter_det + "_21_ImMainClusterClearImageLogical" + g_im_save_format, w, h);
			}

			ImFF.copy_data(ImMainCluster, w * h);
		}
	}

	GetTextLineParameters(ImFF, ImY, ImU, ImV, w, h, LH, LMAXY, DXB, DXE, DYB, DYE, mY, mI, mQ, 255);

	ww = w_orig * g_scale;
	int min_y = std::max<int>(DYB - (LH / 2), 0);
	int max_y = std::min<int>(DYE + (LH / 2), h - 1);

	bln = 0;
	val = 0;
	for (y = 0, i = 0; y < h; y++)
	{
		for (x = 0; x < w; x++, i++)
		{
			if (ImFF[i] != 0)
			{
				val = y;
				bln = 1;
				break;
			}
		}

		if (bln == 1)
		{
			break;
		}
	}
	min_y = std::min<int>(min_y, val);

	bln = 0;
	val = h - 1;
	for (y = h-1; y >= 0; y--)
	{
		for (x = 0; x < w; x++)
		{
			i = y * w + x;

			if (ImFF[i] != 0)
			{
				val = y;
				bln = 1;
				break;
			}
		}

		if (bln == 1)
		{
			break;
		}
	}
	max_y = std::max<int>(max_y, val);

	min_y -= (min_y % g_scale);
	hh = max_y - min_y + 1;

	simple_buffer<u8> ImSubForSave(ww* hh, (u8)255);

	cnt = 0;
	for (y = min_y; y <= max_y; y++)
	{
		for (x = 0; x < w; x++)
		{
			i = y * w + x;

			if (ImFF[i] != 0)
			{
				cnt++;
				ImSubForSave[(y - min_y) * ww + (int)(XB * g_scale) + x] = 0;
			}
		}
	}

	if (cnt > 0)
	{
		res.m_YB = YB + (min_y / g_scale);
		custom_assert(g_scale > 0, "FindText: g_scale > 0");
		res.m_im_h = hh / g_scale;

		simple_buffer<u8> ImFFD(w_orig*res.m_im_h, (u8)0), ImSF(w_orig*res.m_im_h), ImTF(w_orig*res.m_im_h);

		{
			cv::Mat cv_ImDilate(res.m_im_h, w_orig, CV_8UC1);
			GreyscaleImageToMat(ImSubForSave, ww, hh, res.m_cv_ImClearedTextScaled);
			custom_assert(g_scale > 0, "FindText: g_scale > 0");
			cv::resize(res.m_cv_ImClearedTextScaled, res.m_cv_ImClearedText, cv::Size(0, 0), 1.0/g_scale, 1.0/g_scale);

			for (i = 0; i < res.m_im_h * w_orig; i++)
			{
				if (res.m_cv_ImClearedText.data[i] <= 245)
				{
					res.m_cv_ImClearedText.data[i] = 0;
				}				
			}

			for (i = 0; i < res.m_im_h* w_orig; i++)
			{
				if (res.m_cv_ImClearedText.data[i] == 0)
				{
					cv_ImDilate.data[i] = 255;
				}
				else
				{
					cv_ImDilate.data[i] = 0;
				}
			}
			//cv::dilate(cv_ImDilate, cv_ImDilate, cv::Mat(), cv::Point(-1, -1), 2);

			for (i = 0; i < res.m_im_h* w_orig; i++)
			{
				if (cv_ImDilate.data[i] != 0)
				{
					ImFFD[i] = 255;
				}
			}

			if (g_show_results) SaveGreyscaleImage(ImFFD, "/DebugImages/FindText_" + iter_det + "_78_01_ImTXTDilate" + g_im_save_format, w_orig, res.m_im_h);
		}
		
		ImSF.copy_data(ImFFD, w_orig* res.m_im_h);
		
		simple_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = res.m_im_h - 1;
		simple_buffer<u8> ImNE_sub_buffer(ImNE, res.m_YB* w_orig);
		res.m_res = FilterTransformedImage(ImFFD, ImSF, ImTF, ImNE_sub_buffer, LB, LE, 1, w_orig, res.m_im_h, W_orig, H_orig, 0, w_orig - 1, iter_det);

		if (g_show_results) SaveGreyscaleImage(ImTF, "/DebugImages/FindText_" + iter_det + "_78_02_ImTXTF" + g_im_save_format, w_orig, res.m_im_h);

		if (res.m_res == 1)
		{
			custom_assert(g_scale > 0, "FindText: g_scale > 0");
			res.m_LH = LH / g_scale;
			res.m_LY = YB + LMAXY / g_scale;
			res.m_LXB = XB + DXB / g_scale;
			res.m_LXE = XB + DXE / g_scale;
			res.m_LYB = YB + DYB / g_scale;
			res.m_LYE = YB + DYE / g_scale;
			res.m_mY = mY;
			res.m_mI = mI;
			res.m_mQ = mQ;

			wxString FullName = SaveDir + SaveName + wxT("_") + FormatImInfoAddData(W_orig, H_orig, xmin_orig, ymin_orig + res.m_YB, w_orig, res.m_im_h, 1) + g_im_save_format;
			res.m_ImageName = FullName;

			if (g_save_each_substring_separately)
			{
				if (g_save_scaled_images)
				{
					SaveGreyscaleImage(ImSubForSave, FullName, ww, hh);
				}
				else
				{
					GreyscaleMatToImage(res.m_cv_ImClearedText, w_orig, res.m_im_h, ImSubForSave);
					SaveGreyscaleImage(ImSubForSave, FullName, w_orig, res.m_im_h);
				}
			}
		}
	}
}

inline shared_custom_task TaskFindText(FindTextRes &res, simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImF, simple_buffer<u8> &ImNF, simple_buffer<u8> &ImNE, simple_buffer<u8> &ImIL, simple_buffer<u8> &FullImY, wxString SaveDir, wxString SaveName, wxString iter_det, int N, int k, simple_buffer<int> &LL, simple_buffer<int> &LR, simple_buffer<int> &LLB, simple_buffer<int> &LLE, int w_orig, int h_orig, int W_orig, int H_orig, int xmin_orig, int ymin_orig)
{	
	return shared_custom_task([&res, &ImBGR, &ImF, &ImNF, &ImNE, &ImIL, &FullImY, SaveDir, SaveName, iter_det, N, k, LL, LR, LLB, LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig]	{
			FindText(res, ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, iter_det, N, k, LL, LR, LLB, LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig);
		}
	);
}

int FindTextLines(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImClearedTextScaled, simple_buffer<u8>& ImF, simple_buffer<u8>& ImNF, simple_buffer<u8>& ImNE, simple_buffer<u8>& ImIL, wxString SaveDir, wxString SaveName, const int w_orig, const int h_orig, const int W_orig, const int H_orig, const int xmin_orig, const int ymin_orig)
{
	simple_buffer<int> LL(h_orig, 0), LR(h_orig, 0), LLB(h_orig, 0), LLE(h_orig, 0), LW(h_orig, 0);
	simple_buffer<int> GRStr(STR_SIZE, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	simple_buffer<u8> FullImY(w_orig * h_orig);
	simple_buffer<u8> ImClearedText;

	int i, j, k, l, r, x, y, ib, bln, N, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, ww, hh, cnt;
	int XB, XE, YB = h_orig, YE = -1, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, lb, le, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, NN, ys1, ys2, ys3, val_min, val_max;	
	int j1, j2, j3, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max;
	int mY, mI, mQ;
	int LH, LMAXY;
	double mthr;
	wxString FullName, Str;
	char str[30];
	int res = 0;
	int iter = 0;	
	int min_h = g_min_h * H_orig;
	int ln = 0;
	
	mthr = 0.3;
	segh = g_segh;

	ImClearedTextScaled.set_values(255);

	if (!g_save_each_substring_separately && !g_save_scaled_images)
	{
		ImClearedText.set_size(w_orig * h_orig);
		ImClearedText.set_values(255);
	}	

	if (g_show_results) SaveBGRImage(ImBGR, "/DebugImages/FindTextLines_01_1_ImBGR" + g_im_save_format, w_orig, h_orig);
	if (g_show_results) SaveGreyscaleImage(ImNF, "/DebugImages/FindTextLines_01_2_ImNF" + g_im_save_format, w_orig, h_orig);
	if (g_show_results) SaveGreyscaleImage(ImF, "/DebugImages/FindTextLines_01_3_ImF" + g_im_save_format, w_orig, h_orig);
	if (g_show_results && ImIL.m_size) SaveGreyscaleImage(ImIL, "/DebugImages/FindTextLines_01_4_FullImIL" + g_im_save_format, w_orig, h_orig);

	{
		cv::Mat cv_FullImBGR, cv_FullImY;
		BGRImageToMat(ImBGR, w_orig, h_orig, cv_FullImBGR);
		cv::cvtColor(cv_FullImBGR, cv_FullImY, cv::COLOR_BGR2GRAY);
		GreyscaleMatToImage(cv_FullImY, w_orig, h_orig, FullImY);
		if (g_show_results) SaveGreyscaleImage(FullImY, "/DebugImages/FindTextLines_01_5_FullImY" + g_im_save_format, w_orig, h_orig);
	}	

	// Getting info about text lines position in ImF
	// -----------------------------------------------------
	// LW[y] contain number of text pixels in 'y' of ImF image
	N = 0; // number of lines
	LLB[0] = -1; // line 'i' begin by 'y'
	LLE[0] = -1; // line 'i' end by 'y'
	LL[0] = w_orig-1; // line 'i' begin by 'x'
	LR[0] = 0; // line 'i' end by 'x'

	for (y=0, ib=0; y<h_orig; y++, ib+=w_orig)
	{
		bln = 0;
		cnt = 0;
		for(x=0; x<w_orig; x++)
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
			LL[N] = w_orig-1;
			LR[N] = 0;
		}

		if (bln == 1)
		{
			if (LL[N]>l) LL[N] = l;
			if (LR[N]<r) LR[N] = r;
		}
		
		LW[y] = cnt;
	}
	if (LLE[N] == h_orig-1) N++;
	// -----------------------------------------------------

	k = 0;
	while (k < N)
	{
		if (k < N - 1)
		{
			if (LLB[k + 1] - LLE[k] <= g_segh)
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
				continue;
			}
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

	k = 0;
	while (k < N)
	{
		if (LLE[k] - LLB[k] + 1 > H_orig / 2)
		{
			N++;

			for (i = N-1; i > k; i--)
			{
				LL[i] = LL[i - 1];
				LR[i] = LR[i - 1];
				LLB[i] = LLB[i - 1];
				LLE[i] = LLE[i - 1];
			}

			LLE[k] = (H_orig / 2) - 1;
			LLB[k + 1] = LLE[k] + 1;
		}

		k++;
	}

	if (g_show_results)	SaveBGRImageWithLinesInfo(ImBGR, "/DebugImages/FindTextLines_01_6_ImBGRWithLinesInfo" + g_im_save_format, LLB, LLE, N, w_orig, h_orig);

#ifndef WINX86
	vector<shared_custom_task> FindTextTasks;
	vector<FindTextRes*> FindTextTasksRes(N);

	for (k = 0; k < N; k++)
	{
		FindTextTasksRes[k] = new FindTextRes();
		FindTextTasks.emplace_back(TaskFindText(*(FindTextTasksRes[k]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, wxString(wxT("l")) + std::to_string(k + 1), N, k, LL, LR, LLB, LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig));
	}

	k = 0;
	while (k < FindTextTasks.size())
	{
		FindTextTasks[k].wait();
		FindTextRes* p_ft_res = FindTextTasksRes[k];

		if (p_ft_res->m_LL.m_size > 0) // ned to split text on 2 parts
		{
			FindTextTasksRes.push_back(new FindTextRes());
			FindTextTasks.emplace_back(TaskFindText(*(FindTextTasksRes[FindTextTasksRes.size() - 1]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, p_ft_res->m_iter_det + "_sl1", p_ft_res->m_N, p_ft_res->m_k, p_ft_res->m_LL, p_ft_res->m_LR, p_ft_res->m_LLB, p_ft_res->m_LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig));
			FindTextTasksRes.push_back(new FindTextRes());
			FindTextTasks.emplace_back(TaskFindText(*(FindTextTasksRes[FindTextTasksRes.size() - 1]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, p_ft_res->m_iter_det + "_sl2", p_ft_res->m_N, p_ft_res->m_k + 1, p_ft_res->m_LL, p_ft_res->m_LR, p_ft_res->m_LLB, p_ft_res->m_LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig));
		}
		else
		{
			if (p_ft_res->m_res == 1)
			{
				ln++;

				if (p_ft_res->m_YB < YB)
				{
					YB = p_ft_res->m_YB;
				}

				if ((p_ft_res->m_YB + p_ft_res->m_im_h - 1) > YE)
				{
					YE = p_ft_res->m_YB + p_ft_res->m_im_h - 1;
				}

				if (!g_save_each_substring_separately && !g_save_scaled_images)
				{
					for (y = 0, i = 0; y < p_ft_res->m_im_h; y++)
					{
						for (x = 0; x < w_orig; x++, i++)
						{
							if (p_ft_res->m_cv_ImClearedText.data[i] == 0)
							{
								ImClearedText[(p_ft_res->m_YB * w_orig) + i] = 0;
							}
						}
					}
				}

				for (y = 0, i = 0; y < p_ft_res->m_im_h * g_scale; y++)
				{
					for (x = 0; x < w_orig * g_scale; x++, i++)
					{
						if (p_ft_res->m_cv_ImClearedTextScaled.data[i] == 0)
						{
							ImClearedTextScaled[(p_ft_res->m_YB * g_scale * w_orig * g_scale) + i] = 0;
						}
					}
				}

				res = 1;
			}
		}

		delete p_ft_res;
		k++;
	}
#else
	vector<FindTextRes*> FindTextTasks(N);

	for (k = 0; k < N; k++)
	{
		FindTextTasks[k] = new FindTextRes();
		FindText(*(FindTextTasks[k]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, "l" + std::to_string(k + 1), N, k, LL, LR, LLB, LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig);
	}

	k = 0;
	while (k < FindTextTasks.size())
	{
		FindTextRes *p_ft_res = FindTextTasks[k];

		if (p_ft_res->m_LL.m_size > 0) // ned to split text on 2 parts
		{
			FindTextTasks.push_back(new FindTextRes());
			FindText(*(FindTextTasks[FindTextTasks.size() - 1]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, p_ft_res->m_iter_det + "_sl1", p_ft_res->m_N, p_ft_res->m_k, p_ft_res->m_LL, p_ft_res->m_LR, p_ft_res->m_LLB, p_ft_res->m_LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig);
			FindTextTasks.push_back(new FindTextRes());
			FindText(*(FindTextTasks[FindTextTasks.size() - 1]), ImBGR, ImF, ImNF, ImNE, ImIL, FullImY, SaveDir, SaveName, p_ft_res->m_iter_det + "_sl2", p_ft_res->m_N, p_ft_res->m_k + 1, p_ft_res->m_LL, p_ft_res->m_LR, p_ft_res->m_LLB, p_ft_res->m_LLE, w_orig, h_orig, W_orig, H_orig, xmin_orig, ymin_orig);
		}
		else
		{
			if (p_ft_res->m_res == 1)
			{
				ln++;

				if (p_ft_res->m_YB < YB)
				{
					YB = p_ft_res->m_YB;
				}

				if ((p_ft_res->m_YB + p_ft_res->m_im_h - 1) > YE)
				{
					YE = p_ft_res->m_YB + p_ft_res->m_im_h - 1;
				}

				if (!g_save_each_substring_separately && !g_save_scaled_images)
				{
					for (y = 0, i = 0; y < p_ft_res->m_im_h; y++)
					{
						for (x = 0; x < w_orig; x++, i++)
						{
							if (p_ft_res->m_cv_ImClearedText.data[i] == 0)
							{
								ImClearedText[(p_ft_res->m_YB * w_orig) + i] = 0;
							}
						}
					}
				}

				for (y = 0, i = 0; y < p_ft_res->m_im_h * g_scale; y++)
				{
					for (x = 0; x < w_orig * g_scale; x++, i++)
					{
						if (p_ft_res->m_cv_ImClearedTextScaled.data[i] == 0)
						{
							ImClearedTextScaled[(p_ft_res->m_YB * g_scale * w_orig * g_scale) + i] = 0;
						}
					}
				}

				res = 1;
			}
		}

		delete p_ft_res;
		k++;
	}
#endif

	if (res == 1)
	{
		if (!g_save_each_substring_separately)
		{
			FullName = SaveDir + SaveName + wxT("_") + FormatImInfoAddData(W_orig, H_orig, xmin_orig, ymin_orig + YB, w_orig, YE - YB + 1, ln) + g_im_save_format;

			if (g_save_scaled_images)
			{
				SaveBinaryImage(simple_buffer<u8>(ImClearedTextScaled, YB * g_scale * w_orig * g_scale), FullName, w_orig * g_scale, (YE - YB + 1) * g_scale);
			}
			else
			{
				SaveBinaryImage(simple_buffer<u8>(ImClearedText, YB * w_orig), FullName, w_orig, (YE - YB + 1));
			}
		}
	}

	return res;
}

//Extending ImF with data from ImNF
//adding all figures from ImNF which intersect with minY-maxY figures position in ImF
void ExtendImFWithDataFromImNF(simple_buffer<u8>& ImF, simple_buffer<u8>& ImNF, int w, int h)
{
	// Getting info about text position in ImF
	int x, y, i, ib, k, bln, l, r, N;
	simple_buffer<int> LL(h, 0), LR(h, 0), LLB(h, 0), LLE(h, 0);

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

int ClearImageOptimal(simple_buffer<u8> &Im, int w, int h, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, N /*num of closed figures*/, NNY, min_h;
	int val=0, val1, val2, ddy1, ddy2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();	

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			(g_remove_wide_symbols && (pFigure->width() >= h * 2))
			)
		{	
			for(l=0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageOptimal2(simple_buffer<u8> &Im, int w, int h, int min_x, int max_x, int min_y, int max_y, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, N /*num of closed figures*/, NNY, min_h;
	int val = 0, val1, val2, ddy1, ddy2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			(g_remove_wide_symbols && (pFigure->width() >= h * 2)) ||
			((pFigure->width() < 3) || (pFigure->height() < 3)) //|| bad for multiple points with main color like: ......
			//( (pFigure->m_PointsArray.m_size >= 0.95*((M_PI*pFigure->width()*pFigure->height()) / 4.0)) && //is like ellipse or rectangle on 95% 
//			  (std::max<int>(pFigure->height(), pFigure->width()) <= 2 * std::min<int>(pFigure->height(), pFigure->width())) )
			)
		{
			/*if((pFigure->m_PointsArray.m_size >= 0.95*((M_PI*pFigure->width()*pFigure->height()) / 4.0)) && //is like ellipse or rectangle on 95% 
				(std::max<int>(pFigure->height(), pFigure->width()) <= 2 * std::min<int>(pFigure->height(), pFigure->width())))
			{
				i = i;
			}*/

			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

void CombineFiguresRelatedToEachOther(simple_buffer<CMyClosedFigure*> &ppFigures, int &N, int min_h, wxString iter_det)
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
					 (pFigureJ->width() < 5*min_h)
					)
				{
					int val1 = (pFigureI->m_maxX + pFigureI->m_minX) - (pFigureJ->m_maxX + pFigureJ->m_minX);
					if (val1 < 0) val1 = -val1;
					val1 = ((pFigureI->m_maxX - pFigureI->m_minX) + (pFigureJ->m_maxX - pFigureJ->m_minX) + 2 - val1) / 2;

					int my = (pFigureI->m_maxY + pFigureI->m_minY) / 2;

					if ((pFigureI->height() >= min_h / 3) && // ImI is not too small
						(pFigureI->width() <= 1.5*(double)pFigureJ->width()) && // ImI is not too wide
						(pFigureI->m_minY < pFigureJ->m_minY) && // ImI is on top of ImJ						
						( ( (pFigureI->m_maxY < pFigureJ->m_minY + (pFigureJ->height() / 2)) &&
						    (pFigureI->width() >= 1.5*(double)pFigureI->height()) && 
							(pFigureI->height() <= pFigureJ->height()) ) || // ImI is wider then its height (if on top half or higher) and not too high
							( (pFigureI->m_maxY < pFigureJ->m_maxY) && // ImI max is located on first half of ImJ
							  (pFigureI->m_maxY >= pFigureJ->m_minY + (pFigureJ->height() / 2)) ) ) &&
						( 
						(val1 >= 0.75*(double)pFigureI->width()) && // ImI intersect with ImJ by x more then 0.75*ImI_width
						((pFigureJ->m_minY - pFigureI->m_maxY - 1) <= std::min<int>((std::min<int>(pFigureI->height(), pFigureJ->height()) / 3), std::max<int>(pFigureI->height(), pFigureJ->height()) / 4)) // distance between ImI and ImJ is not bigger then Images_height / 4
						))
					{
						/*
#ifdef CUSTOM_DEBUG
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

int GetSubParams(simple_buffer<u8>& Im, int w, int h, u8 white, int& LH, int& LMAXY, int& lb, int& le, int min_h, int real_im_x_center, int yb, int ye, wxString iter_det, bool combine_figures_related_to_each_other)
{
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii, val, N, minN, H, delta, NNN, jY, jI, jQ, maxX;
	simple_buffer<int> GRStr(256 * 2, 0), smax(256 * 2, 0), smaxi(256 * 2, 0);
	int val1, val2, val3;
	int NNY;
	u64 t;

	LMAXY = 0;
	LH = 0;
	lb = h - 1;
	le = 0;

	if ( ((real_im_x_center <= 0) && ((g_text_alignment == TextAlignment::Center) || (g_text_alignment == TextAlignment::Left))) ||
		((real_im_x_center >= w - 1) && (g_text_alignment == TextAlignment::Right)) )
	{
		return 0;
	}

	if (g_text_alignment == TextAlignment::Center)
	{
		maxX = real_im_x_center;
	}
	else
	{
		maxX = w;
	}

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();	

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	simple_buffer<int> NN(N, 0), NY(N, 0), NL(N, 0), NR(N, 0);
	simple_buffer<CMyClosedFigure*> good_figures(N);
	
	/*
#ifdef CUSTOM_DEBUG
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

		if ( (pFigure->height() >= min_h) &&
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
			if ((pFigure->height() > LH) && (pFigure->m_minX < maxX))
			{
				LH = pFigure->height();
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
		if ((NN[i] > NN[j]) && (NL[i] < maxX))
		{
			j = i;
		}
	}
	for (i = 0; i < k; i++)
	{
		//if ((NY[i] > NY[j]) && ((NN[i] >= 2) || (NN[i] >= NN[j])) && (NL[i] < maxX))
		if ((NR[i] - NL[i] > NR[j] - NL[j]) && (NN[i] >= NN[j]) && (NL[i] < maxX))
		{
			j = i;
		}
	}
	LMAXY = NY[j];

	simple_buffer<int> arMinY(N, 0);

	for (i = 0, k = 0; i < NNY; i++)
	{
		pFigure = good_figures[i];

		if ( //(pFigure->m_minX < maxX) &&
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

	custom_assert(i > 0, "GetSubParams: i > 0");
	LH = H / i;

	return 1;
}

int ClearImageByMask(simple_buffer<u8> &Im, simple_buffer<u8> &ImMASK, int w, int h, u8 white, double thr)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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

		for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
		{
			ii = pFigure->m_PointsArray[l];

			if (ImMASK[ii] == 0)
			{
				cnt++;
			}
		}

		if (cnt >= std::max<int>(thr*pFigure->m_PointsArray.m_size, 1))
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageFromMainSymbols(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, u8 white, wxString iter_det)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageOpt2(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, u8 white, wxString iter_det)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;
	int max_len = std::min<int>(0.25*W, h * 2);

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			(g_remove_wide_symbols && (pFigure->width() >= LH * 3)) ||
			(g_remove_wide_symbols && (pFigure->width() >= max_len)) //||
			//((pFigure->width() < 3) || (pFigure->height() < 3)) || image can be too broken in 6 clusters
			//(pFigure->m_PointsArray.m_size >= (LH * LH)/2) //remove good symbols
			)
		{
			/*
#ifdef CUSTOM_DEBUG
			if (iter_det == "l2_02_05_01_id4")
			{
				i = i;
			}
#endif			
			*/

			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageOpt3(simple_buffer<u8> &Im, int w, int h, int real_im_x_center, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;

	if (g_text_alignment != TextAlignment::Center)
	{
		return 1;
	}

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageOpt4(simple_buffer<u8> &Im, int w, int h, int W, int H, int LH, int LMAXY, int real_im_x_center, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;
	int min_h = H * g_min_h;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			(g_remove_wide_symbols && (pFigure->width() >= LH * 3)) ||
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (6 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (1 * LH) / 5) || // do agressive even Arabic
			((pFigure->width() < 3) || (pFigure->height() < 3)) ||
			( ((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) <= (LH/8))  &&
			  ((pFigure->m_maxY - LMAXY) >= (LH / 16)) ) ||
			( (pFigure->height() < 8) && (pFigure->width() >= W / 10) ) ||
			( (pFigure->height() <= 4) && (pFigure->width() >= W / 20) && ((pFigure->m_maxY >= LMAXY) || (pFigure->m_minY <= LMAXY - LH + 1)) )
			//(pFigure->m_maxY < LMAXY - (LH / 2)) || // only main characters are saved
			//(pFigure->m_minY > LMAXY - LH / 5) ||
			//(pFigure->m_PointsArray.m_size >= (LH * LH) / 2) //|| remove good symbols
			//(pFigure->m_PointsArray.m_size <= 0.1*(pFigure->width()*pFigure->height())) ||
			//( (pFigure->m_PointsArray.m_size >= 0.7*(pFigure->width()*pFigure->height())) &&
			// (pFigure->height() >= LH/2) && (pFigure->width() >= pFigure->height()/3) )
			)
		{
			/*
#ifdef CUSTOM_DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif			
			*/

			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

int ClearImageOpt5(simple_buffer<u8> &Im, int w, int h, int LH, int LMAXY, int real_im_x_center, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N;
	int val1, val2, ddy1, ddy2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			(g_remove_wide_symbols && (pFigure->width() >= LH * 3)) ||
			(g_remove_wide_symbols && (pFigure->width() >= h * 2)) ||
			((LMAXY - ((pFigure->m_minY + pFigure->m_maxY) / 2)) > (7 * LH) / 5) ||
			((((pFigure->m_minY + pFigure->m_maxY) / 2) - LMAXY) > (3 * LH) / 5) || // final clear: Arabic symbols can have dots below line
			((pFigure->width() < 3) || (pFigure->height() < 3))
			)
		{
			/*
#ifdef CUSTOM_DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif
			*/

			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

void CombineFiguresRelatedToEachOther2(simple_buffer<CMyClosedFigure*> &ppFigures, int &N)
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

template <class T>
int ClearImageByTextDistance(simple_buffer<T>& Im, int w, int h, int W, int H, int real_im_x_center, T white, wxString iter_det)
{
	if (g_text_alignment == TextAlignment::Any)
	{
		return 1;
	}

	const int dw = (int)(g_btd*(double)W);
	const int dw2 = (int)(g_to*(double)W*2.0);

	CMyClosedFigure *pFigure;
	int i, j, k, val1, val2, l, ii, N, res = 0, x, y, y2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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

	simple_buffer<int> max_x(N, 0);
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

			if ( ( (g_text_alignment == TextAlignment::Center) && (ppFigures[i + 1]->m_minX > real_im_x_center) ) ||
				 (g_text_alignment == TextAlignment::Left) )
			{
				// removing from right side				
				for (j = i + 1; j < N; j++)
				{
					pFigure = ppFigures[j];

					for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
					{
						ii = pFigure->m_PointsArray[l];
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

					for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
					{
						ii = pFigure->m_PointsArray[l];
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

	return N;
}

int ClearImageFromSmallSymbols(simple_buffer<u8>& Im, int w, int h, int W, int H, u8 white)
{
	CMyClosedFigure *pFigure;
	int i, l, ii, N, res = 0, x, y, y2;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;
	t = SearchClosedFigures(Im, w, h, white, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
	for (i = 0; i < N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	//CombineFiguresRelatedToEachOther2(ppFigures, N);

	i = 0;
	while (i < N)
	{
		pFigure = ppFigures[i];

		if ((pFigure->height() < g_msh*(double)H) ||
			(pFigure->width() <= g_segh) ||
			(pFigure->height() <= g_segh) //||
			//(pFigure->m_PointsArray.m_size < g_msd*(pFigure->height()*pFigure->width()))
			)
		{
			/*
#ifdef CUSTOM_DEBUG
			if ((pFigure->m_minX >= 1320) &&
				(pFigure->m_minX <= 1350))
			{
				i = i;
			}
#endif			
			*/

			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N - 1];
			N--;
			continue;
		}
		i++;
	}

	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/ClearImageFromSmallSymbols_01" + g_im_save_format, w, h);

	if (N > 0)
	{
		res = 1;
	}

	return res;
}

void RestoreStillExistLines(simple_buffer<u8>& Im, simple_buffer<u8>& ImOrig, int w, int h, int W, int H)
{
	int i, x, y, y2;
	
	int dy = (g_msh * (double)H) + 1;
	simple_buffer<int> lines_info(h, 0);

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
			Im.copy_data(ImOrig, y * w, y * w, w);
		}
	}

	if (g_show_results) SaveGreyscaleImage(Im, "/DebugImages/RestoreStillExistLines_01" + g_im_save_format, w, h);
}

template <class T>
void InvertBinaryImage(simple_buffer<T>& Im, int w, int h)
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

template <class T>
int GetImageWithOutsideFigures(simple_buffer<T>& Im, simple_buffer<T>& ImRes, int w, int h, T white)
{
	CMyClosedFigure *pFigure;
	int i, l, x, y, ii, cnt, N;

	ImRes.set_values(0, w * h);

	custom_buffer<CMyClosedFigure> pFigures;
	SearchClosedFigures(Im, w, h, (T)0, pFigures);
	N = pFigures.size();

	if (N == 0)	return 0;

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
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

// can_be_optimized
// note: both images should be binary type of images (not greyscale) (should have 0 or white values of pixels)
template <class T>
void MergeImagesByIntersectedFigures(simple_buffer<T>& ImInOut, simple_buffer<T>& ImIn2, int w, int h, T white)
{
	simple_buffer<T> Im2(ImIn2);
	CMyClosedFigure *pFigure;
	int i, j, k, l, ii;
	u64 t;

	custom_buffer<CMyClosedFigure> pFigures;

	t = SearchClosedFigures(ImInOut, w, h, white, pFigures);
	for (i = 0; i < pFigures.size(); i++)
	{
		pFigure = &(pFigures[i]);

		bool found = false;
		for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
		{
			ii = pFigure->m_PointsArray[l];
			if (Im2[ii] != 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				ImInOut[ii] = 0;
			}
		}
	}

	t = SearchClosedFigures(Im2, w, h, white, pFigures);
	for (i = 0; i < pFigures.size(); i++)
	{
		pFigure = &(pFigures[i]);

		bool found = false;
		for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
		{
			ii = pFigure->m_PointsArray[l];
			if (ImInOut[ii] != 0)
			{
				found = true;
				break;
			}
		}

		if (!found)
		{
			for (l = 0; l < pFigure->m_PointsArray.m_size; l++)
			{
				ii = pFigure->m_PointsArray[l];
				Im2[ii] = 0;
			}
		}
	}

	CombineTwoImages(ImInOut, Im2, w, h, white);
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int IsPoint(CMyClosedFigure *pFigure, int LMAXY, int LLH, int W, int H)
{
	int ret;
	double dval;
	
	ret = 0;

	custom_assert(pFigure->width() > 0, "IsPoint: pFigure->width() > 0");
	if (pFigure->height() < pFigure->width()) dval = (double)pFigure->height()/pFigure->width();
	else dval = (double)pFigure->width()/pFigure->height();

	if ( (pFigure->width() >= g_minpw * W) && (pFigure->width() <= g_maxpw * W) &&
	     (pFigure->height() >= g_minph * H) && (pFigure->height() <= g_maxph * H) &&
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

	custom_assert(pFigure->width() > 0, "IsComma: pFigure->width() > 0");
	custom_assert(pFigure->height() > 0, "IsComma: pFigure->height() > 0");
	if (pFigure->height() < pFigure->width()) dval = (double)pFigure->height()/pFigure->width();
	else dval = (double)pFigure->width()/pFigure->height();

	if ( (pFigure->width() >= g_minpw * W) && (pFigure->width() <= 2 * g_maxpw * W) &&
	     (dval <= (2.0/3.0)) && (pFigure->height() <= (int)((double)LLH*0.8)) )
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

void GetSymbolAvgColor(CMyClosedFigure* pFigure, simple_buffer<u8>& ImY, simple_buffer<u8>& ImI, simple_buffer<u8>& ImQ, int& mY, int& mI, int& mQ, int& weight, int W, int H)
{
	int i, ii, j, w, h, x, y, xx, yy, val;
	int r, min_x, max_x, min_y, max_y;

	w = pFigure->m_maxX - pFigure->m_minX + 1;
	h = pFigure->m_maxY - pFigure->m_minY + 1;

	r = max(8, h/6);

	int SIZE = w * h;

	simple_buffer<char> pImage(SIZE, (char)0);
	simple_buffer<u8> pImageY(SIZE, (u8)0), pImageI(SIZE, (u8)0), pImageQ(SIZE, (u8)0);

	for(i=0; i < pFigure->m_PointsArray.m_size; i++)
	{
		ii = pFigure->m_PointsArray[i];
		custom_assert(W > 0, "GetSymbolAvgColor: W > 0");
		y = ii / W;
		x = ii - (y * W);
		j = (y - pFigure->m_minY)*w + (x - pFigure->m_minX);

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
					mY += (int)pImageY[i];
					mI += (int)pImageI[i];
					mQ += (int)pImageQ[i];
					weight++;
				}
			}
		}

		if (weight < pFigure->m_PointsArray.m_size/5)
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
								mY += (int)pImageY[i];
								mI += (int)pImageI[i];
								mQ += (int)pImageQ[i];
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
	} while (weight < pFigure->m_PointsArray.m_size/5);

	custom_assert(weight > 0, "GetSymbolAvgColor: weight > 0");
	mY = mY/weight;
	mI = mI/weight;
	mQ = mQ/weight;
}

void GetTextLineParameters(simple_buffer<u8>& Im, simple_buffer<u8>& ImY, simple_buffer<u8>& ImI, simple_buffer<u8>& ImQ, int w, int h, int& LH, int& LMAXY, int& XB, int& XE, int& YB, int& YE, int& mY, int& mI, int& mQ, u8 white)
{
	CMyClosedFigure *pFigure = NULL;
	int i, j, k, l, N, val, val1, val2, val3, val4;
	int *NH = NULL, NNY, min_h, min_w, prev_min_w;
	u64 t;

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

	simple_buffer<CMyClosedFigure*> ppFigures(N);
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

			if ( (pFigure->height() >= min_h) && (pFigure->width() >= min_w) )
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
	int weight;

	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];
		
		if ( (pFigure->height() >= min_h) && (pFigure->width() >= min_w) )
		{		
			GetSymbolAvgColor(pFigure, ImY, ImI, ImQ, mY, mI, mQ, weight, w, h);

			val += weight;
			val1 += mY*weight;
			val2 += mI*weight;
			val3 += mQ*weight;
		}
	}

	custom_assert(val > 0, "GetTextLineParameters: val > 0");
	mY = val1/val;
	mI = val2/val;
	mQ = val3/val;

	// определяем размеры символов и расположение текста по высоте

	simple_buffer<int> maxY(N, 0), NN(N, 0), NY(N, 0);

	for(i=0, j=0; i < N; i++)
	{
		if (ppFigures[i]->height() >= min_h)
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
             (pFigure->height() >= min_h) )
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
			(pFigure->height() >= min_h) )
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

void StrAnalyseImage(simple_buffer<u8> &Im, simple_buffer<u8> &ImGR, simple_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset)
{
	int i, ib, ie, x, y, val;

	GRStr.set_values(0, (256 + offset));

	ib = yb*w;
	ie = ye*w;
	for (y=yb; y<=ye; y++, ib+=w)
	{
		for (i = ib + xb; i <= ib + xe; i++)
		{
			if ( Im[i] != 0 )
			{
				val = (int)ImGR[i] + offset;
				GRStr[val]++;
			}
		}
	}
}

void FindMaxStrDistribution(simple_buffer<int> &GRStr, int delta, simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &N, int offset)
{
	int i, imax, ys, ys1, ys2, val, NN;

	ys = 0;
	for (i=0; i<delta; i++) ys += GRStr[i];

	ys1 = ys2 = ys;
	NN = 0;
	smax.set_values(0, STR_SIZE);
	smaxi.set_values(0, STR_SIZE);

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

void FindMaxStr(simple_buffer<int> &smax, simple_buffer<int> &smaxi, int &max_i, int &max_val, int N)
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

void ReadFile(cv::Mat& res_data, wxString name)
{
	wxFileInputStream ffin(name);
	size_t size = ffin.GetSize();
	res_data.reserveBuffer(size);
	ffin.ReadAll(res_data.data, size);
}

void WriteFile(std::vector<uchar>& write_data, wxString name)
{
	wxFileOutputStream fout(name);

	if (fout.IsOk())
	{
		fout.WriteAll(write_data.data(), write_data.size());
		fout.Close();
	}
	else
	{
		SaveToReportLog(wxString::Format(wxT("ERROR: Failed to open file for write: %s\n"), name));
	}
}

void GetImageSize(wxString name, int &w, int &h)
{
	cv::Mat data;
	ReadFile(data, name);
	cv::Mat im = cv::imdecode(data, 1);
	w = im.cols;
	h = im.rows;
}

void LoadBGRImage(simple_buffer<u8>& ImBGR, wxString name, int& w, int& h, bool allocate_im_size)
{
	cv::Mat data;
	ReadFile(data, name);
	cv::Mat im = cv::imdecode(data, cv::IMREAD_COLOR); // load in BGR format
	w = im.cols;
	h = im.rows;

	if (allocate_im_size)
	{
		ImBGR.set_size(w * h * 3);
	}

	ImBGR.copy_data(im.data, w * h * 3);
}

void LoadBGRImage(simple_buffer<u8>& ImBGR, wxString name)
{
	int w, h;
	LoadBGRImage(ImBGR, name, w, h, false);
}

void SaveBGRImage(simple_buffer<u8>& ImBGR, wxString name, int w, int h)
{
	if (g_disable_save_images) return;

	cv::Mat im;
	BGRImageToMat(ImBGR, w, h, im);

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
		wxMessageBox(msg, "ERROR: SaveBGRImage");
	}
}

void SaveGreyscaleImage(simple_buffer<u8>& Im, wxString name, int w, int h, int add, double scale, int quality, int dpi)
{
	if (g_disable_save_images) return;

	cv::Mat im(h, w, CV_8UC1);
	u8 color;

	if ((add == 0) && (scale == 1.0))
	{
		custom_assert(w * h <= Im.m_size, "SaveGreyscaleImage(...): not: w * h <= Im.m_size");
		memcpy(im.data, Im.m_pData, w * h);
	}
	else
	{
		for (int i = 0; i < w * h; i++)
		{
			im.data[i] = (double)((int)(Im[i]) + add) * scale;
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
		std::vector<uchar> write_data;
		cv::imencode(cv::String(g_im_save_format), im, write_data, compression_params);
		WriteFile(write_data, g_work_dir + name);
	}
	catch (runtime_error& ex) {
		wxString msg;
		msg.Printf(wxT("Exception saving image to %s format: %s\n"), g_im_save_format, wxString(ex.what()));
		wxMessageBox(msg, "ERROR: SaveGreyscaleImage");
	}
	catch (const exception& ex)
	{
		wxString msg;
		msg.Printf(wxT("Exception saving image to %s format: %s\n"), g_im_save_format, wxString(ex.what()));
		wxMessageBox(msg, "ERROR: SaveGreyscaleImage");
	}
	catch (...)
	{
		wxString msg;
		msg.Printf(wxT("Exception saving image to %s format\n"), g_im_save_format);
		wxMessageBox(msg, "ERROR: SaveGreyscaleImage");
	}
}

void SaveBinaryImageWithLinesInfo(simple_buffer<u8> &Im, wxString name, int lb1, int le1, int lb2, int le2, int w, int h)
{
	if (g_disable_save_images) return;

	simple_buffer<u8> ImTMP(w*h*3, (u8)0);
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
			SetBGRColor(ImTMP, i, wc);
		}
	}

	for (x = 0; x < w; x++)
	{
		if ((lb1 >= 0) && (lb1 < h)) SetBGRColor(ImTMP, lb1 * w + x, rc);
		if ((le1 >= 0) && (le1 < h)) SetBGRColor(ImTMP, le1 * w + x, gc);

		if ((lb2 >= 0) && (lb2 < h)) SetBGRColor(ImTMP, lb2 * w + x, rc);
		if ((le2 >= 0) && (le2 < h)) SetBGRColor(ImTMP, le2 * w + x, gc);
	}

	SaveBGRImage(ImTMP, name, w, h);
}

void SaveBGRImageWithLinesInfo(simple_buffer<u8>& ImBGR, wxString name, simple_buffer<int>& LB, simple_buffer<int>& LE, int& N, int w, int h, int k)
{
	simple_buffer<u8> ImTMP(ImBGR);
	int color, rc, gc, bc;
	u8* pClr;

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

	for (int i = 0; i < N; i++)
	{
		for (int x = 0; x < w; x++)
		{
			if (i == k)
			{
				SetBGRColor(ImTMP, LB[i] * w + x, bc);
				SetBGRColor(ImTMP, LE[i] * w + x, bc);
			}
			else
			{
				SetBGRColor(ImTMP, LB[i] * w + x, rc);
				SetBGRColor(ImTMP, LE[i] * w + x, gc);
			}
		}
	}

	SaveBGRImage(ImTMP, name, w, h);
}

void SaveBGRImageWithLinesInfo(simple_buffer<u8>& ImBGR, wxString name, int w, int h, std::vector<std::pair<int, int>> HLPC, std::vector<std::pair<int, int>> VLPC)
{
	int color, rc, gc, bc;
	simple_buffer<u8> ImTMP(ImBGR, 0, w * h * 3);
	u8* pClr = (u8*)(&color);

	for (auto&& val : HLPC)
	{
		if ((val.first >= 0) && (val.first <= h - 1))
		{
			for (int x = 0; x < w; x++)
			{
				SetBGRColor(ImTMP, val.first * w + x, val.second);
			}
		}
	}

	for (auto&& val : VLPC)
	{
		if ((val.first >= 0) && (val.first <= w - 1))
		{
			for (int y = 0; y < h; y++)
			{
				SetBGRColor(ImTMP, y * w + val.first, val.second);
			}
		}
	}

	SaveBGRImage(ImTMP, name, w, h);
}

void SaveGreyscaleImageWithLinesInfo(simple_buffer<u8>& Im, wxString name, int w, int h, std::vector<std::pair<int, int>> HLPC, std::vector<std::pair<int, int>> VLPC)
{
	simple_buffer<u8> ImTMP(w * h * 3);
	GreyscaleImageToBGR(ImTMP, Im, w, h);
	SaveBGRImageWithLinesInfo(ImTMP, name, w, h, HLPC, VLPC);
}

void SaveGreyscaleImageWithSubParams(simple_buffer<u8>& Im, wxString name, int lb, int le, int LH, int LMAXY, int real_im_x_center, int w, int h)
{
	if (g_disable_save_images) return;

	simple_buffer<u8> ImTMP(w * h * 3);
	int x, y;
	int color, rc, gc, bc, yc, cc, wc;
	u8 *pClr = (u8*)(&color);

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

	GreyscaleImageToBGR(ImTMP, Im, w, h);

	for (x = 0; x < w; x++)
	{
		SetBGRColor(ImTMP, lb * w + x, cc);
		SetBGRColor(ImTMP, le * w + x, cc);

		SetBGRColor(ImTMP, (LMAXY - LH + 1) * w + x, gc);
		SetBGRColor(ImTMP, LMAXY * w + x, gc);
	}

	if (real_im_x_center < 0) real_im_x_center = 0;
	if (real_im_x_center >= w) real_im_x_center = w - 1;

	for (y = 0; y < h; y++)
	{
		SetBGRColor(ImTMP, y * w + real_im_x_center, rc);
	}

	SaveBGRImage(ImTMP, name, w, h);
}

void GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::Mat& res)
{
	res = cv::Mat(h, w, CV_8UC1);
	custom_assert(w * h <= ImGR.m_size, "GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::Mat& res)\nnot: w * h <= ImGR.m_size");
	memcpy(res.data, ImGR.m_pData, w * h);
}

void GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::UMat& res)
{
	cv::Mat im(h, w, CV_8UC1);
	custom_assert(w * h <= ImGR.m_size, "GreyscaleImageToMat(simple_buffer<u8>& ImGR, int w, int h, cv::UMat& res)\nnot: w * h <= ImGR.m_size");
	memcpy(im.data, ImGR.m_pData, w * h);
	im.copyTo(res);
}

void GreyscaleMatToImage(cv::Mat& ImGR, int w, int h, simple_buffer<u8>& res)
{
	res.copy_data(ImGR.data, w * h);
}

void GreyscaleMatToImage(cv::UMat& ImGR, int w, int h, simple_buffer<u8>& res)
{
	cv::Mat im;
	ImGR.copyTo(im);
	res.copy_data(im.data, w * h);
}

void BGRImageToMat(simple_buffer<u8>& ImBGR, int w, int h, cv::Mat& res)
{
	res = cv::Mat(h, w, CV_8UC3);
	custom_assert(w * h * 3 <= ImBGR.m_size, "BGRImageToMat(simple_buffer<T>& ImBGR, int w, int h, cv::Mat& res)\nnot: w * h <= ImBGR.m_size");
	memcpy(res.data, ImBGR.m_pData, w * h * 3);
}

void BGRImageToMat(simple_buffer<u8>& ImBGR, int w, int h, cv::UMat& res)
{
	cv::Mat cv_ImBGR(h, w, CV_8UC3);
	custom_assert(w * h * 3 <= ImBGR.m_size, "BGRImageToMat(simple_buffer<T>& ImBGR, int w, int h, cv::UMat& res)\nnot: w * h <= ImBGR.m_size");
	memcpy(cv_ImBGR.data, ImBGR.m_pData, w * h * 3);
	cv_ImBGR.copyTo(res);
}

void BGRMatToImage(cv::Mat& ImBGR, int w, int h, simple_buffer<u8>& res)
{
	custom_assert(w * h * 3 <= res.m_size, "BGRMatToImage(cv::Mat& ImBGR, int w, int h, simple_buffer<u8>& res)\nnot: w * h <= res.m_size");
	custom_assert(w * h == ImBGR.rows * ImBGR.cols, "BGRMatToImage(cv::Mat& ImBGR, int w, int h, simple_buffer<u8>& res)\nnot: w * h == ImBGR.rows * ImBGR.cols");
	memcpy(res.m_pData, ImBGR.data, w * h * 3);
}

template <class T>
void GreyscaleImageToBinary(simple_buffer<T>& ImRES, simple_buffer<T>& ImGR, int w, int h, T white)
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

void GreyscaleImageToBGR(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImGR, int w, int h)
{
	int x, y;
	int color;
	u8* pClr;
	pClr = (u8*)(&color);

	for (int i = 0; i < w * h; i++)
	{
		if (ImGR[i])
		{
			color = 0;
			pClr[0] = ImGR[i];
			pClr[1] = ImGR[i];
			pClr[2] = ImGR[i];
			SetBGRColor(ImBGR, i, color);
		}
		else
		{
			color = 0;
			SetBGRColor(ImBGR, i, color);
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
