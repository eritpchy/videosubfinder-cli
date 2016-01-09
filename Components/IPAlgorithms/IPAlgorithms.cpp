                              //IPAlgorithms.cpp//                                
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

#include "IPAlgorithms.h"
#include <math.h>
#include <assert.h>
#include <emmintrin.h>

void    (*g_pViewRGBImage)(int *Im, int w, int h);
void    (*g_pViewImage[2])(int *Im, int w, int h);

int g_W;
int g_H;
int g_w;
int g_h;
int g_xmin;
int g_xmax;
int g_ymin;
int g_ymax;

string  g_dir;

double	g_mthr = 0.4;
double	g_mvthr = 0.3;
double	g_mhthr = 0.2;
double	g_mnthr = 0.3;
int		g_hvt = 65;	 
int		g_segw = 8;
int     g_segh = 3;  
int		g_msegc = 2;
int		g_scd = 800;
int		g_smcd = 600;
double	g_btd = 0.05;
double	g_tco = 0.1;
double	g_tcpo = 0.5;

int		g_mpn = 50;	 //min points number
double	g_mpd = 0.7; //0.7
double	g_mpvd = 0.3; //0.3
double	g_mphd = 0.2; //0.2
double	g_mpnd = 0.3; //0.3
double	g_mpved = 0.3; //0.3
double	g_mpned = 0.4; //0.4 

int g_min_color = 5;

int g_dmaxy = 8;
int g_dminy = 8;

int g_minpw = 4;
int g_maxpw = 18;
int g_minph = 4;
int g_maxph = 18; 
double g_minpwh = 2.0/3.0;

int g_min_dI = 9;
int g_min_dQ = 9;
int g_min_ddI = 14;
int g_min_ddQ = 14;

s64 *g_ImYIQ = NULL;
s64 *g_ImRES1_64 = NULL;
s64 *g_ImRES2_64 = NULL;

int *g_ImY = NULL;
int *g_ImU = NULL;
int *g_ImV = NULL;
int *g_ImI = NULL;
int *g_ImQ = NULL;

int *g_ImYMOE = NULL;
int *g_ImUMOE = NULL;
int *g_ImVMOE = NULL;
int *g_ImCMOE = NULL;

int *g_Im = NULL;
int *g_ImRES1 = NULL;
int *g_ImRES2 = NULL;
int *g_ImRES3 = NULL;
int *g_ImRES4 = NULL;
int *g_ImRES5 = NULL;
int *g_ImRES6 = NULL;
int *g_ImRES7 = NULL;
int *g_ImRES8 = NULL;
int *g_ImRES9 = NULL;

int *g_ImSF = NULL;
int *g_ImFF = NULL;
int *g_ImRR = NULL;

int *g_ImRGB = NULL;
int *g_ImF[6] = {NULL, NULL, NULL, NULL, NULL, NULL};

//JSAMPLE *g_jsrow = NULL;

#define MAX_EDGE_STR 786432 //на сам деле ~ 32*3*255

int g_edgeStr[MAX_EDGE_STR];
int *g_pLB = NULL;
int *g_pLE = NULL;

int g_LN;

int *g_pL2 = NULL;
int *g_pLB2 = NULL;
int *g_pLE2 = NULL;
int *g_pLB3 = NULL;
int *g_pLE3 = NULL;
int *g_pLB4 = NULL;
int *g_pLE4 = NULL;
int *g_pLB5 = NULL;
int *g_pLE5 = NULL;
int *g_pLB6 = NULL;
int *g_pLE6 = NULL;
int *g_pLB7 = NULL;
int *g_pLE7 = NULL;
int *g_pLB8 = NULL;
int *g_pLE8 = NULL;
int *g_pLB9 = NULL;
int *g_pLE9 = NULL;

int *g_pLL = NULL;
int *g_pLR = NULL;
int *g_pLLB = NULL;
int *g_pLLE = NULL;
int *g_pLW = NULL;
int *g_pLNN = NULL;
int **g_ppLLLB = NULL;
int **g_ppLLLE = NULL;

int *g_pLL2 = NULL;
int *g_pLR2 = NULL;
int *g_pLLB2 = NULL;
int *g_pLLE2 = NULL;
int *g_pLW2 = NULL;
int *g_pLNN2 = NULL;
int **g_ppLLLB2 = NULL;
int **g_ppLLLE2 = NULL;

int *g_pLL3 = NULL;
int *g_pLR3 = NULL;
int *g_pLLB3 = NULL;
int *g_pLLE3 = NULL;
int *g_pLW3 = NULL;
int *g_pLNN3 = NULL;
int **g_ppLN3 = NULL;

int *g_pLLB4 = NULL;
int *g_pLLE4 = NULL;

int *g_pImFF1 = NULL;
int *g_pImVE1 = NULL;
int *g_pImNE1 = NULL;
int *g_pImFF2 = NULL;
int *g_pImVE2 = NULL;
int *g_pImNE2 = NULL;
int *g_pImTEMP1 = NULL;
int *g_pImTEMP2 = NULL;
int *g_pImTEMP3 = NULL;
int *g_ImRES10 = NULL;
int *g_ImRES11 = NULL;
int *g_ImRES12 = NULL;

int g_scale = 1;

int g_blnVNE = 0;
int g_blnHE = 0;

int g_str_size = 256*2;

bool g_hard_sub_mining = false;
int g_show_results = 1;

bool g_wxImageHandlersInitialized = false;

#ifdef _DEBUG
int g_debug = 1;
#else
int g_debug = 0;
#endif

bool g_MMX_SSE = true;

void InitIPData(int w, int h, int scale)
{
	int size, i;
	
	ReleaseIPData();

	g_W = w;
	g_H = h;

	g_scale = scale;

    //-------------
	size = w*h*scale;	
	
	g_ImYIQ = new s64[size];
	memset(g_ImYIQ, 0, size*sizeof(s64));

	g_ImRES1_64 = new s64[size];
	memset(g_ImRES1_64, 0, size*sizeof(s64));

	g_ImRES2_64 = new s64[size];
	memset(g_ImRES2_64, 0, size*sizeof(s64));

    g_ImY = new int[size];
	memset(g_ImY, 0, size*sizeof(int));

	g_ImU = new int[size];
	memset(g_ImU, 0, size*sizeof(int));

	g_ImV = new int[size];
	memset(g_ImV, 0, size*sizeof(int));

	g_ImI = new int[size];
	memset(g_ImI, 0, size*sizeof(int));

	g_ImQ = new int[size];
	memset(g_ImQ, 0, size*sizeof(int));

    g_ImYMOE = new int[size];
	memset(g_ImYMOE, 0, size*sizeof(int));

	g_ImUMOE = new int[size];
	memset(g_ImUMOE, 0, size*sizeof(int));

	g_ImVMOE = new int[size];
	memset(g_ImVMOE, 0, size*sizeof(int));

	g_ImCMOE = new int[size];
	memset(g_ImCMOE, 0, size*sizeof(int));

    g_Im = new int[size];
	memset(g_Im, 0, size*sizeof(int));

	g_ImRES1 = new int[size];
	memset(g_ImRES1, 0, size*sizeof(int));

	g_ImRES2 = new int[size];
	memset(g_ImRES2, 0, size*sizeof(int));

	g_ImRES3 = new int[size];
	memset(g_ImRES3, 0, size*sizeof(int));

	g_ImRES4 = new int[size];
	memset(g_ImRES4, 0, size*sizeof(int));

	g_ImRES5 = new int[size];
	memset(g_ImRES5, 0, size*sizeof(int));

	g_ImRES6 = new int[size];
	memset(g_ImRES6, 0, size*sizeof(int));

	g_ImRES7 = new int[size];
	memset(g_ImRES7, 0, size*sizeof(int));

	g_ImRES8 = new int[size];
	memset(g_ImRES8, 0, size*sizeof(int));

	g_ImRES9 = new int[size];
	memset(g_ImRES9, 0, size*sizeof(int));
	
	g_ImSF = new int[size];
	memset(g_ImSF, 0, size*sizeof(int));

	g_ImFF = new int[size];
	memset(g_ImFF, 0, size*sizeof(int));

	g_ImRR = new int[size];
	memset(g_ImRR, 0, size*sizeof(int));
	//-------------

	//-------------
	memset(g_edgeStr, 0, MAX_EDGE_STR*sizeof(int));
	//-------------

	//-------------
	size = g_W*g_H;

	g_ImRGB = new int[size];
	memset(g_ImRGB, 0, size*sizeof(int));

	for(i=0; i<6; i++)
	{
		g_ImF[i] = new int[size];
		memset(g_ImF[i], 0, size*sizeof(int));	
	}
	//-------------

	//-------------
	size = g_W*3*4;

	//g_jsrow = new JSAMPLE[size];
	//memset(g_jsrow, 0, size*sizeof(JSAMPLE));	
	//-------------

	//-------------
	size = max(g_W, g_H);

	g_pLB = new int[size];
	memset(g_pLB, 0, size*sizeof(int));

	g_pLE = new int[size];
	memset(g_pLE, 0, size*sizeof(int));	

	g_pL2 = new int[size];
	memset(g_pL2, 0, size*sizeof(int));

	g_pLB2 = new int[size];
	memset(g_pLB2, 0, size*sizeof(int));

	g_pLE2 = new int[size];
	memset(g_pLE2, 0, size*sizeof(int));	

	g_pLB3 = new int[size];
	memset(g_pLB3, 0, size*sizeof(int));

	g_pLE3 = new int[size];
	memset(g_pLE3, 0, size*sizeof(int));	

	g_pLB4 = new int[size];
	memset(g_pLB4, 0, size*sizeof(int));

	g_pLE4 = new int[size];
	memset(g_pLE4, 0, size*sizeof(int));	

	g_pLB5 = new int[size];
	memset(g_pLB5, 0, size*sizeof(int));

	g_pLE5 = new int[size];
	memset(g_pLE5, 0, size*sizeof(int));	

	g_pLB6 = new int[size];
	memset(g_pLB6, 0, size*sizeof(int));

	g_pLE6 = new int[size];
	memset(g_pLE6, 0, size*sizeof(int));

	g_pLB7 = new int[size];
	memset(g_pLB7, 0, size*sizeof(int));

	g_pLE7 = new int[size];
	memset(g_pLE7, 0, size*sizeof(int));

	g_pLB8 = new int[size];
	memset(g_pLB8, 0, size*sizeof(int));

	g_pLE8 = new int[size];
	memset(g_pLE8, 0, size*sizeof(int));

	g_pLB9 = new int[size];
	memset(g_pLB9, 0, size*sizeof(int));

	g_pLE9 = new int[size];
	memset(g_pLE9, 0, size*sizeof(int));
	//-------------

	//-------------
	size = max(g_W, g_H);

	g_pLL = new int[size];
	memset(g_pLL, 0, size*sizeof(int));	

	g_pLR = new int[size];
	memset(g_pLR, 0, size*sizeof(int));	

	g_pLLB = new int[size];
	memset(g_pLLB, 0, size*sizeof(int));	

	g_pLLE = new int[size];
	memset(g_pLLE, 0, size*sizeof(int));

	g_pLW = new int[size];
	memset(g_pLW, 0, size*sizeof(int));	

	g_pLNN = new int[size];
	memset(g_pLNN, 0, size*sizeof(int));	

	g_pLL2 = new int[size];
	memset(g_pLL2, 0, size*sizeof(int));	

	g_pLR2 = new int[size];
	memset(g_pLR2, 0, size*sizeof(int));	

	g_pLLB2 = new int[size];
	memset(g_pLLB2, 0, size*sizeof(int));	

	g_pLLE2 = new int[size];
	memset(g_pLLE2, 0, size*sizeof(int));

	g_pLW2 = new int[size];
	memset(g_pLW2, 0, size*sizeof(int));	

	g_pLNN2 = new int[size];
	memset(g_pLNN2, 0, size*sizeof(int));

	g_pLL3 = new int[size];
	memset(g_pLL3, 0, size*sizeof(int));	

	g_pLR3 = new int[size];
	memset(g_pLR3, 0, size*sizeof(int));	

	g_pLLB3 = new int[size];
	memset(g_pLLB3, 0, size*sizeof(int));	

	g_pLLE3 = new int[size];
	memset(g_pLLE3, 0, size*sizeof(int));

	g_pLW3 = new int[size];
	memset(g_pLW3, 0, size*sizeof(int));	

	g_pLNN3 = new int[size];
	memset(g_pLNN3, 0, size*sizeof(int));

	g_pLLB4 = new int[size];
	memset(g_pLLB4, 0, size*sizeof(int));	

	g_pLLE4 = new int[size];
	memset(g_pLLE4, 0, size*sizeof(int));
	//-------------

	//-------------
	size = g_H;	

	g_ppLLLB = new int*[size];
	memset(g_ppLLLB, 0, size*sizeof(int*));	

	for(i=0; i<size; i++)
	{
		g_ppLLLB[i] = new int[g_H];
		memset(g_ppLLLB[i], 0, g_H*sizeof(int));	
	}

	g_ppLLLE = new int*[size];
	memset(g_ppLLLE, 0, size*sizeof(int*));	

	for(i=0; i<size; i++)
	{
		g_ppLLLE[i] = new int[g_H];
		memset(g_ppLLLE[i], 0, g_H*sizeof(int));	
	}

	g_ppLLLB2 = new int*[size];
	memset(g_ppLLLB2, 0, size*sizeof(int*));	

	for(i=0; i<size; i++)
	{
		g_ppLLLB2[i] = new int[g_H];
		memset(g_ppLLLB2[i], 0, g_H*sizeof(int));	
	}

	g_ppLLLE2 = new int*[size];
	memset(g_ppLLLE2, 0, size*sizeof(int*));	

	for(i=0; i<size; i++)
	{
		g_ppLLLE2[i] = new int[g_H];
		memset(g_ppLLLE2[i], 0, g_H*sizeof(int));	
	}

	g_ppLN3 = new int*[size];
	memset(g_ppLN3, 0, size*sizeof(int*));	

	for(i=0; i<size; i++)
	{
		g_ppLN3[i] = new int[g_W];
		memset(g_ppLN3[i], 0, g_W*sizeof(int));	
	}
	//-------------

	//-------------
	size = g_W*g_H;

	g_pImFF1 = new int[size];
	memset(g_pImFF1, 0, size*sizeof(int));

	g_pImVE1 = new int[size];
	memset(g_pImVE1, 0, size*sizeof(int));

	g_pImNE1 = new int[size];
	memset(g_pImNE1, 0, size*sizeof(int));

	g_pImFF2 = new int[size];
	memset(g_pImFF2, 0, size*sizeof(int));

	g_pImVE2 = new int[size];
	memset(g_pImVE2, 0, size*sizeof(int));

	g_pImNE2 = new int[size];
	memset(g_pImNE2, 0, size*sizeof(int));

	g_pImTEMP1 = new int[size];
	memset(g_pImTEMP1, 0, size*sizeof(int));

	g_pImTEMP2 = new int[size];
	memset(g_pImTEMP2, 0, size*sizeof(int));

	g_pImTEMP3 = new int[size];
	memset(g_pImTEMP3, 0, size*sizeof(int));

	g_ImRES10 = new int[size];
	memset(g_ImRES10, 0, size*sizeof(int));

	g_ImRES11 = new int[size];
	memset(g_ImRES11, 0, size*sizeof(int));

	g_ImRES12 = new int[size];
	memset(g_ImRES12, 0, size*sizeof(int));
	//-------------
}

void ReleaseIPData()
{
	int i;

	//-------------
	if (g_ImYIQ != NULL) 
	{
		delete[] g_ImYIQ;
		g_ImYIQ = NULL;
	}

	if (g_ImRES1_64 != NULL) 
	{
		delete[] g_ImRES1_64;
		g_ImRES1_64 = NULL;
	}

	if (g_ImRES2_64 != NULL) 
	{
		delete[] g_ImRES2_64;
		g_ImRES2_64 = NULL;
	}
	//-------------

	//-------------
	if (g_ImY != NULL) 
	{
		delete[] g_ImY;
		g_ImY = NULL;
	}

	if (g_ImU != NULL) 
	{
		delete[] g_ImU;
		g_ImU = NULL;
	}

	if (g_ImV != NULL) 
	{
		delete[] g_ImV;
		g_ImV = NULL;
	}

	if (g_ImI != NULL) 
	{
		delete[] g_ImI;
		g_ImI = NULL;
	}

	if (g_ImQ != NULL) 
	{
		delete[] g_ImQ;
		g_ImQ = NULL;
	}
	//-------------

	//-------------
	if (g_ImYMOE != NULL) 
	{
		delete[] g_ImYMOE;
		g_ImYMOE = NULL;
	}

	if (g_ImUMOE != NULL) 
	{
		delete[] g_ImUMOE;
		g_ImUMOE = NULL;
	}

	if (g_ImVMOE != NULL) 
	{
		delete[] g_ImVMOE;
		g_ImVMOE = NULL;
	}

	if (g_ImCMOE != NULL) 
	{
		delete[] g_ImCMOE;
		g_ImCMOE = NULL;
	}
	//-------------


	//-------------
	if (g_Im != NULL) 
	{
		delete[] g_Im;
		g_Im = NULL;
	}

	if (g_ImRES1 != NULL) 
	{
		delete[] g_ImRES1;
		g_ImRES1 = NULL;
	}

	if (g_ImRES2 != NULL) 
	{
		delete[] g_ImRES2;
		g_ImRES2 = NULL;
	}

	if (g_ImRES3 != NULL) 
	{
		delete[] g_ImRES3;
		g_ImRES3 = NULL;
	}

	if (g_ImRES4 != NULL) 
	{
		delete[] g_ImRES4;
		g_ImRES4 = NULL;
	}

	if (g_ImRES5 != NULL) 
	{
		delete[] g_ImRES5;
		g_ImRES5 = NULL;
	}

	if (g_ImRES6 != NULL) 
	{
		delete[] g_ImRES6;
		g_ImRES6 = NULL;
	}

	if (g_ImRES7 != NULL) 
	{
		delete[] g_ImRES7;
		g_ImRES7 = NULL;
	}

	if (g_ImRES8 != NULL) 
	{
		delete[] g_ImRES8;
		g_ImRES8 = NULL;
	}

	if (g_ImRES9 != NULL) 
	{
		delete[] g_ImRES9;
		g_ImRES9 = NULL;
	}
	//-------------

	//-------------
	if (g_ImSF != NULL) 
	{
		delete[] g_ImSF;
		g_ImSF = NULL;
	}

	if (g_ImFF != NULL) 
	{
		delete[] g_ImFF;
		g_ImFF = NULL;
	}

	if (g_ImRR != NULL) 
	{
		delete[] g_ImRR;
		g_ImRR = NULL;
	}
	//-------------

	//-------------
	if (g_ImRGB != NULL)
	{
		delete[] g_ImRGB;
		g_ImRGB = NULL;
	}

	for(i=0; i<6; i++)
	{
		if (g_ImF[i] != NULL)
		{
			delete[] g_ImF[i];
			g_ImF[i] = NULL;
		}
	}
	//-------------

	//-------------
	/*if (g_jsrow != NULL)
	{
		delete[] g_jsrow;
		g_jsrow = NULL;
	}*/
	//-------------

	//-------------
	if (g_pLB != NULL) 
	{
		delete[] g_pLB;
		g_pLB = NULL;
	}
	
	if (g_pLE != NULL) 
	{
		delete[] g_pLE;
		g_pLE = NULL;
	}

	if (g_pL2 != NULL) 
	{
		delete[] g_pL2;
		g_pL2 = NULL;
	}

	if (g_pLB2 != NULL) 
	{
		delete[] g_pLB2;
		g_pLB2 = NULL;
	}
	
	if (g_pLE2 != NULL) 
	{
		delete[] g_pLE2;
		g_pLE2 = NULL;
	}

	if (g_pLB3 != NULL) 
	{
		delete[] g_pLB3;
		g_pLB3 = NULL;
	}

	if (g_pLE3 != NULL) 
	{
		delete[] g_pLE3;
		g_pLE3 = NULL;
	}

	if (g_pLB4 != NULL) 
	{
		delete[] g_pLB4;
		g_pLB4 = NULL;
	}
	
	if (g_pLE4 != NULL) 
	{
		delete[] g_pLE4;
		g_pLE4 = NULL;
	}

	if (g_pLB5 != NULL) 
	{
		delete[] g_pLB5;
		g_pLB5 = NULL;
	}
	
	if (g_pLE5 != NULL) 
	{
		delete[] g_pLE5;
		g_pLE5 = NULL;
	}

	if (g_pLB6 != NULL) 
	{
		delete[] g_pLB6;
		g_pLB6 = NULL;
	}
	
	if (g_pLE6 != NULL) 
	{
		delete[] g_pLE6;
		g_pLE6 = NULL;
	}

	if (g_pLB7 != NULL) 
	{
		delete[] g_pLB7;
		g_pLB7 = NULL;
	}
	
	if (g_pLE7 != NULL) 
	{
		delete[] g_pLE7;
		g_pLE7 = NULL;
	}

	if (g_pLB8 != NULL) 
	{
		delete[] g_pLB8;
		g_pLB8 = NULL;
	}
	
	if (g_pLE8 != NULL) 
	{
		delete[] g_pLE8;
		g_pLE8 = NULL;
	}

	if (g_pLB9 != NULL) 
	{
		delete[] g_pLB9;
		g_pLB9 = NULL;
	}
	
	if (g_pLE9 != NULL) 
	{
		delete[] g_pLE9;
		g_pLE9 = NULL;
	}
	//-------------

	//-------------
	if (g_pLL != NULL)
	{
		delete[] g_pLL;
		g_pLL = NULL;
	}

	if (g_pLR != NULL)
	{
		delete[] g_pLR;
		g_pLR = NULL;
	}

	if (g_pLLB != NULL)
	{
		delete[] g_pLLB;
		g_pLLB = NULL;
	}

	if (g_pLLE != NULL)
	{
		delete[] g_pLLE;
		g_pLLE = NULL;
	}

	if (g_pLW != NULL)
	{
		delete[] g_pLW;
		g_pLW = NULL;
	}

	if (g_pLNN != NULL)
	{
		delete[] g_pLNN;
		g_pLNN = NULL;
	}

	if (g_pLL2 != NULL)
	{
		delete[] g_pLL2;
		g_pLL2 = NULL;
	}

	if (g_pLR2 != NULL)
	{
		delete[] g_pLR2;
		g_pLR2 = NULL;
	}

	if (g_pLLB2 != NULL)
	{
		delete[] g_pLLB2;
		g_pLLB2 = NULL;
	}

	if (g_pLLE2 != NULL)
	{
		delete[] g_pLLE2;
		g_pLLE2 = NULL;
	}

	if (g_pLW2 != NULL)
	{
		delete[] g_pLW2;
		g_pLW2 = NULL;
	}

	if (g_pLNN2 != NULL)
	{
		delete[] g_pLNN2;
		g_pLNN2 = NULL;
	}

	if (g_pLL3 != NULL)
	{
		delete[] g_pLL3;
		g_pLL3 = NULL;
	}

	if (g_pLR3 != NULL)
	{
		delete[] g_pLR3;
		g_pLR3 = NULL;
	}

	if (g_pLLB3 != NULL)
	{
		delete[] g_pLLB3;
		g_pLLB3 = NULL;
	}

	if (g_pLLE3 != NULL)
	{
		delete[] g_pLLE3;
		g_pLLE3 = NULL;
	}

	if (g_pLW3 != NULL)
	{
		delete[] g_pLW3;
		g_pLW3 = NULL;
	}

	if (g_pLNN3 != NULL)
	{
		delete[] g_pLNN3;
		g_pLNN3 = NULL;
	}

	if (g_pLLB4 != NULL)
	{
		delete[] g_pLLB4;
		g_pLLB4 = NULL;
	}

	if (g_pLLE4 != NULL)
	{
		delete[] g_pLLE4;
		g_pLLE4 = NULL;
	}
	//-------------

	//-------------
	if (g_ppLLLB != NULL)
	{
		for(i=0; i<g_H; i++)
		{
			if (g_ppLLLB[i] != NULL)
			{
				delete[] g_ppLLLB[i];
				g_ppLLLB[i] = NULL;
			}
		}
	
		delete[] g_ppLLLB;
		g_ppLLLB = NULL;
	}

	if (g_ppLLLE != NULL)
	{
		for(i=0; i<g_H; i++)
		{
			if (g_ppLLLE[i] != NULL)
			{
				delete[] g_ppLLLE[i];
				g_ppLLLE[i] = NULL;
			}
		}
	
		delete[] g_ppLLLE;
		g_ppLLLE = NULL;
	}

	if (g_ppLLLB2 != NULL)
	{
		for(i=0; i<g_H; i++)
		{
			if (g_ppLLLB2[i] != NULL)
			{
				delete[] g_ppLLLB2[i];
				g_ppLLLB2[i] = NULL;
			}
		}
	
		delete[] g_ppLLLB2;
		g_ppLLLB2 = NULL;
	}

	if (g_ppLLLE2 != NULL)
	{
		for(i=0; i<g_H; i++)
		{
			if (g_ppLLLE2[i] != NULL)
			{
				delete[] g_ppLLLE2[i];
				g_ppLLLE2[i] = NULL;
			}
		}
	
		delete[] g_ppLLLE2;
		g_ppLLLE2 = NULL;
	}

	if (g_ppLN3 != NULL)
	{
		for(i=0; i<g_H; i++)
		{
			if (g_ppLN3[i] != NULL)
			{
				delete[] g_ppLN3[i];
				g_ppLN3[i] = NULL;
			}
		}
	
		delete[] g_ppLN3;
		g_ppLN3 = NULL;
	}
	//-------------

	//-------------
	if (g_pImFF1 != NULL)
	{
		delete[] g_pImFF1;
		g_pImFF1 = NULL;
	}

	if (g_pImVE1 != NULL)
	{
		delete[] g_pImVE1;
		g_pImVE1 = NULL;
	}

	if (g_pImNE1 != NULL)
	{
		delete[] g_pImNE1;
		g_pImNE1 = NULL;
	}

	if (g_pImFF2 != NULL)
	{
		delete[] g_pImFF2;
		g_pImFF2 = NULL;
	}

	if (g_pImVE2 != NULL)
	{
		delete[] g_pImVE2;
		g_pImVE2 = NULL;
	}

	if (g_pImNE2 != NULL)
	{
		delete[] g_pImNE2;
		g_pImNE2 = NULL;
	}

	if (g_pImTEMP1 != NULL)
	{
		delete[] g_pImTEMP1;
		g_pImTEMP1 = NULL;
	}

	if (g_pImTEMP2 != NULL)
	{
		delete[] g_pImTEMP2;
		g_pImTEMP2 = NULL;
	}

	if (g_pImTEMP3 != NULL)
	{
		delete[] g_pImTEMP3;
		g_pImTEMP3 = NULL;
	}

	if (g_ImRES10 != NULL)
	{
		delete[] g_ImRES10;
		g_ImRES10 = NULL;
	}

	if (g_ImRES11 != NULL)
	{
		delete[] g_ImRES11;
		g_ImRES11 = NULL;
	}

	if (g_ImRES12 != NULL)
	{
		delete[] g_ImRES12;
		g_ImRES12 = NULL;
	}
	//-------------
}

