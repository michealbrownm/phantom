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

#ifndef PBFT_H_
#define PBFT_H_

#include "consensus.h"
#include "bft_instance.h"

namespace phantom {

	class Pbft : public Consensus {
		friend class PbftInstance;
		friend class PbftVcInstance;
	private:
		//For pbft instance
		PbftInstanceMap instances_;
		int64_t view_number_;
		int64_t sequence_;

		int64_t ckp_interval_;
		int64_t last_exe_seq_;

		int64_t low_water_mark_;

		size_t fault_number_;

		bool view_active_;

		//For view change 
		PbftVcInstanceMap vc_instances_;

		//For synchronization
		PbftInstanceMap out_pbft_instances_;

		//Check interval
		int64_t last_check_time_;

		//For change view timer
		int64_t new_view_repond_timer_;

		PbftEnvPointer NewPrePrepare(const std::string &value, int64_t sequence);
		protocol::PbftEnv NewPrePrepare(const protocol::PbftPrePrepare &pre_prepare);
		PbftEnvPointer NewPrepare(const protocol::PbftPrePrepare &pre_prepare, int64_t round_number);
		PbftEnvPointer NewCommit(const protocol::PbftPrepare &prepare, int64_t round_number);
		PbftEnvPointer NewCheckPoint(const std::string &state_digest, int64_t seq);
		PbftEnvPointer NewViewChangeRawValue(int64_t view_number, const protocol::PbftPreparedSet &prepared_set);
		PbftEnvPointer NewNewView(PbftVcInstance &vc_instance);
		bool OnPrePrepare(const protocol::Pbft &pre_prepare, PbftInstance &pinstance, int32_t check_value_ret);
		bool OnPrepare(const protocol::Pbft &prepare, PbftInstance &pinstance);
		bool OnCommit(const protocol::Pbft &commit, PbftInstance &pinstance);
		bool OnViewChangeWithRawValue(const protocol::PbftEnv &pbft);
		bool OnNewView(const protocol::PbftEnv &pbft);

		static bool CheckViewChangeWithRawValue(const protocol::PbftViewChangeWithRawValue &view_change, const ValidatorMap &validators);
		bool CreateViewChangeParam(const PbftVcInstance &vc_instance, std::map<int64_t, protocol::PbftEnv> &pre_prepares);
		bool ProcessQuorumViewChange(PbftVcInstance &vc_instance);

		PbftInstance *CreateInstanceIfNotExist(const protocol::PbftEnv &env);
		bool InWaterMark(int64_t seq);
		bool TryExecuteValue();
		static protocol::PbftMessageType GetMessageType(const protocol::PbftEnv &env);
		bool CheckMessageItem(const protocol::PbftEnv &env);
		static bool CheckMessageItem(const protocol::PbftEnv &env, const ValidatorMap &validators);
		bool TraceOutPbftCommit(const protocol::PbftEnv &env);
		bool TraceOutPbftPrePrepare(const protocol::PbftEnv &env);
		void TryDoTraceOut(const PbftInstanceIndex &index, const PbftInstance &instance);
		static size_t GetQuorumSize(size_t size);
		void ClearNotCommitedInstance();

		void LoadValues();
		int32_t LoadVcInstance();
		int32_t LoadValidators();
		bool SaveValidators(ValueSaver &saver);
		bool SaveViewChange(ValueSaver &saver);
		bool SendMessage(const PbftEnvPointer &msg);
		void ClearViewChanges();
	public:
		Pbft();
		~Pbft();

		PbftEnvPointer IncPeerMessageRound(const protocol::PbftEnv &message, uint32_t round_number);

		virtual bool Initialize();
		virtual bool Exit();
		virtual bool Request(const std::string &value);
		virtual bool OnRecv(const ConsensusMsg &meesage);

		virtual void OnTimer(int64_t current_time);
		virtual void OnSlowTimer(int64_t current_time) {};
		virtual void OnTxTimeout();
		virtual size_t GetQuorumSize();
		virtual void GetModuleStatus(Json::Value &data);
		virtual int32_t IsLeader();
		virtual bool CheckProof(const protocol::ValidatorSet &validators, const std::string &previous_value_hash, const std::string &proof);
		virtual bool UpdateValidators(const protocol::ValidatorSet &validators, const std::string &proof);

		static int64_t GetSeq(const protocol::PbftEnv &pbft_env);
		static std::string GetNodeAddress(const protocol::PbftEnv &pbft_env);
		static std::vector<std::string> GetValue(const protocol::PbftEnv &pbft_env);
		static const char *GetPhaseDesc(PbftInstancePhase phase);
		static void ClearStatus();

	};

}

#endif
