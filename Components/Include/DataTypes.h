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

#include <time.h>
#include <memory.h>
#include <wx/wx.h>
#include <ppl.h>
#include "opencv2/imgcodecs.hpp"

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned __int64	u64;
typedef __int64	            s64;

wxString get_add_info();

// NOTE: for debugging!!!
//#define CUSTOM_DEBUG
//#define CUSTOM_DEBUG2
//#define custom_assert(cond, msg)  if(!(cond)) { wxASSERT_MSG(cond, wxString(msg) + get_add_info()); }
//#define custom_assert(cond, msg) wxASSERT_MSG(cond, msg)

//#define CUSTOM_TA

#define my_event custom_event
//#define custom_set_started(pevent) (pevent)->set_started()

// for final release build
#define custom_assert(cond, msg)
//#define my_event concurrency::event
#define custom_set_started(pevent)

#define cvMAT cv::UMat
//#define cvMAT cv::Mat

extern bool g_use_ocl;

extern double g_video_contrast;
extern double g_video_gamma;

enum TextAlignment {
	Center,
	Left,
	Right,	
	Any
};

class custom_event : public concurrency::event
{
public:
	bool m_started = false;
	bool m_finished = false;
	bool m_need_to_skip = false;

	void set_started()
	{
		custom_assert(m_started == false, "class custom_event:set_started\nnot: m_started == false");
		custom_assert(m_finished == false, "class custom_event:set_started\nnot: m_finished == false");
		m_started = true;
	}

	void set()
	{
		custom_assert(m_started == true, "class custom_event:set\nnot: m_started == true");
		custom_assert(m_finished == false, "class custom_event:set\nnot: m_finished == false");
		m_finished = true;
		concurrency::event::set();
	}

	void reset()
	{
		custom_assert(m_started == m_finished, "class custom_event:reset\nnot: m_started == m_finished");
		m_started = false;
		m_finished = false;
		m_need_to_skip = false;
		concurrency::event::reset();
	}
};

template <typename T>
class custom_buffer
{
public:
	T *m_pData;
	int m_size;
	bool m_need_to_release;

	~custom_buffer()
	{
		if (m_need_to_release)
		{
			delete[] m_pData;
		}
		m_pData = NULL;
		m_size = 0;
		m_need_to_release = false;
	}

	custom_buffer(int size)
	{
		custom_assert(size > 0, "custom_buffer(int size): not: size > 0");

		m_pData = new T[size];
		custom_assert(m_pData != NULL, "custom_buffer<T>::custom_buffer(int size): not: m_pData != NULL");
		m_size = size;
		m_need_to_release = true;
	}

	custom_buffer(int size, T val)
	{
		custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");

		m_pData = new T[size];
		custom_assert(m_pData != NULL, "custom_buffer<T>::custom_buffer(int size, T val): not: m_pData != NULL");
		m_size = size;
		m_need_to_release = true;

		for (int i = 0; i < m_size; i++)
		{
			m_pData[i] = val;
		}
	}

	custom_buffer()
	{
		m_pData = NULL;
		m_size = 0;
		m_need_to_release = false;
	}

	custom_buffer(T *pData, int size)
	{
		m_pData = pData;
		m_size = size;
		m_need_to_release = false;
	}

	custom_buffer(const custom_buffer<T> &obj)
	{
		custom_assert(obj.m_size >= 0, "custom_buffer(const custom_buffer<T> &obj): not: obj.m_size >= 0");

		if (obj.m_size == 0)
		{
			m_pData = NULL;
			m_size = 0;
			m_need_to_release = false;
		}
		else
		{
			m_size = obj.m_size;
			m_need_to_release = obj.m_need_to_release;

			if (m_need_to_release)
			{
				m_pData = new T[m_size];
				custom_assert(m_pData != NULL, "custom_buffer(const custom_buffer<T> &obj): not: m_pData != NULL");
				for (int i = 0; i < m_size; i++)
				{
					m_pData[i] = obj.m_pData[i];
				}
			}
			else
			{
				m_pData = obj.m_pData;
			}
		}
	}

