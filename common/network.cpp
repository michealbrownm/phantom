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

#include <utils/timestamp.h>
#include <utils/logger.h>
#include "general.h"
#include "network.h"

#define OVERLAY_PING 1
namespace phantom {

	Connection::Connection(server *server_h, client *client_h, 
		tls_server *tls_server_h, tls_client *tls_client_h, 
		connection_hdl con, const std::string &uri, int64_t id) :
		server_(server_h),
		client_(client_h),
		tls_server_(tls_server_h),
		tls_client_(tls_client_h),
		handle_(con),
		uri_(uri), 
		id_(id), 
		sequence_(0){
		connect_start_time_ = 0;
		connect_end_time_ = 0;
		last_receive_time_ = 0;
		last_send_time_ = 0;

		std::error_code ec;
		last_receive_time_ = connect_start_time_ = utils::Timestamp::HighResolution();
		if (server_ || tls_server_){
			in_bound_ = true;
			connect_end_time_ = connect_start_time_;
			
			if (server_) {
				server::connection_ptr con = server_->get_con_from_hdl(handle_, ec);
				if (!ec) peer_address_ = utils::InetAddress(con->get_remote_endpoint());
			}
			else {
				tls_server::connection_ptr con = tls_server_->get_con_from_hdl(handle_, ec);
				if (!ec) peer_address_ = utils::InetAddress(con->get_remote_endpoint());
			}

		}
		else {
			in_bound_ = false;
			if (client_) {
				client::connection_ptr con = client_->get_con_from_hdl(handle_, ec);
				if (!ec) peer_address_ = utils::InetAddress(con->get_host(), con->get_port());
			}
			else {
				tls_client::connection_ptr con = tls_client_->get_con_from_hdl(handle_, ec);
				if (!ec) peer_address_ = utils::InetAddress(con->get_host(), con->get_port());
			}
		}
	}
	Connection::~Connection() {}

	utils::InetAddress Connection::GetPeerAddress() const {
		return peer_address_;
	}

	void Connection::TouchReceiveTime() {
		last_receive_time_ = utils::Timestamp::HighResolution();
	}

	bool Connection::NeedPing(int64_t interval) {
		return connect_end_time_ > 0 && utils::Timestamp::HighResolution() - last_send_time_ > interval;
	}

	void Connection::SetConnectTime() {
		connect_end_time_ = utils::Timestamp::HighResolution();
		last_receive_time_ = connect_end_time_;
	}

	int64_t Connection::GetId() const{
		return id_;
	}

	connection_hdl Connection::GetHandle() const {
		return handle_;
	}

	websocketpp::lib::error_code Connection::GetErrorCode() const {
		std::error_code ec;
		if (in_bound_) {
			if (server_) {
				server::connection_ptr con = server_->get_con_from_hdl(handle_, ec);
				if (!ec) {
					ec = con->get_ec();
				}
			}
			else {
				tls_server::connection_ptr con = tls_server_->get_con_from_hdl(handle_, ec);
				if (!ec) {
					ec = con->get_ec();
				}
			}
		}
		else {
			if (client_) {
				client::connection_ptr con = client_->get_con_from_hdl(handle_, ec);
				if (!ec) {
					ec = con->get_ec();
				}
			}
			else {
				tls_client::connection_ptr con = tls_client_->get_con_from_hdl(handle_, ec);
				if (!ec) {
					ec = con->get_ec();
				}
			}
		}

		return ec;
	}

	bool Connection::InBound() const {
		return in_bound_;
	}

	bool Connection::SendByteMessage(const std::string &message, std::error_code &ec) {
		std::error_code ec1;
		if (in_bound_){
			if (server_) {
				server_->send(handle_, message, websocketpp::frame::opcode::BINARY, ec1);
			}
			else {
				tls_server_->send(handle_, message, websocketpp::frame::opcode::BINARY, ec1);
			}
		} else{
			if (client_) {
				client_->send(handle_, message, websocketpp::frame::opcode::BINARY, ec1);
			}
			else {
				tls_client_->send(handle_, message, websocketpp::frame::opcode::BINARY, ec1);
			}
		}

		if (ec1.value() == 0) {
			return true;
		} else{
			ec = ec1;
			return false;
		}
	}

