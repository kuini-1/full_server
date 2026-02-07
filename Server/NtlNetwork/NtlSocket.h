//***********************************************************************************
//
//	File		:	NtlSocket.h
//
//	Begin		:	2005-12-13
//
//	Copyright	:	?? NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	Socket Class
//
//***********************************************************************************

#pragma once

#if defined(_WIN32)
#include <mswsock.h>
#else
#include "../../Shared/NtlSharedCommon.h"
typedef void* PVOID;
typedef struct _GUID { unsigned long Data1; unsigned short Data2; unsigned short Data3; unsigned char Data4[8]; } GUID;
typedef int* LPINT;
typedef DWORD* LPDWORD;
typedef OVERLAPPED WSAOVERLAPPED;
typedef WSAOVERLAPPED* LPWSAOVERLAPPED;
typedef WSABUF* LPWSABUF;
typedef struct sockaddr SOCKADDR;
typedef SOCKADDR* LPSOCKADDR;
typedef void (*LPWSAOVERLAPPED_COMPLETION_ROUTINE)(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
typedef BOOL (*LPFN_ACCEPTEX)(SOCKET, SOCKET, PVOID, DWORD, DWORD, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (*LPFN_CONNECTEX)(SOCKET, const struct sockaddr*, int, PVOID, DWORD, LPDWORD, LPOVERLAPPED);
typedef BOOL (*LPFN_DISCONNECTEX)(SOCKET, LPOVERLAPPED, DWORD, DWORD);
typedef void (*LPFN_GETACCEPTEXSOCKADDRS)(PVOID, DWORD, DWORD, DWORD, struct sockaddr**, LPINT, struct sockaddr**, LPINT);
typedef BOOL (*LPFN_TRANSMITFILE)(SOCKET, HANDLE, DWORD, DWORD, LPOVERLAPPED, void*, DWORD);

#ifndef SOMAXCONN
#define SOMAXCONN 128
#endif
#ifndef ERROR_IO_PENDING
#define ERROR_IO_PENDING 997L
#endif
inline int WSAGetLastError() { return (int)GetLastError(); }
inline int WSARecv(SOCKET, WSABUF*, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) { SetLastError(ENOSYS); return SOCKET_ERROR; }
inline int WSASend(SOCKET, WSABUF*, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE) { SetLastError(ENOSYS); return SOCKET_ERROR; }
#endif
#include "NtlSockAddr.h"
#include "NtlString.h"

class CNtlSocket
{
public:
	enum
	{
		eSOCKET_TCP = 0,
		eSOCKET_UDP,
	};

public:

	CNtlSocket();

	virtual ~CNtlSocket();


public:

	static int							StartUp();

	static int							CleanUp();


protected:

	static int							LoadExtensionAPI();

	static int							LoadExtensionFunction(GUID functionID, LPVOID *pFunc);


public:

	int									Create(int nSocketType = 0);

	int									Bind(CNtlSockAddr& rSockAddr);

	int									Listen(int nBackLog = SOMAXCONN);

	int									Close();

	int									Shutdown(int how);

	int									Connect(struct sockaddr_in * sockaddr);

	int									SendStream(BYTE * pSendBuffer, int nSendSize, bool bSendOut);

	int									RecvStream(BYTE * pRecvBuffer, int nRecvSize);


	int									AcceptEx(CNtlSocket &rAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped);

	int									ConnectEx(const struct sockaddr* name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped);

	int									DisconnectEx(LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved);

	void								GetAcceptExSockaddrs(PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPSOCKADDR* LocalSockaddr, LPINT LocalSockaddrLength, LPSOCKADDR* RemoteSockaddr, LPINT RemoteSockaddrLength);

	int									RecvEx(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped);

	int									SendEx(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped);



	void								Attach(SOCKET socket) { m_socket = socket; }

	void								Detach() { m_socket = INVALID_SOCKET; }


	int									GetPeerName(CNtlString & rAddress, WORD & rPort);

	int									GetLocalName(CNtlString & rAddress, WORD & rPort);

	int									GetPeerAddr(CNtlSockAddr & rAddr);

	int									GetLocalAddr(CNtlSockAddr & rAddr);


	SOCKET								GetRawSocket() { return m_socket; }

	bool								IsCreated() { return INVALID_SOCKET != m_socket; }


	operator							SOCKET() { return *((SOCKET *)&m_socket); }

	CNtlSocket &							operator=(const CNtlSocket & rhs);


public:

	int									SetOption(DWORD dwSockOption);

	int									SetNonBlocking(BOOL bActive);

	int									SetReuseAddr(BOOL bActive);

	int									SetLinger(BOOL bActive, WORD wTime);

	int									SetTCPNoDelay(BOOL bActive);

	int									SetKeepAlive(BOOL bActive);

	int									SetKeepAlive(DWORD dwKeepAliveTime, DWORD dwKeepAliveInterval);

	int									SetConditionalAccept(BOOL bActive);

	int									GetCurReadSocketBuffer();




protected:


	SOCKET								m_socket;

	static LPFN_ACCEPTEX				m_lpfnAcceptEx;

	static LPFN_CONNECTEX				m_lpfnConnectEx;

	static LPFN_DISCONNECTEX			m_lpfnDisconnectEx;

	static LPFN_GETACCEPTEXSOCKADDRS	m_lpfnGetAcceptExSockAddrs;

	static LPFN_TRANSMITFILE			m_lpfnTransmitFile;

};


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline int CNtlSocket::AcceptEx(CNtlSocket &rAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped)
{
#if !defined(_WIN32)
	(void)rAcceptSocket; (void)lpOutputBuffer; (void)dwReceiveDataLength; (void)dwLocalAddressLength; (void)dwRemoteAddressLength; (void)lpdwBytesReceived; (void)lpOverlapped;
	return ENOSYS;
#else
	if( !m_lpfnAcceptEx( m_socket, rAcceptSocket.GetRawSocket(), lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped ) )
	{
		int rc = WSAGetLastError();
		if( ERROR_IO_PENDING !=  rc )
		{
			return rc;
		}
	}

	return NTL_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline int CNtlSocket::ConnectEx(const struct sockaddr* name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
#if !defined(_WIN32)
	(void)name; (void)namelen; (void)lpSendBuffer; (void)dwSendDataLength; (void)lpdwBytesSent; (void)lpOverlapped;
	return ENOSYS;
#else
	if( !m_lpfnConnectEx( m_socket, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped ) )
	{
		int rc = WSAGetLastError();
		if( ERROR_IO_PENDING !=  rc )
		{
			return rc;
		}
	}

	return NTL_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline int CNtlSocket::DisconnectEx(LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved)
{
#if !defined(_WIN32)
	(void)lpOverlapped; (void)dwFlags; (void)reserved;
	return ENOSYS;
#else
	if( !m_lpfnDisconnectEx( m_socket, lpOverlapped, dwFlags, reserved) )
	{
		int rc = WSAGetLastError();
		if( ERROR_IO_PENDING !=  rc )
		{
			return rc;
		}
	}

	return NTL_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline void CNtlSocket::GetAcceptExSockaddrs(PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPSOCKADDR* LocalSockaddr, LPINT LocalSockaddrLength, LPSOCKADDR* RemoteSockaddr, LPINT RemoteSockaddrLength)
{
#if !defined(_WIN32)
	(void)lpOutputBuffer; (void)dwReceiveDataLength; (void)dwLocalAddressLength; (void)dwRemoteAddressLength; (void)LocalSockaddr; (void)LocalSockaddrLength; (void)RemoteSockaddr; (void)RemoteSockaddrLength;
	return;
#else
	m_lpfnGetAcceptExSockAddrs(lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, LocalSockaddr, LocalSockaddrLength, RemoteSockaddr, RemoteSockaddrLength);
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline int CNtlSocket::RecvEx(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped)
{
#if !defined(_WIN32)
	(void)lpBuffers; (void)dwBufferCount; (void)lpNumberOfBytesRecvd; (void)lpFlags; (void)lpOverlapped;
	return ENOSYS;
#else
	if( 0 != ::WSARecv( m_socket, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, NULL) )
	{
		int rc = WSAGetLastError();
		if( ERROR_IO_PENDING !=  rc )
		{
			return rc;
		}
	}

	return NTL_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline int CNtlSocket::SendEx(LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED lpOverlapped)
{
#if !defined(_WIN32)
	(void)lpBuffers; (void)dwBufferCount; (void)lpNumberOfBytesSent; (void)dwFlags; (void)lpOverlapped;
	return ENOSYS;
#else
	if( 0 != ::WSASend( m_socket, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, NULL) )
	{
		int rc = WSAGetLastError();
		if( ERROR_IO_PENDING !=  rc )
		{
			return rc;
		}
	}

	return NTL_SUCCESS;
#endif
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
inline CNtlSocket & CNtlSocket::operator=(const CNtlSocket & rhs)
{
	m_socket = rhs.m_socket;
	return *this;
}

