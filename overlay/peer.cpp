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

#include <proto/cpp/overlay.pb.h>
#include <common/general.h>
#include "peer.h"

namespace phantom {
	Peer::Peer(server *server_h, client *client_h, tls_server *tls_server_h, tls_client *tls_client_h, connection_hdl con, const std::string &uri, int64_t id) :
		Connection(server_h, client_h, tls_server_h, tls_client_h, con, uri, id) {
		active_time_ = 0;
		delay_ = 0;

		peer_ledger_version_ = 0;
		peer_overlay_version_ = 0;
		peer_listen_port_ = 0;
	}

	Peer::~Peer() {}

	utils::InetAddress Peer::GetRemoteAddress() const {
		utils::InetAddress address = GetPeerAddress();
		if (InBound()) {
			address.SetPort((uint16_t)peer_listen_port_);
		}
		return address;
	}

	std::string Peer::GetPeerNodeAddress() const {
		return peer_node_address_;
	}

	int64_t Peer::GetActiveTime() const {
		return active_time_;
	}

	bool Peer::IsActive() const {
		return active_time_ > 0;
	}

	bool Peer::SendPeers(const protocol::Peers &db_peers, std::error_code &ec) {
		return SendRequest(protocol::OVERLAY_MSGTYPE_PEERS, db_peers.SerializeAsString(), ec);
	}

	void Peer::SetPeerInfo(const protocol::Hello &hello) {
		peer_overlay_version_ = hello.overlay_version();
		peer_ledger_version_ = hello.ledger_version();
		peer_version_ = hello.phantom_version();
		peer_listen_port_ = hello.listening_port();
		peer_node_address_ = hello.node_address();
	}

	void Peer::SetActiveTime(int64_t current_time) {
		active_time_ = current_time;
	}

	bool Peer::SendHello(int32_t listen_port, const std::string &node_address, const int64_t &network_id, const std::string &node_rand, std::error_code &ec) {
		protocol::Hello hello;

		hello.set_ledger_version(General::LEDGER_VERSION);
		hello.set_overlay_version(General::OVERLAY_VERSION);
		hello.set_listening_port(listen_port);
		hello.set_phantom_version(General::PHANTOM_VERSION);
		hello.set_node_address(node_address);
		hello.set_node_rand(node_rand);
		hello.set_network_id(network_id);
		return SendRequest(protocol::OVERLAY_MSGTYPE_HELLO, hello.SerializeAsString(), ec);
	}

	void Peer::ToJson(Json::Value &status) const {
		Connection::ToJson(status);

		status["node_address"] = peer_node_address_;
		status["delay"] = delay_;
		status["active"] = IsActive();
		status["active_time"] = active_time_;
	}

	int64_t Peer::GetDelay() const {
		return delay_;
	}

	bool Peer::OnNetworkTimer(int64_t current_time) {
		if (!IsActive() && current_time - connect_start_time_ > 10 * utils::MICRO_UNITS_PER_SEC) {
			LOG_ERROR("Peer(%s) active timeout", GetPeerAddress().ToIpPort().c_str());
			return false;
		} 

		return true;
	}

}