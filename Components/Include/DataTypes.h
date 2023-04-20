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
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include "opencv2/imgcodecs.hpp"
#include <condition_variable>
#include <queue>
#include <thread>
#ifdef WIN32
#include <ppl.h>
#endif

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;

#ifdef WIN32
typedef unsigned __int64	    u64;
typedef __int64	                    s64;
#else
typedef unsigned long long	u64;
typedef long long	                s64;
#endif

wxString get_add_info();

extern void PlaySound(wxString path);

extern wxString g_ReportFileName;
extern wxString g_ErrorFileName;
extern wxString GetFileNameWithExtension(wxString FilePath);

inline void SaveToReportLog(char const* file, int line, wxString msg, wxString mode = wxT("ab"))
{
	wxFFileOutputStream ffout(g_ReportFileName, mode);
	wxTextOutputStream fout(ffout);
	fout << GetFileNameWithExtension(wxString(file)) << wxT(":") << line << wxT(" ") << msg;
	fout.Flush();
	ffout.Close();
}
#define SaveToReportLog(...) SaveToReportLog(__FILE__, __LINE__, __VA_ARGS__)

inline void SaveError(char const* file, int line, wxString error)
{
	wxFFileOutputStream ffout(g_ErrorFileName, wxT("ab"));
	wxTextOutputStream fout(ffout);
	fout << GetFileNameWithExtension(wxString(file)) << wxT(":") << line << wxT(" ") << error;
	fout.Flush();
	ffout.Close();
}
#define SaveError(error) SaveError(__FILE__, __LINE__, error)

extern int exception_filter(unsigned int code, struct _EXCEPTION_POINTERS* ep, char* det);

// NOTE: for debugging!!!
//----------------------------------
//#define CUSTOM_DEBUG
//#define CUSTOM_DEBUG2

// #define CUSTOM_ASSERT

//#define CUSTOM_TA
//----------------------------------

#define my_event custom_event

// NOTE: UMat lead to app process hang on exit in x86 version
// also leads to lower performance: time ./VideoSubFinderWXW.exe -ccti -nocrthr 16
// NOTE: cv::UMat real    3m31.686s
// NOTE: cv::Mat real    3m23.222s
#define cvMAT cv::Mat

extern bool g_use_ocl;

extern double g_video_contrast;
extern double g_video_gamma;

#ifdef CUSTOM_DEBUG
#define wxDEBUG_DET(det) det
#else
#define wxDEBUG_DET(det) wxT("")
#endif

#ifdef CUSTOM_ASSERT
//#define custom_assert(cond, msg)  if(!(cond)) { wxASSERT_MSG(cond, wxString(msg) + get_add_info()); }
#define custom_assert(cond, msg) wxASSERT_MSG(cond, msg)
#define custom_set_started(pevent) (pevent)->set_started()
#else
#define custom_assert(cond, msg)
#define custom_set_started(pevent)
#endif

enum ColorSpace {
	RGB,
	Lab
};

struct color_range
{
	std::array<int, 3> m_min_data;
	std::array<int, 3> m_max_data;
	ColorSpace m_color_space;
};

inline bool operator==(const color_range& lhs, const color_range& rhs)
{
	return ((lhs.m_min_data == rhs.m_min_data) &&
		(lhs.m_max_data == rhs.m_max_data) &&
		(lhs.m_color_space == rhs.m_color_space));
}

enum TextAlignment : int {
	Center = 0,
	Left,
	Right,	
	Any
};

class custom_event
{
	std::mutex m_cv_mutex;
	std::condition_variable m_cv;

public:
#ifdef CUSTOM_ASSERT
	bool m_started = false;
#endif
	bool m_finished = false;
	bool m_need_to_skip = false;

#ifdef CUSTOM_ASSERT
	void set_started()
	{
		custom_assert(m_started == false, "class custom_event:set_started\nnot: m_started == false");
		custom_assert(m_finished == false, "class custom_event:set_started\nnot: m_finished == false");
		m_started = true;
	}
#endif

