//***********************************************************************************
//
//	File		:	NtlPerformance.h
//
//	Begin		:	2007-02-08
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "NtlPdh.h"

#if !defined(_WIN32)
#include "../../Shared/NtlSharedCommon.h"
typedef struct _SYSTEM_INFO { int dummy; } SYSTEM_INFO;
typedef struct _MEMORYSTATUSEX
{
	DWORD dwLength;
	DWORD dwMemoryLoad;
	QWORD ullTotalPhys;
	QWORD ullAvailPhys;
	QWORD ullTotalPageFile;
	QWORD ullAvailPageFile;
	QWORD ullTotalVirtual;
	QWORD ullAvailVirtual;
	QWORD ullAvailExtendedVirtual;
} MEMORYSTATUSEX;
typedef QWORD DWORDLONG;
#endif


class CNtlPerformance
{
public:

	CNtlPerformance(void);

	~CNtlPerformance(void);


public:

	int							Create(HANDLE hProcess = NULL);

public:

	SYSTEM_INFO *				GetSystemInfo() { return &m_systemInfo; }
	MEMORYSTATUSEX *			GetMemoryStatus(bool bRefresh = true);
	void						RefreshMemoryStatus() { GetMemoryStatus(true); }

	void						UpdateLog();


public:

	// cpu
	DWORD  						GetProcessProcessorLoad();
	DWORD						GetSystemProcessorLoad();

	// memory
	size_t						GetProcessMemoryUsage();
	DWORDLONG					GetSystemMemoryTotalPhy() { return m_memoryStatus.ullTotalPhys; }
	DWORDLONG					GetSystemMemoryAvailPhy() { return m_memoryStatus.ullAvailPhys; }
	DWORDLONG					GetSystemMemoryTotalVir() { return m_memoryStatus.ullTotalVirtual; }
	DWORDLONG					GetSystemMemoryAvailVir() { return m_memoryStatus.ullAvailVirtual; }
	DWORDLONG					GetSystemMemoryTotalPageFile() { return m_memoryStatus.ullTotalPageFile; }
	DWORDLONG					GetSystemMemoryAvailPageFile() { return m_memoryStatus.ullAvailPageFile; }
	DWORD 						GetSystemMemoryLoad() { return m_memoryStatus.dwMemoryLoad; }


private:

	void						Init();

	void						Destroy();


private:

	HANDLE						m_hProcess;


	CNtlPdh						m_pdhProcessUsage;

	CNtlPdh						m_pdhSystemProcessorUsage;

	CNtlPdh						m_pdhMemoryUsage;

	SYSTEM_INFO					m_systemInfo;

	MEMORYSTATUSEX				m_memoryStatus;
};