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

#include <overlay/peer_manager.h>
#include <ledger/ledger_manager.h>
#include "glue_manager.h"
#include "ledger_upgrade.h"

namespace phantom {
	LedgerUpgradeFrm::LedgerUpgradeFrm() : recv_time_(0) {}
	LedgerUpgradeFrm::~LedgerUpgradeFrm() {}
	bool LedgerUpgradeFrm::operator < (const LedgerUpgradeFrm &frm) const {
		std::string str1 = msg_.upgrade().SerializeAsString();
		std::string str2 = frm.msg_.upgrade().SerializeAsString();
		return str1.compare(str2) < 0;
	}

	void LedgerUpgradeFrm::ToJson(Json::Value &value) const {
		value["recv_time"] = recv_time_;
		value["address"] = address_;
		value["msg"] = Proto2Json(msg_);
	}

	LedgerUpgrade::LedgerUpgrade() :
		last_send_time_(0){}
	LedgerUpgrade::~LedgerUpgrade() {}

	void LedgerUpgrade::OnTimer(int64_t current_time) {

		do {
			utils::MutexGuard guard(lock_);

			//Delete the expired
			for (LedgerUpgradeFrmMap::iterator iter = current_states_.begin();
				iter != current_states_.end();
				) {
				const LedgerUpgradeFrm &frm = iter->second;
				if (frm.recv_time_ + 300 * utils::MICRO_UNITS_PER_SEC < current_time) {
					current_states_.erase(iter++);
				}
				else {
					iter++;
				}
			}

		} while (false);

		//Send the current state every 30s
		protocol::LedgerUpgradeNotify *notify = NULL;
		do {
			utils::MutexGuard guard(lock_);
			if (current_time - last_send_time_ > 30 * utils::MICRO_UNITS_PER_SEC &&
				local_state_.new_ledger_version() > 0) {
				
				notify = new protocol::LedgerUpgradeNotify;
				notify->set_nonce(current_time);
				*notify->mutable_upgrade() = local_state_;

				std::string raw_data = notify->upgrade().SerializeAsString();
				raw_data += utils::String::ToString(current_time);
				*notify->mutable_signature() = GlueManager::Instance().SignConsensusData(raw_data);

				last_send_time_ = current_time;
			}
		} while (false);

		if (notify && ConsensusManager::Instance().GetConsensus()->IsValidator()){
			PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_LEDGER_UPGRADE_NOTIFY, notify->SerializeAsString());
			Recv(*notify);
		}

		if (notify){
			delete notify;
		}
	}

	void LedgerUpgrade::Recv(const protocol::LedgerUpgradeNotify &msg) {
		const protocol::LedgerUpgrade &upgrade = msg.upgrade();
		const protocol::Signature &sig = msg.signature();
		std::string raw_data = upgrade.SerializeAsString();
		raw_data += utils::String::ToString(msg.nonce());

		if (!PublicKey::Verify(raw_data, sig.sign_data(), sig.public_key())) {
			LOG_ERROR("Failed to verify ledger upgrade message.");
			return;
		} 

		PublicKey pub(sig.public_key());
		LedgerUpgradeFrm frm;
        frm.address_ = pub.GetEncAddress();
		frm.recv_time_ = utils::Timestamp::HighResolution();
		frm.msg_ = msg;

		utils::MutexGuard guard(lock_);
		current_states_[frm.address_] = frm;
	}

	bool LedgerUpgrade::GetValid(const protocol::ValidatorSet &validators, size_t quorum_size, protocol::LedgerUpgrade &proto_upgrade) {
		utils::MutexGuard guard(lock_);
		
		if (current_states_.size() == 0) {
			return false;
		} 

		std::set<std::string> validator_set;
		for (int32_t i = 0; i < validators.validators_size(); i++) {
			validator_set.insert(validators.validators(i).address());
		}

		std::map<LedgerUpgradeFrm, size_t> counter_upgrade;
		for (LedgerUpgradeFrmMap::iterator iter = current_states_.begin();
			iter != current_states_.end();
			iter++
			) {
			const LedgerUpgradeFrm &frm = iter->second;
			if (counter_upgrade.find(frm) == counter_upgrade.end()) {
				counter_upgrade[frm] = 0;
			}

			if (validator_set.find(frm.address_) != validator_set.end()) {
				counter_upgrade[frm] = counter_upgrade[frm] + 1;
			}
		}

		for (std::map<LedgerUpgradeFrm, size_t>::iterator iter = counter_upgrade.begin();
			iter != counter_upgrade.end();
			iter++) {
			if (iter->second >= quorum_size){
				const LedgerUpgradeFrm &frm = iter->first;
				proto_upgrade = frm.msg_.upgrade();
				return true;
			} 
		}

		return false;
	}

	bool LedgerUpgrade::ConfNewVersion(int32_t new_version) {
		local_state_.set_new_ledger_version(new_version);
		LOG_INFO("Pre-configured new version(%d).", new_version);
		return true;
	}

	protocol::LedgerUpgrade LedgerUpgrade::GetLocalState() {
		utils::MutexGuard guard(lock_);
		return local_state_;
	}

	void LedgerUpgrade::LedgerHasUpgrade() {
		utils::MutexGuard guard(lock_);
		local_state_.Clear();
		current_states_.clear();
	}

	void LedgerUpgrade::GetModuleStatus(Json::Value &value) {
		utils::MutexGuard guard(lock_);
		value["local_state"] = Proto2Json(local_state_);
		Json::Value &current_states = value["current_states"];
		for (LedgerUpgradeFrmMap::iterator iter = current_states_.begin();
			iter != current_states_.end();
			iter++) {
			iter->second.ToJson(current_states[current_states.size()]);
		}
	}
}