	void set()
	{
		const std::lock_guard<std::mutex> lock(m_cv_mutex);
		custom_assert(m_started == true, "class custom_event:set\nnot: m_started == true");
		custom_assert(m_finished == false, "class custom_event:set\nnot: m_finished == false");
		m_finished = true;
		m_cv.notify_all();
	}

	void wait()
	{
		std::unique_lock<std::mutex> lock(m_cv_mutex);
		m_cv.wait(lock, [this] { return m_finished; });
	}

	void reset()
	{
		custom_assert(m_started == m_finished, "class custom_event:reset\nnot: m_started == m_finished");
#ifdef CUSTOM_ASSERT
		m_started = false;
#endif
		m_finished = false;
		m_need_to_skip = false;
	}
};

template <typename T>
class threadsafe_queue
{
	std::mutex m_cv_mutex;
	std::condition_variable m_cv;
	std::queue<T> m_data_queue;

public:
	void push(T& new_data)
	{
		const std::lock_guard<std::mutex> lock(m_cv_mutex);
		m_data_queue.push(new_data);
		m_cv.notify_all();
	}

	void wait_and_pop(T& data)
	{
		std::unique_lock<std::mutex> lock(m_cv_mutex);
		m_cv.wait(lock, [this] { return m_data_queue.size() > 0; });
		data = m_data_queue.front();
		m_data_queue.pop();
	}
};

template <typename T>
class ForwardIteratorForDefineRange
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = T;
	using pointer = T*;
	using reference = T&;

	ForwardIteratorForDefineRange() = default;
	ForwardIteratorForDefineRange(T index) : m_index(index) {}
	const T& operator*() const { return m_index; }
	const void operator++() { ++m_index; }
	bool operator!=(const ForwardIteratorForDefineRange& lhs) const { return m_index != lhs.m_index; }
private:
	T m_index;
};

template <typename T>
class ForwardIteratorForDefineRangeWithStep
{
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = T;
	using pointer = T*;
	using reference = T&;

	ForwardIteratorForDefineRangeWithStep() = default;
	ForwardIteratorForDefineRangeWithStep(T index, T step) : m_index(index), m_step(step) {}
	const T& operator*() const { return m_index; }
	const void operator++() { m_index += m_step; }
	bool operator!=(const ForwardIteratorForDefineRangeWithStep& lhs) const { return m_index != lhs.m_index; }
private:
	T m_index;
	T m_step;
};

#ifdef WIN32 // WINX86 or WIN64
#ifdef WIN64
#define run_in_parallel concurrency::parallel_invoke
#endif
#ifdef WINX86
// replacing run_in_parallel to sequential run in 1 thread
template <typename _Function1, typename _Function2>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2)
{
	_Func1();
	_Func2();
}

template <typename _Function1, typename _Function2, typename _Function3>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3)
{
	_Func1();
	_Func2();
	_Func3();
}

template <typename _Function1, typename _Function2, typename _Function3, typename _Function4>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3, const _Function4& _Func4)
{
	_Func1();
	_Func2();
	_Func3();
	_Func4();
}

template <typename _Function1, typename _Function2, typename _Function3, typename _Function4, typename _Function5>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3, const _Function4& _Func4, const _Function5& _Func5)
{
	_Func1();
	_Func2();
	_Func3();
	_Func4();
	_Func5();
}
#endif
#else // not WIN32 == Linux
template <typename _Function1, typename _Function2>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2)
{
	std::thread t1(_Func1);
	std::thread t2(_Func2);

	t1.join();
	t2.join();
}

template <typename _Function1, typename _Function2, typename _Function3>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3)
{
	std::thread t1(_Func1);
	std::thread t2(_Func2);
	std::thread t3(_Func3);

	t1.join();
	t2.join();
	t3.join();
}