	custom_buffer<T>& operator= (const custom_buffer<T> &obj)
	{
		if (m_need_to_release)
		{
			if (obj.m_need_to_release)
			{
				if (m_size != obj.m_size)
				{
					delete[] m_pData;
					m_pData = new T[obj.m_size];
					custom_assert(m_pData != NULL, "custom_buffer<T>::operator= (const custom_buffer<T> &obj): not: m_pData != NULL");
				}
			}
			else
			{
				delete[] m_pData;
			}
		}
		else
		{
			if (obj.m_need_to_release)
			{
				m_pData = new T[obj.m_size];
				custom_assert(m_pData != NULL, "custom_buffer<T>::operator= (const custom_buffer<T> &obj): not: m_pData != NULL");
			}
		}

		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{
			for (int i = 0; i < m_size; i++)
			{
				m_pData[i] = obj.m_pData[i];
			}
		}
		else
		{
			m_pData = obj.m_pData;
		}

		return *this;
	}

	void copy_data(const custom_buffer<T>& obj, int size)
	{
		custom_assert(size >= 0, "custom_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size >= 0");
		custom_assert(size <= obj.m_size, "custom_buffer<T>::opy_data(const custom_buffer<T>& obj, int size): not: size <= obj.m_size");
		custom_assert(size <= m_size, "custom_buffer<T>::opy_data(const custom_buffer<T>& obj, int size): not: size <= m_size");

		if (size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				m_pData[i] = obj.m_pData[i];
			}
		}
	}

	void copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_dst + size - 1 <= m_size - 1");
		custom_assert(offset_src + size - 1 <= obj.m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_src + size - 1 <= src.m_size - 1");

		if (size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				m_pData[offset_dst + i] = src.m_pData[offset_src + i];
			}
		}
	}

	void copy_data(T* pData, int size)
	{
		custom_assert(size >= 0, "custom_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(size <= m_size, "custom_buffer<T>::copy_data(T* pData, int size): not: size <= m_size");		

		if (size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				m_pData[i] = pData[i];
			}
		}
	}

	void copy_data(T* pData, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= m_size - 1, "simple_buffer<T>::copy_data(T* pData, int size): not: offset_dst + size - 1 <= m_size - 1");

		if (size > 0)
		{
			for (int i = 0; i < size; i++)
			{
				m_pData[offset_dst + i] = pData[offset_src + i];
			}
		}
	}

	custom_buffer<T> get_sub_buffer(int offset)
	{
		custom_assert(offset < m_size, "get_sub_buffer(int offset): not: offset < m_size");

		return custom_buffer<T>(m_pData + offset, m_size - offset);
	}

	inline int size()
	{
		return m_size;
	}

	inline void set_size(int size)
	{
		if ((!m_need_to_release) || (m_size != size))
		{
			if (m_need_to_release)
			{
				delete[] m_pData;
			}

			custom_assert(size >= 0, "custom_buffer<T>::set_size(int size): not: size >= 0");

			if (size == 0)
			{
				m_pData = NULL;
				m_size = 0;
				m_need_to_release = false;
			}
			else
			{
				m_pData = new T[size];
				custom_assert(m_pData != NULL, "custom_buffer<T>::set_size(int size): not: m_pData != NULL");
				m_size = size;
				m_need_to_release = true;
			}
		}
	}

	inline T& operator[](int idx)
	{
		custom_assert(idx >= 0, "operator[](int idx): not: idx >= 0");
		custom_assert(idx < m_size, "operator[](int idx): not: idx < m_size");

		return m_pData[idx];
	}

	inline T get_max_value()
	{
		custom_assert(m_size > 0, "m_size is not > 0");

		T res = m_pData[0];
		for (int i = 1; i < m_size; i++)
		{
			if (m_pData[i] > res) res = m_pData[i];
		}

		return res;
	}

	inline T get_max_value(int cnt)
	{
		custom_assert(m_size > 0, "m_size is not > 0");
		custom_assert(cnt <= m_size, "cnt is not <= m_size");

		T res = m_pData[0];
		for (int i = 1; i < cnt; i++)
		{
			if (m_pData[i] > res) res = m_pData[i];
		}

		return res;
	}

	inline T get_min_value()
	{
		custom_assert(m_size > 0, "m_size is not > 0");

		T res = m_pData[0];
		for (int i = 1; i < m_size; i++)
		{
			if (m_pData[i] < res) res = m_pData[i];
		}

		return res;
	}
};

