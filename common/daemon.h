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

#ifndef  DAEMON_H_
#define  DAEMON_H_

#include <utils/singleton.h>
#include <common/general.h>
#ifdef WIN32
#else 
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include<sys/mman.h>
#endif
#define TEXT_SZ 2048

namespace utils {
	class Daemon : public utils::Singleton<utils::Daemon>, public phantom::TimerNotify {
		friend class utils::Singleton<utils::Daemon>;
	private:
		Daemon();
		~Daemon();
		pthread_mutex_t *mptr;

		int64_t last_write_time_;

		int running;
		void *shm;
		int64_t* shared;
		int shmid;

	public:
		bool Initialize(int32_t key);
		bool Exit();
		void OnTimer(int64_t current_time);

		virtual void OnSlowTimer(int64_t current_time);
		void GetModuleStatus(Json::Value &data) {

		};
		//virtual void GetModuleStatus(Json::Value &data) const;
	};
}
#endif