void RGB_to_YUV(int *ImIn, int *ImY,int *ImU,int *ImV, int w, int h)
{
	u8 *color;
	int i, r, g, b, y, u, v;

	for(i=0; i<w*h; i++)
	{
		color = (u8*)(&ImIn[i]);

		r = color[2];
		g = color[1];
		b = color[0];
		
		//---(0.299*2^16)(0.587*2^16)(0.114*2^16)-----
		y = 19595*r + 38470*g + 7471*b;

		//-(-0.147*2^16)(-0.289*2^16)(0.436*2^16)-----      
		u = -9634*r - 18940*g + 28574*b;
		
		//---(0.615*2^16)(-0.515*2^16)(-0.100*2^16)---
		v = 40305*r - 33751*g - 6554*b;
		
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

void RGB_to_YIQ(int *ImIn, int *ImY,int *ImI,int *ImQ, int w, int h)
{
	u8 *color;
	int i, r, g, b, Y, I, Q;

	for(i=0; i<w*h; i++)
	{
		color = (u8*)(&ImIn[i]);

		r = color[2];
		g = color[1];
		b = color[0];
		
		//---(0.29889531*2^16)(0.58662247*2^16)(0.11448223*2^16)-----
		Y = 19588*r + 38445*g + 7503*b;

		//---(0.59597799*2^16)(-0.27417610*2^16)(-0.32180189*2^16)-----      
		I = 39058*r - 17968*g - 21090*b;
		
		//---(0.21147017*2^16)(-0.52261711*2^16)(0.31114694*2^16)---
		Q = 13859*r - 34250*g + 20391*b;
		
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
}

void RGB_to_YIQ(int *ImRGB, s64 *ImYIQ, int w, int h)
{
	u8 *color;
	int i;
	int r, g, b, Y, I, Q;
	int *pImRGB = ImRGB;
	__int64 *pImYIQ = ImYIQ;
	__int64 val = 0;
	short *pval = (short*)(&val);

	for(i=0; i<w*h; i++, pImRGB++, pImYIQ++)
	{
		color = (u8*)pImRGB;

		r = color[2];
		g = color[1];
		b = color[0];
		
		//---(0.29889531*2^16)(0.58662247*2^16)(0.11448223*2^16)-----
		Y = 19588*r + 38445*g + 7503*b;

		//---(0.59597799*2^16)(-0.27417610*2^16)(-0.32180189*2^16)-----      
		I = 39058*r - 17968*g - 21090*b;
		
		//---(0.21147017*2^16)(-0.52261711*2^16)(0.31114694*2^16)---
		Q = 13859*r - 34250*g + 20391*b;
		
		Y = Y >> 16;
		
		if (I >= 0)
		{
			I = I >> 16;
		}
		else
		{
			I = -((-I) >> 16);
		}
		
		if (Q >= 0)
		{
			Q = Q >> 16;
		}
		else
		{
			Q = -((-Q) >> 16);
		}

		pval[0] = Y;
		pval[1] = I;
		pval[2] = Q;
		
		*pImYIQ = val;
	}
}

void RGB_to_HSL(int *ImIn, int *ImH,int *ImS,int *ImL, int w, int h)
{
}

void GetGrayscaleImage(int *ImIn, int *ImY, int w, int h)
{
	u8 *color;
	int i, r, g, b, wh;

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

void SobelMEdge(int *ImIn, int *ImMOE, int w, int h)
{
	int i, x, y, mx, my, val, val1, val2, val3, val4, max;

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

void ImprovedSobelMEdge(int *ImIn, int *ImMOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2, val3, val4, max;
	int* pIm = ImIn;
	int* pImMOE = ImMOE;

	mx = w-1;
	my = h-1;
	pIm += w+1;
	pImMOE += w+1;
	for(y=1; y<my; y++, pIm += 2, pImMOE += 2)
	for(x=1; x<mx; x++, pIm++, pImMOE++)
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
		val = 3*(val1 + val2) + 10*val3;
		if (val < 0) max = -val;
		else max = val;

		//val = lt - rt + lb - rb + 2*(lm-rm);
		val = 3*(val1 - val2) + 10*val4;
		if (val < 0) val = -val;
		if (max < val) max = val;

		//val = mt + lm - rm - mb + 2*(lt-rb);
		val = 3*(val3 + val4) + 10*val1;
		if (val < 0) val = -val;
		if (max < val) max = val;

		//val = mt + rm - lm - mb + 2*(rt-lb);
		val = 3*(val3 - val4) + 10*val2;
		if (val < 0) val = -val;
		if (max < val) max = val;

		*pImMOE = max;
	}
}

void ImprovedSobelAllEdge_MMX_SSE(s64 *ImYIQ, int *ImMOE1, int *ImMOE2, int *ImVOE, int *ImNOE, int *ImHOE, double vthr, double nthr, double hthr, int w, int h)
{
	int x, y, mx, my;
	int vth1, vth2, nth1, nth2, hth1, hth2;
	__m64 voe, noe, hoe, soe, moe;
	__m64 mmx1, mmx2, mmx3, mmx4, mmx5, mmx6, mmx7, mmx8;
	__m64 mmx_0, mmx_3, mmx_10, mmx_255;
	__m64 thr1, thr2;
	__m64 max1, max2; 
	s64* pImYIQ;
	s64* pImRES1;
	s64* pImRES2;
	int* pImMOE1;
	int* pImMOE2;
	int* pImVOE;
	int* pImNOE;
	int* pImHOE;

	mmx_0 = _mm_setzero_si64();
	mmx_3.m64_i64 = 0x0003000300030003;
	mmx_10.m64_i64 = 0x000a000a000a000a;
	mmx_255.m64_i64 = 0x00ff00ff00ff00ff;
	
	max1 = mmx_0;
	max2 = mmx_0;

	mx = w-1;
	my = h-1;
	pImYIQ = ImYIQ;
	pImRES1 = g_ImRES1_64;
	pImRES2 = g_ImRES2_64;
	pImYIQ += w+1;
	pImRES1 += w+1;
	pImRES2 += w+1;
	for(y=1; y<my; y++, pImYIQ += 2, pImRES1 += 2, pImRES2 += 2)
	for(x=1; x<mx; x++, pImYIQ++, pImRES1++, pImRES2++)
	{
		mmx1.m64_i64 = *(pImYIQ - w - 1);
		mmx2.m64_i64 = *(pImYIQ + w + 1);
		mmx3.m64_i64 = *(pImYIQ - w + 1);
		mmx4.m64_i64 = *(pImYIQ + w - 1);
		mmx5.m64_i64 = *(pImYIQ - w);
		mmx6.m64_i64 = *(pImYIQ + w);
		mmx7.m64_i64 = *(pImYIQ - 1);
		mmx8.m64_i64 = *(pImYIQ + 1);

		//val1 = *(pIm - w - 1) - *(pIm + w + 1);
		mmx1 = _mm_subs_pi16(mmx1, mmx2);
		//val2 = *(pIm - w + 1) - *(pIm + w - 1);
		mmx2 = _mm_subs_pi16(mmx3, mmx4);
		//val3 = *(pIm - w) - *(pIm + w);
		mmx3 = _mm_subs_pi16(mmx5, mmx6);
		//val4 = *(pIm - 1) - *(pIm + 1);
		mmx4 = _mm_subs_pi16(mmx7, mmx8);

		mmx5 = _mm_mullo_pi16(mmx1, mmx_10);
		mmx6 = _mm_mullo_pi16(mmx2, mmx_10);
		mmx7 = _mm_mullo_pi16(mmx3, mmx_10);
		mmx8 = _mm_mullo_pi16(mmx4, mmx_10);

		mmx1 = _mm_mullo_pi16(mmx1, mmx_3);
		mmx2 = _mm_mullo_pi16(mmx2, mmx_3);
		mmx3 = _mm_mullo_pi16(mmx3, mmx_3);
		mmx4 = _mm_mullo_pi16(mmx4, mmx_3);

		//voe = 3*(val1 - val2) + 10*val4;
		voe = _mm_adds_pi16(_mm_subs_pi16(mmx1, mmx2), mmx8);
		//if (voe < 0) voe = -voe;
		voe = _mm_max_pi16(_mm_sub_pi16(mmx_0, voe), voe);
		moe = voe;

		//noe = 3*(val3 + val4) + 10*val1;
		noe = _mm_adds_pi16(_mm_adds_pi16(mmx3, mmx4), mmx5);
		//if (noe < 0) noe = -noe;
		noe = _mm_max_pi16(_mm_sub_pi16(mmx_0, noe), noe);
		moe  = _mm_max_pi16(moe, noe);

		//hoe = 3*(val1 + val2) + 10*val3;
		hoe = _mm_adds_pi16(_mm_adds_pi16(mmx1, mmx2), mmx7);
		//if (hoe < 0) hoe = -hoe;
		hoe = _mm_max_pi16(_mm_sub_pi16(mmx_0, hoe), hoe);
		moe  = _mm_max_pi16(moe, hoe);

		//soe = 3*(val3 - val4) + 10*val2;
		soe = _mm_adds_pi16(_mm_subs_pi16(mmx3, mmx4), mmx6);
		//if (soe < 0) soe = -soe;
		soe = _mm_max_pi16(_mm_sub_pi16(mmx_0, soe), soe);
		moe  = _mm_max_pi16(moe, soe);

		mmx1.m64_i16[0] = moe.m64_i16[0];
		mmx1.m64_i16[1] = voe.m64_i16[0];
		mmx1.m64_i16[2] = noe.m64_i16[0];
		mmx1.m64_i16[3] = hoe.m64_i16[0];

		mmx2.m64_i16[0] = moe.m64_i16[1];
		mmx2.m64_i16[1] = voe.m64_i16[1];
		mmx2.m64_i16[2] = noe.m64_i16[1];
		mmx2.m64_i16[3] = hoe.m64_i16[1];

		mmx3.m64_i16[0] = moe.m64_i16[2];
		mmx3.m64_i16[1] = voe.m64_i16[2];
		mmx3.m64_i16[2] = noe.m64_i16[2];
		mmx3.m64_i16[3] = hoe.m64_i16[2];

		//_val4 = _val1 + _val2 + _val3;
		//_val5 = _val1 + (_val2 + _val3)*5;
		mmx3 = _mm_adds_pi16(mmx3, mmx2);
		mmx2 = _mm_adds_pi16(mmx1, mmx3);
		mmx4 = _mm_adds_pi16(mmx1, _mm_adds_pi16(_mm_slli_pi16(mmx3, 2), mmx3));

		max1 = _mm_max_pi16(max1, mmx2);
		max2 = _mm_max_pi16(max2, mmx4);

		//*pImRES = (__int64)moe | ((__int64)noe << 16) | ((__int64)voe << 32) | ((__int64)hoe << 48); 
		*pImRES1 = mmx2.m64_i64;
		*pImRES2 = mmx4.m64_i64;		
	}

	vth1 = max1.m64_i16[1];
	nth1 = max1.m64_i16[2];
	hth1 = max1.m64_i16[3];

	vth2 = max2.m64_i16[1];
	nth2 = max2.m64_i16[2];
	hth2 = max2.m64_i16[3];

	_mm_empty();

	vth1 = (int)((double)vth1*vthr)-1;
	vth2 = (int)((double)vth2*vthr)-1;
	
	nth1 = (int)((double)nth1*nthr)-1;
	nth2 = (int)((double)nth2*nthr)-1;

	hth1 = (int)((double)hth1*hthr)-1;
	hth2 = (int)((double)hth2*hthr)-1;

	thr1.m64_i16[0] = 0;
	thr1.m64_i16[1] = vth1;
	thr1.m64_i16[2] = nth1;
	thr1.m64_i16[3] = hth1;

	thr2.m64_i16[0] = 0;
	thr2.m64_i16[1] = vth2;
	thr2.m64_i16[2] = nth2;
	thr2.m64_i16[3] = hth2;

	mx = w-1;
	my = h-1;
	pImRES1 = g_ImRES1_64;
	pImRES2 = g_ImRES2_64;
	pImMOE1 = ImMOE1;
	pImMOE2 = ImMOE2;
	pImVOE = ImVOE;
	pImNOE = ImNOE;
	pImHOE = ImHOE;
	pImRES1 += w+1;
	pImRES2 += w+1;
	pImMOE1 += w+1;
	pImMOE2 += w+1;
	pImVOE += w+1;
	pImNOE += w+1;
	pImHOE += w+1;
	for(y=1; y<my; y++, pImRES1 += 2, pImRES2 += 2, pImMOE1 += 2, pImMOE2 += 2, pImVOE += 2, pImNOE += 2, pImHOE += 2)
	for(x=1; x<mx; x++, pImRES1++, pImRES2++, pImMOE1++, pImMOE2++, pImVOE++, pImNOE++, pImHOE++)
	{
		mmx1.m64_i64 = *pImRES1;
		mmx2.m64_i64 = *pImRES2;

		*pImMOE1 = mmx1.m64_i16[0];
		*pImMOE2 = mmx2.m64_i16[0];

		mmx1 = _mm_cmpgt_pi16(mmx1, thr1);
		mmx2 = _mm_cmpgt_pi16(mmx2, thr2);
		mmx3 = _mm_and_si64(_mm_or_si64(mmx1, mmx2), mmx_255);

		*pImVOE = mmx3.m64_i16[1];
		*pImNOE = mmx3.m64_i16[2];
		*pImHOE = mmx3.m64_i16[3];
	}

	_mm_empty();
}

void SobelHEdge(int *ImIn, int *ImHOE, int w, int h)
{
	int i, ii, x, y, mx, my, val;

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

void FastImprovedSobelHEdge(int *ImIn, int *ImHOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2;
	int* pIm = ImIn;
	int* pImHOE = ImHOE;

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

void FastSobelVEdge(int *ImIn, int *ImVOE, int w, int h)
{
	int i, ii, x, y, mx, my, val;

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

void FastImprovedSobelVEdge(int *ImIn, int *ImVOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2, val3;
	int* pIm = ImIn;
	int* pImVOE = ImVOE;

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

void FullSobelVEdge(int *ImIn, int *ImVOE1, int *ImVOE2, int w, int h)
{
	int i, ii, x, y, mx, my, val, size, hvt, rhvt, MX;

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

void SobelVEdge(int *ImIn, int *ImVOE, int w, int h)
{
	int i, ii, x, y, mx, my, val, size, hvt, rhvt, MX;

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

void SobelNEdge(int *ImIn, int *ImNOE, int w, int h)
{
	int i, j, k;
	int	mx, val;
	int lt,mt,rt, lm,mm,rm, lb,mb,rb;
	int blt,bmt,brt, blm,bmm,brm, blb,bmb,brb;

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

void FastImprovedSobelNEdge(int *ImIn, int *ImNOE, int w, int h)
{
	int x, y, mx, my, val, val1, val2;
	int* pIm = ImIn;
	int* pImNOE = ImNOE;

	mx = w-1;
	my = h-1;
	pIm += w+1;
	pImNOE += w+1;
	for(y=1; y<my; y++, pIm += 2, pImNOE += 2)
	for(x=1; x<mx; x++, pIm++, pImNOE++)
	{
		val1 = *(pIm - w);
		val2 = *(pIm - w - 1);

		val1 += *(pIm - 1) - *(pIm + 1);

		val1 -= *(pIm + w);
		val2 -= *(pIm + w + 1);

		val = 3*val1 + 10*val2;

		if (val<0) val = -val;
		*pImNOE = val;
	}
}

void SobelSEdge(int *ImIn, int *ImSOE, int w, int h)
{
	int i, j, k;
	int	mx, val;
	int lt,mt,rt, lm,mm,rm, lb,mb,rb;
	int blt,bmt,brt, blm,bmm,brm, blb,bmb,brb;

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

void IncreaseContrastOperator(int *ImIn, int *ImRES, int w, int h)
{
	int i, ii, x, y, mx, my, val;

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

void CEDOperator(int *ImY, int *ImI, int *ImQ, int *ImCED, int w, int h)
{
	int i, x, y, mx, my, l1, l2, l3;
	int val, dy1, dy2, di1, di2, dq1, dq2; 

	l1 = 1; //?
	l2 = 1; //?
	l3 = 1; //?

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		dy1 = ImY[i]-ImY[i+w+1];
		dy2 = ImY[i+1]-ImY[i+w];

		di1 = ImI[i]-ImI[i+w+1];
		di2 = ImI[i+1]-ImI[i+w];

		dq1 = ImQ[i]-ImQ[i+w+1];
		dq2 = ImQ[i+1]-ImQ[i+w];

		val = l1*(dy1*dy1 + dy2*dy2) + l2*(di1*di1 + di2*di2) + l3*(dq1*dq1 + dq2*dq2);
		ImCED[i] = (int)sqrt((float)val);
	}
}

void FindAndApplyGlobalThreshold(int *Im, int w, int h)
{
	int i, imx, mx, start=3, dx=5;
	int beg, end, val, MX, thr;

	MX = 0;

	for(i=0; i<w*h; i++)
	{
		val = Im[i];
		
		if (val > MX) 
		{
			MX = val;			
		}

		g_edgeStr[val]++;
	}

	imx = start;
	mx = g_edgeStr[imx];
	for(i=start; i<=20; i++)
	{
		if(g_edgeStr[i] > mx) 
		{
			imx = i;
			mx = g_edgeStr[i];
		}
	}

	beg = imx-dx;
	if (beg<start) beg = start;
	end = beg+2*dx; 

	val = 0;
	for(i=beg; i<=end; i++)
	{
		val += g_edgeStr[i]; 
	}
	val /= (end-beg+1);
	val = (9*val)/10;

	i = imx+1;

	while((g_edgeStr[i] > val) && (i < MX)) i++;
	thr = i;

	for(i=0; i<w*h; i++)
	{
		if (Im[i]<thr) Im[i] = 0;
	}

	memset(g_edgeStr, 0, (MX+1)*sizeof(int));
}

void FindAndApplyLocalThresholding(int *Im, int dw, int dh, int w, int h)
{
	int i, di, ia, da, x, y, nx, ny, mx, my, MX;
	int val, min, max, mid, lmax, rmax, li, ri, thr;
	
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

			g_edgeStr[val]++;			
		}
		mid = (min+max)/2;

		li = min;
		lmax = g_edgeStr[li];
		for(i=min; i<mid; i++)
		{
			if (g_edgeStr[i]>lmax)
			{
				li = i;
				lmax = g_edgeStr[li];
			}
		}

		ri = mid;
		rmax = g_edgeStr[ri];
		for(i=mid; i<=max; i++)
		{
			if (g_edgeStr[i]>rmax)
			{
				ri = i;
				rmax = g_edgeStr[ri];
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
			if (g_edgeStr[i]<val) 
			{
				thr = i;
				val = g_edgeStr[i];
			}
		}

		i = ia;
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			if (Im[i]<thr) Im[i] = 0;
		}

		memset(g_edgeStr, 0, (MX+1)*sizeof(int));
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
			
			g_edgeStr[val]++;			
		}
		mid = (min+max)/2;

		li = min;
		lmax = g_edgeStr[li];
		for(i=min; i<mid; i++)
		{
			if (g_edgeStr[i]>lmax)
			{
				li = i;
				lmax = g_edgeStr[li];
			}
		}

		ri = mid;
		rmax = g_edgeStr[ri];
		for(i=mid; i<=max; i++)
		{
			if (g_edgeStr[i]>rmax)
			{
				ri = i;
				rmax = g_edgeStr[ri];
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
			if (g_edgeStr[i]<val) 
			{
				thr = i;
				val = g_edgeStr[i];
			}
		}

		i = ia;
		for(y=0; y<dh; y++, i+=di)
		for(x=0; x<dw; x++, i++)
		{
			if (Im[i]<thr) Im[i] = 0;
		}
		
		memset(g_edgeStr, 0, (MX+1)*sizeof(int));
	}
}

void ApplyModerateThreshold(int *Im, double mthr, int w, int h)
{
	int thr;
	int mx = 0;
	int *pIm = Im;
	int *pImMAX = pIm + w*h;
	
	for(; pIm < pImMAX; pIm++)
	{
		if (*pIm > mx) mx = *pIm;
	}

	thr = (int)((double)mx*mthr);

	for(pIm = Im; pIm < pImMAX; pIm++)
	{
		if (*pIm < thr) *pIm = 0;
		else *pIm = 255;
	}
}

void ApplyModerateThreshold_MMX_SSE(int *Im, double mthr, int w, int h)
{
	int thr;
	int mx;
	__m64 mmx1, mmx2, mmx3 = _mm_setzero_si64();
	__int64 *pIm64 = (__int64*)Im;
	__int64 *pImMAX64 = pIm64 + (w*h)/2;
	int *pIm = Im;
	int *pImMAX = pIm + w*h;
	
	while(pIm64 < pImMAX64)
	{
		mmx1.m64_i64 = *pIm64;
		pIm64++;
		mmx2.m64_i64 = *pIm64;
		pIm64++;
		
		mmx1 = _mm_packs_pi32(mmx1, mmx2);
		mmx3 = _mm_max_pi16(mmx3, mmx1);
	}
	
	mx = mmx3.m64_i16[0];
	if (mmx3.m64_i16[1] > mx) mx = mmx3.m64_i16[1];
	if (mmx3.m64_i16[2] > mx) mx = mmx3.m64_i16[2];
	if (mmx3.m64_i16[3] > mx) mx = mmx3.m64_i16[3];

	_mm_empty();

	thr = (int)((double)mx*mthr);

	for(pIm = Im; pIm < pImMAX; pIm++)
	{
		if (*pIm < thr) *pIm = 0;
		else *pIm = 255;
	}
}

void AplyESS(int *ImIn, int* ImOut, int w, int h)
{
	int i, x, y, mx, my, val;

	mx = w-2;
	my = h-2;
	i = (w+1)*2;
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		val = 2*(ImIn[i - w*2 - 2] + ImIn[i - w*2 + 2] + ImIn[i + w*2 - 2] + ImIn[i + w*2 + 2]) +
			+ 4*(ImIn[i - w*2 - 1] + ImIn[i - w*2 + 1] + ImIn[i - w - 2] + ImIn[i - w + 2] + ImIn[i + w - 2] + ImIn[i + w + 2] + ImIn[i + w*2 - 1] + ImIn[i + w*2 + 1]) +
			+ 5*(ImIn[i - w*2] + ImIn[i - 2] + ImIn[i + 2] + ImIn[i + w*2]) +
			+ 10*(ImIn[i - w - 1] + ImIn[i - w + 1] + ImIn[i + w - 1] + ImIn[i + w + 1]) +
			+ 20*(ImIn[i - w] + ImIn[i - 1] + ImIn[i + 1] + ImIn[i + w]) +
			+ 40*ImIn[i];

		ImOut[i] = val/220;
	}
}

void AplyECP(int *ImIn, int* ImOut, int w, int h)
{
	int i, ii, x, y, mx, my, val;

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		if(ImIn[i] == 0)
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

		ii = i - ((w+1)<<1);
		val = 8*ImIn[ii] + 5*ImIn[ii+1] + 4*ImIn[ii+2] + 5*ImIn[ii+3] + 8*ImIn[ii+4];

		ii += w;
		val += 5*ImIn[ii] + 2*ImIn[ii+1] + ImIn[ii+2] + 2*ImIn[ii+3] + 5*ImIn[ii+4];

		ii += w;
		val += 4*ImIn[ii] + ImIn[ii+1] + ImIn[ii+3] + 4*ImIn[ii+4];

		ii += w;
		val += 5*ImIn[ii] + 2*ImIn[ii+1] + ImIn[ii+2] + 2*ImIn[ii+3] + 5*ImIn[ii+4];

		ii += w;
		val += 8*ImIn[ii] + 5*ImIn[ii+1] + 4*ImIn[ii+2] + 5*ImIn[ii+3] + 8*ImIn[ii+4];

		ImOut[i] = val/100;
	}
}

void ColorFiltration(int *Im, int *LB, int *LE, int &N, int w, int h)
{
	int *line=g_pL2, *lb=g_pLB2, *le=g_pLE2;
	int r0, g0, b0, r1, g1, b1;
	int scd, segw, msegc, i, ib, ia, mi, cnt;
	int dif, rdif, gdif, bdif;
	int y, nx, mx, val;
	int sbegin, n, k;
	u8 *color;

	memset(line, 0, h*sizeof(int));

	scd = g_scd;
	segw = g_segw;
	msegc = g_msegc;

	mx = w/segw;

	for(y=0, ib=0; y<h; y++, ib+=w)
	{
		cnt = 0;

		for(nx=0, ia=ib; nx<mx; nx++, ia+=segw)
		{
			color = (u8*)(&Im[ia]);
			r0 = color[2];
			g0 = color[1];
			b0 = color[0];	

			mi = ia+segw;
			dif = 0;
			
			for(i=ia+1; i<=mi; i++)
			{
				color = (u8*)(&Im[i]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	
				
				rdif = r1-r0;
				if (rdif<0) rdif = -rdif;

				gdif = g1-g0;
				if (gdif<0) gdif = -gdif;

				bdif = b1-b0;
				if (bdif<0) bdif = -bdif;

				dif += rdif+gdif+bdif;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}

			if (dif>=scd) cnt++;
			else cnt = 0;

			if (cnt==msegc) 
			{
				line[y] = 1;
				break;
			}
		}
	}

	n = 0;
	sbegin = 1; //searching begin
	for(y=0; y<h; y++)
	{
		if (line[y]==1)
		{
			if (sbegin==1) 
			{
				lb[n] = y;
				sbegin = 0;
			}	
		}
		else
		{
			if (sbegin==0)
			{
				le[n] = y-1;
				sbegin = 1;
				n++;
			}
		}
	}
	if (sbegin==0)
	{
		le[n] = y-1;
		n++;
	}
	
	if (n==0) 
	{
		N = 0;
		return;
	}


	int dd, bd, md;

	dd = 6;
	bd = 2*dd+1; 
	md = 2+dd;

	k = 0;
	val = lb[0] - dd; 
	if (val<0) val = 0;
	LB[0] = val;

	for(i=0; i<n-1; i++)
	{
		if ((lb[i+1]-le[i])>bd) 
		{
			if ((le[i]-LB[k])>=md) 
			{
				LE[k] = le[i] + dd;
				k++; 
				LB[k] = lb[i+1] - dd;
			}
			else
			{
				LB[k] = lb[i+1] - dd;
			}
		}
	}
	if ((le[i]-LB[k])>=md)
	{
		val = le[i] + dd;
		if (val>h-1) val = h-1;
		LE[k] = val;
		k++; 
	}

	N = k;
}

void ColorFiltration2(int *Im, int *ImRES, int w, int h, int scd)
{
	int r0, g0, b0, r1, g1, b1;
	int segw, i, ib, ia, mi;
	int dif, rdif, gdif, bdif;
	int y, x, mx;
	u8 *color;

	memset(ImRES, 0, (w*h)*sizeof(int));

	segw = g_segw;

	mx = w-segw;

	for(y=0, ib=0; y<h; y++, ib+=w)
	{
		for(x=0, ia=ib; x<mx; x++, ia++)
		{
			color = (u8*)(&Im[ia]);
			r0 = color[2];
			g0 = color[1];
			b0 = color[0];	

			mi = ia+segw;
			dif = 0;
			
			for(i=ia+1; i<=mi; i++)
			{
				color = (u8*)(&Im[i]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	
				
				rdif = r1-r0;
				if (rdif<0) rdif = -rdif;

				gdif = g1-g0;
				if (gdif<0) gdif = -gdif;

				bdif = b1-b0;
				if (bdif<0) bdif = -bdif;

				dif += rdif+gdif+bdif;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}

			if (dif>=scd) 
			{	
				for(i=ia+1; i<=mi; i++)
				{
					ImRES[i] = 255;
				}
			}
		}
	}
}

void BorderClear(int *Im, int dd, int w, int h)
{
	int i, di, x, y;

	memset(Im, 0, w*dd*sizeof(int));
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

void EasyBorderClear(int *Im, int w, int h)
{
	int i, y;

	memset(Im, 0, w*sizeof(int));
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

void CombineTwoImages(int *Im1, int *Im2, int w, int h)
{
	int i, size;

	size = w*h;
	for(i=0; i<size; i++) 
	{
		if (Im2[i] != 0)
		{
			Im1[i] = 255;
		}
	}
}

void IntersectTwoImages(int *Im1, int *Im2, int w, int h)
{
	int i, size;

	size = w*h;
	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 0)
		{
			Im1[i] = 0;
		}
	}
}

void CombineStrengthOfTwoImages(int *Im1, int *Im2, int w, int h)
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

void GetFirstFilteredImage(int *ImRGB, int *ImSF, int *ImRES, int w, int h, int xb, int xe, int yb, int ye)
{
	int i, ib, x, y, mx, my,MX, S, SS, thr, size;

	RGB_to_YIQ(ImRGB, g_ImY, g_ImU, g_ImV, w, h);

	EasyBorderClear(g_ImCMOE, w, h);
	BorderClear(ImRES, 2, w, h);
	BorderClear(g_ImRES2, 2, w, h);
	BorderClear(g_ImRES3, 2, w, h);
	BorderClear(g_ImRES4, 2, w, h);
	

	ImprovedSobelMEdge(g_ImY, g_ImYMOE, w, h);
	ImprovedSobelMEdge(g_ImU, g_ImUMOE, w, h);
	ImprovedSobelMEdge(g_ImV, g_ImVMOE, w, h);

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, 32, h, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES3[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}
	ResizeGrayscaleImage4x(g_ImRES3, g_ImRES4, w, h);
	//ApplyModerateThreshold(g_ImRES4, mthr, w*4, h*4);

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, 32, h, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES3[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}
	ResizeGrayscaleImage4x(g_ImRES3, ImRES, w, h);
	//ApplyModerateThreshold(ImRES, mthr, w*4, h*4);
	
	w *= 4;
	h *= 4;
	CombineStrengthOfTwoImages(ImRES, g_ImRES4, w, h);
	
	SS = 0;
	for (y=yb, ib=yb*w+xb; y<ye; y++, ib+=w)
	for (x=xb, i=ib; x<xe; x++, i++)
	{
		if (ImSF[i] == 255)
		{
			SS++;
		}
	}

	MX = 0;
	for (y=yb, ib=yb*w+xb; y<ye; y++, ib+=w)
	for (x=xb, i=ib; x<xe; x++, i++)
	{
		g_edgeStr[ImRES[i]]++;
		if (ImRES[i] > MX) MX = ImRES[i];
	}

	S = 0; 
	for (i=MX; i>0; i--)
	{
		S += g_edgeStr[i];
		if (S > SS/2) break;
	}
	thr = i+1;

	size = w*h;
	for(i=0; i<size; i++)
	{
		if (ImRES[i] >= thr)
		{
			ImRES[i] = 255;
		}
		else
		{
			ImRES[i] = 0;
		}
	}

	memset(g_edgeStr, 0, (MX+1)*sizeof(int));
}

int GetTransformedImage(int *ImRGB, int *ImFF, int *ImSF, int *ImTF, int *ImVE, int *ImNE, int *ImHE, int W, int H)
{
	int i, k, cnt, val, N;
	int x, y, mx, my, segh;
	int w, h;
	int res;
	clock_t t1, t2;

	//ColorFiltration2(ImRGB, g_ImRES1, W, H, 600);
	//g_pMF->m_pImageBox->g_pViewImage(g_ImRES1, W, H);
	//return;

	t1 = clock();

	res = 0;

	ColorFiltration(ImRGB, g_pLB, g_pLE, N, W, H);

	ImVE[0] = -1;
	ImNE[0] = -1;
	ImHE[0] = -1;
	ImFF[0] = -1;
	ImSF[0] = -1;
	ImTF[0] = -1;

	if (N == 0) 
	{	
		return res;
	}

	memset(ImFF, 0, W*H*sizeof(int));	

	w = W;
	i = 0;
	h = 0;
	for(k=0; k<N; k++)
	{
		h += g_pLE[k]-g_pLB[k]+1;
		cnt = W*(g_pLE[k]-g_pLB[k]+1);
		memcpy(&g_Im[i], &ImRGB[W*g_pLB[k]], cnt*sizeof(int));
		i += cnt;
	}

	RGB_to_YIQ(g_Im, g_ImY, g_ImU, g_ImV, w, h);

	EasyBorderClear(g_ImCMOE, w, h);
	BorderClear(g_ImRES1, 2, w, h);
	BorderClear(g_ImRES2, 2, w, h);
	BorderClear(g_ImRES3, 2, w, h);
	BorderClear(g_ImRES4, 2, w, h);

	ImprovedSobelMEdge(g_ImY, g_ImYMOE, w, h);
	ImprovedSobelMEdge(g_ImU, g_ImUMOE, w, h);
	ImprovedSobelMEdge(g_ImV, g_ImVMOE, w, h);

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, w, 32, w, h);
	//FindAndApplyLocalThresholding(g_ImCMOE, 32, h, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES4[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold(&g_ImRES4[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}
	//ApplyModerateThreshold(g_ImRES4, g_mthr, w, h);

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, w, 32, w, h);
	//FindAndApplyLocalThresholding(g_ImCMOE, 32, h, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES1[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold(&g_ImRES1[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}
	//ApplyModerateThreshold(g_ImRES1, g_mthr, w, h);
	CombineTwoImages(g_ImRES1, g_ImRES4, w, h);
	
	t1 = clock()-t1;
	t2 = clock();
	
	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		memcpy(&ImFF[W*g_pLB[k]], &g_ImRES1[i], cnt*sizeof(int));
		i += cnt;
	}

	memcpy(ImSF, ImFF, W*H*sizeof(int));

	RGB_to_YIQ(ImRGB, g_ImY, g_ImU, g_ImV, W, H);
	//RGB_to_YUV(ImRGB, g_ImY, g_ImU, g_ImV, W, H);	

	EasyBorderClear(g_ImRES1, W, H);
	EasyBorderClear(ImVE, W, H);
	EasyBorderClear(ImNE, W, H);
	EasyBorderClear(ImHE, W, H);

	FastImprovedSobelVEdge(g_ImY, g_ImYMOE, W, H);
	FastImprovedSobelVEdge(g_ImU, g_ImUMOE, W, H);
	FastImprovedSobelVEdge(g_ImV, g_ImVMOE, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES1[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	ApplyModerateThreshold(g_ImRES1, g_mvthr, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ImVE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	ApplyModerateThreshold(ImVE, g_mvthr, W, H);
	CombineTwoImages(ImVE, g_ImRES1, W, H);

	FastImprovedSobelNEdge(g_ImY, g_ImYMOE, W, H);
	FastImprovedSobelNEdge(g_ImU, g_ImUMOE, W, H);
	FastImprovedSobelNEdge(g_ImV, g_ImVMOE, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES1[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	ApplyModerateThreshold(g_ImRES1, g_mnthr, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ImNE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	ApplyModerateThreshold(ImNE, g_mnthr, W, H);
	CombineTwoImages(ImNE, g_ImRES1, W, H);

	//RGB_to_YIQ(ImRGB, g_ImY, g_ImU, g_ImV, W, H);
	FastImprovedSobelHEdge(g_ImY, g_ImYMOE, W, H);
	FastImprovedSobelHEdge(g_ImU, g_ImUMOE, W, H);
	FastImprovedSobelHEdge(g_ImV, g_ImVMOE, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES1[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	ApplyModerateThreshold(g_ImRES1, g_mhthr, W, H);
	mx = W-1;
	my = H-1;
	i = W+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		ImHE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	ApplyModerateThreshold(ImHE, g_mhthr, W, H);
	CombineTwoImages(ImHE, g_ImRES1, W, H);

	segh = g_segh;
	for(k=0; k<N; k++)
	{
		val = g_pLB[k]%segh;
		g_pLB[k] -= val;
		
		val = g_pLE[k]%segh;
		if (val > 0) val = segh - val;
		if (g_pLE[k] + val < H) g_pLE[k] += val;
	}

    if ((g_pLE[N-1] + g_segh) > H)
	{
		val = g_pLE[N-1]-(H-g_segh);
		g_pLE[N-1] = H-g_segh;

		memset(&ImSF[W*(g_pLE[N-1]+1)], 0, W*val*sizeof(int));
		memset(&ImVE[W*(g_pLE[N-1]+1)], 0, W*val*sizeof(int));		
	}

	t2 = clock()-t2;

	res = SecondFiltration(ImSF, ImRGB, ImVE, ImNE, g_pLB, g_pLE, N, W, H);
	memcpy(ImTF, ImSF, W*H*sizeof(int));

	if (res == 1) res = ThirdFiltration(ImTF, ImVE, ImNE, ImHE, g_pLB, g_pLE, N, W, H);

	if (res == 1) res = SecondFiltration(ImTF, ImRGB, ImVE, ImNE, g_pLB, g_pLE, N, W, H);

	if (res == 1) res = ThirdFiltration(ImTF, ImVE, ImNE, ImHE, g_pLB, g_pLE, N, W, H);

	return res;
}

void FreeImage(int *Im, int* LB, int* LE, int N, int w, int h)
{
	int *LLB=g_pLLB4, *LLE=g_pLLE4;
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

void UnpackImage(int *ImIn, int* ImRES, int *LB, int *LE, int LN, int w, int h)
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

int GetFastTransformedImage(int *ImRGB, int *ImF, int *ImVE, int W, int H)
{
	int i, j, k, cnt, val, N;
	int x, y, mx, my, segh;
	int *LB=g_pLB3, *LE=g_pLE3; 
	int w, h, dh;
	int res;

	/*g_pLB[0] = 0;
	g_pLE[0] = H-1;
	N = 1;*/

	res = 0;
	g_blnVNE = 0;
	g_blnHE = 0;

	ColorFiltration(ImRGB, g_pLB, g_pLE, N, W, H);

	if (N == 0) 
	{	
		return res;
	}	

	/////////////////
	segh = g_segh;
	val = (int)(0.02*(double)H)+1;
	for(k=0; k<N; k++)
	{
		g_pLB[k] -= val;		
		g_pLE[k] += val;

		if (g_pLB[k] < 0) g_pLB[k] = 0;
		if (g_pLE[k] > H-1) g_pLE[k] = H-1;
	}

	i=0;
	while(i < N-1)
	{
		if (g_pLB[i+1] <= g_pLE[i])
		{
			g_pLE[i] = g_pLE[i+1];

			for (j=i+1; j<N-1; j++)
			{
				g_pLB[j] = g_pLB[j+1];
				g_pLE[j] = g_pLE[j+1];
			}

			N--;
			continue;
		}

		i++;
	}

	g_LN = N;
	/////////////////

	w = W;
	i = 0;
	h = 0;
	for(k=0; k<N; k++)
	{
		dh = g_pLE[k]-g_pLB[k]+1;
		LB[k] = h;
		h += dh;
		LE[k] = h-1;
		cnt = W*dh;
		memcpy(&g_Im[i], &ImRGB[W*g_pLB[k]], cnt*sizeof(int));
		i += cnt;
	}

	RGB_to_YIQ(g_Im, g_ImY, g_ImU, g_ImV, w, h);

	EasyBorderClear(g_ImCMOE, w, h);
	BorderClear(g_ImRES1, 2, w, h);
	BorderClear(g_ImRES2, 2, w, h);
	BorderClear(g_ImRES3, 2, w, h);
	BorderClear(g_ImRES4, 2, w, h);
	BorderClear(g_ImRES5, 2, w, h);

	ImprovedSobelMEdge(g_ImY, g_ImYMOE, w, h);
	ImprovedSobelMEdge(g_ImU, g_ImUMOE, w, h);
	ImprovedSobelMEdge(g_ImV, g_ImVMOE, w, h);

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, w, 32, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES5[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold(&g_ImRES5[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}

	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImCMOE[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	
	FindAndApplyGlobalThreshold(g_ImCMOE, w, h);	
	FindAndApplyLocalThresholding(g_ImCMOE, w, 32, w, h);

	AplyESS(g_ImCMOE, g_ImRES2, w, h);
	AplyECP(g_ImRES2, g_ImRES3, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES1[i] = (g_ImRES2[i] + g_ImRES3[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold(&g_ImRES1[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}
	CombineTwoImages(g_ImRES5, g_ImRES1, w, h);
		

	FastImprovedSobelVEdge(g_ImY, g_ImYMOE, w, h);
	FastImprovedSobelVEdge(g_ImU, g_ImUMOE, w, h);
	FastImprovedSobelVEdge(g_ImV, g_ImVMOE, w, h);
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES1[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	ApplyModerateThreshold(g_ImRES1, g_mvthr, w, h);
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES2[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	ApplyModerateThreshold(g_ImRES2, g_mvthr, w, h);
	CombineTwoImages(g_ImRES1, g_ImRES2, w, h);

	
	FastImprovedSobelNEdge(g_ImY, g_ImYMOE, w, h);
	FastImprovedSobelNEdge(g_ImU, g_ImUMOE, w, h);
	FastImprovedSobelNEdge(g_ImV, g_ImVMOE, w, h);
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES2[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
	}
	ApplyModerateThreshold(g_ImRES2, g_mnthr, w, h);
	mx = w-1;
	my = h-1;
	i = w+1;
	for(y=1; y<my; y++, i+=2)
	for(x=1; x<mx; x++, i++)
	{
		g_ImRES3[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
	}
	ApplyModerateThreshold(g_ImRES3, g_mnthr, w, h);
	CombineTwoImages(g_ImRES2, g_ImRES3, w, h);

	//return 1;

	/////////////////
	for(k=0; k<N; k++)
	{
		memset(&g_ImRES5[W*LB[k]], 0, w*sizeof(int));
		memset(&g_ImRES5[W*LE[k]], 0, w*sizeof(int));
		memset(&g_ImRES1[W*LB[k]], 0, w*sizeof(int));
		memset(&g_ImRES1[W*LE[k]], 0, w*sizeof(int));

		LB[k] += 1;		
		LE[k] -= 1;
	}
	/////////////////

	g_blnVNE = 1;

	res = SecondFiltration(g_ImRES5, g_Im, g_ImRES1, g_ImRES2, LB, LE, N, w, h);

	if (res == 1)
	{
		FastImprovedSobelHEdge(g_ImY, g_ImYMOE, w, h);
		FastImprovedSobelHEdge(g_ImU, g_ImUMOE, w, h);
		FastImprovedSobelHEdge(g_ImV, g_ImVMOE, w, h);
		mx = w-1;
		my = h-1;
		i = w+1;
		for(y=1; y<my; y++, i+=2)
		for(x=1; x<mx; x++, i++)
		{
			g_ImRES3[i] = g_ImYMOE[i] + g_ImUMOE[i] + g_ImVMOE[i];
		}
		ApplyModerateThreshold(g_ImRES3, g_mhthr, w, h);
		mx = w-1;
		my = h-1;
		i = w+1;
		for(y=1; y<my; y++, i+=2)
		for(x=1; x<mx; x++, i++)
		{
			g_ImRES4[i] = g_ImYMOE[i] + (g_ImUMOE[i] + g_ImVMOE[i])*5;
		}
		ApplyModerateThreshold(g_ImRES4, g_mhthr, w, h);
		CombineTwoImages(g_ImRES3, g_ImRES4, w, h);

		g_blnHE = 1;
	}

	if (res == 1) res = ThirdFiltrationForGFTI(g_ImRES5, g_ImRES1, g_ImRES2, g_ImRES3, LB, LE, N, w, h);

	if (res == 1) res = SecondFiltration(g_ImRES5, g_Im, g_ImRES1, g_ImRES2, LB, LE, N, w, h);

	if (res == 1) res = ThirdFiltrationForGFTI(g_ImRES5, g_ImRES1, g_ImRES2, g_ImRES3, LB, LE, N, w, h);

	/////////////////
	if (res == 1)
	{
		FreeImage(ImF, g_pLB, g_pLE, N, W, H);
		FreeImage(ImVE, g_pLB, g_pLE, N, W, H);

		i = 0;
		for(k=0; k<N; k++)
		{
			cnt = W*(g_pLE[k]-g_pLB[k]+1);		
			memcpy(&ImF[W*g_pLB[k]], &g_ImRES5[i], cnt*sizeof(int));
			memcpy(&ImVE[W*g_pLB[k]], &g_ImRES1[i], cnt*sizeof(int));
			i += cnt;
		}		
	}
	/////////////////

	return res;
}

int GetVeryFastTransformedImage(int *ImRGB, int *ImF, int *ImVE, int W, int H)
{
	int i, j, k, cnt, val, N;
	int x, y, mx, my, segh;
	int *LB=g_pLB4, *LE=g_pLE4; 
	int w, h, dh;
	int res;

	/*g_pLB[0] = 0;
	g_pLE[0] = H-1;
	N = 1;*/
	
	res = 0;
	g_blnVNE = 0;
	g_blnHE = 0;

	ColorFiltration(ImRGB, g_pLB, g_pLE, N, W, H);

	if (N == 0) 
	{	
		return res;
	}	

	/////////////////
	segh = g_segh;
	val = (int)(0.02*(double)H)+1;
	for(k=0; k<N; k++)
	{
		g_pLB[k] -= val;		
		g_pLE[k] += val;

		if (g_pLB[k] < 0) g_pLB[k] = 0;
		if (g_pLE[k] > H-1) g_pLE[k] = H-1;
	}

	i=0;
	while(i < N-1)
	{
		if (g_pLB[i+1] <= g_pLE[i])
		{
			g_pLE[i] = g_pLE[i+1];

			for (j=i+1; j<N-1; j++)
			{
				g_pLB[j] = g_pLB[j+1];
				g_pLE[j] = g_pLE[j+1];
			}

			N--;
			continue;
		}

		i++;
	}

	g_LN = N;
	/////////////////

	w = W;
	i = 0;
	h = 0;
	for(k=0; k<N; k++)
	{
		dh = g_pLE[k]-g_pLB[k]+1;
		LB[k] = h;
		h += dh;
		LE[k] = h-1;
		cnt = W*dh;
		memcpy(&g_Im[i], &ImRGB[W*g_pLB[k]], cnt*sizeof(int));
		i += cnt;
	}

	RGB_to_YIQ(g_Im, g_ImYIQ, w, h);

	EasyBorderClear(g_ImRES1, w, h);
	EasyBorderClear(g_ImRES4, w, h);
	EasyBorderClear(g_ImRES5, w, h);
	BorderClear(g_ImRES6, 2, w, h);
	BorderClear(g_ImRES7, 2, w, h);
	BorderClear(g_ImRES8, 2, w, h);

	/*memset(g_ImRES1, 0, (w*h)*sizeof(int));
	memset(g_ImRES2, 0, (w*h)*sizeof(int));
	memset(g_ImRES3, 0, (w*h)*sizeof(int));
	memset(g_ImRES4, 0, (w*h)*sizeof(int));
	memset(g_ImRES5, 0, (w*h)*sizeof(int));
	memset(g_ImRES6, 0, (w*h)*sizeof(int));
	memset(g_ImRES7, 0, (w*h)*sizeof(int));*/

	ImprovedSobelAllEdge_MMX_SSE(g_ImYIQ, g_ImRES4, g_ImRES5, g_ImRES1, g_ImRES2, g_ImRES3, g_mvthr, g_mnthr, g_mhthr, w, h);

	//return 1;
	/*ApplyModerateThreshold(g_ImRES5, g_mthr, w, h);
	FreeImage(ImF, g_pLB, g_pLE, N, W, H);
	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		memcpy(&ImF[W*g_pLB[k]], &g_ImRES5[i], cnt*sizeof(int));
		i += cnt;
	}
	return 1;*/

	FindAndApplyGlobalThreshold(g_ImRES4, w, h);	
	FindAndApplyLocalThresholding(g_ImRES4, w, 32, w, h);

	AplyESS(g_ImRES4, g_ImRES6, w, h);
	AplyECP(g_ImRES6, g_ImRES7, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES8[i] = (g_ImRES6[i] + g_ImRES7[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold_MMX_SSE(&g_ImRES8[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}
	
	FindAndApplyGlobalThreshold(g_ImRES5, w, h);	
	FindAndApplyLocalThresholding(g_ImRES5, w, 32, w, h);

	AplyESS(g_ImRES5, g_ImRES6, w, h);
	AplyECP(g_ImRES6, g_ImRES7, w, h);

	mx = w-2;
	my = h-2;
	i = ((w+1)<<1);
	for(y=2; y<my; y++, i+=4)
	for(x=2; x<mx; x++, i++)
	{
		g_ImRES7[i] = (g_ImRES6[i] + g_ImRES7[i])/2; 
	}

	i = 0;
	for(k=0; k<N; k++)
	{
		cnt = W*(g_pLE[k]-g_pLB[k]+1);		
		ApplyModerateThreshold_MMX_SSE(&g_ImRES7[i], g_mthr, W, g_pLE[k]-g_pLB[k]+1);
		i += cnt;
	}
	CombineTwoImages(g_ImRES7, g_ImRES8, w, h);

	//return 1;

	/////////////////
	for(k=0; k<N; k++)
	{
		memset(&g_ImRES7[w*LB[k]], 0, w*sizeof(int));
		memset(&g_ImRES7[w*LE[k]], 0, w*sizeof(int));
		memset(&g_ImRES1[w*LB[k]], 0, w*sizeof(int));
		memset(&g_ImRES1[w*LE[k]], 0, w*sizeof(int));

		LB[k] += 1;		
		LE[k] -= 1;
	}
	/////////////////

	if ((LE[N-1] + g_segh) > H)
	{
		val = LE[N-1]-(H-g_segh);
		LE[N-1] = H-g_segh;

		memset(&g_ImRES7[w*(LE[N-1]+1)], 0, w*val*sizeof(int));
		memset(&g_ImRES1[w*(LE[N-1]+1)], 0, w*val*sizeof(int));		
	}

	g_blnVNE = 1;
	g_blnHE = 1;

	res = SecondFiltration(g_ImRES7, g_Im, g_ImRES1, g_ImRES2, LB, LE, N, w, h);

	if (res == 1) res = ThirdFiltrationForGFTI(g_ImRES7, g_ImRES1, g_ImRES2, g_ImRES3, LB, LE, N, w, h);

	if (res == 1) res = SecondFiltration(g_ImRES7, g_Im, g_ImRES1, g_ImRES2, LB, LE, N, w, h);

	if (res == 1) res = ThirdFiltrationForGFTI(g_ImRES7, g_ImRES1, g_ImRES2, g_ImRES3, LB, LE, N, w, h);

	/////////////////
	if (res == 1)
	{
		FreeImage(ImF, g_pLB, g_pLE, N, W, H);
		FreeImage(ImVE, g_pLB, g_pLE, N, W, H);

		i = 0;
		for(k=0; k<N; k++)
		{
			cnt = W*(g_pLE[k]-g_pLB[k]+1);		
			memcpy(&ImF[W*g_pLB[k]], &g_ImRES7[i], cnt*sizeof(int));
			memcpy(&ImVE[W*g_pLB[k]], &g_ImRES1[i], cnt*sizeof(int));
			i += cnt;
		}		
	}
	/////////////////

	return res;
}

///////////////////////////////////////////////////////////////////////////////

int SecondFiltration(int* Im, int* ImRGB, int* ImVE, int* ImNE, int *LB, int *LE, int N, int w, int h)
{
	int *lb=g_pLB5, *le=g_pLE5;
	int segh, ln;
	int x, y, da, ia, ib, ic, ie, i, k, l, ll, val, val1, val2, offset;
	int bln, res;
	int w_2, dw, dw2;
	double tcpo;
	int mpn;
	double mpd, mpved, mpned;
	int segw, msegc, smcd, sb, ns, ngs;
	int r0, g0, b0, r1, g1, b1;
	int mi, dif, rdif, gdif, bdif;
	u8 *color;
	int nVE = 0;
	int nNE = 0;
	int S = 0;
	int SS = 0;
	int S1, S2;

	res = 0;

	segw = g_segw;
	msegc = g_msegc;
	smcd = g_smcd;

	segh = g_segh;
	w_2 = w/2;
	dw = (int)(g_btd*(double)g_W);
	dw2 = (int)(g_tco*(double)g_W);
	tcpo = g_tcpo;

	mpn = g_mpn;
	mpd = g_mpd;
	mpved = g_mpved;
	mpned = g_mpned;
	
	da = segh*w;

	for(k=0; k<N; k++)
	{
		ia = LB[k]*w;
		ie = LE[k]*w;

		for(; ia<=ie;  ia+=da)
		{
			l = 0;
			bln = 0;

			// searching segments
			for(x=0; x<w; x++)
			{
				for(y=0, i=ia+x; y<segh; y++, i+=w)
				{	
					if(Im[i] == 255) 
					{
						if(bln == 0)
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

				if(bln == 1)
				if(le[l] != x)
				{
					bln = 0;
					l++;
				}
			}
			if(bln == 1)
			if(le[l] == w-1) 
			{
				l++;
			}
			ln = l;

			if (ln==0) continue;

			l=0;
			while(l<ln-1)
			{
				//проверяем расстояние между соседними подстроками
				if ((lb[l+1]-le[l])>dw)
				{
					//определяем подстроку наиболее удаленую от центра
					val1 = lb[l]+le[l]-w;
					val2 = lb[l+1]+le[l+1]-w;
					if (val1<0) val1 = -val1;
					if (val2<0) val2 = -val2;
					
					if (val1>val2) ll = l;
					else ll = l+1;
					
					//удаляем наиболее удаленую подстроку
					val = (le[ll]-lb[ll]+1)*sizeof(int);
					for(y=0, i=ia+lb[ll]; y<segh; y++, i+=w)
					{
						memset(&Im[i], 0, val);
					}

					for(i=ll; i<ln-1; i++)
					{
						lb[i]=lb[i+1];
						le[i]=le[i+1];
					}

					ln--;

					if (ll==l) if (l>0) l--;

					continue;
				}

				l++;
			}

			if(ln==0) continue;

			// есть не удаленые подстроки
			
			// потенциальный текст не расположен в обоих половинах изображения ?
			if ((lb[0]>=w_2)||(le[ln-1]<=w_2))
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}
				
				continue;
			}

			offset = le[ln-1]+lb[0]-w;			
			if (offset<0) offset = -offset;
			
			// потенциальный текст слишком сильно сдвинут от центра изображения ?
			if (offset>dw2)
			{
				l = ln-1;
				bln = 0;
				while(l > 0)
				{
					val1 = le[l-1]+lb[0]-w;
					if (val1 < 0) val1 = -val1;

					val2 = le[l]+lb[1]-w;
					if (val2 < 0) val2 = -val2;

					if (val1 > val2)
					{
						ll = 0;
						val = (le[ll]-lb[ll]+1)*sizeof(int);
						for(y=0, i=ia+lb[ll]; y<segh; y++, i+=w)
						{
							memset(&Im[i], 0, val);
						}

						for(i=0; i<l; i++)
						{
							lb[i] = lb[i+1];
							le[i] = le[i+1];
						}
					}
					else
					{
						ll = l;
						val = (le[ll]-lb[ll]+1)*sizeof(int);
						for(y=0, i=ia+lb[ll]; y<segh; y++, i+=w)
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
				
					offset = le[l]+lb[0]-w;			
					if (offset<0) offset = -offset;
					if (offset<=dw2)
					{
						bln = 1;
						break;
					}
				};
				
				if(bln == 0)
				{
					val = (le[l]-lb[0]+1)*sizeof(int);
					for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
					{
						memset(&Im[i], 0, val);
					}

					continue;
				}

				ln = l+1;
			}

			// текст состоит всего из 2-х подстрок растояние между которыми больше их размеров ?
			if (ln == 2)
			{
				val1 = le[0]-lb[0]+1;
				val2 = le[1]-lb[1]+1;
				if (val1 < val2) val1 = val2;
				
				val2 = lb[1]-le[0]-1;
				
				if (val2 > val1)
				{
					//удаляем эти под строки
					val = (le[1]-lb[0]+1)*sizeof(int);
					for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
					{
						memset(&Im[i], 0, val);
					}

					continue;
				}
			}

			bln = 0;
			while((ln > 0) && (bln == 0))
			{
				S = 0;
				for(ll=0; ll<ln; ll++)  S += le[ll]-lb[ll]+1;

				SS = le[ln-1]-lb[0]+1;

				if ((double)S/(double)SS < mpd)
				{					
					//определяем подстроку наиболее удаленую от центра
					val1 = lb[ln-1]+le[ln-1]-w;
					val2 = lb[0]+le[0]-w;
					if (val1<0) val1 = -val1;
					if (val2<0) val2 = -val2;
					
					if (val1>val2) ll = ln-1;
					else ll = 0;
					
					//удаляем наиболее удаленую подстроку
					val = (le[ll]-lb[ll]+1)*sizeof(int);
					for(y=0, i=ia+lb[ll]; y<segh; y++, i+=w)
					{
						memset(&Im[i], 0, val);
					}

					for(i=ll; i<ln-1; i++)
					{
						lb[i]=lb[i+1];
						le[i]=le[i+1];
					}

					ln--;
				}
				else
				{
					bln = 1;
				}
			}

			if(ln==0) continue;

			offset = le[ln-1]+lb[0]-w;			
			if (offset<0) offset = -offset;
			
			// потенциальный текст слишком сильно сдвинут от центра изображения ?
			if (offset>dw2)
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			// потенциальный текст не расположен в обоих половинах изображения ?
			if ((lb[0]>=w_2)||(le[ln-1]<=w_2))
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}
				
				continue;
			}

			offset = le[ln-1]+lb[0]-w;			
			if (offset<0)
			{
				val = w - 2*lb[0];
				offset = -offset;
			}
			else
			{
				val = 2*le[ln-1] - w;
			}

			if ((double)offset/(double)val > tcpo)
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			bln = 0;
			while((ln > 0) && (bln == 0))
			{
				S1 = 0;
				S2 = 0;
				ib = ia + lb[0];
				ic = ia + le[ln-1];
				for(y=0; y<segh; y++, ib += w, ic += w)
				{
					for(i = ib; i<=ic; i++)
					{
						if (ImVE[i] == 255) S1++;
						if (ImNE[i] == 255) S2++;
					}
				}

				SS = (le[ln-1]-lb[0]+1)*segh;

				if ( ((double)S1/(double)SS < mpved) && ((double)S2/(double)SS < mpned) )
				{					
					//определяем подстроку наиболее удаленую от центра
					val1 = lb[ln-1]+le[ln-1]-w;
					val2 = lb[0]+le[0]-w;
					if (val1<0) val1 = -val1;
					if (val2<0) val2 = -val2;
					
					if (val1>val2) ll = ln-1;
					else ll = 0;
					
					//удаляем наиболее удаленую подстроку
					val = (le[ll]-lb[ll]+1)*sizeof(int);
					for(y=0, i=ia+lb[ll]; y<segh; y++, i+=w)
					{
						memset(&Im[i], 0, val);
					}

					for(i=ll; i<ln-1; i++)
					{
						lb[i]=lb[i+1];
						le[i]=le[i+1];
					}

					ln--;
				}
				else
				{
					bln = 1;
				}
			}

			if(ln==0) continue;

			offset = le[ln-1]+lb[0]-w;			
			if (offset<0) offset = -offset;
			
			// потенциальный текст слишком сильно сдвинут от центра изображения ?
			if (offset>dw2)
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			// потенциальный текст не расположен в обоих половинах изображения ?
			if ((lb[0]>=w_2)||(le[ln-1]<=w_2))
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}
				
				continue;
			}

			offset = le[ln-1]+lb[0]-w;			
			if (offset<0)
			{
				val = w - 2*lb[0];
				offset = -offset;
			}
			else
			{
				val = 2*le[ln-1] - w;
			}

			if ((double)offset/(double)val > tcpo)
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			// определяем число течек в строке толщиной segh
			// а также их плотность
			ib = ia;
			
			S = 0;
			for(ll=0; ll<ln; ll++)  S += le[ll]-lb[ll]+1;
			S *= segh;

			for(y=0; y<segh; y++, ib += w)
			{
				for(ll=0; ll<ln; ll++)
				{					
					i = ib + lb[ll];
					val = ib + le[ll];
					
					for(; i<=val; i++)
					{
						if (ImVE[i] == 255) nVE++;
						if (ImNE[i] == 255) nNE++;
					}
				}
			}			 

			if ((nVE < mpn) || ((double)nVE/(double)S < mpved))
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			if ((nNE < mpn) || ((double)nNE/(double)S < mpned))
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}

				continue;
			}

			// цветовая фильтрация		

			/*val = msegc*segw - (le[ln-1]-lb[0]+1);

			if (val > 0)
			{
				sb = lb[0] - val/2;
				if (sb < 0) sb = 0;
				ns = msegc;
			}
			else
			{
				sb = lb[0];
				ns = (le[ln-1]-lb[0]+1)/segw;
			}*/

			ns = (le[ln-1]-lb[0]+1)/segw;
			sb = lb[0];

			if (ns < msegc)
			{
				i = le[ln-1];
				do
				{
					if (i+segw < w) { ns++; i+=segw; }
					if (sb-segw > 0) { ns++; sb-=segw; }
				}
				while (ns < msegc);
			}

			ngs = 0;

			for(l=0, ib=ia+sb; l<ns; l++, ib+=segw)
			{
				bln = 1;
				for(y=0, ic=ib; y<segh; y++, ic+=w)
				{
					color = (u8*)(&ImRGB[ic]);
					r0 = color[2];
					g0 = color[1];
					b0 = color[0];	

					mi = ic+segw;
					dif = 0;
					
					for(i=ic+1; i<mi; i++)
					{
						color = (u8*)(&ImRGB[i]);
						r1 = color[2];
						g1 = color[1];
						b1 = color[0];	
						
						rdif = r1-r0;
						if (rdif<0) rdif = -rdif;

						gdif = g1-g0;
						if (gdif<0) gdif = -gdif;

						bdif = b1-b0;
						if (bdif<0) bdif = -bdif;

						dif += rdif+gdif+bdif;

						r0 = r1;
						g0 = g1;
						b0 = b1;
					}

					if (dif < smcd)
					{
						bln = 0;
						break;
					}
				}
				if (bln == 1) ngs++;
				else ngs = 0;

				if (ngs >= msegc) break;
			}

			if (ngs < msegc)
			{
				//удаляем оставшиеся подстроки
				val = (le[ln-1]-lb[0]+1)*sizeof(int);
				for(y=0, i=ia+lb[0]; y<segh; y++, i+=w)
				{
					memset(&Im[i], 0, val);
				}				
				continue;
			}
			else
			{
				res = 1;
			}
		}
	}

	return res;
}

int ThirdFiltration(int* Im, int* ImVE, int* ImNE, int *ImHE, int *LB, int *LE, int LN, int w, int h)
{
	int *LL=g_pLL, *LR=g_pLR, *LLB=g_pLLB, *LLE=g_pLLE, *LW=g_pLW, **LLLB=g_ppLLLB, **LLLE=g_ppLLLE, *NN=g_pLNN;
	int wmin, wmax, nmin, im, vmin, vmax, bln, bln2, res, x, y, k, l, r, val, val1, val2;
	int i, j, da, ib, ie, S, segh, N;
	double mphd, mpnd, mpvd;
	int dy = g_H/16;
	int w_2, ww, mw = g_W/10, yb, ym_temp, ym, xb, xm, nHE1, nHE2, nNE1, nNE2, npNE1, npNE2, nVE1, nVE2;

	res = 0;

	segh = g_segh;
	mpvd = g_mpvd;
	mphd = g_mphd;
	mpnd = g_mpnd;

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
			/*LL[N] = 0;
			LR[N] = 0;*/
		}

		/*if (bln == 1)
		{
			LL[N] += l;
			LR[N] += r;
		}*/
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
			 (LL[k] >= w_2) || 
			 (LR[k] <= w_2) )
		{
			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}
		
		ib = LLB[k]*w+LL[k];
		ie = ib-LL[k]+LR[k];
		im = LLE[k]*w+LL[k];
		vmax = 0;
		for (; ib<=im; ib+=w, ie+=w)
		{
			val = 0;
			for (i=ib; i<=ie; i++)
			{
				if (ImVE[i] == 255) val++;
			}

			if (val > vmax) vmax = val;
		}

		yb = (LLE[k] + LLB[k])/2;
		yb -= yb%segh;

		if ((LLE[k]-LLB[k]+1) > segh)
		{
			ym = yb + dy;			
			
			if (k < N-1)
			{
				if (LLB[k+1] < ym)
				{
					ym = LLB[k+1] - 1;
				}
			}

			if ((ym + segh - 1) >= h)
			{
				ym = h-segh;
			}

			ym -= ym%segh;
		}
		else
		{
			yb = LLB[k];

			ym = yb + 2*segh;
			
			while ((ym + segh - 1) >= h)
			{
				ym -= segh;
			}
		}

		val1 = w_2-LL[k];
		val2 = LR[k]-w_2;
		if (val2 < val1) val1 = val2;

		xb = w_2-val1;
		xm = w_2+val1;

		bln = 0;

		if ((xm-xb+1 < g_W/5) && ((g_ymin + yb) > 0.7*g_H))
		{
			bln2 = 1;
		}
		else
		{
			bln2 = 0;
		}

		val = xb*sizeof(int);
		val1 = LLB[k]*w;
		val2 = LLE[k]-LLB[k]+1;
		for(y=0, i=val1; y<val2; y++, i+=w)
		{
			memset(&Im[i], 0, val);
		}
		val = (w-xm)*sizeof(int);
		val1 += xm+1;
		for(y=0, i=val1; y<val2; y++, i+=w)
		{
			memset(&Im[i], 0, val);
		}		

		ym_temp = ym;

L:		ib = yb*w;
		ie = ym*w;
		da = w*segh;
		ww = xm-xb+1;
		S = ww*segh;

		npNE1 = 0;
		npNE2 = 0;

		while (ib<=ie)
		{
			nHE1 = 0;
			nHE2 = 0;
			nNE1 = 0;
			nNE2 = 0;			
			nVE1 = 0;
			nVE2 = 0;

			for (y=0; y<segh; y++, ib+=w)
			{
				for (x=xb; x<=xm; x++)
				{
					i = ib + x; 
					
					if (ImHE[i] == 255)
					{
						if (x < w_2) nHE1++;
						else nHE2++;
					}

					if (ImNE[i] == 255)
					{
						if (x < w_2) nNE1++;
						else nNE2++;
					}
					
					if (ImVE[i] == 255)
					{	
						if (x < w_2) nVE1++;
						else nVE2++;
					}
				}
			}			

			if ( ( ((double)(nNE1*2)/S >= mpnd) && ((double)(nNE2*2)/S >= mpnd) ) &&
				 ((double)(nHE1*2)/S >= mphd) &&
				 ((double)(nHE2*2)/S >= mphd) )
			{
					bln = 1;
					break;
			}

			if (ww < mw)
			{
				if ( ((double)(nNE1*2)/S >= mpnd) && 
					 ((double)(nNE2*2)/S >= 0.5*mpnd) &&
				     ((double)(nHE1*2)/S >= mphd) &&
				     ((double)(nHE2*2)/S >= mphd) )
				{
						bln = 1;
						break;
				}
			}

			if ( ((double)(nVE1*2)/(double)S < 0.5*mpvd) ||
				 ((double)(nVE2*2)/(double)S < 0.5*mpvd) )
			{
				break;
			}

			npNE1 = nNE1;
			npNE2 = nNE2;
		}

		if (bln == 0)
		{
			if (bln2 == 1)
			{
				bln2 = 0;
				val = (int)((double)(xm-xb+1)*0.20);
				xb += val;
				xm -= val;
				ym = ym_temp;

				goto L;
			}

			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}

		vmin = vmax;
		ib = LLE[k]*w+xb;
		ie = ib-xb+xm;
		val = LLE[k]+(int)(0.04*(double)g_H);
		if (val > h-1) val = h-1;
		im = val*w+xb;
		for (;ib<=im; ib+=w, ie+=w)
		{
			val = 0;
			for (i=ib; i<=ie; i++)
			{								
				if (ImVE[i] == 255)	val++;
			}	

			if (val < vmin) vmin = val;
		}

		if ((double)vmin/vmax > 0.33)
		{
			if (bln2 == 1)
			{
				bln2 = 0;
				val = (int)((double)(xm-xb+1)*0.20);
				xb += val;
				xm -= val;
				ym = ym_temp;

				goto L;
			}

			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}

		if ((LLE[k]-LLB[k]+1) > segh)
		{
			ym = yb - dy;
			if (ym < 0) ym = 0;
			if (k > 0)
			{
				if (LLE[k-1] > ym)
				{
					ym = LLE[k-1] + 1;
				}
			}
			ym -= ym%segh;
		}
		else
		{
			ym = yb - 2*segh;
			if (ym < 0) ym = 0;
		}
		
		bln = 0;

		ib = yb*w;
		ie = ym*w;
		da = w*segh;
		S = (xm-xb+1)*segh;

		npNE1 = 0;
		npNE2 = 0;

		while (ib>=ie)
		{
			nHE1 = 0;
			nHE2 = 0;
			nNE1 = 0;
			nNE2 = 0;			
			nVE1 = 0;
			nVE2 = 0;

			for (y=0; y<segh; y++, ib+=w)
			{
				for (x=xb; x<=xm; x++)
				{
					i = ib + x; 
					
					if (ImHE[i] == 255)
					{
						if (x < w_2) nHE1++;
						else nHE2++;
					}

					if (ImNE[i] == 255)
					{
						if (x < w_2) nNE1++;
						else nNE2++;
					}
					
					if (ImVE[i] == 255)
					{
						if (x < w_2) nVE1++;
						else nVE2++;
					}
				}
			}

			ib -= 2*da;

			if ( ( ((double)(nNE1*2)/S >= mpnd) && ((double)(nNE2*2)/S >= mpnd) ) &&
				 ( ((double)(nHE1*2)/S >= 0.5*mphd) && ((double)(nHE2*2)/S >= mphd) ||
					((double)(nHE1*2)/S >= mphd) && ((double)(nHE2*2)/S >= 0.5*mphd)
					)
				 )
			{
					bln = 1;
					break;
			}

			if (ww < mw)
			{
				if ( ((double)(nNE1*2)/S >= mpnd) && 
					 ((double)(nNE2*2)/S >= 0.33*mpnd) &&
					 ((double)(nNE1+nNE2)/S >= mpnd) &&
					 ((double)(nHE1*2)/S >= mphd) &&
					 ((double)(nHE2*2)/S >= 0.5*mphd) &&
					 ((double)(nHE1+nHE2)/S >= mphd) )
				{
						bln = 1;
						break;
				}
			}

			if ( ((double)(nVE1*2)/(double)S < 0.5*mpvd) ||
				 ((double)(nVE2*2)/(double)S < 0.5*mpvd) )
			{
				break;
			}

			npNE1 = nNE1;
			npNE2 = nNE2;
		}

		if (bln == 0)
		{
			if (bln2 == 1)
			{
				bln2 = 0;
				val = (int)((double)(xm-xb+1)*0.20);
				xb += val;
				xm -= val;
				ym = ym_temp;

				goto L;
			}

			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}

		vmin = vmax;
		ib = LLB[k]*w+xb;
		ie = ib-xb+xm;
		val = LLB[k]-(int)(0.04*(double)g_H);
		if (val < 0) val = 0;
		im = val*w+xb;
		for (;ib>=im; ib-=w, ie-=w)
		{
			val = 0;
			for (i=ib; i<=ie; i++)
			{								
				if (ImVE[i] == 255)	val++;
			}	

			if (val < vmin) vmin = val;
		}

		if ((double)vmin/vmax > 0.33)
		{
			if (bln2 == 1)
			{
				bln2 = 0;
				val = (int)((double)(xm-xb+1)*0.20);
				xb += val;
				xm -= val;
				ym = ym_temp;

				goto L;
			}

			memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
			continue;
		}

		res = 1;
	}

	return res;
}

int ThirdFiltrationForGFTI(int* Im, int* ImVE, int* ImNE, int *ImHE, int *LB, int *LE, int LN, int w, int h)
{
	int *LL=g_pLL2, *LR=g_pLR2, *LLB=g_pLLB2, *LLE=g_pLLE2, *LW=g_pLW2, **LLLB=g_ppLLLB2, **LLLE=g_ppLLLE2, *NN=g_pLNN2;
	int wmin, wmax, nmin, im, vmin, vmax, bln, bln2, res, x, y, k, l, r, val, val1, val2;
	int i, j, da, ib, ie, S, segh, YMIN, YMAX, K, N;
	double mphd, mpnd, mpvd;
	int dy = g_H/16;
	int w_2, ww, mw = g_W/10, yb, ym_temp, ym, xb, xm, nHE1, nHE2, nNE1, nNE2, npNE1, npNE2, nVE1, nVE2;

	res = 0;

	segh = g_segh;
	mpvd = g_mpvd;
	mphd = g_mphd;
	mpnd = g_mpnd;

	w_2 = w/2;

	for(K=0; K < LN; K++)
	{
		YMIN = LB[K];
		YMAX = LE[K];

		N = 0;
		LLB[0] = -1;
		LLE[0] = -1;
		LL[0] = 0;
		LR[0] = 0;
		for (y=YMIN, ib=YMIN*w; y<YMAX; y++, ib+=w)
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
		if (LLE[N] == YMAX-1) N++;

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
				(LL[k] >= w_2) || 
				(LR[k] <= w_2) )
			{
				memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
				continue;
			}
			
			ib = LLB[k]*w+LL[k];
			ie = ib-LL[k]+LR[k];
			im = LLE[k]*w+LL[k];
			vmax = 0;
			for (; ib<=im; ib+=w, ie+=w)
			{
				val = 0;
				for (i=ib; i<=ie; i++)
				{
					if (ImVE[i] == 255) val++;
				}

				if (val > vmax) vmax = val;
			}

			yb = (LLE[k] + LLB[k])/2;
			yb -= yb%segh;

			if ((LLE[k]-LLB[k]+1) > segh)
			{
				ym = yb + dy;
				if (ym >= h) ym = h-1;
				if (k < N-1)
				{
					if (LLB[k+1] < ym)
					{
						ym = LLB[k+1] - 1;
					}
				}
				ym -= ym%segh;
			}
			else
			{
				yb = LLB[k];

				ym = yb + 2*segh;
			}
			if (ym > YMAX) 
			{
				ym = YMAX;
				ym -= ym%segh;
			}

			val1 = w_2-LL[k];
			val2 = LR[k]-w_2;
			if (val2 < val1) val1 = val2;

			xb = w_2-val1;
			xm = w_2+val1;

			bln = 0;

			if ((xm-xb+1 < g_W/5) && ((g_ymin + g_pLB[K] + yb - LB[K]) > 0.7*g_H))
			{
				bln2 = 1;
			}
			else
			{
				bln2 = 0;
			}

			val = xb*sizeof(int);
			val1 = LLB[k]*w;
			val2 = LLE[k]-LLB[k]+1;
			for(y=0, i=val1; y<val2; y++, i+=w)
			{
				memset(&Im[i], 0, val);
			}
			val = (w-xm)*sizeof(int);
			val1 += xm+1;
			for(y=0, i=val1; y<val2; y++, i+=w)
			{
				memset(&Im[i], 0, val);
			}		

			ym_temp = ym;

	L:		ib = yb*w;
			ie = ym*w;
			da = w*segh;
			ww = xm-xb+1;
			S = ww*segh;

			npNE1 = 0;
			npNE2 = 0;

			while (ib<=ie)
			{
				nHE1 = 0;
				nHE2 = 0;
				nNE1 = 0;
				nNE2 = 0;			
				nVE1 = 0;
				nVE2 = 0;

				for (y=0; y<segh; y++, ib+=w)
				{
					for (x=xb; x<=xm; x++)
					{
						i = ib + x; 
						
						if (ImHE[i] == 255)
						{
							if (x < w_2) nHE1++;
							else nHE2++;
						}

						if (ImNE[i] == 255)
						{
							if (x < w_2) nNE1++;
							else nNE2++;
						}
						
						if (ImVE[i] == 255)
						{	
							if (x < w_2) nVE1++;
							else nVE2++;
						}
					}
				}			

				if ( ( ((double)(nNE1*2)/S >= mpnd) && ((double)(nNE2*2)/S >= mpnd) ) &&
					((double)(nHE1*2)/S >= mphd) &&
					((double)(nHE2*2)/S >= mphd) )
				{
						bln = 1;
						break;
				}

				if (ww < mw)
				{
					if ( ((double)(nNE1*2)/S >= mpnd) && 
						((double)(nNE2*2)/S >= 0.5*mpnd) &&
						((double)(nHE1*2)/S >= mphd) &&
						((double)(nHE2*2)/S >= mphd) )
					{
							bln = 1;
							break;
					}
				}

				if ( ((double)(nVE1*2)/(double)S < 0.5*mpvd) ||
					((double)(nVE2*2)/(double)S < 0.5*mpvd) )
				{
					break;
				}

				npNE1 = nNE1;
				npNE2 = nNE2;
			}

			if (bln == 0)
			{
				if (bln2 == 1)
				{
					bln2 = 0;
					val = (int)((double)(xm-xb+1)*0.20);
					xb += val;
					xm -= val;
					ym = ym_temp;

					goto L;
				}

				memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
				continue;
			}

			vmin = vmax;
			ib = LLE[k]*w+xb;
			ie = ib-xb+xm;
			val = LLE[k]+(int)(0.04*(double)g_H);
			if (val > YMAX) val = YMAX;
			im = val*w+xb;
			for (;ib<=im; ib+=w, ie+=w)
			{
				val = 0;
				for (i=ib; i<=ie; i++)
				{								
					if (ImVE[i] == 255)	val++;
				}	

				if (val < vmin) vmin = val;
			}

			if ((double)vmin/vmax > 0.33)
			{
				if (bln2 == 1)
				{
					bln2 = 0;
					val = (int)((double)(xm-xb+1)*0.20);
					xb += val;
					xm -= val;
					ym = ym_temp;

					goto L;
				}

				memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
				continue;
			}

			if ((LLE[k]-LLB[k]+1) > segh)
			{
				ym = yb - dy;
				if (ym < 0) ym = 0;
				if (k > 0)
				{
					if (LLE[k-1] > ym)
					{
						ym = LLE[k-1] + 1;
					}
				}
				ym -= ym%segh;
			}
			else
			{
				ym = yb - 2*segh;				
			}
			if (ym < YMIN) ym = YMIN;
			
			bln = 0;

			ib = yb*w;
			ie = ym*w;
			da = w*segh;
			S = (xm-xb+1)*segh;

			npNE1 = 0;
			npNE2 = 0;

			while (ib>=ie)
			{
				nHE1 = 0;
				nHE2 = 0;
				nNE1 = 0;
				nNE2 = 0;			
				nVE1 = 0;
				nVE2 = 0;

				for (y=0; y<segh; y++, ib+=w)
				{
					for (x=xb; x<=xm; x++)
					{
						i = ib + x; 
						
						if (ImHE[i] == 255)
						{
							if (x < w_2) nHE1++;
							else nHE2++;
						}

						if (ImNE[i] == 255)
						{
							if (x < w_2) nNE1++;
							else nNE2++;
						}
						
						if (ImVE[i] == 255)
						{
							if (x < w_2) nVE1++;
							else nVE2++;
						}
					}
				}

				ib -= 2*da;

				if ( ( ((double)(nNE1*2)/S >= mpnd) && ((double)(nNE2*2)/S >= mpnd) ) &&
					( ((double)(nHE1*2)/S >= 0.5*mphd) && ((double)(nHE2*2)/S >= mphd) ||
						((double)(nHE1*2)/S >= mphd) && ((double)(nHE2*2)/S >= 0.5*mphd)
						)
					)
				{
						bln = 1;
						break;
				}

				if (ww < mw)
				{
					if ( ((double)(nNE1*2)/S >= mpnd) && 
						((double)(nNE2*2)/S >= 0.33*mpnd) &&
						((double)(nNE1+nNE2)/S >= mpnd) &&
						((double)(nHE1*2)/S >= mphd) &&
						((double)(nHE2*2)/S >= 0.5*mphd) &&
						((double)(nHE1+nHE2)/S >= mphd) )
					{
							bln = 1;
							break;
					}
				}

				if ( ((double)(nVE1*2)/(double)S < 0.5*mpvd) ||
					((double)(nVE2*2)/(double)S < 0.5*mpvd) )
				{
					break;
				}

				npNE1 = nNE1;
				npNE2 = nNE2;
			}

			if (bln == 0)
			{
				if (bln2 == 1)
				{
					bln2 = 0;
					val = (int)((double)(xm-xb+1)*0.20);
					xb += val;
					xm -= val;
					ym = ym_temp;

					goto L;
				}

				memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
				continue;
			}

			vmin = vmax;
			ib = LLB[k]*w+xb;
			ie = ib-xb+xm;
			val = LLB[k]-(int)(0.04*(double)g_H);
			if (val < YMIN) val = YMIN;
			im = val*w+xb;
			for (;ib>=im; ib-=w, ie-=w)
			{
				val = 0;
				for (i=ib; i<=ie; i++)
				{								
					if (ImVE[i] == 255)	val++;
				}	

				if (val < vmin) vmin = val;
			}

			if ((double)vmin/vmax > 0.33)
			{
				if (bln2 == 1)
				{
					bln2 = 0;
					val = (int)((double)(xm-xb+1)*0.20);
					xb += val;
					xm -= val;
					ym = ym_temp;

					goto L;
				}

				memset(&Im[LLB[k]*w], 0, (LLE[k]-LLB[k]+1)*w*sizeof(int));
				continue;
			}

			res = 1;
		}
	}

	return res;
}

int FindTextLines(int *ImRGB, int *ImF, int *ImNF, vector<string> &SavedFiles, int W, int H)
{
	int *LL = g_pLL3, *LR = g_pLR3, *LLB = g_pLLB3, *LLE = g_pLLE3, *LW = g_pLW3;
	int **LN = g_ppLN3, *LNN = g_pLNN3;
	int i, j, k, l, r, x, y, ib, bln, N, N1, N2, N3, N4, N5, N6, N7, minN, maxN, w, h, ww, hh, cnt;
	int XB, XE, YB, YE, DXB, DXE, DYB, DYE;
	int xb, xe, yb, ye, segh;
	int delta, val, val1, val2, val3, val4, val5, cnt1, cnt2, NN, ys1, ys2, ys3, ys4, ys5, val_min, val_max;
	static int GRStr[256*2], smax[256*2], smaxi[256*2];
	int j1, j2, j3, j4, j5, j1_min, j1_max, j2_min, j2_max, j3_min, j3_max, j4_min, j4_max, j5_min, j5_max/*, j1_min_prev, j1_max_prev*/;
	int mY, dY, mI, mQ, dI, dQ, jY_min, jY_max, mmY, ddY1, ddY2, mmI, mmQ, ddI, ddQ;
	int LH, LMAXY;
	double mthr;
	string SaveName, FullName, Str;
	char str[30];
	int res;

	res = 0;

	SaveName = SavedFiles[0];
	SavedFiles.clear();

	mthr = 0.3;
	segh = g_segh;

	if (g_show_results == 1) SaveImage(ImF, "\\TestImages\\Image0_F.jpeg", W, H);
	if (g_show_results == 1) SaveImage(ImNF, "\\TestImages\\Image0_NF.jpeg", W, H);

	N = 0;
	LLB[0] = -1;
	LLE[0] = -1;
	LL[0] = W-1;
	LR[0] = 0;
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

	
	val = (int)((double)W*0.3);
	for(y=0, ib=0; y<H; y++, ib+=W)
	{
		for(x=0, i=ib; x<val; x++, i++)
		{
			ImNF[i] = 0;
		}

		for(x=W-val+1, i=ib+W-val; x<W; x++, i++)
		{
			ImNF[i] = 0;
		}
	}

	//g_pMF->m_pImageBox->g_pViewImage(ImNF, W, H);
	//return;

	CreateIndexedImage(ImNF, g_ImRES1, W, H, 255, val);

	for (k=0; k<N; k++)
	{
		LNN[k] = 0;
	}

	val = (int)((double)W*0.3);
	for (k=0; k<N; k++)
	{
		XB = LL[k];
		XE = LR[k];
		if (XB < val) XB = val;
		if (XE > W-val) XE = W-val;

		YB = LLB[k];
		YE = LLE[k];

		for (y=YB, ib=YB*W; y<=YE; y++, ib+=W)
		{
			for (x=XB, i=ib+XB; x<=XE; x++, i++)
			{
				if (ImF[i] == 255) 
				{
					val1 = g_ImRES1[i];
					bln = 0;

					for(j=0; j<LNN[k]; j++)
					{
						if (LN[k][j] == val1)
						{
							bln = 1;
							break;
						}
					}

					if (bln == 0)
					{
						LN[k][LNN[k]] = val1;
						//if (LNN[k]+1 < 1024) LNN[k]++;
					}
				}
			}
		}
	}

	val = (int)(0.06*(double)g_H);
	k=N-2;
	while (k >= 0)
	{
		if (  LLE[k+1]-LLB[k]+1 <= val )
		{
			bln = 0;
			for (i=0; i<LNN[k]; i++)
			{
				for (j=0; j<LNN[k+1]; j++)
				{
					if (LN[k][i] == LN[k+1][j])
					{
						bln = 1;
						break;
					}
				}
				if (bln == 1) break;
			}

			if (bln == 1)
			{
				if (LL[k+1] < LL[k]) LL[k] = LL[k+1];
				if (LR[k+1] > LR[k]) LR[k] = LR[k+1];
				LLE[k] = LLE[k+1];
				
				for(i=k+1; i<N-1; i++)
				{
					LL[i] = LL[i+1];
					LR[i] = LR[i+1];
					LLB[i] = LLB[i+1];
					LLE[i] = LLE[i+1];
					memcpy(LN[i], LN[i+1], LNN[i+1]*sizeof(int));
					LNN[i] = LNN[i+1];
				}

				N--;
			}
		}

		k--;
	}


	val = (int)(0.08*(double)g_H);
	k=N-1;
	while (k >= 0)
	{
		if (  LLE[k]-LLB[k]+1 > val )
		{
			bln = 0;

			val1 = LW[LLB[k]];
			for (y=LLB[k]+1; y<=LLE[k]; y++)
			{
				if (LW[y] > val1) val1 = LW[y];
			}

			val2 = val1;
			for (y=LLB[k]+(LLE[k]-LLB[k]+1)/4; y<=LLE[k]-(LLE[k]-LLB[k]+1)/4; y++)
			{
				if (LW[y] < val2) 
				{
					val2 = LW[y];
					val3 = y;
				}
			}

			if ((double)val2/val1 <= 0.5)
			{	
				LL[k+1] = LL[k];
				LR[k+1] = LR[k];
				LLB[k+1] = val3+1;
				LLE[k+1] = LLE[k];

				LLE[k] = val3;

				N++;				
			}
		}

		k--;
	}

	for (k=0; k<N; k++)
	{
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
		h = YE-YB+1;

		if (h<6)
		{
			val = (6-h)/2;
			LLB[k] -= val;
			LLE[k] = LLB[k]+6-1;
			
			YB = LLB[k];
			YE = LLE[k];
			h = YE-YB+1;
		}

		val = (int)((double)g_H*0.15) - h;
		if (val > 10*2)
		{
			YB -= val/2;
			YE += val/2;
		}
		else
		{
			YB -= 10;
			YE += 10;
		}
		if (YB < 0) YB = 0;
		if (YE > H-1) YE = H-1;

		if (k>0)
		{
			val = (LLB[k-1]+LLE[k-1]*2)/3;
			if (YB < val) YB = val;
		}
		if (k<N-1)
		{
			val = (LLB[k+1]*2+LLE[k+1])/3;
			if (YE > val) YE = val;
		}
		h = YE-YB+1;

		for(y=YB, i=YB*W+XB, j=0; y<=YE; y++, i+=W, j+=w)
		{
			memcpy(&g_ImRES1[j], &ImRGB[i], w*sizeof(int));
		}
		ResizeImage4x(g_ImRES1, g_Im, w, h);
		if (g_show_results == 1) SaveRGBImage(g_Im, "\\TestImages\\Image1_RGB.jpeg", w*4, h*4);
		
		for(y=YB, i=YB*W+XB, j=0; y<=YE; y++, i+=W, j+=w)
		{
			memcpy(&g_ImRES2[j], &ImF[i], w*sizeof(int));
		}
		SimpleResizeImage4x(g_ImRES2, g_ImSF, w, h);
		if (g_show_results == 1) SaveImage(g_ImSF, "\\TestImages\\Image2_SF.jpeg", w*4, h*4);
		//g_pMF->m_pImageBox->g_pViewImage(ImSF, w*4, h*4);
		//break;

		GetFirstFilteredImage(g_ImRES1, g_ImSF, g_ImFF, w, h, (LL[k]-XB)*4, (LR[k]-XB)*4, (LLB[k]-YB)*4, (LLE[k]-YB)*4);
		if (g_show_results == 1) SaveImage(g_ImFF, "\\TestImages\\Image3_FF.jpeg", w*4, h*4);
		//g_pMF->m_pImageBox->g_pViewImage(ImFF, w*4, h*4);
		//break;
			
		w *= 4;
		h *= 4;
		
		memcpy(g_ImRES1, g_ImFF, (w*h)*sizeof(int));
		IntersectTwoImages(g_ImRES1, g_ImSF, w, h);
		if (g_show_results == 1) SaveImage(g_ImRES1, "\\TestImages\\Image4_IF.jpeg", w, h);
		//g_pMF->m_pImageBox->g_pViewImage(g_ImRES1, w, h);
		//break;

		RGB_to_YUV(g_Im, g_ImY, g_ImU, g_ImV, w, h);
		RGB_to_YIQ(g_Im, g_ImY, g_ImI, g_ImQ, w, h);

		delta = 40;

		yb = (LLB[k]-YB+LLE[k]-YB)*2-1;
		ye = yb+segh-1;
		xb = (LL[k]-XB)*4;
		xe = (LR[k]-XB)*4;
		
		StrAnalyseImage(g_ImRES1, g_ImY, GRStr, w, h, xb, xe, yb, ye, 0);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
		FindMaxStr(smax, smaxi, j1, ys1, NN);
		j1_min = j1;
		j1_max = j1+delta-1;

		for (y=0, i=0; y<h; y++)
		{
			for (x=0; x<w; x++, i++)
			{				
				val = g_ImY[i];

				if ( ( (g_ImY[i] < j1_min) || (g_ImY[i] > j1_max) ) && 
					 (g_ImSF[i] == 255) )
				{
					g_ImRES2[i] = 255;
				}
				else
				{
					g_ImRES2[i] = 0;
				}
			}
		}

		val1 = (int)((double)((LLE[k]-LLB[k]+1)*4)*0.3);
		val2 = (int)((double)((LR[k]-LL[k]+1)*4)*0.1);
		yb = (LLB[k]-YB)*4+val1;
		ye = (LLE[k]-YB)*4-val1;
		xb = (LL[k]-XB)*4+val2;
		xe = (LR[k]-XB)*4-val2;
		
		StrAnalyseImage(g_ImRES2, g_ImY, GRStr, w, h, xb, xe, yb, ye, 0);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
		FindMaxStr(smax, smaxi, j2, ys2, NN);
		j2_min = j2;
		j2_max = j2+delta-1;
		
		for (i=j2; i<j2+delta; i++) GRStr[i] = 0;

		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
		FindMaxStr(smax, smaxi, j3, ys3, NN);
		j3_min = j3;
		j3_max = j3+delta-1;

		int color, r, g, b, c;
		u8 *pClr;

		pClr = (u8*)(&color);

		color = 0;
		pClr[2] = 255;
		r = color;

		color = 0;
		pClr[1] = 255;
		g = color;

		color = 0;
		pClr[0] = 255;
		b = color;

		color = 0;
		pClr[2] = 128;
		pClr[0] = 128;
		c = color;

		for (y=0, i=0; y<h; y++)
		{
			for (x=0; x<w; x++, i++)
			{				
				val = g_ImY[i];					

				if ( ( val >= j1_min ) && (val <= j1_max) )
				{
					g_ImRES2[i] = r;
				}
				else if ( ( val >= j2_min ) && (val <= j2_max) )
				{
					g_ImRES2[i] = g;
				}
				else if ( ( val >= j3_min ) && (val <= j3_max) )
				{
					g_ImRES2[i] = b;
				}
				else
				{
					g_ImRES2[i] = c;
				}
			}
		}
		if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image5_SE1.jpeg", w, h);
		//g_pMF->m_pImageBox->g_pViewRGBImage(g_ImRES2, w, h);
		//break;

		yb = (LLB[k]-YB)*4;
		ye = (LLE[k]-YB)*4;

		val = ClearImage(g_ImRES2, w, h, yb, ye, r);
		val += ClearImage(g_ImRES2, w, h, yb, ye, g);
		val += ClearImage(g_ImRES2, w, h, yb, ye, b);
		val += ClearImage(g_ImRES2, w, h, yb, ye, c);

		if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image6_SE2.jpeg", w, h);
		//g_pMF->m_pImageBox->g_pViewRGBImage(g_ImRES2, w, h);
		//break;

		if (val == 0) continue;

		memcpy(g_ImRES3, g_ImSF, (w*h)*sizeof(int));
		for(i=0; i<w*h; i++)
		{
			if (g_ImRES2[i] == 0)
			{
				g_ImRES3[i] = 0;	
			}
		}
		if (g_show_results == 1) SaveImage(g_ImRES3, "\\TestImages\\Image7_IF.jpeg", w, h);
		//g_pMF->m_pImageBox->g_pViewImage(g_ImRES3, w, h);
		//break;

		yb = (LLB[k]-YB)*4;
		ye = (LLE[k]-YB)*4;
		xb = (LL[k]-XB)*4;
		xe = (LR[k]-XB)*4;
		ClearImageSpecific1(g_ImRES3, w, h, yb, ye, xb, xe, 255);
		if (g_show_results == 1) SaveImage(g_ImRES3, "\\TestImages\\Image7_IFB.jpeg", w, h);
		
		delta = 40;
		
		val1 = (int)((double)((LLE[k]-LLB[k]+1)*4)*0.3);
		val2 = (int)((double)((LR[k]-LL[k]+1)*4)*0.1);
		yb = (LLB[k]-YB)*4+val1;
		ye = (LLE[k]-YB)*4-val1;
		xb = (LL[k]-XB)*4+val2;
		xe = (LR[k]-XB)*4-val2;

		StrAnalyseImage(g_ImRES3, g_ImY, GRStr, w, h, xb, xe, yb, ye, 0);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
		FindMaxStr(smax, smaxi, j1, ys1, NN);

		val1 = j1-80;
		if (val1 < 0) val1 = 0;
		val2 = j1+80;
		if (val2 > 255) val2 = 255;

		for (i=val1; i<=val2; i++) GRStr[i] = 0;
		
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
		FindMaxStr(smax, smaxi, j2, ys2, NN);

		if ( (j2 > j1) && ((double)ys1/ys2 < 3) )
		{
			j1 = j2;
		}

		int cnt3 = 0;
		for(i=0; i<w*h; i++)
		{
			if (g_ImRES3[i] != 0)
			{
				cnt3++;	
			}
		}

		int delta_new;
		int val_prev;
		int d_cnt, d_cnt_prev, j1_min_prev, j1_max_prev;
		int recn;
		
		yb = (LLB[k]-YB)*4;
		ye = (LLE[k]-YB)*4;
		xb = (LL[k]-XB)*4;
		xe = (LR[k]-XB)*4;

		recn = 1;
		delta_new = delta;
		val_prev = -1;
		d_cnt_prev = -1;

		while(1)
		{
			val = (delta_new-delta)/2;
			delta = delta_new;
			j1 -= val;
			j1_min = j1;
			j1_max = j1_min+delta-1;
			if (j1_min < 0) 
			{
				j1_min = 0;
				j1_max = j1_min+delta-1;
			}
			else if (j1_max > 255)
			{
				j1_max = 255;
				j1_min = 255-delta+1;
			}

			AnalizeAndClearImage(g_ImRES1, g_ImY, w, h, j1_min, j1_max, r, g, yb, ye, xb, xe, cnt1, cnt2);

			if (cnt1 > cnt2) val = cnt1;
			else val = cnt2;
			d_cnt = cnt2-cnt1;

			if ( ((double)cnt3/val > 6) && (delta_new < 120) && 
				( (val_prev == -1) || (val >= val_prev) ) &&
				( (d_cnt_prev == -1) || (delta_new<=80) || ( (d_cnt_prev*d_cnt > 0) && (delta_new > 80) ) )
				)
			{
				d_cnt_prev = d_cnt;
				j1_min_prev = j1_min;
				j1_max_prev = j1_max;
				val_prev = val;
				memcpy(g_ImRES4, g_ImRES1, (w*h)*sizeof(int));

				delta_new += 20;
				continue;
			}

			if ( (val_prev != -1) && ( ( (val < val_prev) && (d_cnt_prev*d_cnt >= 0) ) || ( (d_cnt_prev*d_cnt <= 0) && (delta_new > 80) ) ) ) 
			{
				d_cnt = d_cnt_prev;
				j1_min = j1_min_prev;
				j1_max = j1_max_prev;
				memcpy(g_ImRES1, g_ImRES4, (w*h)*sizeof(int));
				if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_RGF.jpeg", w, h);
			}

			if ( (d_cnt > 0) && (recn == 2) )
			{
				val1 = 0;
				val2 = 0;
				val3 = 0;
				val4 = 0;

				for(delta = 60; delta <= 120; delta += 20)
				{
					j1_min = 0;
					j1_max = j1_min+delta-1;

					while(1)
					{
						val = AnalizeAndClearImage(g_ImRES1, g_ImY, w, h, j1_min, j1_max, r, g, yb, ye, xb, xe, cnt1, cnt2);
						if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_RGFC.jpeg", w, h);

						if (val > (val4*4)/3)
						{
							val1 = cnt1;
							val2 = j1_min;
							val3 = delta;
							val4 = val;
						}
						
						if ( (cnt1 > val1) && ((3*val)/4 >= val4) )
						{
							val1 = cnt1;
							val2 = j1_min;
							val3 = delta;
							val4 = val;
						}

						if (j1_max == 255) break;
						
						j1_min += 20;
						j1_max += 20;

						if (j1_max > 255)
						{
							j1_max = 255;
							j1_min = j1_max-delta+1;
						}
					}
				}

				j1_min = j1 = val2;
				delta = val3;
				j1_max = j1_min+delta-1;

				AnalizeAndClearImage(g_ImRES1, g_ImY, w, h, j1_min, j1_max, r, g, yb, ye, xb, xe, cnt1, cnt2);

				for (i=0; i<=w*h; i++)
				{
					if (g_ImRES1[i] == g) g_ImRES1[i] = 0;
				}
				
				recn++;

				break;
			}
			else if ( (d_cnt > 0) && (recn < 2) )
			{
				recn++;

				for (i=0; i<=w*h; i++)
				{
					if (g_ImRES1[i] == r) g_ImRES1[i] = 0;
				}

				ClearImageSpecific(g_ImRES1, w, h, g);
				if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_RGFB.jpeg", w, h);

				delta = 60;
				StrAnalyseImage(g_ImRES1, g_ImY, GRStr, w, h, xb, xe, yb, ye, 0);
				FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 0);
				FindMaxStr(smax, smaxi, j1, ys1, NN);
				val_prev = -1;
				d_cnt_prev = -1;
				delta_new = delta;
			}
			else
			{
				for (i=0; i<=w*h; i++)
				{
					if (g_ImRES1[i] == g) g_ImRES1[i] = 0;
				}

				break;
			}
		}

		if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_RGFF.jpeg", w, h);

		/*if ((recn < 2) && (delta > 60))
		{
			delta = j1_max-j1_min+1;

			if ( (j1_min < 100) && (j1_max < 255) && (delta > 40) )
			{
				val = (delta-40)/2;
				if (val > 20) val = 20;
				j1_min += val;
			}
		}*/
		j1 = j1_min;

		delta = 20;
		StrAnalyseImage(g_ImRES1, g_ImU, GRStr, w, h, xb, xe, yb, ye, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 256);
		if (NN == 0) continue;
		FindMaxStr(smax, smaxi, j2, ys2, NN);
		
		delta_new = 50;
		val = (delta_new-delta)/2;
		delta = delta_new;
		j2 -= val;
		j2_min = j2-256;
		j2_max = j2_min+delta-1;

		delta = 20;
		StrAnalyseImage(g_ImRES1, g_ImV, GRStr, w, h, xb, xe, yb, ye, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 256);
		if (NN == 0) continue;
		FindMaxStr(smax, smaxi, j3, ys3, NN);

		delta_new = 50;
		val = (delta_new-delta)/2;
		delta = delta_new;
		j3 -= val;
		j3_min = j3-256;
		j3_max = j3_min+delta-1;

		delta = 20;
		StrAnalyseImage(g_ImRES1, g_ImI, GRStr, w, h, xb, xe, yb, ye, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 256);
		if (NN == 0) continue;
		FindMaxStr(smax, smaxi, j4, ys4, NN);
		
		delta_new = 50;
		val = (delta_new-delta)/2;
		delta = delta_new;
		j4 -= val;
		j4_min = j4-256;
		j4_max = j4_min+delta-1;

		delta = 20;
		StrAnalyseImage(g_ImRES1, g_ImQ, GRStr, w, h, xb, xe, yb, ye, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NN, 256);
		if (NN == 0) continue;
		FindMaxStr(smax, smaxi, j5, ys5, NN);

		delta_new = 50;
		val = (delta_new-delta)/2;
		delta = delta_new;
		j5 -= val;
		j5_min = j5-256;
		j5_max = j5_min+delta-1;

		j1_min_prev = j1_min;
		j1_max_prev = j1_max;

		//-----применяем первый метод отделения текста-----//
		if (g_hard_sub_mining == true) while(1)
		{
			if ( j1_max-j1_min+1 > 60 )
			{
				j1_min += 10;
				j1_max -= 10;
			}

			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= j1_min ) && (val1 <= j1_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES1[i] = r;
				}
				else
				{
					g_ImRES1[i] = 0;
				}
			}

			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= j1_min ) && (val1 <= j1_max) ) 
				{
					g_ImRES2[i] = r;
				}
				else
				{
					g_ImRES2[i] = 0;
				}
			}

			val_min = j1_min-20;
			val_max = j1_max-20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
				{
					g_ImRES3[i] = r;
				}
				else
				{
					g_ImRES3[i] = 0;
				}
			}

			val_min = j1_min+20;
			val_max = j1_max+20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
				{
					g_ImRES4[i] = r;
				}
				else
				{
					g_ImRES4[i] = 0;
				}
			}

			val_min = j1_min;
			val_max = j1_max+20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES5[i] = r;
				}
				else
				{
					g_ImRES5[i] = 0;
				}
			}

			val_min = j1_min-20;
			if (val_min > 70)
			{
				if (val_min-30 < 70) val_min = 70;
				else val_min -= 30;
			}
			val_max = j1_max;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES6[i] = r;
				}
				else
				{
					g_ImRES6[i] = 0;
				}
			}

			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE1_01.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image8_SE1_02.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES3, "\\TestImages\\Image8_SE1_03.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES4, "\\TestImages\\Image8_SE1_04.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES5, "\\TestImages\\Image8_SE1_05.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE1_06.jpeg", w, h);

			yb = (LLB[k]-YB)*4;
			ye = (LLE[k]-YB)*4;
			xb = (LL[k]-XB)*4;
			xe = (LR[k]-XB)*4;

			memcpy(g_ImRES7, g_ImRES6, (w*h)*sizeof(int));
			//memcpy(g_ImRES9, g_ImRES1, (w*h)*sizeof(int));

			ClearImage4x4(g_ImRES1, w, h, r);
			N1 = ClearImageOptimal(g_ImRES1, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE1_012.jpeg", w, h);

			ClearImage4x4(g_ImRES2, w, h, r);
			N2 = ClearImageOptimal(g_ImRES2, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image8_SE1_022.jpeg", w, h);

			ClearImage4x4(g_ImRES3, w, h, r);
			N3 = ClearImageOptimal(g_ImRES3, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES3, "\\TestImages\\Image8_SE1_032.jpeg", w, h);

			ClearImage4x4(g_ImRES4, w, h, r);
			N4 = ClearImageOptimal(g_ImRES4, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES4, "\\TestImages\\Image8_SE1_042.jpeg", w, h);

			ClearImage4x4(g_ImRES5, w, h, r);
			N5 = ClearImageOptimal(g_ImRES5, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES5, "\\TestImages\\Image8_SE1_052.jpeg", w, h);

			ClearImage4x4(g_ImRES6, w, h, r);
			N6 = ClearImageOptimal(g_ImRES6, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE1_062.jpeg", w, h);

			maxN = 0;
			val = 1;

			if (N6 >= maxN) { maxN = N6; val = 6; }
			if (N5 >= maxN) { maxN = N5; val = 5; }
			if (N4 >= maxN) { maxN = N4; val = 4; }
			if (N3 >= maxN) { maxN = N3; val = 3; }
			if (N2 >= maxN) { maxN = N2; val = 2; }
			if (N1 >= maxN) { maxN = N1; val = 1; }

			if (val == 1)
			{
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];
					val2 = g_ImU[i];
					val3 = g_ImV[i];
					val4 = g_ImI[i];
					val5 = g_ImQ[i];

					if ( ( val1 >= j1_min ) && (val1 <= j1_max) &&
						( val2 >= j2_min ) && (val2 <= j2_max) &&
						( val3 >= j3_min ) && (val3 <= j3_max) &&
						( val4 >= j4_min ) && (val4 <= j4_max) &&
						( val5 >= j5_min ) && (val5 <= j5_max)
						) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}
			else if (val == 2)
			{
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];

					if ( ( val1 >= j1_min ) && (val1 <= j1_max) ) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}
			else if (val == 3)
			{
				val_min = j1_min-20;
				val_max = j1_max-20;
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];

					if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}
			else if (val == 4)
			{
				val_min = j1_min+20;
				val_max = j1_max+20;
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];

					if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}
			else if (val == 5)
			{
				val_min = j1_min;
				val_max = j1_max+20;
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];
					val2 = g_ImU[i];
					val3 = g_ImV[i];
					val4 = g_ImI[i];
					val5 = g_ImQ[i];

					if ( ( val1 >= val_min ) && (val1 <= val_max) &&
						( val2 >= j2_min ) && (val2 <= j2_max) &&
						( val3 >= j3_min ) && (val3 <= j3_max) &&
						( val4 >= j4_min ) && (val4 <= j4_max) &&
						( val5 >= j5_min ) && (val5 <= j5_max)
						) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}
			else if (val == 6)
			{
				val_min = j1_min-20;
				if (val_min > 70)
				{
					if (val_min-30 < 70) val_min = 70;
					else val_min -= 30;
				}
				val_max = j1_max;
				for (i=0; i<w*h; i++)
				{
					val1 = g_ImY[i];
					val2 = g_ImU[i];
					val3 = g_ImV[i];
					val4 = g_ImI[i];
					val5 = g_ImQ[i];

					if ( ( val1 >= val_min ) && (val1 <= val_max) &&
						( val2 >= j2_min ) && (val2 <= j2_max) &&
						( val3 >= j3_min ) && (val3 <= j3_max) &&
						( val4 >= j4_min ) && (val4 <= j4_max) &&
						( val5 >= j5_min ) && (val5 <= j5_max)
						) 
					{
						g_ImRES9[i] = r;
					}
					else
					{
						g_ImRES9[i] = 0;
					}
				}
			}

			if (maxN > 6) minN = (int)((double)maxN*0.9);
			else minN = maxN/2;

			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (g_ImRES1[i] != 0) ) ||
					( (N2 >= minN) && (g_ImRES2[i] != 0) ) ||
					( (N3 >= minN) && (g_ImRES3[i] != 0) ) ||
					( (N4 >= minN) && (g_ImRES4[i] != 0) ) ||
					( (N5 >= minN) && (g_ImRES5[i] != 0) ) ||
					( (N6 >= minN) && (g_ImRES6[i] != 0) )
					)
				{
					g_ImRES8[i] = r;
				}
				else
				{
					g_ImRES8[i] = 0;
				}
			}		

			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE1_RES00.jpeg", w, h);

			ClearImage4x4(g_ImRES8, w, h, r);
			
			LH = (LLE[k] - LLB[k])*4;
			val = ClearImageOpt2(g_ImRES8, w, h, r, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ);
			ddI = ddQ = max(min((ddI*4)/3, 20), ddI);
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE1_RES01.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImRES9, 0, (w*h)*sizeof(int));
				break;
			}

			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (g_ImRES1[i] != 0) ) ||
					( (N2 >= minN) && (g_ImRES2[i] != 0) ) ||
					( (N3 >= minN) && (g_ImRES3[i] != 0) ) ||
					( (N4 >= minN) && (g_ImRES4[i] != 0) ) ||
					( (N5 >= minN) && (g_ImRES5[i] != 0) ) ||
					( (N6 >= minN) && (g_ImRES6[i] != 0) )
					)
				{
					g_ImRES9[i] = r;
				}
			}

			ClearImage4x4(g_ImRES9, w, h, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES02.jpeg", w, h);

			for (i=0; i<w*h; i++)
			{
				if (g_ImRES8[i] != 0)
				{
					g_ImRES9[i] = r;
				}
			}
			
			ClearImage4x4(g_ImRES9, w, h, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES03.jpeg", w, h);

			ClearImageOpt5(g_ImRES9, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES04.jpeg", w, h);

			val = ClearImageLogical(g_ImRES9, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES05!.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImRES9, 0, (w*h)*sizeof(int));
				break;
			}

			memcpy(g_ImRES8, g_ImRES9, (w*h)*sizeof(int));
			for (i=0; i<w*h; i++)
			{
				if ( (N5 >= minN) && (g_ImRES5[i] != 0) )
				{
					g_ImRES8[i] = r;
				}
			}

			ClearImageOpt5(g_ImRES8, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			ClearImage4x4(g_ImRES8, w, h, r);
			ClearImageOpt5(g_ImRES8, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE1_RES06.jpeg", w, h);		

			val = ClearImageLogical(g_ImRES8, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE1_RES07!.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImRES9, 0, (w*h)*sizeof(int));
				break;
			}

			for (i=0; i<w*h; i++)
			{
				if ( (N4 >= minN) && (g_ImRES4[i] != 0) )
				{
					g_ImRES9[i] = r;
				}
			}

			ClearImageOpt5(g_ImRES9, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			ClearImage4x4(g_ImRES9, w, h, r);
			ClearImageOpt5(g_ImRES9, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES08.jpeg", w, h);		

			val = ClearImageLogical(g_ImRES9, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES09!.jpeg", w, h);

			for (i=0; i<w*h; i++)
			{
				if (g_ImRES8[i] != 0)
				{
					g_ImRES9[i] = r;
				}
			}
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES10.jpeg", w, h);
			ClearImageOpt5(g_ImRES9, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES11.jpeg", w, h);

			val = ClearImageLogical(g_ImRES9, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES12!.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImRES9, 0, (w*h)*sizeof(int));
				break;
			}

			ClearImageOpt5(g_ImRES4, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			ClearImage4x4(g_ImRES4, w, h, r);
			ClearImageOpt5(g_ImRES4, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);

			val1 = LH;
			val2 = LMAXY;
			val = ClearImageLogical(g_ImRES4, w, h, val1, val2, xb, xe, r);
			ClearImageSpecific2(g_ImRES4, w, h, LMAXY, LH, r);
			
			if (g_show_results == 1) SaveRGBImage(g_ImRES4, "\\TestImages\\Image8_SE1_RES13.jpeg", w, h);

			for (i=0; i<w*h; i++)
			{
				if (g_ImRES4[i] != 0)
				{
					g_ImRES9[i] = r;
				}
			}

			if (g_show_results == 1) SaveRGBImage(g_ImRES9, "\\TestImages\\Image8_SE1_RES14.jpeg", w, h);

			break;
		}
		///////////////////////////////////////////////////////////////////////

		//-----применяем второй метод отделения текста-----//
		while(1)
		{			
			j1_min = j1_min_prev;
			j1_max = j1_max_prev;

			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= j1_min ) && (val1 <= j1_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES1[i] = r;
				}
				else
				{
					g_ImRES1[i] = 0;
				}
			}

			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= j1_min ) && (val1 <= j1_max) ) 
				{
					g_ImRES2[i] = r;
				}
				else
				{
					g_ImRES2[i] = 0;
				}
			}

			val_min = j1_min-20;
			val_max = j1_max-20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
				{
					g_ImRES3[i] = r;
				}
				else
				{
					g_ImRES3[i] = 0;
				}
			}

			val_min = j1_min+20;
			val_max = j1_max+20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) ) 
				{
					g_ImRES4[i] = r;
				}
				else
				{
					g_ImRES4[i] = 0;
				}
			}

			val_min = j1_min;
			val_max = j1_max+20;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES5[i] = r;
				}
				else
				{
					g_ImRES5[i] = 0;
				}
			}

			val_min = j1_min-20;
			if (val_min > 70)
			{
				if (val_min-30 < 70) val_min = 70;
				else val_min -= 30;
			}
			val_max = j1_max;
			for (i=0; i<w*h; i++)
			{
				val1 = g_ImY[i];
				val1 = g_ImY[i];
				val2 = g_ImU[i];
				val3 = g_ImV[i];
				val4 = g_ImI[i];
				val5 = g_ImQ[i];

				if ( ( val1 >= val_min ) && (val1 <= val_max) &&
					( val2 >= j2_min ) && (val2 <= j2_max) &&
					( val3 >= j3_min ) && (val3 <= j3_max) &&
					( val4 >= j4_min ) && (val4 <= j4_max) &&
					( val5 >= j5_min ) && (val5 <= j5_max)
					) 
				{
					g_ImRES6[i] = r;//r
				}
				else
				{
					g_ImRES6[i] = 0;
				}
			}

			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE2_01.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image8_SE2_02.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES3, "\\TestImages\\Image8_SE2_03.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES4, "\\TestImages\\Image8_SE2_04.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES5, "\\TestImages\\Image8_SE2_05.jpeg", w, h);
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE2_06.jpeg", w, h);

			yb = (LLB[k]-YB)*4;
			ye = (LLE[k]-YB)*4;
			xb = (LL[k]-XB)*4;
			xe = (LR[k]-XB)*4;

			memcpy(g_ImRES7, g_ImRES4, (w*h)*sizeof(int));
			memcpy(g_ImFF, g_ImRES1, (w*h)*sizeof(int));

			N1 = ClearImageOptimal(g_ImRES1, w, h, yb, ye, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE2_012.jpeg", w, h);

			//ClearImage4x4(g_ImRES2, w, h, r);
			N2 = ClearImageOptimal(g_ImRES2, w, h, yb, ye, r);		
			if (g_show_results == 1) SaveRGBImage(g_ImRES2, "\\TestImages\\Image8_SE2_022.jpeg", w, h);

			//ClearImage4x4(g_ImRES3, w, h, r);
			N3 = ClearImageOptimal(g_ImRES3, w, h, yb, ye, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES3, "\\TestImages\\Image8_SE2_032.jpeg", w, h);

			//ClearImage4x4(g_ImRES4, w, h, r);
			N4 = ClearImageOptimal(g_ImRES4, w, h, yb, ye, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES4, "\\TestImages\\Image8_SE2_042.jpeg", w, h);

			//ClearImage4x4(g_ImRES4, w, h, r);
			N5 = ClearImageOptimal(g_ImRES5, w, h, yb, ye, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES5, "\\TestImages\\Image8_SE2_052.jpeg", w, h);
			
			ClearImage4x4(g_ImRES4, w, h, r);
			N6 = ClearImageOptimal(g_ImRES6, w, h, yb, ye, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE2_062.jpeg", w, h);

			minN = N5/2;

			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (g_ImRES1[i] != 0) ) ||
					 ( (N2 >= minN) && (g_ImRES2[i] != 0) ) ||
					 ( (N3 >= minN) && (g_ImRES3[i] != 0) ) ||
					 ( (N4 >= minN) && (g_ImRES4[i] != 0) ) ||
					 ( (N5 >= minN) && (g_ImRES5[i] != 0) ) ||
					 ( (N6 >= minN) && (g_ImRES6[i] != 0) )
				   )
				{
					g_ImRES8[i] = r;
				}
				else
				{
					g_ImRES8[i] = 0;
				}
			}		
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE2_RES00.jpeg", w, h);

			LH = (LLE[k] - LLB[k])*4;
			val = ClearImageOpt2(g_ImRES8, w, h, r, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ);
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE2_RES01.jpeg", w, h);

			ClearImageOpt5(g_ImRES6, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE2_RES02.jpeg", w, h);			

			if (val == 0)
			{
				memset(g_ImFF, 0, (w*h)*sizeof(int));
				break;
			}

			for (i=0; i<w*h; i++)
			{
				if (g_ImRES8[i] != 0)
				{
					g_ImFF[i] = r;
				}
			}			

			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES03.jpeg", w, h);			
			
			ClearImageOpt5(g_ImFF, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);

			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES04.jpeg", w, h);

			memcpy(g_ImRES8, g_ImFF, (w*h)*sizeof(int));

			N7 = ClearImageOpt5(g_ImRES7, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES7, "\\TestImages\\Image8_SE2_RES05.jpeg", w, h);

			for (i=0; i<w*h; i++)
			{
				if ( ( (N1 >= minN) && (g_ImRES1[i] != 0) ) ||
					 ( (N5 >= minN) && (g_ImRES5[i] != 0) ) ||
					 ( (N7 >= minN) && (g_ImRES7[i] != 0) )
					)
				{
					g_ImFF[i] = r;
				}
			}
			
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES06.jpeg", w, h);

			ClearImageOpt5(g_ImFF, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES07.jpeg", w, h);
			
			val = ClearImageLogical(g_ImFF, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES08!.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImFF, 0, (w*h)*sizeof(int));
				break;
			}

			val = ClearImageLogical(g_ImRES8, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES8, "\\TestImages\\Image8_SE2_RES09!.jpeg", w, h);

			val = ClearImageLogical(g_ImRES7, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES7, "\\TestImages\\Image8_SE2_RES10!.jpeg", w, h);

			val = ClearImageLogical(g_ImRES6, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES6, "\\TestImages\\Image8_SE2_RES11!.jpeg", w, h);

			for (i=0; i<w*h; i++)
			{
				if ( (g_ImRES7[i] != 0) ||
					 (g_ImRES6[i] != 0) )
				{
					g_ImFF[i] = r;
				}
			}
			val = ClearImageLogical(g_ImFF, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES12!.jpeg", w, h);
			
			for (i=0; i<w*h; i++)
			{
				if (g_ImRES8[i] != 0)
				{
					g_ImFF[i] = r;
				}
			}
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES13.jpeg", w, h);

			/*ClearImageOpt5(g_ImFF, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES08.jpeg", w, h);
			ClearImage4x4(g_ImFF, w, h, r);

			val = ClearImageLogical(g_ImFF, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES09!.jpeg", w, h);

			if (val == 0)
			{
				memset(g_ImFF, 0, (w*h)*sizeof(int));
				break;
			}

			ClearImageOpt5(g_ImRES1, w, h, LH, LMAXY, jY_min, jY_max, j4_min, j4_max, j5_min, j5_max, mY, dY, mI, dI, mQ, dQ, mmY, ddY1, ddY2, mmI, ddI, mmQ, ddQ, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE2_RES10.jpeg", w, h);

			ClearImageLogical(g_ImRES1, w, h, LH, LMAXY, xb, xe, r);
			if (g_show_results == 1) SaveRGBImage(g_ImRES1, "\\TestImages\\Image8_SE2_RES11!.jpeg", w, h);
			
			for (i=0; i<w*h; i++)
			{
				if (g_ImRES1[i] != 0)
				{
					g_ImFF[i] = r;
				}
			}
			if (g_show_results == 1) SaveRGBImage(g_ImFF, "\\TestImages\\Image8_SE2_RES12.jpeg", w, h);*/

			break;
		}
		///////////////////////////////////////////////////////////////////////

		if (g_hard_sub_mining == true)
		{
			bln = 0;

			yb = (LLB[k]-YB)*4;
			ye = (LLE[k]-YB)*4;
			xb = (LL[k]-XB)*4;
			xe = (LR[k]-XB)*4;

			int n = (int)ceil((double)(xe-xb+1)/(25*4));
			val = (xe-xb+1)/n;

			xe = xb+val;
			for (i=0; i<n; i++, xb+=val, xe+= val)
			{
				cnt1 = 0;
				cnt2 = 0;

				for(y=yb; y<ye; y++)
				for(x=xb; x<xe; x++)
				{
					j = y*w + x;

					if (g_ImRES9[j] != 0) cnt1++;					
					if (g_ImFF[j] != 0) cnt2++;					
				}

				if (cnt2 < cnt1)
				{
					bln = 1;
					break;
				}
			}

			if (bln == 1)
			{
				memcpy(g_ImFF, g_ImRES9, (w*h)*sizeof(int));
			}
			else
			{
				for (i=0; i<w*h; i++)
				{
					if (g_ImRES9[i] != 0)
					{
						g_ImFF[i] = r;
					}
				}
			}
		}

		
        g_pViewRGBImage(g_ImFF, w, h);

		ww = W*4;
		hh = h;

		memset(g_ImRES1, 0, (ww*hh)*sizeof(int));

		for(y=0, i=0; y<h; y++)
		for(x=0; x<w; x++, i++)
		{
			j = y*ww + (XB*4) + x;

			if (g_ImFF[i] != 0) g_ImRES1[j] = 255;
		}

		GetTextLineParameters(g_ImFF, w, h, LH, LMAXY, DXB, DXE, DYB, DYE, mY, mI, mQ, r);

        FullName = string("/TXTImages/");
		FullName += SaveName;

		sprintf(str, "%.2d", (int)SavedFiles.size() + 1);
		FullName += string("_");
        FullName += string(str);

		FullName += string(".jpeg");

		SaveTextLineParameters(	FullName, YB, 
								LH/4, YB + LMAXY/4, 
								XB + DXB/4, XB + DXE/4,
								YB + DYB/4, YB + DYE/4,
								mY, mI, mQ );

		SavedFiles.push_back(FullName);

		SaveImage(g_ImRES1, FullName, ww, hh, -1, 300);

		res = 1;
	}

	return res;
}

