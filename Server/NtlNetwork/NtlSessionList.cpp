//***********************************************************************************
//
//	File		:	SessionList.cpp
//
//	Begin		:	2007-01-02
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "NtlSessionList.h"
#include "NtlSession.h"
#include "NtlNetwork.h"
#if !defined(_WIN32)
#include <map>
#endif


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlSessionList::CNtlSessionList()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlSessionList::~CNtlSessionList()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlSessionList::Add(CNtlSession * pSession)
{
	HSESSION handle = CNtlSessionListBase::_Add(pSession);

	if (handle == INVALID_HSESSION)
	{
		return false;
	}

	SetSessionHandle(pSession, handle);

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlSessionList::Remove(CNtlSession * pSession)
{
	HSESSION handle = CNtlSessionListBase::_Remove(pSession);

	if (handle == INVALID_HSESSION)
	{
		return false;
	}

	SetSessionHandle(pSession, INVALID_HSESSION);

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlSessionList::ValidCheck(DWORD dwTickTime)
{
	CNtlAutoMutex mutex( &m_mutex );
	mutex.Lock();

	//DWORD dwTime = timeGetTime();

	CNtlSession * pSession = NULL;
	for( LISTIT it = m_sessionList.Begin(); it != m_sessionList.End(); )
	{
		pSession = *( m_sessionList.GetPtr( it ) );

		if( pSession )
		{
			//((void (__fastcall *)(CNtlConnection *, _QWORD))v9->vfptr[1].GetAliveCheckTime)(v9, dwTime);

		//	if (false == pSession->CheckAuth(dwTime))
		//	{
		//		NTL_PRINT(PRINT_SYSTEM, "The session[%X] closed by unauthorized.", pSession);
		//		pSession->Disconnect(false);
		//	}
			/*else*/ if( false == pSession->ValidCheck( dwTickTime ) )
			{
				NTL_PRINT(PRINT_SYSTEM, "The session[%X] should be disconnected due to timeout.", pSession);
				pSession->Disconnect( false );	
			}
			else if (pSession->PacketLogTime(dwTickTime))
				PacketLog(pSession);

#if !defined(_WIN32)
			// On Linux, retry PostRecv for active sessions to detect when data arrives
			// This is needed because RecvEx returns ERROR_IO_PENDING when no data is available,
			// and we need to retry PostRecv when data arrives
			if (pSession->IsStatus(CNtlConnection::STATUS_ACTIVE))
			{
				// Retry PostRecv occasionally (every 50ms) to check for incoming data
				// Use a per-session timestamp to avoid calling too frequently
				static std::map<CNtlSession*, DWORD> s_lastRetryTime;
				DWORD dwNow = GetTickCount();
				DWORD dwLastRetry = s_lastRetryTime[pSession];
				if (dwLastRetry == 0 || (dwNow - dwLastRetry >= 50))
				{
					s_lastRetryTime[pSession] = dwNow;
					// PostRecv will use select() to check if data is available
					// If data is available, it will receive it and post to IOCP
					// If not, it will return ERROR_IO_PENDING (which we ignore here)
					int rc = pSession->PostRecv();
					// Only log if there's an actual error (not ERROR_IO_PENDING which is expected)
					if (rc != NTL_SUCCESS && rc != NTL_ERR_NET_SESSION_CLOSED)
					{
						NTL_PRINT(PRINT_SYSTEM, "[ValidCheck] PostRecv retry returned error: %d for Session=%p", rc, pSession);
					}
				}
			}
#endif

			if( true == pSession->IsShutdownable() ) //closed connections will be removed here
			{
				pSession->Shutdown();

				m_pNetworkRef->PostNetEventMessage( (WPARAM)NETEVENT_CLOSE, (LPARAM)pSession );
			}

			if (pSession->CanbeDestroy())
			{
				it = m_sessionList.Remove(it);
				m_pNetworkRef->PostNetEventMessage((WPARAM)NETEVENT_DESTROY, (LPARAM)pSession);

				continue;
			}

		}

		it = m_sessionList.Next( it );
	}

}