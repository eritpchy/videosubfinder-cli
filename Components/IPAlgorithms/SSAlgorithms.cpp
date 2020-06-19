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
#include <wx/regex.h>
#include <ppl.h>
#include <ppltasks.h>
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>

#ifdef CUSTOM_TA 
#include "ittnotify.h"

__itt_domain* domain = __itt_domain_create(L"MyTraces.MyDomain");

__itt_string_handle* shOneStep = __itt_string_handle_create(L"OneStep");
__itt_string_handle* shConvertToRGB = __itt_string_handle_create(L"ConvertToRGB");
__itt_string_handle* shGetTransformedImage = __itt_string_handle_create(L"GetTransformedImage");
#endif

int		g_RunSubSearch = 0;

int    g_threads = -1;

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

inline int AnalizeImageForSubPresence(simple_buffer<int> &ImRGB, simple_buffer<int> &ImNE, simple_buffer<int> &ImISA, simple_buffer<int> &ImIL, s64 CurPos, int fn, int w, int h, int W, int H)
{
	int res = 1;

	if (g_use_ISA_images_for_search_subtitles)
	{
		simple_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = h - 1;

		simple_buffer<int> ImFF(ImISA), ImTF(w*h, 0);

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(ImISA, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_01_ImISA_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}
#endif

		if (g_use_ILA_images_for_search_subtitles)
		{
#ifdef CUSTOM_DEBUG
			{
				SaveGreyscaleImage(ImIL, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_02_ImIL_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}
#endif

			IntersectTwoImages(ImFF, ImIL, w, h);

#ifdef CUSTOM_DEBUG
			{
				SaveGreyscaleImage(ImFF, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_03_ImFF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}
#endif
		}
		
		simple_buffer<int> ImSF(ImFF);

		res = FilterTransformedImage(ImFF, ImSF, ImTF, ImNE, LB, LE, 1, w, h, W, H, VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn));

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(ImTF, string("/TestImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_04_ImTF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}
#endif

		if (g_replace_ISA_by_filtered_version)
		{
			memcpy(&(ImISA[0]), &(ImTF[0]), w*h*sizeof(int));
		}
	}

	return res;
}

inline void IntersectYImages(simple_buffer<int> &ImRes, simple_buffer<int> &Im2, int w, int h, int offset_for_im2)
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

int CompareTwoSubsByOffset(simple_buffer<simple_buffer<int>*> &ImForward, simple_buffer<simple_buffer<int>*> &ImYForward, simple_buffer<simple_buffer<int>*> &ImNEForward,
			simple_buffer<int> &ImIntS, simple_buffer<int> &ImYS, simple_buffer<int> &ImNES, simple_buffer<int> &prevImNE,
			int w, int h, int ymin, int W, int H, int offset )
{
	simple_buffer<int> ImInt2(w*h, 0);
	simple_buffer<int> ImYInt2(w*h, 0);
	int bln = 0;
	int BufferSize = w * h * sizeof(int);
	int DL = g_DL;

	if (bln == 0)
	{
		concurrency::parallel_invoke(
			[&ImInt2, &ImForward, DL, BufferSize, offset, w, h] {
				memcpy(ImInt2.m_pData, ImForward[offset]->m_pData, BufferSize);

				for (int _thr_n = offset + 1; _thr_n < DL - 1; _thr_n++)
				{
					IntersectTwoImages(ImInt2, *(ImForward[_thr_n]), w, h);
				}
			},
				[&ImYInt2, &ImYForward, DL, BufferSize, offset, w, h] {
				if (g_use_ILA_images_for_search_subtitles)
				{
					memcpy(ImYInt2.m_pData, ImYForward[offset]->m_pData, BufferSize);

					for (int i = 0; i < w*h; i++)
					{
						ImYInt2[i] += 255;
					}

					for (int _thr_n = offset + 1; _thr_n < DL - 1; _thr_n++)
					{
						IntersectYImages(ImYInt2, *(ImYForward[_thr_n]), w, h, 255);
					}
				}
			}
		);
	}

	bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, prevImNE, ImInt2, &ImYInt2, *(ImNEForward[offset]), w, h, W, H, ymin);	

	return bln;
}

int FindOffsetForNewSub(simple_buffer<simple_buffer<int>*> &ImForward, simple_buffer<simple_buffer<int>*> &ImYForward, simple_buffer<simple_buffer<int>*> &ImNEForward,
	simple_buffer<int> &ImIntS, simple_buffer<int> &ImYS, simple_buffer<int> &ImNES, simple_buffer<int> &prevImNE,
	int w, int h, int ymin, int W, int H)
{	
	int DL = g_DL;
	int offset = 0;

	if (g_threads == 1)
	{
		for (offset = 0; offset < DL - 1; offset++)
		{
			if (CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H, offset) == 0)
			{
				break;
			}
		}
	}
	
	if (g_threads >= 2)
	{
		simple_buffer<int> blns(DL, -1);
		int l = 0, r = (DL - 2)/2;

		while (1)
		{
			if ((l != r) && (blns[l] == -1) && (blns[r] == -1))
			{
				concurrency::parallel_invoke(
					[&blns, &ImForward, &ImYForward, &ImNEForward, &ImIntS, &ImYS, &ImNES, &prevImNE, w, h, ymin, W, H, l] {
						blns[l] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H, l);
					},
					[&blns, &ImForward, &ImYForward, &ImNEForward, &ImIntS, &ImYS, &ImNES, &prevImNE, w, h, ymin, W, H, r] {
						blns[r] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H, r);
					}
				);				
			}
			else
			{
				if (blns[l] == -1)
				{
					blns[l] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H, l);
				}

				if (blns[r] == -1)
				{
					blns[r] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H, r);
				}
			}

			if ((blns[l] == 0) || (l == r))
			{
				break;
			}
			else
			{
				if (blns[r] == 0)
				{
					while(blns[l] != -1) l++;
					if (l > r)
					{
						break;
					}
					r = (l + r) / 2;
				}
				else
				{			
					if (r == DL - 2)
					{
						break;
					}

					l = r;
					while (blns[l] != -1) l++;

					if (r == DL - 3)
					{
						r = DL - 2;
					}
					else
					{
						r = (r + DL - 2) / 2;
					}

					if (l > r)
					{
						break;
					}
				}
			}
		}

		offset = 0;
		while (((blns[offset] > 0) || (blns[offset] == -1)) && (offset < DL - 1)) offset++;
	}

	return offset;
}

inline concurrency::task<void> TaskConvertImage(int fn, my_event &evt_rgb, my_event &evt, simple_buffer<int> &ImRGB, simple_buffer<int> &ImF, simple_buffer<int> &ImNE, simple_buffer<int> &ImY, int &w, int &h, int &W, int &H, int &res)
{
	return concurrency::create_task([fn, &evt_rgb, &evt, &ImRGB, &ImF, &ImNE, &ImY, &w, &h, &W, &H, &res]
	{
		simple_buffer<int> ImFF(w*h, 0), ImSF(w*h, 0);
		evt_rgb.wait();

		custom_set_started(&evt);

		if (!evt_rgb.m_need_to_skip)
		{
#ifdef CUSTOM_TA
			__itt_task_begin(domain, __itt_null, __itt_null, __itt_string_handle_create((std::wstring(L"GetTransformedImage_") + std::to_wstring(fn)).c_str()));
#endif
			res = GetTransformedImage(ImRGB, ImFF, ImSF, ImF, ImNE, ImY, w, h, W, H);
#ifdef CUSTOM_TA 
			__itt_task_end(domain);
#endif
		}
		else
		{
			evt.m_need_to_skip = true;
			res = 0;
		}

		evt.set();
	});
}