	bool Connection::Ping(std::error_code &ec) {
		do {
			std::error_code ec1;
			std::string payload = utils::String::Format("%s - %d", utils::Timestamp::Now().ToFormatString(true).c_str(), rand());
			
			if (in_bound_) {
				if (server_) {
					server::connection_ptr con = server_->get_con_from_hdl(handle_, ec1);
					if (ec1.value() != 0) break;
					con->ping(payload, ec);
				} else{
					tls_server::connection_ptr con = tls_server_->get_con_from_hdl(handle_, ec1);
					if (ec1.value() != 0) break;
					con->ping(payload, ec);
				}
			}
			else {
				if (client_) {
					client::connection_ptr con = client_->get_con_from_hdl(handle_, ec1);
					if (ec1.value() != 0) break;
					con->ping(payload, ec);
				}
				else {
					tls_client::connection_ptr con = tls_client_->get_con_from_hdl(handle_, ec1);
					con->ping(payload, ec);
					if (ec1.value() != 0) break;
				}
			}

			last_send_time_ = utils::Timestamp::HighResolution();
		} while (false);

		LOG_TRACE("Sent ping to ip (%s),error code(%d)", GetPeerAddress().ToIpPort().c_str(), ec.value());
		return ec.value() == 0;
	}

	bool Connection::PingCustom(std::error_code &ec) {
		protocol::Ping ping;
		ping.set_nonce(utils::Timestamp::HighResolution());
		bool ret = SendRequest(OVERLAY_PING, ping.SerializeAsString(), ec);
		last_send_time_ = utils::Timestamp::HighResolution();
		LOG_TRACE("Sent custom ping to ip(%s),error code(%d:%s)", GetPeerAddress().ToIpPort().c_str(), ec.value(), ec.message().c_str());
		return !ec;
	}

	bool Connection::SendMsg(int64_t type, bool request, int64_t sequence, const std::string &data, std::error_code &ec) {
		protocol::WsMessage message;
		message.set_type(type);
		message.set_request(request);
		message.set_sequence(sequence);
		message.set_data(data);
		return SendByteMessage(message.SerializeAsString(), ec);
	}

	bool Connection::SendRequest(int64_t type, const std::string &data, std::error_code &ec) {
		protocol::WsMessage message;
		message.set_type(type);
		message.set_request(true);
		message.set_sequence(sequence_++);
		message.set_data(data);
		return SendByteMessage(message.SerializeAsString(), ec);
	}

	bool Connection::SendResponse(const protocol::WsMessage &req_message, const std::string &data, std::error_code &ec) {
		return SendMsg(req_message.type(), false, req_message.sequence(), data, ec);
	}

	bool Connection::Close(const std::string &reason) {
		std::error_code ec1;
		if (in_bound_) {
			if (server_) {
				server_->pause_reading(handle_, ec1);
				server_->close(handle_, 0, reason, ec1);
			}
			else {
				tls_server_->pause_reading(handle_, ec1);
				tls_server_->close(handle_, 0, reason, ec1);
			}
		} else{
			if (client_) {
				client_->close(handle_, 0, reason, ec1);
			}
			else {
				tls_client_->close(handle_, 0, reason, ec1);
			}
		}

		return ec1.value() == 0;
	}

	bool Connection::IsConnectExpired(int64_t time_out) const {
		return connect_end_time_ == 0 &&
			utils::Timestamp::HighResolution() - connect_start_time_ > time_out &&
			!in_bound_;
	}

	bool Connection::IsDataExpired(int64_t time_out) const {
		return connect_end_time_ > 0 && utils::Timestamp::HighResolution() - last_receive_time_ > time_out;
	}

	void Connection::ToJson(Json::Value &status) const {
		status["id"] = id_;
		status["in_bound"] = in_bound_;
		status["peer_address"] = GetPeerAddress().ToIpPort();
		status["last_receive_time"] = last_receive_time_;
	}

	bool Connection::OnNetworkTimer(int64_t current_time) { return true; }

	SslParameter::SslParameter() :enable_(false) {}
	SslParameter::~SslParameter() {}