int AnalizeAndClearImage(int *Im, int *ImGR, int w, int h, int j1_min, int j1_max, int r, int g, int yb, int ye, int xb, int xe, int &cnt1, int &cnt2)
{
	int i, x, y, val, ib, val1, val2, N;

	for (i=0; i< w*h; i++)
	{
		val = ImGR[i];

		if ( ( val >= j1_min ) && (val <= j1_max) )
		{
			Im[i] = r;
		}
		else
		{
			Im[i] = g;
		}
	}
	if (g_debug == 1) SaveRGBImage(Im, "\\TestImages\\Image8_RG.jpeg", w, h);

	ClearImageDetailed(Im, w, h, yb, ye, r);
	ClearImageDetailed(Im, w, h, yb, ye, g);
	
	ClearImage4x4(Im, w, h, r);
	ClearImage4x4(Im, w, h, g);

	N = ClearImageDetailed(Im, w, h, yb, ye, r);
	ClearImageDetailed(Im, w, h, yb, ye, g);

	if (g_debug == 1) SaveRGBImage(Im, "\\TestImages\\Image8_RGF.jpeg", w, h);

	val1 = (int)((double)(ye-yb+1)*0.3);
	val2 = (int)((double)(xe-xb+1)*0.1);
	yb += val1;
	ye -= val1;
	xb += val2;
	xe -= val2;

	cnt1 = 0;
	cnt2 = 0;
	for (y=yb, ib=yb*w; y<=ye; y++, ib+=w)
	for (x=xb, i=ib+xb; x<=xe; x++, i++)
	{
		if (g_ImRES3[i] != 0)
		{
			if (g_ImRES1[i] == r) cnt1++;
			if (g_ImRES1[i] == g) cnt2++;
		}
	}

	return N;
}