class RunSearch
{
	int m_threads;
	int m_w, m_h, m_W, m_H, m_xmin, m_xmax, m_ymin, m_ymax, m_size, m_BufferSize;
	CVideo *m_pV;
	int m_fn_start;
	s64 m_prevPos;
	simple_buffer<s64> m_Pos;
	simple_buffer<s64*> m_pPos;
	custom_buffer<simple_buffer<u8>> m_FrameData;
	simple_buffer<simple_buffer<u8>*> m_pFrameData;
	custom_buffer<simple_buffer<int>> m_ImRGB;	
	simple_buffer<simple_buffer<int>*> m_pImRGB;
	custom_buffer<simple_buffer<int>> m_ImNE;
	simple_buffer<simple_buffer<int>*> m_pImNE;
	custom_buffer<simple_buffer<int>> m_ImY;
	simple_buffer<simple_buffer<int>*> m_pImY;
	custom_buffer<simple_buffer<int>> m_Im;
	simple_buffer<simple_buffer<int>*> m_pIm;

	custom_buffer<simple_buffer<int>> m_ImInt;
	simple_buffer<simple_buffer<int>*> m_pImInt;
	custom_buffer<simple_buffer<int>> m_ImYInt;
	simple_buffer<simple_buffer<int>*> m_pImYInt;

	vector<concurrency::task<void>> m_thrs_one_step;
	vector<concurrency::task<void>> m_thrs_rgb;
	vector<concurrency::task<void>> m_thrs;
	vector<concurrency::task<void>> m_thrs_int;

	vector<my_event> m_events_one_step; // events for one step done
	simple_buffer<my_event*> m_p_events_one_step;

	vector<my_event> m_events_rgb;// events for get rgb image done
	simple_buffer<my_event*> m_p_events_rgb;

	vector<my_event> m_events; // events for convert image done
	simple_buffer<my_event*> m_p_events;

	simple_buffer<int> m_thrs_res;
	simple_buffer<int*> m_pthrs_res;

	simple_buffer<int> m_thrs_int_res;
	simple_buffer<int*> m_pthrs_int_res;

public:
	int m_N;

	RunSearch(int threads, CVideo *pV)
	{
		m_threads = threads;
		m_pV = pV;
		m_w = pV->m_w;
		m_h = pV->m_h;
		m_W = pV->m_Width;
		m_H = pV->m_Height;
		m_xmin = pV->m_xmin;
		m_xmax = pV->m_xmax;
		m_ymin = pV->m_ymin;
		m_ymax = pV->m_ymax;
		m_size = m_w * m_h;
		m_BufferSize = m_size * sizeof(int);
		m_fn_start = 0;
		m_prevPos = -2;

		m_N = std::max<int>(g_DL + threads, (g_DL / 2) * threads);
		m_Pos = simple_buffer<s64>(m_N, -1);
		m_pPos = simple_buffer<s64*>(m_N, NULL);
		m_FrameData = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(pV->GetFrameDataSize(), 0));
		m_pFrameData = simple_buffer<simple_buffer<u8>*>(m_N);
		m_ImRGB = custom_buffer<simple_buffer<int>>(m_N, simple_buffer<int>(m_size, 0));
		m_pImRGB = simple_buffer<simple_buffer<int>*>(m_N);
		m_ImNE = custom_buffer<simple_buffer<int>>(m_N, simple_buffer<int>(m_size, 0));
		m_pImNE = simple_buffer<simple_buffer<int>*>(m_N);
		m_ImY = custom_buffer<simple_buffer<int>>(m_N, simple_buffer<int>(m_size, 0));
		m_pImY = simple_buffer<simple_buffer<int>*>(m_N);
		m_Im = custom_buffer<simple_buffer<int>>(m_N, simple_buffer<int>(m_size, 0));
		m_pIm = simple_buffer<simple_buffer<int>*>(m_N);

		m_ImInt = custom_buffer<simple_buffer<int>>(m_threads, simple_buffer<int>(m_size, 0));
		m_pImInt = simple_buffer<simple_buffer<int>*>(m_threads);
		m_ImYInt = custom_buffer<simple_buffer<int>>(m_threads, simple_buffer<int>(m_size, 0));
		m_pImYInt = simple_buffer<simple_buffer<int>*>(m_threads);

		m_thrs_res = simple_buffer<int>(m_N, -1);
		m_pthrs_res = simple_buffer<int*>(m_N);

		m_events_one_step = vector<my_event>(m_N);
		m_p_events_one_step = simple_buffer<my_event*>(m_N);

		m_events_rgb = vector<my_event>(m_N);
		m_p_events_rgb = simple_buffer<my_event*>(m_N);

		m_events = vector<my_event>(m_N);
		m_p_events = simple_buffer<my_event*>(m_N);

		for (int i = 0; i < m_N; i++)
		{
			m_pPos[i] = &(m_Pos[i]);
			m_pFrameData[i] = &(m_FrameData[i]);
			m_pImRGB[i] = &(m_ImRGB[i]);
			m_pImNE[i] = &(m_ImNE[i]);
			m_pImY[i] = &(m_ImY[i]);
			m_pIm[i] = &(m_Im[i]);
			m_pthrs_res[i] = &(m_thrs_res[i]);
			m_p_events_one_step[i] = &(m_events_one_step[i]);
			m_p_events_rgb[i] = &(m_events_rgb[i]);
			m_p_events[i] = &(m_events[i]);
		}

		m_thrs_int_res = simple_buffer<int>(m_threads, -1);
		m_pthrs_int_res = simple_buffer<int*>(m_threads);

		for (int i = 0; i < m_threads; i++)
		{
			m_pImInt[i] = &(m_ImInt[i]);
			m_pImYInt[i] = &(m_ImYInt[i]);
			m_pthrs_int_res[i] = &(m_thrs_int_res[i]);
		}

