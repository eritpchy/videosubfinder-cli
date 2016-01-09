                              //SSAlgorithms.cpp//                                
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

#include "SSAlgorithms.h"
#include <math.h>

int		g_RunSubSearch = 0;

int		g_DL = 6;	 //sub frame length
double	g_tp = 0.3;	 //text procent
double	g_mtpl = 0.022;  //min text len (in procent)
double	g_sse = 0.3;   //sub square error
double	g_veple = 0.35; //vedges points line error
//double	g_de;	 //density error
//double	g_lle;	 //line length error

int **g_lb; // lines begins
int **g_le; // lines ends
int	*g_ln;

bool g_fast_search = true;

CVideo *g_pV;

void SetVideoWindowSettins(CVideo *pV, double dx_min, double dx_max, double dy_min, double dy_max)
{	
	g_W = pV->m_Width;
	g_H = pV->m_Height;

	g_xmin = (int)(dx_min*(double)g_W);
	g_xmax = (int)(dx_max*(double)g_W)-1;
	g_ymin = (int)(dy_min*(double)g_H);
	g_ymax = (int)(dy_max*(double)g_H)-1;

	g_w = g_xmax-g_xmin+1;
	g_h = g_ymax-g_ymin+1;
}

s64 SearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{	
	int *ImRGB;
	int	*Im;
	int *ImSF;
	int	*ImNFF;
	int	*ImNFFS;
	int *ImSP; //store image prev
	int *ImS; //store image
	int *ImFSP; //image for save prev
	int *ImFS; //image for save
	int *ImVE;
	int *ImVES;
	int *ImVESS;
	int *ImVESP;
	int *ImVESSP;
	int *ImNE;
	int *ImNES;
	int *ImNESP;
	int *ImHE;
	int *ImRES;
	int **ImS_SQ;
	int **ImVES_SQ;
	int **ImNES_SQ;
	int *lb=g_pLB8;
	int *le=g_pLE8;

	string Str;
	
	s64 CurPos;
	int fn; //frame num
	int i, k, n, nn, ln;
	int S, SP, w, h, size, BufferSize;
	int mtl, DL, segh;
	double sse;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;
	s64 prevPos;

    int bln, finded_prev;
	
	g_RunSubSearch = 1;

	g_pV = pV;

	w = g_w;
	h = g_h;

	size = w*h;
	BufferSize = size*sizeof(int);

//	if (g_debug == 1) Begin = GetVideoTime(9, 56, 000);
	pV->SetPos(Begin);

    pV->OneStep();
    CurPos = pV->GetPos();

	mtl = (int)(g_mtpl*(double)w);
	DL = g_DL;
	segh = g_segh;
	n = h/g_segh;
	sse = g_sse;

	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	int SIZE = g_W*g_H;

	ImRGB = new int[SIZE];
	Im = new int[SIZE];
	ImSF = new int[SIZE];
	ImNFF = new int[SIZE];
	ImNFFS = new int[SIZE];
	ImS = new int[SIZE];
	ImSP = new int[SIZE];
	ImFS = new int[SIZE];
	ImFSP = new int[SIZE];
	ImVE = new int[SIZE];
	ImVES = new int[SIZE];
	ImVESS = new int[SIZE];
	ImVESP = new int[SIZE];
	ImVESSP = new int[SIZE];
	ImNE = new int[SIZE];
	ImNES = new int[SIZE];
	ImNESP = new int[SIZE];
	ImHE = new int[SIZE];
	ImRES = new int[SIZE];

	ImS_SQ = new int*[DL];
	ImVES_SQ = new int*[DL];
	ImNES_SQ = new int*[DL];

	for (i=0; i<DL; i++)
	{
		ImS_SQ[i] = new int[SIZE];
		ImVES_SQ[i] = new int[SIZE];
		ImNES_SQ[i] = new int[SIZE];
	}

	g_lb = new int*[n];
	g_le = new int*[n];
	g_ln = new int[n];

	for(k=0; k<n; k++)
	{
		g_lb[k] = new int[w]; 
		g_le[k] = new int[w]; 
	}

	prevPos = -2;

	while ((CurPos < End) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{	
		Str = VideoTimeToStr(CurPos);
		/*if (CurPos >= (s64)((1*60+25)*1000+250)*(s64)10000)
		{
			pTime->SetWindowText(Str);	
		}*/

		//*******
		S = GetAndConvertImage(ImRGB, ImNFF, ImSF, Im, ImVE, ImNE, ImHE, pV, w, h);

		if ( (S > 0) && (CurPos != prevPos) )
		{	
			if (bf == -2)
			{
L:				bf = fn;
				bt = CurPos;

				SP = S;
				memcpy(ImS, Im, BufferSize);
				memcpy(ImNFFS, ImNFF, BufferSize);
				memcpy(ImVES, ImVE, BufferSize);
				memcpy(ImNES, ImNE, BufferSize);
				memcpy(ImFS, ImRGB, BufferSize);				
				memcpy(ImVESS, ImVE, BufferSize);

				nn = 0;
			}
			else
			{
				if (fn-bf < DL)
				{
					/*g_pMF->SaveImage(ImS, "\\TestImages\\Cmb1!.jpeg", w, h);
					g_pMF->SaveImage(Im, "\\TestImages\\Cmb2!.jpeg", w, h);
					g_pMF->SaveImage(ImNFFS, "\\TestImages\\Cmb3!.jpeg", w, h);
					g_pMF->SaveImage(ImNFF, "\\TestImages\\Cmb4!.jpeg", w, h);*/

					//GetGrayscaleImage(ImFS, ImRES, w, h);
					//SobelVEdge(ImRES, ImVES, w, h);

					//if(CompareTwoImages(ImS, ImNFFS, Im, ImNFF, size) == 0) 
					if(CompareTwoImages(ImS, ImVES, Im, ImVE, size) == 0) 
					{
						if (finded_prev == 1) 
						{
							for(i=0; i<nn; i++)
							{
								SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
								SimpleCombineTwoImages(ImVESS, ImVES_SQ[i], size);
								SimpleCombineTwoImages(ImNES, ImNES_SQ[i], size);
							}

							goto L2;
						}

						goto L;
					}

					if ((finded_prev == 0) && (fn-bf == 1))
					{
						memcpy(ImS, Im, BufferSize);
						memcpy(ImNFFS, ImNFF, BufferSize);
						memcpy(ImVES, ImVE, BufferSize);
						memcpy(ImNES, ImNE, BufferSize);
					}
					else
					{
						memcpy(ImS_SQ[nn], Im, BufferSize);
						memcpy(ImVES_SQ[nn], ImVE, BufferSize);
						memcpy(ImNES_SQ[nn], ImNE, BufferSize);
						nn++;
					}

					if (fn-bf == 3)
					{
						memcpy(ImFS, ImRGB, BufferSize);
						memcpy(ImVESS, ImVE, BufferSize);
						memcpy(ImNES, ImNE, BufferSize);
					}
				}
				else
				{
					if (fn-bf == DL)
					{
						/*val1 = 0;
						val2 = h-1;
						LineAndColorFiltration(ImS, ImFS, &val1, &val2, 1, w, h);*/

						for(i=0; i<nn; i++)
						{
							SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
						}

						bln = AnalyseImage(ImS, w, h);

						if (bln == 1) 
						{
							for(i=0; i<nn; i++)
							{
								SimpleCombineTwoImages(ImVESS, ImVES_SQ[i], size);
								SimpleCombineTwoImages(ImNES, ImNES_SQ[i], size);
							}	
						}
						else
						{
							bf = -2;
							goto L;
						}
					}

					if (fn-bf >= DL)
					{												
						if (CompareTwoSubs(ImS, ImVES, Im, ImVE, w, h) == 0)
						{
L2:							if (finded_prev == 1)
							{
								ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
								
								if (
										(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVES, w, h) == 1) ||
										(FinalCompareTwoSubs1(ImRES, lb, le, ln, ImNESP, ImNES, w, h) == 1) ||
										(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESSP, ImVESS, w, h) == 1) ||
										(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVESS, w, h) == 1) ||
										(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESSP, ImVES, w, h) == 1)
									)
								{
									pef = fn-1;
									pet = CurPos-10000;
								}
								else
								{
									Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
									ImToNativeSize(ImFSP, w, h);
									ImToNativeSize(ImSP, w, h);
									ImToNativeSize(ImVESSP, w, h);
									g_pViewImage[0](ImFSP, g_W, g_H);									
									g_pViewImage[1](ImSP, g_W, g_H);									
									SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
									SaveImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
									SaveImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);
									
									pbf = bf;
									pbt = bt;
									pef = fn-1;
									pet = CurPos-10000;
								}
							}	
							else
							{
								pbf = bf;
								pbt = bt;
								pef = fn-1;
								pet = CurPos-10000;
							}

							if (pef-pbf+1 >= DL)
							{
								memcpy(ImSP, ImS, BufferSize);
								memcpy(ImFSP, ImFS, BufferSize);
								memcpy(ImVESP, ImVES, BufferSize);
								memcpy(ImVESSP, ImVESS, BufferSize);
								memcpy(ImNESP, ImNES, BufferSize);
											
								finded_prev = 1;
							}
							else
							{
								finded_prev = 0;
							}

							goto L;	
						}	
					}
				}
			}
		}
		else if ( ( (S == 0) && (CurPos != prevPos) ) ||
				  ( (S > 0) &&  (CurPos == prevPos) ) )
		{
			if (finded_prev == 1)
			{
				if (fn-bf <= DL)
				{
					for(i=0; i<nn; i++)
					{
						SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
					}

					bln = AnalyseImage(ImS, w, h);

					if (bln == 1) 
					{
						for(i=0; i<nn; i++)
						{
							SimpleCombineTwoImages(ImVESS, ImVES_SQ[i], size);
							SimpleCombineTwoImages(ImNES, ImNES_SQ[i], size);
						}	
					}
					else
					{
						Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
						ImToNativeSize(ImFSP, w, h);
						ImToNativeSize(ImSP, w, h);
						ImToNativeSize(ImVESSP, w, h);
						g_pViewImage[0](ImFSP, g_W, g_H);									
						g_pViewImage[1](ImSP, g_W, g_H);	
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
						SaveImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
						SaveImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
						finded_prev = 0;
						bf = -2;
					}
				}

				if (finded_prev == 1)
				{
					ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
								
					if (
							(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVES, w, h) == 1) ||
							(FinalCompareTwoSubs1(ImRES, lb, le, ln, ImNESP, ImNES, w, h) == 1) ||
							(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESSP, ImVESS, w, h) == 1) ||
							(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVESS, w, h) == 1) ||
							(FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESSP, ImVES, w, h) == 1)
						)
					{
						memcpy(ImS, ImRES, BufferSize);
						memcpy(ImFS, ImFSP, BufferSize);
						memcpy(ImVESS, ImVESSP, BufferSize);
						bf = pbf;
						bt = pbt;
					}
					else
					{
						Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
                        ImToNativeSize(ImFSP, w, h);
						ImToNativeSize(ImSP, w, h);
						ImToNativeSize(ImVESSP, w, h);
						g_pViewImage[0](ImFSP, g_W, g_H);									
						g_pViewImage[1](ImSP, g_W, g_H);									
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
						SaveImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
						SaveImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
					}
				}
			}

			if (bf != -2)
			{
				if (fn-bf > DL)
				{			
					et = CurPos-10000;
					Str = VideoTimeToStr(bt)+string("__")+VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h);
					ImToNativeSize(ImS, w, h);
					ImToNativeSize(ImVESS, w, h);
					g_pViewImage[0](ImFS, g_W, g_H);									
					g_pViewImage[1](ImS, g_W, g_H);									
					SaveRGBImage(ImFS, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
					SaveImage(ImVESS, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
					SaveImage(ImS, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
				}
			}

			finded_prev = 0;
			bf = -2;
		}

		prevPos = CurPos;
		
        CurPos = pV->OneStepWithTimeout();

		fn++;
	}

	delete[] ImRGB;
	delete[] Im;
	delete[] ImSF;
	delete[] ImNFF;
	delete[] ImNFFS;
	delete[] ImS;
	delete[] ImSP;
	delete[] ImFS;
	delete[] ImFSP;
	delete[] ImVE;
	delete[] ImVES;
	delete[] ImVESS;
	delete[] ImVESP;
	delete[] ImVESSP;
	delete[] ImNE;
	delete[] ImNES;
	delete[] ImNESP;
	delete[] ImHE;
	delete[] ImRES;

	for (i=0; i<DL; i++)
	{
		delete[] ImS_SQ[i];
		delete[] ImVES_SQ[i];
		delete[] ImNES_SQ[i];
	}
	
	delete[] ImS_SQ;
	delete[] ImVES_SQ;
	delete[] ImNES_SQ;

	for(int k=0; k<n; k++)
	{
		delete[] g_lb[k]; 
		delete[] g_le[k]; 
	}
	delete[] g_lb;
	delete[] g_le;
	delete[] g_ln;

	if (g_RunSubSearch == 0)
	{
		if (bf != -2)
		{
			if (finded_prev == 1)
			{
				return pbt;
			}
			return bt;
		}
		return CurPos;
	}

	return 0;
}

