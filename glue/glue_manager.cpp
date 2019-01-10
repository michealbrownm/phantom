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
#include <overlay/peer_manager.h>
#include <ledger/ledger_manager.h>
#include <api/websocket_server.h>
#include "glue_manager.h"

namespace phantom {

	int64_t const  MAX_LEDGER_TIMESPAN_SECONDS = 20 * utils::MICRO_UNITS_PER_SEC;
	int64_t const QUEUE_TRANSACTION_TIMEOUT = 60 * utils::MICRO_UNITS_PER_SEC;
	GlueManager::GlueManager() {
		time_start_consenus_ = 0;
		ledgerclose_check_timer_ = 0;
		check_interval_ = 2 * utils::MICRO_UNITS_PER_SEC;
		start_consensus_timer_ = 0;
		process_uptime_ = 0;
	}
	GlueManager::~GlueManager() {}

	bool GlueManager::Initialize() {

		tx_pool_ = std::make_shared<TransactionQueue>(Configure::Instance().ledger_configure_.queue_limit_,  Configure::Instance().ledger_configure_.queue_per_account_txs_limit_);
		process_uptime_ = time(NULL);
		consensus_ = ConsensusManager::Instance().GetConsensus();
		consensus_->SetNotify(this);

		//start consensus
		start_consensus_timer_ = utils::Timer::Instance().AddTimer(3 * utils::MICRO_UNITS_PER_SEC, 0, [this](int64_t data) {
			StartConsensus("");
		});

		protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();
		if (lcl.version() < General::LEDGER_VERSION) {
			ledger_upgrade_.ConfNewVersion(General::LEDGER_VERSION);
		}

		//init hardfork points
		const utils::StringList &conf_hardfork_points = Configure::Instance().ledger_configure_.hardfork_points_;
		for (utils::StringList::const_iterator iter = conf_hardfork_points.begin();
			iter != conf_hardfork_points.end();
			iter++) {
			hardfork_points_.insert(utils::String::HexStringToBin(*iter));
		}

		StatusModule::RegisterModule(this);
		TimerNotify::RegisterModule(this);
		StartLedgerCloseTimer();
		return true;
	}

	void GlueManager::StartLedgerCloseTimer() {
		//kill the ledger check timer
		utils::Timer::Instance().DelTimer(ledgerclose_check_timer_);
		ledgerclose_check_timer_ = utils::Timer::Instance().AddTimer(MAX_LEDGER_TIMESPAN_SECONDS + 10 * utils::MICRO_UNITS_PER_SEC, 0,
			[this](int64_t data) {
			LOG_INFO("Ledger close timeout, call consensus view change");
			consensus_->OnTxTimeout();
		});
	}

	std::string GlueManager::CalculateTxTreeHash(const std::vector<TransactionFrm::pointer> &tx_array) {
		HashWrapper hash_func;
		for (std::size_t i = 0; i < tx_array.size(); i++) {
			TransactionFrm::pointer env = tx_array[i];
			hash_func.Update(env->GetFullHash());
		}
		return hash_func.Final();
	}

	bool GlueManager::Exit() {
		return true;
	}

