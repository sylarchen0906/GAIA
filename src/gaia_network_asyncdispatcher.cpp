﻿#include <gaia_type.h>
#include <gaia_assert.h>
#include <gaia_network_asyncdispatcher.h>

#include <gaia_assert_impl.h>
#include <gaia_thread_base_impl.h>

#if GAIA_OS == GAIA_OS_WINDOWS
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
		class AsyncDispatcherThread : public GAIA::THREAD::Thread
		{
		public:
			GINL AsyncDispatcherThread(AsyncDispatcher* pDispatcher)
			{
				m_pDispatcher = pDispatcher;
			}

			virtual GAIA::GVOID Run()
			{
				for(;;)
				{
				}
			}

		private:
			AsyncDispatcher* m_pDispatcher;
		};

		AsyncDispatcher::AsyncDispatcher()
		{
			this->init();
		}

		AsyncDispatcher::~AsyncDispatcher()
		{
			if(this->IsCreated())
				this->Destroy();
		}

		GAIA::BL AsyncDispatcher::Create(const GAIA::NETWORK::AsyncDispatcher::Desc& desc)
		{
			if(this->IsCreated())
				return GAIA::False;
			if(!desc.check())
				return GAIA::False;

			m_threads.resize(desc.sThreadCount);
			for(GAIA::NUM x = 0; x < m_threads.size(); ++x)
				m_threads[x] = gnew AsyncDispatcherThread(this);

			m_desc = desc;
			m_bCreated = GAIA::True;
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::Destroy()
		{
			if(!this->IsCreated())
				return GAIA::False;

			if(this->IsBegin())
				this->End();

			for(GAIA::NUM x = 0; x < m_threads.size(); ++x)
			{
				AsyncDispatcherThread* pThread = (AsyncDispatcherThread*)m_threads[x];
				GAST(pThread != GNIL);
				gdel pThread;
			}
			m_threads.clear();

			m_desc.reset();
			m_bCreated = GAIA::False;
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::IsCreated() const
		{
			return m_bCreated;
		}

		const GAIA::NETWORK::AsyncDispatcher::Desc& AsyncDispatcher::GetDesc() const
		{
			return m_desc;
		}

		GAIA::BL AsyncDispatcher::Begin()
		{
			if(this->IsBegin())
				return GAIA::False;

			// Create async controller in OS.
		#if GAIA_OS == GAIA_OS_WINDOWS
			m_pIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			m_kqueue = kqueue();
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			m_epoll = epoll_create();
		#endif

			// Start thread.
			for(GAIA::NUM x = 0; x < m_threads.size(); ++x)
				m_threads[x]->Start();

			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::End()
		{
			if(!this->IsBegin())
				return GAIA::False;

			// Notify the thread exit.

			// End thread.
			for(GAIA::NUM x = 0; x < m_threads.size(); ++x)
				m_threads[x]->Wait();

			// Close async controller in OS.
		#if GAIA_OS == GAIA_OS_WINDOWS
			::CloseHandle(m_pIOCP);
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			close(m_kqueue);
			m_kqueue = GINVALID;
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			close(m_epoll);
			m_epoll = GINVALID;
		#endif

			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::IsBegin() const
		{
		#if GAIA_OS == GAIA_OS_WINDOWS
			if(m_pIOCP != GNIL)
				return GAIA::True;
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			if(m_kqueue != GINVALID)
				return GAIA::True;
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			if(m_epoll != GINVALID)
				return GAIA::True;
		#endif
			return GAIA::False;
		}

		GAIA::BL AsyncDispatcher::AddListenSocket(const GAIA::NETWORK::Addr& addr)
		{
			GAIA::SYNC::AutolockW al(m_rwListenSockets);

			Node nfinder;
			nfinder.addrLocal = addr;
			nfinder.pSock = GNIL;
			if(m_listen_sockets.find(nfinder) != GNIL)
				return GAIA::False;

			//
			AsyncSocket* pSock = this->OnCreateListenSocket();
			pSock->Create();
			pSock->Bind(addr);

		#if GAIA_OS == GAIA_OS_WINDOWS
			this->attach_socket_iocp(*pSock);
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX

		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID

		#endif

			nfinder.pSock = pSock;
			m_listen_sockets.insert(nfinder);

			pSock->m_sock.Listen();

		#if GAIA_OS == GAIA_OS_WINDOWS
			for(GAIA::NUM x = 0; x < m_desc.sAcceptEventCount; ++x)
			{
				GAIA::NETWORK::AsyncSocket* pAcceptedSocket = this->OnCreateAcceptedSocket();
				pAcceptedSocket->Create();

				GAIA::NETWORK::IOCPOverlapped* pIOCPOverlapped = this->alloc_iocpol();
				pIOCPOverlapped->type = IOCP_OVERLAPPED_TYPE_ACCEPT;
				pIOCPOverlapped->pListenSocket = pSock;
				pIOCPOverlapped->pAcceptedSocket = pAcceptedSocket;
				pSock->rise_ref();
				pAcceptedSocket->rise_ref();

				GAIA::N32 nAddrLen = sizeof(SOCKADDR_IN) + 16;
				DWORD dwRecved = 0;
				if(!((LPFN_ACCEPTEX)pSock->m_pfnAcceptEx)(pSock->GetFD(), pAcceptedSocket->GetFD(), pIOCPOverlapped->data, 0, nAddrLen, nAddrLen, &dwRecved, (OVERLAPPED*)pIOCPOverlapped))
				{
					DWORD err = WSAGetLastError();
					if(err != ERROR_IO_PENDING)
					{
						pSock->rise_ref();
						pAcceptedSocket->drop_ref();
						pAcceptedSocket->drop_ref();
						this->release_iocpol(pIOCPOverlapped);
						GERR << "GAIA AsyncDispatcher IOCP error, cannot AcceptEx, ErrorCode = " << ::WSAGetLastError() << GEND;
					}
				}
			}
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX

		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID

		#endif

			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::RemoveListenSocket(const GAIA::NETWORK::Addr& addr)
		{
			GAIA::SYNC::AutolockW al(m_rwListenSockets);

			Node nfinder;
			nfinder.addrLocal = addr;
			nfinder.pSock = GNIL;
			Node* pFinded = m_listen_sockets.find(nfinder);
			if(pFinded == GNIL)
				return GAIA::False;

			pFinded->pSock->Shutdown();
			pFinded->pSock->Close();
			pFinded->pSock->drop_ref();

			m_listen_sockets.erase(nfinder);

			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::RemoveListenSocketAll()
		{
			GAIA::SYNC::AutolockW al(m_rwListenSockets);
			if(m_listen_sockets.empty())
				return GAIA::False;

			for(GAIA::CTN::Set<Node>::it it = m_listen_sockets.frontit(); !it.empty(); ++it)
			{
				Node& n = *it;
				n.pSock->Shutdown();
				n.pSock->Close();
				n.pSock->drop_ref();
			}
			m_listen_sockets.clear();

			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::IsExistListenSocket(const GAIA::NETWORK::Addr& addr) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwListenSockets);
			Node nfinder;
			nfinder.addrLocal = addr;
			nfinder.pSock = GNIL;
			const Node* pFinded = m_listen_sockets.find(nfinder);
			if(pFinded == GNIL)
				return GAIA::False;
			return GAIA::True;
		}

		GAIA::NUM AsyncDispatcher::GetListenSocketCount() const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwListenSockets);
			return m_listen_sockets.size();
		}

		GAIA::BL AsyncDispatcher::CollectListenSocket(CallBack& cb) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwListenSockets);
			for(GAIA::CTN::Set<Node>::const_it it = m_listen_sockets.const_frontit(); !it.empty(); ++it)
			{
				const Node& n = *it;
				if(!cb.OnCollectAsyncSocket(*GCCAST(AsyncDispatcher*)(this), *n.pSock))
					return GAIA::False;
			}
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::IsExistAcceptedSocket(const GAIA::NETWORK::Addr& addr) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwAcceptedSockets);
			Node nfinder;
			nfinder.addrLocal = addr;
			nfinder.pSock = GNIL;
			const Node* pFinded  = m_accepted_sockets.find(nfinder);
			if(pFinded == GNIL)
				return GAIA::False;
			return GAIA::True;
		}

		GAIA::NUM AsyncDispatcher::GetAcceptedSocketCount() const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwAcceptedSockets);
			return m_accepted_sockets.size();
		}

		GAIA::BL AsyncDispatcher::CollectAcceptedSocket(CallBack& cb) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwAcceptedSockets);
			for(GAIA::CTN::Set<Node>::const_it it = m_accepted_sockets.const_frontit(); !it.empty(); ++it)
			{
				const Node& n = *it;
				if(!cb.OnCollectAsyncSocket(*GCCAST(AsyncDispatcher*)(this), *n.pSock))
					return GAIA::False;
			}
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::IsExistConnectedSocket(const GAIA::NETWORK::Addr& addr) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwConnectedSockets);
			Node nfinder;
			nfinder.addrLocal = addr;
			nfinder.pSock = GNIL;
			const Node* pFinded  = m_connected_sockets.find(nfinder);
			if(pFinded == GNIL)
				return GAIA::False;
			return GAIA::True;
		}

		GAIA::NUM AsyncDispatcher::GetConnectedSocketCount() const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwConnectedSockets);
			return m_connected_sockets.size();
		}

		GAIA::BL AsyncDispatcher::CollectConnectedSocket(CallBack& cb) const
		{
			GAIA::SYNC::AutolockR al(GCCAST(AsyncDispatcher*)(this)->m_rwConnectedSockets);
			for(GAIA::CTN::Set<Node>::const_it it = m_connected_sockets.const_frontit(); !it.empty(); ++it)
			{
				const Node& n = *it;
				if(!cb.OnCollectAsyncSocket(*GCCAST(AsyncDispatcher*)(this), *n.pSock))
					return GAIA::False;
			}
			return GAIA::True;
		}

		GAIA::NETWORK::AsyncSocket* AsyncDispatcher::OnCreateListenSocket()
		{
			GAIA::NETWORK::AsyncSocket* pListenSocket = gnew GAIA::NETWORK::AsyncSocket(*this, GAIA::NETWORK::AsyncSocket::ASYNC_SOCKET_TYPE_LISTEN);
			return pListenSocket;
		}

		GAIA::NETWORK::AsyncSocket* AsyncDispatcher::OnCreateAcceptedSocket()
		{
			GAIA::NETWORK::AsyncSocket* pAcceptedSocket = gnew GAIA::NETWORK::AsyncSocket(*this, GAIA::NETWORK::AsyncSocket::ASYNC_SOCKET_TYPE_ACCEPTED);
			return pAcceptedSocket;
		}

		GAIA::BL AsyncDispatcher::OnAcceptSocket(GAIA::NETWORK::AsyncSocket& sock, const GAIA::NETWORK::Addr& addrListen)
		{
			return GAIA::False;
		}

		GAIA::GVOID AsyncDispatcher::init()
		{
			m_bCreated = GAIA::False;
			m_desc.reset();
		#if GAIA_OS == GAIA_OS_WINDOWS
			m_pIOCP = GNIL;
		#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX
			m_kqueue = GINVALID;
		#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID
			m_epoll = GINVALID;
		#endif
		}

		GAIA::BL AsyncDispatcher::AddAcceptedSocket(GAIA::NETWORK::AsyncSocket& sock)
		{
			GAIA::SYNC::AutolockW al(m_rwAcceptedSockets);
			Node nfinder;
			GAIA::BL bResult = sock.GetLocalAddress(nfinder.addrLocal);
			GAST(bResult);
			nfinder.pSock = &sock;
			if(m_accepted_sockets.find(nfinder) != GNIL)
				return GAIA::False;
			m_accepted_sockets.insert(nfinder);
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::RemoveAcceptedSocket(GAIA::NETWORK::AsyncSocket& sock)
		{
			GAIA::SYNC::AutolockW al(m_rwAcceptedSockets);
			Node nfinder;
			GAIA::BL bResult = sock.GetLocalAddress(nfinder.addrLocal);
			GAST(bResult);
			nfinder.pSock = &sock;
			if(!m_accepted_sockets.erase(nfinder))
				return GAIA::False;
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::AddConnectedSocket(GAIA::NETWORK::AsyncSocket& sock)
		{
			GAIA::SYNC::AutolockW al(m_rwConnectedSockets);
			Node nfinder;
			GAIA::BL bResult = sock.GetLocalAddress(nfinder.addrLocal);
			GAST(bResult);
			nfinder.pSock = &sock;
			if(m_connected_sockets.find(nfinder) != GNIL)
				return GAIA::False;
			m_connected_sockets.insert(nfinder);
			return GAIA::True;
		}

		GAIA::BL AsyncDispatcher::RemoveConnectedSocket(GAIA::NETWORK::AsyncSocket& sock)
		{
			GAIA::SYNC::AutolockW al(m_rwConnectedSockets);
			Node nfinder;
			GAIA::BL bResult = sock.GetLocalAddress(nfinder.addrLocal);
			GAST(bResult);
			nfinder.pSock = &sock;
			if(!m_connected_sockets.erase(nfinder))
				return GAIA::False;
			return GAIA::True;
		}

	#if GAIA_OS == GAIA_OS_WINDOWS
		GAIA::NETWORK::IOCPOverlapped* AsyncDispatcher::alloc_iocpol()
		{
			GAIA::SYNC::Autolock al(m_lrIOCPOLPool);
			GAIA::NETWORK::IOCPOverlapped* pRet = m_IOCPOLPool.alloc();
			zeromem(&pRet->_ovlp);
			pRet->type = IOCP_OVERLAPPED_TYPE_INVALID;
			pRet->pListenSocket = GNIL;
			pRet->pAcceptedSocket = GNIL;
			pRet->_buf.len = 0;
			pRet->_buf.buf = (GAIA::CH*)pRet->data;
			return pRet;
		}

		GAIA::GVOID AsyncDispatcher::release_iocpol(GAIA::NETWORK::IOCPOverlapped* pIOCPOverlapped)
		{
			GAST(pIOCPOverlapped != GNIL);
			GAIA::SYNC::Autolock al(m_lrIOCPOLPool);
			return m_IOCPOLPool.release(pIOCPOverlapped);
		}

		GAIA::BL AsyncDispatcher::attach_socket_iocp(GAIA::NETWORK::AsyncSocket& sock)
		{
			if(!this->IsCreated())
				return GAIA::False;
			if(!this->IsBegin())
				return GAIA::False;
			if(!sock.IsCreated())
				return GAIA::False;
			::CreateIoCompletionPort((HANDLE)sock.GetFD(), m_pIOCP, 0, 0);
			return GAIA::True;
		}
	#elif GAIA_OS == GAIA_OS_OSX || GAIA_OS == GAIA_OS_IOS || GAIA_OS == GAIA_OS_UNIX

	#elif GAIA_OS == GAIA_OS_LINUX || GAIA_OS == GAIA_OS_ANDROID

	#endif
	}
}