void ClearImage4x4(int *Im, int w, int h, int white)
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

int ClearImage(int *Im, int w, int h, int yb, int ye, int white)
{
	CMyClosedFigure *pFigures, *pFigure;
	int i, l, ii, val, N;
	int res;
	CMyPoint *PA;
	clock_t t;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);
	
	if (N == 0)	return 0;

	res = N;
	val = yb+ye;
	
	for(i=0; i<N; i++)
	{
		pFigure = &(pFigures[i]);

		if	(	(pFigure->m_minX <= 2) ||
				(pFigure->m_maxX >= (w-1)-2) ||
				(pFigure->m_minY <= 2) ||
				(pFigure->m_maxY >= (h-1)-2) || 
				(pFigure->m_w >= h) ||
				(
					( (pFigure->m_h >= h/5) || (pFigure->m_w >= 25*4) ) &&
					( ((val-(pFigure->m_minY+pFigure->m_maxY)) >= h/3) || (((pFigure->m_minY+pFigure->m_maxY)-val) >= h/3) )
				)
			)
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = (PA[l].m_y*w)+PA[l].m_x;
				Im[ii] = 0;
			}

			res--;
		}
	}
	
	delete[] pFigures;

	return res;
}

int ClearImageDetailed(int *Im, int w, int h, int yb, int ye, int white)
{
	CMyClosedFigure *pFigures, *pFigure, **ppFigures;
	int i, l, ii, val, N;
	CMyPoint *PA;
	clock_t t;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);
	val = yb+ye;
	
	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if	(	(pFigure->m_minX <= 2) ||
				(pFigure->m_maxX >= (w-1)-2) ||
				(pFigure->m_minY <= 2) ||
				(pFigure->m_maxY >= (h-1)-2) ||
				(pFigure->m_w >= h) ||
				(pFigure->m_w > 30*4) ||
				(
					( (pFigure->m_h >= h/5) || (pFigure->m_w >= 25*4) ) &&
					( ((val-(pFigure->m_minY+pFigure->m_maxY)) >= h/3) || (((pFigure->m_minY+pFigure->m_maxY)-val) >= h/3) )
				) ||
				//( (double)pFigure->m_Square/(pFigure->m_w*pFigure->m_h) > 0.75 ) ||
				( (pFigure->m_minY <= 0.1*h) && (pFigure->m_maxY >= 0.75*h) ) ||
				(pFigure->m_maxY < val/2) ||
				(pFigure->m_minY > val/2)
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
	
	int min_h = (int)((double)(ye-yb+1)*0.6);

	l = 0;
	for (i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if (pFigure->m_h >= min_h) l++;
	}

	delete[] pFigures;
	delete[] ppFigures;

	return l;
}

