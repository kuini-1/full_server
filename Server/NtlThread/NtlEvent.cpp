//***********************************************************************************
//
//	File		:	NtlEvent.cpp
//
//	Begin		:	2005-11-30
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Event ????ȭ ??????Ʈ Ŭ????
//
//***********************************************************************************


#include "stdafx.h"
#include "NtlEvent.h"
#include "NtlThreadException.h"


CNtlEvent::CNtlEvent()
	: m_hEvent( INVALID_HANDLE_VALUE )
{
#if defined(_WIN32)
	m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	if( NULL == m_hEvent )
	{
		DWORD rc = GetLastError();
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_EVENT_CREATE, rc);
	}
#endif
}


CNtlEvent::~CNtlEvent(void)
{
#if defined(_WIN32)
	if( false == CloseHandle( m_hEvent ) )
	{
		DWORD rc = GetLastError();
		THROW_THREAD_EXCEPTION((int)eTHREAD_ERR_EVENT_DESTROY, (int)rc);
	}
#endif
	m_hEvent = INVALID_HANDLE_VALUE;
}


void CNtlEvent::Reset()
{
#if defined(_WIN32)
	if( false == ResetEvent( m_hEvent ) )
	{
		DWORD rc = GetLastError();
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_EVENT_RESET, rc);
	}
#endif
}


void CNtlEvent::Wait()
{
#if defined(_WIN32)
	if( WAIT_FAILED == WaitForSingleObject( m_hEvent, INFINITE ) )
	{
		DWORD rc = GetLastError();
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_EVENT_WAIT, rc);
	}
#endif
}


int CNtlEvent::Wait( unsigned int millisecs )
{
#if defined(_WIN32)
	DWORD rc = NO_ERROR;
	DWORD status = WaitForSingleObject( m_hEvent, millisecs );

	if( WAIT_TIMEOUT == status  )
	{
		rc = ERROR_TIMEOUT;
	}
	else if( WAIT_FAILED == status )
	{
		rc = GetLastError();
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_EVENT_WAIT, rc);
	}

	return rc;
#else
	(void)millisecs;
	return 0;
#endif
}


void CNtlEvent::Signal()
{
#if defined(_WIN32)
	if( !SetEvent( m_hEvent ) )
	{
		DWORD rc = GetLastError();
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_EVENT_NOTIFY, rc);
	}
#endif
}
