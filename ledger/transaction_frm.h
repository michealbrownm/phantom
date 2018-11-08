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

#ifndef TRANSACTION_FRM_H_
#define TRANSACTION_FRM_H_

#include <unordered_map>
#include <utils/common.h>
#include <common/general.h>
#include <ledger/account.h>
#include <overlay/peer.h>
#include <api/web_server.h>
#include <proto/cpp/overlay.pb.h>
#include "operation_frm.h"
#include "environment.h"

namespace phantom {

	class OperationFrm;
	class LedgerFrm;
	class TransactionFrm {
	public:
		typedef std::shared_ptr<phantom::TransactionFrm> pointer;

		std::set<std::string> involved_accounts_;
		std::vector<protocol::TransactionEnvStore> instructions_;
		std::shared_ptr<Environment> environment_;
	public:
		//Valid only when the transaction belongs to a txset.
		TransactionFrm();
		TransactionFrm(const protocol::TransactionEnv &env);
		
		virtual ~TransactionFrm();
		
		static bool AccountFromDB(const std::string &address, AccountFrm::pointer &account_ptr);

		std::string GetContentHash() const;
		std::string GetContentData() const;
		std::string GetFullHash() const;

		void ToJson(Json::Value &result);
		void CacheTxToJson(Json::Value &result);

		std::string GetSourceAddress() const;
		int64_t GetNonce() const;

		const protocol::TransactionEnv &GetTransactionEnv() const;

		bool CheckValid(int64_t last_seq, bool check_priv, int64_t& nonce);
		bool CheckExpr(const std::string &code, const std::string &log_prefix);

		bool SignerHashPriv(AccountFrm::pointer account_ptr, int32_t type) const;

		const protocol::Transaction &GetTx() const;

		Result GetResult() const;

		void Initialize();

		uint32_t LoadFromDb(const std::string &hash);

		bool CheckTimeout(int64_t expire_time);
		void NonceIncrease(LedgerFrm* ledger_frm, std::shared_ptr<Environment> env);
		bool Apply(LedgerFrm* ledger_frm, std::shared_ptr<Environment> env, bool bool_contract = false);
		bool ApplyExpr(const std::string &code, const std::string &log_prefix);

		protocol::TransactionEnv &GetProtoTxEnv() {
			return transaction_env_;
		}

		std::string &GetFullData() {
			return full_data_;
		}

		void ApplyExpireResult(); // for sync node

		int64_t GetSelfGas();

		bool ValidForParameter(bool contract_trigger = false);
		
		bool ValidForApply(std::shared_ptr<Environment> environment, bool check_priv = true);

		bool CheckFee(const int64_t& gas_price, const int64_t& fee_limit, AccountFrm::pointer account);
		static bool AddActualFee(TransactionFrm::pointer bottom_tx, TransactionFrm* txfrm);

		bool PayFee(std::shared_ptr<Environment> environment,int64_t& total_fee);
		bool ReturnFee(int64_t& total_fee);
		int64_t GetFeeLimit() const;
		int64_t GetGasPrice() const;
		int64_t GetActualGas() const;
		void AddActualGas(int64_t gas);

		void SetApplyStartTime(int64_t time);
		void SetApplyEndTime(int64_t time);
		int64_t GetApplyTime();

		void SetMaxEndTime(int64_t end_time);
		int64_t GetMaxEndTime();
		void ContractStepInc(int32_t step);
		int32_t GetContractStep();
		void SetMemoryUsage(int64_t memory_usage);
		int64_t GetMemoryUsage();
		void SetStackRemain(int64_t remain_size);
		int64_t GetStackUsage();
		bool IsExpire(std::string &error_info);
		void EnableChecked();
		const int64_t GetInComingTime() const;

		uint64_t apply_time_;
		int64_t ledger_seq_;
		Result result_;	
		int32_t processing_operation_;
		LedgerFrm* ledger_;

	private:		
		protocol::TransactionEnv transaction_env_;
		std::string hash_;
		std::string full_hash_;
		std::string data_;
		std::string full_data_;
		std::set<std::string> valid_signature_;
		
		int64_t incoming_time_;
		int64_t actual_gas_;
		int64_t actual_gas_for_query_;

		//All the following variables will point to the initial transaction.
		int64_t max_end_time_;
		int32_t contract_step_;
		int64_t contract_memory_usage_;
		int64_t contract_stack_max_vaule_;
		int64_t contract_stack_usage_;
		bool enable_check_;
		int64_t apply_start_time_;
		int64_t apply_use_time_;
	};
};

#endif
