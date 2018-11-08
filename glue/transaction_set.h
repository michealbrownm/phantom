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

#ifndef TRANSACTION_SET_
#define TRANSACTION_SET_

#include <proto/cpp/overlay.pb.h>
#include <proto/cpp/chain.pb.h>
#include <ledger/transaction_frm.h>

namespace phantom {

	
	class TransactionSetFrm {
		protocol::TransactionEnvSet raw_txs_;
		std::map<std::string, int64_t> topic_seqs_;
	public:
		TransactionSetFrm();
		TransactionSetFrm(const protocol::TransactionEnvSet &env);
		~TransactionSetFrm();
		int32_t Add(const TransactionFrm::pointer &tx);
		std::string GetSerializeString() const;
		int32_t Size() const;
		const protocol::TransactionEnvSet &GetRaw() const;
	};
	typedef std::map<int64_t, TransactionFrm::pointer> TransactionFrmMap;

	//Topic key
	class TopicKey {
		std::string topic_;
		int64_t sequence_;
	public:
		TopicKey();
		TopicKey(const std::string &topic, int64_t sequence);
		~TopicKey();

		const std::string &GetTopic() const;
		const int64_t GetSeq() const;

		bool operator<(const TopicKey &key) const;
	};

	typedef std::map<TopicKey, TransactionFrm::pointer> TransactionMap;
}

#endif
