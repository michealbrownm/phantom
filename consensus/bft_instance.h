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

#ifndef PBFT_INSTANCE_
#define PBFT_INSTANCE_

#include <utils/headers.h>

namespace phantom {
	typedef std::map<int64_t, protocol::PbftPrepare> PbftPrepareMap; //replica id => message
	typedef std::map<int64_t, protocol::PbftCommit> PbftCommitMap; //replica id => message
	typedef std::map<int64_t, protocol::PbftViewChange> PbftViewChangeMap; //replica id => message
	typedef std::vector<protocol::PbftEnv> PbftPhaseVector;
	typedef std::vector<PbftPhaseVector> PbftPhaseVector2;

	typedef std::shared_ptr<protocol::PbftEnv> PbftEnvPointer;

	const int64_t g_pbft_vcinstance_timeout_ = 60 * utils::MICRO_UNITS_PER_SEC;
	const int64_t g_pbft_instance_timeout_ = 30 * utils::MICRO_UNITS_PER_SEC;
	const int64_t g_pbft_commit_send_interval = 15 * utils::MICRO_UNITS_PER_SEC; // for retransmit
	const int64_t g_pbft_newview_send_interval = 15 * utils::MICRO_UNITS_PER_SEC; // for retransmit

	typedef enum PbftInstancePhaseTag {
		PBFT_PHASE_NONE,
		PBFT_PHASE_PREPREPARED,
		PBFT_PHASE_PREPARED,
		PBFT_PHASE_COMMITED,
		PBFT_PHASE_MAX
	}PbftInstancePhase;

	//Phase => message type
	//Phase           NONE          | PRE-PREPARED     | PREPARED | COMMITED
	//message type    PRE-PREPARE   | PREPARE          | COMMIT   | REPLY


	//pbft sequence    1  2  3  4  5  6  7  8  9  10
	//pbft checkpoint              5              10

	class PbftInstanceIndex {
	public:
		PbftInstanceIndex(int64_t view_number, int64_t sequence);
		~PbftInstanceIndex();

		int64_t view_number_;
		int64_t sequence_;

		bool operator < (const PbftInstanceIndex &index) const;
	};

	class Pbft;
	class PbftInstance {
	public:
		PbftInstance();
		~PbftInstance();
		PbftInstancePhase  phase_;
		size_t phase_item_;

		protocol::PbftPrePrepare pre_prepare_;
		PbftPrepareMap prepares_;
		PbftCommitMap commits_;

		PbftPhaseVector2 msg_buf_;
		protocol::PbftEnv pre_prepare_msg_;

		int64_t start_time_;
		int64_t end_time_;
		int64_t last_propose_time_;
		int64_t last_commit_send_time_;
		bool have_send_viewchange_;

		uint32_t pre_prepare_round_;
		uint32_t commit_round_;

		int32_t check_value_result_;

		bool Go(const protocol::PbftEnv &env, Pbft *pbft, int32_t check_value);
		bool IsExpire(int64_t current_time);
		bool NeedSendAgain(int64_t current_time);
		bool NeedSendCommitAgain(int64_t current_time);
		void SetLastProposeTime(int64_t current_time);
		void SetLastCommitSendTime(int64_t current_time);
		bool SendPrepareAgain(Pbft *pbft, int64_t current_time);
	};

	typedef std::map<PbftInstanceIndex, PbftInstance> PbftInstanceMap;

	class PbftCkpInstanceIndex {
	public:
		PbftCkpInstanceIndex();
		PbftCkpInstanceIndex(int64_t seq, const std::string &state_digest);
		~PbftCkpInstanceIndex();

		int64_t sequence_;
		std::string state_digest_;

		bool operator < (const PbftCkpInstanceIndex &index) const;
	};

	class PbftVcInstance {
	public:
		PbftVcInstance();
		PbftVcInstance(int64_t view_number);
		~PbftVcInstance();

		protocol::PbftEnv view_change_msg_;
		int64_t view_number_;
		int64_t seq;
		PbftViewChangeMap viewchanges_;
		protocol::PbftPreparedSet pre_prepared_env_set; // Last prepared related pre-prepared env
		uint32_t view_change_round_;

		PbftPhaseVector msg_buf_; //View change message

		int64_t start_time_;
		int64_t last_propose_time_;
		int64_t end_time_;

		int64_t last_newview_time_;
		protocol::PbftEnv newview_;
		uint32_t new_view_round_;
		bool ShouldTeminated(int64_t current_time, int64_t time_out);
		bool NeedSendAgain(int64_t current_time);
		void SetLastProposeTime(int64_t current_time);

		bool NeedSendNewViewAgain(int64_t current_time);
		bool SendNewViewAgain(Pbft *pbft, int64_t current_time);
		bool SendNewView(Pbft *pbft, int64_t current_time, PbftEnvPointer new_ptr);

		void ChangeComplete(int64_t current_time);

	private:
		void SetLastNewViewTime(int64_t current_time);
	};
	typedef std::map<int64_t, PbftVcInstance> PbftVcInstanceMap; // view_number => map

	class PbftDesc {
	public:
		static std::string GetPbft(const protocol::Pbft &message);
		static std::string GetPrePrepare( const protocol::PbftPrePrepare &pre_prepare);
		static std::string GetPrepare(const protocol::PbftPrepare &prepare);
		static std::string GetCommit(const protocol::PbftCommit &commit);
		static std::string GetViewChange(const protocol::PbftViewChange &viewchange);
		static std::string GetViewChangeRawValue(const protocol::PbftViewChangeWithRawValue &viewchange_raw);
		static std::string GetNewView(const protocol::PbftNewView &newview);
		static const char *GetMessageTypeDesc(enum protocol::PbftMessageType type);

		static const char *VALIDATORS;
		static const char *VIEW_ACTIVE;
		static const char *SEQUENCE_NAME;
		static const char *VIEWNUMBER_NAME;
		static const char *CHECKPOINT_NAME;
		static const char *LAST_EXE_SEQUENCE_NAME;
		static const char *LOW_WATER_MRAK_NAME;
		static const char *INSTANCE_NAME;
		static const char *VIEW_CHANGE_NAME;
	};
}

#endif
