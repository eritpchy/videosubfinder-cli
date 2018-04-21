                              //SSAlgorithms.cpp//                                
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
	string Str;
	
	s64 CurPos;
	int fn; //frame num
	int i, n, nn, ln;
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

	custom_buffer<int> lb(g_H, 0), le(g_H, 0);

	custom_buffer<int> ImRGB(SIZE, 0), Im(SIZE, 0), ImSF(SIZE, 0), ImNFF(SIZE, 0), ImNFFS(SIZE, 0);
	custom_buffer<int> ImS(SIZE, 0); //store image
	custom_buffer<int> ImSP(SIZE, 0); //store image prev
	custom_buffer<int> ImFS(SIZE, 0); //image for save
	custom_buffer<int> ImFSP(SIZE, 0); //image for save prev
	custom_buffer<int> ImVE(SIZE, 0), ImVES(SIZE, 0), ImVESS(SIZE, 0), ImVESP(SIZE, 0), ImVESSP(SIZE, 0);
	custom_buffer<int> ImNE(SIZE, 0), ImNES(SIZE, 0), ImNESP(SIZE, 0), ImHE(SIZE, 0), ImRES(SIZE, 0);
	custom_buffer<custom_buffer<int>> ImS_SQ(DL, custom_buffer<int>(SIZE, 0)), ImVES_SQ(DL, custom_buffer<int>(SIZE, 0)), ImNES_SQ(DL, custom_buffer<int>(SIZE, 0));

	custom_buffer<custom_buffer<int>> g_lb(n, custom_buffer<int>(w, 0)), g_le(n, custom_buffer<int>(w, 0));	
	custom_buffer<int> g_ln(n, 0);

	prevPos = -2;

	while ((CurPos < End) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{	
		Str = VideoTimeToStr(CurPos);

		S = GetAndConvertImage(ImRGB, ImNFF, ImSF, Im, ImVE, ImNE, ImHE, pV, w, h);

		if ( (S > 0) && (CurPos != prevPos) )
		{	
			if (bf == -2)
			{
L:				bf = fn;
				bt = CurPos;

				SP = S;
				memcpy(&ImS[0], &Im[0], BufferSize);
				memcpy(&ImNFFS[0], &ImNFF[0], BufferSize);
				memcpy(&ImVES[0], &ImVE[0], BufferSize);
				memcpy(&ImNES[0], &ImNE[0], BufferSize);
				memcpy(&ImFS[0], &ImRGB[0], BufferSize);
				memcpy(&ImVESS[0], &ImVE[0], BufferSize);

				nn = 0;
			}
			else
			{
				if (fn-bf < DL)
				{
					/*g_pMF->SaveGreyscaleImage(ImS, "\\TestImages\\Cmb1!.jpeg", w, h);
					g_pMF->SaveGreyscaleImage(Im, "\\TestImages\\Cmb2!.jpeg", w, h);
					g_pMF->SaveGreyscaleImage(ImNFFS, "\\TestImages\\Cmb3!.jpeg", w, h);
					g_pMF->SaveGreyscaleImage(ImNFF, "\\TestImages\\Cmb4!.jpeg", w, h);*/

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
						memcpy(&ImS[0], &Im[0], BufferSize);
						memcpy(&ImNFFS[0], &ImNFF[0], BufferSize);
						memcpy(&ImVES[0], &ImVE[0], BufferSize);
						memcpy(&ImNES[0], &ImNE[0], BufferSize);
					}
					else
					{
						memcpy(&(ImS_SQ[nn][0]), &Im[0], BufferSize);
						memcpy(&(ImVES_SQ[nn][0]), &ImVE[0], BufferSize);
						memcpy(&(ImNES_SQ[nn][0]), &ImNE[0], BufferSize);
						nn++;
					}

					if (fn-bf == 3)
					{
						memcpy(&ImFS[0], &ImRGB[0], BufferSize);
						memcpy(&ImVESS[0], &ImVE[0], BufferSize);
						memcpy(&ImNES[0], &ImNE[0], BufferSize);
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
									pet = CurPos-1;
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
									SaveGreyscaleImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
									SaveGreyscaleImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);
									
									pbf = bf;
									pbt = bt;
									pef = fn-1;
									pet = CurPos-1;
								}
							}	
							else
							{
								pbf = bf;
								pbt = bt;
								pef = fn-1;
								pet = CurPos-1;
							}

							if (pef-pbf+1 >= DL)
							{
								memcpy(&ImSP[0], &ImS[0], BufferSize);
								memcpy(&ImFSP[0], &ImFS[0], BufferSize);
								memcpy(&ImVESP[0], &ImVES[0], BufferSize);
								memcpy(&ImVESSP[0], &ImVESS[0], BufferSize);
								memcpy(&ImNESP[0], &ImNES[0], BufferSize);
											
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
						SaveGreyscaleImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
						SaveGreyscaleImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
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
						memcpy(&ImS[0], &ImRES[0], BufferSize);
						memcpy(&ImFS[0], &ImFSP[0], BufferSize);
						memcpy(&ImVESS[0], &ImVESSP[0], BufferSize);
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
						SaveGreyscaleImage(ImVESSP, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
						SaveGreyscaleImage(ImSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
					}
				}
			}

			if (bf != -2)
			{
				if (fn-bf > DL)
				{			
					et = CurPos-1;
					Str = VideoTimeToStr(bt)+string("__")+VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h);
					ImToNativeSize(ImS, w, h);
					ImToNativeSize(ImVESS, w, h);
					g_pViewImage[0](ImFS, g_W, g_H);									
					g_pViewImage[1](ImS, g_W, g_H);									
					SaveRGBImage(ImFS, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
					SaveGreyscaleImage(ImVESS, string("\\FRDImages\\")+Str+string("!!.jpeg"), g_W, g_H);
					SaveGreyscaleImage(ImS, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);						
				}
			}

			finded_prev = 0;
			bf = -2;
		}

		prevPos = CurPos;
		
        CurPos = pV->OneStepWithTimeout();

		fn++;
	}

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
	string Str;
	
	s64 CurPos, pos;
	int fn; //frame num
	int i, n, nn, ln;
	int w, h, size, BufferSize;
	int mtl, DL, segh;
	double sse;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;
	s64 prevPos;

	int found_sub, n_fs;	
	s64 prev_pos;

	int bln, finded_prev;
	
	g_RunSubSearch = 1;

	g_pV = pV;

	w = g_w;
	h = g_h;

	size = w*h;
	BufferSize = size*sizeof(int);

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

	custom_buffer<int> lb(g_H, 0), le(g_H, 0);

	custom_buffer<int> ImRGB(SIZE, 0), Im(SIZE, 0), ImSF(SIZE, 0), ImNFF(SIZE, 0), ImNFFS(SIZE, 0);
	custom_buffer<int> ImS(SIZE, 0); //store image
	custom_buffer<int> ImSP(SIZE, 0); //store image prev
	custom_buffer<int> ImFS(SIZE, 0); //image for save
	custom_buffer<int> ImFSP(SIZE, 0); //image for save prev
	custom_buffer<int> ImVE(SIZE, 0), ImVES(SIZE, 0), ImVESS(SIZE, 0), ImVESP(SIZE, 0), ImVESSP(SIZE, 0), ImVET(SIZE, 0), ImT(SIZE, 0), ImSS(SIZE, 0), ImSSP(SIZE, 0);
	custom_buffer<int> ImNE(SIZE, 0), ImNES(SIZE, 0), ImNESP(SIZE, 0), ImHE(SIZE, 0), ImRES(SIZE, 0);
	custom_buffer<custom_buffer<int>> mImRGB(DL, custom_buffer<int>(SIZE, 0)), ImS_SQ(DL, custom_buffer<int>(SIZE, 0)), ImVES_SQ(DL, custom_buffer<int>(SIZE, 0)), ImNES_SQ(DL, custom_buffer<int>(SIZE, 0));

	custom_buffer<custom_buffer<int>> g_lb(n, custom_buffer<int>(w, 0)), g_le(n, custom_buffer<int>(w, 0));
	custom_buffer<int> g_ln(n, 0);
	custom_buffer<int> *pImRGB, *pIm, *pImVE;
	custom_buffer<s64> mPrevPos(DL, 0);

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
					memcpy(&(mImRGB[n_fs][0]), &(mImRGB[n_fs-1][0]), BufferSize);

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
			pImRGB = &(mImRGB[n_fs]);
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

			pImRGB = &ImRGB;

			fn++;
		}

		if (n_fs != DL)
		{
			bln = ConvertImage(*pImRGB, Im, ImVE, w, h);
			pIm = &Im;
			pImVE = &ImVE;
		}
		else
		{
			bln = 1;
			pIm = &ImT;
			pImVE = &ImVET;
			n_fs++;
		}

		if ( (bln == 1) && (CurPos != prevPos) )
		{	
			if (bf == -2)
			{
L:				bf = fn;
				bt = CurPos;

				memcpy(ImS.m_pData, pIm->m_pData, BufferSize);
				memcpy(ImVES.m_pData, pImVE->m_pData, BufferSize);
				memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);

				nn = 0;
			}
			else
			{
				if (fn-bf < DL)
				{
					/*if (g_debug == 1)
					{
						g_pMF->SaveGreyscaleImage(ImS, "\\TestImages\\Cmb1!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(Im, "\\TestImages\\Cmb2!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(ImVES, "\\TestImages\\Cmb3!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(ImVE, "\\TestImages\\Cmb4!.jpeg", w, h);
					}*/

					if(CompareTwoImages(ImS, ImVES, *pIm, *pImVE, size) == 0) 
					{
						if (finded_prev == 1) 
						{
							memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);

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
						memcpy(ImS.m_pData, pIm->m_pData, BufferSize);
						memcpy(ImVES.m_pData, pImVE->m_pData, BufferSize);
					}
					else
					{
						memcpy(ImS_SQ[nn].m_pData, pIm->m_pData, BufferSize);
						nn++;
					}

					if (fn-bf == 3)
					{
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);
						memcpy(ImVES.m_pData, pImVE->m_pData, BufferSize);
					}
				}
				else
				{
					if (fn-bf == DL)
					{	
						//'bln' here is combined size of interesting pixels
						for(i=0, bln=1; (i<nn) && (bln>0); i++)
						{
							bln = SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
						}

						if (bln > 0)
						{
							bln = AnalyseImage(ImS, w, h);
						}

						if (bln == 0) 						
						{
							bf = -2;
							goto L;
						}
						else
						{
							if (finded_prev == 0) memcpy(ImSS.m_pData, ImS_SQ[nn - 1].m_pData, BufferSize);
							else memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);
						}
					}

					if (fn-bf >= DL)
					{												
						if (CompareTwoSubs(ImS, ImVES, *pIm, *pImVE, w, h) == 0)
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
									pet = CurPos-1;

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
									SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);								

									pbf = bf;
									pbt = bt;
									pef = fn-1;
									pet = CurPos-1;

									memcpy(ImSSP.m_pData, ImSS.m_pData, BufferSize);
								}
							}	
							else
							{								
								pbf = bf;
								pbt = bt;
								pef = fn-1;
								pet = CurPos-1;

								memcpy(ImSSP.m_pData, ImSS.m_pData, BufferSize);
							}

							if (pef-pbf+1 >= DL)
							{
								memcpy(ImSP.m_pData, ImS.m_pData, BufferSize);
								memcpy(ImFSP.m_pData, ImFS.m_pData, BufferSize);
								memcpy(ImVESP.m_pData, ImVES.m_pData, BufferSize);
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
							SimpleCombineTwoImages(ImSS, *pIm, size);
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
					memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);

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
						SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
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
						memcpy(ImS.m_pData, ImRES.m_pData, BufferSize);
						memcpy(ImFS.m_pData, ImFSP.m_pData, BufferSize);
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
						SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
					}
				}
			}

			if (bf != -2)
			{
				if (fn-bf > DL)
				{			
					et = CurPos-1;
					Str = VideoTimeToStr(bt)+string("__")+VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h);
					ImToNativeSize(ImSS, w, h);
					g_pViewImage[0](ImFS, g_W, g_H);									
					g_pViewImage[1](ImSS, g_W, g_H);									
					SaveRGBImage(ImFS, string("\\RGBImages\\")+Str+string(".jpeg"), g_W, g_H);
					SaveGreyscaleImage(ImSS, string("\\FRDImages\\")+Str+string("!.jpeg"), g_W, g_H);														
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

int CompareTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &ImNFF1, custom_buffer<int> &Im2, custom_buffer<int> &ImNFF2, int size)
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

