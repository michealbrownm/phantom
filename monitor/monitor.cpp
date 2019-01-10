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

#include <common/general.h>
#include "monitor.h"

namespace phantom {
	Monitor::Monitor(phantom::server *server_h, phantom::client *client_h, phantom::tls_server *tls_server_h, phantom::tls_client *tls_client_h,
		phantom::connection_hdl con, const std::string &uri, int64_t id) : 
		Connection(server_h, client_h, tls_server_h, tls_client_h, con, uri, id), active_time_(0){
	}

	void Monitor::SetSessionId(const std::string &session_id) {
		session_id_ = session_id;
	}

	void Monitor::SetActiveTime(int64_t current_time) {
		active_time_ = current_time;
	}

	int64_t Monitor::GetActiveTime() const {
		return active_time_;
	}

	bool Monitor::IsActive() const {
		return active_time_ > 0;
	}

	utils::InetAddress Monitor::GetRemoteAddress() const {
		utils::InetAddress address = GetPeerAddress();
		return address;
	}

	std::string Monitor::GetPeerNodeAddress() const {
		return bubi_node_address_;
	}

	void Monitor::SetBubiInfo(const protocol::ChainStatus &hello) {
		monitor_version_ = hello.monitor_version();
		bubi_ledger_version_ = hello.ledger_version();
		phantom_version_ = hello.phantom_version();
		bubi_node_address_ = hello.self_addr();
	}

	bool Monitor::SendHello(int32_t listen_port, const std::string &node_address, std::error_code &ec) {
		protocol::ChainStatus hello;
		hello.set_monitor_version(phantom::General::MONITOR_VERSION);
		hello.set_ledger_version(phantom::General::LEDGER_VERSION);
		hello.set_phantom_version(phantom::General::PHANTOM_VERSION);
		hello.set_self_addr(node_address);

		return SendRequest(protocol::CHAIN_HELLO, hello.SerializeAsString(), ec);
	}
}