s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{	
	int *ImRGB;
	int *pImRGB;
	int	*Im;
	int	*ImT;
	int	*pIm;
	int *ImSF;
	int *ImSP; //store image prev
	int *ImSSP;
	int *ImS; //store image
	int *ImSS;
	int *ImFSP; //image for save prev
	int *ImFS; //image for save
	int *ImVE;
	int	*ImVET;
	int	*pImVE;
	int *ImVES;
	int *ImVESP;
	int *ImRES;
	int **ImS_SQ;
	int **mImRGB;
	int *lb=g_pLB9;
	int *le=g_pLE9;

	string Str;
	
	s64 CurPos, pos;
	int fn; //frame num
	int i, k, n, nn, ln;
	int w, h, size, BufferSize;
	int mtl, DL, segh;
	double sse;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;
	s64 prevPos;

	int found_sub, n_fs;
	s64 *mPrevPos;
	s64 prev_pos;

	int bln, finded_prev;
	
	g_RunSubSearch = 1;

	g_pV = pV;

	w = g_w;
	h = g_h;

	size = w*h;
	BufferSize = size*sizeof(int);

	//if (g_debug == 1) Begin = GetVideoTime(9, 56, 000);
	pV->SetPos(Begin);

    pV->OneStep();
    CurPos = pV->GetPos();

	mtl = (int)(g_mtpl*(double)w);
	DL = g_DL;
	segh = g_segh;
	n = h/g_segh;
	sse = g_sse;

	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	int SIZE = g_W*g_H;

	ImRGB = new int[SIZE];
	Im = new int[SIZE];
	ImT = new int[SIZE];
	ImSF = new int[SIZE];
	ImS = new int[SIZE];
	ImSS = new int[SIZE];
	ImSP = new int[SIZE];
	ImSSP = new int[SIZE];
	ImFS = new int[SIZE];
	ImFSP = new int[SIZE];
	ImVE = new int[SIZE];
	ImVET = new int[SIZE];
	ImVES = new int[SIZE];
	ImVESP = new int[SIZE];
	ImRES = new int[SIZE];

	ImS_SQ = new int*[DL];
	mImRGB = new int*[DL];
	mPrevPos = new s64[DL];

	for (i=0; i<DL; i++)
	{
		ImS_SQ[i] = new int[SIZE];
		mImRGB[i] = new int[SIZE];
	}

	g_lb = new int*[n];
	g_le = new int*[n];
	g_ln = new int[n];

	for(k=0; k<n; k++)
	{
		g_lb[k] = new int[w]; 
		g_le[k] = new int[w]; 
	}

	found_sub = 0;
	prev_pos = -2;
	mPrevPos[0] = CurPos;
	pV->GetRGBImage(mImRGB[0], g_xmin, g_xmax, g_ymin, g_ymax);
	n_fs = 1;

	prevPos = -2;

	while ((CurPos < End) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{		
		while (found_sub == 0)
		{
			pos = CurPos;
            n_fs = 0;

			while( (n_fs < DL) && (pos < End) )
			{			
				mPrevPos[n_fs] = pos = pV->OneStepWithTimeout();

                pV->GetRGBImage(mImRGB[n_fs], g_xmin, g_xmax, g_ymin, g_ymax);

				fn++;
				n_fs++;
			}
			if (pos >= End)
			{
				while(n_fs < DL)
				{
					mPrevPos[n_fs] = pos;
					memcpy(mImRGB[n_fs], mImRGB[n_fs-1], BufferSize);

					fn++;
					n_fs++;
				}
			}

			bln = ConvertImage(mImRGB[DL-1], ImT, ImVET, w, h);

			if (bln == 1)
			{
				fn -= DL; 
				n_fs = 0;
				found_sub = 1;
			}
			else
			{
				prev_pos = CurPos = mPrevPos[DL-1];
				n_fs = 0;
			}

			if ( (mPrevPos[DL-1] >= End) || (g_RunSubSearch == 0) || (mPrevPos[DL-1] == mPrevPos[DL-2]) )
			{
				break;
			}
		}

		if ( (mPrevPos[DL-1] > End) || (g_RunSubSearch == 0) || (mPrevPos[DL-1] == mPrevPos[DL-2]) ||
			 ( (found_sub == false) && (mPrevPos[DL-1] == End) ) )
		{
			break;
		}

		if (n_fs < DL)
		{
			pImRGB = mImRGB[n_fs];
			CurPos = mPrevPos[n_fs];

			if (n_fs == 0) prevPos = prev_pos;
			else prevPos = mPrevPos[n_fs-1];
			
			fn++;
			n_fs++;
		}
		else
		{
			prevPos = CurPos;
	
			CurPos = pV->OneStepWithTimeout();

            pV->GetRGBImage(ImRGB, g_xmin, g_xmax, g_ymin, g_ymax);

			pImRGB = ImRGB;

			fn++;
		}

		if (n_fs != DL)
		{
			bln = ConvertImage(pImRGB, Im, ImVE, w, h);
			pIm = Im;
			pImVE = ImVE;
		}
		else
		{
			bln = 1;
			pIm = ImT;
			pImVE = ImVET;
			n_fs++;
		}

		if ( (bln == 1) && (CurPos != prevPos) )
		{	
			if (bf == -2)
			{
L:				bf = fn;
				bt = CurPos;

				memcpy(ImS, pIm, BufferSize);
				memcpy(ImVES, pImVE, BufferSize);
				memcpy(ImFS, pImRGB, BufferSize);				

				nn = 0;
			}
			else
			{
				if (fn-bf < DL)
				{
					/*if (g_debug == 1)
					{
						g_pMF->SaveImage(ImS, "\\TestImages\\Cmb1!.jpeg", w, h);
						g_pMF->SaveImage(Im, "\\TestImages\\Cmb2!.jpeg", w, h);
						g_pMF->SaveImage(ImVES, "\\TestImages\\Cmb3!.jpeg", w, h);
						g_pMF->SaveImage(ImVE, "\\TestImages\\Cmb4!.jpeg", w, h);
					}*/

					if(CompareTwoImages(ImS, ImVES, pIm, pImVE, size) == 0) 
					{
						if (finded_prev == 1) 
						{
							memcpy(ImSS, ImS, BufferSize);

							for(i=0; i<nn; i++)
							{
								SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
							}							

							goto L2;
						}

						goto L;
					}

					if ((finded_prev == 0) && (fn-bf == 1))
					{
						memcpy(ImS, pIm, BufferSize);
						memcpy(ImVES, pImVE, BufferSize);
					}
					else
					{
						memcpy(ImS_SQ[nn], pIm, BufferSize);
						nn++;
					}

					if (fn-bf == 3)
					{
						memcpy(ImFS, pImRGB, BufferSize);
						memcpy(ImVES, pImVE, BufferSize);
					}
				}
				else
				{
					if (fn-bf == DL)
					{						
						for(i=0; i<nn; i++)
						{
							SimpleCombineTwoImages(ImS, ImS_SQ[i], size);							
						}

						bln = AnalyseImage(ImS, w, h);

						if (bln == 0) 						
						{
							bf = -2;
							goto L;
						}
						else
						{
							if (finded_prev == 0) memcpy(ImSS, ImS_SQ[nn-1], BufferSize);
							else memcpy(ImSS, ImS, BufferSize);
						}
					}

					if (fn-bf >= DL)
					{												
						if (CompareTwoSubs(ImS, ImVES, pIm, pImVE, w, h) == 0)
						{
L2:							if (finded_prev == 1)
							{
								bln = CompareTwoSubs(ImSP, ImVESP, ImS, ImVES, w, h);
								if (bln == 0)
								{
									ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
									bln = FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVES, w, h);
								}
								if (bln == 0) bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h);
								
								if (bln == 1)
								{
									pef = fn-1;
									pet = CurPos-10000;

									SimpleCombineTwoImages(ImSSP, ImSS, size);
								}
								else
								{
									Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
									ImToNativeSize(ImFSP, w, h);
									ImToNativeSize(ImSSP, w, h);
									g_pViewImage[0](ImFSP, g_W, g_H);									
									g_pViewImage[1](ImSSP, g_W, g_H);									
									SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
									SaveImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);								

									pbf = bf;
									pbt = bt;
									pef = fn-1;
									pet = CurPos-10000;

									memcpy(ImSSP, ImSS, BufferSize);
								}
							}	
							else
							{								
								pbf = bf;
								pbt = bt;
								pef = fn-1;
								pet = CurPos-10000;

								memcpy(ImSSP, ImSS, BufferSize);
							}

							if (pef-pbf+1 >= DL)
							{
								memcpy(ImSP, ImS, BufferSize);
								memcpy(ImFSP, ImFS, BufferSize);
								memcpy(ImVESP, ImVES, BufferSize);											
								finded_prev = 1;
							}
							else
							{
								finded_prev = 0;
							}

							goto L;	
						}
						else
						{
							SimpleCombineTwoImages(ImSS, pIm, size);
						}
					}
				}
			}
		}
		else if ( ( (bln == 0) && (CurPos != prevPos) ) ||
				  ( (bln == 1) && (CurPos == prevPos) ) )
		{
			if (finded_prev == 1)
			{
				if (fn-bf <= DL)
				{
					memcpy(ImSS, ImS, BufferSize);

					for(i=0; i<nn; i++)
					{
						SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
					}

					bln = AnalyseImage(ImS, w, h);

					if (bln == 0) 
					{
						Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
                        ImToNativeSize(ImFSP, w, h);
						ImToNativeSize(ImSSP, w, h);
						g_pViewImage[0](ImFSP, g_W, g_H);									
						g_pViewImage[1](ImSSP, g_W, g_H);									
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
						SaveImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
						finded_prev = 0;
						bf = -2;
					}
				}

				if (finded_prev == 1)
				{
					bln = CompareTwoSubs(ImSP, ImVESP, ImS, ImVES, w, h);
					if (bln == 0)
					{
						ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
						bln = FinalCompareTwoSubs2(ImRES, lb, le, ln, ImVESP, ImVES, w, h);
					}
					if (bln == 0) bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h);
					
					if (bln == 1)
					{		
						SimpleCombineTwoImages(ImSS, ImSSP, size);
						memcpy(ImS, ImRES, BufferSize);
						memcpy(ImFS, ImFSP, BufferSize);
						bf = pbf;
						bt = pbt;
					}
					else
					{
						Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
						ImToNativeSize(ImFSP, w, h);
						ImToNativeSize(ImSSP, w, h);
						g_pViewImage[0](ImFSP, g_W, g_H);									
						g_pViewImage[1](ImSSP, g_W, g_H);									
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
						SaveImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
					}
				}
			}

			if (bf != -2)
			{
				if (fn-bf > DL)
				{			
					et = CurPos-10000;
					Str = VideoTimeToStr(bt)+string("__")+VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h);
					ImToNativeSize(ImSS, w, h);
					g_pViewImage[0](ImFS, g_W, g_H);									
					g_pViewImage[1](ImSS, g_W, g_H);									
					SaveRGBImage(ImFS, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
					SaveImage(ImSS, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
				}
			}

			finded_prev = 0;
			bf = -2;

			if (n_fs >= DL)
			{
				found_sub = 0;
				prev_pos = CurPos;
				n_fs = 0;			
			}
		}
	}

	delete[] ImRGB;
	delete[] Im;
	delete[] ImT;
	delete[] ImSF;
	delete[] ImS;
	delete[] ImSS;
	delete[] ImSP;
	delete[] ImSSP;
	delete[] ImFS;
	delete[] ImFSP;
	delete[] ImVE;
	delete[] ImVET;
	delete[] ImVES;
	delete[] ImVESP;
	delete[] ImRES;

	for (i=0; i<DL; i++)
	{
		delete[] ImS_SQ[i];
		delete[] mImRGB[i];
	}
	
	delete[] ImS_SQ;
	delete[] mImRGB;
	delete[] mPrevPos;

	for(int k=0; k<n; k++)
	{
		delete[] g_lb[k]; 
		delete[] g_le[k]; 
	}
	delete[] g_lb;
	delete[] g_le;
	delete[] g_ln;

	if (g_RunSubSearch == 0)
	{
		if (bf != -2)
		{
			if (finded_prev == 1)
			{
				return pbt;
			}
			return bt;
		}
		return CurPos;
	}

	return 0;
}

