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

#ifndef CONSENSUS_MSG_
#define CONSENSUS_MSG_

#include <proto/cpp/consensus.pb.h>

namespace phantom {
	class ConsensusMsg {
		int64_t seq_;
		std::string type_;
		protocol::PbftEnv pbft_env_;
		std::vector<std::string> values_;
		std::string node_address_;
		std::string hash_;
	public:
		ConsensusMsg() {}
		ConsensusMsg(const protocol::PbftEnv &pbft_env);
		~ConsensusMsg();

		bool operator < (const ConsensusMsg &msg) const;
		bool operator == (const ConsensusMsg &value_frm) const;
		int64_t GetSeq() const;
		std::vector<std::string> GetValues() const;
		const char *GetNodeAddress() const;
		std::string GetType() const;
		protocol::PbftEnv  GetPbft() const;
		std::string  GetHash() const;
		size_t GetSize() const;
	};
}

#endif