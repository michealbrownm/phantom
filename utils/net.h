/*
	phantom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	phantom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with phantom.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTILS_NET_H_
#define UTILS_NET_H_

#include "common.h"
#include "thread.h"
#include <asio.hpp>
#include <asio/ssl.hpp>

#ifndef WIN32
typedef int SOCKET;
#endif

namespace utils {
	class InetAddress {
	public:
		InetAddress();
		InetAddress(const InetAddress &address);
		explicit InetAddress(uint16_t port_litter_endian);
		explicit InetAddress(const std::string &ip);
		explicit InetAddress(const std::string &ip, uint16_t port_litter_endian);
		explicit InetAddress(uint32_t ip_big_endian, uint16_t port_litter_endian);
		explicit InetAddress(const struct sockaddr_in &addr);
		explicit InetAddress(const struct in_addr &in_addr);
		explicit InetAddress(const asio::ip::tcp::endpoint &endpoint);
		explicit InetAddress(const asio::ip::udp::endpoint &endpoint);

		std::string ToIp() const;
		std::string ToIpPort() const;

		asio::ip::tcp::endpoint tcp_endpoint() const;
		asio::ip::udp::endpoint udp_endpoint() const;

		// default copy/assignment are Okay
		const struct sockaddr_in &sock_addr_in() const { return addr_; }
		const struct sockaddr *sock_addr() const { return (const struct sockaddr *)&addr_; }
		socklen_t size() const { return sizeof(addr_); }
		void set_sock_addr(const struct sockaddr_in &addr) { addr_ = addr; }

		uint32_t IpBigEndian() const { return addr_.sin_addr.s_addr; }
		uint16_t PortBigEndian() const { return addr_.sin_port; }

		int32_t GetFamily() const { return AF_INET; }
		bool Resolve(const std::string &address);
		void SetPort(uint16_t port) { addr_.sin_port = htons(port); };
		uint16_t GetPort() const { return htons(addr_.sin_port); };

		void operator=(struct sockaddr_in addr);
		void operator=(const InetAddress &address);
		bool operator==(const InetAddress &address);

		bool IsNone() const;
		bool IsLoopback() const;
		bool IsAny() const;

		static InetAddress Any();
		static InetAddress Loopback();
		static InetAddress None();

	private:
		struct sockaddr_in addr_;
	};

	std::string GetPeerName(SOCKET s);

	typedef std::vector<utils::InetAddress> InetAddressVec;
	typedef std::list<utils::InetAddress> InetAddressList;

	class net {
	public:
		net() {};
		~net() {};

		static bool Initialize();
		static bool GetNetworkAddress(InetAddressVec &addresses);

	};


	class Socket {
	public:
		typedef enum SocketTypeEnum {
			SOCKET_TYPE_TCP = 0,
			SOCKET_TYPE_UDP = 1
		}SocketType;

		explicit Socket();
		~Socket();

		bool Create(SocketType type, const InetAddress &address);
		InetAddress local_address() const { return local_address_; };
		bool Close();
		bool SetBlock(bool block);
		void SetKeepAlive(bool on);
		void SetTcpNoDelay(bool on);

		bool Connect(const InetAddress &server);
		bool Connect(const InetAddress &server, int timeout_milli);
		bool Accept(Socket &new_socket);
		InetAddress peer_address() const { return peer_address_; };

		int Send(const void *buffer, int size);
		bool SendComplete(const void *buffer, int size);
		int Receive(void *buffer, int size);
		int SendTo(const void *buffer, int size, const utils::InetAddress &address);
		int ReceiveFrom(void *buffer, int size, const utils::InetAddress &address);

		SOCKET handle() const { return handle_; }
		bool IsValid() const { return handle_ != INVALID_HANDLE; }

		static bool IsNomralError(uint32_t error_code);
		static const SOCKET INVALID_HANDLE;
		static const int ERR_VALUE;

	protected:

		UTILS_DISALLOW_EVIL_CONSTRUCTORS(Socket);
		SOCKET handle_;
		InetAddress local_address_;
		utils::InetAddress peer_address_;
		bool blocked_;
	};

	class AsyncIoThread : public utils::Thread {
		friend class AsyncIo;
	protected:
		asio::io_service io_service;

	public:
		AsyncIoThread() {};
		virtual ~AsyncIoThread() {};

		virtual void Run();
		void Stop();
	};

	typedef std::vector<AsyncIoThread *> AsyncIoThreadArray;
	typedef std::shared_ptr<asio::io_service::work> work_ptr;

	class AsyncIo {
	public:
		AsyncIo();
		~AsyncIo();

		bool Create(size_t poll_size, size_t thread_count);
		bool Close();
		bool AttachServiceIo(asio::io_service *io);

		asio::io_service *GetIoService();
	private:
		bool enabled_;
		size_t next_id_;

	protected:
		AsyncIoThreadArray *threads_ptr_;
		std::vector<work_ptr> work_;

		asio::io_service *io_service_ptr_;
	};

	class AsyncSocket {
		DISALLOW_COPY_AND_ASSIGN(AsyncSocket);
	protected:
		AsyncIo *asyncio_ptr_;

		InetAddress local_address_;
		InetAddress peer_address_;

	protected:
		AsyncSocket(AsyncIo *asyncio_ptr);
		virtual ~AsyncSocket();

		virtual bool Bind(const utils::InetAddress &address) = 0;
		virtual bool Close() = 0;
		virtual bool IsValid() = 0;
		virtual bool SetKeepAlive(bool on) = 0;
		virtual bool SetReuse(bool on) = 0;

	public:
		InetAddress local_address() { return local_address_; };
		InetAddress peer_address() { return peer_address_; };
	};

	class AsyncSocketAcceptor;
	class AsyncSocketTcp : public AsyncSocket {
		friend class AsyncIo;
		friend class AsyncSocketAcceptor;
	public:
		AsyncSocketTcp(AsyncIo *asyncio_ptr);
		virtual ~AsyncSocketTcp();

	private:
		asio::ip::tcp::socket *tcpsocket_ptr_;
		char buffer_[utils::ETH_MAX_PACKET_SIZE];

	public:
		virtual bool Bind(const utils::InetAddress &address);
		virtual bool Close();
		virtual bool IsValid();
		virtual bool SetKeepAlive(bool on);
		virtual bool SetReuse(bool on);

		bool SetTcpNoDelay(bool on);
		bool Connect(const utils::InetAddress &server);
		bool AsyncConnect(const utils::InetAddress &server);
		size_t SendSome(const void *buffer, size_t size);
		int AsyncSendSome(const void *buffer, size_t size);
		size_t ReceiveSome(void *buffer, size_t size);
		int AsyncReceiveSome(size_t max_size = utils::ETH_MAX_PACKET_SIZE);

		virtual void OnError();
		virtual void OnConnect();
		virtual void OnSend(std::size_t bytes_transferred);
		virtual void OnReceive(void *buffer, size_t bytes_transferred);
	};

	class IAsyncSocketAcceptorNotify {
	public:
		IAsyncSocketAcceptorNotify() {};
		~IAsyncSocketAcceptorNotify() {};
		virtual void OnAccept(AsyncSocketAcceptor *acceptor) = 0;
		virtual void OnError(AsyncSocketAcceptor *acceptor) = 0;
	};

	class AsyncSocketSsl;
	class AsyncSocketAcceptor : public AsyncSocket {
		friend class AsyncIo;
	private:
		asio::ip::tcp::acceptor *acceptor_ptr_;
		IAsyncSocketAcceptorNotify *notify_lptr_;
		AsyncSocketTcp *tcpsocket_lptr_;
		AsyncSocketSsl *sslsocket_lptr_;
	public:
		AsyncSocketAcceptor(AsyncIo *asyncio_ptr, IAsyncSocketAcceptorNotify *notify_ptr = NULL);
		virtual ~AsyncSocketAcceptor();

		virtual bool Bind(const utils::InetAddress &address);
		virtual bool Close();
		virtual bool IsValid();
		virtual bool SetKeepAlive(bool on);
		virtual bool SetReuse(bool on);

		bool Listen(int back_log = SOMAXCONN);
		bool Accept(AsyncSocketTcp *new_socket);
		bool AsyncAccept(AsyncSocketTcp *new_socket);
		bool AsyncAccept(AsyncSocketSsl *new_socket);
		virtual void OnAccept();
		virtual void OnError();
	};

	class AsyncSocketUdp : public AsyncSocket {
		friend class AsyncIo;
	public:
		AsyncSocketUdp(AsyncIo *asyncio_ptr);
		virtual ~AsyncSocketUdp();

	private:
		asio::ip::udp::socket *udpsocket_ptr_;
		char buffer_[utils::ETH_MAX_PACKET_SIZE];
		asio::ip::udp::endpoint sender_endpoint_;

	public:
		virtual bool Bind(const utils::InetAddress &address);
		virtual bool Close();
		virtual bool IsValid();
		virtual bool SetKeepAlive(bool on);
		virtual bool SetReuse(bool on);

		size_t SendTo(const void *buffer, size_t size, const utils::InetAddress &address);
		size_t ReceiveFrom(void *buffer, size_t size, utils::InetAddress &address);
		int AsyncSendTo(const void *buffer, size_t size, const utils::InetAddress &address);
		int AsyncReceiveFrom(size_t max_size = utils::ETH_MAX_PACKET_SIZE);

		virtual void OnError();
		virtual void OnSend(size_t bytes_transferred);
		virtual void OnReceive(void *buffer, size_t bytes_transferred, const utils::InetAddress &address);
	};

	typedef asio::ssl::stream<asio::ip::tcp::socket> SslSocket;
	class AsyncSocketSsl : public AsyncSocket {
		friend class AsyncIo;
		friend class AsyncSocketAcceptor;
	public:
		AsyncSocketSsl(AsyncIo *asyncio_ptr, asio::ssl::context& context);
		virtual ~AsyncSocketSsl();

	private:
		SslSocket *sslsocket_ptr_;
		char buffer_[utils::ETH_MAX_PACKET_SIZE];

	public:
		virtual bool Bind(const utils::InetAddress &address);
		virtual bool Close();
		virtual bool IsValid();
		virtual bool SetKeepAlive(bool on);
		virtual bool SetReuse(bool on);

		bool SetTcpNoDelay(bool on);
		bool Connect(const utils::InetAddress &server);
		bool AsyncConnect(const utils::InetAddress &server);
		size_t SendSome(const void *buffer, size_t size);
		int AsyncSendSome(const void *buffer, size_t size);
		size_t ReceiveSome(void *buffer, size_t size);
		int AsyncReceiveSome(size_t max_size = utils::ETH_MAX_PACKET_SIZE);
		bool AsyncHandShake();
		bool HandShake();

		virtual void OnError();
		virtual void OnConnect();
		virtual void OnSend(std::size_t bytes_transferred);
		virtual void OnReceive(void *buffer, size_t bytes_transferred);

		virtual bool OnVerifyCertificate(bool preverified, asio::ssl::verify_context& ctx);
		virtual void OnHandShake();
	};


	class NameResolver {
	private:
		asio::ip::tcp::resolver *resolver_ptr_;
	public:
		NameResolver(asio::io_service &async_io_lptr);
		~NameResolver();

		bool Query(const std::string &name, utils::InetAddressList &list);

	};

}

#endif

