//***********************************************************************************
//
//	File		:	NtlFile.h
//
//	Begin		:	2007-03-19
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#pragma once

#include "NtlString.h"

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#include "../../Shared/NtlSharedCommon.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif


//-----------------------------------------------------------------------------------
// General File Class
//-----------------------------------------------------------------------------------
class CNtlFile
{
public:

	CNtlFile(void);


	virtual ~CNtlFile(void);


public:

	int				Create(LPCTSTR lpszFileName, int nOperationFlag = _O_CREAT | _O_APPEND | _O_RDWR, int nSharingFlag = _SH_DENYNO, int nPermissionMode = _S_IREAD | _S_IWRITE, bool bAutoClose = false);

	void			Close();

	bool			IsOpened() const noexcept { return (HFILE_ERROR != m_hFile); }

	HFILE			GetFileHandle() const noexcept { return m_hFile; }

	void			SetAutoClose(bool bAutoClose) noexcept { m_bAutoClose = bAutoClose; }


private:

	void			Init();

	void			Destroy();


private:

	HFILE			m_hFile = HFILE_ERROR;

	CNtlString		m_strFileName;

	bool			m_bAutoClose;
};



//-----------------------------------------------------------------------------------
// File Stream Class
//-----------------------------------------------------------------------------------
class CNtlFileStream
{
public:

	CNtlFileStream(void);

	virtual ~CNtlFileStream(void);


public:

	int				Create(LPCTSTR lpszFileName, int nOperationFlag = _O_CREAT | _O_APPEND | _O_RDWR, int nSharingFlag = _SH_DENYNO, int nPermissionMode = _S_IREAD | _S_IWRITE);

	int				Attach(CNtlFile & rFile, LPCTSTR lpszMode = TEXT("w+") );

	FILE *			GetFilePtr() noexcept { return m_pFilePtr; }

	void			Close();

	bool			IsOpened() noexcept { return ( NULL != m_pFilePtr ) ? true : false; }


private:

	void			Init();

	void			Destroy();


private:

	FILE *			m_pFilePtr;
};
