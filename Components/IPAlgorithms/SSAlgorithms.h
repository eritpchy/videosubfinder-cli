                              //SSAlgorithms.h//                                
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

#include "IPAlgorithms.h"
#include "DataTypes.h"
#include "MyClosedFigure.h"
#include "Video.h"
#include <chrono>
#include <vector>
#include "ImageCropper.h"
#include <nlohmann/json.hpp>

using namespace std;

extern std::chrono::time_point<std::chrono::high_resolution_clock> g_StartTimeRunSubSearch;
extern int		g_RunSubSearch;

extern int      g_threads; // number of threads
extern int		g_DL;	 //sub frame length
extern double	g_tp;	 //text percent
extern double	g_mtpl;  //min text len (in percent)
extern double	g_veple; //vedges points line error
extern double	g_ilaple; //ILA points line error
extern bool     g_save_images;
extern vector<std::string> g_file_names_vector;

extern bool g_use_ISA_images_for_search_subtitles;
extern bool g_use_ILA_images_for_search_subtitles;
extern bool g_replace_ISA_by_filtered_version;
extern int g_max_dl_down;
extern int g_max_dl_up;

struct FrameData {
    const char* task_id;
    ImageCropper image_cropper;
    cv::Mat* cv_ImBGR;
    void* flutter_context;
    const char* name;
    const char* begin_time_str;
    const char* end_time_str;
    int64_t begin_time;
    int64_t end_time;
    int64_t video_finish_time;
    uint64_t starttime_nanoseconds;
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    int width;
    int height;
    nlohmann::json ocr_result_json;
};

typedef std::function<void(const FrameData*)> FrameFoundCallback;
extern FrameFoundCallback g_frame_found_callback;
extern void* g_flutter_context;
extern std::string g_flutter_task_id;
extern uint64_t g_starttime_nanoseconds;


s64 FastSearchSubtitles(CVideo *pV, s64 Begin, s64 End);

int AnalyseImage(simple_buffer<u8> &Im, simple_buffer<u16> *pImILA, int w, int h);

int CompareTwoSubs(simple_buffer<u8> &Im1, simple_buffer<u16> *pImILA1, simple_buffer<u8> &ImVE11, simple_buffer<u8> &ImVE12, simple_buffer<u8> &Im2, simple_buffer<u16> *pImILA2, simple_buffer<u8> &ImVE2, int w, int h, int W, int H, std::string iter_det);

int DifficultCompareTwoSubs2(simple_buffer<u8> &ImF1, simple_buffer<u16> *pImILA1, simple_buffer<u8> &ImNE11, simple_buffer<u8> &ImNE12, simple_buffer<u8> &ImF2, simple_buffer<u16> *pImILA2, simple_buffer<u8> &ImNE2, int w, int h, int W, int H, int min_x, int max_x, std::string iter_det);

int CompareTwoSubsOptimal(simple_buffer<u8> &Im1, simple_buffer<u16> *pImILA1, simple_buffer<u8> &ImVE11, simple_buffer<u8> &ImVE12, simple_buffer<u8> &Im2, simple_buffer<u16> *pImILA2, simple_buffer<u8> &ImVE2, int w, int h, int W, int H, int min_x, int max_x, std::string iter_det);

template <class T>
void AddTwoImages(simple_buffer<T> &Im1, simple_buffer<T> &Im2, simple_buffer<T> &ImRES, int size);
template <class T>
void AddTwoImages(simple_buffer<T> &Im1, simple_buffer<T> &Im2, int size);

void ImBGRToNativeSize(const simple_buffer<u8> &ImBGROrig, simple_buffer<u8> &ImBGRRes, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax);

std::string VideoTimeToStr(s64 pos);
s64 GetVideoTime(std::string time);
s64 GetVideoTime(int minute, int sec, int mili_sec);



// W - full image include scale (if is) width
// H - full image include scale (if is) height
template <class T>
void ImToNativeSize(simple_buffer<T>& ImOrig, simple_buffer<T>& ImRes, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	int i, j, dj, x, y;

	custom_assert(ImRes.m_size >= W * H, "ImToNativeSize: Im.m_size >= W*H");
	memset(ImRes.m_pData, 255, W * H * sizeof(T));

	i = 0;
	j = ymin * W + xmin;
	for (y = 0; y < h; y++)
	{
		ImRes.copy_data(ImOrig, j, i, w);
		i += w;
		j += W;
	}
}


// W - full image include scale (if is) width
// H - full image include scale (if is) height
template <class T>
void ImToNativeSize(simple_buffer<T>& Im, int w, int h, int W, int H, int xmin, int xmax, int ymin, int ymax)
{
	simple_buffer<T> ImTMP(Im, 0, w*h);
	ImToNativeSize(ImTMP, Im, w, h, W, H, xmin, xmax, ymin, ymax);
}
extern std::function<void(s64, s64)> g_progress_callback;
