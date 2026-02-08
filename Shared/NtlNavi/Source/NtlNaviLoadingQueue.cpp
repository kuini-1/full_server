#include "precomp_navi.h"
#include "NtlNaviLoadingQueue.h"
#if defined(_WIN32)
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>

struct AutoResetEvent
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	volatile int signaled;
};
#endif


//////////////////////////////////////////////////////////////////////////
//
//	CNtlNaviLoadEntity
//
//////////////////////////////////////////////////////////////////////////


bool CNtlNaviLoadingEntity::IsError( void )
{
	m_csError.Lock();
	bool bError = m_bError;
	m_csError.Unlock();

	return bError;
}

void CNtlNaviLoadingEntity::SetError( bool bError )
{
	m_csError.Lock();
	m_bError = bError;
	m_csError.Unlock();
}


//////////////////////////////////////////////////////////////////////////
//
//	CNtlLoadingQueue
//
//////////////////////////////////////////////////////////////////////////


CNtlLoadingQueue* CNtlLoadingQueue::s_pLoadingQueue = NULL;


CNtlLoadingQueue* CNtlLoadingQueue::GetInstance( void )
{
	return s_pLoadingQueue;
}


CNtlLoadingQueue::CNtlLoadingQueue( void )
{
	s_pLoadingQueue = this;

	m_bCreated = false;

	m_bExit = false;

	m_hEvent = NULL;

	memset( m_arhThread, 0, sizeof( m_arhThread ) );
}

CNtlLoadingQueue::~CNtlLoadingQueue( void )
{
	Delete();

	s_pLoadingQueue = NULL;
}

bool CNtlLoadingQueue::Create( void )
{
	Delete();

	m_bCreated = false;

	m_bExit = false;

#if defined(_WIN32)
	m_hEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	if ( NULL == m_hEvent )
	{
		goto error;
	}

	for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
	{
		m_arhThread[i] = (HANDLE)_beginthreadex( NULL, 0, &ThreaFuncCB, this, 0, NULL );

		if ( NULL == m_arhThread[i] )
		{
			goto error;
		}
	}

	Sleep( 15 );

	m_bCreated = true;

	return true;

error:

	for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
	{
		if ( m_arhThread[i] )
		{
			TerminateThread( m_arhThread[i], 0 );
			m_arhThread[i] = NULL;
		}
	}

	if ( m_hEvent )
	{
		CloseHandle( m_hEvent );
		m_hEvent = NULL;
	}

	m_bExit = false;

	m_bCreated = false;

	return false;
#else
	/* Linux: auto-reset event using pthread mutex + cond */
	AutoResetEvent* ev = new AutoResetEvent;
	pthread_mutex_init(&ev->mutex, NULL);
	pthread_cond_init(&ev->cond, NULL);
	ev->signaled = 0;
	m_hEvent = ev;

	for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
	{
		pthread_t* th = new pthread_t;
		if ( 0 != pthread_create( th, NULL, (void*(*)(void*))ThreaFuncCB, this ) )
		{
			delete th;
			goto error_linux;
		}
		m_arhThread[i] = th;
	}

	usleep( 15000 );

	m_bCreated = true;

	return true;

error_linux:
	SetExit();
	for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
	{
		if ( m_arhThread[i] )
		{
			pthread_join( *(pthread_t*)m_arhThread[i], NULL );
			delete (pthread_t*)m_arhThread[i];
			m_arhThread[i] = NULL;
		}
	}
	if ( m_hEvent )
	{
		AutoResetEvent* ev = (AutoResetEvent*)m_hEvent;
		pthread_cond_destroy(&ev->cond);
		pthread_mutex_destroy(&ev->mutex);
		delete ev;
		m_hEvent = NULL;
	}
	m_bExit = false;
	m_bCreated = false;
	return false;
#endif
}

void CNtlLoadingQueue::Delete( void )
{
	if ( m_bCreated )
	{
		SetExit();

#if defined(_WIN32)
		WaitForMultipleObjectsEx( eMAX_THREAD_CNT, m_arhThread, TRUE, INFINITE, FALSE );

		Sleep( 15 );

		for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
		{
			if ( m_arhThread[i] )
			{
				CloseHandle( m_arhThread[i] );
				m_arhThread[i] = NULL;
			}
		}

		if ( m_hEvent )
		{
			CloseHandle( m_hEvent );
			m_hEvent = NULL;
		}
#else
		for ( int i = 0; i < eMAX_THREAD_CNT; ++i )
		{
			if ( m_arhThread[i] )
			{
				pthread_join( *(pthread_t*)m_arhThread[i], NULL );
				delete (pthread_t*)m_arhThread[i];
				m_arhThread[i] = NULL;
			}
		}

		usleep( 15000 );

		if ( m_hEvent )
		{
			AutoResetEvent* ev = (AutoResetEvent*)m_hEvent;
			pthread_cond_destroy(&ev->cond);
			pthread_mutex_destroy(&ev->mutex);
			delete ev;
			m_hEvent = NULL;
		}
#endif

		m_bExit = false;

		m_bCreated = false;
	}
}

bool CNtlLoadingQueue::IsExit( void )
{
	m_clExitCS.Lock();
	bool bExit = m_bExit;
	m_clExitCS.Unlock();

	return bExit;
}

