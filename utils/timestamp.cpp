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
#include "utils.h"
#include "timestamp.h"

#ifdef OS_MAC
#include <mach/clock.h>
#include <mach/mach.h>
#endif

utils::Timestamp::Timestamp()
	: timestamp_(0) {}

utils::Timestamp::Timestamp(int64_t usTimestamp)
	: timestamp_(usTimestamp) {}

utils::Timestamp::Timestamp(const Timestamp &ts)
	: timestamp_(ts.timestamp_) {}

std::string utils::Timestamp::ToString() const {
	char buf[32] = { 0 };
	int64_t seconds = timestamp_ / kMicroSecondsPerSecond;
	int64_t microseconds = timestamp_ % kMicroSecondsPerSecond;
#ifdef WIN32
	snprintf(buf, sizeof(buf), "%I64d.%06I64d", seconds, microseconds);
#else 
#ifdef __x86_64__
	snprintf(buf, sizeof(buf), "%ld.%06ld", seconds, microseconds);
#else 
	snprintf(buf, sizeof(buf), "%lld.%06lld", seconds, microseconds);
#endif
#endif	
	return buf;
}

std::string utils::Timestamp::ToFormatString(bool with_micro) const {
	char buf[32] = { 0 };
	time_t seconds = static_cast<time_t>(timestamp_ / kMicroSecondsPerSecond);
	int microseconds = static_cast<int>(timestamp_ % kMicroSecondsPerSecond);
	tm *tm_time = localtime(&seconds);

	if (with_micro) {
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06d",
			tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec,
			microseconds);
	}
	else {
		snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
			tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
			tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
	}
	return buf;
}

time_t utils::Timestamp::ToUnixTimestamp() const {
	return static_cast<time_t>(timestamp_ / kMicroSecondsPerSecond);
}

int64_t utils::Timestamp::timestamp() const {
	return timestamp_;
}

bool utils::Timestamp::Valid() const {
	return timestamp_ > 0;
}

utils::Timestamp utils::Timestamp::Now() {
#ifdef WIN32
	FILETIME ft;
	ULARGE_INTEGER ui;
	GetSystemTimeAsFileTime(&ft);
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;
	return Timestamp(static_cast<int64_t>(ui.QuadPart - 116444736000000000) / 10);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	int64_t seconds = tv.tv_sec;
	return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
#endif
}

int64_t utils::Timestamp::HighResolution() {
	static int64_t uprefertime = 0;
	int64_t uptime = 0;

#ifdef WIN32
	LARGE_INTEGER nFreq = { 0 }, nTicks = { 0 };
	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&nTicks);
	//int64 nUpTime = nFreq.QuadPart > 0 ? nTicks.QuadPart * utils::MICRO_UNITS_PER_SEC / nFreq.QuadPart : 0;
	uptime = nFreq.QuadPart > 0 ? (int64_t)(double(nTicks.QuadPart) / double(nFreq.QuadPart) * double(utils::MICRO_UNITS_PER_SEC)) : 0;
#elif defined OS_MAC
	struct timespec nTime = { 0, 0 };
	clock_serv_t cclock;
	mach_timespec_t mts;
	host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	nTime.tv_sec = mts.tv_sec;
	nTime.tv_nsec = mts.tv_nsec;
	uptime = (int64_t)(nTime.tv_sec) * utils::MICRO_UNITS_PER_SEC + (int64_t)(nTime.tv_nsec) / utils::NANO_UNITS_PER_MICRO;
#else
	struct timespec nTime = { 0, 0 };
	clock_gettime(CLOCK_MONOTONIC, &nTime);
	uptime = (int64_t)(nTime.tv_sec) * utils::MICRO_UNITS_PER_SEC + (int64_t)(nTime.tv_nsec) / utils::NANO_UNITS_PER_MICRO;
#endif //WIN32

	if (uprefertime == 0) {
		uprefertime = Now().timestamp() - uptime;
	}
	return uptime + uprefertime;
}

std::string utils::Timestamp::Format(bool milli_second) const {
	char buffer[256] = { 0 };
	struct tm time_info = { 0 };
	time_t time_value = (time_t)(timestamp_ / utils::MICRO_UNITS_PER_SEC);
	int    micro_part = (int)(timestamp_ % utils::MICRO_UNITS_PER_SEC);

	GetLocalTimestamp(time_value, time_info);

	if (milli_second) {
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
			time_info.tm_year + 1900,
			time_info.tm_mon + 1,
			time_info.tm_mday,
			time_info.tm_hour,
			time_info.tm_min,
			time_info.tm_sec,
			micro_part / 1000);
	}
	else {
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
			time_info.tm_year + 1900,
			time_info.tm_mon + 1,
			time_info.tm_mday,
			time_info.tm_hour,
			time_info.tm_min,
			time_info.tm_sec,
			micro_part);
	}

	return std::string(buffer);
}

bool utils::Timestamp::GetLocalTimestamp(time_t timestamp, struct tm &timevalue) {
#ifdef WIN32
	return localtime_s(&timevalue, &timestamp) == 0;
#else
	const time_t timestampTmp = timestamp;
	return localtime_r(&timestampTmp, &timevalue) != NULL;
#endif
}
