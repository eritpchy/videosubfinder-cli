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
double	g_veple = 0.35; //vedges points line error

bool g_use_ISA_images_for_search_subtitles = true;
bool g_use_ILA_images_for_search_subtitles = true;
bool g_replace_ISA_by_filtered_version = true;
int g_max_dl_down = 20;
int g_max_dl_up = 40;

CVideo *g_pV;

inline int AnalizeImageForSubPresence(custom_buffer<int> &ImRGB, custom_buffer<int> &ImNE, custom_buffer<int> &ImISA, custom_buffer<int> &ImIL, s64 CurPos, int fn, int w, int h, int W, int H)
{
	int res = 1;
	int debug = 0;

	if (g_use_ISA_images_for_search_subtitles)
	{
		custom_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = h - 1;

		custom_buffer<int> ImFF(ImISA), ImTF(w*h, 0);

		if (debug)
		{
			SaveGreyscaleImage(ImISA, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_01_ImISA_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}

		if (g_use_ILA_images_for_search_subtitles)
		{
			if (debug)
			{
				SaveGreyscaleImage(ImIL, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_02_ImIL_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}

			IntersectTwoImages(ImFF, ImIL, w, h);

			if (debug)
			{
				SaveGreyscaleImage(ImFF, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_03_ImFF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}
		}
		
		custom_buffer<int> ImSF(ImFF);

		res = FilterTransformedImage(ImFF, ImSF, ImTF, ImNE, LB, LE, 1, w, h, W, H, VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn));

		if (debug)
		{
			SaveGreyscaleImage(ImTF, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_04_ImTF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}

		if (g_replace_ISA_by_filtered_version)
		{
			memcpy(&(ImISA[0]), &(ImTF[0]), w*h*sizeof(int));
		}
	}

	return res;
}

inline void IntersectYImages(custom_buffer<int> &ImRes, custom_buffer<int> &Im2, int w, int h, int offset_for_im2)
{
	int i, size;

	size = w * h;
	for (i = 0; i < size; i++)
	{
		if (ImRes[i] > 0)
		{
			if (Im2[i] + offset_for_im2 < ImRes[i] - g_max_dl_down)
			{
				ImRes[i] = 0;
			}
			else if (Im2[i] + offset_for_im2 > ImRes[i] + g_max_dl_up)
			{
				ImRes[i] = 0;
			}
		}
	}
}

inline concurrency::task<void> TaskConvertImage(custom_buffer<int> &ImRGB, custom_buffer<int> &ImF, custom_buffer<int> &ImNE, custom_buffer<int> &ImY, int &w, int &h, int &W, int &H, int &res)
{	
	return concurrency::create_task([&]
	{
		custom_buffer<int> ImFF(w*h, 0), ImSF(w*h, 0);
		res = GetTransformedImage(ImRGB, ImFF, ImSF, ImF, ImNE, ImY, w, h, W, H);
	});
}

s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{
	string Str;

	s64 CurPos, prevPos, prev_pos, pos;
	int fn; //frame num
	int w, h, W, H, xmin, xmax, ymin, ymax, size, BufferSize;
	int DL, threads;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;

	int found_sub, n_fs, n_fs_start, prev_n_fs;

	int bln, finded_prev;

	int debug = 0;

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

	/*if (debug)
	{
		ymin = g_pV->m_ymin = 380;
		ymax = g_pV->m_ymax = 449;
		h = g_pV->m_h = ymax - ymin + 1;
	}*/

	size = w * h;
	BufferSize = size * sizeof(int);

	pV->SetPos(Begin);
	CurPos = pV->GetPos();

	DL = g_DL;
	threads = g_threads;

	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	int SIZE = W * H;

	custom_buffer<int> lb(H, 0), le(H, 0);

	custom_buffer<int> ImInt(SIZE, 0), ImInt2(SIZE, 0); //store image
	custom_buffer<int> ImIntS(SIZE, 0); //store image
	custom_buffer<int> ImIntSP(SIZE, 0); //store image prev
	custom_buffer<int> ImFS(SIZE, 0); //image for save
	custom_buffer<int> ImFSP(SIZE, 0); //image for save prev
	custom_buffer<int> ImNES(SIZE, 0), ImNESP(SIZE, 0);
	custom_buffer<int> ImYS(SIZE, 0), ImYSP(SIZE, 0);
	custom_buffer<int> ImRES(SIZE, 0);
	custom_buffer<custom_buffer<int>> mImRGB(DL*threads, custom_buffer<int>(SIZE, 0)), ImS_SQ(DL + 1, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImNET(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImYT(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImT(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImNE(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImY(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> Im(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<int> *pImRGB, *pIm, *pImInt, *pImNE, *pImY;
	custom_buffer<int> prevImRGB(SIZE, 0), prevImNE(SIZE, 0), prevIm(SIZE, 0);

	custom_buffer<s64> ImS_Pos(DL + 1, 0);
	custom_buffer<s64> ImS_fn(DL + 1, 0);
	custom_buffer<s64> mPrevPos(DL*threads, 0);
	vector<concurrency::task<void>> thrsT(threads), thrsForward(DL, concurrency::create_task([] {})), thrs(threads, concurrency::create_task([]{}));
	custom_buffer<int> thrs_resT(threads, 0), thrs_res(threads, 0), thrs_resForward(DL, 0);
	custom_buffer<int> thrs_fn(threads, 0);
	custom_buffer<custom_buffer<int>> thrImRGB(threads, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImRGBForward(DL, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImNEForward(DL, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImYForward(DL, custom_buffer<int>(SIZE, 0));
	custom_buffer<custom_buffer<int>> ImForward(DL, custom_buffer<int>(SIZE, 0));
	custom_buffer<s64> thrPrevPos(threads, 0);
	custom_buffer<s64> PosForward(DL, 0);
	int thr_n, thr_nT, forward_n;
	bool thrs_were_created = false;	

	found_sub = 0;
	prev_pos = -2;
	mPrevPos[0] = CurPos;
	pV->GetRGBImage(mImRGB[0], xmin, xmax, ymin, ymax);
	n_fs = 0;
	n_fs_start = 0;
	prev_n_fs = DL * threads;

	prevPos = -2;

	thr_n = threads;
	forward_n = DL;
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
					forward_n++;
				}
				else if (forward_n < DL)
				{
					mPrevPos[n_fs] = PosForward[forward_n];
					mImRGB[n_fs] = ImRGBForward[forward_n];
					forward_n++;
				}
				else
				{
					if (fn > 0)
					{
						mPrevPos[n_fs] = pos = pV->OneStepWithTimeout();
						pV->GetRGBImage(mImRGB[n_fs], xmin, xmax, ymin, ymax);
					}					
				}

				fn++;
				n_fs++;

				if (n_fs % DL == 0)
				{
					int thr_nT = (n_fs / DL) - 1;
					thrsT[thr_nT] = TaskConvertImage(mImRGB[n_fs - 1], ImT[thr_nT], ImNET[thr_nT], ImYT[thr_nT], w, h, W, H, thrs_resT[thr_nT]);
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

				int _thr_n, _n_fs;
				pos = CurPos = mPrevPos[n_fs];
				for (_thr_n = 0, _n_fs = n_fs; (_thr_n < DL - 1) && (pos < End); _thr_n++)
				{
					if (_n_fs >= (DL*threads))
					{
						PosForward[_thr_n] = pV->OneStepWithTimeout();
						pV->GetRGBImage(ImRGBForward[_thr_n], xmin, xmax, ymin, ymax);
					}
					else
					{
						PosForward[_thr_n] = mPrevPos[_n_fs];
						memcpy(ImRGBForward[_thr_n].m_pData, mImRGB[_n_fs].m_pData, BufferSize);
					}

					pos = PosForward[_thr_n];

					_n_fs++;

					if ((_n_fs > (DL*threads)) || (_n_fs % DL != 0))
					{
						thrsForward[_thr_n] = TaskConvertImage(ImRGBForward[_thr_n], ImForward[_thr_n], ImNEForward[_thr_n], ImYForward[_thr_n], w, h, W, H, thrs_resForward[_thr_n]);
					}
					else
					{
						int _thr_nT = (_n_fs / DL) - 1;
						thrs_resForward[_thr_n] = thrs_resT[_thr_nT];
						memcpy(ImForward[_thr_n].m_pData, ImT[_thr_nT].m_pData, BufferSize);
						memcpy(ImNEForward[_thr_n].m_pData, ImNET[_thr_nT].m_pData, BufferSize);
						memcpy(ImYForward[_thr_n].m_pData, ImY[_thr_nT].m_pData, BufferSize);
					}
				}

				concurrency::when_all(begin(thrsForward), end(thrsForward)).wait();

				if (pos >= End)
				{
					if (_thr_n == 0)
					{
						break;
					}

					while (_thr_n < DL - 1)
					{
						PosForward[_thr_n] = PosForward[_thr_n - 1];
						memcpy(ImRGBForward[_thr_n].m_pData, ImRGBForward[_thr_n - 1].m_pData, BufferSize);
						memcpy(ImForward[_thr_n].m_pData, ImForward[_thr_n - 1].m_pData, BufferSize);
						memcpy(ImNEForward[_thr_n].m_pData, ImNEForward[_thr_n - 1].m_pData, BufferSize);
						memcpy(ImYForward[_thr_n].m_pData, ImYForward[_thr_n - 1].m_pData, BufferSize);
						_thr_n++;
					}
				}
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

		if ((mPrevPos[n_fs_start + DL - 1] >= End) || (g_RunSubSearch == 0) || (mPrevPos[n_fs_start + DL - 1] == mPrevPos[n_fs_start + DL - 2]) || (found_sub == 0))
		{
			break;
		}

		if (debug)
		{
			if (fn >= 20)
			{
				fn = fn;
			}
		}

		if (thr_n % threads == 0)
		{
			thr_n = 0;
			int _thr_n, _n_fs;
			pos = PosForward[DL - 2];
			for (_thr_n = 0, _n_fs = n_fs + DL - 1; (_thr_n < threads) && (pos < End); _thr_n++)
			{
				if (_n_fs >= (DL*threads))
				{
					thrPrevPos[_thr_n] = pV->OneStepWithTimeout();
					pV->GetRGBImage(thrImRGB[_thr_n], xmin, xmax, ymin, ymax);
				}
				else
				{
					thrPrevPos[_thr_n] = mPrevPos[_n_fs];
					memcpy(thrImRGB[_thr_n].m_pData, mImRGB[_n_fs].m_pData, BufferSize);
				}

				pos = thrPrevPos[_thr_n];

				_n_fs++;

				if ((_n_fs > (DL*threads)) || (_n_fs % DL != 0))
				{
					thrs[_thr_n] = TaskConvertImage(thrImRGB[_thr_n], Im[_thr_n], ImNE[_thr_n], ImY[_thr_n], w, h, W, H, thrs_res[_thr_n]);
				}
				else
				{
					int _thr_nT = (_n_fs / DL) - 1;
					thrs_res[_thr_n] = thrs_resT[_thr_nT];
					memcpy(Im[_thr_n].m_pData, ImT[_thr_nT].m_pData, BufferSize);
					memcpy(ImNE[_thr_n].m_pData, ImNET[_thr_nT].m_pData, BufferSize);
					memcpy(ImY[_thr_n].m_pData, ImYT[_thr_nT].m_pData, BufferSize);
				}
			}
		}

		pos = PosForward[DL - 2];

		if (pos < End)
		{
			thrs[thr_n].wait();
			thrs_resForward[DL - 1] = thrs_res[thr_n];
			PosForward[DL - 1] = thrPrevPos[thr_n];
			memcpy(ImRGBForward[DL - 1].m_pData, thrImRGB[thr_n].m_pData, BufferSize);
			memcpy(ImForward[DL - 1].m_pData, Im[thr_n].m_pData, BufferSize);
			memcpy(ImNEForward[DL - 1].m_pData, ImNE[thr_n].m_pData, BufferSize);
			memcpy(ImYForward[DL - 1].m_pData, ImY[thr_n].m_pData, BufferSize);
		}
		else
		{
			thrs_resForward[DL - 1] = thrs_resForward[DL - 2];
			PosForward[DL - 1] = PosForward[DL - 2];
			memcpy(ImRGBForward[DL - 1].m_pData, ImRGBForward[DL - 2].m_pData, BufferSize);
			memcpy(ImForward[DL - 1].m_pData, ImForward[DL - 2].m_pData, BufferSize);
			memcpy(ImNEForward[DL - 1].m_pData, ImNEForward[DL - 2].m_pData, BufferSize);
			memcpy(ImYForward[DL - 1].m_pData, ImYForward[DL - 2].m_pData, BufferSize);
		}

		{
			int _thr_n;
			memcpy(ImInt.m_pData, ImForward[0].m_pData, BufferSize);

			for (_thr_n = 1; _thr_n < DL; _thr_n++)
			{
				IntersectTwoImages(ImInt, ImForward[_thr_n], w, h);
			}
		}

		bln = AnalyseImage(ImInt, w, h);
		pImRGB = &(ImRGBForward[0]);		
		pImNE = &(ImNEForward[0]);
		pImY = &(ImYForward[0]);
		pIm = &(ImForward[0]);
		pImInt = &ImInt;

		if (n_fs == 0)
		{
			prevPos = prev_pos;
		}
		else
		{
			if (n_fs < DL*threads)
			{
				prevPos = mPrevPos[n_fs - 1];
			}
			else
			{
				prevPos = CurPos;
			}
		}

		CurPos = PosForward[0];

		fn++;
		n_fs++;
		thr_n++;

		if (debug)
		{
			SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles_ImCombined_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
			SaveGreyscaleImage(ImForward[0], string("/TestImages/FastSearchSubtitles_ImForwardBegin_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
			SaveGreyscaleImage(*pImNE, string("/TestImages/FastSearchSubtitles_ImNEForwardBegin_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
			SaveGreyscaleImage(*pImY, string("/TestImages/FastSearchSubtitles_ImYForwardBegin_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
			SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles_ImRGBForwardBegin_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);

			if (CurPos >= 172305)
			{
				CurPos = CurPos;
			}

			if (fn >= 28)
			{
				fn = fn;
			}			
		}		

		if (fn > ef)
		{
			if ((bln > 0) && (CurPos != prevPos))
			{
				if (bf == -2)
				{
					bf = fn;
					ef = bf;
					bt = CurPos;

					memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
					memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
					memcpy(ImYS.m_pData, pImY->m_pData, BufferSize);
					memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);

					for (int i = 0; i < w*h; i++)
					{
						ImYS[i] += 255;
					}
				}
				else
				{
					bln = CompareTwoSubs(ImIntS, ImNES, *pImInt, *pImNE, w, h, W, H, ymin);

					if (bln == 0)
					{
						bln = DifficultCompareTwoSubs2(ImIntS, ImNES, *pImInt, *pImNE, w, h, W, H, ymin);
					}

					if (bln == 0)
					{
						bln = CompareTwoSubs(ImIntS, prevImNE, *pImInt, *pImNE, w, h, W, H, ymin);

						if (bln > 0)
						{
							if (debug)
							{
								SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNE_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

								SaveRGBImage(prevImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGBprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(prevImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNEprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							}
						}
					}

					if (bln == 0)
					{
						bln = DifficultCompareTwoSubs2(ImIntS, prevImNE, *pImInt, *pImNE, w, h, W, H, ymin);

						if (bln > 0)
						{
							if (debug)
							{
								SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNE_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

								SaveRGBImage(prevImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGBprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(prevImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNEprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							}
						}
					}

					if ((bln > 0) && (fn - bf + 1 == 3))
					{
						memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
						memcpy(ImYS.m_pData, pImY->m_pData, BufferSize);
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);

						for (int i = 0; i < w*h; i++)
						{
							ImYS[i] += 255;
						}
					}

					if (bln == 0)
					{
						if (debug)
						{
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNE_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(prevImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImRGBprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(prevImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNEprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFprev_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						int new_sub_offset = 0; // DL - 1;							

						if (finded_prev == 1)
						{
							bln = CompareTwoSubs(ImIntSP, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);

							if (bln == 0)
							{
								bln = DifficultCompareTwoSubs2(ImIntSP, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);
							}

							if (bln == 1)
							{
								if (debug)
								{
									SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImYS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									SaveBinaryImage(ImYSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImYSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								}

								IntersectTwoImages(ImYS, ImYSP, w, h);
								IntersectYImages(ImYS, ImYSP, w, h, 0);
								
								if (debug)
								{
									SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImYSIntYSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								}

								pef = fn + new_sub_offset - 1;
								pet = PosForward[new_sub_offset] - 1;
							}
							else
							{
								if (debug)
								{
									SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNES_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

									SaveRGBImage(ImFSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNESP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNESP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								}

								if (AnalizeImageForSubPresence(ImFSP, ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H) == 1)
								{
									Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
									ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
									ImToNativeSize(ImIntSP, w, h, W, H, xmin, xmax, ymin, ymax);
									ImToNativeSize(ImYSP, w, h, W, H, xmin, xmax, ymin, ymax);
									g_pViewImage[0](ImFSP, W, H);
									g_pViewImage[1](ImIntSP, W, H);
									SaveRGBImage(ImFSP, string("/RGBImages/") + Str + g_im_save_format, W, H);
									SaveGreyscaleImage(ImIntSP, string("/ISAImages/") + Str + g_im_save_format, W, H);
									SaveBinaryImage(ImYSP, string("/ILAImages/") + Str + g_im_save_format, W, H);
								}

								pbf = bf;
								pbt = bt;
								pef = fn + new_sub_offset - 1;
								pet = PosForward[new_sub_offset] - 1;
							}
						}
						else
						{
							pbf = bf;
							pbt = bt;
							pef = fn + new_sub_offset - 1;
							pet = PosForward[new_sub_offset] - 1;
						}

						if (pef - pbf + 1 < DL)
						{
							int offset;
							for (offset = 0; offset < DL - 1; offset++)
							{
								bln = CompareTwoSubs(ImIntS, ImNES, ImIntS, ImNEForward[offset], w, h, W, H, ymin);

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, ImNES, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = CompareTwoSubs(ImIntS, prevImNE, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, prevImNE, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									break;
								}

								pef = fn + offset;

								IntersectYImages(ImYS, ImYForward[offset], w, h, 255);

								if (pef - pbf + 1 >= DL)
								{
									break;
								}
							}
						}

						if (pef - pbf + 1 >= DL)
						{
							if (debug)
							{
								SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImIntS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
								SaveGreyscaleImage(prevIm, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImLast_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							}

							IntersectTwoImages(ImIntS, prevIm, w, h);

							if (debug)
							{
								SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImIntSIntImLast_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							}

							memcpy(ImIntSP.m_pData, ImIntS.m_pData, BufferSize);
							memcpy(ImFSP.m_pData, ImFS.m_pData, BufferSize);
							memcpy(ImNESP.m_pData, ImNES.m_pData, BufferSize);
							memcpy(ImYSP.m_pData, ImYS.m_pData, BufferSize);
							finded_prev = 1;
						}
						else
						{
							finded_prev = 0;
						}

						bf = fn;
						ef = bf;
						bt = CurPos;

						memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);
					}
					else
					{
						IntersectYImages(ImYS, *pImY, w, h, 255);
					}
				}
			}
			else if (((bln == 0) && (CurPos != prevPos)) ||
				((bln == 1) && (CurPos == prevPos)))
			{
				if (finded_prev == 1)
				{
					bln = CompareTwoSubs(ImIntSP, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);

					if (bln == 0)
					{
						bln = DifficultCompareTwoSubs2(ImIntSP, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);
					}

					if (bln == 1)
					{
						memcpy(ImFS.m_pData, ImFSP.m_pData, BufferSize);
						bf = pbf;
						ef = bf;
						bt = pbt;
						finded_prev = 0;
					}
					else
					{
						if (debug)
						{
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNES_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

							SaveRGBImage(ImFSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImSP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNESP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImNESP_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						if (AnalizeImageForSubPresence(ImFSP, ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H) == 1)
						{
							Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
							ImToNativeSize(ImFSP, w, h, W, H, xmin, xmax, ymin, ymax);
							ImToNativeSize(ImIntSP, w, h, W, H, xmin, xmax, ymin, ymax);
							ImToNativeSize(ImYSP, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[0](ImFSP, W, H);
							g_pViewImage[1](ImIntSP, W, H);
							SaveRGBImage(ImFSP, string("/RGBImages/") + Str + g_im_save_format, W, H);
							SaveGreyscaleImage(ImIntSP, string("/ISAImages/") + Str + g_im_save_format, W, H);
							SaveBinaryImage(ImYSP, string("/ILAImages/") + Str + g_im_save_format, W, H);
						}
					}
				}

				if (bf != -2)
				{
					if (CurPos != prevPos)
					{
						int offset;
						custom_buffer<int> *pPrevImNE = &prevImNE;
						custom_buffer<int> *pPrevIm = &prevIm;

						if (AnalyseImage(ImForward[DL - 1], w, h) == 0)
						{
							for (offset = 0; offset < DL - 1; offset++)
							{
								bln = CompareTwoSubs(ImIntS, ImNES, ImIntS, ImNEForward[offset], w, h, W, H, ymin);

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, ImNES, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = CompareTwoSubs(ImIntS, *pPrevImNE, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, *pPrevImNE, ImIntS, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									break;
								}

								IntersectYImages(ImYS, ImYForward[offset], w, h, 255);

								pPrevImNE = &(ImNEForward[offset]);
								//pPrevIm = &(ImForward[offset]);
							}
						}
						else
						{
							for (offset = 0; offset < DL - 1; offset++)
							{
								bln = CompareTwoSubs(ImIntS, ImNES, ImForward[offset], ImNEForward[offset], w, h, W, H, ymin);

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, ImNES, ImForward[offset], ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = CompareTwoSubs(ImIntS, *pPrevImNE, ImForward[offset], ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, *pPrevImNE, ImForward[offset], ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									int _thr_n;
									memcpy(ImInt2.m_pData, ImForward[offset].m_pData, BufferSize);

									for (_thr_n = offset + 1; _thr_n < DL - 1; _thr_n++)
									{
										IntersectTwoImages(ImInt2, ImForward[_thr_n], w, h);
									}

									bln = CompareTwoSubs(ImIntS, ImNES, ImInt2, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, ImNES, ImInt2, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = CompareTwoSubs(ImIntS, *pPrevImNE, ImInt2, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									bln = DifficultCompareTwoSubs2(ImIntS, *pPrevImNE, ImInt2, ImNEForward[offset], w, h, W, H, ymin);
								}

								if (bln == 0)
								{
									break;
								}

								IntersectYImages(ImYS, ImYForward[offset], w, h, 255);

								pPrevImNE = &(ImNEForward[offset]);
								//pPrevIm = &(ImForward[offset]);
							}
						}

						if (debug)
						{
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImIntS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pPrevIm, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImLast_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						IntersectTwoImages(ImIntS, *pPrevIm, w, h);

						if (debug)
						{
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImIntSIntImLast_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						ef = fn + offset - 1;
						et = PosForward[offset] - 1; // using 0 instead of offset (there can be intersected sequential subs)
					}
					else
					{
						ef = fn-1;
						et = CurPos;
					}

					if (ef - bf + 1 < DL)
					{
						if (finded_prev == 1)
						{
							bln = CompareTwoSubs(ImIntS, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);

							if (bln == 0)
							{
								bln = DifficultCompareTwoSubs2(ImIntS, ImNESP, ImIntS, ImNES, w, h, W, H, ymin);
							}

							if (bln == 1)
							{
								bf = pbf;
							}
						}
					}

					if (ef - bf + 1 >= DL)
					{
						if (debug)
						{
							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImFS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_ImS_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pImRGB_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_pIm_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
						}

						if (AnalizeImageForSubPresence(ImFS, ImNES, ImIntS, ImYS, bt, bf, w, h, W, H) == 1)
						{
							Str = VideoTimeToStr(bt) + string("__") + VideoTimeToStr(et);
							ImToNativeSize(ImFS, w, h, W, H, xmin, xmax, ymin, ymax);
							ImToNativeSize(ImIntS, w, h, W, H, xmin, xmax, ymin, ymax);
							ImToNativeSize(ImYS, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[0](ImFS, W, H);
							g_pViewImage[1](ImIntS, W, H);
							SaveRGBImage(ImFS, string("/RGBImages/") + Str + g_im_save_format, W, H);
							SaveGreyscaleImage(ImIntS, string("/ISAImages/") + Str + g_im_save_format, W, H);
							SaveBinaryImage(ImYS, string("/ILAImages/") + Str + g_im_save_format, W, H);
						}
					}
				}

				finded_prev = 0;
				bf = -2;
				// ef = -2; shouldn't be erased, required to separate next sub

				if (fn > ef)
				{
					if ((n_fs - n_fs_start) >= DL)
					{
						if (thr_n == threads)
						{
							found_sub = 0;
							prev_pos = prevPos;
							prev_n_fs = n_fs - 1; // note: mPrevPos[nfs-1] == PosForward[0], prev_n_fs = n_fs-1 required for correct new images sequence timming: Next Image Pos should be != Previos Image Pos in sequence
							forward_n = 0;
							thr_n = 0;
							ef = -2;
						}
					}
				}
			}
		}
		
		if (found_sub != 0)
		{
			memcpy(prevImRGB.m_pData, pImRGB->m_pData, BufferSize);
			memcpy(prevImNE.m_pData, pImNE->m_pData, BufferSize);
			memcpy(prevIm.m_pData, pIm->m_pData, BufferSize);

			{
				int _thr_n;
				for (_thr_n = 0; _thr_n < DL - 1; _thr_n++)
				{
					thrs_resForward[_thr_n] = thrs_resForward[_thr_n + 1];
					PosForward[_thr_n] = PosForward[_thr_n + 1];
					memcpy(ImRGBForward[_thr_n].m_pData, ImRGBForward[_thr_n + 1].m_pData, BufferSize);
					memcpy(ImForward[_thr_n].m_pData, ImForward[_thr_n + 1].m_pData, BufferSize);
					memcpy(ImNEForward[_thr_n].m_pData, ImNEForward[_thr_n + 1].m_pData, BufferSize);
					memcpy(ImYForward[_thr_n].m_pData, ImYForward[_thr_n + 1].m_pData, BufferSize);
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
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_01_1_ImFF1" + g_im_save_format, w, h);
			FilterImage(ImFF1, ImNE1, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_01_2_ImFF1F" + g_im_save_format, w, h);
			filter_image(ImFF1);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_01_3_ImFF1FF" + g_im_save_format, w, h);
		},
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_02_1_ImFF2" + g_im_save_format, w, h);
			FilterImage(ImFF2, ImNE2, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_02_2_ImFF2F" + g_im_save_format, w, h);
			filter_image(ImFF2);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_02_3_ImFF2FF" + g_im_save_format, w, h);
		}
	);	

	res = CompareTwoSubs(ImFF1, ImNE1, ImFF2, ImNE2, w, h, W, H, ymin);

	return res;
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
int DifficultCompareTwoSubs(custom_buffer<int> &ImRGB1, custom_buffer<int> &ImF1, custom_buffer<int> &ImRGB2, custom_buffer<int> &ImF2, int w, int h, int W, int H, int ymin)
{
	custom_buffer<int> ImFF1(w*h, 0), ImNE1(w*h, 0), ImY1(w*h, 0);
	custom_buffer<int> ImFF2(w*h, 0), ImNE2(w*h, 0), ImY2(w*h, 0);
	custom_buffer<int> ImTEMP1(w*h, 0), ImTEMP2(w*h, 0), ImTEMP3(w*h, 0);
	custom_buffer<int> lb(h, 0), le(h, 0);
	int res, size, i;

	res = 0;

	size = w*h;

	GetTransformedImage(ImRGB1, ImTEMP1, ImTEMP2, ImFF1, ImNE1, ImY1, w, h, W, H);
	GetTransformedImage(ImRGB2, ImTEMP1, ImTEMP2, ImFF2, ImNE2, ImY2, w, h, W, H);

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
