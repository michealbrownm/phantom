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

#include <utils/headers.h>
#include <common/general.h>
#include <main/configure.h>
#include <proto/cpp/monitor.pb.h>
#include <overlay/peer_manager.h>
#include <glue/glue_manager.h>
#include <ledger/ledger_manager.h>
#include <monitor/monitor.h>

#include "websocket_server.h"

namespace phantom {
	WsPeer::WsPeer(server *server_h, client *client_h, tls_server *tls_server_h, tls_client *tls_client_h, connection_hdl con, const std::string &uri, int64_t id) :
		Connection(server_h, client_h, tls_server_h, tls_client_h, con, uri, id) {
	}

	WsPeer::~WsPeer() {}

	bool WsPeer::Set(const protocol::ChainSubscribeTx &sub) {
		if (sub.address_size() > 100) {
			LOG_ERROR("Subscribe tx size large than 100");
			return false;
		}

		tx_filter_address_.clear();
		for (int32_t i = 0; i < sub.address_size(); i++) {
			if (!PublicKey::IsAddressValid(sub.address(i))) {
				LOG_ERROR("Subscribe tx failed, address(%s) not valid", sub.address(i).c_str());
				return false;
			} 
			tx_filter_address_.insert(sub.address(i));
		}

		return true;
	}

	bool WsPeer::Filter(const protocol::TransactionEnvStore &tx_msg) {
		if (tx_filter_address_.empty()) {
			return true;
		}

		const protocol::Transaction &trans = tx_msg.transaction_env().transaction();
		if (tx_filter_address_.find(trans.source_address()) != tx_filter_address_.end()) {
			return true;
		}

		for (int32_t i = 0; i < trans.operations_size(); i++) {
			const protocol::Operation &ope = trans.operations(i);
			if (!ope.source_address().empty() && tx_filter_address_.find(ope.source_address()) != tx_filter_address_.end()) {
				return true;
			}

			switch (ope.type()) {
			case protocol::Operation_Type_CREATE_ACCOUNT:{
				if (tx_filter_address_.find(ope.create_account().dest_address()) != tx_filter_address_.end()) {
					return true;
				}
				break;
			}
			case protocol::Operation_Type_PAY_COIN:{
				if (tx_filter_address_.find(ope.pay_coin().dest_address()) != tx_filter_address_.end()) {
					return true;
				}
				break;
			}
			case protocol::Operation_Type_PAYMENT:{
				if (tx_filter_address_.find(ope.payment().dest_address()) != tx_filter_address_.end()) {
					return true;
				}
				break;
			}
	
			default:
				break;
			}
		}

		return false;
	}

	WebSocketServer::WebSocketServer() : Network(SslParameter()) {
		connect_interval_ = 120 * utils::MICRO_UNITS_PER_SEC;
		last_connect_time_ = 0;

		request_methods_[protocol::CHAIN_HELLO] = std::bind(&WebSocketServer::OnChainHello, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::CHAIN_PEER_MESSAGE] = std::bind(&WebSocketServer::OnChainPeerMessage, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::CHAIN_SUBMITTRANSACTION] = std::bind(&WebSocketServer::OnSubmitTransaction, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::CHAIN_SUBSCRIBE_TX] = std::bind(&WebSocketServer::OnSubscribeTx, this, std::placeholders::_1, std::placeholders::_2);
		thread_ptr_ = NULL;
	}

	WebSocketServer::~WebSocketServer() {
		if (thread_ptr_){
			delete thread_ptr_;
		} 
	}

	bool WebSocketServer::Initialize(WsServerConfigure &ws_server_configure) {
		thread_ptr_ = new utils::Thread(this);
		if (!thread_ptr_->Start("websocket")) {
			return false;
		}

		StatusModule::RegisterModule(this);
		LOG_INFO("Websocket server initialized");
		return true;
	}

	bool WebSocketServer::Exit() {
		Stop();
		thread_ptr_->JoinWithStop();
		return true;
	}

	void WebSocketServer::Run(utils::Thread *thread) {
		Start(phantom::Configure::Instance().wsserver_configure_.listen_address_);
	}

	bool WebSocketServer::OnChainHello(protocol::WsMessage &message, int64_t conn_id) {
		protocol::ChainStatus cmsg;
		cmsg.set_phantom_version(General::PHANTOM_VERSION);
		cmsg.set_monitor_version(General::MONITOR_VERSION);
		cmsg.set_ledger_version(General::LEDGER_VERSION);
		cmsg.set_self_addr(PeerManager::Instance().GetPeerNodeAddress());
		cmsg.set_timestamp(utils::Timestamp::HighResolution());
		std::error_code ignore_ec;

		utils::MutexGuard guard_(conns_list_lock_);
		Connection *conn = GetConnection(conn_id);
		if (conn) {
			conn->SendResponse(message, cmsg.SerializeAsString(), ignore_ec);
			LOG_INFO("Recv chain hello from ip(%s), send response result(%d:%s)", conn->GetPeerAddress().ToIpPort().c_str(),
				ignore_ec.value(), ignore_ec.message().c_str());
		}
		return true;
	}

