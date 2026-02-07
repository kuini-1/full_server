//***********************************************************************************
//
//	File		:	NtlMiniDump.h
//
//	Begin		:	2007-03-05
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#pragma once

#include "../../Shared/NtlSharedCommon.h"

class CNtlMiniDump
{
public:

	CNtlMiniDump(void);

	virtual ~CNtlMiniDump(void);


public:

	static bool			Start();

	static void			Stop();

	static void			Snapshot();

	//static void			ThreadSnapshot(void *hThread, SYSTEMTIME *SystemTime, bool bIsDump);


public:

	static int			UnitTest();

	static void			SetDumpPath( const char * lptFilePath );
	static char*		GetDumpPath() { return m_szDumpPath; }
private:

	static char			m_szDumpPath[MAX_PATH + 1];
};
