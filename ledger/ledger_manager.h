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

#ifndef LEDGER_MANAGER_H_
#define LEDGER_MANAGER_H_

#include <utils/headers.h>
#include <utils/exprparser.h>
#include <utils/entry_cache.h>
#include <common/general.h>
#include <common/storage.h>
#include <common/private_key.h>
#include <main/configure.h>
#include <overlay/peer.h>
#include "ledger/ledger_frm.h"
#include "ledgercontext_manager.h"
#include "environment.h"
#include "kv_trie.h"
#include "proto/cpp/consensus.pb.h"

#ifdef WIN32
#include <leveldb/leveldb.h>
#include <leveldb/slice.h>
#else
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#endif

namespace phantom {
	class LedgerFetch;
	class ContractManager;
	class LedgerManager : public utils::Singleton<phantom::LedgerManager>,
		public phantom::TimerNotify,
		public phantom::StatusModule {
		friend class utils::Singleton<phantom::LedgerManager>;
		friend class LedgerFetch;
	public:

		bool Initialize();
		bool Exit();

		int OnConsent(const protocol::ConsensusValue &value, const std::string& proof);

		protocol::LedgerHeader GetLastClosedLedger();

		int GetAccountNum();

		void OnRequestLedgers(const protocol::GetLedgers &message, int64_t peer_id);

		void OnReceiveLedgers(const protocol::Ledgers &message, int64_t peer_id);

		bool GetValidators(int64_t seq, protocol::ValidatorSet& validators_set);
		const protocol::ValidatorSet& Validators()
		{
			return validators_;
		}

		static bool FeesConfigGet(const std::string& hash, protocol::FeeConfig &fee);
		bool ConsensusValueFromDB(int64_t seq, protocol::ConsensusValue& request);
		protocol::FeeConfig GetCurFeeConfig();

		Result DoTransaction(protocol::TransactionEnv& env, LedgerContext *ledger_context); // -1: false, 0 : successs, > 0 exception
		void NotifyLedgerClose(LedgerFrm::pointer closing_ledger, bool has_upgrade);

		virtual void OnTimer(int64_t current_time) override;
		virtual void OnSlowTimer(int64_t current_time) override;
		virtual void GetModuleStatus(Json::Value &data);

		static void CreateHardforkLedger();
	public:
		utils::Mutex gmutex_;
		Json::Value statistics_;
		KVTrie* tree_;

		LedgerContextManager context_manager_;
	private:
		LedgerManager();
		~LedgerManager();

		void RequestConsensusValues(int64_t pid, protocol::GetLedgers& gl, int64_t time);

		int64_t GetMaxLedger();

		bool CloseLedger(const protocol::ConsensusValue& request, const std::string& proof);

		bool CreateGenesisAccount();

		static void ValidatorsSet(std::shared_ptr<WRITE_BATCH> batch, const protocol::ValidatorSet& validators);
		static bool ValidatorsGet(const std::string& hash, protocol::ValidatorSet& vlidators_set);

		static void FeesConfigSet(std::shared_ptr<WRITE_BATCH> batch, const protocol::FeeConfig &fee);
		
		LedgerFrm::pointer last_closed_ledger_;
		protocol::ValidatorSet validators_;
		std::string proof_;

		utils::ReadWriteLock lcl_header_mutex_;
		protocol::LedgerHeader lcl_header_;
		int64_t chain_max_ledger_probaly_;

		utils::ReadWriteLock fee_config_mutex_;
		protocol::FeeConfig fees_;

		struct SyncStat{
			int64_t send_time_;
			protocol::GetLedgers gl_;
			int64_t probation_; //
			SyncStat(){
				send_time_ = 0;
				probation_ = 0;
			}
			Json::Value ToJson(){
				Json::Value v;
				v["send_time"] = send_time_;
				v["probation"] = probation_;
				v["gl"] = Proto2Json(gl_);
				return v;
			}
		};
		
		struct Sync{
			int64_t update_time_;
			/*std::map<int64_t, int> buffer_;*/
			std::map<int64_t, SyncStat> peers_;
			Sync(){
				update_time_ = 0;
			}
			Json::Value ToJson(){
				Json::Value v;
				v["update_time"] = update_time_;
				Json::Value& peers = v["peers"];
				for (auto it = peers_.begin(); it != peers_.end(); it++){
					Json::Value tmp = it->second.ToJson();
					tmp["pid"] = it->first;
					peers[peers.size()] = tmp;
				}
				return v;
			}
		};

		Sync sync_;
	};
}
#endif