	bool GlueManager::StartConsensus(const std::string &last_consavlue) {

		time_start_consenus_ = utils::Timestamp::HighResolution();
		if (!consensus_->IsLeader()) {
			LOG_INFO("Start consensus process, but it is not leader, just waiting");
			return true;
		} 
		else {
			LOG_INFO("Start consensus process, it is leader, just continue");
		}

		protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();
		protocol::TransactionEnvSet txset_raw = tx_pool_->TopTransaction(Configure::Instance().ledger_configure_.max_trans_per_ledger_);

		int64_t next_close_time = utils::Timestamp::Now().timestamp();
		if (next_close_time < lcl.close_time() + Configure::Instance().ledger_configure_.close_interval_) {
			next_close_time = lcl.close_time() + Configure::Instance().ledger_configure_.close_interval_;
		}

		//get previous block proof
		std::string proof;
		Storage::Instance().account_db()->Get(General::LAST_PROOF, proof);

		if (!last_consavlue.empty()) {
			LOG_INFO("Last prepared consvalue not empty, value digest(%s)", 
				utils::String::BinToHexString(HashWrapper::Crypto(last_consavlue)).c_str());
			//protocol::TransactionEnvSet txset_raw = tx_pool_->top.GetRaw();
			if (CheckValue(last_consavlue) == Consensus::CHECK_VALUE_VALID) {
				protocol::ConsensusValue propose_value;
				propose_value.ParseFromString(last_consavlue);
				LOG_INFO("Proposed last consvalue %d tx(s), lcl hash(%s) tx(s)", propose_value.txset().txs_size(),
					utils::String::Bin4ToHexString(lcl.hash()).c_str());

				return consensus_->Request(last_consavlue);
			}
		}


		protocol::ConsensusValue propose_value;
		do {
			*propose_value.mutable_txset() = txset_raw;
			propose_value.set_close_time(next_close_time);
			propose_value.set_ledger_seq(lcl.seq() + 1);
			propose_value.set_previous_ledger_hash(lcl.hash());
			propose_value.set_previous_proof(proof);

			//judge if we need upgrade the ledger
			protocol::ValidatorSet validator_set;
			size_t quorum_size = 0;
			consensus_->GetValidation(validator_set, quorum_size);
			protocol::LedgerUpgrade up;
			if (ledger_upgrade_.GetValid(validator_set, quorum_size + 1, up)) {
				LOG_INFO("Get valid upgrade value(%s)", Proto2Json(up).toFastString().c_str());

				if (lcl.version() < up.new_ledger_version() && up.new_ledger_version() <= General::LEDGER_VERSION) {
					LOG_ERROR("Not propose the invalid upgrade value");
					*propose_value.mutable_ledger_upgrade() = up;
				}
			}

			ProposeTxsResult propose_result;
			LedgerManager::Instance().context_manager_.SyncPreProcess(propose_value, true, propose_result);

			if (propose_result.block_timeout_) {
				//remove the time out tx
				//reduct to 1/2
				protocol::TransactionEnvSet tmp_raw;
				for (int32_t i = 0; i < txset_raw.txs_size() / 2; i ++) {
					*tmp_raw.add_txs() = txset_raw.txs(i);
				}

				txset_raw = tmp_raw;

				continue;
			}

			//need drop some tx
			if (propose_result.need_dropped_tx_.size()) {
				protocol::TransactionEnvSet *txs = propose_value.mutable_txset();
				txs->clear_txs();

				protocol::TransactionEnvSet tmp_raw;
				for (int32_t i = 0; i < txset_raw.txs_size(); i++) {
					if (propose_result.need_dropped_tx_.find(i) != propose_result.need_dropped_tx_.end()) {
						//remove from the cache
						*tmp_raw.add_txs() = txset_raw.txs(i);
					} else{
						*txs->add_txs() = txset_raw.txs(i);
					}
				}
				tx_pool_->RemoveTxs(tmp_raw);
			} 

			if (propose_result.cons_validation_.error_tx_ids_size() > 0 ||
				propose_result.cons_validation_.expire_tx_ids_size() > 0) {
				*propose_value.mutable_validation() = propose_result.cons_validation_;
			}

			LOG_INFO("Check validation, validation(%d,%d) ",
				propose_result.cons_validation_.expire_tx_ids_size(), propose_result.cons_validation_.error_tx_ids_size());

			break;
		} while (true);

		LOG_INFO("Proposed %d tx(s), lcl hash(%s) tx(s)", propose_value.txset().txs_size(),
			utils::String::Bin4ToHexString(lcl.hash()).c_str());
		consensus_->Request(propose_value.SerializeAsString());
		return true;
	}

	bool GlueManager::OnTransaction(TransactionFrm::pointer tx, Result &err) {
		TopicKey key(tx->GetSourceAddress(), tx->GetNonce());
		std::string hash_value = tx->GetContentHash();
		std::string address = tx->GetSourceAddress();

		do {
			if (tx_pool_->IsExist(tx->GetContentHash())){
				//dont't reply the tx, then break;
				//err.set_code(protocol::ERRCODE_ALREADY_EXIST);
				//err.set_desc(utils::String::Format("Receive duplicate transaction, source address(%s) hash(%s)", address.c_str(), utils::String::Bin4ToHexString(hash_value).c_str()));
				LOG_INFO("Receive duplicate transaction, source address(%s) hash(%s)", address.c_str(), utils::String::Bin4ToHexString(hash_value).c_str());
				break;
			}

			//验证交易有效性
			int64_t nonce = 0;
			if (!tx->CheckValid(/*high_sequence*/ -1, true, nonce)) {
				err = tx->GetResult();
				Json::Value js;
				js["action"] = "apply";
				js["error_code"] = err.code();
				js["desc"] = err.desc();
				LOG_ERROR("Check transaction failed, source address(%s) hash(%s), return(%s)",
					address.c_str(), utils::String::Bin4ToHexString(hash_value).c_str(), js.toFastString().c_str());
				break;
			}

			if (!tx_pool_->Import(tx, nonce, err)) {
				LOG_ERROR("source address(%s) tx hash(%s) insert tx queue failed",
					address.c_str(), utils::String::Bin4ToHexString(hash_value).c_str());
			}

		} while (false);


		return err.code() == protocol::ERRCODE_SUCCESS;
	}

