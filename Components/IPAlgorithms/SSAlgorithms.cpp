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
#include <opencv2/core.hpp>
#include <opencv2/core/ocl.hpp>

#ifdef CUSTOM_TA 
#include "ittnotify.h"

__itt_domain* domain = __itt_domain_create(L"MyTraces.MyDomain");

__itt_string_handle* shOneStep = __itt_string_handle_create(L"OneStep");
__itt_string_handle* shConvertToRGB = __itt_string_handle_create(L"ConvertToRGB");
__itt_string_handle* shGetTransformedImage = __itt_string_handle_create(L"GetTransformedImage");
__itt_string_handle* shFirstCheck = __itt_string_handle_create(L"FirstCheck");
__itt_string_handle* shSecondCheckPart1 = __itt_string_handle_create(L"SecondCheckPart1");
__itt_string_handle* shSecondCheckPart2 = __itt_string_handle_create(L"SecondCheckPart2");
__itt_string_handle* shAddIntersectImagesTaskV1 = __itt_string_handle_create(L"AddIntersectImagesTaskV1");
__itt_string_handle* shAddIntersectImagesTaskV2 = __itt_string_handle_create(L"AddIntersectImagesTaskV2");
__itt_string_handle* shSubFound = __itt_string_handle_create(L"SubFound");
#endif

std::chrono::time_point<std::chrono::high_resolution_clock> g_StartTimeRunSubSearch = std::chrono::high_resolution_clock::now();
int		g_RunSubSearch = 0;

int    g_threads = -1;

int		g_DL = 6;	 //sub frame length
double	g_tp = 0.3;	 //text percent
double	g_mtpl = 0.022;  //min text len (in percent)
double	g_veple = 0.30; //vedges points line error
double	g_ilaple = 0.30; //ILA points line error

bool g_use_ISA_images_for_search_subtitles = true;
bool g_use_ILA_images_for_search_subtitles = true;
bool g_replace_ISA_by_filtered_version = true;
int g_max_dl_down = 20;
int g_max_dl_up = 40;

double	g_video_contrast = 1.0; //1.0 default without change
double	g_video_gamma = 1.0; //1.0 default without change

CVideo *g_pV;

