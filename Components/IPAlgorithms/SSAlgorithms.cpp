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
#include <regex>
#include <ppl.h>
#include <ppltasks.h>

int		g_RunSubSearch = 0;

int    g_threads = 4;

int		g_DL = 6;	 //sub frame length
double	g_tp = 0.3;	 //text procent
double	g_mtpl = 0.022;  //min text len (in procent)
double	g_sse = 0.3;   //sub square error
double	g_veple = 0.35; //vedges points line error
//double	g_de;	 //density error
//double	g_lle;	 //line length error

CVideo *g_pV;

inline concurrency::task<void> TaskConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNE, int &w, int &h, int &W, int &H, int &res)
{	
	return concurrency::create_task([&]
	{
		custom_buffer<int> ImFF(w*h, 0), ImSF(w*h, 0);
		res = ConvertImage(ImRGB, ImFF, ImSF, ImF, ImNE, w, h, W, H);
	});
}

s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{
	string Str;

	s64 CurPos, prevPos, prev_pos, pos;
	int fn; //frame num
	int i, n, nn;
	int w, h, W, H, xmin, xmax, ymin, ymax, size, BufferSize;
	int mtl, DL, segh, threads;
	double sse;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;

	int found_sub, n_fs, n_fs_start, prev_n_fs;

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

	size = w * h;
	BufferSize = size * sizeof(int);

	pV->SetPos(Begin);

	pV->OneStep();
	CurPos = pV->GetPos();

	mtl = (int)(g_mtpl*(double)w);
	DL = g_DL;
	threads = g_threads;
	segh = g_segh;
	n = h / g_segh;
	sse = g_sse;

	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	int SIZE = W * H;

	custom_buffer<int> lb(H, 0), le(H, 0);

	custom_buffer<int> ImS(SIZE, 0); //store image
	custom_buffer<int> ImSP(SIZE, 0); //store image prev
	custom_buffer<int> ImFS(SIZE, 0); //image for save
	custom_buffer<int> ImFSP(SIZE, 0); //image for save prev
	custom_buffer<int> ImSS(SIZE, 0), ImSSP(SIZE, 0);
	custom_buffer<int> ImNES(SIZE, 0), ImNESS(SIZE, 0), ImNESP(SIZE, 0), ImNESSP(SIZE, 0), ImRES(SIZE, 0);
	custom_buffer<custom_buffer<int>> mImRGB(DL*threads, custom_buffer<int>(SIZE, 0)), ImS_SQ(DL + 1, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImNET(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImT(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImNE(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> Im(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> g_lb(n, custom_buffer<int>(w, 0)), g_le(n, custom_buffer<int>(w, 0));
	custom_buffer<int> g_ln(n, 0);
	custom_buffer<int> *pImRGB, *pIm, *pImNE;
	custom_buffer<s64> ImS_Pos(DL + 1, 0);
	custom_buffer<s64> ImS_fn(DL + 1, 0);
	custom_buffer<s64> mPrevPos(DL*threads, 0);
	vector<concurrency::task<void>> thrsT(threads), thrs(threads, concurrency::create_task([]{}));
	custom_buffer<int> thrs_resT(threads, 0), thrs_res(threads, 0);
	custom_buffer<int> thrs_fn(threads, 0);
	custom_buffer<custom_buffer<int>> thrImRGB(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<s64> thrPrevPos(threads, 0);
	int thr_n, thr_nT;
	bool thrs_were_created = false;

	int debug = 0;

	found_sub = 0;
	prev_pos = -2;
	mPrevPos[0] = CurPos;
	pV->GetRGBImage(mImRGB[0], xmin, xmax, ymin, ymax);
	n_fs = 0;
	n_fs_start = 0;
	prev_n_fs = DL * threads;

	prevPos = -2;

	thr_n = threads;
	while ((CurPos < End) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{
		while (found_sub == 0)
		{
			pos = CurPos;
			n_fs = 0;
			n_fs_start = 0;

			while ((n_fs < DL*threads) && (pos < End))
			{
				if (prev_n_fs < DL * threads)
				{
					mPrevPos[n_fs] = mPrevPos[prev_n_fs];
					mImRGB[n_fs] = mImRGB[prev_n_fs];
					prev_n_fs++;
					thr_n++;
				}
				else if (thr_n < threads)
				{
					mPrevPos[n_fs] = thrPrevPos[thr_n];
					mImRGB[n_fs] = thrImRGB[thr_n];
					thr_n++;
				}
				else
				{
					mPrevPos[n_fs] = pos = pV->OneStepWithTimeout();
					pV->GetRGBImage(mImRGB[n_fs], xmin, xmax, ymin, ymax);
				}

				fn++;
				n_fs++;

				if (n_fs % DL == 0)
				{
					int thr_nT = (n_fs / DL) - 1;
					thrsT[thr_nT] = TaskConvertImage(mImRGB[n_fs - 1], ImT[thr_nT], ImNET[thr_nT], w, h, W, H, thrs_resT[thr_nT]);
				}
			}
			if (pos >= End)
			{
				if (n_fs == 0)
				{
					break;
				}

				while (n_fs < DL*threads)
				{
					mPrevPos[n_fs] = pos;
					memcpy(&(mImRGB[n_fs][0]), &(mImRGB[n_fs - 1][0]), BufferSize);

					fn++;
					n_fs++;
				}
			}

			concurrency::when_all(begin(thrsT), end(thrsT)).wait();

			for (thr_nT = 0; thr_nT < threads; thr_nT++)
			{
				bln = thrs_resT[thr_nT];
				if (bln == 1) break;
			}

			if (bln == 1)
			{
				fn = (fn - DL * threads) + DL * thr_nT;
				n_fs = DL * thr_nT;
				n_fs_start = n_fs;
				found_sub = 1;
				thr_n = 0;
			}
			else
			{
				prev_pos = CurPos = mPrevPos[(DL * threads) - 1];
				n_fs = 0;
				n_fs_start = 0;
			}

			if ((mPrevPos[n_fs_start + DL - 1] >= End) || (g_RunSubSearch == 0) || (mPrevPos[n_fs_start + DL - 1] == mPrevPos[n_fs_start + DL - 2]))
			{
				break;
			}
		}

		if ((mPrevPos[n_fs_start + DL - 1] > End) || (g_RunSubSearch == 0) || (mPrevPos[n_fs_start + DL - 1] == mPrevPos[n_fs_start + DL - 2]) ||
			((found_sub == 0) && (mPrevPos[n_fs_start + DL - 1] == End)))
		{			
			break;
		}

		if (thr_n % threads == 0)
		{
			thr_n = 0;
			int _thr_n, _n_fs;
			for (_thr_n = 0, _n_fs = n_fs; _thr_n < threads; _thr_n++)
			{
				if (_n_fs >= (DL*threads))
				{
					thrPrevPos[_thr_n] = pV->OneStepWithTimeout();
					pV->GetRGBImage(thrImRGB[_thr_n], xmin, xmax, ymin, ymax);
					pImRGB = &(thrImRGB[_thr_n]);
				}
				else
				{
					pImRGB = &(mImRGB[_n_fs]);
				}
				_n_fs++;

				if ((_n_fs > (DL*threads)) || (_n_fs % DL != 0))
				{
					thrs[_thr_n] = TaskConvertImage(*pImRGB, Im[_thr_n], ImNE[_thr_n], w, h, W, H, thrs_res[_thr_n]);
				}
				/*else
				{
					int thr_nT = (_n_fs / DL) - 1;
					thrs_res[_thr_n] = thrs_resT[thr_nT];
					Im[_thr_n] = ImT[thr_nT];
					ImVE[_thr_n] = ImVET[thr_nT];
				}*/
			}
		}

		if (n_fs < (DL*threads))
		{
			pImRGB = &(mImRGB[n_fs]);
			CurPos = mPrevPos[n_fs];

			if (n_fs == 0) prevPos = prev_pos;
			else prevPos = mPrevPos[n_fs - 1];
		}
		else
		{
			pImRGB = &(thrImRGB[thr_n]);
			prevPos = CurPos;
			CurPos = thrPrevPos[thr_n];
		}

		fn++;
		n_fs++;

		if ((n_fs > (DL*threads)) || (n_fs % DL != 0))
		{
			thrs[thr_n].wait();
			bln = thrs_res[thr_n];
			pIm = &Im[thr_n];
			pImNE = &ImNE[thr_n];
		}
		else
		{
			int thr_nT = (n_fs / DL) - 1;
			bln = thrs_resT[thr_nT];
			pIm = &ImT[thr_nT];
			pImNE = &ImNET[thr_nT];
		}

		thr_n++;

		if ((bln == 1) && (CurPos != prevPos))
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
				if (fn - bf < DL)
				{
					if (CompareTwoImages(ImS, ImNES, *pIm, *pImNE, size) == 0)
					{
						if (debug)
						{
							SaveGreyscaleImage(ImS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNES_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pIm, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImNE, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImNE_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						//'bln' here is combined size of interesting pixels
						for (i = 0, bln = 1; (i < nn) && (bln > 0); i++)
						{
							bln = SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
						}
						memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);
						//if (debug) {  SaveGreyscaleImage(ImSS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSS_line" + std::to_string(__LINE__) + "" + g_im_save_format, w, h); }

						bln = SimpleCombineTwoImages(ImS, *pIm, size);

						if (bln > 0)
						{
							bln = AnalyseImage(ImS, w, h);
						}

						if (bln > 0)
						{
							bln = CompareTwoImages(ImS, ImNES, ImS, *pImNE, size);
						}

						if (bln == 0)
						{
							if (finded_prev == 1)
							{
								// restoring ImS
								memcpy(ImS.m_pData, ImSS.m_pData, BufferSize);

								goto L2;
							}
							else
							{
								memcpy(ImS.m_pData, pIm->m_pData, BufferSize);

								int ii = 0;
								for (i = nn - 1, bln = 1; (i > 0) && (bln > 0); i--)
								{
									bln = SimpleCombineTwoImages(ImS, ImS_SQ[i], size);

									if (bln > 0)
									{
										bln = AnalyseImage(ImS, w, h);
									}

									if (bln > 0)
									{
										bln = CompareTwoImages(ImS, ImNES, ImS, *pImNE, size);
									}

									if (bln > 0)
									{
										ii = i;
									}
								}

								if (ii == 0)
								{
									bf = -2;
									goto L;
								}
								else
								{
									memcpy(ImS.m_pData, ImS_SQ[ii].m_pData, BufferSize);
									for (i = 0; i < nn - ii; i++)
									{
										memcpy(ImS_SQ[i].m_pData, ImS_SQ[i + ii].m_pData, BufferSize);
										ImS_Pos[i] = ImS_Pos[i + ii];
										ImS_fn[i] = ImS_fn[i + ii];
									}
									nn -= ii;
									bt = ImS_Pos[ii];
									bf = ImS_fn[ii];
								}
							}
						}
					}

					memcpy(ImS_SQ[nn].m_pData, pIm->m_pData, BufferSize);
					ImS_Pos[nn] = CurPos;
					ImS_fn[nn] = fn;
					nn++;

					if (fn - bf <= 3)
					{
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);
						memcpy(ImS.m_pData, pIm->m_pData, BufferSize);
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
					}
				}
				else
				{
					if (fn - bf == DL)
					{
						//'bln' here is combined size of interesting pixels
						for (i = 0, bln = 1; (i < nn) && (bln > 0); i++)
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
							memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);
							//if (debug) {  SaveGreyscaleImage(ImSS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSS_line" + std::to_string(__LINE__)  + g_im_save_format, w, h); }
						}
					}

					if (fn - bf >= DL)
					{
						if (CompareTwoSubs(ImS, ImNES, *pIm, *pImNE, w, h, W, H, ymin) == 0)
						{
L2:							if (finded_prev == 1)
							{
								bln = CompareTwoSubs(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);

								if (bln == 0)
								{
									bln = CompareTwoSubs(ImSSP, ImNESP, ImSS, ImNES, w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);
									//bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImSSP, ImNESP, ImSS, ImNES, w, h, W, H, ymin);
									//bln = DifficultCompareTwoSubs(ImFSP, ImSSP, ImFS, ImSS, w, h, W, H, ymin);
								}

								if (bln == 1)
								{
									pef = fn - 1;
									pet = CurPos - 1;

									SimpleCombineTwoImages(ImSSP, ImSS, size);
								}
								else
								{
									if (debug)
									{
										SaveRGBImage(ImFS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImSS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImNES, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNES_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

										SaveRGBImage(ImFSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImSSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
										SaveGreyscaleImage(ImNESP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNESP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									}

									Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
									ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
									ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
									g_pViewImage[0](ImFSP, W, H);
									g_pViewImage[1](ImSP, W, H);
									SaveRGBImage(ImFSP, string("\\RGBImages\\") + Str + g_im_save_format, W, H);
									SaveGreyscaleImage(ImSP, string("\\FRDImages\\") + Str + string("!") + g_im_save_format, W, H);

									pbf = bf;
									pbt = bt;
									pef = fn - 1;
									pet = CurPos - 1;

									memcpy(ImSSP.m_pData, ImSS.m_pData, BufferSize);
								}
							}
							else
							{
								pbf = bf;
								pbt = bt;
								pef = fn - 1;
								pet = CurPos - 1;

								memcpy(ImSSP.m_pData, ImSS.m_pData, BufferSize);
							}

							if (pef - pbf + 1 >= DL)
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
							//if (debug) {  SaveGreyscaleImage(ImSS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSS_line" + std::to_string(__LINE__)  + g_im_save_format, w, h); }
						}
					}
				}
			}
		}
		else if (((bln == 0) && (CurPos != prevPos)) ||
			((bln == 1) && (CurPos == prevPos)))
		{
			if (finded_prev == 1)
			{
				if (fn - bf <= DL)
				{
					for (i = 0; i < nn; i++)
					{
						SimpleCombineTwoImages(ImS, ImS_SQ[i], size);
					}
					memcpy(ImSS.m_pData, ImS.m_pData, BufferSize);

					bln = AnalyseImage(ImS, w, h);

					if (bln == 0)
					{
						if (debug)
						{
							SaveRGBImage(*pImRGB, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pIm, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(ImFS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);							
						}

						Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
						ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
						ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewImage[0](ImFSP, W, H);
						g_pViewImage[1](ImSP, W, H);
						SaveRGBImage(ImFSP, string("\\RGBImages\\") + Str + g_im_save_format, W, H);
						SaveGreyscaleImage(ImSP, string("\\FRDImages\\") + Str + string("!") + g_im_save_format, W, H);
						finded_prev = 0;
						bf = -2;
					}
				}

				if (finded_prev == 1)
				{
					bln = CompareTwoSubs(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);

					if (bln == 0)
					{
						bln = CompareTwoSubs(ImSSP, ImNESP, ImSS, ImNES, w, h, W, H, ymin);
					}

					if (bln == 0)
					{
						bln = DifficultCompareTwoSubs2(ImSP, ImNESP, ImS, ImNES, w, h, W, H, ymin);
						//bln = DifficultCompareTwoSubs(ImFSP, ImSP, ImFS, ImS, w, h, W, H, ymin);
					}

					if (bln == 0)
					{
						bln = DifficultCompareTwoSubs2(ImSSP, ImNESP, ImSS, ImNES, w, h, W, H, ymin);
						//bln = DifficultCompareTwoSubs(ImFSP, ImSSP, ImFS, ImSS, w, h, W, H, ymin);
					}

					if (bln == 1)
					{
						SimpleCombineTwoImages(ImSS, ImSSP, size);
						memcpy(ImFS.m_pData, ImFSP.m_pData, BufferSize);
						bf = pbf;
						bt = pbt;
					}
					else
					{
						if (debug)
						{
							SaveRGBImage(*pImRGB, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pIm, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(ImFS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImSS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNES_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(ImFSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImSSP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNESP, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNESP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
						ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
						ImToNativeSize(ImSSP, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewImage[0](ImFSP, W, H);
						g_pViewImage[1](ImSP, W, H);
						SaveRGBImage(ImFSP, string("\\RGBImages\\") + Str + g_im_save_format, W, H);
						SaveGreyscaleImage(ImSP, string("\\FRDImages\\") + Str + string("!") + g_im_save_format, W, H);
					}
				}
			}

			if (bf != -2)
			{
				if (fn - bf > DL)
				{
					if (debug)
					{
						SaveRGBImage(ImFS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						SaveGreyscaleImage(ImS, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						SaveRGBImage(*pImRGB, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						SaveGreyscaleImage(*pIm, string("\\TestImages\\FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
					}

					et = CurPos - 1;
					Str = VideoTimeToStr(bt) + string("__") + VideoTimeToStr(et);
					ImToNativeSize(ImFS, w, h, W, H, xmin, xmax, ymin, ymax);
					ImToNativeSize(ImSS, w, h, W, H, xmin, xmax, ymin, ymax);
					g_pViewImage[0](ImFS, W, H);
					g_pViewImage[1](ImS, W, H);
					SaveRGBImage(ImFS, string("\\RGBImages\\") + Str + g_im_save_format, W, H);
					SaveGreyscaleImage(ImS, string("\\FRDImages\\") + Str + string("!") + g_im_save_format, W, H);
				}
			}

			finded_prev = 0;
			bf = -2;

			if ((n_fs - n_fs_start) >= DL)
			{
				while ( (thr_n < threads) && (bln == 0) )
				{
					if (n_fs < (DL*threads))
					{
						pImRGB = &(mImRGB[n_fs]);
						CurPos = mPrevPos[n_fs];

						if (n_fs == 0) prevPos = prev_pos;
						else prevPos = mPrevPos[n_fs - 1];
					}
					else
					{
						pImRGB = &(thrImRGB[thr_n]);
						prevPos = CurPos;
						CurPos = thrPrevPos[thr_n];
					}

					fn++;
					n_fs++;

					if ((n_fs > (DL*threads)) || (n_fs % DL != 0))
					{
						thrs[thr_n].wait();
						bln = thrs_res[thr_n];
						pIm = &Im[thr_n];
						pImNE = &ImNE[thr_n];
					}
					else
					{
						int thr_nT = (n_fs / DL) - 1;
						bln = thrs_resT[thr_nT];
						pIm = &ImT[thr_nT];
						pImNE = &ImNET[thr_nT];
					}

					thr_n++;
				}

				if ((bln == 1) && (CurPos != prevPos))
				{
					goto L;
				}
				else
				{
					found_sub = 0;
					prev_pos = CurPos;
					prev_n_fs = n_fs;
					concurrency::when_all(begin(thrs), end(thrs)).wait();
				}
			}
		}
	}

	g_pV = NULL;

	concurrency::when_all(begin(thrs), end(thrs)).wait();

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

// return 1 if similar else 0
int CompareTwoImages(custom_buffer<int> &Im1, custom_buffer<int> &ImNFF1, custom_buffer<int> &Im2, custom_buffer<int> &ImNFF2, int size)
{
	int i, dif1, dif2, cmp, val1, val2, val3;

	dif1 = 0;
	dif2 = 0;
	cmp = 0;

	for(i=0; i<size; i++)
	{
		val1 = //Im1[i] | //can skip sequential subs with mostly same size
				ImNFF1[i];
		val2 = //Im2[i] | //can skip sequential subs with mostly same size
				ImNFF2[i];
		val3 = Im1[i] | Im2[i];

		if (val3 == 255)
		{
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

	if (cmp == 0) return 0;

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

int GetLinesInfo(custom_buffer<int> &ImRES, custom_buffer<int> &lb, custom_buffer<int> &le, int w, int h) // return ln
{
	int i, ib, ie, y, l, ln, bln, val1, val2, segh, dn;
	
	segh = g_segh;
		
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

int DifficultCompareTwoSubs2(custom_buffer<int> &ImF1, custom_buffer<int> &ImNE1, custom_buffer<int> &ImF2, custom_buffer<int> &ImNE2, int w, int h, int W, int H, int ymin)
{
	custom_buffer<int> ImFF1(ImF1), ImFF2(ImF2);
	custom_buffer<int> ImRES(w*h, 0);
	custom_buffer<int> lb(h, 0), le(h, 0);
	int res, ln;	

	auto filter_image = [&w, &h](custom_buffer<int> &ImFF) {
		custom_buffer<int> lb(h, 0), le(h, 0);
		int ln;
		ln = GetLinesInfo(ImFF, lb, le, w, h);
		for (int l = 0; l < ln; l++)
		{
			int hh = (le[l] - lb[l] + 1);
			custom_buffer<int> SubIm(w*hh, 0);
			memcpy(&SubIm[0], &ImFF[w*lb[l]], w*hh * sizeof(int));
			if (AnalyseImage(SubIm, w, hh) == 0)
			{
				memset(&ImFF[w*lb[l]], 0, w*hh * sizeof(int));
			}
		}
	};

	AddTwoImages(ImF1, ImF2, ImRES, w*h);
	ln = GetLinesInfo(ImRES, lb, le, w, h);
	
	concurrency::parallel_invoke(
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF1, "\\TestImages\\DifficultCompareTwoSubs2_01_1_ImFF1" + g_im_save_format, w, h);
			FilterImage(ImFF1, ImNE1, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "\\TestImages\\DifficultCompareTwoSubs2_01_2_ImFF1F" + g_im_save_format, w, h);
			filter_image(ImFF1);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "\\TestImages\\DifficultCompareTwoSubs2_01_3_ImFF1FF" + g_im_save_format, w, h);
		},
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "\\TestImages\\DifficultCompareTwoSubs2_02_1_ImFF2" + g_im_save_format, w, h);
			FilterImage(ImFF2, ImNE2, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "\\TestImages\\DifficultCompareTwoSubs2_02_2_ImFF2F" + g_im_save_format, w, h);
			filter_image(ImFF2);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "\\TestImages\\DifficultCompareTwoSubs2_02_3_ImFF2FF" + g_im_save_format, w, h);
		}
	);	

	res = CompareTwoSubs(ImFF1, ImNE1, ImFF2, ImNE2, w, h, W, H, ymin);

	return res;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int DifficultCompareTwoSubs(custom_buffer<int> &ImRGB1, custom_buffer<int> &ImF1, custom_buffer<int> &ImRGB2, custom_buffer<int> &ImF2, int w, int h, int W, int H, int ymin)
{
	custom_buffer<int> ImFF1(w*h, 0), ImNE1(w*h, 0);
	custom_buffer<int> ImFF2(w*h, 0), ImNE2(w*h, 0);
	custom_buffer<int> ImTEMP1(w*h, 0), ImTEMP2(w*h, 0), ImTEMP3(w*h, 0);
	custom_buffer<int> lb(h, 0), le(h, 0);
	int res, size, i;

	res = 0;

	size = w*h;

	GetTransformedImage(ImRGB1, ImTEMP1, ImTEMP2, ImFF1, ImNE1, w, h, W, H);
	GetTransformedImage(ImRGB2, ImTEMP1, ImTEMP2, ImFF2, ImNE2, w, h, W, H);

	for(i=0; i<size; i++) 
	{
		if (ImF1[i] == 0)
		{
			ImFF1[i] = 0;
		}

		if (ImF2[i] == 0)
		{
			ImFF2[i] = 0;
		}
	}
	
	res = CompareTwoSubs(ImFF1, ImNE1, ImFF2, ImNE2, w, h, W, H, ymin);

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

	if (ln == 0)
	{
		return 0;
	}

	bln = 1;
	for(k=0; k<ln; k++)
	{
		ib = (lb[k]+1)*w;
		ie = (le[k]-1)*w;
		
		if (((ln > 0) && (l < ln - 1) && (lb[l] + ymin > H / 4) &&
			(le[l] + ymin < H / 2) && (lb[ln - 1] + ymin > H / 2))) // egnoring garbage line
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
				val1 = //Im1[i] | //can skip sequential subs with mostly same size
						ImVE1[i];
				val2 = //Im2[i] | //can skip sequential subs with mostly same size
						ImVE2[i];

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

		if (cmb == 0)
		{
			bln = 0;
			break;
		}

		if ((double)dif/(double)cmb <= veple)
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
				val1 = //Im1[i] | Im1[i - w] | Im1[i + w] | //can skip sequential subs with mostly same size
					   ImVE1[i] | ImVE1[i-w] | ImVE1[i+w];
				val2 = //Im2[i] | Im2[i - w] | Im2[i + w] | //can skip sequential subs with mostly same size
					   ImVE2[i] | ImVE2[i-w] | ImVE2[i+w];

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

		if (cmb == 0)
		{
			bln = 0;
			break;
		}

		if ((double)dif/(double)cmb > veple)
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

std::string VideoTimeToStr(s64 pos)
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

std::string GetFileName(std::string FilePath)
{
	std::regex re("([^\\\\\\/\\.]+)\\.[^\\\\\\/\\.]+$");
	std::smatch match;
	std::string res;

	if (std::regex_search(FilePath, match, re) && match.size() > 1) {
		res = match.str(1);
	}

	return res;
}