template <typename _Function1, typename _Function2, typename _Function3, typename _Function4>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3, const _Function4& _Func4)
{
	std::thread t1(_Func1);
	std::thread t2(_Func2);
	std::thread t3(_Func3);
	std::thread t4(_Func4);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
}

template <typename _Function1, typename _Function2, typename _Function3, typename _Function4, typename _Function5>
inline void run_in_parallel(const _Function1& _Func1, const _Function2& _Func2, const _Function3& _Func3, const _Function4& _Func4, const _Function5& _Func5)
{
	std::thread t1(_Func1);
	std::thread t2(_Func2);
	std::thread t3(_Func3);
	std::thread t4(_Func4);
	std::thread t5(_Func5);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
}
#endif

class shared_custom_task
{
	struct custom_task_data
	{
		std::thread m_thr;
		std::mutex m_mutex;

		template<typename _Function>
		custom_task_data(const _Function& _Func) : m_thr(_Func) {}

		void wait()
		{
			const std::lock_guard<std::mutex> lock(m_mutex);
			if (m_thr.joinable())
			{
				m_thr.join();
			}
		}
	};

	std::shared_ptr<custom_task_data> m_data;

public:
	shared_custom_task() {}

	template<typename _Function>
	shared_custom_task(const _Function& _Func)
	{
		m_data = std::make_shared<custom_task_data>(_Func);
	}

	shared_custom_task(const shared_custom_task& other)
	{
		m_data = other.m_data;
	}

	shared_custom_task(shared_custom_task&& other)
	{
		m_data = std::move(other.m_data);
	}

	shared_custom_task & operator= (const shared_custom_task& other)
	{
		custom_assert(static_cast<bool>(m_data) == false, "shared_custom_task: 'operator= &' m_data != false");

		if (other.m_data)
		{
			m_data = other.m_data;
		}		

		return *this;
	}

	shared_custom_task& operator= (shared_custom_task&& other)
	{
		custom_assert(static_cast<bool>(m_data) == false, "shared_custom_task: 'operator= &&' m_data != false");

		m_data = std::move(other.m_data);

		return *this;
	}

	void wait()
	{
		if (m_data)
		{
			m_data->wait();
			m_data.reset();
		}
	}
};

template <typename _Iterator>
inline void wait_all(_Iterator first, _Iterator last)
{
	for (; first != last; ++first)
	{
		first->wait();
	}
}

template <typename T>
class custom_buffer
{
public:
	T *m_pData;
	int m_size;
	bool m_need_to_release;

