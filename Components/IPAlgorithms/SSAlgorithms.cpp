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

long    g_threads = 16;

int		g_DL = 6;	 //sub frame length
double	g_tp = 0.3;	 //text procent
double	g_mtpl = 0.022;  //min text len (in procent)
double	g_sse = 0.3;   //sub square error
double	g_veple = 0.35; //vedges points line error
//double	g_de;	 //density error
//double	g_lle;	 //line length error

CVideo *g_pV;

s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{	
	string Str;
	
	s64 CurPos, pos;
	int fn; //frame num
	int i, n, nn, ln;
	int w, h, W, H, xmin, xmax, ymin, ymax, size, BufferSize;
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

	w = g_pV->m_w;
	h = g_pV->m_h;
	W = g_pV->m_Width;
	H = g_pV->m_Height;
	xmin = g_pV->m_xmin;
	xmax = g_pV->m_xmax;
	ymin = g_pV->m_ymin;
	ymax = g_pV->m_ymax;

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

	int SIZE = W*H;

	custom_buffer<int> lb(H, 0), le(H, 0);

	custom_buffer<int> ImRGB(SIZE, 0), Im(SIZE, 0), ImNFF(SIZE, 0), ImNFFS(SIZE, 0);
	custom_buffer<int> ImS(SIZE, 0); //store image
	custom_buffer<int> ImSP(SIZE, 0); //store image prev
	custom_buffer<int> ImFS(SIZE, 0); //image for save
	custom_buffer<int> ImFSP(SIZE, 0); //image for save prev
	custom_buffer<int> ImFF(SIZE, 0), ImSF(SIZE, 0), ImTF(SIZE, 0), ImSS(SIZE, 0), ImSSP(SIZE, 0);
	custom_buffer<int> ImNE(SIZE, 0), ImNES(SIZE, 0), ImNESS(SIZE, 0), ImNESP(SIZE, 0), ImNESSP(SIZE, 0), ImNET(SIZE, 0), ImRES(SIZE, 0);
	custom_buffer<custom_buffer<int>> mImRGB(DL, custom_buffer<int>(SIZE, 0)), ImS_SQ(DL, custom_buffer<int>(SIZE, 0)), ImVES_SQ(DL, custom_buffer<int>(SIZE, 0)), ImNES_SQ(DL, custom_buffer<int>(SIZE, 0));

	custom_buffer<custom_buffer<int>> g_lb(n, custom_buffer<int>(w, 0)), g_le(n, custom_buffer<int>(w, 0));
	custom_buffer<int> g_ln(n, 0);
	custom_buffer<int> *pImRGB, *pIm, *pImNE;
	custom_buffer<s64> mPrevPos(DL, 0);

	found_sub = 0;
	prev_pos = -2;
	mPrevPos[0] = CurPos;
	pV->GetRGBImage(mImRGB[0], xmin, xmax, ymin, ymax);
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

				pV->GetRGBImage(mImRGB[n_fs], xmin, xmax, ymin, ymax);

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

			bln = ConvertImage(mImRGB[DL-1], ImFF, ImSF, ImTF, ImNET, w, h, W, H);

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

            pV->GetRGBImage(ImRGB, xmin, xmax, ymin, ymax);

			pImRGB = &ImRGB;

			fn++;
		}

		if (n_fs != DL)
		{
			bln = ConvertImage(*pImRGB, Im, ImSF, ImTF, ImNE, w, h, W, H);
			pIm = &Im;
			pImNE = &ImNE;
		}
		else
		{
			bln = 1;
			pIm = &ImFF;
			pImNE = &ImNET;
			n_fs++;
		}

		if ( (bln == 1) && (CurPos != prevPos) )
		{	
			if (bf == -2)
			{
L:				bf = fn;
				bt = CurPos;

				memcpy(ImS.m_pData, pIm->m_pData, BufferSize);
				memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
				memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);

				nn = 0;
			}
			else
			{
				if (fn-bf < DL)
				{
					/*if (g_show_results)
					{
						g_pMF->SaveGreyscaleImage(ImS, "\\TestImages\\Cmb1!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(Im, "\\TestImages\\Cmb2!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(ImVES, "\\TestImages\\Cmb3!.jpeg", w, h);
						g_pMF->SaveGreyscaleImage(ImVE, "\\TestImages\\Cmb4!.jpeg", w, h);
					}*/

					if(CompareTwoImages(ImS, ImNES, *pIm, *pImNE, size) == 0) 
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
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
					}
					else
					{
						memcpy(ImS_SQ[nn].m_pData, pIm->m_pData, BufferSize);
						nn++;
					}

					if (fn-bf == 3)
					{
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
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
						if (CompareTwoSubs(ImS, ImNES, *pIm, *pImNE, w, h, W, H, ymin) == 0)
						{
L2:							if (finded_prev == 1)
							{
								bln = CompareTwoSubs(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);
								if (bln == 0)
								{
									ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
									bln = FinalCompareTwoSubs2(ImRES, lb, le, ln, ImNESP, ImNES, w, h, W, H, ymin);
								}
								if (bln == 0) bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h, W, H, ymin);
								
								if (bln == 1)
								{
									pef = fn-1;
									pet = CurPos-1;

									SimpleCombineTwoImages(ImSSP, ImSS, size);
								}
								else
								{
									Str = VideoTimeToStr(pbt)+string("__")+VideoTimeToStr(pet);
									ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
									ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
									g_pViewImage[0](ImFSP, W, H);									
									g_pViewImage[1](ImSSP, W, H);									
									SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), W, H);
									SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), W, H);								

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
								memcpy(ImNESP.m_pData, ImNES.m_pData, BufferSize);
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
                        ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
						ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewImage[0](ImFSP, W, H);									
						g_pViewImage[1](ImSSP, W, H);									
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), W, H);
						SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), W, H);														
						finded_prev = 0;
						bf = -2;
					}
				}

				if (finded_prev == 1)
				{
					bln = CompareTwoSubs(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);
					if (bln == 0)
					{
						ln = PreCompareTwoSubs(ImSP, ImS, ImRES, lb, le, w, h);
						bln = FinalCompareTwoSubs2(ImRES, lb, le, ln, ImNESP, ImNES, w, h, W, H, ymin);
					}
					if (bln == 0) bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h, W, H, ymin);
					
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
						ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
						ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewImage[0](ImFSP, W, H);									
						g_pViewImage[1](ImSSP, W, H);									
						SaveRGBImage(ImFSP, string("\\RGBImages\\")+Str+string(".jpeg"), W, H);
						SaveGreyscaleImage(ImSSP, string("\\FRDImages\\")+Str+string("!.jpeg"), W, H);														
					}
				}
			}

			if (bf != -2)
			{
				if (fn-bf > DL)
				{			
					et = CurPos-1;
					Str = VideoTimeToStr(bt)+string("__")+VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h, W, H, xmin, xmax, ymin, ymax);
					ImToNativeSize(ImSS, w, h, W, H, xmin, xmax, ymin, ymax);
					g_pViewImage[0](ImFS, W, H);									
					g_pViewImage[1](ImSS, W, H);									
					SaveRGBImage(ImFS, string("\\RGBImages\\")+Str+string(".jpeg"), W, H);
					SaveGreyscaleImage(ImSS, string("\\FRDImages\\")+Str+string("!.jpeg"), W, H);														
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

	g_pV = NULL;

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

int FinalCompareTwoSubs2(custom_buffer<int> &ImRES, custom_buffer<int> &lb, custom_buffer<int> &le, int ln, custom_buffer<int> &ImVE1, custom_buffer<int> &ImVE2, int w, int h, int W, int H, int ymin)
{
	int i, ib, ie, k, val1, val2, dif, dif1, dif2, cmb;
	int bln;
	double veple;

	/*if (g_show_results) 
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
			 ( (ln > 0) && (k < ln-1) && (lb[k]+ymin > H/4) && 
			   (le[k]+ymin < H/2) && (lb[ln-1]+ymin > H/2) ) )
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
			!( (ln > 0) && (k < ln-1) && (lb[k]+ymin > H/4) && 
			   (le[k]+ymin < H/2) && (lb[ln-1]+ymin > H/2) ) )
		{
			bln = 0;
			break;
		}
	}
	return bln;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int DifficultCompareTwoSubs(custom_buffer<int> &ImRGB1, custom_buffer<int> &ImF1, custom_buffer<int> &ImRGB2, custom_buffer<int> &ImF2, int w, int h, int W, int H, int ymin)
{
	custom_buffer<int> ImFF1(w*h, 0), ImNE1(w*h, 0);
	custom_buffer<int> ImFF2(w*h, 0), ImNE2(w*h, 0);
	custom_buffer<int> ImTEMP1(w*h, 0), ImTEMP2(w*h, 0), ImTEMP3(w*h, 0);
	custom_buffer<int> lb(h, 0), le(h, 0);
	int res, size, ln, i;

	res = 0;

	size = w*h;

	GetTransformedImage(ImRGB1, ImTEMP1, ImTEMP2, ImFF1, ImNE1, w, h, W, H);
	GetTransformedImage(ImRGB2, ImTEMP1, ImTEMP2, ImFF2, ImNE2, w, h, W, H);

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
	
	res = FinalCompareTwoSubs2(ImTEMP1, lb, le, ln, ImNE1, ImNE2, w, h, W, H, ymin);

	if (res == 0) res = FinalCompareTwoSubs1(ImTEMP1, lb, le, ln, ImNE1, ImNE2, w, h);

	return res;
}

int CompareTwoSubs(custom_buffer<int> &Im1, custom_buffer<int> &ImVE1, custom_buffer<int> &Im2, custom_buffer<int> &ImVE2, int w, int h, int W, int H, int ymin)
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
			 ( (ln > 0) && (l < ln-1) && (lb[l]+ymin > H/4) && 
			   (le[l]+ymin < H/2) && (lb[ln-1]+ymin > H/2) ) )
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
			!( (ln > 0) && (l < ln-1) && (lb[l]+ymin > H/4) && 
			   (le[l]+ymin < H/2) && (lb[ln-1]+ymin > H/2) ) )
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

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int ConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, int w, int h, int W, int H)
{
	int res = GetTransformedImage(ImRGB, ImFF, ImSF, ImTF, ImNE, w, h, W, H);
	return res;
}

int GetAndConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImFF, custom_buffer<int> &ImSF, custom_buffer<int> &ImTF, custom_buffer<int> &ImNE, CVideo *pVideo, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	custom_buffer<int> ImRES(w*h, 0);
	int i, wh, S;
	int res;
	
	wh = w*h;

    pVideo->GetRGBImage(ImRGB, xmin, xmax, ymin, ymax);

	res = GetTransformedImage(ImRGB, ImFF, ImSF, ImTF, ImNE, w, h, W, H);

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

// W - full image include scale (if is) width
// H - full image include scale (if is) height
void ImToNativeSize(custom_buffer<int> &Im, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	custom_buffer<int> ImRES(w*h, 0);
	int i, j, dj, x, y;

	memcpy(ImRES.m_pData, Im.m_pData, w*h*sizeof(int));

	memset(Im.m_pData, 255, W*H*sizeof(int));
				
	i = 0;
	j = ymin*W + xmin;
	dj = W-w;
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

inline std::string VideoTimeToStr(s64 pos)
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

