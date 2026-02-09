//***********************************************************************************
//
//	File		:	NtlThread.cpp
//
//	Begin		:	2005-11-30 
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	NTL Thread Class
//
//***********************************************************************************


#include "stdafx.h"
#include "NtlThread.h"
#include "NtlThreadException.h"
#include "NtlLog.h"

#if !defined(_WIN32)

// Non-Windows stub implementation: compile/link compatibility only.

static const char * s_thread_status_string[CNtlThread::MAX_STATUS] =
{
	"STATUS_NOT_RUNNING",
	"STATUS_PREPARING_TO_RUN",
	"STATUS_RUNNING",
	"STATUS_PAUSED",
	"STATUS_DEAD",
};

CThreadKey *	CNtlThread::m_pThreadKey = 0;
CNtlThread *	CNtlThread::m_pMainThread = 0;

const char * CNtlThread::GetStatusString()
{
	if( m_status >= MAX_STATUS )
		return "Unknown Status";
	return s_thread_status_string[m_status];
}

CNtlThread::CNtlThread(CNtlRunObject * pRunObject, const char * name, bool bAutoDelete)
:m_strName(name ? name : "Unknown Thread"),
 m_status(eSTATUS_NOT_RUNNING),
 m_hThread(INVALID_HANDLE_VALUE),
 m_threadID(0),
 m_bSignaled(false),
 m_pRunObject(pRunObject),
 m_bAutoDelete(bAutoDelete)
{
	if (m_pRunObject)
		m_pRunObject->SetThread(this);
}

CNtlThread::~CNtlThread(void)
{
	Close();
	CleanUp();
	if (m_bAutoDelete)
	{
		SAFE_DELETE(m_pRunObject);
	}
}

void CNtlThread::Init()
{
	m_status = eSTATUS_RUNNING;
}

void CNtlThread::Close()
{
	if( m_pRunObject )
	{
		m_pRunObject->Close();
	}
}

void CNtlThread::CleanUp()
{
	m_status = eSTATUS_DEAD;
}

void CNtlThread::Execute()
{
	Init();
	if (m_pRunObject)
	{
		m_pRunObject->Run();
	}
	CleanUp();
}

void CNtlThread::Start()
{
	// Run synchronously on non-Windows for now.
	Execute();
}

void CNtlThread::Join()
{
}

void CNtlThread::Wait()
{
}

int CNtlThread::Wait(unsigned int millisecs)
{
	(void)millisecs;
	return 0;
}

void CNtlThread::Exit()
{
	Close();
	CleanUp();
}

void CNtlThread::Notify(CNtlThread * pThread)
{
	if (pThread)
		pThread->SetSignaled(true);
}

CNtlThread * CNtlThread::GetCurrentThread()
{
	return m_pMainThread;
}

void CNtlThread::UnitTest()
{
}

void CNtlRunObject::Terminate()
{
	Close();
}

void CNtlRunObject::Wait()
{
}

int CNtlRunObject::Wait(unsigned int millisecs)
{
	(void)millisecs;
	return 0;
}

void CNtlRunObject::Exit()
{
	if (m_pOwner)
		m_pOwner->Exit();
}