	virtual ~custom_buffer()
	{
		if (m_need_to_release)
		{
			custom_assert(m_pData != NULL, "custom_buffer<T>::~custom_buffer(): not: m_pData != NULL");
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

	// return sub buffer
	custom_buffer(const custom_buffer<T>& obj, int offset)
	{
		custom_assert(offset < obj.m_size, "custom_buffer(const custom_buffer<T>& obj, int offset): not: offset < obj.m_size");
		m_pData = obj.m_pData + offset;
		m_size = obj.m_size - offset;
		m_need_to_release = false;
		
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
		custom_assert(offset_dst >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_dst >= 0");
		custom_assert(offset_src >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_src >= 0");
		custom_assert(offset_dst + size - 1 <= m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_dst + size - 1 <= m_size - 1");
		custom_assert(offset_src + size - 1 <= src.m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_src + size - 1 <= src.m_size - 1");

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

	simple_buffer() : custom_buffer<T>()
	{
	}

	~simple_buffer()
	{
	}

	simple_buffer(T* pData, int size) : custom_buffer<T>(pData, size)
	{
	}	

	simple_buffer(int size) : custom_buffer<T>(size)
	{
	}

	simple_buffer(int size, T val)
	{
		custom_assert(size > 0, "simple_buffer<T>::simple_buffer(int size, T val): not: size > 0");

		this->m_pData = new T[size];
		custom_assert(this->m_pData != NULL, "simple_buffer<T>::simple_buffer(int size, T val): not: this->m_pData != NULL");
		this->m_size = size;
		this->m_need_to_release = true;

		if (val == 0)
		{
			memset(this->m_pData, 0, this->m_size * sizeof(T));
		}
		else
		{
			for (int i = 0; i < this->m_size; i++)
			{
				this->m_pData[i] = val;
			}
		}
	}

	simple_buffer(const simple_buffer<T>& obj)
	{
		custom_assert(obj.m_size >= 0, "simple_buffer(const simple_buffer<T>& obj): not: obj.m_size >= 0");
		this->m_size = obj.m_size;
		this->m_need_to_release = obj.m_need_to_release;

		if (obj.m_size == 0)
		{
			this->m_pData = NULL;
			this->m_size = 0;
			this->m_need_to_release = false;
		}
		else
		{
			if (this->m_need_to_release)
			{
				this->m_pData = new T[this->m_size];
				custom_assert(this->m_pData != NULL, "simple_buffer<T>::simple_buffer(const simple_buffer<T>& obj): not: this->m_pData != NULL");
				memcpy(this->m_pData, obj.m_pData, this->m_size * sizeof(T));
			}
			else
			{
				this->m_pData = obj.m_pData;
			}
		}
	}

	// return sub buffer
	simple_buffer(const simple_buffer<T>& obj, int offset)
	{
		custom_assert(offset < obj.m_size, "simple_buffer(const simple_buffer<T>& obj, int offset): not: offset < obj.m_size");
		this->m_pData = obj.m_pData + offset;
		this->m_size = obj.m_size - offset;
		this->m_need_to_release = false;
	}

	// make copy if required
	simple_buffer(const simple_buffer<T>& obj, int offset, int size)
	{
		custom_assert(size > 0, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: size >= 0");
		custom_assert(offset >= 0, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: offset >= 0");
		custom_assert(offset + size <= obj.m_size, "simple_buffer(const simple_buffer<T>& obj, int offset, int size): not: offset + size <= obj.m_size");
		this->m_size = size;

		this->m_need_to_release = obj.m_need_to_release;

		if (this->m_need_to_release)
		{
			this->m_pData = new T[this->m_size];
			custom_assert(this->m_pData != NULL, "simple_buffer<T>::simple_buffer(const simple_buffer<T>& obj): not: this->m_pData != NULL");
			memcpy(this->m_pData, obj.m_pData + offset, this->m_size * sizeof(T));
		}
		else
		{
			this->m_pData = obj.m_pData + offset;
		}
	}

	simple_buffer<T>& operator= (const simple_buffer<T>& obj)
	{
		if (this->m_need_to_release)
		{
			if (obj.m_need_to_release)
			{
				if (this->m_size != obj.m_size)
				{
					delete[] this->m_pData;
					this->m_pData = new T[obj.m_size];
					custom_assert(this->m_pData != NULL, "simple_buffer<T>::operator= (const simple_buffer<T>& obj): not: this->m_pData != NULL");
				}
			}
			else
			{
				delete[] this->m_pData;
			}
		}
		else
		{
			if (obj.m_need_to_release)
			{
				this->m_pData = new T[obj.m_size];
				custom_assert(this->m_pData != NULL, "simple_buffer<T>::operator= (const simple_buffer<T>& obj): not: this->m_pData != NULL");
			}
		}

		this->m_size = obj.m_size;
		this->m_need_to_release = obj.m_need_to_release;

		if (this->m_need_to_release)
		{			
			memcpy(this->m_pData, obj.m_pData, this->m_size * sizeof(T));
		}
		else
		{
			this->m_pData = obj.m_pData;
		}

		return *this;		
	}

	void operator+=(custom_buffer<T>& obj)
	{
		custom_assert(this->m_size >= 0, "operator+=(custom_buffer<T>& obj): not: this->m_size >= 0");
		custom_assert(obj.m_size >= 0, "operator+=(custom_buffer<T>& obj): not: obj.m_size >= 0");

		int new_size = this->m_size + obj.m_size;

		if (new_size > 0)
		{
			T* pOldData = this->m_pData;

			this->m_pData = new T[new_size];
			custom_assert(this->m_pData != NULL, "operator+=(custom_buffer<T>& obj): not: this->m_pData != NULL");
			this->m_need_to_release = true;

			if (pOldData) memcpy(this->m_pData, pOldData, this->m_size * sizeof(T));
			if (obj.m_pData) memcpy(this->m_pData + this->m_size, obj.m_pData, obj.m_size * sizeof(T));
			if (pOldData) delete[] pOldData;

			this->m_size = new_size;
		}
	}

	void copy_data(const custom_buffer<T>& obj, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size >= 0");
		custom_assert(size <= obj.m_size, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size <= obj.m_size");
		custom_assert(size <= this->m_size, "simple_buffer<T>::copy_data(const custom_buffer<T>& obj, int size): not: size <= this->m_size");		

		if (size > 0)
		{
			memcpy(this->m_pData, obj.m_pData, size * sizeof(T));
		}
	}

	void copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= this->m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_dst + size - 1 <= this->m_size - 1");
		custom_assert(offset_src + size - 1 <= src.m_size - 1, "simple_buffer<T>::copy_data(const custom_buffer<T>& src, int offset_dst, int offset_src, int size): not: offset_src + size - 1 <= src.m_size - 1");		

		if (size > 0)
		{
			memcpy(this->m_pData + offset_dst, src.m_pData + offset_src, size * sizeof(T));
		}
	}

	void copy_data(T* pData, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(size <= this->m_size, "simple_buffer<T>::copy_data(T* pData, int size): not: size <= this->m_size");		

		if (size > 0)
		{
			memcpy(this->m_pData, pData, size * sizeof(T));
		}
	}

	void copy_data(T* pData, int offset_dst, int offset_src, int size)
	{
		custom_assert(size >= 0, "simple_buffer<T>::copy_data(T* pData, int size): not: size >= 0");
		custom_assert(offset_dst + size - 1 <= this->m_size - 1, "simple_buffer<T>::copy_data(T* pData, int size): not: offset_dst + size - 1 <= this->m_size - 1");

		if (size > 0)
		{
			memcpy(this->m_pData + offset_dst, pData + offset_src, size * sizeof(T));
		}
	}

	inline void set_values(T val)
	{
		set_values(val, this->m_size);
	}

	inline void set_values(T val, int size)
	{
		custom_assert((size <= this->m_size) && (size >= 0), "simple_buffer<T>::set_values(T val, int size = this->m_size): not: (size <= this->m_size) && (size >= 0)");

		if ((val == 0) || (sizeof(T) == 1))
		{
			memset(this->m_pData, (int)val, size * sizeof(T));
		}
		else
		{
			for (int i = 0; i < size; i++)
			{
				this->m_pData[i] = val;
			}
		}
	}

	inline void set_values(T val, int offset, int size)
	{
		custom_assert((offset + size - 1 <= this->m_size) && (size >= 0), "simple_buffer<T>::set_values(T val, int offset, int size): not: (offset + size - 1 <= this->m_size) && (size >= 0)");

		if ((val == 0) || (sizeof(T) == 1))
		{
			memset(this->m_pData + offset, (int)val, size * sizeof(T));
		}
		else
		{
			for (int i = offset; i < (offset + size); i++)
			{
				this->m_pData[i] = val;
			}
		}
	}
};

template<> inline
simple_buffer<u8> :: simple_buffer(int size, u8 val)
{
	custom_assert(size > 0, "simple_buffer<u8>::simple_buffer(int size, u8 val): not: size > 0");

	this->m_pData = new u8[size];
	custom_assert(this->m_pData != NULL, "simple_buffer<u8>::simple_buffer(int size, u8 val): not: this->m_pData != NULL");
	this->m_size = size;
	this->m_need_to_release = true;

	memset(this->m_pData, int(val), this->m_size );
}

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
