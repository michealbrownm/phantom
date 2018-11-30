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
	/* monitor manager */
	class MonitorManager :public utils::Singleton<MonitorManager>,
		public StatusModule,
		public TimerNotify,
		public Network,
		public utils::Runnable {
		friend class utils::Singleton<phantom::MonitorManager>;
	public:
		MonitorManager();
		~MonitorManager();

		/*************************************************
		Function:       Initialize
		Description:    Initialize the variables of this class
		Calls:          StatusModule::RegisterModule; TimerNotify::RegisterModule
		Return:         boolean success or failure
		*************************************************/
		bool Initialize();

		/*************************************************
		Function:       Exit
		Description:    Exit this class
		Calls:          Stop; Join
		Return:         boolean success or failure
		*************************************************/
		bool Exit();

		/*************************************************
		Function:       OnTimer
		Description:    Check the connection of WebSocket
		Calls:          GetClientConnection; Connect
		Input:          current_time The current time
		*************************************************/
		virtual void OnTimer(int64_t current_time) override;

		/*************************************************
		Function:       OnSlowTimer
		Description:    Check alerts and send alerts
		Calls:          GetLastClosedLedger; GetPeerNodeAddress; GetSystemMonitor; 
		                GetClientConnection; SendRequest
		Input:          current_time The current time
		*************************************************/
		virtual void OnSlowTimer(int64_t current_time) override;

		/*************************************************
		Function:       GetModuleStatus
		Description:    Get the status of the monitor
		Input:          data Json::Value The data of status
		*************************************************/
		virtual void GetModuleStatus(Json::Value &data);

		/*************************************************
		Function:       SendMonitor
		Description:    Send the monitor info
		Calls:          GetClientConnection; SendRequest
		Input:          type int64_t The type of monitor
		                data std::string The data of monitor
		Return:         bool success or failure
		*************************************************/
		bool SendMonitor(int64_t type, const std::string &data);
		
	protected:
		/*************************************************
		Function:       Run
		Description:    Start the monitor thread
		Calls:          Start
		Input:          thread utils::Thread The handle of thread
		*************************************************/
		virtual void Run(utils::Thread *thread) override;

	private:
		/*************************************************
		Function:       OnDisconnect
		Description:    Disconnect the monitor
		Calls:          SetActiveTime
		Input:          conn Connection The connection to monitor
		*************************************************/
		virtual void OnDisconnect(Connection *conn);

		/*************************************************
		Function:       CreateConnectObject
		Description:    Create a connection to monitor
		Input:          server_h phantom::server* The http server
						client_ phantom::client* The http client
						tls_server_h phantom::tls_server* The tls server
						tls_client_h phantom::tls_client* The tis client
						con phantom::connection_hdl The handle of connection
						uri const std::string The url
						id int64_t The id of connection
		Return:         The connection of websocket
		*************************************************/
		virtual phantom::Connection *CreateConnectObject(phantom::server *server_h, phantom::client *client_,
			phantom::tls_server *tls_server_h, phantom::tls_client *tls_client_h,
			phantom::connection_hdl con, const std::string &uri, int64_t id);

		// Handlers
		/*************************************************
		Function:       OnMonitorHello
		Description:    The monitor to hello message
		Calls:          SendRequest
		Input:          message protocol::WsMessage The message of monitor
		                conn_id int64_t The id of connection
		Return:         bool success or failure
		*************************************************/
		bool OnMonitorHello(protocol::WsMessage &message, int64_t conn_id);

		/*************************************************
		Function:       OnMonitorRegister
		Description:    The monitor to register message
		Calls:          SendRequest
		Input:          message protocol::WsMessage The message of monitor
		                conn_id int64_t The id of connection
		Return:         bool success or failure
		*************************************************/
		bool OnMonitorRegister(protocol::WsMessage &message, int64_t conn_id);

		/*************************************************
		Function:       OnPhantomStatus
		Description:    The monitor to phantom status message
		Calls:          SendRequest
		Input:          message protocol::WsMessage The message of monitor
		                conn_id int64_t The id of connection
		Return:         bool success or failure
		*************************************************/
		bool OnPhantomStatus(protocol::WsMessage &message, int64_t conn_id);

		/*************************************************
		Function:       OnLedgerStatus
		Description:    The monitor to ledger status message
		Calls:          SendRequest
		Input:          message protocol::WsMessage The message of monitor
		                conn_id int64_t The id of connection
		Return:         bool success or failure
		*************************************************/
		bool OnLedgerStatus(protocol::WsMessage &message, int64_t conn_id);

		/*************************************************
		Function:       OnSystemStatus
		Description:    The monitor to system status message
		Calls:          SendRequest
		Input:          message protocol::WsMessage The message of monitor
		                conn_id int64_t The id of connection
		Return:         bool success or failure
		*************************************************/
		bool OnSystemStatus(protocol::WsMessage &message, int64_t conn_id);

		/*************************************************
		Function:       GetPhantomStatus
		Description:    Get the status of phantom
		Calls:          GetPeers
		Input:          phantom_status monitor::PhantomStatus The status of phantom
		Return:         bool success or failure
		*************************************************/
		bool GetPhantomStatus(monitor::PhantomStatus &phantom_status);

		/*************************************************
		Function:       GetClientConnection
		Description:    Get the Connection of client
		Return:         The connection of client
		*************************************************/
		Connection * GetClientConnection();

	private:
		utils::Thread *thread_ptr_;    /* The pointer of the thread */

		std::string monitor_id_;  /* The id of the monitor */

		uint64_t last_connect_time_; /* The time of last connection */
		uint64_t connect_interval_; /* The interval between two connections */

		uint64_t check_alert_interval_; /* The interval between alerts checking */
		uint64_t last_alert_time_; /* The time of last checking the alert */

		SystemManager system_manager_; /* The system manager */
	};
}

#endif