	bool WebSocketServer::OnChainPeerMessage(protocol::WsMessage &message, int64_t conn_id) {
		// send peer
		utils::MutexGuard guard_(conns_list_lock_);
		Connection *conn = GetConnection(conn_id);
		if (!conn) {
			return false;
		}

		LOG_INFO("Recv chain peer message from ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
		protocol::ChainPeerMessage cpm;
		if (!cpm.ParseFromString(message.data())) {
			LOG_ERROR("ChainPeerMessage FromString fail");
			return true;
		}

		//bubi::PeerManager::Instance().BroadcastPayLoad(cpm);
		return true;
	}

	void WebSocketServer::BroadcastMsg(int64_t type, const std::string &data) {
		utils::MutexGuard guard(conns_list_lock_);

		for (ConnectionMap::iterator iter = connections_.begin();
			iter != connections_.end();
			iter++) {
			std::error_code ec;
			iter->second->SendRequest(type, data, ec);
		}
	}

	void WebSocketServer::BroadcastChainTxMsg(const protocol::TransactionEnvStore& tx_msg) {
		utils::MutexGuard guard(conns_list_lock_);

		for (auto iter = connections_.begin(); iter != connections_.end(); iter++) {
			WsPeer *peer = (WsPeer *)iter->second;
			if (peer->Filter(tx_msg)) {
				std::error_code ec;
				std::string str = tx_msg.SerializeAsString();
				peer->SendRequest(protocol::CHAIN_TX_ENV_STORE, str, ec);
			}
		}
	}

	bool WebSocketServer::OnSubmitTransaction(protocol::WsMessage &message, int64_t conn_id) {
		utils::MutexGuard guard_(conns_list_lock_);
		Connection *conn = GetConnection(conn_id);
		if (!conn) {
			return false;
		}

		Result result;
		protocol::TransactionEnv tran_env;
		do {
			if (!tran_env.ParseFromString(message.data())) {
				LOG_ERROR("Parse submit transaction string fail, ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
				result.set_code(protocol::ERRCODE_INVALID_PARAMETER);
				result.set_desc("Parse the transaction failed");
				break;
			}
			Json::Value real_json;
			real_json = Proto2Json(tran_env);
			printf("%s",real_json.toStyledString().c_str());
			std::string content = tran_env.transaction().SerializeAsString();

			TransactionFrm::pointer ptr = std::make_shared<TransactionFrm>(tran_env);
			GlueManager::Instance().OnTransaction(ptr, result);
			PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_TRANSACTION, tran_env.SerializeAsString());
		
		} while (false);

		//notice WebSocketServer Tx status
		std::string hash = HashWrapper::Crypto(tran_env.transaction().SerializeAsString());
		protocol::ChainTxStatus cts;
		cts.set_tx_hash(utils::String::BinToHexString(hash));
		cts.set_error_code((protocol::ERRORCODE)result.code());
		cts.set_source_address(tran_env.transaction().source_address());
		cts.set_status(result.code() == protocol::ERRCODE_SUCCESS ? protocol::ChainTxStatus_TxStatus_CONFIRMED : protocol::ChainTxStatus_TxStatus_FAILURE);
		cts.set_error_desc(result.desc());
		cts.set_timestamp(utils::Timestamp::Now().timestamp());
		std::string str = cts.SerializeAsString();
			
		BroadcastMsg(protocol::CHAIN_TX_STATUS, str);
		
		return true;
	}

	bool WebSocketServer::OnSubscribeTx(protocol::WsMessage &message, int64_t conn_id){
		utils::MutexGuard guard_(conns_list_lock_);
		WsPeer *conn = (WsPeer *)GetConnection(conn_id);
		if (!conn) {
			return false;
		}

		protocol::ChainResponse default_response;
		do {
			LOG_INFO("Recv chain peer message from ip(%s)", conn->GetPeerAddress().ToIpPort().c_str());
			protocol::ChainSubscribeTx subs;
			if (!subs.ParseFromString(message.data())) {
				default_response.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				default_response.set_error_desc("ChainPeerMessage FromString fail");
				LOG_ERROR("%s", default_response.error_desc().c_str());
				break;
			}

			bool ret = conn->Set(subs);
			if (!ret) {
				default_response.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				default_response.set_error_desc("ChainPeerMessage FromString fail");
				LOG_ERROR("%s", default_response.error_desc().c_str());
				break;
			} 
		} while (false);

		std::error_code ec;
		conn->SendResponse(message, default_response.SerializeAsString(), ec);
		return default_response.error_code() == protocol::ERRCODE_SUCCESS;
	}

	void WebSocketServer::GetModuleStatus(Json::Value &data) {
		data["name"] = "websocket_server";
		data["listen_port"] = GetListenPort();
		Json::Value &peers = data["clients"];
		int32_t active_size = 0;
		utils::MutexGuard guard(conns_list_lock_);
		for (auto &item : connections_) {
			item.second->ToJson(peers[peers.size()]);
		}
	}

	Connection *WebSocketServer::CreateConnectObject(server *server_h, client *client_,
		tls_server *tls_server_h, tls_client *tls_client_h,
		connection_hdl con, const std::string &uri, int64_t id) {
		return new WsPeer(server_h, client_, tls_server_h, tls_client_h, con, uri, id);
	}
}