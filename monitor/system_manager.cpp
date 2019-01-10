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

#include <utils/headers.h>
#include "system_manager.h"

namespace phantom {
	SystemManager::SystemManager()
		: system_(true) {
		cpu_used_percent_ = 0.0;
		check_interval_ = 5 * utils::MICRO_UNITS_PER_SEC;
		last_check_time_ = utils::Timestamp::HighResolution();
	}

	SystemManager::~SystemManager() {

	}

	void SystemManager::OnSlowTimer(int64_t current_time) {
		const utils::SystemProcessor processor = system_.GetProcessor();

		if (current_time - last_check_time_ > check_interval_) {
			if (!system_.UpdateProcessor()) {
				return;
			}
			last_check_time_ = current_time;
		}
		cpu_used_percent_ = processor.usage_percent_;
	}

	bool SystemManager::GetSystemMonitor(std::string paths, monitor::SystemStatus* &system_status) {

		monitor::SystemProperty *property = system_status->mutable_property();
		property->set_host_name(system_.GetHostName());
		property->set_os_version(system_.GetOsVersion());
		property->set_startup_time((int64_t)system_.GetStartupTime(0));
		property->set_os_bit(system_.GetOsBits());

		utils::PhysicalMemory physical_memory;
		if (false == system_.GetPhysicalMemory(physical_memory)) {
			LOG_ERROR("Common::SystemManager , Get physical memory status failed");
		}
		else {
			monitor::SystemResource *memory = system_status->mutable_memory();
			memory->set_available(physical_memory.available_bytes_);
			memory->set_total(physical_memory.total_bytes_);
			memory->set_used_percent(physical_memory.usage_percent_);
		}
		
		uint64_t total_bytes = 0;
		utils::PhysicalPartitionVector partition_vector;
		if (!system_.GetPhysicalPartition(total_bytes, partition_vector)) {
			LOG_ERROR("Common::SystemManager , Get physical Partitions status failed");
		}
		else {
			total_bytes = 0;
			monitor::Partition *disk = system_status->mutable_partitions();
			if (partition_vector.size() > 0) {
				utils::StringVector partitions_path = utils::String::split(paths, ",");
				utils::PhysicalPartitionVector partition_vector_found;
				bool is_bounded = false;
				for (uint32_t j = 0; j < partitions_path.size(); j++) {
					uint32_t i = 0;
					for (auto partition : partition_vector) {
						if (partitions_path.at(j).compare(partition.describe_) == 0) {
							is_bounded = true;
							partition_vector_found.push_back(partition);
							break;
						}
					}
				}
				if (is_bounded) {
					for (auto item : partition_vector_found) {
#ifdef WIN32
						if (item.describe_.at(1) == ':') {

#else
						if (item.describe_.at(0) == '/') {
#endif
							monitor::SystemResource *part = disk->add_partition();
							part->set_total(item.total_bytes_);
							part->set_available(item.available_bytes_);
							part->set_used_percent(item.usage_percent_);
							part->set_description(item.describe_);
							total_bytes += item.total_bytes_;
						}
					}
				}
				else {
					for (auto item : partition_vector) {
#ifdef WIN32
						if (item.describe_.compare("C:\\") == 0) {
#else
						if (item.describe_.compare("/") == 0) {
#endif
							monitor::SystemResource *part = disk->add_partition();
							part->set_total(item.total_bytes_);
							part->set_available(item.available_bytes_);
							part->set_used_percent(item.usage_percent_);
							part->set_description(item.describe_);
							total_bytes += item.total_bytes_;
							break;
						}
					}
				}
				disk->set_total_bytes(total_bytes);
			}
			else {
				LOG_ERROR("Common::SystemManager , Get physical Partitions status failed");
			}
		}

		system_status->mutable_cpu()->set_used_percent(cpu_used_percent_);

		return true;
	}
}