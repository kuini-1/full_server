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
				// Retry PostRecv more frequently (every 10ms) to detect data faster
				static std::map<CNtlSession*, DWORD> s_lastRetryTime;
				static std::map<CNtlSession*, DWORD> s_retryCount;
				static std::map<CNtlSession*, DWORD> s_lastLogTime;
				DWORD dwNow = GetTickCount();
				DWORD dwLastRetry = s_lastRetryTime[pSession];
				if (dwLastRetry == 0 || (dwNow - dwLastRetry >= 10))
				{
					s_lastRetryTime[pSession] = dwNow;
					s_retryCount[pSession] = (s_retryCount[pSession] ? s_retryCount[pSession] : 0) + 1;
					// Log at most once per 5 minutes per session so important logs (packets, login) stay visible
					DWORD dwLastLog = s_lastLogTime[pSession];
					if (dwLastLog == 0 || (dwNow - dwLastLog >= 300000))
					{
						s_lastLogTime[pSession] = dwNow;
						NTL_PRINT(PRINT_SYSTEM, "[ValidCheck] PostRecv poll Session=%p, IP=%s (retry #%u)", pSession, pSession->GetRemoteIP(), s_retryCount[pSession]);
					}
					int rc = pSession->PostRecv();
					static DWORD s_dwLastErrorLog = 0;
					if (rc != NTL_SUCCESS && rc != NTL_ERR_NET_SESSION_CLOSED && (dwNow - s_dwLastErrorLog > 5000))
					{
						s_dwLastErrorLog = dwNow;
						NTL_PRINT(PRINT_SYSTEM, "[ValidCheck] PostRecv retry returned error: %d for Session=%p, IP=%s", rc, pSession, pSession->GetRemoteIP());
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