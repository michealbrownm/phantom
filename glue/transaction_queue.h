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

#ifndef TRANSACTION_QUEUE_
#define TRANSACTION_QUEUE_

#include <proto/cpp/overlay.pb.h>
#include <proto/cpp/chain.pb.h>
#include <ledger/transaction_frm.h>
#include "utils/thread.h"
#include <set>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace phantom {

	class TransactionQueue{
	public:
		TransactionQueue(uint32_t queue_limit, uint32_t account_txs_limit);
		~TransactionQueue();

		bool Import(TransactionFrm::pointer tx, const int64_t& cur_source_nonce, Result &result);
		protocol::TransactionEnvSet TopTransaction(uint32_t limit);
		uint32_t RemoveTxs(const protocol::TransactionEnvSet& set, bool close_ledger = false);
		void RemoveTxs(std::vector<TransactionFrm::pointer>& txs, bool close_ledger = false);
		void CheckTimeout(int64_t current_time, std::vector<TransactionFrm::pointer>& timeout_txs);
		void CheckTimeoutAndDel(int64_t current_time, std::vector<TransactionFrm::pointer>& timeout_txs);
		bool IsExist(const TransactionFrm::pointer& tx);
		bool IsExist(const std::string& hash);
		void SafeRemoveTx(const std::string& account_address, const int64_t& nonce);
		size_t Size();

		void Query(const uint32_t& num,std::vector<TransactionFrm::pointer>& txs);
		bool Query(const std::string& hash,TransactionFrm::pointer& tx);
	private:

		struct PriorityCompare
		{
			TransactionQueue& transaction_queue_;
			/// Compare transaction by nonce height and fee.
			bool operator()(TransactionFrm::pointer const& first, TransactionFrm::pointer const& second) const
			{
				int64_t const& height1 = first->GetNonce() - transaction_queue_.account_nonce_[first->GetSourceAddress()];
				int64_t const& height2 = second->GetNonce() - transaction_queue_.account_nonce_[second->GetSourceAddress()];
				return height1 < height2 || (height1 == height2 && first->GetGasPrice() > second->GetGasPrice());
			}
		};

		using PriorityQueue = std::multiset<TransactionFrm::pointer, PriorityCompare>;
		PriorityQueue queue_;


		struct TimePriorityCompare
		{
			/// Compare transaction by incoming time
			bool operator()(TransactionFrm::pointer const& first, TransactionFrm::pointer const& second) const
			{
				return first->GetInComingTime() < second->GetInComingTime();
			}
		};

		//time order
		using TimeQueue = std::multiset<TransactionFrm::pointer, TimePriorityCompare>;
		TimeQueue time_queue_;

		using QueueIterPair = std::pair<PriorityQueue::iterator, TimeQueue::iterator>;
		using QueueByNonce = std::map<int64_t, QueueIterPair>;
		using QueueByAddressAndNonce = std::unordered_map<std::string, QueueByNonce>;
		QueueByAddressAndNonce queue_by_address_and_nonce_;

		std::unordered_map<std::string, TransactionFrm::pointer> queue_by_hash_;
		//record account system nonce
		std::unordered_map<std::string, int64_t> account_nonce_;

		uint32_t queue_limit_;
		//Maximum number of transaction per account
		uint32_t account_txs_limit_;

		std::pair<bool, TransactionFrm::pointer> Remove(const std::string& account_address,const int64_t& nonce);
		std::pair<bool, TransactionFrm::pointer> Remove(QueueByAddressAndNonce::iterator& account_it, QueueByNonce::iterator& tx_it, bool del_empty = true);
		void Insert(TransactionFrm::pointer const& tx);

		utils::ReadWriteLock lock_;
	};
}

#endif