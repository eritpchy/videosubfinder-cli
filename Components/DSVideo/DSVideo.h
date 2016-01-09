                              //MyVideo.h//                                
//////////////////////////////////////////////////////////////////////////////////
//							  Version 1.76              						//
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

#include "DataTypes.h"
#include "Video.h"

#include <streams.h>
#include <limits.h>
#include <Dshow.h>
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <errors.h>
#include <amvideo.h>
#include <qedit.h>
#include <dmodshow.h>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class DSVideo;

class CTransNull32 : public CTransInPlaceFilter
{
public:
    int             **m_ppBuffer;
    bool            *m_pImageGeted;
    bool            *m_pIsSetNullRender;
	bool			 m_TriengToGetImage; 
    s64             *m_pST;
    IMediaControl	*m_pMC;
    int             m_ft;
    int             m_w;
    int             m_h;
	int				m_blnReInit;

    CTransNull32( int **ppBuffer, s64 *pST, 
                  bool *pImageGeted, IMediaControl *pMC,
                  bool *pIsSetNullRender, LPUNKNOWN punk, HRESULT *phr );

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    HRESULT Transform(IMediaSample *pSample);
    HRESULT CheckInputType(const CMediaType *mtIn);
};

class MySampleGrabberCallback :	public ISampleGrabberCB
{
public:
	int             **m_ppBuffer;
    bool            *m_pImageGeted;
    bool            *m_pIsSetNullRender;
    s64             *m_pST;
    DSVideo	        *m_pVideo;
	
	MySampleGrabberCallback( int **ppBuffer, s64 *pST, 
                             bool *pImageGeted, DSVideo *pVideo,
                             bool *pIsSetNullRender);

	STDMETHODIMP_(ULONG) AddRef() { return 1; }
	
	STDMETHODIMP_(ULONG) Release() { return 2; }

	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		if (NULL == ppvObject) return E_POINTER;
		
		if (riid == __uuidof(IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
			return S_OK;
		}
		
		if (riid == __uuidof(ISampleGrabberCB))
		{
			*ppvObject = static_cast<ISampleGrabberCB*>(this);
			return S_OK;
		}
		
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample)
	{
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen);
};

/////////////////////////////////////////////////////////////////////////////

class DSVideo: public CVideo
{
public:
	DSVideo();
	~DSVideo();
	
public:		
	bool			m_IsMSSuported;
	bool			m_IsSetNullRender;

    int     *m_pBuffer;
    int     m_BufferSize;
    bool    m_ImageGeted;
    s64     m_st;
	int		m_type; //video open type

	IGraphBuilder	*m_pGB;
	IMediaControl	*m_pMC; 
	IMediaEventEx	*m_pME;
	IMediaSeeking	*m_pMS;
	IVideoWindow	*m_pVW;	
	IBasicVideo		*m_pBV;
	IBasicAudio     *m_pBA;
	IMediaFilter	*m_pMF;

	ISampleGrabber	*m_pGrabber;

	IBaseFilter		*m_pDecoder;
	IBaseFilter		*m_pSourceFilter;
	IBaseFilter		*m_pSampleGrabberFilter; 
	IBaseFilter		*m_pVideoRenderFilter;
    IBaseFilter		*m_pTransNull32Filter;
	CTransNull32	*m_pTransNull32;

	ICaptureGraphBuilder2  *m_pBuilder;

	MySampleGrabberCallback *m_pSGCallback;

public:
	IBaseFilter* GetDecoder();
	IBaseFilter* GetSourceFilter();

	bool OpenMovieNormally(string csMovieName, void *pHWnd);
	bool OpenMovieAllDefault(string csMovieName, void *pHWnd);
	bool OpenMovieHard(string csMovieName, void *pHWnd);

	bool SetVideoWindowPlacement(void *pHWnd);
	bool SetNullRender();

	bool CloseMovie();
	
	void SetPos(s64 Pos);
	void SetPos(double pos);
	void SetPosFast(s64 Pos);

	void SetImageGeted(bool ImageGeted);

	void Run();
	void Pause();

	void WaitForCompletion(s64 timeout);

	void StopFast();

	void RunWithTimeout(s64 timeout);

	void Stop();
    void OneStep();
	s64  OneStepWithTimeout();
	s64  GetPos();
    void GetRGBImage(int *ImRGB, int xmin, int xmax, int ymin, int ymax);

	s64 PosToMilliSeconds(s64 pos);

	void SetVideoWindowPosition(int left, int top, int width, int height);

	void ErrorMessage(string str);

	HRESULT CheckMediaType(IPin *pPinIn, IPin *pPinOut, AM_MEDIA_TYPE *pmtOut);

public:
	HRESULT CleanUp();
	HRESULT ConnectFilters(IGraphBuilder *pGraph,IBaseFilter *pFirst,IBaseFilter *pSecond);
	HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
};

LPCWSTR StringToLPCWSTR(string csStr);
string IntToCStr(int n);
string WCSToStr(WCHAR *wstr);