int CompareTwoImages(int *Im1, int *ImNFF1, int *Im2, int *ImNFF2, int size)
{
	int i, dif1, dif2, cmp, val1, val2;

	dif1 = 0;
	dif2 = 0;
	cmp = 0;

	for(i=0; i<size; i++)
	{
		if ((Im1[i] == 255) || (Im2[i] == 255))
		{
			val1 = ImNFF1[i];
			val2 = ImNFF2[i];

			if (val1 != val2)
			{
				if (val1 == 255) dif1++;
				else dif2++;
			}
			else
			{
				if (val1 == 255) cmp++;
			}
		}
	}

	if (dif2 > dif1) dif1 = dif2;

	if ((double)dif1/(double)cmp > g_sse) return 0;
	
	return 1;
}

int AnalyseImage(int *Im, int w, int h)
{
	int i, k, l, x, y, ia, da, pl, mpl, i_mpl, len, len2, val1, val2, n, bln;
	int segh, mtl;
	double tp;
	
	segh = g_segh;
	tp = g_tp;
	mtl = (int)(g_mtpl*(double)w);
	
	n = h/segh;
	da = w*segh;

	mpl = 0;
	i_mpl = 0;

	// находим все строки, а также строку с максимальной плотностью
	for(k=0, ia=0; k<n; k++, ia+=da)
	{
		l = 0;
		bln = 0;
		
		pl = 0;
		// находим все под строки
		for(x=0; x<w; x++)
		{
			for(y=0, i=ia+x; y<segh; y++, i+=w)
			{
				if(Im[i] == 255) 
				{
					pl++;
					if(bln == 0)
					{
						g_lb[k][l] = g_le[k][l] = x;
						bln = 1;
					}
					else
					{
						g_le[k][l] = x;
					}
				}
			}

			if(bln == 1)
			if(g_le[k][l] != x)
			{
				bln = 0;
				l++;
			}
		}
		
		if(bln == 1)
		if(g_le[k][l] == w-1) 
		{
			l++;
		}
		g_ln[k] = l;

		if (pl>mpl) 
		{
			mpl = pl;
			i_mpl = k;
		}
	}

	// находим cоотнощение длины текста к занимаемому им месту
	k = i_mpl;
	len = 0;
	for (l=0; l<g_ln[k]; l++)
	{
		len += g_le[k][l]-g_lb[k][l]+1;
	}
	l--;
	
	while(l)
	{
		if (g_lb[k][0]*2 >= w) return 0;
		if (g_le[k][l]*2 <= w) return 0;
		if (len < mtl) return 0;

		len2 = g_le[k][l]-g_lb[k][0]+1;

		if ((double)len/(double)len2 >= tp) return 1;

		val1 = (g_le[k][l-1]+g_lb[k][0]+1)-w;
		if (val1<0) val1 = -val1;

		val2 = (g_le[k][l]+g_lb[k][1]+1)-w;
		if (val2<0) val2 = -val2;

		if (val1 <= val2)
		{
			len -= g_le[k][l]-g_lb[k][l]+1;
		}
		else
		{
			len -= g_le[k][0]-g_lb[k][0]+1;

			for(i=0; i<l; i++)
			{
				g_lb[k][i] = g_lb[k][i+1];
				g_le[k][i] = g_le[k][i+1];
			}
		}

		l--;
	};
	
	if (len > mtl)
	if (g_lb[k][0]*2 < w)
	if (g_le[k][0]*2 > w)
	{ 
		return 1;
	}

	return 0;
}