	Network::Network(const SslParameter &ssl_parameter) : next_id_(0), enabled_(false), ssl_parameter_(ssl_parameter) {
		last_check_time_ = 0;
		connect_time_out_ = 60 * utils::MICRO_UNITS_PER_SEC;
		std::error_code err;
		if (ssl_parameter.enable_) {
			tls_server_.init_asio(&io_);
			tls_server_.set_reuse_addr(true);
			// Register handler callbacks
			tls_server_.set_open_handler(bind(&Network::OnOpen, this, _1));
			tls_server_.set_close_handler(bind(&Network::OnClose, this, _1));
			tls_server_.set_fail_handler(bind(&Network::OnFailed, this, _1));
			tls_server_.set_message_handler(bind(&Network::OnMessage, this, _1, _2));
			tls_server_.set_pong_handler(bind(&Network::OnPong, this, _1, _2));
			tls_server_.set_tls_init_handler(bind(&Network::OnTlsInit, this, MOZILLA_MODERN, _1));
			tls_server_.set_validate_handler(bind(&Network::OnValidate, this, _1));

			tls_server_.clear_access_channels(websocketpp::log::alevel::all);
			tls_server_.clear_error_channels(websocketpp::log::elevel::all);
		}
		else {
			server_.init_asio(&io_);
			server_.set_reuse_addr(true);
			// Register handler callbacks
			server_.set_open_handler(bind(&Network::OnOpen, this, _1));
			server_.set_close_handler(bind(&Network::OnClose, this, _1));
			server_.set_fail_handler(bind(&Network::OnFailed, this, _1));
			server_.set_message_handler(bind(&Network::OnMessage, this, _1, _2));
			server_.set_pong_handler(bind(&Network::OnPong, this, _1, _2));

			server_.clear_access_channels(websocketpp::log::alevel::all);
			server_.clear_error_channels(websocketpp::log::elevel::all);
		}

		if (ssl_parameter_.enable_) {
			tls_client_.init_asio(&io_);
			tls_client_.clear_access_channels(websocketpp::log::alevel::all);
			tls_client_.clear_error_channels(websocketpp::log::elevel::all);
			tls_client_.set_tls_init_handler(bind(&Network::OnTlsInit, this, MOZILLA_MODERN, _1));
			tls_client_.set_validate_handler(bind(&Network::OnValidate, this, _1));
		}
		else {
			client_.init_asio(&io_);
			client_.clear_access_channels(websocketpp::log::alevel::all);
			client_.clear_error_channels(websocketpp::log::elevel::all);
		}

		if (err.value() != 0){
			LOG_ERROR_ERRNO("Failed to initiate websocket network", err.value(), err.message().c_str());
		}

		//Register function
		request_methods_[OVERLAY_PING] = std::bind(&Network::OnRequestPing, this, std::placeholders::_1, std::placeholders::_2);
		response_methods_[OVERLAY_PING] = std::bind(&Network::OnResponsePing, this, std::placeholders::_1, std::placeholders::_2);
	}

	Network::~Network() {
		for (ConnectionMap::iterator iter = connections_.begin();
			iter != connections_.end();
			iter++) {
			delete iter->second;
		}

		for (ConnectionMap::iterator iter = connections_delete_.begin();
			iter != connections_delete_.end();
			iter++) {
			delete iter->second;
		}
	}

