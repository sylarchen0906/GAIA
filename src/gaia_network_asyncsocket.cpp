﻿#include <gaia_type.h>
#include <gaia_assert.h>
#include <gaia_network_base.h>
#include <gaia_network_asyncsocket.h>
#include <gaia_network_asyncdispatcher.h>

#include <gaia_assert_impl.h>
#include <gaia_thread_base_impl.h>
#include <gaia_network_base_impl.h>
#include <gaia_network_socket_impl.h>

#if GAIA_OS == GAIA_OS_WINDOWS
#	include <ws2tcpip.h>
#	include <mswsock.h>
#	include <winsock2.h>
#	include <windows.h>
#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
#	include <sys/types.h>
#	include <sys/event.h>
#	include <sys/time.h>
#	include <unistd.h>
#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID

#endif

namespace GAIA
{
	namespace NETWORK
	{
		AsyncSocket::AsyncSocket(GAIA::NETWORK::AsyncDispatcher& disp, GAIA::NETWORK::ASYNC_SOCKET_TYPE socktype)
		{
			this->init();
			m_pDispatcher = &disp;
			m_socktype = socktype;

			if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_ACCEPTING)
				m_pDispatcher->AddAcceptingSocket(*this);
			else if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_ACCEPTED)
				m_pDispatcher->AddAcceptedSocket(*this);
			else if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_CONNECTED)
				m_pDispatcher->AddConnectedSocket(*this);
		}

		AsyncSocket::~AsyncSocket()
		{
			if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_ACCEPTING)
				m_pDispatcher->RemoveAcceptingSocket(*this);
			else if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_ACCEPTED)
				m_pDispatcher->RemoveAcceptedSocket(*this);
			else if(m_socktype == GAIA::NETWORK::ASYNC_SOCKET_TYPE_CONNECTED)
				m_pDispatcher->RemoveConnectedSocket(*this);
		}

		GAIA::GVOID AsyncSocket::Create()
		{
		#if GAIA_OS == GAIA_OS_WINDOWS
			GAIA::N32 nSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
			if(nSocket == INVALID_SOCKET)
			{
				this->OnCreated(GAIA::False);
				return;
			}
			m_sock.SetFD(nSocket);
			m_sock.SetType(GAIA::NETWORK::Socket::SOCKET_TYPE_STREAM);
			DWORD dwBytes;
			GUID GuidAcceptEx = WSAID_ACCEPTEX;
			GUID GuidConnectEx = WSAID_CONNECTEX;
			GUID GuidDisonnectEx = WSAID_DISCONNECTEX;
			WSAIoctl(nSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
					 &GuidAcceptEx, sizeof(GuidAcceptEx), 
					 &m_pfnAcceptEx, sizeof(m_pfnAcceptEx), 
					 &dwBytes, NULL, NULL);
			WSAIoctl(nSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
					 &GuidConnectEx, sizeof(GuidConnectEx), 
					 &m_pfnConnectEx, sizeof(m_pfnConnectEx), 
					 &dwBytes, NULL, NULL);
			WSAIoctl(nSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, 
					 &GuidDisonnectEx, sizeof(GuidDisonnectEx), 
					 &m_pfnDisconnectEx, sizeof(m_pfnDisconnectEx), 
					 &dwBytes, NULL, NULL);
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			m_sock.Create(GAIA::NETWORK::Socket::SOCKET_TYPE_STREAM);
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			m_sock.Create(GAIA::NETWORK::Socket::SOCKET_TYPE_STREAM);
		#endif

			this->OnCreated(GAIA::True);
		}

		GAIA::GVOID AsyncSocket::Close()
		{
			m_sock.Close();
			this->OnClosed(GAIA::True);
		}

		GAIA::GVOID AsyncSocket::Shutdown(GAIA::N32 nShutdownFlag)
		{
			m_sock.Shutdown(nShutdownFlag);
			this->OnShutdowned(GAIA::True, nShutdownFlag);
		}

		GAIA::GVOID AsyncSocket::Bind(const GAIA::NETWORK::Addr& addr)
		{
			m_sock.Bind(addr);
			this->OnBound(GAIA::True, addr);
		}

		GAIA::GVOID AsyncSocket::Connect(const GAIA::NETWORK::Addr& addr)
		{
			GAST(addr.check());
			if(!addr.check())
				return;
			GAIA::NETWORK::Addr addrPeer;
			addrPeer.reset();
			this->GetPeerAddress(addrPeer);
			if(addrPeer.check())
				return;

			this->SetPeerAddress(addr);

		#if GAIA_OS == GAIA_OS_WINDOWS
			m_pDispatcher->attach_socket_iocp(*this);

			AsyncContext* pCtx = m_pDispatcher->alloc_async_ctx();
			pCtx->type = ASYNC_CONTEXT_TYPE_CONNECT;
			pCtx->pDataSocket = this;
			this->rise_ref();

			sockaddr_in saddr_in;
			GAIA::NETWORK::addr2saddr(addr, &saddr_in);
			GAIA::N32 nAddrLen = sizeof(SOCKADDR_IN) + 16;
			DWORD dwSent;
			if(!((LPFN_CONNECTEX)m_pfnConnectEx)(this->GetFD(), (sockaddr*)&saddr_in, nAddrLen, GNIL, 0, &dwSent, (OVERLAPPED*)pCtx))
			{
				DWORD err = WSAGetLastError();
				if(err != ERROR_IO_PENDING)
				{
					m_pDispatcher->release_async_ctx(pCtx);
					this->OnConnected(GAIA::False, addr);
					this->drop_ref();
					return;
				}
			}
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX

		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID

		#endif
		}

		GAIA::GVOID AsyncSocket::Disconnect()
		{
		#if GAIA_OS == GAIA_OS_WINDOWS
			AsyncContext* pCtx = m_pDispatcher->alloc_async_ctx();
			pCtx->type = ASYNC_CONTEXT_TYPE_DISCONNECT;
			pCtx->pDataSocket = this;
			this->rise_ref();

			if(!((LPFN_DISCONNECTEX)m_pfnDisconnectEx)(this->GetFD(), (OVERLAPPED*)pCtx, 0, 0))
			{
				DWORD err = WSAGetLastError();
				if(err != ERROR_IO_PENDING)
				{
					m_pDispatcher->release_async_ctx(pCtx);
					this->OnDisconnected(GAIA::False);
					this->drop_ref();
					return;
				}
			}
		#else
			this->Shutdown();
		#endif
		}

		GAIA::NUM AsyncSocket::Send(const GAIA::GVOID* pData, GAIA::NUM sSize)
		{
			GAIA::SYNC::Autolock al(m_lrSend);

		#if GAIA_OS == GAIA_OS_WINDOWS
			GAIA::NUM sPieceSize = gsizeofobj(AsyncContext, data);
			GAIA::NUM sPieceCount = sSize / sPieceSize + ((sSize % sPieceSize) != 0 ? 1 : 0);
			GAIA::NUM sOffset = 0;
			for(GAIA::NUM x = 0; x < sPieceCount; ++x)
			{
				GAIA::NUM sCurrentPieceSize = GAIA::ALGO::gmin(sSize - sOffset, sPieceSize);
				GAST(sCurrentPieceSize != 0);

				AsyncContext* pCtx = m_pDispatcher->alloc_async_ctx();
				pCtx->type = ASYNC_CONTEXT_TYPE_SEND;
				pCtx->pDataSocket = this;
				GAIA::ALGO::gmemcpy(pCtx->data, (const GAIA::U8*)pData + sOffset, sCurrentPieceSize);
				pCtx->_buf.len = sCurrentPieceSize;
				this->rise_ref();

				DWORD dwTrans = 0;
				DWORD dwFlag = 0;
				if(!::WSASend(this->GetFD(), &pCtx->_buf, 1, &dwTrans, dwFlag, (OVERLAPPED*)pCtx, GNIL))
				{
					DWORD err = WSAGetLastError();
					if(err != ERROR_IO_PENDING)
					{
						this->OnSent(GAIA::False, pData, sOffset, sSize);
						m_pDispatcher->release_async_ctx(pCtx);
						this->drop_ref();
						return sOffset;
					}
				}

				sOffset += sCurrentPieceSize;
			}

			GAST(sOffset == sSize);
			return sOffset;
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			return GINVALID;
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			return GINVALID;
		#endif

		}

		GAIA::GVOID AsyncSocket::init()
		{
			m_pDispatcher = GNIL;
			m_socktype = GAIA::NETWORK::ASYNC_SOCKET_TYPE_CONNECTED;

		#if GAIA_OS == GAIA_OS_WINDOWS
			m_pfnAcceptEx = GNIL;
			m_pfnConnectEx = GNIL;
			m_pfnDisconnectEx = GNIL;
		#endif
		}
	}
}