template <typename T>
class simple_buffer : public custom_buffer<T>
{
public:

	simple_buffer() : custom_buffer()
	{
	}

	simple_buffer(T* pData, int size) : custom_buffer(pData, size)
	{
	}

	simple_buffer(int size) : custom_buffer(size)
	{
	}

	simple_buffer(int size, T val)
	{
		custom_assert(size > 0, "simple_buffer<T>::simple_buffer(int size, T val): not: size > 0");

		m_pData = new T[size];
		custom_assert(m_pData != NULL, "simple_buffer<T>::simple_buffer(int size, T val): not: m_pData != NULL");
		m_size = size;
		m_need_to_release = true;

		if ( (val == 0) || (sizeof(T) == 1) )
		{
			memset(m_pData, int(val), m_size * sizeof(T));
		}
		else
		{
			for (int i = 0; i < m_size; i++)
			{
				m_pData[i] = val;
			}
		}
	}

	simple_buffer(const simple_buffer<T>& obj)
	{
		custom_assert(obj.m_size >= 0, "simple_buffer(const simple_buffer<T>& obj): not: obj.m_size >= 0");
		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (obj.m_size == 0)
		{
			m_pData = NULL;
			m_size = 0;
			m_need_to_release = false;
		}
		else
		{
			if (m_need_to_release)
			{
				m_pData = new T[m_size];
				custom_assert(m_pData != NULL, "simple_buffer<T>::simple_buffer(const simple_buffer<T>& obj): not: m_pData != NULL");
				memcpy(m_pData, obj.m_pData, m_size * sizeof(T));
			}
			else
			{
				m_pData = obj.m_pData;
			}
		}
	}

