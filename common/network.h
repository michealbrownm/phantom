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

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <utils/net.h>
#include <utils/strings.h>
#include <utils/net.h>
#include <json/value.h>
#include <proto/cpp/common.pb.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/client.hpp>

namespace phantom {
	typedef websocketpp::client<websocketpp::config::asio_client> client;
	typedef websocketpp::client<websocketpp::config::asio_tls_client> tls_client;

	typedef websocketpp::server<websocketpp::config::asio> server;
	typedef websocketpp::server<websocketpp::config::asio_tls> tls_server;
	typedef websocketpp::lib::shared_ptr<asio::ssl::context> context_ptr;

	using websocketpp::connection_hdl;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;

	typedef std::set<connection_hdl, std::owner_less<connection_hdl> > con_list;

	enum tls_mode {
		MOZILLA_INTERMEDIATE = 1,
		MOZILLA_MODERN = 2
	};

	class Connection {
	private:
		server *server_;
		client *client_;
		tls_server *tls_server_;
		tls_client *tls_client_;
		connection_hdl handle_;

		//status
		int64_t connect_end_time_;

		int64_t last_receive_time_;

		std::string uri_;
		int64_t id_;
		bool in_bound_;
		utils::InetAddress peer_address_;

	protected:
		int64_t connect_start_time_;
		int64_t sequence_;
		int64_t last_send_time_;

	public:
		Connection(server *server_h, client *client_h,
			tls_server *tls_server_h, tls_client *tls_client_h, 
			connection_hdl con, const std::string &uri, int64_t id);
		virtual ~Connection();
		
		bool SendByteMessage(const std::string &message, std::error_code &ec);
		bool SendMsg(int64_t type, bool request, int64_t sequence, const std::string &data, std::error_code &ec);
		bool SendRequest(int64_t type, const std::string &data, std::error_code &ec);
		bool SendResponse(const protocol::WsMessage &req_message, const std::string &data, std::error_code &ec);
		bool Ping(std::error_code &ec);
		virtual bool PingCustom(std::error_code &ec);
		bool Close(const std::string &reason);

		bool NeedPing(int64_t interval);
		void TouchReceiveTime();
		void SetConnectTime();
		utils::InetAddress GetPeerAddress() const;
		int64_t GetId() const;
		connection_hdl GetHandle() const;
		websocketpp::lib::error_code GetErrorCode() const;
		bool InBound() const;

		//get status
		bool IsConnectExpired(int64_t time_out) const;
		bool IsDataExpired(int64_t time_out) const;
		virtual void ToJson(Json::Value &status) const;
		virtual bool OnNetworkTimer(int64_t current_time);
	};

	typedef std::map<int64_t, Connection *> ConnectionMap;
	typedef std::map<connection_hdl, int64_t, std::owner_less<connection_hdl>> ConnectHandleMap;

	typedef std::function<bool(protocol::WsMessage &message, int64_t conn_id)> MessageConnPoc;
	typedef std::map<int64_t, MessageConnPoc> MessageConnPocMap;

	class SslParameter {
	public:
		SslParameter();
		~SslParameter();

		bool enable_;
		std::string chain_file_;
		std::string private_key_file_;
		std::string tmp_dh_file_;
		std::string verify_file_;
		std::string cert_password_;
	};

	class Network {
	protected:
		asio::io_service io_;
		int64_t last_check_time_;
		int64_t connect_time_out_;

		server server_;
		tls_server tls_server_;
		client client_;
		tls_client tls_client_;

		ConnectionMap connections_;
		ConnectionMap connections_delete_;
		ConnectHandleMap connection_handles_;

		int64_t next_id_;
		bool enabled_;

		SslParameter ssl_parameter_;

		std::error_code ec_;
		utils::Mutex conns_list_lock_;
        uint16_t listen_port_;
	public:
		Network(const SslParameter &ssl_parameter);
		virtual ~Network();

		void Start(const utils::InetAddress &ip);
		void Stop();
		//for client
		bool Connect(std::string const & uri);
		uint16_t GetListenPort() const;
	protected:
		//for server
		void OnOpen(connection_hdl hdl);
		void OnClose(connection_hdl hdl);
		virtual void OnMessage(connection_hdl hdl, server::message_ptr msg);
		void OnFailed(connection_hdl hdl);

		//for client
		void OnClientOpen(connection_hdl hdl);
		//void OnClientMessage(connection_hdl hdl, server::message_ptr msg);

		//for tls
		context_ptr OnTlsInit(tls_mode mode, websocketpp::connection_hdl hdl);
		virtual bool OnValidate(websocketpp::connection_hdl hdl);
		virtual bool OnVerifyCallback(
			bool preverified, // True if the certificate passed pre-verification.
			asio::ssl::verify_context& ctx // The peer certificate and other context.
			);

		void OnPong(connection_hdl hdl, std::string payload);
		
		//get password
		std::string GetCertPassword();

		//Get peer object not thread safe
		Connection *GetConnection(int64_t id);
		Connection *GetConnection(connection_hdl hdl);

		//remove peer,  not thread safe
		void RemoveConnection(Connection *conn);
		void RemoveConnection(int64_t conn_id);

		//message type to function
		MessageConnPocMap request_methods_;
		MessageConnPocMap response_methods_;

		//send custom message
		bool OnRequestPing(protocol::WsMessage &message, int64_t conn_id);
		bool OnResponsePing(protocol::WsMessage &message, int64_t conn_id);

		//could be drived
		virtual Connection *CreateConnectObject(server *server_h, client *client_h, 
			tls_server *tls_server_h, tls_client *tls_client_h,
			connection_hdl con, const std::string &uri, int64_t id);
		virtual void OnDisconnect(Connection *conn) {};
		virtual bool OnConnectOpen(Connection *conn) { return true; };
	};

}
#endif
