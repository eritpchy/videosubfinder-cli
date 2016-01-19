// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
#ifndef WINVER				// Allow use of features specific to Win 2K or later
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to NT4 or later
#define _WIN32_WINNT 0x0400
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Win 98 or later
#define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 5.01 or later
#define _WIN32_IE 0x0501
#endif

#include <afxwin.h>		// MFC core and standard components
#include <afxdlgs.h>	// CommDlg Wrappers
#include <afxext.h>		// MFC-Erweiterungen
#include <afxdisp.h>	// MFC Automatisierungsklassen
#include <afxdtctl.h>
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif

#include <emmintrin.h>		// MMX, SSE, SSE2 intrinsic support

#pragma warning(disable: 4799)