		m_thrs_one_step = vector<concurrency::task<void>>(m_N, concurrency::create_task([] {}));
		m_thrs_rgb = vector<concurrency::task<void>>(m_N, concurrency::create_task([] {}));
		m_thrs = vector<concurrency::task<void>>(m_N, concurrency::create_task([] {}));
		m_thrs_int = vector<concurrency::task<void>>(m_threads, concurrency::create_task([] {}));
	}

	~RunSearch()
	{		
		concurrency::when_all(begin(m_thrs_one_step), end(m_thrs_one_step)).wait();
		concurrency::when_all(begin(m_thrs_rgb), end(m_thrs_rgb)).wait();
		concurrency::when_all(begin(m_thrs), end(m_thrs)).wait();
		concurrency::when_all(begin(m_thrs_int), end(m_thrs_int)).wait();
	}

	int AddGetRGBImagesTask(int fn, int num)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N), "AddGetRGBImagesTask: (fdn >= 0) && (fdn < m_N)");
		custom_assert((fdn + num - 1 < m_N), "AddGetRGBImagesTask: (fdn + num - 1 < m_N)");
		custom_assert((num >= 1), "AddGetRGBImagesTask: (num >= 1)");
		
		simple_buffer<simple_buffer<u8>*> pFrameData(m_pFrameData);
		simple_buffer<simple_buffer<int>*> pImRGB(m_pImRGB);
		simple_buffer<s64*> pPos(m_pPos);
		simple_buffer<bool> need_to_get(m_N, false);
		simple_buffer<my_event*> p_events_one_step(m_p_events_one_step);
		simple_buffer<my_event*> p_events_rgb(m_p_events_rgb);
		CVideo *pV = m_pV;
		int xmin = m_xmin;
		int xmax = m_xmax;
		int ymin = m_ymin;
		int ymax = m_ymax;
		int num_to_get = 0;

		for (int i = 0; i < num; i++)
		{
			if (*(pPos[fdn + i]) == -1)
			{
				custom_assert(!m_p_events_rgb[fdn + i]->m_need_to_skip, "AddGetRGBImagesTask: not: !m_p_events_rgb[fdn + i]->m_need_to_skip");
				*(pPos[fdn + i]) = 0;
				need_to_get[fdn + i] = true;
				num_to_get++;
			}
		}

		if (num_to_get > 0)
		{
			int j = fdn;
			for (int i = 0; i < num; i++)
			{
				if (need_to_get[fdn + i])
				{
					j = fdn + i;
					break;
				}
			}

			m_thrs_one_step[j] = concurrency::create_task([fn, fdn, num, pPos, pFrameData, pV, p_events_one_step, need_to_get]() mutable
			{
				for (int i = 0; i < num; i++)
				{
					if (need_to_get[fdn + i])
					{
						if ((fdn + i) > 0)
						{
							p_events_one_step[fdn + i - 1]->wait();
						}

#ifdef CUSTOM_TA 
						__itt_task_begin(domain, __itt_null, __itt_null, shOneStep);
#endif
						custom_set_started(p_events_one_step[fdn + i]);
						*(pPos[fdn + i]) = pV->OneStepWithTimeout();						
						pV->GetFrameData(*(pFrameData[fdn + i]));
#ifdef CUSTOM_TA
						__itt_task_end(domain);
#endif

						p_events_one_step[fdn + i]->set();

					}
				}
			});

			m_thrs_rgb[j] = concurrency::create_task([fn, fdn, num, pFrameData, pImRGB, pV, xmin, xmax, ymin, ymax, p_events_one_step, p_events_rgb, need_to_get]() mutable
				{
					for (int i = 0; i < num; i++)
					{
						if (need_to_get[fdn + i])
						{
							custom_set_started(p_events_rgb[fdn + i]);

							if (!p_events_rgb[fdn + i]->m_need_to_skip)
							{
								p_events_one_step[fdn + i]->wait();

								if (!p_events_rgb[fdn + i]->m_need_to_skip)
								{									
#ifdef CUSTOM_TA
									__itt_id task_id = { (unsigned long long)fn + i, 0, 0 };
									__itt_task_begin(domain, __itt_null, __itt_null, shConvertToRGB);
#endif

									pV->ConvertToRGB(&((*(pFrameData[fdn + i]))[0]), *(pImRGB[fdn + i]), xmin, xmax, ymin, ymax);
#ifdef CUSTOM_TA 
									__itt_task_end(domain);
#endif
								}
#ifdef CUSTOM_DEBUG2
								else
								{
									p_events_rgb[fdn + i]->m_need_to_skip = p_events_rgb[fdn + i]->m_need_to_skip;
								}
#endif
							}
#ifdef CUSTOM_DEBUG2
							else
							{
								p_events_rgb[fdn + i]->m_need_to_skip = p_events_rgb[fdn + i]->m_need_to_skip;
							}

							//SaveRGBImage(*(pImRGB[fdn + i]), string("/TestImages/AddGetRGBImagesTask_ImRGB") + "_fn" + std::to_string(fn + i) + g_im_save_format, xmax - xmin + 1, ymax - ymin + 1);
#endif							

							p_events_rgb[fdn + i]->set();
						}
					}
				});
		}

		return (fn + num - 1);
	}

	void AddIntersectImagesTask(int fn)
	{
		int fdn = fn - m_fn_start;
		int fn_start = m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_threads), "AddIntersectImagesTask: (fdn >= 0) && (fdn < m_threads)");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "AddIntersectImagesTask: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		int* pthrs_int_res = m_pthrs_int_res[fdn];

		if (*pthrs_int_res == -1)
		{
			*pthrs_int_res = 0;

			simple_buffer<simple_buffer<int>*> pIm(m_pIm);
			simple_buffer<simple_buffer<int>*> pImY(m_pImY);
			simple_buffer<my_event*> p_events(m_p_events);
			simple_buffer<int*> pthrs_res(m_pthrs_res);
			simple_buffer<int>* pImInt = m_pImInt[fdn];
			simple_buffer<int>* pImYInt = m_pImYInt[fdn];			
			int threads = m_threads;
			int BufferSize = m_BufferSize;
			int w = m_w;
			int h = m_h;
			int DL = g_DL;

			m_thrs_int[fdn] = concurrency::create_task([pthrs_int_res, fn, fn_start, fdn, pIm, pImY, p_events, pthrs_res, pImInt, pImYInt, threads, BufferSize, w, h, DL]() mutable
			{
				int bln = 1;
				bool need_to_skip = false;

				for (int i = 0; i < DL; i++)
				{
					p_events[fdn + i]->wait();
					if (!p_events[fdn + i]->m_need_to_skip)
					{
						bln = bln & (*(pthrs_res[fdn + i]));						
					}
					else
					{
						//need to skip
						bln = 0;
						need_to_skip = true;
					}

					if (bln == 0)
					{
						break;
					}
				}

				if (bln)
				{
					concurrency::parallel_invoke(
						[&pImInt, &pIm, fdn, DL, BufferSize, w, h] {
						memcpy(pImInt->m_pData, pIm[fdn]->m_pData, BufferSize);

						for (int _thr_n = 1; _thr_n < DL; _thr_n++)
						{
							IntersectTwoImages(*(pImInt), *(pIm[fdn + _thr_n]), w, h);
						}
					},
						[&pImYInt, &pImY, fdn, DL, BufferSize, w, h] {
						if (g_use_ILA_images_for_search_subtitles)
						{
							memcpy(pImYInt->m_pData, pImY[fdn]->m_pData, BufferSize);

							for (int i = 0; i < w*h; i++)
							{
								(*pImYInt)[i] += 255;
							}

							for (int _thr_n = 1; _thr_n < DL; _thr_n++)
							{
								IntersectYImages(*(pImYInt), *(pImY[fdn + _thr_n]), w, h, 255);
							}
						}
					}
					);

#ifdef CUSTOM_DEBUG
					{
						if (fn == 54)
						{
							SaveGreyscaleImage(*pImInt, string("/TestImages/AddIntersectImagesTask_ImCombined_") + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
							SaveBinaryImage(*pImYInt, string("/TestImages/AddIntersectImagesTask_ImYInt_") + "_fn" + std::to_string(fn) + g_im_save_format, w, h);

							for (int i = 0; i < DL; i++)
							{
								SaveGreyscaleImage(*(pIm[fdn + i]), string("/TestImages/AddIntersectImagesTask_Im_") + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
								SaveGreyscaleImage(*(pImY[fdn + i]), string("/TestImages/AddIntersectImagesTask_ImY_") + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
							}

							fn = fn;
						}
					}
#endif

					bln = AnalyseImage(*pImInt, pImYInt, w, h);					
				}
				else if (!need_to_skip)
				{
					memcpy(pImInt->m_pData, pIm[fdn]->m_pData, BufferSize);
					memcpy(pImYInt->m_pData, pImY[fdn]->m_pData, BufferSize);
					for (int i = 0; i < w*h; i++)
					{
						(*pImYInt)[i] += 255;
					}
				}

				*pthrs_int_res = bln;
			});
		}
	}

	int GetIntersectImages(int fn, simple_buffer<int> *&pImInt, simple_buffer<int> *&pImYInt)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_threads), "GetIntersectImages: not: (fdn >= 0) && (fdn < m_threads)");		
		custom_assert(*(m_pthrs_int_res[fdn]) != -1, "GetIntersectImages: not: *(m_pthrs_int_res[fdn]) != -1");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "GetIntersectImages: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		m_thrs_int[fdn].wait();

		pImInt = m_pImInt[fdn];
		pImYInt = m_pImYInt[fdn];

		return *(m_pthrs_int_res[fdn]);
	}

	void AddConvertImageTask(int fn)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N), "AddConvertImageTask: (fdn >= 0) && (fdn < m_N)");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "AddConvertImageTask: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		if (*(m_pthrs_res[fdn]) == -1)
		{
			*(m_pthrs_res[fdn]) = 0;
			m_thrs[fdn] = TaskConvertImage(fn, *(m_p_events_rgb[fdn]), *(m_p_events[fdn]), *(m_pImRGB[fdn]), *(m_pIm[fdn]), *(m_pImNE[fdn]), *(m_pImY[fdn]), m_w, m_h, m_W, m_H, *(m_pthrs_res[fdn]));
		}
	}

	int GetConvertImageCopy(int fn, simple_buffer<int> *pImRGB = NULL, simple_buffer<int> *pIm = NULL, simple_buffer<int> *pImNE = NULL, simple_buffer<int> *pImY = NULL)
	{
		int fdn = fn - m_fn_start;
		
		custom_assert((fdn >= 0) && (fdn < m_N), "GetConvertImageCopy: not: (fdn >= 0) && (fdn < m_N)");
		custom_assert(*(m_pthrs_res[fdn]) != -1, "GetConvertImageCopy: not: *(m_pthrs_res[fdn]) != -1");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "GetConvertImageCopy: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		m_thrs[fdn].wait();
		
		if (pImRGB != NULL)
		{
			memcpy(pImRGB->m_pData, m_pImRGB[fdn]->m_pData, m_BufferSize);
		}
		if (pIm != NULL)
		{
			memcpy(pIm->m_pData, m_pIm[fdn]->m_pData, m_BufferSize);
		}
		if (pImNE != NULL)
		{
			memcpy(pImNE->m_pData, m_pImNE[fdn]->m_pData, m_BufferSize);
		}
		if (pImY != NULL)
		{
			memcpy(pImY->m_pData, m_pImY[fdn]->m_pData, m_BufferSize);
		}

		return *(m_pthrs_res[fdn]);
	}

	int GetConvertImage(int fn, simple_buffer<int> *&pImRGB, simple_buffer<int> *&pIm, simple_buffer<int> *&pImNE, simple_buffer<int> *&pImY, s64 &pos)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N), "GetConvertImage: not: (fdn >= 0) && (fdn < m_N)");
		custom_assert(*(m_pthrs_res[fdn]) != -1, "GetConvertImage: not: m_pthrs_res[fdn] != -1");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "GetConvertImage: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		m_thrs[fdn].wait();

		pImRGB = m_pImRGB[fdn];
		pIm = m_pIm[fdn];
		pImNE = m_pImNE[fdn];
		pImY = m_pImY[fdn];
		pos = *(m_pPos[fdn]);

		return *(m_pthrs_res[fdn]);
	}

	s64 GetPos(int fn)
	{
		int fdn = fn - m_fn_start;		

		custom_assert((fdn >= -1) && (fdn < m_N), "GetPos: (fdn >= -1) && (fdn < m_N)");

		int pos;

		if (fdn >= 0)
		{
			m_p_events_rgb[fdn]->wait();
			pos = *(m_pPos[fdn]);
		}
		else
		{
			pos = m_prevPos;
		}

		return pos;
	}

	void ShiftStartFrameNumberTo(int fn)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N-1), "ShiftStartFrameNumberTo: (fdn >= 0) && (fdn < m_N-1)");

		if (fdn > 0)
		{
			for (int i = fdn - 1; i >= 0; i--)
			{
				m_p_events_rgb[i]->m_need_to_skip = true;
			}			
			
			// NOTE: there can be dependent thread from AddGetRGBImagesTask which wait for m_p_events_one_step[fdn - 1] finish
			// if we will reset/change m_p_events_one_step[fdn - 1] states, m_p_events_one_step[fdn] will can wait wrong event
			// so waiting for m_p_events_one_step[fdn] will finish
			// in case of threads == 2 it can be not scheduled for do one step on fdn
			if (*(m_pPos[fdn]) == 0)
			{
				m_p_events_one_step[fdn]->wait();
			}

			for (int i = 0; i < fdn; i++)
			{
				m_p_events_rgb[i]->wait();
			}

			concurrency::when_all(begin(m_thrs), next(begin(m_thrs),fdn)).wait();
			concurrency::when_all(begin(m_thrs_int), next(begin(m_thrs_int), std::min<int>(fdn, m_threads))).wait();

			simple_buffer<simple_buffer<u8>*>  pFrameData(m_pFrameData);
			simple_buffer<simple_buffer<int>*> pImRGB(m_pImRGB);
			simple_buffer<simple_buffer<int>*> pImNE(m_pImNE);
			simple_buffer<simple_buffer<int>*> pImY(m_pImY);
			simple_buffer<simple_buffer<int>*> pIm(m_pIm);
			simple_buffer<s64*> pPos(m_pPos);
			simple_buffer<int*> pthrs_res(m_pthrs_res);
			simple_buffer<my_event*> p_events_one_step(m_p_events_one_step);
			simple_buffer<my_event*> p_events_rgb(m_p_events_rgb);
			simple_buffer<my_event*> p_events(m_p_events);
			int i, j;

			m_prevPos = *(m_pPos[fdn - 1]);

			for (i = 0; i < (m_N - fdn); i++)
			{
				m_pFrameData[i] = pFrameData[i + fdn];
				m_pImRGB[i] = pImRGB[i + fdn];
				m_pImNE[i] = pImNE[i + fdn];
				m_pImY[i] = pImY[i + fdn];
				m_pIm[i] = pIm[i + fdn];
				m_pPos[i] = pPos[i + fdn];
				m_thrs[i] = m_thrs[i + fdn];
				m_pthrs_res[i] = pthrs_res[i + fdn];
				m_p_events_one_step[i] = p_events_one_step[i + fdn];
				m_p_events_rgb[i] = p_events_rgb[i + fdn];
				m_p_events[i] = p_events[i + fdn];
				m_thrs_one_step[i] = m_thrs_one_step[i + fdn];				
				m_thrs_rgb[i] = m_thrs_rgb[i + fdn];
			}			

			for (i = m_N - fdn, j=0; i < m_N; i++, j++)
			{
				m_pFrameData[i] = pFrameData[j];
				m_pImRGB[i] = pImRGB[j];
				m_pImNE[i] = pImNE[j];
				m_pImY[i] = pImY[j];
				m_pIm[i] = pIm[j];
				m_pthrs_res[i] = pthrs_res[j];
				m_pPos[i] = pPos[j];

				m_p_events_one_step[i] = p_events_one_step[j];
				(m_p_events_one_step[i])->reset();

				m_p_events_rgb[i] = p_events_rgb[j];
				(m_p_events_rgb[i])->reset();

				m_p_events[i] = p_events[j];
				(m_p_events[i])->reset();
				*(m_pPos[i]) = -1;
				*(m_pthrs_res[i]) = -1;
			}

			if (fdn < m_threads)
			{
				simple_buffer<simple_buffer<int>*> pImInt(m_pImInt);
				simple_buffer<simple_buffer<int>*> pImYInt(m_pImYInt);
				simple_buffer<int*> pthrs_int_res(m_pthrs_int_res);

				for (i = 0; i < (m_threads - fdn); i++)
				{
					m_pImInt[i] = pImInt[i + fdn];
					m_pImYInt[i] = pImYInt[i + fdn];
					m_thrs_int[i] = m_thrs_int[i + fdn];
					m_pthrs_int_res[i] = pthrs_int_res[i + fdn];
				}

				for (i = m_threads - fdn, j = 0; i < m_threads; i++, j++)
				{
					m_pImInt[i] = pImInt[j];
					m_pImYInt[i] = pImYInt[j];
					m_pthrs_int_res[i] = pthrs_int_res[j];
					*(m_pthrs_int_res[i]) = -1;
				}
			}
			else
			{
				for (i = 0; i < m_threads; i++)
				{
					*(m_pthrs_int_res[i]) = -1;
				}
			}

			m_fn_start = fn;
		}
	}
};

