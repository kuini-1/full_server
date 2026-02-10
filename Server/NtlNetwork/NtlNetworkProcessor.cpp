//***********************************************************************************
//
//	File		:	NetworkDispatcher.cpp
//
//	Begin		:	2007-01-04
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	
//
//***********************************************************************************

#include "stdafx.h"
#include "NtlNetworkProcessor.h"

#include "NtlNetwork.h"
#include "NtlSession.h"
#include "NtlPacket.h"

#include "NtlLog.h"
#include "NtlError.h"


//---------------------------------------------------------------------------------------
//
//---------------------------------------------------------------------------------------
const ULONG_PTR THREAD_CLOSE = (ULONG_PTR)(-1);	// thread terminate value
//---------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlNetworkProcessor::CNtlNetworkProcessor(CNtlNetwork *  pNetwork):
m_hEventIOCP( 0 )
{
	SetArg( pNetwork);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
CNtlNetworkProcessor::~CNtlNetworkProcessor()
{
	printf("[DEBUG] CNtlNetworkProcessor::~CNtlNetworkProcessor - Destructor called\n");
	fflush(stdout);
	Destroy();
	printf("[DEBUG] CNtlNetworkProcessor::~CNtlNetworkProcessor - Destroy() completed\n");
	fflush(stdout);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlNetworkProcessor::Create()
{
	if (NULL != m_hEventIOCP)
	{
		NTL_PRINT(PRINT_SYSTEM, "(NULL != m_hEventIOCP) m_hEventIOCP = %016x", m_hEventIOCP);
	}

	m_hEventIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if( NULL == m_hEventIOCP )
	{
		return GetLastError();
	}

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlNetworkProcessor::Destroy()
{
	printf("[DEBUG] CNtlNetworkProcessor::Destroy - Called, m_hEventIOCP = %p\n", m_hEventIOCP);
	fflush(stdout);
	if( NULL != m_hEventIOCP )
	{
		CloseHandle( m_hEventIOCP );
		m_hEventIOCP = NULL;
	}
	printf("[DEBUG] CNtlNetworkProcessor::Destroy - Completed\n");
	fflush(stdout);
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlNetworkProcessor::Run()
{
	printf("[DEBUG] CNtlNetworkProcessor::Run - Thread started\n");
	fflush(stdout);
	
	CNtlNetwork * pNetwork = (CNtlNetwork*) GetArg();
	if( NULL == pNetwork )
	{
		NTL_PRINT(PRINT_SYSTEM, "(NULL == pNetwork)");
		printf("[DEBUG] CNtlNetworkProcessor::Run - pNetwork is NULL, exiting\n");
		fflush(stdout);
		return;	
	}

	printf("[DEBUG] CNtlNetworkProcessor::Run - Entering main loop, m_hEventIOCP = %p\n", m_hEventIOCP);
	fflush(stdout);

	BOOL bResult = FALSE;
	DWORD dwBytesTransferred = 0;
	ULONG_PTR netEvent = INVALID_NETEVENT;
	CNtlSession * pSession = NULL;

	while( IsRunnable() )
	{	
		bResult = GetQueuedCompletionStatus(	m_hEventIOCP,
												&dwBytesTransferred,
												(ULONG_PTR*) &netEvent,
												(LPOVERLAPPED*) &pSession,
												INFINITE );

		printf("[DEBUG] CNtlNetworkProcessor::Run - Got completion status, bResult=%d, netEvent=%lu\n", bResult, (unsigned long)netEvent);
		fflush(stdout);

		if( THREAD_CLOSE == (ULONG_PTR) netEvent )
		{
			NTL_PRINT( PRINT_SYSTEM,"Thread Close" );
			printf("[DEBUG] CNtlNetworkProcessor::Run - Received THREAD_CLOSE, exiting\n");
			fflush(stdout);
			break;
		}	

		if( FALSE == bResult )
		{
			int rc = GetLastError();
			NTL_PRINT( PRINT_SYSTEM, "Dispatcher\tGQCS Failed : Err:%d(%s)", rc, NtlGetErrorMessage(rc) );
			continue;
		}

		if( NULL == pSession )
		{
			continue;
		}

		ProcessNetEvent(netEvent, pSession);

	} // end of while( m_bRunning )

}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlNetworkProcessor::ProcessNetEvent(ULONG_PTR netEvent, CNtlSession *pSession)
{
	switch (netEvent)
	{
		case NETEVENT_ACCEPT:
		{
			int rc = pSession->OnAccept();
			if (NTL_SUCCESS != rc)
			{
				pSession->Disconnect(false);
			}
		}
		break;

		case NETEVENT_CONNECT:
		{
			int rc = pSession->OnConnect();
			if (NTL_SUCCESS != rc)
			{
				pSession->Disconnect(false);
			}
		}
		break;

		case NETEVENT_CLOSE:
		{
			//static int nCount = 0;
			//NTL_PRINT(PRINT_SYSTEM, "Session[%X] NETEVENT_CLOSE Close Count[%u]", pSession, ++nCount );
			pSession->OnClose();
			pSession->SetDestroy();
		}
		break;

		case NETEVENT_RECV:
		{
			int rc = pSession->ProcessPacket();

			if (rc != NTL_SUCCESS && pSession->IsSetControlFlag(CNtlConnection::CONTROL_FLAG_CHECK_OPCODE))
				pSession->Disconnect(false);
		}
		break;

		case NETEVENT_DESTROY:
		{
			RELEASE_SESSION(pSession);
		}
		break;

		default: NTL_PRINT(PRINT_SYSTEM, "netEvent is not valid. netEvent = %d", netEvent); break;
	}
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlNetworkProcessor::PostNetEvent(WPARAM wParam, LPARAM lParam)
{
	if (NULL == m_hEventIOCP)
	{
		NTL_PRINT(PRINT_SYSTEM, "(NULL == m_hEventIOCP)");
	}

	if( 0 == PostQueuedCompletionStatus( m_hEventIOCP, 0, (ULONG_PTR)wParam, (LPOVERLAPPED)lParam ) )
	{
		return GetLastError();
	}

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
int CNtlNetworkProcessor::SendNetEvent(WPARAM wParam, LPARAM lParam)
{
	ProcessNetEvent(wParam, (CNtlSession*)lParam);

	return NTL_SUCCESS;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void CNtlNetworkProcessor::Close()
{
	printf("[DEBUG] CNtlNetworkProcessor::Close - Called, posting THREAD_CLOSE to IOCP\n");
	fflush(stdout);
	
	if (NULL == m_hEventIOCP)
	{
		printf("[DEBUG] CNtlNetworkProcessor::Close - ERROR: m_hEventIOCP is NULL!\n");
		fflush(stdout);
		CNtlRunObject::Close();
		return;
	}
	
	PostQueuedCompletionStatus( m_hEventIOCP, 0, THREAD_CLOSE, NULL );

	CNtlRunObject::Close();
}