inline int AnalizeImageForSubPresence(simple_buffer<u8> &ImNE, simple_buffer<u8> &ImISA, simple_buffer<u16> &ImIL, s64 CurPos, int fn, int w, int h, int W, int H, int min_x, int max_x)
{
	int res = 1;

	if (g_use_ISA_images_for_search_subtitles)
	{
		simple_buffer<int> LB(1, 0), LE(1, 0);
		LB[0] = 0;
		LE[0] = h - 1;

		simple_buffer<u8> ImFF(ImISA), ImTF(w*h, (u8)0);

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(ImISA, wxString("/DebugImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_01_ImISA_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}
#endif

		if (g_use_ILA_images_for_search_subtitles)
		{
#ifdef CUSTOM_DEBUG
			{
				SaveBinaryImage(ImIL, wxString("/DebugImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_02_ImIL_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}
#endif

			if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
			{
				cv::Mat cv_im_gr;
				simple_buffer<u8> ImTMP(w * h);
				BinaryImageToMat(ImIL, w, h, cv_im_gr);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 4);
				BinaryMatToImage(cv_im_gr, w, h, ImTMP, (u8)255);

#ifdef CUSTOM_DEBUG
				{
					SaveBinaryImage(ImTMP, wxString("/DebugImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_02_ImILDilate_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
				}
#endif

				IntersectTwoImages(ImFF, ImTMP, w, h);
			}
			else
			{
				IntersectTwoImages(ImFF, ImIL, w, h);
			}

#ifdef CUSTOM_DEBUG
			{
				SaveGreyscaleImage(ImFF, wxString("/DebugImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_03_ImFF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			}
#endif
		}
		
		simple_buffer<u8> ImSF(ImFF);

		res = FilterTransformedImage(ImFF, ImSF, ImTF, ImNE, LB, LE, 1, w, h, W, H, min_x, max_x, VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn));

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(ImTF, wxString("/DebugImages/AnalizeImageForSubPresence_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_04_ImTF_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
		}
#endif

		if (g_replace_ISA_by_filtered_version)
		{
			ImISA.copy_data(ImTF, w*h);
		}
	}

	return res;
}

template <class T1, class T2>
inline void IntersectYImages(simple_buffer<T1> &ImRes, simple_buffer<T2> &Im2, int w, int h)
{
	int i, size;

	size = w * h;
	for (i = 0; i < size; i++)
	{
		if (ImRes[i])
		{
			if ((int)Im2[i]  < (int)ImRes[i] - g_max_dl_down)
			{
				ImRes[i] = 0;
			}
			else if ((int)Im2[i] > (int)ImRes[i] + g_max_dl_up)
			{
				ImRes[i] = 0;
			}
		}
	}
}

template <class T1, class T2>
inline void IntersectYImages(simple_buffer<T1>& ImRes, simple_buffer<simple_buffer<T2>*>& ImIn, int min_id_im_in, int max_id_im_in, int w, int h)
{
	int i, size, im_id;

	size = w * h;
	for (i = 0; i < size; i++)
	{	
		for (im_id = min_id_im_in; (im_id <= max_id_im_in) && ImRes[i]; im_id++)
		{
			if ((int)(*(ImIn[im_id]))[i] < (int)ImRes[i] - g_max_dl_down)
			{
				ImRes[i] = 0;
			}
			else if ((int)(*(ImIn[im_id]))[i] > (int)ImRes[i] + g_max_dl_up)
			{
				ImRes[i] = 0;
			}
		}
	}
}


int CompareTwoSubsByOffset(simple_buffer<simple_buffer<u8>*> &ImForward, simple_buffer<simple_buffer<u16>*> &ImYForward, simple_buffer<simple_buffer<u8>*> &ImNEForward,
			simple_buffer<u8> &ImIntS, simple_buffer<u16> &ImYS, simple_buffer<u8> &ImNES, simple_buffer<u8> &prevImNE,
			int w, int h, int W, int H, int min_x, int max_x, int offset, int fn)
{
	simple_buffer<u8>* pImNE12 = (offset == 0) ? &prevImNE : ImNEForward[offset - 1];
	int bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, *pImNE12, *(ImForward[offset]), NULL, *(ImNEForward[offset]), w, h, W, H, min_x, max_x, wxDEBUG_DET("CompareTwoSubsByOffset_fn" + std::to_string(fn) + "_offset" + std::to_string(offset) + "_line" + std::to_string(__LINE__)));

	if (bln == 0)
	{
		simple_buffer<u8> ImInt2(w*h, (u8)0);
		simple_buffer<u16> ImYInt2(w*h, (u16)0);
	
		int DL = g_DL;

		run_in_parallel(
			[&ImInt2, &ImForward, DL, offset, w, h] {
				ImInt2 = *(ImForward[offset]);

				IntersectImages(ImInt2, ImForward, offset + 1, DL - 2, w, h);
			},
				[&ImYInt2, &ImYForward, DL, offset, w, h] {
				if (g_use_ILA_images_for_search_subtitles)
				{
					ImYInt2 = *(ImYForward[offset]);
					IntersectYImages(ImYInt2, ImYForward, offset + 1, DL - 2, w, h);
				}
			}
		);

		bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, *pImNE12, ImInt2, &ImYInt2, *(ImNEForward[offset]), w, h, W, H, min_x, max_x, wxDEBUG_DET("CompareTwoSubsByOffset_fn" + std::to_string(fn) + "_offset" + std::to_string(offset) + "_line" + std::to_string(__LINE__)));
	}

	return bln;
}

int FindOffsetForNewSub(simple_buffer<simple_buffer<u8>*> &ImForward, simple_buffer<simple_buffer<u16>*> &ImYForward, simple_buffer<simple_buffer<u8>*> &ImNEForward,
	simple_buffer<u8> &ImIntS, simple_buffer<u16> &ImYS, simple_buffer<u8> &ImNES, simple_buffer<u8> &prevImNE,
	int w, int h, int W, int H, int min_x, int max_x, int fn)
{	
	int DL = g_DL;
	int offset = 0;

#ifdef CUSTOM_DEBUG
	{
		for (int i = 0; i < DL; i++)
		{
			SaveGreyscaleImage(*(ImForward[i]), wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImForward" + std::to_string(i) + g_im_save_format, w, h);			
			SaveBinaryImage(*(ImYForward[i]), wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImForward" + std::to_string(i) + g_im_save_format, w, h);
			SaveGreyscaleImage(*(ImNEForward[i]), wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImForward" + std::to_string(i) + g_im_save_format, w, h);
		}		
		SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
		SaveBinaryImage(ImYS, wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImNES, wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
		SaveGreyscaleImage(prevImNE, wxString("/DebugImages/FindOffsetForNewSub") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_prevImNE" + g_im_save_format, w, h);
	}
#endif

	//if (g_threads == 1)
	{
		for (offset = 0; offset < DL - 1; offset++)
		{
			if (CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, min_x, max_x, offset, fn) == 0)
			{
				break;
			}
		}
	}
	
	/*
	if (g_threads >= 2)
	{
		simple_buffer<int> blns(DL, -1);
		int l = 0, r = (DL - 2)/2;

		while (1)
		{
			if ((l != r) && (blns[l] == -1) && (blns[r] == -1))
			{
				run_in_parallel(
					[&blns, &ImForward, &ImYForward, &ImNEForward, &ImIntS, &ImYS, &ImNES, &prevImNE, w, h, W, H, min_x, max_x, l] {
						blns[l] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, min_x, max_x, l, fn);
					},
					[&blns, &ImForward, &ImYForward, &ImNEForward, &ImIntS, &ImYS, &ImNES, &prevImNE, w, h, W, H, min_x, max_x, r] {
						blns[r] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, min_x, max_x, r, fn);
					}
				);				
			}
			else
			{
				if (blns[l] == -1)
				{
					blns[l] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, min_x, max_x, l, fn);
				}

				if (blns[r] == -1)
				{
					blns[r] = CompareTwoSubsByOffset(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, min_x, max_x, r, fn);
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
	*/

	return offset;
}

inline shared_custom_task TaskConvertImage(int fn, my_event &evt_rgb, my_event &evt, simple_buffer<u8> &ImBGR, simple_buffer<u8> &ImF, simple_buffer<u8> &ImNE, simple_buffer<u16> &ImY, simple_buffer<u8>* pImLab, int w, int h, int W, int H, int &res)
{
	return shared_custom_task([fn, &evt_rgb, &evt, &ImBGR, &ImF, &ImNE, &ImY, pImLab, w, h, W, H, &res]
	{		
		evt_rgb.wait();

		custom_set_started(&evt);

		if (!evt_rgb.m_need_to_skip)
		{
#ifdef CUSTOM_TA
			__itt_task_begin(domain, __itt_null, __itt_null, shGetTransformedImage);
#endif
			if (pImLab != NULL)
			{
				run_in_parallel(
					[&ImBGR, &ImF, &ImNE, &ImY, w, h, W, H, &res] {
						simple_buffer<u8> ImFF(w * h, (u8)0), ImSF(w * h, (u8)0), ImYOrig(w * h, (u8)0);
						res = GetTransformedImage(ImBGR, ImFF, ImSF, ImF, ImNE, ImYOrig, w, h, W, H, 0, w - 1);
						if (res == 0)
						{
							ImF.set_values(0, w * h);
							ImNE.set_values(0, w * h);
							ImY.set_values(0, w * h);
						}
						else
						{
							for (int i = 0; i < w * h; i++)
							{
								ImY[i] = (u16)(ImYOrig[i]) + (u16)255;
							}
						}
					},
					[&ImBGR, pImLab, w, h, &res] {
						cv::Mat cv_ImBGR, cv_ImLab;
						BGRImageToMat(ImBGR, w, h, cv_ImBGR);
						cv::cvtColor(cv_ImBGR, cv_ImLab, cv::COLOR_BGR2Lab);
						BGRMatToImage(cv_ImLab, w, h, *pImLab);
					} );
			}
			else
			{
				simple_buffer<u8> ImFF(w * h, (u8)0), ImSF(w * h, (u8)0), ImYOrig(w * h, (u8)0);
				res = GetTransformedImage(ImBGR, ImFF, ImSF, ImF, ImNE, ImYOrig, w, h, W, H, 0, w - 1);
				if (res == 0)
				{
					ImF.set_values(0, w * h);
					ImNE.set_values(0, w * h);
					ImY.set_values(0, w * h);
				}
				else
				{
					for (int i = 0; i < w * h; i++)
					{
						ImY[i] = (u16)(ImYOrig[i]) + (u16)255;
					}
				}
			}

			if (res != 0)
			{
				res = FilterImageByPixelColorIsInRange(ImY, &ImBGR, pImLab, w, h, wxT(""), (u16)0, true, true);

				if (res == 0)
				{
					ImF.set_values(0, w * h);
					ImNE.set_values(0, w * h);
				}
			}

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
	int m_w, m_h, m_W, m_H, m_xmin, m_xmax, m_ymin, m_ymax, m_size;
	CVideo *m_pV;
	int m_fn_start;
	s64 m_prevPos;
	simple_buffer<s64> m_Pos;
	simple_buffer<s64*> m_pPos;
	custom_buffer<simple_buffer<u8>> m_FrameData;
	simple_buffer<simple_buffer<u8>*> m_pFrameData;
	custom_buffer<simple_buffer<u8>> m_ImBGR;
	simple_buffer<simple_buffer<u8>*> m_pImBGR;
	custom_buffer<simple_buffer<u8>> m_ImNE;
	simple_buffer<simple_buffer<u8>*> m_pImNE;
	custom_buffer<simple_buffer<u16>> m_ImY;
	simple_buffer<simple_buffer<u16>*> m_pImY;
	custom_buffer<simple_buffer<u8>> m_Im;
	simple_buffer<simple_buffer<u8>*> m_pIm;
	custom_buffer<simple_buffer<u8>> m_ImLab;
	simple_buffer<simple_buffer<u8>*> m_pImLab;

	custom_buffer<simple_buffer<u8>> m_ImInt;
	simple_buffer<simple_buffer<u8>*> m_pImInt;
	custom_buffer<simple_buffer<u16>> m_ImYInt;
	simple_buffer<simple_buffer<u16>*> m_pImYInt;

	vector<shared_custom_task> m_thrs_one_step;
	vector<shared_custom_task> m_thrs_rgb;
	vector<shared_custom_task> m_thrs;
	vector<shared_custom_task> m_thrs_int;
	vector<shared_custom_task> m_thrs_save_images;

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
	
	bool m_convert_to_lab;

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
		m_fn_start = 0;
		m_prevPos = -2;

		m_convert_to_lab = false;
		if (g_color_ranges.size() > 0)
		{
			for (int i = 0; i < g_color_ranges.size(); i++)
			{
				if (g_color_ranges[i].m_color_space == ColorSpace::Lab)
				{
					m_convert_to_lab = true;
					break;
				}
			}
		}
		if (!m_convert_to_lab)
		{
			if (g_outline_color_ranges.size() > 0)
			{
				for (int i = 0; i < g_outline_color_ranges.size(); i++)
				{
					if (g_outline_color_ranges[i].m_color_space == ColorSpace::Lab)
					{
						m_convert_to_lab = true;
						break;
					}
				}
			}
		}

		m_N = std::max<int>(g_DL + threads, (g_DL / 2) * threads);
		m_Pos = simple_buffer<s64>(m_N, (s64)-1);
		m_pPos = simple_buffer<s64*>(m_N, (s64*)NULL);
		m_FrameData = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(pV->GetFrameDataSize(), (u8)0));
		m_pFrameData = simple_buffer<simple_buffer<u8>*>(m_N);
		m_ImBGR = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(m_size * 3, (u8)0));
		m_pImBGR = simple_buffer<simple_buffer<u8>*>(m_N);
		m_ImNE = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(m_size, (u8)0));
		m_pImNE = simple_buffer<simple_buffer<u8>*>(m_N);
		m_ImY = custom_buffer<simple_buffer<u16>>(m_N, simple_buffer<u16>(m_size, (u16)0));
		m_pImY = simple_buffer<simple_buffer<u16>*>(m_N);
		m_Im = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(m_size, (u8)0));
		m_pIm = simple_buffer<simple_buffer<u8>*>(m_N);		

		m_ImInt = custom_buffer<simple_buffer<u8>>(m_threads, simple_buffer<u8>(m_size, (u8)0));
		m_pImInt = simple_buffer<simple_buffer<u8>*>(m_threads);
		m_ImYInt = custom_buffer<simple_buffer<u16>>(m_threads, simple_buffer<u16>(m_size, (u16)0));
		m_pImYInt = simple_buffer<simple_buffer<u16>*>(m_threads);

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
			m_pImBGR[i] = &(m_ImBGR[i]);
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

		if (m_convert_to_lab)
		{
			m_ImLab = custom_buffer<simple_buffer<u8>>(m_N, simple_buffer<u8>(m_size * 3, (u8)0));
			m_pImLab = simple_buffer<simple_buffer<u8>*>(m_N);

			for (int i = 0; i < m_N; i++)
			{
				m_pImLab[i] = &(m_ImLab[i]);
			}
		}

		m_thrs_one_step = vector<shared_custom_task>(m_N, shared_custom_task([] {}));
		wait_all(begin(m_thrs_one_step), end(m_thrs_one_step));

		m_thrs_rgb = vector<shared_custom_task>(m_N, shared_custom_task([] {}));
		wait_all(begin(m_thrs_rgb), end(m_thrs_rgb));

		m_thrs = vector<shared_custom_task>(m_N, shared_custom_task([] {}));
		wait_all(begin(m_thrs), end(m_thrs));

		m_thrs_int = vector<shared_custom_task>(m_threads, shared_custom_task([] {}));
		wait_all(begin(m_thrs_int), end(m_thrs_int));
	}

	~RunSearch()
	{		
		wait_all(begin(m_thrs_one_step), end(m_thrs_one_step));
		wait_all(begin(m_thrs_rgb), end(m_thrs_rgb));
		wait_all(begin(m_thrs), end(m_thrs));
		wait_all(begin(m_thrs_int), end(m_thrs_int));
		wait_all(begin(m_thrs_save_images), end(m_thrs_save_images));
	}

	void AddSaveImagesTask(simple_buffer<u8>& ImBGR, simple_buffer<u8>& ImISA, simple_buffer<u16>& ImILA, wxString name)
	{
		int w = m_w;
		int h = m_h;
		int W = m_W;
		int H = m_H;
		int xmin = m_xmin;
		int xmax = m_xmax;
		int ymin = m_ymin;
		int ymax = m_ymax;
		bool convert_to_lab = m_convert_to_lab;

		m_thrs_save_images.emplace_back(shared_custom_task([ImBGR, ImISA, ImILA, name, w, h, W, H, xmin, xmax, ymin, ymax, convert_to_lab]() mutable {
					{
						simple_buffer<u8> ImTMP_BGR(W * H * 3);
						ImBGRToNativeSize(ImBGR, ImTMP_BGR, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewBGRImage[0](ImTMP_BGR, W, H);
						SaveBGRImage(ImBGR, wxT("/RGBImages/") + name + g_im_save_format, w, h);
					}
					{
						simple_buffer<u8> ImTMP_U8(W * H);
						ImToNativeSize(ImISA, ImTMP_U8, w, h, W, H, xmin, xmax, ymin, ymax);
						g_pViewGreyscaleImage[1](ImTMP_U8, W, H);
						SaveGreyscaleImage(ImISA, wxT("/ISAImages/") + name + g_im_save_format, w, h);
					}
					{
						if ((g_color_ranges.size() > 0) || (g_outline_color_ranges.size() > 0))
						{
							simple_buffer<u16> ImILARes(ImILA);
							simple_buffer<u8> ImLab, *pImLab = NULL;

							if (convert_to_lab)
							{
								ImLab.set_size(w * h * 3);
								pImLab = &ImLab;

								cv::Mat cv_ImBGR, cv_ImLab;
								BGRImageToMat(ImBGR, w, h, cv_ImBGR);
								cv::cvtColor(cv_ImBGR, cv_ImLab, cv::COLOR_BGR2Lab);
								BGRMatToImage(cv_ImLab, w, h, *pImLab);
							}

							FilterImageByPixelColorIsInRange(ImILARes, &ImBGR, pImLab, w, h, wxT(""), (u16)0, false, true);

							SaveBinaryImage(ImILARes, wxT("/ILAImages/") + name + g_im_save_format, w, h);
						}
						else
						{
							SaveBinaryImage(ImILA, wxT("/ILAImages/") + name + g_im_save_format, w, h);
						}
					}
				}
			)
		);
	}

	int AddGetRGBImagesTask(int fn, int num)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N), "AddGetRGBImagesTask: (fdn >= 0) && (fdn < m_N)");
		custom_assert((fdn + num - 1 < m_N), "AddGetRGBImagesTask: (fdn + num - 1 < m_N)");
		custom_assert((num >= 1), "AddGetRGBImagesTask: (num >= 1)");
		
		simple_buffer<simple_buffer<u8>*> pFrameData(m_pFrameData);
		simple_buffer<simple_buffer<u8>*> pImBGR(m_pImBGR);
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

			m_thrs_one_step[j] = shared_custom_task([fn, fdn, num, pPos, pFrameData, pV, p_events_one_step, need_to_get]() mutable
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

						if ((fn == 0) && (i == 0))
						{
							*(pPos[fdn + i]) = pV->GetPos();
							pV->GetFrameData(*(pFrameData[fdn + i]));
						}
						else
						{
							*(pPos[fdn + i]) = pV->OneStepWithTimeout();
							pV->GetFrameData(*(pFrameData[fdn + i]));
						}
#ifdef CUSTOM_TA
						__itt_task_end(domain);
#endif

						p_events_one_step[fdn + i]->set();

					}
				}
			});

			m_thrs_rgb[j] = shared_custom_task([fn, fdn, num, pFrameData, pImBGR, pV, xmin, xmax, ymin, ymax, p_events_one_step, p_events_rgb, need_to_get]() mutable
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

									pV->ConvertToBGR(&((*(pFrameData[fdn + i]))[0]), *(pImBGR[fdn + i]), xmin, xmax, ymin, ymax);
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

							SaveBGRImage(*(pImBGR[fdn + i]), wxString("/DebugImages/AddGetRGBImagesTask_ImBGR") + "_fn" + std::to_string(fn + i) + g_im_save_format, xmax - xmin + 1, ymax - ymin + 1);
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

			simple_buffer<simple_buffer<u8>*> pIm(m_pIm);
			simple_buffer<simple_buffer<u16>*> pImY(m_pImY);
			simple_buffer<my_event*> p_events(m_p_events);
			simple_buffer<int*> pthrs_res(m_pthrs_res);
			simple_buffer<u8>* pImInt = m_pImInt[fdn];
			simple_buffer<u16>* pImYInt = m_pImYInt[fdn];			
			int threads = m_threads;
			int w = m_w;
			int h = m_h;
			int DL = g_DL;

			m_thrs_int[fdn] = shared_custom_task([pthrs_int_res, fn, fn_start, fdn, pIm, pImY, p_events, pthrs_res, pImInt, pImYInt, threads, w, h, DL]() mutable
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
#ifdef CUSTOM_TA
					__itt_task_begin(domain, __itt_null, __itt_null, shAddIntersectImagesTaskV1);
#endif
					run_in_parallel(
							[&pImInt, &pIm, fdn, DL, w, h] {
							*pImInt = *pIm[fdn];
							IntersectImages(*(pImInt), pIm, fdn + 1, fdn + DL - 1, w, h);
						},
							[&pImYInt, &pImY, fdn, DL, w, h] {
							if (g_use_ILA_images_for_search_subtitles)
							{
								*pImYInt = *(pImY[fdn]);
								IntersectYImages(*pImYInt, pImY, fdn + 1, fdn + DL - 1, w, h);
							}
						}
					);

#ifdef CUSTOM_DEBUG
					{
						if (fn == 54)
						{
							SaveGreyscaleImage(*pImInt, wxString("/DebugImages/AddIntersectImagesTask_ImCombined_") + "_fn" + std::to_string(fn) + g_im_save_format, w, h);
							SaveBinaryImage(*pImYInt, wxString("/DebugImages/AddIntersectImagesTask_ImYInt_") + "_fn" + std::to_string(fn) + g_im_save_format, w, h);

							for (int i = 0; i < DL; i++)
							{
								SaveGreyscaleImage(*(pIm[fdn + i]), wxString("/DebugImages/AddIntersectImagesTask_Im_") + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
								SaveBinaryImage(*(pImY[fdn + i]), wxString("/DebugImages/AddIntersectImagesTask_ImY_") + "_fn" + std::to_string(fn + i) + g_im_save_format, w, h);
							}

							fn = fn;
						}
					}
#endif

					bln = AnalyseImage(*pImInt, pImYInt, w, h);
#ifdef CUSTOM_TA 
					__itt_task_end(domain);
#endif
				}
				else if (!need_to_skip)
				{
#ifdef CUSTOM_TA
					__itt_task_begin(domain, __itt_null, __itt_null, shAddIntersectImagesTaskV2);
#endif
					*pImInt = *pIm[fdn];
					*pImYInt = *pImY[fdn];					
#ifdef CUSTOM_TA
					__itt_task_end(domain);
#endif
				}

				*pthrs_int_res = bln;
			});
		}
	}

	int GetIntersectImages(int fn, simple_buffer<u8> *&pImInt, simple_buffer<u16> *&pImYInt)
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

			simple_buffer<u8>* pImLab = (m_convert_to_lab) ? m_pImLab[fdn] : NULL;
			m_thrs[fdn] = TaskConvertImage(fn, *(m_p_events_rgb[fdn]), *(m_p_events[fdn]), *(m_pImBGR[fdn]), *(m_pIm[fdn]), *(m_pImNE[fdn]), *(m_pImY[fdn]), pImLab, m_w, m_h, m_W, m_H, *(m_pthrs_res[fdn]));
		}
	}

	int GetConvertImageCopy(int fn, simple_buffer<u8> *pImBGR = NULL, simple_buffer<u8> *pIm = NULL, simple_buffer<u8> *pImNE = NULL, simple_buffer<u16> *pImY = NULL)
	{
		int fdn = fn - m_fn_start;
		
		custom_assert((fdn >= 0) && (fdn < m_N), "GetConvertImageCopy: not: (fdn >= 0) && (fdn < m_N)");
		custom_assert(*(m_pthrs_res[fdn]) != -1, "GetConvertImageCopy: not: *(m_pthrs_res[fdn]) != -1");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "GetConvertImageCopy: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		m_thrs[fdn].wait();
		
		if (pImBGR != NULL)
		{
			*pImBGR = *m_pImBGR[fdn];
		}
		if (pIm != NULL)
		{
			*pIm = *m_pIm[fdn];
		}
		if (pImNE != NULL)
		{
			*pImNE = *m_pImNE[fdn];
		}
		if (pImY != NULL)
		{
			*pImY = *m_pImY[fdn];
		}

		return *(m_pthrs_res[fdn]);
	}

	int GetConvertImage(int fn, simple_buffer<u8> *&pImBGR, simple_buffer<u8> *&pIm, simple_buffer<u8> *&pImNE, simple_buffer<u16> *&pImY, s64 &pos)
	{
		int fdn = fn - m_fn_start;

		custom_assert((fdn >= 0) && (fdn < m_N), "GetConvertImage: not: (fdn >= 0) && (fdn < m_N)");
		custom_assert(*(m_pthrs_res[fdn]) != -1, "GetConvertImage: not: m_pthrs_res[fdn] != -1");
		custom_assert(!m_p_events_rgb[fdn]->m_need_to_skip, "GetConvertImage: not: !m_p_events_rgb[fdn]->m_need_to_skip");

		m_thrs[fdn].wait();

		pImBGR = m_pImBGR[fdn];
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

			wait_all(begin(m_thrs), next(begin(m_thrs),fdn));
			wait_all(begin(m_thrs_one_step), next(begin(m_thrs_one_step), fdn));
			wait_all(begin(m_thrs_rgb), next(begin(m_thrs_rgb), fdn));
			wait_all(begin(m_thrs_int), next(begin(m_thrs_int), std::min<int>(fdn, m_threads)));

			simple_buffer<simple_buffer<u8>*>  pFrameData(m_pFrameData);
			simple_buffer<simple_buffer<u8>*> pImBGR(m_pImBGR);
			simple_buffer<simple_buffer<u8>*> pImNE(m_pImNE);
			simple_buffer<simple_buffer<u16>*> pImY(m_pImY);
			simple_buffer<simple_buffer<u8>*> pIm(m_pIm);
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
				m_pImBGR[i] = pImBGR[i + fdn];
				m_pImNE[i] = pImNE[i + fdn];
				m_pImY[i] = pImY[i + fdn];
				m_pIm[i] = pIm[i + fdn];
				m_pPos[i] = pPos[i + fdn];				
				m_pthrs_res[i] = pthrs_res[i + fdn];
				m_p_events_one_step[i] = p_events_one_step[i + fdn];
				m_p_events_rgb[i] = p_events_rgb[i + fdn];
				m_p_events[i] = p_events[i + fdn];
				m_thrs[i] = std::move(m_thrs[i + fdn]);
				m_thrs_one_step[i] = std::move(m_thrs_one_step[i + fdn]);
				m_thrs_rgb[i] = std::move(m_thrs_rgb[i + fdn]);
			}			

			for (i = m_N - fdn, j=0; i < m_N; i++, j++)
			{
				m_pFrameData[i] = pFrameData[j];
				m_pImBGR[i] = pImBGR[j];
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

			if (m_convert_to_lab)
			{
				simple_buffer<simple_buffer<u8>*> pImLab(m_pImLab);

				for (i = 0; i < (m_N - fdn); i++)
				{
					m_pImLab[i] = pImLab[i + fdn];
				}

				for (i = m_N - fdn, j = 0; i < m_N; i++, j++)
				{
					m_pImLab[i] = pImLab[j];
				}
			}

			if (fdn < m_threads)
			{
				simple_buffer<simple_buffer<u8>*> pImInt(m_pImInt);
				simple_buffer<simple_buffer<u16>*> pImYInt(m_pImYInt);
				simple_buffer<int*> pthrs_int_res(m_pthrs_int_res);

				for (i = 0; i < (m_threads - fdn); i++)
				{
					m_pImInt[i] = pImInt[i + fdn];
					m_pImYInt[i] = pImYInt[i + fdn];
					m_pthrs_int_res[i] = pthrs_int_res[i + fdn];
					m_thrs_int[i] = std::move(m_thrs_int[i + fdn]);
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

	//NOTE: works more faster without SchedulerPolicy on x64

	wxString Str;
	s64 CurPos, prevPos;
	int fn; //cur frame num
	int max_rgb_fn; //max rgb frame num which will be obtained
	int w, h, W, H, xmin, xmax , ymin, ymax, size;
	int DL, threads;

	int bf, ef; // begin, end frame
	int pbf, pef;
	s64 bt, et; // begin, end time
	s64 pbt, pet;

	int found_sub;

	int bln = 0, cmp_prev = 0, bln1 = 0, bln2 = 0, finded_prev;

	//g_disable_save_images = true;
	
	g_StartTimeRunSubSearch = std::chrono::high_resolution_clock::now();	

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

	pV->SetPos(Begin);
	CurPos = pV->GetPos();

	DL = g_DL;

	if (g_threads <= 0)
	{
		g_threads = std::thread::hardware_concurrency();

#ifdef WINX86
		if (g_threads > 12)
		{
			g_threads = 12;
		}
#endif
	}
	if (g_threads < 2)
	{
		g_threads = 2;
	}

	threads = g_threads;

	pbf = -2;
	bf = -2;
	ef = -2;
	et = -2;
	fn = 0;

	finded_prev = 0;

	simple_buffer<u8> ImInt(size, (u8)0);
	simple_buffer<u8> ImIntS(size, (u8)0); //store image
	simple_buffer<u8> ImIntSP(size, (u8)0); //store image prev
	simple_buffer<u8> ImFS(size * 3, (u8)0); //image for save
	simple_buffer<u8> ImFSP(size * 3, (u8)0); //image for save prev
	simple_buffer<u8> ImNES(size, (u8)0), ImNESP(size, (u8)0);
	simple_buffer<u16> ImYInt(size, (u16)0), ImYS(size, (u16)0), ImYSP(size, (u16)0), * pImYInt, * pImY;
	simple_buffer<u8>* pImBGR;
	simple_buffer<u8> *pIm, *pImInt, *pImNE;
	simple_buffer<u8> prevImBGR(size * 3, (u8)0);
	simple_buffer<u8> prevImNE(size, (u8)0), prevIm(size, (u8)0);

	simple_buffer<simple_buffer<u8>*> ImBGRForward(DL, (simple_buffer<u8>*)NULL);
	simple_buffer<simple_buffer<u8>*> ImNEForward(DL, (simple_buffer<u8>*)NULL);
	simple_buffer<simple_buffer<u16>*> ImYForward(DL, (simple_buffer<u16>*)NULL);
	simple_buffer<simple_buffer<u8>*> ImForward(DL, (simple_buffer<u8>*)NULL);
	simple_buffer<s64> PosForward(DL, (s64)0);

	found_sub = 0;

	prevPos = -2;

	RunSearch rs(threads, pV);	
	
	const int ddl = (DL / 2);
	const int ddl1_ofset = ddl - 1;
	const int ddl2_ofset = (ddl * 2) - 1;

	int fn_start;
		
	fn_start = fn;
	max_rgb_fn = -1;

	while (((CurPos < End) || (End < 0)) && (g_RunSubSearch == 1) && (CurPos != prevPos))
	{
		int create_new_threads = threads;	

#ifdef CUSTOM_TA
		if (fn >= 1000)
		{
			CurPos = prevPos;
			break;
		}
#endif

		while ((found_sub == 0) && ((CurPos < End) || (End < 0)) && (CurPos != prevPos) && (g_RunSubSearch == 1))
		{
			prevPos = CurPos;

#ifdef CUSTOM_TA
			if (fn >= 1000)
			{
				CurPos = prevPos;
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

			bln1 = rs.GetConvertImage(fn_start + ddl1_ofset, ImBGRForward[0], ImForward[0], ImNEForward[0], ImYForward[0], PosForward[0]);
			CurPos = rs.GetPos(fn_start + ddl1_ofset);

#ifdef CUSTOM_DEBUG
			SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: searching sub: (fn_start + ddl1_ofset): %d, rs.GetPos(fn_start + ddl1_ofset): %lld(%s)...\n", (int)__LINE__, (fn_start + ddl1_ofset), CurPos, VideoTimeToStr(CurPos)));
#endif

			if (bln1)
			{
				bln2 = rs.GetConvertImage(fn_start + ddl2_ofset, ImBGRForward[1], ImForward[1], ImNEForward[1], ImYForward[1], PosForward[1]);
				CurPos = rs.GetPos(fn_start + ddl2_ofset);

#ifdef CUSTOM_DEBUG
				if (CurPos >= 98000)
				{
					CurPos = CurPos;
				}
#endif
				
				if (bln2)
				{					
#ifdef CUSTOM_TA
					__itt_task_begin(domain, __itt_null, __itt_null, shFirstCheck);
#endif
					run_in_parallel(
						[&rs , &ImInt, &ImForward, fn_start, ddl1_ofset, w, h] {
							rs.GetConvertImageCopy(fn_start + ddl1_ofset, NULL, &ImInt, NULL, NULL);
							IntersectTwoImages(ImInt, *(ImForward[1]), w, h);
						},
						[&rs, &ImYInt, &ImYForward, fn_start, ddl1_ofset, w, h] {
							if (g_use_ILA_images_for_search_subtitles)
							{
								ImYInt = *(ImYForward[0]);
								IntersectYImages(ImYInt, *(ImYForward[1]), w, h);
							}
						}
					);

					bln = AnalyseImage(ImInt, &ImYInt, w, h);
#ifdef CUSTOM_TA 
					__itt_task_end(domain);
#endif

					if (bln)
					{
#ifdef CUSTOM_TA
						__itt_task_begin(domain, __itt_null, __itt_null, shSecondCheckPart1);
#endif
						for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
						{
							rs.AddConvertImageTask(fn_start + i);
						}

						bln = 1;
						for (int i = ddl1_ofset + 1; i <= ddl2_ofset - 1; i++)
						{
							bln = bln && rs.GetConvertImage(fn_start + i, ImBGRForward[i], ImForward[i], ImNEForward[i], ImYForward[i], PosForward[i]);
						}
#ifdef CUSTOM_TA 
						__itt_task_end(domain);
#endif
						if (bln)
						{
#ifdef CUSTOM_TA
							__itt_task_begin(domain, __itt_null, __itt_null, shSecondCheckPart2);
#endif
							run_in_parallel(
								[&ImInt, &ImForward, ddl1_ofset, ddl2_ofset, w, h] {
									IntersectImages(ImInt, ImForward, ddl1_ofset + 1, ddl2_ofset - 1, w, h);
								},
								[&ImYInt, &ImYForward, ddl1_ofset, ddl2_ofset, w, h] {
									if (g_use_ILA_images_for_search_subtitles)
									{
										IntersectYImages(ImYInt, ImYForward, ddl1_ofset + 1, ddl2_ofset - 1, w, h);
									}
								});

							bln = AnalyseImage(ImInt, &ImYInt, w, h);
#ifdef CUSTOM_TA 
							__itt_task_end(domain);
#endif
						}
					}
				}
								
				if (bln)
				{
					found_sub = 1;
					fn = fn_start;

#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: found sub start: fn: %d, fn_time: %lld(%s)\n", (int)__LINE__, fn, rs.GetPos(fn), VideoTimeToStr(rs.GetPos(fn))));
#endif

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

		if ((g_RunSubSearch == 0) || (found_sub == 0) || ((CurPos >= End) && (End >= 0)) || (CurPos == prevPos))
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
			rs.GetConvertImage(fn + i, ImBGRForward[i], ImForward[i], ImNEForward[i], ImYForward[i], PosForward[i]);
		}

#ifdef CUSTOM_TA
		__itt_task_begin(domain, __itt_null, __itt_null, shSubFound);
#endif

		pImBGR = ImBGRForward[0];
		pImNE = ImNEForward[0];
		pImY = ImYForward[0];
		pIm = ImForward[0];

		prevPos = rs.GetPos(fn - 1);
		CurPos = PosForward[0];

#ifdef CUSTOM_DEBUG
		{
			SaveGreyscaleImage(*pImInt, wxString("/DebugImages/FastSearchSubtitles_pImInt_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
			SaveBinaryImage(*pImYInt, wxString("/DebugImages/FastSearchSubtitles_pImYInt_") + VideoTimeToStr(CurPos) + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);

			for (int i = ((fn == fn_start) ? 0 : DL - 1); i < DL; i++)
			{					
				SaveGreyscaleImage(*(ImForward[i]), wxString("/DebugImages/FastSearchSubtitles_ImForward_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
				SaveGreyscaleImage(*(ImNEForward[i]), wxString("/DebugImages/FastSearchSubtitles_ImNEForward_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
				SaveBinaryImage(*(ImYForward[i]), wxString("/DebugImages/FastSearchSubtitles_ImYForward_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
				SaveBGRImage(*(ImBGRForward[i]), wxString("/DebugImages/FastSearchSubtitles_ImBGRForward_") + VideoTimeToStr(PosForward[i]) + "_fn" + std::to_string(fn + i) + "_line" + std::to_string(__LINE__) + g_im_save_format, w, h);
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
			ImIntS = *pImInt;
			ImNES = *pImNE;
			ImYS = *pImYInt;
			ImFS = *pImBGR;
			
#ifdef CUSTOM_DEBUG
			SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): CurPos(%lld): fn(%d) == bf(%d)\n", (int)__LINE__, fn, CurPos, fn, bf));
#endif

#ifdef CUSTOM_DEBUG
			{
				SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
				SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
				SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
			}
#endif
		}

		if (fn > ef)
		{
#ifdef CUSTOM_DEBUG
			SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): CurPos(%lld): fn(%d) > ef(%d)\n", (int)__LINE__, fn, CurPos, fn, ef));
#endif

			if ((bln > 0) && (CurPos != prevPos))
			{
#ifdef CUSTOM_DEBUG
				SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && (CurPos(%lld) != prevPos(%lld))\n", (int)__LINE__, fn, CurPos, prevPos));
#endif

				if (bf == -2)
				{
#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf == -2\n", (int)__LINE__, fn));
#endif

					bf = fn;
					ef = bf;
					bt = CurPos;

#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): set: bf(%d), ef(%d), bt(%lld) [(bln(GetIntersectImages) > 0) && bf == -2]\n", (int)__LINE__, fn, bf, ef, CurPos));
#endif

					ImIntS = *pImInt;
					ImNES = *pImNE;
					ImYS = *pImYInt;
					ImFS = *pImBGR;
					
#ifdef CUSTOM_DEBUG
					{
						SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
						SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
						SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
						SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						bt = bt;
					}
#endif
				}
				else
				{
#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf(%d) != -2\n", (int)__LINE__, fn, bf));
#endif

					bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, prevImNE, *pImInt, pImYInt, *pImNE, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));
					
					if ((bln > 0) && (fn - bf + 1 == 3))
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf != -2 && (bln(CompareTwoSubsOptimal(ImIntS..pImInt)) > 0) && ((fn - bf + 1 == 3)||(fn - bf + 1 == DL(%d)))\n", (int)__LINE__, fn, DL));
#endif

						ImFS = *pImBGR;
						ImNES = *pImNE;						
						ImIntS = *pImInt;
						ImYS = *pImYInt;

#ifdef CUSTOM_DEBUG
						{
							SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif						
					}

					if (bln == 0)
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0\n", (int)__LINE__, fn));
#endif
#ifdef CUSTOM_DEBUG
						{
							SaveBGRImage(*pImBGR, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImBGR" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImNE, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNE" + g_im_save_format, w, h);
							SaveGreyscaleImage(*pImInt, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImInt" + g_im_save_format, w, h);
							SaveBinaryImage(*pImYInt, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYInt" + g_im_save_format, w, h);

							SaveBGRImage(prevImBGR, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImBGRprev" + g_im_save_format, w, h);
							SaveGreyscaleImage(prevImNE, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNEprev" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif

						if (finded_prev == 1)
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && finded_prev == 1\n", (int)__LINE__, fn));
#endif

							cmp_prev = CompareTwoSubsOptimal(ImIntSP, &ImYSP, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));

#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): cmp_prev(%d)(CompareTwoSubsOptimal(ImIntSP..ImIntS))  (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && finded_prev == 1\n", (int)__LINE__, fn, cmp_prev));
#endif

							if (cmp_prev == 0)
							{
#ifdef CUSTOM_DEBUG
								{
									SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImS" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);

									SaveBGRImage(ImFSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNESP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
								}
#endif

								if (AnalizeImageForSubPresence(ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H, 0, w - 1) == 1)
								{
#ifdef CUSTOM_DEBUG
									SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): saving: ImFSP [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && finded_prev == 1 &&  cmp_prev(CompareTwoSubsOptimal(ImIntSP..ImIntS)) == 0 && AnalizeImageForSubPresence(ImIntSP) == 1]\n", (int)__LINE__, fn));
#endif

									Str = VideoTimeToStr(pbt) + wxT("__") + VideoTimeToStr(pet) + wxT("_") + FormatImInfoAddData(W, H, xmin, ymin, w, h);
									rs.AddSaveImagesTask(ImFSP, ImIntSP, ImYSP, Str);
								}

#ifdef CUSTOM_DEBUG
								SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: pbf(%d) = bf(%d), pbt(%lld) = bt(%lld) (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && finded_prev == 1 && cmp_prev(CompareTwoSubsOptimal(ImIntSP..ImIntS)) == 0\n", (int)__LINE__, fn, pbf, bf, pbt, bt));
#endif

								pbf = bf;
								pbt = bt;
							}
						}
						else
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: pbf(%d) = bf(%d), pbt(%lld) = bt(%lld) (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && finded_prev != 1\n", (int)__LINE__, fn, pbf, bf, pbt, bt));
#endif

							pbf = bf;
							pbt = bt;							
						}

						pef = fn - 1;
						pet = PosForward[0] - 1;

						int offset = 0;

#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): bf(%d) [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0]\n", (int)__LINE__, fn, bf));
#endif
						if (fn > bf + 1)
						{
#ifdef CUSTOM_DEBUG
							{
								SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
								SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							}
#endif

							offset = FindOffsetForNewSub(ImForward, ImYForward, ImNEForward, ImIntS, ImYS, ImNES, prevImNE, w, h, W, H, 0, w - 1, fn);

							pef = fn + offset - 1;
							pet = PosForward[offset] - 1;

#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: pef(%d) = fn(%d) + offset(%d) - 1 [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && fn(%d) > bf(%d) + 1]\n", (int)__LINE__, fn, pef, fn, offset,  fn, bf));
#endif
						}

						if (pef - pbf + 1 >= DL)
						{
							if (!((finded_prev == 1) && (cmp_prev == 1)))
							{								
								ImIntSP = ImIntS;
								ImFSP = ImFS;
								ImNESP = ImNES;
								ImYSP = ImYS;

#ifdef CUSTOM_DEBUG
								{
									SaveBGRImage(ImFSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImIntSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntSP" + g_im_save_format, w, h);
									SaveGreyscaleImage(ImNESP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
								}
#endif
							}
							finded_prev = 1;							

#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: finded_prev = 1, ImFSP = ImFS [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && pef(%d) - pbf(%d) + 1 >= DL(%d)]\n", (int)__LINE__, fn, pef, pbf, DL));
#endif
						}
						else
						{
							finded_prev = 0;

#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: finded_prev = 0 [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0 && pef(%d) - pbf(%d) + 1 < DL(%d)]\n", (int)__LINE__, fn, pef, pbf, DL));
#endif
						}

						bf = fn + offset;
						ef = bf;
						bt = PosForward[offset];
						
						ImNES = *ImNEForward[offset];
						ImFS = *ImBGRForward[offset];

#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: bf(%d) = fn(%d) + offset(%d), ImFS = *ImBGRForward[offset] [(bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) == 0]\n", (int)__LINE__, fn, bf, fn, offset));
#endif

						if (offset == 0)
						{
							ImIntS = *pImInt;
							ImYS = *pImYInt;
						}
						else
						{
							ImIntS = *ImForward[offset];
							for (int i = 0; i < w*h; i++)
							{
								ImYS[i] = (u16)((*ImYForward[offset])[i]) + (u16)255;
							}
						}

#ifdef CUSTOM_DEBUG
						{
							SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
							SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
						}
#endif
					}
					else
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (bln(GetIntersectImages) > 0) && bf != -2 && bln(CompareTwoSubsOptimal(ImIntS..pImInt)) != 0\n", (int)__LINE__, fn));
#endif
						IntersectYImages(ImYS, *pImY, w, h);
					}
				}
			}
			else if (((bln == 0) && (CurPos != prevPos)) ||
				((bln == 1) && (CurPos == prevPos)))
			{
#ifdef CUSTOM_DEBUG
				SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(%d)(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(%d)(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld))))\n", (int)__LINE__, fn, bln, CurPos, prevPos, bln, CurPos, prevPos));
#endif
				if (finded_prev == 1)
				{
#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && finded_prev == 1\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos));
#endif

					bln = CompareTwoSubsOptimal(ImIntSP, &ImYSP, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));

					if (bln == 1)
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && finded_prev == 1 && bln(CompareTwoSubsOptimal(ImIntSP..ImIntS) == 1\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos));
#endif

						bf = pbf;
						ef = bf;
						bt = pbt;
						finded_prev = 0;
					}
				}

				if (bf != -2)
				{
#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos));
#endif

					if (CurPos != prevPos)
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && CurPos != prevPos\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos));
#endif

						int offset = 0;
						simple_buffer<u8> *pPrevImNE = &prevImNE;

						for (offset = 0; offset < DL - 1; offset++)
						{
							bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, *pPrevImNE, ImIntS, &ImYS, *(ImNEForward[offset]), w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_offset" + std::to_string(offset)));

							if (bln == 0)
							{
								simple_buffer<u8> ImNEFF(*(ImNEForward[offset]));
								IntersectTwoImages(ImNEFF, ImNES, w, h);
								bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNES, *pPrevImNE, ImIntS, &ImYS, ImNEFF, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_offset" + std::to_string(offset)));
							}

							if (bln == 0)
							{
								break;
							}

							// note: don't intersect fist DL and last DL frames in ImIL

							pPrevImNE = ImNEForward[offset];
						}

						ef = fn + offset - 1;
						et = PosForward[offset] - 1; // using 0 instead of offset (there can be intersected sequential subs)

#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: ef(%d) = fn(%d) + offset(%d) - 1, et(%lld) = PosForward[offset] - 1  [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && CurPos != prevPos]\n", (int)__LINE__, fn, ef, fn, offset, et, CurPos, prevPos, CurPos, prevPos));
#endif
					}
					else
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && CurPos == prevPos\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos));
#endif

						ef = fn-1;
						et = CurPos;
					}

					if (ef - bf + 1 < DL)
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 < DL(%d)\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif
						if (finded_prev == 1)
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 < DL(%d) && finded_prev == 1\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif
							bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNESP, ImNESP, ImIntS, &ImYS, ImNES, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));

							if (bln == 0)
							{
								simple_buffer<u8> ImNESF(ImNES);
								IntersectTwoImages(ImNESF, ImNESP, w, h);
								bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNESP, ImNESP, ImIntS, &ImYS, ImNESF, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));

								if (bln == 0)
								{
									simple_buffer<u8> ImNESPF(ImNESP);
									IntersectTwoImages(ImNESPF, ImNES, w, h);									
									bln = CompareTwoSubsOptimal(ImIntS, &ImYS, ImNESPF, ImNESPF, ImIntS, &ImYS, ImNES, w, h, W, H, 0, w - 1, wxDEBUG_DET("FastSearchSubtitles_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__)));
								}
							}

							if (bln == 1)
							{
#ifdef CUSTOM_DEBUG
								SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): bf(%d) = pbf(%d) [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 < DL(%d) && finded_prev == 1 && bln(CompareTwoSubsOptimal(ImNESP..ImNES)) == 1]\n", (int)__LINE__, fn, bf, pbf, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif

								bf = pbf;
								bt = pbt;
							}
						}
					}

					if ((finded_prev == 1) && (bf != pbf))
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ((finded_prev == 1) && (bf(%d) != pbf(%d)))]\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, bf, pbf));
#endif

#ifdef CUSTOM_DEBUG
						{
							SaveBGRImage(ImFSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImIntSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntSP" + g_im_save_format, w, h);
							SaveGreyscaleImage(ImNESP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
							SaveBinaryImage(ImYSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYSP" + g_im_save_format, w, h);
						}
#endif

						if (AnalizeImageForSubPresence(ImNESP, ImIntSP, ImYSP, pbt, pbf, w, h, W, H, 0, w - 1) == 1)
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): saving: ImFSP pbt:(%lld) pet:(%lld) [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && finded_prev == 1 && (bf(%d) != pbf(%d))) && AnalizeImageForSubPresence(ImIntSP) == 1]\n", (int)__LINE__, fn, pbt, pet, CurPos, prevPos, CurPos, prevPos));
#endif

							Str = VideoTimeToStr(pbt) + wxT("__") + VideoTimeToStr(pet) + wxT("_") + FormatImInfoAddData(W, H, xmin, ymin, w, h);
							rs.AddSaveImagesTask(ImFSP, ImIntSP, ImYSP, Str);
						}
					}

					if (ef - bf + 1 >= DL)
					{
						if (bf != pbf)
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 >= DL(%d) && bf != pbf\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif
#ifdef CUSTOM_DEBUG
							{
								SaveBGRImage(ImFS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFS" + g_im_save_format, w, h);
								SaveGreyscaleImage(ImNES, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNES" + g_im_save_format, w, h);
								SaveGreyscaleImage(ImIntS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntS" + g_im_save_format, w, h);
								SaveBinaryImage(ImYS, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYS" + g_im_save_format, w, h);
								SaveBGRImage(*pImBGR, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pImBGR" + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImInt, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pIm" + g_im_save_format, w, h);
							}
#endif

							if (AnalizeImageForSubPresence(ImNES, ImIntS, ImYS, bt, bf, w, h, W, H, 0, w - 1) == 1)
							{
#ifdef CUSTOM_DEBUG
								SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): saving: ImFS [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 >= DL(%d) && bf != pbf && AnalizeImageForSubPresence(ImIntS) == 1]\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif

								Str = VideoTimeToStr(bt) + wxT("__") + VideoTimeToStr(et) + wxT("_") + FormatImInfoAddData(W, H, xmin, ymin, w, h);
								rs.AddSaveImagesTask(ImFS, ImIntS, ImYS, Str);
							}
						}
						else
						{
#ifdef CUSTOM_DEBUG
							SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 >= DL(%d) && bf == pbf\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif
#ifdef CUSTOM_DEBUG
							{
								SaveBGRImage(ImFSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImFSP" + g_im_save_format, w, h);
								SaveGreyscaleImage(ImNESP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImNESP" + g_im_save_format, w, h);
								SaveGreyscaleImage(ImIntSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImIntSP" + g_im_save_format, w, h);
								SaveBinaryImage(ImYSP, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_ImYSP" + g_im_save_format, w, h);
								SaveBGRImage(*pImBGR, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pImBGR" + g_im_save_format, w, h);
								SaveGreyscaleImage(*pImInt, wxString("/DebugImages/FastSearchSubtitles") + "_fn" + std::to_string(fn) + "_line" + std::to_string(__LINE__) + "_pIm" + g_im_save_format, w, h);
							}
#endif

							if (AnalizeImageForSubPresence(ImNESP, ImIntSP, ImYSP, bt, bf, w, h, W, H, 0, w - 1) == 1)
							{
#ifdef CUSTOM_DEBUG
								SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): saving: ImFSP [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && bf != -2 && ef(%d) - bf(%d) + 1 >= DL(%d) && bf == pbf && AnalizeImageForSubPresence(ImIntSP) == 1]\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, ef, bf, DL));
#endif

								Str = VideoTimeToStr(bt) + wxT("__") + VideoTimeToStr(et) + wxT("_") + FormatImInfoAddData(W, H, xmin, ymin, w, h);
								rs.AddSaveImagesTask(ImFSP, ImIntSP, ImYSP, Str);
							}
						}
					}
				}

				finded_prev = 0;
				bf = -2;

				if (fn > ef)
				{
#ifdef CUSTOM_DEBUG
					SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): (((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && fn(%d) > ef(%d)\n", (int)__LINE__, fn, CurPos, prevPos, CurPos, prevPos, fn, ef));
#endif

					if ((fn - fn_start) >= DL)
					{
#ifdef CUSTOM_DEBUG
						SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): seting: fn_start(%d) = fn(%d) [(((bln(GetIntersectImages) == 0) && (CurPos(%lld) != prevPos(%lld))) || ((bln(GetIntersectImages) == 1) && (CurPos(%lld) == prevPos(%lld)))) && fn(%d) > ef(%d) && (fn(%d) - fn_start(%d)) >= DL(%d)]\n", (int)__LINE__, fn, fn_start, fn, CurPos, prevPos, CurPos, prevPos, fn, ef, fn, fn_start, DL));
#endif

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
			prevImBGR = *(pImBGR);
			prevImNE = *(pImNE);
			prevIm = *(pIm);

			fn++;
			rs.ShiftStartFrameNumberTo(fn);
		}

#ifdef CUSTOM_TA 
		__itt_task_end(domain);
#endif
	}

	if (finded_prev == 1)
	{
		if (AnalizeImageForSubPresence(ImNESP, ImIntSP, ImYSP, pbt, bf, w, h, W, H, 0, w - 1) == 1)
		{
#ifdef CUSTOM_DEBUG
			SaveToReportLog(wxString::Format("FastSearchSubtitles [line: %d]: fn(%d): finded_prev == 1 saving: ImFSP\n", (int)__LINE__, fn));
#endif

			Str = VideoTimeToStr(pbt) + wxT("__") + VideoTimeToStr(pet) + wxT("_") + FormatImInfoAddData(W, H, xmin, ymin, w, h);
			rs.AddSaveImagesTask(ImFSP, ImIntSP, ImYSP, Str);
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

	return Begin;
}

int AnalyseImage(simple_buffer<u8>& Im, simple_buffer<u16>* pImILA, int w, int h)
{
	int i, k, l, x, y, ia, da, pl, mpl, i_mpl, len, len2, val1, val2, n, bln;
	int segh, mtl;	
	double tp;
	simple_buffer<u8> ImRes(Im);
	
	if ((g_use_ILA_images_for_search_subtitles) && (pImILA != NULL))
	{
		IntersectTwoImages(ImRes, *pImILA, w, h);
	}

	segh = g_segh;
	tp = g_tp;
	mtl = (int)(g_mtpl*(double)w);
	
	custom_assert(segh > 0, "AnalyseImage: segh > 0");
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
	
	if (g_text_alignment != TextAlignment::Any)
	{
		while (l > 0)
		{
			if (len < mtl) return 0;

			len2 = g_le[k][l] - g_lb[k][0] + 1;

			custom_assert(len2 > 0, "AnalyseImage: len2 > 0");
			if ((double)len / (double)len2 >= tp) return 1;

			if (g_text_alignment == TextAlignment::Center)
			{
				if (g_lb[k][0] * 2 >= w) return 0;

				val1 = (g_le[k][l - 1] + g_lb[k][0] + 1) - w;
				if (val1 < 0) val1 = -val1;

				val2 = (g_le[k][l] + g_lb[k][1] + 1) - w;
				if (val2 < 0) val2 = -val2;

				if (val1 <= val2)
				{
					len -= g_le[k][l] - g_lb[k][l] + 1;
				}
				else
				{
					len -= g_le[k][0] - g_lb[k][0] + 1;

					for (i = 0; i < l; i++)
					{
						g_lb[k][i] = g_lb[k][i + 1];
						g_le[k][i] = g_le[k][i + 1];
					}
				}
			}
			else if (g_text_alignment == TextAlignment::Left)
			{
				len -= g_le[k][l] - g_lb[k][l] + 1;
			}
			else
			{
				len -= g_le[k][0] - g_lb[k][0] + 1;

				for (i = 0; i < l; i++)
				{
					g_lb[k][i] = g_lb[k][i + 1];
					g_le[k][i] = g_le[k][i + 1];
				}
			}

			l--;
		};
	}
	
	if (len > mtl)
	{
		if ((g_lb[k][0] * 2 < w) || (g_text_alignment != TextAlignment::Center))
		{
			return 1;
		}
	}

	return 0;
}

template <class T>
int GetLinesInfo(simple_buffer<T> &ImRES, simple_buffer<int> &lb, simple_buffer<int> &le, int w, int h) // return ln
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

int DifficultCompareTwoSubs2(simple_buffer<u8>& ImF1, simple_buffer<u16>* pImILA1, simple_buffer<u8>& ImNE11, simple_buffer<u8>& ImNE12, simple_buffer<u8>& ImF2, simple_buffer<u16>* pImILA2, simple_buffer<u8>& ImNE2, int w, int h, int W, int H, int min_x, int max_x, wxString iter_det)
{
	simple_buffer<u8> ImFF1(ImF1), ImFF2(ImF2);
	simple_buffer<u8> ImRES(w*h, (u8)0);
	simple_buffer<int> lb(h, 0), le(h, 0);
	int res, ln;		

	if (g_use_ILA_images_for_search_subtitles)
	{		
		if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/DifficultCompareTwoSubs2_01_1_ImFF1" + g_im_save_format, w, h);
		if (pImILA1 != NULL)
		{
			if (g_show_results) SaveBinaryImage(*pImILA1, "/DebugImages/DifficultCompareTwoSubs2_01_2_ImILA1" + g_im_save_format, w, h);
			IntersectTwoImages(ImFF1, *pImILA1, w, h);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/DifficultCompareTwoSubs2_01_3_ImFF1IntImILA1" + g_im_save_format, w, h);
		}

		if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/DifficultCompareTwoSubs2_01_4_ImFF2" + g_im_save_format, w, h);
		if (pImILA2 != NULL)
		{
			if (g_show_results) SaveBinaryImage(*pImILA2, "/DebugImages/DifficultCompareTwoSubs2_01_5_ImILA2" + g_im_save_format, w, h);
			IntersectTwoImages(ImFF2, *pImILA2, w, h);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/DifficultCompareTwoSubs2_01_6_ImFF2IntImILA2" + g_im_save_format, w, h);
		}
	}

	auto filter_image = [&w, &h](simple_buffer<u8> &ImFF) {
		simple_buffer<int> lb(h, 0), le(h, 0);
		int ln;
		ln = GetLinesInfo(ImFF, lb, le, w, h);
		for (int l = 0; l < ln; l++)
		{
			int hh = (le[l] - lb[l] + 1);
			simple_buffer<u8> SubIm(w*hh, (u8)0);
			SubIm.copy_data(ImFF, 0, w*lb[l], w*hh);
			if (AnalyseImage(SubIm, NULL, w, hh) == 0)
			{
				ImFF.set_values(0, w * lb[l], w*hh);
			}
		}
	};

	AddTwoImages(ImF1, ImF2, ImRES, w*h);
	ln = GetLinesInfo(ImRES, lb, le, w, h);
	
	run_in_parallel(
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/DifficultCompareTwoSubs2_02_1_ImFF1" + g_im_save_format, w, h);
			FilterImage(ImFF1, ImNE11, w, h, W, H, min_x, max_x, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/DifficultCompareTwoSubs2_02_2_ImFF1F" + g_im_save_format, w, h);
			filter_image(ImFF1);
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/DifficultCompareTwoSubs2_02_3_ImFF1FF" + g_im_save_format, w, h);
		},
		[&] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/DifficultCompareTwoSubs2_03_1_ImFF2" + g_im_save_format, w, h);
			FilterImage(ImFF2, ImNE2, w, h, W, H, min_x, max_x, lb, le, ln);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/DifficultCompareTwoSubs2_03_2_ImFF2F" + g_im_save_format, w, h);
			filter_image(ImFF2);
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/DifficultCompareTwoSubs2_03_3_ImFF2FF" + g_im_save_format, w, h);
		}
	);	

	res = CompareTwoSubs(ImFF1, pImILA1, ImNE11, ImNE12, ImFF2, pImILA2, ImNE2, w, h, W, H, wxDEBUG_DET(iter_det + "_DifficultCompareTwoSubs2"));

	return res;
}

int CompareTwoSubs(simple_buffer<u8>& Im1, simple_buffer<u16>* pImILA1, simple_buffer<u8>& ImVE11, simple_buffer<u8>& ImVE12, simple_buffer<u8>& Im2, simple_buffer<u16>* pImILA2, simple_buffer<u8>& ImVE2, int w, int h, int W, int H, wxString iter_det)
{
	simple_buffer<u8> ImRES(w * h, (u8)0);
	simple_buffer<u8> ImFF1, ImFF2;
	simple_buffer<int> lb(h, 0), le(h, 0);
	int i, ib, ie, k, y, l, ln, bln, val1, val2, val3, segh, dn;	
	double veple, ilaple;

	veple = g_veple;
	ilaple = g_ilaple;
	segh = g_segh;

#ifdef CUSTOM_DEBUG
	{
		SaveGreyscaleImage(Im1, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1" + g_im_save_format, w, h);
		SaveGreyscaleImage(Im2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im2" + g_im_save_format, w, h);
		
		if (pImILA1 != NULL) SaveBinaryImage(*pImILA1, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImILA1" + g_im_save_format, w, h);
		if (pImILA2 != NULL) SaveBinaryImage(*pImILA2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImILA2" + g_im_save_format, w, h);

		SaveGreyscaleImage(ImVE11, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImVE11" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImVE12, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImVE12" + g_im_save_format, w, h);
		SaveGreyscaleImage(ImVE2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImVE2" + g_im_save_format, w, h);
	}
#endif

	if (g_use_ILA_images_for_search_subtitles && ((pImILA1 != NULL) || (pImILA2 != NULL)))
	{
		ImFF1 = Im1;
		ImFF2 = Im2;

		run_in_parallel(
			[&ImFF1, pImILA1, w, h, &iter_det] {
			if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/CompareTwoSubs_01_01_01_ImFF1" + g_im_save_format, w, h);
			if (pImILA1 != NULL)
			{
				if (g_show_results) SaveBinaryImage(*pImILA1, "/DebugImages/CompareTwoSubs_01_01_02_ImILA1" + g_im_save_format, w, h);
				IntersectTwoImages(ImFF1, *pImILA1, w, h);
				if (g_show_results) SaveGreyscaleImage(ImFF1, "/DebugImages/CompareTwoSubs_01_01_03_ImFF1IntImILA1" + g_im_save_format, w, h);

#ifdef CUSTOM_DEBUG
				{
					SaveGreyscaleImage(ImFF1, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1IntImILA1" + g_im_save_format, w, h);
				}
#endif
			}
		},
			[&ImFF2, pImILA2, w, h, &iter_det] {
			if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/CompareTwoSubs_01_02_01_ImFF2" + g_im_save_format, w, h);
			if (pImILA2 != NULL)
			{
				if (g_show_results) SaveBinaryImage(*pImILA2, "/DebugImages/CompareTwoSubs_01_02_02_ImILA2" + g_im_save_format, w, h);
				IntersectTwoImages(ImFF2, *pImILA2, w, h);
				if (g_show_results) SaveGreyscaleImage(ImFF2, "/DebugImages/CompareTwoSubs_01_02_03_ImFF2IntImILA2" + g_im_save_format, w, h);

#ifdef CUSTOM_DEBUG
				{
					SaveGreyscaleImage(ImFF2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im2IntImILA2" + g_im_save_format, w, h);
				}
#endif
			}
		} );

		AddTwoImages(ImFF1, ImFF2, ImRES, w*h);
		if (g_show_results) SaveGreyscaleImage(ImRES, "/DebugImages/CompareTwoSubs_01_03_ImRES" + g_im_save_format, w, h);
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

	auto compare = [&ImRES, &lb, &le, ln, w, veple](simple_buffer<u8> &ImVE1, simple_buffer<u8> &ImVE2, double &max_dif) {
		int bln = 1, k, i, ib, ie, dif, dif1, dif2, cmb, val1, val2;
		double cur_dif;
		
		max_dif = 0;

		for (k = 0; k < ln; k++)
		{
			ib = (lb[k] + 1)*w;
			ie = (le[k] - 1)*w;			

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
				max_dif = 10;
				bln = 0;
				break;
			}

			cur_dif = (double)dif / (double)cmb;
			max_dif = std::max<double>(max_dif, cur_dif);

			if (cur_dif > veple)
			{
				bln = 0;
				break;
			}
		}

		return bln;
	};

	auto compare2 = [&lb, &le, ln, w, ilaple](simple_buffer<u8>& Im1, simple_buffer<u8>& Im2, double& max_dif) {
		int bln = 1, k, i, ib, ie, val1 = 0, val2 = 0, dif1, dif2, cmb;
		double dif;
		
		max_dif = 0;

		for (k = 0; k < ln; k++)
		{
			ib = (lb[k] + 1) * w;
			ie = (le[k] - 1) * w;

			dif1 = 0;
			dif2 = 0;
			cmb = 0;

			for (i = ib; i < ie; i++)
			{
				val1 = Im1[i];
				val2 = Im2[i];

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

			if (dif2 > dif1) dif = dif2;
			else dif = dif1;

			if (cmb == 0)
			{
				max_dif = 10;
				bln = 0;
				break;
			}

			dif = (double)dif / (double)cmb;
			max_dif = std::max<double>(max_dif, dif);

			if (dif > ilaple)
			{
				bln = 0;
				break;
			}
		}

		return bln;
	};

	double min_dif, dif1, dif2;

	run_in_parallel(
		[&ImVE11, &ImVE2, &val1, &dif1, &compare] {
		val1 = compare(ImVE11, ImVE2, dif1);
	},
		[&ImVE11, &ImVE12, &ImVE2, &val2, &dif2, &compare] {
		if (ImVE11.m_pData != ImVE12.m_pData)
		{
			val2 = compare(ImVE12, ImVE2, dif2);
		}
		else
		{
			val2 = 0;
		}
	},
		[&ImFF1, pImILA1, pImILA2, &ImVE11, &ImVE12, &ImVE2, &val3, w, h, &compare2, &iter_det] {
		if (g_use_ILA_images_for_search_subtitles && (pImILA1 != NULL) && (pImILA2 != NULL))
		{
			simple_buffer<u8> Im1(ImFF1), Im2;
			simple_buffer<u16> ImILAInt(*pImILA1);
			double dif3;

			IntersectTwoImages(Im1, ImVE11, w, h);

			if (ImVE11.m_pData != ImVE12.m_pData)
			{
				IntersectTwoImages(Im1, ImVE12, w, h);
			}
			
			Im2 = Im1;
			
			IntersectYImages(ImILAInt, *pImILA2, w, h);

			IntersectTwoImages(Im2, ImILAInt, w, h);
			
#ifdef CUSTOM_DEBUG
			{
				SaveBinaryImage(ImILAInt, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_ImILAInt" + g_im_save_format, w, h);
				SaveGreyscaleImage(Im2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1IntILA1VE11VE12ILAInt" + g_im_save_format, w, h);
			}
#endif

			{
				cv::Mat cv_im_gr;
				BinaryImageToMat(Im2, w, h, cv_im_gr);
				cv::dilate(cv_im_gr, cv_im_gr, cv::Mat(), cv::Point(-1, -1), 1);
				BinaryMatToImage(cv_im_gr, w, h, Im2, (u8)255);
				IntersectTwoImages(Im2, Im1, w, h);
			}

#ifdef CUSTOM_DEBUG
			{
				SaveGreyscaleImage(Im2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1IntILA1VE11VE12ILAIntExt" + g_im_save_format, w, h);
			}
#endif

			IntersectTwoImages(Im2, ImVE2, w, h);

			val3 = compare2(Im2, Im1, dif3);

#ifdef CUSTOM_DEBUG
			{
				SaveGreyscaleImage(Im1, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1IntILA1VE11VE12_cmp_maxdif" + std::to_string(dif3) + "_res" + std::to_string(val3) + g_im_save_format, w, h);
				SaveGreyscaleImage(Im2, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im1IntILA1VE11VE12ILAIntExtVE2_cmp_maxdif" + std::to_string(dif3) + "_res" + std::to_string(val3) + g_im_save_format, w, h);
			}
#endif
		}
		else
		{
			val3 = 1;
		}
	} );

	bln = (val1 | val2) & val3;

	min_dif = (ImVE11.m_pData != ImVE12.m_pData) ? std::min<double>(dif1, dif2) : dif1;

#ifdef CUSTOM_DEBUG
	{
		std::vector<std::pair<int, int>> HLPC, VLPC;
		int yc = GetBGRColor(Yellow), rc = GetBGRColor(Red);

		for (k = 0; k < ln; k++)
		{
			HLPC.push_back(std::pair<int, int>{lb[k], yc});
			HLPC.push_back(std::pair<int, int>{le[k], rc});
		}

		SaveGreyscaleImageWithLinesInfo(ImRES, wxString("/DebugImages/CompareTwoSubs_") + iter_det + "_Im2+1FByImILA" + "_ln" + std::to_string(ln) + "_minmaxdif" + std::to_string(min_dif) + "_bln" + std::to_string(bln) + g_im_save_format, w, h, HLPC, VLPC);
	}
#endif

	return bln;
}

int CompareTwoSubsOptimal(simple_buffer<u8>& Im1, simple_buffer<u16>* pImILA1, simple_buffer<u8>& ImVE11, simple_buffer<u8>& ImVE12, simple_buffer<u8>& Im2, simple_buffer<u16>* pImILA2, simple_buffer<u8>& ImVE2, int w, int h, int W, int H, int min_x, int max_x, wxString iter_det)
{
	int bln = 0;
	
	if (bln == 0)
	{
		bln = CompareTwoSubs(Im1, pImILA1, ImVE11, ImVE12, Im2, pImILA2, ImVE2, w, h, W, H, wxDEBUG_DET(iter_det + "_CompareTwoSubsOptimal"));
	}

	if (bln == 0)
	{
		bln = DifficultCompareTwoSubs2(Im1, pImILA1, ImVE11, ImVE12, Im2, pImILA2, ImVE2, w, h, W, H, min_x, max_x, wxDEBUG_DET(iter_det + "_CompareTwoSubsOptimal"));
	}

	return bln;
}

template <class T>
void AddTwoImages(simple_buffer<T>& Im1, simple_buffer<T>& Im2, simple_buffer<T>& ImRES, int size)
{	
	int i;

	ImRES.copy_data(Im1, size);

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) ImRES[i] = 255;
	}
}

template <class T>
void AddTwoImages(simple_buffer<T>& Im1, simple_buffer<T>& Im2, int size)
{
	int i;

	for(i=0; i<size; i++) 
	{
		if (Im2[i] == 255) Im1[i] = 255;
	}
}

// W - full image include scale (if is) width
// H - full image include scale (if is) height
void ImBGRToNativeSize(simple_buffer<u8>& ImBGROrig, simple_buffer<u8>& ImBGRRes, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	int i, j, x, y;

	custom_assert(ImBGRRes.m_size >= W * H * 3, "ImBGRToNativeSize: Im.m_size >= W*H");

	ImBGRRes.set_values(255, W * H * 3);

	i = 0;
	j = ymin * W + xmin;

	for (y = 0; y < h; y++)
	{
		ImBGRRes.copy_data(ImBGROrig, j * 3, i * 3, w * 3);
		i += w;
		j += W;
	}
}

wxString VideoTimeToStr(s64 pos)
{
	wxString str;
	int hour, min, sec, msec, val;

	val = (int)(pos / 1000); // seconds
	msec = pos - ((s64)val * (s64)1000);
	hour = val / 3600;
	val -= hour * 3600;
	min = val / 60;
	val -= min * 60;
	sec = val;

	str.Printf(wxT("%d_%02d_%02d_%03d"), hour, min, sec, msec);

	return str;
}

s64 GetVideoTime(wxString time)
{
	s64 res;
	int hour, min, sec, msec;

	if (time == wxT("N/A"))
	{
		res = -1;
	}
	else
	{
		wxASSERT_MSG(sscanf(time.c_str(), "%d:%d:%d:%d", &hour, &min, &sec, &msec) == 4, wxString::Format(wxT("Wrong video time format '%s'"), time.c_str()));
		res = (s64)(((hour * 60 + min) * 60 + sec) * 1000 + msec);
	}
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

	wxRegEx re(wxT("([^\\\\\\/]+)\\.[^\\\\\\/\\.]+$"));
	if (re.Matches(FilePath))
	{
		res = re.GetMatch(FilePath, 1);
	}

	return res;
}

wxString GetFileExtension(wxString FilePath)
{
	wxString res;

	wxRegEx re(wxT("[^\\\\\\/]+\\.([^\\\\\\/\\.]+)$"));
	if (re.Matches(FilePath))
	{
		res = re.GetMatch(FilePath, 1);
	}

	return res;
}

wxString GetFileNameWithExtension(wxString FilePath)
{
	wxString res;

	wxRegEx re(wxT("([^\\\\\\/]+\\.[^\\\\\\/\\.]+)$"));
	if (re.Matches(FilePath))
	{
		res = re.GetMatch(FilePath, 1);
	}

	return res;
}

wxString GetFileDir(wxString FilePath)
{
	wxString res;

	wxRegEx re(wxT("(^.+[\\\\\\/])"));
	if (re.Matches(FilePath))
	{
		res = re.GetMatch(FilePath, 1);
	}

	return res;
}
