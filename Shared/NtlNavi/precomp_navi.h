#ifndef _PRECOMP_NAVI_H_
#define _PRECOMP_NAVI_H_


#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS 1

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "../NtlSharedCommon.h"
#include <hash_map>
#else
#include "../NtlSharedCommon.h"
#include <unordered_map>
namespace stdext {
	template<typename K, typename V>
	using hash_map = std::unordered_map<K, V>;
}
#endif

#include <math.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "NtlNaviDataMng.h"

#include "./PathEngine/i_pathengine.h"
#include "NtlNaviDefine.h"
#include "NtlNavi.h"


#endif
