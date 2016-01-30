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

extern int		g_W;
extern int		g_H;
extern int		g_w;
extern int		g_h;
extern int		g_xmin;
extern int		g_xmax;
extern int		g_ymin;
extern int		g_ymax;

extern string   g_dir;

extern double	g_mthr;  //moderate threshold
extern double	g_mvthr; //moderate threshold for VEdges
extern double	g_mhthr; //moderate threshold for HEdges
extern double	g_mnthr; //moderate threshold for NEdges
extern int		g_hvt;	 //horizontal-vertical edges threshold
extern int		g_segw;  //segment width
extern int		g_segh;  //segment height
extern int		g_msegc; //minimum segments count
extern int		g_scd;   //min sum color diff
extern int		g_smcd;  //min sum multiple color diff
extern double	g_btd;   //between text distace
extern double	g_tco;   //text centre offset
extern double	g_tcpo;  //text centre percent offset

extern int		g_mpn;	 //min points number
extern double	g_mpd;   //min points density
extern double	g_mpvd;  //min VEdges points density (per full line)
extern double	g_mphd;  //min HEdges points density (per full line)
extern double	g_mpnd;  //min NEdges points density (per full line)
extern double	g_mpved; //min VEdges points density
extern double	g_mpned; //min NEdges points density

extern int		g_scale;

extern int		g_blnVNE;
extern int		g_blnHE;

extern int		g_debug;

extern bool		g_MMX_SSE;

extern bool		g_hard_sub_mining;
extern int		g_show_results;

extern int		g_dmaxy;

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
void FullSobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE1, custom_buffer<int> &ImVOE2, int w, int h);
void SobelVEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImVOE, int w, int h);
void SobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h);
void FastImprovedSobelNEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImNOE, int w, int h);
void SobelSEdge(custom_buffer<int> &ImIn, custom_buffer<int> &ImSOE, int w, int h);

void IncreaseContrastOperator(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, int w, int h);
void CEDOperator(custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, custom_buffer<int> &ImCED, int w, int h);

void FindAndApplyGlobalThreshold(custom_buffer<int> &Im, int w, int h);
void FindAndApplyLocalThresholding(custom_buffer<int> &Im, int dw, int dh, int w, int h);
void ApplyModerateThreshold(custom_buffer<int> &Im, double mthr, int w, int h);

void AplyESS(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h);
void AplyECP(custom_buffer<int> &ImIn, custom_buffer<int> &ImOut, int w, int h);

void ColorFiltration(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int &N, int w, int h);
void ColorFiltration2(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h, int scd);

void BorderClear(custom_buffer<int> &Im, int dd, int w, int h);
void EasyBorderClear(custom_buffer<int> &Im, int w, int h);

void FreeImage(custom_buffer<int> &Im, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h);
void UnpackImage(custom_buffer<int> &ImIn, custom_buffer<int> &ImRES, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h);

int GetTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImVE, custom_buffer<int> &ImNE, custom_buffer<int> &ImHE, int W, int H);
int GetFastTransformedImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImVE, int W, int H);

int SecondFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImRGB, custom_buffer<int> &ImVE, custom_buffer<int> &ImNE, custom_buffer<int> &LB, custom_buffer<int> &LE, int N, int w, int h);
int ThirdFiltration(custom_buffer<int> &Im, custom_buffer<int> &ImVE, custom_buffer<int> &ImNE, custom_buffer<int> &ImHE, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h);
int ThirdFiltrationForGFTI(custom_buffer<int> &Im, custom_buffer<int> &ImVE, custom_buffer<int> &ImNE, custom_buffer<int> &ImHE, custom_buffer<int> &LB, custom_buffer<int> &LE, int LN, int w, int h);

int FindTextLines(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNF, vector<string> &SavedFiles, int W, int H);

void StrAnalyseImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);
void FindMaxStrDistribution(custom_buffer<int> &GRStr, int delta, custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &N, int offset);
void FindMaxStr(custom_buffer<int> &smax, custom_buffer<int> &smaxi, int &max_i, int &max_val, int N);

int AnalizeAndClearImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, int w, int h, int j1_min, int j1_max, int r, int g, int yb, int ye, int xb, int xe, int &cnt1, int &cnt2);

void StrAnalyseImage(custom_buffer<int> &Im, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset);
void StrAnalyseImage(CMyClosedFigure *pFigure, custom_buffer<int> &ImGR, custom_buffer<int> &GRStr, int offset);

void ClearImage4x4(custom_buffer<int> &Im, int w, int h, int white);
void ClearImageSpecific1(custom_buffer<int> &Im, int w, int h, int yb, int ye, int xb, int xe, int white);
void ClearImageSpecific2(custom_buffer<int> &Im, int w, int h, int LMAXY, int LH, int white);
void ClearImageSpecific(custom_buffer<int> &Im, int w, int h, int white);
int ClearImage(custom_buffer<int> &Im, int w, int h, int yb, int ye, int white);
int ClearImageDetailed(custom_buffer<int> &Im, int w, int h, int yb, int ye, int white);
int ClearImageOptimal(custom_buffer<int> &Im, int w, int h, int yb, int ye, int white);

int ClearImageOpt2(custom_buffer<int> &Im, int w, int h, int white, int &LH, int &LMAXY, 
					int &jY_min, int &jY_max, int &jI_min, int &jI_max, int &jQ_min, int &jQ_max,
					int &mY, int &dY, int &mI, int &dI, int &mQ, int &dQ,
					int &mmY, int &ddY1, int &ddY2, int &mmI, int &ddI, int &mmQ, int &ddQ);

void ClearImageOpt3(custom_buffer<int> &Im, int w, int h, int LH, int LMAXY, int jI_min, int jI_max, int jQ_min, int jQ_max, int white);

int ClearImageLogical(custom_buffer<int> &Im, int w, int h, int &LH, int &LMAXY, int xb, int xe, int white);

void SaveTextLineParameters(string ImageName, int YB, int LH, int LY, int LXB, int LXE, int LYB, int LYE, int mY, int mI, int mQ);
void GetSymbolAvgColor(CMyClosedFigure *pFigure, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ);
void GetTextLineParameters(custom_buffer<int> &Im, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ, int w, int h, int &LH, int &LMAXY, int &XB, int &XE, int &YB, int &YE, int &mY, int &mI, int &mQ, int white);

int ClearImageOpt5(custom_buffer<int> &Im, custom_buffer<int> &ImY, custom_buffer<int> &ImI, custom_buffer<int> &ImQ,
	int w, int h, int LH, int LMAXY, int jY_min, int jY_max, int jI_min, int jI_max, int jQ_min, int jQ_max,
	int mY, int dY, int mI, int dI, int mQ, int dQ, int mmY, int ddY1, int ddY2, int mmI, int ddI, int mmQ, int ddQ, int white);

void ResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);
void SimpleResizeImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);
void ResizeGrayscaleImage4x(custom_buffer<int> &Im, custom_buffer<int> &ImRES, int w, int h);

int CompareTXTImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int w1, int h1, int w2, int h2, int YB1, int YB2);

void GetImageSize(string name, int &w, int &h);
void SaveRGBImage(custom_buffer<int> &Im, string name, int w, int h);
void LoadRGBImage(custom_buffer<int> &Im, string name, int &w, int &h);
void SaveGreyscaleImage(custom_buffer<int> &Im, string name, int w, int h, int quality = -1, int dpi = -1);
void LoadGreyscaleImage(custom_buffer<int> &Im, string name, int &w, int &h);
