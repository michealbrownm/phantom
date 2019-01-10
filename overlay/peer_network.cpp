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

#include <utils/logger.h>
#include <utils/timestamp.h>
#include <common/general.h>
#include <common/storage.h>
#include <common/private_key.h>
#include <main/configure.h>
#include <glue/glue_manager.h>
#include <proto/cpp/overlay.pb.h>
#include <ledger/ledger_manager.h>

#include "peer_network.h"

namespace phantom {

	PeerNetwork::PeerNetwork(const SslParameter &ssl_parameter_) :Network(ssl_parameter_),
		context_(asio::ssl::context::tlsv12),
		cert_enabled_(false),
		cert_is_valid_(false),
		broadcast_(this) {
		check_interval_ = 5 * utils::MICRO_UNITS_PER_SEC;
		dns_seed_inited_ = false; 
		total_peers_count_ = 0;
		timer_name_ = utils::String::Format("%s Network", "Consensus" );

		request_methods_[protocol::OVERLAY_MSGTYPE_HELLO] = std::bind(&PeerNetwork::OnMethodHello, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::OVERLAY_MSGTYPE_PEERS] = std::bind(&PeerNetwork::OnMethodPeers, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::OVERLAY_MSGTYPE_TRANSACTION] = std::bind(&PeerNetwork::OnMethodTransaction, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::OVERLAY_MSGTYPE_LEDGERS] = std::bind(&PeerNetwork::OnMethodGetLedgers, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::OVERLAY_MSGTYPE_PBFT] = std::bind(&PeerNetwork::OnMethodPbft, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[protocol::OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY] = std::bind(&PeerNetwork::OnMethodLedgerUpNotify, this, std::placeholders::_1, std::placeholders::_2);


		response_methods_[protocol::OVERLAY_MSGTYPE_LEDGERS] = std::bind(&PeerNetwork::OnMethodLedgers, this, std::placeholders::_1, std::placeholders::_2);
		response_methods_[protocol::OVERLAY_MSGTYPE_HELLO] = std::bind(&PeerNetwork::OnMethodHelloResponse, this, std::placeholders::_1, std::placeholders::_2);
		last_update_peercache_time_ = 0;
	}

	PeerNetwork::~PeerNetwork() {
	}

	void PeerNetwork::Clean() {
	}

	bool PeerNetwork::Initialize(const std::string &node_address) {
		do {
			peer_node_address_ = node_address;

			//get network id from ledger db
			LedgerFrm ledger;
			if (!ledger.LoadFromDb(1)) {
				LOG_ERROR("Load from db failed");
				break;
			}
			network_id_ = Configure::Instance().p2p_configure_.network_id_;
			node_rand_ = utils::String::Format("node-rand-" FMT_I64 "-%d", utils::Timestamp::HighResolution(), rand() * rand());

			TimerNotify::RegisterModule(this);
			return ResetPeerInActive();
		} while (false);

		Clean();
		return false;
	}

	bool PeerNetwork::Exit() {
		//join and wait
		LOG_INFO("close async OK");

		return true;
	}

	bool PeerNetwork::OnMethodPeers(protocol::WsMessage &message, int64_t conn_id) {
		utils::MutexGuard guard(conns_list_lock_);
		Peer *peer = (Peer *)GetConnection(conn_id);
		if (!peer->IsActive()) {
			return true;
		}

		const P2pNetwork &p2p_configure = Configure::Instance().p2p_configure_.consensus_network_configure_;
		protocol::Peers peers;
		peers.ParseFromString(message.data());
		for (int i = 0; i < peers.peers_size(); i++) {
			const protocol::Peer &peerp = peers.peers(i);
			std::string ip = peerp.ip();
			uint16_t port = (uint16_t)peerp.port();

			//check if it's local address
			utils::InetAddressVec addresses;
			bool is_local_addr = false;
			if (utils::net::GetNetworkAddress(addresses)) {
				for (utils::InetAddressVec::iterator iter = addresses.begin();
					iter != addresses.end();
					iter++) {
					if (iter->ToIp() == ip && p2p_configure.listen_port_ == port) {
						is_local_addr = true;
						break;
					}
				}
			}

			if (is_local_addr) {
				continue;
			}

			if (total_peers_count_ < General::PEER_DB_COUNT) CreatePeerIfNotExist(utils::InetAddress(ip, port));
		}

		return true;
	}

	bool PeerNetwork::OnMethodHello(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Hello hello;
		hello.ParseFromString(message.data());

		utils::MutexGuard guard(conns_list_lock_);
		Peer *peer = (Peer *)GetConnection(conn_id);
		if (!peer){
			return true;
		} 

		protocol::HelloResponse res;

		do {
			peer->SetPeerInfo(hello);

			if (NodeExist(hello.node_address(), peer->GetId())) {
				res.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				res.set_error_desc(utils::String::Format("Disconnect duplicated connection with ip(%s), id(" FMT_I64 ")", peer->GetPeerAddress().ToIp().c_str(), peer->GetId()));
				LOG_ERROR("%s",res.error_desc().c_str());
				break;
			}
			
			if (network_id_ != hello.network_id()) {
				res.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				res.set_error_desc(utils::String::Format("Peer connect break as peer network id(" FMT_I64 ") not equal to local id(" FMT_I64 ")",
					hello.network_id(), network_id_));
				LOG_ERROR("%s", res.error_desc().c_str());
				break;
			}

			if (peer_node_address_ == hello.node_address()) {
				res.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				if (node_rand_ != hello.node_rand()) {
					res.set_error_desc(utils::String::Format("Peer connect break as configure node address duplicated"));
					LOG_ERROR("%s", res.error_desc().c_str());
				}
				else {
					res.set_error_desc(utils::String::Format("Peer connect self break"));
					LOG_ERROR("%s", res.error_desc().c_str());
				}
				break;
			}

			if (hello.overlay_version() < phantom::General::OVERLAY_MIN_VERSION) {
				res.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				res.set_error_desc(utils::String::Format("Peer's overlay version(%d) is too old,", hello.overlay_version()));
				LOG_ERROR("%s", res.error_desc().c_str());
				break;
			}
			//if (hello->ledger_version() < bubi::General::LEDGER_MIN_VERSION){
			//	LOG_ERROR("Peer's leger version(%d) is too old,", hello->ledger_version());
			//	break;
			//}
			if (hello.listening_port() <= 0 ||
				hello.listening_port() > utils::MAX_UINT16) {
				res.set_error_code(protocol::ERRCODE_INVALID_PARAMETER);
				res.set_error_desc(utils::String::Format("Peer's listen port(%d) is not valid", hello.listening_port()));
				LOG_ERROR("%s", res.error_desc().c_str());
				break;
			}

			LOG_INFO("Recv hello, peer(%s) is active", peer->GetRemoteAddress().ToIpPort().c_str());
			peer->SetActiveTime(utils::Timestamp::HighResolution());

			if (peer->InBound()) {
				const P2pNetwork &p2p_configure = phantom::Configure::Instance().p2p_configure_.consensus_network_configure_;

				std::error_code ec;
				peer->SendHello(p2p_configure.listen_port_, peer_node_address_, network_id_, node_rand_, last_ec_);

				//create
				if (total_peers_count_ < General::PEER_DB_COUNT) CreatePeerIfNotExist(peer->GetRemoteAddress());

				//async send peers
				int64_t peer_id = peer->GetId();
				Global::GetInstance()->GetIoService().post([peer_id, this] {
					//send local peer list
					GetActivePeers(50);

					utils::MutexGuard guard(conns_list_lock_);
					Peer *peer = (Peer *)GetConnection(peer_id);
					if (peer && peer->IsActive()) {
						std::error_code ignore_ec;
						peer->SendPeers(db_peer_cache_, ignore_ec);
					}
				});
			}
			else {
			}

			//update status
			protocol::Peer values;
			values.set_num_failures(0);
			values.set_active_time(peer->GetActiveTime());
			values.set_next_attempt_time(-1);
			values.set_connection_id(conn_id);
			UpdateItem(peer->GetRemoteAddress(), values);

		} while (false);

		std::error_code ignore_ec;
		peer->SendResponse(message, res.SerializeAsString(), ignore_ec);
		return res.error_code() == 0;
	}

	bool PeerNetwork::OnMethodTransaction(protocol::WsMessage &message, int64_t conn_id) {
		if (message.data().size() > General::TRANSACTION_LIMIT_SIZE + 2 * utils::BYTES_PER_MEGA) {
			LOG_ERROR("Transaction p2p data size(" FMT_SIZE ") too large", message.data().size());
			return false;
		}

		if (ReceiveBroadcastMsg(protocol::OVERLAY_MSGTYPE_TRANSACTION, message.data(), conn_id)) {
			protocol::TransactionEnv tran;
			tran.ParseFromString(message.data());
			TransactionFrm::pointer tran_ptr = std::make_shared<TransactionFrm>(tran);
			//switch to main thread
			Global::Instance().GetIoService().post([tran_ptr, message, this]() {
				Result ig_err;
				if (GlueManager::Instance().OnTransaction(tran_ptr, ig_err)) {
					BroadcastMsg(message.type(), message.data());
				}
			});
		}
		return true;
	}

	bool PeerNetwork::OnMethodGetLedgers(protocol::WsMessage &message, int64_t conn_id) {
		protocol::GetLedgers getledgers;
		getledgers.ParseFromString(message.data());
		LedgerManager::Instance().OnRequestLedgers(getledgers, conn_id);
		return true;
	}

	bool PeerNetwork::OnMethodLedgers(protocol::WsMessage &message, int64_t conn_id) {
		protocol::Ledgers ledgers;
		ledgers.ParseFromString(message.data());
		LedgerManager::Instance().OnReceiveLedgers(ledgers, conn_id);
		return true;
	}

	bool PeerNetwork::OnMethodHelloResponse(protocol::WsMessage &message, int64_t conn_id) {
		utils::MutexGuard guard(conns_list_lock_);
		Peer *peer = (Peer *)GetConnection(conn_id);
		if (!peer) {
			return true;
		}

		protocol::HelloResponse env;
		env.ParseFromString(message.data());
		if (env.error_code() != 0) {
			LOG_ERROR("Peer reponse error code(%d), desc(%s)", env.error_code(), env.error_desc().c_str());
			return false;
		}

		return true;
	}

	bool PeerNetwork::OnMethodPbft(protocol::WsMessage &message, int64_t conn_id) {
		if (message.data().size() > General::TXSET_LIMIT_SIZE + 2 * utils::BYTES_PER_MEGA) {
			LOG_ERROR("Consensus p2p data size(" FMT_SIZE ") too large", message.data().size());
			return false;
		}

		protocol::PbftEnv env;
		env.ParseFromString(message.data());
		if (!env.has_pbft()) {
			LOG_ERROR("Pbft env is not initialize");
			return false;
		}

		//should in validators
		ConsensusMsg msg(env);
		if (ConsensusManager::Instance().GetConsensus()->GetValidatorIndex(msg.GetNodeAddress()) < 0) {
			LOG_TRACE("Cann't find the validator(%s) in list", msg.GetNodeAddress());
			return true;
		}

		std::string hash = utils::String::Bin4ToHexString(msg.GetHash());

		LOG_TRACE("On pbft hash(%s), receive consensus from node address(%s) sequence(" FMT_I64 ") pbft type(%s) size(" FMT_SIZE ")",
			hash.c_str(), msg.GetNodeAddress(), msg.GetSeq(),
			PbftDesc::GetMessageTypeDesc(msg.GetPbft().pbft().type()), msg.GetSize());


		//switch to main thread
		Global::Instance().GetIoService().post([conn_id, msg, message, hash, this]() {
			if (ReceiveBroadcastMsg(protocol::OVERLAY_MSGTYPE_PBFT, message.data(), conn_id)) {
				LOG_TRACE("Pbft hash(%s) would be processed", hash.c_str());
				BroadcastMsg(protocol::OVERLAY_MSGTYPE_PBFT, message.data());
				GlueManager::Instance().OnConsensus(msg);
			}
		});
		return true;
	}

	bool PeerNetwork::OnMethodLedgerUpNotify(protocol::WsMessage &message, int64_t conn_id) {
		protocol::LedgerUpgradeNotify notify;
		if (!notify.ParseFromString(message.data())) {
			LOG_ERROR("Parse ledger upgrade notify failed");
			return false;
		}

		//pre filter
		const protocol::Signature &sig = notify.signature();
		PublicKey pub_key(sig.public_key());
        if (ConsensusManager::Instance().GetConsensus()->GetValidatorIndex(pub_key.GetEncAddress()) < 0) {
            LOG_TRACE("Cann't find the validator(%s) in list", pub_key.GetEncAddress().c_str());
			return true;
		}

		LOG_INFO("Receive ledger up notify(%s)", Proto2Json(notify).toFastString().c_str());
		if (ReceiveBroadcastMsg(protocol::OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY, message.data(), conn_id)) {
			BroadcastMsg(protocol::OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY, message.data());
			GlueManager::Instance().OnRecvLedgerUpMsg(notify);
		}
		return true;
	}

	bool PeerNetwork::OnConnectOpen(Connection *conn) { 
		const P2pNetwork &p2p_configure = Configure::Instance().p2p_configure_.consensus_network_configure_;
		size_t total_connection = p2p_configure.max_connection_;
		if (connections_.size() < total_connection) {
			if (!conn->InBound()) {
				Peer *peer = (Peer *)conn;
				peer->SendHello(p2p_configure.listen_port_, peer_node_address_, network_id_, node_rand_, last_ec_);
			}
			return true;
		} else{
			LOG_ERROR("Connection open failed, exceed the threshold(" FMT_SIZE ")", total_connection);
			return false;
		}
	}

	Connection *PeerNetwork::CreateConnectObject(server *server_h, client *client_,
		tls_server *tls_server_h, tls_client *tls_client_h,
		connection_hdl con, const std::string &uri, int64_t id) {
		return new Peer(server_h, client_, tls_server_h, tls_client_h,con, uri, id);
	}

	void PeerNetwork::OnDisconnect(Connection *conn) {
		Peer *peer = (Peer *)conn;
		UpdateItemDisconnect(peer->GetRemoteAddress(), conn->GetId());
	}

	void PeerNetwork::CleanNotActivePeers() {
		std::string peers;
		KeyValueDb *db = Storage::Instance().keyvalue_db();
		int32_t count = db->Get(General::PEERS_TABLE, peers);
		if (count < 0) {
			return ;
		}

		protocol::Peers all;
		if (!all.ParseFromString(peers)) {
			LOG_ERROR("Parse peers string failed");
			return ;
		}
		total_peers_count_ = all.peers_size();

		protocol::Peers new_all;
		for (int32_t i = 0; i < all.peers_size(); i++) {
			protocol::Peer *record_temp = all.mutable_peers(i);
			if (record_temp->num_failures() < 50 ) {
				*new_all.add_peers() = *record_temp;
			}
		}

		if (all.peers_size() > new_all.peers_size()) {
			if (!db->Put(General::PEERS_TABLE, new_all.SerializeAsString())) {
				LOG_ERROR("Write new peer table failed, error desc(%s)", db->error_desc().c_str());
			}
			else {
				LOG_INFO("Clean %d not active peers, left %d peers", all.peers_size() - new_all.peers_size(), new_all.peers_size());
			}
		}
		total_peers_count_ = new_all.peers_size();
	}

	bool PeerNetwork::ConnectToPeers(size_t max) {
		const P2pNetwork &p2p_configure = Configure::Instance().p2p_configure_.consensus_network_configure_;

		protocol::Peers records;
		do {
			int32_t row_count = QueryTopItem(false, max, utils::Timestamp::Now().timestamp(), records);
			if (row_count < 0) {
				LOG_ERROR("Query records from db failed");
				break;
			}

			utils::InetAddressVec addresses;
			utils::net::GetNetworkAddress(addresses);

			utils::MutexGuard guard(conns_list_lock_);

			for (int32_t i = 0; i < records.peers_size(); i++) {
				const protocol::Peer &item = records.peers(i);

				utils::InetAddress address(item.ip(), (uint16_t)item.port());
				int64_t num_failures = item.num_failures();

				LOG_TRACE("checking address ip(%s), thread id(" FMT_SIZE ")", address.ToIpPort().c_str(), utils::Thread::current_thread_id());

				bool exist = false;
				for (ConnectionMap::iterator iter = connections_.begin(); iter != connections_.end(); iter++) {
					Peer *peer = (Peer *)iter->second;
					if (peer->GetRemoteAddress() == address) {
						exist = true;
						break;
					}
				}

				if (exist) {
					LOG_TRACE("skip to connect exist ip(%s), thread id(" FMT_SIZE ")", address.ToIpPort().c_str(), utils::Thread::current_thread_id());
					continue;
				}
				bool is_local_addr = false;
				for (utils::InetAddressVec::iterator iter = addresses.begin();
					iter != addresses.end();
					iter++) {
					if (iter->ToIp() == address.ToIp() && p2p_configure.listen_port_ == address.GetPort()) {
						is_local_addr = true;
						break;
					}
				}

				if (is_local_addr) {
					LOG_TRACE("skip to connect self ip(%s), thread id(" FMT_SIZE ")", address.ToIpPort().c_str(), utils::Thread::current_thread_id());
					continue;
				}


				LOG_TRACE("connect to %s, " FMT_SIZE, address.ToIpPort().c_str(), utils::Thread::current_thread_id());
				
				std::string uri = utils::String::Format("%s://%s", ssl_parameter_.enable_ ? "wss" : "ws",address.ToIpPort().c_str());
				
				Connect(uri);

				protocol::Peer update_values;
				num_failures++;
				update_values.set_next_attempt_time(utils::Timestamp::Now().timestamp() + num_failures * 10 * utils::MICRO_UNITS_PER_SEC);
				update_values.set_num_failures(num_failures);
				update_values.set_active_time(-1);
				if (!UpdateItem(address, update_values)) {
					LOG_ERROR("Update peers failed");
				}

				if (connections_.size() >= p2p_configure.target_peer_connection_) {
					break;
				}
			}

			return true;
		} while (false);

		return false;
	}

	bool PeerNetwork::ResolveSeeds(const utils::StringList &address_list, int32_t rank) {
		utils::NameResolver resolver(SlowTimer::Instance().io_service_);
		for (utils::StringList::const_iterator iter = address_list.begin();
			iter != address_list.end();
			iter++) {
			const std::string &longip = *iter;
			utils::StringVector ip_array = utils::String::Strtok(longip, ':');
			std::string ip = longip;
			uint16_t port = General::CONSENSUS_PORT ;
			if (ip_array.size() > 1) {
				port = utils::String::Stoui(ip_array[1]);
				ip = ip_array[0];
			}

			utils::InetAddressList resolved_ips;
			do {
				utils::InetAddress address(ip);
				if (!address.IsNone()) {
					resolved_ips.push_back(address);
					break;
				}
				//go to resolve
				resolver.Query(ip, resolved_ips);
			} while (false);

			for (utils::InetAddressList::iterator iter = resolved_ips.begin();
				iter != resolved_ips.end();
				iter++) {

				utils::InetAddress &address = *iter;
				address.SetPort(port);

				CreatePeerIfNotExist(address);
			}

		}
		return true;
	}

	bool PeerNetwork::CreatePeerIfNotExist(const utils::InetAddress &address) {
		if (address.IsAny() || address.GetPort() == 0) {
			LOG_ERROR("Peer address(%s) not valid", address.ToIpPort().c_str());
			return false;
		} 

		protocol::Peers peers;
		int32_t peer_count = QueryItem(address, peers);
		if (peer_count < 0) {
			LOG_ERROR("Query peer if exist failed, address(%s)", address.ToIpPort().c_str());
			return false;
		}

		if (peer_count > 0) {
			LOG_TRACE("Query peer(%s) exist", address.ToIpPort().c_str());
			return true;
		}

		protocol::Peer values;
		values.set_ip(address.ToIp());
		values.set_port(address.GetPort());

		if (!UpdateItem(address, values)) {
			LOG_ERROR("Insert peer failed");
			return false;
		}

		return true;
	}

	bool PeerNetwork::ResetPeerInActive() {
		protocol::Peer values;
		values.set_num_failures(-1);
		values.set_next_attempt_time(-1);
		values.set_active_time(0);
		values.set_connection_id(0);
		return UpdateItem(utils::InetAddress::Any(), values);
	}

	bool PeerNetwork::GetActivePeers(int32_t max) {
		int64_t cur_time = utils::Timestamp::HighResolution();
		if (db_peer_cache_.peers_size() == 0 || last_update_peercache_time_ == 0 ||
			(cur_time - last_update_peercache_time_) > 10 * utils::MICRO_UNITS_PER_SEC) {
			last_update_peercache_time_ = cur_time;
			db_peer_cache_.clear_peers();
			protocol::Peers peers;
			
			int32_t row_count = QueryTopItem(true, max, -1, peers);
			if (row_count < 0) {
				LOG_ERROR("Query records failed");
				return false;
			}

			db_peer_cache_ = peers;
		}

		return true;
	}

	int32_t PeerNetwork::QueryItem(const utils::InetAddress &address, protocol::Peers &records) {
		std::string peers;
		KeyValueDb *db = Storage::Instance().keyvalue_db();
		int32_t count = db->Get(General::PEERS_TABLE, peers);
		if (count <= 0) {
			return count;
		}

		protocol::Peers all;
		if (!all.ParseFromString(peers)) {
			LOG_ERROR("Parse peers string failed");
			return -1;
		} 
		total_peers_count_ = all.peers_size();

		int32_t peer_count = 0;
		std::string ip = address.ToIp();
		int64_t port = address.GetPort();
		for (int32_t i = 0; i < all.peers_size(); i++) {
			const protocol::Peer record = all.peers(i);
			if (record.ip() == ip && record.port() == port ) {
				peer_count++;
				*records.add_peers() = record;
			} 
		}

		return peer_count;
	}

	bool PeerNetwork::UpdateItem(const utils::InetAddress &address, protocol::Peer &record) {
		std::string peers;
		KeyValueDb *db = Storage::Instance().keyvalue_db();
		int32_t count = db->Get(General::PEERS_TABLE, peers);
		if (count < 0) {
			return false;
		}

		protocol::Peers all;
		if (!all.ParseFromString(peers)) {
			LOG_ERROR("Parse peers string failed");
			return false;
		}
		total_peers_count_ = all.peers_size();

		int32_t peer_count = 0;
		std::string ip = address.ToIp();
		int64_t port = address.GetPort();
		for (int32_t i = 0; i < all.peers_size(); i++) {
			protocol::Peer *record_temp = all.mutable_peers(i);
			if (record_temp->ip() == ip && record_temp->port() == port
				|| address.IsAny()
				) {
				peer_count++;

				if (record.num_failures() >= 0) record_temp->set_num_failures(record.num_failures());
				if (record.next_attempt_time() >= 0) record_temp->set_next_attempt_time(record.next_attempt_time());
				if (record.active_time() >= 0) record_temp->set_active_time(record.active_time());
				if (record.connection_id() >= 0) record_temp->set_connection_id(record.connection_id());
			}
		}

		if (peer_count == 0 && !address.IsAny()) {
			*all.add_peers() = record;
		} 

		bool ret = db->Put(General::PEERS_TABLE, all.SerializeAsString());
		if (!ret) {
			LOG_ERROR("Write peer table failed, error desc(%s)", db->error_desc().c_str());
		}
		total_peers_count_ = all.peers_size();

		return ret;
	}

	bool PeerNetwork::UpdateItemDisconnect(const utils::InetAddress &address, int64_t conn_id) {
		std::string peers;
		KeyValueDb *db = Storage::Instance().keyvalue_db();
		int32_t count = db->Get(General::PEERS_TABLE, peers);
		if (count < 0) {
			return false;
		}

		protocol::Peers all;
		if (!all.ParseFromString(peers)) {
			LOG_ERROR("Parse peers string failed");
			return false;
		}
		total_peers_count_ = all.peers_size();

		int32_t peer_count = 0;
		std::string ip = address.ToIp();
		int64_t port = address.GetPort();
		for (int32_t i = 0; i < all.peers_size(); i++) {
			protocol::Peer *record_temp = all.mutable_peers(i);
			if (record_temp->ip() == ip && 
				record_temp->port() == port && 
				record_temp->connection_id() == conn_id
				) {
				peer_count++;
				record_temp->set_active_time(0);
				record_temp->set_connection_id(0);
			}
		}
		
		if (peer_count > 0 && !db->Put(General::PEERS_TABLE, all.SerializeAsString())) {
			LOG_ERROR("Write peer table failed, error desc(%s)", db->error_desc().c_str());
			return false;
		}
		total_peers_count_ = all.peers_size();
		return true;
	}

	int32_t PeerNetwork::QueryTopItem(bool active, int64_t limit, int64_t next_attempt_time, protocol::Peers &records) {
		std::string peers;
		KeyValueDb *db = Storage::Instance().keyvalue_db();
		int32_t count = db->Get(General::PEERS_TABLE, peers);
		if (count <= 0) {
			return count;
		}

		protocol::Peers all;
		if (!all.ParseFromString(peers)) {
			LOG_ERROR("Parse peers string failed");
			return -1;
		}
		total_peers_count_ = all.peers_size();

		std::multimap<int64_t, protocol::Peer> sorted_records;

		int32_t peer_count = 0;
		for (int32_t i = 0; i < all.peers_size(); i++) {
			const protocol::Peer &record = all.peers(i);
			sorted_records.insert(std::make_pair(record.num_failures(), record));
		}

		for (std::map<int64_t, protocol::Peer>::iterator iter = sorted_records.begin();
			iter != sorted_records.end();
			iter++) {
			const protocol::Peer &record = iter->second;
			if ((active == (record.active_time() > 0)) &&
				(next_attempt_time < 0 || record.next_attempt_time() < next_attempt_time) &&
				peer_count < limit ) {
				peer_count++;
				*records.add_peers() = record;
			}
		}

		return peer_count;
	}

	void PeerNetwork::GetPeers(Json::Value &peers) {
		utils::MutexGuard guard(conns_list_lock_);
		for (auto item : connections_) {
			item.second->ToJson(peers[peers.size()]);
		}
	}

	void PeerNetwork::OnTimer(int64_t current_time) {
		const P2pNetwork &p2p_configure = Configure::Instance().p2p_configure_.consensus_network_configure_;
		if (!dns_seed_inited_) {
			ResolveSeeds(p2p_configure.known_peer_list_, 2);
			dns_seed_inited_ = true;
			return;
		}

		size_t con_size = 0;
		do {
			utils::MutexGuard guard(conns_list_lock_);
			con_size = connections_.size();
		} while (false);

		//start to connect peers
		if (con_size < p2p_configure.target_peer_connection_) {
			ConnectToPeers(p2p_configure.target_peer_connection_ - con_size);
		}

		CleanNotActivePeers();

		broadcast_.OnTimer();
	}

	void PeerNetwork::AddReceivedPeers(const utils::StringMap &item) {
		utils::MutexGuard guard(peer_lock_);
		received_peer_list_.push_back(item);
	}

	void PeerNetwork::BroadcastMsg(int64_t type, const std::string &data) {
		broadcast_.Send(type, data);
	}

	bool PeerNetwork::ReceiveBroadcastMsg(int64_t type, const std::string &data, int64_t peer_id) {
		return broadcast_.Add(type, data, peer_id);
	}

	bool PeerNetwork::SendMsgToPeer(int64_t peer_id, WsMessagePointer message) {
		utils::MutexGuard guard(conns_list_lock_);
		Peer *peer = (Peer *)GetConnection(peer_id);
		if (peer && peer->IsActive()) {
			return peer->SendByteMessage(message->SerializeAsString(), last_ec_);
		}

		return false;
	}

	bool PeerNetwork::SendRequest(int64_t peer_id, int64_t type, const std::string &data) {
		utils::MutexGuard guard(conns_list_lock_);
		Peer *peer = (Peer *)GetConnection(peer_id);
		if (peer && peer->IsActive()) {
			return peer->SendRequest(type, data, last_ec_);
		}

		return false;
	}

	std::set<int64_t> PeerNetwork::GetActivePeerIds() {
		std::set<int64_t> ids;
		utils::MutexGuard guard(conns_list_lock_);
		for (auto item : connections_) {
			Peer *peer = (Peer *)item.second;
			if (peer->IsActive()) {
				ids.insert(peer->GetId());
			}
		}

		return ids;
	}

	bool PeerNetwork::NodeExist(std::string node_address, int64_t peer_id) {
		bool exist = false;
		for (ConnectionMap::iterator iter = connections_.begin(); iter != connections_.end(); iter++) {
			Peer *peer = (Peer *)iter->second;
			if (peer->GetPeerNodeAddress() == node_address && peer->GetId() != peer_id) {
				exist = true;
				break;
			}
		}
		return exist;
	}

	void PeerNetwork::GetModuleStatus(Json::Value &data) {
		do {
			utils::MutexGuard guard(conns_list_lock_);
			data["peer_list_size"] = (Json::UInt64)connections_.size();
			data["peer_listdel_size"] = (Json::UInt64)connections_delete_.size();
		} while (false);
		data["peer_cache_size"] = (Json::UInt64)db_peer_cache_.peers_size();
		data["recv_peerlist_size"] = (Json::UInt64)received_peer_list_.size();
		data["broad_record_size"] = (Json::UInt64)broadcast_.GetRecordSize();
		int active_size = 0;
		Json::Value peers;
		do {
			utils::MutexGuard guard(conns_list_lock_);
			for (auto &item : connections_) {
				Peer *peer = (Peer *)item.second;
				if (peers.size() < 20) { //only record the 20
					peer->ToJson(peers[peers.size()]);
				}
				if (peer->IsActive()) {
					active_size++;
				}
			}
		} while (false);
		data["peers"] = peers;
		data["peer_active_size"] = active_size;
		data["node_rand"] = node_rand_;
	}

	bool PeerNetwork::OnVerifyCallback(bool preverified, asio::ssl::verify_context& ctx) {
		return true;
	}

	bool PeerNetwork::OnValidate(websocketpp::connection_hdl hdl) {
		return cert_is_valid_;
	}
}