int PreCompareTwoSubs(int *Im1, int *Im2, int *ImRES, int *lb, int *le, int w, int h) // return ln
{
	int i, ib, ie, y, l, ln, bln, val1, val2, segh, dn;
	
	segh = g_segh;

	AddTwoImages(Im1, Im2, ImRES, w*h);
		
	bln = 0;
	l = 0;
	for(y=0, ib=0, ie=w; y<h; y++, ib+=w, ie+=w)
	{
		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255) 
			{
				if (bln == 0)
				{
					lb[l] = le[l] = y;
					bln = 1;
				}
				else
				{
					le[l] = y;
				}
				break;
			}
		}
		
		if (bln == 1)
		if (i == ie)
		{
			bln = 0;
			l++;
		}
	}
	if (bln == 1)
	{
		l++;
	}
	ln = l;

	l=0; 
	while ((l<ln) && (ln > 1))
	{
		if ((le[l]-lb[l]+1) <= segh) 
		{
			if (l == 0)
			{
				dn = 1;
			}
			else 
			{
				if (l == ln-1)
				{
					dn = -1;
				}			
				else
				{
					val1 = lb[l]-le[l-1];
					val2 = lb[l+1]-le[l];
					
					if (val2 <= val1)
					{
						dn = 1;
					}
					else
					{
						dn = -1;
					}
				}
			}
			
			if (dn == 1)
			{
				lb[l+1] = lb[l];

				for(i=l; i<ln-1; i++)
				{
					lb[i] = lb[i+1];
					le[i] = le[i+1];
				}
			}
			else
			{
				le[l-1] = le[l];

				for(i=l; i<ln-1; i++)
				{
					lb[i] = lb[i+1];
					le[i] = le[i+1];
				}
			}
			ln--;
			continue;
		}
		l++;
	}

	l=0; 
	while (l < ln-1)
	{
		if ( ((lb[l+1]-le[l]-1) <= 8) && 
			 ( ((lb[l]-le[l]+1) <= 2*segh) || ((lb[l+1]-le[l+1]+1) <= 2*segh) ) )
		{
			lb[l+1] = lb[l];

			for(i=l; i<ln-1; i++)
			{
				lb[i] = lb[i+1];
				le[i] = le[i+1];
			}
			ln--;
			continue;
		}
		l++;
	}

	return ln;
}