	void GlueManager::OnConsensus(const ConsensusMsg &msg) {
		consensus_->OnRecv(msg);
	}

	void GlueManager::OnTimer(int64_t current_time) {
		//check the timeout transaction

		std::vector<TransactionFrm::pointer> timeout_txs;
		tx_pool_->CheckTimeoutAndDel(current_time, timeout_txs);

		if (timeout_txs.size() > 0 ){
			NotifyErrTx(timeout_txs);
		} 

		ledger_upgrade_.OnTimer(current_time);
	}


	void GlueManager::NotifyErrTx(std::vector<TransactionFrm::pointer> &txs) {
		for (std::vector<TransactionFrm::pointer>::iterator iter = txs.begin();
			iter != txs.end();
			iter++) {

//			TransactionFrm::pointer tx = *iter;
//			WebSocketServer::Instance().BroadcastChainTxMsg(tx->GetContentHash(), tx->GetSourceAddress(), 
//				tx->GetResult(), tx->GetResult().code() == protocol::ERRCODE_SUCCESS ? protocol::ChainTxStatus_TxStatus_COMPLETE : protocol::ChainTxStatus_TxStatus_FAILURE);
		}
	}

	void GlueManager::UpdateValidators(const protocol::ValidatorSet &validators, const std::string &proof) {
		consensus_->UpdateValidators(validators, proof);
	}

	void GlueManager::LedgerHasUpgrade() {
		ledger_upgrade_.LedgerHasUpgrade();
	}

	void GlueManager::OnRecvLedgerUpMsg(const protocol::LedgerUpgradeNotify &msg) {
		ledger_upgrade_.Recv(msg);
	}

	protocol::Signature GlueManager::SignConsensusData(const std::string &data) {
		return consensus_->SignData(data);
	}

	std::string GlueManager::OnValueCommited(int64_t request_seq, const std::string &value, const std::string &proof, bool calculate_total) {
		protocol::ConsensusValue request;
		request.ParseFromString(value);

		//TransactionSetFrm txset_frm(request.txset());

		//temp upgrade the validator, need done by ledger manager

		//write to db
		int64_t time_start = utils::Timestamp::HighResolution();
		
		protocol::ConsensusValue req;
		req.ParseFromString(value);
		//call consensus
		LedgerManager::Instance().OnConsent(req, proof);

		int64_t time_use = utils::Timestamp::HighResolution() - time_start;

		//delete the cache 
		//size_t ret1 = RemoveTxset(txset_frm);
		tx_pool_->RemoveTxs(request.txset(),true);

		//start time
		int64_t next_interval = GetIntervalTime(request.txset().txs_size() == 0);
		int64_t next_timestamp = next_interval + req.close_time();
		int64_t seq = req.ledger_seq();
		Global::Instance().GetIoService().post([next_timestamp, time_use, seq, this]() {
			int64_t waiting_time = next_timestamp - utils::Timestamp::Now().timestamp();
			if (waiting_time <= 0)  waiting_time = 1;

			if (consensus_->IsLeader()) {
				start_consensus_timer_ = utils::Timer::Instance().AddTimer(waiting_time, 0, [this](int64_t data) {
					StartConsensus("");
				});

				LOG_INFO("Close ledger(" FMT_I64 ") successful, use time(" FMT_I64 "ms), waiting(" FMT_I64 "ms) to start next consensus",
					seq, (int64_t)(time_use / utils::MILLI_UNITS_PER_SEC), (int64_t)(waiting_time / utils::MILLI_UNITS_PER_SEC));
			}
			else {
				LOG_INFO("Close ledger(" FMT_I64 ") successful, use time(" FMT_I64 "ms), waiting(" FMT_I64 "ms) to check next consensus",
					seq, (int64_t)(time_use / utils::MILLI_UNITS_PER_SEC), (int64_t)(waiting_time / utils::MILLI_UNITS_PER_SEC));
			}

		});

		StartLedgerCloseTimer();
		protocol::LedgerHeader lcl1 = LedgerManager::Instance().GetLastClosedLedger();
		return lcl1.hash();
	}

	void GlueManager::OnViewChanged(const std::string &last_consvalue) {
		LOG_INFO("Consenter on view changed");
		StartConsensus(last_consvalue);
		StartLedgerCloseTimer();
	}

