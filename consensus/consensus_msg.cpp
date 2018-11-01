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
#include "bft.h"
#include "consensus_msg.h"

namespace phantom {
	ConsensusMsg::ConsensusMsg(const protocol::PbftEnv &pbft_env) :pbft_env_(pbft_env) {
		type_ = "pbft";
		seq_ = Pbft::GetSeq(pbft_env_);
		values_ = Pbft::GetValue(pbft_env_);
		node_address_ = Pbft::GetNodeAddress(pbft_env_);
		hash_ = HashWrapper::Crypto(pbft_env_.SerializeAsString());
	};

	ConsensusMsg::~ConsensusMsg() {}

	bool ConsensusMsg::operator < (const ConsensusMsg &msg) const {
		return memcmp(hash_.c_str(), msg.hash_.c_str(), hash_.size()) < 0;
	}
	bool ConsensusMsg::operator == (const ConsensusMsg &value_frm) const {
		return type_ == "pbft" &&
			type_ == value_frm.type_ &&
			hash_ == value_frm.hash_;
	}

	int64_t ConsensusMsg::GetSeq() const {
		return seq_;
	}

	std::vector<std::string> ConsensusMsg::GetValues() const {
		return values_;
	}

	const char *ConsensusMsg::GetNodeAddress() const{
		return node_address_.c_str();
	}

	std::string ConsensusMsg::GetType() const{
		return type_;
	}

	protocol::PbftEnv ConsensusMsg::GetPbft() const{
		return pbft_env_;
	}

	std::string  ConsensusMsg::GetHash() const {
		return hash_;
	}

	size_t ConsensusMsg::GetSize() const {
		return (size_t)pbft_env_.ByteSize();
	}
}