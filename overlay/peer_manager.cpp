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
#include <glue/glue_manager.h>
#include <proto/cpp/overlay.pb.h>
#include <ledger/ledger_manager.h>
#include <main/configure.h>
#include <ledger/transaction_frm.h>
#include "peer_manager.h"

namespace phantom {

	void PeerManager::Run(utils::Thread *thread) {
		const P2pNetwork &p2p_configure = Configure::Instance().p2p_configure_.consensus_network_configure_;
		utils::InetAddress listen_address_ = utils::InetAddress::Any();
		listen_address_.SetPort(p2p_configure.listen_port_);
		consensus_network_->Start(listen_address_);
	}

	PeerManager::PeerManager()
		:consensus_network_(NULL),
		thread_ptr_(NULL),
		priv_key_(SIGNTYPE_CFCASM2),
		cert_enabled_(false) {}

	PeerManager::~PeerManager() {
		if (thread_ptr_) {
			delete thread_ptr_;
		}
		if (consensus_network_) {
			delete consensus_network_;
		}
	}

	bool PeerManager::Initialize(char *serial_num, bool cert_enabled) {
		cert_enabled_ = cert_enabled;

		if (serial_num != NULL) {
			serial_num_ = serial_num;
		}

		if (!priv_key_.From(Configure::Instance().p2p_configure_.node_private_key_)) {
			LOG_ERROR("Initialize node private key failed");
			return false;
		}
		peer_node_address_ = priv_key_.GetEncAddress();

		SslParameter ssl_parameter;
		ssl_parameter.enable_ = false;

		consensus_network_ = new PeerNetwork(ssl_parameter);
		if (!consensus_network_->Initialize(peer_node_address_)) {
			return false;
		}

		thread_ptr_ = new utils::Thread(this);
		if (!thread_ptr_->Start("peer-manager")) {
			return false;
		}
		StatusModule::RegisterModule(this);
		TimerNotify::RegisterModule(this);

		return true;
	}

	bool PeerManager::Exit() {
		bool ret1 = false;
		bool ret2 = false;
		if (consensus_network_) {
			consensus_network_->Stop();
		}
		if (thread_ptr_) {
			ret1 = thread_ptr_->JoinWithStop();
		}
		if (consensus_network_) {
			ret2 = consensus_network_->Exit();
		}
		return ret1 && ret2;
	}


	void PeerManager::Broadcast(int64_t type, const std::string &data) {
		if (consensus_network_) consensus_network_->BroadcastMsg(type, data);
	}


	bool PeerManager::SendRequest(int64_t peer_id, int64_t type, const std::string &data) {
		if (consensus_network_) consensus_network_->SendRequest(peer_id, type, data);
		return true;
	}

	void PeerManager::GetModuleStatus(Json::Value &data) {
		data["name"] = "peer_manager";
		data["peer_node_address"] = peer_node_address_;
		if (consensus_network_) consensus_network_->GetModuleStatus(data["consensus_network"]);
	}

	void PeerManager::OnSlowTimer(int64_t current_time) {
		if (!cert_enabled_) {
			return;
		}
	}
}