int AnalyseImage(custom_buffer<int> &Im, int w, int h)
{
	int i, k, l, x, y, ia, da, pl, mpl, i_mpl, len, len2, val1, val2, n, bln;
	int segh, mtl;	
	double tp;
	
	segh = g_segh;
	tp = g_tp;
	mtl = (int)(g_mtpl*(double)w);
	
	n = h/segh;
	da = w*segh;
	
	custom_buffer<custom_buffer<int>> g_lb(n, custom_buffer<int>(w, 0)), g_le(n, custom_buffer<int>(w, 0));
	custom_buffer<int> g_ln(n, 0);

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

	if (mpl == 0)
	{
		return 0;
	}

	// находим cоотнощение длины текста к занимаемому им месту
	k = i_mpl;
	len = 0;
	for (l=0; l<g_ln[k]; l++)
	{
		len += g_le[k][l]-g_lb[k][l]+1;
	}
	l--;
	
	while(l>0)
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

int PreCompareTwoSubs(custom_buffer<int> &Im1, custom_buffer<int> &Im2, custom_buffer<int> &ImRES, custom_buffer<int> &lb, custom_buffer<int> &le, int w, int h) // return ln
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

int FinalCompareTwoSubs1(custom_buffer<int> &ImRES, custom_buffer<int> &lb, custom_buffer<int> &le, int ln, custom_buffer<int> &ImVE1, custom_buffer<int> &ImVE2, int w, int h)
{
	int i, ib, ie, k, val1, val2, dif1, dif2, cmb;	
	int bln;
	double veple;

	//g_pMF->SaveGreyscaleImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
	//g_pMF->SaveGreyscaleImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
	//g_pMF->SaveGreyscaleImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);

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

int FinalCompareTwoSubs2(custom_buffer<int> &ImRES, custom_buffer<int> &lb, custom_buffer<int> &le, int ln, custom_buffer<int> &ImVE1, custom_buffer<int> &ImVE2, int w, int h)
{
	int i, ib, ie, k, val1, val2, dif, dif1, dif2, cmb;
	int bln;
	double veple;

	/*if (g_debug == 1) 
	{
		g_pMF->SaveGreyscaleImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
		g_pMF->SaveGreyscaleImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
		g_pMF->SaveGreyscaleImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);
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

int DifficultCompareTwoSubs(custom_buffer<int> &ImRGB1, custom_buffer<int> &ImF1, custom_buffer<int> &ImRGB2, custom_buffer<int> &ImF2, int w, int h)
{
	custom_buffer<int> ImFF1(w*h, 0), ImVE1(w*h, 0), ImNE1(w*h, 0);
	custom_buffer<int> ImFF2(w*h, 0), ImVE2(w*h, 0), ImNE2(w*h, 0);
	custom_buffer<int> ImTEMP1(w*h, 0), ImTEMP2(w*h, 0), ImTEMP3(w*h, 0);
	custom_buffer<int> lb(h, 0), le(h, 0);
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

int CompareTwoSubs(custom_buffer<int> &Im1, custom_buffer<int> &ImVE1, custom_buffer<int> &Im2, custom_buffer<int> &ImVE2, int w, int h)
{
	custom_buffer<int> ImRES(w*h, 0), lb(h, 0), le(h, 0);
	int i, ib, ie, k, y, l, ln, bln, val1, val2, dif, dif1, dif2, cmb, segh, dn;
	double veple;

	veple = g_veple;
	segh = g_segh;

	AddTwoImages(Im1, Im2, ImRES, w*h);
	
	//g_pMF->SaveGreyscaleImage(ImVE1, "\\TestImages\\Cmb1!.jpeg", w, h);
	//g_pMF->SaveGreyscaleImage(ImVE2, "\\TestImages\\Cmb2!.jpeg", w, h);
	//g_pMF->SaveGreyscaleImage(ImRES, "\\TestImages\\Cmb3!.jpeg", w, h);
	
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

int SimpleCombineTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int size)
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

int GetCombinedSquare(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int size)
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

void AddTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, custom_buffer<int> &ImRES, int size)
{	
	int i;

	memcpy(ImRES.m_pData, Im1.m_pData, size*sizeof(int));

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) ImRES[i] = 255;
	}
}

void AddTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &Im2, int size)
{
	int i;

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) Im1[i] = 255;
	}
}

int ConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImVE, int w, int h)
{
	int res = GetFastTransformedImage(ImRGB, ImF, ImVE, w, h);
	return res;
}

int GetAndConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImVE, custom_buffer<int> &ImNE, custom_buffer<int> &ImHE, CVideo *pVideo, int w, int h)
{
	custom_buffer<int> ImRES(w*h, 0);
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

void ImToNativeSize(custom_buffer<int> &Im, int w, int h)
{
	custom_buffer<int> ImRES(w*h, 0);
	int i, j, dj, x, y;

	memcpy(ImRES.m_pData, Im.m_pData, w*h*sizeof(int));

	memset(Im.m_pData, 255, g_W*g_H*sizeof(int));
				
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
	static char str[100];
	int hour, min, sec, msec, val;

	val = (int)(pos / 1000); // seconds
	msec = pos - ((s64)val * (s64)1000);
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	sprintf(str, "%d_%02d_%02d_%03d", hour, min, sec, msec);

	return string(str);
}
