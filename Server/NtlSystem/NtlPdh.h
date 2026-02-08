//***********************************************************************************
//
//	File		:	NtlPdh.h
//
//	Begin		:	2007-02-07
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "NtlString.h"

#if defined(_WIN32)
#include <windows.h>
#include <pdh.h>
#else
#include "../../Shared/NtlSharedCommon.h"
#endif

class CNtlPdh
{
private:

	enum eCONSTANT
	{
		MAX_COUNTER_COUNT = 16
	};


public:

	CNtlPdh();

	~CNtlPdh(void);


public:

	int							Prepare();

	int							RegisterCounter(const char * lpszCounter);

	int							Start(const char * lpszFileName);

	void						Stop();

	LONG						GetValue();

	void						UpdateLog();


public:

	static int					UnitTest();


private:

	void						Init();


public:

#if defined(_WIN32)
	HQUERY						m_hQuery;

	HLOG						m_hLog;

	DWORD						m_dwCounterCount;

	HCOUNTER					m_ahCounter[MAX_COUNTER_COUNT];

	std::string					m_astrCounterName[MAX_COUNTER_COUNT];
#else
	void*						m_hQuery;

	void*						m_hLog;

	DWORD						m_dwCounterCount;

	void*						m_ahCounter[MAX_COUNTER_COUNT];

	std::string					m_astrCounterName[MAX_COUNTER_COUNT];
#endif
};