int FinalCompareTwoSubs1(int *ImRES, int *lb, int *le, int ln, int *ImVE1, int *ImVE2, int w, int h)
{
	int i, ib, ie, k, val1, val2, dif1, dif2, cmb;	
	int bln;
	double veple;

	//g_pMF->SaveImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
	//g_pMF->SaveImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
	//g_pMF->SaveImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);

	veple = g_veple;

	bln = 1;
	for(k=0; k<ln; k++)
	{
		ib = lb[k]*w;
		ie = (le[k]+1)*w;
		
		dif1 = 0;
		dif2 = 0;
		cmb = 0;

		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255)
			{
				val1 = ImVE1[i];
				val2 = ImVE2[i];

				if (val1 != val2) 
				{
					if (val1 == 255) dif1++;
					else dif2++;
				}
				else 
				{
					if (val1 == 255) cmb++;
				}
			}
		}

		if (dif2 > dif1) dif1 = dif2;

		if ((double)dif1/(double)cmb > veple) 
		{	
			bln = 0;
			break;
		}		
	}
	return bln;
}

int FinalCompareTwoSubs2(int *ImRES, int *lb, int *le, int ln, int *ImVE1, int *ImVE2, int w, int h)
{
	int i, ib, ie, k, val1, val2, dif, dif1, dif2, cmb;
	int bln;
	double veple;

	/*if (g_debug == 1) 
	{
		g_pMF->SaveImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
		g_pMF->SaveImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
		g_pMF->SaveImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);
	}*/

	veple = g_veple;

	bln = 1;
	for(k=0; k<ln; k++)
	{
		ib = (lb[k]+1)*w;
		ie = le[k]*w;
		
		dif1 = 0;
		dif2 = 0;
		cmb = 0;

		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255)
			{
				val1 = ImVE1[i];
				val2 = ImVE2[i];

				if (val1 != val2) 
				{
					if (val1 == 255) dif1++;
					else dif2++;
				}
				else 
				{
					if (val1 == 255) cmb++;
				}
			}
		}

		if (dif2 > dif1) dif = dif2;
		else dif = dif1;

		if ( ((double)dif/(double)cmb <= veple) || 
			 ( (ln > 0) && (k < ln-1) && (lb[k]+g_ymin > g_H/4) && 
			   (le[k]+g_ymin < g_H/2) && (lb[ln-1]+g_ymin > g_H/2) ) )
		{
			continue;
		}

		dif1 = 0;
		dif2 = 0;
		cmb = 0;

		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255)
			{
				val1 = ImVE1[i] | ImVE1[i-w] | ImVE1[i+w];
				val2 = ImVE2[i] | ImVE2[i-w] | ImVE2[i+w];

				if (val1 != val2) 
				{
					if (val1 == 255) dif1++;
					else dif2++;
				}
				else 
				{
					if (val1 == 255) cmb++;
				}
			}
		}

		if (dif2 > dif1) dif = dif2;
		else dif = dif1;

		if ( ((double)dif/(double)cmb > veple) && 
			!( (ln > 0) && (k < ln-1) && (lb[k]+g_ymin > g_H/4) && 
			   (le[k]+g_ymin < g_H/2) && (lb[ln-1]+g_ymin > g_H/2) ) )
		{
			bln = 0;
			break;
		}
	}
	return bln;
}