int ClearImageOptimal(int *Im, int w, int h, int yb, int ye, int white)
{
	CMyClosedFigure *pFigures=NULL, *pFigure, **ppFigures=NULL;
	int i, j, k, l, ii, val, N, NNY, min_h;
	int val1, val2;
	int *maxY = NULL, *NN = NULL, *NY = NULL; 
	CMyPoint *PA;
	clock_t t;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);
	val = yb+ye;

	if (N == 0)	return 0;

	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}
	
	min_h = (int)((double)(ye-yb+1)*0.4);
	
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if	(	(pFigure->m_minX <= 2) ||
				(pFigure->m_maxX >= (w-1)-2) ||
				(pFigure->m_minY <= 2) ||
				(pFigure->m_maxY >= (h-1)-2) ||
				(pFigure->m_w >= h) ||
				(pFigure->m_w > 40*4) ||
				(
					( (pFigure->m_h >= h/5) || (pFigure->m_w >= 25*4) ) &&
					( ((val-(pFigure->m_minY+pFigure->m_maxY)) >= h/3) || (((pFigure->m_minY+pFigure->m_maxY)-val) >= h/3) )
				) ||
				( (pFigure->m_minY <= 0.1*h) && (pFigure->m_maxY >= 0.75*h) ) ||
				(pFigure->m_maxY < val/2) ||
				(pFigure->m_minY > val/2) ||
				(pFigure->m_h < min_h)
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
		if (pFigures != NULL) delete[] pFigures;
		if (ppFigures != NULL) delete[] ppFigures;

		return 0;
	}

	maxY = new int[N];
	NN = new int[N];
	NY = new int[N];

	min_h = (int)((double)(ye-yb+1)*0.6);
	val1 = (yb+ye)/2 - (int)((double)(ye-yb+1)*0.2);
	val2 = (yb+ye)/2 + (int)((double)(ye-yb+1)*0.1);

	for(i=0, j=0; i < N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_h >= min_h) &&
			 (pFigure->m_minY < val1) &&
			 (pFigure->m_maxY > val2) )
		{
			maxY[j] = pFigure->m_maxY;
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

	if (maxY != NULL) delete[] maxY;
	if (NN != NULL) delete[] NN;
	if (NY != NULL) delete[] NY;

	if (pFigures != NULL) delete[] pFigures;
	if (ppFigures != NULL) delete[] ppFigures;

	return val;
}

