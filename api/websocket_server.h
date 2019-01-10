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

#ifndef WEBSOCKET_SERVER_H_
#define WEBSOCKET_SERVER_H_

#include <proto/cpp/chain.pb.h>
#include <common/network.h>
#include <monitor/system_manager.h>

namespace phantom {

	class WsPeer : public Connection {
	private:

		//Peer infomation
		std::set<std::string> tx_filter_address_;
	public:
		WsPeer(server *server_h, client *client_h, tls_server *tls_server_h, tls_client *tls_client_h, connection_hdl con, const std::string &uri, int64_t id);
		virtual ~WsPeer();

		bool Set(const protocol::ChainSubscribeTx &sub);
		bool Filter(const protocol::TransactionEnvStore &tx_msg);
	};

	class WebSocketServer :public utils::Singleton<WebSocketServer>,
		public StatusModule,
		public Network,
		public utils::Runnable {
		friend class utils::Singleton<phantom::WebSocketServer>;
	public:
		WebSocketServer();
		~WebSocketServer();

		
		//virtual bool Send(const ZMQTaskType type, const std::string& buf);

		bool Initialize(WsServerConfigure & ws_server_configure);
		bool Exit();

		// Handlers
		bool OnChainHello(protocol::WsMessage &message, int64_t conn_id);
		bool OnChainPeerMessage(protocol::WsMessage &message, int64_t conn_id);
		bool OnSubmitTransaction(protocol::WsMessage &message, int64_t conn_id);
		bool OnSubscribeTx(protocol::WsMessage &message, int64_t conn_id);

		void BroadcastMsg(int64_t type, const std::string &data);
		void BroadcastChainTxMsg(const protocol::TransactionEnvStore& txMsg);

		virtual Connection *CreateConnectObject(server *server_h, client *client_,
			tls_server *tls_server_h, tls_client *tls_client_h,
			connection_hdl con, const std::string &uri, int64_t id);

		virtual void GetModuleStatus(Json::Value &data);
	protected:
		virtual void Run(utils::Thread *thread) override;

	private:
		utils::Thread *thread_ptr_;

		uint64_t last_connect_time_;
		uint64_t connect_interval_;
	};
}

#endif