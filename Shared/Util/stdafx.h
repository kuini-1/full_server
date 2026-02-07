// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers


// Turn-off this Warning for BoundsChecker

#if defined(_MSC_VER)
#pragma warning( disable:4651 )
#pragma warning( disable:4652 )
#pragma warning( disable:4653 )
#pragma warning( disable:4748 )
#pragma warning( disable:4100 )
#pragma warning( disable:4996 )
#endif


#if defined(_WIN32)
#include <ws2tcpip.h>
#include <windows.h>
#include <tchar.h>
#include <wincrypt.h>
#else
#include "../../Shared/NtlSharedCommon.h"
#endif
#include <stdio.h>

// TODO: reference additional headers your program requires here
