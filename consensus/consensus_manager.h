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

#ifndef CONSENSUS_MANAGER_H_
#define CONSENSUS_MANAGER_H_

#include <utils/singleton.h>
#include <utils/net.h>
#include <common/general.h>
#include <main/configure.h>
#include "bft.h"

namespace phantom {

	class ConsensusManager : public utils::Singleton<ConsensusManager>,
		public TimerNotify,
		public StatusModule {
		friend class utils::Singleton<ConsensusManager>;
	private:
		ConsensusManager();
		~ConsensusManager();

		std::shared_ptr<Consensus> consensus_;
	public:
		bool Initialize(const std::string &validation_type);
		bool Exit();
		std::shared_ptr<Consensus> GetConsensus();

		virtual void OnTimer(int64_t current_time);
		virtual void OnSlowTimer(int64_t current_time);
		virtual void GetModuleStatus(Json::Value &data);
	};

}

#endif