const char * CNtlRunObject::GetName() const
{
	return m_pOwner ? m_pOwner->GetName() : "";
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThreadFactory::CNtlThreadFactory()
{
	m_bClosed = false;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThreadFactory::~CNtlThreadFactory()
{
	Shutdown();
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::GarbageCollect(bool bShutDown)
{
	// clean thread
	CNtlThread * pThread;

	{
		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*)m_ThreadList.GetFirst();
	}

	while( pThread )
	{
		if( pThread->IsAutoDelete() && 
			( bShutDown && !pThread->IsStatus( CNtlThread::eSTATUS_NOT_RUNNING ) ||
			 !bShutDown && pThread->IsStatus( CNtlThread::eSTATUS_DEAD ) ) )
		{
			pThread->Join();
			
			CNtlLock lock(&m_Mutex);

			pThread = (CNtlThread*) m_ThreadList.GetFirst();	// Reset to first
		}
		else
		{
			CNtlLock lock(&m_Mutex);

			pThread = (CNtlThread*) pThread->GetNext();
		}
	}

	// Exit thread
	CNtlLock lock(&m_Mutex);

	pThread = (CNtlThread*) m_ThreadList.GetFirst();
	while( pThread )
	{
		CNtlThread * pNext = (CNtlThread*) pThread->GetNext();
		if( pThread->IsStatus( CNtlThread::eSTATUS_NOT_RUNNING ) )
		{
			m_ThreadList.Remove( pThread );
			SAFE_DELETE( pThread );
		}

		pThread = pNext;
	}
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::SingleGarbageCollect(CNtlThread* pGarbageThread)
{
	CNtlLock lock(&m_Mutex);

	CNtlThread* pThread = (CNtlThread*)(m_ThreadList.GetFirst());
	while (NULL != pThread)
	{
		if (pGarbageThread == pThread)
		{
			if (pThread->IsStatus(CNtlThread::eSTATUS_NOT_RUNNING) ||
				pThread->IsStatus(CNtlThread::eSTATUS_PREPARING_TO_RUN))
			{
				m_ThreadList.Remove(pThread);
				SAFE_DELETE(pThread);
			}

			return;
		}

		pThread = (CNtlThread*) pThread->GetNext();
	}
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThread * CNtlThreadFactory::CreateThread(CNtlRunObject * pRunObject, const char * name, bool bAutoDelete)
{
	GarbageCollect();

	CNtlThread * pThread = new CNtlThread( pRunObject, name, bAutoDelete );
	if (NULL == pThread)
	{
		NTL_LOG_ASSERT("\"new CNtlThread( pRunObject, name, bAutoDelete )\" failed.");
		return NULL;
	}

	CNtlLock lock(&m_Mutex);

	m_ThreadList.Append(pThread);

	return pThread;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::Shutdown()
{
	CloseAll();
	JoinAll();
	GarbageCollect(true);
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::CloseAll()
{
	CNtlLock lock(&m_Mutex);

	if( false == m_bClosed )
	{
		m_bClosed = true;

		// Close all threads
		CNtlThread * pThread = (CNtlThread*) m_ThreadList.GetFirst();
		while( pThread )
		{
			pThread->Close();

			pThread = (CNtlThread*) pThread->GetNext();
		}
	}
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::JoinAll()
{
	CNtlThread * pThread;

	{
		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*)m_ThreadList.GetFirst();
	}

	while( pThread )
	{
		pThread->Join();

		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*) pThread->GetNext();
	}
}

#else // _WIN32

//---------------------------------------------------------------------------------------
// Key class ( Thread ????? ????? )
//---------------------------------------------------------------------------------------
class CThreadKey
{
public:
	CThreadKey()
	{
		m_dwKey = ::TlsAlloc();
		if( TLS_OUT_OF_INDEXES == m_dwKey )
		{
			THROW_THREAD_EXCEPTION( eTHREAD_ERR_TLS_KEY_CREATE, GetLastError() );
		}
	}

	~CThreadKey()
	{
		if (TLS_OUT_OF_INDEXES != m_dwKey)
		{
			::TlsFree(m_dwKey);

			m_dwKey = TLS_OUT_OF_INDEXES;
		}
	}

public:
	DWORD				GetKey() { return m_dwKey; }

private:

	DWORD				m_dwKey;
};


//---------------------------------------------------------------------------------------
// MainThread class  ( Thread ????? ????? ) Application?? ???? ?????? ?????? ??????? ????
//---------------------------------------------------------------------------------------
class CMainRun : public CNtlRunObject
{
public:
	void Run() { /*MainRun?? ?????? ????? ???.*/ };
};

class CMainThread : public CNtlThread
{
public:
	CMainThread() : CNtlThread(new CMainRun, "MainThread", true ) 
	{
		m_threadID = ::GetCurrentThreadId();
		Init();
	}
};

//---------------------------------------------------------------------------------------
// Helper class  ( Thread ????? ????? )
//---------------------------------------------------------------------------------------
class CThreadHelper
{
public:
	//- yoshiki : What if 1 or more of "new" operators return NULL?
	CThreadHelper()
	{
		CNtlThread::m_pThreadKey		= new CThreadKey;
		CNtlThread::m_pMainThread		= new CMainThread;
	}

	virtual ~CThreadHelper()
	{
		SAFE_DELETE( CNtlThread::m_pMainThread );
		SAFE_DELETE( CNtlThread::m_pThreadKey );
	}
};


char * s_thread_status_string[CNtlThread::MAX_STATUS] = 
{
	"STATUS_NOT_RUNNING",		// ???????? ??? ???? ( ??? ???? )
	"STATUS_PREPARING_TO_RUN",	// ???? ??? ????
	"STATUS_RUNNING",			// ???? ????
	"STATUS_PAUSED",			// ???? ????
	"STATUS_DEAD",				// ???? ????
};

const char * CNtlThread::GetStatusString()
{
	if( m_status >= MAX_STATUS )
		return "Unknown Status";

	return s_thread_status_string[m_status];
}


//------------------------------------------------------
// static function declare
//------------------------------------------------------
/*static void ThreadCleanUp(void * arg)
{
	CNtlThread * pThread = (CNtlThread*) arg;
	pThread->CleanUp();
}*/

static unsigned __stdcall ThreadMain(void * arg)
{
	CNtlThread * pThread = (CNtlThread*) arg;
	pThread->Execute();

	return 0;
}


//---------------------------------------------------------------------------------------
// static variable declare
//---------------------------------------------------------------------------------------
CThreadHelper	threadHelper;
CThreadKey *	CNtlThread::m_pThreadKey = 0;
CNtlThread *	CNtlThread::m_pMainThread = 0;

//------------------------------------------------------


//------------------------------------------------------
// ?????? Run?? ?????? ???? 1?? ???
//------------------------------------------------------
void CNtlThread::Init()
{
	InterlockedExchange((LONG*)&m_status, eSTATUS_RUNNING);

	if( !TlsSetValue( m_pThreadKey->GetKey(), (LPVOID) this ) )
	{
		THROW_THREAD_EXCEPTION( eTHREAD_ERR_TLS_SET_VALUE, GetLastError() );
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Close()
{
	if( m_pRunObject )
	{
		m_pRunObject->Close();
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::CleanUp()
{
	InterlockedExchange((LONG*)&m_status, eSTATUS_DEAD);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Execute()
{
	Init();

	try
	{
		m_pRunObject->Run();
	}
	catch( CNtlThreadException e )
	{
		e.Dump();
	}
	catch(...)
	{				
	}

	CleanUp();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Start()
{
	m_hThread = (HANDLE) _beginthreadex(NULL, 0, ThreadMain, this, 0, &m_threadID);

	if( !m_hThread )
	{
		THROW_THREAD_EXCEPTION(eTHREAD_ERR_THREAD_CREATE, GetLastError());
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Exit()
{
	CleanUp();
	_endthreadex(0);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Wait()
{
	//-- thread sleep

	m_event.Wait();

	//-- thread wakeup
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlThread::Wait(unsigned int millisecs)
{
	//-- thread sleep

	int rc = m_event.Wait( millisecs );

	//-- thread wakeup

	return rc;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Notify(CNtlThread * pThread)
{
	pThread->m_event.Signal();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThread::Join()
{
	int rc = ::WaitForSingleObject( m_hThread, INFINITE );
	if( WAIT_FAILED == rc )
	{
		THROW_THREAD_EXCEPTION( eTHREAD_ERR_THREAD_JOIN, GetLastError() );
	}
	else
	{
		::CloseHandle( m_hThread );
	}

	InterlockedExchange((LONG*)&m_status, eSTATUS_NOT_RUNNING);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThread::CNtlThread(CNtlRunObject * pRunObject, const char * name, bool bAutoDelete)
{
	// ??? ???? ????
	InterlockedExchange((LONG*)&m_status, eSTATUS_NOT_RUNNING);

	m_bAutoDelete			= bAutoDelete;
	m_bSignaled				= false;
	m_strName				= name;

	m_pRunObject			= pRunObject;

	m_pRunObject->SetThread( this );

	// ???? ????
	InterlockedExchange((LONG*)&m_status, eSTATUS_PREPARING_TO_RUN);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThread::~CNtlThread()
{
	//if( eSTATUS_NOT_RUNNING != m_status && this != CNtlThread::m_pMainThread  )
	//{
	//}

	if( m_bAutoDelete && m_pRunObject)
	{
		SAFE_DELETE( m_pRunObject );
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThread * CNtlThread::GetCurrentThread()
{
	return (CNtlThread*) TlsGetValue( m_pThreadKey->GetKey() );
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
// Thread ?????? ????? ???
// 1. ???????? ?????? ???? ??? ????
// 2. ?????? ??? ????? ???? DEAD ?????? THREAD?? ????
// ???????!! : GarbageCollect?? ??? Lock()?? ??? ?????.
//-----------------------------------------------------------------------------------
CNtlThreadFactory::CNtlThreadFactory()
{
	m_bClosed = false;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThreadFactory::~CNtlThreadFactory()
{
	Shutdown();
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::GarbageCollect(bool bShutDown)
{
	// clean thread
	CNtlThread * pThread;

	{
		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*)m_ThreadList.GetFirst();
	}

	while( pThread )
	{
		if( pThread->IsAutoDelete() && 
			( bShutDown && !pThread->IsStatus( CNtlThread::eSTATUS_NOT_RUNNING ) ||
			 !bShutDown && pThread->IsStatus( CNtlThread::eSTATUS_DEAD ) ) )
		{
			pThread->Join();
			
			CNtlLock lock(&m_Mutex);

			pThread = (CNtlThread*) m_ThreadList.GetFirst();	// ??????? ??????
		}
		else
		{
			CNtlLock lock(&m_Mutex);

			pThread = (CNtlThread*) pThread->GetNext();
		}
	}

	// Exit thread
	CNtlLock lock(&m_Mutex);

	pThread = (CNtlThread*) m_ThreadList.GetFirst();
	while( pThread )
	{
		CNtlThread * pNext = (CNtlThread*) pThread->GetNext();
		if( pThread->IsStatus( CNtlThread::eSTATUS_NOT_RUNNING ) )
		{
			m_ThreadList.Remove( pThread );
			SAFE_DELETE( pThread );
		}

		pThread = pNext;
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::SingleGarbageCollect(CNtlThread* pGarbageThread)
{
	CNtlLock lock(&m_Mutex);

	CNtlThread* pThread = (CNtlThread*)(m_ThreadList.GetFirst());
	while (NULL != pThread)
	{
		if (pGarbageThread == pThread)
		{
			if (pThread->IsStatus(CNtlThread::eSTATUS_NOT_RUNNING) ||
				pThread->IsStatus(CNtlThread::eSTATUS_PREPARING_TO_RUN))
			{
				m_ThreadList.Remove(pThread);
				SAFE_DELETE(pThread);
			}

			return;
		}

		pThread = (CNtlThread*) pThread->GetNext();
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlThread * CNtlThreadFactory::CreateThread(CNtlRunObject * pRunObject, const char * name, bool bAutoDelete)
{
	GarbageCollect();

	CNtlThread * pThread = new CNtlThread( pRunObject, name, bAutoDelete );
	if (NULL == pThread)
	{
		NTL_LOG_ASSERT("\"new CNtlThread( pRunObject, name, bAutoDelete )\" failed.");
		return NULL;
	}

	CNtlLock lock(&m_Mutex);

	m_ThreadList.Append(pThread);

	return pThread;
}


void CNtlThreadFactory::AllThreadDump()
{
	int nThreadCount = 0;
	bool bIsDump = false;

	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	CNtlLock lock(&m_Mutex);

	CNtlThread * pThread = (CNtlThread*)m_ThreadList.GetFirst();
	while (pThread)
	{
		if (pThread->IsStatus(CNtlThread::eSTATUS_RUNNING))
		{
			if (nThreadCount == 0)
				bIsDump = true;

			ERR_LOG(LOG_WARNING, "Handle(%d) Thread SnapShot", pThread->m_hThread);
		//	CNtlMiniDump::ThreadSnapshot(pThread->m_hThread, &SystemTime, bIsDump);

			++nThreadCount;
			bIsDump = false;
		}
		else
		{
			ERR_LOG(LOG_WARNING, "Handle(%d) Thread skip state(%s)", pThread->m_hThread, pThread->GetStatusString());
		}

		pThread = (CNtlThread*)pThread->GetNext();
	}
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::Shutdown()
{
	CloseAll();
	JoinAll();
	GarbageCollect(true);

	//Lock();

	//if( false == m_bClosed )
	//{
	//	m_bClosed = true;

	//	// ?????? ????
	//	CNtlThread * pThread = (CNtlThread*) m_pThreadList->GetFirst();
	//	while( pThread )
	//	{
	//		Unlock();

	//		pThread->Close();

	//		Lock();

	//		pThread = (CNtlThread*) pThread->GetNext();
	//	}
	//}

	//GarbageCollect( true );

	//Unlock();
}



//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::CloseAll()
{
	CNtlLock lock(&m_Mutex);

	if( false == m_bClosed )
	{
		m_bClosed = true;

		// ?????? ????
		CNtlThread * pThread = (CNtlThread*) m_ThreadList.GetFirst();
		while( pThread )
		{
			pThread->Close();

			pThread = (CNtlThread*) pThread->GetNext();
		}
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlThreadFactory::JoinAll()
{
	CNtlThread * pThread;

	{
		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*)m_ThreadList.GetFirst();
	}

	while( pThread )
	{
		pThread->Join();

		CNtlLock lock(&m_Mutex);
		pThread = (CNtlThread*) pThread->GetNext();
	}
}




#include <iostream>

using namespace std;
void CNtlThread::UnitTest()
{
	class CTestRun : public CNtlRunObject
	{
	public:

		CTestRun()
			:m_bRun(true) {}

		void Run()
		{
			while( m_bRun )
			{
				cout << "TestRun : " << this->GetName() << " : " << GetThread()->GetStatusString() << endl;

				Sleep( 33 );
			}
		}

		void SetRunFlag(bool bFlag) { m_bRun = bFlag; }

	private:

		bool volatile m_bRun;

	};

	try
	{
		CTestRun * pTestRun = new CTestRun;

		CNtlThread * pThread = tThreadFactory::Instance().CreateThread( pTestRun, "Test Thread");

		pThread->Start();

		Sleep(100);

		pTestRun->SetRunFlag( false );

		pThread->Join();
	}
	catch( CNtlThreadException e )
	{
		e.Dump();
	}
	catch( ... )
	{
	}
}

#endif // _WIN32