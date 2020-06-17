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
#define custom_assert(cond, msg) wxASSERT_MSG(cond, msg)
#define my_event custom_event
#define custom_set_started(pevent) (pevent)->set_started()

// for final release build
//#define custom_assert(cond, msg)
//#define my_event concurrency::event
//#define custom_set_started(pevent)

#define cvMAT cv::UMat
//#define cvMAT cv::Mat

extern bool g_use_ocl;


class custom_event : public concurrency::event
{
public:
	bool m_started = false;
	bool m_finished = false;

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
		concurrency::event::reset();
	}

	/*size_t wait(unsigned int _Timeout = Concurrency::COOPERATIVE_TIMEOUT_INFINITE)
	{
		custom_assert(m_started == true, "class custom_event:wait\nnot: m_started == true");
		return concurrency::event::wait(_Timeout);
	}*/
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
		m_size = size;
		m_need_to_release = true;
	}

	custom_buffer(int size, T val)
	{
		custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");

		m_pData = new T[size];
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
		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{
			m_pData = new T[m_size];
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

	custom_buffer<T>& operator= (const custom_buffer<T> &obj)
	{
		if (m_need_to_release)
		{
			delete[] m_pData;
		}

		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{
			m_pData = new T[m_size];
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

		/*if (m_need_to_release)
		{
			if (obj.m_need_to_release)
			{
				if (m_size != obj.m_size)
				{
					delete[] m_pData;
					m_pData = new T[obj.m_size];
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

		return *this;*/
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
		//if ((!m_need_to_release) || (m_size != size))
		//{
			if (m_need_to_release)
			{
				delete[] m_pData;
			}

			custom_assert(size >= 0, "custom_buffer(int size): not: size >= 0");

			if (size == 0)
			{
				m_pData = NULL;
				m_size = 0;
				m_need_to_release = false;
			}
			else
			{
				m_pData = new T[size];
				m_size = size;
				m_need_to_release = true;
			}
		//}
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

template<>
custom_buffer<char>::custom_buffer(int size, char val)
{
	custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");
	m_pData = new char[size];
	m_size = size;
	m_need_to_release = true;

	memset(m_pData, val, m_size);
}

template<>
custom_buffer<u8>::custom_buffer(int size, u8 val)
{
	custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");
	m_pData = new u8[size];
	m_size = size;
	m_need_to_release = true;

	memset(m_pData, val, m_size);
}

template<>
custom_buffer<s64>::custom_buffer(int size, s64 val)
{
	custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");
	m_pData = new s64[size];
	m_size = size;
	m_need_to_release = true;

	if (val == 0)
	{
		memset(m_pData, 0, m_size * sizeof(s64));
	}
	else
	{
		for (int i = 0; i < m_size; i++)
		{
			m_pData[i] = val;
		}
	}
}

template<>
custom_buffer<int>::custom_buffer(int size, int val)
{
	custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");
	m_pData = new int[size];
	m_size = size;
	m_need_to_release = true;

	if (val == 0)
	{
		memset(m_pData, 0, m_size * sizeof(int));
	}
	else
	{
		for (int i = 0; i < m_size; i++)
		{
			m_pData[i] = val;
		}
	}
}

/*
template<>
custom_buffer<u8>& custom_buffer<u8>::operator= (const custom_buffer<u8>& obj)
{
	if (m_need_to_release)
	{
		delete[] m_pData;
	}

	m_size = obj.m_size;
	m_need_to_release = obj.m_need_to_release;

	if (m_need_to_release)
	{
		m_pData = new u8[m_size];
		memcpy(m_pData, obj.m_pData, m_size * sizeof(u8));
	}
	else
	{
		m_pData = obj.m_pData;
	}

	return *this;
}*/

template<>
custom_buffer<int>& custom_buffer<int>::operator= (const custom_buffer<int> &obj)
{
	if (m_need_to_release)
	{
		delete[] m_pData;
	}

	m_size = obj.m_size;
	m_need_to_release = obj.m_need_to_release;

	if (m_need_to_release)
	{
		m_pData = new int[m_size];
		memcpy(m_pData, obj.m_pData, m_size * sizeof(int));
	}
	else
	{
		m_pData = obj.m_pData;
	}

	return *this;
}

template<>
custom_buffer<s64>& custom_buffer<s64>::operator= (const custom_buffer<s64> &obj)
{
	if (m_need_to_release)
	{
		delete[] m_pData;
	}

	m_size = obj.m_size;
	m_need_to_release = obj.m_need_to_release;

	if (m_need_to_release)
	{
		m_pData = new s64[m_size];
		memcpy(m_pData, obj.m_pData, m_size * sizeof(s64));
	}
	else
	{
		m_pData = obj.m_pData;
	}

	return *this;
}


template <typename T>
class simple_buffer : public custom_buffer<T>
{
public:

	simple_buffer(int size, T val)
	{
		custom_assert(size > 0, "custom_buffer(int size, T val): not: size > 0");
		m_pData = new T[size];
		m_size = size;
		m_need_to_release = true;

		if ( (val == 0) || (sizeof(T) == 1) )
		{
			memset(m_pData, 0, m_size * sizeof(T));
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
		m_size = obj.m_size;
		m_need_to_release = obj.m_need_to_release;

		if (m_need_to_release)
		{
			m_pData = new T[m_size];
			memcpy(m_pData, obj.m_pData, m_size * sizeof(T));
		}
		else
		{
			m_pData = obj.m_pData;
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
};

