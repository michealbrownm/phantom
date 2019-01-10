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

#ifndef MONITOR_MANAGER_H_
#define MONITOR_MANAGER_H_

#include <proto/cpp/chain.pb.h>
#include <common/network.h>
#include <monitor/system_manager.h>

namespace phantom {
	class MonitorManager :public utils::Singleton<MonitorManager>,
		public StatusModule,
		public TimerNotify,
		public Network,
		public utils::Runnable {
		friend class utils::Singleton<phantom::MonitorManager>;
	public:
		MonitorManager();
		~MonitorManager();

		
		//virtual bool Send(const ZMQTaskType type, const std::string& buf);

		bool Initialize();
		bool Exit();

		virtual void OnTimer(int64_t current_time) override;
		virtual void OnSlowTimer(int64_t current_time) override;
		virtual void GetModuleStatus(Json::Value &data);

		bool SendMonitor(int64_t type, const std::string &data);
		
	protected:
		virtual void Run(utils::Thread *thread) override;

	private:
		virtual void OnDisconnect(Connection *conn);
		virtual phantom::Connection *CreateConnectObject(phantom::server *server_h, phantom::client *client_,
			phantom::tls_server *tls_server_h, phantom::tls_client *tls_client_h,
			phantom::connection_hdl con, const std::string &uri, int64_t id);

		// Handlers
		bool OnMonitorHello(protocol::WsMessage &message, int64_t conn_id);
		bool OnMonitorRegister(protocol::WsMessage &message, int64_t conn_id);
		bool OnPhantomStatus(protocol::WsMessage &message, int64_t conn_id);
		bool OnLedgerStatus(protocol::WsMessage &message, int64_t conn_id);
		bool OnSystemStatus(protocol::WsMessage &message, int64_t conn_id);

		bool GetPhantomStatus(monitor::PhantomStatus &phantom_status);
		Connection * GetClientConnection();

	private:
		utils::Thread *thread_ptr_;

		std::string monitor_id_;

		uint64_t last_connect_time_;
		uint64_t connect_interval_;

		uint64_t check_alert_interval_;
		uint64_t last_alert_time_;

		SystemManager system_manager_;
	};
}

#endif