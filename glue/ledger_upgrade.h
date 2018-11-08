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

#ifndef LEDGER_UPGRADE_H_
#define LEDGER_UPGRADE_H_

#include <overlay/peer.h>

namespace phantom {
	class LedgerUpgradeFrm {
	public:
		~LedgerUpgradeFrm();
		LedgerUpgradeFrm();

		int64_t recv_time_;
		std::string address_;
		protocol::LedgerUpgradeNotify msg_;
		bool operator < (const LedgerUpgradeFrm &frm) const;
		void ToJson(Json::Value &value) const;
	};

	typedef std::map<std::string, LedgerUpgradeFrm> LedgerUpgradeFrmMap;

	class LedgerUpgrade {
	public:
		LedgerUpgrade();
		~LedgerUpgrade();

		void OnTimer(int64_t current_time);
		void Recv(const protocol::LedgerUpgradeNotify &msg);
		bool GetValid(const protocol::ValidatorSet &validators, size_t quorum_size, protocol::LedgerUpgrade &proto_upgrade);
		bool ConfNewVersion(int32_t new_version);
		protocol::LedgerUpgrade GetLocalState();
		void LedgerHasUpgrade();
		void GetModuleStatus(Json::Value &value);

		int64_t last_send_time_;
		protocol::LedgerUpgrade local_state_;
		LedgerUpgradeFrmMap current_states_;
		utils::Mutex lock_;
	};
};

#endif