int ClearImageOpt2(int *Im, int w, int h, int white, int &LH, int &LMAXY, 
					int &jY_min, int &jY_max, int &jI_min, int &jI_max, int &jQ_min, int &jQ_max,
					int &mY, int &dY, int &mI, int &dI, int &mQ, int &dQ,
					int &mmY, int &ddY1, int &ddY2, int &mmI, int &ddI, int &mmQ, int &ddQ)
{
	CMyClosedFigure *pFigures, **ppFigures, *pFigure;
	int i, j, k, l, ii, val, N, minN, H;
	int GRStr[256*2], delta, smax[256*2], smaxi[256*2], NNN, jY, jI, jQ;
	int val1, val2, val3;
	int min_h;
	int res;

	int *maxY;
	int *NN, *NY, NNY;

	CMyPoint *PA;
	clock_t t;
	int dmaxy = g_dmaxy;

	res = 0;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)	return res;

	if (N < 2)
	{
		delete[] pFigures;
		return res;
	}

	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	maxY = new int[N];
	NN = new int[N];
	NY = new int[N];

	min_h = (int)(0.6*(double)LH);

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

	j=0;
	k=0;
	i=0;
	while(i < NNY)
	{
		if ((maxY[j]-maxY[i]) > dmaxy)
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

	LMAXY = val = NY[j];

	delete[] maxY;
	delete[] NN;
	delete[] NY;

	if (val < 2)
	{
		delete[] pFigures;
		delete[] ppFigures;		

		return res;
	}
	
	H = 0;
	k = 0;
	i = 0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if	( (pFigure->m_maxY < LMAXY-dmaxy)
			)
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}
			ppFigures[i] = ppFigures[N-1];
			N--;
			continue;
		}
		else
		{
			if ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_h >= 0.6*LH) )
			{
				H += pFigure->m_h;
				k++;
			}
		}

		i++;
	}

	if (k < 1)
	{
		delete[] pFigures;
		delete[] ppFigures;

		return res;
	}

	LH = H/k;

	memset(g_ImRR, 0, (w*h)*sizeof(int));
	k = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) && (pFigure->m_h >= 0.6*LH) ) 
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				g_ImRR[PA[l].m_i] = 255;
			}
				
			ppFigures[k] = pFigure;
			k++;
		}
	}
	N = k;

	for(i=0; i<k; i++)
	{
		pFigure = ppFigures[i];

		delta = 80;
		StrAnalyseImage(pFigure, g_ImY, GRStr, 0);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 0);
		FindMaxStr(smax, smaxi, jY, val, NNN);
		pFigure->m_mmY = jY;

		delta = 5;
		StrAnalyseImage(pFigure, g_ImI, GRStr, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
		FindMaxStr(smax, smaxi, jI, val, NNN);
		jI -= 256;
		pFigure->m_mmI = jI;
		
		delta = 5;
		StrAnalyseImage(pFigure, g_ImQ, GRStr, 256);
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
		FindMaxStr(smax, smaxi, jQ, val, NNN);
		jQ -= 256;
		pFigure->m_mmQ = jQ;
	}

	minN = (4*N)/5;
	if (minN < 3) minN = 3;
	if (minN > N) minN = N;

	delta = 4;
	memset(GRStr, 0, 512*sizeof(int));
	for(i=0; i<k; i++) GRStr[ppFigures[i]->m_mmI+256]++;
	do
	{
		delta += 2;
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
		FindMaxStr(smax, smaxi, jI, val, NNN);
	} 
	while(val < minN);
	
	jI -= 256;
	i = 0;
	while(i < k)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_mmI < jI) || (pFigure->m_mmI >= jI+delta) )
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				g_ImRR[PA[l].m_i] = 0;
			}
				
			ppFigures[i] = ppFigures[k-1];
			k--;

			continue;
		}

		i++;
	}

	delta = 4;
	memset(GRStr, 0, 512*sizeof(int));
	for(i=0; i<k; i++) GRStr[ppFigures[i]->m_mmQ+256]++;
	do
	{
		delta += 2;
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
		FindMaxStr(smax, smaxi, jQ, val, NNN);
	} 
	while(val < minN);
	
	jQ -= 256;
	i = 0;
	while(i < k)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_mmQ < jQ) || (pFigure->m_mmQ >= jQ+delta) )
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				g_ImRR[PA[l].m_i] = 0;
			}
				
			ppFigures[i] = ppFigures[k-1];
			k--;

			continue;
		}
		
		i++;
	}

	/*delta = 60;
	memset(GRStr, 0, 256*sizeof(int));
	for(i=0; i<k; i++) GRStr[ppFigures[i]->m_mmY]++;
	do
	{
		delta += 2;
		FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 0);
		FindMaxStr(smax, smaxi, jY, val, NNN);
	} 
	while(val < ((4*N)/5));*/
	
	mmI = 0;
	mmQ = 0;
	mmY = 0;
	H = 0;
	i = 0;
	while(i < k)
	{
		pFigure = ppFigures[i];

		/*if ( (pFigure->m_mmY < jY) || (pFigure->m_mmY >= jY+delta) )
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				g_ImR[PA[l].m_i] = 0;
			}
				
			ppFigures[i] = ppFigures[k-1];
			k--;

			continue;
		}
		else*/
		{
			mmI += pFigure->m_mmI;
			mmQ += pFigure->m_mmQ;
			mmY += pFigure->m_mmY;
			H += pFigure->m_h;
		}

		i++;
	}
	mmI = mmI/k;
	mmQ = mmQ/k;
	mmY = mmY/k;
	LH = H/k;

	ddY1 = 0;
	ddY2 = 0;
	ddI = 0;
	ddQ = 0;
	for(i=0; i<k; i++)
	{
		pFigure = ppFigures[i];
			
		val = pFigure->m_mmY - mmY;
		if (val < ddY1) ddY1 = val;
		if (val > ddY2) ddY2 = val;

		val = pFigure->m_mmI - mmI;
		if (val < 0) val = -val;
		if (val > ddI) ddI = val;

		val = pFigure->m_mmQ - mmQ;
		if (val < 0) val = -val;
		if (val > ddQ) ddQ = val;
	}
	ddY1 = ddY1*2;
	if (ddY1 > -15) ddY1 = -15;
	ddY2 = ddY2*2;
	if (ddY2 < 45) ddY2 = 45;

	ddI = ddI*3;
	if (ddI < g_min_ddI) ddI = g_min_ddI;

	ddQ = ddQ*3;
	if (ddQ < g_min_ddQ) ddQ = g_min_ddI;

	ddI = ddQ = max(ddI, ddQ);

	delta = 30;
	StrAnalyseImage(g_ImRR, g_ImY, GRStr, w, h, 0, w-1, 0, h-1, 0);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 0);
	FindMaxStr(smax, smaxi, jY, val, NNN);
	jY_min = jY;
	jY_max = jY_min+delta-1;

	delta = 30;
	StrAnalyseImage(g_ImRR, g_ImI, GRStr, w, h, 0, w-1, 0, h-1, 256);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
	FindMaxStr(smax, smaxi, jI, val, NNN);
	jI_min = jI-256;
	jI_max = jI_min+delta-1;

	delta = 30;
	StrAnalyseImage(g_ImRR, g_ImQ, GRStr, w, h, 0, w-1, 0, h-1, 256);
	FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
	FindMaxStr(smax, smaxi, jQ, val, NNN);
	jQ_min = jQ-256;
	jQ_max = jQ_min+delta-1;

	mI = 0;
	mQ = 0;
	mY = 0;
	for(i=0; i<k; i++)
	{
		pFigure = ppFigures[i];
		
		PA = pFigure->m_PointsArray;
		
		val1 = 0;
		val2 = 0;
		val3 = 0;
		for(l=0; l < pFigure->m_Square; l++)
		{
			ii = PA[l].m_i;
			val1 += g_ImI[ii];
			val2 += g_ImQ[ii];
			val3 += g_ImY[ii];
		}
			
		pFigure->m_mI = val1/pFigure->m_Square;
		pFigure->m_mQ = val2/pFigure->m_Square;
		pFigure->m_mY = val3/pFigure->m_Square;
		mI += pFigure->m_mI;
		mQ += pFigure->m_mQ;
		mY += pFigure->m_mY;
	}
	mI = mI/k;
	mQ = mQ/k;
	mY = mY/k;

	dI = 0;
	dQ = 0;
	dY = 0;
	for(i=0; i<k; i++)
	{
		pFigure = ppFigures[i];
			
		val = pFigure->m_mI - mI;
		if (val < 0) val = -val;
		if (val > dI) dI = val;

		val = pFigure->m_mQ - mQ;
		if (val < 0) val = -val;
		if (val > dQ) dQ = val;

		val = pFigure->m_mY - mY;
		if (val < 0) val = -val;
		if (val > dY) dY = val;
	}
	dY = dY*2;
	if (dY < 45) dY = 45;

	dI = dI*3;
	if (dI < g_min_dI) dI = g_min_dI;

	dQ = dQ*3;
	if (dQ < g_min_dQ) dQ = g_min_dQ;

	dI = dQ = max(dI, dQ);

	ddI = ddQ = max(dI, ddI);

	delete[] pFigures;
	delete[] ppFigures;

	res = 1;

	return res;
}