	bool GlueManager::CheckValueAndProof(const std::string &consensus_value, const std::string &proof) {
		protocol::ConsensusValue proto_value;
		if (!proto_value.ParseFromString(consensus_value)) {
			LOG_ERROR("Parse consensus value failed");
			return false;
		}

		protocol::ValidatorSet set;
		if (!LedgerManager::Instance().GetValidators(proto_value.ledger_seq() - 1, set)) {
			LOG_ERROR("Check valid failed, get validator failed of ledger seq(" FMT_I64 ")",
				proto_value.ledger_seq() - 1);
			return false;
		}
		
		//if it exist in hardfork point, we ignore the proof
		std::string consensus_value_hash = HashWrapper::Crypto(consensus_value);
		std::set<std::string>::const_iterator iter = hardfork_points_.find(consensus_value_hash);
		return CheckValueHelper(proto_value, -1) == Consensus::CHECK_VALUE_VALID &&   //-1 not check time
			(consensus_->CheckProof(set, HashWrapper::Crypto(consensus_value), proof)
			|| iter != hardfork_points_.end());
	}

	int32_t GlueManager::CheckValue(const std::string &value) {
		protocol::ConsensusValue consensus_value;
		if (!consensus_value.ParseFromString(value)) {
			LOG_ERROR("Parse consensus value failed");
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		int32_t check_helper_ret = CheckValueHelper(consensus_value, utils::Timestamp::Now().timestamp());
		if (check_helper_ret > 0) {
			return check_helper_ret;
		}

		ProposeTxsResult ignor_cons_validation;
		if (!LedgerManager::Instance().context_manager_.SyncPreProcess(consensus_value,
			false,
			ignor_cons_validation)) {
			LOG_ERROR("Pre process consvalue failed");
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		return Consensus::CHECK_VALUE_VALID;
	}

	int32_t GlueManager::CheckValueHelper(const protocol::ConsensusValue &consensus_value, int64_t now) {
		if (consensus_value.ByteSize() >= General::TXSET_LIMIT_SIZE + (int32_t)(2 * utils::BYTES_PER_MEGA)) {
			LOG_ERROR("Consensus value byte size(%d) will be exceed than limit(%d)",
				consensus_value.ByteSize(),
				General::TXSET_LIMIT_SIZE + 2 * utils::BYTES_PER_MEGA);
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		protocol::LedgerHeader lcl = LedgerManager::Instance().GetLastClosedLedger();

		//check previous ledger sequence
		if (consensus_value.ledger_seq() != lcl.seq() + 1) {
			LOG_ERROR("Check value failed, previous ledger seq(" FMT_I64 ") + 1 not equal to consensus message ledger seq( " FMT_I64 ")",
				lcl.seq(),
				consensus_value.ledger_seq());
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		//check previous hash
		if (consensus_value.previous_ledger_hash() != lcl.hash()) {
			LOG_ERROR("Check value failed, previous ledger(seq:" FMT_I64 ") hash(%s) not equal to consensus message ledger hash(%s)",
				lcl.seq(),
				utils::String::Bin4ToHexString(lcl.hash()).c_str(),
				utils::String::Bin4ToHexString(consensus_value.previous_ledger_hash()).c_str());
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		//not too closed, can tolerate 1 second 
		if (now != -1 && 
			!(
			now > (consensus_value.close_time() - utils::MICRO_UNITS_PER_SEC)
			&& 
			consensus_value.close_time() >= lcl.close_time() + Configure::Instance().ledger_configure_.close_interval_
			)
			) {
			LOG_WARN("Now time(" FMT_I64 ") > Close time(" FMT_I64 ") > (lcl time(" FMT_I64 ") + interval(" FMT_I64")) Not valid. Consensus network time error!", 
				now, consensus_value.close_time() ,
				lcl.close_time(), Configure::Instance().ledger_configure_.close_interval_ );
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		if (consensus_value.has_ledger_upgrade()) {
			const protocol::LedgerUpgrade &upgrade = consensus_value.ledger_upgrade();
			if (upgrade.new_ledger_version() != 0) {
				if (lcl.version() >= upgrade.new_ledger_version()) {
					LOG_ERROR("Check value failed,  new version(" FMT_I64 ") less or equal than lcl ledger version(" FMT_I64 ")",
						upgrade.new_ledger_version(), lcl.version());
					return Consensus::CHECK_VALUE_MAYVALID;
				}

				if (upgrade.new_ledger_version() > General::LEDGER_VERSION) {
					LOG_ERROR("Check value failed, new ledger version (" FMT_I64 ") large than program version(%u)",
						upgrade.new_ledger_version(),
						General::LEDGER_VERSION);
					return Consensus::CHECK_VALUE_MAYVALID;
				}
			}

			//normal block should not exit the upgarde
			bool new_validator_exist = !upgrade.new_validator().empty();
			std::string consensus_value_hash = HashWrapper::Crypto(consensus_value.SerializeAsString());
			if (hardfork_points_.end() == hardfork_points_.find(consensus_value_hash) && new_validator_exist) {
				return Consensus::CHECK_VALUE_MAYVALID;
			} 
		}

		//check the second block
		if (lcl.seq() == 1 && consensus_value.previous_proof() != "") {
			LOG_ERROR("Check value failed, the second consensus value's prevous proof filed must be empty");
			return Consensus::CHECK_VALUE_MAYVALID;
		}

		//check this proof 
		if (lcl.seq() > 1) {
			//get pre pre ledger validator
			protocol::ValidatorSet set;
			if (!LedgerManager::Instance().GetValidators(consensus_value.ledger_seq() - 2, set)) {
				LOG_ERROR("Check value failed, get validator failed of ledger seq(" FMT_I64 ")",
					consensus_value.ledger_seq() - 2);
				return Consensus::CHECK_VALUE_MAYVALID;
			}

			//check if the consensus value hash is forked
			std::set<std::string>::const_iterator iter = hardfork_points_.find(lcl.consensus_value_hash());
			if (iter == hardfork_points_.end() && !consensus_->CheckProof(set, lcl.consensus_value_hash(), consensus_value.previous_proof())) {
					LOG_ERROR("Check value failed, proof not valid");
					return Consensus::CHECK_VALUE_MAYVALID;
			}
		}

		return Consensus::CHECK_VALUE_VALID;
	}

	void GlueManager::SendConsensusMessage(const std::string &message) {
		Global::Instance().GetIoService().post([this, message] (){
			PeerManager::Instance().Broadcast(protocol::OVERLAY_MSGTYPE_PBFT, message);

			protocol::PbftEnv env;
			env.ParseFromString(message);
			ConsensusMsg msg(env);
			LOG_INFO("Receive consensus from self node address(%s) sequence(" FMT_I64 ") pbft type(%s)",
				msg.GetNodeAddress(), msg.GetSeq(),PbftDesc::GetMessageTypeDesc(msg.GetPbft().pbft().type()));
			consensus_->OnRecv(msg);
		});
	}

	std::string GlueManager::FetchNullMsg() {
		return "null";
	}

	void GlueManager::GetModuleStatus(Json::Value &data) {
		data["name"] = "glue_manager";
		data["transaction_size"] = (Json::UInt64)tx_pool_->Size();

		Json::Value &system_json = data["system"];
		utils::Timestamp time_stamp(utils::GetStartupTime() * utils::MICRO_UNITS_PER_SEC);
		system_json["uptime"] = time_stamp.ToFormatString(false);
		utils::Timestamp process_time_stamp(process_uptime_ * utils::MICRO_UNITS_PER_SEC);
		system_json["process_uptime"] = process_time_stamp.ToFormatString(false);
		system_json["current_time"] = utils::Timestamp::Now().ToFormatString(false);
		 
		ledger_upgrade_.GetModuleStatus(data["ledger_upgrade"]);
	}

	int64_t GlueManager::GetIntervalTime(bool empty_block) {
		return Configure::Instance().ledger_configure_.close_interval_;
	}

	void GlueManager::OnResetCloseTimer() {
		StartLedgerCloseTimer();
	}

	std::string GlueManager::DescConsensusValue(const std::string &request) {
		protocol::ConsensusValue value;
		value.ParseFromString(request);
		return utils::String::Format("value hash(%s) | close time(" FMT_I64 ") | lcl hash(%s) | ledger seq(" FMT_I64 ") ", 
			utils::String::BinToHexString(HashWrapper::Crypto(request)).c_str(),
			value.close_time(),
			utils::String::Bin4ToHexString(value.previous_ledger_hash()).c_str(),
			value.ledger_seq());
	}

	time_t GlueManager::GetProcessUptime() {
		return process_uptime_;
	}

	size_t GlueManager::GetTransactionCacheSize() {
		return tx_pool_->Size();
	}

	void GlueManager::QueryTransactionCache(const uint32_t& num, std::vector<TransactionFrm::pointer>& txs){
		tx_pool_->Query(num,txs);
	}

	bool GlueManager::QueryTransactionCache(const std::string& hash, TransactionFrm::pointer& tx){
		return tx_pool_->Query(hash, tx);
	}

}