int DifficultCompareTwoSubs(int *ImRGB1, int *ImF1, int *ImRGB2, int *ImF2, int w, int h)
{
	int *ImFF1=g_pImFF1, *ImVE1=g_pImVE1, *ImNE1=g_pImNE1;
	int *ImFF2=g_pImFF2, *ImVE2=g_pImVE2, *ImNE2=g_pImNE2;
	int *ImTEMP1=g_pImTEMP1, *ImTEMP2=g_pImTEMP2, *ImTEMP3=g_pImTEMP3;
	int *lb=g_pLB6, *le=g_pLE6;
	int res, size, ln, i;

	res = 0;

	size = w*h;

	GetTransformedImage(ImRGB1, ImTEMP1, ImTEMP2, ImFF1, ImVE1, ImNE1, ImTEMP3, w, h);
	GetTransformedImage(ImRGB2, ImTEMP1, ImTEMP2, ImFF2, ImVE2, ImNE2, ImTEMP3, w, h);

	for(i=0; i<size; i++) 
	{
		if (ImFF1[i] == 0)
		{
			ImF1[i] = 0;
		}

		if (ImFF2[i] == 0)
		{
			ImF2[i] = 0;
		}
	}
	
	ln = PreCompareTwoSubs(ImF1, ImF2, ImTEMP1, lb, le, w, h);
	
	res = FinalCompareTwoSubs2(ImTEMP1, lb, le, ln, ImVE1, ImVE2, w, h);

	if (res == 0) res = FinalCompareTwoSubs1(ImTEMP1, lb, le, ln, ImNE1, ImNE2, w, h);

	return res;
}