	simple_buffer(const simple_buffer<T>& obj, int offset, int size)
	{
		custom_assert(size > 0, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: size >= 0");
		custom_assert(offset >= 0, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: offset >= 0");
		custom_assert(offset + size <= obj.m_size, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: offset + size <= obj.m_size");
		m_size = size;

		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{
			m_pData = new T[m_size];
			custom_assert(m_pData != NULL, "simple_buffer<T>::simple_buffer(const simple_buffer<T>& obj): not: m_pData != NULL");
			memcpy(m_pData, obj.m_pData + offset, m_size * sizeof(T));
		}
		else
		{
			m_pData = obj.m_pData + offset;
		}
	}

	simple_buffer<T>& operator= (const simple_buffer<T>& obj)
	{
		if (m_need_to_release)
		{
			if (obj.m_need_to_release)
			{
				if (m_size != obj.m_size)
				{
					delete[] m_pData;
					m_pData = new T[obj.m_size];
					custom_assert(m_pData != NULL, "simple_buffer<T>::operator= (const simple_buffer<T>& obj): not: m_pData != NULL");
				}
			}
			else
			{
				delete[] m_pData;
			}
		}
		else
		{
			if (obj.m_need_to_release)
			{
				m_pData = new T[obj.m_size];
				custom_assert(m_pData != NULL, "simple_buffer<T>::operator= (const simple_buffer<T>& obj): not: m_pData != NULL");
			}
		}

		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{			
			memcpy(m_pData, obj.m_pData, m_size * sizeof(T));
		}
		else
		{
			m_pData = obj.m_pData;
		}

		return *this;		
	}

	void operator+=(custom_buffer<T>& obj)
	{
		custom_assert(m_size >= 0, "operator+=(custom_buffer<T>& obj): not: m_size >= 0");
		custom_assert(obj.m_size >= 0, "operator+=(custom_buffer<T>& obj): not: obj.m_size >= 0");

		int new_size = m_size + obj.m_size;

		if (new_size > 0)
		{
			T* pOldData = m_pData;

			m_pData = new T[new_size];
			custom_assert(m_pData != NULL, "operator+=(custom_buffer<T>& obj): not: m_pData != NULL");
			m_need_to_release = true;

			if (pOldData) memcpy(m_pData, pOldData, m_size * sizeof(T));
			if (obj.m_pData) memcpy(m_pData + m_size, obj.m_pData, obj.m_size * sizeof(T));
			if (pOldData) delete[] pOldData;

			m_size = new_size;
		}
	}

	void copy_data(const custom_buffer<T>& obj, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size >= 0");
		custom_assert(size <= obj.m_size, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size <= obj.m_size");
		custom_assert(size <= m_size, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size <= m_size");		

		if (size > 0)
		{
			memcpy(m_pData, obj.m_pData, size * sizeof(T));
		}
	}

	void copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_dst + size - 1 <= m_size - 1");
		custom_assert(offset_src + size - 1 <= src.m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_src + size - 1 <= src.m_size - 1");		

		if (size > 0)
		{
			memcpy(m_pData + offset_dst, src.m_pData + offset_src, size * sizeof(T));
		}
	}

	void copy_data(T* pData, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(size <= m_size, "simple_buffer<T>::copy_data(T* pData, int size): not: size <= m_size");		

		if (size > 0)
		{
			memcpy(m_pData, pData, size * sizeof(T));
		}
	}

	void copy_data(T* pData, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= m_size - 1, "simple_buffer<T>::copy_data(T* pData, int size): not: offset_dst + size - 1 <= m_size - 1");

		if (size > 0)
		{
			memcpy(m_pData + offset_dst, pData + offset_src, size * sizeof(T));
		}
	}

	simple_buffer<T> get_sub_buffer(int offset)
	{
		custom_assert(offset < m_size, "simple_buffer<T>::get_sub_buffer(int offset): not: offset < m_size");

		return simple_buffer<T>(m_pData + offset, m_size - offset);
	}

	inline void set_values(T val)
	{
		set_values(val, m_size);
	}

	inline void set_values(T val, int size)
	{
		custom_assert((size <= m_size) && (size >= 0), "simple_buffer<T>::set_values(T val, int size = m_size): not: (size <= m_size) && (size >= 0)");

		if ((val == 0) || (sizeof(T) == 1))
		{
			memset(m_pData, (int)val, size * sizeof(T));
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				m_pData[i] = val;
			}
		}
	}

	inline void set_values(T val, int offset, int size)
	{
		custom_assert((offset + size - 1 <= m_size) && (size >= 0), "simple_buffer<T>::set_values(T val, int offset, int size): not: (offset + size - 1 <= m_size) && (size >= 0)");

		if ((val == 0) || (sizeof(T) == 1))
		{
			memset(m_pData + offset, (int)val, size * sizeof(T));
		}
		else
		{
			for (int i = offset; i < (offset + size); i++)
			{
				m_pData[i] = val;
			}
		}
	}
};

inline void UpdateImageColor(u8 *pImBGR, int w, int h)
{
	if (g_video_contrast != 1.0)
	{
		if (g_video_gamma != 1.0)
		{
			simple_buffer<u8> p(256);

			for (int i = 0; i < 256; ++i)
			{
				p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, g_video_gamma) * 255.0);
			}

			for (int i = 0; i < w * h * 3; i++)
			{
				pImBGR[i] = p[cv::saturate_cast<uchar>(g_video_contrast * pImBGR[i])];
			}
		}
		else
		{
			for (int i = 0; i < w * h * 3; i++)
			{
				pImBGR[i] = cv::saturate_cast<uchar>(g_video_contrast * pImBGR[i]);
			}
		}
	}
	else if (g_video_gamma != 1.0)
	{
		simple_buffer<u8> p(256);

		for (int i = 0; i < 256; ++i)
		{
			p[i] = cv::saturate_cast<uchar>(pow(i / 255.0, g_video_gamma) * 255.0);
		}

		for (int i = 0; i < w * h * 3; i++)
		{
			pImBGR[i] = p[pImBGR[i]];
		}
	}
}

inline void UpdateImageColor(simple_buffer<u8>& ImBGR, int w, int h)
{
	custom_assert(ImBGR.m_size >= w * h * 3, "UpdateImageColor not: ImBGR.m_size >= w * h * 3");
	UpdateImageColor(ImBGR.m_pData, w, h);
}
