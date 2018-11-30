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

#ifndef SYSTEM_MANAGER_H_
#define SYSTEM_MANAGER_H_

#include <utils/system.h>
#include <common/general.h>
#include <proto/cpp/monitor.pb.h>

namespace phantom {
	class SystemManager {
	public:
		SystemManager();
		~SystemManager();

	public:
		void OnSlowTimer(int64_t current_time);

		bool GetSystemMonitor(std::string paths, monitor::SystemStatus* &system_status);

	private:
		utils::System system_;      // os
		double cpu_used_percent_;   // cpu percentage
		int64_t check_interval_;    // timer interval
		int64_t last_check_time_;   // last check time
	};
}

#endif
