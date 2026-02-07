// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#if defined(_WIN32)
#include <windows.h>
#else
#include "../../Shared/NtlSharedCommon.h"
#endif
#include <assert.h>


// TODO: reference additional headers your program requires here
