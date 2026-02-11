//***********************************************************************************
//
//	File		:	NtlAcceptingSessionList.cpp
//
//	Begin		:	25 OCT 17
//
//	Copyright	:	DBO GLOBAL
//
//	Author		:	DANEOS
//
//	Desc		:	
//
//***********************************************************************************


#include "stdafx.h"
#include "NtlAcceptingSessionList.h"
#include "NtlSession.h"
#include "NtlNetwork.h"
#include "NtlAcceptor.h"
#include "NtlAcceptor.h"


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlAcceptingSessionList::CNtlAcceptingSessionList()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlAcceptingSessionList::~CNtlAcceptingSessionList()
{
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlAcceptingSessionList::Add(CNtlSession * pSession)
{
	HSESSION handle = CNtlSessionListBase::_Add(pSession);

	if (handle == INVALID_HSESSION)
	{
		return false;
	}

	SetAcceptingSessionHandle(pSession, handle);

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
bool CNtlAcceptingSessionList::Remove(CNtlSession * pSession)
{
	HSESSION handle = CNtlSessionListBase::_Remove(pSession->GetAcceptingHandle());

	if (handle == INVALID_HSESSION)
	{
		return false;
	}

	SetAcceptingSessionHandle(pSession, INVALID_HSESSION);

	return true;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlAcceptingSessionList::ValidCheck(DWORD dwTickTime)
{
	UNREFERENCED_PARAMETER(dwTickTime);

	CNtlAutoMutex mutex(&m_mutex);
	mutex.Lock();
	
	NTL_PRINT(PRINT_SYSTEM, "m_sessionList size[%d]", m_sessionList.GetSize());

	int PostCount = 0;
	int PostFailCount = 0;

	CNtlSession * pSession = NULL;
	for (LISTIT it = m_sessionList.Begin(); it != m_sessionList.End(); )
	{
		pSession = *(m_sessionList.GetPtr(it));

		if (pSession && pSession->GetStatus() == CNtlConnection::STATUS_ACCEPT)
		{
			if (m_pNetworkRef->PostIocpEventMessage((WPARAM)NETEVENT_REMOVE_ACCEPTING, (LPARAM)pSession))
				++PostCount;
			else
				++PostFailCount;

			it = m_sessionList.Next(it);
			
			Remove(pSession);
		}
		else
		{
			it = m_sessionList.Next(it);
		}
	}

}

//-----------------------------------------------------------------------------------
//		Purpose	: Retry AcceptEx on sessions in accepting state
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlAcceptingSessionList::RetryAccept(CNtlAcceptor* pAcceptor)
{
	if (!pAcceptor)
		return;

	CNtlAutoMutex mutex(&m_mutex);
	mutex.Lock();
	
	CNtlSession * pSession = NULL;
	for (LISTIT it = m_sessionList.Begin(); it != m_sessionList.End(); )
	{
		pSession = *(m_sessionList.GetPtr(it));
		
		if (pSession && pSession->GetStatus() == CNtlConnection::STATUS_ACCEPT)
		{
			// Retry PostAccept - if a connection is now available, AcceptEx will succeed
			int rc = pSession->PostAccept(pAcceptor);
			if (rc == NTL_SUCCESS)
			{
				// AcceptEx succeeded or is pending - keep the session
				it = m_sessionList.Next(it);
			}
			else
			{
				// Real error - remove the session
				NTL_PRINT(PRINT_SYSTEM, "[RetryAccept] PostAccept failed with rc=%d, removing session=%p", rc, pSession);
				it = m_sessionList.Next(it);
				Remove(pSession);
			}
		}
		else
		{
			it = m_sessionList.Next(it);
		}
	}
}