	void Network::OnOpen(connection_hdl hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		int64_t new_id = next_id_++;
		Connection *conn = CreateConnectObject(ssl_parameter_.enable_ ? NULL : &server_, NULL, 
			ssl_parameter_.enable_ ? &tls_server_ : NULL, NULL , hdl, "", new_id);
		connections_.insert(std::make_pair(new_id, conn));
		connection_handles_.insert(std::make_pair(hdl, new_id));

		LOG_INFO("Accepted a new connection, ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
		//peer->Ping(ec_);
		if (!OnConnectOpen(conn)) { //delete
			conn->Close("connections exceed");
			RemoveConnection(conn);
		}
	}

	void Network::OnClose(connection_hdl hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		Connection *conn = GetConnection(hdl);
		if (conn) {
			LOG_INFO("Closed a connection, ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
			OnDisconnect(conn);
			RemoveConnection(conn);
		} 
	}

	void Network::OnFailed(connection_hdl hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		Connection *conn = GetConnection(hdl);
		if (conn) {
			LOG_ERROR("Got a network failed event, ip(%s), error desc(%s)", conn->GetPeerAddress().ToIpPort().c_str(), conn->GetErrorCode().message().c_str());
			OnDisconnect(conn);
			RemoveConnection(conn);
		}
	}

	void Network::OnMessage(connection_hdl hdl, server::message_ptr msg) {
		//LOG_INFO("Recv message %s %d", 
		//	utils::String::BinToHexString(msg->get_payload()).c_str(), msg->get_opcode());

		protocol::WsMessage message;
		try {
			message.ParseFromString(msg->get_payload());
		}
		catch (std::exception const e) {
			LOG_ERROR("Failed to parse websocket message (%s)", e.what());
			return;
		}

		int64_t conn_id = -1;
		do {
			utils::MutexGuard guard(conns_list_lock_);
			Connection *conn = GetConnection(hdl);
			if (!conn) { return; }

			conn->TouchReceiveTime();
			conn_id = conn->GetId();
		} while (false);

		do {
			MessageConnPoc proc;
			if (message.request()) {
				MessageConnPocMap::iterator iter = request_methods_.find(message.type());
				if (iter == request_methods_.end()) { LOG_TRACE("Type(" FMT_I64 ") not found", message.type()); break; } // methond not found, break;
				proc = iter->second;
			} else{
				MessageConnPocMap::iterator iter = response_methods_.find(message.type());
				if (iter == response_methods_.end()) { LOG_TRACE("Type(" FMT_I64 ") not found", message.type()); break; } // methond not found, break;
				proc = iter->second;
			}

			if (proc(message, conn_id)) break; //Break if returned true;

			LOG_ERROR("Failed to process message, the method type (" FMT_I64 ") (%s) handles exceptions, need to delete it here",
				message.type(), message.request() ? "true" : "false");
			// Delete the connection if returned false.
			do {
				utils::MutexGuard guard(conns_list_lock_);
				Connection *conn = GetConnection(hdl);
				if (!conn) {
					LOG_ERROR("Failed to process network message. Handle not found");
					break;  //Not found
				}
				OnDisconnect(conn);
			} while (false);

			RemoveConnection(conn_id);
		} while (false);
	}

	void Network::Stop() {
		enabled_ = false;
	}

	void Network::Start(const utils::InetAddress &ip) {
		//try {
			if (!ip.IsNone()) {
				if (ssl_parameter_.enable_) {
					// Listen on a specified port.
					tls_server_.listen(ip.tcp_endpoint());
					// Start the TLS server.
					tls_server_.start_accept();
					listen_port_ = tls_server_.get_local_endpoint().port();
				}
				else {
					// Listen on a specified port.
					server_.listen(ip.tcp_endpoint());
					// Start the TLS server.
					server_.start_accept();
					listen_port_ = server_.get_local_endpoint().port();
				}
				LOG_INFO("Network listen at ip(%s)", ip.ToIpPort().c_str());
			}
			enabled_ = true;

			asio::io_service::work work(io_);
			// Start the ASIO io_service run loop.
			int64_t last_check_time = 0;
			while (enabled_) {
				io_.poll();

				utils::Sleep(1);

				int64_t now = utils::Timestamp::HighResolution();
				if (now - last_check_time > utils::MICRO_UNITS_PER_SEC) {

					utils::MutexGuard guard_(conns_list_lock_);
					//Ping the client to see if the connectin times out.
					std::list<Connection *> delete_list;
					for (ConnectionMap::iterator iter = connections_.begin();
						iter != connections_.end();
						iter++) {

						if (iter->second->NeedPing(connect_time_out_ / 4)) {
							iter->second->PingCustom(ec_);
						}

						if (iter->second->IsDataExpired(connect_time_out_)) {
							iter->second->Close("expired");
							delete_list.push_back(iter->second);
							LOG_ERROR("Failed to process data by network module.Peer(%s) data receive timeout", iter->second->GetPeerAddress().ToIpPort().c_str());
						}

						//Check application timer.
						if (!iter->second->OnNetworkTimer(now)) {
							iter->second->Close("app error");
							delete_list.push_back(iter->second);
						} 
					}

					//Remove the current connection to delete array.
					for (std::list<Connection *>::iterator iter = delete_list.begin();
						iter != delete_list.end();
						iter++) {
						LOG_INFO("Connection is closed as expired, ip(%s)", (*iter)->GetPeerAddress().ToIpPort().c_str());
						OnDisconnect(*iter);
						RemoveConnection(*iter);
					}

					//Check if the connections are deleted.
					for (ConnectionMap::iterator iter = connections_delete_.begin();
						iter != connections_delete_.end();) {
						if (iter->first < now) {
							LOG_TRACE("Deleted connect id:%lld", iter->second->GetId());
							delete iter->second;
							iter = connections_delete_.erase(iter);
						}
						else {
							iter++;
						}
					}

					last_check_time = now;
				}
			}
	//	}
		//catch (const std::exception & e) {
		//	LOG_ERROR("%s", e.what());
		//}

		enabled_ = false;
		LOG_INFO("Network listen server(%s) has exited", ip.ToIpPort().c_str());
	}
	
	uint16_t Network::GetListenPort() const {
		return listen_port_;
	}

	bool Network::Connect(const std::string &uri) {
		websocketpp::lib::error_code ec;

		tls_client::connection_ptr tls_con = NULL;
		client::connection_ptr con = NULL;
		connection_hdl handle;
		if (ssl_parameter_.enable_) {
			tls_con = tls_client_.get_connection(uri, ec);
			if (tls_con) {
				tls_con->set_open_handler(bind(&Network::OnClientOpen, this, _1));
				tls_con->set_close_handler(bind(&Network::OnClose, this, _1));
				tls_con->set_message_handler(bind(&Network::OnMessage, this, _1, _2));
				tls_con->set_fail_handler(bind(&Network::OnFailed, this, _1));
				tls_con->set_pong_handler(bind(&Network::OnPong, this, _1, _2));
				handle = tls_con->get_handle();
			}
			else {
				LOG_ERROR("Failed to connect network.Url(%s), error(%s)", uri.c_str(), ec.message().c_str());
				return false;
			}
		}
		else {
			con = client_.get_connection(uri, ec);
			if (con) {
				con->set_open_handler(bind(&Network::OnClientOpen, this, _1));
				con->set_close_handler(bind(&Network::OnClose, this, _1));
				con->set_message_handler(bind(&Network::OnMessage, this, _1, _2));
				con->set_fail_handler(bind(&Network::OnFailed, this, _1));
				con->set_pong_handler(bind(&Network::OnPong, this, _1, _2));
				handle = con->get_handle();
			}
			else {
				LOG_ERROR("Failed to connect network.Url(%s), error(%s)", uri.c_str(), ec.message().c_str());
				return false;
			}
		}

		if (ec) {
			LOG_INFO("Failed to connect uri(%s), error(%s)", uri.c_str(), ec.message().c_str());
			return false;
		}

		utils::MutexGuard guard_(conns_list_lock_);
		int64_t new_id = next_id_++;
		Connection *peer = CreateConnectObject(NULL, ssl_parameter_.enable_? NULL : &client_, 
			NULL, ssl_parameter_.enable_ ? &tls_client_ : NULL,
			handle, uri, new_id);
		connections_.insert(std::make_pair(new_id, peer));
		connection_handles_.insert(std::make_pair(handle, new_id));

	
		if (ssl_parameter_.enable_) {
			tls_client_.connect(tls_con);
		}
		else {
			client_.connect(con);
		}

		LOG_INFO("Connecting uri(%s), network id(" FMT_I64 ")", uri.c_str(), new_id);
		return true;
	}

	Connection *Network::GetConnection(int64_t id) {
		ConnectionMap::iterator iter = connections_.find(id);
		if (iter != connections_.end()){
			return iter->second;
		}

		return NULL;
	}

	Connection *Network::GetConnection(connection_hdl hdl) {
		ConnectHandleMap::iterator iter = connection_handles_.find(hdl);
		if (iter == connection_handles_.end()) {
			return NULL;
		}

		return GetConnection(iter->second);
	}

	void Network::RemoveConnection(int64_t conn_id) {
		utils::MutexGuard guard(conns_list_lock_);
		Connection *conn = GetConnection(conn_id);
		if(conn) RemoveConnection(conn);
	}

	void Network::RemoveConnection(Connection *conn) {
		LOG_INFO("Remove connection id(" FMT_I64 "), peer ip(%s)", conn->GetId(), conn->GetPeerAddress().ToIpPort().c_str());
		conn->Close("no reason");
		connections_.erase(conn->GetId());
		connection_handles_.erase(conn->GetHandle());
		connections_delete_.insert(std::make_pair(utils::Timestamp::HighResolution() + 5 * utils::MICRO_UNITS_PER_SEC,
			conn));
	}

	void Network::OnClientOpen(connection_hdl hdl) {
		utils::MutexGuard guard_(conns_list_lock_);
		Connection * conn = GetConnection(hdl);
		if (conn) {
			LOG_INFO("Peer is connected, ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
			conn->SetConnectTime();

			if (!OnConnectOpen(conn)) { //delete
				conn->Close("no reason");
				RemoveConnection(conn);
			} 
			//conn->Ping(ec_);
		}
	}

	std::string Network::GetCertPassword() {
		return ssl_parameter_.cert_password_;
	}

	bool Network::OnVerifyCallback(
		bool preverified, // True if the certificate passed pre-verification.
		asio::ssl::verify_context& ctx // The peer certificate and other context.
		) {
		return true;
	}

	bool Network::OnValidate(websocketpp::connection_hdl hdl) {
		return true;
	}

	context_ptr Network::OnTlsInit(tls_mode mode, connection_hdl hdl) {
		//LOG_INFO("using TLS mode :%s ", (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate"));
		context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12);

		try {
			// Modern disables TLSv1
			ctx->set_options(asio::ssl::context::default_workarounds |
				asio::ssl::context::no_sslv2 |
				asio::ssl::context::no_sslv3 |
				asio::ssl::context::no_tlsv1 |
				asio::ssl::context::no_tlsv1_1 |
				asio::ssl::context::single_dh_use);

			ctx->set_password_callback(std::bind(&Network::GetCertPassword, this));
			ctx->use_certificate_chain_file(ssl_parameter_.chain_file_);
			ctx->use_private_key_file(ssl_parameter_.private_key_file_, asio::ssl::context::pem);
			ctx->load_verify_file(ssl_parameter_.verify_file_);
			ctx->set_verify_callback(std::bind(&Network::OnVerifyCallback, this, _1, _2));

			// Example method of generating this file:
			// `openssl dhparam -out dh.pem 2048`
			// Mozilla Intermediate suggests 1024 as the minimum size to use
			// Mozilla Modern suggests 2048 as the minimum size to use.
			ctx->use_tmp_dh_file(ssl_parameter_.tmp_dh_file_);
			ctx->set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert);

			std::string ciphers;

			if (mode == MOZILLA_MODERN) {
				ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
			}
			else {
				ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
			}

			if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
				std::cout << "Error setting cipher list" << std::endl;
			}
		}
		catch (std::exception& e) {
			PROCESS_EXIT("Exception: %s " , e.what());
		}
		return ctx;
	}