void ClearImageSpecific1(int *Im, int w, int h, int yb, int ye, int xb, int xe, int white)
{
	CMyClosedFigure *pFigures, *pFigure;
	CMyPoint *PA;
	clock_t t;
	int bln, dh, i, l, N;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);
	
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

	delete[] pFigures;
}

void ClearImageSpecific2(int *Im, int w, int h, int LMAXY, int LH, int white)
{
	CMyClosedFigure *pFigures, **ppFigures, *pFigure;
	int i, l, N;
	CMyPoint *PA;
	clock_t t;
	int dmaxy = g_dmaxy;
	int min_h = (int)((double)LH*0.6);

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)
	{
		return;
	}

	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	i = 0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if	( !( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY-dmaxy) &&
				 (pFigure->m_h >= min_h) )
		    )
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				Im[PA[l].m_i] = 0;
			}
			ppFigures[i] = ppFigures[N-1];
			N--;
			continue;
		}

		i++;
	}

	delete[] pFigures;
	delete[] ppFigures;
}

void ClearImageSpecific(int *Im, int w, int h, int white)
{
	CMyClosedFigure *pFigures, **ppFigures, *pFigure;
	int i, j, k, l, ii, val, N, H, LMAXY;
	int *maxY;
	int *NN, *NY;
	CMyPoint *PA;
	clock_t t;
	int dmaxy = g_dmaxy;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)	return;

	if (N <= 2)
	{
		delete[] pFigures;
		return;
	}

	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	maxY = new int[N];
	NN = new int[N];
	NY = new int[N];

	for(i=0; i<N; i++)
	{
		maxY[i] = ppFigures[i]->m_maxY;
	}

	for(i=0; i<N-1; i++)
	{
		for(j=i+1; j<N; j++)
		{
			if(maxY[j] > maxY[i])
			{
				val = maxY[i];
				maxY[i] = maxY[j];
				maxY[j] = val;
			}
		}
	}

	j=0;
	k=0;
	i=0;
	while(i < N)
	{
		if ((maxY[j]-maxY[i]) > dmaxy)
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

	LMAXY = val = NY[j];
	H = 0;
	k = 0;
	i = 0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if	( (pFigure->m_maxY < LMAXY-dmaxy) ||
			  (pFigure->m_maxY > LMAXY)
			)
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}
			ppFigures[i] = ppFigures[N-1];
			N--;
			continue;
		}
		else
		{
			if (pFigure->m_maxY <= LMAXY)
			{
				H += pFigure->m_h;
				k++;
			}
		}

		i++;
	}

	delete[] pFigures;
	delete[] ppFigures;
	delete[] maxY;
	delete[] NN;
	delete[] NY;
}

void ClearImageOpt3(int *Im, int w, int h, int LH, int LMAXY, int jI_min, int jI_max, int jQ_min, int jQ_max, int white)
{
	CMyClosedFigure *pFigures, *pFigure;
	int i, l, ii, val, valI, valQ, N, bln;
	CMyPoint *PA;
	clock_t t;
	int dmaxy = g_dmaxy;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)	return;

	val = 2*LMAXY - LH;
	
	for(i=0; i<N; i++)
	{
		pFigure = &(pFigures[i]);

		if	(	(pFigure->m_minX <= 2) ||
				(pFigure->m_maxX >= (w-1)-2) ||
				(pFigure->m_minY <= 2) ||
				(pFigure->m_maxY >= (h-1)-2) || 
				( (pFigure->m_w >= LH*3) && 
				  ( (pFigure->m_maxY > LMAXY) || (pFigure->m_maxY < LMAXY-dmaxy) ) 
				) ||
				(
					( ((val-(pFigure->m_minY+pFigure->m_maxY)) > LH*2) || (((pFigure->m_minY+pFigure->m_maxY)-val) > LH*2) )
				)
			)
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = (PA[l].m_y*w)+PA[l].m_x;
				Im[ii] = 0;
			}
		}
		else
		{
			PA = pFigure->m_PointsArray;
			
			bln = 0;
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = (PA[l].m_y*w)+PA[l].m_x;
				valI = g_ImI[ii];
				valQ = g_ImQ[ii];

				if ( (valI>=jI_min) && (valI<=jI_max) &&
					 (valQ>=jQ_min) && (valQ<=jQ_max)
					)
				{
					bln = 1;
					break;
				}
			}

			if (bln == 0)
			{
				for(l=0; l < pFigure->m_Square; l++)
				{
					ii = (PA[l].m_y*w)+PA[l].m_x;
					Im[ii] = 0;
				}
			}
		}
	}
	
	delete[] pFigures;
}

int IsPoint(CMyClosedFigure *pFigure, int LMAXY, int LLH)
{
	int ret;
	double dval;
	
	ret = 0;

	if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
	else dval = (double)pFigure->m_w/pFigure->m_h;

	if ( (pFigure->m_w >= g_minpw) && (pFigure->m_w <= g_maxpw) &&
	     (pFigure->m_h >= g_minph) && (pFigure->m_h <= g_maxph) && 
	     (dval >= g_minpwh) ) 
	{
		if ( ( (pFigure->m_maxY <= LMAXY) && 
			   (pFigure->m_maxY >= LMAXY-g_dmaxy) ) ||
			 ( (pFigure->m_maxY <= LMAXY-LLH) && 
			   (pFigure->m_maxY >= LMAXY-LLH*1.25) )
			)
		{
			ret = 1;
		}
	}

	return ret;
}

