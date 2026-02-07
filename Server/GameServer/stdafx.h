#pragma once

//#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "NtlBase.h"

#if defined(_WIN32)
#include <tchar.h>
#include <wincrypt.h>
#endif
#include <stdio.h>
#include <ctime>
#include <set>
#include <map>


// TODO: reference additional headers your program requires here
#include "NtlLog.h"
#include "Utils.h"