int CompareTwoSubs(int *Im1, int *ImVE1, int *Im2, int *ImVE2, int w, int h)
{
	int *ImRES=g_ImRES10, *lb=g_pLB7, *le=g_pLE7;
	int i, ib, ie, k, y, l, ln, bln, val1, val2, dif, dif1, dif2, cmb, segh, dn;
	double veple;

	veple = g_veple;
	segh = g_segh;

	AddTwoImages(Im1, Im2, ImRES, w*h);
	
	//g_pMF->SaveImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
	//g_pMF->SaveImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
	//g_pMF->SaveImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);
	
	bln = 0;
	l = 0;
	for(y=0, ib=0, ie=w; y<h; y++, ib+=w, ie+=w)
	{
		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255) 
			{
				if (bln == 0)
				{
					lb[l] = le[l] = y;
					bln = 1;
				}
				else
				{
					le[l] = y;
				}
				break;
			}
		}
		
		if (bln == 1)
		if (i == ie)
		{
			bln = 0;
			l++;
		}
	}
	if (bln == 1)
	{
		l++;
	}
	ln = l;

	l=0; 
	while ((l<ln) && (ln > 1))
	{
		if ((le[l]-lb[l]+1) <= segh) 
		{
			if (l == 0)
			{
				dn = 1;
			}
			else 
			{
				if (l == ln-1)
				{
					dn = -1;
				}			
				else
				{
					val1 = lb[l]-le[l-1];
					val2 = lb[l+1]-le[l];
					
					if (val2 <= val1)
					{
						dn = 1;
					}
					else
					{
						dn = -1;
					}
				}
			}
			
			if (dn == 1)
			{
				lb[l+1] = lb[l];

				for(i=l; i<ln-1; i++)
				{
					lb[i] = lb[i+1];
					le[i] = le[i+1];
				}
			}
			else
			{
				le[l-1] = le[l];

				for(i=l; i<ln-1; i++)
				{
					lb[i] = lb[i+1];
					le[i] = le[i+1];
				}
			}
			ln--;
			continue;
		}
		l++;
	}

	l=0; 
	while (l < ln-1)
	{
		if ( ((lb[l+1]-le[l]-1) <= 8) && 
			 ( ((lb[l]-le[l]+1) <= 2*segh) || ((lb[l+1]-le[l+1]+1) <= 2*segh) ) )
		{
			lb[l+1] = lb[l];

			for(i=l; i<ln-1; i++)
			{
				lb[i] = lb[i+1];
				le[i] = le[i+1];
			}
			ln--;
			continue;
		}
		l++;
	}

	bln = 1;
	for(k=0; k<ln; k++)
	{
		ib = (lb[k]+1)*w;
		ie = le[k]*w;
		
		dif1 = 0;
		dif2 = 0;
		cmb = 0;

		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255)
			{
				val1 = ImVE1[i];
				val2 = ImVE2[i];

				if (val1 != val2) 
				{
					if (val1 == 255) dif1++;
					else dif2++;
				}
				else 
				{
					if (val1 == 255) cmb++;
				}
			}
		}

		if (dif2 > dif1) dif = dif2;
		else dif = dif1;

		if ( ((double)dif/(double)cmb <= veple) || 
			 ( (ln > 0) && (l < ln-1) && (lb[l]+g_ymin > g_H/4) && 
			   (le[l]+g_ymin < g_H/2) && (lb[ln-1]+g_ymin > g_H/2) ) )
		{
			continue;
		}

		dif1 = 0;
		dif2 = 0;
		cmb = 0;

		for(i=ib; i<ie; i++)
		{
			if (ImRES[i] == 255)
			{
				val1 = ImVE1[i] | ImVE1[i-w] | ImVE1[i+w];
				val2 = ImVE2[i] | ImVE2[i-w] | ImVE2[i+w];

				if (val1 != val2) 
				{
					if (val1 == 255) dif1++;
					else dif2++;
				}
				else 
				{
					if (val1 == 255) cmb++;
				}
			}
		}

		if (dif2 > dif1) dif = dif2;
		else dif = dif1;

		if ( ((double)dif/(double)cmb > veple) && 
			!( (ln > 0) && (l < ln-1) && (lb[l]+g_ymin > g_H/4) && 
			   (le[l]+g_ymin < g_H/2) && (lb[ln-1]+g_ymin > g_H/2) ) )
		{
			bln = 0;
			break;
		}
	}
	return bln;
}

