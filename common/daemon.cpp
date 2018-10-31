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

#include <utils/timestamp.h>
#include <utils/logger.h>
#include "daemon.h"

namespace utils {
	Daemon::Daemon() {
		last_write_time_ = 0;
		timer_name_ = "Daemon";
		shared = NULL;
	}

	Daemon::~Daemon() {}

	//void Daemon::GetModuleStatus(Json::Value &data) const {
	//}

	bool Daemon::Initialize(int32_t key) {
		phantom::TimerNotify::RegisterModule(this);
#ifdef WIN32

#else
		//Initialize mutex
		int fd;
		pthread_mutexattr_t mattr;
		fd = open("/dev/zero", O_RDWR, 0);
		mptr = (pthread_mutex_t*)mmap(0, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		close(fd);
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
		pthread_mutex_init(mptr, &mattr);

		//Allocate shared memory
		shmid = shmget((key_t)key, sizeof(int64_t), 0666 | IPC_CREAT);
		if (shmid == -1) {
			LOG_ERROR("Failed to initialize daemon, invalid shmget");
			return true;
		}
		//Attach the shared memory at the the current thread 
		shm = shmat(shmid, (void*)0, 0);
		if (shm == (void*)-1) {
			LOG_ERROR("Failed to initialize daemon, invalid shmget");
			return false;
		}
		LOG_INFO("Attached to shared memory address at %lx\n", (unsigned long int)shm);
		//Set the shared memory
		shared = (int64_t*)shm;

#endif
		return true;
	}

	bool Daemon::Exit() {
#ifdef WIN32
#else
		//Detach the shared memory from the current thread
		if (shmdt(shm) == -1) {
			LOG_ERROR("Failed to exit daemon,shmdt failed");
			return false;
		}
		return true;
#endif
		return true;
	}

	void Daemon::OnTimer(int64_t current_time) {
		//int64_t now_time = utils::Timestamp::GetLocalTimestamp(,);
		//int64_t now_time = utils::Timestamp::Now().timestamp();
#ifdef WIN32
#else
		if (current_time - last_write_time_ > 500000) {
			pthread_mutex_lock(mptr);
			if (shared) *shared = current_time;
			last_write_time_ = current_time;
			pthread_mutex_unlock(mptr);
		}
#endif
	}

	void Daemon::OnSlowTimer(int64_t current_time) {

	}
}