s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End)
{
	//NOTE: FastSearchSubtitles doesn't use cv:: at all
	// in case of x86 cv::ocl::setUseOpenCL(g_use_ocl) breack ffmpeg cuda usage
	//cv::ocl::setUseOpenCL(g_use_ocl);

	// Create a scheduler policy that allows up to 1000
	// simultaneous tasks.
	concurrency::SchedulerPolicy policy(2, concurrency::MaxConcurrency, 1000, concurrency::ContextPriority, THREAD_PRIORITY_HIGHEST);

	// Attach the policy to the current scheduler.
	concurrency::CurrentScheduler::Create(policy);

	string Str;
	s64 CurPos, prevPos;
	int fn; //cur frame num
	int max_rgb_fn; //max rgb frame num which will be obtained
	int w, h, W, H, xmin, xmax, ymin, ymax, size, BufferSize;
	int DL, threads;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;

	int found_sub;

	int bln = 0, bln1 = 0, bln2 = 0, finded_prev;

	//g_disable_save_images = true;

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

#ifdef CUSTOM_DEBUG
	{		
		/*ymin = g_pV->m_ymin = 380;
		ymax = g_pV->m_ymax = 449;
		h = g_pV->m_h = ymax - ymin + 1;*/
	}
#endif

	size = w * h;
	BufferSize = size * sizeof(int);

	pV->SetPos(Begin);
	CurPos = pV->GetPos();

	DL = g_DL;

	if (g_threads <= 0)
	{
		g_threads = std::thread::hardware_concurrency();
	}
	if (g_threads < 2)
	{
		g_threads = 2;
	}

	threads = g_threads;

	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	simple_buffer<int> ImInt(size, 0), ImYInt(size, 0);
	simple_buffer<int> ImIntS(size, 0); //store image
	simple_buffer<int> ImIntSP(size, 0); //store image prev
	simple_buffer<int> ImFS(size, 0); //image for save
	simple_buffer<int> ImFSP(size, 0); //image for save prev
	simple_buffer<int> ImNES(size, 0), ImNESP(size, 0);
	simple_buffer<int> ImYS(size, 0), ImYSP(size, 0);
	simple_buffer<int> *pImRGB, *pIm, *pImInt, *pImYInt, *pImNE, *pImY;
	simple_buffer<int> prevImRGB(size, 0), prevImNE(size, 0), prevIm(size, 0);

	simple_buffer<simple_buffer<int>*> ImRGBForward(DL, NULL);
	simple_buffer<simple_buffer<int>*> ImNEForward(DL, NULL);
	simple_buffer<simple_buffer<int>*> ImYForward(DL, NULL);
	simple_buffer<simple_buffer<int>*> ImForward(DL, NULL);
	simple_buffer<s64> PosForward(DL, 0);

	found_sub = 0;

	prevPos = -2;

	RunSearch rs(threads, pV);	
	
	const int ddl = (DL / 2);
	const int ddl1_ofset = ddl - 1;
	const int ddl2_ofset = (ddl * 2) - 1;

	int fn_start;

	/*DWORD start_time = GetTickCount();
	int num = 18000;
	for (int i = 0; i < num; i++)
	{
		CurPos = pV->OneStepWithTimeout();
		pV->GetRGBImage(ImInt, xmin, xmax, ymin, ymax);
	}
	DWORD dt = GetTickCount() - start_time;

	char msg[500];
	sprintf(msg, "dt: %d CurPos: %d", (int)dt, (int)CurPos);
	wxMessageBox(msg);*/
	
	
	/*fn = 0;
	while (CurPos < End)
	{
		CurPos = pV->OneStepWithTimeout();
		pV->GetRGBImage(ImInt, xmin, xmax, ymin, ymax);		
	}
	return 0;*/

	fn_start = fn = 0;
	max_rgb_fn = -1;

	while ((CurPos < End) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{
		int create_new_threads = threads;		

		while ((found_sub == 0) && (CurPos < End) && (CurPos != prevPos) && (g_RunSubSearch == 1))
		{
			prevPos = CurPos;

#ifdef CUSTOM_TA
			if (fn_start >= 12)
			{
				CurPos = End;
				break;
			}
#endif

			for (int thr_n = 0; thr_n < create_new_threads; thr_n++)
			{
				if (max_rgb_fn < fn + ddl - 1)
				{
					max_rgb_fn = max<int>(max_rgb_fn, rs.AddGetRGBImagesTask(fn, ddl - 1));
					max_rgb_fn = max<int>(max_rgb_fn, rs.AddGetRGBImagesTask(fn + ddl - 1, 1));
				}
				rs.AddConvertImageTask(fn + ddl - 1);
				fn += ddl;
			}

			bln = 0;

			bln1 = rs.GetConvertImageCopy(fn_start + ddl1_ofset);			
			CurPos = rs.GetPos(fn_start + ddl1_ofset);

			if (bln1)
			{
				bln2 = rs.GetConvertImageCopy(fn_start + ddl2_ofset);
				CurPos = rs.GetPos(fn_start + ddl2_ofset);

#ifdef CUSTOM_DEBUG
				if (CurPos >= 98000)
				{
					CurPos = CurPos;
				}
#endif
				
				if (bln2)
				{					
					rs.GetConvertImage(fn_start + ddl2_ofset, ImRGBForward[1], ImForward[1], ImNEForward[1], ImYForward[1], PosForward[1]);

					concurrency::parallel_invoke(
						[&rs , &ImInt, &ImForward, fn_start, ddl1_ofset, w, h] {
							rs.GetConvertImageCopy(fn_start + ddl1_ofset, NULL, &ImInt, NULL, NULL);
							IntersectTwoImages(ImInt, *(ImForward[1]), w, h);
						},
						[&rs, &ImYInt, &ImYForward, fn_start, ddl1_ofset, w, h] {
							rs.GetConvertImageCopy(fn_start + ddl1_ofset, NULL, NULL, NULL, &ImYInt);
							if (g_use_ILA_images_for_search_subtitles)
							{
								for (int i = 0; i < w * h; i++)
								{
									ImYInt[i] += 255;
								}
								IntersectYImages(ImYInt, *(ImYForward[1]), w, h, 255);
							}
						}
					);

					bln = AnalyseImage(ImInt, &ImYInt, w, h);

					if (bln)
					{
						for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
						{
							rs.AddConvertImageTask(fn_start + i);
						}

						bln = 1;
						for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
						{
							bln = bln && rs.GetConvertImage(fn_start + i, ImRGBForward[i], ImForward[i], ImNEForward[i], ImYForward[i], PosForward[i]);
						}

						if (bln)
						{
							concurrency::parallel_invoke(
								[&ImInt, &ImForward, ddl1_ofset, ddl2_ofset, w, h] {
									for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
									{
										IntersectTwoImages(ImInt, *(ImForward[i]), w, h);
									}
								},
								[&ImYInt, &ImYForward, ddl1_ofset, ddl2_ofset, w, h] {
									if (g_use_ILA_images_for_search_subtitles)
									{
										for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
										{
											IntersectYImages(ImYInt, *(ImYForward[i]), w, h, 255);
										}
									}
								});

							bln = AnalyseImage(ImInt, &ImYInt, w, h);
						}
					}
				}
								
				if (bln)
				{
					found_sub = 1;
					fn = fn_start;

					for (int i = 0; i < (DL + threads - 1); i++)
					{
						if (max_rgb_fn < fn + i)
						{
							max_rgb_fn = max<int>(max_rgb_fn, rs.AddGetRGBImagesTask(fn + i, 1));
						}
						rs.AddConvertImageTask(fn + i);
					}

					for (int i = 0; i < (threads - 1); i++)
					{
						rs.AddIntersectImagesTask(fn + i);
					}
				}
				else
				{
					if (bln2)
					{
						fn_start += ddl;
						rs.ShiftStartFrameNumberTo(fn_start);
						create_new_threads = 1;
					}
					else
					{
						fn_start += (ddl * 2);
						rs.ShiftStartFrameNumberTo(fn_start);
						create_new_threads = 2;
					}
				}
			}
			else
			{

				fn_start += ddl;
				rs.ShiftStartFrameNumberTo(fn_start);
				create_new_threads = 1;
			}			
		}		

		if ((g_RunSubSearch == 0) || (found_sub == 0) || (CurPos >= End) || (CurPos == prevPos))
		{
			break;
		}

#ifdef CUSTOM_DEBUG
		{
			if (fn >= 20)
			{
				fn = fn;
			}
		}
#endif		

		if (max_rgb_fn < fn + DL + threads - 1)
		{
			max_rgb_fn = max<int>(max_rgb_fn, rs.AddGetRGBImagesTask(fn + DL + threads - 1, 1));
		}

		rs.AddConvertImageTask(fn + DL + threads - 1);
		rs.AddIntersectImagesTask(fn + threads - 1);

		bln = rs.GetIntersectImages(fn, pImInt, pImYInt);

		for (int i = 0; i < DL; i++)
		{
			rs.GetConvertImage(fn + i, ImRGBForward[i], ImForward[i], ImNEForward[i], ImYForward[i], PosForward[i]);
		}

		pImRGB = ImRGBForward[0];		
		pImNE = ImNEForward[0];
		pImY = ImYForward[0];
		pIm = ImForward[0];

		prevPos = rs.GetPos(fn - 1);
		CurPos = PosForward[0];		

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles_ImCombined_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
			SaveBinaryImage(*pImYInt, string("/TestImages/FastSearchSubtitles_ImYInt_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + g_im_save_format, w, h);

			for (int i = ((fn == fn_start) ? 0 : DL - 1); i < DL; i++)
			{					
				SaveGreyscaleImage(*(ImForward[i]), string("/TestImages/FastSearchSubtitles_ImForwardBegin_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
				SaveGreyscaleImage(*(ImNEForward[i]), string("/TestImages/FastSearchSubtitles_ImNEForwardBegin_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
				SaveGreyscaleImage(*(ImYForward[i]), string("/TestImages/FastSearchSubtitles_ImYForwardBegin_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
				SaveRGBImage(*(ImRGBForward[i]), string("/TestImages/FastSearchSubtitles_ImRGBForwardBegin_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
			}
			
			if (CurPos >= 98000)
			{
				CurPos = CurPos;
			}

			if (fn >= 184)
			{
				fn = fn;
			}			
		}		
#endif

		if (fn == bf)
		{
			memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
			memcpy(ImYS.m_pData, pImYInt->m_pData, BufferSize);
			
#ifdef CUSTOM_DEBUG
			{
				SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
				SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
			}
#endif
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
					memcpy(ImYS.m_pData, pImYInt->m_pData, BufferSize);
					memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);
					
#ifdef CUSTOM_DEBUG
					{
						SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
						SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
						SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
						SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						bt = bt;
					}
#endif
				}
				else
				{
					bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, prevImNE, *pImInt, pImYInt, *pImNE, w, h, W, H, ymin);
					
					if ((bln > 0) && ((fn - bf + 1 == 3)||(fn - bf + 1 == DL)))
					{
						memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
						memcpy(ImNES.m_pData, pImNE->m_pData, BufferSize);
						memcpy(ImYS.m_pData, pImYInt->m_pData, BufferSize);
						memcpy(ImFS.m_pData, pImRGB->m_pData, BufferSize);

#ifdef CUSTOM_DEBUG
						{
							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif
					}

					if (bln == 0)
					{
#ifdef CUSTOM_DEBUG
						{
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImRGB" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNE" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImInt" + g_im_save_format, w, h);
							SaveBinaryImage(*pImYInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYInt" + g_im_save_format, w, h);

							SaveRGBImage(prevImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImRGBprev" + g_im_save_format, w, h);
							SaveGreyscaleImage(prevImNE, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNEprev" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif

						if (finded_prev == 1)
						{
							bln = CompareTwoSubsOptimal(ImIntSP, &ImYSP, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, ymin);
							
							if (bln == 0)
							{														
#ifdef CUSTOM_DEBUG
								{
									SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImS" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);

									SaveRGBImage(ImFSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNESP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
								}
#endif

								if (AnalizeImageForSubPresence(ImFSP, ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H) == 1)
								{
									Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
									
									simple_buffer<int> ImTMP(W*H, 0);

									ImToNativeSize(ImFSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
									g_pViewImage[0](ImTMP, W, H);
									SaveRGBImage(ImTMP, string("/RGBImages/") + Str + g_im_save_format, W, H);

									ImToNativeSize(ImIntSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
									g_pViewImage[1](ImTMP, W, H);
									SaveGreyscaleImage(ImTMP, string("/ISAImages/") + Str + g_im_save_format, W, H);

									ImToNativeSize(ImYSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
									SaveBinaryImage(ImTMP, string("/ILAImages/") + Str + g_im_save_format, W, H);
								}

								pbf = bf;
								pbt = bt;
							}
						}
						else
						{
							pbf = bf;
							pbt = bt;							
						}

						pef = fn - 1;
						pet = PosForward[0] - 1;

						int offset = 0;
						
						if (fn > bf + 1)
						{
							offset = FindOffsetForNewSub(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H);

							pef = fn + offset - 1;
							pet = PosForward[offset] - 1;
						}

						if (pef - pbf + 1 >= DL)
						{
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

						bf = fn + offset;
						ef = bf;
						bt = PosForward[offset];
						
						memcpy(ImNES.m_pData, ImNEForward[offset]->m_pData, BufferSize);						
						memcpy(ImFS.m_pData, ImRGBForward[offset]->m_pData, BufferSize);

						if (offset == 0)
						{
							memcpy(ImIntS.m_pData, pImInt->m_pData, BufferSize);
							memcpy(ImYS.m_pData, pImYInt->m_pData, BufferSize);
						}
						else
						{
							memcpy(ImIntS.m_pData, ImForward[offset]->m_pData, BufferSize);
							memcpy(ImYS.m_pData, ImYForward[offset]->m_pData, BufferSize);
							for (int i = 0; i < w*h; i++)
							{
								ImYS[i] += 255;
							}
						}

#ifdef CUSTOM_DEBUG
						{
							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif
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
					bln = CompareTwoSubsOptimal(ImIntSP, &ImYSP, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, ymin);

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
#ifdef CUSTOM_DEBUG
						{
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pImRGB" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pIm" + g_im_save_format, w, h);

							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);

							SaveRGBImage(ImFSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntSP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImSP" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNESP, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
						}
#endif

						if (AnalizeImageForSubPresence(ImFSP, ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H) == 1)
						{
							Str = VideoTimeToStr(pbt) + string("__") + VideoTimeToStr(pet);
							
							simple_buffer<int> ImTMP(W*H, 0);

							ImToNativeSize(ImFSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[0](ImTMP, W, H);
							SaveRGBImage(ImTMP, string("/RGBImages/") + Str + g_im_save_format, W, H);

							ImToNativeSize(ImIntSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[1](ImTMP, W, H);
							SaveGreyscaleImage(ImTMP, string("/ISAImages/") + Str + g_im_save_format, W, H);

							ImToNativeSize(ImYSP, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							SaveBinaryImage(ImTMP, string("/ILAImages/") + Str + g_im_save_format, W, H);
						}
					}
				}

				if (bf != -2)
				{
					if (CurPos != prevPos)
					{
						int offset = 0;
						simple_buffer<int> *pPrevImNE = &prevImNE;
						simple_buffer<int> *pPrevIm = &prevIm;

						if (AnalyseImage(*(ImForward[DL - 1]), NULL, w, h) == 0)
						{
							for (offset = 0; offset < DL - 1; offset++)
							{
								bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, *pPrevImNE, ImIntS, &ImYS, *(ImNEForward[offset]), w, h, W, H, ymin);

								if (bln == 0)
								{
									break;
								}

								// note: don't intersect fist DL and last DL frames in ImIL

								pPrevImNE = ImNEForward[offset];
							}
						}
						else
						{			
							offset = FindOffsetForNewSub(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, ymin, W, H);

							if (offset > 0)
							{
								pPrevImNE = ImNEForward[offset - 1];
							}
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
							bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, ymin);

							if (bln == 1)
							{
								bf = pbf;
							}
						}
					}

					if (ef - bf + 1 >= DL)
					{
#ifdef CUSTOM_DEBUG
						{
							SaveRGBImage(ImFS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImYS, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
							SaveRGBImage(*pImRGB, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pImRGB" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, string("/TestImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pIm" + g_im_save_format, w, h);
						}
#endif

						if (AnalizeImageForSubPresence(ImFS, ImNES, ImIntS, ImYS, bt, bf, w, h, W, H) == 1)
						{
							Str = VideoTimeToStr(bt) + string("__") + VideoTimeToStr(et);

							simple_buffer<int> ImTMP(W*H, 0);

							ImToNativeSize(ImFS, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[0](ImTMP, W, H);
							SaveRGBImage(ImTMP, string("/RGBImages/") + Str + g_im_save_format, W, H);

							ImToNativeSize(ImIntS, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							g_pViewImage[1](ImTMP, W, H);
							SaveGreyscaleImage(ImTMP, string("/ISAImages/") + Str + g_im_save_format, W, H);

							ImToNativeSize(ImYS, ImTMP, w, h, W, H, xmin, xmax, ymin, ymax);
							SaveBinaryImage(ImTMP, string("/ILAImages/") + Str + g_im_save_format, W, H);							
						}
					}
				}

				finded_prev = 0;
				bf = -2;

				if (fn > ef)
				{
					if ((fn - fn_start) >= DL)
					{
						found_sub = 0;
						fn_start = fn;
						// NOTE: not needed, aready done on fn before
						// by code under: if (found_sub != 0)
						// rs.ShiftStartFrameNumberTo(fn_start);
					}
				}
			}
		}
		
		if (found_sub != 0)
		{
			memcpy(prevImRGB.m_pData, pImRGB->m_pData, BufferSize);
			memcpy(prevImNE.m_pData, pImNE->m_pData, BufferSize);
			memcpy(prevIm.m_pData, pIm->m_pData, BufferSize);

			fn++;
			rs.ShiftStartFrameNumberTo(fn);
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

int AnalyseImage(simple_buffer<int> &Im, simple_buffer<int> *pImILA, int w, int h)
{
	int i, k, l, x, y, ia, da, pl, mpl, i_mpl, len, len2, val1, val2, n, bln;
	int segh, mtl;	
	double tp;
	simple_buffer<int> ImRes(Im);
	
	if ((g_use_ILA_images_for_search_subtitles) && (pImILA != NULL))
	{
		IntersectTwoImages(ImRes, *pImILA, w, h);
	}

	segh = g_segh;
	tp = g_tp;
	mtl = (int)(g_mtpl*(double)w);
	
	n = h/segh;
	da = w*segh;

	if (n == 0)
	{
		return 0;
	}
	
	custom_buffer<simple_buffer<int>> g_lb(n, simple_buffer<int>(w, 0)), g_le(n, simple_buffer<int>(w, 0));
	simple_buffer<int> g_ln(n, 0);

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
				if(ImRes[i] == 255)
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

int GetLinesInfo(simple_buffer<int> &ImRES, simple_buffer<int> &lb, simple_buffer<int> &le, int w, int h) // return ln
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

int DifficultCompareTwoSubs2(simple_buffer<int> &ImF1, simple_buffer<int> *pImILA1, simple_buffer<int> &ImNE11, simple_buffer<int> &ImNE12, simple_buffer<int> &ImF2, simple_buffer<int> *pImILA2, simple_buffer<int> &ImNE2, int w, int h, int W, int H, int ymin)
{
	simple_buffer<int> ImFF1(ImF1), ImFF2(ImF2);
	simple_buffer<int> ImRES(w*h, 0);
	simple_buffer<int> lb(h, 0), le(h, 0);
	int res, ln;		

	if (g_use_ILA_images_for_search_subtitles)
	{		
		if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_01_1_ImFF1" + g_im_save_format, w, h);
		if (pImILA1 != NULL)
		{
			if (g_show_results) SaveBinaryImage(*pImILA1, "/TestImages/DifficultCompareTwoSubs2_01_2_ImILA1" + g_im_save_format, w, h);
			IntersectTwoImages(ImFF1, *pImILA1, w, h);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_01_3_ImFF1IntImILA1" + g_im_save_format, w, h);
		}

		if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_01_4_ImFF2" + g_im_save_format, w, h);
		if (pImILA2 != NULL)
		{
			if (g_show_results) SaveBinaryImage(*pImILA2, "/TestImages/DifficultCompareTwoSubs2_01_5_ImILA2" + g_im_save_format, w, h);
			IntersectTwoImages(ImFF2, *pImILA2, w, h);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_01_6_ImFF2IntImILA2" + g_im_save_format, w, h);
		}
	}

	auto filter_image = [&w, &h](simple_buffer<int> &ImFF) {
		simple_buffer<int> lb(h, 0), le(h, 0);
		int ln;
		ln = GetLinesInfo(ImFF, lb, le, w, h);
		for (int l = 0; l < ln; l++)
		{
			int hh = (le[l] - lb[l] + 1);
			simple_buffer<int> SubIm(w*hh, 0);
			memcpy(&SubIm[0], &ImFF[w*lb[l]], w*hh * sizeof(int));
			if (AnalyseImage(SubIm, NULL, w, hh) == 0)
			{
				memset(&ImFF[w*lb[l]], 0, w*hh * sizeof(int));
			}
		}
	};

	AddTwoImages(ImF1, ImF2, ImRES, w*h);
	ln = GetLinesInfo(ImRES, lb, le, w, h);
	
	concurrency::parallel_invoke(
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_02_1_ImFF1" + g_im_save_format, w, h);
			FilterImage(ImFF1, ImNE11, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_02_2_ImFF1F" + g_im_save_format, w, h);
			filter_image(ImFF1);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/DifficultCompareTwoSubs2_02_3_ImFF1FF" + g_im_save_format, w, h);
		},
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_03_1_ImFF2" + g_im_save_format, w, h);
			FilterImage(ImFF2, ImNE2, w, h, W, H, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_03_2_ImFF2F" + g_im_save_format, w, h);
			filter_image(ImFF2);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/DifficultCompareTwoSubs2_03_3_ImFF2FF" + g_im_save_format, w, h);
		}
	);	

	res = CompareTwoSubs(ImFF1, pImILA1, ImNE11, ImNE12, ImFF2, pImILA2, ImNE2, w, h, W, H, ymin);

	return res;
}
/*
// W - full image include scale (if is) width
// H - full image include scale (if is) height
int DifficultCompareTwoSubs(simple_buffer<int> &ImRGB1, simple_buffer<int> &ImF1, simple_buffer<int> &ImRGB2, simple_buffer<int> &ImF2, int w, int h, int W, int H, int ymin)
{
	simple_buffer<int> ImFF1(w*h, 0), ImNE1(w*h, 0), ImY1(w*h, 0);
	simple_buffer<int> ImFF2(w*h, 0), ImNE2(w*h, 0), ImY2(w*h, 0);
	simple_buffer<int> ImTEMP1(w*h, 0), ImTEMP2(w*h, 0), ImTEMP3(w*h, 0);
	simple_buffer<int> lb(h, 0), le(h, 0);
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
}*/

int CompareTwoSubs(simple_buffer<int> &Im1, simple_buffer<int> *pImILA1, simple_buffer<int> &ImVE11, simple_buffer<int> &ImVE12, simple_buffer<int> &Im2, simple_buffer<int> *pImILA2, simple_buffer<int> &ImVE2, int w, int h, int W, int H, int ymin)
{
	simple_buffer<int> ImRES(w*h, 0), ImVE11R(ImVE11), ImVE12R(ImVE12), ImVE2R(ImVE2), lb(h, 0), le(h, 0);
	int i, ib, ie, k, y, l, ln, bln, val1, val2, dif, dif1, dif2, cmb, segh, dn;
	double veple;

	veple = g_veple;
	segh = g_segh;

	if (g_use_ILA_images_for_search_subtitles && ((pImILA1 != NULL) || (pImILA2 != NULL)))
	{
		simple_buffer<int> ImFF1(Im1), ImFF2(Im2);

		concurrency::parallel_invoke(
			[&ImFF1, pImILA1, w, h] {
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/CompareTwoSubs_01_01_01_ImFF1" + g_im_save_format, w, h);
			if (pImILA1 != NULL)
			{
				if (g_show_results) SaveBinaryImage(*pImILA1, "/TestImages/CompareTwoSubs_01_01_02_ImILA1" + g_im_save_format, w, h);
				IntersectTwoImages(ImFF1, *pImILA1, w, h);
				if (g_show_results) SaveGreyscaleImage(ImFF1, "/TestImages/CompareTwoSubs_01_01_03_ImFF1IntImILA1" + g_im_save_format, w, h);
			}
		},
			[&ImVE11R, pImILA1, w, h] {
			if (pImILA1 != NULL)
			{
				if (g_show_results) SaveGreyscaleImage(ImVE11R, "/TestImages/CompareTwoSubs_01_01_04_ImVE11R" + g_im_save_format, w, h);
				IntersectTwoImages(ImVE11R, *pImILA1, w, h);
				if (g_show_results) SaveGreyscaleImage(ImVE11R, "/TestImages/CompareTwoSubs_01_01_05_ImVE11RIntImILA1" + g_im_save_format, w, h);
			}
		},
			[&ImVE12R, &ImVE11, &ImVE12, pImILA1, w, h] {
			if ((pImILA1 != NULL) && (ImVE11.m_pData != ImVE12.m_pData))
			{
				if (g_show_results) SaveGreyscaleImage(ImVE12R, "/TestImages/CompareTwoSubs_01_02_01_ImVE12R" + g_im_save_format, w, h);
				IntersectTwoImages(ImVE12R, *pImILA1, w, h);
				if (g_show_results) SaveGreyscaleImage(ImVE12R, "/TestImages/CompareTwoSubs_01_02_01_ImVE12RIntImILA1" + g_im_save_format, w, h);
			}
		},
			[&ImFF2, pImILA2, w, h] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/CompareTwoSubs_01_02_01_ImFF2" + g_im_save_format, w, h);
			if (pImILA2 != NULL)
			{
				if (g_show_results) SaveBinaryImage(*pImILA2, "/TestImages/CompareTwoSubs_01_02_02_ImILA2" + g_im_save_format, w, h);
				IntersectTwoImages(ImFF2, *pImILA2, w, h);
				if (g_show_results) SaveGreyscaleImage(ImFF2, "/TestImages/CompareTwoSubs_01_02_03_ImFF2IntImILA2" + g_im_save_format, w, h);
			}
		},
			[&ImVE2R, pImILA2, w, h] {
			if (pImILA2 != NULL)
			{
				if (g_show_results) SaveGreyscaleImage(ImVE2R, "/TestImages/CompareTwoSubs_01_02_04_ImVE2R" + g_im_save_format, w, h);
				IntersectTwoImages(ImVE2R, *pImILA2, w, h);
				if (g_show_results) SaveGreyscaleImage(ImVE2R, "/TestImages/CompareTwoSubs_01_02_05_ImVE2RIntImILA2" + g_im_save_format, w, h);
			}
		});

		AddTwoImages(ImFF1, ImFF2, ImRES, w*h);
		if (g_show_results) SaveGreyscaleImage(ImRES, "/TestImages/CompareTwoSubs_01_03_ImRES" + g_im_save_format, w, h);
	}
	else
	{
		AddTwoImages(Im1, Im2, ImRES, w*h);
	}
		
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

	auto compare = [&ImRES, &lb, &le, ln, l, w, ymin, H, veple](simple_buffer<int> &ImVE1, simple_buffer<int> &ImVE2) {
		int bln = 1, k, i, ib, ie, dif, dif1, dif2, cmb, val1, val2;
		for (k = 0; k < ln; k++)
		{
			ib = (lb[k] + 1)*w;
			ie = (le[k] - 1)*w;

			if (((ln > 0) && (l < ln - 1) && (lb[l] + ymin > H / 4) &&
				(le[l] + ymin < H / 2) && (lb[ln - 1] + ymin > H / 2))) // egnoring garbage line
			{
				continue;
			}

			dif1 = 0;
			dif2 = 0;
			cmb = 0;

			for (i = ib; i < ie; i++)
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

			if (cmb == 0)
			{
				bln = 0;
				break;
			}

			if ((double)dif / (double)cmb <= veple)
			{
				continue;
			}

			dif1 = 0;
			dif2 = 0;
			cmb = 0;

			for (i = ib; i < ie; i++)
			{
				if (ImRES[i] == 255)
				{
					if ((ImRES[i - w] == 255) && (ImRES[i + w] == 255))
					{
						val1 = ImVE1[i] | ImVE1[i - w] | ImVE1[i + w];
						val2 = ImVE2[i] | ImVE2[i - w] | ImVE2[i + w];
					}
					else if (ImRES[i - w] == 255)
					{
						val1 = ImVE1[i] | ImVE1[i - w];
						val2 = ImVE2[i] | ImVE2[i - w];
					}
					else if (ImRES[i + w] == 255)
					{
						val1 = ImVE1[i] | ImVE1[i + w];
						val2 = ImVE2[i] | ImVE2[i + w];
					}
					else
					{
						val1 = ImVE1[i];
						val2 = ImVE2[i];
					}

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

			if ((double)dif / (double)cmb > veple)
			{
				bln = 0;
				break;
			}
		}

		return bln;
	};

	if (ImVE11.m_pData != ImVE12.m_pData)
	{
		concurrency::parallel_invoke(
			[&ImVE11R, &ImVE2R, &val1, &compare] {
			val1 = compare(ImVE11R, ImVE2R);
		},
			[&ImVE12R, &ImVE2R, &val2, &compare] {
			val2 = compare(ImVE12R, ImVE2R);
		});

		bln = val1 | val2;
	}
	else
	{
		bln = compare(ImVE11R, ImVE2R);
	}

	return bln;
}

int CompareTwoSubsOptimal(simple_buffer<int> &Im1, simple_buffer<int> *pImILA1, simple_buffer<int> &ImVE11, simple_buffer<int> &ImVE12, simple_buffer<int> &Im2, simple_buffer<int> *pImILA2, simple_buffer<int> &ImVE2, int w, int h, int W, int H, int ymin)
{
	int bln = 0;
	
	if (bln == 0)
	{
		bln = CompareTwoSubs(Im1, pImILA1, ImVE11, ImVE12, Im2, pImILA2, ImVE2, w, h, W, H, ymin);
	}

	if (bln == 0)
	{
		bln = DifficultCompareTwoSubs2(Im1, pImILA1, ImVE11, ImVE12, Im2, pImILA2, ImVE2, w, h, W, H, ymin);
	}

	return bln;
}

int SimpleCombineTwoImages(simple_buffer<int> &Im1, simple_buffer<int> &Im2, int size)
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

int GetCombinedSquare(simple_buffer<int> &Im1, simple_buffer<int> &Im2, int size)
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

void AddTwoImages(simple_buffer<int> &Im1, simple_buffer<int> &Im2, simple_buffer<int> &ImRES, int size)
{	
	int i;

	memcpy(ImRES.m_pData, Im1.m_pData, size*sizeof(int));

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) ImRES[i] = 255;
	}
}

void AddTwoImages(simple_buffer<int> &Im1, simple_buffer<int> &Im2, int size)
{
	int i;

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) Im1[i] = 255;
	}
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
void ImToNativeSize(simple_buffer<int> &ImOrig, simple_buffer<int> &ImRes, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	int i, j, dj, x, y;

	custom_assert(ImRes.m_size >= W * H, "ImToNativeSize: Im.m_size >= W*H");

	memset(ImRes.m_pData, 255, W*H * sizeof(int));

	i = 0;
	j = ymin * W + xmin;
	dj = W - w;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			ImRes[j] = ImOrig[i];
			i++;
			j++;
		}
		j += dj;
	}
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
void ImToNativeSize(simple_buffer<int> &Im, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	simple_buffer<int> ImTMP(Im);
	ImToNativeSize(ImTMP, Im, w, h, W, H, xmin, xmax, ymin, ymax);	
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

s64 GetVideoTime(string time)
{
	s64 res;
	int hour, min, sec, msec;
	wxASSERT_MSG(sscanf(time.c_str(), "%d:%d:%d:%d", &hour, &min, &sec, &msec) == 4, wxString::Format(wxT("Wrong video time format '%s'"), time.c_str()));
	res = (s64)(((hour * 60 + min) * 60 + sec) * 1000 + msec);
	return res;
}

s64 GetVideoTime(int minute, int sec, int mili_sec)
{
	s64 res;
	res = (s64)((minute * 60 + sec) * 1000 + mili_sec);
	return res;
}

wxString GetFileName(wxString FilePath)
{
	wxString res;

	wxRegEx re = "([^\\\\\\/]+)\\.[^\\\\\\/\\.]+$";
	if (re.Matches(FilePath))
	{
		res = re.GetMatch(FilePath, 1);
	}

	return res;
}