	void Network::OnPong(connection_hdl hdl, std::string payload) {
		Connection *peer = GetConnection(hdl);
		if (peer){
			peer->TouchReceiveTime();
			LOG_INFO("Recv pong, payload(%s) from ip(%s)", payload.c_str(), peer->GetPeerAddress().ToIpPort().c_str());
		} 
	}

	bool Network::OnRequestPing(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Ping ping;
		if (!ping.ParseFromString(message.data())){
			LOG_ERROR("Failed to parse ping");
			return false;
		}

		utils::MutexGuard guard_(conns_list_lock_);
		Connection *con = GetConnection(conn_id);
		if (!con) {
			LOG_ERROR("Failed to get connection by id(" FMT_I64 ")", conn_id);
			return false;
		} 

		protocol::Pong pong;
		pong.set_nonce(ping.nonce());
		return con->SendResponse(message, pong.SerializeAsString(), ec_);
	}

	bool Network::OnResponsePing(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Pong pong;
		if (!pong.ParseFromString(message.data())) {
			LOG_ERROR("Failed to parse pong");
			return false;
		}

		Connection *conn = GetConnection(conn_id);
		if (conn) {
			conn->TouchReceiveTime();
			LOG_TRACE("Recv pong, nonce(" FMT_I64 ") from ip(%s)", pong.nonce(), conn->GetPeerAddress().ToIpPort().c_str());
		}

		return true;
	}

	Connection *Network::CreateConnectObject(server *server_h, client *client_h,
		tls_server *tls_server_h, tls_client *tls_client_h, 
		connection_hdl con, const std::string &uri, int64_t id) {
		return new Connection(server_h, client_h, tls_server_h, tls_client_h, con, uri, id);
	}

}
