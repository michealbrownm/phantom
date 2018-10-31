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

#include "timer.h"

namespace utils {
	TimerElement::TimerElement(int64_t id, int64_t data, int64_t expire_time, std::function<void(int64_t)> const &func) :
		id_(id), data_(data), expire_time_(expire_time), func_(func) {}

	int64_t TimerElement::GetIndex() {
		return id_;
	}

	void TimerElement::Excute() {
		func_(data_);
	}

	Timer::Timer() :check_interval_(100 * utils::MICRO_UNITS_PER_MILLI),
		global_element_id_(1),
		last_check_time_(0) {}

	Timer::~Timer() {}

	bool Timer::Initialize() {
		return true;
	}

	bool Timer::Exit() {
		return true;
	}

	int64_t Timer::AddTimer(int64_t micro_time, int64_t data, std::function<void(int64_t)> const &func) {
		utils::MutexGuard guard(lock_);
		int64_t expire_time = utils::Timestamp::HighResolution() + micro_time;
		TimerElement element(global_element_id_++, data, expire_time, func);

		time_ele_.insert(std::make_pair(expire_time, element));
		return element.GetIndex();
	}

	bool Timer::DelTimer(int64_t index) {
		utils::MutexGuard guard(lock_);
		for (std::multimap<int64_t, TimerElement>::iterator iter = time_ele_.begin();
			iter != time_ele_.end();
			iter++) {
			TimerElement &ele = iter->second;
			if (ele.GetIndex() == index) {
				time_ele_.erase(iter);
				return true;
			}
		}

		return false;
	}

	void Timer::OnTimer(int64_t current_time) {
		if (current_time > last_check_time_ + check_interval_) {
			CheckExpire(current_time);

			for (std::list<TimerElement>::iterator iter = exeute_list_.begin(); iter != exeute_list_.end(); iter++) {
				iter->Excute();
			}
			exeute_list_.clear();

			last_check_time_ = current_time;
		}
	}

	void Timer::CheckExpire(int64_t cur_time) {
		utils::MutexGuard guard(lock_);
		for (std::multimap<int64_t, TimerElement>::iterator iter = time_ele_.begin();
			iter != time_ele_.end();
			) {
			TimerElement ele = iter->second;
			if (iter->first > cur_time) {
				break;
			}

			time_ele_.erase(iter++);
			exeute_list_.push_back(ele);
		}
	}

}
