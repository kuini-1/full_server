//***********************************************************************************
//
//	File		:	NtlMutex.h
//
//	Begin		:	2005-11-30
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Mutex ????? ??????? ?????
//
//***********************************************************************************

#ifndef __NTLMUTEX_H__
#define __NTLMUTEX_H__

#include "assert.h"

#ifdef _WIN32
#include "windows.h"

// Mutex Class
class CNtlMutex
{
public:
	CNtlMutex(DWORD dwSpinCount = 4000);

	virtual ~CNtlMutex();

	void				Lock();

	void				Unlock();

	CRITICAL_SECTION *	GetRealMutex();

private:

	CRITICAL_SECTION	m_mutex;
};

// Mutex helper class ( ????? ??? ???? )
class CNtlAutoMutex
{
public:
	CNtlAutoMutex(CNtlMutex * pMutex);

	virtual ~CNtlAutoMutex(void);


	void				Lock();

	void				Unlock();

private:

	CNtlMutex *			m_pMutex;

	bool				m_bLocked;
};


inline CNtlMutex::CNtlMutex(DWORD dwSpinCount)
{
	::InitializeCriticalSectionAndSpinCount( &m_mutex, dwSpinCount );
}

inline CNtlMutex::~CNtlMutex()
{
	::DeleteCriticalSection( &m_mutex );
}

inline void CNtlMutex::Lock()
{
	::EnterCriticalSection( &m_mutex );
}

inline void CNtlMutex::Unlock()
{
	::LeaveCriticalSection( &m_mutex );
}

inline CRITICAL_SECTION * CNtlMutex::GetRealMutex()
{
	return &m_mutex;
}


inline CNtlAutoMutex::CNtlAutoMutex(CNtlMutex *pMutex)
:m_pMutex( pMutex ), m_bLocked( false )
{
}

inline CNtlAutoMutex::~CNtlAutoMutex()
{
	Unlock();
}

inline void CNtlAutoMutex::Lock()
{
	if( !m_bLocked )
	{
		m_pMutex->Lock();
		m_bLocked = true;
	}
}

inline void CNtlAutoMutex::Unlock()
{
	if( m_bLocked )
	{
		m_bLocked = false;
		m_pMutex->Unlock();
	}
}






class CNtlLock
{
public:
	CNtlLock(CNtlMutex * pMutex);
	virtual ~CNtlLock(void);

private:

	CNtlMutex *			m_mutex;
};


inline CNtlLock::CNtlLock(CNtlMutex *pMutex)
	:m_mutex(pMutex)
{
	m_mutex->Lock();
}

inline CNtlLock::~CNtlLock()
{
	m_mutex->Unlock();
}


#else
// Linux: pthread-based mutex (CRITICAL_SECTION and wrappers from NtlSharedCommon.h)
#include "../../Shared/NtlSharedCommon.h"

class CNtlMutex
{
public:
	CNtlMutex(DWORD dwSpinCount = 4000);

	virtual ~CNtlMutex();

	void				Lock();

	void				Unlock();

	CRITICAL_SECTION *	GetRealMutex();

private:

	CRITICAL_SECTION	m_mutex;
};

class CNtlAutoMutex
{
public:
	CNtlAutoMutex(CNtlMutex * pMutex);

	virtual ~CNtlAutoMutex(void);

	void				Lock();

	void				Unlock();

private:

	CNtlMutex *			m_pMutex;

	bool				m_bLocked;
};


inline CNtlMutex::CNtlMutex(DWORD dwSpinCount)
{
	(void)dwSpinCount;
	pthread_mutex_init(&m_mutex, NULL);
}

inline CNtlMutex::~CNtlMutex()
{
	pthread_mutex_destroy(&m_mutex);
}

inline void CNtlMutex::Lock()
{
	pthread_mutex_lock(&m_mutex);
}

inline void CNtlMutex::Unlock()
{
	pthread_mutex_unlock(&m_mutex);
}

inline CRITICAL_SECTION * CNtlMutex::GetRealMutex()
{
	return &m_mutex;
}


inline CNtlAutoMutex::CNtlAutoMutex(CNtlMutex *pMutex)
:m_pMutex( pMutex ), m_bLocked( false )
{
}

inline CNtlAutoMutex::~CNtlAutoMutex()
{
	Unlock();
}

inline void CNtlAutoMutex::Lock()
{
	if( !m_bLocked )
	{
		m_pMutex->Lock();
		m_bLocked = true;
	}
}

inline void CNtlAutoMutex::Unlock()
{
	if( m_bLocked )
	{
		m_bLocked = false;
		m_pMutex->Unlock();
	}
}


class CNtlLock
{
public:
	CNtlLock(CNtlMutex * pMutex);
	virtual ~CNtlLock(void);

private:

	CNtlMutex *			m_mutex;
};


inline CNtlLock::CNtlLock(CNtlMutex *pMutex)
	:m_mutex(pMutex)
{
	m_mutex->Lock();
}

inline CNtlLock::~CNtlLock()
{
	m_mutex->Unlock();
}

#endif // _WIN32




#endif // __NTLMUTEX_H__
