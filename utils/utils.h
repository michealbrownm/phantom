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

#ifndef UTIS_UTILS_H_
#define UTIS_UTILS_H_

#include <stack>
#include "common.h"
#include "basen.h"

#ifdef OS_LINUX
#include <sys/sysinfo.h>
#elif defined OS_MAC
#include <sys/sysctl.h>
#endif

namespace utils {
	// Seconds
	static const int64_t  MILLI_UNITS_PER_SEC = 1000;
	static const int64_t  MICRO_UNITS_PER_MILLI = 1000;
	static const int64_t  NANO_UNITS_PER_MICRO = 1000;
	static const int64_t  MICRO_UNITS_PER_SEC = MICRO_UNITS_PER_MILLI * MILLI_UNITS_PER_SEC;
	static const int64_t  NANO_UNITS_PER_SEC = NANO_UNITS_PER_MICRO * MICRO_UNITS_PER_SEC;

	static const time_t SECOND_UNITS_PER_MINUTE = 60;
	static const time_t MINUTE_UNITS_PER_HOUR = 60;
	static const time_t HOUR_UNITS_PER_DAY = 24;
	static const time_t SECOND_UNITS_PER_HOUR = SECOND_UNITS_PER_MINUTE * MINUTE_UNITS_PER_HOUR;
	static const time_t SECOND_UNITS_PER_DAY = SECOND_UNITS_PER_HOUR * HOUR_UNITS_PER_DAY;

	static const size_t  BYTES_PER_KILO = 1024;
	static const size_t  KILO_PER_MEGA = 1024;
	static const size_t  BYTES_PER_MEGA = BYTES_PER_KILO * KILO_PER_MEGA;

	static const size_t MAX_OPERATIONS_NUM_PER_TRANSACTION = 100;

	static const uint16_t  MAX_UINT16 = 0xFFFF;
	static const uint32_t  MAX_UINT32 = 0xFFFFFFFF;
	static const int32_t   MAX_INT32 = 0x7FFFFFFF;
	static const int64_t   MAX_INT64 = 0x7FFFFFFFFFFFFFFF;

	// Low-high
	static const uint64_t LOW32_BITS_MASK = 0xffffffffULL;
	static const uint64_t HIGH32_BITS_MASK = 0xffffffff00000000ULL;

	static const size_t ETH_MAX_PACKET_SIZE = 1600;

	uint32_t error_code();
	void set_error_code(uint32_t code);

	std::string error_desc(uint32_t code = -1);

#ifdef WIN32
#define LOCK_CAS(mem, with, cmp) InterlockedCompareExchange(mem, with, cmp)
#define LOCK_YIELD()             SwitchToThread()
#elif defined OS_LINUX
#define LOCK_CAS(mem, with, cmp) __sync_val_compare_and_swap(mem, cmp, with)
#define LOCK_YIELD()             pthread_yield();
#elif defined OS_MAC
#define LOCK_CAS(mem, with, cmp) __sync_val_compare_and_swap(mem, cmp, with)
#define LOCK_YIELD()             pthread_yield_np();
#endif

#ifdef WIN32
	inline LONG AtomicInc(volatile LONG *value) {
		return InterlockedIncrement(value);
	}
#elif defined OS_LINUX
	inline int32_t AtomicInc(volatile int32_t *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}
#elif defined OS_MAC
	inline int32_t AtomicInc(volatile long *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}
#endif

#ifdef WIN32
	inline LONGLONG AtomicInc(volatile LONGLONG *value) {
		return InterlockedIncrement64(value);
	}
#elif defined OS_LINUX
	inline int64_t AtomicInc(volatile int64_t *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}
#elif defined OS_MAC
	inline int64_t AtomicInc(volatile int64_t *value) {
		__sync_fetch_and_add(value, 1);
		return *value;
	}
#endif

#ifdef WIN32
	inline LONG AtomicDec(volatile LONG *value) {
		return InterlockedDecrement(value);
	}
#elif defined OS_LINUX
	inline int32_t AtomicDec(volatile int32_t *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}
#elif defined OS_MAC
	inline int32_t AtomicDec(volatile long *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}
#endif

#ifdef WIN32
	inline LONGLONG AtomicDec(volatile LONGLONG *value) {
		return InterlockedDecrement64(value);
	}
#elif defined OS_LINUX
	inline int64_t AtomicDec(volatile int64_t *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}
#elif defined OS_MAC
	inline int64_t AtomicDec(volatile int64_t *value) {
		__sync_fetch_and_sub(value, 1);
		return *value;
	}
#endif


	template<typename T>
	class AtomicInteger {
	public:
		AtomicInteger()
			: value_(0) {}

		T Inc() {
			return AtomicInc(&value_);
		}
		T Dec() {
			return AtomicDec(&value_);
		}
		T value() const {
			return value_;
		}
	private:
		T value_;
	};

#ifdef WIN32
	typedef AtomicInteger<LONG> AtomicInt32;
	typedef AtomicInteger<LONGLONG> AtomicInt64;
#else
	typedef AtomicInteger<int32_t> AtomicInt32;
	typedef AtomicInteger<int64_t> AtomicInt64;
#endif

	void Sleep(int nMillSecs);

	size_t GetCpuCoreCount();
	time_t GetStartupTime(time_t time_now = 0);
	std::string GetCinPassword(const std::string &_prompt);

	void SetExceptionHandle();

#if __cplusplus >= 201402L || (defined(_MSC_VER) && _MSC_VER >= 1900)

	using std::make_unique;

#else
	template <typename T, typename... Args>
	std::unique_ptr<T>
		make_unique(Args&&... args) {
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}
#endif


	class ObjectExit {
	public:
		typedef std::function<bool()> ExitHandler;
		ObjectExit() {};
		~ObjectExit() {
			while (!s_.empty()) {
				s_.top()();
				s_.pop();
			}
		}
		void Push(ExitHandler fun) { s_.push(fun); };

	private:
		std::stack<ExitHandler> s_;
	};
}
#endif

