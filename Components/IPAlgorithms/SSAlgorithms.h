                              //SSAlgorithms.h//                                
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

#pragma once

#include "IPAlgorithms.h"
#include "DataTypes.h"
#include "Video.h"
#include <string>

using namespace std;

extern int		g_RunSubSearch;

extern int		g_DL;	 //sub frame length
extern double	g_tp;	 //text procent
extern double	g_mtpl;  //min text len (in procent)
extern double	g_sse;	 //sub square error
extern double	g_veple; //vedges points line error

extern bool g_fast_search;

void SetVideoWindowSettins(CVideo *pV, double dx_min, double dx_max, double dy_min, double dy_max);

s64 SearchSubtitles(CVideo *pV, s64 Begin, s64 End);
s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End);

int CompareTwoImages(int *Im1, int *ImNFF1, int *Im2, int *ImNFF2, int size);

int AnalyseImage(int *Im, int w, int h);

int CompareTwoSubs(int *Im1, int *ImVE1, int *Im2, int *ImVE2, int w, int h);

int PreCompareTwoSubs(int *Im1, int *Im2, int *ImRES, int *lb, int *le, int w, int h); // return ln

int FinalCompareTwoSubs1(int *ImRES, int *lb, int *le, int ln, int *ImVE1, int *ImVE2, int w, int h); // return 0 or 1
int FinalCompareTwoSubs2(int *ImRES, int *lb, int *le, int ln, int *ImVE1, int *ImVE2, int w, int h);

int DifficultCompareTwoSubs(int *ImRGB1, int *ImF1, int *ImRGB2, int *ImF2, int w, int h);

int SimpleCombineTwoImages(int *Im1, int *Im2, int size);

int GetCombinedSquare(int *Im1, int *Im2, int size);

void AddTwoImages(int *Im1, int *Im2, int *ImRES, int size);
void AddTwoImages(int *Im1, int *Im2, int size);

int ConvertImage(int *ImRGB, int *ImF, int *ImVE, int w, int h);

int GetAndConvertImage(int *ImRGB, int *ImFF, int *ImSF, int *ImTF, int *ImVE, int *ImNE, int *ImHE, CVideo *pVideo, int w, int h);

void ImToNativeSize(int *Im, int w, int h);

string VideoTimeToStr(s64 pos);