void CNtlLoadingQueue::SetExit( void )
{
	m_clExitCS.Lock();
	m_bExit = true;
	m_clExitCS.Unlock();

#if defined(_WIN32)
	SetEvent( m_hEvent );
#else
	if ( m_hEvent )
	{
		AutoResetEvent* ev = (AutoResetEvent*)m_hEvent;
		pthread_mutex_lock(&ev->mutex);
		ev->signaled = 1;
		pthread_cond_broadcast(&ev->cond);
		pthread_mutex_unlock(&ev->mutex);
	}
#endif
}

bool CNtlLoadingQueue::IsEmptyEntityToLoad( void )
{
	m_clEntityToLoadCS.Lock();
	bool bEmpty = m_defEntityToLoadList.empty();
	m_clEntityToLoadCS.Unlock();

	return bEmpty;
}

void CNtlLoadingQueue::AttachEntityToLoad( CNtlNaviLoadingEntity* pEntity )
{
	m_clEntityToLoadCS.Lock();
	m_defEntityToLoadList.push_back( pEntity );
	m_clEntityToLoadCS.Unlock();

#if defined(_WIN32)
	SetEvent( m_hEvent );
#else
	if ( m_hEvent )
	{
		AutoResetEvent* ev = (AutoResetEvent*)m_hEvent;
		pthread_mutex_lock(&ev->mutex);
		ev->signaled = 1;
		pthread_cond_signal(&ev->cond);
		pthread_mutex_unlock(&ev->mutex);
	}
#endif
}

bool CNtlLoadingQueue::DetachEntityToLoad( CNtlNaviLoadingEntity* pEntity )
{
	m_clEntityToLoadCS.Lock();

	bool bRet = false;

	for ( vecdef_ENTITY_LIST::iterator it = m_defEntityToLoadList.begin(); it != m_defEntityToLoadList.end(); ++it )
	{
		CNtlNaviLoadingEntity* pRetEntity = *it;

		if ( pEntity == pRetEntity )
		{
			m_defEntityToLoadList.erase( it );

			bRet = true;

			break;
		}
	}

	m_clEntityToLoadCS.Unlock();

	return bRet;
}

CNtlNaviLoadingEntity* CNtlLoadingQueue::TakeEntityToLoad( void )
{
	m_clEntityToLoadCS.Lock();

	CNtlNaviLoadingEntity* pEntity = NULL;

	if ( !m_defEntityToLoadList.empty() )
	{
		vecdef_ENTITY_LIST::iterator it = m_defEntityToLoadList.begin();

		pEntity = *it;

		m_defEntityToLoadList.erase( it );
	}

	m_clEntityToLoadCS.Unlock();

	return pEntity;
}

void CNtlLoadingQueue::AttachEntityLoaded( CNtlNaviLoadingEntity* pEntity )
{
	m_clEntityLoadedCS.Lock();
	m_defEntityLoadedList.push_back( pEntity );
	m_clEntityLoadedCS.Unlock();
}

bool CNtlLoadingQueue::DetachEntityLoaded( CNtlNaviLoadingEntity* pEntity )
{
	m_clEntityLoadedCS.Lock();

	bool bRet = false;

	for ( vecdef_ENTITY_LIST::iterator it = m_defEntityLoadedList.begin(); it != m_defEntityLoadedList.end(); ++it )
	{
		CNtlNaviLoadingEntity* pRetEntity = *it;

		if ( pEntity == pRetEntity )
		{
			m_defEntityLoadedList.erase( it );

			bRet = true;

			break;
		}
	}

	m_clEntityLoadedCS.Unlock();

	return bRet;
}

unsigned int CNtlLoadingQueue::ThreadCallBackFunc( void )
{
#if defined(_WIN32)
	while ( !IsExit() )
	{
		WaitForSingleObjectEx( m_hEvent, INFINITE, FALSE );

		if ( IsExit() )
		{
			SetEvent( m_hEvent );
			break;
		}

		CNtlNaviLoadingEntity* pEntity = TakeEntityToLoad();

		if ( pEntity )
		{
			SetEvent( m_hEvent );

			pEntity->RunMultiThread();
			AttachEntityLoaded( pEntity );
		}
	}

	return 0;
#else
	while ( !IsExit() )
	{
		AutoResetEvent* ev = (AutoResetEvent*)m_hEvent;
		pthread_mutex_lock(&ev->mutex);
		while ( !ev->signaled && !IsExit() )
			pthread_cond_wait(&ev->cond, &ev->mutex);
		int was_signaled = ev->signaled;
		ev->signaled = 0;
		pthread_mutex_unlock(&ev->mutex);

		if ( IsExit() )
		{
			pthread_mutex_lock(&ev->mutex);
			ev->signaled = 1;
			pthread_cond_broadcast(&ev->cond);
			pthread_mutex_unlock(&ev->mutex);
			break;
		}

		if ( !was_signaled )
			continue;

		CNtlNaviLoadingEntity* pEntity = TakeEntityToLoad();

		if ( pEntity )
		{
			pthread_mutex_lock(&ev->mutex);
			ev->signaled = 1;
			pthread_cond_signal(&ev->cond);
			pthread_mutex_unlock(&ev->mutex);

			pEntity->RunMultiThread();
			AttachEntityLoaded( pEntity );
		}
	}

	return 0;
#endif
}

#if defined(_WIN32)
unsigned int __stdcall CNtlLoadingQueue::ThreaFuncCB( void* pParam )
#else
void* CNtlLoadingQueue::ThreaFuncCB( void* pParam )
#endif
{
	((CNtlLoadingQueue*)pParam)->ThreadCallBackFunc();
#if defined(_WIN32)
	return 0;
#else
	return NULL;
#endif
}