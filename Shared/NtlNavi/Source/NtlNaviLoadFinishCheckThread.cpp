#include "precomp_navi.h"
#include "NtlNaviLoadFinishCheckThread.h"
#if defined(_WIN32)
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <cerrno>
#endif
#include "NtlNaviLog.h"

#if !defined(_WIN32)
/* Linux: manual-reset event using pthread mutex + cond */
struct sLoadFinishEvent
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	volatile int signaled;
};
static void* CreateLoadFinishEvent(void)
{
	sLoadFinishEvent* ev = new sLoadFinishEvent;
	pthread_mutex_init(&ev->mutex, NULL);
	pthread_cond_init(&ev->cond, NULL);
	ev->signaled = 0;
	return ev;
}
static void ResetLoadFinishEvent(void* h)
{
	sLoadFinishEvent* ev = (sLoadFinishEvent*)h;
	pthread_mutex_lock(&ev->mutex);
	ev->signaled = 0;
	pthread_mutex_unlock(&ev->mutex);
}
static void SetLoadFinishEvent(void* h)
{
	sLoadFinishEvent* ev = (sLoadFinishEvent*)h;
	pthread_mutex_lock(&ev->mutex);
	ev->signaled = 1;
	pthread_cond_broadcast(&ev->cond);
	pthread_mutex_unlock(&ev->mutex);
}
static void CloseLoadFinishEvent(void* h)
{
	sLoadFinishEvent* ev = (sLoadFinishEvent*)h;
	pthread_cond_destroy(&ev->cond);
	pthread_mutex_destroy(&ev->mutex);
	delete ev;
}
static unsigned int WaitLoadFinishEvent(void* h, unsigned int ms)
{
	sLoadFinishEvent* ev = (sLoadFinishEvent*)h;
	struct timespec ts;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	ts.tv_sec = tv.tv_sec + ms / 1000;
	ts.tv_nsec = tv.tv_usec * 1000 + (ms % 1000) * 1000000;
	if (ts.tv_nsec >= 1000000000) { ts.tv_sec++; ts.tv_nsec -= 1000000000; }

	pthread_mutex_lock(&ev->mutex);
	while (!ev->signaled)
	{
		int r = pthread_cond_timedwait(&ev->cond, &ev->mutex, &ts);
		if (r == ETIMEDOUT) { pthread_mutex_unlock(&ev->mutex); return 0x00000102; } /* WAIT_TIMEOUT */
	}
	pthread_mutex_unlock(&ev->mutex);
	return 0; /* WAIT_OBJECT_0 */
}
#endif


//////////////////////////////////////////////////////////////////////////
//
//	CNtlNaviLoadFinishCheckThread
//
//////////////////////////////////////////////////////////////////////////


CNtlNaviLoadFinishCheckThread::CNtlNaviLoadFinishCheckThread( void )
{
	m_hLoadingFinish = NULL;

	m_bExit = false;
}

CNtlNaviLoadFinishCheckThread::~CNtlNaviLoadFinishCheckThread( void )
{
}

bool CNtlNaviLoadFinishCheckThread::Create( void )
{
#if defined(_WIN32)
	m_hLoadingFinish = CreateEvent( NULL, TRUE, FALSE, NULL );
#else
	m_hLoadingFinish = CreateLoadFinishEvent();
#endif

	if ( NULL == m_hLoadingFinish )
	{
		CNtlNaviLog::GetInstance()->Log( "[IMPORT] Creating the loading finish check thread failed." );

		return false;
	}

	SetExit( false );

	return true;
}

void CNtlNaviLoadFinishCheckThread::Delete( void )
{
	SetExit( true );

	if ( m_hLoadingFinish )
	{
#if defined(_WIN32)
		CloseHandle( m_hLoadingFinish );
#else
		CloseLoadFinishEvent( m_hLoadingFinish );
#endif
		m_hLoadingFinish = NULL;
	}
}

bool CNtlNaviLoadFinishCheckThread::Begin( void )
{
#if defined(_WIN32)
	ResetEvent( m_hLoadingFinish );

	SetExit( false );

	HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, &ThreaFuncCB, this, 0, NULL );

	if ( NULL == hThread )
	{
		return false;
	}

	Sleep( 15 );

	return true;
#else
	ResetLoadFinishEvent( m_hLoadingFinish );

	SetExit( false );

	pthread_t thread;
	if ( 0 != pthread_create( &thread, NULL, (void*(*)(void*))ThreaFuncCB, this ) )
	{
		return false;
	}

	pthread_detach( thread );

	usleep( 15000 );

	return true;
#endif
}

void CNtlNaviLoadFinishCheckThread::End( void )
{
	SetExit( true );

#if defined(_WIN32)
	SetEvent( m_hLoadingFinish );
#else
	SetLoadFinishEvent( m_hLoadingFinish );
#endif
}

HANDLE CNtlNaviLoadFinishCheckThread::GetLoadingFinishEvent( void )
{
	return m_hLoadingFinish;
}

#if !defined(_WIN32)
unsigned int CNtlNaviLoadFinishCheckThread::WaitForEvent( HANDLE hEvent, unsigned int ms )
{
	return hEvent ? WaitLoadFinishEvent( hEvent, ms ) : 0xffffffff;
}
#endif

bool CNtlNaviLoadFinishCheckThread::IsExit( void )
{
	m_clExitCS.Lock();
	bool bExit = m_bExit;
	m_clExitCS.Unlock();

	return bExit;
}

void CNtlNaviLoadFinishCheckThread::SetExit( bool bExit )
{
	m_clExitCS.Lock();
	m_bExit = bExit;
	m_clExitCS.Unlock();
}

#if defined(_WIN32)
unsigned int __stdcall CNtlNaviLoadFinishCheckThread::ThreaFuncCB( void* pParam )
#else
void* CNtlNaviLoadFinishCheckThread::ThreaFuncCB( void* pParam )
#endif
{
	CNtlNaviLoadFinishCheckThread* pThis = (CNtlNaviLoadFinishCheckThread*)pParam;

	while ( !pThis->IsExit() )
	{
		pThis->ThreadCallBackFunc();

#if defined(_WIN32)
		Sleep( 15 );
#else
		usleep( 15000 );
#endif
	}

#if defined(_WIN32)
	_endthread();

	return 0;
#else
	return NULL;
#endif
}