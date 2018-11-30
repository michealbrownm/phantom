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

#include "monitor_manager.h"

namespace phantom {
	MonitorManager::MonitorManager() : Network(SslParameter()) {
		// By default the interval between two connections is 120 seconds
		connect_interval_ = 120 * utils::MICRO_UNITS_PER_SEC;
		// By default the interval between checking two alerts is 5 seconds
		check_alert_interval_ = 5 * utils::MICRO_UNITS_PER_SEC; 
		last_alert_time_ = utils::Timestamp::HighResolution();
		last_connect_time_ = 0;

		// Add the monitor to message
		request_methods_[monitor::MONITOR_MSGTYPE_HELLO] = std::bind(&MonitorManager::OnMonitorHello, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[monitor::MONITOR_MSGTYPE_REGISTER] = std::bind(&MonitorManager::OnMonitorRegister, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[monitor::MONITOR_MSGTYPE_PHANTOM] = std::bind(&MonitorManager::OnPhantomStatus, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[monitor::MONITOR_MSGTYPE_LEDGER] = std::bind(&MonitorManager::OnLedgerStatus, this, std::placeholders::_1, std::placeholders::_2);
		request_methods_[monitor::MONITOR_MSGTYPE_SYSTEM] = std::bind(&MonitorManager::OnSystemStatus, this, std::placeholders::_1, std::placeholders::_2);

		thread_ptr_ = NULL;
	}

	MonitorManager::~MonitorManager() {
		if (thread_ptr_){
			delete thread_ptr_;
			thread_ptr_ = NULL;
		} 
	}

	bool MonitorManager::Initialize() {
		// Check whether the monitor is enabled
		MonitorConfigure& monitor_configure = Configure::Instance().monitor_configure_;
		if (!monitor_configure.enabled_){
			LOG_TRACE("Failed to initialize monitor, config file does not allow startup");
			return true;
		}

		// Get the monitor id
		monitor_id_ = monitor_configure.id_;

        // Start the thread of the monitor
		thread_ptr_ = new utils::Thread(this);
		if (!thread_ptr_->Start("monitor")) {
			return false;
		}

		// Add the register of StatusModule and TimeNotify
		StatusModule::RegisterModule(this);
		TimerNotify::RegisterModule(this);
		LOG_INFO("Initialized monitor manager successfully");
		return true;
	}

	bool MonitorManager::Exit() {
		Stop();
		if (thread_ptr_) {
			thread_ptr_->JoinWithStop();
		}
		return true;
	}

	void MonitorManager::Run(utils::Thread *thread) {
		// Start the thread of the monitor
		Start(utils::InetAddress::None());
	}

	phantom::Connection * MonitorManager::CreateConnectObject(phantom::server *server_h, phantom::client *client_, phantom::tls_server *tls_server_h, 
		phantom::tls_client *tls_client_h, phantom::connection_hdl con, const std::string &uri, int64_t id) {
		return new Monitor(server_h, client_, tls_server_h, tls_client_h, con, uri, id);
	}

	void MonitorManager::OnDisconnect(Connection *conn) {
		// Update the active time to zero
		Monitor *monitor = (Monitor *)conn;
		monitor->SetActiveTime(0);
	}

	bool MonitorManager::SendMonitor(int64_t type, const std::string &data) {
		bool bret = false;
		MonitorConfigure& monitor_configure = Configure::Instance().monitor_configure_;
		if (!monitor_configure.enabled_){
			LOG_TRACE("Failed to send message, configuration file is not allowed");
			return true;
		}

		do {
			utils::MutexGuard guard(conns_list_lock_);
			// Get the connection of the client
			Monitor *monitor = (Monitor *)GetClientConnection();
			if (NULL == monitor || !monitor->IsActive()) {
				break;
			}

			std::error_code ignore_ec;
			// Send the monitor request
			if (!monitor->SendRequest(type, data, ignore_ec)) {
				LOG_ERROR("Failed to send a monitor message, (type: " FMT_I64 ") from ip(%s) (%d:%s)", type, monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value(), ignore_ec.message().c_str());
				break;
			}
			bret = true;
		} while (false);
		
		return bret;
	}

	bool MonitorManager::OnMonitorHello(protocol::WsMessage &message, int64_t conn_id) {
		bool bret = false;
		do {
			// Get the connection
			Monitor *monitor = (Monitor*)GetConnection(conn_id);
			std::error_code ignore_ec;

			monitor::Hello hello;
			// Parse hello message
			if (!hello.ParseFromString(message.data())) {
				LOG_ERROR("Failed to receive hello message from ip(%s) (%d:parse hello message)", monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value());
				break;
			}
			// Check the phantom version
			if (hello.service_version() != 3) {
				LOG_ERROR("Failed to receive hello message from ip(%s) (%d: monitor center version is low (3))", monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value());
				break;
			}

			connect_time_out_ = hello.connection_timeout();

			LOG_INFO("Received a hello message from center (ip: %s, version: %d, timestamp: %lld)", monitor->GetPeerAddress().ToIpPort().c_str(), 
				hello.service_version(), hello.timestamp());

			monitor::Register reg;
			reg.set_id(utils::MD5::GenerateMD5((unsigned char*)monitor_id_.c_str(), monitor_id_.length()));
			reg.set_blockchain_version(phantom::General::PHANTOM_VERSION);
			reg.set_data_version(phantom::General::MONITOR_VERSION);
			reg.set_timestamp(utils::Timestamp::HighResolution());

			// Send the hello request
			if (NULL == monitor || !monitor->SendRequest(monitor::MONITOR_MSGTYPE_REGISTER, reg.SerializeAsString(), ignore_ec)) {
				LOG_ERROR("Failed to send register from monitor ip(%s) (%d:%s)", monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value(), ignore_ec.message().c_str());
				break;
			}

			bret = true;
		} while (false);
		
		return bret;
	}

	bool MonitorManager::OnMonitorRegister(protocol::WsMessage &message, int64_t conn_id) {
		bool bret = false;
		do {
			// Get the connection
			Monitor *monitor = (Monitor*)GetConnection(conn_id);
			std::error_code ignore_ec;

			monitor::Register reg;
			// Parse the register message
			if (!reg.ParseFromString(message.data())) {
				LOG_ERROR("Failed to receive register from ip(%s) (%d:failed to parse register message)", monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value());
				break;
			}

			// Set the active time
			monitor->SetActiveTime(utils::Timestamp::HighResolution());

			LOG_INFO("Received a register message from center (ip: %s, timestamp: " FMT_I64 ")", monitor->GetPeerAddress().ToIpPort().c_str(), reg.timestamp());
			bret = true;
		} while (false);

		return bret;
	}

	bool MonitorManager::OnPhantomStatus(protocol::WsMessage &message, int64_t conn_id) {
		monitor::PhantomStatus phantom_status;
		// Get the status of phantom
		GetPhantomStatus(phantom_status);

		bool bret = true;
		std::error_code ignore_ec;
		// Get the connection
		Connection *monitor = GetConnection(conn_id);

		// Send the response of phantom status
		if (NULL == monitor || !monitor->SendResponse(message, phantom_status.SerializeAsString(), ignore_ec)) {
			bret = false;
			LOG_ERROR("Failed to send phantom status from ip(%s) (%d:%s)", monitor->GetPeerAddress().ToIpPort().c_str(),
				ignore_ec.value(), ignore_ec.message().c_str());
		}
		return bret;
	}

	bool MonitorManager::OnLedgerStatus(protocol::WsMessage &message, int64_t conn_id) {
		// Get the ledger status
		monitor::LedgerStatus ledger_status;
		ledger_status.mutable_ledger_header()->CopyFrom(LedgerManager::Instance().GetLastClosedLedger());
		ledger_status.set_transaction_size(GlueManager::Instance().GetTransactionCacheSize());
		ledger_status.set_account_count(LedgerManager::Instance().GetAccountNum());
		ledger_status.set_timestamp(utils::Timestamp::HighResolution());

		bool bret = true;
		std::error_code ignore_ec;
		// Get the connection
		Monitor *monitor = (Monitor *)GetConnection(conn_id);

		// Send the response of ledger status
		if (NULL == monitor || !monitor->SendResponse(message, ledger_status.SerializeAsString(), ignore_ec)) {
			bret = false;
			LOG_ERROR("Failed to send ledger status from ip(%s) (%d:%s)", monitor->GetPeerAddress().ToIpPort().c_str(),
				ignore_ec.value(), ignore_ec.message().c_str());
		}
		return bret;
	}

	bool MonitorManager::OnSystemStatus(protocol::WsMessage &message, int64_t conn_id) {
		// Get the system status
		monitor::SystemStatus* system_status = new monitor::SystemStatus();
		std::string disk_paths = Configure::Instance().monitor_configure_.disk_path_;
		system_manager_.GetSystemMonitor(disk_paths, system_status);

		bool bret = true;
		std::error_code ignore_ec;

		utils::MutexGuard guard(conns_list_lock_);
		// Get the connection
		Connection *monitor = GetConnection(conn_id);

		// Send the response of system status
		if (NULL == monitor || !monitor->SendResponse(message, system_status->SerializeAsString(), ignore_ec)) {
			bret = false;
			LOG_ERROR("Failed to send system status from ip(%s) (%d:%s)", monitor->GetPeerAddress().ToIpPort().c_str(),
				ignore_ec.value(), ignore_ec.message().c_str());
		}
		if (system_status) {
			delete system_status;
			system_status = NULL;
		}
		return bret;
	}

	Connection * MonitorManager::GetClientConnection() {
		phantom::Connection* monitor = NULL;
		for (auto item : connections_) {
			Monitor *peer = (Monitor *)item.second;
			// Not self
			if (!peer->InBound()) {
				monitor = peer;
				break;
			}
		}
		return monitor;
	}


	void MonitorManager::GetModuleStatus(Json::Value &data) {
		data["name"] = "monitor_manager";
		Json::Value &peers = data["clients"];
		int32_t active_size = 0;
		utils::MutexGuard guard(conns_list_lock_);
		for (auto &item : connections_) {
			item.second->ToJson(peers[peers.size()]);
		}
	}

	void MonitorManager::OnTimer(int64_t current_time) {
		// Reconnect if disconnected
		if (current_time - last_connect_time_ > connect_interval_) {
			utils::MutexGuard guard(conns_list_lock_);
			Monitor *monitor = (Monitor *)GetClientConnection();
			// Check whether the monitor is NULL
			if (NULL == monitor) {
				std::string url = utils::String::Format("ws://%s", Configure::Instance().monitor_configure_.center_.c_str());

				// Reconnect
				Connect(url);
			}
			// Update the last connection time
			last_connect_time_ = current_time;
		}
	}

	void MonitorManager::OnSlowTimer(int64_t current_time) {

		// Update cpu
		system_manager_.OnSlowTimer(current_time);

		// Send alerts
		if (current_time - last_alert_time_ > check_alert_interval_) {
			monitor::AlertStatus alert_status;
			alert_status.set_ledger_sequence(LedgerManager::Instance().GetLastClosedLedger().seq());
			alert_status.set_node_id(PeerManager::Instance().GetPeerNodeAddress());
			monitor::SystemStatus *system_status = alert_status.mutable_system();
			std::string disk_paths = Configure::Instance().monitor_configure_.disk_path_;
			system_manager_.GetSystemMonitor(disk_paths, system_status);

			bool bret = true;
			std::error_code ignore_ec;

			utils::MutexGuard guard(conns_list_lock_);
			Monitor *monitor = (Monitor *)GetClientConnection();

			// Send the request of alert
			if ( monitor && !monitor->SendRequest(monitor::MONITOR_MSGTYPE_ALERT, alert_status.SerializeAsString(), ignore_ec)) {
				bret = false;
				LOG_ERROR("Failed to send alert status message to ip(%s) (%d:%s)", monitor->GetPeerAddress().ToIpPort().c_str(),
					ignore_ec.value(), ignore_ec.message().c_str());
			}

			// Update the checking time of last alert
			last_alert_time_ = current_time;
		}
	}

	bool MonitorManager::GetPhantomStatus(monitor::PhantomStatus &phantom_status) {
		time_t process_uptime = GlueManager::Instance().GetProcessUptime();
		utils::Timestamp time_stamp(utils::GetStartupTime() * utils::MICRO_UNITS_PER_SEC);
		utils::Timestamp process_time_stamp(process_uptime * utils::MICRO_UNITS_PER_SEC);

		monitor::GlueManager *glue_manager = phantom_status.mutable_glue_manager();
		glue_manager->set_system_uptime(time_stamp.ToFormatString(false));
		glue_manager->set_process_uptime(process_time_stamp.ToFormatString(false));
		glue_manager->set_system_current_time(utils::Timestamp::Now().ToFormatString(false));

		monitor::PeerManager *peer_manager = phantom_status.mutable_peer_manager();
		peer_manager->set_peer_id(PeerManager::Instance().GetPeerNodeAddress());

		Json::Value connections;
		PeerManager::Instance().ConsensusNetwork().GetPeers(connections);
		for (size_t i = 0; i < connections.size(); i++) {
			monitor::Peer *peer = peer_manager->add_peer();
			const Json::Value &item = connections[i];
			peer->set_id(item["node_address"].asString());
			peer->set_delay(item["delay"].asInt64());
			peer->set_ip_address(item["ip_address"].asString());
			peer->set_active(item["active"].asBool());
		}
		return true;
	}
}