int IsComma(CMyClosedFigure *pFigure, int LMAXY, int LLH)
{
	int ret;
	double dval;
	
	ret = 0;

	if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
	else dval = (double)pFigure->m_w/pFigure->m_h;

	if ( (pFigure->m_w >= g_minpw) && (pFigure->m_w <= g_maxpw+4) &&
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
	
	fname = g_dir + string("\\text_lines.info");
	fout.open(fname.c_str(), ios::out | ios::app);

	fout << ImageName << " = " << PropString << '\n';

	fout.close();
}

void GetSymbolAvgColor(CMyClosedFigure *pFigure)
{
	int *pImage;
	int *pImageY;
	int *pImageI;
	int *pImageQ;
	CMyPoint *PA;
	int i, ii, j, w, h, x, y, xx, yy, val;
	int r, min_x, max_x, min_y, max_y, mY, mI, mQ, weight;

	w = pFigure->m_maxX - pFigure->m_minX + 1;
	h = pFigure->m_maxY - pFigure->m_minY + 1;

	r = max(8, h/6);

	pImage = new int[w*h];
	pImageY = new int[w*h];
	pImageI = new int[w*h];
	pImageQ = new int[w*h];

	memset(pImage, 0, w*h*sizeof(int));

	PA = pFigure->m_PointsArray;

	for(i=0; i < pFigure->m_Square; i++)
	{
		ii = PA[i].m_i;
		j = (PA[i].m_y - pFigure->m_minY)*w + (PA[i].m_x - pFigure->m_minX);

		pImage[j] = 1;
		pImageY[j] = g_ImY[ii];
		pImageI[j] = g_ImI[ii];
		pImageQ[j] = g_ImQ[ii];
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

	delete[] pImage;
	delete[] pImageY;
	delete[] pImageI;
	delete[] pImageQ;
}

void GetTextLineParameters(int *Im, int w, int h, int &LH, int &LMAXY, int &XB, int &XE, int &YB, int &YE, int &mY, int &mI, int &mQ, int white)
{
	CMyClosedFigure *pFigures = NULL, **ppFigures = NULL, *pFigure = NULL;
	CMyPoint *PA = NULL;
	int i, j, k, l, N, val, val1, val2, val3, val4;
	int *maxY = NULL, *NN = NULL, *NY = NULL, *NH = NULL, NNY, min_h, min_w, prev_min_w;
	int dmaxy = g_dmaxy;
	int dminy = g_dminy;
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

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)
	{
		return;
	}

	ppFigures = new CMyClosedFigure*[N];
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
			GetSymbolAvgColor(pFigure);

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

	maxY = new int[N];
	NN = new int[N];
	NY = new int[N];

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

	// отыскиваем группы символов, чья высота различается не более чем на dmaxy по всевозможным высотам
	// (такие группы могут частично содержать одни и теже символы)
	j=0;
	k=0;
	i=0;
	while(i < NNY)
	{
		if ((maxY[j]-maxY[i]) > dmaxy)
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
		val = maxY[NNY-1] + dmaxy;
		j = NNY-2;

		while ((j >= 0) && (maxY[j] <= val)) j--;
		j++;
		
		LMAXY = NY[j];
	}

	delete[] maxY;
	delete[] NN;
	delete[] NY;

	if (val == 0)
	{
		delete[] pFigures;
		delete[] ppFigures;

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
			 (pFigure->m_maxY >= LMAXY-dmaxy) &&
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
			(pFigure->m_maxY >= LMAXY-dmaxy) &&
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

	delete[] pFigures;
	delete[] ppFigures;
}

int ClearImageLogical(int *Im, int w, int h, int &LH, int &LMAXY, int xb, int xe, int white)
{
	CMyClosedFigure *pFigures = NULL, **ppFigures = NULL, *pFigure = NULL, *pFigure2 = NULL, **ppFgs = NULL;
	int i, ib, i1, i2, i3, j, k, l, x, y, val, val1, N, bln, bln1, bln2, bln3, LMINY, LM1, LM2;
	int res, is_point, is_comma, LLH;
	CMyPoint *PA = NULL, *PA1 = NULL, *PA2 = NULL;
	clock_t t;
	int dmaxy = g_dmaxy;
	int dminy = g_dminy;
	int minpw, maxpw, minph, maxph; 
	double minpwh;
	double dval;

	int *maxY;
	int *NN = NULL, *NY = NULL, *NH = NULL, NNY, min_h, H;

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

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)	return res;

	ppFigures = new CMyClosedFigure*[N];
	ppFgs = new CMyClosedFigure*[N];
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
	maxY = new int[N];
	NN = new int[N];
	NY = new int[N];

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

	// отыскиваем группы символов, чья высота различается не более чем на dmaxy по всевозможным высотам
	// (такие группы могут частично содержать одни и теже символы)
	j=0;
	k=0;
	i=0;
	while(i < NNY)
	{
		if ((maxY[j]-maxY[i]) > dmaxy)
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

	delete[] maxY;
	delete[] NN;
	delete[] NY;

	if (val == 0)
	{
		delete[] pFigures;
		delete[] ppFigures;
		delete[] ppFgs;

		return res;
	}
	//--------------------

	//--------------------
	H = 0;
	j = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-dmaxy) &&
             (pFigure->m_h >= 0.6*LH) )
		{
			H += pFigure->m_h;
			j++;
		}
	}

	if (j == 0) return res;

	LH = H/j;
	//--------------------

	LMINY = LMAXY - (int)((double)LH*1.25);
	LM1 = LMAXY - LH*2;
	LM2 = LMAXY - LH/2;
	
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		is_point = IsPoint(pFigure, LMAXY, LH);
		is_comma = IsComma(pFigure, LMAXY, LH);

		if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
		else dval = (double)pFigure->m_w/pFigure->m_h;

		if ( ( (pFigure->m_maxY < LMAXY-LH) && 
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

		is_point = IsPoint(pFigure, LMAXY, LH);
		is_comma = IsComma(pFigure, LMAXY, LH);

		if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
		else dval = (double)pFigure->m_w/pFigure->m_h;
		
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (  ( (pFigure2->m_maxY <= LMAXY) && (pFigure2->m_maxY >= LMAXY-dmaxy) ) ||
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
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) ) ||
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
				if ( !( (pFigure->m_minY >= LM1) && (pFigure->m_maxY <= LM2) && (k==2) && (pFigure->m_w <= maxpw) ) )
				{
					bln = 1;
				}
			}
		}
		else if(k == 1)
		{						
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) ) ||
					( (pFigure->m_minY*2 < 2*LMAXY-LH) && (pFigure->m_maxY > LMAXY) )
				 ) &&
				 (pFigure->m_h >= 0.6*LH)
				)
			{
				if ( !( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) ) &&
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

				bln1 = ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) &&
						 (pFigure->m_w >= minpw) && (pFigure->m_w <= maxpw) && 
					     (pFigure->m_h >= minph) && (pFigure->m_h <= maxph) && 
				         (dval >= minpwh) );

				if ( ( (pFigure->m_minY >= ppFgs[0]->m_minY) && (pFigure->m_maxY <= ppFgs[0]->m_maxY) && (bln1 == 0) ) ||
					 ( (pFigure->m_minY < ppFgs[0]->m_minY) && (pFigure->m_maxY > ppFgs[0]->m_maxY) ) ||
					 (pFigure->m_minY > LMAXY) ||
					 (pFigure->m_maxY < LMINY) ||
					 ( (pFigure->m_h >= 0.6*LH) && !( (pFigure->m_maxY < LMAXY) && (pFigure->m_minY <= LMAXY-LH) && (pFigure->m_h <= LH*1.5) && (pFigure->m_maxX > ppFgs[0]->m_maxX) ) ) ||
					 ( (pFigure->m_h < pFigure->m_w) && 
					   ( (pFigure->m_maxY > LMAXY) || (pFigure->m_minY > LMAXY-0.25*LH) ) &&
					   !( (pFigure->m_w >= minpw) && (pFigure->m_w <= maxpw) &&
					      (pFigure->m_h >= minph) && (pFigure->m_h <= maxph) && 
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
			if ( (  ( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) ) ||
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
					   !( (pFigure->m_w >= minpw) && (pFigure->m_w <= maxpw) &&
				          (pFigure->m_h >= minph) && (pFigure->m_h <= maxph) && 
				          (dval >= minpwh) 
						)
					 )
					)
				{
					bln = 1;
				}
			}
		}

		if ( (pFigure->m_h < minph) || ( (pFigure->m_w < minpw) && ( pFigure->m_h < 2*pFigure->m_w) ) ||
			 ( (pFigure->m_w < minpw) && 
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

	/*j = 0;
	val1 = 0;
	int val2 = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		val = (pFigure->m_minX+pFigure->m_maxX)/2;

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-dmaxy) &&
             (pFigure->m_h >= 0.6*LH) &&
			 (val > xb) &&
			 (val < xe) )
		{
			val1 += pFigure->m_maxY;
			if (pFigure->m_maxY > val2) val2 = pFigure->m_maxY;
			j++;
		}
	}
	val1 = val1/j;
			
	int val3 = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		val = (pFigure->m_minX+pFigure->m_maxX)/2;

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-dmaxy) &&
             (pFigure->m_h >= 0.6*LH) &&
			 (val > xb) &&
			 (val < xe) )
		{
			val = pFigure->m_maxY-val1;
			val3 += val*val;
		}
	}
	val = ceil(2.0*sqrt((double)val3/(double)j));
	if (val < 1) val = 1;

	LMAXY = min((val1+val), val2);
	dmaxy = min((LMAXY-(val1-val)), dmaxy);

	val3 = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		val = (pFigure->m_minX+pFigure->m_maxX)/2;

		if ( (pFigure->m_maxY <= LMAXY) && 
			 (pFigure->m_maxY >= LMAXY-dmaxy) &&
             (pFigure->m_h >= 0.6*LH) &&
			 (val > xb) &&
			 (val < xe) )
		{
			val = pFigure->m_maxY-val1;
			val3 += val*val;
		}
	}
	val = ceil(2.0*sqrt((double)val3/(double)j));
	if (val < 1) val = 1;

	LMAXY = min((val1+val), val2);
	dmaxy = min((LMAXY-(val1-val)), dmaxy);*/

	min_h = (int)((double)LH*0.6);

	//-----очищаем с левого края-----//
	i=0;
	while(i < N)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY < LMAXY-dmaxy*1.5) ||
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
				 (pFigure->m_maxY >= LMAXY-dmaxy) &&
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
		is_comma = IsComma(pFigure, LMAXY, LH);

		if (pFigure->m_h < pFigure->m_w) dval = (double)pFigure->m_h/pFigure->m_w;
		else dval = (double)pFigure->m_w/pFigure->m_h;

		if ( (pFigure->m_minY < LMAXY - 2*LH) ||
		     ( (pFigure->m_h < min_h) && 
			   !( (pFigure->m_maxY <= LMAXY) && (pFigure->m_maxY >= LMAXY-dmaxy) &&
				  (pFigure->m_w >= minpw) && (pFigure->m_w <= maxpw) &&
				  (pFigure->m_h >= minph) && (pFigure->m_h <= maxph) && 
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
				 (pFigure->m_maxY >= LMAXY-dmaxy) &&
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
	NH = new int[N];

	k = 0;
	for(i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_maxY <= LMAXY) && 
				 (pFigure->m_maxY >= LMAXY-dmaxy) &&
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

	delete[] NH;

	//-----финальная упорядоченная очистка-----//
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];
		PA1 = pFigure->m_PointsArray;

		is_point = IsPoint(pFigure, LMAXY, LLH);
		is_comma = IsComma(pFigure, LMAXY, LLH);
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (pFigure2->m_maxY <= LMAXY) && 
				 (pFigure2->m_maxY >= LMAXY-dmaxy) &&
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

		is_point = IsPoint(pFigure, LMAXY, LLH);
		is_comma = IsComma(pFigure, LMAXY, LLH);
		bln = 0;
		k = 0;

		for(j=0; j<N; j++)
		{
			if (j == i) continue;

			pFigure2 = ppFigures[j];
			
			if ( (pFigure2->m_maxY <= LMAXY) && 
				 (pFigure2->m_maxY >= LMAXY-dmaxy) &&
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
						  (pFigure->m_maxY >= LMAXY-dmaxy) )
				{
					if (val >= 3)//наезжают друг на друга минимум на 3 пиксела
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
	k=0;
	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		if ( !( (pFigure->m_maxY <= LMAXY) && 
			    (pFigure->m_maxY >= LMAXY-dmaxy) &&
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

		if ( (w/2 - pFigure->m_minX <= (120*4)/2) &&
			 (pFigure->m_maxX - w/2 <= (120*4)/2) )
		{
			res = 1;
		}
	}
	//////////////////////////////

	delete[] pFigures;
	delete[] ppFigures;
	delete[] ppFgs;

	return res;
}


int ClearImageOpt5(int *Im, int w, int h, int LH, int LMAXY, 
					int jY_min, int jY_max, int jI_min, int jI_max, int jQ_min, int jQ_max,
					int mY, int dY, int mI, int dI, int mQ, int dQ, 
					int mmY, int ddY1, int ddY2, int mmI, int ddI, int mmQ, int ddQ, int white)
{
	CMyClosedFigure *pFigures, **ppFigures, *pFigure;
	int i, l, ii, val, valY, valI, valQ, max_val, N;
	int val1, val2, val3, val4, val5, val6, ddy1, ddy2;
	int GRStr[256*2], delta, smax[256*2], smaxi[256*2], NNN;
	CMyPoint *PA;
	clock_t t;
	int dmaxy = g_dmaxy;

	t = SearchClosedFigures(Im, w, h, white, pFigures, N);

	if (N == 0)	return 0;

	ppFigures = new CMyClosedFigure*[N];
	for(i=0; i<N; i++)
	{
		ppFigures[i] = &(pFigures[i]);
	}

	val = 2*LMAXY - LH;
	
	ddy1 = (int)((double)val*0.05);
	if (ddy1 < 2) ddy1 = 2;

	ddy2 = (int)((double)(2*h-val)*0.05);
	if (ddy2 < 2) ddy2 = 2;
	ddy2 = (h-1)-ddy2;

	i=0;
	while(i < N) 
	{
		pFigure = ppFigures[i];

		if	(	(pFigure->m_minX <= 2) ||
				(pFigure->m_maxX >= (w-1)-2) ||
				(pFigure->m_minY <= ddy1) ||
				(pFigure->m_maxY >= ddy2) || 
				(pFigure->m_w == 1) ||
				(pFigure->m_h == 1) ||
				( (pFigure->m_w >= LH*3) && 
				  ( (pFigure->m_maxY > LMAXY) || (pFigure->m_maxY < LMAXY-dmaxy) ) 
				) ||
				(
					( ((val-(pFigure->m_minY+pFigure->m_maxY)) > LH*2) || (((pFigure->m_minY+pFigure->m_maxY)-val) > LH*2) )
				) ||
				( (pFigure->m_w<=3)&& (pFigure->m_h<=3)	&& (pFigure->m_minY > LMAXY) )
			)
		{
			PA = pFigure->m_PointsArray;
			
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				Im[ii] = 0;
			}

			ppFigures[i] = ppFigures[N-1];
			N--;
			continue;
		}
		else
		{
			PA = pFigure->m_PointsArray;
			
			//bln = 0;
			val1 = 0;
			val2 = 0;
			val3 = 0;
			for(l=0; l < pFigure->m_Square; l++)
			{
				ii = PA[l].m_i;
				valI = g_ImI[ii];
				valQ = g_ImQ[ii];
				valY = g_ImY[ii];
				val1 += valI;
				val2 += valQ;
				val3 += valY;

				/*if (bln == 0)
				{
					if ( (valI>=jI_min) && (valI<=jI_max) &&
						 (valQ>=jQ_min) && (valQ<=jQ_max) &&
						 (valY>=jY_min) && (valY<=jY_max)
						)
					{
						bln = 1;
					}
				}*/
			}
			pFigure->m_mI = val1/pFigure->m_Square;
			pFigure->m_mQ = val2/pFigure->m_Square;
			pFigure->m_mY = val3/pFigure->m_Square;
			
			StrAnalyseImage(pFigure, g_ImY, GRStr, 0);			
			delta = 80;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val1, max_val, NNN);
			val1 -= (80-delta)/2;
			val3 = val1 - mmY;
			if (val3 < 0) val3 = -val3;
			delta = 10;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val2, max_val, NNN);
			val2 -= (80-delta)/2;
			val4 = val2 - mmY;
			if (val4 < 0) val4 = -val4;
			if (val3 < val4) pFigure->m_mmY = val1;
			else pFigure->m_mmY = val2;

			StrAnalyseImage(pFigure, g_ImI, GRStr, 256);
			delta = 5;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val1, max_val, NNN);
			val1 -= 256 + (5-delta)/2;
			val3 = val1 - mmI;
			if (val3 < 0) val3 = -val3;
			delta = 10;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val2, max_val, NNN);
			val2 -= 256 + (5-delta)/2;
			val4 = val2 - mmI;
			if (val4 < 0) val4 = -val4;
			if (val3 < val4) pFigure->m_mmI = val1;
			else pFigure->m_mmI = val2;
			
			StrAnalyseImage(pFigure, g_ImQ, GRStr, 256);
			delta = 5;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val1, max_val, NNN);
			val1 -= 256 + (5-delta)/2;
			val3 = val1 - mmQ;
			if (val3 < 0) val3 = -val3;
			delta = 10;
			FindMaxStrDistribution(GRStr, delta, smax, smaxi, NNN, 256);
			FindMaxStr(smax, smaxi, val2, max_val, NNN);
			val2 -= 256 + (5-delta)/2;
			val4 = val2 - mmQ;
			if (val4 < 0) val4 = -val4;
			if (val3 < val4) pFigure->m_mmQ = val1;
			else pFigure->m_mmQ = val2;

			/*if (bln == 0)
			{
				for(l=0; l < pFigure->m_Square; l++)
				{
					ii = (PA[l].m_y*w)+PA[l].m_x;
					Im[ii] = 0;
				}

				ppFigures[i] = ppFigures[N-1];
				N--;
				continue;
			}
			else*/
			{
				val1 = pFigure->m_mI - mI;
				if (val1 < 0) val1 = -val1;
				
				val2 = pFigure->m_mQ - mQ;
				if (val2 < 0) val2 = -val2;

				val3 = pFigure->m_mY - mY;
				if (val3 < 0) val3 = -val3;

				val4 = pFigure->m_mmI - mmI;
				if (val4 < 0) val4 = -val4;
				
				val5 = pFigure->m_mmQ - mmQ;
				if (val5 < 0) val5 = -val5;

				val6 = pFigure->m_mmY - mmY;

				if ( (val1 > dI) || (val2 > dQ) || (val3 > dY) || (val4 > ddI) || (val5 > ddQ) || (val6 < ddY1) || (val6 > ddY2) )
				{
					for(l=0; l < pFigure->m_Square; l++)
					{
						ii = (PA[l].m_y*w)+PA[l].m_x;
						Im[ii] = 0;
					}

					ppFigures[i] = ppFigures[N-1];
					N--;
					continue;
				}
			}
		}

		i++;
	}

	int min_h;

	min_h = (int)((double)LH*0.6);
	val1 = LMAXY - LH/2 - (int)((double)LH*0.2);
	val2 = LMAXY - LH/2 + (int)((double)LH*0.2);

	l = 0;
	for (i=0; i<N; i++)
	{
		pFigure = ppFigures[i];

		if ( (pFigure->m_h >= min_h) &&
			 (pFigure->m_minY < val1) &&
			 (pFigure->m_maxY > val2) )
		{
			l++;
		}
	}

	delete[] pFigures;
	delete[] ppFigures;

	return l;
}

void StrAnalyseImage(int *Im, int *ImGR, int *GRStr, int w, int h, int xb, int xe, int yb, int ye, int offset)
{
	int i, ib, ie, x, y, val;

	memset(GRStr, 0, (256+offset)*sizeof(int));

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

void StrAnalyseImage(CMyClosedFigure *pFigure, int *ImGR, int *GRStr, int offset)
{
	int l, val;
	CMyPoint *PA;

	memset(GRStr, 0, g_str_size*sizeof(int));

	PA = pFigure->m_PointsArray;
		
	for(l=0; l < pFigure->m_Square; l++)
	{
		val = ImGR[PA[l].m_i]+offset; 
		GRStr[val]++;
	}
}

void FindMaxStrDistribution(int *GRStr, int delta, int *smax, int *smaxi, int &N, int offset)
{
	int i, imax, ys, ys1, ys2, val, NN;

	ys = 0;
	for (i=0; i<delta; i++) ys += GRStr[i];

	ys1 = ys2 = ys;
	NN = 0;
	memset(smax, 0, g_str_size*sizeof(int));
	memset(smaxi, 0, g_str_size*sizeof(int));

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

void FindMaxStr(int *smax, int *smaxi, int &max_i, int &max_val, int N)
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

void ResizeImage4x(int *Im, int *ImRES, int w, int h)
{
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
				g_ImRES2[j]   = Im[i];
				g_ImRES2[j+1] = Im[i];
				g_ImRES2[j+2] = Im[i];
				g_ImRES2[j+3] = Im[i];
			}
			else
			{
				color = (u8*)(&Im[i+1]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	

				g_ImRES2[j] = Im[i];
				
				clr[2] = (r0*3+r1)/4;
				clr[1] = (g0*3+g1)/4;
				clr[0] = (b0*3+b1)/4;
				
				g_ImRES2[j+1] = clr_res;

				clr[2] = (r0+r1)/2;
				clr[1] = (g0+g1)/2;
				clr[0] = (b0+b1)/2;
				
				g_ImRES2[j+2] = clr_res;

				clr[2] = (r0+r1*3)/4;
				clr[1] = (g0+g1*3)/4;
				clr[0] = (b0+b1*3)/4;
				
				g_ImRES2[j+3] = clr_res;

				r0 = r1;
				g0 = g1;
				b0 = b1;
			}
		}
	}

	for(x=0; x<4*w; x++)
	{
		i = y*4*w+x;
		color = (u8*)(&g_ImRES2[i]);
		r0 = color[2];
		g0 = color[1];
		b0 = color[0];	

		for(y=0; y<h; y++)		
		{
			i = y*4*w+x;
			j = 4*y*4*w+x;

			if (y == h-1)
			{
				ImRES[j]   = g_ImRES2[i];
				ImRES[j+1*4*w] = g_ImRES2[i];
				ImRES[j+2*4*w] = g_ImRES2[i];
				ImRES[j+3*4*w] = g_ImRES2[i];
			}
			else
			{
				color = (u8*)(&g_ImRES2[i+4*w]);
				r1 = color[2];
				g1 = color[1];
				b1 = color[0];	

				ImRES[j] = g_ImRES2[i];
				
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

void SimpleResizeImage4x(int *Im, int *ImRES, int w, int h)
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

void ResizeGrayscaleImage4x(int *Im, int *ImRES, int w, int h)
{
	int i, j, x, y;
	int val0, val1;

	for(y=0, i=0, j=0; y<h; y++)
	{
		val0 = Im[i];

		for(x=0; x<w; x++, i++, j+=4)
		{
			if (x == w-1)
			{
				g_ImRES2[j]   = val0;
				g_ImRES2[j+1] = val0;
				g_ImRES2[j+2] = val0;
				g_ImRES2[j+3] = val0;
			}
			else
			{
				val1 = Im[i+1];

				g_ImRES2[j] = val0;
				
				g_ImRES2[j+1] = (val0*3+val1)/4;
				
				g_ImRES2[j+2] = (val0+val1)/2;
				
				g_ImRES2[j+3] = (val0+val1*3)/4;

				val0 = val1;
			}
		}
	}

	for(x=0; x<4*w; x++)
	{
		i = y*4*w+x;
		val0 = g_ImRES2[i];

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
				val1 = g_ImRES2[i+4*w];

				ImRES[j] = val0;
				
				ImRES[j+1*4*w] = (val0*3+val1)/4;
				
				ImRES[j+2*4*w] = (val0+val1)/2;
				
				ImRES[j+3*4*w] = (val0+val1*3)/4;

				val0 = val1;
			}
		}
	}
}

int CompareTXTImages(int *Im1, int *Im2, int w1, int h1, int w2, int h2, int YB1, int YB2)
{
	return 0;
}

void GetImageSize(string name, int &w, int &h)
{
	if (!g_wxImageHandlersInitialized)
	{
		wxImage::AddHandler(new wxJPEGHandler);	
		g_wxImageHandlersInitialized = true;
	}

	wxImage wxIm(name);

	w = wxIm.GetWidth();
	h = wxIm.GetHeight();
}

void SaveRGBImage(int *Im, string name, int w, int h)
{	
	wxImage wxIm(w, h, true);
	int i, x, y;
	u8 *color;
	string full_name;

	if (!g_wxImageHandlersInitialized)
	{
		wxImage::AddHandler(new wxJPEGHandler);	
		g_wxImageHandlersInitialized = true;
	}

    full_name = g_dir;
    full_name += name; 	

	for (y=0, i=0; y<h; y++)
	for (x=0; x<w; x++, i++)
	{
		color = (u8*)(&Im[i]);
		wxIm.SetRGB(x, y, color[2], color[1], color[0]);		
	}

	wxIm.SetOption(wxIMAGE_OPTION_QUALITY, 100);
	wxIm.SaveFile(full_name, wxBITMAP_TYPE_JPEG);
}

void LoadRGBImage(int *Im, string name, int &w, int &h)
{
	if (!g_wxImageHandlersInitialized)
	{
		wxImage::AddHandler(new wxJPEGHandler);	
		g_wxImageHandlersInitialized = true;
	}

	wxImage wxIm(name);
	int i, x, y;
	u8 *color;

	w = wxIm.GetWidth();
	h = wxIm.GetHeight();

	for (y=0, i=0; y<h; y++)
	for (x=0; x<w; x++, i++)
	{		
		Im[i] = 0;
		color = (u8*)(&Im[i]);
		color[2] = wxIm.GetRed(x, y);
		color[1] = wxIm.GetGreen(x, y);
		color[0] = wxIm.GetBlue(x, y);
	}
}

void SaveImage(int *Im, string name, int w, int h, int quality, int dpi)
{
	wxImage wxIm(w, h, true);
	int i, x, y;
	u8 *color;
	string full_name;

	if (!g_wxImageHandlersInitialized)
	{
		wxImage::AddHandler(new wxJPEGHandler);	
		g_wxImageHandlersInitialized = true;
	}

    full_name = g_dir;
    full_name += name; 	

	for (y=0, i=0; y<h; y++)
	for (x=0; x<w; x++, i++)
	{
		color = (u8*)(&Im[i]);
		wxIm.SetRGB(x, y, color[0], color[0], color[0]);		
	}

	if (dpi != -1)
	{
		wxIm.SetOption(wxIMAGE_OPTION_RESOLUTIONX, dpi);
		wxIm.SetOption(wxIMAGE_OPTION_RESOLUTIONY, dpi);
	}
	
	if (quality != -1)
	{
		wxIm.SetOption(wxIMAGE_OPTION_QUALITY, 100);
	}
	
	wxIm.SaveFile(full_name, wxBITMAP_TYPE_JPEG);
}

void LoadImage(int *Im, string name, int &w, int &h)
{
	if (!g_wxImageHandlersInitialized)
	{
		wxImage::AddHandler(new wxJPEGHandler);	
		g_wxImageHandlersInitialized = true;
	}

	wxImage wxIm(name);
	int i, x, y;

	w = wxIm.GetWidth();
	h = wxIm.GetHeight();

	for (y=0, i=0; y<h; y++)
	for (x=0; x<w; x++, i++)
	{		
		if (wxIm.GetRed(x, y) != 0)
		{
			Im[i] = 255;
		}
		else
		{
			Im[i] = 0;
		}
	}
}