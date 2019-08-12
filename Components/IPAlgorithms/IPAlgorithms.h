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

using namespace std;

#include <stdio.h>

extern void     (*g_pViewRGBImage)(custom_buffer<int> &Im, int w, int h);
extern void     (*g_pViewImage[2])(custom_buffer<int> &Im, int w, int h);

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

void RGB_to_YUV(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImU, custom_buffer<int> &ImV, int w, int h);
void YIQ_to_RGB(int Y, int I, int Q, int &R, int &G, int &B, int max_val);
void RGB_to_YIQ(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h);
void GetGrayscaleImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImY, int w, int h);

void SobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h);
void ImprovedSobelMEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImMOE, int w, int h);
void SobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h);
void FastImprovedSobelHEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImHOE, int w, int h);
void FastSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h);
void FastImprovedSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h);
void SobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h);
void FastImprovedSobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h);
void SobelSEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImSOE, int w, int h);

void IncreaseContrastOperator(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, int w, int h);

void FindAndApplyGlobalThreshold(custom_buffer<int> &Im, int w, int h);
void FindAndApplyLocalThresholding(custom_buffer<int> &Im, int dw, int dh, int w, int h);
void ApplyModerateThresholdBySplit(custom_buffer<int> &Im, double mthr, int w, int h, int W, int H);
void ApplyModerateThreshold(custom_buffer<int> &Im, double mthr, int w, int h);

void AplyESS(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h);
void AplyECP(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h);

void ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h);

void BorderClear(custom_buffer<int> &Im, int dd, int w, int h);
void EasyBorderClear(custom_buffer<int> &Im, int w, int h);

int GetTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, custom_buffer<int> &ImY, int w, int h, int W, int H);
int FilterTransformedImage(custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h, int W, int H, std::string iter_det);
int FilterImage(custom_buffer<int> &ImF, custom_buffer<int> &ImNE, int w, int h, int W, int H, custom_buffer<int> &LB, custom_buffer<int> &LE, int N);

int FindTextLines(custom_buffer<int> &ImRGB, custom_buffer<int> &ImClearedText, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, custom_buffer<int> &ImNE, custom_buffer<int> &ImIL, vector<wxString> &SavedFiles, int W, int H);

void StrAnalyseImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);
void FindMaxStrDistribution(custom_buffer<int> &GRStr, int delta, custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &N, int offset);
void FindMaxStr(custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &max_i, int &max_val, int N);

void ExtendImFWithDataFromImNF(custom_buffer<int> &ImF, custom_buffer<int> &ImNF, int w, int h);

void StrAnalyseImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);
void StrAnalyseImage(CMyClosedFigure *pFigure, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int offset);

void ClearImage4x4(custom_buffer<int> &Im, int w, int h, int white);
void ClearImageSpecific1(custom_buffer<int> &Im, int w, int h, int yb, int ye, int xb, int xe, int white);
int ClearImageFromBorders(custom_buffer<int> &Im, int w, int h, int ddy1, int ddy2, int white);

int ClearImageLogical(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int xb, int xe, int white, int W, int H, int real_im_x_center);

void SaveTextLineParameters(string ImageName, int YB, int LH, int LY, int LXB, int LXE, int LYB, int LYE, int mY, int mI, int mQ, int W, int H);
void GetSymbolAvgColor(CMyClosedFigure *pFigure, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ);
void GetTextLineParameters(custom_buffer<int> &Im, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h, int &LH, int &LMAXY, int &XB, int &XE, int &YB, int &YE, int &mY, int &mI, int &mQ, int white);

void ResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);
void SimpleResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);
void ResizeGrayscaleImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);

int CompareTXTImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int w1, int h1, int w2, int h2, int YB1, int YB2);

void GetImageSize(string name, int &w, int &h);
void SaveRGBImage(custom_buffer<int> &Im, string name, int w, int h);
void LoadRGBImage(custom_buffer<int> &Im, string name, int w, int h);
void SaveGreyscaleImage(custom_buffer<int> &Im, string name, int w, int h, int add = 0, double scale = 1.0, int quality = -1, int dpi = -1);
void LoadGreyscaleImage(custom_buffer<int> &Im, string name, int &w, int &h);
void SaveBinaryImage(custom_buffer<int> &Im, string name, int w, int h, int quality = -1, int dpi = -1);
void IntersectTwoImages(custom_buffer<int> &ImRes, custom_buffer<int> &Im2, int w, int h);

bool InitCUDADevice();

void RestoreStillExistLines(custom_buffer<int> &Im, custom_buffer<int> &ImOrig, int w, int h);