int SimpleCombineTwoImages(int *Im1, int *Im2, int size)
{	
	int i, S;

	S = 0;
	for(i=0; i<size; i++) 
	{
		if ((Im1[i]==255) && (Im2[i]==255))
		{
			S++;
		}
		else Im1[i] = 0;
	}

	return S;
}

int GetCombinedSquare(int *Im1, int *Im2, int size)
{
	int i, S;

	S = 0;
	for(i=0; i<size; i++) 
	{
		if (Im1[i]==255) 
		if (Im2[i]==255)
		{
			S++;
		}
	}

	return S;
}

void AddTwoImages(int *Im1, int *Im2, int *ImRES, int size)
{	
	int i;

	memcpy(ImRES, Im1, size*sizeof(int));

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) ImRES[i] = 255;
	}
}

void AddTwoImages(int *Im1, int *Im2, int size)
{
	int i;

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) Im1[i] = 255;
	}
}

int ConvertImage(int *ImRGB, int *ImF, int *ImVE, int w, int h)
{
	int res;

	if (g_MMX_SSE == true)
	{
		res = GetVeryFastTransformedImage(ImRGB, ImF, ImVE, w, h);
	}
	else
	{
		res = GetFastTransformedImage(ImRGB, ImF, ImVE, w, h);
	}

	return res;
}

int GetAndConvertImage(int *ImRGB, int *ImFF, int *ImSF, int *ImTF, int *ImVE, int *ImNE, int *ImHE, CVideo *pVideo, int w, int h)
{
	int *ImRES=g_ImRES11;
	int i, wh, S;
	int res;
	
	wh = w*h;

    pVideo->GetRGBImage(ImRGB, g_xmin, g_xmax, g_ymin, g_ymax);

	res = GetTransformedImage(ImRGB, ImFF, ImSF, ImTF, ImVE, ImNE, ImHE, w, h);

	S = 0;

	if (res == 1)
	{		
		for(i=0; i<wh; i++)
		{
			if (ImTF[i] == 255) S++;
		}
	}

	return S;
}

void ImToNativeSize(int *Im, int w, int h)
{
	int *ImRES=g_ImRES12;
	int i, j, dj, x, y;

	memcpy(ImRES, Im, w*h*sizeof(int));

	memset(Im, 255, g_W*g_H*sizeof(int));
				
	i = 0;
	j = g_ymin*g_W + g_xmin;
	dj = g_W-w;
	for(y=0; y<h; y++)
	{
		for(x=0; x<w; x++)
		{
			Im[j] = ImRES[i];
			i++;
			j++;
		}
		j += dj; 
	}
}

string VideoTimeToStr(s64 pos)
{
	string Str;
	static char str[100];
	int hour, min, sec, sec_1000, vl;
	
	vl = (int)(pos/10000000);
	hour = vl/3600;
	vl -= hour*3600;
	min = vl/60;
	vl -= min*60;
	sec = vl;
	
	_itoa(hour,str,10);
	Str += string(str);
    Str += string("_");
	
	_itoa(min,str,10);
	if (min<=9)
	{
		Str += string("0");
        Str += string(str);
        Str += string("_");
	}
	else 
    {
        Str += string(str);
        Str += string("_");
    }

	_itoa(sec,str,10);
	if (sec<=9)
	{
		Str += string("0");
        Str += string(str);
        Str += string("_");
	}
	else 
    {
        Str += string(str);
        Str += string("_");
    }

	sec_1000 = (int)((pos%10000000)/10000);
	_itoa(sec_1000,str,10);
	if (sec_1000<=9)
	{
		Str += string("00");
        Str += string(str);
	}
	else 
	{
		if (sec_1000<=99)
		{
			Str += string("0");
            Str += string(str);
		}
		else Str += string(str);
	}

	return Str;
}
